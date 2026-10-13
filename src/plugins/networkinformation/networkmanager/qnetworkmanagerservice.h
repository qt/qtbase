// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QNETWORKMANAGERSERVICE_H
#define QNETWORKMANAGERSERVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qvariant.h>
#include <QtCore/qmap.h>
#include <QtDBus/qdbusabstractinterface.h>

#include <optional>

// Matches 'NMDeviceState' from https://developer.gnome.org/NetworkManager/stable/nm-dbus-types.html
enum NMDeviceState {
    NM_DEVICE_STATE_UNKNOWN = 0,
    NM_DEVICE_STATE_UNMANAGED = 10,
    NM_DEVICE_STATE_UNAVAILABLE = 20,
    NM_DEVICE_STATE_DISCONNECTED = 30,
    NM_DEVICE_STATE_PREPARE = 40,
    NM_DEVICE_STATE_CONFIG = 50,
    NM_DEVICE_STATE_NEED_AUTH = 60,
    NM_DEVICE_STATE_IP_CONFIG = 70,
    NM_DEVICE_STATE_ACTIVATED = 100,
    NM_DEVICE_STATE_DEACTIVATING = 110,
    NM_DEVICE_STATE_FAILED = 120
};

QT_BEGIN_NAMESPACE

class QDBusObjectPath;
class QNetworkManagerNetworkInformationBackend;

// This tiny class exists for the purpose of seeing if NetworkManager is available without
// initializing everything the derived/full class needs.
class QNetworkManagerInterfaceBase : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    explicit QNetworkManagerInterfaceBase(QObject *parent = nullptr);
    ~QNetworkManagerInterfaceBase() = default;

    static bool networkManagerAvailable();

private:
    Q_DISABLE_COPY_MOVE(QNetworkManagerInterfaceBase)
};

class QNetworkManagerInterface final : public QNetworkManagerInterfaceBase
{
    Q_OBJECT

public:
    // Matches 'NMState' from https://developer.gnome.org/NetworkManager/stable/nm-dbus-types.html
    enum NMState {
        NM_STATE_UNKNOWN = 0,
        NM_STATE_ASLEEP = 10,
        NM_STATE_DISCONNECTED = 20,
        NM_STATE_DISCONNECTING = 30,
        NM_STATE_CONNECTING = 40,
        NM_STATE_CONNECTED_LOCAL = 50,
        NM_STATE_CONNECTED_SITE = 60,
        NM_STATE_CONNECTED_GLOBAL = 70
    };
    Q_ENUM(NMState)
    // Matches 'NMConnectivityState' from
    // https://developer.gnome.org/NetworkManager/stable/nm-dbus-types.html#NMConnectivityState
    enum NMConnectivityState {
        NM_CONNECTIVITY_UNKNOWN = 0,
        NM_CONNECTIVITY_NONE = 1,
        NM_CONNECTIVITY_PORTAL = 2,
        NM_CONNECTIVITY_LIMITED = 3,
        NM_CONNECTIVITY_FULL = 4,
    };
    Q_ENUM(NMConnectivityState)
    // Matches 'NMDeviceType' from
    // https://developer-old.gnome.org/NetworkManager/stable/nm-dbus-types.html#NMDeviceType
    enum NMDeviceType {
        NM_DEVICE_TYPE_UNKNOWN = 0,
        NM_DEVICE_TYPE_GENERIC = 14,
        NM_DEVICE_TYPE_ETHERNET = 1,
        NM_DEVICE_TYPE_WIFI = 2,
        NM_DEVICE_TYPE_UNUSED1 = 3,
        NM_DEVICE_TYPE_UNUSED2 = 4,
        NM_DEVICE_TYPE_BT = 5,
        NM_DEVICE_TYPE_OLPC_MESH = 6,
        NM_DEVICE_TYPE_WIMAX = 7,
        NM_DEVICE_TYPE_MODEM = 8,
        NM_DEVICE_TYPE_INFINIBAND = 9,
        NM_DEVICE_TYPE_BOND = 10,
        NM_DEVICE_TYPE_VLAN = 11,
        NM_DEVICE_TYPE_ADSL = 12,
        NM_DEVICE_TYPE_BRIDGE = 13,
        NM_DEVICE_TYPE_TEAM = 15,
        NM_DEVICE_TYPE_TUN = 16,
        NM_DEVICE_TYPE_IP_TUNNEL = 17,
        NM_DEVICE_TYPE_MACVLAN = 18,
        NM_DEVICE_TYPE_VXLAN = 19,
        NM_DEVICE_TYPE_VETH = 20,
        NM_DEVICE_TYPE_MACSEC = 21,
        NM_DEVICE_TYPE_DUMMY = 22,
        NM_DEVICE_TYPE_PPP = 23,
        NM_DEVICE_TYPE_OVS_INTERFACE = 24,
        NM_DEVICE_TYPE_OVS_PORT = 25,
        NM_DEVICE_TYPE_OVS_BRIDGE = 26,
        NM_DEVICE_TYPE_WPAN = 27,
        NM_DEVICE_TYPE_6LOWPAN = 28,
        NM_DEVICE_TYPE_WIREGUARD = 29,
        NM_DEVICE_TYPE_WIFI_P2P = 30,
        NM_DEVICE_TYPE_VRF = 31,
    };
    // Matches 'NMMetered' from
    // https://developer-old.gnome.org/NetworkManager/stable/nm-dbus-types.html#NMMetered
    enum NMMetered {
        NM_METERED_UNKNOWN,
        NM_METERED_YES,
        NM_METERED_NO,
        NM_METERED_GUESS_YES,
        NM_METERED_GUESS_NO,
    };

    explicit QNetworkManagerInterface(QObject *parent = nullptr);
    ~QNetworkManagerInterface();

    void setBackend(QNetworkManagerNetworkInformationBackend *ourBackend);

    NMState state() const;
    NMConnectivityState connectivityState() const;
    NMDeviceType deviceType() const;
    NMMetered meteredState() const;

    bool isValid() const { return QDBusAbstractInterface::isValid() && validDBusConnection; }

private Q_SLOTS:
    void setProperties(const QString &interfaceName, const QMap<QString, QVariant> &map,
                       const QStringList &invalidatedProperties);

private:
    Q_DISABLE_COPY_MOVE(QNetworkManagerInterface)

    NMDeviceType extractDeviceType(const QDBusObjectPath &devicePath) const;
    NMMetered extractDeviceMetered(const QDBusObjectPath &devicePath) const;

    std::optional<QDBusObjectPath> primaryConnectionDevicePath() const;

    QVariantMap propertyMap;
    QNetworkManagerNetworkInformationBackend *backend = nullptr;
    bool validDBusConnection = true;
};

class PropertiesDBusInterface : public QDBusAbstractInterface
{
public:
    PropertiesDBusInterface(const QString &service, const QString &path, const QString &interface,
                            const QDBusConnection &connection, QObject *parent = nullptr)
        : QDBusAbstractInterface(service, path, interface.toLatin1().data(), connection, parent)
    {
    }
};

QT_END_NAMESPACE

#endif
