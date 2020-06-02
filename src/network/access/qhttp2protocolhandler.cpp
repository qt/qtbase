/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qhttpnetworkconnection_p.h"
#include "qhttp2protocolhandler_p.h"

#include "http2/http2frames_p.h"
#include "http2/bitstreams_p.h"

#include <private/qnoncontiguousbytedevice_p.h>

#include <QtNetwork/qabstractsocket.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qendian.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <QtCore/qurl.h>

#include <qhttp2configuration.h>

#ifndef QT_NO_NETWORKPROXY
#include <QtNetwork/qnetworkproxy.h>
#endif

#include <qcoreapplication.h>

#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

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
    header.push_back(HeaderField(":authority", auth));
    header.push_back(HeaderField(":method", request.methodName()));
    header.push_back(HeaderField(":path", request.uri(useProxy)));
    header.push_back(HeaderField(":scheme", request.url().scheme().toLatin1()));

    HeaderSize size = header_size(header);
    if (!size.first) // Ooops!
        return HttpHeader();

    if (size.second > maxHeaderListSize)
        return HttpHeader(); // Bad, we cannot send this request ...

    const auto requestHeader = request.header();
    for (const auto &field : requestHeader) {
        const HeaderSize delta = entry_size(field.first, field.second);
        if (!delta.first) // Overflow???
            break;
        if (std::numeric_limits<quint32>::max() - delta.second < size.second)
            break;
        size.second += delta.second;
        if (size.second > maxHeaderListSize)
            break;

        if (field.first.compare("connection", Qt::CaseInsensitive) == 0 ||
                field.first.compare("host", Qt::CaseInsensitive) == 0 ||
                field.first.compare("keep-alive", Qt::CaseInsensitive) == 0 ||
                field.first.compare("proxy-connection", Qt::CaseInsensitive) == 0 ||
                field.first.compare("transfer-encoding", Qt::CaseInsensitive) == 0)
            continue; // Those headers are not valid (section 3.2.1) - from QSpdyProtocolHandler
        // TODO: verify with specs, which fields are valid to send ....
        // toLower - 8.1.2 .... "header field names MUST be converted to lowercase prior
        // to their encoding in HTTP/2.
        // A request or response containing uppercase header field names
        // MUST be treated as malformed (Section 8.1.2.6)".
        header.push_back(HeaderField(field.first.toLower(), field.second));
    }

    return header;
}

std::vector<uchar> assemble_hpack_block(const std::vector<Http2::Frame> &frames)
{
    std::vector<uchar> hpackBlock;

    quint32 total = 0;
    for (const auto &frame : frames)
        total += frame.hpackBlockSize();

    if (!total)
        return hpackBlock;

    hpackBlock.resize(total);
    auto dst = hpackBlock.begin();
    for (const auto &frame : frames) {
        if (const auto hpackBlockSize = frame.hpackBlockSize()) {
            const uchar *src = frame.hpackBlockBegin();
            std::copy(src, src + hpackBlockSize, dst);
            dst += hpackBlockSize;
        }
    }

    return hpackBlock;
}

QUrl urlkey_from_request(const QHttpNetworkRequest &request)
{
    QUrl url;

    url.setScheme(request.url().scheme());
    url.setAuthority(request.url().authority(QUrl::FullyEncoded | QUrl::RemoveUserInfo));
    url.setPath(QLatin1String(request.uri(false)));

    return url;
}

bool sum_will_overflow(qint32 windowSize, qint32 delta)
{
    if (windowSize > 0)
        return std::numeric_limits<qint32>::max() - windowSize < delta;
    return std::numeric_limits<qint32>::min() - windowSize > delta;
}

}// Unnamed namespace

// Since we anyway end up having this in every function definition:
using namespace Http2;

const std::deque<quint32>::size_type QHttp2ProtocolHandler::maxRecycledStreams = 10000;
const quint32 QHttp2ProtocolHandler::maxAcceptableTableSize;

QHttp2ProtocolHandler::QHttp2ProtocolHandler(QHttpNetworkConnectionChannel *channel)
    : QAbstractProtocolHandler(channel),
      decoder(HPack::FieldLookupTable::DefaultSize),
      encoder(HPack::FieldLookupTable::DefaultSize, true)
{
    Q_ASSERT(channel && m_connection);
    continuedFrames.reserve(20);

    const auto h2Config = m_connection->http2Parameters();
    maxSessionReceiveWindowSize = h2Config.sessionReceiveWindowSize();
    pushPromiseEnabled = h2Config.serverPushEnabled();
    streamInitialReceiveWindowSize = h2Config.streamReceiveWindowSize();
    encoder.setCompressStrings(h2Config.huffmanCompressionEnabled());

    if (!channel->ssl && m_connection->connectionType() != QHttpNetworkConnection::ConnectionTypeHTTP2Direct) {
        // We upgraded from HTTP/1.1 to HTTP/2. channel->request was already sent
        // as HTTP/1.1 request. The response with status code 101 triggered
        // protocol switch and now we are waiting for the real response, sent
        // as HTTP/2 frames.
        Q_ASSERT(channel->reply);
        const quint32 initialStreamID = createNewStream(HttpMessagePair(channel->request, channel->reply),
                                                        true /* uploaded by HTTP/1.1 */);
        Q_ASSERT(initialStreamID == 1);
        Stream &stream = activeStreams[initialStreamID];
        stream.state = Stream::halfClosedLocal;
    }
}

void QHttp2ProtocolHandler::handleConnectionClosure()
{
    // The channel has just received RemoteHostClosedError and since it will
    // not try (for HTTP/2) to re-connect, it's time to finish all replies
    // with error.

    // Maybe we still have some data to read and can successfully finish
    // a stream/request?
    _q_receiveReply();

    // Finish all still active streams. If we previously had GOAWAY frame,
    // we probably already closed some (or all) streams with ContentReSend
    // error, but for those still active, not having any data to finish,
    // we now report RemoteHostClosedError.
    const auto errorString = QCoreApplication::translate("QHttp", "Connection closed");
    for (auto it = activeStreams.begin(), eIt = activeStreams.end(); it != eIt; ++it)
        finishStreamWithError(it.value(), QNetworkReply::RemoteHostClosedError, errorString);

    // Make sure we'll never try to read anything later:
    activeStreams.clear();
    goingAway = true;
}

void QHttp2ProtocolHandler::ensureClientPrefaceSent()
{
    if (!prefaceSent)
        sendClientPreface();
}

void QHttp2ProtocolHandler::_q_uploadDataReadyRead()
{
    if (!sender()) // QueuedConnection, firing after sender (byte device) was deleted.
        return;

    auto data = qobject_cast<QNonContiguousByteDevice *>(sender());
    Q_ASSERT(data);
    const qint32 streamID = streamIDs.value(data);
    Q_ASSERT(streamID != 0);
    Q_ASSERT(activeStreams.contains(streamID));
    auto &stream = activeStreams[streamID];

    if (!sendDATA(stream)) {
        finishStreamWithError(stream, QNetworkReply::UnknownNetworkError,
                              QLatin1String("failed to send DATA"));
        sendRST_STREAM(streamID, INTERNAL_ERROR);
        markAsReset(streamID);
        deleteActiveStream(streamID);
    }
}

void QHttp2ProtocolHandler::_q_replyDestroyed(QObject *reply)
{
    const quint32 streamID = streamIDs.take(reply);
    if (activeStreams.contains(streamID)) {
        sendRST_STREAM(streamID, CANCEL);
        markAsReset(streamID);
        deleteActiveStream(streamID);
    }
}

void QHttp2ProtocolHandler::_q_uploadDataDestroyed(QObject *uploadData)
{
    streamIDs.remove(uploadData);
}

void QHttp2ProtocolHandler::_q_readyRead()
{
    _q_receiveReply();
}

void QHttp2ProtocolHandler::_q_receiveReply()
{
    Q_ASSERT(m_socket);
    Q_ASSERT(m_channel);

    while (!goingAway || activeStreams.size()) {
        const auto result = frameReader.read(*m_socket);
        switch (result) {
        case FrameStatus::incompleteFrame:
            return;
        case FrameStatus::protocolError:
            return connectionError(PROTOCOL_ERROR, "invalid frame");
        case FrameStatus::sizeError:
            return connectionError(FRAME_SIZE_ERROR, "invalid frame size");
        default:
            break;
        }

        Q_ASSERT(result == FrameStatus::goodFrame);

        inboundFrame = std::move(frameReader.inboundFrame());

        const auto frameType = inboundFrame.type();
        if (continuationExpected && frameType != FrameType::CONTINUATION)
            return connectionError(PROTOCOL_ERROR, "CONTINUATION expected");

        switch (frameType) {
        case FrameType::DATA:
            handleDATA();
            break;
        case FrameType::HEADERS:
            handleHEADERS();
            break;
        case FrameType::PRIORITY:
            handlePRIORITY();
            break;
        case FrameType::RST_STREAM:
            handleRST_STREAM();
            break;
        case FrameType::SETTINGS:
            handleSETTINGS();
            break;
        case FrameType::PUSH_PROMISE:
            handlePUSH_PROMISE();
            break;
        case FrameType::PING:
            handlePING();
            break;
        case FrameType::GOAWAY:
            handleGOAWAY();
            break;
        case FrameType::WINDOW_UPDATE:
            handleWINDOW_UPDATE();
            break;
        case FrameType::CONTINUATION:
            handleCONTINUATION();
            break;
        case FrameType::LAST_FRAME_TYPE:
            // 5.1 - ignore unknown frames.
            break;
        }
    }
}

bool QHttp2ProtocolHandler::sendRequest()
{
    if (goingAway) {
        // Stop further calls to this method: we have received GOAWAY
        // so we cannot create new streams.
        m_channel->emitFinishedWithError(QNetworkReply::ProtocolUnknownError,
                                         "GOAWAY received, cannot start a request");
        m_channel->spdyRequestsToSend.clear();
        return false;
    }

    // Process 'fake' (created by QNetworkAccessManager::connectToHostEncrypted())
    // requests first:
    auto &requests = m_channel->spdyRequestsToSend;
    for (auto it = requests.begin(), endIt = requests.end(); it != endIt;) {
        const auto &pair = *it;
        const QString scheme(pair.first.url().scheme());
        if (scheme == QLatin1String("preconnect-http")
            || scheme == QLatin1String("preconnect-https")) {
            m_connection->preConnectFinished();
            emit pair.second->finished();
            it = requests.erase(it);
            if (!requests.size()) {
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

    if (!prefaceSent && !sendClientPreface())
        return false;

    if (!requests.size())
        return true;

    m_channel->state = QHttpNetworkConnectionChannel::WritingState;
    // Check what was promised/pushed, maybe we do not have to send a request
    // and have a response already?

    for (auto it = requests.begin(), endIt = requests.end(); it != endIt;) {
        const auto key = urlkey_from_request(it->first).toString();
        if (!promisedData.contains(key)) {
            ++it;
            continue;
        }
        // Woo-hoo, we do not have to ask, the answer is ready for us:
        HttpMessagePair message = *it;
        it = requests.erase(it);
        initReplyFromPushPromise(message, key);
    }

    const auto streamsToUse = std::min<quint32>(maxConcurrentStreams - activeStreams.size(),
                                                requests.size());
    auto it = requests.begin();
    for (quint32 i = 0; i < streamsToUse; ++i) {
        const qint32 newStreamID = createNewStream(*it);
        if (!newStreamID) {
            // TODO: actually we have to open a new connection.
            qCCritical(QT_HTTP2, "sendRequest: out of stream IDs");
            break;
        }

        it = requests.erase(it);

        Stream &newStream = activeStreams[newStreamID];
        if (!sendHEADERS(newStream)) {
            finishStreamWithError(newStream, QNetworkReply::UnknownNetworkError,
                                  QLatin1String("failed to send HEADERS frame(s)"));
            deleteActiveStream(newStreamID);
            continue;
        }

        if (newStream.data() && !sendDATA(newStream)) {
            finishStreamWithError(newStream, QNetworkReply::UnknownNetworkError,
                                  QLatin1String("failed to send DATA frame(s)"));
            sendRST_STREAM(newStreamID, INTERNAL_ERROR);
            markAsReset(newStreamID);
            deleteActiveStream(newStreamID);
        }
    }

    m_channel->state = QHttpNetworkConnectionChannel::IdleState;

    return true;
}


bool QHttp2ProtocolHandler::sendClientPreface()
{
     // 3.5 HTTP/2 Connection Preface
    Q_ASSERT(m_socket);

    if (prefaceSent)
        return true;

    const qint64 written = m_socket->write(Http2::Http2clientPreface,
                                           Http2::clientPrefaceLength);
    if (written != Http2::clientPrefaceLength)
        return false;

    // 6.5 SETTINGS
    frameWriter.setOutboundFrame(Http2::configurationToSettingsFrame(m_connection->http2Parameters()));
    Q_ASSERT(frameWriter.outboundFrame().payloadSize());

    if (!frameWriter.write(*m_socket))
        return false;

    sessionReceiveWindowSize = maxSessionReceiveWindowSize;
    // We only send WINDOW_UPDATE for the connection if the size differs from the
    // default 64 KB:
    const auto delta = maxSessionReceiveWindowSize - Http2::defaultSessionWindowSize;
    if (delta && !sendWINDOW_UPDATE(Http2::connectionStreamID, delta))
        return false;

    prefaceSent = true;
    waitingForSettingsACK = true;

    return true;
}

bool QHttp2ProtocolHandler::sendSETTINGS_ACK()
{
    Q_ASSERT(m_socket);

    if (!prefaceSent && !sendClientPreface())
        return false;

    frameWriter.start(FrameType::SETTINGS, FrameFlag::ACK, Http2::connectionStreamID);

    return frameWriter.write(*m_socket);
}

bool QHttp2ProtocolHandler::sendHEADERS(Stream &stream)
{
    using namespace HPack;

    frameWriter.start(FrameType::HEADERS, FrameFlag::PRIORITY | FrameFlag::END_HEADERS,
                      stream.streamID);

    if (!stream.data()) {
        frameWriter.addFlag(FrameFlag::END_STREAM);
        stream.state = Stream::halfClosedLocal;
    } else {
        stream.state = Stream::open;
    }

    frameWriter.append(quint32()); // No stream dependency in Qt.
    frameWriter.append(stream.weight());

    bool useProxy = false;
#ifndef QT_NO_NETWORKPROXY
    useProxy = m_connection->d_func()->networkProxy.type() != QNetworkProxy::NoProxy;
#endif
    const auto headers = build_headers(stream.request(), maxHeaderListSize, useProxy);
    if (!headers.size()) // nothing fits into maxHeaderListSize
        return false;

    // Compress in-place:
    BitOStream outputStream(frameWriter.outboundFrame().buffer);
    if (!encoder.encodeRequest(outputStream, headers))
        return false;

    return frameWriter.writeHEADERS(*m_socket, maxFrameSize);
}

bool QHttp2ProtocolHandler::sendDATA(Stream &stream)
{
    Q_ASSERT(maxFrameSize > frameHeaderSize);
    Q_ASSERT(m_socket);
    Q_ASSERT(stream.data());

    const auto &request = stream.request();
    auto reply = stream.reply();
    Q_ASSERT(reply);
    const auto replyPrivate = reply->d_func();
    Q_ASSERT(replyPrivate);

    auto slot = std::min<qint32>(sessionSendWindowSize, stream.sendWindow);
    while (!stream.data()->atEnd() && slot) {
        qint64 chunkSize = 0;
        const uchar *src =
            reinterpret_cast<const uchar *>(stream.data()->readPointer(slot, chunkSize));

        if (chunkSize == -1)
            return false;

        if (!src || !chunkSize) {
            // Stream is not suspended by the flow control,
            // we do not have data ready yet.
            return true;
        }

        frameWriter.start(FrameType::DATA, FrameFlag::EMPTY, stream.streamID);
        const qint32 bytesWritten = std::min<qint32>(slot, chunkSize);

        if (!frameWriter.writeDATA(*m_socket, maxFrameSize, src, bytesWritten))
            return false;

        stream.data()->advanceReadPointer(bytesWritten);
        stream.sendWindow -= bytesWritten;
        sessionSendWindowSize -= bytesWritten;
        replyPrivate->totallyUploadedData += bytesWritten;
        emit reply->dataSendProgress(replyPrivate->totallyUploadedData,
                                     request.contentLength());
        slot = std::min(sessionSendWindowSize, stream.sendWindow);
    }

    if (replyPrivate->totallyUploadedData == request.contentLength()) {
        frameWriter.start(FrameType::DATA, FrameFlag::END_STREAM, stream.streamID);
        frameWriter.setPayloadSize(0);
        frameWriter.write(*m_socket);
        stream.state = Stream::halfClosedLocal;
        stream.data()->disconnect(this);
        removeFromSuspended(stream.streamID);
    } else if (!stream.data()->atEnd()) {
        addToSuspended(stream);
    }

    return true;
}

bool QHttp2ProtocolHandler::sendWINDOW_UPDATE(quint32 streamID, quint32 delta)
{
    Q_ASSERT(m_socket);

    frameWriter.start(FrameType::WINDOW_UPDATE, FrameFlag::EMPTY, streamID);
    frameWriter.append(delta);
    return frameWriter.write(*m_socket);
}

bool QHttp2ProtocolHandler::sendRST_STREAM(quint32 streamID, quint32 errorCode)
{
    Q_ASSERT(m_socket);

    frameWriter.start(FrameType::RST_STREAM, FrameFlag::EMPTY, streamID);
    frameWriter.append(errorCode);
    return frameWriter.write(*m_socket);
}

bool QHttp2ProtocolHandler::sendGOAWAY(quint32 errorCode)
{
    Q_ASSERT(m_socket);

    frameWriter.start(FrameType::GOAWAY, FrameFlag::EMPTY, connectionStreamID);
    frameWriter.append(quint32(connectionStreamID));
    frameWriter.append(errorCode);
    return frameWriter.write(*m_socket);
}

void QHttp2ProtocolHandler::handleDATA()
{
    Q_ASSERT(inboundFrame.type() == FrameType::DATA);

    const auto streamID = inboundFrame.streamID();
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "DATA on stream 0x0");

    if (!activeStreams.contains(streamID) && !streamWasReset(streamID))
        return connectionError(ENHANCE_YOUR_CALM, "DATA on invalid stream");

    if (qint32(inboundFrame.payloadSize()) > sessionReceiveWindowSize)
        return connectionError(FLOW_CONTROL_ERROR, "Flow control error");

    sessionReceiveWindowSize -= inboundFrame.payloadSize();

    if (activeStreams.contains(streamID)) {
        auto &stream = activeStreams[streamID];

        if (qint32(inboundFrame.payloadSize()) > stream.recvWindow) {
            finishStreamWithError(stream, QNetworkReply::ProtocolFailure,
                                  QLatin1String("flow control error"));
            sendRST_STREAM(streamID, FLOW_CONTROL_ERROR);
            markAsReset(streamID);
            deleteActiveStream(streamID);
        } else {
            stream.recvWindow -= inboundFrame.payloadSize();
            // Uncompress data if needed and append it ...
            updateStream(stream, inboundFrame);

            if (inboundFrame.flags().testFlag(FrameFlag::END_STREAM)) {
                finishStream(stream);
                deleteActiveStream(stream.streamID);
            } else if (stream.recvWindow < streamInitialReceiveWindowSize / 2) {
                QMetaObject::invokeMethod(this, "sendWINDOW_UPDATE", Qt::QueuedConnection,
                                          Q_ARG(quint32, stream.streamID),
                                          Q_ARG(quint32, streamInitialReceiveWindowSize - stream.recvWindow));
                stream.recvWindow = streamInitialReceiveWindowSize;
            }
        }
    }

    if (sessionReceiveWindowSize < maxSessionReceiveWindowSize / 2) {
        QMetaObject::invokeMethod(this, "sendWINDOW_UPDATE", Qt::QueuedConnection,
                                  Q_ARG(quint32, connectionStreamID),
                                  Q_ARG(quint32, maxSessionReceiveWindowSize - sessionReceiveWindowSize));
        sessionReceiveWindowSize = maxSessionReceiveWindowSize;
    }
}

void QHttp2ProtocolHandler::handleHEADERS()
{
    Q_ASSERT(inboundFrame.type() == FrameType::HEADERS);

    const auto streamID = inboundFrame.streamID();
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "HEADERS on 0x0 stream");

    if (!activeStreams.contains(streamID) && !streamWasReset(streamID))
        return connectionError(ENHANCE_YOUR_CALM, "HEADERS on invalid stream");

    const auto flags = inboundFrame.flags();
    if (flags.testFlag(FrameFlag::PRIORITY)) {
        handlePRIORITY();
        if (goingAway)
            return;
    }

    const bool endHeaders = flags.testFlag(FrameFlag::END_HEADERS);
    continuedFrames.clear();
    continuedFrames.push_back(std::move(inboundFrame));
    if (!endHeaders) {
        continuationExpected = true;
        return;
    }

    handleContinuedHEADERS();
}

void QHttp2ProtocolHandler::handlePRIORITY()
{
    Q_ASSERT(inboundFrame.type() == FrameType::PRIORITY ||
             inboundFrame.type() == FrameType::HEADERS);

    const auto streamID = inboundFrame.streamID();
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "PIRORITY on 0x0 stream");

    if (!activeStreams.contains(streamID) && !streamWasReset(streamID))
        return connectionError(ENHANCE_YOUR_CALM, "PRIORITY on invalid stream");

    quint32 streamDependency = 0;
    uchar weight = 0;
    const bool noErr = inboundFrame.priority(&streamDependency, &weight);
    Q_UNUSED(noErr) Q_ASSERT(noErr);


    const bool exclusive = streamDependency & 0x80000000;
    streamDependency &= ~0x80000000;

    // Ignore this for now ...
    // Can be used for streams (re)prioritization - 5.3
    Q_UNUSED(exclusive);
    Q_UNUSED(weight);
}

void QHttp2ProtocolHandler::handleRST_STREAM()
{
    Q_ASSERT(inboundFrame.type() == FrameType::RST_STREAM);

    // "RST_STREAM frames MUST be associated with a stream.
    // If a RST_STREAM frame is received with a stream identifier of 0x0,
    // the recipient MUST treat this as a connection error (Section 5.4.1)
    // of type PROTOCOL_ERROR.
    const auto streamID = inboundFrame.streamID();
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "RST_STREAM on 0x0");

    if (!(streamID & 0x1)) {
        // RST_STREAM on a promised stream:
        // since we do not keep track of such streams,
        // just ignore.
        return;
    }

    if (streamID >= nextID) {
        // "RST_STREAM frames MUST NOT be sent for a stream
        // in the "idle" state. .. the recipient MUST treat this
        // as a connection error (Section 5.4.1) of type PROTOCOL_ERROR."
        return connectionError(PROTOCOL_ERROR, "RST_STREAM on idle stream");
    }

    if (!activeStreams.contains(streamID)) {
        // 'closed' stream, ignore.
        return;
    }

    Q_ASSERT(inboundFrame.dataSize() == 4);

    Stream &stream = activeStreams[streamID];
    finishStreamWithError(stream, qFromBigEndian<quint32>(inboundFrame.dataBegin()));
    markAsReset(stream.streamID);
    deleteActiveStream(stream.streamID);
}

void QHttp2ProtocolHandler::handleSETTINGS()
{
    // 6.5 SETTINGS.
    Q_ASSERT(inboundFrame.type() == FrameType::SETTINGS);

    if (inboundFrame.streamID() != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "SETTINGS on invalid stream");

    if (inboundFrame.flags().testFlag(FrameFlag::ACK)) {
        if (!waitingForSettingsACK)
            return connectionError(PROTOCOL_ERROR, "unexpected SETTINGS ACK");
        waitingForSettingsACK = false;
        return;
    }

    if (inboundFrame.dataSize()) {
        auto src = inboundFrame.dataBegin();
        for (const uchar *end = src + inboundFrame.dataSize(); src != end; src += 6) {
            const Settings identifier = Settings(qFromBigEndian<quint16>(src));
            const quint32 intVal = qFromBigEndian<quint32>(src + 2);
            if (!acceptSetting(identifier, intVal)) {
                // If not accepted - we finish with connectionError.
                return;
            }
        }
    }

    sendSETTINGS_ACK();
}


void QHttp2ProtocolHandler::handlePUSH_PROMISE()
{
    // 6.6 PUSH_PROMISE.
    Q_ASSERT(inboundFrame.type() == FrameType::PUSH_PROMISE);

    if (!pushPromiseEnabled && prefaceSent && !waitingForSettingsACK) {
        // This means, server ACKed our 'NO PUSH',
        // but sent us PUSH_PROMISE anyway.
        return connectionError(PROTOCOL_ERROR, "unexpected PUSH_PROMISE frame");
    }

    const auto streamID = inboundFrame.streamID();
    if (streamID == connectionStreamID) {
        return connectionError(PROTOCOL_ERROR,
                               "PUSH_PROMISE with invalid associated stream (0x0)");
    }

    if (!activeStreams.contains(streamID) && !streamWasReset(streamID)) {
        return connectionError(ENHANCE_YOUR_CALM,
                               "PUSH_PROMISE with invalid associated stream");
    }

    const auto reservedID = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    if ((reservedID & 1) || reservedID <= lastPromisedID ||
        reservedID > Http2::lastValidStreamID) {
        return connectionError(PROTOCOL_ERROR,
                               "PUSH_PROMISE with invalid promised stream ID");
    }

    lastPromisedID = reservedID;

    if (!pushPromiseEnabled) {
        // "ignoring a PUSH_PROMISE frame causes the stream state to become
        // indeterminate" - let's send RST_STREAM frame with REFUSE_STREAM code.
        resetPromisedStream(inboundFrame, Http2::REFUSE_STREAM);
    }

    const bool endHeaders = inboundFrame.flags().testFlag(FrameFlag::END_HEADERS);
    continuedFrames.clear();
    continuedFrames.push_back(std::move(inboundFrame));

    if (!endHeaders) {
        continuationExpected = true;
        return;
    }

    handleContinuedHEADERS();
}

void QHttp2ProtocolHandler::handlePING()
{
    // Since we're implementing a client and not
    // a server, we only reply to a PING, ACKing it.
    Q_ASSERT(inboundFrame.type() == FrameType::PING);
    Q_ASSERT(m_socket);

    if (inboundFrame.streamID() != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "PING on invalid stream");

    if (inboundFrame.flags() & FrameFlag::ACK)
        return connectionError(PROTOCOL_ERROR, "unexpected PING ACK");

    Q_ASSERT(inboundFrame.dataSize() == 8);

    frameWriter.start(FrameType::PING, FrameFlag::ACK, connectionStreamID);
    frameWriter.append(inboundFrame.dataBegin(), inboundFrame.dataBegin() + 8);
    frameWriter.write(*m_socket);
}

void QHttp2ProtocolHandler::handleGOAWAY()
{
    // 6.8 GOAWAY

    Q_ASSERT(inboundFrame.type() == FrameType::GOAWAY);
    // "An endpoint MUST treat a GOAWAY frame with a stream identifier
    // other than 0x0 as a connection error (Section 5.4.1) of type PROTOCOL_ERROR."
    if (inboundFrame.streamID() != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "GOAWAY on invalid stream");

    const auto src = inboundFrame.dataBegin();
    quint32 lastStreamID = qFromBigEndian<quint32>(src);
    const quint32 errorCode = qFromBigEndian<quint32>(src + 4);

    if (!lastStreamID) {
        // "The last stream identifier can be set to 0 if no
        // streams were processed."
        lastStreamID = 1;
    } else if (!(lastStreamID & 0x1)) {
        // 5.1.1 - we (client) use only odd numbers as stream identifiers.
        return connectionError(PROTOCOL_ERROR, "GOAWAY with invalid last stream ID");
    } else if (lastStreamID >= nextID) {
        // "A server that is attempting to gracefully shut down a connection SHOULD
        // send an initial GOAWAY frame with the last stream identifier set to 2^31-1
        // and a NO_ERROR code."
        if (lastStreamID != Http2::lastValidStreamID || errorCode != HTTP2_NO_ERROR)
            return connectionError(PROTOCOL_ERROR, "GOAWAY invalid stream/error code");
    } else {
        lastStreamID += 2;
    }

    goingAway = true;

    // For the requests (and streams) we did not start yet, we have to report an
    // error.
    m_channel->emitFinishedWithError(QNetworkReply::ProtocolUnknownError,
                                     "GOAWAY received, cannot start a request");
    // Also, prevent further calls to sendRequest:
    m_channel->spdyRequestsToSend.clear();

    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QString message;
    qt_error(errorCode, error, message);

    // Even if the GOAWAY frame contains NO_ERROR we must send an error
    // when terminating streams to ensure users can distinguish from a
    // successful completion.
    if (errorCode == HTTP2_NO_ERROR) {
        error = QNetworkReply::ContentReSendError;
        message = QLatin1String("Server stopped accepting new streams before this stream was established");
    }

    for (quint32 id = lastStreamID; id < nextID; id += 2) {
        const auto it = activeStreams.find(id);
        if (it != activeStreams.end()) {
            Stream &stream = *it;
            finishStreamWithError(stream, error, message);
            markAsReset(id);
            deleteActiveStream(id);
        } else {
            removeFromSuspended(id);
        }
    }

    if (!activeStreams.size())
        closeSession();
}

void QHttp2ProtocolHandler::handleWINDOW_UPDATE()
{
    Q_ASSERT(inboundFrame.type() == FrameType::WINDOW_UPDATE);


    const quint32 delta = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    const bool valid = delta && delta <= quint32(std::numeric_limits<qint32>::max());
    const auto streamID = inboundFrame.streamID();

    if (streamID == Http2::connectionStreamID) {
        if (!valid || sum_will_overflow(sessionSendWindowSize, delta))
            return connectionError(PROTOCOL_ERROR, "WINDOW_UPDATE invalid delta");
        sessionSendWindowSize += delta;
    } else {
        if (!activeStreams.contains(streamID)) {
            // WINDOW_UPDATE on closed streams can be ignored.
            return;
        }
        auto &stream = activeStreams[streamID];
        if (!valid || sum_will_overflow(stream.sendWindow, delta)) {
            finishStreamWithError(stream, QNetworkReply::ProtocolFailure,
                                  QLatin1String("invalid WINDOW_UPDATE delta"));
            sendRST_STREAM(streamID, PROTOCOL_ERROR);
            markAsReset(streamID);
            deleteActiveStream(streamID);
            return;
        }
        stream.sendWindow += delta;
    }

    // Since we're in _q_receiveReply at the moment, let's first handle other
    // frames and resume suspended streams (if any) == start sending our own frame
    // after handling these frames, since one them can be e.g. GOAWAY.
    QMetaObject::invokeMethod(this, "resumeSuspendedStreams", Qt::QueuedConnection);
}

void QHttp2ProtocolHandler::handleCONTINUATION()
{
    Q_ASSERT(inboundFrame.type() == FrameType::CONTINUATION);
    Q_ASSERT(continuedFrames.size()); // HEADERS frame must be already in.

    if (inboundFrame.streamID() != continuedFrames.front().streamID())
        return connectionError(PROTOCOL_ERROR, "CONTINUATION on invalid stream");

    const bool endHeaders = inboundFrame.flags().testFlag(FrameFlag::END_HEADERS);
    continuedFrames.push_back(std::move(inboundFrame));

    if (!endHeaders)
        return;

    continuationExpected = false;
    handleContinuedHEADERS();
}

void QHttp2ProtocolHandler::handleContinuedHEADERS()
{
    // 'Continued' HEADERS can be: the initial HEADERS/PUSH_PROMISE frame
    // with/without END_HEADERS flag set plus, if no END_HEADERS flag,
    // a sequence of one or more CONTINUATION frames.
    Q_ASSERT(continuedFrames.size());
    const auto firstFrameType = continuedFrames[0].type();
    Q_ASSERT(firstFrameType == FrameType::HEADERS ||
             firstFrameType == FrameType::PUSH_PROMISE);

    const auto streamID = continuedFrames[0].streamID();

    if (firstFrameType == FrameType::HEADERS) {
        if (activeStreams.contains(streamID)) {
            Stream &stream = activeStreams[streamID];
            if (stream.state != Stream::halfClosedLocal
                && stream.state != Stream::remoteReserved
                && stream.state != Stream::open) {
                // We can receive HEADERS on streams initiated by our requests
                // (these streams are in halfClosedLocal or open state) or
                // remote-reserved streams from a server's PUSH_PROMISE.
                finishStreamWithError(stream, QNetworkReply::ProtocolFailure,
                                      QLatin1String("HEADERS on invalid stream"));
                sendRST_STREAM(streamID, CANCEL);
                markAsReset(streamID);
                deleteActiveStream(streamID);
                return;
            }
        } else if (!streamWasReset(streamID)) {
            return connectionError(PROTOCOL_ERROR, "HEADERS on invalid stream");
        }
        // Else: we cannot just ignore our peer's HEADERS frames - they change
        // HPACK context - even though the stream was reset; apparently the peer
        // has yet to see the reset.
    }

    std::vector<uchar> hpackBlock(assemble_hpack_block(continuedFrames));
    if (!hpackBlock.size()) {
        // It could be a PRIORITY sent in HEADERS - already handled by this
        // point in handleHEADERS. If it was PUSH_PROMISE (HTTP/2 8.2.1):
        // "The header fields in PUSH_PROMISE and any subsequent CONTINUATION
        // frames MUST be a valid and complete set of request header fields
        // (Section 8.1.2.3) ... If a client receives a PUSH_PROMISE that does
        // not include a complete and valid set of header fields or the :method
        // pseudo-header field identifies a method that is not safe, it MUST
        // respond with a stream error (Section 5.4.2) of type PROTOCOL_ERROR."
        if (firstFrameType == FrameType::PUSH_PROMISE)
            resetPromisedStream(continuedFrames[0], Http2::PROTOCOL_ERROR);

        return;
    }

    HPack::BitIStream inputStream{&hpackBlock[0], &hpackBlock[0] + hpackBlock.size()};
    if (!decoder.decodeHeaderFields(inputStream))
        return connectionError(COMPRESSION_ERROR, "HPACK decompression failed");

    switch (firstFrameType) {
    case FrameType::HEADERS:
        if (activeStreams.contains(streamID)) {
            Stream &stream = activeStreams[streamID];
            updateStream(stream, decoder.decodedHeader());
            // No DATA frames.
            if (continuedFrames[0].flags() & FrameFlag::END_STREAM) {
                finishStream(stream);
                deleteActiveStream(stream.streamID);
            }
        }
        break;
    case FrameType::PUSH_PROMISE:
        if (!tryReserveStream(continuedFrames[0], decoder.decodedHeader()))
            resetPromisedStream(continuedFrames[0], Http2::PROTOCOL_ERROR);
        break;
    default:
        break;
    }
}

bool QHttp2ProtocolHandler::acceptSetting(Http2::Settings identifier, quint32 newValue)
{
    if (identifier == Settings::HEADER_TABLE_SIZE_ID) {
        if (newValue > maxAcceptableTableSize) {
            connectionError(PROTOCOL_ERROR, "SETTINGS invalid table size");
            return false;
        }
        encoder.setMaxDynamicTableSize(newValue);
    }

    if (identifier == Settings::INITIAL_WINDOW_SIZE_ID) {
        // For every active stream - adjust its window
        // (and handle possible overflows as errors).
        if (newValue > quint32(std::numeric_limits<qint32>::max())) {
            connectionError(FLOW_CONTROL_ERROR, "SETTINGS invalid initial window size");
            return false;
        }

        const qint32 delta = qint32(newValue) - streamInitialSendWindowSize;
        streamInitialSendWindowSize = newValue;

        std::vector<quint32> brokenStreams;
        brokenStreams.reserve(activeStreams.size());
        for (auto &stream : activeStreams) {
            if (sum_will_overflow(stream.sendWindow, delta)) {
                brokenStreams.push_back(stream.streamID);
                continue;
            }
            stream.sendWindow += delta;
        }

        for (auto id : brokenStreams) {
            auto &stream = activeStreams[id];
            finishStreamWithError(stream, QNetworkReply::ProtocolFailure,
                                  QLatin1String("SETTINGS window overflow"));
            sendRST_STREAM(id, PROTOCOL_ERROR);
            markAsReset(id);
            deleteActiveStream(id);
        }

        QMetaObject::invokeMethod(this, "resumeSuspendedStreams", Qt::QueuedConnection);
    }

    if (identifier == Settings::MAX_CONCURRENT_STREAMS_ID) {
        if (newValue > maxPeerConcurrentStreams) {
            connectionError(PROTOCOL_ERROR, "SETTINGS invalid number of concurrent streams");
            return false;
        }
        maxConcurrentStreams = newValue;
    }

    if (identifier == Settings::MAX_FRAME_SIZE_ID) {
        if (newValue < Http2::minPayloadLimit || newValue > Http2::maxPayloadSize) {
            connectionError(PROTOCOL_ERROR, "SETTGINGS max frame size is out of range");
            return false;
        }
        maxFrameSize = newValue;
    }

    if (identifier == Settings::MAX_HEADER_LIST_SIZE_ID) {
        // We just remember this value, it can later
        // prevent us from sending any request (and this
        // will end up in request/reply error).
        maxHeaderListSize = newValue;
    }

    return true;
}

void QHttp2ProtocolHandler::updateStream(Stream &stream, const HPack::HttpHeader &headers,
                                         Qt::ConnectionType connectionType)
{
    const auto httpReply = stream.reply();
    const auto &httpRequest = stream.request();
    Q_ASSERT(httpReply || stream.state == Stream::remoteReserved);

    if (!httpReply) {
        // It's a PUSH_PROMISEd HEADERS, no actual request/reply
        // exists yet, we have to cache this data for a future
        // (potential) request.

        // TODO: the part with assignment is not especially cool
        // or beautiful, good that at least QByteArray is implicitly
        // sharing data. To be refactored (std::move).
        Q_ASSERT(promisedData.contains(stream.key));
        PushPromise &promise = promisedData[stream.key];
        promise.responseHeader = headers;
        return;
    }

    const auto httpReplyPrivate = httpReply->d_func();

    // For HTTP/1 'location' is handled (and redirect URL set) when a protocol
    // handler emits channel->allDone(). Http/2 protocol handler never emits
    // allDone, since we have many requests multiplexed in one channel at any
    // moment and we are probably not done yet. So we extract url and set it
    // here, if needed.
    int statusCode = 0;
    QUrl redirectUrl;

    for (const auto &pair : headers) {
        const auto &name = pair.name;
        auto value = pair.value;

        // TODO: part of this code copies what SPDY protocol handler does when
        // processing headers. Binary nature of HTTP/2 and SPDY saves us a lot
        // of parsing and related errors/bugs, but it would be nice to have
        // more detailed validation of headers.
        if (name == ":status") {
            statusCode = value.left(3).toInt();
            httpReply->setStatusCode(statusCode);
            httpReplyPrivate->reasonPhrase = QString::fromLatin1(value.mid(4));
        } else if (name == ":version") {
            httpReplyPrivate->majorVersion = value.at(5) - '0';
            httpReplyPrivate->minorVersion = value.at(7) - '0';
        } else if (name == "content-length") {
            bool ok = false;
            const qlonglong length = value.toLongLong(&ok);
            if (ok)
                httpReply->setContentLength(length);
        } else {
            if (name == "location")
                redirectUrl = QUrl::fromEncoded(value);
            QByteArray binder(", ");
            if (name == "set-cookie")
                binder = "\n";
            httpReplyPrivate->fields.append(qMakePair(name, value.replace('\0', binder)));
        }
    }

    if (QHttpNetworkReply::isHttpRedirect(statusCode) && redirectUrl.isValid())
        httpReply->setRedirectUrl(redirectUrl);

    if (httpReplyPrivate->isCompressed() && httpRequest.d->autoDecompress)
        httpReplyPrivate->removeAutoDecompressHeader();

    if (QHttpNetworkReply::isHttpRedirect(statusCode)
        || statusCode == 401 || statusCode == 407) {
        // These are the status codes that can trigger uploadByteDevice->reset()
        // in QHttpNetworkConnectionChannel::handleStatus. Alas, we have no
        // single request/reply, we multiplex several requests and thus we never
        // simply call 'handleStatus'. If we have byte-device - we try to reset
        // it here, we don't (and can't) handle any error during reset operation.
        if (stream.data())
            stream.data()->reset();
    }

    if (connectionType == Qt::DirectConnection)
        emit httpReply->headerChanged();
    else
        QMetaObject::invokeMethod(httpReply, "headerChanged", connectionType);
}

void QHttp2ProtocolHandler::updateStream(Stream &stream, const Frame &frame,
                                         Qt::ConnectionType connectionType)
{
    Q_ASSERT(frame.type() == FrameType::DATA);
    auto httpReply = stream.reply();
    Q_ASSERT(httpReply || stream.state == Stream::remoteReserved);

    if (!httpReply) {
        Q_ASSERT(promisedData.contains(stream.key));
        PushPromise &promise = promisedData[stream.key];
        // TODO: refactor this to use std::move.
        promise.dataFrames.push_back(frame);
        return;
    }

    if (const auto length = frame.dataSize()) {
        const char *data = reinterpret_cast<const char *>(frame.dataBegin());
        auto &httpRequest = stream.request();
        auto replyPrivate = httpReply->d_func();

        replyPrivate->totalProgress += length;

        const QByteArray wrapped(data, length);
        if (httpRequest.d->autoDecompress && replyPrivate->isCompressed()) {
            QByteDataBuffer inDataBuffer;
            inDataBuffer.append(wrapped);
            replyPrivate->uncompressBodyData(&inDataBuffer, &replyPrivate->responseData);
            // Now, make sure replyPrivate's destructor will properly clean up
            // buffers allocated (if any) by zlib.
            replyPrivate->autoDecompress = true;
        } else {
            replyPrivate->responseData.append(wrapped);
        }

        if (replyPrivate->shouldEmitSignals()) {
            if (connectionType == Qt::DirectConnection) {
                emit httpReply->readyRead();
                emit httpReply->dataReadProgress(replyPrivate->totalProgress,
                                                 replyPrivate->bodyLength);
            } else {
                QMetaObject::invokeMethod(httpReply, "readyRead", connectionType);
                QMetaObject::invokeMethod(httpReply, "dataReadProgress", connectionType,
                                          Q_ARG(qint64, replyPrivate->totalProgress),
                                          Q_ARG(qint64, replyPrivate->bodyLength));
            }
        }
    }
}

void QHttp2ProtocolHandler::finishStream(Stream &stream, Qt::ConnectionType connectionType)
{
    Q_ASSERT(stream.state == Stream::remoteReserved || stream.reply());

    stream.state = Stream::closed;
    auto httpReply = stream.reply();
    if (httpReply) {
        httpReply->disconnect(this);
        if (stream.data())
            stream.data()->disconnect(this);

        if (connectionType == Qt::DirectConnection)
            emit httpReply->finished();
        else
            QMetaObject::invokeMethod(httpReply, "finished", connectionType);
    }

    qCDebug(QT_HTTP2) << "stream" << stream.streamID << "closed";
}

void QHttp2ProtocolHandler::finishStreamWithError(Stream &stream, quint32 errorCode)
{
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QString message;
    qt_error(errorCode, error, message);
    finishStreamWithError(stream, error, message);
}

void QHttp2ProtocolHandler::finishStreamWithError(Stream &stream, QNetworkReply::NetworkError error,
                                                  const QString &message)
{
    Q_ASSERT(stream.state == Stream::remoteReserved || stream.reply());

    stream.state = Stream::closed;
    if (auto httpReply = stream.reply()) {
        httpReply->disconnect(this);
        if (stream.data())
            stream.data()->disconnect(this);

        // TODO: error message must be translated!!! (tr)
        emit httpReply->finishedWithError(error, message);
    }

    qCWarning(QT_HTTP2) << "stream" << stream.streamID
                        << "finished with error:" << message;
}

quint32 QHttp2ProtocolHandler::createNewStream(const HttpMessagePair &message, bool uploadDone)
{
    const qint32 newStreamID = allocateStreamID();
    if (!newStreamID)
        return 0;

    Q_ASSERT(!activeStreams.contains(newStreamID));

    const auto reply = message.second;
    const auto replyPrivate = reply->d_func();
    replyPrivate->connection = m_connection;
    replyPrivate->connectionChannel = m_channel;
    reply->setSpdyWasUsed(true);
    streamIDs.insert(reply, newStreamID);
    connect(reply, SIGNAL(destroyed(QObject*)),
            this, SLOT(_q_replyDestroyed(QObject*)));

    const Stream newStream(message, newStreamID,
                           streamInitialSendWindowSize,
                           streamInitialReceiveWindowSize);

    if (!uploadDone) {
        if (auto src = newStream.data()) {
            connect(src, SIGNAL(readyRead()), this,
                    SLOT(_q_uploadDataReadyRead()), Qt::QueuedConnection);
            connect(src, &QHttp2ProtocolHandler::destroyed,
                    this, &QHttp2ProtocolHandler::_q_uploadDataDestroyed);
            streamIDs.insert(src, newStreamID);
        }
    }

    activeStreams.insert(newStreamID, newStream);

    return newStreamID;
}

void QHttp2ProtocolHandler::addToSuspended(Stream &stream)
{
    qCDebug(QT_HTTP2) << "stream" << stream.streamID
                      << "suspended by flow control";
    const auto priority = stream.priority();
    Q_ASSERT(int(priority) >= 0 && int(priority) < 3);
    suspendedStreams[priority].push_back(stream.streamID);
}

void QHttp2ProtocolHandler::markAsReset(quint32 streamID)
{
    Q_ASSERT(streamID);

    qCDebug(QT_HTTP2) << "stream" << streamID << "was reset";
    // This part is quite tricky: I have to clear this set
    // so that it does not become tOOO big.
    if (recycledStreams.size() > maxRecycledStreams) {
        // At least, I'm erasing the oldest first ...
        recycledStreams.erase(recycledStreams.begin(),
                              recycledStreams.begin() +
                              recycledStreams.size() / 2);
    }

    const auto it = std::lower_bound(recycledStreams.begin(), recycledStreams.end(),
                                     streamID);
    if (it != recycledStreams.end() && *it == streamID)
        return;

    recycledStreams.insert(it, streamID);
}

quint32 QHttp2ProtocolHandler::popStreamToResume()
{
    quint32 streamID = connectionStreamID;
    using QNR = QHttpNetworkRequest;
    const QNR::Priority ranks[] = {QNR::HighPriority,
                                   QNR::NormalPriority,
                                   QNR::LowPriority};

    for (const QNR::Priority rank : ranks) {
        auto &queue = suspendedStreams[rank];
        auto it = queue.begin();
        for (; it != queue.end(); ++it) {
            if (!activeStreams.contains(*it))
                continue;
            if (activeStreams[*it].sendWindow > 0)
                break;
        }

        if (it != queue.end()) {
            streamID = *it;
            queue.erase(it);
            break;
        }
    }

    return streamID;
}

void QHttp2ProtocolHandler::removeFromSuspended(quint32 streamID)
{
    for (auto &q : suspendedStreams) {
        q.erase(std::remove(q.begin(), q.end(), streamID), q.end());
    }
}

void QHttp2ProtocolHandler::deleteActiveStream(quint32 streamID)
{
    if (activeStreams.contains(streamID)) {
        auto &stream = activeStreams[streamID];
        if (stream.reply()) {
            stream.reply()->disconnect(this);
            streamIDs.remove(stream.reply());
        }
        if (stream.data()) {
            stream.data()->disconnect(this);
            streamIDs.remove(stream.data());
        }
        activeStreams.remove(streamID);
    }

    removeFromSuspended(streamID);
    if (m_channel->spdyRequestsToSend.size())
        QMetaObject::invokeMethod(this, "sendRequest", Qt::QueuedConnection);
}

bool QHttp2ProtocolHandler::streamWasReset(quint32 streamID) const
{
    const auto it = std::lower_bound(recycledStreams.begin(),
                                     recycledStreams.end(),
                                     streamID);
    return it != recycledStreams.end() && *it == streamID;
}

void QHttp2ProtocolHandler::resumeSuspendedStreams()
{
    while (sessionSendWindowSize > 0) {
        const auto streamID = popStreamToResume();
        if (!streamID)
            return;

        if (!activeStreams.contains(streamID))
            continue;

        Stream &stream = activeStreams[streamID];
        if (!sendDATA(stream)) {
            finishStreamWithError(stream, QNetworkReply::UnknownNetworkError,
                                  QLatin1String("failed to send DATA"));
            sendRST_STREAM(streamID, INTERNAL_ERROR);
            markAsReset(streamID);
            deleteActiveStream(streamID);
        }
    }
}

quint32 QHttp2ProtocolHandler::allocateStreamID()
{
    // With protocol upgrade streamID == 1 will become
    // invalid. The logic must be updated.
    if (nextID > Http2::lastValidStreamID)
        return 0;

    const quint32 streamID = nextID;
    nextID += 2;

    return streamID;
}

bool QHttp2ProtocolHandler::tryReserveStream(const Http2::Frame &pushPromiseFrame,
                                             const HPack::HttpHeader &requestHeader)
{
    Q_ASSERT(pushPromiseFrame.type() == FrameType::PUSH_PROMISE);

    QMap<QByteArray, QByteArray> pseudoHeaders;
    for (const auto &field : requestHeader) {
        if (field.name == ":scheme" || field.name == ":path"
            || field.name == ":authority" || field.name == ":method") {
            if (field.value.isEmpty() || pseudoHeaders.contains(field.name))
                return false;
            pseudoHeaders[field.name] = field.value;
        }
    }

    if (pseudoHeaders.size() != 4) {
        // All four required, HTTP/2 8.1.2.3.
        return false;
    }

    const QByteArray method = pseudoHeaders[":method"];
    if (method.compare("get", Qt::CaseInsensitive) != 0 &&
            method.compare("head", Qt::CaseInsensitive) != 0)
        return false;

    QUrl url;
    url.setScheme(QLatin1String(pseudoHeaders[":scheme"]));
    url.setAuthority(QLatin1String(pseudoHeaders[":authority"]));
    url.setPath(QLatin1String(pseudoHeaders[":path"]));

    if (!url.isValid())
        return false;

    Q_ASSERT(activeStreams.contains(pushPromiseFrame.streamID()));
    const Stream &associatedStream = activeStreams[pushPromiseFrame.streamID()];

    const auto associatedUrl = urlkey_from_request(associatedStream.request());
    if (url.adjusted(QUrl::RemovePath) != associatedUrl.adjusted(QUrl::RemovePath))
        return false;

    const auto urlKey = url.toString();
    if (promisedData.contains(urlKey)) // duplicate push promise
        return false;

    const auto reservedID = qFromBigEndian<quint32>(pushPromiseFrame.dataBegin());
    // By this time all sanity checks on reservedID were done already
    // in handlePUSH_PROMISE. We do not repeat them, only those below:
    Q_ASSERT(!activeStreams.contains(reservedID));
    Q_ASSERT(!streamWasReset(reservedID));

    auto &promise = promisedData[urlKey];
    promise.reservedID = reservedID;
    promise.pushHeader = requestHeader;

    activeStreams.insert(reservedID, Stream(urlKey, reservedID, streamInitialReceiveWindowSize));
    return true;
}

void QHttp2ProtocolHandler::resetPromisedStream(const Frame &pushPromiseFrame,
                                                Http2::Http2Error reason)
{
    Q_ASSERT(pushPromiseFrame.type() == FrameType::PUSH_PROMISE);
    const auto reservedID = qFromBigEndian<quint32>(pushPromiseFrame.dataBegin());
    sendRST_STREAM(reservedID, reason);
    markAsReset(reservedID);
}

void QHttp2ProtocolHandler::initReplyFromPushPromise(const HttpMessagePair &message,
                                                     const QString &cacheKey)
{
    Q_ASSERT(promisedData.contains(cacheKey));
    auto promise = promisedData.take(cacheKey);
    Q_ASSERT(message.second);
    message.second->setSpdyWasUsed(true);

    qCDebug(QT_HTTP2) << "found cached/promised response on stream" << promise.reservedID;

    bool replyFinished = false;
    Stream *promisedStream = nullptr;
    if (activeStreams.contains(promise.reservedID)) {
        promisedStream = &activeStreams[promise.reservedID];
        // Ok, we have an active (not closed yet) stream waiting for more frames,
        // let's pretend we requested it:
        promisedStream->httpPair = message;
    } else {
        // Let's pretent we're sending a request now:
        Stream closedStream(message, promise.reservedID,
                            streamInitialSendWindowSize,
                            streamInitialReceiveWindowSize);
        closedStream.state = Stream::halfClosedLocal;
        activeStreams.insert(promise.reservedID, closedStream);
        promisedStream = &activeStreams[promise.reservedID];
        replyFinished = true;
    }

    Q_ASSERT(promisedStream);

    if (!promise.responseHeader.empty())
        updateStream(*promisedStream, promise.responseHeader, Qt::QueuedConnection);

    for (const auto &frame : promise.dataFrames)
        updateStream(*promisedStream, frame, Qt::QueuedConnection);

    if (replyFinished) {
        // Good, we already have received ALL the frames of that PUSH_PROMISE,
        // nothing more to do.
        finishStream(*promisedStream, Qt::QueuedConnection);
        deleteActiveStream(promisedStream->streamID);
    }
}

void QHttp2ProtocolHandler::connectionError(Http2::Http2Error errorCode,
                                            const char *message)
{
    Q_ASSERT(message);
    Q_ASSERT(!goingAway);

    qCCritical(QT_HTTP2) << "connection error:" << message;

    goingAway = true;
    sendGOAWAY(errorCode);
    const auto error = qt_error(errorCode);
    m_channel->emitFinishedWithError(error, message);

    for (auto &stream: activeStreams)
        finishStreamWithError(stream, error, QLatin1String(message));

    closeSession();
}

void QHttp2ProtocolHandler::closeSession()
{
    activeStreams.clear();
    for (auto &q: suspendedStreams)
        q.clear();
    recycledStreams.clear();

    m_channel->close();
}

QT_END_NAMESPACE
