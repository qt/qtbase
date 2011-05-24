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

#include "symbianengine.h"
#include "qnetworksession_impl.h"

#include <commdb.h>
#include <cdbcols.h>
#include <d32dbms.h>
#include <nifvar.h>
#include <QTimer>
#include <QTime>  // For randgen seeding
#include <QtCore> // For randgen seeding


#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
#include <QDebug>
#endif

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    #include <cmdestination.h>
    #include <cmconnectionmethod.h>
    #include <cmconnectionmethoddef.h>
    #include <cmpluginwlandef.h>
    #include <cmpluginpacketdatadef.h>
    #include <cmplugindialcommondefs.h>
#else
    #include <ApAccessPointItem.h>
    #include <ApDataHandler.h>
    #include <ApUtils.h>
#endif

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

static const int KUserChoiceIAPId = 0;

SymbianNetworkConfigurationPrivate::SymbianNetworkConfigurationPrivate()
:   numericId(0), connectionId(0)
{
}

SymbianNetworkConfigurationPrivate::~SymbianNetworkConfigurationPrivate()
{
}

SymbianEngine::SymbianEngine(QObject *parent)
:   QBearerEngine(parent), CActive(CActive::EPriorityHigh), iFirstUpdate(true), ipCommsDB(0),
    iInitOk(true), iUpdatePending(false), ipAccessPointsAvailabilityScanner(0)
{
}

void SymbianEngine::initialize()
{
    QMutexLocker locker(&mutex);

    CActiveScheduler::Add(this);

    // Seed the randomgenerator
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()) + QCoreApplication::applicationPid());

    TRAPD(error, ipCommsDB = CCommsDatabase::NewL(EDatabaseTypeIAP));
    if (error != KErrNone) {
        iInitOk = false;
        return;
    }

    TRAP_IGNORE(iConnectionMonitor.ConnectL());
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    TRAP_IGNORE(iConnectionMonitor.SetUintAttribute(EBearerIdAll, 0, KBearerGroupThreshold, 1));
#endif
    TRAP_IGNORE(iConnectionMonitor.NotifyEventL(*this));

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    TRAP(error, iCmManager.OpenL());
    if (error != KErrNone) {
        iInitOk = false;
        return;
    }
#endif

    SymbianNetworkConfigurationPrivate *cpPriv = new SymbianNetworkConfigurationPrivate;
    cpPriv->name = "UserChoice";
    cpPriv->bearerType = QNetworkConfiguration::BearerUnknown;
    cpPriv->state = QNetworkConfiguration::Discovered;
    cpPriv->isValid = true;
    cpPriv->id = QString::number(qHash(KUserChoiceIAPId));
    cpPriv->numericId = KUserChoiceIAPId;
    cpPriv->connectionId = 0;
    cpPriv->type = QNetworkConfiguration::UserChoice;
    cpPriv->purpose = QNetworkConfiguration::UnknownPurpose;
    cpPriv->roamingSupported = false;

    QNetworkConfigurationPrivatePointer ptr(cpPriv);
    userChoiceConfigurations.insert(ptr->id, ptr);

    updateConfigurations();
    updateStatesToSnaps();
    updateAvailableAccessPoints(); // On first time updates (without WLAN scans)
    // Start monitoring IAP and/or SNAP changes in Symbian CommsDB
    startCommsDatabaseNotifications();
}

SymbianEngine::~SymbianEngine()
{
    Cancel();

    //The scanner may be using the connection monitor so it needs to be
    //deleted first while the handle is still valid.
    delete ipAccessPointsAvailabilityScanner;

    iConnectionMonitor.CancelNotifications();
    iConnectionMonitor.Close();

    // CCommsDatabase destructor and RCmManager.Close() use cleanup stack. Since QNetworkConfigurationManager
    // is a global static, but the time we are here, E32Main() has been exited already and
    // the thread's default cleanup stack has been deleted. Without this line, a
    // 'E32USER-CBase 69' -panic will occur.
    CTrapCleanup* cleanup = CTrapCleanup::New();
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    iCmManager.Close();
#endif
    delete ipCommsDB;
    delete cleanup;
}

void SymbianEngine::delayedConfigurationUpdate()
{
    QMutexLocker locker(&mutex);

    if (iUpdatePending) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
        qDebug("QNCM delayed configuration update (ECommit or ERecover occurred).");
#endif
        TRAPD(error, updateConfigurationsL());
        if (error == KErrNone) {
            updateStatesToSnaps();
        }
        iUpdatePending = false;
        // Start monitoring again.
        if (!IsActive()) {
            SetActive();
            // Start waiting for new notification
            ipCommsDB->RequestNotification(iStatus);
        }
    }
}

bool SymbianEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);

    return accessPointConfigurations.contains(id) ||
           snapConfigurations.contains(id) ||
           userChoiceConfigurations.contains(id);
}

QNetworkConfigurationManager::Capabilities SymbianEngine::capabilities() const
{
    QNetworkConfigurationManager::Capabilities capFlags;

    capFlags = QNetworkConfigurationManager::CanStartAndStopInterfaces |
               QNetworkConfigurationManager::DirectConnectionRouting |
               QNetworkConfigurationManager::SystemSessionSupport |
               QNetworkConfigurationManager::DataStatistics |
               QNetworkConfigurationManager::NetworkSessionRequired;

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    capFlags |= QNetworkConfigurationManager::ApplicationLevelRoaming |
                QNetworkConfigurationManager::ForcedRoaming;
#endif

    return capFlags;
}

QNetworkSessionPrivate *SymbianEngine::createSessionBackend()
{
    return new QNetworkSessionPrivateImpl(this);
}

void SymbianEngine::requestUpdate()
{
    QMutexLocker locker(&mutex);

    if (!iInitOk || iUpdateGoingOn) {
        return;
    }
    iUpdateGoingOn = true;

    stopCommsDatabaseNotifications();
    updateConfigurations(); // Synchronous call
    updateAvailableAccessPoints(); // Asynchronous call
}

void SymbianEngine::updateConfigurations()
{
    if (!iInitOk)
        return;

    TRAP_IGNORE(updateConfigurationsL());
}

void SymbianEngine::updateConfigurationsL()
{
    QList<QString> knownConfigs = accessPointConfigurations.keys();
    QList<QString> knownSnapConfigs = snapConfigurations.keys();

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    // S60 version is >= Series60 3rd Edition Feature Pack 2
    TInt error = KErrNone;
    
    // Loop through all IAPs
    RArray<TUint32> connectionMethods; // IAPs
    CleanupClosePushL(connectionMethods);
    iCmManager.ConnectionMethodL(connectionMethods);
    for(int i = 0; i < connectionMethods.Count(); i++) {
        RCmConnectionMethod connectionMethod = iCmManager.ConnectionMethodL(connectionMethods[i]);
        CleanupClosePushL(connectionMethod);
        TUint32 iapId = connectionMethod.GetIntAttributeL(CMManager::ECmIapId);
        QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(iapId));
        if (accessPointConfigurations.contains(ident)) {
            knownConfigs.removeOne(ident);
        } else {
            SymbianNetworkConfigurationPrivate* cpPriv = NULL;
            TRAP(error, cpPriv = configFromConnectionMethodL(connectionMethod));
            if (error == KErrNone) {
                QNetworkConfigurationPrivatePointer ptr(cpPriv);
                accessPointConfigurations.insert(ptr->id, ptr);
                if (!iFirstUpdate) {
                    // Emit configuration added. Connected slots may throw execptions
                    // which propagate here --> must be converted to leaves (standard
                    // std::exception would cause any TRAP trapping this function to terminate
                    // program).
                    QT_TRYCATCH_LEAVING(updateActiveAccessPoints());
                    updateStatesToSnaps();
                    mutex.unlock();
                    QT_TRYCATCH_LEAVING(emit configurationAdded(ptr));
                    mutex.lock();
                }
            }
        }
        CleanupStack::PopAndDestroy(&connectionMethod);
    }
    CleanupStack::PopAndDestroy(&connectionMethods);
    
    // Loop through all SNAPs
    RArray<TUint32> destinations;
    CleanupClosePushL(destinations);
    iCmManager.AllDestinationsL(destinations);
    for(int i = 0; i < destinations.Count(); i++) {
        RCmDestination destination;

        // Some destinatsions require ReadDeviceData -capability (MMS/WAP)
        // The below function will leave in these cases. Don't. Proceed to
        // next destination (if any).
        TRAPD(error, destination = iCmManager.DestinationL(destinations[i]));
        if (error == KErrPermissionDenied) {
            continue;
        } else {
            User::LeaveIfError(error);
        }

        CleanupClosePushL(destination);
        QString ident = QT_BEARERMGMT_CONFIGURATION_SNAP_PREFIX +
                        QString::number(qHash(destination.Id()));
        if (snapConfigurations.contains(ident)) {
            knownSnapConfigs.removeOne(ident);
        } else {
            SymbianNetworkConfigurationPrivate *cpPriv = new SymbianNetworkConfigurationPrivate;

            HBufC *pName = destination.NameLC();
            QT_TRYCATCH_LEAVING(cpPriv->name = QString::fromUtf16(pName->Ptr(),pName->Length()));
            CleanupStack::PopAndDestroy(pName);
            pName = NULL;

            cpPriv->isValid = true;
            cpPriv->id = ident;
            cpPriv->numericId = destination.Id();
            cpPriv->connectionId = 0;
            cpPriv->state = QNetworkConfiguration::Defined;
            cpPriv->type = QNetworkConfiguration::ServiceNetwork;
            cpPriv->purpose = QNetworkConfiguration::UnknownPurpose;
            cpPriv->roamingSupported = false;

            QNetworkConfigurationPrivatePointer ptr(cpPriv);
            snapConfigurations.insert(ident, ptr);
            if (!iFirstUpdate) {
                QT_TRYCATCH_LEAVING(updateActiveAccessPoints());
                updateStatesToSnaps();
                mutex.unlock();
                QT_TRYCATCH_LEAVING(emit configurationAdded(ptr));
                mutex.lock();
            }
        }

        // Loop through all connection methods in this SNAP
        QMap<unsigned int, QNetworkConfigurationPrivatePointer> connections;
        for (int j=0; j < destination.ConnectionMethodCount(); j++) {
            RCmConnectionMethod connectionMethod = destination.ConnectionMethodL(j);
            CleanupClosePushL(connectionMethod);

            TUint32 iapId = connectionMethod.GetIntAttributeL(CMManager::ECmIapId);
            QString iface = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(iapId));
            // Check that IAP can be found from accessPointConfigurations list
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(iface);
            if (!ptr) {
                SymbianNetworkConfigurationPrivate *cpPriv = NULL;
                TRAP(error, cpPriv = configFromConnectionMethodL(connectionMethod));
                if (error == KErrNone) {
                    ptr = QNetworkConfigurationPrivatePointer(cpPriv);
                    accessPointConfigurations.insert(ptr->id, ptr);

                    if (!iFirstUpdate) {
                        QT_TRYCATCH_LEAVING(updateActiveAccessPoints());
                        updateStatesToSnaps();
                        mutex.unlock();
                        QT_TRYCATCH_LEAVING(emit configurationAdded(ptr));
                        mutex.lock();
                    }
                }
            } else {
                knownConfigs.removeOne(iface);
            }

            if (ptr) {
                unsigned int priority;
                TRAPD(error, priority = destination.PriorityL(connectionMethod));
                if (!error)
                    connections.insert(priority, ptr);
            }

            CleanupStack::PopAndDestroy(&connectionMethod);
        }

        QNetworkConfigurationPrivatePointer privSNAP = snapConfigurations.value(ident);
        QMutexLocker snapConfigLocker(&privSNAP->mutex);

        if (privSNAP->serviceNetworkMembers != connections) {
            privSNAP->serviceNetworkMembers = connections;

            // Roaming is supported only if SNAP contains more than one IAP
            privSNAP->roamingSupported = privSNAP->serviceNetworkMembers.count() > 1;

            snapConfigLocker.unlock();

            updateStatesToSnaps();

            mutex.unlock();
            QT_TRYCATCH_LEAVING(emit configurationChanged(privSNAP));
            mutex.lock();
        }

        CleanupStack::PopAndDestroy(&destination);
    }
    CleanupStack::PopAndDestroy(&destinations);
#else
    // S60 version is < Series60 3rd Edition Feature Pack 2
    CCommsDbTableView* pDbTView = ipCommsDB->OpenTableLC(TPtrC(IAP));

    // Loop through all IAPs
    TUint32 apId = 0;
    TInt retVal = pDbTView->GotoFirstRecord();
    while (retVal == KErrNone) {
        pDbTView->ReadUintL(TPtrC(COMMDB_ID), apId);
        QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(apId));
        if (accessPointConfigurations.contains(ident)) {
            knownConfigs.removeOne(ident);
        } else {
            SymbianNetworkConfigurationPrivate *cpPriv = new SymbianNetworkConfigurationPrivate;
            if (readNetworkConfigurationValuesFromCommsDb(apId, cpPriv)) {
                QNetworkConfigurationPrivatePointer ptr(cpPriv);
                accessPointConfigurations.insert(ident, ptr);
                if (!iFirstUpdate) {
                    QT_TRYCATCH_LEAVING(updateActiveAccessPoints());
                    updateStatesToSnaps();
                    mutex.unlock();
                    QT_TRYCATCH_LEAVING(emit configurationAdded(ptr));
                    mutex.lock();
                }
            } else {
                delete cpPriv;
            }
        }
        retVal = pDbTView->GotoNextRecord();
    }
    CleanupStack::PopAndDestroy(pDbTView);
#endif

    QT_TRYCATCH_LEAVING(updateActiveAccessPoints());

    foreach (const QString &oldIface, knownConfigs) {
        //remove non existing IAP
        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.take(oldIface);

        mutex.unlock();
        emit configurationRemoved(ptr);
        QT_TRYCATCH_LEAVING(emit configurationRemoved(ptr));
        mutex.lock();

        // Remove non existing IAP from SNAPs
        foreach (const QString &iface, snapConfigurations.keys()) {
            QNetworkConfigurationPrivatePointer ptr2 = snapConfigurations.value(iface);
            // => Check if one of the IAPs of the SNAP is active
            QMutexLocker snapConfigLocker(&ptr2->mutex);
            QMutableMapIterator<unsigned int, QNetworkConfigurationPrivatePointer> i(ptr2->serviceNetworkMembers);
            while (i.hasNext()) {
                i.next();

                if (toSymbianConfig(i.value())->numericIdentifier() ==
                    toSymbianConfig(ptr)->numericIdentifier()) {
                    i.remove();
                    break;
                }
            }
        }    
    }

    foreach (const QString &oldIface, knownSnapConfigs) {
        //remove non existing SNAPs
        QNetworkConfigurationPrivatePointer ptr = snapConfigurations.take(oldIface);

        mutex.unlock();
        emit configurationRemoved(ptr);
        QT_TRYCATCH_LEAVING(emit configurationRemoved(ptr));
        mutex.lock();
    }

    // find default configuration.
    stopCommsDatabaseNotifications();
    TRAP_IGNORE(defaultConfig = defaultConfigurationL());
    startCommsDatabaseNotifications();

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    updateStatesToSnaps();
#endif
}

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
SymbianNetworkConfigurationPrivate *SymbianEngine::configFromConnectionMethodL(
        RCmConnectionMethod& connectionMethod)
{
    SymbianNetworkConfigurationPrivate *cpPriv = new SymbianNetworkConfigurationPrivate;
    TUint32 iapId = connectionMethod.GetIntAttributeL(CMManager::ECmIapId);
    QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(iapId));
    
    HBufC *pName = connectionMethod.GetStringAttributeL(CMManager::ECmName);
    CleanupStack::PushL(pName);
    QT_TRYCATCH_LEAVING(cpPriv->name = QString::fromUtf16(pName->Ptr(),pName->Length()));
    CleanupStack::PopAndDestroy(pName);
    pName = NULL;
    
    TUint32 bearerId = connectionMethod.GetIntAttributeL(CMManager::ECmCommsDBBearerType);
    switch (bearerId) {
    case KCommDbBearerCSD:
        cpPriv->bearerType = QNetworkConfiguration::Bearer2G;
        break;
    case KCommDbBearerWcdma:
        cpPriv->bearerType = QNetworkConfiguration::BearerWCDMA;
        break;
    case KCommDbBearerLAN:
        cpPriv->bearerType = QNetworkConfiguration::BearerEthernet;
        break;
    case KCommDbBearerVirtual:
        cpPriv->bearerType = QNetworkConfiguration::BearerUnknown;
        break;
    case KCommDbBearerPAN:
        cpPriv->bearerType = QNetworkConfiguration::BearerUnknown;
        break;
    case KCommDbBearerWLAN:
        cpPriv->bearerType = QNetworkConfiguration::BearerWLAN;
        break;
    default:
        cpPriv->bearerType = QNetworkConfiguration::BearerUnknown;
        break;
    }
    
    TInt error = KErrNone;
    TUint32 bearerType = connectionMethod.GetIntAttributeL(CMManager::ECmBearerType);
    switch (bearerType) {
    case KUidPacketDataBearerType:
        // "Packet data" Bearer => Mapping is done using "Access point name"
        TRAP(error, pName = connectionMethod.GetStringAttributeL(CMManager::EPacketDataAPName));
        break;
    case KUidWlanBearerType:
        // "Wireless LAN" Bearer => Mapping is done using "WLAN network name" = SSID
        TRAP(error, pName = connectionMethod.GetStringAttributeL(CMManager::EWlanSSID));
        break;
    }
    if (!pName) {
        // "Data call" Bearer or "High Speed (GSM)" Bearer => Mapping is done using "Dial-up number"
        TRAP(error, pName = connectionMethod.GetStringAttributeL(CMManager::EDialDefaultTelNum));
    }

    if (error == KErrNone && pName) {
        CleanupStack::PushL(pName);
        QT_TRYCATCH_LEAVING(cpPriv->mappingName = QString::fromUtf16(pName->Ptr(),pName->Length()));
        CleanupStack::PopAndDestroy(pName);
        pName = NULL;
    }
 
    cpPriv->state = QNetworkConfiguration::Defined;
    TBool isConnected = connectionMethod.GetBoolAttributeL(CMManager::ECmConnected);
    if (isConnected) {
        cpPriv->state = QNetworkConfiguration::Active;
    }
    
    cpPriv->isValid = true;
    cpPriv->id = ident;
    cpPriv->numericId = iapId;
    cpPriv->connectionId = 0;
    cpPriv->type = QNetworkConfiguration::InternetAccessPoint;
    cpPriv->purpose = QNetworkConfiguration::UnknownPurpose;
    cpPriv->roamingSupported = false;
    return cpPriv;
}
#else
bool SymbianEngine::readNetworkConfigurationValuesFromCommsDb(
        TUint32 aApId, SymbianNetworkConfigurationPrivate *apNetworkConfiguration)
{
    TRAPD(error, readNetworkConfigurationValuesFromCommsDbL(aApId,apNetworkConfiguration));
    if (error != KErrNone) {
        return false;        
    }
    return true;
}

void SymbianEngine::readNetworkConfigurationValuesFromCommsDbL(
        TUint32 aApId, SymbianNetworkConfigurationPrivate *apNetworkConfiguration)
{
    CApDataHandler* pDataHandler = CApDataHandler::NewLC(*ipCommsDB); 
    CApAccessPointItem* pAPItem = CApAccessPointItem::NewLC(); 
    TBuf<KCommsDbSvrMaxColumnNameLength> name;
    
    CApUtils* pApUtils = CApUtils::NewLC(*ipCommsDB);
    TUint32 apId = pApUtils->WapIdFromIapIdL(aApId);
    
    pDataHandler->AccessPointDataL(apId,*pAPItem);
    pAPItem->ReadTextL(EApIapName, name);
    if (name.Compare(_L("Easy WLAN")) == 0) {
        // "Easy WLAN" won't be accepted to the Configurations list
        User::Leave(KErrNotFound);
    }
    
    QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(aApId));
    
    QT_TRYCATCH_LEAVING(apNetworkConfiguration->name = QString::fromUtf16(name.Ptr(),name.Length()));
    apNetworkConfiguration->isValid = true;
    apNetworkConfiguration->id = ident;
    apNetworkConfiguration->numericId = aApId;
    apNetworkConfiguration->connectionId = 0;
    apNetworkConfiguration->state = (QNetworkConfiguration::Defined);
    apNetworkConfiguration->type = QNetworkConfiguration::InternetAccessPoint;
    apNetworkConfiguration->purpose = QNetworkConfiguration::UnknownPurpose;
    apNetworkConfiguration->roamingSupported = false;
    switch (pAPItem->BearerTypeL()) {
    case EApBearerTypeCSD:      
        apNetworkConfiguration->bearerType = QNetworkConfiguration::Bearer2G;
        break;
    case EApBearerTypeGPRS:
        apNetworkConfiguration->bearerType = QNetworkConfiguration::Bearer2G;
        break;
    case EApBearerTypeHSCSD:
        apNetworkConfiguration->bearerType = QNetworkConfiguration::BearerHSPA;
        break;
    case EApBearerTypeCDMA:
        apNetworkConfiguration->bearerType = QNetworkConfiguration::BearerCDMA2000;
        break;
    case EApBearerTypeWLAN:
        apNetworkConfiguration->bearerType = QNetworkConfiguration::BearerWLAN;
        break;
    case EApBearerTypeLAN:
        apNetworkConfiguration->bearerType = QNetworkConfiguration::BearerEthernet;
        break;
    case EApBearerTypeLANModem:
        apNetworkConfiguration->bearerType = QNetworkConfiguration::BearerEthernet;
        break;
    default:
        apNetworkConfiguration->bearerType = QNetworkConfiguration::BearerUnknown;
        break;
    }
    
    CleanupStack::PopAndDestroy(pApUtils);
    CleanupStack::PopAndDestroy(pAPItem);
    CleanupStack::PopAndDestroy(pDataHandler);
}
#endif

QNetworkConfigurationPrivatePointer SymbianEngine::defaultConfiguration()
{
    QMutexLocker locker(&mutex);

    return defaultConfig;
}

QStringList SymbianEngine::accessPointConfigurationIdentifiers()
{
    QMutexLocker locker(&mutex);

    return accessPointConfigurations.keys();
}

QNetworkConfigurationPrivatePointer SymbianEngine::defaultConfigurationL()
{
    QNetworkConfigurationPrivatePointer ptr;

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    // Check Default Connection (SNAP or IAP)
    TCmDefConnValue defaultConnectionValue;
    iCmManager.ReadDefConnL(defaultConnectionValue);
    if (defaultConnectionValue.iType == ECmDefConnDestination) {
        QString iface = QT_BEARERMGMT_CONFIGURATION_SNAP_PREFIX +
                        QString::number(qHash(defaultConnectionValue.iId));
        ptr = snapConfigurations.value(iface);
    } else if (defaultConnectionValue.iType == ECmDefConnConnectionMethod) {
        QString iface = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX +
                        QString::number(qHash(defaultConnectionValue.iId));
        ptr = accessPointConfigurations.value(iface);
    }
#endif
    
    if (ptr) {
        QMutexLocker configLocker(&ptr->mutex);
        if (ptr->isValid)
            return ptr;
    }

    QString iface = QString::number(qHash(KUserChoiceIAPId));
    return userChoiceConfigurations.value(iface);
}

void SymbianEngine::updateActiveAccessPoints()
{
    bool online = false;
    QList<QString> inactiveConfigs = accessPointConfigurations.keys();

    TRequestStatus status;
    TUint connectionCount;
    iConnectionMonitor.GetConnectionCount(connectionCount, status);
    User::WaitForRequest(status);
    
    // Go through all connections and set state of related IAPs to Active.
    // Status needs to be checked carefully, because ConnMon lists also e.g.
    // WLAN connections that are being currently tried --> we don't want to
    // state these as active.
    TUint connectionId;
    TUint subConnectionCount;
    TUint apId;
    TInt connectionStatus;
    if (status.Int() == KErrNone) {
        for (TUint i = 1; i <= connectionCount; i++) {
            iConnectionMonitor.GetConnectionInfo(i, connectionId, subConnectionCount);
            iConnectionMonitor.GetUintAttribute(connectionId, subConnectionCount, KIAPId, apId, status);
            User::WaitForRequest(status);
            QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(apId));
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(ident);
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
            if (!ptr) {
                // If IAP was not found, check if the update was about EasyWLAN
                ptr = configurationFromEasyWlan(apId, connectionId);
                // Change the ident correspondingly
                if (ptr) {
                    ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX +
                            QString::number(qHash(toSymbianConfig(ptr)->numericIdentifier()));
                }
            }
#endif
            if (ptr) {
                iConnectionMonitor.GetIntAttribute(connectionId, subConnectionCount, KConnectionStatus, connectionStatus, status);
                User::WaitForRequest(status);

                if (connectionStatus == KLinkLayerOpen) {
                    online = true;
                    inactiveConfigs.removeOne(ident);

                    ptr->mutex.lock();
                    toSymbianConfig(ptr)->connectionId = connectionId;
                    ptr->mutex.unlock();

                    // Configuration is Active
                    changeConfigurationStateTo(ptr, QNetworkConfiguration::Active);
                }
            }
        }
    }

    // Make sure that state of rest of the IAPs won't be Active
    foreach (const QString &iface, inactiveConfigs) {
        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(iface);
        if (ptr) {
            // Configuration is either Defined or Discovered
            changeConfigurationStateAtMaxTo(ptr, QNetworkConfiguration::Discovered);
        }
    }

    if (iOnline != online) {
        iOnline = online;
        mutex.unlock();
        emit this->onlineStateChanged(online);
        mutex.lock();
    }
}

void SymbianEngine::updateAvailableAccessPoints()
{
    if (!ipAccessPointsAvailabilityScanner) {
        ipAccessPointsAvailabilityScanner = new AccessPointsAvailabilityScanner(*this, iConnectionMonitor);
    }
    if (ipAccessPointsAvailabilityScanner) {
        // Scanning may take a while because WLAN scanning will be done (if device supports WLAN).
        ipAccessPointsAvailabilityScanner->StartScanning();
    }
}

void SymbianEngine::accessPointScanningReady(TBool scanSuccessful, TConnMonIapInfo iapInfo)
{
    iUpdateGoingOn = false;
    if (scanSuccessful) {
        QList<QString> unavailableConfigs = accessPointConfigurations.keys();
        
        // Set state of returned IAPs to Discovered
        // if state is not already Active
        for(TUint i=0; i<iapInfo.iCount; i++) {
            QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX +
                            QString::number(qHash(iapInfo.iIap[i].iIapId));
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(ident);
            if (ptr) {
                unavailableConfigs.removeOne(ident);

                QMutexLocker configLocker(&ptr->mutex);
                if (ptr->state < QNetworkConfiguration::Active) {
                    // Configuration is either Discovered or Active
                    changeConfigurationStateAtMinTo(ptr, QNetworkConfiguration::Discovered);
                }
            }
        }
        
        // Make sure that state of rest of the IAPs won't be Active
        foreach (const QString &iface, unavailableConfigs) {
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(iface);
            if (ptr) {
                // Configuration is Defined
                changeConfigurationStateAtMaxTo(ptr, QNetworkConfiguration::Discovered);
            }
        }
    }

    updateStatesToSnaps();
    
    if (!iFirstUpdate) {
        startCommsDatabaseNotifications();
        mutex.unlock();
        emit updateCompleted();
        mutex.lock();
    } else {
        iFirstUpdate = false;
        if (iScanInQueue) {
            iScanInQueue = EFalse;
            updateAvailableAccessPoints();
        }
    }
}

void SymbianEngine::updateStatesToSnaps()
{
    // Go through SNAPs and set correct state to SNAPs
    foreach (const QString &iface, snapConfigurations.keys()) {
        bool discovered = false;
        bool active = false;
        QNetworkConfigurationPrivatePointer ptr = snapConfigurations.value(iface);

        QMutexLocker snapConfigLocker(&ptr->mutex);

        // => Check if one of the IAPs of the SNAP is discovered or active
        //    => If one of IAPs is active, also SNAP is active
        //    => If one of IAPs is discovered but none of the IAPs is active, SNAP is discovered
        QMapIterator<unsigned int, QNetworkConfigurationPrivatePointer> i(ptr->serviceNetworkMembers);
        while (i.hasNext()) {
            i.next();

            const QNetworkConfigurationPrivatePointer child = i.value();

            QMutexLocker configLocker(&child->mutex);

            if ((child->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
                active = true;
                break;
            } else if ((child->state & QNetworkConfiguration::Discovered) ==
                       QNetworkConfiguration::Discovered) {
                discovered = true;
            }
        }
        snapConfigLocker.unlock();
        if (active) {
            changeConfigurationStateTo(ptr, QNetworkConfiguration::Active);
        } else if (discovered) {
            changeConfigurationStateTo(ptr, QNetworkConfiguration::Discovered);
        } else {
            changeConfigurationStateTo(ptr, QNetworkConfiguration::Defined);
        }
    }    
}

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
void SymbianEngine::updateMobileBearerToConfigs(TConnMonBearerInfo bearerInfo)
{
    QHash<QString, QNetworkConfigurationPrivatePointer>::const_iterator i =
        accessPointConfigurations.constBegin();
    while (i != accessPointConfigurations.constEnd()) {
        QNetworkConfigurationPrivatePointer ptr = i.value();

        QMutexLocker locker(&ptr->mutex);

        SymbianNetworkConfigurationPrivate *p = toSymbianConfig(ptr);

        if (p->bearerType >= QNetworkConfiguration::Bearer2G &&
            p->bearerType <= QNetworkConfiguration::BearerHSPA) {
            switch (bearerInfo) {
            case EBearerInfoCSD:
                p->bearerType = QNetworkConfiguration::Bearer2G;
                break;
            case EBearerInfoWCDMA:
                p->bearerType = QNetworkConfiguration::BearerWCDMA;
                break;
            case EBearerInfoCDMA2000:
                p->bearerType = QNetworkConfiguration::BearerCDMA2000;
                break;
            case EBearerInfoGPRS:
                p->bearerType = QNetworkConfiguration::Bearer2G;
                break;
            case EBearerInfoHSCSD:
                p->bearerType = QNetworkConfiguration::Bearer2G;
                break;
            case EBearerInfoEdgeGPRS:
                p->bearerType = QNetworkConfiguration::Bearer2G;
                break;
            case EBearerInfoWcdmaCSD:
                p->bearerType = QNetworkConfiguration::BearerWCDMA;
                break;
            case EBearerInfoHSDPA:
                p->bearerType = QNetworkConfiguration::BearerHSPA;
                break;
            case EBearerInfoHSUPA:
                p->bearerType = QNetworkConfiguration::BearerHSPA;
                break;
            case EBearerInfoHSxPA:
                p->bearerType = QNetworkConfiguration::BearerHSPA;
                break;
            }
        }

        ++i;
    }
}
#endif

bool SymbianEngine::changeConfigurationStateTo(QNetworkConfigurationPrivatePointer ptr,
                                               QNetworkConfiguration::StateFlags newState)
{
    ptr->mutex.lock();
    if (newState != ptr->state) {
        ptr->state = newState;
        ptr->mutex.unlock();

        mutex.unlock();
        emit configurationChanged(ptr);
        mutex.lock();

        return true;
    } else {
        ptr->mutex.unlock();
    }
    return false;
}

/* changeConfigurationStateAtMinTo function does not overwrite possible better
 * state (e.g. Discovered state does not overwrite Active state) but
 * makes sure that state is at minimum given state.
*/
bool SymbianEngine::changeConfigurationStateAtMinTo(QNetworkConfigurationPrivatePointer ptr,
                                                    QNetworkConfiguration::StateFlags newState)
{
    ptr->mutex.lock();
    if ((newState | ptr->state) != ptr->state) {
        ptr->state = (ptr->state | newState);
        ptr->mutex.unlock();

        mutex.unlock();
        emit configurationChanged(ptr);
        mutex.lock();

        return true;
    } else {
        ptr->mutex.unlock();
    }
    return false;
}

/* changeConfigurationStateAtMaxTo function overwrites possible better
 * state (e.g. Discovered state overwrites Active state) and
 * makes sure that state is at maximum given state (e.g. Discovered state
 * does not overwrite Defined state).
*/
bool SymbianEngine::changeConfigurationStateAtMaxTo(QNetworkConfigurationPrivatePointer ptr,
                                                    QNetworkConfiguration::StateFlags newState)
{
    ptr->mutex.lock();
    if ((newState & ptr->state) != ptr->state) {
        ptr->state = (newState & ptr->state);
        ptr->mutex.unlock();

        mutex.unlock();
        emit configurationChanged(ptr);
        mutex.lock();

        return true;
    } else {
        ptr->mutex.unlock();
    }
    return false;
}

void SymbianEngine::startCommsDatabaseNotifications()
{
    if (!iWaitingCommsDatabaseNotifications) {
        iWaitingCommsDatabaseNotifications = ETrue;
        if (!IsActive()) {
            SetActive();
            // Start waiting for new notification
            ipCommsDB->RequestNotification(iStatus);
        }
    }
}

void SymbianEngine::stopCommsDatabaseNotifications()
{
    if (iWaitingCommsDatabaseNotifications) {
        iWaitingCommsDatabaseNotifications = EFalse;
        Cancel();
    }
}

void SymbianEngine::RunL()
{
    QMutexLocker locker(&mutex);

    if (iStatus != KErrCancel) {
        // By default, start relistening notifications. Stop only if interesting event occurred.
        iWaitingCommsDatabaseNotifications = true;
        RDbNotifier::TEvent event = STATIC_CAST(RDbNotifier::TEvent, iStatus.Int());
        switch (event) {
        case RDbNotifier::ECommit:   /** A transaction has been committed.  */
        case RDbNotifier::ERecover:  /** The database has been recovered    */
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
            qDebug("QNCM CommsDB event (of type RDbNotifier::TEvent) received: %d", iStatus.Int());
#endif
            // Mark that there is update pending. No need to ask more events,
            // as we know we will be updating anyway when the timer expires.
            if (!iUpdatePending) {
                iUpdatePending = true;
                iWaitingCommsDatabaseNotifications = false;
                // Update after random time, so that many processes won't
                // start updating simultaneously
                updateConfigurationsAfterRandomTime();
            }
            break;
        default:
            // Do nothing
            break;
        }
    }

    if (iWaitingCommsDatabaseNotifications) {
        if (!IsActive()) {
            SetActive();
            // Start waiting for new notification
            ipCommsDB->RequestNotification(iStatus);
        }
    }
}

void SymbianEngine::DoCancel()
{
    QMutexLocker locker(&mutex);

    ipCommsDB->CancelRequestNotification();
}

void SymbianEngine::EventL(const CConnMonEventBase& aEvent)
{
    QMutexLocker locker(&mutex);

    switch (aEvent.EventType()) {
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    case EConnMonBearerInfoChange:
        {
        CConnMonBearerInfoChange* realEvent;
        realEvent = (CConnMonBearerInfoChange*) &aEvent;
        TUint connectionId = realEvent->ConnectionId();
        if (connectionId == EBearerIdAll) {
            //Network level event
            TConnMonBearerInfo bearerInfo = (TConnMonBearerInfo)realEvent->BearerInfo();
            updateMobileBearerToConfigs(bearerInfo);
        }
        break;
        }
#endif
    case EConnMonConnectionStatusChange:
        {
        CConnMonConnectionStatusChange* realEvent;
        realEvent = (CConnMonConnectionStatusChange*) &aEvent;
        TInt connectionStatus = realEvent->ConnectionStatus();
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
        qDebug() << "QNCM Connection status : " << QString::number(connectionStatus) << " , connection monitor Id : " << realEvent->ConnectionId();
#endif
        if (connectionStatus == KConfigDaemonStartingRegistration) {
            TUint connectionId = realEvent->ConnectionId();
            TUint subConnectionCount = 0;
            TUint apId;            
            TRequestStatus status;
            iConnectionMonitor.GetUintAttribute(connectionId, subConnectionCount, KIAPId, apId, status);
            User::WaitForRequest(status);

            QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(apId));
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(ident);
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
            if (!ptr) {
                // Check if status was regarding EasyWLAN
                ptr = configurationFromEasyWlan(apId, connectionId);
            }
#endif
            if (ptr) {
                ptr->mutex.lock();
                toSymbianConfig(ptr)->connectionId = connectionId;
                ptr->mutex.unlock();
                QT_TRYCATCH_LEAVING(
                    emit configurationStateChanged(toSymbianConfig(ptr)->numericIdentifier(),
                                                   connectionId, QNetworkSession::Connecting)
                );
            }
        } else if (connectionStatus == KLinkLayerOpen) {
            // Connection has been successfully opened
            TUint connectionId = realEvent->ConnectionId();
            TUint subConnectionCount = 0;
            TUint apId;            
            TRequestStatus status;
            iConnectionMonitor.GetUintAttribute(connectionId, subConnectionCount, KIAPId, apId, status);
            User::WaitForRequest(status);
            QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(apId));
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(ident);
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
            if (!ptr) {
                // Check for EasyWLAN
                ptr = configurationFromEasyWlan(apId, connectionId);
            }
#endif
            if (ptr) {
                ptr->mutex.lock();
                toSymbianConfig(ptr)->connectionId = connectionId;
                ptr->mutex.unlock();

                // Configuration is Active
                QT_TRYCATCH_LEAVING(
                    if (changeConfigurationStateTo(ptr, QNetworkConfiguration::Active)) {
                        updateStatesToSnaps();
                    }
                    emit configurationStateChanged(toSymbianConfig(ptr)->numericIdentifier(),
                                                   connectionId, QNetworkSession::Connected);

                    if (!iOnline) {
                        iOnline = true;
                        emit this->onlineStateChanged(iOnline);
                    }
                );
            }
        } else if (connectionStatus == KConfigDaemonStartingDeregistration) {
            TUint connectionId = realEvent->ConnectionId();
            QNetworkConfigurationPrivatePointer ptr = dataByConnectionId(connectionId);
            if (ptr) {
                QT_TRYCATCH_LEAVING(
                    emit configurationStateChanged(toSymbianConfig(ptr)->numericIdentifier(),
                                                   connectionId, QNetworkSession::Closing)
                );
            }
        } else if (connectionStatus == KLinkLayerClosed ||
                   connectionStatus == KConnectionClosed) {
            // Connection has been closed. Which of the above events is reported, depends on the Symbian
            // platform.
            TUint connectionId = realEvent->ConnectionId();
            QNetworkConfigurationPrivatePointer ptr = dataByConnectionId(connectionId);
            if (ptr) {
                // Configuration is either Defined or Discovered
                QT_TRYCATCH_LEAVING(
                    if (changeConfigurationStateAtMaxTo(ptr, QNetworkConfiguration::Discovered)) {
                        updateStatesToSnaps();
                    }
                    emit configurationStateChanged(toSymbianConfig(ptr)->numericIdentifier(),
                                                   connectionId, QNetworkSession::Disconnected);
                );
            }
            
            bool online = false;
            foreach (const QString &iface, accessPointConfigurations.keys()) {
                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(iface);
                QMutexLocker configLocker(&ptr->mutex);
                if (ptr->state == QNetworkConfiguration::Active) {
                    online = true;
                    break;
                }
            }
            if (iOnline != online) {
                iOnline = online;
                QT_TRYCATCH_LEAVING(emit this->onlineStateChanged(iOnline));
            }
        }
        }
        break;    

    case EConnMonIapAvailabilityChange:
        {
        CConnMonIapAvailabilityChange* realEvent;
        realEvent = (CConnMonIapAvailabilityChange*) &aEvent;
        TConnMonIapInfo iaps = realEvent->IapAvailability();
        QList<QString> unDiscoveredConfigs = accessPointConfigurations.keys();
        for ( TUint i = 0; i < iaps.Count(); i++ ) {
            QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX +
                            QString::number(qHash(iaps.iIap[i].iIapId));

            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(ident);
            if (ptr) {
                // Configuration is either Discovered or Active 
                QT_TRYCATCH_LEAVING(changeConfigurationStateAtMinTo(ptr, QNetworkConfiguration::Discovered));
                unDiscoveredConfigs.removeOne(ident);
            }
        }
        foreach (const QString &iface, unDiscoveredConfigs) {
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(iface);
            if (ptr) {
                // Configuration is Defined
                QT_TRYCATCH_LEAVING(changeConfigurationStateAtMaxTo(ptr, QNetworkConfiguration::Defined));
            }
        }
        // Something has in IAPs, update states to SNAPs
        updateStatesToSnaps();
        }
        break;

    case EConnMonCreateConnection:
        {
        // This event is caught to keep connection monitor IDs up-to-date.
        CConnMonCreateConnection* realEvent;
        realEvent = (CConnMonCreateConnection*) &aEvent;
        TUint subConnectionCount = 0;
        TUint apId;
        TUint connectionId = realEvent->ConnectionId();
        TRequestStatus status;
        iConnectionMonitor.GetUintAttribute(connectionId, subConnectionCount, KIAPId, apId, status);
        User::WaitForRequest(status);
        QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX+QString::number(qHash(apId));
        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(ident);
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
        if (!ptr) {
            // If IAP was not found, check if the update was about EasyWLAN
            ptr = configurationFromEasyWlan(apId, connectionId);
        }
#endif
        if (ptr) {
            QMutexLocker configLocker(&ptr->mutex);
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
            qDebug() << "QNCM updating connection monitor ID : from, to, whose: " << toSymbianConfig(ptr)->connectionId << connectionId << ptr->name;
#endif
            toSymbianConfig(ptr)->connectionId = connectionId;
        }
        }
        break;
    default:
        // For unrecognized events
        break;
    }
}

/*
    Returns the network configuration that matches the given SSID.
*/
QNetworkConfigurationPrivatePointer SymbianEngine::configurationFromSsid(const QString &ssid)
{
    QMutexLocker locker(&mutex);

    // Browser through all items and check their name for match
    QHash<QString, QNetworkConfigurationPrivatePointer>::ConstIterator i =
        accessPointConfigurations.constBegin();
    while (i != accessPointConfigurations.constEnd()) {
        QNetworkConfigurationPrivatePointer ptr = i.value();

        QMutexLocker configLocker(&ptr->mutex);

        if (ptr->name == ssid) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
            qDebug() << "QNCM EasyWlan uses real SSID: " << ssid;
#endif
            return ptr;
        }
        ++i;
    }

    return QNetworkConfigurationPrivatePointer();
}

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
// Tries to derive configuration from EasyWLAN.
// First checks if the interface brought up was EasyWLAN, then derives the real SSID,
// and looks up configuration based on that one.
QNetworkConfigurationPrivatePointer SymbianEngine::configurationFromEasyWlan(TUint32 apId,
                                                                             TUint connectionId)
{
    if (apId == iCmManager.EasyWlanIdL()) {
        TRequestStatus status;
        TBuf<50> easyWlanNetworkName;
        iConnectionMonitor.GetStringAttribute( connectionId, 0, KNetworkName,
                                               easyWlanNetworkName, status );
        User::WaitForRequest(status);
        if (status.Int() == KErrNone) {
            const QString realSSID = QString::fromUtf16(easyWlanNetworkName.Ptr(),
                                                        easyWlanNetworkName.Length());

            // Browser through all items and check their name for match
            QHash<QString, QNetworkConfigurationPrivatePointer>::ConstIterator i =
                accessPointConfigurations.constBegin();
            while (i != accessPointConfigurations.constEnd()) {
                QNetworkConfigurationPrivatePointer ptr = i.value();

                QMutexLocker configLocker(&ptr->mutex);

                if (ptr->name == realSSID) {
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
                    qDebug() << "QNCM EasyWlan uses real SSID: " << realSSID;
#endif
                    return ptr;
                }
                ++i;
            }
        }
    }
    return QNetworkConfigurationPrivatePointer();
}
#endif

// Sessions may use this function to report configuration state changes,
// because on some Symbian platforms (especially Symbian^3) all state changes are not
// reported by the RConnectionMonitor, in particular in relation to stop() call,
// whereas they _are_ reported on RConnection progress notifier used by sessions --> centralize
// this data here so that other sessions may benefit from it too (not all sessions necessarily have
// RConnection progress notifiers available but they relay on having e.g. disconnected information from
// manager). Currently only 'Disconnected' state is of interest because it has proven to be troublesome.
void SymbianEngine::configurationStateChangeReport(TUint32 accessPointId, QNetworkSession::State newState)
{
    QMutexLocker locker(&mutex);

#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug() << "QNCM A session reported state change for IAP ID: " << accessPointId << " whose new state is: " << newState;
#endif
    switch (newState) {
    case QNetworkSession::Disconnected:
        {
            QString ident = QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX +
                            QString::number(qHash(accessPointId));
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(ident);
            if (ptr) {
                // Configuration is either Defined or Discovered
                if (changeConfigurationStateAtMaxTo(ptr, QNetworkConfiguration::Discovered)) {
                    updateStatesToSnaps();
                }

                locker.unlock();
                emit configurationStateChanged(toSymbianConfig(ptr)->numericIdentifier(),
                                               toSymbianConfig(ptr)->connectionIdentifier(),
                                               QNetworkSession::Disconnected);
                locker.relock();
            }
        }
        break;
    default:
        break;
    }
}

// Waits for 2..6 seconds.
void SymbianEngine::updateConfigurationsAfterRandomTime()
{
    int iTimeToWait = qMax(1000, (qAbs(qrand()) % 68) * 100);
#ifdef QT_BEARERMGMT_SYMBIAN_DEBUG
    qDebug("QNCM waiting random time: %d ms", iTimeToWait);
#endif
    QTimer::singleShot(iTimeToWait, this, SLOT(delayedConfigurationUpdate()));
}

QNetworkConfigurationPrivatePointer SymbianEngine::dataByConnectionId(TUint aConnectionId)
{
    QNetworkConfiguration item;
    QHash<QString, QNetworkConfigurationPrivatePointer>::const_iterator i =
            accessPointConfigurations.constBegin();
    while (i != accessPointConfigurations.constEnd()) {
        QNetworkConfigurationPrivatePointer ptr = i.value();
        if (toSymbianConfig(ptr)->connectionIdentifier() == aConnectionId)
            return ptr;

        ++i;
    }

    return QNetworkConfigurationPrivatePointer();
}

AccessPointsAvailabilityScanner::AccessPointsAvailabilityScanner(SymbianEngine& owner,
                                                               RConnectionMonitor& connectionMonitor)
    : CActive(CActive::EPriorityHigh), iOwner(owner), iConnectionMonitor(connectionMonitor)
{
    CActiveScheduler::Add(this);  
}

AccessPointsAvailabilityScanner::~AccessPointsAvailabilityScanner()
{
    Cancel();
}

void AccessPointsAvailabilityScanner::DoCancel()
{
    iConnectionMonitor.CancelAsyncRequest(EConnMonGetPckgAttribute);
    iScanActive = EFalse;
    iOwner.iScanInQueue = EFalse;
}

void AccessPointsAvailabilityScanner::StartScanning()
{
    if (!iScanActive) {
        iScanActive = ETrue;
        if (iOwner.iFirstUpdate) {
            // On first update (the mgr is being instantiated) update only those bearers who
            // don't need time-consuming scans (WLAN).
            // Note: EBearerIdWCDMA covers also GPRS bearer
            iConnectionMonitor.GetPckgAttribute(EBearerIdWCDMA, 0, KIapAvailability, iIapBuf, iStatus);
        } else {
            iConnectionMonitor.GetPckgAttribute(EBearerIdAll, 0, KIapAvailability, iIapBuf, iStatus);
        }

        if (!IsActive()) {
            SetActive();
        }
    } else {
        // Queue scan for getting WLAN info after first request returns
        if (iOwner.iFirstUpdate) {
            iOwner.iScanInQueue = ETrue;
        }
    }
}

void AccessPointsAvailabilityScanner::RunL()
{
    QMutexLocker locker(&iOwner.mutex);

    iScanActive = EFalse;
    if (iStatus.Int() != KErrNone) {
        iIapBuf().iCount = 0;
        QT_TRYCATCH_LEAVING(iOwner.accessPointScanningReady(false,iIapBuf()));
    } else {
        QT_TRYCATCH_LEAVING(iOwner.accessPointScanningReady(true,iIapBuf()));
    }
}

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT
