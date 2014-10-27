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

#include "qnetworkmanagerengine.h"
#include "qnetworkmanagerservice.h"
#include "../qnetworksession_impl.h"

#include <QtNetwork/private/qnetworkconfiguration_p.h>

#include <QtNetwork/qnetworksession.h>

#include <QtCore/qdebug.h>

#include <QtDBus>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QNetworkManagerEngine::QNetworkManagerEngine(QObject *parent)
:   QBearerEngineImpl(parent),
    managerInterface(new QNetworkManagerInterface(this)),
    systemSettings(new QNetworkManagerSettings(NM_DBUS_SERVICE, this)),
    userSettings(new QNetworkManagerSettings(NM_DBUS_SERVICE, this))
{
    if (!managerInterface->isValid())
        return;

    managerInterface->setConnections();
    connect(managerInterface, SIGNAL(deviceAdded(QDBusObjectPath)),
            this, SLOT(deviceAdded(QDBusObjectPath)));
    connect(managerInterface, SIGNAL(deviceRemoved(QDBusObjectPath)),
            this, SLOT(deviceRemoved(QDBusObjectPath)));
    connect(managerInterface, SIGNAL(activationFinished(QDBusPendingCallWatcher*)),
            this, SLOT(activationFinished(QDBusPendingCallWatcher*)));
    connect(managerInterface, SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)),
            this, SLOT(interfacePropertiesChanged(QString,QMap<QString,QVariant>)));

    qDBusRegisterMetaType<QNmSettingsMap>();

    systemSettings->setConnections();
    connect(systemSettings, SIGNAL(newConnection(QDBusObjectPath)),
            this, SLOT(newConnection(QDBusObjectPath)));

    userSettings->setConnections();
    connect(userSettings, SIGNAL(newConnection(QDBusObjectPath)),
            this, SLOT(newConnection(QDBusObjectPath)));
}

QNetworkManagerEngine::~QNetworkManagerEngine()
{
    qDeleteAll(connections);
    qDeleteAll(accessPoints);
    qDeleteAll(wirelessDevices);
    qDeleteAll(activeConnections);
}

void QNetworkManagerEngine::initialize()
{
    QMutexLocker locker(&mutex);

    // Get current list of access points.
    foreach (const QDBusObjectPath &devicePath, managerInterface->getDevices()) {
        locker.unlock();
        deviceAdded(devicePath); //add all accesspoints
        locker.relock();
    }

    // Get connections.

    foreach (const QDBusObjectPath &settingsPath, systemSettings->listConnections()) {
        locker.unlock();
        if (!hasIdentifier(settingsPath.path()))
            newConnection(settingsPath, systemSettings); //add system connection configs
        locker.relock();
    }

    foreach (const QDBusObjectPath &settingsPath, userSettings->listConnections()) {
        locker.unlock();
        if (!hasIdentifier(settingsPath.path()))
            newConnection(settingsPath, userSettings);
        locker.relock();
    }

    // Get active connections.
    foreach (const QDBusObjectPath &acPath, managerInterface->activeConnections()) {
        QNetworkManagerConnectionActive *activeConnection =
            new QNetworkManagerConnectionActive(acPath.path(),this);
        activeConnections.insert(acPath.path(), activeConnection);

        activeConnection->setConnections();
        connect(activeConnection, SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)),
                this, SLOT(activeConnectionPropertiesChanged(QString,QMap<QString,QVariant>)));
    }
    Q_EMIT updateCompleted();
}

bool QNetworkManagerEngine::networkManagerAvailable() const
{
    QMutexLocker locker(&mutex);

    return managerInterface->isValid();
}

QString QNetworkManagerEngine::getInterfaceFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    foreach (const QDBusObjectPath &acPath, managerInterface->activeConnections()) {
        QNetworkManagerConnectionActive activeConnection(acPath.path());

        const QString identifier = activeConnection.connection().path();

        if (id == identifier) {
            QList<QDBusObjectPath> devices = activeConnection.devices();

            if (devices.isEmpty())
                continue;

            QNetworkManagerInterfaceDevice device(devices.at(0).path());
            return device.networkInterface();
        }
    }

    return QString();
}


bool QNetworkManagerEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);
    return accessPointConfigurations.contains(id);
}

void QNetworkManagerEngine::connectToId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection = connectionFromId(id);

    if (!connection)
        return;

    QNmSettingsMap map = connection->getSettings();
    const QString connectionType = map.value("connection").value("type").toString();

    QString dbusDevicePath;
    foreach (const QDBusObjectPath &devicePath, managerInterface->getDevices()) {
        QNetworkManagerInterfaceDevice device(devicePath.path());
        if (device.deviceType() == DEVICE_TYPE_ETHERNET &&
            connectionType == QLatin1String("802-3-ethernet")) {
            dbusDevicePath = devicePath.path();
            break;
        } else if (device.deviceType() == DEVICE_TYPE_WIFI &&
                   connectionType == QLatin1String("802-11-wireless")) {
            dbusDevicePath = devicePath.path();
            break;
        } else if (device.deviceType() == DEVICE_TYPE_MODEM &&
                connectionType == QLatin1String("gsm")) {
            dbusDevicePath = devicePath.path();
            break;
        }
    }

    const QString service = connection->connectionInterface()->service();
    const QString settingsPath = connection->connectionInterface()->path();
    QString specificPath = configuredAccessPoints.key(settingsPath);

    if (specificPath.isEmpty())
        specificPath = "/";

    managerInterface->activateConnection(service, QDBusObjectPath(settingsPath),
                                  QDBusObjectPath(dbusDevicePath), QDBusObjectPath(specificPath));
}

void QNetworkManagerEngine::disconnectFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection = connectionFromId(id);
    QNmSettingsMap map = connection->getSettings();
    bool connectionAutoconnect = map.value("connection").value("autoconnect",true).toBool(); //if not present is true !!
    if (connectionAutoconnect) { //autoconnect connections will simply be reconnected by nm
        emit connectionError(id, QBearerEngineImpl::OperationNotSupported);
        return;
    }

    foreach (const QDBusObjectPath &acPath, managerInterface->activeConnections()) {
        QNetworkManagerConnectionActive activeConnection(acPath.path());

        const QString identifier = activeConnection.connection().path();

        if (id == identifier && accessPointConfigurations.contains(id)) {
            managerInterface->deactivateConnection(acPath);
            break;
        }
    }
}

void QNetworkManagerEngine::requestUpdate()
{
    if (managerInterface->wirelessEnabled()) {
        QHashIterator<QString, QNetworkManagerInterfaceDeviceWireless *> i(wirelessDevices);
        while (i.hasNext()) {
            i.next();
            i.value()->requestScan();
        }
    }
    QMetaObject::invokeMethod(this, "updateCompleted", Qt::QueuedConnection);
}

void QNetworkManagerEngine::scanFinished()
{
    QMetaObject::invokeMethod(this, "updateCompleted", Qt::QueuedConnection);
}

void QNetworkManagerEngine::interfacePropertiesChanged(const QString &path,
                                                       const QMap<QString, QVariant> &properties)
{
    Q_UNUSED(path)
    QMutexLocker locker(&mutex);

    QMapIterator<QString, QVariant> i(properties);
    while (i.hasNext()) {
        i.next();

        if (i.key() == QLatin1String("ActiveConnections")) {
            // Active connections changed, update configurations.

            QList<QDBusObjectPath> activeConnections =
                qdbus_cast<QList<QDBusObjectPath> >(i.value().value<QDBusArgument>());

            QStringList identifiers = accessPointConfigurations.keys();
            foreach (const QString &id, identifiers)
                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

            QStringList priorActiveConnections = this->activeConnections.keys();

            foreach (const QDBusObjectPath &acPath, activeConnections) {
                priorActiveConnections.removeOne(acPath.path());
                QNetworkManagerConnectionActive *activeConnection =
                    this->activeConnections.value(acPath.path());
                if (!activeConnection) {
                    activeConnection = new QNetworkManagerConnectionActive(acPath.path(),this);
                    this->activeConnections.insert(acPath.path(), activeConnection);

                    activeConnection->setConnections();
                    connect(activeConnection, SIGNAL(propertiesChanged(QString,QMap<QString,QVariant>)),
                            this, SLOT(activeConnectionPropertiesChanged(QString,QMap<QString,QVariant>)));
                }

                const QString id = activeConnection->connection().path();

                identifiers.removeOne(id);

                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
                if (ptr) {
                    ptr->mutex.lock();
                    if (activeConnection->state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED &&
                        ptr->state != QNetworkConfiguration::Active) {
                        ptr->state = QNetworkConfiguration::Active;
                        ptr->mutex.unlock();

                        locker.unlock();
                        emit configurationChanged(ptr);
                        locker.relock();
                    } else {
                        ptr->mutex.unlock();
                    }
                }
            }

            while (!priorActiveConnections.isEmpty())
                delete this->activeConnections.take(priorActiveConnections.takeFirst());

            while (!identifiers.isEmpty()) {
                QNetworkConfigurationPrivatePointer ptr =
                    accessPointConfigurations.value(identifiers.takeFirst());

                ptr->mutex.lock();
                if ((ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
                    QNetworkConfiguration::StateFlags flag = QNetworkConfiguration::Defined;
                    ptr->state = (flag | QNetworkConfiguration::Discovered);
                    ptr->mutex.unlock();

                    locker.unlock();
                    emit configurationChanged(ptr);
                    locker.relock();
                } else {
                    ptr->mutex.unlock();
                }
            }
        }
    }
}

void QNetworkManagerEngine::activeConnectionPropertiesChanged(const QString &path,
                                                              const QMap<QString, QVariant> &properties)
{
    QMutexLocker locker(&mutex);

    Q_UNUSED(properties)

    QNetworkManagerConnectionActive *activeConnection = activeConnections.value(path);

    if (!activeConnection)
        return;

    const QString id = activeConnection->connection().path();

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
    if (ptr) {
        ptr->mutex.lock();
        if (activeConnection->state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED &&
            ptr->state != QNetworkConfiguration::Active) {
            ptr->state |= QNetworkConfiguration::Active;
            ptr->mutex.unlock();

            locker.unlock();
            emit configurationChanged(ptr);
            locker.relock();
        } else {
            ptr->mutex.unlock();
        }
    }
}

void QNetworkManagerEngine::devicePropertiesChanged(const QString &/*path*/,quint32 /*state*/)
{
//    Q_UNUSED(path);
//    Q_UNUSED(state)
}

void QNetworkManagerEngine::deviceConnectionsChanged(const QStringList &activeConnectionsList)
{
    QMutexLocker locker(&mutex);
    for (int i = 0; i < connections.count(); ++i) {
        if (activeConnectionsList.contains(connections.at(i)->connectionInterface()->path()))
            continue;

        const QString settingsPath = connections.at(i)->connectionInterface()->path();

        QNetworkConfigurationPrivatePointer ptr =
            accessPointConfigurations.value(settingsPath);
        ptr->mutex.lock();
        QNetworkConfiguration::StateFlags flag = QNetworkConfiguration::Defined;
        ptr->state = (flag | QNetworkConfiguration::Discovered);
        ptr->mutex.unlock();

        locker.unlock();
        emit configurationChanged(ptr);
        locker.relock();
        Q_EMIT updateCompleted();
    }
}

void QNetworkManagerEngine::deviceAdded(const QDBusObjectPath &path)
{
    QMutexLocker locker(&mutex);
    QNetworkManagerInterfaceDevice *iDevice;
    iDevice = new QNetworkManagerInterfaceDevice(path.path(),this);
    connect(iDevice,SIGNAL(connectionsChanged(QStringList)),
            this,SLOT(deviceConnectionsChanged(QStringList)));

    connect(iDevice,SIGNAL(stateChanged(QString,quint32)),
            this,SLOT(devicePropertiesChanged(QString,quint32)));
    iDevice->setConnections();
    interfaceDevices.insert(path.path(),iDevice);

    if (iDevice->deviceType() == DEVICE_TYPE_WIFI) {
        QNetworkManagerInterfaceDeviceWireless *wirelessDevice =
            new QNetworkManagerInterfaceDeviceWireless(iDevice->connectionInterface()->path(),this);

        wirelessDevice->setConnections();
        connect(wirelessDevice, SIGNAL(accessPointAdded(QString)),
                this, SLOT(newAccessPoint(QString)));
        connect(wirelessDevice, SIGNAL(accessPointRemoved(QString)),
                this, SLOT(removeAccessPoint(QString)));
        connect(wirelessDevice,SIGNAL(scanDone()),this,SLOT(scanFinished()));

        foreach (const QDBusObjectPath &apPath, wirelessDevice->getAccessPoints())
            newAccessPoint(apPath.path());

        wirelessDevices.insert(path.path(), wirelessDevice);
    }
}

void QNetworkManagerEngine::deviceRemoved(const QDBusObjectPath &path)
{
    QMutexLocker locker(&mutex);

    if (interfaceDevices.contains(path.path())) {
        locker.unlock();
        delete interfaceDevices.take(path.path());
        locker.relock();
    }
    if (wirelessDevices.contains(path.path())) {
        locker.unlock();
        delete wirelessDevices.take(path.path());
        locker.relock();
    }
}

void QNetworkManagerEngine::newConnection(const QDBusObjectPath &path,
                                          QNetworkManagerSettings *settings)
{
    QMutexLocker locker(&mutex);
    if (!settings)
        settings = qobject_cast<QNetworkManagerSettings *>(sender());

    if (!settings)
        return;

    settings->deleteLater();
    QNetworkManagerSettingsConnection *connection =
        new QNetworkManagerSettingsConnection(settings->connectionInterface()->service(),
                                              path.path(),this);
    QString apPath;
    for (int i = 0; i < accessPoints.count(); ++i) {
        if (connection->getSsid() == accessPoints.at(i)->ssid()) {
            // remove the corresponding accesspoint from configurations
            apPath = accessPoints.at(i)->connectionInterface()->path();

            QNetworkConfigurationPrivatePointer ptr
                    = accessPointConfigurations.take(apPath);
            if (ptr) {
                locker.unlock();
                emit configurationRemoved(ptr);
                locker.relock();
            }
        }
    }
    connections.append(connection);

    connect(connection,SIGNAL(removed(QString)),this,SLOT(removeConnection(QString)));
    connect(connection,SIGNAL(updated()),this,SLOT(updateConnection()));
    connection->setConnections();

    const QString settingsPath = connection->connectionInterface()->path();

    if (connection->getType() == DEVICE_TYPE_WIFI
            && !configuredAccessPoints.contains(settingsPath))
        configuredAccessPoints.insert(apPath,settingsPath);

    QNetworkConfigurationPrivate *cpPriv =
        parseConnection(settingsPath, connection->getSettings());

    // Check if connection is active.
    foreach (const QDBusObjectPath &acPath, managerInterface->activeConnections()) {
        QNetworkManagerConnectionActive activeConnection(acPath.path());

        if (activeConnection.defaultRoute() &&
            activeConnection.connection().path() == settingsPath &&
            activeConnection.state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
            cpPriv->state |= QNetworkConfiguration::Active;
            break;
        }
    }
    QNetworkConfigurationPrivatePointer ptr(cpPriv);
    accessPointConfigurations.insert(ptr->id, ptr);

    locker.unlock();
    emit configurationAdded(ptr);
}

void QNetworkManagerEngine::removeConnection(const QString &path)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection =
        qobject_cast<QNetworkManagerSettingsConnection *>(sender());
    if (!connection)
        return;

    connection->deleteLater();
    connections.removeAll(connection);

    const QString id = path;

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.take(id);

    if (ptr) {
        locker.unlock();
        emit configurationRemoved(ptr);
        locker.relock();
    }
    // add base AP back into configurations
    QMapIterator<QString, QString> i(configuredAccessPoints);
    while (i.hasNext()) {
        i.next();
        if (i.value() == path) {
            newAccessPoint(i.key());
        }
    }
}

void QNetworkManagerEngine::updateConnection()
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection =
        qobject_cast<QNetworkManagerSettingsConnection *>(sender());
    if (!connection)
        return;

    connection->deleteLater();
    const QString settingsPath = connection->connectionInterface()->path();

    QNetworkConfigurationPrivate *cpPriv = parseConnection(settingsPath, connection->getSettings());

    // Check if connection is active.
    foreach (const QDBusObjectPath &acPath, managerInterface->activeConnections()) {
        QNetworkManagerConnectionActive activeConnection(acPath.path());

        if (activeConnection.connection().path() == settingsPath &&
            activeConnection.state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
            cpPriv->state |= QNetworkConfiguration::Active;
            break;
        }
    }

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(cpPriv->id);

    ptr->mutex.lock();

    ptr->isValid = cpPriv->isValid;
    ptr->name = cpPriv->name;
    ptr->id = cpPriv->id;
    ptr->state = cpPriv->state;

    ptr->mutex.unlock();

    locker.unlock();
    emit configurationChanged(ptr);
    locker.relock();
    delete cpPriv;
}

void QNetworkManagerEngine::activationFinished(QDBusPendingCallWatcher *watcher)
{
    QMutexLocker locker(&mutex);

    watcher->deleteLater();

    QDBusPendingReply<QDBusObjectPath> reply(*watcher);
    if (!reply.isError()) {
        QDBusObjectPath result = reply.value();

        QNetworkManagerConnectionActive activeConnection(result.path());

        const QString id = activeConnection.connection().path();

        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
        if (ptr) {
            ptr->mutex.lock();
            if (activeConnection.state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED &&
                ptr->state != QNetworkConfiguration::Active) {
                ptr->state |= QNetworkConfiguration::Active;
                ptr->mutex.unlock();

                locker.unlock();
                emit configurationChanged(ptr);
                locker.relock();
            } else {
                ptr->mutex.unlock();
            }
        }
    }
}

void QNetworkManagerEngine::newAccessPoint(const QString &path)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerInterfaceAccessPoint *accessPoint =
        new QNetworkManagerInterfaceAccessPoint(path,this);

    bool okToAdd = true;
    for (int i = 0; i < accessPoints.count(); ++i) {
        if (accessPoints.at(i)->connectionInterface()->path() == path) {
            okToAdd = false;
        }
    }
    if (okToAdd) {
        accessPoints.append(accessPoint);
        accessPoint->setConnections();
        connect(accessPoint, SIGNAL(propertiesChanged(QMap<QString,QVariant>)),
                this, SLOT(updateAccessPoint(QMap<QString,QVariant>)));
    }

    // Check if configuration exists for connection.
    if (!accessPoint->ssid().isEmpty()) {
        for (int i = 0; i < connections.count(); ++i) {
            QNetworkManagerSettingsConnection *connection = connections.at(i);
            const QString settingsPath = connection->connectionInterface()->path();
            if (accessPoint->ssid() == connection->getSsid()) {
                if (!configuredAccessPoints.contains(path)) {
                    configuredAccessPoints.insert(path,settingsPath);
                }

                QNetworkConfigurationPrivatePointer ptr =
                    accessPointConfigurations.value(settingsPath);
                ptr->mutex.lock();
                QNetworkConfiguration::StateFlags flag = QNetworkConfiguration::Defined;
                ptr->state = (flag | QNetworkConfiguration::Discovered);
                ptr->mutex.unlock();

                locker.unlock();
                emit configurationChanged(ptr);
                return;
            }
        }
    }

    // New access point.
    QNetworkConfigurationPrivatePointer ptr(new QNetworkConfigurationPrivate);

    ptr->name = accessPoint->ssid();
    ptr->isValid = true;
    ptr->id = path;
    ptr->type = QNetworkConfiguration::InternetAccessPoint;
    if (accessPoint->flags() == NM_802_11_AP_FLAGS_PRIVACY) {
        ptr->purpose = QNetworkConfiguration::PrivatePurpose;
    } else {
        ptr->purpose = QNetworkConfiguration::PublicPurpose;
    }
    ptr->state = QNetworkConfiguration::Undefined;
    ptr->bearerType = QNetworkConfiguration::BearerWLAN;

    accessPointConfigurations.insert(ptr->id, ptr);

    locker.unlock();
    emit configurationAdded(ptr);
}

void QNetworkManagerEngine::removeAccessPoint(const QString &path)
{
    QMutexLocker locker(&mutex);
    for (int i = 0; i < accessPoints.count(); ++i) {
        QNetworkManagerInterfaceAccessPoint *accessPoint = accessPoints.at(i);

        if (accessPoint->connectionInterface()->path() == path) {
            accessPoints.removeOne(accessPoint);

            if (configuredAccessPoints.contains(accessPoint->connectionInterface()->path())) {
                // find connection and change state to Defined
                configuredAccessPoints.remove(accessPoint->connectionInterface()->path());
                for (int i = 0; i < connections.count(); ++i) {
                    QNetworkManagerSettingsConnection *connection = connections.at(i);

                    if (accessPoint->ssid() == connection->getSsid()) {//might not have bssid yet
                        const QString settingsPath = connection->connectionInterface()->path();
                        const QString connectionId = settingsPath;

                        QNetworkConfigurationPrivatePointer ptr =
                            accessPointConfigurations.value(connectionId);
                        ptr->mutex.lock();
                        ptr->state = QNetworkConfiguration::Defined;
                        ptr->mutex.unlock();

                        locker.unlock();
                        emit configurationChanged(ptr);
                        locker.relock();
                        break;
                    }
                }
            } else {
                QNetworkConfigurationPrivatePointer ptr =
                    accessPointConfigurations.take(path);

                if (ptr) {
                    locker.unlock();

                    locker.unlock();
                    emit configurationRemoved(ptr);
                    locker.relock();
                }
            }
            delete accessPoint;
            break;
        }
    }
}

void QNetworkManagerEngine::updateAccessPoint(const QMap<QString, QVariant> &map)
{
    QMutexLocker locker(&mutex);

    Q_UNUSED(map)

    QNetworkManagerInterfaceAccessPoint *accessPoint =
        qobject_cast<QNetworkManagerInterfaceAccessPoint *>(sender());
    if (!accessPoint)
        return;
    accessPoint->deleteLater();
    for (int i = 0; i < connections.count(); ++i) {
        QNetworkManagerSettingsConnection *connection = connections.at(i);

        if (accessPoint->ssid() == connection->getSsid()) {
            const QString settingsPath = connection->connectionInterface()->path();
            const QString connectionId = settingsPath;

            QNetworkConfigurationPrivatePointer ptr =
                accessPointConfigurations.value(connectionId);
            ptr->mutex.lock();
            QNetworkConfiguration::StateFlags flag = QNetworkConfiguration::Defined;
            ptr->state = (flag | QNetworkConfiguration::Discovered);
            ptr->mutex.unlock();

            locker.unlock();
            emit configurationChanged(ptr);
            return;
        }
    }
}

QNetworkConfigurationPrivate *QNetworkManagerEngine::parseConnection(const QString &settingsPath,
                                                                     const QNmSettingsMap &map)
{
   // Q_UNUSED(service);
    QMutexLocker locker(&mutex);
    QNetworkConfigurationPrivate *cpPriv = new QNetworkConfigurationPrivate;
    cpPriv->name = map.value("connection").value("id").toString();

    cpPriv->isValid = true;
    cpPriv->id = settingsPath;
    cpPriv->type = QNetworkConfiguration::InternetAccessPoint;

    cpPriv->purpose = QNetworkConfiguration::PublicPurpose;

    cpPriv->state = QNetworkConfiguration::Defined;

    const QString connectionType = map.value("connection").value("type").toString();

    if (connectionType == QLatin1String("802-3-ethernet")) {
        cpPriv->bearerType = QNetworkConfiguration::BearerEthernet;
        cpPriv->purpose = QNetworkConfiguration::PublicPurpose;

        foreach (const QDBusObjectPath &devicePath, managerInterface->getDevices()) {
            QNetworkManagerInterfaceDevice device(devicePath.path());
            if (device.deviceType() == DEVICE_TYPE_ETHERNET) {
                QNetworkManagerInterfaceDeviceWired wiredDevice(device.connectionInterface()->path());
                if (wiredDevice.carrier()) {
                    cpPriv->state |= QNetworkConfiguration::Discovered;
                    break;
                }
            }
        }
    } else if (connectionType == QLatin1String("802-11-wireless")) {
        cpPriv->bearerType = QNetworkConfiguration::BearerWLAN;

        const QString connectionSsid = map.value("802-11-wireless").value("ssid").toString();
        const QString connectionSecurity = map.value("802-11-wireless").value("security").toString();
        if (!connectionSecurity.isEmpty()) {
            cpPriv->purpose = QNetworkConfiguration::PrivatePurpose;
        } else {
            cpPriv->purpose = QNetworkConfiguration::PublicPurpose;
        }
        for (int i = 0; i < accessPoints.count(); ++i) {
            if (connectionSsid == accessPoints.at(i)->ssid()
                    && map.value("802-11-wireless").value("seen-bssids").toStringList().contains(accessPoints.at(i)->hwAddress())) {
                cpPriv->state |= QNetworkConfiguration::Discovered;
                if (!configuredAccessPoints.contains(accessPoints.at(i)->connectionInterface()->path())) {
                    configuredAccessPoints.insert(accessPoints.at(i)->connectionInterface()->path(),settingsPath);

                    const QString accessPointId = accessPoints.at(i)->connectionInterface()->path();
                    QNetworkConfigurationPrivatePointer ptr =
                        accessPointConfigurations.take(accessPointId);

                    if (ptr) {
                        locker.unlock();
                        emit configurationRemoved(ptr);
                        locker.relock();
                    }
                }
                break;
            }
        }
    } else if (connectionType == QLatin1String("gsm")) {

        foreach (const QDBusObjectPath &devicePath, managerInterface->getDevices()) {
            QNetworkManagerInterfaceDevice device(devicePath.path());

            if (device.deviceType() == DEVICE_TYPE_MODEM) {
                QNetworkManagerInterfaceDeviceModem deviceModem(device.connectionInterface()->path(),this);
                switch (deviceModem.currentCapabilities()) {
                case 2:
                    cpPriv->bearerType = QNetworkConfiguration::Bearer2G;
                    break;
                case 4:
                    cpPriv->bearerType = QNetworkConfiguration::Bearer3G;
                    break;
                case 8:
                    cpPriv->bearerType = QNetworkConfiguration::Bearer4G;
                    break;
                default:
                    cpPriv->bearerType = QNetworkConfiguration::BearerUnknown;
                    break;
                };
            }
        }

        cpPriv->purpose = QNetworkConfiguration::PrivatePurpose;
        cpPriv->state |= QNetworkConfiguration::Discovered;
    }

    return cpPriv;
}

QNetworkManagerSettingsConnection *QNetworkManagerEngine::connectionFromId(const QString &id) const
{
    for (int i = 0; i < connections.count(); ++i) {
        QNetworkManagerSettingsConnection *connection = connections.at(i);
        if (id == connection->connectionInterface()->path())
            return connection;
    }

    return 0;
}

QNetworkSession::State QNetworkManagerEngine::sessionStateForId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

    if (!ptr)
        return QNetworkSession::Invalid;

    if (!ptr->isValid)
        return QNetworkSession::Invalid;

    foreach (const QString &acPath, activeConnections.keys()) {
        QNetworkManagerConnectionActive *activeConnection = activeConnections.value(acPath);

        const QString identifier = activeConnection->connection().path();

        if (id == identifier) {
            switch (activeConnection->state()) {
            case 0:
                return QNetworkSession::Disconnected;
            case 1:
                return QNetworkSession::Connecting;
            case 2:
                return QNetworkSession::Connected;
            }
        }
    }

    if ((ptr->state & QNetworkConfiguration::Discovered) == QNetworkConfiguration::Discovered)
        return QNetworkSession::Disconnected;
    else if ((ptr->state & QNetworkConfiguration::Defined) == QNetworkConfiguration::Defined)
        return QNetworkSession::NotAvailable;
    else if ((ptr->state & QNetworkConfiguration::Undefined) == QNetworkConfiguration::Undefined)
        return QNetworkSession::NotAvailable;

    return QNetworkSession::Invalid;
}

quint64 QNetworkManagerEngine::bytesWritten(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
    if (ptr && (ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        const QString networkInterface = getInterfaceFromId(id);
        if (!networkInterface.isEmpty()) {
            const QString devFile = QLatin1String("/sys/class/net/") +
                                    networkInterface +
                                    QLatin1String("/statistics/tx_bytes");

            quint64 result = Q_UINT64_C(0);

            QFile tx(devFile);
            if (tx.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&tx);
                in >> result;
                tx.close();
            }

            return result;
        }
    }

    return Q_UINT64_C(0);
}

quint64 QNetworkManagerEngine::bytesReceived(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
    if (ptr && (ptr->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        const QString networkInterface = getInterfaceFromId(id);
        if (!networkInterface.isEmpty()) {
            const QString devFile = QLatin1String("/sys/class/net/") +
                                    networkInterface +
                                    QLatin1String("/statistics/rx_bytes");

            quint64 result = Q_UINT64_C(0);

            QFile tx(devFile);
            if (tx.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&tx);
                in >> result;
                tx.close();
            }

            return result;
        }
    }

    return Q_UINT64_C(0);
}

quint64 QNetworkManagerEngine::startTime(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection = connectionFromId(id);
    if (connection)
        return connection->getTimestamp();
    else
        return Q_UINT64_C(0);
}

QNetworkConfigurationManager::Capabilities QNetworkManagerEngine::capabilities() const
{
    return QNetworkConfigurationManager::ForcedRoaming |
            QNetworkConfigurationManager::DataStatistics |
            QNetworkConfigurationManager::CanStartAndStopInterfaces;
}

QNetworkSessionPrivate *QNetworkManagerEngine::createSessionBackend()
{
    return new QNetworkSessionPrivateImpl;
}

QNetworkConfigurationPrivatePointer QNetworkManagerEngine::defaultConfiguration()
{
    return QNetworkConfigurationPrivatePointer();
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT
