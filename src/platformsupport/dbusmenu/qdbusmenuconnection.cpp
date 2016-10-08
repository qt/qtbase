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

#ifndef QT_NO_SYSTEMTRAYICON
#include "qdbustrayicon_p.h"
#endif
#include "qdbusmenuconnection_p.h"
#include "qdbusmenuadaptor_p.h"
#include "qdbusplatformmenu_p.h"

#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusServiceWatcher>
#include <QtDBus/QDBusConnectionInterface>
#include <qdebug.h>
#include <qcoreapplication.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcMenu)

const QString StatusNotifierWatcherService = QLatin1String("org.kde.StatusNotifierWatcher");
const QString StatusNotifierWatcherPath = QLatin1String("/StatusNotifierWatcher");
const QString StatusNotifierItemPath = QLatin1String("/StatusNotifierItem");
const QString MenuBarPath = QLatin1String("/MenuBar");

/*!
    \class QDBusMenuConnection
    \internal
    A D-Bus connection which is used for both menu and tray icon services.
    Connects to the session bus and registers with the respective watcher services.
*/
QDBusMenuConnection::QDBusMenuConnection(QObject *parent, const QString &serviceName)
    : QObject(parent)
    , m_connection(serviceName.isNull() ? QDBusConnection::sessionBus()
                                        : QDBusConnection::connectToBus(QDBusConnection::SessionBus, serviceName))
    , m_dbusWatcher(new QDBusServiceWatcher(StatusNotifierWatcherService, m_connection, QDBusServiceWatcher::WatchForRegistration, this))
    , m_statusNotifierHostRegistered(false)
{
#ifndef QT_NO_SYSTEMTRAYICON
    QDBusInterface systrayHost(StatusNotifierWatcherService, StatusNotifierWatcherPath, StatusNotifierWatcherService, m_connection);
    if (systrayHost.isValid() && systrayHost.property("IsStatusNotifierHostRegistered").toBool())
        m_statusNotifierHostRegistered = true;
    else
        qCDebug(qLcMenu) << "StatusNotifierHost is not registered";
#endif
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
    bool success = connection().registerService(item->instanceId());
    if (!success) {
        qWarning() << "failed to register service" << item->instanceId();
        return false;
    }

    success = connection().registerObject(StatusNotifierItemPath, item);
    if (!success) {
        unregisterTrayIcon(item);
        qWarning() << "failed to register" << item->instanceId() << StatusNotifierItemPath;
        return false;
    }

    if (item->menu())
        registerTrayIconMenu(item);

    QDBusMessage registerMethod = QDBusMessage::createMethodCall(
                StatusNotifierWatcherService, StatusNotifierWatcherPath, StatusNotifierWatcherService,
                QLatin1String("RegisterStatusNotifierItem"));
    registerMethod.setArguments(QVariantList() << item->instanceId());
    success = m_connection.callWithCallback(registerMethod, this, SIGNAL(trayIconRegistered()), SLOT(dbusError(QDBusError)));

    return success;
}

bool QDBusMenuConnection::unregisterTrayIcon(QDBusTrayIcon *item)
{
    unregisterTrayIconMenu(item);
    connection().unregisterObject(StatusNotifierItemPath);
    bool success = connection().unregisterService(item->instanceId());
    if (!success)
        qWarning() << "failed to unregister service" << item->instanceId();
    return success;
}
#endif // QT_NO_SYSTEMTRAYICON

QT_END_NAMESPACE
