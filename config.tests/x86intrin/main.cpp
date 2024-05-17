// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: BSD-3-Clause

#include <immintrin.h>

// Skylake AVX512 was added to GCC 4.9, Clang 3.7, and MSVC 2015.
// Cannon Lake was added to GCC 5, Clang 3.8, and MSVC 2017 15.7,
// so that's our minimum.
// Ice Lake was completed with GCC 8, Clang 6, and MSVC 2017 15.8.

int test(int argc, char **argv)
{
    unsigned randomvalue;
    _rdrand32_step(&randomvalue);               // RDRND (IVB)
#ifndef __QNXNTO__  // buggy compiler is missing this intrinsic, but we allow it
    _rdseed32_step(&randomvalue);               // RDSEED (BDW)
#endif
    unsigned mask = _blsmsk_u32(argc);          // BMI (HSW)
    int clz = _lzcnt_u32(mask);                 // LZCNT (HSW)
    int ctz = _tzcnt_u32(mask);                 // BMI (HSW)
    mask = _bzhi_u32(-1, argc);                 // BMI2 (HSW)

    __m128d d = _mm_setzero_pd();               // SSE2
    d = _mm_cvtsi32_sd(d, argc);                // SSE2
    __m256d d2 = _mm256_broadcastsd_pd(d);      // AVX (SNB)
    d2 = _mm256_fmadd_pd(d2, d2, d2);           // FMA (HSW)

    __m128 f = _mm256_cvtpd_ps(d2);             // AVX (SNB)
    __m128i a = _mm_cvtps_ph(f, 0);             // F16C (IVB)
    __m128i b = _mm_aesenc_si128(a, a);         // AESNI (WSM)
    __m128i c = _mm_sha1rnds4_epu32(a, a, 0);   // SHA (CNL)
    __m128i e = _mm_sha1msg1_epu32(a, b);       // SHA (CNL)
    __m128i g = _mm_sha256msg2_epu32(b, c);     // SHA (CNL)

    __m512i zero = _mm512_setzero_si512();                  // AVX512F (SKX)
    __m512i data = _mm512_maskz_loadu_epi8(mask, argv[0]);  // AVX512BW (SKX)
    __m256i ptrs = _mm256_maskz_loadu_epi64(mask, argv);    // AVX512VL (SKX)
    __m512i data2 = _mm512_broadcast_i64x4(ptrs);           // AVX512DQ (SKX)
    __m256i data3 = _mm256_madd52lo_epu64(ptrs, ptrs, ptrs);// AVX512IFMA (CNL)
    data2 = _mm512_multishift_epi64_epi8(data, data2);      // AVX512VBMI (CNL)

    return _mm256_extract_epi32(data3, 0);      // AVX2 (HSW)
}

int main(int argc, char **argv)
{
    return test(argc, argv);
}
