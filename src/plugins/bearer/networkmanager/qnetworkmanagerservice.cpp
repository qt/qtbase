/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QObject>
#include <QList>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusPendingCall>

#include "qnetworkmanagerservice.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QNetworkManagerInterfacePrivate
{
public:
    QDBusInterface *connectionInterface;
    bool valid;
};

QNetworkManagerInterface::QNetworkManagerInterface(QObject *parent)
        : QObject(parent)
{
    d = new QNetworkManagerInterfacePrivate();
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                QLatin1String(NM_DBUS_PATH),
                                                QLatin1String(NM_DBUS_INTERFACE),
                                                QDBusConnection::systemBus(),parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;

    QDBusInterface managerPropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  QLatin1String(NM_DBUS_PATH),
                                                  QLatin1String("org.freedesktop.DBus.Properties"),
                                                  QDBusConnection::systemBus());
    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE);
    QDBusPendingReply<QVariantMap> propsReply
            = managerPropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);
    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusPendingReply<QList <QDBusObjectPath> > nmReply
            = d->connectionInterface->call(QLatin1String("GetDevices"));
    nmReply.waitForFinished();
    if (!nmReply.isError()) {
        devicesPathList = nmReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
}

QNetworkManagerInterface::~QNetworkManagerInterface()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterface::isValid()
{
    return d->valid;
}

bool QNetworkManagerInterface::setConnections()
{
    if (!isValid())
        return false;

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));

    bool allOk = false;
    if (QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("DeviceAdded"),
                                this,SIGNAL(deviceAdded(QDBusObjectPath)))) {
        allOk = true;
    }
    if (QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  QLatin1String(NM_DBUS_PATH),
                                  QLatin1String(NM_DBUS_INTERFACE),
                                  QLatin1String("DeviceRemoved"),
                                  this,SIGNAL(deviceRemoved(QDBusObjectPath)))) {
        allOk = true;
    }

    return allOk;
}

QDBusInterface *QNetworkManagerInterface::connectionInterface()  const
{
    return d->connectionInterface;
}

QList <QDBusObjectPath> QNetworkManagerInterface::getDevices()
{
    if (devicesPathList.isEmpty()) {
        qWarning() << "using blocking call!";
        QDBusReply<QList<QDBusObjectPath> > reply = d->connectionInterface->call(QLatin1String("GetDevices"));
        devicesPathList = reply.value();
    }
    return devicesPathList;
}

void QNetworkManagerInterface::activateConnection(QDBusObjectPath connectionPath,
                                                  QDBusObjectPath devicePath,
                                                  QDBusObjectPath specificObject)
{
    QDBusPendingCall pendingCall = d->connectionInterface->asyncCall(QLatin1String("ActivateConnection"),
                                                                    QVariant::fromValue(connectionPath),
                                                                    QVariant::fromValue(devicePath),
                                                                    QVariant::fromValue(specificObject));

   QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(pendingCall);
   connect(callWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                    this, SIGNAL(activationFinished(QDBusPendingCallWatcher*)));
}

void QNetworkManagerInterface::deactivateConnection(QDBusObjectPath connectionPath)  const
{
    d->connectionInterface->asyncCall(QLatin1String("DeactivateConnection"), QVariant::fromValue(connectionPath));
}

bool QNetworkManagerInterface::wirelessEnabled()  const
{
    if (propertyMap.contains("WirelessEnabled"))
        return propertyMap.value("WirelessEnabled").toBool();
    return false;
}

bool QNetworkManagerInterface::wirelessHardwareEnabled()  const
{
    if (propertyMap.contains("WirelessHardwareEnabled"))
        return propertyMap.value("WirelessHardwareEnabled").toBool();
    return false;
}

QList <QDBusObjectPath> QNetworkManagerInterface::activeConnections() const
{
    if (propertyMap.contains("ActiveConnections")) {

        const QDBusArgument &dbusArgs = propertyMap.value("ActiveConnections").value<QDBusArgument>();
        QDBusObjectPath path;
        QList <QDBusObjectPath> list;

        dbusArgs.beginArray();
        while (!dbusArgs.atEnd()) {
            dbusArgs >> path;
            list.append(path);
        }
        dbusArgs.endArray();

        return list;
    }

    QList <QDBusObjectPath> list;
    list << QDBusObjectPath();
    return list;
}

QNetworkManagerInterface::NMState QNetworkManagerInterface::state()
{
    if (propertyMap.contains("State"))
        return static_cast<QNetworkManagerInterface::NMState>(propertyMap.value("State").toUInt());
    return QNetworkManagerInterface::NM_STATE_UNKNOWN;
}

QString QNetworkManagerInterface::version() const
{
    if (propertyMap.contains("Version"))
        return propertyMap.value("Version").toString();
    return QString();
}

void QNetworkManagerInterface::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());

        if (i.key() == QStringLiteral("State")) {
            quint32 state = i.value().toUInt();
            if (state == NM_DEVICE_STATE_ACTIVATED
                || state == NM_DEVICE_STATE_DISCONNECTED
                || state == NM_DEVICE_STATE_UNAVAILABLE
                || state == NM_DEVICE_STATE_FAILED) {
                Q_EMIT propertiesChanged(map);
                Q_EMIT stateChanged(state);
            }
        } else if (i.key() == QStringLiteral("ActiveConnections")) {
            Q_EMIT propertiesChanged(map);
        }
    }
}

class QNetworkManagerInterfaceAccessPointPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerInterfaceAccessPoint::QNetworkManagerInterfaceAccessPoint(const QString &dbusPathName, QObject *parent)
        : QObject(parent)
{
    d = new QNetworkManagerInterfaceAccessPointPrivate();
    d->path = dbusPathName;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_ACCESS_POINT),
                                                QDBusConnection::systemBus(),parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    QDBusInterface accessPointPropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  d->path,
                                                  QLatin1String("org.freedesktop.DBus.Properties"),
                                                  QDBusConnection::systemBus());

    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_ACCESS_POINT);
    QDBusPendingReply<QVariantMap> propsReply
            = accessPointPropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);
    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  d->path,
                                  QLatin1String(NM_DBUS_INTERFACE_ACCESS_POINT),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));

    d->valid = true;

}

QNetworkManagerInterfaceAccessPoint::~QNetworkManagerInterfaceAccessPoint()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterfaceAccessPoint::isValid()
{
    return d->valid;
}

bool QNetworkManagerInterfaceAccessPoint::setConnections()
{
    if (!isValid())
        return false;

    return true;
}

QDBusInterface *QNetworkManagerInterfaceAccessPoint::connectionInterface() const
{
    return d->connectionInterface;
}

quint32 QNetworkManagerInterfaceAccessPoint::flags() const
{
    if (propertyMap.contains("Flags"))
        return propertyMap.value("Flags").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceAccessPoint::wpaFlags() const
{
    if (propertyMap.contains("WpaFlags"))
        return propertyMap.value("WpaFlags").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceAccessPoint::rsnFlags() const
{
    if (propertyMap.contains("RsnFlags"))
        return propertyMap.value("RsnFlags").toUInt();
    return 0;
}

QString QNetworkManagerInterfaceAccessPoint::ssid() const
{
    if (propertyMap.contains("Ssid"))
        return propertyMap.value("Ssid").toString();
    return QString();
}

quint32 QNetworkManagerInterfaceAccessPoint::frequency() const
{
    if (propertyMap.contains("Frequency"))
        return propertyMap.value("Frequency").toUInt();
    return 0;
}

QString QNetworkManagerInterfaceAccessPoint::hwAddress() const
{
    if (propertyMap.contains("HwAddress"))
        return propertyMap.value("HwAddress").toString();
    return QString();
}

quint32 QNetworkManagerInterfaceAccessPoint::mode() const
{
    if (propertyMap.contains("Mode"))
        return propertyMap.value("Mode").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceAccessPoint::maxBitrate() const
{
    if (propertyMap.contains("MaxBitrate"))
        return propertyMap.value("MaxBitrate").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceAccessPoint::strength() const
{
    if (propertyMap.contains("Strength"))
        return propertyMap.value("Strength").toUInt();
    return 0;
}

void QNetworkManagerInterfaceAccessPoint::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());
    }
}

class QNetworkManagerInterfaceDevicePrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerInterfaceDevice::QNetworkManagerInterfaceDevice(const QString &deviceObjectPath, QObject *parent)
        : QObject(parent)
{

    d = new QNetworkManagerInterfaceDevicePrivate();
    d->path = deviceObjectPath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_DEVICE),
                                                QDBusConnection::systemBus(),parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    QDBusInterface devicePropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  d->path,
                                                  QLatin1String("org.freedesktop.DBus.Properties"),
                                                  QDBusConnection::systemBus(),parent);

    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_DEVICE);
    QDBusPendingReply<QVariantMap> propsReply
            = devicePropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);

    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  d->path,
                                  QLatin1String(NM_DBUS_INTERFACE_DEVICE),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
    d->valid = true;
}

QNetworkManagerInterfaceDevice::~QNetworkManagerInterfaceDevice()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterfaceDevice::isValid()
{
    return d->valid;
}

bool QNetworkManagerInterfaceDevice::setConnections()
{
    if (!isValid())
        return false;

    return true;
}

QDBusInterface *QNetworkManagerInterfaceDevice::connectionInterface() const
{
    return d->connectionInterface;
}

QString QNetworkManagerInterfaceDevice::udi() const
{
    if (propertyMap.contains("Udi"))
        return propertyMap.value("Udi").toString();
    return QString();
}

QString QNetworkManagerInterfaceDevice::networkInterface() const
{
    if (propertyMap.contains("Interface"))
        return propertyMap.value("Interface").toString();
    return QString();
}

quint32 QNetworkManagerInterfaceDevice::ip4Address() const
{
    if (propertyMap.contains("Ip4Address"))
        return propertyMap.value("Ip4Address").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceDevice::state() const
{
    if (propertyMap.contains("State"))
        return propertyMap.value("State").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceDevice::deviceType() const
{
    if (propertyMap.contains("DeviceType"))
        return propertyMap.value("DeviceType").toUInt();
    return 0;
}

QDBusObjectPath QNetworkManagerInterfaceDevice::ip4config() const
{
    if (propertyMap.contains("Ip4Config"))
        return propertyMap.value("Ip4Config").value<QDBusObjectPath>();
    return QDBusObjectPath();
}

void QNetworkManagerInterfaceDevice::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        if (i.key() == QStringLiteral("AvailableConnections")) { //Device
            const QDBusArgument &dbusArgs = i.value().value<QDBusArgument>();
            QDBusObjectPath path;
            QStringList paths;
            dbusArgs.beginArray();
            while (!dbusArgs.atEnd()) {
                dbusArgs >> path;
                paths << path.path();
            }
            dbusArgs.endArray();
            Q_EMIT connectionsChanged(paths);
        }
        propertyMap.insert(i.key(),i.value());
    }
    Q_EMIT propertiesChanged(map);
}

class QNetworkManagerInterfaceDeviceWiredPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerInterfaceDeviceWired::QNetworkManagerInterfaceDeviceWired(const QString &ifaceDevicePath, QObject *parent)
    : QObject(parent)
{
    d = new QNetworkManagerInterfaceDeviceWiredPrivate();
    d->path = ifaceDevicePath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRED),
                                                QDBusConnection::systemBus(), parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    QDBusInterface deviceWiredPropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  d->path,
                                                  QLatin1String("org.freedesktop.DBus.Properties"),
                                                  QDBusConnection::systemBus(),parent);

    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRED);
    QDBusPendingReply<QVariantMap> propsReply
            = deviceWiredPropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);

    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  d->path,
                                  QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRED),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));

    d->valid = true;
}

QNetworkManagerInterfaceDeviceWired::~QNetworkManagerInterfaceDeviceWired()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterfaceDeviceWired::isValid()
{

    return d->valid;
}

bool QNetworkManagerInterfaceDeviceWired::setConnections()
{
    if (!isValid())
        return false;
    return true;
}

QDBusInterface *QNetworkManagerInterfaceDeviceWired::connectionInterface() const
{
    return d->connectionInterface;
}

QString QNetworkManagerInterfaceDeviceWired::hwAddress() const
{
    if (propertyMap.contains("HwAddress"))
        return propertyMap.value("HwAddress").toString();
    return QString();
}

quint32 QNetworkManagerInterfaceDeviceWired::speed() const
{
    if (propertyMap.contains("Speed"))
        return propertyMap.value("Speed").toUInt();
    return 0;
}

bool QNetworkManagerInterfaceDeviceWired::carrier() const
{
    if (propertyMap.contains("Carrier"))
        return propertyMap.value("Carrier").toBool();
    return false;
}

QStringList QNetworkManagerInterfaceDeviceWired::availableConnections()
{
    QStringList list;
    if (propertyMap.contains("AvailableConnections")) {
        const QDBusArgument &dbusArgs = propertyMap.value("Carrier").value<QDBusArgument>();
        QDBusObjectPath path;
        dbusArgs.beginArray();
        while (!dbusArgs.atEnd()) {
            dbusArgs >> path;
            list << path.path();
        }
        dbusArgs.endArray();
    }

    return list;
}

void QNetworkManagerInterfaceDeviceWired::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());
        if (i.key() == QStringLiteral("Carrier")) {
            Q_EMIT carrierChanged(i.value().toBool());
        }
    }
    Q_EMIT propertiesChanged(map);
}

class QNetworkManagerInterfaceDeviceWirelessPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerInterfaceDeviceWireless::QNetworkManagerInterfaceDeviceWireless(const QString &ifaceDevicePath, QObject *parent)
    : QObject(parent)
{
    d = new QNetworkManagerInterfaceDeviceWirelessPrivate();
    d->path = ifaceDevicePath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                                                QDBusConnection::systemBus(), parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }


    QDBusPendingReply<QList <QDBusObjectPath> > nmReply
            = d->connectionInterface->call(QLatin1String("GetAccessPoints"));

    if (!nmReply.isError()) {
        accessPointsList = nmReply.value();
    }

    QDBusInterface deviceWirelessPropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  d->path,
                                                  QLatin1String("org.freedesktop.DBus.Properties"),
                                                  QDBusConnection::systemBus(),parent);

    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS);
    QDBusPendingReply<QVariantMap> propsReply
            = deviceWirelessPropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);
    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  d->path,
                                  QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
    d->valid = true;
}

QNetworkManagerInterfaceDeviceWireless::~QNetworkManagerInterfaceDeviceWireless()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterfaceDeviceWireless::isValid()
{
    return d->valid;
}

void QNetworkManagerInterfaceDeviceWireless::slotAccessPointAdded(QDBusObjectPath path)
{
    if (path.path().length() > 2)
        Q_EMIT accessPointAdded(path.path());
}

void QNetworkManagerInterfaceDeviceWireless::slotAccessPointRemoved(QDBusObjectPath path)
{
    if (path.path().length() > 2)
        Q_EMIT accessPointRemoved(path.path());
}

bool QNetworkManagerInterfaceDeviceWireless::setConnections()
{
    if (!isValid())
        return false;

    QDBusConnection dbusConnection = QDBusConnection::systemBus();
    bool allOk = true;

    if (!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                                d->path,
                              QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                              QLatin1String("AccessPointAdded"),
                              this, SLOT(slotAccessPointAdded(QDBusObjectPath)))) {
        allOk = false;
    }


    if (!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                              d->path,
                              QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                              QLatin1String("AccessPointRemoved"),
                              this, SLOT(slotAccessPointRemoved(QDBusObjectPath)))) {
        allOk = false;
    }

    if (!dbusConnection.connect(QLatin1String(NM_DBUS_SERVICE),
                               d->path,
                               QLatin1String(NM_DBUS_INTERFACE_DEVICE_WIRELESS),
                               QLatin1String("ScanDone"),
                               this, SLOT(scanIsDone()))) {
        allOk = false;
    }
    return allOk;
}

QDBusInterface *QNetworkManagerInterfaceDeviceWireless::connectionInterface() const
{
    return d->connectionInterface;
}

QList <QDBusObjectPath> QNetworkManagerInterfaceDeviceWireless::getAccessPoints()
{
    if (accessPointsList.isEmpty()) {
        qWarning() << "Using blocking call!";
        QDBusReply<QList<QDBusObjectPath> > reply
                = d->connectionInterface->call(QLatin1String("GetAccessPoints"));
        accessPointsList = reply.value();
    }
    return accessPointsList;
}

QString QNetworkManagerInterfaceDeviceWireless::hwAddress() const
{
    if (propertyMap.contains("HwAddress"))
        return propertyMap.value("HwAddress").toString();
    return QString();
}

quint32 QNetworkManagerInterfaceDeviceWireless::mode() const
{
    if (propertyMap.contains("Mode"))
        return propertyMap.value("Mode").toUInt();
    return 0;
}

quint32 QNetworkManagerInterfaceDeviceWireless::bitrate() const
{
    if (propertyMap.contains("Bitrate"))
        return propertyMap.value("Bitrate").toUInt();
    return 0;
}

QDBusObjectPath QNetworkManagerInterfaceDeviceWireless::activeAccessPoint() const
{
    if (propertyMap.contains("ActiveAccessPoint"))
        return propertyMap.value("ActiveAccessPoint").value<QDBusObjectPath>();
    return QDBusObjectPath();
}

quint32 QNetworkManagerInterfaceDeviceWireless::wirelessCapabilities() const
{
    if (propertyMap.contains("WirelelessCapabilities"))
        return propertyMap.value("WirelelessCapabilities").toUInt();
    return 0;
}

void QNetworkManagerInterfaceDeviceWireless::scanIsDone()
{
    Q_EMIT scanDone();
}

void QNetworkManagerInterfaceDeviceWireless::requestScan()
{
    d->connectionInterface->asyncCall(QLatin1String("RequestScan"));
}

void QNetworkManagerInterfaceDeviceWireless::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());
        if (i.key() == QStringLiteral("ActiveAccessPoint")) { //DeviceWireless
            Q_EMIT propertiesChanged(map);
        }
    }
}

class QNetworkManagerInterfaceDeviceModemPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerInterfaceDeviceModem::QNetworkManagerInterfaceDeviceModem(const QString &ifaceDevicePath, QObject *parent)
    : QObject(parent)
{
    d = new QNetworkManagerInterfaceDeviceModemPrivate();
    d->path = ifaceDevicePath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_DEVICE_MODEM),
                                                QDBusConnection::systemBus(), parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    QDBusInterface deviceModemPropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  d->path,
                                                  QLatin1String("org.freedesktop.DBus.Properties"),
                                                  QDBusConnection::systemBus(),parent);

    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_DEVICE_MODEM);
    QDBusPendingReply<QVariantMap> propsReply
            = deviceModemPropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);
    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  d->path,
                                  QLatin1String(NM_DBUS_INTERFACE_DEVICE_MODEM),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
    d->valid = true;
}

QNetworkManagerInterfaceDeviceModem::~QNetworkManagerInterfaceDeviceModem()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerInterfaceDeviceModem::isValid()
{

    return d->valid;
}

bool QNetworkManagerInterfaceDeviceModem::setConnections()
{
    if (!isValid() )
        return false;

    return true;
}

QDBusInterface *QNetworkManagerInterfaceDeviceModem::connectionInterface() const
{
    return d->connectionInterface;
}

QNetworkManagerInterfaceDeviceModem::ModemCapabilities QNetworkManagerInterfaceDeviceModem::modemCapabilities() const
{
    if (propertyMap.contains("ModemCapabilities"))
        return static_cast<QNetworkManagerInterfaceDeviceModem::ModemCapabilities>(propertyMap.value("ModemCapabilities").toUInt());
    return QNetworkManagerInterfaceDeviceModem::None;
}

QNetworkManagerInterfaceDeviceModem::ModemCapabilities QNetworkManagerInterfaceDeviceModem::currentCapabilities() const
{
    if (propertyMap.contains("CurrentCapabilities"))
        return static_cast<QNetworkManagerInterfaceDeviceModem::ModemCapabilities>(propertyMap.value("CurrentCapabilities").toUInt());
    return QNetworkManagerInterfaceDeviceModem::None;
}

void QNetworkManagerInterfaceDeviceModem::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());
    }
    Q_EMIT propertiesChanged(map);
}

class QNetworkManagerSettingsPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerSettings::QNetworkManagerSettings(const QString &settingsService, QObject *parent)
        : QObject(parent)
{
    d = new QNetworkManagerSettingsPrivate();
    d->path = settingsService;
    d->connectionInterface = new QDBusInterface(settingsService,
                                                QLatin1String(NM_DBUS_PATH_SETTINGS),
                                                QLatin1String(NM_DBUS_IFACE_SETTINGS),
                                                QDBusConnection::systemBus());
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }

    QDBusPendingReply<QList <QDBusObjectPath> > nmReply
            = d->connectionInterface->call(QLatin1String("ListConnections"));

    if (!nmReply.isError()) {
        connectionsList = nmReply.value();
    }

    d->valid = true;
}

QNetworkManagerSettings::~QNetworkManagerSettings()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerSettings::isValid()
{
    return d->valid;
}

bool QNetworkManagerSettings::setConnections()
{
    bool allOk = true;

    if (!QDBusConnection::systemBus().connect(d->path,
                                             QLatin1String(NM_DBUS_PATH_SETTINGS),
                                             QLatin1String(NM_DBUS_IFACE_SETTINGS),
                                             QLatin1String("NewConnection"),
                                             this, SIGNAL(newConnection(QDBusObjectPath)))) {
        allOk = false;
    }

    return allOk;
}

QList <QDBusObjectPath> QNetworkManagerSettings::listConnections()
{
    if (connectionsList.isEmpty()) {
        qWarning() << "Using blocking call!";
        QDBusReply<QList<QDBusObjectPath> > reply
                = d->connectionInterface->call(QLatin1String("ListConnections"));
        connectionsList = reply.value();
    }
    return connectionsList;
}


QString QNetworkManagerSettings::getConnectionByUuid(const QString &uuid)
{
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(uuid);
    QDBusReply<QDBusObjectPath > reply = d->connectionInterface->callWithArgumentList(QDBus::Block,QLatin1String("GetConnectionByUuid"), argumentList);
    return reply.value().path();
}

QDBusInterface *QNetworkManagerSettings::connectionInterface() const
{
    return d->connectionInterface;
}


class QNetworkManagerSettingsConnectionPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    QString service;
    QNmSettingsMap settingsMap;
    bool valid;
};

QNetworkManagerSettingsConnection::QNetworkManagerSettingsConnection(const QString &settingsService, const QString &connectionObjectPath, QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<QNmSettingsMap>();
    d = new QNetworkManagerSettingsConnectionPrivate();
    d->path = connectionObjectPath;
    d->service = settingsService;
    d->connectionInterface = new QDBusInterface(settingsService,
                                                d->path,
                                                QLatin1String(NM_DBUS_IFACE_SETTINGS_CONNECTION),
                                                QDBusConnection::systemBus(), parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;

    QDBusPendingReply<QNmSettingsMap> nmReply
            = d->connectionInterface->call(QLatin1String("GetSettings"));
    if (!nmReply.isError()) {
        d->settingsMap = nmReply.value();
    }
}

QNetworkManagerSettingsConnection::~QNetworkManagerSettingsConnection()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerSettingsConnection::isValid()
{
    return d->valid;
}

bool QNetworkManagerSettingsConnection::setConnections()
{
    if (!isValid())
        return false;

    QDBusConnection dbusConnection = QDBusConnection::systemBus();
    bool allOk = true;
    if (!dbusConnection.connect(d->service,
                               d->path,
                               QLatin1String(NM_DBUS_IFACE_SETTINGS_CONNECTION),
                               QLatin1String("Updated"),
                               this, SIGNAL(updated()))) {
        allOk = false;
    }

    if (!dbusConnection.connect(d->service,
                               d->path,
                               QLatin1String(NM_DBUS_IFACE_SETTINGS_CONNECTION),
                               QLatin1String("Removed"),
                               this, SIGNAL(slotSettingsRemoved()))) {
        allOk = false;
    }
    return allOk;
}

void QNetworkManagerSettingsConnection::slotSettingsRemoved()
{
    Q_EMIT removed(d->path);
}

QDBusInterface *QNetworkManagerSettingsConnection::connectionInterface() const
{
    return d->connectionInterface;
}

QNmSettingsMap QNetworkManagerSettingsConnection::getSettings()
{
    if (d->settingsMap.isEmpty()) {
        qWarning() << "Using blocking call!";
        QDBusReply<QNmSettingsMap> reply = d->connectionInterface->call(QLatin1String("GetSettings"));
        d->settingsMap = reply.value();
    }
    return d->settingsMap;
}

NMDeviceType QNetworkManagerSettingsConnection::getType()
{
    const QString devType =
        d->settingsMap.value(QLatin1String("connection")).value(QLatin1String("type")).toString();

    if (devType == QLatin1String("802-3-ethernet"))
        return DEVICE_TYPE_ETHERNET;
    else if (devType == QLatin1String("802-11-wireless"))
        return DEVICE_TYPE_WIFI;
    else if (devType == QLatin1String("gsm"))
        return DEVICE_TYPE_MODEM;
    else
        return DEVICE_TYPE_UNKNOWN;
}

bool QNetworkManagerSettingsConnection::isAutoConnect()
{
    const QVariant autoConnect =
        d->settingsMap.value(QLatin1String("connection")).value(QLatin1String("autoconnect"));

    // NetworkManager default is to auto connect
    if (!autoConnect.isValid())
        return true;

    return autoConnect.toBool();
}

quint64 QNetworkManagerSettingsConnection::getTimestamp()
{
    return d->settingsMap.value(QLatin1String("connection"))
                         .value(QLatin1String("timestamp")).toUInt();
}

QString QNetworkManagerSettingsConnection::getId()
{
    return d->settingsMap.value(QLatin1String("connection")).value(QLatin1String("id")).toString();
}

QString QNetworkManagerSettingsConnection::getUuid()
{
    const QString id = d->settingsMap.value(QLatin1String("connection"))
                                     .value(QLatin1String("uuid")).toString();

    // is no uuid, return the connection path
    return id.isEmpty() ? d->connectionInterface->path() : id;
}

QString QNetworkManagerSettingsConnection::getSsid()
{
    return d->settingsMap.value(QLatin1String("802-11-wireless"))
                         .value(QLatin1String("ssid")).toString();
}

QString QNetworkManagerSettingsConnection::getMacAddress()
{
    NMDeviceType type = getType();

    if (type == DEVICE_TYPE_ETHERNET) {
        return d->settingsMap.value(QLatin1String("802-3-ethernet"))
                             .value(QLatin1String("mac-address")).toString();
    } else if (type == DEVICE_TYPE_WIFI) {
        return d->settingsMap.value(QLatin1String("802-11-wireless"))
                             .value(QLatin1String("mac-address")).toString();
    } else {
        return QString();
    }
}

QStringList QNetworkManagerSettingsConnection::getSeenBssids()
{
    if (getType() == DEVICE_TYPE_WIFI) {
        return d->settingsMap.value(QLatin1String("802-11-wireless"))
                             .value(QLatin1String("seen-bssids")).toStringList();
    } else {
        return QStringList();
    }
}

class QNetworkManagerConnectionActivePrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerConnectionActive::QNetworkManagerConnectionActive(const QString &activeConnectionObjectPath, QObject *parent)
    : QObject(parent)
{
    d = new QNetworkManagerConnectionActivePrivate();
    d->path = activeConnectionObjectPath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_ACTIVE_CONNECTION),
                                                QDBusConnection::systemBus(), parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    QDBusInterface connectionActivePropertiesInterface(QLatin1String(NM_DBUS_SERVICE),
                                                  d->path,
                                                  QLatin1String("org.freedesktop.DBus.Properties"),
                                                  QDBusConnection::systemBus());


    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE_ACTIVE_CONNECTION);
    QDBusPendingReply<QVariantMap> propsReply
            = connectionActivePropertiesInterface.callWithArgumentList(QDBus::Block,QLatin1String("GetAll"),
                                                                       argumentList);

    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    } else {
        qWarning() << propsReply.error().message();
    }

    QDBusConnection::systemBus().connect(QLatin1String(NM_DBUS_SERVICE),
                                  d->path,
                                  QLatin1String(NM_DBUS_INTERFACE_ACTIVE_CONNECTION),
                                  QLatin1String("PropertiesChanged"),
                                  this,SLOT(propertiesSwap(QMap<QString,QVariant>)));
    d->valid = true;
}

QNetworkManagerConnectionActive::~QNetworkManagerConnectionActive()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerConnectionActive::isValid()
{
    return d->valid;
}

bool QNetworkManagerConnectionActive::setConnections()
{
    if (!isValid())
        return false;

    return true;
}

QDBusInterface *QNetworkManagerConnectionActive::connectionInterface() const
{
    return d->connectionInterface;
}

QDBusObjectPath QNetworkManagerConnectionActive::connection() const
{
    if (propertyMap.contains("Connection"))
        return propertyMap.value("Connection").value<QDBusObjectPath>();
    return QDBusObjectPath();
}

QDBusObjectPath QNetworkManagerConnectionActive::specificObject() const
{
    if (propertyMap.contains("SpecificObject"))
        return propertyMap.value("SpecificObject").value<QDBusObjectPath>();
    return QDBusObjectPath();
}

QStringList QNetworkManagerConnectionActive::devices() const
{
    QStringList list;
    if (propertyMap.contains("Devices")) {
        const QDBusArgument &dbusArgs = propertyMap.value("Devices").value<QDBusArgument>();
        QDBusObjectPath path;

        dbusArgs.beginArray();
        while (!dbusArgs.atEnd()) {
            dbusArgs >> path;
            list.append(path.path());
        }
        dbusArgs.endArray();
    }
    return list;
}

quint32 QNetworkManagerConnectionActive::state() const
{
    if (propertyMap.contains("State"))
        return propertyMap.value("State").toUInt();
    return 0;
}

bool QNetworkManagerConnectionActive::defaultRoute() const
{
    if (propertyMap.contains("Default"))
        return propertyMap.value("Default").toBool();
    return false;
}

bool QNetworkManagerConnectionActive::default6Route() const
{
    if (propertyMap.contains("Default6"))
        return propertyMap.value("Default6").toBool();
    return false;
}

void QNetworkManagerConnectionActive::propertiesSwap(QMap<QString,QVariant> map)
{
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        propertyMap.insert(i.key(),i.value());
        if (i.key() == QStringLiteral("State")) {
            quint32 state = i.value().toUInt();
            if (state == NM_ACTIVE_CONNECTION_STATE_ACTIVATED
                || state == NM_ACTIVE_CONNECTION_STATE_DEACTIVATED) {
                Q_EMIT propertiesChanged(map);
            }
        }
    }
}

class QNetworkManagerIp4ConfigPrivate
{
public:
    QDBusInterface *connectionInterface;
    QString path;
    bool valid;
};

QNetworkManagerIp4Config::QNetworkManagerIp4Config( const QString &deviceObjectPath, QObject *parent)
    : QObject(parent)
{
    d = new QNetworkManagerIp4ConfigPrivate();
    d->path = deviceObjectPath;
    d->connectionInterface = new QDBusInterface(QLatin1String(NM_DBUS_SERVICE),
                                                d->path,
                                                QLatin1String(NM_DBUS_INTERFACE_IP4_CONFIG),
                                                QDBusConnection::systemBus(), parent);
    if (!d->connectionInterface->isValid()) {
        d->valid = false;
        return;
    }
    d->valid = true;
}

QNetworkManagerIp4Config::~QNetworkManagerIp4Config()
{
    delete d->connectionInterface;
    delete d;
}

bool QNetworkManagerIp4Config::isValid()
{
    return d->valid;
}

QStringList QNetworkManagerIp4Config::domains() const
{
    return d->connectionInterface->property("Domains").toStringList();
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
