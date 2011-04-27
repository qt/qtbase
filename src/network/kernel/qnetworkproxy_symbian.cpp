/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the FOO module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/**
 * Some notes about the code:
 *
 * ** It is assumed that the system proxies are for url based requests
 *  ie. HTTP/HTTPS based.
 * ** It is assumed that proxies don't use authentication.
 * ** It is assumed that there is no exceptions to proxy use (Symbian side
 *  does have the field for it but it is not user modifiable by default).
 * ** There is no checking for protocol name.
 */

#include <QtNetwork/qnetworkproxy.h>

#ifndef QT_NO_NETWORKPROXY

#include <metadatabase.h> // CMDBSession
#include <commsdattypeinfov1_1.h> // CCDIAPRecord, CCDProxiesRecord
#include <commsdattypesv1_1.h> // KCDTIdIAPRecord, KCDTIdProxiesRecord
#include <QtNetwork/QNetworkConfigurationManager>
#include <QFlags>

using namespace CommsDat;

QT_BEGIN_NAMESPACE

class SymbianIapId
{
public:
    enum State{
        NotValid,
        Valid
    };
    Q_DECLARE_FLAGS(States, State)
    SymbianIapId() {}
    ~SymbianIapId() {}
    void setIapId(TUint32 iapId) { iapState |= Valid; id = iapId; }
    bool isValid() { return iapState == Valid; }
    TUint32 iapId() { return id; }
private:
    QFlags<States> iapState;
    TUint32 id;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SymbianIapId::States)

class SymbianProxyQuery
{
public:
    static QNetworkConfiguration findCurrentConfiguration(QNetworkConfigurationManager& configurationManager);
    static SymbianIapId getIapId(QNetworkConfigurationManager& configurationManager);
    static CCDIAPRecord *getIapRecordLC(TUint32 aIAPId, CMDBSession &aDb);
    static CMDBRecordSet<CCDProxiesRecord> *prepareQueryLC(TUint32 serviceId, TDesC& serviceType);
    static QList<QNetworkProxy> proxyQueryL(TUint32 aIAPId, const QNetworkProxyQuery &query);
};

QNetworkConfiguration SymbianProxyQuery::findCurrentConfiguration(QNetworkConfigurationManager& configurationManager)
{
    QList<QNetworkConfiguration> activeConfigurations = configurationManager.allConfigurations(
        QNetworkConfiguration::Active);
    QNetworkConfiguration currentConfig;
    if (activeConfigurations.count() > 0) {
        currentConfig = activeConfigurations.at(0);
    } else {
        // No active configurations, try default one
        QNetworkConfiguration defaultConfiguration = configurationManager.defaultConfiguration();
        if (defaultConfiguration.isValid()) {
            switch (defaultConfiguration.type()) {
            case QNetworkConfiguration::InternetAccessPoint:
                currentConfig = defaultConfiguration;
                break;
            case QNetworkConfiguration::ServiceNetwork:
            {
                // Note: This code assumes that the only unambigious way to
                // find current proxy config is if there is only one access point
                // or if the found access point is immediately usable.
                QList<QNetworkConfiguration> childConfigurations = defaultConfiguration.children();
                if (childConfigurations.count() == 1) {
                    currentConfig = childConfigurations.at(0);
                } else {
                    for (int index = 0; index < childConfigurations.count(); index++) {
                        QNetworkConfiguration childConfig = childConfigurations.at(index);
                        if (childConfig.isValid() && childConfig.state() == QNetworkConfiguration::Discovered) {
                            currentConfig = childConfig;
                            break;
                        }
                    }
                }
            }
                break;
            case QNetworkConfiguration::UserChoice:
                // User choice is not a valid configuration for proxy discovery
                break;
            }
        }
    }
    return currentConfig;
}

SymbianIapId SymbianProxyQuery::getIapId(QNetworkConfigurationManager& configurationManager)
{
    SymbianIapId iapId;

    QNetworkConfiguration currentConfig = findCurrentConfiguration(configurationManager);
    if (currentConfig.isValid()) {
        // Note: the following code assumes that the identifier is in format
        // I_xxxx where xxxx is the identifier of IAP. This is meant as a
        // temporary solution until there is a support for returning
        // implementation specific identifier.
        const int generalPartLength = 2;
        const int identifierNumberLength = currentConfig.identifier().length() - generalPartLength;
        QString idString(currentConfig.identifier().right(identifierNumberLength));
        bool success;
        uint id = idString.toUInt(&success);
        if (success)
            iapId.setIapId(id);
        else
            qWarning() << "Failed to convert identifier to access point identifier: "
                << currentConfig.identifier();
    }

    return iapId;
}

CCDIAPRecord *SymbianProxyQuery::getIapRecordLC(TUint32 aIAPId, CMDBSession &aDb)
{
    CCDIAPRecord *iap = static_cast<CCDIAPRecord*> (CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
    CleanupStack::PushL(iap);
    iap->SetRecordId(aIAPId);
    iap->LoadL(aDb);
    return iap;
}

CMDBRecordSet<CCDProxiesRecord> *SymbianProxyQuery::prepareQueryLC(TUint32 serviceId, TDesC& serviceType)
{
    // Create a recordset of type CCDProxiesRecord
    // for priming search.
    // This will ultimately contain record(s)
    // matching the priming record attributes
    CMDBRecordSet<CCDProxiesRecord> *proxyRecords = new (ELeave) CMDBRecordSet<CCDProxiesRecord> (
        KCDTIdProxiesRecord);
    CleanupStack::PushL(proxyRecords);

    CCDProxiesRecord *primingProxyRecord =
        static_cast<CCDProxiesRecord *> (CCDRecordBase::RecordFactoryL(KCDTIdProxiesRecord));
    CleanupStack::PushL(primingProxyRecord);

    primingProxyRecord->iServiceType.SetMaxLengthL(serviceType.Length());
    primingProxyRecord->iServiceType = serviceType;
    primingProxyRecord->iService = serviceId;
    primingProxyRecord->iUseProxyServer = ETrue;

    proxyRecords->iRecords.AppendL(primingProxyRecord);
    // Ownership of primingProxyRecord is transferred to
    // proxyRecords, just remove it from the CleanupStack
    CleanupStack::Pop(primingProxyRecord);
    return proxyRecords;
}

QList<QNetworkProxy> SymbianProxyQuery::proxyQueryL(TUint32 aIAPId, const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> foundProxies;
    if (query.queryType() != QNetworkProxyQuery::UrlRequest) {
        return foundProxies;
    }

    CMDBSession *iDb = CMDBSession::NewLC(KCDVersion1_1);
    CCDIAPRecord *iap = getIapRecordLC(aIAPId, *iDb);

    // Read service table id and service type
    // from the IAP record found
    TUint32 serviceId = iap->iService;
    RBuf serviceType;
    serviceType.CreateL(iap->iServiceType);
    CleanupStack::PopAndDestroy(iap);
    CleanupClosePushL(serviceType);

    CMDBRecordSet<CCDProxiesRecord> *proxyRecords = prepareQueryLC(serviceId, serviceType);

    // Now to find a proxy table matching our criteria
    if (proxyRecords->FindL(*iDb)) {
        TInt count = proxyRecords->iRecords.Count();
        for(TInt index = 0; index < count; index++) {
            CCDProxiesRecord *proxyRecord = static_cast<CCDProxiesRecord *> (proxyRecords->iRecords[index]);
            RBuf serverName;
            serverName.CreateL(proxyRecord->iServerName);
            CleanupClosePushL(serverName);
            if (serverName.Length() == 0)
                User::Leave(KErrNotFound);
            QString serverNameQt((const QChar*)serverName.Ptr(), serverName.Length());
            CleanupStack::Pop(); // serverName
            TUint32 port = proxyRecord->iPortNumber;

            QNetworkProxy proxy(QNetworkProxy::HttpProxy, serverNameQt, port);
            foundProxies.append(proxy);
        }
    }

    CleanupStack::PopAndDestroy(proxyRecords);
    CleanupStack::Pop(); // serviceType
    CleanupStack::PopAndDestroy(iDb);

    return foundProxies;
}

QList<QNetworkProxy> QNetworkProxyFactory::systemProxyForQuery(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxies;
    SymbianIapId iapId;
    TInt error;
    QNetworkConfigurationManager manager;
    iapId = SymbianProxyQuery::getIapId(manager);
    if (iapId.isValid()) {
        TRAP(error, proxies = SymbianProxyQuery::proxyQueryL(iapId.iapId(), query))
        if (error != KErrNone) {
            qWarning() << "Error while retrieving proxies: '" << error << '"';
            proxies.clear();
        }
    }
    proxies << QNetworkProxy::NoProxy;

    return proxies;
}

QT_END_NAMESPACE

#endif
