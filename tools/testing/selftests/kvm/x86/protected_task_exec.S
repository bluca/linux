/* SPDX-License-Identifier: GPL-2.0-only */

.set UCONTEXT_FPSTATE, 224
.set FPSTATE_FCW, 0
.set FPSTATE_MXCSR, 24
.set FPSTATE_XMM15, 400
.set SA_RESTORER_SIGINFO, 0x04000004

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

	.balign 8
signal_action:
	.quad signal_handler
	.quad SA_RESTORER_SIGINFO
	.quad signal_restorer
	.quad 0

	.data
	.balign 4
signal_seen:
	.long 0

	.text
	.globl _start
	.type _start, @function
_start:
	mov $1, %eax
	mov $1, %edi
	lea message(%rip), %rsi
	mov $message_length, %edx
	syscall
	cmp $message_length, %rax
	jne 1f

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

	mov $57, %eax
	syscall
	cmp $-95, %rax
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

	xor %edi, %edi
	jmp 2f
1:
	mov $1, %edi
2:
	mov $231, %eax
	syscall
	ud2
	.size _start, . - _start

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

	.type signal_restorer, @function
signal_restorer:
	mov $15, %eax
	syscall
	ud2
	.size signal_restorer, . - signal_restorer

	.section .note.GNU-stack,"",@progbits
