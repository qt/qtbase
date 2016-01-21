/****************************************************************************
**
** Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtNetwork/QDnsLookup>
#include <QtNetwork/QHostAddress>

static bool waitForDone(QDnsLookup *lookup)
{
    if (lookup->isFinished())
        return true;

    QObject::connect(lookup, SIGNAL(finished()),
                     &QTestEventLoop::instance(), SLOT(exitLoop()));
    QTestEventLoop::instance().enterLoop(10);
    return !QTestEventLoop::instance().timeout();
}

class tst_QDnsLookup: public QObject
{
    Q_OBJECT

    QString domainName(const QString &input);
    QString domainNameList(const QString &input);
    QStringList domainNameListAlternatives(const QString &input);
public slots:
    void initTestCase();

private slots:
    void lookup_data();
    void lookup();
    void lookupReuse();
    void lookupAbortRetry();
};

void tst_QDnsLookup::initTestCase()
{
    QTest::addColumn<QString>("tld");
    QTest::newRow("normal") << ".test.qt-project.org";
    QTest::newRow("idn") << ".alqualond\xc3\xab.test.qt-project.org";
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

    QFETCH_GLOBAL(QString, tld);
    return input + tld;
}

QString tst_QDnsLookup::domainNameList(const QString &input)
{
    QStringList list = input.split(QLatin1Char(';'));
    QString result;
    foreach (const QString &s, list) {
        if (!result.isEmpty())
            result += ';';
        result += domainName(s);
    }
    return result;
}

QStringList tst_QDnsLookup::domainNameListAlternatives(const QString &input)
{
    QStringList alternatives = input.split('|');
    for (int i = 0; i < alternatives.length(); ++i)
        alternatives[i] = domainNameList(alternatives[i]);
    return alternatives;
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

    QTest::newRow("a-empty") << int(QDnsLookup::A) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << ""<< "" << "";
    QTest::newRow("a-notfound") << int(QDnsLookup::A) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("a-single") << int(QDnsLookup::A) << "a-single" << int(QDnsLookup::NoError) << "" << "192.0.2.1" << "" << "" << "" << "" << "";
    QTest::newRow("a-multi") << int(QDnsLookup::A) << "a-multi" << int(QDnsLookup::NoError) << "" << "192.0.2.1;192.0.2.2;192.0.2.3" << "" << "" << "" << "" << "";
    QTest::newRow("aaaa-empty") << int(QDnsLookup::AAAA) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("aaaa-notfound") << int(QDnsLookup::AAAA) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("aaaa-single") << int(QDnsLookup::AAAA) << "aaaa-single" << int(QDnsLookup::NoError) << "" << "2001:db8::1" << "" << "" << "" << "" << "";
    QTest::newRow("aaaa-multi") << int(QDnsLookup::AAAA) << "aaaa-multi" << int(QDnsLookup::NoError) << "" << "2001:db8::1;2001:db8::2;2001:db8::3" << "" << "" << "" << "" << "";

    QTest::newRow("any-empty") << int(QDnsLookup::ANY) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("any-notfound") << int(QDnsLookup::ANY) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("any-a-single") << int(QDnsLookup::ANY) << "a-single" << int(QDnsLookup::NoError) << "" << "192.0.2.1" << "" << "" << "" << ""  << "";
    QTest::newRow("any-a-plus-aaaa") << int(QDnsLookup::ANY) << "a-plus-aaaa" << int(QDnsLookup::NoError) << "" << "198.51.100.1;2001:db8::1:1" << "" << "" << "" << ""  << "";
    QTest::newRow("any-multi") << int(QDnsLookup::ANY) << "multi" << int(QDnsLookup::NoError) << "" << "198.51.100.1;198.51.100.2;198.51.100.3;2001:db8::1:1;2001:db8::1:2" << "" << "" << "" << ""  << "";

    QTest::newRow("mx-empty") << int(QDnsLookup::MX) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("mx-notfound") << int(QDnsLookup::MX) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("mx-single") << int(QDnsLookup::MX) << "mx-single" << int(QDnsLookup::NoError) << "" << "" << "10 multi" << "" << "" << "" << "";
    QTest::newRow("mx-single-cname") << int(QDnsLookup::MX) << "mx-single-cname" << int(QDnsLookup::NoError) << "" << "" << "10 cname" << "" << "" << "" << "";
    QTest::newRow("mx-multi") << int(QDnsLookup::MX) << "mx-multi" << int(QDnsLookup::NoError) << "" << "" << "10 multi;20 a-single" << "" << "" << "" << "";
    QTest::newRow("mx-multi-sameprio") << int(QDnsLookup::MX) << "mx-multi-sameprio" << int(QDnsLookup::NoError) << "" << ""
                                       << "10 multi;10 a-single|"
                                          "10 a-single;10 multi" << "" << "" << "" << "";

    QTest::newRow("ns-empty") << int(QDnsLookup::NS) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("ns-notfound") << int(QDnsLookup::NS) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("ns-single") << int(QDnsLookup::NS) << "ns-single" << int(QDnsLookup::NoError) << "" << "" << "" << "ns11.cloudns.net." << "" << "" << "";
    QTest::newRow("ns-multi") << int(QDnsLookup::NS) << "ns-multi" << int(QDnsLookup::NoError) << "" << "" << "" << "ns11.cloudns.net.;ns12.cloudns.net." << "" << "" << "";

    QTest::newRow("ptr-empty") << int(QDnsLookup::PTR) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("ptr-notfound") << int(QDnsLookup::PTR) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
#if 0
    // temporarily disabled since the new hosting provider can't insert
    // PTR records outside of the in-addr.arpa zone
    QTest::newRow("ptr-single") << int(QDnsLookup::PTR) << "ptr-single" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "a-single" << "" << "";
#endif

    QTest::newRow("srv-empty") << int(QDnsLookup::SRV) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("srv-notfound") << int(QDnsLookup::SRV) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("srv-single") << int(QDnsLookup::SRV) << "_echo._tcp.srv-single" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "5 0 7 multi" << "";
    QTest::newRow("srv-prio") << int(QDnsLookup::SRV) << "_echo._tcp.srv-prio" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "1 0 7 multi;2 0 7 a-plus-aaaa" << "";
    QTest::newRow("srv-weighted") << int(QDnsLookup::SRV) << "_echo._tcp.srv-weighted" << int(QDnsLookup::NoError) << "" << "" << "" << "" << ""
                                  << "5 75 7 multi;5 25 7 a-plus-aaaa|"
                                     "5 25 7 a-plus-aaaa;5 75 7 multi" << "";
    QTest::newRow("srv-multi") << int(QDnsLookup::SRV) << "_echo._tcp.srv-multi" << int(QDnsLookup::NoError) << "" << "" << "" << "" << ""
                               << "1 50 7 multi;2 50 7 a-single;2 50 7 aaaa-single;3 50 7 a-multi|"
                                  "1 50 7 multi;2 50 7 aaaa-single;2 50 7 a-single;3 50 7 a-multi" << "";

    QTest::newRow("txt-empty") << int(QDnsLookup::TXT) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("txt-notfound") << int(QDnsLookup::TXT) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << "";
    QTest::newRow("txt-single") << int(QDnsLookup::TXT) << "txt-single" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "" << "Hello";
    QTest::newRow("txt-multi-onerr") << int(QDnsLookup::TXT) << "txt-multi-onerr" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << ""
                                     << QString::fromLatin1("Hello\0World", sizeof("Hello\0World") - 1);
    QTest::newRow("txt-multi-multirr") << int(QDnsLookup::TXT) << "txt-multi-multirr" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "" << "Hello;World";
}

static QByteArray msgDnsLookup(QDnsLookup::Error actualError,
                               int expectedError,
                               const QString &domain,
                               const QString &cname,
                               const QString &host,
                               const QString &srv,
                               const QString &mx,
                               const QString &ns,
                               const QString &ptr,
                               const QString &errorString)
{
    QString result;
    QTextStream str(&result);
    str << "Actual error: " << actualError;
    if (!errorString.isEmpty())
        str << " (" << errorString << ')';
    str << ", expected: " << expectedError;
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
    QVERIFY(waitForDone(&lookup));
    QVERIFY(lookup.isFinished());

#if defined(Q_OS_ANDROID)
    if (lookup.errorString() == QStringLiteral("Not yet supported on Android"))
        QEXPECT_FAIL("", "Not yet supported on Android", Abort);
#endif

    QVERIFY2(int(lookup.error()) == error,
             msgDnsLookup(lookup.error(), error, domain, cname, host, srv, mx, ns, ptr, lookup.errorString()));
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

void tst_QDnsLookup::lookupReuse()
{
    QDnsLookup lookup;

    // first lookup
    lookup.setType(QDnsLookup::A);
    lookup.setName(domainName("a-single"));
    lookup.lookup();
    QVERIFY(waitForDone(&lookup));
    QVERIFY(lookup.isFinished());

#if defined(Q_OS_ANDROID)
    if (lookup.errorString() == QStringLiteral("Not yet supported on Android"))
        QEXPECT_FAIL("", "Not yet supported on Android", Abort);
#endif

    QCOMPARE(int(lookup.error()), int(QDnsLookup::NoError));
    QVERIFY(!lookup.hostAddressRecords().isEmpty());
    QCOMPARE(lookup.hostAddressRecords().first().name(), domainName("a-single"));
    QCOMPARE(lookup.hostAddressRecords().first().value(), QHostAddress("192.0.2.1"));

    // second lookup
    lookup.setType(QDnsLookup::AAAA);
    lookup.setName(domainName("aaaa-single"));
    lookup.lookup();
    QVERIFY(waitForDone(&lookup));
    QVERIFY(lookup.isFinished());
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
    QVERIFY(waitForDone(&lookup));
    QVERIFY(lookup.isFinished());
    QCOMPARE(int(lookup.error()), int(QDnsLookup::OperationCancelledError));
    QVERIFY(lookup.hostAddressRecords().isEmpty());

    // retry a different lookup
    lookup.setType(QDnsLookup::AAAA);
    lookup.setName(domainName("aaaa-single"));
    lookup.lookup();
    QVERIFY(waitForDone(&lookup));
    QVERIFY(lookup.isFinished());

#if defined(Q_OS_ANDROID)
    if (lookup.errorString() == QStringLiteral("Not yet supported on Android"))
        QEXPECT_FAIL("", "Not yet supported on Android", Abort);
#endif

    QCOMPARE(int(lookup.error()), int(QDnsLookup::NoError));
    QVERIFY(!lookup.hostAddressRecords().isEmpty());
    QCOMPARE(lookup.hostAddressRecords().first().name(), domainName("aaaa-single"));
    QCOMPARE(lookup.hostAddressRecords().first().value(), QHostAddress("2001:db8::1"));
}

QTEST_MAIN(tst_QDnsLookup)
#include "tst_qdnslookup.moc"
