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

QT_REQUIRE_CONFIG(libresolv);

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#if __has_include(<arpa/nameser_compat.h>)
#  include <arpa/nameser_compat.h>
#endif
#include <errno.h>
#include <resolv.h>

#include <array>

#ifndef T_OPT
// the older arpa/nameser_compat.h wasn't updated between 1999 and 2016 in glibc
#  define T_OPT     ns_t_opt
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// minimum IPv6 MTU (1280) minus the IPv6 (40) and UDP headers (8)
static constexpr qsizetype ReplyBufferSize = 1280 - 40 - 8;

// https://www.rfc-editor.org/rfc/rfc6891
static constexpr unsigned char Edns0Record[] = {
    0x00,                                           // root label
    T_OPT >> 8, T_OPT & 0xff,                       // type OPT
    ReplyBufferSize >> 8, ReplyBufferSize & 0xff,   // payload size
    NOERROR,                                        // extended rcode
    0,                                              // version
    0x00, 0x00,                                     // flags
    0x00, 0x00,                                     // option length
};

// maximum length of a EDNS0 query with a 255-character domain (rounded up to 16)
static constexpr qsizetype QueryBufferSize =
        HFIXEDSZ + QFIXEDSZ + MAXCDNAME + 1 + sizeof(Edns0Record);
using QueryBuffer = std::array<unsigned char, (QueryBufferSize + 15) / 16 * 16>;

namespace {
struct QDnsCachedName
{
    QString name;
    int code = 0;
    QDnsCachedName(const QString &name, int code) : name(name), code(code) {}
};
}
Q_DECLARE_TYPEINFO(QDnsCachedName, Q_RELOCATABLE_TYPE);
using Cache = QList<QDnsCachedName>;    // QHash or QMap are overkill

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

static int
prepareQueryBuffer(res_state state, QueryBuffer &buffer, const char *label, ns_rcode type)
{
    // Create header and our query
    int queryLength = res_nmkquery(state, QUERY, label, C_IN, type, nullptr, 0, nullptr,
                                   buffer.data(), buffer.size());
    Q_ASSERT(queryLength < int(buffer.size()));
    if (Q_UNLIKELY(queryLength < 0))
        return queryLength;

    // Append EDNS0 record and set the number of additional RRs to 1
    Q_ASSERT(queryLength + sizeof(Edns0Record) < buffer.size());
    std::copy_n(std::begin(Edns0Record), sizeof(Edns0Record), buffer.begin() + queryLength);
    reinterpret_cast<HEADER *>(buffer.data())->arcount = qToBigEndian<quint16>(1);

    return queryLength + sizeof(Edns0Record);
}

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
    if (!applyNameServer(&state, nameserver, port))
        return reply->setError(QDnsLookup::ResolverError,
                               QDnsLookup::tr("IPv6 nameservers are currently not supported on this OS"));
#ifdef QDNSLOOKUP_DEBUG
    state.options |= RES_DEBUG;
#endif

    // Prepare the DNS query.
    QueryBuffer qbuffer;
    int queryLength = prepareQueryBuffer(&state, qbuffer, requestName, ns_rcode(requestType));
    if (Q_UNLIKELY(queryLength < 0))
        return reply->makeResolverSystemError();

    // Perform DNS query.
    QVarLengthArray<unsigned char, ReplyBufferSize> buffer(ReplyBufferSize);
    auto attemptToSend = [&]() {
        std::memset(buffer.data(), 0, HFIXEDSZ);        // the header is enough
        int responseLength = res_nsend(&state, qbuffer.data(), queryLength, buffer.data(), buffer.size());
        if (responseLength < 0) {
            // network error of some sort
            if (errno == ETIMEDOUT)
                reply->makeTimeoutError();
            else
                reply->makeResolverSystemError();
        }
        return responseLength;
    };

    // strictly use UDP, we'll deal with truncated replies ourselves
    state.options |= RES_IGNTC;
    int responseLength = attemptToSend();
    if (responseLength < 0)
        return;

    // check if we need to use the virtual circuit (TCP)
    auto header = reinterpret_cast<HEADER *>(buffer.data());
    if (header->rcode == NOERROR && header->tc) {
        // yes, increase our buffer size
        buffer.resize(std::numeric_limits<quint16>::max());
        header = reinterpret_cast<HEADER *>(buffer.data());

        // remove the EDNS record in the query
        reinterpret_cast<HEADER *>(qbuffer.data())->arcount = 0;
        queryLength -= sizeof(Edns0Record);

        // send using the virtual circuit
        state.options |= RES_USEVC;
        responseLength = attemptToSend();
        if (Q_UNLIKELY(responseLength > buffer.size())) {
            // Ok, we give up.
            return reply->setError(QDnsLookup::ResolverError,
                                   QDnsLookup::tr("Reply was too large"));
        }
    }
    if (responseLength < 0)
        return;

    // Check the reply is valid.
    if (responseLength < int(sizeof(HEADER)))
        return reply->makeInvalidReplyError();

    // Parse the reply.
    if (header->rcode)
        return reply->makeDnsRcodeError(header->rcode);

    qptrdiff offset = sizeof(HEADER);
    unsigned char *response = buffer.data();
    int status;

    auto expandHost = [&, cache = Cache{}](qptrdiff offset) mutable {
        if (uchar n = response[offset]; n & NS_CMPRSFLGS) {
            // compressed name, see if we already have it cached
            if (offset + 1 < responseLength) {
                int id = ((n & ~NS_CMPRSFLGS) << 8) | response[offset + 1];
                auto it = std::find_if(cache.constBegin(), cache.constEnd(),
                                  [id](const QDnsCachedName &n) { return n.code == id; });
                if (it != cache.constEnd()) {
                    status = 2;
                    return it->name;
                }
            }
        }

        // uncached, expand it
        char host[MAXCDNAME + 1];
        status = dn_expand(response, response + responseLength, response + offset,
                           host, sizeof(host));
        if (status >= 0)
            return cache.emplaceBack(decodeLabel(QLatin1StringView(host)), offset).name;

        // failed
        reply->makeInvalidReplyError(QDnsLookup::tr("Could not expand domain name"));
        return QString();
    };

    if (ntohs(header->qdcount) == 1) {
        // Skip the query host, type (2 bytes) and class (2 bytes).
        expandHost(offset);
        if (status < 0)
            return;
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
        const QString name = expandHost(offset);
        if (status < 0)
            return;

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
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = expandHost(offset);
            if (status < 0)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid canonical name record"));
            reply->canonicalNameRecords.append(record);
        } else if (type == QDnsLookup::NS) {
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = expandHost(offset);
            if (status < 0)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid name server record"));
            reply->nameServerRecords.append(record);
        } else if (type == QDnsLookup::PTR) {
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = expandHost(offset);
            if (status < 0)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid pointer record"));
            reply->pointerRecords.append(record);
        } else if (type == QDnsLookup::MX) {
            const quint16 preference = qFromBigEndian<quint16>(response + offset);
            QDnsMailExchangeRecord record;
            record.d->exchange = expandHost(offset + 2);
            record.d->name = name;
            record.d->preference = preference;
            record.d->timeToLive = ttl;
            if (status < 0)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid mail exchange record"));
            reply->mailExchangeRecords.append(record);
        } else if (type == QDnsLookup::SRV) {
            const quint16 priority = qFromBigEndian<quint16>(response + offset);
            const quint16 weight = qFromBigEndian<quint16>(response + offset + 2);
            const quint16 port = qFromBigEndian<quint16>(response + offset + 4);
            QDnsServiceRecord record;
            record.d->name = name;
            record.d->target = expandHost(offset + 6);
            record.d->port = port;
            record.d->priority = priority;
            record.d->timeToLive = ttl;
            record.d->weight = weight;
            if (status < 0)
                return reply->makeInvalidReplyError(QDnsLookup::tr("Invalid service record"));
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
