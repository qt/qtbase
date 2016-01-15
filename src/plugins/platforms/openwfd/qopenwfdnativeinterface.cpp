/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
