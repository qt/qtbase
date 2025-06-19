// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qnetworkrequest.h"
#include "qnetworkrequest_p.h"
#include "qplatformdefs.h"
#include "qnetworkcookie.h"
#include "qsslconfiguration.h"
#include "qhttpheadershelper_p.h"
#if QT_CONFIG(http)
#include "qhttp1configuration.h"
#include "qhttp2configuration.h"
#include "private/http2protocol_p.h"
#endif

#include "QtCore/qdatetime.h"
#include "QtCore/qlocale.h"
#include "QtCore/qshareddata.h"
#include "QtCore/qtimezone.h"
#include "QtCore/private/qduplicatetracker_p.h"
#include "QtCore/private/qtools_p.h"

#include <ctype.h>
#if QT_CONFIG(datestring)
# include <stdio.h>
#endif

#include <algorithm>
#include <q20algorithm.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

constexpr std::chrono::milliseconds QNetworkRequest::DefaultTransferTimeout;

QT_IMPL_METATYPE_EXTERN(QNetworkRequest)
QT_IMPL_METATYPE_EXTERN_TAGGED(QNetworkRequest::RedirectPolicy, QNetworkRequest__RedirectPolicy)

/*!
    \class QNetworkRequest
    \since 4.4
    \ingroup network
    \ingroup shared
    \inmodule QtNetwork

    \brief The QNetworkRequest class holds a request to be sent with QNetworkAccessManager.

    QNetworkRequest is part of the Network Access API and is the class
    holding the information necessary to send a request over the
    network. It contains a URL and some ancillary information that can
    be used to modify the request.

    \sa QNetworkReply, QNetworkAccessManager
*/

/*!
    \enum QNetworkRequest::KnownHeaders

    List of known header types that QNetworkRequest parses. Each known
    header is also represented in raw form with its full HTTP name.

    \value ContentDispositionHeader  Corresponds to the HTTP
    Content-Disposition header and contains a string containing the
    disposition type (for instance, attachment) and a parameter (for
    instance, filename).

    \value ContentTypeHeader    Corresponds to the HTTP Content-Type
    header and contains a string containing the media (MIME) type and
    any auxiliary data (for instance, charset).

    \value ContentLengthHeader  Corresponds to the HTTP Content-Length
    header and contains the length in bytes of the data transmitted.

    \value LocationHeader       Corresponds to the HTTP Location
    header and contains a URL representing the actual location of the
    data, including the destination URL in case of redirections.

    \value LastModifiedHeader   Corresponds to the HTTP Last-Modified
    header and contains a QDateTime representing the last modification
    date of the contents.

    \value IfModifiedSinceHeader   Corresponds to the HTTP If-Modified-Since
    header and contains a QDateTime. It is usually added to a
    QNetworkRequest. The server shall send a 304 (Not Modified) response
    if the resource has not changed since this time.

    \value ETagHeader              Corresponds to the HTTP ETag
    header and contains a QString representing the last modification
    state of the contents.

    \value IfMatchHeader           Corresponds to the HTTP If-Match
    header and contains a QStringList. It is usually added to a
    QNetworkRequest. The server shall send a 412 (Precondition Failed)
    response if the resource does not match.

    \value IfNoneMatchHeader       Corresponds to the HTTP If-None-Match
    header and contains a QStringList. It is usually added to a
    QNetworkRequest. The server shall send a 304 (Not Modified) response
    if the resource does match.

    \value CookieHeader         Corresponds to the HTTP Cookie header
    and contains a QList<QNetworkCookie> representing the cookies to
    be sent back to the server.

    \value SetCookieHeader      Corresponds to the HTTP Set-Cookie
    header and contains a QList<QNetworkCookie> representing the
    cookies sent by the server to be stored locally.

    \value UserAgentHeader      The User-Agent header sent by HTTP clients.

    \value ServerHeader         The Server header received by HTTP clients.

    \omitvalue NumKnownHeaders

    \sa header(), setHeader(), rawHeader(), setRawHeader()
*/

/*!
    \enum QNetworkRequest::Attribute
    \since 4.7

    Attribute codes for the QNetworkRequest and QNetworkReply.

    Attributes are extra meta-data that are used to control the
    behavior of the request and to pass further information from the
    reply back to the application. Attributes are also extensible,
    allowing custom implementations to pass custom values.

    The following table explains what the default attribute codes are,
    the QVariant types associated, the default value if said attribute
    is missing and whether it's used in requests or replies.

    \value HttpStatusCodeAttribute
        Replies only, type: QMetaType::Int (no default)
        Indicates the HTTP status code received from the HTTP server
        (like 200, 304, 404, 401, etc.). If the connection was not
        HTTP-based, this attribute will not be present.

    \value HttpReasonPhraseAttribute
        Replies only, type: QMetaType::QByteArray (no default)
        Indicates the HTTP reason phrase as received from the HTTP
        server (like "Ok", "Found", "Not Found", "Access Denied",
        etc.) This is the human-readable representation of the status
        code (see above). If the connection was not HTTP-based, this
        attribute will not be present. \e{Note:} The reason phrase is
        not used when using HTTP/2.

    \value RedirectionTargetAttribute
        Replies only, type: QMetaType::QUrl (no default)
        If present, it indicates that the server is redirecting the
        request to a different URL. The Network Access API does follow
        redirections by default, unless
        QNetworkRequest::ManualRedirectPolicy is used. Additionally, if
        QNetworkRequest::UserVerifiedRedirectPolicy is used, then this
        attribute will be set if the redirect was not followed.
        The returned URL might be relative. Use QUrl::resolved()
        to create an absolute URL out of it.

    \value ConnectionEncryptedAttribute
        Replies only, type: QMetaType::Bool (default: false)
        Indicates whether the data was obtained through an encrypted
        (secure) connection.

    \value CacheLoadControlAttribute
        Requests only, type: QMetaType::Int (default: QNetworkRequest::PreferNetwork)
        Controls how the cache should be accessed. The possible values
        are those of QNetworkRequest::CacheLoadControl. Note that the
        default QNetworkAccessManager implementation does not support
        caching. However, this attribute may be used by certain
        backends to modify their requests (for example, for caching proxies).

    \value CacheSaveControlAttribute
        Requests only, type: QMetaType::Bool (default: true)
        Controls if the data obtained should be saved to cache for
        future uses. If the value is false, the data obtained will not
        be automatically cached. If true, data may be cached, provided
        it is cacheable (what is cacheable depends on the protocol
        being used).

    \value SourceIsFromCacheAttribute
        Replies only, type: QMetaType::Bool (default: false)
        Indicates whether the data was obtained from cache
        or not.

    \value DoNotBufferUploadDataAttribute
        Requests only, type: QMetaType::Bool (default: false)
        Indicates whether the QNetworkAccessManager code is
        allowed to buffer the upload data, e.g. when doing a HTTP POST.
        When using this flag with sequential upload data, the ContentLengthHeader
        header must be set.

    \value HttpPipeliningAllowedAttribute
        Requests only, type: QMetaType::Bool (default: false)
        Indicates whether the QNetworkAccessManager code is
        allowed to use HTTP pipelining with this request.

    \value HttpPipeliningWasUsedAttribute
        Replies only, type: QMetaType::Bool
        Indicates whether the HTTP pipelining was used for receiving
        this reply.

    \value CustomVerbAttribute
       Requests only, type: QMetaType::QByteArray
       Holds the value for the custom HTTP verb to send (destined for usage
       of other verbs than GET, POST, PUT and DELETE). This verb is set
       when calling QNetworkAccessManager::sendCustomRequest().

    \value CookieLoadControlAttribute
        Requests only, type: QMetaType::Int (default: QNetworkRequest::Automatic)
        Indicates whether to send 'Cookie' headers in the request.
        This attribute is set to false by Qt WebKit when creating a cross-origin
        XMLHttpRequest where withCredentials has not been set explicitly to true by the
        Javascript that created the request.
        See \l{http://www.w3.org/TR/XMLHttpRequest2/#credentials-flag}{here} for more information.
        (This value was introduced in 4.7.)

    \value CookieSaveControlAttribute
        Requests only, type: QMetaType::Int (default: QNetworkRequest::Automatic)
        Indicates whether to save 'Cookie' headers received from the server in reply
        to the request.
        This attribute is set to false by Qt WebKit when creating a cross-origin
        XMLHttpRequest where withCredentials has not been set explicitly to true by the
        Javascript that created the request.
        See \l{http://www.w3.org/TR/XMLHttpRequest2/#credentials-flag} {here} for more information.
        (This value was introduced in 4.7.)

    \value AuthenticationReuseAttribute
        Requests only, type: QMetaType::Int (default: QNetworkRequest::Automatic)
        Indicates whether to use cached authorization credentials in the request,
        if available. If this is set to QNetworkRequest::Manual and the authentication
        mechanism is 'Basic' or 'Digest', Qt will not send an 'Authorization' HTTP
        header with any cached credentials it may have for the request's URL.
        This attribute is set to QNetworkRequest::Manual by Qt WebKit when creating a cross-origin
        XMLHttpRequest where withCredentials has not been set explicitly to true by the
        Javascript that created the request.
        See \l{http://www.w3.org/TR/XMLHttpRequest2/#credentials-flag} {here} for more information.
        (This value was introduced in 4.7.)

    \omitvalue MaximumDownloadBufferSizeAttribute

    \omitvalue DownloadBufferAttribute

    \omitvalue SynchronousRequestAttribute

    \value BackgroundRequestAttribute
        Type: QMetaType::Bool (default: false)
        Indicates that this is a background transfer, rather than a user initiated
        transfer. Depending on the platform, background transfers may be subject
        to different policies.

    \value Http2AllowedAttribute
        Requests only, type: QMetaType::Bool (default: true)
        Indicates whether the QNetworkAccessManager code is
        allowed to use HTTP/2 with this request. This applies
        to SSL requests or 'cleartext' HTTP/2 if Http2CleartextAllowedAttribute
        is set.

    \value Http2WasUsedAttribute
        Replies only, type: QMetaType::Bool (default: false)
        Indicates whether HTTP/2 was used for receiving this reply.
        (This value was introduced in 5.9.)

    \value EmitAllUploadProgressSignalsAttribute
        Requests only, type: QMetaType::Bool (default: false)
        Indicates whether all upload signals should be emitted.
        By default, the uploadProgress signal is emitted only
        in 100 millisecond intervals.
        (This value was introduced in 5.5.)

    \value OriginalContentLengthAttribute
        Replies only, type QMetaType::Int
        Holds the original content-length attribute before being invalidated and
        removed from the header when the data is compressed and the request was
        marked to be decompressed automatically.
        (This value was introduced in 5.9.)

    \value RedirectPolicyAttribute
        Requests only, type: QMetaType::Int, should be one of the
        QNetworkRequest::RedirectPolicy values
        (default: NoLessSafeRedirectPolicy).
        (This value was introduced in 5.9.)

    \value Http2DirectAttribute
        Requests only, type: QMetaType::Bool (default: false)
        If set, this attribute will force QNetworkAccessManager to use
        HTTP/2 protocol without initial HTTP/2 protocol negotiation.
        Use of this attribute implies prior knowledge that a particular
        server supports HTTP/2. The attribute works with SSL or with 'cleartext'
        HTTP/2 if Http2CleartextAllowedAttribute is set.
        If a server turns out to not support HTTP/2, when HTTP/2 direct
        was specified, QNetworkAccessManager gives up, without attempting to
        fall back to HTTP/1.1. If both Http2AllowedAttribute and
        Http2DirectAttribute are set, Http2DirectAttribute takes priority.
        (This value was introduced in 5.11.)

    \omitvalue ResourceTypeAttribute

    \value AutoDeleteReplyOnFinishAttribute
        Requests only, type: QMetaType::Bool (default: false)
        If set, this attribute will make QNetworkAccessManager delete
        the QNetworkReply after having emitted "finished".
        (This value was introduced in 5.14.)

    \value ConnectionCacheExpiryTimeoutSecondsAttribute
        Requests only, type: QMetaType::Int
        To set when the TCP connections to a server (HTTP1 and HTTP2) should
        be closed after the last pending request had been processed.
        (This value was introduced in 6.3.)

    \value Http2CleartextAllowedAttribute
        Requests only, type: QMetaType::Bool (default: false)
        If set, this attribute will tell QNetworkAccessManager to attempt
        an upgrade to HTTP/2 over cleartext (also known as h2c).
        Until Qt 7 the default value for this attribute can be overridden
        to true by setting the QT_NETWORK_H2C_ALLOWED environment variable.
        This attribute is ignored if the Http2AllowedAttribute is not set.
        (This value was introduced in 6.3.)

    \value UseCredentialsAttribute
        Requests only, type: QMetaType::Bool (default: false)
        Indicates if the underlying XMLHttpRequest cross-site Access-Control
        requests should be made using credentials. Has no effect on
        same-origin requests. This only affects the WebAssembly platform.
        (This value was introduced in 6.5.)

    \value FullLocalServerNameAttribute
        Requests only, type: QMetaType::String
        Holds the full local server name to be used for the underlying
        QLocalSocket. This attribute is used by the QNetworkAccessManager
        to connect to a specific local server, when QLocalSocket's behavior for
        a simple name isn't enough. The URL in the QNetworkRequest must still
        use unix+http: or local+http: scheme. And the hostname in the URL will
        be used for the Host header in the HTTP request.
        (This value was introduced in 6.8.)

    \value User
        Special type. Additional information can be passed in
        QVariants with types ranging from User to UserMax. The default
        implementation of Network Access will ignore any request
        attributes in this range and it will not produce any
        attributes in this range in replies. The range is reserved for
        extensions of QNetworkAccessManager.

    \value UserMax
        Special type. See User.
*/

/*!
    \enum QNetworkRequest::CacheLoadControl

    Controls the caching mechanism of QNetworkAccessManager.

    \value AlwaysNetwork        always load from network and do not
    check if the cache has a valid entry (similar to the
    "Reload" feature in browsers); in addition, force intermediate
    caches to re-validate.

    \value PreferNetwork        default value; load from the network
    if the cached entry is older than the network entry. This will never
    return stale data from the cache, but revalidate resources that
    have become stale.

    \value PreferCache          load from cache if available,
    otherwise load from network. Note that this can return possibly
    stale (but not expired) items from cache.

    \value AlwaysCache          only load from cache, indicating error
    if the item was not cached (i.e., off-line mode)
*/

/*!
    \enum QNetworkRequest::LoadControl
    \since 4.7

    Indicates if an aspect of the request's loading mechanism has been
    manually overridden, e.g. by Qt WebKit.

    \value Automatic            default value: indicates default behaviour.

    \value Manual               indicates behaviour has been manually overridden.
*/

/*!
    \enum QNetworkRequest::RedirectPolicy
    \since 5.9

    Indicates whether the Network Access API should automatically follow a
    HTTP redirect response or not.

    \value ManualRedirectPolicy        Not following any redirects.

    \value NoLessSafeRedirectPolicy    Default value: Only "http"->"http",
                                       "http" -> "https" or "https" -> "https" redirects
                                       are allowed.

    \value SameOriginRedirectPolicy    Require the same protocol, host and port.
                                       Note, http://example.com and http://example.com:80
                                       will fail with this policy (implicit/explicit ports
                                       are considered to be a mismatch).

    \value UserVerifiedRedirectPolicy  Client decides whether to follow each
                                       redirect by handling the redirected()
                                       signal, emitting redirectAllowed() on
                                       the QNetworkReply object to allow
                                       the redirect or aborting/finishing it to
                                       reject the redirect.  This can be used,
                                       for example, to ask the user whether to
                                       accept the redirect, or to decide
                                       based on some app-specific configuration.

    \note When Qt handles redirects it will, for legacy and compatibility
    reasons, issue the redirected request using GET when the server returns
    a 301 or 302 response, regardless of the original method used, unless it was
    HEAD.
*/

/*!
    \enum QNetworkRequest::TransferTimeoutConstant
    \since 5.15

    A constant that can be used for enabling transfer
    timeouts with a preset value.

    \value DefaultTransferTimeoutConstant     The transfer timeout in milliseconds.
                                              Used if setTransferTimeout() is called
                                              without an argument.

    \sa QNetworkRequest::DefaultTransferTimeout
 */

/*!
    \variable QNetworkRequest::DefaultTransferTimeout

    The transfer timeout with \l {QNetworkRequest::TransferTimeoutConstant}
    milliseconds. Used if setTransferTimeout() is called without an
    argument.
 */

class QNetworkRequestPrivate: public QSharedData, public QNetworkHeadersPrivate
{
public:
    static const int maxRedirectCount = 50;
    inline QNetworkRequestPrivate()
        : priority(QNetworkRequest::NormalPriority)
#ifndef QT_NO_SSL
        , sslConfiguration(nullptr)
#endif
        , maxRedirectsAllowed(maxRedirectCount)
    { qRegisterMetaType<QNetworkRequest>(); }
    ~QNetworkRequestPrivate()
    {
#ifndef QT_NO_SSL
        delete sslConfiguration;
#endif
    }


    QNetworkRequestPrivate(const QNetworkRequestPrivate &other)
        : QSharedData(other), QNetworkHeadersPrivate(other)
    {
        url = other.url;
        priority = other.priority;
        maxRedirectsAllowed = other.maxRedirectsAllowed;
#ifndef QT_NO_SSL
        sslConfiguration = nullptr;
        if (other.sslConfiguration)
            sslConfiguration = new QSslConfiguration(*other.sslConfiguration);
#endif
        peerVerifyName = other.peerVerifyName;
#if QT_CONFIG(http)
        h1Configuration = other.h1Configuration;
        h2Configuration = other.h2Configuration;
        decompressedSafetyCheckThreshold = other.decompressedSafetyCheckThreshold;
#endif
        transferTimeout = other.transferTimeout;
    }

    inline bool operator==(const QNetworkRequestPrivate &other) const
    {
        return url == other.url &&
            priority == other.priority &&
            attributes == other.attributes &&
            maxRedirectsAllowed == other.maxRedirectsAllowed &&
            peerVerifyName == other.peerVerifyName
#if QT_CONFIG(http)
            && h1Configuration == other.h1Configuration
            && h2Configuration == other.h2Configuration
            && decompressedSafetyCheckThreshold == other.decompressedSafetyCheckThreshold
#endif
            && transferTimeout == other.transferTimeout
            && QHttpHeadersHelper::compareStrict(httpHeaders, other.httpHeaders)
            ;
        // don't compare cookedHeaders
    }

    QUrl url;
    QNetworkRequest::Priority priority;
#ifndef QT_NO_SSL
    mutable QSslConfiguration *sslConfiguration;
#endif
    int maxRedirectsAllowed;
    QString peerVerifyName;
#if QT_CONFIG(http)
    QHttp1Configuration h1Configuration;
    QHttp2Configuration h2Configuration;
    qint64 decompressedSafetyCheckThreshold = 10ll * 1024ll * 1024ll;
#endif
    std::chrono::milliseconds transferTimeout = 0ms;
};

/*!
    Constructs a QNetworkRequest object with no URL to be requested.
    Use setUrl() to set one.

    \sa url(), setUrl()
*/
QNetworkRequest::QNetworkRequest()
    : d(new QNetworkRequestPrivate)
{
#if QT_CONFIG(http)
    // Initial values proposed by RFC 7540 are quite draconian, but we
    // know about servers configured with this value as maximum possible,
    // rejecting our SETTINGS frame and sending us a GOAWAY frame with the
    // flow control error set. If this causes a problem - the app should
    // set a proper configuration. We'll use our defaults, as documented.
    d->h2Configuration.setStreamReceiveWindowSize(Http2::qtDefaultStreamReceiveWindowSize);
    d->h2Configuration.setSessionReceiveWindowSize(Http2::maxSessionReceiveWindowSize);
    d->h2Configuration.setServerPushEnabled(false);
#endif // QT_CONFIG(http)
}

/*!
    Constructs a QNetworkRequest object with \a url as the URL to be
    requested.

    \sa url(), setUrl()
*/
QNetworkRequest::QNetworkRequest(const QUrl &url)
    : QNetworkRequest()
{
    d->url = url;
}

/*!
    Creates a copy of \a other.
*/
QNetworkRequest::QNetworkRequest(const QNetworkRequest &other)
    : d(other.d)
{
}

/*!
    Disposes of the QNetworkRequest object.
*/
QNetworkRequest::~QNetworkRequest()
{
    // QSharedDataPointer auto deletes
    d = nullptr;
}

/*!
    Returns \c true if this object is the same as \a other (i.e., if they
    have the same URL, same headers and same meta-data settings).

    \sa operator!=()
*/
bool QNetworkRequest::operator==(const QNetworkRequest &other) const
{
    return d == other.d || *d == *other.d;
}

/*!
    \fn bool QNetworkRequest::operator!=(const QNetworkRequest &other) const

    Returns \c false if this object is not the same as \a other.

    \sa operator==()
*/

/*!
    Creates a copy of \a other
*/
QNetworkRequest &QNetworkRequest::operator=(const QNetworkRequest &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QNetworkRequest::swap(QNetworkRequest &other)
    \since 5.0
    \memberswap{network request}
*/

/*!
    Returns the URL this network request is referring to.

    \sa setUrl()
*/
QUrl QNetworkRequest::url() const
{
    return d->url;
}

/*!
    Sets the URL this network request is referring to be \a url.

    \sa url()
*/
void QNetworkRequest::setUrl(const QUrl &url)
{
    d->url = url;
}

/*!
    \since 6.8

    Returns headers that are set in this network request.

    \sa setHeaders()
*/
QHttpHeaders QNetworkRequest::headers() const
{
    return d->headers();
}

/*!
    \since 6.8

    Sets \a newHeaders as headers in this network request, overriding
    any previously set headers.

    If some headers correspond to the known headers, the values will
    be parsed and the corresponding parsed form will also be set.

    \sa headers(), KnownHeaders
*/
void QNetworkRequest::setHeaders(QHttpHeaders &&newHeaders)
{
    d->setHeaders(std::move(newHeaders));
}

/*!
    \overload
    \since 6.8
*/
void QNetworkRequest::setHeaders(const QHttpHeaders &newHeaders)
{
    d->setHeaders(newHeaders);
}

/*!
    Returns the value of the known network header \a header if it is
    present in this request. If it is not present, returns QVariant()
    (i.e., an invalid variant).

    \sa KnownHeaders, rawHeader(), setHeader()
*/
QVariant QNetworkRequest::header(KnownHeaders header) const
{
    return d->cookedHeaders.value(header);
}

/*!
    Sets the value of the known header \a header to be \a value,
    overriding any previously set headers. This operation also sets
    the equivalent raw HTTP header.

    \sa KnownHeaders, setRawHeader(), header()
*/
void QNetworkRequest::setHeader(KnownHeaders header, const QVariant &value)
{
    d->setCookedHeader(header, value);
}

/*!
    Returns \c true if the raw header \a headerName is present in this
    network request.

    \sa rawHeader(), setRawHeader()
    \note In Qt versions prior to 6.7, this function took QByteArray only.
*/
bool QNetworkRequest::hasRawHeader(QAnyStringView headerName) const
{
    return d->headers().contains(headerName);
}

/*!
    Returns the raw form of header \a headerName. If no such header is
    present, an empty QByteArray is returned, which may be
    indistinguishable from a header that is present but has no content
    (use hasRawHeader() to find out if the header exists or not).

    Raw headers can be set with setRawHeader() or with setHeader().

    \sa header(), setRawHeader()
    \note In Qt versions prior to 6.7, this function took QByteArray only.
*/
QByteArray QNetworkRequest::rawHeader(QAnyStringView headerName) const
{
    return d->rawHeader(headerName);
}

/*!
    Returns a list of all raw headers that are set in this network
    request. The list is in the order that the headers were set.

    \sa hasRawHeader(), rawHeader()
*/
QList<QByteArray> QNetworkRequest::rawHeaderList() const
{
    return d->rawHeadersKeys();
}

/*!
    Sets the header \a headerName to be of value \a headerValue. If \a
    headerName corresponds to a known header (see
    QNetworkRequest::KnownHeaders), the raw format will be parsed and
    the corresponding "cooked" header will be set as well.

    For example:
    \snippet code/src_network_access_qnetworkrequest.cpp 0

    will also set the known header LastModifiedHeader to be the
    QDateTime object of the parsed date.

    \note Setting the same header twice overrides the previous
    setting. To accomplish the behaviour of multiple HTTP headers of
    the same name, you should concatenate the two values, separating
    them with a comma (",") and set one single raw header.

    \note Since Qt 6.8, the header field names are normalized
    by converting them to lowercase.

    \sa KnownHeaders, setHeader(), hasRawHeader(), rawHeader()
*/
void QNetworkRequest::setRawHeader(const QByteArray &headerName, const QByteArray &headerValue)
{
    d->setRawHeader(headerName, headerValue);
}

/*!
    Returns the attribute associated with the code \a code. If the
    attribute has not been set, it returns \a defaultValue.

    \note This function does not apply the defaults listed in
    QNetworkRequest::Attribute.

    \sa setAttribute(), QNetworkRequest::Attribute
*/
QVariant QNetworkRequest::attribute(Attribute code, const QVariant &defaultValue) const
{
    return d->attributes.value(code, defaultValue);
}

/*!
    Sets the attribute associated with code \a code to be value \a
    value. If the attribute is already set, the previous value is
    discarded. In special, if \a value is an invalid QVariant, the
    attribute is unset.

    \sa attribute(), QNetworkRequest::Attribute
*/
void QNetworkRequest::setAttribute(Attribute code, const QVariant &value)
{
    if (value.isValid())
        d->attributes.insert(code, value);
    else
        d->attributes.remove(code);
}

#ifndef QT_NO_SSL
/*!
    Returns this network request's SSL configuration. By default this is the same
    as QSslConfiguration::defaultConfiguration().

    \sa setSslConfiguration(), QSslConfiguration::defaultConfiguration()
*/
QSslConfiguration QNetworkRequest::sslConfiguration() const
{
    if (!d->sslConfiguration)
        d->sslConfiguration = new QSslConfiguration(QSslConfiguration::defaultConfiguration());
    return *d->sslConfiguration;
}

/*!
    Sets this network request's SSL configuration to be \a config. The
    settings that apply are the private key, the local certificate,
    the TLS protocol (e.g. TLS 1.3), the CA certificates and the ciphers that
    the SSL backend is allowed to use.

    \sa sslConfiguration(), QSslConfiguration::defaultConfiguration()
*/
void QNetworkRequest::setSslConfiguration(const QSslConfiguration &config)
{
    if (!d->sslConfiguration)
        d->sslConfiguration = new QSslConfiguration(config);
    else
        *d->sslConfiguration = config;
}
#endif

/*!
    \since 4.6

    Allows setting a reference to the \a object initiating
    the request.

    For example Qt WebKit sets the originating object to the
    QWebFrame that initiated the request.

    \sa originatingObject()
*/
void QNetworkRequest::setOriginatingObject(QObject *object)
{
    d->originatingObject = object;
}

/*!
    \since 4.6

    Returns a reference to the object that initiated this
    network request; returns \nullptr if not set or the object has
    been destroyed.

    \sa setOriginatingObject()
*/
QObject *QNetworkRequest::originatingObject() const
{
    return d->originatingObject.data();
}

/*!
    \since 4.7

    Return the priority of this request.

    \sa setPriority()
*/
QNetworkRequest::Priority QNetworkRequest::priority() const
{
    return d->priority;
}

/*! \enum QNetworkRequest::Priority

  \since 4.7

  This enum lists the possible network request priorities.

  \value HighPriority   High priority
  \value NormalPriority Normal priority
  \value LowPriority    Low priority
 */

/*!
    \since 4.7

    Set the priority of this request to \a priority.

    \note The \a priority is only a hint to the network access
    manager.  It can use it or not. Currently it is used for HTTP to
    decide which request should be sent first to a server.

    \sa priority()
*/
void QNetworkRequest::setPriority(Priority priority)
{
    d->priority = priority;
}

/*!
    \since 5.6

    Returns the maximum number of redirects allowed to be followed for this
    request.

    \sa setMaximumRedirectsAllowed()
*/
int QNetworkRequest::maximumRedirectsAllowed() const
{
    return d->maxRedirectsAllowed;
}

/*!
    \since 5.6

    Sets the maximum number of redirects allowed to be followed for this
    request to \a maxRedirectsAllowed.

    \sa maximumRedirectsAllowed()
*/
void QNetworkRequest::setMaximumRedirectsAllowed(int maxRedirectsAllowed)
{
    d->maxRedirectsAllowed = maxRedirectsAllowed;
}

/*!
    \since 5.13

    Returns the host name set for the certificate validation, as set by
    setPeerVerifyName. By default this returns a null string.

    \sa setPeerVerifyName
*/
QString QNetworkRequest::peerVerifyName() const
{
    return d->peerVerifyName;
}

/*!
    \since 5.13

    Sets \a peerName as host name for the certificate validation, instead of the one used for the
    TCP connection.

    \sa peerVerifyName
*/
void QNetworkRequest::setPeerVerifyName(const QString &peerName)
{
    d->peerVerifyName = peerName;
}

#if QT_CONFIG(http)
/*!
    \since 6.5

    Returns the current parameters that QNetworkAccessManager is
    using for the underlying HTTP/1 connection of this request.

    \sa setHttp1Configuration
*/
QHttp1Configuration QNetworkRequest::http1Configuration() const
{
    return d->h1Configuration;
}
/*!
    \since 6.5

    Sets request's HTTP/1 parameters from \a configuration.

    \sa http1Configuration, QNetworkAccessManager, QHttp1Configuration
*/
void QNetworkRequest::setHttp1Configuration(const QHttp1Configuration &configuration)
{
    d->h1Configuration = configuration;
}

/*!
    \since 5.14

    Returns the current parameters that QNetworkAccessManager is
    using for this request and its underlying HTTP/2 connection.
    This is either a configuration previously set by an application
    or a default configuration.

    The default values that QNetworkAccessManager is using are:

    \list
      \li Window size for connection-level flowcontrol is 2147483647 octets
      \li Window size for stream-level flowcontrol is 214748364 octets
      \li Max frame size is 16384
    \endlist

    By default, server push is disabled, Huffman compression and
    string indexing are enabled.

    \sa setHttp2Configuration
*/
QHttp2Configuration QNetworkRequest::http2Configuration() const
{
    return d->h2Configuration;
}

/*!
    \since 5.14

    Sets request's HTTP/2 parameters from \a configuration.

    \note The configuration must be set prior to making a request.
    \note HTTP/2 multiplexes several streams in a single HTTP/2
    connection. This implies that QNetworkAccessManager will use
    the configuration found in the first request from  a series
    of requests sent to the same host.

    \sa http2Configuration, QNetworkAccessManager, QHttp2Configuration
*/
void QNetworkRequest::setHttp2Configuration(const QHttp2Configuration &configuration)
{
    d->h2Configuration = configuration;
}

/*!
    \since 6.2

    Returns the threshold for archive bomb checks.

    If the decompressed size of a reply is smaller than this, Qt will simply
    decompress it, without further checking.

    \sa setDecompressedSafetyCheckThreshold()
*/
qint64 QNetworkRequest::decompressedSafetyCheckThreshold() const
{
    return d->decompressedSafetyCheckThreshold;
}

/*!
    \since 6.2

    Sets the \a threshold for archive bomb checks.

    Some supported compression algorithms can, in a tiny compressed file, encode
    a spectacularly huge decompressed file. This is only possible if the
    decompressed content is extremely monotonous, which is seldom the case for
    real files being transmitted in good faith: files exercising such insanely
    high compression ratios are typically payloads of buffer-overrun attacks, or
    denial-of-service (by using up too much memory) attacks. Consequently, files
    that decompress to huge sizes, particularly from tiny compressed forms, are
    best rejected as suspected malware.

    If a reply's decompressed size is bigger than this threshold (by default,
    10 MiB, i.e. 10 * 1024 * 1024), Qt will check the compression ratio: if that
    is unreasonably large (40:1 for GZip and Deflate, or 100:1 for Brotli and
    ZStandard), the reply will be treated as an error. Setting the threshold
    to \c{-1} disables this check.

    \sa decompressedSafetyCheckThreshold()
*/
void QNetworkRequest::setDecompressedSafetyCheckThreshold(qint64 threshold)
{
    d->decompressedSafetyCheckThreshold = threshold;
}
#endif // QT_CONFIG(http)

#if QT_CONFIG(http) || defined (Q_OS_WASM)
/*!
    \fn int QNetworkRequest::transferTimeout() const
    \since 5.15

    Returns the timeout used for transfers, in milliseconds.

    If transferTimeoutAsDuration().count() cannot be represented in \c{int},
    this function returns \c{INT_MAX}/\c{INT_MIN} instead.

    \sa setTransferTimeout(), transferTimeoutAsDuration()
*/

/*!
    \fn void QNetworkRequest::setTransferTimeout(int timeout)
    \since 5.15

    Sets \a timeout as the transfer timeout in milliseconds.

    \sa setTransferTimeout(std::chrono::milliseconds),
        transferTimeout(), transferTimeoutAsDuration()
*/

/*!
    \since 6.7

    Returns the timeout duration after which the transfer is aborted if no
    data is exchanged.

    The default duration is zero, which means that the timeout is not used.

    \sa setTransferTimeout(std::chrono::milliseconds)
*/
std::chrono::milliseconds QNetworkRequest::transferTimeoutAsDuration() const
{
    return d->transferTimeout;
}

/*!
    \since 6.7

    Sets the timeout \a duration to abort the transfer if no data is exchanged.

    Transfers are aborted if no bytes are transferred before
    the timeout expires. Zero means no timer is set. If no
    argument is provided, the timeout is
    QNetworkRequest::DefaultTransferTimeout. If this function
    is not called, the timeout is disabled and has the
    value zero.

    \sa transferTimeoutAsDuration()
*/
void QNetworkRequest::setTransferTimeout(std::chrono::milliseconds duration)
{
    d->transferTimeout = duration;
}
#endif // QT_CONFIG(http) || defined (Q_OS_WASM)

namespace  {

struct HeaderPair {
    QHttpHeaders::WellKnownHeader wellKnownHeader;
    QNetworkRequest::KnownHeaders knownHeader;
};

constexpr bool operator<(const HeaderPair &lhs, const HeaderPair &rhs)
{
    return lhs.wellKnownHeader < rhs.wellKnownHeader;
}

constexpr bool operator<(const HeaderPair &lhs, QHttpHeaders::WellKnownHeader rhs)
{
    return lhs.wellKnownHeader < rhs;
}

constexpr bool operator<(QHttpHeaders::WellKnownHeader lhs, const HeaderPair &rhs)
{
    return lhs < rhs.wellKnownHeader;
}

} // anonymous namespace

static constexpr HeaderPair knownHeadersArr[] = {
    { QHttpHeaders::WellKnownHeader::ContentDisposition, QNetworkRequest::KnownHeaders::ContentDispositionHeader },
    { QHttpHeaders::WellKnownHeader::ContentLength,      QNetworkRequest::KnownHeaders::ContentLengthHeader },
    { QHttpHeaders::WellKnownHeader::ContentType,        QNetworkRequest::KnownHeaders::ContentTypeHeader },
    { QHttpHeaders::WellKnownHeader::Cookie,             QNetworkRequest::KnownHeaders::CookieHeader },
    { QHttpHeaders::WellKnownHeader::ETag,               QNetworkRequest::KnownHeaders::ETagHeader },
    { QHttpHeaders::WellKnownHeader::IfMatch ,           QNetworkRequest::KnownHeaders::IfMatchHeader },
    { QHttpHeaders::WellKnownHeader::IfModifiedSince,    QNetworkRequest::KnownHeaders::IfModifiedSinceHeader },
    { QHttpHeaders::WellKnownHeader::IfNoneMatch,        QNetworkRequest::KnownHeaders::IfNoneMatchHeader },
    { QHttpHeaders::WellKnownHeader::LastModified,       QNetworkRequest::KnownHeaders::LastModifiedHeader},
    { QHttpHeaders::WellKnownHeader::Location,           QNetworkRequest::KnownHeaders::LocationHeader},
    { QHttpHeaders::WellKnownHeader::Server,             QNetworkRequest::KnownHeaders::ServerHeader },
    { QHttpHeaders::WellKnownHeader::SetCookie,          QNetworkRequest::KnownHeaders::SetCookieHeader },
    { QHttpHeaders::WellKnownHeader::UserAgent,          QNetworkRequest::KnownHeaders::UserAgentHeader }
};

static_assert(std::size(knownHeadersArr) == size_t(QNetworkRequest::KnownHeaders::NumKnownHeaders));
static_assert(q20::is_sorted(std::begin(knownHeadersArr), std::end(knownHeadersArr)));

static std::optional<QNetworkRequest::KnownHeaders> toKnownHeader(QHttpHeaders::WellKnownHeader key)
{
    const auto it = std::lower_bound(std::begin(knownHeadersArr), std::end(knownHeadersArr), key);
    if (it == std::end(knownHeadersArr) || key < *it)
        return std::nullopt;
    return it->knownHeader;
}

static std::optional<QHttpHeaders::WellKnownHeader> toWellKnownHeader(QNetworkRequest::KnownHeaders key)
{
    auto pred = [key](const HeaderPair &pair) { return pair.knownHeader == key; };
    const auto it = std::find_if(std::begin(knownHeadersArr), std::end(knownHeadersArr), pred);
    if (it == std::end(knownHeadersArr))
        return std::nullopt;
    return it->wellKnownHeader;
}

static QByteArray makeCookieHeader(const QList<QNetworkCookie> &cookies,
                                   QNetworkCookie::RawForm type,
                                   QByteArrayView separator)
{
    QByteArray result;
    for (const QNetworkCookie &cookie : cookies) {
        result += cookie.toRawForm(type);
        result += separator;
    }
    if (!result.isEmpty())
        result.chop(separator.size());
    return result;
}

static QByteArray makeCookieHeader(const QVariant &value, QNetworkCookie::RawForm type,
                                   QByteArrayView separator)
{
    const QList<QNetworkCookie> *cookies = get_if<QList<QNetworkCookie>>(&value);
    if (!cookies)
        return {};
    return makeCookieHeader(*cookies, type, separator);
}

static QByteArray headerValue(QNetworkRequest::KnownHeaders header, const QVariant &value)
{
    switch (header) {
    case QNetworkRequest::ContentTypeHeader:
    case QNetworkRequest::ContentLengthHeader:
    case QNetworkRequest::ContentDispositionHeader:
    case QNetworkRequest::UserAgentHeader:
    case QNetworkRequest::ServerHeader:
    case QNetworkRequest::ETagHeader:
    case QNetworkRequest::IfMatchHeader:
    case QNetworkRequest::IfNoneMatchHeader:
        return value.toByteArray();

    case QNetworkRequest::LocationHeader:
        switch (value.userType()) {
        case QMetaType::QUrl:
            return value.toUrl().toEncoded();

        default:
            return value.toByteArray();
        }

    case QNetworkRequest::LastModifiedHeader:
    case QNetworkRequest::IfModifiedSinceHeader:
        switch (value.userType()) {
            // Generate RFC 1123/822 dates:
        case QMetaType::QDate:
            return QNetworkHeadersPrivate::toHttpDate(value.toDate().startOfDay(QTimeZone::UTC));
        case QMetaType::QDateTime:
            return QNetworkHeadersPrivate::toHttpDate(value.toDateTime());

        default:
            return value.toByteArray();
        }

    case QNetworkRequest::CookieHeader:
        return makeCookieHeader(value, QNetworkCookie::NameAndValueOnly, "; ");

    case QNetworkRequest::SetCookieHeader:
        return makeCookieHeader(value, QNetworkCookie::Full, ", ");

    default:
        Q_UNREACHABLE_RETURN({});
    }
}

static int parseHeaderName(QByteArrayView headerName)
{
    if (headerName.isEmpty())
        return -1;

    auto is = [headerName](QByteArrayView what) {
        return headerName.compare(what, Qt::CaseInsensitive) == 0;
    };

    switch (QtMiscUtils::toAsciiLower(headerName.front())) {
    case 'c':
        if (is("content-type"))
            return QNetworkRequest::ContentTypeHeader;
        else if (is("content-length"))
            return QNetworkRequest::ContentLengthHeader;
        else if (is("cookie"))
            return QNetworkRequest::CookieHeader;
        else if (is("content-disposition"))
            return QNetworkRequest::ContentDispositionHeader;
        break;

    case 'e':
        if (is("etag"))
            return QNetworkRequest::ETagHeader;
        break;

    case 'i':
        if (is("if-modified-since"))
            return QNetworkRequest::IfModifiedSinceHeader;
        if (is("if-match"))
            return QNetworkRequest::IfMatchHeader;
        if (is("if-none-match"))
            return QNetworkRequest::IfNoneMatchHeader;
        break;

    case 'l':
        if (is("location"))
            return QNetworkRequest::LocationHeader;
        else if (is("last-modified"))
            return QNetworkRequest::LastModifiedHeader;
        break;

    case 's':
        if (is("set-cookie"))
            return QNetworkRequest::SetCookieHeader;
        else if (is("server"))
            return QNetworkRequest::ServerHeader;
        break;

    case 'u':
        if (is("user-agent"))
            return QNetworkRequest::UserAgentHeader;
        break;
    }

    return -1; // nothing found
}

static QVariant parseHttpDate(QByteArrayView raw)
{
    QDateTime dt = QNetworkHeadersPrivate::fromHttpDate(raw);
    if (dt.isValid())
        return dt;
    return QVariant();          // transform an invalid QDateTime into a null QVariant
}

static QList<QNetworkCookie> parseCookieHeader(QByteArrayView raw)
{
    QList<QNetworkCookie> result;
    for (auto cookie : QLatin1StringView(raw).tokenize(';'_L1)) {
        QList<QNetworkCookie> parsed = QNetworkCookie::parseCookies(cookie.trimmed());
        if (parsed.size() != 1)
            return {};  // invalid Cookie: header

        result += parsed;
    }

    return result;
}

static QVariant parseETag(QByteArrayView raw)
{
    const QByteArrayView trimmed = raw.trimmed();
    if (!trimmed.startsWith('"') && !trimmed.startsWith(R"(W/")"))
        return QVariant();

    if (!trimmed.endsWith('"'))
        return QVariant();

    return QString::fromLatin1(trimmed);
}

template<typename T>
static QStringList parseMatchImpl(QByteArrayView raw, T op)
{
    const QByteArrayView trimmedRaw = raw.trimmed();
    if (trimmedRaw == "*")
        return QStringList(QStringLiteral("*"));

    QStringList tags;
    for (auto &element : QLatin1StringView(trimmedRaw).tokenize(','_L1)) {
        if (const auto trimmed = element.trimmed(); op(trimmed))
            tags += QString::fromLatin1(trimmed);
    }
    return tags;
}


static QStringList parseIfMatch(QByteArrayView raw)
{
    return parseMatchImpl(raw, [](QByteArrayView element) {
        return element.startsWith('"') && element.endsWith('"');
    });
}

static QStringList parseIfNoneMatch(QByteArrayView raw)
{
    return parseMatchImpl(raw, [](QByteArrayView element) {
        return (element.startsWith('"') || element.startsWith(R"(W/")")) && element.endsWith('"');
    });
}


static QVariant parseHeaderValue(QNetworkRequest::KnownHeaders header, QByteArrayView value)
{
    // header is always a valid value
    switch (header) {
    case QNetworkRequest::UserAgentHeader:
    case QNetworkRequest::ServerHeader:
    case QNetworkRequest::ContentTypeHeader:
    case QNetworkRequest::ContentDispositionHeader:
        // copy exactly, convert to QString
        return QString::fromLatin1(value);

    case QNetworkRequest::ContentLengthHeader: {
        bool ok;
        qint64 result = QByteArrayView(value).trimmed().toLongLong(&ok);
        if (ok)
            return result;
        return QVariant();
    }

    case QNetworkRequest::LocationHeader: {
        QUrl result = QUrl::fromEncoded(value, QUrl::StrictMode);
        if (result.isValid() && !result.scheme().isEmpty())
            return result;
        return QVariant();
    }

    case QNetworkRequest::LastModifiedHeader:
    case QNetworkRequest::IfModifiedSinceHeader:
        return parseHttpDate(value);

    case QNetworkRequest::ETagHeader:
        return parseETag(value);

    case QNetworkRequest::IfMatchHeader:
        return parseIfMatch(value);

    case QNetworkRequest::IfNoneMatchHeader:
        return parseIfNoneMatch(value);

    case QNetworkRequest::CookieHeader:
        return QVariant::fromValue(parseCookieHeader(value));

    case QNetworkRequest::SetCookieHeader:
        return QVariant::fromValue(QNetworkCookie::parseCookies(value));

    default:
        Q_UNREACHABLE_RETURN({});
    }
}

static QVariant parseHeaderValue(QNetworkRequest::KnownHeaders header, QList<QByteArray> values)
{
    if (values.empty())
        return QVariant();

    // header is always a valid value
    switch (header) {
    case QNetworkRequest::IfMatchHeader: {
        QStringList res;
        for (const auto &val : values)
            res << parseIfMatch(val);
        return res;
    }
    case QNetworkRequest::IfNoneMatchHeader: {
        QStringList res;
        for (const auto &val : values)
            res << parseIfNoneMatch(val);
        return res;
    }
    case QNetworkRequest::CookieHeader: {
        auto listOpt = QNetworkHeadersPrivate::toCookieList(values);
        return listOpt.has_value() ? QVariant::fromValue(listOpt.value()) : QVariant();
    }
    case QNetworkRequest::SetCookieHeader: {
        QList<QNetworkCookie> res;
        for (const auto &val : values)
            res << QNetworkCookie::parseCookies(val);
        return QVariant::fromValue(res);
    }
    default:
        return parseHeaderValue(header, values.first());
    }
    return QVariant();
}

static bool isSetCookie(QByteArrayView name)
{
    return name.compare(QHttpHeaders::wellKnownHeaderName(QHttpHeaders::WellKnownHeader::SetCookie),
                        Qt::CaseInsensitive) == 0;
}

static bool isSetCookie(QHttpHeaders::WellKnownHeader name)
{
    return name == QHttpHeaders::WellKnownHeader::SetCookie;
}

template<class HeaderName>
static void setFromRawHeader(QHttpHeaders &headers, HeaderName header,
                             QByteArrayView value)
{
    headers.removeAll(header);

    if (value.isNull())
        // only wanted to erase key
        return;

    if (isSetCookie(header)) {
        for (auto cookie : QLatin1StringView(value).tokenize('\n'_L1))
            headers.append(QHttpHeaders::WellKnownHeader::SetCookie, cookie);
    } else {
        headers.append(header, value);
    }
}

const QNetworkHeadersPrivate::RawHeadersList &QNetworkHeadersPrivate::allRawHeaders() const
{
    if (rawHeaderCache.isCached)
        return rawHeaderCache.headersList;

    rawHeaderCache.headersList = fromHttpToRaw(httpHeaders);
    rawHeaderCache.isCached = true;
    return rawHeaderCache.headersList;
}

QList<QByteArray> QNetworkHeadersPrivate::rawHeadersKeys() const
{
    if (httpHeaders.isEmpty())
        return {};

    QList<QByteArray> result;
    result.reserve(httpHeaders.size());
    QDuplicateTracker<QByteArray> seen(httpHeaders.size());

    for (qsizetype i = 0; i < httpHeaders.size(); i++) {
        const auto nameL1 = httpHeaders.nameAt(i);
        const auto name = QByteArray(nameL1.data(), nameL1.size());
        if (seen.hasSeen(name))
            continue;

        result << name;
    }

    return result;
}

QByteArray QNetworkHeadersPrivate::rawHeader(QAnyStringView headerName) const
{
    QByteArrayView setCookieStr = QHttpHeaders::wellKnownHeaderName(
            QHttpHeaders::WellKnownHeader::SetCookie);
    if (QAnyStringView::compare(headerName, setCookieStr, Qt::CaseInsensitive) != 0)
        return httpHeaders.combinedValue(headerName);

    QByteArray result;
    const char* separator = "";
    for (qsizetype i = 0; i < httpHeaders.size(); ++i) {
        if (QAnyStringView::compare(httpHeaders.nameAt(i), headerName, Qt::CaseInsensitive) == 0) {
            result.append(separator);
            result.append(httpHeaders.valueAt(i));
            separator = "\n";
        }
    }
    return result;
}

void QNetworkHeadersPrivate::setRawHeader(const QByteArray &key, const QByteArray &value)
{
    if (key.isEmpty())
        // refuse to accept an empty raw header
        return;

    setFromRawHeader(httpHeaders, key, value);
    parseAndSetHeader(key, value);

    invalidateHeaderCache();
}

void QNetworkHeadersPrivate::setCookedHeader(QNetworkRequest::KnownHeaders header,
                                             const QVariant &value)
{
    const auto wellKnownOpt = toWellKnownHeader(header);
    if (!wellKnownOpt) {
        // verifies that \a header is a known value
        qWarning("QNetworkRequest::setHeader: invalid header value KnownHeader(%d) received", header);
        return;
    }

    if (value.isNull()) {
        httpHeaders.removeAll(wellKnownOpt.value());
        cookedHeaders.remove(header);
    } else {
        QByteArray rawValue = headerValue(header, value);
        if (rawValue.isEmpty()) {
            qWarning("QNetworkRequest::setHeader: QVariant of type %s cannot be used with header %s",
                     value.typeName(),
                     QHttpHeaders::wellKnownHeaderName(wellKnownOpt.value()).constData());
            return;
        }

        setFromRawHeader(httpHeaders, wellKnownOpt.value(), rawValue);
        cookedHeaders.insert(header, value);
    }

    invalidateHeaderCache();
}

QHttpHeaders QNetworkHeadersPrivate::headers() const
{
    return httpHeaders;
}

void QNetworkHeadersPrivate::setHeaders(const QHttpHeaders &newHeaders)
{
    httpHeaders = newHeaders;
    setCookedFromHttp(httpHeaders);
    invalidateHeaderCache();
}

void QNetworkHeadersPrivate::setHeaders(QHttpHeaders &&newHeaders)
{
    httpHeaders = std::move(newHeaders);
    setCookedFromHttp(httpHeaders);
    invalidateHeaderCache();
}

void QNetworkHeadersPrivate::setHeader(QHttpHeaders::WellKnownHeader name, QByteArrayView value)
{
    httpHeaders.replaceOrAppend(name, value);

    // set cooked header
    const auto knownHeaderOpt = toKnownHeader(name);
    if (knownHeaderOpt)
        parseAndSetHeader(knownHeaderOpt.value(), value);

    invalidateHeaderCache();
}

void QNetworkHeadersPrivate::clearHeaders()
{
    httpHeaders.clear();
    cookedHeaders.clear();
    invalidateHeaderCache();
}

void QNetworkHeadersPrivate::parseAndSetHeader(QByteArrayView key, QByteArrayView value)
{
    // is it a known header?
    const int parsedKeyAsInt = parseHeaderName(key);
    if (parsedKeyAsInt != -1) {
        const QNetworkRequest::KnownHeaders parsedKey
                = static_cast<QNetworkRequest::KnownHeaders>(parsedKeyAsInt);
        parseAndSetHeader(parsedKey, value);
    }
}

void QNetworkHeadersPrivate::parseAndSetHeader(QNetworkRequest::KnownHeaders key,
                                               QByteArrayView value)
{
    if (value.isNull()) {
        cookedHeaders.remove(key);
    } else if (key == QNetworkRequest::ContentLengthHeader
               && cookedHeaders.contains(QNetworkRequest::ContentLengthHeader)) {
        // Only set the cooked header "Content-Length" once.
        // See bug QTBUG-15311
    } else {
        cookedHeaders.insert(key, parseHeaderValue(key, value));
    }
}

// Fast month string to int conversion. This code
// assumes that the Month name is correct and that
// the string is at least three chars long.
static int name_to_month(const char* month_str)
{
    switch (month_str[0]) {
    case 'J':
        switch (month_str[1]) {
        case 'a':
            return 1;
        case 'u':
            switch (month_str[2] ) {
            case 'n':
                return 6;
            case 'l':
                return 7;
            }
        }
        break;
    case 'F':
        return 2;
    case 'M':
        switch (month_str[2] ) {
        case 'r':
            return 3;
        case 'y':
            return 5;
        }
        break;
    case 'A':
        switch (month_str[1]) {
        case 'p':
            return 4;
        case 'u':
            return 8;
        }
        break;
    case 'O':
        return 10;
    case 'S':
        return 9;
    case 'N':
        return 11;
    case 'D':
        return 12;
    }

    return 0;
}

QDateTime QNetworkHeadersPrivate::fromHttpDate(QByteArrayView value)
{
    // HTTP dates have three possible formats:
    //  RFC 1123/822      -   ddd, dd MMM yyyy hh:mm:ss "GMT"
    //  RFC 850           -   dddd, dd-MMM-yy hh:mm:ss "GMT"
    //  ANSI C's asctime  -   ddd MMM d hh:mm:ss yyyy
    // We only handle them exactly. If they deviate, we bail out.

    int pos = value.indexOf(',');
    QDateTime dt;
#if QT_CONFIG(datestring)
    if (pos == -1) {
        // no comma -> asctime(3) format
        dt = QDateTime::fromString(QString::fromLatin1(value), Qt::TextDate);
    } else {
        // Use sscanf over QLocal/QDateTimeParser for speed reasons. See the
        // Qt WebKit performance benchmarks to get an idea.
        if (pos == 3) {
            char month_name[4];
            int day, year, hour, minute, second;
#ifdef Q_CC_MSVC
            // Use secure version to avoid compiler warning
            if (sscanf_s(value.constData(), "%*3s, %d %3s %d %d:%d:%d 'GMT'", &day, month_name, 4, &year, &hour, &minute, &second) == 6)
#else
            // The POSIX secure mode is %ms (which allocates memory), too bleeding edge for now
            // In any case this is already safe as field width is specified.
            if (sscanf(value.constData(), "%*3s, %d %3s %d %d:%d:%d 'GMT'", &day, month_name, &year, &hour, &minute, &second) == 6)
#endif
                dt = QDateTime(QDate(year, name_to_month(month_name), day), QTime(hour, minute, second));
        } else {
            QLocale c = QLocale::c();
            // eat the weekday, the comma and the space following it
            QString sansWeekday = QString::fromLatin1(value.constData() + pos + 2);
            // must be RFC 850 date
            dt = c.toDateTime(sansWeekday, "dd-MMM-yy hh:mm:ss 'GMT'"_L1);
        }
    }
#endif // datestring

    if (dt.isValid())
        dt.setTimeZone(QTimeZone::UTC);
    return dt;
}

QByteArray QNetworkHeadersPrivate::toHttpDate(const QDateTime &dt)
{
    return QLocale::c().toString(dt.toUTC(), u"ddd, dd MMM yyyy hh:mm:ss 'GMT'").toLatin1();
}

QNetworkHeadersPrivate::RawHeadersList QNetworkHeadersPrivate::fromHttpToRaw(
        const QHttpHeaders &headers)
{
    if (headers.isEmpty())
        return {};

    QNetworkHeadersPrivate::RawHeadersList list;
    QHash<QByteArray, qsizetype> nameToIndex;
    list.reserve(headers.size());
    nameToIndex.reserve(headers.size());

    for (qsizetype i = 0; i < headers.size(); ++i) {
        const auto nameL1 = headers.nameAt(i);
        const auto value = headers.valueAt(i);

        const bool isSetCookie = nameL1 == QHttpHeaders::wellKnownHeaderName(
                                         QHttpHeaders::WellKnownHeader::SetCookie);

        const auto name = QByteArray(nameL1.data(), nameL1.size());
        if (auto it = nameToIndex.find(name); it != nameToIndex.end()) {
            list[it.value()].second += isSetCookie ? "\n" : ", ";
            list[it.value()].second += value;
        } else {
            nameToIndex[name] = list.size();
            list.emplaceBack(name, value.toByteArray());
        }
    }

    return list;
}

QHttpHeaders QNetworkHeadersPrivate::fromRawToHttp(const RawHeadersList &raw)
{
    if (raw.empty())
        return {};

    QHttpHeaders headers;
    headers.reserve(raw.size());

    for (const auto &[key, value] : raw) {
        const bool isSetCookie = key.compare(QHttpHeaders::wellKnownHeaderName(
                                             QHttpHeaders::WellKnownHeader::SetCookie),
                                             Qt::CaseInsensitive) == 0;
        if (isSetCookie) {
            for (auto header : QLatin1StringView(value).tokenize('\n'_L1))
                headers.append(key, header);
        } else {
            headers.append(key, value);
        }
    }

    return headers;
}

std::optional<qint64> QNetworkHeadersPrivate::toInt(QByteArrayView value)
{
    if (value.empty())
        return std::nullopt;

    bool ok;
    qint64 res = value.toLongLong(&ok);
    if (ok)
        return res;
    return std::nullopt;
}

std::optional<QNetworkHeadersPrivate::NetworkCookieList> QNetworkHeadersPrivate::toSetCookieList(
        const QList<QByteArray> &values)
{
    if (values.empty())
        return std::nullopt;

    QList<QNetworkCookie> cookies;
    for (const auto &s : values)
        cookies += QNetworkCookie::parseCookies(s);

    if (cookies.empty())
        return std::nullopt;
    return cookies;
}

QByteArray QNetworkHeadersPrivate::fromCookieList(const QList<QNetworkCookie> &cookies)
{
    return makeCookieHeader(cookies, QNetworkCookie::NameAndValueOnly, "; ");
}

std::optional<QNetworkHeadersPrivate::NetworkCookieList> QNetworkHeadersPrivate::toCookieList(
        const QList<QByteArray> &values)
{
    if (values.empty())
        return std::nullopt;

    QList<QNetworkCookie> cookies;
    for (const auto &s : values)
        cookies += parseCookieHeader(s);

    if (cookies.empty())
        return std::nullopt;
    return cookies;
}

void QNetworkHeadersPrivate::invalidateHeaderCache()
{
    rawHeaderCache.headersList.clear();
    rawHeaderCache.isCached = false;
}

void QNetworkHeadersPrivate::setCookedFromHttp(const QHttpHeaders &newHeaders)
{
    cookedHeaders.clear();

    QMap<QNetworkRequest::KnownHeaders, QList<QByteArray>> multipleHeadersMap;
    for (int i = 0; i < newHeaders.size(); ++i) {
        const auto name = newHeaders.nameAt(i);
        const auto value = newHeaders.valueAt(i);

        const int parsedKeyAsInt = parseHeaderName(name);
        if (parsedKeyAsInt == -1)
            continue;

        const QNetworkRequest::KnownHeaders parsedKey
                = static_cast<QNetworkRequest::KnownHeaders>(parsedKeyAsInt);

        auto &list = multipleHeadersMap[parsedKey];
        list.append(value.toByteArray());
    }

    for (auto i = multipleHeadersMap.cbegin(), end = multipleHeadersMap.cend(); i != end; ++i)
        cookedHeaders.insert(i.key(), parseHeaderValue(i.key(), i.value()));
}

QT_END_NAMESPACE

#include "moc_qnetworkrequest.cpp"
