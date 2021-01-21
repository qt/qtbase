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

#include "qopenwfdport.h"

#include "qopenwfdportmode.h"
#include "qopenwfdscreen.h"

#include <QtCore/QDebug>

QOpenWFDPort::QOpenWFDPort(QOpenWFDDevice *device, WFDint portEnumeration)
    : mDevice(device)
    , mPortId(portEnumeration)
    , mAttached(false)
    , mPipelineId(WFD_INVALID_PIPELINE_ID)
    , mPipeline(WFD_INVALID_HANDLE)
{
    mPort = wfdCreatePort(device->handle(),portEnumeration,0);
    WFDint isPortAttached = wfdGetPortAttribi(device->handle(),mPort,WFD_PORT_ATTACHED);
    if (isPortAttached) {
        attach();
    }
}

QOpenWFDPort::~QOpenWFDPort()
{
    detach();

    wfdDestroyPort(mDevice->handle(),mPort);
}

void QOpenWFDPort::attach()
{
    if (mAttached) {
        return;
    }

    //just forcing port to be on
    wfdSetPortAttribi(mDevice->handle(), mPort, WFD_PORT_POWER_MODE, WFD_POWER_MODE_ON);

    int numberOfPortModes = wfdGetPortModes(mDevice->handle(),mPort,0,0);
    WFDPortMode portModes[numberOfPortModes];
    int actualNumberOfPortModes = wfdGetPortModes(mDevice->handle(),mPort,portModes,numberOfPortModes);
    Q_ASSERT(actualNumberOfPortModes == numberOfPortModes);

    if (!actualNumberOfPortModes) {
        qDebug("didn't find any available port modes");
        return;
    }

    for (int i = 0; i < actualNumberOfPortModes; i++) {
        if (portModes[i] != WFD_INVALID_HANDLE) {
            mPortModes.append(QOpenWFDPortMode(this,portModes[i]));
            qDebug() << "PortModeAdded:" << mPortModes.at(mPortModes.size()-1);
        }
    }


    mPixelSize = setNativeResolutionMode();
    if (mPixelSize.isEmpty()) {
        qDebug("Could not set native resolution mode in QOpenWFPort");
    }

    WFDfloat physicalWFDSize[2];
    wfdGetPortAttribfv(mDevice->handle(),mPort,WFD_PORT_PHYSICAL_SIZE,2,physicalWFDSize);
    mPhysicalSize = QSizeF(physicalWFDSize[0],physicalWFDSize[1]);

    WFDint numAvailablePipelines = wfdGetPortAttribi(mDevice->handle(),mPort,WFD_PORT_PIPELINE_ID_COUNT);
    if (Q_UNLIKELY(!numAvailablePipelines))
        qFatal("Not possible to make screen that is not possible to create WFPort with no pipline");

    WFDint pipeIds[numAvailablePipelines];
    wfdGetPortAttribiv(mDevice->handle(),mPort,WFD_PORT_BINDABLE_PIPELINE_IDS,numAvailablePipelines,pipeIds);

    for (int i = 0; i < numAvailablePipelines; i++) {
        if (pipeIds[i] != WFD_INVALID_PIPELINE_ID && !mDevice->isPipelineUsed(pipeIds[i])) {
            mPipelineId = pipeIds[i];
            mDevice-> addToUsedPipelineSet(mPipelineId,this);

            mPipeline = wfdCreatePipeline(mDevice->handle(),mPipelineId,WFD_NONE);
            if (Q_UNLIKELY(mPipeline == WFD_INVALID_HANDLE))
                qFatal("Failed to create pipeline for port %p", this);

            break;
        }
    }

    if (mPipeline == WFD_INVALID_HANDLE) {
        qWarning("Failed to create pipeline and can't bind it to port");
    }

    WFDint geomerty[] = { 0, 0, mPixelSize.width(), mPixelSize.height() };

    wfdSetPipelineAttribiv(mDevice->handle(),mPipeline, WFD_PIPELINE_SOURCE_RECTANGLE, 4, geomerty);
    wfdSetPipelineAttribiv(mDevice->handle(),mPipeline, WFD_PIPELINE_DESTINATION_RECTANGLE, 4, geomerty);

    wfdBindPipelineToPort(mDevice->handle(),mPort,mPipeline);

    mScreen = new QOpenWFDScreen(this);
    QWindowSystemInterface::handleScreenAdded(mScreen);
    mAttached = true;
}

void QOpenWFDPort::detach()
{
    if (!mAttached)
        return;

    mAttached = false;
    mOn = false;

    QWindowSystemInterface::handleScreenRemoved(mScreen);

    wfdDestroyPipeline(mDevice->handle(),mPipeline);
    mPipelineId = WFD_INVALID_PIPELINE_ID;
    mPipeline = WFD_INVALID_HANDLE;
}

bool QOpenWFDPort::attached() const
{
    return mAttached;
}

QSize QOpenWFDPort::setNativeResolutionMode()
{
    WFDint nativePixelSize[2];
    wfdGetPortAttribiv(device()->handle(),mPort,WFD_PORT_NATIVE_RESOLUTION,2,nativePixelSize);
    QSize nativeSize(nativePixelSize[0],nativePixelSize[1]);

    for (int i = 0; i < mPortModes.size(); i++) {
        const QOpenWFDPortMode &mode = mPortModes.at(i);
        if (nativeSize == mode.size()) {
            wfdSetPortMode(device()->handle(),mPort,mode.handle());
            return nativeSize;
        }
    }
    return QSize();
}

QSize QOpenWFDPort::pixelSize() const
{
    return mPixelSize;
}

QSizeF QOpenWFDPort::physicalSize() const
{
    return mPhysicalSize;
}

QOpenWFDDevice * QOpenWFDPort::device() const
{
    return mDevice;
}

WFDPort QOpenWFDPort::handle() const
{
    return mPort;
}

WFDint QOpenWFDPort::portId() const
{
    return mPortId;
}

WFDPipeline QOpenWFDPort::pipeline() const
{
    return mPipeline;
}

QOpenWFDScreen * QOpenWFDPort::screen() const
{
    return mScreen;
}
