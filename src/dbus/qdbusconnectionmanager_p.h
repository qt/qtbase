/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtDBus module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
