/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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

#ifndef QT_NO_BEARERMANAGEMENT
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
}

QOfonoManagerInterface::~QOfonoManagerInterface()
{
}

QList <QDBusObjectPath> QOfonoManagerInterface::getModems()
{
    QList <QDBusObjectPath> modemList;
    QList<QVariant> argumentList;
    QDBusReply<PathPropertiesList > reply = this->asyncCallWithArgumentList(QLatin1String("GetModems"), argumentList);
    if (reply.isValid()) {
        foreach (ObjectPathProperties modem, reply.value()) {
            modemList << modem.path;
        }
    }

    return modemList;
}

QDBusObjectPath QOfonoManagerInterface::currentModem()
{
    QList<QDBusObjectPath> modems = getModems();
    foreach (const QDBusObjectPath &modem, modems) {
        QOfonoModemInterface device(modem.path());
        if (device.isPowered() && device.isOnline())
        return modem;
    }
    return QDBusObjectPath();
}


void QOfonoManagerInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoManagerInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
        if (!connection().connect(QLatin1String(OFONO_SERVICE),
                               QLatin1String(OFONO_MANAGER_PATH),
                               QLatin1String(OFONO_MANAGER_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               this,SIGNAL(propertyChanged(QString,QDBusVariant)))) {
            qWarning() << "PropertyCHanged not connected";
        }
    }

    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QOfonoManagerInterface::propertyChangedContext);
    if (signal == propertyChangedContextSignal) {
        QOfonoDBusHelper *helper;
        helper = new QOfonoDBusHelper(this);

        QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
                               QLatin1String(OFONO_MANAGER_PATH),
                               QLatin1String(OFONO_MANAGER_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               helper,SLOT(propertyChanged(QString,QDBusVariant)));


        QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)));
    }
}

void QOfonoManagerInterface::disconnectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoManagerInterface::propertyChanged);
    if (signal == propertyChangedSignal) {

    }
}

QVariant QOfonoManagerInterface::getProperty(const QString &property)
{
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        return map.value(property);
    } else {
        qDebug() << Q_FUNC_INFO << "does not contain" << property;
    }
    return QVariant();
}

QVariantMap QOfonoManagerInterface::getProperties()
{
    QDBusReply<QVariantMap > reply = this->call(QLatin1String("GetProperties"));
    if (reply.isValid())
        return reply.value();
    else
        return QVariantMap();
}

QOfonoDBusHelper::QOfonoDBusHelper(QObject * parent)
        : QObject(parent)
{
}

QOfonoDBusHelper::~QOfonoDBusHelper()
{
}

void QOfonoDBusHelper::propertyChanged(const QString &item, const QDBusVariant &var)
{
    QDBusMessage msg = this->message();
    Q_EMIT propertyChangedContext(msg.path() ,item, var);
}


QOfonoModemInterface::QOfonoModemInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                             dbusPathName,
                             OFONO_MODEM_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QOfonoModemInterface::~QOfonoModemInterface()
{
}

bool QOfonoModemInterface::isPowered()
{
    QVariant var = getProperty("Powered");
    return qdbus_cast<bool>(var);
}

bool QOfonoModemInterface::isOnline()
{
    QVariant var = getProperty("Online");
    return qdbus_cast<bool>(var);
}

QString QOfonoModemInterface::getName()
{
    QVariant var = getProperty("Name");
    return qdbus_cast<QString>(var);
}

QString QOfonoModemInterface::getManufacturer()
{
    QVariant var = getProperty("Manufacturer");
    return qdbus_cast<QString>(var);

}

QString QOfonoModemInterface::getModel()
{

    QVariant var = getProperty("Model");
    return qdbus_cast<QString>(var);
}

QString QOfonoModemInterface::getRevision()
{
    QVariant var = getProperty("Revision");
    return qdbus_cast<QString>(var);

}
QString QOfonoModemInterface::getSerial()
{
    QVariant var = getProperty("Serial");
    return qdbus_cast<QString>(var);

}

QStringList QOfonoModemInterface::getFeatures()
{
    //sms, sim
    QVariant var = getProperty("Features");
    return qdbus_cast<QStringList>(var);
}

QStringList QOfonoModemInterface::getInterfaces()
{
    QVariant var = getProperty("Interfaces");
    return qdbus_cast<QStringList>(var);
}

QString QOfonoModemInterface::defaultInterface()
{
    foreach (const QString &modem,getInterfaces()) {
     return modem;
    }
    return QString();
}


void QOfonoModemInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoModemInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
            if (!connection().connect(QLatin1String(OFONO_SERVICE),
                                   this->path(),
                                   QLatin1String(OFONO_MODEM_INTERFACE),
                                   QLatin1String("PropertyChanged"),
                                   this,SIGNAL(propertyChanged(QString,QDBusVariant)))) {
                qWarning() << "PropertyCHanged not connected";
            }
        }

    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QOfonoModemInterface::propertyChangedContext);
        if (signal == propertyChangedContextSignal) {
            QOfonoDBusHelper *helper;
            helper = new QOfonoDBusHelper(this);

            QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
                                   this->path(),
                                   QLatin1String(OFONO_MODEM_INTERFACE),
                                   QLatin1String("PropertyChanged"),
                                   helper,SLOT(propertyChanged(QString,QDBusVariant)));


            QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                    this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)), Qt::UniqueConnection);
        }}

void QOfonoModemInterface::disconnectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoModemInterface::propertyChanged);
    if (signal == propertyChangedSignal) {

    }
}

QVariantMap QOfonoModemInterface::getProperties()
{
    QDBusReply<QVariantMap > reply = this->call(QLatin1String("GetProperties"));
    return reply.value();
}

QVariant QOfonoModemInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    } else {
        qDebug() << Q_FUNC_INFO << "does not contain" << property;
    }
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

QString QOfonoNetworkRegistrationInterface::getStatus()
{
    /*
                "unregistered"  Not registered to any network
                "registered"    Registered to home network
                "searching"     Not registered, but searching
                "denied"        Registration has been denied
                "unknown"       Status is unknown
                "roaming"       Registered, but roaming*/
    QVariant var = getProperty("Status");
    return qdbus_cast<QString>(var);
}

quint16 QOfonoNetworkRegistrationInterface::getLac()
{
    QVariant var = getProperty("LocationAreaCode");
    return var.value<quint16>();
}


quint32 QOfonoNetworkRegistrationInterface::getCellId()
{
    QVariant var = getProperty("CellId");
    return var.value<quint32>();
}

QString QOfonoNetworkRegistrationInterface::getTechnology()
{
    // "gsm", "edge", "umts", "hspa","lte"
    QVariant var = getProperty("Technology");
    return qdbus_cast<QString>(var);
}

QString QOfonoNetworkRegistrationInterface::getOperatorName()
{
    QVariant var = getProperty("Name");
    return qdbus_cast<QString>(var);
}

int QOfonoNetworkRegistrationInterface::getSignalStrength()
{
    QVariant var = getProperty("Strength");
    return qdbus_cast<int>(var);

}

QString QOfonoNetworkRegistrationInterface::getBaseStation()
{
    QVariant var = getProperty("BaseStation");
    return qdbus_cast<QString>(var);
}

QList <QDBusObjectPath> QOfonoNetworkRegistrationInterface::getOperators()
{
    QList <QDBusObjectPath> operatorList;
    QList<QVariant> argumentList;
    QDBusReply<PathPropertiesList > reply = this->asyncCallWithArgumentList(QLatin1String("GetOperators"),
                                                                                argumentList);
    if (reply.isValid()) {
        foreach (ObjectPathProperties netop, reply.value()) {
            operatorList << netop.path;
        }
    }
    return operatorList;
}

void QOfonoNetworkRegistrationInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoNetworkRegistrationInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
        if (!connection().connect(QLatin1String(OFONO_SERVICE),
                               this->path(),
                               QLatin1String(OFONO_NETWORK_REGISTRATION_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               this,SIGNAL(propertyChanged(QString,QDBusVariant)))) {
            qWarning() << "PropertyCHanged not connected";
        }
    }

    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QOfonoNetworkRegistrationInterface::propertyChangedContext);
    if (signal == propertyChangedContextSignal) {
        QOfonoDBusHelper *helper;
        helper = new QOfonoDBusHelper(this);

        QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
                               this->path(),
                               QLatin1String(OFONO_NETWORK_REGISTRATION_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               helper,SLOT(propertyChanged(QString,QDBusVariant)));


        QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)), Qt::UniqueConnection);
    }
}

void QOfonoNetworkRegistrationInterface::disconnectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoNetworkRegistrationInterface::propertyChanged);
    if (signal == propertyChangedSignal) {

    }
}

QVariant QOfonoNetworkRegistrationInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    } else {
        qDebug() << Q_FUNC_INFO << "does not contain" << property;
    }
    return var;
}

QVariantMap QOfonoNetworkRegistrationInterface::getProperties()
{
    QDBusReply<QVariantMap > reply =  this->call(QLatin1String("GetProperties"));
    return reply.value();
}



QOfonoNetworkOperatorInterface::QOfonoNetworkOperatorInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                             dbusPathName,
                             OFONO_NETWORK_OPERATOR_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QOfonoNetworkOperatorInterface::~QOfonoNetworkOperatorInterface()
{
}

QString QOfonoNetworkOperatorInterface::getName()
{
    QVariant var = getProperty("Name");
    return qdbus_cast<QString>(var);
}

QString QOfonoNetworkOperatorInterface::getStatus()
{
    // "unknown", "available", "current" and "forbidden"
    QVariant var = getProperty("Status");
    return qdbus_cast<QString>(var);
}

QString QOfonoNetworkOperatorInterface::getMcc()
{
    QVariant var = getProperty("MobileCountryCode");
    return qdbus_cast<QString>(var);
}

QString QOfonoNetworkOperatorInterface::getMnc()
{
    QVariant var = getProperty("MobileNetworkCode");
    return qdbus_cast<QString>(var);
}

QStringList QOfonoNetworkOperatorInterface::getTechnologies()
{
    QVariant var = getProperty("Technologies");
    return qdbus_cast<QStringList>(var);
}

void QOfonoNetworkOperatorInterface::connectNotify(const QMetaMethod &signal)
{
    Q_UNUSED(signal);
//    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoNetworkOperatorInterface::propertyChanged);
//    if (signal == propertyChangedSignal) {
//        if (!connection().connect(QLatin1String(OFONO_SERVICE),
//                               this->path(),
//                               QLatin1String(OFONO_NETWORK_OPERATOR_INTERFACE),
//                               QLatin1String("PropertyChanged"),
//                               this,SIGNAL(propertyChanged(QString,QDBusVariant)))) {
//            qWarning() << "PropertyCHanged not connected";
//        }
//    }

//    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QOfonoNetworkOperatorInterface::propertyChangedContext);
//    if (signal == propertyChangedContextSignal) {
//        QOfonoDBusHelper *helper;
//        helper = new QOfonoDBusHelper(this);

//        QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
//                               this->path(),
//                               QLatin1String(OFONO_NETWORK_OPERATOR_INTERFACE),
//                               QLatin1String("PropertyChanged"),
//                               helper,SLOT(propertyChanged(QString,QDBusVariant)));


//        QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
//                this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)), Qt::UniqueConnection);
//    }
}

void QOfonoNetworkOperatorInterface::disconnectNotify(const QMetaMethod &signal)
{
    Q_UNUSED(signal);
//    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoNetworkOperatorInterface::propertyChanged);
//    if (signal == propertyChangedSignal) {

//    }
}

QVariant QOfonoNetworkOperatorInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    } else {
        qDebug() << Q_FUNC_INFO << "does not contain" << property;
    }
    return var;
}

QVariantMap QOfonoNetworkOperatorInterface::getProperties()
{
    QDBusReply<QVariantMap > reply =  this->call(QLatin1String("GetProperties"));
    return reply.value();
}

QOfonoSimInterface::QOfonoSimInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                             dbusPathName,
                             OFONO_SIM_MANAGER_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QOfonoSimInterface::~QOfonoSimInterface()
{
}

bool QOfonoSimInterface::isPresent()
{
    QVariant var = getProperty("Present");
    return qdbus_cast<bool>(var);
}

QString QOfonoSimInterface::getHomeMcc()
{
    QVariant var = getProperty("MobileCountryCode");
    return qdbus_cast<QString>(var);
}

QString QOfonoSimInterface::getHomeMnc()
{
    QVariant var = getProperty("MobileNetworkCode");
    return qdbus_cast<QString>(var);
}

//    QStringList subscriberNumbers();
//    QMap<QString,QString> serviceNumbers();
QString QOfonoSimInterface::pinRequired()
{
    QVariant var = getProperty("PinRequired");
    return qdbus_cast<QString>(var);
}

QString QOfonoSimInterface::lockedPins()
{
    QVariant var = getProperty("LockedPins");
    return qdbus_cast<QString>(var);
}

QString QOfonoSimInterface::cardIdentifier()
{
    QVariant var = getProperty("CardIdentifier");
    return qdbus_cast<QString>(var);
}

void QOfonoSimInterface::connectNotify(const QMetaMethod &signal)
{
    Q_UNUSED(signal);
//    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoSimInterface::propertyChanged);
//    if (signal == propertyChangedSignal) {
//        if (!connection().connect(QLatin1String(OFONO_SERVICE),
//                               this->path(),
//                               QLatin1String(OFONO_SIM_MANAGER_INTERFACE),
//                               QLatin1String("PropertyChanged"),
//                               this,SIGNAL(propertyChanged(QString,QDBusVariant)))) {
//            qWarning() << "PropertyCHanged not connected";
//        }
//    }

//    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QOfonoSimInterface::propertyChangedContext);
//    if (signal == propertyChangedContextSignal) {
//        QOfonoDBusHelper *helper;
//        helper = new QOfonoDBusHelper(this);

//        QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
//                               this->path(),
//                               QLatin1String(OFONO_SIM_MANAGER_INTERFACE),
//                               QLatin1String("PropertyChanged"),
//                               helper,SLOT(propertyChanged(QString,QDBusVariant)));


//        QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
//                this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)), Qt::UniqueConnection);
//    }
}

void QOfonoSimInterface::disconnectNotify(const QMetaMethod &signal)
{
    Q_UNUSED(signal);
//    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoSimInterface::propertyChanged);
//    if (signal == propertyChangedSignal) {

//    }
}

QVariant QOfonoSimInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    } else {
        qDebug() << Q_FUNC_INFO << "does not contain" << property;
    }
    return var;
}

QVariantMap QOfonoSimInterface::getProperties()
{
    QDBusReply<QVariantMap > reply =  this->call(QLatin1String("GetProperties"));
    return reply.value();
}

QOfonoDataConnectionManagerInterface::QOfonoDataConnectionManagerInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                             dbusPathName,
                             OFONO_DATA_CONNECTION_MANAGER_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QOfonoDataConnectionManagerInterface::~QOfonoDataConnectionManagerInterface()
{
}

QList<QDBusObjectPath> QOfonoDataConnectionManagerInterface::getPrimaryContexts()
{
    QList <QDBusObjectPath> contextList;
    QList<QVariant> argumentList;
    QDBusReply<PathPropertiesList > reply = this->asyncCallWithArgumentList(QLatin1String("GetContexts"),
                                                                         argumentList);
    if (reply.isValid()) {
        foreach (ObjectPathProperties context, reply.value()) {
            contextList << context.path;
        }
    }
    return contextList;
}

bool QOfonoDataConnectionManagerInterface::isAttached()
{
    QVariant var = getProperty("Attached");
    return qdbus_cast<bool>(var);
}

bool QOfonoDataConnectionManagerInterface::isRoamingAllowed()
{
    QVariant var = getProperty("RoamingAllowed");
    return qdbus_cast<bool>(var);
}

bool QOfonoDataConnectionManagerInterface::isPowered()
{
    QVariant var = getProperty("Powered");
    return qdbus_cast<bool>(var);
}

void QOfonoDataConnectionManagerInterface::connectNotify(const QMetaMethod &signal)
{
    Q_UNUSED(signal);
//    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoDataConnectionManagerInterface::propertyChanged);
//    if (signal == propertyChangedSignal) {
//        if (!connection().connect(QLatin1String(OFONO_SERVICE),
//                               this->path(),
//                               QLatin1String(OFONO_DATA_CONNECTION_MANAGER_INTERFACE),
//                               QLatin1String("PropertyChanged"),
//                               this,SIGNAL(propertyChanged(QString,QDBusVariant)))) {
//            qWarning() << "PropertyCHanged not connected";
//        }
//    }

//    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QOfonoDataConnectionManagerInterface::propertyChangedContext);
//    if (signal == propertyChangedContextSignal) {
//        QOfonoDBusHelper *helper;
//        helper = new QOfonoDBusHelper(this);

//        QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
//                               this->path(),
//                               QLatin1String(OFONO_DATA_CONNECTION_MANAGER_INTERFACE),
//                               QLatin1String("PropertyChanged"),
//                               helper,SLOT(propertyChanged(QString,QDBusVariant)));


//        QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
//                this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)), Qt::UniqueConnection);
//    }
}

void QOfonoDataConnectionManagerInterface::disconnectNotify(const QMetaMethod &signal)
{
    Q_UNUSED(signal);
//    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoDataConnectionManagerInterface::propertyChanged);
//    if (signal == propertyChangedSignal) {

//    }
}

QVariant QOfonoDataConnectionManagerInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    } else {
        qDebug() << Q_FUNC_INFO << "does not contain" << property;
    }
    return var;
}

QVariantMap QOfonoDataConnectionManagerInterface::getProperties()
{
    QDBusReply<QVariantMap > reply =  this->call(QLatin1String("GetProperties"));
    return reply.value();
}

QOfonoConnectionContextInterface::QOfonoConnectionContextInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                             dbusPathName,
                             OFONO_DATA_CONTEXT_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QOfonoConnectionContextInterface::~QOfonoConnectionContextInterface()
{
}

bool QOfonoConnectionContextInterface::isActive()
{
    QVariant var = getProperty("Active");
    return qdbus_cast<bool>(var);
}

QString QOfonoConnectionContextInterface::getApName()
{
    QVariant var = getProperty("AccessPointName");
    return qdbus_cast<QString>(var);
}

QString QOfonoConnectionContextInterface::getType()
{
    QVariant var = getProperty("Type");
    return qdbus_cast<QString>(var);
}

QString QOfonoConnectionContextInterface::getName()
{
    QVariant var = getProperty("Name");
    return qdbus_cast<QString>(var);
}

QVariantMap QOfonoConnectionContextInterface::getSettings()
{
    QVariant var = getProperty("Settings");
    return qdbus_cast<QVariantMap>(var);
}

QString QOfonoConnectionContextInterface::getInterface()
{
    QVariant var = getProperty("Interface");
    return qdbus_cast<QString>(var);
}

QString QOfonoConnectionContextInterface::getAddress()
{
    QVariant var = getProperty("Address");
    return qdbus_cast<QString>(var);
}

bool QOfonoConnectionContextInterface::setActive(bool on)
{
//    this->setProperty("Active", QVariant(on));

    return setProp("Active", QVariant::fromValue(on));
}

bool QOfonoConnectionContextInterface::setApn(const QString &name)
{
    return setProp("AccessPointName", QVariant::fromValue(name));
}

void QOfonoConnectionContextInterface::connectNotify(const QMetaMethod &signal)
{
    Q_UNUSED(signal);
//    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoConnectionContextInterface::propertyChanged);
//    if (signal == propertyChangedSignal) {
//        if (!connection().connect(QLatin1String(OFONO_SERVICE),
//                               this->path(),
//                               QLatin1String(OFONO_DATA_CONTEXT_INTERFACE),
//                               QLatin1String("PropertyChanged"),
//                               this,SIGNAL(propertyChanged(QString,QDBusVariant)))) {
//            qWarning() << "PropertyCHanged not connected";
//        }
//    }

//    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QOfonoConnectionContextInterface::propertyChangedContext);
//    if (signal == propertyChangedContextSignal) {
//        QOfonoDBusHelper *helper;
//        helper = new QOfonoDBusHelper(this);

//        QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
//                               this->path(),
//                               QLatin1String(OFONO_DATA_CONTEXT_INTERFACE),
//                               QLatin1String("PropertyChanged"),
//                               helper,SLOT(propertyChanged(QString,QDBusVariant)));


//        QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
//                this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)), Qt::UniqueConnection);
//    }
}

void QOfonoConnectionContextInterface::disconnectNotify(const QMetaMethod &signal)
{
    Q_UNUSED(signal);
//    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoConnectionContextInterface::propertyChanged);
//    if (signal == propertyChangedSignal) {

//    }
}

QVariant QOfonoConnectionContextInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    } else {
        qDebug() << Q_FUNC_INFO << "does not contain" << property;
    }
    return var;
}

QVariantMap QOfonoConnectionContextInterface::getProperties()
{
    QDBusReply<QVariantMap > reply =  this->call(QLatin1String("GetProperties"));
    return reply.value();
}

bool QOfonoConnectionContextInterface::setProp(const QString &property, const QVariant &var)
{
    QList<QVariant> args;
    args << QVariant::fromValue(property) << QVariant::fromValue(QDBusVariant(var));

    QDBusMessage reply = this->callWithArgumentList(QDBus::AutoDetect,
                                                    QLatin1String("SetProperty"),
                                                    args);
    bool ok = true;
    if (reply.type() != QDBusMessage::ReplyMessage) {
        qWarning() << reply.errorMessage();
        ok = false;
    }
    qWarning() << reply.errorMessage();
    return ok;
}

QOfonoSmsInterface::QOfonoSmsInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(OFONO_SERVICE),
                             dbusPathName,
                             OFONO_SMS_MANAGER_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QOfonoSmsInterface::~QOfonoSmsInterface()
{
}

void QOfonoSmsInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoSmsInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
        if (!connection().connect(QLatin1String(OFONO_SERVICE),
                                 this->path(),
                                 QLatin1String(OFONO_SMS_MANAGER_INTERFACE),
                                 QLatin1String("PropertyChanged"),
                                 this,SIGNAL(propertyChanged(QString,QDBusVariant)))) {
            qWarning() << "PropertyCHanged not connected";
        }
    }

    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QOfonoSmsInterface::propertyChangedContext);
    if (signal == propertyChangedContextSignal) {
        QOfonoDBusHelper *helper;
        helper = new QOfonoDBusHelper(this);

        QDBusConnection::systemBus().connect(QLatin1String(OFONO_SERVICE),
                               this->path(),
                               QLatin1String(OFONO_SMS_MANAGER_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               helper,SLOT(propertyChanged(QString,QDBusVariant)));


        QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                         this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)));
    }

    static const QMetaMethod immediateMessageSignal = QMetaMethod::fromSignal(&QOfonoSmsInterface::immediateMessage);
    if (signal == immediateMessageSignal) {
        if (!connection().connect(QLatin1String(OFONO_SERVICE),
                                 this->path(),
                                 QLatin1String(OFONO_SMS_MANAGER_INTERFACE),
                                 QLatin1String("ImmediateMessage"),
                                 this,SIGNAL(immediateMessage(QString,QVariantMap)))) {
            qWarning() << "PropertyCHanged not connected";
        }
    }

    static const QMetaMethod incomingMessageSignal = QMetaMethod::fromSignal(&QOfonoSmsInterface::incomingMessage);
    if (signal == incomingMessageSignal) {
        if (!connection().connect(QLatin1String(OFONO_SERVICE),
                                 this->path(),
                                 QLatin1String(OFONO_SMS_MANAGER_INTERFACE),
                                 QLatin1String("IncomingMessage"),
                                 this,SIGNAL(incomingMessage(QString,QVariantMap)))) {
            qWarning() << "PropertyCHanged not connected";
        }
    }
}

void QOfonoSmsInterface::disconnectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QOfonoSmsInterface::propertyChanged);
    if (signal == propertyChangedSignal) {

    }
}

QVariant QOfonoSmsInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    } else {
        qDebug() << Q_FUNC_INFO << "does not contain" << property;
    }
    return var;
}

QVariantMap QOfonoSmsInterface::getProperties()
{
    QDBusReply<QVariantMap > reply = this->call(QLatin1String("GetProperties"));
    return reply.value();
}

void QOfonoSmsInterface::sendMessage(const QString &to, const QString &message)
{
    QDBusReply<QString> reply =  this->call(QLatin1String("SendMessage"),
                                            QVariant::fromValue(to),
                                            QVariant::fromValue(message));
    if (reply.error().type() == QDBusError::InvalidArgs)
        qWarning("%s", qPrintable(reply.error().message()));
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT
