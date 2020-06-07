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

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include "architecture.h"

#define MAX_LEN	24

#if defined(_WIN32) && !defined(__GNUC__)
	#pragma warning(disable:4996)
#endif

#if defined(_WIN32) && !defined(__GNUC__)
	#include <time.h>
	//#include <conio.h>
	#define TIMER_TYPE           clock_t
	#define TIMER_FUNC(t)        t = clock()
	#define TIMER_DIFF(s,e)      ((e - s) / (double)CLOCKS_PER_SEC)
	#define DIRECTORY_SEPARATOR  '\\'

	#ifndef UINT64_C
		#define UINT64_C(c)      (c)
	#endif
	#define MAX_UINT64           0xffffffffffffffffI64
	#define UINT64_MAX_UINT32    0xffffffffI64

	#define uint8     unsigned char
	#define uint16    unsigned short
	#define sint32             __int32
	#define sint64             __int64
	#define uint32    unsigned __int32
	#define uint64    unsigned __int64
	#define pfsint32  "%d"
	#define pfuint32  "%u"
	#define pfsint64  "%I64d"
	#define pfuint64  "%I64u"

	#define WIN_ALIGN_16 __declspec(align(16))
	#define ALIGN_16
	#define FSEEK64(file,off,origin)  _fseeki64(file,off,origin)
#else
	#include <sys/time.h>
	//#include <fcntl.h>
	#define TIMER_TYPE           timeval
	#define TIMER_FUNC(t)        gettimeofday(&t, NULL)
	#define TIMER_DIFF(s,e)      ((e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec) / (double)1000000.0)
	#define DIRECTORY_SEPARATOR  '/'

	#ifndef UINT64_C
		#define UINT64_C(c)      c##ull
	#endif
	#define MAX_UINT64           0xffffffffffffffffull
	#define UINT64_MAX_UINT32    0xffffffffull

	#define uint8     unsigned char
	#define uint16    unsigned short
	#define sint32             int
	#define sint64             long long
	#define uint32    unsigned int
	#define uint64    unsigned long long

	#define pfsint32  "%d"
	#define pfuint32  "%u"
	#define pfsint64  "%lld"
	#define pfuint64  "%llu"

	#define WIN_ALIGN_16
	#define ALIGN_16 __attribute__((aligned(16)))
	#define FSEEK64(file,off,origin)  fseeko64(file,off,origin)
#endif
/*
#ifdef ARC_64
	#define MAX_UINT  MAX_UINT64
	#define sint      sint64
	#define uint      uint64
	#define pfsint    pfuint64
	#define pfuint    pfuint64
#else
	#define MAX_UINT  0xffffffff
	#define sint      sint32
	#define uint      uint32
	#define pfsint    pfuint32
	#define pfuint    pfuint32
#endif
*/
#ifdef ARC_x86
	#if defined(_WIN32) && !defined(__GNUC__)
		#define MUL_32_RET_64(a32,b32,r64) \
			{ \
				__asm mov eax,a32 \
				__asm mul b32 \
				__asm mov [DWORD PTR r64],eax \
				__asm mov [DWORD PTR r64+4],edx \
			}

		#define DIV_MOD_64_RET_32_32(n64,d32,r32,q32) \
			{ \
				__asm mov eax,[DWORD PTR n64] \
				__asm mov edx,[DWORD PTR n64+4] \
				__asm div d32 \
				__asm mov q32,eax \
				__asm mov r32,edx \
			}

		#ifdef ARC_x86_64
			#define VECTOR_CLEAR_PW(pw,vectors) \
				{ \
					__asm mov    rdi,[QWORD PTR pw] \
					__asm xorps  xmm0,xmm0 \
					__asm mov    ecx,[DWORD PTR vectors] \
					__asm VECTOR_CLEAR_PW_LOOP: \
					__asm movaps [rdi],xmm0 \
					__asm movaps [rdi+16],xmm0 \
					__asm movaps [rdi+32],xmm0 \
					__asm movaps [rdi+48],xmm0 \
					__asm movaps [rdi+64],xmm0 \
					__asm add    rdi,80 \
					__asm dec    ecx \
					__asm jnz    VECTOR_CLEAR_PW_LOOP \
				}
		#else
			#define VECTOR_CLEAR_PW(pw,vectors) \
				{ \
					__asm mov    edi,[DWORD PTR pw] \
					__asm xorps  xmm0,xmm0 \
					__asm mov    ecx,[DWORD PTR vectors] \
					__asm VECTOR_CLEAR_PW_LOOP: \
					__asm movaps [edi],xmm0 \
					__asm movaps [edi+16],xmm0 \
					__asm movaps [edi+32],xmm0 \
					__asm movaps [edi+48],xmm0 \
					__asm movaps [edi+64],xmm0 \
					__asm add    edi,80 \
					__asm dec    ecx \
					__asm jnz    VECTOR_CLEAR_PW_LOOP \
				}
		#endif
	#else
		#define MUL_32_RET_64(a32,b32,r64) \
			asm( \
				"mull %2" \
				: "=a"(((uint64u*)&r64)->lo32), "=d"(((uint64u*)&r64)->hi32) \
				: "m"(b32), "a"(a32) );

		#define DIV_MOD_64_RET_32_32(n64,d32,r32,q32) \
			asm( \
				"divl %2" \
				: "=a"(q32), "=d"(r32) \
				: "rm"(d32), "a"(((uint64u*)&n64)->lo32), "d"(((uint64u*)&n64)->hi32) );

		#ifdef ARC_64
			#define VECTOR_CLEAR_PW(pw,vectors) \
				asm( \
					"movll %0,%%rdi\n\t" \
					"xorps %%xmm0,%%xmm0\n\t" \
					"movl %1,%%ecx\n\t" \
					"VECTOR_CLEAR_PW_LOOP:\n\t" \
					"movaps %%xmm0,(%%rdi)\n\t" \
					"movaps %%xmm0,16(%%rdi)\n\t" \
					"movaps %%xmm0,32(%%rdi)\n\t" \
					"movaps %%xmm0,48(%%rdi)\n\t" \
					"movaps %%xmm0,64(%%rdi)\n\t" \
					"addll $80,%%rdi\n\t" \
					"decl %%ecx\n\t" \
					"jnz VECTOR_CLEAR_PW_LOOP" \
					: \
					: "m"(pw), "rm"(vectors) \
					: "ecx", "rdi");
		#else
			#define VECTOR_CLEAR_PW(pw,vectors) \
				asm( \
					"movl %0,%%edi\n\t" \
					"xorps %%xmm0,%%xmm0\n\t" \
					"movl %1,%%ecx\n\t" \
					"VECTOR_CLEAR_PW_LOOP:\n\t" \
					"movaps %%xmm0,(%%edi)\n\t" \
					"movaps %%xmm0,16(%%edi)\n\t" \
					"movaps %%xmm0,32(%%edi)\n\t" \
					"movaps %%xmm0,48(%%edi)\n\t" \
					"movaps %%xmm0,64(%%edi)\n\t" \
					"addl $80,%%edi\n\t" \
					"decl %%ecx\n\t" \
					"jnz VECTOR_CLEAR_PW_LOOP" \
					: \
					: "m"(pw), "rm"(vectors) \
					: "ecx", "edi");
		#endif
	#endif
#else
	#define MUL_32_RET_64(a32,b32,r64) \
		r64 = (uint64) a32 * b32;

	#define DIV_MOD_64_RET_32_32(n64,d32,r32,q32) \
		r32 = n64 % d32; \
		q32 = n64 / d32;

	#define VECTOR_CLEAR_PW(pw,vectors)
#endif

union uint64u
{
	uint64 u64;
	struct
	{
#ifdef ARC_LITTLE_ENDIAN
		uint32 lo32, hi32;
#else
		uint32 hi32, lo32;
#endif
	};
};

struct vector4
{
	WIN_ALIGN_16
	uint32 data[4] ALIGN_16;
};

// You must call MMX_CLEAN_UP before you do any floating point math
#ifdef ARC_x86
	#if defined(_WIN32) && !defined(__GNUC__)
		#define MMX_CLEAN_UP __asm emms
	#else
		#define MMX_CLEAN_UP asm("emms");
	#endif
#else
	#define MMX_CLEAN_UP
#endif

// All the important ones
enum instructionSets
{
	IS_MMX    =   1,
	IS_SSE    =   2,
	IS_SSE2   =   4,
	IS_SSE41  =   8, // ptest xmm1,xmm2
	IS_AVX    =  16,
	IS_XOP    =  32, // vprotd xmmd,xmms,imm8
	IS_OS_YMM =  64,
	IS_POPCNT = 128  // popcnt r,r/m16; popcnt r,r/m32; popcnt r,r/m64
};

uint32 getInstructionSets();

#endif
