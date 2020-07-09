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

#include <qglobal.h>

#include <qstylehints.h>
#include <qguiapplication.h>
#include <qatomic.h>
#include <private/qcolortrclut_p.h>
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
#include <qendian.h>
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
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_BGR888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB4444_Premultiplied>() { return 4; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB8555_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB8565_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB6666_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGBX8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGBA8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGBA8888_Premultiplied>() { return 8; }

template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB16>() { return  11; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB444>() { return  8; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB555>() { return 10; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB666>() { return 12; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB888>() { return 16; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_BGR888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB4444_Premultiplied>() { return  8; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB8555_Premultiplied>() { return 18; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB8565_Premultiplied>() { return 19; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB6666_Premultiplied>() { return 12; }
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGBX8888>() { return 24; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGBA8888>() { return 24; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGBA8888_Premultiplied>() { return 24; }
#else
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGBX8888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGBA8888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGBA8888_Premultiplied>() { return 0; }
#endif
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB16>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_BGR888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB4444_Premultiplied>() { return 4; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB8555_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB8565_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB6666_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGBX8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGBA8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGBA8888_Premultiplied>() { return 8; }

template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB16>() { return  5; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_BGR888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB4444_Premultiplied>() { return  4; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB8555_Premultiplied>() { return 13; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB8565_Premultiplied>() { return 13; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB6666_Premultiplied>() { return  6; }
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGBX8888>() { return 16; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGBA8888>() { return 16; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGBA8888_Premultiplied>() { return 16; }
#else
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGBX8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGBA8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGBA8888_Premultiplied>() { return 8; }
#endif
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB16>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_BGR888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB4444_Premultiplied>() { return 4; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB8555_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB8565_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB6666_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGBX8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGBA8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGBA8888_Premultiplied>() { return 8; }

template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB16>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB444>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB555>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB666>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_BGR888>() { return 16; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB4444_Premultiplied>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB8555_Premultiplied>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB8565_Premultiplied>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB6666_Premultiplied>() { return 0; }
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGBX8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGBA8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGBA8888_Premultiplied>() { return 8; }
#else
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGBX8888>() { return 16; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGBA8888>() { return 16; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGBA8888_Premultiplied>() { return 16; }
#endif
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGB16>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGB444>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGB555>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGB666>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGB888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_BGR888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB4444_Premultiplied>() { return  4; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB8555_Premultiplied>() { return  8; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB8565_Premultiplied>() { return  8; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB6666_Premultiplied>() { return  6; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGBX8888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGBA8888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_RGBA8888_Premultiplied>() { return 8; }

template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGB16>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGB444>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGB555>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGB666>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGB888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_BGR888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB4444_Premultiplied>() { return 12; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB8555_Premultiplied>() { return  0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB8565_Premultiplied>() { return  0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB6666_Premultiplied>() { return 18; }
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGBX8888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGBA8888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGBA8888_Premultiplied>() { return 0; }
#else
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGBX8888>() { return 24; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGBA8888>() { return 24; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_RGBA8888_Premultiplied>() { return 24; }
#endif

template<QImage::Format> constexpr QPixelLayout::BPP bitsPerPixel();
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB16>() { return QPixelLayout::BPP16; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB444>() { return QPixelLayout::BPP16; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB555>() { return QPixelLayout::BPP16; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB666>() { return QPixelLayout::BPP24; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB888>() { return QPixelLayout::BPP24; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_BGR888>() { return QPixelLayout::BPP24; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB4444_Premultiplied>() { return QPixelLayout::BPP16; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB8555_Premultiplied>() { return QPixelLayout::BPP24; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB8565_Premultiplied>() { return QPixelLayout::BPP24; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB6666_Premultiplied>() { return QPixelLayout::BPP24; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_RGBX8888>() { return QPixelLayout::BPP32; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_RGBA8888>() { return QPixelLayout::BPP32; }
template<> constexpr QPixelLayout::BPP bitsPerPixel<QImage::Format_RGBA8888_Premultiplied>() { return QPixelLayout::BPP32; }


typedef const uint *(QT_FASTCALL *FetchPixelsFunc)(uint *buffer, const uchar *src, int index, int count);

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

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP64>(const uchar *src, int index)
{
    // We have to do the conversion in fetch to fit into a 32bit uint
    QRgba64 c = reinterpret_cast<const QRgba64 *>(src)[index];
    return c.toArgb32();
}

template <QPixelLayout::BPP bpp>
static quint64 QT_FASTCALL fetchPixel64(const uchar *src, int index)
{
    Q_STATIC_ASSERT(bpp != QPixelLayout::BPP64);
    return fetchPixel<bpp>(src, index);
}

template <QPixelLayout::BPP width> static
void QT_FASTCALL storePixel(uchar *dest, int index, uint pixel);

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

typedef uint (QT_FASTCALL *FetchPixelFunc)(const uchar *src, int index);

static const FetchPixelFunc qFetchPixel[QPixelLayout::BPPCount] = {
    nullptr, // BPPNone
    fetchPixel<QPixelLayout::BPP1MSB>, // BPP1MSB
    fetchPixel<QPixelLayout::BPP1LSB>, // BPP1LSB
    fetchPixel<QPixelLayout::BPP8>, // BPP8
    fetchPixel<QPixelLayout::BPP16>, // BPP16
    fetchPixel<QPixelLayout::BPP24>, // BPP24
    fetchPixel<QPixelLayout::BPP32>, // BPP32
    fetchPixel<QPixelLayout::BPP64> // BPP64
};

template<QImage::Format Format>
static Q_ALWAYS_INLINE uint convertPixelToRGB32(uint s)
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

    uint red   = (s >> redShift<Format>()) & redMask;
    uint green = (s >> greenShift<Format>()) & greenMask;
    uint blue  = (s >> blueShift<Format>()) & blueMask;

    red = ((red << redLeftShift) | (red >> redRightShift)) << 16;
    green = ((green << greenLeftShift) | (green >> greenRightShift)) << 8;
    blue = (blue << blueLeftShift) | (blue >> blueRightShift);
    return 0xff000000 | red | green | blue;
}

template<QImage::Format Format>
static void QT_FASTCALL convertToRGB32(uint *buffer, int count, const QVector<QRgb> *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = convertPixelToRGB32<Format>(buffer[i]);
}

#if defined(__SSE2__) && !defined(__SSSE3__) && QT_COMPILER_SUPPORTS_SSSE3
extern const uint * QT_FASTCALL fetchPixelsBPP24_ssse3(uint *dest, const uchar*src, int index, int count);
#endif

template<QImage::Format Format>
static const uint *QT_FASTCALL fetchRGBToRGB32(uint *buffer, const uchar *src, int index, int count,
                                               const QVector<QRgb> *, QDitherInfo *)
{
    constexpr QPixelLayout::BPP BPP = bitsPerPixel<Format>();
#if defined(__SSE2__) && !defined(__SSSE3__) && QT_COMPILER_SUPPORTS_SSSE3
    if (BPP == QPixelLayout::BPP24 && qCpuHasFeature(SSSE3)) {
        // With SSE2 can convertToRGB32 be vectorized, but it takes SSSE3
        // to vectorize the deforested version below.
        fetchPixelsBPP24_ssse3(buffer, src, index, count);
        convertToRGB32<Format>(buffer, count, nullptr);
        return buffer;
    }
#endif
    for (int i = 0; i < count; ++i)
        buffer[i] = convertPixelToRGB32<Format>(fetchPixel<BPP>(src, index + i));
    return buffer;
}

template<QImage::Format Format>
static Q_ALWAYS_INLINE QRgba64 convertPixelToRGB64(uint s)
{
    return QRgba64::fromArgb32(convertPixelToRGB32<Format>(s));
}

template<QImage::Format Format>
static const QRgba64 *QT_FASTCALL convertToRGB64(QRgba64 *buffer, const uint *src, int count,
                                                 const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = convertPixelToRGB64<Format>(src[i]);
    return buffer;
}

template<QImage::Format Format>
static const QRgba64 *QT_FASTCALL fetchRGBToRGB64(QRgba64 *buffer, const uchar *src, int index, int count,
                                                  const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = convertPixelToRGB64<Format>(fetchPixel<bitsPerPixel<Format>()>(src, index + i));
    return buffer;
}

template<QImage::Format Format>
static Q_ALWAYS_INLINE uint convertPixelToARGB32PM(uint s)
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

    uint alpha = (s >> alphaShift<Format>()) & alphaMask;
    uint red   = (s >> redShift<Format>()) & redMask;
    uint green = (s >> greenShift<Format>()) & greenMask;
    uint blue  = (s >> blueShift<Format>()) & blueMask;

    alpha = (alpha << alphaLeftShift) | (alpha >> alphaRightShift);
    red   = (red << redLeftShift) | (red >> redRightShift);
    green = (green << greenLeftShift) | (green >> greenRightShift);
    blue  = (blue << blueLeftShift) | (blue >> blueRightShift);

    if (mustMin) {
        red   = qMin(alpha, red);
        green = qMin(alpha, green);
        blue  = qMin(alpha, blue);
    }

    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

template<QImage::Format Format>
static void QT_FASTCALL convertARGBPMToARGB32PM(uint *buffer, int count, const QVector<QRgb> *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = convertPixelToARGB32PM<Format>(buffer[i]);
}

template<QImage::Format Format>
static const uint *QT_FASTCALL fetchARGBPMToARGB32PM(uint *buffer, const uchar *src, int index, int count,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    constexpr QPixelLayout::BPP BPP = bitsPerPixel<Format>();
#if defined(__SSE2__) && !defined(__SSSE3__) && QT_COMPILER_SUPPORTS_SSSE3
    if (BPP == QPixelLayout::BPP24 && qCpuHasFeature(SSSE3)) {
        // With SSE2 can convertToRGB32 be vectorized, but it takes SSSE3
        // to vectorize the deforested version below.
        fetchPixelsBPP24_ssse3(buffer, src, index, count);
        convertARGBPMToARGB32PM<Format>(buffer, count, nullptr);
        return buffer;
    }
#endif
    for (int i = 0; i < count; ++i)
        buffer[i] = convertPixelToARGB32PM<Format>(fetchPixel<BPP>(src, index + i));
    return buffer;
}

template<QImage::Format Format>
static Q_ALWAYS_INLINE QRgba64 convertPixelToRGBA64PM(uint s)
{
    return QRgba64::fromArgb32(convertPixelToARGB32PM<Format>(s));
}

template<QImage::Format Format>
static const QRgba64 *QT_FASTCALL convertARGBPMToRGBA64PM(QRgba64 *buffer, const uint *src, int count,
                                                          const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = convertPixelToRGB64<Format>(src[i]);
    return buffer;
}

template<QImage::Format Format>
static const QRgba64 *QT_FASTCALL fetchARGBPMToRGBA64PM(QRgba64 *buffer, const uchar *src, int index, int count,
                                                        const QVector<QRgb> *, QDitherInfo *)
{
    constexpr QPixelLayout::BPP bpp = bitsPerPixel<Format>();
    for (int i = 0; i < count; ++i)
        buffer[i] = convertPixelToRGBA64PM<Format>(fetchPixel<bpp>(src, index + i));
    return buffer;
}

template<QImage::Format Format, bool fromRGB>
static void QT_FASTCALL storeRGBFromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                             const QVector<QRgb> *, QDitherInfo *dither)
{
    Q_CONSTEXPR uchar rWidth = redWidth<Format>();
    Q_CONSTEXPR uchar gWidth = greenWidth<Format>();
    Q_CONSTEXPR uchar bWidth = blueWidth<Format>();
    constexpr QPixelLayout::BPP BPP = bitsPerPixel<Format>();

    // RGB32 -> RGB888 is not a precision loss.
    if (!dither || (rWidth == 8 && gWidth == 8 && bWidth == 8)) {
        Q_CONSTEXPR uint rMask = (1 << redWidth<Format>()) - 1;
        Q_CONSTEXPR uint gMask = (1 << greenWidth<Format>()) - 1;
        Q_CONSTEXPR uint bMask = (1 << blueWidth<Format>()) - 1;
        Q_CONSTEXPR uchar rRightShift = 24 - redWidth<Format>();
        Q_CONSTEXPR uchar gRightShift = 16 - greenWidth<Format>();
        Q_CONSTEXPR uchar bRightShift =  8 - blueWidth<Format>();

        for (int i = 0; i < count; ++i) {
            const uint c = fromRGB ? src[i] : qUnpremultiply(src[i]);
            const uint r = ((c >> rRightShift) & rMask) << redShift<Format>();
            const uint g = ((c >> gRightShift) & gMask) << greenShift<Format>();
            const uint b = ((c >> bRightShift) & bMask) << blueShift<Format>();
            storePixel<BPP>(dest, index + i, r | g | b);
        };
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
            const uint s = (r << redShift<Format>())
                         | (g << greenShift<Format>())
                         | (b << blueShift<Format>());
            storePixel<BPP>(dest, index + i, s);
        }
    }
}

template<QImage::Format Format, bool fromRGB>
static void QT_FASTCALL storeARGBPMFromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                                const QVector<QRgb> *, QDitherInfo *dither)
{
    constexpr QPixelLayout::BPP BPP = bitsPerPixel<Format>();
    if (!dither) {
        Q_CONSTEXPR uint aMask = (1 << alphaWidth<Format>()) - 1;
        Q_CONSTEXPR uint rMask = (1 << redWidth<Format>()) - 1;
        Q_CONSTEXPR uint gMask = (1 << greenWidth<Format>()) - 1;
        Q_CONSTEXPR uint bMask = (1 << blueWidth<Format>()) - 1;

        Q_CONSTEXPR uchar aRightShift = 32 - alphaWidth<Format>();
        Q_CONSTEXPR uchar rRightShift = 24 - redWidth<Format>();
        Q_CONSTEXPR uchar gRightShift = 16 - greenWidth<Format>();
        Q_CONSTEXPR uchar bRightShift =  8 - blueWidth<Format>();

        Q_CONSTEXPR uint aOpaque = aMask << alphaShift<Format>();
        for (int i = 0; i < count; ++i) {
            const uint c = src[i];
            const uint a = fromRGB ? aOpaque : (((c >> aRightShift) & aMask) << alphaShift<Format>());
            const uint r = ((c >> rRightShift) & rMask) << redShift<Format>();
            const uint g = ((c >> gRightShift) & gMask) << greenShift<Format>();
            const uint b = ((c >> bRightShift) & bMask) << blueShift<Format>();
            storePixel<BPP>(dest, index + i, a | r | g | b);
        };
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
            uint s = (a << alphaShift<Format>())
                   | (r << redShift<Format>())
                   | (g << greenShift<Format>())
                   | (b << blueShift<Format>());
            storePixel<BPP>(dest, index + i, s);
        }
    }
}

template<QImage::Format Format>
static void QT_FASTCALL rbSwap(uchar *dst, const uchar *src, int count)
{
    Q_CONSTEXPR uchar aWidth = alphaWidth<Format>();
    Q_CONSTEXPR uchar aShift = alphaShift<Format>();
    Q_CONSTEXPR uchar rWidth = redWidth<Format>();
    Q_CONSTEXPR uchar rShift = redShift<Format>();
    Q_CONSTEXPR uchar gWidth = greenWidth<Format>();
    Q_CONSTEXPR uchar gShift = greenShift<Format>();
    Q_CONSTEXPR uchar bWidth = blueWidth<Format>();
    Q_CONSTEXPR uchar bShift = blueShift<Format>();
#ifdef Q_COMPILER_CONSTEXPR
    Q_STATIC_ASSERT(rWidth == bWidth);
#endif
    Q_CONSTEXPR uint redBlueMask = (1 << rWidth) - 1;
    Q_CONSTEXPR uint alphaGreenMask = (((1 << aWidth) - 1) << aShift)
                                    | (((1 << gWidth) - 1) << gShift);
    constexpr QPixelLayout::BPP bpp = bitsPerPixel<Format>();

    for (int i = 0; i < count; ++i) {
        const uint c = fetchPixel<bpp>(src, i);
        const uint r = (c >> rShift) & redBlueMask;
        const uint b = (c >> bShift) & redBlueMask;
        const uint t = (c & alphaGreenMask)
                     | (r << bShift)
                     | (b << rShift);
        storePixel<bpp>(dst, i, t);
    }
}

static void QT_FASTCALL rbSwap_rgb32(uchar *d, const uchar *s, int count)
{
    const uint *src = reinterpret_cast<const uint *>(s);
    uint *dest = reinterpret_cast<uint *>(d);
    for (int i = 0; i < count; ++i) {
        const uint c = src[i];
        const uint ag = c & 0xff00ff00;
        const uint rb = c & 0x00ff00ff;
        dest[i] = ag | (rb << 16) | (rb >> 16);
    }
}

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
template<>
void QT_FASTCALL rbSwap<QImage::Format_RGBA8888>(uchar *d, const uchar *s, int count)
{
    return rbSwap_rgb32(d, s, count);
}
#else
template<>
void QT_FASTCALL rbSwap<QImage::Format_RGBA8888>(uchar *d, const uchar *s, int count)
{
    const uint *src = reinterpret_cast<const uint *>(s);
    uint *dest = reinterpret_cast<uint *>(d);
    for (int i = 0; i < count; ++i) {
        const uint c = src[i];
        const uint rb = c & 0xff00ff00;
        const uint ga = c & 0x00ff00ff;
        dest[i] = ga | (rb << 16) | (rb >> 16);
    }
}
#endif

static void QT_FASTCALL rbSwap_rgb30(uchar *d, const uchar *s, int count)
{
    const uint *src = reinterpret_cast<const uint *>(s);
    uint *dest = reinterpret_cast<uint *>(d);
    UNALIASED_CONVERSION_LOOP(dest, src, count, qRgbSwapRgb30);
}

template<QImage::Format Format> Q_DECL_CONSTEXPR static inline QPixelLayout pixelLayoutRGB()
{
    return QPixelLayout{
        false,
        false,
        bitsPerPixel<Format>(),
        rbSwap<Format>,
        convertToRGB32<Format>,
        convertToRGB64<Format>,
        fetchRGBToRGB32<Format>,
        fetchRGBToRGB64<Format>,
        storeRGBFromARGB32PM<Format, false>,
        storeRGBFromARGB32PM<Format, true>
    };
}

template<QImage::Format Format> Q_DECL_CONSTEXPR static inline QPixelLayout pixelLayoutARGBPM()
{
    return QPixelLayout{
        true,
        true,
        bitsPerPixel<Format>(),
        rbSwap<Format>,
        convertARGBPMToARGB32PM<Format>,
        convertARGBPMToRGBA64PM<Format>,
        fetchARGBPMToARGB32PM<Format>,
        fetchARGBPMToRGBA64PM<Format>,
        storeARGBPMFromARGB32PM<Format, false>,
        storeARGBPMFromARGB32PM<Format, true>
    };
}

static void QT_FASTCALL convertIndexedToARGB32PM(uint *buffer, int count, const QVector<QRgb> *clut)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qPremultiply(clut->at(buffer[i]));
}

template<QPixelLayout::BPP BPP>
static const uint *QT_FASTCALL fetchIndexedToARGB32PM(uint *buffer, const uchar *src, int index, int count,
                                                      const QVector<QRgb> *clut, QDitherInfo *)
{
    for (int i = 0; i < count; ++i) {
        const uint s = fetchPixel<BPP>(src, index + i);
        buffer[i] = qPremultiply(clut->at(s));
    }
    return buffer;
}

template<QPixelLayout::BPP BPP>
static const QRgba64 *QT_FASTCALL fetchIndexedToRGBA64PM(QRgba64 *buffer, const uchar *src, int index, int count,
                                                         const QVector<QRgb> *clut, QDitherInfo *)
{
    for (int i = 0; i < count; ++i) {
        const uint s = fetchPixel<BPP>(src, index + i);
        buffer[i] = QRgba64::fromArgb32(clut->at(s)).premultiplied();
    }
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertIndexedToRGBA64PM(QRgba64 *buffer, const uint *src, int count,
                                                           const QVector<QRgb> *clut, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(clut->at(src[i])).premultiplied();
    return buffer;
}

static void QT_FASTCALL convertPassThrough(uint *, int, const QVector<QRgb> *)
{
}

static const uint *QT_FASTCALL fetchPassThrough(uint *, const uchar *src, int index, int,
                                                const QVector<QRgb> *, QDitherInfo *)
{
    return reinterpret_cast<const uint *>(src) + index;
}

static const QRgba64 *QT_FASTCALL fetchPassThrough64(QRgba64 *, const uchar *src, int index, int,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    return reinterpret_cast<const QRgba64 *>(src) + index;
}

static void QT_FASTCALL storePassThrough(uchar *dest, const uint *src, int index, int count,
                                         const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    if (d != src)
        memcpy(d, src, count * sizeof(uint));
}

static void QT_FASTCALL convertARGB32ToARGB32PM(uint *buffer, int count, const QVector<QRgb> *)
{
    qt_convertARGB32ToARGB32PM(buffer, buffer, count);
}

static const uint *QT_FASTCALL fetchARGB32ToARGB32PM(uint *buffer, const uchar *src, int index, int count,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    return qt_convertARGB32ToARGB32PM(buffer, reinterpret_cast<const uint *>(src) + index, count);
}

static void QT_FASTCALL convertRGBA8888PMToARGB32PM(uint *buffer, int count, const QVector<QRgb> *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = RGBA2ARGB(buffer[i]);
}

static const uint *QT_FASTCALL fetchRGBA8888PMToARGB32PM(uint *buffer, const uchar *src, int index, int count,
                                                         const QVector<QRgb> *, QDitherInfo *)
{
    const uint *s  = reinterpret_cast<const uint *>(src) + index;
    UNALIASED_CONVERSION_LOOP(buffer, s, count, RGBA2ARGB);
    return buffer;
}

static void QT_FASTCALL convertRGBA8888ToARGB32PM(uint *buffer, int count, const QVector<QRgb> *)
{
    qt_convertRGBA8888ToARGB32PM(buffer, buffer, count);
}

static const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM(uint *buffer, const uchar *src, int index, int count,
                                                       const QVector<QRgb> *, QDitherInfo *)
{
    return qt_convertRGBA8888ToARGB32PM(buffer, reinterpret_cast<const uint *>(src) + index, count);
}

static void QT_FASTCALL convertAlpha8ToRGB32(uint *buffer, int count, const QVector<QRgb> *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qRgba(0, 0, 0, buffer[i]);
}

static const uint *QT_FASTCALL fetchAlpha8ToRGB32(uint *buffer, const uchar *src, int index, int count,
                                                  const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qRgba(0, 0, 0, src[index + i]);
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertAlpha8ToRGB64(QRgba64 *buffer, const uint *src, int count,
                                                       const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromRgba(0, 0, 0, src[i]);
    return buffer;
}
static const QRgba64 *QT_FASTCALL fetchAlpha8ToRGB64(QRgba64 *buffer, const uchar *src, int index, int count,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromRgba(0, 0, 0, src[index + i]);
    return buffer;
}

static void QT_FASTCALL convertGrayscale8ToRGB32(uint *buffer, int count, const QVector<QRgb> *)
{
    for (int i = 0; i < count; ++i) {
        const uint s = buffer[i];
        buffer[i] = qRgb(s, s, s);
    }
}

static const uint *QT_FASTCALL fetchGrayscale8ToRGB32(uint *buffer, const uchar *src, int index, int count,
                                                      const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i) {
        const uint s = src[index + i];
        buffer[i] = qRgb(s, s, s);
    }
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertGrayscale8ToRGB64(QRgba64 *buffer, const uint *src, int count,
                                                           const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromRgba(src[i], src[i], src[i], 255);
    return buffer;
}

static const QRgba64 *QT_FASTCALL fetchGrayscale8ToRGB64(QRgba64 *buffer, const uchar *src, int index, int count,
                                                         const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i) {
        const uint s = src[index + i];
        buffer[i] = QRgba64::fromRgba(s, s, s, 255);
    }
    return buffer;
}

static void QT_FASTCALL convertGrayscale16ToRGB32(uint *buffer, int count, const QVector<QRgb> *)
{
    for (int i = 0; i < count; ++i) {
        const uint x = qt_div_257(buffer[i]);
        buffer[i] = qRgb(x, x, x);
    }
}

static const uint *QT_FASTCALL fetchGrayscale16ToRGB32(uint *buffer, const uchar *src, int index, int count,
                                                      const QVector<QRgb> *, QDitherInfo *)
{
    const unsigned short *s = reinterpret_cast<const unsigned short *>(src) + index;
    for (int i = 0; i < count; ++i) {
        const uint x = qt_div_257(s[i]);
        buffer[i] = qRgb(x, x, x);
    }
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertGrayscale16ToRGBA64(QRgba64 *buffer, const uint *src, int count,
                                                           const QVector<QRgb> *, QDitherInfo *)
{
    const unsigned short *s = reinterpret_cast<const unsigned short *>(src);
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromRgba64(s[i], s[i], s[i], 65535);
    return buffer;
}

static const QRgba64 *QT_FASTCALL fetchGrayscale16ToRGBA64(QRgba64 *buffer, const uchar *src, int index, int count,
                                                         const QVector<QRgb> *, QDitherInfo *)
{
    const unsigned short *s = reinterpret_cast<const unsigned short *>(src) + index;
    for (int i = 0; i < count; ++i) {
        buffer[i] = QRgba64::fromRgba64(s[i], s[i], s[i], 65535);
    }
    return buffer;
}

static void QT_FASTCALL storeARGB32FromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                                const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    UNALIASED_CONVERSION_LOOP(d, src, count, [](uint c) { return qUnpremultiply(c); });
}

static void QT_FASTCALL storeRGBA8888PMFromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                                    const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    UNALIASED_CONVERSION_LOOP(d, src, count, ARGB2RGBA);
}

#ifdef __SSE2__
template<bool RGBA, bool maskAlpha>
static inline void qConvertARGB32PMToRGBA64PM_sse2(QRgba64 *buffer, const uint *src, int count)
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

template<QtPixelOrder PixelOrder>
static inline void qConvertRGBA64PMToA2RGB30PM_sse2(uint *dest, const QRgba64 *buffer, int count)
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
    qConvertARGB32PMToRGBA64PM_sse2<false, true>(buffer, src, count);
#elif defined(__ARM_NEON__)
    qConvertARGB32PMToRGBA64PM_neon<false, true>(buffer, src, count);
#else
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(0xff000000 | src[i]);
#endif
    return buffer;
}

static const QRgba64 *QT_FASTCALL fetchRGB32ToRGB64(QRgba64 *buffer, const uchar *src, int index, int count,
                                                    const QVector<QRgb> *, QDitherInfo *)
{
    return convertRGB32ToRGB64(buffer, reinterpret_cast<const uint *>(src) + index, count, nullptr, nullptr);
}

static const QRgba64 *QT_FASTCALL convertARGB32ToRGBA64PM(QRgba64 *buffer, const uint *src, int count,
                                                          const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(src[i]).premultiplied();
    return buffer;
}

static const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM(QRgba64 *buffer, const uchar *src, int index, int count,
                                                        const QVector<QRgb> *, QDitherInfo *)
{
    return convertARGB32ToRGBA64PM(buffer, reinterpret_cast<const uint *>(src) + index, count, nullptr, nullptr);
}

static const QRgba64 *QT_FASTCALL convertARGB32PMToRGBA64PM(QRgba64 *buffer, const uint *src, int count,
                                                            const QVector<QRgb> *, QDitherInfo *)
{
#ifdef __SSE2__
    qConvertARGB32PMToRGBA64PM_sse2<false, false>(buffer, src, count);
#elif defined(__ARM_NEON__)
    qConvertARGB32PMToRGBA64PM_neon<false, false>(buffer, src, count);
#else
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(src[i]);
#endif
    return buffer;
}

static const QRgba64 *QT_FASTCALL fetchARGB32PMToRGBA64PM(QRgba64 *buffer, const uchar *src, int index, int count,
                                                          const QVector<QRgb> *, QDitherInfo *)
{
    return convertARGB32PMToRGBA64PM(buffer, reinterpret_cast<const uint *>(src) + index, count, nullptr, nullptr);
}

#if QT_CONFIG(raster_64bit)
static void convertRGBA64ToRGBA64PM(QRgba64 *buffer, int count)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = buffer[i].premultiplied();
}

static void convertRGBA64PMToRGBA64PM(QRgba64 *, int)
{
}
#endif

static const QRgba64 *QT_FASTCALL fetchRGBA64ToRGBA64PM(QRgba64 *buffer, const uchar *src, int index, int count,
                                                        const QVector<QRgb> *, QDitherInfo *)
{
    const QRgba64 *s = reinterpret_cast<const QRgba64 *>(src) + index;
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromRgba64(s[i]).premultiplied();
    return buffer;
}

static const QRgba64 *QT_FASTCALL convertRGBA8888ToRGBA64PM(QRgba64 *buffer, const uint *src, int count,
                                                            const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(RGBA2ARGB(src[i])).premultiplied();
    return buffer;
}

static const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM(QRgba64 *buffer, const uchar *src, int index, int count,
                                                          const QVector<QRgb> *, QDitherInfo *)
{
    return convertRGBA8888ToRGBA64PM(buffer, reinterpret_cast<const uint *>(src) + index, count, nullptr, nullptr);
}

static const QRgba64 *QT_FASTCALL convertRGBA8888PMToRGBA64PM(QRgba64 *buffer, const uint *src, int count,
                                                              const QVector<QRgb> *, QDitherInfo *)
{
#ifdef __SSE2__
    qConvertARGB32PMToRGBA64PM_sse2<true, false>(buffer, src, count);
#elif defined(__ARM_NEON__)
    qConvertARGB32PMToRGBA64PM_neon<true, false>(buffer, src, count);
#else
    for (int i = 0; i < count; ++i)
        buffer[i] = QRgba64::fromArgb32(RGBA2ARGB(src[i]));
#endif
    return buffer;
}

static const QRgba64 *QT_FASTCALL fetchRGBA8888PMToRGBA64PM(QRgba64 *buffer, const uchar *src, int index, int count,
                                                            const QVector<QRgb> *, QDitherInfo *)
{
    return convertRGBA8888PMToRGBA64PM(buffer, reinterpret_cast<const uint *>(src) + index, count, nullptr, nullptr);
}

static void QT_FASTCALL storeRGBA8888FromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                                  const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    UNALIASED_CONVERSION_LOOP(d, src, count, [](uint c) { return ARGB2RGBA(qUnpremultiply(c)); });
}

static void QT_FASTCALL storeRGBXFromRGB32(uchar *dest, const uint *src, int index, int count,
                                           const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    UNALIASED_CONVERSION_LOOP(d, src, count, [](uint c) { return ARGB2RGBA(0xff000000 | c); });
}

static void QT_FASTCALL storeRGBXFromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                              const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    UNALIASED_CONVERSION_LOOP(d, src, count, [](uint c) { return ARGB2RGBA(0xff000000 | qUnpremultiply(c)); });
}

template<QtPixelOrder PixelOrder>
static void QT_FASTCALL convertA2RGB30PMToARGB32PM(uint *buffer, int count, const QVector<QRgb> *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qConvertA2rgb30ToArgb32<PixelOrder>(buffer[i]);
}

template<QtPixelOrder PixelOrder>
static const uint *QT_FASTCALL fetchA2RGB30PMToARGB32PM(uint *buffer, const uchar *s, int index, int count,
                                                        const QVector<QRgb> *, QDitherInfo *dither)
{
    const uint *src = reinterpret_cast<const uint *>(s) + index;
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
static inline void qConvertA2RGB30PMToRGBA64PM_sse2(QRgba64 *buffer, const uint *src, int count)
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
static const QRgba64 *QT_FASTCALL convertA2RGB30PMToRGBA64PM(QRgba64 *buffer, const uint *src, int count,
                                                             const QVector<QRgb> *, QDitherInfo *)
{
#ifdef __SSE2__
    qConvertA2RGB30PMToRGBA64PM_sse2<PixelOrder>(buffer, src, count);
#else
    for (int i = 0; i < count; ++i)
        buffer[i] = qConvertA2rgb30ToRgb64<PixelOrder>(src[i]);
#endif
    return buffer;
}

template<QtPixelOrder PixelOrder>
static const QRgba64 *QT_FASTCALL fetchA2RGB30PMToRGBA64PM(QRgba64 *buffer, const uchar *src, int index, int count,
                                                           const QVector<QRgb> *, QDitherInfo *)
{
    return convertA2RGB30PMToRGBA64PM<PixelOrder>(buffer, reinterpret_cast<const uint *>(src) + index, count, nullptr, nullptr);
}

template<QtPixelOrder PixelOrder>
static void QT_FASTCALL storeA2RGB30PMFromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                                   const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    UNALIASED_CONVERSION_LOOP(d, src, count, qConvertArgb32ToA2rgb30<PixelOrder>);
}

template<QtPixelOrder PixelOrder>
static void QT_FASTCALL storeRGB30FromRGB32(uchar *dest, const uint *src, int index, int count,
                                            const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    UNALIASED_CONVERSION_LOOP(d, src, count, qConvertRgb32ToRgb30<PixelOrder>);
}

template<QtPixelOrder PixelOrder>
static void QT_FASTCALL storeRGB30FromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                               const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = reinterpret_cast<uint *>(dest) + index;
    UNALIASED_CONVERSION_LOOP(d, src, count, qConvertRgb32ToRgb30<PixelOrder>);
}

template<bool RGBA>
void qt_convertRGBA64ToARGB32(uint *dst, const QRgba64 *src, int count)
{
    int i = 0;
#ifdef __SSE2__
    if (((uintptr_t)dst & 0x7) && count > 0) {
        uint s = (*src++).toArgb32();
        if (RGBA)
            s = ARGB2RGBA(s);
        *dst++ = s;
        i++;
    }
    const __m128i vhalf = _mm_set1_epi32(0x80);
    const __m128i vzero = _mm_setzero_si128();
    for (; i < count-1; i += 2) {
        __m128i vs = _mm_loadu_si128((const __m128i*)src);
        src += 2;
        if (!RGBA) {
            vs = _mm_shufflelo_epi16(vs, _MM_SHUFFLE(3, 0, 1, 2));
            vs = _mm_shufflehi_epi16(vs, _MM_SHUFFLE(3, 0, 1, 2));
        }
        __m128i v1 = _mm_unpacklo_epi16(vs, vzero);
        __m128i v2 = _mm_unpackhi_epi16(vs, vzero);
        v1 = _mm_add_epi32(v1, vhalf);
        v2 = _mm_add_epi32(v2, vhalf);
        v1 = _mm_sub_epi32(v1, _mm_srli_epi32(v1, 8));
        v2 = _mm_sub_epi32(v2, _mm_srli_epi32(v2, 8));
        v1 = _mm_srli_epi32(v1, 8);
        v2 = _mm_srli_epi32(v2, 8);
        v1 = _mm_packs_epi32(v1, v2);
        v1 = _mm_packus_epi16(v1, vzero);
        _mm_storel_epi64((__m128i*)(dst), v1);
        dst += 2;
    }
#endif
    for (; i < count; i++) {
        uint s = (*src++).toArgb32();
        if (RGBA)
            s = ARGB2RGBA(s);
        *dst++ = s;
    }
}
template void qt_convertRGBA64ToARGB32<false>(uint *dst, const QRgba64 *src, int count);
template void qt_convertRGBA64ToARGB32<true>(uint *dst, const QRgba64 *src, int count);


static void QT_FASTCALL storeAlpha8FromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                                const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        dest[index + i] = qAlpha(src[i]);
}

static void QT_FASTCALL storeGrayscale8FromRGB32(uchar *dest, const uint *src, int index, int count,
                                                 const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        dest[index + i] = qGray(src[i]);
}

static void QT_FASTCALL storeGrayscale8FromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                                    const QVector<QRgb> *, QDitherInfo *)
{
    for (int i = 0; i < count; ++i)
        dest[index + i] = qGray(qUnpremultiply(src[i]));
}

static void QT_FASTCALL storeGrayscale16FromRGB32(uchar *dest, const uint *src, int index, int count,
                                                 const QVector<QRgb> *, QDitherInfo *)
{
    unsigned short *d = reinterpret_cast<unsigned short *>(dest) + index;
    for (int i = 0; i < count; ++i)
        d[i] = qGray(src[i]) * 257;
}

static void QT_FASTCALL storeGrayscale16FromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                                    const QVector<QRgb> *, QDitherInfo *)
{
    unsigned short *d = reinterpret_cast<unsigned short *>(dest) + index;
    for (int i = 0; i < count; ++i)
        d[i] = qGray(qUnpremultiply(src[i])) * 257;
}

static const uint *QT_FASTCALL fetchRGB64ToRGB32(uint *buffer, const uchar *src, int index, int count,
                                                 const QVector<QRgb> *, QDitherInfo *)
{
    const QRgba64 *s = reinterpret_cast<const QRgba64 *>(src) + index;
    for (int i = 0; i < count; ++i)
        buffer[i] = toArgb32(s[i]);
    return buffer;
}

static void QT_FASTCALL storeRGB64FromRGB32(uchar *dest, const uint *src, int index, int count,
                                            const QVector<QRgb> *, QDitherInfo *)
{
    QRgba64 *d = reinterpret_cast<QRgba64 *>(dest) + index;
    for (int i = 0; i < count; ++i)
        d[i] = QRgba64::fromArgb32(src[i]);
}

static const uint *QT_FASTCALL fetchRGBA64ToARGB32PM(uint *buffer, const uchar *src, int index, int count,
                                                     const QVector<QRgb> *, QDitherInfo *)
{
    const QRgba64 *s = reinterpret_cast<const QRgba64 *>(src) + index;
    for (int i = 0; i < count; ++i)
        buffer[i] = toArgb32(s[i].premultiplied());
    return buffer;
}

static void QT_FASTCALL storeRGBA64FromARGB32PM(uchar *dest, const uint *src, int index, int count,
                                                const QVector<QRgb> *, QDitherInfo *)
{
    QRgba64 *d = reinterpret_cast<QRgba64 *>(dest) + index;
    for (int i = 0; i < count; ++i)
        d[i] = QRgba64::fromArgb32(src[i]).unpremultiplied();
}

// Note:
// convertToArgb32() assumes that no color channel is less than 4 bits.
// storeRGBFromARGB32PM() assumes that no color channel is more than 8 bits.
// QImage::rgbSwapped() assumes that the red and blue color channels have the same number of bits.
QPixelLayout qPixelLayouts[QImage::NImageFormats] = {
    { false, false, QPixelLayout::BPPNone, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }, // Format_Invalid
    { false, false, QPixelLayout::BPP1MSB, nullptr,
      convertIndexedToARGB32PM, convertIndexedToRGBA64PM,
      fetchIndexedToARGB32PM<QPixelLayout::BPP1MSB>, fetchIndexedToRGBA64PM<QPixelLayout::BPP1MSB>,
      nullptr, nullptr }, // Format_Mono
    { false, false, QPixelLayout::BPP1LSB, nullptr,
      convertIndexedToARGB32PM, convertIndexedToRGBA64PM,
      fetchIndexedToARGB32PM<QPixelLayout::BPP1LSB>, fetchIndexedToRGBA64PM<QPixelLayout::BPP1LSB>,
      nullptr, nullptr }, // Format_MonoLSB
    { false, false, QPixelLayout::BPP8, nullptr,
      convertIndexedToARGB32PM, convertIndexedToRGBA64PM,
      fetchIndexedToARGB32PM<QPixelLayout::BPP8>, fetchIndexedToRGBA64PM<QPixelLayout::BPP8>,
      nullptr, nullptr }, // Format_Indexed8
    // Technically using convertPassThrough to convert from ARGB32PM to RGB32 is wrong,
    // but everywhere this generic conversion would be wrong is currently overloaded.
    { false, false, QPixelLayout::BPP32, rbSwap_rgb32, convertPassThrough,
      convertRGB32ToRGB64, fetchPassThrough, fetchRGB32ToRGB64, storePassThrough, storePassThrough }, // Format_RGB32
    { true, false, QPixelLayout::BPP32, rbSwap_rgb32, convertARGB32ToARGB32PM,
      convertARGB32ToRGBA64PM, fetchARGB32ToARGB32PM, fetchARGB32ToRGBA64PM, storeARGB32FromARGB32PM, storePassThrough }, // Format_ARGB32
    { true, true, QPixelLayout::BPP32, rbSwap_rgb32, convertPassThrough,
      convertARGB32PMToRGBA64PM, fetchPassThrough, fetchARGB32PMToRGBA64PM, storePassThrough, storePassThrough }, // Format_ARGB32_Premultiplied
    pixelLayoutRGB<QImage::Format_RGB16>(),
    pixelLayoutARGBPM<QImage::Format_ARGB8565_Premultiplied>(),
    pixelLayoutRGB<QImage::Format_RGB666>(),
    pixelLayoutARGBPM<QImage::Format_ARGB6666_Premultiplied>(),
    pixelLayoutRGB<QImage::Format_RGB555>(),
    pixelLayoutARGBPM<QImage::Format_ARGB8555_Premultiplied>(),
    pixelLayoutRGB<QImage::Format_RGB888>(),
    pixelLayoutRGB<QImage::Format_RGB444>(),
    pixelLayoutARGBPM<QImage::Format_ARGB4444_Premultiplied>(),
    { false, false, QPixelLayout::BPP32, rbSwap<QImage::Format_RGBA8888>, convertRGBA8888PMToARGB32PM,
      convertRGBA8888PMToRGBA64PM, fetchRGBA8888PMToARGB32PM, fetchRGBA8888PMToRGBA64PM, storeRGBXFromARGB32PM, storeRGBXFromRGB32 }, // Format_RGBX8888
    { true, false, QPixelLayout::BPP32, rbSwap<QImage::Format_RGBA8888>, convertRGBA8888ToARGB32PM,
      convertRGBA8888ToRGBA64PM, fetchRGBA8888ToARGB32PM, fetchRGBA8888ToRGBA64PM, storeRGBA8888FromARGB32PM, storeRGBXFromRGB32 }, // Format_RGBA8888
    { true, true, QPixelLayout::BPP32, rbSwap<QImage::Format_RGBA8888>, convertRGBA8888PMToARGB32PM,
      convertRGBA8888PMToRGBA64PM, fetchRGBA8888PMToARGB32PM, fetchRGBA8888PMToRGBA64PM, storeRGBA8888PMFromARGB32PM, storeRGBXFromRGB32 },  // Format_RGBA8888_Premultiplied
    { false, false, QPixelLayout::BPP32, rbSwap_rgb30,
      convertA2RGB30PMToARGB32PM<PixelOrderBGR>,
      convertA2RGB30PMToRGBA64PM<PixelOrderBGR>,
      fetchA2RGB30PMToARGB32PM<PixelOrderBGR>,
      fetchA2RGB30PMToRGBA64PM<PixelOrderBGR>,
      storeRGB30FromARGB32PM<PixelOrderBGR>,
      storeRGB30FromRGB32<PixelOrderBGR>
    }, // Format_BGR30
    { true, true, QPixelLayout::BPP32, rbSwap_rgb30,
      convertA2RGB30PMToARGB32PM<PixelOrderBGR>,
      convertA2RGB30PMToRGBA64PM<PixelOrderBGR>,
      fetchA2RGB30PMToARGB32PM<PixelOrderBGR>,
      fetchA2RGB30PMToRGBA64PM<PixelOrderBGR>,
      storeA2RGB30PMFromARGB32PM<PixelOrderBGR>,
      storeRGB30FromRGB32<PixelOrderBGR>
    },  // Format_A2BGR30_Premultiplied
    { false, false, QPixelLayout::BPP32, rbSwap_rgb30,
      convertA2RGB30PMToARGB32PM<PixelOrderRGB>,
      convertA2RGB30PMToRGBA64PM<PixelOrderRGB>,
      fetchA2RGB30PMToARGB32PM<PixelOrderRGB>,
      fetchA2RGB30PMToRGBA64PM<PixelOrderRGB>,
      storeRGB30FromARGB32PM<PixelOrderRGB>,
      storeRGB30FromRGB32<PixelOrderRGB>
    }, // Format_RGB30
    { true, true, QPixelLayout::BPP32, rbSwap_rgb30,
      convertA2RGB30PMToARGB32PM<PixelOrderRGB>,
      convertA2RGB30PMToRGBA64PM<PixelOrderRGB>,
      fetchA2RGB30PMToARGB32PM<PixelOrderRGB>,
      fetchA2RGB30PMToRGBA64PM<PixelOrderRGB>,
      storeA2RGB30PMFromARGB32PM<PixelOrderRGB>,
      storeRGB30FromRGB32<PixelOrderRGB>
    },  // Format_A2RGB30_Premultiplied
    { true, true, QPixelLayout::BPP8, nullptr,
      convertAlpha8ToRGB32, convertAlpha8ToRGB64,
      fetchAlpha8ToRGB32, fetchAlpha8ToRGB64,
      storeAlpha8FromARGB32PM, nullptr }, // Format_Alpha8
    { false, false, QPixelLayout::BPP8, nullptr,
      convertGrayscale8ToRGB32, convertGrayscale8ToRGB64,
      fetchGrayscale8ToRGB32, fetchGrayscale8ToRGB64,
      storeGrayscale8FromARGB32PM, storeGrayscale8FromRGB32 }, // Format_Grayscale8
    { false, false, QPixelLayout::BPP64, nullptr,
      convertPassThrough, nullptr,
      fetchRGB64ToRGB32, fetchPassThrough64,
      storeRGB64FromRGB32, storeRGB64FromRGB32 }, // Format_RGBX64
    { true, false, QPixelLayout::BPP64, nullptr,
      convertARGB32ToARGB32PM, nullptr,
      fetchRGBA64ToARGB32PM, fetchRGBA64ToRGBA64PM,
      storeRGBA64FromARGB32PM, storeRGB64FromRGB32 }, // Format_RGBA64
    { true, true, QPixelLayout::BPP64, nullptr,
      convertPassThrough, nullptr,
      fetchRGB64ToRGB32, fetchPassThrough64,
      storeRGB64FromRGB32, storeRGB64FromRGB32 }, // Format_RGBA64_Premultiplied
    { false, false, QPixelLayout::BPP16, nullptr,
      convertGrayscale16ToRGB32, convertGrayscale16ToRGBA64,
      fetchGrayscale16ToRGB32, fetchGrayscale16ToRGBA64,
      storeGrayscale16FromARGB32PM, storeGrayscale16FromRGB32 }, // Format_Grayscale16
    pixelLayoutRGB<QImage::Format_BGR888>(),
};

Q_STATIC_ASSERT(sizeof(qPixelLayouts) / sizeof(*qPixelLayouts) == QImage::NImageFormats);

static void QT_FASTCALL convertFromRgb64(uint *dest, const QRgba64 *src, int length)
{
    for (int i = 0; i < length; ++i) {
        dest[i] = toArgb32(src[i]);
    }
}

template<QImage::Format format>
static void QT_FASTCALL storeGenericFromRGBA64PM(uchar *dest, const QRgba64 *src, int index, int count,
                                                 const QVector<QRgb> *clut, QDitherInfo *dither)
{
    uint buffer[BufferSize];
    convertFromRgb64(buffer, src, count);
    qPixelLayouts[format].storeFromARGB32PM(dest, buffer, index, count, clut, dither);
}

static void QT_FASTCALL storeARGB32FromRGBA64PM(uchar *dest, const QRgba64 *src, int index, int count,
                                                const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = (uint*)dest + index;
    for (int i = 0; i < count; ++i)
        d[i] = toArgb32(src[i].unpremultiplied());
}

static void QT_FASTCALL storeRGBA8888FromRGBA64PM(uchar *dest, const QRgba64 *src, int index, int count,
                                                  const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = (uint*)dest + index;
    for (int i = 0; i < count; ++i)
        d[i] = toRgba8888(src[i].unpremultiplied());
}

template<QtPixelOrder PixelOrder>
static void QT_FASTCALL storeRGB30FromRGBA64PM(uchar *dest, const QRgba64 *src, int index, int count,
                                               const QVector<QRgb> *, QDitherInfo *)
{
    uint *d = (uint*)dest + index;
#ifdef __SSE2__
    qConvertRGBA64PMToA2RGB30PM_sse2<PixelOrder>(d, src, count);
#else
    for (int i = 0; i < count; ++i)
        d[i] = qConvertRgb64ToRgb30<PixelOrder>(src[i]);
#endif
}

static void QT_FASTCALL storeRGBX64FromRGBA64PM(uchar *dest, const QRgba64 *src, int index, int count,
                                                const QVector<QRgb> *, QDitherInfo *)
{
    QRgba64 *d = reinterpret_cast<QRgba64*>(dest) + index;
    for (int i = 0; i < count; ++i) {
        d[i] = src[i].unpremultiplied();
        d[i].setAlpha(65535);
    }
}

static void QT_FASTCALL storeRGBA64FromRGBA64PM(uchar *dest, const QRgba64 *src, int index, int count,
                                                const QVector<QRgb> *, QDitherInfo *)
{
    QRgba64 *d = reinterpret_cast<QRgba64*>(dest) + index;
    for (int i = 0; i < count; ++i)
        d[i] = src[i].unpremultiplied();
}

static void QT_FASTCALL storeRGBA64PMFromRGBA64PM(uchar *dest, const QRgba64 *src, int index, int count,
                                                  const QVector<QRgb> *, QDitherInfo *)
{
    QRgba64 *d = reinterpret_cast<QRgba64*>(dest) + index;
    if (d != src)
        memcpy(d, src, count * sizeof(QRgba64));
}

static void QT_FASTCALL storeGray16FromRGBA64PM(uchar *dest, const QRgba64 *src, int index, int count,
                                                const QVector<QRgb> *, QDitherInfo *)
{
    quint16 *d = reinterpret_cast<quint16*>(dest) + index;
    for (int i = 0; i < count; ++i) {
        QRgba64 s =  src[i].unpremultiplied();
        d[i] = qGray(s.red(), s.green(), s.blue());
    }
}

ConvertAndStorePixelsFunc64 qStoreFromRGBA64PM[QImage::NImageFormats] = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    storeGenericFromRGBA64PM<QImage::Format_RGB32>,
    storeARGB32FromRGBA64PM,
    storeGenericFromRGBA64PM<QImage::Format_ARGB32_Premultiplied>,
    storeGenericFromRGBA64PM<QImage::Format_RGB16>,
    storeGenericFromRGBA64PM<QImage::Format_ARGB8565_Premultiplied>,
    storeGenericFromRGBA64PM<QImage::Format_RGB666>,
    storeGenericFromRGBA64PM<QImage::Format_ARGB6666_Premultiplied>,
    storeGenericFromRGBA64PM<QImage::Format_RGB555>,
    storeGenericFromRGBA64PM<QImage::Format_ARGB8555_Premultiplied>,
    storeGenericFromRGBA64PM<QImage::Format_RGB888>,
    storeGenericFromRGBA64PM<QImage::Format_RGB444>,
    storeGenericFromRGBA64PM<QImage::Format_ARGB4444_Premultiplied>,
    storeGenericFromRGBA64PM<QImage::Format_RGBX8888>,
    storeRGBA8888FromRGBA64PM,
    storeGenericFromRGBA64PM<QImage::Format_RGBA8888_Premultiplied>,
    storeRGB30FromRGBA64PM<PixelOrderBGR>,
    storeRGB30FromRGBA64PM<PixelOrderBGR>,
    storeRGB30FromRGBA64PM<PixelOrderRGB>,
    storeRGB30FromRGBA64PM<PixelOrderRGB>,
    storeGenericFromRGBA64PM<QImage::Format_Alpha8>,
    storeGenericFromRGBA64PM<QImage::Format_Grayscale8>,
    storeRGBX64FromRGBA64PM,
    storeRGBA64FromRGBA64PM,
    storeRGBA64PMFromRGBA64PM,
    storeGray16FromRGBA64PM,
    storeGenericFromRGBA64PM<QImage::Format_BGR888>,
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
    return const_cast<uint *>(layout->fetchToARGB32PM(buffer, rasterBuffer->scanLine(y), x, length, nullptr, nullptr));
}

static uint *QT_FASTCALL destFetchUndefined(uint *buffer, QRasterBuffer *, int, int, int)
{
    return buffer;
}

static DestFetchProc destFetchProc[QImage::NImageFormats] =
{
    nullptr,            // Format_Invalid
    destFetchMono,      // Format_Mono,
    destFetchMonoLsb,   // Format_MonoLSB
    nullptr,            // Format_Indexed8
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
    destFetch,          // Format_RGBX64
    destFetch,          // Format_RGBA64
    destFetch,          // Format_RGBA64_Premultiplied
    destFetch,          // Format_Grayscale16
    destFetch,          // Format_BGR888
};

#if QT_CONFIG(raster_64bit)
static QRgba64 *QT_FASTCALL destFetch64(QRgba64 *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[rasterBuffer->format];
    return const_cast<QRgba64 *>(layout->fetchToRGBA64PM(buffer, rasterBuffer->scanLine(y), x, length, nullptr, nullptr));
}

static QRgba64 * QT_FASTCALL destFetchRGB64(QRgba64 *, QRasterBuffer *rasterBuffer, int x, int y, int)
{
    return (QRgba64 *)rasterBuffer->scanLine(y) + x;
}

static QRgba64 * QT_FASTCALL destFetch64Undefined(QRgba64 *buffer, QRasterBuffer *, int, int, int)
{
    return buffer;
}

static DestFetchProc64 destFetchProc64[QImage::NImageFormats] =
{
    nullptr,            // Format_Invalid
    nullptr,            // Format_Mono,
    nullptr,            // Format_MonoLSB
    nullptr,            // Format_Indexed8
    destFetch64,        // Format_RGB32
    destFetch64,        // Format_ARGB32,
    destFetch64,        // Format_ARGB32_Premultiplied
    destFetch64,        // Format_RGB16
    destFetch64,        // Format_ARGB8565_Premultiplied
    destFetch64,        // Format_RGB666
    destFetch64,        // Format_ARGB6666_Premultiplied
    destFetch64,        // Format_RGB555
    destFetch64,        // Format_ARGB8555_Premultiplied
    destFetch64,        // Format_RGB888
    destFetch64,        // Format_RGB444
    destFetch64,        // Format_ARGB4444_Premultiplied
    destFetch64,        // Format_RGBX8888
    destFetch64,        // Format_RGBA8888
    destFetch64,        // Format_RGBA8888_Premultiplied
    destFetch64,        // Format_BGR30
    destFetch64,        // Format_A2BGR30_Premultiplied
    destFetch64,        // Format_RGB30
    destFetch64,        // Format_A2RGB30_Premultiplied
    destFetch64,        // Format_Alpha8
    destFetch64,        // Format_Grayscale8
    destFetchRGB64,     // Format_RGBX64
    destFetch64,        // Format_RGBA64
    destFetchRGB64,     // Format_RGBA64_Premultiplied
    destFetch64,        // Format_Grayscale16
    destFetch64,        // Format_BGR888
};
#endif

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
    const QPixelLayout *layout = &qPixelLayouts[rasterBuffer->format];
    ConvertAndStorePixelsFunc store = layout->storeFromARGB32PM;
    if (!layout->premultiplied && !layout->hasAlphaChannel)
        store = layout->storeFromRGB32;
    uchar *dest = rasterBuffer->scanLine(y);
    store(dest, buffer, x, length, nullptr, nullptr);
}

static DestStoreProc destStoreProc[QImage::NImageFormats] =
{
    nullptr,            // Format_Invalid
    destStoreMono,      // Format_Mono,
    destStoreMonoLsb,   // Format_MonoLSB
    nullptr,            // Format_Indexed8
    nullptr,            // Format_RGB32
    destStore,          // Format_ARGB32,
    nullptr,            // Format_ARGB32_Premultiplied
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
    destStore,          // Format_RGBX64
    destStore,          // Format_RGBA64
    destStore,          // Format_RGBA64_Premultiplied
    destStore,          // Format_Grayscale16
    destStore,          // Format_BGR888
};

#if QT_CONFIG(raster_64bit)
static void QT_FASTCALL destStore64(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length)
{
    auto store = qStoreFromRGBA64PM[rasterBuffer->format];
    uchar *dest = rasterBuffer->scanLine(y);
    store(dest, buffer, x, length, nullptr, nullptr);
}

static void QT_FASTCALL destStore64RGBA64(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length)
{
    QRgba64 *dest = reinterpret_cast<QRgba64*>(rasterBuffer->scanLine(y)) + x;
    for (int i = 0; i < length; ++i) {
        dest[i] = buffer[i].unpremultiplied();
    }
}

static DestStoreProc64 destStoreProc64[QImage::NImageFormats] =
{
    nullptr,            // Format_Invalid
    nullptr,            // Format_Mono,
    nullptr,            // Format_MonoLSB
    nullptr,            // Format_Indexed8
    destStore64,        // Format_RGB32
    destStore64,        // Format_ARGB32,
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
    destStore64,        // Format_RGBA8888
    destStore64,        // Format_RGBA8888_Premultiplied
    destStore64,        // Format_BGR30
    destStore64,        // Format_A2BGR30_Premultiplied
    destStore64,        // Format_RGB30
    destStore64,        // Format_A2RGB30_Premultiplied
    destStore64,        // Format_Alpha8
    destStore64,        // Format_Grayscale8
    nullptr,            // Format_RGBX64
    destStore64RGBA64,  // Format_RGBA64
    nullptr,            // Format_RGBA64_Premultiplied
    destStore64,        // Format_Grayscale16
    destStore64,        // Format_BGR888
};
#endif

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
    return layout->fetchToARGB32PM(buffer, data->texture.scanLine(y), x, length, data->texture.colorTable, nullptr);
}

static const uint *QT_FASTCALL fetchUntransformedARGB32PM(uint *, const Operator *,
                                                          const QSpanData *data, int y, int x, int)
{
    const uchar *scanLine = data->texture.scanLine(y);
    return reinterpret_cast<const uint *>(scanLine) + x;
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

#if QT_CONFIG(raster_64bit)
static const QRgba64 *QT_FASTCALL fetchUntransformed64(QRgba64 *buffer, const Operator *,
                                                       const QSpanData *data, int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    return layout->fetchToRGBA64PM(buffer, data->texture.scanLine(y), x, length, data->texture.colorTable, nullptr);
}

static const QRgba64 *QT_FASTCALL fetchUntransformedRGBA64PM(QRgba64 *, const Operator *,
                                                             const QSpanData *data, int y, int x, int)
{
    const uchar *scanLine = data->texture.scanLine(y);
    return reinterpret_cast<const QRgba64 *>(scanLine) + x;
}
#endif

template<TextureBlendType blendType>
inline void fetchTransformed_pixelBounds(int max, int l1, int l2, int &v)
{
    Q_STATIC_ASSERT(blendType == BlendTransformed || blendType == BlendTransformedTiled);
    if (blendType == BlendTransformedTiled) {
        if (v < 0 || v >= max) {
            v %= max;
            if (v < 0) v += max;
        }
    } else {
        v = qBound(l1, v, l2);
    }
}

static inline bool canUseFastMatrixPath(const qreal cx, const qreal cy, const qsizetype length, const QSpanData *data)
{
    if (Q_UNLIKELY(!data->fast_matrix))
        return false;

    qreal fx = (data->m21 * cy + data->m11 * cx + data->dx) * fixed_scale;
    qreal fy = (data->m22 * cy + data->m12 * cx + data->dy) * fixed_scale;
    qreal minc = std::min(fx, fy);
    qreal maxc = std::max(fx, fy);
    fx += std::trunc(data->m11 * fixed_scale) * length;
    fy += std::trunc(data->m12 * fixed_scale) * length;
    minc = std::min(minc, std::min(fx, fy));
    maxc = std::max(maxc, std::max(fx, fy));

    return minc >= std::numeric_limits<int>::min() && maxc <= std::numeric_limits<int>::max();
}

template<TextureBlendType blendType, QPixelLayout::BPP bpp, typename T>
static void QT_FASTCALL fetchTransformed_fetcher(T *buffer, const QSpanData *data,
                                                 int y, int x, int length)
{
    Q_STATIC_ASSERT(blendType == BlendTransformed || blendType == BlendTransformedTiled);
    const QTextureData &image = data->texture;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    constexpr bool useFetch = (bpp < QPixelLayout::BPP32) && sizeof(T) == sizeof(uint);
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    if (!useFetch)
        Q_ASSERT(layout->bpp == bpp);
    // When templated 'fetch' should be inlined at compile time:
    const FetchPixelFunc fetch = (bpp == QPixelLayout::BPPNone) ? qFetchPixel[layout->bpp] : FetchPixelFunc(fetchPixel<bpp>);

    if (canUseFastMatrixPath(cx, cy, length, data)) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy
                      + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy
                      + data->m12 * cx + data->dy) * fixed_scale);

        if (fdy == 0) { // simple scale, no rotation or shear
            int py = (fy >> 16);
            fetchTransformed_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, py);
            const uchar *src = image.scanLine(py);

            int i = 0;
            if (blendType == BlendTransformed) {
                int fastLen = length;
                if (fdx > 0)
                    fastLen = qMin(fastLen, int((qint64(image.x2 - 1) * fixed_scale - fx) / fdx));
                else if (fdx < 0)
                    fastLen = qMin(fastLen, int((qint64(image.x1) * fixed_scale - fx) / fdx));

                for (; i < fastLen; ++i) {
                    int x1 = (fx >> 16);
                    int x2 = x1;
                    fetchTransformed_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1);
                    if (x1 == x2)
                        break;
                    if (useFetch)
                        buffer[i] = fetch(src, x1);
                    else
                        buffer[i] = reinterpret_cast<const T*>(src)[x1];
                    fx += fdx;
                }

                for (; i < fastLen; ++i) {
                    int px = (fx >> 16);
                    if (useFetch)
                        buffer[i] = fetch(src, px);
                    else
                        buffer[i] = reinterpret_cast<const T*>(src)[px];
                    fx += fdx;
                }
            }

            for (; i < length; ++i) {
                int px = (fx >> 16);
                fetchTransformed_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, px);
                if (useFetch)
                    buffer[i] = fetch(src, px);
                else
                    buffer[i] = reinterpret_cast<const T*>(src)[px];
                fx += fdx;
            }
        } else { // rotation or shear
            int i = 0;
            if (blendType == BlendTransformed) {
                int fastLen = length;
                if (fdx > 0)
                    fastLen = qMin(fastLen, int((qint64(image.x2 - 1) * fixed_scale - fx) / fdx));
                else if (fdx < 0)
                    fastLen = qMin(fastLen, int((qint64(image.x1) * fixed_scale - fx) / fdx));
                if (fdy > 0)
                    fastLen = qMin(fastLen, int((qint64(image.y2 - 1) * fixed_scale - fy) / fdy));
                else if (fdy < 0)
                    fastLen = qMin(fastLen, int((qint64(image.y1) * fixed_scale - fy) / fdy));

                for (; i < fastLen; ++i) {
                    int x1 = (fx >> 16);
                    int y1 = (fy >> 16);
                    int x2 = x1;
                    int y2 = y1;
                    fetchTransformed_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1);
                    fetchTransformed_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1);
                    if (x1 == x2 && y1 == y2)
                        break;
                    if (useFetch)
                        buffer[i] = fetch(image.scanLine(y1), x1);
                    else
                        buffer[i] = reinterpret_cast<const T*>(image.scanLine(y1))[x1];
                    fx += fdx;
                    fy += fdy;
                }

                for (; i < fastLen; ++i) {
                    int px = (fx >> 16);
                    int py = (fy >> 16);
                    if (useFetch)
                        buffer[i] = fetch(image.scanLine(py), px);
                    else
                        buffer[i] = reinterpret_cast<const T*>(image.scanLine(py))[px];
                    fx += fdx;
                    fy += fdy;
                }
            }

            for (; i < length; ++i) {
                int px = (fx >> 16);
                int py = (fy >> 16);
                fetchTransformed_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, px);
                fetchTransformed_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, py);
                if (useFetch)
                    buffer[i] = fetch(image.scanLine(py), px);
                else
                    buffer[i] = reinterpret_cast<const T*>(image.scanLine(py))[px];
                fx += fdx;
                fy += fdy;
            }
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        T *const end = buffer + length;
        T *b = buffer;
        while (b < end) {
            const qreal iw = fw == 0 ? 1 : 1 / fw;
            const qreal tx = fx * iw;
            const qreal ty = fy * iw;
            int px = qFloor(tx);
            int py = qFloor(ty);

            fetchTransformed_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, py);
            fetchTransformed_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, px);
            if (useFetch)
                *b = fetch(image.scanLine(py), px);
            else
                *b = reinterpret_cast<const T*>(image.scanLine(py))[px];

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
}

template<TextureBlendType blendType, QPixelLayout::BPP bpp>
static const uint *QT_FASTCALL fetchTransformed(uint *buffer, const Operator *, const QSpanData *data,
                                                int y, int x, int length)
{
    Q_STATIC_ASSERT(blendType == BlendTransformed || blendType == BlendTransformedTiled);
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    fetchTransformed_fetcher<blendType, bpp, uint>(buffer, data, y, x, length);
    layout->convertToARGB32PM(buffer, length, data->texture.colorTable);
    return buffer;
}

#if QT_CONFIG(raster_64bit)
template<TextureBlendType blendType>  /* either BlendTransformed or BlendTransformedTiled */
static const QRgba64 *QT_FASTCALL fetchTransformed64(QRgba64 *buffer, const Operator *, const QSpanData *data,
                                                     int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    if (layout->bpp != QPixelLayout::BPP64) {
        uint buffer32[BufferSize];
        Q_ASSERT(length <= BufferSize);
        if (layout->bpp == QPixelLayout::BPP32)
            fetchTransformed_fetcher<blendType, QPixelLayout::BPP32, uint>(buffer32, data, y, x, length);
        else
            fetchTransformed_fetcher<blendType, QPixelLayout::BPPNone, uint>(buffer32, data, y, x, length);
        return layout->convertToRGBA64PM(buffer, buffer32, length, data->texture.colorTable, nullptr);
    }

    fetchTransformed_fetcher<blendType, QPixelLayout::BPP64, QRgba64>(buffer, data, y, x, length);
    if (data->texture.format == QImage::Format_RGBA64)
        convertRGBA64ToRGBA64PM(buffer, length);
    return buffer;
}
#endif

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
    SimpleScaleTransform,
    UpscaleTransform,
    DownscaleTransform,
    RotateTransform,
    FastRotateTransform,
    NFastTransformTypes
};

// Completes the partial interpolation stored in IntermediateBuffer.
// by performing the x-axis interpolation and joining the RB and AG buffers.
static void QT_FASTCALL intermediate_adder(uint *b, uint *end, const IntermediateBuffer &intermediate, int offset, int &fx, int fdx)
{
#if defined(QT_COMPILER_SUPPORTS_AVX2)
    extern void QT_FASTCALL intermediate_adder_avx2(uint *b, uint *end, const IntermediateBuffer &intermediate, int offset, int &fx, int fdx);
    if (qCpuHasFeature(ArchHaswell))
        return intermediate_adder_avx2(b, end, intermediate, offset, fx, fdx);
#endif

    // Switch to intermediate buffer coordinates
    fx -= offset * fixed_scale;

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
    fx += offset * fixed_scale;
}

typedef void (QT_FASTCALL *BilinearFastTransformHelper)(uint *b, uint *end, const QTextureData &image, int &fx, int &fy, int fdx, int fdy);

template<TextureBlendType blendType>
static void QT_FASTCALL fetchTransformedBilinearARGB32PM_simple_scale_helper(uint *b, uint *end, const QTextureData &image,
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

    IntermediateBuffer intermediate;
    // count is the size used in the intermediate.buffer.
    int count = (qint64(length) * qAbs(fdx) + fixed_scale - 1) / fixed_scale + 2;
    // length is supposed to be <= BufferSize either because data->m11 < 1 or
    // data->m11 < 2, and any larger buffers split
    Q_ASSERT(count <= BufferSize + 2);
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
                intermediate.buffer_rb[f] = rb;
                intermediate.buffer_ag[f] = ag;
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
            _mm_storeu_si128((__m128i*)(&intermediate.buffer_ag[f]), rAG);
            __m128i rRB =_mm_add_epi16(topRB, bottomRB);
            rRB = _mm_srli_epi16(rRB, 8);
            _mm_storeu_si128((__m128i*)(&intermediate.buffer_rb[f]), rRB);
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
            vst1q_s16((int16_t*)(&intermediate.buffer_ag[f]), rAG);
            int16x8_t rRB = vaddq_s16(topRB, bottomRB);
            rRB = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(rRB), 8));
            vst1q_s16((int16_t*)(&intermediate.buffer_rb[f]), rRB);
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

        intermediate.buffer_rb[f] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
        intermediate.buffer_ag[f] = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
        x++;
    }

    // Now interpolate the values from the intermediate.buffer to get the final result.
    intermediate_adder(b, end, intermediate, offset, fx, fdx);
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
        fetchTransformedBilinearARGB32PM_simple_scale_helper<BlendTransformedBilinear>,
        fetchTransformedBilinearARGB32PM_upscale_helper<BlendTransformedBilinear>,
        fetchTransformedBilinearARGB32PM_downscale_helper<BlendTransformedBilinear>,
        fetchTransformedBilinearARGB32PM_rotate_helper<BlendTransformedBilinear>,
        fetchTransformedBilinearARGB32PM_fast_rotate_helper<BlendTransformedBilinear>
    },
    {
        fetchTransformedBilinearARGB32PM_simple_scale_helper<BlendTransformedBilinearTiled>,
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
    if (canUseFastMatrixPath(cx, cy, length, data)) {
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
                bilinearFastTransformHelperARGB32PM[tiled][SimpleScaleTransform](b, end, data->texture, fx, fy, fdx, fdy);
            } else if (qAbs(fdx) <= 2 * fixed_scale) {
                // simple scale down on X, less than 2x
                const int mid = (length * 2 < BufferSize) ? length : ((length + 1) / 2);
                bilinearFastTransformHelperARGB32PM[tiled][SimpleScaleTransform](buffer, buffer + mid, data->texture, fx, fy, fdx, fdy);
                if (mid != length)
                    bilinearFastTransformHelperARGB32PM[tiled][SimpleScaleTransform](buffer + mid, buffer + length, data->texture, fx, fy, fdx, fdy);
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

template<TextureBlendType blendType>
static void QT_FASTCALL fetchTransformedBilinear_simple_scale_helper(uint *b, uint *end, const QTextureData &image,
                                                                     int &fx, int &fy, int fdx, int /*fdy*/)
{
    const QPixelLayout *layout = &qPixelLayouts[image.format];
    const QVector<QRgb> *clut = image.colorTable;
    const FetchAndConvertPixelsFunc fetch = layout->fetchToARGB32PM;

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

    IntermediateBuffer intermediate;
    uint *buf1 = intermediate.buffer_rb;
    uint *buf2 = intermediate.buffer_ag;
    const uint *ptr1;
    const uint *ptr2;

    int count = (qint64(length) * qAbs(fdx) + fixed_scale - 1) / fixed_scale + 2;
    Q_ASSERT(count <= BufferSize + 2);

    if (blendType == BlendTransformedBilinearTiled) {
        x %= image.width;
        if (x < 0)
            x += image.width;
        int len1 = qMin(count, image.width - x);
        int len2 = qMin(x, count - len1);

        ptr1 = fetch(buf1, s1, x, len1, clut, nullptr);
        ptr2 = fetch(buf2, s2, x, len1, clut, nullptr);
        for (int i = 0; i < len1; ++i) {
            uint t = ptr1[i];
            uint b = ptr2[i];
            buf1[i] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
            buf2[i] = ((((t >> 8) & 0xff00ff) * idisty + ((b >> 8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
        }

        if (len2) {
            ptr1 = fetch(buf1 + len1, s1, 0, len2, clut, nullptr);
            ptr2 = fetch(buf2 + len1, s2, 0, len2, clut, nullptr);
            for (int i = 0; i < len2; ++i) {
                uint t = ptr1[i];
                uint b = ptr2[i];
                buf1[i + len1] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
                buf2[i + len1] = ((((t >> 8) & 0xff00ff) * idisty + ((b >> 8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
            }
        }
        // Generate the rest by repeatedly repeating the previous set of pixels
        for (int i = image.width; i < count; ++i) {
            buf1[i] = buf1[i - image.width];
            buf2[i] = buf2[i - image.width];
        }
    } else {
        int start = qMax(x, image.x1);
        int end = qMin(x + count, image.x2);
        int len = qMax(1, end - start);
        int leading = start - x;

        ptr1 = fetch(buf1 + leading, s1, start, len, clut, nullptr);
        ptr2 = fetch(buf2 + leading, s2, start, len, clut, nullptr);

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

    // Now interpolate the values from the intermediate.buffer to get the final result.
    intermediate_adder(b, end, intermediate, offset, fx, fdx);
}


template<TextureBlendType blendType, QPixelLayout::BPP bpp, typename T>
static void QT_FASTCALL fetchTransformedBilinear_fetcher(T *buf1, T *buf2, const int len, const QTextureData &image,
                                                         int fx, int fy, const int fdx, const int fdy)
{
    const QPixelLayout &layout = qPixelLayouts[image.format];
    constexpr bool useFetch = (bpp < QPixelLayout::BPP32);
    if (useFetch)
        Q_ASSERT(sizeof(T) == sizeof(uint));
    else
        Q_ASSERT(layout.bpp == bpp);
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
                if (useFetch) {
                    buf1[i * 2 + 0] = buf1[i * 2 + 1] = fetch1(s1, x1);
                    buf2[i * 2 + 0] = buf2[i * 2 + 1] = fetch1(s2, x1);
                } else {
                    buf1[i * 2 + 0] = buf1[i * 2 + 1] = reinterpret_cast<const T *>(s1)[x1];
                    buf2[i * 2 + 0] = buf2[i * 2 + 1] = reinterpret_cast<const T *>(s2)[x1];
                }
                fx += fdx;
            }
            int fastLen = len;
            if (fdx > 0)
                fastLen = qMin(fastLen, int((qint64(image.x2 - 1) * fixed_scale - fx) / fdx));
            else if (fdx < 0)
                fastLen = qMin(fastLen, int((qint64(image.x1) * fixed_scale - fx) / fdx));

            for (; i < fastLen; ++i) {
                int x = (fx >> 16);
                if (useFetch) {
                    buf1[i * 2 + 0] = fetch1(s1, x);
                    buf1[i * 2 + 1] = fetch1(s1, x + 1);
                    buf2[i * 2 + 0] = fetch1(s2, x);
                    buf2[i * 2 + 1] = fetch1(s2, x + 1);
                } else {
                    buf1[i * 2 + 0] = reinterpret_cast<const T *>(s1)[x];
                    buf1[i * 2 + 1] = reinterpret_cast<const T *>(s1)[x + 1];
                    buf2[i * 2 + 0] = reinterpret_cast<const T *>(s2)[x];
                    buf2[i * 2 + 1] = reinterpret_cast<const T *>(s2)[x + 1];
                }
                fx += fdx;
            }
        }

        for (; i < len; ++i) {
            int x1 = (fx >> 16);
            int x2;
            fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
            if (useFetch) {
                buf1[i * 2 + 0] = fetch1(s1, x1);
                buf1[i * 2 + 1] = fetch1(s1, x2);
                buf2[i * 2 + 0] = fetch1(s2, x1);
                buf2[i * 2 + 1] = fetch1(s2, x2);
            } else {
                buf1[i * 2 + 0] = reinterpret_cast<const T *>(s1)[x1];
                buf1[i * 2 + 1] = reinterpret_cast<const T *>(s1)[x2];
                buf2[i * 2 + 0] = reinterpret_cast<const T *>(s2)[x1];
                buf2[i * 2 + 1] = reinterpret_cast<const T *>(s2)[x2];
            }
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
                if (useFetch) {
                    buf1[i * 2 + 0] = fetch1(s1, x1);
                    buf1[i * 2 + 1] = fetch1(s1, x2);
                    buf2[i * 2 + 0] = fetch1(s2, x1);
                    buf2[i * 2 + 1] = fetch1(s2, x2);
                } else {
                    buf1[i * 2 + 0] = reinterpret_cast<const T *>(s1)[x1];
                    buf1[i * 2 + 1] = reinterpret_cast<const T *>(s1)[x2];
                    buf2[i * 2 + 0] = reinterpret_cast<const T *>(s2)[x1];
                    buf2[i * 2 + 1] = reinterpret_cast<const T *>(s2)[x2];
                }
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
                if (useFetch) {
                    buf1[i * 2 + 0] = fetch1(s1, x);
                    buf1[i * 2 + 1] = fetch1(s1, x + 1);
                    buf2[i * 2 + 0] = fetch1(s2, x);
                    buf2[i * 2 + 1] = fetch1(s2, x + 1);
                } else {
                    buf1[i * 2 + 0] = reinterpret_cast<const T *>(s1)[x];
                    buf1[i * 2 + 1] = reinterpret_cast<const T *>(s1)[x + 1];
                    buf2[i * 2 + 0] = reinterpret_cast<const T *>(s2)[x];
                    buf2[i * 2 + 1] = reinterpret_cast<const T *>(s2)[x + 1];
                }
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
            if (useFetch) {
                buf1[i * 2 + 0] = fetch1(s1, x1);
                buf1[i * 2 + 1] = fetch1(s1, x2);
                buf2[i * 2 + 0] = fetch1(s2, x1);
                buf2[i * 2 + 1] = fetch1(s2, x2);
            } else {
                buf1[i * 2 + 0] = reinterpret_cast<const T *>(s1)[x1];
                buf1[i * 2 + 1] = reinterpret_cast<const T *>(s1)[x2];
                buf2[i * 2 + 0] = reinterpret_cast<const T *>(s2)[x1];
                buf2[i * 2 + 1] = reinterpret_cast<const T *>(s2)[x2];
            }
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

    if (canUseFastMatrixPath(cx, cy, length, data)) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;

        if (fdy == 0) { // simple scale, no rotation or shear
            if (qAbs(fdx) <= fixed_scale) { // scale up on X
                fetchTransformedBilinear_simple_scale_helper<blendType>(buffer, buffer + length, data->texture, fx, fy, fdx, fdy);
            } else if (qAbs(fdx) <= 2 * fixed_scale) { // scale down on X less than 2x
                const int mid = (length * 2 < BufferSize) ? length : ((length + 1) / 2);
                fetchTransformedBilinear_simple_scale_helper<blendType>(buffer, buffer + mid, data->texture, fx, fy, fdx, fdy);
                if (mid != length)
                    fetchTransformedBilinear_simple_scale_helper<blendType>(buffer + mid, buffer + length, data->texture, fx, fy, fdx, fdy);
            } else {
                const auto fetcher = fetchTransformedBilinear_fetcher<blendType,bpp,uint>;

                uint buf1[BufferSize];
                uint buf2[BufferSize];
                uint *b = buffer;
                while (length) {
                    int len = qMin(length, BufferSize / 2);
                    fetcher(buf1, buf2, len, data->texture, fx, fy, fdx, 0);
                    layout->convertToARGB32PM(buf1, len * 2, clut);
                    layout->convertToARGB32PM(buf2, len * 2, clut);

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
            const auto fetcher = fetchTransformedBilinear_fetcher<blendType,bpp,uint>;

            uint buf1[BufferSize];
            uint buf2[BufferSize];
            uint *b = buffer;
            while (length) {
                int len = qMin(length, BufferSize / 2);
                fetcher(buf1, buf2, len, data->texture, fx, fy, fdx, fdy);
                layout->convertToARGB32PM(buf1, len * 2, clut);
                layout->convertToARGB32PM(buf2, len * 2, clut);

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

        uint buf1[BufferSize];
        uint buf2[BufferSize];
        uint *b = buffer;

        int distxs[BufferSize / 2];
        int distys[BufferSize / 2];

        while (length) {
            int len = qMin(length, BufferSize / 2);
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

            layout->convertToARGB32PM(buf1, len * 2, clut);
            layout->convertToARGB32PM(buf2, len * 2, clut);

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

#if QT_CONFIG(raster_64bit)
template<TextureBlendType blendType>
static const QRgba64 *QT_FASTCALL fetchTransformedBilinear64_uint32(QRgba64 *buffer, const QSpanData *data,
                                                                    int y, int x, int length)
{
    const QTextureData &texture = data->texture;
    const QPixelLayout *layout = &qPixelLayouts[texture.format];
    const QVector<QRgb> *clut = data->texture.colorTable;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    uint sbuf1[BufferSize];
    uint sbuf2[BufferSize];
    alignas(8) QRgba64 buf1[BufferSize];
    alignas(8) QRgba64 buf2[BufferSize];
    QRgba64 *end = buffer + length;
    QRgba64 *b = buffer;

    if (canUseFastMatrixPath(cx, cy, length, data)) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;

        const auto fetcher =
                (layout->bpp == QPixelLayout::BPP32)
                        ? fetchTransformedBilinear_fetcher<blendType, QPixelLayout::BPP32, uint>
                        : fetchTransformedBilinear_fetcher<blendType, QPixelLayout::BPPNone, uint>;

        if (fdy == 0) { //simple scale, no rotation
            while (length) {
                int len = qMin(length, BufferSize / 2);
                int disty = (fy & 0x0000ffff);
#if defined(__SSE2__)
                const __m128i vdy = _mm_set1_epi16(disty);
                const __m128i vidy = _mm_set1_epi16(0x10000 - disty);
#endif
                fetcher(sbuf1, sbuf2, len, data->texture, fx, fy, fdx, fdy);

                layout->convertToRGBA64PM(buf1, sbuf1, len * 2, clut, nullptr);
                if (disty)
                    layout->convertToRGBA64PM(buf2, sbuf2, len * 2, clut, nullptr);

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
                    b[i] = interpolate_4_pixels_rgb64(buf1 + i*2, buf2 + i*2, distx, disty);
#endif
                    fx += fdx;
                }
                length -= len;
                b += len;
            }
        } else { // rotation or shear
            while (b < end) {
                int len = qMin(length, BufferSize / 2);

                fetcher(sbuf1, sbuf2, len, data->texture, fx, fy, fdx, fdy);

                layout->convertToRGBA64PM(buf1, sbuf1, len * 2, clut, nullptr);
                layout->convertToRGBA64PM(buf2, sbuf2, len * 2, clut, nullptr);

                for (int i = 0; i < len; ++i) {
                    int distx = (fx & 0x0000ffff);
                    int disty = (fy & 0x0000ffff);
                    b[i] = interpolate_4_pixels_rgb64(buf1 + i*2, buf2 + i*2, distx, disty);
                    fx += fdx;
                    fy += fdy;
                }

                length -= len;
                b += len;
            }
        }
    } else { // !(data->fast_matrix)
        const QTextureData &image = data->texture;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        FetchPixelFunc fetch = qFetchPixel[layout->bpp];

        int distxs[BufferSize / 2];
        int distys[BufferSize / 2];

        while (b < end) {
            int len = qMin(length, BufferSize / 2);
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

                const uchar *s1 = texture.scanLine(y1);
                const uchar *s2 = texture.scanLine(y2);

                sbuf1[i * 2 + 0] = fetch(s1, x1);
                sbuf1[i * 2 + 1] = fetch(s1, x2);
                sbuf2[i * 2 + 0] = fetch(s2, x1);
                sbuf2[i * 2 + 1] = fetch(s2, x2);

                fx += fdx;
                fy += fdy;
                fw += fdw;
                //force increment to avoid /0
                if (!fw)
                    fw += fdw;
            }

            layout->convertToRGBA64PM(buf1, sbuf1, len * 2, clut, nullptr);
            layout->convertToRGBA64PM(buf2, sbuf2, len * 2, clut, nullptr);

            for (int i = 0; i < len; ++i) {
                int distx = distxs[i];
                int disty = distys[i];
                b[i] = interpolate_4_pixels_rgb64(buf1 + i*2, buf2 + i*2, distx, disty);
            }

            length -= len;
            b += len;
        }
    }
    return buffer;
}

template<TextureBlendType blendType>
static const QRgba64 *QT_FASTCALL fetchTransformedBilinear64_uint64(QRgba64 *buffer, const QSpanData *data,
                                                                    int y, int x, int length)
{
    const QTextureData &texture = data->texture;
    Q_ASSERT(qPixelLayouts[texture.format].bpp == QPixelLayout::BPP64);
    const auto convert = (data->texture.format == QImage::Format_RGBA64) ? convertRGBA64ToRGBA64PM : convertRGBA64PMToRGBA64PM;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    alignas(8) QRgba64 buf1[BufferSize];
    alignas(8) QRgba64 buf2[BufferSize];
    QRgba64 *end = buffer + length;
    QRgba64 *b = buffer;

    if (canUseFastMatrixPath(cx, cy, length, data)) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;
        const auto fetcher = fetchTransformedBilinear_fetcher<blendType, QPixelLayout::BPP64, QRgba64>;

        if (fdy == 0) { //simple scale, no rotation
            while (length) {
                int len = qMin(length, BufferSize / 2);
                int disty = (fy & 0x0000ffff);
#if defined(__SSE2__)
                const __m128i vdy = _mm_set1_epi16(disty);
                const __m128i vidy = _mm_set1_epi16(0x10000 - disty);
#endif
                fetcher(buf1, buf2, len, data->texture, fx, fy, fdx, fdy);

                convert(buf1, len * 2);
                if (disty)
                    convert(buf2, len * 2);

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
                    b[i] = interpolate_4_pixels_rgb64(buf1 + i*2, buf2 + i*2, distx, disty);
#endif
                    fx += fdx;
                }
                length -= len;
                b += len;
            }
        } else { // rotation or shear
            while (b < end) {
                int len = qMin(length, BufferSize / 2);

                fetcher(buf1, buf2, len, data->texture, fx, fy, fdx, fdy);

                convert(buf1, len * 2);
                convert(buf2, len * 2);

                for (int i = 0; i < len; ++i) {
                    int distx = (fx & 0x0000ffff);
                    int disty = (fy & 0x0000ffff);
                    b[i] = interpolate_4_pixels_rgb64(buf1 + i*2, buf2 + i*2, distx, disty);
                    fx += fdx;
                    fy += fdy;
                }

                length -= len;
                b += len;
            }
        }
    } else { // !(data->fast_matrix)
        const QTextureData &image = data->texture;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        int distxs[BufferSize / 2];
        int distys[BufferSize / 2];

        while (b < end) {
            int len = qMin(length, BufferSize / 2);
            for (int i = 0; i < len; ++i) {
                const qreal iw = fw == 0 ? 1 : 1 / fw;
                const qreal px = fx * iw - qreal(0.5);
                const qreal py = fy * iw - qreal(0.5);

                int x1 = int(px) - (px < 0);
                int x2;
                int y1 = int(py) - (py < 0);
                int y2;

                distxs[i] = int((px - x1) * (1<<16));
                distys[i] = int((py - y1) * (1<<16));

                fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
                fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);

                const uchar *s1 = texture.scanLine(y1);
                const uchar *s2 = texture.scanLine(y2);

                buf1[i * 2 + 0] = reinterpret_cast<const QRgba64 *>(s1)[x1];
                buf1[i * 2 + 1] = reinterpret_cast<const QRgba64 *>(s1)[x2];
                buf2[i * 2 + 0] = reinterpret_cast<const QRgba64 *>(s2)[x1];
                buf2[i * 2 + 1] = reinterpret_cast<const QRgba64 *>(s2)[x2];

                fx += fdx;
                fy += fdy;
                fw += fdw;
                //force increment to avoid /0
                if (!fw)
                    fw += fdw;
            }

            convert(buf1, len * 2);
            convert(buf2, len * 2);

            for (int i = 0; i < len; ++i) {
                int distx = distxs[i];
                int disty = distys[i];
                b[i] = interpolate_4_pixels_rgb64(buf1 + i*2, buf2 + i*2, distx, disty);
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
    if (qPixelLayouts[data->texture.format].bpp == QPixelLayout::BPP64)
        return fetchTransformedBilinear64_uint64<blendType>(buffer, data, y, x, length);
    return fetchTransformedBilinear64_uint32<blendType>(buffer, data, y, x, length);
}
#endif

// FetchUntransformed can have more specialized methods added depending on SIMD features.
static SourceFetchProc sourceFetchUntransformed[QImage::NImageFormats] = {
    nullptr,                    // Invalid
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
    fetchUntransformed,         // RGBX64
    fetchUntransformed,         // RGBA64
    fetchUntransformed,         // RGBA64_Premultiplied
    fetchUntransformed,         // Grayscale16
    fetchUntransformed,         // BGR888
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

static SourceFetchProc sourceFetchAny16[NBlendTypes] = {
    fetchUntransformed,                                                             // Untransformed
    fetchUntransformed,                                                             // Tiled
    fetchTransformed<BlendTransformed, QPixelLayout::BPP16>,                        // Transformed
    fetchTransformed<BlendTransformedTiled, QPixelLayout::BPP16>,                   // TransformedTiled
    fetchTransformedBilinear<BlendTransformedBilinear, QPixelLayout::BPP16>,        // TransformedBilinear
    fetchTransformedBilinear<BlendTransformedBilinearTiled, QPixelLayout::BPP16>    // TransformedBilinearTiled
};

static SourceFetchProc sourceFetchAny32[NBlendTypes] = {
    fetchUntransformed,                                                             // Untransformed
    fetchUntransformed,                                                             // Tiled
    fetchTransformed<BlendTransformed, QPixelLayout::BPP32>,                        // Transformed
    fetchTransformed<BlendTransformedTiled, QPixelLayout::BPP32>,                   // TransformedTiled
    fetchTransformedBilinear<BlendTransformedBilinear, QPixelLayout::BPP32>,        // TransformedBilinear
    fetchTransformedBilinear<BlendTransformedBilinearTiled, QPixelLayout::BPP32>    // TransformedBilinearTiled
};

static inline SourceFetchProc getSourceFetch(TextureBlendType blendType, QImage::Format format)
{
    if (format == QImage::Format_RGB32 || format == QImage::Format_ARGB32_Premultiplied)
        return sourceFetchARGB32PM[blendType];
    if (blendType == BlendUntransformed || blendType == BlendTiled)
        return sourceFetchUntransformed[format];
    if (qPixelLayouts[format].bpp == QPixelLayout::BPP16)
        return sourceFetchAny16[blendType];
    if (qPixelLayouts[format].bpp == QPixelLayout::BPP32)
        return sourceFetchAny32[blendType];
    return sourceFetchGeneric[blendType];
}

#if QT_CONFIG(raster_64bit)
static const SourceFetchProc64 sourceFetchGeneric64[NBlendTypes] = {
    fetchUntransformed64,                                     // Untransformed
    fetchUntransformed64,                                     // Tiled
    fetchTransformed64<BlendTransformed>,                     // Transformed
    fetchTransformed64<BlendTransformedTiled>,                // TransformedTiled
    fetchTransformedBilinear64<BlendTransformedBilinear>,     // Bilinear
    fetchTransformedBilinear64<BlendTransformedBilinearTiled> // BilinearTiled
};

static const SourceFetchProc64 sourceFetchRGBA64PM[NBlendTypes] = {
    fetchUntransformedRGBA64PM,                               // Untransformed
    fetchUntransformedRGBA64PM,                               // Tiled
    fetchTransformed64<BlendTransformed>,                     // Transformed
    fetchTransformed64<BlendTransformedTiled>,                // TransformedTiled
    fetchTransformedBilinear64<BlendTransformedBilinear>,     // Bilinear
    fetchTransformedBilinear64<BlendTransformedBilinearTiled> // BilinearTiled
};

static inline SourceFetchProc64 getSourceFetch64(TextureBlendType blendType, QImage::Format format)
{
    if (format == QImage::Format_RGBX64 || format == QImage::Format_RGBA64_Premultiplied)
        return sourceFetchRGBA64PM[blendType];
    return sourceFetchGeneric64[blendType];
}
#endif


#define FIXPT_BITS 8
#define FIXPT_SIZE (1<<FIXPT_BITS)

static uint qt_gradient_pixel_fixed(const QGradientData *data, int fixed_pos)
{
    int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;
    return data->colorTable32[qt_gradient_clamp(data, ipos)];
}

#if QT_CONFIG(raster_64bit)
static const QRgba64& qt_gradient_pixel64_fixed(const QGradientData *data, int fixed_pos)
{
    int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;
    return data->colorTable64[qt_gradient_clamp(data, ipos)];
}
#endif

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

#if QT_CONFIG(raster_64bit)
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
#endif

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

#if QT_CONFIG(raster_64bit)
static const QRgba64 * QT_FASTCALL qt_fetch_linear_gradient_rgb64(QRgba64 *buffer, const Operator *op, const QSpanData *data,
                                                                 int y, int x, int length)
{
    return qt_fetch_linear_gradient_template<GradientBase64, QRgba64>(buffer, op, data, y, x, length);
}
#endif

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

#if QT_CONFIG(raster_64bit)
const QRgba64 * QT_FASTCALL qt_fetch_radial_gradient_rgb64(QRgba64 *buffer, const Operator *op, const QSpanData *data,
                                                        int y, int x, int length)
{
    return qt_fetch_radial_gradient_template<RadialFetchPlain<GradientBase64>, QRgba64>(buffer, op, data, y, x, length);
}
#endif

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

#if QT_CONFIG(raster_64bit)
static const QRgba64 * QT_FASTCALL qt_fetch_conical_gradient_rgb64(QRgba64 *buffer, const Operator *, const QSpanData *data,
                                                                   int y, int x, int length)
{
    return qt_fetch_conical_gradient_template<GradientBase64, QRgba64>(buffer, data, y, x, length);
}
#endif

extern CompositionFunctionSolid qt_functionForModeSolid_C[];
extern CompositionFunctionSolid64 qt_functionForModeSolid64_C[];

static const CompositionFunctionSolid *functionForModeSolid = qt_functionForModeSolid_C;
#if QT_CONFIG(raster_64bit)
static const CompositionFunctionSolid64 *functionForModeSolid64 = qt_functionForModeSolid64_C;
#endif

extern CompositionFunction qt_functionForMode_C[];
extern CompositionFunction64 qt_functionForMode64_C[];

static const CompositionFunction *functionForMode = qt_functionForMode_C;
#if QT_CONFIG(raster_64bit)
static const CompositionFunction64 *functionForMode64 = qt_functionForMode64_C;
#endif

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
        solidSource = data->solidColor.isOpaque();
        op.srcFetch = nullptr;
#if QT_CONFIG(raster_64bit)
        op.srcFetch64 = nullptr;
#endif
        break;
    case QSpanData::LinearGradient:
        solidSource = !data->gradient.alphaColor;
        getLinearGradientValues(&op.linear, data);
        op.srcFetch = qt_fetch_linear_gradient;
#if QT_CONFIG(raster_64bit)
        op.srcFetch64 = qt_fetch_linear_gradient_rgb64;
#endif
        break;
    case QSpanData::RadialGradient:
        solidSource = !data->gradient.alphaColor;
        getRadialGradientValues(&op.radial, data);
        op.srcFetch = qt_fetch_radial_gradient;
#if QT_CONFIG(raster_64bit)
        op.srcFetch64 = qt_fetch_radial_gradient_rgb64;
#endif
        break;
    case QSpanData::ConicalGradient:
        solidSource = !data->gradient.alphaColor;
        op.srcFetch = qt_fetch_conical_gradient;
#if QT_CONFIG(raster_64bit)
        op.srcFetch64 = qt_fetch_conical_gradient_rgb64;
#endif
        break;
    case QSpanData::Texture:
        solidSource = !data->texture.hasAlpha;
        op.srcFetch = getSourceFetch(getBlendType(data), data->texture.format);
#if QT_CONFIG(raster_64bit)
        op.srcFetch64 = getSourceFetch64(getBlendType(data), data->texture.format);;
#endif
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
#if !QT_CONFIG(raster_64bit)
    op.srcFetch64 = 0;
#endif

    op.mode = data->rasterBuffer->compositionMode;
    if (op.mode == QPainter::CompositionMode_SourceOver && solidSource)
        op.mode = QPainter::CompositionMode_Source;

    op.destFetch = destFetchProc[data->rasterBuffer->format];
#if QT_CONFIG(raster_64bit)
    op.destFetch64 = destFetchProc64[data->rasterBuffer->format];
#else
    op.destFetch64 = 0;
#endif
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
        if (!alphaSpans && spanCount > 0) {
            // If all spans are opaque we do not need to fetch dest.
            // But don't clear passthrough destFetch as they are just as fast and save destStore.
            if (op.destFetch != destFetchARGB32P)
                op.destFetch = destFetchUndefined;
#if QT_CONFIG(raster_64bit)
            if (op.destFetch64 != destFetchRGB64)
                op.destFetch64 = destFetch64Undefined;
#endif
        }
    }

    op.destStore = destStoreProc[data->rasterBuffer->format];
    op.funcSolid = functionForModeSolid[op.mode];
    op.func = functionForMode[op.mode];
#if QT_CONFIG(raster_64bit)
    op.destStore64 = destStoreProc64[data->rasterBuffer->format];
    op.funcSolid64 = functionForModeSolid64[op.mode];
    op.func64 = functionForMode64[op.mode];
#else
    op.destStore64 = 0;
    op.funcSolid64 = 0;
    op.func64 = 0;
#endif

    return op;
}

static void spanfill_from_first(QRasterBuffer *rasterBuffer, QPixelLayout::BPP bpp, int x, int y, int length)
{
    switch (bpp) {
    case QPixelLayout::BPP64: {
        quint64 *dest = reinterpret_cast<quint64 *>(rasterBuffer->scanLine(y)) + x;
        qt_memfill_template(dest + 1, dest[0], length - 1);
        break;
    }
    case QPixelLayout::BPP32: {
        quint32 *dest = reinterpret_cast<quint32 *>(rasterBuffer->scanLine(y)) + x;
        qt_memfill_template(dest + 1, dest[0], length - 1);
        break;
    }
    case QPixelLayout::BPP24: {
        quint24 *dest = reinterpret_cast<quint24 *>(rasterBuffer->scanLine(y)) + x;
        qt_memfill_template(dest + 1, dest[0], length - 1);
        break;
    }
    case QPixelLayout::BPP16: {
        quint16 *dest = reinterpret_cast<quint16 *>(rasterBuffer->scanLine(y)) + x;
        qt_memfill_template(dest + 1, dest[0], length - 1);
        break;
    }
    case QPixelLayout::BPP8: {
        uchar *dest = rasterBuffer->scanLine(y) + x;
        memset(dest + 1, dest[0], length - 1);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}


// -------------------- blend methods ---------------------

static void blend_color_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    uint buffer[BufferSize];
    Operator op = getOperator(data, nullptr, 0);
    const uint color = data->solidColor.toArgb32();
    const bool solidFill = op.mode == QPainter::CompositionMode_Source;
    const QPixelLayout::BPP bpp = qPixelLayouts[data->rasterBuffer->format].bpp;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        if (solidFill && bpp >= QPixelLayout::BPP8 && spans->coverage == 255 && length) {
            // If dest doesn't matter we don't need to bother with blending or converting all the identical pixels
            op.destStore(data->rasterBuffer, x, spans->y, &color, 1);
            spanfill_from_first(data->rasterBuffer, bpp, x, spans->y, length);
            length = 0;
        }

        while (length) {
            int l = qMin(BufferSize, length);
            uint *dest = op.destFetch(buffer, data->rasterBuffer, x, spans->y, l);
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

    const Operator op = getOperator(data, nullptr, 0);
    const uint color = data->solidColor.toArgb32();

    if (op.mode == QPainter::CompositionMode_Source) {
        // inline for performance
        while (count--) {
            uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                qt_memfill(target, color, spans->len);
#ifdef __SSE2__
            } else if (spans->len > 16) {
                op.funcSolid(target, spans->len, color, spans->coverage);
#endif
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
#if QT_CONFIG(raster_64bit)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    Operator op = getOperator(data, nullptr, 0);
    if (!op.funcSolid64) {
        qCDebug(lcQtGuiDrawHelper, "blend_color_generic_rgb64: unsupported 64bit blend attempted, falling back to 32-bit");
        return blend_color_generic(count, spans, userData);
    }

    alignas(8) QRgba64 buffer[BufferSize];
    const QRgba64 color = data->solidColor;
    const bool solidFill = op.mode == QPainter::CompositionMode_Source;
    const QPixelLayout::BPP bpp = qPixelLayouts[data->rasterBuffer->format].bpp;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        if (solidFill && bpp >= QPixelLayout::BPP8 && spans->coverage == 255 && length && op.destStore64) {
            // If dest doesn't matter we don't need to bother with blending or converting all the identical pixels
            op.destStore64(data->rasterBuffer, x, spans->y, &color, 1);
            spanfill_from_first(data->rasterBuffer, bpp, x, spans->y, length);
            length = 0;
        }

        while (length) {
            int l = qMin(BufferSize, length);
            QRgba64 *dest = op.destFetch64(buffer, data->rasterBuffer, x, spans->y, l);
            op.funcSolid64(dest, l, color, spans->coverage);
            if (op.destStore64)
                op.destStore64(data->rasterBuffer, x, spans->y, dest, l);
            length -= l;
            x += l;
        }
        ++spans;
    }
#else
    blend_color_generic(count, spans, userData);
#endif
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
    if (mode == QPainter::CompositionMode_SourceOver && data->solidColor.isOpaque())
        mode = QPainter::CompositionMode_Source;

    if (mode == QPainter::CompositionMode_Source) {
        // inline for performance
        ushort c = data->solidColor.toRgb16();
        for (; count--; spans++) {
            if (!spans->len)
                continue;
            ushort *target = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                qt_memfill(target, c, spans->len);
            } else {
                ushort color = BYTE_MUL_RGB16(c, spans->coverage);
                int ialpha = 255 - spans->coverage;
                const ushort *end = target + spans->len;
                while (target < end) {
                    *target = color + BYTE_MUL_RGB16(*target, ialpha);
                    ++target;
                }
            }
        }
        return;
    }

    if (mode == QPainter::CompositionMode_SourceOver) {
        for (; count--; spans++) {
            if (!spans->len)
                continue;
            uint color = BYTE_MUL(data->solidColor.toArgb32(), spans->coverage);
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
        if (!spans->len) {
            ++spans;
            --count;
            continue;
        }
        int x = spans->x;
        const int y = spans->y;
        int right = x + spans->len;

        // compute length of adjacent spans
        for (int i = 1; i < count && spans[i].y == y && spans[i].x == right; ++i)
            right += spans[i].len;
        int length = right - x;

        while (length) {
            int l = qMin(BufferSize, length);
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
        , dest(nullptr)
    {
    }

    QSpanData *data;
    Operator op;

    BlendType *dest;

    alignas(8) BlendType buffer[BufferSize];
    alignas(8) BlendType src_buffer[BufferSize];
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
        dest = op.destFetch(buffer, data->rasterBuffer, x, y, len);
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

#if QT_CONFIG(raster_64bit)
class BlendSrcGenericRGB64 : public QBlendBase<QRgba64>
{
public:
    BlendSrcGenericRGB64(QSpanData *d, const Operator &o)
        : QBlendBase<QRgba64>(d, o)
    {
    }

    bool isSupported() const
    {
        return op.func64 && op.destFetch64;
    }

    const QRgba64 *fetch(int x, int y, int len)
    {
        dest = op.destFetch64(buffer, data->rasterBuffer, x, y, len);
        return op.srcFetch64(src_buffer, &op, data, y, x, len);
    }

    void process(int, int, int len, int coverage, const QRgba64 *src, int offset)
    {
        op.func64(dest + offset, src + offset, len, coverage);
    }

    void store(int x, int y, int len)
    {
        if (op.destStore64)
            op.destStore64(data->rasterBuffer, x, y, dest, len);
    }
};
#endif

static void blend_src_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    BlendSrcGeneric blend(data, getOperator(data, spans, count));
    handleSpans(count, spans, data, blend);
}

#if QT_CONFIG(raster_64bit)
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
#endif

static void blend_untransformed_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[BufferSize];
    uint src_buffer[BufferSize];
    Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    for (; count--; spans++) {
        if (!spans->len)
            continue;
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
                    int l = qMin(BufferSize, length);
                    const uint *src = op.srcFetch(src_buffer, &op, data, sy, sx, l);
                    uint *dest = op.destFetch(buffer, data->rasterBuffer, x, spans->y, l);
                    op.func(dest, src, l, coverage);
                    if (op.destStore)
                        op.destStore(data->rasterBuffer, x, spans->y, dest, l);
                    x += l;
                    sx += l;
                    length -= l;
                }
            }
        }
    }
}

#if QT_CONFIG(raster_64bit)
static void blend_untransformed_generic_rgb64(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    Operator op = getOperator(data, spans, count);
    if (!op.func64) {
        qCDebug(lcQtGuiDrawHelper, "blend_untransformed_generic_rgb64: unsupported 64-bit blend attempted, falling back to 32-bit");
        return blend_untransformed_generic(count, spans, userData);
    }
    alignas(8) QRgba64 buffer[BufferSize];
    alignas(8) QRgba64 src_buffer[BufferSize];

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    for (; count--; spans++) {
        if (!spans->len)
            continue;
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
                    int l = qMin(BufferSize, length);
                    const QRgba64 *src = op.srcFetch64(src_buffer, &op, data, sy, sx, l);
                    QRgba64 *dest = op.destFetch64(buffer, data->rasterBuffer, x, spans->y, l);
                    op.func64(dest, src, l, coverage);
                    if (op.destStore64)
                        op.destStore64(data->rasterBuffer, x, spans->y, dest, l);
                    x += l;
                    sx += l;
                    length -= l;
                }
            }
        }
    }
}
#endif

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

    for (; count--; spans++) {
        if (!spans->len)
            continue;
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

    const QSpan *end = spans + count;
    while (spans < end) {
        if (!spans->len) {
            ++spans;
            continue;
        }
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

    uint buffer[BufferSize];
    uint src_buffer[BufferSize];
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
            if (BufferSize < l)
                l = BufferSize;
            const uint *src = op.srcFetch(src_buffer, &op, data, sy, sx, l);
            uint *dest = op.destFetch(buffer, data->rasterBuffer, x, spans->y, l);
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

#if QT_CONFIG(raster_64bit)
static void blend_tiled_generic_rgb64(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    Operator op = getOperator(data, spans, count);
    if (!op.func64) {
        qCDebug(lcQtGuiDrawHelper, "blend_tiled_generic_rgb64: unsupported 64-bit blend attempted, falling back to 32-bit");
        return blend_tiled_generic(count, spans, userData);
    }
    alignas(8) QRgba64 buffer[BufferSize];
    alignas(8) QRgba64 src_buffer[BufferSize];

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    bool isBpp32 = qPixelLayouts[data->rasterBuffer->format].bpp == QPixelLayout::BPP32;
    bool isBpp64 = qPixelLayouts[data->rasterBuffer->format].bpp == QPixelLayout::BPP64;
    if (op.destFetch64 == destFetch64Undefined && image_width <= BufferSize && (isBpp32 || isBpp64)) {
        // If destination isn't blended into the result, we can do the tiling directly on destination pixels.
        while (count--) {
            int x = spans->x;
            int y = spans->y;
            int length = spans->len;
            int sx = (xoff + spans->x) % image_width;
            int sy = (spans->y + yoff) % image_height;
            if (sx < 0)
                sx += image_width;
            if (sy < 0)
                sy += image_height;

            int sl = qMin(image_width, length);
            if (sx > 0 && sl > 0) {
                int l = qMin(image_width - sx, sl);
                const QRgba64 *src = op.srcFetch64(src_buffer, &op, data, sy, sx, l);
                op.destStore64(data->rasterBuffer, x, y, src, l);
                x += l;
                sx += l;
                sl -= l;
                if (sx >= image_width)
                    sx = 0;
            }
            if (sl > 0) {
                Q_ASSERT(sx == 0);
                const QRgba64 *src = op.srcFetch64(src_buffer, &op, data, sy, sx, sl);
                op.destStore64(data->rasterBuffer, x, y, src, sl);
                x += sl;
                sx += sl;
                sl -= sl;
                if (sx >= image_width)
                    sx = 0;
            }
            if (isBpp32) {
                uint *dest = reinterpret_cast<uint *>(data->rasterBuffer->scanLine(y)) + x - image_width;
                for (int i = image_width; i < length; ++i)
                    dest[i] = dest[i - image_width];
            } else {
                quint64 *dest = reinterpret_cast<quint64 *>(data->rasterBuffer->scanLine(y)) + x - image_width;
                for (int i = image_width; i < length; ++i)
                    dest[i] = dest[i - image_width];
            }
            ++spans;
        }
        return;
    }

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
            if (BufferSize < l)
                l = BufferSize;
            const QRgba64 *src = op.srcFetch64(src_buffer, &op, data, sy, sx, l);
            QRgba64 *dest = op.destFetch64(buffer, data->rasterBuffer, x, spans->y, l);
            op.func64(dest, src, l, coverage);
            if (op.destStore64)
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
#endif

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
            if (BufferSize < l)
                l = BufferSize;
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
                if (BufferSize < l)
                    l = BufferSize;
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
                    if (BufferSize < l)
                        l = BufferSize;
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

/* Image formats here are target formats */
static const ProcessSpans processTextureSpansARGB32PM[NBlendTypes] = {
    blend_untransformed_argb,           // Untransformed
    blend_tiled_argb,                   // Tiled
    blend_src_generic,                  // Transformed
    blend_src_generic,                  // TransformedTiled
    blend_src_generic,                  // TransformedBilinear
    blend_src_generic                   // TransformedBilinearTiled
};

static const ProcessSpans processTextureSpansRGB16[NBlendTypes] = {
    blend_untransformed_rgb565,         // Untransformed
    blend_tiled_rgb565,                 // Tiled
    blend_src_generic,                  // Transformed
    blend_src_generic,                  // TransformedTiled
    blend_src_generic,                  // TransformedBilinear
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

#if QT_CONFIG(raster_64bit)
static const ProcessSpans processTextureSpansGeneric64[NBlendTypes] = {
    blend_untransformed_generic_rgb64,  // Untransformed
    blend_tiled_generic_rgb64,          // Tiled
    blend_src_generic_rgb64,            // Transformed
    blend_src_generic_rgb64,            // TransformedTiled
    blend_src_generic_rgb64,            // TransformedBilinear
    blend_src_generic_rgb64             // TransformedBilinearTiled
};
#endif

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
#if QT_CONFIG(raster_64bit)
#if defined(__SSE2__) || defined(__ARM_NEON__) || (Q_PROCESSOR_WORDSIZE == 8)
    case QImage::Format_ARGB32:
    case QImage::Format_RGBA8888:
#endif
    case QImage::Format_BGR30:
    case QImage::Format_A2BGR30_Premultiplied:
    case QImage::Format_RGB30:
    case QImage::Format_A2RGB30_Premultiplied:
    case QImage::Format_RGBX64:
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
    case QImage::Format_Grayscale16:
        proc = processTextureSpansGeneric64[blendType];
        break;
#endif // QT_CONFIG(raster_64bit)
    case QImage::Format_Invalid:
        Q_UNREACHABLE();
        return;
    default:
        proc = processTextureSpansGeneric[blendType];
        break;
    }
    proc(count, spans, userData);
}

static void blend_vertical_gradient_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

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
}

template<ProcessSpans blend_color>
static void blend_vertical_gradient(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    LinearGradientValues linear;
    getLinearGradientValues(&linear, data);

    // Based on the same logic as blend_vertical_gradient_argb.

    const int gss = GRADIENT_STOPTABLE_SIZE - 1;
    int yinc = int((linear.dy * data->m22 * gss) * FIXPT_SIZE);
    int off = int((((linear.dy * (data->m22 * qreal(0.5) + data->dy) + linear.off) * gss) * FIXPT_SIZE));

    while (count--) {
        int y = spans->y;

#if QT_CONFIG(raster_64bit)
        data->solidColor = qt_gradient_pixel64_fixed(&data->gradient, yinc * y + off);
#else
        data->solidColor = QRgba64::fromArgb32(qt_gradient_pixel_fixed(&data->gradient, yinc * y + off));
#endif
        blend_color(1, spans, userData);
        ++spans;
    }
}

void qBlendGradient(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    bool isVerticalGradient =
        data->txop <= QTransform::TxScale &&
        data->type == QSpanData::LinearGradient &&
        data->gradient.linear.end.x == data->gradient.linear.origin.x;
    switch (data->rasterBuffer->format) {
    case QImage::Format_RGB16:
        if (isVerticalGradient)
            return blend_vertical_gradient<blend_color_rgb16>(count, spans, userData);
        return blend_src_generic(count, spans, userData);
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        if (isVerticalGradient)
            return blend_vertical_gradient_argb(count, spans, userData);
        return blend_src_generic(count, spans, userData);
#if QT_CONFIG(raster_64bit)
#if defined(__SSE2__) || defined(__ARM_NEON__) || (Q_PROCESSOR_WORDSIZE == 8)
    case QImage::Format_ARGB32:
    case QImage::Format_RGBA8888:
#endif
    case QImage::Format_BGR30:
    case QImage::Format_A2BGR30_Premultiplied:
    case QImage::Format_RGB30:
    case QImage::Format_A2RGB30_Premultiplied:
    case QImage::Format_RGBX64:
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied:
        if (isVerticalGradient)
            return blend_vertical_gradient<blend_color_generic_rgb64>(count, spans, userData);
        return blend_src_generic_rgb64(count, spans, userData);
#endif // QT_CONFIG(raster_64bit)
    case QImage::Format_Invalid:
        break;
    default:
        if (isVerticalGradient)
            return blend_vertical_gradient<blend_color_generic>(count, spans, userData);
        return blend_src_generic(count, spans, userData);
    }
    Q_UNREACHABLE();
}

template <class DST> static
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

static inline void grayBlendPixel(quint32 *dst, int coverage, QRgba64 srcLinear, const QColorTrcLut *colorProfile)
{
    // Do a gammacorrected gray alphablend...
    const QRgba64 dstLinear = colorProfile ? colorProfile->toLinear64(*dst) : QRgba64::fromArgb32(*dst);

    QRgba64 blend = interpolate255(srcLinear, coverage, dstLinear, 255 - coverage);

    *dst = colorProfile ? colorProfile->fromLinear64(blend) : toArgb32(blend);
}

static inline void alphamapblend_argb32(quint32 *dst, int coverage, QRgba64 srcLinear, quint32 src, const QColorTrcLut *colorProfile)
{
    if (coverage == 0) {
        // nothing
    } else if (coverage == 255 || !colorProfile) {
        blend_pixel(*dst, src, coverage);
    } else if (*dst < 0xff000000) {
        // Give up and do a naive gray alphablend. Needed to deal with ARGB32 and invalid ARGB32_premultiplied, see QTBUG-60571
        blend_pixel(*dst, src, coverage);
    } else if (src >= 0xff000000) {
        grayBlendPixel(dst, coverage, srcLinear, colorProfile);
    } else {
        // First do naive blend with text-color
        QRgb s = *dst;
        blend_pixel(s, src);
        // Then gamma-corrected blend with glyph shape
        QRgba64 s64 = colorProfile ? colorProfile->toLinear64(s) : QRgba64::fromArgb32(s);
        grayBlendPixel(dst, coverage, s64, colorProfile);
    }
}

#if QT_CONFIG(raster_64bit)

static inline void grayBlendPixel(QRgba64 &dst, int coverage, QRgba64 srcLinear, const QColorTrcLut *colorProfile)
{
    // Do a gammacorrected gray alphablend...
    QRgba64 dstColor = dst;
    if (colorProfile) {
        if (dstColor.isOpaque())
            dstColor = colorProfile->toLinear(dstColor);
        else if (!dstColor.isTransparent())
            dstColor = colorProfile->toLinear(dstColor.unpremultiplied()).premultiplied();
    }

    blend_pixel(dstColor, srcLinear, coverage);

    if (colorProfile) {
        if (dstColor.isOpaque())
            dstColor = colorProfile->fromLinear(dstColor);
        else if (!dstColor.isTransparent())
            dstColor = colorProfile->fromLinear(dstColor.unpremultiplied()).premultiplied();
    }
    dst = dstColor;
}

static inline void alphamapblend_generic(int coverage, QRgba64 *dest, int x, const QRgba64 &srcLinear, const QRgba64 &src, const QColorTrcLut *colorProfile)
{
    if (coverage == 0) {
        // nothing
    } else if (coverage == 255) {
        blend_pixel(dest[x], src);
    } else if (src.isOpaque()) {
        grayBlendPixel(dest[x], coverage, srcLinear, colorProfile);
    } else {
        // First do naive blend with text-color
        QRgba64 s = dest[x];
        blend_pixel(s, src);
        // Then gamma-corrected blend with glyph shape
        if (colorProfile)
            s = colorProfile->toLinear(s);
        grayBlendPixel(dest[x], coverage, s, colorProfile);
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

    const QColorTrcLut *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA8Text();

    QRgba64 srcColor = color;
    if (colorProfile && color.isOpaque())
        srcColor = colorProfile->toLinear(srcColor);

    alignas(8) QRgba64 buffer[BufferSize];
    const DestFetchProc64 destFetch64 = destFetchProc64[rasterBuffer->format];
    const DestStoreProc64 destStore64 = destStoreProc64[rasterBuffer->format];

    if (!clip) {
        for (int ly = 0; ly < mapHeight; ++ly) {
            int i = x;
            int length = mapWidth;
            while (length > 0) {
                int l = qMin(BufferSize, length);
                QRgba64 *dest = destFetch64(buffer, rasterBuffer, i, y + ly, l);
                for (int j=0; j < l; ++j) {
                    const int coverage = map[j + (i - x)];
                    alphamapblend_generic(coverage, dest, j, srcColor, color, colorProfile);
                }
                if (destStore64)
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
                Q_ASSERT(end - start <= BufferSize);
                QRgba64 *dest = destFetch64(buffer, rasterBuffer, start, clip.y, end - start);

                for (int xp=start; xp<end; ++xp) {
                    const int coverage = map[xp - x];
                    alphamapblend_generic(coverage, dest, xp - start, srcColor, color, colorProfile);
                }
                if (destStore64)
                    destStore64(rasterBuffer, start, clip.y, dest, end - start);
            } // for (i -> line.count)
            map += mapStride;
        } // for (yp -> bottom)
    }
}
#else
static void qt_alphamapblit_generic(QRasterBuffer *rasterBuffer,
                                    int x, int y, const QRgba64 &color,
                                    const uchar *map,
                                    int mapWidth, int mapHeight, int mapStride,
                                    const QClipData *clip, bool useGammaCorrection)
{
    if (color.isTransparent())
        return;

    const quint32 c = color.toArgb32();

    const QColorTrcLut *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA8Text();

    QRgba64 srcColor = color;
    if (colorProfile && color.isOpaque())
        srcColor = colorProfile->toLinear(srcColor);

    quint32 buffer[BufferSize];
    const DestFetchProc destFetch = destFetchProc[rasterBuffer->format];
    const DestStoreProc destStore = destStoreProc[rasterBuffer->format];

    if (!clip) {
        for (int ly = 0; ly < mapHeight; ++ly) {
            int i = x;
            int length = mapWidth;
            while (length > 0) {
                int l = qMin(BufferSize, length);
                quint32 *dest = destFetch(buffer, rasterBuffer, i, y + ly, l);
                for (int j=0; j < l; ++j) {
                    const int coverage = map[j + (i - x)];
                    alphamapblend_argb32(dest + j, coverage, srcColor, c, colorProfile);
                }
                if (destStore)
                    destStore(rasterBuffer, i, y + ly, dest, l);
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
                Q_ASSERT(end - start <= BufferSize);
                quint32 *dest = destFetch(buffer, rasterBuffer, start, clip.y, end - start);

                for (int xp=start; xp<end; ++xp) {
                    const int coverage = map[xp - x];
                    alphamapblend_argb32(dest + xp - x, coverage, srcColor, color, colorProfile);
                }
                if (destStore)
                    destStore(rasterBuffer, start, clip.y, dest, end - start);
            } // for (i -> line.count)
            map += mapStride;
        } // for (yp -> bottom)
    }
}
#endif

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
    if (useGammaCorrection || !color.isOpaque()) {
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

    const QColorTrcLut *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA8Text();

    QRgba64 srcColor = color;
    if (colorProfile && color.isOpaque())
        srcColor = colorProfile->toLinear(srcColor);

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

static inline void rgbBlendPixel(quint32 *dst, int coverage, QRgba64 slinear, const QColorTrcLut *colorProfile)
{
    // Do a gammacorrected RGB alphablend...
    const QRgba64 dlinear = colorProfile ? colorProfile->toLinear64(*dst) : QRgba64::fromArgb32(*dst);

    QRgba64 blend = rgbBlend(dlinear, slinear, coverage);

    *dst = colorProfile ? colorProfile->fromLinear64(blend) : toArgb32(blend);
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

static inline void alphargbblend_argb32(quint32 *dst, uint coverage, const QRgba64 &srcLinear, quint32 src, const QColorTrcLut *colorProfile)
{
    if (coverage == 0xff000000) {
        // nothing
    } else if (coverage == 0xffffffff && qAlpha(src) == 255) {
        blend_pixel(*dst, src);
    } else if (*dst < 0xff000000) {
        // Give up and do a naive gray alphablend. Needed to deal with ARGB32 and invalid ARGB32_premultiplied, see QTBUG-60571
        blend_pixel(*dst, src, qRgbAvg(coverage));
    } else if (!colorProfile) {
        // First do naive blend with text-color
        QRgb s = *dst;
        blend_pixel(s, src);
        // Then a naive blend with glyph shape
        *dst = rgbBlend(*dst, s, coverage);
    } else if (srcLinear.isOpaque()) {
        rgbBlendPixel(dst, coverage, srcLinear, colorProfile);
    } else {
        // First do naive blend with text-color
        QRgb s = *dst;
        blend_pixel(s, src);
        // Then gamma-corrected blend with glyph shape
        QRgba64 s64 = colorProfile ? colorProfile->toLinear64(s) : QRgba64::fromArgb32(s);
        rgbBlendPixel(dst, coverage, s64, colorProfile);
    }
}

#if QT_CONFIG(raster_64bit)
static inline void rgbBlendPixel(QRgba64 &dst, int coverage, QRgba64 slinear, const QColorTrcLut *colorProfile)
{
    // Do a gammacorrected RGB alphablend...
    const QRgba64 dlinear = colorProfile ? colorProfile->toLinear64(dst) : dst;

    QRgba64 blend = rgbBlend(dlinear, slinear, coverage);

    dst = colorProfile ? colorProfile->fromLinear(blend) : blend;
}

static inline void alphargbblend_generic(uint coverage, QRgba64 *dest, int x, const QRgba64 &srcLinear, const QRgba64 &src, const QColorTrcLut *colorProfile)
{
    if (coverage == 0xff000000) {
        // nothing
    } else if (coverage == 0xffffffff) {
        blend_pixel(dest[x], src);
    } else if (!dest[x].isOpaque()) {
        // Do a gray alphablend.
        alphamapblend_generic(qRgbAvg(coverage), dest, x, srcLinear, src, colorProfile);
    } else if (src.isOpaque()) {
        rgbBlendPixel(dest[x], coverage, srcLinear, colorProfile);
    } else {
        // First do naive blend with text-color
        QRgba64 s = dest[x];
        blend_pixel(s, src);
        // Then gamma-corrected blend with glyph shape
        if (colorProfile)
            s = colorProfile->toLinear(s);
        rgbBlendPixel(dest[x], coverage, s, colorProfile);
    }
}

static void qt_alphargbblit_generic(QRasterBuffer *rasterBuffer,
                                    int x, int y, const QRgba64 &color,
                                    const uint *src, int mapWidth, int mapHeight, int srcStride,
                                    const QClipData *clip, bool useGammaCorrection)
{
    if (color.isTransparent())
        return;

    const QColorTrcLut *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA32Text();

    QRgba64 srcColor = color;
    if (colorProfile && color.isOpaque())
        srcColor = colorProfile->toLinear(srcColor);

    alignas(8) QRgba64 buffer[BufferSize];
    const DestFetchProc64 destFetch64 = destFetchProc64[rasterBuffer->format];
    const DestStoreProc64 destStore64 = destStoreProc64[rasterBuffer->format];

    if (!clip) {
        for (int ly = 0; ly < mapHeight; ++ly) {
            int i = x;
            int length = mapWidth;
            while (length > 0) {
                int l = qMin(BufferSize, length);
                QRgba64 *dest = destFetch64(buffer, rasterBuffer, i, y + ly, l);
                for (int j=0; j < l; ++j) {
                    const uint coverage = src[j + (i - x)];
                    alphargbblend_generic(coverage, dest, j, srcColor, color, colorProfile);
                }
                if (destStore64)
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
                Q_ASSERT(end - start <= BufferSize);
                QRgba64 *dest = destFetch64(buffer, rasterBuffer, start, clip.y, end - start);

                for (int xp=start; xp<end; ++xp) {
                    const uint coverage = src[xp - x];
                    alphargbblend_generic(coverage, dest, xp - start, srcColor, color, colorProfile);
                }
                if (destStore64)
                    destStore64(rasterBuffer, start, clip.y, dest, end - start);
            } // for (i -> line.count)
            src += srcStride;
        } // for (yp -> bottom)
    }
}
#else
static void qt_alphargbblit_generic(QRasterBuffer *rasterBuffer,
                                    int x, int y, const QRgba64 &color,
                                    const uint *src, int mapWidth, int mapHeight, int srcStride,
                                    const QClipData *clip, bool useGammaCorrection)
{
    if (color.isTransparent())
        return;

    const quint32 c = color.toArgb32();

    const QColorTrcLut *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA32Text();

    QRgba64 srcColor = color;
    if (colorProfile && color.isOpaque())
        srcColor = colorProfile->toLinear(srcColor);

    quint32 buffer[BufferSize];
    const DestFetchProc destFetch = destFetchProc[rasterBuffer->format];
    const DestStoreProc destStore = destStoreProc[rasterBuffer->format];

    if (!clip) {
        for (int ly = 0; ly < mapHeight; ++ly) {
            int i = x;
            int length = mapWidth;
            while (length > 0) {
                int l = qMin(BufferSize, length);
                quint32 *dest = destFetch(buffer, rasterBuffer, i, y + ly, l);
                for (int j=0; j < l; ++j) {
                    const uint coverage = src[j + (i - x)];
                    alphargbblend_argb32(dest + j, coverage, srcColor, c, colorProfile);
                }
                if (destStore)
                    destStore(rasterBuffer, i, y + ly, dest, l);
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
                Q_ASSERT(end - start <= BufferSize);
                quint32 *dest = destFetch(buffer, rasterBuffer, start, clip.y, end - start);

                for (int xp=start; xp<end; ++xp) {
                    const uint coverage = src[xp - x];
                    alphargbblend_argb32(dest + xp - start, coverage, srcColor, c, colorProfile);
                }
                if (destStore)
                    destStore(rasterBuffer, start, clip.y, dest, end - start);
            } // for (i -> line.count)
            src += srcStride;
        } // for (yp -> bottom)
    }
}
#endif

static void qt_alphargbblit_argb32(QRasterBuffer *rasterBuffer,
                                   int x, int y, const QRgba64 &color,
                                   const uint *src, int mapWidth, int mapHeight, int srcStride,
                                   const QClipData *clip, bool useGammaCorrection)
{
    if (color.isTransparent())
        return;

    const quint32 c = color.toArgb32();

    const QColorTrcLut *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA32Text();

    QRgba64 srcColor = color;
    if (colorProfile && color.isOpaque())
        srcColor = colorProfile->toLinear(srcColor);

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
    const QPixelLayout &layout = qPixelLayouts[rasterBuffer->format];
    quint32 c32 = color.toArgb32();
    quint16 c16;
    layout.storeFromARGB32PM(reinterpret_cast<uchar *>(&c16), &c32, 0, 1, nullptr, nullptr);
    qt_rectfill<quint16>(reinterpret_cast<quint16 *>(rasterBuffer->buffer()),
                         c16, x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_quint24(QRasterBuffer *rasterBuffer,
                                int x, int y, int width, int height,
                                const QRgba64 &color)
{
    const QPixelLayout &layout = qPixelLayouts[rasterBuffer->format];
    quint32 c32 = color.toArgb32();
    quint24 c24;
    layout.storeFromARGB32PM(reinterpret_cast<uchar *>(&c24), &c32, 0, 1, nullptr, nullptr);
    qt_rectfill<quint24>(reinterpret_cast<quint24 *>(rasterBuffer->buffer()),
                         c24, x, y, width, height, rasterBuffer->bytesPerLine());
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

static void qt_rectfill_quint64(QRasterBuffer *rasterBuffer,
                                int x, int y, int width, int height,
                                const QRgba64 &color)
{
    const auto store = qStoreFromRGBA64PM[rasterBuffer->format];
    quint64 c64;
    store(reinterpret_cast<uchar *>(&c64), &color, 0, 1, nullptr, nullptr);
    qt_rectfill<quint64>(reinterpret_cast<quint64 *>(rasterBuffer->buffer()),
                         c64, x, y, width, height, rasterBuffer->bytesPerLine());
}

// Map table for destination image format. Contains function pointers
// for blends of various types unto the destination

DrawHelper qDrawHelper[QImage::NImageFormats] =
{
    // Format_Invalid,
    { nullptr, nullptr, nullptr, nullptr, nullptr },
    // Format_Mono,
    {
        blend_color_generic,
        nullptr, nullptr, nullptr, nullptr
    },
    // Format_MonoLSB,
    {
        blend_color_generic,
        nullptr, nullptr, nullptr, nullptr
    },
    // Format_Indexed8,
    {
        blend_color_generic,
        nullptr, nullptr, nullptr, nullptr
    },
    // Format_RGB32,
    {
        blend_color_argb,
        qt_bitmapblit_argb32,
        qt_alphamapblit_argb32,
        qt_alphargbblit_argb32,
        qt_rectfill_argb32
    },
    // Format_ARGB32,
    {
        blend_color_generic,
        qt_bitmapblit_argb32,
        qt_alphamapblit_argb32,
        qt_alphargbblit_argb32,
        qt_rectfill_nonpremul_argb32
    },
    // Format_ARGB32_Premultiplied
    {
        blend_color_argb,
        qt_bitmapblit_argb32,
        qt_alphamapblit_argb32,
        qt_alphargbblit_argb32,
        qt_rectfill_argb32
    },
    // Format_RGB16
    {
        blend_color_rgb16,
        qt_bitmapblit_quint16,
        qt_alphamapblit_quint16,
        qt_alphargbblit_generic,
        qt_rectfill_quint16
    },
    // Format_ARGB8565_Premultiplied
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint24
    },
    // Format_RGB666
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint24
    },
    // Format_ARGB6666_Premultiplied
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint24
    },
    // Format_RGB555
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint16
    },
    // Format_ARGB8555_Premultiplied
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint24
    },
    // Format_RGB888
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint24
    },
    // Format_RGB444
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint16
    },
    // Format_ARGB4444_Premultiplied
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint16
    },
    // Format_RGBX8888
    {
        blend_color_generic,
        qt_bitmapblit_rgba8888,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgba
    },
    // Format_RGBA8888
    {
        blend_color_generic,
        qt_bitmapblit_rgba8888,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_nonpremul_rgba
    },
    // Format_RGB8888_Premultiplied
    {
        blend_color_generic,
        qt_bitmapblit_rgba8888,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgba
    },
    // Format_BGR30
    {
        blend_color_generic_rgb64,
        qt_bitmapblit_rgb30<PixelOrderBGR>,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgb30<PixelOrderBGR>
    },
    // Format_A2BGR30_Premultiplied
    {
        blend_color_generic_rgb64,
        qt_bitmapblit_rgb30<PixelOrderBGR>,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgb30<PixelOrderBGR>
    },
    // Format_RGB30
    {
        blend_color_generic_rgb64,
        qt_bitmapblit_rgb30<PixelOrderRGB>,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgb30<PixelOrderRGB>
    },
    // Format_A2RGB30_Premultiplied
    {
        blend_color_generic_rgb64,
        qt_bitmapblit_rgb30<PixelOrderRGB>,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_rgb30<PixelOrderRGB>
    },
    // Format_Alpha8
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_alpha
    },
    // Format_Grayscale8
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_gray
    },
    // Format_RGBX64
    {
        blend_color_generic_rgb64,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint64
    },
    // Format_RGBA64
    {
        blend_color_generic_rgb64,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint64
    },
    // Format_RGBA64_Premultiplied
    {
        blend_color_generic_rgb64,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint64
    },
    // Format_Grayscale16
    {
        blend_color_generic_rgb64,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint16
    },
    // Format_BGR888
    {
        blend_color_generic,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint24
    },
};

#if !defined(__SSE2__)
void qt_memfill64(quint64 *dest, quint64 color, qsizetype count)
{
    qt_memfill_template<quint64>(dest, color, count);
}
#endif

#if defined(QT_COMPILER_SUPPORTS_SSSE3) && defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && !defined(Q_CC_CLANG)
__attribute__((optimize("no-tree-vectorize")))
#endif
void qt_memfill24(quint24 *dest, quint24 color, qsizetype count)
{
#  ifdef QT_COMPILER_SUPPORTS_SSSE3
    extern void qt_memfill24_ssse3(quint24 *, quint24, qsizetype);
    if (qCpuHasFeature(SSSE3))
        return qt_memfill24_ssse3(dest, color, count);
#  endif

    const quint32 v = color;
    quint24 *end = dest + count;

    // prolog: align dest to 32bit
    while ((quintptr(dest) & 0x3) && dest < end) {
        *dest++ = v;
    }
    if (dest >= end)
        return;

    const uint val1 = qFromBigEndian((v <<  8) | (v >> 16));
    const uint val2 = qFromBigEndian((v << 16) | (v >>  8));
    const uint val3 = qFromBigEndian((v << 24) | (v >>  0));

    for ( ; dest <= (end - 4); dest += 4) {
       quint32 *dst = reinterpret_cast<quint32 *>(dest);
       dst[0] = val1;
       dst[1] = val2;
       dst[2] = val3;
    }

    // less than 4px left
    switch (end - dest) {
    case 3:
        *dest++ = v;
        Q_FALLTHROUGH();
    case 2:
        *dest++ = v;
        Q_FALLTHROUGH();
    case 1:
        *dest++ = v;
    }
}

void qt_memfill16(quint16 *dest, quint16 value, qsizetype count)
{
    const int align = quintptr(dest) & 0x3;
    if (align) {
        *dest++ = value;
        --count;
    }

    if (count & 0x1)
        dest[count - 1] = value;

    const quint32 value32 = (value << 16) | value;
    qt_memfill32(reinterpret_cast<quint32*>(dest), value32, count / 2);
}

#if !defined(__SSE2__) && !defined(__ARM_NEON__) && !defined(__MIPS_DSP__)
void qt_memfill32(quint32 *dest, quint32 color, qsizetype count)
{
    qt_memfill_template<quint32>(dest, color, count);
}
#endif
#ifdef __SSE2__
decltype(qt_memfill32_sse2) *qt_memfill32 = nullptr;
decltype(qt_memfill64_sse2) *qt_memfill64 = nullptr;
#endif

#ifdef QT_COMPILER_SUPPORTS_SSE4_1
template<QtPixelOrder> void QT_FASTCALL storeA2RGB30PMFromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count, const QVector<QRgb> *, QDitherInfo *);
#endif

extern void qInitBlendFunctions();

static void qInitDrawhelperFunctions()
{
    // Set up basic blend function tables.
    qInitBlendFunctions();

#ifdef __SSE2__
#  ifndef __AVX2__
    qt_memfill32 = qt_memfill32_sse2;
    qt_memfill64 = qt_memfill64_sse2;
#  endif
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
    extern void QT_FASTCALL comp_func_solid_Source_sse2(uint *destPixels, int length, uint color, uint const_alpha);
    extern void QT_FASTCALL comp_func_Plus_sse2(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
    qt_functionForMode_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_sse2;
    qt_functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_sse2;
    qt_functionForMode_C[QPainter::CompositionMode_Source] = comp_func_Source_sse2;
    qt_functionForModeSolid_C[QPainter::CompositionMode_Source] = comp_func_solid_Source_sse2;
    qt_functionForMode_C[QPainter::CompositionMode_Plus] = comp_func_Plus_sse2;

#ifdef QT_COMPILER_SUPPORTS_SSSE3
    if (qCpuHasFeature(SSSE3)) {
        extern void qt_blend_argb32_on_argb32_ssse3(uchar *destPixels, int dbpl,
                                                    const uchar *srcPixels, int sbpl,
                                                    int w, int h,
                                                    int const_alpha);

        extern const uint * QT_FASTCALL qt_fetchUntransformed_888_ssse3(uint *buffer, const Operator *, const QSpanData *data,
                                                                        int y, int x, int length);
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        sourceFetchUntransformed[QImage::Format_RGB888] = qt_fetchUntransformed_888_ssse3;
        extern void QT_FASTCALL rbSwap_888_ssse3(uchar *dst, const uchar *src, int count);
        qPixelLayouts[QImage::Format_RGB888].rbSwap = rbSwap_888_ssse3;
        qPixelLayouts[QImage::Format_BGR888].rbSwap = rbSwap_888_ssse3;
    }
#endif // SSSE3

#if defined(QT_COMPILER_SUPPORTS_SSE4_1)
    if (qCpuHasFeature(SSE4_1)) {
        extern void QT_FASTCALL convertARGB32ToARGB32PM_sse4(uint *buffer, int count, const QVector<QRgb> *);
        extern void QT_FASTCALL convertRGBA8888ToARGB32PM_sse4(uint *buffer, int count, const QVector<QRgb> *);
        extern const uint *QT_FASTCALL fetchARGB32ToARGB32PM_sse4(uint *buffer, const uchar *src, int index, int count,
                                                                  const QVector<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_sse4(uint *buffer, const uchar *src, int index, int count,
                                                                    const QVector<QRgb> *, QDitherInfo *);
        extern const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_sse4(QRgba64 *buffer, const uint *src, int count,
                                                                        const QVector<QRgb> *, QDitherInfo *);
        extern const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_sse4(QRgba64 *buffer, const uint *src, int count,
                                                                          const QVector<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_sse4(QRgba64 *buffer, const uchar *src, int index, int count,
                                                                     const QVector<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_sse4(QRgba64 *buffer, const uchar *src, int index, int count,
                                                                       const QVector<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeARGB32FromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                                                      const QVector<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA8888FromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                                                        const QVector<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBXFromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                                                    const QVector<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeARGB32FromRGBA64PM_sse4(uchar *dest, const QRgba64 *src, int index, int count,
                                                             const QVector<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA8888FromRGBA64PM_sse4(uchar *dest, const QRgba64 *src, int index, int count,
                                                              const QVector<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL destStore64ARGB32_sse4(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length);
        extern void QT_FASTCALL destStore64RGBA8888_sse4(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length);
#  ifndef __AVX2__
        qPixelLayouts[QImage::Format_ARGB32].fetchToARGB32PM = fetchARGB32ToARGB32PM_sse4;
        qPixelLayouts[QImage::Format_ARGB32].convertToARGB32PM = convertARGB32ToARGB32PM_sse4;
        qPixelLayouts[QImage::Format_RGBA8888].fetchToARGB32PM = fetchRGBA8888ToARGB32PM_sse4;
        qPixelLayouts[QImage::Format_RGBA8888].convertToARGB32PM = convertRGBA8888ToARGB32PM_sse4;
        qPixelLayouts[QImage::Format_ARGB32].fetchToRGBA64PM = fetchARGB32ToRGBA64PM_sse4;
        qPixelLayouts[QImage::Format_ARGB32].convertToRGBA64PM = convertARGB32ToRGBA64PM_sse4;
        qPixelLayouts[QImage::Format_RGBA8888].fetchToRGBA64PM = fetchRGBA8888ToRGBA64PM_sse4;
        qPixelLayouts[QImage::Format_RGBA8888].convertToRGBA64PM = convertRGBA8888ToRGBA64PM_sse4;
        qPixelLayouts[QImage::Format_RGBX8888].fetchToRGBA64PM = fetchRGBA8888ToRGBA64PM_sse4;
        qPixelLayouts[QImage::Format_RGBX8888].convertToRGBA64PM = convertRGBA8888ToRGBA64PM_sse4;
#  endif
        qPixelLayouts[QImage::Format_ARGB32].storeFromARGB32PM = storeARGB32FromARGB32PM_sse4;
        qPixelLayouts[QImage::Format_RGBA8888].storeFromARGB32PM = storeRGBA8888FromARGB32PM_sse4;
        qPixelLayouts[QImage::Format_RGBX8888].storeFromARGB32PM = storeRGBXFromARGB32PM_sse4;
        qPixelLayouts[QImage::Format_A2BGR30_Premultiplied].storeFromARGB32PM = storeA2RGB30PMFromARGB32PM_sse4<PixelOrderBGR>;
        qPixelLayouts[QImage::Format_A2RGB30_Premultiplied].storeFromARGB32PM = storeA2RGB30PMFromARGB32PM_sse4<PixelOrderRGB>;
        qStoreFromRGBA64PM[QImage::Format_ARGB32] = storeARGB32FromRGBA64PM_sse4;
        qStoreFromRGBA64PM[QImage::Format_RGBA8888] = storeRGBA8888FromRGBA64PM_sse4;
#if QT_CONFIG(raster_64bit)
        destStoreProc64[QImage::Format_ARGB32] = destStore64ARGB32_sse4;
        destStoreProc64[QImage::Format_RGBA8888] = destStore64RGBA8888_sse4;
#endif
    }
#endif

#if defined(QT_COMPILER_SUPPORTS_AVX2)
    if (qCpuHasFeature(ArchHaswell)) {
        qt_memfill32 = qt_memfill32_avx2;
        qt_memfill64 = qt_memfill64_avx2;
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
        extern void QT_FASTCALL comp_func_SourceOver_avx2(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_solid_SourceOver_avx2(uint *destPixels, int length, uint color, uint const_alpha);
        qt_functionForMode_C[QPainter::CompositionMode_Source] = comp_func_Source_avx2;
        qt_functionForMode_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_avx2;
        qt_functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_avx2;
#if QT_CONFIG(raster_64bit)
        extern void QT_FASTCALL comp_func_Source_rgb64_avx2(QRgba64 *destPixels, const QRgba64 *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_SourceOver_rgb64_avx2(QRgba64 *destPixels, const QRgba64 *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_solid_SourceOver_rgb64_avx2(QRgba64 *destPixels, int length, QRgba64 color, uint const_alpha);
        qt_functionForMode64_C[QPainter::CompositionMode_Source] = comp_func_Source_rgb64_avx2;
        qt_functionForMode64_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_rgb64_avx2;
        qt_functionForModeSolid64_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_rgb64_avx2;
#endif

        extern void QT_FASTCALL fetchTransformedBilinearARGB32PM_simple_scale_helper_avx2(uint *b, uint *end, const QTextureData &image,
                                                                                          int &fx, int &fy, int fdx, int /*fdy*/);
        extern void QT_FASTCALL fetchTransformedBilinearARGB32PM_downscale_helper_avx2(uint *b, uint *end, const QTextureData &image,
                                                                                       int &fx, int &fy, int fdx, int /*fdy*/);
        extern void QT_FASTCALL fetchTransformedBilinearARGB32PM_fast_rotate_helper_avx2(uint *b, uint *end, const QTextureData &image,
                                                                                         int &fx, int &fy, int fdx, int fdy);

        bilinearFastTransformHelperARGB32PM[0][SimpleScaleTransform] = fetchTransformedBilinearARGB32PM_simple_scale_helper_avx2;
        bilinearFastTransformHelperARGB32PM[0][DownscaleTransform] = fetchTransformedBilinearARGB32PM_downscale_helper_avx2;
        bilinearFastTransformHelperARGB32PM[0][FastRotateTransform] = fetchTransformedBilinearARGB32PM_fast_rotate_helper_avx2;

        extern void QT_FASTCALL convertARGB32ToARGB32PM_avx2(uint *buffer, int count, const QVector<QRgb> *);
        extern void QT_FASTCALL convertRGBA8888ToARGB32PM_avx2(uint *buffer, int count, const QVector<QRgb> *);
        extern const uint *QT_FASTCALL fetchARGB32ToARGB32PM_avx2(uint *buffer, const uchar *src, int index, int count,
                                                                  const QVector<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_avx2(uint *buffer, const uchar *src, int index, int count,
                                                                    const QVector<QRgb> *, QDitherInfo *);
        qPixelLayouts[QImage::Format_ARGB32].fetchToARGB32PM = fetchARGB32ToARGB32PM_avx2;
        qPixelLayouts[QImage::Format_ARGB32].convertToARGB32PM = convertARGB32ToARGB32PM_avx2;
        qPixelLayouts[QImage::Format_RGBA8888].fetchToARGB32PM = fetchRGBA8888ToARGB32PM_avx2;
        qPixelLayouts[QImage::Format_RGBA8888].convertToARGB32PM = convertRGBA8888ToARGB32PM_avx2;

#if QT_CONFIG(raster_64bit)
        extern const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_avx2(QRgba64 *, const uint *, int, const QVector<QRgb> *, QDitherInfo *);
        extern const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_avx2(QRgba64 *, const uint *, int count, const QVector<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_avx2(QRgba64 *, const uchar *, int, int, const QVector<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_avx2(QRgba64 *, const uchar *, int, int, const QVector<QRgb> *, QDitherInfo *);
        qPixelLayouts[QImage::Format_ARGB32].convertToRGBA64PM = convertARGB32ToRGBA64PM_avx2;
        qPixelLayouts[QImage::Format_RGBX8888].convertToRGBA64PM = convertRGBA8888ToRGBA64PM_avx2;
        qPixelLayouts[QImage::Format_ARGB32].fetchToRGBA64PM = fetchARGB32ToRGBA64PM_avx2;
        qPixelLayouts[QImage::Format_RGBX8888].fetchToRGBA64PM = fetchRGBA8888ToRGBA64PM_avx2;
#endif
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
    extern void QT_FASTCALL convertARGB32ToARGB32PM_neon(uint *buffer, int count, const QVector<QRgb> *);
    extern void QT_FASTCALL convertRGBA8888ToARGB32PM_neon(uint *buffer, int count, const QVector<QRgb> *);
    extern const uint *QT_FASTCALL fetchARGB32ToARGB32PM_neon(uint *buffer, const uchar *src, int index, int count,
                                                              const QVector<QRgb> *, QDitherInfo *);
    extern const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_neon(uint *buffer, const uchar *src, int index, int count,
                                                                const QVector<QRgb> *, QDitherInfo *);
   extern const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_neon(QRgba64 *buffer, const uint *src, int count,
                                                                   const QVector<QRgb> *, QDitherInfo *);
   extern const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_neon(QRgba64 *buffer, const uint *src, int count,
                                                                     const QVector<QRgb> *, QDitherInfo *);
   extern const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_neon(QRgba64 *buffer, const uchar *src, int index, int count,
                                                                const QVector<QRgb> *, QDitherInfo *);
   extern const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_neon(QRgba64 *buffer, const uchar *src, int index, int count,
                                                                  const QVector<QRgb> *, QDitherInfo *);
    extern void QT_FASTCALL storeARGB32FromARGB32PM_neon(uchar *dest, const uint *src, int index, int count,
                                                         const QVector<QRgb> *, QDitherInfo *);
    extern void QT_FASTCALL storeRGBA8888FromARGB32PM_neon(uchar *dest, const uint *src, int index, int count,
                                                           const QVector<QRgb> *, QDitherInfo *);
    extern void QT_FASTCALL storeRGBXFromARGB32PM_neon(uchar *dest, const uint *src, int index, int count,
                                                       const QVector<QRgb> *, QDitherInfo *);
    qPixelLayouts[QImage::Format_ARGB32].fetchToARGB32PM = fetchARGB32ToARGB32PM_neon;
    qPixelLayouts[QImage::Format_ARGB32].convertToARGB32PM = convertARGB32ToARGB32PM_neon;
    qPixelLayouts[QImage::Format_ARGB32].storeFromARGB32PM = storeARGB32FromARGB32PM_neon;
    qPixelLayouts[QImage::Format_ARGB32].fetchToRGBA64PM = fetchARGB32ToRGBA64PM_neon;
    qPixelLayouts[QImage::Format_ARGB32].convertToRGBA64PM = convertARGB32ToRGBA64PM_neon;
    qPixelLayouts[QImage::Format_RGBA8888].fetchToARGB32PM = fetchRGBA8888ToARGB32PM_neon;
    qPixelLayouts[QImage::Format_RGBA8888].convertToARGB32PM = convertRGBA8888ToARGB32PM_neon;
    qPixelLayouts[QImage::Format_RGBA8888].storeFromARGB32PM = storeRGBA8888FromARGB32PM_neon;
    qPixelLayouts[QImage::Format_RGBA8888].fetchToRGBA64PM = fetchRGBA8888ToRGBA64PM_neon;
    qPixelLayouts[QImage::Format_RGBA8888].convertToRGBA64PM = convertRGBA8888ToRGBA64PM_neon;
    qPixelLayouts[QImage::Format_RGBX8888].storeFromARGB32PM = storeRGBXFromARGB32PM_neon;
    qPixelLayouts[QImage::Format_RGBX8888].fetchToRGBA64PM = fetchRGBA8888ToRGBA64PM_neon;
    qPixelLayouts[QImage::Format_RGBX8888].convertToRGBA64PM = convertRGBA8888ToRGBA64PM_neon;
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
