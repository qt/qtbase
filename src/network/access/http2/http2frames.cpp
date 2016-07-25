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

FrameStatus validate_frame_header(FrameType type, FrameFlags flags, quint32 payloadSize)
{
    // 4.2 Frame Size
    if (payloadSize > maxPayloadSize)
        return FrameStatus::sizeError;

    switch (type) {
    case FrameType::SETTINGS:
        // SETTINGS ACK can not have any payload.
        // The payload of a SETTINGS frame consists of zero
        // or more parameters, each consisting of an unsigned
        // 16-bit setting identifier and an unsigned 32-bit value.
        // Thus the payload size must be a multiple of 6.
        if (flags.testFlag(FrameFlag::ACK) ? payloadSize : payloadSize % 6)
            return FrameStatus::sizeError;
        break;
    case FrameType::PRIORITY:
        // 6.3 PRIORITY
        if (payloadSize != 5)
            return FrameStatus::sizeError;
        break;
    case FrameType::PING:
        // 6.7 PING
        if (payloadSize != 8)
            return FrameStatus::sizeError;
        break;
    case FrameType::GOAWAY:
        // 6.8 GOAWAY
        if (payloadSize < 8)
            return FrameStatus::sizeError;
        break;
    case FrameType::RST_STREAM:
    case FrameType::WINDOW_UPDATE:
        // 6.4 RST_STREAM, 6.9 WINDOW_UPDATE
        if (payloadSize != 4)
            return FrameStatus::sizeError;
        break;
    case FrameType::PUSH_PROMISE:
        // 6.6 PUSH_PROMISE
        if (payloadSize < 4)
            return FrameStatus::sizeError;
    default:
        // DATA/HEADERS/CONTINUATION will be verified
        // when we have payload.
        // Frames of unknown types are ignored (5.1)
        break;
    }

    return FrameStatus::goodFrame;
}

FrameStatus validate_frame_payload(FrameType type, FrameFlags flags,
                                   quint32 size, const uchar *src)
{
    Q_ASSERT(!size || src);

    // Ignored, 5.1
    if (type == FrameType::LAST_FRAME_TYPE)
        return FrameStatus::goodFrame;

    // 6.1 DATA, 6.2 HEADERS
    if (type == FrameType::DATA || type == FrameType::HEADERS) {
        if (flags.testFlag(FrameFlag::PADDED)) {
            if (!size || size < src[0])
                return FrameStatus::sizeError;
            size -= src[0];
        }
        if (type == FrameType::HEADERS && flags.testFlag(FrameFlag::PRIORITY)) {
            if (size < 5)
                return FrameStatus::sizeError;
        }
    }

    // 6.6 PUSH_PROMISE
    if (type == FrameType::PUSH_PROMISE) {
        if (flags.testFlag(FrameFlag::PADDED)) {
            if (!size || size < src[0])
                return FrameStatus::sizeError;
            size -= src[0];
        }

        if (size < 4)
            return FrameStatus::sizeError;
    }

    return FrameStatus::goodFrame;
}

FrameStatus validate_frame_payload(FrameType type, FrameFlags flags,
                                   const std::vector<uchar> &payload)
{
    const uchar *src = payload.size() ? &payload[0] : nullptr;
    return validate_frame_payload(type, flags, quint32(payload.size()), src);
}


FrameReader::FrameReader(FrameReader &&rhs)
    : framePayload(std::move(rhs.framePayload))
{
    type = rhs.type;
    rhs.type = FrameType::LAST_FRAME_TYPE;

    flags = rhs.flags;
    rhs.flags = FrameFlag::EMPTY;

    streamID = rhs.streamID;
    rhs.streamID = 0;

    payloadSize = rhs.payloadSize;
    rhs.payloadSize = 0;

    incompleteRead = rhs.incompleteRead;
    rhs.incompleteRead = false;

    offset = rhs.offset;
    rhs.offset = 0;
}

FrameReader &FrameReader::operator = (FrameReader &&rhs)
{
    framePayload = std::move(rhs.framePayload);

    type = rhs.type;
    rhs.type = FrameType::LAST_FRAME_TYPE;

    flags = rhs.flags;
    rhs.flags = FrameFlag::EMPTY;

    streamID = rhs.streamID;
    rhs.streamID = 0;

    payloadSize = rhs.payloadSize;
    rhs.payloadSize = 0;

    incompleteRead = rhs.incompleteRead;
    rhs.incompleteRead = false;

    offset = rhs.offset;
    rhs.offset = 0;

    return *this;
}

FrameStatus FrameReader::read(QAbstractSocket &socket)
{
    if (!incompleteRead) {
        if (!readHeader(socket))
            return FrameStatus::incompleteFrame;

        const auto status = validate_frame_header(type, flags, payloadSize);
        if (status != FrameStatus::goodFrame) {
            // No need to read any payload.
            return status;
        }

        if (Http2PredefinedParameters::maxFrameSize < payloadSize)
            return FrameStatus::sizeError;

        framePayload.resize(payloadSize);
        offset = 0;
    }

    if (framePayload.size()) {
        if (!readPayload(socket))
            return FrameStatus::incompleteFrame;
    }

    return validate_frame_payload(type, flags, framePayload);
}

bool FrameReader::padded(uchar *pad) const
{
    Q_ASSERT(pad);

    if (!flags.testFlag(FrameFlag::PADDED))
        return false;

    if (type == FrameType::DATA
        || type == FrameType::PUSH_PROMISE
        || type == FrameType::HEADERS) {
        Q_ASSERT(framePayload.size() >= 1);
        *pad = framePayload[0];
        return true;
    }

    return false;
}

bool FrameReader::priority(quint32 *streamID, uchar *weight) const
{
    Q_ASSERT(streamID);
    Q_ASSERT(weight);

    if (!framePayload.size())
        return false;

    const uchar *src = &framePayload[0];
    if (type == FrameType::HEADERS && flags.testFlag(FrameFlag::PADDED))
        ++src;

    if ((type == FrameType::HEADERS && flags.testFlag(FrameFlag::PRIORITY))
        || type == FrameType::PRIORITY) {
        *streamID = qFromBigEndian<quint32>(src);
        *weight = src[4];
        return true;
    }

    return false;
}

quint32 FrameReader::dataSize() const
{
    quint32 size = quint32(framePayload.size());
    uchar pad = 0;
    if (padded(&pad)) {
        // + 1 one for a byte with padding number itself:
        size -= pad + 1;
    }

    quint32 dummyID = 0;
    uchar dummyW = 0;
    if (priority(&dummyID, &dummyW))
        size -= 5;

    return size;
}

const uchar *FrameReader::dataBegin() const
{
    if (!framePayload.size())
        return nullptr;

    const uchar *src = &framePayload[0];
    uchar dummyPad = 0;
    if (padded(&dummyPad))
        ++src;

    quint32 dummyID = 0;
    uchar dummyW = 0;
    if (priority(&dummyID, &dummyW))
        src += 5;

    return src;
}

bool FrameReader::readHeader(QAbstractSocket &socket)
{
    if (socket.bytesAvailable() < frameHeaderSize)
        return false;

    uchar src[frameHeaderSize] = {};
    socket.read(reinterpret_cast<char*>(src), frameHeaderSize);

    payloadSize = src[0] << 16 | src[1] << 8 | src[2];

    type = FrameType(src[3]);
    if (int(type) >= int(FrameType::LAST_FRAME_TYPE))
        type = FrameType::LAST_FRAME_TYPE; // To be ignored, 5.1

    flags = FrameFlags(src[4]);
    streamID = qFromBigEndian<quint32>(src + 5);

    return true;
}

bool FrameReader::readPayload(QAbstractSocket &socket)
{
    Q_ASSERT(offset <= framePayload.size());

    // Casts and ugliness - to deal with MSVC. Values are guaranteed to fit into quint32.
    if (const auto residue = std::min(qint64(framePayload.size() - offset), socket.bytesAvailable())) {
        socket.read(reinterpret_cast<char *>(&framePayload[offset]), residue);
        offset += quint32(residue);
    }

    incompleteRead = offset < framePayload.size();
    return !incompleteRead;
}


FrameWriter::FrameWriter()
{
    frameBuffer.reserve(Http2PredefinedParameters::maxFrameSize +
                        Http2PredefinedParameters::frameHeaderSize);
}

FrameWriter::FrameWriter(FrameType type, FrameFlags flags, quint32 streamID)
{
    frameBuffer.reserve(Http2PredefinedParameters::maxFrameSize +
                        Http2PredefinedParameters::frameHeaderSize);
    start(type, flags, streamID);
}

void FrameWriter::start(FrameType type, FrameFlags flags, quint32 streamID)
{
    frameBuffer.resize(frameHeaderSize);
    // The first three bytes - payload size, which is 0 for now.
    frameBuffer[0] = 0;
    frameBuffer[1] = 0;
    frameBuffer[2] = 0;

    frameBuffer[3] = uchar(type);
    frameBuffer[4] = uchar(flags);

    qToBigEndian(streamID, &frameBuffer[5]);
}

void FrameWriter::setPayloadSize(quint32 size)
{
    Q_ASSERT(frameBuffer.size() >= frameHeaderSize);
    Q_ASSERT(size < maxPayloadSize);

    frameBuffer[0] = size >> 16;
    frameBuffer[1] = size >> 8;
    frameBuffer[2] = size;
}

quint32 FrameWriter::payloadSize() const
{
    Q_ASSERT(frameBuffer.size() >= frameHeaderSize);
    return frameBuffer[0] << 16 | frameBuffer[1] << 8 | frameBuffer[2];
}

void FrameWriter::setType(FrameType type)
{
    Q_ASSERT(frameBuffer.size() >= frameHeaderSize);
    frameBuffer[3] = uchar(type);
}

FrameType FrameWriter::type() const
{
    Q_ASSERT(frameBuffer.size() >= frameHeaderSize);
    return FrameType(frameBuffer[3]);
}

void FrameWriter::setFlags(FrameFlags flags)
{
    Q_ASSERT(frameBuffer.size() >= frameHeaderSize);
    frameBuffer[4] = uchar(flags);
}

void FrameWriter::addFlag(FrameFlag flag)
{
    setFlags(flags() | flag);
}

FrameFlags FrameWriter::flags() const
{
    Q_ASSERT(frameBuffer.size() >= frameHeaderSize);
    return FrameFlags(frameBuffer[4]);
}

quint32 FrameWriter::streamID() const
{
    return qFromBigEndian<quint32>(&frameBuffer[5]);
}

void FrameWriter::append(uchar val)
{
    frameBuffer.push_back(val);
    updatePayloadSize();
}

void FrameWriter::append(const uchar *begin, const uchar *end)
{
    Q_ASSERT(begin && end);
    Q_ASSERT(begin < end);

    frameBuffer.insert(frameBuffer.end(), begin, end);
    updatePayloadSize();
}

void FrameWriter::updatePayloadSize()
{
    // First, compute size:
    const quint32 payloadSize = quint32(frameBuffer.size() - frameHeaderSize);
    Q_ASSERT(payloadSize <= maxPayloadSize);
    setPayloadSize(payloadSize);
}

bool FrameWriter::write(QAbstractSocket &socket) const
{
    Q_ASSERT(frameBuffer.size() >= frameHeaderSize);
    // Do some sanity check first:
    Q_ASSERT(int(type()) < int(FrameType::LAST_FRAME_TYPE));
    Q_ASSERT(validate_frame_header(type(), flags(), payloadSize()) == FrameStatus::goodFrame);

    const auto nWritten = socket.write(reinterpret_cast<const char *>(&frameBuffer[0]),
                                       frameBuffer.size());
    return nWritten != -1 && size_type(nWritten) == frameBuffer.size();
}

bool FrameWriter::writeHEADERS(QAbstractSocket &socket, quint32 sizeLimit)
{
    Q_ASSERT(frameBuffer.size() >= frameHeaderSize);

    if (sizeLimit > quint32(maxPayloadSize))
        sizeLimit = quint32(maxPayloadSize);

    if (quint32(frameBuffer.size() - frameHeaderSize) <= sizeLimit) {
        updatePayloadSize();
        return write(socket);
    }

    // Write a frame's header (not controlled by sizeLimit) and
    // as many bytes of payload as we can within sizeLimit,
    // then send CONTINUATION frames, as needed.
    setPayloadSize(sizeLimit);
    const quint32 firstChunkSize = frameHeaderSize + sizeLimit;
    qint64 written = socket.write(reinterpret_cast<const char *>(&frameBuffer[0]),
                                  firstChunkSize);

    if (written != qint64(firstChunkSize))
        return false;

    FrameWriter continuationFrame(FrameType::CONTINUATION, FrameFlag::EMPTY, streamID());
    quint32 offset = firstChunkSize;

    while (offset != frameBuffer.size()) {
        const auto chunkSize = std::min(sizeLimit, quint32(frameBuffer.size() - offset));
        if (chunkSize + offset == frameBuffer.size())
            continuationFrame.addFlag(FrameFlag::END_HEADERS);
        continuationFrame.setPayloadSize(chunkSize);
        if (!continuationFrame.write(socket))
            return false;
        written = socket.write(reinterpret_cast<const char *>(&frameBuffer[offset]),
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

}

QT_END_NAMESPACE
