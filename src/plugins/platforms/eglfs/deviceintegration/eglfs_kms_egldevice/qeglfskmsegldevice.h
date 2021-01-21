/****************************************************************************
**
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

#ifndef QEGLFSKMSEGLDEVICE_H
#define QEGLFSKMSEGLDEVICE_H

#include <qeglfskmsdevice.h>

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
