// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhttp2connection_p.h"

#include <private/bitstreams_p.h>

#include <QtCore/private/qnumeric_p.h>
#include <QtCore/private/qiodevice_p.h>
#include <QtCore/private/qnoncontiguousbytedevice_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>

#include <algorithm>
#include <memory>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qHttp2ConnectionLog, "qt.network.http2.connection", QtCriticalMsg)

using namespace Qt::StringLiterals;
using namespace Http2;

QHttp2Stream::QHttp2Stream(QHttp2Connection *connection, quint32 streamID) noexcept
    : QObject(connection), m_streamID(streamID)
{
    Q_ASSERT(connection);
    Q_ASSERT(streamID); // stream id 0 is reserved for connection control messages
    qCDebug(qHttp2ConnectionLog, "[%p] new stream %u", connection, streamID);
}

QHttp2Stream::~QHttp2Stream() noexcept = default;

void QHttp2Stream::finishWithError(Http2::Http2Error errorCode, const QString &message)
{
    qCDebug(qHttp2ConnectionLog, "[%p] stream %u finished with error: %ls (error code: %u)",
            getConnection(), m_streamID, qUtf16Printable(message), errorCode);
    transitionState(StateTransition::RST);
    emit errorOccurred(errorCode, message);
}

void QHttp2Stream::finishWithError(Http2::Http2Error errorCode)
{
    QNetworkReply::NetworkError ignored = QNetworkReply::NoError;
    QString message;
    qt_error(errorCode, ignored, message);
    finishWithError(errorCode, message);
}

/*!
    \internal
    Sends a RST_STREAM frame with the given \a errorCode.
    This closes the stream for both sides, any further frames will be dropped.

    Returns \c false if the stream is closed or idle, also if it fails to send
    the RST_STREAM frame. Otherwise, returns \c true.
*/
bool QHttp2Stream::sendRST_STREAM(Http2::Http2Error errorCode)
{
    if (m_state == State::Closed || m_state == State::Idle)
        return false;
    qCDebug(qHttp2ConnectionLog, "[%p] sending RST_STREAM on stream %u, code: %u", getConnection(),
            m_streamID, errorCode);
    transitionState(StateTransition::RST);

    QHttp2Connection *connection = getConnection();
    FrameWriter &frameWriter = connection->frameWriter;
    frameWriter.start(FrameType::RST_STREAM, FrameFlag::EMPTY, m_streamID);
    frameWriter.append(quint32(errorCode));
    return frameWriter.write(*connection->getSocket());
}

/*!
    \internal
    Sends a DATA frame with the bytes obtained from \a device.

    This function will send as many DATA frames as needed to send all the data
    from \a device. If \a endStream is \c true, the END_STREAM flag will be set.

    \a device must stay alive for the duration of the upload.
    A way of doing this is to heap-allocate the \a device and parent it to the
    QHttp2Stream.
*/
void QHttp2Stream::sendDATA(QIODevice *device, bool endStream)
{
    Q_ASSERT(!m_uploadDevice);
    Q_ASSERT(!m_uploadByteDevice);
    Q_ASSERT(device);
    if (m_state != State::Open && m_state != State::HalfClosedRemote)
        return;

    auto *byteDevice = QNonContiguousByteDeviceFactory::create(device);
    connect(this, &QHttp2Stream::uploadFinished, byteDevice, &QObject::deleteLater);
    byteDevice->setParent(this);
    m_uploadDevice = device;
    qCDebug(qHttp2ConnectionLog, "[%p] starting sendDATA on stream %u, of IODevice: %p",
            getConnection(), m_streamID, device);
    sendDATA(byteDevice, endStream);
}

/*!
    \internal
    Sends a DATA frame with the bytes obtained from \a device.

    This function will send as many DATA frames as needed to send all the data
    from \a device. If \a endStream is \c true, the END_STREAM flag will be set.

    \a device must stay alive for the duration of the upload.
    A way of doing this is to heap-allocate the \a device and parent it to the
    QHttp2Stream.
*/
void QHttp2Stream::sendDATA(QNonContiguousByteDevice *device, bool endStream)
{
    Q_ASSERT(!m_uploadByteDevice);
    Q_ASSERT(device);
    if (m_state != State::Open && m_state != State::HalfClosedRemote)
        return;

    qCDebug(qHttp2ConnectionLog, "[%p] starting sendDATA on stream %u, of device: %p",
            getConnection(), m_streamID, device);

    m_uploadByteDevice = device;
    m_endStreamAfterDATA = endStream;
    connect(m_uploadByteDevice, &QNonContiguousByteDevice::readyRead, this,
            &QHttp2Stream::maybeResumeUpload);
    connect(m_uploadByteDevice, &QObject::destroyed, this, &QHttp2Stream::uploadDeviceDestroyed);

    internalSendDATA();
}

void QHttp2Stream::internalSendDATA()
{
    Q_ASSERT(m_uploadByteDevice);
    QHttp2Connection *connection = getConnection();
    Q_ASSERT(connection->maxFrameSize > frameHeaderSize);
    QIODevice *socket = connection->getSocket();

    qCDebug(qHttp2ConnectionLog,
            "[%p] stream %u, about to write to socket, current session window size: %d, stream "
            "window size: %d, bytes available: %lld",
            connection, m_streamID, connection->sessionSendWindowSize, m_sendWindow,
            m_uploadByteDevice->size() - m_uploadByteDevice->pos());

    qint32 remainingWindowSize = std::min<qint32>(connection->sessionSendWindowSize, m_sendWindow);
    FrameWriter &frameWriter = connection->frameWriter;
    qint64 totalBytesWritten = 0;
    const auto deviceCanRead = [this, connection] {
        // We take advantage of knowing the internals of one of the devices used.
        // It will request X bytes to move over to the http thread if there's
        // not enough left, so we give it a large size. It will anyway return
        // the size it can actually provide.
        const qint64 requestSize = connection->maxFrameSize * 10ll;
        qint64 tmp = 0;
        return m_uploadByteDevice->readPointer(requestSize, tmp) != nullptr && tmp > 0;
    };

    bool sentEND_STREAM = false;
    while (remainingWindowSize && deviceCanRead()) {
        quint32 bytesWritten = 0;
        qint32 remainingBytesInFrame = qint32(connection->maxFrameSize);
        frameWriter.start(FrameType::DATA, FrameFlag::EMPTY, streamID());

        while (remainingWindowSize && deviceCanRead() && remainingBytesInFrame) {
            const qint32 maxToWrite = std::min(remainingWindowSize, remainingBytesInFrame);

            qint64 outBytesAvail = 0;
            const char *readPointer = m_uploadByteDevice->readPointer(maxToWrite, outBytesAvail);
            if (!readPointer || outBytesAvail <= 0) {
                qCDebug(qHttp2ConnectionLog,
                        "[%p] stream %u, cannot write data, device (%p) has %lld bytes available",
                        connection, m_streamID, m_uploadByteDevice, outBytesAvail);
                break;
            }
            const qint32 bytesToWrite = qint32(std::min<qint64>(maxToWrite, outBytesAvail));
            frameWriter.append(QByteArrayView(readPointer, bytesToWrite));
            m_uploadByteDevice->advanceReadPointer(bytesToWrite);

            bytesWritten += bytesToWrite;

            m_sendWindow -= bytesToWrite;
            Q_ASSERT(m_sendWindow >= 0);
            connection->sessionSendWindowSize -= bytesToWrite;
            Q_ASSERT(connection->sessionSendWindowSize >= 0);
            remainingBytesInFrame -= bytesToWrite;
            Q_ASSERT(remainingBytesInFrame >= 0);
            remainingWindowSize -= bytesToWrite;
            Q_ASSERT(remainingWindowSize >= 0);
        }

        qCDebug(qHttp2ConnectionLog, "[%p] stream %u, writing %u bytes to socket", connection,
                m_streamID, bytesWritten);
        if (!deviceCanRead() && m_uploadByteDevice->atEnd() && m_endStreamAfterDATA) {
            sentEND_STREAM = true;
            frameWriter.addFlag(FrameFlag::END_STREAM);
        }
        if (!frameWriter.write(*socket)) {
            qCDebug(qHttp2ConnectionLog, "[%p] stream %u, failed to write to socket", connection,
                    m_streamID);
            finishWithError(INTERNAL_ERROR, "failed to write to socket"_L1);
            return;
        }

        totalBytesWritten += bytesWritten;
    }

    qCDebug(qHttp2ConnectionLog,
            "[%p] stream %u, wrote %lld bytes total, if the device is not exhausted, we'll write "
            "more later. Remaining window size: %d",
            connection, m_streamID, totalBytesWritten, remainingWindowSize);

    emit bytesWritten(totalBytesWritten);
    if (sentEND_STREAM || (!deviceCanRead() && m_uploadByteDevice->atEnd())) {
        qCDebug(qHttp2ConnectionLog,
                "[%p] stream %u, exhausted device %p, sent END_STREAM? %d, %ssending end stream "
                "after DATA",
                connection, m_streamID, m_uploadByteDevice, sentEND_STREAM,
                m_endStreamAfterDATA ? "" : "not ");
        if (!sentEND_STREAM && m_endStreamAfterDATA) {
            // We need to send an empty DATA frame with END_STREAM since we
            // have exhausted the device, but we haven't sent END_STREAM yet.
            // This can happen if we got a final readyRead to signify no more
            // data available, but we hadn't sent the END_STREAM flag yet.
            frameWriter.start(FrameType::DATA, FrameFlag::END_STREAM, streamID());
            frameWriter.write(*socket);
        }
        finishSendDATA();
    } else if (isUploadBlocked()) {
        qCDebug(qHttp2ConnectionLog, "[%p] stream %u, upload blocked", connection, m_streamID);
        emit uploadBlocked();
    }
}

void QHttp2Stream::finishSendDATA()
{
    if (m_endStreamAfterDATA)
        transitionState(StateTransition::CloseLocal);

    disconnect(m_uploadByteDevice, nullptr, this, nullptr);
    m_uploadDevice = nullptr;
    m_uploadByteDevice = nullptr;
    emit uploadFinished();
}

void QHttp2Stream::maybeResumeUpload()
{
    qCDebug(qHttp2ConnectionLog,
            "[%p] stream %u, maybeResumeUpload. Upload device: %p, bytes available: %lld, blocked? "
            "%d",
            getConnection(), m_streamID, m_uploadByteDevice,
            !m_uploadByteDevice ? 0 : m_uploadByteDevice->size() - m_uploadByteDevice->pos(),
            isUploadBlocked());
    if (isUploadingDATA() && !isUploadBlocked())
        internalSendDATA();
}

bool QHttp2Stream::isUploadBlocked() const noexcept
{
    constexpr auto MinFrameSize = Http2::frameHeaderSize + 1; // 1 byte payload
    return isUploadingDATA()
            && (m_sendWindow <= MinFrameSize
                || getConnection()->sessionSendWindowSize <= MinFrameSize);
}

void QHttp2Stream::uploadDeviceReadChannelFinished()
{
    maybeResumeUpload();
}

bool QHttp2Stream::sendHEADERS(const HPack::HttpHeader &headers, bool endStream, quint8 priority)
{
    using namespace HPack;
    if (auto hs = header_size(headers);
        !hs.first || hs.second > getConnection()->maxHeaderListSize()) {
        return false;
    }

    transitionState(StateTransition::Open);

    Q_ASSERT(m_state == State::Open || m_state == State::HalfClosedRemote);

    QHttp2Connection *connection = getConnection();

    qCDebug(qHttp2ConnectionLog, "[%p] stream %u, sending HEADERS frame with %u entries",
            connection, streamID(), uint(headers.size()));

    QIODevice *socket = connection->getSocket();
    FrameWriter &frameWriter = connection->frameWriter;

    frameWriter.start(FrameType::HEADERS, FrameFlag::PRIORITY | FrameFlag::END_HEADERS, streamID());
    if (endStream)
        frameWriter.addFlag(FrameFlag::END_STREAM);

    frameWriter.append(quint32()); // No stream dependency in Qt.
    frameWriter.append(priority);

    // Compress in-place:
    BitOStream outputStream(frameWriter.outboundFrame().buffer);
    if (connection->m_connectionType == QHttp2Connection::Type::Client) {
        if (!connection->encoder.encodeRequest(outputStream, headers))
            return false;
    } else {
        if (!connection->encoder.encodeResponse(outputStream, headers))
            return false;
    }

    bool result = frameWriter.writeHEADERS(*socket, connection->maxFrameSize);
    if (endStream)
        transitionState(StateTransition::CloseLocal);

    return result;
}

void QHttp2Stream::sendWINDOW_UPDATE(quint32 delta)
{
    QHttp2Connection *connection = getConnection();
    m_recvWindow += qint32(delta);
    connection->sendWINDOW_UPDATE(streamID(), delta);
}

void QHttp2Stream::uploadDeviceDestroyed()
{
    if (isUploadingDATA()) {
        // We're in the middle of sending DATA frames, we need to abort
        // the stream.
        sendRST_STREAM(CANCEL);
        emit uploadDeviceError("Upload device destroyed while uploading"_L1);
    }
    m_uploadDevice = nullptr;
}

void QHttp2Stream::setState(State newState)
{
    if (m_state == newState)
        return;
    qCDebug(qHttp2ConnectionLog, "[%p] stream %u, state changed from %d to %d", getConnection(),
            streamID(), int(m_state), int(newState));
    m_state = newState;
    emit stateChanged(newState);
}

// Changes the state as appropriate given the current state and the transition.
// Always call this before emitting any signals since the recipient might rely
// on the new state!
void QHttp2Stream::transitionState(StateTransition transition)
{
    switch (m_state) {
    case State::Idle:
        if (transition == StateTransition::Open)
            setState(State::Open);
        else
            Q_UNREACHABLE(); // We should transition to Open before ever getting here
        break;
    case State::Open:
        switch (transition) {
        case StateTransition::CloseLocal:
            setState(State::HalfClosedLocal);
            break;
        case StateTransition::CloseRemote:
            setState(State::HalfClosedRemote);
            break;
        case StateTransition::RST:
            setState(State::Closed);
            break;
        case StateTransition::Open: // no-op
            break;
        }
        break;
    case State::HalfClosedLocal:
        if (transition == StateTransition::CloseRemote || transition == StateTransition::RST)
            setState(State::Closed);
        break;
    case State::HalfClosedRemote:
        if (transition == StateTransition::CloseLocal || transition == StateTransition::RST)
            setState(State::Closed);
        break;
    case State::ReservedRemote:
        if (transition == StateTransition::RST) {
            setState(State::Closed);
        } else if (transition == StateTransition::CloseLocal) { // Receiving HEADER closes local
            setState(State::HalfClosedLocal);
        }
        break;
    case State::Closed:
        break;
    }
}

void QHttp2Stream::handleDATA(const Frame &inboundFrame)
{
    QHttp2Connection *connection = getConnection();

    qCDebug(qHttp2ConnectionLog, "[%p] stream %u, received DATA frame with payload of %u bytes",
            connection, m_streamID, inboundFrame.payloadSize());

    if (qint32(inboundFrame.payloadSize()) > m_recvWindow) {
        qCDebug(qHttp2ConnectionLog,
                "[%p] stream %u, received DATA frame with payload size %u, "
                "but recvWindow is %d, sending FLOW_CONTROL_ERROR",
                connection, m_streamID, inboundFrame.payloadSize(), m_recvWindow);
        finishWithError(FLOW_CONTROL_ERROR, "flow control error"_L1);
        sendRST_STREAM(FLOW_CONTROL_ERROR);
        return;
    }
    m_recvWindow -= qint32(inboundFrame.payloadSize());
    const bool endStream = inboundFrame.flags().testFlag(FrameFlag::END_STREAM);
    // Uncompress data if needed and append it ...
    if (inboundFrame.dataSize() > 0 || endStream) {
        QByteArray fragment(reinterpret_cast<const char *>(inboundFrame.dataBegin()),
                            inboundFrame.dataSize());
        if (endStream)
            transitionState(StateTransition::CloseRemote);
        emit dataReceived(fragment, endStream);
        m_downloadBuffer.append(std::move(fragment));
    }

    if (!endStream && m_recvWindow < connection->streamInitialReceiveWindowSize / 2) {
        // @future[consider]: emit signal instead
        sendWINDOW_UPDATE(quint32(connection->streamInitialReceiveWindowSize - m_recvWindow));
    }
}

void QHttp2Stream::handleHEADERS(Http2::FrameFlags frameFlags, const HPack::HttpHeader &headers)
{
    if (m_state == State::Idle)
        transitionState(StateTransition::Open);
    const bool endStream = frameFlags.testFlag(FrameFlag::END_STREAM);
    if (endStream)
        transitionState(StateTransition::CloseRemote);
    if (!headers.empty()) {
        m_headers.insert(m_headers.end(), headers.begin(), headers.end());
        emit headersUpdated();
    }
    emit headersReceived(headers, endStream);
}

void QHttp2Stream::handleRST_STREAM(const Frame &inboundFrame)
{
    transitionState(StateTransition::RST);
    m_RST_STREAM_code = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    if (isUploadingDATA()) {
        disconnect(m_uploadByteDevice, nullptr, this, nullptr);
        m_uploadDevice = nullptr;
        m_uploadByteDevice = nullptr;
    }
    finishWithError(Http2Error(*m_RST_STREAM_code));
}

void QHttp2Stream::handleWINDOW_UPDATE(const Frame &inboundFrame)
{
    const quint32 delta = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    const bool valid = delta && delta <= quint32(std::numeric_limits<qint32>::max());
    qint32 sum = 0;
    if (!valid || qAddOverflow(m_sendWindow, qint32(delta), &sum)) {
        qCDebug(qHttp2ConnectionLog,
                "[%p] stream %u, received WINDOW_UPDATE frame with invalid delta %u, sending "
                "PROTOCOL_ERROR",
                getConnection(), m_streamID, delta);
        finishWithError(PROTOCOL_ERROR, "invalid WINDOW_UPDATE delta"_L1);
        sendRST_STREAM(PROTOCOL_ERROR);
        return;
    }
    m_sendWindow = sum;
    // Stream may have been unblocked, so maybe try to write again
    if (isUploadingDATA())
        maybeResumeUpload();
}

/*!
    Create a new HTTP2 connection given a \a config and a \a socket.
    This function assumes that the Upgrade headers etc. in http/1 have already
    been sent and that the connection is already upgraded to http/2.

    The object returned will be a child to the \a socket, or null on failure.
*/
QHttp2Connection *QHttp2Connection::createUpgradedConnection(QIODevice *socket,
                                                             const QHttp2Configuration &config)
{
    Q_ASSERT(socket);

    auto connection = std::unique_ptr<QHttp2Connection>(new QHttp2Connection(socket));
    connection->setH2Configuration(config);
    connection->m_connectionType = QHttp2Connection::Type::Client;
    // HTTP2 connection is already established and request was sent, so stream 1
    // is already 'active' and is closed for any further outgoing data.
    QHttp2Stream *stream = connection->createStreamInternal().unwrap();
    Q_ASSERT(stream->streamID() == 1);
    stream->setState(QHttp2Stream::State::HalfClosedLocal);
    connection->m_upgradedConnection = true;

    if (!connection->sendClientPreface()) {
        qCWarning(qHttp2ConnectionLog, "[%p] Failed to send client preface", connection.get());
        return nullptr;
    }

    return connection.release();
}

/*!
    Create a new HTTP2 connection given a \a config and a \a socket.
    This function will immediately send the client preface.

    The object returned will be a child to the \a socket, or null on failure.
*/
QHttp2Connection *QHttp2Connection::createDirectConnection(QIODevice *socket,
                                                           const QHttp2Configuration &config)
{
    auto connection = std::unique_ptr<QHttp2Connection>(new QHttp2Connection(socket));
    connection->setH2Configuration(config);
    connection->m_connectionType = QHttp2Connection::Type::Client;

    if (!connection->sendClientPreface()) {
        qCWarning(qHttp2ConnectionLog, "[%p] Failed to send client preface", connection.get());
        return nullptr;
    }

    return connection.release();
}

/*!
    Create a new HTTP2 connection given a \a config and a \a socket.

    The object returned will be a child to the \a socket, or null on failure.
*/
QHttp2Connection *QHttp2Connection::createDirectServerConnection(QIODevice *socket,
                                                                 const QHttp2Configuration &config)
{
    auto connection = std::unique_ptr<QHttp2Connection>(new QHttp2Connection(socket));
    connection->setH2Configuration(config);
    connection->m_connectionType = QHttp2Connection::Type::Server;

    connection->m_nextStreamID = 2; // server-initiated streams must be even

    connection->m_waitingForClientPreface = true;

    return connection.release();
}

QH2Expected<QHttp2Stream *, QHttp2Connection::CreateStreamError> QHttp2Connection::createStream()
{
    Q_ASSERT(m_connectionType == Type::Client); // This overload is just for clients
    if (m_nextStreamID > lastValidStreamID)
        return { QHttp2Connection::CreateStreamError::StreamIdsExhausted };
    return createStreamInternal();
}

QH2Expected<QHttp2Stream *, QHttp2Connection::CreateStreamError>
QHttp2Connection::createStreamInternal()
{
    if (m_goingAway)
        return { QHttp2Connection::CreateStreamError::ReceivedGOAWAY };
    const quint32 streamID = m_nextStreamID;
    if (size_t(m_maxConcurrentStreams) <= size_t(numActiveLocalStreams()))
        return { QHttp2Connection::CreateStreamError::MaxConcurrentStreamsReached };
    m_nextStreamID += 2;
    return { createStreamInternal_impl(streamID) };
}

QHttp2Stream *QHttp2Connection::createStreamInternal_impl(quint32 streamID)
{
    qsizetype numStreams = m_streams.size();
    QPointer<QHttp2Stream> &stream = m_streams[streamID];
    if (numStreams == m_streams.size()) // stream already existed
        return nullptr;
    stream = new QHttp2Stream(this, streamID);
    stream->m_recvWindow = streamInitialReceiveWindowSize;
    stream->m_sendWindow = streamInitialSendWindowSize;
    return stream;
}

qsizetype QHttp2Connection::numActiveStreamsImpl(quint32 mask) const noexcept
{
    const auto shouldCount = [mask](const QPointer<QHttp2Stream> &stream) -> bool {
        return stream && (stream->streamID() & 1) == mask;
    };
    return std::count_if(m_streams.cbegin(), m_streams.cend(), shouldCount);
}

/*!
    \internal
    The number of streams the remote peer has started that are still active.
*/
qsizetype QHttp2Connection::numActiveRemoteStreams() const noexcept
{
    const quint32 RemoteMask = m_connectionType == Type::Client ? 0 : 1;
    return numActiveStreamsImpl(RemoteMask);
}

/*!
    \internal
    The number of streams we have started that are still active.
*/
qsizetype QHttp2Connection::numActiveLocalStreams() const noexcept
{
    const quint32 LocalMask = m_connectionType == Type::Client ? 1 : 0;
    return numActiveStreamsImpl(LocalMask);
}

QHttp2Stream *QHttp2Connection::getStream(quint32 streamID) const
{
    return m_streams.value(streamID, nullptr).get();
}

QHttp2Connection::QHttp2Connection(QIODevice *socket) : QObject(socket)
{
    Q_ASSERT(socket);
    Q_ASSERT(socket->isOpen());
    Q_ASSERT(socket->openMode() & QIODevice::ReadWrite);
    // We don't make any connections directly because this is used in
    // in the http2 protocol handler, which is used by
    // QHttpNetworkConnectionChannel. Which in turn owns and deals with all the
    // socket connections.
}

QHttp2Connection::~QHttp2Connection() noexcept
{
    // delete streams now so that any calls it might make back to this
    // Connection will operate on a valid object.
    for (QPointer<QHttp2Stream> &stream : std::exchange(m_streams, {}))
        delete stream.get();
}

bool QHttp2Connection::serverCheckClientPreface()
{
    if (!m_waitingForClientPreface)
        return true;
    auto *socket = getSocket();
    if (socket->bytesAvailable() < Http2::clientPrefaceLength)
        return false;
    if (!readClientPreface()) {
        socket->close();
        emit errorOccurred(Http2Error::PROTOCOL_ERROR, "invalid client preface"_L1);
        qCDebug(qHttp2ConnectionLog, "[%p] Invalid client preface", this);
        return false;
    }
    qCDebug(qHttp2ConnectionLog, "[%p] Peer sent valid client preface", this);
    m_waitingForClientPreface = false;
    if (!sendServerPreface()) {
        connectionError(INTERNAL_ERROR, "Failed to send server preface");
        return false;
    }
    return true;
}

void QHttp2Connection::handleReadyRead()
{
    /* event loop */
    if (m_connectionType == Type::Server && !serverCheckClientPreface())
        return;

    const auto streamIsActive = [](const QPointer<QHttp2Stream> &stream) {
        return stream && stream->isActive();
    };
    if (m_goingAway && std::none_of(m_streams.cbegin(), m_streams.cend(), streamIsActive)) {
        close();
        return;
    }
    QIODevice *socket = getSocket();

    qCDebug(qHttp2ConnectionLog, "[%p] Receiving data, %lld bytes available", this,
            socket->bytesAvailable());

    using namespace Http2;
    while (!m_goingAway || std::any_of(m_streams.cbegin(), m_streams.cend(), streamIsActive)) {
        const auto result = frameReader.read(*socket);
        if (result != FrameStatus::goodFrame)
            qCDebug(qHttp2ConnectionLog, "[%p] Tried to read frame, got %d", this, int(result));
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
        qCDebug(qHttp2ConnectionLog, "[%p] Successfully read a frame, with type: %d", this,
                int(frameType));
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

bool QHttp2Connection::readClientPreface()
{
    auto *socket = getSocket();
    Q_ASSERT(socket->bytesAvailable() >= Http2::clientPrefaceLength);
    char buffer[Http2::clientPrefaceLength];
    const qint64 read = socket->read(buffer, Http2::clientPrefaceLength);
    if (read != Http2::clientPrefaceLength)
        return false;
    return memcmp(buffer, Http2::Http2clientPreface, Http2::clientPrefaceLength) == 0;
}

void QHttp2Connection::handleConnectionClosure()
{
    const auto errorString = QCoreApplication::translate("QHttp", "Connection closed");
    for (auto it = m_streams.begin(), end = m_streams.end(); it != end; ++it) {
        auto stream = it.value();
        if (stream && stream->isActive())
            stream->finishWithError(PROTOCOL_ERROR, errorString);
    }
}

void QHttp2Connection::setH2Configuration(QHttp2Configuration config)
{
    m_config = std::move(config);

    // These values comes from our own API so trust it to be sane.
    maxSessionReceiveWindowSize = qint32(m_config.sessionReceiveWindowSize());
    pushPromiseEnabled = m_config.serverPushEnabled();
    streamInitialReceiveWindowSize = qint32(m_config.streamReceiveWindowSize());
    encoder.setCompressStrings(m_config.huffmanCompressionEnabled());
}

void QHttp2Connection::connectionError(Http2Error errorCode, const char *message)
{
    Q_ASSERT(message);
    if (m_goingAway)
        return;

    qCCritical(qHttp2ConnectionLog, "[%p] Connection error: %s (%d)", this, message,
               int(errorCode));

    m_goingAway = true;
    sendGOAWAY(errorCode);
    auto messageView = QLatin1StringView(message);

    for (QHttp2Stream *stream : std::as_const(m_streams)) {
        if (stream && stream->isActive())
            stream->finishWithError(errorCode, messageView);
    }

    closeSession();
}

void QHttp2Connection::closeSession()
{
    emit connectionClosed();
}

bool QHttp2Connection::streamWasReset(quint32 streamID) noexcept
{
    return m_resetStreamIDs.contains(streamID);
}

bool QHttp2Connection::isInvalidStream(quint32 streamID) noexcept
{
    auto stream = m_streams.value(streamID, nullptr);
    return !stream && !streamWasReset(streamID);
}

bool QHttp2Connection::sendClientPreface()
{
    QIODevice *socket = getSocket();
    // 3.5 HTTP/2 Connection Preface
    const qint64 written = socket->write(Http2clientPreface, clientPrefaceLength);
    if (written != clientPrefaceLength)
        return false;

    if (!sendSETTINGS()) {
        qCWarning(qHttp2ConnectionLog, "[%p] Failed to send SETTINGS", this);
        return false;
    }
    return true;
}

bool QHttp2Connection::sendServerPreface()
{
    // We send our SETTINGS frame and ACK the client's SETTINGS frame when it
    // arrives.
    if (!sendSETTINGS()) {
        qCWarning(qHttp2ConnectionLog, "[%p] Failed to send SETTINGS", this);
        return false;
    }
    return true;
}

bool QHttp2Connection::sendSETTINGS()
{
    QIODevice *socket = getSocket();
    // 6.5 SETTINGS
    frameWriter.setOutboundFrame(configurationToSettingsFrame(m_config));
    qCDebug(qHttp2ConnectionLog, "[%p] Sending SETTINGS frame, %d bytes", this,
            frameWriter.outboundFrame().payloadSize());
    Q_ASSERT(frameWriter.outboundFrame().payloadSize());

    if (!frameWriter.write(*socket))
        return false;

    sessionReceiveWindowSize = maxSessionReceiveWindowSize;
    // We only send WINDOW_UPDATE for the connection if the size differs from the
    // default 64 KB:
    const auto delta = maxSessionReceiveWindowSize - defaultSessionWindowSize;
    if (delta && !sendWINDOW_UPDATE(connectionStreamID, delta))
        return false;

    waitingForSettingsACK = true;
    return true;
}

bool QHttp2Connection::sendWINDOW_UPDATE(quint32 streamID, quint32 delta)
{
    qCDebug(qHttp2ConnectionLog, "[%p] Sending WINDOW_UPDATE frame, stream %d, delta %u", this,
            streamID, delta);
    frameWriter.start(FrameType::WINDOW_UPDATE, FrameFlag::EMPTY, streamID);
    frameWriter.append(delta);
    return frameWriter.write(*getSocket());
}

bool QHttp2Connection::sendGOAWAY(Http2::Http2Error errorCode)
{
    frameWriter.start(FrameType::GOAWAY, FrameFlag::EMPTY,
                      Http2PredefinedParameters::connectionStreamID);
    frameWriter.append(quint32(m_lastIncomingStreamID));
    frameWriter.append(quint32(errorCode));
    return frameWriter.write(*getSocket());
}

bool QHttp2Connection::sendSETTINGS_ACK()
{
    frameWriter.start(FrameType::SETTINGS, FrameFlag::ACK, Http2::connectionStreamID);
    return frameWriter.write(*getSocket());
}

void QHttp2Connection::handleDATA()
{
    Q_ASSERT(inboundFrame.type() == FrameType::DATA);

    const auto streamID = inboundFrame.streamID();
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "DATA on the connection stream");

    if (isInvalidStream(streamID))
        return connectionError(ENHANCE_YOUR_CALM, "DATA on invalid stream");

    if (qint32(inboundFrame.payloadSize()) > sessionReceiveWindowSize) {
        qCDebug(qHttp2ConnectionLog,
                "[%p] Received DATA frame with payload size %u, "
                "but recvWindow is %d, sending FLOW_CONTROL_ERROR",
                this, inboundFrame.payloadSize(), sessionReceiveWindowSize);
        return connectionError(FLOW_CONTROL_ERROR, "Flow control error");
    }

    sessionReceiveWindowSize -= inboundFrame.payloadSize();

    auto it = m_streams.find(streamID);
    if (it != m_streams.end() && it.value())
        it.value()->handleDATA(inboundFrame);

    if (sessionReceiveWindowSize < maxSessionReceiveWindowSize / 2) {
        // @future[consider]: emit signal instead
        QMetaObject::invokeMethod(this, &QHttp2Connection::sendWINDOW_UPDATE, Qt::QueuedConnection,
                                  quint32(connectionStreamID),
                                  quint32(maxSessionReceiveWindowSize - sessionReceiveWindowSize));
        sessionReceiveWindowSize = maxSessionReceiveWindowSize;
    }
}

void QHttp2Connection::handleHEADERS()
{
    Q_ASSERT(inboundFrame.type() == FrameType::HEADERS);

    const auto streamID = inboundFrame.streamID();
    qCDebug(qHttp2ConnectionLog, "[%p] Received HEADERS frame on stream %d", this, streamID);

    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "HEADERS on 0x0 stream");

    const bool isClient = m_connectionType == Type::Client;
    const bool isClientInitiatedStream = !!(streamID & 1);
    const bool isRemotelyInitiatedStream = isClient ^ isClientInitiatedStream;

    if (isRemotelyInitiatedStream && streamID > m_lastIncomingStreamID) {
        QHttp2Stream *newStream = createStreamInternal_impl(streamID);
        Q_ASSERT(newStream);
        m_lastIncomingStreamID = streamID;
        qCDebug(qHttp2ConnectionLog, "[%p] Created new incoming stream %d", this, streamID);
        emit newIncomingStream(newStream);
    } else if (auto it = m_streams.constFind(streamID); it == m_streams.cend()) {
        qCDebug(qHttp2ConnectionLog, "[%p] Received HEADERS on non-existent stream %d", this,
                streamID);
        return connectionError(PROTOCOL_ERROR, "HEADERS on invalid stream");
    } else if (!*it || (*it)->wasReset()) {
        qCDebug(qHttp2ConnectionLog, "[%p] Received HEADERS on reset stream %d", this, streamID);
        return connectionError(ENHANCE_YOUR_CALM, "HEADERS on invalid stream");
    }

    const auto flags = inboundFrame.flags();
    if (flags.testFlag(FrameFlag::PRIORITY)) {
        qCDebug(qHttp2ConnectionLog, "[%p] HEADERS frame on stream %d has PRIORITY flag", this,
                streamID);
        handlePRIORITY();
        if (m_goingAway)
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

void QHttp2Connection::handlePRIORITY()
{
    Q_ASSERT(inboundFrame.type() == FrameType::PRIORITY
             || inboundFrame.type() == FrameType::HEADERS);

    const auto streamID = inboundFrame.streamID();
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "PRIORITY on 0x0 stream");

    if (isInvalidStream(streamID))
        return connectionError(ENHANCE_YOUR_CALM, "PRIORITY on invalid stream");

    quint32 streamDependency = 0;
    uchar weight = 0;
    const bool noErr = inboundFrame.priority(&streamDependency, &weight);
    Q_UNUSED(noErr);
    Q_ASSERT(noErr);

    const bool exclusive = streamDependency & 0x80000000;
    streamDependency &= ~0x80000000;

    // Ignore this for now ...
    // Can be used for streams (re)prioritization - 5.3
    Q_UNUSED(exclusive);
    Q_UNUSED(weight);
}

void QHttp2Connection::handleRST_STREAM()
{
    Q_ASSERT(inboundFrame.type() == FrameType::RST_STREAM);

    // "RST_STREAM frames MUST be associated with a stream.
    // If a RST_STREAM frame is received with a stream identifier of 0x0,
    // the recipient MUST treat this as a connection error (Section 5.4.1)
    // of type PROTOCOL_ERROR.
    const auto streamID = inboundFrame.streamID();
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "RST_STREAM on 0x0");

    if (!(streamID & 0x1)) { // @future[server]: must be updated for server-side handling
        // RST_STREAM on a promised stream:
        // since we do not keep track of such streams,
        // just ignore.
        return;
    }

    // Anything greater than m_nextStreamID has not been started yet.
    if (streamID >= m_nextStreamID) {
        // "RST_STREAM frames MUST NOT be sent for a stream
        // in the "idle" state. .. the recipient MUST treat this
        // as a connection error (Section 5.4.1) of type PROTOCOL_ERROR."
        return connectionError(PROTOCOL_ERROR, "RST_STREAM on idle stream");
    }

    Q_ASSERT(inboundFrame.dataSize() == 4);

    if (QPointer<QHttp2Stream> stream = m_streams[streamID])
        stream->handleRST_STREAM(inboundFrame);
}

void QHttp2Connection::handleSETTINGS()
{
    // 6.5 SETTINGS.
    Q_ASSERT(inboundFrame.type() == FrameType::SETTINGS);

    if (inboundFrame.streamID() != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "SETTINGS on invalid stream");

    if (inboundFrame.flags().testFlag(FrameFlag::ACK)) {
        if (!waitingForSettingsACK)
            return connectionError(PROTOCOL_ERROR, "unexpected SETTINGS ACK");
        qCDebug(qHttp2ConnectionLog, "[%p] Received SETTINGS ACK", this);
        waitingForSettingsACK = false;
        return;
    }
    qCDebug(qHttp2ConnectionLog, "[%p] Received SETTINGS frame", this);

    if (inboundFrame.dataSize()) {
        auto src = inboundFrame.dataBegin();
        for (const uchar *end = src + inboundFrame.dataSize(); src != end; src += 6) {
            const Settings identifier = Settings(qFromBigEndian<quint16>(src));
            const quint32 intVal = qFromBigEndian<quint32>(src + 2);
            if (!acceptSetting(identifier, intVal)) {
                // If not accepted - we finish with connectionError.
                qCDebug(qHttp2ConnectionLog, "[%p] Received an unacceptable setting, %u, %u", this,
                        quint32(identifier), intVal);
                return; // connectionError already called in acceptSetting.
            }
        }
    }

    qCDebug(qHttp2ConnectionLog, "[%p] Sending SETTINGS ACK", this);
    emit settingsFrameReceived();
    sendSETTINGS_ACK();
}

void QHttp2Connection::handlePUSH_PROMISE()
{
    // 6.6 PUSH_PROMISE.
    Q_ASSERT(inboundFrame.type() == FrameType::PUSH_PROMISE);

    if (!pushPromiseEnabled && !waitingForSettingsACK) {
        // This means, server ACKed our 'NO PUSH',
        // but sent us PUSH_PROMISE anyway.
        return connectionError(PROTOCOL_ERROR, "unexpected PUSH_PROMISE frame");
    }

    const auto streamID = inboundFrame.streamID();
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "PUSH_PROMISE with invalid associated stream (0x0)");

    auto it = m_streams.constFind(streamID);
#if 0 // Needs to be done after some timeout in case the stream has only just been reset
    if (it != m_streams.constEnd()) {
        QHttp2Stream *associatedStream = it->get();
        if (associatedStream->state() != QHttp2Stream::State::Open
            && associatedStream->state() != QHttp2Stream::State::HalfClosedLocal) {
            // Cause us to error out below:
            it = m_streams.constEnd();
        }
    }
#endif
    if (it == m_streams.constEnd())
        return connectionError(ENHANCE_YOUR_CALM, "PUSH_PROMISE with invalid associated stream");

    const auto reservedID = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    if ((reservedID & 1) || reservedID <= m_lastIncomingStreamID || reservedID > lastValidStreamID)
        return connectionError(PROTOCOL_ERROR, "PUSH_PROMISE with invalid promised stream ID");

    auto *stream = createStreamInternal_impl(reservedID);
    if (!stream)
        return connectionError(PROTOCOL_ERROR, "PUSH_PROMISE with already active stream ID");
    m_lastIncomingStreamID = reservedID;
    stream->setState(QHttp2Stream::State::ReservedRemote);

    if (!pushPromiseEnabled) {
        // "ignoring a PUSH_PROMISE frame causes the stream state to become
        // indeterminate" - let's send RST_STREAM frame with REFUSE_STREAM code.
        stream->sendRST_STREAM(REFUSE_STREAM);
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

void QHttp2Connection::handlePING()
{
    // @future[server]
    // Since we're implementing a client and not
    // a server, we only reply to a PING, ACKing it.
    Q_ASSERT(inboundFrame.type() == FrameType::PING);

    if (inboundFrame.streamID() != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "PING on invalid stream");

    if (inboundFrame.flags() & FrameFlag::ACK)
        return connectionError(PROTOCOL_ERROR, "unexpected PING ACK");

    Q_ASSERT(inboundFrame.dataSize() == 8);

    frameWriter.start(FrameType::PING, FrameFlag::ACK, connectionStreamID);
    frameWriter.append(inboundFrame.dataBegin(), inboundFrame.dataBegin() + 8);
    frameWriter.write(*getSocket());
}

void QHttp2Connection::handleGOAWAY()
{
    // 6.8 GOAWAY

    Q_ASSERT(inboundFrame.type() == FrameType::GOAWAY);
    // "An endpoint MUST treat a GOAWAY frame with a stream identifier
    // other than 0x0 as a connection error (Section 5.4.1) of type PROTOCOL_ERROR."
    if (inboundFrame.streamID() != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "GOAWAY on invalid stream");

    const uchar *const src = inboundFrame.dataBegin();
    quint32 lastStreamID = qFromBigEndian<quint32>(src);
    const Http2Error errorCode = Http2Error(qFromBigEndian<quint32>(src + 4));

    if (!lastStreamID) {
        // "The last stream identifier can be set to 0 if no
        // streams were processed."
        lastStreamID = 1;
    } else if (!(lastStreamID & 0x1)) {
        // 5.1.1 - we (client) use only odd numbers as stream identifiers.
        return connectionError(PROTOCOL_ERROR, "GOAWAY with invalid last stream ID");
    } else if (lastStreamID >= m_nextStreamID) {
        // "A server that is attempting to gracefully shut down a connection SHOULD
        // send an initial GOAWAY frame with the last stream identifier set to 2^31-1
        // and a NO_ERROR code."
        if (lastStreamID != lastValidStreamID || errorCode != HTTP2_NO_ERROR)
            return connectionError(PROTOCOL_ERROR, "GOAWAY invalid stream/error code");
    } else {
        lastStreamID += 2;
    }

    m_goingAway = true;

    emit receivedGOAWAY(errorCode, lastStreamID);

    for (quint32 id = lastStreamID; id < m_nextStreamID; id += 2) {
        QHttp2Stream *stream = m_streams.value(id, nullptr);
        if (stream && stream->isActive())
            stream->finishWithError(errorCode, "Received GOAWAY"_L1);
    }

    const auto isActive = [](const QHttp2Stream *stream) { return stream && stream->isActive(); };
    if (std::none_of(m_streams.cbegin(), m_streams.cend(), isActive))
        closeSession();
}

void QHttp2Connection::handleWINDOW_UPDATE()
{
    Q_ASSERT(inboundFrame.type() == FrameType::WINDOW_UPDATE);

    const quint32 delta = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    const bool valid = delta && delta <= quint32(std::numeric_limits<qint32>::max());
    const auto streamID = inboundFrame.streamID();

    qCDebug(qHttp2ConnectionLog(), "[%p] Received WINDOW_UPDATE, stream %d, delta %d", this,
            streamID, delta);
    if (streamID == connectionStreamID) {
        qint32 sum = 0;
        if (!valid || qAddOverflow(sessionSendWindowSize, qint32(delta), &sum))
            return connectionError(PROTOCOL_ERROR, "WINDOW_UPDATE invalid delta");
        sessionSendWindowSize = sum;
        for (auto &stream : m_streams) {
            if (!stream || !stream->isActive())
                continue;
            // Stream may have been unblocked, so maybe try to write again
            if (stream->isUploadingDATA() && !stream->isUploadBlocked())
                QMetaObject::invokeMethod(stream, &QHttp2Stream::maybeResumeUpload,
                                          Qt::QueuedConnection);
        }
    } else {
        QHttp2Stream *stream = m_streams.value(streamID);
        if (!stream || !stream->isActive()) {
            // WINDOW_UPDATE on closed streams can be ignored.
            qCDebug(qHttp2ConnectionLog, "[%p] Received WINDOW_UPDATE on closed stream %d", this,
                    streamID);
            return;
        }
        stream->handleWINDOW_UPDATE(inboundFrame);
    }
}

void QHttp2Connection::handleCONTINUATION()
{
    Q_ASSERT(inboundFrame.type() == FrameType::CONTINUATION);
    if (continuedFrames.empty())
        return connectionError(PROTOCOL_ERROR,
                               "CONTINUATION without a preceding HEADERS or PUSH_PROMISE");

    if (inboundFrame.streamID() != continuedFrames.front().streamID())
        return connectionError(PROTOCOL_ERROR, "CONTINUATION on invalid stream");

    const bool endHeaders = inboundFrame.flags().testFlag(FrameFlag::END_HEADERS);
    continuedFrames.push_back(std::move(inboundFrame));

    if (!endHeaders)
        return;

    continuationExpected = false;
    handleContinuedHEADERS();
}

void QHttp2Connection::handleContinuedHEADERS()
{
    // 'Continued' HEADERS can be: the initial HEADERS/PUSH_PROMISE frame
    // with/without END_HEADERS flag set plus, if no END_HEADERS flag,
    // a sequence of one or more CONTINUATION frames.
    Q_ASSERT(!continuedFrames.empty());
    const auto firstFrameType = continuedFrames[0].type();
    Q_ASSERT(firstFrameType == FrameType::HEADERS || firstFrameType == FrameType::PUSH_PROMISE);

    const auto streamID = continuedFrames[0].streamID();

    const auto streamIt = m_streams.find(streamID);
    if (firstFrameType == FrameType::HEADERS) {
        if (streamIt != m_streams.end()) {
            QHttp2Stream *stream = streamIt.value();
            if (stream->state() != QHttp2Stream::State::HalfClosedLocal
                && stream->state() != QHttp2Stream::State::ReservedRemote
                && stream->state() != QHttp2Stream::State::Idle
                && stream->state() != QHttp2Stream::State::Open) {
                // We can receive HEADERS on streams initiated by our requests
                // (these streams are in halfClosedLocal or open state) or
                // remote-reserved streams from a server's PUSH_PROMISE.
                stream->finishWithError(PROTOCOL_ERROR,
                                        "HEADERS on invalid stream"_L1);
                stream->sendRST_STREAM(CANCEL);
                return;
            }
        }
        // Else: we cannot just ignore our peer's HEADERS frames - they change
        // HPACK context - even though the stream was reset; apparently the peer
        // has yet to see the reset.
    }

    std::vector<uchar> hpackBlock(assemble_hpack_block(continuedFrames));
    const bool hasHeaderFields = !hpackBlock.empty();
    if (hasHeaderFields) {
        HPack::BitIStream inputStream{ hpackBlock.data(), hpackBlock.data() + hpackBlock.size() };
        if (!decoder.decodeHeaderFields(inputStream))
            return connectionError(COMPRESSION_ERROR, "HPACK decompression failed");
    } else {
        if (firstFrameType == FrameType::PUSH_PROMISE) {
            // It could be a PRIORITY sent in HEADERS - already handled by this
            // point in handleHEADERS. If it was PUSH_PROMISE (HTTP/2 8.2.1):
            // "The header fields in PUSH_PROMISE and any subsequent CONTINUATION
            // frames MUST be a valid and complete set of request header fields
            // (Section 8.1.2.3) ... If a client receives a PUSH_PROMISE that does
            // not include a complete and valid set of header fields or the :method
            // pseudo-header field identifies a method that is not safe, it MUST
            // respond with a stream error (Section 5.4.2) of type PROTOCOL_ERROR."
            if (streamIt != m_streams.end())
                (*streamIt)->sendRST_STREAM(PROTOCOL_ERROR);
            return;
        }

        // We got back an empty hpack block. Now let's figure out if there was an error.
        constexpr auto hpackBlockHasContent = [](const auto &c) { return c.hpackBlockSize() > 0; };
        const bool anyHpackBlock = std::any_of(continuedFrames.cbegin(), continuedFrames.cend(),
                                               hpackBlockHasContent);
        if (anyHpackBlock) // There was hpack block data, but returned empty => it overflowed.
            return connectionError(FRAME_SIZE_ERROR, "HEADERS frame too large");
    }

    if (streamIt == m_streams.end()) // No more processing without a stream from here on.
        return;

    switch (firstFrameType) {
    case FrameType::HEADERS:
        streamIt.value()->handleHEADERS(continuedFrames[0].flags(), decoder.decodedHeader());
        break;
    case FrameType::PUSH_PROMISE: {
        std::optional<QUrl> promiseKey = HPack::makePromiseKeyUrl(decoder.decodedHeader());
        if (!promiseKey)
            return; // invalid URL/key !
        if (m_promisedStreams.contains(*promiseKey))
            return; // already promised!
        const auto promiseID = qFromBigEndian<quint32>(continuedFrames[0].dataBegin());
        QHttp2Stream *stream = m_streams.value(promiseID);
        stream->transitionState(QHttp2Stream::StateTransition::CloseLocal);
        stream->handleHEADERS(continuedFrames[0].flags(), decoder.decodedHeader());
        emit newPromisedStream(stream); // @future[consider] add promise key as argument?
        m_promisedStreams.emplace(*promiseKey, promiseID);
        break;
    }
    default:
        break;
    }
}

bool QHttp2Connection::acceptSetting(Http2::Settings identifier, quint32 newValue)
{
    switch (identifier) {
    case Settings::HEADER_TABLE_SIZE_ID: {
        qCDebug(qHttp2ConnectionLog, "[%p] Received SETTINGS HEADER_TABLE_SIZE %d", this, newValue);
        if (newValue > maxAcceptableTableSize) {
            connectionError(PROTOCOL_ERROR, "SETTINGS invalid table size");
            return false;
        }
        encoder.setMaxDynamicTableSize(newValue);
        break;
    }
    case Settings::INITIAL_WINDOW_SIZE_ID: {
        qCDebug(qHttp2ConnectionLog, "[%p] Received SETTINGS INITIAL_WINDOW_SIZE %d", this,
                newValue);
        // For every active stream - adjust its window
        // (and handle possible overflows as errors).
        if (newValue > quint32(std::numeric_limits<qint32>::max())) {
            connectionError(FLOW_CONTROL_ERROR, "SETTINGS invalid initial window size");
            return false;
        }

        const qint32 delta = qint32(newValue) - streamInitialSendWindowSize;
        streamInitialSendWindowSize = qint32(newValue);

        qCDebug(qHttp2ConnectionLog, "[%p] Adjusting initial window size for %zu streams by %d",
                this, size_t(m_streams.size()), delta);
        for (const QPointer<QHttp2Stream> &stream : std::as_const(m_streams)) {
            if (!stream || !stream->isActive())
                continue;
            qint32 sum = 0;
            if (qAddOverflow(stream->m_sendWindow, delta, &sum)) {
                stream->sendRST_STREAM(PROTOCOL_ERROR);
                stream->finishWithError(PROTOCOL_ERROR,
                                        "SETTINGS window overflow"_L1);
                continue;
            }
            stream->m_sendWindow = sum;
            if (delta > 0 && stream->isUploadingDATA() && !stream->isUploadBlocked()) {
                QMetaObject::invokeMethod(stream, &QHttp2Stream::maybeResumeUpload,
                                          Qt::QueuedConnection);
            }
        }
        break;
    }
    case Settings::MAX_CONCURRENT_STREAMS_ID: {
        qCDebug(qHttp2ConnectionLog, "[%p] Received SETTINGS MAX_CONCURRENT_STREAMS %d", this,
                newValue);
        m_maxConcurrentStreams = newValue;
        break;
    }
    case Settings::MAX_FRAME_SIZE_ID: {
        qCDebug(qHttp2ConnectionLog, "[%p] Received SETTINGS MAX_FRAME_SIZE %d", this, newValue);
        if (newValue < Http2::minPayloadLimit || newValue > Http2::maxPayloadSize) {
            connectionError(PROTOCOL_ERROR, "SETTINGS max frame size is out of range");
            return false;
        }
        maxFrameSize = newValue;
        break;
    }
    case Settings::MAX_HEADER_LIST_SIZE_ID: {
        qCDebug(qHttp2ConnectionLog, "[%p] Received SETTINGS MAX_HEADER_LIST_SIZE %d", this,
                newValue);
        // We just remember this value, it can later
        // prevent us from sending any request (and this
        // will end up in request/reply error).
        m_maxHeaderListSize = newValue;
        break;
    }
    case Http2::Settings::ENABLE_PUSH_ID:
        qCDebug(qHttp2ConnectionLog, "[%p] Received SETTINGS ENABLE_PUSH %d", this, newValue);
        if (newValue != 0 && newValue != 1) {
            connectionError(PROTOCOL_ERROR, "SETTINGS peer sent illegal value for ENABLE_PUSH");
            return false;
        }
        if (m_connectionType == Type::Client) {
            if (newValue == 1) {
                connectionError(PROTOCOL_ERROR, "SETTINGS server sent ENABLE_PUSH=1");
                return false;
            }
        } else { // server-side
            pushPromiseEnabled = newValue;
            break;
        }
    }

    return true;
}

QT_END_NAMESPACE

#include "moc_qhttp2connection_p.cpp"
