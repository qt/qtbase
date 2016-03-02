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

#include <private/qdrawhelper_x86_p.h>

#ifdef QT_COMPILER_SUPPORTS_SSSE3

#include <private/qdrawingprimitive_sse2_p.h>

QT_BEGIN_NAMESPACE

inline static void blend_pixel(quint32 &dst, const quint32 src)
{
    if (src >= 0xff000000)
        dst = src;
    else if (src != 0)
        dst = src + BYTE_MUL(dst, qAlpha(~src));
}


/* The instruction palignr uses direct arguments, so we have to generate the code fo the different
   shift (4, 8, 12). Checking the alignment inside the loop is unfortunatelly way too slow.
 */
#define BLENDING_LOOP(palignrOffset, length)\
    for (; x-minusOffsetToAlignSrcOn16Bytes < length-7; x += 4) { \
        const __m128i srcVectorLastLoaded = _mm_load_si128((const __m128i *)&src[x - minusOffsetToAlignSrcOn16Bytes + 4]);\
        const __m128i srcVector = _mm_alignr_epi8(srcVectorLastLoaded, srcVectorPrevLoaded, palignrOffset); \
        const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask); \
        if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) { \
            _mm_store_si128((__m128i *)&dst[x], srcVector); \
        } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) != 0xffff) { \
            __m128i alphaChannel = _mm_shuffle_epi8(srcVector, alphaShuffleMask); \
            alphaChannel = _mm_sub_epi16(one, alphaChannel); \
            const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]); \
            __m128i destMultipliedByOneMinusAlpha; \
            BYTE_MUL_SSE2(destMultipliedByOneMinusAlpha, dstVector, alphaChannel, colorMask, half); \
            const __m128i result = _mm_add_epi8(srcVector, destMultipliedByOneMinusAlpha); \
            _mm_store_si128((__m128i *)&dst[x], result); \
        } \
        srcVectorPrevLoaded = srcVectorLastLoaded;\
    }


// Basically blend src over dst with the const alpha defined as constAlphaVector.
// nullVector, half, one, colorMask are constant across the whole image/texture, and should be defined as:
//const __m128i nullVector = _mm_set1_epi32(0);
//const __m128i half = _mm_set1_epi16(0x80);
//const __m128i one = _mm_set1_epi16(0xff);
//const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
//const __m128i alphaMask = _mm_set1_epi32(0xff000000);
//
// The computation being done is:
// result = s + d * (1-alpha)
// with shortcuts if fully opaque or fully transparent.
#define BLEND_SOURCE_OVER_ARGB32_SSSE3(dst, src, length, nullVector, half, one, colorMask, alphaMask) { \
    int x = 0; \
\
    /* First, get dst aligned. */ \
    ALIGNMENT_PROLOGUE_16BYTES(dst, x, length) { \
        blend_pixel(dst[x], src[x]); \
    } \
\
    const int minusOffsetToAlignSrcOn16Bytes = (reinterpret_cast<quintptr>(&(src[x])) >> 2) & 0x3;\
\
    if (!minusOffsetToAlignSrcOn16Bytes) {\
        /* src is aligned, usual algorithm but with aligned operations.\
           See the SSE2 version for more documentation on the algorithm itself. */\
        const __m128i alphaShuffleMask = _mm_set_epi8(char(0xff),15,char(0xff),15,char(0xff),11,char(0xff),11,char(0xff),7,char(0xff),7,char(0xff),3,char(0xff),3);\
        for (; x < length-3; x += 4) { \
            const __m128i srcVector = _mm_load_si128((const __m128i *)&src[x]); \
            const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask); \
            if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) { \
                _mm_store_si128((__m128i *)&dst[x], srcVector); \
            } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) != 0xffff) { \
                __m128i alphaChannel = _mm_shuffle_epi8(srcVector, alphaShuffleMask); \
                alphaChannel = _mm_sub_epi16(one, alphaChannel); \
                const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]); \
                __m128i destMultipliedByOneMinusAlpha; \
                BYTE_MUL_SSE2(destMultipliedByOneMinusAlpha, dstVector, alphaChannel, colorMask, half); \
                const __m128i result = _mm_add_epi8(srcVector, destMultipliedByOneMinusAlpha); \
                _mm_store_si128((__m128i *)&dst[x], result); \
            } \
        } /* end for() */\
    } else if ((length - x) >= 8) {\
        /* We use two vectors to extract the src: prevLoaded for the first pixels, lastLoaded for the current pixels. */\
        __m128i srcVectorPrevLoaded = _mm_load_si128((const __m128i *)&src[x - minusOffsetToAlignSrcOn16Bytes]);\
        const int palignrOffset = minusOffsetToAlignSrcOn16Bytes << 2;\
\
        const __m128i alphaShuffleMask = _mm_set_epi8(char(0xff),15,char(0xff),15,char(0xff),11,char(0xff),11,char(0xff),7,char(0xff),7,char(0xff),3,char(0xff),3);\
        switch (palignrOffset) {\
        case 4:\
            BLENDING_LOOP(4, length)\
            break;\
        case 8:\
            BLENDING_LOOP(8, length)\
            break;\
        case 12:\
            BLENDING_LOOP(12, length)\
            break;\
        }\
    }\
    for (; x < length; ++x) \
        blend_pixel(dst[x], src[x]); \
}

void qt_blend_argb32_on_argb32_ssse3(uchar *destPixels, int dbpl,
                                     const uchar *srcPixels, int sbpl,
                                     int w, int h,
                                     int const_alpha)
{
    const quint32 *src = (const quint32 *) srcPixels;
    quint32 *dst = (quint32 *) destPixels;
    if (const_alpha == 256) {
        const __m128i alphaMask = _mm_set1_epi32(0xff000000);
        const __m128i nullVector = _mm_setzero_si128();
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i one = _mm_set1_epi16(0xff);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);

        for (int y = 0; y < h; ++y) {
            BLEND_SOURCE_OVER_ARGB32_SSSE3(dst, src, w, nullVector, half, one, colorMask, alphaMask);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    } else if (const_alpha != 0) {
        // dest = (s + d * sia) * ca + d * cia
        //      = s * ca + d * (sia * ca + cia)
        //      = s * ca + d * (1 - sa*ca)
        const_alpha = (const_alpha * 255) >> 8;
        const __m128i nullVector = _mm_setzero_si128();
        const __m128i half = _mm_set1_epi16(0x80);
        const __m128i one = _mm_set1_epi16(0xff);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        const __m128i constAlphaVector = _mm_set1_epi16(const_alpha);
        for (int y = 0; y < h; ++y) {
            BLEND_SOURCE_OVER_ARGB32_WITH_CONST_ALPHA_SSE2(dst, src, w, nullVector, half, one, colorMask, constAlphaVector)
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    }
}

static inline void store_uint24_ssse3(uchar *dst, const uint *src, int len)
{
    int i = 0;

    quint24 *dst24 = reinterpret_cast<quint24*>(dst);
    // Align dst on 16 bytes
    for (; i < len && (reinterpret_cast<quintptr>(dst24) & 0xf); ++i)
        *dst24++ = quint24(*src++);

    // Shuffle masks for first and second half of every output, all outputs are aligned so the shuffled ends are not used.
    const __m128i shuffleMask1 = _mm_setr_epi8(char(0x80), char(0x80), char(0x80), char(0x80), 2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12);
    const __m128i shuffleMask2 = _mm_setr_epi8(2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, char(0x80), char(0x80), char(0x80), char(0x80));

    const __m128i *inVectorPtr = (const __m128i *)src;
    __m128i *dstVectorPtr = (__m128i *)dst24;

    for (; i < (len - 15); i += 16) {
        // Load four vectors, store three.
        // Create each output vector by combining two shuffled input vectors.
        __m128i srcVector1 = _mm_loadu_si128(inVectorPtr);
        ++inVectorPtr;
        __m128i srcVector2 = _mm_loadu_si128(inVectorPtr);
        ++inVectorPtr;
        __m128i outputVector1 = _mm_shuffle_epi8(srcVector1, shuffleMask1);
        __m128i outputVector2 = _mm_shuffle_epi8(srcVector2, shuffleMask2);
        __m128i outputVector = _mm_alignr_epi8(outputVector2, outputVector1, 4);
        _mm_store_si128(dstVectorPtr, outputVector);
        ++dstVectorPtr;

        srcVector1 = _mm_loadu_si128(inVectorPtr);
        ++inVectorPtr;
        outputVector1 = _mm_shuffle_epi8(srcVector2, shuffleMask1);
        outputVector2 = _mm_shuffle_epi8(srcVector1, shuffleMask2);
        outputVector = _mm_alignr_epi8(outputVector2, outputVector1, 8);
        _mm_store_si128(dstVectorPtr, outputVector);
        ++dstVectorPtr;

        srcVector2 = _mm_loadu_si128(inVectorPtr);
        ++inVectorPtr;
        outputVector1 = _mm_shuffle_epi8(srcVector1, shuffleMask1);
        outputVector2 = _mm_shuffle_epi8(srcVector2, shuffleMask2);
        outputVector = _mm_alignr_epi8(outputVector2, outputVector1, 12);
        _mm_store_si128(dstVectorPtr, outputVector);
        ++dstVectorPtr;
    }
    dst24 = reinterpret_cast<quint24*>(dstVectorPtr);
    src = reinterpret_cast<const uint*>(inVectorPtr);

    for (; i < len; ++i)
        *dst24++ = quint24(*src++);
}

void QT_FASTCALL storePixelsBPP24_ssse3(uchar *dest, const uint *src, int index, int count)
{
    store_uint24_ssse3(dest + index * 3, src, count);
}

extern void QT_FASTCALL qt_convert_rgb888_to_rgb32_ssse3(quint32 *dst, const uchar *src, int len);

const uint * QT_FASTCALL qt_fetchUntransformed_888_ssse3(uint *buffer, const Operator *, const QSpanData *data,
                                                         int y, int x, int length)
{
    const uchar *line = data->texture.scanLine(y) + x * 3;
    qt_convert_rgb888_to_rgb32_ssse3(buffer, line, length);
    return buffer;
}

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_SSSE3
