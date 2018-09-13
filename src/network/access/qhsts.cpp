/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qhsts_p.h"

#include "QtCore/private/qipaddress_p.h"
#include "QtCore/qvector.h"
#include "QtCore/qlist.h"

#if QT_CONFIG(settings)
#include "qhstsstore_p.h"
#endif // QT_CONFIG(settings)

QT_BEGIN_NAMESPACE

static bool is_valid_domain_name(const QString &host)
{
    if (!host.size())
        return false;

    // RFC6797 8.1.1
    // If the substring matching the host production from the Request-URI
    // (of the message to which the host responded) syntactically matches
    //the IP-literal or IPv4address productions from Section 3.2.2 of
    //[RFC3986], then the UA MUST NOT note this host as a Known HSTS Host.
    using namespace QIPAddressUtils;

    IPv4Address ipv4Addr = {};
    if (parseIp4(ipv4Addr, host.constBegin(), host.constEnd()))
        return false;

    IPv6Address ipv6Addr = {};
    // Unlike parseIp4, parseIp6 returns nullptr if it managed to parse IPv6
    // address successfully.
    if (!parseIp6(ipv6Addr, host.constBegin(), host.constEnd()))
        return false;

    // TODO: for now we do not test IPvFuture address, it must be addressed
    // by introducing parseIpFuture (actually, there is an implementation
    // in QUrl that can be adopted/modified/moved to QIPAddressUtils).
    return true;
}

void QHstsCache::updateFromHeaders(const QList<QPair<QByteArray, QByteArray>> &headers,
                                   const QUrl &url)
{
    if (!url.isValid())
        return;

    QHstsHeaderParser parser;
    if (parser.parse(headers)) {
        updateKnownHost(url.host(), parser.expirationDate(), parser.includeSubDomains());
#if QT_CONFIG(settings)
        if (hstsStore)
            hstsStore->synchronize();
#endif // QT_CONFIG(settings)
    }
}

void QHstsCache::updateFromPolicies(const QVector<QHstsPolicy> &policies)
{
    for (const auto &policy : policies)
        updateKnownHost(policy.host(), policy.expiry(), policy.includesSubDomains());

#if QT_CONFIG(settings)
    if (hstsStore && policies.size()) {
        // These policies are coming either from store or from QNAM's setter
        // function. As a result we can notice expired or new policies, time
        // to sync ...
        hstsStore->synchronize();
    }
#endif // QT_CONFIG(settings)
}

void QHstsCache::updateKnownHost(const QUrl &url, const QDateTime &expires,
                                 bool includeSubDomains)
{
    if (!url.isValid())
        return;

    updateKnownHost(url.host(), expires, includeSubDomains);
#if QT_CONFIG(settings)
    if (hstsStore)
        hstsStore->synchronize();
#endif // QT_CONFIG(settings)
}

void QHstsCache::updateKnownHost(const QString &host, const QDateTime &expires,
                                 bool includeSubDomains)
{
    if (!is_valid_domain_name(host))
        return;

    // HSTS is a per-host policy, regardless of protocol, port or any of the other
    // details in an URL; so we only want the host part.  QUrl::host handles
    // IDNA 2003 (RFC3490) for us, as required by HSTS (RFC6797, section 10).
    const HostName hostName(host);
    const auto pos = knownHosts.find(hostName);
    QHstsPolicy::PolicyFlags flags;
    if (includeSubDomains)
        flags = QHstsPolicy::IncludeSubDomains;

    const QHstsPolicy newPolicy(expires, flags, hostName.name);
    if (pos == knownHosts.end()) {
        // A new, previously unknown host.
        if (newPolicy.isExpired()) {
            // Nothing to do at all - we did not know this host previously,
            // we do not have to - since its policy expired.
            return;
        }

        knownHosts.insert(pos, {hostName, newPolicy});
#if QT_CONFIG(settings)
        if (hstsStore)
            hstsStore->addToObserved(newPolicy);
#endif // QT_CONFIG(settings)
        return;
    }

    if (newPolicy.isExpired())
        knownHosts.erase(pos);
    else  if (pos->second != newPolicy)
        pos->second = std::move(newPolicy);
    else
        return;

#if QT_CONFIG(settings)
    if (hstsStore)
        hstsStore->addToObserved(newPolicy);
#endif // QT_CONFIG(settings)
}

bool QHstsCache::isKnownHost(const QUrl &url) const
{
    if (!url.isValid() || !is_valid_domain_name(url.host()))
        return false;

    /*
        RFC6797, 8.2.  Known HSTS Host Domain Name Matching

        * Superdomain Match
          If a label-for-label match between an entire Known HSTS Host's
          domain name and a right-hand portion of the given domain name
          is found, then this Known HSTS Host's domain name is a
          superdomain match for the given domain name.  There could be
          multiple superdomain matches for a given domain name.
        * Congruent Match
          If a label-for-label match between a Known HSTS Host's domain
          name and the given domain name is found -- i.e., there are no
          further labels to compare -- then the given domain name
          congruently matches this Known HSTS Host.

        We start from the congruent match, and then chop labels and dots and
        proceed with superdomain match. While RFC6797 recommends to start from
        superdomain, the result is the same - some valid policy will make a host
        known.
    */

    bool superDomainMatch = false;
    const QString hostNameAsString(url.host());
    HostName nameToTest(static_cast<QStringRef>(&hostNameAsString));
    while (nameToTest.fragment.size()) {
        auto const pos = knownHosts.find(nameToTest);
        if (pos != knownHosts.end()) {
            if (pos->second.isExpired()) {
                knownHosts.erase(pos);
#if QT_CONFIG(settings)
                if (hstsStore) {
                    // Inform our store that this policy has expired.
                    hstsStore->addToObserved(pos->second);
                }
#endif // QT_CONFIG(settings)
            } else if (!superDomainMatch || pos->second.includesSubDomains()) {
                return true;
            }
        }

        const int dot = nameToTest.fragment.indexOf(QLatin1Char('.'));
        if (dot == -1)
            break;

        nameToTest.fragment = nameToTest.fragment.mid(dot + 1);
        superDomainMatch = true;
    }

    return false;
}

void QHstsCache::clear()
{
    knownHosts.clear();
}

QVector<QHstsPolicy> QHstsCache::policies() const
{
    QVector<QHstsPolicy> values;
    values.reserve(int(knownHosts.size()));
    for (const auto &host : knownHosts)
        values << host.second;
    return values;
}

#if QT_CONFIG(settings)
void QHstsCache::setStore(QHstsStore *store)
{
    // Caller retains ownership of store, which must outlive this cache.
    if (store != hstsStore) {
        hstsStore = store;

        if (!hstsStore)
            return;

        // First we augment our store with the policies we already know about
        // (and thus the cached policy takes priority over whatever policy we
        // had in the store for the same host, if any).
        if (knownHosts.size()) {
            const QVector<QHstsPolicy> observed(policies());
            for (const auto &policy : observed)
                hstsStore->addToObserved(policy);
            hstsStore->synchronize();
        }

        // Now we update the cache with anything we have not observed yet, but
        // the store knows about (well, it can happen we synchronize again as a
        // result if some policies managed to expire or if we add a new one
        // from the store to cache):
        const QVector<QHstsPolicy> restored(store->readPolicies());
        updateFromPolicies(restored);
    }
}
#endif // QT_CONFIG(settings)

// The parser is quite simple: 'nextToken' knowns exactly what kind of tokens
// are valid and it will return false if something else was found; then
// we immediately stop parsing. 'parseDirective' knows how these tokens can
// be combined into a valid directive and if some weird combination of
// valid tokens is found - we immediately stop.
// And finally we call parseDirective again and again until some error found or
// we have no more bytes in the header.

// The following isXXX functions are based on RFC2616, 2.2 Basic Rules.

static bool isCHAR(int c)
{
    // CHAR           = <any US-ASCII character (octets 0 - 127)>
    return c >= 0 && c <= 127;
}

static bool isCTL(int c)
{
    // CTL            = <any US-ASCII control character
    //                  (octets 0 - 31) and DEL (127)>
    return (c >= 0 && c <= 31) || c == 127;
}


static bool isLWS(int c)
{
    // LWS            = [CRLF] 1*( SP | HT )
    //
    // CRLF           = CR LF
    // CR             = <US-ASCII CR, carriage return (13)>
    // LF             = <US-ASCII LF, linefeed (10)>
    // SP             = <US-ASCII SP, space (32)>
    // HT             = <US-ASCII HT, horizontal-tab (9)>
    //
    // CRLF is handled by the time we parse a header (they were replaced with
    // spaces). We only have to deal with remaining SP|HT
    return c == ' '  || c == '\t';
}

static bool isTEXT(char c)
{
    // TEXT           = <any OCTET except CTLs,
    //                  but including LWS>
    return !isCTL(c) || isLWS(c);
}

static bool isSeparator(char c)
{
    // separators     = "(" | ")" | "<" | ">" | "@"
    //                      | "," | ";" | ":" | "\" | <">
    //                      | "/" | "[" | "]" | "?" | "="
    //                      | "{" | "}" | SP | HT
    static const char separators[] = "()<>@,;:\\\"/[]?={}";
    static const char *end = separators + sizeof separators - 1;
    return isLWS(c) || std::find(separators, end, c) != end;
}

static QByteArray unescapeMaxAge(const QByteArray &value)
{
    if (value.size() < 2 || value[0] != '"')
        return value;

    Q_ASSERT(value[value.size() - 1] == '"');
    return value.mid(1, value.size() - 2);
}

static bool isTOKEN(char c)
{
    // token          = 1*<any CHAR except CTLs or separators>
    return isCHAR(c) && !isCTL(c) && !isSeparator(c);
}

/*

RFC6797, 6.1 Strict-Transport-Security HTTP Response Header Field.
Syntax:

Strict-Tranposrt-Security = "Strict-Transport-Security" ":"
                              [ directive ] *( ";" [ directive ] )

directive = directive-name [ "=" directive-value ]
directive-name = token
directive-value = token | quoted-string

RFC 2616, 2.2 Basic Rules.

token          = 1*<any CHAR except CTLs or separators>
quoted-string  = ( <"> *(qdtext | quoted-pair ) <"> )


qdtext         = <any TEXT except <">>
quoted-pair    = "\" CHAR

*/

bool QHstsHeaderParser::parse(const QList<QPair<QByteArray, QByteArray>> &headers)
{
    for (const auto &h : headers) {
        // We use '==' since header name was already 'trimmed' for us:
        if (h.first == "Strict-Transport-Security") {
            header = h.second;
            // RFC6797, 8.1:
            //
            //  The UA MUST ignore any STS header fields not conforming to the
            // grammar specified in Section 6.1 ("Strict-Transport-Security HTTP
            // Response Header Field").
            //
            // If a UA receives more than one STS header field in an HTTP
            // response message over secure transport, then the UA MUST process
            // only the first such header field.
            //
            // We read this as: ignore all invalid headers and take the first valid:
            if (parseSTSHeader() && maxAgeFound) {
                expiry = QDateTime::currentDateTimeUtc().addSecs(maxAge);
                return true;
            }
        }
    }

    // In case it was set by a syntactically correct header (but without
    // REQUIRED max-age directive):
    subDomainsFound = false;

    return false;
}

bool QHstsHeaderParser::parseSTSHeader()
{
    expiry = QDateTime();
    maxAgeFound = false;
    subDomainsFound = false;
    maxAge = 0;
    tokenPos = 0;
    token.clear();

    while (tokenPos < header.size()) {
        if (!parseDirective())
            return false;

        if (token.size() && token != ";") {
            // After a directive we can only have a ";" or no more tokens.
            // Invalid syntax.
            return false;
        }
    }

    return true;
}

bool QHstsHeaderParser::parseDirective()
{
    // RFC 6797, 6.1:
    //
    // directive = directive-name [ "=" directive-value ]
    // directive-name = token
    // directive-value = token | quoted-string


    // RFC 2616, 2.2:
    //
    // token          = 1*<any CHAR except CTLs or separators>

    if (!nextToken())
        return false;

    if (!token.size()) // No more data, but no error.
        return true;

    if (token == ";") // That's a weird grammar, but that's what it is.
        return true;

    if (!isTOKEN(token[0])) // Not a valid directive-name.
        return false;

    const QByteArray directiveName = token;
    // 2. Try to read "=" or ";".
    if (!nextToken())
        return false;

    QByteArray directiveValue;
    if (token == ";") // No directive-value
        return processDirective(directiveName, directiveValue);

    if (token == "=") {
        // We expect a directive-value now:
        if (!nextToken() || !token.size())
            return false;
        directiveValue = token;
    } else if (token.size()) {
        // Invalid syntax:
        return false;
    }

    if (!processDirective(directiveName, directiveValue))
        return false;

    // Read either ";", or 'end of header', or some invalid token.
    return nextToken();
}

bool QHstsHeaderParser::processDirective(const QByteArray &name, const QByteArray &value)
{
    Q_ASSERT(name.size());
    // RFC6797 6.1/3 Directive names are case-insensitive
    const auto lcName = name.toLower();
    if (lcName == "max-age") {
        // RFC 6797, 6.1.1
        // The syntax of the max-age directive's REQUIRED value (after
        // quoted-string unescaping, if necessary) is defined as:
        //
        // max-age-value = delta-seconds
        if (maxAgeFound) {
            // RFC 6797, 6.1/2:
            // All directives MUST appear only once in an STS header field.
            return false;
        }

        const QByteArray unescapedValue = unescapeMaxAge(value);
        if (!unescapedValue.size())
            return false;

        bool ok = false;
        const qint64 age = unescapedValue.toLongLong(&ok);
        if (!ok || age < 0)
            return false;

        maxAge = age;
        maxAgeFound = true;
    } else if (lcName == "includesubdomains") {
        // RFC 6797, 6.1.2.  The includeSubDomains Directive.
        // The OPTIONAL "includeSubDomains" directive is a valueless directive.

        if (subDomainsFound) {
            // RFC 6797, 6.1/2:
            // All directives MUST appear only once in an STS header field.
            return false;
        }

        subDomainsFound = true;
    } // else we do nothing, skip unknown directives (RFC 6797, 6.1/5)

    return true;
}

bool QHstsHeaderParser::nextToken()
{
    // Returns true if we found a valid token or we have no more data (token is
    // empty then).

    token.clear();

    // Fortunately enough, by this point qhttpnetworkreply already got rid of
    // [CRLF] parts, but we can have 1*(SP|HT) yet.
    while (tokenPos < header.size() && isLWS(header[tokenPos]))
        ++tokenPos;

    if (tokenPos == header.size())
        return true;

    const char ch = header[tokenPos];
    if (ch == ';' || ch == '=') {
        token.append(ch);
        ++tokenPos;
        return true;
    }

    // RFC 2616, 2.2.
    //
    // quoted-string  = ( <"> *(qdtext | quoted-pair ) <"> )
    // qdtext         = <any TEXT except <">>
    if (ch == '"') {
        int last = tokenPos + 1;
        while (last < header.size()) {
            if (header[last] == '"') {
                // The end of a quoted-string.
                break;
            } else if (header[last] == '\\') {
                // quoted-pair    = "\" CHAR
                if (last + 1 < header.size() && isCHAR(header[last + 1]))
                    last += 2;
                else
                    return false;
            } else {
                if (!isTEXT(header[last]))
                    return false;
                ++last;
            }
        }

        if (last >= header.size()) // no closing '"':
            return false;

        token = header.mid(tokenPos, last - tokenPos + 1);
        tokenPos = last + 1;
        return true;
    }

    // RFC 2616, 2.2:
    //
    // token          = 1*<any CHAR except CTLs or separators>
    if (!isTOKEN(ch))
        return false;

    int last = tokenPos + 1;
    while (last < header.size() && isTOKEN(header[last]))
        ++last;

    token = header.mid(tokenPos, last - tokenPos);
    tokenPos = last;

    return true;
}

QT_END_NAMESPACE
