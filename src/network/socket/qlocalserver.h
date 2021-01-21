/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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

#ifndef QLOCALSERVER_H
#define QLOCALSERVER_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtNetwork/qabstractsocket.h>

QT_REQUIRE_CONFIG(localserver);

QT_BEGIN_NAMESPACE

class QLocalSocket;
class QLocalServerPrivate;

class Q_NETWORK_EXPORT QLocalServer : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QLocalServer)
    Q_PROPERTY(SocketOptions socketOptions READ socketOptions WRITE setSocketOptions)

Q_SIGNALS:
    void newConnection();

public:
    enum SocketOption {
        NoOptions = 0x0,
        UserAccessOption = 0x01,
        GroupAccessOption = 0x2,
        OtherAccessOption = 0x4,
        WorldAccessOption = 0x7
    };
    Q_FLAG(SocketOption)
    Q_DECLARE_FLAGS(SocketOptions, SocketOption)
    Q_FLAG(SocketOptions)

    explicit QLocalServer(QObject *parent = nullptr);
    ~QLocalServer();

    void close();
    QString errorString() const;
    virtual bool hasPendingConnections() const;
    bool isListening() const;
    bool listen(const QString &name);
    bool listen(qintptr socketDescriptor);
    int maxPendingConnections() const;
    virtual QLocalSocket *nextPendingConnection();
    QString serverName() const;
    QString fullServerName() const;
    static bool removeServer(const QString &name);
    QAbstractSocket::SocketError serverError() const;
    void setMaxPendingConnections(int numConnections);
    bool waitForNewConnection(int msec = 0, bool *timedOut = nullptr);

    void setSocketOptions(SocketOptions options);
    SocketOptions socketOptions() const;

    qintptr socketDescriptor() const;

protected:
    virtual void incomingConnection(quintptr socketDescriptor);

private:
    Q_DISABLE_COPY(QLocalServer)
    Q_PRIVATE_SLOT(d_func(), void _q_onNewConnection())
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QLocalServer::SocketOptions)

QT_END_NAMESPACE

#endif // QLOCALSERVER_H

