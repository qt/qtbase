/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Copyright (C) 2018 Intel Corporation.
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
#include "qdrawhelper_x86_p.h"
#include "qdrawingprimitive_sse2_p.h"
#include "qrgba64_p.h"

#if defined(QT_COMPILER_SUPPORTS_AVX2)

QT_BEGIN_NAMESPACE

enum {
    FixedScale = 1 << 16,
    HalfPoint = 1 << 15
};

// Vectorized blend functions:

// See BYTE_MUL_SSE2 for details.
inline static void Q_DECL_VECTORCALL
BYTE_MUL_AVX2(__m256i &pixelVector, __m256i alphaChannel, __m256i colorMask, __m256i half)
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

inline static void Q_DECL_VECTORCALL
BYTE_MUL_RGB64_AVX2(__m256i &pixelVector, __m256i alphaChannel, __m256i colorMask, __m256i half)
{
    __m256i pixelVectorAG = _mm256_srli_epi32(pixelVector, 16);
    __m256i pixelVectorRB = _mm256_and_si256(pixelVector, colorMask);

    pixelVectorAG = _mm256_mullo_epi32(pixelVectorAG, alphaChannel);
    pixelVectorRB = _mm256_mullo_epi32(pixelVectorRB, alphaChannel);

    pixelVectorRB = _mm256_add_epi32(pixelVectorRB, _mm256_srli_epi32(pixelVectorRB, 16));
    pixelVectorAG = _mm256_add_epi32(pixelVectorAG, _mm256_srli_epi32(pixelVectorAG, 16));
    pixelVectorRB = _mm256_add_epi32(pixelVectorRB, half);
    pixelVectorAG = _mm256_add_epi32(pixelVectorAG, half);

    pixelVectorRB = _mm256_srli_epi32(pixelVectorRB, 16);
    pixelVectorAG = _mm256_andnot_si256(colorMask, pixelVectorAG);

    pixelVector = _mm256_or_si256(pixelVectorAG, pixelVectorRB);
}

// See INTERPOLATE_PIXEL_255_SSE2 for details.
inline static void Q_DECL_VECTORCALL
INTERPOLATE_PIXEL_255_AVX2(__m256i srcVector, __m256i &dstVector, __m256i alphaChannel, __m256i oneMinusAlphaChannel, __m256i colorMask, __m256i half)
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

inline static void Q_DECL_VECTORCALL
INTERPOLATE_PIXEL_RGB64_AVX2(__m256i srcVector, __m256i &dstVector, __m256i alphaChannel, __m256i oneMinusAlphaChannel, __m256i colorMask, __m256i half)
{
    const __m256i srcVectorAG = _mm256_srli_epi32(srcVector, 16);
    const __m256i dstVectorAG = _mm256_srli_epi32(dstVector, 16);
    const __m256i srcVectorRB = _mm256_and_si256(srcVector, colorMask);
    const __m256i dstVectorRB = _mm256_and_si256(dstVector, colorMask);
    const __m256i srcVectorAGalpha = _mm256_mullo_epi32(srcVectorAG, alphaChannel);
    const __m256i srcVectorRBalpha = _mm256_mullo_epi32(srcVectorRB, alphaChannel);
    const __m256i dstVectorAGoneMinusAlpha = _mm256_mullo_epi32(dstVectorAG, oneMinusAlphaChannel);
    const __m256i dstVectorRBoneMinusAlpha = _mm256_mullo_epi32(dstVectorRB, oneMinusAlphaChannel);
    __m256i finalAG = _mm256_add_epi32(srcVectorAGalpha, dstVectorAGoneMinusAlpha);
    __m256i finalRB = _mm256_add_epi32(srcVectorRBalpha, dstVectorRBoneMinusAlpha);
    finalAG = _mm256_add_epi32(finalAG, _mm256_srli_epi32(finalAG, 16));
    finalRB = _mm256_add_epi32(finalRB, _mm256_srli_epi32(finalRB, 16));
    finalAG = _mm256_add_epi32(finalAG, half);
    finalRB = _mm256_add_epi32(finalRB, half);
    finalAG = _mm256_andnot_si256(colorMask, finalAG);
    finalRB = _mm256_srli_epi32(finalRB, 16);

    dstVector = _mm256_or_si256(finalAG, finalRB);
}


// See BLEND_SOURCE_OVER_ARGB32_SSE2 for details.
inline static void Q_DECL_VECTORCALL BLEND_SOURCE_OVER_ARGB32_AVX2(quint32 *dst, const quint32 *src, const int length)
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
inline static void Q_DECL_VECTORCALL
BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_AVX2(quint32 *dst, const quint32 *src, const int length, const int const_alpha)
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
            __m256i dstVector = _mm256_load_si256((__m256i *)&dst[x]);
            INTERPOLATE_PIXEL_255_AVX2(srcVector, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half);
            _mm256_store_si256((__m256i *)&dst[x], dstVector);
        }

        // 3) Epilogue
        SIMD_EPILOGUE(x, w, 7)
            dst[x] = INTERPOLATE_PIXEL_255(src[x], const_alpha, dst[x], one_minus_const_alpha);

        srcPixels += sbpl;
        destPixels += dbpl;
    }
}

static Q_NEVER_INLINE
void Q_DECL_VECTORCALL qt_memfillXX_avx2(uchar *dest, __m256i value256, qsizetype bytes)
{
    __m128i value128 = _mm256_castsi256_si128(value256);

    // main body
    __m256i *dst256 = reinterpret_cast<__m256i *>(dest);
    uchar *end = dest + bytes;
    while (reinterpret_cast<uchar *>(dst256 + 4) <= end) {
        _mm256_storeu_si256(dst256 + 0, value256);
        _mm256_storeu_si256(dst256 + 1, value256);
        _mm256_storeu_si256(dst256 + 2, value256);
        _mm256_storeu_si256(dst256 + 3, value256);
        dst256 += 4;
    }

    // first epilogue: fewer than 128 bytes / 32 entries
    bytes = end - reinterpret_cast<uchar *>(dst256);
    switch (bytes / sizeof(value256)) {
    case 3: _mm256_storeu_si256(dst256++, value256); Q_FALLTHROUGH();
    case 2: _mm256_storeu_si256(dst256++, value256); Q_FALLTHROUGH();
    case 1: _mm256_storeu_si256(dst256++, value256);
    }

    // second epilogue: fewer than 32 bytes
    __m128i *dst128 = reinterpret_cast<__m128i *>(dst256);
    if (bytes & sizeof(value128))
        _mm_storeu_si128(dst128++, value128);

    // third epilogue: fewer than 16 bytes
    if (bytes & 8)
        _mm_storel_epi64(reinterpret_cast<__m128i *>(end - 8), value128);
}

void qt_memfill64_avx2(quint64 *dest, quint64 value, qsizetype count)
{
#if defined(Q_CC_GNU) && !defined(Q_CC_CLANG) && !defined(Q_CC_INTEL)
    // work around https://gcc.gnu.org/bugzilla/show_bug.cgi?id=80820
    __m128i value64 = _mm_set_epi64x(0, value); // _mm_cvtsi64_si128(value);
#  ifdef Q_PROCESSOR_X86_64
    asm ("" : "+x" (value64));
#  endif
    __m256i value256 =  _mm256_broadcastq_epi64(value64);
#else
    __m256i value256 = _mm256_set1_epi64x(value);
#endif

    qt_memfillXX_avx2(reinterpret_cast<uchar *>(dest), value256, count * sizeof(quint64));
}

void qt_memfill32_avx2(quint32 *dest, quint32 value, qsizetype count)
{
    if (count % 2) {
        // odd number of pixels, round to even
        *dest++ = value;
        --count;
    }
    qt_memfillXX_avx2(reinterpret_cast<uchar *>(dest), _mm256_set1_epi32(value), count * sizeof(quint32));
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

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_SourceOver_rgb64_avx2(QRgba64 *dst, const QRgba64 *src, int length, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256); // const_alpha is in [0-255]
    const __m256i half = _mm256_set1_epi32(0x8000);
    const __m256i one  = _mm256_set1_epi32(0xffff);
    const __m256i colorMask = _mm256_set1_epi32(0x0000ffff);
    __m256i alphaMask = _mm256_set1_epi32(0xff000000);
    alphaMask = _mm256_unpacklo_epi8(alphaMask, alphaMask);
    const __m256i alphaShuffleMask = _mm256_set_epi8(char(0xff),char(0xff),15,14,char(0xff),char(0xff),15,14,char(0xff),char(0xff),7,6,char(0xff),char(0xff),7,6,
                                                     char(0xff),char(0xff),15,14,char(0xff),char(0xff),15,14,char(0xff),char(0xff),7,6,char(0xff),char(0xff),7,6);

    if (const_alpha == 255) {
        int x = 0;
        for (; x < length && (quintptr(dst + x) & 31); ++x)
            blend_pixel(dst[x], src[x]);
        for (; x < length - 3; x += 4) {
            const __m256i srcVector = _mm256_lddqu_si256((const __m256i *)&src[x]);
            if (!_mm256_testz_si256(srcVector, alphaMask)) {
                // Not all transparent
                if (_mm256_testc_si256(srcVector, alphaMask)) {
                    // All opaque
                    _mm256_store_si256((__m256i *)&dst[x], srcVector);
                } else {
                    __m256i alphaChannel = _mm256_shuffle_epi8(srcVector, alphaShuffleMask);
                    alphaChannel = _mm256_sub_epi32(one, alphaChannel);
                    __m256i dstVector = _mm256_load_si256((__m256i *)&dst[x]);
                    BYTE_MUL_RGB64_AVX2(dstVector, alphaChannel, colorMask, half);
                    dstVector = _mm256_add_epi16(dstVector, srcVector);
                    _mm256_store_si256((__m256i *)&dst[x], dstVector);
                }
            }
        }
        SIMD_EPILOGUE(x, length, 3)
            blend_pixel(dst[x], src[x]);
    } else {
        const __m256i constAlphaVector = _mm256_set1_epi32(const_alpha | (const_alpha << 8));
        int x = 0;
        for (; x < length && (quintptr(dst + x) & 31); ++x)
            blend_pixel(dst[x], src[x], const_alpha);
        for (; x < length - 3; x += 4) {
            __m256i srcVector = _mm256_lddqu_si256((const __m256i *)&src[x]);
            if (!_mm256_testz_si256(srcVector, alphaMask)) {
                // Not all transparent
                BYTE_MUL_RGB64_AVX2(srcVector, constAlphaVector, colorMask, half);

                __m256i alphaChannel = _mm256_shuffle_epi8(srcVector, alphaShuffleMask);
                alphaChannel = _mm256_sub_epi32(one, alphaChannel);
                __m256i dstVector = _mm256_load_si256((__m256i *)&dst[x]);
                BYTE_MUL_RGB64_AVX2(dstVector, alphaChannel, colorMask, half);
                dstVector = _mm256_add_epi16(dstVector, srcVector);
                _mm256_store_si256((__m256i *)&dst[x], dstVector);
            }
        }
        SIMD_EPILOGUE(x, length, 3)
            blend_pixel(dst[x], src[x], const_alpha);
    }
}
#endif

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

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_Source_rgb64_avx2(QRgba64 *dst, const QRgba64 *src, int length, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256); // const_alpha is in [0-255]
    if (const_alpha == 255) {
        ::memcpy(dst, src, length * sizeof(QRgba64));
    } else {
        const uint ca = const_alpha | (const_alpha << 8); // adjust to [0-65535]
        const uint cia = 65535 - ca;

        int x = 0;

        // 1) prologue, align on 32 bytes
        for (; x < length && (quintptr(dst + x) & 31); ++x)
            dst[x] = interpolate65535(src[x], ca, dst[x], cia);

        // 2) interpolate pixels with AVX2
        const __m256i half = _mm256_set1_epi32(0x8000);
        const __m256i colorMask = _mm256_set1_epi32(0x0000ffff);
        const __m256i constAlphaVector = _mm256_set1_epi32(ca);
        const __m256i oneMinusConstAlpha =  _mm256_set1_epi32(cia);
        for (; x < length - 3; x += 4) {
            const __m256i srcVector = _mm256_lddqu_si256((const __m256i *)&src[x]);
            __m256i dstVector = _mm256_load_si256((__m256i *)&dst[x]);
            INTERPOLATE_PIXEL_RGB64_AVX2(srcVector, dstVector, constAlphaVector, oneMinusConstAlpha, colorMask, half);
            _mm256_store_si256((__m256i *)&dst[x], dstVector);
        }

        // 3) Epilogue
        SIMD_EPILOGUE(x, length, 3)
            dst[x] = interpolate65535(src[x], ca, dst[x], cia);
    }
}
#endif

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

#if QT_CONFIG(raster_64bit)
void QT_FASTCALL comp_func_solid_SourceOver_rgb64_avx2(QRgba64 *destPixels, int length, QRgba64 color, uint const_alpha)
{
    Q_ASSERT(const_alpha < 256); // const_alpha is in [0-255]
    if (const_alpha == 255 && color.isOpaque()) {
        qt_memfill64((quint64*)destPixels, color, length);
    } else {
        if (const_alpha != 255)
            color = multiplyAlpha255(color, const_alpha);

        const uint minusAlphaOfColor = 65535 - color.alpha();
        int x = 0;
        quint64 *dst = (quint64 *) destPixels;
        const __m256i colorVector = _mm256_set1_epi64x(color);
        const __m256i colorMask = _mm256_set1_epi32(0x0000ffff);
        const __m256i half = _mm256_set1_epi32(0x8000);
        const __m256i minusAlphaOfColorVector = _mm256_set1_epi32(minusAlphaOfColor);

        for (; x < length && (quintptr(dst + x) & 31); ++x)
            destPixels[x] = color + multiplyAlpha65535(destPixels[x], minusAlphaOfColor);

        for (; x < length - 3; x += 4) {
            __m256i dstVector = _mm256_load_si256((__m256i *)&dst[x]);
            BYTE_MUL_RGB64_AVX2(dstVector, minusAlphaOfColorVector, colorMask, half);
            dstVector = _mm256_add_epi16(colorVector, dstVector);
            _mm256_store_si256((__m256i *)&dst[x], dstVector);
        }
        SIMD_EPILOGUE(x, length, 3)
            destPixels[x] = color + multiplyAlpha65535(destPixels[x], minusAlphaOfColor);
    }
}
#endif

#define interpolate_4_pixels_16_avx2(tlr1, tlr2, blr1, blr2, distx, disty, colorMask, v_256, b)  \
{ \
    /* Correct for later unpack */ \
    const __m256i vdistx = _mm256_permute4x64_epi64(distx, _MM_SHUFFLE(3, 1, 2, 0)); \
    const __m256i vdisty = _mm256_permute4x64_epi64(disty, _MM_SHUFFLE(3, 1, 2, 0)); \
    \
    __m256i dxdy = _mm256_mullo_epi16 (vdistx, vdisty); \
    const __m256i distx_ = _mm256_slli_epi16(vdistx, 4); \
    const __m256i disty_ = _mm256_slli_epi16(vdisty, 4); \
    __m256i idxidy =  _mm256_add_epi16(dxdy, _mm256_sub_epi16(v_256, _mm256_add_epi16(distx_, disty_))); \
    __m256i dxidy =  _mm256_sub_epi16(distx_, dxdy); \
    __m256i idxdy =  _mm256_sub_epi16(disty_, dxdy); \
 \
    __m256i tlr1AG = _mm256_srli_epi16(tlr1, 8); \
    __m256i tlr1RB = _mm256_and_si256(tlr1, colorMask); \
    __m256i tlr2AG = _mm256_srli_epi16(tlr2, 8); \
    __m256i tlr2RB = _mm256_and_si256(tlr2, colorMask); \
    __m256i blr1AG = _mm256_srli_epi16(blr1, 8); \
    __m256i blr1RB = _mm256_and_si256(blr1, colorMask); \
    __m256i blr2AG = _mm256_srli_epi16(blr2, 8); \
    __m256i blr2RB = _mm256_and_si256(blr2, colorMask); \
 \
    __m256i odxidy1 = _mm256_unpacklo_epi32(idxidy, dxidy); \
    __m256i odxidy2 = _mm256_unpackhi_epi32(idxidy, dxidy); \
    tlr1AG = _mm256_mullo_epi16(tlr1AG, odxidy1); \
    tlr1RB = _mm256_mullo_epi16(tlr1RB, odxidy1); \
    tlr2AG = _mm256_mullo_epi16(tlr2AG, odxidy2); \
    tlr2RB = _mm256_mullo_epi16(tlr2RB, odxidy2); \
    __m256i odxdy1 = _mm256_unpacklo_epi32(idxdy, dxdy); \
    __m256i odxdy2 = _mm256_unpackhi_epi32(idxdy, dxdy); \
    blr1AG = _mm256_mullo_epi16(blr1AG, odxdy1); \
    blr1RB = _mm256_mullo_epi16(blr1RB, odxdy1); \
    blr2AG = _mm256_mullo_epi16(blr2AG, odxdy2); \
    blr2RB = _mm256_mullo_epi16(blr2RB, odxdy2); \
 \
    /* Add the values, and shift to only keep 8 significant bits per colors */ \
    __m256i topAG = _mm256_hadd_epi32(tlr1AG, tlr2AG); \
    __m256i topRB = _mm256_hadd_epi32(tlr1RB, tlr2RB); \
    __m256i botAG = _mm256_hadd_epi32(blr1AG, blr2AG); \
    __m256i botRB = _mm256_hadd_epi32(blr1RB, blr2RB); \
    __m256i rAG = _mm256_add_epi16(topAG, botAG); \
    __m256i rRB = _mm256_add_epi16(topRB, botRB); \
    rRB = _mm256_srli_epi16(rRB, 8); \
    /* Correct for hadd */ \
    rAG = _mm256_permute4x64_epi64(rAG, _MM_SHUFFLE(3, 1, 2, 0)); \
    rRB = _mm256_permute4x64_epi64(rRB, _MM_SHUFFLE(3, 1, 2, 0)); \
    _mm256_storeu_si256((__m256i*)(b), _mm256_blendv_epi8(rAG, rRB, colorMask)); \
}

inline void fetchTransformedBilinear_pixelBounds(int, int l1, int l2, int &v1, int &v2)
{
    if (v1 < l1)
        v2 = v1 = l1;
    else if (v1 >= l2)
        v2 = v1 = l2;
    else
        v2 = v1 + 1;
    Q_ASSERT(v1 >= l1 && v1 <= l2);
    Q_ASSERT(v2 >= l1 && v2 <= l2);
}

void QT_FASTCALL intermediate_adder_avx2(uint *b, uint *end, const IntermediateBuffer &intermediate, int offset, int &fx, int fdx);

void QT_FASTCALL fetchTransformedBilinearARGB32PM_simple_scale_helper_avx2(uint *b, uint *end, const QTextureData &image,
                                                                           int &fx, int &fy, int fdx, int /*fdy*/)
{
    int y1 = (fy >> 16);
    int y2;
    fetchTransformedBilinear_pixelBounds(image.height, image.y1, image.y2 - 1, y1, y2);
    const uint *s1 = (const uint *)image.scanLine(y1);
    const uint *s2 = (const uint *)image.scanLine(y2);

    const int disty = (fy & 0x0000ffff) >> 8;
    const int idisty = 256 - disty;
    const int length = end - b;

    // The intermediate buffer is generated in the positive direction
    const int adjust = (fdx < 0) ? fdx * length : 0;
    const int offset = (fx + adjust) >> 16;
    int x = offset;

    IntermediateBuffer intermediate;
    // count is the size used in the intermediate_buffer.
    int count = (qint64(length) * qAbs(fdx) + FixedScale - 1) / FixedScale + 2;
    // length is supposed to be <= BufferSize either because data->m11 < 1 or
    // data->m11 < 2, and any larger buffers split
    Q_ASSERT(count <= BufferSize + 2);
    int f = 0;
    int lim = qMin(count, image.x2 - x);
    if (x < image.x1) {
        Q_ASSERT(x < image.x2);
        uint t = s1[image.x1];
        uint b = s2[image.x1];
        quint32 rb = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
        quint32 ag = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
        do {
            intermediate.buffer_rb[f] = rb;
            intermediate.buffer_ag[f] = ag;
            f++;
            x++;
        } while (x < image.x1 && f < lim);
    }

    const __m256i disty_ = _mm256_set1_epi16(disty);
    const __m256i idisty_ = _mm256_set1_epi16(idisty);
    const __m256i colorMask = _mm256_set1_epi32(0x00ff00ff);

    lim -= 7;
    for (; f < lim; x += 8, f += 8) {
        // Load 8 pixels from s1, and split the alpha-green and red-blue component
        __m256i top = _mm256_loadu_si256((const __m256i*)((const uint *)(s1)+x));
        __m256i topAG = _mm256_srli_epi16(top, 8);
        __m256i topRB = _mm256_and_si256(top, colorMask);
        // Multiplies each color component by idisty
        topAG = _mm256_mullo_epi16 (topAG, idisty_);
        topRB = _mm256_mullo_epi16 (topRB, idisty_);

        // Same for the s2 vector
        __m256i bottom = _mm256_loadu_si256((const __m256i*)((const uint *)(s2)+x));
        __m256i bottomAG = _mm256_srli_epi16(bottom, 8);
        __m256i bottomRB = _mm256_and_si256(bottom, colorMask);
        bottomAG = _mm256_mullo_epi16 (bottomAG, disty_);
        bottomRB = _mm256_mullo_epi16 (bottomRB, disty_);

        // Add the values, and shift to only keep 8 significant bits per colors
        __m256i rAG =_mm256_add_epi16(topAG, bottomAG);
        rAG = _mm256_srli_epi16(rAG, 8);
        _mm256_storeu_si256((__m256i*)(&intermediate.buffer_ag[f]), rAG);
        __m256i rRB =_mm256_add_epi16(topRB, bottomRB);
        rRB = _mm256_srli_epi16(rRB, 8);
        _mm256_storeu_si256((__m256i*)(&intermediate.buffer_rb[f]), rRB);
    }

    for (; f < count; f++) { // Same as above but without simd
        x = qMin(x, image.x2 - 1);

        uint t = s1[x];
        uint b = s2[x];

        intermediate.buffer_rb[f] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
        intermediate.buffer_ag[f] = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
        x++;
    }

    // Now interpolate the values from the intermediate_buffer to get the final result.
    intermediate_adder_avx2(b, end, intermediate, offset, fx, fdx);
}

void QT_FASTCALL intermediate_adder_avx2(uint *b, uint *end, const IntermediateBuffer &intermediate, int offset, int &fx, int fdx)
{
    fx -= offset * FixedScale;

    const __m128i v_fdx = _mm_set1_epi32(fdx * 4);
    const __m128i v_blend = _mm_set1_epi32(0x00800080);
    const __m128i vdx_shuffle = _mm_set_epi8(char(0x80), 13, char(0x80), 13, char(0x80), 9, char(0x80), 9,
                                             char(0x80),  5, char(0x80),  5, char(0x80), 1, char(0x80), 1);
    __m128i v_fx = _mm_setr_epi32(fx, fx + fdx, fx + fdx + fdx, fx + fdx + fdx + fdx);

    while (b < end - 3) {
        const __m128i offset = _mm_srli_epi32(v_fx, 16);
        __m256i vrb = _mm256_i32gather_epi64((const long long *)intermediate.buffer_rb, offset, 4);
        __m256i vag = _mm256_i32gather_epi64((const long long *)intermediate.buffer_ag, offset, 4);

        __m128i vdx = _mm_shuffle_epi8(v_fx, vdx_shuffle);
        __m128i vidx = _mm_sub_epi16(_mm_set1_epi16(256), vdx);
        __m256i vmulx = _mm256_castsi128_si256(_mm_unpacklo_epi32(vidx, vdx));
        vmulx = _mm256_inserti128_si256(vmulx, _mm_unpackhi_epi32(vidx, vdx), 1);

        vrb = _mm256_mullo_epi16(vrb, vmulx);
        vag = _mm256_mullo_epi16(vag, vmulx);

        __m256i vrbag = _mm256_hadd_epi32(vrb, vag);
        vrbag = _mm256_permute4x64_epi64(vrbag, _MM_SHUFFLE(3, 1, 2, 0));

        __m128i rb = _mm256_castsi256_si128(vrbag);
        __m128i ag = _mm256_extracti128_si256(vrbag, 1);
        rb = _mm_srli_epi16(rb, 8);

        _mm_storeu_si128((__m128i*)b, _mm_blendv_epi8(ag, rb, v_blend));

        b += 4;
        v_fx = _mm_add_epi32(v_fx, v_fdx);
    }
    fx = _mm_cvtsi128_si32(v_fx);
    while (b < end) {
        const int x = (fx >> 16);

        const uint distx = (fx & 0x0000ffff) >> 8;
        const uint idistx = 256 - distx;
        const uint rb = (intermediate.buffer_rb[x] * idistx + intermediate.buffer_rb[x + 1] * distx) & 0xff00ff00;
        const uint ag = (intermediate.buffer_ag[x] * idistx + intermediate.buffer_ag[x + 1] * distx) & 0xff00ff00;
        *b = (rb >> 8) | ag;
        b++;
        fx += fdx;
    }
    fx += offset * FixedScale;
}

void QT_FASTCALL fetchTransformedBilinearARGB32PM_downscale_helper_avx2(uint *b, uint *end, const QTextureData &image,
                                                                        int &fx, int &fy, int fdx, int /*fdy*/)
{
    int y1 = (fy >> 16);
    int y2;
    fetchTransformedBilinear_pixelBounds(image.height, image.y1, image.y2 - 1, y1, y2);
    const uint *s1 = (const uint *)image.scanLine(y1);
    const uint *s2 = (const uint *)image.scanLine(y2);
    const int disty8 = (fy & 0x0000ffff) >> 8;
    const int disty4 = (disty8 + 0x08) >> 4;

    const qint64 min_fx = qint64(image.x1) * FixedScale;
    const qint64 max_fx = qint64(image.x2 - 1) * FixedScale;
    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        fetchTransformedBilinear_pixelBounds(image.width, image.x1, image.x2 - 1, x1, x2);
        if (x1 != x2)
            break;
        uint top = s1[x1];
        uint bot = s2[x1];
        *b = INTERPOLATE_PIXEL_256(top, 256 - disty8, bot, disty8);
        fx += fdx;
        ++b;
    }
    uint *boundedEnd = end;
    if (fdx > 0)
        boundedEnd = qMin(boundedEnd, b + (max_fx - fx) / fdx);
    else if (fdx < 0)
        boundedEnd = qMin(boundedEnd, b + (min_fx - fx) / fdx);

    // A fast middle part without boundary checks
    const __m256i vdistShuffle =
        _mm256_setr_epi8(0, char(0x80), 0, char(0x80), 4, char(0x80), 4, char(0x80), 8, char(0x80), 8, char(0x80), 12, char(0x80), 12, char(0x80),
                         0, char(0x80), 0, char(0x80), 4, char(0x80), 4, char(0x80), 8, char(0x80), 8, char(0x80), 12, char(0x80), 12, char(0x80));
    const __m256i colorMask = _mm256_set1_epi32(0x00ff00ff);
    const __m256i v_256 = _mm256_set1_epi16(256);
    const __m256i v_disty = _mm256_set1_epi16(disty4);
    const __m256i v_fdx = _mm256_set1_epi32(fdx * 8);
    const __m256i v_fx_r = _mm256_set1_epi32(0x08);
    const __m256i v_index = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
    __m256i v_fx = _mm256_set1_epi32(fx);
    v_fx = _mm256_add_epi32(v_fx, _mm256_mullo_epi32(_mm256_set1_epi32(fdx), v_index));

    while (b < boundedEnd - 7) {
        const __m256i offset = _mm256_srli_epi32(v_fx, 16);
        const __m128i offsetLo = _mm256_castsi256_si128(offset);
        const __m128i offsetHi = _mm256_extracti128_si256(offset, 1);
        const __m256i toplo = _mm256_i32gather_epi64((const long long *)s1, offsetLo, 4);
        const __m256i tophi = _mm256_i32gather_epi64((const long long *)s1, offsetHi, 4);
        const __m256i botlo = _mm256_i32gather_epi64((const long long *)s2, offsetLo, 4);
        const __m256i bothi = _mm256_i32gather_epi64((const long long *)s2, offsetHi, 4);

        __m256i v_distx = _mm256_srli_epi16(v_fx, 8);
        v_distx = _mm256_srli_epi16(_mm256_add_epi32(v_distx, v_fx_r), 4);
        v_distx = _mm256_shuffle_epi8(v_distx, vdistShuffle);

        interpolate_4_pixels_16_avx2(toplo, tophi, botlo, bothi, v_distx, v_disty, colorMask, v_256, b);
        b += 8;
        v_fx = _mm256_add_epi32(v_fx, v_fdx);
    }
    fx = _mm_extract_epi32(_mm256_castsi256_si128(v_fx) , 0);

    while (b < boundedEnd) {
        int x = (fx >> 16);
        int distx8 = (fx & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(s1 + x, s2 + x, distx8, disty8);
        fx += fdx;
        ++b;
    }

    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        fetchTransformedBilinear_pixelBounds(image.width, image.x1, image.x2 - 1, x1, x2);
        uint tl = s1[x1];
        uint tr = s1[x2];
        uint bl = s2[x1];
        uint br = s2[x2];
        int distx8 = (fx & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(tl, tr, bl, br, distx8, disty8);
        fx += fdx;
        ++b;
    }
}

void QT_FASTCALL fetchTransformedBilinearARGB32PM_fast_rotate_helper_avx2(uint *b, uint *end, const QTextureData &image,
                                                                          int &fx, int &fy, int fdx, int fdy)
{
    const qint64 min_fx = qint64(image.x1) * FixedScale;
    const qint64 max_fx = qint64(image.x2 - 1) * FixedScale;
    const qint64 min_fy = qint64(image.y1) * FixedScale;
    const qint64 max_fy = qint64(image.y2 - 1) * FixedScale;
    // first handle the possibly bounded part in the beginning
    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        int y1 = (fy >> 16);
        int y2;
        fetchTransformedBilinear_pixelBounds(image.width, image.x1, image.x2 - 1, x1, x2);
        fetchTransformedBilinear_pixelBounds(image.height, image.y1, image.y2 - 1, y1, y2);
        if (x1 != x2 && y1 != y2)
            break;
        const uint *s1 = (const uint *)image.scanLine(y1);
        const uint *s2 = (const uint *)image.scanLine(y2);
        uint tl = s1[x1];
        uint tr = s1[x2];
        uint bl = s2[x1];
        uint br = s2[x2];
        int distx = (fx & 0x0000ffff) >> 8;
        int disty = (fy & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);
        fx += fdx;
        fy += fdy;
        ++b;
    }
    uint *boundedEnd = end;
    if (fdx > 0)
        boundedEnd = qMin(boundedEnd, b + (max_fx - fx) / fdx);
    else if (fdx < 0)
        boundedEnd = qMin(boundedEnd, b + (min_fx - fx) / fdx);
    if (fdy > 0)
        boundedEnd = qMin(boundedEnd, b + (max_fy - fy) / fdy);
    else if (fdy < 0)
        boundedEnd = qMin(boundedEnd, b + (min_fy - fy) / fdy);

    // until boundedEnd we can now have a fast middle part without boundary checks
    const __m256i vdistShuffle =
        _mm256_setr_epi8(0, char(0x80), 0, char(0x80), 4, char(0x80), 4, char(0x80), 8, char(0x80), 8, char(0x80), 12, char(0x80), 12, char(0x80),
                         0, char(0x80), 0, char(0x80), 4, char(0x80), 4, char(0x80), 8, char(0x80), 8, char(0x80), 12, char(0x80), 12, char(0x80));
    const __m256i colorMask = _mm256_set1_epi32(0x00ff00ff);
    const __m256i v_256 = _mm256_set1_epi16(256);
    const __m256i v_fdx = _mm256_set1_epi32(fdx * 8);
    const __m256i v_fdy = _mm256_set1_epi32(fdy * 8);
    const __m256i v_fxy_r = _mm256_set1_epi32(0x08);
    const __m256i v_index = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
    __m256i v_fx = _mm256_set1_epi32(fx);
    __m256i v_fy = _mm256_set1_epi32(fy);
    v_fx = _mm256_add_epi32(v_fx, _mm256_mullo_epi32(_mm256_set1_epi32(fdx), v_index));
    v_fy = _mm256_add_epi32(v_fy, _mm256_mullo_epi32(_mm256_set1_epi32(fdy), v_index));

    const uchar *textureData = image.imageData;
    const qsizetype bytesPerLine = image.bytesPerLine;
    const __m256i vbpl = _mm256_set1_epi16(bytesPerLine/4);

    while (b < boundedEnd - 7) {
        const __m256i vy = _mm256_packs_epi32(_mm256_srli_epi32(v_fy, 16), _mm256_setzero_si256());
        // 8x16bit * 8x16bit -> 8x32bit
        __m256i offset = _mm256_unpacklo_epi16(_mm256_mullo_epi16(vy, vbpl), _mm256_mulhi_epi16(vy, vbpl));
        offset = _mm256_add_epi32(offset, _mm256_srli_epi32(v_fx, 16));
        const __m128i offsetLo = _mm256_castsi256_si128(offset);
        const __m128i offsetHi = _mm256_extracti128_si256(offset, 1);
        const uint *topData = (const uint *)(textureData);
        const uint *botData = (const uint *)(textureData + bytesPerLine);
        const __m256i toplo = _mm256_i32gather_epi64((const long long *)topData, offsetLo, 4);
        const __m256i tophi = _mm256_i32gather_epi64((const long long *)topData, offsetHi, 4);
        const __m256i botlo = _mm256_i32gather_epi64((const long long *)botData, offsetLo, 4);
        const __m256i bothi = _mm256_i32gather_epi64((const long long *)botData, offsetHi, 4);

        __m256i v_distx = _mm256_srli_epi16(v_fx, 8);
        __m256i v_disty = _mm256_srli_epi16(v_fy, 8);
        v_distx = _mm256_srli_epi16(_mm256_add_epi32(v_distx, v_fxy_r), 4);
        v_disty = _mm256_srli_epi16(_mm256_add_epi32(v_disty, v_fxy_r), 4);
        v_distx = _mm256_shuffle_epi8(v_distx, vdistShuffle);
        v_disty = _mm256_shuffle_epi8(v_disty, vdistShuffle);

        interpolate_4_pixels_16_avx2(toplo, tophi, botlo, bothi, v_distx, v_disty, colorMask, v_256, b);
        b += 8;
        v_fx = _mm256_add_epi32(v_fx, v_fdx);
        v_fy = _mm256_add_epi32(v_fy, v_fdy);
    }
    fx = _mm_extract_epi32(_mm256_castsi256_si128(v_fx) , 0);
    fy = _mm_extract_epi32(_mm256_castsi256_si128(v_fy) , 0);

    while (b < boundedEnd) {
        int x = (fx >> 16);
        int y = (fy >> 16);

        const uint *s1 = (const uint *)image.scanLine(y);
        const uint *s2 = (const uint *)image.scanLine(y + 1);

        int distx = (fx & 0x0000ffff) >> 8;
        int disty = (fy & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(s1 + x, s2 + x, distx, disty);

        fx += fdx;
        fy += fdy;
        ++b;
    }

    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        int y1 = (fy >> 16);
        int y2;

        fetchTransformedBilinear_pixelBounds(image.width, image.x1, image.x2 - 1, x1, x2);
        fetchTransformedBilinear_pixelBounds(image.height, image.y1, image.y2 - 1, y1, y2);

        const uint *s1 = (const uint *)image.scanLine(y1);
        const uint *s2 = (const uint *)image.scanLine(y2);

        uint tl = s1[x1];
        uint tr = s1[x2];
        uint bl = s2[x1];
        uint br = s2[x2];

        int distx = (fx & 0x0000ffff) >> 8;
        int disty = (fy & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);

        fx += fdx;
        fy += fdy;
        ++b;
    }
}

static inline __m256i epilogueMaskFromCount(qsizetype count)
{
    Q_ASSERT(count > 0);
    static const __m256i offsetMask = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);
    return _mm256_add_epi32(offsetMask, _mm256_set1_epi32(-count));
}

template<bool RGBA>
static void convertARGBToARGB32PM_avx2(uint *buffer, const uint *src, qsizetype count)
{
    qsizetype i = 0;
    const __m256i alphaMask = _mm256_set1_epi32(0xff000000);
    const __m256i rgbaMask = _mm256_broadcastsi128_si256(_mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15));
    const __m256i shuffleMask = _mm256_broadcastsi128_si256(_mm_setr_epi8(6, 7, 6, 7, 6, 7, 6, 7, 14, 15, 14, 15, 14, 15, 14, 15));
    const __m256i half = _mm256_set1_epi16(0x0080);
    const __m256i zero = _mm256_setzero_si256();

    for (; i < count - 7; i += 8) {
        __m256i srcVector = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + i));
        if (!_mm256_testz_si256(srcVector, alphaMask)) {
            // keep the two _mm_test[zc]_siXXX next to each other
            bool cf = _mm256_testc_si256(srcVector, alphaMask);
            if (RGBA)
                srcVector = _mm256_shuffle_epi8(srcVector, rgbaMask);
            if (!cf) {
                __m256i src1 = _mm256_unpacklo_epi8(srcVector, zero);
                __m256i src2 = _mm256_unpackhi_epi8(srcVector, zero);
                __m256i alpha1 = _mm256_shuffle_epi8(src1, shuffleMask);
                __m256i alpha2 = _mm256_shuffle_epi8(src2, shuffleMask);
                src1 = _mm256_mullo_epi16(src1, alpha1);
                src2 = _mm256_mullo_epi16(src2, alpha2);
                src1 = _mm256_add_epi16(src1, _mm256_srli_epi16(src1, 8));
                src2 = _mm256_add_epi16(src2, _mm256_srli_epi16(src2, 8));
                src1 = _mm256_add_epi16(src1, half);
                src2 = _mm256_add_epi16(src2, half);
                src1 = _mm256_srli_epi16(src1, 8);
                src2 = _mm256_srli_epi16(src2, 8);
                src1 = _mm256_blend_epi16(src1, alpha1, 0x88);
                src2 = _mm256_blend_epi16(src2, alpha2, 0x88);
                srcVector = _mm256_packus_epi16(src1, src2);
                _mm256_storeu_si256(reinterpret_cast<__m256i *>(buffer + i), srcVector);
            } else {
                if (buffer != src || RGBA)
                    _mm256_storeu_si256(reinterpret_cast<__m256i *>(buffer + i), srcVector);
            }
        } else {
            _mm256_storeu_si256(reinterpret_cast<__m256i *>(buffer + i), zero);
        }
    }

    if (i < count) {
        const __m256i epilogueMask = epilogueMaskFromCount(count - i);
        __m256i srcVector = _mm256_maskload_epi32(reinterpret_cast<const int *>(src + i), epilogueMask);
        const __m256i epilogueAlphaMask = _mm256_blendv_epi8(_mm256_setzero_si256(), alphaMask, epilogueMask);

        if (!_mm256_testz_si256(srcVector, epilogueAlphaMask)) {
            // keep the two _mm_test[zc]_siXXX next to each other
            bool cf = _mm256_testc_si256(srcVector, epilogueAlphaMask);
            if (RGBA)
                srcVector = _mm256_shuffle_epi8(srcVector, rgbaMask);
            if (!cf) {
                __m256i src1 = _mm256_unpacklo_epi8(srcVector, zero);
                __m256i src2 = _mm256_unpackhi_epi8(srcVector, zero);
                __m256i alpha1 = _mm256_shuffle_epi8(src1, shuffleMask);
                __m256i alpha2 = _mm256_shuffle_epi8(src2, shuffleMask);
                src1 = _mm256_mullo_epi16(src1, alpha1);
                src2 = _mm256_mullo_epi16(src2, alpha2);
                src1 = _mm256_add_epi16(src1, _mm256_srli_epi16(src1, 8));
                src2 = _mm256_add_epi16(src2, _mm256_srli_epi16(src2, 8));
                src1 = _mm256_add_epi16(src1, half);
                src2 = _mm256_add_epi16(src2, half);
                src1 = _mm256_srli_epi16(src1, 8);
                src2 = _mm256_srli_epi16(src2, 8);
                src1 = _mm256_blend_epi16(src1, alpha1, 0x88);
                src2 = _mm256_blend_epi16(src2, alpha2, 0x88);
                srcVector = _mm256_packus_epi16(src1, src2);
                _mm256_maskstore_epi32(reinterpret_cast<int *>(buffer + i), epilogueMask, srcVector);
            } else {
                if (buffer != src || RGBA)
                    _mm256_maskstore_epi32(reinterpret_cast<int *>(buffer + i), epilogueMask, srcVector);
            }
        } else {
            _mm256_maskstore_epi32(reinterpret_cast<int *>(buffer + i), epilogueMask, zero);
        }
    }
}

void QT_FASTCALL convertARGB32ToARGB32PM_avx2(uint *buffer, int count, const QVector<QRgb> *)
{
    convertARGBToARGB32PM_avx2<false>(buffer, buffer, count);
}

void QT_FASTCALL convertRGBA8888ToARGB32PM_avx2(uint *buffer, int count, const QVector<QRgb> *)
{
    convertARGBToARGB32PM_avx2<true>(buffer, buffer, count);
}

const uint *QT_FASTCALL fetchARGB32ToARGB32PM_avx2(uint *buffer, const uchar *src, int index, int count,
                                                  const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToARGB32PM_avx2<false>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_avx2(uint *buffer, const uchar *src, int index, int count,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToARGB32PM_avx2<true>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

template<bool RGBA>
static void convertARGBToRGBA64PM_avx2(QRgba64 *buffer, const uint *src, qsizetype count)
{
    qsizetype i = 0;
    const __m256i alphaMask = _mm256_set1_epi32(0xff000000);
    const __m256i rgbaMask = _mm256_broadcastsi128_si256(_mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15));
    const __m256i shuffleMask = _mm256_broadcastsi128_si256(_mm_setr_epi8(6, 7, 6, 7, 6, 7, 6, 7, 14, 15, 14, 15, 14, 15, 14, 15));
    const __m256i zero = _mm256_setzero_si256();

    for (; i < count - 7; i += 8) {
        __m256i dst1, dst2;
        __m256i srcVector = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(src + i));
        if (!_mm256_testz_si256(srcVector, alphaMask)) {
            // keep the two _mm_test[zc]_siXXX next to each other
            bool cf = _mm256_testc_si256(srcVector, alphaMask);
            if (!RGBA)
                srcVector = _mm256_shuffle_epi8(srcVector, rgbaMask);

            // The two unpack instructions unpack the low and upper halves of
            // each 128-bit half of the 256-bit register. Here's the tracking
            // of what's where: (p is 32-bit, P is 64-bit)
            //  as loaded:        [ p1, p2, p3, p4; p5, p6, p7, p8 ]
            //  after permute4x64 [ p1, p2, p5, p6; p3, p4, p7, p8 ]
            //  after unpacklo/hi [ P1, P2; P3, P4 ] [ P5, P6; P7, P8 ]
            srcVector = _mm256_permute4x64_epi64(srcVector, _MM_SHUFFLE(3, 1, 2, 0));

            const __m256i src1 = _mm256_unpacklo_epi8(srcVector, srcVector);
            const __m256i src2 = _mm256_unpackhi_epi8(srcVector, srcVector);
            if (!cf) {
                const __m256i alpha1 = _mm256_shuffle_epi8(src1, shuffleMask);
                const __m256i alpha2 = _mm256_shuffle_epi8(src2, shuffleMask);
                dst1 = _mm256_mulhi_epu16(src1, alpha1);
                dst2 = _mm256_mulhi_epu16(src2, alpha2);
                dst1 = _mm256_add_epi16(dst1, _mm256_srli_epi16(dst1, 15));
                dst2 = _mm256_add_epi16(dst2, _mm256_srli_epi16(dst2, 15));
                dst1 = _mm256_blend_epi16(dst1, src1, 0x88);
                dst2 = _mm256_blend_epi16(dst2, src2, 0x88);
            } else {
                dst1 = src1;
                dst2 = src2;
            }
        } else {
            dst1 = dst2 = zero;
        }
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(buffer + i), dst1);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(buffer + i) + 1, dst2);
    }

    if (i < count) {
        __m256i epilogueMask = epilogueMaskFromCount(count - i);
        const __m256i epilogueAlphaMask = _mm256_blendv_epi8(_mm256_setzero_si256(), alphaMask, epilogueMask);
        __m256i dst1, dst2;
        __m256i srcVector = _mm256_maskload_epi32(reinterpret_cast<const int *>(src + i), epilogueMask);

        if (!_mm256_testz_si256(srcVector, epilogueAlphaMask)) {
            // keep the two _mm_test[zc]_siXXX next to each other
            bool cf = _mm256_testc_si256(srcVector, epilogueAlphaMask);
            if (!RGBA)
                srcVector = _mm256_shuffle_epi8(srcVector, rgbaMask);
            srcVector = _mm256_permute4x64_epi64(srcVector, _MM_SHUFFLE(3, 1, 2, 0));
            const __m256i src1 = _mm256_unpacklo_epi8(srcVector, srcVector);
            const __m256i src2 = _mm256_unpackhi_epi8(srcVector, srcVector);
            if (!cf) {
                const __m256i alpha1 = _mm256_shuffle_epi8(src1, shuffleMask);
                const __m256i alpha2 = _mm256_shuffle_epi8(src2, shuffleMask);
                dst1 = _mm256_mulhi_epu16(src1, alpha1);
                dst2 = _mm256_mulhi_epu16(src2, alpha2);
                dst1 = _mm256_add_epi16(dst1, _mm256_srli_epi16(dst1, 15));
                dst2 = _mm256_add_epi16(dst2, _mm256_srli_epi16(dst2, 15));
                dst1 = _mm256_blend_epi16(dst1, src1, 0x88);
                dst2 = _mm256_blend_epi16(dst2, src2, 0x88);
            } else {
                dst1 = src1;
                dst2 = src2;
            }
        } else {
            dst1 = dst2 = zero;
        }
        epilogueMask = _mm256_permute4x64_epi64(epilogueMask, _MM_SHUFFLE(3, 1, 2, 0));
        _mm256_maskstore_epi64(reinterpret_cast<qint64 *>(buffer + i),
                               _mm256_unpacklo_epi32(epilogueMask, epilogueMask),
                               dst1);
        _mm256_maskstore_epi64(reinterpret_cast<qint64 *>(buffer + i + 4),
                               _mm256_unpackhi_epi32(epilogueMask, epilogueMask),
                               dst2);
    }
}

const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_avx2(QRgba64 *buffer, const uint *src, int count,
                                                         const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_avx2<false>(buffer, src, count);
    return buffer;
}

const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_avx2(QRgba64 *buffer, const uint *src, int count,
                                                           const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_avx2<true>(buffer, src, count);
    return buffer;
}

const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_avx2(QRgba64 *buffer, const uchar *src, int index, int count,
                                                      const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_avx2<false>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_avx2(QRgba64 *buffer, const uchar *src, int index, int count,
                                                        const QVector<QRgb> *, QDitherInfo *)
{
    convertARGBToRGBA64PM_avx2<true>(buffer, reinterpret_cast<const uint *>(src) + index, count);
    return buffer;
}

QT_END_NAMESPACE

#endif
