/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KVM_PROTECTED_TASK_H
#define _LINUX_KVM_PROTECTED_TASK_H

#include <linux/compiler_types.h>

struct linux_binprm;
struct pt_regs;
struct task_struct;

struct kvm_protected_task_context;

/*
 * The retained context file pins the provider while these callbacks run.
 * stage_exec may set @state on error; cleanup_exec runs before bprm->mm is put.
 */
struct kvm_protected_task_ops {
	/* @state may be populated even when staging returns an error. */
	int (*stage_exec)(struct kvm_protected_task_context *context,
			  struct linux_binprm *bprm, void **state);
	int (*finalize_exec)(void *state, struct pt_regs *regs);
	int (*run)(void *state, struct pt_regs *regs);
	void (*cleanup_exec)(void *state);
};

struct kvm_protected_task_context {
	const struct kvm_protected_task_ops *ops;
};

bool kvm_protected_task_can_arm(void);
void kvm_protected_task_init(struct task_struct *task);
void kvm_protected_task_cleanup(struct task_struct *task);
void kvm_protected_task_take_exec(struct linux_binprm *bprm);
void kvm_protected_task_cleanup_exec(struct linux_binprm *bprm);
int kvm_protected_task_prepare_exec(struct linux_binprm *bprm);
void kvm_protected_task_deactivate_exec(void);
void kvm_protected_task_commit_exec(struct linux_binprm *bprm);
int kvm_protected_task_finalize_exec(struct pt_regs *regs);
bool kvm_protected_task_run(struct pt_regs *regs);
void kvm_protected_task_exit(struct task_struct *task);

#endif /* _LINUX_KVM_PROTECTED_TASK_H */
