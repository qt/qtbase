/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDIRECTFBPIXMAP_H
#define QDIRECTFBPIXMAP_H

#include <qglobal.h>

#ifndef QT_NO_QWS_DIRECTFB

#include <QtGui/private/qpixmapdata_p.h>
#include <QtGui/private/qpaintengine_raster_p.h>
#include "qdirectfbpaintdevice.h"
#include <directfb.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QDirectFBPaintEngine;

class QDirectFBPixmapData : public QPixmapData, public QDirectFBPaintDevice
{
public:
    QDirectFBPixmapData(QDirectFBScreen *screen, PixelType pixelType);
    ~QDirectFBPixmapData();

    // Re-implemented from QPixmapData:
    virtual void resize(int width, int height);
    virtual void fromImage(const QImage &image, Qt::ImageConversionFlags flags);
#ifdef QT_DIRECTFB_IMAGEPROVIDER
    virtual bool fromFile(const QString &filename, const char *format,
                          Qt::ImageConversionFlags flags);
    virtual bool fromData(const uchar *buffer, uint len, const char *format,
                          Qt::ImageConversionFlags flags);
#endif
    virtual void copy(const QPixmapData *data, const QRect &rect);
    virtual void fill(const QColor &color);
    virtual QPixmap transformed(const QTransform &matrix,
                                Qt::TransformationMode mode) const;
    virtual QImage toImage() const;
    virtual QPaintEngine *paintEngine() const;
    virtual QImage *buffer();
    virtual bool scroll(int dx, int dy, const QRect &rect);
    // Pure virtual in QPixmapData, so re-implement here and delegate to QDirectFBPaintDevice
    virtual int metric(QPaintDevice::PaintDeviceMetric m) const { return QDirectFBPaintDevice::metric(m); }

    inline QImage::Format pixelFormat() const { return imageFormat; }
    inline bool hasAlphaChannel() const { return alpha; }
    static bool hasAlphaChannel(const QImage &img, Qt::ImageConversionFlags flags = Qt::AutoColor);
private:
#ifdef QT_DIRECTFB_IMAGEPROVIDER
    bool fromDataBufferDescription(const DFBDataBufferDescription &dataBuffer);
#endif
    void invalidate();
    bool alpha;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_QWS_DIRECTFB

#endif // QDIRECTFBPIXMAP_H
