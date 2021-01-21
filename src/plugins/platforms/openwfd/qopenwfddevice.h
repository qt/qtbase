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

#ifndef QOPENWFDDEVICE_H
#define QOPENWFDDEVICE_H

#include "qopenwfdintegration.h"

#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QSocketNotifier>

#include <WF/wfd.h>

#include <gbm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

class QOpenWFDPort;

class QOpenWFDDevice : public QObject
{
    Q_OBJECT
public:
    QOpenWFDDevice(QOpenWFDIntegration *integration, WFDint handle);
    ~QOpenWFDDevice();
    WFDDevice handle() const;
    QOpenWFDIntegration *integration() const;


    bool isPipelineUsed(WFDint pipelineId);
    void addToUsedPipelineSet(WFDint pipelineId, QOpenWFDPort *port);
    void removeFromUsedPipelineSet(WFDint pipelineId);

    gbm_device *gbmDevice() const;
    EGLDisplay eglDisplay() const;
    EGLContext eglContext() const;

    PFNEGLCREATEIMAGEKHRPROC eglCreateImage;
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImage;
    PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC glEglImageTargetRenderBufferStorage;

    void commit(WFDCommitType type, WFDHandle handle);
    bool isDeviceInitializedAndCommited() const { return mCommitedDevice; }

    void waitForPipelineBindSourceCompleteEvent();

public slots:
    void readEvents(WFDtime wait = 0);
private:
    void initializeGbmAndEgl();
    void handlePortAttachDetach();
    void handlePipelineBindSourceComplete();
    QOpenWFDIntegration *mIntegration;
    WFDint mDeviceEnum;
    WFDDevice mDevice;

    WFDEvent mEvent;
    QSocketNotifier *mEventSocketNotifier;

    QList<QOpenWFDPort *> mPorts;

    QMap<WFDint, QOpenWFDPort *> mUsedPipelines;

    struct gbm_device *mGbmDevice;
    EGLDisplay mEglDisplay;
    EGLContext mEglContext;

    bool mCommitedDevice;
    bool mWaitingForBindSourceEvent;
};

#endif // QOPENWFDDEVICE_H
