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
.set YMMH15_OFFSET, 240
.set OPMASK7_OFFSET, 56
.set ZMM_HI256_15_OFFSET, 480
.set HI16_ZMM31_OFFSET, 960
.set FP_XSTATE_MAGIC1, 0x46505853
.set FP_XSTATE_MAGIC2, 0x46505845
.set FP_XSTATE_SIZE, 576
.set FP_XSTATE_EXTENDED_SIZE, 580
.set XFEATURE_MASK_FPSSE, 3
.set XFEATURE_MASK_YMM, 4
.set XFEATURE_MASK_AVX512, 0xe0
.set XFEATURE_MASK_PKU, 0x203
.set XFEATURE_MASK_CET_USER, 0x800
.set XFEATURE_MASK_XTILE_CFG, 0x20000
.set XFEATURE_MASK_XTILE_DATA, 0x40000
.set XFEATURE_MASK_XTILE, 0x60000
.set AVX512_OPMASK_SIZE, 64
.set AVX512_ZMM_HI256_SIZE, 512
.set AVX512_HI16_ZMM_SIZE, 1024
.set XTILE_CFG_SIZE, 64
.set XTILE_DATA_SIZE, 8192
.set __NR_map_shadow_stack, 453
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
.set ARCH_SHSTK_DISABLE, 0x5002
.set ARCH_SHSTK_LOCK, 0x5003
.set ARCH_SHSTK_STATUS, 0x5005
.set ARCH_SHSTK_SHSTK, 1
.set ARCH_SHSTK_WRSS, 2
.set SHADOW_STACK_SET_TOKEN, 1
.set SEGV_CPERR, 10
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
.set MAP_PRIVATE_ANONYMOUS, 0x22
.set MAP_GROWSDOWN_PRIVATE_ANONYMOUS, 0x122
.set MAP_FIXED_NOREPLACE_PRIVATE_ANONYMOUS, 0x100022
.set MAP_FIXED_NOREPLACE_GROWSDOWN_PRIVATE_ANONYMOUS, 0x100122

	.section .rodata
message:
	.ascii "protected task exec\n"
.set message_length, . - message
sse3_message:
	.ascii "protected task sse3\n"
.set sse3_message_length, . - sse3_message
pclmulqdq_message:
	.ascii "protected task pclmulqdq\n"
.set pclmulqdq_message_length, . - pclmulqdq_message
ssse3_message:
	.ascii "protected task ssse3\n"
.set ssse3_message_length, . - ssse3_message
sse41_message:
	.ascii "protected task sse4_1\n"
.set sse41_message_length, . - sse41_message
sse42_message:
	.ascii "protected task sse4_2\n"
.set sse42_message_length, . - sse42_message
popcnt_message:
	.ascii "protected task popcnt\n"
.set popcnt_message_length, . - popcnt_message
aes_message:
	.ascii "protected task aes\n"
.set aes_message_length, . - aes_message
shani_message:
	.ascii "protected task sha_ni\n"
.set shani_message_length, . - shani_message
movbe_message:
	.ascii "protected task movbe\n"
.set movbe_message_length, . - movbe_message
rdrand_message:
	.ascii "protected task rdrand\n"
.set rdrand_message_length, . - rdrand_message
bmi1_message:
	.ascii "protected task bmi1\n"
.set bmi1_message_length, . - bmi1_message
bmi2_message:
	.ascii "protected task bmi2\n"
.set bmi2_message_length, . - bmi2_message
rdseed_message:
	.ascii "protected task rdseed\n"
.set rdseed_message_length, . - rdseed_message
adx_message:
	.ascii "protected task adx\n"
.set adx_message_length, . - adx_message
lahf_message:
	.ascii "protected task lahf\n"
.set lahf_message_length, . - lahf_message
abm_message:
	.ascii "protected task abm\n"
.set abm_message_length, . - abm_message
rdpid_message:
	.ascii "protected task rdpid\n"
.set rdpid_message_length, . - rdpid_message
rdtscp_message:
	.ascii "protected task rdtscp\n"
.set rdtscp_message_length, . - rdtscp_message
clflushopt_message:
	.ascii "protected task clflushopt\n"
.set clflushopt_message_length, . - clflushopt_message
clwb_message:
	.ascii "protected task clwb\n"
.set clwb_message_length, . - clwb_message
clzero_message:
	.ascii "protected task clzero\n"
.set clzero_message_length, . - clzero_message
avx_message:
	.ascii "protected task avx\n"
.set avx_message_length, . - avx_message
fma_message:
	.ascii "protected task fma\n"
.set fma_message_length, . - fma_message
f16c_message:
	.ascii "protected task f16c\n"
.set f16c_message_length, . - f16c_message
avx2_message:
	.ascii "protected task avx2\n"
.set avx2_message_length, . - avx2_message
avx512_message:
	.ascii "protected task avx512\n"
.set avx512_message_length, . - avx512_message
avx512dq_message:
	.ascii "protected task avx512dq\n"
.set avx512dq_message_length, . - avx512dq_message
avx512ifma_message:
	.ascii "protected task avx512ifma\n"
.set avx512ifma_message_length, . - avx512ifma_message
avx512pf_message:
	.ascii "protected task avx512pf\n"
.set avx512pf_message_length, . - avx512pf_message
avx512er_message:
	.ascii "protected task avx512er\n"
.set avx512er_message_length, . - avx512er_message
avx512cd_message:
	.ascii "protected task avx512cd\n"
.set avx512cd_message_length, . - avx512cd_message
avx512bw_message:
	.ascii "protected task avx512bw\n"
.set avx512bw_message_length, . - avx512bw_message
avx512vl_message:
	.ascii "protected task avx512vl\n"
.set avx512vl_message_length, . - avx512vl_message
avx512vbmi_message:
	.ascii "protected task avx512vbmi\n"
.set avx512vbmi_message_length, . - avx512vbmi_message
avx512vbmi2_message:
	.ascii "protected task avx512vbmi2\n"
.set avx512vbmi2_message_length, . - avx512vbmi2_message
gfni_message:
	.ascii "protected task gfni\n"
.set gfni_message_length, . - gfni_message
vaes_message:
	.ascii "protected task vaes\n"
.set vaes_message_length, . - vaes_message
vpclmulqdq_message:
	.ascii "protected task vpclmulqdq\n"
.set vpclmulqdq_message_length, . - vpclmulqdq_message
avx512vnni_message:
	.ascii "protected task avx512vnni\n"
.set avx512vnni_message_length, . - avx512vnni_message
avx512bitalg_message:
	.ascii "protected task avx512bitalg\n"
.set avx512bitalg_message_length, . - avx512bitalg_message
avx512vpopcntdq_message:
	.ascii "protected task avx512vpopcntdq\n"
.set avx512vpopcntdq_message_length, . - avx512vpopcntdq_message
avx5124vnniw_message:
	.ascii "protected task avx5124vnniw\n"
.set avx5124vnniw_message_length, . - avx5124vnniw_message
avx5124fmaps_message:
	.ascii "protected task avx5124fmaps\n"
.set avx5124fmaps_message_length, . - avx5124fmaps_message
avx512vp2intersect_message:
	.ascii "protected task avx512vp2intersect\n"
.set avx512vp2intersect_message_length, . - avx512vp2intersect_message
avx512fp16_message:
	.ascii "protected task avx512fp16\n"
.set avx512fp16_message_length, . - avx512fp16_message
avx512bf16_message:
	.ascii "protected task avx512bf16\n"
.set avx512bf16_message_length, . - avx512bf16_message
avxvnni_message:
	.ascii "protected task avxvnni\n"
.set avxvnni_message_length, . - avxvnni_message
avxifma_message:
	.ascii "protected task avxifma\n"
.set avxifma_message_length, . - avxifma_message
avxvnniint8_message:
	.ascii "protected task avxvnniint8\n"
.set avxvnniint8_message_length, . - avxvnniint8_message
avxneconvert_message:
	.ascii "protected task avxneconvert\n"
.set avxneconvert_message_length, . - avxneconvert_message
avxvnniint16_message:
	.ascii "protected task avxvnniint16\n"
.set avxvnniint16_message_length, . - avxvnniint16_message
sha512_message:
	.ascii "protected task sha512\n"
.set sha512_message_length, . - sha512_message
sm3_message:
	.ascii "protected task sm3\n"
.set sm3_message_length, . - sm3_message
sm4_message:
	.ascii "protected task sm4\n"
.set sm4_message_length, . - sm4_message
avx10_v1_message:
	.ascii "protected task avx10_v1\n"
.set avx10_v1_message_length, . - avx10_v1_message
avx10_v2_message:
	.ascii "protected task avx10_v2\n"
.set avx10_v2_message_length, . - avx10_v2_message
avx10_vnni_int_message:
	.ascii "protected task avx10_vnni_int\n"
.set avx10_vnni_int_message_length, . - avx10_vnni_int_message
amx_bf16_message:
	.ascii "protected task amx_bf16\n"
.set amx_bf16_message_length, . - amx_bf16_message
amx_int8_message:
	.ascii "protected task amx_int8\n"
.set amx_int8_message_length, . - amx_int8_message
amx_fp16_message:
	.ascii "protected task amx_fp16\n"
.set amx_fp16_message_length, . - amx_fp16_message
amx_complex_message:
	.ascii "protected task amx_complex\n"
.set amx_complex_message_length, . - amx_complex_message

	.balign 64
initial_zmm15:
initial_ymm:
initial_xmm:
	.quad 0x5566778811223344, 0xddeeff0099aabbcc
initial_ymmh:
	.quad 0x0123456789abcdef, 0xfedcba9876543210
initial_zmm15h:
	.quad 0x1111222233334444, 0x5555666677778888
	.quad 0x9999aaaabbbbcccc, 0xddddeeeeffff0000
	.balign 64
replacement_zmm15:
replacement_ymm:
replacement_xmm:
	.quad 0x76543210fedcba98, 0xdeadbeef0badcafe
replacement_ymmh:
	.quad 0x0f1e2d3c4b5a6978, 0x8796a5b4c3d2e1f0
replacement_zmm15h:
	.quad 0x1021324354657687, 0x98a9bacbdcedfe0f
	.quad 0xf0e0d0c0b0a09080, 0x7060504030201000
	.balign 64
initial_zmm31:
	.quad 0x0011223344556677, 0x8899aabbccddeeff
	.quad 0x123456789abcdef0, 0x0fedcba987654321
	.quad 0x13579bdf2468ace0, 0xfdb97531eca86420
	.quad 0x55aa55aa33cc33cc, 0xaa55aa55cc33cc33
	.balign 64
replacement_zmm31:
	.quad 0xffeeddccbbaa9988, 0x7766554433221100
	.quad 0xfedcba9876543210, 0x0123456789abcdef
	.quad 0xeca86420fdb97531, 0x2468ace013579bdf
	.quad 0xaa55aa55cc33cc33, 0x55aa55aa33cc33cc
	.balign 8
initial_opmask:
	.quad 0xa55a
replacement_opmask:
	.quad 0x5aa5
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
execute_only_code:
	.byte 0xb8
	.long 0x12345678
	.byte 0xc3
.set execute_only_code_length, . - execute_only_code
	.balign 8
fs_marker:
	.quad 0x0123456789abcdef
gs_marker:
	.quad 0xfedcba9876543210
	.balign 8
lam_marker:
	.quad 0x13579bdf2468ace0
	.balign 64
amx_config:
	.byte 1, 0
	.zero 14
	.rept 8
	.word 64
	.endr
	.zero 16
	.rept 8
	.byte 16
	.endr
	.zero 8
	.balign 64
amx_source:
	.fill 1024, 1, 0x5a
	.balign 64
amx_replacement:
	.fill 1024, 1, 0xa5
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
aio_context:
	.quad 0
	.balign 8
io_uring_params:
	.zero 120
	.balign 8
uffdio_api:
	.zero 24
	.balign 8
uffdio_register:
	.zero 32
	.balign 8
uffd_msg:
	.zero 32
	.balign 8
uffdio_copy:
	.zero 40
	.balign 8
uffdio_writeprotect:
	.zero 24
	.balign 8
uffdio_continue:
	.zero 32
	.balign 8
shstk_repair_address:
	.quad 0
shstk_repair_value:
	.quad 0
	.balign 8
exception_resume:
	.quad 0
	.balign 4
failure_code:
	.long 1
	.balign 8
initial_stack:
	.quad 0
	.balign 64
cache_line:
	.zero 64
	.balign 8
affinity_mask:
	.zero 1024
single_cpu_mask:
	.zero 1024
	.balign 4
getcpu_cpu:
	.long 0
getcpu_node:
	.long 0
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
	.balign 8
thread_shstk_address:
	.quad 0
	.balign 4
pku_available:
	.long 0
	.balign 4
sse3_available:
	.long 0
	.balign 4
pclmulqdq_available:
	.long 0
	.balign 4
ssse3_available:
	.long 0
	.balign 4
sse41_available:
	.long 0
	.balign 4
sse42_available:
	.long 0
	.balign 4
popcnt_available:
	.long 0
	.balign 4
aes_available:
	.long 0
	.balign 4
shani_available:
	.long 0
	.balign 4
movbe_available:
	.long 0
	.balign 4
rdrand_available:
	.long 0
	.balign 4
bmi1_available:
	.long 0
	.balign 4
bmi2_available:
	.long 0
	.balign 4
rdseed_available:
	.long 0
	.balign 4
adx_available:
	.long 0
	.balign 4
lahf_available:
	.long 0
	.balign 4
abm_available:
	.long 0
	.balign 4
rdpid_available:
	.long 0
	.balign 4
rdtscp_available:
	.long 0
	.balign 4
clflushopt_available:
	.long 0
	.balign 4
clwb_available:
	.long 0
	.balign 4
clzero_available:
	.long 0
	.balign 4
avx_available:
	.long 0
	.balign 4
fma_available:
	.long 0
	.balign 4
f16c_available:
	.long 0
	.balign 4
avx2_available:
	.long 0
	.balign 4
avx512_available:
	.long 0
	.balign 4
avx512dq_available:
	.long 0
	.balign 4
avx512ifma_available:
	.long 0
	.balign 4
avx512pf_available:
	.long 0
	.balign 4
avx512er_available:
	.long 0
	.balign 4
avx512cd_available:
	.long 0
	.balign 4
avx512bw_available:
	.long 0
	.balign 4
avx512vl_available:
	.long 0
	.balign 4
avx512vbmi_available:
	.long 0
	.balign 4
avx512vbmi2_available:
	.long 0
	.balign 4
gfni_available:
	.long 0
	.balign 4
vaes_available:
	.long 0
	.balign 4
vpclmulqdq_available:
	.long 0
	.balign 4
avx512vnni_available:
	.long 0
	.balign 4
avx512bitalg_available:
	.long 0
	.balign 4
avx512vpopcntdq_available:
	.long 0
	.balign 4
avx5124vnniw_available:
	.long 0
	.balign 4
avx5124fmaps_available:
	.long 0
	.balign 4
avx512vp2intersect_available:
	.long 0
	.balign 4
avx512fp16_available:
	.long 0
	.balign 4
avx512bf16_available:
	.long 0
	.balign 4
avxvnni_available:
	.long 0
	.balign 4
avxifma_available:
	.long 0
	.balign 4
avxvnniint8_available:
	.long 0
	.balign 4
avxneconvert_available:
	.long 0
	.balign 4
avxvnniint16_available:
	.long 0
	.balign 4
sha512_available:
	.long 0
	.balign 4
sm3_available:
	.long 0
	.balign 4
sm4_available:
	.long 0
	.balign 4
avx10_available:
	.long 0
	.balign 4
avx10_version:
	.long 0
	.balign 4
avx10_vnni_int_available:
	.long 0
	.balign 4
amx_bf16_available:
	.long 0
	.balign 4
amx_int8_available:
	.long 0
	.balign 4
amx_fp16_available:
	.long 0
	.balign 4
amx_complex_available:
	.long 0
	.balign 4
leaf7_max_subleaf:
	.long 0
	.balign 4
lam_available:
	.long 0
	.balign 4
shstk_available:
	.long 0
	.balign 4
amx_available:
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
	.balign 4
expected_ymm_offset:
	.long 0
	.balign 4
expected_opmask_offset:
	.long 0
	.balign 4
expected_zmm_hi256_offset:
	.long 0
	.balign 4
expected_hi16_zmm_offset:
	.long 0
	.balign 4
expected_dynamic_xstate_size:
	.long FP_XSTATE_SIZE
	.balign 4
expected_tiledata_offset:
	.long 0
	.balign 8
expected_xfeatures:
	.quad XFEATURE_MASK_FPSSE
	.balign 8
expected_xcomp_supported:
	.quad XFEATURE_MASK_FPSSE
	.balign 64
amx_output:
	.zero 1024
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
	movl $216, failure_code(%rip)
	call check_stack_growth
	test %eax, %eax
	jne 1f
	movl $217, failure_code(%rip)
	call check_growdown_mapping
	test %eax, %eax
	jne 1f
	movl $218, failure_code(%rip)
	call check_growdown_guard
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
	cmpl $0, sse3_available(%rip)
	je .Lcpu_profile_sse3_reported
	movl $172, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea sse3_message(%rip), %rsi
	mov $sse3_message_length, %edx
	syscall
	cmp $sse3_message_length, %rax
	jne 1f
.Lcpu_profile_sse3_reported:
	cmpl $0, pclmulqdq_available(%rip)
	je .Lcpu_profile_pclmulqdq_reported
	movl $173, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea pclmulqdq_message(%rip), %rsi
	mov $pclmulqdq_message_length, %edx
	syscall
	cmp $pclmulqdq_message_length, %rax
	jne 1f
.Lcpu_profile_pclmulqdq_reported:
	cmpl $0, ssse3_available(%rip)
	je .Lcpu_profile_ssse3_reported
	movl $174, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea ssse3_message(%rip), %rsi
	mov $ssse3_message_length, %edx
	syscall
	cmp $ssse3_message_length, %rax
	jne 1f
.Lcpu_profile_ssse3_reported:
	cmpl $0, sse41_available(%rip)
	je .Lcpu_profile_sse41_reported
	movl $175, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea sse41_message(%rip), %rsi
	mov $sse41_message_length, %edx
	syscall
	cmp $sse41_message_length, %rax
	jne 1f
.Lcpu_profile_sse41_reported:
	cmpl $0, sse42_available(%rip)
	je .Lcpu_profile_sse42_reported
	movl $176, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea sse42_message(%rip), %rsi
	mov $sse42_message_length, %edx
	syscall
	cmp $sse42_message_length, %rax
	jne 1f
.Lcpu_profile_sse42_reported:
	cmpl $0, popcnt_available(%rip)
	je .Lcpu_profile_popcnt_reported
	movl $177, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea popcnt_message(%rip), %rsi
	mov $popcnt_message_length, %edx
	syscall
	cmp $popcnt_message_length, %rax
	jne 1f
.Lcpu_profile_popcnt_reported:
	cmpl $0, aes_available(%rip)
	je .Lcpu_profile_aes_reported
	movl $178, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea aes_message(%rip), %rsi
	mov $aes_message_length, %edx
	syscall
	cmp $aes_message_length, %rax
	jne 1f
.Lcpu_profile_aes_reported:
	cmpl $0, shani_available(%rip)
	je .Lcpu_profile_shani_reported
	movl $179, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea shani_message(%rip), %rsi
	mov $shani_message_length, %edx
	syscall
	cmp $shani_message_length, %rax
	jne 1f
.Lcpu_profile_shani_reported:
	cmpl $0, movbe_available(%rip)
	je .Lcpu_profile_movbe_reported
	movl $180, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea movbe_message(%rip), %rsi
	mov $movbe_message_length, %edx
	syscall
	cmp $movbe_message_length, %rax
	jne 1f
.Lcpu_profile_movbe_reported:
	cmpl $0, rdrand_available(%rip)
	je .Lcpu_profile_rdrand_reported
	movl $181, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea rdrand_message(%rip), %rsi
	mov $rdrand_message_length, %edx
	syscall
	cmp $rdrand_message_length, %rax
	jne 1f
.Lcpu_profile_rdrand_reported:
	cmpl $0, bmi1_available(%rip)
	je .Lcpu_profile_bmi1_reported
	movl $182, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea bmi1_message(%rip), %rsi
	mov $bmi1_message_length, %edx
	syscall
	cmp $bmi1_message_length, %rax
	jne 1f
.Lcpu_profile_bmi1_reported:
	cmpl $0, bmi2_available(%rip)
	je .Lcpu_profile_bmi2_reported
	movl $183, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea bmi2_message(%rip), %rsi
	mov $bmi2_message_length, %edx
	syscall
	cmp $bmi2_message_length, %rax
	jne 1f
.Lcpu_profile_bmi2_reported:
	cmpl $0, rdseed_available(%rip)
	je .Lcpu_profile_rdseed_reported
	movl $184, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea rdseed_message(%rip), %rsi
	mov $rdseed_message_length, %edx
	syscall
	cmp $rdseed_message_length, %rax
	jne 1f
.Lcpu_profile_rdseed_reported:
	cmpl $0, adx_available(%rip)
	je .Lcpu_profile_adx_reported
	movl $185, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea adx_message(%rip), %rsi
	mov $adx_message_length, %edx
	syscall
	cmp $adx_message_length, %rax
	jne 1f
.Lcpu_profile_adx_reported:
	cmpl $0, lahf_available(%rip)
	je .Lcpu_profile_lahf_reported
	movl $187, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea lahf_message(%rip), %rsi
	mov $lahf_message_length, %edx
	syscall
	cmp $lahf_message_length, %rax
	jne 1f
.Lcpu_profile_lahf_reported:
	cmpl $0, abm_available(%rip)
	je .Lcpu_profile_abm_reported
	movl $188, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea abm_message(%rip), %rsi
	mov $abm_message_length, %edx
	syscall
	cmp $abm_message_length, %rax
	jne 1f
.Lcpu_profile_abm_reported:
	cmpl $0, rdpid_available(%rip)
	je .Lcpu_profile_rdpid_reported
	movl $205, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea rdpid_message(%rip), %rsi
	mov $rdpid_message_length, %edx
	syscall
	cmp $rdpid_message_length, %rax
	jne 1f
.Lcpu_profile_rdpid_reported:
	cmpl $0, rdtscp_available(%rip)
	je .Lcpu_profile_rdtscp_reported
	movl $206, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea rdtscp_message(%rip), %rsi
	mov $rdtscp_message_length, %edx
	syscall
	cmp $rdtscp_message_length, %rax
	jne 1f
.Lcpu_profile_rdtscp_reported:
	cmpl $0, clflushopt_available(%rip)
	je .Lcpu_profile_clflushopt_reported
	movl $190, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea clflushopt_message(%rip), %rsi
	mov $clflushopt_message_length, %edx
	syscall
	cmp $clflushopt_message_length, %rax
	jne 1f
.Lcpu_profile_clflushopt_reported:
	cmpl $0, clwb_available(%rip)
	je .Lcpu_profile_clwb_reported
	movl $191, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea clwb_message(%rip), %rsi
	mov $clwb_message_length, %edx
	syscall
	cmp $clwb_message_length, %rax
	jne 1f
.Lcpu_profile_clwb_reported:
	cmpl $0, clzero_available(%rip)
	je .Lcpu_profile_clzero_reported
	movl $192, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea clzero_message(%rip), %rsi
	mov $clzero_message_length, %edx
	syscall
	cmp $clzero_message_length, %rax
	jne 1f
.Lcpu_profile_clzero_reported:
	cmpl $0, avx_available(%rip)
	je .Lcpu_profile_avx_reported
	movl $133, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx_message(%rip), %rsi
	mov $avx_message_length, %edx
	syscall
	cmp $avx_message_length, %rax
	jne 1f
.Lcpu_profile_avx_reported:
	cmpl $0, fma_available(%rip)
	je .Lcpu_profile_fma_reported
	movl $135, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea fma_message(%rip), %rsi
	mov $fma_message_length, %edx
	syscall
	cmp $fma_message_length, %rax
	jne 1f
.Lcpu_profile_fma_reported:
	cmpl $0, f16c_available(%rip)
	je .Lcpu_profile_f16c_reported
	movl $136, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea f16c_message(%rip), %rsi
	mov $f16c_message_length, %edx
	syscall
	cmp $f16c_message_length, %rax
	jne 1f
.Lcpu_profile_f16c_reported:
	cmpl $0, avx2_available(%rip)
	je .Lcpu_profile_avx2_reported
	movl $137, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx2_message(%rip), %rsi
	mov $avx2_message_length, %edx
	syscall
	cmp $avx2_message_length, %rax
	jne 1f
.Lcpu_profile_avx2_reported:
	cmpl $0, avx512_available(%rip)
	je .Lcpu_profile_avx512_reported
	movl $138, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512_message(%rip), %rsi
	mov $avx512_message_length, %edx
	syscall
	cmp $avx512_message_length, %rax
	jne 1f
.Lcpu_profile_avx512_reported:
	cmpl $0, avx512dq_available(%rip)
	je .Lcpu_profile_avx512dq_reported
	movl $140, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512dq_message(%rip), %rsi
	mov $avx512dq_message_length, %edx
	syscall
	cmp $avx512dq_message_length, %rax
	jne 1f
.Lcpu_profile_avx512dq_reported:
	cmpl $0, avx512ifma_available(%rip)
	je .Lcpu_profile_avx512ifma_reported
	movl $141, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512ifma_message(%rip), %rsi
	mov $avx512ifma_message_length, %edx
	syscall
	cmp $avx512ifma_message_length, %rax
	jne 1f
.Lcpu_profile_avx512ifma_reported:
	cmpl $0, avx512pf_available(%rip)
	je .Lcpu_profile_avx512pf_reported
	movl $142, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512pf_message(%rip), %rsi
	mov $avx512pf_message_length, %edx
	syscall
	cmp $avx512pf_message_length, %rax
	jne 1f
.Lcpu_profile_avx512pf_reported:
	cmpl $0, avx512er_available(%rip)
	je .Lcpu_profile_avx512er_reported
	movl $143, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512er_message(%rip), %rsi
	mov $avx512er_message_length, %edx
	syscall
	cmp $avx512er_message_length, %rax
	jne 1f
.Lcpu_profile_avx512er_reported:
	cmpl $0, avx512cd_available(%rip)
	je .Lcpu_profile_avx512cd_reported
	movl $144, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512cd_message(%rip), %rsi
	mov $avx512cd_message_length, %edx
	syscall
	cmp $avx512cd_message_length, %rax
	jne 1f
.Lcpu_profile_avx512cd_reported:
	cmpl $0, avx512bw_available(%rip)
	je .Lcpu_profile_avx512bw_reported
	movl $145, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512bw_message(%rip), %rsi
	mov $avx512bw_message_length, %edx
	syscall
	cmp $avx512bw_message_length, %rax
	jne 1f
.Lcpu_profile_avx512bw_reported:
	cmpl $0, avx512vl_available(%rip)
	je .Lcpu_profile_avx512vl_reported
	movl $146, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512vl_message(%rip), %rsi
	mov $avx512vl_message_length, %edx
	syscall
	cmp $avx512vl_message_length, %rax
	jne 1f
.Lcpu_profile_avx512vl_reported:
	cmpl $0, avx512vbmi_available(%rip)
	je .Lcpu_profile_avx512vbmi_reported
	movl $147, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512vbmi_message(%rip), %rsi
	mov $avx512vbmi_message_length, %edx
	syscall
	cmp $avx512vbmi_message_length, %rax
	jne 1f
.Lcpu_profile_avx512vbmi_reported:
	cmpl $0, avx512vbmi2_available(%rip)
	je .Lcpu_profile_avx512vbmi2_reported
	movl $148, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512vbmi2_message(%rip), %rsi
	mov $avx512vbmi2_message_length, %edx
	syscall
	cmp $avx512vbmi2_message_length, %rax
	jne 1f
.Lcpu_profile_avx512vbmi2_reported:
	cmpl $0, gfni_available(%rip)
	je .Lcpu_profile_gfni_reported
	movl $149, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea gfni_message(%rip), %rsi
	mov $gfni_message_length, %edx
	syscall
	cmp $gfni_message_length, %rax
	jne 1f
.Lcpu_profile_gfni_reported:
	cmpl $0, vaes_available(%rip)
	je .Lcpu_profile_vaes_reported
	movl $150, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea vaes_message(%rip), %rsi
	mov $vaes_message_length, %edx
	syscall
	cmp $vaes_message_length, %rax
	jne 1f
.Lcpu_profile_vaes_reported:
	cmpl $0, vpclmulqdq_available(%rip)
	je .Lcpu_profile_vpclmulqdq_reported
	movl $151, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea vpclmulqdq_message(%rip), %rsi
	mov $vpclmulqdq_message_length, %edx
	syscall
	cmp $vpclmulqdq_message_length, %rax
	jne 1f
.Lcpu_profile_vpclmulqdq_reported:
	cmpl $0, avx512vnni_available(%rip)
	je .Lcpu_profile_avx512vnni_reported
	movl $152, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512vnni_message(%rip), %rsi
	mov $avx512vnni_message_length, %edx
	syscall
	cmp $avx512vnni_message_length, %rax
	jne 1f
.Lcpu_profile_avx512vnni_reported:
	cmpl $0, avx512bitalg_available(%rip)
	je .Lcpu_profile_avx512bitalg_reported
	movl $153, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512bitalg_message(%rip), %rsi
	mov $avx512bitalg_message_length, %edx
	syscall
	cmp $avx512bitalg_message_length, %rax
	jne 1f
.Lcpu_profile_avx512bitalg_reported:
	cmpl $0, avx512vpopcntdq_available(%rip)
	je .Lcpu_profile_avx512vpopcntdq_reported
	movl $154, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512vpopcntdq_message(%rip), %rsi
	mov $avx512vpopcntdq_message_length, %edx
	syscall
	cmp $avx512vpopcntdq_message_length, %rax
	jne 1f
.Lcpu_profile_avx512vpopcntdq_reported:
	cmpl $0, avx5124vnniw_available(%rip)
	je .Lcpu_profile_avx5124vnniw_reported
	movl $160, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx5124vnniw_message(%rip), %rsi
	mov $avx5124vnniw_message_length, %edx
	syscall
	cmp $avx5124vnniw_message_length, %rax
	jne 1f
.Lcpu_profile_avx5124vnniw_reported:
	cmpl $0, avx5124fmaps_available(%rip)
	je .Lcpu_profile_avx5124fmaps_reported
	movl $161, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx5124fmaps_message(%rip), %rsi
	mov $avx5124fmaps_message_length, %edx
	syscall
	cmp $avx5124fmaps_message_length, %rax
	jne 1f
.Lcpu_profile_avx5124fmaps_reported:
	cmpl $0, avx512vp2intersect_available(%rip)
	je .Lcpu_profile_avx512vp2intersect_reported
	movl $162, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512vp2intersect_message(%rip), %rsi
	mov $avx512vp2intersect_message_length, %edx
	syscall
	cmp $avx512vp2intersect_message_length, %rax
	jne 1f
.Lcpu_profile_avx512vp2intersect_reported:
	cmpl $0, avx512fp16_available(%rip)
	je .Lcpu_profile_avx512fp16_reported
	movl $163, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512fp16_message(%rip), %rsi
	mov $avx512fp16_message_length, %edx
	syscall
	cmp $avx512fp16_message_length, %rax
	jne 1f
.Lcpu_profile_avx512fp16_reported:
	cmpl $0, avx512bf16_available(%rip)
	je .Lcpu_profile_avx512bf16_reported
	movl $155, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx512bf16_message(%rip), %rsi
	mov $avx512bf16_message_length, %edx
	syscall
	cmp $avx512bf16_message_length, %rax
	jne 1f
.Lcpu_profile_avx512bf16_reported:
	cmpl $0, avxvnni_available(%rip)
	je .Lcpu_profile_avxvnni_reported
	movl $164, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avxvnni_message(%rip), %rsi
	mov $avxvnni_message_length, %edx
	syscall
	cmp $avxvnni_message_length, %rax
	jne 1f
.Lcpu_profile_avxvnni_reported:
	cmpl $0, avxifma_available(%rip)
	je .Lcpu_profile_avxifma_reported
	movl $165, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avxifma_message(%rip), %rsi
	mov $avxifma_message_length, %edx
	syscall
	cmp $avxifma_message_length, %rax
	jne 1f
.Lcpu_profile_avxifma_reported:
	cmpl $0, avxvnniint8_available(%rip)
	je .Lcpu_profile_avxvnniint8_reported
	movl $166, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avxvnniint8_message(%rip), %rsi
	mov $avxvnniint8_message_length, %edx
	syscall
	cmp $avxvnniint8_message_length, %rax
	jne 1f
.Lcpu_profile_avxvnniint8_reported:
	cmpl $0, avxneconvert_available(%rip)
	je .Lcpu_profile_avxneconvert_reported
	movl $167, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avxneconvert_message(%rip), %rsi
	mov $avxneconvert_message_length, %edx
	syscall
	cmp $avxneconvert_message_length, %rax
	jne 1f
.Lcpu_profile_avxneconvert_reported:
	cmpl $0, avxvnniint16_available(%rip)
	je .Lcpu_profile_avxvnniint16_reported
	movl $168, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avxvnniint16_message(%rip), %rsi
	mov $avxvnniint16_message_length, %edx
	syscall
	cmp $avxvnniint16_message_length, %rax
	jne 1f
.Lcpu_profile_avxvnniint16_reported:
	cmpl $0, sha512_available(%rip)
	je .Lcpu_profile_sha512_reported
	movl $169, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea sha512_message(%rip), %rsi
	mov $sha512_message_length, %edx
	syscall
	cmp $sha512_message_length, %rax
	jne 1f
.Lcpu_profile_sha512_reported:
	cmpl $0, sm3_available(%rip)
	je .Lcpu_profile_sm3_reported
	movl $170, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea sm3_message(%rip), %rsi
	mov $sm3_message_length, %edx
	syscall
	cmp $sm3_message_length, %rax
	jne 1f
.Lcpu_profile_sm3_reported:
	cmpl $0, sm4_available(%rip)
	je .Lcpu_profile_sm4_reported
	movl $171, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea sm4_message(%rip), %rsi
	mov $sm4_message_length, %edx
	syscall
	cmp $sm4_message_length, %rax
	jne 1f
.Lcpu_profile_sm4_reported:
	cmpl $1, avx10_version(%rip)
	jne .Lcpu_profile_avx10_v1_reported
	movl $207, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx10_v1_message(%rip), %rsi
	mov $avx10_v1_message_length, %edx
	syscall
	cmp $avx10_v1_message_length, %rax
	jne 1f
.Lcpu_profile_avx10_v1_reported:
	cmpl $2, avx10_version(%rip)
	jne .Lcpu_profile_avx10_v2_reported
	movl $208, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx10_v2_message(%rip), %rsi
	mov $avx10_v2_message_length, %edx
	syscall
	cmp $avx10_v2_message_length, %rax
	jne 1f
.Lcpu_profile_avx10_v2_reported:
	cmpl $0, avx10_vnni_int_available(%rip)
	je .Lcpu_profile_avx10_vnni_int_reported
	movl $209, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea avx10_vnni_int_message(%rip), %rsi
	mov $avx10_vnni_int_message_length, %edx
	syscall
	cmp $avx10_vnni_int_message_length, %rax
	jne 1f
.Lcpu_profile_avx10_vnni_int_reported:
	cmpl $0, amx_bf16_available(%rip)
	je .Lcpu_profile_amx_bf16_reported
	movl $210, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea amx_bf16_message(%rip), %rsi
	mov $amx_bf16_message_length, %edx
	syscall
	cmp $amx_bf16_message_length, %rax
	jne 1f
.Lcpu_profile_amx_bf16_reported:
	cmpl $0, amx_int8_available(%rip)
	je .Lcpu_profile_amx_int8_reported
	movl $211, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea amx_int8_message(%rip), %rsi
	mov $amx_int8_message_length, %edx
	syscall
	cmp $amx_int8_message_length, %rax
	jne 1f
.Lcpu_profile_amx_int8_reported:
	cmpl $0, amx_fp16_available(%rip)
	je .Lcpu_profile_amx_fp16_reported
	movl $212, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea amx_fp16_message(%rip), %rsi
	mov $amx_fp16_message_length, %edx
	syscall
	cmp $amx_fp16_message_length, %rax
	jne 1f
.Lcpu_profile_amx_fp16_reported:
	cmpl $0, amx_complex_available(%rip)
	je .Lcpu_profile_amx_complex_reported
	movl $213, failure_code(%rip)
	mov $1, %eax
	mov $1, %edi
	lea amx_complex_message(%rip), %rsi
	mov $amx_complex_message_length, %edx
	syscall
	cmp $amx_complex_message_length, %rax
	jne 1f
.Lcpu_profile_amx_complex_reported:
	movl $124, failure_code(%rip)
	call check_initial_shstk
	test %eax, %eax
	jne 1f
	cmpl $0, shstk_available(%rip)
	je .Lshstk_enabled
	mov $158, %eax
	mov $ARCH_SHSTK_ENABLE, %edi
	mov $ARCH_SHSTK_SHSTK, %esi
	syscall
	test %rax, %rax
	jne 1f
.Lshstk_enabled:
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
	movl $129, failure_code(%rip)
	call check_amx_without_permission
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

	movl $82, failure_code(%rip)
	call check_signal_roundtrip
	test %eax, %eax
	jne 1f
	movl $130, failure_code(%rip)
	call request_amx_permission
	test %eax, %eax
	jne 1f
	call check_signal_roundtrip
	test %eax, %eax
	jne 1f
	movl $131, failure_code(%rip)
	call enable_amx_tiledata
	test %eax, %eax
	jne 1f
	mov $39, %eax
	syscall
	lea amx_source(%rip), %rdi
	call check_amx_tiledata
	test %eax, %eax
	jne 1f
	movl $132, failure_code(%rip)
	call check_signal_roundtrip
	test %eax, %eax
	jne 1f
	mov $39, %eax
	syscall
	lea amx_replacement(%rip), %rdi
	call check_amx_tiledata
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
	movl $221, failure_code(%rip)
	call check_truncated_file_fault
	test %eax, %eax
	jne 1f
	movl $222, failure_code(%rip)
	call check_aio_ring
	test %eax, %eax
	jne 1f
	movl $223, failure_code(%rip)
	call check_io_uring_mappings
	test %eax, %eax
	jne 1f
	movl $224, failure_code(%rip)
	call check_userfaultfd_sigbus
	test %eax, %eax
	jne 1f
	movl $226, failure_code(%rip)
	call check_userfaultfd_event
	test %eax, %eax
	jne 1f
	movl $228, failure_code(%rip)
	call check_userfaultfd_minor
	test %eax, %eax
	jne 1f
	movl $225, failure_code(%rip)
	call check_sysv_shared_memory
	test %eax, %eax
	jne 1f
	movl $227, failure_code(%rip)
	call check_hugetlb_mapping
	test %eax, %eax
	jne 1f
	movl $229, failure_code(%rip)
	call check_write_only_mapping
	test %eax, %eax
	jne 1f
	movl $230, failure_code(%rip)
	call check_execute_only_mapping
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
	call set_amx_dynamic_expectations
	lea amx_replacement(%rip), %rdi
	call check_amx_tiledata
	test %eax, %eax
	jne 1f

	movl $21, failure_code(%rip)
	mov $58, %eax
	syscall
	call check_process_clone
	test %eax, %eax
	jne 1f
	call set_amx_dynamic_expectations
	lea amx_replacement(%rip), %rdi
	call check_amx_tiledata
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
	call set_amx_dynamic_expectations
	lea amx_replacement(%rip), %rdi
	call check_amx_tiledata
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
	call check_released_thread_shstk
	test %eax, %eax
	jne 1f
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
	movl $220, failure_code(%rip)
	call check_growdown_mapping
	test %eax, %eax
	jne 15f
	call check_amx_caller_saved
	test %eax, %eax
	jne 12f
	call check_lam_enabled
	test %eax, %eax
	jne 12f
	call check_shstk_enabled
	test %eax, %eax
	jne 12f
	cmpl $0, shstk_available(%rip)
	je .Lthread_shstk_recorded
	xor %eax, %eax
	rdsspq %rax
	sub $8, %rax
	and $-4096, %rax
	mov %rax, thread_shstk_address(%rip)
.Lthread_shstk_recorded:
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
	call set_amx_dynamic_expectations
	lea amx_replacement(%rip), %rdi
	call check_amx_tiledata
	test %eax, %eax
	jne 1f
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

	.type check_stack_growth, @function
check_stack_growth:
	push %rbp
	mov %rsp, %rbp
	mov $512, %ecx
.Lstack_growth:
	sub $4096, %rsp
	movb $0, (%rsp)
	dec %ecx
	jne .Lstack_growth
	mov %rbp, %rsp
	pop %rbp
	xor %eax, %eax
	ret
	.size check_stack_growth, . - check_stack_growth

	.type check_growdown_mapping, @function
check_growdown_mapping:
	push %r12
	push %r13
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $MAP_GROWSDOWN_PRIVATE_ANONYMOUS, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Lgrowdown_mapping_fail
	mov %rax, %r12
	mov %rax, %r13
	mov $64, %ecx
.Lgrowdown_mapping_expand:
	sub $4096, %r13
	movb %cl, (%r13)
	dec %ecx
	jne .Lgrowdown_mapping_expand
	mov $11, %eax
	mov %r13, %rdi
	mov $266240, %esi
	syscall
	test %rax, %rax
	jne .Lgrowdown_mapping_fail
	pop %r13
	pop %r12
	xor %eax, %eax
	ret
.Lgrowdown_mapping_fail:
	pop %r13
	pop %r12
	mov $1, %eax
	ret
	.size check_growdown_mapping, . - check_growdown_mapping

	.type check_growdown_guard, @function
check_growdown_guard:
	push %r12
	push %r13
	push %r14
	mov $9, %eax
	xor %edi, %edi
	mov $16384, %esi
	xor %edx, %edx
	mov $MAP_PRIVATE_ANONYMOUS, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Lgrowdown_guard_fail
	mov %rax, %r12
	mov $11, %eax
	mov %r12, %rdi
	mov $16384, %esi
	syscall
	test %rax, %rax
	jne .Lgrowdown_guard_fail
	mov $9, %eax
	mov %r12, %rdi
	mov $4096, %esi
	mov $3, %edx
	mov $MAP_FIXED_NOREPLACE_PRIVATE_ANONYMOUS, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp %r12, %rax
	jne .Lgrowdown_guard_fail
	lea 12288(%r12), %r13
	mov $9, %eax
	mov %r13, %rdi
	mov $4096, %esi
	mov $3, %edx
	mov $MAP_FIXED_NOREPLACE_GROWSDOWN_PRIVATE_ANONYMOUS, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp %r13, %rax
	jne .Lgrowdown_guard_upper_fail
	lea 8192(%r12), %r14
	mov $11, %edi
	mov $1, %esi
	mov $14, %edx
	mov %r14, %rcx
	lea .Lgrowdown_guard_resume(%rip), %r8
	mov %r14, %r9
	call prepare_exception
	movq $7, expected_error_mask(%rip)
	movq $4, expected_error(%rip)
	movb $1, (%r14)
.Lgrowdown_guard_resume:
	call check_exception
	mov %eax, %r14d
	mov $11, %eax
	mov %r13, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	je .Lgrowdown_guard_cleanup_lower
.Lgrowdown_guard_upper_fail:
	mov $1, %r14d
.Lgrowdown_guard_cleanup_lower:
	mov $11, %eax
	mov %r12, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	je .Lgrowdown_guard_done
	mov $1, %r14d
.Lgrowdown_guard_done:
	mov %r14d, %eax
	pop %r14
	pop %r13
	pop %r12
	ret
.Lgrowdown_guard_fail:
	mov $1, %r14d
	jmp .Lgrowdown_guard_done
	.size check_growdown_guard, . - check_growdown_guard

	.type check_truncated_file_fault, @function
check_truncated_file_fault:
	push %r12
	push %r13
	push %r14
	mov $-1, %r12
	xor %r13d, %r13d
	mov $319, %eax
	lea security_memfd_name(%rip), %rdi
	mov $1, %esi
	syscall
	cmp $-4095, %rax
	jae .Ltruncated_file_fail
	mov %rax, %r12
	mov $77, %eax
	mov %r12d, %edi
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne .Ltruncated_file_fail
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $1, %r10d
	mov %r12d, %r8d
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Ltruncated_file_fail
	mov %rax, %r13
	movb $0x3c, (%r13)
	cmpb $0x3c, (%r13)
	jne .Ltruncated_file_fail
	mov $77, %eax
	mov %r12d, %edi
	xor %esi, %esi
	syscall
	test %rax, %rax
	jne .Ltruncated_file_fail
	mov $7, %edi
	mov $2, %esi
	mov $14, %edx
	mov %r13, %rcx
	lea .Ltruncated_file_resume(%rip), %r8
	mov %r13, %r9
	call prepare_exception
	movq $7, expected_error_mask(%rip)
	movq $4, expected_error(%rip)
	movb (%r13), %al
.Ltruncated_file_resume:
	call check_exception
	test %eax, %eax
	jne .Ltruncated_file_fail
	mov $77, %eax
	mov %r12d, %edi
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne .Ltruncated_file_fail
	movb $0xa5, (%r13)
	cmpb $0xa5, (%r13)
	jne .Ltruncated_file_fail
	xor %r14d, %r14d
	jmp .Ltruncated_file_cleanup
.Ltruncated_file_fail:
	mov $1, %r14d
.Ltruncated_file_cleanup:
	test %r13, %r13
	jz .Ltruncated_file_close
	mov $11, %eax
	mov %r13, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	je .Ltruncated_file_close
	mov $1, %r14d
.Ltruncated_file_close:
	cmp $0, %r12
	jl .Ltruncated_file_done
	mov $3, %eax
	mov %r12d, %edi
	syscall
	test %rax, %rax
	je .Ltruncated_file_done
	mov $1, %r14d
.Ltruncated_file_done:
	mov %r14d, %eax
	pop %r14
	pop %r13
	pop %r12
	ret
	.size check_truncated_file_fault, . - check_truncated_file_fault

	.type check_aio_ring, @function
check_aio_ring:
	push %r12
	movq $0, aio_context(%rip)
	mov $206, %eax
	mov $2, %edi
	lea aio_context(%rip), %rsi
	syscall
	test %rax, %rax
	jne .Laio_ring_fail
	mov aio_context(%rip), %r12
	test %r12, %r12
	jz .Laio_ring_fail
	cmpl $0xa10a10a1, 16(%r12)
	jne .Laio_ring_destroy_fail
	cmpl $1, 20(%r12)
	jne .Laio_ring_destroy_fail
	cmpl $0, 24(%r12)
	jne .Laio_ring_destroy_fail
	cmpl $32, 28(%r12)
	jne .Laio_ring_destroy_fail
	cmpl $0, 8(%r12)
	jne .Laio_ring_destroy_fail
	cmpl $0, 12(%r12)
	jne .Laio_ring_destroy_fail
	cmpl $2, 4(%r12)
	jbe .Laio_ring_destroy_fail
	mov $207, %eax
	mov %r12, %rdi
	syscall
	test %rax, %rax
	jne .Laio_ring_fail
	movq $0, aio_context(%rip)
	mov $11, %edi
	mov $1, %esi
	mov $14, %edx
	mov %r12, %rcx
	lea .Laio_ring_resume(%rip), %r8
	mov %r12, %r9
	call prepare_exception
	movq $7, expected_error_mask(%rip)
	movq $4, expected_error(%rip)
	mov (%r12), %eax
.Laio_ring_resume:
	call check_exception
	pop %r12
	ret
.Laio_ring_destroy_fail:
	mov $207, %eax
	mov %r12, %rdi
	syscall
.Laio_ring_fail:
	pop %r12
	mov $1, %eax
	ret
	.size check_aio_ring, . - check_aio_ring

	.type check_io_uring_mappings, @function
check_io_uring_mappings:
	push %r12
	push %r13
	push %r14
	push %r15
	mov $-1, %r12
	xor %r13d, %r13d
	xor %r14d, %r14d
	lea io_uring_params(%rip), %rdi
	xor %eax, %eax
	mov $15, %ecx
	rep stosq
	mov $425, %eax
	mov $2, %edi
	lea io_uring_params(%rip), %rsi
	syscall
	cmp $-4095, %rax
	jae .Lio_uring_fail
	mov %rax, %r12
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $1, %r10d
	mov %r12d, %r8d
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Lio_uring_fail
	mov %rax, %r13
	mov 52+io_uring_params(%rip), %ecx
	mov (%r13,%rcx), %edx
	cmp io_uring_params(%rip), %edx
	jne .Lio_uring_fail
	mov 48+io_uring_params(%rip), %ecx
	mov (%r13,%rcx), %edx
	mov io_uring_params(%rip), %r15d
	dec %r15d
	cmp %r15d, %edx
	jne .Lio_uring_fail
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $1, %r10d
	mov %r12d, %r8d
	mov $0x10000000, %r9d
	syscall
	cmp $-4095, %rax
	jae .Lio_uring_fail
	mov %rax, %r14
	movabs $0x0123456789abcdef, %rax
	mov %rax, (%r14)
	cmp %rax, (%r14)
	jne .Lio_uring_fail
	mov $3, %eax
	mov %r12d, %edi
	syscall
	test %rax, %rax
	jne .Lio_uring_fail
	mov $-1, %r12
	mov 52+io_uring_params(%rip), %ecx
	mov (%r13,%rcx), %edx
	cmp io_uring_params(%rip), %edx
	jne .Lio_uring_fail
	movabs $0x0123456789abcdef, %rax
	cmp %rax, (%r14)
	jne .Lio_uring_fail
	mov $11, %eax
	mov %r14, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne .Lio_uring_fail
	xor %r14d, %r14d
	mov $11, %eax
	mov %r13, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne .Lio_uring_fail
	mov $11, %edi
	mov $1, %esi
	mov $14, %edx
	mov %r13, %rcx
	lea .Lio_uring_resume(%rip), %r8
	mov %r13, %r9
	call prepare_exception
	movq $7, expected_error_mask(%rip)
	movq $4, expected_error(%rip)
	mov (%r13), %eax
.Lio_uring_resume:
	call check_exception
	mov %eax, %r15d
	xor %r13d, %r13d
	jmp .Lio_uring_cleanup
.Lio_uring_fail:
	mov $1, %r15d
.Lio_uring_cleanup:
	test %r14, %r14
	jz .Lio_uring_cleanup_ring
	mov $11, %eax
	mov %r14, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	je .Lio_uring_cleanup_ring
	mov $1, %r15d
.Lio_uring_cleanup_ring:
	test %r13, %r13
	jz .Lio_uring_close
	mov $11, %eax
	mov %r13, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	je .Lio_uring_close
	mov $1, %r15d
.Lio_uring_close:
	cmp $0, %r12
	jl .Lio_uring_done
	mov $3, %eax
	mov %r12d, %edi
	syscall
	test %rax, %rax
	je .Lio_uring_done
	mov $1, %r15d
.Lio_uring_done:
	mov %r15d, %eax
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	ret
	.size check_io_uring_mappings, . - check_io_uring_mappings

	.type check_userfaultfd_sigbus, @function
check_userfaultfd_sigbus:
	push %r12
	push %r13
	push %r14
	mov $-1, %r12
	xor %r13d, %r13d
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $MAP_PRIVATE_ANONYMOUS, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Luserfaultfd_fail
	mov %rax, %r13
	mov $323, %eax
	mov $0x80801, %edi
	syscall
	cmp $-4095, %rax
	jae .Luserfaultfd_fail
	mov %rax, %r12
	movq $0xaa, uffdio_api(%rip)
	movq $0x80, 8+uffdio_api(%rip)
	movq $0, 16+uffdio_api(%rip)
	mov $16, %eax
	mov %r12d, %edi
	mov $0xc018aa3f, %esi
	lea uffdio_api(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Luserfaultfd_fail
	cmpq $0xaa, uffdio_api(%rip)
	jne .Luserfaultfd_fail
	testq $0x80, 8+uffdio_api(%rip)
	jz .Luserfaultfd_fail
	mov %r13, uffdio_register(%rip)
	movq $4096, 8+uffdio_register(%rip)
	movq $1, 16+uffdio_register(%rip)
	movq $0, 24+uffdio_register(%rip)
	mov $16, %eax
	mov %r12d, %edi
	mov $0xc020aa00, %esi
	lea uffdio_register(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Luserfaultfd_fail
	mov $7, %edi
	mov $2, %esi
	mov $14, %edx
	mov %r13, %rcx
	lea .Luserfaultfd_resume(%rip), %r8
	mov %r13, %r9
	call prepare_exception
	movq $7, expected_error_mask(%rip)
	movq $4, expected_error(%rip)
	mov (%r13), %eax
.Luserfaultfd_resume:
	call check_exception
	test %eax, %eax
	jne .Luserfaultfd_fail
	mov $3, %eax
	mov %r12d, %edi
	syscall
	test %rax, %rax
	jne .Luserfaultfd_fail
	mov $-1, %r12
	movb $0x5a, (%r13)
	cmpb $0x5a, (%r13)
	jne .Luserfaultfd_fail
	xor %r14d, %r14d
	jmp .Luserfaultfd_cleanup
.Luserfaultfd_fail:
	mov $1, %r14d
.Luserfaultfd_cleanup:
	cmp $0, %r12
	jl .Luserfaultfd_unmap
	mov $3, %eax
	mov %r12d, %edi
	syscall
	test %rax, %rax
	je .Luserfaultfd_unmap
	mov $1, %r14d
.Luserfaultfd_unmap:
	test %r13, %r13
	jz .Luserfaultfd_done
	mov $11, %eax
	mov %r13, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	je .Luserfaultfd_done
	mov $1, %r14d
.Luserfaultfd_done:
	mov %r14d, %eax
	pop %r14
	pop %r13
	pop %r12
	ret
	.size check_userfaultfd_sigbus, . - check_userfaultfd_sigbus

	.type check_userfaultfd_event, @function
check_userfaultfd_event:
	push %r12
	push %r13
	push %r14
	push %r15
	sub $16, %rsp
	mov $-1, %r12
	xor %r13d, %r13d
	xor %r14d, %r14d
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $MAP_PRIVATE_ANONYMOUS, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Luserfaultfd_event_fail
	mov %rax, %r13
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $MAP_PRIVATE_ANONYMOUS, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Luserfaultfd_event_fail
	mov %rax, %r14
	movb $0x6b, (%r14)
	mov $323, %eax
	mov $0x80001, %edi
	syscall
	cmp $-4095, %rax
	jae .Luserfaultfd_event_fail
	mov %rax, %r12
	movq $0xaa, uffdio_api(%rip)
	movq $1, 8+uffdio_api(%rip)
	movq $0, 16+uffdio_api(%rip)
	mov $16, %eax
	mov %r12d, %edi
	mov $0xc018aa3f, %esi
	lea uffdio_api(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Luserfaultfd_event_fail
	testq $1, 8+uffdio_api(%rip)
	jz .Luserfaultfd_event_fail
	mov %r13, uffdio_register(%rip)
	movq $4096, 8+uffdio_register(%rip)
	movq $3, 16+uffdio_register(%rip)
	movq $0, 24+uffdio_register(%rip)
	mov $16, %eax
	mov %r12d, %edi
	mov $0xc020aa00, %esi
	lea uffdio_register(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Luserfaultfd_event_fail
	mov $57, %eax
	syscall
	test %rax, %rax
	jz .Luserfaultfd_event_child
	js .Luserfaultfd_event_fail
	mov %rax, %r15
	movb (%r13), %al
	cmpb $0x6b, %al
	sete 8(%rsp)
	mov %r15, %rdi
	mov %rsp, %rsi
	xor %edx, %edx
	xor %r10d, %r10d
	mov $61, %eax
	syscall
	cmp %r15, %rax
	jne .Luserfaultfd_event_parent_fail
	cmpl $0, (%rsp)
	jne .Luserfaultfd_event_parent_fail
	cmpb $0, 8(%rsp)
	jz .Luserfaultfd_event_parent_fail
	mov %r13, uffdio_writeprotect(%rip)
	movq $4096, 8+uffdio_writeprotect(%rip)
	movq $1, 16+uffdio_writeprotect(%rip)
	mov $16, %eax
	mov %r12d, %edi
	mov $0xc018aa06, %esi
	lea uffdio_writeprotect(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Luserfaultfd_event_parent_fail
	mov $57, %eax
	syscall
	test %rax, %rax
	jz .Luserfaultfd_wp_child
	js .Luserfaultfd_event_parent_fail
	mov %rax, %r15
	movb $0x7c, (%r13)
	cmpb $0x7c, (%r13)
	sete 8(%rsp)
	mov %r15, %rdi
	mov %rsp, %rsi
	xor %edx, %edx
	xor %r10d, %r10d
	mov $61, %eax
	syscall
	cmp %r15, %rax
	jne .Luserfaultfd_event_parent_fail
	cmpl $0, (%rsp)
	jne .Luserfaultfd_event_parent_fail
	cmpb $0, 8(%rsp)
	jz .Luserfaultfd_event_parent_fail
	xor %r15d, %r15d
	jmp .Luserfaultfd_event_cleanup
.Luserfaultfd_event_parent_fail:
	mov $1, %r15d
	jmp .Luserfaultfd_event_cleanup
.Luserfaultfd_event_child:
	mov $0, %eax
	mov %r12d, %edi
	lea uffd_msg(%rip), %rsi
	mov $32, %edx
	syscall
	cmp $32, %rax
	jne .Luserfaultfd_event_child_fail
	cmpb $0x12, uffd_msg(%rip)
	jne .Luserfaultfd_event_child_fail
	cmp 16+uffd_msg(%rip), %r13
	jne .Luserfaultfd_event_child_fail
	mov %r13, uffdio_copy(%rip)
	mov %r14, 8+uffdio_copy(%rip)
	movq $4096, 16+uffdio_copy(%rip)
	movq $0, 24+uffdio_copy(%rip)
	movq $0, 32+uffdio_copy(%rip)
	mov $16, %eax
	mov %r12d, %edi
	mov $0xc028aa03, %esi
	lea uffdio_copy(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Luserfaultfd_event_child_fail
	cmpq $4096, 32+uffdio_copy(%rip)
	jne .Luserfaultfd_event_child_fail
	xor %edi, %edi
	jmp .Luserfaultfd_event_child_exit
.Luserfaultfd_event_child_fail:
	mov $1, %edi
.Luserfaultfd_event_child_exit:
	mov $60, %eax
	syscall
	ud2
.Luserfaultfd_wp_child:
	mov $0, %eax
	mov %r12d, %edi
	lea uffd_msg(%rip), %rsi
	mov $32, %edx
	syscall
	cmp $32, %rax
	jne .Luserfaultfd_event_child_fail
	cmpb $0x12, uffd_msg(%rip)
	jne .Luserfaultfd_event_child_fail
	mov 8+uffd_msg(%rip), %rax
	and $3, %eax
	cmp $3, %eax
	jne .Luserfaultfd_event_child_fail
	cmp 16+uffd_msg(%rip), %r13
	jne .Luserfaultfd_event_child_fail
	mov %r13, uffdio_writeprotect(%rip)
	movq $4096, 8+uffdio_writeprotect(%rip)
	movq $0, 16+uffdio_writeprotect(%rip)
	mov $16, %eax
	mov %r12d, %edi
	mov $0xc018aa06, %esi
	lea uffdio_writeprotect(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Luserfaultfd_event_child_fail
	xor %edi, %edi
	jmp .Luserfaultfd_event_child_exit
.Luserfaultfd_event_fail:
	mov $1, %r15d
.Luserfaultfd_event_cleanup:
	cmp $0, %r12
	jl .Luserfaultfd_event_unmap_source
	mov $3, %eax
	mov %r12d, %edi
	syscall
	test %rax, %rax
	je .Luserfaultfd_event_unmap_source
	mov $1, %r15d
.Luserfaultfd_event_unmap_source:
	test %r14, %r14
	jz .Luserfaultfd_event_unmap_target
	mov $11, %eax
	mov %r14, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	je .Luserfaultfd_event_unmap_target
	mov $1, %r15d
.Luserfaultfd_event_unmap_target:
	test %r13, %r13
	jz .Luserfaultfd_event_done
	mov $11, %eax
	mov %r13, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	je .Luserfaultfd_event_done
	mov $1, %r15d
.Luserfaultfd_event_done:
	mov %r15d, %eax
	add $16, %rsp
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	ret
	.size check_userfaultfd_event, . - check_userfaultfd_event

	.type check_userfaultfd_minor, @function
check_userfaultfd_minor:
	push %r12
	push %r13
	push %r14
	push %r15
	sub $16, %rsp
	mov $-1, %r12
	mov $-1, %r13
	xor %r14d, %r14d
	mov $319, %eax
	lea security_memfd_name(%rip), %rdi
	mov $1, %esi
	syscall
	cmp $-4095, %rax
	jae .Luserfaultfd_minor_fail
	mov %rax, %r13
	mov $77, %eax
	mov %r13d, %edi
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne .Luserfaultfd_minor_fail
	mov $18, %eax
	mov %r13d, %edi
	lea syscall_initial_byte(%rip), %rsi
	mov $1, %edx
	xor %r10d, %r10d
	syscall
	cmp $1, %rax
	jne .Luserfaultfd_minor_fail
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $3, %edx
	mov $1, %r10d
	mov %r13d, %r8d
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Luserfaultfd_minor_fail
	mov %rax, %r14
	mov $323, %eax
	mov $0x80001, %edi
	syscall
	cmp $-4095, %rax
	jae .Luserfaultfd_minor_fail
	mov %rax, %r12
	movq $0xaa, uffdio_api(%rip)
	movq $0, 8+uffdio_api(%rip)
	movq $0, 16+uffdio_api(%rip)
	mov $16, %eax
	mov %r12d, %edi
	mov $0xc018aa3f, %esi
	lea uffdio_api(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Luserfaultfd_minor_fail
	testq $0x400, 8+uffdio_api(%rip)
	jz .Luserfaultfd_minor_skip
	mov %r14, uffdio_register(%rip)
	movq $4096, 8+uffdio_register(%rip)
	movq $4, 16+uffdio_register(%rip)
	movq $0, 24+uffdio_register(%rip)
	mov $16, %eax
	mov %r12d, %edi
	mov $0xc020aa00, %esi
	lea uffdio_register(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Luserfaultfd_minor_fail
	mov $57, %eax
	syscall
	test %rax, %rax
	jz .Luserfaultfd_minor_child
	js .Luserfaultfd_minor_fail
	mov %rax, %r15
	movb (%r14), %al
	cmpb $0x3c, %al
	sete 8(%rsp)
	mov %r15, %rdi
	mov %rsp, %rsi
	xor %edx, %edx
	xor %r10d, %r10d
	mov $61, %eax
	syscall
	cmp %r15, %rax
	jne .Luserfaultfd_minor_parent_fail
	cmpl $0, (%rsp)
	jne .Luserfaultfd_minor_parent_fail
	cmpb $0, 8(%rsp)
	jz .Luserfaultfd_minor_parent_fail
.Luserfaultfd_minor_skip:
	xor %r15d, %r15d
	jmp .Luserfaultfd_minor_cleanup
.Luserfaultfd_minor_parent_fail:
	mov $1, %r15d
	jmp .Luserfaultfd_minor_cleanup
.Luserfaultfd_minor_child:
	mov $0, %eax
	mov %r12d, %edi
	lea uffd_msg(%rip), %rsi
	mov $32, %edx
	syscall
	cmp $32, %rax
	jne .Luserfaultfd_minor_child_fail
	cmpb $0x12, uffd_msg(%rip)
	jne .Luserfaultfd_minor_child_fail
	mov 8+uffd_msg(%rip), %rax
	test $4, %eax
	jz .Luserfaultfd_minor_child_fail
	cmp 16+uffd_msg(%rip), %r14
	jne .Luserfaultfd_minor_child_fail
	mov %r14, uffdio_continue(%rip)
	movq $4096, 8+uffdio_continue(%rip)
	movq $0, 16+uffdio_continue(%rip)
	movq $0, 24+uffdio_continue(%rip)
	mov $16, %eax
	mov %r12d, %edi
	mov $0xc020aa07, %esi
	lea uffdio_continue(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Luserfaultfd_minor_child_fail
	cmpq $4096, 24+uffdio_continue(%rip)
	jne .Luserfaultfd_minor_child_fail
	xor %edi, %edi
	jmp .Luserfaultfd_minor_child_exit
.Luserfaultfd_minor_child_fail:
	mov $1, %edi
.Luserfaultfd_minor_child_exit:
	mov $60, %eax
	syscall
	ud2
.Luserfaultfd_minor_fail:
	mov $1, %r15d
.Luserfaultfd_minor_cleanup:
	cmp $0, %r12
	jl .Luserfaultfd_minor_unmap
	mov $3, %eax
	mov %r12d, %edi
	syscall
	test %rax, %rax
	je .Luserfaultfd_minor_unmap
	mov $1, %r15d
.Luserfaultfd_minor_unmap:
	test %r14, %r14
	jz .Luserfaultfd_minor_close_memfd
	mov $11, %eax
	mov %r14, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	je .Luserfaultfd_minor_close_memfd
	mov $1, %r15d
.Luserfaultfd_minor_close_memfd:
	cmp $0, %r13
	jl .Luserfaultfd_minor_done
	mov $3, %eax
	mov %r13d, %edi
	syscall
	test %rax, %rax
	je .Luserfaultfd_minor_done
	mov $1, %r15d
.Luserfaultfd_minor_done:
	mov %r15d, %eax
	add $16, %rsp
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	ret
	.size check_userfaultfd_minor, . - check_userfaultfd_minor

	.type check_sysv_shared_memory, @function
check_sysv_shared_memory:
	push %r12
	push %r13
	push %r14
	mov $-1, %r12
	xor %r13d, %r13d
	mov $29, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $0x380, %edx
	syscall
	test %rax, %rax
	js .Lsysv_shm_fail
	mov %rax, %r12
	mov $30, %eax
	mov %r12d, %edi
	xor %esi, %esi
	xor %edx, %edx
	syscall
	cmp $-4095, %rax
	jae .Lsysv_shm_fail
	mov %rax, %r13
	movabs $0xfedcba9876543210, %rax
	mov %rax, (%r13)
	cmp %rax, (%r13)
	jne .Lsysv_shm_fail
	mov $67, %eax
	mov %r13, %rdi
	syscall
	test %rax, %rax
	jne .Lsysv_shm_fail
	mov $11, %edi
	mov $1, %esi
	mov $14, %edx
	mov %r13, %rcx
	lea .Lsysv_shm_resume(%rip), %r8
	mov %r13, %r9
	call prepare_exception
	movq $7, expected_error_mask(%rip)
	movq $4, expected_error(%rip)
	mov (%r13), %rax
.Lsysv_shm_resume:
	call check_exception
	mov %eax, %r14d
	xor %r13d, %r13d
	jmp .Lsysv_shm_remove
.Lsysv_shm_fail:
	mov $1, %r14d
	test %r13, %r13
	jz .Lsysv_shm_remove
	mov $67, %eax
	mov %r13, %rdi
	syscall
	test %rax, %rax
	je .Lsysv_shm_remove
	mov $1, %r14d
.Lsysv_shm_remove:
	cmp $0, %r12
	jl .Lsysv_shm_done
	mov $31, %eax
	mov %r12d, %edi
	xor %esi, %esi
	xor %edx, %edx
	syscall
	test %rax, %rax
	je .Lsysv_shm_done
	mov $1, %r14d
.Lsysv_shm_done:
	mov %r14d, %eax
	pop %r14
	pop %r13
	pop %r12
	ret
	.size check_sysv_shared_memory, . - check_sysv_shared_memory

	.type check_hugetlb_mapping, @function
check_hugetlb_mapping:
	push %r12
	mov $9, %eax
	xor %edi, %edi
	mov $0x200000, %esi
	mov $3, %edx
	mov $0x40022, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-12, %rax
	je .Lhugetlb_skip
	cmp $-4095, %rax
	jae .Lhugetlb_fail
	mov %rax, %r12
	movb $0x34, (%r12)
	movb $0x78, 0x1fffff(%r12)
	cmpb $0x34, (%r12)
	jne .Lhugetlb_cleanup_fail
	cmpb $0x78, 0x1fffff(%r12)
	jne .Lhugetlb_cleanup_fail
	mov $11, %eax
	mov %r12, %rdi
	mov $0x200000, %esi
	syscall
	test %rax, %rax
	jne .Lhugetlb_fail
	mov $11, %edi
	mov $1, %esi
	mov $14, %edx
	mov %r12, %rcx
	lea .Lhugetlb_resume(%rip), %r8
	mov %r12, %r9
	call prepare_exception
	movq $7, expected_error_mask(%rip)
	movq $4, expected_error(%rip)
	mov (%r12), %eax
.Lhugetlb_resume:
	call check_exception
	pop %r12
	ret
.Lhugetlb_cleanup_fail:
	mov $11, %eax
	mov %r12, %rdi
	mov $0x200000, %esi
	syscall
.Lhugetlb_fail:
	pop %r12
	mov $1, %eax
	ret
.Lhugetlb_skip:
	pop %r12
	xor %eax, %eax
	ret
	.size check_hugetlb_mapping, . - check_hugetlb_mapping

	.type check_write_only_mapping, @function
check_write_only_mapping:
	push %r12
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $2, %edx
	mov $MAP_PRIVATE_ANONYMOUS, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Lwrite_only_fail
	mov %rax, %r12
	cmpb $0, (%r12)
	jne .Lwrite_only_cleanup_fail
	movb $0xa6, (%r12)
	cmpb $0xa6, (%r12)
	jne .Lwrite_only_cleanup_fail
	mov $11, %eax
	mov %r12, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne .Lwrite_only_fail
	pop %r12
	xor %eax, %eax
	ret
.Lwrite_only_cleanup_fail:
	mov $11, %eax
	mov %r12, %rdi
	mov $4096, %esi
	syscall
.Lwrite_only_fail:
	pop %r12
	mov $1, %eax
	ret
	.size check_write_only_mapping, . - check_write_only_mapping

	.type check_execute_only_mapping, @function
check_execute_only_mapping:
	push %r12
	push %r13
	push %r14
	mov $-1, %r12
	xor %r13d, %r13d
	mov $319, %eax
	lea security_memfd_name(%rip), %rdi
	mov $0x11, %esi
	syscall
	cmp $-4095, %rax
	jae .Lexecute_only_fail
	mov %rax, %r12
	mov $77, %eax
	mov %r12d, %edi
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne .Lexecute_only_fail
	mov $18, %eax
	mov %r12d, %edi
	lea execute_only_code(%rip), %rsi
	mov $execute_only_code_length, %edx
	xor %r10d, %r10d
	syscall
	cmp $execute_only_code_length, %rax
	jne .Lexecute_only_fail
	mov $9, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $4, %edx
	mov $2, %r10d
	mov %r12d, %r8d
	xor %r9d, %r9d
	syscall
	cmp $-4095, %rax
	jae .Lexecute_only_fail
	mov %rax, %r13
	call *%r13
	cmp $0x12345678, %eax
	jne .Lexecute_only_fail
	cmpl $0, pku_available(%rip)
	je .Lexecute_only_readable
	mov $11, %edi
	mov $4, %esi
	mov $14, %edx
	mov %r13, %rcx
	lea .Lexecute_only_read_resume(%rip), %r8
	mov %r13, %r9
	call prepare_exception
	movq $0x25, expected_error_mask(%rip)
	movq $0x25, expected_error(%rip)
	mov (%r13), %al
.Lexecute_only_read_resume:
	call check_exception
	test %eax, %eax
	jne .Lexecute_only_fail
	jmp .Lexecute_only_success
.Lexecute_only_readable:
	cmpb $0xb8, (%r13)
	jne .Lexecute_only_fail
.Lexecute_only_success:
	xor %r14d, %r14d
	jmp .Lexecute_only_cleanup
.Lexecute_only_fail:
	mov $1, %r14d
.Lexecute_only_cleanup:
	test %r13, %r13
	jz .Lexecute_only_close
	mov $11, %eax
	mov %r13, %rdi
	mov $4096, %esi
	syscall
	test %rax, %rax
	je .Lexecute_only_close
	mov $1, %r14d
.Lexecute_only_close:
	cmp $0, %r12
	jl .Lexecute_only_done
	mov $3, %eax
	mov %r12d, %edi
	syscall
	test %rax, %rax
	je .Lexecute_only_done
	mov $1, %r14d
.Lexecute_only_done:
	mov %r14d, %eax
	pop %r14
	pop %r13
	pop %r12
	ret
	.size check_execute_only_mapping, . - check_execute_only_mapping

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
	movl $219, failure_code(%rip)
	call check_stack_growth
	test %eax, %eax
	jne .Lprocess_child_fail
	call check_growdown_mapping
	test %eax, %eax
	jne .Lprocess_child_fail
	call check_amx_caller_saved
	test %eax, %eax
	jne .Lprocess_child_fail
	call check_lam_enabled
	test %eax, %eax
	jne .Lprocess_child_fail
	call check_shstk_enabled
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
	mov expected_xcomp_supported(%rip), %rcx
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
	cmpl $0, amx_available(%rip)
	jne 2f
	mov $158, %eax
	mov $ARCH_REQ_XCOMP_PERM, %edi
	mov $18, %esi
	syscall
	cmp $-95, %rax
	jne 1f
2:
	xor %eax, %eax
	add $16, %rsp
	ret
1:
	add $16, %rsp
	mov $1, %eax
	ret
	.size check_xcomp_controls, . - check_xcomp_controls

	.type check_amx_without_permission, @function
check_amx_without_permission:
	cmpl $0, amx_available(%rip)
	je .Lamx_without_permission_success
	lea amx_config(%rip), %rax
	.byte 0xc4, 0xe2, 0x78, 0x49, 0x00
	mov $4, %edi
	mov $1, %esi
	mov $7, %edx
	lea .Lamx_without_permission(%rip), %rcx
	lea .Lamx_without_permission_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	lea amx_source(%rip), %rax
	mov $64, %edx
.Lamx_without_permission:
	.byte 0xc4, 0xe2, 0x7b, 0x4b, 0x04, 0x10
.Lamx_without_permission_resume:
	call check_exception
	ret
.Lamx_without_permission_success:
	xor %eax, %eax
	ret
	.size check_amx_without_permission, . - check_amx_without_permission

	.type request_amx_permission, @function
request_amx_permission:
	cmpl $0, amx_available(%rip)
	je .Lamx_permission_success
	sub $8, %rsp
	mov $158, %eax
	mov $ARCH_REQ_XCOMP_PERM, %edi
	mov $18, %esi
	syscall
	test %rax, %rax
	jne .Lamx_permission_fail
	mov $158, %eax
	mov $ARCH_GET_XCOMP_PERM, %edi
	mov %rsp, %rsi
	syscall
	test %rax, %rax
	jne .Lamx_permission_fail
	mov expected_xcomp_supported(%rip), %rcx
	cmp %rcx, (%rsp)
	jne .Lamx_permission_fail
	mov $158, %eax
	mov $ARCH_REQ_XCOMP_PERM, %edi
	mov $18, %esi
	syscall
	test %rax, %rax
	jne .Lamx_permission_fail
	add $8, %rsp
.Lamx_permission_success:
	xor %eax, %eax
	ret
.Lamx_permission_fail:
	add $8, %rsp
	mov $1, %eax
	ret
	.size request_amx_permission, . - request_amx_permission

	.type enable_amx_tiledata, @function
enable_amx_tiledata:
	cmpl $0, amx_available(%rip)
	je .Lamx_enable_success
	mov $4, %edi
	mov $1, %esi
	mov $7, %edx
	lea .Lamx_enable(%rip), %rcx
	lea .Lamx_enable_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	lea amx_source(%rip), %rax
	mov $64, %edx
.Lamx_enable:
	.byte 0xc4, 0xe2, 0x7b, 0x4b, 0x04, 0x10
.Lamx_enable_resume:
	cmpl $0, exception_seen(%rip)
	jne .Lamx_enable_fail
	call set_amx_dynamic_expectations
.Lamx_enable_success:
	xor %eax, %eax
	ret
.Lamx_enable_fail:
	mov $1, %eax
	ret
	.size enable_amx_tiledata, . - enable_amx_tiledata

	.type set_amx_dynamic_expectations, @function
set_amx_dynamic_expectations:
	cmpl $0, amx_available(%rip)
	je .Lamx_expectations_success
	orq $XFEATURE_MASK_XTILE_DATA, expected_xfeatures(%rip)
	mov expected_dynamic_xstate_size(%rip), %eax
	mov %eax, expected_xstate_size(%rip)
	add $4, %eax
	mov %eax, expected_xstate_extended_size(%rip)
.Lamx_expectations_success:
	xor %eax, %eax
	ret
	.size set_amx_dynamic_expectations, . - set_amx_dynamic_expectations

	.type check_amx_tiledata, @function
check_amx_tiledata:
	cmpl $0, amx_available(%rip)
	je .Lamx_tiledata_success
	mov %rdi, %r8
	lea amx_output(%rip), %rax
	mov $64, %edx
	.byte 0xc4, 0xe2, 0x7a, 0x4b, 0x04, 0x10
	lea amx_output(%rip), %rsi
	mov %r8, %rdi
	mov $128, %ecx
	repe cmpsq
	jne .Lamx_tiledata_fail
	mov %r8, %rdi
	call check_amx_compute
	test %eax, %eax
	jne .Lamx_tiledata_fail
.Lamx_tiledata_success:
	xor %eax, %eax
	ret
.Lamx_tiledata_fail:
	mov $1, %eax
	ret
	.size check_amx_tiledata, . - check_amx_tiledata

	.type check_amx_compute, @function
check_amx_compute:
	cmpl $0, amx_bf16_available(%rip)
	jne .Lamx_compute
	cmpl $0, amx_int8_available(%rip)
	jne .Lamx_compute
	cmpl $0, amx_fp16_available(%rip)
	jne .Lamx_compute
	cmpl $0, amx_complex_available(%rip)
	je .Lamx_compute_success
.Lamx_compute:
	push %r12
	mov %rdi, %r12
	mov %r12, %rax
	mov $64, %edx
	tileloadd (%rax,%rdx,1), %tmm1
	tileloadd (%rax,%rdx,1), %tmm2
	cmpl $0, amx_bf16_available(%rip)
	je .Lamx_compute_int8
	tdpbf16ps %tmm1, %tmm2, %tmm0
.Lamx_compute_int8:
	cmpl $0, amx_int8_available(%rip)
	je .Lamx_compute_fp16
	tdpbssd %tmm1, %tmm2, %tmm0
.Lamx_compute_fp16:
	cmpl $0, amx_fp16_available(%rip)
	je .Lamx_compute_complex
	tdpfp16ps %tmm1, %tmm2, %tmm0
.Lamx_compute_complex:
	cmpl $0, amx_complex_available(%rip)
	je .Lamx_compute_restore
	tcmmimfp16ps %tmm1, %tmm2, %tmm0
.Lamx_compute_restore:
	mov %r12, %rax
	tileloadd (%rax,%rdx,1), %tmm0
	tilezero %tmm1
	tilezero %tmm2
	pop %r12
.Lamx_compute_success:
	xor %eax, %eax
	ret
	.size check_amx_compute, . - check_amx_compute

	.type check_amx_caller_saved, @function
check_amx_caller_saved:
	cmpl $0, amx_available(%rip)
	je .Lamx_caller_saved_success
	sub $8, %rsp
	mov $158, %eax
	mov $ARCH_GET_XCOMP_PERM, %edi
	mov %rsp, %rsi
	syscall
	test %rax, %rax
	jne .Lamx_caller_saved_fail
	mov expected_xcomp_supported(%rip), %rcx
	cmp %rcx, (%rsp)
	jne .Lamx_caller_saved_fail
	add $8, %rsp
	mov $4, %edi
	mov $1, %esi
	mov $7, %edx
	lea .Lamx_caller_saved_store(%rip), %rcx
	lea .Lamx_caller_saved_store_resume(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	lea amx_output(%rip), %rax
	mov $64, %edx
.Lamx_caller_saved_store:
	.byte 0xc4, 0xe2, 0x7a, 0x4b, 0x04, 0x10
.Lamx_caller_saved_store_resume:
	cmpl $0, exception_seen(%rip)
	jne .Lamx_caller_saved_error
	lea amx_output(%rip), %rdi
	mov $128, %ecx
	xor %eax, %eax
	repe scasq
	jne .Lamx_caller_saved_error
.Lamx_caller_saved_success:
	xor %eax, %eax
	ret
.Lamx_caller_saved_fail:
	add $8, %rsp
.Lamx_caller_saved_error:
	mov $1, %eax
	ret
	.size check_amx_caller_saved, . - check_amx_caller_saved

	.type check_initial_shstk, @function
check_initial_shstk:
	sub $8, %rsp
	mov $158, %eax
	mov $ARCH_SHSTK_STATUS, %edi
	mov %rsp, %rsi
	syscall
	test %rax, %rax
	jz .Linitial_shstk_status
	cmpl $0, shstk_available(%rip)
	jne .Linitial_shstk_fail
	cmp $-22, %rax
	je .Linitial_shstk_success
	jmp .Linitial_shstk_fail
.Linitial_shstk_status:
	cmpq $0, (%rsp)
	jne .Linitial_shstk_fail
.Linitial_shstk_success:
	add $8, %rsp
	xor %eax, %eax
	ret
.Linitial_shstk_fail:
	add $8, %rsp
	mov $1, %eax
	ret
	.size check_initial_shstk, . - check_initial_shstk

	.type check_shstk_enabled, @function
check_shstk_enabled:
	cmpl $0, shstk_available(%rip)
	je .Lshstk_enabled_success
	movl $125, failure_code(%rip)
	sub $8, %rsp
	mov $158, %eax
	mov $ARCH_SHSTK_STATUS, %edi
	mov %rsp, %rsi
	syscall
	test %rax, %rax
	jne .Lshstk_enabled_fail
	cmpq $ARCH_SHSTK_SHSTK, (%rsp)
	jne .Lshstk_enabled_fail
	xor %eax, %eax
	rdsspq %rax
	test %rax, %rax
	je .Lshstk_enabled_fail
	add $8, %rsp
.Lshstk_enabled_success:
	xor %eax, %eax
	ret
.Lshstk_enabled_fail:
	add $8, %rsp
	mov $1, %eax
	ret
	.size check_shstk_enabled, . - check_shstk_enabled

	.type check_released_thread_shstk, @function
check_released_thread_shstk:
	push %r12
	cmpl $0, shstk_available(%rip)
	je .Lreleased_thread_shstk_success
	movl $130, failure_code(%rip)
	mov thread_shstk_address(%rip), %rdi
	test %rdi, %rdi
	je .Lreleased_thread_shstk_fail
	mov $9, %eax
	mov $4096, %esi
	mov $3, %edx
	mov $0x32, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp thread_shstk_address(%rip), %rax
	jne .Lreleased_thread_shstk_fail
	mov $11, %edi
	mov $2, %esi
	mov $14, %edx
	mov %rax, %rcx
	lea .Lreleased_thread_shstk_resume(%rip), %r8
	mov %rax, %r9
	call prepare_exception
	movq $0x47, expected_error_mask(%rip)
	movq $0x47, expected_error(%rip)
	mov thread_shstk_address(%rip), %rax
	movabs $0x3cc3a55a96696996, %rcx
	mov %rcx, (%rax)
.Lreleased_thread_shstk_resume:
	xor %r12d, %r12d
	cmpl $0, exception_seen(%rip)
	setne %r12b
	cmp %rcx, (%rax)
	jne .Lreleased_thread_shstk_cleanup_fail
	mov %rax, %rdi
	mov $11, %eax
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne .Lreleased_thread_shstk_cleanup_fail
	test %r12d, %r12d
	jne .Lreleased_thread_shstk_fail
.Lreleased_thread_shstk_success:
	xor %eax, %eax
	pop %r12
	ret
.Lreleased_thread_shstk_cleanup_fail:
	mov $1, %r12d
	mov thread_shstk_address(%rip), %rdi
	mov $11, %eax
	mov $4096, %esi
	syscall
.Lreleased_thread_shstk_fail:
	mov $1, %eax
	pop %r12
	ret
	.size check_released_thread_shstk, . - check_released_thread_shstk

	.type check_shstk_control_protection, @function
check_shstk_control_protection:
	mov $11, %edi
	mov $SEGV_CPERR, %esi
	mov $21, %edx
	xor %ecx, %ecx
	lea .Lshstk_retry_ret(%rip), %r8
	xor %r9d, %r9d
	call prepare_exception
	movq $0x7fff, expected_error_mask(%rip)
	movq $1, expected_error(%rip)
	xor %eax, %eax
	rdsspq %rax
	mov %rax, shstk_repair_address(%rip)
	mov (%rax), %rcx
	mov %rcx, shstk_repair_value(%rip)
	xor %ecx, %ecx
	wrssq %rcx, (%rax)
.Lshstk_retry_ret:
	ret
	.size check_shstk_control_protection, . - check_shstk_control_protection

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
	jne 1f
	cmpl $0, shstk_available(%rip)
	jne 1f
	jmp .Larch_prctl_success
.Larch_prctl_cet:
	cmpl $0, shstk_available(%rip)
	jne .Larch_prctl_cet_enabled
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
	movl $122, failure_code(%rip)
	mov $__NR_map_shadow_stack, %eax
	xor %edi, %edi
	mov $4096, %esi
	xor %edx, %edx
	syscall
	cmp $-95, %rax
	jne 1f
	jmp .Larch_prctl_success
.Larch_prctl_cet_enabled:
	cmpq $ARCH_SHSTK_SHSTK, 8(%rsp)
	jne 1f
	mov $158, %eax
	mov $ARCH_SHSTK_ENABLE, %edi
	mov $ARCH_SHSTK_SHSTK, %esi
	syscall
	test %rax, %rax
	jne 1f
	mov $158, %eax
	mov $ARCH_SHSTK_ENABLE, %edi
	mov $ARCH_SHSTK_WRSS, %esi
	syscall
	test %rax, %rax
	jne 1f
	movl $129, failure_code(%rip)
	call check_shstk_control_protection
	call check_exception
	test %eax, %eax
	jne 1f
	movl $126, failure_code(%rip)
	mov $__NR_map_shadow_stack, %eax
	xor %edi, %edi
	mov $4096, %esi
	mov $SHADOW_STACK_SET_TOKEN, %edx
	syscall
	cmp $-4095, %rax
	jae 1f
	mov %rax, 8(%rsp)
	cmpq $0, (%rax)
	jne 1f
	lea 4096(%rax), %rcx
	or $1, %rcx
	cmp %rcx, 4088(%rax)
	jne 1f
	movl $127, failure_code(%rip)
	mov $11, %edi
	mov $2, %esi
	mov $14, %edx
	mov %rax, %rcx
	lea .Lshstk_write_resume(%rip), %r8
	mov %rax, %r9
	call prepare_exception
	movq $0x47, expected_error_mask(%rip)
	movq $0x47, expected_error(%rip)
	mov 8(%rsp), %rax
	movq $1, (%rax)
.Lshstk_write_resume:
	call check_exception
	test %eax, %eax
	jne 1f
	mov 8(%rsp), %rcx
	movabs $0x5a5aa5a55aa5a5a5, %rax
	wrssq %rax, (%rcx)
	cmp %rax, (%rcx)
	jne 1f
	movl $128, failure_code(%rip)
	mov $9, %eax
	mov 8(%rsp), %rdi
	mov $4096, %esi
	mov $3, %edx
	mov $0x32, %r10d
	mov $-1, %r8
	xor %r9d, %r9d
	syscall
	cmp 8(%rsp), %rax
	jne 1f
	movabs $0xa55a5aa55aa55aa5, %rcx
	mov %rcx, (%rax)
	cmp %rcx, (%rax)
	jne 1f
	mov %rax, %rdi
	mov $11, %eax
	mov $4096, %esi
	syscall
	test %rax, %rax
	jne 1f
	mov $158, %eax
	mov $ARCH_SHSTK_DISABLE, %edi
	mov $ARCH_SHSTK_WRSS, %esi
	syscall
	test %rax, %rax
	jne 1f
	mov $158, %eax
	mov $ARCH_SHSTK_LOCK, %edi
	mov $ARCH_SHSTK_SHSTK, %esi
	syscall
	test %rax, %rax
	jne 1f
	mov $158, %eax
	mov $ARCH_SHSTK_DISABLE, %edi
	mov $ARCH_SHSTK_SHSTK, %esi
	syscall
	cmp $-1, %rax
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

	.type check_current_cpu_identity, @function
check_current_cpu_identity:
	push %rbx
	push %r12
	push %r13
	movl $0, getcpu_cpu(%rip)
	movl $0, getcpu_node(%rip)
	mov $309, %eax
	lea getcpu_cpu(%rip), %rdi
	lea getcpu_node(%rip), %rsi
	xor %edx, %edx
	syscall
	test %rax, %rax
	jne .Lcpu_identity_getcpu_fail
	mov getcpu_node(%rip), %r12d
	shl $12, %r12d
	or getcpu_cpu(%rip), %r12d
	cmpl $0, rdpid_available(%rip)
	je .Lcpu_identity_rdtscp
	rdpid %rax
	cmp %r12d, %eax
	jne .Lcpu_identity_rdpid_fail
.Lcpu_identity_rdtscp:
	cmpl $0, rdtscp_available(%rip)
	je .Lcpu_identity_success
	lfence
	rdtsc
	shl $32, %rdx
	or %rdx, %rax
	mov %rax, %r13
	rdtscp
	cmp %r12d, %ecx
	jne .Lcpu_identity_rdtscp_fail
	shl $32, %rdx
	or %rdx, %rax
	cmp %r13, %rax
	jb .Lcpu_identity_order_fail
	mov %rax, %r13
	lfence
	rdtsc
	shl $32, %rdx
	or %rdx, %rax
	cmp %r13, %rax
	jb .Lcpu_identity_order_fail
.Lcpu_identity_success:
	xor %eax, %eax
	jmp .Lcpu_identity_done
.Lcpu_identity_getcpu_fail:
	mov $200, %eax
	jmp .Lcpu_identity_done
.Lcpu_identity_rdpid_fail:
	mov $201, %eax
	jmp .Lcpu_identity_done
.Lcpu_identity_rdtscp_fail:
	mov $202, %eax
	jmp .Lcpu_identity_done
.Lcpu_identity_order_fail:
	mov $203, %eax
.Lcpu_identity_done:
	pop %r13
	pop %r12
	pop %rbx
	ret
	.size check_current_cpu_identity, . - check_current_cpu_identity

	.type check_cpu_identity_on, @function
check_cpu_identity_on:
	push %r12
	push %r13
	sub $8, %rsp
	mov %edi, %r12d
	mov %esi, %r13d
	lea single_cpu_mask(%rip), %rdi
	mov %r13, %rcx
	xor %eax, %eax
	rep stosb
	lea single_cpu_mask(%rip), %rax
	btsq %r12, (%rax)
	mov $203, %eax
	xor %edi, %edi
	mov %r13d, %esi
	lea single_cpu_mask(%rip), %rdx
	syscall
	test %rax, %rax
	jne .Lcpu_identity_affinity_set_fail
	call check_current_cpu_identity
	jmp .Lcpu_identity_on_done
.Lcpu_identity_affinity_set_fail:
	mov $199, %eax
.Lcpu_identity_on_done:
	add $8, %rsp
	pop %r13
	pop %r12
	ret
	.size check_cpu_identity_on, . - check_cpu_identity_on

	.type check_cpu_identity, @function
check_cpu_identity:
	cmpl $0, rdpid_available(%rip)
	jne .Lcpu_identity_check
	cmpl $0, rdtscp_available(%rip)
	jne .Lcpu_identity_check
	xor %eax, %eax
	ret
.Lcpu_identity_check:
	push %rbx
	push %r12
	push %r13
	push %r14
	push %r15
	mov $204, %eax
	xor %edi, %edi
	mov $1024, %esi
	lea affinity_mask(%rip), %rdx
	syscall
	test %rax, %rax
	jle .Lcpu_identity_affinity_get_fail
	cmp $1024, %rax
	ja .Lcpu_identity_affinity_get_fail
	mov %rax, %r12
	mov %rax, %r15
	shl $3, %r15
	mov $-1, %r13d
	mov $-1, %r14d
	xor %ebx, %ebx
.Lcpu_identity_find_cpu:
	cmp %r15, %rbx
	jae .Lcpu_identity_cpus_found
	lea affinity_mask(%rip), %rax
	btq %rbx, (%rax)
	jnc .Lcpu_identity_next_cpu
	cmpl $-1, %r13d
	jne .Lcpu_identity_second_cpu
	mov %ebx, %r13d
	jmp .Lcpu_identity_next_cpu
.Lcpu_identity_second_cpu:
	mov %ebx, %r14d
	jmp .Lcpu_identity_cpus_found
.Lcpu_identity_next_cpu:
	inc %rbx
	jmp .Lcpu_identity_find_cpu
.Lcpu_identity_cpus_found:
	cmpl $-1, %r13d
	je .Lcpu_identity_affinity_get_fail
	mov %r13d, %edi
	mov %r12d, %esi
	call check_cpu_identity_on
	mov %eax, %ebx
	test %ebx, %ebx
	jne .Lcpu_identity_restore_affinity
	cmpl $-1, %r14d
	je .Lcpu_identity_restore_affinity
	mov %r14d, %edi
	mov %r12d, %esi
	call check_cpu_identity_on
	mov %eax, %ebx
.Lcpu_identity_restore_affinity:
	mov $203, %eax
	xor %edi, %edi
	mov %r12d, %esi
	lea affinity_mask(%rip), %rdx
	syscall
	test %rax, %rax
	je .Lcpu_identity_restored
	test %ebx, %ebx
	jne .Lcpu_identity_restored
	mov $204, %ebx
.Lcpu_identity_restored:
	mov %ebx, %eax
	jmp .Lcpu_identity_check_done
.Lcpu_identity_affinity_get_fail:
	mov $198, %eax
.Lcpu_identity_check_done:
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbx
	ret
	.size check_cpu_identity, . - check_cpu_identity

	.type check_cpu_profile, @function
check_cpu_profile:
	push %rbx
	push %r12
	push %r13
	movl $0, sse3_available(%rip)
	movl $0, pclmulqdq_available(%rip)
	movl $0, ssse3_available(%rip)
	movl $0, sse41_available(%rip)
	movl $0, sse42_available(%rip)
	movl $0, popcnt_available(%rip)
	movl $0, aes_available(%rip)
	movl $0, shani_available(%rip)
	movl $0, movbe_available(%rip)
	movl $0, rdrand_available(%rip)
	movl $0, bmi1_available(%rip)
	movl $0, bmi2_available(%rip)
	movl $0, rdseed_available(%rip)
	movl $0, adx_available(%rip)
	movl $0, lahf_available(%rip)
	movl $0, abm_available(%rip)
	movl $0, rdpid_available(%rip)
	movl $0, rdtscp_available(%rip)
	movl $0, clflushopt_available(%rip)
	movl $0, clwb_available(%rip)
	movl $0, clzero_available(%rip)
	movl $0, avx_available(%rip)
	movl $0, fma_available(%rip)
	movl $0, f16c_available(%rip)
	movl $0, avx2_available(%rip)
	movl $0, avx512_available(%rip)
	movl $0, avx512dq_available(%rip)
	movl $0, avx512ifma_available(%rip)
	movl $0, avx512pf_available(%rip)
	movl $0, avx512er_available(%rip)
	movl $0, avx512cd_available(%rip)
	movl $0, avx512bw_available(%rip)
	movl $0, avx512vl_available(%rip)
	movl $0, avx512vbmi_available(%rip)
	movl $0, avx512vbmi2_available(%rip)
	movl $0, gfni_available(%rip)
	movl $0, vaes_available(%rip)
	movl $0, vpclmulqdq_available(%rip)
	movl $0, avx512vnni_available(%rip)
	movl $0, avx512bitalg_available(%rip)
	movl $0, avx512vpopcntdq_available(%rip)
	movl $0, avx5124vnniw_available(%rip)
	movl $0, avx5124fmaps_available(%rip)
	movl $0, avx512vp2intersect_available(%rip)
	movl $0, avx512fp16_available(%rip)
	movl $0, avx512bf16_available(%rip)
	movl $0, avxvnni_available(%rip)
	movl $0, avxifma_available(%rip)
	movl $0, avxvnniint8_available(%rip)
	movl $0, avxneconvert_available(%rip)
	movl $0, avxvnniint16_available(%rip)
	movl $0, sha512_available(%rip)
	movl $0, sm3_available(%rip)
	movl $0, sm4_available(%rip)
	movl $0, avx10_available(%rip)
	movl $0, avx10_version(%rip)
	movl $0, avx10_vnni_int_available(%rip)
	movl $0, amx_bf16_available(%rip)
	movl $0, amx_int8_available(%rip)
	movl $0, amx_fp16_available(%rip)
	movl $0, amx_complex_available(%rip)
	movl $0, leaf7_max_subleaf(%rip)
	movl $0, pku_available(%rip)
	movl $0, lam_available(%rip)
	movl $0, shstk_available(%rip)
	movl $0, amx_available(%rip)
	movl $FP_XSTATE_SIZE, expected_xstate_size(%rip)
	movl $FP_XSTATE_EXTENDED_SIZE, expected_xstate_extended_size(%rip)
	movl $0, expected_pkru_offset(%rip)
	movl $0, expected_ymm_offset(%rip)
	movl $0, expected_opmask_offset(%rip)
	movl $0, expected_zmm_hi256_offset(%rip)
	movl $0, expected_hi16_zmm_offset(%rip)
	movl $FP_XSTATE_SIZE, expected_dynamic_xstate_size(%rip)
	movl $0, expected_tiledata_offset(%rip)
	movq $XFEATURE_MASK_FPSSE, expected_xfeatures(%rip)
	movq $XFEATURE_MASK_FPSSE, expected_xcomp_supported(%rip)
	mov $73, %r11d
	xor %eax, %eax
	xor %ecx, %ecx
	cpuid
	mov %eax, %r12d
	mov $1, %eax
	xor %ecx, %ecx
	cpuid
	test $0x1, %ecx
	jz .Lcpu_detect_pclmulqdq
	movl $1, sse3_available(%rip)
.Lcpu_detect_pclmulqdq:
	test $0x2, %ecx
	jz .Lcpu_detect_ssse3
	movl $1, pclmulqdq_available(%rip)
.Lcpu_detect_ssse3:
	test $0x200, %ecx
	jz .Lcpu_detect_sse41
	movl $1, ssse3_available(%rip)
.Lcpu_detect_sse41:
	test $0x80000, %ecx
	jz .Lcpu_detect_sse42
	movl $1, sse41_available(%rip)
.Lcpu_detect_sse42:
	test $0x100000, %ecx
	jz .Lcpu_detect_movbe
	movl $1, sse42_available(%rip)
.Lcpu_detect_movbe:
	test $0x400000, %ecx
	jz .Lcpu_detect_popcnt
	movl $1, movbe_available(%rip)
.Lcpu_detect_popcnt:
	test $0x800000, %ecx
	jz .Lcpu_detect_aes
	movl $1, popcnt_available(%rip)
.Lcpu_detect_aes:
	test $0x2000000, %ecx
	jz .Lcpu_detect_rdrand
	movl $1, aes_available(%rip)
.Lcpu_detect_rdrand:
	test $0x40000000, %ecx
	jz .Lcpu_detect_avx
	movl $1, rdrand_available(%rip)
.Lcpu_detect_avx:
	test $0x10000000, %ecx
	jz .Lcpu_detect_extended_max
	movl $1, avx_available(%rip)
	test $0x1000, %ecx
	jz .Lcpu_detect_f16c
	movl $1, fma_available(%rip)
.Lcpu_detect_f16c:
	test $0x20000000, %ecx
	jz .Lcpu_detect_extended_max
	movl $1, f16c_available(%rip)
.Lcpu_detect_extended_max:
	mov %r12d, %eax
	cmp $1, %eax
	je .Lcpu_profile_features_done
	cmp $7, %eax
	je .Lcpu_detect_extended_features
	cmp $0xd, %eax
	je .Lcpu_detect_extended_features
	cmp $0x1e, %eax
	je .Lcpu_detect_extended_features
	cmp $0x24, %eax
	jne 2f
.Lcpu_detect_extended_features:
	mov $105, %r11d
	mov $7, %eax
	xor %ecx, %ecx
	cpuid
	test $0x8, %ecx
	jz .Lcpu_detect_rdpid
	movl $1, pku_available(%rip)
.Lcpu_detect_rdpid:
	test $0x400000, %ecx
	jz .Lcpu_detect_shstk
	movl $1, rdpid_available(%rip)
.Lcpu_detect_shstk:
	test $0x80, %ecx
	jz .Lcpu_detect_amx
	movl $1, shstk_available(%rip)
.Lcpu_detect_amx:
	test $0x400000, %edx
	jz .Lcpu_detect_amx_tile
	movl $1, amx_bf16_available(%rip)
.Lcpu_detect_amx_tile:
	test $0x1000000, %edx
	jz .Lcpu_detect_amx_int8
	movl $1, amx_available(%rip)
.Lcpu_detect_amx_int8:
	test $0x2000000, %edx
	jz .Lcpu_detect_lam
	movl $1, amx_int8_available(%rip)
.Lcpu_detect_lam:
	mov $108, %r11d
	mov $7, %eax
	mov $1, %ecx
	cpuid
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test $0x100, %edx
	jz .Lcpu_detect_avx10
	movl $1, amx_complex_available(%rip)
.Lcpu_detect_avx10:
	test $0x80000, %edx
	jz .Lcpu_detect_leaf7_1_edx
	movl $1, avx10_available(%rip)
.Lcpu_detect_leaf7_1_edx:
	mov %edx, %r13d
	and $~0x80530, %r13d
	jne 2f
	test $0x200000, %eax
	jz .Lcpu_detect_leaf7_1_eax
	movl $1, amx_fp16_available(%rip)
.Lcpu_detect_leaf7_1_eax:
	mov %eax, %r13d
	and $~0x4a00037, %r13d
	jne 2f
	test $0x4000000, %eax
	jz .Lcpu_profile_features_done
	movl $1, lam_available(%rip)
.Lcpu_profile_features_done:
	mov $1, %r13d
	cmpl $0, avx10_available(%rip)
	je .Lcpu_profile_no_avx10
	mov $0x24, %r13d
	jmp .Lcpu_profile_check_max
.Lcpu_profile_no_avx10:
	cmpl $0, amx_available(%rip)
	je .Lcpu_profile_no_amx
	mov $0x1e, %r13d
	jmp .Lcpu_profile_check_max
.Lcpu_profile_no_amx:
	cmpl $0, avx_available(%rip)
	jne .Lcpu_profile_xstate_max
	cmpl $0, pku_available(%rip)
	jne .Lcpu_profile_xstate_max
	cmpl $0, shstk_available(%rip)
	jne .Lcpu_profile_xstate_max
	cmpl $0, lam_available(%rip)
	jne .Lcpu_profile_leaf7_max
	cmpl $0, shani_available(%rip)
	jne .Lcpu_profile_leaf7_max
	cmpl $0, bmi1_available(%rip)
	jne .Lcpu_profile_leaf7_max
	cmpl $0, bmi2_available(%rip)
	jne .Lcpu_profile_leaf7_max
	cmpl $0, rdseed_available(%rip)
	jne .Lcpu_profile_leaf7_max
	cmpl $0, adx_available(%rip)
	jne .Lcpu_profile_leaf7_max
	cmpl $0, clflushopt_available(%rip)
	jne .Lcpu_profile_leaf7_max
	cmpl $0, clwb_available(%rip)
	jne .Lcpu_profile_leaf7_max
	cmpl $0, rdpid_available(%rip)
	je .Lcpu_profile_check_max
.Lcpu_profile_leaf7_max:
	mov $7, %r13d
	jmp .Lcpu_profile_check_max
.Lcpu_profile_xstate_max:
	mov $0xd, %r13d
.Lcpu_profile_check_max:
	cmp %r13d, %r12d
	jne 2f

	mov $74, %r11d
	mov $1, %eax
	xor %ecx, %ecx
	cpuid
	mov $ELF_HWCAP, %r9d
	cmp $ELF_HWCAP, %edx
	jne 2f
	xor %r13d, %r13d
	cmpl $0, sse3_available(%rip)
	je .Lcpu_leaf1_pclmulqdq
	or $0x1, %r13d
.Lcpu_leaf1_pclmulqdq:
	cmpl $0, pclmulqdq_available(%rip)
	je .Lcpu_leaf1_ssse3
	or $0x2, %r13d
.Lcpu_leaf1_ssse3:
	cmpl $0, ssse3_available(%rip)
	je .Lcpu_leaf1_sse41
	or $0x200, %r13d
.Lcpu_leaf1_sse41:
	cmpl $0, sse41_available(%rip)
	je .Lcpu_leaf1_sse42
	or $0x80000, %r13d
.Lcpu_leaf1_sse42:
	cmpl $0, sse42_available(%rip)
	je .Lcpu_leaf1_movbe
	or $0x100000, %r13d
.Lcpu_leaf1_movbe:
	cmpl $0, movbe_available(%rip)
	je .Lcpu_leaf1_popcnt
	or $0x400000, %r13d
.Lcpu_leaf1_popcnt:
	cmpl $0, popcnt_available(%rip)
	je .Lcpu_leaf1_aes
	or $0x800000, %r13d
.Lcpu_leaf1_aes:
	cmpl $0, aes_available(%rip)
	je .Lcpu_leaf1_rdrand
	or $0x2000000, %r13d
.Lcpu_leaf1_rdrand:
	cmpl $0, rdrand_available(%rip)
	je .Lcpu_leaf1_extended_state
	or $0x40000000, %r13d
.Lcpu_leaf1_extended_state:
	cmpl $0, avx_available(%rip)
	jne .Lcpu_leaf1_xsave
	cmpl $0, pku_available(%rip)
	jne .Lcpu_leaf1_xsave
	cmpl $0, shstk_available(%rip)
	jne .Lcpu_leaf1_xsave
	cmpl $0, amx_available(%rip)
	je .Lcpu_leaf1_ecx
.Lcpu_leaf1_xsave:
	or $0xc000000, %r13d
.Lcpu_leaf1_ecx:
	cmpl $0, avx_available(%rip)
	je .Lcpu_leaf1_expected
	or $0x10000000, %r13d
	cmpl $0, fma_available(%rip)
	je .Lcpu_leaf1_f16c
	or $0x1000, %r13d
.Lcpu_leaf1_f16c:
	cmpl $0, f16c_available(%rip)
	je .Lcpu_leaf1_expected
	or $0x20000000, %r13d
.Lcpu_leaf1_expected:
	cmp %r13d, %ecx
	jne 2f
	cmpl $1, %r12d
	jne .Lcpu_leaf7
	jmp .Lcpu_extended

.Lcpu_leaf7:
	mov $105, %r11d
	mov $7, %eax
	xor %ecx, %ecx
	cpuid
	mov %eax, leaf7_max_subleaf(%rip)
	xor %r13d, %r13d
	test $0x8, %ebx
	jz .Lcpu_leaf7_bmi2
	movl $1, bmi1_available(%rip)
	or $0x8, %r13d
.Lcpu_leaf7_bmi2:
	test $0x100, %ebx
	jz .Lcpu_leaf7_rdseed
	movl $1, bmi2_available(%rip)
	or $0x100, %r13d
.Lcpu_leaf7_rdseed:
	test $0x40000, %ebx
	jz .Lcpu_leaf7_adx
	movl $1, rdseed_available(%rip)
	or $0x40000, %r13d
.Lcpu_leaf7_adx:
	test $0x80000, %ebx
	jz .Lcpu_leaf7_clflushopt
	movl $1, adx_available(%rip)
	or $0x80000, %r13d
.Lcpu_leaf7_clflushopt:
	test $0x800000, %ebx
	jz .Lcpu_leaf7_clwb
	movl $1, clflushopt_available(%rip)
	or $0x800000, %r13d
.Lcpu_leaf7_clwb:
	test $0x1000000, %ebx
	jz .Lcpu_leaf7_shani
	movl $1, clwb_available(%rip)
	or $0x1000000, %r13d
.Lcpu_leaf7_shani:
	test $0x20000000, %ebx
	jz .Lcpu_leaf7_avx
	movl $1, shani_available(%rip)
	or $0x20000000, %r13d
.Lcpu_leaf7_avx:
	cmpl $0, avx_available(%rip)
	je .Lcpu_leaf7_ebx
	test $0x20, %ebx
	jz .Lcpu_leaf7_avx512
	movl $1, avx2_available(%rip)
	or $0x20, %r13d
.Lcpu_leaf7_avx512:
	test $0x10000, %ebx
	jz .Lcpu_leaf7_ebx
	movl $1, avx512_available(%rip)
	or $0x10000, %r13d
	test $0x20000, %ebx
	jz .Lcpu_leaf7_avx512ifma
	movl $1, avx512dq_available(%rip)
	or $0x20000, %r13d
.Lcpu_leaf7_avx512ifma:
	test $0x200000, %ebx
	jz .Lcpu_leaf7_avx512pf
	movl $1, avx512ifma_available(%rip)
	or $0x200000, %r13d
.Lcpu_leaf7_avx512pf:
	test $0x4000000, %ebx
	jz .Lcpu_leaf7_avx512er
	movl $1, avx512pf_available(%rip)
	or $0x4000000, %r13d
.Lcpu_leaf7_avx512er:
	test $0x8000000, %ebx
	jz .Lcpu_leaf7_avx512cd
	movl $1, avx512er_available(%rip)
	or $0x8000000, %r13d
.Lcpu_leaf7_avx512cd:
	test $0x10000000, %ebx
	jz .Lcpu_leaf7_avx512bw
	movl $1, avx512cd_available(%rip)
	or $0x10000000, %r13d
.Lcpu_leaf7_avx512bw:
	test $0x40000000, %ebx
	jz .Lcpu_leaf7_avx512vl
	movl $1, avx512bw_available(%rip)
	or $0x40000000, %r13d
.Lcpu_leaf7_avx512vl:
	test $0x80000000, %ebx
	jz .Lcpu_leaf7_ebx
	movl $1, avx512vl_available(%rip)
	or $0x80000000, %r13d
.Lcpu_leaf7_ebx:
	cmp %r13d, %ebx
	jne 2f
	xor %r13d, %r13d
	cmpl $0, pku_available(%rip)
	je .Lcpu_leaf7_shstk
	or $0x18, %r13d
.Lcpu_leaf7_shstk:
	cmpl $0, shstk_available(%rip)
	je .Lcpu_leaf7_gfni
	or $0x80, %r13d
.Lcpu_leaf7_gfni:
	cmpl $0, avx_available(%rip)
	je .Lcpu_leaf7_avx512vbmi
	test $0x100, %ecx
	jz .Lcpu_leaf7_vaes
	movl $1, gfni_available(%rip)
	or $0x100, %r13d
.Lcpu_leaf7_vaes:
	test $0x200, %ecx
	jz .Lcpu_leaf7_vpclmulqdq
	movl $1, vaes_available(%rip)
	or $0x200, %r13d
.Lcpu_leaf7_vpclmulqdq:
	test $0x400, %ecx
	jz .Lcpu_leaf7_avx512vbmi
	movl $1, vpclmulqdq_available(%rip)
	or $0x400, %r13d
.Lcpu_leaf7_avx512vbmi:
	cmpl $0, avx512_available(%rip)
	je .Lcpu_leaf7_ecx
	test $0x2, %ecx
	jz .Lcpu_leaf7_avx512vbmi2
	movl $1, avx512vbmi_available(%rip)
	or $0x2, %r13d
.Lcpu_leaf7_avx512vbmi2:
	test $0x40, %ecx
	jz .Lcpu_leaf7_avx512vnni
	movl $1, avx512vbmi2_available(%rip)
	or $0x40, %r13d
.Lcpu_leaf7_avx512vnni:
	test $0x800, %ecx
	jz .Lcpu_leaf7_avx512bitalg
	movl $1, avx512vnni_available(%rip)
	or $0x800, %r13d
.Lcpu_leaf7_avx512bitalg:
	test $0x1000, %ecx
	jz .Lcpu_leaf7_avx512vpopcntdq
	movl $1, avx512bitalg_available(%rip)
	or $0x1000, %r13d
.Lcpu_leaf7_avx512vpopcntdq:
	test $0x4000, %ecx
	jz .Lcpu_leaf7_rdpid
	movl $1, avx512vpopcntdq_available(%rip)
	or $0x4000, %r13d
.Lcpu_leaf7_rdpid:
	test $0x400000, %ecx
	jz .Lcpu_leaf7_ecx
	movl $1, rdpid_available(%rip)
	or $0x400000, %r13d
.Lcpu_leaf7_ecx:
	cmp %r13d, %ecx
	jne 2f
	xor %r13d, %r13d
	cmpl $0, avx512_available(%rip)
	je .Lcpu_leaf7_amx
	test $0x4, %edx
	jz .Lcpu_leaf7_avx5124fmaps
	movl $1, avx5124vnniw_available(%rip)
	or $0x4, %r13d
.Lcpu_leaf7_avx5124fmaps:
	test $0x8, %edx
	jz .Lcpu_leaf7_avx512vp2intersect
	movl $1, avx5124fmaps_available(%rip)
	or $0x8, %r13d
.Lcpu_leaf7_avx512vp2intersect:
	test $0x100, %edx
	jz .Lcpu_leaf7_avx512fp16
	movl $1, avx512vp2intersect_available(%rip)
	or $0x100, %r13d
.Lcpu_leaf7_avx512fp16:
	test $0x800000, %edx
	jz .Lcpu_leaf7_amx
	movl $1, avx512fp16_available(%rip)
	or $0x800000, %r13d
.Lcpu_leaf7_amx:
	cmpl $0, amx_available(%rip)
	je .Lcpu_leaf7_edx
	or $0x1000000, %r13d
	cmpl $0, amx_bf16_available(%rip)
	je .Lcpu_leaf7_amx_int8
	or $0x400000, %r13d
.Lcpu_leaf7_amx_int8:
	cmpl $0, amx_int8_available(%rip)
	je .Lcpu_leaf7_edx
	or $0x2000000, %r13d
.Lcpu_leaf7_edx:
	cmp %r13d, %edx
	jne 2f
	mov $108, %r11d
	mov $7, %eax
	mov $1, %ecx
	cpuid
	xor %r13d, %r13d
	xor %r10d, %r10d
	cmpl $0, avx_available(%rip)
	je .Lcpu_leaf7_1_avx512bf16
	test $0x1, %eax
	jz .Lcpu_leaf7_1_sm3
	movl $1, sha512_available(%rip)
	or $0x1, %r13d
.Lcpu_leaf7_1_sm3:
	test $0x2, %eax
	jz .Lcpu_leaf7_1_sm4
	movl $1, sm3_available(%rip)
	or $0x2, %r13d
.Lcpu_leaf7_1_sm4:
	test $0x4, %eax
	jz .Lcpu_leaf7_1_avxvnni
	movl $1, sm4_available(%rip)
	or $0x4, %r13d
.Lcpu_leaf7_1_avxvnni:
	test $0x10, %eax
	jz .Lcpu_leaf7_1_avxifma
	movl $1, avxvnni_available(%rip)
	or $0x10, %r13d
.Lcpu_leaf7_1_avxifma:
	test $0x800000, %eax
	jz .Lcpu_leaf7_1_avxvnniint8
	movl $1, avxifma_available(%rip)
	or $0x800000, %r13d
.Lcpu_leaf7_1_avxvnniint8:
	test $0x10, %edx
	jz .Lcpu_leaf7_1_avxneconvert
	movl $1, avxvnniint8_available(%rip)
	or $0x10, %r10d
.Lcpu_leaf7_1_avxneconvert:
	test $0x20, %edx
	jz .Lcpu_leaf7_1_avxvnniint16
	movl $1, avxneconvert_available(%rip)
	or $0x20, %r10d
.Lcpu_leaf7_1_avxvnniint16:
	test $0x400, %edx
	jz .Lcpu_leaf7_1_avx512bf16
	movl $1, avxvnniint16_available(%rip)
	or $0x400, %r10d
.Lcpu_leaf7_1_avx512bf16:
	cmpl $0, avx512_available(%rip)
	je .Lcpu_leaf7_1_lam
	test $0x20, %eax
	jz .Lcpu_leaf7_1_lam
	movl $1, avx512bf16_available(%rip)
	or $0x20, %r13d
.Lcpu_leaf7_1_lam:
	cmpl $0, amx_fp16_available(%rip)
	je .Lcpu_leaf7_1_lam_check
	or $0x200000, %r13d
.Lcpu_leaf7_1_lam_check:
	cmpl $0, lam_available(%rip)
	je .Lcpu_leaf7_1_eax
	or $0x4000000, %r13d
.Lcpu_leaf7_1_eax:
	cmp %r13d, %eax
	jne 2f
	mov $156, %r11d
	mov %r13d, %r12d
	or %r10d, %r12d
	setne %r12b
	movzbl %r12b, %r12d
	cmp %r12d, leaf7_max_subleaf(%rip)
	jne 2f
	mov $157, %r11d
	test %ebx, %ebx
	jne 2f
	mov $158, %r11d
	test %ecx, %ecx
	jne 2f
	mov $159, %r11d
	cmpl $0, amx_complex_available(%rip)
	je .Lcpu_leaf7_1_avx10
	or $0x100, %r10d
.Lcpu_leaf7_1_avx10:
	cmpl $0, avx10_available(%rip)
	je .Lcpu_leaf7_1_edx
	or $0x80000, %r10d
.Lcpu_leaf7_1_edx:
	cmp %r10d, %edx
	jne 2f
	cmpl $0, avx_available(%rip)
	jne .Lcpu_check_avx_xstate
	mov $132, %r11d
	mov $0xd, %eax
	mov $2, %ecx
	cpuid
	test %eax, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	jmp .Lcpu_check_no_avx512_xstate
.Lcpu_after_avx_xstate:
	cmpl $0, pku_available(%rip)
	jne .Lcpu_check_pku_xstate
	cmpl $0, shstk_available(%rip)
	jne .Lcpu_check_user_xstate
	cmpl $0, amx_available(%rip)
	je .Lcpu_extended
	jmp .Lcpu_check_user_xstate

.Lcpu_check_avx_xstate:
	mov $132, %r11d
	mov $0xd, %eax
	mov $2, %ecx
	cpuid
	cmp $256, %eax
	jne 2f
	cmp $FP_XSTATE_SIZE, %ebx
	jb 2f
	mov %ecx, %r12d
	and $~2, %r12d
	jne 2f
	test %edx, %edx
	jne 2f
	mov %ebx, expected_ymm_offset(%rip)
	add %ebx, %eax
	jc 2f
	cmp expected_xstate_size(%rip), %eax
	jbe .Lcpu_record_ymm
	mov %eax, expected_xstate_size(%rip)
	mov %eax, expected_dynamic_xstate_size(%rip)
	add $4, %eax
	mov %eax, expected_xstate_extended_size(%rip)
.Lcpu_record_ymm:
	orq $XFEATURE_MASK_YMM, expected_xfeatures(%rip)
	orq $XFEATURE_MASK_YMM, expected_xcomp_supported(%rip)
	cmpl $0, avx512_available(%rip)
	je .Lcpu_check_no_avx512_xstate
	jmp .Lcpu_check_avx512_xstate

.Lcpu_check_no_avx512_xstate:
	mov $139, %r11d
	mov $5, %r12d
.Lcpu_check_no_avx512_component:
	mov $0xd, %eax
	mov %r12d, %ecx
	cpuid
	test %eax, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	inc %r12d
	cmp $8, %r12d
	jne .Lcpu_check_no_avx512_component
	jmp .Lcpu_after_avx_xstate

.Lcpu_check_avx512_xstate:
	mov $139, %r11d
	mov $0xd, %eax
	mov $5, %ecx
	cpuid
	cmp $AVX512_OPMASK_SIZE, %eax
	jne 2f
	cmp expected_xstate_size(%rip), %ebx
	jb 2f
	mov %ecx, %r12d
	and $~2, %r12d
	jne 2f
	test %edx, %edx
	jne 2f
	mov %ebx, expected_opmask_offset(%rip)
	add %ebx, %eax
	jc 2f
	mov %eax, %r13d
	mov $0xd, %eax
	mov $6, %ecx
	cpuid
	cmp $AVX512_ZMM_HI256_SIZE, %eax
	jne 2f
	cmp %r13d, %ebx
	jb 2f
	mov %ecx, %r12d
	and $~2, %r12d
	jne 2f
	test %edx, %edx
	jne 2f
	mov %ebx, expected_zmm_hi256_offset(%rip)
	add %ebx, %eax
	jc 2f
	mov %eax, %r13d
	mov $0xd, %eax
	mov $7, %ecx
	cpuid
	cmp $AVX512_HI16_ZMM_SIZE, %eax
	jne 2f
	cmp %r13d, %ebx
	jb 2f
	mov %ecx, %r12d
	and $~2, %r12d
	jne 2f
	test %edx, %edx
	jne 2f
	mov %ebx, expected_hi16_zmm_offset(%rip)
	add %ebx, %eax
	jc 2f
	mov %eax, expected_xstate_size(%rip)
	mov %eax, expected_dynamic_xstate_size(%rip)
	add $4, %eax
	mov %eax, expected_xstate_extended_size(%rip)
	orq $XFEATURE_MASK_AVX512, expected_xfeatures(%rip)
	orq $XFEATURE_MASK_AVX512, expected_xcomp_supported(%rip)
	jmp .Lcpu_after_avx_xstate

.Lcpu_check_pku_xstate:
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
	cmp expected_xstate_size(%rip), %eax
	jbe .Lcpu_record_pkru
	mov %eax, expected_xstate_size(%rip)
	mov %eax, expected_dynamic_xstate_size(%rip)
	add $4, %eax
	mov %eax, expected_xstate_extended_size(%rip)
.Lcpu_record_pkru:
	mov %r13d, expected_pkru_offset(%rip)
	orq $0x200, expected_xfeatures(%rip)
	orq $0x200, expected_xcomp_supported(%rip)
.Lcpu_check_user_xstate:
	cmpl $0, amx_available(%rip)
	je .Lcpu_no_amx_components
	mov $125, %r11d
	mov $0xd, %eax
	mov $17, %ecx
	cpuid
	cmp $XTILE_CFG_SIZE, %eax
	jne 2f
	cmp $FP_XSTATE_SIZE, %ebx
	jb 2f
	mov %ecx, %r12d
	and $~6, %r12d
	jne 2f
	test %edx, %edx
	jne 2f
	add %ebx, %eax
	jc 2f
	mov %eax, %r13d
	cmp expected_xstate_size(%rip), %eax
	jbe .Lcpu_check_tiledata_component
	mov %eax, expected_xstate_size(%rip)
	add $4, %eax
	mov %eax, expected_xstate_extended_size(%rip)
.Lcpu_check_tiledata_component:
	mov $126, %r11d
	mov $0xd, %eax
	mov $18, %ecx
	cpuid
	cmp $XTILE_DATA_SIZE, %eax
	jne 2f
	cmp %r13d, %ebx
	jb 2f
	test $4, %ecx
	jz 2f
	mov %ecx, %r12d
	and $~6, %r12d
	jne 2f
	test %edx, %edx
	jne 2f
	mov %ebx, expected_tiledata_offset(%rip)
	add %ebx, %eax
	jc 2f
	mov %eax, expected_dynamic_xstate_size(%rip)
	orq $XFEATURE_MASK_XTILE_CFG, expected_xfeatures(%rip)
	orq $XFEATURE_MASK_XTILE, expected_xcomp_supported(%rip)
	jmp .Lcpu_xstate_components_done
.Lcpu_no_amx_components:
	mov $125, %r11d
	mov $0xd, %eax
	mov $17, %ecx
	cpuid
	test %eax, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	mov $0xd, %eax
	mov $18, %ecx
	cpuid
	test %eax, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
.Lcpu_xstate_components_done:
	mov $0xd, %eax
	xor %ecx, %ecx
	cpuid
	mov $XFEATURE_MASK_FPSSE, %r13d
	cmpl $0, avx_available(%rip)
	je .Lcpu_user_xfeatures_pku
	or $XFEATURE_MASK_YMM, %r13d
.Lcpu_user_xfeatures_pku:
	cmpl $0, avx512_available(%rip)
	je .Lcpu_user_xfeatures_pkru
	or $XFEATURE_MASK_AVX512, %r13d
.Lcpu_user_xfeatures_pkru:
	cmpl $0, pku_available(%rip)
	je .Lcpu_user_xfeatures_amx
	or $0x200, %r13d
.Lcpu_user_xfeatures_amx:
	cmpl $0, amx_available(%rip)
	je .Lcpu_user_xfeatures
	or $XFEATURE_MASK_XTILE, %r13d
.Lcpu_user_xfeatures:
	cmp %r13d, %eax
	jne 2f
	test %edx, %edx
	jne 2f
	mov expected_xstate_size(%rip), %r12d
	cmpl $0, amx_available(%rip)
	je .Lcpu_user_xstate_size
	mov expected_dynamic_xstate_size(%rip), %r12d
.Lcpu_user_xstate_size:
	cmp %r12d, %ebx
	jne 2f
	mov expected_dynamic_xstate_size(%rip), %r12d
	cmp %r12d, %ecx
	jne 2f

	mov $123, %r11d
	mov $0xd, %eax
	mov $1, %ecx
	cpuid
	xor %r12d, %r12d
	cmpl $0, shstk_available(%rip)
	je .Lcpu_xstate_amx_controls
	or $0x8, %r12d
.Lcpu_xstate_amx_controls:
	cmpl $0, amx_available(%rip)
	je .Lcpu_xstate_controls
	or $0x1c, %r12d
.Lcpu_xstate_controls:
	cmp %r12d, %eax
	jne 2f
	test %r12d, %r12d
	jz .Lcpu_no_extended_xstate_controls
	test %ebx, %ebx
	jz 2f
	xor %r12d, %r12d
	cmpl $0, shstk_available(%rip)
	je .Lcpu_xstate_supervisor_controls
	mov $XFEATURE_MASK_CET_USER, %r12d
.Lcpu_xstate_supervisor_controls:
	cmp %r12d, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	jmp .Lcpu_check_cet_component
.Lcpu_no_extended_xstate_controls:
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
.Lcpu_check_cet_component:
	mov $0xd, %eax
	mov $11, %ecx
	cpuid
	cmpl $0, shstk_available(%rip)
	je .Lcpu_no_cet_component
	cmp $16, %eax
	jne 2f
	cmp $1, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	jmp .Lcpu_check_amx_palette
.Lcpu_no_cet_component:
	test %eax, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
.Lcpu_check_amx_palette:
	cmpl $0, amx_available(%rip)
	je .Lcpu_check_avx10
	mov $127, %r11d
	mov $0x1d, %eax
	xor %ecx, %ecx
	cpuid
	cmp $1, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	mov $0x1d, %eax
	mov $1, %ecx
	cpuid
	cmp $0x04002000, %eax
	jne 2f
	cmp $0x00080040, %ebx
	jne 2f
	cmp $16, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	mov $0x1d, %eax
	mov $2, %ecx
	cpuid
	test %eax, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	mov $214, %r11d
	mov $0x1e, %eax
	xor %ecx, %ecx
	cpuid
	cmp $1, %eax
	ja 2f
	mov %eax, %r12d
	cmp $0x4010, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	mov $0x1e, %eax
	mov $1, %ecx
	cpuid
	xor %r13d, %r13d
	test %r12d, %r12d
	jz .Lcpu_check_amx_aliases
	cmpl $0, amx_int8_available(%rip)
	je .Lcpu_check_amx_alias_bf16
	or $0x1, %r13d
.Lcpu_check_amx_alias_bf16:
	cmpl $0, amx_bf16_available(%rip)
	je .Lcpu_check_amx_alias_complex
	or $0x2, %r13d
.Lcpu_check_amx_alias_complex:
	cmpl $0, amx_complex_available(%rip)
	je .Lcpu_check_amx_alias_fp16
	or $0x4, %r13d
.Lcpu_check_amx_alias_fp16:
	cmpl $0, amx_fp16_available(%rip)
	je .Lcpu_check_amx_aliases
	or $0x8, %r13d
.Lcpu_check_amx_aliases:
	cmp %r13d, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f

.Lcpu_check_avx10:
	cmpl $0, avx10_available(%rip)
	je .Lcpu_extended
	mov $215, %r11d
	mov $0x24, %eax
	xor %ecx, %ecx
	cpuid
	cmp $1, %eax
	ja 2f
	mov %eax, %r12d
	mov %ebx, %r13d
	and $0xff, %r13d
	cmp $1, %r13d
	jb 2f
	cmp $2, %r13d
	ja 2f
	cmp $2, %r13d
	jne .Lcpu_avx10_version_ready
	cmp $1, %r12d
	jne 2f
.Lcpu_avx10_version_ready:
	mov %r13d, avx10_version(%rip)
	or $0x70000, %r13d
	cmp %r13d, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
	jne 2f
	mov $0x24, %eax
	mov $1, %ecx
	cpuid
	test %r12d, %r12d
	jz .Lcpu_avx10_no_subleaf
	mov %ecx, %r13d
	and $~0x4, %r13d
	jne 2f
	test $0x4, %ecx
	jz .Lcpu_avx10_subleaf_regs
	movl $1, avx10_vnni_int_available(%rip)
	jmp .Lcpu_avx10_subleaf_regs
.Lcpu_avx10_no_subleaf:
	test %ecx, %ecx
	jne 2f
.Lcpu_avx10_subleaf_regs:
	test %eax, %eax
	jne 2f
	test %ebx, %ebx
	jne 2f
	test %edx, %edx
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
	xor %r13d, %r13d
	test $0x1, %ecx
	jz .Lcpu_extended_abm
	movl $1, lahf_available(%rip)
	or $0x1, %r13d
.Lcpu_extended_abm:
	test $0x20, %ecx
	jz .Lcpu_extended_ecx
	movl $1, abm_available(%rip)
	or $0x20, %r13d
.Lcpu_extended_ecx:
	cmp %r13d, %ecx
	jne 2f
	mov $0x20100800, %r13d
	test $0x8000000, %edx
	jz .Lcpu_extended_edx
	movl $1, rdtscp_available(%rip)
	or $0x8000000, %r13d
.Lcpu_extended_edx:
	cmp %r13d, %edx
	jne 2f
	mov $193, %r11d
	mov $0x80000008, %eax
	xor %ecx, %ecx
	cpuid
	xor %r13d, %r13d
	test $0x1, %ebx
	jz .Lcpu_extended_8_ebx
	movl $1, clzero_available(%rip)
	or $0x1, %r13d
.Lcpu_extended_8_ebx:
	cmp %r13d, %ebx
	jne 2f
	test %ecx, %ecx
	jne 2f
	test %edx, %edx
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
	cmpl $0, avx_available(%rip)
	jne .Lcpu_xgetbv
	cmpl $0, pku_available(%rip)
	jne .Lcpu_xgetbv
	cmpl $0, shstk_available(%rip)
	jne .Lcpu_xgetbv
	cmpl $0, amx_available(%rip)
	jne .Lcpu_xgetbv
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
.Lcpu_xgetbv:
	mov $107, %r11d
	xor %ecx, %ecx
	xgetbv
	mov $XFEATURE_MASK_FPSSE, %r12d
	cmpl $0, avx_available(%rip)
	je .Lcpu_xgetbv_pku
	or $XFEATURE_MASK_YMM, %r12d
.Lcpu_xgetbv_pku:
	cmpl $0, avx512_available(%rip)
	je .Lcpu_xgetbv_pkru
	or $XFEATURE_MASK_AVX512, %r12d
.Lcpu_xgetbv_pkru:
	cmpl $0, pku_available(%rip)
	je .Lcpu_xgetbv_amx
	or $0x200, %r12d
.Lcpu_xgetbv_amx:
	cmpl $0, amx_available(%rip)
	je .Lcpu_xgetbv_value
	or $XFEATURE_MASK_XTILE, %r12d
.Lcpu_xgetbv_value:
	cmp %r12d, %eax
	jne 2f
	test %edx, %edx
	jne 2f
	cmpl $0, amx_available(%rip)
	je .Lcpu_profile_done
	mov $128, %r11d
	mov $1, %ecx
	xgetbv
	test %edx, %edx
	jne 2f
	not %r12d
	test %r12d, %eax
	jne 2f
.Lcpu_profile_done:
	call check_cpu_identity
	test %eax, %eax
	je .Lcpu_profile_identity_done
	mov %eax, %r11d
	jmp 2f
.Lcpu_profile_identity_done:
	mov $134, %r11d
	cmpl $0, fma_available(%rip)
	je .Lcpu_instruction_f16c
	vxorps %ymm0, %ymm0, %ymm0
	vfmadd132ps %ymm0, %ymm0, %ymm0
.Lcpu_instruction_f16c:
	cmpl $0, f16c_available(%rip)
	je .Lcpu_instruction_avx2
	vxorps %xmm0, %xmm0, %xmm0
	vcvtps2ph $0, %xmm0, %xmm1
.Lcpu_instruction_avx2:
	cmpl $0, avx2_available(%rip)
	je .Lcpu_instruction_avx512
	vxorps %xmm0, %xmm0, %xmm0
	vpbroadcastd %xmm0, %ymm1
.Lcpu_instruction_avx512:
	cmpl $0, avx512_available(%rip)
	je .Lcpu_instructions_done
	vpxord %zmm0, %zmm0, %zmm0
	kxnorw %k1, %k1, %k1
	cmpl $0, avx512dq_available(%rip)
	je .Lcpu_instruction_avx512ifma
	vpmullq %zmm0, %zmm0, %zmm0
.Lcpu_instruction_avx512ifma:
	cmpl $0, avx512ifma_available(%rip)
	je .Lcpu_instruction_avx512pf
	vpmadd52luq %zmm0, %zmm0, %zmm0
.Lcpu_instruction_avx512pf:
	cmpl $0, avx512pf_available(%rip)
	je .Lcpu_instruction_avx512er
	kxorw %k1, %k1, %k1
	vgatherpf0dps (%rsp,%zmm0,1){%k1}
.Lcpu_instruction_avx512er:
	cmpl $0, avx512er_available(%rip)
	je .Lcpu_instruction_avx512cd
	vpxord %zmm0, %zmm0, %zmm0
	vrcp28ps %zmm0, %zmm0
.Lcpu_instruction_avx512cd:
	cmpl $0, avx512cd_available(%rip)
	je .Lcpu_instruction_avx512bw
	vpconflictd %zmm0, %zmm0
.Lcpu_instruction_avx512bw:
	cmpl $0, avx512bw_available(%rip)
	je .Lcpu_instruction_avx512vl
	vpaddb %zmm0, %zmm0, %zmm0
.Lcpu_instruction_avx512vl:
	cmpl $0, avx512vl_available(%rip)
	je .Lcpu_instruction_avx512vbmi
	vpxord %zmm0, %zmm0, %zmm0
	kxnorw %k1, %k1, %k1
	vaddps %ymm0, %ymm0, %ymm0{%k1}
.Lcpu_instruction_avx512vbmi:
	cmpl $0, avx512vbmi_available(%rip)
	je .Lcpu_instruction_avx512vbmi2
	vpermb %zmm0, %zmm0, %zmm0
.Lcpu_instruction_avx512vbmi2:
	cmpl $0, avx512vbmi2_available(%rip)
	je .Lcpu_instruction_gfni
	vpshldvd %zmm0, %zmm0, %zmm0
.Lcpu_instruction_gfni:
	cmpl $0, gfni_available(%rip)
	je .Lcpu_instruction_vaes
	vgf2p8mulb %ymm0, %ymm0, %ymm0
.Lcpu_instruction_vaes:
	cmpl $0, vaes_available(%rip)
	je .Lcpu_instruction_vpclmulqdq
	vaesenc %ymm0, %ymm0, %ymm0
.Lcpu_instruction_vpclmulqdq:
	cmpl $0, vpclmulqdq_available(%rip)
	je .Lcpu_instruction_avx512vnni
	vpclmulqdq $0, %ymm0, %ymm0, %ymm0
.Lcpu_instruction_avx512vnni:
	cmpl $0, avx512vnni_available(%rip)
	je .Lcpu_instruction_avx512bitalg
	vpdpbusd %zmm0, %zmm0, %zmm0
.Lcpu_instruction_avx512bitalg:
	cmpl $0, avx512bitalg_available(%rip)
	je .Lcpu_instruction_avx512vpopcntdq
	vpshufbitqmb %zmm0, %zmm0, %k1
.Lcpu_instruction_avx512vpopcntdq:
	cmpl $0, avx512vpopcntdq_available(%rip)
	je .Lcpu_instruction_avx5124vnniw
	vpopcntd %zmm0, %zmm0
.Lcpu_instruction_avx5124vnniw:
	cmpl $0, avx5124vnniw_available(%rip)
	je .Lcpu_instruction_avx5124fmaps
	vp4dpwssd initial_zmm15(%rip), %zmm0, %zmm4
.Lcpu_instruction_avx5124fmaps:
	cmpl $0, avx5124fmaps_available(%rip)
	je .Lcpu_instruction_avx512vp2intersect
	v4fmaddps initial_zmm15(%rip), %zmm0, %zmm4
.Lcpu_instruction_avx512vp2intersect:
	cmpl $0, avx512vp2intersect_available(%rip)
	je .Lcpu_instruction_avx512fp16
	vp2intersectd %zmm0, %zmm0, %k1
.Lcpu_instruction_avx512fp16:
	cmpl $0, avx512fp16_available(%rip)
	je .Lcpu_instruction_avx512bf16
	vaddph %zmm0, %zmm0, %zmm0
.Lcpu_instruction_avx512bf16:
	cmpl $0, avx512bf16_available(%rip)
	je .Lcpu_instruction_avxvnni
	vdpbf16ps %zmm0, %zmm0, %zmm0
.Lcpu_instruction_avxvnni:
	cmpl $0, avxvnni_available(%rip)
	je .Lcpu_instruction_avxifma
	{vex} vpdpbusd %ymm0, %ymm0, %ymm0
.Lcpu_instruction_avxifma:
	cmpl $0, avxifma_available(%rip)
	je .Lcpu_instruction_avxvnniint8
	{vex} vpmadd52luq %ymm0, %ymm0, %ymm0
.Lcpu_instruction_avxvnniint8:
	cmpl $0, avxvnniint8_available(%rip)
	je .Lcpu_instruction_avxneconvert
	vpdpbssd %ymm0, %ymm0, %ymm0
.Lcpu_instruction_avxneconvert:
	cmpl $0, avxneconvert_available(%rip)
	je .Lcpu_instruction_avxvnniint16
	vbcstnebf162ps initial_xmm(%rip), %ymm0
.Lcpu_instruction_avxvnniint16:
	cmpl $0, avxvnniint16_available(%rip)
	je .Lcpu_instruction_sha512
	vpdpwsud %ymm0, %ymm0, %ymm0
.Lcpu_instruction_sha512:
	cmpl $0, sha512_available(%rip)
	je .Lcpu_instruction_sm3
	vsha512msg1 %xmm0, %ymm0
.Lcpu_instruction_sm3:
	cmpl $0, sm3_available(%rip)
	je .Lcpu_instruction_sm4
	vsm3msg1 %xmm0, %xmm0, %xmm0
.Lcpu_instruction_sm4:
	cmpl $0, sm4_available(%rip)
	je .Lcpu_instruction_avx10
	{vex} vsm4rnds4 %ymm0, %ymm0, %ymm0
.Lcpu_instruction_avx10:
	cmpl $0, avx10_available(%rip)
	je .Lcpu_instruction_sse3
	kxnorw %k1, %k1, %k1
	vaddps %xmm0, %xmm0, %xmm0{%k1}
	vaddps %ymm0, %ymm0, %ymm0{%k1}
	vaddps %zmm0, %zmm0, %zmm0{%k1}
	cmpl $0, avx10_vnni_int_available(%rip)
	je .Lcpu_instruction_sse3
	vpdpbsud %zmm0, %zmm0, %zmm0
.Lcpu_instruction_sse3:
	cmpl $0, sse3_available(%rip)
	je .Lcpu_instruction_pclmulqdq
	addsubps %xmm0, %xmm0
.Lcpu_instruction_pclmulqdq:
	cmpl $0, pclmulqdq_available(%rip)
	je .Lcpu_instruction_ssse3
	pclmulqdq $0, %xmm0, %xmm0
.Lcpu_instruction_ssse3:
	cmpl $0, ssse3_available(%rip)
	je .Lcpu_instruction_sse41
	pshufb %xmm0, %xmm0
.Lcpu_instruction_sse41:
	cmpl $0, sse41_available(%rip)
	je .Lcpu_instruction_sse42
	pmuldq %xmm0, %xmm0
.Lcpu_instruction_sse42:
	cmpl $0, sse42_available(%rip)
	je .Lcpu_instruction_popcnt
	pcmpgtq %xmm0, %xmm0
.Lcpu_instruction_popcnt:
	cmpl $0, popcnt_available(%rip)
	je .Lcpu_instruction_aes
	popcnt %rax, %rax
.Lcpu_instruction_aes:
	cmpl $0, aes_available(%rip)
	je .Lcpu_instruction_shani
	aesenc %xmm0, %xmm0
.Lcpu_instruction_shani:
	cmpl $0, shani_available(%rip)
	je .Lcpu_instruction_movbe
	sha1msg1 %xmm0, %xmm0
.Lcpu_instruction_movbe:
	cmpl $0, movbe_available(%rip)
	je .Lcpu_instruction_rdrand
	movbe initial_stack(%rip), %rax
.Lcpu_instruction_rdrand:
	cmpl $0, rdrand_available(%rip)
	je .Lcpu_instruction_bmi1
	rdrand %rax
.Lcpu_instruction_bmi1:
	cmpl $0, bmi1_available(%rip)
	je .Lcpu_instruction_bmi2
	andn %rax, %rax, %rax
.Lcpu_instruction_bmi2:
	cmpl $0, bmi2_available(%rip)
	je .Lcpu_instruction_rdseed
	bzhi %rax, %rax, %rax
.Lcpu_instruction_rdseed:
	cmpl $0, rdseed_available(%rip)
	je .Lcpu_instruction_adx
	rdseed %rax
.Lcpu_instruction_adx:
	cmpl $0, adx_available(%rip)
	je .Lcpu_instruction_lahf
	adcx %rax, %rax
.Lcpu_instruction_lahf:
	cmpl $0, lahf_available(%rip)
	je .Lcpu_instruction_abm
	lahf
	sahf
.Lcpu_instruction_abm:
	cmpl $0, abm_available(%rip)
	je .Lcpu_instruction_clflushopt
	lzcnt %rax, %rax
.Lcpu_instruction_clflushopt:
	cmpl $0, clflushopt_available(%rip)
	je .Lcpu_instruction_clwb
	clflushopt cache_line(%rip)
	sfence
.Lcpu_instruction_clwb:
	cmpl $0, clwb_available(%rip)
	je .Lcpu_instruction_clzero
	clwb cache_line(%rip)
	sfence
.Lcpu_instruction_clzero:
	cmpl $0, clzero_available(%rip)
	je .Lcpu_instruction_clflushopt_fault
	movabs $0x5a5a5a5a5a5a5a5a, %rdx
	mov %rdx, cache_line(%rip)
	mov %rdx, cache_line+8(%rip)
	mov %rdx, cache_line+16(%rip)
	mov %rdx, cache_line+24(%rip)
	mov %rdx, cache_line+32(%rip)
	mov %rdx, cache_line+40(%rip)
	mov %rdx, cache_line+48(%rip)
	mov %rdx, cache_line+56(%rip)
	lea cache_line(%rip), %rax
	.byte 0x0f, 0x01, 0xfc
	mfence
	mov $194, %r11d
	cmpq $0, cache_line(%rip)
	jne 2f
	cmpq $0, cache_line+8(%rip)
	jne 2f
	cmpq $0, cache_line+16(%rip)
	jne 2f
	cmpq $0, cache_line+24(%rip)
	jne 2f
	cmpq $0, cache_line+32(%rip)
	jne 2f
	cmpq $0, cache_line+40(%rip)
	jne 2f
	cmpq $0, cache_line+48(%rip)
	jne 2f
	cmpq $0, cache_line+56(%rip)
	jne 2f
.Lcpu_instruction_clflushopt_fault:
	cmpl $0, clflushopt_available(%rip)
	je .Lcpu_instruction_clwb_fault
	mov $195, %r11d
	mov $11, %edi
	mov $1, %esi
	mov $14, %edx
	mov $0x1000, %ecx
	lea .Lcpu_instruction_clflushopt_resume(%rip), %r8
	mov $0x1000, %r9d
	call prepare_exception
	mov $0x1000, %eax
	clflushopt (%rax)
.Lcpu_instruction_clflushopt_resume:
	call check_exception
	test %eax, %eax
	jne 2f
.Lcpu_instruction_clwb_fault:
	cmpl $0, clwb_available(%rip)
	je .Lcpu_instruction_clzero_fault
	mov $196, %r11d
	mov $11, %edi
	mov $1, %esi
	mov $14, %edx
	mov $0x1000, %ecx
	lea .Lcpu_instruction_clwb_resume(%rip), %r8
	mov $0x1000, %r9d
	call prepare_exception
	mov $0x1000, %eax
	clwb (%rax)
.Lcpu_instruction_clwb_resume:
	call check_exception
	test %eax, %eax
	jne 2f
.Lcpu_instruction_clzero_fault:
	cmpl $0, clzero_available(%rip)
	je .Lcpu_instructions_done
	mov $197, %r11d
	mov $11, %edi
	mov $1, %esi
	mov $14, %edx
	mov $0x1000, %ecx
	lea .Lcpu_instruction_clzero_resume(%rip), %r8
	mov $0x1000, %r9d
	call prepare_exception
	mov $0x1000, %eax
	.byte 0x0f, 0x01, 0xfc
.Lcpu_instruction_clzero_resume:
	call check_exception
	test %eax, %eax
	jne 2f
.Lcpu_instructions_done:
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

	.type check_signal_roundtrip, @function
check_signal_roundtrip:
	push %r12
	push %r13
	movl $0, signal_seen(%rip)
	cmpl $0, avx512_available(%rip)
	je .Lsignal_roundtrip_no_avx512
	vmovdqu64 initial_zmm15(%rip), %zmm15
	vmovdqu64 initial_zmm31(%rip), %zmm31
	kmovw initial_opmask(%rip), %k7
	jmp .Lsignal_roundtrip_fpu_ready
.Lsignal_roundtrip_no_avx512:
	cmpl $0, avx_available(%rip)
	je .Lsignal_roundtrip_no_avx
	vmovdqu initial_ymm(%rip), %ymm15
	jmp .Lsignal_roundtrip_fpu_ready
.Lsignal_roundtrip_no_avx:
	movdqu initial_xmm(%rip), %xmm15
.Lsignal_roundtrip_fpu_ready:
	ldmxcsr initial_mxcsr(%rip)
	fldcw initial_fcw(%rip)
	mov $39, %eax
	syscall
	mov %eax, %r12d
	mov $186, %eax
	syscall
	mov %eax, %r13d
	mov $234, %eax
	mov %r12d, %edi
	mov %r13d, %esi
	mov $10, %edx
	syscall
	test %rax, %rax
	jne .Lsignal_roundtrip_fail
	cmpl $1, signal_seen(%rip)
	jne .Lsignal_roundtrip_fail
	movdqu %xmm15, %xmm0
	pcmpeqb replacement_xmm(%rip), %xmm0
	pmovmskb %xmm0, %eax
	cmp $0xffff, %eax
	jne .Lsignal_roundtrip_fail
	cmpl $0, avx_available(%rip)
	je .Lsignal_roundtrip_no_ymmh
	vextractf128 $1, %ymm15, %xmm0
	pcmpeqb replacement_ymmh(%rip), %xmm0
	pmovmskb %xmm0, %eax
	cmp $0xffff, %eax
	jne .Lsignal_roundtrip_fail
.Lsignal_roundtrip_no_ymmh:
	call check_replacement_avx512
	test %eax, %eax
	jne .Lsignal_roundtrip_fail
	stmxcsr -4(%rsp)
	cmpl $0x5f80, -4(%rsp)
	jne .Lsignal_roundtrip_fail
	fnstcw -2(%rsp)
	cmpw $0x037f, -2(%rsp)
	jne .Lsignal_roundtrip_fail
	call check_pkey_signal_result
	test %eax, %eax
	jne .Lsignal_roundtrip_fail
	pop %r13
	pop %r12
	xor %eax, %eax
	ret
.Lsignal_roundtrip_fail:
	pop %r13
	pop %r12
	mov $1, %eax
	ret
	.size check_signal_roundtrip, . - check_signal_roundtrip

	.type check_replacement_fpu, @function
check_replacement_fpu:
	movdqu %xmm15, %xmm0
	pcmpeqb replacement_xmm(%rip), %xmm0
	pmovmskb %xmm0, %eax
	cmp $0xffff, %eax
	jne 11f
	cmpl $0, avx_available(%rip)
	je .Lreplacement_fpu_no_ymmh
	vextractf128 $1, %ymm15, %xmm0
	pcmpeqb replacement_ymmh(%rip), %xmm0
	pmovmskb %xmm0, %eax
	cmp $0xffff, %eax
	jne 11f
.Lreplacement_fpu_no_ymmh:
	call check_replacement_avx512
	test %eax, %eax
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

	.type check_replacement_avx512, @function
check_replacement_avx512:
	cmpl $0, avx512_available(%rip)
	je .Lreplacement_avx512_success
	kmovw %k7, %eax
	cmpw replacement_opmask(%rip), %ax
	jne .Lreplacement_avx512_fail
	vpcmpeqd replacement_zmm15(%rip), %zmm15, %k1
	kmovw %k1, %eax
	cmp $0xffff, %eax
	jne .Lreplacement_avx512_fail
	vpcmpeqd replacement_zmm31(%rip), %zmm31, %k1
	kmovw %k1, %eax
	cmp $0xffff, %eax
	jne .Lreplacement_avx512_fail
.Lreplacement_avx512_success:
	xor %eax, %eax
	ret
.Lreplacement_avx512_fail:
	mov $1, %eax
	ret
	.size check_replacement_avx512, . - check_replacement_avx512

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
	cmpl $0, avx512_available(%rip)
	je .Lsignal_handler_no_avx512
	mov expected_opmask_offset(%rip), %eax
	add $OPMASK7_OFFSET, %eax
	mov initial_opmask(%rip), %rcx
	cmpq %rcx, (%r8,%rax)
	jne 3f
	mov replacement_opmask(%rip), %rcx
	movq %rcx, (%r8,%rax)
	mov expected_zmm_hi256_offset(%rip), %eax
	add $ZMM_HI256_15_OFFSET, %eax
	lea (%r8,%rax), %rdi
	lea initial_zmm15h(%rip), %rsi
	mov $4, %ecx
	repe cmpsq
	jne 3f
	mov expected_zmm_hi256_offset(%rip), %eax
	add $ZMM_HI256_15_OFFSET, %eax
	lea (%r8,%rax), %rdi
	lea replacement_zmm15h(%rip), %rsi
	mov $4, %ecx
	rep movsq
	mov expected_hi16_zmm_offset(%rip), %eax
	add $HI16_ZMM31_OFFSET, %eax
	lea (%r8,%rax), %rdi
	lea initial_zmm31(%rip), %rsi
	mov $8, %ecx
	repe cmpsq
	jne 3f
	mov expected_hi16_zmm_offset(%rip), %eax
	add $HI16_ZMM31_OFFSET, %eax
	lea (%r8,%rax), %rdi
	lea replacement_zmm31(%rip), %rsi
	mov $8, %ecx
	rep movsq
.Lsignal_handler_no_avx512:
	testq $XFEATURE_MASK_XTILE_DATA, expected_xfeatures(%rip)
	jz 5f
	mov expected_tiledata_offset(%rip), %eax
	lea (%r8,%rax), %rdi
	lea amx_source(%rip), %rsi
	mov $128, %ecx
	repe cmpsq
	jne 3f
	mov expected_tiledata_offset(%rip), %eax
	lea (%r8,%rax), %rdi
	lea amx_replacement(%rip), %rsi
	mov $128, %ecx
	rep movsq
5:
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
	cmpl $0, avx_available(%rip)
	je .Lsignal_handler_no_ymmh
	mov expected_ymm_offset(%rip), %eax
	add $YMMH15_OFFSET, %eax
	movdqu (%r8,%rax), %xmm0
	pcmpeqb initial_ymmh(%rip), %xmm0
	pmovmskb %xmm0, %eax
	cmp $0xffff, %eax
	jne 3f
	mov expected_ymm_offset(%rip), %eax
	add $YMMH15_OFFSET, %eax
	movdqu replacement_ymmh(%rip), %xmm0
	movdqu %xmm0, (%r8,%rax)
.Lsignal_handler_no_ymmh:
	movdqu replacement_xmm(%rip), %xmm0
	movdqu %xmm0, FPSTATE_XMM15(%r8)
	movl $0x5f80, FPSTATE_MXCSR(%r8)
	movw $0x037f, FPSTATE_FCW(%r8)
	call check_cpu_identity
	test %eax, %eax
	je .Lsignal_handler_identity_ok
	mov %eax, failure_code(%rip)
	jmp 3f
.Lsignal_handler_identity_ok:
	movl $1, signal_seen(%rip)
3:
	ret
	.size signal_handler, . - signal_handler

	.type exception_handler, @function
exception_handler:
	andq $~0x40000, UCONTEXT_EFL(%rdx)
	cmpq $21, expected_trap(%rip)
	jne .Lexception_validate
	mov shstk_repair_address(%rip), %rax
	mov shstk_repair_value(%rip), %rcx
	wrssq %rcx, (%rax)
.Lexception_validate:
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
