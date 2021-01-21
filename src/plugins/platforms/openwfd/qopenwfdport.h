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

#ifndef QOPENWFDPORT_H
#define QOPENWFDPORT_H

#include "qopenwfddevice.h"
#include "qopenwfdportmode.h"

#include <WF/wfd.h>

class QOpenWFDPort
{
public:
    QOpenWFDPort(QOpenWFDDevice *device, WFDint portEnumeration);
    ~QOpenWFDPort();
    void attach();
    void detach();
    bool attached() const;

    QSize setNativeResolutionMode();

    QSize pixelSize() const;
    QSizeF physicalSize() const;

    QOpenWFDDevice *device() const;
    WFDPort handle() const;
    WFDint portId() const;
    WFDPipeline pipeline() const;
    QOpenWFDScreen *screen() const;

private:
    QOpenWFDDevice *mDevice;
    WFDPort mPort;
    WFDint mPortId;

    QList<QOpenWFDPortMode> mPortModes;

    bool mAttached;
    bool mOn;
    QSize mPixelSize;
    QSizeF mPhysicalSize;

    QOpenWFDScreen *mScreen;

    WFDint mPipelineId;
    WFDPipeline mPipeline;

};

#endif // QOPENWFDPORT_H
