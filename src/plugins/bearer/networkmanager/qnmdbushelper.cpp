/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// this class is for helping qdbus get stuff

#include "qnmdbushelper.h"

#include "qnetworkmanagerservice.h"

#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>

#include <QDebug>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

QNmDBusHelper::QNmDBusHelper(QObject * parent)
        : QObject(parent)
{
}

QNmDBusHelper::~QNmDBusHelper()
{
}

void QNmDBusHelper::deviceStateChanged(quint32 state)
 {
    QDBusMessage msg = this->message();
    if (state == NM_DEVICE_STATE_ACTIVATED
       || state == NM_DEVICE_STATE_DISCONNECTED
       || state == NM_DEVICE_STATE_UNAVAILABLE
       || state == NM_DEVICE_STATE_FAILED) {
        emit pathForStateChanged(msg.path(), state);
    }
 }

void QNmDBusHelper::slotAccessPointAdded(QDBusObjectPath path)
{
    if (path.path().length() > 2)
        emit pathForAccessPointAdded(path.path());
}

void QNmDBusHelper::slotAccessPointRemoved(QDBusObjectPath path)
{
    if (path.path().length() > 2)
        emit pathForAccessPointRemoved(path.path());
}

void QNmDBusHelper::slotPropertiesChanged(QMap<QString,QVariant> map)
{
    QDBusMessage msg = this->message();
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        if (i.key() == QStringLiteral("State")) {
            quint32 state = i.value().toUInt();
            if (state == NM_DEVICE_STATE_ACTIVATED
                || state == NM_DEVICE_STATE_DISCONNECTED
                || state == NM_DEVICE_STATE_UNAVAILABLE
                || state == NM_DEVICE_STATE_FAILED) {
                emit pathForPropertiesChanged(msg.path(), map);
            }
        } else if (i.key() == QStringLiteral("ActiveAccessPoint")) {
            emit pathForPropertiesChanged(msg.path(), map);
        } else if (i.key() == QStringLiteral("ActiveConnections")) {
            emit pathForPropertiesChanged(msg.path(), map);
        } else if (i.key() == QStringLiteral("AvailableConnections")) {
            const QDBusArgument &dbusArgs = i.value().value<QDBusArgument>();
            QDBusObjectPath path;
            QStringList paths;
            dbusArgs.beginArray();
            while (!dbusArgs.atEnd()) {
                dbusArgs >> path;
                paths << path.path();
            }
            dbusArgs.endArray();
            emit pathForConnectionsChanged(paths);
        }
    }
}

void QNmDBusHelper::slotSettingsRemoved()
{
    QDBusMessage msg = this->message();
    emit pathForSettingsRemoved(msg.path());
}

void QNmDBusHelper::activeConnectionPropertiesChanged(QMap<QString,QVariant> map)
{
    QDBusMessage msg = this->message();
    QMapIterator<QString, QVariant> i(map);
    while (i.hasNext()) {
        i.next();
        if (i.key() == QStringLiteral("State")) {
            quint32 state = i.value().toUInt();
            if (state == NM_ACTIVE_CONNECTION_STATE_ACTIVATED
                || state == NM_ACTIVE_CONNECTION_STATE_DEACTIVATED) {
                emit pathForPropertiesChanged(msg.path(), map);
            }
        }
    }
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
