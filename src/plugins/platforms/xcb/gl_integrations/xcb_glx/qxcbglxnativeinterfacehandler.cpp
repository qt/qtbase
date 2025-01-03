// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbglxnativeinterfacehandler.h"

#include "qglxintegration.h"
#include <QtGui/QOpenGLContext>
QT_BEGIN_NAMESPACE

static int resourceType(const QByteArray &key)
{
    static const QByteArray names[] = { // match QXcbGlxNativeInterfaceHandler::ResourceType
        QByteArrayLiteral("glxconfig"),
        QByteArrayLiteral("glxcontext"),
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (key == names[i])
            return i;
    }

    return sizeof(names) / sizeof(names[0]);
}

QXcbGlxNativeInterfaceHandler::QXcbGlxNativeInterfaceHandler(QXcbNativeInterface *nativeInterface)
    : QXcbNativeInterfaceHandler(nativeInterface)
{
}

QPlatformNativeInterface::NativeResourceForContextFunction QXcbGlxNativeInterfaceHandler::nativeResourceFunctionForContext(const QByteArray &resource) const
{
    switch (resourceType(resource)) {
    case GLXConfig:
        return glxConfigForContext;
    case GLXContext:
        return glxContextForContext;
    default:
        break;
    }
    return nullptr;
}

void *QXcbGlxNativeInterfaceHandler::glxContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);
    QGLXContext *glxPlatformContext = static_cast<QGLXContext *>(context->handle());
    return glxPlatformContext->glxContext();
}

void *QXcbGlxNativeInterfaceHandler::glxConfigForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);
    QGLXContext *glxPlatformContext = static_cast<QGLXContext *>(context->handle());
    return glxPlatformContext->glxConfig();

}

QT_END_NAMESPACE
