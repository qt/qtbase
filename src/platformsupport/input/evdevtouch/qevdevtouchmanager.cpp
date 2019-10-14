/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qevdevtouchmanager_p.h"
#include "qevdevtouchhandler_p.h"

#include <QtInputSupport/private/qevdevutil_p.h>

#include <QStringList>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QtDeviceDiscoverySupport/private/qdevicediscovery_p.h>
#include <private/qguiapplication_p.h>
#include <private/qinputdevicemanager_p_p.h>
#include <private/qmemory_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcEvdevTouch)

QEvdevTouchManager::QEvdevTouchManager(const QString &key, const QString &specification, QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(key);

    if (qEnvironmentVariableIsSet("QT_QPA_EVDEV_DEBUG"))
        const_cast<QLoggingCategory &>(qLcEvdevTouch()).setEnabled(QtDebugMsg, true);

    QString spec = QString::fromLocal8Bit(qgetenv("QT_QPA_EVDEV_TOUCHSCREEN_PARAMETERS"));

    if (spec.isEmpty())
        spec = specification;

    auto parsed = QEvdevUtil::parseSpecification(spec);
    m_spec = std::move(parsed.spec);

    for (const QString &device : qAsConst(parsed.devices))
        addDevice(device);

    // when no devices specified, use device discovery to scan and monitor
    if (parsed.devices.isEmpty()) {
        qCDebug(qLcEvdevTouch, "evdevtouch: Using device discovery");
        if (auto deviceDiscovery = QDeviceDiscovery::create(QDeviceDiscovery::Device_Touchpad | QDeviceDiscovery::Device_Touchscreen, this)) {
            const QStringList devices = deviceDiscovery->scanConnectedDevices();
            for (const QString &device : devices)
                addDevice(device);

            connect(deviceDiscovery, &QDeviceDiscovery::deviceDetected,
                    this, &QEvdevTouchManager::addDevice);
            connect(deviceDiscovery, &QDeviceDiscovery::deviceRemoved,
                    this, &QEvdevTouchManager::removeDevice);
        }
    }
}

QEvdevTouchManager::~QEvdevTouchManager()
{
}

void QEvdevTouchManager::addDevice(const QString &deviceNode)
{
    qCDebug(qLcEvdevTouch, "evdevtouch: Adding device at %ls", qUtf16Printable(deviceNode));
    auto handler = qt_make_unique<QEvdevTouchScreenHandlerThread>(deviceNode, m_spec);
    if (handler) {
        connect(handler.get(), &QEvdevTouchScreenHandlerThread::touchDeviceRegistered, this, &QEvdevTouchManager::updateInputDeviceCount);
        m_activeDevices.add(deviceNode, std::move(handler));
    } else {
        qWarning("evdevtouch: Failed to open touch device %ls", qUtf16Printable(deviceNode));
    }
}

void QEvdevTouchManager::removeDevice(const QString &deviceNode)
{
    if (m_activeDevices.remove(deviceNode)) {
        qCDebug(qLcEvdevTouch, "evdevtouch: Removing device at %ls", qUtf16Printable(deviceNode));
        updateInputDeviceCount();
    }
}

void QEvdevTouchManager::updateInputDeviceCount()
{
    int registeredTouchDevices = 0;
    for (const auto &device : m_activeDevices) {
        if (device.handler->isTouchDeviceRegistered())
            ++registeredTouchDevices;
    }

    qCDebug(qLcEvdevTouch, "evdevtouch: Updating QInputDeviceManager device count: %d touch devices, %d pending handler(s)",
            registeredTouchDevices, m_activeDevices.count() - registeredTouchDevices);

    QInputDeviceManagerPrivate::get(QGuiApplicationPrivate::inputDeviceManager())->setDeviceCount(
        QInputDeviceManager::DeviceTypeTouch, registeredTouchDevices);
}

QT_END_NAMESPACE
