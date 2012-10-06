/*
 * Copyright 2008  Veselin Georgiev,
 * anrieffNOSPAM @ mgail_DOT.com (convert to gmail)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include "utils.h"
#include "cpuid.h"

#ifdef	__x86_64__
void
exec_cpuid(uint32_t *regs)
{
#ifdef __GNUC__
	__asm __volatile(
		"	push	%%rbx\n"
		"	push	%%rcx\n"
		"	push	%%rdx\n"
		"	push	%%rdi\n"
		
		"	mov	%0,	%%rdi\n"
		
		"	mov	(%%rdi),	%%eax\n"
		"	mov	4(%%rdi),	%%ebx\n"
		"	mov	8(%%rdi),	%%ecx\n"
		"	mov	12(%%rdi),	%%edx\n"
		
		"	cpuid\n"
		
		"	movl	%%eax,	(%%rdi)\n"
		"	movl	%%ebx,	4(%%rdi)\n"
		"	movl	%%ecx,	8(%%rdi)\n"
		"	movl	%%edx,	12(%%rdi)\n"
		"	pop	%%rdi\n"
		"	pop	%%rdx\n"
		"	pop	%%rcx\n"
		"	pop	%%rbx\n"
		:
		:"rdi"(regs)
		:"memory", "eax"
	);
#else
#error	"Unsupported compiler"
#endif
}

static void
cpu_exec_cpuid(uint32_t eax, uint32_t* regs)
{
	regs[0] = eax;
	regs[1] = regs[2] = regs[3] = 0;
	exec_cpuid(regs);
}

static void
cpu_exec_cpuid_ext(uint32_t* regs)
{
	exec_cpuid(regs);
}

void
cpuid_get_raw_data(struct cpu_raw_data_t* data)
{
	unsigned i;
	for (i = 0; i < 32; i++)
		cpu_exec_cpuid(i, data->basic_cpuid[i]);
	for (i = 0; i < 32; i++)
		cpu_exec_cpuid(0x80000000 + i, data->ext_cpuid[i]);
	for (i = 0; i < 4; i++) {
		memset(data->intel_fn4[i], 0, sizeof(data->intel_fn4[i]));
		data->intel_fn4[i][0] = 4;
		data->intel_fn4[i][2] = i;
		cpu_exec_cpuid_ext(data->intel_fn4[i]);
	}
}

void
cpuid_basic_identify(processor_info_t *pc)
{
	struct cpu_raw_data_t raw;
	cpuid_get_raw_data(&raw);

	memcpy(raw.vendor_str + 0, &raw.basic_cpuid[0][1], 4);
	memcpy(raw.vendor_str + 4, &raw.basic_cpuid[0][3], 4);
	memcpy(raw.vendor_str + 8, &raw.basic_cpuid[0][2], 4);
	raw.vendor_str[12] = 0;
	pc->avx_level = 0;
	pc->sse_level = 0;

	if (strcmp(raw.vendor_str, "GenuineIntel") == 0) {
		pc->proc_type = PROC_X64_INTEL;

		pc->sse_level = 2;
		if (raw.basic_cpuid[0][0] >= 1) {
			// ECX has SSE 4.2 and AVX flags
			// Bit 20 is SSE 4.2 and bit 28 indicates AVX
			if (raw.basic_cpuid[1][2] & (1 << 20)) {
				pc->sse_level = 4;
			} else {
				pc->sse_level = 3;
			}
			pc->avx_level = 0;
			if (raw.basic_cpuid[1][2] & (1 << 28)) {
				pc->avx_level = 1;
			}
		}
	} else if (strcmp(raw.vendor_str, "AuthenticAMD") == 0) {
		pc->proc_type = PROC_X64_AMD;
		pc->sse_level = 2;
	}
}

#endif