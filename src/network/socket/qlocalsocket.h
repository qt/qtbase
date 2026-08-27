// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QLOCALSOCKET_H
#define QLOCALSOCKET_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qiodevice.h>
#include <QtNetwork/qabstractsocket.h>

QT_REQUIRE_CONFIG(localserver);

QT_BEGIN_NAMESPACE

class QDebug;
class QLocalSocketPrivate;

class Q_NETWORK_EXPORT QLocalSocket : public QIODevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QLocalSocket)
    Q_PROPERTY(SocketOptions socketOptions READ socketOptions WRITE setSocketOptions
               BINDABLE bindableSocketOptions)

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
    Q_ENUM(LocalSocketError)

    enum LocalSocketState
    {
        UnconnectedState = QAbstractSocket::UnconnectedState,
        ConnectingState = QAbstractSocket::ConnectingState,
        ConnectedState = QAbstractSocket::ConnectedState,
        ClosingState = QAbstractSocket::ClosingState
    };
    Q_ENUM(LocalSocketState)

    enum SocketOption {
        NoOptions = 0x00,
        AbstractNamespaceOption = 0x01
    };
    Q_DECLARE_FLAGS(SocketOptions, SocketOption)
    Q_FLAG(SocketOptions)

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

    void setSocketOptions(SocketOptions option);
    SocketOptions socketOptions() const;
    QBindable<SocketOptions> bindableSocketOptions();

    LocalSocketState state() const;

    using QIODevice::waitForBytesWritten;
    using QIODevice::waitForReadyRead;
    bool waitForBytesWritten(QDeadlineTimer deadline = std::chrono::seconds(30)) QT7_ONLY(override);
    bool waitForConnected(QDeadlineTimer deadline = std::chrono::seconds(30));
    bool waitForDisconnected(QDeadlineTimer deadline = std::chrono::seconds(30));
    bool waitForReadyRead(QDeadlineTimer deadline = std::chrono::seconds(30)) QT7_ONLY(override);

#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QT_NETWORK_INLINE_SINCE(6, 13)
    bool waitForBytesWritten(int msecs) override;
#endif
    QT_NETWORK_INLINE_SINCE(6, 13)
    bool waitForConnected(int msecs);
    QT_NETWORK_INLINE_SINCE(6, 13)
    bool waitForDisconnected(int msecs);
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QT_NETWORK_INLINE_SINCE(6, 13)
    bool waitForReadyRead(int msecs) override;
#endif

Q_SIGNALS:
    void connected();
    void disconnected();
    void errorOccurred(QLocalSocket::LocalSocketError socketError);
    void stateChanged(QLocalSocket::LocalSocketState socketState);

protected:
    virtual qint64 readData(char*, qint64) override;
    qint64 readLineData(char *data, qint64 maxSize) override;
    qint64 skipData(qint64 maxSize) override;
    virtual qint64 writeData(const char*, qint64) override;

private:
    Q_DISABLE_COPY(QLocalSocket)
#if defined(QT_LOCALSOCKET_TCP)
    Q_PRIVATE_SLOT(d_func(), void _q_stateChanged(QAbstractSocket::SocketState))
    Q_PRIVATE_SLOT(d_func(), void _q_errorOccurred(QAbstractSocket::SocketError))
#elif defined(Q_OS_WIN)
    Q_PRIVATE_SLOT(d_func(), void _q_pipeClosed())
    Q_PRIVATE_SLOT(d_func(), void _q_winError(ulong, const QString &))
#else
    Q_PRIVATE_SLOT(d_func(), void _q_stateChanged(QAbstractSocket::SocketState))
    Q_PRIVATE_SLOT(d_func(), void _q_errorOccurred(QAbstractSocket::SocketError))
    Q_PRIVATE_SLOT(d_func(), void _q_connectToSocket())
    Q_PRIVATE_SLOT(d_func(), void _q_abortConnectionAttempt())
#endif
};

#if QT_NETWORK_REMOVED_SINCE(6, 12)
#ifndef QT_NO_DEBUG_STREAM
Q_NETWORK_EXPORT QDebug operator<<(QDebug, QLocalSocket::LocalSocketError);
Q_NETWORK_EXPORT QDebug operator<<(QDebug, QLocalSocket::LocalSocketState);
#endif
#endif // QT_NETWORK_REMOVED_SINCE(6, 12)

Q_DECLARE_OPERATORS_FOR_FLAGS(QLocalSocket::SocketOptions)

#if QT_NETWORK_INLINE_IMPL_SINCE(6, 13)
bool QLocalSocket::waitForConnected(int msecs)
{ return waitForConnected(QDeadlineTimer(msecs)); }
bool QLocalSocket::waitForDisconnected(int msecs)
{ return waitForDisconnected(QDeadlineTimer(msecs)); }
#endif
#if QT_NETWORK_INLINE_IMPL_SINCE(6, 13) && QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
bool QLocalSocket::waitForBytesWritten(int msecs)
{ return waitForBytesWritten(QDeadlineTimer(msecs)); }
bool QLocalSocket::waitForReadyRead(int msecs)
{ return waitForReadyRead(QDeadlineTimer(msecs)); }
#endif

QT_END_NAMESPACE

#endif // QLOCALSOCKET_H
