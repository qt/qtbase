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

#include <private/qdrawhelper_p.h>
#include <private/qdrawingprimitive_sse2_p.h>

#if defined(QT_COMPILER_SUPPORTS_SSE4_1)

QT_BEGIN_NAMESPACE

template<bool RGBA>
static inline void convertARGBToARGB32PM_sse4(uint *buffer, const uint *src, int count)
{
    int i = 0;
    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
    const __m128i rgbaMask = _mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15);
    const __m128i shuffleMask = _mm_setr_epi8(6, 7, 6, 7, 6, 7, 6, 7, 14, 15, 14, 15, 14, 15, 14, 15);
    const __m128i half = _mm_set1_epi16(0x0080);
    const __m128i zero = _mm_setzero_si128();

    for (; i < count - 3; i += 4) {
        __m128i srcVector = _mm_loadu_si128((const __m128i *)&src[i]);
        if (!_mm_testz_si128(srcVector, alphaMask)) {
            if (!_mm_testc_si128(srcVector, alphaMask)) {
                if (RGBA)
                    srcVector = _mm_shuffle_epi8(srcVector, rgbaMask);
                __m128i src1 = _mm_unpacklo_epi8(srcVector, zero);
                __m128i src2 = _mm_unpackhi_epi8(srcVector, zero);
                __m128i alpha1 = _mm_shuffle_epi8(src1, shuffleMask);
                __m128i alpha2 = _mm_shuffle_epi8(src2, shuffleMask);
                src1 = _mm_mullo_epi16(src1, alpha1);
                src2 = _mm_mullo_epi16(src2, alpha2);
                src1 = _mm_add_epi16(src1, _mm_srli_epi16(src1, 8));
                src2 = _mm_add_epi16(src2, _mm_srli_epi16(src2, 8));
                src1 = _mm_add_epi16(src1, half);
                src2 = _mm_add_epi16(src2, half);
                src1 = _mm_srli_epi16(src1, 8);
                src2 = _mm_srli_epi16(src2, 8);
                src1 = _mm_blend_epi16(src1, alpha1, 0x88);
                src2 = _mm_blend_epi16(src2, alpha2, 0x88);
                srcVector = _mm_packus_epi16(src1, src2);
                _mm_storeu_si128((__m128i *)&buffer[i], srcVector);
            } else {
                if (RGBA)
                    _mm_storeu_si128((__m128i *)&buffer[i], _mm_shuffle_epi8(srcVector, rgbaMask));
                else if (buffer != src)
                    _mm_storeu_si128((__m128i *)&buffer[i], srcVector);
            }
        } else {
            _mm_storeu_si128((__m128i *)&buffer[i], _mm_setzero_si128());
        }
    }

    SIMD_EPILOGUE(i, count, 3) {
        uint v = qPremultiply(src[i]);
        buffer[i] = RGBA ? RGBA2ARGB(v) : v;
    }
}

const uint *QT_FASTCALL convertARGB32ToARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToARGB32PM_sse4<false>(buffer, src, count);
    return buffer;
}

const uint *QT_FASTCALL convertRGBA8888ToARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                       const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToARGB32PM_sse4<true>(buffer, src, count);
    return buffer;
}

const uint *QT_FASTCALL convertARGB32FromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                       const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qUnpremultiply_sse4(src[i]);
    return buffer;
}

const uint *QT_FASTCALL convertRGBA8888FromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                         const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = ARGB2RGBA(qUnpremultiply_sse4(src[i]));
    return buffer;
}

const uint *QT_FASTCALL convertRGBXFromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = ARGB2RGBA(0xff000000 | qUnpremultiply_sse4(src[i]));
    return buffer;
}

template<QtPixelOrder PixelOrder>
const uint *QT_FASTCALL convertA2RGB30PMFromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                          const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qConvertArgb32ToA2rgb30_sse4<PixelOrder>(src[i]);
    return buffer;
}

template
const uint *QT_FASTCALL convertA2RGB30PMFromARGB32PM_sse4<PixelOrderBGR>(uint *buffer, const uint *src, int count,
                                                                         const QVector<QRgb> *, QDitherInfo *);
template
const uint *QT_FASTCALL convertA2RGB30PMFromARGB32PM_sse4<PixelOrderRGB>(uint *buffer, const uint *src, int count,
                                                                         const QVector<QRgb> *, QDitherInfo *);

QT_END_NAMESPACE

#endif
