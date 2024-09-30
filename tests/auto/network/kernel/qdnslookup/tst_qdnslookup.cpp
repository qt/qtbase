// Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QSignalSpy>
#include <QtTest/private/qpropertytesthelper_p.h>

#include <QtNetwork/QDnsLookup>

#include <QtCore/QRandomGenerator>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

#if QT_CONFIG(networkproxy)
#  include <QtNetwork/QNetworkProxyFactory>
#endif
#if QT_CONFIG(process)
#  include <QtCore/QProcess>
#endif
#if QT_CONFIG(ssl)
#  include <QtNetwork/QSslSocket>
#endif

#ifdef Q_OS_UNIX
#  include <QtCore/QFile>
#else
#  include <winsock2.h>
#  include <iphlpapi.h>
#endif

using namespace Qt::StringLiterals;
static const int Timeout = 15000; // 15s

class tst_QDnsLookup: public QObject
{
    Q_OBJECT
public:
    const QString normalDomain = u".test.qt-project.org"_s;
    const QString idnDomain = u".alqualondë.test.qt-project.org"_s;
    QHostAddress alternateDnsServer;
    quint16 alternateDnsServerPort = 53;
    bool usingIdnDomain = false;
    bool dnsServersMustWork = false;

private:
    QString domainName(const QString &input);
    QString domainNameList(const QString &input);
    QStringList domainNameListAlternatives(const QString &input);

    std::unique_ptr<QDnsLookup> lookupCommon(QDnsLookup::Type type, const QString &domain,
                                             QHostAddress server = {}, quint16 port = 0,
                                             QDnsLookup::Protocol protocol = QDnsLookup::Standard);
    QStringList formatReply(const QDnsLookup *lookup) const;

    void setNameserver_helper(QDnsLookup::Protocol protocol);
public slots:
    void initTestCase();

private slots:
    void lookupLocalhost();
    void lookupRoot();
    void lookupNxDomain_data();
    void lookupNxDomain();
    void lookup_data();
    void lookup();
    void lookupIdn_data() { lookup_data(); }
    void lookupIdn();

    void lookupReuse();
    void lookupAbortRetry();
    void setNameserverLoopback();
    void setNameserver_data();
    void setNameserver();
    void dnsOverTls_data();
    void dnsOverTls();
    void bindingsAndProperties();
    void automatedBindings();
};

static constexpr qsizetype HeaderSize = 6 * sizeof(quint16);
static const char preparedDnsQuery[] =
        // header
        "\x00\x00"              // transaction ID, we'll replace
        "\x01\x20"              // flags
        "\x00\x01"              // qdcount
        "\x00\x00"              // ancount
        "\x00\x00"              // nscount
        "\x00\x00"              // arcount
        // query:
        "\x00\x00\x06\x00\x01"  // <root domain> IN SOA
        ;

static QList<QHostAddress> systemNameservers(QDnsLookup::Protocol protocol)
{
    QList<QHostAddress> result;
    if (protocol != QDnsLookup::Standard)
        return result;
    if (auto tst = static_cast<tst_QDnsLookup *>(QTest::testObject()); !tst->alternateDnsServer.isNull()) {
        // if the user provided an alternate server, that's our "system"
        if (tst->alternateDnsServerPort == 53)
            result.emplaceBack(tst->alternateDnsServer);
        return result;
    }

#ifdef Q_OS_WIN
    ULONG infosize = 0;
    DWORD r = GetNetworkParams(nullptr, &infosize);
    auto buffer = std::make_unique<uchar[]>(infosize);
    auto info = new (buffer.get()) FIXED_INFO;
    r = GetNetworkParams(info, &infosize);
    if (r == NO_ERROR) {
        for (PIP_ADDR_STRING ptr = &info->DnsServerList; ptr; ptr = ptr->Next) {
            QLatin1StringView addr(ptr->IpAddress.String);
            result.emplaceBack(addr);
        }
    }
#else
    auto parseFile = [&](QLatin1StringView path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return;

        while (!f.atEnd()) {
            static const char command[] = "nameserver";
            QByteArray line = f.readLine().simplified();
            if (!line.startsWith(command))
                continue;

            QHostAddress addr(QLatin1StringView(line).mid(sizeof(command)));
            if (!result.contains(addr))
                result.emplaceBack(std::move(addr));
        }
    };
    parseFile("/etc/resolv.conf"_L1);
    parseFile("/run/systemd/resolve/resolv.conf"_L1);
#endif

    return result;
}

static QList<QHostAddress> globalPublicNameservers(QDnsLookup::Protocol proto)
{
    const char *const candidates[] = {
        // Google's dns.google
        "8.8.8.8", "2001:4860:4860::8888",
        //"8.8.4.4", "2001:4860:4860::8844",

        // CloudFare's one.one.one.one
        "1.1.1.1", "2606:4700:4700::1111",
        //"1.0.0.1", "2606:4700:4700::1001",

        // Quad9's dns9
        //"9.9.9.9", "2620:fe::9",
    };

    auto udpSendAndReceive = [](const QHostAddress &addr, QByteArray &data) {
        QUdpSocket socket;
        socket.connectToHost(addr, 53);
        if (socket.waitForConnected(1))
            socket.write(data);

        if (!socket.waitForReadyRead(1000))
            return socket.errorString();

        QNetworkDatagram dgram = socket.receiveDatagram();
        if (!dgram.isValid())
            return socket.errorString();

        data = dgram.data();
        return QString();
    };

    auto tlsSendAndReceive = [](const QHostAddress &addr, QByteArray &data) {
#if QT_CONFIG(ssl)
        QSslSocket socket;
        QDeadlineTimer timeout(2000);
        socket.connectToHostEncrypted(addr.toString(), 853);
        if (!socket.waitForEncrypted(2000))
            return socket.errorString();

        quint16 size = qToBigEndian<quint16>(data.size());
        socket.write(reinterpret_cast<char *>(&size), sizeof(size));
        socket.write(data);

        if (!socket.waitForReadyRead(timeout.remainingTime()))
            return socket.errorString();
        if (socket.bytesAvailable() < 2)
            return u"protocol error"_s;

        socket.read(reinterpret_cast<char *>(&size), sizeof(size));
        size = qFromBigEndian(size);

        while (socket.bytesAvailable() < size) {
            int remaining = timeout.remainingTime();
            if (remaining < 0 || !socket.waitForReadyRead(remaining))
                return socket.errorString();
        }

        data = socket.readAll();
        return QString();
#else
        return u"SSL/TLS support not compiled in"_s;
#endif
    };

    QList<QHostAddress> result;
    QRandomGenerator &rng = *QRandomGenerator::system();
    for (auto name : candidates) {
        // check the candidates for reachability
        QHostAddress addr{QLatin1StringView(name)};
        quint16 id = quint16(rng());
        QByteArray data(preparedDnsQuery, sizeof(preparedDnsQuery));
        char *ptr = data.data();
        qToBigEndian(id, ptr);

        QString errorString = [&] {
            switch (proto) {
            case QDnsLookup::Standard:      return udpSendAndReceive(addr, data);
            case QDnsLookup::DnsOverTls:    return tlsSendAndReceive(addr, data);
            }
            Q_UNREACHABLE();
        }();
        if (!errorString.isEmpty()) {
            qDebug() << addr << "discarded:" << errorString;
            continue;
        }

        ptr = data.data();
        if (data.size() < HeaderSize) {
            qDebug() << addr << "discarded: reply too small";
            continue;
        }

        bool ok = qFromBigEndian<quint16>(ptr) == id
                && (ptr[2] & 0x80)                          // is a reply
                && (ptr[3] & 0xf) == 0                      // rcode NOERROR
                && qFromBigEndian<quint16>(ptr + 4) == 1    // qdcount
                && qFromBigEndian<quint16>(ptr + 6) >= 1;   // ancount
        if (!ok) {
            qDebug() << addr << "discarded: invalid reply";
            continue;
        }

        result.emplaceBack(std::move(addr));
    }

    return result;
}

void tst_QDnsLookup::initTestCase()
{
    if (qgetenv("QTEST_ENVIRONMENT") == "ci")
        dnsServersMustWork = true;

#if QT_CONFIG(networkproxy)
    // for DNS-over-TLS
    QNetworkProxyFactory::setUseSystemConfiguration(true);
#endif

    if (QString alternateDns = qEnvironmentVariable("QTEST_DNS_SERVER"); !alternateDns.isEmpty()) {
        // use QUrl to parse host:port, so we get IPv6 too
        QUrl u("dns://" + alternateDns);
        alternateDnsServer = QHostAddress(u.host());
        alternateDnsServerPort = u.port(alternateDnsServerPort);
    }

#if QT_CONFIG(process)
    // make sure these match something in lookup_data()
    QString checkedDomain = domainName(u"a-multi"_s);
    static constexpr QByteArrayView expectedAddresses[] = {
        "192.0.2.1", "192.0.2.2", "192.0.2.3"
    };
    QByteArray output;
    auto dnsServerDoesWork = [&]() {
        // check if the DNS server is too broken
        QProcess nslookup;
#ifdef Q_OS_WIN
        nslookup.setProcessChannelMode(QProcess::MergedChannels);
#else
        nslookup.setProcessChannelMode(QProcess::ForwardedErrorChannel);
#endif
        nslookup.start(u"nslookup"_s, { checkedDomain } );
        if (!nslookup.waitForStarted()) {
            // executable didn't start, we err on assuming the servers work
            return true;
        }

        // if nslookup is running, then we must have the correct answers
        if (!nslookup.waitForFinished(120'000)) {
            qWarning() << "nslookup timed out";
            return true;
        }

        output = nslookup.readAll();
        bool ok = nslookup.exitCode() == 0;
        if (ok)
            ok = std::all_of(std::begin(expectedAddresses), std::end(expectedAddresses),
                             [&](QByteArrayView addr) { return output.contains(addr); });
        if (!ok)
            return false;

        // check a domain that shouldn't exist
        nslookup.setArguments({ domainName(u"invalid.invalid"_s) });
        nslookup.start();
        if (!nslookup.waitForFinished(120'000)) {
            qWarning() << "nslookup timed out";
            return true;
        }
        output = nslookup.readAll();

        return nslookup.exitCode() != 0
#ifdef Q_OS_WIN
            || output.contains("Non-existent domain")
#endif
            ;
    };
    if (!dnsServersMustWork && alternateDnsServer.isNull() && !dnsServerDoesWork()) {
        qWarning() << "Default DNS server in this system cannot correctly resolve" << checkedDomain;
        qWarning() << "Please check if you are connected to the Internet or set the "
                      "QTEST_DNS_SERVER environment variable to a working server.";
        qDebug("Output was:\n%s", output.constData());
        QSKIP("DNS server does not appear to work");
    }
#endif
}

QString tst_QDnsLookup::domainName(const QString &input)
{
    if (input.isEmpty())
        return input;

    if (input.endsWith(QLatin1Char('.'))) {
        QString nodot = input;
        nodot.chop(1);
        return nodot;
    }

    if (usingIdnDomain)
        return input + idnDomain;
    return input + normalDomain;
}

QString tst_QDnsLookup::domainNameList(const QString &input)
{
    const QStringList list = input.split(QLatin1Char(';'));
    QString result;
    for (const QString &s : list) {
        if (!result.isEmpty())
            result += ';';
        result += domainName(s);
    }
    return result;
}

QStringList tst_QDnsLookup::domainNameListAlternatives(const QString &input)
{
    QStringList alternatives = input.split('|');
    for (int i = 0; i < alternatives.size(); ++i)
        alternatives[i] = domainNameList(alternatives[i]);
    return alternatives;
}

std::unique_ptr<QDnsLookup>
tst_QDnsLookup::lookupCommon(QDnsLookup::Type type, const QString &domain,
                             QHostAddress server, quint16 port,
                             QDnsLookup::Protocol protocol)
{
    if (server.isNull()) {
        server = alternateDnsServer;
        port = alternateDnsServerPort;
    }
    auto lookup = std::make_unique<QDnsLookup>(type, domainName(domain), protocol, server, port);
    QObject::connect(lookup.get(), &QDnsLookup::finished,
                     &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    lookup->lookup();
    QTestEventLoop::instance().enterLoopMSecs(Timeout);

    QDnsLookup::Error error = lookup->error();
    if (QTestEventLoop::instance().timeout())
        error = QDnsLookup::TimeoutError;

    if (!dnsServersMustWork && (error == QDnsLookup::ServerFailureError
                                || error == QDnsLookup::ServerRefusedError
                                || error == QDnsLookup::TimeoutError)) {
        // It's not a QDnsLookup problem if the server refuses to answer the query.
        // This happens for queries of type ANY through Dnsmasq, for example.
        [&] {
            auto me = QMetaEnum::fromType<QDnsLookup::Type>();
            QString msg = u"Server refused or was unable to answer query; %1 type %3: %2"_s
                    .arg(domain, lookup->errorString(), QString(me.valueToKey(int(type))));
            QSKIP(msg.toLocal8Bit());
        }();
        return {};
    }

    return lookup;
}

QStringList tst_QDnsLookup::formatReply(const QDnsLookup *lookup) const
{
    QStringList result;
    QString domain = lookup->name();

    auto shorter = [this](QString value) {
        const QString &ending = usingIdnDomain ? idnDomain : normalDomain;
        if (value.endsWith(ending))
            value.chop(ending.size());
        else
            value += u'.';
        return value;
    };

    for (const QDnsMailExchangeRecord &rr : lookup->mailExchangeRecords()) {
        QString entry = u"MX %1 %2"_s.arg(rr.preference(), 5).arg(shorter(rr.exchange()));
        if (rr.name() != domain)
            entry = "MX unexpected label to "_L1 + rr.name();
        result.append(std::move(entry));
    }

    for (const QDnsServiceRecord &rr : lookup->serviceRecords()) {
        QString entry = u"SRV %1 %2 %3 %4"_s.arg(rr.priority(), 5).arg(rr.weight())
                .arg(rr.port()).arg(shorter(rr.target()));
        if (rr.name() != domain)
            entry = "SRV unexpected label to "_L1 + rr.name();
        result.append(std::move(entry));
    }

    auto addNameRecords = [&](QLatin1StringView rrtype, const QList<QDnsDomainNameRecord> &rrset) {
        for (const QDnsDomainNameRecord &rr : rrset) {
            QString entry = u"%1 %2"_s.arg(rrtype, shorter(rr.value()));
            if (rr.name() != domain)
                entry = rrtype + " unexpected label to "_L1 + rr.name();
            result.append(std::move(entry));
        }
    };
    addNameRecords("NS"_L1, lookup->nameServerRecords());
    addNameRecords("PTR"_L1, lookup->pointerRecords());
    addNameRecords("CNAME"_L1, lookup->canonicalNameRecords());

    for (const QDnsHostAddressRecord &rr : lookup->hostAddressRecords()) {
        if (rr.name() != domain)
            continue;   // A and AAAA may appear as extra records in the answer section
        QHostAddress addr = rr.value();
        result.append(u"%1 %2"_s
                      .arg(addr.protocol() == QHostAddress::IPv6Protocol ? "AAAA" : "A",
                           addr.toString()));
    }

    for (const QDnsTextRecord &rr : lookup->textRecords()) {
        QString entry = "TXT"_L1;
        for (const QByteArray &data : rr.values()) {
            entry += u' ';
            entry += QDebug::toString(data);
        }
        result.append(std::move(entry));
    }

    for (const QDnsTlsAssociationRecord &rr : lookup->tlsAssociationRecords()) {
        QString entry = u"TLSA %1 %2 %3 %4"_s.arg(int(rr.usage())).arg(int(rr.selector()))
                .arg(int(rr.matchType())).arg(rr.value().toHex().toUpper());
        if (rr.name() != domain)
            entry = "TLSA unexpected label to "_L1 + rr.name();
        result.append(std::move(entry));
    }

    result.sort();
    return result;
}

void tst_QDnsLookup::lookupLocalhost()
{
    // The "localhost" label is reserved by RFC 2606 to point to the IPv4
    // address 127.0.0.1. However, DNS servers are not required to answer this
    // (though it appears to be a good practice).
    auto lookup = lookupCommon(QDnsLookup::Type::A, u"localhost."_s);
    QVERIFY(lookup);
    QVERIFY(lookup->error() == QDnsLookup::NoError || lookup->error() == QDnsLookup::NotFoundError);
    if (lookup->error() == QDnsLookup::NotFoundError)
        return;

    QList<QDnsHostAddressRecord> hosts = lookup->hostAddressRecords();
    QCOMPARE(hosts.size(), 1);
    QCOMPARE(hosts.at(0).value(), QHostAddress::LocalHost);
    QVERIFY2(hosts.at(0).name().startsWith(lookup->name()),
             qPrintable(hosts.at(0).name()));
}

void tst_QDnsLookup::lookupRoot()
{
#ifdef Q_OS_WIN
    QSKIP("This test fails on Windows as it seems to treat the lookup as a local one.");
#else
    auto lookup = lookupCommon(QDnsLookup::Type::NS, u""_s);
    if (!lookup)
        return;
    QCOMPARE(lookup->error(), QDnsLookup::NoError);

    const QList<QDnsDomainNameRecord> servers = lookup->nameServerRecords();
    QVERIFY(!servers.isEmpty());
    for (const QDnsDomainNameRecord &ns : servers) {
        QCOMPARE(ns.name(), QString());
        QVERIFY(ns.value().endsWith(".root-servers.net"));
    }
#endif
}

void tst_QDnsLookup::lookupNxDomain_data()
{
    QTest::addColumn<QDnsLookup::Type>("type");
    QTest::addColumn<QString>("domain");

    QTest::newRow("a") << QDnsLookup::A << "invalid.invalid";
    QTest::newRow("aaaa") << QDnsLookup::AAAA << "invalid.invalid";
    QTest::newRow("any") << QDnsLookup::ANY << "invalid.invalid";
    QTest::newRow("mx") << QDnsLookup::MX << "invalid.invalid";
    QTest::newRow("ns") << QDnsLookup::NS << "invalid.invalid";
    QTest::newRow("ptr") << QDnsLookup::PTR << "invalid.invalid";
    QTest::newRow("srv") << QDnsLookup::SRV << "invalid.invalid";
    QTest::newRow("txt") << QDnsLookup::TXT << "invalid.invalid";
}

void tst_QDnsLookup::lookupNxDomain()
{
    QFETCH(QDnsLookup::Type, type);
    QFETCH(QString, domain);

    auto lookup = lookupCommon(type, domain);
    if (!lookup)
        return;
    QCOMPARE(lookup->name(), domainName(domain));
    QCOMPARE(lookup->type(), type);
    QCOMPARE(lookup->error(), QDnsLookup::NotFoundError);
}

void tst_QDnsLookup::lookup_data()
{
    QTest::addColumn<QDnsLookup::Type>("type");
    QTest::addColumn<QString>("domain");
    QTest::addColumn<QString>("expected");

    QTest::newRow("a-single") << QDnsLookup::A << "a-single"
                              << "A 192.0.2.1";
    QTest::newRow("a-multi") << QDnsLookup::A << "a-multi"
                             << "A 192.0.2.1;A 192.0.2.2;A 192.0.2.3";
    QTest::newRow("aaaa-single") << QDnsLookup::AAAA << "aaaa-single"
                                 << "AAAA 2001:db8::1";
    QTest::newRow("aaaa-multi") << QDnsLookup::AAAA << "aaaa-multi"
                                << "AAAA 2001:db8::1;AAAA 2001:db8::2;AAAA 2001:db8::3";

    QTest::newRow("any-a-single") << QDnsLookup::ANY << "a-single"
                                  << "A 192.0.2.1";
    QTest::newRow("any-a-plus-aaaa") << QDnsLookup::ANY << "a-plus-aaaa"
                                     << "A 198.51.100.1;AAAA 2001:db8::1:1";
    QTest::newRow("any-multi") << QDnsLookup::ANY << "multi"
                               << "A 198.51.100.1;A 198.51.100.2;A 198.51.100.3;"
                                  "AAAA 2001:db8::1:1;AAAA 2001:db8::1:2" ;

    QTest::newRow("mx-single") << QDnsLookup::MX << "mx-single"
                               << "MX    10 multi";
    QTest::newRow("mx-single-cname") << QDnsLookup::MX << "mx-single-cname"
                                     << "MX    10 cname";
    QTest::newRow("mx-multi") << QDnsLookup::MX << "mx-multi"
                              << "MX    10 multi;MX    20 a-single";
    QTest::newRow("mx-multi-sameprio") << QDnsLookup::MX << "mx-multi-sameprio"
                                       << "MX    10 a-single;MX    10 multi";

    QTest::newRow("ns-single") << QDnsLookup::NS << "ns-single"
                               << "NS ns11.cloudns.net.";
    QTest::newRow("ns-multi") << QDnsLookup::NS << "ns-multi"
                              << "NS ns11.cloudns.net.;NS ns12.cloudns.net.";

#if 0
    // temporarily disabled since the new hosting provider can't insert
    // PTR records outside of the in-addr.arpa zone
    QTest::newRow("ptr-single") << QDnsLookup::PTR << "ptr-single"
                                << "PTR a-single";
#endif
    QTest::newRow("ptr-1.1.1.1") << QDnsLookup::PTR << "1.1.1.1.in-addr.arpa."
                                 << "PTR one.one.one.one.";
    QTest::newRow("ptr-8.8.8.8") << QDnsLookup::PTR << "8.8.8.8.in-addr.arpa."
                                 << "PTR dns.google.";
    QTest::newRow("ptr-2001:4860:4860::8888")
            << QDnsLookup::PTR << "8.8.8.8.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.6.8.4.0.6.8.4.1.0.0.2.ip6.arpa."
            << "PTR dns.google.";
    QTest::newRow("ptr-2606:4700:4700::1111")
            << QDnsLookup::PTR << "1.1.1.1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.7.4.0.0.7.4.6.0.6.2.ip6.arpa."
            << "PTR one.one.one.one.";

    QTest::newRow("srv-single") << QDnsLookup::SRV << "_echo._tcp.srv-single"
                                << "SRV     5 0 7 multi";
    QTest::newRow("srv-prio") << QDnsLookup::SRV << "_echo._tcp.srv-prio"
                              << "SRV     1 0 7 multi;SRV     2 0 7 a-plus-aaaa";
    QTest::newRow("srv-weighted") << QDnsLookup::SRV << "_echo._tcp.srv-weighted"
                                  << "SRV     5 25 7 a-plus-aaaa;SRV     5 75 7 multi";
    QTest::newRow("srv-multi") << QDnsLookup::SRV << "_echo._tcp.srv-multi"
                               << "SRV     1 50 7 multi;"
                                  "SRV     2 50 7 a-single;"
                                  "SRV     2 50 7 aaaa-single;"
                                  "SRV     3 50 7 a-multi";

    QTest::newRow("tlsa") << QDnsLookup::Type::TLSA << "_25._tcp.multi"
                          << "TLSA 3 1 1 0123456789ABCDEFFEDCBA9876543210"
                             "0123456789ABCDEFFEDCBA9876543210";

    QTest::newRow("txt-single") << QDnsLookup::TXT << "txt-single"
                                << "TXT \"Hello\"";
    QTest::newRow("txt-multi-onerr") << QDnsLookup::TXT << "txt-multi-onerr"
                                     << "TXT \"Hello\" \"World\"";
    QTest::newRow("txt-multi-multirr") << QDnsLookup::TXT << "txt-multi-multirr"
                                       << "TXT \"Hello\";TXT \"World\"";
}

void tst_QDnsLookup::lookup()
{
    QFETCH(QDnsLookup::Type, type);
    QFETCH(QString, domain);
    QFETCH(QString, expected);

    std::unique_ptr<QDnsLookup> lookup = lookupCommon(type, domain);
    if (!lookup)
        return;

#ifdef Q_OS_WIN
    if (QTest::currentDataTag() == "tlsa"_L1)
        QSKIP("WinDNS doesn't work properly with TLSA records and we don't know why");
#endif
    QCOMPARE(lookup->errorString(), QString());
    QCOMPARE(lookup->error(), QDnsLookup::NoError);
    QCOMPARE(lookup->type(), type);
    QCOMPARE(lookup->name(), domainName(domain));

    QString result = formatReply(lookup.get()).join(u';');
    QCOMPARE(result, expected);

    // confirm that MX and SRV records are properly sorted
    const QList<QDnsMailExchangeRecord> mx = lookup->mailExchangeRecords();
    for (qsizetype i = 1; i < mx.size(); ++i)
        QCOMPARE_GE(mx[i].preference(), mx[i - 1].preference());

    const QList<QDnsServiceRecord> srv = lookup->serviceRecords();
    for (qsizetype i = 1; i < srv.size(); ++i)
        QCOMPARE_GE(srv[i].priority(), srv[i - 1].priority());
}

void tst_QDnsLookup::lookupIdn()
{
    usingIdnDomain = true;
    lookup();
    usingIdnDomain = false;
}

void tst_QDnsLookup::lookupReuse()
{
    // first lookup
    std::unique_ptr<QDnsLookup> lookup = lookupCommon(QDnsLookup::Type::A, "a-single");

    QCOMPARE(lookup->error(), QDnsLookup::NoError);
    QVERIFY(!lookup->hostAddressRecords().isEmpty());
    QCOMPARE(lookup->hostAddressRecords().first().name(), domainName("a-single"));
    QCOMPARE(lookup->hostAddressRecords().first().value(), QHostAddress("192.0.2.1"));

    // second lookup
    lookup->setType(QDnsLookup::AAAA);
    lookup->setName(domainName("aaaa-single"));
    lookup->lookup();
    QTRY_VERIFY_WITH_TIMEOUT(lookup->isFinished(), Timeout);
    QCOMPARE(lookup->error(), QDnsLookup::NoError);
    QVERIFY(!lookup->hostAddressRecords().isEmpty());
    QCOMPARE(lookup->hostAddressRecords().first().name(), domainName("aaaa-single"));
    QCOMPARE(lookup->hostAddressRecords().first().value(), QHostAddress("2001:db8::1"));
}


void tst_QDnsLookup::lookupAbortRetry()
{
    QDnsLookup lookup;

    // try and abort the lookup
    lookup.setType(QDnsLookup::A);
    lookup.setName(domainName("a-single"));
    lookup.lookup();
    lookup.abort();
    QTRY_VERIFY_WITH_TIMEOUT(lookup.isFinished(), Timeout);
    QCOMPARE(int(lookup.error()), int(QDnsLookup::OperationCancelledError));
    QVERIFY(lookup.hostAddressRecords().isEmpty());

    // retry a different lookup
    lookup.setType(QDnsLookup::AAAA);
    lookup.setName(domainName("aaaa-single"));
    lookup.lookup();
    QTRY_VERIFY_WITH_TIMEOUT(lookup.isFinished(), Timeout);

    QCOMPARE(int(lookup.error()), int(QDnsLookup::NoError));
    QVERIFY(!lookup.hostAddressRecords().isEmpty());
    QCOMPARE(lookup.hostAddressRecords().first().name(), domainName("aaaa-single"));
    QCOMPARE(lookup.hostAddressRecords().first().value(), QHostAddress("2001:db8::1"));
}

void tst_QDnsLookup::setNameserverLoopback()
{
#ifdef Q_OS_WIN
    // Windows doesn't like sending DNS requests to ports other than 53, so
    // let's try it first.
    constexpr quint16 DesiredPort = 53;
#else
    // Trying to bind to port 53 will fail on Unix systems unless this test is
    // run as root, so we try mDNS's port (to help decoding in a packet capture).
    constexpr quint16 DesiredPort = 5353;   // mDNS
#endif
    // random loopback address so multiple copies of this test can run
    QHostAddress desiredAddress(0x7f000000 | QRandomGenerator::system()->bounded(0xffffff));

    QUdpSocket server;
    if (!server.bind(desiredAddress, DesiredPort)) {
        // port in use, try a random one
        server.bind(QHostAddress::LocalHost, 0);
    }
    QCOMPARE(server.state(), QUdpSocket::BoundState);

    QDnsLookup lookup(QDnsLookup::Type::A, u"somelabel.somedomain"_s);
    QSignalSpy spy(&lookup, SIGNAL(finished()));
    lookup.setNameserver(server.localAddress(), server.localPort());

    // QDnsLookup is threaded, so we can answer on the main thread
    QObject::connect(&server, &QUdpSocket::readyRead,
                     &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    QObject::connect(&lookup, &QDnsLookup::finished,
                     &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    lookup.lookup();
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY2(spy.isEmpty(), qPrintable(lookup.errorString()));

    QNetworkDatagram dgram = server.receiveDatagram();
    QByteArray data = dgram.data();
    QCOMPARE_GT(data.size(), HeaderSize);

    quint8 opcode = (quint8(data.at(2)) >> 3) & 0xF;
    QCOMPARE(opcode, 0);        // standard query

    // send an NXDOMAIN reply to release the lookup thread
    QByteArray reply = data;
    reply[2] = 0x80U;   // header->qr = true;
    reply[3] = 3;       // header->rcode = NXDOMAIN;
    server.writeDatagram(reply.constData(), reply.size(), dgram.senderAddress(),
                         dgram.senderPort());
    server.close();

    // now check that the QDnsLookup finished
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(spy.size(), 1);
    QCOMPARE(lookup.error(), QDnsLookup::NotFoundError);
}

template <QDnsLookup::Protocol Protocol>
static void setNameserver_data_helper(const QByteArray &protoName)
{
    if (!QDnsLookup::isProtocolSupported(Protocol))
        QSKIP(protoName + " not supported");

    static QList<QHostAddress> servers = systemNameservers(Protocol)
            + globalPublicNameservers(Protocol);
    QTest::addColumn<QHostAddress>("server");

    if (servers.isEmpty()) {
        QSKIP("No reachable " + protoName + " servers were found");
    } else {
        for (const QHostAddress &h : std::as_const(servers))
            QTest::addRow("%s", qUtf8Printable(h.toString())) << h;
    }
}

void tst_QDnsLookup::setNameserver_data()
{
    setNameserver_data_helper<QDnsLookup::Standard>("DNS");
}

void tst_QDnsLookup::setNameserver_helper(QDnsLookup::Protocol protocol)
{
    QFETCH(QHostAddress, server);
    QElapsedTimer timer;
    timer.start();
    std::unique_ptr<QDnsLookup> lookup =
            lookupCommon(QDnsLookup::Type::A, "a-single", server, 0, protocol);
    if (!lookup)
        return;
    qDebug() << "Lookup took" << timer.elapsed() << "ms";
    QCOMPARE(lookup->error(), QDnsLookup::NoError);
    QString result = formatReply(lookup.get()).join(';');
    QCOMPARE(result, "A 192.0.2.1");
}

void tst_QDnsLookup::setNameserver()
{
    setNameserver_helper(QDnsLookup::Standard);
}

void tst_QDnsLookup::dnsOverTls_data()
{
    setNameserver_data_helper<QDnsLookup::DnsOverTls>("DNS-over-TLS");
}

void tst_QDnsLookup::dnsOverTls()
{
    setNameserver_helper(QDnsLookup::DnsOverTls);
}

void tst_QDnsLookup::bindingsAndProperties()
{
    QDnsLookup lookup;

    lookup.setType(QDnsLookup::A);
    QProperty<QDnsLookup::Type> dnsTypeProp;
    lookup.bindableType().setBinding(Qt::makePropertyBinding(dnsTypeProp));
    const QSignalSpy typeChangeSpy(&lookup, &QDnsLookup::typeChanged);

    dnsTypeProp = QDnsLookup::AAAA;
    QCOMPARE(typeChangeSpy.size(), 1);
    QCOMPARE(lookup.type(), QDnsLookup::AAAA);

    dnsTypeProp.setBinding(lookup.bindableType().makeBinding());
    lookup.setType(QDnsLookup::A);
    QCOMPARE(dnsTypeProp.value(), QDnsLookup::A);

    QProperty<QString> nameProp;
    lookup.bindableName().setBinding(Qt::makePropertyBinding(nameProp));
    const QSignalSpy nameChangeSpy(&lookup, &QDnsLookup::nameChanged);

    nameProp = QStringLiteral("a-plus-aaaa");
    QCOMPARE(nameChangeSpy.size(), 1);
    QCOMPARE(lookup.name(), QStringLiteral("a-plus-aaaa"));

    nameProp.setBinding(lookup.bindableName().makeBinding());
    lookup.setName(QStringLiteral("a-single"));
    QCOMPARE(nameProp.value(), QStringLiteral("a-single"));

    QProperty<QHostAddress> nameserverProp;
    lookup.bindableNameserver().setBinding(Qt::makePropertyBinding(nameserverProp));
    const QSignalSpy nameserverChangeSpy(&lookup, &QDnsLookup::nameserverChanged);
    const QSignalSpy nameserverPortChangeSpy(&lookup, &QDnsLookup::nameserverPortChanged);

    nameserverProp = QHostAddress::LocalHost;
    QCOMPARE(nameserverChangeSpy.size(), 1);
    QCOMPARE(nameserverPortChangeSpy.size(), 0);
    QCOMPARE(lookup.nameserver(), QHostAddress::LocalHost);

    nameserverProp.setBinding(lookup.bindableNameserver().makeBinding());
    lookup.setNameserver(QHostAddress::Any);
    QCOMPARE(nameserverProp.value(), QHostAddress::Any);
    QCOMPARE(nameserverChangeSpy.size(), 2);
    QCOMPARE(nameserverPortChangeSpy.size(), 0);

    lookup.setNameserver(QHostAddress::LocalHostIPv6, 10053);
    QCOMPARE(nameserverProp.value(), QHostAddress::LocalHostIPv6);
    QCOMPARE(nameserverChangeSpy.size(), 3);
    QCOMPARE(nameserverPortChangeSpy.size(), 1);
}

void tst_QDnsLookup::automatedBindings()
{
    QDnsLookup lookup;

    QTestPrivate::testReadWritePropertyBasics(lookup, u"aaaa"_s, u"txt"_s, "name");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QDnsLookup::name");
        return;
    }

    QTestPrivate::testReadWritePropertyBasics(lookup, QDnsLookup::AAAA, QDnsLookup::TXT, "type");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QDnsLookup::type");
        return;
    }

    QTestPrivate::testReadWritePropertyBasics(lookup, QHostAddress{QHostAddress::Any},
                                              QHostAddress{QHostAddress::LocalHost},
                                              "nameserver");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QDnsLookup::nameserver");
        return;
    }

    QTestPrivate::testReadWritePropertyBasics(lookup, quint16(123), quint16(456),
                                              "nameserverPort");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QDnsLookup::nameserverPort");
        return;
    }
}

QTEST_MAIN(tst_QDnsLookup)
#include "tst_qdnslookup.moc"
