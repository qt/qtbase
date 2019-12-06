/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
#include "../linux_common/qofonoservice_linux_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QNetworkManagerEngine::QNetworkManagerEngine(QObject *parent)
:   QBearerEngineImpl(parent),
    managerInterface(NULL),
    systemSettings(NULL),
    ofonoManager(NULL),
    nmAvailable(false)
{
    qDBusRegisterMetaType<QNmSettingsMap>();

    nmWatcher = new QDBusServiceWatcher(NM_DBUS_SERVICE,QDBusConnection::systemBus(),
            QDBusServiceWatcher::WatchForRegistration |
            QDBusServiceWatcher::WatchForUnregistration, this);
    connect(nmWatcher, SIGNAL(serviceRegistered(QString)),
            this, SLOT(nmRegistered(QString)));
    connect(nmWatcher, SIGNAL(serviceUnregistered(QString)),
            this, SLOT(nmUnRegistered(QString)));

    ofonoWatcher = new QDBusServiceWatcher("org.ofono",QDBusConnection::systemBus(),
            QDBusServiceWatcher::WatchForRegistration |
            QDBusServiceWatcher::WatchForUnregistration, this);
    connect(ofonoWatcher, SIGNAL(serviceRegistered(QString)),
            this, SLOT(ofonoRegistered(QString)));
    connect(ofonoWatcher, SIGNAL(serviceUnregistered(QString)),
            this, SLOT(ofonoUnRegistered(QString)));

    QDBusConnectionInterface *interface = QDBusConnection::systemBus().interface();

    if (!interface) return;

    if (interface->isServiceRegistered("org.ofono"))
        QMetaObject::invokeMethod(this, "ofonoRegistered", Qt::QueuedConnection);

    if (interface->isServiceRegistered(NM_DBUS_SERVICE))
        QMetaObject::invokeMethod(this, "nmRegistered", Qt::QueuedConnection);
}

QNetworkManagerEngine::~QNetworkManagerEngine()
{
    qDeleteAll(connections);
    connections.clear();
    qDeleteAll(accessPoints);
    accessPoints.clear();
    qDeleteAll(wirelessDevices);
    wirelessDevices.clear();
    qDeleteAll(activeConnectionsList);
    activeConnectionsList.clear();
    qDeleteAll(interfaceDevices);
    interfaceDevices.clear();

    connectionInterfaces.clear();

    qDeleteAll(ofonoContextManagers);
    ofonoContextManagers.clear();

    qDeleteAll(wiredDevices);
    wiredDevices.clear();
}

void QNetworkManagerEngine::initialize()
{
    if (nmAvailable)
        setupConfigurations();
}

void QNetworkManagerEngine::setupConfigurations()
{
    QMutexLocker locker(&mutex);
    // Get active connections.
    const auto acPaths = managerInterface->activeConnections();
    for (const QDBusObjectPath &acPath : acPaths) {

        if (activeConnectionsList.contains(acPath.path()))
            continue;

        QNetworkManagerConnectionActive *activeConnection =
                new QNetworkManagerConnectionActive(acPath.path(),this);
        activeConnectionsList.insert(acPath.path(), activeConnection);
        connect(activeConnection, SIGNAL(propertiesChanged(QMap<QString,QVariant>)),
                this, SLOT(activeConnectionPropertiesChanged(QMap<QString,QVariant>)));

        QStringList devices = activeConnection->devices();
        if (!devices.isEmpty()) {
            QNetworkManagerInterfaceDevice device(devices.at(0),this);
            connectionInterfaces.insert(activeConnection->connection().path(),device.networkInterface());
        }
    }

    // Get connections.
    const auto settingsPaths = systemSettings->listConnections();
    for (const QDBusObjectPath &settingsPath : settingsPaths) {
        locker.unlock();
        if (!hasIdentifier(settingsPath.path()))
            newConnection(settingsPath, systemSettings); //add system connection configs
        locker.relock();
    }

    Q_EMIT updateCompleted();
}

bool QNetworkManagerEngine::networkManagerAvailable() const
{
    return nmAvailable;
}

QString QNetworkManagerEngine::getInterfaceFromId(const QString &settingsPath)
{
    return connectionInterfaces.value(settingsPath);
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

    NMDeviceType connectionType = connection->getType();

    QString dbusDevicePath;
    const QString settingsPath = connection->path();
    QString specificPath = configuredAccessPoints.key(settingsPath);

    if (isConnectionActive(settingsPath))
        return;

    for (auto i = interfaceDevices.cbegin(), end = interfaceDevices.cend(); i != end; ++i) {
        const auto type = i.value()->deviceType();
        if (type == DEVICE_TYPE_ETHERNET || type == DEVICE_TYPE_WIFI || type == DEVICE_TYPE_MODEM) {
            if (type == connectionType) {
                dbusDevicePath = i.key();
                break;
            }
        }
    }

    if (specificPath.isEmpty())
        specificPath = "/";

    managerInterface->activateConnection(QDBusObjectPath(settingsPath),
                                  QDBusObjectPath(dbusDevicePath), QDBusObjectPath(specificPath));
}

void QNetworkManagerEngine::disconnectFromId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection = connectionFromId(id);

    if (!connection)
        return;

    QNmSettingsMap map = connection->getSettings();
    bool connectionAutoconnect = map.value("connection").value("autoconnect",true).toBool(); //if not present is true !!
    if (connectionAutoconnect) { //autoconnect connections will simply be reconnected by nm
        emit connectionError(id, QBearerEngineImpl::OperationNotSupported);
        return;
    }

    for (auto i = activeConnectionsList.cbegin(), end = activeConnectionsList.cend(); i != end; ++i) {
        if (id == i.value()->connection().path() && accessPointConfigurations.contains(id)) {
            managerInterface->deactivateConnection(QDBusObjectPath(i.key()));
            break;
        }
    }
}

void QNetworkManagerEngine::requestUpdate()
{
    if (managerInterface && managerInterface->wirelessEnabled()) {
        for (auto *wirelessDevice : qAsConst(wirelessDevices))
            wirelessDevice->requestScan();
    }
    QMetaObject::invokeMethod(this, "updateCompleted", Qt::QueuedConnection);
}

void QNetworkManagerEngine::interfacePropertiesChanged(const QMap<QString, QVariant> &properties)
{
    QMutexLocker locker(&mutex);

    for (auto i = properties.cbegin(), end = properties.cend(); i != end; ++i) {
        if (i.key() == QLatin1String("ActiveConnections")) {
            // Active connections changed, update configurations.

            const auto activeConnections = qdbus_cast<QList<QDBusObjectPath> >(qvariant_cast<QDBusArgument>(i.value()));

            QStringList identifiers = accessPointConfigurations.keys();
            QStringList priorActiveConnections = activeConnectionsList.keys();

            for (const QDBusObjectPath &acPath : activeConnections) {
                priorActiveConnections.removeOne(acPath.path());
                QNetworkManagerConnectionActive *activeConnection =
                    activeConnectionsList.value(acPath.path());

                if (!activeConnection) {
                    activeConnection = new QNetworkManagerConnectionActive(acPath.path(),this);
                    activeConnectionsList.insert(acPath.path(), activeConnection);

                    connect(activeConnection, SIGNAL(propertiesChanged(QMap<QString,QVariant>)),
                            this, SLOT(activeConnectionPropertiesChanged(QMap<QString,QVariant>)));
                }

                const QString id = activeConnection->connection().path();

                identifiers.removeOne(id);

                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
                if (ptr) {
                    ptr->mutex.lock();
                    if (activeConnection->state() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED &&
                            (ptr->state & QNetworkConfiguration::Active) != QNetworkConfiguration::Active) {

                        ptr->state |= QNetworkConfiguration::Active;

                        if (activeConnectionsList.value(id) && activeConnectionsList.value(id)->defaultRoute()
                                && managerInterface->state() < QNetworkManagerInterface::NM_STATE_CONNECTED_GLOBAL) {
                            ptr->purpose = QNetworkConfiguration::PrivatePurpose;
                        }
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
                delete activeConnectionsList.take(priorActiveConnections.takeFirst());

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

void QNetworkManagerEngine::activeConnectionPropertiesChanged(const QMap<QString, QVariant> &properties)
{
    QMutexLocker locker(&mutex);

    Q_UNUSED(properties)

    QNetworkManagerConnectionActive *activeConnection = qobject_cast<QNetworkManagerConnectionActive *>(sender());

    if (!activeConnection)
        return;

    const QString id = activeConnection->connection().path();

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);
    if (ptr) {
        if (properties.contains(QStringLiteral("State"))) {
            ptr->mutex.lock();
            if (properties.value("State").toUInt() == NM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
                QStringList devices = activeConnection->devices();
                if (!devices.isEmpty()) {
                    QNetworkManagerInterfaceDevice device(devices.at(0),this);
                    connectionInterfaces.insert(id,device.networkInterface());
                }

                ptr->state |= QNetworkConfiguration::Active;
                ptr->mutex.unlock();

                locker.unlock();
                emit configurationChanged(ptr);
                locker.relock();
            } else {
                connectionInterfaces.remove(id);
                ptr->mutex.unlock();
            }
        }
    }
}

void QNetworkManagerEngine::deviceConnectionsChanged(const QStringList &connectionsList)
{
    QMutexLocker locker(&mutex);
    for (int i = 0; i < connections.count(); ++i) {
        if (connectionsList.contains(connections.at(i)->path()))
            continue;

        const QString settingsPath = connections.at(i)->path();

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

void QNetworkManagerEngine::wiredCarrierChanged(bool carrier)
{
    QNetworkManagerInterfaceDeviceWired *deviceWired = qobject_cast<QNetworkManagerInterfaceDeviceWired *>(sender());
    if (!deviceWired)
        return;
    QMutexLocker locker(&mutex);
    const auto settingsPaths = systemSettings->listConnections();
    for (const QDBusObjectPath &settingsPath : settingsPaths) {
        for (int i = 0; i < connections.count(); ++i) {
            QNetworkManagerSettingsConnection *connection = connections.at(i);
            if (connection->getType() == DEVICE_TYPE_ETHERNET
                    && settingsPath.path() == connection->path()) {
                QNetworkConfigurationPrivatePointer ptr =
                        accessPointConfigurations.value(settingsPath.path());

                if (ptr) {
                    ptr->mutex.lock();
                    if (carrier)
                        ptr->state |= QNetworkConfiguration::Discovered;
                    else
                        ptr->state = QNetworkConfiguration::Defined;
                    ptr->mutex.unlock();
                    locker.unlock();
                    emit configurationChanged(ptr);
                    return;
                }
            }
        }
    }
}

void QNetworkManagerEngine::newConnection(const QDBusObjectPath &path,
                                          QNetworkManagerSettings *settings)
{
    QMutexLocker locker(&mutex);
    if (!settings)
        settings = qobject_cast<QNetworkManagerSettings *>(sender());

    if (!settings) {
        return;
    }

    QNetworkManagerSettingsConnection *connection =
        new QNetworkManagerSettingsConnection(settings->service(),
                                              path.path(),this);
    const QString settingsPath = connection->path();
    if (accessPointConfigurations.contains(settingsPath)) {
        return;
    }

    connections.append(connection);

    connect(connection,SIGNAL(removed(QString)),this,SLOT(removeConnection(QString)));
    connect(connection,SIGNAL(updated()),this,SLOT(updateConnection()));
    connection->setConnections();

    NMDeviceType deviceType = connection->getType();

    if (deviceType == DEVICE_TYPE_WIFI) {
        QString apPath;
        for (int i = 0; i < accessPoints.count(); ++i) {
            if (connection->getSsid() == accessPoints.at(i)->ssid()) {
                // remove the corresponding accesspoint from configurations
                apPath = accessPoints.at(i)->path();
                QNetworkConfigurationPrivatePointer ptr
                        = accessPointConfigurations.take(apPath);
                if (ptr) {
                    locker.unlock();
                    emit configurationRemoved(ptr);
                    locker.relock();
                }
            }
        }
        if (!configuredAccessPoints.contains(settingsPath))
            configuredAccessPoints.insert(apPath,settingsPath);
    }

    QNetworkConfigurationPrivate *cpPriv =
        parseConnection(settingsPath, connection->getSettings());

    // Check if connection is active.
    if (isConnectionActive(settingsPath))
        cpPriv->state |= QNetworkConfiguration::Active;

    if (deviceType == DEVICE_TYPE_ETHERNET) {
        for (auto interfaceDevice : qAsConst(interfaceDevices)) {
             if (interfaceDevice->deviceType() == deviceType) {
                 auto *wiredDevice = wiredDevices.value(interfaceDevice->path());
                 if (wiredDevice && wiredDevice->carrier()) {
                     cpPriv->state |= QNetworkConfiguration::Discovered;
                 }
             }
         }
     }

    QNetworkConfigurationPrivatePointer ptr(cpPriv);
    accessPointConfigurations.insert(ptr->id, ptr);
    locker.unlock();
    emit configurationAdded(ptr);
}

bool QNetworkManagerEngine::isConnectionActive(const QString &settingsPath) const
{
    for (QNetworkManagerConnectionActive *activeConnection : activeConnectionsList) {
        if (activeConnection->connection().path() == settingsPath) {
            const auto state = activeConnection->state();
            if (state == NM_ACTIVE_CONNECTION_STATE_ACTIVATING
                    || state == NM_ACTIVE_CONNECTION_STATE_ACTIVATED) {
                return true;
            } else {
                break;
            }
        }
    }

    QNetworkManagerSettingsConnection *settingsConnection = connectionFromId(settingsPath);
    if (settingsConnection && settingsConnection->getType() == DEVICE_TYPE_MODEM) {
        return isActiveContext(settingsConnection->path());
    }

    return false;
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

    // removed along with all AP props code...
}

void QNetworkManagerEngine::updateConnection()
{
    QMutexLocker locker(&mutex);

    QNetworkManagerSettingsConnection *connection =
        qobject_cast<QNetworkManagerSettingsConnection *>(sender());
    if (!connection)
        return;
    const QString settingsPath = connection->path();

    QNetworkConfigurationPrivate *cpPriv = parseConnection(settingsPath, connection->getSettings());

    // Check if connection is active.
    const auto acPaths = managerInterface->activeConnections();
    for (const QDBusObjectPath &acPath : acPaths) {
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
    QDBusPendingReply<QDBusObjectPath> reply(*watcher);
    watcher->deleteLater();

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

QNetworkConfigurationPrivate *QNetworkManagerEngine::parseConnection(const QString &settingsPath,
                                                                     const QNmSettingsMap &map)
{
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

        const auto devicePaths = managerInterface->getDevices();
        for (const QDBusObjectPath &devicePath : devicePaths) {
            QNetworkManagerInterfaceDevice device(devicePath.path(),this);
            if (device.deviceType() == DEVICE_TYPE_ETHERNET) {
                QNetworkManagerInterfaceDeviceWired *wiredDevice = wiredDevices.value(device.path());
                if (wiredDevice && wiredDevice->carrier()) {
                    cpPriv->state |= QNetworkConfiguration::Discovered;
                    break;
                }
            }
        }
    } else if (connectionType == QLatin1String("802-11-wireless")) {
        cpPriv->bearerType = QNetworkConfiguration::BearerWLAN;

        const QString connectionSsid = map.value("802-11-wireless").value("ssid").toString();
        for (int i = 0; i < accessPoints.count(); ++i) {
            if (connectionSsid == accessPoints.at(i)->ssid()
                    && map.value("802-11-wireless").value("seen-bssids").toStringList().contains(accessPoints.at(i)->hwAddress())) {
                cpPriv->state |= QNetworkConfiguration::Discovered;
                if (!configuredAccessPoints.contains(accessPoints.at(i)->path())) {
                    configuredAccessPoints.insert(accessPoints.at(i)->path(),settingsPath);

                    const QString accessPointId = accessPoints.at(i)->path();
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

        const QString connectionPath = map.value("connection").value("id").toString();
        cpPriv->name = contextName(connectionPath);
        cpPriv->bearerType = currentBearerType(connectionPath);

        if (ofonoManager && ofonoManager->isValid()) {
            const QString contextPart = connectionPath.section('/', -1);
            for (auto i = ofonoContextManagers.cbegin(), end = ofonoContextManagers.cend(); i != end; ++i) {
                const QString path = i.key() + QLatin1Char('/') +contextPart;
                if (isActiveContext(path)) {
                    cpPriv->state |= QNetworkConfiguration::Active;
                    break;
                }
            }
        }
    }

    return cpPriv;
}

bool QNetworkManagerEngine::isActiveContext(const QString &contextPath) const
{
    if (ofonoManager && ofonoManager->isValid()) {
        const QString contextPart = contextPath.section('/', -1);
        for (QOfonoDataConnectionManagerInterface *iface : ofonoContextManagers) {
            const PathPropertiesList list = iface->contextsWithProperties();
            for (int i = 0; i < list.size(); ++i) {
                if (list.at(i).path.path().contains(contextPart)) {
                    return list.at(i).properties.value(QStringLiteral("Active")).toBool();

                }
            }
        }
    }
    return false;
}

QNetworkManagerSettingsConnection *QNetworkManagerEngine::connectionFromId(const QString &id) const
{
    for (int i = 0; i < connections.count(); ++i) {
        QNetworkManagerSettingsConnection *connection = connections.at(i);
        if (id == connection->path())
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

    for (QNetworkManagerConnectionActive *activeConnection : qAsConst(activeConnectionsList)) {
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
        const QString networkInterface = connectionInterfaces.value(id);
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
        const QString networkInterface = connectionInterfaces.value(id);
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
    for (QNetworkManagerConnectionActive *activeConnection : qAsConst(activeConnectionsList)) {
        if ((activeConnection->defaultRoute() || activeConnection->default6Route())) {
            return accessPointConfigurations.value(activeConnection->connection().path());
        }
    }

    return QNetworkConfigurationPrivatePointer();
}

QNetworkConfiguration::BearerType QNetworkManagerEngine::currentBearerType(const QString &id) const
{
    QString contextPart = id.section('/', -1);
    for (auto i = ofonoContextManagers.begin(), end = ofonoContextManagers.end(); i != end; ++i) {
        QString contextPath = i.key() + QLatin1Char('/') +contextPart;

        if (i.value()->contexts().contains(contextPath)) {

            QString bearer = i.value()->bearer();

            if (bearer == QLatin1String("gsm")) {
                return QNetworkConfiguration::Bearer2G;
            } else if (bearer == QLatin1String("edge")) {
                return QNetworkConfiguration::Bearer2G;
            } else if (bearer == QLatin1String("umts")) {
                return QNetworkConfiguration::BearerWCDMA;
            } else if (bearer == QLatin1String("hspa")
                       || bearer == QLatin1String("hsdpa")
                       || bearer == QLatin1String("hsupa")) {
                return QNetworkConfiguration::BearerHSPA;
            } else if (bearer == QLatin1String("lte")) {
                return QNetworkConfiguration::BearerLTE;
            }
        }
    }

    return QNetworkConfiguration::BearerUnknown;
}

QString QNetworkManagerEngine::contextName(const QString &path) const
{
    QString contextPart = path.section('/', -1);
    for (QOfonoDataConnectionManagerInterface *iface : ofonoContextManagers) {
        const PathPropertiesList list = iface->contextsWithProperties();
        for (int i = 0; i < list.size(); ++i) {
            if (list.at(i).path.path().contains(contextPart)) {
                return list.at(i).properties.value(QStringLiteral("Name")).toString();
            }
        }
    }
    return path;
}

void QNetworkManagerEngine::nmRegistered(const QString &)
{
    if (ofonoManager) {
        delete ofonoManager;
        ofonoManager = NULL;
    }
    managerInterface = new QNetworkManagerInterface(this);
    systemSettings = new QNetworkManagerSettings(NM_DBUS_SERVICE, this);

    connect(managerInterface, SIGNAL(activationFinished(QDBusPendingCallWatcher*)),
            this, SLOT(activationFinished(QDBusPendingCallWatcher*)));
    connect(managerInterface, SIGNAL(propertiesChanged(QMap<QString,QVariant>)),
            this, SLOT(interfacePropertiesChanged(QMap<QString,QVariant>)));
    managerInterface->setConnections();

    connect(systemSettings, SIGNAL(newConnection(QDBusObjectPath)),
            this, SLOT(newConnection(QDBusObjectPath)));
    systemSettings->setConnections();
    nmAvailable = true;

    setupConfigurations();
}

void QNetworkManagerEngine::nmUnRegistered(const QString &)
{
    if (systemSettings) {
        delete systemSettings;
        systemSettings = NULL;
    }
    if (managerInterface) {
        delete managerInterface;
        managerInterface = NULL;
    }
}

void QNetworkManagerEngine::ofonoRegistered(const QString &)
{
    if (ofonoManager) {
        delete ofonoManager;
        ofonoManager = NULL;
    }
    ofonoManager = new QOfonoManagerInterface(this);
    if (ofonoManager && ofonoManager->isValid()) {
        const auto modems = ofonoManager->getModems();
        for (const QString &modem : modems) {
            QOfonoDataConnectionManagerInterface *ofonoContextManager
                    = new QOfonoDataConnectionManagerInterface(modem,this);
            ofonoContextManagers.insert(modem, ofonoContextManager);
        }
    }
}

void QNetworkManagerEngine::ofonoUnRegistered(const QString &)
{
    ofonoContextManagers.clear();
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
