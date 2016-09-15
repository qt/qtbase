/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QObject>
#include <QList>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusPendingCall>

#include "qofonoservice_linux_p.h"

#ifndef QT_NO_DBUS

QDBusArgument &operator<<(QDBusArgument &argument, const ObjectPathProperties &item)
{
    argument.beginStructure();
    argument << item.path << item.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, ObjectPathProperties &item)
{
    argument.beginStructure();
    argument >> item.path >> item.properties;
    argument.endStructure();
    return argument;
}

QT_BEGIN_NAMESPACE

QOfonoManagerInterface::QOfonoManagerInterface( QObject *parent)
        : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                                 QLatin1String(OFONO_MANAGER_PATH),
                                 OFONO_MANAGER_INTERFACE,
                                 QDBusConnection::systemBus(), parent)
{
    qDBusRegisterMetaType<ObjectPathProperties>();
    qDBusRegisterMetaType<PathPropertiesList>();

    QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
                           QLatin1String(OFONO_MANAGER_PATH),
                           QLatin1String(OFONO_MANAGER_INTERFACE),
                           QLatin1String("ModemAdded"),
                           this,SLOT(modemAdded(QDBusObjectPath, QVariantMap)));
    QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
                           QLatin1String(OFONO_MANAGER_PATH),
                           QLatin1String(OFONO_MANAGER_INTERFACE),
                           QLatin1String("ModemRemoved"),
                           this,SLOT(modemRemoved(QDBusObjectPath)));
}

QOfonoManagerInterface::~QOfonoManagerInterface()
{
}

QStringList QOfonoManagerInterface::getModems()
{
    if (modemList.isEmpty()) {
        QList<QVariant> argumentList;
        QDBusPendingReply<PathPropertiesList> reply = callWithArgumentList(QDBus::Block, QLatin1String("GetModems"), argumentList);
        reply.waitForFinished();
        if (!reply.isError()) {
            const auto modems = reply.value();
            for (const ObjectPathProperties &modem : modems)
                modemList << modem.path.path();
        }
    }

    return modemList;
}

QString QOfonoManagerInterface::currentModem()
{
    const QStringList modems = getModems();
    for (const QString &modem : modems) {
        QOfonoModemInterface device(modem);
        if (device.isPowered() && device.isOnline()
                && device.interfaces().contains(QStringLiteral("org.ofono.NetworkRegistration")))
        return modem;
    }
    return QString();
}

void QOfonoManagerInterface::modemAdded(const QDBusObjectPath &path, const QVariantMap &/*var*/)
{
    if (!modemList.contains(path.path())) {
        modemList << path.path();
        Q_EMIT modemChanged();
    }
}

void QOfonoManagerInterface::modemRemoved(const QDBusObjectPath &path)
{
    if (modemList.contains(path.path())) {
        modemList.removeOne(path.path());
        Q_EMIT modemChanged();
    }
}


QOfonoModemInterface::QOfonoModemInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                             dbusPathName,
                             OFONO_MODEM_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
    QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
                                         path(),
                                         OFONO_MODEM_INTERFACE,
                                         QLatin1String("PropertyChanged"),
                                         this,SLOT(propertyChanged(QString,QDBusVariant)));
}

QOfonoModemInterface::~QOfonoModemInterface()
{
}

void QOfonoModemInterface::propertyChanged(const QString &name,const QDBusVariant &value)
{
    propertiesMap[name] = value.variant();
}

bool QOfonoModemInterface::isPowered()
{
    QVariant var = getProperty(QStringLiteral("Powered"));
    return qdbus_cast<bool>(var);
}

bool QOfonoModemInterface::isOnline()
{
    QVariant var = getProperty(QStringLiteral("Online"));
    return qdbus_cast<bool>(var);
}

QStringList QOfonoModemInterface::interfaces()
{
    const QVariant var = getProperty(QStringLiteral("Interfaces"));
    return var.toStringList();
}

QVariantMap QOfonoModemInterface::getProperties()
{
    if (propertiesMap.isEmpty()) {
        QList<QVariant> argumentList;
        QDBusPendingReply<QVariantMap> reply = callWithArgumentList(QDBus::Block, QLatin1String("GetProperties"), argumentList);
        if (!reply.isError()) {
            propertiesMap = reply.value();
        }
    }
    return propertiesMap;
}

QVariant QOfonoModemInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property))
        var = map.value(property);
    return var;
}


QOfonoNetworkRegistrationInterface::QOfonoNetworkRegistrationInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                             dbusPathName,
                             OFONO_NETWORK_REGISTRATION_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QOfonoNetworkRegistrationInterface::~QOfonoNetworkRegistrationInterface()
{
}

QString QOfonoNetworkRegistrationInterface::getTechnology()
{
    QVariant var = getProperty(QStringLiteral("Technology"));
    return qdbus_cast<QString>(var);
}

QVariant QOfonoNetworkRegistrationInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property))
        var = map.value(property);
    return var;
}

QVariantMap QOfonoNetworkRegistrationInterface::getProperties()
{
    if (propertiesMap.isEmpty()) {
        QList<QVariant> argumentList;
        QDBusPendingReply<QVariantMap> reply = callWithArgumentList(QDBus::Block, QLatin1String("GetProperties"), argumentList);
        reply.waitForFinished();
        if (!reply.isError()) {
            propertiesMap = reply.value();
        }
    }
    return propertiesMap;
}

QOfonoDataConnectionManagerInterface::QOfonoDataConnectionManagerInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                             dbusPathName,
                             OFONO_DATA_CONNECTION_MANAGER_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
    QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
                                         path(),
                                         QLatin1String(OFONO_MODEM_INTERFACE),
                                         QLatin1String("PropertyChanged"),
                                         this,SLOT(propertyChanged(QString,QDBusVariant)));
}

QOfonoDataConnectionManagerInterface::~QOfonoDataConnectionManagerInterface()
{
}

QStringList QOfonoDataConnectionManagerInterface::contexts()
{
    if (contextList.isEmpty()) {
        QDBusPendingReply<PathPropertiesList > reply = call(QLatin1String("GetContexts"));
        reply.waitForFinished();
        if (!reply.isError()) {
            const auto contexts = reply.value();
            for (const ObjectPathProperties &context : contexts)
                contextList << context.path.path();
        }
    }
    return contextList;
}

PathPropertiesList QOfonoDataConnectionManagerInterface::contextsWithProperties()
{
    if (contextListProperties.isEmpty()) {
        QDBusPendingReply<PathPropertiesList > reply = call(QLatin1String("GetContexts"));
        reply.waitForFinished();
        if (!reply.isError()) {
            contextListProperties = reply.value();
        }
    }
    return contextListProperties;
}

bool QOfonoDataConnectionManagerInterface::roamingAllowed()
{
    QVariant var = getProperty(QStringLiteral("RoamingAllowed"));
    return qdbus_cast<bool>(var);
}

QString QOfonoDataConnectionManagerInterface::bearer()
{
    QVariant var = getProperty(QStringLiteral("Bearer"));
    return qdbus_cast<QString>(var);
}

QVariant QOfonoDataConnectionManagerInterface::getProperty(const QString &property)
{
    return getProperties().value(property);
}

QVariantMap &QOfonoDataConnectionManagerInterface::getProperties()
{
    if (propertiesMap.isEmpty()) {
        QList<QVariant> argumentList;
        QDBusPendingReply<QVariantMap> reply = callWithArgumentList(QDBus::Block, QLatin1String("GetProperties"), argumentList);
        if (!reply.isError()) {
            propertiesMap = reply.value();
        }
    }
    return propertiesMap;
}

void QOfonoDataConnectionManagerInterface::propertyChanged(const QString &name, const QDBusVariant &value)
{
    propertiesMap[name] = value.variant();
    if (name == QLatin1String("RoamingAllowed"))
        Q_EMIT roamingAllowedChanged(value.variant().toBool());
}


QOfonoConnectionContextInterface::QOfonoConnectionContextInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                             dbusPathName,
                             OFONO_CONNECTION_CONTEXT_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
    QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
                                         path(),
                                         QLatin1String(OFONO_MODEM_INTERFACE),
                                         QLatin1String("PropertyChanged"),
                                         this,SLOT(propertyChanged(QString,QDBusVariant)));
}

QOfonoConnectionContextInterface::~QOfonoConnectionContextInterface()
{
}

QVariantMap QOfonoConnectionContextInterface::getProperties()
{
    if (propertiesMap.isEmpty()) {
        QList<QVariant> argumentList;
        QDBusPendingReply<QVariantMap> reply = callWithArgumentList(QDBus::Block, QLatin1String("GetProperties"), argumentList);
        if (!reply.isError()) {
            propertiesMap = reply.value();
        }
    }
    return propertiesMap;
}

void QOfonoConnectionContextInterface::propertyChanged(const QString &name, const QDBusVariant &value)
{
    propertiesMap[name] = value.variant();
}

QVariant QOfonoConnectionContextInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property))
        var = map.value(property);
    return var;
}

bool QOfonoConnectionContextInterface::active()
{
    QVariant var = getProperty(QStringLiteral("Active"));
    return qdbus_cast<bool>(var);
}

QString QOfonoConnectionContextInterface::accessPointName()
{
    QVariant var = getProperty(QStringLiteral("AccessPointName"));
    return qdbus_cast<QString>(var);
}

QString QOfonoConnectionContextInterface::name()
{
    QVariant var = getProperty(QStringLiteral("Name"));
    return qdbus_cast<QString>(var);
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
