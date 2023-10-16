// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnetworkmanagerservice.h"

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

#define DBUS_PROPERTIES_INTERFACE "org.freedesktop.DBus.Properties"

#define NM_DBUS_SERVICE "org.freedesktop.NetworkManager"

#define NM_DBUS_PATH "/org/freedesktop/NetworkManager"
#define NM_DBUS_INTERFACE NM_DBUS_SERVICE
#define NM_CONNECTION_DBUS_INTERFACE NM_DBUS_SERVICE ".Connection.Active"
#define NM_DEVICE_DBUS_INTERFACE NM_DBUS_SERVICE ".Device"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QNetworkManagerInterfaceBase::QNetworkManagerInterfaceBase(QObject *parent)
    : QDBusAbstractInterface(NM_DBUS_SERVICE ""_L1, NM_DBUS_PATH ""_L1,
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
            NM_DBUS_SERVICE ""_L1, NM_DBUS_PATH ""_L1, DBUS_PROPERTIES_INTERFACE,
            QDBusConnection::systemBus());
    QList<QVariant> argumentList;
    argumentList << NM_DBUS_INTERFACE ""_L1;
    QDBusPendingReply<QVariantMap> propsReply = managerPropertiesInterface.callWithArgumentList(
            QDBus::Block, "GetAll"_L1, argumentList);
    if (propsReply.isError()) {
        validDBusConnection = false;
        if (auto error = propsReply.error(); error.type() != QDBusError::AccessDenied)
            qWarning() << "Failed to query NetworkManager properties:" << error.message();
        return;
    }
    propertyMap = propsReply.value();

    validDBusConnection = QDBusConnection::systemBus().connect(NM_DBUS_SERVICE ""_L1, NM_DBUS_PATH ""_L1,
            DBUS_PROPERTIES_INTERFACE""_L1, "PropertiesChanged"_L1, this,
            SLOT(setProperties(QString,QMap<QString,QVariant>,QList<QString>)));
}

QNetworkManagerInterface::~QNetworkManagerInterface()
{
    QDBusConnection::systemBus().disconnect(NM_DBUS_SERVICE ""_L1, NM_DBUS_PATH ""_L1,
            DBUS_PROPERTIES_INTERFACE ""_L1, "PropertiesChanged"_L1, this,
            SLOT(setProperties(QString,QMap<QString,QVariant>,QList<QString>)));
}

QNetworkManagerInterface::NMState QNetworkManagerInterface::state() const
{
    if (propertyMap.contains("State"))
        return static_cast<QNetworkManagerInterface::NMState>(propertyMap.value("State").toUInt());
    return QNetworkManagerInterface::NM_STATE_UNKNOWN;
}

QNetworkManagerInterface::NMConnectivityState QNetworkManagerInterface::connectivityState() const
{
    if (propertyMap.contains("Connectivity"))
        return static_cast<NMConnectivityState>(propertyMap.value("Connectivity").toUInt());
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
    auto it = propertyMap.constFind(u"PrimaryConnection"_s);
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
            if (i.key() == "State"_L1) {
                quint32 state = i.value().toUInt();
                Q_EMIT stateChanged(static_cast<NMState>(state));
            } else if (i.key() == "Connectivity"_L1) {
                quint32 state = i.value().toUInt();
                Q_EMIT connectivityChanged(static_cast<NMConnectivityState>(state));
            } else if (i.key() == "PrimaryConnection"_L1) {
                const QDBusObjectPath devicePath = i->value<QDBusObjectPath>();
                Q_EMIT deviceTypeChanged(extractDeviceType(devicePath));
                Q_EMIT meteredChanged(extractDeviceMetered(devicePath));
            } else if (i.key() == "Metered"_L1) {
                Q_EMIT meteredChanged(static_cast<NMMetered>(i->toUInt()));
            }
        }
    }
}

QT_END_NAMESPACE

#include "moc_qnetworkmanagerservice.cpp"
