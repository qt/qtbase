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

#ifndef HTTP2FRAMES_P_H
#define HTTP2FRAMES_P_H

#include "http2protocol_p.h"
#include "hpack_p.h"

#include <QtCore/qendian.h>
#include <QtCore/qglobal.h>

#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

class QHttp2ProtocolHandler;
class QAbstractSocket;

namespace Http2
{

class Q_AUTOTEST_EXPORT FrameReader
{
    friend class QT_PREPEND_NAMESPACE(QHttp2ProtocolHandler);

public:
    FrameReader() = default;

    FrameReader(const FrameReader &) = default;
    FrameReader(FrameReader &&rhs);

    FrameReader &operator = (const FrameReader &) = default;
    FrameReader &operator = (FrameReader &&rhs);

    FrameStatus read(QAbstractSocket &socket);

    bool padded(uchar *pad) const;
    bool priority(quint32 *streamID, uchar *weight) const;

    // N of bytes without padding and/or priority
    quint32 dataSize() const;
    // Beginning of payload without priority/padding
    // bytes.
    const uchar *dataBegin() const;

    FrameType type = FrameType::LAST_FRAME_TYPE;
    FrameFlags flags = FrameFlag::EMPTY;
    quint32 streamID = 0;
    quint32 payloadSize = 0;

private:
    bool readHeader(QAbstractSocket &socket);
    bool readPayload(QAbstractSocket &socket);

    // As soon as we got a header, we
    // know payload size, offset is
    // needed if we do not have enough
    // data and will read the next chunk.
    bool incompleteRead = false;
    quint32 offset = 0;
    std::vector<uchar> framePayload;
};

class Q_AUTOTEST_EXPORT FrameWriter
{
    friend class QT_PREPEND_NAMESPACE(QHttp2ProtocolHandler);

public:
    using payload_type = std::vector<uchar>;
    using size_type = payload_type::size_type;

    FrameWriter();
    FrameWriter(FrameType type, FrameFlags flags, quint32 streamID);

    void start(FrameType type, FrameFlags flags, quint32 streamID);

    void setPayloadSize(quint32 size);
    quint32 payloadSize() const;

    void setType(FrameType type);
    FrameType type() const;

    void setFlags(FrameFlags flags);
    void addFlag(FrameFlag flag);
    FrameFlags flags() const;

    quint32 streamID() const;

    // All append functions also update frame's payload
    // length.
    template<typename ValueType>
    void append(ValueType val)
    {
        uchar wired[sizeof val] = {};
        qToBigEndian(val, wired);
        append(wired, wired + sizeof val);
    }
    void append(uchar val);
    void append(Settings identifier)
    {
        append(quint16(identifier));
    }
    void append(const payload_type &payload)
    {
        append(&payload[0], &payload[0] + payload.size());
    }

    void append(const uchar *begin, const uchar *end);

    // Write 'frameBuffer' as a single frame:
    bool write(QAbstractSocket &socket) const;
    // Write as a single frame if we can, or write headers and
    // CONTINUATION(s) frame(s).
    bool writeHEADERS(QAbstractSocket &socket, quint32 sizeLimit);
    // Write either a single DATA frame or several DATA frames
    // depending on 'sizeLimit'. Data itself is 'external' to
    // FrameWriter, since it's a 'readPointer' from QNonContiguousData.
    bool writeDATA(QAbstractSocket &socket, quint32 sizeLimit,
                   const uchar *src, quint32 size);

    std::vector<uchar> &rawFrameBuffer()
    {
        return frameBuffer;
    }

private:
    void updatePayloadSize();
    std::vector<uchar> frameBuffer;
};

}

QT_END_NAMESPACE

#endif
