/****************************************************************************
**
** Copyright (C) 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com>
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
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

#ifndef QEGLFSKMSINTEGRATION_H
#define QEGLFSKMSINTEGRATION_H

#include "private/qeglfsdeviceintegration_p.h"
#include <QtCore/QMap>
#include <QtCore/QVariant>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

class QKmsDevice;
class QKmsScreenConfig;

Q_EGLFS_EXPORT Q_DECLARE_LOGGING_CATEGORY(qLcEglfsKmsDebug)

class Q_EGLFS_EXPORT QEglFSKmsIntegration : public QEglFSDeviceIntegration
{
public:
    QEglFSKmsIntegration();
    ~QEglFSKmsIntegration();

    void platformInit() override;
    void platformDestroy() override;
    EGLNativeDisplayType platformDisplay() const override;
    bool usesDefaultScreen() override;
    void screenInit() override;
    QSurfaceFormat surfaceFormatFor(const QSurfaceFormat &inputFormat) const override;
    bool hasCapability(QPlatformIntegration::Capability cap) const override;
    void waitForVSync(QPlatformSurface *surface) const override;
    bool supportsPBuffers() const override;
    void *nativeResourceForIntegration(const QByteArray &name) override;
    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen) override;

    QKmsDevice *device() const;
    QKmsScreenConfig *screenConfig() const;

protected:
    virtual QKmsDevice *createDevice() = 0;

    QKmsDevice *m_device;
    QKmsScreenConfig *m_screenConfig;
};

QT_END_NAMESPACE

#endif
