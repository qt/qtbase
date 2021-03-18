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
#include <private/qpaintengine_raster_p.h>

#if defined(QT_COMPILER_SUPPORTS_SSE4_1)

QT_BEGIN_NAMESPACE

#ifndef __AVX2__
template<bool RGBA>
static void convertARGBToARGB32PM_sse4(uint *buffer, const uint *src, int count)
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
            _mm_storeu_si128((__m128i *)&buffer[i], zero);
        }
    }

    SIMD_EPILOGUE(i, count, 3) {
        uint v = qPremultiply(src[i]);
        buffer[i] = RGBA ? RGBA2ARGB(v) : v;
    }
}

template<bool RGBA>
static void convertARGBToRGBA64PM_sse4(QRgba64 *buffer, const uint *src, int count)
{
    int i = 0;
    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
    const __m128i rgbaMask = _mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15);
    const __m128i shuffleMask = _mm_setr_epi8(6, 7, 6, 7, 6, 7, 6, 7, 14, 15, 14, 15, 14, 15, 14, 15);
    const __m128i zero = _mm_setzero_si128();

    for (; i < count - 3; i += 4) {
        __m128i srcVector = _mm_loadu_si128((const __m128i *)&src[i]);
        if (!_mm_testz_si128(srcVector, alphaMask)) {
            bool cf = _mm_testc_si128(srcVector, alphaMask);

            if (!RGBA)
                srcVector = _mm_shuffle_epi8(srcVector, rgbaMask);
            const __m128i src1 = _mm_unpacklo_epi8(srcVector, srcVector);
            const __m128i src2 = _mm_unpackhi_epi8(srcVector, srcVector);
            if (!cf) {
                __m128i alpha1 = _mm_shuffle_epi8(src1, shuffleMask);
                __m128i alpha2 = _mm_shuffle_epi8(src2, shuffleMask);
                __m128i dst1 = _mm_mulhi_epu16(src1, alpha1);
                __m128i dst2 = _mm_mulhi_epu16(src2, alpha2);
                // Map 0->0xfffe to 0->0xffff
                dst1 = _mm_add_epi16(dst1, _mm_srli_epi16(dst1, 15));
                dst2 = _mm_add_epi16(dst2, _mm_srli_epi16(dst2, 15));
                // correct alpha value:
                dst1 = _mm_blend_epi16(dst1, src1, 0x88);
                dst2 = _mm_blend_epi16(dst2, src2, 0x88);
                _mm_storeu_si128((__m128i *)&buffer[i], dst1);
                _mm_storeu_si128((__m128i *)&buffer[i + 2], dst2);
            } else {
                _mm_storeu_si128((__m128i *)&buffer[i], src1);
                _mm_storeu_si128((__m128i *)&buffer[i + 2], src2);
            }
        } else {
            _mm_storeu_si128((__m128i *)&buffer[i], zero);
            _mm_storeu_si128((__m128i *)&buffer[i + 2], zero);
        }
    }

    SIMD_EPILOGUE(i, count, 3) {
        const uint s = RGBA ? RGBA2ARGB(src[i]) : src[i];
        buffer[i] = QRgba64::fromArgb32(s).premultiplied();
    }
}
#endif // __AVX2__

static inline __m128 Q_DECL_VECTORCALL reciprocal_mul_ps(__m128 a, float mul)
{
    __m128 ia = _mm_rcp_ps(a); // Approximate 1/a
    // Improve precision of ia using Newton-Raphson
    ia = _mm_sub_ps(_mm_add_ps(ia, ia), _mm_mul_ps(ia, _mm_mul_ps(ia, a)));
    ia = _mm_mul_ps(ia, _mm_set1_ps(mul));
    return ia;
}

template<bool RGBA, bool RGBx>
static inline void convertARGBFromARGB32PM_sse4(uint *buffer, const uint *src, int count)
{
    int i = 0;
    if ((_MM_GET_EXCEPTION_MASK() & _MM_MASK_INVALID) == 0) {
        for (; i < count; ++i) {
            uint v = qUnpremultiply(src[i]);
            if (RGBx)
                v = 0xff000000 | v;
            if (RGBA)
                v = ARGB2RGBA(v);
            buffer[i] = v;
        }
        return;
    }
    const __m128i alphaMask = _mm_set1_epi32(0xff000000);
    const __m128i rgbaMask = _mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15);
    const __m128i zero = _mm_setzero_si128();

    for (; i < count - 3; i += 4) {
        __m128i srcVector = _mm_loadu_si128((const __m128i *)&src[i]);
        if (!_mm_testz_si128(srcVector, alphaMask)) {
            if (!_mm_testc_si128(srcVector, alphaMask)) {
                __m128i srcVectorAlpha = _mm_srli_epi32(srcVector, 24);
                if (RGBA)
                    srcVector = _mm_shuffle_epi8(srcVector, rgbaMask);
                const __m128 a = _mm_cvtepi32_ps(srcVectorAlpha);
                const __m128 ia = reciprocal_mul_ps(a, 255.0f);
                __m128i src1 = _mm_unpacklo_epi8(srcVector, zero);
                __m128i src3 = _mm_unpackhi_epi8(srcVector, zero);
                __m128i src2 = _mm_unpackhi_epi16(src1, zero);
                __m128i src4 = _mm_unpackhi_epi16(src3, zero);
                src1 = _mm_unpacklo_epi16(src1, zero);
                src3 = _mm_unpacklo_epi16(src3, zero);
                __m128 ia1 = _mm_shuffle_ps(ia, ia, _MM_SHUFFLE(0, 0, 0, 0));
                __m128 ia2 = _mm_shuffle_ps(ia, ia, _MM_SHUFFLE(1, 1, 1, 1));
                __m128 ia3 = _mm_shuffle_ps(ia, ia, _MM_SHUFFLE(2, 2, 2, 2));
                __m128 ia4 = _mm_shuffle_ps(ia, ia, _MM_SHUFFLE(3, 3, 3, 3));
                src1 = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(src1), ia1));
                src2 = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(src2), ia2));
                src3 = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(src3), ia3));
                src4 = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(src4), ia4));
                src1 = _mm_packus_epi32(src1, src2);
                src3 = _mm_packus_epi32(src3, src4);
                src1 = _mm_packus_epi16(src1, src3);
                // Handle potential alpha == 0 values:
                __m128i srcVectorAlphaMask = _mm_cmpeq_epi32(srcVectorAlpha, zero);
                src1 = _mm_andnot_si128(srcVectorAlphaMask, src1);
                // Fixup alpha values:
                if (RGBx)
                    srcVector = _mm_or_si128(src1, alphaMask);
                else
                    srcVector = _mm_blendv_epi8(src1, srcVector, alphaMask);
                _mm_storeu_si128((__m128i *)&buffer[i], srcVector);
            } else {
                if (RGBA)
                    _mm_storeu_si128((__m128i *)&buffer[i], _mm_shuffle_epi8(srcVector, rgbaMask));
                else if (buffer != src)
                    _mm_storeu_si128((__m128i *)&buffer[i], srcVector);
            }
        } else {
            if (RGBx)
                _mm_storeu_si128((__m128i *)&buffer[i], alphaMask);
            else
                _mm_storeu_si128((__m128i *)&buffer[i], zero);
        }
    }

    SIMD_EPILOGUE(i, count, 3) {
        uint v = qUnpremultiply_sse4(src[i]);
        if (RGBx)
            v = 0xff000000 | v;
        if (RGBA)
            v = ARGB2RGBA(v);
        buffer[i] = v;
    }
}

template<bool RGBA>
static inline void convertARGBFromRGBA64PM_sse4(uint *buffer, const QRgba64 *src, int count)
{
    int i = 0;
    if ((_MM_GET_EXCEPTION_MASK() & _MM_MASK_INVALID) == 0) {
        for (; i < count; ++i) {
            const QRgba64 v = src[i].unpremultiplied();
            buffer[i] = RGBA ? toRgba8888(v) : toArgb32(v);
        }
        return;
    }
    const __m128i alphaMask = _mm_set1_epi64x(qint64(Q_UINT64_C(0xffff) << 48));
    const __m128i alphaMask32 = _mm_set1_epi32(0xff000000);
    const __m128i rgbaMask = _mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15);
    const __m128i zero = _mm_setzero_si128();

    for (; i < count - 3; i += 4) {
        __m128i srcVector1 = _mm_loadu_si128((const __m128i *)&src[i]);
        __m128i srcVector2 = _mm_loadu_si128((const __m128i *)&src[i + 2]);
        bool transparent1 = _mm_testz_si128(srcVector1, alphaMask);
        bool opaque1 = _mm_testc_si128(srcVector1, alphaMask);
        bool transparent2 = _mm_testz_si128(srcVector2, alphaMask);
        bool opaque2 = _mm_testc_si128(srcVector2, alphaMask);

        if (!(transparent1 && transparent2)) {
            if (!(opaque1 && opaque2)) {
                __m128i srcVector1Alpha = _mm_srli_epi64(srcVector1, 48);
                __m128i srcVector2Alpha = _mm_srli_epi64(srcVector2, 48);
                __m128i srcVectorAlpha = _mm_packus_epi32(srcVector1Alpha, srcVector2Alpha);
                const __m128 a = _mm_cvtepi32_ps(srcVectorAlpha);
                // Convert srcVectorAlpha to final 8-bit alpha channel
                srcVectorAlpha = _mm_add_epi32(srcVectorAlpha, _mm_set1_epi32(128));
                srcVectorAlpha = _mm_sub_epi32(srcVectorAlpha, _mm_srli_epi32(srcVectorAlpha, 8));
                srcVectorAlpha = _mm_srli_epi32(srcVectorAlpha, 8);
                srcVectorAlpha = _mm_slli_epi32(srcVectorAlpha, 24);
                const __m128 ia = reciprocal_mul_ps(a, 255.0f);
                __m128i src1 = _mm_unpacklo_epi16(srcVector1, zero);
                __m128i src2 = _mm_unpackhi_epi16(srcVector1, zero);
                __m128i src3 = _mm_unpacklo_epi16(srcVector2, zero);
                __m128i src4 = _mm_unpackhi_epi16(srcVector2, zero);
                __m128 ia1 = _mm_shuffle_ps(ia, ia, _MM_SHUFFLE(0, 0, 0, 0));
                __m128 ia2 = _mm_shuffle_ps(ia, ia, _MM_SHUFFLE(1, 1, 1, 1));
                __m128 ia3 = _mm_shuffle_ps(ia, ia, _MM_SHUFFLE(2, 2, 2, 2));
                __m128 ia4 = _mm_shuffle_ps(ia, ia, _MM_SHUFFLE(3, 3, 3, 3));
                src1 = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(src1), ia1));
                src2 = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(src2), ia2));
                src3 = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(src3), ia3));
                src4 = _mm_cvtps_epi32(_mm_mul_ps(_mm_cvtepi32_ps(src4), ia4));
                src1 = _mm_packus_epi32(src1, src2);
                src3 = _mm_packus_epi32(src3, src4);
                // Handle potential alpha == 0 values:
                __m128i srcVector1AlphaMask = _mm_cmpeq_epi64(srcVector1Alpha, zero);
                __m128i srcVector2AlphaMask = _mm_cmpeq_epi64(srcVector2Alpha, zero);
                src1 = _mm_andnot_si128(srcVector1AlphaMask, src1);
                src3 = _mm_andnot_si128(srcVector2AlphaMask, src3);
                src1 = _mm_packus_epi16(src1, src3);
                // Fixup alpha values:
                src1 = _mm_blendv_epi8(src1, srcVectorAlpha, alphaMask32);
                // Fix RGB order
                if (!RGBA)
                    src1 = _mm_shuffle_epi8(src1, rgbaMask);
                _mm_storeu_si128((__m128i *)&buffer[i], src1);
            } else {
                __m128i src1 = _mm_unpacklo_epi16(srcVector1, zero);
                __m128i src2 = _mm_unpackhi_epi16(srcVector1, zero);
                __m128i src3 = _mm_unpacklo_epi16(srcVector2, zero);
                __m128i src4 = _mm_unpackhi_epi16(srcVector2, zero);
                src1 = _mm_add_epi32(src1, _mm_set1_epi32(128));
                src2 = _mm_add_epi32(src2, _mm_set1_epi32(128));
                src3 = _mm_add_epi32(src3, _mm_set1_epi32(128));
                src4 = _mm_add_epi32(src4, _mm_set1_epi32(128));
                src1 = _mm_sub_epi32(src1, _mm_srli_epi32(src1, 8));
                src2 = _mm_sub_epi32(src2, _mm_srli_epi32(src2, 8));
                src3 = _mm_sub_epi32(src3, _mm_srli_epi32(src3, 8));
                src4 = _mm_sub_epi32(src4, _mm_srli_epi32(src4, 8));
                src1 = _mm_srli_epi32(src1, 8);
                src2 = _mm_srli_epi32(src2, 8);
                src3 = _mm_srli_epi32(src3, 8);
                src4 = _mm_srli_epi32(src4, 8);
                src1 = _mm_packus_epi32(src1, src2);
                src3 = _mm_packus_epi32(src3, src4);
                src1 = _mm_packus_epi16(src1, src3);
                if (!RGBA)
                    src1 = _mm_shuffle_epi8(src1, rgbaMask);
                _mm_storeu_si128((__m128i *)&buffer[i], src1);
            }
        } else {
            _mm_storeu_si128((__m128i *)&buffer[i], zero);
        }
    }

    SIMD_EPILOGUE(i, count, 3) {
        buffer[i] = qConvertRgba64ToRgb32_sse4<RGBA ? PixelOrderRGB : PixelOrderBGR>(src[i]);
    }
}

#ifndef __AVX2__
void QT_FASTCALL convertARGB32ToARGB32PM_sse4(uint *buffer, int count, const QVector<QRgb> *)
{
    convertARGBToARGB32PM_sse4<false>(buffer, buffer, count);
}

void QT_FASTCALL convertRGBA8888ToARGB32PM_sse4(uint *buffer, int count, const QVector<QRgb> *)
{
    convertARGBToARGB32PM_sse4<true>(buffer, buffer, count);
}

const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_sse4(QRgba64 *buffer, const uint *src, int count,
                                                         const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_sse4<false>(buffer, src, count);
    return buffer;
}

const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_sse4(QRgba64 *buffer, const uint *src, int count,
                                                           const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_sse4<true>(buffer, src, count);
    return buffer;
}

const uint *QT_FASTCALL fetchARGB32ToARGB32PM_sse4(uint *buffer, const uchar *src, int index, int count,
                                                  const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToARGB32PM_sse4<false>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_sse4(uint *buffer, const uchar *src, int index, int count,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToARGB32PM_sse4<true>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_sse4(QRgba64 *buffer, const uchar *src, int index, int count,
                                                      const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_sse4<false>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_sse4(QRgba64 *buffer, const uchar *src, int index, int count,
                                                        const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_sse4<true>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}
#endif // __AVX2__

void QT_FASTCALL storeRGB32FromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                             const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    convertARGBFromARGB32PM_sse4<false,true>(d, src, count);
}

void QT_FASTCALL storeARGB32FromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                              const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    convertARGBFromARGB32PM_sse4<false,false>(d, src, count);
}

void QT_FASTCALL storeRGBA8888FromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                                const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    convertARGBFromARGB32PM_sse4<true,false>(d, src, count);
}

void QT_FASTCALL storeRGBXFromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                            const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    convertARGBFromARGB32PM_sse4<true,true>(d, src, count);
}

template<QtPixelOrder PixelOrder>
void QT_FASTCALL storeA2RGB30PMFromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                                 const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    for (int i = 0; i < count; ++i)
        d[i] = qConvertArgb32ToA2rgb30_sse4<PixelOrder>(src[i]);
}

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL destStore64ARGB32_sse4(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length)
{
    uint *dest = (uint*)rasterBuffer->scanLine(y) + x;
    convertARGBFromRGBA64PM_sse4<false>(dest, buffer, length);
}

void QT_FASTCALL destStore64RGBA8888_sse4(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length)
{
    uint *dest = (uint*)rasterBuffer->scanLine(y) + x;
    convertARGBFromRGBA64PM_sse4<true>(dest, buffer, length);
}
#endif

void QT_FASTCALL storeARGB32FromRGBA64PM_sse4(uchar *dest, const QRgba64 *src, int index, int count,
                                              const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = (uint*)dest + index;
    convertARGBFromRGBA64PM_sse4<false>(d, src, count);
}

void QT_FASTCALL storeRGBA8888FromRGBA64PM_sse4(uchar *dest, const QRgba64 *src, int index, int count,
                                                const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = (uint*)dest + index;
    convertARGBFromRGBA64PM_sse4<true>(d, src, count);
}

template
void QT_FASTCALL storeA2RGB30PMFromARGB32PM_sse4<PixelOrderBGR>(uchar *dest, const uint *src, int index, int count,
                                                                const QVector<QRgb> *, QDitherInfo *);
template
void QT_FASTCALL storeA2RGB30PMFromARGB32PM_sse4<PixelOrderRGB>(uchar *dest, const uint *src, int index, int count,
                                                                const QVector<QRgb> *, QDitherInfo *);

QT_END_NAMESPACE

#endif
