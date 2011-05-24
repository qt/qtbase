/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qnetworksession_impl.h"
#include "qicdengine.h"

#include <QHash>

#include <maemo_icd.h>
#include <iapconf.h>
#include <proxyconf.h>

#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

QDBusArgument &operator<<(QDBusArgument &argument,
                          const ICd2DetailsDBusStruct &icd2)
{
    argument.beginStructure();

    argument << icd2.serviceType;
    argument << icd2.serviceAttributes;
    argument << icd2.setviceId;
    argument << icd2.networkType;
    argument << icd2.networkAttributes;
    argument << icd2.networkId;

    argument.endStructure();

    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                ICd2DetailsDBusStruct &icd2)
{
    argument.beginStructure();

    argument >> icd2.serviceType;
    argument >> icd2.serviceAttributes;
    argument >> icd2.setviceId;
    argument >> icd2.networkType;
    argument >> icd2.networkAttributes;
    argument >> icd2.networkId;

    argument.endStructure();

    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                ICd2DetailsList &detailsList)
{
     argument.beginArray();
     detailsList.clear();

     while (!argument.atEnd()) {
         ICd2DetailsDBusStruct element;
         argument >> element;
         detailsList.append(element);
     }

     argument.endArray();
     return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument,
                          const ICd2DetailsList &detailsList)
{
     argument.beginArray(qMetaTypeId<ICd2DetailsDBusStruct>());

     for (int i = 0; i < detailsList.count(); ++i)
         argument << detailsList[i];

     argument.endArray();

     return argument;
}

static QHash<QString, QVariant> properties;

static QString get_network_interface();

void QNetworkSessionPrivateImpl::iapStateChanged(const QString& iapid, uint icd_connection_state)
{

    if (((publicConfig.type() == QNetworkConfiguration::UserChoice) &&
         (activeConfig.identifier() == iapid)) ||
        (publicConfig.identifier() == iapid)) {
        switch (icd_connection_state) {
        case ICD_STATE_CONNECTING:
            updateState(QNetworkSession::Connecting);
            break;
        case ICD_STATE_CONNECTED:
            updateState(QNetworkSession::Connected);
            break;
        case ICD_STATE_DISCONNECTING:
            updateState(QNetworkSession::Closing);
            break;
        case ICD_STATE_DISCONNECTED:
            updateState(QNetworkSession::Disconnected);
            break;
        default:
            break;
        }
    }
    if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
        updateIdentifier(iapid);
    }
}

void QNetworkSessionPrivateImpl::cleanupSession(void)
{
    QObject::disconnect(q, SIGNAL(stateChanged(QNetworkSession::State)),
                        this, SLOT(updateProxies(QNetworkSession::State)));
}


void QNetworkSessionPrivateImpl::updateState(QNetworkSession::State newState)
{
    if (newState != state) {
        if (newState == QNetworkSession::Disconnected) {
            if (isOpen) {
                // The Session was aborted by the user or system
                lastError = QNetworkSession::SessionAbortedError;
                emit QNetworkSessionPrivate::error(lastError);
                emit closed();
            }
            if (m_stopTimer.isActive()) {
                // Session was closed by calling stop()
                m_stopTimer.stop();
            }
            isOpen = false;
            opened = false;
            currentNetworkInterface.clear();
            if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
                copyConfig(publicConfig, activeConfig);
                IcdNetworkConfigurationPrivate *icdConfig =
                    toIcdConfig(privateConfiguration(activeConfig));

                icdConfig->mutex.lock();
                icdConfig->state = QNetworkConfiguration::Defined;
                icdConfig->mutex.unlock();
            } else {
                if (!activeConfig.isValid()) {
                    // Active configuration (IAP) was removed from system
                    // => Connection was disconnected and configuration became
                    //    invalid
                    // => Also Session state must be changed to invalid
                    newState = QNetworkSession::Invalid;
                }
            }
        } else if (newState == QNetworkSession::Connected) {
            if (opened) {
                isOpen = true;
            }
	    if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
            IcdNetworkConfigurationPrivate *icdConfig =
                toIcdConfig(privateConfiguration(activeConfig));

            icdConfig->mutex.lock();
            icdConfig->state = QNetworkConfiguration::Active;
            icdConfig->type = QNetworkConfiguration::InternetAccessPoint;
            icdConfig->mutex.unlock();
	    }

        IcdNetworkConfigurationPrivate *icdConfig =
            toIcdConfig(privateConfiguration(publicConfig));

        icdConfig->mutex.lock();
        icdConfig->state = QNetworkConfiguration::Active;
        icdConfig->mutex.unlock();
	}

        if (newState != state) {
            state = newState;
            emit stateChanged(newState);
        }
    }
}


void QNetworkSessionPrivateImpl::updateIdentifier(const QString &newId)
{
    if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
        IcdNetworkConfigurationPrivate *icdConfig =
            toIcdConfig(privateConfiguration(activeConfig));

        icdConfig->mutex.lock();
        icdConfig->network_attrs |= ICD_NW_ATTR_IAPNAME;
        icdConfig->id = newId;
        icdConfig->mutex.unlock();
    } else {
        IcdNetworkConfigurationPrivate *icdConfig =
            toIcdConfig(privateConfiguration(publicConfig));

        icdConfig->mutex.lock();
        icdConfig->network_attrs |= ICD_NW_ATTR_IAPNAME;
        if (icdConfig->id != newId)
            icdConfig->id = newId;
        icdConfig->mutex.unlock();
    }
}


QNetworkSessionPrivateImpl::Statistics QNetworkSessionPrivateImpl::getStatistics() const
{
    /* This could be also implemented by using the Maemo::Icd::statistics()
     * that gets the statistics data for a specific IAP. Change if
     * necessary.
     */
    Maemo::Icd icd;
    QList<Maemo::IcdStatisticsResult> stats_results;
    Statistics stats = { 0, 0, 0};

    if (!icd.statistics(stats_results))
        return stats;

    foreach (const Maemo::IcdStatisticsResult &res, stats_results) {
        if (res.params.network_attrs & ICD_NW_ATTR_IAPNAME) {
            /* network_id is the IAP UUID */
            if (QString(res.params.network_id.data()) == activeConfig.identifier()) {
                stats.txData = res.bytes_sent;
                stats.rxData = res.bytes_received;
                stats.activeTime = res.time_active;
            }
        } else {
            /* We probably will never get to this branch */
            IcdNetworkConfigurationPrivate *icdConfig =
                toIcdConfig(privateConfiguration(activeConfig));

            icdConfig->mutex.lock();
            if (res.params.network_id == icdConfig->network_id) {
                stats.txData = res.bytes_sent;
                stats.rxData = res.bytes_received;
                stats.activeTime = res.time_active;
            }
            icdConfig->mutex.unlock();
        }
    }

    return stats;
}


quint64 QNetworkSessionPrivateImpl::bytesWritten() const
{
    return getStatistics().txData;
}

quint64 QNetworkSessionPrivateImpl::bytesReceived() const
{
    return getStatistics().rxData;
}

quint64 QNetworkSessionPrivateImpl::activeTime() const
{
    return getStatistics().activeTime;
}


QNetworkConfiguration& QNetworkSessionPrivateImpl::copyConfig(QNetworkConfiguration &fromConfig,
                                                              QNetworkConfiguration &toConfig,
                                                              bool deepCopy)
{
    IcdNetworkConfigurationPrivate *cpPriv;
    if (deepCopy) {
        cpPriv = new IcdNetworkConfigurationPrivate;
        setPrivateConfiguration(toConfig, QNetworkConfigurationPrivatePointer(cpPriv));
    } else {
        cpPriv = toIcdConfig(privateConfiguration(toConfig));
    }

    IcdNetworkConfigurationPrivate *fromPriv = toIcdConfig(privateConfiguration(fromConfig));

    QMutexLocker toLocker(&cpPriv->mutex);
    QMutexLocker fromLocker(&fromPriv->mutex);

    cpPriv->name = fromPriv->name;
    cpPriv->isValid = fromPriv->isValid;
    // Note that we do not copy id field here as the publicConfig does
    // not contain a valid IAP id.
    cpPriv->state = fromPriv->state;
    cpPriv->type = fromPriv->type;
    cpPriv->roamingSupported = fromPriv->roamingSupported;
    cpPriv->purpose = fromPriv->purpose;
    cpPriv->network_id = fromPriv->network_id;
    cpPriv->iap_type = fromPriv->iap_type;
    cpPriv->bearerType = fromPriv->bearerType;
    cpPriv->network_attrs = fromPriv->network_attrs;
    cpPriv->service_type = fromPriv->service_type;
    cpPriv->service_id = fromPriv->service_id;
    cpPriv->service_attrs = fromPriv->service_attrs;

    return toConfig;
}


/* This is called by QNetworkSession constructor and it updates the current
 * state of the configuration.
 */
void QNetworkSessionPrivateImpl::syncStateWithInterface()
{
    /* Initially we are not active although the configuration might be in
     * connected state.
     */
    isOpen = false;
    opened = false;

    connect(engine, SIGNAL(iapStateChanged(const QString&, uint)),
            this, SLOT(iapStateChanged(const QString&, uint)));

    QObject::connect(q, SIGNAL(stateChanged(QNetworkSession::State)), this, SLOT(updateProxies(QNetworkSession::State)));

    state = QNetworkSession::Invalid;
    lastError = QNetworkSession::UnknownSessionError;

    switch (publicConfig.type()) {
    case QNetworkConfiguration::InternetAccessPoint:
        activeConfig = publicConfig;
        break;
    case QNetworkConfiguration::ServiceNetwork:
        serviceConfig = publicConfig;
	break;
    case QNetworkConfiguration::UserChoice:
	// active config will contain correct data after open() has succeeded
        copyConfig(publicConfig, activeConfig);

	/* We create new configuration that holds the actual configuration
	 * returned by icd. This way publicConfig still contains the
	 * original user specified configuration.
	 *
	 * Note that the new activeConfig configuration is not inserted
	 * to configurationManager as manager class will get the newly
	 * connected configuration from gconf when the IAP is saved.
	 * This configuration manager update is done by IapMonitor class.
	 * If the ANY connection fails in open(), then the configuration
	 * data is not saved to gconf and will not be added to
	 * configuration manager IAP list.
	 */
#ifdef BEARER_MANAGEMENT_DEBUG
	qDebug()<<"New configuration created for" << publicConfig.identifier();
#endif
	break;
    default:
	/* Invalid configuration, no point continuing */
	return;
    }

    if (!activeConfig.isValid())
	return;

    /* Get the initial state from icd */
    Maemo::Icd icd;
    QList<Maemo::IcdStateResult> state_results;

    /* Update the active config from first connection, this is ok as icd
     * supports only one connection anyway.
     */
    if (icd.state(state_results) && !state_results.isEmpty()) {
	/* If we did not get full state back, then we are not
	 * connected and can skip the next part.
	 */
	if (!(state_results.first().params.network_attrs == 0 &&
		state_results.first().params.network_id.isEmpty())) {

	    /* If we try to connect to specific IAP and we get results back
	     * that tell the icd is actually connected to another IAP,
	     * then do not update current state etc.
	     */
	    if (publicConfig.type() == QNetworkConfiguration::UserChoice ||
            publicConfig.identifier() == state_results.first().params.network_id) {
		switch (state_results.first().state) {
		case ICD_STATE_DISCONNECTED:
            state = QNetworkSession::Disconnected;
            if (QNetworkConfigurationPrivatePointer ptr = privateConfiguration(activeConfig)) {
                ptr->mutex.lock();
                ptr->isValid = true;
                ptr->mutex.unlock();
            }
            break;
		case ICD_STATE_CONNECTING:
            state = QNetworkSession::Connecting;
            if (QNetworkConfigurationPrivatePointer ptr = privateConfiguration(activeConfig)) {
                ptr->mutex.lock();
                ptr->isValid = true;
                ptr->mutex.unlock();
            }
            break;
		case ICD_STATE_CONNECTED:
        {
            if (!state_results.first().error.isEmpty())
                break;

            const QString id = state_results.first().params.network_id;

            QNetworkConfiguration config = manager.configurationFromIdentifier(id);
            if (config.isValid()) {
                //we don't want the copied data if the config is already known by the manager
                //just reuse it so that existing references to the old data get the same update
                setPrivateConfiguration(activeConfig, privateConfiguration(config));
            }

            QNetworkConfigurationPrivatePointer ptr = privateConfiguration(activeConfig);

            QMutexLocker configLocker(&ptr->mutex);

			state = QNetworkSession::Connected;
            toIcdConfig(ptr)->network_id = state_results.first().params.network_id;
            ptr->id = toIcdConfig(ptr)->network_id;
            toIcdConfig(ptr)->network_attrs = state_results.first().params.network_attrs;
            toIcdConfig(ptr)->iap_type = state_results.first().params.network_type;
            ptr->bearerType = bearerTypeFromIapType(toIcdConfig(ptr)->iap_type);
            toIcdConfig(ptr)->service_type = state_results.first().params.service_type;
            toIcdConfig(ptr)->service_id = state_results.first().params.service_id;
            toIcdConfig(ptr)->service_attrs = state_results.first().params.service_attrs;
            ptr->type = QNetworkConfiguration::InternetAccessPoint;
            ptr->state = QNetworkConfiguration::Active;
            ptr->isValid = true;
            currentNetworkInterface = get_network_interface();

            Maemo::IAPConf iap_name(ptr->id);
			QString name_value = iap_name.value("name").toString();
			if (!name_value.isEmpty())
                ptr->name = name_value;
			else
                ptr->name = ptr->id;

            const QString identifier = ptr->id;

            configLocker.unlock();

            // Add the new active configuration to manager or update the old config
            if (!engine->hasIdentifier(identifier))
                engine->addSessionConfiguration(ptr);
            else
                engine->changedSessionConfiguration(ptr);
        }
        break;

		case ICD_STATE_DISCONNECTING:
            state = QNetworkSession::Closing;
            if (QNetworkConfigurationPrivatePointer ptr = privateConfiguration(activeConfig)) {
                ptr->mutex.lock();
                ptr->isValid = true;
                ptr->mutex.unlock();
            }
            break;
		default:
            break;
		}
    }
	} else {
#ifdef BEARER_MANAGEMENT_DEBUG
	    qDebug() << "status_req tells icd is not connected";
#endif
	}
    } else {
#ifdef BEARER_MANAGEMENT_DEBUG
	qDebug() << "status_req did not return any results from icd";
#endif
    }

    networkConfigurationsChanged();
}


void QNetworkSessionPrivateImpl::networkConfigurationsChanged()
{
    if (serviceConfig.isValid())
        updateStateFromServiceNetwork();
    else
        updateStateFromActiveConfig();
}


void QNetworkSessionPrivateImpl::updateStateFromServiceNetwork()
{
    QNetworkSession::State oldState = state;

    foreach (const QNetworkConfiguration &config, serviceConfig.children()) {
        if ((config.state() & QNetworkConfiguration::Active) != QNetworkConfiguration::Active)
            continue;

        if (activeConfig != config) {
            activeConfig = config;
            emit newConfigurationActivated();
        }

        state = QNetworkSession::Connected;
        if (state != oldState)
            emit stateChanged(state);

        return;
    }

    if (serviceConfig.children().isEmpty())
        state = QNetworkSession::NotAvailable;
    else
        state = QNetworkSession::Disconnected;

    if (state != oldState)
        emit stateChanged(state);
}


void QNetworkSessionPrivateImpl::clearConfiguration(QNetworkConfiguration &config)
{
    IcdNetworkConfigurationPrivate *icdConfig = toIcdConfig(privateConfiguration(config));

    QMutexLocker locker(&icdConfig->mutex);

    icdConfig->network_id.clear();
    icdConfig->iap_type.clear();
    icdConfig->network_attrs = 0;
    icdConfig->service_type.clear();
    icdConfig->service_id.clear();
    icdConfig->service_attrs = 0;
}


void QNetworkSessionPrivateImpl::updateStateFromActiveConfig()
{
    QNetworkSession::State oldState = state;

    bool newActive = false;

    if (!activeConfig.isValid())
        return;

    if (!activeConfig.isValid()) {
        state = QNetworkSession::Invalid;
        clearConfiguration(activeConfig);
    } else if ((activeConfig.state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        state = QNetworkSession::Connected;
        newActive = opened;
    } else if ((activeConfig.state() & QNetworkConfiguration::Discovered) == QNetworkConfiguration::Discovered) {
        state = QNetworkSession::Disconnected;
    } else if ((activeConfig.state() & QNetworkConfiguration::Defined) == QNetworkConfiguration::Defined) {
        state = QNetworkSession::NotAvailable;
    } else if ((activeConfig.state() & QNetworkConfiguration::Undefined) == QNetworkConfiguration::Undefined) {
        state = QNetworkSession::NotAvailable;
    }

    bool oldActive = isOpen;
    isOpen = newActive;

    if (!oldActive && isOpen)
        emit quitPendingWaitsForOpened();

    if (oldActive && !isOpen)
        emit closed();

    if (oldState != state) {
        emit stateChanged(state);

    if (state == QNetworkSession::Disconnected && oldActive) {
#ifdef BEARER_MANAGEMENT_DEBUG
	    //qDebug()<<"session aborted error emitted for"<<activeConfig.identifier();
#endif
	    lastError = QNetworkSession::SessionAbortedError;
        emit QNetworkSessionPrivate::error(lastError);
	}
    }

#ifdef BEARER_MANAGEMENT_DEBUG
    //qDebug()<<"oldState ="<<oldState<<" state ="<<state<<" oldActive ="<<oldActive<<" newActive ="<<newActive<<" opened ="<<opened;
#endif
}

static QString get_network_interface()
{
    Maemo::Icd icd;
    QList<Maemo::IcdAddressInfoResult> addr_results;
    uint ret;
    QString iface;

    ret = icd.addrinfo(addr_results);
    if (ret == 0) {
	/* No results */
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug() << "Cannot get addrinfo from icd, are you connected or is icd running?";
#endif
        return iface;
    }

    if (addr_results.first().ip_info.isEmpty())
        return QString();

    QByteArray data = addr_results.first().ip_info.first().address.toAscii();
    struct in_addr addr;
    if (inet_aton(data.constData(), &addr) == 0) {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug() << "address" << data.constData() << "invalid";
#endif
        return iface;
    }

    struct ifaddrs *ifaddr, *ifa;
    int family;

    if (getifaddrs(&ifaddr) == -1) {
#ifdef BEARER_MANAGEMENT_DEBUG
	qDebug() << "getifaddrs() failed";
#endif
	return iface;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr) {
            family = ifa->ifa_addr->sa_family;
            if (family != AF_INET) {
                continue; /* Currently only IPv4 is supported by icd dbus interface */
            }
            if (((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr == addr.s_addr) {
                iface = QString(ifa->ifa_name);
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return iface;
}


void QNetworkSessionPrivateImpl::open()
{
    if (m_stopTimer.isActive()) {
        m_stopTimer.stop();
    }
    if (!publicConfig.isValid()) {
        lastError = QNetworkSession::InvalidConfigurationError;
        emit QNetworkSessionPrivate::error(lastError);
        return;
    }
    if (serviceConfig.isValid()) {
        lastError = QNetworkSession::OperationNotSupportedError;
        emit QNetworkSessionPrivate::error(lastError);
    } else if (!opened) {
	if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
	    /* Caller is trying to connect to default IAP.
	     * At this time we will not know the IAP details so we just
	     * connect and update the active config when the IAP is
	     * connected.
	     */
	    opened = true;
            state = QNetworkSession::Connecting;
            emit stateChanged(state);
	    QTimer::singleShot(0, this, SLOT(do_open()));
	    return;
	}

	/* User is connecting to one specific IAP. If that IAP is not
	 * in discovered state we cannot continue.
	 */
        if ((activeConfig.state() & QNetworkConfiguration::Discovered) !=
            QNetworkConfiguration::Discovered) {
            lastError =QNetworkSession::InvalidConfigurationError;
            emit QNetworkSessionPrivate::error(lastError);
            return;
        }
        opened = true;

        if ((activeConfig.state() & QNetworkConfiguration::Active) != QNetworkConfiguration::Active) {
            state = QNetworkSession::Connecting;
            emit stateChanged(state);
            QTimer::singleShot(0, this, SLOT(do_open()));
            return;
        }
        isOpen = (activeConfig.state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active;
        if (isOpen)
            emit quitPendingWaitsForOpened();
    } else {
        /* We seem to be active so inform caller */
        emit quitPendingWaitsForOpened();
    }
}

void QNetworkSessionPrivateImpl::do_open()
{
    icd_connection_flags flags = connectFlags;
    QString iap = publicConfig.identifier();

    if (state == QNetworkSession::Connected) {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug() << "Already connected to" << activeConfig.identifier();
#endif
        emit stateChanged(QNetworkSession::Connected);
        emit quitPendingWaitsForOpened();
	return;
    }

    if (publicConfig.type() == QNetworkConfiguration::UserChoice)
        config = activeConfig;
    else
        config = publicConfig;

    if (iap == OSSO_IAP_ANY) {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug() << "connecting to default IAP" << iap;
#endif
        m_connectRequestTimer.start(ICD_LONG_CONNECT_TIMEOUT);
        m_dbusInterface->asyncCall(ICD_DBUS_API_CONNECT_REQ, (uint)flags); // Return value ignored
        m_asynchCallActive = true;
    } else {
        IcdNetworkConfigurationPrivate *icdConfig = toIcdConfig(privateConfiguration(config));

        icdConfig->mutex.lock();
        ICd2DetailsDBusStruct icd2;
        icd2.serviceType = icdConfig->service_type;
        icd2.serviceAttributes = icdConfig->service_attrs;
        icd2.setviceId = icdConfig->service_id;
        icd2.networkType = icdConfig->iap_type;
        icd2.networkAttributes = icdConfig->network_attrs;
        if (icdConfig->network_attrs & ICD_NW_ATTR_IAPNAME) {
            icd2.networkId  = QByteArray(iap.toLatin1());
        } else {
            icd2.networkId  = icdConfig->network_id;
        }
        icdConfig->mutex.unlock();

#ifdef BEARER_MANAGEMENT_DEBUG
    qDebug("connecting to %s/%s/0x%x/%s/0x%x/%s",
        icd2.networkId.data(),
        icd2.networkType.toAscii().constData(),
        icd2.networkAttributes,
        icd2.serviceType.toAscii().constData(),
        icd2.serviceAttributes,
        icd2.setviceId.toAscii().constData());
#endif

        ICd2DetailsList paramArray;
        paramArray.append(icd2);
        m_connectRequestTimer.start(ICD_LONG_CONNECT_TIMEOUT);
        m_dbusInterface->asyncCall(ICD_DBUS_API_CONNECT_REQ, (uint)flags, QVariant::fromValue(paramArray)); // Return value ignored
        m_asynchCallActive = true;
    }
}

void QNetworkSessionPrivateImpl::stateChange(const QDBusMessage& rep)
{
     if (m_asynchCallActive == true) {
        if (m_connectRequestTimer.isActive())
            m_connectRequestTimer.stop();
        m_asynchCallActive = false;

        QString result = rep.arguments().at(5).toString(); // network id or empty string
        QString connected_iap = result;
        if (connected_iap.isEmpty()) {
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug() << "connect to"<< publicConfig.identifier() << "failed, result is empty";
#endif
            updateState(QNetworkSession::Disconnected);
            emit QNetworkSessionPrivate::error(QNetworkSession::SessionAbortedError);
            if (publicConfig.type() == QNetworkConfiguration::UserChoice)
                    copyConfig(publicConfig, activeConfig);
            return;
        }

         /* If the user tried to connect to some specific connection (foo)
         * and we were already connected to some other connection (bar),
         * then we cannot activate this session although icd has a valid
         * connection to somewhere.
         */
        if ((publicConfig.type() != QNetworkConfiguration::UserChoice) &&
            (connected_iap != config.identifier())) {
            updateState(QNetworkSession::Disconnected);
            emit QNetworkSessionPrivate::error(QNetworkSession::UnknownSessionError);
            return;
        }

        IcdNetworkConfigurationPrivate *icdConfig = toIcdConfig(privateConfiguration(config));

        /* Did we connect to non saved IAP? */
        icdConfig->mutex.lock();
        if (!(icdConfig->network_attrs & ICD_NW_ATTR_IAPNAME)) {
            /* Because the connection succeeded, the IAP is now known.
             */
            icdConfig->network_attrs |= ICD_NW_ATTR_IAPNAME;
            icdConfig->id = connected_iap;
        }

        /* User might have changed the IAP name when a new IAP was saved */
        Maemo::IAPConf iap_name(icdConfig->id);
        QString name = iap_name.value("name").toString();
        if (!name.isEmpty())
            icdConfig->name = name;

        icdConfig->iap_type = rep.arguments().at(3).toString(); // connect_result.connect.network_type;
        icdConfig->bearerType = bearerTypeFromIapType(icdConfig->iap_type);
        icdConfig->isValid = true;
        icdConfig->state = QNetworkConfiguration::Active;
        icdConfig->type = QNetworkConfiguration::InternetAccessPoint;

        icdConfig->mutex.unlock();

        startTime = QDateTime::currentDateTime();
        updateState(QNetworkSession::Connected);
        //currentNetworkInterface = get_network_interface();
#ifdef BEARER_MANAGEMENT_DEBUG
        //qDebug() << "connected to" << result << config.name() << "at" << currentNetworkInterface;
#endif

        /* We first check if the configuration already exists in the manager
         * and if it is not found there, we then insert it. Note that this
         * is only done for user choice config only because it can be missing
         * from config manager list.
         */
        if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
            if (!engine->hasIdentifier(result)) {
                engine->addSessionConfiguration(privateConfiguration(config));
            } else {
                QNetworkConfigurationPrivatePointer priv = engine->configuration(result);
                QNetworkConfiguration reference;
                setPrivateConfiguration(reference, priv);
                copyConfig(config, reference, false);
                privateConfiguration(reference)->id = result; // Note: Id was not copied in copyConfig() function
                config = reference;
                activeConfig = reference;
                engine->changedSessionConfiguration(privateConfiguration(config));

#ifdef BEARER_MANAGEMENT_DEBUG
                qDebug()<<"Existing configuration"<<result<<"updated in manager in open";
#endif
            }
        }

        emit quitPendingWaitsForOpened();
    }
}

void QNetworkSessionPrivateImpl::connectTimeout()
{
    updateState(QNetworkSession::Disconnected);
    if (publicConfig.type() == QNetworkConfiguration::UserChoice)
            copyConfig(publicConfig, activeConfig);
        emit QNetworkSessionPrivate::error(QNetworkSession::UnknownSessionError);
}

void QNetworkSessionPrivateImpl::close()
{
    if (m_connectRequestTimer.isActive())
        m_connectRequestTimer.stop();

    if (serviceConfig.isValid()) {
        lastError = QNetworkSession::OperationNotSupportedError;
        emit QNetworkSessionPrivate::error(lastError);
    } else if (isOpen) {
        if ((activeConfig.state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
	    // We will not wait any disconnect from icd as it might never come
#ifdef BEARER_MANAGEMENT_DEBUG
	    qDebug() << "closing session" << publicConfig.identifier();
#endif
	    state = QNetworkSession::Closing;
	    emit stateChanged(state);

	    // we fake a disconnection, session error is sent
	    updateState(QNetworkSession::Disconnected);

	    opened = false;
	    isOpen = false;

        m_dbusInterface->call(ICD_DBUS_API_DISCONNECT_REQ, ICD_CONNECTION_FLAG_APPLICATION_EVENT);
	    startTime = QDateTime();
        } else {
	    opened = false;
	    isOpen = false;
	    emit closed();
	}
    }
}


void QNetworkSessionPrivateImpl::stop()
{
    if (m_connectRequestTimer.isActive())
        m_connectRequestTimer.stop();

    if (serviceConfig.isValid()) {
        lastError = QNetworkSession::OperationNotSupportedError;
        emit QNetworkSessionPrivate::error(lastError);
    } else {
        if ((activeConfig.state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
#ifdef BEARER_MANAGEMENT_DEBUG
	    qDebug() << "stopping session" << publicConfig.identifier();
#endif
	    state = QNetworkSession::Closing;
	    emit stateChanged(state);

	    // we fake a disconnection, a session error is sent also
	    updateState(QNetworkSession::Disconnected);

	    opened = false;
	    isOpen = false;

        m_dbusInterface->call(ICD_DBUS_API_DISCONNECT_REQ, ICD_CONNECTION_FLAG_APPLICATION_EVENT);
	    startTime = QDateTime();
        } else {
	    opened = false;
	    isOpen = false;
	    emit closed();
	}
    }
}

void QNetworkSessionPrivateImpl::finishStopBySendingClosedSignal()
{
    if ((activeConfig.state() & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        state = QNetworkSession::Connected;
        emit stateChanged(state);
    }

    emit closed();
}

void QNetworkSessionPrivateImpl::migrate()
{
}


void QNetworkSessionPrivateImpl::accept()
{
}


void QNetworkSessionPrivateImpl::ignore()
{
}


void QNetworkSessionPrivateImpl::reject()
{
}

#ifndef QT_NO_NETWORKINTERFACE
QNetworkInterface QNetworkSessionPrivateImpl::currentInterface() const
{
    if (!publicConfig.isValid() || state != QNetworkSession::Connected)
        return QNetworkInterface();

    if (currentNetworkInterface.isEmpty())
        return QNetworkInterface();

    return QNetworkInterface::interfaceFromName(currentNetworkInterface);
}
#endif

void QNetworkSessionPrivateImpl::setSessionProperty(const QString& key, const QVariant& value)
{
    if (value.isValid()) {
	properties.insert(key, value);

	if (key == "ConnectInBackground") {
	    bool v = value.toBool();
	    if (v)
		connectFlags = ICD_CONNECTION_FLAG_APPLICATION_EVENT;
	    else
		connectFlags = ICD_CONNECTION_FLAG_USER_EVENT;
	}
    } else {
	properties.remove(key);

	/* Set default value when property is removed */
	if (key == "ConnectInBackground")
	    connectFlags = ICD_CONNECTION_FLAG_USER_EVENT;
    }
}


QVariant QNetworkSessionPrivateImpl::sessionProperty(const QString& key) const
{
    return properties.value(key);
}


QString QNetworkSessionPrivateImpl::errorString() const
{
    QString errorStr;
    switch(q->error()) {
    case QNetworkSession::RoamingError:
        errorStr = QNetworkSessionPrivateImpl::tr("Roaming error");
        break;
    case QNetworkSession::SessionAbortedError:
        errorStr = QNetworkSessionPrivateImpl::tr("Session aborted by user or system");
        break;
    case QNetworkSession::InvalidConfigurationError:
        errorStr = QNetworkSessionPrivateImpl::tr("The specified configuration cannot be used.");
        break;
    default:
    case QNetworkSession::UnknownSessionError:
        errorStr = QNetworkSessionPrivateImpl::tr("Unidentified Error");
        break;
    }
    return errorStr;
}


QNetworkSession::SessionError QNetworkSessionPrivateImpl::error() const
{
    return QNetworkSession::UnknownSessionError;
}

void QNetworkSessionPrivateImpl::updateProxies(QNetworkSession::State newState)
{
    if ((newState == QNetworkSession::Connected) &&
	(newState != currentState))
	updateProxyInformation();
    else if ((newState == QNetworkSession::Disconnected) &&
	    (currentState == QNetworkSession::Closing))
	clearProxyInformation();

    currentState = newState;
}


void QNetworkSessionPrivateImpl::updateProxyInformation()
{
    Maemo::ProxyConf::update();
}


void QNetworkSessionPrivateImpl::clearProxyInformation()
{
    Maemo::ProxyConf::clear();
}

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT
