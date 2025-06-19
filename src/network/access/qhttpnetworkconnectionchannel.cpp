// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2014 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:network-protocol

#include "qhttpnetworkconnectionchannel_p.h"
#include "qhttpnetworkconnection_p.h"
#include "qhttp2configuration.h"
#include "private/qnoncontiguousbytedevice_p.h"

#include <qdebug.h>

#include <private/qhttp2protocolhandler_p.h>
#include <private/qhttpprotocolhandler_p.h>
#include <private/http2protocol_p.h>
#include <private/qsocketabstraction_p.h>

#ifndef QT_NO_SSL
#    include <private/qsslsocket_p.h>
#    include <QtNetwork/qsslkey.h>
#    include <QtNetwork/qsslcipher.h>
#endif

#include "private/qnetconmonitor_p.h"

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <memory>
#include <utility>

QT_BEGIN_NAMESPACE

// TODO: Put channel specific stuff here so it does not pollute qhttpnetworkconnection.cpp

// Because in-flight when sending a request, the server might close our connection (because the persistent HTTP
// connection times out)
// We use 3 because we can get a _q_error 3 times depending on the timing:
static const int reconnectAttemptsDefault = 3;

QHttpNetworkConnectionChannel::QHttpNetworkConnectionChannel()
    : socket(nullptr)
    , ssl(false)
    , isInitialized(false)
    , state(IdleState)
    , reply(nullptr)
    , written(0)
    , bytesTotal(0)
    , resendCurrent(false)
    , lastStatus(0)
    , pendingEncrypt(false)
    , reconnectAttempts(reconnectAttemptsDefault)
    , authenticationCredentialsSent(false)
    , proxyCredentialsSent(false)
    , protocolHandler(nullptr)
#ifndef QT_NO_SSL
    , ignoreAllSslErrors(false)
#endif
    , pipeliningSupported(PipeliningSupportUnknown)
    , networkLayerPreference(QAbstractSocket::AnyIPProtocol)
    , connection(nullptr)
{
    // Inlining this function in the header leads to compiler error on
    // release-armv5, on at least timebox 9.2 and 10.1.
}

void QHttpNetworkConnectionChannel::init()
{
#ifndef QT_NO_SSL
    if (connection->d_func()->encrypt)
        socket = new QSslSocket;
#if QT_CONFIG(localserver)
    else if (connection->d_func()->isLocalSocket)
        socket = new QLocalSocket;
#endif
    else
        socket = new QTcpSocket;
#else
    socket = new QTcpSocket;
#endif
#ifndef QT_NO_NETWORKPROXY
    // Set by QNAM anyway, but let's be safe here
    if (auto s = qobject_cast<QAbstractSocket *>(socket))
        s->setProxy(QNetworkProxy::NoProxy);
#endif

    // After some back and forth in all the last years, this is now a DirectConnection because otherwise
    // the state inside the *Socket classes gets messed up, also in conjunction with the socket notifiers
    // which behave slightly differently on Windows vs Linux
    QObject::connect(socket, &QIODevice::bytesWritten,
                     this, &QHttpNetworkConnectionChannel::_q_bytesWritten,
                     Qt::DirectConnection);
    QObject::connect(socket, &QIODevice::readyRead,
                     this, &QHttpNetworkConnectionChannel::_q_readyRead,
                     Qt::DirectConnection);


    QSocketAbstraction::visit([this](auto *socket){
        using SocketType = std::remove_pointer_t<decltype(socket)>;
        QObject::connect(socket, &SocketType::connected,
                        this, &QHttpNetworkConnectionChannel::_q_connected,
                        Qt::DirectConnection);

        // The disconnected() and error() signals may already come
        // while calling connectToHost().
        // In case of a cached hostname or an IP this
        // will then emit a signal to the user of QNetworkReply
        // but cannot be caught because the user did not have a chance yet
        // to connect to QNetworkReply's signals.
        QObject::connect(socket, &SocketType::disconnected,
                        this, &QHttpNetworkConnectionChannel::_q_disconnected,
                        Qt::DirectConnection);
        if constexpr (std::is_same_v<SocketType, QAbstractSocket>) {
            QObject::connect(socket, &QAbstractSocket::errorOccurred,
                            this, &QHttpNetworkConnectionChannel::_q_error,
                            Qt::DirectConnection);
#if QT_CONFIG(localserver)
        } else if constexpr (std::is_same_v<SocketType, QLocalSocket>) {
            auto convertAndForward = [this](QLocalSocket::LocalSocketError error) {
                _q_error(static_cast<QAbstractSocket::SocketError>(error));
            };
            QObject::connect(socket, &SocketType::errorOccurred,
                            this, std::move(convertAndForward),
                            Qt::DirectConnection);
#endif
        }
    }, socket);



#ifndef QT_NO_NETWORKPROXY
    if (auto *s = qobject_cast<QAbstractSocket *>(socket)) {
        QObject::connect(s, &QAbstractSocket::proxyAuthenticationRequired,
                        this, &QHttpNetworkConnectionChannel::_q_proxyAuthenticationRequired,
                        Qt::DirectConnection);
    }
#endif

#ifndef QT_NO_SSL
    QSslSocket *sslSocket = qobject_cast<QSslSocket*>(socket);
    if (sslSocket) {
        // won't be a sslSocket if encrypt is false
        QObject::connect(sslSocket, &QSslSocket::encrypted,
                         this, &QHttpNetworkConnectionChannel::_q_encrypted,
                         Qt::DirectConnection);
        QObject::connect(sslSocket, &QSslSocket::sslErrors,
                         this, &QHttpNetworkConnectionChannel::_q_sslErrors,
                         Qt::DirectConnection);
        QObject::connect(sslSocket, &QSslSocket::preSharedKeyAuthenticationRequired,
                         this, &QHttpNetworkConnectionChannel::_q_preSharedKeyAuthenticationRequired,
                         Qt::DirectConnection);
        QObject::connect(sslSocket, &QSslSocket::encryptedBytesWritten,
                         this, &QHttpNetworkConnectionChannel::_q_encryptedBytesWritten,
                         Qt::DirectConnection);

        if (ignoreAllSslErrors)
            sslSocket->ignoreSslErrors();

        if (!ignoreSslErrorsList.isEmpty())
            sslSocket->ignoreSslErrors(ignoreSslErrorsList);

        if (sslConfiguration && !sslConfiguration->isNull())
           sslSocket->setSslConfiguration(*sslConfiguration);
    } else {
#endif // !QT_NO_SSL
        if (connection->connectionType() != QHttpNetworkConnection::ConnectionTypeHTTP2)
            protocolHandler.reset(new QHttpProtocolHandler(this));
#ifndef QT_NO_SSL
    }
#endif

#ifndef QT_NO_NETWORKPROXY
    if (auto *s = qobject_cast<QAbstractSocket *>(socket);
        s && proxy.type() != QNetworkProxy::NoProxy) {
        s->setProxy(proxy);
    }
#endif
    isInitialized = true;
}


void QHttpNetworkConnectionChannel::close()
{
    if (state == QHttpNetworkConnectionChannel::ClosingState)
        return;

    if (!socket)
        state = QHttpNetworkConnectionChannel::IdleState;
    else if (QSocketAbstraction::socketState(socket) == QAbstractSocket::UnconnectedState)
        state = QHttpNetworkConnectionChannel::IdleState;
    else
        state = QHttpNetworkConnectionChannel::ClosingState;

    // pendingEncrypt must only be true in between connected and encrypted states
    pendingEncrypt = false;

    if (socket) {
        // socket can be 0 since the host lookup is done from qhttpnetworkconnection.cpp while
        // there is no socket yet.
        socket->close();
    }
}


void QHttpNetworkConnectionChannel::abort()
{
    if (!socket)
        state = QHttpNetworkConnectionChannel::IdleState;
    else if (QSocketAbstraction::socketState(socket) == QAbstractSocket::UnconnectedState)
        state = QHttpNetworkConnectionChannel::IdleState;
    else
        state = QHttpNetworkConnectionChannel::ClosingState;

    // pendingEncrypt must only be true in between connected and encrypted states
    pendingEncrypt = false;

    if (socket) {
        // socket can be 0 since the host lookup is done from qhttpnetworkconnection.cpp while
        // there is no socket yet.
        auto callAbort = [](auto *s) {
            s->abort();
        };
        QSocketAbstraction::visit(callAbort, socket);
    }
}


void QHttpNetworkConnectionChannel::sendRequest()
{
    Q_ASSERT(protocolHandler);
    if (waitingForPotentialAbort) {
        needInvokeSendRequest = true;
        return;
    }
    protocolHandler->sendRequest();
}

/*
 * Invoke "protocolHandler->sendRequest" using a queued connection.
 * It's used to return to the event loop before invoking sendRequest when
 * there's a very real chance that the request could have been aborted
 * (i.e. after having emitted 'encrypted').
 */
void QHttpNetworkConnectionChannel::sendRequestDelayed()
{
    QMetaObject::invokeMethod(this, [this] {
        if (reply)
            sendRequest();
    }, Qt::ConnectionType::QueuedConnection);
}

void QHttpNetworkConnectionChannel::_q_receiveReply()
{
    Q_ASSERT(protocolHandler);
    if (waitingForPotentialAbort) {
        needInvokeReceiveReply = true;
        return;
    }
    protocolHandler->_q_receiveReply();
}

void QHttpNetworkConnectionChannel::_q_readyRead()
{
    Q_ASSERT(protocolHandler);
    if (waitingForPotentialAbort) {
        needInvokeReadyRead = true;
        return;
    }
    protocolHandler->_q_readyRead();
}

// called when unexpectedly reading a -1 or when data is expected but socket is closed
void QHttpNetworkConnectionChannel::handleUnexpectedEOF()
{
    Q_ASSERT(reply);
    if (reconnectAttempts <= 0 || !request.methodIsIdempotent()) {
        // too many errors reading/receiving/parsing the status, close the socket and emit error
        requeueCurrentlyPipelinedRequests();
        close();
        reply->d_func()->errorString = connection->d_func()->errorDetail(QNetworkReply::RemoteHostClosedError, socket);
        emit reply->finishedWithError(QNetworkReply::RemoteHostClosedError, reply->d_func()->errorString);
        reply = nullptr;
        if (protocolHandler)
            protocolHandler->setReply(nullptr);
        request = QHttpNetworkRequest();
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    } else {
        reconnectAttempts--;
        reply->d_func()->clear();
        reply->d_func()->connection = connection;
        reply->d_func()->connectionChannel = this;
        closeAndResendCurrentRequest();
    }
}

bool QHttpNetworkConnectionChannel::ensureConnection()
{
    if (!isInitialized)
        init();

    QAbstractSocket::SocketState socketState = QSocketAbstraction::socketState(socket);

    // resend this request after we receive the disconnected signal
    // If !socket->isOpen() then we have already called close() on the socket, but there was still a
    // pending connectToHost() for which we hadn't seen a connected() signal, yet. The connected()
    // has now arrived (as indicated by socketState != ClosingState), but we cannot send anything on
    // such a socket anymore.
    if (socketState == QAbstractSocket::ClosingState ||
            (socketState != QAbstractSocket::UnconnectedState && !socket->isOpen())) {
        if (reply)
            resendCurrent = true;
        return false;
    }

    // already trying to connect?
    if (socketState == QAbstractSocket::HostLookupState ||
        socketState == QAbstractSocket::ConnectingState) {
        return false;
    }

    // make sure that this socket is in a connected state, if not initiate
    // connection to the host.
    if (socketState != QAbstractSocket::ConnectedState) {
        // connect to the host if not already connected.
        state = QHttpNetworkConnectionChannel::ConnectingState;
        pendingEncrypt = ssl;

        // reset state
        pipeliningSupported = PipeliningSupportUnknown;
        authenticationCredentialsSent = false;
        proxyCredentialsSent = false;
        authenticator.detach();
        QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(authenticator);
        priv->hasFailed = false;
        proxyAuthenticator.detach();
        priv = QAuthenticatorPrivate::getPrivate(proxyAuthenticator);
        priv->hasFailed = false;

        // This workaround is needed since we use QAuthenticator for NTLM authentication. The "phase == Done"
        // is the usual criteria for emitting authentication signals. The "phase" is set to "Done" when the
        // last header for Authorization is generated by the QAuthenticator. Basic & Digest logic does not
        // check the "phase" for generating the Authorization header. NTLM authentication is a two stage
        // process & needs the "phase". To make sure the QAuthenticator uses the current username/password
        // the phase is reset to Start.
        priv = QAuthenticatorPrivate::getPrivate(authenticator);
        if (priv && priv->phase == QAuthenticatorPrivate::Done)
            priv->phase = QAuthenticatorPrivate::Start;
        priv = QAuthenticatorPrivate::getPrivate(proxyAuthenticator);
        if (priv && priv->phase == QAuthenticatorPrivate::Done)
            priv->phase = QAuthenticatorPrivate::Start;

        QString connectHost = connection->d_func()->hostName;
        quint16 connectPort = connection->d_func()->port;

        QHttpNetworkReply *potentialReply = connection->d_func()->predictNextRequestsReply();
        if (potentialReply) {
            QMetaObject::invokeMethod(potentialReply, "socketStartedConnecting", Qt::QueuedConnection);
        } else if (!h2RequestsToSend.isEmpty()) {
            QMetaObject::invokeMethod(std::as_const(h2RequestsToSend).first().second, "socketStartedConnecting", Qt::QueuedConnection);
        }

#ifndef QT_NO_NETWORKPROXY
        // HTTPS always use transparent proxy.
        if (connection->d_func()->networkProxy.type() != QNetworkProxy::NoProxy && !ssl) {
            connectHost = connection->d_func()->networkProxy.hostName();
            connectPort = connection->d_func()->networkProxy.port();
        }
        if (auto *abSocket = qobject_cast<QAbstractSocket *>(socket);
            abSocket && abSocket->proxy().type() == QNetworkProxy::HttpProxy) {
            // Make user-agent field available to HTTP proxy socket engine (QTBUG-17223)
            QByteArray value;
            // ensureConnection is called before any request has been assigned, but can also be
            // called again if reconnecting
            if (request.url().isEmpty()) {
                if (connection->connectionType()
                            == QHttpNetworkConnection::ConnectionTypeHTTP2Direct
                    || (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2
                        && !h2RequestsToSend.isEmpty())) {
                    value = std::as_const(h2RequestsToSend).first().first.headerField("user-agent");
                } else {
                    value = connection->d_func()->predictNextRequest().headerField("user-agent");
                }
            } else {
                value = request.headerField("user-agent");
            }
            if (!value.isEmpty()) {
                QNetworkProxy proxy(abSocket->proxy());
                auto h = proxy.headers();
                h.replaceOrAppend(QHttpHeaders::WellKnownHeader::UserAgent, value);
                proxy.setHeaders(std::move(h));
                abSocket->setProxy(proxy);
            }
        }
#endif
        if (ssl) {
#ifndef QT_NO_SSL
            QSslSocket *sslSocket = qobject_cast<QSslSocket*>(socket);

            // check whether we can re-use an existing SSL session
            // (meaning another socket in this connection has already
            // performed a full handshake)
            if (auto ctx = connection->sslContext())
                QSslSocketPrivate::checkSettingSslContext(sslSocket, std::move(ctx));

            sslSocket->setPeerVerifyName(connection->d_func()->peerVerifyName);
            sslSocket->connectToHostEncrypted(connectHost, connectPort, QIODevice::ReadWrite, networkLayerPreference);
            if (ignoreAllSslErrors)
                sslSocket->ignoreSslErrors();
            sslSocket->ignoreSslErrors(ignoreSslErrorsList);

            // limit the socket read buffer size. we will read everything into
            // the QHttpNetworkReply anyway, so let's grow only that and not
            // here and there.
            sslSocket->setReadBufferSize(64*1024);
#else
            // Need to dequeue the request so that we can emit the error.
            if (!reply)
                connection->d_func()->dequeueRequest(socket);
            connection->d_func()->emitReplyError(socket, reply, QNetworkReply::ProtocolUnknownError);
#endif
        } else {
            // In case of no proxy we can use the Unbuffered QTcpSocket
#ifndef QT_NO_NETWORKPROXY
            if (connection->d_func()->networkProxy.type() == QNetworkProxy::NoProxy
                    && connection->cacheProxy().type() == QNetworkProxy::NoProxy
                    && connection->transparentProxy().type() == QNetworkProxy::NoProxy) {
#endif
                if (auto *s = qobject_cast<QAbstractSocket *>(socket)) {
                    s->connectToHost(connectHost, connectPort,
                                     QIODevice::ReadWrite | QIODevice::Unbuffered,
                                     networkLayerPreference);
                    // For an Unbuffered QTcpSocket, the read buffer size has a special meaning.
                    s->setReadBufferSize(1 * 1024);
#if QT_CONFIG(localserver)
                } else if (auto *s = qobject_cast<QLocalSocket *>(socket)) {
                    s->connectToServer(connectHost);
#endif
                }
#ifndef QT_NO_NETWORKPROXY
            } else {
                auto *s = qobject_cast<QAbstractSocket *>(socket);
                Q_ASSERT(s);
                // limit the socket read buffer size. we will read everything into
                // the QHttpNetworkReply anyway, so let's grow only that and not
                // here and there.
                s->connectToHost(connectHost, connectPort, QIODevice::ReadWrite, networkLayerPreference);
                s->setReadBufferSize(64 * 1024);
            }
#endif
        }
        return false;
    }

    // This code path for ConnectedState
    if (pendingEncrypt) {
        // Let's only be really connected when we have received the encrypted() signal. Else the state machine seems to mess up
        // and corrupt the things sent to the server.
        return false;
    }

    return true;
}

void QHttpNetworkConnectionChannel::allDone()
{
    Q_ASSERT(reply);

    if (!reply) {
        qWarning("QHttpNetworkConnectionChannel::allDone() called without reply. Please report at http://bugreports.qt.io/");
        return;
    }

    // For clear text HTTP/2 we tried to upgrade from HTTP/1.1 to HTTP/2; for
    // ConnectionTypeHTTP2Direct we can never be here in case of failure
    // (after an attempt to read HTTP/1.1 as HTTP/2 frames) or we have a normal
    // HTTP/2 response and thus can skip this test:
    if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2
        && !ssl && !switchedToHttp2) {
        if (Http2::is_protocol_upgraded(*reply)) {
            switchedToHttp2 = true;
            protocolHandler->setReply(nullptr);

            // As allDone() gets called from the protocol handler, it's not yet
            // safe to delete it. There is no 'deleteLater', since
            // QAbstractProtocolHandler is not a QObject. Instead delete it in
            // a queued emission.

            QMetaObject::invokeMethod(this, [oldHandler = std::move(protocolHandler)]() mutable {
                oldHandler.reset();
            }, Qt::QueuedConnection);

            connection->fillHttp2Queue();
            protocolHandler.reset(new QHttp2ProtocolHandler(this));
            QHttp2ProtocolHandler *h2c = static_cast<QHttp2ProtocolHandler *>(protocolHandler.get());
            QMetaObject::invokeMethod(h2c, "_q_receiveReply", Qt::QueuedConnection);
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
            return;
        } else {
            // Ok, whatever happened, we do not try HTTP/2 anymore ...
            connection->setConnectionType(QHttpNetworkConnection::ConnectionTypeHTTP);
            connection->d_func()->activeChannelCount = connection->d_func()->channelCount;
        }
    }

    // while handling 401 & 407, we might reset the status code, so save this.
    bool emitFinished = reply->d_func()->shouldEmitSignals();
    bool connectionCloseEnabled = reply->d_func()->isConnectionCloseEnabled();
    detectPipeliningSupport();

    handleStatus();
    // handleStatus() might have removed the reply because it already called connection->emitReplyError()

    // queue the finished signal, this is required since we might send new requests from
    // slot connected to it. The socket will not fire readyRead signal, if we are already
    // in the slot connected to readyRead
    if (reply && emitFinished)
        QMetaObject::invokeMethod(reply, "finished", Qt::QueuedConnection);


    // reset the reconnection attempts after we receive a complete reply.
    // in case of failures, each channel will attempt two reconnects before emitting error.
    reconnectAttempts = reconnectAttemptsDefault;

    // now the channel can be seen as free/idle again, all signal emissions for the reply have been done
    if (state != QHttpNetworkConnectionChannel::ClosingState)
        state = QHttpNetworkConnectionChannel::IdleState;

    // if it does not need to be sent again we can set it to 0
    // the previous code did not do that and we had problems with accidental re-sending of a
    // finished request.
    // Note that this may trigger a segfault at some other point. But then we can fix the underlying
    // problem.
    if (!resendCurrent) {
        request = QHttpNetworkRequest();
        reply = nullptr;
        protocolHandler->setReply(nullptr);
    }

    // move next from pipeline to current request
    if (!alreadyPipelinedRequests.isEmpty()) {
        if (resendCurrent || connectionCloseEnabled || QSocketAbstraction::socketState(socket) != QAbstractSocket::ConnectedState) {
            // move the pipelined ones back to the main queue
            requeueCurrentlyPipelinedRequests();
            close();
        } else {
            // there were requests pipelined in and we can continue
            HttpMessagePair messagePair = alreadyPipelinedRequests.takeFirst();

            request = messagePair.first;
            reply = messagePair.second;
            protocolHandler->setReply(messagePair.second);
            state = QHttpNetworkConnectionChannel::ReadingState;
            resendCurrent = false;

            written = 0; // message body, excluding the header, irrelevant here
            bytesTotal = 0; // message body total, excluding the header, irrelevant here

            // pipeline even more
            connection->d_func()->fillPipeline(socket);

            // continue reading
            //_q_receiveReply();
            // this was wrong, allDone gets called from that function anyway.
        }
    } else if (alreadyPipelinedRequests.isEmpty() && socket->bytesAvailable() > 0) {
        // this is weird. we had nothing pipelined but still bytes available. better close it.
        close();

        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    } else if (alreadyPipelinedRequests.isEmpty()) {
        if (connectionCloseEnabled)
            if (QSocketAbstraction::socketState(socket) != QAbstractSocket::UnconnectedState)
                close();
        if (qobject_cast<QHttpNetworkConnection*>(connection))
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    }
}

void QHttpNetworkConnectionChannel::detectPipeliningSupport()
{
    Q_ASSERT(reply);
    // detect HTTP Pipelining support
    QByteArray serverHeaderField;
    if (
            // check for HTTP/1.1
            (reply->majorVersion() == 1 && reply->minorVersion() == 1)
            // check for not having connection close
            && (!reply->d_func()->isConnectionCloseEnabled())
            // check if it is still connected
            && (QSocketAbstraction::socketState(socket) == QAbstractSocket::ConnectedState)
            // check for broken servers in server reply header
            // this is adapted from http://mxr.mozilla.org/firefox/ident?i=SupportsPipelining
            && (serverHeaderField = reply->headerField("Server"), !serverHeaderField.contains("Microsoft-IIS/4."))
            && (!serverHeaderField.contains("Microsoft-IIS/5."))
            && (!serverHeaderField.contains("Netscape-Enterprise/3."))
            // this is adpoted from the knowledge of the Nokia 7.x browser team (DEF143319)
            && (!serverHeaderField.contains("WebLogic"))
            && (!serverHeaderField.startsWith("Rocket")) // a Python Web Server, see Web2py.com
            ) {
        pipeliningSupported = QHttpNetworkConnectionChannel::PipeliningProbablySupported;
    } else {
        pipeliningSupported = QHttpNetworkConnectionChannel::PipeliningSupportUnknown;
    }
}

// called when the connection broke and we need to queue some pipelined requests again
void QHttpNetworkConnectionChannel::requeueCurrentlyPipelinedRequests()
{
    for (int i = 0; i < alreadyPipelinedRequests.size(); i++)
        connection->d_func()->requeueRequest(alreadyPipelinedRequests.at(i));
    alreadyPipelinedRequests.clear();

    // only run when the QHttpNetworkConnection is not currently being destructed, e.g.
    // this function is called from _q_disconnected which is called because
    // of ~QHttpNetworkConnectionPrivate
    if (qobject_cast<QHttpNetworkConnection*>(connection))
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
}

void QHttpNetworkConnectionChannel::handleStatus()
{
    Q_ASSERT(socket);
    Q_ASSERT(reply);

    int statusCode = reply->statusCode();
    bool resend = false;

    switch (statusCode) {
    case 301:
    case 302:
    case 303:
    case 305:
    case 307:
    case 308: {
        // Parse the response headers and get the "location" url
        QUrl redirectUrl = connection->d_func()->parseRedirectResponse(socket, reply);
        if (redirectUrl.isValid())
            reply->setRedirectUrl(redirectUrl);

        if ((statusCode == 307 || statusCode == 308) && !resetUploadData()) {
            // Couldn't reset the upload data, which means it will be unable to POST the data -
            // this would lead to a long wait until it eventually failed and then retried.
            // Instead of doing that we fail here instead, resetUploadData will already have emitted
            // a ContentReSendError, so we're done.
        } else if (qobject_cast<QHttpNetworkConnection *>(connection)) {
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
        }
        break;
    }
    case 401: // auth required
    case 407: // proxy auth required
        if (connection->d_func()->handleAuthenticateChallenge(socket, reply, (statusCode == 407), resend)) {
            if (resend) {
                if (!resetUploadData())
                    break;

                reply->d_func()->eraseData();

                if (alreadyPipelinedRequests.isEmpty()) {
                    // this does a re-send without closing the connection
                    resendCurrent = true;
                    QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
                } else {
                    // we had requests pipelined.. better close the connection in closeAndResendCurrentRequest
                    closeAndResendCurrentRequest();
                    QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
                }
            } else {
                //authentication cancelled, close the channel.
                close();
            }
        } else {
            emit reply->headerChanged();
            emit reply->readyRead();
            QNetworkReply::NetworkError errorCode = (statusCode == 407)
                ? QNetworkReply::ProxyAuthenticationRequiredError
                : QNetworkReply::AuthenticationRequiredError;
            reply->d_func()->errorString = connection->d_func()->errorDetail(errorCode, socket);
            emit reply->finishedWithError(errorCode, reply->d_func()->errorString);
        }
        break;
    default:
        if (qobject_cast<QHttpNetworkConnection*>(connection))
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    }
}

bool QHttpNetworkConnectionChannel::resetUploadData()
{
    if (!reply) {
        //this happens if server closes connection while QHttpNetworkConnectionPrivate::_q_startNextRequest is pending
        return false;
    }
    if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2Direct
        || switchedToHttp2) {
        // The else branch doesn't make any sense for HTTP/2, since 1 channel is multiplexed into
        // many streams. And having one stream fail to reset upload data should not completely close
        // the channel. Handled in the http2 protocol handler.
    } else if (QNonContiguousByteDevice *uploadByteDevice = request.uploadByteDevice()) {
        if (!uploadByteDevice->reset()) {
            connection->d_func()->emitReplyError(socket, reply, QNetworkReply::ContentReSendError);
            return false;
        }
        written = 0;
    }
    return true;
}

#ifndef QT_NO_NETWORKPROXY

void QHttpNetworkConnectionChannel::setProxy(const QNetworkProxy &networkProxy)
{
    if (auto *s = qobject_cast<QAbstractSocket *>(socket))
        s->setProxy(networkProxy);

    proxy = networkProxy;
}

#endif

#ifndef QT_NO_SSL

void QHttpNetworkConnectionChannel::ignoreSslErrors()
{
    if (socket)
        static_cast<QSslSocket *>(socket)->ignoreSslErrors();

    ignoreAllSslErrors = true;
}


void QHttpNetworkConnectionChannel::ignoreSslErrors(const QList<QSslError> &errors)
{
    if (socket)
        static_cast<QSslSocket *>(socket)->ignoreSslErrors(errors);

    ignoreSslErrorsList = errors;
}

void QHttpNetworkConnectionChannel::setSslConfiguration(const QSslConfiguration &config)
{
    if (socket)
        static_cast<QSslSocket *>(socket)->setSslConfiguration(config);

    if (sslConfiguration)
        *sslConfiguration = config;
    else
        sslConfiguration = QSslConfiguration(config);
}

#endif

void QHttpNetworkConnectionChannel::pipelineInto(HttpMessagePair &pair)
{
    // this is only called for simple GET

    QHttpNetworkRequest &request = pair.first;
    QHttpNetworkReply *reply = pair.second;
    reply->d_func()->clear();
    reply->d_func()->connection = connection;
    reply->d_func()->connectionChannel = this;
    reply->d_func()->autoDecompress = request.d->autoDecompress;
    reply->d_func()->pipeliningUsed = true;

#ifndef QT_NO_NETWORKPROXY
    pipeline.append(QHttpNetworkRequestPrivate::header(request,
                                                           (connection->d_func()->networkProxy.type() != QNetworkProxy::NoProxy)));
#else
    pipeline.append(QHttpNetworkRequestPrivate::header(request, false));
#endif

    alreadyPipelinedRequests.append(pair);

    // pipelineFlush() needs to be called at some point afterwards
}

void QHttpNetworkConnectionChannel::pipelineFlush()
{
    if (pipeline.isEmpty())
        return;

    // The goal of this is so that we have everything in one TCP packet.
    // For the Unbuffered QTcpSocket this is manually needed, the buffered
    // QTcpSocket does it automatically.
    // Also, sometimes the OS does it for us (Nagle's algorithm) but that
    // happens only sometimes.
    socket->write(pipeline);
    pipeline.clear();
}


void QHttpNetworkConnectionChannel::closeAndResendCurrentRequest()
{
    requeueCurrentlyPipelinedRequests();
    close();
    if (reply)
        resendCurrent = true;
    if (qobject_cast<QHttpNetworkConnection*>(connection))
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
}

void QHttpNetworkConnectionChannel::resendCurrentRequest()
{
    requeueCurrentlyPipelinedRequests();
    if (reply)
        resendCurrent = true;
    if (qobject_cast<QHttpNetworkConnection*>(connection))
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
}

bool QHttpNetworkConnectionChannel::isSocketBusy() const
{
    return (state & QHttpNetworkConnectionChannel::BusyState);
}

bool QHttpNetworkConnectionChannel::isSocketWriting() const
{
    return (state & QHttpNetworkConnectionChannel::WritingState);
}

bool QHttpNetworkConnectionChannel::isSocketWaiting() const
{
    return (state & QHttpNetworkConnectionChannel::WaitingState);
}

bool QHttpNetworkConnectionChannel::isSocketReading() const
{
    return (state & QHttpNetworkConnectionChannel::ReadingState);
}

void QHttpNetworkConnectionChannel::_q_bytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);
    if (ssl) {
        // In the SSL case we want to send data from encryptedBytesWritten signal since that one
        // is the one going down to the actual network, not only into some SSL buffer.
        return;
    }

    // bytes have been written to the socket. write even more of them :)
    if (isSocketWriting())
        sendRequest();
    // otherwise we do nothing
}

void QHttpNetworkConnectionChannel::_q_disconnected()
{
    if (state == QHttpNetworkConnectionChannel::ClosingState) {
        state = QHttpNetworkConnectionChannel::IdleState;
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
        return;
    }

    // read the available data before closing (also done in _q_error for other codepaths)
    if ((isSocketWaiting() || isSocketReading()) && socket->bytesAvailable()) {
        if (reply) {
            state = QHttpNetworkConnectionChannel::ReadingState;
            _q_receiveReply();
        }
    } else if (state == QHttpNetworkConnectionChannel::IdleState && resendCurrent) {
        // re-sending request because the socket was in ClosingState
        QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    }
    state = QHttpNetworkConnectionChannel::IdleState;
    if (alreadyPipelinedRequests.size()) {
        // If nothing was in a pipeline, no need in calling
        // _q_startNextRequest (which it does):
        requeueCurrentlyPipelinedRequests();
    }

    pendingEncrypt = false;
}


void QHttpNetworkConnectionChannel::_q_connected_abstract_socket(QAbstractSocket *absSocket)
{
    // For the Happy Eyeballs we need to check if this is the first channel to connect.
    if (connection->d_func()->networkLayerState == QHttpNetworkConnectionPrivate::HostLookupPending || connection->d_func()->networkLayerState == QHttpNetworkConnectionPrivate::IPv4or6) {
        if (connection->d_func()->delayedConnectionTimer.isActive())
            connection->d_func()->delayedConnectionTimer.stop();
        if (networkLayerPreference == QAbstractSocket::IPv4Protocol)
            connection->d_func()->networkLayerState = QHttpNetworkConnectionPrivate::IPv4;
        else if (networkLayerPreference == QAbstractSocket::IPv6Protocol)
            connection->d_func()->networkLayerState = QHttpNetworkConnectionPrivate::IPv6;
        else {
            if (absSocket->peerAddress().protocol() == QAbstractSocket::IPv4Protocol)
                connection->d_func()->networkLayerState = QHttpNetworkConnectionPrivate::IPv4;
            else
                connection->d_func()->networkLayerState = QHttpNetworkConnectionPrivate::IPv6;
        }
        connection->d_func()->networkLayerDetected(networkLayerPreference);
        if (connection->d_func()->activeChannelCount > 1 && !connection->d_func()->encrypt)
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
    } else {
        bool anyProtocol = networkLayerPreference == QAbstractSocket::AnyIPProtocol;
        if (((connection->d_func()->networkLayerState == QHttpNetworkConnectionPrivate::IPv4)
             && (networkLayerPreference != QAbstractSocket::IPv4Protocol && !anyProtocol))
            || ((connection->d_func()->networkLayerState == QHttpNetworkConnectionPrivate::IPv6)
                && (networkLayerPreference != QAbstractSocket::IPv6Protocol && !anyProtocol))) {
            close();
            // This is the second connection so it has to be closed and we can schedule it for another request.
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
            return;
        }
        //The connections networkLayerState had already been decided.
    }

    // improve performance since we get the request sent by the kernel ASAP
    //absSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    // We have this commented out now. It did not have the effect we wanted. If we want to
    // do this properly, Qt has to combine multiple HTTP requests into one buffer
    // and send this to the kernel in one syscall and then the kernel immediately sends
    // it as one TCP packet because of TCP_NODELAY.
    // However, this code is currently not in Qt, so we rely on the kernel combining
    // the requests into one TCP packet.

    // not sure yet if it helps, but it makes sense
    absSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    pipeliningSupported = QHttpNetworkConnectionChannel::PipeliningSupportUnknown;

    if (QNetworkConnectionMonitor::isEnabled()) {
        auto connectionPrivate = connection->d_func();
        if (!connectionPrivate->connectionMonitor.isMonitoring()) {
            // Now that we have a pair of addresses, we can start monitoring the
            // connection status to handle its loss properly.
            if (connectionPrivate->connectionMonitor.setTargets(absSocket->localAddress(), absSocket->peerAddress()))
                connectionPrivate->connectionMonitor.startMonitoring();
        }
    }

    // ### FIXME: if the server closes the connection unexpectedly, we shouldn't send the same broken request again!
    //channels[i].reconnectAttempts = 2;
    if (ssl || pendingEncrypt) { // FIXME: Didn't work properly with pendingEncrypt only, we should refactor this into an EncrypingState
#ifndef QT_NO_SSL
        if (!connection->sslContext()) {
            // this socket is making the 1st handshake for this connection,
            // we need to set the SSL context so new sockets can reuse it
            if (auto socketSslContext = QSslSocketPrivate::sslContext(static_cast<QSslSocket*>(absSocket)))
                connection->setSslContext(std::move(socketSslContext));
        }
#endif
    } else if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
        state = QHttpNetworkConnectionChannel::IdleState;
        protocolHandler.reset(new QHttp2ProtocolHandler(this));
        if (h2RequestsToSend.size() > 0) {
            // In case our peer has sent us its settings (window size, max concurrent streams etc.)
            // let's give _q_receiveReply a chance to read them first ('invokeMethod', QueuedConnection).
            QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
        }
    } else {
        state = QHttpNetworkConnectionChannel::IdleState;
        const bool tryProtocolUpgrade = connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2;
        if (tryProtocolUpgrade) {
            // For HTTP/1.1 it's already created and never reset.
            protocolHandler.reset(new QHttpProtocolHandler(this));
        }
        switchedToHttp2 = false;

        if (!reply)
            connection->d_func()->dequeueRequest(absSocket);

        if (reply) {
            if (tryProtocolUpgrade) {
                // Let's augment our request with some magic headers and try to
                // switch to HTTP/2.
                Http2::appendProtocolUpgradeHeaders(connection->http2Parameters(), &request);
            }
            sendRequest();
        }
    }
}

#if QT_CONFIG(localserver)
void QHttpNetworkConnectionChannel::_q_connected_local_socket(QLocalSocket *localSocket)
{
    state = QHttpNetworkConnectionChannel::IdleState;
    if (!reply) // No reply object, try to dequeue a request (which is paired with a reply):
        connection->d_func()->dequeueRequest(localSocket);
    if (reply)
        sendRequest();
}
#endif

void QHttpNetworkConnectionChannel::_q_connected()
{
    if (auto *s = qobject_cast<QAbstractSocket *>(socket))
        _q_connected_abstract_socket(s);
#if QT_CONFIG(localserver)
    else if (auto *s = qobject_cast<QLocalSocket *>(socket))
        _q_connected_local_socket(s);
#endif
}

void QHttpNetworkConnectionChannel::_q_error(QAbstractSocket::SocketError socketError)
{
    if (!socket)
        return;
    QNetworkReply::NetworkError errorCode = QNetworkReply::UnknownNetworkError;

    switch (socketError) {
    case QAbstractSocket::HostNotFoundError:
        errorCode = QNetworkReply::HostNotFoundError;
        break;
    case QAbstractSocket::ConnectionRefusedError:
        errorCode = QNetworkReply::ConnectionRefusedError;
#ifndef QT_NO_NETWORKPROXY
        if (connection->d_func()->networkProxy.type() != QNetworkProxy::NoProxy && !ssl)
            errorCode = QNetworkReply::ProxyConnectionRefusedError;
#endif
        break;
    case QAbstractSocket::RemoteHostClosedError:
        // This error for SSL comes twice in a row, first from SSL layer ("The TLS/SSL connection has been closed") then from TCP layer.
        // Depending on timing it can also come three times in a row (first time when we try to write into a closing QSslSocket).
        // The reconnectAttempts handling catches the cases where we can re-send the request.
        if (!reply && state == QHttpNetworkConnectionChannel::IdleState) {
            // Not actually an error, it is normal for Keep-Alive connections to close after some time if no request
            // is sent on them. No need to error the other replies below. Just bail out here.
            // The _q_disconnected will handle the possibly pipelined replies. HTTP/2 is special for now,
            // we do not resend, but must report errors if any request is in progress (note, while
            // not in its sendRequest(), protocol handler switches the channel to IdleState, thus
            // this check is under this condition in 'if'):
            if (protocolHandler) {
                if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2Direct
                    || (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2
                        && switchedToHttp2)) {
                    auto h2Handler = static_cast<QHttp2ProtocolHandler *>(protocolHandler.get());
                    h2Handler->handleConnectionClosure();
                }
            }
            return;
        } else if (state != QHttpNetworkConnectionChannel::IdleState && state != QHttpNetworkConnectionChannel::ReadingState) {
            // Try to reconnect/resend before sending an error.
            // While "Reading" the _q_disconnected() will handle this.
            // If we're using ssl then the protocolHandler is not initialized until
            // "encrypted" has been emitted, since retrying requires the protocolHandler (asserted)
            // we will not try if encryption is not done.
            if (!pendingEncrypt && reconnectAttempts-- > 0) {
                resendCurrentRequest();
                return;
            } else {
                errorCode = QNetworkReply::RemoteHostClosedError;
            }
        } else if (state == QHttpNetworkConnectionChannel::ReadingState) {
            if (!reply)
                break;

            if (!reply->d_func()->expectContent()) {
                // No content expected, this is a valid way to have the connection closed by the server
                // We need to invoke this asynchronously to make sure the state() of the socket is on QAbstractSocket::UnconnectedState
                QMetaObject::invokeMethod(this, "_q_receiveReply", Qt::QueuedConnection);
                return;
            }
            if (reply->contentLength() == -1 && !reply->d_func()->isChunked()) {
                // There was no content-length header and it's not chunked encoding,
                // so this is a valid way to have the connection closed by the server
                // We need to invoke this asynchronously to make sure the state() of the socket is on QAbstractSocket::UnconnectedState
                QMetaObject::invokeMethod(this, "_q_receiveReply", Qt::QueuedConnection);
                return;
            }
            // ok, we got a disconnect even though we did not expect it
            // Try to read everything from the socket before we emit the error.
            if (socket->bytesAvailable()) {
                // Read everything from the socket into the reply buffer.
                // we can ignore the readbuffersize as the data is already
                // in memory and we will not receive more data on the socket.
                reply->setReadBufferSize(0);
                reply->setDownstreamLimited(false);
                _q_receiveReply();
                if (!reply) {
                    // No more reply assigned after the previous call? Then it had been finished successfully.
                    requeueCurrentlyPipelinedRequests();
                    state = QHttpNetworkConnectionChannel::IdleState;
                    QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
                    return;
                }
            }

            errorCode = QNetworkReply::RemoteHostClosedError;
        } else {
            errorCode = QNetworkReply::RemoteHostClosedError;
        }
        break;
    case QAbstractSocket::SocketTimeoutError:
        // try to reconnect/resend before sending an error.
        if (state == QHttpNetworkConnectionChannel::WritingState && (reconnectAttempts-- > 0)) {
            resendCurrentRequest();
            return;
        }
        errorCode = QNetworkReply::TimeoutError;
        break;
    case QAbstractSocket::ProxyConnectionRefusedError:
        errorCode = QNetworkReply::ProxyConnectionRefusedError;
        break;
    case QAbstractSocket::ProxyAuthenticationRequiredError:
        errorCode = QNetworkReply::ProxyAuthenticationRequiredError;
        break;
    case QAbstractSocket::SslHandshakeFailedError:
        errorCode = QNetworkReply::SslHandshakeFailedError;
        break;
    case QAbstractSocket::ProxyConnectionClosedError:
        // try to reconnect/resend before sending an error.
        if (reconnectAttempts-- > 0) {
            resendCurrentRequest();
            return;
        }
        errorCode = QNetworkReply::ProxyConnectionClosedError;
        break;
    case QAbstractSocket::ProxyConnectionTimeoutError:
        // try to reconnect/resend before sending an error.
        if (reconnectAttempts-- > 0) {
            resendCurrentRequest();
            return;
        }
        errorCode = QNetworkReply::ProxyTimeoutError;
        break;
    default:
        // all other errors are treated as NetworkError
        errorCode = QNetworkReply::UnknownNetworkError;
        break;
    }
    QPointer<QHttpNetworkConnection> that = connection;
    QString errorString = connection->d_func()->errorDetail(errorCode, socket, socket->errorString());

    // In the HostLookupPending state the channel should not emit the error.
    // This will instead be handled by the connection.
    if (!connection->d_func()->shouldEmitChannelError(socket))
        return;

    // emit error for all waiting replies
    do {
        // First requeue the already pipelined requests for the current failed reply,
        // then dequeue pending requests so we can also mark them as finished with error
        if (reply)
            requeueCurrentlyPipelinedRequests();
        else
            connection->d_func()->dequeueRequest(socket);

        if (reply) {
            reply->d_func()->errorString = errorString;
            reply->d_func()->httpErrorCode = errorCode;
            emit reply->finishedWithError(errorCode, errorString);
            reply = nullptr;
            if (protocolHandler)
                protocolHandler->setReply(nullptr);
        }
    } while (!connection->d_func()->highPriorityQueue.isEmpty()
             || !connection->d_func()->lowPriorityQueue.isEmpty());

    if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2
        || connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
        const auto h2RequestsToSendCopy = std::exchange(h2RequestsToSend, {});
        for (const auto &httpMessagePair : h2RequestsToSendCopy) {
            // emit error for all replies
            QHttpNetworkReply *currentReply = httpMessagePair.second;
            currentReply->d_func()->errorString = errorString;
            currentReply->d_func()->httpErrorCode = errorCode;
            Q_ASSERT(currentReply);
            emit currentReply->finishedWithError(errorCode, errorString);
        }
    }

    // send the next request
    QMetaObject::invokeMethod(that, "_q_startNextRequest", Qt::QueuedConnection);

    if (that) {
        //signal emission triggered event loop
        if (!socket)
            state = QHttpNetworkConnectionChannel::IdleState;
        else if (QSocketAbstraction::socketState(socket) == QAbstractSocket::UnconnectedState)
            state = QHttpNetworkConnectionChannel::IdleState;
        else
            state = QHttpNetworkConnectionChannel::ClosingState;

        // pendingEncrypt must only be true in between connected and encrypted states
        pendingEncrypt = false;
    }
}

#ifndef QT_NO_NETWORKPROXY
void QHttpNetworkConnectionChannel::_q_proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator* auth)
{
    if ((connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2
         && (switchedToHttp2 || h2RequestsToSend.size() > 0))
        || connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
        if (h2RequestsToSend.size() > 0)
            connection->d_func()->emitProxyAuthenticationRequired(this, proxy, auth);
    } else { // HTTP
        // Need to dequeue the request before we can emit the error.
        if (!reply)
            connection->d_func()->dequeueRequest(socket);
        if (reply)
            connection->d_func()->emitProxyAuthenticationRequired(this, proxy, auth);
    }
}
#endif

void QHttpNetworkConnectionChannel::_q_uploadDataReadyRead()
{
    if (reply)
        sendRequest();
}

void QHttpNetworkConnectionChannel::emitFinishedWithError(QNetworkReply::NetworkError error,
                                                          const char *message)
{
    if (reply)
        emit reply->finishedWithError(error, QHttpNetworkConnectionChannel::tr(message));
    const auto h2RequestsToSendCopy = h2RequestsToSend;
    for (const auto &httpMessagePair : h2RequestsToSendCopy) {
        QHttpNetworkReply *currentReply = httpMessagePair.second;
        Q_ASSERT(currentReply);
        emit currentReply->finishedWithError(error, QHttpNetworkConnectionChannel::tr(message));
    }
}

#ifndef QT_NO_SSL
void QHttpNetworkConnectionChannel::_q_encrypted()
{
    QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket);
    Q_ASSERT(sslSocket);

    if (!protocolHandler && connection->connectionType() != QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
        // ConnectionTypeHTTP2Direct does not rely on ALPN/NPN to negotiate HTTP/2,
        // after establishing a secure connection we immediately start sending
        // HTTP/2 frames.
        switch (sslSocket->sslConfiguration().nextProtocolNegotiationStatus()) {
        case QSslConfiguration::NextProtocolNegotiationNegotiated: {
            QByteArray nextProtocol = sslSocket->sslConfiguration().nextNegotiatedProtocol();
            if (nextProtocol == QSslConfiguration::NextProtocolHttp1_1) {
                // fall through to create a QHttpProtocolHandler
            } else if (nextProtocol == QSslConfiguration::ALPNProtocolHTTP2) {
                switchedToHttp2 = true;
                protocolHandler.reset(new QHttp2ProtocolHandler(this));
                connection->setConnectionType(QHttpNetworkConnection::ConnectionTypeHTTP2);
                break;
            } else {
                emitFinishedWithError(QNetworkReply::SslHandshakeFailedError,
                                      "detected unknown Next Protocol Negotiation protocol");
                break;
            }
        }
            Q_FALLTHROUGH();
        case QSslConfiguration::NextProtocolNegotiationUnsupported: // No agreement, try HTTP/1(.1)
        case QSslConfiguration::NextProtocolNegotiationNone: {
            protocolHandler.reset(new QHttpProtocolHandler(this));

            QSslConfiguration newConfiguration = sslSocket->sslConfiguration();
            QList<QByteArray> protocols = newConfiguration.allowedNextProtocols();
            const int nProtocols = protocols.size();
            // Clear the protocol that we failed to negotiate, so we do not try
            // it again on other channels that our connection can create/open.
            if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2)
                protocols.removeAll(QSslConfiguration::ALPNProtocolHTTP2);

            if (nProtocols > protocols.size()) {
                newConfiguration.setAllowedNextProtocols(protocols);
                const int channelCount = connection->d_func()->channelCount;
                for (int i = 0; i < channelCount; ++i)
                    connection->d_func()->channels[i].setSslConfiguration(newConfiguration);
            }

            connection->setConnectionType(QHttpNetworkConnection::ConnectionTypeHTTP);
            // We use only one channel for HTTP/2, but normally six for
            // HTTP/1.1 - let's restore this number to the reserved number of
            // channels:
            if (connection->d_func()->activeChannelCount < connection->d_func()->channelCount) {
                connection->d_func()->activeChannelCount = connection->d_func()->channelCount;
                // re-queue requests from HTTP/2 queue to HTTP queue, if any
                requeueHttp2Requests();
            }
            break;
        }
        default:
            emitFinishedWithError(QNetworkReply::SslHandshakeFailedError,
                                  "detected unknown Next Protocol Negotiation protocol");
        }
    } else if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2
               || connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
        // We have to reset QHttp2ProtocolHandler's state machine, it's a new
        // connection and the handler's state is unique per connection.
        protocolHandler.reset(new QHttp2ProtocolHandler(this));
    }

    if (!socket)
        return; // ### error
    state = QHttpNetworkConnectionChannel::IdleState;
    pendingEncrypt = false;

    if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2 ||
        connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
        if (!h2RequestsToSend.isEmpty()) {
            // Similar to HTTP/1.1 counterpart below:
            const auto &pair = std::as_const(h2RequestsToSend).first();
            waitingForPotentialAbort = true;
            emit pair.second->encrypted();

            // We don't send or handle any received data until any effects from
            // emitting encrypted() have been processed. This is necessary
            // because the user may have called abort(). We may also abort the
            // whole connection if the request has been aborted and there is
            // no more requests to send.
            QMetaObject::invokeMethod(this,
                                      &QHttpNetworkConnectionChannel::checkAndResumeCommunication,
                                      Qt::QueuedConnection);

            // In case our peer has sent us its settings (window size, max concurrent streams etc.)
            // let's give _q_receiveReply a chance to read them first ('invokeMethod', QueuedConnection).
        }
    } else { // HTTP
        if (!reply)
            connection->d_func()->dequeueRequest(socket);
        if (reply) {
            reply->setHttp2WasUsed(false);
            Q_ASSERT(reply->d_func()->connectionChannel == this);
            emit reply->encrypted();
        }
        if (reply)
            sendRequestDelayed();
    }
    QMetaObject::invokeMethod(connection, "_q_startNextRequest", Qt::QueuedConnection);
}


void QHttpNetworkConnectionChannel::checkAndResumeCommunication()
{
    Q_ASSERT(connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2
             || connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP2Direct);

    // Because HTTP/2 requires that we send a SETTINGS frame as the first thing we do, and respond
    // to a SETTINGS frame with an ACK, we need to delay any handling until we can ensure that any
    // effects from emitting encrypted() have been processed.
    // This function is called after encrypted() was emitted, so check for changes.

    if (!reply && h2RequestsToSend.isEmpty())
        abort();
    waitingForPotentialAbort = false;
    if (needInvokeReadyRead)
        _q_readyRead();
    if (needInvokeReceiveReply)
        _q_receiveReply();
    if (needInvokeSendRequest)
        sendRequest();
}

void QHttpNetworkConnectionChannel::requeueHttp2Requests()
{
    const auto h2RequestsToSendCopy = std::exchange(h2RequestsToSend, {});
    for (const auto &httpMessagePair : h2RequestsToSendCopy)
        connection->d_func()->requeueRequest(httpMessagePair);
}

void QHttpNetworkConnectionChannel::_q_sslErrors(const QList<QSslError> &errors)
{
    if (!socket)
        return;
    //QNetworkReply::NetworkError errorCode = QNetworkReply::ProtocolFailure;
    // Also pause the connection because socket notifiers may fire while an user
    // dialog is displaying
    connection->d_func()->pauseConnection();
    if (pendingEncrypt && !reply)
        connection->d_func()->dequeueRequest(socket);
    if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP) {
        if (reply)
            emit reply->sslErrors(errors);
    }
#ifndef QT_NO_SSL
    else { // HTTP/2
        const auto h2RequestsToSendCopy = h2RequestsToSend;
        for (const auto &httpMessagePair : h2RequestsToSendCopy) {
            // emit SSL errors for all replies
            QHttpNetworkReply *currentReply = httpMessagePair.second;
            Q_ASSERT(currentReply);
            emit currentReply->sslErrors(errors);
        }
    }
#endif // QT_NO_SSL
    connection->d_func()->resumeConnection();
}

void QHttpNetworkConnectionChannel::_q_preSharedKeyAuthenticationRequired(QSslPreSharedKeyAuthenticator *authenticator)
{
    connection->d_func()->pauseConnection();

    if (pendingEncrypt && !reply)
        connection->d_func()->dequeueRequest(socket);

    if (connection->connectionType() == QHttpNetworkConnection::ConnectionTypeHTTP) {
        if (reply)
            emit reply->preSharedKeyAuthenticationRequired(authenticator);
    } else {
        const auto h2RequestsToSendCopy = h2RequestsToSend;
        for (const auto &httpMessagePair : h2RequestsToSendCopy) {
            // emit SSL errors for all replies
            QHttpNetworkReply *currentReply = httpMessagePair.second;
            Q_ASSERT(currentReply);
            emit currentReply->preSharedKeyAuthenticationRequired(authenticator);
        }
    }

    connection->d_func()->resumeConnection();
}

void QHttpNetworkConnectionChannel::_q_encryptedBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);
    // bytes have been written to the socket. write even more of them :)
    if (isSocketWriting())
        sendRequest();
    // otherwise we do nothing
}

#endif

void QHttpNetworkConnectionChannel::setConnection(QHttpNetworkConnection *c)
{
    // Inlining this function in the header leads to compiler error on
    // release-armv5, on at least timebox 9.2 and 10.1.
    connection = c;
}

QT_END_NAMESPACE

#include "moc_qhttpnetworkconnectionchannel_p.cpp"
