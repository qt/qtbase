// Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdnslookup_p.h"

#include <qendian.h>
#include <qscopedpointer.h>
#include <qurl.h>
#include <qvarlengtharray.h>
#include <private/qnativesocketengine_p.h>      // for setSockAddr
#include <private/qtnetwork-config_p.h>

QT_REQUIRE_CONFIG(res_ninit);

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#if __has_include(<arpa/nameser_compat.h>)
#  include <arpa/nameser_compat.h>
#endif
#include <resolv.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(res_setservers)
// https://www.ibm.com/docs/en/i/7.3?topic=ssw_ibm_i_73/apis/ressetservers.html
// https://docs.oracle.com/cd/E86824_01/html/E54774/res-setservers-3resolv.html
static bool applyNameServer(res_state state, const QHostAddress &nameserver, quint16 port)
{
    if (!nameserver.isNull()) {
        union res_sockaddr_union u;
        setSockaddr(reinterpret_cast<sockaddr *>(&u.sin), nameserver, port);
        res_setservers(state, &u, 1);
    }
    return true;
}
#else
template <typename T> void setNsMap(T &ext, std::enable_if_t<sizeof(T::nsmap) != 0, uint16_t> v)
{
    // Set nsmap[] to indicate that nsaddrs[0] is an IPv6 address
    // See: https://sourceware.org/ml/libc-hacker/2002-05/msg00035.html
    // Unneeded since glibc 2.22 (2015), but doesn't hurt to set it
    // See: https://sourceware.org/git/?p=glibc.git;a=commit;h=2212c1420c92a33b0e0bd9a34938c9814a56c0f7
    ext.nsmap[0] = v;
}
template <typename T> void setNsMap(T &, ...)
{
    // fallback
}

template <bool Condition>
using EnableIfIPv6 = std::enable_if_t<Condition, const QHostAddress *>;

template <typename State>
bool setIpv6NameServer(State *state,
                       EnableIfIPv6<sizeof(std::declval<State>()._u._ext.nsaddrs) != 0> addr,
                       quint16 port)
{
    // glibc-like API to set IPv6 name servers
    struct sockaddr_in6 *ns = state->_u._ext.nsaddrs[0];

    // nsaddrs will be NULL if no nameserver is set in /etc/resolv.conf
    if (!ns) {
        // Memory allocated here will be free()'d in res_close() as we
        // have done res_init() above.
        ns = static_cast<struct sockaddr_in6*>(calloc(1, sizeof(struct sockaddr_in6)));
        Q_CHECK_PTR(ns);
        state->_u._ext.nsaddrs[0] = ns;
    }

    setNsMap(state->_u._ext, MAXNS + 1);
    state->_u._ext.nscount6 = 1;
    setSockaddr(ns, *addr, port);
    return true;
}

template <typename State> bool setIpv6NameServer(State *, const void *, quint16)
{
    // fallback
    return false;
}

static bool applyNameServer(res_state state, const QHostAddress &nameserver, quint16 port)
{
    if (nameserver.isNull())
        return true;

    state->nscount = 1;
    state->nsaddr_list[0].sin_family = AF_UNSPEC;
    if (nameserver.protocol() == QAbstractSocket::IPv6Protocol)
        return setIpv6NameServer(state, &nameserver, port);
    setSockaddr(&state->nsaddr_list[0], nameserver, port);
    return true;
}
#endif // !QT_CONFIG(res_setservers)

void QDnsLookupRunnable::query(QDnsLookupReply *reply)
{
    // Initialize state.
    std::remove_pointer_t<res_state> state = {};
    if (res_ninit(&state) < 0) {
        int error = errno;
        qErrnoWarning(error, "QDnsLookup: Resolver initialization failed");
        return reply->makeResolverSystemError(error);
    }
    auto guard = qScopeGuard([&] { res_nclose(&state); });

    //Check if a nameserver was set. If so, use it
    if (!applyNameServer(&state, nameserver, port)) {
        qWarning("QDnsLookup: %s", "IPv6 nameservers are currently not supported on this OS");
        return reply->setError(QDnsLookup::ResolverError,
                               QDnsLookup::tr("IPv6 nameservers are currently not supported on this OS"));
    }
#ifdef QDNSLOOKUP_DEBUG
    state.options |= RES_DEBUG;
#endif

    // Perform DNS query.
    QVarLengthArray<unsigned char, PACKETSZ> buffer(PACKETSZ);
    std::memset(buffer.data(), 0, buffer.size());
    int responseLength = res_nquery(&state, requestName, C_IN, requestType, buffer.data(), buffer.size());
    if (Q_UNLIKELY(responseLength > PACKETSZ)) {
        buffer.resize(responseLength);
        std::memset(buffer.data(), 0, buffer.size());
        responseLength = res_nquery(&state, requestName, C_IN, requestType, buffer.data(), buffer.size());
        if (Q_UNLIKELY(responseLength > buffer.size())) {
            // Ok, we give up.
            return reply->setError(QDnsLookup::ResolverError,
                                   QDnsLookup::tr("Reply was too large"));
        }
    }

    unsigned char *response = buffer.data();
    // Check the response header. Though res_nquery returns -1 as a
    // responseLength in case of error, we still can extract the
    // exact error code from the response.
    HEADER *header = (HEADER*)response;
    if (header->rcode)
        return reply->makeDnsRcodeError(header->rcode);

    // Check the reply is valid.
    if (responseLength < int(sizeof(HEADER)))
        return reply->makeInvalidReplyError();

    char host[PACKETSZ], answer[PACKETSZ];
    qptrdiff offset = sizeof(HEADER);
    int status;

    if (ntohs(header->qdcount) == 1) {
        // Skip the query host, type (2 bytes) and class (2 bytes).
        status = dn_expand(response, response + responseLength, response + offset, host, sizeof(host));
        if (status < 0) {
            reply->makeInvalidReplyError(QDnsLookup::tr("Could not expand domain name"));
            return;
        }
        if (offset + status + 4 >= responseLength)
            header->qdcount = 0xffff;   // invalid reply below
        else
            offset += status + 4;
    }
    if (ntohs(header->qdcount) > 1)
        return reply->makeInvalidReplyError();

    // Extract results.
    const int answerCount = ntohs(header->ancount);
    int answerIndex = 0;
    while ((offset < responseLength) && (answerIndex < answerCount)) {
        status = dn_expand(response, response + responseLength, response + offset, host, sizeof(host));
        if (status < 0) {
            reply->makeInvalidReplyError(QDnsLookup::tr("Could not expand domain name"));
            return;
        }
        const QString name = QUrl::fromAce(host);

        offset += status;
        if (offset + RRFIXEDSZ > responseLength) {
            // probably just a truncated reply, return what we have
            return;
        }
        const quint16 type = qFromBigEndian<quint16>(response + offset);
        const qint16 rrclass = qFromBigEndian<quint16>(response + offset + 2);
        const quint32 ttl = qFromBigEndian<quint32>(response + offset + 4);
        const quint16 size = qFromBigEndian<quint16>(response + offset + 8);
        offset += RRFIXEDSZ;
        if (offset + size > responseLength)
            return;             // truncated
        if (rrclass != C_IN)
            continue;

        if (type == QDnsLookup::A) {
            if (size != 4)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid IPv4 address record"));
            const quint32 addr = qFromBigEndian<quint32>(response + offset);
            QDnsHostAddressRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QHostAddress(addr);
            reply->hostAddressRecords.append(record);
        } else if (type == QDnsLookup::AAAA) {
            if (size != 16)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid IPv6 address record"));
            QDnsHostAddressRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QHostAddress(response + offset);
            reply->hostAddressRecords.append(record);
        } else if (type == QDnsLookup::CNAME) {
            status = dn_expand(response, response + responseLength, response + offset, answer, sizeof(answer));
            if (status < 0)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid canonical name record"));
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QUrl::fromAce(answer);
            reply->canonicalNameRecords.append(record);
        } else if (type == QDnsLookup::NS) {
            status = dn_expand(response, response + responseLength, response + offset, answer, sizeof(answer));
            if (status < 0)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid name server record"));
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QUrl::fromAce(answer);
            reply->nameServerRecords.append(record);
        } else if (type == QDnsLookup::PTR) {
            status = dn_expand(response, response + responseLength, response + offset, answer, sizeof(answer));
            if (status < 0)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid pointer record"));
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QUrl::fromAce(answer);
            reply->pointerRecords.append(record);
        } else if (type == QDnsLookup::MX) {
            const quint16 preference = qFromBigEndian<quint16>(response + offset);
            status = dn_expand(response, response + responseLength, response + offset + 2, answer, sizeof(answer));
            if (status < 0)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid mail exchange record"));
            QDnsMailExchangeRecord record;
            record.d->exchange = QUrl::fromAce(answer);
            record.d->name = name;
            record.d->preference = preference;
            record.d->timeToLive = ttl;
            reply->mailExchangeRecords.append(record);
        } else if (type == QDnsLookup::SRV) {
            const quint16 priority = qFromBigEndian<quint16>(response + offset);
            const quint16 weight = qFromBigEndian<quint16>(response + offset + 2);
            const quint16 port = qFromBigEndian<quint16>(response + offset + 4);
            status = dn_expand(response, response + responseLength, response + offset + 6, answer, sizeof(answer));
            if (status < 0)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid service record"));
            QDnsServiceRecord record;
            record.d->name = name;
            record.d->target = QUrl::fromAce(answer);
            record.d->port = port;
            record.d->priority = priority;
            record.d->timeToLive = ttl;
            record.d->weight = weight;
            reply->serviceRecords.append(record);
        } else if (type == QDnsLookup::TXT) {
            QDnsTextRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            qptrdiff txt = offset;
            while (txt < offset + size) {
                const unsigned char length = response[txt];
                txt++;
                if (txt + length > offset + size)
                    return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid text record"));
                record.d->values << QByteArrayView(response + txt, length).toByteArray();
                txt += length;
            }
            reply->textRecords.append(record);
        }
        offset += size;
        answerIndex++;
    }
}

QT_END_NAMESPACE
