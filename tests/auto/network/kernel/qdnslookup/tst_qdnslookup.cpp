// Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QTest>
#include <QSignalSpy>
#include <QtTest/private/qpropertytesthelper_p.h>

#include <QtNetwork/QDnsLookup>

#include <QtCore/QRandomGenerator>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

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

    const QString normalDomain = u".test.qt-project.org"_s;
    const QString idnDomain = u".alqualondë.test.qt-project.org"_s;
    bool usingIdnDomain = false;
    bool dnsServersMustWork = false;

    QString domainName(const QString &input);
    QString domainNameList(const QString &input);
    QStringList domainNameListAlternatives(const QString &input);
public slots:
    void initTestCase();

private slots:
    void lookupLocalhost();
    void lookupRoot();
    void lookup_data();
    void lookup();
    void lookupIdn_data() { lookup_data(); }
    void lookupIdn();

    void lookupReuse();
    void lookupAbortRetry();
    void setNameserverLoopback();
    void setNameserver_data();
    void setNameserver();
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

static QList<QHostAddress> systemNameservers()
{
    QList<QHostAddress> result;

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
    QFile f("/etc/resolv.conf");
    if (!f.open(QIODevice::ReadOnly))
        return result;

    while (!f.atEnd()) {
        static const char command[] = "nameserver";
        QByteArray line = f.readLine().simplified();
        if (!line.startsWith(command))
            continue;

        QString addr = QLatin1StringView(line).mid(sizeof(command));
        result.emplaceBack(addr);
    }
#endif

    return result;
}

static QList<QHostAddress> globalPublicNameservers()
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

    QList<QHostAddress> result;
    QRandomGenerator &rng = *QRandomGenerator::system();
    for (auto name : candidates) {
        // check the candidates for reachability
        QHostAddress addr{QLatin1StringView(name)};
        quint16 id = quint16(rng());
        QByteArray data(preparedDnsQuery, sizeof(preparedDnsQuery));
        char *ptr = data.data();
        qToBigEndian(id, ptr);

        QUdpSocket socket;
        socket.connectToHost(addr, 53);
        if (socket.waitForConnected(1))
            socket.write(data);

        if (!socket.waitForReadyRead(1000)) {
            qDebug() << addr << "discarded:" << socket.errorString();
            continue;
        }

        QNetworkDatagram dgram = socket.receiveDatagram();
        if (!dgram.isValid()) {
            qDebug() << addr << "discarded:" << socket.errorString();
            continue;
        }

        data = dgram.data();
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

void tst_QDnsLookup::lookupLocalhost()
{
    QDnsLookup lookup(QDnsLookup::Type::A, u"localhost"_s);
    lookup.lookup();
    QTRY_VERIFY_WITH_TIMEOUT(lookup.isFinished(), Timeout);
    QCOMPARE(lookup.error(), QDnsLookup::NoError);

    QList<QDnsHostAddressRecord> hosts = lookup.hostAddressRecords();
    QCOMPARE(hosts.size(), 1);
    QCOMPARE(hosts.at(0).value(), QHostAddress::LocalHost);
    QVERIFY2(hosts.at(0).name().startsWith(lookup.name()),
             qPrintable(hosts.at(0).name()));
}

void tst_QDnsLookup::lookupRoot()
{
#ifdef Q_OS_WIN
    QSKIP("This test fails on Windows as it seems to treat the lookup as a local one.");
#else
    QDnsLookup lookup(QDnsLookup::Type::NS, u""_s);
    lookup.lookup();
    QTRY_VERIFY_WITH_TIMEOUT(lookup.isFinished(), Timeout);
    QCOMPARE(lookup.error(), QDnsLookup::NoError);

    const QList<QDnsDomainNameRecord> servers = lookup.nameServerRecords();
    QVERIFY(!servers.isEmpty());
    for (const QDnsDomainNameRecord &ns : servers) {
        QCOMPARE(ns.name(), QString());
        QVERIFY(ns.value().endsWith(".root-servers.net"));
    }
#endif
}

void tst_QDnsLookup::lookup_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<QString>("domain");
    QTest::addColumn<int>("error");
    QTest::addColumn<QString>("cname");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("mx");
    QTest::addColumn<QString>("ns");
    QTest::addColumn<QString>("ptr");
    QTest::addColumn<QString>("srv");
    QTest::addColumn<QString>("txt");

    QTest::newRow("a-notfound") << int(QDnsLookup::A) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("a-single") << int(QDnsLookup::A) << "a-single" << int(QDnsLookup::NoError) << "" << "192.0.2.1" << "" << "" << "" << "" << "";
    QTest::newRow("a-multi") << int(QDnsLookup::A) << "a-multi" << int(QDnsLookup::NoError) << "" << "192.0.2.1;192.0.2.2;192.0.2.3" << "" << "" << "" << "" << "";
    QTest::newRow("aaaa-notfound") << int(QDnsLookup::AAAA) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("aaaa-single") << int(QDnsLookup::AAAA) << "aaaa-single" << int(QDnsLookup::NoError) << "" << "2001:db8::1" << "" << "" << "" << "" << "";
    QTest::newRow("aaaa-multi") << int(QDnsLookup::AAAA) << "aaaa-multi" << int(QDnsLookup::NoError) << "" << "2001:db8::1;2001:db8::2;2001:db8::3" << "" << "" << "" << "" << "";

    QTest::newRow("any-notfound") << int(QDnsLookup::ANY) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("any-a-single") << int(QDnsLookup::ANY) << "a-single" << int(QDnsLookup::NoError) << "" << "192.0.2.1" << "" << "" << "" << ""  << "";
    QTest::newRow("any-a-plus-aaaa") << int(QDnsLookup::ANY) << "a-plus-aaaa" << int(QDnsLookup::NoError) << "" << "198.51.100.1;2001:db8::1:1" << "" << "" << "" << ""  << "";
    QTest::newRow("any-multi") << int(QDnsLookup::ANY) << "multi" << int(QDnsLookup::NoError) << "" << "198.51.100.1;198.51.100.2;198.51.100.3;2001:db8::1:1;2001:db8::1:2" << "" << "" << "" << ""  << "";

    QTest::newRow("mx-notfound") << int(QDnsLookup::MX) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("mx-single") << int(QDnsLookup::MX) << "mx-single" << int(QDnsLookup::NoError) << "" << "" << "10 multi" << "" << "" << "" << "";
    QTest::newRow("mx-single-cname") << int(QDnsLookup::MX) << "mx-single-cname" << int(QDnsLookup::NoError) << "" << "" << "10 cname" << "" << "" << "" << "";
    QTest::newRow("mx-multi") << int(QDnsLookup::MX) << "mx-multi" << int(QDnsLookup::NoError) << "" << "" << "10 multi;20 a-single" << "" << "" << "" << "";
    QTest::newRow("mx-multi-sameprio") << int(QDnsLookup::MX) << "mx-multi-sameprio" << int(QDnsLookup::NoError) << "" << ""
                                       << "10 multi;10 a-single|"
                                          "10 a-single;10 multi" << "" << "" << "" << "";

    QTest::newRow("ns-notfound") << int(QDnsLookup::NS) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("ns-single") << int(QDnsLookup::NS) << "ns-single" << int(QDnsLookup::NoError) << "" << "" << "" << "ns11.cloudns.net." << "" << "" << "";
    QTest::newRow("ns-multi") << int(QDnsLookup::NS) << "ns-multi" << int(QDnsLookup::NoError) << "" << "" << "" << "ns11.cloudns.net.;ns12.cloudns.net." << "" << "" << "";

    QTest::newRow("ptr-notfound") << int(QDnsLookup::PTR) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
#if 0
    // temporarily disabled since the new hosting provider can't insert
    // PTR records outside of the in-addr.arpa zone
    QTest::newRow("ptr-single") << int(QDnsLookup::PTR) << "ptr-single" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "a-single" << "" << "";
#endif

    QTest::newRow("srv-notfound") << int(QDnsLookup::SRV) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("srv-single") << int(QDnsLookup::SRV) << "_echo._tcp.srv-single" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "5 0 7 multi" << "";
    QTest::newRow("srv-prio") << int(QDnsLookup::SRV) << "_echo._tcp.srv-prio" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "1 0 7 multi;2 0 7 a-plus-aaaa" << "";
    QTest::newRow("srv-weighted") << int(QDnsLookup::SRV) << "_echo._tcp.srv-weighted" << int(QDnsLookup::NoError) << "" << "" << "" << "" << ""
                                  << "5 75 7 multi;5 25 7 a-plus-aaaa|"
                                     "5 25 7 a-plus-aaaa;5 75 7 multi" << "";
    QTest::newRow("srv-multi") << int(QDnsLookup::SRV) << "_echo._tcp.srv-multi" << int(QDnsLookup::NoError) << "" << "" << "" << "" << ""
                               << "1 50 7 multi;2 50 7 a-single;2 50 7 aaaa-single;3 50 7 a-multi|"
                                  "1 50 7 multi;2 50 7 aaaa-single;2 50 7 a-single;3 50 7 a-multi" << "";

    QTest::newRow("txt-notfound") << int(QDnsLookup::TXT) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("txt-single") << int(QDnsLookup::TXT) << "txt-single" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "" << "Hello";
    QTest::newRow("txt-multi-onerr") << int(QDnsLookup::TXT) << "txt-multi-onerr" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << ""
                                     << QString::fromLatin1("Hello\0World", sizeof("Hello\0World") - 1);
    QTest::newRow("txt-multi-multirr") << int(QDnsLookup::TXT) << "txt-multi-multirr" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "" << "Hello;World";
}

void tst_QDnsLookup::lookup()
{
    QFETCH(int, type);
    QFETCH(QString, domain);
    QFETCH(int, error);
    QFETCH(QString, cname);
    QFETCH(QString, host);
    QFETCH(QString, mx);
    QFETCH(QString, ns);
    QFETCH(QString, ptr);
    QFETCH(QString, srv);
    QFETCH(QString, txt);

    // transform the inputs
    domain = domainName(domain);
    cname = domainName(cname);
    ns = domainNameList(ns);
    ptr = domainNameList(ptr);

    // SRV and MX have reply entries that can change order
    // and we can't sort
    QStringList mx_alternatives = domainNameListAlternatives(mx);
    QStringList srv_alternatives = domainNameListAlternatives(srv);

    QDnsLookup lookup;
    lookup.setType(static_cast<QDnsLookup::Type>(type));
    lookup.setName(domain);
    lookup.lookup();
    QTRY_VERIFY_WITH_TIMEOUT(lookup.isFinished(), Timeout);

    auto extraErrorMsg = [&] () {
        QString result;
        QTextStream str(&result);
        str << "Actual error: " << lookup.error();
        if (QString errorString = lookup.errorString(); !errorString.isEmpty())
            str << " (" << errorString << ')';
        str << ", expected: " << error;
        str << ", domain: " << domain;
        if (!cname.isEmpty())
            str << ", cname: " << cname;
        str << ", host: " << host;
        if (!srv.isEmpty())
            str << " server: " << srv;
        if (!mx.isEmpty())
            str << " mx: " << mx;
        if (!ns.isEmpty())
            str << " ns: " << ns;
        if (!ptr.isEmpty())
            str << " ptr: " << ptr;
        return result.toLocal8Bit();
    };

    if (!dnsServersMustWork && (lookup.error() == QDnsLookup::ServerFailureError
                                || lookup.error() == QDnsLookup::ServerRefusedError
                                || lookup.error() == QDnsLookup::TimeoutError)) {
        // It's not a QDnsLookup problem if the server refuses to answer the query.
        // This happens for queries of type ANY through Dnsmasq, for example.
        qWarning("Server refused or was unable to answer query; %s", extraErrorMsg().constData());
        return;
    }

    QVERIFY2(int(lookup.error()) == error, extraErrorMsg());
    if (error == QDnsLookup::NoError)
        QVERIFY(lookup.errorString().isEmpty());
    QCOMPARE(int(lookup.type()), type);
    QCOMPARE(lookup.name(), domain);

    // canonical names
    if (!cname.isEmpty()) {
        QVERIFY(!lookup.canonicalNameRecords().isEmpty());
        const QDnsDomainNameRecord cnameRecord = lookup.canonicalNameRecords().first();
        QCOMPARE(cnameRecord.name(), domain);
        QCOMPARE(cnameRecord.value(), cname);
    } else {
        QVERIFY(lookup.canonicalNameRecords().isEmpty());
    }

    // host addresses
    const QString hostName = cname.isEmpty() ? domain : cname;
    QStringList addresses;
    foreach (const QDnsHostAddressRecord &record, lookup.hostAddressRecords()) {
        //reply may include A & AAAA records for nameservers, ignore them and only look at records matching the query
        if (record.name() == hostName)
            addresses << record.value().toString().toLower();
    }
    addresses.sort();
    QCOMPARE(addresses.join(';'), host);

    // mail exchanges
    QStringList mailExchanges;
    foreach (const QDnsMailExchangeRecord &record, lookup.mailExchangeRecords()) {
        QCOMPARE(record.name(), domain);
        mailExchanges << QString::number(record.preference()) + QLatin1Char(' ') + record.exchange();
    }
    QVERIFY2(mx_alternatives.contains(mailExchanges.join(';')),
             qPrintable("Actual: " + mailExchanges.join(';') + "\nExpected one of:\n" + mx_alternatives.join('\n')));

    // name servers
    QStringList nameServers;
    foreach (const QDnsDomainNameRecord &record, lookup.nameServerRecords()) {
        //reply may include NS records for authoritative nameservers, ignore them and only look at records matching the query
        if (record.name() == domain)
            nameServers << record.value();
    }
    nameServers.sort();
    QCOMPARE(nameServers.join(';'), ns);

    // pointers
    if (!ptr.isEmpty()) {
        QVERIFY(!lookup.pointerRecords().isEmpty());
        const QDnsDomainNameRecord ptrRecord = lookup.pointerRecords().first();
        QCOMPARE(ptrRecord.name(), domain);
        QCOMPARE(ptrRecord.value(), ptr);
    } else {
        QVERIFY(lookup.pointerRecords().isEmpty());
    }

    // services
    QStringList services;
    foreach (const QDnsServiceRecord &record, lookup.serviceRecords()) {
        QCOMPARE(record.name(), domain);
        services << (QString::number(record.priority()) + QLatin1Char(' ')
                     + QString::number(record.weight()) + QLatin1Char(' ')
                     + QString::number(record.port()) + QLatin1Char(' ') + record.target());
    }
    QVERIFY2(srv_alternatives.contains(services.join(';')),
             qPrintable("Actual: " + services.join(';') + "\nExpected one of:\n" + srv_alternatives.join('\n')));

    // text
    QStringList texts;
    foreach (const QDnsTextRecord &record, lookup.textRecords()) {
        QCOMPARE(record.name(), domain);
        QString text;
        foreach (const QByteArray &ba, record.values()) {
            if (!text.isEmpty())
                text += '\0';
            text += QString::fromLatin1(ba);
        }
        texts << text;
    }
    texts.sort();
    QCOMPARE(texts.join(';'), txt);
}

void tst_QDnsLookup::lookupIdn()
{
    usingIdnDomain = true;
    lookup();
    usingIdnDomain = false;
}

void tst_QDnsLookup::lookupReuse()
{
    QDnsLookup lookup;

    // first lookup
    lookup.setType(QDnsLookup::A);
    lookup.setName(domainName("a-single"));
    lookup.lookup();
    QTRY_VERIFY_WITH_TIMEOUT(lookup.isFinished(), Timeout);

    QCOMPARE(int(lookup.error()), int(QDnsLookup::NoError));
    QVERIFY(!lookup.hostAddressRecords().isEmpty());
    QCOMPARE(lookup.hostAddressRecords().first().name(), domainName("a-single"));
    QCOMPARE(lookup.hostAddressRecords().first().value(), QHostAddress("192.0.2.1"));

    // second lookup
    lookup.setType(QDnsLookup::AAAA);
    lookup.setName(domainName("aaaa-single"));
    lookup.lookup();
    QTRY_VERIFY_WITH_TIMEOUT(lookup.isFinished(), Timeout);
    QCOMPARE(int(lookup.error()), int(QDnsLookup::NoError));
    QVERIFY(!lookup.hostAddressRecords().isEmpty());
    QCOMPARE(lookup.hostAddressRecords().first().name(), domainName("aaaa-single"));
    QCOMPARE(lookup.hostAddressRecords().first().value(), QHostAddress("2001:db8::1"));
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
    reply[2] = 0x80;    // header->qr = true;
    reply[3] = 3;       // header->rcode = NXDOMAIN;
    server.writeDatagram(dgram.makeReply(reply));
    server.close();

    // now check that the QDnsLookup finished
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(spy.size(), 1);
    QCOMPARE(lookup.error(), QDnsLookup::NotFoundError);
}

void tst_QDnsLookup::setNameserver_data()
{
    static QList<QHostAddress> servers = systemNameservers() + globalPublicNameservers();
    QTest::addColumn<QHostAddress>("server");

    if (servers.isEmpty()) {
        QSKIP("No reachable DNS servers were found");
    } else {
        for (const QHostAddress &h : std::as_const(servers))
            QTest::addRow("%s", qUtf8Printable(h.toString())) << h;
    }
}

void tst_QDnsLookup::setNameserver()
{
    QFETCH(QHostAddress, server);
    QDnsLookup lookup;
    lookup.setNameserver(server);

    lookup.setType(QDnsLookup::Type::A);
    lookup.setName(domainName("a-single"));
    lookup.lookup();

    QTRY_VERIFY_WITH_TIMEOUT(lookup.isFinished(), Timeout);
    QCOMPARE(int(lookup.error()), int(QDnsLookup::NoError));
    QVERIFY(!lookup.hostAddressRecords().isEmpty());
    QCOMPARE(lookup.hostAddressRecords().first().name(), domainName("a-single"));
    QCOMPARE(lookup.hostAddressRecords().first().value(), QHostAddress("192.0.2.1"));
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
