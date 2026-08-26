// SPDX-License-Identifier: GPL-2.0-only

#include <cpuid.h>
#include <dirent.h>
#include <dlfcn.h>
#include <elf.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/auxv.h>
#include <sys/rseq.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/xattr.h>
#include <ucontext.h>
#include <unistd.h>

#include <asm/prctl.h>
#include <linux/bpf.h>
#include <linux/capability.h>
#include <linux/futex.h>
#include <linux/io_uring.h>
#include <linux/kvm.h>
#include <linux/landlock.h>
#include <linux/mempolicy.h>
#include <linux/perf_event.h>
#include <linux/ptrace.h>
#include <linux/sched.h>
#include <linux/seccomp.h>
#include <linux/securebits.h>
#include <linux/userfaultfd.h>
#include <uapi/linux/filter.h>

#include "kvm_util.h"
#include "test_util.h"
#include "cgroup_util.h"

static void get_exec_helper_path(char path[PATH_MAX]);

extern void process_observability_payload(unsigned long iterations);
extern char process_observability_payload_end[];

asm(
".pushsection .text\n"
".balign 64\n"
".globl process_observability_payload\n"
".type process_observability_payload,@function\n"
"process_observability_payload:\n"
"xor %eax,%eax\n"
"1:\n"
"add %rdi,%rax\n"
"dec %rdi\n"
"jnz 1b\n"
"ret\n"
".size process_observability_payload,.-process_observability_payload\n"
".globl process_observability_payload_end\n"
"process_observability_payload_end:\n"
".popsection\n"
);

enum {
	PROCESS_OBSERVABILITY_READY_FD = 100,
	PROCESS_OBSERVABILITY_CONTROL_FD,
	PROCESS_OBSERVABILITY_RESULT_FD,
	PROCESS_OBSERVABILITY_CALLS = 16,
	PROCESS_OBSERVABILITY_ITERATIONS = 2000000,
};

struct process_observability_target {
	unsigned long payload_start;
	unsigned long payload_end;
	pid_t pid;
	pid_t tid;
	char comm[16];
};

struct process_observability_result {
	uint64_t elapsed_ns;
};

static int protected_entry_probe(void)
{
	unsigned int eax, ebx, ecx, edx;

	__cpuid_count(7, 0, eax, ebx, ecx, edx);
	return !!(ebx & bit_FSGSBASE);
}

static void assert_protected_entry(void)
{
	if (protected_entry_probe())
		_exit(203);
}

static int run_process_failure_signal_target(void)
{
	struct sigaction action = {
		.sa_handler = SIG_DFL,
	};

	sigemptyset(&action.sa_mask);
	if (sigaction(SIGILL, &action, NULL))
		return 1;
	asm volatile("ud2");
	return 2;
}

static int run_process_failure_engine_target(void)
{
	unsigned int eax, ebx, ecx, edx;
	char vendor[13] = {};

	assert_protected_entry();
	if (prctl(PR_SET_NAME, "pt-failure", 0, 0, 0))
		return 1;
	__cpuid(0, eax, ebx, ecx, edx);
	memcpy(vendor, &ebx, sizeof(ebx));
	memcpy(vendor + 4, &edx, sizeof(edx));
	memcpy(vendor + 8, &ecx, sizeof(ecx));
	if (!strcmp(vendor, "AuthenticAMD"))
		asm volatile("vmmcall" ::: "memory");
	else if (!strcmp(vendor, "GenuineIntel"))
		asm volatile("vmcall" ::: "memory");
	else
		return 2;
	return 3;
}

static volatile sig_atomic_t protected_entry_signal_seen;

static void protected_entry_signal_handler(int signal)
{
	if (signal != SIGUSR1 || protected_entry_probe())
		_exit(204);
	protected_entry_signal_seen = 1;
}

static int run_entry_restart(void)
{
	struct sigaction action = {
		.sa_handler = protected_entry_signal_handler,
		.sa_flags = SA_RESTART,
	};
	const struct timespec delay = { .tv_nsec = 20 * 1000 * 1000 };
	char value = 0;
	int pipefd[2], status;
	pid_t child, parent = getpid();

	sigemptyset(&action.sa_mask);
	if (sigaction(SIGUSR1, &action, NULL) || pipe(pipefd))
		return 1;

	protected_entry_signal_seen = 0;
	child = fork();
	if (child < 0)
		return 1;
	if (!child) {
		close(pipefd[0]);
		assert_protected_entry();
		if (nanosleep(&delay, NULL) || kill(parent, SIGUSR1) ||
		    nanosleep(&delay, NULL) ||
		    write(pipefd[1], &value, sizeof(value)) != sizeof(value))
			_exit(1);
		_exit(0);
	}

	close(pipefd[1]);
	assert_protected_entry();
	if (read(pipefd[0], &value, sizeof(value)) != sizeof(value))
		return 1;
	assert_protected_entry();
	if (!protected_entry_signal_seen || waitpid(child, &status, 0) != child ||
	    !WIFEXITED(status) || WEXITSTATUS(status))
		return 1;
	close(pipefd[0]);
	return 0;
}

static int run_entry_task_work(void)
{
	const uint64_t user_data = 0x0123456789abcdef;
	struct io_uring_params params = {};
	struct io_uring_sqe *sqes = MAP_FAILED;
	struct io_uring_cqe *cqe;
	void *cq_ring = MAP_FAILED, *sq_ring = MAP_FAILED;
	size_t cq_ring_size, sq_ring_size, sqes_size;
	unsigned int *cq_head, *cq_tail, *sq_array, *sq_mask, *sq_tail;
	unsigned int index, tail;
	int fd = -1, ret = 1;

	fd = syscall(SYS_io_uring_setup, 2, &params);
	if (fd < 0)
		goto out;

	sq_ring_size = params.sq_off.array +
		params.sq_entries * sizeof(*sq_array);
	cq_ring_size = params.cq_off.cqes +
		params.cq_entries * sizeof(*cqe);
	if (params.features & IORING_FEAT_SINGLE_MMAP) {
		if (cq_ring_size > sq_ring_size)
			sq_ring_size = cq_ring_size;
		cq_ring_size = sq_ring_size;
	}
	sqes_size = params.sq_entries * sizeof(*sqes);

	sq_ring = mmap(NULL, sq_ring_size, PROT_READ | PROT_WRITE,
		       MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_SQ_RING);
	if (sq_ring == MAP_FAILED)
		goto out;
	if (params.features & IORING_FEAT_SINGLE_MMAP)
		cq_ring = sq_ring;
	else {
		cq_ring = mmap(NULL, cq_ring_size, PROT_READ | PROT_WRITE,
			       MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_CQ_RING);
		if (cq_ring == MAP_FAILED)
			goto out;
	}
	sqes = mmap(NULL, sqes_size, PROT_READ | PROT_WRITE,
		    MAP_SHARED | MAP_POPULATE, fd, IORING_OFF_SQES);
	if (sqes == MAP_FAILED)
		goto out;

	sq_tail = sq_ring + params.sq_off.tail;
	sq_mask = sq_ring + params.sq_off.ring_mask;
	sq_array = sq_ring + params.sq_off.array;
	tail = __atomic_load_n(sq_tail, __ATOMIC_RELAXED);
	index = tail & *sq_mask;
	memset(&sqes[index], 0, sizeof(sqes[index]));
	sqes[index].opcode = IORING_OP_NOP;
	sqes[index].nop_flags = IORING_NOP_TW;
	sqes[index].user_data = user_data;
	sq_array[index] = index;
	__atomic_store_n(sq_tail, tail + 1, __ATOMIC_RELEASE);

	if (syscall(SYS_io_uring_enter, fd, 1, 0, 0, NULL, 0) != 1)
		goto out;
	assert_protected_entry();

	cq_head = cq_ring + params.cq_off.head;
	cq_tail = cq_ring + params.cq_off.tail;
	if (__atomic_load_n(cq_head, __ATOMIC_RELAXED) ==
	    __atomic_load_n(cq_tail, __ATOMIC_ACQUIRE))
		goto out;
	cqe = cq_ring + params.cq_off.cqes;
	cqe += *cq_head & *(unsigned int *)(cq_ring + params.cq_off.ring_mask);
	if (cqe->user_data != user_data || cqe->res)
		goto out;
	__atomic_store_n(cq_head, *cq_head + 1, __ATOMIC_RELEASE);
	ret = 0;

out:
	if (sqes != MAP_FAILED)
		munmap(sqes, sqes_size);
	if (cq_ring != MAP_FAILED && cq_ring != sq_ring)
		munmap(cq_ring, cq_ring_size);
	if (sq_ring != MAP_FAILED)
		munmap(sq_ring, sq_ring_size);
	if (fd >= 0)
		close(fd);
	return ret;
}

static int run_entry_target(void)
{
	assert_protected_entry();
	if (raise(SIGSTOP))
		return 1;
	assert_protected_entry();
	if (syscall(SYS_sched_yield))
		return 1;
	assert_protected_entry();
	if (run_entry_restart())
		return 1;
	return run_entry_task_work();
}

enum protected_rseq_action {
	PROTECTED_RSEQ_PREEMPT = 1,
	PROTECTED_RSEQ_MIGRATE,
	PROTECTED_RSEQ_SIGNAL,
	PROTECTED_RSEQ_FORK,
	PROTECTED_RSEQ_EXEC,
	PROTECTED_RSEQ_EXIT,
};

struct protected_rseq_args {
	void *data;
	long result;
	int aborted;
};

struct protected_rseq_preempt {
	int start;
	int done;
};

struct protected_rseq_migration {
	int start;
	int done;
	int ready;
	int result;
	pid_t tid;
	size_t size;
	cpu_set_t mask;
};

struct protected_rseq_signal {
	int start;
	int result;
	volatile sig_atomic_t *seen;
	pid_t tgid;
	pid_t tid;
};

struct protected_rseq_fork {
	int state;
	int release;
	size_t size;
	cpu_set_t mask;
};

struct protected_rseq_exec {
	const char *path;
	char *const *argv;
	char *const *envp;
};

extern const struct rseq_cs protected_rseq_cs;
extern int protected_rseq_action(struct rseq *rseq, int action,
				 struct protected_rseq_args *args);

_Static_assert(offsetof(struct protected_rseq_args, data) == 0);
_Static_assert(offsetof(struct protected_rseq_args, result) == 8);
_Static_assert(offsetof(struct protected_rseq_args, aborted) == 16);
_Static_assert(offsetof(struct protected_rseq_preempt, start) == 0);
_Static_assert(offsetof(struct protected_rseq_preempt, done) == 4);
_Static_assert(offsetof(struct protected_rseq_migration, start) == 0);
_Static_assert(offsetof(struct protected_rseq_migration, done) == 4);
_Static_assert(offsetof(struct protected_rseq_migration, size) == 24);
_Static_assert(offsetof(struct protected_rseq_migration, mask) == 32);
_Static_assert(offsetof(struct protected_rseq_signal, start) == 0);
_Static_assert(offsetof(struct protected_rseq_signal, seen) == 8);
_Static_assert(offsetof(struct protected_rseq_signal, tgid) == 16);
_Static_assert(offsetof(struct protected_rseq_signal, tid) == 20);
_Static_assert(offsetof(struct protected_rseq_fork, state) == 0);
_Static_assert(offsetof(struct protected_rseq_fork, release) == 4);
_Static_assert(offsetof(struct protected_rseq_fork, size) == 8);
_Static_assert(offsetof(struct protected_rseq_fork, mask) == 16);
_Static_assert(offsetof(struct protected_rseq_exec, path) == 0);
_Static_assert(offsetof(struct protected_rseq_exec, argv) == 8);
_Static_assert(offsetof(struct protected_rseq_exec, envp) == 16);

asm(
".pushsection __rseq_cs, \"aw\"\n"
".balign 32\n"
".global protected_rseq_cs\n"
".type protected_rseq_cs, @object\n"
"protected_rseq_cs:\n"
".long 0, 0\n"
".quad .Lprotected_rseq_start\n"
".quad .Lprotected_rseq_post_commit - .Lprotected_rseq_start\n"
".quad .Lprotected_rseq_abort\n"
".size protected_rseq_cs, .-protected_rseq_cs\n"
".popsection\n"
".pushsection __rseq_cs_ptr_array, \"aw\"\n"
".quad protected_rseq_cs\n"
".popsection\n"
".pushsection .text\n"
".balign 16\n"
".global protected_rseq_action\n"
".type protected_rseq_action, @function\n"
"protected_rseq_action:\n"
"mov %rdi, %r8\n"
"mov %rdx, %r9\n"
"lea protected_rseq_cs(%rip), %rax\n"
"mov %rax, 8(%r8)\n"
"movl $0, 16(%r9)\n"
"mov $-11, %rax\n"
".Lprotected_rseq_start:\n"
"cmp $1, %esi\n"
"je .Lprotected_rseq_preempt\n"
"cmp $2, %esi\n"
"je .Lprotected_rseq_migrate\n"
"cmp $3, %esi\n"
"je .Lprotected_rseq_signal\n"
"cmp $4, %esi\n"
"je .Lprotected_rseq_fork\n"
"cmp $5, %esi\n"
"je .Lprotected_rseq_exec\n"
"cmp $6, %esi\n"
"je .Lprotected_rseq_exit\n"
"mov $-22, %rax\n"
"jmp .Lprotected_rseq_post_commit\n"
".Lprotected_rseq_preempt:\n"
"mov 0(%r9), %r10\n"
"xor %eax, %eax\n"
"movl $1, 0(%r10)\n"
".Lprotected_rseq_preempt_wait:\n"
"pause\n"
"cmpl $0, 4(%r10)\n"
"je .Lprotected_rseq_preempt_wait\n"
"jmp .Lprotected_rseq_post_commit\n"
".Lprotected_rseq_migrate:\n"
"mov 0(%r9), %r10\n"
"xor %eax, %eax\n"
"movl $1, 0(%r10)\n"
".Lprotected_rseq_migrate_wait:\n"
"pause\n"
"cmpl $0, 4(%r10)\n"
"je .Lprotected_rseq_migrate_wait\n"
"jmp .Lprotected_rseq_post_commit\n"
".Lprotected_rseq_signal:\n"
"mov 0(%r9), %r10\n"
"xor %eax, %eax\n"
"movl $1, 0(%r10)\n"
"mov 8(%r10), %r11\n"
".Lprotected_rseq_signal_wait:\n"
"pause\n"
"cmpl $0, 0(%r11)\n"
"je .Lprotected_rseq_signal_wait\n"
"jmp .Lprotected_rseq_post_commit\n"
".Lprotected_rseq_fork:\n"
"mov $57, %eax\n"
"syscall\n"
"test %rax, %rax\n"
"jne .Lprotected_rseq_post_commit\n"
"mov 0(%r9), %r10\n"
"movl $1, 0(%r10)\n"
".Lprotected_rseq_fork_wait:\n"
"pause\n"
"cmpl $0, 4(%r10)\n"
"je .Lprotected_rseq_fork_wait\n"
"jmp .Lprotected_rseq_post_commit\n"
".Lprotected_rseq_exec:\n"
"mov 0(%r9), %r10\n"
"mov 0(%r10), %rdi\n"
"mov 8(%r10), %rsi\n"
"mov 16(%r10), %rdx\n"
"mov $59, %eax\n"
"syscall\n"
"jmp .Lprotected_rseq_post_commit\n"
".Lprotected_rseq_exit:\n"
"xor %edi, %edi\n"
"mov $60, %eax\n"
"syscall\n"
"ud2\n"
".Lprotected_rseq_post_commit:\n"
"mov %rax, 8(%r9)\n"
"movq $0, 8(%r8)\n"
"xor %eax, %eax\n"
"ret\n"
".pushsection __rseq_failure, \"ax\"\n"
".byte 0x0f, 0xb9, 0x3d\n"
".long 0x53053053\n"
".Lprotected_rseq_abort:\n"
"movl $1, 16(%r9)\n"
"mov %rax, 8(%r9)\n"
"movq $0, 8(%r8)\n"
"xor %eax, %eax\n"
"ret\n"
".popsection\n"
".size protected_rseq_action, .-protected_rseq_action\n"
".popsection\n"
);

static struct rseq *current_rseq(void)
{
	if (__rseq_size < offsetof(struct rseq, rseq_cs) + sizeof(uint64_t) ||
	    __rseq_flags)
		return NULL;
	return (void *)((char *)__builtin_thread_pointer() + __rseq_offset);
}

static volatile sig_atomic_t protected_rseq_signal_seen;

static void protected_rseq_signal_handler(int signal)
{
	if (signal != SIGUSR1)
		_exit(211);
	protected_rseq_signal_seen = 1;
}

static int validate_current_rseq(void)
{
	struct rseq *rseq = current_rseq();

	return !rseq || rseq->rseq_cs ||
		rseq->cpu_id == RSEQ_CPU_ID_UNINITIALIZED ||
		rseq->cpu_id == RSEQ_CPU_ID_REGISTRATION_FAILED;
}

static void *protected_rseq_preempt_thread(void *arg)
{
	struct protected_rseq_preempt *preempt = arg;

	while (!__atomic_load_n(&preempt->start, __ATOMIC_ACQUIRE))
		sched_yield();
	__atomic_store_n(&preempt->done, 1, __ATOMIC_RELEASE);
	return NULL;
}

static void *protected_rseq_migration_thread(void *arg)
{
	struct protected_rseq_migration *migration = arg;

	if (sched_setaffinity(0, migration->size, &migration->mask))
		migration->result = -errno;
	__atomic_store_n(&migration->ready, 1, __ATOMIC_RELEASE);
	if (migration->result)
		return NULL;

	while (!__atomic_load_n(&migration->start, __ATOMIC_ACQUIRE))
		sched_yield();
	if (sched_setaffinity(migration->tid, migration->size, &migration->mask))
		migration->result = -errno;
	__atomic_store_n(&migration->done, 1, __ATOMIC_RELEASE);
	return NULL;
}

static void *protected_rseq_signal_thread(void *arg)
{
	struct protected_rseq_signal *signal_data = arg;

	while (!__atomic_load_n(&signal_data->start, __ATOMIC_ACQUIRE))
		sched_yield();
	if (syscall(SYS_tgkill, signal_data->tgid, signal_data->tid, SIGUSR1)) {
		signal_data->result = -errno;
		__atomic_store_n(signal_data->seen, 1, __ATOMIC_RELEASE);
	}
	return NULL;
}

static int run_protected_rseq_tests(void)
{
	struct protected_rseq_preempt preempt = {};
	struct protected_rseq_migration migration = {};
	struct protected_rseq_fork *fork_data;
	struct protected_rseq_signal signal_data = {
		.seen = &protected_rseq_signal_seen,
		.tgid = getpid(),
		.tid = syscall(SYS_gettid),
	};
	struct protected_rseq_args args = {};
	struct sigaction action = {
		.sa_handler = protected_rseq_signal_handler,
	};
	cpu_set_t original, single;
	pthread_t thread;
	struct rseq *rseq;
	int cpus[2], force_errno = 0, nr_cpus = 0, restore_ret, ret, status;

	if (validate_current_rseq() || sched_getaffinity(0, sizeof(original),
						    &original))
		return 1;
	for (int cpu = 0; cpu < CPU_SETSIZE && nr_cpus < ARRAY_SIZE(cpus); cpu++)
		if (CPU_ISSET(cpu, &original))
			cpus[nr_cpus++] = cpu;
	if (!nr_cpus)
		return 2;

	CPU_ZERO(&single);
	CPU_SET(cpus[0], &single);
	if (sched_setaffinity(0, sizeof(single), &single))
		return 3;
	rseq = current_rseq();
	if (pthread_create(&thread, NULL, protected_rseq_preempt_thread, &preempt))
		return 4;
	args.data = &preempt;
	ret = protected_rseq_action(rseq, PROTECTED_RSEQ_PREEMPT, &args);
	__atomic_store_n(&preempt.start, 1, __ATOMIC_RELEASE);
	if (pthread_join(thread, NULL))
		return 4;
	if (ret)
		return 5;
	if (!args.aborted)
		return 27;
	if (args.result)
		return 28;
	if (!__atomic_load_n(&preempt.done, __ATOMIC_ACQUIRE))
		return 29;

	if (nr_cpus > 1) {
		migration.tid = syscall(SYS_gettid);
		migration.size = sizeof(migration.mask);
		CPU_ZERO(&migration.mask);
		CPU_SET(cpus[1], &migration.mask);
		if (pthread_create(&thread, NULL, protected_rseq_migration_thread,
				   &migration))
			return 6;
		while (!__atomic_load_n(&migration.ready, __ATOMIC_ACQUIRE))
			sched_yield();
		if (migration.result) {
			pthread_join(thread, NULL);
			return 6;
		}
		args = (struct protected_rseq_args) { .data = &migration };
		ret = protected_rseq_action(rseq, PROTECTED_RSEQ_MIGRATE, &args);
		__atomic_store_n(&migration.start, 1, __ATOMIC_RELEASE);
		if (pthread_join(thread, NULL) || ret)
			return 7;
		if (!args.aborted)
			return 13;
		if (args.result || migration.result)
			return 14;
		if (sched_getcpu() != cpus[1])
			return 15;
	}
	if (sched_setaffinity(0, sizeof(original), &original))
		return 8;

	sigemptyset(&action.sa_mask);
	if (sigaction(SIGUSR1, &action, NULL))
		return 9;
	protected_rseq_signal_seen = 0;
	if (pthread_create(&thread, NULL, protected_rseq_signal_thread,
			   &signal_data))
		return 10;
	args = (struct protected_rseq_args) { .data = &signal_data };
	ret = protected_rseq_action(rseq, PROTECTED_RSEQ_SIGNAL, &args);
	__atomic_store_n(&signal_data.start, 1, __ATOMIC_RELEASE);
	if (pthread_join(thread, NULL) || ret)
		return 10;
	if (!args.aborted)
		return 16;
	if (args.result || signal_data.result)
		return 17;
	if (!protected_rseq_signal_seen)
		return 18;

	fork_data = mmap(NULL, sizeof(*fork_data), PROT_READ | PROT_WRITE,
			 MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (fork_data == MAP_FAILED)
		return 19;
	fork_data->size = sizeof(fork_data->mask);
	CPU_ZERO(&fork_data->mask);
	if (nr_cpus > 1)
		CPU_SET(cpus[1], &fork_data->mask);
	if (sched_setaffinity(0, sizeof(single), &single)) {
		munmap(fork_data, sizeof(*fork_data));
		return 19;
	}

	args = (struct protected_rseq_args) { .data = fork_data };
	ret = protected_rseq_action(rseq, PROTECTED_RSEQ_FORK, &args);
	if (!args.result) {
		__atomic_store_n(&fork_data->state, 2, __ATOMIC_RELEASE);
		if (!args.aborted)
			_exit(1);
		_exit(validate_current_rseq() ? 2 : 0);
	}
	if (ret || args.result < 0) {
		sched_setaffinity(0, sizeof(original), &original);
		munmap(fork_data, sizeof(*fork_data));
		return ret ? 11 : 12;
	}

	while (!__atomic_load_n(&fork_data->state, __ATOMIC_ACQUIRE))
		sched_yield();
	if (__atomic_load_n(&fork_data->state, __ATOMIC_ACQUIRE) == 1) {
		if (nr_cpus > 1)
			ret = sched_setaffinity(args.result, fork_data->size,
					    &fork_data->mask);
		else
			ret = syscall(SYS_tgkill, args.result, args.result, SIGUSR1);
		if (ret)
			force_errno = errno;
	}
	__atomic_store_n(&fork_data->release, 1, __ATOMIC_RELEASE);
	ret = waitpid(args.result, &status, 0);
	restore_ret = sched_setaffinity(0, sizeof(original), &original);
	if (munmap(fork_data, sizeof(*fork_data)))
		return 26;
	if (force_errno && force_errno != ESRCH)
		return 24;
	if (restore_ret)
		return 25;
	if (ret != args.result)
		return 19;
	if (!WIFEXITED(status))
		return 20;
	if (WEXITSTATUS(status))
		return 20 + WEXITSTATUS(status);
	if (rseq->rseq_cs)
		return 23;
	return 0;
}

static void *protected_rseq_exit_thread(void *arg)
{
	struct protected_rseq_args args = {};
	struct rseq *rseq = current_rseq();

	if (!rseq || rseq->rseq_cs) {
		*(int *)arg = 1;
		return NULL;
	}
	protected_rseq_action(rseq, PROTECTED_RSEQ_EXIT, &args);
	*(int *)arg = 2;
	return NULL;
}

static void *protected_rseq_reuse_thread(void *arg)
{
	*(int *)arg = validate_current_rseq();
	return NULL;
}

struct protected_cancel_state {
	pthread_mutex_t mutex;
	atomic_bool ready;
};

struct protected_robust_entry {
	struct robust_list list;
	int futex;
};

struct protected_fatal_state {
	struct robust_list_head head;
	struct protected_robust_entry entry;
	atomic_bool ready;
	atomic_int clear_tid;
};

static int init_robust_mutex(pthread_mutex_t *mutex, bool shared)
{
	pthread_mutexattr_t attr;
	int ret;

	ret = pthread_mutexattr_init(&attr);
	if (ret)
		return ret;
	ret = pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
	if (!ret && shared)
		ret = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
	if (!ret)
		ret = pthread_mutex_init(mutex, &attr);
	if (pthread_mutexattr_destroy(&attr) && !ret)
		ret = EINVAL;
	return ret;
}

static void *protected_cancel_thread(void *arg)
{
	struct protected_cancel_state *state = arg;

	if (pthread_mutex_lock(&state->mutex))
		return (void *)1;
	atomic_store_explicit(&state->ready, true, memory_order_release);
	for (;;) {
		pthread_testcancel();
		sched_yield();
	}
	return NULL;
}

static int run_robust_cancel_test(void)
{
	struct protected_cancel_state state = {};
	void *result;
	pthread_t thread;
	int ret;

	ret = init_robust_mutex(&state.mutex, false);
	if (ret || pthread_create(&thread, NULL, protected_cancel_thread, &state))
		return 1;
	while (!atomic_load_explicit(&state.ready, memory_order_acquire))
		sched_yield();
	if (pthread_cancel(thread) || pthread_join(thread, &result) ||
	    result != PTHREAD_CANCELED)
		return 2;
	ret = pthread_mutex_lock(&state.mutex);
	if (ret != EOWNERDEAD || pthread_mutex_consistent(&state.mutex) ||
	    pthread_mutex_unlock(&state.mutex) ||
	    pthread_mutex_destroy(&state.mutex))
		return 3;
	return 0;
}

static int protected_fatal_child(void *arg)
{
	const struct timespec delay = { .tv_nsec = 20 * 1000 * 1000 };
	struct protected_fatal_state *state = arg;
	pid_t tid = syscall(SYS_gettid);

	atomic_store_explicit(&state->clear_tid, tid, memory_order_release);
	__atomic_store_n(&state->entry.futex, tid, __ATOMIC_RELEASE);
	if (syscall(SYS_set_tid_address, &state->clear_tid) != tid ||
	    syscall(SYS_set_robust_list, &state->head, sizeof(state->head)))
		return 1;
	atomic_store_explicit(&state->ready, true, memory_order_release);
	if (syscall(SYS_nanosleep, &delay, NULL) ||
	    syscall(SYS_tgkill, syscall(SYS_getpid), tid, SIGKILL))
		return 2;
	return 3;
}

static int run_robust_fatal_test(void)
{
	const size_t stack_size = 1 << 20;
	struct protected_fatal_state *state;
	void *stack;
	int ret, status, tid;
	pid_t child;

	state = mmap(NULL, sizeof(*state), PROT_READ | PROT_WRITE,
		     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (state == MAP_FAILED)
		return 1;
	memset(state, 0, sizeof(*state));
	state->head.list.next = &state->entry.list;
	state->head.futex_offset = (char *)&state->entry.futex -
				   (char *)&state->entry.list;
	state->entry.list.next = &state->head.list;
	stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	if (stack == MAP_FAILED)
		return 2;

	child = clone(protected_fatal_child, stack + stack_size,
		      CLONE_VM | SIGCHLD, state);
	if (child < 0)
		return 3;

	while (!atomic_load_explicit(&state->ready, memory_order_acquire))
		sched_yield();
	tid = atomic_load_explicit(&state->clear_tid, memory_order_acquire);
	do {
		ret = syscall(SYS_futex, &state->clear_tid, FUTEX_WAIT, tid,
			      NULL, NULL, 0);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0 && (errno != EAGAIN ||
			atomic_load_explicit(&state->clear_tid,
					     memory_order_acquire)))
		return 4;
	if (waitpid(child, &status, 0) != child || !WIFSIGNALED(status) ||
	    WTERMSIG(status) != SIGKILL ||
	    atomic_load_explicit(&state->clear_tid, memory_order_acquire))
		return 5;
	if (__atomic_load_n(&state->entry.futex, __ATOMIC_ACQUIRE) !=
	    FUTEX_OWNER_DIED ||
	    munmap(stack, stack_size) ||
	    munmap(state, sizeof(*state)))
		return 6;
	return 0;
}

static int run_process_rseq_target(bool reexec)
{
	struct protected_rseq_exec exec_data;
	struct protected_rseq_args args;
	char *const argv[] = {
		"protected_task_test",
		"--process-rseq-reexec-target",
		NULL,
	};
	extern char **environ;
	pthread_t thread;
	int result = 0;

	if (validate_current_rseq())
		return 20;
	if (reexec)
		return 0;
	result = run_protected_rseq_tests();
	if (result)
		return 20 + result;

	if (pthread_create(&thread, NULL, protected_rseq_exit_thread, &result) ||
	    pthread_join(thread, NULL) || result)
		return 40;
	if (pthread_create(&thread, NULL, protected_rseq_reuse_thread, &result) ||
	    pthread_join(thread, NULL) || result)
		return 41;
	result = run_robust_cancel_test();
	if (result)
		return 50 + result;
	result = run_robust_fatal_test();
	if (result)
		return 60 + result;

	exec_data = (struct protected_rseq_exec) {
		.path = "/proc/self/exe",
		.argv = argv,
		.envp = environ,
	};
	args = (struct protected_rseq_args) { .data = &exec_data };
	protected_rseq_action(current_rseq(), PROTECTED_RSEQ_EXEC, &args);
	return 42;
}

enum {
	PROCESS_LIFECYCLE_WORKERS = 64,
	PROCESS_LIFECYCLE_CANCEL_WORKERS = 8,
	PROCESS_LIFECYCLE_FORK_WORKERS = 16,
	PROCESS_LIFECYCLE_STACK_SIZE = 1 << 20,
};

struct process_lifecycle_group {
	bool protected;
	atomic_int workers_ready;
	atomic_int nested_ready;
	atomic_int cancel_ready;
	atomic_bool release;
	atomic_bool failed;
};

struct process_lifecycle_exec {
	bool protected;
	pid_t pid;
};

struct process_lifecycle_fork_group {
	bool protected;
	pid_t pid;
	atomic_int ready;
	atomic_bool start;
	atomic_bool failed;
};

struct process_lifecycle_namespace {
	bool protected;
	ino_t user_namespace;
	ino_t pid_namespace;
};

struct process_lifecycle_daemon_record {
	pid_t pid;
	pid_t parent;
	pid_t session;
	int result;
};

static bool process_lifecycle_mode_mismatch(bool protected)
{
	return protected_entry_probe() == protected;
}

static void *process_lifecycle_nested_thread(void *arg)
{
	struct process_lifecycle_group *group = arg;

	if (process_lifecycle_mode_mismatch(group->protected))
		atomic_store_explicit(&group->failed, true, memory_order_relaxed);
	atomic_fetch_add_explicit(&group->nested_ready, 1, memory_order_release);
	while (!atomic_load_explicit(&group->release, memory_order_acquire))
		sched_yield();
	return NULL;
}

static void *process_lifecycle_worker(void *arg)
{
	struct process_lifecycle_group *group = arg;
	void *result = NULL;
	pthread_t nested;
	int ret;

	if (process_lifecycle_mode_mismatch(group->protected))
		atomic_store_explicit(&group->failed, true, memory_order_relaxed);
	ret = pthread_create(&nested, NULL, process_lifecycle_nested_thread, group);
	if (ret) {
		atomic_store_explicit(&group->failed, true, memory_order_relaxed);
		atomic_fetch_add_explicit(&group->nested_ready, 1,
					  memory_order_release);
	}
	atomic_fetch_add_explicit(&group->workers_ready, 1, memory_order_release);
	while (!atomic_load_explicit(&group->release, memory_order_acquire))
		sched_yield();
	if (!ret && (pthread_join(nested, &result) || result))
		atomic_store_explicit(&group->failed, true, memory_order_relaxed);
	return NULL;
}

static void *process_lifecycle_cancel_thread(void *arg)
{
	struct process_lifecycle_group *group = arg;

	if (process_lifecycle_mode_mismatch(group->protected))
		atomic_store_explicit(&group->failed, true, memory_order_relaxed);
	atomic_fetch_add_explicit(&group->cancel_ready, 1, memory_order_release);
	for (;;) {
		pthread_testcancel();
		sched_yield();
	}
	return NULL;
}

static int run_process_lifecycle_threads(bool protected)
{
	struct process_lifecycle_group group = {
		.protected = protected,
	};
	pthread_t cancel_threads[PROCESS_LIFECYCLE_CANCEL_WORKERS];
	pthread_t workers[PROCESS_LIFECYCLE_WORKERS];
	void *result;
	int i;

	if (process_lifecycle_mode_mismatch(protected))
		return 1;
	for (i = 0; i < PROCESS_LIFECYCLE_WORKERS; i++)
		if (pthread_create(&workers[i], NULL, process_lifecycle_worker,
				   &group))
			return 2;
	for (i = 0; i < PROCESS_LIFECYCLE_CANCEL_WORKERS; i++)
		if (pthread_create(&cancel_threads[i], NULL,
				   process_lifecycle_cancel_thread, &group))
			return 3;

	while (atomic_load_explicit(&group.workers_ready,
				    memory_order_acquire) < PROCESS_LIFECYCLE_WORKERS ||
	       atomic_load_explicit(&group.nested_ready,
				    memory_order_acquire) < PROCESS_LIFECYCLE_WORKERS ||
	       atomic_load_explicit(&group.cancel_ready,
				    memory_order_acquire) < PROCESS_LIFECYCLE_CANCEL_WORKERS)
		sched_yield();

	for (i = 0; i < PROCESS_LIFECYCLE_CANCEL_WORKERS; i++)
		if (pthread_cancel(cancel_threads[i]))
			atomic_store_explicit(&group.failed, true,
					      memory_order_relaxed);
	for (i = 0; i < PROCESS_LIFECYCLE_CANCEL_WORKERS; i++)
		if (pthread_join(cancel_threads[i], &result) ||
		    result != PTHREAD_CANCELED)
			atomic_store_explicit(&group.failed, true,
					      memory_order_relaxed);

	atomic_store_explicit(&group.release, true, memory_order_release);
	for (i = 0; i < PROCESS_LIFECYCLE_WORKERS; i++)
		if (pthread_join(workers[i], &result) || result)
			atomic_store_explicit(&group.failed, true,
					      memory_order_relaxed);
	return atomic_load_explicit(&group.failed, memory_order_relaxed) ? 4 : 0;
}

static int run_process_lifecycle_leaf(bool protected, pid_t parent_pid)
{
	if (process_lifecycle_mode_mismatch(protected))
		return 1;
	if (syscall(SYS_gettid) != getpid())
		return 2;
	if (getppid() != parent_pid)
		return 3;
	return 0;
}

static void *process_lifecycle_fork_thread(void *arg)
{
	struct process_lifecycle_fork_group *group = arg;
	char parent_string[32];
	char *const argv[] = {
		"protected_task_test",
		"--process-lifecycle-leaf",
		group->protected ? "protected" : "native",
		parent_string,
		NULL,
	};
	int status;
	pid_t child;

	snprintf(parent_string, sizeof(parent_string), "%d", group->pid);
	atomic_fetch_add_explicit(&group->ready, 1, memory_order_release);
	while (!atomic_load_explicit(&group->start, memory_order_acquire))
		sched_yield();

	child = fork();
	if (!child) {
		execv("/proc/self/exe", argv);
		_exit(127);
	}
	if (child < 0 || waitpid(child, &status, 0) != child ||
	    !WIFEXITED(status) || WEXITSTATUS(status))
		atomic_store_explicit(&group->failed, true, memory_order_relaxed);
	return NULL;
}

static int run_process_lifecycle_fork_exec(bool protected)
{
	struct process_lifecycle_fork_group group = {
		.protected = protected,
		.pid = getpid(),
	};
	pthread_t threads[PROCESS_LIFECYCLE_FORK_WORKERS];
	void *result;
	int i;

	for (i = 0; i < PROCESS_LIFECYCLE_FORK_WORKERS; i++)
		if (pthread_create(&threads[i], NULL,
				   process_lifecycle_fork_thread, &group))
			return 1;
	while (atomic_load_explicit(&group.ready, memory_order_acquire) <
	       PROCESS_LIFECYCLE_FORK_WORKERS)
		sched_yield();
	atomic_store_explicit(&group.start, true, memory_order_release);
	for (i = 0; i < PROCESS_LIFECYCLE_FORK_WORKERS; i++)
		if (pthread_join(threads[i], &result) || result)
			atomic_store_explicit(&group.failed, true,
					      memory_order_relaxed);
	return atomic_load_explicit(&group.failed, memory_order_relaxed) ? 2 : 0;
}

static void process_lifecycle_signal_handler(int signal)
{
}

static int run_process_lifecycle_clone3(bool protected)
{
	struct sigaction action = {
		.sa_handler = process_lifecycle_signal_handler,
	};
	struct sigaction current, old_action;
	struct clone_args args = {
		.flags = CLONE_PIDFD | CLONE_PARENT_SETTID |
			 CLONE_CLEAR_SIGHAND,
		.exit_signal = SIGCHLD,
	};
	siginfo_t info = {};
	int failure = 0, parent_tid = -1, pidfd = -1;
	pid_t child;

	sigemptyset(&action.sa_mask);
	if (sigaction(SIGUSR2, &action, &old_action))
		return 1;
	args.pidfd = (uintptr_t)&pidfd;
	args.parent_tid = (uintptr_t)&parent_tid;
	child = syscall(SYS_clone3, &args, sizeof(args));
	if (!child) {
		if (process_lifecycle_mode_mismatch(protected))
			_exit(1);
		if (sigaction(SIGUSR2, NULL, &current) ||
		    current.sa_handler != SIG_DFL)
			_exit(2);
		_exit(0);
	}
	if (child < 0)
		failure = 2;
	else if (pidfd < 0 || parent_tid != child ||
		 !(fcntl(pidfd, F_GETFD) & FD_CLOEXEC))
		failure = 3;
	else if (waitid(P_PIDFD, pidfd, &info, WEXITED) ||
		 info.si_code != CLD_EXITED || info.si_pid != child ||
		 info.si_status)
		failure = 4;
	if (pidfd >= 0 && close(pidfd) && !failure)
		failure = 5;
	if (sigaction(SIGUSR2, NULL, &current) && !failure)
		failure = 6;
	else if (!failure && current.sa_handler != process_lifecycle_signal_handler)
		failure = 7;
	if (sigaction(SIGUSR2, &old_action, NULL) && !failure)
		failure = 8;
	return failure;
}

static int process_lifecycle_namespace_child(void *arg)
{
	struct process_lifecycle_namespace *state = arg;
	struct stat namespace;
	int status;
	pid_t child;

	if (process_lifecycle_mode_mismatch(state->protected))
		return 1;
	if (stat("/proc/self/ns/user", &namespace) ||
	    namespace.st_ino == state->user_namespace)
		return 2;
	if (unshare(CLONE_NEWPID))
		return 3;
	if (stat("/proc/self/ns/pid_for_children", &namespace) ||
	    namespace.st_ino == state->pid_namespace)
		return 4;

	child = fork();
	if (!child) {
		if (process_lifecycle_mode_mismatch(state->protected))
			_exit(1);
		if (getpid() != 1 || syscall(SYS_gettid) != 1)
			_exit(2);
		if (stat("/proc/self/ns/pid", &namespace) ||
		    namespace.st_ino == state->pid_namespace)
			_exit(3);
		_exit(0);
	}
	if (child < 0 || waitpid(child, &status, 0) != child ||
	    !WIFEXITED(status) || WEXITSTATUS(status))
		return 5;
	return 0;
}

static int run_process_lifecycle_namespaces(bool protected)
{
	struct process_lifecycle_namespace state = {
		.protected = protected,
	};
	struct stat namespace;
	void *stack;
	int status;
	pid_t child;

	if (stat("/proc/self/ns/user", &namespace))
		return 1;
	state.user_namespace = namespace.st_ino;
	if (stat("/proc/self/ns/pid", &namespace))
		return 2;
	state.pid_namespace = namespace.st_ino;
	stack = mmap(NULL, PROCESS_LIFECYCLE_STACK_SIZE,
		     PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	if (stack == MAP_FAILED)
		return 3;
	child = clone(process_lifecycle_namespace_child,
		      stack + PROCESS_LIFECYCLE_STACK_SIZE,
		      CLONE_NEWUSER | SIGCHLD, &state);
	if (child < 0 || waitpid(child, &status, 0) != child ||
	    !WIFEXITED(status) || WEXITSTATUS(status)) {
		munmap(stack, PROCESS_LIFECYCLE_STACK_SIZE);
		return 4;
	}
	if (munmap(stack, PROCESS_LIFECYCLE_STACK_SIZE))
		return 5;
	return 0;
}

static int process_lifecycle_read(int fd, void *data, size_t size)
{
	char *p = data;

	while (size) {
		ssize_t n = read(fd, p, size);

		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return -1;
		p += n;
		size -= n;
	}
	return 0;
}

static int process_lifecycle_write(int fd, const void *data, size_t size)
{
	const char *p = data;

	while (size) {
		ssize_t n = write(fd, p, size);

		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return -1;
		p += n;
		size -= n;
	}
	return 0;
}

static int run_process_lifecycle_daemon(bool protected)
{
	struct process_lifecycle_daemon_record record;
	struct pollfd pollfd = {
		.events = POLLIN,
	};
	char pid_file[] = "/tmp/protected-process-XXXXXX";
	char pid_string[32], value;
	int control[2], fd, ready[2], status;
	pid_t child;

	fd = mkstemp(pid_file);
	if (fd < 0 || close(fd) || pipe(control) || pipe(ready))
		return 1;
	child = fork();
	if (!child) {
		pid_t daemon;

		close(control[1]);
		close(ready[0]);
		if (setsid() < 0)
			_exit(1);
		daemon = fork();
		if (daemon)
			_exit(daemon < 0 ? 2 : 0);
		if (process_lifecycle_read(control[0], &value, sizeof(value)))
			_exit(3);
		record = (struct process_lifecycle_daemon_record) {
			.pid = getpid(),
			.parent = getppid(),
			.session = getsid(0),
			.result = process_lifecycle_mode_mismatch(protected),
		};
		fd = open(pid_file, O_WRONLY | O_TRUNC | O_CLOEXEC);
		if (fd < 0)
			record.result = 4;
		else {
			int length = snprintf(pid_string, sizeof(pid_string), "%d\n",
					      record.pid);

			if (process_lifecycle_write(fd, pid_string, length) || close(fd))
				record.result = 5;
		}
		if (process_lifecycle_write(ready[1], &record, sizeof(record)) ||
		    process_lifecycle_read(control[0], &value, sizeof(value)))
			_exit(6);
		_exit(record.result);
	}
	close(control[0]);
	close(ready[1]);
	if (child < 0 || waitpid(child, &status, 0) != child ||
	    !WIFEXITED(status) || WEXITSTATUS(status))
		return 2;
	value = 1;
	if (process_lifecycle_write(control[1], &value, sizeof(value)) ||
	    process_lifecycle_read(ready[0], &record, sizeof(record)))
		return 3;
	if (record.result || record.pid == child || record.pid == getpid() ||
	    record.parent != 1 || record.session != child)
		return 4;
	fd = open(pid_file, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return 5;
	memset(pid_string, 0, sizeof(pid_string));
	if (read(fd, pid_string, sizeof(pid_string) - 1) <= 0 || close(fd) ||
	    strtol(pid_string, NULL, 10) != record.pid)
		return 6;
	pollfd.fd = syscall(SYS_pidfd_open, record.pid, 0);
	if (pollfd.fd < 0)
		return 7;
	if (process_lifecycle_write(control[1], &value, sizeof(value)) ||
	    poll(&pollfd, 1, 5000) != 1 || !(pollfd.revents & POLLIN))
		return 8;
	if (close(pollfd.fd) || close(control[1]) || close(ready[0]) ||
	    unlink(pid_file))
		return 9;
	return 0;
}

static int process_lifecycle_reexec(bool protected, unsigned int generation,
				    pid_t pid)
{
	char generation_string[32];
	char pid_string[32];

	snprintf(generation_string, sizeof(generation_string), "%u", generation);
	snprintf(pid_string, sizeof(pid_string), "%d", pid);
	execl("/proc/self/exe", "protected_task_test",
	      "--process-lifecycle-target", protected ? "protected" : "native",
	      generation_string, pid_string, NULL);
	return errno ?: EIO;
}

static void *process_lifecycle_exec_thread(void *arg)
{
	struct process_lifecycle_exec *exec = arg;
	int ret;

	ret = process_lifecycle_reexec(exec->protected, 1, exec->pid);
	return (void *)(uintptr_t)ret;
}

static int run_process_lifecycle_target(bool protected,
					unsigned int generation,
					pid_t original_pid)
{
	struct process_lifecycle_exec exec = {
		.protected = protected,
		.pid = original_pid,
	};
	void *result;
	pthread_t thread;
	int ret;

	if (process_lifecycle_mode_mismatch(protected))
		return 1;
	if (getpid() != original_pid || syscall(SYS_gettid) != original_pid)
		return 5;
	if (generation) {
		if (generation == 4)
			return 0;
		ret = process_lifecycle_reexec(protected, generation + 1,
					       original_pid);
		return 20 + !!ret;
	}

	ret = run_process_lifecycle_threads(protected);
	if (ret)
		return ret;
	ret = run_process_lifecycle_fork_exec(protected);
	if (ret)
		return 10 + ret;
	ret = run_process_lifecycle_clone3(protected);
	if (ret)
		return 20 + ret;
	ret = run_process_lifecycle_namespaces(protected);
	if (ret)
		return 30 + ret;
	ret = run_process_lifecycle_daemon(protected);
	if (ret)
		return 40 + ret;
	if (pthread_create(&thread, NULL, process_lifecycle_exec_thread, &exec))
		return 50;
	if (pthread_join(thread, &result))
		return 51;
	return 52 + !!result;
}

static int run_process_security_target(bool protected, int expected_securebits)
{
	struct __user_cap_header_struct header = {
		.version = _LINUX_CAPABILITY_VERSION_3,
	};
	struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3] = {};
	unsigned int mask = 1U << CAP_NET_RAW;

	if (process_lifecycle_mode_mismatch(protected))
		return 1;
	if (prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) != 1)
		return 2;
	if (prctl(PR_GET_SECUREBITS, 0, 0, 0, 0) != expected_securebits)
		return 3;
	if (prctl(PR_GET_SECCOMP, 0, 0, 0, 0) != SECCOMP_MODE_FILTER)
		return 4;
	errno = 0;
	if (syscall(SYS_getppid) != -1 || errno != EACCES)
		return 5;
	if (syscall(SYS_getpgid, 0) != getpgrp())
		return 6;
	if (prctl(PR_CAPBSET_READ, CAP_NET_RAW, 0, 0, 0) != 0)
		return 7;
	if (syscall(SYS_capget, &header, data))
		return 8;
	if ((data[0].effective | data[0].permitted) & mask)
		return 9;
	return 0;
}

static int run_process_security_strict_target(bool protected)
{
	if (process_lifecycle_mode_mismatch(protected))
		return 1;
	if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT, 0, 0, 0))
		return 2;
	syscall(SYS_getpid);
	return 3;
}

static int run_process_security_landlock_target(bool protected,
						 const char *path)
{
	int fd;

	if (process_lifecycle_mode_mismatch(protected))
		return 1;
	errno = 0;
	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd >= 0) {
		close(fd);
		return 2;
	}
	return errno == EACCES ? 0 : 3;
}

static int run_process_security_credential_target(bool protected)
{
	uid_t ruid, euid, suid;
	gid_t rgid, egid, sgid;
	int signal;

	if (process_lifecycle_mode_mismatch(protected))
		return 1;
	if (getresuid(&ruid, &euid, &suid) ||
	    getresgid(&rgid, &egid, &sgid))
		return 2;
	if (ruid != TEST_UID || euid || suid ||
	    rgid != TEST_UID || egid || sgid)
		return 3;
	if (getauxval(AT_SECURE) != 1)
		return 4;
	if (getenv("LD_PRELOAD") || getenv("LD_LIBRARY_PATH"))
		return 5;
	if (!getenv("PROCESS_SECURITY_MARKER") ||
	    secure_getenv("PROCESS_SECURITY_MARKER"))
		return 6;
	if (prctl(PR_GET_PDEATHSIG, &signal) || signal)
		return 7;
	return 0;
}

static int run_process_observability_target(bool protected)
{
	struct process_observability_target target = {
		.payload_start = (unsigned long)process_observability_payload,
		.payload_end = (unsigned long)process_observability_payload_end,
		.pid = getpid(),
		.tid = syscall(SYS_gettid),
	};
	struct process_observability_result result;
	struct timespec before, after;
	char command;
	int i;

	if (process_lifecycle_mode_mismatch(protected))
		return 1;
	strcpy(target.comm, protected ? "pt-obs-prot" : "pt-obs-native");
	if (prctl(PR_SET_NAME, target.comm, 0, 0, 0))
		return 2;
	if (process_lifecycle_write(PROCESS_OBSERVABILITY_READY_FD, &target,
				    sizeof(target)))
		return 3;

	for (;;) {
		if (process_lifecycle_read(PROCESS_OBSERVABILITY_CONTROL_FD,
					   &command, sizeof(command)))
			return 4;
		if (command != 'B' && command != 'I')
			return 5;
		if (clock_gettime(CLOCK_MONOTONIC, &before))
			return 6;
		for (i = 0; i < PROCESS_OBSERVABILITY_CALLS; i++)
			process_observability_payload(
				PROCESS_OBSERVABILITY_ITERATIONS);
		if (clock_gettime(CLOCK_MONOTONIC, &after))
			return 7;
		result.elapsed_ns = (after.tv_sec - before.tv_sec) * 1000000000ULL +
			(after.tv_nsec - before.tv_nsec);
		if (process_lifecycle_write(PROCESS_OBSERVABILITY_RESULT_FD,
					    &result, sizeof(result)))
			return 8;
		if (command == 'I')
			break;
	}

	return 0;
}

static int install_process_security_filter(void)
{
	struct sock_filter filter[] = {
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS,
			 offsetof(struct seccomp_data, nr)),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_getppid, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EACCES),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_getpgid, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_LOG),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
	};
	struct sock_fprog program = {
		.len = ARRAY_SIZE(filter),
		.filter = filter,
	};

	return syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, &program);
}

static int create_process_security_executable(const char *source_path,
					      char path[PATH_MAX], bool setid)
{
	struct vfs_cap_data capabilities = {
		.magic_etc = htole32(VFS_CAP_REVISION_2 |
					  VFS_CAP_FLAGS_EFFECTIVE),
		.data[0].permitted = htole32(1U << CAP_NET_BIND_SERVICE),
	};
	char buffer[16384];
	int source = -1, target = -1, result = 0;

	strcpy(path, "/protected-task-security-XXXXXX");
	target = mkstemp(path);
	if (target < 0)
		return -errno;
	source = open(source_path, O_RDONLY | O_CLOEXEC);
	if (source < 0) {
		result = -errno;
		goto out;
	}
	for (;;) {
		ssize_t size = read(source, buffer, sizeof(buffer));

		if (size < 0 && errno == EINTR)
			continue;
		if (size < 0) {
			result = -errno;
			goto out;
		}
		if (!size)
			break;
		if (process_lifecycle_write(target, buffer, size)) {
			result = -errno;
			goto out;
		}
	}
	if (fchmod(target, setid ? 06755 : 0755)) {
		result = -errno;
		goto out;
	}
	if (!setid && fsetxattr(target, "security.capability", &capabilities,
				       XATTR_CAPS_SZ_2, 0))
		result = -errno;

out:
	if (source >= 0)
		close(source);
	if (close(target) && !result)
		result = -errno;
	if (result)
		unlink(path);
	return result;
}

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
	bool sse3;
	bool pclmulqdq;
	bool ssse3;
	bool sse41;
	bool sse42;
	bool popcnt;
	bool aes;
	bool shani;
	bool movbe;
	bool rdrand;
	bool bmi1;
	bool bmi2;
	bool rdseed;
	bool adx;
	bool lahf;
	bool abm;
	bool rdpid;
	bool rdtscp;
	bool clflushopt;
	bool clwb;
	bool clzero;
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
	bool avxvnni;
	bool avxifma;
	bool avxvnniint8;
	bool avxneconvert;
	bool avxvnniint16;
	bool sha512;
	bool sm3;
	bool sm4;
	bool avx10;
	bool avx10_vnni_int;
	bool amx_bf16;
	bool amx_int8;
	bool amx_fp16;
	bool amx_complex;
	bool pku;
	bool amx;
	bool shstk;
	__u8 avx10_version;
	__u32 ymm_offset;
	__u32 opmask_offset;
	__u32 zmm_hi256_offset;
	__u32 hi16_zmm_offset;
	__u32 pkru_offset;
	__u32 tilecfg_offset;
	__u32 tiledata_offset;
};

static bool kvm_supported_xstate_component(
		const struct kvm_cpuid_entry2 *entry, __u32 size, bool xfd,
		__u32 *end)
{
	return entry->eax == size && entry->ebx >= 576 &&
		!(entry->ecx & (1U << 0)) && !!(entry->ecx & (1U << 2)) == xfd &&
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
	struct kvm_cpuid_entry2 avx10_0 = {}, avx10_1 = {};
	bool avx = false, avx512 = false, avx512_xstate = false;
	bool avx10 = false, have_avx10_0 = false, have_avx10_1 = false;
	bool amx_controls = false, amx_palette0 = false, amx_palette1 = false;
	bool amx_tile = false, amx_tmul = false, tilecfg = false, tiledata = false;
	bool hi16_zmm = false, opmask = false, ymm = false;
	bool pku = false, pkru_component = false, xsave = false;
	bool shstk = false, shstk_component = false, shstk_controls = false;
	bool ymm_component = false, zmm_hi256 = false;
	__u32 xfeatures = 0;
	__u32 hi16_zmm_end = 0, opmask_end = 0, ymm_end = 0;
	__u32 tilecfg_end = 0, tiledata_end = 0, zmm_hi256_end = 0;
	unsigned int i;
	int ret;

	ret = ioctl(kvm_fd, KVM_GET_SUPPORTED_CPUID, &cpuid);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_GET_SUPPORTED_CPUID, ret));
	for (i = 0; i < cpuid.nent; i++) {
		struct kvm_cpuid_entry2 *entry = &cpuid.entries[i];

		if (entry->function == 1 && !entry->index) {
			features.sse3 = entry->ecx & (1U << 0);
			features.pclmulqdq = entry->ecx & (1U << 1);
			features.ssse3 = entry->ecx & (1U << 9);
			features.sse41 = entry->ecx & (1U << 19);
			features.sse42 = entry->ecx & (1U << 20);
			features.movbe = entry->ecx & (1U << 22);
			features.popcnt = entry->ecx & (1U << 23);
			features.aes = entry->ecx & (1U << 25);
			features.rdrand = entry->ecx & (1U << 30);
			xsave = entry->ecx & (1U << 26);
			avx = (entry->ecx & ((1U << 26) | (1U << 28))) ==
				((1U << 26) | (1U << 28));
			features.fma = entry->ecx & (1U << 12);
			features.f16c = entry->ecx & (1U << 29);
		} else if (entry->function == 7 && !entry->index) {
			features.bmi1 = entry->ebx & (1U << 3);
			features.bmi2 = entry->ebx & (1U << 8);
			features.rdseed = entry->ebx & (1U << 18);
			features.adx = entry->ebx & (1U << 19);
			features.clflushopt = entry->ebx & (1U << 23);
			features.clwb = entry->ebx & (1U << 24);
			features.shani = entry->ebx & (1U << 29);
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
			features.rdpid = entry->ecx & (1U << 22);
			pku = entry->ecx & (1U << 3);
			shstk = entry->ecx & (1U << 7);
			features.avx5124vnniw = entry->edx & (1U << 2);
			features.avx5124fmaps = entry->edx & (1U << 3);
			features.avx512vp2intersect = entry->edx & (1U << 8);
			features.avx512fp16 = entry->edx & (1U << 23);
			features.amx_bf16 = entry->edx & (1U << 22);
			amx_tile = entry->edx & (1U << 24);
			features.amx_int8 = entry->edx & (1U << 25);
		} else if (entry->function == 7 && entry->index == 1) {
			features.sha512 = entry->eax & (1U << 0);
			features.sm3 = entry->eax & (1U << 1);
			features.sm4 = entry->eax & (1U << 2);
			features.avxvnni = entry->eax & (1U << 4);
			features.avx512bf16 = entry->eax & (1U << 5);
			features.avxifma = entry->eax & (1U << 23);
			features.amx_fp16 = entry->eax & (1U << 21);
			features.avxvnniint8 = entry->edx & (1U << 4);
			features.avxneconvert = entry->edx & (1U << 5);
			features.amx_complex = entry->edx & (1U << 8);
			features.avxvnniint16 = entry->edx & (1U << 10);
			avx10 = entry->edx & (1U << 19);
		} else if (entry->function == 0x80000001) {
			features.lahf = entry->ecx & (1U << 0);
			features.abm = entry->ecx & (1U << 5);
			features.rdtscp = entry->edx & (1U << 27);
		} else if (entry->function == 0x80000008) {
			features.clzero = entry->ebx & (1U << 0);
		} else if (entry->function == 0xd && !entry->index) {
			xfeatures = entry->eax;
			ymm = entry->eax & (1U << 2);
			avx512_xstate = (entry->eax & (0xe0U)) == 0xe0U;
		} else if (entry->function == 0xd && entry->index == 2) {
			ymm_component = kvm_supported_xstate_component(entry, 256, false,
								 &ymm_end);
			features.ymm_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 5) {
			opmask = kvm_supported_xstate_component(entry, 64, false,
							       &opmask_end);
			features.opmask_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 6) {
			zmm_hi256 = kvm_supported_xstate_component(entry, 512, false,
								  &zmm_hi256_end);
			features.zmm_hi256_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 7) {
			hi16_zmm = kvm_supported_xstate_component(entry, 1024, false,
								 &hi16_zmm_end);
			features.hi16_zmm_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 9) {
			pkru_component = entry->eax && entry->ebx >= 576;
			features.pkru_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 1) {
			amx_controls = (entry->eax & 0x1cU) == 0x1cU;
			shstk_controls = (entry->eax & (1U << 3)) &&
				(entry->ecx & (1U << 11));
		} else if (entry->function == 0xd && entry->index == 11) {
			shstk_component = entry->eax == 16 &&
				(entry->ecx & (1U << 0));
		} else if (entry->function == 0xd && entry->index == 17) {
			tilecfg = kvm_supported_xstate_component(entry, 64, false,
								 &tilecfg_end);
			features.tilecfg_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 18) {
			tiledata = kvm_supported_xstate_component(entry, 8192, true,
								  &tiledata_end);
			features.tiledata_offset = entry->ebx;
		} else if (entry->function == 0x1d && !entry->index) {
			amx_palette0 = entry->eax >= 1;
		} else if (entry->function == 0x1d && entry->index == 1) {
			amx_palette1 = entry->eax == 0x04002000 &&
				entry->ebx == 0x00080040 && entry->ecx == 16 &&
				!entry->edx;
		} else if (entry->function == 0x1e && !entry->index) {
			amx_tmul = entry->eax <= 1 && entry->ebx == 0x4010 &&
				!entry->ecx && !entry->edx;
		} else if (entry->function == 0x24 && !entry->index) {
			avx10_0 = *entry;
			have_avx10_0 = true;
		} else if (entry->function == 0x24 && entry->index == 1) {
			avx10_1 = *entry;
			have_avx10_1 = true;
		}
	}

	features.avx = avx && ymm && ymm_component;
	features.avx512 = features.avx && avx512 && avx512_xstate &&
		opmask && zmm_hi256 && hi16_zmm &&
		opmask_end - 64 >= ymm_end &&
		zmm_hi256_end - 512 >= opmask_end &&
		hi16_zmm_end - 1024 >= zmm_hi256_end;
	features.pku = xsave && pku && (xfeatures & (1U << 9)) &&
		pkru_component;
	features.amx = amx_tile &&
		(xfeatures & 0x60000U) == 0x60000U && amx_controls &&
		tilecfg && tiledata && features.tiledata_offset >= tilecfg_end &&
		amx_palette0 && amx_palette1 && amx_tmul;
	if (features.avx512 && avx10 && have_avx10_0) {
		__u8 version = avx10_0.ebx & 0xff;

		features.avx10 = version >= 1 && version <= 2 &&
			(avx10_0.ebx & 0x70000U) == 0x70000U &&
			!(avx10_0.ebx & ~(0x70000U | 0xffU)) &&
			avx10_0.eax <= 1 && !avx10_0.ecx && !avx10_0.edx &&
			(version < 2 || avx10_0.eax == 1) &&
			(!avx10_0.eax ||
			 (have_avx10_1 && !avx10_1.eax && !avx10_1.ebx &&
			  !(avx10_1.ecx & ~4U) && !avx10_1.edx));
		if (features.avx10) {
			features.avx10_version = version;
			features.avx10_vnni_int = avx10_0.eax &&
				(avx10_1.ecx & 4U);
		}
	}
	if (!features.amx)
		features.amx_bf16 = features.amx_int8 =
			features.amx_fp16 = features.amx_complex = false;
	if (shstk && shstk_controls && shstk_component) {
		unsigned long status;

		features.shstk = syscall(SYS_arch_prctl, ARCH_SHSTK_STATUS,
					 &status) == 0;
	}
	if (!features.avx)
		features.fma = features.f16c = features.avx2 =
			features.gfni = features.vaes = features.vpclmulqdq =
			features.avxvnni = features.avxifma =
			features.avxvnniint8 = features.avxneconvert =
			features.avxvnniint16 = features.sha512 =
			features.sm3 = features.sm4 = false;
	if (!features.avx512)
		features.avx512dq = features.avx512ifma =
			features.avx512pf = features.avx512er =
			features.avx512cd = features.avx512bw =
			features.avx512vl = features.avx512vbmi =
			features.avx512vbmi2 =
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
	if (features->sse3)
		append_expected_output(output, output_size, length,
				       "protected task sse3\n");
	if (features->pclmulqdq)
		append_expected_output(output, output_size, length,
				       "protected task pclmulqdq\n");
	if (features->ssse3)
		append_expected_output(output, output_size, length,
				       "protected task ssse3\n");
	if (features->sse41)
		append_expected_output(output, output_size, length,
				       "protected task sse4_1\n");
	if (features->sse42)
		append_expected_output(output, output_size, length,
				       "protected task sse4_2\n");
	if (features->popcnt)
		append_expected_output(output, output_size, length,
				       "protected task popcnt\n");
	if (features->aes)
		append_expected_output(output, output_size, length,
				       "protected task aes\n");
	if (features->shani)
		append_expected_output(output, output_size, length,
				       "protected task sha_ni\n");
	if (features->movbe)
		append_expected_output(output, output_size, length,
				       "protected task movbe\n");
	if (features->rdrand)
		append_expected_output(output, output_size, length,
				       "protected task rdrand\n");
	if (features->bmi1)
		append_expected_output(output, output_size, length,
				       "protected task bmi1\n");
	if (features->bmi2)
		append_expected_output(output, output_size, length,
				       "protected task bmi2\n");
	if (features->rdseed)
		append_expected_output(output, output_size, length,
				       "protected task rdseed\n");
	if (features->adx)
		append_expected_output(output, output_size, length,
				       "protected task adx\n");
	if (features->lahf)
		append_expected_output(output, output_size, length,
				       "protected task lahf\n");
	if (features->abm)
		append_expected_output(output, output_size, length,
				       "protected task abm\n");
	if (features->rdpid)
		append_expected_output(output, output_size, length,
				       "protected task rdpid\n");
	if (features->rdtscp)
		append_expected_output(output, output_size, length,
				       "protected task rdtscp\n");
	if (features->clflushopt)
		append_expected_output(output, output_size, length,
				       "protected task clflushopt\n");
	if (features->clwb)
		append_expected_output(output, output_size, length,
				       "protected task clwb\n");
	if (features->clzero)
		append_expected_output(output, output_size, length,
				       "protected task clzero\n");
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
	if (features->avxvnni)
		append_expected_output(output, output_size, length,
				       "protected task avxvnni\n");
	if (features->avxifma)
		append_expected_output(output, output_size, length,
				       "protected task avxifma\n");
	if (features->avxvnniint8)
		append_expected_output(output, output_size, length,
				       "protected task avxvnniint8\n");
	if (features->avxneconvert)
		append_expected_output(output, output_size, length,
				       "protected task avxneconvert\n");
	if (features->avxvnniint16)
		append_expected_output(output, output_size, length,
				       "protected task avxvnniint16\n");
	if (features->sha512)
		append_expected_output(output, output_size, length,
				       "protected task sha512\n");
	if (features->sm3)
		append_expected_output(output, output_size, length,
				       "protected task sm3\n");
	if (features->sm4)
		append_expected_output(output, output_size, length,
				       "protected task sm4\n");
	if (features->avx10_version == 1)
		append_expected_output(output, output_size, length,
				       "protected task avx10_v1\n");
	else if (features->avx10_version == 2)
		append_expected_output(output, output_size, length,
				       "protected task avx10_v2\n");
	if (features->avx10_vnni_int)
		append_expected_output(output, output_size, length,
				       "protected task avx10_vnni_int\n");
	if (features->amx_bf16)
		append_expected_output(output, output_size, length,
				       "protected task amx_bf16\n");
	if (features->amx_int8)
		append_expected_output(output, output_size, length,
				       "protected task amx_int8\n");
	if (features->amx_fp16)
		append_expected_output(output, output_size, length,
				       "protected task amx_fp16\n");
	if (features->amx_complex)
		append_expected_output(output, output_size, length,
				       "protected task amx_complex\n");
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

static void test_protected_entry_returns(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	unsigned int eax, ebx, ecx, edx;
	int status;
	pid_t child;

	__cpuid_count(7, 0, eax, ebx, ecx, edx);
	if (!(ebx & bit_FSGSBASE)) {
		print_skip("Native FSGSBASE is unavailable");
		return;
	}

	child = fork();
	TEST_ASSERT(child >= 0, "native control fork() failed: %d", errno);
	if (!child) {
		execl("/proc/self/exe", "protected_task_test", "--entry-target",
		      NULL);
		_exit(127);
	}
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for native entry control failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 203,
		    "Native entry control produced unexpected status: %#x", status);

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		TEST_ASSERT(ptrace(PTRACE_TRACEME, 0, NULL, NULL) == 0,
			    "PTRACE_TRACEME failed: %d", errno);
		TEST_ASSERT(raise(SIGSTOP) == 0,
			    "Initial trace stop failed: %d", errno);
		fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl("/proc/self/exe", "protected_task_test", "--entry-target",
		      NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at initial trace stop failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP,
		    "Initial trace stop produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_SETOPTIONS, child, NULL,
			   PTRACE_O_TRACEEXEC) == 0,
		    "PTRACE_SETOPTIONS failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT to exec failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at exec event failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP &&
		    (unsigned int)status >> 16 == PTRACE_EVENT_EXEC,
		    "Exec event produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT after exec failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at target trace stop failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP,
		    "Target trace stop produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT after target stop failed: %d", errno);
	for (;;) {
		TEST_ASSERT(waitpid(child, &status, 0) == child,
			    "waitpid() after entry target failed: %d", errno);
		if (WIFEXITED(status))
			break;
		TEST_ASSERT(WIFSTOPPED(status) &&
			    (WSTOPSIG(status) == SIGUSR1 ||
			     WSTOPSIG(status) == SIGCHLD),
			    "Entry target produced unexpected status: %#x", status);
		TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL,
				   (void *)(uintptr_t)WSTOPSIG(status)) == 0,
			    "PTRACE_CONT with signal %d failed: %d",
			    WSTOPSIG(status), errno);
	}
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Protected entry target failed: %#x", status);
}

static void run_process_state_target(int kvm_fd, bool protected)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "process-state fork() failed: %d", errno);
	if (!child) {
		if (protected) {
			int fd, ret;

			fd = create_context_with_features(kvm_fd,
						  KVM_PROTECTED_TASK_FEATURE_EXEC);
			ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
			TEST_ASSERT(ret == 0,
				    KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		}
		execl("/proc/self/exe", "protected_task_test",
		      "--process-state-target", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for %s process-state target failed: %d",
		    protected ? "protected" : "native", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "%s process-state target failed: %#x",
		    protected ? "Protected" : "Native", status);
}

static void test_protected_process_state(int kvm_fd)
{
	run_process_state_target(kvm_fd, false);
	run_process_state_target(kvm_fd, true);
}

static void run_process_failure_signal_control(int kvm_fd, bool protected)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct rlimit core_limit = {};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "process-failure fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		TEST_ASSERT(setrlimit(RLIMIT_CORE, &core_limit) == 0,
			    "setrlimit(RLIMIT_CORE) failed: %d", errno);
		if (protected) {
			fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
			ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
			TEST_ASSERT(ret == 0,
				    KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		}
		execl("/proc/self/exe", "protected_task_test",
		      "--process-failure-signal-target", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for %s failure-signal target failed: %d",
		    protected ? "protected" : "native", errno);
	TEST_ASSERT(WIFSIGNALED(status) && WTERMSIG(status) == SIGILL,
		    "%s expected guest fault produced status %#x",
		    protected ? "Protected" : "Native", status);
}

static bool process_failure_log_matches(int kmsg_fd, pid_t pid)
{
	struct pollfd pollfd = {
		.fd = kmsg_fd,
		.events = POLLIN,
	};
	char expected[256], record[8192];
	int retries = 2;

	snprintf(expected, sizeof(expected),
		 "task=pt-failure[%d] category=engine phase=syscall-exit reason=unexpected-rip error=%d exit=%d rip=0x",
		 pid, -EIO, KVM_EXIT_HYPERCALL);
	for (;;) {
		ssize_t length = read(kmsg_fd, record, sizeof(record) - 1);

		if (length >= 0) {
			record[length] = '\0';
			if (strstr(record, expected)) {
				char *rip = strstr(record, " rip=0x");

				return rip && strtoul(rip + 7, NULL, 16);
			}
			continue;
		}
		if (errno == EINTR)
			continue;
		if (errno != EAGAIN || !retries--)
			return false;
		if (poll(&pollfd, 1, 1000) < 0 && errno != EINTR)
			return false;
	}
}

static void test_protected_process_failure(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct rlimit core_limit = {};
	int kmsg_fd, status;
	pid_t child;

	run_process_failure_signal_control(kvm_fd, false);
	run_process_failure_signal_control(kvm_fd, true);

	kmsg_fd = open("/dev/kmsg", O_RDONLY | O_CLOEXEC | O_NONBLOCK);
	if (kmsg_fd >= 0 && lseek(kmsg_fd, 0, SEEK_END) < 0) {
		close(kmsg_fd);
		kmsg_fd = -1;
	}

	child = fork();
	TEST_ASSERT(child >= 0, "engine-failure fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		TEST_ASSERT(setrlimit(RLIMIT_CORE, &core_limit) == 0,
			    "setrlimit(RLIMIT_CORE) failed: %d", errno);
		fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl("/proc/self/exe", "protected_task_test",
		      "--process-failure-engine-target", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for engine-failure target failed: %d", errno);
	TEST_ASSERT(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL,
		    "Protected engine failure produced status %#x", status);
	if (kmsg_fd < 0)
		print_skip("Kernel log access is unavailable");
	else {
		TEST_ASSERT(process_failure_log_matches(kmsg_fd, child),
			    "Missing protected engine failure diagnostic for PID %d",
			    child);
		close(kmsg_fd);
	}
}

static void run_process_lifecycle_control(int kvm_fd, bool protected)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "process-lifecycle fork() failed: %d", errno);
	if (!child) {
		if (protected) {
			int fd, ret;

			fd = create_context_with_features(kvm_fd,
						  KVM_PROTECTED_TASK_FEATURE_EXEC);
			ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
			TEST_ASSERT(ret == 0,
				    KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		}
		execl("/proc/self/exe", "protected_task_test",
		      "--process-lifecycle-target",
		      protected ? "protected" : "native", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for %s process-lifecycle target failed: %d",
		    protected ? "protected" : "native", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "%s process-lifecycle target failed: %#x",
		    protected ? "Protected" : "Native", status);
}

static void test_protected_process_lifecycle(int kvm_fd)
{
	run_process_lifecycle_control(kvm_fd, false);
	run_process_lifecycle_control(kvm_fd, true);
}

static void run_process_security_control(int kvm_fd, bool protected)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	char securebits_string[32];
	int securebits, status;
	pid_t child;

	securebits = prctl(PR_GET_SECUREBITS, 0, 0, 0, 0);
	TEST_ASSERT(securebits >= 0, "PR_GET_SECUREBITS failed: %d", errno);
	securebits |= SECBIT_NO_CAP_AMBIENT_RAISE |
		SECBIT_NO_CAP_AMBIENT_RAISE_LOCKED;
	snprintf(securebits_string, sizeof(securebits_string), "%d", securebits);

	child = fork();
	TEST_ASSERT(child >= 0, "process-security fork() failed: %d", errno);
	if (!child) {
		int fd = -1, ret;

		if (protected)
			fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		TEST_ASSERT(prctl(PR_SET_SECUREBITS, securebits, 0, 0, 0) == 0,
			    "PR_SET_SECUREBITS failed: %d", errno);
		TEST_ASSERT(prctl(PR_CAPBSET_DROP, CAP_NET_RAW, 0, 0, 0) == 0,
			    "PR_CAPBSET_DROP failed: %d", errno);
		TEST_ASSERT(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0,
			    "PR_SET_NO_NEW_PRIVS failed: %d", errno);
		TEST_ASSERT(install_process_security_filter() == 0,
			    "SECCOMP_SET_MODE_FILTER failed: %d", errno);
		if (protected) {
			ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
			TEST_ASSERT(ret == 0,
				    KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		}
		execl("/proc/self/exe", "protected_task_test",
		      "--process-security-target",
		      protected ? "protected" : "native", securebits_string,
		      NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for %s process-security target failed: %d",
		    protected ? "protected" : "native", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "%s process-security target failed: %#x",
		    protected ? "Protected" : "Native", status);
}

static void run_process_security_strict_control(int kvm_fd, bool protected)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "strict-seccomp fork() failed: %d", errno);
	if (!child) {
		if (protected) {
			int fd, ret;

			fd = create_context_with_features(kvm_fd,
						  KVM_PROTECTED_TASK_FEATURE_EXEC);
			ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
			TEST_ASSERT(ret == 0,
				    KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		}
		execl("/proc/self/exe", "protected_task_test",
		      "--process-security-strict-target",
		      protected ? "protected" : "native", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for %s strict-seccomp target failed: %d",
		    protected ? "protected" : "native", errno);
	TEST_ASSERT(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL,
		    "%s strict-seccomp target produced unexpected status: %#x",
		    protected ? "Protected" : "Native", status);
}

static int process_security_landlock_allow(int ruleset_fd, const char *path)
{
	struct landlock_path_beneath_attr rule = {
		.allowed_access = LANDLOCK_ACCESS_FS_READ_FILE,
	};
	int ret;

	rule.parent_fd = open(path, O_PATH | O_DIRECTORY | O_CLOEXEC);
	if (rule.parent_fd < 0)
		return -errno;
	ret = syscall(SYS_landlock_add_rule, ruleset_fd,
		      LANDLOCK_RULE_PATH_BENEATH, &rule, 0);
	if (ret)
		ret = -errno;
	if (close(rule.parent_fd) && !ret)
		ret = -errno;
	return ret;
}

static void run_process_security_landlock_control(
		int kvm_fd, bool protected, const char *self, const char *path)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "Landlock fork() failed: %d", errno);
	if (!child) {
		struct landlock_ruleset_attr ruleset = {
			.handled_access_fs = LANDLOCK_ACCESS_FS_READ_FILE,
		};
		int fd = -1, ret, ruleset_fd;

		if (protected)
			fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		ruleset_fd = syscall(SYS_landlock_create_ruleset, &ruleset,
				     sizeof(ruleset), 0);
		TEST_ASSERT(ruleset_fd >= 0,
			    "landlock_create_ruleset() failed: %d", errno);
		ret = process_security_landlock_allow(ruleset_fd, "/usr");
		TEST_ASSERT(!ret, "Failed to allow /usr through Landlock: %d",
			    -ret);
		ret = process_security_landlock_allow(ruleset_fd, "/etc");
		TEST_ASSERT(!ret, "Failed to allow /etc through Landlock: %d",
			    -ret);
		ret = process_security_landlock_allow(ruleset_fd, "/dev");
		TEST_ASSERT(!ret, "Failed to allow /dev through Landlock: %d",
			    -ret);
		ret = process_security_landlock_allow(ruleset_fd, "/proc");
		TEST_ASSERT(!ret, "Failed to allow /proc through Landlock: %d",
			    -ret);
		ret = process_security_landlock_allow(ruleset_fd, "/sys");
		TEST_ASSERT(!ret, "Failed to allow /sys through Landlock: %d",
			    -ret);
		TEST_ASSERT(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0,
			    "PR_SET_NO_NEW_PRIVS failed: %d", errno);
		TEST_ASSERT(syscall(SYS_landlock_restrict_self, ruleset_fd, 0) == 0,
			    "landlock_restrict_self() failed: %d", errno);
		TEST_ASSERT(close(ruleset_fd) == 0,
			    "Failed to close Landlock ruleset: %d", errno);
		if (protected) {
			ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
			TEST_ASSERT(ret == 0,
				    KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		}
		execl(self, "protected_task_test",
		      "--process-security-landlock-target",
		      protected ? "protected" : "native", path, NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for %s Landlock target failed: %d",
		    protected ? "protected" : "native", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "%s Landlock target failed: %#x",
		    protected ? "Protected" : "Native", status);
}

static void test_protected_process_landlock(int kvm_fd)
{
	char path[] = "/protected-task-landlock-XXXXXX";
	char self[PATH_MAX];
	long abi;
	int fd;

	abi = syscall(SYS_landlock_create_ruleset, NULL, 0,
		      LANDLOCK_CREATE_RULESET_VERSION);
	if (abi < 0 && (errno == ENOSYS || errno == EOPNOTSUPP)) {
		print_skip("Landlock is unavailable");
		return;
	}
	TEST_ASSERT(abi >= 1, "Landlock ABI query failed: %d", errno);
	fd = mkstemp(path);
	TEST_ASSERT(fd >= 0, "Failed to create Landlock target: %d", errno);
	TEST_ASSERT(write(fd, "x", 1) == 1,
		    "Failed to write Landlock target: %d", errno);
	TEST_ASSERT(close(fd) == 0,
		    "Failed to close Landlock target: %d", errno);
	fd = readlink("/proc/self/exe", self, sizeof(self) - 1);
	TEST_ASSERT(fd > 0, "readlink(/proc/self/exe) failed: %d", errno);
	self[fd] = '\0';
	run_process_security_landlock_control(kvm_fd, false, self, path);
	run_process_security_landlock_control(kvm_fd, true, self, path);
	TEST_ASSERT(unlink(path) == 0,
		    "Failed to remove Landlock target: %d", errno);
}

static void run_process_security_credential_control(
		int kvm_fd, bool protected, const char *path, const char *mode)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "%s credential fork() failed: %d", mode,
		    errno);
	if (!child) {
		int fd = -1, ret;

		if (protected)
			fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		TEST_ASSERT(setgroups(0, NULL) == 0,
			    "setgroups() failed: %d", errno);
		TEST_ASSERT(setresgid(TEST_UID, TEST_UID, TEST_UID) == 0,
			    "setresgid() failed: %d", errno);
		TEST_ASSERT(setresuid(TEST_UID, TEST_UID, TEST_UID) == 0,
			    "setresuid() failed: %d", errno);
		TEST_ASSERT(prctl(PR_SET_PDEATHSIG, SIGUSR1) == 0,
			    "PR_SET_PDEATHSIG failed: %d", errno);
		if (protected) {
			ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
			TEST_ASSERT(ret == 0,
				    KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		}
		if (!strcmp(mode, "setid")) {
			TEST_ASSERT(setenv("LD_PRELOAD", "/protected-task-unused.so", 1) == 0,
				    "Failed to set LD_PRELOAD: %d", errno);
			TEST_ASSERT(setenv("LD_LIBRARY_PATH", "/protected-task-unused", 1) == 0,
				    "Failed to set LD_LIBRARY_PATH: %d", errno);
			TEST_ASSERT(setenv("PROCESS_SECURITY_MARKER", "present", 1) == 0,
				    "Failed to set secure-exec marker: %d", errno);
			execl(path, "protected_task_test",
			      "--process-security-credential-target",
			      protected ? "protected" : "native", NULL);
		} else
			execl(path, protected ? "p-filecap" : "n-filecap", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for %s %s credential target failed: %d",
		    protected ? "protected" : "native", mode, errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "%s %s credential target failed: %#x",
		    protected ? "Protected" : "Native", mode, status);
}

static void test_protected_process_credentials(int kvm_fd)
{
	char filecap_path[PATH_MAX], helper[PATH_MAX], self[PATH_MAX];
	char setid_path[PATH_MAX];
	int ret;
	ssize_t length;

	if (geteuid()) {
		print_skip("Set-ID and file-capability tests require root");
		return;
	}
	get_exec_helper_path(helper);
	length = readlink("/proc/self/exe", self, sizeof(self) - 1);
	TEST_ASSERT(length > 0, "readlink(/proc/self/exe) failed: %d", errno);
	self[length] = '\0';
	ret = create_process_security_executable(self, setid_path, true);
	TEST_ASSERT(!ret, "Failed to create set-ID executable: %d", -ret);
	run_process_security_credential_control(kvm_fd, false, setid_path,
						"setid");
	run_process_security_credential_control(kvm_fd, true, setid_path,
						"setid");
	TEST_ASSERT(unlink(setid_path) == 0,
		    "Failed to remove set-ID executable: %d", errno);

	ret = create_process_security_executable(helper, filecap_path, false);
	if (ret == -EOPNOTSUPP || ret == -ENOTSUP || ret == -EPERM) {
		print_skip("File capabilities are unavailable");
		return;
	}
	TEST_ASSERT(!ret, "Failed to create file-capability executable: %d",
		    -ret);
	run_process_security_credential_control(kvm_fd, false, filecap_path,
						"filecap");
	run_process_security_credential_control(kvm_fd, true, filecap_path,
						"filecap");
	TEST_ASSERT(unlink(filecap_path) == 0,
		    "Failed to remove file-capability executable: %d", errno);
}

static void test_protected_process_security(int kvm_fd)
{
	run_process_security_control(kvm_fd, false);
	run_process_security_control(kvm_fd, true);
	run_process_security_strict_control(kvm_fd, false);
	run_process_security_strict_control(kvm_fd, true);
	test_protected_process_credentials(kvm_fd);
	test_protected_process_landlock(kvm_fd);
}

struct process_observability_samples {
	uint64_t samples;
	uint64_t payload_samples;
	uint64_t payload_callchains;
	uint64_t payload_without_callchain;
	uint64_t callchain_ips;
	uint64_t zero_guest_kernel_samples;
};

struct process_observability_bpf_record {
	uint64_t pid_tgid;
	char comm[16];
	uint64_t hits;
};

#define PROCESS_OBSERVABILITY_BPF_INSN(CODE, DST, SRC, OFFSET, IMMEDIATE) \
	((struct bpf_insn) { \
		.code = CODE, \
		.dst_reg = DST, \
		.src_reg = SRC, \
		.off = OFFSET, \
		.imm = IMMEDIATE, \
	})

static struct perf_event_attr process_observability_perf_attr(void)
{
	struct perf_event_attr attr = {
		.type = PERF_TYPE_HARDWARE,
		.size = sizeof(attr),
		.config = PERF_COUNT_HW_INSTRUCTIONS,
		.sample_period = 250000,
		.sample_type = PERF_SAMPLE_IP | PERF_SAMPLE_TID |
			PERF_SAMPLE_CALLCHAIN,
		.disabled = 1,
		.exclude_kernel = 1,
		.exclude_hv = 1,
		.wakeup_events = 1,
	};

	return attr;
}

static int process_observability_perf_event_open(
		struct perf_event_attr *attr, pid_t pid)
{
	return syscall(SYS_perf_event_open, attr, pid, -1, -1, 0);
}

static int process_observability_uprobe_type(void)
{
	FILE *file;
	int saved_errno, type;

	file = fopen("/sys/bus/event_source/devices/uprobe/type", "re");
	if (!file)
		return -errno;
	if (fscanf(file, "%d", &type) != 1) {
		fclose(file);
		return -EINVAL;
	}
	saved_errno = errno;
	if (fclose(file))
		return -errno;
	errno = saved_errno;
	return type;
}

static unsigned long process_observability_file_offset(
		pid_t pid, unsigned long address)
{
	char maps_path[64], *line = NULL;
	size_t line_size = 0;
	unsigned long result = ULONG_MAX;
	FILE *maps;

	snprintf(maps_path, sizeof(maps_path), "/proc/%d/maps", pid);
	maps = fopen(maps_path, "re");
	TEST_ASSERT(maps, "Failed to open %s: %d", maps_path, errno);
	while (getline(&line, &line_size, maps) >= 0) {
		unsigned long start, end, offset;
		char permissions[5];

		if (sscanf(line, "%lx-%lx %4s %lx", &start, &end,
			   permissions, &offset) != 4)
			continue;
		if (address >= start && address < end) {
			TEST_ASSERT(strchr(permissions, 'x'),
				    "Payload mapping is not executable: %s", line);
			result = offset + address - start;
			break;
		}
	}
	free(line);
	TEST_ASSERT(fclose(maps) == 0, "Failed to close %s: %d", maps_path,
		    errno);
	TEST_ASSERT(result != ULONG_MAX,
		    "Failed to find payload address %#lx in %s", address,
		    maps_path);
	return result;
}

static int process_observability_bpf_map_create(void)
{
	union bpf_attr attr = {
		.map_type = BPF_MAP_TYPE_ARRAY,
		.key_size = sizeof(uint32_t),
		.value_size = sizeof(struct process_observability_bpf_record),
		.max_entries = 1,
	};

	return syscall(SYS_bpf, BPF_MAP_CREATE, &attr, sizeof(attr));
}

static int process_observability_bpf_program_load(int map_fd, char *log,
						   size_t log_size)
{
	static const char license[] = "GPL";
	struct bpf_insn instructions[] = {
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_ST | BPF_MEM | BPF_W,
			BPF_REG_10, 0, -4, 0),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_LD | BPF_DW | BPF_IMM,
			BPF_REG_1, BPF_PSEUDO_MAP_FD, 0, map_fd),
		PROCESS_OBSERVABILITY_BPF_INSN(0, 0, 0, 0, 0),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_ALU64 | BPF_MOV | BPF_X,
			BPF_REG_2, BPF_REG_10, 0, 0),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_ALU64 | BPF_ADD | BPF_K,
			BPF_REG_2, 0, 0, -4),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_JMP | BPF_CALL, 0, 0, 0,
			BPF_FUNC_map_lookup_elem),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_JMP | BPF_JEQ | BPF_K,
			BPF_REG_0, 0, 10, 0),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_ALU64 | BPF_MOV | BPF_X,
			BPF_REG_6, BPF_REG_0, 0, 0),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_JMP | BPF_CALL, 0, 0, 0,
			BPF_FUNC_get_current_pid_tgid),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_STX | BPF_MEM | BPF_DW,
			BPF_REG_6, BPF_REG_0,
			offsetof(struct process_observability_bpf_record, pid_tgid),
			0),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_ALU64 | BPF_MOV | BPF_X,
			BPF_REG_1, BPF_REG_6, 0, 0),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_ALU64 | BPF_ADD | BPF_K,
			BPF_REG_1, 0, 0,
			offsetof(struct process_observability_bpf_record, comm)),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_ALU64 | BPF_MOV | BPF_K,
			BPF_REG_2, 0, 0,
			sizeof(((struct process_observability_bpf_record *)0)->comm)),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_JMP | BPF_CALL, 0, 0, 0,
			BPF_FUNC_get_current_comm),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_LDX | BPF_MEM | BPF_DW,
			BPF_REG_7, BPF_REG_6,
			offsetof(struct process_observability_bpf_record, hits), 0),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_ALU64 | BPF_ADD | BPF_K,
			BPF_REG_7, 0, 0, 1),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_STX | BPF_MEM | BPF_DW,
			BPF_REG_6, BPF_REG_7,
			offsetof(struct process_observability_bpf_record, hits), 0),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_ALU64 | BPF_MOV | BPF_K,
			BPF_REG_0, 0, 0, 1),
		PROCESS_OBSERVABILITY_BPF_INSN(BPF_JMP | BPF_EXIT, 0, 0, 0, 0),
	};
	union bpf_attr attr = {
		.prog_type = BPF_PROG_TYPE_KPROBE,
		.insn_cnt = ARRAY_SIZE(instructions),
		.insns = (uintptr_t)instructions,
		.license = (uintptr_t)license,
		.log_level = 1,
		.log_size = log_size,
		.log_buf = (uintptr_t)log,
	};

	return syscall(SYS_bpf, BPF_PROG_LOAD, &attr, sizeof(attr));
}

static void process_observability_bpf_update(
		int map_fd, const struct process_observability_bpf_record *record)
{
	uint32_t key = 0;
	union bpf_attr attr;

	memset(&attr, 0, sizeof(attr));
	attr.map_fd = map_fd;
	attr.key = (uintptr_t)&key;
	attr.value = (uintptr_t)record;
	attr.flags = BPF_ANY;

	TEST_ASSERT(syscall(SYS_bpf, BPF_MAP_UPDATE_ELEM, &attr,
			    sizeof(attr)) == 0,
		    "BPF_MAP_UPDATE_ELEM failed: %d", errno);
}

static struct process_observability_bpf_record
process_observability_bpf_lookup(int map_fd)
{
	struct process_observability_bpf_record record;
	uint32_t key = 0;
	union bpf_attr attr;

	memset(&attr, 0, sizeof(attr));
	attr.map_fd = map_fd;
	attr.key = (uintptr_t)&key;
	attr.value = (uintptr_t)&record;

	TEST_ASSERT(syscall(SYS_bpf, BPF_MAP_LOOKUP_ELEM, &attr,
			    sizeof(attr)) == 0,
		    "BPF_MAP_LOOKUP_ELEM failed: %d", errno);
	return record;
}

static void process_observability_ring_copy(void *destination,
		const char *data, size_t data_size, uint64_t offset, size_t size)
{
	size_t position = offset & (data_size - 1);
	size_t first = size < data_size - position ? size : data_size - position;

	memcpy(destination, data + position, first);
	memcpy(destination + first, data, size - first);
}

static struct process_observability_samples
process_observability_read_samples(struct perf_event_mmap_page *metadata,
				     size_t page_size, bool protected,
				     const struct process_observability_target *target)
{
	struct process_observability_samples samples = {};
	uint64_t head = __atomic_load_n(&metadata->data_head, __ATOMIC_ACQUIRE);
	uint64_t tail = metadata->data_tail;
	size_t data_size = metadata->data_size ?: page_size * 32;
	const char *data = (char *)metadata +
		(metadata->data_offset ?: page_size);
	uint16_t expected_misc = protected ? PERF_RECORD_MISC_GUEST_USER :
		PERF_RECORD_MISC_USER;

	TEST_ASSERT(!(data_size & (data_size - 1)),
		    "Perf ring size %zu is not a power of two", data_size);
	while (tail < head) {
		struct perf_event_header header;
		unsigned char *record, *cursor, *end;
		uint64_t ip, nr;
		uint32_t pid, tid;
		bool payload_callchain = false;

		process_observability_ring_copy(&header, data, data_size, tail,
						sizeof(header));
		TEST_ASSERT(header.size >= sizeof(header) &&
			    header.size <= data_size,
			    "Invalid perf record size %u", header.size);
		record = malloc(header.size);
		TEST_ASSERT(record, "Failed to allocate perf record");
		process_observability_ring_copy(record, data, data_size, tail,
						header.size);
		tail += header.size;
		if (header.type != PERF_RECORD_SAMPLE) {
			free(record);
			continue;
		}

		cursor = record + sizeof(header);
		end = record + header.size;
		TEST_ASSERT(end - cursor >= sizeof(ip) + sizeof(pid) +
			    sizeof(tid) + sizeof(nr),
			    "Truncated perf sample");
		memcpy(&ip, cursor, sizeof(ip));
		cursor += sizeof(ip);
		memcpy(&pid, cursor, sizeof(pid));
		cursor += sizeof(pid);
		memcpy(&tid, cursor, sizeof(tid));
		cursor += sizeof(tid);
		memcpy(&nr, cursor, sizeof(nr));
		cursor += sizeof(nr);
		TEST_ASSERT(nr <= (uint64_t)(end - cursor) / sizeof(uint64_t),
			    "Truncated perf callchain with %llu entries",
			    (unsigned long long)nr);

		if (protected &&
		    (header.misc & PERF_RECORD_MISC_CPUMODE_MASK) ==
			PERF_RECORD_MISC_GUEST_KERNEL && !ip)
			samples.zero_guest_kernel_samples++;
		if (pid != (uint32_t)target->pid ||
		    tid != (uint32_t)target->tid ||
		    (header.misc & PERF_RECORD_MISC_CPUMODE_MASK) != expected_misc) {
			free(record);
			continue;
		}

		samples.samples++;
		for (uint64_t i = 0; i < nr; i++) {
			uint64_t callchain_ip;

			memcpy(&callchain_ip, cursor + i * sizeof(callchain_ip),
			       sizeof(callchain_ip));
			if (callchain_ip >= target->payload_start &&
			    callchain_ip < target->payload_end)
				payload_callchain = true;
			if (callchain_ip < PERF_CONTEXT_MAX)
				samples.callchain_ips++;
		}
		if (ip >= target->payload_start && ip < target->payload_end) {
			samples.payload_samples++;
			if (payload_callchain)
				samples.payload_callchains++;
			else
				samples.payload_without_callchain++;
		}
		free(record);
	}
	__atomic_store_n(&metadata->data_tail, head, __ATOMIC_RELEASE);
	return samples;
}

static void run_process_observability_control(int kvm_fd, bool protected,
					      int uprobe_type, int bpf_map_fd,
					      int bpf_program_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct process_observability_result baseline, instrumented;
	struct process_observability_target target;
	struct process_observability_samples samples;
	struct process_observability_bpf_record bpf_record = {};
	struct perf_event_mmap_page *metadata;
	struct perf_event_attr attr, uprobe_attr;
	uint64_t count, uprobe_count = 0;
	char executable[64];
	char command;
	unsigned long uprobe_offset;
	size_t page_size = sysconf(_SC_PAGESIZE);
	int control[2], ready[2], result[2], event_fd, status;
	int uprobe_fd = -1;
	pid_t child;

	TEST_ASSERT(pipe(control) == 0 && pipe(ready) == 0 && pipe(result) == 0,
		    "process-observability pipe() failed: %d", errno);
	child = fork();
	TEST_ASSERT(child >= 0, "process-observability fork() failed: %d",
		    errno);
	if (!child) {
		int fd = -1, ret;

		close(control[1]);
		close(ready[0]);
		close(result[0]);
		if (protected)
			fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		TEST_ASSERT(dup2(ready[1], PROCESS_OBSERVABILITY_READY_FD) ==
			    PROCESS_OBSERVABILITY_READY_FD,
			    "dup2() for observability ready fd failed: %d", errno);
		TEST_ASSERT(dup2(control[0], PROCESS_OBSERVABILITY_CONTROL_FD) ==
			    PROCESS_OBSERVABILITY_CONTROL_FD,
			    "dup2() for observability control fd failed: %d", errno);
		TEST_ASSERT(dup2(result[1], PROCESS_OBSERVABILITY_RESULT_FD) ==
			    PROCESS_OBSERVABILITY_RESULT_FD,
			    "dup2() for observability result fd failed: %d", errno);
		close(control[0]);
		close(ready[1]);
		close(result[1]);
		if (protected) {
			ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
			TEST_ASSERT(ret == 0,
				    KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		}
		execl("/proc/self/exe", "protected_task_test",
		      "--process-observability-target",
		      protected ? "protected" : "native", NULL);
		_exit(127);
	}

	close(control[0]);
	close(ready[1]);
	close(result[1]);
	TEST_ASSERT(!process_lifecycle_read(ready[0], &target, sizeof(target)),
		    "Failed to read %s observability target",
		    protected ? "protected" : "native");
	TEST_ASSERT(target.pid == child && target.tid == child &&
		    target.payload_start < target.payload_end,
		    "Invalid %s observability target identity",
		    protected ? "protected" : "native");

	command = 'B';
	TEST_ASSERT(!process_lifecycle_write(control[1], &command,
				     sizeof(command)) &&
		    !process_lifecycle_read(result[0], &baseline,
					    sizeof(baseline)),
		    "Failed to run %s observability baseline",
		    protected ? "protected" : "native");

	attr = process_observability_perf_attr();
	event_fd = process_observability_perf_event_open(&attr, child);
	TEST_ASSERT(event_fd >= 0,
		    "perf_event_open() for %s target failed: %d",
		    protected ? "protected" : "native", errno);
	metadata = mmap(NULL, page_size * 33, PROT_READ | PROT_WRITE,
			MAP_SHARED, event_fd, 0);
	TEST_ASSERT(metadata != MAP_FAILED,
		    "perf event mmap() for %s target failed: %d",
		    protected ? "protected" : "native", errno);
	if (uprobe_type >= 0) {
		uprobe_offset = process_observability_file_offset(
			child, target.payload_start);
		snprintf(executable, sizeof(executable), "/proc/%d/exe", child);
		uprobe_attr = (struct perf_event_attr) {
			.type = uprobe_type,
			.size = sizeof(uprobe_attr),
			.sample_period = 1,
			.disabled = 1,
			.uprobe_path = (uintptr_t)executable,
			.probe_offset = uprobe_offset,
		};
		uprobe_fd = process_observability_perf_event_open(&uprobe_attr,
								 child);
		TEST_ASSERT(uprobe_fd >= 0,
			    "perf_event_open() for %s uprobe failed: %d",
			    protected ? "protected" : "native", errno);
		if (bpf_program_fd >= 0) {
			process_observability_bpf_update(bpf_map_fd, &bpf_record);
			TEST_ASSERT(ioctl(uprobe_fd, PERF_EVENT_IOC_SET_BPF,
					  bpf_program_fd) == 0,
				    "Failed to attach BPF to %s uprobe: %d",
				    protected ? "protected" : "native", errno);
		}
		TEST_ASSERT(ioctl(uprobe_fd, PERF_EVENT_IOC_RESET, 0) == 0 &&
			    ioctl(uprobe_fd, PERF_EVENT_IOC_ENABLE, 0) == 0,
			    "Failed to enable %s uprobe: %d",
			    protected ? "protected" : "native", errno);
	}
	TEST_ASSERT(ioctl(event_fd, PERF_EVENT_IOC_RESET, 0) == 0 &&
		    ioctl(event_fd, PERF_EVENT_IOC_ENABLE, 0) == 0,
		    "Failed to enable %s perf event: %d",
		    protected ? "protected" : "native", errno);
	command = 'I';
	TEST_ASSERT(!process_lifecycle_write(control[1], &command,
				     sizeof(command)) &&
		    !process_lifecycle_read(result[0], &instrumented,
					    sizeof(instrumented)),
		    "Failed to run %s instrumented payload",
		    protected ? "protected" : "native");
	TEST_ASSERT(ioctl(event_fd, PERF_EVENT_IOC_DISABLE, 0) == 0,
		    "Failed to disable %s perf event: %d",
		    protected ? "protected" : "native", errno);
	if (uprobe_fd >= 0)
		TEST_ASSERT(ioctl(uprobe_fd, PERF_EVENT_IOC_DISABLE, 0) == 0,
			    "Failed to disable %s uprobe: %d",
			    protected ? "protected" : "native", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for %s observability target failed: %d",
		    protected ? "protected" : "native", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "%s observability target failed: %#x",
		    protected ? "Protected" : "Native", status);
	TEST_ASSERT(read(event_fd, &count, sizeof(count)) == sizeof(count) &&
		    count,
		    "%s retired-instruction event did not count payload work",
		    protected ? "Protected" : "Native");
	if (uprobe_fd >= 0) {
		TEST_ASSERT(read(uprobe_fd, &uprobe_count,
				 sizeof(uprobe_count)) == sizeof(uprobe_count),
			    "Failed to read %s uprobe count: %d",
			    protected ? "protected" : "native", errno);
		TEST_ASSERT(uprobe_count == PROCESS_OBSERVABILITY_CALLS,
			    "%s uprobe count is %llu, expected %d",
			    protected ? "Protected" : "Native",
			    (unsigned long long)uprobe_count,
			    PROCESS_OBSERVABILITY_CALLS);
		if (bpf_program_fd >= 0) {
			bpf_record = process_observability_bpf_lookup(bpf_map_fd);
			TEST_ASSERT(bpf_record.pid_tgid ==
				    ((uint64_t)child << 32 | (uint32_t)child),
				    "%s BPF task identity is %#llx, expected PID/TID %d",
				    protected ? "Protected" : "Native",
				    (unsigned long long)bpf_record.pid_tgid, child);
			TEST_ASSERT(!strcmp(bpf_record.comm, target.comm),
				    "%s BPF comm is '%s', expected '%s'",
				    protected ? "Protected" : "Native",
				    bpf_record.comm, target.comm);
			TEST_ASSERT(bpf_record.hits == uprobe_count,
				    "%s BPF hit count is %llu, expected %llu",
				    protected ? "Protected" : "Native",
				    (unsigned long long)bpf_record.hits,
				    (unsigned long long)uprobe_count);
		}
	}
	samples = process_observability_read_samples(metadata, page_size,
						 protected, &target);
	TEST_ASSERT(samples.samples && samples.payload_samples,
		    "%s perf samples did not identify payload [%#lx, %#lx): total=%llu payload=%llu",
		    protected ? "Protected" : "Native", target.payload_start,
		    target.payload_end, (unsigned long long)samples.samples,
		    (unsigned long long)samples.payload_samples);
	TEST_ASSERT(!samples.zero_guest_kernel_samples,
		    "Protected perf emitted %llu guest-kernel samples at IP zero",
		    (unsigned long long)samples.zero_guest_kernel_samples);
	if (protected)
		TEST_ASSERT(!samples.payload_callchains &&
			    !samples.callchain_ips &&
			    samples.payload_without_callchain ==
				samples.payload_samples,
			    "Protected samples exposed %llu unexpected callchain IPs",
			    (unsigned long long)samples.callchain_ips);
	else
		TEST_ASSERT(samples.payload_callchains,
			    "Native samples did not include the payload in callchains");

	printf("%s observability: baseline=%llu ns instrumented=%llu ns instructions=%llu samples=%llu payload=%llu callchains=%llu uprobes=%llu bpf=%llu\n",
	       protected ? "protected" : "native",
	       (unsigned long long)baseline.elapsed_ns,
	       (unsigned long long)instrumented.elapsed_ns,
	       (unsigned long long)count,
	       (unsigned long long)samples.samples,
	       (unsigned long long)samples.payload_samples,
	       (unsigned long long)samples.payload_callchains,
	       (unsigned long long)uprobe_count,
	       (unsigned long long)bpf_record.hits);
	TEST_ASSERT(munmap(metadata, page_size * 33) == 0,
		    "Failed to unmap perf ring: %d", errno);
	close(event_fd);
	if (uprobe_fd >= 0)
		close(uprobe_fd);
	close(control[1]);
	close(ready[0]);
	close(result[0]);
}

static void test_protected_process_observability(int kvm_fd)
{
	struct perf_event_attr attr = process_observability_perf_attr();
	char bpf_log[65536] = {};
	int bpf_map_fd = -1, bpf_program_fd = -1;
	int fd, uprobe_type;

	fd = process_observability_perf_event_open(&attr, 0);
	if (fd < 0 && (errno == EACCES || errno == EPERM || errno == ENOENT ||
		       errno == ENODEV || errno == EOPNOTSUPP || errno == ENOSYS)) {
		print_skip("Retired-instruction sampling is unavailable");
		return;
	}
	TEST_ASSERT(fd >= 0, "perf_event_open() probe failed: %d", errno);
	close(fd);

	uprobe_type = process_observability_uprobe_type();
	if (uprobe_type < 0)
		print_skip("Uprobe perf events and BPF attachment are unavailable");
	else {
		bpf_map_fd = process_observability_bpf_map_create();
		if (bpf_map_fd < 0 &&
		    (errno == EACCES || errno == EPERM || errno == ENOSYS ||
		     errno == EOPNOTSUPP))
			print_skip("BPF task-identity helpers are unavailable");
		else {
			TEST_ASSERT(bpf_map_fd >= 0,
				    "BPF_MAP_CREATE failed: %d", errno);
			bpf_program_fd = process_observability_bpf_program_load(
				bpf_map_fd, bpf_log, sizeof(bpf_log));
			TEST_ASSERT(bpf_program_fd >= 0,
				    "BPF_PROG_LOAD failed: %d\n%s", errno, bpf_log);
		}
	}
	run_process_observability_control(kvm_fd, false, uprobe_type,
					    bpf_map_fd, bpf_program_fd);
	run_process_observability_control(kvm_fd, true, uprobe_type,
					    bpf_map_fd, bpf_program_fd);
	if (bpf_program_fd >= 0)
		close(bpf_program_fd);
	if (bpf_map_fd >= 0)
		close(bpf_map_fd);
}

static int run_dso_stress_target(void)
{
	static const char soname[] = "libm.so.6";
	double (*cosine)(double angle);
	const char *error;
	void *handle;
	int i;

	handle = dlopen(soname, RTLD_NOW | RTLD_LOCAL | RTLD_NOLOAD);
	if (handle) {
		dlclose(handle);
		return 1;
	}
	dlerror();

	for (i = 0; i < 64; i++) {
		handle = dlopen(soname, RTLD_NOW | RTLD_LOCAL);
		if (!handle)
			return 2;

		dlerror();
		cosine = dlsym(handle, "cos");
		error = dlerror();
		if (error || !cosine || cosine(0.0) != 1.0) {
			dlclose(handle);
			return 3;
		}
		if (dlclose(handle))
			return 4;

		handle = dlopen(soname, RTLD_NOW | RTLD_LOCAL | RTLD_NOLOAD);
		if (handle) {
			dlclose(handle);
			return 5;
		}
		dlerror();
	}

	return 0;
}

static void test_dso_stress(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl("/proc/self/exe", "protected_task_test",
		      "--dso-stress-target", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Protected DSO stress failed: %#x", status);
}

static unsigned long get_anon_huge_pages(void *address)
{
	unsigned long target = (unsigned long)address;
	char line[256];
	bool found = false;
	FILE *smaps;

	smaps = fopen("/proc/self/smaps", "re");
	if (!smaps)
		return ULONG_MAX;

	while (fgets(line, sizeof(line), smaps)) {
		unsigned long start, end, size;

		if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
			found = target >= start && target < end;
			continue;
		}
		if (found && sscanf(line, "AnonHugePages: %lu kB", &size) == 1) {
			fclose(smaps);
			return size << 10;
		}
	}

	fclose(smaps);
	return ULONG_MAX;
}

static int madvise_collapse_retry(void *address, size_t size)
{
	int ret;

	ret = madvise(address, size, MADV_COLLAPSE);
	if (ret && errno == EAGAIN)
		ret = madvise(address, size, MADV_COLLAPSE);
	return ret;
}

static long get_protected_kvm_stat(const char *name)
{
	char path[PATH_MAX], prefix[32];
	struct dirent *entry;
	long value = -ENOENT;
	DIR *directory;
	FILE *file;
	int length;

	length = snprintf(prefix, sizeof(prefix), "%d-pt", getpid());
	if (length < 0 || length >= sizeof(prefix))
		return -ENAMETOOLONG;

	directory = opendir("/sys/kernel/debug/kvm");
	if (!directory)
		return -errno;

	while ((entry = readdir(directory))) {
		if (strncmp(entry->d_name, prefix, length))
			continue;

		length = snprintf(path, sizeof(path),
				  "/sys/kernel/debug/kvm/%s/%s",
				  entry->d_name, name);
		if (length < 0 || length >= sizeof(path)) {
			value = -ENAMETOOLONG;
			break;
		}

		file = fopen(path, "re");
		if (!file) {
			value = -errno;
			break;
		}
		if (fscanf(file, "%ld", &value) != 1)
			value = -EIO;
		fclose(file);
		break;
	}

	closedir(directory);
	return value;
}

static bool protected_kvm_vm_has_process_tid(const char *vm_name, pid_t pid)
{
	char path[PATH_MAX], task_path[64];
	struct dirent *entry;
	DIR *directory;
	bool found = false;
	int length;

	length = snprintf(path, sizeof(path), "/sys/kernel/debug/kvm/%s",
			  vm_name);
	TEST_ASSERT(length >= 0 && length < sizeof(path),
		    "Protected KVM debugfs path is too long");
	directory = opendir(path);
	TEST_ASSERT(directory, "opendir(%s) failed: %d", path, errno);
	while ((entry = readdir(directory))) {
		long tid;
		FILE *file;

		if (strncmp(entry->d_name, "vcpu", strlen("vcpu")))
			continue;
		length = snprintf(path, sizeof(path),
				  "/sys/kernel/debug/kvm/%s/%s/pid",
				  vm_name, entry->d_name);
		TEST_ASSERT(length >= 0 && length < sizeof(path),
			    "Protected vCPU debugfs path is too long");
		file = fopen(path, "re");
		TEST_ASSERT(file, "fopen(%s) failed: %d", path, errno);
		TEST_ASSERT(fscanf(file, "%ld", &tid) == 1,
			    "Failed to read %s", path);
		TEST_ASSERT(fclose(file) == 0, "fclose(%s) failed: %d",
			    path, errno);

		length = snprintf(task_path, sizeof(task_path),
				  "/proc/%d/task/%ld", pid, tid);
		TEST_ASSERT(length >= 0 && length < sizeof(task_path),
			    "Task path is too long");
		if (!access(task_path, F_OK)) {
			found = true;
			break;
		}
	}
	TEST_ASSERT(closedir(directory) == 0,
		    "closedir protected KVM VM failed: %d", errno);
	return found;
}

static size_t get_protected_kvm_vms(pid_t pid, char names[][NAME_MAX + 1],
				    size_t max_names)
{
	struct dirent *entry;
	size_t n = 0;
	DIR *directory;

	directory = opendir("/sys/kernel/debug/kvm");
	TEST_ASSERT(directory, "opendir KVM debugfs failed: %d", errno);
	while ((entry = readdir(directory))) {
		if (!strstr(entry->d_name, "-pt") ||
		    !protected_kvm_vm_has_process_tid(entry->d_name, pid))
			continue;
		TEST_ASSERT(n < max_names,
			    "Too many protected KVM VMs for process %d", pid);
		TEST_ASSERT(strlen(entry->d_name) <= NAME_MAX,
			    "Protected KVM VM name is too long");
		strcpy(names[n++], entry->d_name);
	}
	TEST_ASSERT(closedir(directory) == 0,
		    "closedir KVM debugfs failed: %d", errno);
	return n;
}

static size_t get_protected_kvm_vcpus(const char *vm_name)
{
	char path[PATH_MAX];
	struct dirent *entry;
	size_t n = 0;
	DIR *directory;
	int length;

	length = snprintf(path, sizeof(path), "/sys/kernel/debug/kvm/%s",
			  vm_name);
	TEST_ASSERT(length >= 0 && length < sizeof(path),
		    "Protected KVM debugfs path is too long");
	directory = opendir(path);
	TEST_ASSERT(directory, "opendir(%s) failed: %d", path, errno);
	while ((entry = readdir(directory)))
		if (!strncmp(entry->d_name, "vcpu", strlen("vcpu")))
			n++;
	TEST_ASSERT(closedir(directory) == 0,
		    "closedir protected KVM VM failed: %d", errno);
	return n;
}

static bool wait_for_protected_kvm_vcpus(pid_t pid, size_t min_vcpus,
					 size_t max_vcpus,
					 char vm_names[][NAME_MAX + 1],
					 size_t max_names)
{
	for (int i = 0; i < 1000; i++) {
		size_t nr_vms, nr_vcpus;

		nr_vms = get_protected_kvm_vms(pid, vm_names, max_names);
		if (nr_vms == 1) {
			nr_vcpus = get_protected_kvm_vcpus(vm_names[0]);
			if (nr_vcpus >= min_vcpus && nr_vcpus <= max_vcpus)
				return true;
		}
		usleep(1000);
	}
	return false;
}

static void *run_vcpu_reuse_thread(void *arg)
{
	return (void *)(uintptr_t)(syscall(SYS_gettid) <= 0);
}

static void *run_vcpu_high_water_thread(void *arg)
{
	if (syscall(SYS_gettid) <= 0)
		return (void *)1;
	barrier_wait();
	barrier_wait();
	return NULL;
}

static int run_vcpu_reuse_target(void)
{
	enum {
		NR_SEQUENTIAL_THREADS = 5000,
		NR_CONCURRENT_THREADS = 64,
		MAX_IDLE_VCPUS = 16,
	};
	char vm_names[2][NAME_MAX + 1];
	pthread_t threads[NR_CONCURRENT_THREADS];
	size_t nr_vms, nr_vcpus;
	void *thread_result;
	pthread_t thread;
	int i, ret;

	for (i = 0; i < NR_SEQUENTIAL_THREADS; i++) {
		ret = pthread_create(&thread, NULL, run_vcpu_reuse_thread, NULL);
		if (ret)
			return 10;
		ret = pthread_join(thread, &thread_result);
		if (ret || thread_result)
			return 11;
	}

	if (!wait_for_protected_kvm_vcpus(getpid(), 1, MAX_IDLE_VCPUS + 1,
					 vm_names, ARRAY_SIZE(vm_names)))
		return 12;

	ret = pthread_barrier_init(&thread_barrier, NULL,
				   NR_CONCURRENT_THREADS + 1);
	if (ret)
		return 14;
	for (i = 0; i < NR_CONCURRENT_THREADS; i++) {
		ret = pthread_create(&threads[i], NULL,
				     run_vcpu_high_water_thread, NULL);
		if (ret)
			return 15;
	}
	barrier_wait();
	nr_vms = get_protected_kvm_vms(getpid(), vm_names,
					ARRAY_SIZE(vm_names));
	if (nr_vms != 1)
		return 16;
	nr_vcpus = get_protected_kvm_vcpus(vm_names[0]);
	if (nr_vcpus != NR_CONCURRENT_THREADS + 1)
		return 17;
	barrier_wait();
	for (i = 0; i < NR_CONCURRENT_THREADS; i++) {
		ret = pthread_join(threads[i], &thread_result);
		if (ret || thread_result)
			return 18;
	}
	ret = pthread_barrier_destroy(&thread_barrier);
	if (ret)
		return 19;
	if (syscall(SYS_gettid) <= 0)
		return 20;
	if (!wait_for_protected_kvm_vcpus(getpid(), 1, 1, vm_names,
					 ARRAY_SIZE(vm_names)))
		return 21;
	return 0;
}

static void test_vcpu_reuse(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl("/proc/self/exe", "protected_task_test",
		      "--vcpu-reuse-target", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Protected vCPU reuse failed: %#x", status);
}

static int run_thp_stress_target(void)
{
	const unsigned long thp_size = 2UL << 20;
	const unsigned long page_size = 4096;
	const unsigned long dropped_page = 257;
	unsigned long base, aligned, end, i;
	long baseline_2m, pages_2m;
	void *reservation, *region;
	int status;
	pid_t child;

	baseline_2m = get_protected_kvm_stat("pages_2m");
	if (baseline_2m < 0)
		return KSFT_SKIP;

	reservation = mmap(NULL, thp_size * 2, PROT_READ | PROT_WRITE,
			   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (reservation == MAP_FAILED)
		return 10;

	base = (unsigned long)reservation;
	aligned = (base + thp_size - 1) & ~(thp_size - 1);
	end = base + thp_size * 2;
	if ((aligned > base && munmap((void *)base, aligned - base)) ||
	    (aligned + thp_size < end &&
	     munmap((void *)(aligned + thp_size), end - aligned - thp_size)))
		return 11;
	region = (void *)aligned;

	for (i = 0; i < thp_size / page_size; i++)
		*(unsigned long *)(aligned + i * page_size) = 0x5a000000UL + i;

	if (madvise_collapse_retry(region, thp_size)) {
		int saved_errno = errno;

		munmap(region, thp_size);
		if (saved_errno == EINVAL || saved_errno == EOPNOTSUPP)
			return KSFT_SKIP;
		return 12;
	}
	if (get_anon_huge_pages(region) != thp_size)
		return 13;
	for (i = 0; i < thp_size / page_size; i++)
		if (*(unsigned long *)(aligned + i * page_size) !=
		    0x5a000000UL + i)
			return 21;
	pages_2m = get_protected_kvm_stat("pages_2m");
	if (pages_2m <= baseline_2m)
		return 22;

	if (madvise((char *)region + dropped_page * page_size, page_size,
		    MADV_DONTNEED))
		return 14;
	if (get_anon_huge_pages(region) != 0)
		return 15;

	for (i = 0; i < thp_size / page_size; i++) {
		unsigned long expected = i == dropped_page ? 0 : 0x5a000000UL + i;

		if (*(unsigned long *)(aligned + i * page_size) != expected)
			return 16;
	}
	if (get_protected_kvm_stat("pages_2m") != baseline_2m)
		return 23;
	*(unsigned long *)(aligned + dropped_page * page_size) =
		0x5a000000UL + dropped_page;

	if (madvise_collapse_retry(region, thp_size))
		return 17;
	if (get_anon_huge_pages(region) != thp_size)
		return 18;
	for (i = 0; i < thp_size / page_size; i++) {
		unsigned long value = *(unsigned long *)(aligned + i * page_size);

		if (value != 0x5a000000UL + i)
			return 19;
	}
	if (get_protected_kvm_stat("pages_2m") <= baseline_2m)
		return 24;

	if (mprotect((char *)region + dropped_page * page_size, page_size,
		     PROT_READ))
		return 25;
	if (get_protected_kvm_stat("pages_2m") != baseline_2m)
		return 26;
	child = fork();
	if (child < 0)
		return 27;
	if (!child) {
		if (signal(SIGSEGV, SIG_DFL) == SIG_ERR)
			_exit(33);
		*(volatile unsigned long *)(aligned + dropped_page * page_size) = 0;
		_exit(0);
	}
	if (waitpid(child, &status, 0) != child || !WIFSIGNALED(status) ||
	    WTERMSIG(status) != SIGSEGV)
		return 28;
	if (*(unsigned long *)(aligned + dropped_page * page_size) !=
	    0x5a000000UL + dropped_page)
		return 29;
	if (mprotect((char *)region + dropped_page * page_size, page_size,
		     PROT_READ | PROT_WRITE) ||
	    madvise_collapse_retry(region, thp_size))
		return 30;
	for (i = 0; i < thp_size / page_size; i++)
		if (*(unsigned long *)(aligned + i * page_size) !=
		    0x5a000000UL + i)
			return 31;
	if (get_anon_huge_pages(region) != thp_size ||
	    get_protected_kvm_stat("pages_2m") <= baseline_2m)
		return 32;

	return munmap(region, thp_size) ? 20 : 0;
}

static void test_thp_stress(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		fd = create_context_with_features(kvm_fd, KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl("/proc/self/exe", "protected_task_test",
		      "--thp-stress-target", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	if (WIFEXITED(status) && WEXITSTATUS(status) == KSFT_SKIP) {
		print_skip("MADV_COLLAPSE is unavailable for protected THP stress");
		return;
	}
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Protected THP stress failed: %#x", status);
}

static unsigned long get_mapping_swap(void *address)
{
	unsigned long target = (unsigned long)address;
	char line[256];
	bool found = false;
	FILE *smaps;

	smaps = fopen("/proc/self/smaps", "re");
	if (!smaps)
		return ULONG_MAX;

	while (fgets(line, sizeof(line), smaps)) {
		unsigned long start, end, size;

		if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
			found = target >= start && target < end;
			continue;
		}
		if (found && sscanf(line, "Swap: %lu kB", &size) == 1) {
			fclose(smaps);
			return size << 10;
		}
	}

	fclose(smaps);
	return ULONG_MAX;
}

static unsigned long get_swap_total(void)
{
	char line[256];
	FILE *meminfo;

	meminfo = fopen("/proc/meminfo", "re");
	if (!meminfo)
		return ULONG_MAX;

	while (fgets(line, sizeof(line), meminfo)) {
		unsigned long size;

		if (sscanf(line, "SwapTotal: %lu kB", &size) == 1) {
			fclose(meminfo);
			return size << 10;
		}
	}

	fclose(meminfo);
	return ULONG_MAX;
}

static int open_current_memory_reclaim(void)
{
	char line[PATH_MAX], path[PATH_MAX];
	FILE *cgroup;
	int length;

	cgroup = fopen("/proc/self/cgroup", "re");
	if (!cgroup)
		return -1;

	while (fgets(line, sizeof(line), cgroup)) {
		if (strncmp(line, "0::", 3))
			continue;

		line[strcspn(line, "\n")] = '\0';
		length = snprintf(path, sizeof(path),
				  "/sys/fs/cgroup%s/memory.reclaim", line + 3);
		fclose(cgroup);
		if (length < 0 || length >= sizeof(path)) {
			errno = ENAMETOOLONG;
			return -1;
		}
		return open(path, O_WRONLY | O_CLOEXEC);
	}

	fclose(cgroup);
	errno = ENOENT;
	return -1;
}

static int reclaim_current_cgroup(size_t size)
{
	char request[64];
	ssize_t written;
	int fd, length, saved_errno;

	fd = open_current_memory_reclaim();
	if (fd < 0)
		return -errno;

	length = snprintf(request, sizeof(request), "%zu swappiness=max", size);
	if (length < 0 || length >= sizeof(request)) {
		close(fd);
		return -EOVERFLOW;
	}

	errno = 0;
	written = write(fd, request, length);
	saved_errno = errno;
	close(fd);
	if (written == length)
		return 0;
	return written < 0 ? -saved_errno : -EIO;
}

static void set_swap_pattern(void *mapping, size_t size)
{
	const size_t page_size = 4096;
	size_t offset;

	for (offset = 0; offset < size; offset += page_size) {
		unsigned long *page = (unsigned long *)((char *)mapping + offset);

		page[0] = 0x9e3779b97f4a7c15UL ^ offset;
		page[page_size / sizeof(*page) - 1] = ~page[0];
	}
}

static bool check_swap_pattern(void *mapping, size_t size)
{
	const size_t page_size = 4096;
	size_t offset;

	for (offset = 0; offset < size; offset += page_size) {
		unsigned long *page = (unsigned long *)((char *)mapping + offset);
		unsigned long expected = 0x9e3779b97f4a7c15UL ^ offset;

		if (page[0] != expected ||
		    page[page_size / sizeof(*page) - 1] != ~expected)
			return false;
	}
	return true;
}

static int run_swap_reclaim_target(void)
{
	const size_t size = 64UL << 20;
	unsigned long swap_total, swapped;
	void *mapping;
	int iteration, ret;

	swap_total = get_swap_total();
	if (swap_total == ULONG_MAX)
		return 60;
	if (!swap_total)
		return KSFT_SKIP;

	mapping = mmap(NULL, size, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED)
		return 61;
	set_swap_pattern(mapping, size);

	if (madvise(mapping, size, MADV_PAGEOUT)) {
		int saved_errno = errno;

		munmap(mapping, size);
		if (saved_errno == EINVAL || saved_errno == EOPNOTSUPP)
			return KSFT_SKIP;
		return 62;
	}
	swapped = get_mapping_swap(mapping);
	if (swapped == ULONG_MAX)
		return 63;
	if (!swapped)
		return 64;
	if (!check_swap_pattern(mapping, size))
		return 65;
	if (get_mapping_swap(mapping))
		return 66;

	swapped = 0;
	for (iteration = 0; iteration < 4 && !swapped; iteration++) {
		if (madvise(mapping, size, MADV_COLD)) {
			if (errno == EINVAL || errno == EOPNOTSUPP)
				return KSFT_SKIP;
			return 67;
		}

		ret = reclaim_current_cgroup(size);
		if (ret && ret != -EAGAIN) {
			if (ret == -ENOENT || ret == -EACCES || ret == -EROFS ||
			    ret == -EINVAL)
				return KSFT_SKIP;
			return 68;
		}
		swapped = get_mapping_swap(mapping);
		if (swapped == ULONG_MAX)
			return 69;
	}
	if (!swapped)
		return 70;
	if (!check_swap_pattern(mapping, size))
		return 71;
	if (get_mapping_swap(mapping))
		return 72;

	return munmap(mapping, size) ? 73 : 0;
}

static void test_swap_reclaim(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		fd = create_context_with_features(kvm_fd, KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl("/proc/self/exe", "protected_task_test",
		      "--swap-reclaim-target", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	if (WIFEXITED(status) && WEXITSTATUS(status) == KSFT_SKIP) {
		print_skip("Swap or cgroup reclaim is unavailable");
		return;
	}
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Protected swap/reclaim stress failed: %#x", status);
}

#define NUMA_MASK_WORDS 16
#define NUMA_TEST_PAGES 1024

static int get_allowed_numa_nodes(unsigned long mask[NUMA_MASK_WORDS])
{
	const unsigned long maxnode = sizeof(unsigned long) * CHAR_BIT *
		NUMA_MASK_WORDS;
	int mode;

	memset(mask, 0, sizeof(unsigned long) * NUMA_MASK_WORDS);
	if (syscall(SYS_get_mempolicy, &mode, mask, maxnode, NULL,
		    MPOL_F_MEMS_ALLOWED))
		return -errno;
	return 0;
}

static int find_numa_node(const unsigned long mask[NUMA_MASK_WORDS],
			  int previous)
{
	const int bits = sizeof(unsigned long) * CHAR_BIT;
	int node;

	for (node = previous + 1; node < NUMA_MASK_WORDS * bits; node++)
		if (mask[node / bits] & (1UL << (node % bits)))
			return node;
	return -1;
}

static void make_numa_mask(unsigned long mask[NUMA_MASK_WORDS], int node)
{
	const int bits = sizeof(unsigned long) * CHAR_BIT;

	memset(mask, 0, sizeof(unsigned long) * NUMA_MASK_WORDS);
	mask[node / bits] = 1UL << (node % bits);
}

static int query_numa_pages(void **pages, int *status, int expected_node)
{
	long ret;
	int i;

	ret = syscall(SYS_move_pages, 0, NUMA_TEST_PAGES, pages, NULL,
		      status, 0);
	if (ret)
		return ret < 0 ? -errno : -EIO;
	for (i = 0; i < NUMA_TEST_PAGES; i++)
		if (status[i] != expected_node)
			return -EIO;
	return 0;
}

static int move_numa_pages(void **pages, int *nodes, int *status, int node)
{
	long ret;
	int i;

	for (i = 0; i < NUMA_TEST_PAGES; i++)
		nodes[i] = node;
	ret = syscall(SYS_move_pages, 0, NUMA_TEST_PAGES, pages, nodes,
		      status, MPOL_MF_MOVE);
	if (ret)
		return ret < 0 ? -errno : -EIO;
	for (i = 0; i < NUMA_TEST_PAGES; i++)
		if (status[i] != node)
			return -EIO;
	return 0;
}

static void set_numa_pattern(void *mapping)
{
	const size_t page_size = 4096;
	int i;

	for (i = 0; i < NUMA_TEST_PAGES; i++) {
		unsigned long *page = mapping + i * page_size;

		page[0] = 0xd1b54a32d192ed03UL ^ i;
		page[page_size / sizeof(*page) - 1] = ~page[0];
	}
}

static bool check_numa_pattern(void *mapping)
{
	const size_t page_size = 4096;
	int i;

	for (i = 0; i < NUMA_TEST_PAGES; i++) {
		unsigned long *page = mapping + i * page_size;
		unsigned long expected = 0xd1b54a32d192ed03UL ^ i;

		if (page[0] != expected ||
		    page[page_size / sizeof(*page) - 1] != ~expected)
			return false;
	}
	return true;
}

static void set_numa_pages(void **pages, void *mapping)
{
	const size_t page_size = 4096;
	int i;

	for (i = 0; i < NUMA_TEST_PAGES; i++)
		pages[i] = mapping + i * page_size;
}

static int run_numa_policy_target(void)
{
	const unsigned long maxnode = sizeof(unsigned long) * CHAR_BIT *
		NUMA_MASK_WORDS;
	const size_t size = NUMA_TEST_PAGES * 4096UL;
	unsigned long allowed[NUMA_MASK_WORDS], actual[NUMA_MASK_WORDS];
	unsigned long bind[NUMA_MASK_WORDS];
	void *pages[NUMA_TEST_PAGES];
	int status[NUMA_TEST_PAGES];
	void *mapping;
	int mode, node, ret;

	ret = get_allowed_numa_nodes(allowed);
	if (ret)
		return 80;
	node = find_numa_node(allowed, -1);
	if (node < 0)
		return 81;
	make_numa_mask(bind, node);

	if (syscall(SYS_set_mempolicy, MPOL_BIND, bind, maxnode))
		return 82;
	memset(actual, 0, sizeof(actual));
	if (syscall(SYS_get_mempolicy, &mode, actual, maxnode, NULL, 0) ||
	    mode != MPOL_BIND || memcmp(actual, bind, sizeof(actual)))
		return 83;

	mapping = mmap(NULL, size, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED)
		return 84;
	if (syscall(SYS_mbind, mapping, size, MPOL_BIND, bind, maxnode, 0))
		return 85;
	memset(actual, 0, sizeof(actual));
	if (syscall(SYS_get_mempolicy, &mode, actual, maxnode, mapping,
		    MPOL_F_ADDR) ||
	    mode != MPOL_BIND || memcmp(actual, bind, sizeof(actual)))
		return 86;

	set_numa_pattern(mapping);
	set_numa_pages(pages, mapping);
	if (query_numa_pages(pages, status, node) ||
	    !check_numa_pattern(mapping))
		return 87;
	if (syscall(SYS_set_mempolicy, MPOL_DEFAULT, NULL, 0))
		return 88;

	return munmap(mapping, size) ? 89 : 0;
}

struct numa_access_args {
	void *mapping;
	atomic_bool stop;
	atomic_bool failed;
};

static void *run_numa_access(void *opaque)
{
	struct numa_access_args *args = opaque;

	while (!atomic_load_explicit(&args->stop, memory_order_acquire)) {
		if (!check_numa_pattern(args->mapping)) {
			atomic_store_explicit(&args->failed, true,
					      memory_order_release);
			break;
		}
	}
	return NULL;
}

static int run_numa_migration_target(void)
{
	const unsigned long maxnode = sizeof(unsigned long) * CHAR_BIT *
		NUMA_MASK_WORDS;
	const size_t size = NUMA_TEST_PAGES * 4096UL;
	unsigned long allowed[NUMA_MASK_WORDS], source_mask[NUMA_MASK_WORDS];
	void *pages[NUMA_TEST_PAGES];
	int nodes[NUMA_TEST_PAGES], status[NUMA_TEST_PAGES];
	struct numa_access_args args;
	pthread_t thread;
	void *mapping;
	int iteration, node, ret, source, target;

	ret = get_allowed_numa_nodes(allowed);
	if (ret)
		return 90;
	source = find_numa_node(allowed, -1);
	target = find_numa_node(allowed, source);
	if (source < 0 || target < 0)
		return KSFT_SKIP;
	make_numa_mask(source_mask, source);

	mapping = mmap(NULL, size, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED)
		return 91;
	if (syscall(SYS_mbind, mapping, size, MPOL_BIND, source_mask,
		    maxnode, 0))
		return 92;
	set_numa_pattern(mapping);
	set_numa_pages(pages, mapping);
	if (query_numa_pages(pages, status, source))
		return 93;

	args.mapping = mapping;
	atomic_init(&args.stop, false);
	atomic_init(&args.failed, false);
	ret = pthread_create(&thread, NULL, run_numa_access, &args);
	if (ret)
		return 94;

	ret = move_numa_pages(pages, nodes, status, target);
	if (ret == -EPERM || ret == -EACCES) {
		ret = KSFT_SKIP;
		goto stop;
	}
	if (ret) {
		ret = 95;
		goto stop;
	}
	if (syscall(SYS_mbind, mapping, size, MPOL_BIND, source_mask,
		    maxnode, MPOL_MF_MOVE | MPOL_MF_STRICT) ||
	    query_numa_pages(pages, status, source)) {
		ret = 96;
		goto stop;
	}

	for (iteration = 0; iteration < 32; iteration++) {
		node = iteration & 1 ? source : target;
		if (move_numa_pages(pages, nodes, status, node)) {
			ret = 97;
			goto stop;
		}
	}
	ret = check_numa_pattern(mapping) ? 0 : 98;

stop:
	atomic_store_explicit(&args.stop, true, memory_order_release);
	if (pthread_join(thread, NULL))
		return 99;
	if (atomic_load_explicit(&args.failed, memory_order_acquire))
		return 100;
	if (munmap(mapping, size))
		return 101;
	return ret;
}

static int run_numa_target(int kvm_fd, const char *target)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		fd = create_context_with_features(kvm_fd, KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl("/proc/self/exe", "protected_task_test", target, NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	return status;
}

static void test_numa_stress(int kvm_fd)
{
	int status;

	status = run_numa_target(kvm_fd, "--numa-policy-target");
	if (WIFEXITED(status) && WEXITSTATUS(status) == KSFT_SKIP)
		print_skip("NUMA policy is unavailable");
	else
		TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
			    "Protected NUMA policy stress failed: %#x", status);

	status = run_numa_target(kvm_fd, "--numa-migration-target");
	if (WIFEXITED(status) && WEXITSTATUS(status) == KSFT_SKIP) {
		print_skip("Cross-node NUMA migration is unavailable");
		return;
	}
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Protected NUMA migration stress failed: %#x", status);
}

#define HOTPLUG_TEST_SIZE	(64UL << 20)
#define PAGEMAP_PRESENT		(1ULL << 63)
#define PAGEMAP_PFN_MASK	((1ULL << 55) - 1)

struct memory_hotplug_target_info {
	__u64 address;
	__u64 size;
};

struct memory_hotplug_access_args {
	void *mapping;
	size_t size;
	atomic_bool stop;
	atomic_bool failed;
};

struct mapped_memory_block {
	unsigned long long index;
	size_t pages;
	bool movable;
};

static void *run_memory_hotplug_access(void *opaque)
{
	struct memory_hotplug_access_args *args = opaque;

	while (!atomic_load_explicit(&args->stop, memory_order_acquire)) {
		if (!check_swap_pattern(args->mapping, args->size)) {
			atomic_store_explicit(&args->failed, true,
					      memory_order_release);
			break;
		}
	}
	return NULL;
}

static int run_memory_hotplug_target(int socket_fd)
{
	struct memory_hotplug_target_info info;
	struct memory_hotplug_access_args args;
	pthread_t thread;
	char command;
	ssize_t length;
	void *mapping;
	int ret;

	mapping = mmap(NULL, HOTPLUG_TEST_SIZE, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED)
		return 180;
	set_swap_pattern(mapping, HOTPLUG_TEST_SIZE);

	args.mapping = mapping;
	args.size = HOTPLUG_TEST_SIZE;
	atomic_init(&args.stop, false);
	atomic_init(&args.failed, false);
	ret = pthread_create(&thread, NULL, run_memory_hotplug_access, &args);
	if (ret) {
		munmap(mapping, HOTPLUG_TEST_SIZE);
		return 181;
	}

	info.address = (uintptr_t)mapping;
	info.size = HOTPLUG_TEST_SIZE;
	length = send(socket_fd, &info, sizeof(info), MSG_NOSIGNAL);
	if (length == sizeof(info)) {
		do {
			length = recv(socket_fd, &command, sizeof(command), 0);
		} while (length < 0 && errno == EINTR);
	}

	atomic_store_explicit(&args.stop, true, memory_order_release);
	ret = pthread_join(thread, NULL);
	if (ret) {
		munmap(mapping, HOTPLUG_TEST_SIZE);
		return 182;
	}
	if (length != sizeof(command))
		ret = 183;
	else if (command == 'S')
		ret = KSFT_SKIP;
	else if (command != 'C' ||
		 atomic_load_explicit(&args.failed, memory_order_acquire) ||
		 !check_swap_pattern(mapping, HOTPLUG_TEST_SIZE))
		ret = 184;
	else
		ret = 0;
	if (munmap(mapping, HOTPLUG_TEST_SIZE) && !ret)
		ret = 185;
	return ret;
}

static int hotplug_read_file(const char *path, char *buffer, size_t size)
{
	int fd, saved_errno;
	ssize_t length;

	if (size < 2)
		return -EINVAL;
	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return -errno;
	length = read(fd, buffer, size - 1);
	saved_errno = errno;
	close(fd);
	if (length < 0)
		return -saved_errno;
	if (length == size - 1)
		return -EOVERFLOW;
	buffer[length] = '\0';
	return 0;
}

static int hotplug_write_file(const char *path, const char *value)
{
	size_t size = strlen(value);
	int fd, saved_errno;
	ssize_t length;

	fd = open(path, O_WRONLY | O_CLOEXEC);
	if (fd < 0)
		return -errno;
	length = write(fd, value, size);
	saved_errno = errno;
	close(fd);
	if (length < 0)
		return -saved_errno;
	return length == size ? 0 : -EIO;
}

static int hotplug_block_path(char *path, size_t size,
			      unsigned long long index, const char *file)
{
	int length;

	length = snprintf(path, size,
			  "/sys/devices/system/memory/memory%llu/%s",
			  index, file);
	return length < 0 || length >= size ? -ENAMETOOLONG : 0;
}

static int hotplug_get_block_size(unsigned long long *block_size)
{
	char buffer[64], *end;
	unsigned long long value;
	int ret;

	ret = hotplug_read_file("/sys/devices/system/memory/block_size_bytes",
				buffer, sizeof(buffer));
	if (ret)
		return ret;
	errno = 0;
	value = strtoull(buffer, &end, 16);
	if (errno || end == buffer || !value)
		return -EINVAL;
	while (*end == ' ' || *end == '\t' || *end == '\n')
		end++;
	if (*end || value % 4096)
		return -EINVAL;
	*block_size = value;
	return 0;
}

static int hotplug_check_block(struct mapped_memory_block *block,
			       bool *eligible)
{
	char path[PATH_MAX], buffer[128];
	int ret;

	*eligible = false;
	ret = hotplug_block_path(path, sizeof(path), block->index, "removable");
	if (ret)
		return ret;
	ret = hotplug_read_file(path, buffer, sizeof(buffer));
	if (ret == -ENOENT)
		return 0;
	if (ret)
		return ret;
	if (buffer[0] != '1')
		return 0;

	ret = hotplug_block_path(path, sizeof(path), block->index, "state");
	if (ret)
		return ret;
	ret = hotplug_read_file(path, buffer, sizeof(buffer));
	if (ret)
		return ret;
	if (strncmp(buffer, "online", strlen("online")))
		return 0;

	ret = hotplug_block_path(path, sizeof(path), block->index,
				 "valid_zones");
	if (ret)
		return ret;
	ret = hotplug_read_file(path, buffer, sizeof(buffer));
	if (!ret)
		block->movable = strstr(buffer, "Movable") != NULL;
	else if (ret != -ENOENT)
		return ret;
	*eligible = true;
	return 0;
}

static int hotplug_set_block_state(unsigned long long index,
				   const char *state)
{
	char path[PATH_MAX];
	int ret;

	ret = hotplug_block_path(path, sizeof(path), index, "state");
	if (ret)
		return ret;
	return hotplug_write_file(path, state);
}

static int hotplug_block_has_state(unsigned long long index,
				   const char *state)
{
	char path[PATH_MAX], buffer[32];
	int ret;

	ret = hotplug_block_path(path, sizeof(path), index, "state");
	if (ret)
		return ret;
	ret = hotplug_read_file(path, buffer, sizeof(buffer));
	if (ret)
		return ret;
	return strncmp(buffer, state, strlen(state)) ? 0 : 1;
}

static int hotplug_collect_mapping_blocks(
		pid_t pid, const struct memory_hotplug_target_info *info,
		unsigned long long block_size,
		struct mapped_memory_block **ret_blocks, size_t *ret_count)
{
	const unsigned long page_size = 4096;
	struct mapped_memory_block *blocks;
	char path[64];
	size_t i, nr_blocks = 0, pages;
	int fd, length, ret = 0;
	bool have_pfn = false;

	if (!info->size || info->address % page_size ||
	    info->size % page_size || info->address > UINT64_MAX - info->size)
		return -EINVAL;
	pages = info->size / page_size;
	blocks = calloc(pages, sizeof(*blocks));
	if (!blocks)
		return -ENOMEM;
	length = snprintf(path, sizeof(path), "/proc/%d/pagemap", pid);
	if (length < 0 || length >= sizeof(path)) {
		ret = -ENAMETOOLONG;
		goto out;
	}
	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0) {
		ret = -errno;
		goto out;
	}

	for (i = 0; i < pages; i++) {
		unsigned long long address = info->address + i * page_size;
		unsigned long long entry, pfn, index;
		off_t offset = address / page_size * sizeof(entry);
		ssize_t nread;
		size_t j;

		nread = pread(fd, &entry, sizeof(entry), offset);
		if (nread != sizeof(entry)) {
			ret = nread < 0 ? -errno : -EIO;
			break;
		}
		if (!(entry & PAGEMAP_PRESENT))
			continue;
		pfn = entry & PAGEMAP_PFN_MASK;
		if (!pfn)
			continue;
		have_pfn = true;
		if (pfn > ULLONG_MAX / page_size) {
			ret = -EOVERFLOW;
			break;
		}
		index = pfn * page_size / block_size;
		for (j = 0; j < nr_blocks; j++)
			if (blocks[j].index == index)
				break;
		if (j == nr_blocks)
			blocks[nr_blocks++].index = index;
		blocks[j].pages++;
	}
	close(fd);
	if (!ret && !have_pfn)
		ret = -EACCES;
out:
	if (ret) {
		free(blocks);
		return ret;
	}
	*ret_blocks = blocks;
	*ret_count = nr_blocks;
	return 0;
}

static int hotplug_count_mapping_pages(
		pid_t pid, const struct memory_hotplug_target_info *info,
		unsigned long long block_size, unsigned long long index,
		size_t *count)
{
	struct mapped_memory_block *blocks;
	size_t i, nr_blocks;
	int ret;

	ret = hotplug_collect_mapping_blocks(pid, info, block_size, &blocks,
					     &nr_blocks);
	if (ret)
		return ret;
	*count = 0;
	for (i = 0; i < nr_blocks; i++)
		if (blocks[i].index == index) {
			*count = blocks[i].pages;
			break;
		}
	free(blocks);
	return 0;
}

static void test_memory_hotplug(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct memory_hotplug_target_info info;
	struct mapped_memory_block *blocks = NULL;
	unsigned long long block_size, selected = 0;
	const char *skip_reason = NULL;
	size_t i, nr_blocks = 0, pages = 0;
	int sockets[2], status = 0, ret, operation_error = 0;
	int online_error = 0, state, pass;
	ssize_t length;
	pid_t child;
	char command = 'S';
	bool offlined = false, send_failed = false;

	ret = hotplug_get_block_size(&block_size);
	if (ret) {
		print_skip("Memory hotplug sysfs is unavailable");
		return;
	}
	TEST_ASSERT(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sockets) == 0,
		    "socketpair() failed: %d", errno);
	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		char fd_string[32];
		int fd;

		close(sockets[0]);
		fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		snprintf(fd_string, sizeof(fd_string), "%d", sockets[1]);
		execl("/proc/self/exe", "protected_task_test",
		      "--memory-hotplug-target", fd_string, NULL);
		_exit(127);
	}

	close(sockets[1]);
	do {
		length = recv(sockets[0], &info, sizeof(info), MSG_WAITALL);
	} while (length < 0 && errno == EINTR);
	if (length != sizeof(info)) {
		operation_error = length < 0 ? -errno : -EIO;
		goto stop;
	}

	ret = hotplug_collect_mapping_blocks(child, &info, block_size,
					     &blocks, &nr_blocks);
	if (ret == -EACCES || ret == -EPERM) {
		skip_reason = "PFNs are unavailable for protected memory hotplug stress";
		goto stop;
	}
	if (ret) {
		operation_error = ret;
		goto stop;
	}

	for (i = 0; i < nr_blocks; i++) {
		bool eligible;

		ret = hotplug_check_block(&blocks[i], &eligible);
		if (ret) {
			operation_error = ret;
			goto stop;
		}
		if (!eligible)
			blocks[i].pages = 0;
	}

	for (pass = 0; pass < 2 && !offlined; pass++) {
		for (i = 0; i < nr_blocks; i++) {
			if (!blocks[i].pages || blocks[i].movable != !pass)
				continue;
			ret = hotplug_count_mapping_pages(child, &info, block_size,
							 blocks[i].index, &pages);
			if (ret) {
				operation_error = ret;
				goto stop;
			}
			if (!pages)
				continue;
			ret = hotplug_set_block_state(blocks[i].index, "offline");
			if (ret == -EACCES || ret == -EPERM || ret == -EROFS) {
				skip_reason = "Memory hotplug sysfs is not writable";
				goto stop;
			}
			if (ret == -EBUSY || ret == -EINVAL || ret == -EAGAIN)
				continue;
			if (ret) {
				operation_error = ret;
				goto stop;
			}
			selected = blocks[i].index;
			offlined = true;
			command = 'C';
			state = hotplug_block_has_state(selected, "offline");
			if (state != 1) {
				operation_error = state < 0 ? state : -EIO;
				goto stop;
			}
			ret = hotplug_count_mapping_pages(child, &info, block_size,
							 selected, &pages);
			if (ret || pages) {
				operation_error = ret ? ret : -EIO;
				goto stop;
			}
			break;
		}
	}
	if (!offlined)
		skip_reason = "No mapped online memory block could be offlined";

stop:
	length = send(sockets[0], &command, sizeof(command), MSG_NOSIGNAL);
	if (length != sizeof(command)) {
		send_failed = true;
		kill(child, SIGKILL);
	}
	close(sockets[0]);
	if (waitpid(child, &status, 0) != child) {
		operation_error = operation_error ?: -errno;
		kill(child, SIGKILL);
		waitpid(child, &status, 0);
	}
	if (offlined) {
		online_error = hotplug_set_block_state(selected, "online");
		if (!online_error) {
			state = hotplug_block_has_state(selected, "online");
			if (state != 1)
				online_error = state < 0 ? state : -EIO;
		}
	}
	free(blocks);

	TEST_ASSERT(!online_error,
		    "Failed to restore memory%llu online: %d",
		    selected, -online_error);
	TEST_ASSERT(!operation_error,
		    "Protected memory hotplug operation failed: %d",
		    -operation_error);
	TEST_ASSERT(!send_failed, "Protected memory hotplug target stopped early");
	if (!offlined) {
		TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == KSFT_SKIP,
			    "Protected memory hotplug skip failed: %#x", status);
		print_skip(skip_reason ?: "Memory hotplug is unavailable");
		return;
	}
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Protected memory hotplug stress failed: %#x", status);
}

static int run_oom_target(bool protected, int ready_fd)
{
	const size_t size = 512UL << 20;
	const size_t page_size = 4096;
	atomic_char *mapping;
	char ready = 'R';
	size_t offset;

	if (process_lifecycle_mode_mismatch(protected))
		return 109;
	if (write(ready_fd, &ready, sizeof(ready)) != sizeof(ready))
		return 110;
	close(ready_fd);

	mapping = mmap(NULL, size, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED)
		return 111;
	for (offset = 0; offset < size; offset += page_size)
		atomic_store_explicit(&mapping[offset], offset >> PAGE_SHIFT,
				      memory_order_relaxed);

	return 112;
}

static void test_cgroup_oom_mode(int kvm_fd, bool protected)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	char root[PATH_MAX], name[64], fd_string[32], ready;
	long oom_before, oom_kill_before, oom_after, oom_kill_after;
	char *cgroup;
	int pipefd[2], status;
	pid_t child;

	if (cg_find_unified_root(root, sizeof(root), NULL)) {
		print_skip("Cgroup v2 is unavailable for protected OOM stress");
		return;
	}
	snprintf(name, sizeof(name), "%s_task_oom_%d",
		 protected ? "protected" : "native", getpid());
	cgroup = cg_name(root, name);
	TEST_ASSERT(cgroup, "Failed to allocate cgroup path");
	if (cg_create(cgroup)) {
		int saved_errno = errno;

		free(cgroup);
		if (saved_errno == EACCES || saved_errno == EROFS ||
		    saved_errno == EPERM) {
			print_skip("Cgroup creation is unavailable for protected OOM stress");
			return;
		}
		TEST_FAIL("Failed to create OOM cgroup: %d", saved_errno);
	}

	if (cg_write(cgroup, "memory.max", "128M") ||
	    cg_write(cgroup, "memory.swap.max", "0") ||
	    cg_write(cgroup, "memory.oom.group", "1")) {
		cg_destroy(cgroup);
		free(cgroup);
		print_skip("Memory controller limits are unavailable");
		return;
	}
	oom_before = cg_read_key_long(cgroup, "memory.events", "oom ");
	oom_kill_before = cg_read_key_long(cgroup, "memory.events", "oom_kill ");
	TEST_ASSERT(oom_before >= 0 && oom_kill_before >= 0,
		    "Failed to read initial OOM counters");
	TEST_ASSERT(pipe(pipefd) == 0, "pipe() failed: %d", errno);

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd = -1, ret;

		close(pipefd[0]);
		if (cg_enter_current(cgroup))
			_exit(113);
		if (protected) {
			fd = create_context_with_features(kvm_fd,
						  KVM_PROTECTED_TASK_FEATURE_EXEC);
			ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
			TEST_ASSERT(ret == 0,
				    KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		}
		snprintf(fd_string, sizeof(fd_string), "%d", pipefd[1]);
		execl("/proc/self/exe", "protected_task_test", "--oom-target",
		      protected ? "protected" : "native", fd_string, NULL);
		_exit(127);
	}

	close(pipefd[1]);
	TEST_ASSERT(read(pipefd[0], &ready, sizeof(ready)) == sizeof(ready) &&
		    ready == 'R', "%s OOM target did not start",
		    protected ? "Protected" : "Native");
	close(pipefd[0]);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL,
		    "%s OOM target status was %#x",
		    protected ? "Protected" : "Native", status);
	oom_after = cg_read_key_long(cgroup, "memory.events", "oom ");
	oom_kill_after = cg_read_key_long(cgroup, "memory.events", "oom_kill ");
	TEST_ASSERT(oom_after > oom_before && oom_kill_after > oom_kill_before,
		    "OOM counters did not increase: %ld/%ld -> %ld/%ld",
		    oom_before, oom_kill_before, oom_after, oom_kill_after);
	TEST_ASSERT(cg_destroy(cgroup) == 0, "Failed to destroy OOM cgroup");
	free(cgroup);
}

static void test_cgroup_oom(int kvm_fd)
{
	test_cgroup_oom_mode(kvm_fd, false);
	test_cgroup_oom_mode(kvm_fd, true);
}

enum uffd_invalidation_op {
	UFFD_INVALIDATE_REMOVE,
	UFFD_INVALIDATE_REMAP,
	UFFD_INVALIDATE_UNMAP,
};

struct uffd_invalidation_args {
	enum uffd_invalidation_op op;
	void *source;
	void *target;
	size_t size;
	long result;
	int error;
};

static void *run_uffd_invalidation(void *opaque)
{
	struct uffd_invalidation_args *args = opaque;

	errno = 0;
	switch (args->op) {
	case UFFD_INVALIDATE_REMOVE:
		args->result = madvise(args->source, args->size, MADV_DONTNEED);
		break;
	case UFFD_INVALIDATE_REMAP:
		args->result = (long)mremap(args->source, args->size, args->size,
					     MREMAP_MAYMOVE | MREMAP_FIXED,
					     args->target);
		break;
	case UFFD_INVALIDATE_UNMAP:
		args->result = munmap(args->source, args->size);
		break;
	}
	if (args->result == -1)
		args->error = errno;
	return NULL;
}

static int register_uffd_missing(int uffd, void *address, size_t size)
{
	struct uffdio_register reg = {
		.range.start = (unsigned long)address,
		.range.len = size,
		.mode = UFFDIO_REGISTER_MODE_MISSING,
	};

	return ioctl(uffd, UFFDIO_REGISTER, &reg);
}

static int unregister_uffd(int uffd, void *address, size_t size)
{
	struct uffdio_range range = {
		.start = (unsigned long)address,
		.len = size,
	};

	return ioctl(uffd, UFFDIO_UNREGISTER, &range);
}

static int check_uffd_invalidation_event(int uffd,
					 struct uffd_invalidation_args *args,
					 unsigned int expected_event)
{
	struct uffd_msg msg, unmap_msg;
	pthread_t thread;
	ssize_t bytes;
	int ret;

	ret = pthread_create(&thread, NULL, run_uffd_invalidation, args);
	if (ret)
		return 30;
	do {
		bytes = read(uffd, &msg, sizeof(msg));
	} while (bytes < 0 && errno == EINTR);
	if (bytes != sizeof(msg))
		return 31;
	if (expected_event == UFFD_EVENT_REMAP) {
		do {
			bytes = read(uffd, &unmap_msg, sizeof(unmap_msg));
		} while (bytes < 0 && errno == EINTR);
		if (bytes != sizeof(unmap_msg))
			return 37;
	}
	if (pthread_join(thread, NULL))
		return 32;
	if (args->error)
		return 33;
	if (msg.event != expected_event)
		return 34;

	if (expected_event == UFFD_EVENT_REMAP) {
		if (msg.arg.remap.from != (unsigned long)args->source ||
		    msg.arg.remap.to != (unsigned long)args->target ||
		    msg.arg.remap.len != args->size ||
		    args->result != (long)args->target)
			return 35;
		if (unmap_msg.event != UFFD_EVENT_UNMAP ||
		    unmap_msg.arg.remove.start != (unsigned long)args->source ||
		    unmap_msg.arg.remove.end !=
			    (unsigned long)args->source + args->size)
			return 38;
	} else if (msg.arg.remove.start != (unsigned long)args->source ||
		   msg.arg.remove.end != (unsigned long)args->source + args->size ||
		   args->result) {
		return 36;
	}

	return 0;
}

static int run_uffd_invalidation_target(void)
{
	const uint64_t features = UFFD_FEATURE_EVENT_REMOVE |
		UFFD_FEATURE_EVENT_REMAP | UFFD_FEATURE_EVENT_UNMAP;
	const size_t page_size = 4096;
	struct uffdio_api api = {
		.api = UFFD_API,
		.features = features,
	};
	struct uffd_invalidation_args args = {
		.size = page_size,
	};
	void *mapping, *target;
	int ret, uffd;

	uffd = syscall(SYS_userfaultfd, O_CLOEXEC | UFFD_USER_MODE_ONLY);
	if (uffd < 0)
		return 40;
	if (ioctl(uffd, UFFDIO_API, &api))
		return 41;
	if ((api.features & features) != features)
		return KSFT_SKIP;

	mapping = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED)
		return 42;
	*(unsigned long *)mapping = 0x12345678;
	if (register_uffd_missing(uffd, mapping, page_size))
		return 43;
	args.op = UFFD_INVALIDATE_REMOVE;
	args.source = mapping;
	args.target = NULL;
	args.result = -1;
	args.error = 0;
	ret = check_uffd_invalidation_event(uffd, &args, UFFD_EVENT_REMOVE);
	if (ret)
		return ret;
	if (unregister_uffd(uffd, mapping, page_size) ||
	    munmap(mapping, page_size))
		return 44;

	mapping = mmap(NULL, page_size * 2, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED)
		return 45;
	target = (char *)mapping + page_size;
	*(unsigned long *)mapping = 0x87654321;
	if (munmap(target, page_size) ||
	    register_uffd_missing(uffd, mapping, page_size))
		return 46;
	args.op = UFFD_INVALIDATE_REMAP;
	args.source = mapping;
	args.target = target;
	args.result = -1;
	args.error = 0;
	ret = check_uffd_invalidation_event(uffd, &args, UFFD_EVENT_REMAP);
	if (ret)
		return ret;
	if (*(unsigned long *)target != 0x87654321 ||
	    unregister_uffd(uffd, target, page_size) ||
	    munmap(target, page_size))
		return 47;

	mapping = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mapping == MAP_FAILED)
		return 48;
	*(unsigned long *)mapping = 0xabcdef;
	if (register_uffd_missing(uffd, mapping, page_size))
		return 49;
	args.op = UFFD_INVALIDATE_UNMAP;
	args.source = mapping;
	args.target = NULL;
	args.result = -1;
	args.error = 0;
	ret = check_uffd_invalidation_event(uffd, &args, UFFD_EVENT_UNMAP);
	if (ret)
		return ret;

	return close(uffd) ? 50 : 0;
}

static void test_uffd_invalidation(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		fd = create_context_with_features(kvm_fd, KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl("/proc/self/exe", "protected_task_test",
		      "--uffd-invalidation-target", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	if (WIFEXITED(status) && WEXITSTATUS(status) == KSFT_SKIP) {
		print_skip("UFFD invalidation events are unavailable");
		return;
	}
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Protected UFFD invalidation stress failed: %#x", status);
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

static void assert_hidden_mapping_fully_resident(pid_t pid,
						 unsigned long address)
{
	unsigned long header_size = ULONG_MAX, size = ULONG_MAX;
	unsigned long rss = ULONG_MAX, locked = ULONG_MAX;
	char path[64], line[256];
	bool found = false;
	FILE *smaps;

	snprintf(path, sizeof(path), "/proc/%d/smaps", pid);
	smaps = fopen(path, "re");
	TEST_ASSERT(smaps, "fopen(%s) failed: %d", path, errno);
	while (fgets(line, sizeof(line), smaps)) {
		unsigned long start, end, value;

		if (sscanf(line, "%lx-%lx", &start, &end) == 2) {
			if (found)
				break;
			found = start == address;
			if (found)
				header_size = (end - start) >> 10;
			continue;
		}
		if (!found)
			continue;
		if (sscanf(line, "Size: %lu kB", &value) == 1)
			size = value;
		else if (sscanf(line, "Rss: %lu kB", &value) == 1)
			rss = value;
		else if (sscanf(line, "Locked: %lu kB", &value) == 1)
			locked = value;
	}
	TEST_ASSERT(fclose(smaps) == 0, "fclose(%s) failed: %d", path, errno);
	TEST_ASSERT(found && size != ULONG_MAX && rss != ULONG_MAX &&
		    locked != ULONG_MAX,
		    "Missing smaps accounting for hidden mapping %#lx in task %d",
		    address, pid);
	TEST_ASSERT(size == header_size && rss == size && locked == size,
		    "Hidden mapping %#lx in task %d has Size/Rss/Locked "
		    "%lu/%lu/%lu kB, expected %lu/%lu/%lu kB",
		    address, pid, size, rss, locked,
		    header_size, header_size, header_size);
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
					    int address_fd,
					    const struct protected_avx_features *features)
{
	const unsigned long sentinel = 0x0123456789abcdef;
	const unsigned char xmm_sentinel[16] = {
		0x98, 0xba, 0xdc, 0xfe, 0x10, 0x32, 0x54, 0x76,
		0xfe, 0xca, 0xad, 0x0b, 0xef, 0xbe, 0xad, 0xde,
	};
	const size_t xstate_bv_offset = 512;
	const size_t xmm15_offset =
		offsetof(struct user_fpregs_struct, xmm_space) + 15 * 16;
	const size_t mxcsr_offset = offsetof(struct user_fpregs_struct, mxcsr);
	unsigned int eax, ebx, xstate_capacity, edx;
	struct user_regs_struct original_regs, regs;
	struct ptrace_syscall_info syscall_info;
	unsigned char *expected_xstate, *original_xstate, *xstate;
	unsigned long breakpoint_rip, event_child;
	__u64 opmask = 0x5aa5;
	__u64 xstate_bv;
	__u32 mxcsr = 0x5f80, pkru = 3U << 30;
	__u16 fcw = 0x077f;
	struct iovec iov;
	size_t xstate_size;
	siginfo_t siginfo;
	long dr6, instruction, value;
	int status;

	__cpuid_count(0xd, 0, eax, ebx, xstate_capacity, edx);
	TEST_ASSERT(xstate_capacity >= xmm15_offset + sizeof(xmm_sentinel),
		    "Invalid xstate buffer size: %u", xstate_capacity);
	xstate = malloc(xstate_capacity);
	expected_xstate = malloc(xstate_capacity);
	original_xstate = malloc(xstate_capacity);
	TEST_ASSERT(xstate && expected_xstate && original_xstate,
		    "Failed to allocate xstate buffers");

	TEST_ASSERT(ptrace(PTRACE_ATTACH, child, NULL, NULL) == 0,
		    "PTRACE_ATTACH failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after PTRACE_ATTACH failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status), "Ptraced child did not stop: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_SETOPTIONS, child, NULL,
			   PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK) == 0,
		    "PTRACE_SETOPTIONS failed: %d", errno);

	errno = 0;
	value = ptrace(PTRACE_PEEKDATA, child, (void *)address, NULL);
	TEST_ASSERT(value == -1 && errno == EIO,
		    "PTRACE_PEEKDATA returned %ld/%d, expected -1/%d",
		    value, errno, EIO);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS failed: %d", errno);
	original_regs = regs;
	iov.iov_base = xstate;
	iov.iov_len = xstate_capacity;
	TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_GETREGSET NT_X86_XSTATE failed: %d", errno);
	xstate_size = iov.iov_len;
	TEST_ASSERT(xstate_size >= xmm15_offset + sizeof(xmm_sentinel),
		    "NT_X86_XSTATE is too short: %zu", xstate_size);
	TEST_ASSERT(xstate_size >= xstate_bv_offset + sizeof(xstate_bv),
		    "NT_X86_XSTATE has no header: %zu", xstate_size);
	memcpy(original_xstate, xstate, xstate_size);
	memcpy(&xstate_bv, xstate + xstate_bv_offset, sizeof(xstate_bv));
	xstate_bv |= 3;
	memcpy(xstate + xstate_bv_offset, &xstate_bv, sizeof(xstate_bv));
	memcpy(xstate, &fcw, sizeof(fcw));
	memcpy(xstate + mxcsr_offset, &mxcsr, sizeof(mxcsr));
	memcpy(xstate + xmm15_offset, xmm_sentinel, sizeof(xmm_sentinel));
	if (features->avx) {
		TEST_ASSERT(xstate_size >= features->ymm_offset + 256,
			    "NT_X86_XSTATE has no YMM component: %zu", xstate_size);
		xstate_bv |= 1ULL << 2;
		memset(xstate + features->ymm_offset + 15 * 16, 0xa5, 16);
	}
	if (features->avx512) {
		TEST_ASSERT(xstate_size >= features->opmask_offset + 64 &&
			    xstate_size >= features->zmm_hi256_offset + 512 &&
			    xstate_size >= features->hi16_zmm_offset + 1024,
			    "NT_X86_XSTATE has incomplete AVX-512 state: %zu",
			    xstate_size);
		xstate_bv |= 0xe0;
		memcpy(xstate + features->opmask_offset + 7 * 8,
		       &opmask, sizeof(opmask));
		memset(xstate + features->zmm_hi256_offset + 15 * 32, 0x3c, 32);
		memset(xstate + features->hi16_zmm_offset + 15 * 64, 0xc3, 64);
	}
	if (features->pku) {
		TEST_ASSERT(xstate_size >= features->pkru_offset + sizeof(pkru),
			    "NT_X86_XSTATE has no PKRU component: %zu", xstate_size);
		xstate_bv |= 1ULL << 9;
		memcpy(xstate + features->pkru_offset, &pkru, sizeof(pkru));
	}
	memcpy(xstate + xstate_bv_offset, &xstate_bv, sizeof(xstate_bv));
	memcpy(expected_xstate, xstate, xstate_size);
	TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_SETREGSET NT_X86_XSTATE failed: %d", errno);
	iov.iov_len = xstate_capacity;
	TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_GETREGSET after xstate write failed: %d", errno);
	TEST_ASSERT(iov.iov_len == xstate_size,
		    "NT_X86_XSTATE size changed from %zu to %zu",
		    xstate_size, iov.iov_len);
	TEST_ASSERT(!memcmp(xstate, expected_xstate, xstate_size),
		    "PTRACE_SETREGSET did not update complete xstate");
	memcpy(expected_xstate, xstate, xstate_size);
	breakpoint_rip = regs.rip;
	regs.r15 = sentinel;
	regs.r14 = 0x1122334455667788;
	regs.r13 = 0x2233445566778899;
	regs.r12 = 0x33445566778899aa;
	regs.rbp = 0x445566778899aabb;
	regs.rbx = 0x5566778899aabbcc;
	regs.r11 = 0x66778899aabbccdd;
	regs.r10 = 0x778899aabbccddee;
	regs.r9 = 0x8899aabbccddeeff;
	regs.r8 = 0x99aabbccddeeff00;
	regs.rax = 0xaabbccddeeff0011;
	regs.rcx = 0xbbccddeeff001122;
	regs.rdx = 0xccddeeff00112233;
	regs.rsi = 0xddeeff0011223344;
	regs.rdi = 0xeeff001122334455;
	regs.rsp -= 16;
	TEST_ASSERT(ptrace(PTRACE_SETREGS, child, NULL, &regs) == 0,
		    "PTRACE_SETREGS failed: %d", errno);
	{
		struct user_regs_struct expected_regs = regs;

		TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
			    "PTRACE_GETREGS after write failed: %d", errno);
		TEST_ASSERT(!memcmp(&regs, &expected_regs,
				    offsetof(struct user_regs_struct, orig_rax)) &&
			    regs.rsp == expected_regs.rsp,
			    "PTRACE_SETREGS did not update complete GPR state");
	}
	errno = 0;
	instruction = ptrace(PTRACE_PEEKTEXT, child, (void *)breakpoint_rip, NULL);
	TEST_ASSERT(instruction != -1 || !errno,
		    "PTRACE_PEEKTEXT failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_POKETEXT, child, (void *)breakpoint_rip,
			   (void *)((instruction & ~0xffUL) | 0xcc)) == 0,
		    "PTRACE_POKETEXT breakpoint failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT to software breakpoint failed: %d", errno);
	TEST_ASSERT(write(address_fd, &address, sizeof(address)) == sizeof(address),
		    "Failed to release software-breakpoint target: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after software breakpoint failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
		    "Software breakpoint produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0,
		    "PTRACE_GETSIGINFO after software breakpoint failed: %d", errno);
	TEST_ASSERT(siginfo.si_signo == SIGTRAP && siginfo.si_code == SI_KERNEL,
		    "Software breakpoint produced signal %d/%d",
		    siginfo.si_signo, siginfo.si_code);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS after software breakpoint failed: %d", errno);
	TEST_ASSERT(regs.r14 == 0x1122334455667788 &&
		    regs.r13 == 0x2233445566778899 &&
		    regs.r12 == 0x33445566778899aa &&
		    regs.rbp == 0x445566778899aabb &&
		    regs.rbx == 0x5566778899aabbcc &&
		    regs.r11 == 0x66778899aabbccdd &&
		    regs.r10 == 0x778899aabbccddee &&
		    regs.r9 == 0x8899aabbccddeeff &&
		    regs.r8 == 0x99aabbccddeeff00 &&
		    regs.rax == 0xaabbccddeeff0011 &&
		    regs.rcx == 0xbbccddeeff001122 &&
		    regs.rdx == 0xccddeeff00112233 &&
		    regs.rsi == 0xddeeff0011223344 &&
		    regs.rdi == 0xeeff001122334455 &&
		    regs.r15 == sentinel && regs.rsp == original_regs.rsp - 16,
		    "Complete GPR state changed at software breakpoint");
	TEST_ASSERT(regs.rip == breakpoint_rip + 1,
		    "Software breakpoint produced RIP %#llx, expected %#lx",
		    regs.rip, breakpoint_rip + 1);
	TEST_ASSERT(ptrace(PTRACE_POKETEXT, child, (void *)breakpoint_rip,
			   (void *)((instruction & ~0xffUL) | 0xf1)) == 0,
		    "PTRACE_POKETEXT ICEBP failed: %d", errno);
	regs.rip = breakpoint_rip;
	TEST_ASSERT(ptrace(PTRACE_SETREGS, child, NULL, &regs) == 0,
		    "PTRACE_SETREGS before ICEBP failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT to ICEBP failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after ICEBP failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
		    "ICEBP produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS after ICEBP failed: %d", errno);
	TEST_ASSERT(regs.rip == breakpoint_rip + 1,
		    "ICEBP produced RIP %#llx, expected %#lx",
		    regs.rip, breakpoint_rip + 1);
	TEST_ASSERT(ptrace(PTRACE_POKETEXT, child, (void *)breakpoint_rip,
			   (void *)instruction) == 0,
		    "PTRACE_POKETEXT restore failed: %d", errno);
	regs = original_regs;
	regs.r15 = sentinel;
	TEST_ASSERT(ptrace(PTRACE_SETREGS, child, NULL, &regs) == 0,
		    "PTRACE_SETREGS after software breakpoint failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_POKEUSER, child,
			   (void *)offsetof(struct user, u_debugreg[0]),
			   breakpoint_rip) == 0,
		    "PTRACE_POKEUSER DR0 failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_POKEUSER, child,
			   (void *)offsetof(struct user, u_debugreg[7]), 1) == 0,
		    "PTRACE_POKEUSER DR7 failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after hardware breakpoint failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
		    "Hardware breakpoint produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0,
		    "PTRACE_GETSIGINFO failed: %d", errno);
	TEST_ASSERT(siginfo.si_signo == SIGTRAP && siginfo.si_code == TRAP_HWBKPT,
		    "Hardware breakpoint produced signal %d/%d",
		    siginfo.si_signo, siginfo.si_code);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS after hardware breakpoint failed: %d", errno);
	TEST_ASSERT(regs.rip == breakpoint_rip,
		    "Hardware breakpoint advanced RIP to %#llx from %#lx",
		    regs.rip, breakpoint_rip);
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
	errno = 0;
	TEST_ASSERT(ptrace(PTRACE_SINGLEBLOCK, child, NULL, NULL) == -1 &&
		    errno == EIO,
		    "PTRACE_SINGLEBLOCK returned unexpected result: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_SINGLESTEP, child, NULL, NULL) == 0,
		    "PTRACE_SINGLESTEP failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after single-step failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
		    "Single-step produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0,
		    "PTRACE_GETSIGINFO after single-step failed: %d", errno);
	TEST_ASSERT(siginfo.si_signo == SIGTRAP && siginfo.si_code == TRAP_TRACE,
		    "Single-step produced signal %d/%d",
		    siginfo.si_signo, siginfo.si_code);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS after single-step failed: %d", errno);
	TEST_ASSERT(regs.rip != breakpoint_rip,
		    "Single-step did not advance RIP from %#lx", breakpoint_rip);
	TEST_ASSERT(regs.r15 == sentinel,
		    "PTRACE_SETREGS value was not preserved: %#llx",
		    regs.r15);
	regs.r15 = original_regs.r15;
	TEST_ASSERT(ptrace(PTRACE_SETREGS, child, NULL, &regs) == 0,
		    "PTRACE_SETREGS GPR restore failed: %d", errno);
	iov.iov_len = xstate_capacity;
	TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_GETREGSET after single-step failed: %d", errno);
	TEST_ASSERT(iov.iov_len == xstate_size,
		    "NT_X86_XSTATE size changed after single-step: %zu", iov.iov_len);
	TEST_ASSERT(!memcmp(xstate, expected_xstate, xstate_size),
		    "NT_X86_XSTATE changed after single-step");
	iov.iov_base = original_xstate;
	iov.iov_len = xstate_size;
	TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_SETREGSET xstate restore failed: %d", errno);
	iov.iov_base = xstate;
	iov.iov_len = xstate_capacity;
	TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_GETREGSET after xstate restore failed: %d", errno);
	TEST_ASSERT(iov.iov_len == xstate_size &&
		    !memcmp(xstate, original_xstate, xstate_size),
		    "PTRACE_SETREGSET did not restore complete xstate");
	TEST_ASSERT(ptrace(PTRACE_SYSCALL, child, NULL, NULL) == 0,
		    "PTRACE_SYSCALL to entry failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at syscall entry failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | 0x80),
		    "Syscall entry produced unexpected status: %#x", status);
	memset(&syscall_info, 0, sizeof(syscall_info));
	value = ptrace(PTRACE_GET_SYSCALL_INFO, child, sizeof(syscall_info),
		       &syscall_info);
	TEST_ASSERT(value > 0, "PTRACE_GET_SYSCALL_INFO at entry failed: %d", errno);
	TEST_ASSERT(syscall_info.op == PTRACE_SYSCALL_INFO_ENTRY &&
		    syscall_info.entry.nr == SYS_write &&
		    syscall_info.entry.args[0] == STDOUT_FILENO &&
		    syscall_info.entry.args[1] == address &&
		    syscall_info.entry.args[2] == 1,
		    "Unexpected syscall entry: op=%u nr=%llu args=%llu/%#llx/%llu",
		    syscall_info.op, syscall_info.entry.nr,
		    syscall_info.entry.args[0], syscall_info.entry.args[1],
		    syscall_info.entry.args[2]);
	TEST_ASSERT(ptrace(PTRACE_SYSCALL, child, NULL, NULL) == 0,
		    "PTRACE_SYSCALL to exit failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at syscall exit failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | 0x80),
		    "Syscall exit produced unexpected status: %#x", status);
	memset(&syscall_info, 0, sizeof(syscall_info));
	value = ptrace(PTRACE_GET_SYSCALL_INFO, child, sizeof(syscall_info),
		       &syscall_info);
	TEST_ASSERT(value > 0, "PTRACE_GET_SYSCALL_INFO at exit failed: %d", errno);
	TEST_ASSERT(syscall_info.op == PTRACE_SYSCALL_INFO_EXIT &&
		    syscall_info.exit.is_error &&
		    syscall_info.exit.rval == -EFAULT,
		    "Unexpected syscall exit: op=%u error=%u rval=%lld",
		    syscall_info.op, syscall_info.exit.is_error,
		    syscall_info.exit.rval);
	TEST_ASSERT(kill(child, SIGUSR1) == 0,
		    "kill(SIGUSR1) failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT to signal stop failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at signal stop failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGUSR1,
		    "Signal delivery produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0,
		    "PTRACE_GETSIGINFO at signal stop failed: %d", errno);
	TEST_ASSERT(siginfo.si_signo == SIGUSR1 && siginfo.si_code == SI_USER &&
		    siginfo.si_pid == getpid(),
		    "Signal stop produced signal %d/%d from %d",
		    siginfo.si_signo, siginfo.si_code, siginfo.si_pid);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT to fork event failed: %d", errno);
	for (;;) {
		TEST_ASSERT(waitpid(child, &status, 0) == child,
			    "waitpid() before fork event failed: %d", errno);
		TEST_ASSERT(WIFSTOPPED(status),
			    "Tracee exited before fork event: %#x", status);
		if (WSTOPSIG(status) == SIGTRAP &&
		    (unsigned int)status >> 16 == PTRACE_EVENT_FORK)
			break;
		TEST_ASSERT((unsigned int)status >> 16 == 0,
			    "Unexpected ptrace event before fork: %#x", status);
		TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL,
				   (void *)(long)WSTOPSIG(status)) == 0,
			    "PTRACE_CONT before fork event failed: %d", errno);
	}
	TEST_ASSERT(ptrace(PTRACE_GETEVENTMSG, child, NULL, &event_child) == 0,
		    "PTRACE_GETEVENTMSG at fork failed: %d", errno);
	TEST_ASSERT(event_child > 0, "Fork event returned child %lu", event_child);
	TEST_ASSERT(waitpid(event_child, &status, __WALL) == event_child,
		    "waitpid() for traced fork child failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status),
		    "Traced fork child did not stop: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_DETACH, event_child, NULL, NULL) == 0,
		    "PTRACE_DETACH fork child failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_DETACH, child, NULL, NULL) == 0,
		    "PTRACE_DETACH failed: %d", errno);
	free(expected_xstate);
	free(original_xstate);
	free(xstate);
}

static void test_exec_event_ptrace(pid_t child, unsigned long address,
				    int address_fd,
				    const struct protected_avx_features *features)
{
	struct user_regs_struct regs;
	unsigned long event_msg;
	long value;
	int status;

	TEST_ASSERT(ptrace(PTRACE_ATTACH, child, NULL, NULL) == 0,
		    "PTRACE_ATTACH before exec failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() before exec failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status),
		    "Ptraced exec child did not stop: %#x", status);
	errno = 0;
	value = ptrace(PTRACE_PEEKDATA, child, (void *)address, NULL);
	TEST_ASSERT(value == -1 && errno == EIO,
		    "PTRACE_PEEKDATA before exec returned %ld/%d, expected -1/%d",
		    value, errno, EIO);
	if (features->amx) {
		unsigned char *expected_xstate, *original_xstate, *xstate;
		unsigned int eax, ebx, xstate_capacity, edx;
		const size_t xstate_bv_offset = 512;
		__u64 xstate_bv;
		struct iovec iov;
		siginfo_t siginfo;
		size_t xstate_size;
		unsigned int i;

		__cpuid_count(0xd, 0, eax, ebx, xstate_capacity, edx);
		xstate = malloc(xstate_capacity);
		expected_xstate = malloc(xstate_capacity);
		original_xstate = malloc(xstate_capacity);
		TEST_ASSERT(xstate && expected_xstate && original_xstate,
			    "Failed to allocate AMX xstate buffers");
		iov.iov_base = xstate;
		iov.iov_len = xstate_capacity;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0,
			    "PTRACE_GETREGSET before AMX write failed: %d", errno);
		xstate_size = iov.iov_len;
		TEST_ASSERT(xstate_size >= features->tilecfg_offset + 64 &&
			    xstate_size >= features->tiledata_offset + 8192,
			    "NT_X86_XSTATE has incomplete AMX state: %zu", xstate_size);
		memcpy(original_xstate, xstate, xstate_size);
		memset(xstate + features->tilecfg_offset, 0, 64);
		xstate[features->tilecfg_offset] = 1;
		for (i = 0; i < 8; i++) {
			__u16 colsb = 64;

			memcpy(xstate + features->tilecfg_offset + 16 + i * 2,
			       &colsb, sizeof(colsb));
			xstate[features->tilecfg_offset + 48 + i] = 16;
		}
		memset(xstate + features->tiledata_offset, 0xa5, 8192);
		memcpy(&xstate_bv, xstate + xstate_bv_offset, sizeof(xstate_bv));
		xstate_bv |= 0x60000;
		memcpy(xstate + xstate_bv_offset, &xstate_bv, sizeof(xstate_bv));
		memcpy(expected_xstate, xstate, xstate_size);
		iov.iov_len = xstate_size;
		TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0,
			    "PTRACE_SETREGSET AMX write failed: %d", errno);
		iov.iov_len = xstate_capacity;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0 && iov.iov_len == xstate_size &&
			    !memcmp(xstate, expected_xstate, xstate_size),
			    "PTRACE_SETREGSET did not update complete AMX state");
		TEST_ASSERT(ptrace(PTRACE_SINGLESTEP, child, NULL, NULL) == 0,
			    "PTRACE_SINGLESTEP with AMX state failed: %d", errno);
		TEST_ASSERT(waitpid(child, &status, 0) == child,
			    "waitpid() after AMX single-step failed: %d", errno);
		TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
			    "AMX single-step produced unexpected status: %#x", status);
		TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0 &&
			    siginfo.si_code == TRAP_TRACE,
			    "AMX single-step produced unexpected siginfo: %d/%d",
			    siginfo.si_signo, siginfo.si_code);
		iov.iov_len = xstate_capacity;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0 && iov.iov_len == xstate_size &&
			    !memcmp(xstate, expected_xstate, xstate_size),
			    "Complete AMX state changed after single-step");
		iov.iov_base = original_xstate;
		iov.iov_len = xstate_size;
		TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0,
			    "PTRACE_SETREGSET AMX restore failed: %d", errno);
		iov.iov_base = xstate;
		iov.iov_len = xstate_capacity;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0 && iov.iov_len == xstate_size &&
			    !memcmp(xstate, original_xstate, xstate_size),
			    "PTRACE_SETREGSET did not restore complete AMX state");
		free(original_xstate);
		free(expected_xstate);
		free(xstate);
	}
	if (features->shstk) {
		unsigned long original_ssp, ssp;
		struct iovec iov = {
			.iov_base = &original_ssp,
			.iov_len = sizeof(original_ssp),
		};
		siginfo_t siginfo;

		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0 && iov.iov_len == sizeof(original_ssp),
			    "PTRACE_GETREGSET NT_X86_SHSTK failed: %d", errno);
		TEST_ASSERT(original_ssp && original_ssp <= ULONG_MAX - 8,
			    "Invalid protected SSP: %#lx", original_ssp);
		ssp = original_ssp + 8;
		iov.iov_base = &ssp;
		iov.iov_len = sizeof(ssp);
		TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0,
			    "PTRACE_SETREGSET NT_X86_SHSTK failed: %d", errno);
		ssp = 0;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0 && ssp == original_ssp + 8,
			    "PTRACE_SETREGSET did not update SSP");
		TEST_ASSERT(ptrace(PTRACE_SINGLESTEP, child, NULL, NULL) == 0,
			    "PTRACE_SINGLESTEP with edited SSP failed: %d", errno);
		TEST_ASSERT(waitpid(child, &status, 0) == child,
			    "waitpid() after SSP single-step failed: %d", errno);
		TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
			    "SSP single-step produced unexpected status: %#x", status);
		TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0 &&
			    siginfo.si_code == TRAP_TRACE,
			    "SSP single-step produced unexpected siginfo: %d/%d",
			    siginfo.si_signo, siginfo.si_code);
		ssp = 0;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0 && ssp == original_ssp + 8,
			    "Protected SSP changed after single-step");
		iov.iov_base = &original_ssp;
		TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0,
			    "PTRACE_SETREGSET SSP restore failed: %d", errno);
		ssp = 0;
		iov.iov_base = &ssp;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0 && ssp == original_ssp,
			    "PTRACE_SETREGSET did not restore SSP");
	}
	TEST_ASSERT(ptrace(PTRACE_SETOPTIONS, child, NULL,
			   PTRACE_O_TRACEEXEC) == 0,
		    "PTRACE_SETOPTIONS before exec failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT to exec event failed: %d", errno);
	TEST_ASSERT(write(address_fd, &address, sizeof(address)) == sizeof(address),
		    "Failed to release exec-event target: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at exec event failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP &&
		    (unsigned int)status >> 16 == PTRACE_EVENT_EXEC,
		    "Exec event produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETEVENTMSG, child, NULL, &event_msg) == 0,
		    "PTRACE_GETEVENTMSG at exec failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS at exec failed: %d", errno);
	TEST_ASSERT(regs.rip, "Exec event has a zero RIP");
	TEST_ASSERT(ptrace(PTRACE_DETACH, child, NULL, NULL) == 0,
		    "PTRACE_DETACH after exec failed: %d", errno);
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
	char expected_output[4096] = {}, output[sizeof(expected_output)] = {};
	char main_vm[NAME_MAX + 1] = {};
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
		char vm_names[2][NAME_MAX + 1];
		ssize_t nread;
		size_t n, nr_vms;
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
		nr_vms = get_protected_kvm_vms(target, vm_names,
					       ARRAY_SIZE(vm_names));
		TEST_ASSERT(nr_vms == 1,
			    "Task %d has %zu protected KVM VMs", target, nr_vms);
		for (size_t j = 0; j < n; j++)
			assert_hidden_mapping_fully_resident(target, addresses[j]);
		if (stage == 0) {
			TEST_ASSERT(target == child && n == 1,
				    "Initial helper has unexpected mappings");
			main_address = address = addresses[0];
			strcpy(main_vm, vm_names[0]);
		} else if (stage == 1 || stage == 3) {
			unsigned long main_addresses[2];
			char main_vms[2][NAME_MAX + 1];
			size_t main_n;
			size_t main_nr_vms;

			TEST_ASSERT(target != child && n == 1,
				    "Process child has unexpected mappings");
			address = addresses[0];
			main_n = get_hidden_mappings(child, main_addresses,
						     ARRAY_SIZE(main_addresses));
			TEST_ASSERT(main_n == 1,
					    "Main helper has unexpected mappings");
			main_address = main_addresses[0];
			main_nr_vms = get_protected_kvm_vms(child, main_vms,
							   ARRAY_SIZE(main_vms));
			TEST_ASSERT(main_nr_vms == 1,
					    "Main helper has %zu protected KVM VMs",
					    main_nr_vms);
			strcpy(main_vm, main_vms[0]);
			TEST_ASSERT(strcmp(vm_names[0], main_vm),
					    "Process child shared the main KVM VM");
		} else if (stage == 2) {
			TEST_ASSERT(target != child && n == 1,
				    "vfork child has unexpected mappings");
			address = addresses[0];
			TEST_ASSERT(address == main_address,
				    "vfork child did not share the hidden mapping");
			TEST_ASSERT(!strcmp(vm_names[0], main_vm),
					    "vfork child did not share the main KVM VM");
		} else if (stage == 4) {
			TEST_ASSERT(target == child && n == 1,
				    "Thread created another hidden mapping");
			address = addresses[0];
			TEST_ASSERT(address == main_address,
				    "Thread did not share the hidden mapping");
			TEST_ASSERT(!strcmp(vm_names[0], main_vm),
					    "Thread did not share the main KVM VM");
		} else {
			TEST_ASSERT(target == child && n == 1,
				    "Post-thread helper %d has %zu hidden mappings, expected task %d with one",
				    target, n, child);
			address = addresses[0];
		}
		if (stage == 0)
			test_hardware_breakpoint_ptrace(target, address,
						 address_pipe[1], &features);
		else if (i == 5)
			test_exec_event_ptrace(target, address, address_pipe[1],
					       &features);
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

static int image_teardown_child(void *opaque)
{
	assert_protected_entry();
	syscall(SYS_kill, syscall(SYS_getpid), SIGSTOP);
	return 1;
}

static int run_image_teardown_target(void)
{
	const size_t stack_size = 1 << 20;
	size_t page_size = getpagesize();
	unsigned long addresses[2], old_address;
	void *stack, *mapping;
	int key, status;
	pid_t child;

	assert_protected_entry();
	key = pkey_alloc(0, 0);
	if (key < 0) {
		TEST_ASSERT(errno == EINVAL || errno == ENOSYS || errno == ENOSPC,
			    "pkey_alloc() failed: %d", errno);
		return KSFT_SKIP;
	}
	stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	TEST_ASSERT(stack != MAP_FAILED, "Clone stack mmap() failed: %d", errno);
	mapping = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	TEST_ASSERT(mapping != MAP_FAILED, "Pkey mmap() failed: %d", errno);
	for (int iteration = 0; iteration < 16; iteration++) {
		TEST_ASSERT(get_hidden_mappings(getpid(), addresses, ARRAY_SIZE(addresses)) == 1,
			    "Expected one image before cloning");
		old_address = addresses[0];
		child = clone(image_teardown_child, stack + stack_size,
			      CLONE_VM | SIGCHLD, NULL);
		TEST_ASSERT(child >= 0, "Image teardown clone() failed: %d", errno);
		TEST_ASSERT(waitpid(child, &status, WUNTRACED) == child,
			    "Waiting for stopped image holder failed: %d", errno);
		TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGSTOP,
			    "Image holder did not stop: %#x", status);
		TEST_ASSERT(pkey_mprotect(mapping, page_size, PROT_READ | PROT_WRITE,
					 iteration % 2 ? 0 : key) == 0,
			    "Retiring old image failed: %d", errno);
		TEST_ASSERT(get_hidden_mappings(getpid(), addresses, ARRAY_SIZE(addresses)) == 2,
			    "Stopped child did not retain the old image");
		TEST_ASSERT(kill(child, SIGKILL) == 0, "Killing image holder failed: %d", errno);
		TEST_ASSERT(waitpid(child, &status, 0) == child,
			    "Waiting for killed image holder failed: %d", errno);
		TEST_ASSERT(WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL,
			    "Image holder exited unexpectedly: %#x", status);
		TEST_ASSERT(get_hidden_mappings(getpid(), addresses, ARRAY_SIZE(addresses)) == 1 &&
			    addresses[0] != old_address,
			    "Killed image holder left its retired mapping behind");
	}
	TEST_ASSERT(munmap(mapping, page_size) == 0, "Pkey munmap() failed: %d", errno);
	TEST_ASSERT(munmap(stack, stack_size) == 0, "Stack munmap() failed: %d", errno);
	TEST_ASSERT(pkey_free(key) == 0, "pkey_free() failed: %d", errno);
	return 0;
}

static void test_protected_image_teardown(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "Image teardown fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		fd = create_context_with_features(kvm_fd, KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl("/proc/self/exe", "protected_task_test", "--image-teardown-target", NULL);
		_exit(127);
	}
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "Waiting for image teardown target failed: %d", errno);
	if (WIFEXITED(status) && WEXITSTATUS(status) == KSFT_SKIP) {
		pr_info("Pkeys are unavailable, skipping image retirement test\n");
		return;
	}
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Image teardown target failed: %#x", status);
}

static unsigned long memory_fault_address, memory_fault_error_code;
static int memory_fault_signal_code;

static void memory_fault_handler(int signal, siginfo_t *info, void *context)
{
	ucontext_t *ucontext = context;
	greg_t *gregs = ucontext->uc_mcontext.gregs;

	TEST_ASSERT(signal == SIGSEGV && info->si_code == memory_fault_signal_code,
		    "Unexpected memory-fault signal: %d/%d", signal, info->si_code);
	TEST_ASSERT((unsigned long)info->si_addr == memory_fault_address,
		    "Fault address %p, expected %#lx", info->si_addr, memory_fault_address);
	TEST_ASSERT(gregs[REG_CR2] == memory_fault_address,
		    "Fault CR2 %#llx, expected %#lx", gregs[REG_CR2], memory_fault_address);
	TEST_ASSERT(gregs[REG_ERR] == memory_fault_error_code,
		    "Fault error code %#llx, expected %#lx",
		    gregs[REG_ERR], memory_fault_error_code);
	TEST_ASSERT(gregs[REG_TRAPNO] == 14,
		    "Fault trap number %lld, expected 14", gregs[REG_TRAPNO]);
	_exit(0);
}

static int run_memory_fault_target(bool protected, const char *operation)
{
	struct sigaction action = {
		.sa_sigaction = memory_fault_handler,
		.sa_flags = SA_SIGINFO,
	};
	size_t page_size = getpagesize();
	size_t length = !strcmp(operation, "execute-cross-page") ?
		2 * page_size : page_size;
	unsigned char *mapping;

	if (protected)
		assert_protected_entry();
	sigemptyset(&action.sa_mask);
	TEST_ASSERT(sigaction(SIGSEGV, &action, NULL) == 0,
		    "sigaction(SIGSEGV) failed: %d", errno);
	mapping = mmap(NULL, length, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	TEST_ASSERT(mapping != MAP_FAILED, "Fault mmap() failed: %d", errno);
	mapping[127] = 0xc3;
	memory_fault_address = (unsigned long)mapping + 127;
	memory_fault_signal_code = SEGV_ACCERR;
	if (!strcmp(operation, "read") || !strcmp(operation, "write-unmapped")) {
		memory_fault_signal_code = SEGV_MAPERR;
		memory_fault_error_code = !strcmp(operation, "read") ? 4 : 6;
		TEST_ASSERT(munmap(mapping, page_size) == 0,
			    "Fault munmap() failed: %d", errno);
		if (!strcmp(operation, "read"))
			(void)*(volatile unsigned char *)memory_fault_address;
		else
			*(volatile unsigned char *)memory_fault_address = 0x55;
		return 1;
	}
	if (!strcmp(operation, "write")) {
		memory_fault_error_code = 7;
		TEST_ASSERT(mprotect(mapping, page_size, PROT_READ) == 0,
			    "Fault mprotect() failed: %d", errno);
		*(volatile unsigned char *)memory_fault_address = 0x55;
	} else if (!strcmp(operation, "execute")) {
		memory_fault_error_code = 0x15;
		((void (*)(void))memory_fault_address)();
	} else if (!strcmp(operation, "execute-cross-page")) {
		mapping[page_size - 1] = 0xb8;
		mapping[page_size] = 0;
		memory_fault_address = (unsigned long)mapping + page_size;
		memory_fault_error_code = 0x15;
		TEST_ASSERT(mprotect(mapping, page_size, PROT_READ | PROT_EXEC) == 0,
			    "Cross-page mprotect() failed: %d", errno);
		((void (*)(void))(mapping + page_size - 1))();
	}
	return 1;
}

static void test_protected_memory_fault_metadata(int kvm_fd)
{
	static const char * const operations[] = {
		"read", "write", "write-unmapped", "execute", "execute-cross-page",
	};
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	for (int mode = 0; mode < 2; mode++) {
		for (size_t index = 0; index < ARRAY_SIZE(operations); index++) {
			child = fork();
			TEST_ASSERT(child >= 0, "Memory-fault fork() failed: %d", errno);
			if (!child) {
				if (mode) {
					int fd, ret;

					fd = create_context_with_features(kvm_fd,
							KVM_PROTECTED_TASK_FEATURE_EXEC);
					ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
					TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
				}
				execl("/proc/self/exe", "protected_task_test",
				      "--memory-fault-target", mode ? "protected" : "native",
				      operations[index], NULL);
				_exit(127);
			}
			TEST_ASSERT(waitpid(child, &status, 0) == child,
				    "waitpid() for memory-fault target failed: %d", errno);
			TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
				    "%s %s fault target failed: %#x",
				    mode ? "Protected" : "Native", operations[index], status);
		}
	}
}

static int run_mremap_failure_target(bool protected)
{
	const size_t length = 256UL * 1024 * 1024;
	const size_t headroom = 64UL * 1024 * 1024;
	struct rlimit original_limit, limit;
	size_t page_size = getpagesize();
	unsigned long mapped_pages;
	unsigned char resident;
	void *source, *destination, *result;
	FILE *statm;
	int key, error;

	if (protected)
		assert_protected_entry();
	key = pkey_alloc(0, 0);
	if (key < 0) {
		TEST_ASSERT(errno == EINVAL || errno == ENOSYS || errno == ENOSPC,
			    "pkey_alloc() failed: %d", errno);
		return KSFT_SKIP;
	}
	source = mmap(NULL, length, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	TEST_ASSERT(source != MAP_FAILED, "Source mmap() failed: %d", errno);
	destination = mmap(NULL, length, PROT_READ,
			   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	TEST_ASSERT(destination != MAP_FAILED,
		    "Destination mmap() failed: %d", errno);
	TEST_ASSERT(pkey_mprotect(destination, page_size, PROT_READ, key) == 0,
		    "pkey_mprotect() failed: %d", errno);
	TEST_ASSERT(*(volatile unsigned char *)destination == 0,
		    "Unexpected destination contents");
	statm = fopen("/proc/self/statm", "re");
	TEST_ASSERT(statm, "Opening statm failed: %d", errno);
	TEST_ASSERT(fscanf(statm, "%lu", &mapped_pages) == 1,
		    "Reading statm failed");
	TEST_ASSERT(fclose(statm) == 0, "Closing statm failed: %d", errno);
	TEST_ASSERT(getrlimit(RLIMIT_AS, &original_limit) == 0,
		    "getrlimit(RLIMIT_AS) failed: %d", errno);
	limit = original_limit;
	limit.rlim_cur = mapped_pages * page_size - length + headroom;
	TEST_ASSERT(setrlimit(RLIMIT_AS, &limit) == 0,
		    "Lowering RLIMIT_AS failed: %d", errno);
	result = mremap(source, length, length,
			MREMAP_MAYMOVE | MREMAP_FIXED | MREMAP_DONTUNMAP,
			destination);
	error = errno;
	TEST_ASSERT(setrlimit(RLIMIT_AS, &original_limit) == 0,
		    "Restoring RLIMIT_AS failed: %d", errno);
	TEST_ASSERT(result == MAP_FAILED && error == ENOMEM,
		    "mremap() returned %p/%d, expected ENOMEM", result, error);
	errno = 0;
	TEST_ASSERT(mincore(destination, page_size, &resident) == -1 &&
		    errno == ENOMEM, "Failed mremap() retained its destination");
	result = mmap(destination, page_size, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	TEST_ASSERT(result == destination,
		    "Replacing the unmapped destination failed: %d", errno);
	TEST_ASSERT(pkey_set(key, PKEY_DISABLE_ACCESS) == 0,
		    "Disabling old pkey failed: %d", errno);
	*(volatile unsigned char *)destination = 0x55;
	TEST_ASSERT(*(volatile unsigned char *)destination == 0x55,
		    "Replacement mapping retained the old pkey");
	TEST_ASSERT(pkey_set(key, 0) == 0, "Restoring pkey failed: %d", errno);
	TEST_ASSERT(munmap(source, length) == 0, "Source munmap() failed: %d", errno);
	TEST_ASSERT(munmap(destination, page_size) == 0,
		    "Destination munmap() failed: %d", errno);
	TEST_ASSERT(pkey_free(key) == 0, "pkey_free() failed: %d", errno);
	return 0;
}

static void test_protected_mremap_failure(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	for (int mode = 0; mode < 2; mode++) {
		child = fork();
		TEST_ASSERT(child >= 0, "mremap-failure fork() failed: %d", errno);
		if (!child) {
			if (mode) {
				int fd, ret;

				fd = create_context_with_features(kvm_fd,
						KVM_PROTECTED_TASK_FEATURE_EXEC);
				ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
				TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
			}
			execl("/proc/self/exe", "protected_task_test",
			      "--mremap-failure-target", mode ? "protected" : "native", NULL);
			_exit(127);
		}
		TEST_ASSERT(waitpid(child, &status, 0) == child,
			    "waitpid() for mremap-failure target failed: %d", errno);
		if (WIFEXITED(status) && WEXITSTATUS(status) == KSFT_SKIP) {
			pr_info("%s pkeys are unavailable, skipping failed-remap test\n",
				mode ? "Protected" : "Native");
			continue;
		}
		TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
			    "%s mremap-failure target failed: %#x",
			    mode ? "Protected" : "Native", status);
	}
}

static int run_syscall_stub_target(const char *operation)
{
	struct sigaction action = {
		.sa_handler = SIG_DFL,
	};
	unsigned long addresses[2], start, end, stub = 0;
	char line[256];
	FILE *maps;

	sigemptyset(&action.sa_mask);
	if (sigaction(SIGSEGV, &action, NULL))
		return 8;
	assert_protected_entry();
	if (!strcmp(operation, "hypercall")) {
		unsigned int eax, ebx, ecx, edx;
		char vendor[13] = {};

		__cpuid(0, eax, ebx, ecx, edx);
		memcpy(vendor, &ebx, sizeof(ebx));
		memcpy(vendor + 4, &edx, sizeof(edx));
		memcpy(vendor + 8, &ecx, sizeof(ecx));
		if (!strcmp(vendor, "AuthenticAMD"))
			asm volatile("vmmcall" ::: "memory");
		else if (!strcmp(vendor, "GenuineIntel"))
			asm volatile("vmcall" ::: "memory");
		return 1;
	}

	if (get_hidden_mappings(getpid(), addresses, ARRAY_SIZE(addresses)) != 1)
		return 2;
	maps = fopen("/proc/self/maps", "re");
	if (!maps)
		return 3;
	while (fgets(line, sizeof(line), maps))
		if (sscanf(line, "%lx-%lx", &start, &end) == 2 &&
		    start == addresses[0]) {
			stub = end - 4096;
			break;
		}
	if (fclose(maps) || !stub)
		return 4;

	if (!strcmp(operation, "read"))
		return *(volatile unsigned char *)stub ? 5 : 6;
	if (!strcmp(operation, "execute"))
		asm volatile("jmp *%0" : : "r"(stub) : "memory");
	return 7;
}

static void test_protected_syscall_stub(int kvm_fd)
{
	static const struct {
		const char *operation;
		int signal;
	} tests[] = {
		{ "read", SIGSEGV },
		{ "execute", SIGSEGV },
		{ "hypercall", SIGKILL },
	};
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct rlimit core_limit = {};
	int status;
	pid_t child;

	for (size_t index = 0; index < ARRAY_SIZE(tests); index++) {
		child = fork();
		TEST_ASSERT(child >= 0, "syscall-stub fork() failed: %d", errno);
		if (!child) {
			int fd, ret;

			TEST_ASSERT(setrlimit(RLIMIT_CORE, &core_limit) == 0,
				    "setrlimit(RLIMIT_CORE) failed: %d", errno);
			fd = create_context_with_features(kvm_fd,
					KVM_PROTECTED_TASK_FEATURE_EXEC);
			ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
			TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
			execl("/proc/self/exe", "protected_task_test",
			      "--syscall-stub-target", tests[index].operation, NULL);
			_exit(127);
		}

		TEST_ASSERT(waitpid(child, &status, 0) == child,
			    "waitpid() for syscall-stub target failed: %d", errno);
		TEST_ASSERT(WIFSIGNALED(status) &&
			    WTERMSIG(status) == tests[index].signal,
			    "Protected syscall-stub %s produced status %#x, expected signal %d",
			    tests[index].operation, status, tests[index].signal);
	}
}

static void test_protected_exec_memlock_limit(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "memlock-limit fork() failed: %d", errno);
	if (!child) {
		struct __user_cap_header_struct header = {
			.version = _LINUX_CAPABILITY_VERSION_3,
		};
		struct __user_cap_data_struct data[_LINUX_CAPABILITY_U32S_3] = {};
		struct rlimit limit = {};
		int fd, ret;

		fd = create_context_with_features(kvm_fd,
				KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		TEST_ASSERT(setrlimit(RLIMIT_MEMLOCK, &limit) == 0,
			    "setrlimit(RLIMIT_MEMLOCK) failed: %d", errno);
		TEST_ASSERT(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0,
			    "PR_SET_NO_NEW_PRIVS failed: %d", errno);
		TEST_ASSERT(syscall(SYS_capset, &header, data) == 0,
			    "capset() failed: %d", errno);
		execl("/proc/self/exe", "protected_task_test",
		      "--io-permission-exec-target", NULL);
		_exit(127);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() for memlock-limit child failed: %d", errno);
	TEST_ASSERT(WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV,
		    "Protected memlock-limit exec produced unexpected status: %#x",
		    status);
}

int main(int argc, char *argv[])
{
	struct kvm_protected_task_info first_info, second_info;
	bool process_failure_only, process_lifecycle_only;
	bool process_observability_only;
	bool process_security_only;
	int kvm_fd, first_fd, second_fd;

	if (argc == 2 && !strcmp(argv[1], "--entry-target"))
		return run_entry_target();
	if (argc == 3 && !strcmp(argv[1], "--syscall-stub-target"))
		return run_syscall_stub_target(argv[2]);
	if (argc == 2 && !strcmp(argv[1], "--image-teardown-target"))
		return run_image_teardown_target();
	if (argc == 4 && !strcmp(argv[1], "--memory-fault-target")) {
		if (strcmp(argv[2], "native") && strcmp(argv[2], "protected"))
			return 127;
		return run_memory_fault_target(!strcmp(argv[2], "protected"), argv[3]);
	}
	if (argc == 3 && !strcmp(argv[1], "--mremap-failure-target")) {
		if (strcmp(argv[2], "native") && strcmp(argv[2], "protected"))
			return 127;
		return run_mremap_failure_target(!strcmp(argv[2], "protected"));
	}
	if (argc == 2 && !strcmp(argv[1], "--process-state-target"))
		return run_process_rseq_target(false);
	if (argc == 2 && !strcmp(argv[1], "--process-rseq-reexec-target"))
		return run_process_rseq_target(true);
	if (argc == 2 && !strcmp(argv[1], "--process-failure-signal-target"))
		return run_process_failure_signal_target();
	if (argc == 2 && !strcmp(argv[1], "--process-failure-engine-target"))
		return run_process_failure_engine_target();
	if ((argc == 3 || argc == 5) &&
	    !strcmp(argv[1], "--process-lifecycle-target")) {
		unsigned long generation = 0;
		long original_pid = getpid();
		char *end;

		if (strcmp(argv[2], "native") && strcmp(argv[2], "protected"))
			return 127;
		if (argc == 5) {
			errno = 0;
			generation = strtoul(argv[3], &end, 10);
			if (errno || *end || generation > 4)
				return 127;
			errno = 0;
			original_pid = strtol(argv[4], &end, 10);
			if (errno || *end || original_pid <= 0 ||
			    original_pid > INT_MAX)
				return 127;
		}
		return run_process_lifecycle_target(!strcmp(argv[2], "protected"),
						    generation, original_pid);
	}
	if (argc == 4 && !strcmp(argv[1], "--process-lifecycle-leaf")) {
		long parent_pid;
		char *end;

		if (strcmp(argv[2], "native") && strcmp(argv[2], "protected"))
			return 127;
		errno = 0;
		parent_pid = strtol(argv[3], &end, 10);
		if (errno || *end || parent_pid <= 0 || parent_pid > INT_MAX)
			return 127;
		return run_process_lifecycle_leaf(!strcmp(argv[2], "protected"),
						  parent_pid);
	}
	if (argc == 4 && !strcmp(argv[1], "--process-security-target")) {
		long securebits;
		char *end;

		if (strcmp(argv[2], "native") && strcmp(argv[2], "protected"))
			return 127;
		errno = 0;
		securebits = strtol(argv[3], &end, 10);
		if (errno || *end || securebits < 0 || securebits > INT_MAX)
			return 127;
		return run_process_security_target(!strcmp(argv[2], "protected"),
						   securebits);
	}
	if (argc == 3 &&
	    !strcmp(argv[1], "--process-security-strict-target")) {
		if (strcmp(argv[2], "native") && strcmp(argv[2], "protected"))
			return 127;
		return run_process_security_strict_target(
			!strcmp(argv[2], "protected"));
	}
	if (argc == 4 &&
	    !strcmp(argv[1], "--process-security-landlock-target")) {
		if (strcmp(argv[2], "native") && strcmp(argv[2], "protected"))
			return 127;
		return run_process_security_landlock_target(
			!strcmp(argv[2], "protected"), argv[3]);
	}
	if (argc == 3 &&
	    !strcmp(argv[1], "--process-security-credential-target")) {
		if (strcmp(argv[2], "native") && strcmp(argv[2], "protected"))
			return 127;
		return run_process_security_credential_target(
			!strcmp(argv[2], "protected"));
	}
	if (argc == 3 &&
	    !strcmp(argv[1], "--process-observability-target")) {
		if (strcmp(argv[2], "native") && strcmp(argv[2], "protected"))
			return 127;
		return run_process_observability_target(
			!strcmp(argv[2], "protected"));
	}
	if (argc == 4 && !strcmp(argv[1], "--oom-target")) {
		if (strcmp(argv[2], "native") && strcmp(argv[2], "protected"))
			return 127;
		return run_oom_target(!strcmp(argv[2], "protected"),
				      atoi(argv[3]));
	}
	if (argc == 2 && !strcmp(argv[1], "--io-permission-exec-target"))
		return 42;
	if (argc == 2 && !strcmp(argv[1], "--dso-stress-target"))
		return run_dso_stress_target();
	if (argc == 2 && !strcmp(argv[1], "--vcpu-reuse-target"))
		return run_vcpu_reuse_target();
	if (argc == 2 && !strcmp(argv[1], "--thp-stress-target"))
		return run_thp_stress_target();
	if (argc == 2 && !strcmp(argv[1], "--swap-reclaim-target"))
		return run_swap_reclaim_target();
	if (argc == 2 && !strcmp(argv[1], "--numa-policy-target"))
		return run_numa_policy_target();
	if (argc == 2 && !strcmp(argv[1], "--numa-migration-target"))
		return run_numa_migration_target();
	if (argc == 3 && !strcmp(argv[1], "--memory-hotplug-target"))
		return run_memory_hotplug_target(atoi(argv[2]));
	if (argc == 2 && !strcmp(argv[1], "--uffd-invalidation-target"))
		return run_uffd_invalidation_target();
	process_lifecycle_only = argc == 2 &&
		!strcmp(argv[1], "--process-lifecycle-test");
	process_security_only = argc == 2 &&
		!strcmp(argv[1], "--process-security-test");
	process_observability_only = argc == 2 &&
		!strcmp(argv[1], "--process-observability-test");
	process_failure_only = argc == 2 &&
		!strcmp(argv[1], "--process-failure-test");

	kvm_fd = open_kvm_dev_path_or_exit();
	TEST_REQUIRE(ioctl(kvm_fd, KVM_CHECK_EXTENSION,
			   KVM_CAP_PROTECTED_TASK) == 1);
	if (argc == 2 && !strcmp(argv[1], "--image-teardown-test")) {
		test_protected_image_teardown(kvm_fd);
		close(kvm_fd);
		return 0;
	}
	if (argc == 2 && !strcmp(argv[1], "--memory-fault-test")) {
		test_protected_memory_fault_metadata(kvm_fd);
		close(kvm_fd);
		return 0;
	}
	if (argc == 2 && !strcmp(argv[1], "--mremap-failure-test")) {
		test_protected_mremap_failure(kvm_fd);
		close(kvm_fd);
		return 0;
	}
	if (argc == 2 && !strcmp(argv[1], "--syscall-stub-test")) {
		test_protected_syscall_stub(kvm_fd);
		close(kvm_fd);
		return 0;
	}
	if (argc == 2 && !strcmp(argv[1], "--memlock-limit-test")) {
		test_protected_exec_memlock_limit(kvm_fd);
		close(kvm_fd);
		return 0;
	}
	if (argc == 2 && !strcmp(argv[1], "--ptrace-test")) {
		test_protected_exec(kvm_fd);
		close(kvm_fd);
		return 0;
	}
	if (process_lifecycle_only) {
		test_protected_process_lifecycle(kvm_fd);
		close(kvm_fd);
		return 0;
	}
	if (process_security_only) {
		test_protected_process_security(kvm_fd);
		test_cgroup_oom(kvm_fd);
		close(kvm_fd);
		return 0;
	}
	if (process_observability_only) {
		test_protected_process_observability(kvm_fd);
		close(kvm_fd);
		return 0;
	}
	if (process_failure_only) {
		test_protected_process_failure(kvm_fd);
		close(kvm_fd);
		return 0;
	}

	test_create_validation(kvm_fd);
	test_protected_syscall_stub(kvm_fd);
	test_protected_image_teardown(kvm_fd);
	test_protected_memory_fault_metadata(kvm_fd);
	test_protected_mremap_failure(kvm_fd);
	test_protected_exec_memlock_limit(kvm_fd);
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
	test_protected_entry_returns(kvm_fd);
	test_protected_process_state(kvm_fd);
	test_protected_process_lifecycle(kvm_fd);
	test_protected_process_security(kvm_fd);
	test_protected_process_observability(kvm_fd);
	test_protected_process_failure(kvm_fd);
	test_swap_reclaim(kvm_fd);
	test_numa_stress(kvm_fd);
	test_memory_hotplug(kvm_fd);
	test_cgroup_oom(kvm_fd);
	test_uffd_invalidation(kvm_fd);
	test_thp_stress(kvm_fd);
	test_dso_stress(kvm_fd);
	test_vcpu_reuse(kvm_fd);
	test_protected_exec(kvm_fd);
	close(kvm_fd);
	return 0;
}
