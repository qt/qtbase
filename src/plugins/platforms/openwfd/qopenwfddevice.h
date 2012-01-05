/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
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
