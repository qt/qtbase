/****************************************************************************
**
** Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: http://www.qt.io/licensing/
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <stdio.h>

static int typeFromParameter(const QString &type)
{
    if (type == "a")
        return QDnsLookup::A;
    if (type == "aaaa")
        return QDnsLookup::AAAA;
    if (type == "any")
        return QDnsLookup::ANY;
    if (type == "cname")
        return QDnsLookup::CNAME;
    if (type == "mx")
        return QDnsLookup::MX;
    if (type == "ns")
        return QDnsLookup::NS;
    if (type == "ptr")
        return QDnsLookup::PTR;
    if (type == "srv")
        return QDnsLookup::SRV;
    if (type == "txt")
        return QDnsLookup::TXT;
    return -1;
}

//! [0]

enum CommandLineParseResult
{
    CommandLineOk,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, DnsQuery *query, QString *errorMessage)
{
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    const QCommandLineOption nameServerOption("n", "The name server to use.", "nameserver");
    parser.addOption(nameServerOption);
    const QCommandLineOption typeOption("t", "The lookup type.", "type");
    parser.addOption(typeOption);
    parser.addPositionalArgument("name", "The name to look up.");
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    if (!parser.parse(QCoreApplication::arguments())) {
        *errorMessage = parser.errorText();
        return CommandLineError;
    }

    if (parser.isSet(versionOption))
        return CommandLineVersionRequested;

    if (parser.isSet(helpOption))
        return CommandLineHelpRequested;

    if (parser.isSet(nameServerOption)) {
        const QString nameserver = parser.value(nameServerOption);
        query->nameServer = QHostAddress(nameserver);
        if (query->nameServer.isNull() || query->nameServer.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol) {
            *errorMessage = "Bad nameserver address: " + nameserver;
            return CommandLineError;
        }
    }

    if (parser.isSet(typeOption)) {
        const QString typeParameter = parser.value(typeOption);
        const int type = typeFromParameter(typeParameter.toLower());
        if (type < 0) {
            *errorMessage = "Bad record type: " + typeParameter;
            return CommandLineError;
        }
        query->type = static_cast<QDnsLookup::Type>(type);
    }

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty()) {
        *errorMessage = "Argument 'name' missing.";
        return CommandLineError;
    }
    if (positionalArguments.size() > 1) {
        *errorMessage = "Several 'name' arguments specified.";
        return CommandLineError;
    }
    query->name = positionalArguments.first();

    return CommandLineOk;
}

//! [0]

DnsManager::DnsManager()
{
    dns = new QDnsLookup(this);
    connect(dns, SIGNAL(finished()), this, SLOT(showResults()));
}

void DnsManager::execute()
{
    // lookup type
    dns->setType(query.type);
    if (!query.nameServer.isNull())
        dns->setNameserver(query.nameServer);
    dns->setName(query.name);
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

//! [1]
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCoreApplication::setApplicationName(QCoreApplication::translate("QDnsLookupExample", "DNS Lookup Example"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("QDnsLookupExample", "An example demonstrating the class QDnsLookup."));
    DnsQuery query;
    QString errorMessage;
    switch (parseCommandLine(parser, &query, &errorMessage)) {
    case CommandLineOk:
        break;
    case CommandLineError:
        fputs(qPrintable(errorMessage), stderr);
        fputs("\n\n", stderr);
        fputs(qPrintable(parser.helpText()), stderr);
        return 1;
    case CommandLineVersionRequested:
        printf("%s %s\n", qPrintable(QCoreApplication::applicationName()),
               qPrintable(QCoreApplication::applicationVersion()));
        return 0;
    case CommandLineHelpRequested:
        parser.showHelp();
        Q_UNREACHABLE();
    }
//! [1]

    DnsManager manager;
    manager.setQuery(query);
    QTimer::singleShot(0, &manager, SLOT(execute()));

    return app.exec();
}
