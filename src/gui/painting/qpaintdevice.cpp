/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qpaintdevice.h"

QT_BEGIN_NAMESPACE

extern void qt_painter_removePaintDevice(QPaintDevice *); //qpainter.cpp

QPaintDevice::QPaintDevice()
{
    painters = 0;
}

QPaintDevice::~QPaintDevice()
{
    if (paintingActive())
        qWarning("QPaintDevice: Cannot destroy paint device that is being "
                  "painted");
    qt_painter_removePaintDevice(this);
}


#ifndef Q_WS_QPA
int QPaintDevice::metric(PaintDeviceMetric) const
{
    qWarning("QPaintDevice::metrics: Device has no metric information");
    return 0;
}
#endif

void QPaintDevice::init(QPainter *) const
{
}

QPaintDevice *QPaintDevice::redirected(QPoint *) const
{
    return 0;
}

QPainter *QPaintDevice::sharedPainter() const
{
    return 0;
}

Q_GUI_EXPORT int qt_paint_device_metric(const QPaintDevice *device, QPaintDevice::PaintDeviceMetric metric)
{
    return device->metric(metric);
}

QT_END_NAMESPACE
