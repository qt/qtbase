// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dpaintdevice.h"
#include "qwindowsdirect2dplatformpixmap.h"
#include "qwindowsdirect2dbitmap.h"
#include "qwindowsdirect2dhelpers.h"

#include <QtGui/qpainter.h>
#include <QtGui/qimage.h>
#include <QtGui/qpaintdevice.h>
#include <QtGui/qpaintengine.h>

QT_BEGIN_NAMESPACE

class QWindowsDirect2DPlatformPixmapPrivate
{
public:
    QWindowsDirect2DPlatformPixmapPrivate()
        : owns_bitmap(true)
        , bitmap(new QWindowsDirect2DBitmap)
        , device(new QWindowsDirect2DPaintDevice(bitmap, QInternal::Pixmap))
    {}

    QWindowsDirect2DPlatformPixmapPrivate(QWindowsDirect2DBitmap *bitmap,
                                          QWindowsDirect2DPaintEngine::Flags flags)
        : bitmap(bitmap)
        , device(new QWindowsDirect2DPaintDevice(bitmap, QInternal::Pixmap, flags))
    {}

    ~QWindowsDirect2DPlatformPixmapPrivate()
    {
        if (owns_bitmap)
            delete bitmap;
    }

    bool owns_bitmap = false;
    QWindowsDirect2DBitmap *bitmap;
    QScopedPointer<QWindowsDirect2DPaintDevice> device;
    qreal devicePixelRatio = 1.0;
};

static int qt_d2dpixmap_serno = 0;

QWindowsDirect2DPlatformPixmap::QWindowsDirect2DPlatformPixmap(PixelType pixelType)
    : QPlatformPixmap(pixelType, Direct2DClass)
    , d_ptr(new QWindowsDirect2DPlatformPixmapPrivate)
{
    setSerialNumber(qt_d2dpixmap_serno++);
}

QWindowsDirect2DPlatformPixmap::QWindowsDirect2DPlatformPixmap(QPlatformPixmap::PixelType pixelType,
                                                               QWindowsDirect2DPaintEngine::Flags flags,
                                                               QWindowsDirect2DBitmap *bitmap)
    : QPlatformPixmap(pixelType, Direct2DClass)
    , d_ptr(new QWindowsDirect2DPlatformPixmapPrivate(bitmap, flags))
{
    setSerialNumber(qt_d2dpixmap_serno++);

    is_null = false;
    w = bitmap->size().width();
    h = bitmap->size().height();
    this->d = 32;
}

QWindowsDirect2DPlatformPixmap::~QWindowsDirect2DPlatformPixmap()
{

}

void QWindowsDirect2DPlatformPixmap::resize(int width, int height)
{
    Q_D(QWindowsDirect2DPlatformPixmap);

    if (!d->bitmap->resize(width, height)) {
        qWarning("%s: Could not resize bitmap", __FUNCTION__);
        return;
    }

    is_null = false;
    w = width;
    h = height;
    this->d = 32;
}

void QWindowsDirect2DPlatformPixmap::fromImage(const QImage &image,
                                               Qt::ImageConversionFlags flags)
{
    Q_D(QWindowsDirect2DPlatformPixmap);

    if (!d->bitmap->fromImage(image, flags)) {
        qWarning("%s: Could not init from image", __FUNCTION__);
        return;
    }

    is_null = false;
    w = image.width();
    h = image.height();
    this->d = 32;
}

int QWindowsDirect2DPlatformPixmap::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    Q_D(const QWindowsDirect2DPlatformPixmap);
    return qt_paint_device_metric(d->device.data(), metric);
}

void QWindowsDirect2DPlatformPixmap::fill(const QColor &color)
{
    Q_D(QWindowsDirect2DPlatformPixmap);
    d->bitmap->fill(color);
}

bool QWindowsDirect2DPlatformPixmap::hasAlphaChannel() const
{
    return true;
}

QImage QWindowsDirect2DPlatformPixmap::toImage() const
{
    return toImage(QRect());
}

QImage QWindowsDirect2DPlatformPixmap::toImage(const QRect &rect) const
{
    Q_D(const QWindowsDirect2DPlatformPixmap);

    QWindowsDirect2DPaintEngineSuspender suspender(static_cast<QWindowsDirect2DPaintEngine *>(d->device->paintEngine()));
    return d->bitmap->toImage(rect);
}

QPaintEngine* QWindowsDirect2DPlatformPixmap::paintEngine() const
{
    Q_D(const QWindowsDirect2DPlatformPixmap);
    return d->device->paintEngine();
}

qreal QWindowsDirect2DPlatformPixmap::devicePixelRatio() const
{
    Q_D(const QWindowsDirect2DPlatformPixmap);
    return d->devicePixelRatio;
}

void QWindowsDirect2DPlatformPixmap::setDevicePixelRatio(qreal scaleFactor)
{
    Q_D(QWindowsDirect2DPlatformPixmap);
    d->devicePixelRatio = scaleFactor;
}

QWindowsDirect2DBitmap *QWindowsDirect2DPlatformPixmap::bitmap() const
{
    Q_D(const QWindowsDirect2DPlatformPixmap);
    return d->bitmap;
}

QT_END_NAMESPACE
