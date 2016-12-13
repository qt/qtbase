/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qimage.h"
#include <private/qimage_p.h>
#include <private/qsimd_p.h>
#include <private/qdrawhelper_p.h>
#include <private/qdrawingprimitive_sse2_p.h>

#ifdef QT_COMPILER_SUPPORTS_SSE2

QT_BEGIN_NAMESPACE

bool convert_ARGB_to_ARGB_PM_inplace_sse2(QImageData *data, Qt::ImageConversionFlags)
{
    Q_ASSERT(data->format == QImage::Format_ARGB32 || data->format == QImage::Format_RGBA8888);

    const int width = data->width;
    const int height = data->height;
    const int bpl = data->bytes_per_line;

    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
    const __m128i nullVector = _mm_setzero_si128();
    const __m128i half = _mm_set1_epi16(0x80);
    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);

    uchar *d = data->data;
    for (int y = 0; y < height; ++y) {
        int i = 0;
        quint32 *d32 = reinterpret_cast<quint32 *>(d);
        ALIGNMENT_PROLOGUE_16BYTES(d, i, width) {
            const quint32 p = d32[i];
            if (p <= 0x00ffffff)
                d32[i] = 0;
            else if (p < 0xff000000)
                d32[i] = qPremultiply(p);
        }
        __m128i *d128 = reinterpret_cast<__m128i *>(d32 + i);
        for (; i < (width - 3); i += 4) {
            const __m128i srcVector = _mm_load_si128(d128);
#ifdef __SSE4_1__
            if (_mm_testc_si128(srcVector, alphaMask)) {
                // opaque, data is unchanged
            } else if (_mm_testz_si128(srcVector, alphaMask)) {
                // fully transparent
                _mm_store_si128(d128, nullVector);
            } else {
                const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
#else
            const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
            if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) {
                // opaque, data is unchanged
            } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) == 0xffff) {
                // fully transparent
                _mm_store_si128(d128, nullVector);
            } else {
#endif
                __m128i alphaChannel = _mm_srli_epi32(srcVector, 24);
                alphaChannel = _mm_or_si128(alphaChannel, _mm_slli_epi32(alphaChannel, 16));

                __m128i result;
                BYTE_MUL_SSE2(result, srcVector, alphaChannel, colorMask, half);
                result = _mm_or_si128(_mm_andnot_si128(alphaMask, result), srcVectorAlpha);
                _mm_store_si128(d128, result);
            }
            d128++;
        }

        SIMD_EPILOGUE(i, width, 3) {
            const quint32 p = d32[i];
            if (p <= 0x00ffffff)
                d32[i] = 0;
            else if (p < 0xff000000)
                d32[i] = qPremultiply(p);
        }

        d += bpl;
    }

    if (data->format == QImage::Format_ARGB32)
        data->format = QImage::Format_ARGB32_Premultiplied;
    else
        data->format = QImage::Format_RGBA8888_Premultiplied;
    return true;
}

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_SSE2
