// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdevicediscovery_vxworks_p.h"

#include <QStringList>
#include <QCoreApplication>
#include <QObject>
#include <QHash>
#include <QDir>
#include <QLoggingCategory>
#include <QtCore/private/qcore_unix_p.h>

#include <evdevLib.h>
#include <fcntl.h>

#define LONG_BITS (sizeof(long) * 8 )
#define LONG_FIELD_SIZE(bits) ((bits / LONG_BITS) + 1)

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcDD, "qt.qpa.input")

QDeviceDiscovery *QDeviceDiscovery::create(QDeviceTypes types, QObject *parent)
{
    return new QDeviceDiscoveryVxWorks(types, parent);
}

QDeviceDiscoveryVxWorks::QDeviceDiscoveryVxWorks(QDeviceTypes types, QObject *parent)
    : QDeviceDiscovery(types, parent)
{
    qCDebug(lcDD) << "vxworks device discovery for type" << types;
}

QStringList QDeviceDiscoveryVxWorks::scanConnectedDevices()
{
    QStringList devices;

    // check for input devices
    if (m_types & Device_InputMask) {
        for (const auto &entry : QDirListing(QString::fromLatin1("/input/"))) {
            QString absoluteFilePath = entry.absoluteFilePath();
            if (checkDeviceType(absoluteFilePath))
                devices << absoluteFilePath;
        }
    }

    if (m_types & Device_VideoMask) {
        devices << QString::fromLatin1("/dev/dri/card0");
    }

    return devices;
}

bool QDeviceDiscoveryVxWorks::checkDeviceType(const QString &device)
{
    int fd = QT_OPEN(device.toLocal8Bit().constData(), O_RDONLY | O_NDELAY, 0);
    if (Q_UNLIKELY(fd == -1)) {
        // This is changed to debug type message due the nature of scanning
        // and adding new device for VxWorks by getting dev count from
        // dev /input/event0 which might be already in use
        qCDebug(lcDD) << "Device discovery cannot open device" << device;
        return false;
    }

    qCDebug(lcDD) << "doing static device discovery for " << device;

    if ((m_types & Device_DRM) && device.contains(QT_DRM_DEVICE_PREFIX ""_L1)) {
        QT_CLOSE(fd);
        return true;
    }

    UINT32 devCap = 0;
    if (ERROR != ioctl(fd, EV_DEV_IO_GET_CAP, (char *)&devCap)) {
        if ((m_types & Device_Keyboard) && (devCap & EV_DEV_KEY)) {
            if (!(devCap & EV_DEV_REL) && !(devCap & EV_DEV_ABS)) {
                qCDebug(lcDD) << "DeviceDiscovery found keyboard at" << device;
                QT_CLOSE(fd);
                return true;
            }
        }

        if (m_types & Device_Mouse) {
            if ((devCap & EV_DEV_REL) && (devCap & EV_DEV_KEY) && !(devCap & EV_DEV_ABS)) {
                qCDebug(lcDD) << "DeviceDiscovery found mouse at" << device;
                QT_CLOSE(fd);
                return true;
            }
        }

        if ((m_types & (Device_Touchpad | Device_Touchscreen))) {
            if ((m_types & Device_Touchscreen) && (devCap & EV_DEV_ABS && (devCap & EV_DEV_KEY))) {
                qCDebug(lcDD) << "DeviceDiscovery found touchscreen at" << device;
                QT_CLOSE(fd);
                return true;
            }
        }
    }
    QT_CLOSE(fd);

    return false;
}

QT_END_NAMESPACE
