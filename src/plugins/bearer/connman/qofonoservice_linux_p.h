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

#ifndef QOFONOSERVICE_H
#define QOFONOSERVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtDBus/QtDBus>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>

#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusContext>
#include <QMap>

#ifndef QT_NO_BEARERMANAGEMENT
#ifndef QT_NO_DBUS

#define OFONO_SERVICE	                         "org.ofono"
#define OFONO_MANAGER_INTERFACE                  "org.ofono.Manager"
#define OFONO_MANAGER_PATH                       "/"
#define OFONO_MODEM_INTERFACE                    "org.ofono.Modem"
#define OFONO_NETWORK_REGISTRATION_INTERFACE     "org.ofono.NetworkRegistration"
#define OFONO_NETWORK_OPERATOR_INTERFACE         "org.ofono.NetworkOperator"
#define OFONO_DATA_CONNECTION_MANAGER_INTERFACE  "org.ofono.DataConnectionManager"
#define OFONO_SIM_MANAGER_INTERFACE              "org.ofono.SimManager"
#define OFONO_DATA_CONTEXT_INTERFACE             "org.ofono.ConnectionContext"

#define OFONO_SMS_MANAGER_INTERFACE              "org.ofono.SmsManager"
#define OFONO_PHONEBOOK_INTERFACE                "org.ofono.Phonebook"
#define OFONO_MESSAGE_WAITING_INTERFACE          "org.ofono.MessageWaiting"



QT_BEGIN_NAMESPACE

QT_END_NAMESPACE

struct ObjectPathProperties
{
    QDBusObjectPath path;
    QVariantMap properties;
};
typedef QList<ObjectPathProperties> PathPropertiesList;
Q_DECLARE_METATYPE(ObjectPathProperties)
Q_DECLARE_METATYPE (PathPropertiesList)

QT_BEGIN_NAMESPACE

class QOfonoManagerInterface : public  QDBusAbstractInterface
{
    Q_OBJECT

public:

    QOfonoManagerInterface( QObject *parent = 0);
    ~QOfonoManagerInterface();

     QDBusObjectPath path() const;

    QVariantMap getProperties();
    bool setProperty(const QString &name, const QDBusVariant &value);
    QList <QDBusObjectPath> getModems();
    QDBusObjectPath currentModem();

Q_SIGNALS:
    void propertyChanged(const QString &, const QDBusVariant &value);
    void propertyChangedContext(const QString &,const QString &,const QDBusVariant &);
protected:
    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);

};


class QOfonoDBusHelper: public QObject, protected QDBusContext
 {
     Q_OBJECT
 public:
    QOfonoDBusHelper(QObject *parent = 0);
    ~QOfonoDBusHelper();

 public slots:
    void propertyChanged(const QString &, const QDBusVariant &);
 Q_SIGNALS:
    void propertyChangedContext(const QString &,const QString &,const QDBusVariant &);
};

class QOfonoModemInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QOfonoModemInterface(const QString &dbusModemPathName, QObject *parent = 0);
    ~QOfonoModemInterface();

    QVariantMap getProperties();
    //properties
    bool isPowered();
    bool isOnline();
    QString getName();
    QString getManufacturer();
    QString getModel();
    QString getRevision();
    QString getSerial();

    QStringList getFeatures(); //sms, sim
    QStringList getInterfaces();
    QString defaultInterface();

protected:
    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);
Q_SIGNALS:
    void propertyChanged(const QString &, const QDBusVariant &value);
    void propertyChangedContext(const QString &,const QString &,const QDBusVariant &);
};


class QOfonoNetworkRegistrationInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QOfonoNetworkRegistrationInterface(const QString &dbusModemPathName, QObject *parent = 0);
    ~QOfonoNetworkRegistrationInterface();

    QVariantMap getProperties();

    //properties
    QString getStatus();
    quint16 getLac();
    quint32 getCellId();
    QString getTechnology();
    QString getOperatorName();
    int getSignalStrength();
    QString getBaseStation();
    QList <QDBusObjectPath> getOperators();

protected:
    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);
Q_SIGNALS:
    void propertyChanged(const QString &, const QDBusVariant &value);
    void propertyChangedContext(const QString &,const QString &,const QDBusVariant &);

};

class QOfonoNetworkOperatorInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:
//modem or operator paths
    explicit QOfonoNetworkOperatorInterface(const QString &dbusPathName, QObject *parent = 0);
    ~QOfonoNetworkOperatorInterface();

    QVariantMap getProperties();

    //properties
    QString getName();
    QString getStatus();// "unknown", "available", "current" and "forbidden"
    QString getMcc();
    QString getMnc();
    QStringList getTechnologies();

protected:
    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);
};

class QOfonoSimInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QOfonoSimInterface(const QString &dbusModemPathName, QObject *parent = 0);
    ~QOfonoSimInterface();

    QVariantMap getProperties();

    //properties
    bool isPresent();
    QString getHomeMcc();
    QString getHomeMnc();
//    QStringList subscriberNumbers();
//    QMap<QString,QString> serviceNumbers();
    QString pinRequired();
    QString lockedPins();
    QString cardIdentifier();

protected:
    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);
};


class QOfonoDataConnectionManagerInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QOfonoDataConnectionManagerInterface(const QString &dbusPathName, QObject *parent = 0);
    ~QOfonoDataConnectionManagerInterface();

    QVariantMap getProperties();

    //properties
    QList<QDBusObjectPath> getPrimaryContexts();
    bool isAttached();
    bool isRoamingAllowed();
    bool isPowered();

    bool setPower(bool on);

protected:
    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);
};


class QOfonoConnectionContextInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QOfonoConnectionContextInterface(const QString &dbusPathName, QObject *parent = 0);
    ~QOfonoConnectionContextInterface();

    QVariantMap getProperties();

    //properties
    bool isActive();
    QString getApName();
    QString getType();
    QString getName();
    QVariantMap getSettings();
    QString getInterface();
    QString getAddress();

    bool setActive(bool on);
    bool setApn(const QString &name);

protected:
    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);
    bool setProp(const QString &, const QVariant &var);
};

class QOfonoSmsInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QOfonoSmsInterface(const QString &dbusModemPathName, QObject *parent = 0);
    ~QOfonoSmsInterface();

    QVariantMap getProperties();
    void sendMessage(const QString &to, const QString &message);

    //properties
    QString  serviceCenterAddress();
    bool useDeliveryReports();
    QString bearer();

protected:
    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);
    QVariant getProperty(const QString &);

Q_SIGNALS:
    void propertyChanged(const QString &, const QDBusVariant &value);
    void propertyChangedContext(const QString &,const QString &,const QDBusVariant &);
    void immediateMessage(const QString &message, const QVariantMap &info);
    void incomingMessage(const QString &message, const QVariantMap &info);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif // QT_NO_BEARERMANAGEMENT

#endif //QOFONOSERVICE_H
