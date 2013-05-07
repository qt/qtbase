/****************************************************************************
**
** Copyright (C) 2012 Jeremy Lainé <jeremy.laine@m4x.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
    QTest::newRow("normal") << ".test.macieira.org";
    QTest::newRow("idn") << ".alqualond\xc3\xab.test.macieira.org";
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
    QTest::addColumn<QByteArray>("txt");

    QTest::newRow("a-empty") << int(QDnsLookup::A) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << ""<< "" << QByteArray();
    QTest::newRow("a-notfound") << int(QDnsLookup::A) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("a-single") << int(QDnsLookup::A) << "a-single" << int(QDnsLookup::NoError) << "" << "192.0.2.1" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("a-multi") << int(QDnsLookup::A) << "a-multi" << int(QDnsLookup::NoError) << "" << "192.0.2.1;192.0.2.2;192.0.2.3" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("aaaa-empty") << int(QDnsLookup::AAAA) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("aaaa-notfound") << int(QDnsLookup::AAAA) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("aaaa-single") << int(QDnsLookup::AAAA) << "aaaa-single" << int(QDnsLookup::NoError) << "" << "2001:db8::1" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("aaaa-multi") << int(QDnsLookup::AAAA) << "aaaa-multi" << int(QDnsLookup::NoError) << "" << "2001:db8::1;2001:db8::2;2001:db8::3" << "" << "" << "" << "" << QByteArray();

    QTest::newRow("any-empty") << int(QDnsLookup::ANY) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("any-notfound") << int(QDnsLookup::ANY) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("any-a-single") << int(QDnsLookup::ANY) << "a-single" << int(QDnsLookup::NoError) << "" << "192.0.2.1" << "" << "" << "" << ""  << QByteArray();
    QTest::newRow("any-a-plus-aaaa") << int(QDnsLookup::ANY) << "a-plus-aaaa" << int(QDnsLookup::NoError) << "" << "198.51.100.1;2001:db8::1:1" << "" << "" << "" << ""  << QByteArray();
    QTest::newRow("any-multi") << int(QDnsLookup::ANY) << "multi" << int(QDnsLookup::NoError) << "" << "198.51.100.1;198.51.100.2;198.51.100.3;2001:db8::1:1;2001:db8::1:2" << "" << "" << "" << ""  << QByteArray();

    QTest::newRow("mx-empty") << int(QDnsLookup::MX) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("mx-notfound") << int(QDnsLookup::MX) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("mx-single") << int(QDnsLookup::MX) << "mx-single" << int(QDnsLookup::NoError) << "" << "" << "10 multi" << "" << "" << "" << QByteArray();
    QTest::newRow("mx-single-cname") << int(QDnsLookup::MX) << "mx-single-cname" << int(QDnsLookup::NoError) << "" << "" << "10 cname" << "" << "" << "" << QByteArray();
    QTest::newRow("mx-multi") << int(QDnsLookup::MX) << "mx-multi" << int(QDnsLookup::NoError) << "" << "" << "10 multi;20 a-single" << "" << "" << "" << QByteArray();

    QTest::newRow("ns-empty") << int(QDnsLookup::NS) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("ns-notfound") << int(QDnsLookup::NS) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("ns-single") << int(QDnsLookup::NS) << "ns-single" << int(QDnsLookup::NoError) << "" << "" << "" << "ns3.macieira.info." << "" << "" << QByteArray();
    QTest::newRow("ns-multi") << int(QDnsLookup::NS) << "ns-multi" << int(QDnsLookup::NoError) << "" << "" << "" << "gondolin.macieira.info.;ns3.macieira.info." << "" << "" << QByteArray();

    QTest::newRow("ptr-empty") << int(QDnsLookup::PTR) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("ptr-notfound") << int(QDnsLookup::PTR) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("ptr-single") << int(QDnsLookup::PTR) << "ptr-single" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "a-single" << "" << QByteArray();

    QTest::newRow("srv-empty") << int(QDnsLookup::SRV) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("srv-notfound") << int(QDnsLookup::SRV) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("srv-single") << int(QDnsLookup::SRV) << "_echo._tcp.srv-single" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "5 0 7 multi" << QByteArray();
    QTest::newRow("srv-prio") << int(QDnsLookup::SRV) << "_echo._tcp.srv-prio" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "1 0 7 multi;2 0 7 a-plus-aaaa" << QByteArray();

    QTest::newRow("txt-empty") << int(QDnsLookup::TXT) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("txt-notfound") << int(QDnsLookup::TXT) << "invalid.invalid" << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("txt-single") << int(QDnsLookup::TXT) << "txt-single" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "" << QByteArray("Hello");
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
    QFETCH(QByteArray, txt);

    // transform the inputs
    domain = domainName(domain);
    cname = domainName(cname);
    mx = domainNameList(mx);
    ns = domainNameList(ns);
    ptr = domainNameList(ptr);
    srv = domainNameList(srv);

    QDnsLookup lookup;
    lookup.setType(static_cast<QDnsLookup::Type>(type));
    lookup.setName(domain);
    lookup.lookup();
    QVERIFY(waitForDone(&lookup));
    QVERIFY(lookup.isFinished());
    QVERIFY2(int(lookup.error()) == error, qPrintable(lookup.errorString()));
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
        mailExchanges << QString("%1 %2").arg(QString::number(record.preference()), record.exchange());
    }
    QCOMPARE(mailExchanges.join(';'), mx);

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
        services << QString("%1 %2 %3 %4").arg(
                QString::number(record.priority()),
                QString::number(record.weight()),
                QString::number(record.port()),
                record.target());
    }
    QCOMPARE(services.join(';'), srv);

    // text
    if (!txt.isEmpty()) {
        QVERIFY(!lookup.textRecords().isEmpty());
        const QDnsTextRecord firstRecord = lookup.textRecords().first();
        QCOMPARE(firstRecord.name(), domain);
        QCOMPARE(firstRecord.values().size(), 1);
        QCOMPARE(firstRecord.values().first(), txt);
    } else {
        QVERIFY(lookup.textRecords().isEmpty());
    }
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
    QCOMPARE(int(lookup.error()), int(QDnsLookup::NoError));
    QVERIFY(!lookup.hostAddressRecords().isEmpty());
    QCOMPARE(lookup.hostAddressRecords().first().name(), domainName("aaaa-single"));
    QCOMPARE(lookup.hostAddressRecords().first().value(), QHostAddress("2001:db8::1"));
}

QTEST_MAIN(tst_QDnsLookup)
#include "tst_qdnslookup.moc"
