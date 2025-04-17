// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2018 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdrawhelper_p.h"

#include <qstylehints.h>
#include <qguiapplication.h>
#include <qatomic.h>
#include <private/qcolortransform_p.h>
#include <private/qcolortrclut_p.h>
#include <private/qdrawhelper_p.h>
#include <private/qdrawhelper_x86_p.h>
#include <private/qdrawingprimitive_sse2_p.h>
#include <private/qdrawhelper_loongarch64_p.h>
#include <private/qdrawingprimitive_lsx_p.h>
#include <private/qdrawhelper_neon_p.h>
#if defined(QT_COMPILER_SUPPORTS_MIPS_DSP) || defined(QT_COMPILER_SUPPORTS_MIPS_DSPR2)
#include <private/qdrawhelper_mips_dsp_p.h>
#endif
#include <private/qguiapplication_p.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qpainter_p.h>
#include <private/qpixellayout_p.h>
#include <private/qrgba64_p.h>
#include <qendian.h>
#include <qloggingcategory.h>
#include <qmath.h>

#if QT_CONFIG(qtgui_threadpool)
#include <qsemaphore.h>
#include <qthreadpool.h>
#include <private/qthreadpool_p.h>
#endif

QT_BEGIN_NAMESPACE

#if QT_CONFIG(raster_64bit) || QT_CONFIG(raster_fp)
Q_STATIC_LOGGING_CATEGORY(lcQtGuiDrawHelper, "qt.gui.drawhelper")
#endif

#define MASK(src, a) src = BYTE_MUL(src, a)

/*
  constants and structures
*/

constexpr int fixed_scale = 1 << 16;
constexpr int half_point = 1 << 15;

template <QPixelLayout::BPP bpp> static
inline uint QT_FASTCALL fetch1Pixel(const uchar *, int)
{
    Q_UNREACHABLE_RETURN(0);
}

template <>
inline uint QT_FASTCALL fetch1Pixel<QPixelLayout::BPP1LSB>(const uchar *src, int index)
{
    return (src[index >> 3] >> (index & 7)) & 1;
}

template <>
inline uint QT_FASTCALL fetch1Pixel<QPixelLayout::BPP1MSB>(const uchar *src, int index)
{
    return (src[index >> 3] >> (~index & 7)) & 1;
}

template <>
inline uint QT_FASTCALL fetch1Pixel<QPixelLayout::BPP8>(const uchar *src, int index)
{
    return src[index];
}

template <>
inline uint QT_FASTCALL fetch1Pixel<QPixelLayout::BPP16>(const uchar *src, int index)
{
    return reinterpret_cast<const quint16 *>(src)[index];
}

template <>
inline uint QT_FASTCALL fetch1Pixel<QPixelLayout::BPP24>(const uchar *src, int index)
{
    return reinterpret_cast<const quint24 *>(src)[index];
}

template <>
inline uint QT_FASTCALL fetch1Pixel<QPixelLayout::BPP32>(const uchar *src, int index)
{
    return reinterpret_cast<const uint *>(src)[index];
}

template <>
inline uint QT_FASTCALL fetch1Pixel<QPixelLayout::BPP64>(const uchar *src, int index)
{
    // We have to do the conversion in fetch to fit into a 32bit uint
    QRgba64 c = reinterpret_cast<const QRgba64 *>(src)[index];
    return c.toArgb32();
}

template <>
inline uint QT_FASTCALL fetch1Pixel<QPixelLayout::BPP16FPx4>(const uchar *src, int index)
{
    // We have to do the conversion in fetch to fit into a 32bit uint
    QRgbaFloat16 c = reinterpret_cast<const QRgbaFloat16 *>(src)[index];
    return c.toArgb32();
}

template <>
inline uint QT_FASTCALL fetch1Pixel<QPixelLayout::BPP32FPx4>(const uchar *src, int index)
{
    // We have to do the conversion in fetch to fit into a 32bit uint
    QRgbaFloat32 c = reinterpret_cast<const QRgbaFloat32 *>(src)[index];
    return c.toArgb32();
}

typedef uint (QT_FASTCALL *Fetch1PixelFunc)(const uchar *src, int index);

constexpr Fetch1PixelFunc fetch1PixelTable[QPixelLayout::BPPCount] = {
    nullptr, // BPPNone
    fetch1Pixel<QPixelLayout::BPP1MSB>,
    fetch1Pixel<QPixelLayout::BPP1LSB>,
    fetch1Pixel<QPixelLayout::BPP8>,
    fetch1Pixel<QPixelLayout::BPP16>,
    fetch1Pixel<QPixelLayout::BPP24>,
    fetch1Pixel<QPixelLayout::BPP32>,
    fetch1Pixel<QPixelLayout::BPP64>,
    fetch1Pixel<QPixelLayout::BPP16FPx4>,
    fetch1Pixel<QPixelLayout::BPP32FPx4>,
};

#if QT_CONFIG(raster_64bit)
static void QT_FASTCALL convertRGBA64ToRGBA64PM(QRgba64 *buffer, int count)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = buffer[i].premultiplied();
}

static void QT_FASTCALL convertRGBA64PMToRGBA64PM(QRgba64 *, int)
{
}

static void QT_FASTCALL convertRGBA16FToRGBA64PM(QRgba64 *buffer, int count)
{
    const QRgbaFloat16 *in = reinterpret_cast<const QRgbaFloat16 *>(buffer);
    for (int i = 0; i < count; ++i) {
        QRgbaFloat16 c = in[i];
        buffer[i] = QRgba64::fromRgba64(c.red16(), c.green16(), c.blue16(), c.alpha16()).premultiplied();
    }
}

static void QT_FASTCALL convertRGBA16FPMToRGBA64PM(QRgba64 *buffer, int count)
{
    const QRgbaFloat16 *in = reinterpret_cast<const QRgbaFloat16 *>(buffer);
    for (int i = 0; i < count; ++i) {
        QRgbaFloat16 c = in[i];
        buffer[i] = QRgba64::fromRgba64(c.red16(), c.green16(), c.blue16(), c.alpha16());
    }
}

static void QT_FASTCALL convertRGBA32FToRGBA64PM(QRgba64 *buffer, int count)
{
    const QRgbaFloat32 *in = reinterpret_cast<const QRgbaFloat32 *>(buffer);
    for (int i = 0; i < count; ++i) {
        QRgbaFloat32 c = in[i];
        buffer[i] = QRgba64::fromRgba64(c.red16(), c.green16(), c.blue16(), c.alpha16()).premultiplied();
    }
}

static void QT_FASTCALL convertRGBA32FPMToRGBA64PM(QRgba64 *buffer, int count)
{
    const QRgbaFloat32 *in = reinterpret_cast<const QRgbaFloat32 *>(buffer);
    for (int i = 0; i < count; ++i) {
        QRgbaFloat32 c = in[i];
        buffer[i] = QRgba64::fromRgba64(c.red16(), c.green16(), c.blue16(), c.alpha16());
    }
}

static Convert64Func convert64ToRGBA64PM[] = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    convertRGBA64PMToRGBA64PM,
    convertRGBA64ToRGBA64PM,
    convertRGBA64PMToRGBA64PM,
    nullptr,
    nullptr,
    convertRGBA16FPMToRGBA64PM,
    convertRGBA16FToRGBA64PM,
    convertRGBA16FPMToRGBA64PM,
    convertRGBA32FPMToRGBA64PM,
    convertRGBA32FToRGBA64PM,
    convertRGBA32FPMToRGBA64PM,
    nullptr,
};

static_assert(std::size(convert64ToRGBA64PM) == QImage::NImageFormats);
#endif

#if QT_CONFIG(raster_fp)
static void QT_FASTCALL convertRGBA64PMToRGBA32F(QRgbaFloat32 *buffer, const quint64 *src, int count)
{
    const auto *in = reinterpret_cast<const QRgba64 *>(src);
    for (int i = 0; i < count; ++i) {
        auto c = in[i];
        buffer[i] = QRgbaFloat32::fromRgba64(c.red(), c.green(), c.blue(), c.alpha()).premultiplied();
    }
}

static void QT_FASTCALL convertRGBA64ToRGBA32F(QRgbaFloat32 *buffer, const quint64 *src, int count)
{
    const auto *in = reinterpret_cast<const QRgba64 *>(src);
    for (int i = 0; i < count; ++i) {
        auto c = in[i];
        buffer[i] = QRgbaFloat32::fromRgba64(c.red(), c.green(), c.blue(), c.alpha());
    }
}

static void QT_FASTCALL convertRGBA16FPMToRGBA32F(QRgbaFloat32 *buffer, const quint64 *src, int count)
{
    qFloatFromFloat16((float *)buffer, (const qfloat16 *)src, count * 4);
    for (int i = 0; i < count; ++i)
        buffer[i] = buffer[i].premultiplied();
}

static void QT_FASTCALL convertRGBA16FToRGBA32F(QRgbaFloat32 *buffer, const quint64 *src, int count)
{
    qFloatFromFloat16((float *)buffer, (const qfloat16 *)src, count * 4);
}

static Convert64ToFPFunc convert64ToRGBA32F[] = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    convertRGBA64ToRGBA32F,
    convertRGBA64PMToRGBA32F,
    convertRGBA64ToRGBA32F,
    nullptr,
    nullptr,
    convertRGBA16FToRGBA32F,
    convertRGBA16FPMToRGBA32F,
    convertRGBA16FToRGBA32F,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

static_assert(std::size(convert64ToRGBA32F) == QImage::NImageFormats);

static void convertRGBA32FToRGBA32FPM(QRgbaFloat32 *buffer, int count)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = buffer[i].premultiplied();
}

static void convertRGBA32FToRGBA32F(QRgbaFloat32 *, int)
{
}

#endif

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

static DestFetchProc destFetchProc[] =
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
    destFetch,          // Format_RGBX16FPx4
    destFetch,          // Format_RGBA16FPx4
    destFetch,          // Format_RGBA16FPx4_Premultiplied
    destFetch,          // Format_RGBX32FPx4
    destFetch,          // Format_RGBA32FPx4
    destFetch,          // Format_RGBA32FPx4_Premultiplied
    destFetch,          // Format_CMYK8888
};

static_assert(std::size(destFetchProc) == QImage::NImageFormats);

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

static DestFetchProc64 destFetchProc64[] =
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
    destFetch64,        // Format_RGBX16FPx4
    destFetch64,        // Format_RGBA16FPx4
    destFetch64,        // Format_RGBA16FPx4_Premultiplied
    destFetch64,        // Format_RGBX32FPx4
    destFetch64,        // Format_RGBA32FPx4
    destFetch64,        // Format_RGBA32FPx4_Premultiplied
    destFetch64,        // Format_CMYK8888
};

static_assert(std::size(destFetchProc64) == QImage::NImageFormats);
#endif

#if QT_CONFIG(raster_fp)
static QRgbaFloat32 *QT_FASTCALL destFetchFP(QRgbaFloat32 *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    return const_cast<QRgbaFloat32 *>(qFetchToRGBA32F[rasterBuffer->format](buffer, rasterBuffer->scanLine(y), x, length, nullptr, nullptr));
}

static QRgbaFloat32 *QT_FASTCALL destFetchRGBFP(QRgbaFloat32 *, QRasterBuffer *rasterBuffer, int x, int y, int)
{
    return reinterpret_cast<QRgbaFloat32 *>(rasterBuffer->scanLine(y)) + x;
}

static QRgbaFloat32 *QT_FASTCALL destFetchFPUndefined(QRgbaFloat32 *buffer, QRasterBuffer *, int, int, int)
{
    return buffer;
}
static DestFetchProcFP destFetchProcFP[] =
{
    nullptr,            // Format_Invalid
    nullptr,            // Format_Mono,
    nullptr,            // Format_MonoLSB
    nullptr,            // Format_Indexed8
    destFetchFP,        // Format_RGB32
    destFetchFP,        // Format_ARGB32,
    destFetchFP,        // Format_ARGB32_Premultiplied
    destFetchFP,        // Format_RGB16
    destFetchFP,        // Format_ARGB8565_Premultiplied
    destFetchFP,        // Format_RGB666
    destFetchFP,        // Format_ARGB6666_Premultiplied
    destFetchFP,        // Format_RGB555
    destFetchFP,        // Format_ARGB8555_Premultiplied
    destFetchFP,        // Format_RGB888
    destFetchFP,        // Format_RGB444
    destFetchFP,        // Format_ARGB4444_Premultiplied
    destFetchFP,        // Format_RGBX8888
    destFetchFP,        // Format_RGBA8888
    destFetchFP,        // Format_RGBA8888_Premultiplied
    destFetchFP,        // Format_BGR30
    destFetchFP,        // Format_A2BGR30_Premultiplied
    destFetchFP,        // Format_RGB30
    destFetchFP,        // Format_A2RGB30_Premultiplied
    destFetchFP,        // Format_Alpha8
    destFetchFP,        // Format_Grayscale8
    destFetchFP,        // Format_RGBX64
    destFetchFP,        // Format_RGBA64
    destFetchFP,        // Format_RGBA64_Premultiplied
    destFetchFP,        // Format_Grayscale16
    destFetchFP,        // Format_BGR888
    destFetchFP,        // Format_RGBX16FPx4
    destFetchFP,        // Format_RGBA16FPx4
    destFetchFP,        // Format_RGBA16FPx4_Premultiplied
    destFetchRGBFP,     // Format_RGBX32FPx4
    destFetchFP,        // Format_RGBA32FPx4
    destFetchRGBFP,     // Format_RGBA32FPx4_Premultiplied
    destFetchFP,        // Format_CMYK8888
};

static_assert(std::size(destFetchProcFP) == QImage::NImageFormats);
#endif

/*
   Returns the color in the mono destination color table
   that is the "nearest" to /color/.
*/
static inline QRgb findNearestColor(QRgb color, QRasterBuffer *rbuf)
{
    const QRgb color_0 = rbuf->destColor0;
    const QRgb color_1 = rbuf->destColor1;

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

static void QT_FASTCALL destStoreGray8(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uchar *data = rasterBuffer->scanLine(y) + x;

    bool failed = false;
    for (int k = 0; k < length; ++k) {
        if (!qIsGray(buffer[k])) {
            failed = true;
            break;
        }
        data[k] = qRed(buffer[k]);
    }
    if (failed) { // Non-gray colors
        QColorSpace fromCS = rasterBuffer->colorSpace.isValid() ? rasterBuffer->colorSpace : QColorSpace::SRgb;
        QColorTransform tf = QColorSpacePrivate::get(fromCS)->transformationToXYZ();
        QColorTransformPrivate *tfd = QColorTransformPrivate::get(tf);

        tfd->apply(data, buffer, length, QColorTransformPrivate::InputPremultiplied);
    }
}

static void QT_FASTCALL destStoreGray16(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    quint16 *data = reinterpret_cast<quint16 *>(rasterBuffer->scanLine(y)) + x;

    bool failed = false;
    for (int k = 0; k < length; ++k) {
        if (!qIsGray(buffer[k])) {
            failed = true;
            break;
        }
        data[k] = qRed(buffer[k]) * 257;
    }
    if (failed) { // Non-gray colors
        QColorSpace fromCS = rasterBuffer->colorSpace.isValid() ? rasterBuffer->colorSpace : QColorSpace::SRgb;
        QColorTransform tf = QColorSpacePrivate::get(fromCS)->transformationToXYZ();
        QColorTransformPrivate *tfd = QColorTransformPrivate::get(tf);

        Q_DECL_UNINITIALIZED QRgba64 tmp_line[BufferSize];
        for (int k = 0; k < length; ++k)
            tmp_line[k] = QRgba64::fromArgb32(buffer[k]);
        tfd->apply(data, tmp_line, length, QColorTransformPrivate::InputPremultiplied);
    }
}

static DestStoreProc destStoreProc[] =
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
    destStoreGray8,     // Format_Grayscale8
    destStore,          // Format_RGBX64
    destStore,          // Format_RGBA64
    destStore,          // Format_RGBA64_Premultiplied
    destStoreGray16,    // Format_Grayscale16
    destStore,          // Format_BGR888
    destStore,          // Format_RGBX16FPx4
    destStore,          // Format_RGBA16FPx4
    destStore,          // Format_RGBA16FPx4_Premultiplied
    destStore,          // Format_RGBX32FPx4
    destStore,          // Format_RGBA32FPx4
    destStore,          // Format_RGBA32FPx4_Premultiplied
    destStore,          // Format_CMYK8888
};

static_assert(std::size(destStoreProc) == QImage::NImageFormats);

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

static void QT_FASTCALL destStore64Gray8(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length)
{
    uchar *data = rasterBuffer->scanLine(y) + x;

    bool failed = false;
    for (int k = 0; k < length; ++k) {
        if (buffer[k].red() != buffer[k].green() || buffer[k].red() != buffer[k].blue()) {
            failed = true;
            break;
        }
        data[k] = buffer[k].red8();
    }
    if (failed) { // Non-gray colors
        QColorSpace fromCS = rasterBuffer->colorSpace.isValid() ? rasterBuffer->colorSpace : QColorSpace::SRgb;
        QColorTransform tf = QColorSpacePrivate::get(fromCS)->transformationToXYZ();
        QColorTransformPrivate *tfd = QColorTransformPrivate::get(tf);

        Q_DECL_UNINITIALIZED quint16 gray_line[BufferSize];
        tfd->apply(gray_line, buffer, length, QColorTransformPrivate::InputPremultiplied);
        for (int k = 0; k < length; ++k)
            data[k] = qt_div_257(gray_line[k]);
    }
}

static void QT_FASTCALL destStore64Gray16(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length)
{
    quint16 *data = reinterpret_cast<quint16 *>(rasterBuffer->scanLine(y)) + x;

    bool failed = false;
    for (int k = 0; k < length; ++k) {
        if (buffer[k].red() != buffer[k].green() || buffer[k].red() != buffer[k].blue()) {
            failed = true;
            break;
        }
        data[k] = buffer[k].red();
    }
    if (failed) { // Non-gray colors
        QColorSpace fromCS = rasterBuffer->colorSpace.isValid() ? rasterBuffer->colorSpace : QColorSpace::SRgb;
        QColorTransform tf = QColorSpacePrivate::get(fromCS)->transformationToXYZ();
        QColorTransformPrivate *tfd = QColorTransformPrivate::get(tf);
        tfd->apply(data, buffer, length, QColorTransformPrivate::InputPremultiplied);
    }
}

static DestStoreProc64 destStoreProc64[] =
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
    destStore64Gray8,   // Format_Grayscale8
    nullptr,            // Format_RGBX64
    destStore64RGBA64,  // Format_RGBA64
    nullptr,            // Format_RGBA64_Premultiplied
    destStore64Gray16,  // Format_Grayscale16
    destStore64,        // Format_BGR888
    destStore64,        // Format_RGBX16FPx4
    destStore64,        // Format_RGBA16FPx4
    destStore64,        // Format_RGBA16FPx4_Premultiplied
    destStore64,        // Format_RGBX32FPx4
    destStore64,        // Format_RGBA32FPx4
    destStore64,        // Format_RGBA32FPx4_Premultiplied
    destStore64,        // Format_CMYK8888
};

static_assert(std::size(destStoreProc64) == QImage::NImageFormats);
#endif

#if QT_CONFIG(raster_fp)
static void QT_FASTCALL destStoreFP(QRasterBuffer *rasterBuffer, int x, int y, const QRgbaFloat32 *buffer, int length)
{
    auto store = qStoreFromRGBA32F[rasterBuffer->format];
    uchar *dest = rasterBuffer->scanLine(y);
    store(dest, buffer, x, length, nullptr, nullptr);
}
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

#if QT_CONFIG(raster_fp)
static const QRgbaFloat32 *QT_FASTCALL fetchUntransformedFP(QRgbaFloat32 *buffer, const Operator *,
                                                        const QSpanData *data, int y, int x, int length)
{
    const auto fetch = qFetchToRGBA32F[data->texture.format];
    return fetch(buffer, data->texture.scanLine(y), x, length, data->texture.colorTable, nullptr);
}
#endif

template<TextureBlendType blendType>
inline void fetchTransformed_pixelBounds(int max, int l1, int l2, int &v)
{
    static_assert(blendType == BlendTransformed || blendType == BlendTransformedTiled);
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
    static_assert(blendType == BlendTransformed || blendType == BlendTransformedTiled);
    const QTextureData &image = data->texture;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    constexpr bool useFetch = (bpp < QPixelLayout::BPP32) && sizeof(T) == sizeof(uint);
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    if (!useFetch)
        Q_ASSERT(layout->bpp == bpp || (layout->bpp == QPixelLayout::BPP16FPx4 && bpp == QPixelLayout::BPP64));
    // When templated 'fetch' should be inlined at compile time:
    const Fetch1PixelFunc fetch1 = (bpp == QPixelLayout::BPPNone) ? fetch1PixelTable[layout->bpp] : Fetch1PixelFunc(fetch1Pixel<bpp>);

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
                    if constexpr (useFetch)
                        buffer[i] = fetch1(src, x1);
                    else
                        buffer[i] = reinterpret_cast<const T*>(src)[x1];
                    fx += fdx;
                }

                for (; i < fastLen; ++i) {
                    int px = (fx >> 16);
                    if constexpr (useFetch)
                        buffer[i] = fetch1(src, px);
                    else
                        buffer[i] = reinterpret_cast<const T*>(src)[px];
                    fx += fdx;
                }
            }

            for (; i < length; ++i) {
                int px = (fx >> 16);
                fetchTransformed_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, px);
                if constexpr (useFetch)
                    buffer[i] = fetch1(src, px);
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
                    if constexpr (useFetch)
                        buffer[i] = fetch1(image.scanLine(y1), x1);
                    else
                        buffer[i] = reinterpret_cast<const T*>(image.scanLine(y1))[x1];
                    fx += fdx;
                    fy += fdy;
                }

                for (; i < fastLen; ++i) {
                    int px = (fx >> 16);
                    int py = (fy >> 16);
                    if constexpr (useFetch)
                        buffer[i] = fetch1(image.scanLine(py), px);
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
                if constexpr (useFetch)
                    buffer[i] = fetch1(image.scanLine(py), px);
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
            if constexpr (useFetch)
                *b = fetch1(image.scanLine(py), px);
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
    static_assert(blendType == BlendTransformed || blendType == BlendTransformedTiled);
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
    if (layout->bpp < QPixelLayout::BPP64) {
        Q_DECL_UNINITIALIZED uint buffer32[BufferSize];
        Q_ASSERT(length <= BufferSize);
        if (layout->bpp == QPixelLayout::BPP32)
            fetchTransformed_fetcher<blendType, QPixelLayout::BPP32, uint>(buffer32, data, y, x, length);
        else
            fetchTransformed_fetcher<blendType, QPixelLayout::BPPNone, uint>(buffer32, data, y, x, length);
        return layout->convertToRGBA64PM(buffer, buffer32, length, data->texture.colorTable, nullptr);
    }

    fetchTransformed_fetcher<blendType, QPixelLayout::BPP64, quint64>(reinterpret_cast<quint64*>(buffer), data, y, x, length);
    if (auto convert = convert64ToRGBA64PM[data->texture.format])
        convert(buffer, length);
    return buffer;
}
#endif

#if QT_CONFIG(raster_fp)
template<TextureBlendType blendType>  /* either BlendTransformed or BlendTransformedTiled */
static const QRgbaFloat32 *QT_FASTCALL fetchTransformedFP(QRgbaFloat32 *buffer, const Operator *, const QSpanData *data,
                                                      int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    if (layout->bpp < QPixelLayout::BPP64) {
        Q_DECL_UNINITIALIZED uint buffer32[BufferSize];
        Q_ASSERT(length <= BufferSize);
        if (layout->bpp == QPixelLayout::BPP32)
            fetchTransformed_fetcher<blendType, QPixelLayout::BPP32, uint>(buffer32, data, y, x, length);
        else
            fetchTransformed_fetcher<blendType, QPixelLayout::BPPNone, uint>(buffer32, data, y, x, length);
        qConvertToRGBA32F[data->texture.format](buffer, buffer32, length, data->texture.colorTable, nullptr);
    } else if (layout->bpp < QPixelLayout::BPP32FPx4) {
        Q_DECL_UNINITIALIZED quint64 buffer64[BufferSize];
        fetchTransformed_fetcher<blendType, QPixelLayout::BPP64, quint64>(buffer64, data, y, x, length);
        convert64ToRGBA32F[data->texture.format](buffer, buffer64, length);
    } else {
        fetchTransformed_fetcher<blendType, QPixelLayout::BPP32FPx4, QRgbaFloat32>(buffer, data, y, x, length);
        if (data->texture.format == QImage::Format_RGBA32FPx4)
            convertRGBA32FToRGBA32FPM(buffer, length);
        return buffer;
    }
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

#if defined(__loongarch_sx)
static inline void interpolate_4_pixels_16_lsx(__m128i tl, __m128i tr, __m128i bl, __m128i br,
                                               __m128i distx, __m128i disty, uint *b)
{
    const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);
    const __m128i v_256 = __lsx_vreplgr2vr_h(256);
    const __m128i dxdy = __lsx_vmul_h(distx, disty);
    const __m128i distx_ = __lsx_vslli_h(distx, 4);
    const __m128i disty_ = __lsx_vslli_h(disty, 4);
    const __m128i idxidy =  __lsx_vadd_h(dxdy, __lsx_vsub_h(v_256, __lsx_vadd_h(distx_, disty_)));
    const __m128i dxidy = __lsx_vsub_h(distx_, dxdy);
    const __m128i idxdy = __lsx_vsub_h(disty_,dxdy);

    __m128i tlAG = __lsx_vsrli_h(tl, 8);
    __m128i tlRB = __lsx_vand_v(tl, colorMask);
    __m128i trAG = __lsx_vsrli_h(tr, 8);
    __m128i trRB = __lsx_vand_v(tr, colorMask);
    __m128i blAG = __lsx_vsrli_h(bl, 8);
    __m128i blRB = __lsx_vand_v(bl, colorMask);
    __m128i brAG = __lsx_vsrli_h(br, 8);
    __m128i brRB = __lsx_vand_v(br, colorMask);

    tlAG = __lsx_vmul_h(tlAG, idxidy);
    tlRB = __lsx_vmul_h(tlRB, idxidy);
    trAG = __lsx_vmul_h(trAG, dxidy);
    trRB = __lsx_vmul_h(trRB, dxidy);
    blAG = __lsx_vmul_h(blAG, idxdy);
    blRB = __lsx_vmul_h(blRB, idxdy);
    brAG = __lsx_vmul_h(brAG, dxdy);
    brRB = __lsx_vmul_h(brRB, dxdy);

    __m128i rAG =__lsx_vadd_h(__lsx_vadd_h(tlAG, trAG), __lsx_vadd_h(blAG, brAG));
    __m128i rRB =__lsx_vadd_h(__lsx_vadd_h(tlRB, trRB), __lsx_vadd_h(blRB, brRB));
    rAG = __lsx_vandn_v(colorMask, rAG);
    rRB = __lsx_vsrli_h(rRB, 8);
    __lsx_vst(__lsx_vor_v(rAG, rRB), b, 0);
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

    Q_DECL_UNINITIALIZED IntermediateBuffer intermediate;
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
#elif defined(__loongarch_sx)
        const __m128i disty_ = __lsx_vreplgr2vr_h(disty);
        const __m128i idisty_ = __lsx_vreplgr2vr_h(idisty);
        const __m128i colorMask = __lsx_vreplgr2vr_w(0x00ff00ff);

        lim -= 3;
        for (; f < lim; x += 4, f += 4) {
            // Load 4 pixels from s1, and split the alpha-green and red-blue component
            __m128i top = __lsx_vld((const __m128i*)((const uint *)(s1)+x), 0);
            __m128i topAG = __lsx_vsrli_h(top, 8);
            __m128i topRB = __lsx_vand_v(top, colorMask);
            // Multiplies each color component by idisty
            topAG = __lsx_vmul_h(topAG, idisty_);
            topRB = __lsx_vmul_h(topRB, idisty_);

            // Same for the s2 vector
            __m128i bottom = __lsx_vld((const __m128i*)((const uint *)(s2)+x), 0);
            __m128i bottomAG = __lsx_vsrli_h(bottom, 8);
            __m128i bottomRB = __lsx_vand_v(bottom, colorMask);
            bottomAG = __lsx_vmul_h(bottomAG, disty_);
            bottomRB = __lsx_vmul_h(bottomRB, disty_);

            // Add the values, and shift to only keep 8 significant bits per colors
            __m128i rAG = __lsx_vadd_h(topAG, bottomAG);
            rAG = __lsx_vsrli_h(rAG, 8);
            __lsx_vst(rAG, (__m128i*)(&intermediate.buffer_ag[f]), 0);
            __m128i rRB = __lsx_vadd_h(topRB, bottomRB);
            rRB = __lsx_vsrli_h(rRB, 8);
            __lsx_vst(rRB, (__m128i*)(&intermediate.buffer_rb[f]), 0);
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

         // Pre-initialize to work-around code-analysis warnings/crashes in MSVC:
        uint32x4x2_t v_top = {};
        uint32x4x2_t v_bot = {};
        while (b < boundedEnd - 3) {
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
#elif defined (__loongarch_sx)
        const __m128i shuffleMask = (__m128i)(v8i16){0, 0, 2, 2, 4, 4, 6, 6};
        const __m128i v_disty = __lsx_vreplgr2vr_h(disty4);
        const __m128i v_fdx = __lsx_vreplgr2vr_w(fdx*4);
        const __m128i v_fx_r = __lsx_vreplgr2vr_w(0x8);
        __m128i v_fx = (__m128i)(v4i32){fx, fx + fdx, fx + fdx + fdx, fx + fdx + fdx + fdx};

        while (b < boundedEnd - 3) {
            __m128i offset = __lsx_vsrli_w(v_fx, 16);
            const int offset0 = __lsx_vpickve2gr_w(offset, 0);
            const int offset1 = __lsx_vpickve2gr_w(offset, 1);
            const int offset2 = __lsx_vpickve2gr_w(offset, 2);
            const int offset3 = __lsx_vpickve2gr_w(offset, 3);
            const __m128i tl = (__m128i)(v4u32){s1[offset0], s1[offset1], s1[offset2], s1[offset3]};
            const __m128i tr = (__m128i)(v4u32){s1[offset0 + 1], s1[offset1 + 1], s1[offset2 + 1], s1[offset3 + 1]};
            const __m128i bl = (__m128i)(v4u32){s2[offset0], s2[offset1], s2[offset2], s2[offset3]};
            const __m128i br = (__m128i)(v4u32){s2[offset0 + 1], s2[offset1 + 1], s2[offset2 + 1], s2[offset3 + 1]};

            __m128i v_distx = __lsx_vsrli_h(v_fx, 8);
            v_distx = __lsx_vsrli_h(__lsx_vadd_w(v_distx, v_fx_r), 4);
            v_distx = __lsx_vshuf_h(shuffleMask, v_distx, v_distx);

            interpolate_4_pixels_16_lsx(tl, tr, bl, br, v_distx, v_disty, b);
            b += 4;
            v_fx = __lsx_vadd_w(v_fx, v_fdx);
        }
        fx = __lsx_vpickve2gr_w(v_fx, 0);
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
        const qsizetype bytesPerLine = image.bytesPerLine;

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

         // Pre-initialize to work-around code-analysis warnings/crashes in MSVC:
        uint32x4x2_t v_top = {};
        uint32x4x2_t v_bot = {};
        while (b < boundedEnd - 3) {
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
#elif defined(__loongarch_sx)
        const __m128i v_fdx = __lsx_vreplgr2vr_w(fdx*4);
        const __m128i v_fdy = __lsx_vreplgr2vr_w(fdy*4);
        const __m128i v_fxy_r = __lsx_vreplgr2vr_w(0x8);
        __m128i v_fx = (__m128i)(v4i32){fx, fx + fdx, fx + fdx + fdx, fx + fdx + fdx + fdx};
        __m128i v_fy = (__m128i)(v4i32){fy, fy + fdy, fy + fdy + fdy, fy + fdy + fdy + fdy};

        const uchar *textureData = image.imageData;
        const qsizetype bytesPerLine = image.bytesPerLine;
        const __m128i zero = __lsx_vldi(0);
        const __m128i shuffleMask = (__m128i)(v8i16){0, 0, 0, 0, 4, 5, 6, 7};
        const __m128i shuffleMask1 = (__m128i)(v8i16){0, 0, 2, 2, 4, 4, 6, 6};
        const __m128i vbpl = __lsx_vshuf_h(shuffleMask, zero, __lsx_vinsgr2vr_w(zero, bytesPerLine/4, 0));

        while (b < boundedEnd - 3) {
            const __m128i vy = __lsx_vpickev_h(zero, __lsx_vsat_w(__lsx_vsrli_w(v_fy, 16), 15));
            // 4x16bit * 4x16bit -> 4x32bit
            __m128i offset = __lsx_vilvl_h(__lsx_vmuh_h(vy, vbpl), __lsx_vmul_h(vy, vbpl));
            offset = __lsx_vadd_w(offset, __lsx_vsrli_w(v_fx, 16));
            const int offset0 = __lsx_vpickve2gr_w(offset, 0);
            const int offset1 = __lsx_vpickve2gr_w(offset, 1);
            const int offset2 = __lsx_vpickve2gr_w(offset, 2);
            const int offset3 = __lsx_vpickve2gr_w(offset, 3);
            const uint *topData = (const uint *)(textureData);
            const __m128i tl = (__m128i)(v4u32){topData[offset0], topData[offset1], topData[offset2], topData[offset3]};
            const __m128i tr = (__m128i)(v4u32){topData[offset0 + 1], topData[offset1 + 1], topData[offset2 + 1], topData[offset3 + 1]};
            const uint *bottomData = (const uint *)(textureData + bytesPerLine);
            const __m128i bl = (__m128i)(v4u32){bottomData[offset0], bottomData[offset1], bottomData[offset2], bottomData[offset3]};
            const __m128i br = (__m128i)(v4u32){bottomData[offset0 + 1], bottomData[offset1 + 1], bottomData[offset2 + 1], bottomData[offset3 + 1]};

            __m128i v_distx = __lsx_vsrli_h(v_fx, 8);
            __m128i v_disty = __lsx_vsrli_h(v_fy, 8);
            v_distx = __lsx_vsrli_h(__lsx_vadd_w(v_distx, v_fxy_r), 4);
            v_disty = __lsx_vsrli_h(__lsx_vadd_w(v_disty, v_fxy_r), 4);
            v_distx = __lsx_vshuf_h(shuffleMask1, zero, v_distx);
            v_disty = __lsx_vshuf_h(shuffleMask1, zero, v_disty);

            interpolate_4_pixels_16_lsx(tl, tr, bl, br, v_distx, v_disty, b);
            b += 4;
            v_fx = __lsx_vadd_w(v_fx, v_fdx);
            v_fy = __lsx_vadd_w(v_fy, v_fdy);
        }
        fx = __lsx_vpickve2gr_w(v_fx, 0);
        fy = __lsx_vpickve2gr_w(v_fy, 0);
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
    constexpr int tiled = (blendType == BlendTransformedBilinearTiled) ? 1 : 0;

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
    const QList<QRgb> *clut = image.colorTable;
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

    Q_DECL_UNINITIALIZED IntermediateBuffer intermediate;
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
        Q_ASSERT(layout.bpp == bpp || (layout.bpp == QPixelLayout::BPP16FPx4 && bpp == QPixelLayout::BPP64));
    const Fetch1PixelFunc fetch1 = (bpp == QPixelLayout::BPPNone) ? fetch1PixelTable[layout.bpp] : fetch1Pixel<bpp>;
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
                if constexpr (useFetch) {
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
                if constexpr (useFetch) {
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
            if constexpr (useFetch) {
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
                if constexpr (useFetch) {
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
                if constexpr (useFetch) {
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
            if constexpr (useFetch) {
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

template<TextureBlendType blendType, QPixelLayout::BPP bpp, typename T>
static void QT_FASTCALL fetchTransformedBilinear_slow_fetcher(T *buf1, T *buf2, ushort *distxs, ushort *distys,
                                                              const int len, const QTextureData &image,
                                                              qreal &fx, qreal &fy, qreal &fw,
                                                              const qreal fdx, const qreal fdy, const qreal fdw)
{
    const QPixelLayout &layout = qPixelLayouts[image.format];
    constexpr bool useFetch = (bpp < QPixelLayout::BPP32);
    if (useFetch)
        Q_ASSERT(sizeof(T) == sizeof(uint));
    else
        Q_ASSERT(layout.bpp == bpp);

    const Fetch1PixelFunc fetch1 = (bpp == QPixelLayout::BPPNone) ? fetch1PixelTable[layout.bpp] : fetch1Pixel<bpp>;

    for (int i = 0; i < len; ++i) {
        const qreal iw = fw == 0 ? 16384 : 1 / fw;
        const qreal px = fx * iw - qreal(0.5);
        const qreal py = fy * iw - qreal(0.5);

        int x1 = qFloor(px);
        int x2;
        int y1 = qFloor(py);
        int y2;

        distxs[i] = ushort((px - x1) * (1<<16));
        distys[i] = ushort((py - y1) * (1<<16));

        fetchTransformedBilinear_pixelBounds<blendType>(image.width, image.x1, image.x2 - 1, x1, x2);
        fetchTransformedBilinear_pixelBounds<blendType>(image.height, image.y1, image.y2 - 1, y1, y2);

        const uchar *s1 = image.scanLine(y1);
        const uchar *s2 = image.scanLine(y2);
        if constexpr (useFetch) {
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
        fw += fdw;
    }
}

// blendType = BlendTransformedBilinear or BlendTransformedBilinearTiled
template<TextureBlendType blendType, QPixelLayout::BPP bpp>
static const uint *QT_FASTCALL fetchTransformedBilinear(uint *buffer, const Operator *,
                                                        const QSpanData *data, int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    const QList<QRgb> *clut = data->texture.colorTable;
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

                Q_DECL_UNINITIALIZED uint buf1[BufferSize];
                Q_DECL_UNINITIALIZED uint buf2[BufferSize];
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

            Q_DECL_UNINITIALIZED uint buf1[BufferSize];
            Q_DECL_UNINITIALIZED uint buf2[BufferSize];
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
        const auto fetcher = fetchTransformedBilinear_slow_fetcher<blendType,bpp,uint>;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        Q_DECL_UNINITIALIZED uint buf1[BufferSize];
        Q_DECL_UNINITIALIZED uint buf2[BufferSize];
        uint *b = buffer;

        Q_DECL_UNINITIALIZED ushort distxs[BufferSize / 2];
        Q_DECL_UNINITIALIZED ushort distys[BufferSize / 2];

        while (length) {
            const int len = qMin(length, BufferSize / 2);
            fetcher(buf1, buf2, distxs, distys, len, data->texture, fx, fy, fw, fdx, fdy, fdw);

            layout->convertToARGB32PM(buf1, len * 2, clut);
            layout->convertToARGB32PM(buf2, len * 2, clut);

            for (int i = 0; i < len; ++i) {
                const int distx = distxs[i] >> 8;
                const int disty = distys[i] >> 8;

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
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    const auto *clut = data->texture.colorTable;
    const auto convert = layout->convertToRGBA64PM;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    Q_DECL_UNINITIALIZED uint sbuf1[BufferSize];
    Q_DECL_UNINITIALIZED uint sbuf2[BufferSize];
    alignas(8) Q_DECL_UNINITIALIZED QRgba64 buf1[BufferSize];
    alignas(8) Q_DECL_UNINITIALIZED QRgba64 buf2[BufferSize];
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
                const int len = qMin(length, BufferSize / 2);
                const int disty = (fy & 0x0000ffff);
#if defined(__SSE2__)
                const __m128i vdy = _mm_set1_epi16(disty);
                const __m128i vidy = _mm_set1_epi16(0x10000 - disty);
#endif
                fetcher(sbuf1, sbuf2, len, data->texture, fx, fy, fdx, fdy);

                convert(buf1, sbuf1, len * 2, clut, nullptr);
                if (disty)
                    convert(buf2, sbuf2, len * 2, clut, nullptr);

                for (int i = 0; i < len; ++i) {
                    const int distx = (fx & 0x0000ffff);
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
            while (length) {
                const int len = qMin(length, BufferSize / 2);

                fetcher(sbuf1, sbuf2, len, data->texture, fx, fy, fdx, fdy);

                convert(buf1, sbuf1, len * 2, clut, nullptr);
                convert(buf2, sbuf2, len * 2, clut, nullptr);

                for (int i = 0; i < len; ++i) {
                    const int distx = (fx & 0x0000ffff);
                    const int disty = (fy & 0x0000ffff);
                    b[i] = interpolate_4_pixels_rgb64(buf1 + i*2, buf2 + i*2, distx, disty);
                    fx += fdx;
                    fy += fdy;
                }

                length -= len;
                b += len;
            }
        }
    } else { // !(data->fast_matrix)
        const auto fetcher =
                (layout->bpp == QPixelLayout::BPP32)
                        ? fetchTransformedBilinear_slow_fetcher<blendType, QPixelLayout::BPP32, uint>
                        : fetchTransformedBilinear_slow_fetcher<blendType, QPixelLayout::BPPNone, uint>;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        Q_DECL_UNINITIALIZED ushort distxs[BufferSize / 2];
        Q_DECL_UNINITIALIZED ushort distys[BufferSize / 2];

        while (length) {
            const int len = qMin(length, BufferSize / 2);
            fetcher(sbuf1, sbuf2, distxs, distys, len, data->texture, fx, fy, fw, fdx, fdy, fdw);

            convert(buf1, sbuf1, len * 2, clut, nullptr);
            convert(buf2, sbuf2, len * 2, clut, nullptr);

            for (int i = 0; i < len; ++i) {
                const int distx = distxs[i];
                const int disty = distys[i];
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
    const auto convert = convert64ToRGBA64PM[data->texture.format];

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    alignas(8) Q_DECL_UNINITIALIZED QRgba64 buf1[BufferSize];
    alignas(8) Q_DECL_UNINITIALIZED QRgba64 buf2[BufferSize];
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
        const auto fetcher = fetchTransformedBilinear_slow_fetcher<blendType, QPixelLayout::BPP64, QRgba64>;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        Q_DECL_UNINITIALIZED ushort distxs[BufferSize / 2];
        Q_DECL_UNINITIALIZED ushort distys[BufferSize / 2];

        while (length) {
            const int len = qMin(length, BufferSize / 2);
            fetcher(buf1, buf2, distxs, distys, len, data->texture, fx, fy, fw, fdx, fdy, fdw);

            convert(buf1, len * 2);
            convert(buf2, len * 2);

            for (int i = 0; i < len; ++i) {
                const int distx = distxs[i];
                const int disty = distys[i];
                b[i] = interpolate_4_pixels_rgb64(buf1 + i*2, buf2 + i*2, distx, disty);
            }

            length -= len;
            b += len;
        }
    }
    return buffer;
}

template<TextureBlendType blendType>
static const QRgba64 *QT_FASTCALL fetchTransformedBilinear64_f32x4(QRgba64 *buffer, const QSpanData *data,
                                                                   int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    const auto *clut = data->texture.colorTable;
    const auto convert = layout->fetchToRGBA64PM;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    Q_DECL_UNINITIALIZED QRgbaFloat32 sbuf1[BufferSize];
    Q_DECL_UNINITIALIZED QRgbaFloat32 sbuf2[BufferSize];
    alignas(8) Q_DECL_UNINITIALIZED QRgba64 buf1[BufferSize];
    alignas(8) Q_DECL_UNINITIALIZED QRgba64 buf2[BufferSize];
    QRgba64 *b = buffer;

    if (canUseFastMatrixPath(cx, cy, length, data)) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;

        const auto fetcher = fetchTransformedBilinear_fetcher<blendType, QPixelLayout::BPP32FPx4, QRgbaFloat32>;

        const bool skipsecond = (fdy == 0) && ((fy & 0x0000ffff) == 0);
        while (length) {
            const int len = qMin(length, BufferSize / 2);

            fetcher(sbuf1, sbuf2, len, data->texture, fx, fy, fdx, fdy);

            convert(buf1, (const uchar *)sbuf1, 0, len * 2, clut, nullptr);
            if (!skipsecond)
                convert(buf2, (const uchar *)sbuf2, 0, len * 2, clut, nullptr);

            for (int i = 0; i < len; ++i) {
                const int distx = (fx & 0x0000ffff);
                const int disty = (fy & 0x0000ffff);
                b[i] = interpolate_4_pixels_rgb64(buf1 + i*2, buf2 + i*2, distx, disty);
                fx += fdx;
                fy += fdy;
            }

            length -= len;
            b += len;
        }
    } else { // !(data->fast_matrix)
        const auto fetcher = fetchTransformedBilinear_slow_fetcher<blendType, QPixelLayout::BPP32FPx4, QRgbaFloat32>;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        Q_DECL_UNINITIALIZED ushort distxs[BufferSize / 2];
        Q_DECL_UNINITIALIZED ushort distys[BufferSize / 2];

        while (length) {
            const int len = qMin(length, BufferSize / 2);
            fetcher(sbuf1, sbuf2, distxs, distys, len, data->texture, fx, fy, fw, fdx, fdy, fdw);

            convert(buf1, (const uchar *)sbuf1, 0, len * 2, clut, nullptr);
            convert(buf2, (const uchar *)sbuf2, 0, len * 2, clut, nullptr);

            for (int i = 0; i < len; ++i) {
                const int distx = distxs[i];
                const int disty = distys[i];
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
    switch (qPixelLayouts[data->texture.format].bpp) {
    case QPixelLayout::BPP64:
    case QPixelLayout::BPP16FPx4:
        return fetchTransformedBilinear64_uint64<blendType>(buffer, data, y, x, length);
    case QPixelLayout::BPP32FPx4:
        return fetchTransformedBilinear64_f32x4<blendType>(buffer, data, y, x, length);
    default:
        return fetchTransformedBilinear64_uint32<blendType>(buffer, data, y, x, length);
    }
}
#endif

#if QT_CONFIG(raster_fp)
static void interpolate_simple_rgba32f(QRgbaFloat32 *b, const QRgbaFloat32 *buf1, const QRgbaFloat32 *buf2, int len,
                                       int &fx, int fdx,
                                       int &fy, int fdy)
{
    for (int i = 0; i < len; ++i) {
        const int distx = (fx & 0x0000ffff);
        const int disty = (fy & 0x0000ffff);
        b[i] = interpolate_4_pixels_rgba32f(buf1 + i*2, buf2 + i*2, distx, disty);
        fx += fdx;
        fy += fdy;
    }
}

static void interpolate_perspective_rgba32f(QRgbaFloat32 *b, const QRgbaFloat32 *buf1, const QRgbaFloat32 *buf2, int len,
                                            unsigned short *distxs,
                                            unsigned short *distys)
{
    for (int i = 0; i < len; ++i) {
        const int dx = distxs[i];
        const int dy = distys[i];
        b[i] = interpolate_4_pixels_rgba32f(buf1 + i*2, buf2 + i*2, dx, dy);
    }
}

template<TextureBlendType blendType>
static const QRgbaFloat32 *QT_FASTCALL fetchTransformedBilinearFP_uint32(QRgbaFloat32 *buffer, const QSpanData *data,
                                                                     int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    const auto *clut = data->texture.colorTable;
    const auto convert = qConvertToRGBA32F[data->texture.format];

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    Q_DECL_UNINITIALIZED uint sbuf1[BufferSize];
    Q_DECL_UNINITIALIZED uint sbuf2[BufferSize];
    Q_DECL_UNINITIALIZED QRgbaFloat32 buf1[BufferSize];
    Q_DECL_UNINITIALIZED QRgbaFloat32 buf2[BufferSize];
    QRgbaFloat32 *b = buffer;

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

        const bool skipsecond = (fdy == 0) && ((fy & 0x0000ffff) == 0);
        while (length) {
            const int len = qMin(length, BufferSize / 2);
            fetcher(sbuf1, sbuf2, len, data->texture, fx, fy, fdx, fdy);

            convert(buf1, sbuf1, len * 2, clut, nullptr);
            if (!skipsecond)
                convert(buf2, sbuf2, len * 2, clut, nullptr);

            interpolate_simple_rgba32f(b, buf1, buf2, len, fx, fdx, fy, fdy);

            length -= len;
            b += len;
        }
    } else { // !(data->fast_matrix)
        const auto fetcher =
                (layout->bpp == QPixelLayout::BPP32)
                        ? fetchTransformedBilinear_slow_fetcher<blendType, QPixelLayout::BPP32, uint>
                        : fetchTransformedBilinear_slow_fetcher<blendType, QPixelLayout::BPPNone, uint>;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;
        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;
        Q_DECL_UNINITIALIZED ushort distxs[BufferSize / 2];
        Q_DECL_UNINITIALIZED ushort distys[BufferSize / 2];

        while (length) {
            const int len = qMin(length, BufferSize / 2);
            fetcher(sbuf1, sbuf2, distxs, distys, len, data->texture, fx, fy, fw, fdx, fdy, fdw);

            convert(buf1, sbuf1, len * 2, clut, nullptr);
            convert(buf2, sbuf2, len * 2, clut, nullptr);

            interpolate_perspective_rgba32f(b, buf1, buf2, len, distxs, distys);

            length -= len;
            b += len;
        }
    }
    return buffer;
}

template<TextureBlendType blendType>
static const QRgbaFloat32 *QT_FASTCALL fetchTransformedBilinearFP_uint64(QRgbaFloat32 *buffer, const QSpanData *data,
                                                                     int y, int x, int length)
{
    const auto convert = convert64ToRGBA32F[data->texture.format];

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    Q_DECL_UNINITIALIZED quint64 sbuf1[BufferSize] ;
    Q_DECL_UNINITIALIZED quint64 sbuf2[BufferSize];
    Q_DECL_UNINITIALIZED QRgbaFloat32 buf1[BufferSize];
    Q_DECL_UNINITIALIZED QRgbaFloat32 buf2[BufferSize];
    QRgbaFloat32 *b = buffer;

    if (canUseFastMatrixPath(cx, cy, length, data)) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;
        const auto fetcher = fetchTransformedBilinear_fetcher<blendType, QPixelLayout::BPP64, quint64>;

        const bool skipsecond = (fdy == 0) && ((fy & 0x0000ffff) == 0);
        while (length) {
            const int len = qMin(length, BufferSize / 2);
            fetcher(sbuf1, sbuf2, len, data->texture, fx, fy, fdx, fdy);

            convert(buf1, sbuf1, len * 2);
            if (!skipsecond)
                convert(buf2, sbuf2, len * 2);

            interpolate_simple_rgba32f(b, buf1, buf2, len, fx, fdx, fy, fdy);

            length -= len;
            b += len;
        }
    } else { // !(data->fast_matrix)
        const auto fetcher = fetchTransformedBilinear_slow_fetcher<blendType, QPixelLayout::BPP64, quint64>;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        Q_DECL_UNINITIALIZED ushort distxs[BufferSize / 2];
        Q_DECL_UNINITIALIZED ushort distys[BufferSize / 2];

        while (length) {
            const int len = qMin(length, BufferSize / 2);
            fetcher(sbuf1, sbuf2, distxs, distys, len, data->texture, fx, fy, fw, fdx, fdy, fdw);

            convert(buf1, sbuf1, len * 2);
            convert(buf2, sbuf2, len * 2);

            interpolate_perspective_rgba32f(b, buf1, buf2, len, distxs, distys);

            length -= len;
            b += len;
        }
    }
    return buffer;
}

template<TextureBlendType blendType>
static const QRgbaFloat32 *QT_FASTCALL fetchTransformedBilinearFP(QRgbaFloat32 *buffer, const QSpanData *data,
                                                              int y, int x, int length)
{
    const auto convert = data->rasterBuffer->format == QImage::Format_RGBA32FPx4 ? convertRGBA32FToRGBA32FPM
                                                                                 : convertRGBA32FToRGBA32F;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    Q_DECL_UNINITIALIZED QRgbaFloat32 buf1[BufferSize];
    Q_DECL_UNINITIALIZED QRgbaFloat32 buf2[BufferSize];
    QRgbaFloat32 *b = buffer;

    if (canUseFastMatrixPath(cx, cy, length, data)) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;
        const auto fetcher = fetchTransformedBilinear_fetcher<blendType, QPixelLayout::BPP32FPx4, QRgbaFloat32>;

        const bool skipsecond = (fdy == 0) && ((fy & 0x0000ffff) == 0);
        while (length) {
            const int len = qMin(length, BufferSize / 2);
            fetcher(buf1, buf2, len, data->texture, fx, fy, fdx, fdy);

            convert(buf1, len * 2);
            if (!skipsecond)
                convert(buf2, len * 2);

            interpolate_simple_rgba32f(b, buf1, buf2, len, fx, fdx, fy, fdy);

            length -= len;
            b += len;
        }
    } else { // !(data->fast_matrix)
        const auto fetcher = fetchTransformedBilinear_slow_fetcher<blendType, QPixelLayout::BPP32FPx4, QRgbaFloat32>;

        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        Q_DECL_UNINITIALIZED ushort distxs[BufferSize / 2];
        Q_DECL_UNINITIALIZED ushort distys[BufferSize / 2];

        while (length) {
            const int len = qMin(length, BufferSize / 2);
            fetcher(buf1, buf2, distxs, distys, len, data->texture, fx, fy, fw, fdx, fdy, fdw);

            convert(buf1, len * 2);
            convert(buf2, len * 2);

            interpolate_perspective_rgba32f(b, buf1, buf2, len, distxs, distys);

            length -= len;
            b += len;
        }
    }
    return buffer;
}

template<TextureBlendType blendType>
static const QRgbaFloat32 *QT_FASTCALL fetchTransformedBilinearFP(QRgbaFloat32 *buffer, const Operator *,
                                                              const QSpanData *data, int y, int x, int length)
{
    switch (qPixelLayouts[data->texture.format].bpp) {
    case QPixelLayout::BPP64:
    case QPixelLayout::BPP16FPx4:
        return fetchTransformedBilinearFP_uint64<blendType>(buffer, data, y, x, length);
    case QPixelLayout::BPP32FPx4:
        return fetchTransformedBilinearFP<blendType>(buffer, data, y, x, length);
    default:
        return fetchTransformedBilinearFP_uint32<blendType>(buffer, data, y, x, length);
    }
}
#endif // QT_CONFIG(raster_fp)

// FetchUntransformed can have more specialized methods added depending on SIMD features.
static SourceFetchProc sourceFetchUntransformed[] = {
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
    fetchUntransformed,         // RGBX16FPx4
    fetchUntransformed,         // RGBA16FPx4
    fetchUntransformed,         // RGBA16FPx4_Premultiplied
    fetchUntransformed,         // RGBX32Px4
    fetchUntransformed,         // RGBA32FPx4
    fetchUntransformed,         // RGBA32FPx4_Premultiplied
    fetchUntransformed,         // CMYK8888
};

static_assert(std::size(sourceFetchUntransformed) == QImage::NImageFormats);

static const SourceFetchProc sourceFetchGeneric[] = {
    fetchUntransformed,                                                             // Untransformed
    fetchUntransformed,                                                             // Tiled
    fetchTransformed<BlendTransformed, QPixelLayout::BPPNone>,                      // Transformed
    fetchTransformed<BlendTransformedTiled, QPixelLayout::BPPNone>,                 // TransformedTiled
    fetchTransformedBilinear<BlendTransformedBilinear, QPixelLayout::BPPNone>,      // TransformedBilinear
    fetchTransformedBilinear<BlendTransformedBilinearTiled, QPixelLayout::BPPNone>  // TransformedBilinearTiled
};

static_assert(std::size(sourceFetchGeneric) == NBlendTypes);

static SourceFetchProc sourceFetchARGB32PM[] = {
    fetchUntransformedARGB32PM,                                     // Untransformed
    fetchUntransformedARGB32PM,                                     // Tiled
    fetchTransformed<BlendTransformed, QPixelLayout::BPP32>,        // Transformed
    fetchTransformed<BlendTransformedTiled, QPixelLayout::BPP32>,   // TransformedTiled
    fetchTransformedBilinearARGB32PM<BlendTransformedBilinear>,     // Bilinear
    fetchTransformedBilinearARGB32PM<BlendTransformedBilinearTiled> // BilinearTiled
};

static_assert(std::size(sourceFetchARGB32PM) == NBlendTypes);

static SourceFetchProc sourceFetchAny16[] = {
    fetchUntransformed,                                                             // Untransformed
    fetchUntransformed,                                                             // Tiled
    fetchTransformed<BlendTransformed, QPixelLayout::BPP16>,                        // Transformed
    fetchTransformed<BlendTransformedTiled, QPixelLayout::BPP16>,                   // TransformedTiled
    fetchTransformedBilinear<BlendTransformedBilinear, QPixelLayout::BPP16>,        // TransformedBilinear
    fetchTransformedBilinear<BlendTransformedBilinearTiled, QPixelLayout::BPP16>    // TransformedBilinearTiled
};

static_assert(std::size(sourceFetchAny16) == NBlendTypes);

static SourceFetchProc sourceFetchAny32[] = {
    fetchUntransformed,                                                             // Untransformed
    fetchUntransformed,                                                             // Tiled
    fetchTransformed<BlendTransformed, QPixelLayout::BPP32>,                        // Transformed
    fetchTransformed<BlendTransformedTiled, QPixelLayout::BPP32>,                   // TransformedTiled
    fetchTransformedBilinear<BlendTransformedBilinear, QPixelLayout::BPP32>,        // TransformedBilinear
    fetchTransformedBilinear<BlendTransformedBilinearTiled, QPixelLayout::BPP32>    // TransformedBilinearTiled
};

static_assert(std::size(sourceFetchAny32) == NBlendTypes);

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
static const SourceFetchProc64 sourceFetchGeneric64[] = {
    fetchUntransformed64,                                     // Untransformed
    fetchUntransformed64,                                     // Tiled
    fetchTransformed64<BlendTransformed>,                     // Transformed
    fetchTransformed64<BlendTransformedTiled>,                // TransformedTiled
    fetchTransformedBilinear64<BlendTransformedBilinear>,     // Bilinear
    fetchTransformedBilinear64<BlendTransformedBilinearTiled> // BilinearTiled
};

static_assert(std::size(sourceFetchGeneric64) == NBlendTypes);

static const SourceFetchProc64 sourceFetchRGBA64PM[] = {
    fetchUntransformedRGBA64PM,                               // Untransformed
    fetchUntransformedRGBA64PM,                               // Tiled
    fetchTransformed64<BlendTransformed>,                     // Transformed
    fetchTransformed64<BlendTransformedTiled>,                // TransformedTiled
    fetchTransformedBilinear64<BlendTransformedBilinear>,     // Bilinear
    fetchTransformedBilinear64<BlendTransformedBilinearTiled> // BilinearTiled
};

static_assert(std::size(sourceFetchRGBA64PM) == NBlendTypes);

static inline SourceFetchProc64 getSourceFetch64(TextureBlendType blendType, QImage::Format format)
{
    if (format == QImage::Format_RGBX64 || format == QImage::Format_RGBA64_Premultiplied)
        return sourceFetchRGBA64PM[blendType];
    return sourceFetchGeneric64[blendType];
}
#endif

#if QT_CONFIG(raster_fp)
static const SourceFetchProcFP sourceFetchGenericFP[] = {
    fetchUntransformedFP,                                     // Untransformed
    fetchUntransformedFP,                                     // Tiled
    fetchTransformedFP<BlendTransformed>,                     // Transformed
    fetchTransformedFP<BlendTransformedTiled>,                // TransformedTiled
    fetchTransformedBilinearFP<BlendTransformedBilinear>,     // Bilinear
    fetchTransformedBilinearFP<BlendTransformedBilinearTiled> // BilinearTiled
};

static_assert(std::size(sourceFetchGenericFP) == NBlendTypes);

static inline SourceFetchProcFP getSourceFetchFP(TextureBlendType blendType, QImage::Format /*format*/)
{
    return sourceFetchGenericFP[blendType];
}
#endif

#define FIXPT_BITS 8
#define FIXPT_SIZE (1<<FIXPT_BITS)
#define FIXPT_MAX (INT_MAX >> (FIXPT_BITS + 1))

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

#if QT_CONFIG(raster_fp)
static inline QRgbaFloat32 qt_gradient_pixelFP(const QGradientData *data, qreal pos)
{
    int ipos = int(pos * (GRADIENT_STOPTABLE_SIZE - 1) + qreal(0.5));
    QRgba64 rgb64 = data->colorTable64[qt_gradient_clamp(data, ipos)];
    return QRgbaFloat32::fromRgba64(rgb64.red(),rgb64.green(), rgb64.blue(), rgb64.alpha());
}

static inline QRgbaFloat32 qt_gradient_pixelFP_fixed(const QGradientData *data, int fixed_pos)
{
    int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;
    QRgba64 rgb64 = data->colorTable64[qt_gradient_clamp(data, ipos)];
    return QRgbaFloat32::fromRgba64(rgb64.red(), rgb64.green(), rgb64.blue(), rgb64.alpha());
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
        Q_ASSERT(std::isfinite(v));
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
        Q_ASSERT(std::isfinite(v));
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

#if QT_CONFIG(raster_fp)
class GradientBaseFP
{
public:
    typedef QRgbaFloat32 Type;
    static Type null() { return QRgbaFloat32::fromRgba64(0,0,0,0); }
    static Type fetchSingle(const QGradientData& gradient, qreal v)
    {
        Q_ASSERT(std::isfinite(v));
        return qt_gradient_pixelFP(&gradient, v);
    }
    static Type fetchSingle(const QGradientData& gradient, int v)
    {
        return qt_gradient_pixelFP_fixed(&gradient, v);
    }
    static void memfill(Type *buffer, Type fill, int length)
    {
        quint64 fillCopy;
        memcpy(&fillCopy, &fill, sizeof(quint64));
        qt_memfill64((quint64*)buffer, fillCopy, length);
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
            if (std::abs(t) < FIXPT_MAX)
                GradientBase::memfill(buffer, GradientBase::fetchSingle(data->gradient, int(t * FIXPT_SIZE)), length);
            else
                GradientBase::memfill(buffer, GradientBase::fetchSingle(data->gradient, t / GRADIENT_STOPTABLE_SIZE), length);
        } else {
            if (std::abs(t) < FIXPT_MAX && std::abs(inc) < FIXPT_MAX && std::abs(t + inc * length) < FIXPT_MAX) {
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
#if QT_CONFIG(raster_fp)
static const QRgbaFloat32 * QT_FASTCALL qt_fetch_linear_gradient_rgbfp(QRgbaFloat32 *buffer, const Operator *op, const QSpanData *data,
                                                                   int y, int x, int length)
{
    return qt_fetch_linear_gradient_template<GradientBaseFP, QRgbaFloat32>(buffer, op, data, y, x, length);
}
#endif

static void QT_FASTCALL getRadialGradientValues(RadialGradientValues *v, const QSpanData *data)
{
    v->dx = data->gradient.radial.center.x - data->gradient.radial.focal.x;
    v->dy = data->gradient.radial.center.y - data->gradient.radial.focal.y;

    v->dr = data->gradient.radial.center.radius - data->gradient.radial.focal.radius;
    v->sqrfr = data->gradient.radial.focal.radius * data->gradient.radial.focal.radius;

    v->a = v->dr * v->dr - v->dx*v->dx - v->dy*v->dy;

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
                BlendType result = GradientBase::null();
                if (det >= 0) {
                    qreal w = qSqrt(det) - b;
                    result = GradientBase::fetchSingle(data->gradient, w);
                }

                *buffer++ = result;

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

#if QT_CONFIG(raster_fp)
static const QRgbaFloat32 * QT_FASTCALL qt_fetch_radial_gradient_rgbfp(QRgbaFloat32 *buffer, const Operator *op, const QSpanData *data,
                                                                   int y, int x, int length)
{
    return qt_fetch_radial_gradient_template<RadialFetchPlain<GradientBaseFP>, QRgbaFloat32>(buffer, op, data, y, x, length);
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

#if QT_CONFIG(raster_fp)
static const QRgbaFloat32 * QT_FASTCALL qt_fetch_conical_gradient_rgbfp(QRgbaFloat32 *buffer, const Operator *, const QSpanData *data,
                                                                    int y, int x, int length)
{
    return qt_fetch_conical_gradient_template<GradientBaseFP, QRgbaFloat32>(buffer, data, y, x, length);
}
#endif

extern CompositionFunctionSolid qt_functionForModeSolid_C[];
extern CompositionFunctionSolid64 qt_functionForModeSolid64_C[];
extern CompositionFunctionSolidFP qt_functionForModeSolidFP_C[];

static const CompositionFunctionSolid *functionForModeSolid = qt_functionForModeSolid_C;
#if QT_CONFIG(raster_64bit)
static const CompositionFunctionSolid64 *functionForModeSolid64 = qt_functionForModeSolid64_C;
#endif
#if QT_CONFIG(raster_fp)
static const CompositionFunctionSolidFP *functionForModeSolidFP = qt_functionForModeSolidFP_C;
#endif

extern CompositionFunction qt_functionForMode_C[];
extern CompositionFunction64 qt_functionForMode64_C[];
extern CompositionFunctionFP qt_functionForModeFP_C[];

static const CompositionFunction *functionForMode = qt_functionForMode_C;
#if QT_CONFIG(raster_64bit)
static const CompositionFunction64 *functionForMode64 = qt_functionForMode64_C;
#endif
#if QT_CONFIG(raster_fp)
static const CompositionFunctionFP *functionForModeFP = qt_functionForModeFP_C;
#endif

static TextureBlendType getBlendType(const QSpanData *data)
{
    TextureBlendType ft;
    if (data->texture.type == QTextureData::Pattern)
        ft = BlendTiled;
    else if (data->txop <= QTransform::TxTranslate)
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

static inline Operator getOperator(const QSpanData *data, const QT_FT_Span *spans, int spanCount)
{
    Operator op;
    bool solidSource = false;
    switch(data->type) {
    case QSpanData::Solid:
        solidSource = data->solidColor.alphaF() >= 1.0f;
        op.noGradient = {};
        op.srcFetch = nullptr;
        op.srcFetch64 = nullptr;
        op.srcFetchFP = nullptr;
        break;
    case QSpanData::LinearGradient:
        solidSource = !data->gradient.alphaColor;
        getLinearGradientValues(&op.linear, data);
        op.srcFetch = qt_fetch_linear_gradient;
#if QT_CONFIG(raster_64bit)
        op.srcFetch64 = qt_fetch_linear_gradient_rgb64;
#endif
#if QT_CONFIG(raster_fp)
        op.srcFetchFP = qt_fetch_linear_gradient_rgbfp;
#endif
        break;
    case QSpanData::RadialGradient:
        solidSource = !data->gradient.alphaColor;
        getRadialGradientValues(&op.radial, data);
        op.srcFetch = qt_fetch_radial_gradient;
#if QT_CONFIG(raster_64bit)
        op.srcFetch64 = qt_fetch_radial_gradient_rgb64;
#endif
#if QT_CONFIG(raster_fp)
        op.srcFetchFP = qt_fetch_radial_gradient_rgbfp;
#endif
        break;
    case QSpanData::ConicalGradient:
        solidSource = !data->gradient.alphaColor;
        op.noGradient = {}; // sic!
        op.srcFetch = qt_fetch_conical_gradient;
#if QT_CONFIG(raster_64bit)
        op.srcFetch64 = qt_fetch_conical_gradient_rgb64;
#endif
#if QT_CONFIG(raster_fp)
        op.srcFetchFP = qt_fetch_conical_gradient_rgbfp;
#endif
        break;
    case QSpanData::Texture:
        solidSource = !data->texture.hasAlpha;
        op.noGradient = {};
        op.srcFetch = getSourceFetch(getBlendType(data), data->texture.format);
#if QT_CONFIG(raster_64bit)
        op.srcFetch64 = getSourceFetch64(getBlendType(data), data->texture.format);
#endif
#if QT_CONFIG(raster_fp)
        op.srcFetchFP = getSourceFetchFP(getBlendType(data), data->texture.format);
#endif
        break;
    default:
        Q_UNREACHABLE();
        break;
    }
#if !QT_CONFIG(raster_64bit)
    op.srcFetch64 = nullptr;
#endif
#if !QT_CONFIG(raster_fp)
    op.srcFetchFP = nullptr;
#endif

    op.mode = data->rasterBuffer->compositionMode;
    if (op.mode == QPainter::CompositionMode_SourceOver && solidSource)
        op.mode = QPainter::CompositionMode_Source;

    op.destFetch = destFetchProc[data->rasterBuffer->format];
#if QT_CONFIG(raster_64bit)
    op.destFetch64 = destFetchProc64[data->rasterBuffer->format];
#else
    op.destFetch64 = nullptr;
#endif
#if QT_CONFIG(raster_fp)
    op.destFetchFP = destFetchProcFP[data->rasterBuffer->format];
#else
    op.destFetchFP = nullptr;
#endif
    if (op.mode == QPainter::CompositionMode_Source &&
            (data->type != QSpanData::Texture || data->texture.const_alpha == 256)) {
        const QT_FT_Span *lastSpan = spans + spanCount;
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
#if QT_CONFIG(raster_fp)
            if (op.destFetchFP != destFetchRGBFP)
                op.destFetchFP = destFetchFPUndefined;
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
    op.destStore64 = nullptr;
    op.funcSolid64 = nullptr;
    op.func64 = nullptr;
#endif
#if QT_CONFIG(raster_fp)
    op.destStoreFP = destStoreFP;
    op.funcSolidFP = functionForModeSolidFP[op.mode];
    op.funcFP = functionForModeFP[op.mode];
#else
    op.destStoreFP = nullptr;
    op.funcSolidFP = nullptr;
    op.funcFP = nullptr;
#endif

    return op;
}

static void spanfill_from_first(QRasterBuffer *rasterBuffer, QPixelLayout::BPP bpp, int x, int y, int length)
{
    switch (bpp) {
    case QPixelLayout::BPP32FPx4: {
        QRgbaFloat32 *dest = reinterpret_cast<QRgbaFloat32 *>(rasterBuffer->scanLine(y)) + x;
        qt_memfill_template(dest + 1, dest[0], length - 1);
        break;
    }
    case QPixelLayout::BPP16FPx4:
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

#if QT_CONFIG(qtgui_threadpool)
#define QT_THREAD_PARALLEL_FILLS(function) \
    const int segments = (count + 32) / 64; \
    QThreadPool *threadPool = QGuiApplicationPrivate::qtGuiThreadPool(); \
    if (segments > 1 && qPixelLayouts[data->rasterBuffer->format].bpp >= QPixelLayout::BPP8 \
             && threadPool && !threadPool->contains(QThread::currentThread())) { \
        QSemaphore semaphore; \
        int c = 0; \
        for (int i = 0; i < segments; ++i) { \
            int cn = (count - c) / (segments - i); \
            threadPool->start([&, c, cn]() { \
                function(c, c + cn); \
                semaphore.release(1); \
            }, 1); \
            c += cn; \
        } \
        semaphore.acquire(segments); \
    } else \
        function(0, count)
#else
#define QT_THREAD_PARALLEL_FILLS(function) function(0, count)
#endif

static void blend_color_generic(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    const Operator op = getOperator(data, nullptr, 0);
    const uint color = data->solidColor.rgba();
    const bool solidFill = op.mode == QPainter::CompositionMode_Source;
    const QPixelLayout::BPP bpp = qPixelLayouts[data->rasterBuffer->format].bpp;

    auto function = [=] (int cStart, int cEnd) {
        alignas(16) Q_DECL_UNINITIALIZED uint buffer[BufferSize];
        for (int c = cStart; c < cEnd; ++c) {
            int x = spans[c].x;
            int length = spans[c].len;
            if (solidFill && bpp >= QPixelLayout::BPP8 && spans[c].coverage == 255 && length && op.destStore) {
                // If dest doesn't matter we don't need to bother with blending or converting all the identical pixels
                op.destStore(data->rasterBuffer, x, spans[c].y, &color, 1);
                spanfill_from_first(data->rasterBuffer, bpp, x, spans[c].y, length);
                length = 0;
            }

            while (length) {
                int l = qMin(BufferSize, length);
                uint *dest = op.destFetch(buffer, data->rasterBuffer, x, spans[c].y, l);
                op.funcSolid(dest, l, color, spans[c].coverage);
                if (op.destStore)
                    op.destStore(data->rasterBuffer, x, spans[c].y, dest, l);
                length -= l;
                x += l;
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}

static void blend_color_argb(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    const Operator op = getOperator(data, nullptr, 0);
    const uint color = data->solidColor.rgba();

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
    const auto funcSolid = op.funcSolid;
    auto function = [=] (int cStart, int cEnd) {
        for (int c = cStart; c < cEnd; ++c) {
            uint *target = ((uint *)data->rasterBuffer->scanLine(spans[c].y)) + spans[c].x;
            funcSolid(target, spans[c].len, color, spans[c].coverage);
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}

static void blend_color_generic_rgb64(int count, const QT_FT_Span *spans, void *userData)
{
#if QT_CONFIG(raster_64bit)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    const Operator op = getOperator(data, nullptr, 0);
    if (!op.funcSolid64) {
        qCDebug(lcQtGuiDrawHelper, "blend_color_generic_rgb64: unsupported 64bit blend attempted, falling back to 32-bit");
        return blend_color_generic(count, spans, userData);
    }

    const QRgba64 color = data->solidColor.rgba64();
    const bool solidFill = op.mode == QPainter::CompositionMode_Source;
    const QPixelLayout::BPP bpp = qPixelLayouts[data->rasterBuffer->format].bpp;

    auto function = [=, &op] (int cStart, int cEnd)
    {
        alignas(16) Q_DECL_UNINITIALIZED QRgba64 buffer[BufferSize];
        for (int c = cStart; c < cEnd; ++c) {
            int x = spans[c].x;
            int length = spans[c].len;
            if (solidFill && bpp >= QPixelLayout::BPP8 && spans[c].coverage == 255 && length && op.destStore64) {
                // If dest doesn't matter we don't need to bother with blending or converting all the identical pixels
                op.destStore64(data->rasterBuffer, x, spans[c].y, &color, 1);
                spanfill_from_first(data->rasterBuffer, bpp, x, spans[c].y, length);
                length = 0;
            }

            while (length) {
                int l = qMin(BufferSize, length);
                QRgba64 *dest = op.destFetch64(buffer, data->rasterBuffer, x, spans[c].y, l);
                op.funcSolid64(dest, l, color, spans[c].coverage);
                if (op.destStore64)
                    op.destStore64(data->rasterBuffer, x, spans[c].y, dest, l);
                length -= l;
                x += l;
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
#else
    blend_color_generic(count, spans, userData);
#endif
}

static void blend_color_generic_fp(int count, const QT_FT_Span *spans, void *userData)
{
#if QT_CONFIG(raster_fp)
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    const Operator op = getOperator(data, nullptr, 0);
    if (!op.funcSolidFP || !op.destFetchFP) {
        qCDebug(lcQtGuiDrawHelper, "blend_color_generic_fp: unsupported 4xF16 blend attempted, falling back to 32-bit");
        return blend_color_generic(count, spans, userData);
    }

    float r, g, b, a;
    data->solidColor.getRgbF(&r, &g, &b, &a);
    const QRgbaFloat32 color{r, g, b, a};
    const bool solidFill = op.mode == QPainter::CompositionMode_Source;
    QPixelLayout::BPP bpp = qPixelLayouts[data->rasterBuffer->format].bpp;

    auto function = [=, &op] (int cStart, int cEnd)
    {
        alignas(16) Q_DECL_UNINITIALIZED QRgbaFloat32 buffer[BufferSize];
        for (int c = cStart; c < cEnd; ++c) {
            int x = spans[c].x;
            int length = spans[c].len;
            if (solidFill && bpp >= QPixelLayout::BPP8 && spans[c].coverage == 255 && length && op.destStoreFP) {
                // If dest doesn't matter we don't need to bother with blending or converting all the identical pixels
                op.destStoreFP(data->rasterBuffer, x, spans[c].y, &color, 1);
                spanfill_from_first(data->rasterBuffer, bpp, x, spans[c].y, length);
                length = 0;
            }

            while (length) {
                int l = qMin(BufferSize, length);
                QRgbaFloat32 *dest = op.destFetchFP(buffer, data->rasterBuffer, x, spans[c].y, l);
                op.funcSolidFP(dest, l, color, spans[c].coverage);
                if (op.destStoreFP)
                    op.destStoreFP(data->rasterBuffer, x, spans[c].y, dest, l);
                length -= l;
                x += l;
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
#else
    blend_color_generic(count, spans, userData);
#endif
}

template <typename T>
void handleSpans(int count, const QT_FT_Span *spans, const QSpanData *data, const Operator &op)
{
    const int const_alpha = (data->type == QSpanData::Texture) ? data->texture.const_alpha : 256;
    const bool solidSource = op.mode == QPainter::CompositionMode_Source && const_alpha == 256;

    auto function = [=, &op] (int cStart, int cEnd)
    {
        T Q_DECL_UNINITIALIZED handler(data, op);
        int coverage = 0;
        for (int c = cStart; c < cEnd;) {
            if (!spans[c].len) {
                ++c;
                continue;
            }
            int x = spans[c].x;
            const int y = spans[c].y;
            int right = x + spans[c].len;
            const bool fetchDest = !solidSource || spans[c].coverage < 255;

            // compute length of adjacent spans
            for (int i = c + 1; i < cEnd && spans[i].y == y && spans[i].x == right && fetchDest == (!solidSource || spans[i].coverage < 255); ++i)
                right += spans[i].len;
            int length = right - x;

            while (length) {
                int l = qMin(BufferSize, length);
                length -= l;

                int process_length = l;
                int process_x = x;

                const auto *src = handler.fetch(process_x, y, process_length, fetchDest);
                int offset = 0;
                while (l > 0) {
                    if (x == spans[c].x) // new span?
                        coverage = (spans[c].coverage * const_alpha) >> 8;

                    int right = spans[c].x + spans[c].len;
                    int len = qMin(l, right - x);

                    handler.process(x, y, len, coverage, src, offset);

                    l -= len;
                    x += len;
                    offset += len;

                    if (x == right) // done with current span?
                        ++c;
                }
                handler.store(process_x, y, process_length);
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}

struct QBlendBase
{
    const QSpanData *data;
    const Operator &op;
};

class BlendSrcGeneric : public QBlendBase
{
public:
    uint *dest = nullptr;
    alignas(16) uint buffer[BufferSize];
    alignas(16) uint src_buffer[BufferSize];
    BlendSrcGeneric(const QSpanData *d, const Operator &o)
        : QBlendBase{d, o}
    {
    }

    const uint *fetch(int x, int y, int len, bool fetchDest)
    {
        if (fetchDest || op.destFetch == destFetchARGB32P)
            dest = op.destFetch(buffer, data->rasterBuffer, x, y, len);
        else
            dest = buffer;
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
class BlendSrcGenericRGB64 : public QBlendBase
{
public:
    QRgba64 *dest = nullptr;
    alignas(16) QRgba64 buffer[BufferSize];
    alignas(16) QRgba64 src_buffer[BufferSize];
    BlendSrcGenericRGB64(const QSpanData *d, const Operator &o)
        : QBlendBase{d, o}
    {
    }

    bool isSupported() const
    {
        return op.func64 && op.destFetch64;
    }

    const QRgba64 *fetch(int x, int y, int len, bool fetchDest)
    {
        if (fetchDest || op.destFetch64 == destFetchRGB64)
            dest = op.destFetch64(buffer, data->rasterBuffer, x, y, len);
        else
            dest = buffer;
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

#if QT_CONFIG(raster_fp)
class BlendSrcGenericRGBFP : public QBlendBase
{
public:
    QRgbaFloat32 *dest = nullptr;
    alignas(16) QRgbaFloat32 buffer[BufferSize];
    alignas(16) QRgbaFloat32 src_buffer[BufferSize];
    BlendSrcGenericRGBFP(const QSpanData *d, const Operator &o)
        : QBlendBase{d, o}
    {
    }

    bool isSupported() const
    {
        return op.funcFP && op.destFetchFP && op.srcFetchFP;
    }

    const QRgbaFloat32 *fetch(int x, int y, int len, bool fetchDest)
    {
        if (fetchDest || op.destFetchFP == destFetchRGBFP)
            dest = op.destFetchFP(buffer, data->rasterBuffer, x, y, len);
        else
            dest = buffer;
        return op.srcFetchFP(src_buffer, &op, data, y, x, len);
    }

    void process(int, int, int len, int coverage, const QRgbaFloat32 *src, int offset)
    {
        op.funcFP(dest + offset, src + offset, len, coverage);
    }

    void store(int x, int y, int len)
    {
        if (op.destStoreFP)
            op.destStoreFP(data->rasterBuffer, x, y, dest, len);
    }
};
#endif

static void blend_src_generic(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    const Operator op = getOperator(data, nullptr, 0);
    handleSpans<BlendSrcGeneric>(count, spans, data, op);
}

#if QT_CONFIG(raster_64bit)
static void blend_src_generic_rgb64(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    const Operator op = getOperator(data, nullptr, 0);
    if (op.func64 && op.destFetch64) {
        handleSpans<BlendSrcGenericRGB64>(count, spans, data, op);
    } else {
        qCDebug(lcQtGuiDrawHelper, "blend_src_generic_rgb64: unsupported 64-bit blend attempted, falling back to 32-bit");
        handleSpans<BlendSrcGeneric>(count, spans, data, op);
    }
}
#endif

#if QT_CONFIG(raster_fp)
static void blend_src_generic_fp(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    const Operator op = getOperator(data, spans, count);
    if (op.funcFP && op.destFetchFP && op.srcFetchFP) {
        handleSpans<BlendSrcGenericRGBFP>(count, spans, data, op);
    } else {
        qCDebug(lcQtGuiDrawHelper, "blend_src_generic_fp: unsupported 4xFP blend attempted, falling back to 32-bit");
        handleSpans<BlendSrcGeneric>(count, spans, data, op);
    }
}
#endif

static void blend_untransformed_generic(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    const Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    const int const_alpha = data->texture.const_alpha;
    const int xoff = -qRound(-data->dx);
    const int yoff = -qRound(-data->dy);
    const bool solidSource = op.mode == QPainter::CompositionMode_Source && const_alpha == 256 && op.destFetch != destFetchARGB32P;

    auto function = [=, &op] (int cStart, int cEnd)
    {
        alignas(16) Q_DECL_UNINITIALIZED uint buffer[BufferSize];
        alignas(16) Q_DECL_UNINITIALIZED uint src_buffer[BufferSize];
        for (int c = cStart; c < cEnd; ++c) {
            if (!spans[c].len)
                continue;
            int x = spans[c].x;
            int length = spans[c].len;
            int sx = xoff + x;
            int sy = yoff + spans[c].y;
            const bool fetchDest = !solidSource || spans[c].coverage < 255;
            if (sy >= 0 && sy < image_height && sx < image_width) {
                if (sx < 0) {
                    x -= sx;
                    length += sx;
                    sx = 0;
                }
                if (sx + length > image_width)
                    length = image_width - sx;
                if (length > 0) {
                    const int coverage = (spans[c].coverage * const_alpha) >> 8;
                    while (length) {
                        int l = qMin(BufferSize, length);
                        const uint *src = op.srcFetch(src_buffer, &op, data, sy, sx, l);
                        uint *dest = fetchDest ? op.destFetch(buffer, data->rasterBuffer, x, spans[c].y, l) : buffer;
                        op.func(dest, src, l, coverage);
                        if (op.destStore)
                            op.destStore(data->rasterBuffer, x, spans[c].y, dest, l);
                        x += l;
                        sx += l;
                        length -= l;
                    }
                }
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}

#if QT_CONFIG(raster_64bit)
static void blend_untransformed_generic_rgb64(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    const Operator op = getOperator(data, spans, count);
    if (!op.func64) {
        qCDebug(lcQtGuiDrawHelper, "blend_untransformed_generic_rgb64: unsupported 64-bit blend attempted, falling back to 32-bit");
        return blend_untransformed_generic(count, spans, userData);
    }

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    const int const_alpha = data->texture.const_alpha;
    const int xoff = -qRound(-data->dx);
    const int yoff = -qRound(-data->dy);
    const bool solidSource = op.mode == QPainter::CompositionMode_Source && const_alpha == 256 && op.destFetch64 != destFetchRGB64;

    auto function = [=, &op] (int cStart, int cEnd)
    {
        alignas(16) Q_DECL_UNINITIALIZED QRgba64 buffer[BufferSize];
        alignas(16) Q_DECL_UNINITIALIZED QRgba64 src_buffer[BufferSize];
        for (int c = cStart; c < cEnd; ++c) {
            if (!spans[c].len)
                continue;
            int x = spans[c].x;
            int length = spans[c].len;
            int sx = xoff + x;
            int sy = yoff + spans[c].y;
            const bool fetchDest = !solidSource || spans[c].coverage < 255;
            if (sy >= 0 && sy < image_height && sx < image_width) {
                if (sx < 0) {
                    x -= sx;
                    length += sx;
                    sx = 0;
                }
                if (sx + length > image_width)
                    length = image_width - sx;
                if (length > 0) {
                    const int coverage = (spans[c].coverage * const_alpha) >> 8;
                    while (length) {
                        int l = qMin(BufferSize, length);
                        const QRgba64 *src = op.srcFetch64(src_buffer, &op, data, sy, sx, l);
                        QRgba64 *dest = fetchDest ? op.destFetch64(buffer, data->rasterBuffer, x, spans[c].y, l) : buffer;
                        op.func64(dest, src, l, coverage);
                        if (op.destStore64)
                            op.destStore64(data->rasterBuffer, x, spans[c].y, dest, l);
                        x += l;
                        sx += l;
                        length -= l;
                    }
                }
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}
#endif

#if QT_CONFIG(raster_fp)
static void blend_untransformed_generic_fp(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    const Operator op = getOperator(data, spans, count);
    if (!op.funcFP) {
        qCDebug(lcQtGuiDrawHelper, "blend_untransformed_generic_rgbaf16: unsupported 4xFP16 blend attempted, falling back to 32-bit");
        return blend_untransformed_generic(count, spans, userData);
    }

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    const int xoff = -qRound(-data->dx);
    const int yoff = -qRound(-data->dy);
    const bool solidSource = op.mode == QPainter::CompositionMode_Source && data->texture.const_alpha == 256 && op.destFetchFP != destFetchRGBFP;

    auto function = [=, &op] (int cStart, int cEnd)
    {
        alignas(16) Q_DECL_UNINITIALIZED QRgbaFloat32 buffer[BufferSize];
        alignas(16) Q_DECL_UNINITIALIZED QRgbaFloat32 src_buffer[BufferSize];
        for (int c = cStart; c < cEnd; ++c) {
            if (!spans[c].len)
                continue;
            int x = spans[c].x;
            int length = spans[c].len;
            int sx = xoff + x;
            int sy = yoff + spans[c].y;
            const bool fetchDest = !solidSource || spans[c].coverage < 255;
            if (sy >= 0 && sy < image_height && sx < image_width) {
                if (sx < 0) {
                    x -= sx;
                    length += sx;
                    sx = 0;
                }
                if (sx + length > image_width)
                    length = image_width - sx;
                if (length > 0) {
                    const int coverage = (spans[c].coverage * data->texture.const_alpha) >> 8;
                    while (length) {
                        int l = qMin(BufferSize, length);
                        const QRgbaFloat32 *src = op.srcFetchFP(src_buffer, &op, data, sy, sx, l);
                        QRgbaFloat32 *dest = fetchDest ? op.destFetchFP(buffer, data->rasterBuffer, x, spans[c].y, l) : buffer;
                        op.funcFP(dest, src, l, coverage);
                        if (op.destStoreFP)
                            op.destStoreFP(data->rasterBuffer, x, spans[c].y, dest, l);
                        x += l;
                        sx += l;
                        length -= l;
                    }
                }
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}
#endif

static void blend_untransformed_argb(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_untransformed_generic(count, spans, userData);
        return;
    }

    const Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    const int const_alpha = data->texture.const_alpha;
    const int xoff = -qRound(-data->dx);
    const int yoff = -qRound(-data->dy);

    auto function = [=, &op] (int cStart, int cEnd)
    {
        for (int c = cStart; c < cEnd; ++c) {
            if (!spans[c].len)
                continue;
            int x = spans[c].x;
            int length = spans[c].len;
            int sx = xoff + x;
            int sy = yoff + spans[c].y;
            if (sy >= 0 && sy < image_height && sx < image_width) {
                if (sx < 0) {
                    x -= sx;
                    length += sx;
                    sx = 0;
                }
                if (sx + length > image_width)
                    length = image_width - sx;
                if (length > 0) {
                    const int coverage = (spans[c].coverage * const_alpha) >> 8;
                    const uint *src = (const uint *)data->texture.scanLine(sy) + sx;
                    uint *dest = ((uint *)data->rasterBuffer->scanLine(spans[c].y)) + x;
                    op.func(dest, src, length, coverage);
                }
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
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

static void blend_untransformed_rgb565(int count, const QT_FT_Span *spans, void *userData)
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

    auto function = [=](int cStart, int cEnd)
    {
        for (int c = cStart; c < cEnd; ++c) {
            if (!spans[c].len)
                continue;
            const quint8 coverage = (data->texture.const_alpha * spans[c].coverage) >> 8;
            if (coverage == 0)
                continue;

            int x = spans[c].x;
            int length = spans[c].len;
            int sx = xoff + x;
            int sy = yoff + spans[c].y;
            if (sy >= 0 && sy < image_height && sx < image_width) {
                if (sx < 0) {
                    x -= sx;
                    length += sx;
                    sx = 0;
                }
                if (sx + length > image_width)
                    length = image_width - sx;
                if (length > 0) {
                    quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans[c].y) + x;
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
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}

static void blend_tiled_generic(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    const Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    const int const_alpha = data->texture.const_alpha;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    auto function = [=, &op](int cStart, int cEnd)
    {
        alignas(16) Q_DECL_UNINITIALIZED uint buffer[BufferSize];
        alignas(16) Q_DECL_UNINITIALIZED uint src_buffer[BufferSize];
        for (int c = cStart; c < cEnd; ++c) {
            int x = spans[c].x;
            int length = spans[c].len;
            int sx = (xoff + spans[c].x) % image_width;
            int sy = (spans[c].y + yoff) % image_height;
            if (sx < 0)
                sx += image_width;
            if (sy < 0)
                sy += image_height;

            const int coverage = (spans[c].coverage * const_alpha) >> 8;
            while (length) {
                int l = qMin(image_width - sx, length);
                if (BufferSize < l)
                    l = BufferSize;
                const uint *src = op.srcFetch(src_buffer, &op, data, sy, sx, l);
                uint *dest = op.destFetch(buffer, data->rasterBuffer, x, spans[c].y, l);
                op.func(dest, src, l, coverage);
                if (op.destStore)
                    op.destStore(data->rasterBuffer, x, spans[c].y, dest, l);
                x += l;
                sx += l;
                length -= l;
                if (sx >= image_width)
                    sx = 0;
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}

#if QT_CONFIG(raster_64bit)
static void blend_tiled_generic_rgb64(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    const Operator op = getOperator(data, spans, count);
    if (!op.func64) {
        qCDebug(lcQtGuiDrawHelper, "blend_tiled_generic_rgb64: unsupported 64-bit blend attempted, falling back to 32-bit");
        return blend_tiled_generic(count, spans, userData);
    }

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
        alignas(16) Q_DECL_UNINITIALIZED QRgba64 src_buffer[BufferSize];
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

    auto function = [=, &op](int cStart, int cEnd)
    {
        alignas(16) Q_DECL_UNINITIALIZED QRgba64 buffer[BufferSize];
        alignas(16) Q_DECL_UNINITIALIZED QRgba64 src_buffer[BufferSize];
        for (int c = cStart; c < cEnd; ++c) {
            int x = spans[c].x;
            int length = spans[c].len;
            int sx = (xoff + spans[c].x) % image_width;
            int sy = (spans[c].y + yoff) % image_height;
            if (sx < 0)
                sx += image_width;
            if (sy < 0)
                sy += image_height;

            const int coverage = (spans[c].coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(image_width - sx, length);
                if (BufferSize < l)
                    l = BufferSize;
                const QRgba64 *src = op.srcFetch64(src_buffer, &op, data, sy, sx, l);
                QRgba64 *dest = op.destFetch64(buffer, data->rasterBuffer, x, spans[c].y, l);
                op.func64(dest, src, l, coverage);
                if (op.destStore64)
                    op.destStore64(data->rasterBuffer, x, spans[c].y, dest, l);
                x += l;
                sx += l;
                length -= l;
                if (sx >= image_width)
                    sx = 0;
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}
#endif

#if QT_CONFIG(raster_fp)
static void blend_tiled_generic_fp(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    const Operator op = getOperator(data, spans, count);
    if (!op.funcFP) {
        qCDebug(lcQtGuiDrawHelper, "blend_tiled_generic_fp: unsupported 4xFP blend attempted, falling back to 32-bit");
        return blend_tiled_generic(count, spans, userData);
    }

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    // Consider tiling optimizing like the other versions.

    auto function = [=, &op](int cStart, int cEnd)
    {
        alignas(16) Q_DECL_UNINITIALIZED QRgbaFloat32 buffer[BufferSize];
        alignas(16) Q_DECL_UNINITIALIZED QRgbaFloat32 src_buffer[BufferSize];
        for (int c = cStart; c < cEnd; ++c) {
            int x = spans[c].x;
            int length = spans[c].len;
            int sx = (xoff + spans[c].x) % image_width;
            int sy = (spans[c].y + yoff) % image_height;
            if (sx < 0)
                sx += image_width;
            if (sy < 0)
                sy += image_height;

            const int coverage = (spans[c].coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(image_width - sx, length);
                if (BufferSize < l)
                    l = BufferSize;
                const QRgbaFloat32 *src = op.srcFetchFP(src_buffer, &op, data, sy, sx, l);
                QRgbaFloat32 *dest = op.destFetchFP(buffer, data->rasterBuffer, x, spans[c].y, l);
                op.funcFP(dest, src, l, coverage);
                if (op.destStoreFP)
                    op.destStoreFP(data->rasterBuffer, x, spans[c].y, dest, l);
                x += l;
                sx += l;
                length -= l;
                if (sx >= image_width)
                    sx = 0;
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}
#endif

static void blend_tiled_argb(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_tiled_generic(count, spans, userData);
        return;
    }

    const Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;
    const auto func = op.func;
    const int const_alpha = data->texture.const_alpha;

    auto function = [=] (int cStart, int cEnd) {
        for (int c = cStart; c < cEnd; ++c) {
            int x = spans[c].x;
            int length = spans[c].len;
            int sx = (xoff + spans[c].x) % image_width;
            int sy = (spans[c].y + yoff) % image_height;
            if (sx < 0)
                sx += image_width;
            if (sy < 0)
                sy += image_height;

            const int coverage = (spans[c].coverage * const_alpha) >> 8;
            while (length) {
                int l = qMin(image_width - sx, length);
                if (BufferSize < l)
                    l = BufferSize;
                const uint *src = (const uint *)data->texture.scanLine(sy) + sx;
                uint *dest = ((uint *)data->rasterBuffer->scanLine(spans[c].y)) + x;
                func(dest, src, l, coverage);
                x += l;
                sx += l;
                length -= l;
                if (sx >= image_width)
                    sx = 0;
            }
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
}

static void blend_tiled_rgb565(int count, const QT_FT_Span *spans, void *userData)
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

    const int const_alpha = data->texture.const_alpha;
    auto function = [=] (int cStart, int cEnd) {
        for (int c = cStart; c < cEnd; ++c) {
            const quint8 coverage = (const_alpha * spans[c].coverage) >> 8;
            if (coverage == 0)
                continue;

            int x = spans[c].x;
            int length = spans[c].len;
            int sx = (xoff + spans[c].x) % image_width;
            int sy = (spans[c].y + yoff) % image_height;
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
                    quint16 *dest = ((quint16 *)data->rasterBuffer->scanLine(spans[c].y)) + tx;
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
                int copy_image_width = qMin(image_width, int(spans[c].len));
                length = spans[c].len - copy_image_width;
                quint16 *src = ((quint16 *)data->rasterBuffer->scanLine(spans[c].y)) + x;
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
                        quint16 *dest = ((quint16 *)data->rasterBuffer->scanLine(spans[c].y)) + x;
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
        }
    };
    QT_THREAD_PARALLEL_FILLS(function);
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

#if QT_CONFIG(raster_fp)
static const ProcessSpans processTextureSpansGenericFP[NBlendTypes] = {
    blend_untransformed_generic_fp,     // Untransformed
    blend_tiled_generic_fp,             // Tiled
    blend_src_generic_fp,               // Transformed
    blend_src_generic_fp,               // TransformedTiled
    blend_src_generic_fp,               // TransformedBilinear
    blend_src_generic_fp                // TransformedBilinearTiled
};
#endif
void qBlendTexture(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    TextureBlendType blendType = getBlendType(data);
    ProcessSpans proc;
    switch (data->rasterBuffer->format) {
    case QImage::Format_Invalid:
        Q_UNREACHABLE_RETURN();
    case QImage::Format_ARGB32_Premultiplied:
        proc = processTextureSpansARGB32PM[blendType];
        break;
    case QImage::Format_RGB16:
        proc = processTextureSpansRGB16[blendType];
        break;
#if defined(__SSE2__) || defined(__ARM_NEON__) || defined(QT_COMPILER_SUPPORTS_LSX) || (Q_PROCESSOR_WORDSIZE == 8)
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
#if !QT_CONFIG(raster_fp)
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
#endif
#if QT_CONFIG(raster_64bit)
        proc = processTextureSpansGeneric64[blendType];
        break;
#endif // QT_CONFIG(raster_64bit)
#if QT_CONFIG(raster_fp)
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        proc = processTextureSpansGenericFP[blendType];
        break;
#endif
    default:
        proc = processTextureSpansGeneric[blendType];
        break;
    }
    proc(count, spans, userData);
}

static inline bool calculate_fixed_gradient_factors(int count, const QT_FT_Span *spans,
                                                    const QSpanData *data,
                                                    const LinearGradientValues &linear,
                                                    int *pyinc, int *poff)
{
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
    qreal ryinc = linear.dy * data->m22 * gss * FIXPT_SIZE;
    qreal roff = (linear.dy * (data->m22 * qreal(0.5) + data->dy) + linear.off) * gss * FIXPT_SIZE;
    const int limit = std::numeric_limits<int>::max() - FIXPT_SIZE;
    if (count && (std::fabs(ryinc) < limit) && (std::fabs(roff) < limit)
        && (std::fabs(ryinc * spans->y + roff) < limit)
        && (std::fabs(ryinc * (spans + count - 1)->y + roff) < limit)) {
        *pyinc = int(ryinc);
        *poff = int(roff);
        return true;
    }
    return false;
}

static bool blend_vertical_gradient_argb(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    LinearGradientValues linear;
    getLinearGradientValues(&linear, data);

    CompositionFunctionSolid funcSolid =
        functionForModeSolid[data->rasterBuffer->compositionMode];

    int yinc(0), off(0);
    if (!calculate_fixed_gradient_factors(count, spans, data, linear, &yinc, &off))
        return false;

    while (count--) {
        int y = spans->y;
        int x = spans->x;

        quint32 *dst = (quint32 *)(data->rasterBuffer->scanLine(y)) + x;
        quint32 color =
            qt_gradient_pixel_fixed(&data->gradient, yinc * y + off);

        funcSolid(dst, spans->len, color, spans->coverage);
        ++spans;
    }
    return true;
}

template<ProcessSpans blend_color>
static bool blend_vertical_gradient(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    LinearGradientValues linear;
    getLinearGradientValues(&linear, data);

    int yinc(0), off(0);
    if (!calculate_fixed_gradient_factors(count, spans, data, linear, &yinc, &off))
        return false;

    while (count--) {
        int y = spans->y;

#if QT_CONFIG(raster_64bit)
        data->solidColor = qt_gradient_pixel64_fixed(&data->gradient, yinc * y + off);
#else
        data->solidColor = qt_gradient_pixel_fixed(&data->gradient, yinc * y + off);
#endif
        blend_color(1, spans, userData);
        ++spans;
    }
    return true;
}

void qBlendGradient(int count, const QT_FT_Span *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    bool isVerticalGradient =
        data->txop <= QTransform::TxScale &&
        data->type == QSpanData::LinearGradient &&
        data->gradient.linear.end.x == data->gradient.linear.origin.x;
    switch (data->rasterBuffer->format) {
    case QImage::Format_Invalid:
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        if (isVerticalGradient && blend_vertical_gradient_argb(count, spans, userData))
            return;
        return blend_src_generic(count, spans, userData);
#if defined(__SSE2__) || defined(__ARM_NEON__) || defined(QT_COMPILER_SUPPORTS_LSX) || (Q_PROCESSOR_WORDSIZE == 8)
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
#if !QT_CONFIG(raster_fp)
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
#endif
#if QT_CONFIG(raster_64bit)
        if (isVerticalGradient && blend_vertical_gradient<blend_color_generic_rgb64>(count, spans, userData))
            return;
        return blend_src_generic_rgb64(count, spans, userData);
#endif // QT_CONFIG(raster_64bit)
#if QT_CONFIG(raster_fp)
    case QImage::Format_RGBX16FPx4:
    case QImage::Format_RGBA16FPx4:
    case QImage::Format_RGBA16FPx4_Premultiplied:
    case QImage::Format_RGBX32FPx4:
    case QImage::Format_RGBA32FPx4:
    case QImage::Format_RGBA32FPx4_Premultiplied:
        if (isVerticalGradient && blend_vertical_gradient<blend_color_generic_fp>(count, spans, userData))
            return;
        return blend_src_generic_fp(count, spans, userData);
#endif
    default:
        if (isVerticalGradient && blend_vertical_gradient<blend_color_generic>(count, spans, userData))
            return;
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
        while (--mapHeight >= 0) {
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
        while (--mapHeight >= 0) {
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

static void qt_alphamapblit_generic_oneline(const uchar *map, int len,
                                            const QRgba64 srcColor, QRgba64 *dest,
                                            const QRgba64 color,
                                            const QColorTrcLut *colorProfile)
{
    for (int j = 0; j < len; ++j)
        alphamapblend_generic(map[j], dest, j, srcColor, color, colorProfile);
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

    alignas(8) Q_DECL_UNINITIALIZED QRgba64 buffer[BufferSize];
    const DestFetchProc64 destFetch64 = destFetchProc64[rasterBuffer->format];
    const DestStoreProc64 destStore64 = destStoreProc64[rasterBuffer->format];

    if (!clip) {
        for (int ly = 0; ly < mapHeight; ++ly) {
            int i = x;
            int length = mapWidth;
            while (length > 0) {
                int l = qMin(BufferSize, length);

                QRgba64 *dest = destFetch64(buffer, rasterBuffer, i, y + ly, l);
                qt_alphamapblit_generic_oneline(map + i - x, l,
                                                srcColor, dest, color,
                                                colorProfile);
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
                const QT_FT_Span &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);
                if (end <= start)
                    continue;
                Q_ASSERT(end - start <= BufferSize);
                QRgba64 *dest = destFetch64(buffer, rasterBuffer, start, clip.y, end - start);
                qt_alphamapblit_generic_oneline(map + start - x, end - start,
                                                srcColor, dest, color,
                                                colorProfile);
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
                const QT_FT_Span &clip = line.spans[i];

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
        while (--mapHeight >= 0) {
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
                const QT_FT_Span &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);

                for (int xp=start; xp<end; ++xp)
                    alphamapblend_quint16(map[xp - x], dest, xp, c);
            } // for (i -> line.count)
            map += mapStride;
        } // for (yp -> bottom)
    }
}

static void qt_alphamapblit_argb32_oneline(const uchar *map,
                                           int mapWidth, const QRgba64 &srcColor,
                                           quint32 *dest, const quint32 c,
                                           const QColorTrcLut *colorProfile)
{
    for (int i = 0; i < mapWidth; ++i)
        alphamapblend_argb32(dest + i, map[i], srcColor, c, colorProfile);
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
        while (--mapHeight >= 0) {
            qt_alphamapblit_argb32_oneline(map, mapWidth, srcColor, dest, c, colorProfile);
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
                const QT_FT_Span &clip = line.spans[i];
                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);
                qt_alphamapblit_argb32_oneline(map + start - x, end - start, srcColor, dest + start, c, colorProfile);
            } // for (yp -> bottom)
            map += mapStride;
        }
    }
}

#if QT_CONFIG(raster_64bit)
static void qt_alphamapblit_nonpremul_argb32(QRasterBuffer *rasterBuffer,
                                             int x, int y, const QRgba64 &color,
                                             const uchar *map,
                                             int mapWidth, int mapHeight, int mapStride,
                                             const QClipData *clip, bool useGammaCorrection)
{
    if (clip)
        return qt_alphamapblit_generic(rasterBuffer, x, y, color, map, mapWidth, mapHeight,
                                       mapStride, clip, useGammaCorrection);

    if (color.isTransparent())
        return;

    const QColorTrcLut *colorProfile = nullptr;

    if (useGammaCorrection)
        colorProfile = QGuiApplicationPrivate::instance()->colorProfileForA8Text();

    const quint32 c = color.toArgb32();
    QRgba64 srcColor = color;
    if (colorProfile && color.isOpaque())
        srcColor = colorProfile->toLinear(srcColor);

    alignas(8) Q_DECL_UNINITIALIZED QRgba64 buffer[BufferSize];
    const DestFetchProc64 destFetch64 = destFetchProc64[rasterBuffer->format];
    const DestStoreProc64 destStore64 = destStoreProc64[rasterBuffer->format];

    for (int ly = 0; ly < mapHeight; ++ly) {
        bool dstFullyOpaque = true;
        int i = x;
        int length = mapWidth;
        while (length > 0) {
            int l = qMin(BufferSize, length);
            quint32 *dest = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y + ly)) + i;
            for (int j = 0; j < l && dstFullyOpaque; ++j)
                dstFullyOpaque = (dest[j] & 0xff000000) == 0xff000000;
            if (dstFullyOpaque) {
                // Use RGB/ARGB32PM optimized version
                qt_alphamapblit_argb32_oneline(map + i - x, l, srcColor, dest, c, colorProfile);
            } else {
                // Use generic version
                QRgba64 *dest64 = destFetch64(buffer, rasterBuffer, i, y + ly, l);
                qt_alphamapblit_generic_oneline(map + i - x, l,
                                                srcColor, dest64, color,
                                                colorProfile);
                if (destStore64)
                    destStore64(rasterBuffer, i, y + ly, dest64, l);
            }
            length -= l;
            i += l;
        }
        map += mapStride;
    }
}
#endif

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
    const QRgba64 dlinear = colorProfile ? colorProfile->toLinear(dst) : dst;

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

    alignas(8) Q_DECL_UNINITIALIZED QRgba64 buffer[BufferSize];
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
                const QT_FT_Span &clip = line.spans[i];

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

    Q_DECL_UNINITIALIZED quint32 buffer[BufferSize];
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
                const QT_FT_Span &clip = line.spans[i];

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
        while (--mapHeight >= 0) {
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
                const QT_FT_Span &clip = line.spans[i];

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

static void qt_rectfill_fp32x4(QRasterBuffer *rasterBuffer,
                               int x, int y, int width, int height,
                               const QRgba64 &color)
{
    const auto store = qStoreFromRGBA64PM[rasterBuffer->format];
    QRgbaFloat32 c;
    store(reinterpret_cast<uchar *>(&c), &color, 0, 1, nullptr, nullptr);
    qt_rectfill<QRgbaFloat32>(reinterpret_cast<QRgbaFloat32 *>(rasterBuffer->buffer()),
                          c, x, y, width, height, rasterBuffer->bytesPerLine());
}

// Map table for destination image format. Contains function pointers
// for blends of various types unto the destination

DrawHelper qDrawHelper[] =
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
#if QT_CONFIG(raster_64bit)
        qt_alphamapblit_nonpremul_argb32,
#else
        qt_alphamapblit_generic,
#endif
        qt_alphargbblit_generic,
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
        blend_color_generic,
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
    // Format_RGBX16FPx4
    {
        blend_color_generic_fp,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint64
    },
    // Format_RGBA16FPx4
    {
        blend_color_generic_fp,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint64
    },
    // Format_RGBA16FPx4_Premultiplied
    {
        blend_color_generic_fp,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_quint64
    },
    // Format_RGBX32FPx4
    {
        blend_color_generic_fp,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_fp32x4
    },
    // Format_RGBA32FPx4
    {
        blend_color_generic_fp,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_fp32x4
    },
    // Format_RGBA32FPx4_Premultiplied
    {
        blend_color_generic_fp,
        nullptr,
        qt_alphamapblit_generic,
        qt_alphargbblit_generic,
        qt_rectfill_fp32x4
    },
};

static_assert(std::size(qDrawHelper) == QImage::NImageFormats);

#if !defined(Q_PROCESSOR_X86) && !defined(QT_COMPILER_SUPPORTS_LSX)
void qt_memfill64(quint64 *dest, quint64 color, qsizetype count)
{
    qt_memfill_template<quint64>(dest, color, count);
}
#endif

#if defined(QT_COMPILER_SUPPORTS_SSSE3) && defined(Q_CC_GNU) && !defined(Q_CC_CLANG)
__attribute__((optimize("no-tree-vectorize")))
#endif
void qt_memfill24(quint24 *dest, quint24 color, qsizetype count)
{
#  ifdef QT_COMPILER_SUPPORTS_SSSE3
    extern void qt_memfill24_ssse3(quint24 *, quint24, qsizetype);
    if (qCpuHasFeature(SSSE3))
        return qt_memfill24_ssse3(dest, color, count);
#  elif defined QT_COMPILER_SUPPORTS_LSX
    extern void qt_memfill24_lsx(quint24 *, quint24, qsizetype);
    if (qCpuHasFeature(LSX))
        return qt_memfill24_lsx(dest, color, count);
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

#if defined(Q_PROCESSOR_X86) || defined(QT_COMPILER_SUPPORTS_LSX)
void (*qt_memfill32)(quint32 *dest, quint32 value, qsizetype count) = nullptr;
void (*qt_memfill64)(quint64 *dest, quint64 value, qsizetype count) = nullptr;
#elif !defined(__ARM_NEON__) && !defined(__MIPS_DSP__)
void qt_memfill32(quint32 *dest, quint32 color, qsizetype count)
{
    qt_memfill_template<quint32>(dest, color, count);
}
#endif

#ifdef QT_COMPILER_SUPPORTS_SSE4_1
template<QtPixelOrder> void QT_FASTCALL storeA2RGB30PMFromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
#elif defined(QT_COMPILER_SUPPORTS_LSX)
template<QtPixelOrder> void QT_FASTCALL storeA2RGB30PMFromARGB32PM_lsx(uchar *dest, const uint *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
#endif

extern void qInitBlendFunctions();

static void qInitDrawhelperFunctions()
{
    // Set up basic blend function tables.
    qInitBlendFunctions();

#if defined(Q_PROCESSOR_X86) && !defined(__SSE2__)
    qt_memfill32 = qt_memfill_template<quint32>;
    qt_memfill64 = qt_memfill_template<quint64>;
#elif defined(__SSE2__)
#  ifndef __haswell__
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
        extern void QT_FASTCALL convertARGB32ToARGB32PM_sse4(uint *buffer, int count, const QList<QRgb> *);
        extern void QT_FASTCALL convertRGBA8888ToARGB32PM_sse4(uint *buffer, int count, const QList<QRgb> *);
        extern const uint *QT_FASTCALL fetchARGB32ToARGB32PM_sse4(uint *buffer, const uchar *src, int index, int count,
                                                                  const QList<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_sse4(uint *buffer, const uchar *src, int index, int count,
                                                                    const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_sse4(QRgba64 *buffer, const uint *src, int count,
                                                                        const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_sse4(QRgba64 *buffer, const uint *src, int count,
                                                                          const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_sse4(QRgba64 *buffer, const uchar *src, int index, int count,
                                                                     const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_sse4(QRgba64 *buffer, const uchar *src, int index, int count,
                                                                       const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeARGB32FromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                                                      const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA8888FromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                                                        const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBXFromARGB32PM_sse4(uchar *dest, const uint *src, int index, int count,
                                                                    const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeARGB32FromRGBA64PM_sse4(uchar *dest, const QRgba64 *src, int index, int count,
                                                             const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA8888FromRGBA64PM_sse4(uchar *dest, const QRgba64 *src, int index, int count,
                                                              const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA64FromRGBA64PM_sse4(uchar *, const QRgba64 *, int, int, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBx64FromRGBA64PM_sse4(uchar *, const QRgba64 *, int, int, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL destStore64ARGB32_sse4(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length);
        extern void QT_FASTCALL destStore64RGBA8888_sse4(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length);
#  ifndef __haswell__
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
        qStoreFromRGBA64PM[QImage::Format_RGBX64] = storeRGBx64FromRGBA64PM_sse4;
        qStoreFromRGBA64PM[QImage::Format_RGBA64] = storeRGBA64FromRGBA64PM_sse4;
#if QT_CONFIG(raster_64bit)
        destStoreProc64[QImage::Format_ARGB32] = destStore64ARGB32_sse4;
        destStoreProc64[QImage::Format_RGBA8888] = destStore64RGBA8888_sse4;
#endif
#if QT_CONFIG(raster_fp)
        extern const QRgbaFloat32 *QT_FASTCALL fetchRGBA32FToRGBA32F_sse4(QRgbaFloat32 *buffer, const uchar *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBX32FFromRGBA32F_sse4(uchar *dest, const QRgbaFloat32 *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA32FFromRGBA32F_sse4(uchar *dest, const QRgbaFloat32 *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        qFetchToRGBA32F[QImage::Format_RGBA32FPx4] = fetchRGBA32FToRGBA32F_sse4;
        qStoreFromRGBA32F[QImage::Format_RGBX32FPx4] = storeRGBX32FFromRGBA32F_sse4;
        qStoreFromRGBA32F[QImage::Format_RGBA32FPx4] = storeRGBA32FFromRGBA32F_sse4;
#endif // QT_CONFIG(raster_fp)
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
#if QT_CONFIG(raster_fp)
        extern void QT_FASTCALL comp_func_Source_rgbafp_avx2(QRgbaFloat32 *destPixels, const QRgbaFloat32 *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_SourceOver_rgbafp_avx2(QRgbaFloat32 *destPixels, const QRgbaFloat32 *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_solid_Source_rgbafp_avx2(QRgbaFloat32 *destPixels, int length, QRgbaFloat32 color, uint const_alpha);
        extern void QT_FASTCALL comp_func_solid_SourceOver_rgbafp_avx2(QRgbaFloat32 *destPixels, int length, QRgbaFloat32 color, uint const_alpha);
        qt_functionForModeFP_C[QPainter::CompositionMode_Source] = comp_func_Source_rgbafp_avx2;
        qt_functionForModeFP_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_rgbafp_avx2;
        qt_functionForModeSolidFP_C[QPainter::CompositionMode_Source] = comp_func_solid_Source_rgbafp_avx2;
        qt_functionForModeSolidFP_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_rgbafp_avx2;
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

        extern void QT_FASTCALL convertARGB32ToARGB32PM_avx2(uint *buffer, int count, const QList<QRgb> *);
        extern void QT_FASTCALL convertRGBA8888ToARGB32PM_avx2(uint *buffer, int count, const QList<QRgb> *);
        extern const uint *QT_FASTCALL fetchARGB32ToARGB32PM_avx2(uint *buffer, const uchar *src, int index, int count,
                                                                  const QList<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_avx2(uint *buffer, const uchar *src, int index, int count,
                                                                    const QList<QRgb> *, QDitherInfo *);
        qPixelLayouts[QImage::Format_ARGB32].fetchToARGB32PM = fetchARGB32ToARGB32PM_avx2;
        qPixelLayouts[QImage::Format_ARGB32].convertToARGB32PM = convertARGB32ToARGB32PM_avx2;
        qPixelLayouts[QImage::Format_RGBA8888].fetchToARGB32PM = fetchRGBA8888ToARGB32PM_avx2;
        qPixelLayouts[QImage::Format_RGBA8888].convertToARGB32PM = convertRGBA8888ToARGB32PM_avx2;

        extern const QRgba64 *QT_FASTCALL convertARGB32ToRGBA64PM_avx2(QRgba64 *, const uint *, int, const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL convertRGBA8888ToRGBA64PM_avx2(QRgba64 *, const uint *, int count, const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_avx2(QRgba64 *, const uchar *, int, int, const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_avx2(QRgba64 *, const uchar *, int, int, const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchRGBA64ToRGBA64PM_avx2(QRgba64 *buffer, const uchar *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        qPixelLayouts[QImage::Format_ARGB32].convertToRGBA64PM = convertARGB32ToRGBA64PM_avx2;
        qPixelLayouts[QImage::Format_RGBX8888].convertToRGBA64PM = convertRGBA8888ToRGBA64PM_avx2;
        qPixelLayouts[QImage::Format_ARGB32].fetchToRGBA64PM = fetchARGB32ToRGBA64PM_avx2;
        qPixelLayouts[QImage::Format_RGBX8888].fetchToRGBA64PM = fetchRGBA8888ToRGBA64PM_avx2;
        qPixelLayouts[QImage::Format_RGBA64].fetchToRGBA64PM = fetchRGBA64ToRGBA64PM_avx2;

        extern const uint *QT_FASTCALL fetchRGB16FToRGB32_avx2(uint *buffer, const uchar *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL fetchRGBA16FToARGB32PM_avx2(uint *buffer, const uchar *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchRGBA16FPMToRGBA64PM_avx2(QRgba64 *buffer, const uchar *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchRGBA16FToRGBA64PM_avx2(QRgba64 *buffer, const uchar *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGB16FFromRGB32_avx2(uchar *dest, const uint *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA16FFromARGB32PM_avx2(uchar *dest, const uint *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        qPixelLayouts[QImage::Format_RGBX16FPx4].fetchToARGB32PM = fetchRGB16FToRGB32_avx2;
        qPixelLayouts[QImage::Format_RGBX16FPx4].fetchToRGBA64PM = fetchRGBA16FPMToRGBA64PM_avx2;
        qPixelLayouts[QImage::Format_RGBX16FPx4].storeFromARGB32PM = storeRGB16FFromRGB32_avx2;
        qPixelLayouts[QImage::Format_RGBX16FPx4].storeFromRGB32 = storeRGB16FFromRGB32_avx2;
        qPixelLayouts[QImage::Format_RGBA16FPx4].fetchToARGB32PM = fetchRGBA16FToARGB32PM_avx2;
        qPixelLayouts[QImage::Format_RGBA16FPx4].fetchToRGBA64PM = fetchRGBA16FToRGBA64PM_avx2;
        qPixelLayouts[QImage::Format_RGBA16FPx4].storeFromARGB32PM = storeRGBA16FFromARGB32PM_avx2;
        qPixelLayouts[QImage::Format_RGBA16FPx4].storeFromRGB32 = storeRGB16FFromRGB32_avx2;
        qPixelLayouts[QImage::Format_RGBA16FPx4_Premultiplied].fetchToARGB32PM = fetchRGB16FToRGB32_avx2;
        qPixelLayouts[QImage::Format_RGBA16FPx4_Premultiplied].fetchToRGBA64PM = fetchRGBA16FPMToRGBA64PM_avx2;
        qPixelLayouts[QImage::Format_RGBA16FPx4_Premultiplied].storeFromARGB32PM = storeRGB16FFromRGB32_avx2;
        qPixelLayouts[QImage::Format_RGBA16FPx4_Premultiplied].storeFromRGB32 = storeRGB16FFromRGB32_avx2;
#if QT_CONFIG(raster_fp)
        extern const QRgbaFloat32 *QT_FASTCALL fetchRGBA16FToRGBA32F_avx2(QRgbaFloat32 *buffer, const uchar *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBX16FFromRGBA32F_avx2(uchar *dest, const QRgbaFloat32 *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA16FFromRGBA32F_avx2(uchar *dest, const QRgbaFloat32 *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        qFetchToRGBA32F[QImage::Format_RGBA16FPx4] = fetchRGBA16FToRGBA32F_avx2;
        qStoreFromRGBA32F[QImage::Format_RGBX16FPx4] = storeRGBX16FFromRGBA32F_avx2;
        qStoreFromRGBA32F[QImage::Format_RGBA16FPx4] = storeRGBA16FFromRGBA32F_avx2;
#endif // QT_CONFIG(raster_fp)
    }

#endif

#endif // SSE2

#if defined(QT_COMPILER_SUPPORTS_LSX)
    if (qCpuHasFeature(LSX)) {
        qt_memfill32 = qt_memfill32_lsx;
        qt_memfill64 = qt_memfill64_lsx;

        qDrawHelper[QImage::Format_RGB32].bitmapBlit = qt_bitmapblit32_lsx;
        qDrawHelper[QImage::Format_ARGB32].bitmapBlit = qt_bitmapblit32_lsx;
        qDrawHelper[QImage::Format_ARGB32_Premultiplied].bitmapBlit = qt_bitmapblit32_lsx;
        qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_lsx;
        qDrawHelper[QImage::Format_RGBX8888].bitmapBlit = qt_bitmapblit8888_lsx;
        qDrawHelper[QImage::Format_RGBA8888].bitmapBlit = qt_bitmapblit8888_lsx;
        qDrawHelper[QImage::Format_RGBA8888_Premultiplied].bitmapBlit = qt_bitmapblit8888_lsx;

        extern void qt_scale_image_argb32_on_argb32_lsx(uchar *destPixels, int dbpl,
                                                        const uchar *srcPixels, int sbpl, int srch,
                                                        const QRectF &targetRect,
                                                        const QRectF &sourceRect,
                                                        const QRect &clip,
                                                        int const_alpha);

        qScaleFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_scale_image_argb32_on_argb32_lsx;
        qScaleFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_scale_image_argb32_on_argb32_lsx;
        qScaleFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_scale_image_argb32_on_argb32_lsx;
        qScaleFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_scale_image_argb32_on_argb32_lsx;

        extern void qt_blend_rgb32_on_rgb32_lsx(uchar *destPixels, int dbpl,
                                                const uchar *srcPixels, int sbpl,
                                                int w, int h,
                                                int const_alpha);

        extern void qt_blend_argb32_on_argb32_lsx(uchar *destPixels, int dbpl,
                                                  const uchar *srcPixels, int sbpl,
                                                  int w, int h,
                                                  int const_alpha);

        qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_lsx;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_lsx;
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_lsx;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_lsx;
        qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_lsx;
        qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_lsx;
        qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_lsx;
        qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_lsx;

        extern const uint * QT_FASTCALL qt_fetch_radial_gradient_lsx(uint *buffer, const Operator *op, const QSpanData *data,
                                                                     int y, int x, int length);

        qt_fetch_radial_gradient = qt_fetch_radial_gradient_lsx;

        extern void QT_FASTCALL comp_func_SourceOver_lsx(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_solid_SourceOver_lsx(uint *destPixels, int length, uint color, uint const_alpha);
        extern void QT_FASTCALL comp_func_Source_lsx(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_solid_Source_lsx(uint *destPixels, int length, uint color, uint const_alpha);
        extern void QT_FASTCALL comp_func_Plus_lsx(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
        qt_functionForMode_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_lsx;
        qt_functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_lsx;
        qt_functionForMode_C[QPainter::CompositionMode_Source] = comp_func_Source_lsx;
        qt_functionForModeSolid_C[QPainter::CompositionMode_Source] = comp_func_solid_Source_lsx;
        qt_functionForMode_C[QPainter::CompositionMode_Plus] = comp_func_Plus_lsx;

        extern const uint * QT_FASTCALL qt_fetchUntransformed_888_lsx(uint *buffer, const Operator *, const QSpanData *data,
                                                                      int y, int x, int length);
        sourceFetchUntransformed[QImage::Format_RGB888] = qt_fetchUntransformed_888_lsx;
        extern void QT_FASTCALL rbSwap_888_lsx(uchar *dst, const uchar *src, int count);
        qPixelLayouts[QImage::Format_RGB888].rbSwap = rbSwap_888_lsx;
        qPixelLayouts[QImage::Format_BGR888].rbSwap = rbSwap_888_lsx;

        extern void QT_FASTCALL convertARGB32ToARGB32PM_lsx(uint *buffer, int count, const QList<QRgb> *);
        extern void QT_FASTCALL convertRGBA8888ToARGB32PM_lsx(uint *buffer, int count, const QList<QRgb> *);
        extern const uint *QT_FASTCALL fetchARGB32ToARGB32PM_lsx(uint *buffer, const uchar *src, int index, int count,
                                                                 const QList<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_lsx(uint *buffer, const uchar *src, int index, int count,
                                                                   const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_lsx(QRgba64 *buffer, const uint *src, int count,
                                                                       const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_lsx(QRgba64 *buffer, const uint *src, int count,
                                                                         const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_lsx(QRgba64 *buffer, const uchar *src, int index, int count,
                                                                    const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_lsx(QRgba64 *buffer, const uchar *src, int index, int count,
                                                                      const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeARGB32FromARGB32PM_lsx(uchar *dest, const uint *src, int index, int count,
                                                            const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA8888FromARGB32PM_lsx(uchar *dest, const uint *src, int index, int count,
                                                              const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBXFromARGB32PM_lsx(uchar *dest, const uint *src, int index, int count,
                                                          const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeARGB32FromRGBA64PM_lsx(uchar *dest, const QRgba64 *src, int index, int count,
                                                            const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA8888FromRGBA64PM_lsx(uchar *dest, const QRgba64 *src, int index, int count,
                                                              const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL destStore64ARGB32_lsx(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length);
        extern void QT_FASTCALL destStore64RGBA8888_lsx(QRasterBuffer *rasterBuffer, int x, int y, const QRgba64 *buffer, int length);
        extern void QT_FASTCALL storeRGBA64FromRGBA64PM_lsx(uchar *, const QRgba64 *, int, int, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBx64FromRGBA64PM_lsx(uchar *, const QRgba64 *, int, int, const QList<QRgb> *, QDitherInfo *);
        qPixelLayouts[QImage::Format_ARGB32].fetchToARGB32PM = fetchARGB32ToARGB32PM_lsx;
        qPixelLayouts[QImage::Format_ARGB32].convertToARGB32PM = convertARGB32ToARGB32PM_lsx;
        qPixelLayouts[QImage::Format_RGBA8888].fetchToARGB32PM = fetchRGBA8888ToARGB32PM_lsx;
        qPixelLayouts[QImage::Format_RGBA8888].convertToARGB32PM = convertRGBA8888ToARGB32PM_lsx;
        qPixelLayouts[QImage::Format_ARGB32].fetchToRGBA64PM = fetchARGB32ToRGBA64PM_lsx;
        qPixelLayouts[QImage::Format_ARGB32].convertToRGBA64PM = convertARGB32ToRGBA64PM_lsx;
        qPixelLayouts[QImage::Format_RGBA8888].fetchToRGBA64PM = fetchRGBA8888ToRGBA64PM_lsx;
        qPixelLayouts[QImage::Format_RGBA8888].convertToRGBA64PM = convertRGBA8888ToRGBA64PM_lsx;
        qPixelLayouts[QImage::Format_RGBX8888].fetchToRGBA64PM = fetchRGBA8888ToRGBA64PM_lsx;
        qPixelLayouts[QImage::Format_RGBX8888].convertToRGBA64PM = convertRGBA8888ToRGBA64PM_lsx;
        qPixelLayouts[QImage::Format_ARGB32].storeFromARGB32PM = storeARGB32FromARGB32PM_lsx;
        qPixelLayouts[QImage::Format_RGBA8888].storeFromARGB32PM = storeRGBA8888FromARGB32PM_lsx;
        qPixelLayouts[QImage::Format_RGBX8888].storeFromARGB32PM = storeRGBXFromARGB32PM_lsx;
        qPixelLayouts[QImage::Format_A2BGR30_Premultiplied].storeFromARGB32PM = storeA2RGB30PMFromARGB32PM_lsx<PixelOrderBGR>;
        qPixelLayouts[QImage::Format_A2RGB30_Premultiplied].storeFromARGB32PM = storeA2RGB30PMFromARGB32PM_lsx<PixelOrderRGB>;
        qStoreFromRGBA64PM[QImage::Format_ARGB32] = storeARGB32FromRGBA64PM_lsx;
        qStoreFromRGBA64PM[QImage::Format_RGBA8888] = storeRGBA8888FromRGBA64PM_lsx;
        qStoreFromRGBA64PM[QImage::Format_RGBX64] = storeRGBx64FromRGBA64PM_lsx;
        qStoreFromRGBA64PM[QImage::Format_RGBA64] = storeRGBA64FromRGBA64PM_lsx;
#if QT_CONFIG(raster_64bit)
        destStoreProc64[QImage::Format_ARGB32] = destStore64ARGB32_lsx;
        destStoreProc64[QImage::Format_RGBA8888] = destStore64RGBA8888_lsx;
#endif
#if QT_CONFIG(raster_fp)
        extern const QRgbaFloat32 *QT_FASTCALL fetchRGBA32FToRGBA32F_lsx(QRgbaFloat32 *buffer, const uchar *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBX32FFromRGBA32F_lsx(uchar *dest, const QRgbaFloat32 *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        extern void QT_FASTCALL storeRGBA32FFromRGBA32F_lsx(uchar *dest, const QRgbaFloat32 *src, int index, int count, const QList<QRgb> *, QDitherInfo *);
        qFetchToRGBA32F[QImage::Format_RGBA32FPx4] = fetchRGBA32FToRGBA32F_lsx;
        qStoreFromRGBA32F[QImage::Format_RGBX32FPx4] = storeRGBX32FFromRGBA32F_lsx;
        qStoreFromRGBA32F[QImage::Format_RGBA32FPx4] = storeRGBA32FFromRGBA32F_lsx;
#endif // QT_CONFIG(raster_fp)
    }

#if defined(QT_COMPILER_SUPPORTS_LASX)
    if (qCpuHasFeature(LASX)) {
        qt_memfill32 = qt_memfill32_lasx;
        qt_memfill64 = qt_memfill64_lasx;

        extern void qt_blend_rgb32_on_rgb32_lasx(uchar *destPixels, int dbpl,
                                                 const uchar *srcPixels, int sbpl,
                                                 int w, int h, int const_alpha);
        extern void qt_blend_argb32_on_argb32_lasx(uchar *destPixels, int dbpl,
                                                   const uchar *srcPixels, int sbpl,
                                                   int w, int h, int const_alpha);
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_lasx;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_lasx;
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_lasx;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_lasx;
        qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_lasx;
        qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_lasx;
        qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_lasx;
        qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_lasx;

        extern void QT_FASTCALL comp_func_Source_lasx(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_SourceOver_lasx(uint *destPixels, const uint *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_solid_SourceOver_lasx(uint *destPixels, int length, uint color, uint const_alpha);
        qt_functionForMode_C[QPainter::CompositionMode_Source] = comp_func_Source_lasx;
        qt_functionForMode_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_lasx;
        qt_functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_lasx;
#if QT_CONFIG(raster_64bit)
        extern void QT_FASTCALL comp_func_Source_rgb64_lasx(QRgba64 *destPixels, const QRgba64 *srcPixels, int length, uint const_alpha);
        extern void QT_FASTCALL comp_func_solid_SourceOver_rgb64_lasx(QRgba64 *destPixels, int length, QRgba64 color, uint const_alpha);
        qt_functionForMode64_C[QPainter::CompositionMode_Source] = comp_func_Source_rgb64_lasx;
        qt_functionForModeSolid64_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_rgb64_lasx;
#endif

        extern void QT_FASTCALL fetchTransformedBilinearARGB32PM_simple_scale_helper_lasx(uint *b, uint *end, const QTextureData &image,
                                                                                          int &fx, int &fy, int fdx, int /*fdy*/);
        extern void QT_FASTCALL fetchTransformedBilinearARGB32PM_downscale_helper_lasx(uint *b, uint *end, const QTextureData &image,
                                                                                       int &fx, int &fy, int fdx, int /*fdy*/);
        extern void QT_FASTCALL fetchTransformedBilinearARGB32PM_fast_rotate_helper_lasx(uint *b, uint *end, const QTextureData &image,
                                                                                         int &fx, int &fy, int fdx, int fdy);

        bilinearFastTransformHelperARGB32PM[0][SimpleScaleTransform] = fetchTransformedBilinearARGB32PM_simple_scale_helper_lasx;
        bilinearFastTransformHelperARGB32PM[0][DownscaleTransform] = fetchTransformedBilinearARGB32PM_downscale_helper_lasx;
        bilinearFastTransformHelperARGB32PM[0][FastRotateTransform] = fetchTransformedBilinearARGB32PM_fast_rotate_helper_lasx;

        extern void QT_FASTCALL convertARGB32ToARGB32PM_lasx(uint *buffer, int count, const QList<QRgb> *);
        extern void QT_FASTCALL convertRGBA8888ToARGB32PM_lasx(uint *buffer, int count, const QList<QRgb> *);
        extern const uint *QT_FASTCALL fetchARGB32ToARGB32PM_lasx(uint *buffer, const uchar *src, int index, int count,
                                                                  const QList<QRgb> *, QDitherInfo *);
        extern const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_lasx(uint *buffer, const uchar *src, int index, int count,
                                                                    const QList<QRgb> *, QDitherInfo *);
        qPixelLayouts[QImage::Format_ARGB32].fetchToARGB32PM = fetchARGB32ToARGB32PM_lasx;
        qPixelLayouts[QImage::Format_ARGB32].convertToARGB32PM = convertARGB32ToARGB32PM_lasx;
        qPixelLayouts[QImage::Format_RGBA8888].fetchToARGB32PM = fetchRGBA8888ToARGB32PM_lasx;
        qPixelLayouts[QImage::Format_RGBA8888].convertToARGB32PM = convertRGBA8888ToARGB32PM_lasx;

        extern const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_lasx(QRgba64 *, const uint *, int, const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_lasx(QRgba64 *, const uint *, int count, const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_lasx(QRgba64 *, const uchar *, int, int, const QList<QRgb> *, QDitherInfo *);
        extern const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_lasx(QRgba64 *, const uchar *, int, int, const QList<QRgb> *, QDitherInfo *);
        qPixelLayouts[QImage::Format_ARGB32].convertToRGBA64PM = convertARGB32ToRGBA64PM_lasx;
        qPixelLayouts[QImage::Format_RGBX8888].convertToRGBA64PM = convertRGBA8888ToRGBA64PM_lasx;
        qPixelLayouts[QImage::Format_ARGB32].fetchToRGBA64PM = fetchARGB32ToRGBA64PM_lasx;
        qPixelLayouts[QImage::Format_RGBX8888].fetchToRGBA64PM = fetchRGBA8888ToRGBA64PM_lasx;
    }
#endif  //QT_COMPILER_SUPPORTS_LASX

#endif //QT_COMPILER_SUPPORTS_LSX

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
    extern void QT_FASTCALL convertARGB32ToARGB32PM_neon(uint *buffer, int count, const QList<QRgb> *);
    extern void QT_FASTCALL convertRGBA8888ToARGB32PM_neon(uint *buffer, int count, const QList<QRgb> *);
    extern const uint *QT_FASTCALL fetchARGB32ToARGB32PM_neon(uint *buffer, const uchar *src, int index, int count,
                                                              const QList<QRgb> *, QDitherInfo *);
    extern const uint *QT_FASTCALL fetchRGBA8888ToARGB32PM_neon(uint *buffer, const uchar *src, int index, int count,
                                                                const QList<QRgb> *, QDitherInfo *);
   extern const QRgba64 * QT_FASTCALL convertARGB32ToRGBA64PM_neon(QRgba64 *buffer, const uint *src, int count,
                                                                   const QList<QRgb> *, QDitherInfo *);
   extern const QRgba64 * QT_FASTCALL convertRGBA8888ToRGBA64PM_neon(QRgba64 *buffer, const uint *src, int count,
                                                                     const QList<QRgb> *, QDitherInfo *);
   extern const QRgba64 *QT_FASTCALL fetchARGB32ToRGBA64PM_neon(QRgba64 *buffer, const uchar *src, int index, int count,
                                                                const QList<QRgb> *, QDitherInfo *);
   extern const QRgba64 *QT_FASTCALL fetchRGBA8888ToRGBA64PM_neon(QRgba64 *buffer, const uchar *src, int index, int count,
                                                                  const QList<QRgb> *, QDitherInfo *);
    extern void QT_FASTCALL storeARGB32FromARGB32PM_neon(uchar *dest, const uint *src, int index, int count,
                                                         const QList<QRgb> *, QDitherInfo *);
    extern void QT_FASTCALL storeRGBA8888FromARGB32PM_neon(uchar *dest, const uint *src, int index, int count,
                                                           const QList<QRgb> *, QDitherInfo *);
    extern void QT_FASTCALL storeRGBXFromARGB32PM_neon(uchar *dest, const uint *src, int index, int count,
                                                       const QList<QRgb> *, QDitherInfo *);
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
