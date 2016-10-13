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

#include "qdrawhelper_p.h"
#include "qdrawingprimitive_sse2_p.h"

#if defined(QT_COMPILER_SUPPORTS_AVX2)

QT_BEGIN_NAMESPACE

// Autovectorized premultiply functions:
const uint *QT_FASTCALL convertARGB32ToARGB32PM_avx2(uint *buffer, const uint *src, int count,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    return qt_convertARGB32ToARGB32PM(buffer, src, count);
}

const uint *QT_FASTCALL convertRGBA8888ToARGB32PM_avx2(uint *buffer, const uint *src, int count,
                                                       const QVector<QRgb> *, QDitherInfo *)
{
    return qt_convertRGBA8888ToARGB32PM(buffer, src, count);
}

// Vectorized blend functions:

// See BYTE_MUL_SSE2 for details.
inline static void BYTE_MUL_AVX2(__m256i &pixelVector, const __m256i &alphaChannel, const __m256i &colorMask, const __m256i &half)
{
    __m256i pixelVectorAG = _mm256_srli_epi16(pixelVector, 8);
    __m256i pixelVectorRB = _mm256_and_si256(pixelVector, colorMask);

    pixelVectorAG = _mm256_mullo_epi16(pixelVectorAG, alphaChannel);
    pixelVectorRB = _mm256_mullo_epi16(pixelVectorRB, alphaChannel);

    pixelVectorRB = _mm256_add_epi16(pixelVectorRB, _mm256_srli_epi16(pixelVectorRB, 8));
    pixelVectorAG = _mm256_add_epi16(pixelVectorAG, _mm256_srli_epi16(pixelVectorAG, 8));
    pixelVectorRB = _mm256_add_epi16(pixelVectorRB, half);
    pixelVectorAG = _mm256_add_epi16(pixelVectorAG, half);

    pixelVectorRB = _mm256_srli_epi16(pixelVectorRB, 8);
    pixelVectorAG = _mm256_andnot_si256(colorMask, pixelVectorAG);

    pixelVector = _mm256_or_si256(pixelVectorAG, pixelVectorRB);
}

// See INTERPOLATE_PIXEL_255_SSE2 for details.
inline static void INTERPOLATE_PIXEL_255_AVX2(const __m256i &srcVector, __m256i &dstVector, const __m256i &alphaChannel, const __m256i &oneMinusAlphaChannel, const __m256i &colorMask, const __m256i &half)
{
    const __m256i srcVectorAG = _mm256_srli_epi16(srcVector, 8);
    const __m256i dstVectorAG = _mm256_srli_epi16(dstVector, 8);
    const __m256i srcVectorRB = _mm256_and_si256(srcVector, colorMask);
    const __m256i dstVectorRB = _mm256_and_si256(dstVector, colorMask);
    const __m256i srcVectorAGalpha = _mm256_mullo_epi16(srcVectorAG, alphaChannel);
    const __m256i srcVectorRBalpha = _mm256_mullo_epi16(srcVectorRB, alphaChannel);
    const __m256i dstVectorAGoneMinusAlpha = _mm256_mullo_epi16(dstVectorAG, oneMinusAlphaChannel);
    const __m256i dstVectorRBoneMinusAlpha = _mm256_mullo_epi16(dstVectorRB, oneMinusAlphaChannel);
    __m256i finalAG = _mm256_add_epi16(srcVectorAGalpha, dstVectorAGoneMinusAlpha);
    __m256i finalRB = _mm256_add_epi16(srcVectorRBalpha, dstVectorRBoneMinusAlpha);
    finalAG = _mm256_add_epi16(finalAG, _mm256_srli_epi16(finalAG, 8));
    finalRB = _mm256_add_epi16(finalRB, _mm256_srli_epi16(finalRB, 8));
    finalAG = _mm256_add_epi16(finalAG, half);
    finalRB = _mm256_add_epi16(finalRB, half);
    finalAG = _mm256_andnot_si256(colorMask, finalAG);
    finalRB = _mm256_srli_epi16(finalRB, 8);

    dstVector = _mm256_or_si256(finalAG, finalRB);
}

// See BLEND_SOURCE_OVER_ARGB32_SSE2 for details.
inline static void BLEND_SOURCE_OVER_ARGB32_AVX2(quint32 *dst, const quint32 *src, const int length)
{
    const __m256i half = _mm256_set1_epi16(0x80);
    const __m256i one = _mm256_set1_epi16(0xff);
    const __m256i colorMask = _mm256_set1_epi32(0x00ff00ff);
    const __m256i alphaMask = _mm256_set1_epi32(0xff000000);
    const __m256i offsetMask = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
    const __m256i alphaShuffleMask = _mm256_set_epi8(char(0xff),15,char(0xff),15,char(0xff),11,char(0xff),11,char(0xff),7,char(0xff),7,char(0xff),3,char(0xff),3,
                                                     char(0xff),15,char(0xff),15,char(0xff),11,char(0xff),11,char(0xff),7,char(0xff),7,char(0xff),3,char(0xff),3);

    const int minusOffsetToAlignDstOn32Bytes = (reinterpret_cast<quintptr>(dst) >> 2) & 0x7;

    int x = 0;
    // Prologue to handle all pixels until dst is 32-byte aligned in one step.
    if (minusOffsetToAlignDstOn32Bytes != 0 && x < (length - 7)) {
        const __m256i prologueMask = _mm256_sub_epi32(_mm256_set1_epi32(minusOffsetToAlignDstOn32Bytes - 1), offsetMask);
        const __m256i srcVector = _mm256_maskload_epi32((const int *)&src[x - minusOffsetToAlignDstOn32Bytes], prologueMask);
        const __m256i prologueAlphaMask = _mm256_blendv_epi8(_mm256_setzero_si256(), alphaMask, prologueMask);
        if (!_mm256_testz_si256(srcVector, prologueAlphaMask)) {
            if (_mm256_testc_si256(srcVector, prologueAlphaMask)) {
                _mm256_maskstore_epi32((int *)&dst[x - minusOffsetToAlignDstOn32Bytes], prologueMask, srcVector);
            } else {
                __m256i alphaChannel = _mm256_shuffle_epi8(srcVector, alphaShuffleMask);
                alphaChannel = _mm256_sub_epi16(one, alphaChannel);
                __m256i dstVector = _mm256_maskload_epi32((int *)&dst[x - minusOffsetToAlignDstOn32Bytes], prologueMask);
                BYTE_MUL_AVX2(dstVector, alphaChannel, colorMask, half);
                dstVector = _mm256_add_epi8(dstVector, srcVector);
                _mm256_maskstore_epi32((int *)&dst[x - minusOffsetToAlignDstOn32Bytes], prologueMask, dstVector);
            }
        }
        x += (8 - minusOffsetToAlignDstOn32Bytes);
    }

    for (; x < (length - 7); x += 8) {
        const __m256i srcVector = _mm256_lddqu_si256((const __m256i *)&src[x]);
        if (!_mm256_testz_si256(srcVector, alphaMask)) {
            if (_mm256_testc_si256(srcVector, alphaMask)) {
                _mm256_store_si256((__m256i *)&dst[x], srcVector);
            } else {
                __m256i alphaChannel = _mm256_shuffle_epi8(srcVector, alphaShuffleMask);
                alphaChannel = _mm256_sub_epi16(one, alphaChannel);
                __m256i dstVector = _mm256_load_si256((__m256i *)&dst[x]);
                 BYTE_MUL_AVX2(dstVector, alphaChannel, colorMask, half);
                dstVector = _mm256_add_epi8(dstVector, srcVector);
                _mm256_store_si256((__m256i *)&dst[x], dstVector);
            }
        }
    }

    // Epilogue to handle all remaining pixels in one step.
    if (x < length) {
        const __m256i epilogueMask = _mm256_add_epi32(offsetMask, _mm256_set1_epi32(x - length));
        const __m256i srcVector = _mm256_maskload_epi32((const int *)&src[x], epilogueMask);
        const __m256i epilogueAlphaMask = _mm256_blendv_epi8(_mm256_setzero_si256(), alphaMask, epilogueMask);
        if (!_mm256_testz_si256(srcVector, epilogueAlphaMask)) {
            if (_mm256_testc_si256(srcVector, epilogueAlphaMask)) {
                _mm256_maskstore_epi32((int *)&dst[x], epilogueMask, srcVector);
            } else {
                __m256i alphaChannel = _mm256_shuffle_epi8(srcVector, alphaShuffleMask);
                alphaChannel = _mm256_sub_epi16(one, alphaChannel);
                __m256i dstVector = _mm256_maskload_epi32((int *)&dst[x], epilogueMask);
                BYTE_MUL_AVX2(dstVector, alphaChannel, colorMask, half);
                dstVector = _mm256_add_epi8(dstVector, srcVector);
                _mm256_maskstore_epi32((int *)&dst[x], epilogueMask, dstVector);
            }
        }
    }
}


// See BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_SSE2 for details.
inline static void BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_AVX2(quint32 *dst, const quint32 *src, const int length, const int const_alpha)
{
    int x = 0;

    ALIGNMENT_PROLOGUE_32BYTES(dst, x, length)
        blend_pixel(dst[x], src[x], const_alpha);

    const __m256i half = _mm256_set1_epi16(0x80);
    const __m256i one = _mm256_set1_epi16(0xff);
    const __m256i colorMask = _mm256_set1_epi32(0x00ff00ff);
    const __m256i alphaMask = _mm256_set1_epi32(0xff000000);
    const __m256i alphaShuffleMask = _mm256_set_epi8(char(0xff),15,char(0xff),15,char(0xff),11,char(0xff),11,char(0xff),7,char(0xff),7,char(0xff),3,char(0xff),3,
                                                     char(0xff),15,char(0xff),15,char(0xff),11,char(0xff),11,char(0xff),7,char(0xff),7,char(0xff),3,char(0xff),3);
    const __m256i constAlphaVector = _mm256_set1_epi16(const_alpha);
    for (; x < (length - 7); x += 8) {
        __m256i srcVector = _mm256_lddqu_si256((const __m256i *)&src[x]);
        if (!_mm256_testz_si256(srcVector, alphaMask)) {
            BYTE_MUL_AVX2(srcVector, constAlphaVector, colorMask, half);

            __m256i alphaChannel = _mm256_shuffle_epi8(srcVector, alphaShuffleMask);
            alphaChannel = _mm256_sub_epi16(one, alphaChannel);
            __m256i dstVector = _mm256_load_si256((__m256i *)&dst[x]);
            BYTE_MUL_AVX2(dstVector, alphaChannel, colorMask, half);
            dstVector = _mm256_add_epi8(dstVector, srcVector);
            _mm256_store_si256((__m256i *)&dst[x], dstVector);
        }
    }
    SIMD_EPILOGUE(x, length, 7)
        blend_pixel(dst[x], src[x], const_alpha);
}

void qt_blend_argb32_on_argb32_avx2(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl,
                                    int w, int h,
                                    int const_alpha)
{
    if (const_alpha == 256) {
        for (int y = 0; y < h; ++y) {
            const quint32 *src = reinterpret_cast<const quint32 *>(srcPixels);
            quint32 *dst = reinterpret_cast<quint32 *>(destPixels);
            BLEND_SOURCE_OVER_ARGB32_AVX2(dst, src, w);
            destPixels += dbpl;
            srcPixels += sbpl;
        }
    } else if (const_alpha != 0) {
        const_alpha = (const_alpha * 255) >> 8;
        for (int y = 0; y < h; ++y) {
            const quint32 *src = reinterpret_cast<const quint32 *>(srcPixels);
            quint32 *dst = reinterpret_cast<quint32 *>(destPixels);
            BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_AVX2(dst, src, w, const_alpha);
            destPixels += dbpl;
            srcPixels += sbpl;
        }
    }
}

void qt_blend_rgb32_on_rgb32_avx2(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl,
                                   int w, int h,
                                   int const_alpha)
{
    if (const_alpha == 256) {
        for (int y = 0; y < h; ++y) {
            const quint32 *src = reinterpret_cast<const quint32 *>(srcPixels);
            quint32 *dst = reinterpret_cast<quint32 *>(destPixels);
            ::memcpy(dst, src, w * sizeof(uint));
            srcPixels += sbpl;
            destPixels += dbpl;
        }
        return;
    }
    if (const_alpha == 0)
        return;

    const __m256i half = _mm256_set1_epi16(0x80);
    const __m256i colorMask = _mm256_set1_epi32(0x00ff00ff);

    const_alpha = (const_alpha * 255) >> 8;
    int one_minus_const_alpha = 255 - const_alpha;
    const __m256i constAlphaVector = _mm256_set1_epi16(const_alpha);
    const __m256i oneMinusConstAlpha =  _mm256_set1_epi16(one_minus_const_alpha);
    for (int y = 0; y < h; ++y) {
        const quint32 *src = reinterpret_cast<const quint32 *>(srcPixels);
        quint32 *dst = reinterpret_cast<quint32 *>(destPixels);
        int x = 0;

        // First, align dest to 32 bytes:
        ALIGNMENT_PROLOGUE_32BYTES(dst, x, w)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], one_minus_const_alpha);

        // 2) interpolate pixels with AVX2
        for (; x < (w - 7); x += 8) {
            const __m256i srcVector = _mm256_lddqu_si256((const __m256i *)&src[x]);
            if (!_mm256_testz_si256(srcVector, srcVector)) {
                __m256i dstVector = _mm256_load_si256((__m256i *)&dst[x]);
                INTERPOLATE_PIXEL_255_AVX2(srcVector, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half);
                _mm256_store_si256((__m256i *)&dst[x], dstVector);
            }
        }

        // 3) Epilogue
        SIMD_EPILOGUE(x, w, 7)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], one_minus_const_alpha);

        srcPixels += sbpl;
        destPixels += dbpl;
    }
}

void QT_FASTCALL comp_func_SourceOver_avx2(uint *destPixels, const uint *srcPixels, int length, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256);

    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;

    if (const_alpha == 255)
        BLEND_SOURCE_OVER_ARGB32_AVX2(dst, src, length);
    else
        BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_AVX2(dst, src, length, const_alpha);
}

void QT_FASTCALL comp_func_Source_avx2(uint *dst, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dst, src, length * sizeof(uint));
    } else {
        const int ialpha = 255 - const_alpha;

        int x = 0;

        // 1) prologue, align on 32 bytes
        ALIGNMENT_PROLOGUE_32BYTES(dst, x, length)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], ialpha);

        // 2) interpolate pixels with AVX2
        const __m256i half = _mm256_set1_epi16(0x80);
        const __m256i colorMask = _mm256_set1_epi32(0x00ff00ff);
        const __m256i constAlphaVector = _mm256_set1_epi16(const_alpha);
        const __m256i oneMinusConstAlpha =  _mm256_set1_epi16(ialpha);
        for (; x < length - 7; x += 8) {
            const __m256i srcVector = _mm256_lddqu_si256((const __m256i *)&src[x]);
            __m256i dstVector = _mm256_load_si256((__m256i *)&dst[x]);
            INTERPOLATE_PIXEL_255_AVX2(srcVector, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half);
            _mm256_store_si256((__m256i *)&dst[x], dstVector);
        }

        // 3) Epilogue
        SIMD_EPILOGUE(x, length, 7)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], ialpha);
    }
}

void QT_FASTCALL comp_func_solid_SourceOver_avx2(uint *destPixels, int length, uint color, uint const_alpha)
{
    if ((const_alpha & qAlpha(color)) == 255) {
        qt_memfill32(destPixels, color, length);
    } else {
        if (const_alpha != 255)
            color = BYTE_MUL(color, const_alpha);

        const quint32 minusAlphaOfColor = qAlpha(~color);
        int x = 0;

        quint32 *dst = (quint32 *) destPixels;
        const __m256i colorVector = _mm256_set1_epi32(color);
        const __m256i colorMask = _mm256_set1_epi32(0x00ff00ff);
        const __m256i half = _mm256_set1_epi16(0x80);
        const __m256i minusAlphaOfColorVector = _mm256_set1_epi16(minusAlphaOfColor);

        ALIGNMENT_PROLOGUE_32BYTES(dst, x, length)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);

        for (; x < length - 7; x += 8) {
            __m256i dstVector = _mm256_load_si256((__m256i *)&dst[x]);
            BYTE_MUL_AVX2(dstVector, minusAlphaOfColorVector, colorMask, half);
            dstVector = _mm256_add_epi8(colorVector, dstVector);
            _mm256_store_si256((__m256i *)&dst[x], dstVector);
        }
        SIMD_EPILOGUE(x, length, 7)
            destPixels[x] = color + BYTE_MUL(destPixels[x], minusAlphaOfColor);
    }
}

QT_END_NAMESPACE

#endif
