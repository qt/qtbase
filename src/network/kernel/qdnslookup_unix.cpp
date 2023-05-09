// Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdnslookup_p.h"

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

static const char *applyNameServer(res_state state, const QHostAddress &nameserver, quint16 port)
{
    if (nameserver.isNull())
        return nullptr;

    state->nscount = 1;
    state->nsaddr_list[0].sin_family = AF_UNSPEC;
    if (nameserver.protocol() == QAbstractSocket::IPv6Protocol) {
        if (!setIpv6NameServer(state, &nameserver, port))
            return QDnsLookupPrivate::msgNoIpV6NameServerAdresses;
    } else {
        setSockaddr(&state->nsaddr_list[0], nameserver, port);
    }
    return nullptr;
}

void QDnsLookupRunnable::query(const int requestType, const QByteArray &requestName, const QHostAddress &nameserver, QDnsLookupReply *reply)
{
    // Initialize state.
    std::remove_pointer_t<res_state> state = {};
    if (res_ninit(&state) < 0) {
        reply->error = QDnsLookup::ResolverError;
        reply->errorString = tr("Resolver initialization failed");
        return;
    }
    auto guard = qScopeGuard([&] { res_nclose(&state); });

    //Check if a nameserver was set. If so, use it
    if (const char *msg = applyNameServer(&state, nameserver, DnsPort)) {
        qWarning("%s", msg);
        reply->error = QDnsLookup::ResolverError;
        reply->errorString = tr(msg);
        return;
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
            reply->error = QDnsLookup::ResolverError;
            reply->errorString.clear(); // We cannot be more specific, alas.
            return;
        }
    }

    unsigned char *response = buffer.data();
    // Check the response header. Though res_nquery returns -1 as a
    // responseLength in case of error, we still can extract the
    // exact error code from the response.
    HEADER *header = (HEADER*)response;
    switch (header->rcode) {
    case NOERROR:
        break;
    case FORMERR:
        reply->error = QDnsLookup::InvalidRequestError;
        reply->errorString = tr("Server could not process query");
        return;
    case SERVFAIL:
    case NOTIMP:
        reply->error = QDnsLookup::ServerFailureError;
        reply->errorString = tr("Server failure");
        return;
    case NXDOMAIN:
        reply->error = QDnsLookup::NotFoundError;
        reply->errorString = tr("Non existent domain");
        return;
    case REFUSED:
        reply->error = QDnsLookup::ServerRefusedError;
        reply->errorString = tr("Server refused to answer");
        return;
    default:
        reply->error = QDnsLookup::InvalidReplyError;
        reply->errorString = tr("Invalid reply received");
        return;
    }

    // Check the reply is valid.
    if (responseLength < int(sizeof(HEADER))) {
        reply->error = QDnsLookup::InvalidReplyError;
        reply->errorString = tr("Invalid reply received");
        return;
    }

    char host[PACKETSZ], answer[PACKETSZ];
    unsigned char *p = response + sizeof(HEADER);
    int status;

    if (ntohs(header->qdcount) == 1) {
        // Skip the query host, type (2 bytes) and class (2 bytes).
        status = dn_expand(response, response + responseLength, p, host, sizeof(host));
        if (status < 0) {
            reply->error = QDnsLookup::InvalidReplyError;
            reply->errorString = tr("Could not expand domain name");
            return;
        }
        if ((p - response) + status + 4 >= responseLength)
            header->qdcount = 0xffff;   // invalid reply below
        else
            p += status + 4;
    }
    if (ntohs(header->qdcount) > 1) {
        reply->error = QDnsLookup::InvalidReplyError;
        reply->errorString = tr("Invalid reply received");
        return;
    }

    // Extract results.
    const int answerCount = ntohs(header->ancount);
    int answerIndex = 0;
    while ((p < response + responseLength) && (answerIndex < answerCount)) {
        status = dn_expand(response, response + responseLength, p, host, sizeof(host));
        if (status < 0) {
            reply->error = QDnsLookup::InvalidReplyError;
            reply->errorString = tr("Could not expand domain name");
            return;
        }
        const QString name = QUrl::fromAce(host);

        p += status;

        if ((p - response) + 10 > responseLength) {
            // probably just a truncated reply, return what we have
            return;
        }
        const quint16 type = (p[0] << 8) | p[1];
        p += 2; // RR type
        p += 2; // RR class
        const quint32 ttl = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
        p += 4;
        const quint16 size = (p[0] << 8) | p[1];
        p += 2;
        if ((p - response) + size > responseLength)
            return;             // truncated

        if (type == QDnsLookup::A) {
            if (size != 4) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid IPv4 address record");
                return;
            }
            const quint32 addr = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
            QDnsHostAddressRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QHostAddress(addr);
            reply->hostAddressRecords.append(record);
        } else if (type == QDnsLookup::AAAA) {
            if (size != 16) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid IPv6 address record");
                return;
            }
            QDnsHostAddressRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QHostAddress(p);
            reply->hostAddressRecords.append(record);
        } else if (type == QDnsLookup::CNAME) {
            status = dn_expand(response, response + responseLength, p, answer, sizeof(answer));
            if (status < 0) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid canonical name record");
                return;
            }
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QUrl::fromAce(answer);
            reply->canonicalNameRecords.append(record);
        } else if (type == QDnsLookup::NS) {
            status = dn_expand(response, response + responseLength, p, answer, sizeof(answer));
            if (status < 0) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid name server record");
                return;
            }
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QUrl::fromAce(answer);
            reply->nameServerRecords.append(record);
        } else if (type == QDnsLookup::PTR) {
            status = dn_expand(response, response + responseLength, p, answer, sizeof(answer));
            if (status < 0) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid pointer record");
                return;
            }
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            record.d->value = QUrl::fromAce(answer);
            reply->pointerRecords.append(record);
        } else if (type == QDnsLookup::MX) {
            const quint16 preference = (p[0] << 8) | p[1];
            status = dn_expand(response, response + responseLength, p + 2, answer, sizeof(answer));
            if (status < 0) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid mail exchange record");
                return;
            }
            QDnsMailExchangeRecord record;
            record.d->exchange = QUrl::fromAce(answer);
            record.d->name = name;
            record.d->preference = preference;
            record.d->timeToLive = ttl;
            reply->mailExchangeRecords.append(record);
        } else if (type == QDnsLookup::SRV) {
            const quint16 priority = (p[0] << 8) | p[1];
            const quint16 weight = (p[2] << 8) | p[3];
            const quint16 port = (p[4] << 8) | p[5];
            status = dn_expand(response, response + responseLength, p + 6, answer, sizeof(answer));
            if (status < 0) {
                reply->error = QDnsLookup::InvalidReplyError;
                reply->errorString = tr("Invalid service record");
                return;
            }
            QDnsServiceRecord record;
            record.d->name = name;
            record.d->target = QUrl::fromAce(answer);
            record.d->port = port;
            record.d->priority = priority;
            record.d->timeToLive = ttl;
            record.d->weight = weight;
            reply->serviceRecords.append(record);
        } else if (type == QDnsLookup::TXT) {
            unsigned char *txt = p;
            QDnsTextRecord record;
            record.d->name = name;
            record.d->timeToLive = ttl;
            while (txt < p + size) {
                const unsigned char length = *txt;
                txt++;
                if (txt + length > p + size) {
                    reply->error = QDnsLookup::InvalidReplyError;
                    reply->errorString = tr("Invalid text record");
                    return;
                }
                record.d->values << QByteArray((char*)txt, length);
                txt += length;
            }
            reply->textRecords.append(record);
        }
        p += size;
        answerIndex++;
    }
}

QT_END_NAMESPACE
