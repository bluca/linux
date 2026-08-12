/* SPDX-License-Identifier: GPL-2.0-only */

.set UCONTEXT_FPSTATE, 224
.set FPSTATE_FCW, 0
.set FPSTATE_MXCSR, 24
.set FPSTATE_XMM15, 400
.set FPSTATE_SW_MAGIC1, 464
.set FPSTATE_SW_EXTENDED_SIZE, 468
.set FPSTATE_SW_XFEATURES, 472
.set FPSTATE_SW_XSTATE_SIZE, 480
.set FPSTATE_XFEATURES, 512
.set FPSTATE_MAGIC2, 576
.set FP_XSTATE_MAGIC1, 0x46505853
.set FP_XSTATE_MAGIC2, 0x46505845
.set FP_XSTATE_SIZE, 576
.set FP_XSTATE_EXTENDED_SIZE, 580
.set XFEATURE_MASK_FPSSE, 3
.set XFEATURE_MASK_PKU, 0x203
.set ARCH_SET_GS, 0x1001
.set ARCH_SET_FS, 0x1002
.set ARCH_GET_FS, 0x1003
.set ARCH_GET_GS, 0x1004
.set ARCH_SET_CPUID, 0x1012
.set ARCH_GET_XCOMP_SUPP, 0x1021
.set ARCH_GET_XCOMP_PERM, 0x1022
.set ARCH_REQ_XCOMP_PERM, 0x1023
.set ARCH_GET_UNTAG_MASK, 0x4001
.set ARCH_ENABLE_TAGGED_ADDR, 0x4002
.set ARCH_GET_MAX_TAG_BITS, 0x4003
.set ARCH_FORCE_TAGGED_SVA, 0x4004
.set ARCH_SHSTK_ENABLE, 0x5001
.set ARCH_SHSTK_LOCK, 0x5003
.set ARCH_SHSTK_STATUS, 0x5005
.set ARCH_SHSTK_SHSTK, 1
.set PR_SET_TSC, 26
.set PR_TSC_ENABLE, 1
.set PR_TSC_SIGSEGV, 2
.set PR_GET_SPECULATION_CTRL, 52
.set PR_SET_SPECULATION_CTRL, 53
.set PR_SPEC_STORE_BYPASS, 0
.set PR_SPEC_INDIRECT_BRANCH, 1
.set PR_SPEC_PRCTL, 1
.set PR_SPEC_ENABLE, 2
.set PR_SPEC_DISABLE, 4
.set PR_SPEC_FORCE_DISABLE, 8
.set PR_SPEC_DISABLE_NOEXEC, 16
.set SIGINFO_CODE, 8
.set SIGINFO_ADDR, 16
.set SIGINFO_PKEY, 32
.set UCONTEXT_RIP, 168
.set UCONTEXT_EFL, 176
.set UCONTEXT_ERR, 192
.set UCONTEXT_TRAPNO, 200
.set UCONTEXT_CR2, 216
.set SA_RESTORER_SIGINFO, 0x04000004
.set AT_HWCAP, 16
.set AT_HWCAP2, 26
.set ELF_HWCAP, 0x7808101
.set ADDRESS_FD, 100
.set READY_FD, 101
.set CONTROL_FD, 102

	.section .rodata
message:
	.ascii "protected task exec\n"
.set message_length, . - message

	.balign 16
initial_xmm:
	.quad 0x5566778811223344, 0xddeeff0099aabbcc
replacement_xmm:
	.quad 0x76543210fedcba98, 0xdeadbeef0badcafe
initial_mxcsr:
	.long 0x3f80
initial_fcw:
	.word 0x077f
replacement_mxcsr:
	.long 0x5f80
replacement_fcw:
	.word 0x037f
simd_exception_mxcsr:
	.long 0x1d80
x87_exception_fcw:
	.word 0x037b
one_float:
	.long 0x3f800000
zero_float:
	.long 0
security_memfd_name:
	.asciz "protected-task-security"
syscall_initial_byte:
	.byte 0x3c
syscall_write_byte:
	.byte 0xa5
	.balign 8
fs_marker:
	.quad 0x0123456789abcdef
gs_marker:
	.quad 0xfedcba9876543210
	.balign 8
lam_marker:
	.quad 0x13579bdf2468ace0
	.balign 4
ldt_entry:
	.long 0, 0, 0xfffff, 0x51

	.balign 8
clone3_args:
	.zero 32
	.quad 17
	.zero 48
.set clone3_args_size, . - clone3_args

	.balign 8
signal_action:
	.quad signal_handler
	.quad SA_RESTORER_SIGINFO
	.quad signal_restorer
	.quad 0

	.balign 8
exception_action:
	.quad exception_handler
	.quad SA_RESTORER_SIGINFO
	.quad signal_restorer
	.quad 0

	.balign 8
default_action:
	.zero 32

	.data
	.balign 4
signal_seen:
	.long 0
	.balign 4
exception_seen:
	.long 0
	.balign 4
expected_signal:
	.long 0
	.balign 4
expected_code:
	.long 0
	.balign 8
expected_trap:
	.quad 0
expected_error_mask:
	.quad 0
expected_error:
	.quad 0
expected_address:
	.quad 0
expected_cr2:
	.quad 0
	.balign 4
expected_pkey:
	.long 0
	.balign 8
exception_resume:
	.quad 0
	.balign 4
failure_code:
	.long 1
	.balign 8
initial_stack:
	.quad 0
	.balign 4
thread_tid:
	.long 0
	.balign 4
thread_result:
	.long 0
	.balign 4
thread_ready:
	.long 0
	.balign 4
thread_release:
	.long 0
	.balign 4
pku_available:
	.long 0
	.balign 4
lam_available:
	.long 0
	.balign 4
pkey_number:
	.long 0
	.balign 4
pkey_allowed_pkru:
	.long 0
pkey_denied_pkru:
	.long 0
	.balign 8
pkey_address:
	.quad 0
	.balign 4
expected_xstate_size:
	.long FP_XSTATE_SIZE
expected_xstate_extended_size:
	.long FP_XSTATE_EXTENDED_SIZE
expected_pkru_offset:
	.long 0
	.balign 8
expected_xfeatures:
	.quad XFEATURE_MASK_FPSSE
	.balign 8
reexec_argv:
	.quad 0
	.quad reexec_marker
	.quad 0
reexec_marker:
	.asciz "reexec"

	.text
	.globl _start
	.type _start, @function
_start:
	mov %rsp, initial_stack(%rip)
	movl $80, failure_code(%rip)
	mov $4, %edi
	call install_exception_handler
	test %eax, %eax
	jne 1f
	mov $11, %edi
	call install_exception_handler
	test %eax, %eax
	jne 1f

	movl $73, failure_code(%rip)
	mov initial_stack(%rip), %rdi
	call check_cpu_profile
	test %eax, %eax
	je .Lcpu_profile_ok
	mov %eax, failure_code(%rip)
	jmp 1f
.Lcpu_profile_ok:
	movl $108, failure_code(%rip)
	call check_initial_pkru
	test %eax, %eax
	jne 1f

	movl $89, failure_code(%rip)
	mov $157, %eax
	mov $PR_SET_TSC, %edi
	mov $PR_TSC_SIGSEGV, %esi
	syscall
	test %rax, %rax
	jne 1f
	mov $11, %edi
	mov $128, %esi
	mov $13, %edx
	xor %ecx, %ecx
	lea .Lrdtsc_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
.Lrdtsc:
	rdtsc
.Lrdtsc_resume:
	call check_exception
	test %eax, %eax
	jne 1f
	mov $157, %eax
	mov $PR_SET_TSC, %edi
	mov $PR_TSC_ENABLE, %esi
	syscall
	test %rax, %rax
	jne 1f
	rdtsc

	movl $90, failure_code(%rip)
	mov $158, %eax
	mov $ARCH_SET_CPUID, %edi
	xor %esi, %esi
	syscall
	cmp $-19, %rax
	je .Lcpuid_fault_unavailable
	test %rax, %rax
	jne 1f
	movl $91, failure_code(%rip)
	mov $11, %edi
	mov $128, %esi
	mov $13, %edx
	xor %ecx, %ecx
	lea .Lcpuid_fault_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	xor %eax, %eax
	xor %ecx, %ecx
.Lcpuid_fault:
	cpuid
.Lcpuid_fault_resume:
	call check_exception
	test %eax, %eax
	jne 1f
	movl $92, failure_code(%rip)
	mov $158, %eax
	mov $ARCH_SET_CPUID, %edi
	mov $1, %esi
	syscall
	test %rax, %rax
	jne 1f
	movl $93, failure_code(%rip)
	xor %eax, %eax
	xor %ecx, %ecx
	cpuid
.Lcpuid_fault_unavailable:
	movl $94, failure_code(%rip)
	call check_initial_arch_bases
	test %eax, %eax
	jne 1f

	movl $95, failure_code(%rip)
	call set_test_arch_bases
	test %eax, %eax
	jne 1f
	call check_test_arch_bases
	test %eax, %eax
	jne 1f
	call clear_arch_bases
	test %eax, %eax
	jne 1f

	movl $96, failure_code(%rip)
	mov $4, %edi
	mov $2, %esi
	mov $6, %edx
	xor %ecx, %ecx
	lea .Lfsgsbase_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
.Lfsgsbase:
	rdfsbase %rax
.Lfsgsbase_resume:
	call check_exception
	test %eax, %eax
	jne 1f

	movl $100, failure_code(%rip)
	call check_xcomp_controls
	test %eax, %eax
	jne 1f
	movl $103, failure_code(%rip)
	call check_arch_prctl_controls
	test %eax, %eax
	jne 1f
	movl $104, failure_code(%rip)
	call check_pkey_controls
	test %eax, %eax
	jne 1f

	movl $101, failure_code(%rip)
	mov $154, %eax
	mov $0x11, %edi
	lea ldt_entry(%rip), %rsi
	mov $16, %edx
	syscall
	cmp $-95, %eax
	jne 1f

	movl $102, failure_code(%rip)
	call check_io_permissions
	test %eax, %eax
	jne 1f

	movl $98, failure_code(%rip)
	mov $PR_SPEC_STORE_BYPASS, %edi
	call check_speculation_control
	test %eax, %eax
	jne 1f
	movl $99, failure_code(%rip)
	mov $PR_SPEC_INDIRECT_BRANCH, %edi
	call check_speculation_control
	test %eax, %eax
	jne 1f

	movl $2, failure_code(%rip)
	call check_hidden_mapping
	test %eax, %eax
	jne 1f
	movl $10, failure_code(%rip)

	cmpl $1, (%rsp)
	jne 4f
	mov $1, %eax
	mov $1, %edi
	lea message(%rip), %rsi
	mov $message_length, %edx
	syscall
	cmp $message_length, %rax
	jne 1f
4:

	mov $13, %eax
	mov $10, %edi
	lea signal_action(%rip), %rsi
	xor %edx, %edx
	mov $8, %r10d
	syscall
	test %rax, %rax
	jne 1f
	movl $109, failure_code(%rip)
	call setup_pkey_lifecycle
	test %eax, %eax
	jne 1f

	mov $39, %eax
	syscall
	mov %eax, %r12d
	mov $186, %eax
	syscall
	mov %eax, %r13d

	movl $82, failure_code(%rip)
	movdqu initial_xmm(%rip), %xmm15
	ldmxcsr initial_mxcsr(%rip)
	fldcw initial_fcw(%rip)
	mov $234, %eax
	mov %r12d, %edi
	mov %r13d, %esi
	mov $10, %edx
	syscall
	test %rax, %rax
	jne 1f

	cmpl $1, signal_seen(%rip)
	jne 1f
	movdqu %xmm15, %xmm0
	pcmpeqb replacement_xmm(%rip), %xmm0
	pmovmskb %xmm0, %eax
	cmp $0xffff, %eax
	jne 1f
	stmxcsr -4(%rsp)
	cmpl $0x5f80, -4(%rsp)
	jne 1f
	fnstcw -2(%rsp)
	cmpw $0x037f, -2(%rsp)
	jne 1f
	movl $109, failure_code(%rip)
	call check_pkey_signal_result
	test %eax, %eax
	jne 1f

	movl $15, failure_code(%rip)
	mov $4, %edi
	call install_exception_handler
	test %eax, %eax
	jne 1f
	mov $5, %edi
	call install_exception_handler
	test %eax, %eax
	jne 1f
	mov $7, %edi
	call install_exception_handler
	test %eax, %eax
	jne 1f
	mov $8, %edi
	call install_exception_handler
	test %eax, %eax
	jne 1f
	mov $11, %edi
	call install_exception_handler
	test %eax, %eax
	jne 1f

	# The ordinary guest range has RWX first-stage PTEs. Permission faults
	# below therefore prove the independent second-stage VMA ceiling.
	movl $60, failure_code(%rip)
	mov $72, %eax
	mov $CONTROL_FD, %edi
	mov $1, %esi
	syscall
	cmp $-9, %rax
	jne 1f

	movl $61, failure_code(%rip)
	mov $319, %eax
	lea security_memfd_name(%rip), %rdi
	mov $1, %esi
	syscall
	cmp $-4095, %rax
	jae 1f
	mov %eax, %r15d

	movl $62, failure_code(%rip)
	mov $77, %eax
	mov %r15d, %edi
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne 1f

	movl $63, failure_code(%rip)
	mov $18, %eax
	mov %r15d, %edi
	lea syscall_initial_byte(%rip), %rsi
	mov $1, %edx
	xor %r10d, %r10d
	syscall
	cmp $1, %rax
	jne 1f

	movl $64, failure_code(%rip)
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $1, %edx
	mov $1, %r10d
	mov %r15d, %r8d
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae 1f
	mov %rax, %r14

	cmpb $0x3c, (%r14)
	jne 1f

	movl $65, failure_code(%rip)
	mov $11, %edi
	mov $2, %esi
	mov $14, %edx
	mov %r14, %rcx
	lea .Lstage2_write_resume(%rip), %r8
	mov %rcx, %r9
	call prepare_exception
	movb $0x5c, (%r14)
.Lstage2_write_resume:
	call check_exception
	test %eax, %eax
	jne 1f

	movl $66, failure_code(%rip)
	mov $18, %eax
	mov %r15d, %edi
	lea syscall_write_byte(%rip), %rsi
	mov $1, %edx
	xor %r10d, %r10d
	syscall
	cmp $1, %rax
	jne 1f
	movl $67, failure_code(%rip)
	cmpb $0xa5, (%r14)
	jne 1f
	movl $68, failure_code(%rip)
	mov $11, %eax
	mov %r14, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne 1f
	movl $69, failure_code(%rip)
	mov $3, %eax
	mov %r15d, %edi
	syscall
	test %rax, %rax
	jne 1f

	movl $70, failure_code(%rip)
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $0x22, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae 1f
	mov %rax, %r14
	movb $0xb8, 0(%r14)
	movl $231, 1(%r14)
	movb $0xbf, 5(%r14)
	movl $69, 6(%r14)
	movw $0x050f, 10(%r14)

	mov $11, %edi
	mov $2, %esi
	mov $14, %edx
	mov %r14, %rcx
	lea .Lstage2_exec_resume(%rip), %r8
	mov %r14, %r9
	call prepare_exception
	jmp *%r14
.Lstage2_exec_resume:
	call check_exception
	test %eax, %eax
	jne 1f
	movl $72, failure_code(%rip)
	mov $11, %eax
	mov %r14, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne 1f
	movl $15, failure_code(%rip)

	mov $8, %edi
	mov $1, %esi
	xor %edx, %edx
	lea .Ldivide(%rip), %rcx
	lea .Ldivide_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	mov $1, %eax
	xor %edx, %edx
	xor %ecx, %ecx
.Ldivide:
	div %ecx
.Ldivide_resume:
	call check_exception
	test %eax, %eax
	jne 1f

	mov $4, %edi
	mov $2, %esi
	mov $6, %edx
	lea .Linvalid_opcode(%rip), %rcx
	lea .Linvalid_opcode_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
.Linvalid_opcode:
	ud2
.Linvalid_opcode_resume:
	call check_exception
	test %eax, %eax
	jne 1f

	mov $5, %edi
	mov $128, %esi
	mov $3, %edx
	xor %ecx, %ecx
	lea .Lbreakpoint_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	int3
.Lbreakpoint_resume:
	call check_exception
	test %eax, %eax
	jne 1f

	mov $5, %edi
	mov $1, %esi
	mov $1, %edx
	lea .Ldebug_resume(%rip), %rcx
	lea .Ldebug_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	.byte 0xf1
.Ldebug_resume:
	call check_exception
	test %eax, %eax
	jne 1f

	mov $11, %edi
	mov $128, %esi
	mov $13, %edx
	xor %ecx, %ecx
	lea .Lgeneral_protection_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	cli
.Lgeneral_protection_resume:
	call check_exception
	test %eax, %eax
	jne 1f

	mov $7, %edi
	mov $1, %esi
	mov $17, %edx
	xor %ecx, %ecx
	lea .Lalignment_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	sub $16, %rsp
	lea 1(%rsp), %rax
	pushfq
	orq $0x40000, (%rsp)
	popfq
	mov (%rax), %eax
.Lalignment_resume:
	add $16, %rsp
	call check_exception
	test %eax, %eax
	jne 1f

	mov $8, %edi
	mov $3, %esi
	mov $19, %edx
	lea .Lsimd_exception(%rip), %rcx
	lea .Lsimd_exception_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	ldmxcsr simd_exception_mxcsr(%rip)
	xorps %xmm0, %xmm0
	movss one_float(%rip), %xmm1
.Lsimd_exception:
	divss %xmm0, %xmm1
.Lsimd_exception_resume:
	ldmxcsr replacement_mxcsr(%rip)
	call check_exception
	test %eax, %eax
	jne 1f

	mov $8, %edi
	mov $3, %esi
	mov $16, %edx
	lea .Lx87_exception(%rip), %rcx
	lea .Lx87_exception_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	fldcw x87_exception_fcw(%rip)
	fld1
	fdivs zero_float(%rip)
.Lx87_exception:
	fwait
.Lx87_exception_resume:
	fninit
	fldcw replacement_fcw(%rip)
	call check_exception
	test %eax, %eax
	jne 1f

	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $0x22, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae 1f
	mov %rax, %r14
	mov %rax, %rdi
	mov $4096, %esi
	mov $11, %eax
	syscall
	test %rax, %rax
	jne 1f
	mov $11, %edi
	mov $1, %esi
	mov $14, %edx
	mov %r14, %rcx
	lea .Lmaperr_resume(%rip), %r8
	mov %r14, %r9
	call prepare_exception
	movq $7, expected_error_mask(%rip)
	movq $4, expected_error(%rip)
	mov (%r14), %rax
.Lmaperr_resume:
	call check_exception
	test %eax, %eax
	jne 1f

	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	xor %edx, %edx
	mov $0x22, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae 1f
	mov %rax, %r14
	mov $11, %edi
	mov $2, %esi
	mov $14, %edx
	mov %r14, %rcx
	lea .Laccerr_resume(%rip), %r8
	mov %r14, %r9
	call prepare_exception
	movq $7, expected_error_mask(%rip)
	movq $5, expected_error(%rip)
	mov (%r14), %rax
.Laccerr_resume:
	call check_exception
	test %eax, %eax
	jne 1f
	mov %r14, %rdi
	mov $4096, %esi
	mov $11, %eax
	syscall
	test %rax, %rax
	jne 1f

	movl $16, failure_code(%rip)
	mov $57, %eax
	syscall
	test %rax, %rax
	jz .Ldefault_signal_child
	js 1f
	sub $16, %rsp
	mov %rax, %r14
	mov %rax, %rdi
	mov %rsp, %rsi
	xor %edx, %edx
	xor %r10d, %r10d
	mov $61, %eax
	syscall
	cmp %r14, %rax
	jne .Ldefault_signal_fail
	cmpl $0x137f, (%rsp)
	jne .Ldefault_signal_fail
	mov $101, %eax
	mov $7, %edi
	mov %r14, %rsi
	xor %edx, %edx
	xor %r10d, %r10d
	syscall
	test %rax, %rax
	jne .Ldefault_signal_fail
	mov %r14, %rdi
	mov %rsp, %rsi
	xor %edx, %edx
	xor %r10d, %r10d
	mov $61, %eax
	syscall
	cmp %r14, %rax
	jne .Ldefault_signal_fail
	cmpl $0x047f, (%rsp)
	jne .Ldefault_signal_fail
	mov $101, %eax
	mov $7, %edi
	mov %r14, %rsi
	xor %edx, %edx
	mov $4, %r10d
	syscall
	test %rax, %rax
	jne .Ldefault_signal_fail
	mov %r14, %rdi
	mov %rsp, %rsi
	xor %edx, %edx
	xor %r10d, %r10d
	mov $61, %eax
	syscall
	cmp %r14, %rax
	jne .Ldefault_signal_fail
	movl (%rsp), %eax
	and $0x7f, %eax
	cmp $4, %eax
	jne .Ldefault_signal_fail
	add $16, %rsp
	jmp .Ldefault_signal_done
.Ldefault_signal_fail:
	add $16, %rsp
	jmp 1f
.Ldefault_signal_child:
	mov $13, %eax
	mov $4, %edi
	lea default_action(%rip), %rsi
	xor %edx, %edx
	mov $8, %r10d
	syscall
	test %rax, %rax
	jne 1f
	mov $101, %eax
	xor %edi, %edi
	xor %esi, %esi
	xor %edx, %edx
	xor %r10d, %r10d
	syscall
	test %rax, %rax
	jne 1f
	mov $39, %eax
	syscall
	mov %eax, %r12d
	mov $186, %eax
	syscall
	mov %eax, %r13d
	mov $234, %eax
	mov %r12d, %edi
	mov %r13d, %esi
	mov $19, %edx
	syscall
	test %rax, %rax
	jne 1f
	ud2
.Ldefault_signal_done:
	movl $97, failure_code(%rip)
	call set_test_arch_bases
	test %eax, %eax
	jne 1f

	movl $20, failure_code(%rip)
	mov $57, %eax
	syscall
	call check_process_clone
	test %eax, %eax
	jne 1f

	movl $21, failure_code(%rip)
	mov $58, %eax
	syscall
	call check_process_clone
	test %eax, %eax
	jne 1f

	movl $22, failure_code(%rip)
	lea clone3_args(%rip), %rdi
	mov $clone3_args_size, %esi
	mov $435, %eax
	syscall
	call check_process_clone
	test %eax, %eax
	jne 1f

	movl $30, failure_code(%rip)
	mov $9, %eax
	xor %edi, %edi
	mov $65536, %esi
	mov $3, %edx
	mov $0x22, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae 1f
	mov %rax, %r14
	lea 65536(%rax), %rsi
	movl $0, thread_tid(%rip)
	movl $0, thread_result(%rip)
	movl $0, thread_ready(%rip)
	movl $0, thread_release(%rip)
	mov $0x310f00, %edi
	lea thread_tid(%rip), %rdx
	lea thread_tid(%rip), %r10
	xor %r8d, %r8d
	mov $56, %eax
	syscall
	test %rax, %rax
	jz 7f
	js 1f
.Lthread_wait_ready:
	cmpl $0, thread_ready(%rip)
	jne .Lthread_ready
	lea thread_ready(%rip), %rdi
	xor %esi, %esi
	xor %edx, %edx
	xor %r10d, %r10d
	xor %r8d, %r8d
	xor %r9d, %r9d
	mov $202, %eax
	syscall
	jmp .Lthread_wait_ready
.Lthread_ready:
	cmpl $0, pku_available(%rip)
	je .Lthread_release
	cmpl $1, thread_ready(%rip)
	jne .Lthread_update_ready
	call check_pkey_inheritance
	test %eax, %eax
	je .Lthread_release_isolation
	movl $114, thread_result(%rip)
.Lthread_release_isolation:
	movl $1, thread_release(%rip)
.Lthread_wait_update_ready:
	cmpl $2, thread_ready(%rip)
	je .Lthread_update_ready
	lea thread_ready(%rip), %rdi
	xor %esi, %esi
	mov $1, %edx
	xor %r10d, %r10d
	xor %r8d, %r8d
	xor %r9d, %r9d
	mov $202, %eax
	syscall
	jmp .Lthread_wait_update_ready
.Lthread_update_ready:
	cmpl $0, thread_result(%rip)
	jne .Lthread_release
	mov $329, %eax
	mov pkey_address(%rip), %rdi
	mov $4096, %esi
	mov $3, %edx
	xor %r10d, %r10d
	syscall
	test %rax, %rax
	jne .Lthread_update_syscall_fail
	call check_pkey_shared_update
	test %eax, %eax
	je .Lthread_release
	movl $112, thread_result(%rip)
	jmp .Lthread_release
.Lthread_update_syscall_fail:
	movl $111, thread_result(%rip)
.Lthread_release:
	movl $2, thread_release(%rip)
8:
	mov thread_tid(%rip), %edx
	test %edx, %edx
	jz 9f
	lea thread_tid(%rip), %rdi
	xor %esi, %esi
	xor %r10d, %r10d
	xor %r8d, %r8d
	xor %r9d, %r9d
	mov $202, %eax
	syscall
	jmp 8b
9:
	mov thread_result(%rip), %edi
	test %edi, %edi
	jnz 2f
	mov %r14, %rdi
	mov $65536, %esi
	mov $11, %eax
	syscall
	test %rax, %rax
	jne 1f
	jmp 10f
7:
	mov initial_stack(%rip), %rdi
	call check_cpu_profile
	test %eax, %eax
	jne 15f
	call check_lam_enabled
	test %eax, %eax
	jne 12f
	call check_pkey_inheritance
	test %eax, %eax
	jne 12f
	call check_test_arch_bases
	test %eax, %eax
	jne 12f
	mov $PR_SPEC_STORE_BYPASS, %edi
	call check_speculation_control
	test %eax, %eax
	jne 12f
	mov $PR_SPEC_INDIRECT_BRANCH, %edi
	call check_speculation_control
	test %eax, %eax
	jne 12f
	call check_hidden_mapping
	test %eax, %eax
	jne 12f
	call check_replacement_fpu
	test %eax, %eax
	je 15f
12:
	mov failure_code(%rip), %eax
15:
	test %eax, %eax
	jz .Lthread_prepare_isolation
	mov %eax, thread_result(%rip)
	jmp .Lthread_signal_update_ready
.Lthread_prepare_isolation:
	cmpl $0, pku_available(%rip)
	je .Lthread_signal_update_ready
	mov pkey_allowed_pkru(%rip), %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
	xor %ecx, %ecx
	rdpkru
	cmpl pkey_allowed_pkru(%rip), %eax
	jne .Lthread_isolation_fail
	movl $1, thread_ready(%rip)
	lea thread_ready(%rip), %rdi
	mov $1, %esi
	mov $1, %edx
	xor %r10d, %r10d
	xor %r8d, %r8d
	xor %r9d, %r9d
	mov $202, %eax
	syscall
.Lthread_wait_isolation_release:
	cmpl $1, thread_release(%rip)
	je .Lthread_check_isolation
	pause
	jmp .Lthread_wait_isolation_release
.Lthread_check_isolation:
	xor %ecx, %ecx
	rdpkru
	cmpl pkey_allowed_pkru(%rip), %eax
	jne .Lthread_isolation_fail
	mov pkey_address(%rip), %rax
	cmpb $0x5a, (%rax)
	jne .Lthread_isolation_fail
	mov pkey_denied_pkru(%rip), %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
	jmp .Lthread_signal_update_ready
.Lthread_isolation_fail:
	mov pkey_denied_pkru(%rip), %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
	movl $115, thread_result(%rip)
.Lthread_signal_update_ready:
	movl $2, thread_ready(%rip)
	lea thread_ready(%rip), %rdi
	mov $1, %esi
	mov $1, %edx
	xor %r10d, %r10d
	xor %r8d, %r8d
	xor %r9d, %r9d
	mov $202, %eax
	syscall
.Lthread_wait_release:
	cmpl $2, thread_release(%rip)
	je .Lthread_check_update
	pause
	jmp .Lthread_wait_release
.Lthread_check_update:
	cmpl $0, thread_result(%rip)
	jne .Lthread_exit
	call check_pkey_shared_update
	test %eax, %eax
	je .Lthread_exit
	movl $113, thread_result(%rip)
.Lthread_exit:
	mov thread_result(%rip), %eax
	mov %eax, %edi
	mov $60, %eax
	syscall
	ud2
10:
	call check_hidden_mapping
	test %eax, %eax
	jne 1f

	movl $40, failure_code(%rip)
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $0x22, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae 1f

	movabs $0x0123456789abcdef, %rcx
	mov %rcx, (%rax)
	cmp %rcx, (%rax)
	jne 1f

	mov %rax, %rdi
	mov $11, %eax
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne 1f
	call prepare_pkey_exec

	movl $50, failure_code(%rip)
	cmpl $1, (%rsp)
	jne 4f
	mov 8(%rsp), %rdi
	mov %rdi, reexec_argv(%rip)
	lea reexec_argv(%rip), %rsi
	xor %edx, %edx
	mov $59, %eax
	syscall
	jmp 1f
4:
	xor %edi, %edi
	jmp 2f
1:
	mov failure_code(%rip), %edi
2:
	mov $231, %eax
	syscall
	ud2
	.size _start, . - _start

	.type install_exception_handler, @function
install_exception_handler:
	mov $13, %eax
	lea exception_action(%rip), %rsi
	xor %edx, %edx
	mov $8, %r10d
	syscall
	ret
	.size install_exception_handler, . - install_exception_handler

	.type prepare_exception, @function
prepare_exception:
	movl %edi, expected_signal(%rip)
	movl %esi, expected_code(%rip)
	movq %rdx, expected_trap(%rip)
	movq $0, expected_error_mask(%rip)
	movq $0, expected_error(%rip)
	movq %rcx, expected_address(%rip)
	movq %r9, expected_cr2(%rip)
	movq %r8, exception_resume(%rip)
	movl $0, exception_seen(%rip)
	movl $0, expected_pkey(%rip)
	ret
	.size prepare_exception, . - prepare_exception

	.type check_exception, @function
check_exception:
	xor %eax, %eax
	cmpl $1, exception_seen(%rip)
	sete %al
	xor $1, %eax
	ret
	.size check_exception, . - check_exception

	.type check_process_clone, @function
check_process_clone:
	test %rax, %rax
	jz .Lprocess_child
	js .Lprocess_fail
	sub $16, %rsp
	mov %rax, 8(%rsp)
	mov %rax, %rdi
	mov %rsp, %rsi
	xor %edx, %edx
	xor %r10d, %r10d
	mov $61, %eax
	syscall
	cmp 8(%rsp), %rax
	jne .Lprocess_wait_fail
	cmpl $0, (%rsp)
	jne .Lprocess_wait_fail
	add $16, %rsp
	xor %eax, %eax
	ret
.Lprocess_child:
	mov initial_stack(%rip), %rdi
	call check_cpu_profile
	test %eax, %eax
	jne .Lprocess_child_fail
	call check_lam_enabled
	test %eax, %eax
	jne .Lprocess_child_fail
	call check_pkey_inheritance
	test %eax, %eax
	jne .Lprocess_child_fail
	call check_test_arch_bases
	test %eax, %eax
	jne .Lprocess_child_fail
	mov $PR_SPEC_STORE_BYPASS, %edi
	call check_speculation_control
	test %eax, %eax
	jne .Lprocess_child_fail
	mov $PR_SPEC_INDIRECT_BRANCH, %edi
	call check_speculation_control
	test %eax, %eax
	jne .Lprocess_child_fail
	call check_hidden_mapping
	test %eax, %eax
	jne .Lprocess_child_fail
	call check_replacement_fpu
	test %eax, %eax
	jne .Lprocess_child_fail
	xor %edi, %edi
	jmp .Lprocess_exit
.Lprocess_wait_fail:
	add $16, %rsp
.Lprocess_fail:
	mov $1, %eax
	ret
.Lprocess_child_fail:
	mov failure_code(%rip), %edi
.Lprocess_exit:
	mov $60, %eax
	syscall
	ud2
	.size check_process_clone, . - check_process_clone

	.type check_initial_arch_bases, @function
check_initial_arch_bases:
	sub $16, %rsp
	mov $158, %eax
	mov $ARCH_GET_FS, %edi
	mov %rsp, %rsi
	syscall
	test %rax, %rax
	jne 1f
	cmpq $0, (%rsp)
	jne 1f
	mov $158, %eax
	mov $ARCH_GET_GS, %edi
	lea 8(%rsp), %rsi
	syscall
	test %rax, %rax
	jne 1f
	cmpq $0, 8(%rsp)
	jne 1f
	add $16, %rsp
	xor %eax, %eax
	ret
1:
	add $16, %rsp
	mov $1, %eax
	ret
	.size check_initial_arch_bases, . - check_initial_arch_bases

	.type set_test_arch_bases, @function
set_test_arch_bases:
	mov $158, %eax
	mov $ARCH_SET_FS, %edi
	lea fs_marker(%rip), %rsi
	syscall
	test %rax, %rax
	jne 1f
	mov $158, %eax
	mov $ARCH_SET_GS, %edi
	lea gs_marker(%rip), %rsi
	syscall
	test %rax, %rax
	setne %al
	movzbl %al, %eax
	ret
1:
	mov $1, %eax
	ret
	.size set_test_arch_bases, . - set_test_arch_bases

	.type check_test_arch_bases, @function
check_test_arch_bases:
	sub $16, %rsp
	mov $158, %eax
	mov $ARCH_GET_FS, %edi
	mov %rsp, %rsi
	syscall
	test %rax, %rax
	jne 1f
	lea fs_marker(%rip), %rax
	cmp %rax, (%rsp)
	jne 1f
	mov $158, %eax
	mov $ARCH_GET_GS, %edi
	lea 8(%rsp), %rsi
	syscall
	test %rax, %rax
	jne 1f
	lea gs_marker(%rip), %rax
	cmp %rax, 8(%rsp)
	jne 1f
	mov %fs:0, %rax
	cmp fs_marker(%rip), %rax
	jne 1f
	mov %gs:0, %rax
	cmp gs_marker(%rip), %rax
	jne 1f
	add $16, %rsp
	xor %eax, %eax
	ret
1:
	add $16, %rsp
	mov $1, %eax
	ret
	.size check_test_arch_bases, . - check_test_arch_bases

	.type clear_arch_bases, @function
clear_arch_bases:
	mov $158, %eax
	mov $ARCH_SET_FS, %edi
	xor %esi, %esi
	syscall
	test %rax, %rax
	jne 1f
	mov $158, %eax
	mov $ARCH_SET_GS, %edi
	xor %esi, %esi
	syscall
	test %rax, %rax
	setne %al
	movzbl %al, %eax
	ret
1:
	mov $1, %eax
	ret
	.size clear_arch_bases, . - clear_arch_bases

	.type check_xcomp_controls, @function
check_xcomp_controls:
	sub $16, %rsp
	mov $158, %eax
	mov $ARCH_GET_XCOMP_SUPP, %edi
	mov %rsp, %rsi
	syscall
	test %rax, %rax
	jne 1f
	mov expected_xfeatures(%rip), %rcx
	cmp %rcx, (%rsp)
	jne 1f
	mov $158, %eax
	mov $ARCH_GET_XCOMP_PERM, %edi
	lea 8(%rsp), %rsi
	syscall
	test %rax, %rax
	jne 1f
	mov expected_xfeatures(%rip), %rcx
	cmp %rcx, 8(%rsp)
	jne 1f
	mov $158, %eax
	mov $ARCH_REQ_XCOMP_PERM, %edi
	mov $64, %esi
	syscall
	cmp $-22, %rax
	jne 1f
	mov $158, %eax
	mov $ARCH_REQ_XCOMP_PERM, %edi
	mov $18, %esi
	syscall
	cmp $-95, %rax
	setne %al
	movzbl %al, %eax
	add $16, %rsp
	ret
1:
	add $16, %rsp
	mov $1, %eax
	ret
	.size check_xcomp_controls, . - check_xcomp_controls

	.type check_arch_prctl_controls, @function
check_arch_prctl_controls:
	sub $16, %rsp
	movl $116, failure_code(%rip)
	mov $158, %eax
	mov $ARCH_GET_MAX_TAG_BITS, %edi
	mov %rsp, %rsi
	syscall
	test %rax, %rax
	jz .Larch_prctl_tag_bits
	cmp $-22, %rax
	jne 1f
	cmpl $0, lam_available(%rip)
	jne 1f
	jmp .Larch_prctl_no_lam
.Larch_prctl_tag_bits:
	cmpl $0, lam_available(%rip)
	jne .Larch_prctl_lam
	cmpq $0, (%rsp)
	jne 1f
	jmp .Larch_prctl_no_lam
.Larch_prctl_lam:
	cmpq $6, (%rsp)
	jne 1f
	movl $117, failure_code(%rip)
	mov $158, %eax
	mov $ARCH_GET_UNTAG_MASK, %edi
	lea 8(%rsp), %rsi
	syscall
	test %rax, %rax
	jne 1f
	cmpq $-1, 8(%rsp)
	jne 1f
	mov $158, %eax
	mov $ARCH_ENABLE_TAGGED_ADDR, %edi
	mov $6, %esi
	syscall
	test %rax, %rax
	jne 1f
	mov $158, %eax
	mov $ARCH_GET_UNTAG_MASK, %edi
	mov %rsp, %rsi
	syscall
	test %rax, %rax
	jne 1f
	movabs $0x81ffffffffffffff, %rax
	cmp %rax, (%rsp)
	jne 1f
	movl $118, failure_code(%rip)
	lea lam_marker(%rip), %rcx
	movabs $0x0200000000000000, %rax
	or %rax, %rcx
	mov (%rcx), %rax
	cmp lam_marker(%rip), %rax
	jne 1f
	movl $119, failure_code(%rip)
	mov $158, %eax
	mov $ARCH_FORCE_TAGGED_SVA, %edi
	xor %esi, %esi
	syscall
	test %rax, %rax
	jne 1f
	jmp .Larch_prctl_cet_status
.Larch_prctl_no_lam:
	movl $117, failure_code(%rip)
	mov $158, %eax
	mov $ARCH_ENABLE_TAGGED_ADDR, %edi
	mov $6, %esi
	syscall
	cmp $-95, %rax
	je .Larch_prctl_force_no_lam
	cmp $-19, %rax
	je .Larch_prctl_force_no_lam
	cmp $-22, %rax
	jne 1f
.Larch_prctl_force_no_lam:
	movl $119, failure_code(%rip)
	mov $158, %eax
	mov $ARCH_FORCE_TAGGED_SVA, %edi
	xor %esi, %esi
	syscall
	cmp $-95, %rax
	je .Larch_prctl_cet_status
	cmp $-22, %rax
	jne 1f
.Larch_prctl_cet_status:
	movl $120, failure_code(%rip)
	mov $158, %eax
	mov $ARCH_SHSTK_STATUS, %edi
	lea 8(%rsp), %rsi
	syscall
	test %rax, %rax
	jz .Larch_prctl_cet
	cmp $-22, %rax
	je .Larch_prctl_success
	jmp 1f
.Larch_prctl_cet:
	cmpq $0, 8(%rsp)
	jne 1f
	mov $158, %eax
	mov $ARCH_SHSTK_ENABLE, %edi
	xor %esi, %esi
	syscall
	cmp $-22, %rax
	jne 1f
	mov $158, %eax
	mov $ARCH_SHSTK_ENABLE, %edi
	mov $ARCH_SHSTK_SHSTK, %esi
	syscall
	cmp $-95, %rax
	jne 1f
	mov $158, %eax
	mov $ARCH_SHSTK_LOCK, %edi
	mov $ARCH_SHSTK_SHSTK, %esi
	syscall
	cmp $-95, %rax
	jne 1f
.Larch_prctl_success:
	add $16, %rsp
	xor %eax, %eax
	ret
1:
	add $16, %rsp
	mov $1, %eax
	ret
	.size check_arch_prctl_controls, . - check_arch_prctl_controls

	.type check_lam_enabled, @function
check_lam_enabled:
	cmpl $0, lam_available(%rip)
	je .Llam_enabled_success
	movl $121, failure_code(%rip)
	sub $8, %rsp
	mov $158, %eax
	mov $ARCH_GET_UNTAG_MASK, %edi
	mov %rsp, %rsi
	syscall
	test %rax, %rax
	jne .Llam_enabled_fail
	movabs $0x81ffffffffffffff, %rax
	cmp %rax, (%rsp)
	jne .Llam_enabled_fail
	lea lam_marker(%rip), %rcx
	movabs $0x0200000000000000, %rax
	or %rax, %rcx
	mov (%rcx), %rax
	cmp lam_marker(%rip), %rax
	jne .Llam_enabled_fail
	add $8, %rsp
.Llam_enabled_success:
	xor %eax, %eax
	ret
.Llam_enabled_fail:
	add $8, %rsp
	mov $1, %eax
	ret
	.size check_lam_enabled, . - check_lam_enabled

	.type check_pkey_controls, @function
check_pkey_controls:
	push %r12
	push %r13
	push %r14
	push %r15
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $0x22, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Lpkey_no_mapping
	mov %rax, %r12
	mov $330, %eax
	xor %edi, %edi
	xor %esi, %esi
	cmpl $0, pku_available(%rip)
	je .Lpkey_alloc
	mov $2, %esi
.Lpkey_alloc:
	syscall
	cmp $-28, %rax
	je .Lpkey_success_unmap
	test %rax, %rax
	js .Lpkey_fail_unmap
	cmp $1, %eax
	jne .Lpkey_fail_unmap
	mov %eax, %r13d
	cmpl $0, pku_available(%rip)
	je .Lpkey_initial_rights_done
	xor %ecx, %ecx
	rdpkru
	mov %eax, %r14d
	mov %r13d, %ecx
	shl $1, %ecx
	mov $3, %edx
	shl %cl, %edx
	not %edx
	and %edx, %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
	mov %r13d, %ecx
	shl $1, %ecx
	shr %cl, %r14d
	and $3, %r14d
	cmp $2, %r14d
	jne .Lpkey_fail_free
.Lpkey_initial_rights_done:
	mov $329, %eax
	mov %r12, %rdi
	mov $4096, %esi
	mov $3, %edx
	xor %r10d, %r10d
	syscall
	test %rax, %rax
	jne .Lpkey_fail_free
	mov $329, %eax
	mov %r12, %rdi
	mov $4096, %esi
	mov $3, %edx
	mov %r13d, %r10d
	syscall
	cmpl $0, pku_available(%rip)
	jne .Lpkey_positive
	cmp $-95, %rax
	jne .Lpkey_fail_free
	jmp .Lpkey_success_free
.Lpkey_positive:
	test %rax, %rax
	jne .Lpkey_fail_free
	movb $0x5a, (%r12)
	xor %ecx, %ecx
	rdpkru
	mov %eax, %r15d
	mov %r13d, %ecx
	shl $1, %ecx
	mov $1, %eax
	shl %cl, %eax
	or %r15d, %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
	mov $11, %edi
	mov $4, %esi
	mov $14, %edx
	mov %r12, %rcx
	lea .Lpkey_fault_resume(%rip), %r8
	mov %r12, %r9
	call prepare_exception
	movl %r13d, expected_pkey(%rip)
	movq $0x25, expected_error_mask(%rip)
	movq $0x25, expected_error(%rip)
	mov (%r12), %rax
.Lpkey_fault_resume:
	call check_exception
	mov %eax, %r14d
	mov %r15d, %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
	movl $0, expected_pkey(%rip)
	test %r14d, %r14d
	jne .Lpkey_fail_free
	cmpb $0x5a, (%r12)
	jne .Lpkey_fail_free
	mov %r13d, %ecx
	shl $1, %ecx
	mov $2, %eax
	shl %cl, %eax
	or %r15d, %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
	cmpb $0x5a, (%r12)
	jne .Lpkey_restore_fail
	mov $11, %edi
	mov $4, %esi
	mov $14, %edx
	mov %r12, %rcx
	lea .Lpkey_write_fault_resume(%rip), %r8
	mov %r12, %r9
	call prepare_exception
	movl %r13d, expected_pkey(%rip)
	movq $0x27, expected_error_mask(%rip)
	movq $0x27, expected_error(%rip)
	movb $0x6b, (%r12)
.Lpkey_write_fault_resume:
	call check_exception
	mov %eax, %r14d
	mov %r15d, %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
	movl $0, expected_pkey(%rip)
	test %r14d, %r14d
	jne .Lpkey_fail_free
	cmpb $0x5a, (%r12)
	jne .Lpkey_fail_free
	mov $331, %eax
	mov %r13d, %edi
	syscall
	test %rax, %rax
	jne .Lpkey_fail_free
	mov $330, %eax
	xor %edi, %edi
	mov $1, %esi
	syscall
	test %rax, %rax
	js .Lpkey_fail_unmap
	cmp %r13d, %eax
	je .Lpkey_reallocated
	mov %eax, %r13d
	jmp .Lpkey_fail_free
.Lpkey_reallocated:
	mov $11, %edi
	mov $4, %esi
	mov $14, %edx
	mov %r12, %rcx
	lea .Lpkey_reallocated_fault_resume(%rip), %r8
	mov %r12, %r9
	call prepare_exception
	movl %r13d, expected_pkey(%rip)
	movq $0x25, expected_error_mask(%rip)
	movq $0x25, expected_error(%rip)
	mov (%r12), %rax
.Lpkey_reallocated_fault_resume:
	call check_exception
	mov %eax, %r14d
	mov %r15d, %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
	movl $0, expected_pkey(%rip)
	test %r14d, %r14d
	jne .Lpkey_fail_free
	mov $329, %eax
	mov %r12, %rdi
	mov $4096, %esi
	mov $3, %edx
	xor %r10d, %r10d
	syscall
	test %rax, %rax
	jne .Lpkey_fail_free
.Lpkey_success_free:
	xor %r14d, %r14d
	jmp .Lpkey_free
.Lpkey_restore_fail:
	mov %r15d, %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
.Lpkey_fail_free:
	mov $1, %r14d
.Lpkey_free:
	mov $331, %eax
	mov %r13d, %edi
	syscall
	test %rax, %rax
	jne .Lpkey_fail_unmap
	jmp .Lpkey_unmap
.Lpkey_success_unmap:
	xor %r14d, %r14d
.Lpkey_unmap:
	mov $11, %eax
	mov %r12, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	cmovne %eax, %r14d
	jmp .Lpkey_return
.Lpkey_fail_unmap:
	mov $1, %r14d
	mov $11, %eax
	mov %r12, %rdi
	mov $4096, %esi
	syscall
	jmp .Lpkey_return
.Lpkey_no_mapping:
	mov $1, %r14d
.Lpkey_return:
	mov %r14d, %eax
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	ret
	.size check_pkey_controls, . - check_pkey_controls

	.type check_initial_pkru, @function
check_initial_pkru:
	cmpl $0, pku_available(%rip)
	je .Linitial_pkru_success
	xor %ecx, %ecx
	rdpkru
	test $4, %eax
	jz .Linitial_pkru_fail
.Linitial_pkru_success:
	xor %eax, %eax
	ret
.Linitial_pkru_fail:
	mov $1, %eax
	ret
	.size check_initial_pkru, . - check_initial_pkru

	.type setup_pkey_lifecycle, @function
setup_pkey_lifecycle:
	cmpl $0, pku_available(%rip)
	je .Lpkey_lifecycle_success
	push %r12
	push %r13
	push %r14
	push %r15
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $0x22, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Lpkey_lifecycle_fail
	mov %rax, %r12
	mov $330, %eax
	xor %edi, %edi
	xor %esi, %esi
	syscall
	cmp $1, %eax
	jne .Lpkey_lifecycle_fail
	mov %eax, %r13d
	mov $329, %eax
	mov %r12, %rdi
	mov $4096, %esi
	mov $3, %edx
	mov %r13d, %r10d
	syscall
	test %rax, %rax
	jne .Lpkey_lifecycle_fail
	movb $0x5a, (%r12)
	xor %ecx, %ecx
	rdpkru
	mov %eax, %r14d
	mov %r13d, %ecx
	shl $1, %ecx
	mov $1, %r15d
	shl %cl, %r15d
	or %r14d, %r15d
	mov %r13d, pkey_number(%rip)
	mov %r14d, pkey_allowed_pkru(%rip)
	mov %r15d, pkey_denied_pkru(%rip)
	mov %r12, pkey_address(%rip)
	mov %r15d, %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
	xor %eax, %eax
	jmp .Lpkey_lifecycle_pop
.Lpkey_lifecycle_fail:
	mov $1, %eax
.Lpkey_lifecycle_pop:
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	ret
.Lpkey_lifecycle_success:
	xor %eax, %eax
	ret
	.size setup_pkey_lifecycle, . - setup_pkey_lifecycle

	.type check_pkey_signal_result, @function
check_pkey_signal_result:
	cmpl $0, pku_available(%rip)
	je .Lpkey_signal_success
	xor %ecx, %ecx
	rdpkru
	cmpl pkey_allowed_pkru(%rip), %eax
	jne .Lpkey_signal_fail
	mov pkey_address(%rip), %rdx
	cmpb $0x5a, (%rdx)
	jne .Lpkey_signal_fail
	mov pkey_denied_pkru(%rip), %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
.Lpkey_signal_success:
	xor %eax, %eax
	ret
.Lpkey_signal_fail:
	mov $1, %eax
	ret
	.size check_pkey_signal_result, . - check_pkey_signal_result

	.type check_pkey_inheritance, @function
check_pkey_inheritance:
	cmpl $0, pku_available(%rip)
	je .Lpkey_inheritance_success
	xor %ecx, %ecx
	rdpkru
	cmpl pkey_denied_pkru(%rip), %eax
	jne .Lpkey_inheritance_fail
	mov $11, %edi
	mov $4, %esi
	mov $14, %edx
	mov pkey_address(%rip), %rcx
	lea .Lpkey_inheritance_resume(%rip), %r8
	mov %rcx, %r9
	call prepare_exception
	movl pkey_number(%rip), %eax
	movl %eax, expected_pkey(%rip)
	movq $0x25, expected_error_mask(%rip)
	movq $0x25, expected_error(%rip)
	mov pkey_address(%rip), %rcx
	mov (%rcx), %rax
.Lpkey_inheritance_resume:
	call check_exception
	test %eax, %eax
	jne .Lpkey_inheritance_fail
.Lpkey_inheritance_success:
	xor %eax, %eax
	ret
.Lpkey_inheritance_fail:
	mov $1, %eax
	ret
	.size check_pkey_inheritance, . - check_pkey_inheritance

	.type check_pkey_shared_update, @function
check_pkey_shared_update:
	cmpl $0, pku_available(%rip)
	je .Lpkey_shared_update_success
	xor %ecx, %ecx
	rdpkru
	cmpl pkey_denied_pkru(%rip), %eax
	jne .Lpkey_shared_update_fail
	mov $11, %edi
	mov $4, %esi
	mov $14, %edx
	mov pkey_address(%rip), %rcx
	lea .Lpkey_shared_update_fault(%rip), %r8
	mov %rcx, %r9
	call prepare_exception
	movl pkey_number(%rip), %eax
	movl %eax, expected_pkey(%rip)
	movq $0x25, expected_error_mask(%rip)
	movq $0x25, expected_error(%rip)
	mov pkey_address(%rip), %rcx
	cmpb $0x5a, (%rcx)
	jne .Lpkey_shared_update_fail
.Lpkey_shared_update_success:
	xor %eax, %eax
	ret
.Lpkey_shared_update_fault:
.Lpkey_shared_update_fail:
	mov $1, %eax
	ret
	.size check_pkey_shared_update, . - check_pkey_shared_update

	.type prepare_pkey_exec, @function
prepare_pkey_exec:
	cmpl $0, pku_available(%rip)
	je .Lpkey_exec_ready
	mov pkey_allowed_pkru(%rip), %eax
	xor %ecx, %ecx
	xor %edx, %edx
	wrpkru
.Lpkey_exec_ready:
	ret
	.size prepare_pkey_exec, . - prepare_pkey_exec

	.type check_io_permissions, @function
check_io_permissions:
	mov $173, %eax
	mov $0xffff, %edi
	mov $2, %esi
	mov $1, %edx
	syscall
	cmp $-22, %rax
	jne 1f
	mov $173, %eax
	mov $0x80, %edi
	mov $1, %esi
	mov $1, %edx
	syscall
	cmp $-95, %rax
	je 2f
	cmp $-1, %rax
	jne 1f
2:
	mov $173, %eax
	mov $0x80, %edi
	mov $1, %esi
	xor %edx, %edx
	syscall
	test %rax, %rax
	jne 1f
	mov $172, %eax
	mov $4, %edi
	syscall
	cmp $-22, %rax
	jne 1f
	mov $172, %eax
	mov $3, %edi
	syscall
	cmp $-95, %rax
	je 3f
	cmp $-1, %rax
	jne 1f
3:
	mov $172, %eax
	xor %edi, %edi
	syscall
	test %rax, %rax
	setne %al
	movzbl %al, %eax
	ret
1:
	mov $1, %eax
	ret
	.size check_io_permissions, . - check_io_permissions

	.type check_speculation_control, @function
check_speculation_control:
	push %rbx
	push %r12
	push %r13
	mov %edi, %ebx
	mov $157, %eax
	mov $PR_GET_SPECULATION_CTRL, %edi
	mov %ebx, %esi
	xor %edx, %edx
	xor %r10d, %r10d
	xor %r8d, %r8d
	xor %r9d, %r9d
	syscall
	test %rax, %rax
	js 2f
	mov %eax, %r12d
	test $PR_SPEC_PRCTL, %r12d
	jz 1f
	test $PR_SPEC_FORCE_DISABLE, %r12d
	jnz 1f
	mov $PR_SPEC_ENABLE, %r13d
	test $PR_SPEC_ENABLE, %r12d
	jz 3f
	mov $PR_SPEC_DISABLE, %r13d
3:
	mov $157, %eax
	mov $PR_SET_SPECULATION_CTRL, %edi
	mov %ebx, %esi
	mov %r13d, %edx
	xor %r10d, %r10d
	xor %r8d, %r8d
	xor %r9d, %r9d
	syscall
	test %rax, %rax
	jne 2f
	mov $157, %eax
	mov $PR_GET_SPECULATION_CTRL, %edi
	mov %ebx, %esi
	xor %edx, %edx
	xor %r10d, %r10d
	xor %r8d, %r8d
	xor %r9d, %r9d
	syscall
	test %rax, %rax
	js 2f
	test %r13d, %eax
	jz 2f
	mov $PR_SPEC_ENABLE, %edx
	test $PR_SPEC_DISABLE_NOEXEC, %r12d
	jz 4f
	mov $PR_SPEC_DISABLE_NOEXEC, %edx
	jmp 5f
4:
	test $PR_SPEC_DISABLE, %r12d
	jz 5f
	mov $PR_SPEC_DISABLE, %edx
5:
	mov $157, %eax
	mov $PR_SET_SPECULATION_CTRL, %edi
	mov %ebx, %esi
	xor %r10d, %r10d
	xor %r8d, %r8d
	xor %r9d, %r9d
	syscall
	test %rax, %rax
	jne 2f
1:
	pop %r13
	pop %r12
	pop %rbx
	xor %eax, %eax
	ret
2:
	pop %r13
	pop %r12
	pop %rbx
	mov $1, %eax
	ret
	.size check_speculation_control, . - check_speculation_control

	.type check_cpu_profile, @function
check_cpu_profile:
	push %rbx
	push %r12
	push %r13
	movl $0, pku_available(%rip)
	movl $0, lam_available(%rip)
	movl $FP_XSTATE_SIZE, expected_xstate_size(%rip)
	movl $FP_XSTATE_EXTENDED_SIZE, expected_xstate_extended_size(%rip)
	movl $0, expected_pkru_offset(%rip)
	movq $XFEATURE_MASK_FPSSE, expected_xfeatures(%rip)
	mov $73, %r11d
	xor %eax, %eax
	xor %ecx, %ecx
	cpuid
	cmp $1, %eax
	je .Lcpu_basic_leaf
	cmp $7, %eax
	je .Lcpu_check_lam
	cmp $0xd, %eax
	jne 2f
	movl $1, pku_available(%rip)
.Lcpu_check_lam:
	mov $108, %r11d
	mov $7, %eax
	mov $1, %ecx
	cpuid
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	test %eax, %eax
	je .Lcpu_basic_leaf
	cmp $0x4000000, %eax
	jne 2f
	movl $1, lam_available(%rip)
.Lcpu_basic_leaf:

	mov $74, %r11d
	mov $1, %eax
	xor %ecx, %ecx
	cpuid
	mov $ELF_HWCAP, %r9d
	cmp $ELF_HWCAP, %edx
	jne 2f
	cmpl $0, pku_available(%rip)
	je .Lcpu_no_pku_leaf1
	cmp $0xc000000, %ecx
	jne 2f
	jmp .Lcpu_leaf7
.Lcpu_no_pku_leaf1:
	test %ecx, %ecx
	jne 2f
	cmpl $0, lam_available(%rip)
	jne .Lcpu_leaf7
	jmp .Lcpu_extended

.Lcpu_leaf7:
	mov $105, %r11d
	mov $7, %eax
	xor %ecx, %ecx
	cpuid
	mov lam_available(%rip), %r12d
	cmp %r12d, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	xor %r13d, %r13d
	cmpl $0, pku_available(%rip)
	je .Lcpu_leaf7_ecx
	mov $0x18, %r13d
.Lcpu_leaf7_ecx:
	cmp %r13d, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	mov $108, %r11d
	mov $7, %eax
	mov $1, %ecx
	cpuid
	mov lam_available(%rip), %r12d
	shl $26, %r12d
	cmp %r12d, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	cmpl $0, pku_available(%rip)
	je .Lcpu_extended

	mov $106, %r11d
	mov $0xd, %eax
	mov $9, %ecx
	cpuid
	test %eax, %eax
	jz 2f
	cmp $FP_XSTATE_SIZE, %ebx
	jb 2f
	mov %eax, %r12d
	mov %ebx, %r13d
	add %r13d, %eax
	mov %eax, expected_xstate_size(%rip)
	add $4, %eax
	mov %eax, expected_xstate_extended_size(%rip)
	mov %r13d, expected_pkru_offset(%rip)
	movq $XFEATURE_MASK_PKU, expected_xfeatures(%rip)
	mov $0xd, %eax
	xor %ecx, %ecx
	cpuid
	cmp $XFEATURE_MASK_PKU, %eax
	jne 2f
	test %edx, %edx
	jne 2f
	mov expected_xstate_size(%rip), %r12d
	cmp %r12d, %ebx
	jne 2f
	cmp %r12d, %ecx
	jne 2f

.Lcpu_extended:

	mov $75, %r11d
	mov $0x80000000, %eax
	xor %ecx, %ecx
	cpuid
	cmp $0x80000008, %eax
	jne 2f
	mov $76, %r11d
	mov $0x80000001, %eax
	xor %ecx, %ecx
	cpuid
	test %ecx, %ecx
	jne 2f
	cmp $0x20100800, %edx
	jne 2f

	mov (%rdi), %rcx
	lea 16(%rdi,%rcx,8), %r8
3:
	cmpq $0, (%r8)
	je 4f
	add $8, %r8
	jmp 3b
4:
	add $8, %r8
	xor %r10d, %r10d
5:
	cmpq $AT_HWCAP, (%r8)
	je 7f
	cmpq $AT_HWCAP2, (%r8)
	je 6f
	cmpq $0, (%r8)
	je 8f
	add $16, %r8
	jmp 5b
6:
	mov $78, %r11d
	cmpq $0, 8(%r8)
	jne 2f
	or $2, %r10d
	add $16, %r8
	jmp 5b
7:
	mov $77, %r11d
	cmpq %r9, 8(%r8)
	jne 2f
	or $1, %r10d
	add $16, %r8
	jmp 5b
8:
	mov $79, %r11d
	cmp $3, %r10d
	jne 2f

	mov $81, %r11d
	cmpl $0, pku_available(%rip)
	jne .Lcpu_xgetbv_pku
	mov $4, %edi
	mov $2, %esi
	mov $6, %edx
	lea .Lxgetbv(%rip), %rcx
	lea .Lxgetbv_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	xor %ecx, %ecx
.Lxgetbv:
	xgetbv
.Lxgetbv_resume:
	call check_exception
	test %eax, %eax
	jne 2f
	jmp .Lcpu_profile_done
.Lcpu_xgetbv_pku:
	mov $107, %r11d
	xor %ecx, %ecx
	xgetbv
	cmp $XFEATURE_MASK_PKU, %eax
	jne 2f
	test %edx, %edx
	jne 2f
.Lcpu_profile_done:
	xor %eax, %eax
	pop %r13
	pop %r12
	pop %rbx
	ret
2:
	mov %r11d, %eax
	pop %r13
	pop %r12
	pop %rbx
	ret
	.size check_cpu_profile, . - check_cpu_profile

	.type check_hidden_mapping, @function
check_hidden_mapping:
	sub $8, %rsp
	mov $39, %eax
	syscall
	mov %eax, (%rsp)
	mov $1, %eax
	mov $READY_FD, %edi
	mov %rsp, %rsi
	mov $4, %edx
	syscall
	cmp $4, %rax
	jne 13f
	xor %eax, %eax
	mov $ADDRESS_FD, %edi
	mov %rsp, %rsi
	mov $8, %edx
	syscall
	cmp $8, %rax
	jne 13f
	mov (%rsp), %rsi
	add $8, %rsp
	mov $1, %eax
	mov $1, %edi
	mov $1, %edx
	syscall
	cmp $-14, %rax
	jne 14f
	xor %eax, %eax
	ret
13:
	add $8, %rsp
14:
	mov $1, %eax
	ret
	.size check_hidden_mapping, . - check_hidden_mapping

	.type check_replacement_fpu, @function
check_replacement_fpu:
	movdqu %xmm15, %xmm0
	pcmpeqb replacement_xmm(%rip), %xmm0
	pmovmskb %xmm0, %eax
	cmp $0xffff, %eax
	jne 11f
	stmxcsr -4(%rsp)
	cmpl $0x5f80, -4(%rsp)
	jne 11f
	fnstcw -2(%rsp)
	cmpw $0x037f, -2(%rsp)
	jne 11f
	xor %eax, %eax
	ret
11:
	mov $1, %eax
	ret
	.size check_replacement_fpu, . - check_replacement_fpu

	.type signal_handler, @function
signal_handler:
	mov UCONTEXT_FPSTATE(%rdx), %r8
	test %r8, %r8
	jz 3f
	movl $83, failure_code(%rip)
	cmpl $FP_XSTATE_MAGIC1, FPSTATE_SW_MAGIC1(%r8)
	jne 3f
	movl $84, failure_code(%rip)
	mov expected_xstate_extended_size(%rip), %eax
	cmpl %eax, FPSTATE_SW_EXTENDED_SIZE(%r8)
	jne 3f
	movl $85, failure_code(%rip)
	mov expected_xfeatures(%rip), %rax
	cmpq %rax, FPSTATE_SW_XFEATURES(%r8)
	jne 3f
	movl $86, failure_code(%rip)
	mov expected_xstate_size(%rip), %eax
	cmpl %eax, FPSTATE_SW_XSTATE_SIZE(%r8)
	jne 3f
	movl $87, failure_code(%rip)
	mov expected_xfeatures(%rip), %rax
	cmpq %rax, FPSTATE_XFEATURES(%r8)
	jne 3f
	movl $88, failure_code(%rip)
	mov expected_xstate_size(%rip), %eax
	cmpl $FP_XSTATE_MAGIC2, (%r8,%rax)
	jne 3f
	cmpl $0, pku_available(%rip)
	je 4f
	mov expected_pkru_offset(%rip), %eax
	mov pkey_denied_pkru(%rip), %ecx
	cmpl %ecx, (%r8,%rax)
	jne 3f
	mov pkey_allowed_pkru(%rip), %ecx
	movl %ecx, (%r8,%rax)
4:
	movl $82, failure_code(%rip)
	cmpw $0x077f, FPSTATE_FCW(%r8)
	jne 3f
	cmpl $0x3f80, FPSTATE_MXCSR(%r8)
	jne 3f
	movdqu FPSTATE_XMM15(%r8), %xmm0
	pcmpeqb initial_xmm(%rip), %xmm0
	pmovmskb %xmm0, %eax
	cmp $0xffff, %eax
	jne 3f
	movdqu replacement_xmm(%rip), %xmm0
	movdqu %xmm0, FPSTATE_XMM15(%r8)
	movl $0x5f80, FPSTATE_MXCSR(%r8)
	movw $0x037f, FPSTATE_FCW(%r8)
	movl $1, signal_seen(%rip)
3:
	ret
	.size signal_handler, . - signal_handler

	.type exception_handler, @function
exception_handler:
	andq $~0x40000, UCONTEXT_EFL(%rdx)
	cmpl expected_signal(%rip), %edi
	jne .Lexception_fail
	movl SIGINFO_CODE(%rsi), %eax
	cmpl expected_code(%rip), %eax
	jne .Lexception_fail
	movq UCONTEXT_TRAPNO(%rdx), %rax
	cmpq expected_trap(%rip), %rax
	jne .Lexception_fail
	movq expected_error_mask(%rip), %rcx
	test %rcx, %rcx
	jz .Lexception_address
	movq UCONTEXT_ERR(%rdx), %rax
	and %rcx, %rax
	cmpq expected_error(%rip), %rax
	jne .Lexception_fail
.Lexception_address:
	movq expected_address(%rip), %rax
	test %rax, %rax
	jz .Lexception_cr2
	cmpq SIGINFO_ADDR(%rsi), %rax
	jne .Lexception_fail
.Lexception_cr2:
	movq expected_cr2(%rip), %rax
	test %rax, %rax
	jz .Lexception_pkey
	cmpq UCONTEXT_CR2(%rdx), %rax
	jne .Lexception_fail
.Lexception_pkey:
	movl expected_pkey(%rip), %eax
	test %eax, %eax
	jz .Lexception_pass
	cmpl SIGINFO_PKEY(%rsi), %eax
	jne .Lexception_fail
.Lexception_pass:
	movl $1, exception_seen(%rip)
	jmp .Lexception_resume
.Lexception_fail:
	movl $-1, exception_seen(%rip)
.Lexception_resume:
	movq exception_resume(%rip), %rax
	movq %rax, UCONTEXT_RIP(%rdx)
	ret
	.size exception_handler, . - exception_handler

	.type signal_restorer, @function
signal_restorer:
	mov $15, %eax
	syscall
	ud2
	.size signal_restorer, . - signal_restorer

	.section .note.GNU-stack,"",@progbits
