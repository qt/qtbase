// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/qdatetime.h>
#include <QtCore/qdir.h>
#include <QtCore/qlist.h>
#include <QtCore/qpair.h>
#include <QtCore/qurl.h>

#include <QtNetwork/qhttpheaders.h>
#include <QtNetwork/private/qhstsstore_p.h>
#include <QtNetwork/private/qhsts_p.h>

QT_USE_NAMESPACE

class tst_QHsts : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSingleKnownHost_data();
    void testSingleKnownHost();
    void testMultilpeKnownHosts();
    void testPolicyExpiration();
    void testSTSHeaderParser();
    void testStore();
};

void tst_QHsts::testSingleKnownHost_data()
{
    QTest::addColumn<QUrl>("knownHost");
    QTest::addColumn<QDateTime>("policyExpires");
    QTest::addColumn<bool>("includeSubDomains");
    QTest::addColumn<QUrl>("hostToTest");
    QTest::addColumn<bool>("isKnown");

    const QDateTime currentUTC = QDateTime::currentDateTimeUtc();
    const QUrl knownHost(QLatin1String("http://example.com"));
    const QUrl validSubdomain(QLatin1String("https://sub.example.com/ohoho"));
    const QUrl unknownDomain(QLatin1String("http://example.org"));
    const QUrl subSubdomain(QLatin1String("https://level3.level2.example.com"));

    const QDateTime validDate(currentUTC.addSecs(1000));
    QTest::newRow("same-known") << knownHost << validDate << false << knownHost << true;
    QTest::newRow("subexcluded") << knownHost << validDate << false << validSubdomain << false;
    QTest::newRow("subincluded") << knownHost << validDate << true << validSubdomain << true;
    QTest::newRow("unknown-subexcluded") << knownHost << validDate << false << unknownDomain << false;
    QTest::newRow("unknown-subincluded") << knownHost << validDate << true << unknownDomain << false;
    QTest::newRow("sub-subdomain-subincluded") << knownHost << validDate << true << subSubdomain << true;
    QTest::newRow("sub-subdomain-subexcluded") << knownHost << validDate << false << subSubdomain << false;

    const QDateTime invalidDate;
    QTest::newRow("invalid-time") << knownHost << invalidDate << false << knownHost << false;
    QTest::newRow("invalid-time-subexcluded") << knownHost << invalidDate << false
                                              << validSubdomain << false;
    QTest::newRow("invalid-time-subincluded") << knownHost << invalidDate << true
                                              << validSubdomain << false;

    const QDateTime expiredDate(currentUTC.addSecs(-1000));
    QTest::newRow("expired-time") << knownHost << expiredDate << false << knownHost << false;
    QTest::newRow("expired-time-subexcluded") << knownHost << expiredDate << false
                                              << validSubdomain << false;
    QTest::newRow("expired-time-subincluded") << knownHost << expiredDate << true
                                              << validSubdomain << false;
    const QUrl ipAsHost(QLatin1String("http://127.0.0.1"));
    QTest::newRow("ip-address-in-hostname") << ipAsHost << validDate << false
                                              << ipAsHost << false;

    const QUrl anyIPv4AsHost(QLatin1String("http://0.0.0.0"));
    QTest::newRow("anyip4-address-in-hostname") << anyIPv4AsHost << validDate
                                                << false << anyIPv4AsHost << false;
    const QUrl anyIPv6AsHost(QLatin1String("http://[::]"));
    QTest::newRow("anyip6-address-in-hostname") << anyIPv6AsHost << validDate
                                                << false << anyIPv6AsHost << false;

}

void tst_QHsts::testSingleKnownHost()
{
    QFETCH(const QUrl, knownHost);
    QFETCH(const QDateTime, policyExpires);
    QFETCH(const bool, includeSubDomains);
    QFETCH(const QUrl, hostToTest);
    QFETCH(const bool, isKnown);

    QHstsCache cache;
    cache.updateKnownHost(knownHost, policyExpires, includeSubDomains);
    QCOMPARE(cache.isKnownHost(hostToTest), isKnown);
}

void tst_QHsts::testMultilpeKnownHosts()
{
    const QDateTime currentUTC = QDateTime::currentDateTimeUtc();
    const QDateTime validDate(currentUTC.addSecs(10000));
    const QDateTime expiredDate(currentUTC.addSecs(-10000));
    const QUrl exampleCom(QLatin1String("https://example.com"));
    const QUrl subExampleCom(QLatin1String("https://sub.example.com"));

    QHstsCache cache;
    // example.com is HSTS and includes subdomains:
    cache.updateKnownHost(exampleCom, validDate, true);
    QVERIFY(cache.isKnownHost(exampleCom));
    QVERIFY(cache.isKnownHost(subExampleCom));
    // example.com can set its policy not to include subdomains:
    cache.updateKnownHost(exampleCom, validDate, false);
    QVERIFY(!cache.isKnownHost(subExampleCom));
    // but sub.example.com can set its own policy:
    cache.updateKnownHost(subExampleCom, validDate, false);
    QVERIFY(cache.isKnownHost(subExampleCom));
    // let's say example.com's policy has expired:
    cache.updateKnownHost(exampleCom, expiredDate, false);
    QVERIFY(!cache.isKnownHost(exampleCom));
    // it should not affect sub.example.com's policy:
    QVERIFY(cache.isKnownHost(subExampleCom));

    // clear cache and invalidate all policies:
    cache.clear();
    QVERIFY(!cache.isKnownHost(exampleCom));
    QVERIFY(!cache.isKnownHost(subExampleCom));

    // siblings:
    const QUrl anotherSub(QLatin1String("https://sub2.example.com"));
    cache.updateKnownHost(subExampleCom, validDate, true);
    cache.updateKnownHost(anotherSub, validDate, true);
    QVERIFY(cache.isKnownHost(subExampleCom));
    QVERIFY(cache.isKnownHost(anotherSub));
    // they cannot set superdomain's policy:
    QVERIFY(!cache.isKnownHost(exampleCom));
    // a sibling cannot set another sibling's policy:
    cache.updateKnownHost(anotherSub, expiredDate, false);
    QVERIFY(cache.isKnownHost(subExampleCom));
    QVERIFY(!cache.isKnownHost(anotherSub));
    QVERIFY(!cache.isKnownHost(exampleCom));
    // let's make example.com known again:
    cache.updateKnownHost(exampleCom, validDate, true);
    // a subdomain cannot affect its superdomain's policy:
    cache.updateKnownHost(subExampleCom, expiredDate, true);
    QVERIFY(cache.isKnownHost(exampleCom));
    // and this superdomain includes subdomains in its HSTS policy:
    QVERIFY(cache.isKnownHost(subExampleCom));
    QVERIFY(cache.isKnownHost(anotherSub));

    // a subdomain (with its subdomains) cannot affect its superdomain's policy:
    cache.updateKnownHost(exampleCom, expiredDate, true);
    cache.updateKnownHost(subExampleCom, validDate, true);
    QVERIFY(cache.isKnownHost(subExampleCom));
    QVERIFY(!cache.isKnownHost(exampleCom));
}

void tst_QHsts::testPolicyExpiration()
{
    QDateTime currentUTC = QDateTime::currentDateTimeUtc();
    const QUrl exampleCom(QLatin1String("http://example.com"));
    const QUrl subdomain(QLatin1String("http://subdomain.example.com"));
    const qint64 lifeTimeMS = 50;

    QHstsCache cache;
    // start with 'includeSubDomains' and 5 s. lifetime:
    cache.updateKnownHost(exampleCom, currentUTC.addMSecs(lifeTimeMS), true);
    QVERIFY(cache.isKnownHost(exampleCom));
    QVERIFY(cache.isKnownHost(subdomain));
    // wait for approx. a half of lifetime:
    QTest::qWait(lifeTimeMS / 2);

    if (QDateTime::currentDateTimeUtc() < currentUTC.addMSecs(lifeTimeMS)) {
        // Should still be valid:
        QVERIFY(cache.isKnownHost(exampleCom));
        QVERIFY(cache.isKnownHost(subdomain));
    }

    QTest::qWait(lifeTimeMS);
    // expired:
    QVERIFY(!cache.isKnownHost(exampleCom));
    QVERIFY(!cache.isKnownHost(subdomain));

    // now check that superdomain's policy expires, but not subdomain's policy:
    currentUTC = QDateTime::currentDateTimeUtc();
    cache.updateKnownHost(exampleCom, currentUTC.addMSecs(lifeTimeMS / 5), true);
    cache.updateKnownHost(subdomain, currentUTC.addMSecs(lifeTimeMS), true);
    QVERIFY(cache.isKnownHost(exampleCom));
    QVERIFY(cache.isKnownHost(subdomain));
    QTest::qWait(lifeTimeMS / 2);
    if (QDateTime::currentDateTimeUtc() < currentUTC.addMSecs(lifeTimeMS)) {
        QVERIFY(!cache.isKnownHost(exampleCom));
        QVERIFY(cache.isKnownHost(subdomain));
    }
}

void tst_QHsts::testSTSHeaderParser()
{
    QHstsHeaderParser parser;

    QVERIFY(!parser.includeSubDomains());
    QVERIFY(!parser.expirationDate().isValid());
    QHttpHeaders headers;
    QVERIFY(!parser.parse(headers));
    QVERIFY(!parser.includeSubDomains());
    QVERIFY(!parser.expirationDate().isValid());

    headers.append("Strict-Transport-security", "200");
    QVERIFY(!parser.parse(headers));
    QVERIFY(!parser.includeSubDomains());
    QVERIFY(!parser.expirationDate().isValid());

    // This header is missing REQUIRED max-age directive, so we'll ignore it:
    headers.append("Strict-Transport-Security", "includeSubDomains");
    QVERIFY(!parser.parse(headers));
    QVERIFY(!parser.includeSubDomains());
    QVERIFY(!parser.expirationDate().isValid());

    headers.removeAt(headers.size() - 1);
    headers.append("Strict-Transport-Security", "includeSubDomains;max-age=1000");
    QVERIFY(parser.parse(headers));
    QVERIFY(parser.expirationDate() > QDateTime::currentDateTimeUtc());
    QVERIFY(parser.includeSubDomains());

    headers.removeAt(headers.size() - 1);
    headers.append("strict-transport-security", "includeSubDomains;max-age=1000");
    QVERIFY(parser.parse(headers));
    QVERIFY(parser.expirationDate() > QDateTime::currentDateTimeUtc());
    QVERIFY(parser.includeSubDomains());

    headers.removeAt(headers.size() - 1);
    // Invalid (includeSubDomains twice):
    headers.append("Strict-Transport-Security", "max-age = 1000 ; includeSubDomains;includeSubDomains");
    QVERIFY(!parser.parse(headers));
    QVERIFY(!parser.includeSubDomains());
    QVERIFY(!parser.expirationDate().isValid());

    headers.removeAt(headers.size() - 1);
    // Invalid (weird number of seconds):
    headers.append("Strict-Transport-Security", "max-age=-1000   ; includeSubDomains");
    QVERIFY(!parser.parse(headers));
    QVERIFY(!parser.includeSubDomains());
    QVERIFY(!parser.expirationDate().isValid());

    headers.removeAt(headers.size() - 1);
    // Note, directives are case-insensitive + we should ignore unknown directive.
    headers.append("Strict-Transport-Security", ";max-age=1000 ;includesubdomains;;"
                   "nowsomeunknownheader=\"somevaluewithescapes\\;\"");
    QVERIFY(parser.parse(headers));
    QVERIFY(parser.includeSubDomains());
    QVERIFY(parser.expirationDate().isValid());

    headers.removeAt(headers.size() - 1);
    // Check that we know how to unescape max-age:
    headers.append("Strict-Transport-Security", "max-age=\"1000\"");
    QVERIFY(parser.parse(headers));
    QVERIFY(!parser.includeSubDomains());
    QVERIFY(parser.expirationDate().isValid());

    headers.removeAt(headers.size() - 1);
    // The only STS header, with invalid syntax though, to be ignored:
    headers.append("Strict-Transport-Security", "max-age; max-age=15768000");
    QVERIFY(!parser.parse(headers));
    QVERIFY(!parser.includeSubDomains());
    QVERIFY(!parser.expirationDate().isValid());

    // Now we check that our parse chosses the first valid STS header and ignores
    // others:
    headers.clear();
    headers.append("Strict-Transport-Security", "includeSubdomains; max-age=\"hehehe\";");
    headers.append("Strict-Transport-Security", "max-age=10101");
    QVERIFY(parser.parse(headers));
    QVERIFY(!parser.includeSubDomains());
    QVERIFY(parser.expirationDate().isValid());


    headers.clear();
    headers.append("Strict-Transport-Security", "max-age=0");
    QVERIFY(parser.parse(headers));
    QVERIFY(!parser.includeSubDomains());
    QVERIFY(parser.expirationDate() <= QDateTime::currentDateTimeUtc());

    // Parsing is case-insensitive:
    headers.removeAt(headers.size() - 1);
    headers.append("Strict-Transport-Security", "Max-aGE=1000; InclUdesUbdomains");
    QVERIFY(parser.parse(headers));
    QVERIFY(parser.includeSubDomains());
    QVERIFY(parser.expirationDate().isValid());

    // Grammar of STS header is quite permissive, let's check we can parse
    // some weird but valid header:
    headers.removeAt(headers.size() - 1);
    headers.append("Strict-Transport-Security", ";;; max-age = 17; ; ; ; ;;; ;;"
                    ";;; ; includeSubdomains ;;thisIsUnknownDirective;;;;");
    QVERIFY(parser.parse(headers));
    QVERIFY(parser.includeSubDomains());
    QVERIFY(parser.expirationDate().isValid());

    headers.removeAt(headers.size() - 1);
    headers.append("Strict-Transport-Security", "max-age=1000; includeSubDomains bogon");
    QVERIFY(!parser.parse(headers));
    QVERIFY(!parser.includeSubDomains());
    QVERIFY(!parser.expirationDate().isValid());
}

const QLatin1String storeDir(".");

struct TestStoreDeleter
{
    ~TestStoreDeleter()
    {
        QDir cwd;
        if (!cwd.remove(QHstsStore::absoluteFilePath(storeDir)))
            qWarning() << "tst_QHsts::testStore: failed to remove the hsts store file";
    }
};

void tst_QHsts::testStore()
{
    // Delete the store's file after we finish the test.
    TestStoreDeleter cleaner;

    const QUrl exampleCom(QStringLiteral("http://example.com"));
    const QUrl subDomain(QStringLiteral("http://subdomain.example.com"));
    const QDateTime validDate(QDateTime::currentDateTimeUtc().addDays(1));

    {
        // We start from an empty cache and empty store:
        QHstsCache cache;
        QHstsStore store(storeDir);
        cache.setStore(&store);
        QVERIFY(!cache.isKnownHost(exampleCom));
        QVERIFY(!cache.isKnownHost(subDomain));
        // (1) This will also store the policy:
        cache.updateKnownHost(exampleCom, validDate, true);
        QVERIFY(cache.isKnownHost(exampleCom));
        QVERIFY(cache.isKnownHost(subDomain));
    }
    {
        // Test the policy stored at (1):
        QHstsCache cache;
        QHstsStore store(storeDir);
        cache.setStore(&store);
        QVERIFY(cache.isKnownHost(exampleCom));
        QVERIFY(cache.isKnownHost(subDomain));
        // (2) Remove subdomains:
        cache.updateKnownHost(exampleCom, validDate, false);
        QVERIFY(!cache.isKnownHost(subDomain));
    }
    {
        // Test the previous update (2):
        QHstsCache cache;
        QHstsStore store(storeDir);
        cache.setStore(&store);
        QVERIFY(cache.isKnownHost(exampleCom));
        QVERIFY(!cache.isKnownHost(subDomain));
    }
    {
        QHstsCache cache;
        cache.updateKnownHost(subDomain, validDate, false);
        QVERIFY(cache.isKnownHost(subDomain));
        QHstsStore store(storeDir);
        // (3) This should store policy from cache, over old policy from store:
        cache.setStore(&store);
    }
    {
        // Test that (3) was stored:
        QHstsCache cache;
        QHstsStore store(storeDir);
        cache.setStore(&store);
        QVERIFY(cache.isKnownHost(subDomain));
    }
}

QTEST_MAIN(tst_QHsts)

#include "tst_qhsts.moc"
