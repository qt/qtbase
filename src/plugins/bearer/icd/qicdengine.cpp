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

#include "qicdengine.h"
#include "qnetworksession_impl.h"

#include <wlancond.h>
#include <wlan-utils.h>
#include <iapconf.h>
#include <iapmonitor.h>

#ifndef QT_NO_BEARERMANAGEMENT

QT_BEGIN_NAMESPACE

IcdNetworkConfigurationPrivate::IcdNetworkConfigurationPrivate()
:   service_attrs(0), network_attrs(0)
{
}

IcdNetworkConfigurationPrivate::~IcdNetworkConfigurationPrivate()
{
}

QString IcdNetworkConfigurationPrivate::bearerTypeName() const
{
    QMutexLocker locker(&mutex);

    return iap_type;
}

/******************************************************************************/
/** IapAddTimer specific                                                      */
/******************************************************************************/

/* The IapAddTimer is a helper class that makes sure we update
 * the configuration only after all db additions to certain
 * iap are finished (after a certain timeout)
 */
class _IapAddTimer : public QObject
{
    Q_OBJECT

public:
    _IapAddTimer() {}
    ~_IapAddTimer()
    {
	if (timer.isActive()) {
	    QObject::disconnect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
	    timer.stop();
	}
    }

    void add(QString& iap_id, QIcdEngine *d);

    QString iap_id;
    QTimer timer;
    QIcdEngine *d;

public Q_SLOTS:
    void timeout();
};


void _IapAddTimer::add(QString& id, QIcdEngine *d_ptr)
{
    iap_id = id;
    d = d_ptr;

    if (timer.isActive()) {
	QObject::disconnect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
	timer.stop();
    }
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(timeout()));
    timer.start(1500);
}


void _IapAddTimer::timeout()
{
    d->addConfiguration(iap_id);
}


class IapAddTimer {
    QHash<QString, _IapAddTimer* > timers;

public:
    IapAddTimer() {}
    ~IapAddTimer() {}

    void add(QString& iap_id, QIcdEngine *d);
    void del(QString& iap_id);
    void removeAll();
};


void IapAddTimer::removeAll()
{
    QHashIterator<QString, _IapAddTimer* > i(timers);
    while (i.hasNext()) {
	i.next();
	_IapAddTimer *t = i.value();
	delete t;
    }
    timers.clear();
}


void IapAddTimer::add(QString& iap_id, QIcdEngine *d)
{
    if (timers.contains(iap_id)) {
	_IapAddTimer *iap = timers.value(iap_id);
	iap->add(iap_id, d);
    } else {
	_IapAddTimer *iap = new _IapAddTimer;
	iap->add(iap_id, d);
	timers.insert(iap_id, iap);
    }
}

void IapAddTimer::del(QString& iap_id)
{
    if (timers.contains(iap_id)) {
	_IapAddTimer *iap = timers.take(iap_id);
	delete iap;
    }
}

/******************************************************************************/
/** IAPMonitor specific                                                       */
/******************************************************************************/

class IapMonitor : public Maemo::IAPMonitor
{
public:
    IapMonitor() : first_call(true) { }

    void setup(QIcdEngine *d);
    void cleanup();

protected:
    void iapAdded(const QString &iapId);
    void iapRemoved(const QString &iapId);

private:
    bool first_call;

    QIcdEngine *d;
    IapAddTimer timers;
};

void IapMonitor::setup(QIcdEngine *d_ptr)
{
    if (first_call) {
	d = d_ptr;
	first_call = false;
    }
}


void IapMonitor::cleanup()
{
    if (!first_call) {
	timers.removeAll();
	first_call = true;
    }
}


void IapMonitor::iapAdded(const QString &iap_id)
{
    /* We cannot know when the IAP is fully added to db, so a timer is
     * installed instead. When the timer expires we hope that IAP is added ok.
     */
    QString id = iap_id;
    timers.add(id, d);
}


void IapMonitor::iapRemoved(const QString &iap_id)
{
    QString id = iap_id;
    d->deleteConfiguration(id);
}


/******************************************************************************/
/** QIcdEngine implementation                                                 */
/******************************************************************************/

QIcdEngine::QIcdEngine(QObject *parent)
:   QBearerEngine(parent), iapMonitor(0), m_dbusInterface(0), m_icdServiceWatcher(0),
    firstUpdate(true), m_scanGoingOn(false)
{
}

QIcdEngine::~QIcdEngine()
{
    cleanup();
    delete iapMonitor;
}

QNetworkConfigurationManager::Capabilities QIcdEngine::capabilities() const
{
    return QNetworkConfigurationManager::CanStartAndStopInterfaces |
           QNetworkConfigurationManager::DataStatistics |
           QNetworkConfigurationManager::ForcedRoaming |
           QNetworkConfigurationManager::NetworkSessionRequired;
}

bool QIcdEngine::ensureDBusConnection()
{
    if (m_dbusInterface)
        return true;

    // Setup DBus Interface for ICD
    m_dbusInterface = new QDBusInterface(ICD_DBUS_API_INTERFACE,
                                         ICD_DBUS_API_PATH,
                                         ICD_DBUS_API_INTERFACE,
                                         QDBusConnection::systemBus(),
                                         this);

    if (!m_dbusInterface->isValid()) {
        delete m_dbusInterface;
        m_dbusInterface = 0;

        if (!m_icdServiceWatcher) {
            m_icdServiceWatcher = new QDBusServiceWatcher(ICD_DBUS_API_INTERFACE,
                                                          QDBusConnection::systemBus(),
                                                          QDBusServiceWatcher::WatchForOwnerChange,
                                                          this);

            connect(m_icdServiceWatcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
                    this, SLOT(icdServiceOwnerChanged(QString,QString,QString)));
        }

        return false;
    }

    connect(&m_scanTimer, SIGNAL(timeout()), this, SLOT(finishAsyncConfigurationUpdate()));
    m_scanTimer.setSingleShot(true);

    /* Turn on IAP state monitoring */
    startListeningStateSignalsForAllConnections();

    /* Turn on IAP add/remove monitoring */
    iapMonitor = new IapMonitor;
    iapMonitor->setup(this);

    /* We create a default configuration which is a pseudo config */
    QNetworkConfigurationPrivate *cpPriv = new IcdNetworkConfigurationPrivate;
    cpPriv->name = "UserChoice";
    cpPriv->state = QNetworkConfiguration::Discovered;
    cpPriv->isValid = true;
    cpPriv->id = OSSO_IAP_ANY;
    cpPriv->type = QNetworkConfiguration::UserChoice;
    cpPriv->purpose = QNetworkConfiguration::UnknownPurpose;
    cpPriv->roamingSupported = false;

    QNetworkConfigurationPrivatePointer ptr(cpPriv);
    userChoiceConfigurations.insert(cpPriv->id, ptr);

    doRequestUpdate();

    getIcdInitialState();

    return true;
}

void QIcdEngine::initialize()
{
    QMutexLocker locker(&mutex);

    if (!ensureDBusConnection()) {
        locker.unlock();
        emit updateCompleted();
        locker.relock();
    }
}

static inline QString network_attrs_to_security(uint network_attrs)
{
    uint cap = 0;
    nwattr2cap(network_attrs, &cap); /* from libicd-network-wlan-dev.h */
    if (cap & WLANCOND_OPEN)
        return "NONE";
    else if (cap & WLANCOND_WEP)
        return "WEP";
    else if (cap & WLANCOND_WPA_PSK)
        return "WPA_PSK";
    else if (cap & WLANCOND_WPA_EAP)
        return "WPA_EAP";
    return "";
}


struct SSIDInfo {
    QString iap_id;
    QString wlan_security;
};


void QIcdEngine::deleteConfiguration(const QString &iap_id)
{
    QMutexLocker locker(&mutex);

    /* Called when IAPs are deleted in db, in this case we do not scan
     * or read all the IAPs from db because it might take too much power
     * (multiple applications would need to scan and read all IAPs from db)
     */
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.take(iap_id);
    if (ptr) {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug() << "IAP" << iap_id << "was removed from storage.";
#endif

        locker.unlock();
        emit configurationRemoved(ptr);
    } else {
#ifdef BEARER_MANAGEMENT_DEBUG
        qDebug("IAP: %s, already missing from the known list", iap_id.toAscii().data());
#endif
    }
}


static quint32 getNetworkAttrs(bool is_iap_id,
                               const QString &iap_id,
                               const QString &iap_type,
                               QString security_method)
{
    guint network_attr = 0;
    dbus_uint32_t cap = 0;

    if (iap_type == "WLAN_INFRA")
    cap |= WLANCOND_INFRA;
    else if (iap_type == "WLAN_ADHOC")
    cap |= WLANCOND_ADHOC;

    if (security_method.isEmpty() && (cap & (WLANCOND_INFRA | WLANCOND_ADHOC))) {
    Maemo::IAPConf saved_ap(iap_id);
    security_method = saved_ap.value("wlan_security").toString();
    }

    if (!security_method.isEmpty()) {
    if (security_method == "WEP")
        cap |= WLANCOND_WEP;
    else if (security_method == "WPA_PSK")
        cap |= WLANCOND_WPA_PSK;
    else if (security_method == "WPA_EAP")
        cap |= WLANCOND_WPA_EAP;
    else if (security_method == "NONE")
        cap |= WLANCOND_OPEN;

    if (cap & (WLANCOND_WPA_PSK | WLANCOND_WPA_EAP)) {
        Maemo::IAPConf saved_iap(iap_id);
        bool wpa2_only = saved_iap.value("EAP_wpa2_only_mode").toBool();
        if (wpa2_only) {
        cap |= WLANCOND_WPA2;
        }
    }
    }

    cap2nwattr(cap, &network_attr);
    if (is_iap_id)
    network_attr |= ICD_NW_ATTR_IAPNAME;

    return quint32(network_attr);
}


void QIcdEngine::addConfiguration(QString& iap_id)
{
    // Note: When new IAP is created, this function gets called multiple times
    //       in a row.
    //       For example: Empty type & name for WLAN was stored into newly
    //                    created IAP data in gconf when this function gets
    //                    called for the first time.
    //                    WLAN type & name are updated into IAP data in gconf
    //                    as soon as WLAN connection is up and running.
    //                    => And this function gets called again.

    QMutexLocker locker(&mutex);

    if (!accessPointConfigurations.contains(iap_id)) {
	Maemo::IAPConf saved_iap(iap_id);
        QString iap_type = saved_iap.value("type").toString();
        QString iap_name = saved_iap.value("name").toString();
        QByteArray ssid = saved_iap.value("wlan_ssid").toByteArray();
        if (!iap_type.isEmpty() && !iap_name.isEmpty()) {
            // Check if new IAP is actually Undefined WLAN configuration
            // Note: SSID is used as an iap id for Undefined WLAN configurations
            //       => configuration must be searched using SSID
            if (!ssid.isEmpty() && accessPointConfigurations.contains(ssid)) {
                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.take(ssid);
                if (ptr) {
                    ptr->mutex.lock();
                    ptr->id = iap_id;
                    toIcdConfig(ptr)->iap_type = iap_type;
                    ptr->bearerType = bearerTypeFromIapType(iap_type);
                    toIcdConfig(ptr)->network_attrs = getNetworkAttrs(true, iap_id, iap_type, QString());
                    toIcdConfig(ptr)->network_id = ssid;
                    toIcdConfig(ptr)->service_id = saved_iap.value("service_id").toString();
                    toIcdConfig(ptr)->service_type = saved_iap.value("service_type").toString();
                    if (m_onlineIapId == iap_id) {
                        ptr->state = QNetworkConfiguration::Active;
                    } else {
                        ptr->state = QNetworkConfiguration::Defined;
                    }
                    ptr->mutex.unlock();
                    accessPointConfigurations.insert(iap_id, ptr);

                    locker.unlock();
                    emit configurationChanged(ptr);
                    locker.relock();
                }
            } else {
                IcdNetworkConfigurationPrivate *cpPriv = new IcdNetworkConfigurationPrivate;
                cpPriv->name = saved_iap.value("name").toString();
                if (cpPriv->name.isEmpty())
                    cpPriv->name = iap_id;
                cpPriv->isValid = true;
                cpPriv->id = iap_id;
                cpPriv->iap_type = iap_type;
                cpPriv->bearerType = bearerTypeFromIapType(iap_type);
                cpPriv->network_attrs = getNetworkAttrs(true, iap_id, iap_type, QString());
                cpPriv->service_id = saved_iap.value("service_id").toString();
                cpPriv->service_type = saved_iap.value("service_type").toString();
                if (iap_type.startsWith(QLatin1String("WLAN"))) {
                    QByteArray ssid = saved_iap.value("wlan_ssid").toByteArray();
                    if (ssid.isEmpty()) {
                        qWarning() << "Cannot get ssid for" << iap_id;
                    }
                    cpPriv->network_id = ssid;
                }
                cpPriv->type = QNetworkConfiguration::InternetAccessPoint;
                if (m_onlineIapId == iap_id) {
                    cpPriv->state = QNetworkConfiguration::Active;
                } else {
                    cpPriv->state = QNetworkConfiguration::Defined;
                }

                QNetworkConfigurationPrivatePointer ptr(cpPriv);
                accessPointConfigurations.insert(iap_id, ptr);

#ifdef BEARER_MANAGEMENT_DEBUG
                qDebug("IAP: %s, name: %s, added to known list", iap_id.toAscii().data(), cpPriv->name.toAscii().data());
#endif
                locker.unlock();
                emit configurationAdded(ptr);
                locker.relock();
            }
        } else {
            qWarning("IAP %s does not have \"type\" or \"name\" fields defined, skipping this IAP.", iap_id.toAscii().data());
        }
    } else {
#ifdef BEARER_MANAGEMENT_DEBUG
	qDebug() << "IAP" << iap_id << "already in db.";
#endif

	/* Check if the data in db changed and update configuration accordingly
	 */
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(iap_id);
	if (ptr) {
	    Maemo::IAPConf changed_iap(iap_id);
	    QString iap_type = changed_iap.value("type").toString();
	    bool update_needed = false; /* if IAP type or ssid changed, we need to change the state */

        QMutexLocker configLocker(&ptr->mutex);

	    toIcdConfig(ptr)->network_attrs = getNetworkAttrs(true, iap_id, iap_type, QString());
	    toIcdConfig(ptr)->service_id = changed_iap.value("service_id").toString();
	    toIcdConfig(ptr)->service_type = changed_iap.value("service_type").toString();

	    if (!iap_type.isEmpty()) {
            ptr->name = changed_iap.value("name").toString();
            if (ptr->name.isEmpty())
                ptr->name = iap_id;
            ptr->isValid = true;
            if (toIcdConfig(ptr)->iap_type != iap_type) {
                toIcdConfig(ptr)->iap_type = iap_type;
                ptr->bearerType = bearerTypeFromIapType(iap_type);
                update_needed = true;
            }
            if (iap_type.startsWith(QLatin1String("WLAN"))) {
                QByteArray ssid = changed_iap.value("wlan_ssid").toByteArray();
                if (ssid.isEmpty()) {
                    qWarning() << "Cannot get ssid for" << iap_id;
                }
                if (toIcdConfig(ptr)->network_id != ssid) {
                    toIcdConfig(ptr)->network_id = ssid;
                    update_needed = true;
                }
            }
	    }

	    if (update_needed) {
            ptr->type = QNetworkConfiguration::InternetAccessPoint;
            if (m_onlineIapId == iap_id) {
                if (ptr->state < QNetworkConfiguration::Active) {
                    ptr->state = QNetworkConfiguration::Active;

                    configLocker.unlock();
                    locker.unlock();
                    emit configurationChanged(ptr);
                    locker.relock();
                }
            } else if (ptr->state < QNetworkConfiguration::Defined) {
                ptr->state = QNetworkConfiguration::Defined;

                configLocker.unlock();
                locker.unlock();
                emit configurationChanged(ptr);
                locker.relock();
            }
	    }
	} else {
	    qWarning("Cannot find IAP %s from current configuration although it should be there.", iap_id.toAscii().data());
	}
    }
}

void QIcdEngine::doRequestUpdate(QList<Maemo::IcdScanResult> scanned)
{
    /* Contains all known iap_ids from storage */
    QList<QString> knownConfigs = accessPointConfigurations.keys();

    /* Contains all known WLAN network ids (like ssid) from storage */
    QMultiHash<QByteArray, SSIDInfo* > notDiscoveredWLANConfigs;

    QList<QString> all_iaps;
    Maemo::IAPConf::getAll(all_iaps);

    foreach (const QString &iap_id, all_iaps) {
        QByteArray ssid;

        Maemo::IAPConf saved_ap(iap_id);
        bool is_temporary = saved_ap.value("temporary").toBool();
        if (is_temporary) {
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug() << "IAP" << iap_id << "is temporary, skipping it.";
#endif
            continue;
        }

        QString iap_type = saved_ap.value("type").toString();
        if (iap_type.startsWith(QLatin1String("WLAN"))) {
            ssid = saved_ap.value("wlan_ssid").toByteArray();
            if (ssid.isEmpty())
                continue;

            QString security_method = saved_ap.value("wlan_security").toString();
            SSIDInfo *info = new SSIDInfo;
            info->iap_id = iap_id;
            info->wlan_security = security_method;
                notDiscoveredWLANConfigs.insert(ssid, info);
        } else if (iap_type.isEmpty()) {
            continue;
        } else {
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug() << "IAP" << iap_id << "network type is" << iap_type;
#endif
            ssid.clear();
        }

        if (!accessPointConfigurations.contains(iap_id)) {
            IcdNetworkConfigurationPrivate *cpPriv = new IcdNetworkConfigurationPrivate;

            cpPriv->name = saved_ap.value("name").toString();
            if (cpPriv->name.isEmpty()) {
                if (!ssid.isEmpty() && ssid.size() > 0)
                    cpPriv->name = ssid.data();
                else
                    cpPriv->name = iap_id;
            }
            cpPriv->isValid = true;
            cpPriv->id = iap_id;
            cpPriv->network_id = ssid;
            cpPriv->network_attrs = getNetworkAttrs(true, iap_id, iap_type, QString());
            cpPriv->iap_type = iap_type;
            cpPriv->bearerType = bearerTypeFromIapType(iap_type);
            cpPriv->service_id = saved_ap.value("service_id").toString();
            cpPriv->service_type = saved_ap.value("service_type").toString();
            cpPriv->type = QNetworkConfiguration::InternetAccessPoint;
            cpPriv->state = QNetworkConfiguration::Defined;

            QNetworkConfigurationPrivatePointer ptr(cpPriv);
            accessPointConfigurations.insert(iap_id, ptr);

            mutex.unlock();
            emit configurationAdded(ptr);
            mutex.lock();

#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug("IAP: %s, name: %s, ssid: %s, added to known list",
                   iap_id.toAscii().data(), ptr->name.toAscii().data(),
                   !ssid.isEmpty() ? ssid.data() : "-");
#endif
        } else {
            knownConfigs.removeOne(iap_id);
#ifdef BEARER_MANAGEMENT_DEBUG
            qDebug("IAP: %s, ssid: %s, already exists in the known list",
                   iap_id.toAscii().data(), !ssid.isEmpty() ? ssid.data() : "-");
#endif
        }
    }

    /* This is skipped in the first update as scanned size is zero */
    if (!scanned.isEmpty()) {
        for (int i=0; i<scanned.size(); ++i) {
            const Maemo::IcdScanResult ap = scanned.at(i);

            if (ap.scan.network_attrs & ICD_NW_ATTR_IAPNAME) {
                /* The network_id is IAP id, so the IAP is a known one */
                QString iapid = ap.scan.network_id.data();
                QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(iapid);
                if (ptr) {
                    bool changed = false;

                    ptr->mutex.lock();

                    if (!ptr->isValid) {
                        ptr->isValid = true;
                        changed = true;
                    }

                    /* If this config is the current active one, we do not set it
                     * to discovered.
                     */
                    if ((ptr->state != QNetworkConfiguration::Discovered) &&
                        (ptr->state != QNetworkConfiguration::Active)) {
                        ptr->state = QNetworkConfiguration::Discovered;
                        changed = true;
                    }

                    toIcdConfig(ptr)->network_attrs = ap.scan.network_attrs;
                    toIcdConfig(ptr)->service_id = ap.scan.service_id;
                    toIcdConfig(ptr)->service_type = ap.scan.service_type;
                    toIcdConfig(ptr)->service_attrs = ap.scan.service_attrs;

#ifdef BEARER_MANAGEMENT_DEBUG
                    qDebug("IAP: %s, ssid: %s, discovered",
                           iapid.toAscii().data(), toIcdConfig(ptr)->network_id.data());
#endif

                    ptr->mutex.unlock();

                    if (changed) {
                        mutex.unlock();
                        emit configurationChanged(ptr);
                        mutex.lock();
                    }

                    if (!ap.scan.network_type.startsWith(QLatin1String("WLAN")))
                        continue; // not a wlan AP

                    /* Remove scanned AP from discovered WLAN configurations so that we can
                     * emit configurationRemoved signal later
                     */
                    ptr->mutex.lock();
                    QList<SSIDInfo* > known_iaps = notDiscoveredWLANConfigs.values(toIcdConfig(ptr)->network_id);
rescan_list:
                    if (!known_iaps.isEmpty()) {
                        for (int k=0; k<known_iaps.size(); ++k) {
                            SSIDInfo *iap = known_iaps.at(k);

                            if (iap->wlan_security ==
                                network_attrs_to_security(ap.scan.network_attrs)) {
                                /* Remove IAP from the list */
                                notDiscoveredWLANConfigs.remove(toIcdConfig(ptr)->network_id, iap);
#ifdef BEARER_MANAGEMENT_DEBUG
                                qDebug() << "Removed IAP" << iap->iap_id << "from unknown config";
#endif
                                known_iaps.removeAt(k);
                                delete iap;
                                goto rescan_list;
                            }
                        }
                    }
                    ptr->mutex.unlock();
                }
            } else {
                /* Non saved access point data */
                QByteArray scanned_ssid = ap.scan.network_id;
                if (!accessPointConfigurations.contains(scanned_ssid)) {
                    IcdNetworkConfigurationPrivate *cpPriv = new IcdNetworkConfigurationPrivate;
                    QString hrs = scanned_ssid.data();

                    cpPriv->name = ap.network_name.isEmpty() ? hrs : ap.network_name;
                    cpPriv->isValid = true;
                    cpPriv->id = scanned_ssid.data();  // Note: id is now ssid, it should be set to IAP id if the IAP is saved
                    cpPriv->network_id = scanned_ssid;
                    cpPriv->iap_type = ap.scan.network_type;
                    cpPriv->bearerType = bearerTypeFromIapType(cpPriv->iap_type);
                    cpPriv->network_attrs = ap.scan.network_attrs;
                    cpPriv->service_id = ap.scan.service_id;
                    cpPriv->service_type = ap.scan.service_type;
                    cpPriv->service_attrs = ap.scan.service_attrs;

                    cpPriv->type = QNetworkConfiguration::InternetAccessPoint;
                    cpPriv->state = QNetworkConfiguration::Undefined;

#ifdef BEARER_MANAGEMENT_DEBUG
                    qDebug() << "IAP with network id" << cpPriv->id << "was found in the scan.";
#endif

                    QNetworkConfigurationPrivatePointer ptr(cpPriv);
                    accessPointConfigurations.insert(ptr->id, ptr);

                    mutex.unlock();
                    emit configurationAdded(ptr);
                    mutex.lock();
                } else {
                    knownConfigs.removeOne(scanned_ssid);
                }
            }
        }
    }

    if (!firstUpdate) {
        // Update Defined status to all defined WLAN IAPs which
        // could not be found when access points were scanned
        QHashIterator<QByteArray, SSIDInfo* > i(notDiscoveredWLANConfigs);
        while (i.hasNext()) {
            i.next();
            SSIDInfo *iap = i.value();
            QString iap_id = iap->iap_id;
            //qDebug() << i.key() << ": " << iap_id;

            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(iap_id);
            if (ptr) {
                QMutexLocker configLocker(&ptr->mutex);

                // WLAN AccessPoint configuration could not be Discovered
                // => Make sure that configuration state is Defined
                if (ptr->state > QNetworkConfiguration::Defined) {
                    ptr->state = QNetworkConfiguration::Defined;

                    configLocker.unlock();
                    mutex.unlock();
                    emit configurationChanged(ptr);
                    mutex.lock();
                }
            }
        }

        /* Remove non existing iaps since last update */
        foreach (const QString &oldIface, knownConfigs) {
            QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.take(oldIface);
            if (ptr) {
                mutex.unlock();
                emit configurationRemoved(ptr);
                mutex.lock();
                //if we would have SNAP support we would have to remove the references
                //from existing ServiceNetworks to the removed access point configuration
            }
        }
    }

    QMutableHashIterator<QByteArray, SSIDInfo* > i(notDiscoveredWLANConfigs);
    while (i.hasNext()) {
        i.next();
        SSIDInfo *iap = i.value();
        delete iap;
        i.remove();
    }

    if (!firstUpdate) {
        mutex.unlock();
        emit updateCompleted();
        mutex.lock();
    }

    if (firstUpdate)
        firstUpdate = false;
}

QNetworkConfigurationPrivatePointer QIcdEngine::defaultConfiguration()
{
    QMutexLocker locker(&mutex);

    if (!ensureDBusConnection())
        return QNetworkConfigurationPrivatePointer();

    // Here we just return [ANY] request to icd and let the icd decide which IAP to connect.
    return userChoiceConfigurations.value(OSSO_IAP_ANY);
}

void QIcdEngine::startListeningStateSignalsForAllConnections()
{
    // Start listening ICD_DBUS_API_STATE_SIG signals
    m_dbusInterface->connection().connect(ICD_DBUS_API_INTERFACE,
                                          ICD_DBUS_API_PATH,
                                          ICD_DBUS_API_INTERFACE,
                                          ICD_DBUS_API_STATE_SIG,
                                          this, SLOT(connectionStateSignalsSlot(QDBusMessage)));
}

void QIcdEngine::getIcdInitialState()
{
    /* Instead of requesting ICD status asynchronously, we ask it synchronously.
     * It ensures that we always get right icd status BEFORE initialize() ends.
     * If not, initialize()  might end before we got icd status and
     * QNetworkConfigurationManager::updateConfigurations()
     * call from user might also end before receiving icd status.
     * In such case, we come up to a bug:
     * QNetworkConfigurationManagerPrivate::isOnline() will be false even
     * if we are connected.
     */
    Maemo::Icd icd;
    QList<Maemo::IcdStateResult> state_results;
    QNetworkConfigurationPrivatePointer ptr;

    if (icd.state(state_results) && !state_results.isEmpty()) {

        if (!(state_results.first().params.network_attrs == 0 &&
              state_results.first().params.network_id.isEmpty())) {

            switch (state_results.first().state) {
            case ICD_STATE_CONNECTED:
                m_onlineIapId = state_results.first().params.network_id;

                ptr = accessPointConfigurations.value(m_onlineIapId);
                if (ptr) {
                    QMutexLocker configLocker(&ptr->mutex);
                    ptr->state = QNetworkConfiguration::Active;
                    configLocker.unlock();

                    mutex.unlock();
                    emit configurationChanged(ptr);
                    mutex.lock();
                }
                break;
            default:
                break;
            }
        }
    }
}

void QIcdEngine::connectionStateSignalsSlot(QDBusMessage msg)
{
    QMutexLocker locker(&mutex);

    QList<QVariant> arguments = msg.arguments();
    if (arguments[1].toUInt() != 0 || arguments.count() < 8) {
        return;
    }

    QString iapid = arguments[5].toByteArray().data();
    uint icd_connection_state = arguments[7].toUInt();

    switch (icd_connection_state) {
    case ICD_STATE_CONNECTED:
        {
        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(iapid);
        if (ptr) {
            QMutexLocker configLocker(&ptr->mutex);

            ptr->type = QNetworkConfiguration::InternetAccessPoint;
            if (ptr->state != QNetworkConfiguration::Active) {
                ptr->state = QNetworkConfiguration::Active;

                configLocker.unlock();
                locker.unlock();
                emit configurationChanged(ptr);
                locker.relock();

                m_onlineIapId = iapid;
            }
        } else {
            // This gets called when new WLAN IAP is created using Connection dialog
            // At this point Undefined WLAN configuration has SSID as iap id
            // => Because of that configuration can not be found from
            //    accessPointConfigurations using correct iap id
            m_onlineIapId = iapid;
        }
        break;
        }
    case ICD_STATE_DISCONNECTED:
        {
        QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(iapid);
        if (ptr) {
            QMutexLocker configLocker(&ptr->mutex);

            ptr->type = QNetworkConfiguration::InternetAccessPoint;
            if (ptr->state == QNetworkConfiguration::Active) {
                ptr->state = QNetworkConfiguration::Discovered;

                configLocker.unlock();
                locker.unlock();
                emit configurationChanged(ptr);
                locker.relock();

                // Note: If ICD switches used IAP from one to another:
                //       1) new IAP is reported to be online first
                //       2) old IAP is reported to be offline then
                // => Device can be reported to be offline only
                //    if last known online IAP is reported to be disconnected
                if (iapid == m_onlineIapId) {
                    // It's known that there is only one global ICD connection
                    // => Because ICD state was reported to be DISCONNECTED, Device is offline
                    m_onlineIapId.clear();
                }
            }
        } else {
            // Disconnected IAP was not found from accessPointConfigurations
            // => Reason: Online IAP was removed which resulted ICD to disconnect
            if (iapid == m_onlineIapId) {
                // It's known that there is only one global ICD connection
                // => Because ICD state was reported to be DISCONNECTED, Device is offline
                m_onlineIapId.clear();
            }
        }
        break;
        }
    default:
        break;
    }
    
    locker.unlock();
    emit iapStateChanged(iapid, icd_connection_state);
    locker.relock();
}

void QIcdEngine::icdServiceOwnerChanged(const QString &serviceName, const QString &oldOwner,
                                        const QString &newOwner)
{
    Q_UNUSED(serviceName);
    Q_UNUSED(oldOwner);

    QMutexLocker locker(&mutex);

    if (newOwner.isEmpty()) {
        // Disconnected from ICD, remove all configurations
        cleanup();
        delete iapMonitor;
        iapMonitor = 0;
        delete m_dbusInterface;
        m_dbusInterface = 0;

        QMutableHashIterator<QString, QNetworkConfigurationPrivatePointer> i(accessPointConfigurations);
        while (i.hasNext()) {
            i.next();

            QNetworkConfigurationPrivatePointer ptr = i.value();
            i.remove();

            locker.unlock();
            emit configurationRemoved(ptr);
            locker.relock();
        }

        userChoiceConfigurations.clear();
    } else {
        // Connected to ICD ensure connection.
        ensureDBusConnection();
    }
}

void QIcdEngine::requestUpdate()
{
    QMutexLocker locker(&mutex);

    if (!ensureDBusConnection()) {
        locker.unlock();
        emit updateCompleted();
        locker.relock();
        return;
    }

    if (m_scanGoingOn)
        return;

    m_scanGoingOn = true;

    m_dbusInterface->connection().connect(ICD_DBUS_API_INTERFACE,
                                          ICD_DBUS_API_PATH,
                                          ICD_DBUS_API_INTERFACE,
                                          ICD_DBUS_API_SCAN_SIG,
                                          this, SLOT(asyncUpdateConfigurationsSlot(QDBusMessage)));

    QDBusMessage msg = m_dbusInterface->call(ICD_DBUS_API_SCAN_REQ,
                                             (uint)ICD_SCAN_REQUEST_ACTIVE);
    m_typesToBeScanned = msg.arguments()[0].value<QStringList>();
    m_scanTimer.start(ICD_SHORT_SCAN_TIMEOUT);
}

void QIcdEngine::cancelAsyncConfigurationUpdate()
{
    if (!ensureDBusConnection())
        return;

    if (!m_scanGoingOn)
        return;

    m_scanGoingOn = false;

    if (m_scanTimer.isActive())
        m_scanTimer.stop();

    m_dbusInterface->connection().disconnect(ICD_DBUS_API_INTERFACE,
                                             ICD_DBUS_API_PATH,
                                             ICD_DBUS_API_INTERFACE,
                                             ICD_DBUS_API_SCAN_SIG,
                                             this, SLOT(asyncUpdateConfigurationsSlot(QDBusMessage)));

    // Stop scanning rounds by calling ICD_DBUS_API_SCAN_CANCEL
    // <=> If ICD_DBUS_API_SCAN_CANCEL is not called, new scanning round will
    //     be started after the module scan timeout.
    m_dbusInterface->call(ICD_DBUS_API_SCAN_CANCEL);
}

void QIcdEngine::finishAsyncConfigurationUpdate()
{
    QMutexLocker locker(&mutex);

    cancelAsyncConfigurationUpdate();
    doRequestUpdate(m_scanResult);
    m_scanResult.clear();
}

void QIcdEngine::asyncUpdateConfigurationsSlot(QDBusMessage msg)
{
    QMutexLocker locker(&mutex);

    QList<QVariant> arguments = msg.arguments();
    uint icd_scan_status = arguments.takeFirst().toUInt();
    if (icd_scan_status == ICD_SCAN_COMPLETE) {
        m_typesToBeScanned.removeOne(arguments[6].toString());
        if (!m_typesToBeScanned.count()) {
            locker.unlock();
            finishAsyncConfigurationUpdate();
            locker.relock();
        }
    } else {
        Maemo::IcdScanResult scanResult;
        scanResult.status = icd_scan_status;
        scanResult.timestamp = arguments.takeFirst().toUInt();
        scanResult.scan.service_type = arguments.takeFirst().toString();
        scanResult.service_name = arguments.takeFirst().toString();
        scanResult.scan.service_attrs = arguments.takeFirst().toUInt();
        scanResult.scan.service_id = arguments.takeFirst().toString();
        scanResult.service_priority = arguments.takeFirst().toInt();
        scanResult.scan.network_type = arguments.takeFirst().toString();
        scanResult.network_name = arguments.takeFirst().toString();
        scanResult.scan.network_attrs = arguments.takeFirst().toUInt();
        scanResult.scan.network_id = arguments.takeFirst().toByteArray();
        scanResult.network_priority = arguments.takeFirst().toInt();
        scanResult.signal_strength = arguments.takeFirst().toInt();
        scanResult.station_id = arguments.takeFirst().toString();
        scanResult.signal_dB = arguments.takeFirst().toInt();

        m_scanResult.append(scanResult);
    }
}

void QIcdEngine::cleanup()
{
    if (m_scanGoingOn) {
        m_scanTimer.stop();
        m_dbusInterface->call(ICD_DBUS_API_SCAN_CANCEL);
    }
    if (iapMonitor)
        iapMonitor->cleanup();
}

bool QIcdEngine::hasIdentifier(const QString &id)
{
    QMutexLocker locker(&mutex);

    return accessPointConfigurations.contains(id) ||
           snapConfigurations.contains(id) ||
           userChoiceConfigurations.contains(id);
}

QNetworkSessionPrivate *QIcdEngine::createSessionBackend()
{
    return new QNetworkSessionPrivateImpl(this);
}

#include "qicdengine.moc"

QT_END_NAMESPACE

#endif // QT_NO_BEARERMANAGEMENT
