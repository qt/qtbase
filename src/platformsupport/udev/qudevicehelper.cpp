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

#include "qudevicehelper_p.h"

#include <QStringList>
#include <QCoreApplication>
#include <QObject>
#include <QHash>
#include <QSocketNotifier>

#include <linux/input.h>

//#define QT_QPA_UDEVICE_HELPER_DEBUG

#ifdef QT_QPA_UDEVICE_HELPER_DEBUG
#include <QtDebug>
#endif

QT_BEGIN_NAMESPACE

QUDeviceHelper *QUDeviceHelper::createUDeviceHelper(QUDeviceTypes types, QObject *parent)
{
#ifdef QT_QPA_UDEVICE_HELPER_DEBUG
    qWarning() << "Try to create new UDeviceHelper";
#endif

    QUDeviceHelper *helper = 0;
    struct udev *udev;

    udev = udev_new();
    if (udev) {
        helper = new QUDeviceHelper(types, udev, parent);
    } else {
        qWarning("Failed to get udev library context.");
    }

    return helper;
}

QUDeviceHelper::QUDeviceHelper(QUDeviceTypes types, struct udev *udev, QObject *parent) :
    QObject(parent),
    m_udev(udev), m_types(types), m_udevMonitor(0), m_udevMonitorFileDescriptor(-1), m_udevSocketNotifier(0)
{
#ifdef QT_QPA_UDEVICE_HELPER_DEBUG
    qWarning() << "New UDeviceHelper created for type" << types;
#endif

    if (!m_udev)
        return;

    m_udevMonitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_udevMonitor) {
#ifdef QT_QPA_UDEVICE_HELPER_DEBUG
        qWarning("Unable to create an Udev monitor. No devices can be detected.");
#endif
        return;
    }

    udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor, "input", 0);
    udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor, "drm", 0);
    udev_monitor_enable_receiving(m_udevMonitor);
    m_udevMonitorFileDescriptor = udev_monitor_get_fd(m_udevMonitor);

    m_udevSocketNotifier = new QSocketNotifier(m_udevMonitorFileDescriptor, QSocketNotifier::Read, this);
    connect(m_udevSocketNotifier, SIGNAL(activated(int)), this, SLOT(handleUDevNotification()));
}

QUDeviceHelper::~QUDeviceHelper()
{
    if (m_udevMonitor)
        udev_monitor_unref(m_udevMonitor);

    if (m_udev)
        udev_unref(m_udev);
}

QStringList QUDeviceHelper::scanConnectedDevices()
{
    QStringList devices;

    if (!m_udev)
        return devices;

    udev_enumerate *ue = udev_enumerate_new(m_udev);
    udev_enumerate_add_match_subsystem(ue, "input");
    udev_enumerate_add_match_subsystem(ue, "drm");

    if (m_types & UDev_Mouse)
        udev_enumerate_add_match_property(ue, "ID_INPUT_MOUSE", "1");
    if (m_types & UDev_Touchpad)
        udev_enumerate_add_match_property(ue, "ID_INPUT_TOUCHPAD", "1");
    if (m_types & UDev_Touchscreen)
        udev_enumerate_add_match_property(ue, "ID_INPUT_TOUCHSCREEN", "1");
    if (m_types & UDev_Keyboard)
        udev_enumerate_add_match_property(ue, "ID_INPUT_KEYBOARD", "1");

    if (udev_enumerate_scan_devices(ue) != 0) {
#ifdef QT_QPA_UDEVICE_HELPER_DEBUG
        qWarning() << "UDeviceHelper scan connected devices for enumeration failed";
#endif
        return devices;
    }

    udev_list_entry *entry;
    udev_list_entry_foreach (entry, udev_enumerate_get_list_entry(ue)) {
        const char *syspath = udev_list_entry_get_name(entry);
        udev_device *udevice = udev_device_new_from_syspath(m_udev, syspath);
        QString candidate = QString::fromUtf8(udev_device_get_devnode(udevice));
        if ((m_types & UDev_InputMask) && candidate.startsWith(QLatin1String("/dev/input/event")))
            devices << candidate;
        if ((m_types & UDev_VideoMask) && candidate.startsWith(QLatin1String("/dev/dri/card")))
            devices << candidate;

        udev_device_unref(udevice);
    }
    udev_enumerate_unref(ue);

#ifdef QT_QPA_UDEVICE_HELPER_DEBUG
    qWarning() << "UDeviceHelper found matching devices" << devices;
#endif

    return devices;
}

void QUDeviceHelper::handleUDevNotification()
{
    if (!m_udevMonitor)
        return;

    struct udev_device *dev;
    QString devNode;
    QUDeviceTypes types = QFlag(UDev_Unknown);

    dev = udev_monitor_receive_device(m_udevMonitor);
    if (!dev)
        goto cleanup;

    const char *action;
    action = udev_device_get_action(dev);
    if (!action)
        goto cleanup;

    const char *str;
    str = udev_device_get_devnode(dev);
    if (!str)
        goto cleanup;

    const char *subsystem;
    devNode = QString::fromUtf8(str);
    if (devNode.startsWith(QLatin1String("/dev/input/event")))
        subsystem = "input";
    else if (devNode.startsWith(QLatin1String("/dev/dri/card")))
        subsystem = "drm";
    else goto cleanup;

    // does not increase the refcount
    dev = udev_device_get_parent_with_subsystem_devtype(dev, subsystem, 0);
    if (!dev)
        goto cleanup;

    types = checkDeviceType(dev);

    if (types && (qstrcmp(action, "add") == 0))
        emit deviceDetected(devNode, types);

    if (types && (qstrcmp(action, "remove") == 0))
        emit deviceRemoved(devNode, types);

cleanup:
    udev_device_unref(dev);
}

QUDeviceHelper::QUDeviceTypes QUDeviceHelper::checkDeviceType(udev_device *dev)
{
    QUDeviceTypes types = QFlag(UDev_Unknown);

    if ((m_types & UDev_Keyboard) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD"), "1") == 0 )) {
        const char *capabilities_key = udev_device_get_sysattr_value(dev, "capabilities/key");
        QStringList val = QString::fromUtf8(capabilities_key).split(QString::fromUtf8(" "), QString::SkipEmptyParts);
        if (!val.isEmpty()) {
            bool ok;
            unsigned long long keys = val.last().toULongLong(&ok, 16);
            if (ok) {
                // Tests if the letter Q is valid for the device.  We may want to alter this test, but it seems mostly reliable.
                bool test = (keys >> KEY_Q) & 1;
                if (test)
                    types |= UDev_Keyboard;
            }
        }
    }

    if ((m_types & UDev_Mouse) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_MOUSE"), "1") == 0))
        types |= UDev_Mouse;

    if ((m_types & UDev_Touchpad) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_TOUCHPAD"), "1") == 0))
        types |= UDev_Touchpad;

    if ((m_types & UDev_Touchscreen) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_TOUCHSCREEN"), "1") == 0))
        types |= UDev_Touchscreen;

    if ((m_types & UDev_DRM) && (qstrcmp(udev_device_get_subsystem(dev), "drm") == 0))
        types |= UDev_DRM;

    return types;
}

QT_END_NAMESPACE
