// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:network-protocol

#include "qhttp2connection_p.h"

#include <private/bitstreams_p.h>

#include <QtCore/private/qnumeric_p.h>
#include <QtCore/private/qiodevice_p.h>
#include <QtCore/private/qnoncontiguousbytedevice_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/QRandomGenerator>
#include <QtCore/qloggingcategory.h>

#include <algorithm>
#include <memory>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qHttp2ConnectionLog, "qt.network.http2.connection", QtCriticalMsg)

using namespace Qt::StringLiterals;
using namespace Http2;

/*!
    \class QHttp2Stream
    \inmodule QtNetwork
    \internal

    The QHttp2Stream class represents a single HTTP/2 stream.
    Must be created by QHttp2Connection.

    \sa QHttp2Connection
*/

QHttp2Stream::QHttp2Stream(QHttp2Connection *connection, quint32 streamID) noexcept
    : QObject(connection), m_streamID(streamID)
{
    Q_ASSERT(connection);
    Q_ASSERT(streamID); // stream id 0 is reserved for connection control messages
    qCDebug(qHttp2ConnectionLog, "[%p] new stream %u", connection, streamID);
}

QHttp2Stream::~QHttp2Stream() noexcept {
    if (auto *connection = getConnection()) {
        if (m_state != State::Idle && m_state != State::Closed) {
            qCDebug(qHttp2ConnectionLog, "[%p] stream %u, destroyed while still open", connection,
                    m_streamID);
            // Check if we can still send data, then send RST_STREAM:
            if (connection->getSocket()) {
                if (isUploadingDATA())
                    sendRST_STREAM(CANCEL);
                else
                    sendRST_STREAM(HTTP2_NO_ERROR);
            }
        }

        connection->m_streams.remove(streamID());
    }
}

/*!
    \fn quint32 QHttp2Stream::streamID() const noexcept

    Returns the stream ID of this stream.
*/

/*!
    \fn void QHttp2Stream::headersReceived(const HPack::HttpHeader &headers, bool endStream)

    This signal is emitted when the remote peer has sent a HEADERS frame, and
    potentially some CONTINUATION frames, ending with the END_HEADERS flag
    to this stream.

    The headers are internally combined and decompressed, and are accessible
    through the \a headers parameter. If the END_STREAM flag was set, the
    \a endStream parameter will be \c true, indicating that the peer does not
    intend to send any more frames on this stream.

    \sa receivedHeaders()
*/

/*!
    \fn void QHttp2Stream::headersUpdated()

    This signal may be emitted if a new HEADERS frame was received after
    already processing a previous HEADERS frame.

    \sa headersReceived(), receivedHeaders()
*/

/*!
    \fn void QHttp2Stream::errorOccurred(Http2::Http2Error errorCode, const QString &errorString)

    This signal is emitted when the stream has encountered an error. The
    \a errorCode parameter is the HTTP/2 error code, and the \a errorString
    parameter is a human-readable description of the error.

    \sa https://www.rfc-editor.org/rfc/rfc7540#section-7
*/

/*!
    \fn void QHttp2Stream::stateChanged(State newState)

    This signal is emitted when the state of the stream changes. The \a newState
    parameter is the new state of the stream.

    Examples of this is sending or receiving a frame with the END_STREAM flag.
    This will transition the stream to the HalfClosedLocal or HalfClosedRemote
    state, respectively.

    \sa state()
*/


/*!
    \fn void QHttp2Stream::promisedStreamReceived(quint32 newStreamID)

    This signal is emitted when the remote peer has promised a new stream with
    the given \a newStreamID.

    \sa QHttp2Connection::promisedStream()
*/

/*!
    \fn void QHttp2Stream::uploadBlocked()

    This signal is emitted when the stream is unable to send more data because
    the remote peer's receive window is full.

    This is mostly intended for diagnostics as there is no expectation that the
    user can do anything to react to this.
*/

/*!
    \fn void QHttp2Stream::dataReceived(const QByteArray &data, bool endStream)

    This signal is emitted when the stream has received a DATA frame from the
    remote peer. The \a data parameter contains the payload of the frame, and
    the \a endStream parameter is \c true if the END_STREAM flag was set.

    \sa downloadBuffer()
*/

/*!
    \fn void QHttp2Stream::bytesWritten(qint64 bytesWritten)

    This signal is emitted when the stream has written \a bytesWritten bytes to
    the network.
*/

/*!
    \fn void QHttp2Stream::uploadDeviceError(const QString &errorString)

    This signal is emitted if the upload device encounters an error while
    sending data. The \a errorString parameter is a human-readable description
    of the error.
*/

/*!
    \fn void QHttp2Stream::uploadFinished()

    This signal is emitted when the stream has finished sending all the data
    from the upload device.

    If the END_STREAM flag was set for sendDATA() then the stream will be
    closed for further writes before this signal is emitted.
*/

/*!
    \fn bool QHttp2Stream::isUploadingDATA() const noexcept

    Returns \c true if the stream is currently sending DATA frames.
*/

/*!
    \fn State QHttp2Stream::state() const noexcept

    Returns the current state of the stream.

    \sa stateChanged()
*/
/*!
    \fn bool QHttp2Stream::isActive() const noexcept

    Returns \c true if the stream has been opened and is not yet closed.
*/
/*!
    \fn bool QHttp2Stream::isPromisedStream() const noexcept

    Returns \c true if the stream was promised by the remote peer.
*/
/*!
    \fn bool QHttp2Stream::wasReset() const noexcept

    Returns \c true if the stream was reset by the remote peer.
*/
/*!
    \fn quint32 QHttp2Stream::RST_STREAM_code() const noexcept

    Returns the HTTP/2 error code if the stream was reset by the remote peer.
    If the stream was not reset, this function returns 0.
*/
/*!
    \fn HPack::HttpHeader QHttp2Stream::receivedHeaders() const noexcept

    Returns the headers received from the remote peer, if any.
*/
/*!
    \fn QByteDataBuffer QHttp2Stream::downloadBuffer() const noexcept

    Returns the buffer containing the data received from the remote peer.
*/

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


void QHttp2Stream::streamError(Http2::Http2Error errorCode,
                               QLatin1StringView message)
{
    qCDebug(qHttp2ConnectionLog, "[%p] stream %u finished with error: %ls (error code: %u)",
            getConnection(), m_streamID, qUtf16Printable(message), errorCode);

    sendRST_STREAM(errorCode);
    emit errorOccurred(errorCode, message);
}

/*!
    Sends a RST_STREAM frame with the given \a errorCode.
    This closes the stream for both sides, any further frames will be dropped.

    Returns \c false if the stream is closed or idle, also if it fails to send
    the RST_STREAM frame. Otherwise, returns \c true.
*/
bool QHttp2Stream::sendRST_STREAM(Http2::Http2Error errorCode)
{
    if (m_state == State::Closed || m_state == State::Idle)
        return false;
    // Never respond to a RST_STREAM with a RST_STREAM or looping might occur.
    if (m_RST_STREAM_received.has_value())
        return false;

    getConnection()->registerStreamAsResetLocally(streamID());

    m_RST_STREAM_sent = errorCode;
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
    Sends a DATA frame with the bytes obtained from \a payload.

    This function will send as many DATA frames as needed to send all the data
    from \a payload. If \a endStream is \c true, the END_STREAM flag will be
    set.

    Returns \c{true} if we were able to \e{start} writing to the socket,
    false otherwise.
    Note that even though we started writing, the socket may error out before
    this function returns. Call state() for the new status.
*/
bool QHttp2Stream::sendDATA(const QByteArray &payload, bool endStream)
{
    Q_ASSERT(!m_uploadByteDevice);
    if (m_state != State::Open && m_state != State::HalfClosedRemote)
        return false;

    auto *byteDevice = QNonContiguousByteDeviceFactory::create(payload);
    m_owningByteDevice = true;
    byteDevice->setParent(this);
    return sendDATA(byteDevice, endStream);
}

/*!
    Sends a DATA frame with the bytes obtained from \a device.

    This function will send as many DATA frames as needed to send all the data
    from \a device. If \a endStream is \c true, the END_STREAM flag will be set.

    \a device must stay alive for the duration of the upload.
    A way of doing this is to heap-allocate the \a device and parent it to the
    QHttp2Stream.

    Returns \c{true} if we were able to \e{start} writing to the socket,
    false otherwise.
    Note that even though we started writing, the socket may error out before
    this function returns. Call state() for the new status.
*/
bool QHttp2Stream::sendDATA(QIODevice *device, bool endStream)
{
    Q_ASSERT(!m_uploadDevice);
    Q_ASSERT(!m_uploadByteDevice);
    Q_ASSERT(device);
    if (m_state != State::Open && m_state != State::HalfClosedRemote) {
        qCWarning(qHttp2ConnectionLog, "[%p] attempt to sendDATA on closed stream %u, "
                                       "of device: %p.",
                  getConnection(), m_streamID, device);
        return false;
    }

    qCDebug(qHttp2ConnectionLog, "[%p] starting sendDATA on stream %u, of device: %p",
            getConnection(), m_streamID, device);
    auto *byteDevice = QNonContiguousByteDeviceFactory::create(device);
    m_owningByteDevice = true;
    byteDevice->setParent(this);
    m_uploadDevice = device;
    return sendDATA(byteDevice, endStream);
}

/*!
    Sends a DATA frame with the bytes obtained from \a device.

    This function will send as many DATA frames as needed to send all the data
    from \a device. If \a endStream is \c true, the END_STREAM flag will be set.

    \a device must stay alive for the duration of the upload.
    A way of doing this is to heap-allocate the \a device and parent it to the
    QHttp2Stream.

    Returns \c{true} if we were able to \e{start} writing to the socket,
    false otherwise.
    Note that even though we started writing, the socket may error out before
    this function returns. Call state() for the new status.
*/
bool QHttp2Stream::sendDATA(QNonContiguousByteDevice *device, bool endStream)
{
    Q_ASSERT(!m_uploadByteDevice);
    Q_ASSERT(device);
    if (m_state != State::Open && m_state != State::HalfClosedRemote) {
        qCWarning(qHttp2ConnectionLog, "[%p] attempt to sendDATA on closed stream %u, "
                                       "of device: %p.",
                  getConnection(), m_streamID, device);
        return false;
    }

    qCDebug(qHttp2ConnectionLog, "[%p] starting sendDATA on stream %u, of device: %p",
            getConnection(), m_streamID, device);
    m_uploadByteDevice = device;
    m_endStreamAfterDATA = endStream;
    connect(m_uploadByteDevice, &QNonContiguousByteDevice::readyRead, this,
            &QHttp2Stream::maybeResumeUpload);
    connect(m_uploadByteDevice, &QObject::destroyed, this, &QHttp2Stream::uploadDeviceDestroyed);

    internalSendDATA();
    // There is no early-out in internalSendDATA so if we reach this spot we
    // have at least started to send something, even if it errors out.
    return true;
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
            return finishWithError(INTERNAL_ERROR, "failed to write to socket"_L1);
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
    if (m_owningByteDevice) {
        m_owningByteDevice = false;
        delete m_uploadByteDevice;
    }
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
    else
        getConnection()->m_blockedStreams.insert(streamID());
}

/*!
    Returns \c true if the stream is currently unable to send more data because
    the remote peer's receive window is full.
*/
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

/*!
    Sends a HEADERS frame with the given \a headers and \a priority.
    If \a endStream is \c true, the END_STREAM flag will be set, and the stream
    will be closed for future writes.
    If the headers are too large, or the stream is not in the correct state,
    this function will return \c false. Otherwise, it will return \c true.
*/
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

    // Possibly perform and notify of dynamic table size update:
    for (auto &maybePendingTableSizeUpdate : connection->pendingTableSizeUpdates) {
        if (!maybePendingTableSizeUpdate)
            break; // They are ordered, so if the first one is null, the other one is too.
        qCDebug(qHttp2ConnectionLog, "[%p] stream %u, sending dynamic table size update of size %u",
                connection, streamID(), *maybePendingTableSizeUpdate);
        connection->encoder.setMaxDynamicTableSize(*maybePendingTableSizeUpdate);
        connection->encoder.encodeSizeUpdate(outputStream, *maybePendingTableSizeUpdate);
        maybePendingTableSizeUpdate.reset();
    }

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

/*!
    Sends a WINDOW_UPDATE frame with the given \a delta.
    This increases our receive window size for this stream, allowing the remote
    peer to send more data.
*/
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
        streamError(CANCEL, QLatin1String("Upload device destroyed while uploading"));
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

    qCDebug(qHttp2ConnectionLog,
            "[%p] stream %u, received DATA frame with payload of %u bytes, closing stream? %s",
            connection, m_streamID, inboundFrame.payloadSize(),
            inboundFrame.flags().testFlag(Http2::FrameFlag::END_STREAM) ? "yes" : "no");

    // RFC 9113, 6.1: If a DATA frame is received whose stream is not in the "open" or "half-closed
    // (local)" state, the recipient MUST respond with a stream error (Section 5.4.2) of type
    // STREAM_CLOSED;
    // checked in QHttp2Connection
    Q_ASSERT(state() != State::HalfClosedRemote && state() != State::Closed);

    if (qint32(inboundFrame.payloadSize()) > m_recvWindow) {
        qCDebug(qHttp2ConnectionLog,
                "[%p] stream %u, received DATA frame with payload size %u, "
                "but recvWindow is %d, sending FLOW_CONTROL_ERROR",
                connection, m_streamID, inboundFrame.payloadSize(), m_recvWindow);
        return streamError(FLOW_CONTROL_ERROR, QLatin1String("data bigger than window size"));
    }
    // RFC 9113, 6.1: The total number of padding octets is determined by the value of the Pad
    // Length field. If the length of the padding is the length of the frame payload or greater,
    // the recipient MUST treat this as a connection error (Section 5.4.1) of type PROTOCOL_ERROR.
    // checked in Framereader
    Q_ASSERT(inboundFrame.buffer.size() >= frameHeaderSize);
    Q_ASSERT(inboundFrame.payloadSize() + frameHeaderSize == inboundFrame.buffer.size());

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
    if (m_state == State::Closed) // The stream is already closed, we're not sending anything anyway
        return;

    transitionState(StateTransition::RST);
    m_RST_STREAM_received = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    if (isUploadingDATA()) {
        disconnect(m_uploadByteDevice, nullptr, this, nullptr);
        m_uploadDevice = nullptr;
        m_uploadByteDevice = nullptr;
    }
    finishWithError(Http2Error(*m_RST_STREAM_received));
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
        return streamError(PROTOCOL_ERROR, "invalid WINDOW_UPDATE delta"_L1);
    }
    m_sendWindow = sum;
    // Stream may have been unblocked, so maybe try to write again
    if (isUploadingDATA())
        maybeResumeUpload();
}

/*!
    \class QHttp2Connection
    \inmodule QtNetwork
    \internal

    The QHttp2Connection class represents a HTTP/2 connection.
    It can only be created through the static functions
    createDirectConnection(), createUpgradedConnection(),
    and createDirectServerConnection().

    createDirectServerConnection() is used for server-side connections, and has
    certain limitations that a client does not.

    As a client you can create a QHttp2Stream with createStream().

    \sa QHttp2Stream
*/

/*!
    \fn void QHttp2Connection::newIncomingStream(QHttp2Stream *stream)

    This signal is emitted when a new \a stream is received from the remote
    peer.
*/

/*!
    \fn void QHttp2Connection::newPromisedStream(QHttp2Stream *stream)

    This signal is emitted when the remote peer has promised a new \a stream.
*/

/*!
    \fn void QHttp2Connection::errorReceived()

    This signal is emitted when the connection has received an error.
*/

/*!
    \fn void QHttp2Connection::connectionClosed()

    This signal is emitted when the connection has been closed.
*/

/*!
    \fn void QHttp2Connection::settingsFrameReceived()

    This signal is emitted when the connection has received a SETTINGS frame.
*/

/*!
    \fn void QHttp2Connection::errorOccurred(Http2::Http2Error errorCode, const QString &errorString)

    This signal is emitted when the connection has encountered an error. The
    \a errorCode parameter is the HTTP/2 error code, and the \a errorString
    parameter is a human-readable description of the error.
*/

/*!
    \fn void QHttp2Connection::receivedGOAWAY(Http2::Http2Error errorCode, quint32 lastStreamID)

    This signal is emitted when the connection has received a GOAWAY frame. The
    \a errorCode parameter is the HTTP/2 error code, and the \a lastStreamID
    parameter is the last stream ID that the remote peer will process.

    Any streams of a higher stream ID created by us will be ignored or reset.
*/

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
    connection->m_upgradedConnection = true;
    // HTTP2 connection is already established and request was sent, so stream 1
    // is already 'active' and is closed for any further outgoing data.
    QHttp2Stream *stream = connection->createLocalStreamInternal().unwrap();
    Q_ASSERT(stream->streamID() == 1);
    stream->setState(QHttp2Stream::State::HalfClosedLocal);

    if (!connection->m_prefaceSent) // Preface is sent as part of initial stream-creation.
        return nullptr;

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

/*!
    Creates a stream on this connection.

    Automatically picks the next available stream ID and returns a pointer to
    the new stream, if possible. Otherwise returns an error.

    \sa QHttp2Connection::CreateStreamError, QHttp2Stream
*/
QH2Expected<QHttp2Stream *, QHttp2Connection::CreateStreamError> QHttp2Connection::createStream()
{
    Q_ASSERT(m_connectionType == Type::Client); // This overload is just for clients
    if (m_nextStreamID > lastValidStreamID)
        return { QHttp2Connection::CreateStreamError::StreamIdsExhausted };
    return createLocalStreamInternal();
}

QH2Expected<QHttp2Stream *, QHttp2Connection::CreateStreamError>
QHttp2Connection::createLocalStreamInternal()
{
    if (m_goingAway)
        return { QHttp2Connection::CreateStreamError::ReceivedGOAWAY };
    const quint32 streamID = m_nextStreamID;
    if (size_t(m_peerMaxConcurrentStreams) <= size_t(numActiveLocalStreams()))
        return { QHttp2Connection::CreateStreamError::MaxConcurrentStreamsReached };

    if (QHttp2Stream *ptr = createStreamInternal_impl(streamID)) {
        m_nextStreamID += 2;
        return {ptr};
    }
    // Connection could be broken, we could've ran out of memory, we don't know
    return { QHttp2Connection::CreateStreamError::UnknownError };
}

QHttp2Stream *QHttp2Connection::createStreamInternal_impl(quint32 streamID)
{
    Q_ASSERT(streamID > m_lastIncomingStreamID || streamID >= m_nextStreamID);

    if (m_connectionType == Type::Client && !m_prefaceSent && !sendClientPreface()) {
        qCWarning(qHttp2ConnectionLog, "[%p] Failed to send client preface", this);
        return nullptr;
    }

    auto result = m_streams.tryEmplace(streamID, nullptr);
    if (!result.inserted)
        return nullptr;
    QPointer<QHttp2Stream> &stream = result.iterator.value();
    stream = new QHttp2Stream(this, streamID);
    stream->m_recvWindow = streamInitialReceiveWindowSize;
    stream->m_sendWindow = streamInitialSendWindowSize;

    connect(stream, &QHttp2Stream::uploadBlocked, this, [this, stream] {
        m_blockedStreams.insert(stream->streamID());
    });
    *result.iterator = stream;
    return *result.iterator;
}

qsizetype QHttp2Connection::numActiveStreamsImpl(quint32 mask) const noexcept
{
    const auto shouldCount = [mask](const QPointer<QHttp2Stream> &stream) -> bool {
        return stream && (stream->streamID() & 1) == mask && stream->isActive();
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

/*!
    Return a pointer to a stream with the given \a streamID, or null if no such
    stream exists or it was deleted.
*/
QHttp2Stream *QHttp2Connection::getStream(quint32 streamID) const
{
    return m_streams.value(streamID, nullptr).get();
}


/*!
    \fn QHttp2Stream *QHttp2Connection::promisedStream(const QUrl &streamKey) const

    Returns a pointer to the stream that was promised with the given
    \a streamKey, if any. Otherwise, returns null.
*/

/*!
    \fn void QHttp2Connection::close()

    This sends a GOAWAY frame on the connection stream, gracefully closing the
    connection.
*/

/*!
    \fn bool QHttp2Connection::isGoingAway() const noexcept

    Returns \c true if the connection is in the process of being closed, or
    \c false otherwise.
*/

/*!
    \fn quint32 QHttp2Connection::maxConcurrentStreams() const noexcept

    Returns the maximum number of concurrent streams we are allowed to have
    active at any given time. This is a directional setting, and the remote
    peer may have a different value.
*/

/*!
    \fn quint32 QHttp2Connection::maxHeaderListSize() const noexcept

    Returns the maximum size of the header which the peer is willing to accept.
*/

/*!
    \fn bool QHttp2Connection::isUpgradedConnection() const noexcept

    Returns \c true if this connection was created as a result of an HTTP/1
    upgrade to HTTP/2, or \c false otherwise.
*/

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

QHttp2Connection::~QHttp2Connection()
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

bool QHttp2Connection::sendPing()
{
    std::array<char, 8> data;

    QRandomGenerator gen;
    gen.generate(data.begin(), data.end());
    return sendPing(data);
}

bool QHttp2Connection::sendPing(QByteArrayView data)
{
    frameWriter.start(FrameType::PING, FrameFlag::EMPTY, connectionStreamID);

    Q_ASSERT(data.length() == 8);
    if (!m_lastPingSignature) {
        m_lastPingSignature = data.toByteArray();
    } else {
        qCWarning(qHttp2ConnectionLog, "[%p] No PING is sent while waiting for the previous PING.", this);
        return false;
    }

    frameWriter.append((uchar*)data.data(), (uchar*)data.end());
    frameWriter.write(*getSocket());
    return true;
}

/*!
    This function must be called when you have received a readyRead signal
    (or equivalent) from the QIODevice. It will read and process any incoming
    HTTP/2 frames and emit signals as appropriate.
*/
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
    if (!m_prefaceSent)
        return;

    while (!m_goingAway || std::any_of(m_streams.cbegin(), m_streams.cend(), streamIsActive)) {
        const auto result = frameReader.read(*socket);
        if (result != FrameStatus::goodFrame)
            qCDebug(qHttp2ConnectionLog, "[%p] Tried to read frame, got %d", this, int(result));
        switch (result) {
        case FrameStatus::incompleteFrame:
            return;
        case FrameStatus::protocolError:
            return connectionError(PROTOCOL_ERROR, "invalid frame");
        case FrameStatus::sizeError: {
            const auto streamID = frameReader.inboundFrame().streamID();
            const auto frameType = frameReader.inboundFrame().type();
            auto stream = getStream(streamID);
            // RFC 9113, 4.2: A frame size error in a frame that could alter the state of the
            // entire connection MUST be treated as a connection error (Section 5.4.1); this
            // includes any frame carrying a field block (Section 4.3) (that is, HEADERS,
            // PUSH_PROMISE, and CONTINUATION), a SETTINGS frame, and any frame with a stream
            // identifier of 0.
            if (frameType == FrameType::HEADERS ||
                frameType == FrameType::SETTINGS ||
                frameType == FrameType::PUSH_PROMISE ||
                frameType == FrameType::CONTINUATION ||
                 // never reply RST_STREAM with RST_STREAM
                frameType == FrameType::RST_STREAM ||
                streamID == connectionStreamID)
                return connectionError(FRAME_SIZE_ERROR, "invalid frame size");
            // DATA; PRIORITY; WINDOW_UPDATE
            if (stream)
                return stream->streamError(Http2Error::FRAME_SIZE_ERROR,
                                           QLatin1String("invalid frame size"));
            else
                return; // most likely a closed and deleted stream. Can be ignored.
        }
        default:
            break;
        }

        Q_ASSERT(result == FrameStatus::goodFrame);

        inboundFrame = std::move(frameReader.inboundFrame());

        const auto frameType = inboundFrame.type();
        qCDebug(qHttp2ConnectionLog, "[%p] Successfully read a frame, with type: %d", this,
                int(frameType));

        // RFC 9113, 6.2/6.6: A HEADERS/PUSH_PROMISE frame without the END_HEADERS flag set MUST be
        // followed by a CONTINUATION frame for the same stream. A receiver MUST treat the
        // receipt of any other type of frame or a frame on a different stream as a
        // connection error
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

/*!
    This function must be called when the socket has been disconnected, and will
    end all remaining streams with an error.
*/
void QHttp2Connection::handleConnectionClosure()
{
    const auto errorString = QCoreApplication::translate("QHttp", "Connection closed");
    for (auto it = m_streams.cbegin(), end = m_streams.cend(); it != end; ++it) {
        const QPointer<QHttp2Stream> &stream = it.value();
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
    m_maxConcurrentStreams = m_config.maxConcurrentStreams();
    encoder.setCompressStrings(m_config.huffmanCompressionEnabled());
}

void QHttp2Connection::connectionError(Http2Error errorCode, const char *message)
{
    Q_ASSERT(message);
    // RFC 9113, 6.8: An endpoint MAY send multiple GOAWAY frames if circumstances change.
    // Anyway, we do not send multiple GOAWAY frames.
    if (m_goingAway)
        return;

    qCCritical(qHttp2ConnectionLog, "[%p] Connection error: %s (%d)", this, message,
               int(errorCode));

    // RFC 9113, 6.8: Endpoints SHOULD always send a GOAWAY frame before closing a connection so
    // that the remote peer can know whether a stream has been partially processed or not.
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

bool QHttp2Connection::streamWasResetLocally(quint32 streamID) noexcept
{
    return m_resetStreamIDs.contains(streamID);
}

void QHttp2Connection::registerStreamAsResetLocally(quint32 streamID)
{
    // RFC 9113, 6.4: However, after sending the RST_STREAM, the sending endpoint MUST be prepared
    // to receive and process additional frames sent on the stream that might have been sent by the
    // peer prior to the arrival of the RST_STREAM.

    // Store the last 100 stream ids that were reset locally. Frames received on these streams
    // are still considered valid for some time (Until 100 other streams are reset locally).
    m_resetStreamIDs.append(streamID);
    while (m_resetStreamIDs.size() > 100)
        m_resetStreamIDs.takeFirst();
}

bool QHttp2Connection::isInvalidStream(quint32 streamID) noexcept
{
    auto stream = m_streams.value(streamID, nullptr);
    return (!stream || stream->wasResetbyPeer()) && !streamWasResetLocally(streamID);
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
    m_prefaceSent = true;
    if (socket->bytesAvailable()) // We ignore incoming data until preface is sent, so handle it now
        QMetaObject::invokeMethod(this, &QHttp2Connection::handleReadyRead, Qt::QueuedConnection);
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
    m_prefaceSent = true;
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

    // RFC9113, 6.1: An endpoint that receives an unexpected stream identifier MUST respond
    // with a connection error.
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "DATA on the connection stream");

    if (isInvalidStream(streamID))
        return connectionError(ENHANCE_YOUR_CALM, "DATA on invalid stream");

    QHttp2Stream *stream = nullptr;
    if (!streamWasResetLocally(streamID)) {
        stream = getStream(streamID);
        // RFC9113, 6.1: If a DATA frame is received whose stream is not in the "open" or
        // "half-closed (local)" state, the recipient MUST respond with a stream error.
        if (stream->state() == QHttp2Stream::State::HalfClosedRemote
            || stream->state() == QHttp2Stream::State::Closed) {
            return stream->streamError(Http2Error::STREAM_CLOSED,
                                       QLatin1String("Data on closed stream"));
        }
    }

    if (qint32(inboundFrame.payloadSize()) > sessionReceiveWindowSize) {
        qCDebug(qHttp2ConnectionLog,
                "[%p] Received DATA frame with payload size %u, "
                "but recvWindow is %d, sending FLOW_CONTROL_ERROR",
                this, inboundFrame.payloadSize(), sessionReceiveWindowSize);
        return connectionError(FLOW_CONTROL_ERROR, "Flow control error");
    }

    sessionReceiveWindowSize -= inboundFrame.payloadSize();

    if (stream)
        stream->handleDATA(inboundFrame);

    if (inboundFrame.flags().testFlag(FrameFlag::END_STREAM))
        emit receivedEND_STREAM(streamID);

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
    qCDebug(qHttp2ConnectionLog, "[%p] Received HEADERS frame on stream %d, end stream? %s", this,
            streamID, inboundFrame.flags().testFlag(Http2::FrameFlag::END_STREAM) ? "yes" : "no");

    // RFC 9113, 6.2: If a HEADERS frame is received whose Stream Identifier field is 0x00, the
    // recipient MUST respond with a connection error.
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "HEADERS on 0x0 stream");

    const bool isClient = m_connectionType == Type::Client;
    const bool isClientInitiatedStream = !!(streamID & 1);
    const bool isRemotelyInitiatedStream = isClient ^ isClientInitiatedStream;

    if (isRemotelyInitiatedStream && streamID > m_lastIncomingStreamID) {
        bool streamCountIsOk = size_t(m_maxConcurrentStreams) > size_t(numActiveRemoteStreams());
        QHttp2Stream *newStream = createStreamInternal_impl(streamID);
        Q_ASSERT(newStream);
        m_lastIncomingStreamID = streamID;

        if (!streamCountIsOk) {
            newStream->setState(QHttp2Stream::State::Open);
            newStream->streamError(PROTOCOL_ERROR, QLatin1String("Max concurrent streams reached"));

            emit incomingStreamErrorOccured(CreateStreamError::MaxConcurrentStreamsReached);
            return;
        }

        qCDebug(qHttp2ConnectionLog, "[%p] Created new incoming stream %d", this, streamID);
        emit newIncomingStream(newStream);
    } else if (streamWasResetLocally(streamID)) {
        qCDebug(qHttp2ConnectionLog,
                "[%p] Received HEADERS on previously locally reset stream %d (must process but ignore)",
                this, streamID);
        // nop
    } else if (auto it = m_streams.constFind(streamID); it == m_streams.cend()) {
        // RFC 9113, 6.2: HEADERS frames MUST be associated with a stream.
        // A connection error is not required but it seems to be the right thing to do.
        qCDebug(qHttp2ConnectionLog, "[%p] Received HEADERS on non-existent stream %d", this,
                streamID);
        return connectionError(PROTOCOL_ERROR, "HEADERS on invalid stream");
    } else if (isInvalidStream(streamID)) {
        // RFC 9113 6.4: After receiving a RST_STREAM on a stream, the receiver MUST NOT send
        // additional frames for that stream
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
    // RFC 9913, 6.3: If a PRIORITY frame is received with a stream identifier of 0x00, the
    // recipient MUST respond with a connection error
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "PRIORITY on 0x0 stream");

    // RFC 9113 6.4: After receiving a RST_STREAM on a stream, the receiver MUST NOT send
    // additional frames for that stream
    if (isInvalidStream(streamID))
        return connectionError(ENHANCE_YOUR_CALM, "PRIORITY on invalid stream");

    // RFC 9913, 6.3:  A PRIORITY frame with a length other than 5 octets MUST be treated as a
    // stream error (Section 5.4.2) of type FRAME_SIZE_ERROR.
    // checked in Frame::validateHeader()
    Q_ASSERT(inboundFrame.type() != FrameType::PRIORITY || inboundFrame.payloadSize() == 5);

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

    // RFC 9113, 6.4: RST_STREAM frames MUST be associated with a stream.
    // If a RST_STREAM frame is received with a stream identifier of 0x0,
    // the recipient MUST treat this as a connection error (Section 5.4.1)
    // of type PROTOCOL_ERROR.
    const auto streamID = inboundFrame.streamID();
    if (streamID == connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "RST_STREAM on 0x0");

    // RFC 9113, 6.4: A RST_STREAM frame with a length other than 4 octets MUST be treated as a
    // connection error (Section 5.4.1) of type FRAME_SIZE_ERROR.
    // checked in Frame::validateHeader()
    Q_ASSERT(inboundFrame.payloadSize() == 4);

    const auto error = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    if (QPointer<QHttp2Stream> stream = m_streams[streamID])
        emit stream->rstFrameReceived(error);

    // Verify that whatever stream is being RST'd is not in the idle state:
    const quint32 lastRelevantStreamID = [this, streamID]() {
        quint32 peerMask = m_connectionType == Type::Client ? 0 : 1;
        return ((streamID & 1) == peerMask) ? m_lastIncomingStreamID : m_nextStreamID - 2;
    }();
    if (streamID > lastRelevantStreamID) {
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

    // RFC 9113, 6.5: If an endpoint receives a SETTINGS frame whose Stream Identifier field is
    // anything other than 0x00, the endpoint MUST respond with a connection error
    if (inboundFrame.streamID() != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "SETTINGS on invalid stream");

    if (inboundFrame.flags().testFlag(FrameFlag::ACK)) {
        // RFC 9113, 6.5: Receipt of a SETTINGS frame with the ACK flag set and a length field
        // value other than 0 MUST be treated as a connection error
        if (inboundFrame.payloadSize ())
            return connectionError(FRAME_SIZE_ERROR, "SETTINGS ACK with data");
        if (!waitingForSettingsACK)
            return connectionError(PROTOCOL_ERROR, "unexpected SETTINGS ACK");
        qCDebug(qHttp2ConnectionLog, "[%p] Received SETTINGS ACK", this);
        waitingForSettingsACK = false;
        return;
    }
    qCDebug(qHttp2ConnectionLog, "[%p] Received SETTINGS frame", this);

    if (inboundFrame.dataSize()) {
        // RFC 9113, 6.5: A SETTINGS frame with a length other than a multiple of 6 octets MUST be
        // treated as a connection error (Section 5.4.1) of type FRAME_SIZE_ERROR.
        // checked in Frame::validateHeader()
        Q_ASSERT (inboundFrame.payloadSize() % 6 == 0);

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
    sendSETTINGS_ACK();
    emit settingsFrameReceived();
}

void QHttp2Connection::handlePUSH_PROMISE()
{
    // 6.6 PUSH_PROMISE.
    Q_ASSERT(inboundFrame.type() == FrameType::PUSH_PROMISE);

    // RFC 9113, 6.6: PUSH_PROMISE MUST NOT be sent if the SETTINGS_ENABLE_PUSH setting of the peer
    // endpoint is set to 0. An endpoint that has set this setting and has received acknowledgment
    // MUST treat the receipt of a PUSH_PROMISE frame as a connection error
    if (!pushPromiseEnabled && !waitingForSettingsACK) {
        // This means, server ACKed our 'NO PUSH',
        // but sent us PUSH_PROMISE anyway.
        return connectionError(PROTOCOL_ERROR, "unexpected PUSH_PROMISE frame");
    }

    // RFC 9113, 6.6: If the Stream Identifier field specifies the value 0x00, a recipient MUST
    // respond with a connection error.
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
    // RFC 9113, 6.6: PUSH_PROMISE frames MUST only be sent on a peer-initiated stream that
    // is in either the "open" or "half-closed (remote)" state.

    // I.e. If you are the server then the client must have initiated the stream you are sending
    // the promise on. And since this is about _sending_ we have to invert "Remote" to "Local"
    // because we are receiving.
    if (it == m_streams.constEnd())
        return connectionError(ENHANCE_YOUR_CALM, "PUSH_PROMISE with invalid associated stream");
    if ((m_connectionType == Type::Client && (streamID & 1) == 0) ||
        (m_connectionType == Type::Server && (streamID & 1) == 1)) {
        return connectionError(ENHANCE_YOUR_CALM, "PUSH_PROMISE with invalid associated stream");
    }
    if ((*it)->state() != QHttp2Stream::State::Open &&
        (*it)->state() != QHttp2Stream::State::HalfClosedLocal) {
        return connectionError(ENHANCE_YOUR_CALM, "PUSH_PROMISE with invalid associated stream");
    }

    // RFC 9113, 6.6: The promised stream identifier MUST be a valid choice for the
    // next stream sent by the sender
    const auto reservedID = qFromBigEndian<quint32>(inboundFrame.dataBegin());
    if ((reservedID & 1) || reservedID <= m_lastIncomingStreamID || reservedID > lastValidStreamID)
        return connectionError(PROTOCOL_ERROR, "PUSH_PROMISE with invalid promised stream ID");

    bool streamCountIsOk = size_t(m_maxConcurrentStreams) > size_t(numActiveRemoteStreams());
    // RFC 9113, 6.6: A receiver MUST treat the receipt of a PUSH_PROMISE that promises an
    // illegal stream identifier (Section 5.1.1) as a connection error
    auto *stream = createStreamInternal_impl(reservedID);
    if (!stream)
        return connectionError(PROTOCOL_ERROR, "PUSH_PROMISE with already active stream ID");
    m_lastIncomingStreamID = reservedID;
    stream->setState(QHttp2Stream::State::ReservedRemote);

    if (!streamCountIsOk) {
        stream->streamError(PROTOCOL_ERROR, QLatin1String("Max concurrent streams reached"));
        emit incomingStreamErrorOccured(CreateStreamError::MaxConcurrentStreamsReached);
        return;
    }

    // "ignoring a PUSH_PROMISE frame causes the stream state to become
    // indeterminate" - let's send RST_STREAM frame with REFUSE_STREAM code.
    if (!pushPromiseEnabled) {
        return stream->streamError(REFUSE_STREAM,
                                   QLatin1String("PUSH_PROMISE not enabled but ignored"));
    }

    // RFC 9113, 6.6: The total number of padding octets is determined by the value of the Pad
    // Length field. If the length of the padding is the length of the frame payload or greater,
    // the recipient MUST treat this as a connection error (Section 5.4.1) of type PROTOCOL_ERROR.
    // checked in Frame::validateHeader()
    Q_ASSERT(inboundFrame.dataSize() > inboundFrame.padding());
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
    Q_ASSERT(inboundFrame.type() == FrameType::PING);

    // RFC 9113, 6.7: PING frames are not associated with any individual stream. If a PING frame is
    // received with a Stream Identifier field value other than 0x00, the recipient MUST respond
    // with a connection error
    if (inboundFrame.streamID() != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "PING on invalid stream");

    // Receipt of a PING frame with a length field value other than 8 MUST be treated
    // as a connection error (Section 5.4.1) of type FRAME_SIZE_ERROR.
    // checked in Frame::validateHeader()
    Q_ASSERT(inboundFrame.payloadSize() == 8);

    if (inboundFrame.flags() & FrameFlag::ACK) {
        QByteArrayView pingSignature(reinterpret_cast<const char *>(inboundFrame.dataBegin()), 8);
        if (!m_lastPingSignature.has_value()) {
            emit pingFrameReceived(PingState::PongNoPingSent);
            qCWarning(qHttp2ConnectionLog, "[%p] PING with ACK received but no PING was sent.", this);
        } else if (pingSignature != m_lastPingSignature) {
            emit pingFrameReceived(PingState::PongSignatureChanged);
            qCWarning(qHttp2ConnectionLog, "[%p] PING signature does not match the last PING.", this);
        } else {
            emit pingFrameReceived(PingState::PongSignatureIdentical);
        }
        m_lastPingSignature.reset();
        return;
    } else {
        emit pingFrameReceived(PingState::Ping);

    }


    frameWriter.start(FrameType::PING, FrameFlag::ACK, connectionStreamID);
    frameWriter.append(inboundFrame.dataBegin(), inboundFrame.dataBegin() + 8);
    frameWriter.write(*getSocket());
}

void QHttp2Connection::handleGOAWAY()
{
    // 6.8 GOAWAY

    Q_ASSERT(inboundFrame.type() == FrameType::GOAWAY);
    // RFC 9113, 6.8: An endpoint MUST treat a GOAWAY frame with a stream identifier
    // other than 0x0 as a connection error (Section 5.4.1) of type PROTOCOL_ERROR.
    if (inboundFrame.streamID() != connectionStreamID)
        return connectionError(PROTOCOL_ERROR, "GOAWAY on invalid stream");

    // RFC 9113, 6.8:
    // Reserved (1) + Last-Stream-ID (31) + Error Code (32) + Additional Debug Data (..)
    // checked in Frame::validateHeader()
    Q_ASSERT(inboundFrame.payloadSize() >= 8);

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
    // RFC 9113, 6.9: A receiver MUST treat the receipt of a WINDOW_UPDATE frame with a
    // flow-control window increment of 0 as a stream error (Section 5.4.2) of type PROTOCOL_ERROR;
    // errors on the connection flow-control window MUST be treated as a connection error
    const bool valid = delta && delta <= quint32(std::numeric_limits<qint32>::max());
    const auto streamID = inboundFrame.streamID();


    // RFC 9113, 6.9: A WINDOW_UPDATE frame with a length other than 4 octets MUST be treated
    // as a connection error (Section 5.4.1) of type FRAME_SIZE_ERROR.
    // checked in Frame::validateHeader()
    Q_ASSERT(inboundFrame.payloadSize() == 4);

    qCDebug(qHttp2ConnectionLog(), "[%p] Received WINDOW_UPDATE, stream %d, delta %d", this,
            streamID, delta);
    if (streamID == connectionStreamID) {
        qint32 sum = 0;
        if (!valid || qAddOverflow(sessionSendWindowSize, qint32(delta), &sum))
            return connectionError(PROTOCOL_ERROR, "WINDOW_UPDATE invalid delta");
        sessionSendWindowSize = sum;

        // Stream may have been unblocked, so maybe try to write again:
        const auto blockedStreams = std::exchange(m_blockedStreams, {});
        for (quint32 blockedStreamID : blockedStreams) {
            const QPointer<QHttp2Stream> stream = m_streams.value(blockedStreamID);
            if (!stream || !stream->isActive())
                continue;
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
        } else if (!valid) {
            return stream->streamError(PROTOCOL_ERROR,
                                       QLatin1String("WINDOW_UPDATE invalid delta"));
        }
        stream->handleWINDOW_UPDATE(inboundFrame);
    }
}

void QHttp2Connection::handleCONTINUATION()
{
    Q_ASSERT(inboundFrame.type() == FrameType::CONTINUATION);
    auto streamID = inboundFrame.streamID();
    qCDebug(qHttp2ConnectionLog,
            "[%p] Received CONTINUATION frame on stream %d, end stream? %s", this, streamID,
            inboundFrame.flags().testFlag(Http2::FrameFlag::END_STREAM) ? "yes" : "no");
    if (continuedFrames.empty())
        return connectionError(PROTOCOL_ERROR,
                               "CONTINUATION without a preceding HEADERS or PUSH_PROMISE");
    if (!continuationExpected)
        return connectionError(PROTOCOL_ERROR,
                               "CONTINUATION after a frame with the END_HEADERS flag set");

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

    const auto streamIt = m_streams.constFind(streamID);
    if (firstFrameType == FrameType::HEADERS) {
        if (streamIt != m_streams.cend() && !streamWasResetLocally(streamID)) {
            QHttp2Stream *stream = streamIt.value();
            if (stream->state() != QHttp2Stream::State::HalfClosedLocal
                && stream->state() != QHttp2Stream::State::ReservedRemote
                && stream->state() != QHttp2Stream::State::Idle
                && stream->state() != QHttp2Stream::State::Open) {
                // We can receive HEADERS on streams initiated by our requests
                // (these streams are in halfClosedLocal or open state) or
                // remote-reserved streams from a server's PUSH_PROMISE.
                return stream->streamError(PROTOCOL_ERROR, "HEADERS on invalid stream"_L1);
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
            if (streamIt != m_streams.cend()) {
                (*streamIt)->streamError(PROTOCOL_ERROR,
                                         QLatin1String("PUSH_PROMISE with incomplete headers"));
            }
            return;
        }

        // We got back an empty hpack block. Now let's figure out if there was an error.
        constexpr auto hpackBlockHasContent = [](const auto &c) { return c.hpackBlockSize() > 0; };
        const bool anyHpackBlock = std::any_of(continuedFrames.cbegin(), continuedFrames.cend(),
                                               hpackBlockHasContent);
        if (anyHpackBlock) // There was hpack block data, but returned empty => it overflowed.
            return connectionError(FRAME_SIZE_ERROR, "HEADERS frame too large");
    }

    if (streamWasResetLocally(streamID) || streamIt == m_streams.cend())
        return; // No more processing without a stream from here on.

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
        if (!pendingTableSizeUpdates[0] && encoder.dynamicTableCapacity() == newValue) {
            qCDebug(qHttp2ConnectionLog,
                    "[%p] Ignoring SETTINGS HEADER_TABLE_SIZE %d (same as current value)", this,
                    newValue);
            break;
        }

        if (pendingTableSizeUpdates[0].value_or(std::numeric_limits<quint32>::max()) >= newValue) {
            pendingTableSizeUpdates[0] = newValue;
            pendingTableSizeUpdates[1].reset(); // 0 is the latest _and_ smallest, so we don't need 1
            qCDebug(qHttp2ConnectionLog, "[%p] Pending table size update to %u", this, newValue);
        } else {
            pendingTableSizeUpdates[1] = newValue; // newValue was larger than 0, so it goes to 1
            qCDebug(qHttp2ConnectionLog, "[%p] Pending 2nd table size update to %u, smallest is %u",
                    this, newValue, *pendingTableSizeUpdates[0]);
        }
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
            if (!stream)
                continue;
            qint32 sum = 0;
            if (qAddOverflow(stream->m_sendWindow, delta, &sum)) {
                stream->streamError(PROTOCOL_ERROR, "SETTINGS window overflow"_L1);
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
        m_peerMaxConcurrentStreams = newValue;
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
