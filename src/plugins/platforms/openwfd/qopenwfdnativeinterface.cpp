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

#include "qopenwfdnativeinterface.h"

#include "qopenwfdscreen.h"
#include "qopenwfdwindow.h"
#include "qopenwfdglcontext.h"

#include <private/qguiapplication_p.h>
#include <QtCore/QMap>

#include <QtCore/QDebug>

#include <QtGui/qguiglcontext_qpa.h>

class QOpenWFDResourceMap : public QMap<QByteArray, QOpenWFDNativeInterface::ResourceType>
{
public:
    QOpenWFDResourceMap()
        :QMap<QByteArray, QOpenWFDNativeInterface::ResourceType>()
    {
        insert("wfddevice",QOpenWFDNativeInterface::WFDDevice);
        insert("egldisplay",QOpenWFDNativeInterface::EglDisplay);
        insert("eglcontext",QOpenWFDNativeInterface::EglContext);
        insert("wfdport",QOpenWFDNativeInterface::WFDPort);
        insert("wfdpipeline",QOpenWFDNativeInterface::WFDPipeline);
    }
};

Q_GLOBAL_STATIC(QOpenWFDResourceMap, qOpenWFDResourceMap)

void *QOpenWFDNativeInterface::nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    ResourceType resource = qOpenWFDResourceMap()->value(lowerCaseResource);
    void *result = 0;
    switch (resource) {
    case EglContext:
        result = eglContextForContext(context);
        break;
    default:
        result = 0;
    }
    return result;
}

void *QOpenWFDNativeInterface::nativeResourceForWindow(const QByteArray &resourceString, QWindow *window)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    ResourceType resource = qOpenWFDResourceMap()->value(lowerCaseResource);
    void *result = 0;
    switch (resource) {
    //What should we do for int wfd handles? This is clearly not the solution
    case WFDDevice:
        result = (void *)wfdDeviceForWindow(window);
        break;
    case WFDPort:
        result = (void *)wfdPortForWindow(window);
        break;
    case WFDPipeline:
        result = (void *)wfdPipelineForWindow(window);
        break;
    case EglDisplay:
        result = eglDisplayForWindow(window);
        break;
    default:
        result = 0;
    }
    return result;
}

WFDHandle QOpenWFDNativeInterface::wfdDeviceForWindow(QWindow *window)
{
    QOpenWFDWindow *openWFDwindow = static_cast<QOpenWFDWindow *>(window->handle());
    return openWFDwindow->port()->device()->handle();
}

WFDHandle QOpenWFDNativeInterface::wfdPortForWindow(QWindow *window)
{
    QOpenWFDWindow *openWFDwindow = static_cast<QOpenWFDWindow *>(window->handle());
    return openWFDwindow->port()->handle();
}

WFDHandle QOpenWFDNativeInterface::wfdPipelineForWindow(QWindow *window)

{
    QOpenWFDWindow *openWFDwindow = static_cast<QOpenWFDWindow *>(window->handle());
    return openWFDwindow->port()->pipeline();

}

void *QOpenWFDNativeInterface::eglDisplayForWindow(QWindow *window)
{
    QOpenWFDWindow *openWFDwindow = static_cast<QOpenWFDWindow *>(window->handle());
    return openWFDwindow->port()->device()->eglDisplay();
}

void * QOpenWFDNativeInterface::eglContextForContext(QOpenGLContext *context)
{
    QOpenWFDGLContext *openWFDContext = static_cast<QOpenWFDGLContext *>(context->handle());
    return openWFDContext->eglContext();
}
