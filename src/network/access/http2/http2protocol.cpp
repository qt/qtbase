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

// TODO: (in 5.11) - remove it!
const char *http2ParametersPropertyName = "QT_HTTP2_PARAMETERS_PROPERTY";

ProtocolParameters::ProtocolParameters()
{
    settingsFrameData[Settings::INITIAL_WINDOW_SIZE_ID] = qtDefaultStreamReceiveWindowSize;
    settingsFrameData[Settings::ENABLE_PUSH_ID] = 0;
}

bool ProtocolParameters::validate() const
{
    // 0. Huffman/indexing: any values are valid and allowed.

    // 1. Session receive window size (client side): HTTP/2 starts from the
    // default value of 64Kb, if a client code tries to set lesser value,
    // the delta would become negative, but this is not allowed.
    if (maxSessionReceiveWindowSize < qint32(defaultSessionWindowSize)) {
        qCWarning(QT_HTTP2, "Session receive window must be at least 65535 bytes");
        return false;
    }

    // 2. HEADER_TABLE_SIZE: we do not validate HEADER_TABLE_SIZE, considering
    // all values as valid. RFC 7540 and 7541 do not provide any lower/upper
    // limits. If it's 0 - we do not index anything, if it's too huge - a user
    // who provided such a value can potentially have a huge memory footprint,
    // up to them to decide.

    // 3. SETTINGS_ENABLE_PUSH: RFC 7540, 6.5.2, a value other than 0 or 1 will
    // be treated by our peer as a PROTOCOL_ERROR.
    if (settingsFrameData.contains(Settings::ENABLE_PUSH_ID)
        && settingsFrameData[Settings::ENABLE_PUSH_ID] > 1) {
        qCWarning(QT_HTTP2, "SETTINGS_ENABLE_PUSH can be only 0 or 1");
        return false;
    }

    // 4. SETTINGS_MAX_CONCURRENT_STREAMS : RFC 7540 recommends 100 as the lower
    // limit, says nothing about the upper limit. The RFC allows 0, but this makes
    // no sense to us at all: there is no way a user can change this later and
    // we'll not be able to get any responses on such a connection.
    if (settingsFrameData.contains(Settings::MAX_CONCURRENT_STREAMS_ID)
        && !settingsFrameData[Settings::MAX_CONCURRENT_STREAMS_ID]) {
        qCWarning(QT_HTTP2, "MAX_CONCURRENT_STREAMS must be a positive number");
        return false;
    }

    // 5. SETTINGS_INITIAL_WINDOW_SIZE.
    if (settingsFrameData.contains(Settings::INITIAL_WINDOW_SIZE_ID)) {
        const quint32 value = settingsFrameData[Settings::INITIAL_WINDOW_SIZE_ID];
        // RFC 7540, 6.5.2 (the upper limit). The lower limit is our own - we send
        // SETTINGS frame only once and will not be able to change this 0, thus
        // we'll suspend all streams.
        if (!value || value > quint32(maxSessionReceiveWindowSize)) {
            qCWarning(QT_HTTP2, "INITIAL_WINDOW_SIZE must be in the range "
                                "(0, 2^31-1]");
            return false;
        }
    }

    // 6. SETTINGS_MAX_FRAME_SIZE: RFC 7540, 6.5.2, a value outside of the range
    // [2^14-1, 2^24-1] will be treated by our peer as a PROTOCOL_ERROR.
    if (settingsFrameData.contains(Settings::MAX_FRAME_SIZE_ID)) {
        const quint32 value = settingsFrameData[Settings::INITIAL_WINDOW_SIZE_ID];
        if (value < maxFrameSize || value > maxPayloadSize) {
            qCWarning(QT_HTTP2, "MAX_FRAME_SIZE must be in the range [2^14, 2^24-1]");
            return false;
        }
    }

    // For SETTINGS_MAX_HEADER_LIST_SIZE RFC 7540 does not provide any specific
    // numbers. It's clear, if a value is too small, no header can ever be sent
    // by our peer at all. The default value is unlimited and we normally do not
    // change this.
    //
    // Note: the size is calculated as the length of uncompressed (no HPACK)
    // name + value + 32 bytes.

    return true;
}

QByteArray ProtocolParameters::settingsFrameToBase64() const
{
    Frame frame(settingsFrame());
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

Frame ProtocolParameters::settingsFrame() const
{
    // 6.5 SETTINGS
    FrameWriter builder(FrameType::SETTINGS, FrameFlag::EMPTY, connectionStreamID);
    for (auto it = settingsFrameData.cbegin(), end = settingsFrameData.cend();
         it != end; ++it) {
        builder.append(it.key());
        builder.append(it.value());
    }

    return builder.outboundFrame();
}

void ProtocolParameters::addProtocolUpgradeHeaders(QHttpNetworkRequest *request) const
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
    // This we just (re)write.
    request->setHeaderField("HTTP2-Settings", settingsFrameToBase64());
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
            if (field.first.toLower() == "upgrade" && field.second.toLower() == "h2c")
                return true;
        }
    }

    return false;
}

} // namespace Http2

QT_END_NAMESPACE
