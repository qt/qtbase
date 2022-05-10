// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QEGLFSKMSEGLDEVICE_H
#define QEGLFSKMSEGLDEVICE_H

#include <private/qeglfskmsdevice_p.h>

QT_BEGIN_NAMESPACE

class QPlatformCursor;
class QEglFSKmsEglDeviceIntegration;

class QEglFSKmsEglDevice: public QEglFSKmsDevice
{
public:
    QEglFSKmsEglDevice(QEglFSKmsEglDeviceIntegration *devInt, QKmsScreenConfig *screenConfig, const QString &path);

    bool open() override;
    void close() override;

    void *nativeDisplay() const override;

    QPlatformScreen *createScreen(const QKmsOutput &output) override;

    QPlatformCursor *globalCursor() { return m_globalCursor; }
    void destroyGlobalCursor();

private:
    QEglFSKmsEglDeviceIntegration *m_devInt;
    QPlatformCursor *m_globalCursor;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSEGLDEVICE_H
