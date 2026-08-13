// SPDX-License-Identifier: GPL-2.0-only

#include <cpuid.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#include <asm/prctl.h>
#include <linux/kvm.h>
#include <linux/ptrace.h>

#include "kvm_util.h"
#include "test_util.h"

static void assert_ioctl_errno(int fd, unsigned long request, void *arg,
			       int expected_errno)
{
	int ret;

	errno = 0;
	ret = ioctl(fd, request, arg);
	TEST_ASSERT(ret == -1 && errno == expected_errno,
		    "ioctl %#lx returned %d/%d, expected -1/%d",
		    request, ret, errno, expected_errno);
}

static void assert_ioctl_noarg_errno(int fd, unsigned long request,
				     int expected_errno)
{
	int ret;

	errno = 0;
	ret = ioctl(fd, request, 0);
	TEST_ASSERT(ret == -1 && errno == expected_errno,
		    "ioctl %#lx returned %d/%d, expected -1/%d",
		    request, ret, errno, expected_errno);
}

static int create_context_with_features(int kvm_fd, __u64 required_features)
{
	struct kvm_protected_task_create create = {
		.size = sizeof(create),
		.required_features = required_features,
	};
	int fd;

	fd = ioctl(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create);
	TEST_ASSERT(fd >= 0, KVM_IOCTL_ERROR(KVM_CREATE_PROTECTED_TASK, fd));
	TEST_ASSERT_EQ(create.supported_features,
		       KVM_PROTECTED_TASK_FEATURE_EXEC);
	TEST_ASSERT(fcntl(fd, F_GETFD) & FD_CLOEXEC,
		    "Protected-task context fd must be close-on-exec");
	return fd;
}

static int create_context(int kvm_fd)
{
	return create_context_with_features(kvm_fd, 0);
}

static struct kvm_protected_task_info get_info(int fd)
{
	struct kvm_protected_task_info info = {
		.size = sizeof(info),
	};
	int ret;

	ret = ioctl(fd, KVM_PT_GET_INFO, &info);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_GET_INFO, ret));
	return info;
}

struct protected_avx_features {
	bool sse3;
	bool pclmulqdq;
	bool ssse3;
	bool sse41;
	bool sse42;
	bool popcnt;
	bool aes;
	bool shani;
	bool movbe;
	bool rdrand;
	bool bmi1;
	bool bmi2;
	bool rdseed;
	bool adx;
	bool lahf;
	bool abm;
	bool rdpid;
	bool rdtscp;
	bool clflushopt;
	bool clwb;
	bool clzero;
	bool avx;
	bool fma;
	bool f16c;
	bool avx2;
	bool avx512;
	bool avx512dq;
	bool avx512ifma;
	bool avx512pf;
	bool avx512er;
	bool avx512cd;
	bool avx512bw;
	bool avx512vl;
	bool avx512vbmi;
	bool avx512vbmi2;
	bool gfni;
	bool vaes;
	bool vpclmulqdq;
	bool avx512vnni;
	bool avx512bitalg;
	bool avx512vpopcntdq;
	bool avx5124vnniw;
	bool avx5124fmaps;
	bool avx512vp2intersect;
	bool avx512fp16;
	bool avx512bf16;
	bool avxvnni;
	bool avxifma;
	bool avxvnniint8;
	bool avxneconvert;
	bool avxvnniint16;
	bool sha512;
	bool sm3;
	bool sm4;
	bool avx10;
	bool avx10_vnni_int;
	bool amx_bf16;
	bool amx_int8;
	bool amx_fp16;
	bool amx_complex;
	bool pku;
	bool amx;
	bool shstk;
	__u8 avx10_version;
	__u32 ymm_offset;
	__u32 opmask_offset;
	__u32 zmm_hi256_offset;
	__u32 hi16_zmm_offset;
	__u32 pkru_offset;
	__u32 tilecfg_offset;
	__u32 tiledata_offset;
};

static bool kvm_supported_xstate_component(
		const struct kvm_cpuid_entry2 *entry, __u32 size, bool xfd,
		__u32 *end)
{
	return entry->eax == size && entry->ebx >= 576 &&
		!(entry->ecx & (1U << 0)) && !!(entry->ecx & (1U << 2)) == xfd &&
		!__builtin_add_overflow(entry->ebx, entry->eax, end);
}

static struct protected_avx_features get_kvm_supported_avx_features(int kvm_fd)
{
	struct {
		__u32 nent;
		__u32 padding;
		struct kvm_cpuid_entry2 entries[256];
	} cpuid = {
		.nent = 256,
	};
	struct protected_avx_features features = {};
	struct kvm_cpuid_entry2 avx10_0 = {}, avx10_1 = {};
	bool avx = false, avx512 = false, avx512_xstate = false;
	bool avx10 = false, have_avx10_0 = false, have_avx10_1 = false;
	bool amx_controls = false, amx_palette0 = false, amx_palette1 = false;
	bool amx_tile = false, amx_tmul = false, tilecfg = false, tiledata = false;
	bool hi16_zmm = false, opmask = false, ymm = false;
	bool pku = false, pkru_component = false, xsave = false;
	bool shstk = false, shstk_component = false, shstk_controls = false;
	bool ymm_component = false, zmm_hi256 = false;
	__u32 xfeatures = 0;
	__u32 hi16_zmm_end = 0, opmask_end = 0, ymm_end = 0;
	__u32 tilecfg_end = 0, tiledata_end = 0, zmm_hi256_end = 0;
	unsigned int i;
	int ret;

	ret = ioctl(kvm_fd, KVM_GET_SUPPORTED_CPUID, &cpuid);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_GET_SUPPORTED_CPUID, ret));
	for (i = 0; i < cpuid.nent; i++) {
		struct kvm_cpuid_entry2 *entry = &cpuid.entries[i];

		if (entry->function == 1 && !entry->index) {
			features.sse3 = entry->ecx & (1U << 0);
			features.pclmulqdq = entry->ecx & (1U << 1);
			features.ssse3 = entry->ecx & (1U << 9);
			features.sse41 = entry->ecx & (1U << 19);
			features.sse42 = entry->ecx & (1U << 20);
			features.movbe = entry->ecx & (1U << 22);
			features.popcnt = entry->ecx & (1U << 23);
			features.aes = entry->ecx & (1U << 25);
			features.rdrand = entry->ecx & (1U << 30);
			xsave = entry->ecx & (1U << 26);
			avx = (entry->ecx & ((1U << 26) | (1U << 28))) ==
				((1U << 26) | (1U << 28));
			features.fma = entry->ecx & (1U << 12);
			features.f16c = entry->ecx & (1U << 29);
		} else if (entry->function == 7 && !entry->index) {
			features.bmi1 = entry->ebx & (1U << 3);
			features.bmi2 = entry->ebx & (1U << 8);
			features.rdseed = entry->ebx & (1U << 18);
			features.adx = entry->ebx & (1U << 19);
			features.clflushopt = entry->ebx & (1U << 23);
			features.clwb = entry->ebx & (1U << 24);
			features.shani = entry->ebx & (1U << 29);
			features.avx2 = entry->ebx & (1U << 5);
			avx512 = entry->ebx & (1U << 16);
			features.avx512dq = entry->ebx & (1U << 17);
			features.avx512ifma = entry->ebx & (1U << 21);
			features.avx512pf = entry->ebx & (1U << 26);
			features.avx512er = entry->ebx & (1U << 27);
			features.avx512cd = entry->ebx & (1U << 28);
			features.avx512bw = entry->ebx & (1U << 30);
			features.avx512vl = entry->ebx & (1U << 31);
			features.avx512vbmi = entry->ecx & (1U << 1);
			features.avx512vbmi2 = entry->ecx & (1U << 6);
			features.gfni = entry->ecx & (1U << 8);
			features.vaes = entry->ecx & (1U << 9);
			features.vpclmulqdq = entry->ecx & (1U << 10);
			features.avx512vnni = entry->ecx & (1U << 11);
			features.avx512bitalg = entry->ecx & (1U << 12);
			features.avx512vpopcntdq = entry->ecx & (1U << 14);
			features.rdpid = entry->ecx & (1U << 22);
			pku = entry->ecx & (1U << 3);
			shstk = entry->ecx & (1U << 7);
			features.avx5124vnniw = entry->edx & (1U << 2);
			features.avx5124fmaps = entry->edx & (1U << 3);
			features.avx512vp2intersect = entry->edx & (1U << 8);
			features.avx512fp16 = entry->edx & (1U << 23);
			features.amx_bf16 = entry->edx & (1U << 22);
			amx_tile = entry->edx & (1U << 24);
			features.amx_int8 = entry->edx & (1U << 25);
		} else if (entry->function == 7 && entry->index == 1) {
			features.sha512 = entry->eax & (1U << 0);
			features.sm3 = entry->eax & (1U << 1);
			features.sm4 = entry->eax & (1U << 2);
			features.avxvnni = entry->eax & (1U << 4);
			features.avx512bf16 = entry->eax & (1U << 5);
			features.avxifma = entry->eax & (1U << 23);
			features.amx_fp16 = entry->eax & (1U << 21);
			features.avxvnniint8 = entry->edx & (1U << 4);
			features.avxneconvert = entry->edx & (1U << 5);
			features.amx_complex = entry->edx & (1U << 8);
			features.avxvnniint16 = entry->edx & (1U << 10);
			avx10 = entry->edx & (1U << 19);
		} else if (entry->function == 0x80000001) {
			features.lahf = entry->ecx & (1U << 0);
			features.abm = entry->ecx & (1U << 5);
			features.rdtscp = entry->edx & (1U << 27);
		} else if (entry->function == 0x80000008) {
			features.clzero = entry->ebx & (1U << 0);
		} else if (entry->function == 0xd && !entry->index) {
			xfeatures = entry->eax;
			ymm = entry->eax & (1U << 2);
			avx512_xstate = (entry->eax & (0xe0U)) == 0xe0U;
		} else if (entry->function == 0xd && entry->index == 2) {
			ymm_component = kvm_supported_xstate_component(entry, 256, false,
								 &ymm_end);
			features.ymm_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 5) {
			opmask = kvm_supported_xstate_component(entry, 64, false,
							       &opmask_end);
			features.opmask_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 6) {
			zmm_hi256 = kvm_supported_xstate_component(entry, 512, false,
								  &zmm_hi256_end);
			features.zmm_hi256_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 7) {
			hi16_zmm = kvm_supported_xstate_component(entry, 1024, false,
								 &hi16_zmm_end);
			features.hi16_zmm_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 9) {
			pkru_component = entry->eax && entry->ebx >= 576;
			features.pkru_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 1) {
			amx_controls = (entry->eax & 0x1cU) == 0x1cU;
			shstk_controls = (entry->eax & (1U << 3)) &&
				(entry->ecx & (1U << 11));
		} else if (entry->function == 0xd && entry->index == 11) {
			shstk_component = entry->eax == 16 &&
				(entry->ecx & (1U << 0));
		} else if (entry->function == 0xd && entry->index == 17) {
			tilecfg = kvm_supported_xstate_component(entry, 64, false,
								 &tilecfg_end);
			features.tilecfg_offset = entry->ebx;
		} else if (entry->function == 0xd && entry->index == 18) {
			tiledata = kvm_supported_xstate_component(entry, 8192, true,
								  &tiledata_end);
			features.tiledata_offset = entry->ebx;
		} else if (entry->function == 0x1d && !entry->index) {
			amx_palette0 = entry->eax >= 1;
		} else if (entry->function == 0x1d && entry->index == 1) {
			amx_palette1 = entry->eax == 0x04002000 &&
				entry->ebx == 0x00080040 && entry->ecx == 16 &&
				!entry->edx;
		} else if (entry->function == 0x1e && !entry->index) {
			amx_tmul = entry->eax <= 1 && entry->ebx == 0x4010 &&
				!entry->ecx && !entry->edx;
		} else if (entry->function == 0x24 && !entry->index) {
			avx10_0 = *entry;
			have_avx10_0 = true;
		} else if (entry->function == 0x24 && entry->index == 1) {
			avx10_1 = *entry;
			have_avx10_1 = true;
		}
	}

	features.avx = avx && ymm && ymm_component;
	features.avx512 = features.avx && avx512 && avx512_xstate &&
		opmask && zmm_hi256 && hi16_zmm &&
		opmask_end - 64 >= ymm_end &&
		zmm_hi256_end - 512 >= opmask_end &&
		hi16_zmm_end - 1024 >= zmm_hi256_end;
	features.pku = xsave && pku && (xfeatures & (1U << 9)) &&
		pkru_component;
	features.amx = amx_tile &&
		(xfeatures & 0x60000U) == 0x60000U && amx_controls &&
		tilecfg && tiledata && features.tiledata_offset >= tilecfg_end &&
		amx_palette0 && amx_palette1 && amx_tmul;
	if (features.avx512 && avx10 && have_avx10_0) {
		__u8 version = avx10_0.ebx & 0xff;

		features.avx10 = version >= 1 && version <= 2 &&
			(avx10_0.ebx & 0x70000U) == 0x70000U &&
			!(avx10_0.ebx & ~(0x70000U | 0xffU)) &&
			avx10_0.eax <= 1 && !avx10_0.ecx && !avx10_0.edx &&
			(version < 2 || avx10_0.eax == 1) &&
			(!avx10_0.eax ||
			 (have_avx10_1 && !avx10_1.eax && !avx10_1.ebx &&
			  !(avx10_1.ecx & ~4U) && !avx10_1.edx));
		if (features.avx10) {
			features.avx10_version = version;
			features.avx10_vnni_int = avx10_0.eax &&
				(avx10_1.ecx & 4U);
		}
	}
	if (!features.amx)
		features.amx_bf16 = features.amx_int8 =
			features.amx_fp16 = features.amx_complex = false;
	if (shstk && shstk_controls && shstk_component) {
		unsigned long status;

		features.shstk = syscall(SYS_arch_prctl, ARCH_SHSTK_STATUS,
					 &status) == 0;
	}
	if (!features.avx)
		features.fma = features.f16c = features.avx2 =
			features.gfni = features.vaes = features.vpclmulqdq =
			features.avxvnni = features.avxifma =
			features.avxvnniint8 = features.avxneconvert =
			features.avxvnniint16 = features.sha512 =
			features.sm3 = features.sm4 = false;
	if (!features.avx512)
		features.avx512dq = features.avx512ifma =
			features.avx512pf = features.avx512er =
			features.avx512cd = features.avx512bw =
			features.avx512vl = features.avx512vbmi =
			features.avx512vbmi2 =
			features.avx512vnni = features.avx512bitalg =
			features.avx512vpopcntdq = features.avx5124vnniw =
			features.avx5124fmaps = features.avx512vp2intersect =
			features.avx512fp16 = features.avx512bf16 = false;

	return features;
}

static void append_expected_output(
		char *output, size_t output_size, size_t *length,
		const char *message)
{
	size_t message_length = strlen(message);

	TEST_ASSERT(message_length <= output_size - *length,
		    "Protected-task expected output is too long");
	memcpy(output + *length, message, message_length);
	*length += message_length;
}

static void append_expected_avx_profile(
		char *output, size_t output_size, size_t *length,
		const struct protected_avx_features *features)
{
	if (features->sse3)
		append_expected_output(output, output_size, length,
				       "protected task sse3\n");
	if (features->pclmulqdq)
		append_expected_output(output, output_size, length,
				       "protected task pclmulqdq\n");
	if (features->ssse3)
		append_expected_output(output, output_size, length,
				       "protected task ssse3\n");
	if (features->sse41)
		append_expected_output(output, output_size, length,
				       "protected task sse4_1\n");
	if (features->sse42)
		append_expected_output(output, output_size, length,
				       "protected task sse4_2\n");
	if (features->popcnt)
		append_expected_output(output, output_size, length,
				       "protected task popcnt\n");
	if (features->aes)
		append_expected_output(output, output_size, length,
				       "protected task aes\n");
	if (features->shani)
		append_expected_output(output, output_size, length,
				       "protected task sha_ni\n");
	if (features->movbe)
		append_expected_output(output, output_size, length,
				       "protected task movbe\n");
	if (features->rdrand)
		append_expected_output(output, output_size, length,
				       "protected task rdrand\n");
	if (features->bmi1)
		append_expected_output(output, output_size, length,
				       "protected task bmi1\n");
	if (features->bmi2)
		append_expected_output(output, output_size, length,
				       "protected task bmi2\n");
	if (features->rdseed)
		append_expected_output(output, output_size, length,
				       "protected task rdseed\n");
	if (features->adx)
		append_expected_output(output, output_size, length,
				       "protected task adx\n");
	if (features->lahf)
		append_expected_output(output, output_size, length,
				       "protected task lahf\n");
	if (features->abm)
		append_expected_output(output, output_size, length,
				       "protected task abm\n");
	if (features->rdpid)
		append_expected_output(output, output_size, length,
				       "protected task rdpid\n");
	if (features->rdtscp)
		append_expected_output(output, output_size, length,
				       "protected task rdtscp\n");
	if (features->clflushopt)
		append_expected_output(output, output_size, length,
				       "protected task clflushopt\n");
	if (features->clwb)
		append_expected_output(output, output_size, length,
				       "protected task clwb\n");
	if (features->clzero)
		append_expected_output(output, output_size, length,
				       "protected task clzero\n");
	if (features->avx)
		append_expected_output(output, output_size, length,
				       "protected task avx\n");
	if (features->fma)
		append_expected_output(output, output_size, length,
				       "protected task fma\n");
	if (features->f16c)
		append_expected_output(output, output_size, length,
				       "protected task f16c\n");
	if (features->avx2)
		append_expected_output(output, output_size, length,
				       "protected task avx2\n");
	if (features->avx512)
		append_expected_output(output, output_size, length,
				       "protected task avx512\n");
	if (features->avx512dq)
		append_expected_output(output, output_size, length,
				       "protected task avx512dq\n");
	if (features->avx512ifma)
		append_expected_output(output, output_size, length,
				       "protected task avx512ifma\n");
	if (features->avx512pf)
		append_expected_output(output, output_size, length,
				       "protected task avx512pf\n");
	if (features->avx512er)
		append_expected_output(output, output_size, length,
				       "protected task avx512er\n");
	if (features->avx512cd)
		append_expected_output(output, output_size, length,
				       "protected task avx512cd\n");
	if (features->avx512bw)
		append_expected_output(output, output_size, length,
				       "protected task avx512bw\n");
	if (features->avx512vl)
		append_expected_output(output, output_size, length,
				       "protected task avx512vl\n");
	if (features->avx512vbmi)
		append_expected_output(output, output_size, length,
				       "protected task avx512vbmi\n");
	if (features->avx512vbmi2)
		append_expected_output(output, output_size, length,
				       "protected task avx512vbmi2\n");
	if (features->gfni)
		append_expected_output(output, output_size, length,
				       "protected task gfni\n");
	if (features->vaes)
		append_expected_output(output, output_size, length,
				       "protected task vaes\n");
	if (features->vpclmulqdq)
		append_expected_output(output, output_size, length,
				       "protected task vpclmulqdq\n");
	if (features->avx512vnni)
		append_expected_output(output, output_size, length,
				       "protected task avx512vnni\n");
	if (features->avx512bitalg)
		append_expected_output(output, output_size, length,
				       "protected task avx512bitalg\n");
	if (features->avx512vpopcntdq)
		append_expected_output(output, output_size, length,
				       "protected task avx512vpopcntdq\n");
	if (features->avx5124vnniw)
		append_expected_output(output, output_size, length,
				       "protected task avx5124vnniw\n");
	if (features->avx5124fmaps)
		append_expected_output(output, output_size, length,
				       "protected task avx5124fmaps\n");
	if (features->avx512vp2intersect)
		append_expected_output(output, output_size, length,
				       "protected task avx512vp2intersect\n");
	if (features->avx512fp16)
		append_expected_output(output, output_size, length,
				       "protected task avx512fp16\n");
	if (features->avx512bf16)
		append_expected_output(output, output_size, length,
				       "protected task avx512bf16\n");
	if (features->avxvnni)
		append_expected_output(output, output_size, length,
				       "protected task avxvnni\n");
	if (features->avxifma)
		append_expected_output(output, output_size, length,
				       "protected task avxifma\n");
	if (features->avxvnniint8)
		append_expected_output(output, output_size, length,
				       "protected task avxvnniint8\n");
	if (features->avxneconvert)
		append_expected_output(output, output_size, length,
				       "protected task avxneconvert\n");
	if (features->avxvnniint16)
		append_expected_output(output, output_size, length,
				       "protected task avxvnniint16\n");
	if (features->sha512)
		append_expected_output(output, output_size, length,
				       "protected task sha512\n");
	if (features->sm3)
		append_expected_output(output, output_size, length,
				       "protected task sm3\n");
	if (features->sm4)
		append_expected_output(output, output_size, length,
				       "protected task sm4\n");
	if (features->avx10_version == 1)
		append_expected_output(output, output_size, length,
				       "protected task avx10_v1\n");
	else if (features->avx10_version == 2)
		append_expected_output(output, output_size, length,
				       "protected task avx10_v2\n");
	if (features->avx10_vnni_int)
		append_expected_output(output, output_size, length,
				       "protected task avx10_vnni_int\n");
	if (features->amx_bf16)
		append_expected_output(output, output_size, length,
				       "protected task amx_bf16\n");
	if (features->amx_int8)
		append_expected_output(output, output_size, length,
				       "protected task amx_int8\n");
	if (features->amx_fp16)
		append_expected_output(output, output_size, length,
				       "protected task amx_fp16\n");
	if (features->amx_complex)
		append_expected_output(output, output_size, length,
				       "protected task amx_complex\n");
}

static void test_create_validation(int kvm_fd)
{
	struct {
		struct kvm_protected_task_create create;
		__u64 extra;
	} extended = {
		.create.size = sizeof(extended),
	};
	struct kvm_protected_task_create create = {
		.size = offsetof(struct kvm_protected_task_create,
				 supported_features),
	};
	int fd;

	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create, EINVAL);

	create.size = sizeof(create);
	create.flags = 1;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create, EINVAL);

	create.flags = 0;
	create.reserved[0] = 1;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create, EINVAL);

	create.reserved[0] = 0;
	create.supported_features = 1;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create, EINVAL);

	create.supported_features = 0;
	create.required_features = KVM_PROTECTED_TASK_FEATURE_EXEC;
	fd = ioctl(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create);
	TEST_ASSERT(fd >= 0, KVM_IOCTL_ERROR(KVM_CREATE_PROTECTED_TASK, fd));
	TEST_ASSERT_EQ(create.supported_features,
		       KVM_PROTECTED_TASK_FEATURE_EXEC);
	close(fd);

	create.supported_features = 0;
	create.required_features = 1ULL << 63;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &create,
			   EOPNOTSUPP);
	TEST_ASSERT_EQ(create.supported_features,
		       KVM_PROTECTED_TASK_FEATURE_EXEC);

	extended.extra = 1;
	assert_ioctl_errno(kvm_fd, KVM_CREATE_PROTECTED_TASK, &extended, E2BIG);

	extended.extra = 0;
	fd = ioctl(kvm_fd, KVM_CREATE_PROTECTED_TASK, &extended);
	TEST_ASSERT(fd >= 0, KVM_IOCTL_ERROR(KVM_CREATE_PROTECTED_TASK, fd));
	close(fd);
}

static void test_context_validation(int fd)
{
	struct kvm_protected_task_info info = {
		.size = sizeof(info),
		.features = 1,
	};
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm.size),
	};

	assert_ioctl_errno(fd, KVM_PT_GET_INFO, &info, EINVAL);
	info.features = 0;
	info.context_id = 1;
	assert_ioctl_errno(fd, KVM_PT_GET_INFO, &info, EINVAL);
	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EINVAL);

	arm.size = sizeof(arm);
	arm.flags = 1;
	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EINVAL);

	arm.flags = 0;
	arm.reserved[0] = 1;
	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EINVAL);
	assert_ioctl_noarg_errno(fd, KVM_PT_CANCEL_ARM, ENOENT);
}

static pthread_barrier_t thread_barrier;

static void barrier_wait(void)
{
	int ret;

	ret = pthread_barrier_wait(&thread_barrier);
	TEST_ASSERT(ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD,
		    "pthread_barrier_wait() failed: %d", ret);
}

static void *thread_fn(void *arg)
{
	barrier_wait();
	barrier_wait();
	return NULL;
}

static void test_multithreaded_arm(int fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	pthread_t thread;
	int ret;

	ret = pthread_barrier_init(&thread_barrier, NULL, 2);
	TEST_ASSERT(ret == 0, "pthread_barrier_init() failed: %d", ret);
	ret = pthread_create(&thread, NULL, thread_fn, NULL);
	TEST_ASSERT(ret == 0, "pthread_create() failed: %d", ret);

	barrier_wait();
	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EBUSY);
	barrier_wait();

	ret = pthread_join(thread, NULL);
	TEST_ASSERT(ret == 0, "pthread_join() failed: %d", ret);
	ret = pthread_barrier_destroy(&thread_barrier);
	TEST_ASSERT(ret == 0, "pthread_barrier_destroy() failed: %d", ret);
}

struct clone_vm_sync {
	int ready[2];
	int done[2];
};

static int clone_vm_fn(void *arg)
{
	struct clone_vm_sync *sync = arg;
	char value = 0;

	TEST_ASSERT(write(sync->ready[1], &value, sizeof(value)) == sizeof(value),
		    "Failed to signal CLONE_VM readiness: %d", errno);
	TEST_ASSERT(read(sync->done[0], &value, sizeof(value)) == sizeof(value),
		    "Failed to wait for CLONE_VM completion: %d", errno);
	return 0;
}

static void test_shared_mm_arm(int fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct clone_vm_sync sync;
	const size_t stack_size = 1 << 20;
	char value;
	void *stack;
	int status;
	pid_t child;

	TEST_ASSERT(pipe(sync.ready) == 0, "pipe() failed: %d", errno);
	TEST_ASSERT(pipe(sync.done) == 0, "pipe() failed: %d", errno);
	stack = mmap(NULL, stack_size, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	TEST_ASSERT(stack != MAP_FAILED, "mmap() failed: %d", errno);

	child = clone(clone_vm_fn, stack + stack_size, CLONE_VM | SIGCHLD, &sync);
	TEST_ASSERT(child >= 0, "clone(CLONE_VM) failed: %d", errno);
	TEST_ASSERT(read(sync.ready[0], &value, sizeof(value)) == sizeof(value),
		    "Failed to wait for CLONE_VM readiness: %d", errno);

	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EBUSY);

	TEST_ASSERT(write(sync.done[1], &value, sizeof(value)) == sizeof(value),
		    "Failed to release CLONE_VM child: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "CLONE_VM child failed: %#x", status);
	TEST_ASSERT(munmap(stack, stack_size) == 0, "munmap() failed: %d", errno);

	close(sync.ready[0]);
	close(sync.ready[1]);
	close(sync.done[0]);
	close(sync.done[1]);
}

static void test_fork_does_not_inherit_arm(int fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct kvm_protected_task_info info;
	int status, ret;
	pid_t child;

	ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		info = get_info(fd);
		_exit(info.flags ? 1 : 0);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Child inherited protected-task arm state: %#x", status);

	info = get_info(fd);
	TEST_ASSERT(info.flags & KVM_PROTECTED_TASK_INFO_ARMED,
		    "Parent lost protected-task arm state across fork");
	ret = ioctl(fd, KVM_PT_CANCEL_ARM, 0);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_CANCEL_ARM, ret));
}

static void test_arm_cancel_and_exec(int fd, int other_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct kvm_protected_task_info info;
	char *const argv[] = { "/bin/false", NULL };
	int ret;

	ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
	info = get_info(fd);
	TEST_ASSERT(info.flags & KVM_PROTECTED_TASK_INFO_ARMED,
		    "Context is not reported as armed");

	assert_ioctl_errno(fd, KVM_PT_ARM_EXEC, &arm, EBUSY);
	assert_ioctl_noarg_errno(other_fd, KVM_PT_CANCEL_ARM, ENOENT);

	ret = ioctl(fd, KVM_PT_CANCEL_ARM, 0);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_CANCEL_ARM, ret));
	TEST_ASSERT(!(get_info(fd).flags & KVM_PROTECTED_TASK_INFO_ARMED),
		    "Context remains armed after cancellation");

	ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
	errno = 0;
	ret = execv(argv[0], argv);
	TEST_ASSERT(ret == -1 && errno == EOPNOTSUPP,
		    "Armed exec returned %d/%d, expected -1/%d",
		    ret, errno, EOPNOTSUPP);
	TEST_ASSERT(!(get_info(fd).flags & KVM_PROTECTED_TASK_INFO_ARMED),
		    "Failed exec did not consume one-shot arm state");
}

static void test_close_while_armed(int fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	char *const argv[] = { "/bin/false", NULL };
	int ret;

	ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
	TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
	close(fd);

	errno = 0;
	ret = execv(argv[0], argv);
	TEST_ASSERT(ret == -1 && errno == EOPNOTSUPP,
		    "Exec after closing armed context returned %d/%d, expected -1/%d",
		    ret, errno, EOPNOTSUPP);
}

static void test_io_permissions_reject_exec(int kvm_fd)
{
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	int status;
	pid_t child;

	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret, saved_errno;

		if (ioperm(0x80, 1, 1) < 0) {
			TEST_ASSERT(errno == EPERM,
				    "ioperm() failed: %d", errno);
			_exit(0);
		}

		fd = create_context_with_features(kvm_fd,
					  KVM_PROTECTED_TASK_FEATURE_EXEC);
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		errno = 0;
		ret = execl("/proc/self/exe", "protected_task_test",
			    "--io-permission-exec-target", NULL);
		saved_errno = errno;
		TEST_ASSERT(ioperm(0x80, 1, 0) == 0,
			    "ioperm() cleanup failed: %d", errno);
		TEST_ASSERT(ret == -1 && saved_errno == EOPNOTSUPP,
			    "Protected exec with I/O permissions returned %d/%d, expected -1/%d",
			    ret, saved_errno, EOPNOTSUPP);
		TEST_ASSERT(!(get_info(fd).flags & KVM_PROTECTED_TASK_INFO_ARMED),
			    "Rejected I/O-permission exec did not consume arm state");
		close(fd);
		_exit(0);
	}

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "I/O-permission exec rejection failed: %#x", status);
}

static void get_exec_helper_path(char path[PATH_MAX])
{
	static const char helper[] = "protected_task_exec";
	char *name;
	ssize_t length;

	length = readlink("/proc/self/exe", path, PATH_MAX - 1);
	TEST_ASSERT(length > 0, "readlink(/proc/self/exe) failed: %d", errno);
	path[length] = '\0';
	name = strrchr(path, '/');
	TEST_ASSERT(name, "Selftest executable path has no directory");
	name++;
	TEST_ASSERT(sizeof(helper) <= PATH_MAX - (name - path),
		    "Protected exec helper path is too long");
	memcpy(name, helper, sizeof(helper));
}

static size_t get_hidden_mappings(pid_t pid, unsigned long *addresses,
				  size_t max_addresses)
{
	char path[64], line[256], permissions[5], extra;
	unsigned long address, end, inode;
	size_t n = 0;
	FILE *maps;

	snprintf(path, sizeof(path), "/proc/%d/maps", pid);
	maps = fopen(path, "re");
	TEST_ASSERT(maps, "fopen(%s) failed: %d", path, errno);
	while (fgets(line, sizeof(line), maps)) {
		int fields;

		fields = sscanf(line, "%lx-%lx %4s %*s %*s %lu %c",
				&address, &end, permissions, &inode, &extra);
		if (fields != 4 || strcmp(permissions, "---p") || inode ||
		    end - address <= 1UL << 20)
			continue;
		TEST_ASSERT(n < max_addresses,
			    "Too many hidden mappings in task %d", pid);
		addresses[n++] = address;
	}
	TEST_ASSERT(fclose(maps) == 0, "fclose(%s) failed: %d", path, errno);
	return n;
}

static void test_hidden_mapping_ptrace(pid_t child, unsigned long address)
{
	long value;
	int status;

	TEST_ASSERT(ptrace(PTRACE_ATTACH, child, NULL, NULL) == 0,
		    "PTRACE_ATTACH failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after PTRACE_ATTACH failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status), "Ptraced child did not stop: %#x", status);

	errno = 0;
	value = ptrace(PTRACE_PEEKDATA, child, (void *)address, NULL);
	TEST_ASSERT(value == -1 && errno == EIO,
		    "PTRACE_PEEKDATA returned %ld/%d, expected -1/%d",
		    value, errno, EIO);
	TEST_ASSERT(ptrace(PTRACE_DETACH, child, NULL, NULL) == 0,
		    "PTRACE_DETACH failed: %d", errno);
}

static void test_hardware_breakpoint_ptrace(pid_t child, unsigned long address,
					    int address_fd,
					    const struct protected_avx_features *features)
{
	const unsigned long sentinel = 0x0123456789abcdef;
	const unsigned char xmm_sentinel[16] = {
		0x98, 0xba, 0xdc, 0xfe, 0x10, 0x32, 0x54, 0x76,
		0xfe, 0xca, 0xad, 0x0b, 0xef, 0xbe, 0xad, 0xde,
	};
	const size_t xstate_bv_offset = 512;
	const size_t xmm15_offset =
		offsetof(struct user_fpregs_struct, xmm_space) + 15 * 16;
	const size_t mxcsr_offset = offsetof(struct user_fpregs_struct, mxcsr);
	unsigned int eax, ebx, xstate_capacity, edx;
	struct user_regs_struct original_regs, regs;
	struct ptrace_syscall_info syscall_info;
	unsigned char *expected_xstate, *original_xstate, *xstate;
	unsigned long breakpoint_rip, event_child;
	__u64 opmask = 0x5aa5;
	__u64 xstate_bv;
	__u32 mxcsr = 0x5f80, pkru = 3U << 30;
	__u16 fcw = 0x077f;
	struct iovec iov;
	size_t xstate_size;
	siginfo_t siginfo;
	long dr6, instruction, value;
	int status;

	__cpuid_count(0xd, 0, eax, ebx, xstate_capacity, edx);
	TEST_ASSERT(xstate_capacity >= xmm15_offset + sizeof(xmm_sentinel),
		    "Invalid xstate buffer size: %u", xstate_capacity);
	xstate = malloc(xstate_capacity);
	expected_xstate = malloc(xstate_capacity);
	original_xstate = malloc(xstate_capacity);
	TEST_ASSERT(xstate && expected_xstate && original_xstate,
		    "Failed to allocate xstate buffers");

	TEST_ASSERT(ptrace(PTRACE_ATTACH, child, NULL, NULL) == 0,
		    "PTRACE_ATTACH failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after PTRACE_ATTACH failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status), "Ptraced child did not stop: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_SETOPTIONS, child, NULL,
			   PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK) == 0,
		    "PTRACE_SETOPTIONS failed: %d", errno);

	errno = 0;
	value = ptrace(PTRACE_PEEKDATA, child, (void *)address, NULL);
	TEST_ASSERT(value == -1 && errno == EIO,
		    "PTRACE_PEEKDATA returned %ld/%d, expected -1/%d",
		    value, errno, EIO);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS failed: %d", errno);
	original_regs = regs;
	iov.iov_base = xstate;
	iov.iov_len = xstate_capacity;
	TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_GETREGSET NT_X86_XSTATE failed: %d", errno);
	xstate_size = iov.iov_len;
	TEST_ASSERT(xstate_size >= xmm15_offset + sizeof(xmm_sentinel),
		    "NT_X86_XSTATE is too short: %zu", xstate_size);
	TEST_ASSERT(xstate_size >= xstate_bv_offset + sizeof(xstate_bv),
		    "NT_X86_XSTATE has no header: %zu", xstate_size);
	memcpy(original_xstate, xstate, xstate_size);
	memcpy(&xstate_bv, xstate + xstate_bv_offset, sizeof(xstate_bv));
	xstate_bv |= 3;
	memcpy(xstate + xstate_bv_offset, &xstate_bv, sizeof(xstate_bv));
	memcpy(xstate, &fcw, sizeof(fcw));
	memcpy(xstate + mxcsr_offset, &mxcsr, sizeof(mxcsr));
	memcpy(xstate + xmm15_offset, xmm_sentinel, sizeof(xmm_sentinel));
	if (features->avx) {
		TEST_ASSERT(xstate_size >= features->ymm_offset + 256,
			    "NT_X86_XSTATE has no YMM component: %zu", xstate_size);
		xstate_bv |= 1ULL << 2;
		memset(xstate + features->ymm_offset + 15 * 16, 0xa5, 16);
	}
	if (features->avx512) {
		TEST_ASSERT(xstate_size >= features->opmask_offset + 64 &&
			    xstate_size >= features->zmm_hi256_offset + 512 &&
			    xstate_size >= features->hi16_zmm_offset + 1024,
			    "NT_X86_XSTATE has incomplete AVX-512 state: %zu",
			    xstate_size);
		xstate_bv |= 0xe0;
		memcpy(xstate + features->opmask_offset + 7 * 8,
		       &opmask, sizeof(opmask));
		memset(xstate + features->zmm_hi256_offset + 15 * 32, 0x3c, 32);
		memset(xstate + features->hi16_zmm_offset + 15 * 64, 0xc3, 64);
	}
	if (features->pku) {
		TEST_ASSERT(xstate_size >= features->pkru_offset + sizeof(pkru),
			    "NT_X86_XSTATE has no PKRU component: %zu", xstate_size);
		xstate_bv |= 1ULL << 9;
		memcpy(xstate + features->pkru_offset, &pkru, sizeof(pkru));
	}
	memcpy(xstate + xstate_bv_offset, &xstate_bv, sizeof(xstate_bv));
	memcpy(expected_xstate, xstate, xstate_size);
	TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_SETREGSET NT_X86_XSTATE failed: %d", errno);
	iov.iov_len = xstate_capacity;
	TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_GETREGSET after xstate write failed: %d", errno);
	TEST_ASSERT(iov.iov_len == xstate_size,
		    "NT_X86_XSTATE size changed from %zu to %zu",
		    xstate_size, iov.iov_len);
	TEST_ASSERT(!memcmp(xstate, expected_xstate, xstate_size),
		    "PTRACE_SETREGSET did not update complete xstate");
	memcpy(expected_xstate, xstate, xstate_size);
	breakpoint_rip = regs.rip;
	regs.r15 = sentinel;
	regs.r14 = 0x1122334455667788;
	regs.r13 = 0x2233445566778899;
	regs.r12 = 0x33445566778899aa;
	regs.rbp = 0x445566778899aabb;
	regs.rbx = 0x5566778899aabbcc;
	regs.r11 = 0x66778899aabbccdd;
	regs.r10 = 0x778899aabbccddee;
	regs.r9 = 0x8899aabbccddeeff;
	regs.r8 = 0x99aabbccddeeff00;
	regs.rax = 0xaabbccddeeff0011;
	regs.rcx = 0xbbccddeeff001122;
	regs.rdx = 0xccddeeff00112233;
	regs.rsi = 0xddeeff0011223344;
	regs.rdi = 0xeeff001122334455;
	regs.rsp -= 16;
	TEST_ASSERT(ptrace(PTRACE_SETREGS, child, NULL, &regs) == 0,
		    "PTRACE_SETREGS failed: %d", errno);
	{
		struct user_regs_struct expected_regs = regs;

		TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
			    "PTRACE_GETREGS after write failed: %d", errno);
		TEST_ASSERT(!memcmp(&regs, &expected_regs,
				    offsetof(struct user_regs_struct, orig_rax)) &&
			    regs.rsp == expected_regs.rsp,
			    "PTRACE_SETREGS did not update complete GPR state");
	}
	errno = 0;
	instruction = ptrace(PTRACE_PEEKTEXT, child, (void *)breakpoint_rip, NULL);
	TEST_ASSERT(instruction != -1 || !errno,
		    "PTRACE_PEEKTEXT failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_POKETEXT, child, (void *)breakpoint_rip,
			   (void *)((instruction & ~0xffUL) | 0xcc)) == 0,
		    "PTRACE_POKETEXT breakpoint failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT to software breakpoint failed: %d", errno);
	TEST_ASSERT(write(address_fd, &address, sizeof(address)) == sizeof(address),
		    "Failed to release software-breakpoint target: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after software breakpoint failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
		    "Software breakpoint produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0,
		    "PTRACE_GETSIGINFO after software breakpoint failed: %d", errno);
	TEST_ASSERT(siginfo.si_signo == SIGTRAP && siginfo.si_code == SI_KERNEL,
		    "Software breakpoint produced signal %d/%d",
		    siginfo.si_signo, siginfo.si_code);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS after software breakpoint failed: %d", errno);
	TEST_ASSERT(regs.r14 == 0x1122334455667788 &&
		    regs.r13 == 0x2233445566778899 &&
		    regs.r12 == 0x33445566778899aa &&
		    regs.rbp == 0x445566778899aabb &&
		    regs.rbx == 0x5566778899aabbcc &&
		    regs.r11 == 0x66778899aabbccdd &&
		    regs.r10 == 0x778899aabbccddee &&
		    regs.r9 == 0x8899aabbccddeeff &&
		    regs.r8 == 0x99aabbccddeeff00 &&
		    regs.rax == 0xaabbccddeeff0011 &&
		    regs.rcx == 0xbbccddeeff001122 &&
		    regs.rdx == 0xccddeeff00112233 &&
		    regs.rsi == 0xddeeff0011223344 &&
		    regs.rdi == 0xeeff001122334455 &&
		    regs.r15 == sentinel && regs.rsp == original_regs.rsp - 16,
		    "Complete GPR state changed at software breakpoint");
	TEST_ASSERT(regs.rip == breakpoint_rip + 1,
		    "Software breakpoint produced RIP %#llx, expected %#lx",
		    regs.rip, breakpoint_rip + 1);
	TEST_ASSERT(ptrace(PTRACE_POKETEXT, child, (void *)breakpoint_rip,
			   (void *)instruction) == 0,
		    "PTRACE_POKETEXT restore failed: %d", errno);
	regs = original_regs;
	regs.r15 = sentinel;
	TEST_ASSERT(ptrace(PTRACE_SETREGS, child, NULL, &regs) == 0,
		    "PTRACE_SETREGS after software breakpoint failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_POKEUSER, child,
			   (void *)offsetof(struct user, u_debugreg[0]),
			   breakpoint_rip) == 0,
		    "PTRACE_POKEUSER DR0 failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_POKEUSER, child,
			   (void *)offsetof(struct user, u_debugreg[7]), 1) == 0,
		    "PTRACE_POKEUSER DR7 failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after hardware breakpoint failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
		    "Hardware breakpoint produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0,
		    "PTRACE_GETSIGINFO failed: %d", errno);
	TEST_ASSERT(siginfo.si_signo == SIGTRAP && siginfo.si_code == TRAP_HWBKPT,
		    "Hardware breakpoint produced signal %d/%d",
		    siginfo.si_signo, siginfo.si_code);
	errno = 0;
	dr6 = ptrace(PTRACE_PEEKUSER, child,
		     (void *)offsetof(struct user, u_debugreg[6]), NULL);
	TEST_ASSERT(dr6 != -1 && (dr6 & 1),
		    "Hardware breakpoint DR6 is %#lx/%d", dr6, errno);
	TEST_ASSERT(ptrace(PTRACE_POKEUSER, child,
			   (void *)offsetof(struct user, u_debugreg[7]), 0) == 0,
		    "Clearing DR7 failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_POKEUSER, child,
			   (void *)offsetof(struct user, u_debugreg[0]), 0) == 0,
		    "Clearing DR0 failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_SINGLESTEP, child, NULL, NULL) == 0,
		    "PTRACE_SINGLESTEP failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() after single-step failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
		    "Single-step produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0,
		    "PTRACE_GETSIGINFO after single-step failed: %d", errno);
	TEST_ASSERT(siginfo.si_signo == SIGTRAP && siginfo.si_code == TRAP_TRACE,
		    "Single-step produced signal %d/%d",
		    siginfo.si_signo, siginfo.si_code);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS after single-step failed: %d", errno);
	TEST_ASSERT(regs.rip != breakpoint_rip,
		    "Single-step did not advance RIP from %#lx", breakpoint_rip);
	TEST_ASSERT(regs.r15 == sentinel,
		    "PTRACE_SETREGS value was not preserved: %#llx",
		    regs.r15);
	regs.r15 = original_regs.r15;
	TEST_ASSERT(ptrace(PTRACE_SETREGS, child, NULL, &regs) == 0,
		    "PTRACE_SETREGS GPR restore failed: %d", errno);
	iov.iov_len = xstate_capacity;
	TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_GETREGSET after single-step failed: %d", errno);
	TEST_ASSERT(iov.iov_len == xstate_size,
		    "NT_X86_XSTATE size changed after single-step: %zu", iov.iov_len);
	TEST_ASSERT(!memcmp(xstate, expected_xstate, xstate_size),
		    "NT_X86_XSTATE changed after single-step");
	iov.iov_base = original_xstate;
	iov.iov_len = xstate_size;
	TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_SETREGSET xstate restore failed: %d", errno);
	iov.iov_base = xstate;
	iov.iov_len = xstate_capacity;
	TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
			   &iov) == 0,
		    "PTRACE_GETREGSET after xstate restore failed: %d", errno);
	TEST_ASSERT(iov.iov_len == xstate_size &&
		    !memcmp(xstate, original_xstate, xstate_size),
		    "PTRACE_SETREGSET did not restore complete xstate");
	TEST_ASSERT(ptrace(PTRACE_SYSCALL, child, NULL, NULL) == 0,
		    "PTRACE_SYSCALL to entry failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at syscall entry failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | 0x80),
		    "Syscall entry produced unexpected status: %#x", status);
	memset(&syscall_info, 0, sizeof(syscall_info));
	value = ptrace(PTRACE_GET_SYSCALL_INFO, child, sizeof(syscall_info),
		       &syscall_info);
	TEST_ASSERT(value > 0, "PTRACE_GET_SYSCALL_INFO at entry failed: %d", errno);
	TEST_ASSERT(syscall_info.op == PTRACE_SYSCALL_INFO_ENTRY &&
		    syscall_info.entry.nr == SYS_write &&
		    syscall_info.entry.args[0] == STDOUT_FILENO &&
		    syscall_info.entry.args[1] == address &&
		    syscall_info.entry.args[2] == 1,
		    "Unexpected syscall entry: op=%u nr=%llu args=%llu/%#llx/%llu",
		    syscall_info.op, syscall_info.entry.nr,
		    syscall_info.entry.args[0], syscall_info.entry.args[1],
		    syscall_info.entry.args[2]);
	TEST_ASSERT(ptrace(PTRACE_SYSCALL, child, NULL, NULL) == 0,
		    "PTRACE_SYSCALL to exit failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at syscall exit failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == (SIGTRAP | 0x80),
		    "Syscall exit produced unexpected status: %#x", status);
	memset(&syscall_info, 0, sizeof(syscall_info));
	value = ptrace(PTRACE_GET_SYSCALL_INFO, child, sizeof(syscall_info),
		       &syscall_info);
	TEST_ASSERT(value > 0, "PTRACE_GET_SYSCALL_INFO at exit failed: %d", errno);
	TEST_ASSERT(syscall_info.op == PTRACE_SYSCALL_INFO_EXIT &&
		    syscall_info.exit.is_error &&
		    syscall_info.exit.rval == -EFAULT,
		    "Unexpected syscall exit: op=%u error=%u rval=%lld",
		    syscall_info.op, syscall_info.exit.is_error,
		    syscall_info.exit.rval);
	TEST_ASSERT(kill(child, SIGUSR1) == 0,
		    "kill(SIGUSR1) failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT to signal stop failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at signal stop failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGUSR1,
		    "Signal delivery produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0,
		    "PTRACE_GETSIGINFO at signal stop failed: %d", errno);
	TEST_ASSERT(siginfo.si_signo == SIGUSR1 && siginfo.si_code == SI_USER &&
		    siginfo.si_pid == getpid(),
		    "Signal stop produced signal %d/%d from %d",
		    siginfo.si_signo, siginfo.si_code, siginfo.si_pid);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT to fork event failed: %d", errno);
	for (;;) {
		TEST_ASSERT(waitpid(child, &status, 0) == child,
			    "waitpid() before fork event failed: %d", errno);
		TEST_ASSERT(WIFSTOPPED(status),
			    "Tracee exited before fork event: %#x", status);
		if (WSTOPSIG(status) == SIGTRAP &&
		    (unsigned int)status >> 16 == PTRACE_EVENT_FORK)
			break;
		TEST_ASSERT((unsigned int)status >> 16 == 0,
			    "Unexpected ptrace event before fork: %#x", status);
		TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL,
				   (void *)(long)WSTOPSIG(status)) == 0,
			    "PTRACE_CONT before fork event failed: %d", errno);
	}
	TEST_ASSERT(ptrace(PTRACE_GETEVENTMSG, child, NULL, &event_child) == 0,
		    "PTRACE_GETEVENTMSG at fork failed: %d", errno);
	TEST_ASSERT(event_child > 0, "Fork event returned child %lu", event_child);
	TEST_ASSERT(waitpid(event_child, &status, __WALL) == event_child,
		    "waitpid() for traced fork child failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status),
		    "Traced fork child did not stop: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_DETACH, event_child, NULL, NULL) == 0,
		    "PTRACE_DETACH fork child failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_DETACH, child, NULL, NULL) == 0,
		    "PTRACE_DETACH failed: %d", errno);
	free(expected_xstate);
	free(original_xstate);
	free(xstate);
}

static void test_exec_event_ptrace(pid_t child, unsigned long address,
				    int address_fd,
				    const struct protected_avx_features *features)
{
	struct user_regs_struct regs;
	unsigned long event_msg;
	long value;
	int status;

	TEST_ASSERT(ptrace(PTRACE_ATTACH, child, NULL, NULL) == 0,
		    "PTRACE_ATTACH before exec failed: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() before exec failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status),
		    "Ptraced exec child did not stop: %#x", status);
	errno = 0;
	value = ptrace(PTRACE_PEEKDATA, child, (void *)address, NULL);
	TEST_ASSERT(value == -1 && errno == EIO,
		    "PTRACE_PEEKDATA before exec returned %ld/%d, expected -1/%d",
		    value, errno, EIO);
	if (features->amx) {
		unsigned char *expected_xstate, *original_xstate, *xstate;
		unsigned int eax, ebx, xstate_capacity, edx;
		const size_t xstate_bv_offset = 512;
		__u64 xstate_bv;
		struct iovec iov;
		siginfo_t siginfo;
		size_t xstate_size;
		unsigned int i;

		__cpuid_count(0xd, 0, eax, ebx, xstate_capacity, edx);
		xstate = malloc(xstate_capacity);
		expected_xstate = malloc(xstate_capacity);
		original_xstate = malloc(xstate_capacity);
		TEST_ASSERT(xstate && expected_xstate && original_xstate,
			    "Failed to allocate AMX xstate buffers");
		iov.iov_base = xstate;
		iov.iov_len = xstate_capacity;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0,
			    "PTRACE_GETREGSET before AMX write failed: %d", errno);
		xstate_size = iov.iov_len;
		TEST_ASSERT(xstate_size >= features->tilecfg_offset + 64 &&
			    xstate_size >= features->tiledata_offset + 8192,
			    "NT_X86_XSTATE has incomplete AMX state: %zu", xstate_size);
		memcpy(original_xstate, xstate, xstate_size);
		memset(xstate + features->tilecfg_offset, 0, 64);
		xstate[features->tilecfg_offset] = 1;
		for (i = 0; i < 8; i++) {
			__u16 colsb = 64;

			memcpy(xstate + features->tilecfg_offset + 16 + i * 2,
			       &colsb, sizeof(colsb));
			xstate[features->tilecfg_offset + 48 + i] = 16;
		}
		memset(xstate + features->tiledata_offset, 0xa5, 8192);
		memcpy(&xstate_bv, xstate + xstate_bv_offset, sizeof(xstate_bv));
		xstate_bv |= 0x60000;
		memcpy(xstate + xstate_bv_offset, &xstate_bv, sizeof(xstate_bv));
		memcpy(expected_xstate, xstate, xstate_size);
		iov.iov_len = xstate_size;
		TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0,
			    "PTRACE_SETREGSET AMX write failed: %d", errno);
		iov.iov_len = xstate_capacity;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0 && iov.iov_len == xstate_size &&
			    !memcmp(xstate, expected_xstate, xstate_size),
			    "PTRACE_SETREGSET did not update complete AMX state");
		TEST_ASSERT(ptrace(PTRACE_SINGLESTEP, child, NULL, NULL) == 0,
			    "PTRACE_SINGLESTEP with AMX state failed: %d", errno);
		TEST_ASSERT(waitpid(child, &status, 0) == child,
			    "waitpid() after AMX single-step failed: %d", errno);
		TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
			    "AMX single-step produced unexpected status: %#x", status);
		TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0 &&
			    siginfo.si_code == TRAP_TRACE,
			    "AMX single-step produced unexpected siginfo: %d/%d",
			    siginfo.si_signo, siginfo.si_code);
		iov.iov_len = xstate_capacity;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0 && iov.iov_len == xstate_size &&
			    !memcmp(xstate, expected_xstate, xstate_size),
			    "Complete AMX state changed after single-step");
		iov.iov_base = original_xstate;
		iov.iov_len = xstate_size;
		TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0,
			    "PTRACE_SETREGSET AMX restore failed: %d", errno);
		iov.iov_base = xstate;
		iov.iov_len = xstate_capacity;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_XSTATE,
				   &iov) == 0 && iov.iov_len == xstate_size &&
			    !memcmp(xstate, original_xstate, xstate_size),
			    "PTRACE_SETREGSET did not restore complete AMX state");
		free(original_xstate);
		free(expected_xstate);
		free(xstate);
	}
	if (features->shstk) {
		unsigned long original_ssp, ssp;
		struct iovec iov = {
			.iov_base = &original_ssp,
			.iov_len = sizeof(original_ssp),
		};
		siginfo_t siginfo;

		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0 && iov.iov_len == sizeof(original_ssp),
			    "PTRACE_GETREGSET NT_X86_SHSTK failed: %d", errno);
		TEST_ASSERT(original_ssp && original_ssp <= ULONG_MAX - 8,
			    "Invalid protected SSP: %#lx", original_ssp);
		ssp = original_ssp + 8;
		iov.iov_base = &ssp;
		iov.iov_len = sizeof(ssp);
		TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0,
			    "PTRACE_SETREGSET NT_X86_SHSTK failed: %d", errno);
		ssp = 0;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0 && ssp == original_ssp + 8,
			    "PTRACE_SETREGSET did not update SSP");
		TEST_ASSERT(ptrace(PTRACE_SINGLESTEP, child, NULL, NULL) == 0,
			    "PTRACE_SINGLESTEP with edited SSP failed: %d", errno);
		TEST_ASSERT(waitpid(child, &status, 0) == child,
			    "waitpid() after SSP single-step failed: %d", errno);
		TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP,
			    "SSP single-step produced unexpected status: %#x", status);
		TEST_ASSERT(ptrace(PTRACE_GETSIGINFO, child, NULL, &siginfo) == 0 &&
			    siginfo.si_code == TRAP_TRACE,
			    "SSP single-step produced unexpected siginfo: %d/%d",
			    siginfo.si_signo, siginfo.si_code);
		ssp = 0;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0 && ssp == original_ssp + 8,
			    "Protected SSP changed after single-step");
		iov.iov_base = &original_ssp;
		TEST_ASSERT(ptrace(PTRACE_SETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0,
			    "PTRACE_SETREGSET SSP restore failed: %d", errno);
		ssp = 0;
		iov.iov_base = &ssp;
		TEST_ASSERT(ptrace(PTRACE_GETREGSET, child, (void *)NT_X86_SHSTK,
				   &iov) == 0 && ssp == original_ssp,
			    "PTRACE_SETREGSET did not restore SSP");
	}
	TEST_ASSERT(ptrace(PTRACE_SETOPTIONS, child, NULL,
			   PTRACE_O_TRACEEXEC) == 0,
		    "PTRACE_SETOPTIONS before exec failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_CONT, child, NULL, NULL) == 0,
		    "PTRACE_CONT to exec event failed: %d", errno);
	TEST_ASSERT(write(address_fd, &address, sizeof(address)) == sizeof(address),
		    "Failed to release exec-event target: %d", errno);
	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() at exec event failed: %d", errno);
	TEST_ASSERT(WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP &&
		    (unsigned int)status >> 16 == PTRACE_EVENT_EXEC,
		    "Exec event produced unexpected status: %#x", status);
	TEST_ASSERT(ptrace(PTRACE_GETEVENTMSG, child, NULL, &event_msg) == 0,
		    "PTRACE_GETEVENTMSG at exec failed: %d", errno);
	TEST_ASSERT(ptrace(PTRACE_GETREGS, child, NULL, &regs) == 0,
		    "PTRACE_GETREGS at exec failed: %d", errno);
	TEST_ASSERT(regs.rip, "Exec event has a zero RIP");
	TEST_ASSERT(ptrace(PTRACE_DETACH, child, NULL, NULL) == 0,
		    "PTRACE_DETACH after exec failed: %d", errno);
}

static void test_protected_exec(int kvm_fd)
{
	enum {
		ADDRESS_FD = 100,
		READY_FD = 101,
		CONTROL_FD = 102,
	};
	struct kvm_protected_task_arm arm = {
		.size = sizeof(arm),
	};
	struct protected_avx_features features;
	char expected_output[4096] = {}, output[sizeof(expected_output)] = {};
	char helper[PATH_MAX];
	unsigned long main_address = 0;
	size_t expected_length = 0, nread = 0;
	int address_pipe[2], pipefd[2], ready_pipe[2];
	int status;
	pid_t child;

	features = get_kvm_supported_avx_features(kvm_fd);
	append_expected_avx_profile(expected_output, sizeof(expected_output),
				    &expected_length, &features);
	append_expected_output(expected_output, sizeof(expected_output),
			       &expected_length, "protected task exec\n");
	append_expected_avx_profile(expected_output, sizeof(expected_output),
				    &expected_length, &features);
	get_exec_helper_path(helper);
	TEST_ASSERT(pipe(pipefd) == 0, "pipe() failed: %d", errno);
	TEST_ASSERT(pipe(address_pipe) == 0, "pipe() failed: %d", errno);
	TEST_ASSERT(pipe(ready_pipe) == 0, "pipe() failed: %d", errno);
	child = fork();
	TEST_ASSERT(child >= 0, "fork() failed: %d", errno);
	if (!child) {
		int fd, ret;

		close(pipefd[0]);
		close(address_pipe[1]);
		close(ready_pipe[0]);
		TEST_ASSERT(dup2(pipefd[1], STDOUT_FILENO) == STDOUT_FILENO,
			    "dup2() failed: %d", errno);
		TEST_ASSERT(dup2(address_pipe[0], ADDRESS_FD) == ADDRESS_FD,
			    "dup2() failed: %d", errno);
		TEST_ASSERT(dup2(ready_pipe[1], READY_FD) == READY_FD,
			    "dup2() failed: %d", errno);
		close(pipefd[1]);
		close(address_pipe[0]);
		close(ready_pipe[1]);
		fd = create_context_with_features(kvm_fd, KVM_PROTECTED_TASK_FEATURE_EXEC);
		if (fd != CONTROL_FD) {
			TEST_ASSERT(dup3(fd, CONTROL_FD, O_CLOEXEC) == CONTROL_FD,
				    "dup3() failed: %d", errno);
			close(fd);
			fd = CONTROL_FD;
		}
		ret = ioctl(fd, KVM_PT_ARM_EXEC, &arm);
		TEST_ASSERT(ret == 0, KVM_IOCTL_ERROR(KVM_PT_ARM_EXEC, ret));
		execl(helper, helper, NULL);
		_exit(127);
	}

	close(pipefd[1]);
	close(address_pipe[0]);
	close(ready_pipe[1]);
	for (int i = 0; i < 12; i++) {
		unsigned long address, addresses[2];
		ssize_t nread;
		size_t n;
		pid_t target;
		int stage = i % 6;

		do {
			errno = 0;
			nread = read(ready_pipe[0], &target, sizeof(target));
		} while (nread < 0 && errno == EINTR);
		if (nread != sizeof(target)) {
			int saved_errno = errno;

			TEST_ASSERT(waitpid(child, &status, 0) == child,
				    "waitpid() after checkpoint failure failed: %d", errno);
			TEST_FAIL("Protected helper stopped before checkpoint %d: read=%zd errno=%d status=%#x",
				  i, nread, saved_errno, status);
		}
		n = get_hidden_mappings(target, addresses,
					sizeof(addresses) / sizeof(addresses[0]));
		if (stage == 0) {
			TEST_ASSERT(target == child && n == 1,
				    "Initial helper has unexpected mappings");
			main_address = address = addresses[0];
		} else if (stage == 1 || stage == 3) {
			TEST_ASSERT(target != child && n == 1,
				    "Process child has unexpected mappings");
			address = addresses[0];
		} else if (stage == 2) {
			TEST_ASSERT(target != child && n == 2,
				    "vfork child has unexpected mappings");
			address = addresses[addresses[0] == main_address];
			TEST_ASSERT(address != main_address,
				    "vfork hidden mapping is not distinct");
		} else if (stage == 4) {
			TEST_ASSERT(target == child && n == 2,
				    "Thread did not create a second hidden mapping");
			address = addresses[addresses[0] == main_address];
			TEST_ASSERT(address != main_address,
				    "Thread hidden mapping is not distinct");
		} else {
			TEST_ASSERT(target == child && n == 1,
				    "Post-thread helper %d has %zu hidden mappings, expected task %d with one",
				    target, n, child);
			address = addresses[0];
		}
		if (stage == 0)
			test_hardware_breakpoint_ptrace(target, address,
						 address_pipe[1], &features);
		else if (i == 5)
			test_exec_event_ptrace(target, address, address_pipe[1],
					       &features);
		else {
			test_hidden_mapping_ptrace(target, address);
			TEST_ASSERT(write(address_pipe[1], &address, sizeof(address)) == sizeof(address),
				    "Failed to send hidden mapping address: %d", errno);
		}
	}
	close(address_pipe[1]);
	close(ready_pipe[0]);
	while (nread < sizeof(output)) {
		ssize_t n;

		n = read(pipefd[0], output + nread, sizeof(output) - nread);
		if (n < 0 && errno == EINTR)
			continue;
		TEST_ASSERT(n >= 0, "read() failed: %d", errno);
		if (!n)
			break;
		nread += n;
	}
	close(pipefd[0]);

	TEST_ASSERT(waitpid(child, &status, 0) == child,
		    "waitpid() failed: %d", errno);
	TEST_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) == 0,
		    "Protected exec helper failed: %#x", status);
	TEST_ASSERT(nread == expected_length &&
		    !memcmp(output, expected_output, expected_length),
		    "Unexpected protected exec output");
}

int main(int argc, char *argv[])
{
	struct kvm_protected_task_info first_info, second_info;
	int kvm_fd, first_fd, second_fd;

	if (argc == 2 && !strcmp(argv[1], "--io-permission-exec-target"))
		return 42;

	kvm_fd = open_kvm_dev_path_or_exit();
	TEST_REQUIRE(ioctl(kvm_fd, KVM_CHECK_EXTENSION,
			   KVM_CAP_PROTECTED_TASK) == 1);

	test_create_validation(kvm_fd);
	first_fd = create_context(kvm_fd);
	second_fd = create_context(kvm_fd);

	first_info = get_info(first_fd);
	second_info = get_info(second_fd);
	TEST_ASSERT(first_info.context_id && second_info.context_id &&
		    first_info.context_id != second_info.context_id,
		    "Protected-task context IDs are not unique");

	test_context_validation(first_fd);
	test_multithreaded_arm(first_fd);
	test_shared_mm_arm(first_fd);
	test_fork_does_not_inherit_arm(first_fd);
	test_arm_cancel_and_exec(first_fd, second_fd);

	close(first_fd);
	test_close_while_armed(second_fd);
	test_io_permissions_reject_exec(kvm_fd);
	test_protected_exec(kvm_fd);
	close(kvm_fd);
	return 0;
}
