/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
