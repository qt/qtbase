// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNATIVESOCKETENGINE_P_H
#define QNATIVESOCKETENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "QtNetwork/qhostaddress.h"
#include "QtNetwork/qnetworkinterface.h"
#include "private/qabstractsocketengine_p.h"
#ifndef Q_OS_WIN
#  include "qplatformdefs.h"
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_OS_WIN
#  define QT_SOCKLEN_T int
#  define QT_SOCKOPTLEN_T int
#endif

namespace {
namespace SetSALen {
    template <typename T> void set(T *sa, typename std::enable_if<(&T::sa_len, true), QT_SOCKLEN_T>::type len)
    { sa->sa_len = len; }
    template <typename T> void set(T *sin6, typename std::enable_if<(&T::sin6_len, true), QT_SOCKLEN_T>::type len)
    { sin6->sin6_len = len; }
    template <typename T> void set(T *, ...) {}
}
}

class QNativeSocketEnginePrivate;
#ifndef QT_NO_NETWORKINTERFACE
class QNetworkInterface;
#endif

class Q_AUTOTEST_EXPORT QNativeSocketEngine : public QAbstractSocketEngine
{
    Q_OBJECT
public:
    QNativeSocketEngine(QObject *parent = nullptr);
    ~QNativeSocketEngine();

    bool initialize(QAbstractSocket::SocketType type, QAbstractSocket::NetworkLayerProtocol protocol = QAbstractSocket::IPv4Protocol) override;
    bool initialize(qintptr socketDescriptor, QAbstractSocket::SocketState socketState = QAbstractSocket::ConnectedState) override;

    qintptr socketDescriptor() const override;

    bool isValid() const override;

    bool connectToHost(const QHostAddress &address, quint16 port) override;
    bool connectToHostByName(const QString &name, quint16 port) override;
    bool bind(const QHostAddress &address, quint16 port) override;
    bool listen(int backlog) override;
    qintptr accept() override;
    void close() override;

    qint64 bytesAvailable() const override;

    qint64 read(char *data, qint64 maxlen) override;
    qint64 write(const char *data, qint64 len) override;

#ifndef QT_NO_UDPSOCKET
#ifndef QT_NO_NETWORKINTERFACE
    bool joinMulticastGroup(const QHostAddress &groupAddress,
                            const QNetworkInterface &iface) override;
    bool leaveMulticastGroup(const QHostAddress &groupAddress,
                             const QNetworkInterface &iface) override;
    QNetworkInterface multicastInterface() const override;
    bool setMulticastInterface(const QNetworkInterface &iface) override;
#endif

    bool hasPendingDatagrams() const override;
    qint64 pendingDatagramSize() const override;
#endif // QT_NO_UDPSOCKET

    qint64 readDatagram(char *data, qint64 maxlen, QIpPacketHeader * = nullptr,
                        PacketHeaderOptions = WantNone) override;
    qint64 writeDatagram(const char *data, qint64 len, const QIpPacketHeader &) override;
    qint64 bytesToWrite() const override;

#if 0   // currently unused
    qint64 receiveBufferSize() const;
    void setReceiveBufferSize(qint64 bufferSize);

    qint64 sendBufferSize() const;
    void setSendBufferSize(qint64 bufferSize);
#endif

    int option(SocketOption option) const override;
    bool setOption(SocketOption option, int value) override;

    bool waitForRead(int msecs = 30000, bool *timedOut = nullptr) override;
    bool waitForWrite(int msecs = 30000, bool *timedOut = nullptr) override;
    bool waitForReadOrWrite(bool *readyToRead, bool *readyToWrite,
                            bool checkRead, bool checkWrite,
                            int msecs = 30000, bool *timedOut = nullptr) override;

    bool isReadNotificationEnabled() const override;
    void setReadNotificationEnabled(bool enable) override;
    bool isWriteNotificationEnabled() const override;
    void setWriteNotificationEnabled(bool enable) override;
    bool isExceptionNotificationEnabled() const override;
    void setExceptionNotificationEnabled(bool enable) override;

public Q_SLOTS:
    // non-virtual override;
    void connectionNotification();

private:
    Q_DECLARE_PRIVATE(QNativeSocketEngine)
    Q_DISABLE_COPY_MOVE(QNativeSocketEngine)
};

QT_END_NAMESPACE

#endif // QNATIVESOCKETENGINE_P_H
