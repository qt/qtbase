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

#include "http2frames_p.h"

#include <QtNetwork/qabstractsocket.h>

#include <algorithm>
#include <utility>

QT_BEGIN_NAMESPACE

namespace Http2
{

// HTTP/2 frames are defined by RFC7540, clauses 4 and 6.

Frame::Frame()
    : buffer(frameHeaderSize)
{
}

FrameType Frame::type() const
{
    Q_ASSERT(buffer.size() >= frameHeaderSize);

    if (int(buffer[3]) >= int(FrameType::LAST_FRAME_TYPE))
        return FrameType::LAST_FRAME_TYPE;

    return FrameType(buffer[3]);
}

quint32 Frame::streamID() const
{
    Q_ASSERT(buffer.size() >= frameHeaderSize);
    return qFromBigEndian<quint32>(&buffer[5]);
}

FrameFlags Frame::flags() const
{
    Q_ASSERT(buffer.size() >= frameHeaderSize);
    return FrameFlags(buffer[4]);
}

quint32 Frame::payloadSize() const
{
    Q_ASSERT(buffer.size() >= frameHeaderSize);
    return buffer[0] << 16 | buffer[1] << 8 | buffer[2];
}

uchar Frame::padding() const
{
    Q_ASSERT(validateHeader() == FrameStatus::goodFrame);

    if (!flags().testFlag(FrameFlag::PADDED))
        return 0;

    switch (type()) {
    case FrameType::DATA:
    case FrameType::PUSH_PROMISE:
    case FrameType::HEADERS:
        Q_ASSERT(buffer.size() > frameHeaderSize);
        return buffer[frameHeaderSize];
    default:
        return 0;
    }
}

bool Frame::priority(quint32 *streamID, uchar *weight) const
{
    Q_ASSERT(validatePayload() == FrameStatus::goodFrame);

    if (buffer.size() <= frameHeaderSize)
        return false;

    const uchar *src = &buffer[0] + frameHeaderSize;
    if (type() == FrameType::HEADERS && flags().testFlag(FrameFlag::PADDED))
        ++src;

    if ((type() == FrameType::HEADERS && flags().testFlag(FrameFlag::PRIORITY))
        || type() == FrameType::PRIORITY) {
        if (streamID)
            *streamID = qFromBigEndian<quint32>(src);
        if (weight)
            *weight = src[4];
        return true;
    }

    return false;
}

FrameStatus Frame::validateHeader() const
{
    // Should be called only on a frame with
    // a complete header.
    Q_ASSERT(buffer.size() >= frameHeaderSize);

    const auto framePayloadSize = payloadSize();
    // 4.2 Frame Size
    if (framePayloadSize > maxPayloadSize)
        return FrameStatus::sizeError;

    switch (type()) {
    case FrameType::SETTINGS:
        // SETTINGS ACK can not have any payload.
        // The payload of a SETTINGS frame consists of zero
        // or more parameters, each consisting of an unsigned
        // 16-bit setting identifier and an unsigned 32-bit value.
        // Thus the payload size must be a multiple of 6.
        if (flags().testFlag(FrameFlag::ACK) ? framePayloadSize : framePayloadSize % 6)
            return FrameStatus::sizeError;
        break;
    case FrameType::PRIORITY:
        // 6.3 PRIORITY
        if (framePayloadSize != 5)
            return FrameStatus::sizeError;
        break;
    case FrameType::PING:
        // 6.7 PING
        if (framePayloadSize != 8)
            return FrameStatus::sizeError;
        break;
    case FrameType::GOAWAY:
        // 6.8 GOAWAY
        if (framePayloadSize < 8)
            return FrameStatus::sizeError;
        break;
    case FrameType::RST_STREAM:
    case FrameType::WINDOW_UPDATE:
        // 6.4 RST_STREAM, 6.9 WINDOW_UPDATE
        if (framePayloadSize != 4)
            return FrameStatus::sizeError;
        break;
    case FrameType::PUSH_PROMISE:
        // 6.6 PUSH_PROMISE
        if (framePayloadSize < 4)
            return FrameStatus::sizeError;
    default:
        // DATA/HEADERS/CONTINUATION will be verified
        // when we have payload.
        // Frames of unknown types are ignored (5.1)
        break;
    }

    return FrameStatus::goodFrame;
}

FrameStatus Frame::validatePayload() const
{
    // Should be called only on a complete frame with a valid header.
    Q_ASSERT(validateHeader() == FrameStatus::goodFrame);

    // Ignored, 5.1
    if (type() == FrameType::LAST_FRAME_TYPE)
        return FrameStatus::goodFrame;

    auto size = payloadSize();
    Q_ASSERT(buffer.size() >= frameHeaderSize && size == buffer.size() - frameHeaderSize);

    const uchar *src = size ? &buffer[0] + frameHeaderSize : nullptr;
    const auto frameFlags = flags();
    switch (type()) {
    // 6.1 DATA, 6.2 HEADERS
    case FrameType::DATA:
    case FrameType::HEADERS:
        if (frameFlags.testFlag(FrameFlag::PADDED)) {
            if (!size || size < src[0])
                return FrameStatus::sizeError;
            size -= src[0];
        }
        if (type() == FrameType::HEADERS && frameFlags.testFlag(FrameFlag::PRIORITY)) {
            if (size < 5)
                return FrameStatus::sizeError;
        }
        break;
    // 6.6 PUSH_PROMISE
    case FrameType::PUSH_PROMISE:
        if (frameFlags.testFlag(FrameFlag::PADDED)) {
            if (!size || size < src[0])
                return FrameStatus::sizeError;
            size -= src[0];
        }

        if (size < 4)
            return FrameStatus::sizeError;
        break;
    default:
        break;
    }

    return FrameStatus::goodFrame;
}


quint32 Frame::dataSize() const
{
    Q_ASSERT(validatePayload() == FrameStatus::goodFrame);

    quint32 size = payloadSize();
    if (const uchar pad = padding()) {
        // + 1 one for a byte with padding number itself:
        size -= pad + 1;
    }

    if (priority())
        size -= 5;

    return size;
}

quint32 Frame::hpackBlockSize() const
{
    Q_ASSERT(validatePayload() == FrameStatus::goodFrame);

    const auto frameType = type();
    Q_ASSERT(frameType == FrameType::HEADERS ||
             frameType == FrameType::PUSH_PROMISE ||
             frameType == FrameType::CONTINUATION);

    quint32 size = dataSize();
    if (frameType == FrameType::PUSH_PROMISE) {
        Q_ASSERT(size >= 4);
        size -= 4;
    }

    return size;
}

const uchar *Frame::dataBegin() const
{
    Q_ASSERT(validatePayload() == FrameStatus::goodFrame);
    if (buffer.size() <= frameHeaderSize)
        return nullptr;

    const uchar *src = &buffer[0] + frameHeaderSize;
    if (padding())
        ++src;

    if (priority())
        src += 5;

    return src;
}

const uchar *Frame::hpackBlockBegin() const
{
    Q_ASSERT(validatePayload() == FrameStatus::goodFrame);

    const auto frameType = type();
    Q_ASSERT(frameType == FrameType::HEADERS ||
             frameType == FrameType::PUSH_PROMISE ||
             frameType == FrameType::CONTINUATION);

    const uchar *begin = dataBegin();
    if (frameType == FrameType::PUSH_PROMISE)
        begin += 4; // That's a promised stream, skip it.
    return begin;
}

FrameStatus FrameReader::read(QAbstractSocket &socket)
{
    if (offset < frameHeaderSize) {
        if (!readHeader(socket))
            return FrameStatus::incompleteFrame;

        const auto status = frame.validateHeader();
        if (status != FrameStatus::goodFrame) {
            // No need to read any payload.
            return status;
        }

        if (Http2PredefinedParameters::maxFrameSize < frame.payloadSize())
            return FrameStatus::sizeError;

        frame.buffer.resize(frame.payloadSize() + frameHeaderSize);
    }

    if (offset < frame.buffer.size() && !readPayload(socket))
        return FrameStatus::incompleteFrame;

    // Reset the offset, our frame can be re-used
    // now (re-read):
    offset = 0;

    return frame.validatePayload();
}

bool FrameReader::readHeader(QAbstractSocket &socket)
{
    Q_ASSERT(offset < frameHeaderSize);

    auto &buffer = frame.buffer;
    if (buffer.size() < frameHeaderSize)
        buffer.resize(frameHeaderSize);

    const auto chunkSize = socket.read(reinterpret_cast<char *>(&buffer[offset]),
                                       frameHeaderSize - offset);
    if (chunkSize > 0)
        offset += chunkSize;

    return offset == frameHeaderSize;
}

bool FrameReader::readPayload(QAbstractSocket &socket)
{
    Q_ASSERT(offset < frame.buffer.size());
    Q_ASSERT(frame.buffer.size() > frameHeaderSize);

    auto &buffer = frame.buffer;
    // Casts and ugliness - to deal with MSVC. Values are guaranteed to fit into quint32.
    const auto chunkSize = socket.read(reinterpret_cast<char *>(&buffer[offset]),
                                       qint64(buffer.size() - offset));
    if (chunkSize > 0)
        offset += quint32(chunkSize);

    return offset == buffer.size();
}

FrameWriter::FrameWriter()
{
}

FrameWriter::FrameWriter(FrameType type, FrameFlags flags, quint32 streamID)
{
    start(type, flags, streamID);
}

void FrameWriter::setOutboundFrame(Frame &&newFrame)
{
    frame = std::move(newFrame);
    updatePayloadSize();
}

void FrameWriter::start(FrameType type, FrameFlags flags, quint32 streamID)
{
    auto &buffer = frame.buffer;

    buffer.resize(frameHeaderSize);
    // The first three bytes - payload size, which is 0 for now.
    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = 0;

    buffer[3] = uchar(type);
    buffer[4] = uchar(flags);

    qToBigEndian(streamID, &buffer[5]);
}

void FrameWriter::setPayloadSize(quint32 size)
{
    auto &buffer = frame.buffer;

    Q_ASSERT(buffer.size() >= frameHeaderSize);
    Q_ASSERT(size < maxPayloadSize);

    buffer[0] = size >> 16;
    buffer[1] = size >> 8;
    buffer[2] = size;
}

void FrameWriter::setType(FrameType type)
{
    Q_ASSERT(frame.buffer.size() >= frameHeaderSize);
    frame.buffer[3] = uchar(type);
}

void FrameWriter::setFlags(FrameFlags flags)
{
    Q_ASSERT(frame.buffer.size() >= frameHeaderSize);
    frame.buffer[4] = uchar(flags);
}

void FrameWriter::addFlag(FrameFlag flag)
{
    setFlags(frame.flags() | flag);
}

void FrameWriter::append(const uchar *begin, const uchar *end)
{
    Q_ASSERT(begin && end);
    Q_ASSERT(begin < end);

    frame.buffer.insert(frame.buffer.end(), begin, end);
    updatePayloadSize();
}

void FrameWriter::updatePayloadSize()
{
    const quint32 size = quint32(frame.buffer.size() - frameHeaderSize);
    Q_ASSERT(size <= maxPayloadSize);
    setPayloadSize(size);
}

bool FrameWriter::write(QAbstractSocket &socket) const
{
    auto &buffer = frame.buffer;
    Q_ASSERT(buffer.size() >= frameHeaderSize);
    // Do some sanity check first:

    Q_ASSERT(int(frame.type()) < int(FrameType::LAST_FRAME_TYPE));
    Q_ASSERT(frame.validateHeader() == FrameStatus::goodFrame);

    const auto nWritten = socket.write(reinterpret_cast<const char *>(&buffer[0]),
                                       buffer.size());
    return nWritten != -1 && size_type(nWritten) == buffer.size();
}

bool FrameWriter::writeHEADERS(QAbstractSocket &socket, quint32 sizeLimit)
{
    auto &buffer = frame.buffer;
    Q_ASSERT(buffer.size() >= frameHeaderSize);

    if (sizeLimit > quint32(maxPayloadSize))
        sizeLimit = quint32(maxPayloadSize);

    if (quint32(buffer.size() - frameHeaderSize) <= sizeLimit) {
        addFlag(FrameFlag::END_HEADERS);
        updatePayloadSize();
        return write(socket);
    }

    // Our HPACK block does not fit into the size limit, remove
    // END_HEADERS bit from the first frame, we'll later set
    // it on the last CONTINUATION frame:
    setFlags(frame.flags() & ~FrameFlags(FrameFlag::END_HEADERS));
    // Write a frame's header (not controlled by sizeLimit) and
    // as many bytes of payload as we can within sizeLimit,
    // then send CONTINUATION frames, as needed.
    setPayloadSize(sizeLimit);
    const quint32 firstChunkSize = frameHeaderSize + sizeLimit;
    qint64 written = socket.write(reinterpret_cast<const char *>(&buffer[0]),
                                  firstChunkSize);

    if (written != qint64(firstChunkSize))
        return false;

    FrameWriter continuationWriter(FrameType::CONTINUATION, FrameFlag::EMPTY, frame.streamID());
    quint32 offset = firstChunkSize;

    while (offset != buffer.size()) {
        const auto chunkSize = std::min(sizeLimit, quint32(buffer.size() - offset));
        if (chunkSize + offset == buffer.size())
            continuationWriter.addFlag(FrameFlag::END_HEADERS);
        continuationWriter.setPayloadSize(chunkSize);
        if (!continuationWriter.write(socket))
            return false;
        written = socket.write(reinterpret_cast<const char *>(&buffer[offset]),
                               chunkSize);
        if (written != qint64(chunkSize))
            return false;

        offset += chunkSize;
    }

    return true;
}

bool FrameWriter::writeDATA(QAbstractSocket &socket, quint32 sizeLimit,
                            const uchar *src, quint32 size)
{
    // With DATA frame(s) we always have:
    // 1) frame's header (9 bytes)
    // 2) a separate payload (from QNonContiguousByteDevice).
    // We either fit within a sizeLimit, or split into several
    // DATA frames.

    Q_ASSERT(src);

    if (sizeLimit > quint32(maxPayloadSize))
        sizeLimit = quint32(maxPayloadSize);
    // We NEVER set END_STREAM, since QHttp2ProtocolHandler works with
    // QNonContiguousByteDevice and this 'writeDATA' is probably
    // not the last one for a given request.
    // This has to be done externally (sending an empty DATA frame with END_STREAM).
    for (quint32 offset = 0; offset != size;) {
        const auto chunkSize = std::min(size - offset, sizeLimit);
        setPayloadSize(chunkSize);
        // Frame's header first:
        if (!write(socket))
            return false;
        // Payload (if any):
        if (chunkSize) {
            const auto written = socket.write(reinterpret_cast<const char*>(src + offset),
                                              chunkSize);
            if (written != qint64(chunkSize))
                return false;
        }

        offset += chunkSize;
    }

    return true;
}

} // Namespace Http2

QT_END_NAMESPACE
