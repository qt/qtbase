/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qconnmanengine.h"
#include "qconnmanservice_linux_p.h"
#include "qofonoservice_linux_p.h"
#include "../qnetworksession_impl.h"

#include <QtNetwork/private/qnetworkconfiguration_p.h>

#include <QtNetwork/qnetworksession.h>

#include <QtCore/qdebug.h>

#include <QtDBus/QtDBus>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QConnmanEngine::QConnmanEngine(QObject *parent)
:   QBearerEngineImpl(parent),
    connmanManager(new QConnmanManagerInterface(this))
{
}

QConnmanEngine::~QConnmanEngine()
{
}

bool QConnmanEngine::connmanAvailable() const
{
    QMutexLocker locker(&mutex);
    return connmanManager->isValid();
}

void QConnmanEngine::initialize()
{
    connect(connmanManager,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
            this,SLOT(propertyChangedContext(QString,QString,QDBusVariant)));

    foreach(const QString techPath, connmanManager->getTechnologies()) {
        QConnmanTechnologyInterface *tech;
        tech = new QConnmanTechnologyInterface(techPath, this);

        connect(tech,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                this,SLOT(technologyPropertyChangedContext(QString,QString,QDBusVariant)));
    }

    foreach(const QString servPath, connmanManager->getServices()) {
        addServiceConfiguration(servPath);
    }

    // Get current list of access points.
    getConfigurations();
}

QList<QNetworkConfigurationPrivate *> QConnmanEngine::getConfigurations()
{
    QMutexLocker locker(&mutex);
    QList<QNetworkConfigurationPrivate *> fetchedConfigurations;
    QNetworkConfigurationPrivate* cpPriv = 0;

    for (int i = 0; i < foundConfigurations.count(); ++i) {
        QNetworkConfigurationPrivate *config = new QNetworkConfigurationPrivate;
        cpPriv = foundConfigurations.at(i);

        config->name = cpPriv->name;
        config->isValid = cpPriv->isValid;
        config->id = cpPriv->id;
        config->state = cpPriv->state;
        config->type = cpPriv->type;
        config->roamingSupported = cpPriv->roamingSupported;
        config->purpose = cpPriv->purpose;
        config->bearerType = cpPriv->bearerType;

        fetchedConfigurations.append(config);
        delete config;
    }
    return fetchedConfigurations;
}

void QConnmanEngine::doRequestUpdate()
{
    connmanManager->requestScan("");
    getConfigurations();
    emit updateCompleted();
}

QString QConnmanEngine::getInterfaceFromId(const QString &id)
{
    QMutexLocker locker(&mutex);
    return configInterfaces.value(id);
}

bool QConnmanEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);
    return accessPointConfigurations.contains(id);
}

void QConnmanEngine::connectToId(const QString &id)
{
    QMutexLocker locker(&mutex);
    QString servicePath = serviceFromId(id);
    QConnmanServiceInterface serv(servicePath);
    if(!serv.isValid()) {
        emit connectionError(id, QBearerEngineImpl::InterfaceLookupError);
    } else {
        if(serv.getType() != "cellular") {

            serv.connect();
        } else {
            QOfonoManagerInterface ofonoManager(0);
            QString modemPath = ofonoManager.currentModem().path();
            QOfonoDataConnectionManagerInterface dc(modemPath,0);
            foreach(const QDBusObjectPath dcPath,dc.getPrimaryContexts()) {
                if(dcPath.path().contains(servicePath.section("_",-1))) {
                    QOfonoPrimaryDataContextInterface primaryContext(dcPath.path(),0);
                    primaryContext.setActive(true);
                }
            }
        }
    }
}

void QConnmanEngine::disconnectFromId(const QString &id)
{
    QMutexLocker locker(&mutex);
    QString servicePath = serviceFromId(id);
    QConnmanServiceInterface serv(servicePath);
    if(!serv.isValid()) {
        emit connectionError(id, DisconnectionError);
    } else {
        if(serv.getType() != "cellular") {
            serv.disconnect();
        } else {
            QOfonoManagerInterface ofonoManager(0);
            QString modemPath = ofonoManager.currentModem().path();
            QOfonoDataConnectionManagerInterface dc(modemPath,0);
            foreach(const QDBusObjectPath dcPath,dc.getPrimaryContexts()) {
                if(dcPath.path().contains(servicePath.section("_",-1))) {
                    QOfonoPrimaryDataContextInterface primaryContext(dcPath.path(),0);
                    primaryContext.setActive(false);
                }
            }
        }
    }
}

void QConnmanEngine::requestUpdate()
{
    QMutexLocker locker(&mutex);
    QTimer::singleShot(0, this, SLOT(doRequestUpdate()));
}

QString QConnmanEngine::serviceFromId(const QString &id)
{
    QMutexLocker locker(&mutex);
    foreach(const QString service, serviceNetworks) {
        if (id == QString::number(qHash(service)))
            return service;
    }

    return QString();
}

QNetworkSession::State QConnmanEngine::sessionStateForId(const QString &id)
{
    QMutexLocker locker(&mutex);

    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

    if (!ptr)
        return QNetworkSession::Invalid;

    if (!ptr->isValid) {
        return QNetworkSession::Invalid;

    }
    QString service = serviceFromId(id);
    QConnmanServiceInterface serv(service);
    QString servState = serv.getState();

    if(serv.isFavorite() && (servState == "idle" || servState == "failure")) {
        return QNetworkSession::Disconnected;
    }

    if(servState == "association" || servState == "configuration" || servState == "login") {
        return QNetworkSession::Connecting;
    }
    if(servState == "ready" || servState == "online") {
        return QNetworkSession::Connected;
    }

    if ((ptr->state & QNetworkConfiguration::Discovered) ==
                QNetworkConfiguration::Discovered) {
        return QNetworkSession::Disconnected;
    } else if ((ptr->state & QNetworkConfiguration::Defined) == QNetworkConfiguration::Defined) {
        return QNetworkSession::NotAvailable;
    } else if ((ptr->state & QNetworkConfiguration::Undefined) ==
                QNetworkConfiguration::Undefined) {
        return QNetworkSession::NotAvailable;
    }

    return QNetworkSession::Invalid;
}

quint64 QConnmanEngine::bytesWritten(const QString &id)
{//TODO use connman counter API
    QMutexLocker locker(&mutex);
    quint64 result = 0;
    QString devFile = getInterfaceFromId(id);
    QFile tx("/sys/class/net/"+devFile+"/statistics/tx_bytes");
    if(tx.exists() && tx.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&tx);
        in >> result;
        tx.close();
    }

    return result;
}

quint64 QConnmanEngine::bytesReceived(const QString &id)
{//TODO use connman counter API
    QMutexLocker locker(&mutex);
    quint64 result = 0;
    QString devFile = getInterfaceFromId(id);
    QFile rx("/sys/class/net/"+devFile+"/statistics/rx_bytes");
    if(rx.exists() && rx.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&rx);
        in >> result;
        rx.close();
    }
    return result;
}

quint64 QConnmanEngine::startTime(const QString &/*id*/)
{
    // TODO
    QMutexLocker locker(&mutex);
    if (activeTime.isNull()) {
        return 0;
    }
    return activeTime.secsTo(QDateTime::currentDateTime());
}

QNetworkConfigurationManager::Capabilities QConnmanEngine::capabilities() const
{
    return QNetworkConfigurationManager::ForcedRoaming |
            QNetworkConfigurationManager::DataStatistics |
           QNetworkConfigurationManager::CanStartAndStopInterfaces;
}

QNetworkSessionPrivate *QConnmanEngine::createSessionBackend()
{
     return new QNetworkSessionPrivateImpl;
}

QNetworkConfigurationPrivatePointer QConnmanEngine::defaultConfiguration()
{
    return QNetworkConfigurationPrivatePointer();
}

void QConnmanEngine::propertyChangedContext(const QString &path,const QString &item, const QDBusVariant &value)
{
    Q_UNUSED(path);

    QMutexLocker locker(&mutex);
    if(item == "Services") {
        QDBusArgument arg = qvariant_cast<QDBusArgument>(value.variant());
        QStringList list = qdbus_cast<QStringList>(arg);

        if(list.count() > accessPointConfigurations.count()) {
            foreach(const QString service, list) {
                addServiceConfiguration(service);
            }
        }
    }

    if(item == "Technologies") {
        QDBusArgument arg = qvariant_cast<QDBusArgument>(value.variant());
        QStringList newlist = qdbus_cast<QStringList>(arg);
        if(newlist.count() > 0) {
            QMap<QString,QConnmanTechnologyInterface *> oldtech = technologies;

            foreach(const QString listPath, newlist) {
                if(!oldtech.contains(listPath)) {
                    QConnmanTechnologyInterface *tech;
                    tech = new QConnmanTechnologyInterface(listPath,this);
                    connect(tech,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                            this,SLOT(technologyPropertyChangedContext(QString,QString,QDBusVariant)));
                    technologies.insert(listPath, tech);
                }
            }
        }
    }
    if(item == "State") {
// qDebug() << value.variant();
    }
}

void QConnmanEngine::servicePropertyChangedContext(const QString &path,const QString &item, const QDBusVariant &value)
{
    QMutexLocker locker(&mutex);
    if(item == "State") {
        configurationChange(QString::number(qHash(path)));

        if(value.variant().toString() == "failure") {
            QConnmanServiceInterface serv(path);
            emit connectionError(QString::number(qHash(path)), ConnectError);
        }
    }
}

void QConnmanEngine::technologyPropertyChangedContext(const QString & path, const QString &item, const QDBusVariant &value)
{
    if(item == "State") {
        if(value.variant().toString() == "offline") {
            QConnmanTechnologyInterface tech(path);
            disconnect(&tech,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                       this,SLOT(technologyPropertyChangedContext(QString,QString,QDBusVariant)));

            technologies.remove(path);
        }
    }
}

void QConnmanEngine::configurationChange(const QString &id)
{
    QMutexLocker locker(&mutex);

    if (accessPointConfigurations.contains(id)) {

        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(id);

        QString servicePath = serviceFromId(id);
        QConnmanServiceInterface *serv;
        serv = new QConnmanServiceInterface(servicePath);
        QString networkName = serv->getName();

        QNetworkConfiguration::StateFlags curState = getStateForService(servicePath);

        ptr->mutex.lock();

        if (!ptr->isValid) {
            ptr->isValid = true;
        }

        if (ptr->name != networkName) {
            ptr->name = networkName;
        }

        if (ptr->state != curState) {
            ptr->state = curState;
        }

        ptr->mutex.unlock();

        locker.unlock();
        emit configurationChanged(ptr);
        locker.relock();
    }

     locker.unlock();
     emit updateCompleted();
}

QNetworkConfiguration::StateFlags QConnmanEngine::getStateForService(const QString &service)
{
    QMutexLocker locker(&mutex);
    QConnmanServiceInterface serv(service);
    QNetworkConfiguration::StateFlags flag = QNetworkConfiguration::Defined;
    if( serv.getType() == "cellular") {
        if(serv.isSetupRequired()) {
            flag = ( flag | QNetworkConfiguration::Defined);
        } else {
            flag = ( flag | QNetworkConfiguration::Discovered);
        }
    } else {
        if(serv.isFavorite()) {
            flag = ( flag | QNetworkConfiguration::Discovered);
        } else {
            flag = QNetworkConfiguration::Undefined;
        }
    }

    if(serv.getState() == "ready" || serv.getState() == "online") {
        flag = ( flag | QNetworkConfiguration::Active);
    }

    return flag;
}

QNetworkConfiguration::BearerType QConnmanEngine::typeToBearer(const QString &type)
{
    if (type == "wifi")
        return QNetworkConfiguration::BearerWLAN;
    if (type == "ethernet")
        return QNetworkConfiguration::BearerEthernet;
    if (type == "bluetooth")
        return QNetworkConfiguration::BearerBluetooth;
    if (type == "cellular") {
        return ofonoTechToBearerType(type);
    }
    if (type == "wimax")
        return QNetworkConfiguration::BearerWiMAX;

//    if(type == "gps")
//    if(type == "vpn")

    return QNetworkConfiguration::BearerUnknown;
}

QNetworkConfiguration::BearerType QConnmanEngine::ofonoTechToBearerType(const QString &/*type*/)
{
    QOfonoManagerInterface ofonoManager(this);
    QOfonoNetworkRegistrationInterface ofonoNetwork(ofonoManager.currentModem().path(),this);

    if(ofonoNetwork.isValid()) {
        foreach(const QDBusObjectPath op,ofonoNetwork.getOperators() ) {
            QOfonoNetworkOperatorInterface opIface(op.path(),this);

            foreach(const QString opTech, opIface.getTechnologies()) {

                if(opTech == "gsm") {
                    return QNetworkConfiguration::Bearer2G;
                }
                if(opTech == "edge"){
                    return QNetworkConfiguration::BearerCDMA2000; //wrong, I know
                }
                if(opTech == "umts"){
                    return QNetworkConfiguration::BearerWCDMA;
                }
                if(opTech == "hspa"){
                    return QNetworkConfiguration::BearerHSPA;
                }
                if(opTech == "lte"){
                    return QNetworkConfiguration::BearerWiMAX; //not exact
                }
            }
        }
    }
    return QNetworkConfiguration::BearerUnknown;
}

bool QConnmanEngine::isRoamingAllowed(const QString &context)
{
    QOfonoManagerInterface ofonoManager(this);
    QString modemPath = ofonoManager.currentModem().path();
    QOfonoDataConnectionManagerInterface dc(modemPath,this);
    foreach(const QDBusObjectPath dcPath,dc.getPrimaryContexts()) {
        if(dcPath.path().contains(context.section("_",-1))) {
            return dc.isRoamingAllowed();
        }
    }
    return false;
}

void QConnmanEngine::removeConfiguration(const QString &id)
{
    QMutexLocker locker(&mutex);

    if (accessPointConfigurations.contains(id)) {

        QString service = serviceFromId(id);
        QConnmanServiceInterface serv(service);

        disconnect(&serv,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                   this,SLOT(servicePropertyChangedContext(QString,QString, QDBusVariant)));

        serviceNetworks.removeOne(service);

        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.take(id);
        locker.unlock();
        emit configurationRemoved(ptr);
        locker.relock();
    }
}

void QConnmanEngine::addServiceConfiguration(const QString &servicePath)
{
    QMutexLocker locker(&mutex);
    QConnmanServiceInterface *serv;
    serv = new QConnmanServiceInterface(servicePath);

    const QString id = QString::number(qHash(servicePath));

    if (!accessPointConfigurations.contains(id)) {
        serviceNetworks.append(servicePath);

        connect(serv,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                this,SLOT(servicePropertyChangedContext(QString,QString, QDBusVariant)));
        QNetworkConfigurationPrivate* cpPriv = new QNetworkConfigurationPrivate();

        QString networkName = serv->getName();

        const QString connectionType = serv->getType();
        if (connectionType == "ethernet") {
            cpPriv->bearerType = QNetworkConfiguration::BearerEthernet;
        } else if (connectionType == "wifi") {
            cpPriv->bearerType = QNetworkConfiguration::BearerWLAN;
        } else if (connectionType == "cellular") {
            cpPriv->bearerType = ofonoTechToBearerType("cellular");
            if(servicePath.isEmpty()) {
                networkName = serv->getAPN();
                if(networkName.isEmpty()) {
                    networkName = serv->getName();
                }
            }
            cpPriv->roamingSupported = isRoamingAllowed(servicePath);
        } else if (connectionType == "wimax") {
            cpPriv->bearerType = QNetworkConfiguration::BearerWiMAX;
        } else {
            cpPriv->bearerType = QNetworkConfiguration::BearerUnknown;
        }

        cpPriv->name = networkName;
        cpPriv->isValid = true;
        cpPriv->id = id;
        cpPriv->type = QNetworkConfiguration::InternetAccessPoint;

        if(serv->getSecurity() == "none") {
            cpPriv->purpose = QNetworkConfiguration::PublicPurpose;
        } else {
            cpPriv->purpose = QNetworkConfiguration::PrivatePurpose;
        }

        cpPriv->state = getStateForService(servicePath);

        QNetworkConfigurationPrivatePointer ptr(cpPriv);
        accessPointConfigurations.insert(ptr->id, ptr);
        foundConfigurations.append(cpPriv);
        configInterfaces[cpPriv->id] = serv->getInterface(); 

        locker.unlock();
        emit configurationAdded(ptr);
        locker.relock();
        emit updateCompleted();
    }
}

bool QConnmanEngine::requiresPolling() const
{
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT
