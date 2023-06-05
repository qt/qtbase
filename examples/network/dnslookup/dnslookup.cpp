// Copyright (C) 2016 Jeremy Lainé <jeremy.laine@m4x.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "dnslookup.h"

#include <QCoreApplication>
#include <QDnsLookup>
#include <QHostAddress>
#include <QStringList>
#include <QTimer>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include <cstdio>

using namespace Qt::StringLiterals;

static std::optional<QDnsLookup::Type> typeFromParameter(QStringView type)
{
    if (type.compare(u"a", Qt::CaseInsensitive) == 0)
        return QDnsLookup::A;
    if (type.compare(u"aaaa", Qt::CaseInsensitive) == 0)
        return QDnsLookup::AAAA;
    if (type.compare(u"any", Qt::CaseInsensitive) == 0)
        return QDnsLookup::ANY;
    if (type.compare(u"cname", Qt::CaseInsensitive) == 0)
        return QDnsLookup::CNAME;
    if (type.compare(u"mx", Qt::CaseInsensitive) == 0)
        return QDnsLookup::MX;
    if (type.compare(u"ns", Qt::CaseInsensitive) == 0)
        return QDnsLookup::NS;
    if (type.compare(u"ptr", Qt::CaseInsensitive) == 0)
        return QDnsLookup::PTR;
    if (type.compare(u"srv", Qt::CaseInsensitive) == 0)
        return QDnsLookup::SRV;
    if (type.compare(u"txt", Qt::CaseInsensitive) == 0)
        return QDnsLookup::TXT;
    return std::nullopt;
}

//! [0]

struct CommandLineParseResult
{
    enum class Status {
        Ok,
        Error,
        VersionRequested,
        HelpRequested
    };
    Status statusCode = Status::Ok;
    std::optional<QString> errorString = std::nullopt;
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, DnsQuery *query)
{
    using Status = CommandLineParseResult::Status;

    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
    const QCommandLineOption nameServerOption("n", "The name server to use.", "nameserver");
    parser.addOption(nameServerOption);
    const QCommandLineOption typeOption("t", "The lookup type.", "type");
    parser.addOption(typeOption);
    parser.addPositionalArgument("name", "The name to look up.");
    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    if (!parser.parse(QCoreApplication::arguments()))
        return { Status::Error, parser.errorText() };

    if (parser.isSet(versionOption))
        return { Status::VersionRequested };

    if (parser.isSet(helpOption))
        return { Status::HelpRequested };

    if (parser.isSet(nameServerOption)) {
        const QString nameserver = parser.value(nameServerOption);
        query->nameServer = QHostAddress(nameserver);
        if (query->nameServer.isNull()
            || query->nameServer.protocol() == QAbstractSocket::UnknownNetworkLayerProtocol) {
            return { Status::Error,
                     u"Bad nameserver address: %1"_s.arg(nameserver) };
        }
    }

    if (parser.isSet(typeOption)) {
        const QString typeParameter = parser.value(typeOption);
        if (std::optional<QDnsLookup::Type> type = typeFromParameter(typeParameter))
            query->type = *type;
        else
            return { Status::Error, u"Bad record type: %1"_s.arg(typeParameter) };
    }

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.isEmpty())
        return { Status::Error, u"Argument 'name' missing."_s };
    if (positionalArguments.size() > 1)
        return { Status::Error, u"Several 'name' arguments specified."_s };
    query->name = positionalArguments.first();

    return { Status::Ok };
}

//! [0]

DnsManager::DnsManager()
    : dns(new QDnsLookup(this))
{
    connect(dns, &QDnsLookup::finished, this, &DnsManager::showResults);
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
        std::printf("Error: %i (%s)\n", dns->error(), qPrintable(dns->errorString()));

    // CNAME records
    const QList<QDnsDomainNameRecord> cnameRecords = dns->canonicalNameRecords();
    for (const QDnsDomainNameRecord &record : cnameRecords) {
        std::printf("%s\t%i\tIN\tCNAME\t%s\n", qPrintable(record.name()), record.timeToLive(),
                    qPrintable(record.value()));
    }

    // A and AAAA records
    const QList<QDnsHostAddressRecord> aRecords = dns->hostAddressRecords();
    for (const QDnsHostAddressRecord &record : aRecords) {
        const char *type =
                (record.value().protocol() == QAbstractSocket::IPv6Protocol) ? "AAAA" : "A";
        std::printf("%s\t%i\tIN\t%s\t%s\n", qPrintable(record.name()), record.timeToLive(), type,
                    qPrintable(record.value().toString()));
    }

    // MX records
    const QList<QDnsMailExchangeRecord> mxRecords = dns->mailExchangeRecords();
    for (const QDnsMailExchangeRecord &record : mxRecords) {
        std::printf("%s\t%i\tIN\tMX\t%u %s\n", qPrintable(record.name()), record.timeToLive(),
                    record.preference(), qPrintable(record.exchange()));
    }

    // NS records
    const QList<QDnsDomainNameRecord> nsRecords = dns->nameServerRecords();
    for (const QDnsDomainNameRecord &record : nsRecords) {
        std::printf("%s\t%i\tIN\tNS\t%s\n", qPrintable(record.name()), record.timeToLive(),
                    qPrintable(record.value()));
    }

    // PTR records
    const QList<QDnsDomainNameRecord> ptrRecords = dns->pointerRecords();
    for (const QDnsDomainNameRecord &record : ptrRecords) {
        std::printf("%s\t%i\tIN\tPTR\t%s\n", qPrintable(record.name()), record.timeToLive(),
                    qPrintable(record.value()));
    }

    // SRV records
    const QList<QDnsServiceRecord> srvRecords = dns->serviceRecords();
    for (const QDnsServiceRecord &record : srvRecords) {
        std::printf("%s\t%i\tIN\tSRV\t%u %u %u %s\n", qPrintable(record.name()),
                    record.timeToLive(), record.priority(), record.weight(), record.port(),
                    qPrintable(record.target()));
    }

    // TXT records
    const QList<QDnsTextRecord> txtRecords = dns->textRecords();
    for (const QDnsTextRecord &record : txtRecords) {
        QStringList values;
        const QList<QByteArray> dnsRecords = record.values();
        for (const QByteArray &ba : dnsRecords)
            values << "\"" + QString::fromLatin1(ba) + "\"";
        std::printf("%s\t%i\tIN\tTXT\t%s\n", qPrintable(record.name()), record.timeToLive(),
                    qPrintable(values.join(' ')));
    }

    QCoreApplication::instance()->quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

//! [1]
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCoreApplication::setApplicationName(QCoreApplication::translate("QDnsLookupExample",
                                                                     "DNS Lookup Example"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("QDnsLookupExample",
                                                                 "An example demonstrating the "
                                                                 "class QDnsLookup."));
    DnsQuery query;
    using Status = CommandLineParseResult::Status;
    CommandLineParseResult parseResult = parseCommandLine(parser, &query);
    switch (parseResult.statusCode) {
    case Status::Ok:
        break;
    case Status::Error:
        std::fputs(qPrintable(parseResult.errorString.value_or(u"Unknown error occurred"_s)),
                   stderr);
        std::fputs("\n\n", stderr);
        std::fputs(qPrintable(parser.helpText()), stderr);
        return 1;
    case Status::VersionRequested:
        parser.showVersion();
        Q_UNREACHABLE_RETURN(0);
    case Status::HelpRequested:
        parser.showHelp();
        Q_UNREACHABLE_RETURN(0);
    }
//! [1]

    DnsManager manager;
    manager.setQuery(query);
    QTimer::singleShot(0, &manager, &DnsManager::execute);

    return app.exec();
}
