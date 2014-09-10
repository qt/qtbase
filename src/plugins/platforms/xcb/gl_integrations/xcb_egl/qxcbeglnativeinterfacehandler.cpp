/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbeglnativeinterfacehandler.h"

#include "qxcbeglwindow.h"
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
    return Q_NULLPTR;
}

QPlatformNativeInterface::NativeResourceForWindowFunction QXcbEglNativeInterfaceHandler::nativeResourceFunctionForWindow(const QByteArray &resource) const
{
    switch (resourceType(resource)) {
    case EglDisplay:
        return eglDisplayForWindow;
    default:
        break;
    }
    return Q_NULLPTR;
}

void *QXcbEglNativeInterfaceHandler::eglDisplayForWindow(QWindow *window)
{
    Q_ASSERT(window);
    Q_ASSERT(window->handle());
    if (window->supportsOpenGL())
        return static_cast<QXcbEglWindow *>(window->handle())->glIntegration()->eglDisplay();
    return Q_NULLPTR;
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
