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

	xor %edi, %edi
	jmp 2f
1:
	mov $1, %edi
2:
	mov $231, %eax
	syscall
	ud2
	.size _start, . - _start
