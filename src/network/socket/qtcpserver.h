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

#ifndef QTCPSERVER_H
#define QTCPSERVER_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qobject.h>
#include <QtNetwork/qabstractsocket.h>
#include <QtNetwork/qhostaddress.h>

QT_BEGIN_NAMESPACE


class QTcpServerPrivate;
#ifndef QT_NO_NETWORKPROXY
class QNetworkProxy;
#endif
class QTcpSocket;

class Q_NETWORK_EXPORT QTcpServer : public QObject
{
    Q_OBJECT
public:
    explicit QTcpServer(QObject *parent = nullptr);
    virtual ~QTcpServer();

    bool listen(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);
    void close();

    bool isListening() const;

    void setMaxPendingConnections(int numConnections);
    int maxPendingConnections() const;

    quint16 serverPort() const;
    QHostAddress serverAddress() const;

    qintptr socketDescriptor() const;
    bool setSocketDescriptor(qintptr socketDescriptor);

    bool waitForNewConnection(int msec = 0, bool *timedOut = nullptr);
    virtual bool hasPendingConnections() const;
    virtual QTcpSocket *nextPendingConnection();

    QAbstractSocket::SocketError serverError() const;
    QString errorString() const;

    void pauseAccepting();
    void resumeAccepting();

#ifndef QT_NO_NETWORKPROXY
    void setProxy(const QNetworkProxy &networkProxy);
    QNetworkProxy proxy() const;
#endif

protected:
    virtual void incomingConnection(qintptr handle);
    void addPendingConnection(QTcpSocket* socket);

    QTcpServer(QAbstractSocket::SocketType socketType, QTcpServerPrivate &dd,
               QObject *parent = nullptr);

Q_SIGNALS:
    void newConnection();
    void acceptError(QAbstractSocket::SocketError socketError);

private:
    Q_DISABLE_COPY(QTcpServer)
    Q_DECLARE_PRIVATE(QTcpServer)
};

QT_END_NAMESPACE

#endif // QTCPSERVER_H
