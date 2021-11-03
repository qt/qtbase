/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtNetwork/private/qnetworkinformation_p.h>

#include "qnetworkmanagerservice.h"

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
    Q_UNREACHABLE();
    return false;
}
} // unnamed namespace

static QString backendName()
{
    return QString::fromUtf16(QNetworkInformationBackend::PluginNames
                                      [QNetworkInformationBackend::PluginNamesLinuxIndex]);
}

class QNetworkManagerNetworkInformationBackend : public QNetworkInformationBackend
{
    Q_OBJECT
public:
    QNetworkManagerNetworkInformationBackend();
    ~QNetworkManagerNetworkInformationBackend() = default;

    QString name() const override { return backendName(); }
    QNetworkInformation::Features featuresSupported() const override
    {
        if (!isValid())
            return {};
        return featuresSupportedStatic();
    }

    static QNetworkInformation::Features featuresSupportedStatic()
    {
        using Feature = QNetworkInformation::Feature;
        return QNetworkInformation::Features(Feature::Reachability | Feature::CaptivePortal
                                             | Feature::TransportMedium | Feature::Metered);
    }

    bool isValid() const { return iface.isValid(); }

private:
    Q_DISABLE_COPY_MOVE(QNetworkManagerNetworkInformationBackend)

    QNetworkManagerInterface iface;
};

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
    auto updateReachability = [this](QNetworkManagerInterface::NMState newState) {
        setReachability(reachabilityFromNMState(newState));
    };
    updateReachability(iface.state());
    connect(&iface, &QNetworkManagerInterface::stateChanged, this, std::move(updateReachability));

    auto updateBehindCaptivePortal = [this](QNetworkManagerInterface::NMConnectivityState state) {
        const bool behindPortal = (state == QNetworkManagerInterface::NM_CONNECTIVITY_PORTAL);
        setBehindCaptivePortal(behindPortal);
    };
    updateBehindCaptivePortal(iface.connectivityState());
    connect(&iface, &QNetworkManagerInterface::connectivityChanged, this,
            std::move(updateBehindCaptivePortal));

    auto updateTransportMedium = [this](QNetworkManagerInterface::NMDeviceType newDevice) {
        setTransportMedium(transportMediumFromDeviceType(newDevice));
    };
    updateTransportMedium(iface.deviceType());
    connect(&iface, &QNetworkManagerInterface::deviceTypeChanged, this,
            std::move(updateTransportMedium));

    auto updateMetered = [this](QNetworkManagerInterface::NMMetered metered) {
        setMetered(isMeteredFromNMMetered(metered));
    };
    updateMetered(iface.meteredState());
    connect(&iface, &QNetworkManagerInterface::meteredChanged, this, std::move(updateMetered));
}

QT_END_NAMESPACE

#include "qnetworkmanagernetworkinformationbackend.moc"
