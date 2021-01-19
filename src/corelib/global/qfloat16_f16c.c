/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "private/qsimd_p.h"

// The x86 F16C instructions operate on AVX registers, so AVX support is
// required. We don't need to check for __F16C__ because we this file wouldn't
// have been compiled if the support was missing in the first place, and not
// all compilers define it. Technically, we didn't need to check for __AVX__
// either.
#if !QT_COMPILER_SUPPORTS_HERE(AVX)
#  error "AVX support required"
#endif

#ifdef __cplusplus
QT_BEGIN_NAMESPACE
extern "C" {
#endif

QT_FUNCTION_TARGET(F16C)
void qFloatToFloat16_fast(quint16 *out, const float *in, qsizetype len) Q_DECL_NOEXCEPT
{
    qsizetype i = 0;
    int epilog_i;
    for (; i < len - 7; i += 8)
        _mm_storeu_si128((__m128i *)(out + i), _mm256_cvtps_ph(_mm256_loadu_ps(in + i), 0));
    if (i < len - 3) {
        _mm_storel_epi64((__m128i *)(out + i), _mm_cvtps_ph(_mm_loadu_ps(in + i), 0));
        i += 4;
    }
    // Inlining "qfloat16::qfloat16(float f)":
    for (epilog_i = 0; i < len && epilog_i < 3; ++i, ++epilog_i)
        out[i] = _mm_extract_epi16(_mm_cvtps_ph(_mm_set_ss(in[i]), 0), 0);
}

QT_FUNCTION_TARGET(F16C)
void qFloatFromFloat16_fast(float *out, const quint16 *in, qsizetype len) Q_DECL_NOEXCEPT
{
    qsizetype i = 0;
    int epilog_i;
    for (; i < len - 7; i += 8)
        _mm256_storeu_ps(out + i, _mm256_cvtph_ps(_mm_loadu_si128((const __m128i *)(in + i))));
    if (i < len - 3) {
        _mm_storeu_ps(out + i, _mm_cvtph_ps(_mm_loadl_epi64((const __m128i *)(in + i))));
        i += 4;
    }
    // Inlining "qfloat16::operator float()":
    for (epilog_i = 0; i < len && epilog_i < 3; ++i, ++epilog_i)
        out[i] = _mm_cvtss_f32(_mm_cvtph_ps(_mm_cvtsi32_si128(in[i])));
}

#ifdef __cplusplus
} // extern "C"
QT_END_NAMESPACE
#endif
