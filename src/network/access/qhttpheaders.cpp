// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qhttpheaders.h"

#include <QtNetwork/private/qnetworkrequest_p.h>

#include <private/qoffsetstringarray_p.h>

#include <QtCore/qcompare.h>
#include <QtCore/qhash.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmap.h>
#include <QtCore/qset.h>
#include <QtCore/qttypetraits.h>
#include <QtCore/qxpfunctional.h>

#include <q20algorithm.h>
#include <string_view>
#include <variant>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcQHttpHeaders, "qt.network.http.headers");

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

    In all, this means:
    \list
        \li \c name must consist of visible ASCII characters, and must not be
            empty
        \li \c value may consist of arbitrary bytes, as long as header
            and use case specific encoding rules are adhered to. \c value
            may be empty
    \endlist

    The setters of this class automatically remove any leading or trailing
    whitespaces from \e value, as they must be ignored during the
    \e value processing.

    \section1 Combining values

    Most HTTP header values can be combined with a single comma \c {','}
    plus an optional whitespace, and the semantic meaning is preserved.
    As an example, these two should be semantically similar:
    \badcode
        // Values as separate header entries
        myheadername: myheadervalue1
        myheadername: myheadervalue2
        // Combined value
        myheadername: myheadervalue1, myheadervalue2
    \endcode

    However, there is a notable exception to this rule:
    \l {https://datatracker.ietf.org/doc/html/rfc9110#name-field-order}
    {Set-Cookie}. Due to this and the possibility of custom use cases,
    QHttpHeaders does not automatically combine the values.

    \section1 Performance

    Most QHttpHeaders functions provide both
    \l QHttpHeaders::WellKnownHeader and \l QAnyStringView overloads.
    From a memory-usage and computation point of view it is recommended
    to use the \l QHttpHeaders::WellKnownHeader overloads.
*/

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
    // If you append here, regenerate the index table
);

namespace {
struct ByIndirectHeaderName
{
    constexpr bool operator()(quint8 lhs, quint8 rhs) const noexcept
    {
        return (*this)(map(lhs), map(rhs));
    }
    constexpr bool operator()(quint8 lhs, QByteArrayView rhs) const noexcept
    {
        return (*this)(map(lhs), rhs);
    }
    constexpr bool operator()(QByteArrayView lhs, quint8 rhs) const noexcept
    {
        return (*this)(lhs, map(rhs));
    }
    constexpr bool operator()(QByteArrayView lhs, QByteArrayView rhs) const noexcept
    {
        // ### just `lhs < rhs` when QByteArrayView relational operators are constexpr
        return std::string_view(lhs) < std::string_view(rhs);
    }
private:
    static constexpr QByteArrayView map(quint8 i) noexcept
    {
        return headerNames.viewAt(i);
    }
};
} // unnamed namespace

// This index table contains the indexes of 'headerNames' entries (above) in alphabetical order.
// This allows a more efficient binary search for the names [O(logN)]. The 'headerNames' itself
// cannot be guaranteed to be in alphabetical order, as it must keep the same order as the
// WellKnownHeader enum, which may get appended over time.
//
// Note: when appending new enums, this must be regenerated
static constexpr quint8 orderedHeaderNameIndexes[] = {
    0, // a-im
    1, // accept
    2, // accept-additions
    3, // accept-ch
    172, // accept-charset
    4, // accept-datetime
    5, // accept-encoding
    6, // accept-features
    7, // accept-language
    8, // accept-patch
    9, // accept-post
    10, // accept-ranges
    11, // accept-signature
    12, // access-control-allow-credentials
    13, // access-control-allow-headers
    14, // access-control-allow-methods
    15, // access-control-allow-origin
    16, // access-control-expose-headers
    17, // access-control-max-age
    18, // access-control-request-headers
    19, // access-control-request-method
    20, // age
    21, // allow
    22, // alpn
    23, // alt-svc
    24, // alt-used
    25, // alternates
    26, // apply-to-redirect-ref
    27, // authentication-control
    28, // authentication-info
    29, // authorization
    173, // c-pep-info
    30, // cache-control
    31, // cache-status
    32, // cal-managed-id
    33, // caldav-timezones
    34, // capsule-protocol
    35, // cdn-cache-control
    36, // cdn-loop
    37, // cert-not-after
    38, // cert-not-before
    39, // clear-site-data
    40, // client-cert
    41, // client-cert-chain
    42, // close
    43, // connection
    44, // content-digest
    45, // content-disposition
    46, // content-encoding
    47, // content-id
    48, // content-language
    49, // content-length
    50, // content-location
    51, // content-range
    52, // content-security-policy
    53, // content-security-policy-report-only
    54, // content-type
    55, // cookie
    56, // cross-origin-embedder-policy
    57, // cross-origin-embedder-policy-report-only
    58, // cross-origin-opener-policy
    59, // cross-origin-opener-policy-report-only
    60, // cross-origin-resource-policy
    61, // dasl
    62, // date
    63, // dav
    64, // delta-base
    65, // depth
    66, // destination
    67, // differential-id
    68, // dpop
    69, // dpop-nonce
    70, // early-data
    71, // etag
    72, // expect
    73, // expect-ct
    74, // expires
    75, // forwarded
    76, // from
    77, // hobareg
    78, // host
    79, // if
    80, // if-match
    81, // if-modified-since
    82, // if-none-match
    83, // if-range
    84, // if-schedule-tag-match
    85, // if-unmodified-since
    86, // im
    87, // include-referred-token-binding-id
    88, // keep-alive
    89, // label
    90, // last-event-id
    91, // last-modified
    92, // link
    93, // location
    94, // lock-token
    95, // max-forwards
    96, // memento-datetime
    97, // meter
    98, // mime-version
    99, // negotiate
    100, // nel
    101, // odata-entityid
    102, // odata-isolation
    103, // odata-maxversion
    104, // odata-version
    105, // optional-www-authenticate
    106, // ordering-type
    107, // origin
    108, // origin-agent-cluster
    109, // oscore
    110, // oslc-core-version
    111, // overwrite
    112, // ping-from
    113, // ping-to
    114, // position
    174, // pragma
    115, // prefer
    116, // preference-applied
    117, // priority
    175, // protocol-info
    176, // protocol-query
    118, // proxy-authenticate
    119, // proxy-authentication-info
    120, // proxy-authorization
    121, // proxy-status
    122, // public-key-pins
    123, // public-key-pins-report-only
    124, // range
    125, // redirect-ref
    126, // referer
    127, // refresh
    128, // replay-nonce
    129, // repr-digest
    130, // retry-after
    131, // schedule-reply
    132, // schedule-tag
    133, // sec-purpose
    134, // sec-token-binding
    135, // sec-websocket-accept
    136, // sec-websocket-extensions
    137, // sec-websocket-key
    138, // sec-websocket-protocol
    139, // sec-websocket-version
    140, // server
    141, // server-timing
    142, // set-cookie
    143, // signature
    144, // signature-input
    145, // slug
    146, // soapaction
    147, // status-uri
    148, // strict-transport-security
    149, // sunset
    150, // surrogate-capability
    151, // surrogate-control
    152, // tcn
    153, // te
    154, // timeout
    155, // topic
    156, // traceparent
    157, // tracestate
    158, // trailer
    159, // transfer-encoding
    160, // ttl
    161, // upgrade
    162, // urgency
    163, // user-agent
    164, // variant-vary
    165, // vary
    166, // via
    167, // want-content-digest
    168, // want-repr-digest
    169, // www-authenticate
    170, // x-content-type-options
    171, // x-frame-options
};
static_assert(std::size(orderedHeaderNameIndexes) == size_t(headerNames.count()));
#if !defined(Q_CC_GNU_ONLY) || Q_CC_GNU_ONLY >= 1000
static_assert(q20::is_sorted(std::begin(orderedHeaderNameIndexes),
                             std::end(orderedHeaderNameIndexes),
                             ByIndirectHeaderName{}));
#endif

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

struct HeaderName
{
    explicit HeaderName(QHttpHeaders::WellKnownHeader name) : data(name)
    {
    }

    explicit HeaderName(QAnyStringView name)
    {
        auto nname = normalizedName(name);
        if (auto h = HeaderName::toWellKnownHeader(nname))
            data = *h;
        else
            data = std::move(nname);
    }

    // Returns an enum corresponding with the 'name' if possible. Uses binary search (O(logN)).
    // The function doesn't normalize the data; needs to be done by the caller if needed
    static std::optional<QHttpHeaders::WellKnownHeader> toWellKnownHeader(QByteArrayView name) noexcept
    {
        auto indexesBegin = std::cbegin(orderedHeaderNameIndexes);
        auto indexesEnd = std::cend(orderedHeaderNameIndexes);

        auto result = std::lower_bound(indexesBegin, indexesEnd, name, ByIndirectHeaderName{});

        if (result != indexesEnd && name == headerNames[*result])
            return static_cast<QHttpHeaders::WellKnownHeader>(*result);
        return std::nullopt;
    }

    QByteArrayView asView() const noexcept
    {
        return std::visit([](const auto &arg) -> QByteArrayView {
            using T = decltype(arg);
            if constexpr (std::is_same_v<T, const QByteArray &>)
                return arg;
            else if constexpr (std::is_same_v<T, const QHttpHeaders::WellKnownHeader &>)
                return headerNames.viewAt(qToUnderlying(arg));
            else
                static_assert(QtPrivate::type_dependent_false<T>());
        }, data);
    }

    QByteArray asByteArray() const noexcept
    {
        return std::visit([](const auto &arg) -> QByteArray {
            using T = decltype(arg);
            if constexpr (std::is_same_v<T, const QByteArray &>) {
                return arg;
            } else if constexpr (std::is_same_v<T, const QHttpHeaders::WellKnownHeader &>) {
                const auto view = headerNames.viewAt(qToUnderlying(arg));
                return QByteArray::fromRawData(view.constData(), view.size());
            } else {
                static_assert(QtPrivate::type_dependent_false<T>());
            }
        }, data);
    }

private:
    // Store the data as 'enum' whenever possible; more performant, and comparison relies on that
    std::variant<QHttpHeaders::WellKnownHeader, QByteArray> data;

    friend bool comparesEqual(const HeaderName &lhs, const HeaderName &rhs) noexcept
    {
        // Here we compare two std::variants, which will return false if the types don't match.
        // That is beneficial here because we avoid unnecessary comparisons; but it also means
        // we must always store the data as WellKnownHeader when possible (in other words, if
        // we get a string that is mappable to a WellKnownHeader). To guard against accidental
        // misuse, the 'data' is private and the constructors must be used.
        return lhs.data == rhs.data;
    }
    Q_DECLARE_EQUALITY_COMPARABLE(HeaderName)
};

// A clarification on case-sensitivity:
// - Header *names*  are case-insensitive; Content-Type and content-type are considered equal
// - Header *values* are case-sensitive
// (In addition, the HTTP/2 and HTTP/3 standards mandate that all headers must be lower-cased when
// encoded into transmission)
struct Header {
    HeaderName name;
    QByteArray value;
};

auto headerNameMatches(const HeaderName &name)
{
    return [&name](const Header &header) { return header.name == name; };
}

class QHttpHeadersPrivate : public QSharedData
{
public:
    QHttpHeadersPrivate() = default;

    // The 'Self' is supplied as parameter to static functions so that
    // we can define common methods which 'detach()' the private itself.
    using Self = QExplicitlySharedDataPointer<QHttpHeadersPrivate>;
    static void removeAll(Self &d, const HeaderName &name);
    static void replaceOrAppend(Self &d, const HeaderName &name, QByteArray value);

    void combinedValue(const HeaderName &name, QByteArray &result) const;
    void values(const HeaderName &name, QList<QByteArray> &result) const;
    QByteArrayView value(const HeaderName &name, QByteArrayView defaultValue) const noexcept;
    void forEachHeader(QAnyStringView name,
                       qxp::function_ref<void(QByteArrayView)> yield);
    std::optional<QByteArrayView> findValue(const HeaderName &name) const noexcept;

    QList<Header> headers;
};

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QHttpHeadersPrivate)
template <> void QExplicitlySharedDataPointer<QHttpHeadersPrivate>::detach()
{
    if (!d) {
        d.reset(new QHttpHeadersPrivate());
        d->ref.ref();
    } else if (d->ref.loadRelaxed() != 1) {
        detach_helper();
    }
}

void QHttpHeadersPrivate::removeAll(Self &d, const HeaderName &name)
{
    const auto it = std::find_if(d->headers.cbegin(), d->headers.cend(), headerNameMatches(name));

    if (it != d->headers.cend()) {
        // Found something to remove, calculate offset so we can proceed from the match-location
        const auto matchOffset = it - d->headers.cbegin();
        d.detach();
        // Rearrange all matches to the end and erase them
        d->headers.erase(std::remove_if(d->headers.begin() + matchOffset, d->headers.end(),
                                        headerNameMatches(name)),
                         d->headers.end());
    }
}

void QHttpHeadersPrivate::combinedValue(const HeaderName &name, QByteArray &result) const
{
    const char* separator = "";
    for (const auto &h : std::as_const(headers)) {
        if (h.name == name) {
            result.append(separator);
            result.append(h.value);
            separator = ", ";
        }
    }
}

void QHttpHeadersPrivate::values(const HeaderName &name, QList<QByteArray> &result) const
{
    for (const auto &h : std::as_const(headers)) {
        if (h.name == name)
            result.append(h.value);
    }
}

QByteArrayView QHttpHeadersPrivate::value(const HeaderName &name, QByteArrayView defaultValue) const noexcept
{
    for (const auto &h : std::as_const(headers)) {
        if (h.name == name)
            return h.value;
    }
    return defaultValue;
}

void QHttpHeadersPrivate::replaceOrAppend(Self &d, const HeaderName &name, QByteArray value)
{
    d.detach();
    auto it = std::find_if(d->headers.begin(), d->headers.end(), headerNameMatches(name));
    if (it != d->headers.end()) {
        // Found something to replace => replace, and then rearrange any remaining
        // matches to the end and erase them
        it->value = std::move(value);
        d->headers.erase(
                std::remove_if(it + 1, d->headers.end(), headerNameMatches(name)),
                d->headers.end());
    } else {
        // Found nothing to replace => append
        d->headers.append(Header{name, std::move(value)});
    }
}

void QHttpHeadersPrivate::forEachHeader(QAnyStringView name,
                                        qxp::function_ref<void(QByteArrayView)> yield)
{
    for (const auto &h : std::as_const(headers)) {
        if (h.name.asView() == name)
            yield(h.value);
    }
}

std::optional<QByteArrayView> QHttpHeadersPrivate::findValue(const HeaderName &name) const noexcept
{
    for (const auto &h : headers) {
        if (h.name == name)
            return h.value;
    }
    return std::nullopt;
}

/*!
    Creates a new QHttpHeaders object.
*/
QHttpHeaders::QHttpHeaders() noexcept : d()
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
    h.reserve(headers.size());
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
    h.reserve(headers.size());
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
    h.reserve(headers.size());
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

    Move-constructs the object from \a other, which will be left
    \l{isEmpty()}{empty}.
*/

/*!
    \fn QHttpHeaders &QHttpHeaders::operator=(QHttpHeaders &&other) noexcept

    Move-assigns \a other and returns a reference to this object.

    \a other will be left \l{isEmpty()}{empty}.
*/

/*!
    \fn void QHttpHeaders::swap(QHttpHeaders &other)
    \memberswap{QHttpHeaders}
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

    debug << "QHttpHeaders(";
    if (headers.d) {
        debug << "headers = ";
        const char *separator = "";
        for (const auto &h : headers.d->headers) {
            debug << separator << h.name.asView() << ':' << h.value;
            separator = " | ";
        }
    }
    debug << ")";
    return debug;
}
#endif

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
    // UTF-8 byte sequences are also used as values directly
    // => allow them as such. UTF-8 byte sequences for characters
    // outside of ASCII should all fit into obs-text (>= 0x80)
    // (see  isValidHttpHeaderValueChar)
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
    d->headers.push_back({HeaderName{name}, normalizedValue(value)});
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
    d->headers.push_back({HeaderName{name}, normalizedValue(value)});
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
    verify(i, 0);
    if (!isValidHttpHeaderNameField(name) || !isValidHttpHeaderValueField(value))
        return false;

    d.detach();
    d->headers.insert(i, {HeaderName{name}, normalizedValue(value)});
    return true;
}

/*!
    \overload insert(qsizetype, QAnyStringView, QAnyStringView)
*/
bool QHttpHeaders::insert(qsizetype i, WellKnownHeader name, QAnyStringView value)
{
    verify(i, 0);
    if (!isValidHttpHeaderValueField(value))
        return false;

    d.detach();
    d->headers.insert(i, {HeaderName{name}, normalizedValue(value)});
    return true;
}

/*!
    Replaces the header entry at index \a i, with \a name and \a newValue.
    The index must be valid (see \l size()). Returns whether the replace
    succeeded.

    \sa append(),
      replace(qsizetype, QHttpHeaders::WellKnownHeader, QAnyStringView), size()
    \sa {Allowed field name and value characters}
*/
bool QHttpHeaders::replace(qsizetype i, QAnyStringView name, QAnyStringView newValue)
{
    verify(i);
    if (!isValidHttpHeaderNameField(name) || !isValidHttpHeaderValueField(newValue))
        return false;

    d.detach();
    d->headers.replace(i, {HeaderName{name}, normalizedValue(newValue)});
    return true;
}

/*!
    \overload replace(qsizetype, QAnyStringView, QAnyStringView)
*/
bool QHttpHeaders::replace(qsizetype i, WellKnownHeader name, QAnyStringView newValue)
{
    verify(i);
    if (!isValidHttpHeaderValueField(newValue))
        return false;

    d.detach();
    d->headers.replace(i, {HeaderName{name}, normalizedValue(newValue)});
    return true;
}

/*!
    \since 6.8

    If QHttpHeaders already contains \a name, replaces its value with
    \a newValue and removes possible additional \a name entries.
    If \a name didn't exist, appends a new entry. Returns \c true
    if successful.

    This function is a convenience method for setting a unique
    \a name : \a newValue header. For most headers the relative order does not
    matter, which allows reusing an existing entry if one exists.

    \sa replaceOrAppend(QAnyStringView, QAnyStringView)
*/
bool QHttpHeaders::replaceOrAppend(WellKnownHeader name, QAnyStringView newValue)
{
    if (isEmpty())
        return append(name, newValue);

    if (!isValidHttpHeaderValueField(newValue))
        return false;

    QHttpHeadersPrivate::replaceOrAppend(d, HeaderName{name}, normalizedValue(newValue));
    return true;
}

/*!
    \overload replaceOrAppend(WellKnownHeader, QAnyStringView)
*/
bool QHttpHeaders::replaceOrAppend(QAnyStringView name, QAnyStringView newValue)
{
    if (isEmpty())
        return append(name, newValue);

    if (!isValidHttpHeaderNameField(name) || !isValidHttpHeaderValueField(newValue))
        return false;

    QHttpHeadersPrivate::replaceOrAppend(d, HeaderName{name}, normalizedValue(newValue));
    return true;
}

/*!
    Returns whether the headers contain header with \a name.

    \sa contains(QHttpHeaders::WellKnownHeader)
*/
bool QHttpHeaders::contains(QAnyStringView name) const
{
    if (isEmpty())
        return false;

    return std::any_of(d->headers.cbegin(), d->headers.cend(), headerNameMatches(HeaderName{name}));
}

/*!
    \overload has(QAnyStringView)
*/
bool QHttpHeaders::contains(WellKnownHeader name) const
{
    if (isEmpty())
        return false;

    return std::any_of(d->headers.cbegin(), d->headers.cend(), headerNameMatches(HeaderName{name}));
}

/*!
    Removes the header \a name.

    \sa removeAt(), removeAll(QHttpHeaders::WellKnownHeader)
*/
void QHttpHeaders::removeAll(QAnyStringView name)
{
    if (isEmpty())
        return;

    return QHttpHeadersPrivate::removeAll(d, HeaderName(name));
}

/*!
    \overload removeAll(QAnyStringView)
*/
void QHttpHeaders::removeAll(WellKnownHeader name)
{
    if (isEmpty())
        return;

    return QHttpHeadersPrivate::removeAll(d, HeaderName(name));
}

/*!
    Removes the header at index \a i. The index \a i must be valid
    (see \l size()).

    \sa removeAll(QHttpHeaders::WellKnownHeader),
        removeAll(QAnyStringView), size()
*/
void QHttpHeaders::removeAt(qsizetype i)
{
    verify(i);
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
    if (isEmpty())
        return defaultValue;

    return d->value(HeaderName{name}, defaultValue);
}

/*!
    \overload value(QAnyStringView, QByteArrayView)
*/
QByteArrayView QHttpHeaders::value(WellKnownHeader name, QByteArrayView defaultValue) const noexcept
{
    if (isEmpty())
        return defaultValue;

    return d->value(HeaderName{name}, defaultValue);
}

/*!
    Returns the values of header \a name in a list. Returns an empty
    list if header with \a name doesn't exist.

    \sa values(QHttpHeaders::WellKnownHeader)
*/
QList<QByteArray> QHttpHeaders::values(QAnyStringView name) const
{
    QList<QByteArray> result;
    if (isEmpty())
        return result;

    d->values(HeaderName{name}, result);
    return result;
}

/*!
    \overload values(QAnyStringView)
*/
QList<QByteArray> QHttpHeaders::values(WellKnownHeader name) const
{
    QList<QByteArray> result;
    if (isEmpty())
        return result;

    d->values(HeaderName{name}, result);
    return result;
}

/*!
    Returns the header value at index \a i. The index \a i must be valid
    (see \l size()).

    \sa size(), value(), values(), combinedValue(), nameAt()
*/
QByteArrayView QHttpHeaders::valueAt(qsizetype i) const noexcept
{
    verify(i);
    return d->headers.at(i).value;
}

/*!
    Returns the header name at index \a i. The index \a i must be valid
    (see \l size()).

    Header names are case-insensitive, and the returned names are lower-cased.

    \sa size(), valueAt()
*/
QLatin1StringView QHttpHeaders::nameAt(qsizetype i) const noexcept
{
    verify(i);
    return QLatin1StringView{d->headers.at(i).name.asView()};
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
    if (isEmpty())
        return result;

    d->combinedValue(HeaderName{name}, result);
    return result;
}

/*!
    \overload combinedValue(QAnyStringView)
*/
QByteArray QHttpHeaders::combinedValue(WellKnownHeader name) const
{
    QByteArray result;
    if (isEmpty())
        return result;

    d->combinedValue(HeaderName{name}, result);
    return result;
}

/*!
    \since 6.10

    Returns the value of the first valid header \a name interpreted as a
    64-bit integer.
    If the header does not exist or cannot be parsed as an integer, returns
    \c std::nullopt.

    \sa intValues(QAnyStringView name), intValueAt(qsizetype i)
*/
std::optional<qint64> QHttpHeaders::intValue(QAnyStringView name) const noexcept
{
    std::optional<QByteArrayView> v = d->findValue(HeaderName{name});
    if (!v)
        return std::nullopt;
    bool ok = false;
    const qint64 result = v->toLongLong(&ok);
    if (ok)
        return result;
    return std::nullopt;
}

/*!
    \since 6.10
    \overload intValue(QAnyStringView)
*/
std::optional<qint64> QHttpHeaders::intValue(WellKnownHeader name) const noexcept
{
    return intValue(wellKnownHeaderName(name));
}

/*!
    \since 6.10

    Returns the values of the header \a name interpreted as 64-bit integer
    in a list. If the header does not exist or cannot be parsed as an integer,
    returns \c std::nullopt.

    \sa intValue(QAnyStringView name), intValueAt(qsizetype i)
*/
std::optional<QList<qint64>> QHttpHeaders::intValues(QAnyStringView name) const
{
    QList<qint64> results;
    d->forEachHeader(name, [&](QByteArrayView value) {
        bool ok = false;
        qint64 result = value.toLongLong(&ok);
        if (ok)
            results.append(result);
    });
    return results.isEmpty() ? std::nullopt :
                               std::make_optional(std::move(results));
}

/*!
    \since 6.10
    \overload intValues(QAnyStringView)
*/
std::optional<QList<qint64>> QHttpHeaders::intValues(WellKnownHeader name) const
{
    return intValues(wellKnownHeaderName(name));
}

/*!
    \since 6.10

    Returns the header value interpreted as 64-bit integer at index \a i.
    The index \a i must be valid.

    \sa intValues(QAnyStringView name), intValue(QAnyStringView name)
*/
std::optional<qint64> QHttpHeaders::intValueAt(qsizetype i) const noexcept
{
    verify(i);
    QByteArrayView v = valueAt(i);
    if (v.isEmpty())
        return std::nullopt;
    bool ok = false;
    const qint64 result = v.toLongLong(&ok);
    return ok ? std::optional<qint64>(result) :
                std::nullopt;
}

/*!
    \since 6.10

    Converts the first found header value of \a name to a QDateTime object, following
    the standard HTTP date formats. If the header does not exist or contains an invalid
    QDateTime, returns \c std::nullopt.

    \sa dateTimeValues(QAnyStringView name), dateTimeValueAt(qsizetype i)
*/
std::optional<QDateTime> QHttpHeaders::dateTimeValue(QAnyStringView name) const
{
    std::optional<QByteArrayView> v = d->findValue(HeaderName{name});
    if (!v)
        return std::nullopt;
    QDateTime dt = QNetworkHeadersPrivate::fromHttpDate(*v);
    if (dt.isValid())
        return dt;
    return std::nullopt;
}

/*!
    \since 6.10
    \overload dateTimeValue(QAnyStringView)
*/
std::optional<QDateTime> QHttpHeaders::dateTimeValue(WellKnownHeader name) const
{
    return dateTimeValue(wellKnownHeaderName(name));
}

/*!
    \since 6.10

    Sets the value of the header name \a name to \a dateTime,
    following the
    \l {https://datatracker.ietf.org/doc/html/rfc9110#name-date-time-formats}{standard HTTP IMF-fixdate format}.
    If the header does not exist, adds a new one.

    \sa dateTimeValue(QAnyStringView name), dateTimeValueAt(qsizetype i)
 */
void QHttpHeaders::setDateTimeValue(QAnyStringView name, const QDateTime &dateTime)
{
    if (!dateTime.isValid()) {
        qWarning("QHttpHeaders::setDateTimeValue: invalid QDateTime value received");
        return;
    }
    replaceOrAppend(name, QNetworkHeadersPrivate::toHttpDate(dateTime));
}

/*!
    \since 6.10
    \overload setDateTimeValue(QAnyStringView)
*/
void QHttpHeaders::setDateTimeValue(WellKnownHeader name, const QDateTime &dateTime)
{
    setDateTimeValue(wellKnownHeaderName(name), dateTime);
}

/*!
    \since 6.10

    Returns all the header values of \a name in a list of QDateTime objects, following
    the standard HTTP date formats. If no valid date-time values are found, returns
    \c std::nullopt.

    \sa dateTimeValue(QAnyStringView name), dateTimeValueAt(qsizetype i)
*/
std::optional<QList<QDateTime>> QHttpHeaders::dateTimeValues(QAnyStringView name) const
{
    QList<QDateTime> results;
    d->forEachHeader(name, [&](QByteArrayView value) {
        QDateTime dt = QNetworkHeadersPrivate::fromHttpDate(value);
        if (dt.isValid())
            results.append(std::move(dt));
    });
    return results.isEmpty() ? std::nullopt :
                               std::make_optional(std::move(results));
}

/*!
    \since 6.10
    \overload dateTimeValues(QAnyStringView)
*/
std::optional<QList<QDateTime>> QHttpHeaders::dateTimeValues(WellKnownHeader name) const
{
    return dateTimeValues(wellKnownHeaderName(name));
}

/*!
    \since 6.10

    Converts the header value at index \a i to a QDateTime object following the standard
    HTTP date formats. The index \a i must be valid.

    \sa dateTimeValue(QAnyStringView name), dateTimeValues(QAnyStringView name)
*/
std::optional<QDateTime> QHttpHeaders::dateTimeValueAt(qsizetype i) const
{
    verify(i);
    QDateTime dt = QNetworkHeadersPrivate::fromHttpDate(valueAt(i));
    return dt.isValid() ? std::make_optional(std::move(dt)) :
                          std::nullopt;
}

/*!
    Returns the number of header entries.
*/
qsizetype QHttpHeaders::size() const noexcept
{
    if (!d)
        return 0;
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
    d.detach();
    d->headers.reserve(size);
}

/*!
    \fn bool QHttpHeaders::isEmpty() const noexcept

    Returns \c true if the headers have size 0; otherwise returns \c false.

    \sa size()
*/

/*!
    Returns a header name corresponding to the provided \a name as a view.
*/
QByteArrayView QHttpHeaders::wellKnownHeaderName(WellKnownHeader name) noexcept
{
    return headerNames[qToUnderlying(name)];
}

/*!
    Returns the header entries as a list of (name, value) pairs.
    Header names are case-insensitive, and the returned names are lower-cased.
*/
QList<std::pair<QByteArray, QByteArray>> QHttpHeaders::toListOfPairs() const
{
    QList<std::pair<QByteArray, QByteArray>> list;
    if (isEmpty())
        return list;
    list.reserve(size());
    for (const auto & h : std::as_const(d->headers))
        list.append({h.name.asByteArray(), h.value});
    return list;
}

/*!
    Returns the header entries as a map from name to value(s).
    Header names are case-insensitive, and the returned names are lower-cased.
*/
QMultiMap<QByteArray, QByteArray> QHttpHeaders::toMultiMap() const
{
    QMultiMap<QByteArray, QByteArray> map;
    if (isEmpty())
        return map;
    for (const auto &h : std::as_const(d->headers))
        map.insert(h.name.asByteArray(), h.value);
    return map;
}

/*!
    Returns the header entries as a hash from name to value(s).
    Header names are case-insensitive, and the returned names are lower-cased.
*/
QMultiHash<QByteArray, QByteArray> QHttpHeaders::toMultiHash() const
{
    QMultiHash<QByteArray, QByteArray> hash;
    if (isEmpty())
        return hash;
    hash.reserve(size());
    for (const auto &h : std::as_const(d->headers))
        hash.insert(h.name.asByteArray(), h.value);
    return hash;
}

/*!
    Clears all header entries.

    \sa size()
*/
void QHttpHeaders::clear()
{
    if (isEmpty())
        return;
    d.detach();
    d->headers.clear();
}

QT_END_NAMESPACE
