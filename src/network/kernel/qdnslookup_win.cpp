/****************************************************************************
**
** Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: http://www.qt-project.org/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <winsock2.h>
#include "qdnslookup_p.h"

#include <qurl.h>
#include <private/qmutexpool_p.h>
#include <private/qsystemlibrary_p.h>

#include <qt_windows.h>
#include <windns.h>

QT_BEGIN_NAMESPACE

typedef DNS_STATUS (*dns_query_utf8_proto)(PCSTR,WORD,DWORD,PIP4_ARRAY,PDNS_RECORD*,PVOID*);
static dns_query_utf8_proto local_dns_query_utf8 = 0;
typedef void (*dns_record_list_free_proto)(PDNS_RECORD,DNS_FREE_TYPE);
static dns_record_list_free_proto local_dns_record_list_free = 0;

static void resolveLibrary()
{
    local_dns_query_utf8 = (dns_query_utf8_proto) QSystemLibrary::resolve(QLatin1String("dnsapi"), "DnsQuery_UTF8");
    local_dns_record_list_free = (dns_record_list_free_proto) QSystemLibrary::resolve(QLatin1String("dnsapi"), "DnsRecordListFree");
}

void QDnsLookupRunnable::query(const int requestType, const QByteArray &requestName, QDnsLookupReply *reply)
{
    // Load DnsQuery_UTF8 and DnsRecordListFree on demand.
    static volatile bool triedResolve = false;
    if (!triedResolve) {
        QMutexLocker locker(QMutexPool::globalInstanceGet(&local_dns_query_utf8));
        if (!triedResolve) {
            resolveLibrary();
            triedResolve = true;
        }
    }

    // If DnsQuery_UTF8 or DnsRecordListFree is missing, fail.
    if (!local_dns_query_utf8 || !local_dns_record_list_free) {
        reply->error = QDnsLookup::ResolverError,
        reply->errorString = tr("Resolver functions not found");
        return;
    }

    // Perform DNS query.
    PDNS_RECORD dns_records;
    const DNS_STATUS status = local_dns_query_utf8(requestName, requestType, DNS_QUERY_STANDARD, NULL, &dns_records, NULL);
    switch (status) {
    case ERROR_SUCCESS:
        break;
    case DNS_ERROR_RCODE_FORMAT_ERROR:
        reply->error = QDnsLookup::InvalidRequestError;
        reply->errorString = tr("Server could not process query");
        return;
    case DNS_ERROR_RCODE_SERVER_FAILURE:
        reply->error = QDnsLookup::ServerFailureError;
        reply->errorString = tr("Server failure");
        return;
    case DNS_ERROR_RCODE_NAME_ERROR:
        reply->error = QDnsLookup::NotFoundError;
        reply->errorString = tr("Non existent domain");
        return;
    case DNS_ERROR_RCODE_REFUSED:
        reply->error = QDnsLookup::ServerRefusedError;
        reply->errorString = tr("Server refused to answer");
        return;
    default:
        reply->error = QDnsLookup::InvalidReplyError;
        reply->errorString = tr("Invalid reply received");
        return;
    }

    // Extract results.
    for (PDNS_RECORD ptr = dns_records; ptr != NULL; ptr = ptr->pNext) {
        const QString name = QUrl::fromAce((char*)ptr->pName);
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
            record.d->value = QUrl::fromAce((char*)ptr->Data.Cname.pNameHost);
            reply->canonicalNameRecords.append(record);
        } else if (ptr->wType == QDnsLookup::MX) {
            QDnsMailExchangeRecord record;
            record.d->name = name;
            record.d->exchange = QUrl::fromAce((char*)ptr->Data.Mx.pNameExchange);
            record.d->preference = ptr->Data.Mx.wPreference;
            record.d->timeToLive = ptr->dwTtl;
            reply->mailExchangeRecords.append(record);
        } else if (ptr->wType == QDnsLookup::NS) {
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = QUrl::fromAce((char*)ptr->Data.Ns.pNameHost);
            reply->nameServerRecords.append(record);
        } else if (ptr->wType == QDnsLookup::PTR) {
            QDnsDomainNameRecord record;
            record.d->name = name;
            record.d->timeToLive = ptr->dwTtl;
            record.d->value = QUrl::fromAce((char*)ptr->Data.Ptr.pNameHost);
            reply->pointerRecords.append(record);
        } else if (ptr->wType == QDnsLookup::SRV) {
            QDnsServiceRecord record;
            record.d->name = name;
            record.d->target = QUrl::fromAce((char*)ptr->Data.Srv.pNameTarget);
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
                record.d->values << QByteArray((char*)ptr->Data.Txt.pStringArray[i]);
            }
            reply->textRecords.append(record);
        }
    }

    local_dns_record_list_free(dns_records, DnsFreeRecordList);
}

QT_END_NAMESPACE
