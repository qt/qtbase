// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    QXcbEglIntegration *eglIntegration = static_cast<QXcbEglIntegration *>(integration->connection()->glIntegration());
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
