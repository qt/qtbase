// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsdirect2dpaintdevice.h"
#include "qwindowsdirect2dpaintengine.h"
#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dhelpers.h"
#include "qwindowsdirect2dbitmap.h"
#include "qwindowsdirect2ddevicecontext.h"

#include "qwindowswindow.h"

QT_BEGIN_NAMESPACE

class QWindowsDirect2DPaintDevicePrivate
{
public:
    QWindowsDirect2DPaintDevicePrivate(QWindowsDirect2DBitmap *bitmap, QInternal::PaintDeviceFlags f,
                                       QWindowsDirect2DPaintEngine::Flags paintFlags)
        : engine(new QWindowsDirect2DPaintEngine(bitmap, paintFlags))
        , bitmap(bitmap)
        , flags(f)
    {}

    QScopedPointer<QWindowsDirect2DPaintEngine> engine;
    QWindowsDirect2DBitmap *bitmap;
    QInternal::PaintDeviceFlags flags;
};

QWindowsDirect2DPaintDevice::QWindowsDirect2DPaintDevice(QWindowsDirect2DBitmap *bitmap, QInternal::PaintDeviceFlags flags,
                                                         QWindowsDirect2DPaintEngine::Flags paintFlags)
    : d_ptr(new QWindowsDirect2DPaintDevicePrivate(bitmap, flags, paintFlags))
{
}

QWindowsDirect2DPaintDevice::~QWindowsDirect2DPaintDevice()
{
}

QPaintEngine *QWindowsDirect2DPaintDevice::paintEngine() const
{
    Q_D(const QWindowsDirect2DPaintDevice);

    return d->engine.data();
}

int QWindowsDirect2DPaintDevice::devType() const
{
    Q_D(const QWindowsDirect2DPaintDevice);

    return d->flags;
}

int QWindowsDirect2DPaintDevice::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    Q_D(const QWindowsDirect2DPaintDevice);

    switch (metric) {
    case QPaintDevice::PdmWidth:
        return int(d->bitmap->bitmap()->GetPixelSize().width);
    case QPaintDevice::PdmHeight:
        return int(d->bitmap->bitmap()->GetPixelSize().height);
    case QPaintDevice::PdmNumColors:
        return INT_MAX;
    case QPaintDevice::PdmDepth:
        return 32;
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmPhysicalDpiX:
    {
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED // See QTBUG-94043
        FLOAT x, y;
        QWindowsDirect2DContext::instance()->d2dFactory()->GetDesktopDpi(&x, &y);
        return qRound(x);
    }
    case QPaintDevice::PdmDpiY:
    case QPaintDevice::PdmPhysicalDpiY:
    {
        FLOAT x, y;
        QWindowsDirect2DContext::instance()->d2dFactory()->GetDesktopDpi(&x, &y);
        return qRound(y);
QT_WARNING_POP
    }
    case QPaintDevice::PdmDevicePixelRatio:
        return 1;
    case QPaintDevice::PdmDevicePixelRatioScaled:
        return int(devicePixelRatioFScale());
    case QPaintDevice::PdmWidthMM:
    case QPaintDevice::PdmHeightMM:
        break;
    default:
        break;
    }

    return -1;
}

QT_END_NAMESPACE
