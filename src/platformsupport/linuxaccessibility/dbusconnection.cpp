/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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



#include "dbusconnection_p.h"

#include <QtDBus/QDBusMessage>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

/*!
    \class DBusConnection
    \internal
    \brief Connects to the accessibility dbus.

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
        if (c.isConnected())
            return c;
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
