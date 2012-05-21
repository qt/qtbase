/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDEVICEDISCOVERY_H
#define QDEVICEDISCOVERY_H

#include <QObject>
#include <QSocketNotifier>

#ifndef QT_NO_LIBUDEV
#include <libudev.h>
#endif

#define QT_EVDEV_DEVICE_PATH "/dev/input/"
#define QT_EVDEV_DEVICE_PREFIX "event"
#define QT_EVDEV_DEVICE QT_EVDEV_DEVICE_PATH QT_EVDEV_DEVICE_PREFIX

#define QT_DRM_DEVICE_PATH "/dev/dri/"
#define QT_DRM_DEVICE_PREFIX "card"
#define QT_DRM_DEVICE QT_DRM_DEVICE_PATH QT_DRM_DEVICE_PREFIX

QT_BEGIN_NAMESPACE

class QDeviceDiscovery : public QObject
{
    Q_OBJECT
    Q_ENUMS(QDeviceType)

public:
    enum QDeviceType {
        Device_Unknown = 0x00,
        Device_Mouse = 0x01,
        Device_Touchpad = 0x02,
        Device_Touchscreen = 0x04,
        Device_Keyboard = 0x08,
        Device_DRM = 0x10,
        Device_InputMask = Device_Mouse | Device_Touchpad | Device_Touchscreen | Device_Keyboard,
        Device_VideoMask = Device_DRM
    };
    Q_DECLARE_FLAGS(QDeviceTypes, QDeviceType)

    static QDeviceDiscovery *create(QDeviceTypes type, QObject *parent);
    ~QDeviceDiscovery();

    QStringList scanConnectedDevices();

signals:
    void deviceDetected(const QString &deviceNode);
    void deviceRemoved(const QString &deviceNode);

#ifndef QT_NO_LIBUDEV
private slots:
    void handleUDevNotification();
#endif

private:
#ifndef QT_NO_LIBUDEV
    QDeviceDiscovery(QDeviceTypes types, struct udev *udev, QObject *parent = 0);
    bool checkDeviceType(struct udev_device *dev);
#else
    QDeviceDiscovery(QDeviceTypes types, QObject *parent = 0);
    bool checkDeviceType(const QString &device);
#endif

    QDeviceTypes m_types;

#ifndef QT_NO_LIBUDEV
    void startWatching();
    void stopWatching();

    struct udev *m_udev;
    struct udev_monitor *m_udevMonitor;
    int m_udevMonitorFileDescriptor;
    QSocketNotifier *m_udevSocketNotifier;
#endif
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QDeviceDiscovery::QDeviceTypes)

QT_END_NAMESPACE

#endif // QDEVICEDISCOVERY_H
