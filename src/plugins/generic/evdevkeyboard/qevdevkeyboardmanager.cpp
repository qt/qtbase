/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qevdevkeyboardmanager.h"

#include <QStringList>
#include <QDebug>
#include <QCoreApplication>

#include <linux/input.h>
#include <linux/kd.h>

//#define QT_QPA_KEYMAP_DEBUG

QT_BEGIN_NAMESPACE

QEvdevKeyboardManager::QEvdevKeyboardManager(const QString &key, const QString &specification)
    : m_udev(0), m_udevMonitor(0), m_udevMonitorFileDescriptor(-1), m_udevSocketNotifier(0)
{
    Q_UNUSED(key);

    bool useUDev = false;
    QStringList args = specification.split(QLatin1Char(':'));
    QStringList devices;

    foreach (const QString &arg, args) {
        if (arg.startsWith("udev") && !arg.contains("no")) {
            useUDev = true;
        } else if (arg.startsWith("/dev/")) {
            // if device is specified try to use it
            devices.append(arg);
            args.removeAll(arg);
        }
    }

    m_spec = args.join(":");

    // add all keyboards for devices specified in the argument list
    foreach (const QString &device, devices)
        addKeyboard(device);

    // no udev and no devices specified, try a fallback
    if (!useUDev && devices.isEmpty()) {
        addKeyboard();
        return;
    }

    m_udev = udev_new();
    if (!m_udev) {
        qWarning() << "Failed to read udev configuration. Try to open a keyboard without it";
        addKeyboard();
    } else {
        // Look for already attached devices:
        parseConnectedDevices();
        // Watch for device add/remove
        startWatching();
    }
}

QEvdevKeyboardManager::~QEvdevKeyboardManager()
{
    // cleanup udev related resources
    stopWatching();
    if (m_udev)
        udev_unref(m_udev);

    // cleanup all resources of connected keyboards
    qDeleteAll(m_keyboards);
    m_keyboards.clear();
}

void QEvdevKeyboardManager::addKeyboard(const QString &devnode)
{
    QString specification = m_spec;
    QString deviceString = devnode;

    if (!deviceString.isEmpty()) {
        specification.append(":");
        specification.append(deviceString);
    } else {
        deviceString = "default";
    }

#ifdef QT_QPA_KEYMAP_DEBUG
    qWarning() << "Adding keyboard at" << deviceString;
#endif

    QEvdevKeyboardHandler *keyboard;
    keyboard = QEvdevKeyboardHandler::createLinuxInputKeyboardHandler("EvdevKeyboard", specification);
    if (keyboard)
        m_keyboards.insert(deviceString, keyboard);
    else
        qWarning() << "Failed to open keyboard";
}

void QEvdevKeyboardManager::removeKeyboard(const QString &devnode)
{
    if (m_keyboards.contains(devnode)) {
#ifdef QT_QPA_KEYMAP_DEBUG
    qWarning() << "Removing keyboard at" << devnode;
#endif
        QEvdevKeyboardHandler *keyboard = m_keyboards.value(devnode);
        m_keyboards.remove(devnode);
        delete keyboard;
    }
}

void QEvdevKeyboardManager::startWatching()
{
    m_udevMonitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_udevMonitor) {
#ifdef QT_QPA_KEYMAP_DEBUG
        qWarning("Unable to create an Udev monitor. No devices can be detected.");
#endif
        return;
    }

    udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor, "input", NULL);
    udev_monitor_enable_receiving(m_udevMonitor);
    m_udevMonitorFileDescriptor = udev_monitor_get_fd(m_udevMonitor);

    m_udevSocketNotifier = new QSocketNotifier(m_udevMonitorFileDescriptor, QSocketNotifier::Read, this);
    connect(m_udevSocketNotifier, SIGNAL(activated(int)), this, SLOT(deviceDetected()));
}

void QEvdevKeyboardManager::stopWatching()
{
    if (m_udevSocketNotifier)
        delete m_udevSocketNotifier;

    if (m_udevMonitor)
        udev_monitor_unref(m_udevMonitor);

    m_udevSocketNotifier = 0;
    m_udevMonitor = 0;
    m_udevMonitorFileDescriptor = 0;
}

void QEvdevKeyboardManager::deviceDetected()
{
    if (!m_udevMonitor)
        return;

    struct udev_device *dev;
    dev = udev_monitor_receive_device(m_udevMonitor);
    if (!dev)
        return;

    if (qstrcmp(udev_device_get_action(dev), "add") == 0) {
        checkDevice(dev);
    } else {
        // We can't determine what the device was, so we handle false positives outside of this class
        QString str = udev_device_get_devnode(dev);
        if (!str.isEmpty())
            removeKeyboard(str);
    }

    udev_device_unref(dev);
}

void QEvdevKeyboardManager::parseConnectedDevices()
{
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices;
    struct udev_list_entry *dev_list_entry;
    struct udev_device *dev;
    const char *str;

    enumerate = udev_enumerate_new(m_udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(dev_list_entry, devices) {
        str = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(m_udev, str);
        checkDevice(dev);
    }

    udev_enumerate_unref(enumerate);
}

void QEvdevKeyboardManager::checkDevice(udev_device *dev)
{
    const char *str;
    QString devnode;

    str = udev_device_get_devnode(dev);
    if (!str)
        return;

    devnode = str;

    dev = udev_device_get_parent_with_subsystem_devtype(dev, "input", NULL);
    if (!dev)
        return;

    str = udev_device_get_sysattr_value(dev, "capabilities/key");
    QStringList val = QString(str).split(' ', QString::SkipEmptyParts);

    bool ok;
    unsigned long long keys = val.last().toULongLong(&ok, 16);
    if (!ok)
        return;

    // Tests if the letter Q is valid for the device.  We may want to alter this test, but it seems mostly reliable.
    bool test = (keys >> KEY_Q) & 1;
    if (test) {
        addKeyboard(devnode);
        return;
    }
}

QT_END_NAMESPACE
