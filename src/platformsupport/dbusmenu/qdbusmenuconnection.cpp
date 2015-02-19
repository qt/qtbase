/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
QDBusMenuConnection::QDBusMenuConnection(QObject *parent)
    : QObject(parent)
    , m_connection(QDBusConnection::sessionBus())
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

    if (item->menu()) {
        success = connection().registerObject(MenuBarPath, item->menu());
        if (!success) {
            unregisterTrayIcon(item);
            qWarning() << "failed to register" << item->instanceId() << MenuBarPath;
            return false;
        }
    }

    QDBusMessage registerMethod = QDBusMessage::createMethodCall(
                StatusNotifierWatcherService, StatusNotifierWatcherPath, StatusNotifierWatcherService,
                QLatin1String("RegisterStatusNotifierItem"));
    registerMethod.setArguments(QVariantList() << item->instanceId());
    success = m_connection.callWithCallback(registerMethod, this, SIGNAL(trayIconRegistered()), SLOT(dbusError(QDBusError)));

    return success;
}

bool QDBusMenuConnection::unregisterTrayIcon(QDBusTrayIcon *item)
{
    connection().unregisterObject(MenuBarPath);
    connection().unregisterObject(StatusNotifierItemPath);
    bool success = connection().unregisterService(item->instanceId());
    if (!success)
        qWarning() << "failed to unregister service" << item->instanceId();
    return success;
}
#endif // QT_NO_SYSTEMTRAYICON

QT_END_NAMESPACE
