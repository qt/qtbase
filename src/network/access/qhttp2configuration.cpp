// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhttp2configuration.h"

#include "private/http2protocol_p.h"
#include "private/hpack_p.h"

#include "qdebug.h"

QT_BEGIN_NAMESPACE

/*!
    \class QHttp2Configuration
    \brief The QHttp2Configuration class controls HTTP/2 parameters and settings.
    \since 5.14

    \reentrant
    \inmodule QtNetwork
    \ingroup network
    \ingroup shared

    QHttp2Configuration controls HTTP/2 parameters and settings that
    QNetworkAccessManager will use to send requests and process responses
    when the HTTP/2 protocol is enabled.

    The HTTP/2 parameters that QHttp2Configuration currently supports include:

    \list
      \li The session window size for connection-level flow control.
         Will be sent to a remote peer when needed as 'WINDOW_UPDATE'
         frames on the stream with an identifier 0.
      \li The stream receiving window size for stream-level flow control.
         Sent as 'SETTINGS_INITIAL_WINDOW_SIZE' parameter in the initial
         SETTINGS frame and, when needed, 'WINDOW_UPDATE' frames will be
         sent on streams that QNetworkAccessManager opens.
      \li The maximum frame size. This parameter limits the maximum payload
         a frame coming from the remote peer can have. Sent by QNetworkAccessManager
         as 'SETTINGS_MAX_FRAME_SIZE' parameter in the initial 'SETTINGS'
         frame.
      \li The server push. Allows to enable or disable server push. Sent
         as 'SETTINGS_ENABLE_PUSH' parameter in the initial 'SETTINGS'
         frame.
    \endlist

    The QHttp2Configuration class also controls if the header compression
    algorithm (HPACK) is additionally using Huffman coding for string
    compression.

    \note The configuration must be set before the first request
    was sent to a given host (and thus an HTTP/2 session established).

    \note Details about flow control, server push and 'SETTINGS'
    can be found in \l {https://httpwg.org/specs/rfc7540.html}{RFC 7540}.
    Different modes and parameters of the HPACK compression algorithm
    are described in \l {https://httpwg.org/specs/rfc7541.html}{RFC 7541}.

    \sa QNetworkRequest::setHttp2Configuration(), QNetworkRequest::http2Configuration(), QNetworkAccessManager
*/

class QHttp2ConfigurationPrivate : public QSharedData
{
public:
    unsigned sessionWindowSize = Http2::defaultSessionWindowSize;
    unsigned streamWindowSize = Http2::defaultSessionWindowSize;

    unsigned maxFrameSize = Http2::minPayloadLimit; // Initial (default) value of 16Kb.

    bool pushEnabled = false;
    // TODO: for now those two below are noop.
    bool huffmanCompressionEnabled = true;
};

/*!
    Default constructs a QHttp2Configuration object.

    Such a configuration has the following values:
    \list
        \li Server push is disabled
        \li Huffman string compression is enabled
        \li Window size for connection-level flow control is 65535 octets
        \li Window size for stream-level flow control is 65535 octets
        \li Frame size is 16384 octets
    \endlist
*/
QHttp2Configuration::QHttp2Configuration()
    : d(new QHttp2ConfigurationPrivate)
{
}

/*!
    Copy-constructs this QHttp2Configuration.
*/
QHttp2Configuration::QHttp2Configuration(const QHttp2Configuration &) = default;

/*!
    Move-constructs this QHttp2Configuration from \a other
*/
QHttp2Configuration::QHttp2Configuration(QHttp2Configuration &&other) noexcept
{
    swap(other);
}

/*!
    Copy-assigns \a other to this QHttp2Configuration.
*/
QHttp2Configuration &QHttp2Configuration::operator=(const QHttp2Configuration &) = default;

/*!
    Move-assigns \a other to this QHttp2Configuration.
*/
QHttp2Configuration &QHttp2Configuration::operator=(QHttp2Configuration &&) noexcept = default;

/*!
    Destructor.
*/
QHttp2Configuration::~QHttp2Configuration()
{
}

/*!
    If \a enable is \c true, a remote server can potentially
    use server push to send responses in advance.

    \sa serverPushEnabled
*/
void QHttp2Configuration::setServerPushEnabled(bool enable)
{
    d->pushEnabled = enable;
}

/*!
    Returns true if server push was enabled.

    \note By default, QNetworkAccessManager disables server
    push via the 'SETTINGS' frame.

    \sa setServerPushEnabled
*/
bool QHttp2Configuration::serverPushEnabled() const
{
    return d->pushEnabled;
}

/*!
    If \a enable is \c true, HPACK compression will additionally
    compress string using the Huffman coding. Enabled by default.

    \note This parameter only affects 'HEADERS' frames that
    QNetworkAccessManager is sending.

    \sa huffmanCompressionEnabled
*/
void QHttp2Configuration::setHuffmanCompressionEnabled(bool enable)
{
    d->huffmanCompressionEnabled = enable;
}

/*!
    Returns \c true if the Huffman coding in HPACK is enabled.

    \sa setHuffmanCompressionEnabled
*/
bool QHttp2Configuration::huffmanCompressionEnabled() const
{
    return d->huffmanCompressionEnabled;
}

/*!
    Sets the window size for connection-level flow control.
    \a size cannot be 0 and must not exceed 2147483647 octets.

    Returns \c true on success, \c false otherwise.

    \sa sessionReceiveWindowSize
*/
bool QHttp2Configuration::setSessionReceiveWindowSize(unsigned size)
{
    if (!size || size > Http2::maxSessionReceiveWindowSize) { // RFC-7540, 6.9
        qCWarning(QT_HTTP2) << "Invalid session window size";
        return false;
    }

    d->sessionWindowSize = size;
    return true;
}

/*!
    Returns the window size for connection-level flow control.
    The default value QNetworkAccessManager will be using is
    2147483647 octets.
*/
unsigned QHttp2Configuration::sessionReceiveWindowSize() const
{
    return d->sessionWindowSize;
}

/*!
    Sets the window size for stream-level flow control.
    \a size cannot be 0 and must not exceed 2147483647 octets.

    Returns \c true on success, \c false otherwise.

    \sa streamReceiveWindowSize
 */
bool QHttp2Configuration::setStreamReceiveWindowSize(unsigned size)
{
    if (!size || size > Http2::maxSessionReceiveWindowSize) { // RFC-7540, 6.9
        qCWarning(QT_HTTP2) << "Invalid stream window size";
        return false;
    }

    d->streamWindowSize = size;
    return true;
}

/*!
    Returns the window size for stream-level flow control.
    The default value QNetworkAccessManager will be using is
    214748364 octets (see \l {https://httpwg.org/specs/rfc7540.html#SettingValues}{RFC 7540}).
*/
unsigned QHttp2Configuration::streamReceiveWindowSize() const
{
    return d->streamWindowSize;
}

/*!
    Sets the maximum frame size that QNetworkAccessManager
    will advertise to the server when sending its initial SETTINGS frame.
    \note While this \a size is required to be within a range between
    16384 and 16777215 inclusive, the actual payload size in frames
    that carry payload maybe be less than 16384.

    Returns \c true on success, \c false otherwise.
*/
bool QHttp2Configuration::setMaxFrameSize(unsigned size)
{
    if (size < Http2::minPayloadLimit || size > Http2::maxPayloadSize) {
        qCWarning(QT_HTTP2) << "Maximum frame size to advertise is invalid";
        return false;
    }

    d->maxFrameSize = size;
    return true;
}

/*!
    Returns the maximum payload size that HTTP/2 frames can
    have. The default (initial) value is 16384 octets.
*/
unsigned QHttp2Configuration::maxFrameSize() const
{
    return d->maxFrameSize;
}

/*!
    Swaps this configuration with the \a other configuration.
*/
void QHttp2Configuration::swap(QHttp2Configuration &other) noexcept
{
    d.swap(other.d);
}

/*!
    \fn bool QHttp2Configuration::operator==(const QHttp2Configuration &lhs, const QHttp2Configuration &rhs) noexcept
    Returns \c true if \a lhs and \a rhs have the same set of HTTP/2
    parameters.
*/

/*!
    \fn bool QHttp2Configuration::operator!=(const QHttp2Configuration &lhs, const QHttp2Configuration &rhs) noexcept
    Returns \c true if \a lhs and \a rhs do not have the same set of HTTP/2
    parameters.
*/

/*!
    \internal
*/
bool QHttp2Configuration::isEqual(const QHttp2Configuration &other) const noexcept
{
    if (d == other.d)
        return true;

    return d->pushEnabled == other.d->pushEnabled
           && d->huffmanCompressionEnabled == other.d->huffmanCompressionEnabled
           && d->sessionWindowSize == other.d->sessionWindowSize
           && d->streamWindowSize == other.d->streamWindowSize;
}

QT_END_NAMESPACE
