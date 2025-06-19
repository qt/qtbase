// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:network-protocol

#include "access/http2/http2protocol_p.h"
#include "access/qhttp2connection_p.h"
#include "qhttpnetworkconnection_p.h"
#include "qhttp2protocolhandler_p.h"

#include "http2/http2frames_p.h"

#include <private/qnoncontiguousbytedevice_p.h>
#include <private/qsocketabstraction_p.h>

#include <QtNetwork/qabstractsocket.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qendian.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <QtCore/qnumeric.h>
#include <QtCore/qurl.h>

#include <qhttp2configuration.h>

#ifndef QT_NO_NETWORKPROXY
#  include <QtNetwork/qnetworkproxy.h>
#endif

#include <qcoreapplication.h>

#include <algorithm>
#include <vector>
#include <optional>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace
{

HPack::HttpHeader build_headers(const QHttpNetworkRequest &request, quint32 maxHeaderListSize,
                                bool useProxy)
{
    using namespace HPack;

    HttpHeader header;
    header.reserve(300);

    // 1. Before anything - mandatory fields, if they do not fit into maxHeaderList -
    // then stop immediately with error.
    const auto auth = request.url().authority(QUrl::FullyEncoded | QUrl::RemoveUserInfo).toLatin1();
    header.emplace_back(":authority", auth);
    header.emplace_back(":method", request.methodName());
    header.emplace_back(":path", request.uri(useProxy));
    header.emplace_back(":scheme", request.url().scheme().toLatin1());

    HeaderSize size = header_size(header);
    if (!size.first) // Ooops!
        return HttpHeader();

    if (size.second > maxHeaderListSize)
        return HttpHeader(); // Bad, we cannot send this request ...

    const QHttpHeaders requestHeader = request.header();
    for (qsizetype i = 0; i < requestHeader.size(); ++i) {
        const auto name = requestHeader.nameAt(i);
        const auto value = requestHeader.valueAt(i);
        const HeaderSize delta = entry_size(name, value);
        if (!delta.first) // Overflow???
            break;
        if (std::numeric_limits<quint32>::max() - delta.second < size.second)
            break;
        size.second += delta.second;
        if (size.second > maxHeaderListSize)
            break;

        if (name == "connection"_L1 || name == "host"_L1 || name == "keep-alive"_L1
            || name == "proxy-connection"_L1 || name == "transfer-encoding"_L1) {
            continue; // Those headers are not valid (section 3.2.1) - from QSpdyProtocolHandler
        }
        // TODO: verify with specs, which fields are valid to send ....
        //
        // Note: RFC 7450 8.1.2 (HTTP/2) states that header field names must be lower-cased
        // prior to their encoding in HTTP/2; header name fields in QHttpHeaders are already
        // lower-cased
        header.emplace_back(QByteArray{name.data(), name.size()},
                            QByteArray{value.data(), value.size()});
    }

    return header;
}

QUrl urlkey_from_request(const QHttpNetworkRequest &request)
{
    QUrl url;

    url.setScheme(request.url().scheme());
    url.setAuthority(request.url().authority(QUrl::FullyEncoded | QUrl::RemoveUserInfo));
    url.setPath(QLatin1StringView(request.uri(false)));

    return url;
}

} // Unnamed namespace

// Since we anyway end up having this in every function definition:
using namespace Http2;

QHttp2ProtocolHandler::QHttp2ProtocolHandler(QHttpNetworkConnectionChannel *channel)
    : QAbstractProtocolHandler(channel)
{
    const auto h2Config = m_connection->http2Parameters();

    if (!channel->ssl
        && m_connection->connectionType() != QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
        h2Connection = QHttp2Connection::createUpgradedConnection(channel->socket, h2Config);
        // Since we upgraded there is already one stream (the request was sent as http1)
        // and we need to handle it:
        QHttp2Stream *stream = h2Connection->getStream(1);
        Q_ASSERT(stream);
        Q_ASSERT(channel->reply);
        connectStream({ channel->request, channel->reply }, stream);
    } else {
        Q_ASSERT(QSocketAbstraction::socketState(channel->socket) == QAbstractSocket::ConnectedState);
        h2Connection = QHttp2Connection::createDirectConnection(channel->socket, h2Config);
    }
    connect(h2Connection, &QHttp2Connection::receivedGOAWAY, this,
            &QHttp2ProtocolHandler::handleGOAWAY);
    connect(h2Connection, &QHttp2Connection::errorOccurred, this,
            &QHttp2ProtocolHandler::connectionError);
    connect(h2Connection, &QHttp2Connection::newIncomingStream, this,
            [this](QHttp2Stream *stream){
                // Having our peer start streams doesn't make sense. We are
                // doing regular http request-response.
                stream->sendRST_STREAM(REFUSE_STREAM);
                if (!h2Connection->isGoingAway())
                    h2Connection->close(Http2::PROTOCOL_ERROR);
            });
}

void QHttp2ProtocolHandler::handleConnectionClosure()
{
    // The channel has just received RemoteHostClosedError and since it will
    // not try (for HTTP/2) to re-connect, it's time to finish all replies
    // with error.

    // Maybe we still have some data to read and can successfully finish
    // a stream/request?
    _q_receiveReply();
    h2Connection->handleConnectionClosure();
}

void QHttp2ProtocolHandler::_q_uploadDataDestroyed(QObject *uploadData)
{
    QPointer<QHttp2Stream> stream = streamIDs.take(uploadData);
    if (stream && stream->isActive())
        stream->sendRST_STREAM(CANCEL);
}

void QHttp2ProtocolHandler::_q_readyRead()
{
    _q_receiveReply();
}

void QHttp2ProtocolHandler::_q_receiveReply()
{
    // not using QObject::connect because the QHttpNetworkConnectionChannel
    // already handles the signals we care about, so we just call the slot
    // directly.
    Q_ASSERT(h2Connection);
    h2Connection->handleReadyRead();
}

bool QHttp2ProtocolHandler::sendRequest()
{
    if (h2Connection->isGoingAway()) {
        // Stop further calls to this method: we have received GOAWAY
        // so we cannot create new streams.
        m_channel->emitFinishedWithError(QNetworkReply::ProtocolUnknownError,
                                         "GOAWAY received, cannot start a request");
        m_channel->h2RequestsToSend.clear();
        return false;
    }

    // Process 'fake' (created by QNetworkAccessManager::connectToHostEncrypted())
    // requests first:
    auto &requests = m_channel->h2RequestsToSend;
    for (auto it = requests.begin(), endIt = requests.end(); it != endIt;) {
        const auto &pair = *it;
        if (pair.first.isPreConnect()) {
            m_connection->preConnectFinished();
            emit pair.second->finished();
            it = requests.erase(it);
            if (requests.empty()) {
                // Normally, after a connection was established and H2
                // was negotiated, we send a client preface. connectToHostEncrypted
                // though is not meant to send any data, it's just a 'preconnect'.
                // Thus we return early:
                return true;
            }
        } else {
            ++it;
        }
    }

    if (requests.empty())
        return true;

    m_channel->state = QHttpNetworkConnectionChannel::WritingState;
    // Check what was promised/pushed, maybe we do not have to send a request
    // and have a response already?

    for (auto it = requests.begin(), end = requests.end(); it != end;) {
        HttpMessagePair &httpPair = *it;

        QUrl promiseKey = urlkey_from_request(httpPair.first);
        if (h2Connection->promisedStream(promiseKey) != nullptr) {
            // There's a PUSH_PROMISE for this request, so we don't send one
            initReplyFromPushPromise(httpPair, promiseKey);
            it = requests.erase(it);
            continue;
        }

        QHttp2Stream *stream = createNewStream(httpPair);
        if (!stream) { // There was an issue creating the stream
            // Check if it was unrecoverable, ie. the reply is errored out and finished:
            if (httpPair.second->isFinished()) {
                it = requests.erase(it);
            }
            // ... either way we stop looping:
            break;
        }

        QHttpNetworkRequest &request = requestReplyPairs[stream].first;
        if (!sendHEADERS(stream, request)) {
            finishStreamWithError(stream, QNetworkReply::UnknownNetworkError,
                    "failed to send HEADERS frame(s)"_L1);
            continue;
        }
        if (request.uploadByteDevice()) {
            if (!sendDATA(stream, httpPair.second)) {
                finishStreamWithError(stream, QNetworkReply::UnknownNetworkError,
                                      "failed to send DATA frame(s)"_L1);
                continue;
            }
        }
        it = requests.erase(it);
    }

    m_channel->state = QHttpNetworkConnectionChannel::IdleState;

    return true;
}

/*!
    \internal
    This gets called during destruction of \a reply, so do not call any functions
    on \a reply. We check if there is a stream associated with the reply and,
    if there is, we remove the request-reply pair associated with this stream,
    delete the stream and return \c{true}. Otherwise nothing happens and we
    return \c{false}.
*/
bool QHttp2ProtocolHandler::tryRemoveReply(QHttpNetworkReply *reply)
{
    QHttp2Stream *stream = streamIDs.take(reply);
    if (stream) {
        stream->sendRST_STREAM(stream->isUploadingDATA() ? Http2::CANCEL : Http2::HTTP2_NO_ERROR);
        requestReplyPairs.remove(stream);
        stream->deleteLater();
        return true;
    }
    return false;
}

bool QHttp2ProtocolHandler::sendHEADERS(QHttp2Stream *stream, QHttpNetworkRequest &request)
{
    using namespace HPack;

    bool useProxy = false;
#ifndef QT_NO_NETWORKPROXY
    useProxy = m_connection->d_func()->networkProxy.type() != QNetworkProxy::NoProxy;
#endif
    if (request.withCredentials()) {
        m_connection->d_func()->createAuthorization(m_socket, request);
        request.d->needResendWithCredentials = false;
    }
    const auto headers = build_headers(request, h2Connection->maxHeaderListSize(), useProxy);
    if (headers.empty()) // nothing fits into maxHeaderListSize
        return false;

    bool mustUploadData = request.uploadByteDevice();
    return stream->sendHEADERS(headers, !mustUploadData);
}

bool QHttp2ProtocolHandler::sendDATA(QHttp2Stream *stream, QHttpNetworkReply *reply)
{
    Q_ASSERT(reply);
    QHttpNetworkReplyPrivate *replyPrivate = reply->d_func();
    Q_ASSERT(replyPrivate);
    QHttpNetworkRequest &request = replyPrivate->request;
    Q_ASSERT(request.uploadByteDevice());

    bool startedSending = stream->sendDATA(request.uploadByteDevice(), true);
    return startedSending && !stream->wasReset();
}

void QHttp2ProtocolHandler::handleHeadersReceived(const HPack::HttpHeader &headers, bool endStream)
{
    QHttp2Stream *stream = qobject_cast<QHttp2Stream *>(sender());
    Q_ASSERT(stream);
    auto &requestPair = requestReplyPairs[stream];
    auto *httpReply = requestPair.second;
    auto &httpRequest = requestPair.first;
    if (!httpReply)
        return;

    auto *httpReplyPrivate = httpReply->d_func();

    // For HTTP/1 'location' is handled (and redirect URL set) when a protocol
    // handler emits channel->allDone(). Http/2 protocol handler never emits
    // allDone, since we have many requests multiplexed in one channel at any
    // moment and we are probably not done yet. So we extract url and set it
    // here, if needed.
    int statusCode = 0;
    for (const auto &pair : headers) {
        const auto &name = pair.name;
        const auto value = QByteArrayView(pair.value);

        // TODO: part of this code copies what SPDY protocol handler does when
        // processing headers. Binary nature of HTTP/2 and SPDY saves us a lot
        // of parsing and related errors/bugs, but it would be nice to have
        // more detailed validation of headers.
        if (name == ":status") {
            statusCode = value.left(3).toInt();
            httpReply->setStatusCode(statusCode);
            m_channel->lastStatus = statusCode; // Mostly useless for http/2, needed for auth
            httpReply->setReasonPhrase(QString::fromLatin1(value.mid(4)));
        } else if (name == ":version") {
            httpReply->setMajorVersion(value.at(5) - '0');
            httpReply->setMinorVersion(value.at(7) - '0');
        } else if (name == "content-length") {
            bool ok = false;
            const qlonglong length = value.toLongLong(&ok);
            if (ok)
                httpReply->setContentLength(length);
        } else {
            const auto binder = name == "set-cookie" ? QByteArrayView("\n") : QByteArrayView(", ");
            httpReply->appendHeaderField(name, QByteArray(pair.value).replace('\0', binder));
        }
    }

    // Discard all informational (1xx) replies with the exception of 101.
    // Also see RFC 9110 (Chapter 15.2)
    if (statusCode == 100 || (102 <= statusCode && statusCode <= 199)) {
        httpReplyPrivate->clearHttpLayerInformation();
        return;
    }

    if (QHttpNetworkReply::isHttpRedirect(statusCode) && httpRequest.isFollowRedirects()) {
        QHttpNetworkConnectionPrivate::ParseRedirectResult
                result = QHttpNetworkConnectionPrivate::parseRedirectResponse(httpReply);
        if (result.errorCode != QNetworkReply::NoError) {
            auto errorString = m_connection->d_func()->errorDetail(result.errorCode, m_socket);
            finishStreamWithError(stream, result.errorCode, errorString);
            stream->sendRST_STREAM(INTERNAL_ERROR);
            return;
        }

        if (result.redirectUrl.isValid())
            httpReply->setRedirectUrl(result.redirectUrl);
    }

    if (httpReplyPrivate->isCompressed() && httpRequest.d->autoDecompress)
        httpReplyPrivate->removeAutoDecompressHeader();

    if (QHttpNetworkReply::isHttpRedirect(statusCode)) {
        // Note: This status code can trigger uploadByteDevice->reset() in
        // QHttpNetworkConnectionChannel::handleStatus. Alas, we have no single
        // request/reply, we multiplex several requests and thus we never simply
        // call 'handleStatus'. If we have a byte-device - we try to reset it
        // here, we don't (and can't) handle any error during reset operation.
        if (auto *byteDevice = httpRequest.uploadByteDevice()) {
            byteDevice->reset();
            httpReplyPrivate->totallyUploadedData = 0;
        }
    }

    QMetaObject::invokeMethod(httpReply, &QHttpNetworkReply::headerChanged, Qt::QueuedConnection);
    if (endStream)
        finishStream(stream, Qt::QueuedConnection);
}

void QHttp2ProtocolHandler::handleDataReceived(const QByteArray &data, bool endStream)
{
    QHttp2Stream *stream = qobject_cast<QHttp2Stream *>(sender());
    auto &httpPair = requestReplyPairs[stream];
    auto *httpReply = httpPair.second;
    if (!httpReply)
        return;
    Q_ASSERT(!stream->isPromisedStream());

    if (!data.isEmpty() && !httpPair.first.d->needResendWithCredentials) {
        auto *replyPrivate = httpReply->d_func();

        replyPrivate->totalProgress += data.size();

        replyPrivate->responseData.append(data);

        if (replyPrivate->shouldEmitSignals()) {
            QMetaObject::invokeMethod(httpReply, &QHttpNetworkReply::readyRead,
                                      Qt::QueuedConnection);
            QMetaObject::invokeMethod(httpReply, &QHttpNetworkReply::dataReadProgress,
                                      Qt::QueuedConnection, replyPrivate->totalProgress,
                                      replyPrivate->bodyLength);
        }
    }
    stream->clearDownloadBuffer();
    if (endStream)
        finishStream(stream, Qt::QueuedConnection);
}

// After calling this function, either the request will be re-sent or
// the reply will be finishedWithError! Do not emit finished() or similar on the
// reply after this!
void QHttp2ProtocolHandler::handleAuthorization(QHttp2Stream *stream)
{
    auto &requestPair = requestReplyPairs[stream];
    auto *httpReply = requestPair.second;
    auto *httpReplyPrivate = httpReply->d_func();
    auto &httpRequest = requestPair.first;

    Q_ASSERT(httpReply && (httpReply->statusCode() == 401 || httpReply->statusCode() == 407));

    const auto handleAuth = [&, this](QByteArrayView authField, bool isProxy) -> bool {
        Q_ASSERT(httpReply);
        const QByteArrayView auth = authField.trimmed();
        if (auth.startsWith("Negotiate") || auth.startsWith("NTLM")) {
            // @todo: We're supposed to fall back to http/1.1:
            // https://docs.microsoft.com/en-us/iis/get-started/whats-new-in-iis-10/http2-on-iis#when-is-http2-not-supported
            // "Windows authentication (NTLM/Kerberos/Negotiate) is not supported with HTTP/2.
            // In this case IIS will fall back to HTTP/1.1."
            // Though it might be OK to ignore this. The server shouldn't let us connect with
            // HTTP/2 if it doesn't support us using it.
            return false;
        }
        // Somewhat mimics parts of QHttpNetworkConnectionChannel::handleStatus
        bool resend = false;
        const bool authenticateHandled = m_connection->d_func()->handleAuthenticateChallenge(
                m_socket, httpReply, isProxy, resend);
        if (authenticateHandled) {
            if (resend) {
                httpReply->d_func()->eraseData();
                // Add the request back in queue, we'll retry later now that
                // we've gotten some username/password set on it:
                httpRequest.d->needResendWithCredentials = true;
                m_channel->h2RequestsToSend.insert(httpRequest.priority(), requestPair);
                httpReply->d_func()->clearHeaders();
                // If we have data we were uploading we need to reset it:
                if (auto *byteDevice = httpRequest.uploadByteDevice()) {
                    byteDevice->reset();
                    httpReplyPrivate->totallyUploadedData = 0;
                }
                // We automatically try to send new requests when the stream is
                // closed, so we don't need to call sendRequest ourselves.
                return true;
            } // else: we're just not resending the request.
            // @note In the http/1.x case we (at time of writing) call close()
            // for the connectionChannel (which is a bit weird, we could surely
            // reuse the open socket outside "connection:close"?), but in http2
            // we only have one channel, so we won't close anything.
        } else {
            // No authentication header or authentication isn't supported, but
            // we got a 401/407 so we cannot succeed. We need to emit signals
            // for headers and data, and then finishWithError.
            emit httpReply->headerChanged();
            emit httpReply->readyRead();
            QNetworkReply::NetworkError error = httpReply->statusCode() == 401
                    ? QNetworkReply::AuthenticationRequiredError
                    : QNetworkReply::ProxyAuthenticationRequiredError;
            finishStreamWithError(stream, QNetworkReply::AuthenticationRequiredError,
                                  m_connection->d_func()->errorDetail(error, m_socket));
        }
        return false;
    };

    // These statuses would in HTTP/1.1 be handled by
    // QHttpNetworkConnectionChannel::handleStatus. But because h2 has
    // multiple streams/requests in a single channel this structure does not
    // map properly to that function.
    bool authOk = true;
    switch (httpReply->statusCode()) {
    case 401:
        authOk = handleAuth(httpReply->headerField("www-authenticate"), false);
        break;
    case 407:
        authOk = handleAuth(httpReply->headerField("proxy-authenticate"), true);
        break;
    default:
        Q_UNREACHABLE();
    }
    if (authOk) {
        stream->sendRST_STREAM(CANCEL);
    } // else: errors handled inside handleAuth
}

// Called when we have received a frame with the END_STREAM flag set
void QHttp2ProtocolHandler::finishStream(QHttp2Stream *stream, Qt::ConnectionType connectionType)
{
    if (stream->state() != QHttp2Stream::State::Closed)
        stream->sendRST_STREAM(CANCEL);

    auto &pair = requestReplyPairs[stream];
    auto *httpReply = pair.second;
    if (httpReply) {
        int statusCode = httpReply->statusCode();
        if (statusCode == 401 || statusCode == 407) {
            // handleAuthorization will either re-send the request or
            // finishWithError. In either case we don't want to emit finished
            // here.
            handleAuthorization(stream);
            return;
        }

        httpReply->disconnect(this);

        if (!pair.first.d->needResendWithCredentials) {
            if (connectionType == Qt::DirectConnection)
                emit httpReply->finished();
            else
                QMetaObject::invokeMethod(httpReply, &QHttpNetworkReply::finished, connectionType);
        }
    }

    qCDebug(QT_HTTP2) << "stream" << stream->streamID() << "closed";
    stream->deleteLater();
}

void QHttp2ProtocolHandler::handleGOAWAY(Http2Error errorCode, quint32 lastStreamID)
{
    qCDebug(QT_HTTP2) << "GOAWAY received, error code:" << errorCode << "last stream ID:"
                      << lastStreamID;

    // For the requests (and streams) we did not start yet, we have to report an
    // error.
    m_channel->emitFinishedWithError(QNetworkReply::ProtocolUnknownError,
                                     "GOAWAY received, cannot start a request");
    // Also, prevent further calls to sendRequest:
    m_channel->h2RequestsToSend.clear();

    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QString message;
    qt_error(errorCode, error, message);

    // Even if the GOAWAY frame contains NO_ERROR we must send an error
    // when terminating streams to ensure users can distinguish from a
    // successful completion.
    if (errorCode == HTTP2_NO_ERROR) {
        error = QNetworkReply::ContentReSendError;
        message = "Server stopped accepting new streams before this stream was established"_L1;
    }
}

void QHttp2ProtocolHandler::finishStreamWithError(QHttp2Stream *stream, Http2Error errorCode)
{
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QString message;
    qt_error(errorCode, error, message);
    finishStreamWithError(stream, error, message);
}

void QHttp2ProtocolHandler::finishStreamWithError(QHttp2Stream *stream,
        QNetworkReply::NetworkError error, const QString &message)
{
    stream->sendRST_STREAM(CANCEL);
    const HttpMessagePair &pair = requestReplyPairs.value(stream);
    if (auto *httpReply = pair.second) {
        httpReply->disconnect(this);

        // TODO: error message must be translated!!! (tr)
        emit httpReply->finishedWithError(error, message);
    }

    qCWarning(QT_HTTP2) << "stream" << stream->streamID() << "finished with error:" << message;
}

/*!
    \internal

    Creates a QHttp2Stream for the request, will return \nullptr if the stream
    could not be created for some reason, and will finish the reply if required.
*/
QHttp2Stream *QHttp2ProtocolHandler::createNewStream(const HttpMessagePair &message,
                                                     bool uploadDone)
{
    QUrl streamKey = urlkey_from_request(message.first);
    if (auto promisedStream = h2Connection->promisedStream(streamKey)) {
        Q_ASSERT(promisedStream->state() != QHttp2Stream::State::Closed);
        return promisedStream;
    }

    QH2Expected<QHttp2Stream *, QHttp2Connection::CreateStreamError>
            streamResult = h2Connection->createStream();
    if (!streamResult.ok()) {
        if (streamResult.error()
            == QHttp2Connection::CreateStreamError::MaxConcurrentStreamsReached) {
            // We have to wait for a stream to be closed before we can create a new one, so
            // we just return nullptr, the caller should not remove it from the queue.
            return nullptr;
        }
        qCDebug(QT_HTTP2) << "failed to create new stream:" << streamResult.error();
        auto *reply = message.second;
        const char *cstr = "Failed to initialize HTTP/2 stream with errorcode: %1";
        const QString errorString = QCoreApplication::tr("QHttp", cstr)
                                            .arg(QDebug::toString(streamResult.error()));
        emit reply->finishedWithError(QNetworkReply::ProtocolFailure, errorString);
        return nullptr;
    }
    QHttp2Stream *stream = streamResult.unwrap();

    if (!uploadDone) {
        if (auto *src = message.first.uploadByteDevice()) {
            connect(src, &QObject::destroyed, this, &QHttp2ProtocolHandler::_q_uploadDataDestroyed);
            streamIDs.insert(src, stream);
        }
    }

    auto *reply = message.second;
    QMetaObject::invokeMethod(reply, &QHttpNetworkReply::requestSent, Qt::QueuedConnection);

    connectStream(message, stream);
    return stream;
}

void QHttp2ProtocolHandler::connectStream(const HttpMessagePair &message, QHttp2Stream *stream)
{
    auto *reply = message.second;
    auto *replyPrivate = reply->d_func();
    replyPrivate->connection = m_connection;
    replyPrivate->connectionChannel = m_channel;

    reply->setHttp2WasUsed(true);
    QPointer<QHttp2Stream> &oldStream = streamIDs[reply];
    if (oldStream)
        disconnect(oldStream, nullptr, this, nullptr);
    oldStream = stream;
    requestReplyPairs.emplace(stream, message);

    QObject::connect(stream, &QHttp2Stream::headersReceived, this,
                     &QHttp2ProtocolHandler::handleHeadersReceived);
    QObject::connect(stream, &QHttp2Stream::dataReceived, this,
                     &QHttp2ProtocolHandler::handleDataReceived);
    QObject::connect(stream, &QHttp2Stream::errorOccurred, this,
                     [this, stream](Http2Error errorCode, const QString &errorString) {
                         qCWarning(QT_HTTP2)
                                 << "stream" << stream->streamID() << "error:" << errorString;
                         finishStreamWithError(stream, errorCode);
                     });

    QObject::connect(stream, &QHttp2Stream::stateChanged, this, [this](QHttp2Stream::State state) {
        if (state == QHttp2Stream::State::Closed) {
            // Try to send more requests if we have any
            if (!m_channel->h2RequestsToSend.empty()) {
                QMetaObject::invokeMethod(this, &QHttp2ProtocolHandler::sendRequest,
                                          Qt::QueuedConnection);
            }
        }
    });
}

void QHttp2ProtocolHandler::initReplyFromPushPromise(const HttpMessagePair &message,
                                                     const QUrl &cacheKey)
{
    QHttp2Stream *promise = h2Connection->promisedStream(cacheKey);
    Q_ASSERT(promise);
    Q_ASSERT(message.second);
    message.second->setHttp2WasUsed(true);

    qCDebug(QT_HTTP2) << "found cached/promised response on stream" << promise->streamID();

    const bool replyFinished = promise->state() == QHttp2Stream::State::Closed;

    connectStream(message, promise);

    // Now that we have connect()ed, re-emit signals so that the reply
    // can be processed as usual:

    QByteDataBuffer downloadBuffer = promise->takeDownloadBuffer();
    if (const auto &headers = promise->receivedHeaders(); !headers.empty())
        emit promise->headersReceived(headers, replyFinished && downloadBuffer.isEmpty());

    if (!downloadBuffer.isEmpty()) {
        for (qsizetype i = 0; i < downloadBuffer.bufferCount(); ++i) {
            const bool streamEnded = replyFinished && i == downloadBuffer.bufferCount() - 1;
            emit promise->dataReceived(downloadBuffer[i], streamEnded);
        }
    }
}

void QHttp2ProtocolHandler::connectionError(Http2::Http2Error errorCode, const QString &message)
{
    Q_ASSERT(!message.isNull());

    qCCritical(QT_HTTP2) << "connection error:" << message;

    const auto error = qt_error(errorCode);
    m_channel->emitFinishedWithError(error, qPrintable(message));

    closeSession();
}

void QHttp2ProtocolHandler::closeSession()
{
    m_channel->close();
}

QT_END_NAMESPACE

#include "moc_qhttp2protocolhandler_p.cpp"
