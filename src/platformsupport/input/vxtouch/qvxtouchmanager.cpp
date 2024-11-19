// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvxtouchmanager_p.h"
#include "qvxtouchhandler_p.h"

#include <QtInputSupport/private/qevdevutil_p.h>

#include <QStringList>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QtDeviceDiscoverySupport/private/qdevicediscovery_p.h>
#include <private/qguiapplication_p.h>
#include <private/qinputdevicemanager_p_p.h>

QT_BEGIN_NAMESPACE

QVxTouchManager::QVxTouchManager(const QString &key, const QString &specification, QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(key);

    if (qEnvironmentVariableIsSet("QT_QPA_VXEVDEV_DEBUG"))
        const_cast<QLoggingCategory &>(qLcVxTouch()).setEnabled(QtDebugMsg, true);

    QString spec = QString::fromLocal8Bit(qgetenv("QT_QPA_VXEVDEV_TOUCHSCREEN_PARAMETERS"));

    if (spec.isEmpty())
        spec = specification;

    auto parsed = QEvdevUtil::parseSpecification(spec);
    m_spec = std::move(parsed.spec);

    for (const QString &device : std::as_const(parsed.devices))
        addDevice(device);

    // when no devices specified, use device discovery to scan and monitor
    if (parsed.devices.isEmpty()) {
        qCDebug(qLcVxTouch, "vxtouch: Using device discovery");
        if (auto deviceDiscovery = QDeviceDiscovery::create(QDeviceDiscovery::Device_Touchpad | QDeviceDiscovery::Device_Touchscreen, this)) {
            const QStringList devices = deviceDiscovery->scanConnectedDevices();
            for (const QString &device : devices)
                addDevice(device);

            connect(deviceDiscovery, &QDeviceDiscovery::deviceDetected,
                    this, &QVxTouchManager::addDevice);
            connect(deviceDiscovery, &QDeviceDiscovery::deviceRemoved,
                    this, &QVxTouchManager::removeDevice);
        }
    }
}

QVxTouchManager::~QVxTouchManager()
{
}

void QVxTouchManager::addDevice(const QString &deviceNode)
{
    qCDebug(qLcVxTouch, "vxtouch: Adding device at %ls", qUtf16Printable(deviceNode));
    auto handler = std::make_unique<QVxTouchScreenHandlerThread>(deviceNode, m_spec);
    if (handler) {
        connect(handler.get(), &QVxTouchScreenHandlerThread::touchDeviceRegistered, this, &QVxTouchManager::updateInputDeviceCount);
        m_activeDevices.add(deviceNode, std::move(handler));
    } else {
        qWarning("vxtouch: Failed to open touch device %ls", qUtf16Printable(deviceNode));
    }
}

void QVxTouchManager::removeDevice(const QString &deviceNode)
{
    if (m_activeDevices.remove(deviceNode)) {
        qCDebug(qLcVxTouch, "vxtouch: Removing device at %ls", qUtf16Printable(deviceNode));
        updateInputDeviceCount();
    }
}

void QVxTouchManager::updateInputDeviceCount()
{
    int registeredTouchDevices = 0;
    for (const auto &device : m_activeDevices) {
        if (device.handler->isPointingDeviceRegistered())
            ++registeredTouchDevices;
    }

    qCDebug(qLcVxTouch, "vxtouch: Updating QInputDeviceManager device count: %d touch devices, %d pending handler(s)",
            registeredTouchDevices, m_activeDevices.count() - registeredTouchDevices);

    QInputDeviceManagerPrivate::get(QGuiApplicationPrivate::inputDeviceManager())->setDeviceCount(
        QInputDeviceManager::DeviceTypeTouch, registeredTouchDevices);
}

QT_END_NAMESPACE
