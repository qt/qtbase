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

#if !defined(QT_NO_HTTP) && !defined(QT_NO_SSL)

#include "http2/bitstreams_p.h"

#include <private/qnoncontiguousbytedevice_p.h>

#include <QtNetwork/qabstractsocket.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qendian.h>
#include <QtCore/qdebug.h>
#include <QtCore/qlist.h>
#include <QtCore/qurl.h>

#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

namespace
{

HPack::HttpHeader build_headers(const QHttpNetworkRequest &request, quint32 maxHeaderListSize)
{
    using namespace HPack;

    HttpHeader header;
    header.reserve(300);

    // 1. Before anything - mandatory fields, if they do not fit into maxHeaderList -
    // then stop immediately with error.
    const auto auth = request.url().authority(QUrl::FullyEncoded | QUrl::RemoveUserInfo).toLatin1();
    header.push_back(HeaderField(":authority", auth));
    header.push_back(HeaderField(":method", request.methodName()));
    header.push_back(HeaderField(":path", request.uri(false)));
    header.push_back(HeaderField(":scheme", request.url().scheme().toLatin1()));

    HeaderSize size = header_size(header);
    if (!size.first) // Ooops!
        return HttpHeader();

    if (size.second > maxHeaderListSize)
        return HttpHeader(); // Bad, we cannot send this request ...

    for (const auto &field : request.header()) {
        const HeaderSize delta = entry_size(field.first, field.second);
        if (!delta.first) // Overflow???
            break;
        if (std::numeric_limits<quint32>::max() - delta.second < size.second)
            break;
        size.second += delta.second;
        if (size.second > maxHeaderListSize)
            break;

        QByteArray key(field.first.toLower());
        if (key == "connection" || key == "host" || key == "keep-alive"
            || key == "proxy-connection" || key == "transfer-encoding")
            continue; // Those headers are not valid (section 3.2.1) - from QSpdyProtocolHandler
        // TODO: verify with specs, which fields are valid to send ....
        // toLower - 8.1.2 .... "header field names MUST be converted to lowercase prior
        // to their encoding in HTTP/2.
        // A request or response containing uppercase header field names
        // MUST be treated as malformed (Section 8.1.2.6)".
        header.push_back(HeaderField(key, field.second));
    }

    return header;
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
const qint32 QHttp2ProtocolHandler::sessionMaxRecvWindowSize;
const qint32 QHttp2ProtocolHandler::streamInitialRecvWindowSize;
const quint32 QHttp2ProtocolHandler::maxAcceptableTableSize;

QHttp2ProtocolHandler::QHttp2ProtocolHandler(QHttpNetworkConnectionChannel *channel)
    : QAbstractProtocolHandler(channel),
      decoder(HPack::FieldLookupTable::DefaultSize),
      encoder(HPack::FieldLookupTable::DefaultSize, true)
{
    continuedFrames.reserve(20);
}

void QHttp2ProtocolHandler::_q_uploadDataReadyRead()
{
    auto data = qobject_cast<QNonContiguousByteDevice *>(sender());
    Q_ASSERT(data);
    const qint32 streamID = data->property("HTTP2StreamID").toInt();
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
    const quint32 streamID = reply->property("HTTP2StreamID").toInt();
    if (activeStreams.contains(streamID)) {
        sendRST_STREAM(streamID, CANCEL);
        markAsReset(streamID);
        deleteActiveStream(streamID);
    }
}

void QHttp2ProtocolHandler::_q_readyRead()
{
    _q_receiveReply();
}

void QHttp2ProtocolHandler::_q_receiveReply()
{
    Q_ASSERT(m_socket);
    Q_ASSERT(m_channel);

    do {
        const auto result = inboundFrame.read(*m_socket);
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

        if (continuationExpected && inboundFrame.type != FrameType::CONTINUATION)
            return connectionError(PROTOCOL_ERROR, "CONTINUATION expected");

        switch (inboundFrame.type) {
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
    } while (!goingAway || activeStreams.size());
}

bool QHttp2ProtocolHandler::sendRequest()
{
    if (goingAway)
        return false;

    if (!prefaceSent && !sendClientPreface())
        return false;

    auto &requests = m_channel->spdyRequestsToSend;
    if (!requests.size())
        return true;

    const auto streamsToUse = std::min<quint32>(maxConcurrentStreams - activeStreams.size(),
                                                requests.size());
    auto it = requests.begin();
    m_channel->state = QHttpNetworkConnectionChannel::WritingState;
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

    const qint64 written = m_socket->write(Http2clientPreface,
                                           Http2::clientPrefaceLength);
    if (written != Http2::clientPrefaceLength)
        return false;

    // 6.5 SETTINGS
    outboundFrame.start(FrameType::SETTINGS, FrameFlag::EMPTY, Http2::connectionStreamID);
    // MAX frame size (16 kb), disable PUSH
    outboundFrame.append(Settings::MAX_FRAME_SIZE_ID);
    outboundFrame.append(quint32(Http2::maxFrameSize));
    outboundFrame.append(Settings::ENABLE_PUSH_ID);
    outboundFrame.append(quint32(0));

    if (!outboundFrame.write(*m_socket))
        return false;

    sessionRecvWindowSize = sessionMaxRecvWindowSize;
    if (defaultSessionWindowSize < sessionMaxRecvWindowSize) {
        const auto delta = sessionMaxRecvWindowSize - defaultSessionWindowSize;
        if (!sendWINDOW_UPDATE(connectionStreamID, delta))
            return false;
    }

    prefaceSent = true;
    waitingForSettingsACK = true;

    return true;
}

bool QHttp2ProtocolHandler::sendSETTINGS_ACK()
{
    Q_ASSERT(m_socket);

    if (!prefaceSent && !sendClientPreface())
        return false;

    outboundFrame.start(FrameType::SETTINGS, FrameFlag::ACK, Http2::connectionStreamID);

    return outboundFrame.write(*m_socket);
}

bool QHttp2ProtocolHandler::sendHEADERS(Stream &stream)
{
    using namespace HPack;

    outboundFrame.start(FrameType::HEADERS, FrameFlag::PRIORITY | FrameFlag::END_HEADERS,
                        stream.streamID);

    if (!stream.data()) {
        outboundFrame.addFlag(FrameFlag::END_STREAM);
        stream.state = Stream::halfClosedLocal;
    } else {
        stream.state = Stream::open;
    }

    outboundFrame.append(quint32()); // No stream dependency in Qt.
    outboundFrame.append(stream.weight());

    const auto headers = build_headers(stream.request(), maxHeaderListSize);
    if (!headers.size()) // nothing fits into maxHeaderListSize
        return false;

    // Compress in-place:
    BitOStream outputStream(outboundFrame.frameBuffer);
    if (!encoder.encodeRequest(outputStream, headers))
        return false;

    return outboundFrame.writeHEADERS(*m_socket, maxFrameSize);
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

        outboundFrame.start(FrameType::DATA, FrameFlag::EMPTY, stream.streamID);
        const qint32 bytesWritten = std::min<qint32>(slot, chunkSize);

        if (!outboundFrame.writeDATA(*m_socket, maxFrameSize, src, bytesWritten))
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
        outboundFrame.start(FrameType::DATA, FrameFlag::END_STREAM, stream.streamID);
        outboundFrame.setPayloadSize(0);
        outboundFrame.write(*m_socket);
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

    outboundFrame.start(FrameType::WINDOW_UPDATE, FrameFlag::EMPTY, streamID);
    outboundFrame.append(delta);
    return outboundFrame.write(*m_socket);
}

bool QHttp2ProtocolHandler::sendRST_STREAM(quint32 streamID, quint32 errorCode)
{
    Q_ASSERT(m_socket);

    outboundFrame.start(FrameType::RST_STREAM, FrameFlag::EMPTY, streamID);
    outboundFrame.append(errorCode);
    return outboundFrame.write(*m_socket);
}

bool QHttp2ProtocolHandler::sendGOAWAY(quint32 errorCode)
{
    Q_ASSERT(m_socket);

    outboundFrame.start(FrameType::GOAWAY, FrameFlag::EMPTY, connectionStreamID);
    outboundFrame.append(quint32(connectionStreamID));
    outboundFrame.append(errorCode);
    return outboundFrame.write(*m_socket);
}

void QHttp2ProtocolHandler::handleDATA()
{
    Q_ASSERT(inboundFrame.type == FrameType::DATA);

    const auto streamID = inboundFrame.streamID;
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "DATA on stream 0x0");

    if (!activeStreams.contains(streamID) && !streamWasReset(streamID))
        return connectionError(ENHANCE_YOUR_CALM, "DATA on invalid stream");

    if (qint32(inboundFrame.payloadSize) > sessionRecvWindowSize)
        return connectionError(FLOW_CONTROL_ERROR, "Flow control error");

    sessionRecvWindowSize -= inboundFrame.payloadSize;

    if (activeStreams.contains(streamID)) {
        auto &stream = activeStreams[streamID];

        if (qint32(inboundFrame.payloadSize) > stream.recvWindow) {
            finishStreamWithError(stream, QNetworkReply::ProtocolInvalidOperationError,
                                  QLatin1String("flow control error"));
            sendRST_STREAM(streamID, FLOW_CONTROL_ERROR);
            markAsReset(streamID);
            deleteActiveStream(streamID);
        } else {
            stream.recvWindow -= inboundFrame.payloadSize;
            // Uncompress data if needed and append it ...
            updateStream(stream, inboundFrame);

            if (inboundFrame.flags.testFlag(FrameFlag::END_STREAM)) {
                finishStream(stream);
                deleteActiveStream(stream.streamID);
            } else if (stream.recvWindow < streamInitialRecvWindowSize / 2) {
                QMetaObject::invokeMethod(this, "sendWINDOW_UPDATE", Qt::QueuedConnection,
                                          Q_ARG(quint32, stream.streamID),
                                          Q_ARG(quint32, streamInitialRecvWindowSize - stream.recvWindow));
                stream.recvWindow = streamInitialRecvWindowSize;
            }
        }
    }

    if (sessionRecvWindowSize < sessionMaxRecvWindowSize / 2) {
        QMetaObject::invokeMethod(this, "sendWINDOW_UPDATE", Qt::QueuedConnection,
                                  Q_ARG(quint32, connectionStreamID),
                                  Q_ARG(quint32, sessionMaxRecvWindowSize - sessionRecvWindowSize));
        sessionRecvWindowSize = sessionMaxRecvWindowSize;
    }
}

void QHttp2ProtocolHandler::handleHEADERS()
{
    Q_ASSERT(inboundFrame.type == FrameType::HEADERS);

    const auto streamID = inboundFrame.streamID;
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "HEADERS on 0x0 stream");

    if (!activeStreams.contains(streamID) && !streamWasReset(streamID))
        return connectionError(ENHANCE_YOUR_CALM, "HEADERS on invalid stream");

    if (inboundFrame.flags.testFlag(FrameFlag::PRIORITY)) {
        handlePRIORITY();
        if (goingAway)
            return;
    }

    const bool endHeaders = inboundFrame.flags.testFlag(FrameFlag::END_HEADERS);
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
    Q_ASSERT(inboundFrame.type == FrameType::PRIORITY ||
             inboundFrame.type == FrameType::HEADERS);

    const auto streamID = inboundFrame.streamID;
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
    Q_ASSERT(inboundFrame.type == FrameType::RST_STREAM);

    // "RST_STREAM frames MUST be associated with a stream.
    // If a RST_STREAM frame is received with a stream identifier of 0x0,
    // the recipient MUST treat this as a connection error (Section 5.4.1)
    // of type PROTOCOL_ERROR.
    if (inboundFrame.streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "RST_STREAM on 0x0");

    if (!(inboundFrame.streamID & 0x1)) {
        // RST_STREAM on a promised stream:
        // since we do not keep track of such streams,
        // just ignore.
        return;
    }

    if (inboundFrame.streamID >= nextID) {
        // "RST_STREAM frames MUST NOT be sent for a stream
        // in the "idle" state. .. the recipient MUST treat this
        // as a connection error (Section 5.4.1) of type PROTOCOL_ERROR."
        return connectionError(PROTOCOL_ERROR, "RST_STREAM on idle stream");
    }

    if (!activeStreams.contains(inboundFrame.streamID)) {
        // 'closed' stream, ignore.
        return;
    }

    Q_ASSERT(inboundFrame.dataSize() == 4);

    Stream &stream = activeStreams[inboundFrame.streamID];
    finishStreamWithError(stream, qFromBigEndian<quint32>(inboundFrame.dataBegin()));
    markAsReset(stream.streamID);
    deleteActiveStream(stream.streamID);
}

void QHttp2ProtocolHandler::handleSETTINGS()
{
    // 6.5 SETTINGS.
    Q_ASSERT(inboundFrame.type == FrameType::SETTINGS);

    if (inboundFrame.streamID != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "SETTINGS on invalid stream");

    if (inboundFrame.flags.testFlag(FrameFlag::ACK)) {
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
    Q_ASSERT(inboundFrame.type == FrameType::PUSH_PROMISE);

    if (prefaceSent && !waitingForSettingsACK) {
        // This means, server ACKed our 'NO PUSH',
        // but sent us PUSH_PROMISE anyway.
        return connectionError(PROTOCOL_ERROR, "unexpected PUSH_PROMISE frame");
    }

    const auto streamID = inboundFrame.streamID;
    if (streamID == connectionStreamID) {
        return connectionError(PROTOCOL_ERROR,
                               "PUSH_PROMISE with invalid associated stream (0x0)");
    }

    if (!activeStreams.contains(streamID) && !streamWasReset(streamID)) {
        return connectionError(ENHANCE_YOUR_CALM,
                               "PUSH_PROMISE with invalid associated stream");
    }

    const auto reservedID = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    if (!reservedID || (reservedID & 0x1)) {
        return connectionError(PROTOCOL_ERROR,
                               "PUSH_PROMISE with invalid promised stream ID");
    }

    // "ignoring a PUSH_PROMISE frame causes the stream
    // state to become indeterminate" - let's RST_STREAM it then ...
    sendRST_STREAM(reservedID, REFUSE_STREAM);
    markAsReset(reservedID);

    const bool endHeaders = inboundFrame.flags.testFlag(FrameFlag::END_HEADERS);
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
    Q_ASSERT(inboundFrame.type == FrameType::PING);
    Q_ASSERT(m_socket);

    if (inboundFrame.streamID != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "PING on invalid stream");

    if (inboundFrame.flags & FrameFlag::ACK)
        return connectionError(PROTOCOL_ERROR, "unexpected PING ACK");

    Q_ASSERT(inboundFrame.dataSize() == 8);

    outboundFrame.start(FrameType::PING, FrameFlag::ACK, connectionStreamID);
    outboundFrame.append(inboundFrame.dataBegin(), inboundFrame.dataBegin() + 8);
    outboundFrame.write(*m_socket);
}

void QHttp2ProtocolHandler::handleGOAWAY()
{
    // 6.8 GOAWAY

    Q_ASSERT(inboundFrame.type == FrameType::GOAWAY);
    // "An endpoint MUST treat a GOAWAY frame with a stream identifier
    // other than 0x0 as a connection error (Section 5.4.1) of type PROTOCOL_ERROR."
    if (inboundFrame.streamID != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "GOAWAY on invalid stream");

    const auto src = inboundFrame.dataBegin();
    quint32 lastStreamID = qFromBigEndian<quint32>(src);
    const quint32 errorCode = qFromBigEndian<quint32>(src + 4);

    if (!lastStreamID) {
        // "The last stream identifier can be set to 0 if no
        // streams were processed."
        lastStreamID = 1;
    }

    if (!(lastStreamID & 0x1)) {
        // 5.1.1 - we (client) use only odd numbers as stream identifiers.
        return connectionError(PROTOCOL_ERROR, "GOAWAY with invalid last stream ID");
    }

    if (lastStreamID >= nextID) {
        // "A server that is attempting to gracefully shut down a connection SHOULD
        // send an initial GOAWAY frame with the last stream identifier set to 2^31-1
        // and a NO_ERROR code."
        if (lastStreamID != (quint32(1) << 31) - 1 || errorCode != HTTP2_NO_ERROR)
            return connectionError(PROTOCOL_ERROR, "GOAWAY invalid stream/error code");
        lastStreamID = 1;
    } else {
        lastStreamID += 2;
    }

    goingAway = true;

    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QString message;
    qt_error(errorCode, error, message);

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
    Q_ASSERT(inboundFrame.type == FrameType::WINDOW_UPDATE);


    const quint32 delta = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    const bool valid = delta && delta <= quint32(std::numeric_limits<qint32>::max());
    const auto streamID = inboundFrame.streamID;

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
            finishStreamWithError(stream, QNetworkReply::ProtocolInvalidOperationError,
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
    Q_ASSERT(inboundFrame.type == FrameType::CONTINUATION);
    Q_ASSERT(continuedFrames.size()); // HEADERS frame must be already in.

    if (inboundFrame.streamID != continuedFrames.front().streamID)
        return connectionError(PROTOCOL_ERROR, "CONTINUATION on invalid stream");

    const bool endHeaders = inboundFrame.flags.testFlag(FrameFlag::END_HEADERS);
    continuedFrames.push_back(std::move(inboundFrame));

    if (!endHeaders)
        return;

    continuationExpected = false;
    handleContinuedHEADERS();
}

void QHttp2ProtocolHandler::handleContinuedHEADERS()
{
    Q_ASSERT(continuedFrames.size());

    const auto streamID = continuedFrames[0].streamID;

    if (continuedFrames[0].type == FrameType::HEADERS) {
        if (activeStreams.contains(streamID)) {
            Stream &stream = activeStreams[streamID];
            if (stream.state != Stream::halfClosedLocal) {
                // If we're receiving headers, they're a response to a request we sent;
                // and we closed our end when we finished sending that.
                finishStreamWithError(stream, QNetworkReply::ProtocolInvalidOperationError,
                                      QLatin1String("HEADERS on invalid stream"));
                sendRST_STREAM(streamID, CANCEL);
                markAsReset(streamID);
                deleteActiveStream(streamID);
                return;
            }
        } else if (!streamWasReset(streamID)) {
            return connectionError(PROTOCOL_ERROR, "HEADERS on invalid stream");
        }
    }

    quint32 total = 0;
    for (const auto &frame : continuedFrames)
        total += frame.dataSize();

    if (!total) {
        // It could be a PRIORITY sent in HEADERS - handled by this point.
        return;
    }

    std::vector<uchar> hpackBlock(total);
    auto dst = hpackBlock.begin();
    for (const auto &frame : continuedFrames) {
        if (!frame.dataSize())
            continue;
        const uchar *src = frame.dataBegin();
        std::copy(src, src + frame.dataSize(), dst);
        dst += frame.dataSize();
    }

    HPack::BitIStream inputStream{&hpackBlock[0],
                                  &hpackBlock[0] + hpackBlock.size()};

    if (!decoder.decodeHeaderFields(inputStream))
        return connectionError(COMPRESSION_ERROR, "HPACK decompression failed");

    if (continuedFrames[0].type == FrameType::HEADERS) {
        if (activeStreams.contains(streamID)) {
            Stream &stream = activeStreams[streamID];
            updateStream(stream, decoder.decodedHeader());
            if (continuedFrames[0].flags & FrameFlag::END_STREAM) {
                finishStream(stream);
                deleteActiveStream(stream.streamID);
            }
        }
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
            finishStreamWithError(stream, QNetworkReply::ProtocolInvalidOperationError,
                                  QLatin1String("SETTINGS window overflow"));
            sendRST_STREAM(id, PROTOCOL_ERROR);
            markAsReset(id);
            deleteActiveStream(id);
        }

        QMetaObject::invokeMethod(this, "resumeSuspendedStreams", Qt::QueuedConnection);
    }

    if (identifier == Settings::MAX_CONCURRENT_STREAMS_ID) {
        if (maxConcurrentStreams > maxPeerConcurrentStreams) {
            connectionError(PROTOCOL_ERROR, "SETTINGS invalid number of concurrent streams");
            return false;
        }
        maxConcurrentStreams = newValue;
    }

    if (identifier == Settings::MAX_FRAME_SIZE_ID) {
        if (newValue < Http2::maxFrameSize || newValue > Http2::maxPayloadSize) {
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

void QHttp2ProtocolHandler::updateStream(Stream &stream, const HPack::HttpHeader &headers)
{
    const auto httpReply = stream.reply();
    Q_ASSERT(httpReply);
    const auto httpReplyPrivate = httpReply->d_func();
    for (const auto &pair : headers) {
        const auto &name = pair.name;
        auto value = pair.value;

        if (name == ":status") {
            httpReply->setStatusCode(value.left(3).toInt());
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
            QByteArray binder(", ");
            if (name == "set-cookie")
                binder = "\n";
            httpReply->setHeaderField(name, value.replace('\0', binder));
        }
    }

    emit httpReply->headerChanged();
}

void QHttp2ProtocolHandler::updateStream(Stream &stream, const Http2::FrameReader &frame)
{
    Q_ASSERT(frame.type == FrameType::DATA);

    if (const auto length = frame.dataSize()) {
        const char *data = reinterpret_cast<const char *>(frame.dataBegin());
        auto &httpRequest = stream.request();
        auto httpReply = stream.reply();
        Q_ASSERT(httpReply);
        auto replyPrivate = httpReply->d_func();

        replyPrivate->compressedData.append(data, length);
        replyPrivate->totalProgress += length;

        const QByteArray wrapped(data, length);
        if (httpRequest.d->autoDecompress && replyPrivate->isCompressed()) {
            QByteDataBuffer inDataBuffer;
            inDataBuffer.append(wrapped);
            replyPrivate->uncompressBodyData(&inDataBuffer, &replyPrivate->responseData);
        } else {
            replyPrivate->responseData.append(wrapped);
        }

        if (replyPrivate->shouldEmitSignals()) {
            emit httpReply->readyRead();
            emit httpReply->dataReadProgress(replyPrivate->totalProgress, replyPrivate->bodyLength);
        }
    }
}

void QHttp2ProtocolHandler::finishStream(Stream &stream)
{
    stream.state = Stream::closed;
    auto httpReply = stream.reply();
    Q_ASSERT(httpReply);
    httpReply->disconnect(this);
    if (stream.data())
        stream.data()->disconnect(this);

    qCDebug(QT_HTTP2) << "stream" << stream.streamID << "closed";

    emit httpReply->finished();
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
    stream.state = Stream::closed;
    auto httpReply = stream.reply();
    Q_ASSERT(httpReply);
    httpReply->disconnect(this);
    if (stream.data())
        stream.data()->disconnect(this);

    qCWarning(QT_HTTP2) << "stream" << stream.streamID
                        << "finished with error:" << message;

    // TODO: error message must be translated!!! (tr)
    emit httpReply->finishedWithError(error, message);
}

quint32 QHttp2ProtocolHandler::createNewStream(const HttpMessagePair &message)
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
    reply->setProperty("HTTP2StreamID", newStreamID);
    connect(reply, SIGNAL(destroyed(QObject*)),
            this, SLOT(_q_replyDestroyed(QObject*)));

    const Stream newStream(message, newStreamID,
                           streamInitialSendWindowSize,
                           streamInitialRecvWindowSize);

    if (auto src = newStream.data()) {
        connect(src, SIGNAL(readyRead()), this,
                SLOT(_q_uploadDataReadyRead()), Qt::QueuedConnection);
        src->setProperty("HTTP2StreamID", newStreamID);
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
    // For now, we trace only client's streams (created by us,
    // odd integer numbers).
    if (streamID & 0x1) {
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
}

quint32 QHttp2ProtocolHandler::popStreamToResume()
{
    quint32 streamID = connectionStreamID;
    const int nQ = sizeof suspendedStreams / sizeof suspendedStreams[0];
    using QNR = QHttpNetworkRequest;
    const QNR::Priority ranks[nQ] = {QNR::HighPriority,
                                     QNR::NormalPriority,
                                     QNR::LowPriority};

    for (int i = 0; i < nQ; ++i) {
        auto &queue = suspendedStreams[ranks[i]];
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
    const int nQ = sizeof suspendedStreams / sizeof suspendedStreams[0];
    for (int i = 0; i < nQ; ++i) {
        auto &q = suspendedStreams[i];
        q.erase(std::remove(q.begin(), q.end(), streamID), q.end());
    }
}

void QHttp2ProtocolHandler::deleteActiveStream(quint32 streamID)
{
    if (activeStreams.contains(streamID)) {
        auto &stream = activeStreams[streamID];
        if (stream.reply())
            stream.reply()->disconnect(this);
        if (stream.data())
            stream.data()->disconnect(this);
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
    if (nextID > quint32(std::numeric_limits<qint32>::max()))
        return 0;

    const quint32 streamID = nextID;
    nextID += 2;

    return streamID;
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

#endif // !defined(QT_NO_HTTP) && !defined(QT_NO_SSL)
