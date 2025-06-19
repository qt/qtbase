// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qnetworkmanagerservice.h"
#include "qnetworkmanagernetworkinformationbackend.h"

#include <QObject>
#include <QList>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusPendingCall>

#define DBUS_PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"_L1

#define NM_DBUS_INTERFACE "org.freedesktop.NetworkManager"
#define NM_DBUS_SERVICE NM_DBUS_INTERFACE ""_L1

#define NM_DBUS_PATH "/org/freedesktop/NetworkManager"_L1
#define NM_CONNECTION_DBUS_INTERFACE NM_DBUS_SERVICE ".Connection.Active"_L1
#define NM_DEVICE_DBUS_INTERFACE NM_DBUS_SERVICE ".Device"_L1

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace {
constexpr QLatin1StringView propertiesChangedKey = "PropertiesChanged"_L1;
const QString &stateKey()
{
    static auto key = u"State"_s;
    return key;
}
const QString &connectivityKey()
{
    static auto key = u"Connectivity"_s;
    return key;
}
const QString &primaryConnectionKey()
{
    static auto key = u"PrimaryConnection"_s;
    return key;
}
}

QNetworkManagerInterfaceBase::QNetworkManagerInterfaceBase(QObject *parent)
    : QDBusAbstractInterface(NM_DBUS_SERVICE, NM_DBUS_PATH,
                             NM_DBUS_INTERFACE, QDBusConnection::systemBus(), parent)
{
}

bool QNetworkManagerInterfaceBase::networkManagerAvailable()
{
    return QNetworkManagerInterfaceBase().isValid();
}

QNetworkManagerInterface::QNetworkManagerInterface(QObject *parent)
    : QNetworkManagerInterfaceBase(parent)
{
    if (!QDBusAbstractInterface::isValid())
        return;

    PropertiesDBusInterface managerPropertiesInterface(
            NM_DBUS_SERVICE, NM_DBUS_PATH, DBUS_PROPERTIES_INTERFACE,
            QDBusConnection::systemBus());
    QList<QVariant> argumentList;
    argumentList << NM_DBUS_SERVICE;
    QDBusPendingReply<QVariantMap> propsReply = managerPropertiesInterface.callWithArgumentList(
            QDBus::Block, "GetAll"_L1, argumentList);
    if (propsReply.isError()) {
        validDBusConnection = false;
        if (auto error = propsReply.error(); error.type() != QDBusError::AccessDenied)
            qWarning() << "Failed to query NetworkManager properties:" << error.message();
        return;
    }
    propertyMap = propsReply.value();

    validDBusConnection = QDBusConnection::systemBus().connect(NM_DBUS_SERVICE, NM_DBUS_PATH,
            DBUS_PROPERTIES_INTERFACE, propertiesChangedKey, this,
            SLOT(setProperties(QString,QMap<QString,QVariant>,QList<QString>)));
}

QNetworkManagerInterface::~QNetworkManagerInterface()
{
    QDBusConnection::systemBus().disconnect(NM_DBUS_SERVICE, NM_DBUS_PATH,
            DBUS_PROPERTIES_INTERFACE, propertiesChangedKey, this,
            SLOT(setProperties(QString,QMap<QString,QVariant>,QList<QString>)));
}

QNetworkManagerInterface::NMState QNetworkManagerInterface::state() const
{
    auto it = propertyMap.constFind(stateKey());
    if (it != propertyMap.cend())
        return static_cast<QNetworkManagerInterface::NMState>(it->toUInt());
    return QNetworkManagerInterface::NM_STATE_UNKNOWN;
}

QNetworkManagerInterface::NMConnectivityState QNetworkManagerInterface::connectivityState() const
{
    auto it = propertyMap.constFind(connectivityKey());
    if (it != propertyMap.cend())
        return static_cast<NMConnectivityState>(it->toUInt());
    return QNetworkManagerInterface::NM_CONNECTIVITY_UNKNOWN;
}

static std::optional<QDBusInterface> getPrimaryDevice(const QDBusObjectPath &devicePath)
{
    const QDBusInterface connection(NM_DBUS_SERVICE, devicePath.path(),
                                    NM_CONNECTION_DBUS_INTERFACE, QDBusConnection::systemBus());
    if (!connection.isValid())
        return std::nullopt;

    const auto devicePaths = connection.property("Devices").value<QList<QDBusObjectPath>>();
    if (devicePaths.isEmpty())
        return std::nullopt;

    const QDBusObjectPath primaryDevicePath = devicePaths.front();
    return std::make_optional<QDBusInterface>(NM_DBUS_SERVICE, primaryDevicePath.path(),
                                              NM_DEVICE_DBUS_INTERFACE,
                                              QDBusConnection::systemBus());
}

std::optional<QDBusObjectPath> QNetworkManagerInterface::primaryConnectionDevicePath() const
{
    auto it = propertyMap.constFind(primaryConnectionKey());
    if (it != propertyMap.cend())
        return it->value<QDBusObjectPath>();
    return std::nullopt;
}

auto QNetworkManagerInterface::deviceType() const -> NMDeviceType
{
    if (const auto path = primaryConnectionDevicePath())
        return extractDeviceType(*path);
    return NM_DEVICE_TYPE_UNKNOWN;
}

auto QNetworkManagerInterface::meteredState() const -> NMMetered
{
    if (const auto path = primaryConnectionDevicePath())
        return extractDeviceMetered(*path);
    return NM_METERED_UNKNOWN;
}

auto QNetworkManagerInterface::extractDeviceType(const QDBusObjectPath &devicePath) const
        -> NMDeviceType
{
    const auto primaryDevice = getPrimaryDevice(devicePath);
    if (!primaryDevice)
        return NM_DEVICE_TYPE_UNKNOWN;
    if (!primaryDevice->isValid())
        return NM_DEVICE_TYPE_UNKNOWN;
    const QVariant deviceType = primaryDevice->property("DeviceType");
    if (!deviceType.isValid())
        return NM_DEVICE_TYPE_UNKNOWN;
    return static_cast<NMDeviceType>(deviceType.toUInt());
}

auto QNetworkManagerInterface::extractDeviceMetered(const QDBusObjectPath &devicePath) const
        -> NMMetered
{
    const auto primaryDevice = getPrimaryDevice(devicePath);
    if (!primaryDevice)
        return NM_METERED_UNKNOWN;
    if (!primaryDevice->isValid())
        return NM_METERED_UNKNOWN;
    const QVariant metered = primaryDevice->property("Metered");
    if (!metered.isValid())
        return NM_METERED_UNKNOWN;
    return static_cast<NMMetered>(metered.toUInt());
}

void QNetworkManagerInterface::setBackend(QNetworkManagerNetworkInformationBackend *ourBackend)
{
    backend = ourBackend;
}

void QNetworkManagerInterface::setProperties(const QString &interfaceName,
                                             const QMap<QString, QVariant> &map,
                                             const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName);
    Q_UNUSED(invalidatedProperties);

    for (auto i = map.cbegin(), end = map.cend(); i != end; ++i) {
        bool valueChanged = true;

        auto it = propertyMap.lowerBound(i.key());
        if (it != propertyMap.end() && it.key() == i.key()) {
            valueChanged = (it.value() != i.value());
            *it = *i;
        } else {
            propertyMap.insert(it, i.key(), i.value());
        }

        if (valueChanged) {
            if (i.key() == stateKey()) {
                quint32 state = i.value().toUInt();
                backend->onStateChanged(static_cast<NMState>(state));
            } else if (i.key() == connectivityKey()) {
                quint32 state = i.value().toUInt();
                backend->onConnectivityChanged(static_cast<NMConnectivityState>(state));
            } else if (i.key() == primaryConnectionKey()) {
                const QDBusObjectPath devicePath = i->value<QDBusObjectPath>();
                backend->onDeviceTypeChanged(extractDeviceType(devicePath));
                backend->onMeteredChanged(extractDeviceMetered(devicePath));
            } else if (i.key() == "Metered"_L1) {
                backend->onMeteredChanged(static_cast<NMMetered>(i->toUInt()));
            }
        }
    }
}

QT_END_NAMESPACE

#include "moc_qnetworkmanagerservice.cpp"
