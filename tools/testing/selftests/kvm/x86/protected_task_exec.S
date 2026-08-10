/* SPDX-License-Identifier: GPL-2.0-only */

.set UCONTEXT_FPSTATE, 224
.set FPSTATE_FCW, 0
.set FPSTATE_MXCSR, 24
.set FPSTATE_XMM15, 400
.set SIGINFO_CODE, 8
.set SIGINFO_ADDR, 16
.set UCONTEXT_RIP, 168
.set UCONTEXT_EFL, 176
.set UCONTEXT_ERR, 192
.set UCONTEXT_TRAPNO, 200
.set UCONTEXT_CR2, 216
.set SA_RESTORER_SIGINFO, 0x04000004
.set ADDRESS_FD, 100
.set READY_FD, 101

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
exception_resume:
	.quad 0
	.balign 4
failure_code:
	.long 1
	.balign 4
thread_tid:
	.long 0
	.balign 4
thread_result:
	.long 0
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

	mov $39, %eax
	syscall
	mov %eax, %r12d
	mov $186, %eax
	syscall
	mov %eax, %r13d

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
	mov $0x310f00, %edi
	lea thread_tid(%rip), %rdx
	lea thread_tid(%rip), %r10
	xor %r8d, %r8d
	mov $56, %eax
	syscall
	test %rax, %rax
	jz 7f
	js 1f
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
	call check_hidden_mapping
	test %eax, %eax
	jne 12f
	call check_replacement_fpu
	test %eax, %eax
	je 15f
12:
	mov failure_code(%rip), %eax
15:
	mov %eax, thread_result(%rip)
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
	jz .Lexception_pass
	cmpq UCONTEXT_CR2(%rdx), %rax
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
