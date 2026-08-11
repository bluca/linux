// SPDX-License-Identifier: GPL-2.0-only

#include <linux/anon_inodes.h>
#include <linux/binfmts.h>
#include <linux/compat.h>
#include <linux/fdtable.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/kvm.h>
#include <linux/kvm_host.h>
#include <linux/kvm_protected_task.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

struct kvm_protected_task {
	struct kvm_protected_task_context context;
	u64 context_id;
	u64 features;
};

struct kvm_protected_task_exec {
	struct kvm *kvm;
	struct kvm_vcpu *vcpu;
	void *arch_state;
	int debug_id;
	u32 next_slot;
};

struct kvm_protected_task_range {
	unsigned long start;
	unsigned long end;
};

static atomic64_t kvm_protected_task_id = ATOMIC64_INIT(0);
static DEFINE_IDA(kvm_protected_task_debug_ids);

static int kvm_protected_task_map_range(struct kvm_protected_task_exec *exec,
					unsigned long start, unsigned long end)
{
	const u64 max_size = (u64)KVM_MEM_MAX_NR_PAGES << PAGE_SHIFT;

	while (start < end) {
		struct kvm_userspace_memory_region2 region = {
			.slot = exec->next_slot++,
			.guest_phys_addr = start,
			.userspace_addr = start,
			.memory_size = min_t(u64, end - start, max_size),
		};
		int ret;

		if (region.slot >= KVM_USER_MEM_SLOTS)
			return -ENOSPC;

		ret = kvm_set_user_memory_region(exec->kvm, &region);
		if (ret)
			return ret;

		start += region.memory_size;
	}

	return 0;
}

static int kvm_protected_task_map_mm(struct kvm_protected_task_exec *exec,
				     struct mm_struct *mm)
{
	struct kvm_protected_task_range *ranges;
	struct vm_area_struct *vma;
	VMA_ITERATOR(vmi, mm, 0);
	int i = 0, nr, ret = 0;

	mmap_read_lock(mm);
	nr = mm->map_count;
	ranges = kcalloc(nr, sizeof(*ranges), GFP_KERNEL_ACCOUNT);
	if (!ranges) {
		ret = -ENOMEM;
		goto unlock;
	}

	for_each_vma(vmi, vma) {
		if (vma->vm_flags & VM_KVM_PROTECTED)
			continue;
		if (WARN_ON_ONCE(i >= nr)) {
			ret = -EAGAIN;
			goto unlock;
		}
		ranges[i].start = vma->vm_start;
		ranges[i].end = vma->vm_end;
		i++;
	}
	nr = i;

unlock:
	mmap_read_unlock(mm);
	if (ret)
		goto free_ranges;

	for (i = 0; i < nr; i++) {
		ret = kvm_protected_task_map_range(exec, ranges[i].start,
						   ranges[i].end);
		if (ret)
			break;
	}

free_ranges:
	kfree(ranges);
	return ret;
}

static int kvm_protected_task_create_exec(struct mm_struct *mm, void **state)
{
	struct kvm_protected_task_exec *exec;
	char fdname[sizeof("pt2147483647")];
	int ret;

	exec = kzalloc_obj(*exec);
	if (!exec)
		return -ENOMEM;

	exec->debug_id = ida_alloc(&kvm_protected_task_debug_ids, GFP_KERNEL);
	if (exec->debug_id < 0) {
		ret = exec->debug_id;
		goto free_exec;
	}
	snprintf(fdname, sizeof(fdname), "pt%d", exec->debug_id);
	exec->kvm = kvm_create_vm(0, fdname, mm);
	if (IS_ERR(exec->kvm)) {
		ret = PTR_ERR(exec->kvm);
		goto free_debug_id;
	}
	exec->kvm->protected_task = true;

	ret = kvm_protected_task_map_mm(exec, mm);
	if (ret)
		goto put_kvm;

	exec->vcpu = kvm_create_vcpu(exec->kvm, 0, NULL);
	if (IS_ERR(exec->vcpu)) {
		ret = PTR_ERR(exec->vcpu);
		goto put_kvm;
	}

	ret = kvm_arch_protected_task_prepare(exec->vcpu, &exec->arch_state);
	if (ret)
		goto put_kvm;

	*state = exec;
	return 0;

put_kvm:
	kvm_put_kvm(exec->kvm);
free_debug_id:
	ida_free(&kvm_protected_task_debug_ids, exec->debug_id);
free_exec:
	kfree(exec);
	return ret;
}

static int kvm_protected_task_stage_exec(struct kvm_protected_task_context *context,
					 struct linux_binprm *bprm, void **state)
{
	struct kvm_protected_task *protected_task =
		container_of(context, struct kvm_protected_task, context);
	int ret;

	ret = kvm_protected_task_create_exec(bprm->mm, state);
	if (ret)
		return ret;
	return protected_task->features & KVM_PROTECTED_TASK_FEATURE_EXEC ?
		0 : -EOPNOTSUPP;
}

static void kvm_protected_task_release_exec(void *state)
{
	struct kvm_protected_task_exec *exec = state;

	kvm_arch_protected_task_cleanup(exec->vcpu, exec->arch_state);
	kvm_put_kvm(exec->kvm);
	ida_free(&kvm_protected_task_debug_ids, exec->debug_id);
	kfree(exec);
}

static int kvm_protected_task_finalize_vcpu(void *state, struct pt_regs *regs)
{
	struct kvm_protected_task_exec *exec = state;

	return kvm_arch_protected_task_finalize(exec->vcpu, exec->arch_state,
						  regs, exec->next_slot++);
}

static int kvm_protected_task_clone_exec(struct kvm_protected_task_context *context,
					 struct pt_regs *regs, void **state)
{
	struct kvm_protected_task_exec *exec;
	int ret;

	ret = kvm_protected_task_create_exec(current->mm, (void **)&exec);
	if (ret)
		return ret;
	ret = kvm_protected_task_finalize_vcpu(exec, regs);
	if (ret) {
		kvm_protected_task_release_exec(exec);
		return ret;
	}

	*state = exec;
	return 0;
}

static unsigned long kvm_protected_task_adjust_elf_hwcap(void *state,
							 unsigned int type,
							 unsigned long value)
{
	struct kvm_protected_task_exec *exec = state;

	return kvm_arch_protected_task_elf_hwcap(exec->vcpu, type, value);
}

static int kvm_protected_task_run_vcpu(void *state, struct pt_regs *regs)
{
	struct kvm_protected_task_exec *exec = state;

	return kvm_arch_protected_task_run(exec->vcpu, exec->arch_state, regs,
						   &exec->next_slot);
}

static void kvm_protected_task_deactivate_vcpu(void *state)
{
	struct kvm_protected_task_exec *exec = state;

	WARN_ON_ONCE(kvm_arch_protected_task_deactivate(exec->vcpu,
							exec->arch_state));
}

static const struct kvm_protected_task_ops kvm_protected_task_ops = {
	.stage_exec = kvm_protected_task_stage_exec,
	.clone_exec = kvm_protected_task_clone_exec,
	.elf_hwcap = kvm_protected_task_adjust_elf_hwcap,
	.deactivate_exec = kvm_protected_task_deactivate_vcpu,
	.finalize_exec = kvm_protected_task_finalize_vcpu,
	.run = kvm_protected_task_run_vcpu,
	.cleanup_exec = kvm_protected_task_release_exec,
};

static int kvm_protected_task_copy_arg(void *dst, size_t size,
				       size_t min_size, void __user *argp)
{
	u32 user_size;

	if (get_user(user_size, (u32 __user *)argp))
		return -EFAULT;
	if (user_size < min_size)
		return -EINVAL;

	return copy_struct_from_user(dst, size, argp, user_size);
}

static int kvm_protected_task_arm(struct file *file, void __user *argp)
{
	struct kvm_protected_task_arm arm = {};
	struct file *old;
	size_t min_size = offsetofend(struct kvm_protected_task_arm, flags);
	int ret;

	ret = kvm_protected_task_copy_arg(&arm, sizeof(arm), min_size, argp);
	if (ret)
		return ret;
	if (arm.flags || memchr_inv(arm.reserved, 0, sizeof(arm.reserved)))
		return -EINVAL;
	if (!kvm_protected_task_can_arm())
		return -EBUSY;

	get_file(file);
	old = cmpxchg(&current->protected_task_pending, NULL, file);
	if (old) {
		fput(file);
		return -EBUSY;
	}

	return 0;
}

static int kvm_protected_task_cancel(struct file *file)
{
	if (cmpxchg(&current->protected_task_pending, file, NULL) != file)
		return -ENOENT;

	fput(file);
	return 0;
}

static int kvm_protected_task_get_info(struct file *file, void __user *argp)
{
	struct kvm_protected_task *protected_task = file->private_data;
	struct kvm_protected_task_info info = {};
	size_t min_size = offsetofend(struct kvm_protected_task_info, context_id);
	u32 user_size;
	int ret;

	if (get_user(user_size, (u32 __user *)argp))
		return -EFAULT;
	ret = kvm_protected_task_copy_arg(&info, sizeof(info), min_size, argp);
	if (ret)
		return ret;
	if (info.flags || info.features || info.context_id ||
	    memchr_inv(info.reserved, 0, sizeof(info.reserved)))
		return -EINVAL;

	if (READ_ONCE(current->protected_task_pending) == file)
		info.flags = KVM_PROTECTED_TASK_INFO_ARMED;
	info.features = protected_task->features;
	info.context_id = protected_task->context_id;

	if (copy_to_user(argp, &info, min_t(size_t, user_size, sizeof(info))))
		return -EFAULT;
	return 0;
}

static long kvm_protected_task_ioctl(struct file *file, unsigned int ioctl,
				     unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	if (unlikely(_IOC_TYPE(ioctl) != KVMIO))
		return -EINVAL;

	switch (ioctl) {
	case KVM_PT_ARM_EXEC:
		return kvm_protected_task_arm(file, argp);
	case KVM_PT_CANCEL_ARM:
		return arg ? -EINVAL : kvm_protected_task_cancel(file);
	case KVM_PT_GET_INFO:
		return kvm_protected_task_get_info(file, argp);
	default:
		return -ENOIOCTLCMD;
	}
}

static int kvm_protected_task_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static const struct file_operations kvm_protected_task_fops = {
	.owner = THIS_MODULE,
	.release = kvm_protected_task_release,
	.unlocked_ioctl = kvm_protected_task_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
	.llseek = noop_llseek,
};

int kvm_protected_task_create_fd(void __user *argp)
{
	struct kvm_protected_task_create create = {};
	struct kvm_protected_task *protected_task;
	struct file *file;
	size_t min_size = offsetofend(struct kvm_protected_task_create,
				      supported_features);
	u32 user_size;
	int fd, ret;

	if (get_user(user_size, (u32 __user *)argp))
		return -EFAULT;
	ret = kvm_protected_task_copy_arg(&create, sizeof(create), min_size, argp);
	if (ret)
		return ret;
	if (create.flags || create.supported_features ||
	    memchr_inv(create.reserved, 0, sizeof(create.reserved)))
		return -EINVAL;

	create.supported_features = kvm_arch_protected_task_features();
	if (copy_to_user(argp, &create,
			 min_t(size_t, user_size, sizeof(create))))
		return -EFAULT;
	if (create.required_features & ~create.supported_features)
		return -EOPNOTSUPP;

	protected_task = kzalloc_obj(*protected_task);
	if (!protected_task)
		return -ENOMEM;
	protected_task->context.ops = &kvm_protected_task_ops;
	protected_task->features = create.required_features;
	protected_task->context_id = atomic64_inc_return(&kvm_protected_task_id);

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		ret = fd;
		goto put_protected_task;
	}

	file = anon_inode_getfile("[kvm-protected-task]",
				  &kvm_protected_task_fops, protected_task, O_RDWR);
	if (IS_ERR(file)) {
		ret = PTR_ERR(file);
		goto put_fd;
	}

	fd_install(fd, file);
	return fd;

put_fd:
	put_unused_fd(fd);
put_protected_task:
	kfree(protected_task);
	return ret;
}
