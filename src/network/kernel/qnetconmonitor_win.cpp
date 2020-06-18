/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#include "qnetconmonitor_p.h"

#include "private/qobject_p.h"

#include <QtCore/quuid.h>
#include <QtCore/qmetaobject.h>

#include <QtNetwork/qnetworkinterface.h>

#include <objbase.h>
#include <netlistmgr.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <comdef.h>
#include <iphlpapi.h>

#include <algorithm>

using namespace Microsoft::WRL;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcNetMon, "qt.network.monitor");

namespace {
QString errorStringFromHResult(HRESULT hr)
{
    _com_error error(hr);
    return QString::fromWCharArray(error.ErrorMessage());
}

template<typename T>
bool QueryInterfaceImpl(IUnknown *from, REFIID riid, void **ppvObject)
{
    if (riid == __uuidof(T)) {
        *ppvObject = static_cast<T *>(from);
        from->AddRef();
        return true;
    }
    return false;
}

QNetworkInterface getInterfaceFromHostAddress(const QHostAddress &local)
{
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    auto it = std::find_if(
            interfaces.cbegin(), interfaces.cend(), [&local](const QNetworkInterface &iface) {
                const auto &entries = iface.addressEntries();
                return std::any_of(entries.cbegin(), entries.cend(),
                                   [&local](const QNetworkAddressEntry &entry) {
                                       return entry.ip().isEqual(local,
                                                                 QHostAddress::TolerantConversion);
                                   });
            });
    if (it == interfaces.cend()) {
        qCWarning(lcNetMon, "Could not find the interface for the local address.");
        return {};
    }
    return *it;
}
} // anonymous namespace

class QNetworkConnectionEvents : public INetworkConnectionEvents
{
public:
    QNetworkConnectionEvents(QNetworkConnectionMonitorPrivate *monitor);
    virtual ~QNetworkConnectionEvents();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--ref == 0) {
            delete this;
            return 0;
        }
        return ref;
    }

    HRESULT STDMETHODCALLTYPE
    NetworkConnectionConnectivityChanged(GUID connectionId, NLM_CONNECTIVITY connectivity) override;
    HRESULT STDMETHODCALLTYPE NetworkConnectionPropertyChanged(
            GUID connectionId, NLM_CONNECTION_PROPERTY_CHANGE flags) override;

    Q_REQUIRED_RESULT
    bool setTarget(const QNetworkInterface &iface);
    Q_REQUIRED_RESULT
    bool startMonitoring();
    Q_REQUIRED_RESULT
    bool stopMonitoring();

private:
    ComPtr<INetworkConnection> getNetworkConnectionFromAdapterGuid(QUuid guid);

    QUuid currentConnectionId{};

    ComPtr<INetworkListManager> networkListManager;
    ComPtr<IConnectionPoint> connectionPoint;

    QNetworkConnectionMonitorPrivate *monitor = nullptr;

    QAtomicInteger<ULONG> ref = 0;
    DWORD cookie = 0;
};

class QNetworkConnectionMonitorPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QNetworkConnectionMonitor);

public:
    QNetworkConnectionMonitorPrivate();
    ~QNetworkConnectionMonitorPrivate();

    Q_REQUIRED_RESULT
    bool setTargets(const QHostAddress &local, const QHostAddress &remote);
    Q_REQUIRED_RESULT
    bool startMonitoring();
    void stopMonitoring();

    void setConnectivity(NLM_CONNECTIVITY newConnectivity);

private:
    ComPtr<QNetworkConnectionEvents> connectionEvents;
    // We can assume we have access to internet/subnet when this class is created because
    // connection has already been established to the peer:
    NLM_CONNECTIVITY connectivity = NLM_CONNECTIVITY(
            NLM_CONNECTIVITY_IPV4_INTERNET | NLM_CONNECTIVITY_IPV6_INTERNET
            | NLM_CONNECTIVITY_IPV4_SUBNET | NLM_CONNECTIVITY_IPV6_SUBNET
            | NLM_CONNECTIVITY_IPV4_LOCALNETWORK | NLM_CONNECTIVITY_IPV6_LOCALNETWORK
            | NLM_CONNECTIVITY_IPV4_NOTRAFFIC | NLM_CONNECTIVITY_IPV6_NOTRAFFIC);

    bool sameSubnet = false;
    bool isLinkLocal = false;
    bool monitoring = false;
    bool comInitFailed = false;
    bool remoteIsIPv6 = false;
};

QNetworkConnectionEvents::QNetworkConnectionEvents(QNetworkConnectionMonitorPrivate *monitor)
    : monitor(monitor)
{
    auto hr = CoCreateInstance(CLSID_NetworkListManager, nullptr, CLSCTX_INPROC_SERVER,
                               IID_INetworkListManager, &networkListManager);
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Could not get a NetworkListManager instance:"
                            << errorStringFromHResult(hr);
        return;
    }

    ComPtr<IConnectionPointContainer> connectionPointContainer;
    hr = networkListManager.As(&connectionPointContainer);
    if (SUCCEEDED(hr)) {
        hr = connectionPointContainer->FindConnectionPoint(IID_INetworkConnectionEvents,
                                                           &connectionPoint);
    }
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Failed to get connection point for network events:"
                            << errorStringFromHResult(hr);
    }
}

QNetworkConnectionEvents::~QNetworkConnectionEvents()
{
    Q_ASSERT(ref == 0);
}

ComPtr<INetworkConnection> QNetworkConnectionEvents::getNetworkConnectionFromAdapterGuid(QUuid guid)
{
    ComPtr<IEnumNetworkConnections> connections;
    auto hr = networkListManager->GetNetworkConnections(connections.GetAddressOf());
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Failed to enumerate network connections:"
                            << errorStringFromHResult(hr);
        return nullptr;
    }
    ComPtr<INetworkConnection> connection = nullptr;
    do {
        hr = connections->Next(1, connection.GetAddressOf(), nullptr);
        if (FAILED(hr)) {
            qCWarning(lcNetMon) << "Failed to get next network connection in enumeration:"
                                << errorStringFromHResult(hr);
            break;
        }
        if (connection) {
            GUID adapterId;
            hr = connection->GetAdapterId(&adapterId);
            if (FAILED(hr)) {
                qCWarning(lcNetMon) << "Failed to get adapter ID from network connection:"
                                    << errorStringFromHResult(hr);
                continue;
            }
            if (guid == adapterId)
                return connection;
        }
    } while (connection);
    return nullptr;
}

HRESULT STDMETHODCALLTYPE QNetworkConnectionEvents::QueryInterface(REFIID riid, void **ppvObject)
{
    if (!ppvObject)
        return E_INVALIDARG;

    return QueryInterfaceImpl<IUnknown>(this, riid, ppvObject)
                    || QueryInterfaceImpl<INetworkConnectionEvents>(this, riid, ppvObject)
            ? S_OK
            : E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE QNetworkConnectionEvents::NetworkConnectionConnectivityChanged(
        GUID connectionId, NLM_CONNECTIVITY newConnectivity)
{
    // This function is run on a different thread than 'monitor' is created on, so we need to run
    // it on that thread
    QMetaObject::invokeMethod(monitor->q_ptr,
                              [this, connectionId, newConnectivity, monitor = this->monitor]() {
                                  if (connectionId == currentConnectionId)
                                      monitor->setConnectivity(newConnectivity);
                              },
                              Qt::QueuedConnection);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE QNetworkConnectionEvents::NetworkConnectionPropertyChanged(
        GUID connectionId, NLM_CONNECTION_PROPERTY_CHANGE flags)
{
    Q_UNUSED(connectionId);
    Q_UNUSED(flags);
    return E_NOTIMPL;
}

bool QNetworkConnectionEvents::setTarget(const QNetworkInterface &iface)
{
    // Unset this in case it's already set to something
    currentConnectionId = QUuid{};

    NET_LUID luid;
    if (ConvertInterfaceIndexToLuid(iface.index(), &luid) != NO_ERROR) {
        qCWarning(lcNetMon, "Could not get the LUID for the interface.");
        return false;
    }
    GUID guid;
    if (ConvertInterfaceLuidToGuid(&luid, &guid) != NO_ERROR) {
        qCWarning(lcNetMon, "Could not get the GUID for the interface.");
        return false;
    }
    ComPtr<INetworkConnection> connection = getNetworkConnectionFromAdapterGuid(guid);
    if (!connection) {
        qCWarning(lcNetMon, "Could not get the INetworkConnection instance for the adapter GUID.");
        return false;
    }
    auto hr = connection->GetConnectionId(&guid);
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Failed to get the connection's GUID:" << errorStringFromHResult(hr);
        return false;
    }
    currentConnectionId = guid;

    return true;
}

bool QNetworkConnectionEvents::startMonitoring()
{
    if (currentConnectionId.isNull()) {
        qCWarning(lcNetMon, "Can not start monitoring, set targets first");
        return false;
    }
    if (!connectionPoint) {
        qCWarning(lcNetMon,
                  "We don't have the connection point, cannot start listening to events!");
        return false;
    }

    auto hr = connectionPoint->Advise(this, &cookie);
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Failed to subscribe to network connectivity events:"
                            << errorStringFromHResult(hr);
        return false;
    }
    return true;
}

bool QNetworkConnectionEvents::stopMonitoring()
{
    auto hr = connectionPoint->Unadvise(cookie);
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Failed to unsubscribe from network connection events:"
                            << errorStringFromHResult(hr);
        return false;
    }
    cookie = 0;
    currentConnectionId = QUuid{};
    return true;
}

QNetworkConnectionMonitorPrivate::QNetworkConnectionMonitorPrivate()
{
    auto hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Failed to initialize COM:" << errorStringFromHResult(hr);
        comInitFailed = true;
        return;
    }

    connectionEvents = new QNetworkConnectionEvents(this);
}

QNetworkConnectionMonitorPrivate::~QNetworkConnectionMonitorPrivate()
{
    if (comInitFailed)
        return;
    if (monitoring)
        stopMonitoring();
    connectionEvents.Reset();
    CoUninitialize();
}

bool QNetworkConnectionMonitorPrivate::setTargets(const QHostAddress &local,
                                                  const QHostAddress &remote)
{
    if (comInitFailed)
        return false;

    QNetworkInterface iface = getInterfaceFromHostAddress(local);
    if (!iface.isValid())
        return false;
    const auto &addressEntries = iface.addressEntries();
    auto it = std::find_if(
            addressEntries.cbegin(), addressEntries.cend(),
            [&local](const QNetworkAddressEntry &entry) { return entry.ip() == local; });
    if (Q_UNLIKELY(it == addressEntries.cend())) {
        qCWarning(lcNetMon, "The address entry we were working with disappeared");
        return false;
    }
    sameSubnet = remote.isInSubnet(local, it->prefixLength());
    isLinkLocal = remote.isLinkLocal() && local.isLinkLocal();
    remoteIsIPv6 = remote.protocol() == QAbstractSocket::IPv6Protocol;

    return connectionEvents->setTarget(iface);
}

void QNetworkConnectionMonitorPrivate::setConnectivity(NLM_CONNECTIVITY newConnectivity)
{
    Q_Q(QNetworkConnectionMonitor);
    const bool reachable = q->isReachable();
    connectivity = newConnectivity;
    const bool newReachable = q->isReachable();
    if (reachable != newReachable)
        emit q->reachabilityChanged(newReachable);
}

bool QNetworkConnectionMonitorPrivate::startMonitoring()
{
    Q_ASSERT(connectionEvents);
    Q_ASSERT(!monitoring);
    if (connectionEvents->startMonitoring())
        monitoring = true;
    return monitoring;
}

void QNetworkConnectionMonitorPrivate::stopMonitoring()
{
    Q_ASSERT(connectionEvents);
    Q_ASSERT(monitoring);
    if (connectionEvents->stopMonitoring())
        monitoring = false;
}

QNetworkConnectionMonitor::QNetworkConnectionMonitor()
    : QObject(*new QNetworkConnectionMonitorPrivate)
{
}

QNetworkConnectionMonitor::QNetworkConnectionMonitor(const QHostAddress &local,
                                                     const QHostAddress &remote)
    : QObject(*new QNetworkConnectionMonitorPrivate)
{
    setTargets(local, remote);
}

QNetworkConnectionMonitor::~QNetworkConnectionMonitor() = default;

bool QNetworkConnectionMonitor::setTargets(const QHostAddress &local, const QHostAddress &remote)
{
    if (isMonitoring()) {
        qCWarning(lcNetMon, "Monitor is already active, call stopMonitoring() first");
        return false;
    }
    if (local.isNull()) {
        qCWarning(lcNetMon, "Invalid (null) local address, cannot create a reachability target");
        return false;
    }
    // Silently return false for loopback addresses instead of printing warnings later
    if (remote.isLoopback())
        return false;

    return d_func()->setTargets(local, remote);
}

bool QNetworkConnectionMonitor::startMonitoring()
{
    Q_D(QNetworkConnectionMonitor);
    if (isMonitoring()) {
        qCWarning(lcNetMon, "Monitor is already active, call stopMonitoring() first");
        return false;
    }
    return d->startMonitoring();
}

bool QNetworkConnectionMonitor::isMonitoring() const
{
    return d_func()->monitoring;
}

void QNetworkConnectionMonitor::stopMonitoring()
{
    Q_D(QNetworkConnectionMonitor);
    if (!isMonitoring()) {
        qCWarning(lcNetMon, "stopMonitoring was called when not monitoring!");
        return;
    }
    d->stopMonitoring();
}

bool QNetworkConnectionMonitor::isReachable()
{
    Q_D(QNetworkConnectionMonitor);

    const NLM_CONNECTIVITY RequiredSameSubnetIPv6 =
            NLM_CONNECTIVITY(NLM_CONNECTIVITY_IPV6_SUBNET | NLM_CONNECTIVITY_IPV6_LOCALNETWORK
                             | NLM_CONNECTIVITY_IPV6_INTERNET);
    const NLM_CONNECTIVITY RequiredSameSubnetIPv4 =
            NLM_CONNECTIVITY(NLM_CONNECTIVITY_IPV4_SUBNET | NLM_CONNECTIVITY_IPV4_LOCALNETWORK
                             | NLM_CONNECTIVITY_IPV4_INTERNET);

    NLM_CONNECTIVITY required;
    if (d->isLinkLocal) {
        required = NLM_CONNECTIVITY(
                d->remoteIsIPv6 ? NLM_CONNECTIVITY_IPV6_NOTRAFFIC | RequiredSameSubnetIPv6
                                : NLM_CONNECTIVITY_IPV4_NOTRAFFIC | RequiredSameSubnetIPv4);
    } else if (d->sameSubnet) {
        required =
                NLM_CONNECTIVITY(d->remoteIsIPv6 ? RequiredSameSubnetIPv6 : RequiredSameSubnetIPv4);

    } else {
        required = NLM_CONNECTIVITY(d->remoteIsIPv6 ? NLM_CONNECTIVITY_IPV6_INTERNET
                                                    : NLM_CONNECTIVITY_IPV4_INTERNET);
    }

    return d_func()->connectivity & required;
}

class QNetworkListManagerEvents : public INetworkListManagerEvents
{
public:
    QNetworkListManagerEvents(QNetworkStatusMonitorPrivate *monitor);
    virtual ~QNetworkListManagerEvents();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;

    ULONG STDMETHODCALLTYPE AddRef() override { return ++ref; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        if (--ref == 0) {
            delete this;
            return 0;
        }
        return ref;
    }

    HRESULT STDMETHODCALLTYPE ConnectivityChanged(NLM_CONNECTIVITY newConnectivity) override;

    Q_REQUIRED_RESULT
    bool start();
    bool stop();

private:
    ComPtr<INetworkListManager> networkListManager = nullptr;
    ComPtr<IConnectionPoint> connectionPoint = nullptr;

    QNetworkStatusMonitorPrivate *monitor = nullptr;

    QAtomicInteger<ULONG> ref = 0;
    DWORD cookie = 0;
};

class QNetworkStatusMonitorPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QNetworkStatusMonitor);

public:
    QNetworkStatusMonitorPrivate();
    ~QNetworkStatusMonitorPrivate();

    Q_REQUIRED_RESULT
    bool start();
    void stop();

    void setConnectivity(NLM_CONNECTIVITY newConnectivity);

private:
    friend class QNetworkListManagerEvents;

    ComPtr<QNetworkListManagerEvents> managerEvents;
    NLM_CONNECTIVITY connectivity = NLM_CONNECTIVITY_DISCONNECTED;

    bool monitoring = false;
    bool comInitFailed = false;
};

QNetworkListManagerEvents::QNetworkListManagerEvents(QNetworkStatusMonitorPrivate *monitor)
    : monitor(monitor)
{
    auto hr = CoCreateInstance(CLSID_NetworkListManager, nullptr, CLSCTX_INPROC_SERVER,
                               IID_INetworkListManager, &networkListManager);
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Could not get a NetworkListManager instance:"
                            << errorStringFromHResult(hr);
        return;
    }

    // Set initial connectivity
    hr = networkListManager->GetConnectivity(&monitor->connectivity);
    if (FAILED(hr))
        qCWarning(lcNetMon) << "Could not get connectivity:" << errorStringFromHResult(hr);

    ComPtr<IConnectionPointContainer> connectionPointContainer;
    hr = networkListManager.As(&connectionPointContainer);
    if (SUCCEEDED(hr)) {
        hr = connectionPointContainer->FindConnectionPoint(IID_INetworkListManagerEvents,
                                                           &connectionPoint);
    }
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Failed to get connection point for network list manager events:"
                            << errorStringFromHResult(hr);
    }
}

QNetworkListManagerEvents::~QNetworkListManagerEvents()
{
    Q_ASSERT(ref == 0);
}

HRESULT STDMETHODCALLTYPE QNetworkListManagerEvents::QueryInterface(REFIID riid, void **ppvObject)
{
    if (!ppvObject)
        return E_INVALIDARG;

    return QueryInterfaceImpl<IUnknown>(this, riid, ppvObject)
                    || QueryInterfaceImpl<INetworkListManagerEvents>(this, riid, ppvObject)
            ? S_OK
            : E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE
QNetworkListManagerEvents::ConnectivityChanged(NLM_CONNECTIVITY newConnectivity)
{
    // This function is run on a different thread than 'monitor' is created on, so we need to run
    // it on that thread
    QMetaObject::invokeMethod(monitor->q_ptr,
                              [newConnectivity, monitor = this->monitor]() {
                                  monitor->setConnectivity(newConnectivity);
                              },
                              Qt::QueuedConnection);
    return S_OK;
}

bool QNetworkListManagerEvents::start()
{
    if (!connectionPoint) {
        qCWarning(lcNetMon, "Initialization failed, can't start!");
        return false;
    }
    auto hr = connectionPoint->Advise(this, &cookie);
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Failed to subscribe to network connectivity events:"
                            << errorStringFromHResult(hr);
        return false;
    }

    // Update connectivity since it might have changed since this class was constructed
    NLM_CONNECTIVITY connectivity;
    hr = networkListManager->GetConnectivity(&connectivity);
    if (FAILED(hr))
        qCWarning(lcNetMon) << "Could not get connectivity:" << errorStringFromHResult(hr);
    else
        monitor->setConnectivity(connectivity);
    return true;
}

bool QNetworkListManagerEvents::stop()
{
    Q_ASSERT(connectionPoint);
    auto hr = connectionPoint->Unadvise(cookie);
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Failed to unsubscribe from network connectivity events:"
                            << errorStringFromHResult(hr);
        return false;
    }
    cookie = 0;
    return true;
}

QNetworkStatusMonitorPrivate::QNetworkStatusMonitorPrivate()
{
    auto hr = CoInitialize(nullptr);
    if (FAILED(hr)) {
        qCWarning(lcNetMon) << "Failed to initialize COM:" << errorStringFromHResult(hr);
        comInitFailed = true;
        return;
    }
    managerEvents = new QNetworkListManagerEvents(this);
}

QNetworkStatusMonitorPrivate::~QNetworkStatusMonitorPrivate()
{
    if (comInitFailed)
        return;
    if (monitoring)
        stop();
}

void QNetworkStatusMonitorPrivate::setConnectivity(NLM_CONNECTIVITY newConnectivity)
{
    Q_Q(QNetworkStatusMonitor);

    const bool oldAccessibility = q->isNetworkAccessible();
    connectivity = newConnectivity;
    const bool accessibility = q->isNetworkAccessible();
    if (oldAccessibility != accessibility)
        emit q->onlineStateChanged(accessibility);
}

bool QNetworkStatusMonitorPrivate::start()
{
    Q_ASSERT(!monitoring);

    if (comInitFailed) {
        auto hr = CoInitialize(nullptr);
        if (FAILED(hr)) {
            qCWarning(lcNetMon) << "Failed to initialize COM:" << errorStringFromHResult(hr);
            comInitFailed = true;
            return false;
        }
        comInitFailed = false;
    }
    if (!managerEvents)
        managerEvents = new QNetworkListManagerEvents(this);

    if (managerEvents->start())
        monitoring = true;
    return monitoring;
}

void QNetworkStatusMonitorPrivate::stop()
{
    Q_ASSERT(managerEvents);
    Q_ASSERT(monitoring);
    // Can return false but realistically shouldn't since that would break everything:
    managerEvents->stop();
    monitoring = false;
    managerEvents.Reset();

    CoUninitialize();
    comInitFailed = true; // we check this value in start() to see if we need to re-initialize
}

QNetworkStatusMonitor::QNetworkStatusMonitor(QObject *parent)
    : QObject(*new QNetworkStatusMonitorPrivate, parent)
{
}

QNetworkStatusMonitor::~QNetworkStatusMonitor() {}

bool QNetworkStatusMonitor::start()
{
    if (isMonitoring()) {
        qCWarning(lcNetMon, "Monitor is already active, call stopMonitoring() first");
        return false;
    }

    return d_func()->start();
}

void QNetworkStatusMonitor::stop()
{
    if (!isMonitoring()) {
        qCWarning(lcNetMon, "stopMonitoring was called when not monitoring!");
        return;
    }

    d_func()->stop();
}

bool QNetworkStatusMonitor::isMonitoring() const
{
    return d_func()->monitoring;
}

bool QNetworkStatusMonitor::isNetworkAccessible()
{
    return d_func()->connectivity
            & (NLM_CONNECTIVITY_IPV4_INTERNET | NLM_CONNECTIVITY_IPV6_INTERNET
               | NLM_CONNECTIVITY_IPV4_SUBNET | NLM_CONNECTIVITY_IPV6_SUBNET
               | NLM_CONNECTIVITY_IPV4_LOCALNETWORK | NLM_CONNECTIVITY_IPV6_LOCALNETWORK);
}

bool QNetworkStatusMonitor::event(QEvent *event)
{
    if (event->type() == QEvent::ThreadChange && isMonitoring()) {
        stop();
        QMetaObject::invokeMethod(this, &QNetworkStatusMonitor::start, Qt::QueuedConnection);
    }

    return QObject::event(event);
}

bool QNetworkStatusMonitor::isEnabled()
{
    return true;
}

void QNetworkStatusMonitor::reachabilityChanged(bool online)
{
    Q_UNUSED(online);
    Q_UNREACHABLE();
}

QT_END_NAMESPACE
