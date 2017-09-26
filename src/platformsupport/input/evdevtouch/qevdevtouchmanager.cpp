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

#include <QStringList>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QtDeviceDiscoverySupport/private/qdevicediscovery_p.h>
#include <private/qguiapplication_p.h>
#include <private/qinputdevicemanager_p_p.h>

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

    QStringList args = spec.split(QLatin1Char(':'));
    QStringList devices;

    foreach (const QString &arg, args) {
        if (arg.startsWith(QLatin1String("/dev/"))) {
            devices.append(arg);
            args.removeAll(arg);
        }
    }

    // build new specification without /dev/ elements
    m_spec = args.join(QLatin1Char(':'));

    foreach (const QString &device, devices)
        addDevice(device);

    // when no devices specified, use device discovery to scan and monitor
    if (devices.isEmpty()) {
        qCDebug(qLcEvdevTouch) << "evdevtouch: Using device discovery";
        m_deviceDiscovery = QDeviceDiscovery::create(QDeviceDiscovery::Device_Touchpad | QDeviceDiscovery::Device_Touchscreen, this);
        if (m_deviceDiscovery) {
            const QStringList devices = m_deviceDiscovery->scanConnectedDevices();
            for (const QString &device : devices)
                addDevice(device);

            connect(m_deviceDiscovery, &QDeviceDiscovery::deviceDetected,
                    this, &QEvdevTouchManager::addDevice);
            connect(m_deviceDiscovery, &QDeviceDiscovery::deviceRemoved,
                    this, &QEvdevTouchManager::removeDevice);
        }
    }
}

QEvdevTouchManager::~QEvdevTouchManager()
{
    qDeleteAll(m_activeDevices);
}

void QEvdevTouchManager::addDevice(const QString &deviceNode)
{
    qCDebug(qLcEvdevTouch) << "evdevtouch: Adding device at" << deviceNode;
    QEvdevTouchScreenHandlerThread *handler;
    handler = new QEvdevTouchScreenHandlerThread(deviceNode, m_spec);
    if (handler) {
        m_activeDevices.insert(deviceNode, handler);
        connect(handler, &QEvdevTouchScreenHandlerThread::touchDeviceRegistered, this, &QEvdevTouchManager::updateInputDeviceCount);
    } else {
        qWarning("evdevtouch: Failed to open touch device %s", qPrintable(deviceNode));
    }
}

void QEvdevTouchManager::removeDevice(const QString &deviceNode)
{
    if (m_activeDevices.contains(deviceNode)) {
        qCDebug(qLcEvdevTouch) << "evdevtouch: Removing device at" << deviceNode;
        QEvdevTouchScreenHandlerThread *handler = m_activeDevices.value(deviceNode);
        m_activeDevices.remove(deviceNode);
        delete handler;

        updateInputDeviceCount();
    }
}

void QEvdevTouchManager::updateInputDeviceCount()
{
    int registeredTouchDevices = 0;
    Q_FOREACH (QEvdevTouchScreenHandlerThread *handler, m_activeDevices) {
        if (handler->isTouchDeviceRegistered())
            ++registeredTouchDevices;
    }

    qCDebug(qLcEvdevTouch) << "evdevtouch: Updating QInputDeviceManager device count:" << registeredTouchDevices << " touch devices,"
                           << m_activeDevices.count() - registeredTouchDevices << "pending handler(s)" ;

    QInputDeviceManagerPrivate::get(QGuiApplicationPrivate::inputDeviceManager())->setDeviceCount(
        QInputDeviceManager::DeviceTypeTouch, registeredTouchDevices);
}

QT_END_NAMESPACE
