/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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



#include "dbusconnection_p.h"

#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusServiceWatcher>
#include <qdebug.h>

#include <QDBusConnectionInterface>
#include "bus_interface.h"

#include <QtGui/qguiapplication.h>
#include <qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

/* note: do not change these to QStringLiteral;
   we are unloaded before QtDBus is done using the strings.
 */
#define A11Y_SERVICE QLatin1String("org.a11y.Bus")
#define A11Y_PATH QLatin1String("/org/a11y/bus")

/*!
    \class DBusConnection
    \internal
    \brief Connects to the accessibility dbus.

    This is usually a different bus from the session bus.
*/
DBusConnection::DBusConnection(QObject *parent)
    : QObject(parent), m_a11yConnection(QString()), m_enabled(false)
{
    // Start monitoring if "org.a11y.Bus" is registered as DBus service.
    QDBusConnection c = QDBusConnection::sessionBus();
    dbusWatcher = new QDBusServiceWatcher(A11Y_SERVICE, c, QDBusServiceWatcher::WatchForRegistration, this);
    connect(dbusWatcher, SIGNAL(serviceRegistered(QString)), this, SLOT(serviceRegistered()));

    // If it is registered already, setup a11y right away
    if (c.interface()->isServiceRegistered(A11Y_SERVICE))
        serviceRegistered();

    // In addition try if there is an xatom exposing the bus address, this allows applications run as root to work
    QString address = getAddressFromXCB();
    if (!address.isEmpty()) {
        m_enabled = true;
        connectA11yBus(address);
    }
}

QString DBusConnection::getAddressFromXCB()
{
    QGuiApplication *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
    if (!app)
        return QString();
    QPlatformNativeInterface *platformNativeInterface = app->platformNativeInterface();
    QByteArray *addressByteArray = reinterpret_cast<QByteArray*>(
                platformNativeInterface->nativeResourceForIntegration(QByteArrayLiteral("AtspiBus")));
    if (addressByteArray) {
        QString address = QString::fromLatin1(*addressByteArray);
        delete addressByteArray;
        return address;
    }
    return QString();
}

// We have the a11y registry on the session bus.
// Subscribe to updates about a11y enabled state.
// Find out the bus address
void DBusConnection::serviceRegistered()
{
    // listen to enabled changes
    QDBusConnection c = QDBusConnection::sessionBus();
    OrgA11yStatusInterface *a11yStatus = new OrgA11yStatusInterface(A11Y_SERVICE, A11Y_PATH, c, this);

    //The variable was introduced because on some embedded platforms there are custom accessibility
    //clients which don't set Status.ScreenReaderEnabled to true. The variable is also useful for
    //debugging.
    static const bool a11yAlwaysOn = qEnvironmentVariableIsSet("QT_LINUX_ACCESSIBILITY_ALWAYS_ON");

    // a11yStatus->isEnabled() returns always true (since Gnome 3.6)
    bool enabled = a11yAlwaysOn || a11yStatus->screenReaderEnabled();

    if (enabled != m_enabled) {
        m_enabled = enabled;
        if (m_a11yConnection.isConnected()) {
            emit enabledChanged(m_enabled);
        } else {
            QDBusConnection c = QDBusConnection::sessionBus();
            QDBusMessage m = QDBusMessage::createMethodCall(QLatin1String("org.a11y.Bus"),
                                                            QLatin1String("/org/a11y/bus"),
                                                            QLatin1String("org.a11y.Bus"), QLatin1String("GetAddress"));
            c.callWithCallback(m, this, SLOT(connectA11yBus(QString)), SLOT(dbusError(QDBusError)));
        }
    }

    //    connect(a11yStatus, ); QtDbus doesn't support notifications for property changes yet
}

void DBusConnection::serviceUnregistered()
{
    emit enabledChanged(false);
}

void DBusConnection::connectA11yBus(const QString &address)
{
    if (address.isEmpty()) {
        qWarning("Could not find Accessibility DBus address.");
        return;
    }
    m_a11yConnection = QDBusConnection(QDBusConnection::connectToBus(address, QLatin1String("a11y")));

    if (m_enabled)
        emit enabledChanged(true);
}

void DBusConnection::dbusError(const QDBusError &error)
{
    qWarning() << "Accessibility encountered a DBus error:" << error;
}

/*!
  Returns the DBus connection that got established.
  Or an invalid connection if not yet connected.
*/
QDBusConnection DBusConnection::connection() const
{
    return m_a11yConnection;
}

QT_END_NAMESPACE
