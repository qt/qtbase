/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the FOO module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define QGLOBAL_H
#include "../../src/corelib/global/archdetect.cpp"
#include <stdio.h>

extern const char msg[];
const char msg[] = "==Qt=magic=Qt== Architecture:" ARCH_PROCESSOR;

extern const char msg2[];
const char msg2[] = "==Qt=magic=Qt== Sub-architecture:"
        // This is the list of features found in GCC or MSVC
        // We don't use all of them, but this is ready for future expansion

// -- x86 --
#ifdef __3dNOW__
// 3dNow!, introduced with the AMD K6-2, discontinued after 2010
" 3dnow"
#endif
#ifdef __3dNOW_A__
// Athlon
" 3dnow-a"
#endif
#ifdef __ABM__
// Advanced Bit Manipulation, AMD Barcelona (family 10h)
" abm"
#endif
#ifdef __ADX__
// Multi-Precision Add-Carry Instruction Extensions, Intel Core 5th generation ("Broadwell")
" adx"
#endif
#ifdef __AES__
// AES New Instructions, Intel Core-i7 second generation ("Sandy Bridge")
" aes"
#endif
#ifdef __AVX__
// Advanced Vector Extensions, Intel Core-i7 second generation ("Sandy Bridge")
" avx"
#endif
#ifdef __AVX2__
// AVX 2, Intel Core 4th Generation ("Haswell")
" avx2"
#endif
#ifdef __AVX512F__
// AVX512 Foundation, Intel Xeon Phi codename "Knights Landing" and Intel Xeon codename "Skylake"
" avx512f"
#endif
#ifdef __AVX512CD__
// AVX512 Conflict Detection, Intel Xeon Phi codename "Knights Landing" and Intel Xeon codename "Skylake"
" avx512cd"
#endif
#ifdef __AVX512DQ__
// AVX512 Double & Quadword, Intel Xeon processor codename "Skylake"
" avx512dq"
#endif
#ifdef __AVX512BW__
// AVX512 Byte & Word, Intel Xeon processor codename "Skylake"
" avx512bw"
#endif
#ifdef __AVX512ER__
// AVX512 Exponentiation & Reciprocal, Intel Xeon Phi codename "Knights Landing"
" avx512ef"
#endif
#ifdef __AVX512PF__
// AVX512 Prefetch, Intel Xeon Phi codename "Knights Landing"
" avx512pf"
#endif
#ifdef __AVX512VL__
// AVX512 Vector Length, Intel Xeon processor codename "Skylake"
" avx512vl"
#endif
#ifdef __AVX512IFMA__
// AVX512 Integer Fused Multiply-Add, Intel processor codename "Cannonlake"
" avx512ifma"
#endif
#ifdef __AVX512VBMI__
// AVX512 Vector Byte Manipulation Instructions, Intel processor codename "Cannonlake"
" avx512vbmi"
#endif
#ifdef __BMI__
// Bit Manipulation Instructions 1, Intel Core 4th Generation ("Haswell"), AMD "Bulldozer 2"
" bmi"
#endif
#ifdef __BMI2__
// Bit Manipulation Instructions 2, Intel Core 4th Generation ("Haswell")
" bmi2"
#endif
#ifdef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_16
// cmpxchg16b instruction, Intel Pentium 4 64-bit ("Nocona"), AMD Barcelona (family 10h)
// Notably, this instruction is missing on earlier AMD Athlon 64
" cx16"
#endif
#ifdef __F16C__
// 16-bit floating point conversion, Intel Core 3rd Generation ("Ivy Bridge")
" f16c"
#endif
#ifdef __FMA__
// Fused Multiply-Add with 3 arguments, Intel Core 4th Generation ("Haswell"), AMD "Bulldozer 2"
// a.k.a. "FMA3"
" fma"
#endif
#ifdef __FMA4__
// Fused Multiply-Add with 4 arguments, AMD "Bulldozer"
" fma4"
#endif
#ifdef __FSGSBASE__
// rdfsgsbase, wrfsgsbase, Intel Core 3rd Generation ("Ivy Bridge")
" fsgsbase"
#endif
#ifdef __LWP__
// LWP instructions, AMD "Bulldozer"
" lwp"
#endif
#ifdef __LZCNT__
// Leading-Zero bit count, Intel Core 4th Generation ("Haswell")
" lzcnt"
#endif
#ifdef __MMX__
// Multimedia Extensions, Pentium MMX, AMD K6-2
" mmx"
#endif
#ifdef __MOVBE__
// Move Big Endian, Intel Atom & "Haswell"
" movbe"
#endif
#ifdef __MPX__
// Memory Protection Extensions, Intel Core processor codename "Skylake"
" mpx"
#endif
#ifdef __NO_SAHF__
// missing SAHF instruction in 64-bit, up to Intel Pentium 4 64-bit ("Nocona"), AMD Athlon FX
// Note: the macro is not defined, so this will never show up
" no-sahf"
#endif
#ifdef __PCLMUL__
// (Packed) Carry-less multiplication, Intel Core-i7 second generation ("Sandy Bridge")
" pclmul"
#endif
#ifdef __POPCNT__
// Population Count (count of set bits), Intel Core-i7 second generation ("Sandy Bridge")
" popcnt"
#endif
#ifdef __PREFETCHWT1__
// Prefetch data for writing with T1 hint, Intel processor TBA
" prefetchwt1"
#endif
#ifdef __PRFCHW__
// Prefetch data for writing, Intel Core 5th Generation ("Broadwell")
" prfchw"
#endif
#ifdef __RDRND__
// Random number generator, Intel Core 3rd Generation ("Ivy Bridge")
" rdrnd"
#endif
#ifdef __RDSEED__
// Random number generator, Intel Core 5th Generation ("Broadwell")
" rdseed"
#endif
#ifdef __RTM__
// Restricted Transactional Memory, Intel Core 4th Generation ("Haswell")
" rtm"
#endif
#ifdef __SHA__
// SHA-1 and SHA-256 instructions, Intel processor TBA
" sha"
#endif
#if defined(__SSE__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 1) || defined(_M_X64)
// Streaming SIMD Extensions, Intel Pentium III, AMD Athlon
" sse"
#endif
#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64)
// SSE2, Intel Pentium-M, Intel Pentium 4, AMD Opteron and Athlon 64
" sse2"
#endif
#ifdef __SSE3__
// SSE3, Intel Pentium 4 "Prescott", AMD Athlon 64 rev E
" sse3"
#endif
#ifdef __SSSE3__
// Supplemental SSE3, Intel Core 2 ("Merom"), AMD "Bulldozer"
" ssse3"
#endif
#ifdef __SSE4A__
// SSE4a, AMD Barcelona
" sse4a"
#endif
#ifdef __SSE4_1__
// SSE 4.1, Intel Core2 45nm shrink ("Penryn"), AMD "Bulldozer"
" sse4.1"
#endif
#ifdef __SSE4_2__
// SSE 4.2, Intel Core-i7 ("Nehalem"), AMD "Bulldozer"
" sse4.2"
// Since no processor supports SSE4.2 without 4.1 and since no Intel processor
// supports SSE4a, define "sse4" to indicate SSE4"
" sse4"
#endif
#ifdef __TBM__
// TBM, AMD "Bulldozer"
" tbm"
#endif
#ifdef __XOP__
// XOP, AMD "Bulldozer"
" xop"
#endif

// -- ARM --
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
" neon"
#endif
#ifdef __IWMMXT__
" iwmmxt"
#endif
#ifdef __ARM_FEATURE_CRC32
" crc32"
#endif

// -- SPARC --
#ifdef __VIS__
" vis"
# if __VIS__ >= 0x200
" vis2"
# endif
# if __VIS__ >= 0x300
" vis3"
# endif
#endif

// -- MIPS --
# if __mips_dsp
" dsp"
# endif
# if __mips_dspr2
" dspr2"
# endif

// -- POWER, PowerPC --
#ifdef __ALTIVEC__
" altivec"
#endif
#ifdef __SPE__
" spe"
#endif
#ifdef __VSX__
" vsx"
#endif
                   "";

extern const char msg3[];
const char msg3[] = "==Qt=magic=Qt== Build-ABI:" ARCH_FULL;

int main()
{
    puts(msg);
    puts(msg2);
    puts(msg3);
}
