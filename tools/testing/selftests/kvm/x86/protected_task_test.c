// SPDX-License-Identifier: GPL-2.0-only

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
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

static int create_context(int kvm_fd)
{
	struct kvm_protected_task_create create = {
		.size = sizeof(create),
	};
	int fd;

	fd = ioctl(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create);
	TEST_ASSERT(fd >= 0, KVM_IOCTL_ERROR(KVM_CREATE_PROTECTED_TASK, fd));
	TEST_ASSERT_EQ(create.supported_features, 0);
	TEST_ASSERT(fcntl(fd, F_GETFD) & FD_CLOEXEC,
		    "Protected-task context fd must be close-on-exec");
	return fd;
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
	create.required_features = 1;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create,
			   EOPNOTSUPP);
	TEST_ASSERT_EQ(create.supported_features, 0);

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

int main(int argc, char *argv[])
{
	struct kvm_protected_task_info first_info, second_info;
	int kvm_fd, first_fd, second_fd;

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
	close(kvm_fd);
	return 0;
}
