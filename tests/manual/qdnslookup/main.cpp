// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QMetaEnum>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHostInfo>
#include <QtNetwork/QDnsLookup>

#if QT_CONFIG(ssl)
#  include <QtNetwork/QSslCipher>
#  include <QtNetwork/QSslConfiguration>
#endif

#include <stdlib.h>
#include <stdio.h>

#include <chrono>

using namespace Qt::StringLiterals;
using namespace std::chrono;
using namespace std::chrono_literals;

static QDnsLookup::Type typeFromString(QString str)
{
    // we can use the meta object
    QMetaEnum me = QMetaEnum::fromType<QDnsLookup::Type>();

    bool ok;
    int value = me.keyToValue(str.toUpper().toLatin1(), &ok);
    if (!ok)
        return QDnsLookup::Type(0);
    return QDnsLookup::Type(value);
}

template <typename Enum> QByteArray enumToString(Enum value)
{
    QMetaEnum me = QMetaEnum::fromType<Enum>();
    QByteArray keys = me.valueToKeys(int(value));
    if (keys.isEmpty())
        return QByteArrayLiteral("<unknown>");

    // return the last one
    qsizetype idx = keys.lastIndexOf('|');
    if (idx > 0)
        return std::move(keys).sliced(idx + 1);
    return keys;
}

static int showHelp(const char *argv0, int exitcode)
{
    // like dig
    printf("%s [@global-server] [domain] [query-type]\n", argv0);
    return exitcode;
}

static auto parseServerAddress(QString server)
{
    struct R {
        QHostAddress address;
        int port;
    } r;

    // let's use QUrl to help us
    QUrl url;
    url.setAuthority(server);
    if (!url.isValid() || !url.userInfo().isNull())
        return r;           // failed

    r.port = url.port(0);
    r.address.setAddress(url.host());
    if (r.address.isNull()) {
        // try to resolve a hostname
        QHostInfo hostinfo = QHostInfo::fromName(url.host());
        const QList<QHostAddress> addresses = hostinfo.addresses();
        if (!hostinfo.error() && !addresses.isEmpty())
            r.address = addresses.at(0);
    }
    return r;
}

static void printAnswers(const QDnsLookup &lookup)
{
    printf("\n;; ANSWER:\n");
    static auto printRecordCommon = [](const auto &rr, const char *rrtype) {
        printf("%-23s %6d  IN %s\t", qPrintable(rr.name()), rr.timeToLive(), rrtype);
    };
    auto printNameRecords = [](const char *rrtype, const QList<QDnsDomainNameRecord> list) {
        for (const QDnsDomainNameRecord &rr : list) {
            printRecordCommon(rr, rrtype);
            printf("%s\n", qPrintable(rr.value()));
        }
    };

    for (const QDnsMailExchangeRecord &rr : lookup.mailExchangeRecords()) {
        printRecordCommon(rr, "MX");
        printf("%d %s\n", rr.preference(), qPrintable(rr.exchange()));
    }
    for (const QDnsServiceRecord &rr : lookup.serviceRecords()) {
        printRecordCommon(rr, "SRV");
        printf("%d %d %d %s\n", rr.priority(), rr.weight(), rr.port(),
               qPrintable(rr.target()));
    }
    printNameRecords("NS", lookup.nameServerRecords());
    printNameRecords("PTR", lookup.pointerRecords());
    printNameRecords("CNAME", lookup.canonicalNameRecords());

    for (const QDnsHostAddressRecord &rr : lookup.hostAddressRecords()) {
        QHostAddress addr = rr.value();
        printRecordCommon(rr, addr.protocol() == QHostAddress::IPv6Protocol ? "AAAA" : "A");
        printf("%s\n", qPrintable(addr.toString()));
    }

    for (const QDnsTextRecord &rr : lookup.textRecords()) {
        printRecordCommon(rr, "TXT");
        for (const QByteArray &data : rr.values())
            printf("%s ", qPrintable(QDebug::toString(data)));
        puts("");
    }

    for (const QDnsTlsAssociationRecord &rr : lookup.tlsAssociationRecords()) {
        printRecordCommon(rr, "TLSA");
        printf("( %u %u %u ; %s %s %s\n\t%s )\n", quint8(rr.usage()), quint8(rr.selector()),
               quint8(rr.matchType()), enumToString(rr.usage()).constData(),
               enumToString(rr.selector()).constData(), enumToString(rr.matchType()).constData(),
               rr.value().toHex().toUpper().constData());
    }
}

static void printResults(const QDnsLookup &lookup, QElapsedTimer::Duration duration)
{
    if (QDnsLookup::Error error = lookup.error())
        printf(";; status: %s (%s)", QMetaEnum::fromType<QDnsLookup::Error>().valueToKey(error),
               qPrintable(lookup.errorString()));
    else
        printf(";; status: NoError");
    if (lookup.isAuthenticData())
        printf("; AuthenticData");
    puts("");

    QMetaEnum me = QMetaEnum::fromType<QDnsLookup::Type>();
    printf(";; QUESTION:\n");
    printf(";%-30s IN %s\n", qPrintable(lookup.name()),
           me.valueToKey(lookup.type()));

    if (lookup.error() == QDnsLookup::NoError)
        printAnswers(lookup);

    printf("\n;; Query time: %lld ms\n", qint64(duration_cast<milliseconds>(duration).count()));
    if (QHostAddress server = lookup.nameserver(); !server.isNull()) {
        quint16 port = lookup.nameserverPort();
        if (port == 0)
            port = QDnsLookup::defaultPortForProtocol(lookup.nameserverProtocol());
        printf(";; SERVER: %s#%d", qPrintable(server.toString()), port);
#if QT_CONFIG(ssl)
        if (lookup.nameserverProtocol() != QDnsLookup::Standard) {
            if (QSslConfiguration conf = lookup.sslConfiguration(); !conf.isNull()) {
                printf(" (%s %s)", enumToString(conf.sessionProtocol()).constData(),
                       qPrintable(conf.sessionCipher().name()));
            }
        }
#endif
        puts("");
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QDnsLookup::Type type = {};
    QDnsLookup::Protocol protocol = QDnsLookup::Standard;
    QString domain, server;
    const QStringList args = QCoreApplication::arguments().sliced(1);
    for (const QString &arg : args) {
        if (arg.startsWith(u'@')) {
            server = arg.mid(1);
            continue;
        }
        if (arg == u"-h")
            return showHelp(argv[0], EXIT_SUCCESS);
        if (arg == "+tls") {
            protocol = QDnsLookup::DnsOverTls;
            continue;
        } else if (arg == "+notls") {
            protocol = QDnsLookup::Standard;
            continue;
        }

        if (domain.isNull()) {
            domain = arg;
            continue;
        }
        if (type != QDnsLookup::Type{})
            return showHelp(argv[0], EXIT_FAILURE);

        type = typeFromString(arg);
        if (type == QDnsLookup::Type{}) {
            fprintf(stderr, "%s: unknown DNS record type '%s'. Valid types are:\n",
                    argv[0], qPrintable(arg));
            QMetaEnum me = QMetaEnum::fromType<QDnsLookup::Type>();
            for (int i = 0; i < me.keyCount(); ++i)
                fprintf(stderr, "    %s\n", me.key(i));
            return EXIT_FAILURE;
        }
    }

    if (domain.isEmpty())
        domain = u"qt-project.org"_s;
    if (type == QDnsLookup::Type{})
        type = QDnsLookup::A;

    QDnsLookup lookup(type, domain);
    if (!server.isEmpty()) {
        auto addr = parseServerAddress(server);
        if (addr.address.isNull()) {
            fprintf(stderr, "%s: could not parse name server address '%s'\n",
                    argv[0], qPrintable(server));
            return EXIT_FAILURE;
        }
        lookup.setNameserver(protocol, addr.address, addr.port);
    }

    // execute the lookup
    QObject::connect(&lookup, &QDnsLookup::finished, qApp, &QCoreApplication::quit);
    QTimer::singleShot(15s, []() { qApp->exit(EXIT_FAILURE); });

    QElapsedTimer timer;
    timer.start();
    lookup.lookup();
    if (a.exec() == EXIT_FAILURE)
        return EXIT_FAILURE;
    printf("; <<>> QDnsLookup " QT_VERSION_STR " <<>> %s %s\n",
           qPrintable(QCoreApplication::applicationName()), qPrintable(args.join(u' ')));
    printResults(lookup, timer.durationElapsed());
    return 0;
}
