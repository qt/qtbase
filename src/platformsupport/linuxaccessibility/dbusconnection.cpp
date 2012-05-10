/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/



#include "dbusconnection_p.h"

#include <QtDBus/QDBusMessage>
#include <qdebug.h>

/*!
    \class DBusConnection

    \brief DBusConnection

    DBusConnection
*/

QT_BEGIN_NAMESPACE

/*!
  Connects to the accessibility dbus.

  This is usually a different bus from the session bus.
*/
DBusConnection::DBusConnection()
    : dbusConnection(connectDBus())
{}

QDBusConnection DBusConnection::connectDBus()
{
    QString address = getAccessibilityBusAddress();

    if (!address.isEmpty()) {
        QDBusConnection c = QDBusConnection::connectToBus(address, QStringLiteral("a11y"));
        if (c.isConnected()) {
            qDebug() << "Connected to accessibility bus at: " << address;
            return c;
        }
        qWarning("Found Accessibility DBus address but cannot connect. Falling back to session bus.");
    } else {
        qWarning("Accessibility DBus not found. Falling back to session bus.");
    }

    QDBusConnection c = QDBusConnection::sessionBus();
    if (!c.isConnected()) {
        qWarning("Could not connect to DBus.");
    }
    return QDBusConnection::sessionBus();
}

QString DBusConnection::getAccessibilityBusAddress() const
{
    QDBusConnection c = QDBusConnection::sessionBus();

    QDBusMessage m = QDBusMessage::createMethodCall(QLatin1String("org.a11y.Bus"),
                                                    QLatin1String("/org/a11y/bus"),
                                                    QLatin1String("org.a11y.Bus"), QLatin1String("GetAddress"));
    QDBusMessage reply = c.call(m);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "Qt at-spi: error getting the accessibility dbus address: " << reply.errorMessage();
        return QString();
    }

    QString busAddress = reply.arguments().at(0).toString();
    qDebug() << "Got bus address: " << busAddress;
    return busAddress;
}

/*!
  Returns the DBus connection that got established.
*/
QDBusConnection DBusConnection::connection() const
{
    return dbusConnection;
}

QT_END_NAMESPACE
