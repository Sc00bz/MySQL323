/*
	Copyright (C) 2011 Steve Thomas <steve AT tobtu DOT com>

	This file is part of MySQL323 Cracker.

	MySQL323 Cracker is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MySQL323 Cracker is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MySQL323 Cracker.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "common.h"

uint32 getInstructionSets()
{
#ifndef ARC_x86
	return 0;
#else
	static uint32 ret = 0xffffffff;

	if (ret == 0xffffffff)
	{
		uint32 out1, out2, out3, out4;

#if defined(_WIN32) && !defined(__GNUC__)
		__asm
		{
			mov   eax,0x80000001
			cpuid
			mov   eax,1
			mov   out3,ecx
			mov   out4,edx
			cpuid
			mov   out1,ecx
			mov   out2,edx
		}
#else
		asm(
			"movl   $2147483649,%%eax\n\t"
			"cpuid\n\t"
			"movl   $1,%%eax\n\t"
			"movl   %%ecx,%2\n\t"
			"movl   %%edx,%3\n\t"
			"cpuid"
			: "=c"(out1), "=d"(out2), "=m"(out3), "=m"(out4)
			:
			: "eax", "ebx");
#endif
		ret = 0;
		if (out2 & (1 << 23))
		{
			ret |= IS_MMX;
		}
		if (out2 & (1 << 25))
		{
			ret |= IS_SSE;
		}
		if (out2 & (1 << 26))
		{
			ret |= IS_SSE2;
		}
		if (out1 & (1 << 19))
		{
			ret |= IS_SSE41;
		}
		if (out3 & (1 << 11))
		{
			ret |= IS_XOP;
		}
		if (out1 & (1 << 28))
		{
			ret |= IS_AVX;
		}
		if (out1 & (1 << 23))
		{
			ret |= IS_POPCNT;
		}/*
		if (ret & IS_AVX || out1 & (1 << 27)) // OSXSAVE (XGETBV)
		{
#if defined(_WIN32) && !defined(__GNUC__)
			__asm
			{
				xor   ecx,ecx
				xgetbv
				mov   out1,eax
			}
#else
			asm(
				"xor   %%ecx,%%ecx\n\t"
				"xgetbv"
				: "=a"(out1)
				:
				: "ecx");
#endif
			if ((out1 & 0x6) == 0x6)
			{
				ret |= IS_OS_YMM;
			}
		}*/
#ifdef ARC_x86_32
		if ((out2 & (1 << 30)) || (out4 & (1 << 29)) || (out3 & (1 << 6))) // x86_64, long_mode, long_mode_intel
		{
			fprintf(stderr, "WARNING: **** Your CPU can run 64 bit which runs faster than the 32 bit code you are currently running. ****\n");
		}
#endif
	}
	return ret;
#endif
}
