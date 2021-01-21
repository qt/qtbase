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

#ifndef QXCBEGLNATIVEINTERFACEHANDLER_H
#define QXCBEGLNATIVEINTERFACEHANDLER_H

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

#endif //QXCBEGLNATIVEINTERFACEHANDLER_H
