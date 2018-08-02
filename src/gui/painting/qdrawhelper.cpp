/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <qglobal.h>

#include <qstylehints.h>
#include <qguiapplication.h>
#include <qatomic.h>
#include <private/qcolorprofile_p.h>
#include <private/qdrawhelper_p.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qpainter_p.h>
#include <private/qdrawhelper_x86_p.h>
#include <private/qdrawingprimitive_sse2_p.h>
#include <private/qdrawhelper_neon_p.h>
#if defined(QT_COMPILER_SUPPORTS_MIPS_DSP) || defined(QT_COMPILER_SUPPORTS_MIPS_DSPR2)
#include <private/qdrawhelper_mips_dsp_p.h>
#endif
#include <private/qguiapplication_p.h>
#include <private/qrgba64_p.h>
#include <qloggingcategory.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQtGuiDrawHelper, "qt.gui.drawhelper")

#define MASK(src, a) src = BYTE_MUL(src, a)

/*
  constants and structures
*/

enum {
    fixed_scale = 1 << 16,
    half_point = 1 << 15
};

// must be multiple of 4 for easier SIMD implementations
static const int buffer_size = 2048;

template<QImage::Format> Q_DECL_CONSTEXPR uint redWidth();
template<QImage::Format> Q_DECL_CONSTEXPR uint redShift();
template<QImage::Format> Q_DECL_CONSTEXPR uint greenWidth();
template<QImage::Format> Q_DECL_CONSTEXPR uint greenShift();
template<QImage::Format> Q_DECL_CONSTEXPR uint blueWidth();
template<QImage::Format> Q_DECL_CONSTEXPR uint blueShift();
template<QImage::Format> Q_DECL_CONSTEXPR uint alphaWidth();
template<QImage::Format> Q_DECL_CONSTEXPR uint alphaShift();

template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGB16>() { return 5; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB4444_Premultiplied>() { return 4; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB8555_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB8565_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB6666_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB16>() { return  11; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB444>() { return  8; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB555>() { return 10; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB666>() { return 12; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB888>() { return 16; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB4444_Premultiplied>() { return  8; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB8555_Premultiplied>() { return 18; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB8565_Premultiplied>() { return 19; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB6666_Premultiplied>() { return 12; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB16>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB4444_Premultiplied>() { return 4; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB8555_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB8565_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB6666_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB16>() { return  5; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB4444_Premultiplied>() { return  4; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB8555_Premultiplied>() { return 13; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB8565_Premultiplied>() { return 13; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB6666_Premultiplied>() { return  6; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB16>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB4444_Premultiplied>() { return 4; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB8555_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB8565_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB6666_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB16>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB444>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB555>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB666>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB4444_Premultiplied>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB8555_Premultiplied>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB8565_Premultiplied>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB6666_Premultiplied>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGB16>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGB444>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGB555>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGB666>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGB888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB4444_Premultiplied>() { return  4; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB8555_Premultiplied>() { return  8; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB8565_Premultiplied>() { return  8; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB6666_Premultiplied>() { return  6; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGB16>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGB444>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGB555>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGB666>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGB888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB4444_Premultiplied>() { return 12; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB8555_Premultiplied>() { return  0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB8565_Premultiplied>() { return  0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB6666_Premultiplied>() { return 18; }

template<QImage::Format> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel();
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB16>() { return QPixelLayout::BPP16; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB444>() { return QPixelLayout::BPP16; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB555>() { return QPixelLayout::BPP16; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB666>() { return QPixelLayout::BPP24; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB888>() { return QPixelLayout::BPP24; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB4444_Premultiplied>() { return QPixelLayout::BPP16; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB8555_Premultiplied>() { return QPixelLayout::BPP24; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB8565_Premultiplied>() { return QPixelLayout::BPP24; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB6666_Premultiplied>() { return QPixelLayout::BPP24; }


template<QImage::Format Format>
static const uint *QT_FASTCALL convertToRGB32(uint *buffer, const uint *src, int count,
                                              const QVector<QRgb> *, QDitherInfo *)
{
    auto conversion = [](uint s) {
        // MSVC needs these constexpr defined in here otherwise it will create a capture.
        Q_CONSTEXPR uint redMask = ((1 << redWidth<Format>()) - 1);
        Q_CONSTEXPR uint greenMask = ((1 << greenWidth<Format>()) - 1);
        Q_CONSTEXPR uint blueMask = ((1 << blueWidth<Format>()) - 1);

        Q_CONSTEXPR uchar redLeftShift = 8 - redWidth<Format>();
        Q_CONSTEXPR uchar greenLeftShift = 8 - greenWidth<Format>();
        Q_CONSTEXPR uchar blueLeftShift = 8 - blueWidth<Format>();

        Q_CONSTEXPR uchar redRightShift = 2 * redWidth<Format>() - 8;
        Q_CONSTEXPR uchar greenRightShift = 2 * greenWidth<Format>() - 8;
        Q_CONSTEXPR uchar blueRightShift = 2 * blueWidth<Format>() - 8;

        uint red   = (s >> redShift<Format>()) & redMask;
        uint green = (s >> greenShift<Format>()) & greenMask;
        uint blue  = (s >> blueShift<Format>()) & blueMask;

        red = ((red << redLeftShift) | (red >> redRightShift)) << 16;
        green = ((green << greenLeftShift) | (green >> greenRightShift)) << 8;
        blue = (blue << blueLeftShift) | (blue >> blueRightShift);
        return 0xff000000 | red | green | blue;
    };

    UNALIASED_CONVERSION_LOOP(buffer, src, count, conversion);
    return buffer;
}

template<QImage::Format Format>
static const QRgba64 *QT_FASTCALL convertToRGB64(QRgba64 *buffer, const uint *src, int count,
                                                 const QVector<QRgb> *, QDitherInfo *)
{
    Q_CONSTEXPR uint redMask = ((1 << redWidth<Format>()) - 1);
    Q_CONSTEXPR uint greenMask = ((1 << greenWidth<Format>()) - 1);
    Q_CONSTEXPR uint blueMask = ((1 << blueWidth<Format>()) - 1);

    Q_CONSTEXPR uchar redLeftShift = 8 - redWidth<Format>();
    Q_CONSTEXPR uchar greenLeftShift = 8 - greenWidth<Format>();
    Q_CONSTEXPR uchar blueLeftShift = 8 - blueWidth<Format>();

    Q_CONSTEXPR uchar redRightShift = 2 * redWidth<Format>() - 8;
    Q_CONSTEXPR uchar greenRightShift = 2 * greenWidth<Format>() - 8;
    Q_CONSTEXPR uchar blueRightShift = 2 * blueWidth<Format>() - 8;

    for (int i = 0; i < count; ++i) {
        uint red = (src[i] >> redShift<Format>()) & redMask;
        uint green = (src[i] >> greenShift<Format>()) & greenMask;
        uint blue = (src[i] >> blueShift<Format>()) & blueMask;

        red = ((red << redLeftShift) | (red >> redRightShift));
        green = ((green << greenLeftShift) | (green >> greenRightShift));
        blue = (blue << blueLeftShift) | (blue >> blueRightShift);
        buffer[i] = QRgba64::fromRgba(red, green, blue, 255);
    }

    return buffer;
}

template<QImage::Format Format>
static const uint *QT_FASTCALL convertARGBPMToARGB32PM(uint *buffer, const uint *src, int count,
                                                       const QVector<QRgb> *, QDitherInfo *)
{
    Q_CONSTEXPR uint alphaMask = ((1 << alphaWidth<Format>()) - 1);
    Q_CONSTEXPR uint redMask = ((1 << redWidth<Format>()) - 1);
    Q_CONSTEXPR uint greenMask = ((1 << greenWidth<Format>()) - 1);
    Q_CONSTEXPR uint blueMask = ((1 << blueWidth<Format>()) - 1);

    Q_CONSTEXPR uchar alphaLeftShift = 8 - alphaWidth<Format>();
    Q_CONSTEXPR uchar redLeftShift = 8 - redWidth<Format>();
    Q_CONSTEXPR uchar greenLeftShift = 8 - greenWidth<Format>();
    Q_CONSTEXPR uchar blueLeftShift = 8 - blueWidth<Format>();

    Q_CONSTEXPR uchar alphaRightShift = 2 * alphaWidth<Format>() - 8;
    Q_CONSTEXPR uchar redRightShift = 2 * redWidth<Format>() - 8;
    Q_CONSTEXPR uchar greenRightShift = 2 * greenWidth<Format>() - 8;
    Q_CONSTEXPR uchar blueRightShift = 2 * blueWidth<Format>() - 8;

    Q_CONSTEXPR bool mustMin = (alphaWidth<Format>() != redWidth<Format>()) ||
                               (alphaWidth<Format>() != greenWidth<Format>()) ||
                               (alphaWidth<Format>() != blueWidth<Format>());

    if (mustMin) {
        for (int i = 0; i < count; ++i) {
            uint alpha = (src[i] >> alphaShift<Format>()) & alphaMask;
            uint red = (src[i] >> redShift<Format>()) & redMask;
            uint green = (src[i] >> greenShift<Format>()) & greenMask;
            uint blue = (src[i] >> blueShift<Format>()) & blueMask;

            alpha = (alpha << alphaLeftShift) | (alpha >> alphaRightShift);
            red = qMin(alpha, (red << redLeftShift) | (red >> redRightShift));
            green = qMin(alpha, (green << greenLeftShift) | (green >> greenRightShift));
            blue = qMin(alpha, (blue << blueLeftShift) | (blue >> blueRightShift));
            buffer[i] = (alpha << 24) | (red << 16) | (green << 8) | blue;
        }
    } else {
        for (int i = 0; i < count; ++i) {
            uint alpha = (src[i] >> alphaShift<Format>()) & alphaMask;
            uint red   = (src[i] >> redShift<Format>())   & redMask;
            uint green = (src[i] >> greenShift<Format>()) & greenMask;
            uint blue  = (src[i] >> blueShift<Format>())  & blueMask;

            alpha = ((alpha << alphaLeftShift) | (alpha >> alphaRightShift)) << 24;
            red   = ((red << redLeftShift) | (red >> redRightShift)) << 16;
            green = ((green << greenLeftShift) | (green >> greenRightShift)) << 8;
            blue  = (blue << blueLeftShift)  | (blue >> blueRightShift);
            buffer[i] = alpha | red | green | blue;
        }
    }

    return buffer;
}

template<QImage::Format Format>
static const QRgba64 *QT_FASTCALL convertARGBPMToARGB64PM(QRgba64 *buffer, const uint *src, int count,
                                                          const QVector<QRgb> *, QDitherInfo *)
{
    Q_CONSTEXPR uint alphaMask = ((1 << alphaWidth<Format>()) - 1);
    Q_CONSTEXPR uint redMask = ((1 << redWidth<Format>()) - 1);
    Q_CONSTEXPR uint greenMask = ((1 << greenWidth<Format>()) - 1);
    Q_CONSTEXPR uint blueMask = ((1 << blueWidth<Format>()) - 1);

    Q_CONSTEXPR uchar alphaLeftShift = 8 - alphaWidth<Format>();
    Q_CONSTEXPR uchar redLeftShift = 8 - redWidth<Format>();
    Q_CONSTEXPR uchar greenLeftShift = 8 - greenWidth<Format>();
    Q_CONSTEXPR uchar blueLeftShift = 8 - blueWidth<Format>();

    Q_CONSTEXPR uchar alphaRightShift = 2 * alphaWidth<Format>() - 8;
    Q_CONSTEXPR uchar redRightShift = 2 * redWidth<Format>() - 8;
    Q_CONSTEXPR uchar greenRightShift = 2 * greenWidth<Format>() - 8;
    Q_CONSTEXPR uchar blueRightShift = 2 * blueWidth<Format>() - 8;

    Q_CONSTEXPR bool mustMin = (alphaWidth<Format>() != redWidth<Format>()) ||
                               (alphaWidth<Format>() != greenWidth<Format>()) ||
                               (alphaWidth<Format>() != blueWidth<Format>());

    if (mustMin) {
        for (int i = 0; i < count; ++i) {
            uint alpha = (src[i] >> alphaShift<Format>()) & alphaMask;
            uint red = (src[i] >> redShift<Format>()) & redMask;
            uint green = (src[i] >> greenShift<Format>()) & greenMask;
            uint blue = (src[i] >> blueShift<Format>()) & blueMask;

            alpha = (alpha << alphaLeftShift) | (alpha >> alphaRightShift);
            red = qMin(alpha, (red << redLeftShift) | (red >> redRightShift));
            green = qMin(alpha, (green << greenLeftShift) | (green >> greenRightShift));
            blue = qMin(alpha, (blue << blueLeftShift) | (blue >> blueRightShift));
            buffer[i] = QRgba64::fromRgba(red, green, blue, alpha);
        }
    } else {
        for (int i = 0; i < count; ++i) {
            uint alpha = (src[i] >> alphaShift<Format>()) & alphaMask;
            uint red = (src[i] >> redShift<Format>()) & redMask;
            uint green = (src[i] >> greenShift<Format>()) & greenMask;
            uint blue = (src[i] >> blueShift<Format>()) & blueMask;

            alpha = (alpha << alphaLeftShift) | (alpha >> alphaRightShift);
            red = (red << redLeftShift) | (red >> redRightShift);
            green = (green << greenLeftShift) | (green >> greenRightShift);
            blue = (blue << blueLeftShift) | (blue >> blueRightShift);
            buffer[i] = QRgba64::fromRgba(red, green, blue, alpha);
        }
    }

    return buffer;
}

template<QImage::Format Format, bool fromRGB>
static const uint *QT_FASTCALL convertRGBFromARGB32PM(uint *buffer, const uint *src, int count,
                                                      const QVector<QRgb> *, QDitherInfo *dither)
{
    Q_CONSTEXPR uchar rWidth = redWidth<Format>();
    Q_CONSTEXPR uchar gWidth = greenWidth<Format>();
    Q_CONSTEXPR uchar bWidth = blueWidth<Format>();

    // RGB32 -> RGB888 is not a precision loss.
    if (!dither || (rWidth == 8 && gWidth == 8 && bWidth == 8)) {
        auto conversion = [](uint s) {
            const uint c = fromRGB ? s : qUnpremultiply(s);
            Q_CONSTEXPR uint rMask = (1 << redWidth<Format>()) - 1;
            Q_CONSTEXPR uint gMask = (1 << greenWidth<Format>()) - 1;
            Q_CONSTEXPR uint bMask = (1 << blueWidth<Format>()) - 1;
            Q_CONSTEXPR uchar rRightShift = 24 - redWidth<Format>();
            Q_CONSTEXPR uchar gRightShift = 16 - greenWidth<Format>();
            Q_CONSTEXPR uchar bRightShift =  8 - blueWidth<Format>();

            const uint r = ((c >> rRightShift) & rMask) << redShift<Format>();
            const uint g = ((c >> gRightShift) & gMask) << greenShift<Format>();
            const uint b = ((c >> bRightShift) & bMask) << blueShift<Format>();
            return r | g | b;
        };
        UNALIASED_CONVERSION_LOOP(buffer, src, count, conversion);
    } else {
        // We do ordered dither by using a rounding conversion, but instead of
        // adding half of input precision, we add the adjusted result from the
        // bayer matrix before narrowing.
        // Note: Rounding conversion in itself is different from the naive
        // conversion we do above for non-dithering.
        const uint *bayer_line = qt_bayer_matrix[dither->y & 15];
        for (int i = 0; i < count; ++i) {
            const uint c = fromRGB ? src[i] : qUnpremultiply(src[i]);
            const int d = bayer_line[(dither->x + i) & 15];
            const int dr = d - ((d + 1) >> rWidth);
            const int dg = d - ((d + 1) >> gWidth);
            const int db = d - ((d + 1) >> bWidth);
            int r = qRed(c);
            int g = qGreen(c);
            int b = qBlue(c);
            r = (r + ((dr - r) >> rWidth) + 1) >> (8 - rWidth);
            g = (g + ((dg - g) >> gWidth) + 1) >> (8 - gWidth);
            b = (b + ((db - b) >> bWidth) + 1) >> (8 - bWidth);
            buffer[i] = (r << redShift<Format>())
                      | (g << greenShift<Format>())
                      | (b << blueShift<Format>());
        }
    }
    return buffer;
}

template<QImage::Format Format, bool fromRGB>
static const uint *QT_FASTCALL convertARGBPMFromARGB32PM(uint *buffer, const uint *src, int count,
                                                         const QVector<QRgb> *, QDitherInfo *dither)
{
    if (!dither) {
        auto conversion = [](uint c) {
            Q_CONSTEXPR uint aMask = (1 << alphaWidth<Format>()) - 1;
            Q_CONSTEXPR uint rMask = (1 << redWidth<Format>()) - 1;
            Q_CONSTEXPR uint gMask = (1 << greenWidth<Format>()) - 1;
            Q_CONSTEXPR uint bMask = (1 << blueWidth<Format>()) - 1;

            Q_CONSTEXPR uchar aRightShift = 32 - alphaWidth<Format>();
            Q_CONSTEXPR uchar rRightShift = 24 - redWidth<Format>();
            Q_CONSTEXPR uchar gRightShift = 16 - greenWidth<Format>();
            Q_CONSTEXPR uchar bRightShift =  8 - blueWidth<Format>();

            Q_CONSTEXPR uint aOpaque = aMask << alphaShift<Format>();
            const uint a = fromRGB ? aOpaque : (((c >> aRightShift) & aMask) << alphaShift<Format>());
            const uint r = ((c >> rRightShift) & rMask) << redShift<Format>();
            const uint g = ((c >> gRightShift) & gMask) << greenShift<Format>();
            const uint b = ((c >> bRightShift) & bMask) << blueShift<Format>();
            return a | r | g | b;
        };
        UNALIASED_CONVERSION_LOOP(buffer, src, count, conversion);
    } else {
        Q_CONSTEXPR uchar aWidth = alphaWidth<Format>();
        Q_CONSTEXPR uchar rWidth = redWidth<Format>();
        Q_CONSTEXPR uchar gWidth = greenWidth<Format>();
        Q_CONSTEXPR uchar bWidth = blueWidth<Format>();

        const uint *bayer_line = qt_bayer_matrix[dither->y & 15];
        for (int i = 0; i < count; ++i) {
            const uint c = src[i];
            const int d = bayer_line[(dither->x + i) & 15];
            const int da = d - ((d + 1) >> aWidth);
            const int dr = d - ((d + 1) >> rWidth);
            const int dg = d - ((d + 1) >> gWidth);
            const int db = d - ((d + 1) >> bWidth);
            int a = qAlpha(c);
            int r = qRed(c);
            int g = qGreen(c);
            int b = qBlue(c);
            if (fromRGB)
                a = (1 << aWidth) - 1;
            else
                a = (a + ((da - a) >> aWidth) + 1) >> (8 - aWidth);
            r = (r + ((dr - r) >> rWidth) + 1) >> (8 - rWidth);
            g = (g + ((dg - g) >> gWidth) + 1) >> (8 - gWidth);
            b = (b + ((db - b) >> bWidth) + 1) >> (8 - bWidth);
            buffer[i] = (a << alphaShift<Format>())
                      | (r << redShift<Format>())
                      | (g << greenShift<Format>())
                      | (b << blueShift<Format>());
        }
    }
    return buffer;
}

#ifdef Q_COMPILER_CONSTEXPR

template<QImage::Format Format> Q_DECL_CONSTEXPR static inline QPixelLayout pixelLayoutRGB()
{
    return QPixelLayout{
        uchar(redWidth<Format>()), uchar(redShift<Format>()),
        uchar(greenWidth<Format>()), uchar(greenShift<Format>()),
        uchar(blueWidth<Format>()), uchar(blueShift<Format>()),
        0, 0,
        false, bitsPerPixel<Format>(),
        convertToRGB32<Format>,
        convertRGBFromARGB32PM<Format, false>,
        convertRGBFromARGB32PM<Format, true>,
        convertToRGB64<Format>
    };
}

template<QImage::Format Format> Q_DECL_CONSTEXPR static inline QPixelLayout pixelLayoutARGBPM()
{
    return QPixelLayout{
        uchar(redWidth<Format>()), uchar(redShift<Format>()),
        uchar(greenWidth<Format>()), uchar(greenShift<Format>()),
        uchar(blueWidth<Format>()), uchar(blueShift<Format>()),
        uchar(alphaWidth<Format>()), uchar(alphaShift<Format>()),
        true, bitsPerPixel<Format>(),
        convertARGBPMToARGB32PM<Format>,
        convertARGBPMFromARGB32PM<Format, false>,
        convertARGBPMFromARGB32PM<Format, true>,
        convertARGBPMToARGB64PM<Format>
    };
}

#endif

// To convert in place, let 'dest' and 'src' be the same.
static const uint *QT_FASTCALL convertIndexedToARGB32PM(uint *buffer, const uint *src, int count,
                                                        const QVector<QRgb> *clut, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qPremultiply(clut->at(src[i]));
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertIndexedToARGB64PM(QRgba64 *buffer, const uint *src, int count,
                                                           const QVector<QRgb> *clut, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(clut->at(src[i])).premultiplied();
    return buffer;
}

static const uint *QT_FASTCALL convertPassThrough(uint *, const uint *src, int,
                                                  const QVector<QRgb> *, QDitherInfo *)
{
    return src;
}

static const uint *QT_FASTCALL convertARGB32ToARGB32PM(uint *buffer, const uint *src, int count,
                                                       const QVector<QRgb> *, QDitherInfo *)
{
    return qt_convertARGB32ToARGB32PM(buffer, src, count);
}

static const uint *QT_FASTCALL convertRGBA8888PMToARGB32PM(uint *buffer, const uint *src, int count,
                                                           const QVector<QRgb> *, QDitherInfo *)
{
    UNALIASED_CONVERSION_LOOP(buffer, src, count, RGBA2ARGB);
    return buffer;
}

static const uint *QT_FASTCALL convertRGBA8888ToARGB32PM(uint *buffer, const uint *src, int count,
                                                         const QVector<QRgb> *, QDitherInfo *)
{
    return qt_convertRGBA8888ToARGB32PM(buffer, src, count);
}

static const uint *QT_FASTCALL convertAlpha8ToRGB32(uint *buffer, const uint *src, int count,
                                                    const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qRgba(0, 0, 0, src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertGrayscale8ToRGB32(uint *buffer, const uint *src, int count,
                                                        const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qRgb(src[i], src[i], src[i]);
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertAlpha8ToRGB64(QRgba64 *buffer, const uint *src, int count,
                                                       const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromRgba(0, 0, 0, src[i]);
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertGrayscale8ToRGB64(QRgba64 *buffer, const uint *src, int count,
                                                           const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromRgba(src[i], src[i], src[i], 255);
    return buffer;
}

static const uint *QT_FASTCALL convertARGB32FromARGB32PM(uint *buffer, const uint *src, int count,
                                                         const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qUnpremultiply(src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertRGBA8888PMFromARGB32PM(uint *buffer, const uint *src, int count,
                                                             const QVector<QRgb> *, QDitherInfo *)
{
    UNALIASED_CONVERSION_LOOP(buffer, src, count, ARGB2RGBA);
    return buffer;
}

#ifdef __SSE2__
template<bool RGBA, bool maskAlpha>
static inline void qConvertARGB32PMToARGB64PM_sse2(QRgba64 *buffer, const uint *src, int count)
{
    if (count <= 0)
        return;

    const __m128i amask = _mm_set1_epi32(0xff000000);
    int i = 0;
    for (; ((uintptr_t)buffer & 0xf) && i < count; ++i) {
        uint s = *src++;
        if (maskAlpha)
            s = s | 0xff000000;
        if (RGBA)
            s = RGBA2ARGB(s);
        *buffer++ = QRgba64::fromArgb32(s);
    }
    for (; i < count-3; i += 4) {
        __m128i vs = _mm_loadu_si128((const __m128i*)src);
        if (maskAlpha)
            vs = _mm_or_si128(vs, amask);
        src += 4;
        __m128i v1 = _mm_unpacklo_epi8(vs, vs);
        __m128i v2 = _mm_unpackhi_epi8(vs, vs);
        if (!RGBA) {
            v1 = _mm_shufflelo_epi16(v1, _MM_SHUFFLE(3, 0, 1, 2));
            v2 = _mm_shufflelo_epi16(v2, _MM_SHUFFLE(3, 0, 1, 2));
            v1 = _mm_shufflehi_epi16(v1, _MM_SHUFFLE(3, 0, 1, 2));
            v2 = _mm_shufflehi_epi16(v2, _MM_SHUFFLE(3, 0, 1, 2));
        }
        _mm_store_si128((__m128i*)(buffer), v1);
        buffer += 2;
        _mm_store_si128((__m128i*)(buffer), v2);
        buffer += 2;
    }

    SIMD_EPILOGUE(i, count, 3) {
        uint s = *src++;
        if (maskAlpha)
            s = s | 0xff000000;
        if (RGBA)
            s = RGBA2ARGB(s);
        *buffer++ = QRgba64::fromArgb32(s);
    }
}
#elif defined(__ARM_NEON__)
template<bool RGBA, bool maskAlpha>
static inline void qConvertARGB32PMToRGBA64PM_neon(QRgba64 *buffer, const uint *src, int count)
{
    if (count <= 0)
        return;

    const uint32x4_t amask = vdupq_n_u32(0xff000000);
#if defined(Q_PROCESSOR_ARM_64)
    const uint8x16_t rgbaMask  = { 2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15};
#else
    const uint8x8_t rgbaMask  = { 2, 1, 0, 3, 6, 5, 4, 7 };
#endif
    int i = 0;
    for (; i < count-3; i += 4) {
        uint32x4_t vs32 = vld1q_u32(src);
        src += 4;
        if (maskAlpha)
            vs32 = vorrq_u32(vs32, amask);
        uint8x16_t vs8 = vreinterpretq_u8_u32(vs32);
        if (!RGBA) {
#if defined(Q_PROCESSOR_ARM_64)
            vs8 = vqtbl1q_u8(vs8, rgbaMask);
#else
            // no vqtbl1q_u8
            const uint8x8_t vlo = vtbl1_u8(vget_low_u8(vs8), rgbaMask);
            const uint8x8_t vhi = vtbl1_u8(vget_high_u8(vs8), rgbaMask);
            vs8 = vcombine_u8(vlo, vhi);
#endif
        }
        uint8x16x2_t v = vzipq_u8(vs8, vs8);

        vst1q_u16((uint16_t *)buffer, vreinterpretq_u16_u8(v.val[0]));
        buffer += 2;
        vst1q_u16((uint16_t *)buffer, vreinterpretq_u16_u8(v.val[1]));
        buffer += 2;
    }

    SIMD_EPILOGUE(i, count, 3) {
        uint s = *src++;
        if (maskAlpha)
            s = s | 0xff000000;
        if (RGBA)
            s = RGBA2ARGB(s);
        *buffer++ = QRgba64::fromArgb32(s);
    }
}
#endif

static const QRgba64 *QT_FASTCALL convertRGB32ToRGB64(QRgba64 *buffer, const uint *src, int count,
                                                      const QVector<QRgb> *, QDitherInfo *)
{
#ifdef __SSE2__
    qConvertARGB32PMToARGB64PM_sse2<false, true>(buffer, src, count);
#elif defined(__ARM_NEON__)
    qConvertARGB32PMToRGBA64PM_neon<false, true>(buffer, src, count);
#else
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(0xff000000 | src[i]);
#endif
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertARGB32ToARGB64PM(QRgba64 *buffer, const uint *src, int count,
                                                          const QVector<QRgb> *, QDitherInfo *)
{
#ifdef __SSE2__
    qConvertARGB32PMToARGB64PM_sse2<false, false>(buffer, src, count);
    for (int i = 0; i < count; ++i)
        buffer[i] = buffer[i].premultiplied();
#elif defined(__ARM_NEON__)
    qConvertARGB32PMToRGBA64PM_neon<false, false>(buffer, src, count);
    for (int i = 0; i < count; ++i)
        buffer[i] = buffer[i].premultiplied();
#else
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(src[i]).premultiplied();
#endif
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertARGB32PMToARGB64PM(QRgba64 *buffer, const uint *src, int count,
                                                            const QVector<QRgb> *, QDitherInfo *)
{
#ifdef __SSE2__
    qConvertARGB32PMToARGB64PM_sse2<false, false>(buffer, src, count);
#elif defined(__ARM_NEON__)
    qConvertARGB32PMToRGBA64PM_neon<false, false>(buffer, src, count);
#else
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(src[i]);
#endif
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertRGBA8888ToARGB64PM(QRgba64 *buffer, const uint *src, int count,
                                                            const QVector<QRgb> *, QDitherInfo *)
{
#ifdef __SSE2__
    qConvertARGB32PMToARGB64PM_sse2<true, false>(buffer, src, count);
    for (int i = 0; i < count; ++i)
        buffer[i] = buffer[i].premultiplied();
#elif defined(__ARM_NEON__)
    qConvertARGB32PMToRGBA64PM_neon<true, false>(buffer, src, count);
    for (int i = 0; i < count; ++i)
        buffer[i] = buffer[i].premultiplied();
#else
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(RGBA2ARGB(src[i])).premultiplied();
#endif
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertRGBA8888PMToARGB64PM(QRgba64 *buffer, const uint *src, int count,
                                                              const QVector<QRgb> *, QDitherInfo *)
{
#ifdef __SSE2__
    qConvertARGB32PMToARGB64PM_sse2<true, false>(buffer, src, count);
#elif defined(__ARM_NEON__)
    qConvertARGB32PMToRGBA64PM_neon<true, false>(buffer, src, count);
#else
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(RGBA2ARGB(src[i]));
#endif
    return buffer;
}

static const uint *QT_FASTCALL convertRGBA8888FromARGB32PM(uint *buffer, const uint *src, int count,
                                                           const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = ARGB2RGBA(qUnpremultiply(src[i]));
    return buffer;
}

static const uint *QT_FASTCALL convertRGBXFromRGB32(uint *buffer, const uint *src, int count,
                                                    const QVector<QRgb> *, QDitherInfo *)
{
    UNALIASED_CONVERSION_LOOP(buffer, src, count, [](uint c) { return ARGB2RGBA(0xff000000 | c); });
    return buffer;
}

static const uint *QT_FASTCALL convertRGBXFromARGB32PM(uint *buffer, const uint *src, int count,
                                                       const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = ARGB2RGBA(0xff000000 | qUnpremultiply(src[i]));
    return buffer;
}

template<QtPixelOrder PixelOrder>
static const uint *QT_FASTCALL convertA2RGB30PMToARGB32PM(uint *buffer, const uint *src, int count,
                                                          const QVector<QRgb> *, QDitherInfo *dither)
{
    if (!dither) {
        UNALIASED_CONVERSION_LOOP(buffer, src, count, qConvertA2rgb30ToArgb32<PixelOrder>);
    } else {
        for (int i = 0; i < count; ++i) {
            const uint c = src[i];
            short d10 = (qt_bayer_matrix[dither->y & 15][(dither->x + i) & 15] << 2);
            short a10 = (c >> 30) * 0x155;
            short r10 = ((c >> 20) & 0x3ff);
            short g10 = ((c >> 10) & 0x3ff);
            short b10 = (c & 0x3ff);
            if (PixelOrder == PixelOrderBGR)
                std::swap(r10, b10);
            short a8 = (a10 + ((d10 - a10) >> 8)) >> 2;
            short r8 = (r10 + ((d10 - r10) >> 8)) >> 2;
            short g8 = (g10 + ((d10 - g10) >> 8)) >> 2;
            short b8 = (b10 + ((d10 - b10) >> 8)) >> 2;
            buffer[i] = qRgba(r8, g8, b8, a8);
        }
    }
    return buffer;
}

#ifdef __SSE2__
template<QtPixelOrder PixelOrder>
static inline void qConvertA2RGB30PMToARGB64PM_sse2(QRgba64 *buffer, const uint *src, int count)
{
    if (count <= 0)
        return;

    const __m128i rmask = _mm_set1_epi32(0x3ff00000);
    const __m128i gmask = _mm_set1_epi32(0x000ffc00);
    const __m128i bmask = _mm_set1_epi32(0x000003ff);
    const __m128i afactor = _mm_set1_epi16(0x5555);
    int i = 0;

    for (; ((uintptr_t)buffer & 0xf) && i < count; ++i)
        *buffer++ = qConvertA2rgb30ToRgb64<PixelOrder>(*src++);

    for (; i < count-3; i += 4) {
        __m128i vs = _mm_loadu_si128((const __m128i*)src);
        src += 4;
        __m128i va = _mm_srli_epi32(vs, 30);
        __m128i vr = _mm_and_si128(vs, rmask);
        __m128i vb = _mm_and_si128(vs, bmask);
        __m128i vg = _mm_and_si128(vs, gmask);
        va = _mm_mullo_epi16(va, afactor);
        vr = _mm_or_si128(_mm_srli_epi32(vr, 14), _mm_srli_epi32(vr, 24));
        vg = _mm_or_si128(_mm_srli_epi32(vg, 4), _mm_srli_epi32(vg, 14));
        vb = _mm_or_si128(_mm_slli_epi32(vb, 6), _mm_srli_epi32(vb, 4));
        __m128i vrb;
        if (PixelOrder == PixelOrderRGB)
             vrb = _mm_or_si128(vr, _mm_slli_si128(vb, 2));
        else
             vrb = _mm_or_si128(vb, _mm_slli_si128(vr, 2));
        __m128i vga = _mm_or_si128(vg, _mm_slli_si128(va, 2));
        _mm_store_si128((__m128i*)(buffer), _mm_unpacklo_epi16(vrb, vga));
        buffer += 2;
        _mm_store_si128((__m128i*)(buffer), _mm_unpackhi_epi16(vrb, vga));
        buffer += 2;
    }

    SIMD_EPILOGUE(i, count, 3)
        *buffer++ = qConvertA2rgb30ToRgb64<PixelOrder>(*src++);
}
#endif

template<QtPixelOrder PixelOrder>
static const QRgba64 *QT_FASTCALL convertA2RGB30PMToARGB64PM(QRgba64 *buffer, const uint *src, int count,
                                                             const QVector<QRgb> *, QDitherInfo *)
{
#ifdef __SSE2__
    qConvertA2RGB30PMToARGB64PM_sse2<PixelOrder>(buffer, src, count);
#else
    for (int i = 0; i < count; ++i)
        buffer[i] = qConvertA2rgb30ToRgb64<PixelOrder>(src[i]);
#endif
    return buffer;
}

template<QtPixelOrder PixelOrder>
static const uint *QT_FASTCALL convertA2RGB30PMFromARGB32PM(uint *buffer, const uint *src, int count,
                                                            const QVector<QRgb> *, QDitherInfo *)
{
    UNALIASED_CONVERSION_LOOP(buffer, src, count, qConvertArgb32ToA2rgb30<PixelOrder>);
    return buffer;
}

template<QtPixelOrder PixelOrder>
static const uint *QT_FASTCALL convertRGB30FromRGB32(uint *buffer, const uint *src, int count,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qConvertRgb32ToRgb30<PixelOrder>(src[i]);
    return buffer;
}

template<QtPixelOrder PixelOrder>
static const uint *QT_FASTCALL convertRGB30FromARGB32PM(uint *buffer, const uint *src, int count,
                                                        const QVector<QRgb> *, QDitherInfo *)
{
    UNALIASED_CONVERSION_LOOP(buffer, src, count, qConvertRgb32ToRgb30<PixelOrder>);
    return buffer;
}

static const uint *QT_FASTCALL convertAlpha8FromARGB32PM(uint *buffer, const uint *src, int count,
                                                         const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qAlpha(src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertGrayscale8FromRGB32(uint *buffer, const uint *src, int count,
                                                          const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qGray(src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertGrayscale8FromARGB32PM(uint *buffer, const uint *src, int count,
                                                             const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qGray(qUnpremultiply(src[i]));
    return buffer;
}

template <QPixelLayout::BPP bpp> static
uint QT_FASTCALL fetchPixel(const uchar *, int)
{
    Q_UNREACHABLE();
    return 0;
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP1LSB>(const uchar *src, int index)
{
    return (src[index >> 3] >> (index & 7)) & 1;
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP1MSB>(const uchar *src, int index)
{
    return (src[index >> 3] >> (~index & 7)) & 1;
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP8>(const uchar *src, int index)
{
    return src[index];
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP16>(const uchar *src, int index)
{
    return reinterpret_cast<const quint16 *>(src)[index];
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP24>(const uchar *src, int index)
{
    return reinterpret_cast<const quint24 *>(src)[index];
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP32>(const uchar *src, int index)
{
    return reinterpret_cast<const uint *>(src)[index];
}

template <QPixelLayout::BPP bpp>
inline const uint *QT_FASTCALL fetchPixels(uint *buffer, const uchar *src, int index, int count)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = fetchPixel<bpp>(src, index + i);
    return buffer;
}

template <>
inline const uint *QT_FASTCALL fetchPixels<QPixelLayout::BPP32>(uint *, const uchar *src, int index, int)
{
    return reinterpret_cast<const uint *>(src) + index;
}

template <QPixelLayout::BPP width> static
void QT_FASTCALL storePixel(uchar *dest, int index, uint pixel);

template <>
inline void QT_FASTCALL storePixel<QPixelLayout::BPP1LSB>(uchar *dest, int index, uint pixel)
{
    if (pixel)
        dest[index >> 3] |= 1 << (index & 7);
    else
        dest[index >> 3] &= ~(1 << (index & 7));
}

template <>
inline void QT_FASTCALL storePixel<QPixelLayout::BPP1MSB>(uchar *dest, int index, uint pixel)
{
    if (pixel)
        dest[index >> 3] |= 1 << (~index & 7);
    else
        dest[index >> 3] &= ~(1 << (~index & 7));
}

template <>
inline void QT_FASTCALL storePixel<QPixelLayout::BPP8>(uchar *dest, int index, uint pixel)
{
    dest[index] = uchar(pixel);
}

template <>
inline void QT_FASTCALL storePixel<QPixelLayout::BPP16>(uchar *dest, int index, uint pixel)
{
    reinterpret_cast<quint16 *>(dest)[index] = quint16(pixel);
}

template <>
inline void QT_FASTCALL storePixel<QPixelLayout::BPP24>(uchar *dest, int index, uint pixel)
{
    reinterpret_cast<quint24 *>(dest)[index] = quint24(pixel);
}

template <QPixelLayout::BPP width>
inline void QT_FASTCALL storePixels(uchar *dest, const uint *src, int index, int count)
{
    for (int i = 0; i < count; ++i)
        storePixel<width>(dest, index + i, src[i]);
}

template <>
inline void QT_FASTCALL storePixels<QPixelLayout::BPP32>(uchar *dest, const uint *src, int index, int count)
{
    memcpy(reinterpret_cast<uint *>(dest) + index, src, count * sizeof(uint));
}

// Note:
// convertToArgb32() assumes that no color channel is less than 4 bits.
// convertFromArgb32() assumes that no color channel is more than 8 bits.
// QImage::rgbSwapped() assumes that the red and blue color channels have the same number of bits.
QPixelLayout qPixelLayouts[QImage::NImageFormats] = {
    { 0,  0, 0,  0, 0,  0, 0,  0, false, QPixelLayout::BPPNone, 0, 0, 0, 0 }, // Format_Invalid
    { 0,  0, 0,  0, 0,  0, 0,  0, false, QPixelLayout::BPP1MSB, convertIndexedToARGB32PM, 0, 0, convertIndexedToARGB64PM }, // Format_Mono
    { 0,  0, 0,  0, 0,  0, 0,  0, false, QPixelLayout::BPP1LSB, convertIndexedToARGB32PM, 0, 0, convertIndexedToARGB64PM  }, // Format_MonoLSB
    { 0,  0, 0,  0, 0,  0, 0,  0, false, QPixelLayout::BPP8, convertIndexedToARGB32PM, 0, 0, convertIndexedToARGB64PM  }, // Format_Indexed8
    // Technically using convertPassThrough to convert from ARGB32PM to RGB32 is wrong,
    // but everywhere this generic conversion would be wrong is currently overloaded.
    { 8, 16, 8,  8, 8,  0, 0,  0, false, QPixelLayout::BPP32, convertPassThrough, convertPassThrough, convertPassThrough, convertRGB32ToRGB64 }, // Format_RGB32
    { 8, 16, 8,  8, 8,  0, 8, 24, false, QPixelLayout::BPP32, convertARGB32ToARGB32PM, convertARGB32FromARGB32PM, convertPassThrough, convertARGB32ToARGB64PM }, // Format_ARGB32
    { 8, 16, 8,  8, 8,  0, 8, 24,  true, QPixelLayout::BPP32, convertPassThrough, convertPassThrough, convertPassThrough, convertARGB32PMToARGB64PM }, // Format_ARGB32_Premultiplied
#ifdef Q_COMPILER_CONSTEXPR
    pixelLayoutRGB<QImage::Format_RGB16>(),
    pixelLayoutARGBPM<QImage::Format_ARGB8565_Premultiplied>(),
    pixelLayoutRGB<QImage::Format_RGB666>(),
    pixelLayoutARGBPM<QImage::Format_ARGB6666_Premultiplied>(),
    pixelLayoutRGB<QImage::Format_RGB555>(),
    pixelLayoutARGBPM<QImage::Format_ARGB8555_Premultiplied>(),
    pixelLayoutRGB<QImage::Format_RGB888>(),
    pixelLayoutRGB<QImage::Format_RGB444>(),
    pixelLayoutARGBPM<QImage::Format_ARGB4444_Premultiplied>(),
#else
    { 5, 11, 6,  5, 5,  0, 0,  0, false, QPixelLayout::BPP16,
      convertToRGB32<QImage::Format_RGB16>,
      convertRGBFromARGB32PM<QImage::Format_RGB16, false>,
      convertRGBFromARGB32PM<QImage::Format_RGB16, true>,
      convertToRGB64<QImage::Format_RGB16>,
    },
    { 5, 19, 6, 13, 5,  8, 8,  0,  true, QPixelLayout::BPP24,
      convertARGBPMToARGB32PM<QImage::Format_ARGB8565_Premultiplied>,
      convertARGBPMFromARGB32PM<QImage::Format_ARGB8565_Premultiplied, false>,
      convertARGBPMFromARGB32PM<QImage::Format_ARGB8565_Premultiplied, true>,
      convertARGBPMToARGB64PM<QImage::Format_ARGB8565_Premultiplied>,
    },
    { 6, 12, 6,  6, 6,  0, 0,  0, false, QPixelLayout::BPP24,
      convertToRGB32<QImage::Format_RGB666>,
      convertRGBFromARGB32PM<QImage::Format_RGB666, false>,
      convertRGBFromARGB32PM<QImage::Format_RGB666, true>,
      convertToRGB64<QImage::Format_RGB666>,
    },
    { 6, 12, 6,  6, 6,  0, 6, 18,  true, QPixelLayout::BPP24,
      convertARGBPMToARGB32PM<QImage::Format_ARGB6666_Premultiplied>,
      convertARGBPMFromARGB32PM<QImage::Format_ARGB6666_Premultiplied, false>,
      convertARGBPMFromARGB32PM<QImage::Format_ARGB6666_Premultiplied, true>,
      convertARGBPMToARGB64PM<QImage::Format_ARGB6666_Premultiplied>,
    },
    { 5, 10, 5,  5, 5,  0, 0,  0, false, QPixelLayout::BPP16,
      convertToRGB32<QImage::Format_RGB555>,
      convertRGBFromARGB32PM<QImage::Format_RGB555, false>,
      convertRGBFromARGB32PM<QImage::Format_RGB555, true>,
      convertToRGB64<QImage::Format_RGB555>,
    },
    { 5, 18, 5, 13, 5,  8, 8,  0,  true, QPixelLayout::BPP24,
      convertARGBPMToARGB32PM<QImage::Format_ARGB8555_Premultiplied>,
      convertARGBPMFromARGB32PM<QImage::Format_ARGB8555_Premultiplied, false>,
      convertARGBPMFromARGB32PM<QImage::Format_ARGB8555_Premultiplied, true>,
      convertARGBPMToARGB64PM<QImage::Format_ARGB8555_Premultiplied>,
    },
    { 8, 16, 8,  8, 8,  0, 0,  0, false, QPixelLayout::BPP24,
      convertToRGB32<QImage::Format_RGB888>,
      convertRGBFromARGB32PM<QImage::Format_RGB888, false>,
      convertRGBFromARGB32PM<QImage::Format_RGB888, true>,
      convertToRGB64<QImage::Format_RGB888>,
    },
    { 4,  8, 4,  4, 4,  0, 0,  0, false, QPixelLayout::BPP16,
      convertToRGB32<QImage::Format_RGB444>,
      convertRGBFromARGB32PM<QImage::Format_RGB444, false>,
      convertRGBFromARGB32PM<QImage::Format_RGB444, true>,
      convertToRGB64<QImage::Format_RGB444>,
    },
    { 4,  8, 4,  4, 4,  0, 4, 12,  true, QPixelLayout::BPP16,
      convertARGBPMToARGB32PM<QImage::Format_ARGB4444_Premultiplied>,
      convertARGBPMFromARGB32PM<QImage::Format_ARGB4444_Premultiplied, false>,
      convertARGBPMFromARGB32PM<QImage::Format_ARGB4444_Premultiplied, true>,
      convertARGBPMToARGB64PM<QImage::Format_ARGB4444_Premultiplied>,
    },
#endif
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    { 8, 24, 8, 16, 8,  8, 0,  0, false, QPixelLayout::BPP32, convertRGBA8888PMToARGB32PM, convertRGBXFromARGB32PM, convertRGBXFromRGB32, convertRGBA8888PMToARGB64PM }, // Format_RGBX8888
    { 8, 24, 8, 16, 8,  8, 8,  0, false, QPixelLayout::BPP32, convertRGBA8888ToARGB32PM, convertRGBA8888FromARGB32PM, convertRGBXFromRGB32, convertRGBA8888ToARGB64PM }, // Format_RGBA8888
    { 8, 24, 8, 16, 8,  8, 8,  0,  true, QPixelLayout::BPP32, convertRGBA8888PMToARGB32PM, convertRGBA8888PMFromARGB32PM, convertRGBXFromRGB32, convertRGBA8888PMToARGB64PM}, // Format_RGBA8888_Premultiplied
#else
    { 8,  0, 8,  8, 8, 16, 0, 24, false, QPixelLayout::BPP32, convertRGBA8888PMToARGB32PM, convertRGBXFromARGB32PM, convertRGBXFromRGB32, convertRGBA8888PMToARGB64PM }, // Format_RGBX8888
    { 8,  0, 8,  8, 8, 16, 8, 24, false, QPixelLayout::BPP32, convertRGBA8888ToARGB32PM, convertRGBA8888FromARGB32PM, convertRGBXFromRGB32, convertRGBA8888ToARGB64PM }, // Format_RGBA8888 (ABGR32)
    { 8,  0, 8,  8, 8, 16, 8, 24,  true, QPixelLayout::BPP32, convertRGBA8888PMToARGB32PM, convertRGBA8888PMFromARGB32PM, convertRGBXFromRGB32, convertRGBA8888PMToARGB64PM },  // Format_RGBA8888_Premultiplied
#endif
    { 10,  20, 10,  10, 10, 0, 0, 30, false, QPixelLayout::BPP32, convertA2RGB30PMToARGB32PM<PixelOrderBGR>, convertRGB30FromARGB32PM<PixelOrderBGR>, convertRGB30FromRGB32<PixelOrderBGR>, convertA2RGB30PMToARGB64PM<PixelOrderBGR> }, // Format_BGR30
    { 10,  20, 10,  10, 10, 0, 2, 30,  true, QPixelLayout::BPP32, convertA2RGB30PMToARGB32PM<PixelOrderBGR>, convertA2RGB30PMFromARGB32PM<PixelOrderBGR>, convertRGB30FromRGB32<PixelOrderBGR>, convertA2RGB30PMToARGB64PM<PixelOrderBGR> },  // Format_A2BGR30_Premultiplied
    { 10,  0, 10,  10, 10, 20, 0, 30, false, QPixelLayout::BPP32, convertA2RGB30PMToARGB32PM<PixelOrderRGB>, convertRGB30FromARGB32PM<PixelOrderRGB>, convertRGB30FromRGB32<PixelOrderRGB>, convertA2RGB30PMToARGB64PM<PixelOrderRGB> }, // Format_RGB30
    { 10,  0, 10,  10, 10, 20, 2, 30,  true, QPixelLayout::BPP32, convertA2RGB30PMToARGB32PM<PixelOrderRGB>, convertA2RGB30PMFromARGB32PM<PixelOrderRGB>, convertRGB30FromRGB32<PixelOrderRGB>, convertA2RGB30PMToARGB64PM<PixelOrderRGB> },  // Format_A2RGB30_Premultiplied
    { 0, 0,  0, 0,  0, 0,  8, 0, true, QPixelLayout::BPP8, convertAlpha8ToRGB32, convertAlpha8FromARGB32PM, 0, convertAlpha8ToRGB64 }, // Format_Alpha8
    { 0, 0,  0, 0,  0, 0,  0, 0, false, QPixelLayout::BPP8, convertGrayscale8ToRGB32, convertGrayscale8FromARGB32PM, convertGrayscale8FromRGB32, convertGrayscale8ToRGB64 } // Format_Grayscale8
};

const FetchPixelsFunc qFetchPixels[QPixelLayout::BPPCount] = {
    0, // BPPNone
    fetchPixels<QPixelLayout::BPP1MSB>, // BPP1MSB
    fetchPixels<QPixelLayout::BPP1LSB>, // BPP1LSB
    fetchPixels<QPixelLayout::BPP8>, // BPP8
    fetchPixels<QPixelLayout::BPP16>, // BPP16
    fetchPixels<QPixelLayout::BPP24>, // BPP24
    fetchPixels<QPixelLayout::BPP32> // BPP32
};

StorePixelsFunc qStorePixels[QPixelLayout::BPPCount] = {
    0, // BPPNone
    storePixels<QPixelLayout::BPP1MSB>, // BPP1MSB
    storePixels<QPixelLayout::BPP1LSB>, // BPP1LSB
    storePixels<QPixelLayout::BPP8>, // BPP8
    storePixels<QPixelLayout::BPP16>, // BPP16
    storePixels<QPixelLayout::BPP24>, // BPP24
    storePixels<QPixelLayout::BPP32> // BPP32
};

typedef uint (QT_FASTCALL *FetchPixelFunc)(const uchar *src, int index);

static const FetchPixelFunc qFetchPixel[QPixelLayout::BPPCount] = {
    0, // BPPNone
    fetchPixel<QPixelLayout::BPP1MSB>, // BPP1MSB
    fetchPixel<QPixelLayout::BPP1LSB>, // BPP1LSB
    fetchPixel<QPixelLayout::BPP8>, // BPP8
    fetchPixel<QPixelLayout::BPP16>, // BPP16
    fetchPixel<QPixelLayout::BPP24>, // BPP24
    fetchPixel<QPixelLayout::BPP32> // BPP32
};

/*
  Destination fetch. This is simple as we don't have to do bounds checks or
  transformations
*/

static uint * QT_FASTCALL destFetchMono(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    uchar *Q_DECL_RESTRICT data = (uchar *)rasterBuffer->scanLine(y);
    uint *start = buffer;
    const uint *end = buffer + length;
    while (buffer < end) {
        *buffer = data[x>>3] & (0x80 >> (x & 7)) ? rasterBuffer->destColor1 : rasterBuffer->destColor0;
        ++buffer;
        ++x;
    }
    return start;
}

static uint * QT_FASTCALL destFetchMonoLsb(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    uchar *Q_DECL_RESTRICT data = (uchar *)rasterBuffer->scanLine(y);
    uint *start = buffer;
    const uint *end = buffer + length;
    while (buffer < end) {
        *buffer = data[x>>3] & (0x1 << (x & 7)) ? rasterBuffer->destColor1 : rasterBuffer->destColor0;
        ++buffer;
        ++x;
    }
    return start;
}

static uint * QT_FASTCALL destFetchARGB32P(uint *, QRasterBuffer *rasterBuffer, int x, int y, int)
{
    return (uint *)rasterBuffer->scanLine(y) + x;
}

static uint * QT_FASTCALL destFetchRGB16(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const ushort *Q_DECL_RESTRICT data = (const ushort *)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i)
        buffer[i] = qConvertRgb16To32(data[i]);
    return buffer;
}

static uint *QT_FASTCALL destFetch(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[rasterBuffer->format];
    const uint *ptr = qFetchPixels[layout->bpp](buffer, rasterBuffer->scanLine(y), x, length);
    return const_cast<uint *>(layout->convertToARGB32PM(buffer, ptr, length, 0, 0));
}

static QRgba64 *QT_FASTCALL destFetch64(QRgba64 *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[rasterBuffer->format];
    uint buffer32[buffer_size];
    const uint *ptr = qFetchPixels[layout->bpp](buffer32, rasterBuffer->scanLine(y), x, length);
    return const_cast<QRgba64 *>(layout->convertToARGB64PM(buffer, ptr, length, 0, 0));
}

static QRgba64 *QT_FASTCALL destFetch64uint32(QRgba64 *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[rasterBuffer->format];
    const uint *src = ((const uint *)rasterBuffer->scanLine(y)) + x;
    return const_cast<QRgba64 *>(layout->convertToARGB64PM(buffer, src, length, 0, 0));
}

static QRgba64 * QT_FASTCALL destFetch64Undefined(QRgba64 *buffer, QRasterBuffer *, int, int, int)
{
    return buffer;
}

static DestFetchProc destFetchProc[QImage::NImageFormats] =
{
    0,                  // Format_Invalid
    destFetchMono,      // Format_Mono,
    destFetchMonoLsb,   // Format_MonoLSB
    0,                  // Format_Indexed8
    destFetchARGB32P,   // Format_RGB32
    destFetch,          // Format_ARGB32,
    destFetchARGB32P,   // Format_ARGB32_Premultiplied
    destFetchRGB16,     // Format_RGB16
    destFetch,          // Format_ARGB8565_Premultiplied
    destFetch,          // Format_RGB666
    destFetch,          // Format_ARGB6666_Premultiplied
    destFetch,          // Format_RGB555
    destFetch,          // Format_ARGB8555_Premultiplied
    destFetch,          // Format_RGB888
    destFetch,          // Format_RGB444
    destFetch,          // Format_ARGB4444_Premultiplied
    destFetch,          // Format_RGBX8888
    destFetch,          // Format_RGBA8888
    destFetch,          // Format_RGBA8888_Premultiplied
    destFetch,          // Format_BGR30
    destFetch,          // Format_A2BGR30_Premultiplied
    destFetch,          // Format_RGB30
    destFetch,          // Format_A2RGB30_Premultiplied
    destFetch,          // Format_Alpha8
    destFetch,          // Format_Grayscale8
};

static DestFetchProc64 destFetchProc64[QImage::NImageFormats] =
{
    0,                  // Format_Invalid
    0,                  // Format_Mono,
    0,                  // Format_MonoLSB
    0,                  // Format_Indexed8
    destFetch64uint32,  // Format_RGB32
    destFetch64uint32,  // Format_ARGB32,
    destFetch64uint32,  // Format_ARGB32_Premultiplied
    destFetch64,        // Format_RGB16
    destFetch64,        // Format_ARGB8565_Premultiplied
    destFetch64,        // Format_RGB666
    destFetch64,        // Format_ARGB6666_Premultiplied
    destFetch64,        // Format_RGB555
    destFetch64,        // Format_ARGB8555_Premultiplied
    destFetch64,        // Format_RGB888
    destFetch64,        // Format_RGB444
    destFetch64,        // Format_ARGB4444_Premultiplied
    destFetch64uint32,  // Format_RGBX8888
    destFetch64uint32,  // Format_RGBA8888
    destFetch64uint32,  // Format_RGBA8888_Premultiplied
    destFetch64uint32,  // Format_BGR30
    destFetch64uint32,  // Format_A2BGR30_Premultiplied
    destFetch64uint32,  // Format_RGB30
    destFetch64uint32,  // Format_A2RGB30_Premultiplied
    destFetch64,        // Format_Alpha8
    destFetch64,        // Format_Grayscale8
};

/*
   Returns the color in the mono destination color table
   that is the "nearest" to /color/.
*/
static inline QRgb findNearestColor(QRgb color, QRasterBuffer *rbuf)
{
    QRgb color_0 = qPremultiply(rbuf->destColor0);
    QRgb color_1 = qPremultiply(rbuf->destColor1);
    color = qPremultiply(color);

    int r = qRed(color);
    int g = qGreen(color);
    int b = qBlue(color);
    int rx, gx, bx;
    int dist_0, dist_1;

    rx = r - qRed(color_0);
    gx = g - qGreen(color_0);
    bx = b - qBlue(color_0);
    dist_0 = rx*rx + gx*gx + bx*bx;

    rx = r - qRed(color_1);
    gx = g - qGreen(color_1);
    bx = b - qBlue(color_1);
    dist_1 = rx*rx + gx*gx + bx*bx;

    if (dist_0 < dist_1)
        return color_0;
    return color_1;
}

/*
  Destination store.
*/

static void QT_FASTCALL destStoreMono(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uchar *Q_DECL_RESTRICT data = (uchar *)rasterBuffer->scanLine(y);
    if (rasterBuffer->monoDestinationWithClut) {
        for (int i = 0; i < length; ++i) {
            if (buffer[i] == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(0x80 >> (x & 7));
            } else if (buffer[i] == rasterBuffer->destColor1) {
                data[x >> 3] |= 0x80 >> (x & 7);
            } else if (findNearestColor(buffer[i], rasterBuffer) == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(0x80 >> (x & 7));
            } else {
                data[x >> 3] |= 0x80 >> (x & 7);
            }
            ++x;
        }
    } else {
        for (int i = 0; i < length; ++i) {
            if (qGray(buffer[i]) < int(qt_bayer_matrix[y & 15][x & 15]))
                data[x >> 3] |= 0x80 >> (x & 7);
            else
                data[x >> 3] &= ~(0x80 >> (x & 7));
            ++x;
        }
    }
}

static void QT_FASTCALL destStoreMonoLsb(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uchar *Q_DECL_RESTRICT data = (uchar *)rasterBuffer->scanLine(y);
    if (rasterBuffer->monoDestinationWithClut) {
        for (int i = 0; i < length; ++i) {
            if (buffer[i] == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(1 << (x & 7));
            } else if (buffer[i] == rasterBuffer->destColor1) {
                data[x >> 3] |= 1 << (x & 7);
            } else if (findNearestColor(buffer[i], rasterBuffer) == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(1 << (x & 7));
            } else {
                data[x >> 3] |= 1 << (x & 7);
            }
            ++x;
        }
    } else {
        for (int i = 0; i < length; ++i) {
            if (qGray(buffer[i]) < int(qt_bayer_matrix[y & 15][x & 15]))
                data[x >> 3] |= 1 << (x & 7);
            else
                data[x >> 3] &= ~(1 << (x & 7));
            ++x;
        }
    }
}

static void QT_FASTCALL destStoreRGB16(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    quint16 *data = (quint16*)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i)
        data[i] = qConvertRgb32To16(buffer[i]);
}

static void QT_FASTCALL destStore(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uint buf[buffer_size];
    const QPixelLayout *layout = &qPixelLayouts[rasterBuffer->format];
    StorePixelsFunc store = qStorePixels[layout->bpp];
    uchar *dest = rasterBuffer->scanLine(y);
    while (length) {
        int l = qMin(length, buffer_size);
        const uint *ptr = 0;
        if (!layout->premultiplied && !layout->alphaWidth)
            ptr = layout->convertFromRGB32(buf, buffer, l, 0, 0);
        else
            ptr = layout->convertFromARGB32PM(buf, buffer, l, 0, 0);
        store(dest, ptr, x, l);
        length -= l;
        buffer += l;
        x += l;
    }
}

static void QT_FASTCALL convertFromRgb64(uint *dest, const QRgba64 *src, int length)
{
    for (int i = 0; i < length; ++i) {
        dest[i] = toArgb32(src[i]);
    }
}

static void QT_FASTCALL destStore64(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length)
{
    uint buf[buffer_size];
    const QPixelLayout *layout = &qPixelLayouts[rasterBuffer->format];
    StorePixelsFunc store = qStorePixels[layout->bpp];
    uchar *dest = rasterBuffer->scanLine(y);
    while (length) {
        int l = qMin(length, buffer_size);
        const uint *ptr = 0;
        convertFromRgb64(buf, buffer, l);
        if (!layout->premultiplied && !layout->alphaWidth)
            ptr = layout->convertFromRGB32(buf, buf, l, 0, 0);
        else
            ptr = layout->convertFromARGB32PM(buf, buf, l, 0, 0);
        store(dest, ptr, x, l);
        length -= l;
        buffer += l;
        x += l;
    }
}

#ifdef __SSE2__
template<QtPixelOrder PixelOrder>
static inline void qConvertARGB64PMToA2RGB30PM_sse2(uint *dest, const QRgba64 *buffer, int count)
{
    const __m128i gmask = _mm_set1_epi32(0x000ffc00);
    const __m128i cmask = _mm_set1_epi32(0x000003ff);
    int i = 0;
    __m128i vr, vg, vb, va;
    for (; i < count && uintptr_t(buffer) & 0xF; ++i) {
        *dest++ = qConvertRgb64ToRgb30<PixelOrder>(*buffer++);
    }

    for (; i < count-15; i += 16) {
        // Repremultiplying is really expensive and hard to do in SIMD without AVX2,
        // so we try to avoid it by checking if it is needed 16 samples at a time.
        __m128i vOr = _mm_set1_epi32(0);
        __m128i vAnd = _mm_set1_epi32(0xffffffff);
        for (int j = 0; j < 16; j += 2) {
            __m128i vs = _mm_load_si128((const __m128i*)(buffer + j));
            vOr = _mm_or_si128(vOr, vs);
            vAnd = _mm_and_si128(vAnd, vs);
        }
        const quint16 orAlpha = ((uint)_mm_extract_epi16(vOr, 3)) | ((uint)_mm_extract_epi16(vOr, 7));
        const quint16 andAlpha = ((uint)_mm_extract_epi16(vAnd, 3)) & ((uint)_mm_extract_epi16(vAnd, 7));

        if (andAlpha == 0xffff) {
            for (int j = 0; j < 16; j += 2) {
                __m128i vs = _mm_load_si128((const __m128i*)buffer);
                buffer += 2;
                vr = _mm_srli_epi64(vs, 6);
                vg = _mm_srli_epi64(vs, 16 + 6 - 10);
                vb = _mm_srli_epi64(vs, 32 + 6);
                vr = _mm_and_si128(vr, cmask);
                vg = _mm_and_si128(vg, gmask);
                vb = _mm_and_si128(vb, cmask);
                va = _mm_srli_epi64(vs, 48 + 14);
                if (PixelOrder == PixelOrderRGB)
                    vr = _mm_slli_epi32(vr, 20);
                else
                    vb = _mm_slli_epi32(vb, 20);
                va = _mm_slli_epi32(va, 30);
                __m128i vd = _mm_or_si128(_mm_or_si128(vr, vg), _mm_or_si128(vb, va));
                vd = _mm_shuffle_epi32(vd, _MM_SHUFFLE(3, 1, 2, 0));
                _mm_storel_epi64((__m128i*)dest, vd);
                dest += 2;
            }
        } else if (orAlpha == 0) {
            for (int j = 0; j < 16; ++j) {
                *dest++ = 0;
                buffer++;
            }
        } else {
            for (int j = 0; j < 16; ++j)
                *dest++ = qConvertRgb64ToRgb30<PixelOrder>(*buffer++);
        }
    }

    SIMD_EPILOGUE(i, count, 15)
        *dest++ = qConvertRgb64ToRgb30<PixelOrder>(*buffer++);
}
#endif

static void QT_FASTCALL destStore64ARGB32(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length)
{
    uint *dest = (uint*)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i) {
        dest[i] = toArgb32(buffer[i].unpremultiplied());
    }
}

static void QT_FASTCALL destStore64RGBA8888(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length)
{
    uint *dest = (uint*)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i) {
        dest[i] = toRgba8888(buffer[i].unpremultiplied());
    }
}

template<QtPixelOrder PixelOrder>
static void QT_FASTCALL destStore64RGB30(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length)
{
    uint *dest = (uint*)rasterBuffer->scanLine(y) + x;
#ifdef __SSE2__
    qConvertARGB64PMToA2RGB30PM_sse2<PixelOrder>(dest, buffer, length);
#else
    for (int i = 0; i < length; ++i) {
        dest[i] = qConvertRgb64ToRgb30<PixelOrder>(buffer[i]);
    }
#endif
}

static DestStoreProc destStoreProc[QImage::NImageFormats] =
{
    0,                  // Format_Invalid
    destStoreMono,      // Format_Mono,
    destStoreMonoLsb,   // Format_MonoLSB
    0,                  // Format_Indexed8
    0,                  // Format_RGB32
    destStore,          // Format_ARGB32,
    0,                  // Format_ARGB32_Premultiplied
    destStoreRGB16,     // Format_RGB16
    destStore,          // Format_ARGB8565_Premultiplied
    destStore,          // Format_RGB666
    destStore,          // Format_ARGB6666_Premultiplied
    destStore,          // Format_RGB555
    destStore,          // Format_ARGB8555_Premultiplied
    destStore,          // Format_RGB888
    destStore,          // Format_RGB444
    destStore,          // Format_ARGB4444_Premultiplied
    destStore,          // Format_RGBX8888
    destStore,          // Format_RGBA8888
    destStore,          // Format_RGBA8888_Premultiplied
    destStore,          // Format_BGR30
    destStore,          // Format_A2BGR30_Premultiplied
    destStore,          // Format_RGB30
    destStore,          // Format_A2RGB30_Premultiplied
    destStore,          // Format_Alpha8
    destStore,          // Format_Grayscale8
};

static DestStoreProc64 destStoreProc64[QImage::NImageFormats] =
{
    0,                  // Format_Invalid
    destStore64,        // Format_Mono,
    destStore64,        // Format_MonoLSB
    0,                  // Format_Indexed8
    destStore64,        // Format_RGB32
    destStore64ARGB32,  // Format_ARGB32,
    destStore64,        // Format_ARGB32_Premultiplied
    destStore64,        // Format_RGB16
    destStore64,        // Format_ARGB8565_Premultiplied
    destStore64,        // Format_RGB666
    destStore64,        // Format_ARGB6666_Premultiplied
    destStore64,        // Format_RGB555
    destStore64,        // Format_ARGB8555_Premultiplied
    destStore64,        // Format_RGB888
    destStore64,        // Format_RGB444
    destStore64,        // Format_ARGB4444_Premultiplied
    destStore64,        // Format_RGBX8888
    destStore64RGBA8888,        // Format_RGBA8888
    destStore64,        // Format_RGBA8888_Premultiplied
    destStore64RGB30<PixelOrderBGR>,        // Format_BGR30
    destStore64RGB30<PixelOrderBGR>,        // Format_A2BGR30_Premultiplied
    destStore64RGB30<PixelOrderRGB>,        // Format_RGB30
    destStore64RGB30<PixelOrderRGB>,        // Format_A2RGB30_Premultiplied
    destStore64,        // Format_Alpha8
    destStore64,        // Format_Grayscale8
};

/*
  Source fetches

  This is a bit more complicated, as we need several fetch routines for every surface type

  We need 5 fetch methods per surface type:
  untransformed
  transformed (tiled and not tiled)
  transformed bilinear (tiled and not tiled)

  We don't need bounds checks for untransformed, but we need them for the other ones.

  The generic implementation does pixel by pixel fetches
*/

enum TextureBlendType {
    BlendUntransformed,
    BlendTiled,
    BlendTransformed,
    BlendTransformedTiled,
    BlendTransformedBilinear,
    BlendTransformedBilinearTiled,
    NBlendTypes
};

static const uint *QT_FASTCALL fetchUntransformed(uint *buffer, const Operator *,
                                                  const QSpanData *data, int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    const uint *ptr = qFetchPixels[layout->bpp](buffer, data->texture.scanLine(y), x, length);
    return layout->convertToARGB32PM(buffer, ptr, length, data->texture.colorTable, 0);
}

static const uint *QT_FASTCALL fetchUntransformedARGB32PM(uint *, const Operator *,
                                                          const QSpanData *data, int y, int x, int)
{
    const uchar *scanLine = data->texture.scanLine(y);
    return ((const uint *)scanLine) + x;
}

static const uint *QT_FASTCALL fetchUntransformedRGB16(uint *buffer, const Operator *,
                                                       const QSpanData *data, int y, int x,
                                                       int length)
{
    const quint16 *scanLine = (const quint16 *)data->texture.scanLine(y) + x;
    for (int i = 0; i < length; ++i)
        buffer[i] = qConvertRgb16To32(scanLine[i]);
    return buffer;
}

static const QRgba64 *QT_FASTCALL fetchUntransformed64(QRgba64 *buffer, const Operator *,
                                                       const QSpanData *data, int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    if (layout->bpp != QPixelLayout::BPP32) {
        uint buffer32[buffer_size];
        const uint *ptr = qFetchPixels[layout->bpp](buffer32, data->texture.scanLine(y), x, length);
        return layout->convertToARGB64PM(buffer, ptr, length, data->texture.colorTable, 0);
    } else {
        const uint *src = (const uint *)data->texture.scanLine(y) + x;
        return layout->convertToARGB64PM(buffer, src, length, data->texture.colorTable, 0);
    }
}

template<TextureBlendType blendType, QPixelLayout::BPP bpp>
static const uint *QT_FASTCALL fetchTransformed(uint *buffer, const Operator *, const QSpanData *data,
                                         int y, int x, int length)
{
    Q_STATIC_ASSERT(blendType == BlendTransformed || blendType == BlendTransformedTiled);
    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    const int image_x1 = data->texture.x1;
    const int image_y1 = data->texture.y1;
    const int image_x2 = data->texture.x2 - 1;
    const int image_y2 = data->texture.y2 - 1;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    if (bpp != QPixelLayout::BPPNone) // Like this to not ICE on GCC 5.3.1
        Q_ASSERT(layout->bpp == bpp);
    // When templated 'fetch' should be inlined at compile time:
    const FetchPixelFunc fetch = (bpp == QPixelLayout::BPPNone) ? qFetchPixel[layout->bpp] : FetchPixelFunc(fetchPixel<bpp>);

    uint *const end = buffer + length;
    uint *b = buffer;
    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy
                      + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy
                      + data->m12 * cx + data->dy) * fixed_scale);

        while (b < end) {
            int px = fx >> 16;
            int py = fy >> 16;

            if (blendType == BlendTransformedTiled) {
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;
            } else {
                px = qBound(image_x1, px, image_x2);
                py = qBound(image_y1, py, image_y2);
            }
            *b = fetch(data->texture.scanLine(py), px);

            fx += fdx;
            fy += fdy;
            ++b;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        while (b < end) {
            const qreal iw = fw == 0 ? 1 : 1 / fw;
            const qreal tx = fx * iw;
            const qreal ty = fy * iw;
            int px = int(tx) - (tx < 0);
            int py = int(ty) - (ty < 0);

            if (blendType == BlendTransformedTiled) {
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;
            } else {
                px = qBound(image_x1, px, image_x2);
                py = qBound(image_y1, py, image_y2);
            }
            *b = fetch(data->texture.scanLine(py), px);

            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }
    return layout->convertToARGB32PM(buffer, buffer, length, data->texture.colorTable, 0);
}

template<TextureBlendType blendType>  /* either BlendTransformed or BlendTransformedTiled */
static const QRgba64 *QT_FASTCALL fetchTransformed64(QRgba64 *buffer, const Operator *, const QSpanData *data,
                                                     int y, int x, int length)
{
    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    const int image_x1 = data->texture.x1;
    const int image_y1 = data->texture.y1;
    const int image_x2 = data->texture.x2 - 1;
    const int image_y2 = data->texture.y2 - 1;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    FetchPixelFunc fetch = qFetchPixel[layout->bpp];
    const QVector<QRgb> *clut = data->texture.colorTable;

    uint buffer32[buffer_size];
    QRgba64 *b = buffer;
    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy
                      + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy
                      + data->m12 * cx + data->dy) * fixed_scale);

        int i = 0,  j = 0;
        while (i < length) {
            if (j == buffer_size) {
                layout->convertToARGB64PM(b, buffer32, buffer_size, clut, 0);
                b += buffer_size;
                j = 0;
            }
            int px = fx >> 16;
            int py = fy >> 16;

            if (blendType == BlendTransformedTiled) {
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;
            } else {
                px = qBound(image_x1, px, image_x2);
                py = qBound(image_y1, py, image_y2);
            }
            buffer32[j] = fetch(data->texture.scanLine(py), px);

            fx += fdx;
            fy += fdy;
            ++i; ++j;
        }
        if (j > 0) {
            layout->convertToARGB64PM(b, buffer32, j, clut, 0);
            b += j;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        int i = 0,  j = 0;
        while (i < length) {
            if (j == buffer_size) {
                layout->convertToARGB64PM(b, buffer32, buffer_size, clut, 0);
                b += buffer_size;
                j = 0;
            }
            const qreal iw = fw == 0 ? 1 : 1 / fw;
            const qreal tx = fx * iw;
            const qreal ty = fy * iw;
            int px = int(tx) - (tx < 0);
            int py = int(ty) - (ty < 0);

            if (blendType == BlendTransformedTiled) {
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;
            } else {
                px = qBound(image_x1, px, image_x2);
                py = qBound(image_y1, py, image_y2);
            }
            buffer32[j] = fetch(data->texture.scanLine(py), px);

            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++i; ++j;
        }
        if (j > 0) {
            layout->convertToARGB64PM(b, buffer32, j, clut, 0);
            b += j;
        }
    }
    return buffer;
}

/** \internal
  interpolate 4 argb pixels with the distx and disty factor.
  distx and disty must be between 0 and 16
 */
static inline uint interpolate_4_pixels_16(uint tl, uint tr, uint bl, uint br, uint distx, uint disty)
{
    uint distxy = distx * disty;
    //idistx * disty = (16-distx) * disty = 16*disty - distxy
    //idistx * idisty = (16-distx) * (16-disty) = 16*16 - 16*distx -16*disty + distxy
    uint tlrb = (tl & 0x00ff00ff)         * (16*16 - 16*distx - 16*disty + distxy);
    uint tlag = ((tl & 0xff00ff00) >> 8)  * (16*16 - 16*distx - 16*disty + distxy);
    uint trrb = ((tr & 0x00ff00ff)        * (distx*16 - distxy));
    uint trag = (((tr & 0xff00ff00) >> 8) * (distx*16 - distxy));
    uint blrb = ((bl & 0x00ff00ff)        * (disty*16 - distxy));
    uint blag = (((bl & 0xff00ff00) >> 8) * (disty*16 - distxy));
    uint brrb = ((br & 0x00ff00ff)        * (distxy));
    uint brag = (((br & 0xff00ff00) >> 8) * (distxy));
    return (((tlrb + trrb + blrb + brrb) >> 8) & 0x00ff00ff) | ((tlag + trag + blag + brag) & 0xff00ff00);
}

#if defined(__SSE2__)
#define interpolate_4_pixels_16_sse2(tl, tr, bl, br, distx, disty, colorMask, v_256, b)  \
{ \
    const __m128i dxdy = _mm_mullo_epi16 (distx, disty); \
    const __m128i distx_ = _mm_slli_epi16(distx, 4); \
    const __m128i disty_ = _mm_slli_epi16(disty, 4); \
    const __m128i idxidy =  _mm_add_epi16(dxdy, _mm_sub_epi16(v_256, _mm_add_epi16(distx_, disty_))); \
    const __m128i dxidy =  _mm_sub_epi16(distx_, dxdy); \
    const __m128i idxdy =  _mm_sub_epi16(disty_, dxdy); \
 \
    __m128i tlAG = _mm_srli_epi16(tl, 8); \
    __m128i tlRB = _mm_and_si128(tl, colorMask); \
    __m128i trAG = _mm_srli_epi16(tr, 8); \
    __m128i trRB = _mm_and_si128(tr, colorMask); \
    __m128i blAG = _mm_srli_epi16(bl, 8); \
    __m128i blRB = _mm_and_si128(bl, colorMask); \
    __m128i brAG = _mm_srli_epi16(br, 8); \
    __m128i brRB = _mm_and_si128(br, colorMask); \
 \
    tlAG = _mm_mullo_epi16(tlAG, idxidy); \
    tlRB = _mm_mullo_epi16(tlRB, idxidy); \
    trAG = _mm_mullo_epi16(trAG, dxidy); \
    trRB = _mm_mullo_epi16(trRB, dxidy); \
    blAG = _mm_mullo_epi16(blAG, idxdy); \
    blRB = _mm_mullo_epi16(blRB, idxdy); \
    brAG = _mm_mullo_epi16(brAG, dxdy); \
    brRB = _mm_mullo_epi16(brRB, dxdy); \
 \
    /* Add the values, and shift to only keep 8 significant bits per colors */ \
    __m128i rAG =_mm_add_epi16(_mm_add_epi16(tlAG, trAG), _mm_add_epi16(blAG, brAG)); \
    __m128i rRB =_mm_add_epi16(_mm_add_epi16(tlRB, trRB), _mm_add_epi16(blRB, brRB)); \
    rAG = _mm_andnot_si128(colorMask, rAG); \
    rRB = _mm_srli_epi16(rRB, 8); \
    _mm_storeu_si128((__m128i*)(b), _mm_or_si128(rAG, rRB)); \
}
#endif

#if defined(__ARM_NEON__)
#define interpolate_4_pixels_16_neon(tl, tr, bl, br, distx, disty, disty_, colorMask, invColorMask, v_256, b)  \
{ \
    const int16x8_t dxdy = vmulq_s16(distx, disty); \
    const int16x8_t distx_ = vshlq_n_s16(distx, 4); \
    const int16x8_t idxidy =  vaddq_s16(dxdy, vsubq_s16(v_256, vaddq_s16(distx_, disty_))); \
    const int16x8_t dxidy =  vsubq_s16(distx_, dxdy); \
    const int16x8_t idxdy =  vsubq_s16(disty_, dxdy); \
 \
    int16x8_t tlAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(tl), 8)); \
    int16x8_t tlRB = vandq_s16(tl, colorMask); \
    int16x8_t trAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(tr), 8)); \
    int16x8_t trRB = vandq_s16(tr, colorMask); \
    int16x8_t blAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(bl), 8)); \
    int16x8_t blRB = vandq_s16(bl, colorMask); \
    int16x8_t brAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(br), 8)); \
    int16x8_t brRB = vandq_s16(br, colorMask); \
 \
    int16x8_t rAG = vmulq_s16(tlAG, idxidy); \
    int16x8_t rRB = vmulq_s16(tlRB, idxidy); \
    rAG = vmlaq_s16(rAG, trAG, dxidy); \
    rRB = vmlaq_s16(rRB, trRB, dxidy); \
    rAG = vmlaq_s16(rAG, blAG, idxdy); \
    rRB = vmlaq_s16(rRB, blRB, idxdy); \
    rAG = vmlaq_s16(rAG, brAG, dxdy); \
    rRB = vmlaq_s16(rRB, brRB, dxdy); \
 \
    rAG = vandq_s16(invColorMask, rAG); \
    rRB = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(rRB), 8)); \
    vst1q_s16((int16_t*)(b), vorrq_s16(rAG, rRB)); \
}
#endif

#if defined(__SSE2__)
static inline QRgba64 interpolate_4_pixels_rgb64(QRgba64 t[], QRgba64 b[], uint distx, uint disty)
{
    __m128i vt = _mm_loadu_si128((const __m128i*)t);
    if (disty) {
       __m128i vb = _mm_loadu_si128((const __m128i*)b);
        vt = _mm_mulhi_epu16(vt, _mm_set1_epi16(0x10000 - disty));
        vb = _mm_mulhi_epu16(vb, _mm_set1_epi16(disty));
        vt = _mm_add_epi16(vt, vb);
    }
    if (distx) {
        const __m128i vdistx = _mm_shufflelo_epi16(_mm_cvtsi32_si128(distx), _MM_SHUFFLE(0, 0, 0, 0));
        const __m128i vidistx = _mm_shufflelo_epi16(_mm_cvtsi32_si128(0x10000 - distx), _MM_SHUFFLE(0, 0, 0, 0));
        vt = _mm_mulhi_epu16(vt, _mm_unpacklo_epi64(vidistx, vdistx));
        vt = _mm_add_epi16(vt, _mm_srli_si128(vt, 8));
    }
#ifdef Q_PROCESSOR_X86_64
    return QRgba64::fromRgba64(_mm_cvtsi128_si64(vt));
#else
    QRgba64 out;
    _mm_storel_epi64((__m128i*)&out, vt);
    return out;
#endif
}
#else
static inline QRgba64 interpolate_4_pixels_rgb64(QRgba64 t[], QRgba64 b[], uint distx, uint disty)
{
    const uint dx = distx>>8;
    const uint dy = disty>>8;
    const uint idx = 256 - dx;
    const uint idy = 256 - dy;
    QRgba64 xtop = interpolate256(t[0], idx, t[1], dx);
    QRgba64 xbot = interpolate256(b[0], idx, b[1], dx);
    return interpolate256(xtop, idy, xbot, dy);
}
#endif

template<TextureBlendType blendType>
void fetchTransformedBilinear_pixelBounds(int max, int l1, int l2, int &v1, int &v2);

template<>
inline void fetchTransformedBilinear_pixelBounds<BlendTransformedBilinearTiled>(int max, int, int, int &v1, int &v2)
{
    v1 %= max;
    if (v1 < 0)
        v1 += max;
    v2 = v1 + 1;
    if (v2 == max)
        v2 = 0;
    Q_ASSERT(v1 >= 0 && v1 < max);
    Q_ASSERT(v2 >= 0 && v2 < max);
}

template<>
inline void fetchTransformedBilinear_pixelBounds<BlendTransformedBilinear>(int, int l1, int l2, int &v1, int &v2)
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

enum FastTransformTypes {
    SimpleUpscaleTransform,
    UpscaleTransform,
    DownscaleTransform,
    RotateTransform,
    FastRotateTransform,
    NFastTransformTypes
};

typedef void (QT_FASTCALL *BilinearFastTransformHelper)(uint *b, uint *end, const QTextureData &image, int &fx, int &fy, int fdx, int fdy);

template<TextureBlendType blendType>
static void QT_FASTCALL fetchTransformedBilinearARGB32PM_simple_upscale_helper(uint *b, uint *end, const QTextureData &image,
                                                                               int &fx, int &fy, int fdx, int /*fdy*/)
{
    int y1 = (fy >> 16);
    int y2;
    fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);
    const uint *s1 = (const uint *)image.scanLine(y1);
    const uint *s2 = (const uint *)image.scanLine(y2);

    const int disty = (fy & 0x0000ffff) >> 8;
    const int idisty = 256 - disty;
    const int length = end - b;

    // The intermediate buffer is generated in the positive direction
    const int adjust = (fdx < 0) ? fdx * length : 0;
    const int offset = (fx + adjust) >> 16;
    int x = offset;

    // The idea is first to do the interpolation between the row s1 and the row s2
    // into an intermediate buffer, then we interpolate between two pixel of this buffer.

    // intermediate_buffer[0] is a buffer of red-blue component of the pixel, in the form 0x00RR00BB
    // intermediate_buffer[1] is the alpha-green component of the pixel, in the form 0x00AA00GG
    // +1 for the last pixel to interpolate with, and +1 for rounding errors.
    quint32 intermediate_buffer[2][buffer_size + 2];
    // count is the size used in the intermediate_buffer.
    int count = (qint64(length) * qAbs(fdx) + fixed_scale - 1) / fixed_scale + 2;
    Q_ASSERT(count <= buffer_size + 2); //length is supposed to be <= buffer_size and data->m11 < 1 in this case
    int f = 0;
    int lim = count;
    if (blendType == BlendTransformedBilinearTiled) {
        x %= image.width;
        if (x < 0) x += image.width;
    } else {
        lim = qMin(count, image.x2 - x);
        if (x < image.x1) {
            Q_ASSERT(x < image.x2);
            uint t = s1[image.x1];
            uint b = s2[image.x1];
            quint32 rb = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
            quint32 ag = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
            do {
                intermediate_buffer[0][f] = rb;
                intermediate_buffer[1][f] = ag;
                f++;
                x++;
            } while (x < image.x1 && f < lim);
        }
    }

    if (blendType != BlendTransformedBilinearTiled) {
#if defined(__SSE2__)
        const __m128i disty_ = _mm_set1_epi16(disty);
        const __m128i idisty_ = _mm_set1_epi16(idisty);
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);

        lim -= 3;
        for (; f < lim; x += 4, f += 4) {
            // Load 4 pixels from s1, and split the alpha-green and red-blue component
            __m128i top = _mm_loadu_si128((const __m128i*)((const uint *)(s1)+x));
            __m128i topAG = _mm_srli_epi16(top, 8);
            __m128i topRB = _mm_and_si128(top, colorMask);
            // Multiplies each color component by idisty
            topAG = _mm_mullo_epi16 (topAG, idisty_);
            topRB = _mm_mullo_epi16 (topRB, idisty_);

            // Same for the s2 vector
            __m128i bottom = _mm_loadu_si128((const __m128i*)((const uint *)(s2)+x));
            __m128i bottomAG = _mm_srli_epi16(bottom, 8);
            __m128i bottomRB = _mm_and_si128(bottom, colorMask);
            bottomAG = _mm_mullo_epi16 (bottomAG, disty_);
            bottomRB = _mm_mullo_epi16 (bottomRB, disty_);

            // Add the values, and shift to only keep 8 significant bits per colors
            __m128i rAG =_mm_add_epi16(topAG, bottomAG);
            rAG = _mm_srli_epi16(rAG, 8);
            _mm_storeu_si128((__m128i*)(&intermediate_buffer[1][f]), rAG);
            __m128i rRB =_mm_add_epi16(topRB, bottomRB);
            rRB = _mm_srli_epi16(rRB, 8);
            _mm_storeu_si128((__m128i*)(&intermediate_buffer[0][f]), rRB);
        }
#elif defined(__ARM_NEON__)
        const int16x8_t disty_ = vdupq_n_s16(disty);
        const int16x8_t idisty_ = vdupq_n_s16(idisty);
        const int16x8_t colorMask = vdupq_n_s16(0x00ff);

        lim -= 3;
        for (; f < lim; x += 4, f += 4) {
            // Load 4 pixels from s1, and split the alpha-green and red-blue component
            int16x8_t top = vld1q_s16((int16_t*)((const uint *)(s1)+x));
            int16x8_t topAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(top), 8));
            int16x8_t topRB = vandq_s16(top, colorMask);
            // Multiplies each color component by idisty
            topAG = vmulq_s16(topAG, idisty_);
            topRB = vmulq_s16(topRB, idisty_);

            // Same for the s2 vector
            int16x8_t bottom = vld1q_s16((int16_t*)((const uint *)(s2)+x));
            int16x8_t bottomAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(bottom), 8));
            int16x8_t bottomRB = vandq_s16(bottom, colorMask);
            bottomAG = vmulq_s16(bottomAG, disty_);
            bottomRB = vmulq_s16(bottomRB, disty_);

            // Add the values, and shift to only keep 8 significant bits per colors
            int16x8_t rAG = vaddq_s16(topAG, bottomAG);
            rAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(rAG), 8));
            vst1q_s16((int16_t*)(&intermediate_buffer[1][f]), rAG);
            int16x8_t rRB = vaddq_s16(topRB, bottomRB);
            rRB = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(rRB), 8));
            vst1q_s16((int16_t*)(&intermediate_buffer[0][f]), rRB);
        }
#endif
    }
    for (; f < count; f++) { // Same as above but without simd
        if (blendType == BlendTransformedBilinearTiled) {
            if (x >= image.width) x -= image.width;
        } else {
            x = qMin(x, image.x2 - 1);
        }

        uint t = s1[x];
        uint b = s2[x];

        intermediate_buffer[0][f] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
        intermediate_buffer[1][f] = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
        x++;
    }

    // Now interpolate the values from the intermediate_buffer to get the final result.
    fx -= offset * fixed_scale; // Switch to intermediate buffer coordinates

    while (b < end) {
        int x1 = (fx >> 16);
        int x2 = x1 + 1;
        Q_ASSERT(x1 >= 0);
        Q_ASSERT(x2 < count);

        int distx = (fx & 0x0000ffff) >> 8;
        int idistx = 256 - distx;
        int rb = ((intermediate_buffer[0][x1] * idistx + intermediate_buffer[0][x2] * distx) >> 8) & 0xff00ff;
        int ag = (intermediate_buffer[1][x1] * idistx + intermediate_buffer[1][x2] * distx) & 0xff00ff00;
        *b = rb | ag;
        b++;
        fx += fdx;
    }
}

template<TextureBlendType blendType>
static void QT_FASTCALL fetchTransformedBilinearARGB32PM_upscale_helper(uint *b, uint *end, const QTextureData &image,
                                                                        int &fx, int &fy, int fdx, int /*fdy*/)
{
    int y1 = (fy >> 16);
    int y2;
    fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);
    const uint *s1 = (const uint *)image.scanLine(y1);
    const uint *s2 = (const uint *)image.scanLine(y2);
    const int disty = (fy & 0x0000ffff) >> 8;

    if (blendType != BlendTransformedBilinearTiled) {
        const qint64 min_fx = qint64(image.x1) * fixed_scale;
        const qint64 max_fx = qint64(image.x2 - 1) * fixed_scale;
        while (b < end) {
            int x1 = (fx >> 16);
            int x2;
            fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
            if (x1 != x2)
                break;
            uint top = s1[x1];
            uint bot = s2[x1];
            *b = INTERPOLATE_PIXEL_256(top, 256 - disty, bot, disty);
            fx += fdx;
            ++b;
        }
        uint *boundedEnd = end;
        if (fdx > 0)
            boundedEnd = qMin(boundedEnd, b + (max_fx - fx) / fdx);
        else if (fdx < 0)
            boundedEnd = qMin(boundedEnd, b + (min_fx - fx) / fdx);

        // A fast middle part without boundary checks
        while (b < boundedEnd) {
            int x = (fx >> 16);
            int distx = (fx & 0x0000ffff) >> 8;
            *b = interpolate_4_pixels(s1 + x, s2 + x, distx, disty);
            fx += fdx;
            ++b;
        }
    }

    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1 , x1, x2);
        uint tl = s1[x1];
        uint tr = s1[x2];
        uint bl = s2[x1];
        uint br = s2[x2];
        int distx = (fx & 0x0000ffff) >> 8;
        *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);

        fx += fdx;
        ++b;
    }
}

template<TextureBlendType blendType>
static void QT_FASTCALL fetchTransformedBilinearARGB32PM_downscale_helper(uint *b, uint *end, const QTextureData &image,
                                                                          int &fx, int &fy, int fdx, int /*fdy*/)
{
    int y1 = (fy >> 16);
    int y2;
    fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);
    const uint *s1 = (const uint *)image.scanLine(y1);
    const uint *s2 = (const uint *)image.scanLine(y2);
    const int disty8 = (fy & 0x0000ffff) >> 8;
    const int disty4 = (disty8 + 0x08) >> 4;

    if (blendType != BlendTransformedBilinearTiled) {
        const qint64 min_fx = qint64(image.x1) * fixed_scale;
        const qint64 max_fx = qint64(image.x2 - 1) * fixed_scale;
        while (b < end) {
            int x1 = (fx >> 16);
            int x2;
            fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
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
#if defined(__SSE2__)
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        const __m128i v_256 = _mm_set1_epi16(256);
        const __m128i v_disty = _mm_set1_epi16(disty4);
        const __m128i v_fdx = _mm_set1_epi32(fdx*4);
        const __m128i v_fx_r = _mm_set1_epi32(0x8);
        __m128i v_fx = _mm_setr_epi32(fx, fx + fdx, fx + fdx + fdx, fx + fdx + fdx + fdx);

        while (b < boundedEnd - 3) {
            __m128i offset = _mm_srli_epi32(v_fx, 16);
            const int offset0 = _mm_cvtsi128_si32(offset); offset = _mm_srli_si128(offset, 4);
            const int offset1 = _mm_cvtsi128_si32(offset); offset = _mm_srli_si128(offset, 4);
            const int offset2 = _mm_cvtsi128_si32(offset); offset = _mm_srli_si128(offset, 4);
            const int offset3 = _mm_cvtsi128_si32(offset);
            const __m128i tl = _mm_setr_epi32(s1[offset0], s1[offset1], s1[offset2], s1[offset3]);
            const __m128i tr = _mm_setr_epi32(s1[offset0 + 1], s1[offset1 + 1], s1[offset2 + 1], s1[offset3 + 1]);
            const __m128i bl = _mm_setr_epi32(s2[offset0], s2[offset1], s2[offset2], s2[offset3]);
            const __m128i br = _mm_setr_epi32(s2[offset0 + 1], s2[offset1 + 1], s2[offset2 + 1], s2[offset3 + 1]);

            __m128i v_distx = _mm_srli_epi16(v_fx, 8);
            v_distx = _mm_srli_epi16(_mm_add_epi32(v_distx, v_fx_r), 4);
            v_distx = _mm_shufflehi_epi16(v_distx, _MM_SHUFFLE(2,2,0,0));
            v_distx = _mm_shufflelo_epi16(v_distx, _MM_SHUFFLE(2,2,0,0));

            interpolate_4_pixels_16_sse2(tl, tr, bl, br, v_distx, v_disty, colorMask, v_256, b);
            b += 4;
            v_fx = _mm_add_epi32(v_fx, v_fdx);
        }
        fx = _mm_cvtsi128_si32(v_fx);
#elif defined(__ARM_NEON__)
        const int16x8_t colorMask = vdupq_n_s16(0x00ff);
        const int16x8_t invColorMask = vmvnq_s16(colorMask);
        const int16x8_t v_256 = vdupq_n_s16(256);
        const int16x8_t v_disty = vdupq_n_s16(disty4);
        const int16x8_t v_disty_ = vshlq_n_s16(v_disty, 4);
        int32x4_t v_fdx = vdupq_n_s32(fdx*4);

        int32x4_t v_fx = vmovq_n_s32(fx);
        v_fx = vsetq_lane_s32(fx + fdx, v_fx, 1);
        v_fx = vsetq_lane_s32(fx + fdx * 2, v_fx, 2);
        v_fx = vsetq_lane_s32(fx + fdx * 3, v_fx, 3);

        const int32x4_t v_ffff_mask = vdupq_n_s32(0x0000ffff);
        const int32x4_t v_fx_r = vdupq_n_s32(0x0800);

        while (b < boundedEnd - 3) {
            uint32x4x2_t v_top, v_bot;

            int x1 = (fx >> 16);
            fx += fdx;
            v_top = vld2q_lane_u32(s1 + x1, v_top, 0);
            v_bot = vld2q_lane_u32(s2 + x1, v_bot, 0);
            x1 = (fx >> 16);
            fx += fdx;
            v_top = vld2q_lane_u32(s1 + x1, v_top, 1);
            v_bot = vld2q_lane_u32(s2 + x1, v_bot, 1);
            x1 = (fx >> 16);
            fx += fdx;
            v_top = vld2q_lane_u32(s1 + x1, v_top, 2);
            v_bot = vld2q_lane_u32(s2 + x1, v_bot, 2);
            x1 = (fx >> 16);
            fx += fdx;
            v_top = vld2q_lane_u32(s1 + x1, v_top, 3);
            v_bot = vld2q_lane_u32(s2 + x1, v_bot, 3);

            int32x4_t v_distx = vshrq_n_s32(vaddq_s32(vandq_s32(v_fx, v_ffff_mask), v_fx_r), 12);
            v_distx = vorrq_s32(v_distx, vshlq_n_s32(v_distx, 16));

            interpolate_4_pixels_16_neon(
                        vreinterpretq_s16_u32(v_top.val[0]), vreinterpretq_s16_u32(v_top.val[1]),
                    vreinterpretq_s16_u32(v_bot.val[0]), vreinterpretq_s16_u32(v_bot.val[1]),
                    vreinterpretq_s16_s32(v_distx), v_disty, v_disty_,
                    colorMask, invColorMask, v_256, b);
            b+=4;
            v_fx = vaddq_s32(v_fx, v_fdx);
        }
#endif
        while (b < boundedEnd) {
            int x = (fx >> 16);
            if (hasFastInterpolate4()) {
                int distx8 = (fx & 0x0000ffff) >> 8;
                *b = interpolate_4_pixels(s1 + x, s2 + x, distx8, disty8);
            } else {
                int distx4 = ((fx & 0x0000ffff) + 0x0800) >> 12;
                *b = interpolate_4_pixels_16(s1[x], s1[x + 1], s2[x], s2[x + 1], distx4, disty4);
            }
            fx += fdx;
            ++b;
        }
    }

    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
        uint tl = s1[x1];
        uint tr = s1[x2];
        uint bl = s2[x1];
        uint br = s2[x2];
        if (hasFastInterpolate4()) {
            int distx8 = (fx & 0x0000ffff) >> 8;
            *b = interpolate_4_pixels(tl, tr, bl, br, distx8, disty8);
        } else {
            int distx4 = ((fx & 0x0000ffff) + 0x0800) >> 12;
            *b = interpolate_4_pixels_16(tl, tr, bl, br, distx4, disty4);
        }
        fx += fdx;
        ++b;
    }
}

template<TextureBlendType blendType>
static void QT_FASTCALL fetchTransformedBilinearARGB32PM_rotate_helper(uint *b, uint *end, const QTextureData &image,
                                                                       int &fx, int &fy, int fdx, int fdy)
{
    // if we are zooming more than 8 times, we use 8bit precision for the position.
    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        int y1 = (fy >> 16);
        int y2;

        fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
        fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);

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

template<TextureBlendType blendType>
static void QT_FASTCALL fetchTransformedBilinearARGB32PM_fast_rotate_helper(uint *b, uint *end, const QTextureData &image,
                                                                            int &fx, int &fy, int fdx, int fdy)
{
    //we are zooming less than 8x, use 4bit precision
    if (blendType != BlendTransformedBilinearTiled) {
        const qint64 min_fx = qint64(image.x1) * fixed_scale;
        const qint64 max_fx = qint64(image.x2 - 1) * fixed_scale;
        const qint64 min_fy = qint64(image.y1) * fixed_scale;
        const qint64 max_fy = qint64(image.y2 - 1) * fixed_scale;
        // first handle the possibly bounded part in the beginning
        while (b < end) {
            int x1 = (fx >> 16);
            int x2;
            int y1 = (fy >> 16);
            int y2;
            fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
            fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);
            if (x1 != x2 && y1 != y2)
                break;
            const uint *s1 = (const uint *)image.scanLine(y1);
            const uint *s2 = (const uint *)image.scanLine(y2);
            uint tl = s1[x1];
            uint tr = s1[x2];
            uint bl = s2[x1];
            uint br = s2[x2];
            if (hasFastInterpolate4()) {
                int distx = (fx & 0x0000ffff) >> 8;
                int disty = (fy & 0x0000ffff) >> 8;
                *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);
            } else {
                int distx = ((fx & 0x0000ffff) + 0x0800) >> 12;
                int disty = ((fy & 0x0000ffff) + 0x0800) >> 12;
                *b = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty);
            }
            fx += fdx;
            fy += fdy;
            ++b;
        }
        uint *boundedEnd = end; \
        if (fdx > 0) \
            boundedEnd = qMin(boundedEnd, b + (max_fx - fx) / fdx); \
        else if (fdx < 0) \
            boundedEnd = qMin(boundedEnd, b + (min_fx - fx) / fdx); \
        if (fdy > 0) \
            boundedEnd = qMin(boundedEnd, b + (max_fy - fy) / fdy); \
        else if (fdy < 0) \
            boundedEnd = qMin(boundedEnd, b + (min_fy - fy) / fdy); \

        // until boundedEnd we can now have a fast middle part without boundary checks
#if defined(__SSE2__)
        const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
        const __m128i v_256 = _mm_set1_epi16(256);
        const __m128i v_fdx = _mm_set1_epi32(fdx*4);
        const __m128i v_fdy = _mm_set1_epi32(fdy*4);
        const __m128i v_fxy_r = _mm_set1_epi32(0x8);
        __m128i v_fx = _mm_setr_epi32(fx, fx + fdx, fx + fdx + fdx, fx + fdx + fdx + fdx);
        __m128i v_fy = _mm_setr_epi32(fy, fy + fdy, fy + fdy + fdy, fy + fdy + fdy + fdy);

        const uchar *textureData = image.imageData;
        const qsizetype bytesPerLine = image.bytesPerLine;
        const __m128i vbpl = _mm_shufflelo_epi16(_mm_cvtsi32_si128(bytesPerLine/4), _MM_SHUFFLE(0, 0, 0, 0));

        while (b < boundedEnd - 3) {
            const __m128i vy = _mm_packs_epi32(_mm_srli_epi32(v_fy, 16), _mm_setzero_si128());
            // 4x16bit * 4x16bit -> 4x32bit
            __m128i offset = _mm_unpacklo_epi16(_mm_mullo_epi16(vy, vbpl), _mm_mulhi_epi16(vy, vbpl));
            offset = _mm_add_epi32(offset, _mm_srli_epi32(v_fx, 16));
            const int offset0 = _mm_cvtsi128_si32(offset); offset = _mm_srli_si128(offset, 4);
            const int offset1 = _mm_cvtsi128_si32(offset); offset = _mm_srli_si128(offset, 4);
            const int offset2 = _mm_cvtsi128_si32(offset); offset = _mm_srli_si128(offset, 4);
            const int offset3 = _mm_cvtsi128_si32(offset);
            const uint *topData = (const uint *)(textureData);
            const __m128i tl = _mm_setr_epi32(topData[offset0], topData[offset1], topData[offset2], topData[offset3]);
            const __m128i tr = _mm_setr_epi32(topData[offset0 + 1], topData[offset1 + 1], topData[offset2 + 1], topData[offset3 + 1]);
            const uint *bottomData = (const uint *)(textureData + bytesPerLine);
            const __m128i bl = _mm_setr_epi32(bottomData[offset0], bottomData[offset1], bottomData[offset2], bottomData[offset3]);
            const __m128i br = _mm_setr_epi32(bottomData[offset0 + 1], bottomData[offset1 + 1], bottomData[offset2 + 1], bottomData[offset3 + 1]);

            __m128i v_distx = _mm_srli_epi16(v_fx, 8);
            __m128i v_disty = _mm_srli_epi16(v_fy, 8);
            v_distx = _mm_srli_epi16(_mm_add_epi32(v_distx, v_fxy_r), 4);
            v_disty = _mm_srli_epi16(_mm_add_epi32(v_disty, v_fxy_r), 4);
            v_distx = _mm_shufflehi_epi16(v_distx, _MM_SHUFFLE(2,2,0,0));
            v_distx = _mm_shufflelo_epi16(v_distx, _MM_SHUFFLE(2,2,0,0));
            v_disty = _mm_shufflehi_epi16(v_disty, _MM_SHUFFLE(2,2,0,0));
            v_disty = _mm_shufflelo_epi16(v_disty, _MM_SHUFFLE(2,2,0,0));

            interpolate_4_pixels_16_sse2(tl, tr, bl, br, v_distx, v_disty, colorMask, v_256, b);
            b += 4;
            v_fx = _mm_add_epi32(v_fx, v_fdx);
            v_fy = _mm_add_epi32(v_fy, v_fdy);
        }
        fx = _mm_cvtsi128_si32(v_fx);
        fy = _mm_cvtsi128_si32(v_fy);
#elif defined(__ARM_NEON__)
        const int16x8_t colorMask = vdupq_n_s16(0x00ff);
        const int16x8_t invColorMask = vmvnq_s16(colorMask);
        const int16x8_t v_256 = vdupq_n_s16(256);
        int32x4_t v_fdx = vdupq_n_s32(fdx * 4);
        int32x4_t v_fdy = vdupq_n_s32(fdy * 4);

        const uchar *textureData = image.imageData;
        const int bytesPerLine = image.bytesPerLine;

        int32x4_t v_fx = vmovq_n_s32(fx);
        int32x4_t v_fy = vmovq_n_s32(fy);
        v_fx = vsetq_lane_s32(fx + fdx, v_fx, 1);
        v_fy = vsetq_lane_s32(fy + fdy, v_fy, 1);
        v_fx = vsetq_lane_s32(fx + fdx * 2, v_fx, 2);
        v_fy = vsetq_lane_s32(fy + fdy * 2, v_fy, 2);
        v_fx = vsetq_lane_s32(fx + fdx * 3, v_fx, 3);
        v_fy = vsetq_lane_s32(fy + fdy * 3, v_fy, 3);

        const int32x4_t v_ffff_mask = vdupq_n_s32(0x0000ffff);
        const int32x4_t v_round = vdupq_n_s32(0x0800);

        while (b < boundedEnd - 3) {
            uint32x4x2_t v_top, v_bot;

            int x1 = (fx >> 16);
            int y1 = (fy >> 16);
            fx += fdx; fy += fdy;
            const uchar *sl = textureData + bytesPerLine * y1;
            const uint *s1 = reinterpret_cast<const uint *>(sl);
            const uint *s2 = reinterpret_cast<const uint *>(sl + bytesPerLine);
            v_top = vld2q_lane_u32(s1 + x1, v_top, 0);
            v_bot = vld2q_lane_u32(s2 + x1, v_bot, 0);
            x1 = (fx >> 16);
            y1 = (fy >> 16);
            fx += fdx; fy += fdy;
            sl = textureData + bytesPerLine * y1;
            s1 = reinterpret_cast<const uint *>(sl);
            s2 = reinterpret_cast<const uint *>(sl + bytesPerLine);
            v_top = vld2q_lane_u32(s1 + x1, v_top, 1);
            v_bot = vld2q_lane_u32(s2 + x1, v_bot, 1);
            x1 = (fx >> 16);
            y1 = (fy >> 16);
            fx += fdx; fy += fdy;
            sl = textureData + bytesPerLine * y1;
            s1 = reinterpret_cast<const uint *>(sl);
            s2 = reinterpret_cast<const uint *>(sl + bytesPerLine);
            v_top = vld2q_lane_u32(s1 + x1, v_top, 2);
            v_bot = vld2q_lane_u32(s2 + x1, v_bot, 2);
            x1 = (fx >> 16);
            y1 = (fy >> 16);
            fx += fdx; fy += fdy;
            sl = textureData + bytesPerLine * y1;
            s1 = reinterpret_cast<const uint *>(sl);
            s2 = reinterpret_cast<const uint *>(sl + bytesPerLine);
            v_top = vld2q_lane_u32(s1 + x1, v_top, 3);
            v_bot = vld2q_lane_u32(s2 + x1, v_bot, 3);

            int32x4_t v_distx = vshrq_n_s32(vaddq_s32(vandq_s32(v_fx, v_ffff_mask), v_round), 12);
            int32x4_t v_disty = vshrq_n_s32(vaddq_s32(vandq_s32(v_fy, v_ffff_mask), v_round), 12);
            v_distx = vorrq_s32(v_distx, vshlq_n_s32(v_distx, 16));
            v_disty = vorrq_s32(v_disty, vshlq_n_s32(v_disty, 16));
            int16x8_t v_disty_ = vshlq_n_s16(vreinterpretq_s16_s32(v_disty), 4);

            interpolate_4_pixels_16_neon(
                        vreinterpretq_s16_u32(v_top.val[0]), vreinterpretq_s16_u32(v_top.val[1]),
                        vreinterpretq_s16_u32(v_bot.val[0]), vreinterpretq_s16_u32(v_bot.val[1]),
                        vreinterpretq_s16_s32(v_distx), vreinterpretq_s16_s32(v_disty),
                        v_disty_, colorMask, invColorMask, v_256, b);
            b += 4;
            v_fx = vaddq_s32(v_fx, v_fdx);
            v_fy = vaddq_s32(v_fy, v_fdy);
        }
#endif
        while (b < boundedEnd) {
            int x = (fx >> 16);
            int y = (fy >> 16);

            const uint *s1 = (const uint *)image.scanLine(y);
            const uint *s2 = (const uint *)image.scanLine(y + 1);

            if (hasFastInterpolate4()) {
                int distx = (fx & 0x0000ffff) >> 8;
                int disty = (fy & 0x0000ffff) >> 8;
                *b = interpolate_4_pixels(s1 + x, s2 + x, distx, disty);
            } else {
                int distx = ((fx & 0x0000ffff) + 0x0800) >> 12;
                int disty = ((fy & 0x0000ffff) + 0x0800) >> 12;
                *b = interpolate_4_pixels_16(s1[x], s1[x + 1], s2[x], s2[x + 1], distx, disty);
            }

            fx += fdx;
            fy += fdy;
            ++b;
        }
    }

    while (b < end) {
        int x1 = (fx >> 16);
        int x2;
        int y1 = (fy >> 16);
        int y2;

        fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
        fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);

        const uint *s1 = (const uint *)image.scanLine(y1);
        const uint *s2 = (const uint *)image.scanLine(y2);

        uint tl = s1[x1];
        uint tr = s1[x2];
        uint bl = s2[x1];
        uint br = s2[x2];

        if (hasFastInterpolate4()) {
            int distx = (fx & 0x0000ffff) >> 8;
            int disty = (fy & 0x0000ffff) >> 8;
            *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);
        } else {
            int distx = ((fx & 0x0000ffff) + 0x0800) >> 12;
            int disty = ((fy & 0x0000ffff) + 0x0800) >> 12;
            *b = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty);
        }

        fx += fdx;
        fy += fdy;
        ++b;
    }
}


static BilinearFastTransformHelper bilinearFastTransformHelperARGB32PM[2][NFastTransformTypes] = {
    {
        fetchTransformedBilinearARGB32PM_simple_upscale_helper<BlendTransformedBilinear>,
        fetchTransformedBilinearARGB32PM_upscale_helper<BlendTransformedBilinear>,
        fetchTransformedBilinearARGB32PM_downscale_helper<BlendTransformedBilinear>,
        fetchTransformedBilinearARGB32PM_rotate_helper<BlendTransformedBilinear>,
        fetchTransformedBilinearARGB32PM_fast_rotate_helper<BlendTransformedBilinear>
    },
    {
        fetchTransformedBilinearARGB32PM_simple_upscale_helper<BlendTransformedBilinearTiled>,
        fetchTransformedBilinearARGB32PM_upscale_helper<BlendTransformedBilinearTiled>,
        fetchTransformedBilinearARGB32PM_downscale_helper<BlendTransformedBilinearTiled>,
        fetchTransformedBilinearARGB32PM_rotate_helper<BlendTransformedBilinearTiled>,
        fetchTransformedBilinearARGB32PM_fast_rotate_helper<BlendTransformedBilinearTiled>
    }
};

template<TextureBlendType blendType> /* blendType = BlendTransformedBilinear or BlendTransformedBilinearTiled */
static const uint * QT_FASTCALL fetchTransformedBilinearARGB32PM(uint *buffer, const Operator *,
                                                                 const QSpanData *data, int y, int x,
                                                                 int length)
{
    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);
    Q_CONSTEXPR int tiled = (blendType == BlendTransformedBilinearTiled) ? 1 : 0;

    uint *end = buffer + length;
    uint *b = buffer;
    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy
                      + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy
                      + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;

        if (fdy == 0) { // simple scale, no rotation or shear
            if (qAbs(fdx) <= fixed_scale) {
                // simple scale up on X
                bilinearFastTransformHelperARGB32PM[tiled][SimpleUpscaleTransform](b, end, data->texture, fx, fy, fdx, fdy);
            } else if (qAbs(data->m22) < qreal(1./8.)) {
                // scale up more than 8x (on Y)
                bilinearFastTransformHelperARGB32PM[tiled][UpscaleTransform](b, end, data->texture, fx, fy, fdx, fdy);
            } else {
                // scale down on X
                bilinearFastTransformHelperARGB32PM[tiled][DownscaleTransform](b, end, data->texture, fx, fy, fdx, fdy);
            }
        } else { // rotation or shear
            if (qAbs(data->m11) < qreal(1./8.) || qAbs(data->m22) < qreal(1./8.) ) {
                // if we are zooming more than 8 times, we use 8bit precision for the position.
                bilinearFastTransformHelperARGB32PM[tiled][RotateTransform](b, end, data->texture, fx, fy, fdx, fdy);
            } else {
                // we are zooming less than 8x, use 4bit precision
                bilinearFastTransformHelperARGB32PM[tiled][FastRotateTransform](b, end, data->texture, fx, fy, fdx, fdy);
            }
        }
    } else {
        const QTextureData &image = data->texture;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        while (b < end) {
            const qreal iw = fw == 0 ? 1 : 1 / fw;
            const qreal px = fx * iw - qreal(0.5);
            const qreal py = fy * iw - qreal(0.5);

            int x1 = int(px) - (px < 0);
            int x2;
            int y1 = int(py) - (py < 0);
            int y2;

            int distx = int((px - x1) * 256);
            int disty = int((py - y1) * 256);

            fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
            fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);

            const uint *s1 = (const uint *)data->texture.scanLine(y1);
            const uint *s2 = (const uint *)data->texture.scanLine(y2);

            uint tl = s1[x1];
            uint tr = s1[x2];
            uint bl = s2[x1];
            uint br = s2[x2];

            *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);

            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }

    return buffer;
}

template<TextureBlendType blendType, QPixelLayout::BPP bpp>
static void QT_FASTCALL fetchTransformedBilinear_simple_upscale_helper(uint *b, uint *end, const QTextureData &image,
                                                                       int &fx, int &fy, int fdx, int /*fdy*/)
{
    const QPixelLayout *layout = &qPixelLayouts[image.format];
    const QVector<QRgb> *clut = image.colorTable;
    Q_ASSERT(bpp == QPixelLayout::BPPNone || bpp == layout->bpp);
    // When templated 'fetch' should be inlined at compile time:
    const FetchPixelsFunc fetch = (bpp == QPixelLayout::BPPNone) ? qFetchPixels[layout->bpp] : fetchPixels<bpp>;
    const ConvertFunc convertToARGB32PM = layout->convertToARGB32PM;

    int y1 = (fy >> 16);
    int y2;
    fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);
    const uchar *s1 = image.scanLine(y1);
    const uchar *s2 = image.scanLine(y2);

    const int disty = (fy & 0x0000ffff) >> 8;
    const int idisty = 256 - disty;
    const int length = end - b;

    // The intermediate buffer is generated in the positive direction
    const int adjust = (fdx < 0) ? fdx * length : 0;
    const int offset = (fx + adjust) >> 16;
    int x = offset;

    // The idea is first to do the interpolation between the row s1 and the row s2
    // into an intermediate buffer, then we interpolate between two pixel of this buffer.
    // +1 for the last pixel to interpolate with, and +1 for rounding errors.
    uint buf1[buffer_size + 2];
    uint buf2[buffer_size + 2];
    const uint *ptr1;
    const uint *ptr2;

    int count = (qint64(length) * qAbs(fdx) + fixed_scale - 1) / fixed_scale + 2;
    Q_ASSERT(count <= buffer_size + 2); //length is supposed to be <= buffer_size and data->m11 < 1 in this case

    if (blendType == BlendTransformedBilinearTiled) {
        x %= image.width;
        if (x < 0)
            x += image.width;
        int len1 = qMin(count, image.width - x);
        int len2 = qMin(x, count - len1);

        ptr1 = fetch(buf1, s1, x, len1);
        ptr1 = convertToARGB32PM(buf1, ptr1, len1, clut, 0);
        ptr2 = fetch(buf2, s2, x, len1);
        ptr2 = convertToARGB32PM(buf2, ptr2, len1, clut, 0);
        for (int i = 0; i < len1; ++i) {
            uint t = ptr1[i];
            uint b = ptr2[i];
            buf1[i] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
            buf2[i] = ((((t >> 8) & 0xff00ff) * idisty + ((b >> 8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
        }

        if (len2) {
            ptr1 = fetch(buf1 + len1, s1, 0, len2);
            ptr1 = convertToARGB32PM(buf1 + len1, ptr1, len2, clut, 0);
            ptr2 = fetch(buf2 + len1, s2, 0, len2);
            ptr2 = convertToARGB32PM(buf2 + len1, ptr2, len2, clut, 0);
            for (int i = 0; i < len2; ++i) {
                uint t = ptr1[i];
                uint b = ptr2[i];
                buf1[i + len1] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
                buf2[i + len1] = ((((t >> 8) & 0xff00ff) * idisty + ((b >> 8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
            }
        }
        for (int i = image.width; i < count; ++i) {
            buf1[i] = buf1[i - image.width];
            buf2[i] = buf2[i - image.width];
        }
    } else {
        int start = qMax(x, image.x1);
        int end = qMin(x + count, image.x2);
        int len = qMax(1, end - start);
        int leading = start - x;

        ptr1 = fetch(buf1 + leading, s1, start, len);
        ptr1 = convertToARGB32PM(buf1 + leading, ptr1, len, clut, 0);
        ptr2 = fetch(buf2 + leading, s2, start, len);
        ptr2 = convertToARGB32PM(buf2 + leading, ptr2, len, clut, 0);

        for (int i = 0; i < len; ++i) {
            uint t = ptr1[i];
            uint b = ptr2[i];
            buf1[i + leading] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
            buf2[i + leading] = ((((t >> 8) & 0xff00ff) * idisty + ((b >> 8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
        }

        for (int i = 0; i < leading; ++i) {
            buf1[i] = buf1[leading];
            buf2[i] = buf2[leading];
        }
        for (int i = leading + len; i < count; ++i) {
            buf1[i] = buf1[i - 1];
            buf2[i] = buf2[i - 1];
        }
    }

    // Now interpolate the values from the intermediate_buffer to get the final result.
    fx -= offset * fixed_scale; // Switch to intermediate buffer coordinates

    while (b < end) {
        int x1 = (fx >> 16);
        int x2 = x1 + 1;
        Q_ASSERT(x1 >= 0);
        Q_ASSERT(x2 < count);

        int distx = (fx & 0x0000ffff) >> 8;
        int idistx = 256 - distx;
        int rb = ((buf1[x1] * idistx + buf1[x2] * distx) >> 8) & 0xff00ff;
        int ag = (buf2[x1] * idistx + buf2[x2] * distx) & 0xff00ff00;
        *b++ = rb | ag;
        fx += fdx;
    }
}


typedef void (QT_FASTCALL *BilinearFastTransformFetcher)(uint *buf1, uint *buf2, const int len, const QTextureData &image,
                                                         int fx, int fy, const int fdx, const int fdy);

template<TextureBlendType blendType, QPixelLayout::BPP bpp>
static void QT_FASTCALL fetchTransformedBilinear_fetcher(uint *buf1, uint *buf2, const int len, const QTextureData &image,
                                                         int fx, int fy, const int fdx, const int fdy)
{
    const QPixelLayout &layout = qPixelLayouts[image.format];
    Q_ASSERT(bpp == QPixelLayout::BPPNone || bpp == layout.bpp);
    // When templated 'fetch1' should be inlined at compile time:
    const FetchPixelFunc fetch1 = (bpp == QPixelLayout::BPPNone) ? qFetchPixel[layout.bpp] : fetchPixel<bpp>;
    if (fdy == 0) {
        int y1 = (fy >> 16);
        int y2;
        fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);
        const uchar *s1 = image.scanLine(y1);
        const uchar *s2 = image.scanLine(y2);

        int i = 0;
        if (blendType == BlendTransformedBilinear) {
            for (; i < len; ++i) {
                int x1 = (fx >> 16);
                int x2;
                fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
                if (x1 != x2)
                    break;
                buf1[i * 2 + 0] = buf1[i * 2 + 1] = fetch1(s1, x1);
                buf2[i * 2 + 0] = buf2[i * 2 + 1] = fetch1(s2, x1);
                fx += fdx;
            }
            int fastLen = len;
            if (fdx > 0)
                fastLen = qMin(fastLen, int((qint64(image.x2 - 1) * fixed_scale - fx) / fdx));
            else if (fdx < 0)
                fastLen = qMin(fastLen, int((qint64(image.x1) * fixed_scale - fx) / fdx));

            for (; i < fastLen; ++i) {
                int x = (fx >> 16);
                buf1[i * 2 + 0] = fetch1(s1, x);
                buf1[i * 2 + 1] = fetch1(s1, x + 1);
                buf2[i * 2 + 0] = fetch1(s2, x);
                buf2[i * 2 + 1] = fetch1(s2, x + 1);
                fx += fdx;
            }
        }

        for (; i < len; ++i) {
            int x1 = (fx >> 16);
            int x2;
            fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
            buf1[i * 2 + 0] = fetch1(s1, x1);
            buf1[i * 2 + 1] = fetch1(s1, x2);
            buf2[i * 2 + 0] = fetch1(s2, x1);
            buf2[i * 2 + 1] = fetch1(s2, x2);
            fx += fdx;
        }
    } else {
        int i = 0;
        if (blendType == BlendTransformedBilinear) {
            for (; i < len; ++i) {
                int x1 = (fx >> 16);
                int x2;
                int y1 = (fy >> 16);
                int y2;
                fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
                fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);
                if (x1 != x2 && y1 != y2)
                    break;
                const uchar *s1 = image.scanLine(y1);
                const uchar *s2 = image.scanLine(y2);
                buf1[i * 2 + 0] = fetch1(s1, x1);
                buf1[i * 2 + 1] = fetch1(s1, x2);
                buf2[i * 2 + 0] = fetch1(s2, x1);
                buf2[i * 2 + 1] = fetch1(s2, x2);
                fx += fdx;
                fy += fdy;
            }
            int fastLen = len;
            if (fdx > 0)
                fastLen = qMin(fastLen, int((qint64(image.x2 - 1) * fixed_scale - fx) / fdx));
            else if (fdx < 0)
                fastLen = qMin(fastLen, int((qint64(image.x1) * fixed_scale - fx) / fdx));
            if (fdy > 0)
                fastLen = qMin(fastLen, int((qint64(image.y2 - 1) * fixed_scale - fy) / fdy));
            else if (fdy < 0)
                fastLen = qMin(fastLen, int((qint64(image.y1) * fixed_scale - fy) / fdy));

            for (; i < fastLen; ++i) {
                int x = (fx >> 16);
                int y = (fy >> 16);
                const uchar *s1 = image.scanLine(y);
                const uchar *s2 = s1 + image.bytesPerLine;
                buf1[i * 2 + 0] = fetch1(s1, x);
                buf1[i * 2 + 1] = fetch1(s1, x + 1);
                buf2[i * 2 + 0] = fetch1(s2, x);
                buf2[i * 2 + 1] = fetch1(s2, x + 1);
                fx += fdx;
                fy += fdy;
            }
        }

        for (; i < len; ++i) {
            int x1 = (fx >> 16);
            int x2;
            int y1 = (fy >> 16);
            int y2;
            fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
            fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);

            const uchar *s1 = image.scanLine(y1);
            const uchar *s2 = image.scanLine(y2);
            buf1[i * 2 + 0] = fetch1(s1, x1);
            buf1[i * 2 + 1] = fetch1(s1, x2);
            buf2[i * 2 + 0] = fetch1(s2, x1);
            buf2[i * 2 + 1] = fetch1(s2, x2);
            fx += fdx;
            fy += fdy;
        }
    }
}

// blendType = BlendTransformedBilinear or BlendTransformedBilinearTiled
template<TextureBlendType blendType, QPixelLayout::BPP bpp>
static const uint *QT_FASTCALL fetchTransformedBilinear(uint *buffer, const Operator *,
                                                        const QSpanData *data, int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    const QVector<QRgb> *clut = data->texture.colorTable;
    Q_ASSERT(bpp == QPixelLayout::BPPNone || layout->bpp == bpp);

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;

        if (fdy == 0) { // simple scale, no rotation or shear
            if (qAbs(fdx) <= fixed_scale) { // scale up on X
                fetchTransformedBilinear_simple_upscale_helper<blendType, bpp>(buffer, buffer + length, data->texture, fx, fy, fdx, fdy);
            } else {
                const BilinearFastTransformFetcher fetcher = fetchTransformedBilinear_fetcher<blendType,bpp>;

                uint buf1[buffer_size];
                uint buf2[buffer_size];
                uint *b = buffer;
                while (length) {
                    int len = qMin(length, buffer_size / 2);
                    fetcher(buf1, buf2, len, data->texture, fx, fy, fdx, 0);
                    layout->convertToARGB32PM(buf1, buf1, len * 2, clut, 0);
                    layout->convertToARGB32PM(buf2, buf2, len * 2, clut, 0);

                    if (hasFastInterpolate4() || qAbs(data->m22) < qreal(1./8.)) { // scale up more than 8x (on Y)
                        int disty = (fy & 0x0000ffff) >> 8;
                        for (int i = 0; i < len; ++i) {
                            int distx = (fx & 0x0000ffff) >> 8;
                            b[i] = interpolate_4_pixels(buf1 + i * 2, buf2 + i * 2, distx, disty);
                            fx += fdx;
                        }
                    } else {
                        int disty = ((fy & 0x0000ffff) + 0x0800) >> 12;
                        for (int i = 0; i < len; ++i) {
                            uint tl = buf1[i * 2 + 0];
                            uint tr = buf1[i * 2 + 1];
                            uint bl = buf2[i * 2 + 0];
                            uint br = buf2[i * 2 + 1];
                            int distx = ((fx & 0x0000ffff) + 0x0800) >> 12;
                            b[i] = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty);
                            fx += fdx;
                        }
                    }
                    length -= len;
                    b += len;
                }
            }
        } else { // rotation or shear
            const BilinearFastTransformFetcher fetcher = fetchTransformedBilinear_fetcher<blendType,bpp>;

            uint buf1[buffer_size];
            uint buf2[buffer_size];
            uint *b = buffer;
            while (length) {
                int len = qMin(length, buffer_size / 2);
                fetcher(buf1, buf2, len, data->texture, fx, fy, fdx, fdy);
                layout->convertToARGB32PM(buf1, buf1, len * 2, clut, 0);
                layout->convertToARGB32PM(buf2, buf2, len * 2, clut, 0);

                if (hasFastInterpolate4() || qAbs(data->m11) < qreal(1./8.) || qAbs(data->m22) < qreal(1./8.)) {
                    // If we are zooming more than 8 times, we use 8bit precision for the position.
                    for (int i = 0; i < len; ++i) {
                        int distx = (fx & 0x0000ffff) >> 8;
                        int disty = (fy & 0x0000ffff) >> 8;

                        b[i] = interpolate_4_pixels(buf1 + i * 2, buf2 + i * 2, distx, disty);
                        fx += fdx;
                        fy += fdy;
                    }
                } else {
                    // We are zooming less than 8x, use 4bit precision
                    for (int i = 0; i < len; ++i) {
                        uint tl = buf1[i * 2 + 0];
                        uint tr = buf1[i * 2 + 1];
                        uint bl = buf2[i * 2 + 0];
                        uint br = buf2[i * 2 + 1];

                        int distx = ((fx & 0x0000ffff) + 0x0800) >> 12;
                        int disty = ((fy & 0x0000ffff) + 0x0800) >> 12;

                        b[i] = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty);
                        fx += fdx;
                        fy += fdy;
                    }
                }

                length -= len;
                b += len;
            }
        }
    } else {
        // When templated 'fetch' should be inlined at compile time:
        const FetchPixelFunc fetch1 = (bpp == QPixelLayout::BPPNone) ? qFetchPixel[layout->bpp] : fetchPixel<bpp>;

        const QTextureData &image = data->texture;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        uint buf1[buffer_size];
        uint buf2[buffer_size];
        uint *b = buffer;

        int distxs[buffer_size / 2];
        int distys[buffer_size / 2];

        while (length) {
            int len = qMin(length, buffer_size / 2);
            for (int i = 0; i < len; ++i) {
                const qreal iw = fw == 0 ? 1 : 1 / fw;
                const qreal px = fx * iw - qreal(0.5);
                const qreal py = fy * iw - qreal(0.5);

                int x1 = int(px) - (px < 0);
                int x2;
                int y1 = int(py) - (py < 0);
                int y2;

                distxs[i] = int((px - x1) * 256);
                distys[i] = int((py - y1) * 256);

                fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
                fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);

                const uchar *s1 = data->texture.scanLine(y1);
                const uchar *s2 = data->texture.scanLine(y2);
                buf1[i * 2 + 0] = fetch1(s1, x1);
                buf1[i * 2 + 1] = fetch1(s1, x2);
                buf2[i * 2 + 0] = fetch1(s2, x1);
                buf2[i * 2 + 1] = fetch1(s2, x2);

                fx += fdx;
                fy += fdy;
                fw += fdw;
                //force increment to avoid /0
                if (!fw)
                    fw += fdw;
            }

            layout->convertToARGB32PM(buf1, buf1, len * 2, clut, 0);
            layout->convertToARGB32PM(buf2, buf2, len * 2, clut, 0);

            for (int i = 0; i < len; ++i) {
                int distx = distxs[i];
                int disty = distys[i];

                b[i] = interpolate_4_pixels(buf1 + i * 2, buf2 + i * 2, distx, disty);
            }
            length -= len;
            b += len;
        }
    }

    return buffer;
}

template<TextureBlendType blendType>
static const QRgba64 *QT_FASTCALL fetchTransformedBilinear64(QRgba64 *buffer, const Operator *,
                                                             const QSpanData *data, int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    const QVector<QRgb> *clut = data->texture.colorTable;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;

        const BilinearFastTransformFetcher fetcher =
                (layout->bpp == QPixelLayout::BPP32)
                        ? fetchTransformedBilinear_fetcher<blendType, QPixelLayout::BPP32>
                        : fetchTransformedBilinear_fetcher<blendType, QPixelLayout::BPPNone>;

        if (fdy == 0) { //simple scale, no rotation

            uint sbuf1[buffer_size];
            uint sbuf2[buffer_size];
            quint64 buf1[buffer_size];
            quint64 buf2[buffer_size];
            QRgba64 *b = buffer;
            while (length) {
                int len = qMin(length, buffer_size / 2);
                int disty = (fy & 0x0000ffff);
#if defined(__SSE2__)
                const __m128i vdy = _mm_set1_epi16(disty);
                const __m128i vidy = _mm_set1_epi16(0x10000 - disty);
#endif
                fetcher(sbuf1, sbuf2, len, data->texture, fx, fy, fdx, fdy);

                layout->convertToARGB64PM((QRgba64 *)buf1, sbuf1, len * 2, clut, 0);
                if (disty)
                    layout->convertToARGB64PM((QRgba64 *)buf2, sbuf2, len * 2, clut, 0);

                for (int i = 0; i < len; ++i) {
                    int distx = (fx & 0x0000ffff);
#if defined(__SSE2__)
                    __m128i vt = _mm_loadu_si128((const __m128i*)(buf1 + i*2));
                    if (disty) {
                        __m128i vb = _mm_loadu_si128((const __m128i*)(buf2 + i*2));
                        vt = _mm_mulhi_epu16(vt, vidy);
                        vb = _mm_mulhi_epu16(vb, vdy);
                        vt = _mm_add_epi16(vt, vb);
                    }
                    if (distx) {
                        const __m128i vdistx = _mm_shufflelo_epi16(_mm_cvtsi32_si128(distx), _MM_SHUFFLE(0, 0, 0, 0));
                        const __m128i vidistx = _mm_shufflelo_epi16(_mm_cvtsi32_si128(0x10000 - distx), _MM_SHUFFLE(0, 0, 0, 0));
                        vt = _mm_mulhi_epu16(vt, _mm_unpacklo_epi64(vidistx, vdistx));
                        vt = _mm_add_epi16(vt, _mm_srli_si128(vt, 8));
                    }
                    _mm_storel_epi64((__m128i*)(b+i), vt);
#else
                    b[i] = interpolate_4_pixels_rgb64((QRgba64 *)buf1 + i*2, (QRgba64 *)buf2 + i*2, distx, disty);
#endif
                    fx += fdx;
                }
                length -= len;
                b += len;
            }
        } else { //rotation
            uint sbuf1[buffer_size];
            uint sbuf2[buffer_size];
            quint64 buf1[buffer_size];
            quint64 buf2[buffer_size];
            QRgba64 *end = buffer + length;
            QRgba64 *b = buffer;

            while (b < end) {
                int len = qMin(length, buffer_size / 2);

                fetcher(sbuf1, sbuf2, len, data->texture, fx, fy, fdx, fdy);

                layout->convertToARGB64PM((QRgba64 *)buf1, sbuf1, len * 2, clut, 0);
                layout->convertToARGB64PM((QRgba64 *)buf2, sbuf2, len * 2, clut, 0);

                for (int i = 0; i < len; ++i) {
                    int distx = (fx & 0x0000ffff);
                    int disty = (fy & 0x0000ffff);
                    b[i] = interpolate_4_pixels_rgb64((QRgba64 *)buf1 + i*2, (QRgba64 *)buf2 + i*2, distx, disty);
                    fx += fdx;
                    fy += fdy;
                }

                length -= len;
                b += len;
            }
        }
    } else {
        const QTextureData &image = data->texture;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        FetchPixelFunc fetch = qFetchPixel[layout->bpp];
        uint sbuf1[buffer_size];
        uint sbuf2[buffer_size];
        quint64 buf1[buffer_size];
        quint64 buf2[buffer_size];
        QRgba64 *b = buffer;

        int distxs[buffer_size / 2];
        int distys[buffer_size / 2];

        while (length) {
            int len = qMin(length, buffer_size / 2);
            for (int i = 0; i < len; ++i) {
                const qreal iw = fw == 0 ? 1 : 1 / fw;
                const qreal px = fx * iw - qreal(0.5);
                const qreal py = fy * iw - qreal(0.5);

                int x1 = qFloor(px);
                int x2;
                int y1 = qFloor(py);
                int y2;

                distxs[i] = int((px - x1) * (1<<16));
                distys[i] = int((py - y1) * (1<<16));

                fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
                fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);

                const uchar *s1 = data->texture.scanLine(y1);
                const uchar *s2 = data->texture.scanLine(y2);

                if (layout->bpp == QPixelLayout::BPP32) {
                    sbuf1[i * 2 + 0] = ((const uint*)s1)[x1];
                    sbuf1[i * 2 + 1] = ((const uint*)s1)[x2];
                    sbuf2[i * 2 + 0] = ((const uint*)s2)[x1];
                    sbuf2[i * 2 + 1] = ((const uint*)s2)[x2];

                } else {
                    sbuf1[i * 2 + 0] = fetch(s1, x1);
                    sbuf1[i * 2 + 1] = fetch(s1, x2);
                    sbuf2[i * 2 + 0] = fetch(s2, x1);
                    sbuf2[i * 2 + 1] = fetch(s2, x2);
                }

                fx += fdx;
                fy += fdy;
                fw += fdw;
                //force increment to avoid /0
                if (!fw)
                    fw += fdw;
            }

            layout->convertToARGB64PM((QRgba64 *)buf1, sbuf1, len * 2, clut, 0);
            layout->convertToARGB64PM((QRgba64 *)buf2, sbuf2, len * 2, clut, 0);

            for (int i = 0; i < len; ++i) {
                int distx = distxs[i];
                int disty = distys[i];
                b[i] = interpolate_4_pixels_rgb64((QRgba64 *)buf1 + i*2, (QRgba64 *)buf2 + i*2, distx, disty);
            }

            length -= len;
            b += len;
        }
    }

    return buffer;
}

// FetchUntransformed can have more specialized methods added depending on SIMD features.
static SourceFetchProc sourceFetchUntransformed[QImage::NImageFormats] = {
    0,                          // Invalid
    fetchUntransformed,         // Mono
    fetchUntransformed,         // MonoLsb
    fetchUntransformed,         // Indexed8
    fetchUntransformedARGB32PM, // RGB32
    fetchUntransformed,         // ARGB32
    fetchUntransformedARGB32PM, // ARGB32_Premultiplied
    fetchUntransformedRGB16,    // RGB16
    fetchUntransformed,         // ARGB8565_Premultiplied
    fetchUntransformed,         // RGB666
    fetchUntransformed,         // ARGB6666_Premultiplied
    fetchUntransformed,         // RGB555
    fetchUntransformed,         // ARGB8555_Premultiplied
    fetchUntransformed,         // RGB888
    fetchUntransformed,         // RGB444
    fetchUntransformed,         // ARGB4444_Premultiplied
    fetchUntransformed,         // RGBX8888
    fetchUntransformed,         // RGBA8888
    fetchUntransformed,         // RGBA8888_Premultiplied
    fetchUntransformed,         // Format_BGR30
    fetchUntransformed,         // Format_A2BGR30_Premultiplied
    fetchUntransformed,         // Format_RGB30
    fetchUntransformed,         // Format_A2RGB30_Premultiplied
    fetchUntransformed,         // Alpha8
    fetchUntransformed,         // Grayscale8
};

static const SourceFetchProc sourceFetchGeneric[NBlendTypes] = {
    fetchUntransformed,                                                             // Untransformed
    fetchUntransformed,                                                             // Tiled
    fetchTransformed<BlendTransformed, QPixelLayout::BPPNone>,                      // Transformed
    fetchTransformed<BlendTransformedTiled, QPixelLayout::BPPNone>,                 // TransformedTiled
    fetchTransformedBilinear<BlendTransformedBilinear, QPixelLayout::BPPNone>,      // TransformedBilinear
    fetchTransformedBilinear<BlendTransformedBilinearTiled, QPixelLayout::BPPNone>  // TransformedBilinearTiled
};

static SourceFetchProc sourceFetchARGB32PM[NBlendTypes] = {
    fetchUntransformedARGB32PM,                                     // Untransformed
    fetchUntransformedARGB32PM,                                     // Tiled
    fetchTransformed<BlendTransformed, QPixelLayout::BPP32>,        // Transformed
    fetchTransformed<BlendTransformedTiled, QPixelLayout::BPP32>,   // TransformedTiled
    fetchTransformedBilinearARGB32PM<BlendTransformedBilinear>,     // Bilinear
    fetchTransformedBilinearARGB32PM<BlendTransformedBilinearTiled> // BilinearTiled
};

static SourceFetchProc sourceFetchAny32[NBlendTypes] = {
    fetchUntransformed,                                                             // Untransformed
    fetchUntransformed,                                                             // Tiled
    fetchTransformed<BlendTransformed, QPixelLayout::BPP32>,                        // Transformed
    fetchTransformed<BlendTransformedTiled, QPixelLayout::BPP32>,                   // TransformedTiled
    fetchTransformedBilinear<BlendTransformedBilinear, QPixelLayout::BPP32>,        // TransformedBilinear
    fetchTransformedBilinear<BlendTransformedBilinearTiled, QPixelLayout::BPP32>    // TransformedBilinearTiled
};

static const SourceFetchProc64 sourceFetchGeneric64[NBlendTypes] = {
    fetchUntransformed64,                                     // Untransformed
    fetchUntransformed64,                                     // Tiled
    fetchTransformed64<BlendTransformed>,                     // Transformed
    fetchTransformed64<BlendTransformedTiled>,                // TransformedTiled
    fetchTransformedBilinear64<BlendTransformedBilinear>,     // Bilinear
    fetchTransformedBilinear64<BlendTransformedBilinearTiled> // BilinearTiled
};

static inline SourceFetchProc getSourceFetch(TextureBlendType blendType, QImage::Format format)
{
    if (format == QImage::Format_RGB32 || format == QImage::Format_ARGB32_Premultiplied)
        return sourceFetchARGB32PM[blendType];
    if (blendType == BlendUntransformed || blendType == BlendTiled)
        return sourceFetchUntransformed[format];
    if (qPixelLayouts[format].bpp == QPixelLayout::BPP32)
        return sourceFetchAny32[blendType];
    return sourceFetchGeneric[blendType];
}


#define FIXPT_BITS 8
#define FIXPT_SIZE (1<<FIXPT_BITS)

static uint qt_gradient_pixel_fixed(const QGradientData *data, int fixed_pos)
{
    int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;
    return data->colorTable32[qt_gradient_clamp(data, ipos)];
}

static const QRgba64& qt_gradient_pixel64_fixed(const QGradientData *data, int fixed_pos)
{
    int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;
    return data->colorTable64[qt_gradient_clamp(data, ipos)];
}

static void QT_FASTCALL getLinearGradientValues(LinearGradientValues *v, const QSpanData *data)
{
    v->dx = data->gradient.linear.end.x - data->gradient.linear.origin.x;
    v->dy = data->gradient.linear.end.y - data->gradient.linear.origin.y;
    v->l = v->dx * v->dx + v->dy * v->dy;
    v->off = 0;
    if (v->l != 0) {
        v->dx /= v->l;
        v->dy /= v->l;
        v->off = -v->dx * data->gradient.linear.origin.x - v->dy * data->gradient.linear.origin.y;
    }
}

class GradientBase32
{
public:
    typedef uint Type;
    static Type null() { return 0; }
    static Type fetchSingle(const QGradientData& gradient, qreal v)
    {
        return qt_gradient_pixel(&gradient, v);
    }
    static Type fetchSingle(const QGradientData& gradient, int v)
    {
        return qt_gradient_pixel_fixed(&gradient, v);
    }
    static void memfill(Type *buffer, Type fill, int length)
    {
        qt_memfill32(buffer, fill, length);
    }
};

class GradientBase64
{
public:
    typedef QRgba64 Type;
    static Type null() { return QRgba64::fromRgba64(0); }
    static Type fetchSingle(const QGradientData& gradient, qreal v)
    {
        return qt_gradient_pixel64(&gradient, v);
    }
    static Type fetchSingle(const QGradientData& gradient, int v)
    {
        return qt_gradient_pixel64_fixed(&gradient, v);
    }
    static void memfill(Type *buffer, Type fill, int length)
    {
        qt_memfill64((quint64*)buffer, fill, length);
    }
};

template<class GradientBase, typename BlendType>
static inline const BlendType * QT_FASTCALL qt_fetch_linear_gradient_template(
        BlendType *buffer, const Operator *op, const QSpanData *data,
        int y, int x, int length)
{
    const BlendType *b = buffer;
    qreal t, inc;

    bool affine = true;
    qreal rx=0, ry=0;
    if (op->linear.l == 0) {
        t = inc = 0;
    } else {
        rx = data->m21 * (y + qreal(0.5)) + data->m11 * (x + qreal(0.5)) + data->dx;
        ry = data->m22 * (y + qreal(0.5)) + data->m12 * (x + qreal(0.5)) + data->dy;
        t = op->linear.dx*rx + op->linear.dy*ry + op->linear.off;
        inc = op->linear.dx * data->m11 + op->linear.dy * data->m12;
        affine = !data->m13 && !data->m23;

        if (affine) {
            t *= (GRADIENT_STOPTABLE_SIZE - 1);
            inc *= (GRADIENT_STOPTABLE_SIZE - 1);
        }
    }

    const BlendType *end = buffer + length;
    if (affine) {
        if (inc > qreal(-1e-5) && inc < qreal(1e-5)) {
            GradientBase::memfill(buffer, GradientBase::fetchSingle(data->gradient, int(t * FIXPT_SIZE)), length);
        } else {
            if (t+inc*length < qreal(INT_MAX >> (FIXPT_BITS + 1)) &&
                t+inc*length > qreal(INT_MIN >> (FIXPT_BITS + 1))) {
                // we can use fixed point math
                int t_fixed = int(t * FIXPT_SIZE);
                int inc_fixed = int(inc * FIXPT_SIZE);
                while (buffer < end) {
                    *buffer = GradientBase::fetchSingle(data->gradient, t_fixed);
                    t_fixed += inc_fixed;
                    ++buffer;
                }
            } else {
                // we have to fall back to float math
                while (buffer < end) {
                    *buffer = GradientBase::fetchSingle(data->gradient, t/GRADIENT_STOPTABLE_SIZE);
                    t += inc;
                    ++buffer;
                }
            }
        }
    } else { // fall back to float math here as well
        qreal rw = data->m23 * (y + qreal(0.5)) + data->m13 * (x + qreal(0.5)) + data->m33;
        while (buffer < end) {
            qreal x = rx/rw;
            qreal y = ry/rw;
            t = (op->linear.dx*x + op->linear.dy *y) + op->linear.off;

            *buffer = GradientBase::fetchSingle(data->gradient, t);
            rx += data->m11;
            ry += data->m12;
            rw += data->m13;
            if (!rw) {
                rw += data->m13;
            }
            ++buffer;
        }
    }

    return b;
}

static const uint * QT_FASTCALL qt_fetch_linear_gradient(uint *buffer, const Operator *op, const QSpanData *data,
                                                         int y, int x, int length)
{
    return qt_fetch_linear_gradient_template<GradientBase32, uint>(buffer, op, data, y, x, length);
}

static const QRgba64 * QT_FASTCALL qt_fetch_linear_gradient_rgb64(QRgba64 *buffer, const Operator *op, const QSpanData *data,
                                                                 int y, int x, int length)
{
    return qt_fetch_linear_gradient_template<GradientBase64, QRgba64>(buffer, op, data, y, x, length);
}

static void QT_FASTCALL getRadialGradientValues(RadialGradientValues *v, const QSpanData *data)
{
    v->dx = data->gradient.radial.center.x - data->gradient.radial.focal.x;
    v->dy = data->gradient.radial.center.y - data->gradient.radial.focal.y;

    v->dr = data->gradient.radial.center.radius - data->gradient.radial.focal.radius;
    v->sqrfr = data->gradient.radial.focal.radius * data->gradient.radial.focal.radius;

    v->a = v->dr * v->dr - v->dx*v->dx - v->dy*v->dy;
    v->inv2a = 1 / (2 * v->a);

    v->extended = !qFuzzyIsNull(data->gradient.radial.focal.radius) || v->a <= 0;
}

template <class GradientBase>
class RadialFetchPlain : public GradientBase
{
public:
    typedef typename GradientBase::Type BlendType;
    static void fetch(BlendType *buffer, BlendType *end,
                      const Operator *op, const QSpanData *data, qreal det,
                      qreal delta_det, qreal delta_delta_det, qreal b, qreal delta_b)
    {
        if (op->radial.extended) {
            while (buffer < end) {
                BlendType result = GradientBase::null();
                if (det >= 0) {
                    qreal w = qSqrt(det) - b;
                    if (data->gradient.radial.focal.radius + op->radial.dr * w >= 0)
                        result = GradientBase::fetchSingle(data->gradient, w);
                }

                *buffer = result;

                det += delta_det;
                delta_det += delta_delta_det;
                b += delta_b;

                ++buffer;
            }
        } else {
            while (buffer < end) {
                *buffer++ = GradientBase::fetchSingle(data->gradient, qSqrt(det) - b);

                det += delta_det;
                delta_det += delta_delta_det;
                b += delta_b;
            }
        }
    }
};

const uint * QT_FASTCALL qt_fetch_radial_gradient_plain(uint *buffer, const Operator *op, const QSpanData *data,
                                                        int y, int x, int length)
{
    return qt_fetch_radial_gradient_template<RadialFetchPlain<GradientBase32>, uint>(buffer, op, data, y, x, length);
}

static SourceFetchProc qt_fetch_radial_gradient = qt_fetch_radial_gradient_plain;

const QRgba64 * QT_FASTCALL qt_fetch_radial_gradient_rgb64(QRgba64 *buffer, const Operator *op, const QSpanData *data,
                                                        int y, int x, int length)
{
    return qt_fetch_radial_gradient_template<RadialFetchPlain<GradientBase64>, QRgba64>(buffer, op, data, y, x, length);
}

template <class GradientBase, typename BlendType>
static inline const BlendType * QT_FASTCALL qt_fetch_conical_gradient_template(
        BlendType *buffer, const QSpanData *data,
        int y, int x, int length)
{
    const BlendType *b = buffer;
    qreal rx = data->m21 * (y + qreal(0.5))
               + data->dx + data->m11 * (x + qreal(0.5));
    qreal ry = data->m22 * (y + qreal(0.5))
               + data->dy + data->m12 * (x + qreal(0.5));
    bool affine = !data->m13 && !data->m23;

    const qreal inv2pi = M_1_PI / 2.0;

    const BlendType *end = buffer + length;
    if (affine) {
        rx -= data->gradient.conical.center.x;
        ry -= data->gradient.conical.center.y;
        while (buffer < end) {
            qreal angle = qAtan2(ry, rx) + data->gradient.conical.angle;

            *buffer = GradientBase::fetchSingle(data->gradient, 1 - angle * inv2pi);

            rx += data->m11;
            ry += data->m12;
            ++buffer;
        }
    } else {
        qreal rw = data->m23 * (y + qreal(0.5))
                   + data->m33 + data->m13 * (x + qreal(0.5));
        if (!rw)
            rw = 1;
        while (buffer < end) {
            qreal angle = qAtan2(ry/rw - data->gradient.conical.center.x,
                                rx/rw - data->gradient.conical.center.y)
                          + data->gradient.conical.angle;

            *buffer = GradientBase::fetchSingle(data->gradient, 1 - angle * inv2pi);

            rx += data->m11;
            ry += data->m12;
            rw += data->m13;
            if (!rw) {
                rw += data->m13;
            }
            ++buffer;
        }
    }
    return b;
}

static const uint * QT_FASTCALL qt_fetch_conical_gradient(uint *buffer, const Operator *, const QSpanData *data,
                                                          int y, int x, int length)
{
    return qt_fetch_conical_gradient_template<GradientBase32, uint>(buffer, data, y, x, length);
}

static const QRgba64 * QT_FASTCALL qt_fetch_conical_gradient_rgb64(QRgba64 *buffer, const Operator *, const QSpanData *data,
                                                                   int y, int x, int length)
{
    return qt_fetch_conical_gradient_template<GradientBase64, QRgba64>(buffer, data, y, x, length);
}

extern CompositionFunctionSolid qt_functionForModeSolid_C[];
extern CompositionFunctionSolid64 qt_functionForModeSolid64_C[];

static const CompositionFunctionSolid *functionForModeSolid = qt_functionForModeSolid_C;
static const CompositionFunctionSolid64 *functionForModeSolid64 = qt_functionForModeSolid64_C;

extern CompositionFunction qt_functionForMode_C[];
extern CompositionFunction64 qt_functionForMode64_C[];

static const CompositionFunction *functionForMode = qt_functionForMode_C;
static const CompositionFunction64 *functionForMode64 = qt_functionForMode64_C;

static TextureBlendType getBlendType(const QSpanData *data)
{
    TextureBlendType ft;
    if (data->txop <= QTransform::TxTranslate)
        if (data->texture.type == QTextureData::Tiled)
            ft = BlendTiled;
        else
            ft = BlendUntransformed;
    else if (data->bilinear)
        if (data->texture.type == QTextureData::Tiled)
            ft = BlendTransformedBilinearTiled;
        else
            ft = BlendTransformedBilinear;
    else
        if (data->texture.type == QTextureData::Tiled)
            ft = BlendTransformedTiled;
        else
            ft = BlendTransformed;
    return ft;
}

static inline Operator getOperator(const QSpanData *data, const QSpan *spans, int spanCount)
{
    Operator op;
    bool solidSource = false;

    switch(data->type) {
    case QSpanData::Solid:
        solidSource = data->solid.color.isOpaque();
        op.srcFetch = 0;
        op.srcFetch64 = 0;
        break;
    case QSpanData::LinearGradient:
        solidSource = !data->gradient.alphaColor;
        getLinearGradientValues(&op.linear, data);
        op.srcFetch = qt_fetch_linear_gradient;
        op.srcFetch64 = qt_fetch_linear_gradient_rgb64;
        break;
    case QSpanData::RadialGradient:
        solidSource = !data->gradient.alphaColor;
        getRadialGradientValues(&op.radial, data);
        op.srcFetch = qt_fetch_radial_gradient;
        op.srcFetch64 = qt_fetch_radial_gradient_rgb64;
        break;
    case QSpanData::ConicalGradient:
        solidSource = !data->gradient.alphaColor;
        op.srcFetch = qt_fetch_conical_gradient;
        op.srcFetch64 = qt_fetch_conical_gradient_rgb64;
        break;
    case QSpanData::Texture:
        solidSource = !data->texture.hasAlpha;
        op.srcFetch = getSourceFetch(getBlendType(data), data->texture.format);
        op.srcFetch64 = sourceFetchGeneric64[getBlendType(data)];
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    op.mode = data->rasterBuffer->compositionMode;
    if (op.mode == QPainter::CompositionMode_SourceOver && solidSource)
        op.mode = QPainter::CompositionMode_Source;

    op.destFetch = destFetchProc[data->rasterBuffer->format];
    op.destFetch64 = destFetchProc64[data->rasterBuffer->format];
    if (op.mode == QPainter::CompositionMode_Source &&
            (data->type != QSpanData::Texture || data->texture.const_alpha == 256)) {
        const QSpan *lastSpan = spans + spanCount;
        bool alphaSpans = false;
        while (spans < lastSpan) {
            if (spans->coverage != 255) {
                alphaSpans = true;
                break;
            }
            ++spans;
        }
        if (!alphaSpans) {
            // If all spans are opaque we do not need to fetch dest.
            // But don't clear passthrough destFetch as they are just as fast and save destStore.
            if (op.destFetch != destFetchARGB32P)
                op.destFetch = 0;
            op.destFetch64 = destFetch64Undefined;
        }
    }

    op.destStore = destStoreProc[data->rasterBuffer->format];
    op.destStore64 = destStoreProc64[data->rasterBuffer->format];

    op.funcSolid = functionForModeSolid[op.mode];
    op.funcSolid64 = functionForModeSolid64[op.mode];
    op.func = functionForMode[op.mode];
    op.func64 = functionForMode64[op.mode];

    return op;
}



// -------------------- blend methods ---------------------

#if !defined(Q_CC_SUN)
static
#endif
void blend_color_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    uint buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    const uint color = data->solid.color.toArgb32();

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(buffer_size, length);
            uint *dest = op.destFetch ? op.destFetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
            op.funcSolid(dest, l, color, spans->coverage);
            if (op.destStore)
                op.destStore(data->rasterBuffer, x, spans->y, dest, l);
            length -= l;
            x += l;
        }
        ++spans;
    }
}

static void blend_color_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    Operator op = getOperator(data, spans, count);
    const uint color = data->solid.color.toArgb32();

    if (op.mode == QPainter::CompositionMode_Source) {
        // inline for performance
        while (count--) {
            uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                QT_MEMFILL_UINT(target, spans->len, color);
            } else {
                uint c = BYTE_MUL(color, spans->coverage);
                int ialpha = 255 - spans->coverage;
                for (int i = 0; i < spans->len; ++i)
                    target[i] = c + BYTE_MUL(target[i], ialpha);
            }
            ++spans;
        }
        return;
    }

    while (count--) {
        uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
        op.funcSolid(target, spans->len, color, spans->coverage);
        ++spans;
    }
}

void blend_color_generic_rgb64(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    Operator op = getOperator(data, spans, count);
    if (!op.funcSolid64) {
        qCDebug(lcQtGuiDrawHelper, "blend_color_generic_rgb64: unsupported 64bit blend attempted, falling back to 32-bit");
        return blend_color_generic(count, spans, userData);
    }

    quint64 buffer[buffer_size];
    const QRgba64 color = data->solid.color;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(buffer_size, length);
            QRgba64 *dest = op.destFetch64((QRgba64 *)buffer, data->rasterBuffer, x, spans->y, l);
            op.funcSolid64(dest, l, color, spans->coverage);
            op.destStore64(data->rasterBuffer, x, spans->y, dest, l);
            length -= l;
            x += l;
        }
        ++spans;
    }
}

static void blend_color_rgb16(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    /*
        We duplicate a little logic from getOperator() and calculate the
        composition mode directly.  This allows blend_color_rgb16 to be used
        from qt_gradient_quint16 with minimal overhead.
     */
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;
    if (mode == QPainter::CompositionMode_SourceOver && data->solid.color.isOpaque())
        mode = QPainter::CompositionMode_Source;

    if (mode == QPainter::CompositionMode_Source) {
        // inline for performance
        ushort c = data->solid.color.toRgb16();
        while (count--) {
            ushort *target = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                QT_MEMFILL_USHORT(target, spans->len, c);
            } else {
                ushort color = BYTE_MUL_RGB16(c, spans->coverage);
                int ialpha = 255 - spans->coverage;
                const ushort *end = target + spans->len;
                while (target < end) {
                    *target = color + BYTE_MUL_RGB16(*target, ialpha);
                    ++target;
                }
            }
            ++spans;
        }
        return;
    }

    if (mode == QPainter::CompositionMode_SourceOver) {
        while (count--) {
            uint color = BYTE_MUL(data->solid.color.toArgb32(), spans->coverage);
            int ialpha = qAlpha(~color);
            ushort c = qConvertRgb32To16(color);
            ushort *target = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            int len = spans->len;
            bool pre = (((quintptr)target) & 0x3) != 0;
            bool post = false;
            if (pre) {
                // skip to word boundary
                *target = c + BYTE_MUL_RGB16(*target, ialpha);
                ++target;
                --len;
            }
            if (len & 0x1) {
                post = true;
                --len;
            }
            uint *target32 = (uint*)target;
            uint c32 = c | (c<<16);
            len >>= 1;
            uint salpha = (ialpha+1) >> 3; // calculate here rather than in loop
            while (len--) {
                // blend full words
                *target32 = c32 + BYTE_MUL_RGB16_32(*target32, salpha);
                ++target32;
                target += 2;
            }
            if (post) {
                // one last pixel beyond a full word
                *target = c + BYTE_MUL_RGB16(*target, ialpha);
            }
            ++spans;
        }
        return;
    }

    blend_color_generic(count, spans, userData);
}

template <typename T>
void handleSpans(int count, const QSpan *spans, const QSpanData *data, T &handler)
{
    uint const_alpha = 256;
    if (data->type == QSpanData::Texture)
        const_alpha = data->texture.const_alpha;

    int coverage = 0;
    while (count) {
        int x = spans->x;
        const int y = spans->y;
        int right = x + spans->len;

        // compute length of adjacent spans
        for (int i = 1; i < count && spans[i].y == y && spans[i].x == right; ++i)
            right += spans[i].len;
        int length = right - x;

        while (length) {
            int l = qMin(buffer_size, length);
            length -= l;

            int process_length = l;
            int process_x = x;

            const typename T::BlendType *src = handler.fetch(process_x, y, process_length);
            int offset = 0;
            while (l > 0) {
                if (x == spans->x) // new span?
                    coverage = (spans->coverage * const_alpha) >> 8;

                int right = spans->x + spans->len;
                int len = qMin(l, right - x);

                handler.process(x, y, len, coverage, src, offset);

                l -= len;
                x += len;
                offset += len;

                if (x == right) { // done with current span?
                    ++spans;
                    --count;
                }
            }
            handler.store(process_x, y, process_length);
        }
    }
}

template<typename T>
struct QBlendBase
{
    typedef T BlendType;
    QBlendBase(QSpanData *d, const Operator &o)
        : data(d)
        , op(o)
        , dest(0)
    {
    }

    QSpanData *data;
    Operator op;

    BlendType *dest;

    BlendType buffer[buffer_size];
    BlendType src_buffer[buffer_size];
};

class BlendSrcGeneric : public QBlendBase<uint>
{
public:
    BlendSrcGeneric(QSpanData *d, const Operator &o)
        : QBlendBase<uint>(d, o)
    {
    }

    const uint *fetch(int x, int y, int len)
    {
        dest = op.destFetch ? op.destFetch(buffer, data->rasterBuffer, x, y, len) : buffer;
        return op.srcFetch(src_buffer, &op, data, y, x, len);
    }

    void process(int, int, int len, int coverage, const uint *src, int offset)
    {
        op.func(dest + offset, src + offset, len, coverage);
    }

    void store(int x, int y, int len)
    {
        if (op.destStore)
            op.destStore(data->rasterBuffer, x, y, dest, len);
    }
};

class BlendSrcGenericRGB64 : public QBlendBase<quint64>
{
public:
    BlendSrcGenericRGB64(QSpanData *d, const Operator &o)
        : QBlendBase<quint64>(d, o)
    {
    }

    bool isSupported() const
    {
        return op.func64 && op.destFetch64 && op.destStore64;
    }

    const quint64 *fetch(int x, int y, int len)
    {
        dest = (quint64 *)op.destFetch64((QRgba64 *)buffer, data->rasterBuffer, x, y, len);
        return (const quint64 *)op.srcFetch64((QRgba64 *)src_buffer, &op, data, y, x, len);
    }

    void process(int, int, int len, int coverage, const quint64 *src, int offset)
    {
        op.func64((QRgba64 *)dest + offset, (const QRgba64 *)src + offset, len, coverage);
    }

    void store(int x, int y, int len)
    {
        op.destStore64(data->rasterBuffer, x, y, (QRgba64 *)dest, len);
    }
};

static void blend_src_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    BlendSrcGeneric blend(data, getOperator(data, spans, count));
    handleSpans(count, spans, data, blend);
}

static void blend_src_generic_rgb64(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    Operator op = getOperator(data, spans, count);
    BlendSrcGenericRGB64 blend64(data, op);
    if (blend64.isSupported())
        handleSpans(count, spans, data, blend64);
    else {
        qCDebug(lcQtGuiDrawHelper, "blend_src_generic_rgb64: unsupported 64-bit blend attempted, falling back to 32-bit");
        BlendSrcGeneric blend32(data, op);
        handleSpans(count, spans, data, blend32);
    }
}

static void blend_untransformed_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                while (length) {
                    int l = qMin(buffer_size, length);
                    const uint *src = op.srcFetch(src_buffer, &op, data, sy, sx, l);
                    uint *dest = op.destFetch ? op.destFetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
                    op.func(dest, src, l, coverage);
                    if (op.destStore)
                        op.destStore(data->rasterBuffer, x, spans->y, dest, l);
                    x += l;
                    sx += l;
                    length -= l;
                }
            }
        }
        ++spans;
    }
}

static void blend_untransformed_generic_rgb64(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    Operator op = getOperator(data, spans, count);
    if (!op.func64) {
        qCDebug(lcQtGuiDrawHelper, "blend_untransformed_generic_rgb64: unsupported 64-bit blend attempted, falling back to 32-bit");
        return blend_untransformed_generic(count, spans, userData);
    }
    quint64 buffer[buffer_size];
    quint64 src_buffer[buffer_size];

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                while (length) {
                    int l = qMin(buffer_size, length);
                    const QRgba64 *src = op.srcFetch64((QRgba64 *)src_buffer, &op, data, sy, sx, l);
                    QRgba64 *dest = op.destFetch64((QRgba64 *)buffer, data->rasterBuffer, x, spans->y, l);
                    op.func64(dest, src, l, coverage);
                    op.destStore64(data->rasterBuffer, x, spans->y, dest, l);
                    x += l;
                    sx += l;
                    length -= l;
                }
            }
        }
        ++spans;
    }
}

static void blend_untransformed_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_untransformed_generic(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                const uint *src = (const uint *)data->texture.scanLine(sy) + sx;
                uint *dest = ((uint *)data->rasterBuffer->scanLine(spans->y)) + x;
                op.func(dest, src, length, coverage);
            }
        }
        ++spans;
    }
}

static inline quint16 interpolate_pixel_rgb16_255(quint16 x, quint8 a,
                                                  quint16 y, quint8 b)
{
    quint16 t = ((((x & 0x07e0) * a) + ((y & 0x07e0) * b)) >> 5) & 0x07e0;
    t |= ((((x & 0xf81f) * a) + ((y & 0xf81f) * b)) >> 5) & 0xf81f;

    return t;
}

static inline quint32 interpolate_pixel_rgb16x2_255(quint32 x, quint8 a,
                                                    quint32 y, quint8 b)
{
    uint t;
    t = ((((x & 0xf81f07e0) >> 5) * a) + (((y & 0xf81f07e0) >> 5) * b)) & 0xf81f07e0;
    t |= ((((x & 0x07e0f81f) * a) + ((y & 0x07e0f81f) * b)) >> 5) & 0x07e0f81f;
    return t;
}

static inline void blend_sourceOver_rgb16_rgb16(quint16 *Q_DECL_RESTRICT dest,
                                                const quint16 *Q_DECL_RESTRICT src,
                                                int length,
                                                const quint8 alpha,
                                                const quint8 ialpha)
{
    const int dstAlign = ((quintptr)dest) & 0x3;
    if (dstAlign) {
        *dest = interpolate_pixel_rgb16_255(*src, alpha, *dest, ialpha);
        ++dest;
        ++src;
        --length;
    }
    const int srcAlign = ((quintptr)src) & 0x3;
    int length32 = length >> 1;
    if (length32 && srcAlign == 0) {
        while (length32--) {
            const quint32 *src32 = reinterpret_cast<const quint32*>(src);
            quint32 *dest32 = reinterpret_cast<quint32*>(dest);
            *dest32 = interpolate_pixel_rgb16x2_255(*src32, alpha,
                                                    *dest32, ialpha);
            dest += 2;
            src += 2;
        }
        length &= 0x1;
    }
    while (length--) {
        *dest = interpolate_pixel_rgb16_255(*src, alpha, *dest, ialpha);
        ++dest;
        ++src;
    }
}

static void blend_untransformed_rgb565(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (data->texture.format != QImage::Format_RGB16
            || (mode != QPainter::CompositionMode_SourceOver
                && mode != QPainter::CompositionMode_Source))
    {
        blend_untransformed_generic(count, spans, userData);
        return;
    }

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    while (count--) {
        const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
        if (coverage == 0) {
            ++spans;
            continue;
        }

        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + x;
                const quint16 *src = (const quint16 *)data->texture.scanLine(sy) + sx;
                if (coverage == 255) {
                    memcpy(dest, src, length * sizeof(quint16));
                } else {
                    const quint8 alpha = (coverage + 1) >> 3;
                    const quint8 ialpha = 0x20 - alpha;
                    if (alpha > 0)
                        blend_sourceOver_rgb16_rgb16(dest, src, length, alpha, ialpha);
                }
            }
        }
        ++spans;
    }
}

static void blend_tiled_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const uint *src = op.srcFetch(src_buffer, &op, data, sy, sx, l);
            uint *dest = op.destFetch ? op.destFetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
            op.func(dest, src, l, coverage);
            if (op.destStore)
                op.destStore(data->rasterBuffer, x, spans->y, dest, l);
            x += l;
            sx += l;
            length -= l;
            if (sx >= image_width)
                sx = 0;
        }
        ++spans;
    }
}

static void blend_tiled_generic_rgb64(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    Operator op = getOperator(data, spans, count);
    if (!op.func64) {
        qCDebug(lcQtGuiDrawHelper, "blend_tiled_generic_rgb64: unsupported 64-bit blend attempted, falling back to 32-bit");
        return blend_tiled_generic(count, spans, userData);
    }
    quint64 buffer[buffer_size];
    quint64 src_buffer[buffer_size];

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const QRgba64 *src = op.srcFetch64((QRgba64 *)src_buffer, &op, data, sy, sx, l);
            QRgba64 *dest = op.destFetch64((QRgba64 *)buffer, data->rasterBuffer, x, spans->y, l);
            op.func64(dest, src, l, coverage);
            op.destStore64(data->rasterBuffer, x, spans->y, dest, l);
            x += l;
            sx += l;
            length -= l;
            if (sx >= image_width)
                sx = 0;
        }
        ++spans;
    }
}

static void blend_tiled_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_tiled_generic(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const uint *src = (const uint *)data->texture.scanLine(sy) + sx;
            uint *dest = ((uint *)data->rasterBuffer->scanLine(spans->y)) + x;
            op.func(dest, src, l, coverage);
            x += l;
            sx += l;
            length -= l;
            if (sx >= image_width)
                sx = 0;
        }
        ++spans;
    }
}

static void blend_tiled_rgb565(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (data->texture.format != QImage::Format_RGB16
            || (mode != QPainter::CompositionMode_SourceOver
                && mode != QPainter::CompositionMode_Source))
    {
        blend_tiled_generic(count, spans, userData);
        return;
    }

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
        if (coverage == 0) {
            ++spans;
            continue;
        }

        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        if (coverage == 255) {
            // Copy the first texture block
            length = qMin(image_width,length);
            int tx = x;
            while (length) {
                int l = qMin(image_width - sx, length);
                if (buffer_size < l)
                    l = buffer_size;
                quint16 *dest = ((quint16 *)data->rasterBuffer->scanLine(spans->y)) + tx;
                const quint16 *src = (const quint16 *)data->texture.scanLine(sy) + sx;
                memcpy(dest, src, l * sizeof(quint16));
                length -= l;
                tx += l;
                sx += l;
                if (sx >= image_width)
                    sx = 0;
            }

            // Now use the rasterBuffer as the source of the texture,
            // We can now progressively copy larger blocks
            // - Less cpu time in code figuring out what to copy
            // We are dealing with one block of data
            // - More likely to fit in the cache
            // - can use memcpy
            int copy_image_width = qMin(image_width, int(spans->len));
            length = spans->len - copy_image_width;
            quint16 *src = ((quint16 *)data->rasterBuffer->scanLine(spans->y)) + x;
            quint16 *dest = src + copy_image_width;
            while (copy_image_width < length) {
                memcpy(dest, src, copy_image_width * sizeof(quint16));
                dest += copy_image_width;
                length -= copy_image_width;
                copy_image_width *= 2;
            }
            if (length > 0)
                memcpy(dest, src, length * sizeof(quint16));
        } else {
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha > 0) {
                while (length) {
                    int l = qMin(image_width - sx, length);
                    if (buffer_size < l)
                        l = buffer_size;
                    quint16 *dest = ((quint16 *)data->rasterBuffer->scanLine(spans->y)) + x;
                    const quint16 *src = (const quint16 *)data->texture.scanLine(sy) + sx;
                    blend_sourceOver_rgb16_rgb16(dest, src, l, alpha, ialpha);
                    x += l;
                    sx += l;
                    length -= l;
                    if (sx >= image_width)
                        sx = 0;
                }
            }
        }
        ++spans;
    }
}

static void blend_transformed_bilinear_rgb565(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (data->texture.format != QImage::Format_RGB16
            || (mode != QPainter::CompositionMode_SourceOver
                && mode != QPainter::CompositionMode_Source))
    {
        blend_src_generic(count, spans, userData);
        return;
    }

    quint16 buffer[buffer_size];

    const int src_minx = data->texture.x1;
    const int src_miny = data->texture.y1;
    const int src_maxx = data->texture.x2 - 1;
    const int src_maxy = data->texture.y2 - 1;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;
            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);
            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale) - half_point;
            int length = spans->len;

            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    int x1 = (x >> 16);
                    int x2;
                    int y1 = (y >> 16);
                    int y2;

                    fetchTransformedBilinear_pixelBounds<BlendTransformedBilinear>(0, src_minx, src_maxx, x1, x2);
                    fetchTransformedBilinear_pixelBounds<BlendTransformedBilinear>(0, src_miny, src_maxy, y1, y2);

                    const quint16 *src1 = (const quint16*)data->texture.scanLine(y1);
                    const quint16 *src2 = (const quint16*)data->texture.scanLine(y2);
                    quint16 tl = src1[x1];
                    const quint16 tr = src1[x2];
                    quint16 bl = src2[x1];
                    const quint16 br = src2[x2];

                    const uint distxsl8 = x & 0xff00;
                    const uint distysl8 = y & 0xff00;
                    const uint distx = distxsl8 >> 8;
                    const uint disty = distysl8 >> 8;
                    const uint distxy = distx * disty;

                    const uint tlw = 0x10000 - distxsl8 - distysl8 + distxy; // (256 - distx) * (256 - disty)
                    const uint trw = distxsl8 - distxy; // distx * (256 - disty)
                    const uint blw = distysl8 - distxy; // (256 - distx) * disty
                    const uint brw = distxy; // distx * disty
                    uint red = ((tl & 0xf800) * tlw + (tr & 0xf800) * trw
                            + (bl & 0xf800) * blw + (br & 0xf800) * brw) & 0xf8000000;
                    uint green = ((tl & 0x07e0) * tlw + (tr & 0x07e0) * trw
                            + (bl & 0x07e0) * blw + (br & 0x07e0) * brw) & 0x07e00000;
                    uint blue = ((tl & 0x001f) * tlw + (tr & 0x001f) * trw
                            + (bl & 0x001f) * blw + (br & 0x001f) * brw);
                    *b = quint16((red | green | blue) >> 16);

                    ++b;
                    x += fdx;
                    y += fdy;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal px = x * iw - qreal(0.5);
                    const qreal py = y * iw - qreal(0.5);

                    int x1 = int(px) - (px < 0);
                    int x2;
                    int y1 = int(py) - (py < 0);
                    int y2;

                    fetchTransformedBilinear_pixelBounds<BlendTransformedBilinear>(0, src_minx, src_maxx, x1, x2);
                    fetchTransformedBilinear_pixelBounds<BlendTransformedBilinear>(0, src_miny, src_maxy, y1, y2);

                    const quint16 *src1 = (const quint16 *)data->texture.scanLine(y1);
                    const quint16 *src2 = (const quint16 *)data->texture.scanLine(y2);
                    quint16 tl = src1[x1];
                    const quint16 tr = src1[x2];
                    quint16 bl = src2[x1];
                    const quint16 br = src2[x2];

                    const uint distx = uint((px - x1) * 256);
                    const uint disty = uint((py - y1) * 256);
                    const uint distxsl8 = distx << 8;
                    const uint distysl8 = disty << 8;
                    const uint distxy = distx * disty;

                    const uint tlw = 0x10000 - distxsl8 - distysl8 + distxy; // (256 - distx) * (256 - disty)
                    const uint trw = distxsl8 - distxy; // distx * (256 - disty)
                    const uint blw = distysl8 - distxy; // (256 - distx) * disty
                    const uint brw = distxy; // distx * disty
                    uint red = ((tl & 0xf800) * tlw + (tr & 0xf800) * trw
                            + (bl & 0xf800) * blw + (br & 0xf800) * brw) & 0xf8000000;
                    uint green = ((tl & 0x07e0) * tlw + (tr & 0x07e0) * trw
                            + (bl & 0x07e0) * blw + (br & 0x07e0) * brw) & 0x07e00000;
                    uint blue = ((tl & 0x001f) * tlw + (tr & 0x001f) * trw
                            + (bl & 0x001f) * blw + (br & 0x001f) * brw);
                    *b = quint16((red | green | blue) >> 16);

                    ++b;
                    x += fdx;
                    y += fdy;
                    w += fdw;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    uint buffer[buffer_size];
    quint32 mask = (data->texture.format == QImage::Format_RGB32) ? 0xff000000 : 0;

    const int image_x1 = data->texture.x1;
    const int image_y1 = data->texture.y1;
    const int image_x2 = data->texture.x2 - 1;
    const int image_y2 = data->texture.y2 - 1;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = qBound(image_x1, x >> 16, image_x2);
                    int py = qBound(image_y1, y >> 16, image_y2);
                    *b = reinterpret_cast<const uint *>(data->texture.scanLine(py))[px] | mask;

                    x += fdx;
                    y += fdy;
                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;
                    const int px = qBound(image_x1, int(tx) - (tx < 0), image_x2);
                    const int py = qBound(image_y1, int(ty) - (ty < 0), image_y2);

                    *b = reinterpret_cast<const uint *>(data->texture.scanLine(py))[px] | mask;
                    x += fdx;
                    y += fdy;
                    w += fdw;

                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_rgb565(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (data->texture.format != QImage::Format_RGB16
            || (mode != QPainter::CompositionMode_SourceOver
                && mode != QPainter::CompositionMode_Source))
    {
        blend_src_generic(count, spans, userData);
        return;
    }

    quint16 buffer[buffer_size];
    const int image_x1 = data->texture.x1;
    const int image_y1 = data->texture.y1;
    const int image_x2 = data->texture.x2 - 1;
    const int image_y2 = data->texture.y2 - 1;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;
            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);
            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);
            int length = spans->len;

            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    const int px = qBound(image_x1, x >> 16, image_x2);
                    const int py = qBound(image_y1, y >> 16, image_y2);

                    *b = ((const quint16 *)data->texture.scanLine(py))[px];
                    ++b;

                    x += fdx;
                    y += fdy;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;

                    const int px = qBound(image_x1, int(tx) - (tx < 0), image_x2);
                    const int py = qBound(image_y1, int(ty) - (ty < 0), image_y2);

                    *b = ((const quint16 *)data->texture.scanLine(py))[px];
                    ++b;

                    x += fdx;
                    y += fdy;
                    w += fdw;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_tiled_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    const qsizetype scanline_offset = data->texture.bytesPerLine / 4;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            const uint *image_bits = (const uint *)data->texture.imageData;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);

            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            int length = spans->len;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                int px16 = x % (image_width << 16);
                int py16 = y % (image_height << 16);
                int px_delta = fdx % (image_width << 16);
                int py_delta = fdy % (image_height << 16);
                while (b < end) {
                    if (px16 < 0) px16 += image_width << 16;
                    if (py16 < 0) py16 += image_height << 16;
                    int px = px16 >> 16;
                    int py = py16 >> 16;
                    int y_offset = py * scanline_offset;

                    Q_ASSERT(px >= 0 && px < image_width);
                    Q_ASSERT(py >= 0 && py < image_height);

                    *b = image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    px16 += px_delta;
                    if (px16 >= image_width << 16)
                        px16 -= image_width << 16;
                    py16 += py_delta;
                    if (py16 >= image_height << 16)
                        py16 -= image_height << 16;
                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            const uint *image_bits = (const uint *)data->texture.imageData;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            int length = spans->len;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;
                    int px = int(tx) - (tx < 0);
                    int py = int(ty) - (ty < 0);

                    px %= image_width;
                    py %= image_height;
                    if (px < 0) px += image_width;
                    if (py < 0) py += image_height;
                    int y_offset = py * scanline_offset;

                    Q_ASSERT(px >= 0 && px < image_width);
                    Q_ASSERT(py >= 0 && py < image_height);

                    *b = image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }
                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_tiled_rgb565(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (data->texture.format != QImage::Format_RGB16
            || (mode != QPainter::CompositionMode_SourceOver
                && mode != QPainter::CompositionMode_Source))
    {
        blend_src_generic(count, spans, userData);
        return;
    }

    quint16 buffer[buffer_size];
    const int image_width = data->texture.width;
    const int image_height = data->texture.height;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;
            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);
            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);
            int length = spans->len;

            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    int px = (x >> 16) % image_width;
                    int py = (y >> 16) % image_height;

                    if (px < 0)
                        px += image_width;
                    if (py < 0)
                        py += image_height;

                    *b = ((const quint16 *)data->texture.scanLine(py))[px];
                    ++b;

                    x += fdx;
                    y += fdy;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;

                    int px = int(tx) - (tx < 0);
                    int py = int(ty) - (ty < 0);

                    px %= image_width;
                    py %= image_height;
                    if (px < 0)
                        px += image_width;
                    if (py < 0)
                        py += image_height;

                    *b = ((const quint16 *)data->texture.scanLine(py))[px];
                    ++b;

                    x += fdx;
                    y += fdy;
                    w += fdw;
                    // force increment to avoid /0
                    if (!w)
                        w += fdw;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    }
}


/* Image formats here are target formats */
static const ProcessSpans processTextureSpansARGB32PM[NBlendTypes] = {
    blend_untransformed_argb,           // Untransformed
    blend_tiled_argb,                   // Tiled
    blend_transformed_argb,             // Transformed
    blend_transformed_tiled_argb,       // TransformedTiled
    blend_src_generic,                  // TransformedBilinear
    blend_src_generic                   // TransformedBilinearTiled
};

static const ProcessSpans processTextureSpansRGB16[NBlendTypes] = {
    blend_untransformed_rgb565,         // Untransformed
    blend_tiled_rgb565,                 // Tiled
    blend_transformed_rgb565,           // Transformed
    blend_transformed_tiled_rgb565,     // TransformedTiled
    blend_transformed_bilinear_rgb565,  // TransformedBilinear
    blend_src_generic                   // TransformedBilinearTiled
};

static const ProcessSpans processTextureSpansGeneric[NBlendTypes] = {
    blend_untransformed_generic,        // Untransformed
    blend_tiled_generic,                // Tiled
    blend_src_generic,                  // Transformed
    blend_src_generic,                  // TransformedTiled
    blend_src_generic,                  // TransformedBilinear
    blend_src_generic                   // TransformedBilinearTiled
};

static const ProcessSpans processTextureSpansGeneric64[NBlendTypes] = {
    blend_untransformed_generic_rgb64,  // Untransformed
    blend_tiled_generic_rgb64,          // Tiled
    blend_src_generic_rgb64,            // Transformed
    blend_src_generic_rgb64,            // TransformedTiled
    blend_src_generic_rgb64,            // TransformedBilinear
    blend_src_generic_rgb64             // TransformedBilinearTiled
};

void qBlendTexture(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    TextureBlendType blendType = getBlendType(data);
    ProcessSpans proc;
    switch (data->rasterBuffer->format) {
    case QImage::Format_ARGB32_Premultiplied:
        proc = processTextureSpansARGB32PM[blendType];
        break;
    case QImage::Format_RGB16:
        proc = processTextureSpansRGB16[blendType];
        break;
#if defined(__SSE2__) || defined(__ARM_NEON__) || (Q_PROCESSOR_WORDSIZE == 8)
    case QImage::Format_ARGB32:
    case QImage::Format_RGBA8888:
#endif
    case QImage::Format_BGR30:
    case QImage::Format_A2BGR30_Premultiplied:
    case QImage::Format_RGB30:
    case QImage::Format_A2RGB30_Premultiplied:
        proc = processTextureSpansGeneric64[blendType];
        break;
    case QImage::Format_Invalid:
        Q_UNREACHABLE();
        return;
    default:
        proc = processTextureSpansGeneric[blendType];
        break;
    }
    proc(count, spans, userData);
}

template <class DST> Q_STATIC_TEMPLATE_FUNCTION
inline void qt_bitmapblit_template(QRasterBuffer *rasterBuffer,
                                   int x, int y, DST color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    DST *dest = reinterpret_cast<DST *>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->stride<DST>();

    if (mapWidth > 8) {
        while (mapHeight--) {
            int x0 = 0;
            int n = 0;
            for (int x = 0; x < mapWidth; x += 8) {
                uchar s = map[x >> 3];
                for (int i = 0; i < 8; ++i) {
                    if (s & 0x80) {
                        ++n;
                    } else {
                        if (n) {
                            qt_memfill(dest + x0, color, n);
                            x0 += n + 1;
                            n = 0;
                        } else {
                            ++x0;
                        }
                        if (!s) {
                            x0 += 8 - 1 - i;
                            break;
                        }
                    }
                    s <<= 1;
                }
            }
            if (n)
                qt_memfill(dest + x0, color, n);
            dest += destStride;
            map += mapStride;
        }
    } else {
        while (mapHeight--) {
            int x0 = 0;
            int n = 0;
            for (uchar s = *map; s; s <<= 1) {
                if (s & 0x80) {
                    ++n;
                } else if (n) {
                    qt_memfill(dest + x0, color, n);
                    x0 += n + 1;
                    n = 0;
                } else {
                    ++x0;
                }
            }
            if (n)
                qt_memfill(dest + x0, color, n);
            dest += destStride;
            map += mapStride;
        }
    }
}

static void qt_gradient_argb32(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    bool isVerticalGradient =
        data->txop <= QTransform::TxScale &&
        data->type == QSpanData::LinearGradient &&
        data->gradient.linear.end.x == data->gradient.linear.origin.x;

    if (isVerticalGradient) {
        LinearGradientValues linear;
        getLinearGradientValues(&linear, data);

        CompositionFunctionSolid funcSolid =
            functionForModeSolid[data->rasterBuffer->compositionMode];

        /*
            The logic for vertical gradient calculations is a mathematically
            reduced copy of that in fetchLinearGradient() - which is basically:

                qreal ry = data->m22 * (y + 0.5) + data->dy;
                qreal t = linear.dy*ry + linear.off;
                t *= (GRADIENT_STOPTABLE_SIZE - 1);
                quint32 color =
                    qt_gradient_pixel_fixed(&data->gradient,
                                            int(t * FIXPT_SIZE));

            This has then been converted to fixed point to improve performance.
         */
        const int gss = GRADIENT_STOPTABLE_SIZE - 1;
        int yinc = int((linear.dy * data->m22 * gss) * FIXPT_SIZE);
        int off = int((((linear.dy * (data->m22 * qreal(0.5) + data->dy) + linear.off) * gss) * FIXPT_SIZE));

        while (count--) {
            int y = spans->y;
            int x = spans->x;

            quint32 *dst = (quint32 *)(data->rasterBuffer->scanLine(y)) + x;
            quint32 color =
                qt_gradient_pixel_fixed(&data->gradient, yinc * y + off);

            funcSolid(dst, spans->len, color, spans->coverage);
            ++spans;
        }

    } else {
        blend_src_generic(count, spans, userData);
    }
}

static void qt_gradient_quint16(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    bool isVerticalGradient =
        data->txop <= QTransform::TxScale &&
        data->type == QSpanData::LinearGradient &&
        data->gradient.linear.end.x == data->gradient.linear.origin.x;

    if (isVerticalGradient) {

        LinearGradientValues linear;
        getLinearGradientValues(&linear, data);

        /*
            The logic for vertical gradient calculations is a mathematically
            reduced copy of that in fetchLinearGradient() - which is basically:

                qreal ry = data->m22 * (y + 0.5) + data->dy;
                qreal t = linear.dy*ry + linear.off;
                t *= (GRADIENT_STOPTABLE_SIZE - 1);
                quint32 color =
                    qt_gradient_pixel_fixed(&data->gradient,
                                            int(t * FIXPT_SIZE));

            This has then been converted to fixed point to improve performance.
         */
        const int gss = GRADIENT_STOPTABLE_SIZE - 1;
        int yinc = int((linear.dy * data->m22 * gss) * FIXPT_SIZE);
        int off = int((((linear.dy * (data->m22 * qreal(0.5) + data->dy) + linear.off) * gss) * FIXPT_SIZE));

        // Save the fillData since we overwrite it when setting solid.color.
        QGradientData gradient = data->gradient;
        while (count--) {
            int y = spans->y;

            data->solid.color = QRgba64::fromArgb32(qt_gradient_pixel_fixed(&gradient, yinc * y + off));
            blend_color_rgb16(1, spans, userData);
            ++spans;
        }
        data->gradient = gradient;

    } else {
        blend_src_generic(count, spans, userData);
    }
}

inline static void qt_bitmapblit_argb32(QRasterBuffer *rasterBuffer,
                                   int x, int y, const QRgba64 &color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint32>(rasterBuffer, x,  y, color.toArgb32(),
                                    map, mapWidth, mapHeight, mapStride);
}

inline static void qt_bitmapblit_rgba8888(QRasterBuffer *rasterBuffer,
                                   int x, int y, const QRgba64 &color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint32>(rasterBuffer, x, y, ARGB2RGBA(color.toArgb32()),
                                    map, mapWidth, mapHeight, mapStride);
}

template<QtPixelOrder PixelOrder>
inline static void qt_bitmapblit_rgb30(QRasterBuffer *rasterBuffer,
                                   int x, int y, const QRgba64 &color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint32>(rasterBuffer, x, y, qConvertRgb64ToRgb30<PixelOrder>(color),
                                    map, mapWidth, mapHeight, mapStride);
}

inline static void qt_bitmapblit_quint16(QRasterBuffer *rasterBuffer,
                                   int x, int y, const QRgba64 &color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint16>(rasterBuffer, x,  y, color.toRgb16(),
                                    map, mapWidth, mapHeight, mapStride);
}

static inline void alphamapblend_generic(int coverage, QRgba64 *dest, int x, const QRgba64 &srcLinear, const QRgba64 &src, const QColorProfile *colorProfile)
{
    if (coverage == 0) {
        // nothing
    } else if (coverage == 255) {
        dest[x] = src;
    } else {
        QRgba64 dstColor = dest[x];
        if (colorProfile) {
            if (dstColor.isOpaque())
                dstColor = colorProfile->toLinear(dstColor);
            else if (!dstColor.isTransparent())
                dstColor = colorProfile->toLinear(dstColor.unpremultiplied()).premultiplied();
        }

        dstColor = interpolate255(srcLinear, coverage, dstColor, 255 - coverage);
        if (colorProfile) {
            if (dstColor.isOpaque())
                dstColor = colorProfile->fromLinear(dstColor);
            else if (!dstColor.isTransparent())
                dstColor = colorProfile->fromLinear(dstColor.unpremultiplied()).premultiplied();
        }
        dest[x] = dstColor;
    }
}

static void qt_alphamapblit_generic(QRasterBuffer *rasterBuffer,
                                    int x, int y, const QRgba64 &color,
                                    const uchar *map,
                                    int mapWidth, int mapHeight, int mapStride,
                                    const QClipData *clip, bool useGammaCorrection)
{
    if (color.isTransparent())
        return;

    const QColorProfile *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA8Text();

    QRgba64 srcColor = color;
    if (colorProfile) {
        if (color.isOpaque())
            srcColor = colorProfile->toLinear(srcColor);
        else
            srcColor = colorProfile->toLinear(srcColor.unpremultiplied()).premultiplied();
    }

    quint64 buffer[buffer_size];
    const DestFetchProc64 destFetch64 = destFetchProc64[rasterBuffer->format];
    const DestStoreProc64 destStore64 = destStoreProc64[rasterBuffer->format];

    if (!clip) {
        for (int ly = 0; ly < mapHeight; ++ly) {
            int i = x;
            int length = mapWidth;
            while (length > 0) {
                int l = qMin(buffer_size, length);
                QRgba64 *dest = destFetch64((QRgba64*)buffer, rasterBuffer, i, y + ly, l);
                for (int j=0; j < l; ++j) {
                    const int coverage = map[j + (i - x)];
                    alphamapblend_generic(coverage, dest, j, srcColor, color, colorProfile);
                }
                destStore64(rasterBuffer, i, y + ly, dest, l);
                length -= l;
                i += l;
            }
            map += mapStride;
        }
    } else {
        int bottom = qMin(y + mapHeight, rasterBuffer->height());

        int top = qMax(y, 0);
        map += (top - y) * mapStride;

        const_cast<QClipData *>(clip)->initialize();
        for (int yp = top; yp<bottom; ++yp) {
            const QClipData::ClipLine &line = clip->m_clipLines[yp];

            for (int i=0; i<line.count; ++i) {
                const QSpan &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);
                if (end <= start)
                    continue;
                Q_ASSERT(end - start <= buffer_size);
                QRgba64 *dest = destFetch64((QRgba64*)buffer, rasterBuffer, start, clip.y, end - start);

                for (int xp=start; xp<end; ++xp) {
                    const int coverage = map[xp - x];
                    alphamapblend_generic(coverage, dest, xp - start, srcColor, color, colorProfile);
                }
                destStore64(rasterBuffer, start, clip.y, dest, end - start);
            } // for (i -> line.count)
            map += mapStride;
        } // for (yp -> bottom)
    }
}

static inline void alphamapblend_quint16(int coverage, quint16 *dest, int x, const quint16 srcColor)
{
    if (coverage == 0) {
        // nothing
    } else if (coverage == 255) {
        dest[x] = srcColor;
    } else {
        dest[x] = BYTE_MUL_RGB16(srcColor, coverage)
                + BYTE_MUL_RGB16(dest[x], 255 - coverage);
    }
}

void qt_alphamapblit_quint16(QRasterBuffer *rasterBuffer,
                             int x, int y, const QRgba64 &color,
                             const uchar *map,
                             int mapWidth, int mapHeight, int mapStride,
                             const QClipData *clip, bool useGammaCorrection)
{
    if (useGammaCorrection) {
        qt_alphamapblit_generic(rasterBuffer, x, y, color, map, mapWidth, mapHeight, mapStride, clip, useGammaCorrection);
        return;
    }

    const quint16 c = color.toRgb16();

    if (!clip) {
        quint16 *dest = reinterpret_cast<quint16*>(rasterBuffer->scanLine(y)) + x;
        const int destStride = rasterBuffer->stride<quint16>();
        while (mapHeight--) {
            for (int i = 0; i < mapWidth; ++i)
                alphamapblend_quint16(map[i], dest, i, c);
            dest += destStride;
            map += mapStride;
        }
    } else {
        int top = qMax(y, 0);
        int bottom = qMin(y + mapHeight, rasterBuffer->height());
        map += (top - y) * mapStride;

        const_cast<QClipData *>(clip)->initialize();
        for (int yp = top; yp<bottom; ++yp) {
            const QClipData::ClipLine &line = clip->m_clipLines[yp];

            quint16 *dest = reinterpret_cast<quint16*>(rasterBuffer->scanLine(yp));

            for (int i=0; i<line.count; ++i) {
                const QSpan &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);

                for (int xp=start; xp<end; ++xp)
                    alphamapblend_quint16(map[xp - x], dest, xp, c);
            } // for (i -> line.count)
            map += mapStride;
        } // for (yp -> bottom)
    }
}

static inline void rgbBlendPixel(quint32 *dst, int coverage, QRgba64 slinear, const QColorProfile *colorProfile)
{
    // Do a gammacorrected RGB alphablend...
    const QRgba64 dlinear = colorProfile ? colorProfile->toLinear64(*dst) : QRgba64::fromArgb32(*dst);

    QRgba64 blend = rgbBlend(dlinear, slinear, coverage);

    *dst = colorProfile ? colorProfile->fromLinear64(blend) : toArgb32(blend);
}

static inline void grayBlendPixel(quint32 *dst, int coverage, QRgba64 srcLinear, const QColorProfile *colorProfile)
{
    // Do a gammacorrected gray alphablend...
    const QRgba64 dstLinear = colorProfile ? colorProfile->toLinear64(*dst) : QRgba64::fromArgb32(*dst);

    QRgba64 blend = interpolate255(srcLinear, coverage, dstLinear, 255 - coverage);

    *dst = colorProfile ? colorProfile->fromLinear64(blend) : toArgb32(blend);
}

static inline void alphamapblend_argb32(quint32 *dst, int coverage, QRgba64 srcLinear, quint32 src, const QColorProfile *colorProfile)
{
    if (coverage == 0) {
        // nothing
    } else if (coverage == 255) {
        *dst = src;
    } else if (!colorProfile) {
        *dst = INTERPOLATE_PIXEL_255(src, coverage, *dst, 255 - coverage);
    } else {
        if (*dst >= 0xff000000) {
            grayBlendPixel(dst, coverage, srcLinear, colorProfile);
        } else {
            // Give up and do a naive gray alphablend. Needed to deal with ARGB32 and invalid ARGB32_premultiplied, see QTBUG-60571
            *dst = INTERPOLATE_PIXEL_255(src, coverage, *dst, 255 - coverage);
        }
    }
}

static void qt_alphamapblit_argb32(QRasterBuffer *rasterBuffer,
                                   int x, int y, const QRgba64 &color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride,
                                   const QClipData *clip, bool useGammaCorrection)
{
    const quint32 c = color.toArgb32();
    const int destStride = rasterBuffer->stride<quint32>();

    if (color.isTransparent())
        return;

    const QColorProfile *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA8Text();

    QRgba64 srcColor = color;
    if (colorProfile) {
        if (color.isOpaque())
            srcColor = colorProfile->toLinear(srcColor);
        else
            srcColor = colorProfile->toLinear(srcColor.unpremultiplied()).premultiplied();
    }

    if (!clip) {
        quint32 *dest = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y)) + x;
        while (mapHeight--) {
            for (int i = 0; i < mapWidth; ++i) {
                const int coverage = map[i];
                alphamapblend_argb32(dest + i, coverage, srcColor, c, colorProfile);
            }
            dest += destStride;
            map += mapStride;
        }
    } else {
        int bottom = qMin(y + mapHeight, rasterBuffer->height());

        int top = qMax(y, 0);
        map += (top - y) * mapStride;

        const_cast<QClipData *>(clip)->initialize();
        for (int yp = top; yp<bottom; ++yp) {
            const QClipData::ClipLine &line = clip->m_clipLines[yp];

            quint32 *dest = reinterpret_cast<quint32 *>(rasterBuffer->scanLine(yp));

            for (int i=0; i<line.count; ++i) {
                const QSpan &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);

                for (int xp=start; xp<end; ++xp) {
                    const int coverage = map[xp - x];
                    alphamapblend_argb32(dest + xp, coverage, srcColor, c, colorProfile);
                } // for (i -> line.count)
            } // for (yp -> bottom)
            map += mapStride;
        }
    }
}

static inline int qRgbAvg(QRgb rgb)
{
    return (qRed(rgb) * 5 + qGreen(rgb) * 6 + qBlue(rgb) * 5) / 16;
}

static inline QRgb rgbBlend(QRgb d, QRgb s, uint rgbAlpha)
{
#if defined(__SSE2__)
    __m128i vd = _mm_cvtsi32_si128(d);
    __m128i vs = _mm_cvtsi32_si128(s);
    __m128i va = _mm_cvtsi32_si128(rgbAlpha);
    const __m128i vz = _mm_setzero_si128();
    vd = _mm_unpacklo_epi8(vd, vz);
    vs = _mm_unpacklo_epi8(vs, vz);
    va = _mm_unpacklo_epi8(va, vz);
    __m128i vb = _mm_xor_si128(_mm_set1_epi16(255), va);
    vs = _mm_mullo_epi16(vs, va);
    vd = _mm_mullo_epi16(vd, vb);
    vd = _mm_add_epi16(vd, vs);
    vd = _mm_add_epi16(vd, _mm_srli_epi16(vd, 8));
    vd = _mm_add_epi16(vd, _mm_set1_epi16(0x80));
    vd = _mm_srli_epi16(vd, 8);
    vd = _mm_packus_epi16(vd, vd);
    return _mm_cvtsi128_si32(vd);
#else
    const int dr = qRed(d);
    const int dg = qGreen(d);
    const int db = qBlue(d);

    const int sr = qRed(s);
    const int sg = qGreen(s);
    const int sb = qBlue(s);

    const int mr = qRed(rgbAlpha);
    const int mg = qGreen(rgbAlpha);
    const int mb = qBlue(rgbAlpha);

    const int nr = qt_div_255(sr * mr + dr * (255 - mr));
    const int ng = qt_div_255(sg * mg + dg * (255 - mg));
    const int nb = qt_div_255(sb * mb + db * (255 - mb));

    return 0xff000000 | (nr << 16) | (ng << 8) | nb;
#endif
}

static inline void alphargbblend_generic(uint coverage, QRgba64 *dest, int x, const QRgba64 &srcLinear, const QRgba64 &src, const QColorProfile *colorProfile)
{
    if (coverage == 0xff000000) {
        // nothing
    } else if (coverage == 0xffffffff) {
        dest[x] = src;
    } else {
        QRgba64 dstColor = dest[x];
        if (dstColor.isOpaque()) {
            if (colorProfile)
                dstColor = colorProfile->toLinear(dstColor);
            dstColor = rgbBlend(dstColor, srcLinear, coverage);
            if (colorProfile)
                dstColor = colorProfile->fromLinear(dstColor);
            dest[x] = dstColor;
        } else {
            // Do a gray alphablend.
            alphamapblend_generic(qRgbAvg(coverage), dest, x, srcLinear, src, colorProfile);
        }
    }
}

static inline void alphargbblend_argb32(quint32 *dst, uint coverage, const QRgba64 &srcLinear, quint32 src, const QColorProfile *colorProfile)
{
    if (coverage == 0xff000000) {
        // nothing
    } else if (coverage == 0xffffffff) {
        *dst = src;
    } else if (*dst < 0xff000000) {
        // Give up and do a naive gray alphablend. Needed to deal with ARGB32 and invalid ARGB32_premultiplied, see QTBUG-60571
        const int a = qRgbAvg(coverage);
        *dst = INTERPOLATE_PIXEL_255(src, a, *dst, 255 - a);
    } else if (!colorProfile) {
        *dst = rgbBlend(*dst, src, coverage);
    } else  {
        rgbBlendPixel(dst, coverage, srcLinear, colorProfile);
    }
}

static void qt_alphargbblit_generic(QRasterBuffer *rasterBuffer,
                                    int x, int y, const QRgba64 &color,
                                    const uint *src, int mapWidth, int mapHeight, int srcStride,
                                    const QClipData *clip, bool useGammaCorrection)
{
    if (color.isTransparent())
        return;

    const QColorProfile *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA32Text();

    QRgba64 srcColor = color;
    if (colorProfile) {
        if (color.isOpaque())
            srcColor = colorProfile->toLinear(srcColor);
        else
            srcColor = colorProfile->toLinear(srcColor.unpremultiplied()).premultiplied();
    }

    quint64 buffer[buffer_size];
    const DestFetchProc64 destFetch64 = destFetchProc64[rasterBuffer->format];
    const DestStoreProc64 destStore64 = destStoreProc64[rasterBuffer->format];

    if (!clip) {
        for (int ly = 0; ly < mapHeight; ++ly) {
            int i = x;
            int length = mapWidth;
            while (length > 0) {
                int l = qMin(buffer_size, length);
                QRgba64 *dest = destFetch64((QRgba64*)buffer, rasterBuffer, i, y + ly, l);
                for (int j=0; j < l; ++j) {
                    const uint coverage = src[j + (i - x)];
                    alphargbblend_generic(coverage, dest, j, srcColor, color, colorProfile);
                }
                destStore64(rasterBuffer, i, y + ly, dest, l);
                length -= l;
                i += l;
            }
            src += srcStride;
        }
    } else {
        int bottom = qMin(y + mapHeight, rasterBuffer->height());

        int top = qMax(y, 0);
        src += (top - y) * srcStride;

        const_cast<QClipData *>(clip)->initialize();
        for (int yp = top; yp<bottom; ++yp) {
            const QClipData::ClipLine &line = clip->m_clipLines[yp];

            for (int i=0; i<line.count; ++i) {
                const QSpan &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);
                if (end <= start)
                    continue;
                Q_ASSERT(end - start <= buffer_size);
                QRgba64 *dest = destFetch64((QRgba64*)buffer, rasterBuffer, start, clip.y, end - start);

                for (int xp=start; xp<end; ++xp) {
                    const uint coverage = src[xp - x];
                    alphargbblend_generic(coverage, dest, xp - start, srcColor, color, colorProfile);
                }
                destStore64(rasterBuffer, start, clip.y, dest, end - start);
            } // for (i -> line.count)
            src += srcStride;
        } // for (yp -> bottom)
    }
}

static void qt_alphargbblit_argb32(QRasterBuffer *rasterBuffer,
                                   int x, int y, const QRgba64 &color,
                                   const uint *src, int mapWidth, int mapHeight, int srcStride,
                                   const QClipData *clip, bool useGammaCorrection)
{
    if (color.isTransparent())
        return;

    const quint32 c = color.toArgb32();

    const QColorProfile *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA32Text();

    QRgba64 srcColor = color;
    if (colorProfile) {
        if (color.isOpaque())
            srcColor = colorProfile->toLinear(srcColor);
        else
            srcColor = colorProfile->toLinear(srcColor.unpremultiplied()).premultiplied();
    }

    if (!clip) {
        quint32 *dst = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y)) + x;
        const int destStride = rasterBuffer->stride<quint32>();
        while (mapHeight--) {
            for (int i = 0; i < mapWidth; ++i) {
                const uint coverage = src[i];
                alphargbblend_argb32(dst + i, coverage, srcColor, c, colorProfile);
            }

            dst += destStride;
            src += srcStride;
        }
    } else {
        int bottom = qMin(y + mapHeight, rasterBuffer->height());

        int top = qMax(y, 0);
        src += (top - y) * srcStride;

        const_cast<QClipData *>(clip)->initialize();
        for (int yp = top; yp<bottom; ++yp) {
            const QClipData::ClipLine &line = clip->m_clipLines[yp];

            quint32 *dst = reinterpret_cast<quint32 *>(rasterBuffer->scanLine(yp));

            for (int i=0; i<line.count; ++i) {
                const QSpan &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);

                for (int xp=start; xp<end; ++xp) {
                    const uint coverage = src[xp - x];
                    alphargbblend_argb32(dst + xp, coverage, srcColor, c, colorProfile);
                }
            } // for (i -> line.count)
            src += srcStride;
        } // for (yp -> bottom)

    }
}

static void qt_rectfill_argb32(QRasterBuffer *rasterBuffer,
                               int x, int y, int width, int height,
                               const QRgba64 &color)
{
    qt_rectfill<quint32>(reinterpret_cast<quint32 *>(rasterBuffer->buffer()),
                         color.toArgb32(), x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_quint16(QRasterBuffer *rasterBuffer,
                                int x, int y, int width, int height,
                                const QRgba64 &color)
{
    qt_rectfill<quint16>(reinterpret_cast<quint16 *>(rasterBuffer->buffer()),
                         color.toRgb16(), x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_nonpremul_argb32(QRasterBuffer *rasterBuffer,
                                         int x, int y, int width, int height,
                                         const QRgba64 &color)
{
    qt_rectfill<quint32>(reinterpret_cast<quint32 *>(rasterBuffer->buffer()),
                         color.unpremultiplied().toArgb32(), x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_rgba(QRasterBuffer *rasterBuffer,
                             int x, int y, int width, int height,
                             const QRgba64 &color)
{
    qt_rectfill<quint32>(reinterpret_cast<quint32 *>(rasterBuffer->buffer()),
                         ARGB2RGBA(color.toArgb32()), x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_nonpremul_rgba(QRasterBuffer *rasterBuffer,
                                       int x, int y, int width, int height,
                                       const QRgba64 &color)
{
    qt_rectfill<quint32>(reinterpret_cast<quint32 *>(rasterBuffer->buffer()),
                         ARGB2RGBA(color.unpremultiplied().toArgb32()), x, y, width, height, rasterBuffer->bytesPerLine());
}

template<QtPixelOrder PixelOrder>
static void qt_rectfill_rgb30(QRasterBuffer *rasterBuffer,
                              int x, int y, int width, int height,
                              const QRgba64 &color)
{
    qt_rectfill<quint32>(reinterpret_cast<quint32 *>(rasterBuffer->buffer()),
                         qConvertRgb64ToRgb30<PixelOrder>(color), x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_alpha(QRasterBuffer *rasterBuffer,
                             int x, int y, int width, int height,
                             const QRgba64 &color)
{
    qt_rectfill<quint8>(reinterpret_cast<quint8 *>(rasterBuffer->buffer()),
                         color.alpha() >> 8, x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_gray(QRasterBuffer *rasterBuffer,
                             int x, int y, int width, int height,
                             const QRgba64 &color)
{
    qt_rectfill<quint8>(reinterpret_cast<quint8 *>(rasterBuffer->buffer()),
                         qGray(color.toArgb32()), x, y, width, height, rasterBuffer->bytesPerLine());
}

// Map table for destination image format. Contains function pointers
// for blends of various types unto the destination

DrawHelper qDrawHelper[QImage::NImageFormats] =
{
    // Format_Invalid,
    { 0, 0, 0, 0, 0, 0 },
    // Format_Mono,
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_MonoLSB,
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_Indexed8,
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_RGB32,
    {
        blend_color_argb,
        qt_gradient_argb32,
        qt_bitmapblit_argb32,
        qt_alphamapblit_argb32,
        qt_alphargbblit_argb32,
        qt_rectfill_argb32
    },
    // Format_ARGB32,
    {
        blend_color_generic,
        blend_src_generic,
        qt_bitmapblit_argb32,
        qt_alphamapblit_argb32,
        qt_alphargbblit_argb32,
        qt_rectfill_nonpremul_argb32
    },
    // Format_ARGB32_Premultiplied
    {
        blend_color_argb,
        qt_gradient_argb32,
        qt_bitmapblit_argb32,
        qt_alphamapblit_argb32,
        qt_alphargbblit_argb32,
        qt_rectfill_argb32
    },
    // Format_RGB16
    {
        blend_color_rgb16,
        qt_gradient_quint16,
        qt_bitmapblit_quint16,
        qt_alphamapblit_quint16,
        qt_alphargbblit_generic,
        qt_rectfill_quint16
    },
    // Format_ARGB8565_Premultiplied
    {
        blend_color_generic,
        blend_src_generic,
        0,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        0
    },
    // Format_RGB666
    {
        blend_color_generic,
        blend_src_generic,
        0,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        0
    },
    // Format_ARGB6666_Premultiplied
    {
        blend_color_generic,
        blend_src_generic,
        0,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        0
    },
    // Format_RGB555
    {
        blend_color_generic,
        blend_src_generic,
        0,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        0
    },
    // Format_ARGB8555_Premultiplied
    {
        blend_color_generic,
        blend_src_generic,
        0,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        0
    },
    // Format_RGB888
    {
        blend_color_generic,
        blend_src_generic,
        0,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        0
    },
    // Format_RGB444
    {
        blend_color_generic,
        blend_src_generic,
        0,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        0
    },
    // Format_ARGB4444_Premultiplied
    {
        blend_color_generic,
        blend_src_generic,
        0,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        0
    },
    // Format_RGBX8888
    {
        blend_color_generic,
        blend_src_generic,
        qt_bitmapblit_rgba8888,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgba
    },
    // Format_RGBA8888
    {
        blend_color_generic,
        blend_src_generic,
        qt_bitmapblit_rgba8888,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_nonpremul_rgba
    },
    // Format_RGB8888_Premultiplied
    {
        blend_color_generic,
        blend_src_generic,
        qt_bitmapblit_rgba8888,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgba
    },
    // Format_BGR30
    {
        blend_color_generic_rgb64,
        blend_src_generic_rgb64,
        qt_bitmapblit_rgb30<PixelOrderBGR>,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgb30<PixelOrderBGR>
    },
    // Format_A2BGR30_Premultiplied
    {
        blend_color_generic_rgb64,
        blend_src_generic_rgb64,
        qt_bitmapblit_rgb30<PixelOrderBGR>,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgb30<PixelOrderBGR>
    },
    // Format_RGB30
    {
        blend_color_generic_rgb64,
        blend_src_generic_rgb64,
        qt_bitmapblit_rgb30<PixelOrderRGB>,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgb30<PixelOrderRGB>
    },
    // Format_A2RGB30_Premultiplied
    {
        blend_color_generic_rgb64,
        blend_src_generic_rgb64,
        qt_bitmapblit_rgb30<PixelOrderRGB>,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgb30<PixelOrderRGB>
    },
    // Format_Alpha8
    {
        blend_color_generic,
        blend_src_generic,
        0,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_alpha
    },
    // Format_Grayscale8
    {
        blend_color_generic,
        blend_src_generic,
        0,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_gray
    },
};

#if defined(Q_CC_MSVC) && !defined(_MIPS_)
template <class T>
inline void qt_memfill_template(T *dest, T color, int count)
{
    while (count--)
        *dest++ = color;
}

#else

template <class T>
inline void qt_memfill_template(T *dest, T color, int count)
{
    int n = (count + 7) / 8;
    switch (count & 0x07)
    {
    case 0: do { *dest++ = color; Q_FALLTHROUGH();
    case 7:      *dest++ = color; Q_FALLTHROUGH();
    case 6:      *dest++ = color; Q_FALLTHROUGH();
    case 5:      *dest++ = color; Q_FALLTHROUGH();
    case 4:      *dest++ = color; Q_FALLTHROUGH();
    case 3:      *dest++ = color; Q_FALLTHROUGH();
    case 2:      *dest++ = color; Q_FALLTHROUGH();
    case 1:      *dest++ = color;
    } while (--n > 0);
    }
}

template <>
inline void qt_memfill_template(quint16 *dest, quint16 value, int count)
{
    if (count < 3) {
        switch (count) {
        case 2: *dest++ = value; Q_FALLTHROUGH();
        case 1: *dest = value;
        }
        return;
    }

    const int align = (quintptr)(dest) & 0x3;
    switch (align) {
    case 2: *dest++ = value; --count;
    }

    const quint32 value32 = (value << 16) | value;
    qt_memfill(reinterpret_cast<quint32*>(dest), value32, count / 2);
    if (count & 0x1)
        dest[count - 1] = value;
}
#endif

void qt_memfill64(quint64 *dest, quint64 color, int count)
{
    qt_memfill_template<quint64>(dest, color, count);
}

#if !defined(__SSE2__)
void qt_memfill16(quint16 *dest, quint16 color, int count)
{
    qt_memfill_template<quint16>(dest, color, count);
}
#endif
#if !defined(__SSE2__) && !defined(__ARM_NEON__) && !defined(__MIPS_DSP__)
void qt_memfill32(quint32 *dest, quint32 color, int count)
{
    qt_memfill_template<quint32>(dest, color, count);
}
#endif

#ifdef QT_COMPILER_SUPPORTS_SSE4_1
template<QtPixelOrder> const uint *QT_FASTCALL convertA2RGB30PMFromARGB32PM_sse4(uint *buffer, const uint *src, int count, const QVector<QRgb> *, QDitherInfo *);
#endif

extern void qInitBlendFunctions();

static void qInitDrawhelperFunctions()
{
    // Set up basic blend function tables.
    qInitBlendFunctions();

#ifdef __SSE2__
    qDrawHelper[QImage::Format_RGB32].bitmapBlit = qt_bitmapblit32_sse2;
    qDrawHelper[QImage::Format_ARGB32].bitmapBlit = qt_bitmapblit32_sse2;
    qDrawHelper[QImage::Format_ARGB32_Premultiplied].bitmapBlit = qt_bitmapblit32_sse2;
    qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_sse2;
    qDrawHelper[QImage::Format_RGBX8888].bitmapBlit = qt_bitmapblit8888_sse2;
    qDrawHelper[QImage::Format_RGBA8888].bitmapBlit = qt_bitmapblit8888_sse2;
    qDrawHelper[QImage::Format_RGBA8888_Premultiplied].bitmapBlit = qt_bitmapblit8888_sse2;

    extern void qt_scale_image_argb32_on_argb32_sse2(uchar *destPixels, int dbpl,
                                                     const uchar *srcPixels, int sbpl, int srch,
                                                     const QRectF &targetRect,
                                                     const QRectF &sourceRect,
                                                     const QRect &clip,
                                                     int const_alpha);
    qScaleFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_scale_image_argb32_on_argb32_sse2;
    qScaleFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_scale_image_argb32_on_argb32_sse2;
    qScaleFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_scale_image_argb32_on_argb32_sse2;
    qScaleFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_scale_image_argb32_on_argb32_sse2;

    extern void qt_blend_rgb32_on_rgb32_sse2(uchar *destPixels, int dbpl,
                                             const uchar *srcPixels, int sbpl,
                                             int w, int h,
                                             int const_alpha);
    extern void qt_blend_argb32_on_argb32_sse2(uchar *destPixels, int dbpl,
                                               const uchar *srcPixels, int sbpl,
                                               int w, int h,
                                               int const_alpha);

    qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_sse2;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_sse2;
    qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_sse2;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_sse2;
    qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_sse2;
    qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_sse2;
    qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_sse2;
    qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_sse2;

    extern const uint * QT_FASTCALL qt_fetch_radial_gradient_sse2(uint *buffer, const Operator *op, const QSpanData *data,
                                                                  int y, int x, int length);

    qt_fetch_radial_gradient = qt_fetch_radial_gradient_sse2;

    extern void QT_FASTCALL comp_func_SourceOver_sse2(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
    extern void QT_FASTCALL comp_func_solid_SourceOver_sse2(uint *destPixels, int length, uint color, uint const_alpha);
    extern void QT_FASTCALL comp_func_Source_sse2(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
    extern void QT_FASTCALL comp_func_Plus_sse2(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
    qt_functionForMode_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_sse2;
    qt_functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_sse2;
    qt_functionForMode_C[QPainter::CompositionMode_Source] = comp_func_Source_sse2;
    qt_functionForMode_C[QPainter::CompositionMode_Plus] = comp_func_Plus_sse2;

#ifdef QT_COMPILER_SUPPORTS_SSSE3
    if (qCpuHasFeature(SSSE3)) {
        extern void qt_blend_argb32_on_argb32_ssse3(uchar *destPixels, int dbpl,
                                                    const uchar *srcPixels, int sbpl,
                                                    int w, int h,
                                                    int const_alpha);

        extern void QT_FASTCALL storePixelsBPP24_ssse3(uchar *dest, const uint *src, int index, int count);
        extern const uint * QT_FASTCALL qt_fetchUntransformed_888_ssse3(uint *buffer, const Operator *, const QSpanData *data,
                                                                        int y, int x, int length);
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qStorePixels[QPixelLayout::BPP24] = storePixelsBPP24_ssse3;
        sourceFetchUntransformed[QImage::Format_RGB888] = qt_fetchUntransformed_888_ssse3;
    }
#endif // SSSE3

#if defined(QT_COMPILER_SUPPORTS_SSE4_1)
    if (qCpuHasFeature(SSE4_1)) {
        extern const uint *QT_FASTCALL convertARGB32ToARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                                    const QVector<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL convertRGBA8888ToARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                                      const QVector<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL convertARGB32FromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                                      const QVector<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL convertRGBA8888FromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                                        const QVector<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL convertRGBXFromARGB32PM_sse4(uint *buffer, const uint *src, int count,
                                                                    const QVector<QRgb> *, QDitherInfo *);
        qPixelLayouts[QImage::Format_ARGB32].convertToARGB32PM = convertARGB32ToARGB32PM_sse4;
        qPixelLayouts[QImage::Format_RGBA8888].convertToARGB32PM = convertRGBA8888ToARGB32PM_sse4;
        qPixelLayouts[QImage::Format_ARGB32].convertFromARGB32PM = convertARGB32FromARGB32PM_sse4;
        qPixelLayouts[QImage::Format_RGBA8888].convertFromARGB32PM = convertRGBA8888FromARGB32PM_sse4;
        qPixelLayouts[QImage::Format_RGBX8888].convertFromARGB32PM = convertRGBXFromARGB32PM_sse4;
        qPixelLayouts[QImage::Format_A2BGR30_Premultiplied].convertFromARGB32PM = convertA2RGB30PMFromARGB32PM_sse4<PixelOrderBGR>;
        qPixelLayouts[QImage::Format_A2RGB30_Premultiplied].convertFromARGB32PM = convertA2RGB30PMFromARGB32PM_sse4<PixelOrderRGB>;
    }
#endif

#if defined(QT_COMPILER_SUPPORTS_AVX2)
    if (qCpuHasFeature(AVX2)) {
        extern void qt_blend_rgb32_on_rgb32_avx2(uchar *destPixels, int dbpl,
                                                 const uchar *srcPixels, int sbpl,
                                                 int w, int h, int const_alpha);
        extern void qt_blend_argb32_on_argb32_avx2(uchar *destPixels, int dbpl,
                                                   const uchar *srcPixels, int sbpl,
                                                   int w, int h, int const_alpha);
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_avx2;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_avx2;
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_avx2;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_avx2;
        qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_avx2;
        qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_avx2;
        qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_avx2;
        qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_avx2;

        extern void QT_FASTCALL comp_func_Source_avx2(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_Source_rgb64_avx2(QRgba64 *destPixels, const QRgba64 *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_SourceOver_avx2(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_SourceOver_rgb64_avx2(QRgba64 *destPixels, const QRgba64 *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_solid_SourceOver_avx2(uint *destPixels, int length, uint color, uint const_alpha);
        extern void QT_FASTCALL comp_func_solid_SourceOver_rgb64_avx2(QRgba64 *destPixels, int length, QRgba64 color, uint const_alpha);

        qt_functionForMode_C[QPainter::CompositionMode_Source] = comp_func_Source_avx2;
        qt_functionForMode64_C[QPainter::CompositionMode_Source] = comp_func_Source_rgb64_avx2;
        qt_functionForMode_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_avx2;
        qt_functionForMode64_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_rgb64_avx2;
        qt_functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_avx2;
        qt_functionForModeSolid64_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_rgb64_avx2;

        extern void QT_FASTCALL fetchTransformedBilinearARGB32PM_simple_upscale_helper_avx2(uint *b, uint *end, const QTextureData &image,
                                                                                            int &fx, int &fy, int fdx, int /*fdy*/);
        extern void QT_FASTCALL fetchTransformedBilinearARGB32PM_downscale_helper_avx2(uint *b, uint *end, const QTextureData &image,
                                                                                       int &fx, int &fy, int fdx, int /*fdy*/);
        extern void QT_FASTCALL fetchTransformedBilinearARGB32PM_fast_rotate_helper_avx2(uint *b, uint *end, const QTextureData &image,
                                                                                         int &fx, int &fy, int fdx, int fdy);

        bilinearFastTransformHelperARGB32PM[0][SimpleUpscaleTransform] = fetchTransformedBilinearARGB32PM_simple_upscale_helper_avx2;
        bilinearFastTransformHelperARGB32PM[0][DownscaleTransform] = fetchTransformedBilinearARGB32PM_downscale_helper_avx2;
        bilinearFastTransformHelperARGB32PM[0][FastRotateTransform] = fetchTransformedBilinearARGB32PM_fast_rotate_helper_avx2;
    }
#endif

#endif // SSE2

#if defined(__ARM_NEON__)
    qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_neon;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_neon;
    qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_neon;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_neon;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_neon;
    qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_neon;
    qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_neon;
    qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_neon;
#endif

    qt_functionForMode_C[QPainter::CompositionMode_SourceOver] = qt_blend_argb32_on_argb32_scanline_neon;
    qt_functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_neon;
    qt_functionForMode_C[QPainter::CompositionMode_Plus] = comp_func_Plus_neon;

    extern const uint * QT_FASTCALL qt_fetch_radial_gradient_neon(uint *buffer, const Operator *op, const QSpanData *data,
                                                                  int y, int x, int length);

    qt_fetch_radial_gradient = qt_fetch_radial_gradient_neon;

    sourceFetchUntransformed[QImage::Format_RGB888] = qt_fetchUntransformed_888_neon;

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    extern const uint *QT_FASTCALL convertARGB32ToARGB32PM_neon(uint *buffer, const uint *src, int count,
                                                                const QVector<QRgb> *, QDitherInfo *);
    extern const uint *QT_FASTCALL convertRGBA8888ToARGB32PM_neon(uint *buffer, const uint *src, int count,
                                                                  const QVector<QRgb> *, QDitherInfo *);
    qPixelLayouts[QImage::Format_ARGB32].convertToARGB32PM = convertARGB32ToARGB32PM_neon;
    qPixelLayouts[QImage::Format_RGBA8888].convertToARGB32PM = convertRGBA8888ToARGB32PM_neon;
#endif

#if defined(ENABLE_PIXMAN_DRAWHELPERS)
    // The RGB16 helpers are using Arm32 assemblythat has not been ported to AArch64
    qBlendFunctions[QImage::Format_RGB16][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_rgb16_neon;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB16] = qt_blend_rgb16_on_argb32_neon;
    qBlendFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_blend_rgb16_on_rgb16_neon;

    qScaleFunctions[QImage::Format_RGB16][QImage::Format_ARGB32_Premultiplied] = qt_scale_image_argb32_on_rgb16_neon;
    qScaleFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_scale_image_rgb16_on_rgb16_neon;

    qTransformFunctions[QImage::Format_RGB16][QImage::Format_ARGB32_Premultiplied] = qt_transform_image_argb32_on_rgb16_neon;
    qTransformFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_transform_image_rgb16_on_rgb16_neon;

    qDrawHelper[QImage::Format_RGB16].alphamapBlit = qt_alphamapblit_quint16_neon;

    destFetchProc[QImage::Format_RGB16] = qt_destFetchRGB16_neon;
    destStoreProc[QImage::Format_RGB16] = qt_destStoreRGB16_neon;

    qMemRotateFunctions[QPixelLayout::BPP16][0] = qt_memrotate90_16_neon;
    qMemRotateFunctions[QPixelLayout::BPP16][2] = qt_memrotate270_16_neon;
#endif
#endif // defined(__ARM_NEON__)

#if defined(__MIPS_DSP__)
    // Composition functions are all DSP r1
    qt_functionForMode_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_asm_mips_dsp;
    qt_functionForMode_C[QPainter::CompositionMode_Source] = comp_func_Source_mips_dsp;
    qt_functionForMode_C[QPainter::CompositionMode_DestinationOver] = comp_func_DestinationOver_mips_dsp;
    qt_functionForMode_C[QPainter::CompositionMode_SourceIn] = comp_func_SourceIn_mips_dsp;
    qt_functionForMode_C[QPainter::CompositionMode_DestinationIn] = comp_func_DestinationIn_mips_dsp;
    qt_functionForMode_C[QPainter::CompositionMode_DestinationOut] = comp_func_DestinationOut_mips_dsp;
    qt_functionForMode_C[QPainter::CompositionMode_SourceAtop] = comp_func_SourceAtop_mips_dsp;
    qt_functionForMode_C[QPainter::CompositionMode_DestinationAtop] = comp_func_DestinationAtop_mips_dsp;
    qt_functionForMode_C[QPainter::CompositionMode_Xor] = comp_func_XOR_mips_dsp;
    qt_functionForMode_C[QPainter::CompositionMode_SourceOut] = comp_func_SourceOut_mips_dsp;

    qt_functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_mips_dsp;
    qt_functionForModeSolid_C[QPainter::CompositionMode_DestinationOver] = comp_func_solid_DestinationOver_mips_dsp;
    qt_functionForModeSolid_C[QPainter::CompositionMode_SourceIn] = comp_func_solid_SourceIn_mips_dsp;
    qt_functionForModeSolid_C[QPainter::CompositionMode_DestinationIn] = comp_func_solid_DestinationIn_mips_dsp;
    qt_functionForModeSolid_C[QPainter::CompositionMode_SourceAtop] = comp_func_solid_SourceAtop_mips_dsp;
    qt_functionForModeSolid_C[QPainter::CompositionMode_DestinationAtop] = comp_func_solid_DestinationAtop_mips_dsp;
    qt_functionForModeSolid_C[QPainter::CompositionMode_Xor] = comp_func_solid_XOR_mips_dsp;
    qt_functionForModeSolid_C[QPainter::CompositionMode_SourceOut] = comp_func_solid_SourceOut_mips_dsp;

    qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_mips_dsp;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_mips_dsp;
    qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_mips_dsp;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_mips_dsp;

    destFetchProc[QImage::Format_ARGB32] = qt_destFetchARGB32_mips_dsp;

    destStoreProc[QImage::Format_ARGB32] = qt_destStoreARGB32_mips_dsp;

    sourceFetchUntransformed[QImage::Format_RGB888] = qt_fetchUntransformed_888_mips_dsp;
    sourceFetchUntransformed[QImage::Format_RGB444] = qt_fetchUntransformed_444_mips_dsp;
    sourceFetchUntransformed[QImage::Format_ARGB8565_Premultiplied] = qt_fetchUntransformed_argb8565_premultiplied_mips_dsp;

#if defined(__MIPS_DSPR2__)
    qBlendFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_blend_rgb16_on_rgb16_mips_dspr2;
    sourceFetchUntransformed[QImage::Format_RGB16] = qt_fetchUntransformedRGB16_mips_dspr2;
#else
    qBlendFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_blend_rgb16_on_rgb16_mips_dsp;
#endif // defined(__MIPS_DSPR2__)
#endif // defined(__MIPS_DSP__)
}

// Ensure initialization if this object file is linked.
Q_CONSTRUCTOR_FUNCTION(qInitDrawhelperFunctions);

QT_END_NAMESPACE
