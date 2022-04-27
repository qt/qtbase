/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

QT_BEGIN_NAMESPACE

QNetworkManagerInterfaceBase::QNetworkManagerInterfaceBase(QObject *parent)
    : QDBusAbstractInterface(QLatin1String(NM_DBUS_SERVICE), QLatin1String(NM_DBUS_PATH),
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
    if (!isValid())
        return;

    PropertiesDBusInterface managerPropertiesInterface(
            QLatin1String(NM_DBUS_SERVICE), QLatin1String(NM_DBUS_PATH), DBUS_PROPERTIES_INTERFACE,
            QDBusConnection::systemBus());
    QList<QVariant> argumentList;
    argumentList << QLatin1String(NM_DBUS_INTERFACE);
    QDBusPendingReply<QVariantMap> propsReply = managerPropertiesInterface.callWithArgumentList(
            QDBus::Block, QLatin1String("GetAll"), argumentList);
    if (!propsReply.isError()) {
        propertyMap = propsReply.value();
    } else {
        qWarning() << "propsReply" << propsReply.error().message();
    }

    QDBusConnection::systemBus().connect(
            QLatin1String(NM_DBUS_SERVICE), QLatin1String(NM_DBUS_PATH),
            QLatin1String(DBUS_PROPERTIES_INTERFACE), QLatin1String("PropertiesChanged"), this,
            SLOT(setProperties(QString, QMap<QString, QVariant>, QList<QString>)));
}

QNetworkManagerInterface::~QNetworkManagerInterface()
{
    QDBusConnection::systemBus().disconnect(
            QLatin1String(NM_DBUS_SERVICE), QLatin1String(NM_DBUS_PATH),
            QLatin1String(DBUS_PROPERTIES_INTERFACE), QLatin1String("PropertiesChanged"), this,
            SLOT(setProperties(QString, QMap<QString, QVariant>, QList<QString>)));
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

void QNetworkManagerInterface::setProperties(const QString &interfaceName,
                                             const QMap<QString, QVariant> &map,
                                             const QStringList &invalidatedProperties)
{
    Q_UNUSED(interfaceName);
    Q_UNUSED(invalidatedProperties);

    for (auto i = map.cbegin(), end = map.cend(); i != end; ++i) {
        const bool isState = i.key() == QLatin1String("State");
        const bool isConnectivity = i.key() == QLatin1String("Connectivity");
        bool stateUpdate = isState;
        bool connectivityUpdate = isConnectivity;

        auto it = propertyMap.lowerBound(i.key());
        if (it != propertyMap.end() && it.key() == i.key()) {
            stateUpdate &= (it.value() != i.value());
            connectivityUpdate &= (it.value() != i.value());
            *it = *i;
        } else {
            propertyMap.insert(it, i.key(), i.value());
        }

        if (stateUpdate) {
            quint32 state = i.value().toUInt();
            Q_EMIT stateChanged(static_cast<NMState>(state));
        } else if (connectivityUpdate) {
            quint32 state = i.value().toUInt();
            Q_EMIT connectivityChanged(static_cast<NMConnectivityState>(state));
        }
    }
}

QT_END_NAMESPACE

#include "moc_qnetworkmanagerservice.cpp"
