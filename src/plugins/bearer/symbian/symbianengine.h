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

#ifndef SYMBIANENGINE_H
#define SYMBIANENGINE_H

#include <QtCore/qstringlist.h>
#include <QtNetwork/private/qbearerengine_p.h>
#include <QtNetwork/qnetworkconfigmanager.h>

#include <QHash>
#include <rconnmon.h>
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    #include <cmmanager.h>
#endif

// Uncomment and compile QtBearer to gain detailed state tracing
// #define QT_BEARERMGMT_SYMBIAN_DEBUG

#define QT_BEARERMGMT_CONFIGURATION_SNAP_PREFIX QLatin1String("S_")
#define QT_BEARERMGMT_CONFIGURATION_IAP_PREFIX  QLatin1String("I_")

class CCommsDatabase;
class QEventLoop;

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE

class QNetworkSessionPrivate;
class AccessPointsAvailabilityScanner;

class SymbianNetworkConfigurationPrivate : public QNetworkConfigurationPrivate
{
public:
    SymbianNetworkConfigurationPrivate();
    ~SymbianNetworkConfigurationPrivate();

    inline TUint32 numericIdentifier() const
    {
        QMutexLocker locker(&mutex);
        return numericId;
    }

    inline TUint connectionIdentifier() const
    {
        QMutexLocker locker(&mutex);
        return connectionId;
    }

    inline QString configMappingName() const
    {
        QMutexLocker locker(&mutex);
        return mappingName;
    }

    QString mappingName;

    // So called IAP id from the platform. Remains constant as long as the
    // platform is aware of the configuration ie. it is stored in the databases
    // --> does not depend on whether connections are currently open or not.
    // In practice is the same for the lifetime of the QNetworkConfiguration.
    TUint32 numericId;
    // So called connection id, or connection monitor ID. A dynamic ID assigned
    // by RConnectionMonitor whenever a new connection is opened. ConnectionID and
    // numericId/IAP id have 1-to-1 mapping during the lifetime of the connection at
    // connection monitor. Notably however it changes whenever a new connection to
    // a given IAP is created. In a sense it is constant during the time the
    // configuration remains between states Discovered..Active..Discovered, do not
    // however relay on this.
    TUint connectionId;
};

inline SymbianNetworkConfigurationPrivate *toSymbianConfig(QNetworkConfigurationPrivatePointer ptr)
{
    return static_cast<SymbianNetworkConfigurationPrivate *>(ptr.data());
}

class SymbianEngine : public QBearerEngine, public CActive,
                      public MConnectionMonitorObserver
{
    Q_OBJECT

public:
    SymbianEngine(QObject *parent = 0);
    virtual ~SymbianEngine();

    bool hasIdentifier(const QString &id);

    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate();

    QNetworkConfigurationManager::Capabilities capabilities() const;

    QNetworkSessionPrivate *createSessionBackend();

    QNetworkConfigurationPrivatePointer defaultConfiguration();

    QStringList accessPointConfigurationIdentifiers();

    QNetworkConfigurationPrivatePointer configurationFromSsid(const QString &ssid);

    // For QNetworkSessionPrivateImpl to indicate about state changes
    void configurationStateChangeReport(TUint32 accessPointId, QNetworkSession::State newState);

Q_SIGNALS:
    void onlineStateChanged(bool isOnline);
    
    void configurationStateChanged(quint32 accessPointId, quint32 connMonId,
                                   QNetworkSession::State newState);
    
public Q_SLOTS:
    void updateConfigurations();
    void delayedConfigurationUpdate();

private:
    void updateStatesToSnaps();
    bool changeConfigurationStateTo(QNetworkConfigurationPrivatePointer ptr,
                                    QNetworkConfiguration::StateFlags newState);
    bool changeConfigurationStateAtMinTo(QNetworkConfigurationPrivatePointer ptr,
                                         QNetworkConfiguration::StateFlags newState);
    bool changeConfigurationStateAtMaxTo(QNetworkConfigurationPrivatePointer ptr,
                                         QNetworkConfiguration::StateFlags newState);
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    void updateMobileBearerToConfigs(TConnMonBearerInfo bearerInfo);
    SymbianNetworkConfigurationPrivate *configFromConnectionMethodL(RCmConnectionMethod& connectionMethod);
#else
    bool readNetworkConfigurationValuesFromCommsDb(
            TUint32 aApId, SymbianNetworkConfigurationPrivate *apNetworkConfiguration);
    void readNetworkConfigurationValuesFromCommsDbL(
            TUint32 aApId, SymbianNetworkConfigurationPrivate *apNetworkConfiguration);
#endif    
    
    void updateConfigurationsL();
    void updateActiveAccessPoints();
    void updateAvailableAccessPoints();
    void accessPointScanningReady(TBool scanSuccessful, TConnMonIapInfo iapInfo);
    void startCommsDatabaseNotifications();
    void stopCommsDatabaseNotifications();
    void updateConfigurationsAfterRandomTime();

    QNetworkConfigurationPrivatePointer defaultConfigurationL();
    TBool GetS60PlatformVersion(TUint& aMajor, TUint& aMinor) const;
    void startMonitoringIAPData(TUint32 aIapId);
    QNetworkConfigurationPrivatePointer dataByConnectionId(TUint aConnectionId);

protected:
    // From CActive
    void RunL();
    void DoCancel();
    
private:
    // MConnectionMonitorObserver
    void EventL(const CConnMonEventBase& aEvent);
#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    QNetworkConfigurationPrivatePointer configurationFromEasyWlan(TUint32 apId,
                                                                  TUint connectionId);
#endif

private: // Data
    bool               iFirstUpdate; 
    CCommsDatabase*    ipCommsDB;
    RConnectionMonitor iConnectionMonitor;

    TBool              iWaitingCommsDatabaseNotifications;
    TBool              iOnline;
    TBool              iInitOk;
    TBool              iUpdateGoingOn;
    TBool              iUpdatePending;
    TBool              iScanInQueue;

    AccessPointsAvailabilityScanner* ipAccessPointsAvailabilityScanner;

    QNetworkConfigurationPrivatePointer defaultConfig;

    friend class QNetworkSessionPrivate;
    friend class AccessPointsAvailabilityScanner;

#ifdef SNAP_FUNCTIONALITY_AVAILABLE
    RCmManager iCmManager;
#endif
};

class AccessPointsAvailabilityScanner : public CActive
{
public:
    AccessPointsAvailabilityScanner(SymbianEngine& owner,
                                   RConnectionMonitor& connectionMonitor); 
    ~AccessPointsAvailabilityScanner();

    void StartScanning();
    
protected: // From CActive
    void RunL();
    void DoCancel();

private: // Data
    SymbianEngine&      iOwner;
    RConnectionMonitor& iConnectionMonitor;
    TConnMonIapInfoBuf  iIapBuf;
    TBool               iScanActive;
};

QT_END_NAMESPACE

#endif
