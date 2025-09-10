// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdevicediscovery_udev_p.h"

#include <QStringList>
#include <QCoreApplication>
#include <QObject>
#include <QHash>
#include <QSocketNotifier>
#include <QLoggingCategory>

#ifdef Q_OS_FREEBSD
#include <dev/evdev/input.h>
#else
#include <linux/input.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcDD, "qt.qpa.input")

QDeviceDiscovery *QDeviceDiscovery::create(QDeviceTypes types, QObject *parent)
{
    qCDebug(lcDD) << "udev device discovery for type" << types;

    QDeviceDiscovery *helper = nullptr;
    struct udev *udev;

    udev = udev_new();
    if (udev) {
        helper = new QDeviceDiscoveryUDev(types, udev, parent);
    } else {
        qWarning("Failed to get udev library context");
    }

    return helper;
}

QDeviceDiscoveryUDev::QDeviceDiscoveryUDev(QDeviceTypes types, struct udev *udev, QObject *parent) :
    QDeviceDiscovery(types, parent),
    m_udev(udev)
{
    if (!m_udev)
        return;

    m_udevMonitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_udevMonitor) {
        qWarning("Unable to create an udev monitor. No devices can be detected.");
        return;
    }

    udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor, "input", 0);
    udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor, "drm", 0);
    udev_monitor_enable_receiving(m_udevMonitor);
    m_udevMonitorFileDescriptor = udev_monitor_get_fd(m_udevMonitor);

    m_udevSocketNotifier = new QSocketNotifier(m_udevMonitorFileDescriptor, QSocketNotifier::Read, this);
    connect(m_udevSocketNotifier, SIGNAL(activated(QSocketDescriptor)), this, SLOT(handleUDevNotification()));
}

QDeviceDiscoveryUDev::~QDeviceDiscoveryUDev()
{
    if (m_udevMonitor)
        udev_monitor_unref(m_udevMonitor);

    if (m_udev)
        udev_unref(m_udev);
}

QStringList QDeviceDiscoveryUDev::scanConnectedDevices()
{
    QStringList devices;

    if (!m_udev)
        return devices;

    udev_enumerate *ue = udev_enumerate_new(m_udev);
    udev_enumerate_add_match_subsystem(ue, "input");
    udev_enumerate_add_match_subsystem(ue, "drm");

    if (m_types & Device_Mouse)
        udev_enumerate_add_match_property(ue, "ID_INPUT_MOUSE", "1");
    if (m_types & Device_Touchpad)
        udev_enumerate_add_match_property(ue, "ID_INPUT_TOUCHPAD", "1");
    if (m_types & Device_Touchscreen)
        udev_enumerate_add_match_property(ue, "ID_INPUT_TOUCHSCREEN", "1");
    if (m_types & Device_Keyboard) {
        udev_enumerate_add_match_property(ue, "ID_INPUT_KEYBOARD", "1");
        udev_enumerate_add_match_property(ue, "ID_INPUT_KEY", "1");
    }
    if (m_types & Device_Tablet)
        udev_enumerate_add_match_property(ue, "ID_INPUT_TABLET", "1");
    if (m_types & Device_Joystick)
        udev_enumerate_add_match_property(ue, "ID_INPUT_JOYSTICK", "1");

    if (udev_enumerate_scan_devices(ue) != 0) {
        qWarning("Failed to scan devices");
        return devices;
    }

    udev_list_entry *entry;
    udev_list_entry_foreach (entry, udev_enumerate_get_list_entry(ue)) {
        const char *syspath = udev_list_entry_get_name(entry);
        udev_device *udevice = udev_device_new_from_syspath(m_udev, syspath);
        QString candidate = QString::fromUtf8(udev_device_get_devnode(udevice));
        if ((m_types & Device_InputMask) && candidate.startsWith(QT_EVDEV_DEVICE ""_L1))
            devices << candidate;
        if ((m_types & Device_VideoMask) && candidate.startsWith(QT_DRM_DEVICE ""_L1)) {
            if (m_types & Device_DRM_PrimaryGPU) {
                udev_device *pci = udev_device_get_parent_with_subsystem_devtype(udevice, "pci", 0);
                if (pci) {
                    if (qstrcmp(udev_device_get_sysattr_value(pci, "boot_vga"), "1") == 0)
                        devices << candidate;
                }
            } else
                devices << candidate;
        }

        udev_device_unref(udevice);
    }
    udev_enumerate_unref(ue);

    qCDebug(lcDD) << "Found matching devices" << devices;

    return devices;
}

void QDeviceDiscoveryUDev::handleUDevNotification()
{
    if (!m_udevMonitor)
        return;

    struct udev_device *dev;
    QString devNode;

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
    if (devNode.startsWith(QT_EVDEV_DEVICE ""_L1))
        subsystem = "input";
    else if (devNode.startsWith(QT_DRM_DEVICE ""_L1))
        subsystem = "drm";
    else goto cleanup;

    // if we cannot determine a type, walk up the device tree
    if (!checkDeviceType(dev)) {
        // does not increase the refcount
        struct udev_device *parent_dev = udev_device_get_parent_with_subsystem_devtype(dev, subsystem, 0);
        if (!parent_dev)
            goto cleanup;

        if (!checkDeviceType(parent_dev))
            goto cleanup;
    }

    if (qstrcmp(action, "add") == 0)
        emit deviceDetected(devNode);

    if (qstrcmp(action, "remove") == 0)
        emit deviceRemoved(devNode);

cleanup:
    udev_device_unref(dev);
}

bool QDeviceDiscoveryUDev::checkDeviceType(udev_device *dev)
{
    if (!dev)
        return false;

    if ((m_types & Device_Keyboard) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD"), "1") == 0 )) {
        const QString capabilities_key = QString::fromUtf8(udev_device_get_sysattr_value(dev, "capabilities/key"));
        const auto val = QStringView{capabilities_key}.split(u' ', Qt::SkipEmptyParts);
        if (!val.isEmpty()) {
            bool ok;
            unsigned long long keys = val.last().toULongLong(&ok, 16);
            if (ok) {
                // Tests if the letter Q is valid for the device.  We may want to alter this test, but it seems mostly reliable.
                bool test = (keys >> KEY_Q) & 1;
                if (test)
                    return true;
            }
        }
    }

    if ((m_types & Device_Keyboard) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_KEY"), "1") == 0 ))
        return true;

    if ((m_types & Device_Mouse) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_MOUSE"), "1") == 0))
        return true;

    if ((m_types & Device_Touchpad) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_TOUCHPAD"), "1") == 0))
        return true;

    if ((m_types & Device_Touchscreen) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_TOUCHSCREEN"), "1") == 0))
        return true;

    if ((m_types & Device_Tablet) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_TABLET"), "1") == 0))
        return true;

    if ((m_types & Device_Joystick) && (qstrcmp(udev_device_get_property_value(dev, "ID_INPUT_JOYSTICK"), "1") == 0))
        return true;

    if ((m_types & Device_DRM) && (qstrcmp(udev_device_get_subsystem(dev), "drm") == 0))
        return true;

    return false;
}

QT_END_NAMESPACE

#include "moc_qdevicediscovery_udev_p.cpp"
