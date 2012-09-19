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

private slots:
    void lookup_data();
    void lookup();
    void lookupReuse();
    void lookupAbortRetry();
};

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
    QTest::newRow("a-notfound") << int(QDnsLookup::A) << "invalid." << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("a-idn") << int(QDnsLookup::A) << QString::fromUtf8("alqualondë.troll.no") << int(QDnsLookup::NoError) << "alqualonde.troll.no" << "10.3.3.55" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("a-single") << int(QDnsLookup::A) << "lupinella.troll.no" << int(QDnsLookup::NoError) << "" << "10.3.4.6" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("a-multi") << int(QDnsLookup::A) << "multi.dev.troll.no" << int(QDnsLookup::NoError) << "" << "1.2.3.4 1.2.3.5 10.3.3.31" << "" << "" << "" << "" << QByteArray();

    QTest::newRow("aaaa-empty") << int(QDnsLookup::AAAA) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("aaaa-notfound") << int(QDnsLookup::AAAA) << "invalid." << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("aaaa-single") << int(QDnsLookup::AAAA) << "dns6-test-dev.troll.no" << int(QDnsLookup::NoError) << "" << "2001:470:1f01:115::10" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("aaaa-multi") << int(QDnsLookup::AAAA) << "multi-dns6-test-dev.troll.no" << int(QDnsLookup::NoError) << "" << "2001:470:1f01:115::11 2001:470:1f01:115::12" << "" << "" << "" << "" << QByteArray();

    QTest::newRow("any-empty") << int(QDnsLookup::ANY) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("any-notfound") << int(QDnsLookup::ANY) << "invalid." << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("any-ascii") << int(QDnsLookup::ANY) << "fluke.troll.no" << int(QDnsLookup::NoError) << "" << "10.3.3.31" << "" << "" << "" << ""  << QByteArray();

    QTest::newRow("mx-empty") << int(QDnsLookup::MX) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("mx-notfound") << int(QDnsLookup::MX) << "invalid." << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("mx-ascii") << int(QDnsLookup::MX) << "troll.no" << int(QDnsLookup::NoError) << "" << "" << "10 smtp.trolltech.com" << "" << "" << "" << QByteArray();
#if 0
    // FIXME: we need an IDN MX record in the troll.no domain
    QTest::newRow("mx-idn") << int(QDnsLookup::MX) << QString::fromUtf8("råkat.se") << int(QDnsLookup::NoError) << "" << "" << "10 mail.cdr.se" << "" << "" << "" << QByteArray();
#endif

    QTest::newRow("ns-empty") << int(QDnsLookup::NS) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("ns-notfound") << int(QDnsLookup::NS) << "invalid." << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("ns-ascii") << int(QDnsLookup::NS) << "troll.no" << int(QDnsLookup::NoError) << "" << "" << "" << "ns-0.trolltech.net ns-1.trolltech.net" << "" << "" << QByteArray();

    QTest::newRow("ptr-empty") << int(QDnsLookup::PTR) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("ptr-notfound") << int(QDnsLookup::PTR) << "invalid." << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    // FIXME: we need PTR records in the troll.no domain
    QTest::newRow("ptr-ascii") << int(QDnsLookup::PTR) << "8.8.8.8.in-addr.arpa" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "google-public-dns-a.google.com" << "" << QByteArray();

    QTest::newRow("srv-empty") << int(QDnsLookup::SRV) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("srv-notfound") << int(QDnsLookup::SRV) << "invalid." << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
#if 0
    // FIXME: we need SRV records in the troll.no domain
    QTest::newRow("srv-idn") << int(QDnsLookup::SRV) << QString::fromUtf8("_xmpp-client._tcp.råkat.se") << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "5 0 5224 jabber.cdr.se" << QByteArray();
#endif

    QTest::newRow("txt-empty") << int(QDnsLookup::TXT) << "" << int(QDnsLookup::InvalidRequestError) << "" << "" << "" << "" << "" << "" << QByteArray();
    QTest::newRow("txt-notfound") << int(QDnsLookup::TXT) << "invalid." << int(QDnsLookup::NotFoundError) << "" << "" << "" << "" << "" << "" << QByteArray();
    // FIXME: we need TXT records in the troll.no domain
    QTest::newRow("txt-ascii") << int(QDnsLookup::TXT) << "gmail.com" << int(QDnsLookup::NoError) << "" << "" << "" << "" << "" << "" << QByteArray("v=spf1 redirect=_spf.google.com");
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
    QCOMPARE(addresses.join(' '), host);

    // mail exchanges
    QStringList mailExchanges;
    foreach (const QDnsMailExchangeRecord &record, lookup.mailExchangeRecords()) {
        QCOMPARE(record.name(), domain);
        mailExchanges << QString("%1 %2").arg(QString::number(record.preference()), record.exchange());
    }
    QCOMPARE(mailExchanges.join(' '), mx);

    // name servers
    QStringList nameServers;
    foreach (const QDnsDomainNameRecord &record, lookup.nameServerRecords()) {
        //reply may include NS records for authoritative nameservers, ignore them and only look at records matching the query
        if (record.name() == domain)
            nameServers << record.value();
    }
    nameServers.sort();
    QCOMPARE(nameServers.join(' '), ns);

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
    QCOMPARE(services.join(' '), srv);

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
    lookup.setName("lupinella.troll.no");
    lookup.lookup();
    QVERIFY(waitForDone(&lookup));
    QVERIFY(lookup.isFinished());
    QCOMPARE(int(lookup.error()), int(QDnsLookup::NoError));
    QVERIFY(!lookup.hostAddressRecords().isEmpty());
    QCOMPARE(lookup.hostAddressRecords().first().name(), QString("lupinella.troll.no"));
    QCOMPARE(lookup.hostAddressRecords().first().value(), QHostAddress("10.3.4.6"));

    // second lookup
    lookup.setType(QDnsLookup::AAAA);
    lookup.setName("dns6-test-dev.troll.no");
    lookup.lookup();
    QVERIFY(waitForDone(&lookup));
    QVERIFY(lookup.isFinished());
    QCOMPARE(int(lookup.error()), int(QDnsLookup::NoError));
    QVERIFY(!lookup.hostAddressRecords().isEmpty());
    QCOMPARE(lookup.hostAddressRecords().first().name(), QString("dns6-test-dev.troll.no"));
    QCOMPARE(lookup.hostAddressRecords().first().value(), QHostAddress("2001:470:1f01:115::10"));
}


void tst_QDnsLookup::lookupAbortRetry()
{
    QDnsLookup lookup;

    // try and abort the lookup
    lookup.setType(QDnsLookup::A);
    lookup.setName("lupinella.troll.no");
    lookup.lookup();
    lookup.abort();
    QVERIFY(waitForDone(&lookup));
    QVERIFY(lookup.isFinished());
    QCOMPARE(int(lookup.error()), int(QDnsLookup::OperationCancelledError));
    QVERIFY(lookup.hostAddressRecords().isEmpty());

    // retry a different lookup
    lookup.setType(QDnsLookup::AAAA);
    lookup.setName("dns6-test-dev.troll.no");
    lookup.lookup();
    QVERIFY(waitForDone(&lookup));
    QVERIFY(lookup.isFinished());
    QCOMPARE(int(lookup.error()), int(QDnsLookup::NoError));
    QVERIFY(!lookup.hostAddressRecords().isEmpty());
    QCOMPARE(lookup.hostAddressRecords().first().name(), QString("dns6-test-dev.troll.no"));
    QCOMPARE(lookup.hostAddressRecords().first().value(), QHostAddress("2001:470:1f01:115::10"));
}

QTEST_MAIN(tst_QDnsLookup)
#include "tst_qdnslookup.moc"
