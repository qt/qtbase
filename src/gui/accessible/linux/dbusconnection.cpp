// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only



#include "dbusconnection_p.h"

#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusServiceWatcher>
#include <qdebug.h>

#include <QDBusConnectionInterface>
#include "bus_interface.h"

#include <QtGui/qguiapplication.h>
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/* note: do not change these to QStringLiteral;
   we are unloaded before QtDBus is done using the strings.
 */
#define A11Y_SERVICE "org.a11y.Bus"_L1
#define A11Y_PATH "/org/a11y/bus"_L1

/*!
    \class DBusConnection
    \internal
    \brief Connects to the accessibility dbus.

    This is usually a different bus from the session bus.
*/
DBusConnection::DBusConnection(QObject *parent)
    : QObject(parent), m_a11yConnection(QString()), m_enabled(false)
{
    // If the bus is explicitly set via env var it overrides everything else.
    QByteArray addressEnv = qgetenv("AT_SPI_BUS_ADDRESS");
    if (!addressEnv.isEmpty()) {
        m_enabled = true;
        connectA11yBus(QString::fromLocal8Bit(addressEnv));
        return;
    }

    // Start monitoring if "org.a11y.Bus" is registered as DBus service.
    QDBusConnection c = QDBusConnection::sessionBus();
    if (!c.isConnected()) {
        return;
    }

    dbusWatcher = new QDBusServiceWatcher(A11Y_SERVICE, c, QDBusServiceWatcher::WatchForRegistration, this);
    connect(dbusWatcher, SIGNAL(serviceRegistered(QString)), this, SLOT(serviceRegistered()));

    // If it is registered already, setup a11y right away
    if (c.interface()->isServiceRegistered(A11Y_SERVICE))
        serviceRegistered();

    if (QGuiApplication::platformName().startsWith("xcb"_L1)) {
        // In addition try if there is an xatom exposing the bus address, this allows applications run as root to work
        QString address = getAddressFromXCB();
        if (!address.isEmpty()) {
            m_enabled = true;
            connectA11yBus(address);
        }
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

    bool enabled = a11yAlwaysOn || a11yStatus->screenReaderEnabled() || a11yStatus->isEnabled();

    if (enabled != m_enabled) {
        m_enabled = enabled;
        if (m_a11yConnection.isConnected()) {
            emit enabledChanged(m_enabled);
        } else {
            QDBusConnection c = QDBusConnection::sessionBus();
            QDBusMessage m = QDBusMessage::createMethodCall(A11Y_SERVICE, A11Y_PATH, A11Y_SERVICE,
                                                            "GetAddress"_L1);
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
    m_a11yConnection = QDBusConnection(QDBusConnection::connectToBus(address, "a11y"_L1));

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

#include "moc_dbusconnection_p.cpp"
