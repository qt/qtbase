/****************************************************************************
**
** Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "dnslookup.h"

#include <QCoreApplication>
#include <QDnsLookup>
#include <QHostAddress>
#include <QStringList>
#include <QTimer>

#include <stdio.h>

static void usage() {
    printf("Qt DNS example - performs DNS lookups\n"
           "Usage: dnslookup [-t <type>] name\n\n");
}

DnsManager::DnsManager()
{
    dns = new QDnsLookup(this);
    connect(dns, SIGNAL(finished()), this, SLOT(showResults()));
}

void DnsManager::execute()
{
    QStringList args = QCoreApplication::instance()->arguments();
    args.takeFirst();

    // lookup type
    dns->setType(QDnsLookup::A);
    if (args.size() > 1 && args.first() == "-t") {
        args.takeFirst();
        const QString type = args.takeFirst().toLower();
        if (type == "a")
            dns->setType(QDnsLookup::A);
        else if (type == "aaaa")
            dns->setType(QDnsLookup::AAAA);
        else if (type == "any")
            dns->setType(QDnsLookup::ANY);
        else if (type == "cname")
            dns->setType(QDnsLookup::CNAME);
        else if (type == "mx")
            dns->setType(QDnsLookup::MX);
        else if (type == "ns")
            dns->setType(QDnsLookup::NS);
        else if (type == "ptr")
            dns->setType(QDnsLookup::PTR);
        else if (type == "srv")
            dns->setType(QDnsLookup::SRV);
        else if (type == "txt")
            dns->setType(QDnsLookup::TXT);
        else {
            printf("Bad record type: %s\n", qPrintable(type));
            QCoreApplication::instance()->quit();
            return;
        }
    }
    if (args.isEmpty()) {
        usage();
        QCoreApplication::instance()->quit();
        return;
    }
    dns->setName(args.takeFirst());
    dns->lookup();
}

void DnsManager::showResults()
{
    if (dns->error() != QDnsLookup::NoError)
        printf("Error: %i (%s)\n", dns->error(), qPrintable(dns->errorString()));

    // CNAME records
    foreach (const QDnsDomainNameRecord &record, dns->canonicalNameRecords())
        printf("%s\t%i\tIN\tCNAME\t%s\n", qPrintable(record.name()), record.timeToLive(), qPrintable(record.value()));

    // A and AAAA records
    foreach (const QDnsHostAddressRecord &record, dns->hostAddressRecords()) {
        const char *type = (record.value().protocol() == QAbstractSocket::IPv6Protocol) ? "AAAA" : "A";
        printf("%s\t%i\tIN\t%s\t%s\n", qPrintable(record.name()), record.timeToLive(), type, qPrintable(record.value().toString()));
    }

    // MX records
    foreach (const QDnsMailExchangeRecord &record, dns->mailExchangeRecords())
        printf("%s\t%i\tIN\tMX\t%u %s\n", qPrintable(record.name()), record.timeToLive(), record.preference(), qPrintable(record.exchange()));

    // NS records
    foreach (const QDnsDomainNameRecord &record, dns->nameServerRecords())
        printf("%s\t%i\tIN\tNS\t%s\n", qPrintable(record.name()), record.timeToLive(), qPrintable(record.value()));

    // PTR records
    foreach (const QDnsDomainNameRecord &record, dns->pointerRecords())
        printf("%s\t%i\tIN\tPTR\t%s\n", qPrintable(record.name()), record.timeToLive(), qPrintable(record.value()));

    // SRV records
    foreach (const QDnsServiceRecord &record, dns->serviceRecords())
        printf("%s\t%i\tIN\tSRV\t%u %u %u %s\n", qPrintable(record.name()), record.timeToLive(), record.priority(), record.weight(), record.port(), qPrintable(record.target()));

    // TXT records
    foreach (const QDnsTextRecord &record, dns->textRecords()) {
        QStringList values;
        foreach (const QByteArray &ba, record.values())
            values << "\"" + QString::fromLatin1(ba) + "\"";
        printf("%s\t%i\tIN\tTXT\t%s\n", qPrintable(record.name()), record.timeToLive(), qPrintable(values.join(' ')));
    }

    QCoreApplication::instance()->quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    DnsManager manager;
    QTimer::singleShot(0, &manager, SLOT(execute()));

    return app.exec();
}
