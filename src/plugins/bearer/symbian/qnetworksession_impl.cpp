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
#include "symbianengine.h"

#include <es_enum.h>
#include <es_sock.h>
#include <in_sock.h>
#include <private/qcore_symbian_p.h>

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
#include <cmmanager.h>
#endif

#if defined(OCC_FUNCTIONALITY_AVAILABLE) && defined(SNAP_FUNCTIONALITY_AVAILABLE)
#include <extendedconnpref.h>
#endif

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

QNetworkSessionPrivateImpl::QNetworkSessionPrivateImpl(SymbianEngine *engine)
:   engine(engine), iSocketServ(qt_symbianGetSocketServer()),
    ipConnectionNotifier(0), ipConnectionStarter(0),
    iHandleStateNotificationsFromManager(false), iFirstSync(true), iStoppedByUser(false),
    iClosedByUser(false), iError(QNetworkSession::UnknownSessionError), iALREnabled(0),
    iConnectInBackground(false), isOpening(false)
{

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    iMobility = NULL;
#endif

    TRAP_IGNORE(iConnectionMonitor.ConnectL());
}

void QNetworkSessionPrivateImpl::closeHandles()
{
    QMutexLocker lock(&mutex);
    // Cancel Connection Progress Notifications first.
    // Note: ConnectionNotifier must be destroyed before RConnection::Close()
    //       => deleting ipConnectionNotifier results RConnection::CancelProgressNotification()
    delete ipConnectionNotifier;
    ipConnectionNotifier = NULL;

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    // mobility monitor must be deleted before RConnection is closed
    delete iMobility;
    iMobility = NULL;
#endif

    // Cancel possible RConnection::Start() - may call RConnection::Close if Start was in progress
    delete ipConnectionStarter;
    ipConnectionStarter = 0;
    //close any open connection (note Close twice is safe in case Cancel did it above)
    iConnection.Close();

    QSymbianSocketManager::instance().setDefaultConnection(0);

    iConnectionMonitor.Close();
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this)
             << " - handles closed";
#endif

}

QNetworkSessionPrivateImpl::~QNetworkSessionPrivateImpl()
{
    isOpen = false;
    isOpening = false;

    closeHandles();
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this)
             << " - destroyed";
#endif
}

void QNetworkSessionPrivateImpl::configurationStateChanged(quint32 accessPointId, quint32 connMonId, QNetworkSession::State newState)
{
    if (iHandleStateNotificationsFromManager) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
        qDebug() << "QNS this : " << QString::number((uint)this) << " - "
                 << "configurationStateChanged from manager for IAP : " << QString::number(accessPointId)
                 << "connMon ID : " << QString::number(connMonId) << " : to a state: " << newState
                 << "whereas my current state is: " << state;
#else
      Q_UNUSED(connMonId);
#endif
        this->newState(newState, accessPointId);
    }
}

void QNetworkSessionPrivateImpl::configurationRemoved(QNetworkConfigurationPrivatePointer config)
{
    if (!publicConfig.isValid())
        return;

    TUint32 publicNumericId =
        toSymbianConfig(privateConfiguration(publicConfig))->numericIdentifier();

    if (toSymbianConfig(config)->numericIdentifier() == publicNumericId) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
        qDebug() << "QNS this : " << QString::number((uint)this) << " - "
                 << "configurationRemoved IAP: " << QString::number(publicNumericId) << " : going to State: Invalid";
#endif
        this->newState(QNetworkSession::Invalid, publicNumericId);
    }
}

void QNetworkSessionPrivateImpl::configurationAdded(QNetworkConfigurationPrivatePointer config)
{
    Q_UNUSED(config);
    // If session is based on service network, some other app may create new access points
    // to the SNAP --> synchronize session's state with that of interface's.
    if (!publicConfig.isValid() || publicConfig.type() != QNetworkConfiguration::ServiceNetwork)
        return;

#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
        qDebug() << "QNS this : " << QString::number((uint)this) << " - "
                 << "configurationAdded IAP: "
                 << toSymbianConfig(config)->numericIdentifier();
#endif

        syncStateWithInterface();
}

// Function sets the state of the session to match the state
// of the underlying interface (the configuration this session is based on)
void QNetworkSessionPrivateImpl::syncStateWithInterface()
{
    if (!publicConfig.isValid())
        return;

    if (iFirstSync) {
        QObject::connect(engine,
                         SIGNAL(configurationStateChanged(quint32,quint32,QNetworkSession::State)),
                         this,
                         SLOT(configurationStateChanged(quint32,quint32,QNetworkSession::State)));
        // Listen to configuration removals, so that in case the configuration
        // this session is based on is removed, session knows to enter Invalid -state.
        QObject::connect(engine, SIGNAL(configurationRemoved(QNetworkConfigurationPrivatePointer)),
                         this, SLOT(configurationRemoved(QNetworkConfigurationPrivatePointer)));
        // Connect to configuration additions, so that in case a configuration is added
        // in a SNAP this session is based on, the session knows to synch its state with its
        // interface.
        QObject::connect(engine, SIGNAL(configurationAdded(QNetworkConfigurationPrivatePointer)),
                         this, SLOT(configurationAdded(QNetworkConfigurationPrivatePointer)));
    }
    // Start listening IAP state changes from QNetworkConfigurationManagerPrivate
    iHandleStateNotificationsFromManager = true;    

    // Check what is the state of the configuration this session is based on
    // and set the session in appropriate state.
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this) << " - "
             << "syncStateWithInterface() state of publicConfig is: " << publicConfig.state();
#endif
    switch (publicConfig.state()) {
    case QNetworkConfiguration::Active:
        newState(QNetworkSession::Connected);
        break;
    case QNetworkConfiguration::Discovered:
        newState(QNetworkSession::Disconnected);
        break;
    case QNetworkConfiguration::Defined:
        newState(QNetworkSession::NotAvailable);
        break;
    case QNetworkConfiguration::Undefined:
    default:
        newState(QNetworkSession::Invalid);
    }
}

#ifndef QT_NO_NETWORKINTERFACE
QNetworkInterface QNetworkSessionPrivateImpl::interface(TUint iapId) const
{
    QString interfaceName;

    TSoInetInterfaceInfo ifinfo;
    TPckg<TSoInetInterfaceInfo> ifinfopkg(ifinfo);
    TSoInetIfQuery ifquery;
    TPckg<TSoInetIfQuery> ifquerypkg(ifquery);
 
    // Open dummy socket for interface queries
    RSocket socket;
    TInt retVal = socket.Open(iSocketServ, _L("udp"));
    if (retVal != KErrNone) {
        return QNetworkInterface();
    }
 
    // Start enumerating interfaces
    socket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
    while(socket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, ifinfopkg) == KErrNone) {
        ifquery.iName = ifinfo.iName;
        TInt err = socket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, ifquerypkg);
        if(err == KErrNone && ifquery.iZone[1] == iapId) { // IAP ID is index 1 of iZone
            if(ifinfo.iAddress.Address() > 0) {
                interfaceName = QString::fromUtf16(ifinfo.iName.Ptr(),ifinfo.iName.Length());
                break;
            }
        }
    }
 
    socket.Close();
 
    if (interfaceName.isEmpty()) {
        return QNetworkInterface();
    }
 
    return QNetworkInterface::interfaceFromName(interfaceName);
}
#endif

#ifndef QT_NO_NETWORKINTERFACE
QNetworkInterface QNetworkSessionPrivateImpl::currentInterface() const
{
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this) << " - "
             << "currentInterface() requested, state: " << state
             << "publicConfig validity: " << publicConfig.isValid();
    if (activeInterface.isValid())
        qDebug() << "QNS this : " << QString::number((uint)this) << " - "
                 << "interface is: " << activeInterface.humanReadableName();
#endif

    if (!publicConfig.isValid() || state != QNetworkSession::Connected) {
        return QNetworkInterface();
    }
    
    return activeInterface;
}
#endif

QVariant QNetworkSessionPrivateImpl::sessionProperty(const QString& key) const
{
    if (key == "ConnectInBackground") {
        return QVariant(iConnectInBackground);
    }
    return QVariant();
}

void QNetworkSessionPrivateImpl::setSessionProperty(const QString& key, const QVariant& value)
{
    // Valid value means adding property, invalid means removing it.
    if (key == "ConnectInBackground") {
        if (value.isValid()) {
            iConnectInBackground = value.toBool();
        } else {
            iConnectInBackground = EFalse;
        }
    }
}

QString QNetworkSessionPrivateImpl::errorString() const
{
    switch (iError) {
    case QNetworkSession::UnknownSessionError:
        return tr("Unknown session error.");
    case QNetworkSession::SessionAbortedError:
        return tr("The session was aborted by the user or system.");
    case QNetworkSession::OperationNotSupportedError:
        return tr("The requested operation is not supported by the system.");
    case QNetworkSession::InvalidConfigurationError:
        return tr("The specified configuration cannot be used.");
    case QNetworkSession::RoamingError:
        return tr("Roaming was aborted or is not possible.");
    }
 
    return QString();
}

QNetworkSession::SessionError QNetworkSessionPrivateImpl::error() const
{
    return iError;
}

void QNetworkSessionPrivateImpl::open()
{
    QMutexLocker lock(&mutex);
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
        qDebug() << "QNS this : " << QString::number((uint)this) << " - "
                << "open() called, session state is: " << state << " and isOpen is: "
                << isOpen << isOpening;
#endif

    if (isOpen || isOpening)
        return;

    isOpening = true;

    // Stop handling IAP state change signals from QNetworkConfigurationManagerPrivate
    // => RConnection::ProgressNotification will be used for IAP/SNAP monitoring
    iHandleStateNotificationsFromManager = false;

    // Configuration may have been invalidated after session creation by platform
    // (e.g. configuration has been deleted).
    if (!publicConfig.isValid()) {
        newState(QNetworkSession::Invalid);
        iError = QNetworkSession::InvalidConfigurationError;
        emit QNetworkSessionPrivate::error(iError);
        return;
    }
    // If opening a undefined configuration, session emits error and enters
    // NotAvailable -state. Note that we will try ones in 'defined' state to avoid excessive
    // need for WLAN scans (via updateConfigurations()), because user may have walked
    // into a WLAN range, but periodic background scan has not occurred yet -->
    // we don't want to force application to make frequent updateConfigurations() calls
    // to be able to try if e.g. home WLAN is available.
    if (publicConfig.state() == QNetworkConfiguration::Undefined) {
        newState(QNetworkSession::NotAvailable);
        iError = QNetworkSession::InvalidConfigurationError;
        emit QNetworkSessionPrivate::error(iError);
        return;
    }
    // Clear possible previous states
    iStoppedByUser = false;
    iClosedByUser = false;

    Q_ASSERT(!iConnection.SubSessionHandle());
    TInt error = iConnection.Open(iSocketServ);
    if (error != KErrNone) {
        // Could not open RConnection
        newState(QNetworkSession::Invalid);
        iError = QNetworkSession::UnknownSessionError;
        emit QNetworkSessionPrivate::error(iError);
        syncStateWithInterface();    
        return;
    }
    
    // Use RConnection::ProgressNotification for IAP/SNAP monitoring
    // (<=> ConnectionProgressNotifier uses RConnection::ProgressNotification)
    if (!ipConnectionNotifier) {
        ipConnectionNotifier = new ConnectionProgressNotifier(*this,iConnection);
    }
    if (ipConnectionNotifier) {
        ipConnectionNotifier->StartNotifications();
    }
    
    if (publicConfig.type() == QNetworkConfiguration::InternetAccessPoint) {
            SymbianNetworkConfigurationPrivate *symbianConfig =
                toSymbianConfig(privateConfiguration(publicConfig));

#if defined(OCC_FUNCTIONALITY_AVAILABLE) && defined(SNAP_FUNCTIONALITY_AVAILABLE)
            // With One Click Connectivity (Symbian^3 onwards) it is possible
            // to connect silently, without any popups.
            TConnPrefList pref;
            TExtendedConnPref prefs;

            prefs.SetIapId(symbianConfig->numericIdentifier());
            if (iConnectInBackground) {
                prefs.SetNoteBehaviour( TExtendedConnPref::ENoteBehaviourConnSilent );
            }
            pref.AppendL(&prefs);
#else
            TCommDbConnPref pref;
            pref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

            pref.SetIapId(symbianConfig->numericIdentifier());
#endif
            if (!ipConnectionStarter) {
                ipConnectionStarter = new ConnectionStarter(*this, iConnection);
                ipConnectionStarter->Start(pref);
            }
            // Avoid flip flop of states if the configuration is already
            // active. IsOpen/opened() will indicate when ready.
            if (state != QNetworkSession::Connected) {
                newState(QNetworkSession::Connecting);
            }
    } else if (publicConfig.type() == QNetworkConfiguration::ServiceNetwork) {
        SymbianNetworkConfigurationPrivate *symbianConfig =
            toSymbianConfig(privateConfiguration(publicConfig));

#if defined(OCC_FUNCTIONALITY_AVAILABLE) && defined(SNAP_FUNCTIONALITY_AVAILABLE)
        // On Symbian^3 if service network is not reachable, it triggers a UI (aka EasyWLAN) where
        // user can create new IAPs. To detect this, we need to store the number of IAPs
        // there was before connection was started.
        iKnownConfigsBeforeConnectionStart = engine->accessPointConfigurationIdentifiers();
        TConnPrefList snapPref;
        TExtendedConnPref prefs;
        prefs.SetSnapId(symbianConfig->numericIdentifier());
        if (iConnectInBackground) {
            prefs.SetNoteBehaviour( TExtendedConnPref::ENoteBehaviourConnSilent );
        }
        snapPref.AppendL(&prefs);
#else
        TConnSnapPref snapPref(symbianConfig->numericIdentifier());
#endif
        if (!ipConnectionStarter) {
            ipConnectionStarter = new ConnectionStarter(*this, iConnection);
            ipConnectionStarter->Start(snapPref);
        }
        // Avoid flip flop of states if the configuration is already
        // active. IsOpen/opened() will indicate when ready.
        if (state != QNetworkSession::Connected) {
            newState(QNetworkSession::Connecting);
        }
    } else if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
        iKnownConfigsBeforeConnectionStart = engine->accessPointConfigurationIdentifiers();
        if (!ipConnectionStarter) {
            ipConnectionStarter = new ConnectionStarter(*this, iConnection);
            ipConnectionStarter->Start();
        }
        newState(QNetworkSession::Connecting);
    }
 
    if (error != KErrNone) {
        isOpen = false;
        isOpening = false;
        iError = QNetworkSession::UnknownSessionError;
        emit QNetworkSessionPrivate::error(iError);
        closeHandles();
        syncStateWithInterface();    
    }
}

TUint QNetworkSessionPrivateImpl::iapClientCount(TUint aIAPId) const
{
    TRequestStatus status;
    TUint connectionCount;
    iConnectionMonitor.GetConnectionCount(connectionCount, status);
    User::WaitForRequest(status);
    if (status.Int() == KErrNone) {
        for (TUint i = 1; i <= connectionCount; i++) {
            TUint connectionId;
            TUint subConnectionCount;
            iConnectionMonitor.GetConnectionInfo(i, connectionId, subConnectionCount);
            TUint apId;
            iConnectionMonitor.GetUintAttribute(connectionId, subConnectionCount, KIAPId, apId, status);
            User::WaitForRequest(status);
            if (apId == aIAPId) {
                TConnMonClientEnumBuf buf;
                iConnectionMonitor.GetPckgAttribute(connectionId, 0, KClientInfo, buf, status);
                User::WaitForRequest(status);
                if (status.Int() == KErrNone) {
                    return buf().iCount;
                }
            }
        }
    }
    return 0;
}

void QNetworkSessionPrivateImpl::close(bool allowSignals)
{
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this) << " - "
            << "close() called, session state is: " << state << " and isOpen is : "
            << isOpen;
#endif

    if (!isOpen && state != QNetworkSession::Connecting) {
        return;
    }
    // Mark this session as closed-by-user so that we are able to report
    // distinguish between stop() and close() state transitions
    // when reporting.
    iClosedByUser = true;
    isOpen = false;
    isOpening = false;

    serviceConfig = QNetworkConfiguration();
    
    closeHandles();

    // Start handling IAP state change signals from QNetworkConfigurationManagerPrivate
    iHandleStateNotificationsFromManager = true;

    // If UserChoice, go down immediately. If some other configuration,
    // go down immediately if there is no reports expected from the platform;
    // in practice Connection Monitor is aware of connections only after
    // KFinishedSelection event, and hence reports only after that event, but
    // that does not seem to be trusted on all Symbian versions --> safest
    // to go down.
    if (publicConfig.type() == QNetworkConfiguration::UserChoice || state == QNetworkSession::Connecting) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
        qDebug() << "QNS this : " << QString::number((uint)this) << " - "
                 << "going disconnected right away, since either UserChoice or Connecting";
#endif
        newState(QNetworkSession::Closing);
        newState(QNetworkSession::Disconnected);
    }
    if (allowSignals) {
        emit closed();
    }
}

void QNetworkSessionPrivateImpl::stop()
{
    QMutexLocker lock(&mutex);
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this) << " - "
            << "stop() called, session state is: " << state << " and isOpen is : "
            << isOpen;
#endif
    if (!isOpen &&
        publicConfig.isValid() &&
        publicConfig.type() == QNetworkConfiguration::InternetAccessPoint) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this) << " - "
            << "since session is not open, using RConnectionMonitor to stop() the interface";
#endif
        iStoppedByUser = true;
        // If the publicConfig is type of IAP, enumerate through connections at
        // connection monitor. If publicConfig is active in that list, stop it.
        // Otherwise there is nothing to stop. Note: because this QNetworkSession is not open,
        // activeConfig is not usable.
        TUint count;
        TRequestStatus status;
        iConnectionMonitor.GetConnectionCount(count, status);
        User::WaitForRequest(status);
        if (status.Int() != KErrNone) {
            return;
        }
        TUint numSubConnections; // Not used but needed by GetConnectionInfo i/f
        TUint connectionId;
        for (TUint i = 1; i <= count; ++i) {
            // Get (connection monitor's assigned) connection ID
            TInt ret = iConnectionMonitor.GetConnectionInfo(i, connectionId, numSubConnections);            
            if (ret == KErrNone) {
                SymbianNetworkConfigurationPrivate *symbianConfig =
                    toSymbianConfig(privateConfiguration(publicConfig));

                // See if connection Id matches with our Id. If so, stop() it.
                if (symbianConfig->connectionIdentifier() == connectionId) {
                    ret = iConnectionMonitor.SetBoolAttribute(connectionId,
                                                              0, // subConnectionId don't care
                                                              KConnectionStop,
                                                              ETrue);
                }
            }
            // Enter disconnected state right away since the session is not even open.
            // Symbian^3 connection monitor does not emit KLinkLayerClosed when
            // connection is stopped via connection monitor.
            newState(QNetworkSession::Disconnected);
        }
    } else if (isOpen) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this) << " - "
            << "since session is open, using RConnection to stop() the interface";
#endif
        // Since we are open, use RConnection to stop the interface
        isOpen = false;
        isOpening = false;
        iStoppedByUser = true;
        newState(QNetworkSession::Closing);
        if (ipConnectionNotifier) {
            ipConnectionNotifier->StopNotifications();
            // Start handling IAP state change signals from QNetworkConfigurationManagerPrivate
            iHandleStateNotificationsFromManager = true;
        }
        iConnection.Stop(RConnection::EStopAuthoritative);
        isOpen = true;
        isOpening = false;
        close(false);
        emit closed();
    }
}

void QNetworkSessionPrivateImpl::migrate()
{
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    if (iMobility) {
        QSymbianSocketManager::instance().setDefaultConnection(0);
        // Start migrating to new IAP
        iMobility->MigrateToPreferredCarrier();
    }
#endif
}

void QNetworkSessionPrivateImpl::ignore()
{
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    if (iMobility) {
        iMobility->IgnorePreferredCarrier();

        if (!iALRUpgradingConnection) {
            newState(QNetworkSession::Disconnected);
        } else {
            newState(QNetworkSession::Connected,iOldRoamingIap);
        }
    }
#endif
}

void QNetworkSessionPrivateImpl::accept()
{
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    if (iMobility) {
        iMobility->NewCarrierAccepted();

        QNetworkConfiguration newActiveConfig = activeConfiguration(iNewRoamingIap);

        QSymbianSocketManager::instance().setDefaultConnection(&iConnection);

        newState(QNetworkSession::Connected, iNewRoamingIap);
    }
#endif
}

void QNetworkSessionPrivateImpl::reject()
{
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    if (iMobility) {
        iMobility->NewCarrierRejected();

        if (!iALRUpgradingConnection) {
            newState(QNetworkSession::Disconnected);
        } else {
            QNetworkConfiguration newActiveConfig = activeConfiguration(iOldRoamingIap);

            QSymbianSocketManager::instance().setDefaultConnection(&iConnection);

            newState(QNetworkSession::Connected, iOldRoamingIap);
        }
    }
#endif
}

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
void QNetworkSessionPrivateImpl::PreferredCarrierAvailable(TAccessPointInfo aOldAPInfo,
                                                       TAccessPointInfo aNewAPInfo,
                                                       TBool aIsUpgrade,
                                                       TBool aIsSeamless)
{
    iOldRoamingIap = aOldAPInfo.AccessPoint();
    iNewRoamingIap = aNewAPInfo.AccessPoint();
    newState(QNetworkSession::Roaming);
    if (iALREnabled > 0) {
        iALRUpgradingConnection = aIsUpgrade;
        QList<QNetworkConfiguration> configs = publicConfig.children();
        for (int i=0; i < configs.count(); i++) {
            SymbianNetworkConfigurationPrivate *symbianConfig =
                toSymbianConfig(privateConfiguration(configs[i]));

            if (symbianConfig->numericIdentifier() == aNewAPInfo.AccessPoint()) {
                // Any slot connected to the signal might throw an std::exception,
                // which must not propagate into Symbian code (this function is a callback
                // from platform). We could convert exception to a symbian Leave, but since the
                // prototype of this function bans this (no trailing 'L'), we just catch
                // and drop.
                QT_TRY {
                    emit preferredConfigurationChanged(configs[i], aIsSeamless);
                }
                QT_CATCH (std::exception&) {}
            }
        }
    } else {
        migrate();
    }
}

void QNetworkSessionPrivateImpl::NewCarrierActive(TAccessPointInfo /*aNewAPInfo*/, TBool /*aIsSeamless*/)
{
    if (iALREnabled > 0) {
        QT_TRY {
            emit newConfigurationActivated();
        }
        QT_CATCH (std::exception&) {}
    } else {
        accept();
    }
}

void QNetworkSessionPrivateImpl::Error(TInt aError)
{
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this) << " - "
            << "roaming Error() occurred" << aError << ", isOpen is: " << isOpen;
#endif
    if (aError == KErrCancel)
        return; //avoid recursive deletion
    if (isOpen) {
        isOpen = false;
        isOpening = false;
        activeConfig = QNetworkConfiguration();
        serviceConfig = QNetworkConfiguration();
        iError = QNetworkSession::RoamingError;
        emit QNetworkSessionPrivate::error(iError);
        closeHandles();
        QT_TRY {
            syncStateWithInterface();
            // In some cases IAP is still in Connected state when
            // syncStateWithInterface(); is called
            // => Following call makes sure that Session state
            //    changes immediately to Disconnected.
            newState(QNetworkSession::Disconnected);
            emit closed();
        }
        QT_CATCH (std::exception&) {}
    } else if (iStoppedByUser) {
        // If the user of this session has called the stop() and
        // configuration is based on internet SNAP, this needs to be
        // done here because platform might roam.
        QT_TRY {
            newState(QNetworkSession::Disconnected);
        }
        QT_CATCH (std::exception&) {}
    }
}
#endif

void QNetworkSessionPrivateImpl::setALREnabled(bool enabled)
{
    if (enabled) {
        iALREnabled++;
    } else {
        iALREnabled--;
    }
}

QNetworkConfiguration QNetworkSessionPrivateImpl::bestConfigFromSNAP(const QNetworkConfiguration& snapConfig) const
{
    QNetworkConfiguration config;
    QList<QNetworkConfiguration> subConfigurations = snapConfig.children();
    for (int i = 0; i < subConfigurations.count(); i++ ) {
        if (subConfigurations[i].state() == QNetworkConfiguration::Active) {
            config = subConfigurations[i];
            break;
        } else if (!config.isValid() && subConfigurations[i].state() == QNetworkConfiguration::Discovered) {
            config = subConfigurations[i];
        }
    }
    if (!config.isValid() && subConfigurations.count() > 0) {
        config = subConfigurations[0];
    }
    return config;
}

quint64 QNetworkSessionPrivateImpl::bytesWritten() const
{
    return transferredData(KUplinkData);
}

quint64 QNetworkSessionPrivateImpl::bytesReceived() const
{
    return transferredData(KDownlinkData);
}

quint64 QNetworkSessionPrivateImpl::transferredData(TUint dataType) const
{
    if (!publicConfig.isValid()) {
        return 0;
    }
    
    QNetworkConfiguration config;
    if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
        if (serviceConfig.isValid()) {
            config = serviceConfig;
        } else {
            if (activeConfig.isValid()) {
                config = activeConfig;
            }
        }
    } else {
        config = publicConfig;
    }

    if (!config.isValid()) {
        return 0;
    }
    
    TUint count;
    TRequestStatus status;
    iConnectionMonitor.GetConnectionCount(count, status);
    User::WaitForRequest(status);
    if (status.Int() != KErrNone) {
        return 0;
    }
    
    TUint transferredData = 0;
    TUint numSubConnections;
    TUint connectionId;
    bool configFound;
    for (TUint i = 1; i <= count; i++) {
        TInt ret = iConnectionMonitor.GetConnectionInfo(i, connectionId, numSubConnections);
        if (ret == KErrNone) {
            TUint apId;
            iConnectionMonitor.GetUintAttribute(connectionId, 0, KIAPId, apId, status);
            User::WaitForRequest(status);
            if (status.Int() == KErrNone) {
                configFound = false;
                if (config.type() == QNetworkConfiguration::ServiceNetwork) {
                    QList<QNetworkConfiguration> configs = config.children();
                    for (int i=0; i < configs.count(); i++) {
                        SymbianNetworkConfigurationPrivate *symbianConfig =
                            toSymbianConfig(privateConfiguration(configs[i]));

                        if (symbianConfig->numericIdentifier() == apId) {
                            configFound = true;
                            break;
                        }
                    }
                } else {
                    SymbianNetworkConfigurationPrivate *symbianConfig =
                        toSymbianConfig(privateConfiguration(config));

                    if (symbianConfig->numericIdentifier() == apId)
                        configFound = true;
                }
                if (configFound) {
                    TUint tData;
                    iConnectionMonitor.GetUintAttribute(connectionId, 0, dataType, tData, status );
                    User::WaitForRequest(status);
                    if (status.Int() == KErrNone) {
                    transferredData += tData;
                    }
                }
            }
        }
    }
    
    return transferredData;
}

quint64 QNetworkSessionPrivateImpl::activeTime() const
{
    if (!isOpen || startTime.isNull()) {
        return 0;
    }
    return startTime.secsTo(QDateTime::currentDateTime());
}

QNetworkConfiguration QNetworkSessionPrivateImpl::activeConfiguration(TUint32 iapId) const
{
    if (iapId == 0) {
        _LIT(KSetting, "IAP\\Id");
        iConnection.GetIntSetting(KSetting, iapId);
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
        // Check if this is an Easy WLAN configuration. On Symbian^3 RConnection may report
        // the used configuration as 'EasyWLAN' IAP ID if someone has just opened the configuration
        // from WLAN Scan dialog, _and_ that connection is still up. We need to find the
        // real matching configuration. Function alters the Easy WLAN ID to real IAP ID (only if
        // easy WLAN):
        easyWlanTrueIapId(iapId);
#endif
    }

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    if (publicConfig.type() == QNetworkConfiguration::ServiceNetwork) {
        // Try to search IAP from the used SNAP using IAP Id
        QList<QNetworkConfiguration> children = publicConfig.children();
        for (int i=0; i < children.count(); i++) {
            SymbianNetworkConfigurationPrivate *childConfig =
                toSymbianConfig(privateConfiguration(children[i]));

            if (childConfig->numericIdentifier() == iapId)
                return children[i];
        }

        // Given IAP Id was not found from the used SNAP
        // => Try to search matching IAP using mappingName
        //    mappingName contains:
        //      1. "Access point name" for "Packet data" Bearer
        //      2. "WLAN network name" (= SSID) for "Wireless LAN" Bearer
        //      3. "Dial-up number" for "Data call Bearer" or "High Speed (GSM)" Bearer
        //    <=> Note: It's possible that in this case reported IAP is
        //              clone of the one of the IAPs of the used SNAP
        //              => If mappingName matches, clone has been found
        QNetworkConfiguration pt = QNetworkConfigurationManager().configurationFromIdentifier(
                QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(iapId)));

        SymbianNetworkConfigurationPrivate *symbianConfig =
            toSymbianConfig(privateConfiguration(pt));
        if (symbianConfig) {
            for (int i=0; i < children.count(); i++) {
                SymbianNetworkConfigurationPrivate *childConfig =
                    toSymbianConfig(privateConfiguration(children[i]));

                if (childConfig->configMappingName() == symbianConfig->configMappingName()) {
                    return children[i];
                }
            }
        } else {
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
            // On Symbian^3 (only, not earlier or Symbian^4) if the SNAP was not reachable, it
            // triggers user choice type of activity (EasyWLAN). As a result, a new IAP may be
            // created, and hence if was not found yet. Therefore update configurations and see if
            // there is something new.

            // 1. Update knowledge from the databases.
            if (thread() != engine->thread())
                QMetaObject::invokeMethod(engine, "requestUpdate", Qt::BlockingQueuedConnection);
            else
                engine->requestUpdate();

            // 2. Check if new configuration was created during connection creation
            QList<QString> knownConfigs = engine->accessPointConfigurationIdentifiers();
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
            qDebug() << "QNS this : " << QString::number((uint)this) << " - "
                    << "opened configuration was not known beforehand, looking for new.";
#endif
            if (knownConfigs.count() > iKnownConfigsBeforeConnectionStart.count()) {
                // Configuration count increased => new configuration was created
                // => Search new, created configuration
                QString newIapId;
                for (int i=0; i < iKnownConfigsBeforeConnectionStart.count(); i++) {
                    if (knownConfigs[i] != iKnownConfigsBeforeConnectionStart[i]) {
                        newIapId = knownConfigs[i];
                        break;
                    }
                }
                if (newIapId.isEmpty()) {
                    newIapId = knownConfigs[knownConfigs.count()-1];
                }
                pt = QNetworkConfigurationManager().configurationFromIdentifier(newIapId);
                if (pt.isValid()) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
                    qDebug() << "QNS this : " << QString::number((uint)this) << " - "
                            << "new configuration was found, name, IAP id: " << pt.name() << pt.identifier();
#endif
                    return pt;
                }
            }
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
            qDebug() << "QNS this : " << QString::number((uint)this) << " - "
                    << "configuration was not found, returning invalid.";
#endif
#endif
            // Given IAP Id was not found from known IAPs array
            return QNetworkConfiguration();
        }
        // Matching IAP was not found from used SNAP
        // => IAP from another SNAP is returned
        //    (Note: Returned IAP matches to given IAP Id)
        return pt;
    }
#endif
    if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
        if (engine) {
            QNetworkConfiguration pt = QNetworkConfigurationManager().configurationFromIdentifier(
                    QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(iapId)));
            // Try to found User Selected IAP from known IAPs (accessPointConfigurations)
            if (pt.isValid()) {
                return pt;
            } else {
                // Check if new (WLAN) IAP was created in IAP/SNAP dialog
                // 1. Sync internal configurations array to commsdb first
                if (thread() != engine->thread()) {
                    QMetaObject::invokeMethod(engine, "requestUpdate",
                                              Qt::BlockingQueuedConnection);
                } else {
                    engine->requestUpdate();
                }
                // 2. Check if new configuration was created during connection creation
                QStringList knownConfigs = engine->accessPointConfigurationIdentifiers();
                if (knownConfigs.count() > iKnownConfigsBeforeConnectionStart.count()) {
                    // Configuration count increased => new configuration was created
                    // => Search new, created configuration
                    QString newIapId;
                    for (int i=0; i < iKnownConfigsBeforeConnectionStart.count(); i++) {
                        if (knownConfigs[i] != iKnownConfigsBeforeConnectionStart[i]) {
                            newIapId = knownConfigs[i];
                            break;
                        }
                    }
                    if (newIapId.isEmpty()) {
                        newIapId = knownConfigs[knownConfigs.count()-1];
                    }
                    pt = QNetworkConfigurationManager().configurationFromIdentifier(newIapId);
                    if (pt.isValid())
                        return pt;
                }
            }
        }
        return QNetworkConfiguration();
    }

    return publicConfig;
}

void QNetworkSessionPrivateImpl::ConnectionStartComplete(TInt statusCode)
{
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this) << " - "
            << "RConnection::Start completed with status code: " << statusCode;
#endif
    delete ipConnectionStarter;
    ipConnectionStarter = 0;

    switch (statusCode) {
        case KErrNone: // Connection created successfully
            {
            TInt error = KErrNone;
            QNetworkConfiguration newActiveConfig = activeConfiguration();
            if (!newActiveConfig.isValid()) {
                // RConnection startup was successful but no configuration
                // was found. That indicates that user has chosen to create a
                // new WLAN configuration (from scan results), but that new
                // configuration does not have access to Internet (Internet
                // Connectivity Test, ICT, failed).
                error = KErrGeneral;
            } else {
                QSymbianSocketManager::instance().setDefaultConnection(&iConnection);
            }
            if (error != KErrNone) {
                isOpen = false;
                isOpening = false;
                iError = QNetworkSession::UnknownSessionError;
                QT_TRYCATCH_LEAVING(emit QNetworkSessionPrivate::error(iError));
                closeHandles();
                if (!newActiveConfig.isValid()) {
                    // No valid configuration, bail out.
                    // Status updates from QNCM won't be received correctly
                    // because there is no configuration to associate them with so transit here.
                    newState(QNetworkSession::Closing);
                    newState(QNetworkSession::Disconnected);
                }
                QT_TRYCATCH_LEAVING(syncStateWithInterface());
                return;
            }
 
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
            if (publicConfig.type() == QNetworkConfiguration::ServiceNetwork) {
                // Activate ALR monitoring
                iMobility = CActiveCommsMobilityApiExt::NewL(iConnection, *this);
            }
#endif

            isOpen = true;
            isOpening = false;
            activeConfig = newActiveConfig;

            SymbianNetworkConfigurationPrivate *symbianConfig =
                toSymbianConfig(privateConfiguration(activeConfig));

#ifndef QT_NO_NETWORKINTERFACE
            activeInterface = interface(symbianConfig->numericIdentifier());
#endif
            if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
                serviceConfig = QNetworkConfigurationManager()
                    .configurationFromIdentifier(activeConfig.identifier());
            }

            startTime = QDateTime::currentDateTime();

            QT_TRYCATCH_LEAVING({
                    newState(QNetworkSession::Connected);
                    emit quitPendingWaitsForOpened();
                });
            }
            break;
        case KErrNotFound: // Connection failed
            isOpen = false;
            isOpening = false;
            activeConfig = QNetworkConfiguration();
            serviceConfig = QNetworkConfiguration();
            iError = QNetworkSession::InvalidConfigurationError;
            QT_TRYCATCH_LEAVING(emit QNetworkSessionPrivate::error(iError));
            closeHandles();
            QT_TRYCATCH_LEAVING(syncStateWithInterface());
            break;
        case KErrCancel: // Connection attempt cancelled
        case KErrAlreadyExists: // Connection already exists
        default:
            isOpen = false;
            isOpening = false;
            activeConfig = QNetworkConfiguration();
            serviceConfig = QNetworkConfiguration();
            if (statusCode == KErrCancel) {
                iError = QNetworkSession::SessionAbortedError;
            } else if (publicConfig.state() == QNetworkConfiguration::Undefined ||
                publicConfig.state() == QNetworkConfiguration::Defined) {
                iError = QNetworkSession::InvalidConfigurationError;
            } else {
                iError = QNetworkSession::UnknownSessionError;
            }
            QT_TRYCATCH_LEAVING(emit QNetworkSessionPrivate::error(iError));
            closeHandles();
            QT_TRYCATCH_LEAVING(syncStateWithInterface());
            break;
    }
}

// Enters newState if feasible according to current state.
// AccessPointId may be given as parameter. If it is zero, state-change is assumed to
// concern this session's configuration. If non-zero, the configuration is looked up
// and checked if it matches the configuration this session is based on.
bool QNetworkSessionPrivateImpl::newState(QNetworkSession::State newState, TUint accessPointId)
{
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this) << " - "
             << "NEW STATE, IAP ID : " << QString::number(accessPointId) << " , newState : " << QString::number(newState);
#endif
    // Make sure that activeConfig is always updated when SNAP is signaled to be
    // connected.
    if (isOpen && publicConfig.type() == QNetworkConfiguration::ServiceNetwork &&
        newState == QNetworkSession::Connected) {
        activeConfig = activeConfiguration(accessPointId);

#ifndef QT_NO_NETWORKINTERFACE
        SymbianNetworkConfigurationPrivate *symbianConfig =
            toSymbianConfig(privateConfiguration(activeConfig));

        activeInterface = interface(symbianConfig->numericIdentifier());
#endif

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
        QSymbianSocketManager::instance().setDefaultConnection(&iConnection);
#endif
    }

    // Make sure that same state is not signaled twice in a row.
    if (state == newState) {
        return true;
    }

    // Make sure that Connecting state does not overwrite Roaming state
    if (state == QNetworkSession::Roaming && newState == QNetworkSession::Connecting) {
        return false;
    }
    
    // Make sure that Connected state is not reported when Connection is
    // already Closing.
    // Note: Stopping connection results sometimes KLinkLayerOpen
    //       to be reported first (just before KLinkLayerClosed).
    if (state == QNetworkSession::Closing && newState == QNetworkSession::Connected) {
        return false;
    }

    // Make sure that some lagging 'closing' state-changes do not overwrite
    // if we are already disconnected or closed.
    if (state == QNetworkSession::Disconnected && newState == QNetworkSession::Closing) {
        return false;
    }

    // Make sure that some lagging 'connecting' state-changes do not overwrite
    // if we are already connected (may righfully still happen with roaming though).
    if (state == QNetworkSession::Connected && newState == QNetworkSession::Connecting) {
        return false;
    }

    bool emitSessionClosed = false;

    // If we abruptly go down and user hasn't closed the session, we've been aborted.
    // Note that session may be in 'closing' state and not in 'connected' state, because
    // depending on platform the platform may report KConfigDaemonStartingDeregistration
    // event before KLinkLayerClosed
    if ((isOpen && state == QNetworkSession::Connected && newState == QNetworkSession::Disconnected) ||
        (isOpen && !iClosedByUser && newState == QNetworkSession::Disconnected)) {
        // Active & Connected state should change directly to Disconnected state
        // only when something forces connection to close (eg. when another
        // application or session stops connection or when network drops
        // unexpectedly).
        isOpen = false;
        isOpening = false;
        activeConfig = QNetworkConfiguration();
        serviceConfig = QNetworkConfiguration();
        iError = QNetworkSession::SessionAbortedError;
        emit QNetworkSessionPrivate::error(iError);
        closeHandles();
        // Start handling IAP state change signals from QNetworkConfigurationManagerPrivate
        iHandleStateNotificationsFromManager = true;
        emitSessionClosed = true; // Emit SessionClosed after state change has been reported
    }

    bool retVal = false;
    if (accessPointId == 0) {
        state = newState;
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
        qDebug() << "QNS this : " << QString::number((uint)this) << " - " << "===> EMIT State changed A to: " << state;
#endif
        emit stateChanged(state);
        retVal = true;
    } else {
        if (publicConfig.type() == QNetworkConfiguration::InternetAccessPoint) {
            SymbianNetworkConfigurationPrivate *symbianConfig =
                toSymbianConfig(privateConfiguration(publicConfig));

            if (symbianConfig->numericIdentifier() == accessPointId) {
                state = newState;
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
                qDebug() << "QNS this : " << QString::number((uint)this) << " - " << "===> EMIT State changed B to: " << state;
#endif
                emit stateChanged(state);
                retVal = true;
            }
        } else if (publicConfig.type() == QNetworkConfiguration::UserChoice && isOpen) {
            SymbianNetworkConfigurationPrivate *symbianConfig =
                toSymbianConfig(privateConfiguration(activeConfig));

            if (symbianConfig->numericIdentifier() == accessPointId) {
                state = newState;
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
                qDebug() << "QNS this : " << QString::number((uint)this) << " - " << "===> EMIT State changed C to: " << state;
#endif
                emit stateChanged(state);
                retVal = true;
            }
        } else if (publicConfig.type() == QNetworkConfiguration::ServiceNetwork) {
            QList<QNetworkConfiguration> subConfigurations = publicConfig.children();
            for (int i = 0; i < subConfigurations.count(); i++) {
                SymbianNetworkConfigurationPrivate *symbianConfig =
                    toSymbianConfig(privateConfiguration(subConfigurations[i]));

                if (symbianConfig->numericIdentifier() == accessPointId) {
                    if (newState != QNetworkSession::Disconnected) {
                        state = newState;
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
                        qDebug() << "QNS this : " << QString::number((uint)this) << " - " << "===> EMIT State changed D  to: " << state;
#endif
                        emit stateChanged(state);
                        retVal = true;
                    } else {
                        QNetworkConfiguration config = bestConfigFromSNAP(publicConfig);
                        if ((config.state() == QNetworkConfiguration::Defined) ||
                            (config.state() == QNetworkConfiguration::Discovered)) {
                            activeConfig = QNetworkConfiguration();
                            state = newState;
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
                            qDebug() << "QNS this : " << QString::number((uint)this) << " - " << "===> EMIT State changed E  to: " << state;
#endif
                            emit stateChanged(state);
                            retVal = true;
                        } else if (config.state() == QNetworkConfiguration::Active) {
                            // Connection to used IAP was closed, but there is another
                            // IAP that's active in used SNAP
                            // => Change state back to Connected
                            state =  QNetworkSession::Connected;
                            emit stateChanged(state);
                            retVal = true;
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
                            qDebug() << "QNS this : " << QString::number((uint)this) << " - " << "===> EMIT State changed F  to: " << state;
#endif
                        }
                    }
                }
            }
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
            // If the retVal is not true here, it means that the status update may apply to an IAP outside of
            // SNAP (session is based on SNAP but follows IAP outside of it), which may occur on Symbian^3 EasyWlan.
            if (retVal == false && activeConfig.isValid() &&
                toSymbianConfig(privateConfiguration(activeConfig))->numericIdentifier() == accessPointId) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
                qDebug() << "QNS this : " << QString::number((uint)this) << " - " << "===> EMIT State changed G  to: " << state;
#endif
                if (newState == QNetworkSession::Disconnected) {
                    activeConfig = QNetworkConfiguration();
                }
                state = newState;
                emit stateChanged(state);
                retVal = true;
            }
#endif
        }
    }
    if (emitSessionClosed) {
        emit closed();
    }
    if (state == QNetworkSession::Disconnected) {
        // Just in case clear activeConfiguration.
        activeConfig = QNetworkConfiguration();
    }
    return retVal;
}

void QNetworkSessionPrivateImpl::handleSymbianConnectionStatusChange(TInt aConnectionStatus,
                                                                 TInt aError,
                                                                 TUint accessPointId)
{
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNS this : " << QString::number((uint)this) << " - " << QString::number(accessPointId) << " , status : " << QString::number(aConnectionStatus);
#endif
    switch (aConnectionStatus)
        {
        // Connection unitialised
        case KConnectionUninitialised:
            break;

        // Starting connetion selection
        case KStartingSelection:
            break;

        // Selection finished
        case KFinishedSelection:
            if (aError == KErrNone)
                {
                break;
                }
            else
                {
                // The user pressed e.g. "Cancel" and did not select an IAP
                newState(QNetworkSession::Disconnected,accessPointId);
                }
            break;

        // Connection failure
        case KConnectionFailure:
            newState(QNetworkSession::NotAvailable);
            break;

        // Prepearing connection (e.g. dialing)
        case KPsdStartingConfiguration:
        case KPsdFinishedConfiguration:
        case KCsdFinishedDialling:
        case KCsdScanningScript:
        case KCsdGettingLoginInfo:
        case KCsdGotLoginInfo:
            break;

        case KConfigDaemonStartingRegistration:
        // Creating connection (e.g. GPRS activation)
        case KCsdStartingConnect:
        case KCsdFinishedConnect:
            newState(QNetworkSession::Connecting,accessPointId);
            break;

        // Starting log in
        case KCsdStartingLogIn:
            break;

        // Finished login
        case KCsdFinishedLogIn:
            break;

        // Connection open
        case KConnectionOpen:
            break;

        case KLinkLayerOpen:
            newState(QNetworkSession::Connected,accessPointId);
            break;

        // Connection blocked or suspended
        case KDataTransferTemporarilyBlocked:
            break;

        case KConfigDaemonStartingDeregistration:
        // Hangup or GRPS deactivation
        case KConnectionStartingClose:
            newState(QNetworkSession::Closing,accessPointId);
            break;

        // Connection closed
        case KConnectionClosed:
        case KLinkLayerClosed:
            newState(QNetworkSession::Disconnected,accessPointId);
            // Report manager about this to make sure this event
            // is received by all interseted parties (mediated by
            // manager because it does always receive all events from
            // connection monitor).
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
            qDebug() << "QNS this : " << QString::number((uint)this) << " - " << "reporting disconnection to manager.";
#endif
            if (publicConfig.isValid()) {
                SymbianNetworkConfigurationPrivate *symbianConfig =
                    toSymbianConfig(privateConfiguration(publicConfig));

                engine->configurationStateChangeReport(symbianConfig->numericIdentifier(),
                                                       QNetworkSession::Disconnected);
            }
            break;
        // Unhandled state
        default:
            break;
        }
}

#if defined(SNAP_FUNCTIONALITY_AVAILABLE)
bool QNetworkSessionPrivateImpl::easyWlanTrueIapId(TUint32 &trueIapId) const
{
    RCmManager iCmManager;
    TRAPD(err, iCmManager.OpenL());
    if (err != KErrNone)
        return false;

    // Check if this is easy wlan id in the first place
    if (trueIapId != iCmManager.EasyWlanIdL()) {
        iCmManager.Close();
        return false;
    }

    iCmManager.Close();

    // Loop through all connections that connection monitor is aware
    // and check for IAPs based on easy WLAN
    TRequestStatus status;
    TUint connectionCount;
    iConnectionMonitor.GetConnectionCount(connectionCount, status);
    User::WaitForRequest(status);
    TUint connectionId;
    TUint subConnectionCount;
    TUint apId;
    if (status.Int() == KErrNone) {
        for (TUint i = 1; i <= connectionCount; i++) {
            iConnectionMonitor.GetConnectionInfo(i, connectionId, subConnectionCount);
            iConnectionMonitor.GetUintAttribute(connectionId, subConnectionCount,
                                                KIAPId, apId, status);
            User::WaitForRequest(status);
            if (apId == trueIapId) {
                TBuf<50>easyWlanNetworkName;
                iConnectionMonitor.GetStringAttribute(connectionId, 0, KNetworkName,
                                                      easyWlanNetworkName, status);
                User::WaitForRequest(status);
                if (status.Int() != KErrNone)
                    continue;

                const QString ssid = QString::fromUtf16(easyWlanNetworkName.Ptr(),
                                                            easyWlanNetworkName.Length());

                QNetworkConfigurationPrivatePointer ptr = engine->configurationFromSsid(ssid);
                if (ptr) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
                    qDebug() << "QNCM easyWlanTrueIapId(), found true IAP ID: "
                             << toSymbianConfig(ptr)->numericIdentifier();
#endif
                    trueIapId = toSymbianConfig(ptr)->numericIdentifier();
                    return true;
                }
            }
        }
    }
    return false;
}
#endif

RConnection* QNetworkSessionPrivateImpl::nativeSession()
{
    return &iConnection;
}

ConnectionProgressNotifier::ConnectionProgressNotifier(QNetworkSessionPrivateImpl& owner, RConnection& connection)
    : CActive(CActive::EPriorityUserInput), iOwner(owner), iConnection(connection)
{
    CActiveScheduler::Add(this);
}

ConnectionProgressNotifier::~ConnectionProgressNotifier()
{
    Cancel();
}

void ConnectionProgressNotifier::StartNotifications()
{
    if (!IsActive()) {
        SetActive();
        iConnection.ProgressNotification(iProgress, iStatus);
    }
}

void ConnectionProgressNotifier::StopNotifications()
{
    Cancel();
}

void ConnectionProgressNotifier::DoCancel()
{
    iConnection.CancelProgressNotification();
}

void ConnectionProgressNotifier::RunL()
{
    if (iStatus == KErrNone) {
        SetActive();
        iConnection.ProgressNotification(iProgress, iStatus);
        // warning, this object may be deleted in the callback - do nothing after handleSymbianConnectionStatusChange
        QT_TRYCATCH_LEAVING(iOwner.handleSymbianConnectionStatusChange(iProgress().iStage, iProgress().iError));
    }
}

ConnectionStarter::ConnectionStarter(QNetworkSessionPrivateImpl &owner, RConnection &connection)
    : CActive(CActive::EPriorityUserInput), iOwner(owner), iConnection(connection)
{
    CActiveScheduler::Add(this);
}

ConnectionStarter::~ConnectionStarter()
{
    Cancel();
}

void ConnectionStarter::Start()
{
    if (!IsActive()) {
        iConnection.Start(iStatus);
        SetActive();
    }
}

void ConnectionStarter::Start(TConnPref &pref)
{
    if (!IsActive()) {
        iConnection.Start(pref, iStatus);
        SetActive();
    }
}

void ConnectionStarter::RunL()
{
    iOwner.ConnectionStartComplete(iStatus.Int());
    //note owner deletes on callback
}

TInt ConnectionStarter::RunError(TInt err)
{
    qWarning() << "ConnectionStarter::RunError" << err;
    return KErrNone;
}

void ConnectionStarter::DoCancel()
{
    iConnection.Close();
}

QT_END_NAMESPACE

#endif //QT_NO_BEARERMANAGEMENT

