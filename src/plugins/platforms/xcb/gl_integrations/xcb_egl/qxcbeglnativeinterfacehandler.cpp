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

#include "qxcbeglnativeinterfacehandler.h"

#include <QtGui/private/qguiapplication_p.h>
#include "qxcbeglwindow.h"
#include "qxcbintegration.h"
#include "qxcbeglintegration.h"
#include "qxcbeglcontext.h"

QT_BEGIN_NAMESPACE

static int resourceType(const QByteArray &key)
{
    static const QByteArray names[] = { // match QXcbEglNativeInterfaceHandler::ResourceType
        QByteArrayLiteral("egldisplay"),
        QByteArrayLiteral("eglcontext"),
        QByteArrayLiteral("eglconfig")
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (key == names[i])
            return i;
    }

    if (key == QByteArrayLiteral("get_egl_context"))
        return QXcbEglNativeInterfaceHandler::EglContext;

    return sizeof(names) / sizeof(names[0]);
}

QXcbEglNativeInterfaceHandler::QXcbEglNativeInterfaceHandler(QXcbNativeInterface *nativeInterface)
    : QXcbNativeInterfaceHandler(nativeInterface)
{
}

QPlatformNativeInterface::NativeResourceForIntegrationFunction QXcbEglNativeInterfaceHandler::nativeResourceFunctionForIntegration(const QByteArray &resource) const{
    switch (resourceType(resource)) {
    case EglDisplay:
        return eglDisplay;
    default:
        break;
    }
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForContextFunction QXcbEglNativeInterfaceHandler::nativeResourceFunctionForContext(const QByteArray &resource) const
{
    switch (resourceType(resource)) {
    case EglContext:
        return eglContextForContext;
    case EglConfig:
        return eglConfigForContext;
    default:
        break;
    }
    return nullptr;
}

QPlatformNativeInterface::NativeResourceForWindowFunction QXcbEglNativeInterfaceHandler::nativeResourceFunctionForWindow(const QByteArray &resource) const
{
    switch (resourceType(resource)) {
    case EglDisplay:
        return eglDisplayForWindow;
    default:
        break;
    }
    return nullptr;
}

void *QXcbEglNativeInterfaceHandler::eglDisplay()
{
    QXcbIntegration *integration = QXcbIntegration::instance();
    QXcbEglIntegration *eglIntegration = static_cast<QXcbEglIntegration *>(integration->defaultConnection()->glIntegration());
    return eglIntegration->eglDisplay();
}

void *QXcbEglNativeInterfaceHandler::eglDisplayForWindow(QWindow *window)
{
    Q_ASSERT(window);
    if (window->supportsOpenGL() && window->handle() == nullptr)
        return eglDisplay();
    else if (window->supportsOpenGL())
        return static_cast<QXcbEglWindow *>(window->handle())->glIntegration()->eglDisplay();
    return nullptr;
}

void *QXcbEglNativeInterfaceHandler::eglContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);
    Q_ASSERT(context->handle());
    return static_cast<QXcbEglContext *>(context->handle())->eglContext();
}

void *QXcbEglNativeInterfaceHandler::eglConfigForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);
    Q_ASSERT(context->handle());
    return static_cast<QXcbEglContext *>(context->handle())->eglConfig();
}

QT_END_NAMESPACE
