// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: BSD-3-Clause

// All of our supported compilers support <immintrin.h>
#include <immintrin.h>
#define T(x)                      (QT_COMPILER_SUPPORTS_ ## x)

#if !defined(__INTEL_COMPILER) && !defined(_MSC_VER) && !defined(NO_ATTRIBUTE)
/* GCC requires attributes for a function */
#  define attribute_target(x)     __attribute__((__target__(x)))
#else
#  define attribute_target(x)
#endif

#if T(AVX512VBMI2)
attribute_target("avx512vl,avx512vbmi2") void test_avx512vbmi2()
{
    /* AVX512 Vector Byte Manipulation Instructions 2 */
    __m128i a = _mm_maskz_compress_epi16(-1, _mm_set1_epi16(1));
    __m128i b = _mm_shrdi_epi32(a, a, 7);
}
#endif

#if T(VAES)
// VAES does not require AVX512 and works on Alder Lake
attribute_target("avx2,vaes") void test_vaes()
{
    /* 256- and 512-bit AES */
    __m256i a = _mm256_set1_epi32(-1);
    __m256i b = _mm256_aesenc_epi128(a, a);
    __m256i c = _mm256_aesdec_epi128(b, a);
}
#endif

int main()
{
    return 0;
}
