// SPDX-License-Identifier: GPL-2.0-only

#include <linux/err.h>
#include <linux/kvm_host.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include <asm/fpu/api.h>
#include <asm/fsgsbase.h>
#include <asm/msr-index.h>
#include <asm/pgtable_types.h>
#include <asm/processor-flags.h>
#include <asm/segment.h>
#include <asm/syscall.h>

#include "cpuid.h"

#ifdef CONFIG_X86_64

#define KVM_PT_VA_LIMIT		BIT_ULL(47)
#define KVM_PT_NONLEAF_FLAGS	(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER | _PAGE_ACCESSED)
#define KVM_PT_SYSCALL_PORT	0xec
/* Each edge of the hidden range can require one PMD and one PTE page. */
#define KVM_PT_HOLE_TABLE_PAGES	4
#define KVM_PT_IMAGE_PAGES	(2 + (KVM_PT_VA_LIMIT >> PGDIR_SHIFT) + \
				 KVM_PT_HOLE_TABLE_PAGES)

struct kvm_protected_task_x86 {
	unsigned long pgtable_addr;
	unsigned long pgtable_used;
	unsigned long pgd;
	unsigned long syscall_stub;
	bool fpu_active;
};

struct kvm_protected_task_builder {
	u8 *image;
	unsigned long base;
	unsigned long size;
	unsigned long used;
};

static int kvm_protected_task_validate_mm(struct mm_struct *mm)
{
	struct vm_area_struct *vma;
	VMA_ITERATOR(vmi, mm, 0);
	int ret = 0;

	mmap_read_lock(mm);
	for_each_vma(vmi, vma)
		if (vma->vm_end > KVM_PT_VA_LIMIT ||
		    ((vma->vm_flags & (VM_WRITE | VM_EXEC)) &&
		     !(vma->vm_flags & VM_READ))) {
			ret = -EOPNOTSUPP;
			break;
		}
	mmap_read_unlock(mm);

	return ret;
}

static u64 *kvm_protected_task_alloc_table(struct kvm_protected_task_builder *builder,
					   unsigned long *gpa)
{
	u64 *table;

	if (builder->used > builder->size - PAGE_SIZE)
		return NULL;

	table = (u64 *)(builder->image + builder->used);
	*gpa = builder->base + builder->used;
	builder->used += PAGE_SIZE;
	return table;
}

static u64 *kvm_protected_task_next_table(struct kvm_protected_task_builder *builder,
					  u64 *entry)
{
	unsigned long gpa;
	u64 *table;

	if (*entry & _PAGE_PRESENT) {
		if (*entry & _PAGE_PSE)
			return ERR_PTR(-EEXIST);
		gpa = *entry & PTE_PFN_MASK;
		return (u64 *)(builder->image + gpa - builder->base);
	}

	table = kvm_protected_task_alloc_table(builder, &gpa);
	if (!table)
		return ERR_PTR(-ENOSPC);
	*entry = gpa | KVM_PT_NONLEAF_FLAGS;
	return table;
}

static int kvm_protected_task_map_page(struct kvm_protected_task_builder *builder,
				       unsigned long address, unsigned long gpa,
				       unsigned int shift,
				       vm_flags_t vm_flags)
{
	u64 flags = _PAGE_PRESENT | _PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY;
	u64 *pml4, *pud, *pmd, *pte, *entry;

	if (vm_flags & VM_WRITE)
		flags |= _PAGE_RW;
	if (!(vm_flags & VM_EXEC))
		flags |= _PAGE_NX;
	if (shift != PAGE_SHIFT)
		flags |= _PAGE_PSE;

	pml4 = (u64 *)builder->image;
	entry = &pml4[(address >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1)];
	pud = kvm_protected_task_next_table(builder, entry);
	if (IS_ERR(pud))
		return PTR_ERR(pud);

	entry = &pud[(address >> PUD_SHIFT) & (PTRS_PER_PUD - 1)];
	if (shift == PUD_SHIFT)
		goto set_leaf;
	pmd = kvm_protected_task_next_table(builder, entry);
	if (IS_ERR(pmd))
		return PTR_ERR(pmd);

	entry = &pmd[(address >> PMD_SHIFT) & (PTRS_PER_PMD - 1)];
	if (shift == PMD_SHIFT)
		goto set_leaf;
	pte = kvm_protected_task_next_table(builder, entry);
	if (IS_ERR(pte))
		return PTR_ERR(pte);

	entry = &pte[(address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1)];

set_leaf:
	if (*entry & _PAGE_PRESENT)
		return -EEXIST;
	*entry = (gpa & PTE_PFN_MASK) | flags;
	return 0;
}

static int kvm_protected_task_map_range(struct kvm_protected_task_builder *builder,
					unsigned long start, unsigned long end,
					vm_flags_t vm_flags)
{
	unsigned long address = start;

	while (address < end) {
		unsigned int shift;
		int ret;

		if (IS_ALIGNED(address, PUD_SIZE) &&
		    end - address >= PUD_SIZE)
			shift = PUD_SHIFT;
		else if (IS_ALIGNED(address, PMD_SIZE) &&
			 end - address >= PMD_SIZE)
			shift = PMD_SHIFT;
		else
			shift = PAGE_SHIFT;

		ret = kvm_protected_task_map_page(builder, address, address, shift,
						  vm_flags);
		if (ret)
			return ret;
		address += 1UL << shift;
	}

	return 0;
}

static void kvm_protected_task_set_code_segment(struct kvm_segment *segment)
{
	memset(segment, 0, sizeof(*segment));
	segment->selector = __USER_CS;
	segment->limit = UINT_MAX;
	segment->type = 0xb;
	segment->s = 1;
	segment->dpl = 3;
	segment->present = 1;
	segment->l = 1;
	segment->g = 1;
}

static void kvm_protected_task_set_data_segment(struct kvm_segment *segment)
{
	memset(segment, 0, sizeof(*segment));
	segment->selector = __USER_DS;
	segment->limit = UINT_MAX;
	segment->type = 0x3;
	segment->s = 1;
	segment->dpl = 3;
	segment->present = 1;
	segment->db = 1;
	segment->g = 1;
}

static int kvm_protected_task_setup_sregs(struct kvm_vcpu *vcpu,
					  unsigned long pgd)
{
	struct kvm_sregs sregs;
	int ret;

	ret = kvm_arch_vcpu_ioctl_get_sregs(vcpu, &sregs);
	if (ret)
		return ret;

	kvm_protected_task_set_code_segment(&sregs.cs);
	kvm_protected_task_set_data_segment(&sregs.ds);
	kvm_protected_task_set_data_segment(&sregs.es);
	kvm_protected_task_set_data_segment(&sregs.fs);
	kvm_protected_task_set_data_segment(&sregs.gs);
	kvm_protected_task_set_data_segment(&sregs.ss);
	sregs.fs.base = x86_fsbase_read_task(current);
	sregs.gs.base = x86_gsbase_read_task(current);
	sregs.cr0 = X86_CR0_PE | X86_CR0_NE | X86_CR0_PG;
	sregs.cr3 = pgd;
	sregs.cr4 = X86_CR4_PAE | X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT;
	sregs.efer = EFER_SCE | EFER_LME | EFER_LMA | EFER_NX;

	return kvm_arch_vcpu_ioctl_set_sregs(vcpu, &sregs);
}

static int kvm_protected_task_setup_regs(struct kvm_vcpu *vcpu,
					 struct pt_regs *regs)
{
	struct kvm_regs kvm_regs = {
		.rax = regs->ax,
		.rbx = regs->bx,
		.rcx = regs->cx,
		.rdx = regs->dx,
		.rsi = regs->si,
		.rdi = regs->di,
		.rsp = regs->sp,
		.rbp = regs->bp,
		.r8 = regs->r8,
		.r9 = regs->r9,
		.r10 = regs->r10,
		.r11 = regs->r11,
		.r12 = regs->r12,
		.r13 = regs->r13,
		.r14 = regs->r14,
		.r15 = regs->r15,
		.rip = regs->ip,
		.rflags = regs->flags,
	};

	return kvm_arch_vcpu_ioctl_set_regs(vcpu, &kvm_regs);
}

static int kvm_protected_task_setup_msrs(struct kvm_vcpu *vcpu,
					 struct kvm_protected_task_x86 *state)
{
	u64 star = (u64)__USER32_CS << 48 | (u64)__KERNEL_CS << 32;
	u64 mask = X86_EFLAGS_TF | X86_EFLAGS_DF | X86_EFLAGS_IF |
		   X86_EFLAGS_AC;
	int ret;

	vcpu_load(vcpu);
	ret = kvm_msr_write(vcpu, MSR_STAR, star);
	if (!ret)
		ret = kvm_msr_write(vcpu, MSR_LSTAR, state->syscall_stub);
	if (!ret)
		ret = kvm_msr_write(vcpu, MSR_SYSCALL_MASK, mask);
	if (!ret)
		ret = kvm_msr_write(vcpu, MSR_KERNEL_GS_BASE, 0);
	vcpu_put(vcpu);

	return ret ? -EINVAL : 0;
}

static int kvm_protected_task_sync_regs(struct kvm_vcpu *vcpu,
					struct pt_regs *regs, bool syscall)
{
	struct kvm_regs kvm_regs;
	int ret;

	ret = kvm_arch_vcpu_ioctl_get_regs(vcpu, &kvm_regs);
	if (ret)
		return ret;

	regs->r15 = kvm_regs.r15;
	regs->r14 = kvm_regs.r14;
	regs->r13 = kvm_regs.r13;
	regs->r12 = kvm_regs.r12;
	regs->bp = kvm_regs.rbp;
	regs->bx = kvm_regs.rbx;
	regs->r11 = kvm_regs.r11;
	regs->r10 = kvm_regs.r10;
	regs->r9 = kvm_regs.r9;
	regs->r8 = kvm_regs.r8;
	regs->cx = kvm_regs.rcx;
	regs->dx = kvm_regs.rdx;
	regs->si = kvm_regs.rsi;
	regs->di = kvm_regs.rdi;
	regs->sp = kvm_regs.rsp;
	regs->cs = __USER_CS;
	regs->ss = __USER_DS;
	if (syscall) {
		regs->orig_ax = kvm_regs.rax;
		regs->ax = kvm_regs.rax;
		regs->ip = kvm_regs.rcx;
		regs->flags = kvm_regs.r11;
	} else {
		regs->orig_ax = -1;
		regs->ax = kvm_regs.rax;
		regs->ip = kvm_regs.rip;
		regs->flags = kvm_regs.rflags;
	}

	return 0;
}

/* Native signal setup and rt_sigreturn operate on current's active fpstate. */
static int kvm_protected_task_activate_fpu(struct kvm_vcpu *vcpu,
					   struct kvm_protected_task_x86 *state)
{
	int ret;

	if (state->fpu_active)
		return 0;

	ret = fpu_swap_kvm_fpstate(&vcpu->arch.guest_fpu, true);
	if (!ret)
		state->fpu_active = true;
	return ret;
}

static int kvm_protected_task_deactivate_fpu(struct kvm_vcpu *vcpu,
					     struct kvm_protected_task_x86 *state)
{
	int ret;

	if (!state->fpu_active)
		return 0;

	ret = fpu_swap_kvm_fpstate(&vcpu->arch.guest_fpu, false);
	if (!ret)
		state->fpu_active = false;
	return ret;
}

static int kvm_protected_task_handle_memory_fault(struct kvm_vcpu *vcpu,
						  struct pt_regs *regs,
						  u32 *next_slot)
{
	unsigned long address = vcpu->run->memory_fault.gpa & PAGE_MASK;
	struct vm_area_struct *vma;
	unsigned long end = 0;
	bool mapped = false, visible;
	int idx, ret;

	ret = kvm_protected_task_sync_regs(vcpu, regs, false);
	if (ret)
		return ret;

	idx = srcu_read_lock(&vcpu->kvm->srcu);
	visible = kvm_vcpu_is_visible_gfn(vcpu, address >> PAGE_SHIFT);
	srcu_read_unlock(&vcpu->kvm->srcu, idx);

	mmap_read_lock(current->mm);
	vma = vma_lookup(current->mm, address);
	if (vma) {
		mapped = true;
		end = vma->vm_end;
	}
	mmap_read_unlock(current->mm);

	if (!mapped || visible) {
		force_sig_fault(SIGSEGV, mapped ? SEGV_ACCERR : SEGV_MAPERR,
				(void __user *)address);
		return 0;
	}

	ret = kvm_map_user_memory_region(vcpu->kvm, (*next_slot)++,
					 address, end);
	return ret == -EEXIST ? 0 : ret;
}

static int kvm_protected_task_build_page_tables(struct kvm_vcpu *vcpu,
						struct kvm_protected_task_x86 *state,
						u32 slot)
{
	static const u8 syscall_stub[] = {
		0xe6, KVM_PT_SYSCALL_PORT, /* out KVM_PT_SYSCALL_PORT, %al */
		0x0f, 0x0b,              /* ud2 */
	};
	struct kvm_protected_task_builder builder;
	struct kvm_userspace_memory_region2 region = {
		.slot = slot,
		.flags = KVM_MEM_READONLY,
	};
	unsigned long address, pgd, stub_gpa;
	const unsigned long size = KVM_PT_IMAGE_PAGES * PAGE_SIZE;
	u64 *stub_page;
	int ret;

	ret = kvm_protected_task_validate_mm(current->mm);
	if (ret)
		return ret;

	address = vm_mmap(NULL, 0, size, PROT_READ | PROT_WRITE,
			  MAP_PRIVATE | MAP_ANONYMOUS, 0);
	if (IS_ERR_VALUE(address))
		return address;
	state->pgtable_addr = address;
	if (state->pgtable_addr > KVM_PT_VA_LIMIT - size) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	builder.image = vzalloc(size);
	if (!builder.image) {
		ret = -ENOMEM;
		goto out;
	}
	builder.base = state->pgtable_addr;
	builder.size = size;
	builder.used = 0;
	if (!kvm_protected_task_alloc_table(&builder, &pgd)) {
		ret = -ENOSPC;
		goto free_image;
	}

	ret = kvm_protected_task_map_range(&builder, 0,
					       state->pgtable_addr,
					       VM_READ | VM_WRITE | VM_EXEC);
	if (ret)
		goto free_image;
	ret = kvm_protected_task_map_range(&builder,
					       state->pgtable_addr + size,
					       KVM_PT_VA_LIMIT,
					       VM_READ | VM_WRITE | VM_EXEC);
	if (ret)
		goto free_image;
	stub_page = kvm_protected_task_alloc_table(&builder, &stub_gpa);
	if (!stub_page) {
		ret = -ENOSPC;
		goto free_image;
	}
	memcpy(stub_page, syscall_stub, sizeof(syscall_stub));
	ret = kvm_protected_task_map_page(&builder, stub_gpa, stub_gpa,
					  PAGE_SHIFT, VM_READ | VM_EXEC);
	if (ret)
		goto free_image;
	if (copy_to_user((void __user *)state->pgtable_addr, builder.image,
			 builder.used)) {
		ret = -EFAULT;
		goto free_image;
	}
	state->pgtable_used = builder.used;
	state->pgd = pgd;
	state->syscall_stub = stub_gpa;

	/* Only the protected memslot may force-access the hidden image. */
	ret = vm_mprotect(state->pgtable_addr, size, PROT_NONE);
	if (ret)
		goto free_image;
	ret = do_mseal(state->pgtable_addr, size, 0);
	if (ret)
		goto free_image;

	region.guest_phys_addr = state->pgtable_addr;
	region.userspace_addr = state->pgtable_addr;
	region.memory_size = size;
	ret = kvm_set_protected_task_memory_region(vcpu->kvm, &region);

free_image:
	vfree(builder.image);
out:
	return ret;
}

int kvm_arch_protected_task_prepare(struct kvm_vcpu *vcpu, void **statep)
{
	struct kvm_protected_task_x86 *state;
	int ret;

	state = kzalloc_obj(*state);
	if (!state)
		return -ENOMEM;

	ret = kvm_vcpu_set_supported_cpuid(vcpu);
	if (ret) {
		kfree(state);
		return ret;
	}

	*statep = state;
	return 0;
}

u64 kvm_arch_protected_task_features(void)
{
	return KVM_PROTECTED_TASK_FEATURE_EXEC;
}

int kvm_arch_protected_task_finalize(struct kvm_vcpu *vcpu, void *arch_state,
				     struct pt_regs *regs, u32 slot)
{
	struct kvm_protected_task_x86 *state = arch_state;
	int ret;

	if (WARN_ON_ONCE(current->mm != vcpu->kvm->mm))
		return -EIO;

	ret = kvm_protected_task_build_page_tables(vcpu, state, slot);
	if (ret)
		return ret;
	ret = kvm_protected_task_setup_sregs(vcpu, state->pgd);
	if (ret)
		return ret;
	ret = kvm_protected_task_setup_msrs(vcpu, state);
	if (ret)
		return ret;
	return kvm_protected_task_setup_regs(vcpu, regs);
}

int kvm_arch_protected_task_run(struct kvm_vcpu *vcpu, void *arch_state,
				struct pt_regs *regs, u32 *next_slot)
{
	struct kvm_protected_task_x86 *state = arch_state;
	int ret;

	ret = kvm_protected_task_deactivate_fpu(vcpu, state);
	if (ret)
		return ret;

	ret = kvm_protected_task_setup_sregs(vcpu, state->pgd);
	if (ret)
		return ret;
	ret = kvm_protected_task_setup_regs(vcpu, regs);
	if (ret)
		return ret;

	ret = kvm_vcpu_run(vcpu);
	if (ret == -EINTR) {
		ret = kvm_protected_task_sync_regs(vcpu, regs, false);
		goto out;
	}
	if (ret && (ret != -EFAULT ||
		    vcpu->run->exit_reason != KVM_EXIT_MEMORY_FAULT))
		goto out;

	switch (vcpu->run->exit_reason) {
	case KVM_EXIT_IO:
		if (vcpu->run->io.direction != KVM_EXIT_IO_OUT ||
		    vcpu->run->io.size != 1 || vcpu->run->io.count != 1 ||
		    vcpu->run->io.port != KVM_PT_SYSCALL_PORT) {
			ret = -EIO;
			break;
		}
		ret = kvm_protected_task_sync_regs(vcpu, regs, true);
		if (ret)
			break;
		if (regs->orig_ax == __NR_rt_sigreturn) {
			ret = kvm_protected_task_activate_fpu(vcpu, state);
			if (ret)
				break;
		}
		do_protected_syscall_64(regs);
		ret = 0;
		break;
	case KVM_EXIT_MEMORY_FAULT:
		ret = kvm_protected_task_handle_memory_fault(vcpu, regs,
						     next_slot);
		break;
	default:
		ret = -EIO;
		break;
	}

out:
	return kvm_protected_task_activate_fpu(vcpu, state) ?: ret;
}

void kvm_arch_protected_task_cleanup(struct kvm_vcpu *vcpu, void *arch_state)
{
	struct kvm_protected_task_x86 *state = arch_state;

	if (!state)
		return;
	WARN_ON_ONCE(kvm_protected_task_deactivate_fpu(vcpu, state));
	kfree(state);
}

#else

int kvm_arch_protected_task_prepare(struct kvm_vcpu *vcpu, void **state)
{
	return -EOPNOTSUPP;
}

u64 kvm_arch_protected_task_features(void)
{
	return 0;
}

int kvm_arch_protected_task_finalize(struct kvm_vcpu *vcpu, void *state,
				     struct pt_regs *regs, u32 slot)
{
	return -EOPNOTSUPP;
}

int kvm_arch_protected_task_run(struct kvm_vcpu *vcpu, void *state,
				struct pt_regs *regs, u32 *next_slot)
{
	return -EOPNOTSUPP;
}

void kvm_arch_protected_task_cleanup(struct kvm_vcpu *vcpu, void *state)
{
}

#endif
