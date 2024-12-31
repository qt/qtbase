// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qevdevtabletmanager_p.h"
#include "qevdevtablethandler_p.h"

#include <QtInputSupport/private/qevdevutil_p.h>

#include <QStringList>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QtDeviceDiscoverySupport/private/qdevicediscovery_p.h>
#include <private/qguiapplication_p.h>
#include <private/qinputdevicemanager_p_p.h>

QT_BEGIN_NAMESPACE

QEvdevTabletManager::QEvdevTabletManager(const QString &key, const QString &specification, QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(key);

    if (qEnvironmentVariableIsSet("QT_QPA_EVDEV_DEBUG"))
        const_cast<QLoggingCategory &>(qLcEvdevTablet()).setEnabled(QtDebugMsg, true);

    QString spec = qEnvironmentVariable("QT_QPA_EVDEV_TABLET_PARAMETERS");

    if (spec.isEmpty())
        spec = specification;

    auto parsed = QEvdevUtil::parseSpecification(spec);
    m_spec = std::move(parsed.spec);

    for (const QString &device : std::as_const(parsed.devices))
        addDevice(device);

    // when no devices specified, use device discovery to scan and monitor
    if (parsed.devices.isEmpty()) {
        qCDebug(qLcEvdevTablet, "evdevtablet: Using device discovery");
        if (auto deviceDiscovery = QDeviceDiscovery::create(QDeviceDiscovery::Device_Tablet, this)) {
            const QStringList devices = deviceDiscovery->scanConnectedDevices();
            for (const QString &device : devices)
                addDevice(device);

            connect(deviceDiscovery, &QDeviceDiscovery::deviceDetected,
                    this, &QEvdevTabletManager::addDevice);
            connect(deviceDiscovery, &QDeviceDiscovery::deviceRemoved,
                    this, &QEvdevTabletManager::removeDevice);
        }
    }
}

QEvdevTabletManager::~QEvdevTabletManager()
{
}

void QEvdevTabletManager::addDevice(const QString &deviceNode)
{
    qCDebug(qLcEvdevTablet, "Adding device at %ls", qUtf16Printable(deviceNode));
    auto handler = std::make_unique<QEvdevTabletHandlerThread>(deviceNode, m_spec);
    if (handler) {
        m_activeDevices.add(deviceNode, std::move(handler));
        updateDeviceCount();
    } else {
        qWarning("evdevtablet: Failed to open tablet device %ls", qUtf16Printable(deviceNode));
    }
}

void QEvdevTabletManager::removeDevice(const QString &deviceNode)
{
    if (m_activeDevices.remove(deviceNode)) {
        qCDebug(qLcEvdevTablet, "Removing device at %ls", qUtf16Printable(deviceNode));
        updateDeviceCount();
    }
}

void QEvdevTabletManager::updateDeviceCount()
{
    QInputDeviceManagerPrivate::get(QGuiApplicationPrivate::inputDeviceManager())->setDeviceCount(
        QInputDeviceManager::DeviceTypeTablet, m_activeDevices.count());
}

QT_END_NAMESPACE
