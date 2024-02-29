// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "qxcbnativeinterfacehandler.h"

QT_BEGIN_NAMESPACE

class QXcbEglNativeInterfaceHandler : public QXcbNativeInterfaceHandler
{
public:
    enum ResourceType {
        EglDisplay,
        EglContext,
        EglConfig
    };

    QXcbEglNativeInterfaceHandler(QXcbNativeInterface *nativeInterface);

    QPlatformNativeInterface::NativeResourceForIntegrationFunction nativeResourceFunctionForIntegration(const QByteArray &resource) const override;
    QPlatformNativeInterface::NativeResourceForContextFunction nativeResourceFunctionForContext(const QByteArray &resource) const override;
    QPlatformNativeInterface::NativeResourceForWindowFunction nativeResourceFunctionForWindow(const QByteArray &resource) const override;
private:
    static void *eglDisplay();
    static void *eglDisplayForWindow(QWindow *window);
    static void *eglContextForContext(QOpenGLContext *context);
    static void *eglConfigForContext(QOpenGLContext *context);
};

QT_END_NAMESPACE
