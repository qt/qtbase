// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:network-protocol

#include "http2streams_p.h"

#include "private/qhttp2protocolhandler_p.h"
#include "private/qhttpnetworkreply_p.h"

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

namespace Http2
{

Stream::Stream()
{
}

Stream::Stream(const HttpMessagePair &message, quint32 id, qint32 sendSize, qint32 recvSize)
    : httpPair(message),
      streamID(id),
      sendWindow(sendSize),
      recvWindow(recvSize)
{
}

Stream::Stream(const QString &cacheKey, quint32 id, qint32 recvSize)
    : streamID(id),
      // sendWindow is 0, this stream only receives data
      recvWindow(recvSize),
      state(remoteReserved),
      key(cacheKey)
{
}

QHttpNetworkReply *Stream::reply() const
{
    return httpPair.second;
}

const QHttpNetworkRequest &Stream::request() const
{
    return httpPair.first;
}

QHttpNetworkRequest &Stream::request()
{
    return httpPair.first;
}

QHttpNetworkRequest::Priority Stream::priority() const
{
    return httpPair.first.priority();
}

uchar Stream::weight() const
{
    switch (priority()) {
    case QHttpNetworkRequest::LowPriority:
        return 0;
    case QHttpNetworkRequest::NormalPriority:
        return 127;
    case QHttpNetworkRequest::HighPriority:
    default:
        return 255;
    }
}

QNonContiguousByteDevice *Stream::data() const
{
    return httpPair.first.uploadByteDevice();
}

} // namespace Http2

QT_END_NAMESPACE
