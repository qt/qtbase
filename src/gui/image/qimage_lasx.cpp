// Copyright (C) 2024 Loongson Technology Corporation Limited.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qimage.h>
#include <private/qimage_p.h>
#include <private/qsimd_p.h>

#ifdef QT_COMPILER_SUPPORTS_LASX

QT_BEGIN_NAMESPACE

// Convert a scanline of RGB888 (src) to RGB32 (dst)
// src must be at least len * 3 bytes
// dst must be at least len * 4 bytes
Q_GUI_EXPORT void QT_FASTCALL qt_convert_rgb888_to_rgb32_lasx(quint32 *dst, const uchar *src, int len)
{
    int i = 0;

    // Prologue, align dst to 32 bytes.
    ALIGNMENT_PROLOGUE_32BYTES(dst, i, len) {
        dst[i] = qRgb(src[0], src[1], src[2]);
        src += 3;
    }

    // Mask 8 colors of the RGB888 vector
    const __m256i shuffleMask1 = (__m256i)(v32i8){2, 1, 0, 16, 5, 4, 3, 16, 8, 7, 6, 16, 11, 10, 9, 16,
                                                  30, 29, 28, 16, 1, 0, 31, 16, 4, 3, 2, 16, 7, 6, 5, 16};

    // Mask 8 colors of a RGB888 vector with an offset of shuffleMask1
    const __m256i shuffleMask2 = (__m256i)(v32i8){10, 9, 8, 0, 13, 12, 11, 0, 16, 15, 14, 0, 19, 18, 17, 0,
                                                  6, 5, 4, 0, 9, 8, 7, 0, 12, 11, 10, 0, 15, 14, 13, 0};
    const __m256i alphaMask = __lasx_xvreplgr2vr_w(0xff000000);
    const __m256i *inVectorPtr = (const __m256i *)src;
    __m256i *dstVectorPtr = (__m256i *)(dst + i);

    for (; i < (len - 31); i += 32) { // one iteration in the loop converts 32 pixels
        /*
         RGB888 has 10 pixels per vector, + 2 byte from the next pixel. The idea here is
         to load vectors of RGB888 and use palignr to select a vector out of two vectors.

         After 3 loads of RGB888 and 3 stores of RGB32, we have 8 pixels left in the last
         vector of RGB888, we can mask it directly to get a last store or RGB32. After that,
         the first next byte is a R, and we can loop for the next 32 pixels.

         The conversion itself is done with a byte permutation (xvshuf_b and xvpermi_q).
         */
        __m256i firstSrcVector = __lasx_xvld(inVectorPtr, 0);
        __m256i rFirstSrcVector = __lasx_xvpermi_q(firstSrcVector, firstSrcVector, 0b00000001);
        __m256i outputVector = __lasx_xvshuf_b(rFirstSrcVector, firstSrcVector, shuffleMask1);
        __lasx_xvst(__lasx_xvor_v(outputVector, alphaMask), dstVectorPtr, 0);
        ++inVectorPtr;
        ++dstVectorPtr;

        // There are 8 unused bytes left in srcVector, we need to load the next 32 bytes
        // and load the next input with palignr
        __m256i secondSrcVector = __lasx_xvld(inVectorPtr, 0);
        __m256i srcVector = __lasx_xvpermi_q(secondSrcVector, firstSrcVector, 0b00100001);
        __m256i rSrcVector = __lasx_xvpermi_q(srcVector, srcVector, 0b00000001);
        outputVector = __lasx_xvshuf_b(rSrcVector, srcVector, shuffleMask2);

        __lasx_xvst(__lasx_xvor_v(outputVector, alphaMask), dstVectorPtr, 0);
        ++inVectorPtr;
        ++dstVectorPtr;

        // We now have 16 unused bytes left in firstSrcVector
        __m256i thirdSrcVector = __lasx_xvld(inVectorPtr, 0);
        srcVector = __lasx_xvpermi_q(thirdSrcVector, secondSrcVector, 0b00100001);
        rSrcVector = __lasx_xvpermi_q(srcVector, srcVector, 0b00000001);
        outputVector = __lasx_xvshuf_b(rSrcVector, srcVector, shuffleMask1);
        __lasx_xvst(__lasx_xvor_v(outputVector, alphaMask), dstVectorPtr, 0);
        ++inVectorPtr;
        ++dstVectorPtr;

        // There are now 24 unused bytes in firstSrcVector.
        // We can mask them directly, almost there.
        srcVector = thirdSrcVector;
        rSrcVector = __lasx_xvpermi_q(srcVector, srcVector, 0b00000001);
        outputVector = __lasx_xvshuf_b(rSrcVector, srcVector, shuffleMask2);
        __lasx_xvst(__lasx_xvor_v(outputVector, alphaMask), dstVectorPtr, 0);
        ++dstVectorPtr;
    }
    src = (const uchar *)inVectorPtr;

    SIMD_EPILOGUE(i, len, 31) {
        dst[i] = qRgb(src[0], src[1], src[2]);
        src += 3;
    }
}

void convert_RGB888_to_RGB32_lasx(QImageData *dest, const QImageData *src, Qt::ImageConversionFlags)
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
        qt_convert_rgb888_to_rgb32_lasx(dest_data, src_data, src->width);
        src_data += src->bytes_per_line;
        dest_data = (quint32 *)((uchar*)dest_data + dest->bytes_per_line);
    }
}

QT_END_NAMESPACE

#endif // QT_COMPILER_SUPPORTS_LASX
