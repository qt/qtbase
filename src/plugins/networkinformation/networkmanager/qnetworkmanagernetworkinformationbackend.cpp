// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qnetworkmanagernetworkinformationbackend.h"

#include <QtCore/qglobal.h>
#include <QtCore/private/qobject_p.h>

#include <QtDBus/qdbusmessage.h>

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(lcNetInfoNLM)
Q_LOGGING_CATEGORY(lcNetInfoNM, "qt.network.info.networkmanager");

namespace {
QNetworkInformation::Reachability reachabilityFromNMState(QNetworkManagerInterface::NMState state)
{
    switch (state) {
    case QNetworkManagerInterface::NM_STATE_UNKNOWN:
    case QNetworkManagerInterface::NM_STATE_ASLEEP:
    case QNetworkManagerInterface::NM_STATE_CONNECTING:
        return QNetworkInformation::Reachability::Unknown;
    case QNetworkManagerInterface::NM_STATE_DISCONNECTING: // No point in starting new connections:
    case QNetworkManagerInterface::NM_STATE_DISCONNECTED:
        return QNetworkInformation::Reachability::Disconnected;
    case QNetworkManagerInterface::NM_STATE_CONNECTED_LOCAL:
        return QNetworkInformation::Reachability::Local;
    case QNetworkManagerInterface::NM_STATE_CONNECTED_SITE:
        return QNetworkInformation::Reachability::Site;
    case QNetworkManagerInterface::NM_STATE_CONNECTED_GLOBAL:
        return QNetworkInformation::Reachability::Online;
    }
    return QNetworkInformation::Reachability::Unknown;
}

QNetworkInformation::TransportMedium
transportMediumFromDeviceType(QNetworkManagerInterface::NMDeviceType type)
{
    switch (type) {
    case QNetworkManagerInterface::NM_DEVICE_TYPE_ETHERNET:
        return QNetworkInformation::TransportMedium::Ethernet;
    case QNetworkManagerInterface::NM_DEVICE_TYPE_WIFI:
        return QNetworkInformation::TransportMedium::WiFi;
    case QNetworkManagerInterface::NM_DEVICE_TYPE_BT:
        return QNetworkInformation::TransportMedium::Bluetooth;
    case QNetworkManagerInterface::NM_DEVICE_TYPE_MODEM:
        return QNetworkInformation::TransportMedium::Cellular;

    case QNetworkManagerInterface::NM_DEVICE_TYPE_UNKNOWN:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_GENERIC:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_UNUSED1:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_UNUSED2:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_OLPC_MESH:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_WIMAX:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_INFINIBAND:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_BOND:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_VLAN:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_ADSL:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_BRIDGE:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_TEAM:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_TUN:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_IP_TUNNEL:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_MACVLAN:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_VXLAN:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_VETH:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_MACSEC:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_DUMMY:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_PPP:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_OVS_INTERFACE:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_OVS_PORT:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_OVS_BRIDGE:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_WPAN:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_6LOWPAN:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_WIREGUARD:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_WIFI_P2P:
    case QNetworkManagerInterface::NM_DEVICE_TYPE_VRF:
        break;
    }
    // While the list is exhaustive of the enum there can be additional
    // entries added in NetworkManager that isn't listed here
    return QNetworkInformation::TransportMedium::Unknown;
}

bool isMeteredFromNMMetered(QNetworkManagerInterface::NMMetered metered)
{
    switch (metered) {
    case QNetworkManagerInterface::NM_METERED_YES:
    case QNetworkManagerInterface::NM_METERED_GUESS_YES:
        return true;
    case QNetworkManagerInterface::NM_METERED_NO:
    case QNetworkManagerInterface::NM_METERED_GUESS_NO:
    case QNetworkManagerInterface::NM_METERED_UNKNOWN:
        return false;
    }
    Q_UNREACHABLE_RETURN(false);
}
} // unnamed namespace

static QString backendName()
{
    return QStringView(QNetworkInformationBackend::PluginNames
                       [QNetworkInformationBackend::PluginNamesLinuxIndex]).toString();
}

QString QNetworkManagerNetworkInformationBackend::name() const
{
    return backendName();
}

class QNetworkManagerNetworkInformationBackendFactory : public QNetworkInformationBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QNetworkInformationBackendFactory_iid)
    Q_INTERFACES(QNetworkInformationBackendFactory)
public:
    QNetworkManagerNetworkInformationBackendFactory() = default;
    ~QNetworkManagerNetworkInformationBackendFactory() = default;
    QString name() const override { return backendName(); }
    QNetworkInformation::Features featuresSupported() const override
    {
        if (!QNetworkManagerInterfaceBase::networkManagerAvailable())
            return {};
        return QNetworkManagerNetworkInformationBackend::featuresSupportedStatic();
    }

    QNetworkInformationBackend *create(QNetworkInformation::Features requiredFeatures) const override
    {
        if ((requiredFeatures & featuresSupported()) != requiredFeatures)
            return nullptr;
        if (!QNetworkManagerInterfaceBase::networkManagerAvailable())
            return nullptr;
        auto backend = new QNetworkManagerNetworkInformationBackend();
        if (!backend->isValid())
            delete std::exchange(backend, nullptr);
        return backend;
    }
private:
    Q_DISABLE_COPY_MOVE(QNetworkManagerNetworkInformationBackendFactory)
};

QNetworkManagerNetworkInformationBackend::QNetworkManagerNetworkInformationBackend()
{
    if (!iface.isValid())
        return;
    iface.setBackend(this);
    onStateChanged(iface.state());
    onConnectivityChanged(iface.connectivityState());
    onDeviceTypeChanged(iface.deviceType());
    onMeteredChanged(iface.meteredState());
}

void QNetworkManagerNetworkInformationBackend::onStateChanged(
        QNetworkManagerInterface::NMState newState)
{
    setReachability(reachabilityFromNMState(newState));
}

void QNetworkManagerNetworkInformationBackend::onConnectivityChanged(
        QNetworkManagerInterface::NMConnectivityState connectivityState)
{
    const bool behindPortal =
            (connectivityState == QNetworkManagerInterface::NM_CONNECTIVITY_PORTAL);
    setBehindCaptivePortal(behindPortal);
}

void QNetworkManagerNetworkInformationBackend::onDeviceTypeChanged(
        QNetworkManagerInterface::NMDeviceType newDevice)
{
    setTransportMedium(transportMediumFromDeviceType(newDevice));
}

void QNetworkManagerNetworkInformationBackend::onMeteredChanged(
        QNetworkManagerInterface::NMMetered metered)
{
    setMetered(isMeteredFromNMMetered(metered));
};

QT_END_NAMESPACE

#include "qnetworkmanagernetworkinformationbackend.moc"
#include "moc_qnetworkmanagernetworkinformationbackend.cpp"
