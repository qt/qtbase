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

class QDBusConnectionManager : public QDaemonThread
{
    Q_OBJECT
    struct ConnectionRequestData;
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

    mutable QMutex mutex;

signals:
    void connectionRequested(ConnectionRequestData *);
    void serverRequested(const QString &address, void *server);

protected:
    void run() override;

private:
    void executeConnectionRequest(ConnectionRequestData *data);
    void createServer(const QString &address, void *server);

    QHash<QString, QDBusConnectionPrivate *> connectionHash;

    QMutex defaultBusMutex;
    QDBusConnectionPrivate *defaultBuses[2];

    mutable QMutex senderMutex;
    QString senderName; // internal; will probably change
};

// TODO: move into own header and use Q_MOC_INCLUDE
struct QDBusConnectionManager::ConnectionRequestData
{
    enum RequestType {
        ConnectToStandardBus,
        ConnectToBusByAddress,
        ConnectToPeerByAddress
    } type;

    union {
        QDBusConnection::BusType busType;
        const QString *busAddress;
    };
    const QString *name;

    QDBusConnectionPrivate *result;

    bool suspendedDelivery;
};

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
