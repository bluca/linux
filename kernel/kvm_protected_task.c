// SPDX-License-Identifier: GPL-2.0-only

#include <linux/binfmts.h>
#include <linux/export.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kvm_protected_task.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

bool kvm_protected_task_can_arm(void)
{
#if IS_ENABLED(CONFIG_KVM)
	if (current->protected_task_active || current->protected_task_retired)
		return false;
#endif
	return current_is_single_threaded();
}
EXPORT_SYMBOL_GPL(kvm_protected_task_can_arm);

bool kvm_protected_task_is_active(void)
{
#if IS_ENABLED(CONFIG_KVM)
	return current->protected_task_active;
#else
	return false;
#endif
}

u64 kvm_protected_task_xfeatures(void)
{
#if IS_ENABLED(CONFIG_KVM)
	struct kvm_protected_task_context *context;

	if (!current->protected_task_active || !current->protected_task_state)
		return 0;
	context = current->protected_task_active->private_data;
	return context->ops->xfeatures ?
		context->ops->xfeatures(current->protected_task_state) : 0;
#else
	return 0;
#endif
}

void kvm_protected_task_init(struct task_struct *task)
{
#if IS_ENABLED(CONFIG_KVM)
	task->protected_task_pending = NULL;
	task->protected_task_active = NULL;
	task->protected_task_state = NULL;
	task->protected_task_retired = NULL;
	task->protected_task_retired_state = NULL;
	task->protected_task_running = false;
#endif
}

void kvm_protected_task_fork(struct task_struct *task, bool inherit)
{
#if IS_ENABLED(CONFIG_KVM)
	if (!inherit || !current->protected_task_active)
		return;

	get_file(current->protected_task_active);
	task->protected_task_active = current->protected_task_active;
#endif
}

static void kvm_protected_task_cleanup_state(struct file **filep, void **statep)
{
#if IS_ENABLED(CONFIG_KVM)
	if (*statep) {
		struct kvm_protected_task_context *context =
			(*filep)->private_data;

		context->ops->cleanup_exec(*statep);
		*statep = NULL;
	}
	if (*filep) {
		fput(*filep);
		*filep = NULL;
	}
#endif
}

static void kvm_protected_task_cleanup_active(struct task_struct *task)
{
#if IS_ENABLED(CONFIG_KVM)
	if (task->protected_task_running && task->protected_task_active &&
	    !task->protected_task_retired) {
		WARN_ON_ONCE(task->protected_task_retired_state);
		task->protected_task_retired = task->protected_task_active;
		task->protected_task_retired_state = task->protected_task_state;
		task->protected_task_active = NULL;
		task->protected_task_state = NULL;
		return;
	}

	kvm_protected_task_cleanup_state(&task->protected_task_active,
					 &task->protected_task_state);
#endif
}

static void kvm_protected_task_cleanup_retired(struct task_struct *task)
{
#if IS_ENABLED(CONFIG_KVM)
	kvm_protected_task_cleanup_state(&task->protected_task_retired,
					 &task->protected_task_retired_state);
#endif
}

void kvm_protected_task_cleanup(struct task_struct *task)
{
#if IS_ENABLED(CONFIG_KVM)
	struct file *file = xchg(&task->protected_task_pending, NULL);

	if (file)
		fput(file);
	task->protected_task_running = false;
#endif
	kvm_protected_task_cleanup_active(task);
	kvm_protected_task_cleanup_retired(task);
}

void kvm_protected_task_take_exec(struct linux_binprm *bprm)
{
#if IS_ENABLED(CONFIG_KVM)
	bprm->protected_task = xchg(&current->protected_task_pending, NULL);
	if (!bprm->protected_task && current->protected_task_active) {
		get_file(current->protected_task_active);
		bprm->protected_task = current->protected_task_active;
	}
#endif
}

void kvm_protected_task_cleanup_exec(struct linux_binprm *bprm)
{
#if IS_ENABLED(CONFIG_KVM)
	if (bprm->protected_task_state) {
		struct kvm_protected_task_context *context =
			bprm->protected_task->private_data;

		context->ops->cleanup_exec(bprm->protected_task_state);
		bprm->protected_task_state = NULL;
	}
	if (bprm->protected_task) {
		fput(bprm->protected_task);
		bprm->protected_task = NULL;
	}
#endif
}

int kvm_protected_task_prepare_exec(struct linux_binprm *bprm)
{
#if IS_ENABLED(CONFIG_KVM)
	struct kvm_protected_task_context *context;

	if (!bprm->protected_task)
		return 0;

	context = bprm->protected_task->private_data;
	return context->ops->stage_exec(context, bprm,
					&bprm->protected_task_state);
#else
	return 0;
#endif
}

unsigned long kvm_protected_task_elf_hwcap(struct linux_binprm *bprm,
					   unsigned int type,
					   unsigned long value)
{
#if IS_ENABLED(CONFIG_KVM)
	struct kvm_protected_task_context *context;
	struct file *file = bprm->protected_task;
	void *state = bprm->protected_task_state;

	if (!file) {
		file = current->protected_task_active;
		state = current->protected_task_state;
	}
	if (!file || !state)
		return value;

	context = file->private_data;
	if (context->ops->elf_hwcap)
		return context->ops->elf_hwcap(state, type, value);
#endif
	return value;
}

void kvm_protected_task_deactivate_exec(void)
{
#if IS_ENABLED(CONFIG_KVM)
	if (current->protected_task_active && current->protected_task_state) {
		struct kvm_protected_task_context *context =
			current->protected_task_active->private_data;

		context->ops->deactivate_exec(current->protected_task_state);
	}
#endif
	kvm_protected_task_cleanup_active(current);
}

void kvm_protected_task_commit_exec(struct linux_binprm *bprm)
{
#if IS_ENABLED(CONFIG_KVM)
	if (!bprm->protected_task)
		return;

	WARN_ON_ONCE(current->protected_task_active ||
		     current->protected_task_state);
	current->protected_task_active = bprm->protected_task;
	current->protected_task_state = bprm->protected_task_state;
	bprm->protected_task = NULL;
	bprm->protected_task_state = NULL;
#endif
}

int kvm_protected_task_finalize_exec(struct pt_regs *regs)
{
#if IS_ENABLED(CONFIG_KVM)
	struct kvm_protected_task_context *context;

	if (!current->protected_task_active)
		return 0;

	context = current->protected_task_active->private_data;
	return context->ops->finalize_exec(current->protected_task_state, regs);
#else
	return 0;
#endif
}

bool kvm_protected_task_run(struct pt_regs *regs)
{
#if IS_ENABLED(CONFIG_KVM)
	struct kvm_protected_task_context *context;
	int ret;

	if (!current->protected_task_active)
		return false;

	context = current->protected_task_active->private_data;
	current->protected_task_running = true;
	local_irq_enable();
	if (!current->protected_task_state)
		ret = context->ops->clone_exec(context, regs,
					       &current->protected_task_state);
	else
		ret = 0;
	if (!ret)
		ret = context->ops->run(current->protected_task_state, regs);
	current->protected_task_running = false;
	kvm_protected_task_cleanup_retired(current);
	if (ret)
		force_sig(SIGKILL);
	local_irq_disable();
	return true;
#else
	return false;
#endif
}

void kvm_protected_task_exit(struct task_struct *task)
{
	kvm_protected_task_cleanup(task);
}
