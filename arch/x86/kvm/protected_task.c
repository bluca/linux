// SPDX-License-Identifier: GPL-2.0-only

#include <linux/err.h>
#include <linux/kvm_host.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/overflow.h>
#include <linux/pkeys.h>
#include <linux/refcount.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/thread_info.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include <asm/fpu/api.h>
#include <asm/fpu/xcr.h>
#include <asm/fsgsbase.h>
#include <asm/cpufeature.h>
#include <asm/mmu_context.h>
#include <asm/msr-index.h>
#include <asm/nospec-branch.h>
#include <asm/pkru.h>
#include <asm/pgtable_types.h>
#include <asm/prctl.h>
#include <asm/processor-flags.h>
#include <asm/ptrace.h>
#include <asm/segment.h>
#include <asm/syscall.h>
#include <asm/traps.h>

#include <uapi/linux/auxvec.h>

#include "cpuid.h"
#include "lapic.h"
#include "x86.h"

void kvm_arch_protected_task_kick(struct kvm_vcpu *vcpu)
{
	kvm_make_request_and_kick(KVM_REQ_PROTECTED_TASK_EXIT, vcpu);
}

#ifdef CONFIG_X86_64

#define KVM_PT_VA_LIMIT_L4	BIT_ULL(47)
#define KVM_PT_VA_LIMIT_L5	BIT_ULL(48)
#define KVM_PT_PML4_SHIFT	39
#define KVM_PT_PML5_SHIFT	48
#define KVM_PT_NONLEAF_FLAGS	(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER | _PAGE_ACCESSED)
#define KVM_PT_CPUID_1_ECX_BASELINE (BIT(0) | BIT(1) | BIT(9) | BIT(19) | \
				     BIT(20) | BIT(22) | BIT(23) | BIT(25) | BIT(30))
#define KVM_PT_CPUID_1_ECX_XSAVE BIT(26)
#define KVM_PT_CPUID_1_ECX_AVX BIT(28)
#define KVM_PT_CPUID_1_ECX_YMM (BIT(12) | KVM_PT_CPUID_1_ECX_AVX | BIT(29))
#define KVM_PT_CPUID_1_EDX	(BIT(0) | BIT(8) | BIT(15) | BIT(23) | \
				 BIT(24) | BIT(25) | BIT(26))
#define KVM_PT_CPUID_7_EBX_BASELINE (BIT(3) | BIT(8) | BIT(18) | BIT(19) | \
				     BIT(23) | BIT(24) | BIT(29))
#define KVM_PT_CPUID_7_EBX_AVX2 BIT(5)
#define KVM_PT_CPUID_7_EBX_AVX512F BIT(16)
#define KVM_PT_CPUID_7_EBX_AVX512 (KVM_PT_CPUID_7_EBX_AVX512F | BIT(17) | \
				   BIT(21) | BIT(26) | BIT(27) | BIT(28) | \
				   BIT(30) | BIT(31))
#define KVM_PT_CPUID_7_ECX_BASELINE BIT(22)
#define KVM_PT_CPUID_7_ECX_LA57 BIT(16)
#define KVM_PT_CPUID_7_ECX_YMM (BIT(8) | BIT(9) | BIT(10))
#define KVM_PT_CPUID_7_ECX_ZMM (BIT(1) | BIT(6) | BIT(11) | BIT(12) | BIT(14))
#define KVM_PT_CPUID_7_ECX_PKU	BIT(3)
#define KVM_PT_CPUID_7_ECX_SHSTK BIT(7)
#define KVM_PT_CPUID_7_EDX_AVX512 (BIT(2) | BIT(3) | BIT(8) | BIT(23))
#define KVM_PT_CPUID_7_EDX_AMX_TILE BIT(24)
#define KVM_PT_CPUID_7_EDX_AMX_COMPUTE (BIT(22) | BIT(25))
#define KVM_PT_CPUID_7_1_EAX_YMM (BIT(0) | BIT(1) | BIT(2) | BIT(4) | BIT(23))
#define KVM_PT_CPUID_7_1_EAX_AVX512_BF16 BIT(5)
#define KVM_PT_CPUID_7_1_EAX_AMX BIT(21)
#define KVM_PT_CPUID_7_1_EAX_LAM BIT(26)
#define KVM_PT_CPUID_7_1_EDX_AVX (BIT(4) | BIT(5) | BIT(10))
#define KVM_PT_CPUID_7_1_EDX_AMX BIT(8)
#define KVM_PT_CPUID_7_1_EDX_AVX10 BIT(19)
#define KVM_PT_CPUID_D_1_EAX_XGETBV1 BIT(2)
#define KVM_PT_CPUID_D_1_EAX_XSAVES BIT(3)
#define KVM_PT_CPUID_D_1_EAX_XFD BIT(4)
#define KVM_PT_CPUID_24_0_EBX_WIDTHS (BIT(16) | BIT(17) | BIT(18))
#define KVM_PT_CPUID_24_1_ECX_VNNI_INT BIT(2)
#define KVM_PT_CPUID_1E_1_EAX_AMX_ALIASES GENMASK(3, 0)
#define KVM_PT_CPUID_80000001_ECX (BIT(0) | BIT(5))
#define KVM_PT_CPUID_80000001_EDX (BIT(11) | BIT(20) | BIT(27) | BIT(29))
#define KVM_PT_CPUID_80000008_EBX BIT(0)
#define KVM_PT_AMX_TILE_BYTES	8192
#define KVM_PT_AMX_BYTES_PER_TILE 1024
#define KVM_PT_AMX_BYTES_PER_ROW	64
#define KVM_PT_AMX_NR_TILES	8
#define KVM_PT_AMX_NR_ROWS	16
#define KVM_PT_AMX_TMUL_MAX_K	16
#define KVM_PT_AMX_TMUL_MAX_N	64
#define KVM_PT_LAM_U57_BITS	6
/* Each edge of the hidden range can require one PMD and one PTE page. */
#define KVM_PT_HOLE_TABLE_PAGES	4

struct kvm_pt_cpuid_classes {
	u32 baseline;
	u32 ymm;
	u32 zmm;
	u32 tile;
};

static const struct kvm_pt_cpuid_classes kvm_pt_cpuid_1_ecx = {
	.baseline = KVM_PT_CPUID_1_ECX_BASELINE,
	.ymm = KVM_PT_CPUID_1_ECX_YMM,
};

static const struct kvm_pt_cpuid_classes kvm_pt_cpuid_1_edx = {
	.baseline = KVM_PT_CPUID_1_EDX,
};

static const struct kvm_pt_cpuid_classes kvm_pt_cpuid_7_0_ebx = {
	.baseline = KVM_PT_CPUID_7_EBX_BASELINE,
	.ymm = KVM_PT_CPUID_7_EBX_AVX2,
	.zmm = KVM_PT_CPUID_7_EBX_AVX512,
};

static const struct kvm_pt_cpuid_classes kvm_pt_cpuid_7_0_ecx = {
	.baseline = KVM_PT_CPUID_7_ECX_BASELINE,
	.ymm = KVM_PT_CPUID_7_ECX_YMM,
	.zmm = KVM_PT_CPUID_7_ECX_ZMM,
};

static const struct kvm_pt_cpuid_classes kvm_pt_cpuid_7_0_edx = {
	.zmm = KVM_PT_CPUID_7_EDX_AVX512,
	.tile = KVM_PT_CPUID_7_EDX_AMX_TILE |
		KVM_PT_CPUID_7_EDX_AMX_COMPUTE,
};

static const struct kvm_pt_cpuid_classes kvm_pt_cpuid_7_1_eax = {
	.ymm = KVM_PT_CPUID_7_1_EAX_YMM,
	.zmm = KVM_PT_CPUID_7_1_EAX_AVX512_BF16,
	.tile = KVM_PT_CPUID_7_1_EAX_AMX,
};

static const struct kvm_pt_cpuid_classes kvm_pt_cpuid_7_1_edx = {
	.ymm = KVM_PT_CPUID_7_1_EDX_AVX,
	.tile = KVM_PT_CPUID_7_1_EDX_AMX,
};

static const struct kvm_pt_cpuid_classes kvm_pt_cpuid_80000001_ecx = {
	.baseline = KVM_PT_CPUID_80000001_ECX,
};

static const struct kvm_pt_cpuid_classes kvm_pt_cpuid_80000001_edx = {
	.baseline = KVM_PT_CPUID_80000001_EDX,
};

static const struct kvm_pt_cpuid_classes kvm_pt_cpuid_80000008_ebx = {
	.baseline = KVM_PT_CPUID_80000008_EBX,
};

struct kvm_protected_task_x86_image {
	refcount_t refs;
	struct mm_struct *mm;
	u64 pgtable_gen;
	unsigned long address;
	unsigned long size;
	unsigned long used;
	unsigned long pgd;
	unsigned long syscall_stub;
	unsigned long va_limit;
	bool la57;
	bool pku;
	bool shstk;
};

struct kvm_protected_task_x86 {
	struct kvm_protected_task_x86_image *image;
	bool avx;
	bool avx512;
	bool pku;
	bool la57;
	bool lam;
	bool shstk;
	bool amx;
	bool avx10;
	bool fpu_active;
	bool sregs_valid;
	bool task_msrs_valid;
	bool debugregs_valid;
	bool fast_syscall_return;
	u8 avx10_version;
	u8 avx10_max_subleaf;
	u8 amx_max_subleaf;
	u32 avx10_1_ecx;
	u32 amx_1e_1_eax;
	unsigned long va_limit;
	unsigned long fsbase;
	unsigned long gsbase;
	unsigned long debugreg[HBP_NUM];
	unsigned long dr7;
	u64 cr3;
	u64 cr4;
	u64 misc_features;
	u64 spec_ctrl;
	u64 virt_spec_ctrl;
	unsigned int static_user_size;
	unsigned int dynamic_user_size;
};

static u32 kvm_protected_task_cpuid_class_mask(
		struct kvm_protected_task_x86 *state,
		const struct kvm_pt_cpuid_classes *classes)
{
	u32 mask = classes->baseline;

	if (state->avx)
		mask |= classes->ymm;
	if (state->avx512)
		mask |= classes->zmm;
	if (state->amx)
		mask |= classes->tile;

	return mask;
}

struct kvm_protected_task_builder {
	u8 *image;
	unsigned long base;
	unsigned long size;
	unsigned long used;
	unsigned long va_limit;
	bool la57;
};

static u64 kvm_protected_task_xcr0(struct kvm_protected_task_x86 *state)
{
	return XFEATURE_MASK_FPSSE |
		(state->avx ? XFEATURE_MASK_YMM : 0) |
		(state->avx512 ? XFEATURE_MASK_AVX512 : 0) |
		(state->pku ? XFEATURE_MASK_PKRU : 0) |
		(state->amx ? XFEATURE_MASK_XTILE : 0);
}

static u64 kvm_protected_task_profile_xfeatures(
		struct kvm_protected_task_x86 *state)
{
	return kvm_protected_task_xcr0(state) |
		(state->shstk ? XFEATURE_MASK_CET_USER : 0);
}

static bool kvm_protected_task_xstate_component(
		struct kvm_cpuid_entry2 *entry, unsigned int size,
		bool xfd, unsigned int *end)
{
	if (!entry || entry->eax != size || entry->ebx < sizeof(struct xregs_state) ||
	    (entry->ecx & BIT(0)) || !!(entry->ecx & BIT(2)) != xfd ||
	    check_add_overflow(entry->ebx, entry->eax, end))
		return false;

	return true;
}

static void kvm_protected_task_restrict_cpuid(
		struct kvm_vcpu *vcpu, struct kvm_protected_task_x86 *state,
		bool apply)
{
	struct kvm_cpuid_entry2 *leaf1, *leaf7, *leaf71, *leaf80000008;
	struct kvm_cpuid_entry2 *leafd0, *leafd1, *leafd2, *leafd5, *leafd6;
	struct kvm_cpuid_entry2 *leafd7, *leafd9, *leafd11;
	struct kvm_cpuid_entry2 *leafd17, *leafd18, *leaf1d0, *leaf1d1;
	struct kvm_cpuid_entry2 *leaf1e0, *leaf1e1;
	struct kvm_cpuid_entry2 *leaf240, *leaf241;
	unsigned int hi16_zmm_end = 0, opmask_end = 0, tilecfg_end = 0;
	unsigned int tiledata_end = 0, ymm_end = 0, zmm_hi256_end = 0;
	u32 leaf7_1_eax, leaf7_1_edx;
	bool leaf7_enabled;
	u64 xcr0;
	int i;

	leaf1 = kvm_find_cpuid_entry(vcpu, 1);
	leaf7 = kvm_find_cpuid_entry_index(vcpu, 7, 0);
	leaf71 = kvm_find_cpuid_entry_index(vcpu, 7, 1);
	leafd0 = kvm_find_cpuid_entry_index(vcpu, 0xd, 0);
	leafd1 = kvm_find_cpuid_entry_index(vcpu, 0xd, 1);
	leafd2 = kvm_find_cpuid_entry_index(vcpu, 0xd, XFEATURE_YMM);
	leafd5 = kvm_find_cpuid_entry_index(vcpu, 0xd, XFEATURE_OPMASK);
	leafd6 = kvm_find_cpuid_entry_index(vcpu, 0xd, XFEATURE_ZMM_Hi256);
	leafd7 = kvm_find_cpuid_entry_index(vcpu, 0xd, XFEATURE_Hi16_ZMM);
	leafd9 = kvm_find_cpuid_entry_index(vcpu, 0xd, XFEATURE_PKRU);
	leafd11 = kvm_find_cpuid_entry_index(vcpu, 0xd, XFEATURE_CET_USER);
	leafd17 = kvm_find_cpuid_entry_index(vcpu, 0xd, XFEATURE_XTILE_CFG);
	leafd18 = kvm_find_cpuid_entry_index(vcpu, 0xd, XFEATURE_XTILE_DATA);
	leaf1d0 = kvm_find_cpuid_entry_index(vcpu, 0x1d, 0);
	leaf1d1 = kvm_find_cpuid_entry_index(vcpu, 0x1d, 1);
	leaf1e0 = kvm_find_cpuid_entry_index(vcpu, 0x1e, 0);
	leaf1e1 = kvm_find_cpuid_entry_index(vcpu, 0x1e, 1);
	leaf240 = kvm_find_cpuid_entry_index(vcpu, 0x24, 0);
	leaf241 = kvm_find_cpuid_entry_index(vcpu, 0x24, 1);
	leaf80000008 = kvm_find_cpuid_entry(vcpu, 0x80000008);
	state->avx = cpu_feature_enabled(X86_FEATURE_AVX) && leaf1 &&
		(leaf1->ecx & (KVM_PT_CPUID_1_ECX_XSAVE | KVM_PT_CPUID_1_ECX_AVX)) ==
			(KVM_PT_CPUID_1_ECX_XSAVE | KVM_PT_CPUID_1_ECX_AVX) &&
		leafd0 && (leafd0->eax & XFEATURE_MASK_YMM) &&
		kvm_protected_task_xstate_component(leafd2,
				sizeof(struct ymmh_struct), false, &ymm_end) &&
		(vcpu->arch.guest_fpu.fpstate->xfeatures & XFEATURE_MASK_YMM);
	state->avx512 = state->avx && cpu_feature_enabled(X86_FEATURE_AVX512F) &&
		leaf7 && (leaf7->ebx & KVM_PT_CPUID_7_EBX_AVX512F) &&
		leafd0 &&
		(leafd0->eax & XFEATURE_MASK_AVX512) == XFEATURE_MASK_AVX512 &&
		kvm_protected_task_xstate_component(leafd5,
				sizeof(struct avx_512_opmask_state), false,
				&opmask_end) && leafd5->ebx >= ymm_end &&
		kvm_protected_task_xstate_component(leafd6,
				sizeof(struct avx_512_zmm_uppers_state), false,
				&zmm_hi256_end) && leafd6->ebx >= opmask_end &&
		kvm_protected_task_xstate_component(leafd7,
				sizeof(struct avx_512_hi16_state), false,
				&hi16_zmm_end) && leafd7->ebx >= zmm_hi256_end &&
		(vcpu->arch.guest_fpu.fpstate->xfeatures & XFEATURE_MASK_AVX512) ==
			XFEATURE_MASK_AVX512;
	state->pku = leaf1 && (leaf1->ecx & KVM_PT_CPUID_1_ECX_XSAVE) &&
		     leaf7 && (leaf7->ecx & KVM_PT_CPUID_7_ECX_PKU) &&
		     leafd0 && (leafd0->eax & XFEATURE_MASK_PKRU) &&
		     leafd9 && leafd9->eax &&
		     leafd9->ebx >= sizeof(struct xregs_state);
	state->la57 = pgtable_l5_enabled() && kvm_host.maxphyaddr >= 48 &&
		leaf7 && (leaf7->ecx & KVM_PT_CPUID_7_ECX_LA57) &&
		leaf80000008 && (leaf80000008->eax & 0xff) >= 48;
	state->va_limit = state->la57 ? KVM_PT_VA_LIMIT_L5 :
		KVM_PT_VA_LIMIT_L4;
	state->lam = leaf71 && (leaf71->eax & KVM_PT_CPUID_7_1_EAX_LAM);
	state->shstk = cpu_feature_enabled(X86_FEATURE_USER_SHSTK) &&
		leaf1 && (leaf1->ecx & KVM_PT_CPUID_1_ECX_XSAVE) &&
		leaf7 && (leaf7->ecx & KVM_PT_CPUID_7_ECX_SHSTK) &&
		leafd1 && (leafd1->eax & KVM_PT_CPUID_D_1_EAX_XSAVES) &&
		(leafd1->ecx & XFEATURE_MASK_CET_USER) &&
		leafd11 && leafd11->eax == sizeof(struct cet_user_state) &&
		(leafd11->ecx & BIT(0)) &&
		(vcpu->arch.guest_fpu.fpstate->xfeatures &
		 XFEATURE_MASK_CET_USER);
	state->amx = cpu_feature_enabled(X86_FEATURE_AMX_TILE) &&
		leaf7 && (leaf7->edx & KVM_PT_CPUID_7_EDX_AMX_TILE) &&
		leafd0 && (leafd0->eax & XFEATURE_MASK_XTILE) == XFEATURE_MASK_XTILE &&
		leafd1 &&
		(leafd1->eax & (KVM_PT_CPUID_D_1_EAX_XGETBV1 |
				 KVM_PT_CPUID_D_1_EAX_XSAVES |
				 KVM_PT_CPUID_D_1_EAX_XFD)) ==
			(KVM_PT_CPUID_D_1_EAX_XGETBV1 |
			 KVM_PT_CPUID_D_1_EAX_XSAVES |
			 KVM_PT_CPUID_D_1_EAX_XFD) &&
		kvm_protected_task_xstate_component(leafd17,
				sizeof(struct xtile_cfg), false, &tilecfg_end) &&
		kvm_protected_task_xstate_component(leafd18,
				KVM_PT_AMX_TILE_BYTES, true, &tiledata_end) &&
		leafd18->ebx >= tilecfg_end &&
		leaf1d0 && leaf1d0->eax >= 1 && leaf1d1 &&
		(leaf1d1->eax & 0xffff) == KVM_PT_AMX_TILE_BYTES &&
		(leaf1d1->eax >> 16) == KVM_PT_AMX_BYTES_PER_TILE &&
		(leaf1d1->ebx & 0xffff) == KVM_PT_AMX_BYTES_PER_ROW &&
		(leaf1d1->ebx >> 16) == KVM_PT_AMX_NR_TILES &&
		(leaf1d1->ecx & 0xffff) == KVM_PT_AMX_NR_ROWS &&
		leaf1e0 && leaf1e0->eax <= 1 &&
		leaf1e0->ebx == (KVM_PT_AMX_TMUL_MAX_K |
				      KVM_PT_AMX_TMUL_MAX_N << 8) &&
		!leaf1e0->ecx && !leaf1e0->edx;
	if (state->amx && leaf1e0->eax == 1 && leaf1e1 &&
	    !leaf1e1->ebx && !leaf1e1->ecx && !leaf1e1->edx) {
		u32 aliases = 0;

		if (leaf7->edx & BIT(25))
			aliases |= BIT(0);
		if (leaf7->edx & BIT(22))
			aliases |= BIT(1);
		if (leaf71 && (leaf71->edx & BIT(8)))
			aliases |= BIT(2);
		if (leaf71 && (leaf71->eax & BIT(21)))
			aliases |= BIT(3);
		if ((leaf1e1->eax & KVM_PT_CPUID_1E_1_EAX_AMX_ALIASES) ==
		    aliases) {
			state->amx_max_subleaf = 1;
			state->amx_1e_1_eax = aliases;
		}
	}
	if (state->avx512 && leaf71 &&
	    (leaf71->edx & KVM_PT_CPUID_7_1_EDX_AVX10) && leaf240) {
		u8 version = leaf240->ebx & 0xff;

		state->avx10 = version >= 1 && version <= 2 &&
			(leaf240->ebx & KVM_PT_CPUID_24_0_EBX_WIDTHS) ==
				KVM_PT_CPUID_24_0_EBX_WIDTHS &&
			!(leaf240->ebx & ~(GENMASK(7, 0) |
					     KVM_PT_CPUID_24_0_EBX_WIDTHS)) &&
			leaf240->eax <= 1 && !leaf240->ecx && !leaf240->edx &&
			(version < 2 || leaf240->eax == 1) &&
			(!leaf240->eax ||
			 (leaf241 && !leaf241->eax && !leaf241->ebx &&
			  !(leaf241->ecx & ~KVM_PT_CPUID_24_1_ECX_VNNI_INT) &&
			  !leaf241->edx));
		if (state->avx10) {
			state->avx10_version = version;
			state->avx10_max_subleaf = leaf240->eax;
			state->avx10_1_ecx = leaf240->eax ?
				leaf241->ecx & KVM_PT_CPUID_24_1_ECX_VNNI_INT : 0;
		}
	}
	state->static_user_size = sizeof(struct xregs_state);
	if (state->avx)
		state->static_user_size = max(state->static_user_size, ymm_end);
	if (state->avx512)
		state->static_user_size = max(state->static_user_size,
					      hi16_zmm_end);
	if (state->pku)
		state->static_user_size = max(state->static_user_size,
					  leafd9->ebx + leafd9->eax);
	if (state->amx)
		state->static_user_size = max(state->static_user_size, tilecfg_end);
	state->dynamic_user_size = state->amx ? tiledata_end :
		state->static_user_size;
	xcr0 = kvm_protected_task_xcr0(state);
	leaf7_1_eax = kvm_protected_task_cpuid_class_mask(state,
							 &kvm_pt_cpuid_7_1_eax) |
		(state->lam ? KVM_PT_CPUID_7_1_EAX_LAM : 0);
	leaf7_1_eax &= leaf71 ? leaf71->eax : 0;
	leaf7_1_edx = kvm_protected_task_cpuid_class_mask(state,
							 &kvm_pt_cpuid_7_1_edx) |
		(state->avx10 ? KVM_PT_CPUID_7_1_EDX_AVX10 : 0);
	leaf7_1_edx &= leaf71 ? leaf71->edx : 0;
	leaf7_enabled = leaf7_1_eax || leaf7_1_edx ||
		(leaf7 &&
		 ((leaf7->ebx & kvm_protected_task_cpuid_class_mask(
					state, &kvm_pt_cpuid_7_0_ebx)) ||
		   (leaf7->ecx &
		    (kvm_protected_task_cpuid_class_mask(state,
					&kvm_pt_cpuid_7_0_ecx) |
		     (state->la57 ? KVM_PT_CPUID_7_ECX_LA57 : 0))) ||
		  (leaf7->edx & kvm_protected_task_cpuid_class_mask(
					state, &kvm_pt_cpuid_7_0_edx))));
	if (!apply)
		return;

	for (i = 0; i < vcpu->arch.cpuid_nent; i++) {
		struct kvm_cpuid_entry2 *entry = &vcpu->arch.cpuid_entries[i];

		switch (entry->function) {
		case 0:
			entry->eax = min(entry->eax,
					 state->avx10 ? 0x24U :
					 state->amx ? 0x1eU :
					 state->avx || state->pku || state->shstk ? 0xdU :
					 leaf7_enabled ? 7U : 1U);
			break;
		case 1:
			entry->ecx &= kvm_protected_task_cpuid_class_mask(
						state, &kvm_pt_cpuid_1_ecx) |
				(state->avx || state->pku || state->shstk || state->amx ?
				 KVM_PT_CPUID_1_ECX_XSAVE : 0);
			entry->edx &= kvm_protected_task_cpuid_class_mask(
						state, &kvm_pt_cpuid_1_edx);
			break;
		case 7:
			if (!entry->index) {
				entry->eax = leaf7_1_eax || leaf7_1_edx ? 1 : 0;
				entry->ebx &= kvm_protected_task_cpuid_class_mask(
						state, &kvm_pt_cpuid_7_0_ebx);
				entry->ecx &= kvm_protected_task_cpuid_class_mask(
						state, &kvm_pt_cpuid_7_0_ecx) |
					(state->la57 ? KVM_PT_CPUID_7_ECX_LA57 : 0) |
					(state->pku ? KVM_PT_CPUID_7_ECX_PKU : 0) |
					(state->shstk ? KVM_PT_CPUID_7_ECX_SHSTK : 0);
				entry->edx &= kvm_protected_task_cpuid_class_mask(
						state, &kvm_pt_cpuid_7_0_edx);
			} else if (entry->index == 1 &&
				   (leaf7_1_eax || leaf7_1_edx)) {
				entry->eax &= leaf7_1_eax;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx &= leaf7_1_edx;
			} else {
				entry->eax = 0;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx = 0;
			}
			break;
		case 0xd:
			if (!(state->avx || state->pku || state->shstk || state->amx)) {
				entry->eax = 0;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx = 0;
				break;
			}
			if (!entry->index) {
				entry->eax = xcr0;
				entry->edx = 0;
				entry->ecx = state->dynamic_user_size;
			} else if (entry->index == 1 &&
				   (state->shstk || state->amx)) {
				entry->eax &=
					(state->shstk ? KVM_PT_CPUID_D_1_EAX_XSAVES : 0) |
					(state->amx ? KVM_PT_CPUID_D_1_EAX_XGETBV1 |
						      KVM_PT_CPUID_D_1_EAX_XSAVES |
						      KVM_PT_CPUID_D_1_EAX_XFD : 0);
				entry->ecx &= state->shstk ? XFEATURE_MASK_CET_USER : 0;
				entry->edx = 0;
			} else if (entry->index == XFEATURE_YMM && state->avx) {
				entry->eax = sizeof(struct ymmh_struct);
				entry->ecx &= BIT(1);
				entry->edx = 0;
			} else if (entry->index == XFEATURE_OPMASK && state->avx512) {
				entry->eax = sizeof(struct avx_512_opmask_state);
				entry->ecx &= BIT(1);
				entry->edx = 0;
			} else if (entry->index == XFEATURE_ZMM_Hi256 &&
				   state->avx512) {
				entry->eax = sizeof(struct avx_512_zmm_uppers_state);
				entry->ecx &= BIT(1);
				entry->edx = 0;
			} else if (entry->index == XFEATURE_Hi16_ZMM &&
				   state->avx512) {
				entry->eax = sizeof(struct avx_512_hi16_state);
				entry->ecx &= BIT(1);
				entry->edx = 0;
			} else if (entry->index == XFEATURE_PKRU && state->pku) {
				entry->ecx = 0;
				entry->edx = 0;
			} else if (entry->index == XFEATURE_CET_USER &&
				   state->shstk) {
				entry->eax = sizeof(struct cet_user_state);
				entry->ecx = BIT(0);
				entry->edx = 0;
			} else if ((entry->index == XFEATURE_XTILE_CFG ||
				    entry->index == XFEATURE_XTILE_DATA) &&
				   state->amx) {
				entry->ecx &= BIT(1) | BIT(2);
				entry->edx = 0;
			} else {
				entry->eax = 0;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx = 0;
			}
			break;
		case 0x1d:
			if (!state->amx || entry->index > 1) {
				entry->eax = 0;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx = 0;
			} else if (!entry->index) {
				entry->eax = 1;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx = 0;
			} else {
				entry->eax = KVM_PT_AMX_TILE_BYTES |
					KVM_PT_AMX_BYTES_PER_TILE << 16;
				entry->ebx = KVM_PT_AMX_BYTES_PER_ROW |
					KVM_PT_AMX_NR_TILES << 16;
				entry->ecx = KVM_PT_AMX_NR_ROWS;
				entry->edx = 0;
			}
			break;
		case 0x1e:
			if (!state->amx) {
				entry->eax = 0;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx = 0;
			} else if (!entry->index) {
				entry->eax = state->amx_max_subleaf;
				entry->ebx = KVM_PT_AMX_TMUL_MAX_K |
					KVM_PT_AMX_TMUL_MAX_N << 8;
				entry->ecx = 0;
				entry->edx = 0;
			} else if (entry->index == 1 &&
				   state->amx_max_subleaf == 1) {
				entry->eax = state->amx_1e_1_eax;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx = 0;
			} else {
				entry->eax = 0;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx = 0;
			}
			break;
		case 0x24:
			if (!state->avx10) {
				entry->eax = 0;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx = 0;
			} else if (!entry->index) {
				entry->eax = state->avx10_max_subleaf;
				entry->ebx = KVM_PT_CPUID_24_0_EBX_WIDTHS |
					state->avx10_version;
				entry->ecx = 0;
				entry->edx = 0;
			} else if (entry->index == 1 &&
				   state->avx10_max_subleaf == 1) {
				entry->eax = 0;
				entry->ebx = 0;
				entry->ecx = state->avx10_1_ecx;
				entry->edx = 0;
			} else {
				entry->eax = 0;
				entry->ebx = 0;
				entry->ecx = 0;
				entry->edx = 0;
			}
			break;
		case 0x80000000:
			entry->eax = min(entry->eax, 0x80000008U);
			break;
		case 0x80000001:
			entry->ecx &= kvm_protected_task_cpuid_class_mask(
						state, &kvm_pt_cpuid_80000001_ecx);
			entry->edx &= kvm_protected_task_cpuid_class_mask(
						state, &kvm_pt_cpuid_80000001_edx);
			break;
		case 0x80000008:
			entry->eax = (entry->eax & ~GENMASK(15, 8)) |
				((state->la57 ? 57 : 48) << 8);
			entry->ebx &= kvm_protected_task_cpuid_class_mask(
						state, &kvm_pt_cpuid_80000008_ebx);
			entry->ecx = 0;
			entry->edx = 0;
			break;
		default:
			entry->eax = 0;
			entry->ebx = 0;
			entry->ecx = 0;
			entry->edx = 0;
			break;
		}
	}

	kvm_vcpu_after_set_cpuid(vcpu);
}

static bool kvm_protected_task_amx_permitted(void)
{
	return fpu_xstate_get_host_perm() & XFEATURE_MASK_XTILE_DATA;
}

static void kvm_protected_task_sync_dynamic_xstate(
		struct kvm_vcpu *vcpu, struct kvm_protected_task_x86 *state,
		bool enable)
{
	struct fpu_guest *guest_fpu = &vcpu->arch.guest_fpu;
	struct fpstate *fpstate = guest_fpu->fpstate;
	u64 user_xfeatures = kvm_protected_task_xcr0(state);
	u64 xfd = fpstate->xfd;
	bool amx_enabled;

	if (!state->amx)
		return;

	amx_enabled = enable || (fpstate->user_xfeatures &
				 XFEATURE_MASK_XTILE_DATA);
	if (amx_enabled)
		xfd &= ~XFEATURE_MASK_XTILE_DATA;
	else {
		user_xfeatures &= ~XFEATURE_MASK_XTILE_DATA;
		xfd |= XFEATURE_MASK_XTILE_DATA;
	}
	fpstate->user_xfeatures = user_xfeatures;
	fpstate->user_size = amx_enabled ? state->dynamic_user_size :
		state->static_user_size;
	fpu_update_guest_xfd(guest_fpu, xfd);
}

static void kvm_protected_task_restrict_xstate(
		struct kvm_vcpu *vcpu, struct kvm_protected_task_x86 *state)
{
	struct fpstate *fpstate = vcpu->arch.guest_fpu.fpstate;
	u64 user_xfeatures = kvm_protected_task_xcr0(state);

	if (state->amx)
		user_xfeatures &= ~XFEATURE_MASK_XTILE_DATA;
	fpstate->user_xfeatures = user_xfeatures;
	fpstate->user_size = state->static_user_size;

	kvm_protected_task_sync_dynamic_xstate(vcpu, state, false);
}

static int kvm_protected_task_validate_mm(struct mm_struct *mm,
					  struct kvm_protected_task_x86 *state)
{
	struct vm_area_struct *vma;
	VMA_ITERATOR(vmi, mm, 0);
	int ret = 0;

	mmap_read_lock(mm);
	for_each_vma(vmi, vma)
		if (vma->vm_end > state->va_limit ||
		    ((vma->vm_flags & VM_EXEC) &&
		     !(vma->vm_flags & VM_READ) && vma_pkey(vma) &&
		     !state->pku)) {
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
				       vm_flags_t vm_flags, int pkey, bool user)
{
	u64 flags = _PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_DIRTY;
	u64 *root, *p4d, *pud, *pmd, *pte, *entry;

	if (user)
		flags |= _PAGE_USER;
	if (vm_flags & VM_WRITE)
		flags |= _PAGE_RW;
	if (!(vm_flags & VM_EXEC))
		flags |= _PAGE_NX;
	flags |= ((u64)pkey << _PAGE_BIT_PKEY_BIT0) & _PAGE_PKEY_MASK;
	if (shift != PAGE_SHIFT)
		flags |= _PAGE_PSE;

	root = (u64 *)builder->image;
	entry = &root[(address >> (builder->la57 ? KVM_PT_PML5_SHIFT :
					      KVM_PT_PML4_SHIFT)) &
		      (PTRS_PER_PGD - 1)];
	if (builder->la57) {
		p4d = kvm_protected_task_next_table(builder, entry);
		if (IS_ERR(p4d))
			return PTR_ERR(p4d);
		entry = &p4d[(address >> KVM_PT_PML4_SHIFT) &
			     (MAX_PTRS_PER_P4D - 1)];
	}
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
					vm_flags_t vm_flags, int pkey)
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
						  vm_flags, pkey, true);
		if (ret)
			return ret;
		address += 1UL << shift;
	}

	return 0;
}

static int kvm_protected_task_map_visible_range(
		struct kvm_protected_task_builder *builder,
		unsigned long start, unsigned long end,
		unsigned long hidden_start, unsigned long hidden_end, int pkey,
		bool shadow_stack)
{
	vm_flags_t vm_flags = shadow_stack ? VM_READ :
		VM_READ | VM_WRITE | VM_EXEC;
	int ret;

	if (start < hidden_start) {
		ret = kvm_protected_task_map_range(builder, start,
						   min(end, hidden_start),
						   vm_flags,
						   pkey);
		if (ret || end <= hidden_start)
			return ret;
	}
	if (end > hidden_end)
		return kvm_protected_task_map_range(builder,
						    max(start, hidden_end), end,
						    vm_flags,
						    pkey);
	return 0;
}

static int kvm_protected_task_map_mm(struct kvm_protected_task_builder *builder,
				     struct mm_struct *mm,
				     unsigned long hidden_start,
				     unsigned long hidden_end)
{
	unsigned long address = 0;
	struct vm_area_struct *vma;
	VMA_ITERATOR(vmi, mm, 0);
	int ret = 0;

	mmap_read_lock(mm);
	for_each_vma(vmi, vma) {
		unsigned long end, start;
		int pkey;

		if (vma->vm_start >= builder->va_limit)
			break;
		end = min(vma->vm_start, builder->va_limit);
		if (address < end) {
			ret = kvm_protected_task_map_visible_range(builder,
					address, end, hidden_start, hidden_end, 0,
					false);
			if (ret)
				goto out;
		}

		start = max(address, vma->vm_start);
		end = min(vma->vm_end, builder->va_limit);
		pkey = vma->vm_flags & VM_KVM_PROTECTED ? 0 : vma_pkey(vma);
		ret = kvm_protected_task_map_visible_range(builder, start, end,
					hidden_start, hidden_end, pkey,
					vma->vm_flags & VM_SHADOW_STACK);
		if (ret)
			goto out;
		address = end;
	}
	if (address < builder->va_limit)
		ret = kvm_protected_task_map_visible_range(builder, address,
				builder->va_limit, hidden_start, hidden_end, 0, false);
out:
	mmap_read_unlock(mm);
	return ret;
}

static int kvm_protected_task_image_size(struct mm_struct *mm,
					 struct kvm_protected_task_x86 *state,
					 unsigned long *ret_size)
{
	unsigned long extra, pages;
	int map_count;

	mmap_read_lock(mm);
	map_count = mm->map_count;
	mmap_read_unlock(mm);

	if (check_add_overflow((unsigned long)map_count, 2UL, &extra) ||
	    check_mul_overflow(extra, 4UL, &extra) ||
	    check_add_overflow(extra, 2UL + KVM_PT_HOLE_TABLE_PAGES +
				(state->va_limit >> KVM_PT_PML4_SHIFT) +
				state->la57, &pages) ||
	    check_mul_overflow(pages, PAGE_SIZE, ret_size))
		return -EOVERFLOW;
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

void kvm_arch_protected_task_prepare_run(struct kvm_vcpu *vcpu)
{
	struct kvm_segment segment;

	if (vcpu->arch.protected_task_fast_return) {
		vcpu->arch.emulate_regs_need_sync_from_vcpu = true;
		vcpu->arch.emulate_regs_need_sync_to_vcpu = false;
		kvm_rax_write_raw(vcpu, vcpu->arch.protected_task_return_rax);
		kvm_rip_write(vcpu, vcpu->arch.protected_task_return_rip);
		kvm_set_rflags(vcpu, vcpu->arch.protected_task_return_rflags |
			       X86_EFLAGS_FIXED);
		vcpu->arch.exception.pending = false;
		vcpu->arch.exception_vmexit.pending = false;
		vcpu->arch.protected_task_fast_return = false;
	}

	kvm_protected_task_set_code_segment(&segment);
	kvm_set_segment(vcpu, &segment, VCPU_SREG_CS);
	kvm_protected_task_set_data_segment(&segment);
	kvm_set_segment(vcpu, &segment, VCPU_SREG_SS);
}

static int kvm_protected_task_setup_sregs(struct kvm_vcpu *vcpu,
					  struct kvm_protected_task_x86 *state)
{
	struct kvm_sregs sregs;
	unsigned long fsbase, gsbase;
	u64 cr3;
	u64 cr4 = X86_CR4_PAE | X86_CR4_OSFXSR | X86_CR4_OSXMMEXCPT;
	int ret;

	if (state->sregs_valid)
		return 0;

	fsbase = x86_fsbase_read_task(current);
	gsbase = x86_gsbase_read_task(current);
	cr3 = state->image->pgd |
		(state->lam ? mm_lam_cr3_mask(current->mm) : 0);
	if (test_thread_flag(TIF_NOTSC))
		cr4 |= X86_CR4_TSD;
	if (state->avx || state->pku || state->shstk || state->amx)
		cr4 |= X86_CR4_OSXSAVE;
	if (state->pku)
		cr4 |= X86_CR4_PKE;
	if (state->shstk)
		cr4 |= X86_CR4_CET;
	if (state->la57)
		cr4 |= X86_CR4_LA57;

	ret = kvm_arch_vcpu_ioctl_get_sregs(vcpu, &sregs);
	if (ret)
		return ret;

	kvm_protected_task_set_code_segment(&sregs.cs);
	kvm_protected_task_set_data_segment(&sregs.ds);
	kvm_protected_task_set_data_segment(&sregs.es);
	kvm_protected_task_set_data_segment(&sregs.fs);
	kvm_protected_task_set_data_segment(&sregs.gs);
	kvm_protected_task_set_data_segment(&sregs.ss);
	sregs.fs.base = fsbase;
	sregs.gs.base = gsbase;
	sregs.cr0 = X86_CR0_PE | X86_CR0_NE | X86_CR0_AM | X86_CR0_PG |
		(state->shstk ? X86_CR0_WP : 0);
	sregs.cr3 = cr3;
	sregs.cr4 = cr4;
	sregs.efer = EFER_SCE | EFER_LME | EFER_LMA | EFER_NX;

	ret = kvm_arch_vcpu_ioctl_set_sregs(vcpu, &sregs);
	if (ret)
		return ret;
	if ((state->avx || state->pku || state->shstk || state->amx) &&
	    __kvm_set_xcr(vcpu, XCR_XFEATURE_ENABLED_MASK,
			  kvm_protected_task_xcr0(state)))
		return -EINVAL;

	state->fsbase = fsbase;
	state->gsbase = gsbase;
	state->cr3 = cr3;
	state->cr4 = cr4;
	state->sregs_valid = true;
	return 0;
}

static int kvm_protected_task_setup_regs(struct kvm_vcpu *vcpu,
					 struct pt_regs *regs)
{
	struct kvm_regs *kvm_regs = &vcpu->run->s.regs.regs;

	*kvm_regs = (struct kvm_regs) {
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
	vcpu->arch.protected_task_fast_return = false;
	vcpu->run->kvm_dirty_regs |= KVM_SYNC_X86_REGS;
	vcpu->run->kvm_valid_regs |= KVM_SYNC_X86_REGS;
	return 0;
}

static int kvm_protected_task_setup_msrs(struct kvm_vcpu *vcpu,
					 struct kvm_protected_task_x86 *state)
{
	u64 star = (u64)__USER32_CS << 48 | (u64)__KERNEL_CS << 32;
	u64 mask = X86_EFLAGS_TF | X86_EFLAGS_DF | X86_EFLAGS_IF |
		   X86_EFLAGS_AC;
	u64 readback;
	int ret;

	vcpu_load(vcpu);
	ret = kvm_msr_write(vcpu, MSR_STAR, star);
	if (!ret)
		ret = kvm_msr_write(vcpu, MSR_LSTAR,
				    state->image->syscall_stub);
	if (!ret)
		ret = kvm_msr_write(vcpu, MSR_SYSCALL_MASK, mask);
	if (!ret)
		ret = kvm_msr_write(vcpu, MSR_KERNEL_GS_BASE, 0);
	if (!ret && state->shstk)
		ret = kvm_msr_write(vcpu, MSR_IA32_XSS,
				    XFEATURE_MASK_CET_USER);
	if (!ret && state->shstk)
		ret = kvm_msr_read(vcpu, MSR_IA32_XSS, &readback);
	if (!ret && state->shstk && readback != XFEATURE_MASK_CET_USER)
		ret = -EIO;
	vcpu_put(vcpu);

	return ret ? -EINVAL : 0;
}

static int kvm_protected_task_sync_regs(struct kvm_vcpu *vcpu,
					struct pt_regs *regs, bool syscall)
{
	struct kvm_regs *kvm_regs = &vcpu->run->s.regs.regs;

	regs->r15 = kvm_regs->r15;
	regs->r14 = kvm_regs->r14;
	regs->r13 = kvm_regs->r13;
	regs->r12 = kvm_regs->r12;
	regs->bp = kvm_regs->rbp;
	regs->bx = kvm_regs->rbx;
	regs->r11 = kvm_regs->r11;
	regs->r10 = kvm_regs->r10;
	regs->r9 = kvm_regs->r9;
	regs->r8 = kvm_regs->r8;
	regs->cx = kvm_regs->rcx;
	regs->dx = kvm_regs->rdx;
	regs->si = kvm_regs->rsi;
	regs->di = kvm_regs->rdi;
	regs->sp = kvm_regs->rsp;
	regs->cs = __USER_CS;
	regs->ss = __USER_DS;
	if (syscall) {
		regs->orig_ax = kvm_regs->rax;
		regs->ax = kvm_regs->rax;
		regs->ip = kvm_regs->rcx;
		regs->flags = kvm_regs->r11;
	} else {
		regs->orig_ax = -1;
		regs->ax = kvm_regs->rax;
		regs->ip = kvm_regs->rip;
		regs->flags = kvm_regs->rflags;
	}

	return 0;
}

static bool kvm_protected_task_syscall_uses_fpu(unsigned long nr,
						 unsigned long work)
{
	return nr == __NR_arch_prctl || nr == __NR_clone || nr == __NR_clone3 ||
	       nr == __NR_fork || nr == __NR_pkey_alloc ||
	       nr == __NR_rt_sigreturn || nr == __NR_vfork ||
	       work & (SYSCALL_WORK_SECCOMP |
		       SYSCALL_WORK_SYSCALL_TRACE |
		       SYSCALL_WORK_SYSCALL_EMU |
		       SYSCALL_WORK_SYSCALL_EXIT_TRAP);
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

static int kvm_protected_task_setup_task_msrs(
		struct kvm_vcpu *vcpu, struct kvm_protected_task_x86 *state)
{
	u64 misc_features, readback, spec_ctrl, virt_spec_ctrl = 0;
	int ret;

	if (state->task_msrs_valid)
		return 0;

	misc_features = test_thread_flag(TIF_NOCPUID) ?
		MSR_MISC_FEATURES_ENABLES_CPUID_FAULT : 0;
	spec_ctrl = spec_ctrl_current();
	if (static_cpu_has(X86_FEATURE_VIRT_SSBD))
		virt_spec_ctrl = test_thread_flag(TIF_SSBD) ? SPEC_CTRL_SSBD : 0;

	vcpu_load(vcpu);
	ret = kvm_msr_write(vcpu, MSR_PLATFORM_INFO,
			    MSR_PLATFORM_INFO_CPUID_FAULT);
	if (!ret)
		ret = kvm_msr_write(vcpu, MSR_MISC_FEATURES_ENABLES,
				    misc_features);
	if (!ret)
		ret = kvm_msr_read(vcpu, MSR_MISC_FEATURES_ENABLES, &readback);
	if (!ret && readback != misc_features)
		ret = -EIO;
	if (!ret)
		ret = kvm_msr_write(vcpu, MSR_IA32_SPEC_CTRL, spec_ctrl);
	if (!ret)
		ret = kvm_msr_read(vcpu, MSR_IA32_SPEC_CTRL, &readback);
	if (!ret && readback != spec_ctrl)
		ret = -EIO;
	if (static_cpu_has(X86_FEATURE_VIRT_SSBD)) {
		if (!ret)
			ret = kvm_msr_write(vcpu, MSR_AMD64_VIRT_SPEC_CTRL,
					    virt_spec_ctrl);
		if (!ret)
			ret = kvm_msr_read(vcpu, MSR_AMD64_VIRT_SPEC_CTRL,
					   &readback);
		if (!ret && readback != virt_spec_ctrl)
			ret = -EIO;
	}
	vcpu_put(vcpu);
	if (ret)
		return ret;

	state->misc_features = misc_features;
	state->spec_ctrl = spec_ctrl;
	state->virt_spec_ctrl = virt_spec_ctrl;
	state->task_msrs_valid = true;
	return 0;
}

static int kvm_protected_task_setup_debugregs(
		struct kvm_vcpu *vcpu, struct kvm_protected_task_x86 *state)
{
	unsigned long db[HBP_NUM], dr7;
	int i, ret = 0;

	if (state->debugregs_valid)
		return 0;

	x86_ptrace_get_hw_breakpoints(current, db, &dr7);
	vcpu_load(vcpu);
	for (i = 0; i < HBP_NUM && !ret; i++)
		ret = kvm_set_dr(vcpu, i, db[i]);
	if (!ret)
		ret = kvm_set_dr(vcpu, 7, dr7);
	vcpu_put(vcpu);
	if (ret)
		return ret;

	memcpy(state->debugreg, db, sizeof(db));
	state->dr7 = dr7;
	state->debugregs_valid = true;
	return 0;
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

static void kvm_protected_task_setup_pkru(struct kvm_vcpu *vcpu,
					  struct kvm_protected_task_x86 *state)
{
	u32 pkru;

	if (!state->pku)
		return;
	pkru = read_pkru();
	current->thread.pkru = pkru;
	vcpu->arch.pkru = pkru;
}

static void kvm_protected_task_sync_pkru(struct kvm_vcpu *vcpu,
					 struct kvm_protected_task_x86 *state)
{
	if (!state->pku)
		return;
	current->thread.pkru = vcpu->arch.pkru;
	write_pkru(vcpu->arch.pkru);
}

static bool kvm_protected_task_vma_allows_fault(struct vm_area_struct *vma,
						 u32 error_code)
{
	if (error_code & X86_PF_INSTR)
		return vma->vm_flags & VM_EXEC;
	if (error_code & X86_PF_WRITE)
		return vma->vm_flags & VM_WRITE;
	return vma->vm_flags & (VM_READ | VM_WRITE);
}

static int kvm_protected_task_handle_memory_fault(struct kvm_vcpu *vcpu,
						  struct pt_regs *regs,
						  u32 *next_slot)
{
	unsigned long address = vcpu->run->memory_fault.gpa & PAGE_MASK;
	struct vm_area_struct *vma;
	unsigned long end = 0;
	bool access_allowed = false, mapped = false, visible;
	int idx, ret;

	ret = kvm_protected_task_sync_regs(vcpu, regs, false);
	if (ret)
		return ret;

	idx = srcu_read_lock(&vcpu->kvm->srcu);
	visible = kvm_vcpu_is_visible_gfn(vcpu, address >> PAGE_SHIFT);
	srcu_read_unlock(&vcpu->kvm->srcu, idx);
	if (vcpu->arch.protected_task_backing_fault) {
		x86_handle_user_page_fault(regs,
			vcpu->arch.protected_task_pf_error_code | X86_PF_USER,
			address);
		return 0;
	}

	vma = lock_mm_and_find_vma(current->mm, address, regs);
	if (vma) {
		mapped = true;
		end = vma->vm_end;
		access_allowed = kvm_protected_task_vma_allows_fault(vma,
				vcpu->arch.protected_task_pf_error_code);
		mmap_read_unlock(current->mm);
	}

	if (visible && !vcpu->arch.protected_task_growdown_fault &&
	    access_allowed)
		return 0;

	if (!mapped || (visible &&
			!vcpu->arch.protected_task_growdown_fault)) {
		u32 error_code = X86_PF_USER | (mapped ? X86_PF_PROT : 0);

		x86_force_sig_user_page_fault(regs, error_code, address,
					      mapped ? SEGV_ACCERR : SEGV_MAPERR);
		return 0;
	}
	if (visible)
		return 0;

	ret = kvm_map_user_memory_region(vcpu->kvm, next_slot,
					 address, end);
	return ret == -EEXIST ? 0 : ret;
}

static int kvm_protected_task_handle_exception(struct kvm_vcpu *vcpu,
					       struct kvm_protected_task_x86 *state,
					       struct pt_regs *regs)
{
	unsigned long error_code = 0, dr6 = 0;
	unsigned int trapnr;
	int ret;

	if (vcpu->run->exit_reason == KVM_EXIT_DEBUG) {
		trapnr = vcpu->run->debug.arch.exception;
		if (trapnr == X86_TRAP_DB)
			dr6 = vcpu->run->debug.arch.dr6 ^ DR6_ACTIVE_LOW;
	} else {
		trapnr = vcpu->run->ex.exception;
		error_code = vcpu->run->ex.error_code;
	}

	ret = kvm_protected_task_sync_regs(vcpu, regs, false);
	if (ret)
		return ret;
	ret = kvm_protected_task_activate_fpu(vcpu, state);
	if (ret)
		return ret;
	if (trapnr == X86_TRAP_NM) {
		u64 xfd_err = vcpu->arch.guest_fpu.xfd_err;

		vcpu->arch.guest_fpu.xfd_err = 0;
		if (!state->amx ||
		    (xfd_err & XFEATURE_MASK_XTILE_DATA) != XFEATURE_MASK_XTILE_DATA)
			return -EIO;
		if (kvm_protected_task_amx_permitted()) {
			kvm_protected_task_sync_dynamic_xstate(vcpu, state, true);
			return 0;
		}
	}

	if (trapnr == X86_TRAP_PF) {
		unsigned long address = vcpu->arch.cr2;
		struct vm_area_struct *vma;
		u32 pkey = 0;
		bool mapped;

		mmap_read_lock(current->mm);
		vma = vma_lookup(current->mm, address);
		mapped = !!vma;
		if (vma)
			pkey = vma_pkey(vma);
		mmap_read_unlock(current->mm);

		error_code |= X86_PF_USER;
		if (mapped)
			error_code |= X86_PF_PROT;
		else
			error_code &= ~X86_PF_PROT;
		if (mapped && (error_code & X86_PF_PK))
			x86_force_sig_user_pkey_fault(regs, error_code,
						      address, pkey);
		else
			x86_force_sig_user_page_fault(regs, error_code, address,
						      mapped ? SEGV_ACCERR : SEGV_MAPERR);
		return 0;
	}

	return x86_handle_user_exception(regs, trapnr, error_code, dr6) ?
		0 : -EIO;
}

static bool kvm_protected_task_image_matches(
		const struct kvm_protected_task_x86_image *image,
		const struct kvm_protected_task_x86 *state, u64 pgtable_gen)
{
	return image->pgtable_gen == pgtable_gen &&
		image->va_limit == state->va_limit &&
		image->la57 == state->la57 && image->pku == state->pku &&
		image->shstk == state->shstk;
}

static int kvm_protected_task_build_image(
		struct kvm_vcpu *vcpu, struct kvm_protected_task_x86 *state,
		u64 pgtable_gen,
		struct kvm_protected_task_x86_image **imagep)
{
	struct kvm_protected_task_x86_image *image;
	struct kvm_protected_task_builder builder;
	unsigned long address, pgd, size, stub_gpa;
	u64 *stub_page;
	int ret;

	image = kzalloc_obj(*image);
	if (!image)
		return -ENOMEM;

	ret = kvm_protected_task_validate_mm(current->mm, state);
	if (ret)
		goto free_image_state;
	ret = kvm_protected_task_image_size(current->mm, state, &size);
	if (ret)
		goto free_image_state;

	address = vm_mmap_protected_task(size);
	if (IS_ERR_VALUE(address)) {
		ret = address;
		goto free_image_state;
	}
	image->mm = current->mm;
	image->address = address;
	image->size = size;
	if (size > state->va_limit ||
	    image->address > state->va_limit - size) {
		ret = -EOPNOTSUPP;
		goto unmap_image;
	}

	builder.image = vzalloc(size);
	if (!builder.image) {
		ret = -ENOMEM;
		goto unmap_image;
	}
	builder.base = image->address;
	builder.size = size;
	builder.used = 0;
	builder.va_limit = state->va_limit;
	builder.la57 = state->la57;
	if (!kvm_protected_task_alloc_table(&builder, &pgd)) {
		ret = -ENOSPC;
		goto free_builder;
	}

	ret = kvm_protected_task_map_mm(&builder, current->mm,
					image->address,
					image->address + size);
	if (ret)
		goto free_builder;
	stub_page = kvm_protected_task_alloc_table(&builder, &stub_gpa);
	if (!stub_page) {
		ret = -ENOSPC;
		goto free_builder;
	}
	kvm_x86_call(patch_hypercall)(vcpu, (u8 *)stub_page);
	ret = kvm_protected_task_map_page(&builder, stub_gpa, stub_gpa,
					  PAGE_SHIFT, VM_READ | VM_EXEC, 0, false);
	if (ret)
		goto free_builder;
	if (copy_to_user((void __user *)image->address, builder.image,
			 builder.used)) {
		ret = -EFAULT;
		goto free_builder;
	}
	image->used = builder.used;
	image->pgd = pgd;
	image->syscall_stub = stub_gpa;
	if (builder.used < size) {
		ret = vm_unlock_protected_task_tail(image->address, size,
						    builder.used);
		if (ret)
			goto free_builder;
	}

	/* Only the protected memslot may force-access the hidden image. */
	ret = vm_mprotect(image->address, size, PROT_NONE);
	if (ret)
		goto free_builder;
	ret = do_mseal(image->address, size, 0);
	if (ret)
		goto free_builder;

	image->pgtable_gen = pgtable_gen;
	image->va_limit = state->va_limit;
	image->la57 = state->la57;
	image->pku = state->pku;
	image->shstk = state->shstk;
	refcount_set(&image->refs, 1);
	*imagep = image;
	ret = 0;

free_builder:
	vfree(builder.image);
	if (!ret)
		return 0;
unmap_image:
	WARN_ON_ONCE(vm_munmap_protected_task(image->address, image->size));
free_image_state:
	kfree(image);
	return ret;
}

static int kvm_protected_task_put_image(struct kvm_protected_task_x86 *state)
{
	struct kvm_protected_task_x86_image *image = state->image;
	struct mm_struct *mm;
	int ret = 0;

	if (!image)
		return 0;
	mm = image->mm;
	if (WARN_ON_ONCE(current->mm != mm))
		return -EIO;

	state->image = NULL;
	mutex_lock(&mm->protected_task_image_lock);
	if (refcount_dec_and_test(&image->refs)) {
		if (mm->protected_task_arch_image == image)
			mm->protected_task_arch_image = NULL;
		mutex_unlock(&mm->protected_task_image_lock);
		ret = vm_munmap_protected_task(image->address, image->size);
		kfree(image);
		return ret;
	}
	mutex_unlock(&mm->protected_task_image_lock);
	return 0;
}

static int kvm_protected_task_get_image(struct kvm_vcpu *vcpu,
					struct kvm_protected_task_x86 *state,
					u32 slot, u64 *pgtable_gen)
{
	struct mm_struct *mm = vcpu->kvm->mm;
	struct kvm_protected_task_x86_image *image;
	struct kvm_userspace_memory_region2 region = {
		.slot = slot,
		.flags = KVM_MEM_READONLY,
	};
	u64 generation;
	int ret = 0;

	if (WARN_ON_ONCE(current->mm != mm))
		return -EIO;
	generation = atomic64_read(&mm->protected_task_pgtable_gen);
	mutex_lock(&mm->protected_task_image_lock);
	image = mm->protected_task_arch_image;
	if (image && image->pgtable_gen == generation) {
		if (!kvm_protected_task_image_matches(image, state, generation))
			ret = -EIO;
		else
			refcount_inc(&image->refs);
	} else {
		ret = kvm_protected_task_build_image(vcpu, state, generation,
						     &image);
		if (!ret)
			mm->protected_task_arch_image = image;
	}
	mutex_unlock(&mm->protected_task_image_lock);
	if (ret)
		return ret;

	state->image = image;
	region.guest_phys_addr = image->address;
	region.userspace_addr = image->address;
	region.memory_size = image->used;
	ret = kvm_set_protected_task_memory_region(vcpu->kvm, &region);
	if (ret) {
		WARN_ON_ONCE(kvm_protected_task_put_image(state));
		return ret;
	}
	*pgtable_gen = image->pgtable_gen;
	return 0;
}

int kvm_arch_protected_task_prepare(struct kvm_vcpu *vcpu, bool reuse,
				    void **statep)
{
	struct kvm_protected_task_x86 *state;
	int ret;

	if (test_thread_flag(TIF_IO_BITMAP) || current->thread.iopl_emul)
		return -EOPNOTSUPP;

	state = kzalloc_obj(*state);
	if (!state)
		return -ENOMEM;

	if (reuse) {
		vcpu_load(vcpu);
		kvm_vcpu_reset(vcpu, true);
		kvm_set_mp_state(vcpu, KVM_MP_STATE_RUNNABLE);
		vcpu_put(vcpu);
	} else {
		ret = kvm_vcpu_set_supported_cpuid(vcpu);
		if (ret) {
			kfree(state);
			return ret;
		}
	}
	ret = kvm_apic_set_base(vcpu, 0, true);
	if (ret) {
		kfree(state);
		return -EINVAL;
	}
	kvm_protected_task_restrict_cpuid(vcpu, state, !reuse);
	kvm_protected_task_restrict_xstate(vcpu, state);
	vcpu->arch.guest_fpu.xfd_err = 0;

	*statep = state;
	return 0;
}

int kvm_arch_protected_task_prepare_permissions(void)
{
	if (!cpu_feature_enabled(X86_FEATURE_AMX_TILE) ||
	    !kvm_cpu_cap_has(X86_FEATURE_AMX_TILE))
		return 0;

	return fpu_xstate_prctl(ARCH_REQ_XCOMP_GUEST_PERM,
				XFEATURE_XTILE_DATA);
}

u64 kvm_arch_protected_task_features(void)
{
	return KVM_PROTECTED_TASK_FEATURE_EXEC;
}

u64 kvm_arch_protected_task_xfeatures(struct kvm_vcpu *vcpu, void *arch_state)
{
	struct kvm_protected_task_x86 *state = arch_state;

	return kvm_protected_task_profile_xfeatures(state);
}

unsigned int kvm_arch_protected_task_max_tag_bits(struct kvm_vcpu *vcpu,
						  void *arch_state)
{
	struct kvm_protected_task_x86 *state = arch_state;

	return state->lam ? KVM_PT_LAM_U57_BITS : 0;
}

bool kvm_arch_protected_task_needs_pgtable_update(struct kvm_vcpu *vcpu,
						   void *arch_state)
{
	struct kvm_protected_task_x86 *state = arch_state;

	return state->pku || state->shstk;
}

unsigned long kvm_arch_protected_task_elf_hwcap(struct kvm_vcpu *vcpu,
						unsigned int type,
						unsigned long value)
{
	struct kvm_cpuid_entry2 *entry;

	if (type == AT_HWCAP) {
		entry = kvm_find_cpuid_entry(vcpu, 1);
		return entry ? entry->edx : 0;
	}
	if (type == AT_HWCAP2)
		return 0;

	return value;
}

int kvm_arch_protected_task_finalize(struct kvm_vcpu *vcpu, void *arch_state,
				     struct kvm_vcpu *source_vcpu,
				     void *source_arch_state,
				     struct pt_regs *regs, u32 slot,
				     u64 *pgtable_gen)
{
	struct kvm_protected_task_x86 *state = arch_state;
	struct kvm_protected_task_x86 *source = source_arch_state;
	int ret;

	/* Only x86-64 is supported, reject x32/i*86 */
	if (test_thread_flag(TIF_ADDR32))
		return -EOPNOTSUPP;
	if (WARN_ON_ONCE(current->mm != vcpu->kvm->mm))
		return -EIO;

	ret = kvm_protected_task_get_image(vcpu, state, slot, pgtable_gen);
	if (ret)
		return ret;
	ret = kvm_protected_task_setup_sregs(vcpu, state);
	if (ret)
		return ret;
	ret = kvm_protected_task_setup_msrs(vcpu, state);
	if (ret)
		return ret;
	ret = kvm_protected_task_setup_task_msrs(vcpu, state);
	if (ret)
		return ret;
	ret = kvm_protected_task_setup_debugregs(vcpu, state);
	if (ret)
		return ret;
	if (source_vcpu) {
		kvm_protected_task_sync_dynamic_xstate(vcpu, state,
				source_vcpu->arch.guest_fpu.fpstate->user_xfeatures &
				XFEATURE_MASK_XTILE_DATA);
		if (source->fpu_active)
			kvm_protected_task_setup_pkru(source_vcpu, source);
		ret = kvm_protected_task_deactivate_fpu(source_vcpu, source);
		if (!ret)
			ret = fpu_copy_guest_fpstate_to_guest(
				&vcpu->arch.guest_fpu,
				&source_vcpu->arch.guest_fpu,
				kvm_protected_task_profile_xfeatures(state),
				source_vcpu->arch.pkru, &vcpu->arch.pkru);
	} else {
		ret = fpu_copy_task_fpstate_to_guest(&vcpu->arch.guest_fpu,
				kvm_protected_task_xcr0(state),
				&vcpu->arch.pkru);
		if (!ret && state->shstk)
			ret = fpu_copy_task_supervisor_state_to_guest(
					&vcpu->arch.guest_fpu,
					XFEATURE_MASK_CET_USER);
	}
	if (ret)
		return ret;
	kvm_protected_task_sync_dynamic_xstate(vcpu, state, false);
	return kvm_protected_task_setup_regs(vcpu, regs);
}

int kvm_arch_protected_task_prepare_user_work(struct kvm_vcpu *vcpu,
					      void *arch_state)
{
	struct kvm_protected_task_x86 *state = arch_state;

	state->fast_syscall_return = false;
	state->sregs_valid = false;
	state->task_msrs_valid = false;
	state->debugregs_valid = false;
	return kvm_protected_task_activate_fpu(vcpu, state);
}

int kvm_arch_protected_task_run(struct kvm_vcpu *vcpu, void *arch_state,
				struct pt_regs *regs, u32 *next_slot)
{
	struct kvm_protected_task_x86 *state = arch_state;
	bool run_complete = false;
	int ret;

	ret = kvm_protected_task_deactivate_fpu(vcpu, state);
	if (ret)
		goto release;
	kvm_protected_task_sync_dynamic_xstate(vcpu, state, false);

	ret = kvm_protected_task_setup_sregs(vcpu, state);
	if (ret)
		goto out;
	ret = kvm_protected_task_setup_task_msrs(vcpu, state);
	if (ret)
		goto out;
	ret = kvm_protected_task_setup_debugregs(vcpu, state);
	if (ret)
		goto out;
	if (state->fast_syscall_return &&
	    !(vcpu->guest_debug & KVM_GUESTDBG_SINGLESTEP)) {
		state->fast_syscall_return = false;
		vcpu->arch.protected_task_return_rax = regs->ax;
		vcpu->arch.protected_task_return_rip = regs->ip;
		vcpu->arch.protected_task_return_rflags = regs->flags;
		vcpu->arch.protected_task_fast_return = true;
	} else {
		ret = kvm_protected_task_setup_regs(vcpu, regs);
		if (ret)
			goto out;
	}
	kvm_protected_task_setup_pkru(vcpu, state);

	vcpu->arch.protected_task_backing_fault = false;
	vcpu->arch.protected_task_growdown_fault = false;
	ret = kvm_protected_task_vcpu_run(vcpu);
	kvm_protected_task_vcpu_run_complete(vcpu);
	run_complete = true;
	kvm_protected_task_sync_pkru(vcpu, state);
	if (ret == -EINTR &&
	    vcpu->run->s.regs.regs.rip == state->image->syscall_stub) {
		int cpl;

		vcpu_load(vcpu);
		cpl = kvm_x86_call(get_cpl)(vcpu);
		vcpu_put(vcpu);
		if (!cpl) {
			vcpu->run->exit_reason = KVM_EXIT_HYPERCALL;
			ret = 0;
		}
	}
	if (ret == -EINTR) {
		ret = kvm_protected_task_sync_regs(vcpu, regs, false);
		goto out;
	}
	if (ret && (ret != -EFAULT ||
		    vcpu->run->exit_reason != KVM_EXIT_MEMORY_FAULT))
		goto out;

	switch (vcpu->run->exit_reason) {
	case KVM_EXIT_HYPERCALL:
	{
		unsigned long modifying_work, nr, work;

		ret = kvm_protected_task_sync_regs(vcpu, regs, false);
		if (ret)
			break;
		if (regs->ip != state->image->syscall_stub) {
			ret = -EIO;
			break;
		}
		nr = regs->ax;
		work = READ_ONCE(current_thread_info()->syscall_work);
		regs->orig_ax = nr;
		regs->ip = regs->cx;
		regs->flags = regs->r11;
		if (kvm_protected_task_syscall_uses_fpu(nr, work)) {
			ret = kvm_protected_task_activate_fpu(vcpu, state);
			if (ret)
				break;
		}
		do_protected_syscall_64(regs);
		modifying_work = work &
			(SYSCALL_WORK_SECCOMP |
			 SYSCALL_WORK_SYSCALL_TRACE |
			 SYSCALL_WORK_SYSCALL_EMU |
			 SYSCALL_WORK_SYSCALL_USER_DISPATCH |
			 SYSCALL_WORK_SYSCALL_EXIT_TRAP);
		state->fast_syscall_return = nr != __NR_rt_sigreturn &&
			!modifying_work;
		if (nr == __NR_arch_prctl || nr == __NR_prctl ||
		    nr == __NR_seccomp || nr == __NR_rt_sigreturn ||
		    modifying_work) {
			state->sregs_valid = false;
			state->task_msrs_valid = false;
			state->debugregs_valid = false;
		}
		ret = 0;
		break;
	}
	case KVM_EXIT_MEMORY_FAULT:
		ret = kvm_protected_task_handle_memory_fault(vcpu, regs,
						     next_slot);
		break;
	case KVM_EXIT_EXCEPTION:
	case KVM_EXIT_DEBUG:
		ret = kvm_protected_task_handle_exception(vcpu, state, regs);
		break;
	default:
		ret = -EIO;
		break;
	}

out:
release:
	if (!run_complete)
		kvm_protected_task_vcpu_run_complete(vcpu);
	return ret;
}

void kvm_arch_protected_task_cleanup(struct kvm_vcpu *vcpu, void *arch_state)
{
	struct kvm_protected_task_x86 *state = arch_state;

	if (!state)
		return;
	WARN_ON_ONCE(kvm_protected_task_deactivate_fpu(vcpu, state));
	if (state->image && current->mm == vcpu->kvm->mm)
		WARN_ON_ONCE(kvm_arch_protected_task_deactivate(vcpu, state));
	kfree(state);
}

int kvm_arch_protected_task_deactivate(struct kvm_vcpu *vcpu, void *arch_state)
{
	struct kvm_protected_task_x86 *state = arch_state;
	int ret;

	if (WARN_ON_ONCE(current->mm != vcpu->kvm->mm))
		return -EIO;

	ret = kvm_protected_task_put_image(state);
	return ret;
}

#else

int kvm_arch_protected_task_prepare(struct kvm_vcpu *vcpu, bool reuse,
				    void **state)
{
	return -EOPNOTSUPP;
}

int kvm_arch_protected_task_prepare_permissions(void)
{
	return 0;
}

u64 kvm_arch_protected_task_features(void)
{
	return 0;
}

u64 kvm_arch_protected_task_xfeatures(struct kvm_vcpu *vcpu, void *state)
{
	return 0;
}

unsigned int kvm_arch_protected_task_max_tag_bits(struct kvm_vcpu *vcpu,
						  void *state)
{
	return 0;
}

bool kvm_arch_protected_task_needs_pgtable_update(struct kvm_vcpu *vcpu,
						   void *state)
{
	return false;
}

void kvm_arch_protected_task_prepare_run(struct kvm_vcpu *vcpu)
{
}

int kvm_arch_protected_task_finalize(struct kvm_vcpu *vcpu, void *state,
				     struct kvm_vcpu *source_vcpu,
				     void *source_state,
				     struct pt_regs *regs, u32 slot,
				     u64 *pgtable_gen)
{
	return -EOPNOTSUPP;
}

int kvm_arch_protected_task_prepare_user_work(struct kvm_vcpu *vcpu,
					      void *state)
{
	return 0;
}

int kvm_arch_protected_task_run(struct kvm_vcpu *vcpu, void *state,
				struct pt_regs *regs, u32 *next_slot)
{
	return -EOPNOTSUPP;
}

void kvm_arch_protected_task_cleanup(struct kvm_vcpu *vcpu, void *state)
{
}

int kvm_arch_protected_task_deactivate(struct kvm_vcpu *vcpu, void *state)
{
	return -EOPNOTSUPP;
}

#endif
