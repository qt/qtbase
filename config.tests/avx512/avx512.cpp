/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the configuration of the Qt Toolkit.
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

#include <immintrin.h>

#ifndef AVX512WANT
#  error ".pro file must define AVX512WANT macro to the AVX-512 feature to be tested"
#endif

// The following checks if __AVXx__ is defined, where x is the value in
// AVX512WANT
#define HAS2(x)             __AVX512 ## x ## __
#define HAS(x)              HAS2(x)
#if !HAS(AVX512WANT)
#  error "Feature not supported"
#endif

int main(int, char**argv)
{
    /* AVX512 Foundation */
    __m512i i;
    __m512d d;
    __m512 f;
    __mmask16 m = ~1;
    i = _mm512_maskz_loadu_epi32(0, argv);
    d = _mm512_loadu_pd((double *)argv + 64);
    f = _mm512_loadu_ps((float *)argv + 128);

#ifdef __AVX512ER__
    /* AVX512 Exponential and Reciprocal */
    f =  _mm512_exp2a23_round_ps(f, 8);
#endif
#ifdef __AVX512CD__
    /* AVX512 Conflict Detection */
    i = _mm512_maskz_conflict_epi32(m, i);
#endif
#ifdef __AVX512PF__
    /* AVX512 Prefetch */
    _mm512_mask_prefetch_i64scatter_pd(argv, 0xf, i, 2, 2);
#endif
#ifdef __AVX512DQ__
    /* AVX512 Doubleword and Quadword support */
    m = _mm512_movepi32_mask(i);
#endif
#ifdef __AVX512BW__
    /* AVX512 Byte and Word support */
    i =  _mm512_mask_loadu_epi8(i, m, argv - 8);
#endif
#ifdef __AVX512VL__
    /* AVX512 Vector Length */
    __m256i i2 = _mm256_maskz_loadu_epi32(0, argv);
    _mm256_mask_storeu_epi32(argv + 1, m, i2);
#endif
#ifdef __AVX512IFMA__
    /* AVX512 Integer Fused Multiply-Add */
    i = _mm512_madd52lo_epu64(i, i, i);
#endif
#ifdef __AVX512VBMI__
    /* AVX512 Vector Byte Manipulation Instructions */
    i = _mm512_permutexvar_epi8(i, i);
#endif

    _mm512_mask_storeu_epi64(argv, m, i);
    _mm512_mask_storeu_ps(argv + 64, m, f);
    _mm512_mask_storeu_pd(argv + 128, m, d);
    return 0;
}
