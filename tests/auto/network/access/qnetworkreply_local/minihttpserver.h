// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MINIHTTPSERVER_H
#define MINIHTTPSERVER_H

#include <QtNetwork/qtnetworkglobal.h>

#include <QtNetwork/qtcpserver.h>
#include <QtNetwork/qtcpsocket.h>
#if QT_CONFIG(ssl)
#  include <QtNetwork/qsslsocket.h>
#endif
#if QT_CONFIG(localserver)
#  include <QtNetwork/qlocalserver.h>
#  include <QtNetwork/qlocalsocket.h>
#endif

#include <QtCore/qpointer.h>
#include <QtCore/qhash.h>

#include <utility>

static inline QByteArray default200Response()
{
    return QByteArrayLiteral("HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/plain\r\n"
                             "Content-Length: 12\r\n"
                             "\r\n"
                             "Hello World!");
}
class MiniHttpServerV2 : public QObject
{
    Q_OBJECT

public:
    struct State;

#if QT_CONFIG(localserver)
    void bind(QLocalServer *server)
    {
        Q_ASSERT(!localServer);
        localServer = server;
        connect(server, &QLocalServer::newConnection, this,
                &MiniHttpServerV2::incomingLocalConnection);
    }
#endif

    void bind(QTcpServer *server)
    {
        Q_ASSERT(!tcpServer);
        tcpServer = server;
        connect(server, &QTcpServer::pendingConnectionAvailable, this,
                &MiniHttpServerV2::incomingConnection);
    }

    void setDataToTransmit(QByteArray data) { dataToTransmit = std::move(data); }

    void clearServerState()
    {
        auto copy = std::exchange(clientStates, {});
        for (auto [socket, _] : copy.asKeyValueRange()) {
            if (auto *tcpSocket = qobject_cast<QTcpSocket *>(socket))
                tcpSocket->disconnectFromHost();
#if QT_CONFIG(localserver)
            else if (auto *localSocket = qobject_cast<QLocalSocket *>(socket))
                localSocket->disconnectFromServer();
#endif
            else
                Q_UNREACHABLE_RETURN();
            socket->deleteLater();
        }
    }

    bool hasPendingConnections() const
    {
        return
#if QT_CONFIG(localserver)
                (localServer && localServer->hasPendingConnections()) ||
#endif
                (tcpServer && tcpServer->hasPendingConnections());
    }

    QString addressForScheme(QStringView scheme) const
    {
        using namespace Qt::StringLiterals;
        if (scheme.startsWith("unix"_L1) || scheme.startsWith("local"_L1)) {
#if QT_CONFIG(localserver)
            if (localServer)
                return localServer->serverName();
#endif
        } else if (scheme == "http"_L1) {
            if (tcpServer)
                return "%1:%2"_L1.arg(tcpServer->serverAddress().toString(),
                                      QString::number(tcpServer->serverPort()));
        }
        return {};
    }

    QList<State> peerStates() const { return clientStates.values(); }

protected:
#if QT_CONFIG(localserver)
    void incomingLocalConnection()
    {
        auto *socket = localServer->nextPendingConnection();
        connectSocketSignals(socket);
    }
#endif

    void incomingConnection()
    {
        auto *socket = tcpServer->nextPendingConnection();
        connectSocketSignals(socket);
    }

    void reply(QIODevice *socket)
    {
        Q_ASSERT(socket);
        if (dataToTransmit.isEmpty()) {
            emit socket->bytesWritten(0); // emulate having written the data
            return;
        }
        if (!stopTransfer)
            socket->write(dataToTransmit);
    }

private:
    void connectSocketSignals(QIODevice *socket)
    {
        connect(socket, &QIODevice::readyRead, this, [this, socket]() { readyReadSlot(socket); });
        connect(socket, &QIODevice::bytesWritten, this,
                [this, socket]() { bytesWrittenSlot(socket); });
#if QT_CONFIG(ssl)
        if (auto *sslSocket = qobject_cast<QSslSocket *>(socket))
            connect(sslSocket, &QSslSocket::sslErrors, this, &MiniHttpServerV2::slotSslErrors);
#endif

        if (auto *tcpSocket = qobject_cast<QTcpSocket *>(socket)) {
            connect(tcpSocket, &QAbstractSocket::errorOccurred, this, &MiniHttpServerV2::slotError);
#if QT_CONFIG(localserver)
        } else if (auto *localSocket = qobject_cast<QLocalSocket *>(socket)) {
            connect(localSocket, &QLocalSocket::errorOccurred, this,
                    [this](QLocalSocket::LocalSocketError error) {
                        slotError(QAbstractSocket::SocketError(error));
                    });
#endif
        } else {
            Q_UNREACHABLE_RETURN();
        }
    }

    void parseContentLength(State &st, QByteArrayView header)
    {
        qsizetype index = header.indexOf("\r\ncontent-length:");
        if (index == -1)
            return;
        st.foundContentLength = true;

        index += sizeof("\r\ncontent-length:") - 1;
        const auto *end = std::find(header.cbegin() + index, header.cend(), '\r');
        QByteArrayView num = header.mid(index, std::distance(header.cbegin() + index, end));
        bool ok = false;
        st.contentLength = num.toInt(&ok);
        if (!ok)
            st.contentLength = -1;
    }

private slots:
#if QT_CONFIG(ssl)
    void slotSslErrors(const QList<QSslError> &errors)
    {
        QTcpSocket *currentClient = qobject_cast<QTcpSocket *>(sender());
        Q_ASSERT(currentClient);
        qDebug() << "slotSslErrors" << currentClient->errorString() << errors;
    }
#endif
    void slotError(QAbstractSocket::SocketError err)
    {
        QTcpSocket *currentClient = qobject_cast<QTcpSocket *>(sender());
        Q_ASSERT(currentClient);
        qDebug() << "slotError" << err << currentClient->errorString();
    }

public slots:

    void readyReadSlot(QIODevice *socket)
    {
        if (stopTransfer)
            return;
        State &st = clientStates[socket];
        st.receivedData += socket->readAll();
        const qsizetype doubleEndlPos = st.receivedData.indexOf("\r\n\r\n");

        if (doubleEndlPos != -1) {
            const qsizetype endOfHeader = doubleEndlPos + 4;
            st.contentRead = st.receivedData.size() - endOfHeader;

            if (!st.checkedContentLength) {
                parseContentLength(st, QByteArrayView(st.receivedData).first(endOfHeader));
                st.checkedContentLength = true;
            }

            if (st.contentRead < st.contentLength)
                return;

            // multiple requests incoming, remove the bytes of the current one
            if (multiple)
                st.receivedData.remove(0, endOfHeader);

            reply(socket);
        }
    }

    void bytesWrittenSlot(QIODevice *socket)
    {
        // Disconnect and delete in next cycle (else Windows clients will fail with
        // RemoteHostClosedError).
        if (doClose && socket->bytesToWrite() == 0) {
            disconnect(socket, nullptr, this, nullptr);
            socket->deleteLater();
        }
    }

private:
    QByteArray dataToTransmit = default200Response();

    QTcpServer *tcpServer = nullptr;
#if QT_CONFIG(localserver)
    QLocalServer *localServer = nullptr;
#endif

    QHash<QIODevice *, State> clientStates;

public:
    struct State
    {
        QByteArray receivedData;
        qsizetype contentLength = 0;
        qsizetype contentRead = 0;
        bool checkedContentLength = false;
        bool foundContentLength = false;
    };

    bool doClose = true;
    bool multiple = false;
    bool stopTransfer = false;
};

#endif // MINIHTTPSERVER_H
