/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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
