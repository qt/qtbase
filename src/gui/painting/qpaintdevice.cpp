// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpaintdevice.h"

QT_BEGIN_NAMESPACE

QPaintDevice::QPaintDevice() noexcept
{
    painters = 0;
}

QPaintDevice::~QPaintDevice()
{
    if (paintingActive())
        qWarning("QPaintDevice: Cannot destroy paint device that is being "
                  "painted");
}


/*!
    \internal
*/
void QPaintDevice::initPainter(QPainter *) const
{
}

/*!
    \internal
*/
QPaintDevice *QPaintDevice::redirected(QPoint *) const
{
    return nullptr;
}

/*!
    \internal
*/
QPainter *QPaintDevice::sharedPainter() const
{
    return nullptr;
}

Q_GUI_EXPORT int qt_paint_device_metric(const QPaintDevice *device, QPaintDevice::PaintDeviceMetric metric)
{
    return device->metric(metric);
}

int QPaintDevice::metric(PaintDeviceMetric m) const
{
    // Fallback: A subclass has not implemented PdmDevicePixelRatioScaled but might
    // have implemented PdmDevicePixelRatio.
    if (m == PdmDevicePixelRatioScaled)
        return this->metric(PdmDevicePixelRatio) * devicePixelRatioFScale();

    qWarning("QPaintDevice::metrics: Device has no metric information");

    if (m == PdmDpiX) {
        return 72;
    } else if (m == PdmDpiY) {
        return 72;
    } else if (m == PdmNumColors) {
        // FIXME: does this need to be a real value?
        return 256;
    } else if (m == PdmDevicePixelRatio) {
        return 1;
    } else {
        qDebug("Unrecognised metric %d!",m);
        return 0;
    }
}

QT_END_NAMESPACE
