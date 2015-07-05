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

#ifndef QT_NO_DBUS

#define OFONO_SERVICE                            "org.ofono"
#define OFONO_MANAGER_INTERFACE                  "org.ofono.Manager"
#define OFONO_MANAGER_PATH                       "/"

#define OFONO_MODEM_INTERFACE                    "org.ofono.Modem"
#define OFONO_NETWORK_REGISTRATION_INTERFACE     "org.ofono.NetworkRegistration"
#define OFONO_DATA_CONNECTION_MANAGER_INTERFACE  "org.ofono.ConnectionManager"
#define OFONO_CONNECTION_CONTEXT_INTERFACE       "org.ofono.ConnectionContext"

QT_BEGIN_NAMESPACE

QT_END_NAMESPACE

struct ObjectPathProperties
{
    QDBusObjectPath path;
    QVariantMap properties;
};
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(ObjectPathProperties, Q_MOVABLE_TYPE); // QDBusObjectPath is movable, but cannot be
                                                          // marked as such until Qt 6
QT_END_NAMESPACE

typedef QVector<ObjectPathProperties> PathPropertiesList;
Q_DECLARE_METATYPE(ObjectPathProperties)
Q_DECLARE_METATYPE (PathPropertiesList)

QT_BEGIN_NAMESPACE

class QOfonoManagerInterface : public  QDBusAbstractInterface
{
    Q_OBJECT

public:

    QOfonoManagerInterface( QObject *parent = 0);
    ~QOfonoManagerInterface();

    QStringList getModems();
    QString currentModem();
signals:
    void modemChanged();
private:
    QStringList modemList;
private slots:
    void modemAdded(const QDBusObjectPath &path, const QVariantMap &var);
    void modemRemoved(const QDBusObjectPath &path);
};

class QOfonoModemInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QOfonoModemInterface(const QString &dbusModemPathName, QObject *parent = 0);
    ~QOfonoModemInterface();

    bool isPowered();
    bool isOnline();
    QStringList interfaces();
private:
    QVariantMap getProperties();
    QVariantMap propertiesMap;
    QVariant getProperty(const QString &);
    void propertyChanged(const QString &, const QDBusVariant &value);
};


class QOfonoNetworkRegistrationInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QOfonoNetworkRegistrationInterface(const QString &dbusModemPathName, QObject *parent = 0);
    ~QOfonoNetworkRegistrationInterface();

    QString getTechnology();

private:
    QVariantMap getProperties();
    QVariant getProperty(const QString &);
    QVariantMap propertiesMap;
Q_SIGNALS:
    void propertyChanged(const QString &, const QDBusVariant &value);
};

class QOfonoDataConnectionManagerInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QOfonoDataConnectionManagerInterface(const QString &dbusPathName, QObject *parent = 0);
    ~QOfonoDataConnectionManagerInterface();

    QStringList contexts();
    PathPropertiesList contextsWithProperties();
    bool roamingAllowed();
    QVariant getProperty(const QString &);
    QString bearer();
Q_SIGNALS:
    void roamingAllowedChanged(bool);
private:
    QVariantMap &getProperties();
    QVariantMap propertiesMap;
    QStringList contextList;
    PathPropertiesList contextListProperties;
private Q_SLOTS:
    void propertyChanged(const QString &, const QDBusVariant &value);
};

class QOfonoConnectionContextInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:

    explicit QOfonoConnectionContextInterface(const QString &dbusPathName, QObject *parent = 0);
    ~QOfonoConnectionContextInterface();

    QVariant getProperty(const QString &);
    bool active();
    QString accessPointName();
    QString name();

Q_SIGNALS:
private:
    QVariantMap getProperties();
    QVariantMap propertiesMap;
private slots:
    void propertyChanged(const QString &, const QDBusVariant &value);
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS

#endif //QOFONOSERVICE_H
