/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QWINDOWSDIRECT2DPAINTDEVICE_H
#define QWINDOWSDIRECT2DPAINTDEVICE_H

#include <QtCore/qscopedpointer.h>
#include <QtGui/qpaintdevice.h>
#include "qwindowsdirect2dpaintengine.h"

QT_BEGIN_NAMESPACE

class QWindowsDirect2DBitmap;

class QWindowsDirect2DPaintDevicePrivate;
class QWindowsDirect2DPaintDevice : public QPaintDevice
{
    Q_DECLARE_PRIVATE(QWindowsDirect2DPaintDevice)

public:
    QWindowsDirect2DPaintDevice(QWindowsDirect2DBitmap *bitmap, QInternal::PaintDeviceFlags flags,
                                QWindowsDirect2DPaintEngine::Flags paintFlags = QWindowsDirect2DPaintEngine::NoFlag);
    ~QWindowsDirect2DPaintDevice();

    QPaintEngine *paintEngine() const override;
    int devType() const override;

protected:
    int metric(PaintDeviceMetric metric) const override;

private:
    QScopedPointer<QWindowsDirect2DPaintDevicePrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DPAINTDEVICE_H
