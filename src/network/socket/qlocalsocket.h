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

#ifndef QLOCALSOCKET_H
#define QLOCALSOCKET_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qiodevice.h>
#include <QtNetwork/qabstractsocket.h>

QT_REQUIRE_CONFIG(localserver);

QT_BEGIN_NAMESPACE

class QLocalSocketPrivate;

class Q_NETWORK_EXPORT QLocalSocket : public QIODevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QLocalSocket)

public:
    enum LocalSocketError
    {
        ConnectionRefusedError = QAbstractSocket::ConnectionRefusedError,
        PeerClosedError = QAbstractSocket::RemoteHostClosedError,
        ServerNotFoundError = QAbstractSocket::HostNotFoundError,
        SocketAccessError = QAbstractSocket::SocketAccessError,
        SocketResourceError = QAbstractSocket::SocketResourceError,
        SocketTimeoutError = QAbstractSocket::SocketTimeoutError,
        DatagramTooLargeError = QAbstractSocket::DatagramTooLargeError,
        ConnectionError = QAbstractSocket::NetworkError,
        UnsupportedSocketOperationError = QAbstractSocket::UnsupportedSocketOperationError,
        UnknownSocketError = QAbstractSocket::UnknownSocketError,
        OperationError = QAbstractSocket::OperationError
    };

    enum LocalSocketState
    {
        UnconnectedState = QAbstractSocket::UnconnectedState,
        ConnectingState = QAbstractSocket::ConnectingState,
        ConnectedState = QAbstractSocket::ConnectedState,
        ClosingState = QAbstractSocket::ClosingState
    };

    QLocalSocket(QObject *parent = nullptr);
    ~QLocalSocket();

    void connectToServer(OpenMode openMode = ReadWrite);
    void connectToServer(const QString &name, OpenMode openMode = ReadWrite);
    void disconnectFromServer();

    void setServerName(const QString &name);
    QString serverName() const;
    QString fullServerName() const;

    void abort();
    virtual bool isSequential() const override;
    virtual qint64 bytesAvailable() const override;
    virtual qint64 bytesToWrite() const override;
    virtual bool canReadLine() const override;
    virtual bool open(OpenMode openMode = ReadWrite) override;
    virtual void close() override;
    LocalSocketError error() const;
    bool flush();
    bool isValid() const;
    qint64 readBufferSize() const;
    void setReadBufferSize(qint64 size);

    bool setSocketDescriptor(qintptr socketDescriptor,
                             LocalSocketState socketState = ConnectedState,
                             OpenMode openMode = ReadWrite);
    qintptr socketDescriptor() const;

    LocalSocketState state() const;
    bool waitForBytesWritten(int msecs = 30000) override;
    bool waitForConnected(int msecs = 30000);
    bool waitForDisconnected(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000) override;

Q_SIGNALS:
    void connected();
    void disconnected();
#if QT_DEPRECATED_SINCE(5,15)
    QT_DEPRECATED_NETWORK_API_5_15_X("Use QLocalSocket::errorOccurred(QLocalSocket::LocalSocketError) instead")
    void error(QLocalSocket::LocalSocketError socketError);
#endif
    void errorOccurred(QLocalSocket::LocalSocketError socketError);
    void stateChanged(QLocalSocket::LocalSocketState socketState);

protected:
    virtual qint64 readData(char*, qint64) override;
    virtual qint64 writeData(const char*, qint64) override;

private:
    Q_DISABLE_COPY(QLocalSocket)
#if defined(QT_LOCALSOCKET_TCP)
    Q_PRIVATE_SLOT(d_func(), void _q_stateChanged(QAbstractSocket::SocketState))
    Q_PRIVATE_SLOT(d_func(), void _q_errorOccurred(QAbstractSocket::SocketError))
#elif defined(Q_OS_WIN)
    Q_PRIVATE_SLOT(d_func(), void _q_canWrite())
    Q_PRIVATE_SLOT(d_func(), void _q_pipeClosed())
    Q_PRIVATE_SLOT(d_func(), void _q_winError(ulong, const QString &))
#else
    Q_PRIVATE_SLOT(d_func(), void _q_stateChanged(QAbstractSocket::SocketState))
    Q_PRIVATE_SLOT(d_func(), void _q_errorOccurred(QAbstractSocket::SocketError))
    Q_PRIVATE_SLOT(d_func(), void _q_connectToSocket())
    Q_PRIVATE_SLOT(d_func(), void _q_abortConnectionAttempt())
#endif
};

#ifndef QT_NO_DEBUG_STREAM
#include <QtCore/qdebug.h>
Q_NETWORK_EXPORT QDebug operator<<(QDebug, QLocalSocket::LocalSocketError);
Q_NETWORK_EXPORT QDebug operator<<(QDebug, QLocalSocket::LocalSocketState);
#endif

QT_END_NAMESPACE

#endif // QLOCALSOCKET_H
