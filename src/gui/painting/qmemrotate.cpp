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

#include "private/qmemrotate_p.h"

QT_BEGIN_NAMESPACE

static const int tileSize = 32;

template <class T>
static
inline void qt_memrotate90_tiled(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    sstride /= sizeof(T);
    dstride /= sizeof(T);

    const int pack = sizeof(quint32) / sizeof(T);
    const int unaligned =
        qMin(uint((quintptr(dest) & (sizeof(quint32)-1)) / sizeof(T)), uint(h));
    const int restX = w % tileSize;
    const int restY = (h - unaligned) % tileSize;
    const int unoptimizedY = restY % pack;
    const int numTilesX = w / tileSize + (restX > 0);
    const int numTilesY = (h - unaligned) / tileSize + (restY >= pack);

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = w - tx * tileSize - 1;
        const int stopx = qMax(startx - tileSize, 0);

        if (unaligned) {
            for (int x = startx; x >= stopx; --x) {
                T *d = dest + (w - x - 1) * dstride;
                for (int y = 0; y < unaligned; ++y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = ty * tileSize + unaligned;
            const int stopy = qMin(starty + tileSize, h - unoptimizedY);

            for (int x = startx; x >= stopx; --x) {
                quint32 *d = reinterpret_cast<quint32*>(dest + (w - x - 1) * dstride + starty);
                for (int y = starty; y < stopy; y += pack) {
                    quint32 c = src[y * sstride + x];
                    for (int i = 1; i < pack; ++i) {
                        const int shift = (sizeof(T) * 8 * i);
                        const T color = src[(y + i) * sstride + x];
                        c |= color << shift;
                    }
                    *d++ = c;
                }
            }
        }

        if (unoptimizedY) {
            const int starty = h - unoptimizedY;
            for (int x = startx; x >= stopx; --x) {
                T *d = dest + (w - x - 1) * dstride + starty;
                for (int y = starty; y < h; ++y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }
    }
}

template <class T>
static
inline void qt_memrotate90_tiled_unpacked(const T *src, int w, int h, int sstride, T *dest,
                                          int dstride)
{
    const int numTilesX = (w + tileSize - 1) / tileSize;
    const int numTilesY = (h + tileSize - 1) / tileSize;

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = w - tx * tileSize - 1;
        const int stopx = qMax(startx - tileSize, 0);

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = ty * tileSize;
            const int stopy = qMin(starty + tileSize, h);

            for (int x = startx; x >= stopx; --x) {
                T *d = (T *)((char*)dest + (w - x - 1) * dstride) + starty;
                const char *s = (const char*)(src + x) + starty * sstride;
                for (int y = starty; y < stopy; ++y) {
                    *d++ = *(const T *)(s);
                    s += sstride;
                }
            }
        }
    }
}

template <class T>
static
inline void qt_memrotate270_tiled(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    sstride /= sizeof(T);
    dstride /= sizeof(T);

    const int pack = sizeof(quint32) / sizeof(T);
    const int unaligned =
        qMin(uint((quintptr(dest) & (sizeof(quint32)-1)) / sizeof(T)), uint(h));
    const int restX = w % tileSize;
    const int restY = (h - unaligned) % tileSize;
    const int unoptimizedY = restY % pack;
    const int numTilesX = w / tileSize + (restX > 0);
    const int numTilesY = (h - unaligned) / tileSize + (restY >= pack);

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = tx * tileSize;
        const int stopx = qMin(startx + tileSize, w);

        if (unaligned) {
            for (int x = startx; x < stopx; ++x) {
                T *d = dest + x * dstride;
                for (int y = h - 1; y >= h - unaligned; --y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = h - 1 - unaligned - ty * tileSize;
            const int stopy = qMax(starty - tileSize, unoptimizedY);

            for (int x = startx; x < stopx; ++x) {
                quint32 *d = reinterpret_cast<quint32*>(dest + x * dstride
                                                        + h - 1 - starty);
                for (int y = starty; y >= stopy; y -= pack) {
                    quint32 c = src[y * sstride + x];
                    for (int i = 1; i < pack; ++i) {
                        const int shift = (sizeof(T) * 8 * i);
                        const T color = src[(y - i) * sstride + x];
                        c |= color << shift;
                    }
                    *d++ = c;
                }
            }
        }
        if (unoptimizedY) {
            const int starty = unoptimizedY - 1;
            for (int x = startx; x < stopx; ++x) {
                T *d = dest + x * dstride + h - 1 - starty;
                for (int y = starty; y >= 0; --y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }
    }
}

template <class T>
static
inline void qt_memrotate270_tiled_unpacked(const T *src, int w, int h, int sstride, T *dest,
                                           int dstride)
{
    const int numTilesX = (w + tileSize - 1) / tileSize;
    const int numTilesY = (h + tileSize - 1) / tileSize;

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = tx * tileSize;
        const int stopx = qMin(startx + tileSize, w);

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = h - 1 - ty * tileSize;
            const int stopy = qMax(starty - tileSize, 0);

            for (int x = startx; x < stopx; ++x) {
                T *d = (T*)((char*)dest + x * dstride) + h - 1 - starty;
                const char *s = (const char*)(src + x) + starty * sstride;
                for (int y = starty; y >= stopy; --y) {
                    *d++ = *(const T*)s;
                    s -= sstride;
                }
            }
        }
    }
}


template <class T>
static
inline void qt_memrotate90_template(const T *src, int srcWidth, int srcHeight, int srcStride,
                                    T *dest, int dstStride)
{
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    // packed algorithm assumes little endian and that sizeof(quint32)/sizeof(T) is an integer
    if (sizeof(quint32) % sizeof(T) == 0)
        qt_memrotate90_tiled<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
    else
#endif
    qt_memrotate90_tiled_unpacked<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
}

template <>
inline void qt_memrotate90_template<quint32>(const quint32 *src, int w, int h, int sstride, quint32 *dest, int dstride)
{
    // packed algorithm doesn't have any benefit for quint32
    qt_memrotate90_tiled_unpacked(src, w, h, sstride, dest, dstride);
}

template <>
inline void qt_memrotate90_template<quint64>(const quint64 *src, int w, int h, int sstride, quint64 *dest, int dstride)
{
    qt_memrotate90_tiled_unpacked(src, w, h, sstride, dest, dstride);
}

template <class T>
static
inline void qt_memrotate180_template(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    const char *s = (const char*)(src) + (h - 1) * sstride;
    for (int dy = 0; dy < h; ++dy) {
        T *d = reinterpret_cast<T*>((char *)(dest) + dy * dstride);
        src = reinterpret_cast<const T*>(s);
        for (int dx = 0; dx < w; ++dx) {
            d[dx] = src[w - 1 - dx];
        }
        s -= sstride;
    }
}

template <class T>
static
inline void qt_memrotate270_template(const T *src, int srcWidth, int srcHeight, int srcStride,
                                     T *dest, int dstStride)
{
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    // packed algorithm assumes little endian and that sizeof(quint32)/sizeof(T) is an integer
    if (sizeof(quint32) % sizeof(T) == 0)
        qt_memrotate270_tiled<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
    else
#endif
    qt_memrotate270_tiled_unpacked<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
}

template <>
inline void qt_memrotate270_template<quint32>(const quint32 *src, int w, int h, int sstride, quint32 *dest, int dstride)
{
    // packed algorithm doesn't have any benefit for quint32
    qt_memrotate270_tiled_unpacked(src, w, h, sstride, dest, dstride);
}

template <>
inline void qt_memrotate270_template<quint64>(const quint64 *src, int w, int h, int sstride, quint64 *dest, int dstride)
{
    qt_memrotate270_tiled_unpacked(src, w, h, sstride, dest, dstride);
}

#define QT_IMPL_MEMROTATE(type)                                     \
Q_GUI_EXPORT void qt_memrotate90(const type *src, int w, int h, int sstride, \
                                 type *dest, int dstride)           \
{                                                                   \
    qt_memrotate90_template(src, w, h, sstride, dest, dstride);     \
}                                                                   \
Q_GUI_EXPORT void qt_memrotate180(const type *src, int w, int h, int sstride, \
                                  type *dest, int dstride)          \
{                                                                   \
    qt_memrotate180_template(src, w, h, sstride, dest, dstride);    \
}                                                                   \
Q_GUI_EXPORT void qt_memrotate270(const type *src, int w, int h, int sstride, \
                                  type *dest, int dstride)          \
{                                                                   \
    qt_memrotate270_template(src, w, h, sstride, dest, dstride);    \
}

#define QT_IMPL_SIMPLE_MEMROTATE(type)                              \
Q_GUI_EXPORT void qt_memrotate90(const type *src, int w, int h, int sstride,  \
                                 type *dest, int dstride)           \
{                                                                   \
    qt_memrotate90_tiled_unpacked(src, w, h, sstride, dest, dstride); \
}                                                                   \
Q_GUI_EXPORT void qt_memrotate180(const type *src, int w, int h, int sstride, \
                                  type *dest, int dstride)          \
{                                                                   \
    qt_memrotate180_template(src, w, h, sstride, dest, dstride);    \
}                                                                   \
Q_GUI_EXPORT void qt_memrotate270(const type *src, int w, int h, int sstride, \
                                  type *dest, int dstride)          \
{                                                                   \
    qt_memrotate270_tiled_unpacked(src, w, h, sstride, dest, dstride); \
}

QT_IMPL_MEMROTATE(quint64)
QT_IMPL_MEMROTATE(quint32)
QT_IMPL_MEMROTATE(quint16)
QT_IMPL_MEMROTATE(quint24)
QT_IMPL_MEMROTATE(quint8)

void qt_memrotate90_8(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90(srcPixels, w, h, sbpl, destPixels, dbpl);
}

void qt_memrotate180_8(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180(srcPixels, w, h, sbpl, destPixels, dbpl);
}

void qt_memrotate270_8(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270(srcPixels, w, h, sbpl, destPixels, dbpl);
}

void qt_memrotate90_16(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90((const ushort *)srcPixels, w, h, sbpl, (ushort *)destPixels, dbpl);
}

void qt_memrotate180_16(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180((const ushort *)srcPixels, w, h, sbpl, (ushort *)destPixels, dbpl);
}

void qt_memrotate270_16(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270((const ushort *)srcPixels, w, h, sbpl, (ushort *)destPixels, dbpl);
}

void qt_memrotate90_24(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90((const quint24 *)srcPixels, w, h, sbpl, (quint24 *)destPixels, dbpl);
}

void qt_memrotate180_24(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180((const quint24 *)srcPixels, w, h, sbpl, (quint24 *)destPixels, dbpl);
}

void qt_memrotate270_24(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270((const quint24 *)srcPixels, w, h, sbpl, (quint24 *)destPixels, dbpl);
}

void qt_memrotate90_32(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90((const uint *)srcPixels, w, h, sbpl, (uint *)destPixels, dbpl);
}

void qt_memrotate180_32(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180((const uint *)srcPixels, w, h, sbpl, (uint *)destPixels, dbpl);
}

void qt_memrotate270_32(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270((const uint *)srcPixels, w, h, sbpl, (uint *)destPixels, dbpl);
}


void qt_memrotate90_64(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90((const quint64 *)srcPixels, w, h, sbpl, (quint64 *)destPixels, dbpl);
}

void qt_memrotate180_64(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180((const quint64 *)srcPixels, w, h, sbpl, (quint64 *)destPixels, dbpl);
}

void qt_memrotate270_64(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270((const quint64 *)srcPixels, w, h, sbpl, (quint64 *)destPixels, dbpl);
}

MemRotateFunc qMemRotateFunctions[QPixelLayout::BPPCount][3] =
// 90, 180, 270
{
    { nullptr, nullptr, nullptr },      // BPPNone,
    { nullptr, nullptr, nullptr },      // BPP1MSB,
    { nullptr, nullptr, nullptr },      // BPP1LSB,
    { qt_memrotate90_8, qt_memrotate180_8, qt_memrotate270_8 },         // BPP8,
    { qt_memrotate90_16, qt_memrotate180_16, qt_memrotate270_16 },      // BPP16,
    { qt_memrotate90_24, qt_memrotate180_24, qt_memrotate270_24 },      // BPP24
    { qt_memrotate90_32, qt_memrotate180_32, qt_memrotate270_32 },      // BPP32
    { qt_memrotate90_64, qt_memrotate180_64, qt_memrotate270_64 },      // BPP64
};

QT_END_NAMESPACE
