// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhttpnetworkconnection_p.h"
#include <private/qabstractsocket_p.h>
#include "qhttpnetworkconnectionchannel_p.h"
#include "private/qnoncontiguousbytedevice_p.h"
#include <private/qnetworkrequest_p.h>
#include <private/qobject_p.h>
#include <private/qauthenticator_p.h>
#include "private/qhostinfo_p.h"
#include <qnetworkproxy.h>
#include <qauthenticator.h>
#include <qcoreapplication.h>
#include <private/qdecompresshelper_p.h>

#include <qbuffer.h>
#include <qpair.h>
#include <qdebug.h>

#ifndef QT_NO_SSL
#    include <private/qsslsocket_p.h>
#    include <QtNetwork/qsslkey.h>
#    include <QtNetwork/qsslcipher.h>
#    include <QtNetwork/qsslconfiguration.h>
#    include <QtNetwork/qsslerror.h>
#endif



QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Note: Only used from auto tests, normal usage is via QHttp1Configuration
const int QHttpNetworkConnectionPrivate::defaultHttpChannelCount = 6;

// The pipeline length. So there will be 4 requests in flight.
const int QHttpNetworkConnectionPrivate::defaultPipelineLength = 3;
// Only re-fill the pipeline if there's defaultRePipelineLength slots free in the pipeline.
// This means that there are 2 requests in flight and 2 slots free that will be re-filled.
const int QHttpNetworkConnectionPrivate::defaultRePipelineLength = 2;


QHttpNetworkConnectionPrivate::QHttpNetworkConnectionPrivate(const QString &hostName,
                                                             quint16 port, bool encrypt,
                                                             QHttpNetworkConnection::ConnectionType type)
: state(RunningState),
  networkLayerState(Unknown),
  hostName(hostName), port(port), encrypt(encrypt), delayIpv4(true)
  , activeChannelCount(type == QHttpNetworkConnection::ConnectionTypeHTTP2
                       || type == QHttpNetworkConnection::ConnectionTypeHTTP2Direct
                       ? 1 : defaultHttpChannelCount)
  , channelCount(defaultHttpChannelCount)
#ifndef QT_NO_NETWORKPROXY
  , networkProxy(QNetworkProxy::NoProxy)
#endif
  , preConnectRequests(0)
  , connectionType(type)
{
    // We allocate all 6 channels even if it's HTTP/2 enabled connection:
    // in case the protocol negotiation via NPN/ALPN fails, we will have
    // normally working HTTP/1.1.
    Q_ASSERT(channelCount >= activeChannelCount);
    channels = new QHttpNetworkConnectionChannel[channelCount];
}

QHttpNetworkConnectionPrivate::QHttpNetworkConnectionPrivate(quint16 connectionCount, const QString &hostName,
                                                             quint16 port, bool encrypt,
                                                             QHttpNetworkConnection::ConnectionType type)
: state(RunningState), networkLayerState(Unknown),
  hostName(hostName), port(port), encrypt(encrypt), delayIpv4(true),
  channelCount(connectionCount)
#ifndef QT_NO_NETWORKPROXY
  , networkProxy(QNetworkProxy::NoProxy)
#endif
  , preConnectRequests(0)
  , connectionType(type)
{
    channels = new QHttpNetworkConnectionChannel[channelCount];

    activeChannelCount = (type == QHttpNetworkConnection::ConnectionTypeHTTP2 ||
                          type == QHttpNetworkConnection::ConnectionTypeHTTP2Direct)
                       ? 1 : connectionCount;
    // We allocate all 6 channels even if it's an HTTP/2-enabled
    // connection: in case the protocol negotiation via NPN/ALPN fails,
    // we will have normally working HTTP/1.1.
    Q_ASSERT(channelCount >= activeChannelCount);
}



QHttpNetworkConnectionPrivate::~QHttpNetworkConnectionPrivate()
{
    for (int i = 0; i < channelCount; ++i) {
        if (channels[i].socket) {
            QObject::disconnect(channels[i].socket, nullptr, &channels[i], nullptr);
            channels[i].socket->close();
            delete channels[i].socket;
        }
    }
    delete []channels;
}

void QHttpNetworkConnectionPrivate::init()
{
    Q_Q(QHttpNetworkConnection);
    for (int i = 0; i < channelCount; i++) {
        channels[i].setConnection(this->q_func());
        channels[i].ssl = encrypt;
    }

    delayedConnectionTimer.setSingleShot(true);
    QObject::connect(&delayedConnectionTimer, SIGNAL(timeout()), q, SLOT(_q_connectDelayedChannel()));
}

void QHttpNetworkConnectionPrivate::pauseConnection()
{
    state = PausedState;

    // Disable all socket notifiers
    for (int i = 0; i < activeChannelCount; i++) {
        if (channels[i].socket) {
#ifndef QT_NO_SSL
            if (encrypt)
                QSslSocketPrivate::pauseSocketNotifiers(static_cast<QSslSocket*>(channels[i].socket));
            else
#endif
                QAbstractSocketPrivate::pauseSocketNotifiers(channels[i].socket);
        }
    }
}

void QHttpNetworkConnectionPrivate::resumeConnection()
{
    state = RunningState;
    // Enable all socket notifiers
    for (int i = 0; i < activeChannelCount; i++) {
        if (channels[i].socket) {
#ifndef QT_NO_SSL
            if (encrypt)
                QSslSocketPrivate::resumeSocketNotifiers(static_cast<QSslSocket*>(channels[i].socket));
            else
#endif
                QAbstractSocketPrivate::resumeSocketNotifiers(channels[i].socket);

            // Resume pending upload if needed
            if (channels[i].state == QHttpNetworkConnectionChannel::WritingState)
                QMetaObject::invokeMethod(&channels[i], "_q_uploadDataReadyRead", Qt::QueuedConnection);
        }
    }

    // queue _q_startNextRequest
    QMetaObject::invokeMethod(this->q_func(), "_q_startNextRequest", Qt::QueuedConnection);
}

int QHttpNetworkConnectionPrivate::indexOf(QAbstractSocket *socket) const
{
    for (int i = 0; i < activeChannelCount; ++i)
        if (channels[i].socket == socket)
            return i;

    qFatal("Called with unknown socket object.");
    return 0;
}

// If the connection is in the HostLookupPendening state channel errors should not always be
// emitted. This function will check the status of the connection channels if we
// have not decided the networkLayerState and will return true if the channel error
// should be emitted by the channel.
bool QHttpNetworkConnectionPrivate::shouldEmitChannelError(QAbstractSocket *socket)
{
    Q_Q(QHttpNetworkConnection);

    bool emitError = true;
    int i = indexOf(socket);
    int otherSocket = (i == 0 ? 1 : 0);

    // If the IPv4 connection still isn't started we need to start it now.
    if (delayedConnectionTimer.isActive()) {
        delayedConnectionTimer.stop();
        channels[otherSocket].ensureConnection();
    }

    if (activeChannelCount < channelCount) {
        if (networkLayerState == HostLookupPending || networkLayerState == IPv4or6)
            networkLayerState = QHttpNetworkConnectionPrivate::Unknown;
        channels[0].close();
        emitError = true;
    } else {
        if (networkLayerState == HostLookupPending || networkLayerState == IPv4or6) {
            if (channels[otherSocket].isSocketBusy() && (channels[otherSocket].state != QHttpNetworkConnectionChannel::ClosingState)) {
                // this was the first socket to fail.
                channels[i].close();
                emitError = false;
            }
            else {
                // Both connection attempts has failed.
                networkLayerState = QHttpNetworkConnectionPrivate::Unknown;
                channels[i].close();
                emitError = true;
            }
        } else {
            if (((networkLayerState == QHttpNetworkConnectionPrivate::IPv4) && (channels[i].networkLayerPreference != QAbstractSocket::IPv4Protocol))
                || ((networkLayerState == QHttpNetworkConnectionPrivate::IPv6) && (channels[i].networkLayerPreference != QAbstractSocket::IPv6Protocol))) {
                // First connection worked so this is the second one to complete and it failed.
                channels[i].close();
                QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
                emitError = false;
            }
            if (networkLayerState == QHttpNetworkConnectionPrivate::Unknown)
                qWarning("We got a connection error when networkLayerState is Unknown");
        }
    }
    return emitError;
}


qint64 QHttpNetworkConnectionPrivate::uncompressedBytesAvailable(const QHttpNetworkReply &reply) const
{
    return reply.d_func()->responseData.byteAmount();
}

qint64 QHttpNetworkConnectionPrivate::uncompressedBytesAvailableNextBlock(const QHttpNetworkReply &reply) const
{
    return reply.d_func()->responseData.sizeNextBlock();
}

void QHttpNetworkConnectionPrivate::prepareRequest(HttpMessagePair &messagePair)
{
    QHttpNetworkRequest &request = messagePair.first;
    QHttpNetworkReply *reply = messagePair.second;

    // add missing fields for the request
    QByteArray value;
#ifndef Q_OS_WASM
    // check if Content-Length is provided
    QNonContiguousByteDevice* uploadByteDevice = request.uploadByteDevice();
    if (uploadByteDevice) {
        const qint64 contentLength = request.contentLength();
        const qint64 uploadDeviceSize = uploadByteDevice->size();
        if (contentLength != -1 && uploadDeviceSize != -1) {
            // Both values known: use the smaller one.
            if (uploadDeviceSize < contentLength)
                request.setContentLength(uploadDeviceSize);
        } else if (contentLength == -1 && uploadDeviceSize != -1) {
            // content length not supplied by user, but the upload device knows it
            request.setContentLength(uploadDeviceSize);
        } else if (contentLength != -1 && uploadDeviceSize == -1) {
            // everything OK, the user supplied us the contentLength
        } else if (Q_UNLIKELY(contentLength == -1 && uploadDeviceSize == -1)) {
            qFatal("QHttpNetworkConnectionPrivate: Neither content-length nor upload device size were given");
        }
    }
#endif
    // set the Connection/Proxy-Connection: Keep-Alive headers
#ifndef QT_NO_NETWORKPROXY
    if (networkProxy.type() == QNetworkProxy::HttpCachingProxy)  {
        value = request.headerField("proxy-connection");
        if (value.isEmpty())
            request.setHeaderField("Proxy-Connection", "Keep-Alive");
    } else {
#endif
        value = request.headerField("connection");
        if (value.isEmpty())
            request.setHeaderField("Connection", "Keep-Alive");
#ifndef QT_NO_NETWORKPROXY
    }
#endif

    // If the request had a accept-encoding set, we better not mess
    // with it. If it was not set, we announce that we understand gzip
    // and remember this fact in request.d->autoDecompress so that
    // we can later decompress the HTTP reply if it has such an
    // encoding.
    value = request.headerField("accept-encoding");
    if (value.isEmpty()) {
#ifndef QT_NO_COMPRESS
        const QByteArrayList &acceptedEncoding = QDecompressHelper::acceptedEncoding();
        request.setHeaderField("Accept-Encoding", acceptedEncoding.join(", "));
        request.d->autoDecompress = true;
#else
        // if zlib is not available set this to false always
        request.d->autoDecompress = false;
#endif
    }

    // some websites mandate an accept-language header and fail
    // if it is not sent. This is a problem with the website and
    // not with us, but we work around this by setting
    // one always.
    value = request.headerField("accept-language");
    if (value.isEmpty()) {
        QString systemLocale = QLocale::system().name().replace(QChar::fromLatin1('_'),QChar::fromLatin1('-'));
        QString acceptLanguage;
        if (systemLocale == "C"_L1)
            acceptLanguage = QString::fromLatin1("en,*");
        else if (systemLocale.startsWith("en-"_L1))
            acceptLanguage = systemLocale + ",*"_L1;
        else
            acceptLanguage = systemLocale + ",en,*"_L1;
        request.setHeaderField("Accept-Language", std::move(acceptLanguage).toLatin1());
    }

    // set the User Agent
    value = request.headerField("user-agent");
    if (value.isEmpty())
        request.setHeaderField("User-Agent", "Mozilla/5.0");
    // set the host
    value = request.headerField("host");
    if (value.isEmpty()) {
        QHostAddress add;
        QByteArray host;
        if (add.setAddress(hostName)) {
            if (add.protocol() == QAbstractSocket::IPv6Protocol)
                host = '[' + hostName.toLatin1() + ']'; //format the ipv6 in the standard way
            else
                host = hostName.toLatin1();

        } else {
            host = QUrl::toAce(hostName);
        }

        int port = request.url().port();
        if (port != -1) {
            host += ':';
            host += QByteArray::number(port);
        }

        request.prependHeaderField("Host", host);
    }

    reply->d_func()->requestIsPrepared = true;
}




void QHttpNetworkConnectionPrivate::emitReplyError(QAbstractSocket *socket,
                                                   QHttpNetworkReply *reply,
                                                   QNetworkReply::NetworkError errorCode)
{
    Q_Q(QHttpNetworkConnection);

    int i = 0;
    if (socket)
        i = indexOf(socket);

    if (reply) {
        // this error matters only to this reply
        reply->d_func()->errorString = errorDetail(errorCode, socket);
        emit reply->finishedWithError(errorCode, reply->d_func()->errorString);
        // remove the corrupt data if any
        reply->d_func()->eraseData();

        // Clean the channel
        channels[i].close();
        channels[i].reply = nullptr;
        if (channels[i].protocolHandler)
            channels[i].protocolHandler->setReply(nullptr);
        channels[i].request = QHttpNetworkRequest();
        if (socket)
            channels[i].requeueCurrentlyPipelinedRequests();

        // send the next request
        QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
    }
}

void QHttpNetworkConnectionPrivate::copyCredentials(int fromChannel, QAuthenticator *auth, bool isProxy)
{
    Q_ASSERT(auth);

    // NTLM and Negotiate do multi-phase authentication.
    // Copying credentialsbetween authenticators would mess things up.
    if (fromChannel >= 0) {
        QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(*auth);
        if (priv
            && (priv->method == QAuthenticatorPrivate::Ntlm
                || priv->method == QAuthenticatorPrivate::Negotiate)) {
            return;
        }
    }

    // select another channel
    QAuthenticator* otherAuth = nullptr;
    for (int i = 0; i < activeChannelCount; ++i) {
        if (i == fromChannel)
            continue;
        if (isProxy)
            otherAuth = &channels[i].proxyAuthenticator;
        else
            otherAuth = &channels[i].authenticator;
        // if the credentials are different, copy them
        if (otherAuth->user().compare(auth->user()))
            otherAuth->setUser(auth->user());
        if (otherAuth->password().compare(auth->password()))
            otherAuth->setPassword(auth->password());
    }
}


// handles the authentication for one channel and eventually re-starts the other channels
bool QHttpNetworkConnectionPrivate::handleAuthenticateChallenge(QAbstractSocket *socket, QHttpNetworkReply *reply,
                                                                bool isProxy, bool &resend)
{
    Q_ASSERT(socket);
    Q_ASSERT(reply);

    resend = false;
    //create the response header to be used with QAuthenticatorPrivate.
    QList<QPair<QByteArray, QByteArray> > fields = reply->header();

    // Check that any of the proposed authenticate methods are supported
    const QByteArray header = isProxy ? "proxy-authenticate" : "www-authenticate";
    const QByteArrayList &authenticationMethods = reply->d_func()->headerFieldValues(header);
    const bool isSupported = std::any_of(authenticationMethods.begin(), authenticationMethods.end(),
                                         QAuthenticatorPrivate::isMethodSupported);
    if (isSupported) {
        int i = indexOf(socket);
        //Use a single authenticator for all domains. ### change later to use domain/realm
        QAuthenticator *auth = isProxy ? &channels[i].proxyAuthenticator
                                       : &channels[i].authenticator;
        //proceed with the authentication.
        if (auth->isNull())
            auth->detach();
        QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(*auth);
        priv->parseHttpResponse(fields, isProxy, reply->url().host());
        // Update method in case it changed
        if (priv->method == QAuthenticatorPrivate::None)
            return false;

        if (priv->phase == QAuthenticatorPrivate::Done ||
                (priv->phase == QAuthenticatorPrivate::Start
                    && (priv->method == QAuthenticatorPrivate::Ntlm
                        || priv->method == QAuthenticatorPrivate::Negotiate))) {
            if (priv->phase == QAuthenticatorPrivate::Start)
                priv->phase = QAuthenticatorPrivate::Phase1;

            pauseConnection();
            if (!isProxy) {
                if (channels[i].authenticationCredentialsSent) {
                    auth->detach();
                    priv = QAuthenticatorPrivate::getPrivate(*auth);
                    priv->hasFailed = true;
                    priv->phase = QAuthenticatorPrivate::Done;
                    channels[i].authenticationCredentialsSent = false;
                }
                emit reply->authenticationRequired(reply->request(), auth);
#ifndef QT_NO_NETWORKPROXY
            } else {
                if (channels[i].proxyCredentialsSent) {
                    auth->detach();
                    priv = QAuthenticatorPrivate::getPrivate(*auth);
                    priv->hasFailed = true;
                    priv->phase = QAuthenticatorPrivate::Done;
                    channels[i].proxyCredentialsSent = false;
                }
                emit reply->proxyAuthenticationRequired(networkProxy, auth);
#endif
            }
            resumeConnection();

            if (priv->phase != QAuthenticatorPrivate::Done) {
                // send any pending requests
                copyCredentials(i,  auth, isProxy);
            }
        } else if (priv->phase == QAuthenticatorPrivate::Start) {
            // If the url's authenticator has a 'user' set we will end up here (phase is only set to 'Done' by
            // parseHttpResponse above if 'user' is empty). So if credentials were supplied with the request,
            // such as in the case of an XMLHttpRequest, this is our only opportunity to cache them.
            emit reply->cacheCredentials(reply->request(), auth);
        }
        // - Changing values in QAuthenticator will reset the 'phase'. Therefore if it is still "Done"
        //   then nothing was filled in by the user or the cache
        // - If withCredentials has been set to false (e.g. by Qt WebKit for a cross-origin XMLHttpRequest) then
        //   we need to bail out if authentication is required.
        if (priv->phase == QAuthenticatorPrivate::Done || !reply->request().withCredentials()) {
            // Reset authenticator so the next request on that channel does not get messed up
            auth = nullptr;
            if (isProxy)
                channels[i].proxyAuthenticator = QAuthenticator();
            else
                channels[i].authenticator = QAuthenticator();

            // authentication is cancelled, send the current contents to the user.
            emit reply->headerChanged();
            emit reply->readyRead();
            QNetworkReply::NetworkError errorCode =
                isProxy
                ? QNetworkReply::ProxyAuthenticationRequiredError
                : QNetworkReply::AuthenticationRequiredError;
            reply->d_func()->errorString = errorDetail(errorCode, socket);
            emit reply->finishedWithError(errorCode, reply->d_func()->errorString);
            // ### at this point the reply could be deleted
            return true;
        }
        //resend the request
        resend = true;
        return true;
    }
    return false;
}

// Used by the HTTP1 code-path
QUrl QHttpNetworkConnectionPrivate::parseRedirectResponse(QAbstractSocket *socket,
                                                          QHttpNetworkReply *reply)
{
    ParseRedirectResult result = parseRedirectResponse(reply);
    if (result.errorCode != QNetworkReply::NoError) {
        emitReplyError(socket, reply, result.errorCode);
        return {};
    }
    return std::move(result.redirectUrl);
}

QHttpNetworkConnectionPrivate::ParseRedirectResult
QHttpNetworkConnectionPrivate::parseRedirectResponse(QHttpNetworkReply *reply)
{
    if (!reply->request().isFollowRedirects())
        return {{}, QNetworkReply::NoError};

    QUrl redirectUrl;
    const QList<QPair<QByteArray, QByteArray> > fields = reply->header();
    for (const QNetworkReply::RawHeaderPair &header : fields) {
        if (header.first.compare("location", Qt::CaseInsensitive) == 0) {
            redirectUrl = QUrl::fromEncoded(header.second);
            break;
        }
    }

    // If the location url is invalid/empty, we return ProtocolUnknownError
    if (!redirectUrl.isValid())
        return {{}, QNetworkReply::ProtocolUnknownError};

    // Check if we have exceeded max redirects allowed
    if (reply->request().redirectCount() <= 0)
        return {{}, QNetworkReply::TooManyRedirectsError};

    // Resolve the URL if it's relative
    if (redirectUrl.isRelative())
        redirectUrl = reply->request().url().resolved(redirectUrl);

    // Check redirect url protocol
    const QUrl priorUrl(reply->request().url());
    if (redirectUrl.scheme() == "http"_L1 || redirectUrl.scheme() == "https"_L1) {
        switch (reply->request().redirectPolicy()) {
        case QNetworkRequest::NoLessSafeRedirectPolicy:
            // Here we could handle https->http redirects as InsecureProtocolError.
            // However, if HSTS is enabled and redirectUrl.host() is a known STS
            // host, then we'll replace its scheme and this won't downgrade protocol,
            // after all.  We cannot access QNAM's STS cache from here, so delegate
            // this check to QNetworkReplyHttpImpl.
            break;
        case QNetworkRequest::SameOriginRedirectPolicy:
            if (priorUrl.host() != redirectUrl.host()
                || priorUrl.scheme() != redirectUrl.scheme()
                || priorUrl.port() != redirectUrl.port()) {
                return {{}, QNetworkReply::InsecureRedirectError};
            }
            break;
        case QNetworkRequest::UserVerifiedRedirectPolicy:
            break;
        default:
            Q_ASSERT(!"Unexpected redirect policy");
        }
    } else {
        return {{}, QNetworkReply::ProtocolUnknownError};
    }
    return {std::move(redirectUrl), QNetworkReply::NoError};
}

void QHttpNetworkConnectionPrivate::createAuthorization(QAbstractSocket *socket, QHttpNetworkRequest &request)
{
    Q_ASSERT(socket);

    QHttpNetworkConnectionChannel &channel = channels[indexOf(socket)];

    QAuthenticator *authenticator = &channel.authenticator;
    QAuthenticatorPrivate *priv = QAuthenticatorPrivate::getPrivate(*authenticator);
    // Send "Authorization" header, but not if it's NTLM and the socket is already authenticated.
    if (priv && priv->method != QAuthenticatorPrivate::None) {
        const bool ntlmNego = priv->method == QAuthenticatorPrivate::Ntlm
                || priv->method == QAuthenticatorPrivate::Negotiate;
        const bool authNeeded = channel.lastStatus == 401;
        const bool ntlmNegoOk = ntlmNego && authNeeded
                && (priv->phase != QAuthenticatorPrivate::Done
                    || !channel.authenticationCredentialsSent);
        const bool otherOk =
                !ntlmNego && (authNeeded || request.headerField("Authorization").isEmpty());
        if (ntlmNegoOk || otherOk) {
            QByteArray response = priv->calculateResponse(request.methodName(), request.uri(false),
                                                          request.url().host());
            request.setHeaderField("Authorization", response);
            channel.authenticationCredentialsSent = true;
        }
    }

#if QT_CONFIG(networkproxy)
    authenticator = &channel.proxyAuthenticator;
    priv = QAuthenticatorPrivate::getPrivate(*authenticator);
    // Send "Proxy-Authorization" header, but not if it's NTLM and the socket is already authenticated.
    if (priv && priv->method != QAuthenticatorPrivate::None) {
        const bool ntlmNego = priv->method == QAuthenticatorPrivate::Ntlm
                || priv->method == QAuthenticatorPrivate::Negotiate;
        const bool proxyAuthNeeded = channel.lastStatus == 407;
        const bool ntlmNegoOk = ntlmNego && proxyAuthNeeded
                && (priv->phase != QAuthenticatorPrivate::Done || !channel.proxyCredentialsSent);
        const bool otherOk = !ntlmNego;
        if (ntlmNegoOk || otherOk) {
            QByteArray response = priv->calculateResponse(request.methodName(), request.uri(false),
                                                          networkProxy.hostName());
            request.setHeaderField("Proxy-Authorization", response);
            channel.proxyCredentialsSent = true;
        }
    }
#endif // QT_CONFIG(networkproxy)
}

QHttpNetworkReply* QHttpNetworkConnectionPrivate::queueRequest(const QHttpNetworkRequest &request)
{
    Q_Q(QHttpNetworkConnection);

    // The reply component of the pair is created initially.
    QHttpNetworkReply *reply = new QHttpNetworkReply(request.url());
    reply->setRequest(request);
    reply->d_func()->connection = q;
    reply->d_func()->connectionChannel = &channels[0]; // will have the correct one set later
    HttpMessagePair pair = qMakePair(request, reply);

    if (request.isPreConnect())
        preConnectRequests++;

    if (connectionType == QHttpNetworkConnection::ConnectionTypeHTTP
        || (!encrypt && connectionType == QHttpNetworkConnection::ConnectionTypeHTTP2 && !channels[0].switchedToHttp2)) {
        switch (request.priority()) {
        case QHttpNetworkRequest::HighPriority:
            highPriorityQueue.prepend(pair);
            break;
        case QHttpNetworkRequest::NormalPriority:
        case QHttpNetworkRequest::LowPriority:
            lowPriorityQueue.prepend(pair);
            break;
        }
    }
    else { // HTTP/2 ('h2' mode)
        if (!pair.second->d_func()->requestIsPrepared)
            prepareRequest(pair);
        channels[0].h2RequestsToSend.insert(request.priority(), pair);
    }

    // For Happy Eyeballs the networkLayerState is set to Unknown
    // until we have started the first connection attempt. So no
    // request will be started until we know if IPv4 or IPv6
    // should be used.
    if (networkLayerState == Unknown || networkLayerState == HostLookupPending) {
        startHostInfoLookup();
    } else if ( networkLayerState == IPv4 || networkLayerState == IPv6 ) {
        // this used to be called via invokeMethod and a QueuedConnection
        // It is the only place _q_startNextRequest is called directly without going
        // through the event loop using a QueuedConnection.
        // This is dangerous because of recursion that might occur when emitting
        // signals as DirectConnection from this code path. Therefore all signal
        // emissions that can come out from this code path need to
        // be QueuedConnection.
        // We are currently trying to fine-tune this.
        _q_startNextRequest();
    }
    return reply;
}

void QHttpNetworkConnectionPrivate::fillHttp2Queue()
{
    for (auto &pair : highPriorityQueue) {
        if (!pair.second->d_func()->requestIsPrepared)
            prepareRequest(pair);
        channels[0].h2RequestsToSend.insert(QHttpNetworkRequest::HighPriority, pair);
    }

    highPriorityQueue.clear();

    for (auto &pair : lowPriorityQueue) {
        if (!pair.second->d_func()->requestIsPrepared)
            prepareRequest(pair);
        channels[0].h2RequestsToSend.insert(pair.first.priority(), pair);
    }

    lowPriorityQueue.clear();
}

void QHttpNetworkConnectionPrivate::requeueRequest(const HttpMessagePair &pair)
{
    Q_Q(QHttpNetworkConnection);

    QHttpNetworkRequest request = pair.first;
    switch (request.priority()) {
    case QHttpNetworkRequest::HighPriority:
        highPriorityQueue.prepend(pair);
        break;
    case QHttpNetworkRequest::NormalPriority:
    case QHttpNetworkRequest::LowPriority:
        lowPriorityQueue.prepend(pair);
        break;
    }

    QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
}

bool QHttpNetworkConnectionPrivate::dequeueRequest(QAbstractSocket *socket)
{
    int i = 0;
    if (socket)
        i = indexOf(socket);

    if (!highPriorityQueue.isEmpty()) {
        // remove from queue before sendRequest! else we might pipeline the same request again
        HttpMessagePair messagePair = highPriorityQueue.takeLast();
        if (!messagePair.second->d_func()->requestIsPrepared)
            prepareRequest(messagePair);
        updateChannel(i, messagePair);
        return true;
    }

    if (!lowPriorityQueue.isEmpty()) {
        // remove from queue before sendRequest! else we might pipeline the same request again
        HttpMessagePair messagePair = lowPriorityQueue.takeLast();
        if (!messagePair.second->d_func()->requestIsPrepared)
            prepareRequest(messagePair);
        updateChannel(i, messagePair);
        return true;
    }
    return false;
}

void QHttpNetworkConnectionPrivate::updateChannel(int i, const HttpMessagePair &messagePair)
{
    channels[i].request = messagePair.first;
    channels[i].reply = messagePair.second;
    // Now that reply is assigned a channel, correct reply to channel association
    // previously set in queueRequest.
    channels[i].reply->d_func()->connectionChannel = &channels[i];
}

QHttpNetworkRequest QHttpNetworkConnectionPrivate::predictNextRequest() const
{
    if (!highPriorityQueue.isEmpty())
        return highPriorityQueue.last().first;
    if (!lowPriorityQueue.isEmpty())
        return lowPriorityQueue.last().first;
    return QHttpNetworkRequest();
}

QHttpNetworkReply* QHttpNetworkConnectionPrivate::predictNextRequestsReply() const
{
    if (!highPriorityQueue.isEmpty())
        return highPriorityQueue.last().second;
    if (!lowPriorityQueue.isEmpty())
        return lowPriorityQueue.last().second;
    return nullptr;
}

// this is called from _q_startNextRequest and when a request has been sent down a socket from the channel
void QHttpNetworkConnectionPrivate::fillPipeline(QAbstractSocket *socket)
{
    // return fast if there is nothing to pipeline
    if (highPriorityQueue.isEmpty() && lowPriorityQueue.isEmpty())
        return;

    int i = indexOf(socket);

    // return fast if there was no reply right now processed
    if (channels[i].reply == nullptr)
        return;

    if (! (defaultPipelineLength - channels[i].alreadyPipelinedRequests.size() >= defaultRePipelineLength)) {
        return;
    }

    if (channels[i].pipeliningSupported != QHttpNetworkConnectionChannel::PipeliningProbablySupported)
        return;

    // the current request that is in must already support pipelining
    if (!channels[i].request.isPipeliningAllowed())
        return;

    // the current request must be a idempotent (right now we only check GET)
    if (channels[i].request.operation() != QHttpNetworkRequest::Get)
        return;

    // check if socket is connected
    if (socket->state() != QAbstractSocket::ConnectedState)
        return;

    // check for resendCurrent
    if (channels[i].resendCurrent)
        return;

    // we do not like authentication stuff
    // ### make sure to be OK with this in later releases
    if (!channels[i].authenticator.isNull()
        && (!channels[i].authenticator.user().isEmpty()
            || !channels[i].authenticator.password().isEmpty()))
        return;
    if (!channels[i].proxyAuthenticator.isNull()
        && (!channels[i].proxyAuthenticator.user().isEmpty()
            || !channels[i].proxyAuthenticator.password().isEmpty()))
        return;

    // must be in ReadingState or WaitingState
    if (! (channels[i].state == QHttpNetworkConnectionChannel::WaitingState
           || channels[i].state == QHttpNetworkConnectionChannel::ReadingState))
        return;

    int lengthBefore;
    while (!highPriorityQueue.isEmpty()) {
        lengthBefore = channels[i].alreadyPipelinedRequests.size();
        fillPipeline(highPriorityQueue, channels[i]);

        if (channels[i].alreadyPipelinedRequests.size() >= defaultPipelineLength) {
            channels[i].pipelineFlush();
            return;
        }

        if (lengthBefore == channels[i].alreadyPipelinedRequests.size())
            break; // did not process anything, now do the low prio queue
    }

    while (!lowPriorityQueue.isEmpty()) {
        lengthBefore = channels[i].alreadyPipelinedRequests.size();
        fillPipeline(lowPriorityQueue, channels[i]);

        if (channels[i].alreadyPipelinedRequests.size() >= defaultPipelineLength) {
            channels[i].pipelineFlush();
            return;
        }

        if (lengthBefore == channels[i].alreadyPipelinedRequests.size())
            break; // did not process anything
    }


    channels[i].pipelineFlush();
}

// returns true when the processing of a queue has been done
bool QHttpNetworkConnectionPrivate::fillPipeline(QList<HttpMessagePair> &queue, QHttpNetworkConnectionChannel &channel)
{
    if (queue.isEmpty())
        return true;

    for (int i = queue.size() - 1; i >= 0; --i) {
        HttpMessagePair messagePair = queue.at(i);
        const QHttpNetworkRequest &request = messagePair.first;

        // we currently do not support pipelining if HTTP authentication is used
        if (!request.url().userInfo().isEmpty())
            continue;

        // take only GET requests
        if (request.operation() != QHttpNetworkRequest::Get)
            continue;

        if (!request.isPipeliningAllowed())
            continue;

        // remove it from the queue
        queue.takeAt(i);
        // we modify the queue we iterate over here, but since we return from the function
        // afterwards this is fine.

        // actually send it
        if (!messagePair.second->d_func()->requestIsPrepared)
            prepareRequest(messagePair);
        channel.pipelineInto(messagePair);

        // return false because we processed something and need to process again
        return false;
    }

    // return true, the queue has been processed and not changed
    return true;
}


QString QHttpNetworkConnectionPrivate::errorDetail(QNetworkReply::NetworkError errorCode, QAbstractSocket *socket, const QString &extraDetail)
{
    QString errorString;
    switch (errorCode) {
    case QNetworkReply::HostNotFoundError:
        if (socket)
            errorString = QCoreApplication::translate("QHttp", "Host %1 not found").arg(socket->peerName());
        else
            errorString = QCoreApplication::translate("QHttp", "Host %1 not found").arg(hostName);
        break;
    case QNetworkReply::ConnectionRefusedError:
        errorString = QCoreApplication::translate("QHttp", "Connection refused");
        break;
    case QNetworkReply::RemoteHostClosedError:
        errorString = QCoreApplication::translate("QHttp", "Connection closed");
        break;
    case QNetworkReply::TimeoutError:
        errorString = QCoreApplication::translate("QAbstractSocket", "Socket operation timed out");
        break;
    case QNetworkReply::ProxyAuthenticationRequiredError:
        errorString = QCoreApplication::translate("QHttp", "Proxy requires authentication");
        break;
    case QNetworkReply::AuthenticationRequiredError:
        errorString = QCoreApplication::translate("QHttp", "Host requires authentication");
        break;
    case QNetworkReply::ProtocolFailure:
        errorString = QCoreApplication::translate("QHttp", "Data corrupted");
        break;
    case QNetworkReply::ProtocolUnknownError:
        errorString = QCoreApplication::translate("QHttp", "Unknown protocol specified");
        break;
    case QNetworkReply::SslHandshakeFailedError:
        errorString = QCoreApplication::translate("QHttp", "SSL handshake failed");
        if (socket)
            errorString += QStringLiteral(": ") + socket->errorString();
        break;
    case QNetworkReply::TooManyRedirectsError:
        errorString = QCoreApplication::translate("QHttp", "Too many redirects");
        break;
    case QNetworkReply::InsecureRedirectError:
        errorString = QCoreApplication::translate("QHttp", "Insecure redirect");
        break;
    default:
        // all other errors are treated as QNetworkReply::UnknownNetworkError
        errorString = extraDetail;
        break;
    }
    return errorString;
}

// this is called from the destructor of QHttpNetworkReply. It is called when
// the reply was finished correctly or when it was aborted.
void QHttpNetworkConnectionPrivate::removeReply(QHttpNetworkReply *reply)
{
    Q_Q(QHttpNetworkConnection);

    // check if the reply is currently being processed or it is pipelined in
    for (int i = 0; i < activeChannelCount; ++i) {
        // is the reply associated the currently processing of this channel?
        if (channels[i].reply == reply) {
            channels[i].reply = nullptr;
            if (channels[i].protocolHandler)
                channels[i].protocolHandler->setReply(nullptr);
            channels[i].request = QHttpNetworkRequest();
            channels[i].resendCurrent = false;

            if (!reply->isFinished() && !channels[i].alreadyPipelinedRequests.isEmpty()) {
                // the reply had to be prematurely removed, e.g. it was not finished
                // therefore we have to requeue the already pipelined requests.
                channels[i].requeueCurrentlyPipelinedRequests();
            }

            // if HTTP mandates we should close
            // or the reply is not finished yet, e.g. it was aborted
            // we have to close that connection
            if (reply->d_func()->isConnectionCloseEnabled() || !reply->isFinished()) {
                if (reply->isAborted()) {
                    channels[i].abort();
                } else {
                    channels[i].close();
                }
            }

            QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
            return;
        }

        // is the reply inside the pipeline of this channel already?
        for (int j = 0; j < channels[i].alreadyPipelinedRequests.size(); j++) {
            if (channels[i].alreadyPipelinedRequests.at(j).second == reply) {
               // Remove that HttpMessagePair
               channels[i].alreadyPipelinedRequests.removeAt(j);

               channels[i].requeueCurrentlyPipelinedRequests();

               // Since some requests had already been pipelined, but we removed
               // one and re-queued the others
               // we must force a connection close after the request that is
               // currently in processing has been finished.
               if (channels[i].reply)
                   channels[i].reply->d_func()->forceConnectionCloseEnabled = true;

               QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
               return;
            }
        }
        // is the reply inside the H2 pipeline of this channel already?
        const auto foundReply = [reply](const HttpMessagePair &pair) {
            return pair.second == reply;
        };
        auto &seq = channels[i].h2RequestsToSend;
        const auto end = seq.cend();
        auto it = std::find_if(seq.cbegin(), end, foundReply);
        if (it != end) {
            seq.erase(it);
            QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
            return;
        }
    }
    // remove from the high priority queue
    if (!highPriorityQueue.isEmpty()) {
        for (int j = highPriorityQueue.size() - 1; j >= 0; --j) {
            HttpMessagePair messagePair = highPriorityQueue.at(j);
            if (messagePair.second == reply) {
                highPriorityQueue.removeAt(j);
                QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
                return;
            }
        }
    }
    // remove from the low priority queue
    if (!lowPriorityQueue.isEmpty()) {
        for (int j = lowPriorityQueue.size() - 1; j >= 0; --j) {
            HttpMessagePair messagePair = lowPriorityQueue.at(j);
            if (messagePair.second == reply) {
                lowPriorityQueue.removeAt(j);
                QMetaObject::invokeMethod(q, "_q_startNextRequest", Qt::QueuedConnection);
                return;
            }
        }
    }
}



// This function must be called from the event loop. The only
// exception is documented in QHttpNetworkConnectionPrivate::queueRequest
// although it is called _q_startNextRequest, it will actually start multiple requests when possible
void QHttpNetworkConnectionPrivate::_q_startNextRequest()
{
    // If there is no network layer state decided we should not start any new requests.
    if (networkLayerState == Unknown || networkLayerState == HostLookupPending || networkLayerState == IPv4or6)
        return;

    // If the QHttpNetworkConnection is currently paused then bail out immediately
    if (state == PausedState)
        return;

    //resend the necessary ones.
    for (int i = 0; i < activeChannelCount; ++i) {
        if (channels[i].resendCurrent && (channels[i].state != QHttpNetworkConnectionChannel::ClosingState)) {
            if (!channels[i].socket
                || channels[i].socket->state() == QAbstractSocket::UnconnectedState) {
                if (!channels[i].ensureConnection())
                    continue;
            }
            channels[i].resendCurrent = false;

            // if this is not possible, error will be emitted and connection terminated
            if (!channels[i].resetUploadData())
                continue;
            channels[i].sendRequest();
        }
    }

    // dequeue new ones

    switch (connectionType) {
    case QHttpNetworkConnection::ConnectionTypeHTTP: {
        // return fast if there is nothing to do
        if (highPriorityQueue.isEmpty() && lowPriorityQueue.isEmpty())
            return;

        // try to get a free AND connected socket
        for (int i = 0; i < activeChannelCount; ++i) {
            if (channels[i].socket) {
                if (!channels[i].reply && !channels[i].isSocketBusy() && channels[i].socket->state() == QAbstractSocket::ConnectedState) {
                    if (dequeueRequest(channels[i].socket))
                        channels[i].sendRequest();
                }
            }
        }
        break;
    }
    case QHttpNetworkConnection::ConnectionTypeHTTP2Direct:
    case QHttpNetworkConnection::ConnectionTypeHTTP2: {
        if (channels[0].h2RequestsToSend.isEmpty() && !channels[0].reply
            && highPriorityQueue.isEmpty() && lowPriorityQueue.isEmpty()) {
            return;
        }

        if (networkLayerState == IPv4)
            channels[0].networkLayerPreference = QAbstractSocket::IPv4Protocol;
        else if (networkLayerState == IPv6)
            channels[0].networkLayerPreference = QAbstractSocket::IPv6Protocol;
        channels[0].ensureConnection();
        if (channels[0].socket && channels[0].socket->state() == QAbstractSocket::ConnectedState
            && !channels[0].pendingEncrypt) {
            if (channels[0].h2RequestsToSend.size()) {
                channels[0].sendRequest();
            } else if (!channels[0].reply && !channels[0].switchedToHttp2) {
                // This covers an edge-case where we're already connected and the "connected"
                // signal was already sent, but we didn't have any request available at the time,
                // so it was missed. As such we need to dequeue a request and send it now that we
                // have one.
                dequeueRequest(channels[0].socket);
                channels[0].sendRequest();
            }
        }
        break;
    }
    }

    // try to push more into all sockets
    // ### FIXME we should move this to the beginning of the function
    // as soon as QtWebkit is properly using the pipelining
    // (e.g. not for XMLHttpRequest or the first page load)
    // ### FIXME we should also divide the requests more even
    // on the connected sockets
    //tryToFillPipeline(socket);
    // return fast if there is nothing to pipeline
    if (highPriorityQueue.isEmpty() && lowPriorityQueue.isEmpty())
        return;
    for (int i = 0; i < activeChannelCount; i++)
        if (channels[i].socket && channels[i].socket->state() == QAbstractSocket::ConnectedState)
            fillPipeline(channels[i].socket);

    // If there is not already any connected channels we need to connect a new one.
    // We do not pair the channel with the request until we know if it is
    // connected or not. This is to reuse connected channels before we connect new once.
    int queuedRequests = highPriorityQueue.size() + lowPriorityQueue.size();

    // in case we have in-flight preconnect requests and normal requests,
    // we only need one socket for each (preconnect, normal request) pair
    int neededOpenChannels = queuedRequests;
    if (preConnectRequests > 0) {
        int normalRequests = queuedRequests - preConnectRequests;
        neededOpenChannels = qMax(normalRequests, preConnectRequests);
    }

    if (neededOpenChannels <= 0)
        return;

    QQueue<int> channelsToConnect;

    // use previously used channels first
    for (int i = 0; i < activeChannelCount && neededOpenChannels > 0; ++i) {
        if (!channels[i].socket)
            continue;

        if ((channels[i].socket->state() == QAbstractSocket::ConnectingState)
            || (channels[i].socket->state() == QAbstractSocket::HostLookupState)
            || channels[i].pendingEncrypt) { // pendingEncrypt == "EncryptingState"
            neededOpenChannels--;
            continue;
        }

        if (!channels[i].reply && !channels[i].isSocketBusy()
            && (channels[i].socket->state() == QAbstractSocket::UnconnectedState)) {
            channelsToConnect.enqueue(i);
            neededOpenChannels--;
        }
    }

    // use other channels
    for (int i = 0; i < activeChannelCount && neededOpenChannels > 0; ++i) {
        if (channels[i].socket)
            continue;

        channelsToConnect.enqueue(i);
        neededOpenChannels--;
    }

    while (!channelsToConnect.isEmpty()) {
        const int channel = channelsToConnect.dequeue();

        if (networkLayerState == IPv4)
            channels[channel].networkLayerPreference = QAbstractSocket::IPv4Protocol;
        else if (networkLayerState == IPv6)
            channels[channel].networkLayerPreference = QAbstractSocket::IPv6Protocol;

        channels[channel].ensureConnection();
    }
}


void QHttpNetworkConnectionPrivate::readMoreLater(QHttpNetworkReply *reply)
{
    for (int i = 0 ; i < activeChannelCount; ++i) {
        if (channels[i].reply ==  reply) {
            // emulate a readyRead() from the socket
            QMetaObject::invokeMethod(&channels[i], "_q_readyRead", Qt::QueuedConnection);
            return;
        }
    }
}



// The first time we start the connection is used we do not know if we
// should use IPv4 or IPv6. So we start a hostlookup to figure this out.
// Later when we do the connection the socket will not need to do another
// lookup as then the hostinfo will already be in the cache.
void QHttpNetworkConnectionPrivate::startHostInfoLookup()
{
    networkLayerState = HostLookupPending;

    // check if we already now can decide if this is IPv4 or IPv6
    QString lookupHost = hostName;
#ifndef QT_NO_NETWORKPROXY
    if (networkProxy.capabilities() & QNetworkProxy::HostNameLookupCapability) {
        lookupHost = networkProxy.hostName();
    } else if (channels[0].proxy.capabilities() & QNetworkProxy::HostNameLookupCapability) {
        lookupHost = channels[0].proxy.hostName();
    }
#endif
    QHostAddress temp;
    if (temp.setAddress(lookupHost)) {
        const QAbstractSocket::NetworkLayerProtocol protocol = temp.protocol();
        if (protocol == QAbstractSocket::IPv4Protocol) {
            networkLayerState = QHttpNetworkConnectionPrivate::IPv4;
            QMetaObject::invokeMethod(this->q_func(), "_q_startNextRequest", Qt::QueuedConnection);
            return;
        } else if (protocol == QAbstractSocket::IPv6Protocol) {
            networkLayerState = QHttpNetworkConnectionPrivate::IPv6;
            QMetaObject::invokeMethod(this->q_func(), "_q_startNextRequest", Qt::QueuedConnection);
            return;
        }
    } else {
        int hostLookupId;
        bool immediateResultValid = false;
        QHostInfo hostInfo = qt_qhostinfo_lookup(lookupHost,
                                                 this->q_func(),
                                                 SLOT(_q_hostLookupFinished(QHostInfo)),
                                                 &immediateResultValid,
                                                 &hostLookupId);
        if (immediateResultValid) {
            _q_hostLookupFinished(hostInfo);
        }
    }
}


void QHttpNetworkConnectionPrivate::_q_hostLookupFinished(const QHostInfo &info)
{
    bool bIpv4 = false;
    bool bIpv6 = false;
    bool foundAddress = false;
    if (networkLayerState == IPv4 || networkLayerState == IPv6 || networkLayerState == IPv4or6)
        return;

    const auto addresses = info.addresses();
    for (const QHostAddress &address : addresses) {
        const QAbstractSocket::NetworkLayerProtocol protocol = address.protocol();
        if (protocol == QAbstractSocket::IPv4Protocol) {
            if (!foundAddress) {
                foundAddress = true;
                delayIpv4 = false;
            }
            bIpv4 = true;
        } else if (protocol == QAbstractSocket::IPv6Protocol) {
            if (!foundAddress) {
                foundAddress = true;
                delayIpv4 = true;
            }
            bIpv6 = true;
        }
    }

    if (bIpv4 && bIpv6)
        startNetworkLayerStateLookup();
    else if (bIpv4) {
        networkLayerState = QHttpNetworkConnectionPrivate::IPv4;
        QMetaObject::invokeMethod(this->q_func(), "_q_startNextRequest", Qt::QueuedConnection);
    } else if (bIpv6) {
        networkLayerState = QHttpNetworkConnectionPrivate::IPv6;
        QMetaObject::invokeMethod(this->q_func(), "_q_startNextRequest", Qt::QueuedConnection);
    } else {
        auto lookupError = QNetworkReply::HostNotFoundError;
#ifndef QT_NO_NETWORKPROXY
        // if the proxy can lookup hostnames, all hostname lookups except for the lookup of the
        // proxy hostname are delegated to the proxy.
        auto proxyCapabilities = networkProxy.capabilities() | channels[0].proxy.capabilities();
        if (proxyCapabilities & QNetworkProxy::HostNameLookupCapability)
            lookupError = QNetworkReply::ProxyNotFoundError;
#endif
        if (dequeueRequest(channels[0].socket)) {
            emitReplyError(channels[0].socket, channels[0].reply, lookupError);
            networkLayerState = QHttpNetworkConnectionPrivate::Unknown;
        } else if (connectionType == QHttpNetworkConnection::ConnectionTypeHTTP2
                   || connectionType == QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
            for (const HttpMessagePair &h2Pair : std::as_const(channels[0].h2RequestsToSend)) {
                // emit error for all replies
                QHttpNetworkReply *currentReply = h2Pair.second;
                Q_ASSERT(currentReply);
                emitReplyError(channels[0].socket, currentReply, lookupError);
            }
        } else {
            // We can end up here if a request has been aborted or otherwise failed (e.g. timeout)
            // before the host lookup was finished.
            qDebug("QHttpNetworkConnectionPrivate::_q_hostLookupFinished"
                   " could not de-queue request, failed to report HostNotFoundError");
            networkLayerState = QHttpNetworkConnectionPrivate::Unknown;
        }
    }
}


// This will be used if the host lookup found both and Ipv4 and
// Ipv6 address. Then we will start up two connections and pick
// the network layer of the one that finish first. The second
// connection will then be disconnected.
void QHttpNetworkConnectionPrivate::startNetworkLayerStateLookup()
{
    if (activeChannelCount > 1) {
        // At this time all channels should be unconnected.
        Q_ASSERT(!channels[0].isSocketBusy());
        Q_ASSERT(!channels[1].isSocketBusy());

        networkLayerState = IPv4or6;

        channels[0].networkLayerPreference = QAbstractSocket::IPv4Protocol;
        channels[1].networkLayerPreference = QAbstractSocket::IPv6Protocol;

        int timeout = 300;
        delayedConnectionTimer.start(timeout);
        if (delayIpv4)
            channels[1].ensureConnection();
        else
            channels[0].ensureConnection();
    } else {
        networkLayerState = IPv4or6;
        channels[0].networkLayerPreference = QAbstractSocket::AnyIPProtocol;
        channels[0].ensureConnection();
    }
}

void QHttpNetworkConnectionPrivate::networkLayerDetected(QAbstractSocket::NetworkLayerProtocol protocol)
{
    for (int i = 0 ; i < activeChannelCount; ++i) {
        if ((channels[i].networkLayerPreference != protocol) && (channels[i].state == QHttpNetworkConnectionChannel::ConnectingState)) {
            channels[i].close();
        }
    }
}

void QHttpNetworkConnectionPrivate::_q_connectDelayedChannel()
{
    if (delayIpv4)
        channels[0].ensureConnection();
    else
        channels[1].ensureConnection();
}

QHttpNetworkConnection::QHttpNetworkConnection(const QString &hostName, quint16 port, bool encrypt,
                                               QHttpNetworkConnection::ConnectionType connectionType, QObject *parent)
    : QObject(*(new QHttpNetworkConnectionPrivate(hostName, port, encrypt , connectionType)), parent)
{
    Q_D(QHttpNetworkConnection);
    d->init();
    if (QNetworkConnectionMonitor::isEnabled()) {
        connect(&d->connectionMonitor, &QNetworkConnectionMonitor::reachabilityChanged,
                this, &QHttpNetworkConnection::onlineStateChanged, Qt::QueuedConnection);
    }
}

QHttpNetworkConnection::QHttpNetworkConnection(quint16 connectionCount, const QString &hostName,
                                               quint16 port, bool encrypt, QObject *parent,
                                               QHttpNetworkConnection::ConnectionType connectionType)
     : QObject(*(new QHttpNetworkConnectionPrivate(connectionCount, hostName, port, encrypt,
                                                   connectionType)), parent)
{
    Q_D(QHttpNetworkConnection);
    d->init();
    if (QNetworkConnectionMonitor::isEnabled()) {
        connect(&d->connectionMonitor, &QNetworkConnectionMonitor::reachabilityChanged,
                this, &QHttpNetworkConnection::onlineStateChanged, Qt::QueuedConnection);
    }
}

QHttpNetworkConnection::~QHttpNetworkConnection()
{
}

QString QHttpNetworkConnection::hostName() const
{
    Q_D(const QHttpNetworkConnection);
    return d->hostName;
}

quint16 QHttpNetworkConnection::port() const
{
    Q_D(const QHttpNetworkConnection);
    return d->port;
}

QHttpNetworkReply* QHttpNetworkConnection::sendRequest(const QHttpNetworkRequest &request)
{
    Q_D(QHttpNetworkConnection);
    return d->queueRequest(request);
}

void QHttpNetworkConnection::fillHttp2Queue()
{
    Q_D(QHttpNetworkConnection);
    d->fillHttp2Queue();
}

bool QHttpNetworkConnection::isSsl() const
{
    Q_D(const QHttpNetworkConnection);
    return d->encrypt;
}

QHttpNetworkConnectionChannel *QHttpNetworkConnection::channels() const
{
    return d_func()->channels;
}

#ifndef QT_NO_NETWORKPROXY
void QHttpNetworkConnection::setCacheProxy(const QNetworkProxy &networkProxy)
{
    Q_D(QHttpNetworkConnection);
    d->networkProxy = networkProxy;
    // update the authenticator
    if (!d->networkProxy.user().isEmpty()) {
        for (int i = 0; i < d->channelCount; ++i) {
            d->channels[i].proxyAuthenticator.setUser(d->networkProxy.user());
            d->channels[i].proxyAuthenticator.setPassword(d->networkProxy.password());
        }
    }
}

QNetworkProxy QHttpNetworkConnection::cacheProxy() const
{
    Q_D(const QHttpNetworkConnection);
    return d->networkProxy;
}

void QHttpNetworkConnection::setTransparentProxy(const QNetworkProxy &networkProxy)
{
    Q_D(QHttpNetworkConnection);
    for (int i = 0; i < d->channelCount; ++i)
        d->channels[i].setProxy(networkProxy);
}

QNetworkProxy QHttpNetworkConnection::transparentProxy() const
{
    Q_D(const QHttpNetworkConnection);
    return d->channels[0].proxy;
}
#endif

QHttpNetworkConnection::ConnectionType QHttpNetworkConnection::connectionType()
{
    Q_D(QHttpNetworkConnection);
    return d->connectionType;
}

void QHttpNetworkConnection::setConnectionType(ConnectionType type)
{
    Q_D(QHttpNetworkConnection);
    d->connectionType = type;
}

QHttp2Configuration QHttpNetworkConnection::http2Parameters() const
{
    Q_D(const QHttpNetworkConnection);
    return d->http2Parameters;
}

void QHttpNetworkConnection::setHttp2Parameters(const QHttp2Configuration &params)
{
    Q_D(QHttpNetworkConnection);
    d->http2Parameters = params;
}

// SSL support below
#ifndef QT_NO_SSL
void QHttpNetworkConnection::setSslConfiguration(const QSslConfiguration &config)
{
    Q_D(QHttpNetworkConnection);
    if (!d->encrypt)
        return;

    // set the config on all channels
    for (int i = 0; i < d->activeChannelCount; ++i)
        d->channels[i].setSslConfiguration(config);
}

std::shared_ptr<QSslContext> QHttpNetworkConnection::sslContext()
{
    Q_D(QHttpNetworkConnection);
    return d->sslContext;
}

void QHttpNetworkConnection::setSslContext(std::shared_ptr<QSslContext> context)
{
    Q_D(QHttpNetworkConnection);
    d->sslContext = std::move(context);
}

void QHttpNetworkConnection::ignoreSslErrors(int channel)
{
    Q_D(QHttpNetworkConnection);
    if (!d->encrypt)
        return;

    if (channel == -1) { // ignore for all channels
        // We need to ignore for all channels, even the ones that are not in use just in case they
        // will be in the future.
        for (int i = 0; i < d->channelCount; ++i) {
            d->channels[i].ignoreSslErrors();
        }

    } else {
        d->channels[channel].ignoreSslErrors();
    }
}

void QHttpNetworkConnection::ignoreSslErrors(const QList<QSslError> &errors, int channel)
{
    Q_D(QHttpNetworkConnection);
    if (!d->encrypt)
        return;

    if (channel == -1) { // ignore for all channels
        // We need to ignore for all channels, even the ones that are not in use just in case they
        // will be in the future.
        for (int i = 0; i < d->channelCount; ++i) {
            d->channels[i].ignoreSslErrors(errors);
        }

    } else {
        d->channels[channel].ignoreSslErrors(errors);
    }
}

#endif //QT_NO_SSL

void QHttpNetworkConnection::preConnectFinished()
{
    d_func()->preConnectRequests--;
}

QString QHttpNetworkConnection::peerVerifyName() const
{
    Q_D(const QHttpNetworkConnection);
    return d->peerVerifyName;
}

void QHttpNetworkConnection::setPeerVerifyName(const QString &peerName)
{
    Q_D(QHttpNetworkConnection);
    d->peerVerifyName = peerName;
}

void QHttpNetworkConnection::onlineStateChanged(bool isOnline)
{
    Q_D(QHttpNetworkConnection);

    if (isOnline) {
        // If we did not have any 'isOffline' previously - well, good
        // to know, we are 'online' apparently.
        return;
    }

    for (int i = 0; i < d->activeChannelCount; i++) {
        auto &channel = d->channels[i];
        channel.emitFinishedWithError(QNetworkReply::TemporaryNetworkFailureError, "Temporary network failure.");
        channel.close();
    }

    // We don't care, this connection is broken from our POV.
    d->connectionMonitor.stopMonitoring();
}

#ifndef QT_NO_NETWORKPROXY
// only called from QHttpNetworkConnectionChannel::_q_proxyAuthenticationRequired, not
// from QHttpNetworkConnectionChannel::handleAuthenticationChallenge
// e.g. it is for SOCKS proxies which require authentication.
void QHttpNetworkConnectionPrivate::emitProxyAuthenticationRequired(const QHttpNetworkConnectionChannel *chan, const QNetworkProxy &proxy, QAuthenticator* auth)
{
    // Also pause the connection because socket notifiers may fire while an user
    // dialog is displaying
    pauseConnection();
    QHttpNetworkReply *reply;
    if ((connectionType == QHttpNetworkConnection::ConnectionTypeHTTP2
         && (chan->switchedToHttp2 || chan->h2RequestsToSend.size() > 0))
        || connectionType == QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
        // we choose the reply to emit the proxyAuth signal from somewhat arbitrarily,
        // but that does not matter because the signal will ultimately be emitted
        // by the QNetworkAccessManager.
        Q_ASSERT(chan->h2RequestsToSend.size() > 0);
        reply = chan->h2RequestsToSend.cbegin().value().second;
    } else { // HTTP
        reply = chan->reply;
    }

    Q_ASSERT(reply);
    emit reply->proxyAuthenticationRequired(proxy, auth);
    resumeConnection();
    int i = indexOf(chan->socket);
    copyCredentials(i, auth, true);
}
#endif


QT_END_NAMESPACE

#include "moc_qhttpnetworkconnection_p.cpp"
