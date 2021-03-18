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

#include <private/qdrawhelper_x86_p.h>

#if defined(QT_COMPILER_SUPPORTS_SSSE3)

#include <private/qdrawingprimitive_sse2_p.h>

QT_BEGIN_NAMESPACE

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
static inline void Q_DECL_VECTORCALL
BLEND_SOURCE_OVER_ARGB32_SSSE3(quint32 *dst, const quint32 *src, int length,
                               __m128i nullVector, __m128i half, __m128i one, __m128i colorMask, __m128i alphaMask)
{
    int x = 0;

    /* First, get dst aligned. */
    ALIGNMENT_PROLOGUE_16BYTES(dst, x, length) {
        blend_pixel(dst[x], src[x]);
    }

    const int minusOffsetToAlignSrcOn16Bytes = (reinterpret_cast<quintptr>(&(src[x])) >> 2) & 0x3;

    if (!minusOffsetToAlignSrcOn16Bytes) {
        /* src is aligned, usual algorithm but with aligned operations.
           See the SSE2 version for more documentation on the algorithm itself. */
        const __m128i alphaShuffleMask = _mm_set_epi8(char(0xff),15,char(0xff),15,char(0xff),11,char(0xff),11,char(0xff),7,char(0xff),7,char(0xff),3,char(0xff),3);
        for (; x < length-3; x += 4) {
            const __m128i srcVector = _mm_load_si128((const __m128i *)&src[x]);
            const __m128i srcVectorAlpha = _mm_and_si128(srcVector, alphaMask);
            if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, alphaMask)) == 0xffff) {
                _mm_store_si128((__m128i *)&dst[x], srcVector);
            } else if (_mm_movemask_epi8(_mm_cmpeq_epi32(srcVectorAlpha, nullVector)) != 0xffff) {
                __m128i alphaChannel = _mm_shuffle_epi8(srcVector, alphaShuffleMask);
                alphaChannel = _mm_sub_epi16(one, alphaChannel);
                const __m128i dstVector = _mm_load_si128((__m128i *)&dst[x]);
                __m128i destMultipliedByOneMinusAlpha;
                BYTE_MUL_SSE2(destMultipliedByOneMinusAlpha, dstVector, alphaChannel, colorMask, half);
                const __m128i result = _mm_add_epi8(srcVector, destMultipliedByOneMinusAlpha);
                _mm_store_si128((__m128i *)&dst[x], result);
            }
        } /* end for() */
    } else if ((length - x) >= 8) {
        /* We use two vectors to extract the src: prevLoaded for the first pixels, lastLoaded for the current pixels. */
        __m128i srcVectorPrevLoaded = _mm_load_si128((const __m128i *)&src[x - minusOffsetToAlignSrcOn16Bytes]);
        const int palignrOffset = minusOffsetToAlignSrcOn16Bytes << 2;

        const __m128i alphaShuffleMask = _mm_set_epi8(char(0xff),15,char(0xff),15,char(0xff),11,char(0xff),11,char(0xff),7,char(0xff),7,char(0xff),3,char(0xff),3);
        switch (palignrOffset) {
        case 4:
            BLENDING_LOOP(4, length)
            break;
        case 8:
            BLENDING_LOOP(8, length)
            break;
        case 12:
            BLENDING_LOOP(12, length)
            break;
        }
    }
    for (; x < length; ++x)
        blend_pixel(dst[x], src[x]);
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

const uint *QT_FASTCALL fetchPixelsBPP24_ssse3(uint *buffer, const uchar *src, int index, int count)
{
    const quint24 *s = reinterpret_cast<const quint24 *>(src);
    for (int i = 0; i < count; ++i)
        buffer[i] = s[index + i];
    return buffer;
}

extern void QT_FASTCALL qt_convert_rgb888_to_rgb32_ssse3(quint32 *dst, const uchar *src, int len);

const uint * QT_FASTCALL qt_fetchUntransformed_888_ssse3(uint *buffer, const Operator *, const QSpanData *data,
                                                         int y, int x, int length)
{
    const uchar *line = data->texture.scanLine(y) + x * 3;
    qt_convert_rgb888_to_rgb32_ssse3(buffer, line, length);
    return buffer;
}

void qt_memfill24_ssse3(quint24 *dest, quint24 color, qsizetype count)
{
    // LCM of 12 and 16 bytes is 48 bytes (16 px)
    quint32 v = color;
    __m128i m = _mm_cvtsi32_si128(v);
    quint24 *end = dest + count;

    constexpr uchar x = 2, y = 1, z = 0;
    Q_DECL_ALIGN(__m128i) static const uchar
    shuffleMask[16 + 1] = { x, y, z, x,  y, z, x, y,  z, x, y, z,  x, y, z, x,  y };

    __m128i mval1 = _mm_shuffle_epi8(m, _mm_load_si128(reinterpret_cast<const __m128i *>(shuffleMask)));
    __m128i mval2 = _mm_shuffle_epi8(m, _mm_loadu_si128(reinterpret_cast<const __m128i *>(shuffleMask + 1)));
    __m128i mval3 = _mm_alignr_epi8(mval2, mval1, 2);

    for ( ; dest + 16 <= end; dest += 16) {
#ifdef __AVX__
        // Store using 32-byte AVX instruction
        __m256 mval12 = _mm256_castps128_ps256(_mm_castsi128_ps(mval1));
        mval12 = _mm256_insertf128_ps(mval12, _mm_castsi128_ps(mval2), 1);
        _mm256_storeu_ps(reinterpret_cast<float *>(dest), mval12);
#else
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest) + 0, mval1);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest) + 1, mval2);
#endif
        _mm_storeu_si128(reinterpret_cast<__m128i *>(dest) + 2, mval3);
    }

    if (count < 3) {
        if (count > 1)
            end[-2] = v;
        if (count)
            end[-1] = v;
        return;
    }

    // less than 16px/48B left
    uchar *ptr = reinterpret_cast<uchar *>(dest);
    uchar *ptr_end = reinterpret_cast<uchar *>(end);
    qptrdiff left = ptr_end - ptr;
    if (left >= 24) {
        // 8px/24B or more left
        _mm_storeu_si128(reinterpret_cast<__m128i *>(ptr) + 0, mval1);
        _mm_storel_epi64(reinterpret_cast<__m128i *>(ptr) + 1, mval2);
        ptr += 24;
        left -= 24;
    }

    // less than 8px/24B left

    if (left >= 16) {
        // but more than 5px/15B left
        _mm_storeu_si128(reinterpret_cast<__m128i *>(ptr) , mval1);
    } else if (left >= 8) {
        // but more than 2px/6B left
        _mm_storel_epi64(reinterpret_cast<__m128i *>(ptr), mval1);
    }

    if (left) {
        // 1 or 2px left
        // store 8 bytes ending with the right values (will overwrite a bit)
        _mm_storel_epi64(reinterpret_cast<__m128i *>(ptr_end - 8), mval2);
    }
}

void QT_FASTCALL rbSwap_888_ssse3(uchar *dst, const uchar *src, int count)
{
    int i = 0;

    const static __m128i shuffleMask1 = _mm_setr_epi8(2, 1, 0, 5, 4, 3, 8, 7, 6, 11, 10, 9, 14, 13, 12, /*!!*/15);
    const static __m128i shuffleMask2 = _mm_setr_epi8(0, /*!!*/1, 4, 3, 2, 7, 6, 5, 10, 9, 8, 13, 12, 11, /*!!*/14, 15);
    const static __m128i shuffleMask3 = _mm_setr_epi8(/*!!*/0, 3, 2, 1, 6, 5, 4, 9, 8, 7, 12, 11, 10, 15, 14, 13);

    for (; i + 15 < count; i += 16) {
        __m128i s1 = _mm_loadu_si128((const __m128i *)src);
        __m128i s2 = _mm_loadu_si128((const __m128i *)(src + 16));
        __m128i s3 = _mm_loadu_si128((const __m128i *)(src + 32));
        s1 = _mm_shuffle_epi8(s1, shuffleMask1);
        s2 = _mm_shuffle_epi8(s2, shuffleMask2);
        s3 = _mm_shuffle_epi8(s3, shuffleMask3);
        _mm_storeu_si128((__m128i *)dst, s1);
        _mm_storeu_si128((__m128i *)(dst + 16), s2);
        _mm_storeu_si128((__m128i *)(dst + 32), s3);

        // Now fix the last four misplaced values
        std::swap(dst[15], dst[17]);
        std::swap(dst[30], dst[32]);

        src += 48;
        dst += 48;
    }

    if (src != dst) {
        SIMD_EPILOGUE(i, count, 15) {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst += 3;
            src += 3;
        }
    } else {
        SIMD_EPILOGUE(i, count, 15) {
            std::swap(dst[0], dst[2]);
            dst += 3;
        }
    }
}

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_SSSE3
