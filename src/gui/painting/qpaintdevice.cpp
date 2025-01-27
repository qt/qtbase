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
// ### Qt 7: Replace this workaround mechanism: virtual devicePixelRatio() and virtual metricF()
double QPaintDevice::getDecodedMetricF(PaintDeviceMetric metricA, PaintDeviceMetric metricB) const
{
    qint32 buf[2];
    // The Encoded metric enum values come in pairs of one odd and one even value.
    // We map those to the 0 and 1 indexes of buf by taking just the least significant bit.
    // Same mapping here as in the encodeMetricF() function, to ensure correct order.
    buf[metricA & 1] = metric(metricA);
    buf[metricB & 1] = metric(metricB);
    double res;
    memcpy(&res, buf, sizeof(res));
    return res;
}

qreal QPaintDevice::devicePixelRatio() const
{
    Q_STATIC_ASSERT((PdmDevicePixelRatioF_EncodedA & 1) != (PdmDevicePixelRatioF_EncodedB & 1));
    double res;
    int scaledDpr = metric(PdmDevicePixelRatioScaled);
    if (scaledDpr == int(devicePixelRatioFScale())) {
        res = 1; // Shortcut for common case
    } else if (scaledDpr == 2 * int(devicePixelRatioFScale())) {
        res = 2; // Shortcut for common case
    } else {
        res = getDecodedMetricF(PdmDevicePixelRatioF_EncodedA, PdmDevicePixelRatioF_EncodedB);
        if (res <= 0) // These metrics not implemented, fall back to PdmDevicePixelRatioScaled
            res = scaledDpr / devicePixelRatioFScale();
    }
    return res;
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
    if (m == PdmNumColors)
        return 0;
    if (m == PdmDevicePixelRatio)
        return 1;

    qWarning("QPaintDevice::metrics: Device has no metric information");

    switch (m) {
    case PdmDevicePixelRatioScaled:
    case PdmDevicePixelRatio:
    case PdmNumColors:
        Q_UNREACHABLE();
        break;
    case PdmDpiX:
    case PdmDpiY:
        return 72;
    case PdmDevicePixelRatioF_EncodedA:
    case PdmDevicePixelRatioF_EncodedB:
        return 0;
    case PdmWidth:
    case PdmHeight:
    case PdmWidthMM:
    case PdmHeightMM:
    case PdmDepth:
    case PdmPhysicalDpiX:
    case PdmPhysicalDpiY:
        return 0;
    }
    qDebug("Unrecognized metric %d!", m);
    return 0;
}

QT_END_NAMESPACE
