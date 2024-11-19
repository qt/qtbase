// Copyright (C) 2024 Loongson Technology Corporation Limited.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qimage.h>
#include <private/qimage_p.h>
#include <private/qsimd_p.h>

#ifdef QT_COMPILER_SUPPORTS_LSX

QT_BEGIN_NAMESPACE

// Convert a scanline of RGB888 (src) to RGB32 (dst)
// src must be at least len * 3 bytes
// dst must be at least len * 4 bytes
Q_GUI_EXPORT void QT_FASTCALL qt_convert_rgb888_to_rgb32_lsx(quint32 *dst, const uchar *src, int len)
{
    int i = 0;

    // Prologue, align dst to 16 bytes.
    ALIGNMENT_PROLOGUE_16BYTES(dst, i, len) {
        dst[i] = qRgb(src[0], src[1], src[2]);
        src += 3;
    }

    // Mask the 4 first colors of the RGB888 vector
    const __m128i shuffleMask = (__m128i)(v16i8){2, 1, 0, 16, 5, 4, 3, 16,
                                                 8, 7, 6, 16, 11, 10, 9, 16};
    // Mask the 4 last colors of a RGB888 vector with an offset of 1 (so the last 3 bytes are RGB)
    const __m128i shuffleMaskEnd = (__m128i)(v16i8){6, 5, 4, 16, 9, 8, 7, 16,
                                                    12, 11, 10, 16, 15, 14, 13, 16};
    // Mask to have alpha = 0xff
    const __m128i alphaMask = __lsx_vreplgr2vr_b(0xff);

    // Mask to concatenate 16-byte blocks in a and b into a 32-byte temporary result, shift the result right by 12 bytes
    const __m128i indexMask1 = (__m128i)(v16i8){12, 13, 14, 15, 16, 17, 18, 19,
                                                20, 21, 22, 23, 24, 25, 26, 27};

    // Mask to concatenate 16-byte blocks in a and b into a 32-byte temporary result, shift the result right by 8 bytes
    const __m128i indexMask2 = (__m128i)(v16i8){8, 9, 10, 11, 12, 13, 14, 15,
                                                16, 17, 18, 19, 20, 21, 22, 23};

    const __m128i *inVectorPtr = (const __m128i *)src;
    __m128i *dstVectorPtr = (__m128i *)(dst + i);

    for (; i < (len - 15); i += 16) { // one iteration in the loop converts 16 pixels
        /*
         RGB888 has 5 pixels per vector, + 1 byte from the next pixel. The idea here is
         to load vectors of RGB888 and use palignr to select a vector out of two vectors.

         After 3 loads of RGB888 and 3 stores of RGB32, we have 4 pixels left in the last
         vector of RGB888, we can mask it directly to get a last store or RGB32. After that,
         the first next byte is a R, and we can loop for the next 16 pixels.

         The conversion itself is done with a byte permutation (vshuf_b).
         */
        __m128i firstSrcVector = __lsx_vld(inVectorPtr, 0);
        __m128i outputVector = __lsx_vshuf_b(alphaMask, firstSrcVector, shuffleMask);
        __lsx_vst(outputVector, dstVectorPtr, 0);
        ++inVectorPtr;
        ++dstVectorPtr;

        // There are 4 unused bytes left in srcVector, we need to load the next 16 bytes
        __m128i secondSrcVector = __lsx_vld(inVectorPtr, 0);
        __m128i srcVector = __lsx_vshuf_b(secondSrcVector, firstSrcVector, indexMask1);
        outputVector = __lsx_vshuf_b(alphaMask, srcVector, shuffleMask);
        __lsx_vst(outputVector, dstVectorPtr, 0);
        ++inVectorPtr;
        ++dstVectorPtr;
        firstSrcVector = secondSrcVector;

        // We now have 8 unused bytes left in firstSrcVector
        secondSrcVector = __lsx_vld(inVectorPtr, 0);
        srcVector = __lsx_vshuf_b(secondSrcVector, firstSrcVector, indexMask2);
        outputVector = __lsx_vshuf_b(alphaMask, srcVector, shuffleMask);
        __lsx_vst(outputVector, dstVectorPtr, 0);
        ++inVectorPtr;
        ++dstVectorPtr;

        // There are now 12 unused bytes in firstSrcVector.
        // We can mask them directly, almost there.
        outputVector = __lsx_vshuf_b(alphaMask, secondSrcVector, shuffleMaskEnd);
        __lsx_vst(outputVector, dstVectorPtr, 0);
        ++dstVectorPtr;
    }
    src = (const uchar *)inVectorPtr;

    SIMD_EPILOGUE(i, len, 15) {
        dst[i] = qRgb(src[0], src[1], src[2]);
        src += 3;
    }
}

void convert_RGB888_to_RGB32_lsx(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
{
    Q_ASSERT(src->format == QImage::Format_RGB888 || src->format == QImage::Format_BGR888);
    if (src->format == QImage::Format_BGR888)
        Q_ASSERT(dest->format == QImage::Format_RGBX8888 || dest->format == QImage::Format_RGBA8888 || dest->format == QImage::Format_RGBA8888_Premultiplied);
    else
        Q_ASSERT(dest->format == QImage::Format_RGB32 || dest->format == QImage::Format_ARGB32 || dest->format == QImage::Format_ARGB32_Premultiplied);
    Q_ASSERT(src->width == dest->width);
    Q_ASSERT(src->height == dest->height);

    const uchar *src_data = (uchar *) src->data;
    quint32 *dest_data = (quint32 *) dest->data;

    for (int i = 0; i < src->height; ++i) {
        qt_convert_rgb888_to_rgb32_lsx(dest_data, src_data, src->width);
        src_data += src->bytes_per_line;
        dest_data = (quint32 *)((uchar*)dest_data + dest->bytes_per_line);
    }
}

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_LSX
