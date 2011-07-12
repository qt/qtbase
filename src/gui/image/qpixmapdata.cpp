/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpixmapdata_p.h"
#include <QtCore/qbuffer.h>
#include <QtGui/qbitmap.h>
#include <QtGui/qimagereader.h>
#include <private/qguiapplication_p.h>
#include <private/qimagepixmapcleanuphooks_p.h>

QT_BEGIN_NAMESPACE

QPixmapData *QPixmapData::create(int w, int h, PixelType type)
{
    QPixmapData *data = QGuiApplicationPrivate::platformIntegration()->createPixmapData(static_cast<QPixmapData::PixelType>(type));
    data->resize(w, h);
    return data;
}


QPixmapData::QPixmapData(PixelType pixelType, int objectId)
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

QPixmapData::~QPixmapData()
{
    // Sometimes the pixmap cleanup hooks will be called from derrived classes, which will
    // then set is_cached to false. For example, on X11 QtOpenGL needs to delete the GLXPixmap
    // or EGL Pixmap Surface for a given pixmap _before_ the native X11 pixmap is deleted,
    // otherwise some drivers will leak the GL surface. In this case, QX11PixmapData will
    // call the cleanup hooks itself before deleting the native pixmap and set is_cached to
    // false.
    if (is_cached) {
        QImagePixmapCleanupHooks::executePixmapDataDestructionHooks(this);
        is_cached = false;
    }
}

QPixmapData *QPixmapData::createCompatiblePixmapData() const
{
    QPixmapData *d = QGuiApplicationPrivate::platformIntegration()->createPixmapData(pixelType());
    return d;
}

static QImage makeBitmapCompliantIfNeeded(QPixmapData *d, const QImage &image, Qt::ImageConversionFlags flags)
{
    if (d->pixelType() == QPixmapData::BitmapType) {
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

void QPixmapData::fromImageReader(QImageReader *imageReader,
                                  Qt::ImageConversionFlags flags)
{
    const QImage image = imageReader->read();
    fromImage(image, flags);
}

bool QPixmapData::fromFile(const QString &fileName, const char *format,
                           Qt::ImageConversionFlags flags)
{
    QImage image = QImageReader(fileName, format).read();
    if (image.isNull())
        return false;
    fromImage(makeBitmapCompliantIfNeeded(this, image, flags), flags);
    return !isNull();
}

bool QPixmapData::fromData(const uchar *buf, uint len, const char *format, Qt::ImageConversionFlags flags)
{
    QByteArray a = QByteArray::fromRawData(reinterpret_cast<const char *>(buf), len);
    QBuffer b(&a);
    b.open(QIODevice::ReadOnly);
    QImage image = QImageReader(&b, format).read();
    fromImage(makeBitmapCompliantIfNeeded(this, image, flags), flags);
    return !isNull();
}

void QPixmapData::copy(const QPixmapData *data, const QRect &rect)
{
    fromImage(data->toImage(rect), Qt::NoOpaqueDetection);
}

bool QPixmapData::scroll(int dx, int dy, const QRect &rect)
{
    Q_UNUSED(dx);
    Q_UNUSED(dy);
    Q_UNUSED(rect);
    return false;
}

QPixmap QPixmapData::transformed(const QTransform &matrix,
                                 Qt::TransformationMode mode) const
{
    return QPixmap::fromImage(toImage().transformed(matrix, mode));
}

void QPixmapData::setSerialNumber(int serNo)
{
    ser_no = serNo;
}

QImage QPixmapData::toImage(const QRect &rect) const
{
    if (rect.contains(QRect(0, 0, w, h)))
        return toImage();
    else
        return toImage().copy(rect);
}

QImage* QPixmapData::buffer()
{
    return 0;
}

#if defined(Q_OS_SYMBIAN)
void* QPixmapData::toNativeType(NativeType /* type */)
{
    return 0;
}

void QPixmapData::fromNativeType(void* /* pixmap */, NativeType /* typre */)
{
    return;
}
#endif

QT_END_NAMESPACE
