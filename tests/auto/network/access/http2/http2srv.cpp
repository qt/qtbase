/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <QtNetwork/private/http2protocol_p.h>
#include <QtNetwork/private/bitstreams_p.h>

#include "http2srv.h"

#ifndef QT_NO_SSL
#include <QtNetwork/qsslconfiguration.h>
#include <QtNetwork/qsslsocket.h>
#include <QtNetwork/qsslkey.h>
#endif

#include <QtNetwork/qtcpsocket.h>

#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <QtCore/qfile.h>

#include <cstdlib>
#include <cstring>
#include <limits>

QT_BEGIN_NAMESPACE

using namespace Http2;
using namespace HPack;

namespace
{

inline bool is_valid_client_stream(quint32 streamID)
{
    // A valid client stream ID is an odd integer number in the range [1, INT_MAX].
    return (streamID & 0x1) && streamID <= quint32(std::numeric_limits<qint32>::max());
}

void fill_push_header(const HttpHeader &originalRequest, HttpHeader &promisedRequest)
{
    for (const auto &field : originalRequest) {
        if (field.name == QByteArray(":authority") ||
            field.name == QByteArray(":scheme")) {
            promisedRequest.push_back(field);
        }
    }
}

}

Http2Server::Http2Server(H2Type type, const RawSettings &ss, const RawSettings &cs)
    : connectionType(type),
      serverSettings(ss),
      expectedClientSettings(cs)
{
#if !QT_CONFIG(ssl)
    Q_ASSERT(type != H2Type::h2Alpn && type != H2Type::h2Direct);
#endif

    responseBody = "<html>\n"
                   "<head>\n"
                   "<title>Sample \"Hello, World\" Application</title>\n"
                   "</head>\n"
                   "<body bgcolor=white>\n"
                   "<table border=\"0\" cellpadding=\"10\">\n"
                   "<tr>\n"
                   "<td>\n"
                   "<img src=\"images/springsource.png\">\n"
                   "</td>\n"
                   "<td>\n"
                   "<h1>Sample \"Hello, World\" Application</h1>\n"
                   "</td>\n"
                   "</tr>\n"
                   "</table>\n"
                   "<p>This is the home page for the HelloWorld Web application. </p>\n"
                   "</body>\n"
                   "</html>";
}

Http2Server::~Http2Server()
{
}

void Http2Server::enablePushPromise(bool pushEnabled, const QByteArray &path)
{
    pushPromiseEnabled = pushEnabled;
    pushPath = path;
}

void Http2Server::setResponseBody(const QByteArray &body)
{
    responseBody = body;
}

void Http2Server::emulateGOAWAY(int timeout)
{
    Q_ASSERT(timeout >= 0);
    testingGOAWAY = true;
    goawayTimeout = timeout;
}

void Http2Server::redirectOpenStream(quint16 port)
{
    redirectWhileReading = true;
    targetPort = port;
}

bool Http2Server::isClearText() const
{
    return connectionType == H2Type::h2c || connectionType == H2Type::h2cDirect;
}

void Http2Server::startServer()
{
    if (listen()) {
        if (isClearText())
            authority = QStringLiteral("127.0.0.1:%1").arg(serverPort()).toLatin1();
        emit serverStarted(serverPort());
    }
}

bool Http2Server::sendProtocolSwitchReply()
{
    Q_ASSERT(socket);
    Q_ASSERT(connectionType == H2Type::h2c);
    // The first and the last HTTP/1.1 response we send:
    const char response[] = "HTTP/1.1 101 Switching Protocols\r\n"
                            "Connection: Upgrade\r\n"
                            "Upgrade: h2c\r\n\r\n";
    const qint64 size = sizeof response - 1;
    return socket->write(response, size) == size;
}

void Http2Server::sendServerSettings()
{
    Q_ASSERT(socket);

    if (!serverSettings.size())
        return;

    writer.start(FrameType::SETTINGS, FrameFlag::EMPTY, connectionStreamID);
    for (auto it = serverSettings.cbegin(); it != serverSettings.cend(); ++it) {
        writer.append(it.key());
        writer.append(it.value());
        if (it.key() == Settings::INITIAL_WINDOW_SIZE_ID)
            streamRecvWindowSize = it.value();
    }
    writer.write(*socket);
    // Now, let's update our peer on a session recv window size:
    const quint32 updatedSize = 10 * streamRecvWindowSize;
    if (sessionRecvWindowSize < updatedSize) {
        const quint32 delta = updatedSize - sessionRecvWindowSize;
        sessionRecvWindowSize = updatedSize;
        sessionCurrRecvWindow = updatedSize;
        sendWINDOW_UPDATE(connectionStreamID, delta);
    }

    waitingClientAck = true;
    settingsSent = true;
}

void Http2Server::sendGOAWAY(quint32 streamID, quint32 error, quint32 lastStreamID)
{
    Q_ASSERT(socket);

    writer.start(FrameType::GOAWAY, FrameFlag::EMPTY, streamID);
    writer.append(lastStreamID);
    writer.append(error);
    writer.write(*socket);
}

void Http2Server::sendRST_STREAM(quint32 streamID, quint32 error)
{
    Q_ASSERT(socket);

    writer.start(FrameType::RST_STREAM, FrameFlag::EMPTY, streamID);
    writer.append(error);
    writer.write(*socket);
}

void Http2Server::sendDATA(quint32 streamID, quint32 windowSize)
{
    Q_ASSERT(socket);

    const auto it = suspendedStreams.find(streamID);
    Q_ASSERT(it != suspendedStreams.end());

    const quint32 offset = it->second;
    Q_ASSERT(offset < quint32(responseBody.size()));

    quint32 bytesToSend = std::min<quint32>(windowSize, responseBody.size() - offset);
    quint32 bytesSent = 0;
    const quint32 frameSizeLimit(clientSetting(Settings::MAX_FRAME_SIZE_ID, Http2::minPayloadLimit));
    const uchar *src = reinterpret_cast<const uchar *>(responseBody.constData() + offset);
    const bool last = offset + bytesToSend == quint32(responseBody.size());

    // The payload can significantly exceed frameSizeLimit. Internally, writer
    // will do needed fragmentation, but if some test failed, there is no need
    // to wait for writer to send all DATA frames, we check 'interrupted' and
    // stop early instead.
    const quint32 framesInChunk = 10;
    while (bytesToSend) {
        if (interrupted.loadAcquire())
            return;
        const quint32 chunkSize = std::min<quint32>(framesInChunk * frameSizeLimit, bytesToSend);
        writer.start(FrameType::DATA, FrameFlag::EMPTY, streamID);
        writer.writeDATA(*socket, frameSizeLimit, src, chunkSize);
        src += chunkSize;
        bytesToSend -= chunkSize;
        bytesSent += chunkSize;
        if (frameSizeLimit != Http2::minPayloadLimit) {
            // Our test is probably interested in how many DATA frames were sent.
            emit sendingData();
        }
    }

    if (interrupted.loadAcquire())
        return;

    if (last) {
        writer.start(FrameType::DATA, FrameFlag::END_STREAM, streamID);
        writer.setPayloadSize(0);
        writer.write(*socket);
        suspendedStreams.erase(it);
        activeRequests.erase(streamID);

        Q_ASSERT(closedStreams.find(streamID) == closedStreams.end());
        closedStreams.insert(streamID);
    } else {
        it->second += bytesSent;
    }
}

void Http2Server::sendWINDOW_UPDATE(quint32 streamID, quint32 delta)
{
    Q_ASSERT(socket);

    writer.start(FrameType::WINDOW_UPDATE, FrameFlag::EMPTY, streamID);
    writer.append(delta);
    writer.write(*socket);
}

void Http2Server::incomingConnection(qintptr socketDescriptor)
{
    if (isClearText()) {
        socket.reset(new QTcpSocket);
        const bool set = socket->setSocketDescriptor(socketDescriptor);
        Q_ASSERT(set);
        // Stop listening:
        close();
        upgradeProtocol = connectionType == H2Type::h2c;
        QMetaObject::invokeMethod(this, "connectionEstablished",
                                  Qt::QueuedConnection);
    } else {
#if QT_CONFIG(ssl)
        socket.reset(new QSslSocket);
        QSslSocket *sslSocket = static_cast<QSslSocket *>(socket.data());

        if (connectionType == H2Type::h2Alpn) {
            // Add HTTP2 as supported protocol:
            auto conf = QSslConfiguration::defaultConfiguration();
            auto protos = conf.allowedNextProtocols();
            protos.prepend(QSslConfiguration::ALPNProtocolHTTP2);
            conf.setAllowedNextProtocols(protos);
            sslSocket->setSslConfiguration(conf);
        }
        // SSL-related setup ...
        sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
        sslSocket->setProtocol(QSsl::TlsV1_2OrLater);
        connect(sslSocket, SIGNAL(sslErrors(QList<QSslError>)),
                this, SLOT(ignoreErrorSlot()));
        QFile file(SRCDIR "certs/fluke.key");
        file.open(QIODevice::ReadOnly);
        QSslKey key(file.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
        sslSocket->setPrivateKey(key);
        auto localCert = QSslCertificate::fromPath(SRCDIR "certs/fluke.cert");
        sslSocket->setLocalCertificateChain(localCert);
        sslSocket->setSocketDescriptor(socketDescriptor, QAbstractSocket::ConnectedState);
        // Stop listening.
        close();
        // Start SSL handshake and ALPN:
        connect(sslSocket, SIGNAL(encrypted()), this, SLOT(connectionEstablished()));
        sslSocket->startServerEncryption();
#else
        Q_ASSERT(0);
#endif
    }
}

quint32 Http2Server::clientSetting(Http2::Settings identifier, quint32 defaultValue)
{
    const auto it = expectedClientSettings.find(identifier);
    if (it != expectedClientSettings.end())
        return  it.value();
    return defaultValue;
}

bool Http2Server::readMethodLine()
{
    // We know for sure that Qt did the right thing sending us the correct
    // Request-line with CRLF at the end ...
    // We're overly simplistic here but all we need to know - the method.
    while (socket->bytesAvailable()) {
        char c = 0;
        if (socket->read(&c, 1) != 1)
            return false;
        if (c == '\n' && requestLine.endsWith('\r')) {
            if (requestLine.startsWith("GET"))
                requestType = QHttpNetworkRequest::Get;
            else if (requestLine.startsWith("POST"))
                requestType = QHttpNetworkRequest::Post;
            else
                requestType = QHttpNetworkRequest::Custom; // 'invalid'.
            requestLine.clear();

            return true;
        } else {
            requestLine.append(c);
        }
    }

    return false;
}

bool Http2Server::verifyProtocolUpgradeRequest()
{
    Q_ASSERT(protocolUpgradeHandler.data());

    bool connectionOk = false;
    bool upgradeOk = false;
    bool settingsOk = false;

    QHttpNetworkReplyPrivate *firstRequestReader = protocolUpgradeHandler->d_func();

    // That's how we append them, that's what I expect to find:
    for (const auto &header : firstRequestReader->fields) {
        if (header.first == "Connection")
            connectionOk = header.second.contains("Upgrade, HTTP2-Settings");
        else if (header.first == "Upgrade")
            upgradeOk = header.second.contains("h2c");
        else if (header.first == "HTTP2-Settings")
            settingsOk = true;
    }

    return connectionOk && upgradeOk && settingsOk;
}

void Http2Server::triggerGOAWAYEmulation()
{
    Q_ASSERT(testingGOAWAY);
    auto timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [this]() {
        sendGOAWAY(quint32(connectionStreamID), quint32(INTERNAL_ERROR), 0);
    });
    timer->start(goawayTimeout);
}

void Http2Server::connectionEstablished()
{
    using namespace Http2;

    if (testingGOAWAY && !isClearText())
        return triggerGOAWAYEmulation();

    // For clearTextHTTP2 we first have to respond with 'protocol switch'
    // and then continue with whatever logic we have (testingGOAWAY or not),
    // otherwise our 'peer' cannot process HTTP/2 frames yet.

    connect(socket.data(), SIGNAL(readyRead()),
            this, SLOT(readReady()));

    waitingClientPreface = true;
    waitingClientAck = false;
    waitingClientSettings = false;
    settingsSent = false;

    if (connectionType == H2Type::h2c) {
        requestLine.clear();
        // Now we have to handle HTTP/1.1 request. We use Get/Post in our test,
        // so set requestType to something unsupported:
        requestType = QHttpNetworkRequest::Options;
    } else {
        // We immediately send our settings so that our client
        // can use flow control correctly.
        sendServerSettings();
    }

    if (socket->bytesAvailable())
        readReady();
}

void Http2Server::ignoreErrorSlot()
{
#ifndef QT_NO_SSL
    static_cast<QSslSocket *>(socket.data())->ignoreSslErrors();
#endif
}

// Now HTTP2 "server" part:
/*
This code is overly simplified but it tests the basic HTTP2 expected behavior:
1. CONNECTION PREFACE
2. SETTINGS
3. sends our own settings (to modify the flow control)
4. collects and reports requests
5. if asked - sends responds to those requests
6. does some very basic error handling
7. tests frames validity/stream logic at the very basic level.
*/

void Http2Server::readReady()
{
    if (connectionError)
        return;

    if (redirectSent) {
        // We are a "single shot" server, working in 'h2' mode,
        // responding with a redirect code. Don't bother to handle
        // anything else now.
        return;
    }

    if (upgradeProtocol) {
        handleProtocolUpgrade();
    } else if (waitingClientPreface) {
        handleConnectionPreface();
    } else {
        const auto status = reader.read(*socket);
        switch (status) {
        case FrameStatus::incompleteFrame:
            break;
        case FrameStatus::goodFrame:
            handleIncomingFrame();
            break;
        default:
            connectionError = true;
            sendGOAWAY(connectionStreamID, PROTOCOL_ERROR, connectionStreamID);
        }
    }

    if (socket->bytesAvailable())
        QMetaObject::invokeMethod(this, "readReady", Qt::QueuedConnection);
}

void Http2Server::handleProtocolUpgrade()
{
    using ReplyPrivate = QHttpNetworkReplyPrivate;
    Q_ASSERT(upgradeProtocol);

    if (!protocolUpgradeHandler.data())
        protocolUpgradeHandler.reset(new Http11Reply);

    QHttpNetworkReplyPrivate *firstRequestReader = protocolUpgradeHandler->d_func();

    // QHttpNetworkReplyPrivate parses ... reply. It will, unfortunately, fail
    // on the first line ... which is a part of request. So we read this line
    // and extract the method first.
    if (firstRequestReader->state == ReplyPrivate::NothingDoneState) {
        if (!readMethodLine())
            return;

        if (requestType != QHttpNetworkRequest::Get && requestType != QHttpNetworkRequest::Post) {
            emit invalidRequest(1);
            return;
        }

        firstRequestReader->state = ReplyPrivate::ReadingHeaderState;
    }

    if (!socket->bytesAvailable())
        return;

    if (firstRequestReader->state == ReplyPrivate::ReadingHeaderState)
        firstRequestReader->readHeader(socket.data());
    else if (firstRequestReader->state == ReplyPrivate::ReadingDataState)
        firstRequestReader->readBodyFast(socket.data(), &firstRequestReader->responseData);

    switch (firstRequestReader->state) {
    case ReplyPrivate::ReadingHeaderState:
        return;
    case ReplyPrivate::ReadingDataState:
        if (requestType == QHttpNetworkRequest::Post)
            return;
        break;
    case ReplyPrivate::AllDoneState:
        break;
    default:
        socket->close();
        return;
    }

    if (!verifyProtocolUpgradeRequest() || !sendProtocolSwitchReply()) {
        socket->close();
        return;
    }

    upgradeProtocol = false;
    protocolUpgradeHandler.reset(nullptr);

    if (testingGOAWAY)
        return triggerGOAWAYEmulation();

    // HTTP/1.1 'fields' we have in firstRequestRead are useless (they are not
    // even allowed in HTTP/2 header). Let's pretend we have received
    // valid HTTP/2 headers and can extract fields we need:
    HttpHeader h2header;
    h2header.push_back(HeaderField(":scheme", "http")); // we are in clearTextHTTP2 mode.
    h2header.push_back(HeaderField(":authority", authority));
    activeRequests[1] = std::move(h2header);
    // After protocol switch we immediately send our SETTINGS.
    sendServerSettings();
    if (requestType == QHttpNetworkRequest::Get)
        emit receivedRequest(1);
    else
        emit receivedData(1);
}

void Http2Server::handleConnectionPreface()
{
    Q_ASSERT(waitingClientPreface);

    if (socket->bytesAvailable() < clientPrefaceLength)
        return; // Wait for more data ...

    char buf[clientPrefaceLength] = {};
    socket->read(buf, clientPrefaceLength);
    if (std::memcmp(buf, Http2clientPreface, clientPrefaceLength)) {
        sendGOAWAY(connectionStreamID, PROTOCOL_ERROR, connectionStreamID);
        emit clientPrefaceError();
        connectionError = true;
        return;
    }

    waitingClientPreface = false;
    waitingClientSettings = true;
}

void Http2Server::handleIncomingFrame()
{
    // Frames that our implementation can send include:
    // 1. SETTINGS (happens only during connection preface,
    //    handled already by this point)
    // 2. SETTIGNS with ACK should be sent only as a response
    //    to a server's SETTINGS
    // 3. HEADERS
    // 4. CONTINUATION
    // 5. DATA
    // 6. PING
    // 7. RST_STREAM
    // 8. GOAWAY

    if (testingGOAWAY) {
        // GOAWAY test is simplistic for now: after HTTP/2 was
        // negotiated (via ALPN/NPN or a protocol switch), send
        // a GOAWAY frame after some (probably non-zero) timeout.
        // We do not handle any frames, but timeout gives QNAM
        // more time to initiate more streams and thus make the
        // test more interesting/complex (on a client side).
        return;
    }

    inboundFrame = std::move(reader.inboundFrame());

    if (continuedRequest.size()) {
        if (inboundFrame.type() != FrameType::CONTINUATION ||
            inboundFrame.streamID() != continuedRequest.front().streamID()) {
            sendGOAWAY(connectionStreamID, PROTOCOL_ERROR, connectionStreamID);
            emit invalidFrame();
            connectionError = true;
            return;
        }
    }

    switch (inboundFrame.type()) {
    case FrameType::SETTINGS:
        handleSETTINGS();
        break;
    case FrameType::HEADERS:
    case FrameType::CONTINUATION:
        continuedRequest.push_back(std::move(inboundFrame));
        processRequest();
        break;
    case FrameType::DATA:
        handleDATA();
        break;
    case FrameType::RST_STREAM:
        // TODO: this is not tested for now.
        break;
    case FrameType::PING:
        // TODO: this is not tested for now.
        break;
    case FrameType::GOAWAY:
        // TODO: this is not tested for now.
        break;
    case FrameType::WINDOW_UPDATE:
        handleWINDOW_UPDATE();
        break;
    default:;
    }
}

void Http2Server::handleSETTINGS()
{
    // SETTINGS is either a part of the connection preface,
    // or a SETTINGS ACK.
    Q_ASSERT(inboundFrame.type() == FrameType::SETTINGS);

    if (inboundFrame.flags().testFlag(FrameFlag::ACK)) {
        if (!waitingClientAck || inboundFrame.dataSize()) {
            emit invalidFrame();
            connectionError = true;
            waitingClientAck = false;
            return;
        }

        waitingClientAck = false;
        emit serverSettingsAcked();
        return;
    }

    // QHttp2ProtocolHandler always sends some settings,
    // and the size is a multiple of 6.
    if (!inboundFrame.dataSize() || inboundFrame.dataSize() % 6) {
        sendGOAWAY(connectionStreamID, FRAME_SIZE_ERROR, connectionStreamID);
        emit clientPrefaceError();
        connectionError = true;
        return;
    }

    const uchar *src = inboundFrame.dataBegin();
    const uchar *end = src + inboundFrame.dataSize();

    const auto notFound = expectedClientSettings.end();

    while (src != end) {
        const auto id = Http2::Settings(qFromBigEndian<quint16>(src));
        const auto value = qFromBigEndian<quint32>(src + 2);
        if (expectedClientSettings.find(id) == notFound ||
            expectedClientSettings[id] != value) {
            emit clientPrefaceError();
            connectionError = true;
            return;
        }

        src += 6;
    }

    // Send SETTINGS ACK:
    writer.start(FrameType::SETTINGS, FrameFlag::ACK, connectionStreamID);
    writer.write(*socket);
    waitingClientSettings = false;
    emit clientPrefaceOK();
}

void Http2Server::handleDATA()
{
    Q_ASSERT(inboundFrame.type() == FrameType::DATA);

    const auto streamID = inboundFrame.streamID();

    if (!is_valid_client_stream(streamID) ||
        closedStreams.find(streamID) != closedStreams.end()) {
        emit invalidFrame();
        connectionError = true;
        sendGOAWAY(connectionStreamID, PROTOCOL_ERROR, connectionStreamID);
        return;
    }

    const auto payloadSize = inboundFrame.payloadSize();
    if (sessionCurrRecvWindow < payloadSize) {
        // Client does not respect our session window size!
        emit invalidRequest(streamID);
        connectionError = true;
        sendGOAWAY(connectionStreamID, FLOW_CONTROL_ERROR, connectionStreamID);
        return;
    }

    auto it = streamWindows.find(streamID);
    if (it == streamWindows.end())
        it = streamWindows.insert(std::make_pair(streamID, streamRecvWindowSize)).first;


    if (it->second < payloadSize) {
        emit invalidRequest(streamID);
        connectionError = true;
        sendGOAWAY(connectionStreamID, FLOW_CONTROL_ERROR, connectionStreamID);
        return;
    }

    it->second -= payloadSize;
    if (it->second < streamRecvWindowSize / 2) {
        sendWINDOW_UPDATE(streamID, streamRecvWindowSize / 2);
        it->second += streamRecvWindowSize / 2;
    }

    sessionCurrRecvWindow -= payloadSize;

    if (sessionCurrRecvWindow < sessionRecvWindowSize / 2) {
        // This is some quite naive and trivial logic on when to update.

        sendWINDOW_UPDATE(connectionStreamID, sessionRecvWindowSize / 2);
        sessionCurrRecvWindow += sessionRecvWindowSize / 2;
    }

    if (inboundFrame.flags().testFlag(FrameFlag::END_STREAM)) {
        closedStreams.insert(streamID); // Enter "half-closed remote" state.
        streamWindows.erase(it);
        emit receivedData(streamID);
    }
}

void Http2Server::handleWINDOW_UPDATE()
{
    const auto streamID = inboundFrame.streamID();
    if (!streamID) // We ignore this for now to keep things simple.
        return;

    if (streamID && suspendedStreams.find(streamID) == suspendedStreams.end()) {
        if (closedStreams.find(streamID) == closedStreams.end()) {
            sendRST_STREAM(streamID, PROTOCOL_ERROR);
            emit invalidFrame();
            connectionError = true;
        }

        return;
    }

    const quint32 delta = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    if (!delta || delta > quint32(std::numeric_limits<qint32>::max())) {
        sendRST_STREAM(streamID, PROTOCOL_ERROR);
        emit invalidFrame();
        connectionError = true;
        return;
    }

    emit windowUpdate(streamID);
    sendDATA(streamID, delta);
}

void Http2Server::sendResponse(quint32 streamID, bool emptyBody)
{
    Q_ASSERT(activeRequests.find(streamID) != activeRequests.end());

    const quint32 maxFrameSize(clientSetting(Settings::MAX_FRAME_SIZE_ID,
                                             Http2::maxPayloadSize));

    if (pushPromiseEnabled) {
        // A real server supporting PUSH_PROMISE will probably first send
        // PUSH_PROMISE and then a normal response (to a real request),
        // so that a client parsing this response and discovering another
        // resource it needs, will _already_ have this additional resource
        // in PUSH_PROMISE.
        lastPromisedStream += 2;

        writer.start(FrameType::PUSH_PROMISE, FrameFlag::END_HEADERS, streamID);
        writer.append(lastPromisedStream);

        HttpHeader pushHeader;
        fill_push_header(activeRequests[streamID], pushHeader);
        pushHeader.push_back(HeaderField(":method", "GET"));
        pushHeader.push_back(HeaderField(":path", pushPath));

        // Now interesting part, let's make it into 'stream':
        activeRequests[lastPromisedStream] = pushHeader;

        HPack::BitOStream ostream(writer.outboundFrame().buffer);
        const bool result = encoder.encodeRequest(ostream, pushHeader);
        Q_ASSERT(result);

        // Well, it's not HEADERS, it's PUSH_PROMISE with ... HEADERS block.
        // Should work.
        writer.writeHEADERS(*socket, maxFrameSize);
        qDebug() << "server sent a PUSH_PROMISE on" << lastPromisedStream;

        if (responseBody.isEmpty())
            responseBody = QByteArray("I PROMISE (AND PUSH) YOU ...");

        // Now we send this promised data as a normal response on our reserved
        // stream (disabling PUSH_PROMISE for the moment to avoid recursion):
        pushPromiseEnabled = false;
        sendResponse(lastPromisedStream, false);
        pushPromiseEnabled = true;
        // Now we'll continue with _normal_ response.
    }

    writer.start(FrameType::HEADERS, FrameFlag::END_HEADERS, streamID);
    if (emptyBody)
        writer.addFlag(FrameFlag::END_STREAM);

    HttpHeader header;
    if (redirectWhileReading) {
        if (redirectSent) {
            // This is a "single-shot" server responding with a redirect code.
            return;
        }

        redirectSent = true;

        qDebug("server received HEADERS frame (followed by DATA frames), redirecting ...");
        Q_ASSERT(targetPort);
        header.push_back({":status", "308"});
        const QString url("%1://localhost:%2/");
        header.push_back({"location", url.arg(isClearText() ? QStringLiteral("http") : QStringLiteral("https"),
                                              QString::number(targetPort)).toLatin1()});

    } else {
        header.push_back({":status", "200"});
    }

    if (!emptyBody) {
        header.push_back(HPack::HeaderField("content-length",
                         QString("%1").arg(responseBody.size()).toLatin1()));
    }

    HPack::BitOStream ostream(writer.outboundFrame().buffer);
    const bool result = encoder.encodeResponse(ostream, header);
    Q_ASSERT(result);

    writer.writeHEADERS(*socket, maxFrameSize);

    if (!emptyBody) {
        Q_ASSERT(suspendedStreams.find(streamID) == suspendedStreams.end());

        const quint32 windowSize = clientSetting(Settings::INITIAL_WINDOW_SIZE_ID,
                                                 Http2::defaultSessionWindowSize);
        // Suspend to immediately resume it.
        suspendedStreams[streamID] = 0; // start sending from offset 0
        sendDATA(streamID, windowSize);
    } else {
        activeRequests.erase(streamID);
        closedStreams.insert(streamID);
    }
}

void Http2Server::stopSendingDATAFrames()
{
    interrupted.storeRelease(1);
}

void Http2Server::processRequest()
{
    Q_ASSERT(continuedRequest.size());

    if (!continuedRequest.back().flags().testFlag(FrameFlag::END_HEADERS))
        return;

    // We test here:
    // 1. stream is 'idle'.
    // 2. has priority set and dependency (it's 0x0 at the moment).
    // 3. header can be decompressed.
    const auto &headersFrame = continuedRequest.front();
    const auto streamID = headersFrame.streamID();
    if (!is_valid_client_stream(streamID)) {
        emit invalidRequest(streamID);
        connectionError = true;
        sendGOAWAY(connectionStreamID, PROTOCOL_ERROR, connectionStreamID);
        return;
    }

    if (closedStreams.find(streamID) != closedStreams.end()) {
        emit invalidFrame();
        connectionError = true;
        sendGOAWAY(connectionStreamID, PROTOCOL_ERROR, connectionStreamID);
        return;
    }

    quint32 dep = 0;
    uchar w = 0;
    if (!headersFrame.priority(&dep, &w)) {
        emit invalidFrame();
        sendRST_STREAM(streamID, PROTOCOL_ERROR);
        return;
    }

    // Assemble headers ...
    quint32 totalSize = 0;
    for (const auto &frame : continuedRequest) {
        if (std::numeric_limits<quint32>::max() - frame.dataSize() < totalSize) {
            // Resulted in overflow ...
            emit invalidFrame();
            connectionError = true;
            sendGOAWAY(connectionStreamID, PROTOCOL_ERROR, connectionStreamID);
            return;
        }
        totalSize += frame.dataSize();
    }

    std::vector<uchar> hpackBlock(totalSize);
    auto dst = hpackBlock.begin();
    for (const auto &frame : continuedRequest) {
        if (!frame.dataSize())
            continue;
        std::copy(frame.dataBegin(), frame.dataBegin() + frame.dataSize(), dst);
        dst += frame.dataSize();
    }

    HPack::BitIStream inputStream{&hpackBlock[0], &hpackBlock[0] + hpackBlock.size()};

    if (!decoder.decodeHeaderFields(inputStream)) {
        emit decompressionFailed(streamID);
        sendRST_STREAM(streamID, COMPRESSION_ERROR);
        closedStreams.insert(streamID);
        return;
    }

    // Actually, if needed, we can do a comparison here.
    activeRequests[streamID] = decoder.decodedHeader();
    if (headersFrame.flags().testFlag(FrameFlag::END_STREAM))
        emit receivedRequest(streamID);

    if (redirectWhileReading) {
        sendResponse(streamID, true);
        // Don't try to read any DATA frames ...
        socket->disconnect();
    } // else - we're waiting for incoming DATA frames ...

    continuedRequest.clear();
}

QT_END_NAMESPACE
