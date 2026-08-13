// SPDX-License-Identifier: GPL-2.0-only

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/kvm.h>

#include "kvm_util.h"
#include "test_util.h"

static void assert_ioctl_errno(int fd, unsigned long request, void *arg,
			       int expected_errno)
{
	int ret;

	errno = 0;
	ret = ioctl(fd, request, arg);
	TEST_ASSERT(ret == -1 && errno == expected_errno,
		    "ioctl %#lx returned %d/%d, expected -1/%d",
		    request, ret, errno, expected_errno);
}

static void assert_ioctl_noarg_errno(int fd, unsigned long request,
				     int expected_errno)
{
	int ret;

	errno = 0;
	ret = ioctl(fd, request, 0);
	TEST_ASSERT(ret == -1 && errno == expected_errno,
		    "ioctl %#lx returned %d/%d, expected -1/%d",
		    request, ret, errno, expected_errno);
}

static int create_context_with_features(int kvm_fd, __u64 required_features)
{
	struct kvm_protected_task_create create = {
		.size = sizeof(create),
		.required_features = required_features,
	};
	int fd;

	fd = ioctl(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create);
	TEST_ASSERT(fd >= 0, KVM_IOCTL_ERROR(KVM_CREATE_PROTECTED_TASK, fd));
	TEST_ASSERT_EQ(create.supported_features,
		       KVM_PROTECTED_TASK_FEATURE_EXEC);
	TEST_ASSERT(fcntl(fd, F_GETFD) & FD_CLOEXEC,
		    "Protected-task context fd must be close-on-exec");
	return fd;
}

static int create_context(int kvm_fd)
{
	return create_context_with_features(kvm_fd, 0);
}

static struct kvm_protected_task_info get_info(int fd)
{
	struct kvm_protected_task_info info = {
		.size = sizeof(info),
	};
	int ret;

	ret = ioctl(fd, KVM_PT_GET_INFO, &info);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_GET_INFO, ret));
	return info;
}

struct protected_avx_features {
	bool avx;
	bool fma;
	bool f16c;
	bool avx2;
	bool avx512;
	bool avx512dq;
	bool avx512ifma;
	bool avx512pf;
	bool avx512er;
	bool avx512cd;
	bool avx512bw;
	bool avx512vl;
	bool avx512vbmi;
	bool avx512vbmi2;
	bool gfni;
	bool vaes;
	bool vpclmulqdq;
	bool avx512vnni;
	bool avx512bitalg;
	bool avx512vpopcntdq;
	bool avx5124vnniw;
	bool avx5124fmaps;
	bool avx512vp2intersect;
	bool avx512fp16;
	bool avx512bf16;
};

static bool kvm_supported_xstate_component(
		const struct kvm_cpuid_entry2 *entry, __u32 size, __u32 *end)
{
	return entry->eax == size && entry->ebx >= 576 &&
		!(entry->ecx & ((1U << 0) | (1U << 2))) &&
		!__builtin_add_overflow(entry->ebx, entry->eax, end);
}

static struct protected_avx_features get_kvm_supported_avx_features(int kvm_fd)
{
	struct {
		__u32 nent;
		__u32 padding;
		struct kvm_cpuid_entry2 entries[256];
	} cpuid = {
		.nent = 256,
	};
	struct protected_avx_features features = {};
	bool avx = false, avx512 = false, avx512_xstate = false;
	bool hi16_zmm = false, opmask = false, ymm = false;
	bool ymm_component = false, zmm_hi256 = false;
	__u32 hi16_zmm_end = 0, opmask_end = 0, ymm_end = 0;
	__u32 zmm_hi256_end = 0;
	unsigned int i;
	int ret;

	ret = ioctl(kvm_fd, KVM_GET_SUPPORTED_CPUID, &cpuid);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_GET_SUPPORTED_CPUID, ret));
	for (i = 0; i < cpuid.nent; i++) {
		struct kvm_cpuid_entry2 *entry = &cpuid.entries[i];

		if (entry->function == 1 && !entry->index) {
			avx = (entry->ecx & ((1U << 26) | (1U << 28))) ==
				((1U << 26) | (1U << 28));
			features.fma = entry->ecx & (1U << 12);
			features.f16c = entry->ecx & (1U << 29);
		} else if (entry->function == 7 && !entry->index) {
			features.avx2 = entry->ebx & (1U << 5);
			avx512 = entry->ebx & (1U << 16);
			features.avx512dq = entry->ebx & (1U << 17);
			features.avx512ifma = entry->ebx & (1U << 21);
			features.avx512pf = entry->ebx & (1U << 26);
			features.avx512er = entry->ebx & (1U << 27);
			features.avx512cd = entry->ebx & (1U << 28);
			features.avx512bw = entry->ebx & (1U << 30);
			features.avx512vl = entry->ebx & (1U << 31);
			features.avx512vbmi = entry->ecx & (1U << 1);
			features.avx512vbmi2 = entry->ecx & (1U << 6);
			features.gfni = entry->ecx & (1U << 8);
			features.vaes = entry->ecx & (1U << 9);
			features.vpclmulqdq = entry->ecx & (1U << 10);
			features.avx512vnni = entry->ecx & (1U << 11);
			features.avx512bitalg = entry->ecx & (1U << 12);
			features.avx512vpopcntdq = entry->ecx & (1U << 14);
			features.avx5124vnniw = entry->edx & (1U << 2);
			features.avx5124fmaps = entry->edx & (1U << 3);
			features.avx512vp2intersect = entry->edx & (1U << 8);
			features.avx512fp16 = entry->edx & (1U << 23);
		} else if (entry->function == 7 && entry->index == 1) {
			features.avx512bf16 = entry->eax & (1U << 5);
		} else if (entry->function == 0xd && !entry->index) {
			ymm = entry->eax & (1U << 2);
			avx512_xstate = (entry->eax & (0xe0U)) == 0xe0U;
		} else if (entry->function == 0xd && entry->index == 2) {
			ymm_component = kvm_supported_xstate_component(entry, 256,
								 &ymm_end);
		} else if (entry->function == 0xd && entry->index == 5) {
			opmask = kvm_supported_xstate_component(entry, 64,
							       &opmask_end);
		} else if (entry->function == 0xd && entry->index == 6) {
			zmm_hi256 = kvm_supported_xstate_component(entry, 512,
								  &zmm_hi256_end);
		} else if (entry->function == 0xd && entry->index == 7) {
			hi16_zmm = kvm_supported_xstate_component(entry, 1024,
								 &hi16_zmm_end);
		}
	}

	features.avx = avx && ymm && ymm_component;
	features.avx512 = features.avx && avx512 && avx512_xstate &&
		opmask && zmm_hi256 && hi16_zmm &&
		opmask_end - 64 >= ymm_end &&
		zmm_hi256_end - 512 >= opmask_end &&
		hi16_zmm_end - 1024 >= zmm_hi256_end;
	if (!features.avx)
		features.fma = features.f16c = features.avx2 = false;
	if (!features.avx512)
		features.avx512dq = features.avx512ifma =
			features.avx512pf = features.avx512er =
			features.avx512cd = features.avx512bw =
			features.avx512vl = features.avx512vbmi =
			features.avx512vbmi2 = features.gfni =
			features.vaes = features.vpclmulqdq =
			features.avx512vnni = features.avx512bitalg =
			features.avx512vpopcntdq = features.avx5124vnniw =
			features.avx5124fmaps = features.avx512vp2intersect =
			features.avx512fp16 = features.avx512bf16 = false;

	return features;
}

static void append_expected_output(
		char *output, size_t output_size, size_t *length,
		const char *message)
{
	size_t message_length = strlen(message);

	TEST_ASSERT(message_length <= output_size - *length,
		    "Protected-task expected output is too long");
	memcpy(output + *length, message, message_length);
	*length += message_length;
}

static void append_expected_avx_profile(
		char *output, size_t output_size, size_t *length,
		const struct protected_avx_features *features)
{
	if (features->avx)
		append_expected_output(output, output_size, length,
				       "protected task avx\n");
	if (features->fma)
		append_expected_output(output, output_size, length,
				       "protected task fma\n");
	if (features->f16c)
		append_expected_output(output, output_size, length,
				       "protected task f16c\n");
	if (features->avx2)
		append_expected_output(output, output_size, length,
				       "protected task avx2\n");
	if (features->avx512)
		append_expected_output(output, output_size, length,
				       "protected task avx512\n");
	if (features->avx512dq)
		append_expected_output(output, output_size, length,
				       "protected task avx512dq\n");
	if (features->avx512ifma)
		append_expected_output(output, output_size, length,
				       "protected task avx512ifma\n");
	if (features->avx512pf)
		append_expected_output(output, output_size, length,
				       "protected task avx512pf\n");
	if (features->avx512er)
		append_expected_output(output, output_size, length,
				       "protected task avx512er\n");
	if (features->avx512cd)
		append_expected_output(output, output_size, length,
				       "protected task avx512cd\n");
	if (features->avx512bw)
		append_expected_output(output, output_size, length,
				       "protected task avx512bw\n");
	if (features->avx512vl)
		append_expected_output(output, output_size, length,
				       "protected task avx512vl\n");
	if (features->avx512vbmi)
		append_expected_output(output, output_size, length,
				       "protected task avx512vbmi\n");
	if (features->avx512vbmi2)
		append_expected_output(output, output_size, length,
				       "protected task avx512vbmi2\n");
	if (features->gfni)
		append_expected_output(output, output_size, length,
				       "protected task gfni\n");
	if (features->vaes)
		append_expected_output(output, output_size, length,
				       "protected task vaes\n");
	if (features->vpclmulqdq)
		append_expected_output(output, output_size, length,
				       "protected task vpclmulqdq\n");
	if (features->avx512vnni)
		append_expected_output(output, output_size, length,
				       "protected task avx512vnni\n");
	if (features->avx512bitalg)
		append_expected_output(output, output_size, length,
				       "protected task avx512bitalg\n");
	if (features->avx512vpopcntdq)
		append_expected_output(output, output_size, length,
				       "protected task avx512vpopcntdq\n");
	if (features->avx5124vnniw)
		append_expected_output(output, output_size, length,
				       "protected task avx5124vnniw\n");
	if (features->avx5124fmaps)
		append_expected_output(output, output_size, length,
				       "protected task avx5124fmaps\n");
	if (features->avx512vp2intersect)
		append_expected_output(output, output_size, length,
				       "protected task avx512vp2intersect\n");
	if (features->avx512fp16)
		append_expected_output(output, output_size, length,
				       "protected task avx512fp16\n");
	if (features->avx512bf16)
		append_expected_output(output, output_size, length,
				       "protected task avx512bf16\n");
}

static void test_create_validation(int kvm_fd)
{
	struct {
		struct kvm_protected_task_create create;
		__u64 extra;
	} extended = {
		.create.size = sizeof(extended),
	};
	struct kvm_protected_task_create create = {
		.size = offsetof(struct kvm_protected_task_create,
				 supported_features),
	};
	int fd;

	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create, EINVAL);

	create.size = sizeof(create);
	create.flags = 1;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create, EINVAL);

	create.flags = 0;
	create.reserved[0] = 1;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create, EINVAL);

	create.reserved[0] = 0;
	create.supported_features = 1;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create, EINVAL);

	create.supported_features = 0;
	create.required_features = KVM_PROTECTED_TASK_FEATURE_EXEC;
	fd = ioctl(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create);
	TEST_ASSERT(fd >= 0, KVM_IOCTL_ERROR(KVM_CREATE_PROTECTED_TASK, fd));
	TEST_ASSERT_EQ(create.supported_features,
		       KVM_PROTECTED_TASK_FEATURE_EXEC);
	close(fd);

	create.supported_features = 0;
	create.required_features = 1ULL << 63;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create,
			   EOPNOTSUPP);
	TEST_ASSERT_EQ(create.supported_features,
		       KVM_PROTECTED_TASK_FEATURE_EXEC);

	extended.extra = 1;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &extended, E2BIG);

	extended.extra = 0;
	fd = ioctl(kvm_fd, KVM_CREATE_PROTECTED_TASK, &extended);
	TEST_ASSERT(fd >= 0, KVM_IOCTL_ERROR(KVM_CREATE_PROTECTED_TASK, fd));
	close(fd);
}

static void test_context_validation(int fd)
{
	struct kvm_protected_task_info info = {
		.size = sizeof(info),
		.features = 1,
	};
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm.size),
	};

	assert_ioctl_errno(fd, KVM_PT_GET_INFO, &info, EINVAL);
	info.features = 0;
	info.context_id = 1;
	assert_ioctl_errno(fd, KVM_PT_GET_INFO, &info, EINVAL);
	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EINVAL);

	arm.size = sizeof(arm);
	arm.flags = 1;
	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EINVAL);

	arm.flags = 0;
	arm.reserved[0] = 1;
	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EINVAL);
	assert_ioctl_noarg_errno(fd, KVM_PT_CANCEL_ARM, ENOENT);
}

static pthread_barrier_t thread_barrier;

static void barrier_wait(void)
{
	int ret;

	ret = pthread_barrier_wait(&thread_barrier);
	TEST_ASSERT(ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD,
		    "pthread_barrier_wait() failed: %d", ret);
}

static void *thread_fn(void *arg)
{
	barrier_wait();
	barrier_wait();
	return NULL;
}

static void test_multithreaded_arm(int fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	pthread_t thread;
	int ret;

	ret = pthread_barrier_init(&thread_barrier, NULL, 2);
	TEST_ASSERT(ret == 0, "pthread_barrier_init() failed: %d", ret);
	ret = pthread_create(&thread, NULL, thread_fn, NULL);
	TEST_ASSERT(ret == 0, "pthread_create() failed: %d", ret);

	barrier_wait();
	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EBUSY);
	barrier_wait();

	ret = pthread_join(thread, NULL);
	TEST_ASSERT(ret == 0, "pthread_join() failed: %d", ret);
	ret = pthread_barrier_destroy(&thread_barrier);
	TEST_ASSERT(ret == 0, "pthread_barrier_destroy() failed: %d", ret);
}

struct clone_vm_sync {
	int ready[2];
	int done[2];
};

static int clone_vm_fn(void *arg)
{
	struct clone_vm_sync *sync = arg;
	char value = 0;

	TEST_ASSERT(write(sync->ready[1], &value, sizeof(value)) == sizeof(value),
		    "Failed to signal CLONE_VM readiness: %d", errno);
	TEST_ASSERT(read(sync->done[0], &value, sizeof(value)) == sizeof(value),
		    "Failed to wait for CLONE_VM completion: %d", errno);
	return 0;
}

static void test_shared_mm_arm(int fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct clone_vm_sync sync;
	const size_t stack_size = 1 << 20;
	char value;
	void *stack;
	int status;
	pid_t child;

	TEST_ASSERT(pipe(sync.ready) == 0, "pipe() failed: %d", errno);
	TEST_ASSERT(pipe(sync.done) == 0, "pipe() failed: %d", errno);
	stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	TEST_ASSERT(stack != MAP_FAILED, "mmap() failed: %d", errno);

	child = clone(clone_vm_fn, stack + stack_size, CLONE_VM | SIGCHLD, &sync);
	TEST_ASSERT(child >= 0, "clone(CLONE_VM) failed: %d", errno);
	TEST_ASSERT(read(sync.ready[0], &value, sizeof(value)) == sizeof(value),
		    "Failed to wait for CLONE_VM readiness: %d", errno);

	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EBUSY);

	TEST_ASSERT(write(sync.done[1], &value, sizeof(value)) == sizeof(value),
		    "Failed to release CLONE_VM child: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "CLONE_VM child failed: %#x", status);
	TEST_ASSERT(munmap(stack, stack_size) == 0, "munmap() failed: %d", errno);

	close(sync.ready[0]);
	close(sync.ready[1]);
	close(sync.done[0]);
	close(sync.done[1]);
}

static void test_fork_does_not_inherit_arm(int fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct kvm_protected_task_info info;
	int status, ret;
	pid_t child;

	ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		info = get_info(fd);
		_exit(info.flags ? 1 : 0);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Child inherited protected-task arm state: %#x", status);

	info = get_info(fd);
	TEST_ASSERT(info.flags & KVM_PROTECTED_TASK_INFO_ARMED,
		    "Parent lost protected-task arm state across fork");
	ret = ioctl(fd, KVM_PT_CANCEL_ARM, 0);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_CANCEL_ARM, ret));
}

static void test_arm_cancel_and_exec(int fd, int other_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct kvm_protected_task_info info;
	char *const argv[] = { "/bin/false", NULL };
	int ret;

	ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
	info = get_info(fd);
	TEST_ASSERT(info.flags & KVM_PROTECTED_TASK_INFO_ARMED,
		    "Context is not reported as armed");

	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EBUSY);
	assert_ioctl_noarg_errno(other_fd, KVM_PT_CANCEL_ARM, ENOENT);

	ret = ioctl(fd, KVM_PT_CANCEL_ARM, 0);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_CANCEL_ARM, ret));
	TEST_ASSERT(!(get_info(fd).flags & KVM_PROTECTED_TASK_INFO_ARMED),
		    "Context remains armed after cancellation");

	ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
	errno = 0;
	ret = execv(argv[0], argv);
	TEST_ASSERT(ret == -1 && errno == EOPNOTSUPP,
		    "Armed exec returned %d/%d, expected -1/%d",
		    ret, errno, EOPNOTSUPP);
	TEST_ASSERT(!(get_info(fd).flags & KVM_PROTECTED_TASK_INFO_ARMED),
		    "Failed exec did not consume one-shot arm state");
}

static void test_close_while_armed(int fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	char *const argv[] = { "/bin/false", NULL };
	int ret;

	ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
	close(fd);

	errno = 0;
	ret = execv(argv[0], argv);
	TEST_ASSERT(ret == -1 && errno == EOPNOTSUPP,
		    "Exec after closing armed context returned %d/%d, expected -1/%d",
		    ret, errno, EOPNOTSUPP);
}

static void test_io_permissions_reject_exec(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret, saved_errno;

		if (ioperm(0x80, 1, 1) < 0) {
			TEST_ASSERT(errno == EPERM,
				    "ioperm() failed: %d", errno);
			_exit(0);
		}

		fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		errno = 0;
		ret = execl("/proc/self/exe", "protected_task_test",
			    "--io-permission-exec-target", NULL);
		saved_errno = errno;
		TEST_ASSERT(ioperm(0x80, 1, 0) == 0,
			    "ioperm() cleanup failed: %d", errno);
		TEST_ASSERT(ret == -1 && saved_errno == EOPNOTSUPP,
			    "Protected exec with I/O permissions returned %d/%d, expected -1/%d",
			    ret, saved_errno, EOPNOTSUPP);
		TEST_ASSERT(!(get_info(fd).flags & KVM_PROTECTED_TASK_INFO_ARMED),
			    "Rejected I/O-permission exec did not consume arm state");
		close(fd);
		_exit(0);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "I/O-permission exec rejection failed: %#x", status);
}

static void get_exec_helper_path(char path[PATH_MAX])
{
	static const char helper[] = "protected_task_exec";
	char *name;
	ssize_t length;

	length = readlink("/proc/self/exe", path, PATH_MAX - 1);
	TEST_ASSERT(length > 0, "readlink(/proc/self/exe) failed: %d", errno);
	path[length] = '\0';
	name = strrchr(path, '/');
	TEST_ASSERT(name, "Selftest executable path has no directory");
	name++;
	TEST_ASSERT(sizeof(helper) <= PATH_MAX - (name - path),
		    "Protected exec helper path is too long");
	memcpy(name, helper, sizeof(helper));
}

static size_t get_hidden_mappings(pid_t pid, unsigned long *addresses,
				  size_t max_addresses)
{
	char path[64], line[256], permissions[5], extra;
	unsigned long address, end, inode;
	size_t n = 0;
	FILE *maps;

	snprintf(path, sizeof(path), "/proc/%d/maps", pid);
	maps = fopen(path, "re");
	TEST_ASSERT(maps, "fopen(%s) failed: %d", path, errno);
	while (fgets(line, sizeof(line), maps)) {
		int fields;

		fields = sscanf(line, "%lx-%lx %4s %*s %*s %lu %c",
				&address, &end, permissions, &inode, &extra);
		if (fields != 4 || strcmp(permissions, "---p") || inode ||
		    end - address <= 1UL << 20)
			continue;
		TEST_ASSERT(n < max_addresses,
			    "Too many hidden mappings in task %d", pid);
		addresses[n++] = address;
	}
	TEST_ASSERT(fclose(maps) == 0, "fclose(%s) failed: %d", path, errno);
	return n;
}

static void test_hidden_mapping_ptrace(pid_t child, unsigned long address)
{
	long value;
	int status;

	TEST_ASSERT(ptrace(PTRACE_ATTACH, child, NULL, NULL) == 0,
		    "PTRACE_ATTACH failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after PTRACE_ATTACH failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status), "Ptraced child did not stop: %#x", status);

	errno = 0;
	value = ptrace(PTRACE_PEEKDATA, child, (void *)address, NULL);
	TEST_ASSERT(value == -1 && errno == EIO,
		    "PTRACE_PEEKDATA returned %ld/%d, expected -1/%d",
		    value, errno, EIO);
	TEST_ASSERT(ptrace(PTRACE_DETACH, child, NULL, NULL) == 0,
		    "PTRACE_DETACH failed: %d", errno);
}

static void test_hardware_breakpoint_ptrace(pid_t child, unsigned long address,
					    int address_fd)
{
	struct user_regs_struct regs;
	siginfo_t siginfo;
	long dr6, value;
	int status;

	TEST_ASSERT(ptrace(PTRACE_ATTACH, child, NULL, NULL) == 0,
		    "PTRACE_ATTACH failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after PTRACE_ATTACH failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status), "Ptraced child did not stop: %#x", status);

	errno = 0;
	value = ptrace(PTRACE_PEEKDATA, child, (void *)address, NULL);
	TEST_ASSERT(value == -1 && errno == EIO,
		    "PTRACE_PEEKDATA returned %ld/%d, expected -1/%d",
		    value, errno, EIO);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_POKEUSER, child,
			   (void *)offsetof(struct user, u_debugreg[0]),
			   regs.rip) == 0,
		    "PTRACE_POKEUSER DR0 failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_POKEUSER, child,
			   (void *)offsetof(struct user, u_debugreg[7]), 1) == 0,
		    "PTRACE_POKEUSER DR7 failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT failed: %d", errno);
	TEST_ASSERT(write(address_fd, &address, sizeof(address)) == sizeof(address),
		    "Failed to release hardware-breakpoint target: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after hardware breakpoint failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
		    "Hardware breakpoint produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0,
		    "PTRACE_GETSIGINFO failed: %d", errno);
	TEST_ASSERT(siginfo.si_signo == SIGTRAP && siginfo.si_code == TRAP_HWBKPT,
		    "Hardware breakpoint produced signal %d/%d",
		    siginfo.si_signo, siginfo.si_code);
	errno = 0;
	dr6 = ptrace(PTRACE_PEEKUSER, child,
		     (void *)offsetof(struct user, u_debugreg[6]), NULL);
	TEST_ASSERT(dr6 != -1 && (dr6 & 1),
		    "Hardware breakpoint DR6 is %#lx/%d", dr6, errno);
	TEST_ASSERT(ptrace(PTRACE_POKEUSER, child,
			   (void *)offsetof(struct user, u_debugreg[7]), 0) == 0,
		    "Clearing DR7 failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_POKEUSER, child,
			   (void *)offsetof(struct user, u_debugreg[0]), 0) == 0,
		    "Clearing DR0 failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_DETACH, child, NULL, NULL) == 0,
		    "PTRACE_DETACH failed: %d", errno);
}

static void test_protected_exec(int kvm_fd)
{
	enum {
		ADDRESS_FD = 100,
		READY_FD = 101,
		CONTROL_FD = 102,
	};
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct protected_avx_features features;
	char expected_output[1024] = {}, output[sizeof(expected_output)] = {};
	char helper[PATH_MAX];
	unsigned long main_address = 0;
	size_t expected_length = 0, nread = 0;
	int address_pipe[2], pipefd[2], ready_pipe[2];
	int status;
	pid_t child;

	features = get_kvm_supported_avx_features(kvm_fd);
	append_expected_avx_profile(expected_output, sizeof(expected_output),
				    &expected_length, &features);
	append_expected_output(expected_output, sizeof(expected_output),
			       &expected_length, "protected task exec\n");
	append_expected_avx_profile(expected_output, sizeof(expected_output),
				    &expected_length, &features);
	get_exec_helper_path(helper);
	TEST_ASSERT(pipe(pipefd) == 0, "pipe() failed: %d", errno);
	TEST_ASSERT(pipe(address_pipe) == 0, "pipe() failed: %d", errno);
	TEST_ASSERT(pipe(ready_pipe) == 0, "pipe() failed: %d", errno);
	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		close(pipefd[0]);
		close(address_pipe[1]);
		close(ready_pipe[0]);
		TEST_ASSERT(dup2(pipefd[1], STDOUT_FILENO) == STDOUT_FILENO,
			    "dup2() failed: %d", errno);
		TEST_ASSERT(dup2(address_pipe[0], ADDRESS_FD) == ADDRESS_FD,
			    "dup2() failed: %d", errno);
		TEST_ASSERT(dup2(ready_pipe[1], READY_FD) == READY_FD,
			    "dup2() failed: %d", errno);
		close(pipefd[1]);
		close(address_pipe[0]);
		close(ready_pipe[1]);
		fd = create_context_with_features(kvm_fd, KVM_PROTECTED_TASK_FEATURE_EXEC);
		if (fd != CONTROL_FD) {
			TEST_ASSERT(dup3(fd, CONTROL_FD, O_CLOEXEC) == CONTROL_FD,
				    "dup3() failed: %d", errno);
			close(fd);
			fd = CONTROL_FD;
		}
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl(helper, helper, NULL);
		_exit(127);
	}

	close(pipefd[1]);
	close(address_pipe[0]);
	close(ready_pipe[1]);
	for (int i = 0; i < 12; i++) {
		unsigned long address, addresses[2];
		ssize_t nread;
		size_t n;
		pid_t target;
		int stage = i % 6;

		do {
			errno = 0;
			nread = read(ready_pipe[0], &target, sizeof(target));
		} while (nread < 0 && errno == EINTR);
		if (nread != sizeof(target)) {
			int saved_errno = errno;

			TEST_ASSERT(waitpid(child, &status, 0) == child,
				    "waitpid() after checkpoint failure failed: %d", errno);
			TEST_FAIL("Protected helper stopped before checkpoint %d: read=%zd errno=%d status=%#x",
				  i, nread, saved_errno, status);
		}
		n = get_hidden_mappings(target, addresses,
					sizeof(addresses) / sizeof(addresses[0]));
		if (stage == 0) {
			TEST_ASSERT(target == child && n == 1,
				    "Initial helper has unexpected mappings");
			main_address = address = addresses[0];
		} else if (stage == 1 || stage == 3) {
			TEST_ASSERT(target != child && n == 1,
				    "Process child has unexpected mappings");
			address = addresses[0];
		} else if (stage == 2) {
			TEST_ASSERT(target != child && n == 2,
				    "vfork child has unexpected mappings");
			address = addresses[addresses[0] == main_address];
			TEST_ASSERT(address != main_address,
				    "vfork hidden mapping is not distinct");
		} else if (stage == 4) {
			TEST_ASSERT(target == child && n == 2,
				    "Thread did not create a second hidden mapping");
			address = addresses[addresses[0] == main_address];
			TEST_ASSERT(address != main_address,
				    "Thread hidden mapping is not distinct");
		} else {
			TEST_ASSERT(target == child && n == 1,
				    "Post-thread helper %d has %zu hidden mappings, expected task %d with one",
				    target, n, child);
			address = addresses[0];
		}
		if (stage == 0)
			test_hardware_breakpoint_ptrace(target, address,
						 address_pipe[1]);
		else {
			test_hidden_mapping_ptrace(target, address);
			TEST_ASSERT(write(address_pipe[1], &address, sizeof(address)) == sizeof(address),
				    "Failed to send hidden mapping address: %d", errno);
		}
	}
	close(address_pipe[1]);
	close(ready_pipe[0]);
	while (nread < sizeof(output)) {
		ssize_t n;

		n = read(pipefd[0], output + nread, sizeof(output) - nread);
		if (n < 0 && errno == EINTR)
			continue;
		TEST_ASSERT(n >= 0, "read() failed: %d", errno);
		if (!n)
			break;
		nread += n;
	}
	close(pipefd[0]);

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Protected exec helper failed: %#x", status);
	TEST_ASSERT(nread == expected_length &&
		    !memcmp(output, expected_output, expected_length),
		    "Unexpected protected exec output");
}

int main(int argc, char *argv[])
{
	struct kvm_protected_task_info first_info, second_info;
	int kvm_fd, first_fd, second_fd;

	if (argc == 2 && !strcmp(argv[1], "--io-permission-exec-target"))
		return 42;

	kvm_fd = open_kvm_dev_path_or_exit();
	TEST_REQUIRE(ioctl(kvm_fd, KVM_CHECK_EXTENSION,
			   KVM_CAP_PROTECTED_TASK) == 1);

	test_create_validation(kvm_fd);
	first_fd = create_context(kvm_fd);
	second_fd = create_context(kvm_fd);

	first_info = get_info(first_fd);
	second_info = get_info(second_fd);
	TEST_ASSERT(first_info.context_id && second_info.context_id &&
		    first_info.context_id != second_info.context_id,
		    "Protected-task context IDs are not unique");

	test_context_validation(first_fd);
	test_multithreaded_arm(first_fd);
	test_shared_mm_arm(first_fd);
	test_fork_does_not_inherit_arm(first_fd);
	test_arm_cancel_and_exec(first_fd, second_fd);

	close(first_fd);
	test_close_while_armed(second_fd);
	test_io_permissions_reject_exec(kvm_fd);
	test_protected_exec(kvm_fd);
	close(kvm_fd);
	return 0;
}
