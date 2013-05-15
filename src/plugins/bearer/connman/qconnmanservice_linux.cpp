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

#include "qconnmanservice_linux_p.h"

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QDBusArgument &operator<<(QDBusArgument &argument, const ConnmanMap &map)
{
    argument.beginStructure();
    argument << map.objectPath << map.propertyMap;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, ConnmanMap &map)
{
    argument.beginStructure();
    argument >> map.objectPath >> map.propertyMap;
    argument.endStructure();
    return argument;
}

QConnmanManagerInterface::QConnmanManagerInterface( QObject *parent)
        : QDBusAbstractInterface(QLatin1String(CONNMAN_SERVICE),
                                 QLatin1String(CONNMAN_MANAGER_PATH),
                                 CONNMAN_MANAGER_INTERFACE,
                                 QDBusConnection::systemBus(), parent)
{
    qDBusRegisterMetaType<ConnmanMap>();
    qDBusRegisterMetaType<ConnmanMapList>();
}

QConnmanManagerInterface::~QConnmanManagerInterface()
{
}

void QConnmanManagerInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanManagerInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
        if(!connection().connect(QLatin1String(CONNMAN_SERVICE),
                               QLatin1String(CONNMAN_MANAGER_PATH),
                               QLatin1String(CONNMAN_MANAGER_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               this,SIGNAL(propertyChanged(QString,QDBusVariant)))) {
            qWarning() << "PropertyChanged not connected";
        }
    }

    static const QMetaMethod stateChangedSignal = QMetaMethod::fromSignal(&QConnmanManagerInterface::stateChanged);
    if (signal == stateChangedSignal) {
        if (!connection().connect(QLatin1String(CONNMAN_SERVICE),
                                    QLatin1String(CONNMAN_MANAGER_PATH),
                                    QLatin1String(CONNMAN_MANAGER_INTERFACE),
                                    QLatin1String("StateChanged"),
                                    this,SIGNAL(stateChanged(QString)))) {
            qWarning() << "StateChanged not connected";

        }
    }
    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QConnmanManagerInterface::propertyChangedContext);
    if (signal == propertyChangedContextSignal) {
        QConnmanDBusHelper *helper;
        helper = new QConnmanDBusHelper(this);

        QDBusConnection::systemBus().connect(QLatin1String(CONNMAN_SERVICE),
                               QLatin1String(CONNMAN_MANAGER_PATH),
                               QLatin1String(CONNMAN_MANAGER_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               helper,SLOT(propertyChanged(QString,QDBusVariant)));


        QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)), Qt::UniqueConnection);
    }
}

void QConnmanManagerInterface::disconnectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanManagerInterface::propertyChanged);
    if (signal == propertyChangedSignal) {

    }
}

QVariant QConnmanManagerInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    } else {
        qDebug() << "does not contain" << property;
    }
    return var;
}

QVariantMap QConnmanManagerInterface::getProperties()
{
    if(this->isValid()) {
        QDBusReply<QVariantMap > reply =  this->call(QLatin1String("GetProperties"));
        return reply.value();
    } else return QVariantMap();
}

QString QConnmanManagerInterface::getState()
{
    QDBusReply<QString > reply =  this->call("GetState");
    return reply.value();
}

bool QConnmanManagerInterface::setProperty(const QString &name, const QDBusVariant &value)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
    return false;
}

QDBusObjectPath QConnmanManagerInterface::createProfile(const QString &/*name*/)
{
    return QDBusObjectPath();
}

bool QConnmanManagerInterface::removeProfile(QDBusObjectPath /*path*/)
{
    return false;
}

bool QConnmanManagerInterface::requestScan(const QString &type)
{
    QDBusReply<QString> reply =  this->call(QLatin1String("RequestScan"), QVariant::fromValue(type));

    bool ok = true;
    if(reply.error().type() == QDBusError::InvalidArgs) {
        qWarning() << reply.error().message();
        ok = false;
    }
    return ok;
}

bool QConnmanManagerInterface::enableTechnology(const QString &type)
{
    QDBusReply<QList<QDBusObjectPath> > reply =  this->call(QLatin1String("EnableTechnology"), QVariant::fromValue(type));
    bool ok = true;
    if(reply.error().type() == QDBusError::InvalidArgs) {
        qWarning() << reply.error().message();
        ok = false;
    }
    return ok;
}

bool QConnmanManagerInterface::disableTechnology(const QString &type)
{
    QDBusReply<QList<QDBusObjectPath> > reply =  this->call(QLatin1String("DisableTechnology"), QVariant::fromValue(type));
    bool ok = true;
    if(reply.error().type() == QDBusError::InvalidArgs) {
        qWarning() << reply.error().message();
        ok = false;
    }
    return ok;
}

QDBusObjectPath QConnmanManagerInterface::connectService(QVariantMap &map)
{
    QDBusReply<QDBusObjectPath > reply =  this->call(QLatin1String("ConnectService"), QVariant::fromValue(map));
    if(!reply.isValid()) {
        qDebug() << reply.error().message();

    }
    return reply;
}

void QConnmanManagerInterface::registerAgent(QDBusObjectPath &/*path*/)
{
}

void QConnmanManagerInterface::unregisterAgent(QDBusObjectPath /*path*/)
{
}

void QConnmanManagerInterface::registerCounter(const QString &path, quint32 interval)
{   QDBusReply<QList<QDBusObjectPath> > reply =  this->call(QLatin1String("RegisterCounter"),
                                                            QVariant::fromValue(path),
                                                            QVariant::fromValue(interval));
    if(reply.error().type() == QDBusError::InvalidArgs) {
        qWarning() << reply.error().message();
    }
}

void QConnmanManagerInterface::unregisterCounter(const QString &path)
{   QDBusReply<QList<QDBusObjectPath> > reply =  this->call(QLatin1String("UnregisterCounter"),
                                                            QVariant::fromValue(path));
    if(reply.error().type() == QDBusError::InvalidArgs) {
        qWarning() << reply.error().message();
    }
}

QString QConnmanManagerInterface::requestSession(const QString &bearerName)
{
    QDBusReply<QList<QDBusObjectPath> > reply =  this->call(QLatin1String("RequestSession"),
                                                            QVariant::fromValue(bearerName));
    return QString();
}

void QConnmanManagerInterface::releaseSession()
{
    QDBusReply<QList<QDBusObjectPath> > reply =  this->call(QLatin1String("ReleaseSession"));
}


QDBusObjectPath QConnmanManagerInterface::lookupService(const QString &service)
{
    QDBusReply<QDBusObjectPath > reply =  this->call(QLatin1String("LookupService"), QVariant::fromValue(service));
    if(!reply.isValid()) {
        qDebug() << reply.error().message();
    }
    return reply;
}

// properties

QStringList QConnmanManagerInterface::getAvailableTechnologies()
{
    QVariant var = getProperty("AvailableTechnologies");
    return qdbus_cast<QStringList>(var);
}

QStringList QConnmanManagerInterface::getEnabledTechnologies()
{
    QVariant var = getProperty("EnabledTechnologies");
    return qdbus_cast<QStringList>(var);
}

QStringList QConnmanManagerInterface::getConnectedTechnologies()
{
    QVariant var = getProperty("ConnectedTechnologies");
    return qdbus_cast<QStringList>(var);
}

QString QConnmanManagerInterface::getDefaultTechnology()
{
    QVariant var = getProperty("DefaultTechnology");
    return qdbus_cast<QString>(var);
}

bool QConnmanManagerInterface::getOfflineMode()
{
    QVariant var = getProperty("OfflineMode");
    return qdbus_cast<bool>(var);
}

QString QConnmanManagerInterface::getActiveProfile()
{
    QVariant var = getProperty("ActiveProfile");
    return qdbus_cast<QString>(var);
}

QStringList QConnmanManagerInterface::getProfiles()
{
    QVariant var = getProperty("Profiles");
    return qdbus_cast<QStringList>(var);
}

QStringList QConnmanManagerInterface::getTechnologies()
{
    QStringList list;
    QDBusReply<ConnmanMapList> replyList = this->call(QLatin1String("GetTechnologies"));
    if (replyList.isValid()) {
        Q_FOREACH (ConnmanMap map, replyList.value()) {
            list << map.objectPath.path();
        }
    } else {
        // try for older version
        QVariant var = getProperty("Technologies");
        if (!var.isNull()) {
            list = qdbus_cast<QStringList>(var);
        }
    }
    return list;
}

QStringList QConnmanManagerInterface::getServices()
{
    QStringList list;
    QDBusReply<ConnmanMapList> replyList = this->call(QLatin1String("GetServices"));
    if (replyList.isValid()) {
        Q_FOREACH (ConnmanMap map, replyList.value()) {
            list << map.objectPath.path();
        }
    } else {
        QVariant var = getProperty("Services");
        if (!var.isNull()) {
            list = qdbus_cast<QStringList>(var);
        }
    }
    return list;
}

QString QConnmanManagerInterface::getPathForTechnology(const QString &name)
{
    foreach (const QString &path, getTechnologies()) {
        if(path.contains(name)) {
            return path;
        }
    }
    return "";
}


//////////////////////////
QConnmanProfileInterface::QConnmanProfileInterface(const QString &dbusPathName,QObject *parent)
    : QDBusAbstractInterface(QLatin1String(CONNMAN_SERVICE),
                             dbusPathName,
                             CONNMAN_PROFILE_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QConnmanProfileInterface::~QConnmanProfileInterface()
{
}

void QConnmanProfileInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanProfileInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
        QDBusConnection::systemBus().connect(QLatin1String(CONNMAN_SERVICE),
                               this->path(),
                               QLatin1String(CONNMAN_PROFILE_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               this,SIGNAL(propertyChanged(QString,QDBusVariant)));
    }
}

void QConnmanProfileInterface::disconnectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanProfileInterface::propertyChanged);
    if (signal == propertyChangedSignal) {

    }
}

QVariantMap QConnmanProfileInterface::getProperties()
{
    QDBusReply<QVariantMap > reply =  this->call(QLatin1String("GetProperties"));
    return reply.value();
}

QVariant QConnmanProfileInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    }
    return var;
}

// properties
QString QConnmanProfileInterface::getName()
{

    QVariant var = getProperty("Name");
    return qdbus_cast<QString>(var);
}

bool QConnmanProfileInterface::isOfflineMode()
{
    QVariant var = getProperty("OfflineMode");
    return qdbus_cast<bool>(var);
}

QStringList QConnmanProfileInterface::getServices()
{
    QVariant var = getProperty("Services");
    return qdbus_cast<QStringList>(var);
}


///////////////////////////
QConnmanServiceInterface::QConnmanServiceInterface(const QString &dbusPathName,QObject *parent)
    : QDBusAbstractInterface(QLatin1String(CONNMAN_SERVICE),
                             dbusPathName,
                             CONNMAN_SERVICE_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QConnmanServiceInterface::~QConnmanServiceInterface()
{
}

void QConnmanServiceInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanServiceInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
        QDBusConnection::systemBus().connect(QLatin1String(CONNMAN_SERVICE),
                               this->path(),
                               QLatin1String(CONNMAN_SERVICE_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               this,SIGNAL(propertyChanged(QString,QDBusVariant)));
    }
    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QConnmanServiceInterface::propertyChangedContext);
    if (signal == propertyChangedContextSignal) {
        QConnmanDBusHelper *helper;
        helper = new QConnmanDBusHelper(this);

        QDBusConnection::systemBus().connect(QLatin1String(CONNMAN_SERVICE),
                               this->path(),
                               QLatin1String(CONNMAN_SERVICE_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               helper,SLOT(propertyChanged(QString,QDBusVariant)));

        QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)), Qt::UniqueConnection);
    }
}

void QConnmanServiceInterface::disconnectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanServiceInterface::propertyChanged);
    if (signal == propertyChangedSignal) {

    }
}

QVariantMap QConnmanServiceInterface::getProperties()
{
    if(this->isValid()) {
        QDBusReply<QVariantMap> reply =  this->call(QLatin1String("GetProperties"));
        return reply.value();
    }
    else
        return QVariantMap();
}

QVariant QConnmanServiceInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    }
    return var;
}

void QConnmanServiceInterface::connect()
{
    this->asyncCall(QLatin1String("Connect"));
}

void QConnmanServiceInterface::disconnect()
{
    QDBusReply<QVariantMap> reply = this->call(QLatin1String("Disconnect"));
}

void QConnmanServiceInterface::remove()
{
    QDBusReply<QVariantMap> reply = this->call(QLatin1String("Remove"));
}

// properties
QString QConnmanServiceInterface::getState()
{
    QVariant var = getProperty("State");
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::getError()
{
    QVariant var = getProperty("Error");
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::getName()
{
    QVariant var = getProperty("Name");
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::getType()
{
    QVariant var = getProperty("Type");
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::getMode()
{
    QVariant var = getProperty("Mode");
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::getSecurity()
{
    QVariant var = getProperty("Security");
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::getPassphrase()
{
    QVariant var = getProperty("Passphrase");
    return qdbus_cast<QString>(var);
}

bool QConnmanServiceInterface::isPassphraseRequired()
{
    QVariant var = getProperty("PassphraseRequired");
    return qdbus_cast<bool>(var);
}

quint8 QConnmanServiceInterface::getSignalStrength()
{
    QVariant var = getProperty("Strength");
    return qdbus_cast<quint8>(var);
}

bool QConnmanServiceInterface::isFavorite()
{
    QVariant var = getProperty("Favorite");
    return qdbus_cast<bool>(var);
}

bool QConnmanServiceInterface::isImmutable()
{
    QVariant var = getProperty("Immutable");
    return qdbus_cast<bool>(var);
}

bool QConnmanServiceInterface::isAutoConnect()
{
    QVariant var = getProperty("AutoConnect");
    return qdbus_cast<bool>(var);
}

bool QConnmanServiceInterface::isSetupRequired()
{
    QVariant var = getProperty("SetupRequired");
    return qdbus_cast<bool>(var);
}

QString QConnmanServiceInterface::getAPN()
{
    QVariant var = getProperty("APN");
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::getMCC()
{
    QVariant var = getProperty("MCC");
    return qdbus_cast<QString>(var);
}

QString QConnmanServiceInterface::getMNC()
{
    QVariant var = getProperty("MNC");
    return qdbus_cast<QString>(var);
}

bool QConnmanServiceInterface::isRoaming()
{
    QVariant var = getProperty("Roaming");
    return qdbus_cast<bool>(var);
}

QStringList QConnmanServiceInterface::getNameservers()
{
    QVariant var = getProperty("NameServers");
    return qdbus_cast<QStringList>(var);
}

QStringList QConnmanServiceInterface::getDomains()
{
    QVariant var = getProperty("Domains");
    return qdbus_cast<QStringList>(var);
}

QVariantMap QConnmanServiceInterface::getIPv4()
{
    QVariant var = getProperty("IPv4");
    return qdbus_cast<QVariantMap >(var);
}

QVariantMap QConnmanServiceInterface::getIPv4Configuration()
{
    QVariant var = getProperty("IPv4.Configuration");
    return qdbus_cast<QVariantMap >(var);
}

QVariantMap QConnmanServiceInterface::getProxy()
{
    QVariant var = getProperty("Proxy");
    return qdbus_cast<QVariantMap >(var);
}

QVariantMap QConnmanServiceInterface::getEthernet()
{
    QVariant var = getProperty("Ethernet");
    return qdbus_cast<QVariantMap >(var);
}

QString QConnmanServiceInterface::getMethod()
{
    QVariant var;
    QVariantMap map = getEthernet();
    QMapIterator<QString,QVariant> it(map);
    while(it.hasNext()) {
        it.next();
        if(it.key() == "Method") {
            return it.value().toString();
        }
    }
 return QString();
}

QString QConnmanServiceInterface::getInterface()
{
    QVariant var;
    QVariantMap map = getEthernet();

    QMapIterator<QString,QVariant> it(map);
    while(it.hasNext()) {
        it.next();
        if(it.key() == "Interface") {
            return it.value().toString();
        }
    }

    return QString();
}

QString QConnmanServiceInterface::getMacAddress()
{
    QVariant var;
    QVariantMap map = getEthernet();

    QMapIterator<QString,QVariant> it(map);
    while(it.hasNext()) {
        it.next();
        if(it.key() == "Address") {
            return it.value().toString();
        }
    }
    return QString();
}

quint16 QConnmanServiceInterface::getMtu()
{
    quint16 mtu=0;
    QVariant var;
    QVariantMap map = getEthernet();

    QMapIterator<QString,QVariant> it(map);
    while(it.hasNext()) {
        it.next();
        if(it.key() == "MTU") {
            return it.value().toUInt();
        }
    }
    return mtu;
}

quint16 QConnmanServiceInterface::getSpeed()
{
    quint16 speed=0;
    QVariant var;
    QVariantMap map = getEthernet();

    QMapIterator<QString,QVariant> it(map);
    while(it.hasNext()) {
        it.next();
        if(it.key() == "Speed") {
            return it.value().toUInt();
        }
    }
    return speed;
}

QString QConnmanServiceInterface::getDuplex()
{
    QVariant var;
    QVariantMap map = getEthernet();

    QMapIterator<QString,QVariant> it(map);
    while(it.hasNext()) {
        it.next();
        if(it.key() == "Duplex") {
            return it.value().toString();
        }
    }
    return QString();
}


bool QConnmanServiceInterface::isOfflineMode()
{
    QVariant var = getProperty("OfflineMode");
    return qdbus_cast<bool>(var);
}

QStringList QConnmanServiceInterface::getServices()
{
    QVariant var = getProperty("Services");
    return qdbus_cast<QStringList>(var);
}


//////////////////////////
QConnmanTechnologyInterface::QConnmanTechnologyInterface(const QString &dbusPathName,QObject *parent)
    : QDBusAbstractInterface(QLatin1String(CONNMAN_SERVICE),
                             dbusPathName,
                             CONNMAN_TECHNOLOGY_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QConnmanTechnologyInterface::~QConnmanTechnologyInterface()
{
}

void QConnmanTechnologyInterface::connectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanTechnologyInterface::propertyChanged);
    if (signal == propertyChangedSignal) {
        QDBusConnection::systemBus().connect(QLatin1String(CONNMAN_SERVICE),
                               this->path(),
                               QLatin1String(CONNMAN_TECHNOLOGY_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               this,SIGNAL(propertyChanged(QString,QDBusVariant)));
    }
    static const QMetaMethod propertyChangedContextSignal = QMetaMethod::fromSignal(&QConnmanTechnologyInterface::propertyChangedContext);
    if (signal == propertyChangedContextSignal) {
        QConnmanDBusHelper *helper;
        helper = new QConnmanDBusHelper(this);

        QDBusConnection::systemBus().connect(QLatin1String(CONNMAN_SERVICE),
                               this->path(),
                               QLatin1String(CONNMAN_TECHNOLOGY_INTERFACE),
                               QLatin1String("PropertyChanged"),
                               helper,SLOT(propertyChanged(QString,QDBusVariant)));

        QObject::connect(helper,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)),
                this,SIGNAL(propertyChangedContext(QString,QString,QDBusVariant)), Qt::UniqueConnection);
    }
}

void QConnmanTechnologyInterface::disconnectNotify(const QMetaMethod &signal)
{
    static const QMetaMethod propertyChangedSignal = QMetaMethod::fromSignal(&QConnmanTechnologyInterface::propertyChanged);
    if (signal == propertyChangedSignal) {

    }
}

QVariantMap QConnmanTechnologyInterface::getProperties()
{
    QDBusReply<QVariantMap> reply =  this->call(QLatin1String("GetProperties"));
    return reply.value();
}

QVariant QConnmanTechnologyInterface::getProperty(const QString &property)
{
    QVariant var;
    QVariantMap map = getProperties();
    if (map.contains(property)) {
        var = map.value(property);
    }
    return var;
}

// properties
QString QConnmanTechnologyInterface::getState()
{
    QVariant var = getProperty("State");
    return qdbus_cast<QString>(var);
}

QString QConnmanTechnologyInterface::getName()
{
    QVariant var = getProperty("Name");
    return qdbus_cast<QString>(var);
}

QString QConnmanTechnologyInterface::getType()
{
    QVariant var = getProperty("Type");
    return qdbus_cast<QString>(var);
}


//////////////////////////////////
QConnmanAgentInterface::QConnmanAgentInterface(const QString &dbusPathName, QObject *parent)
    : QDBusAbstractInterface(QLatin1String(CONNMAN_SERVICE),
                             dbusPathName,
                             CONNMAN_AGENT_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QConnmanAgentInterface::~QConnmanAgentInterface()
{
}

void QConnmanAgentInterface::connectNotify(const QMetaMethod &signal)
{
    Q_UNUSED(signal);
}

void QConnmanAgentInterface::disconnectNotify(const QMetaMethod &signal)
{
    Q_UNUSED(signal);
}


void QConnmanAgentInterface::release()
{
}

void QConnmanAgentInterface::reportError(QDBusObjectPath &/*path*/, const QString &/*error*/)
{
}

void QConnmanAgentInterface::cancel()
{
}


/////////////////////////////////////////
QConnmanCounterInterface::QConnmanCounterInterface(const QString &dbusPathName,QObject *parent)
    : QDBusAbstractInterface(QLatin1String(CONNMAN_SERVICE),
                             dbusPathName,
                             CONNMAN_COUNTER_INTERFACE,
                             QDBusConnection::systemBus(), parent)
{
}

QConnmanCounterInterface::~QConnmanCounterInterface()
{
}

quint32 QConnmanCounterInterface::getReceivedByteCount()
{
    return 0;
}

quint32 QConnmanCounterInterface::getTransmittedByteCount()
{
    return 0;
}

quint64 QConnmanCounterInterface::getTimeOnline()
{
    return 0;
}

/////////////////////////////////////////
QConnmanDBusHelper::QConnmanDBusHelper(QObject * parent)
        : QObject(parent)
{
}

QConnmanDBusHelper::~QConnmanDBusHelper()
{
}

void QConnmanDBusHelper::propertyChanged(const QString &item, const QDBusVariant &var)
{
    QDBusMessage msg = this->message();
    Q_EMIT propertyChangedContext(msg.path() ,item, var);
}

/////////////////
QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT

