/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
        return d->bitmap->bitmap()->GetPixelSize().width;
        break;
    case QPaintDevice::PdmHeight:
        return d->bitmap->bitmap()->GetPixelSize().height;
        break;
    case QPaintDevice::PdmNumColors:
        return INT_MAX;
        break;
    case QPaintDevice::PdmDepth:
        return 32;
        break;
    case QPaintDevice::PdmDpiX:
    case QPaintDevice::PdmPhysicalDpiX:
    {
        FLOAT x, y;
        QWindowsDirect2DContext::instance()->d2dFactory()->GetDesktopDpi(&x, &y);
        return x;
    }
        break;
    case QPaintDevice::PdmDpiY:
    case QPaintDevice::PdmPhysicalDpiY:
    {
        FLOAT x, y;
        QWindowsDirect2DContext::instance()->d2dFactory()->GetDesktopDpi(&x, &y);
        return y;
    }
        break;
    case QPaintDevice::PdmDevicePixelRatio:
        return 1;
        break;
    case QPaintDevice::PdmDevicePixelRatioScaled:
        return 1 * devicePixelRatioFScale();
        break;
    case QPaintDevice::PdmWidthMM:
    case QPaintDevice::PdmHeightMM:
        return -1;
        break;
    }

    return -1;
}

QT_END_NAMESPACE
