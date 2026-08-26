/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KVM_PROTECTED_TASK_H
#define _LINUX_KVM_PROTECTED_TASK_H

#include <linux/compiler_types.h>
#include <linux/types.h>

struct linux_binprm;
struct pt_regs;
struct task_struct;

struct kvm_protected_task_context;

struct kvm_protected_task_failure {
	const char *category;
	const char *phase;
	const char *reason;
	unsigned long rip;
	u32 exit_reason;
};

/*
 * The retained context file pins the provider while these callbacks run.
 * stage_exec may set @state on error; cleanup_exec runs before bprm->mm is put.
 */
struct kvm_protected_task_ops {
	/* @state may be populated even when staging returns an error. */
	int (*stage_exec)(struct kvm_protected_task_context *context,
			  struct linux_binprm *bprm, void **state);
	int (*clone_exec)(struct kvm_protected_task_context *context,
			  struct pt_regs *regs, void **state);
	unsigned long (*elf_hwcap)(void *state, unsigned int type,
				    unsigned long value);
	u64 (*xfeatures)(void *state);
	unsigned int (*max_tag_bits)(void *state);
	bool (*needs_pgtable_update)(void *state);
	int (*begin_mm_update)(void *state);
	void (*end_mm_update)(void *state);
	void (*deactivate_exec)(void *state);
	int (*finalize_exec)(void *state, struct pt_regs *regs);
	void (*prepare_user_work)(void *state);
	int (*run)(void *state, struct pt_regs *regs,
		   struct kvm_protected_task_failure *failure);
	void (*cleanup_exec)(void *state);
};

struct kvm_protected_task_context {
	const struct kvm_protected_task_ops *ops;
};

bool kvm_protected_task_can_arm(void);
bool kvm_protected_task_is_active(void);
bool kvm_protected_task_can_block_step(struct task_struct *task);
u64 kvm_protected_task_xfeatures(void);
unsigned int kvm_protected_task_max_tag_bits(void);
bool kvm_protected_task_needs_pgtable_update(void);
int kvm_protected_task_begin_mm_update(void);
void kvm_protected_task_end_mm_update(bool changed);
void kvm_protected_task_init(struct task_struct *task);
void kvm_protected_task_fork(struct task_struct *task, bool inherit);
void kvm_protected_task_cleanup(struct task_struct *task);
void kvm_protected_task_take_exec(struct linux_binprm *bprm);
void kvm_protected_task_cleanup_exec(struct linux_binprm *bprm);
int kvm_protected_task_prepare_exec(struct linux_binprm *bprm);
unsigned long kvm_protected_task_elf_hwcap(struct linux_binprm *bprm,
					   unsigned int type,
					   unsigned long value);
void kvm_protected_task_deactivate_exec(void);
void kvm_protected_task_commit_exec(struct linux_binprm *bprm);
int kvm_protected_task_finalize_exec(struct pt_regs *regs);
void kvm_protected_task_prepare_user_work(void);
bool kvm_protected_task_run(struct pt_regs *regs);
void kvm_protected_task_exit(struct task_struct *task);
int kvm_protected_task_create_fd(void __user *argp);

#endif /* _LINUX_KVM_PROTECTED_TASK_H */
