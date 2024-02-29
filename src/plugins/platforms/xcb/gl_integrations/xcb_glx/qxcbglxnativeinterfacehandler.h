// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#include "qxcbnativeinterfacehandler.h"

QT_BEGIN_NAMESPACE

class QXcbGlxNativeInterfaceHandler : public QXcbNativeInterfaceHandler
{
public:
    enum ResourceType {
        GLXConfig,
        GLXContext,
    };

    QXcbGlxNativeInterfaceHandler(QXcbNativeInterface *nativeInterface);
    QPlatformNativeInterface::NativeResourceForContextFunction nativeResourceFunctionForContext(const QByteArray &resource) const override;

private:
    static void *glxContextForContext(QOpenGLContext *context);
    static void *glxConfigForContext(QOpenGLContext *context);
};

QT_END_NAMESPACE
