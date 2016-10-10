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
    return (streamID & 0x1) && streamID <= std::numeric_limits<qint32>::max();
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

Http2Server::Http2Server(bool h2c, const Http2Settings &ss, const Http2Settings &cs)
    : serverSettings(ss),
      clearTextHTTP2(h2c)
{
    for (const auto &s : cs)
        expectedClientSettings[quint16(s.identifier)] = s.value;

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

void Http2Server::startServer()
{
#ifdef QT_NO_SSL
    // Let the test fail with timeout.
    if (!clearTextHTTP2)
        return;
#endif
    if (listen())
        emit serverStarted(serverPort());
}

void Http2Server::sendServerSettings()
{
    Q_ASSERT(socket);

    if (!serverSettings.size())
        return;

    writer.start(FrameType::SETTINGS, FrameFlag::EMPTY, connectionStreamID);
    for (const auto &s : serverSettings) {
        writer.append(s.identifier);
        writer.append(s.value);
        if (s.identifier == Settings::INITIAL_WINDOW_SIZE_ID)
            streamRecvWindowSize = s.value;
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

    const quint32 bytes = std::min<quint32>(windowSize, responseBody.size() - offset);
    const quint32 frameSizeLimit(clientSetting(Settings::MAX_FRAME_SIZE_ID, Http2::maxFrameSize));
    const uchar *src = reinterpret_cast<const uchar *>(responseBody.constData() + offset);
    const bool last = offset + bytes == quint32(responseBody.size());

    writer.start(FrameType::DATA, FrameFlag::EMPTY, streamID);
    writer.writeDATA(*socket, frameSizeLimit, src, bytes);

    if (last) {
        writer.start(FrameType::DATA, FrameFlag::END_STREAM, streamID);
        writer.setPayloadSize(0);
        writer.write(*socket);
        suspendedStreams.erase(it);
        activeRequests.erase(streamID);

        Q_ASSERT(closedStreams.find(streamID) == closedStreams.end());
        closedStreams.insert(streamID);
    } else {
        it->second += bytes;
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
    if (clearTextHTTP2) {
        socket.reset(new QTcpSocket);
        const bool set = socket->setSocketDescriptor(socketDescriptor);
        Q_ASSERT(set);
        // Stop listening:
        close();
        QMetaObject::invokeMethod(this, "connectionEstablished",
                                  Qt::QueuedConnection);
    } else {
#ifndef QT_NO_SSL
        socket.reset(new QSslSocket);
        QSslSocket *sslSocket = static_cast<QSslSocket *>(socket.data());
        // Add HTTP2 as supported protocol:
        auto conf = QSslConfiguration::defaultConfiguration();
        auto protos = conf.allowedNextProtocols();
        protos.prepend(QSslConfiguration::ALPNProtocolHTTP2);
        conf.setAllowedNextProtocols(protos);
        sslSocket->setSslConfiguration(conf);
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
        Q_UNREACHABLE();
#endif
    }
}

quint32 Http2Server::clientSetting(Http2::Settings identifier, quint32 defaultValue)
{
    const auto it = expectedClientSettings.find(quint16(identifier));
    if (it != expectedClientSettings.end())
        return  it->second;
    return defaultValue;
}

void Http2Server::connectionEstablished()
{
    using namespace Http2;

    connect(socket.data(), SIGNAL(readyRead()),
            this, SLOT(readReady()));

    waitingClientPreface = true;
    waitingClientAck = false;
    waitingClientSettings = false;
    settingsSent = false;
    // We immediately send our settings so that our client
    // can use flow control correctly.
    sendServerSettings();

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

    if (waitingClientPreface) {
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
        const auto id = qFromBigEndian<quint16>(src);
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
                                             Http2::maxFrameSize));

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

    HttpHeader header = {{":status", "200"}};
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
    // else - we're waiting for incoming DATA frames ...
    continuedRequest.clear();
}

QT_END_NAMESPACE
