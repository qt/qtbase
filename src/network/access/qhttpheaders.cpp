// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qhttpheaders.h"

#include <private/qoffsetstringarray_p.h>

#include <QtCore/qhash.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <QtCore/qttypetraits.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQHttpHeaders, "qt.network.http.headers");

/*!
    \class QHttpHeaders
    \since 6.7
    \ingroup
    \inmodule QtNetwork

    \brief QHttpHeaders is a class for holding HTTP headers.

    The class is an interface type for Qt networking APIs that
    use or consume such headers.

    \section1 Allowed field name and value characters

    An HTTP header consists of \e name and \e value.
    When setting these, QHttpHeaders validates \e name and \e value
    to only contain characters allowed by the HTTP RFCs. For detailed
    information see
    \l {https://datatracker.ietf.org/doc/html/rfc9110#name-field-values}
    {RFC 9110 Chapters 5.1 and 5.5}.

    Broadly speaking, this means:
    \list
        \li \c name must consist of visible ASCII characters, and must not be
            empty
        \li \c value may consist of arbitrary bytes, as long as header
            and use case specific encoding rules are adhered to. \c value
            may be empty
    \endlist

    Furthermore, \e value may have historically contained leading or
    trailing whitespace, which has to be ignored while processing such
    values. The setters of this class automatically remove any such
    whitespace.

    \section1 Combining values

    Most HTTP header values can be combined with a single comma \c {','},
    and the semantic meaning is preserved. As an example, these two should be
    semantically similar:
    \badcode
        // Values as separate header entries
        myheadername: myheadervalue1
        myheadername: myheadervalue2
        // Combined value
        myheadername: myheadervalue1,myheadervalue2
    \endcode

    However there is a notable exception to this rule:
    \l {https://datatracker.ietf.org/doc/html/rfc9110#name-field-order}
    {Set-Cookie}. Due to this, as well as due to the possibility
    of custom use cases, QHttpHeaders does not automatically combine
    the values.
*/

// A clarification on case-sensitivity:
// - Header *names*  are case-insensitive; Content-Type and content-type are considered equal
// - Header *values* are case-sensitive
// (In addition, the HTTP/2 and HTTP/3 standards mandate that all headers must be lower-cased when
// encoded into transmission)
struct Header {
    QByteArray name;
    QByteArray value;

private:
    friend bool operator==(const Header &lhs, const Header &rhs) noexcept
    {
        return lhs.value == rhs.value && lhs.name == rhs.name;
    }
};

class QHttpHeadersPrivate : public QSharedData
{
public:
    QHttpHeadersPrivate() = default;

    bool equals(const QHttpHeadersPrivate &other,
                QHttpHeaders::CompareOptions options) const noexcept;

    QList<Header> headers;

    Q_ALWAYS_INLINE void verify([[maybe_unused]] qsizetype pos = 0,
                                [[maybe_unused]] qsizetype n = 1) const
    {
        Q_ASSERT(pos >= 0);
        Q_ASSERT(pos <= headers.size());
        Q_ASSERT(n >= 0);
        Q_ASSERT(n <= headers.size() - pos);
    }
};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QHttpHeadersPrivate)

bool QHttpHeadersPrivate::equals(const QHttpHeadersPrivate &other,
                                 QHttpHeaders::CompareOptions options) const noexcept
{
    if (headers.size() != other.headers.size())
        return false;

    if (options & QHttpHeaders::CompareOption::OrderSensitive)
        return headers == other.headers;
    else
        return std::is_permutation(headers.begin(), headers.end(), other.headers.begin());
}

// This list is from IANA HTTP Field Name Registry
// https://www.iana.org/assignments/http-fields
// It contains entries that are either "permanent"
// or "deprecated" as of October 2023.
// Usage relies on enum values keeping in same order.
// ### Qt7 check if some of these headers have been obsoleted,
// and also check if the enums benefit from reordering
static constexpr auto headerNames = qOffsetStringArray(
    // IANA Permanent status:
    "a-im",
    "accept",
    "accept-additions",
    "accept-ch",
    "accept-datetime",
    "accept-encoding",
    "accept-features",
    "accept-language",
    "accept-patch",
    "accept-post",
    "accept-ranges",
    "accept-signature",
    "access-control-allow-credentials",
    "access-control-allow-headers",
    "access-control-allow-methods",
    "access-control-allow-origin",
    "access-control-expose-headers",
    "access-control-max-age",
    "access-control-request-headers",
    "access-control-request-method",
    "age",
    "allow",
    "alpn",
    "alt-svc",
    "alt-used",
    "alternates",
    "apply-to-redirect-ref",
    "authentication-control",
    "authentication-info",
    "authorization",
    "cache-control",
    "cache-status",
    "cal-managed-id",
    "caldav-timezones",
    "capsule-protocol",
    "cdn-cache-control",
    "cdn-loop",
    "cert-not-after",
    "cert-not-before",
    "clear-site-data",
    "client-cert",
    "client-cert-chain",
    "close",
    "connection",
    "content-digest",
    "content-disposition",
    "content-encoding",
    "content-id",
    "content-language",
    "content-length",
    "content-location",
    "content-range",
    "content-security-policy",
    "content-security-policy-report-only",
    "content-type",
    "cookie",
    "cross-origin-embedder-policy",
    "cross-origin-embedder-policy-report-only",
    "cross-origin-opener-policy",
    "cross-origin-opener-policy-report-only",
    "cross-origin-resource-policy",
    "dasl",
    "date",
    "dav",
    "delta-base",
    "depth",
    "destination",
    "differential-id",
    "dpop",
    "dpop-nonce",
    "early-data",
    "etag",
    "expect",
    "expect-ct",
    "expires",
    "forwarded",
    "from",
    "hobareg",
    "host",
    "if",
    "if-match",
    "if-modified-since",
    "if-none-match",
    "if-range",
    "if-schedule-tag-match",
    "if-unmodified-since",
    "im",
    "include-referred-token-binding-id",
    "keep-alive",
    "label",
    "last-event-id",
    "last-modified",
    "link",
    "location",
    "lock-token",
    "max-forwards",
    "memento-datetime",
    "meter",
    "mime-version",
    "negotiate",
    "nel",
    "odata-entityid",
    "odata-isolation",
    "odata-maxversion",
    "odata-version",
    "optional-www-authenticate",
    "ordering-type",
    "origin",
    "origin-agent-cluster",
    "oscore",
    "oslc-core-version",
    "overwrite",
    "ping-from",
    "ping-to",
    "position",
    "prefer",
    "preference-applied",
    "priority",
    "proxy-authenticate",
    "proxy-authentication-info",
    "proxy-authorization",
    "proxy-status",
    "public-key-pins",
    "public-key-pins-report-only",
    "range",
    "redirect-ref",
    "referer",
    "refresh",
    "replay-nonce",
    "repr-digest",
    "retry-after",
    "schedule-reply",
    "schedule-tag",
    "sec-purpose",
    "sec-token-binding",
    "sec-websocket-accept",
    "sec-websocket-extensions",
    "sec-websocket-key",
    "sec-websocket-protocol",
    "sec-websocket-version",
    "server",
    "server-timing",
    "set-cookie",
    "signature",
    "signature-input",
    "slug",
    "soapaction",
    "status-uri",
    "strict-transport-security",
    "sunset",
    "surrogate-capability",
    "surrogate-control",
    "tcn",
    "te",
    "timeout",
    "topic",
    "traceparent",
    "tracestate",
    "trailer",
    "transfer-encoding",
    "ttl",
    "upgrade",
    "urgency",
    "user-agent",
    "variant-vary",
    "vary",
    "via",
    "want-content-digest",
    "want-repr-digest",
    "www-authenticate",
    "x-content-type-options",
    "x-frame-options",
    // IANA Deprecated status:
    "accept-charset",
    "c-pep-info",
    "pragma",
    "protocol-info",
    "protocol-query"
);

/*!
    \enum QHttpHeaders::WellKnownHeader

    List of well known headers as per
    \l {https://www.iana.org/assignments/http-fields}{IANA registry}.

    \value AIM
    \value Accept
    \value AcceptAdditions
    \value AcceptCH
    \value AcceptDatetime
    \value AcceptEncoding
    \value AcceptFeatures
    \value AcceptLanguage
    \value AcceptPatch
    \value AcceptPost
    \value AcceptRanges
    \value AcceptSignature
    \value AccessControlAllowCredentials
    \value AccessControlAllowHeaders
    \value AccessControlAllowMethods
    \value AccessControlAllowOrigin
    \value AccessControlExposeHeaders
    \value AccessControlMaxAge
    \value AccessControlRequestHeaders
    \value AccessControlRequestMethod
    \value Age
    \value Allow
    \value ALPN
    \value AltSvc
    \value AltUsed
    \value Alternates
    \value ApplyToRedirectRef
    \value AuthenticationControl
    \value AuthenticationInfo
    \value Authorization
    \value CacheControl
    \value CacheStatus
    \value CalManagedID
    \value CalDAVTimezones
    \value CapsuleProtocol
    \value CDNCacheControl
    \value CDNLoop
    \value CertNotAfter
    \value CertNotBefore
    \value ClearSiteData
    \value ClientCert
    \value ClientCertChain
    \value Close
    \value Connection
    \value ContentDigest
    \value ContentDisposition
    \value ContentEncoding
    \value ContentID
    \value ContentLanguage
    \value ContentLength
    \value ContentLocation
    \value ContentRange
    \value ContentSecurityPolicy
    \value ContentSecurityPolicyReportOnly
    \value ContentType
    \value Cookie
    \value CrossOriginEmbedderPolicy
    \value CrossOriginEmbedderPolicyReportOnly
    \value CrossOriginOpenerPolicy
    \value CrossOriginOpenerPolicyReportOnly
    \value CrossOriginResourcePolicy
    \value DASL
    \value Date
    \value DAV
    \value DeltaBase
    \value Depth
    \value Destination
    \value DifferentialID
    \value DPoP
    \value DPoPNonce
    \value EarlyData
    \value ETag
    \value Expect
    \value ExpectCT
    \value Expires
    \value Forwarded
    \value From
    \value Hobareg
    \value Host
    \value If
    \value IfMatch
    \value IfModifiedSince
    \value IfNoneMatch
    \value IfRange
    \value IfScheduleTagMatch
    \value IfUnmodifiedSince
    \value IM
    \value IncludeReferredTokenBindingID
    \value KeepAlive
    \value Label
    \value LastEventID
    \value LastModified
    \value Link
    \value Location
    \value LockToken
    \value MaxForwards
    \value MementoDatetime
    \value Meter
    \value MIMEVersion
    \value Negotiate
    \value NEL
    \value ODataEntityId
    \value ODataIsolation
    \value ODataMaxVersion
    \value ODataVersion
    \value OptionalWWWAuthenticate
    \value OrderingType
    \value Origin
    \value OriginAgentCluster
    \value OSCORE
    \value OSLCCoreVersion
    \value Overwrite
    \value PingFrom
    \value PingTo
    \value Position
    \value Prefer
    \value PreferenceApplied
    \value Priority
    \value ProxyAuthenticate
    \value ProxyAuthenticationInfo
    \value ProxyAuthorization
    \value ProxyStatus
    \value PublicKeyPins
    \value PublicKeyPinsReportOnly
    \value Range
    \value RedirectRef
    \value Referer
    \value Refresh
    \value ReplayNonce
    \value ReprDigest
    \value RetryAfter
    \value ScheduleReply
    \value ScheduleTag
    \value SecPurpose
    \value SecTokenBinding
    \value SecWebSocketAccept
    \value SecWebSocketExtensions
    \value SecWebSocketKey
    \value SecWebSocketProtocol
    \value SecWebSocketVersion
    \value Server
    \value ServerTiming
    \value SetCookie
    \value Signature
    \value SignatureInput
    \value SLUG
    \value SoapAction
    \value StatusURI
    \value StrictTransportSecurity
    \value Sunset
    \value SurrogateCapability
    \value SurrogateControl
    \value TCN
    \value TE
    \value Timeout
    \value Topic
    \value Traceparent
    \value Tracestate
    \value Trailer
    \value TransferEncoding
    \value TTL
    \value Upgrade
    \value Urgency
    \value UserAgent
    \value VariantVary
    \value Vary
    \value Via
    \value WantContentDigest
    \value WantReprDigest
    \value WWWAuthenticate
    \value XContentTypeOptions
    \value XFrameOptions
    \value AcceptCharset
    \value CPEPInfo
    \value Pragma
    \value ProtocolInfo
    \value ProtocolQuery
*/

/*!
    \enum QHttpHeaders::CompareOption

    This enum type contains the options for comparing two
    QHttpHeaders instances.

    \value OrderInsensitive
    Specifies that the order of headers is not significant in the comparison.
    With this option, two QHttpHeaders instances will be considered equal
    if they contain the same headers regardless of their order. This is
    true with most HTTP headers and use cases.

    \value OrderSensitive
    Specifies that the order of headers is significant in the comparison.
    With this option, two QHttpHeaders instances will be considered equal
    only if they contain the same headers in the same exact order.
*/

/*!
    Creates a new QHttpHeaders object.
*/
QHttpHeaders::QHttpHeaders() : d(new QHttpHeadersPrivate)
{
}

/*!
    Creates a new QHttpHeaders object that is populated with
    \a headers.

    \sa {Allowed field name and value characters}
*/
QHttpHeaders QHttpHeaders::fromListOfPairs(const QList<std::pair<QByteArray, QByteArray>> &headers)
{
    QHttpHeaders h;
    h.d->headers.reserve(headers.size());
    for (const auto &header : headers)
        h.append(header.first, header.second);
    return h;
}

/*!
    Creates a new QHttpHeaders object that is populated with
    \a headers.

    \sa {Allowed field name and value characters}
*/
QHttpHeaders QHttpHeaders::fromMultiMap(const QMultiMap<QByteArray, QByteArray> &headers)
{
    QHttpHeaders h;
    h.d->headers.reserve(headers.size());
    for (const auto &[name,value] : headers.asKeyValueRange())
        h.append(name, value);
    return h;
}

/*!
    Creates a new QHttpHeaders object that is populated with
    \a headers.

    \sa {Allowed field name and value characters}
*/
QHttpHeaders QHttpHeaders::fromMultiHash(const QMultiHash<QByteArray, QByteArray> &headers)
{
    QHttpHeaders h;
    h.d->headers.reserve(headers.size());
    for (const auto &[name,value] : headers.asKeyValueRange())
        h.append(name, value);
    return h;
}

/*!
    Disposes of the headers object.
*/
QHttpHeaders::~QHttpHeaders()
    = default;

/*!
    Creates a copy of \a other.
*/
QHttpHeaders::QHttpHeaders(const QHttpHeaders &other)
    = default;

/*!
    Assigns the contents of \a other and returns a reference to this object.
*/
QHttpHeaders &QHttpHeaders::operator=(const QHttpHeaders &other)
    = default;

/*!
    \fn QHttpHeaders::QHttpHeaders(QHttpHeaders &&other) noexcept

    Move-constructs the object from \a other.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.
*/

/*!
    \fn QHttpHeaders &QHttpHeaders::operator=(QHttpHeaders &&other) noexcept

    Move-assigns \a other and returns a reference to this object.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.
*/

/*!
    \fn void QHttpHeaders::swap(QHttpHeaders &other)

    Swaps this QHttpHeaders with \a other. This function is very fast and
    never fails.
*/

#ifndef QT_NO_DEBUG_STREAM
/*!
    \fn QDebug QHttpHeaders::operator<<(QDebug debug,
                                        const QHttpHeaders &headers)

    Writes \a headers into \a debug stream.
*/
QDebug operator<<(QDebug debug, const QHttpHeaders &headers)
{
    const QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();

    debug << "QHttpHeaders(headers = ";
    const char *separator = "";
    for (const auto &h : headers.d->headers) {
        debug << separator << h.name << ':' << h.value;
        separator = " | ";
    }
    debug << ")";
    return debug;
}
#endif

// A clarification on string encoding:
// Setters and getters only accept names and values that are Latin-1 representable:
// Either they are directly ASCII/Latin-1, or if they are UTF-X, they only use first 256
// of the unicode points. For example using a '€' (U+20AC) in value would yield a warning
// and the call is ignored.
// Furthermore the 'name' has more strict rules than the 'value'

// TODO FIXME REMOVEME once this is merged:
// https://codereview.qt-project.org/c/qt/qtbase/+/508829
static bool isUtf8Latin1Representable(QUtf8StringView s) noexcept
{
    // L1 encoded in UTF8 has at most the form
    // - 0b0XXX'XXXX - US-ASCII
    // - 0b1100'00XX 0b10XX'XXXX - at most 8 non-zero LSB bits allowed in L1
    bool inMultibyte = false;
    for (unsigned char c : s) {
        if (c < 128) { // US-ASCII
            if (inMultibyte)
                return false; // invalid sequence
        } else {
            // decode as UTF-8:
            if ((c & 0b1110'0000) == 0b1100'0000) { // two-octet UTF-8 leader
                if (inMultibyte)
                    return false; // invalid sequence
                inMultibyte = true;
                const auto bits_7_to_11 = c & 0b0001'1111;
                if (bits_7_to_11 < 0b10)
                    return false; // invalid sequence (US-ASCII encoded in two octets)
                if (bits_7_to_11 > 0b11) // more than the two LSB
                    return false; // outside L1
            } else if ((c & 0b1100'0000) == 0b1000'0000) { // trailing UTF-8 octet
                if (!inMultibyte)
                    return false; // invalid sequence
                inMultibyte = false; // only one continuation allowed
            } else {
                return false; // invalid sequence or outside of L1
            }
        }
    }
    if (inMultibyte)
        return false; // invalid sequence: premature end
    return true;
}

static constexpr auto isValidHttpHeaderNameChar = [](uchar c) noexcept
{
    // RFC 9110 Chapters "5.1 Field Names" and "5.6.2 Tokens"
    // field-name     = token
    // token          = 1*tchar
    // tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*" /
    //                  "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
    //                  / DIGIT / ALPHA
    //                  ; any VCHAR, except delimiters
    // (for explanation on VCHAR see isValidHttpHeaderValueChar)
    return (('A' <= c && c <= 'Z')
            || ('a' <= c && c <= 'z')
            || ('0' <= c && c <= '9')
            || ('#' <= c && c <= '\'')
            || ('^' <= c && c <= '`')
            || c == '|' || c == '~' || c == '!' || c == '*' || c == '+' || c == '-' || c == '.');
};

static bool headerNameValidImpl(QLatin1StringView name) noexcept
{
    return std::all_of(name.begin(), name.end(), isValidHttpHeaderNameChar);
}

static bool headerNameValidImpl(QUtf8StringView name) noexcept
{
    // Traversing the UTF-8 string char-by-char is fine in this case as
    // the isValidHttpHeaderNameChar rejects any value above 0x7E. UTF-8
    // only has bytes <= 0x7F if they truly represent that ASCII character.
    return headerNameValidImpl(QLatin1StringView(QByteArrayView(name)));
}

static bool headerNameValidImpl(QStringView name) noexcept
{
    return std::all_of(name.begin(), name.end(), [](QChar c) {
        return isValidHttpHeaderNameChar(c.toLatin1());
    });
}

static bool isValidHttpHeaderNameField(QAnyStringView name) noexcept
{
    if (name.isEmpty()) {
        qCWarning(lcQHttpHeaders, "HTTP header name cannot be empty");
        return false;
    }
    const bool valid = name.visit([](auto name){ return headerNameValidImpl(name); });
    if (!valid)
        qCWarning(lcQHttpHeaders, "HTTP header name contained illegal character(s)");
    return valid;
}

static constexpr auto isValidHttpHeaderValueChar = [](uchar c) noexcept
{
    // RFC 9110 Chapter 5.5, Field Values
    // field-value    = *field-content
    // field-content  = field-vchar
    //                  [ 1*( SP / HTAB / field-vchar ) field-vchar ]
    // field-vchar    = VCHAR / obs-text
    // obs-text       = %x80-FF
    // VCHAR is defined as "any visible US-ASCII character", and RFC 5234 B.1.
    // defines it as %x21-7E
    // Note: The ABNF above states that field-content and thus field-value cannot
    // start or end with SP/HTAB. The caller should handle this.
    return (c >= 0x80 // obs-text (extended ASCII)
        || (0x20 <= c && c <= 0x7E) // SP (0x20) + VCHAR
        || (c == 0x09)); // HTAB
};

static bool headerValueValidImpl(QLatin1StringView value) noexcept
{
    return std::all_of(value.begin(), value.end(), isValidHttpHeaderValueChar);
}

static bool headerValueValidImpl(QUtf8StringView value) noexcept
{
    if (!isUtf8Latin1Representable(value)) // TODO FIXME see the function
        return false;
    return std::all_of(value.begin(), value.end(), isValidHttpHeaderValueChar);
}

static bool headerValueValidImpl(QStringView value) noexcept
{
    return std::all_of(value.begin(), value.end(), [](QChar c) {
        return isValidHttpHeaderValueChar(c.toLatin1());
    });
}

static bool isValidHttpHeaderValueField(QAnyStringView value) noexcept
{
    const bool valid = value.visit([](auto value){ return headerValueValidImpl(value); });
    if (!valid)
        qCWarning(lcQHttpHeaders, "HTTP header value contained illegal character(s)");
    return valid;
}

static QByteArray fieldToByteArray(QLatin1StringView s) noexcept
{
    return QByteArray(s.data(), s.size());
}

static QByteArray fieldToByteArray(QUtf8StringView s) noexcept
{
    return QByteArray(s.data(), s.size());
}

static QByteArray fieldToByteArray(QStringView s)
{
    return s.toLatin1();
}

static QByteArray normalizedName(QAnyStringView name)
{
    return name.visit([](auto name){ return fieldToByteArray(name); }).toLower();
}

static QByteArray normalizedValue(QAnyStringView value)
{
    // Note on trimming away any leading or trailing whitespace of 'value':
    // RFC 9110 (HTTP 1.1, 2022, Chapter 5.5) does not allow leading or trailing whitespace
    // RFC 7230 (HTTP 1.1, 2014, Chapter 3.2) allows them optionally, but also mandates that
    //          they are ignored during processing
    // RFC 7540 (HTTP/2) does not seem explicit about it
    // => for maximum compatibility, trim away any leading or trailing whitespace
    return value.visit([](auto value){ return fieldToByteArray(value); }).trimmed();
}

static bool headerNameIs(const Header &header, QAnyStringView name)
{
    return header.name == normalizedName(name);
}

/*!
    Appends a header entry with \a name and \a value and returns \c true
    if successful.

    \sa append(QHttpHeaders::WellKnownHeader, QAnyStringView)
    \sa {Allowed field name and value characters}
*/
bool QHttpHeaders::append(QAnyStringView name, QAnyStringView value)
{
    if (!isValidHttpHeaderNameField(name) || !isValidHttpHeaderValueField(value))
        return false;

    d.detach();
    d->headers.push_back({normalizedName(name), normalizedValue(value)});
    return true;
}

/*!
    \overload append(QAnyStringView, QAnyStringView)
*/
bool QHttpHeaders::append(WellKnownHeader name, QAnyStringView value)
{
    if (!isValidHttpHeaderValueField(value))
        return false;

    d.detach();
    d->headers.push_back({headerNames[qToUnderlying(name)], normalizedValue(value)});
    return true;
}

/*!
    Inserts a header entry at index \a i, with \a name and \a value. The index
    must be valid (see \l size()). Returns whether the insert succeeded.

    \sa append(),
       insert(qsizetype, QHttpHeaders::WellKnownHeader, QAnyStringView), size()
    \sa {Allowed field name and value characters}
*/
bool QHttpHeaders::insert(qsizetype i, QAnyStringView name, QAnyStringView value)
{
    d->verify(i, 0);
    if (!isValidHttpHeaderNameField(name) || !isValidHttpHeaderValueField(value))
        return false;

    d.detach();
    d->headers.insert(i, {normalizedName(name), normalizedValue(value)});
    return true;
}

/*!
    \overload insert(qsizetype, QAnyStringView, QAnyStringView)
*/
bool QHttpHeaders::insert(qsizetype i, WellKnownHeader name, QAnyStringView value)
{
    d->verify(i, 0);
    if (!isValidHttpHeaderValueField(value))
        return false;

    d.detach();
    d->headers.insert(i, {headerNames[qToUnderlying(name)], normalizedValue(value)});
    return true;
}

/*!
    Replaces the header entry at index \a i, with \a name and \a value.
    The index must be valid (see \l size()). Returns whether the replace
    succeeded.

    \sa append(),
      replace(qsizetype, QHttpHeaders::WellKnownHeader, QAnyStringView), size()
    \sa {Allowed field name and value characters}
*/
bool QHttpHeaders::replace(qsizetype i, QAnyStringView name, QAnyStringView value)
{
    d->verify(i);
    if (!isValidHttpHeaderNameField(name) || !isValidHttpHeaderValueField(value))
        return false;

    d.detach();
    d->headers.replace(i, {normalizedName(name), normalizedValue(value)});
    return true;
}

/*!
    \overload replace(qsizetype, QAnyStringView, QAnyStringView)
*/
bool QHttpHeaders::replace(qsizetype i, WellKnownHeader name, QAnyStringView value)
{
    d->verify(i);
    if (!isValidHttpHeaderValueField(value))
        return false;

    d.detach();
    d->headers.replace(i, {headerNames[qToUnderlying(name)], normalizedValue(value)});
    return true;
}

/*!
    Returns whether the headers contain header with \a name.

    \sa has(QHttpHeaders::WellKnownHeader)
*/
bool QHttpHeaders::has(QAnyStringView name) const
{
    return std::any_of(d->headers.cbegin(), d->headers.cend(),
                       [&name](const Header &header) { return headerNameIs(header, name); });
}

/*!
    \overload has(QAnyStringView)
*/
bool QHttpHeaders::has(WellKnownHeader name) const
{
    return has(headerNames[qToUnderlying(name)]);
}

/*!
    Returns a list of unique header names.
    Header names are case-insensitive, and the returned
    names are lower-cased.
*/
QList<QByteArray> QHttpHeaders::names() const
{
    QList<QByteArray> names;
    for (const Header &header: d->headers) {
        if (!names.contains(header.name))
            names.append(header.name);
    }
    return names;
}

/*!
    Removes the header \a name.

    \sa removeAt(), removeAll(QHttpHeaders::WellKnownHeader)
*/
void QHttpHeaders::removeAll(QAnyStringView name)
{
    if (has(name)) {
        d.detach();
        d->headers.removeIf([&name](const Header &header){
            return headerNameIs(header, name);
        });
    }
}

/*!
    \overload removeAll(QAnyStringView)
*/
void QHttpHeaders::removeAll(WellKnownHeader name)
{
    removeAll(headerNames[qToUnderlying(name)]);
}

/*!
    Removes the header at index \a i. The index \a i must be valid
    (see \l size()).

    \sa removeAll(QHttpHeaders::WellKnownHeader),
        removeAll(QAnyStringView), size()
*/
void QHttpHeaders::removeAt(qsizetype i)
{
    d->verify(i);
    d.detach();
    d->headers.removeAt(i);
}

/*!
    Returns the value of the (first) header \a name, or \a defaultValue if it
    doesn't exist.

    \sa value(QHttpHeaders::WellKnownHeader, QByteArrayView)
*/
QByteArrayView QHttpHeaders::value(QAnyStringView name, QByteArrayView defaultValue) const noexcept
{
    for (const auto &h : std::as_const(d->headers)) {
        if (headerNameIs(h, name))
            return h.value;
    }
    return defaultValue;
}

/*!
    \overload value(QAnyStringView, QByteArrayView)
*/
QByteArrayView QHttpHeaders::value(WellKnownHeader name, QByteArrayView defaultValue) const noexcept
{
    return value(headerNames[qToUnderlying(name)], defaultValue);
}

/*!
    Returns the values of header \a name in a list. Returns an empty
    list if header with \a name doesn't exist.

    \sa values(QHttpHeaders::WellKnownHeader)
*/
QList<QByteArray> QHttpHeaders::values(QAnyStringView name) const
{
    QList<QByteArray> values;
    for (const auto &h : std::as_const(d->headers)) {
        if (headerNameIs(h, name))
            values.append(h.value);
    }
    return values;
}

/*!
    \overload values(QAnyStringView)
*/
QList<QByteArray> QHttpHeaders::values(WellKnownHeader name) const
{
    return values(headerNames[qToUnderlying(name)]);
}

/*!
    Returns the values of header \a name in a comma-combined string.
    Returns a \c null QByteArray if the header with \a name doesn't
    exist.

    \note Accessing the value(s) of 'Set-Cookie' header this way may not work
    as intended. It is a notable exception in the
    \l {https://datatracker.ietf.org/doc/html/rfc9110#name-field-order}{HTTP RFC}
    in that its values cannot be combined this way. Prefer \l values() instead.

    \sa values(QAnyStringView)
*/
QByteArray QHttpHeaders::combinedValue(QAnyStringView name) const
{
    QByteArray result;

    const char* separator = "";
    auto valueList = values(name);
    for (const auto &v : valueList) {
        result.append(separator);
        result.append(v);
        separator = ",";
    }
    return result;
}

/*!
    \overload combinedValue(QAnyStringView)
*/
QByteArray QHttpHeaders::combinedValue(WellKnownHeader name) const
{
    return combinedValue(headerNames[qToUnderlying(name)]);
}

/*!
    Returns the number of header entries.
*/
qsizetype QHttpHeaders::size() const noexcept
{
    return d->headers.size();
}

/*!
    Attempts to allocate memory for at least \a size header entries.

    If you know in advance how how many header entries there will
    be, you may call this function to prevent reallocations
    and memory fragmentation.
*/
void QHttpHeaders::reserve(qsizetype size)
{
    d->headers.reserve(size);
}

/*!
    Compares this instance with \a other and returns \c true if they
    are considered equal in accordance with the provided \a options.

    The header names are always compared as case-insensitive, and values
    as case-sensitive. For example \e Accept and \e ACCEPT header names
    are considered equal, while values \e something and \e SOMETHING are not.
*/
bool QHttpHeaders::equals(const QHttpHeaders &other, CompareOptions options) const noexcept
{
    return d == other.d || d->equals(*other.d, options);
}

/*!
    Returns the header entries as a list of (name, value) pairs.
    Header names are case-insensitive, and the returned names are lower-cased.
*/
QList<std::pair<QByteArray, QByteArray>> QHttpHeaders::toListOfPairs() const
{
    QList<std::pair<QByteArray, QByteArray>> list;
    list.reserve(size());
    for (const auto & h : std::as_const(d->headers))
        list.append({h.name, h.value});
    return list;
}

/*!
    Returns the header entries as a map from name to value(s).
    Header names are case-insensitive, and the returned names are lower-cased.
*/
QMultiMap<QByteArray, QByteArray> QHttpHeaders::toMultiMap() const
{
    QMultiMap<QByteArray, QByteArray> map;
    for (const auto &h : std::as_const(d->headers))
        map.insert(h.name, h.value);
    return map;
}

/*!
    Returns the header entries as a hash from name to value(s).
    Header names are case-insensitive, and the returned names are lower-cased.
*/
QMultiHash<QByteArray, QByteArray> QHttpHeaders::toMultiHash() const
{
    QMultiHash<QByteArray, QByteArray> hash;
    hash.reserve(size());
    for (const auto &h : std::as_const(d->headers))
        hash.insert(h.name, h.value);
    return hash;
}

/*!
    Clears all header entries.

    \sa size()
*/
void QHttpHeaders::clear()
{
    if (d->headers.isEmpty())
        return;
    d.detach();
    d->headers.clear();
}

QT_END_NAMESPACE
