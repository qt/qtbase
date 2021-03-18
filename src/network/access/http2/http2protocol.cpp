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

#include "http2protocol_p.h"
#include "http2frames_p.h"

#include "private/qhttpnetworkrequest_p.h"
#include "private/qhttpnetworkreply_p.h"

#include <access/qhttp2configuration.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_HTTP2, "qt.network.http2")

namespace Http2
{

// 3.5 HTTP/2 Connection Preface:
// "That is, the connection preface starts with the string
// PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n)."
const char Http2clientPreface[clientPrefaceLength] =
    {0x50, 0x52, 0x49, 0x20, 0x2a, 0x20,
     0x48, 0x54, 0x54, 0x50, 0x2f, 0x32,
     0x2e, 0x30, 0x0d, 0x0a, 0x0d, 0x0a,
     0x53, 0x4d, 0x0d, 0x0a, 0x0d, 0x0a};

Frame configurationToSettingsFrame(const QHttp2Configuration &config)
{
    // 6.5 SETTINGS
    FrameWriter builder(FrameType::SETTINGS, FrameFlag::EMPTY, connectionStreamID);
    // Server push:
    builder.append(Settings::ENABLE_PUSH_ID);
    builder.append(int(config.serverPushEnabled()));
    // Stream receive window size:
    builder.append(Settings::INITIAL_WINDOW_SIZE_ID);
    builder.append(config.streamReceiveWindowSize());

    if (config.maxFrameSize() != minPayloadLimit) {
        builder.append(Settings::MAX_FRAME_SIZE_ID);
        builder.append(config.maxFrameSize());
    }
    // TODO: In future, if the need is proven, we can
    // also send decoding table size and header list size.
    // For now, defaults suffice.
    return builder.outboundFrame();
}

QByteArray settingsFrameToBase64(const Frame &frame)
{
    // SETTINGS frame's payload consists of pairs:
    // 2-byte-identifier | 4-byte-value == multiple of 6.
    Q_ASSERT(frame.payloadSize() && !(frame.payloadSize() % 6));
    const char *src = reinterpret_cast<const char *>(frame.dataBegin());
    const QByteArray wrapper(QByteArray::fromRawData(src, int(frame.dataSize())));
    // 3.2.1
    // The content of the HTTP2-Settings header field is the payload
    // of a SETTINGS frame (Section 6.5), encoded as a base64url string
    // (that is, the URL- and filename-safe Base64 encoding described in
    // Section 5 of [RFC4648], with any trailing '=' characters omitted).
    return wrapper.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

void appendProtocolUpgradeHeaders(const QHttp2Configuration &config, QHttpNetworkRequest *request)
{
    Q_ASSERT(request);
    // RFC 2616, 14.10
    // RFC 7540, 3.2
    QByteArray value(request->headerField("Connection"));
    // We _append_ 'Upgrade':
    if (value.size())
        value += ", ";

    value += "Upgrade, HTTP2-Settings";
    request->setHeaderField("Connection", value);
    // This we just (re)write.
    request->setHeaderField("Upgrade", "h2c");

    const Frame frame(configurationToSettingsFrame(config));
    // This we just (re)write.
    request->setHeaderField("HTTP2-Settings", settingsFrameToBase64(frame));
}

void qt_error(quint32 errorCode, QNetworkReply::NetworkError &error,
              QString &errorMessage)
{
    if (errorCode > quint32(HTTP_1_1_REQUIRED)) {
        error = QNetworkReply::ProtocolFailure;
        errorMessage = QLatin1String("RST_STREAM with unknown error code (%1)");
        errorMessage = errorMessage.arg(errorCode);
        return;
    }

    const Http2Error http2Error = Http2Error(errorCode);

    switch (http2Error) {
    case HTTP2_NO_ERROR:
        error = QNetworkReply::NoError;
        errorMessage.clear();
        break;
    case PROTOCOL_ERROR:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = QLatin1String("HTTP/2 protocol error");
        break;
    case INTERNAL_ERROR:
        error = QNetworkReply::InternalServerError;
        errorMessage = QLatin1String("Internal server error");
        break;
    case FLOW_CONTROL_ERROR:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = QLatin1String("Flow control error");
        break;
    case SETTINGS_TIMEOUT:
        error = QNetworkReply::TimeoutError;
        errorMessage = QLatin1String("SETTINGS ACK timeout error");
        break;
    case STREAM_CLOSED:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = QLatin1String("Server received frame(s) on a half-closed stream");
        break;
    case FRAME_SIZE_ERROR:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = QLatin1String("Server received a frame with an invalid size");
        break;
    case REFUSE_STREAM:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = QLatin1String("Server refused a stream");
        break;
    case CANCEL:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = QLatin1String("Stream is no longer needed");
        break;
    case COMPRESSION_ERROR:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = QLatin1String("Server is unable to maintain the "
                                     "header compression context for the connection");
        break;
    case CONNECT_ERROR:
        // TODO: in Qt6 we'll have to add more error codes in QNetworkReply.
        error = QNetworkReply::UnknownNetworkError;
        errorMessage = QLatin1String("The connection established in response "
                        "to a CONNECT request was reset or abnormally closed");
        break;
    case ENHANCE_YOUR_CALM:
        error = QNetworkReply::UnknownServerError;
        errorMessage = QLatin1String("Server dislikes our behavior, excessive load detected.");
        break;
    case INADEQUATE_SECURITY:
        error = QNetworkReply::ContentAccessDenied;
        errorMessage = QLatin1String("The underlying transport has properties "
                                     "that do not meet minimum security "
                                     "requirements");
        break;
    case HTTP_1_1_REQUIRED:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = QLatin1String("Server requires that HTTP/1.1 "
                                     "be used instead of HTTP/2.");
    }
}

QString qt_error_string(quint32 errorCode)
{
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QString message;
    qt_error(errorCode, error, message);
    return message;
}

QNetworkReply::NetworkError qt_error(quint32 errorCode)
{
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QString message;
    qt_error(errorCode, error, message);
    return error;
}

bool is_protocol_upgraded(const QHttpNetworkReply &reply)
{
    if (reply.statusCode() == 101) {
        // Do some minimal checks here - we expect 'Upgrade: h2c' to be found.
        const auto &header = reply.header();
        for (const QPair<QByteArray, QByteArray> &field : header) {
            if (field.first.compare("upgrade", Qt::CaseInsensitive) == 0 &&
                    field.second.compare("h2c", Qt::CaseInsensitive) == 0)
                return true;
        }
    }

    return false;
}

} // namespace Http2

QT_END_NAMESPACE
