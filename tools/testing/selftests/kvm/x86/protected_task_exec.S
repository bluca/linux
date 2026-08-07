/* SPDX-License-Identifier: GPL-2.0-only */

	.section .rodata
message:
	.ascii "protected task exec\n"
.set message_length, . - message

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
