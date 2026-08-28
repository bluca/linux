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
#include <linux/refcount.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

struct kvm_protected_task {
	struct kvm_protected_task_context context;
	u64 context_id;
	u64 features;
};

struct kvm_protected_task_vcpu {
	struct list_head node;
	struct kvm_vcpu *vcpu;
};

#define KVM_PROTECTED_TASK_IDLE_VCPU_LIMIT 16U
#define KVM_PROTECTED_TASK_QUIESCING ((int)BIT(31))

struct kvm_protected_task_vm {
	refcount_t refs;
	struct kvm *kvm;
	struct list_head idle_vcpus;
	unsigned int nr_idle_vcpus;
	u64 pgtable_gen;
	unsigned long next_vcpu_id;
	int debug_id;
	u32 next_slot;
};

struct kvm_protected_task_exec {
	struct kvm_protected_task_vm *vm;
	struct kvm *kvm;
	struct kvm_vcpu *vcpu;
	struct kvm_protected_task_vcpu *registered_vcpu;
	void *arch_state;
	u64 pgtable_gen;
	u32 next_slot;
};

struct kvm_protected_task_range {
	unsigned long start;
	unsigned long end;
};

static atomic64_t kvm_protected_task_id = ATOMIC64_INIT(0);
static DEFINE_IDA(kvm_protected_task_debug_ids);

static void kvm_protected_task_register_vcpu(struct kvm_protected_task_exec *exec)
{
	struct mm_struct *mm = exec->kvm->mm;

	exec->registered_vcpu->vcpu = exec->vcpu;
	spin_lock(&mm->protected_task_lock);
	list_add(&exec->registered_vcpu->node, &mm->protected_task_vcpus);
	spin_unlock(&mm->protected_task_lock);
}

static void kvm_protected_task_unregister_vcpu(struct kvm_protected_task_exec *exec)
{
	struct mm_struct *mm = exec->kvm->mm;

	spin_lock(&mm->protected_task_lock);
	list_del_init(&exec->registered_vcpu->node);
	spin_unlock(&mm->protected_task_lock);
}

static void kvm_protected_task_kick_vcpus(struct mm_struct *mm)
{
	struct kvm_protected_task_vcpu *registered_vcpu;

	spin_lock(&mm->protected_task_lock);
	list_for_each_entry(registered_vcpu, &mm->protected_task_vcpus, node)
		kvm_make_request_and_kick(KVM_REQ_PROTECTED_TASK_EXIT,
					  registered_vcpu->vcpu);
	spin_unlock(&mm->protected_task_lock);
}

static void kvm_protected_task_quiesce_mm(void *state)
{
	struct kvm_protected_task_exec *exec = state;
	struct mm_struct *mm = exec->kvm->mm;
	struct kvm_protected_task_vcpu *registered_vcpu;

	for (;;) {
		wait_event(mm->protected_task_wait,
			   atomic_read_acquire(
				   &mm->protected_task_run_state) >= 0);

		spin_lock(&mm->protected_task_lock);
		if (atomic_read(&mm->protected_task_run_state) >= 0) {
			atomic_or(KVM_PROTECTED_TASK_QUIESCING,
				  &mm->protected_task_run_state);
			list_for_each_entry(registered_vcpu,
					    &mm->protected_task_vcpus, node) {
				if (registered_vcpu->vcpu == exec->vcpu)
					continue;
				kvm_make_request_and_kick(
					KVM_REQ_PROTECTED_TASK_EXIT,
					registered_vcpu->vcpu);
			}
			spin_unlock(&mm->protected_task_lock);
			break;
		}
		spin_unlock(&mm->protected_task_lock);
	}

	wait_event(mm->protected_task_wait,
		   atomic_read(&mm->protected_task_run_state) ==
			   KVM_PROTECTED_TASK_QUIESCING);
}

static void kvm_protected_task_resume_mm(void *state)
{
	struct kvm_protected_task_exec *exec = state;
	struct mm_struct *mm = exec->kvm->mm;

	spin_lock(&mm->protected_task_lock);
	WARN_ON_ONCE(atomic_read(&mm->protected_task_run_state) !=
		     KVM_PROTECTED_TASK_QUIESCING);
	atomic_set_release(&mm->protected_task_run_state, 0);
	spin_unlock(&mm->protected_task_lock);
	wake_up_all(&mm->protected_task_wait);
}

static void kvm_protected_task_vcpu_run_begin(struct kvm_protected_task_exec *exec)
{
	struct mm_struct *mm = exec->kvm->mm;

	for (;;) {
		if (likely(atomic_inc_unless_negative(
				   &mm->protected_task_run_state)))
			return;

		wait_event(mm->protected_task_wait,
			   atomic_read_acquire(
				   &mm->protected_task_run_state) >= 0);
	}
}

static bool kvm_protected_task_vcpu_run_blocked(struct kvm_protected_task_exec *exec)
{
	struct mm_struct *mm = exec->kvm->mm;

	return atomic_read_acquire(&mm->protected_task_run_state) < 0;
}

void kvm_protected_task_vcpu_run_complete(struct kvm_vcpu *vcpu)
{
	struct mm_struct *mm = vcpu->kvm->mm;
	int state;

	state = atomic_read(&mm->protected_task_run_state);
	if (WARN_ON_ONCE(!(state & ~KVM_PROTECTED_TASK_QUIESCING)))
		return;
	state = atomic_dec_return(&mm->protected_task_run_state);
	if (state == KVM_PROTECTED_TASK_QUIESCING)
		wake_up_all(&mm->protected_task_wait);
}

static int kvm_protected_task_map_range(struct kvm_protected_task_vm *vm,
					unsigned long start, unsigned long end)
{
	const u64 max_size = (u64)KVM_MEM_MAX_NR_PAGES << PAGE_SHIFT;

	while (start < end) {
		struct kvm_userspace_memory_region2 region = {
			.slot = vm->next_slot++,
			.guest_phys_addr = start,
			.userspace_addr = start,
			.memory_size = min_t(u64, end - start, max_size),
		};
		int ret;

		if (region.slot >= KVM_USER_MEM_SLOTS)
			return -ENOSPC;

		ret = kvm_set_user_memory_region(vm->kvm, &region);
		if (ret)
			return ret;

		start += region.memory_size;
	}

	return 0;
}

static int kvm_protected_task_map_mm(struct kvm_protected_task_vm *vm,
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
		if (i && ranges[i - 1].end == vma->vm_start) {
			ranges[i - 1].end = vma->vm_end;
			continue;
		}
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
		ret = kvm_protected_task_map_range(vm, ranges[i].start,
						   ranges[i].end);
		if (ret)
			break;
	}

free_ranges:
	kfree(ranges);
	return ret;
}

static void kvm_protected_task_free_vm(struct kvm_protected_task_vm *vm)
{
	struct kvm_protected_task_vcpu *registered_vcpu, *tmp;

	list_for_each_entry_safe(registered_vcpu, tmp, &vm->idle_vcpus, node) {
		list_del(&registered_vcpu->node);
		kfree(registered_vcpu);
	}
	kvm_put_kvm(vm->kvm);
	ida_free(&kvm_protected_task_debug_ids, vm->debug_id);
	kfree(vm);
}

static bool kvm_protected_task_put_vm_locked(struct kvm_protected_task_vm *vm)
{
	struct mm_struct *mm = vm->kvm->mm;

	lockdep_assert_held(&mm->protected_task_vm_lock);
	if (!refcount_dec_and_test(&vm->refs))
		return false;
	if (mm->protected_task_vm == vm)
		WRITE_ONCE(mm->protected_task_vm, NULL);
	return true;
}

static void kvm_protected_task_put_vm(struct kvm_protected_task_vm *vm)
{
	struct mm_struct *mm = vm->kvm->mm;
	bool free_vm;

	mutex_lock(&mm->protected_task_vm_lock);
	free_vm = kvm_protected_task_put_vm_locked(vm);
	mutex_unlock(&mm->protected_task_vm_lock);
	if (free_vm)
		kvm_protected_task_free_vm(vm);
}

static void kvm_protected_task_retire_vm(struct kvm_protected_task_vm *vm)
{
	struct mm_struct *mm = vm->kvm->mm;

	mutex_lock(&mm->protected_task_vm_lock);
	if (mm->protected_task_vm == vm)
		WRITE_ONCE(mm->protected_task_vm, NULL);
	mutex_unlock(&mm->protected_task_vm_lock);
}

static int kvm_protected_task_get_vm(
		struct mm_struct *mm, struct kvm_protected_task_vm *stale_vm,
		struct kvm_protected_task_vm **vmp,
		struct kvm_protected_task_vcpu **idle_vcpu,
		unsigned long *vcpu_id)
{
	struct kvm_protected_task_vm *vm;
	char fdname[sizeof("pt2147483647")];
	u64 pgtable_gen;
	int ret;

	pgtable_gen = atomic64_read(&mm->protected_task_pgtable_gen);
	mutex_lock(&mm->protected_task_vm_lock);
	vm = mm->protected_task_vm;
	if (vm && vm != stale_vm && vm->pgtable_gen == pgtable_gen &&
	    (!list_empty(&vm->idle_vcpus) ||
	     vm->next_vcpu_id < vm->kvm->max_vcpus)) {
		refcount_inc(&vm->refs);
		goto found;
	}

	vm = kzalloc_obj(*vm);
	if (!vm) {
		ret = -ENOMEM;
		goto unlock;
	}
	vm->debug_id = ida_alloc(&kvm_protected_task_debug_ids, GFP_KERNEL);
	if (vm->debug_id < 0) {
		ret = vm->debug_id;
		goto free_vm;
	}
	snprintf(fdname, sizeof(fdname), "pt%d", vm->debug_id);
	vm->kvm = kvm_create_vm(0, fdname, mm);
	if (IS_ERR(vm->kvm)) {
		ret = PTR_ERR(vm->kvm);
		goto free_debug_id;
	}
	vm->kvm->protected_task = true;
	INIT_LIST_HEAD(&vm->idle_vcpus);

	ret = kvm_protected_task_map_mm(vm, mm);
	if (ret)
		goto put_kvm;

	refcount_set(&vm->refs, 1);
	vm->pgtable_gen = pgtable_gen;
	WRITE_ONCE(mm->protected_task_vm, vm);

found:
	*vmp = vm;
	if (!list_empty(&vm->idle_vcpus)) {
		*idle_vcpu = list_first_entry(&vm->idle_vcpus,
						     struct kvm_protected_task_vcpu,
						     node);
		list_del_init(&(*idle_vcpu)->node);
		vm->nr_idle_vcpus--;
	} else
		*vcpu_id = vm->next_vcpu_id++;
	mutex_unlock(&mm->protected_task_vm_lock);
	return 0;

put_kvm:
	kvm_put_kvm(vm->kvm);
free_debug_id:
	ida_free(&kvm_protected_task_debug_ids, vm->debug_id);
free_vm:
	kfree(vm);
unlock:
	mutex_unlock(&mm->protected_task_vm_lock);
	return ret;
}

static int kvm_protected_task_create_exec(
		struct mm_struct *mm, struct kvm_protected_task_vm *stale_vm,
		void **state)
{
	struct kvm_protected_task_exec *exec;
	struct kvm_protected_task_vcpu *idle_vcpu = NULL;
	unsigned long vcpu_id;
	int ret;

	ret = kvm_arch_protected_task_prepare_permissions();
	if (ret)
		return ret;

	exec = kzalloc_obj(*exec);
	if (!exec)
		return -ENOMEM;
	exec->registered_vcpu = kzalloc_obj(*exec->registered_vcpu);
	if (!exec->registered_vcpu) {
		ret = -ENOMEM;
		goto free_exec;
	}

	ret = kvm_protected_task_get_vm(mm, stale_vm, &exec->vm, &idle_vcpu,
					&vcpu_id);
	if (ret)
		goto free_exec;
	exec->kvm = exec->vm->kvm;
	exec->next_slot = exec->vm->next_slot;

	if (idle_vcpu) {
		kfree(exec->registered_vcpu);
		exec->registered_vcpu = idle_vcpu;
		exec->vcpu = idle_vcpu->vcpu;
	} else {
		exec->vcpu = kvm_create_vcpu(exec->kvm, vcpu_id, NULL);
		if (IS_ERR(exec->vcpu)) {
			ret = PTR_ERR(exec->vcpu);
			goto put_vm;
		}
		exec->registered_vcpu->vcpu = exec->vcpu;
	}

	ret = kvm_arch_protected_task_prepare(exec->vcpu, !!idle_vcpu,
					      &exec->arch_state);
	if (ret)
		goto put_vm;
	kvm_protected_task_register_vcpu(exec);

	*state = exec;
	return 0;

put_vm:
	kvm_protected_task_retire_vm(exec->vm);
	kvm_protected_task_put_vm(exec->vm);
free_exec:
	kfree(exec->registered_vcpu);
	kfree(exec);
	return ret;
}

static int kvm_protected_task_stage_exec(struct kvm_protected_task_context *context,
					 struct linux_binprm *bprm, void **state)
{
	struct kvm_protected_task *protected_task =
		container_of(context, struct kvm_protected_task, context);
	int ret;

	ret = kvm_protected_task_create_exec(bprm->mm, NULL, state);
	if (ret)
		return ret;
	return protected_task->features & KVM_PROTECTED_TASK_FEATURE_EXEC ?
		0 : -EOPNOTSUPP;
}

static void kvm_protected_task_release_exec_resources(struct kvm_protected_task_exec *exec)
{
	struct mm_struct *mm = exec->kvm->mm;
	bool free_vm, retire_vm = false;

	kvm_protected_task_unregister_vcpu(exec);
	kvm_arch_protected_task_cleanup(exec->vcpu, exec->arch_state);
	mutex_lock(&mm->protected_task_vm_lock);
	list_add(&exec->registered_vcpu->node, &exec->vm->idle_vcpus);
	exec->vm->nr_idle_vcpus++;
	free_vm = kvm_protected_task_put_vm_locked(exec->vm);
	if (!free_vm && mm->protected_task_vm == exec->vm &&
	    exec->vm->nr_idle_vcpus > KVM_PROTECTED_TASK_IDLE_VCPU_LIMIT &&
	    refcount_read(&exec->vm->refs) == 1) {
		WRITE_ONCE(mm->protected_task_vm, NULL);
		retire_vm = true;
	}
	mutex_unlock(&mm->protected_task_vm_lock);
	if (retire_vm)
		kvm_protected_task_kick_vcpus(mm);
	if (free_vm)
		kvm_protected_task_free_vm(exec->vm);
}

static void kvm_protected_task_release_exec(void *state)
{
	struct kvm_protected_task_exec *exec = state;

	kvm_protected_task_release_exec_resources(exec);
	kfree(exec);
}

static int kvm_protected_task_finalize_vcpu_from(
		struct kvm_protected_task_exec *exec,
		struct kvm_protected_task_exec *source,
		struct pt_regs *regs)
{
	int ret;

	ret = kvm_arch_protected_task_finalize(exec->vcpu, exec->arch_state,
					       source ? source->vcpu : NULL,
					       source ? source->arch_state : NULL,
					       regs, exec->next_slot++,
					       &exec->pgtable_gen);
	return ret;
}

static int kvm_protected_task_finalize_vcpu(void *state, struct pt_regs *regs)
{
	return kvm_protected_task_finalize_vcpu_from(state, NULL, regs);
}

static void kvm_protected_task_prepare_exec_user_work(void *state)
{
	struct kvm_protected_task_exec *exec = state;

	WARN_ON_ONCE(kvm_arch_protected_task_prepare_user_work(
			exec->vcpu, exec->arch_state));
}

static int kvm_protected_task_clone_exec(struct kvm_protected_task_context *context,
					 struct pt_regs *regs, void **state)
{
	struct kvm_protected_task_exec *exec;
	int ret;

	ret = kvm_protected_task_create_exec(current->mm, NULL, (void **)&exec);
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

static u64 kvm_protected_task_adjust_xfeatures(void *state)
{
	struct kvm_protected_task_exec *exec = state;

	return kvm_arch_protected_task_xfeatures(exec->vcpu, exec->arch_state);
}

static unsigned int kvm_protected_task_adjust_max_tag_bits(void *state)
{
	struct kvm_protected_task_exec *exec = state;

	return kvm_arch_protected_task_max_tag_bits(exec->vcpu,
						    exec->arch_state);
}

static bool kvm_protected_task_adjust_pgtable_update(void *state)
{
	struct kvm_protected_task_exec *exec = state;

	return kvm_arch_protected_task_needs_pgtable_update(exec->vcpu,
							    exec->arch_state);
}

static int kvm_protected_task_refresh(struct kvm_protected_task_exec *exec,
				      struct pt_regs *regs)
{
	struct kvm_protected_task_exec *new_exec;
	struct kvm_protected_task_exec old_exec;
	int ret;

	ret = kvm_protected_task_create_exec(current->mm, exec->vm,
					     (void **)&new_exec);
	if (ret)
		return ret;
	ret = kvm_protected_task_finalize_vcpu_from(new_exec, exec, regs);
	if (ret) {
		kvm_protected_task_release_exec(new_exec);
		return ret;
	}

	old_exec = *exec;
	*exec = *new_exec;
	kfree(new_exec);
	kvm_protected_task_release_exec_resources(&old_exec);
	return 0;
}

static int kvm_protected_task_run_vcpu(void *state, struct pt_regs *regs)
{
	struct kvm_protected_task_exec *exec = state;
	u64 pgtable_gen;
	int ret;

	kvm_protected_task_vcpu_run_begin(exec);
	pgtable_gen = atomic64_read(&current->mm->protected_task_pgtable_gen);

	if (unlikely(exec->pgtable_gen != pgtable_gen ||
		     READ_ONCE(current->mm->protected_task_vm) != exec->vm)) {
		ret = kvm_protected_task_refresh(exec, regs);
		if (ret)
			goto complete;
	}
	if (unlikely(kvm_protected_task_vcpu_run_blocked(exec))) {
		ret = 0;
		goto complete;
	}

	ret = kvm_arch_protected_task_run(exec->vcpu, exec->arch_state,
					  regs, &exec->next_slot);
	if (ret == -ENOSPC) {
		kvm_protected_task_vcpu_run_begin(exec);
		ret = kvm_protected_task_refresh(exec, regs);
		kvm_protected_task_vcpu_run_complete(exec->vcpu);
	}
	return ret;

complete:
	kvm_protected_task_vcpu_run_complete(exec->vcpu);
	return ret;
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
	.xfeatures = kvm_protected_task_adjust_xfeatures,
	.max_tag_bits = kvm_protected_task_adjust_max_tag_bits,
	.needs_pgtable_update = kvm_protected_task_adjust_pgtable_update,
	.begin_mm_update = kvm_protected_task_quiesce_mm,
	.end_mm_update = kvm_protected_task_resume_mm,
	.deactivate_exec = kvm_protected_task_deactivate_vcpu,
	.finalize_exec = kvm_protected_task_finalize_vcpu,
	.prepare_user_work = kvm_protected_task_prepare_exec_user_work,
	.run = kvm_protected_task_run_vcpu,
	.cleanup_exec = kvm_protected_task_release_exec,
};

static int kvm_protected_task_copy_arg(void *dst, size_t size,
				       size_t min_size, void __user *argp,
				       u32 *user_size)
{
	if (get_user(*user_size, (u32 __user *)argp))
		return -EFAULT;
	if (*user_size < min_size)
		return -EINVAL;

	return copy_struct_from_user(dst, size, argp, *user_size);
}

static int kvm_protected_task_arm(struct file *file, void __user *argp)
{
	struct kvm_protected_task_arm arm = {};
	struct file *old;
	size_t min_size = offsetofend(struct kvm_protected_task_arm, flags);
	u32 user_size;
	int ret;

	ret = kvm_protected_task_copy_arg(&arm, sizeof(arm), min_size, argp,
					  &user_size);
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

	ret = kvm_protected_task_copy_arg(&info, sizeof(info), min_size, argp,
					  &user_size);
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

	ret = kvm_protected_task_copy_arg(&create, sizeof(create), min_size, argp,
					  &user_size);
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
