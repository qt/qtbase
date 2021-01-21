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

#ifndef QNATIVESOCKETENGINE_WINRT_P_H
#define QNATIVESOCKETENGINE_WINRT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include <QtCore/QEventLoop>
#include <QtCore/QBuffer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QAtomicInteger>
#include "QtNetwork/qhostaddress.h"
#include "private/qabstractsocketengine_p.h"
#include <wrl.h>
#include <windows.networking.sockets.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcNetworkSocket)
Q_DECLARE_LOGGING_CATEGORY(lcNetworkSocketVerbose)

namespace WinRTSocketEngine {
    enum ErrorString {
        NonBlockingInitFailedErrorString,
        BroadcastingInitFailedErrorString,
        NoIpV6ErrorString,
        RemoteHostClosedErrorString,
        TimeOutErrorString,
        ResourceErrorString,
        OperationUnsupportedErrorString,
        ProtocolUnsupportedErrorString,
        InvalidSocketErrorString,
        HostUnreachableErrorString,
        NetworkUnreachableErrorString,
        AccessErrorString,
        ConnectionTimeOutErrorString,
        ConnectionRefusedErrorString,
        AddressInuseErrorString,
        AddressNotAvailableErrorString,
        AddressProtectedErrorString,
        DatagramTooLargeErrorString,
        SendDatagramErrorString,
        ReceiveDatagramErrorString,
        WriteErrorString,
        ReadErrorString,
        PortInuseErrorString,
        NotSocketErrorString,
        InvalidProxyTypeString,
        TemporaryErrorString,

        UnknownSocketErrorString = -1
    };
}

class QNativeSocketEnginePrivate;
class SocketEngineWorker;

struct WinRtDatagram {
    QByteArray data;
    QIpPacketHeader header;
};

class Q_AUTOTEST_EXPORT QNativeSocketEngine : public QAbstractSocketEngine
{
    Q_OBJECT
public:
    QNativeSocketEngine(QObject *parent = 0);
    ~QNativeSocketEngine();

    bool initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::IPv4Protocol);
    bool initialize(qintptr socketDescriptor, QAbstractSocket::SocketState socketState = QAbstractSocket::ConnectedState);

    qintptr socketDescriptor() const;

    bool isValid() const;

    bool connectToHost(const QHostAddress &address, quint16 port);
    bool connectToHostByName(const QString &name, quint16 port);
    bool bind(const QHostAddress &address, quint16 port);
    bool listen();
    int accept();
    void close();

#ifndef QT_NO_NETWORKINTERFACE
    bool joinMulticastGroup(const QHostAddress &groupAddress,
                            const QNetworkInterface &iface);
    bool leaveMulticastGroup(const QHostAddress &groupAddress,
                             const QNetworkInterface &iface);
    QNetworkInterface multicastInterface() const;
    bool setMulticastInterface(const QNetworkInterface &iface);
#endif

    qint64 bytesAvailable() const;

    qint64 read(char *data, qint64 maxlen);
    qint64 write(const char *data, qint64 len);

    qint64 readDatagram(char *data, qint64 maxlen, QIpPacketHeader * = 0, PacketHeaderOptions = WantNone);
    qint64 writeDatagram(const char *data, qint64 len, const QIpPacketHeader &header);
    bool hasPendingDatagrams() const;
    qint64 pendingDatagramSize() const;

    qint64 bytesToWrite() const;

    qint64 receiveBufferSize() const;
    void setReceiveBufferSize(qint64 bufferSize);

    qint64 sendBufferSize() const;
    void setSendBufferSize(qint64 bufferSize);

    int option(SocketOption option) const;
    bool setOption(SocketOption option, int value);

    bool waitForRead(int msecs = 30000, bool *timedOut = 0);
    bool waitForWrite(int msecs = 30000, bool *timedOut = 0);
    bool waitForReadOrWrite(bool *readyToRead, bool *readyToWrite,
                            bool checkRead, bool checkWrite,
                            int msecs = 30000, bool *timedOut = 0);

    bool isReadNotificationEnabled() const;
    void setReadNotificationEnabled(bool enable);
    bool isWriteNotificationEnabled() const;
    void setWriteNotificationEnabled(bool enable);
    bool isExceptionNotificationEnabled() const;
    void setExceptionNotificationEnabled(bool enable);

signals:
    void connectionReady();
    void readReady();
    void writeReady();
    void newDatagramReceived(const WinRtDatagram &datagram);

private slots:
    void establishRead();
    void handleConnectOpFinished(bool success, QAbstractSocket::SocketError error,
                                 WinRTSocketEngine::ErrorString errorString);
    void handleNewData();
    void handleTcpError(QAbstractSocket::SocketError error);
    void processReadReady();

private:
    Q_DECLARE_PRIVATE(QNativeSocketEngine)
    Q_DISABLE_COPY_MOVE(QNativeSocketEngine)
};

class QNativeSocketEnginePrivate : public QAbstractSocketEnginePrivate
{
    Q_DECLARE_PUBLIC(QNativeSocketEngine)
public:
    QNativeSocketEnginePrivate();
    ~QNativeSocketEnginePrivate();

    qintptr socketDescriptor;
    SocketEngineWorker *worker;

    bool notifyOnRead, notifyOnWrite, notifyOnException;
    QAtomicInt closingDown;

    void setError(QAbstractSocket::SocketError error, WinRTSocketEngine::ErrorString errorString) const;

    // native functions
    int option(QNativeSocketEngine::SocketOption option) const;
    bool setOption(QNativeSocketEngine::SocketOption option, int value);

    bool createNewSocket(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol &protocol);

    bool checkProxy(const QHostAddress &address);
    bool fetchConnectionParameters();

private:
    inline ABI::Windows::Networking::Sockets::IStreamSocket *tcpSocket() const
        { return reinterpret_cast<ABI::Windows::Networking::Sockets::IStreamSocket *>(socketDescriptor); }
    inline ABI::Windows::Networking::Sockets::IDatagramSocket *udpSocket() const
        { return reinterpret_cast<ABI::Windows::Networking::Sockets::IDatagramSocket *>(socketDescriptor); }
    Microsoft::WRL::ComPtr<ABI::Windows::Networking::Sockets::IStreamSocketListener> tcpListener;

    QList<ABI::Windows::Networking::Sockets::IStreamSocket *> pendingConnections;
    QList<ABI::Windows::Networking::Sockets::IStreamSocket *> currentConnections;
    QEventLoop eventLoop;
    QAbstractSocket *sslSocket;
    EventRegistrationToken connectionToken;

    bool emitReadReady = true;
    bool pendingReadNotification = false;

    HRESULT handleClientConnection(ABI::Windows::Networking::Sockets::IStreamSocketListener *tcpListener,
                                   ABI::Windows::Networking::Sockets::IStreamSocketListenerConnectionReceivedEventArgs *args);
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(WinRtDatagram)
Q_DECLARE_METATYPE(WinRTSocketEngine::ErrorString)

#endif // QNATIVESOCKETENGINE_WINRT_P_H
