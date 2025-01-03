// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnetconmonitor_p.h"

#include "private/qobject_p.h"

#include <QtCore/quuid.h>
#include <QtCore/qmetaobject.h>

#include <QtCore/private/qfunctions_win_p.h>
#include <QtCore/private/qsystemerror_p.h>

#include <QtNetwork/qnetworkinterface.h>

#include <objbase.h>
#include <netlistmgr.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>
#include <iphlpapi.h>

#include <algorithm>

using namespace Microsoft::WRL;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcNetMon, "qt.network.monitor");

namespace {
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
        qCDebug(lcNetMon, "Could not find the interface for the local address.");
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

    [[nodiscard]]
    bool setTarget(const QNetworkInterface &iface);
    [[nodiscard]]
    bool startMonitoring();
    [[nodiscard]]
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

    [[nodiscard]]
    bool setTargets(const QHostAddress &local, const QHostAddress &remote);
    [[nodiscard]]
    bool startMonitoring();
    void stopMonitoring();

    void setConnectivity(NLM_CONNECTIVITY newConnectivity);

private:
    QComHelper comHelper;

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
    bool remoteIsIPv6 = false;
};

QNetworkConnectionEvents::QNetworkConnectionEvents(QNetworkConnectionMonitorPrivate *monitor)
    : monitor(monitor)
{
    auto hr = CoCreateInstance(CLSID_NetworkListManager, nullptr, CLSCTX_INPROC_SERVER,
                               IID_INetworkListManager, &networkListManager);
    if (FAILED(hr)) {
        qCDebug(lcNetMon) << "Could not get a NetworkListManager instance:"
                            << QSystemError::windowsComString(hr);
        return;
    }

    ComPtr<IConnectionPointContainer> connectionPointContainer;
    hr = networkListManager.As(&connectionPointContainer);
    if (SUCCEEDED(hr)) {
        hr = connectionPointContainer->FindConnectionPoint(IID_INetworkConnectionEvents,
                                                           &connectionPoint);
    }
    if (FAILED(hr)) {
        qCDebug(lcNetMon) << "Failed to get connection point for network events:"
                            << QSystemError::windowsComString(hr);
    }
}

QNetworkConnectionEvents::~QNetworkConnectionEvents()
{
    Q_ASSERT(ref == 0);
}

ComPtr<INetworkConnection> QNetworkConnectionEvents::getNetworkConnectionFromAdapterGuid(QUuid guid)
{
    if (!networkListManager) {
        qCDebug(lcNetMon) << "Failed to enumerate network connections:"
                          << "NetworkListManager was not instantiated";
        return nullptr;
    }

    ComPtr<IEnumNetworkConnections> connections;
    auto hr = networkListManager->GetNetworkConnections(connections.GetAddressOf());
    if (FAILED(hr)) {
        qCDebug(lcNetMon) << "Failed to enumerate network connections:"
                            << QSystemError::windowsComString(hr);
        return nullptr;
    }
    ComPtr<INetworkConnection> connection = nullptr;
    do {
        hr = connections->Next(1, connection.GetAddressOf(), nullptr);
        if (FAILED(hr)) {
            qCDebug(lcNetMon) << "Failed to get next network connection in enumeration:"
                                << QSystemError::windowsComString(hr);
            break;
        }
        if (connection) {
            GUID adapterId;
            hr = connection->GetAdapterId(&adapterId);
            if (FAILED(hr)) {
                qCDebug(lcNetMon) << "Failed to get adapter ID from network connection:"
                                    << QSystemError::windowsComString(hr);
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
        qCDebug(lcNetMon, "Could not get the LUID for the interface.");
        return false;
    }
    GUID guid;
    if (ConvertInterfaceLuidToGuid(&luid, &guid) != NO_ERROR) {
        qCDebug(lcNetMon, "Could not get the GUID for the interface.");
        return false;
    }
    ComPtr<INetworkConnection> connection = getNetworkConnectionFromAdapterGuid(guid);
    if (!connection) {
        qCDebug(lcNetMon, "Could not get the INetworkConnection instance for the adapter GUID.");
        return false;
    }
    auto hr = connection->GetConnectionId(&guid);
    if (FAILED(hr)) {
        qCDebug(lcNetMon) << "Failed to get the connection's GUID:"
                          << QSystemError::windowsComString(hr);
        return false;
    }
    currentConnectionId = guid;

    return true;
}

bool QNetworkConnectionEvents::startMonitoring()
{
    if (currentConnectionId.isNull()) {
        qCDebug(lcNetMon, "Can not start monitoring, set targets first");
        return false;
    }
    if (!connectionPoint) {
        qCDebug(lcNetMon,
                  "We don't have the connection point, cannot start listening to events!");
        return false;
    }

    auto hr = connectionPoint->Advise(this, &cookie);
    if (FAILED(hr)) {
        qCDebug(lcNetMon) << "Failed to subscribe to network connectivity events:"
                            << QSystemError::windowsComString(hr);
        return false;
    }
    return true;
}

bool QNetworkConnectionEvents::stopMonitoring()
{
    auto hr = connectionPoint->Unadvise(cookie);
    if (FAILED(hr)) {
        qCDebug(lcNetMon) << "Failed to unsubscribe from network connection events:"
                            << QSystemError::windowsComString(hr);
        return false;
    }
    cookie = 0;
    currentConnectionId = QUuid{};
    return true;
}

QNetworkConnectionMonitorPrivate::QNetworkConnectionMonitorPrivate()
{
    if (!comHelper.isValid())
        return;

    connectionEvents = new QNetworkConnectionEvents(this);
}

QNetworkConnectionMonitorPrivate::~QNetworkConnectionMonitorPrivate()
{
    if (!comHelper.isValid())
        return;
    if (monitoring)
        stopMonitoring();
    connectionEvents.Reset();
}

bool QNetworkConnectionMonitorPrivate::setTargets(const QHostAddress &local,
                                                  const QHostAddress &remote)
{
    if (!comHelper.isValid())
        return false;

    QNetworkInterface iface = getInterfaceFromHostAddress(local);
    if (!iface.isValid())
        return false;
    const auto &addressEntries = iface.addressEntries();
    auto it = std::find_if(
            addressEntries.cbegin(), addressEntries.cend(),
            [&local](const QNetworkAddressEntry &entry) { return entry.ip() == local; });
    if (Q_UNLIKELY(it == addressEntries.cend())) {
        qCDebug(lcNetMon, "The address entry we were working with disappeared");
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
        qCDebug(lcNetMon, "Monitor is already active, call stopMonitoring() first");
        return false;
    }
    if (local.isNull()) {
        qCDebug(lcNetMon, "Invalid (null) local address, cannot create a reachability target");
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
        qCDebug(lcNetMon, "Monitor is already active, call stopMonitoring() first");
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
        qCDebug(lcNetMon, "stopMonitoring was called when not monitoring!");
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

bool QNetworkConnectionMonitor::isEnabled()
{
    return true;
}

QT_END_NAMESPACE
