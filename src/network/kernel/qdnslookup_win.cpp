// Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <winsock2.h>
#include "qdnslookup_p.h"

#include <qurl.h>
#include <private/qnativesocketengine_p.h>
#include <private/qsystemerror_p.h>

#include <qt_windows.h>
#include <windns.h>
#include <memory.h>

#ifndef DNS_ADDR_MAX_SOCKADDR_LENGTH
// MinGW headers are missing almost all of this
typedef struct Qt_DnsAddr {
  CHAR  MaxSa[32];
  DWORD DnsAddrUserDword[8];
} DNS_ADDR, *PDNS_ADDR;
typedef struct Qt_DnsAddrArray {
  DWORD    MaxCount;
  DWORD    AddrCount;
  DWORD    Tag;
  WORD     Family;
  WORD     WordReserved;
  DWORD    Flags;
  DWORD    MatchFlag;
  DWORD    Reserved1;
  DWORD    Reserved2;
  DNS_ADDR AddrArray[];
} DNS_ADDR_ARRAY, *PDNS_ADDR_ARRAY;
# ifndef DNS_QUERY_RESULTS_VERSION1
typedef struct Qt_DNS_QUERY_RESULT {
  ULONG Version;
  DNS_STATUS QueryStatus;
  ULONG64 QueryOptions;
  PDNS_RECORD pQueryRecords;
  PVOID Reserved;
} DNS_QUERY_RESULT, *PDNS_QUERY_RESULT;
typedef VOID WINAPI DNS_QUERY_COMPLETION_ROUTINE(PVOID pQueryContext,PDNS_QUERY_RESULT pQueryResults);
typedef DNS_QUERY_COMPLETION_ROUTINE *PDNS_QUERY_COMPLETION_ROUTINE;
# endif
typedef struct Qt_DNS_QUERY_REQUEST {
  ULONG                         Version;
  PCWSTR                        QueryName;
  WORD                          QueryType;
  ULONG64                       QueryOptions;
  PDNS_ADDR_ARRAY               pDnsServerList;
  ULONG                         InterfaceIndex;
  PDNS_QUERY_COMPLETION_ROUTINE pQueryCompletionCallback;
  PVOID                         pQueryContext;
} DNS_QUERY_REQUEST, *PDNS_QUERY_REQUEST;

typedef void *PDNS_QUERY_CANCEL;    // not really, but we don't need it
extern "C" {
DNS_STATUS WINAPI DnsQueryEx(PDNS_QUERY_REQUEST pQueryRequest,
        PDNS_QUERY_RESULT   pQueryResults,
        PDNS_QUERY_CANCEL   pCancelHandle);
}
#endif

QT_BEGIN_NAMESPACE

void QDnsLookupRunnable::query(QDnsLookupReply *reply)
{
    // Perform DNS query.
    alignas(DNS_ADDR_ARRAY) uchar dnsAddresses[sizeof(DNS_ADDR_ARRAY) + sizeof(DNS_ADDR)];
    DNS_QUERY_REQUEST request = {};
    request.Version = 1;
    request.QueryName = reinterpret_cast<const wchar_t *>(requestName.constData());
    request.QueryType = requestType;
    request.QueryOptions = DNS_QUERY_STANDARD | DNS_QUERY_TREAT_AS_FQDN;

    if (!nameserver.isNull()) {
        memset(dnsAddresses, 0, sizeof(dnsAddresses));
        request.pDnsServerList = new (dnsAddresses) DNS_ADDR_ARRAY;
        auto addr = new (request.pDnsServerList->AddrArray) DNS_ADDR[1];
        auto sa = new (addr[0].MaxSa) sockaddr;
        request.pDnsServerList->MaxCount = sizeof(dnsAddresses);
        request.pDnsServerList->AddrCount = 1;
        // ### setting port 53 seems to cause some systems to fail
        setSockaddr(sa, nameserver, port == DnsPort ? 0 : port);
        request.pDnsServerList->Family = sa->sa_family;
    }

    DNS_QUERY_RESULT results = {};
    results.Version = 1;
    const DNS_STATUS status = DnsQueryEx(&request, &results, nullptr);
    if (status >= DNS_ERROR_RCODE_FORMAT_ERROR && status <= DNS_ERROR_RCODE_LAST)
        return reply->makeDnsRcodeError(status - DNS_ERROR_RCODE_FORMAT_ERROR + 1);
    else if (status == ERROR_TIMEOUT)
        return reply->makeTimeoutError();
    else if (status != ERROR_SUCCESS)
        return reply->makeResolverSystemError(status);

    QStringView lastEncodedName;
    QString cachedDecodedName;
    auto extractAndCacheHost = [&](QStringView name) -> const QString & {
        lastEncodedName = name;
        cachedDecodedName = decodeLabel(name);
        return cachedDecodedName;
    };
    auto extractMaybeCachedHost = [&](QStringView name) -> const QString & {
        return lastEncodedName == name ? cachedDecodedName : extractAndCacheHost(name);
    };

    // Extract results.
    for (PDNS_RECORD ptr = results.pQueryRecords; ptr != NULL; ptr = ptr->pNext) {
        // warning: always assign name to the record before calling extractXxxHost() again
        const QString &name = extractMaybeCachedHost(ptr->pName);
        if (ptr->wType == QDnsLookup::A) {
            QDnsHostAddressRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = QHostAddress(ntohl(ptr->Data.A.IpAddress));
            reply->hostAddressRecords.append(record);
        } else if (ptr->wType == QDnsLookup::AAAA) {
            Q_IPV6ADDR addr;
            memcpy(&addr, &ptr->Data.AAAA.Ip6Address, sizeof(Q_IPV6ADDR));

            QDnsHostAddressRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = QHostAddress(addr);
            reply->hostAddressRecords.append(record);
        } else if (ptr->wType == QDnsLookup::CNAME) {
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = extractAndCacheHost(ptr->Data.Cname.pNameHost);
            reply->canonicalNameRecords.append(record);
        } else if (ptr->wType == QDnsLookup::MX) {
            QDnsMailExchangeRecord record;
            record.d->name = name;
            record.d->exchange = decodeLabel(QStringView(ptr->Data.Mx.pNameExchange));
            record.d->preference = ptr->Data.Mx.wPreference;
            record.d->timeToLive = ptr->dwTtl;
            reply->mailExchangeRecords.append(record);
        } else if (ptr->wType == QDnsLookup::NS) {
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = decodeLabel(QStringView(ptr->Data.Ns.pNameHost));
            reply->nameServerRecords.append(record);
        } else if (ptr->wType == QDnsLookup::PTR) {
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = decodeLabel(QStringView(ptr->Data.Ptr.pNameHost));
            reply->pointerRecords.append(record);
        } else if (ptr->wType == QDnsLookup::SRV) {
            QDnsServiceRecord record;
            record.d->name = name;
            record.d->target = decodeLabel(QStringView(ptr->Data.Srv.pNameTarget));
            record.d->port = ptr->Data.Srv.wPort;
            record.d->priority = ptr->Data.Srv.wPriority;
            record.d->timeToLive = ptr->dwTtl;
            record.d->weight = ptr->Data.Srv.wWeight;
            reply->serviceRecords.append(record);
        } else if (ptr->wType == QDnsLookup::TXT) {
            QDnsTextRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            for (unsigned int i = 0; i < ptr->Data.Txt.dwStringCount; ++i) {
                record.d->values << QStringView(ptr->Data.Txt.pStringArray[i]).toLatin1();
            }
            reply->textRecords.append(record);
        }
    }

    DnsRecordListFree(results.pQueryRecords, DnsFreeRecordList);
}

QT_END_NAMESPACE
