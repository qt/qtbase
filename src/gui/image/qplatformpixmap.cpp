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

#include "qplatformpixmap.h"
#include <qpa/qplatformintegration.h>
#include <QtCore/qbuffer.h>
#include <QtGui/qbitmap.h>
#include <QtGui/qimagereader.h>
#include <private/qguiapplication_p.h>
#include <private/qimagepixmapcleanuphooks_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformPixmap
    \since 5.0
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformPixmap class provides an abstraction for native pixmaps.
 */
QPlatformPixmap *QPlatformPixmap::create(int w, int h, PixelType type)
{
    if (Q_UNLIKELY(!QGuiApplicationPrivate::platformIntegration()))
        qFatal("QPlatformPixmap: QGuiApplication required");

    QPlatformPixmap *data = QGuiApplicationPrivate::platformIntegration()->createPlatformPixmap(static_cast<QPlatformPixmap::PixelType>(type));
    data->resize(w, h);
    return data;
}


QPlatformPixmap::QPlatformPixmap(PixelType pixelType, int objectId)
    : w(0),
      h(0),
      d(0),
      is_null(true),
      ref(0),
      detach_no(0),
      type(pixelType),
      id(objectId),
      ser_no(0),
      is_cached(false)
{
}

QPlatformPixmap::~QPlatformPixmap()
{
    // Sometimes the pixmap cleanup hooks will be called from derrived classes, which will
    // then set is_cached to false. For example, on X11 Qt GUI needs to delete the GLXPixmap
    // or EGL Pixmap Surface for a given pixmap _before_ the native X11 pixmap is deleted,
    // otherwise some drivers will leak the GL surface. In this case, QX11PlatformPixmap will
    // call the cleanup hooks itself before deleting the native pixmap and set is_cached to
    // false.
    if (is_cached) {
        QImagePixmapCleanupHooks::executePlatformPixmapDestructionHooks(this);
        is_cached = false;
    }
}

QPlatformPixmap *QPlatformPixmap::createCompatiblePlatformPixmap() const
{
    QPlatformPixmap *d = QGuiApplicationPrivate::platformIntegration()->createPlatformPixmap(pixelType());
    return d;
}

static QImage makeBitmapCompliantIfNeeded(QPlatformPixmap *d, const QImage &image, Qt::ImageConversionFlags flags)
{
    if (d->pixelType() == QPlatformPixmap::BitmapType) {
        QImage img = image.convertToFormat(QImage::Format_MonoLSB, flags);

        // make sure image.color(0) == Qt::color0 (white)
        // and image.color(1) == Qt::color1 (black)
        const QRgb c0 = QColor(Qt::black).rgb();
        const QRgb c1 = QColor(Qt::white).rgb();
        if (img.color(0) == c0 && img.color(1) == c1) {
            img.invertPixels();
            img.setColor(0, c1);
            img.setColor(1, c0);
        }
        return img;
    }

    return image;
}

void QPlatformPixmap::fromImageReader(QImageReader *imageReader,
                                  Qt::ImageConversionFlags flags)
{
    const QImage image = imageReader->read();
    fromImage(image, flags);
}

bool QPlatformPixmap::fromFile(const QString &fileName, const char *format,
                           Qt::ImageConversionFlags flags)
{
    QImage image = QImageReader(fileName, format).read();
    if (image.isNull())
        return false;
    fromImage(makeBitmapCompliantIfNeeded(this, image, flags), flags);
    return !isNull();
}

bool QPlatformPixmap::fromData(const uchar *buf, uint len, const char *format, Qt::ImageConversionFlags flags)
{
    QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(buf), len);
    QBuffer b(&a);
    b.open(QIODevice::ReadOnly);
    QImage image = QImageReader(&b, format).read();
    if (image.isNull())
        return false;
    fromImage(makeBitmapCompliantIfNeeded(this, image, flags), flags);
    return !isNull();
}

void QPlatformPixmap::copy(const QPlatformPixmap *data, const QRect &rect)
{
    fromImage(data->toImage(rect), Qt::NoOpaqueDetection);
}

bool QPlatformPixmap::scroll(int dx, int dy, const QRect &rect)
{
    Q_UNUSED(dx);
    Q_UNUSED(dy);
    Q_UNUSED(rect);
    return false;
}

QBitmap QPlatformPixmap::mask() const
{
    if (!hasAlphaChannel())
        return QBitmap();

    const QImage img = toImage();
    bool shouldConvert = (img.format() != QImage::Format_ARGB32 && img.format() != QImage::Format_ARGB32_Premultiplied);
    const QImage image = (shouldConvert ? img.convertToFormat(QImage::Format_ARGB32_Premultiplied) : img);
    const int w = image.width();
    const int h = image.height();

    QImage mask(w, h, QImage::Format_MonoLSB);
    if (mask.isNull()) // allocation failed
        return QBitmap();

    mask.setDevicePixelRatio(devicePixelRatio());
    mask.setColorCount(2);
    mask.setColor(0, QColor(Qt::color0).rgba());
    mask.setColor(1, QColor(Qt::color1).rgba());

    const int bpl = mask.bytesPerLine();

    for (int y = 0; y < h; ++y) {
        const QRgb *src = reinterpret_cast<const QRgb*>(image.scanLine(y));
        uchar *dest = mask.scanLine(y);
        memset(dest, 0, bpl);
        for (int x = 0; x < w; ++x) {
            if (qAlpha(*src) > 0)
                dest[x >> 3] |= (1 << (x & 7));
            ++src;
        }
    }

    return QBitmap::fromImage(mask);
}

void QPlatformPixmap::setMask(const QBitmap &mask)
{
    QImage image = toImage();
    if (mask.size().isEmpty()) {
        if (image.depth() != 1) { // hw: ????
            image = image.convertToFormat(QImage::Format_RGB32);
        }
    } else {
        const int w = image.width();
        const int h = image.height();

        switch (image.depth()) {
        case 1: {
            const QImage imageMask = mask.toImage().convertToFormat(image.format());
            for (int y = 0; y < h; ++y) {
                const uchar *mscan = imageMask.scanLine(y);
                uchar *tscan = image.scanLine(y);
                int bytesPerLine = image.bytesPerLine();
                for (int i = 0; i < bytesPerLine; ++i)
                    tscan[i] &= mscan[i];
            }
            break;
        }
        default: {
            const QImage imageMask = mask.toImage().convertToFormat(QImage::Format_MonoLSB);
            image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
            for (int y = 0; y < h; ++y) {
                const uchar *mscan = imageMask.scanLine(y);
                QRgb *tscan = (QRgb *)image.scanLine(y);
                for (int x = 0; x < w; ++x) {
                    if (!(mscan[x>>3] & (1 << (x&7))))
                        tscan[x] = 0;
                }
            }
            break;
        }
        }
    }
    fromImage(image, Qt::AutoColor);
}

QPixmap QPlatformPixmap::transformed(const QTransform &matrix,
                                     Qt::TransformationMode mode) const
{
    return QPixmap::fromImage(toImage().transformed(matrix, mode));
}

void QPlatformPixmap::setSerialNumber(int serNo)
{
    ser_no = serNo;
}

void QPlatformPixmap::setDetachNumber(int detNo)
{
    detach_no = detNo;
}

QImage QPlatformPixmap::toImage(const QRect &rect) const
{
    if (rect.contains(QRect(0, 0, w, h)))
        return toImage();
    else
        return toImage().copy(rect);
}

QImage* QPlatformPixmap::buffer()
{
    return nullptr;
}


QT_END_NAMESPACE
