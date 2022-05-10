// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXNATIVEINTERFACE_H
#define QQNXNATIVEINTERFACE_H

#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

class QQnxIntegration;

class QQnxNativeInterface : public QPlatformNativeInterface
{
public:
    QQnxNativeInterface(QQnxIntegration *integration);
    void *nativeResourceForWindow(const QByteArray &resource, QWindow *window) override;
    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen) override;
    void *nativeResourceForIntegration(const QByteArray &resource) override;

#if !defined(QT_NO_OPENGL)
    void *nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context) override;
#endif

    void setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value) override;
    NativeResourceForIntegrationFunction nativeResourceFunctionForIntegration(const QByteArray &resource) override;

private:
    QQnxIntegration *m_integration;
};

QT_END_NAMESPACE

#endif // QQNXNATIVEINTERFACE_H
