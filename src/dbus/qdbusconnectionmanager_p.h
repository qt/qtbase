// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the public API.  This header file may
// change from version to version without notice, or even be
// removed.
//
// We mean it.
//
//

#ifndef QDBUSCONNECTIONMANAGER_P_H
#define QDBUSCONNECTIONMANAGER_P_H

#include <QtDBus/private/qtdbusglobal_p.h>
#include "qdbusconnection_p.h"
#include "private/qthread_p.h"

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusServer;

class QDBusConnectionManager : public QDaemonThread
{
    Q_OBJECT
public:
    QDBusConnectionManager();
    ~QDBusConnectionManager();
    static QDBusConnectionManager* instance();

    QDBusConnectionPrivate *busConnection(QDBusConnection::BusType type);
    QDBusConnectionPrivate *connection(const QString &name) const;
    void removeConnection(const QString &name);
    void setConnection(const QString &name, QDBusConnectionPrivate *c);
    QDBusConnectionPrivate *connectToBus(QDBusConnection::BusType type, const QString &name, bool suspendedDelivery);
    QDBusConnectionPrivate *connectToBus(const QString &address, const QString &name);
    QDBusConnectionPrivate *connectToPeer(const QString &address, const QString &name);

    void createServer(const QString &address, QDBusServer *server);

    mutable QMutex mutex;

protected:
    void run() override;

private:
    QDBusConnectionPrivate *doConnectToBus(QDBusConnection::BusType type, const QString &name,
                                           bool suspendedDelivery);
    QDBusConnectionPrivate *doConnectToBus(const QString &address, const QString &name);
    QDBusConnectionPrivate *doConnectToPeer(const QString &address, const QString &name);

    QHash<QString, QDBusConnectionPrivate *> connectionHash;

    QMutex defaultBusMutex;
    QDBusConnectionPrivate *defaultBuses[2];

    mutable QMutex senderMutex;
    QString senderName; // internal; will probably change
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
