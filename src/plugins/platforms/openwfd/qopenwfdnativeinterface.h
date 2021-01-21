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

#ifndef QOPENWFDNATIVEINTERFACE_H
#define QOPENWFDNATIVEINTERFACE_H

#include <qpa/qplatformnativeinterface.h>

#include <WF/wfdplatform.h>

class QOpenWFDScreen;

class QOpenWFDNativeInterface : public QPlatformNativeInterface
{
public:
    enum ResourceType {
        WFDDevice,
        EglDisplay,
        EglContext,
        WFDPort,
        WFDPipeline
    };

    void *nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context);
    void *nativeResourceForWindow(const QByteArray &resourceString, QWindow *window);

    WFDHandle wfdDeviceForWindow(QWindow *window);
    void *eglDisplayForWindow(QWindow *window);
    WFDHandle wfdPortForWindow(QWindow *window);
    WFDHandle wfdPipelineForWindow(QWindow *window);

    void *eglContextForContext(QOpenGLContext *context);

private:
    static QOpenWFDScreen *qPlatformScreenForWindow(QWindow *window);
};

#endif // QOPENWFDNATIVEINTERFACE_H
