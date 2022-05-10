// Copyright (C) 2017 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// All of our supported compilers support <immintrin.h>
#include <immintrin.h>
#define T(x)                      (QT_COMPILER_SUPPORTS_ ## x)

#if !defined(__INTEL_COMPILER) && !defined(_MSC_VER) && !defined(NO_ATTRIBUTE)
/* GCC requires attributes for a function */
#  define attribute_target(x)     __attribute__((__target__(x)))
#else
#  define attribute_target(x)
#endif

#if T(SSE2)
attribute_target("sse2") void test_sse2()
{
    __m128i a = _mm_setzero_si128();
    _mm_maskmoveu_si128(a, _mm_setzero_si128(), 0);
}
#endif

#if T(SSE3)
attribute_target("sse3") void test_sse3()
{
    __m128d a = _mm_set1_pd(6.28);
    __m128d b = _mm_set1_pd(3.14);
    __m128d result = _mm_addsub_pd(a, b);
    (void) _mm_movedup_pd(result);
}
#endif

#if T(SSSE3)
attribute_target("ssse3") void test_ssse3()
{
    __m128i a = _mm_set1_epi32(42);
    _mm_abs_epi8(a);
    (void) _mm_sign_epi16(a, _mm_set1_epi32(64));
}
#endif

#if T(SSE4_1)
attribute_target("sse4.1") void test_sse4_1()
{
    __m128 a = _mm_setzero_ps();
    _mm_ceil_ps(a);
    __m128i result = _mm_mullo_epi32(_mm_set1_epi32(42), _mm_set1_epi32(64));
    (void)result;
}
#endif

#if T(SSE4_2)
attribute_target("sse4.2") void test_sse4_2()
{
    __m128i a = _mm_setzero_si128();
    __m128i b = _mm_set1_epi32(42);
    (void) _mm_cmpestrm(a, 16, b, 16, 0);
}
#endif

#if T(AESNI)
attribute_target("aes,sse4.2") void test_aesni()
{
    __m128i a = _mm_setzero_si128();
    __m128i b = _mm_aesenc_si128(a, a);
    __m128i c = _mm_aesdec_si128(a, b);
    (void)c;
}
#endif

#if T(F16C)
attribute_target("f16c") void test_f16c()
{
    __m128i a = _mm_setzero_si128();
    __m128 b = _mm_cvtph_ps(a);
    __m256 b256 = _mm256_cvtph_ps(a);
    (void) _mm_cvtps_ph(b, 0);
    (void) _mm256_cvtps_ph(b256, 0);
}
#endif

#if T(RDRND)
attribute_target("rdrnd") int test_rdrnd()
{
    unsigned short us;
    unsigned int ui;
    if (_rdrand16_step(&us))
        return 1;
    if (_rdrand32_step(&ui))
        return 1;
#  if defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
    unsigned long long ull;
    if (_rdrand64_step(&ull))
        return 1;
#  endif
}
#endif

#if T(RDSEED)
attribute_target("rdseed") int test_rdseed()
{
    unsigned short us;
    unsigned int ui;
    if (_rdseed16_step(&us))
        return 1;
    if (_rdseed32_step(&ui))
        return 1;
#  if defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(_M_X64)
    unsigned long long ull;
    if (_rdseed64_step(&ull))
        return 1;
#  endif
}
#endif

#if T(SHANI)
attribute_target("sha") void test_shani()
{
    __m128i a = _mm_setzero_si128();
    __m128i b = _mm_sha1rnds4_epu32(a, a, 0);
    __m128i c = _mm_sha1msg1_epu32(a, b);
    __m128i d = _mm_sha256msg2_epu32(b, c);
    (void)d;
}
#endif

#if T(AVX)
attribute_target("avx") void test_avx()
{
    __m256d a = _mm256_setzero_pd();
    __m256d b = _mm256_set1_pd(42.42);
    (void) _mm256_add_pd(a, b);
}
#endif

#if T(AVX2)
attribute_target("avx2") void test_avx2()
{
    _mm256_zeroall();
    __m256i a = _mm256_setzero_si256();
    __m256i b = _mm256_and_si256(a, a);
    (void) _mm256_add_epi8(a, b);
}
#endif

#if T(AVX512F)
attribute_target("avx512f") void test_avx512f(char *ptr)
{
    /* AVX512 Foundation */
    __mmask16 m = ~1;
    __m512i i;
    __m512d d;
    __m512 f;
    i = _mm512_maskz_loadu_epi32(0, ptr);
    d = _mm512_loadu_pd((double *)ptr + 64);
    f = _mm512_loadu_ps((float *)ptr + 128);
    _mm512_mask_storeu_epi64(ptr, m, i);
    _mm512_mask_storeu_ps(ptr + 64, m, f);
    _mm512_mask_storeu_pd(ptr + 128, m, d);
}
#endif

#if T(AVX512ER)
attribute_target("avx512er") void test_avx512er()
{
    /* AVX512 Exponential and Reciprocal */
    __m512 f;
    f =  _mm512_exp2a23_round_ps(f, 8);
}
#endif

#if T(AVX512CD)
attribute_target("avx512cd") void test_avx512cd()
{
    /* AVX512 Conflict Detection */
    __mmask16 m = ~1;
    __m512i i;
    i = _mm512_maskz_conflict_epi32(m, i);
}
#endif

#if T(AVX512PF)
attribute_target("avx512pf") void test_avx512pf(void *ptr)
{
    /* AVX512 Prefetch */
    __m512i i;
    __mmask16 m = 0xf;
    _mm512_mask_prefetch_i64scatter_pd(ptr, m, i, 2, 2);
}
#endif

#if T(AVX512DQ)
attribute_target("avx512dq") void test_avx512dq()
{
    /* AVX512 Doubleword and Quadword support */
    __m512i i;
    __mmask16 m = ~1;
    m = _mm512_movepi32_mask(i);
}
#endif

#if T(AVX512BW)
attribute_target("avx512bw") void test_avx512bw(char *ptr)
{
    /* AVX512 Byte and Word support */
    __m512i i;
    __mmask16 m = ~1;
    i =  _mm512_mask_loadu_epi8(i, m, ptr - 8);
}
#endif

#if T(AVX512VL)
attribute_target("avx512vl") void test_avx512vl(char *ptr)
{
    /* AVX512 Vector Length */
    __mmask16 m = ~1;
    __m256i i2 = _mm256_maskz_loadu_epi32(0, ptr);
    _mm256_mask_storeu_epi32(ptr + 1, m, i2);
}
#endif

#if T(AVX512IFMA)
attribute_target("avx512ifma") void test_avx512ifma()
{
    /* AVX512 Integer Fused Multiply-Add */
    __m512i i;
    i = _mm512_madd52lo_epu64(i, i, i);
}
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
