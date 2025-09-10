// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDEVICEDISCOVERY_UDEV_H
#define QDEVICEDISCOVERY_UDEV_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qdevicediscovery_p.h"
#include <libudev.h>

QT_BEGIN_NAMESPACE

class QDeviceDiscoveryUDev : public QDeviceDiscovery
{
    Q_OBJECT

public:
    QDeviceDiscoveryUDev(QDeviceTypes types, struct udev *udev, QObject *parent = nullptr);
    ~QDeviceDiscoveryUDev();
    QStringList scanConnectedDevices() override;

private slots:
    void handleUDevNotification();

protected:
    struct udev *m_udev;

private:
    bool checkDeviceType(struct udev_device *dev);

    void startWatching();
    void stopWatching();

    struct udev_monitor *m_udevMonitor = nullptr;
    int m_udevMonitorFileDescriptor = -1;
    QSocketNotifier *m_udevSocketNotifier = nullptr;
};

QT_END_NAMESPACE

#endif // QDEVICEDISCOVERY_UDEV_H
