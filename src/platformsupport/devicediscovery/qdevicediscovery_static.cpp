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

#include "qdevicediscovery_p.h"

#include <QStringList>
#include <QCoreApplication>
#include <QObject>
#include <QHash>
#include <QDir>

//#define QT_QPA_DEVICE_DISCOVERY_DEBUG

#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
#include <QtDebug>
#endif

QT_BEGIN_NAMESPACE

QDeviceDiscovery *QDeviceDiscovery::create(QDeviceTypes types, QObject *parent)
{
    return new QDeviceDiscovery(types, parent);
}

QDeviceDiscovery::QDeviceDiscovery(QDeviceTypes types, QObject *parent) :
    QObject(parent),
    m_types(types)
{
#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
    qWarning() << "New static DeviceDiscovery created for type" << types;
#endif
}

QDeviceDiscovery::~QDeviceDiscovery()
{
}

QStringList QDeviceDiscovery::scanConnectedDevices()
{
    QStringList devices;

    // check for input devices
    QDir dir(QString::fromLatin1(QT_EVDEV_DEVICE_PATH));
    dir.setFilter(QDir::System);

    foreach (const QString &deviceFile, dir.entryList()) {
        if (checkDeviceType(deviceFile))
            devices << (dir.absolutePath() + QString::fromLatin1("/") + deviceFile);
    }

    // check for drm devices
    dir.setPath(QString::fromLatin1(QT_DRM_DEVICE_PATH));
    foreach (const QString &deviceFile, dir.entryList()) {
        if (checkDeviceType(deviceFile))
            devices << (dir.absolutePath() + QString::fromLatin1("/") + deviceFile);
    }

#ifdef QT_QPA_DEVICE_DISCOVERY_DEBUG
    qWarning() << "Static DeviceDiscovery found matching devices" << devices;
#endif

    return devices;
}

bool QDeviceDiscovery::checkDeviceType(const QString &device)
{
    if ((m_types & (Device_Keyboard | Device_Mouse | Device_Touchpad | Device_Touchscreen)) && device.startsWith(QString::fromLatin1(QT_EVDEV_DEVICE_PREFIX)))
       return true;

    if ((m_types & Device_DRM) && device.startsWith(QString::fromLatin1(QT_DRM_DEVICE_PREFIX)))
        return true;

    return false;
}

QT_END_NAMESPACE
