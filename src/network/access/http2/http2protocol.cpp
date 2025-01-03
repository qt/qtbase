// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "http2protocol_p.h"
#include "http2frames_p.h"

#include "private/qhttpnetworkrequest_p.h"
#include "private/qhttpnetworkreply_p.h"

#include <access/qhttp2configuration.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QT_IMPL_METATYPE_EXTERN_TAGGED(Http2::Settings, Http2__Settings)

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

    // Stream receive window size (if it's a default value, don't include):
    if (config.streamReceiveWindowSize() != defaultSessionWindowSize) {
        builder.append(Settings::INITIAL_WINDOW_SIZE_ID);
        builder.append(config.streamReceiveWindowSize());
    }

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
        errorMessage = "RST_STREAM with unknown error code (%1)"_L1;
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
        errorMessage = "HTTP/2 protocol error"_L1;
        break;
    case INTERNAL_ERROR:
        error = QNetworkReply::InternalServerError;
        errorMessage = "Internal server error"_L1;
        break;
    case FLOW_CONTROL_ERROR:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = "Flow control error"_L1;
        break;
    case SETTINGS_TIMEOUT:
        error = QNetworkReply::TimeoutError;
        errorMessage = "SETTINGS ACK timeout error"_L1;
        break;
    case STREAM_CLOSED:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = "Server received frame(s) on a half-closed stream"_L1;
        break;
    case FRAME_SIZE_ERROR:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = "Server received a frame with an invalid size"_L1;
        break;
    case REFUSE_STREAM:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = "Server refused a stream"_L1;
        break;
    case CANCEL:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = "Stream is no longer needed"_L1;
        break;
    case COMPRESSION_ERROR:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = "Server is unable to maintain the "
                       "header compression context for the connection"_L1;
        break;
    case CONNECT_ERROR:
        // TODO: in Qt6 we'll have to add more error codes in QNetworkReply.
        error = QNetworkReply::UnknownNetworkError;
        errorMessage = "The connection established in response "
                       "to a CONNECT request was reset or abnormally closed"_L1;
        break;
    case ENHANCE_YOUR_CALM:
        error = QNetworkReply::UnknownServerError;
        errorMessage = "Server dislikes our behavior, excessive load detected."_L1;
        break;
    case INADEQUATE_SECURITY:
        error = QNetworkReply::ContentAccessDenied;
        errorMessage = "The underlying transport has properties "
                       "that do not meet minimum security "
                       "requirements"_L1;
        break;
    case HTTP_1_1_REQUIRED:
        error = QNetworkReply::ProtocolFailure;
        errorMessage = "Server requires that HTTP/1.1 "
                       "be used instead of HTTP/2."_L1;
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
