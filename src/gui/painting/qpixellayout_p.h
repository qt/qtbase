/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QPIXELLAYOUT_P_H
#define QPIXELLAYOUT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qlist.h>
#include <QtGui/qimage.h>
#include <QtGui/qrgba64.h>

QT_BEGIN_NAMESPACE

enum QtPixelOrder {
    PixelOrderRGB,
    PixelOrderBGR
};

template<enum QtPixelOrder> inline uint qConvertArgb32ToA2rgb30(QRgb);

template<enum QtPixelOrder> inline uint qConvertRgb32ToRgb30(QRgb);

template<enum QtPixelOrder> inline QRgb qConvertA2rgb30ToArgb32(uint c);

// A combined unpremultiply and premultiply with new simplified alpha.
// Needed when alpha loses precision relative to other colors during conversion (ARGB32 -> A2RGB30).
template<unsigned int Shift>
inline QRgb qRepremultiply(QRgb p)
{
    const uint alpha = qAlpha(p);
    if (alpha == 255 || alpha == 0)
        return p;
    p = qUnpremultiply(p);
    constexpr  uint mult = 255 / (255 >> Shift);
    const uint newAlpha = mult * (alpha >> Shift);
    p = (p & ~0xff000000) | (newAlpha<<24);
    return qPremultiply(p);
}

template<unsigned int Shift>
inline QRgba64 qRepremultiply(QRgba64 p)
{
    const uint alpha = p.alpha();
    if (alpha == 65535 || alpha == 0)
        return p;
    p = p.unpremultiplied();
    constexpr  uint mult = 65535 / (65535 >> Shift);
    p.setAlpha(mult * (alpha >> Shift));
    return p.premultiplied();
}

template<>
inline uint qConvertArgb32ToA2rgb30<PixelOrderBGR>(QRgb c)
{
    c = qRepremultiply<6>(c);
    return (c & 0xc0000000)
        | (((c << 22) & 0x3fc00000) | ((c << 14) & 0x00300000))
        | (((c << 4)  & 0x000ff000) | ((c >> 4)  & 0x00000c00))
        | (((c >> 14) & 0x000003fc) | ((c >> 22) & 0x00000003));
}

template<>
inline uint qConvertArgb32ToA2rgb30<PixelOrderRGB>(QRgb c)
{
    c = qRepremultiply<6>(c);
    return (c & 0xc0000000)
        | (((c << 6) & 0x3fc00000) | ((c >> 2) & 0x00300000))
        | (((c << 4) & 0x000ff000) | ((c >> 4) & 0x00000c00))
        | (((c << 2) & 0x000003fc) | ((c >> 6) & 0x00000003));
}

template<>
inline uint qConvertRgb32ToRgb30<PixelOrderBGR>(QRgb c)
{
    return 0xc0000000
        | (((c << 22) & 0x3fc00000) | ((c << 14) & 0x00300000))
        | (((c << 4)  & 0x000ff000) | ((c >> 4)  & 0x00000c00))
        | (((c >> 14) & 0x000003fc) | ((c >> 22) & 0x00000003));
}

template<>
inline uint qConvertRgb32ToRgb30<PixelOrderRGB>(QRgb c)
{
    return 0xc0000000
        | (((c << 6) & 0x3fc00000) | ((c >> 2) & 0x00300000))
        | (((c << 4) & 0x000ff000) | ((c >> 4) & 0x00000c00))
        | (((c << 2) & 0x000003fc) | ((c >> 6) & 0x00000003));
}

template<>
inline QRgb qConvertA2rgb30ToArgb32<PixelOrderBGR>(uint c)
{
    uint a = c >> 30;
    a |= a << 2;
    a |= a << 4;
    return (a << 24)
        | ((c << 14) & 0x00ff0000)
        | ((c >> 4)  & 0x0000ff00)
        | ((c >> 22) & 0x000000ff);
}

template<>
inline QRgb qConvertA2rgb30ToArgb32<PixelOrderRGB>(uint c)
{
    uint a = c >> 30;
    a |= a << 2;
    a |= a << 4;
    return (a << 24)
        | ((c >> 6) & 0x00ff0000)
        | ((c >> 4) & 0x0000ff00)
        | ((c >> 2) & 0x000000ff);
}

template<enum QtPixelOrder> inline QRgba64 qConvertA2rgb30ToRgb64(uint rgb);

template<>
inline QRgba64 qConvertA2rgb30ToRgb64<PixelOrderBGR>(uint rgb)
{
    quint16 alpha = rgb >> 30;
    quint16 blue  = (rgb >> 20) & 0x3ff;
    quint16 green = (rgb >> 10) & 0x3ff;
    quint16 red   = rgb & 0x3ff;
    // Expand the range.
    alpha |= (alpha << 2);
    alpha |= (alpha << 4);
    alpha |= (alpha << 8);
    red   = (red   << 6) | (red   >> 4);
    green = (green << 6) | (green >> 4);
    blue  = (blue  << 6) | (blue  >> 4);
    return qRgba64(red, green, blue, alpha);
}

template<>
inline QRgba64 qConvertA2rgb30ToRgb64<PixelOrderRGB>(uint rgb)
{
    quint16 alpha = rgb >> 30;
    quint16 red   = (rgb >> 20) & 0x3ff;
    quint16 green = (rgb >> 10) & 0x3ff;
    quint16 blue  = rgb & 0x3ff;
    // Expand the range.
    alpha |= (alpha << 2);
    alpha |= (alpha << 4);
    alpha |= (alpha << 8);
    red   = (red   << 6) | (red   >> 4);
    green = (green << 6) | (green >> 4);
    blue  = (blue  << 6) | (blue  >> 4);
    return qRgba64(red, green, blue, alpha);
}

template<enum QtPixelOrder> inline unsigned int qConvertRgb64ToRgb30(QRgba64);

template<>
inline unsigned int qConvertRgb64ToRgb30<PixelOrderBGR>(QRgba64 c)
{
    c = qRepremultiply<14>(c);
    const uint a = c.alpha() >> 14;
    const uint r = c.red()   >> 6;
    const uint g = c.green() >> 6;
    const uint b = c.blue()  >> 6;
    return (a << 30) | (b << 20) | (g << 10) | r;
}

template<>
inline unsigned int qConvertRgb64ToRgb30<PixelOrderRGB>(QRgba64 c)
{
    c = qRepremultiply<14>(c);
    const uint a = c.alpha() >> 14;
    const uint r = c.red()   >> 6;
    const uint g = c.green() >> 6;
    const uint b = c.blue()  >> 6;
    return (a << 30) | (r << 20) | (g << 10) | b;
}

inline uint qRgbSwapRgb30(uint c)
{
    const uint ag = c & 0xc00ffc00;
    const uint rb = c & 0x3ff003ff;
    return ag | (rb << 20) | (rb >> 20);
}

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
static inline quint32 RGBA2ARGB(quint32 x) {
    quint32 rgb = x >> 8;
    quint32 a = x << 24;
    return a | rgb;
}

static inline quint32 ARGB2RGBA(quint32 x) {
    quint32 rgb = x << 8;
    quint32 a = x >> 24;
    return a | rgb;
}
#else
static inline quint32 RGBA2ARGB(quint32 x) {
    // RGBA8888 is ABGR32 on little endian.
    quint32 ag = x & 0xff00ff00;
    quint32 rg = x & 0x00ff00ff;
    return ag | (rg  << 16) | (rg >> 16);
}

static inline quint32 ARGB2RGBA(quint32 x) {
    return RGBA2ARGB(x);
}
#endif

// We manually unalias the variables to make sure the compiler
// fully optimizes both aliased and unaliased cases.
#define UNALIASED_CONVERSION_LOOP(buffer, src, count, conversion) \
    if (src == buffer) { \
        for (int i = 0; i < count; ++i) \
            buffer[i] = conversion(buffer[i]); \
    } else { \
        for (int i = 0; i < count; ++i) \
            buffer[i] = conversion(src[i]); \
    }


inline const uint *qt_convertARGB32ToARGB32PM(uint *buffer, const uint *src, int count)
{
    UNALIASED_CONVERSION_LOOP(buffer, src, count, qPremultiply);
    return buffer;
}

inline const uint *qt_convertRGBA8888ToARGB32PM(uint *buffer, const uint *src, int count)
{
    UNALIASED_CONVERSION_LOOP(buffer, src, count, [](uint s) { return qPremultiply(RGBA2ARGB(s));});
    return buffer;
}

template<bool RGBA> void qt_convertRGBA64ToARGB32(uint *dst, const QRgba64 *src, int count);

struct QDitherInfo {
    int x;
    int y;
};

typedef const uint *(QT_FASTCALL *FetchAndConvertPixelsFunc)(uint *buffer, const uchar *src,
                                                             int index, int count,
                                                             const QList<QRgb> *clut,
                                                             QDitherInfo *dither);
typedef void(QT_FASTCALL *ConvertAndStorePixelsFunc)(uchar *dest, const uint *src, int index,
                                                     int count, const QList<QRgb> *clut,
                                                     QDitherInfo *dither);

typedef const QRgba64 *(QT_FASTCALL *FetchAndConvertPixelsFunc64)(QRgba64 *buffer, const uchar *src,
                                                                  int index, int count,
                                                                  const QList<QRgb> *clut,
                                                                  QDitherInfo *dither);
typedef void(QT_FASTCALL *ConvertAndStorePixelsFunc64)(uchar *dest, const QRgba64 *src, int index,
                                                       int count, const QList<QRgb> *clut,
                                                       QDitherInfo *dither);

typedef void(QT_FASTCALL *ConvertFunc)(uint *buffer, int count, const QList<QRgb> *clut);
typedef void(QT_FASTCALL *Convert64Func)(quint64 *buffer, int count, const QList<QRgb> *clut);
typedef const QRgba64 *(QT_FASTCALL *ConvertTo64Func)(QRgba64 *buffer, const uint *src, int count,
                                                      const QList<QRgb> *clut, QDitherInfo *dither);
typedef void (QT_FASTCALL *RbSwapFunc)(uchar *dst, const uchar *src, int count);

typedef void (*MemRotateFunc)(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl);

struct QPixelLayout
{
    // Bits per pixel
    enum BPP {
        BPPNone,
        BPP1MSB,
        BPP1LSB,
        BPP8,
        BPP16,
        BPP24,
        BPP32,
        BPP64,
        BPPCount
    };

    bool hasAlphaChannel;
    bool premultiplied;
    BPP bpp;
    RbSwapFunc rbSwap;
    ConvertFunc convertToARGB32PM;
    ConvertTo64Func convertToRGBA64PM;
    FetchAndConvertPixelsFunc fetchToARGB32PM;
    FetchAndConvertPixelsFunc64 fetchToRGBA64PM;
    ConvertAndStorePixelsFunc storeFromARGB32PM;
    ConvertAndStorePixelsFunc storeFromRGB32;
};

extern ConvertAndStorePixelsFunc64 qStoreFromRGBA64PM[QImage::NImageFormats];

extern QPixelLayout qPixelLayouts[QImage::NImageFormats];

extern MemRotateFunc qMemRotateFunctions[QPixelLayout::BPPCount][3];

QT_END_NAMESPACE

#endif // QPIXELLAYOUT_P_H
