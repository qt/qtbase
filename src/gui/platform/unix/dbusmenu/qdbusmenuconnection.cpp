// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtgui-config.h>

#include <QDBusMessage>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QDBusConnectionInterface>
#include <QDebug>
#include <QCoreApplication>

#ifndef QT_NO_SYSTEMTRAYICON
#include <private/qdbustrayicon_p.h>
#endif
#include <private/qdbusmenuconnection_p.h>
#include <private/qdbusmenuadaptor_p.h>
#include <private/qdbusplatformmenu_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(qLcMenu)

const QString StatusNotifierWatcherService = "org.kde.StatusNotifierWatcher"_L1;
const QString StatusNotifierWatcherPath = "/StatusNotifierWatcher"_L1;
const QString StatusNotifierItemPath = "/StatusNotifierItem"_L1;
const QString MenuBarPath = "/MenuBar"_L1;

/*!
    \class QDBusMenuConnection
    \internal
    A D-Bus connection which is used for both menu and tray icon services.
    Connects to the session bus and registers with the respective watcher services.
*/
QDBusMenuConnection::QDBusMenuConnection(QObject *parent, const QString &serviceName)
    : QObject(parent)
    , m_serviceName(serviceName)
    , m_connection(serviceName.isNull() ? QDBusConnection::sessionBus()
                                        : QDBusConnection::connectToBus(QDBusConnection::SessionBus, serviceName))
    , m_dbusWatcher(new QDBusServiceWatcher(StatusNotifierWatcherService, m_connection, QDBusServiceWatcher::WatchForRegistration, this))
    , m_watcherRegistered(false)
{
#ifndef QT_NO_SYSTEMTRAYICON
    // Start monitoring if any known tray-related services are registered.
    if (m_connection.interface()->isServiceRegistered(StatusNotifierWatcherService))
        m_watcherRegistered = true;
    else
        qCDebug(qLcMenu) << "failed to find service" << StatusNotifierWatcherService;
#endif
}

QDBusMenuConnection::~QDBusMenuConnection()
{
  if (!m_serviceName.isEmpty() && m_connection.isConnected())
      QDBusConnection::disconnectFromBus(m_serviceName);
}

void QDBusMenuConnection::dbusError(const QDBusError &error)
{
    qWarning() << "QDBusTrayIcon encountered a D-Bus error:" << error;
}

#ifndef QT_NO_SYSTEMTRAYICON
bool QDBusMenuConnection::registerTrayIconMenu(QDBusTrayIcon *item)
{
    bool success = connection().registerObject(MenuBarPath, item->menu());
    if (!success)  // success == false is normal, because the object may be already registered
        qCDebug(qLcMenu) << "failed to register" << item->instanceId() << MenuBarPath;
    return success;
}

void QDBusMenuConnection::unregisterTrayIconMenu(QDBusTrayIcon *item)
{
    if (item->menu())
        connection().unregisterObject(MenuBarPath);
}

bool QDBusMenuConnection::registerTrayIcon(QDBusTrayIcon *item)
{
    bool success = connection().registerObject(StatusNotifierItemPath, item);
    if (!success) {
        unregisterTrayIcon(item);
        qWarning() << "failed to register" << item->instanceId() << StatusNotifierItemPath;
        return false;
    }

    if (item->menu())
        registerTrayIconMenu(item);

    return registerTrayIconWithWatcher(item);
}

bool QDBusMenuConnection::registerTrayIconWithWatcher(QDBusTrayIcon *item)
{
    Q_UNUSED(item);
    QDBusMessage registerMethod = QDBusMessage::createMethodCall(
                StatusNotifierWatcherService, StatusNotifierWatcherPath, StatusNotifierWatcherService,
                "RegisterStatusNotifierItem"_L1);
    registerMethod.setArguments(QVariantList() << m_connection.baseService());
    return m_connection.callWithCallback(registerMethod, this, SIGNAL(trayIconRegistered()), SLOT(dbusError(QDBusError)));
}

void QDBusMenuConnection::unregisterTrayIcon(QDBusTrayIcon *item)
{
    unregisterTrayIconMenu(item);
    connection().unregisterObject(StatusNotifierItemPath);
}
#endif // QT_NO_SYSTEMTRAYICON

QT_END_NAMESPACE

#include "moc_qdbusmenuconnection_p.cpp"
