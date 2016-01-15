/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtNetwork/QNetworkCookieJar>
#include <QtNetwork/QNetworkCookie>
#include <QtNetwork/QNetworkRequest>
#include "private/qtldurl_p.h"

class tst_QNetworkCookieJar: public QObject
{
    Q_OBJECT

private slots:
    void getterSetter();
    void setCookiesFromUrl_data();
    void setCookiesFromUrl();
    void cookiesForUrl_data();
    void cookiesForUrl();
#ifdef QT_BUILD_INTERNAL
    void effectiveTLDs_data();
    void effectiveTLDs();
#endif
    void rfc6265_data();
    void rfc6265();
};

QT_BEGIN_NAMESPACE

namespace QTest {
    template<>
    char *toString(const QNetworkCookie &cookie)
    {
        return qstrdup(cookie.toRawForm());
    }

    template<>
    char *toString(const QList<QNetworkCookie> &list)
    {
        QByteArray result = "QList(";
        bool first = true;
        foreach (QNetworkCookie cookie, list) {
            if (!first)
                result += ", ";
            first = false;
            result += "QNetworkCookie(" + cookie.toRawForm() + ')';
        }

        result.append(')');
        return qstrdup(result.constData());
    }
}

QT_END_NAMESPACE

class MyCookieJar: public QNetworkCookieJar
{
public:
    inline QList<QNetworkCookie> allCookies() const
        { return QNetworkCookieJar::allCookies(); }
    inline void setAllCookies(const QList<QNetworkCookie> &cookieList)
        { QNetworkCookieJar::setAllCookies(cookieList); }
};

void tst_QNetworkCookieJar::getterSetter()
{
    MyCookieJar jar;

    QVERIFY(jar.allCookies().isEmpty());

    QList<QNetworkCookie> list;
    QNetworkCookie cookie;
    cookie.setName("a");
    list << cookie;

    jar.setAllCookies(list);
    QCOMPARE(jar.allCookies(), list);
}

void tst_QNetworkCookieJar::setCookiesFromUrl_data()
{
    QTest::addColumn<QList<QNetworkCookie> >("preset");
    QTest::addColumn<QNetworkCookie>("newCookie");
    QTest::addColumn<QString>("referenceUrl");
    QTest::addColumn<QList<QNetworkCookie> >("expectedResult");
    QTest::addColumn<bool>("setCookies");

    QList<QNetworkCookie> preset;
    QList<QNetworkCookie> result;
    QNetworkCookie cookie;

    cookie.setName("a");
    cookie.setPath("/");
    cookie.setDomain(".foo.tld");
    result += cookie;
    QTest::newRow("just-add") << preset << cookie << "http://www.foo.tld" << result << true;

    preset = result;
    QTest::newRow("replace-1") << preset << cookie << "http://www.foo.tld" << result << true;

    cookie.setValue("bc");
    result.clear();
    result += cookie;
    QTest::newRow("replace-2") << preset << cookie << "http://www.foo.tld" << result << true;

    preset = result;
    cookie.setName("d");
    result += cookie;
    QTest::newRow("append") << preset << cookie << "http://www.foo.tld" << result << true;

    cookie = preset.at(0);
    result = preset;
    cookie.setPath("/something");
    result += cookie;
    QTest::newRow("diff-path") << preset << cookie << "http://www.foo.tld/something" << result << true;

    preset.clear();
    preset += cookie;
    cookie.setPath("/");
    QTest::newRow("diff-path-order") << preset << cookie << "http://www.foo.tld" << result << true;

    preset.clear();
    result.clear();
    QNetworkCookie finalCookie = cookie;
    cookie.setDomain("foo.tld");
    finalCookie.setDomain(".foo.tld");
    result += finalCookie;
    QTest::newRow("should-add-dot-prefix") << preset << cookie << "http://www.foo.tld" << result << true;

    result.clear();
    cookie.setDomain("");
    finalCookie.setDomain("www.foo.tld");
    result += finalCookie;
    QTest::newRow("should-set-default-domain") << preset << cookie << "http://www.foo.tld" << result << true;

    // security test:
    result.clear();
    preset.clear();
    cookie.setDomain("something.completely.different");
    QTest::newRow("security-domain-1") << preset << cookie << "http://www.foo.tld" << result << false;

    // we want the cookie to be accepted although the path does not match, see QTBUG-5815
    cookie.setDomain(".foo.tld");
    cookie.setPath("/something");
    result += cookie;
    QTest::newRow("security-path-1") << preset << cookie << "http://www.foo.tld" << result << true;

    // check effective TLDs
    // 1. co.uk is an effective TLD, should be denied
    result.clear();
    preset.clear();
    cookie.setPath("/");
    cookie.setDomain(".co.uk");
    QTest::newRow("effective-tld1-denied") << preset << cookie << "http://something.co.uk" << result << false;
    cookie.setDomain("co.uk");
    QTest::newRow("effective-tld1-denied2") << preset << cookie << "http://something.co.uk" << result << false;
    cookie.setDomain(".something.co.uk");
    result += cookie;
    QTest::newRow("effective-tld1-accepted") << preset << cookie << "http://something.co.uk" << result << true;

    // 2. anything .mz is an effective TLD ('*.mz'), but 'teledata.mz' is an exception
    result.clear();
    preset.clear();
    cookie.setDomain(".farmacia.mz");
    QTest::newRow("effective-tld2-denied") << preset << cookie << "http://farmacia.mz" << result << false;
    QTest::newRow("effective-tld2-denied2") << preset << cookie << "http://www.farmacia.mz" << result << false;
    QTest::newRow("effective-tld2-denied3") << preset << cookie << "http://www.anything.farmacia.mz" << result << false;
    cookie.setDomain(".teledata.mz");
    result += cookie;
    QTest::newRow("effective-tld2-accepted") << preset << cookie << "http://www.teledata.mz" << result << true;

    result.clear();
    preset.clear();
    cookie.setDomain("127.0.0.1");
    result += cookie;
    QTest::newRow("IPv4-address-as-domain") << preset << cookie << "http://127.0.0.1/" << result << true;

    result.clear();
    preset.clear();
    cookie.setDomain("fe80::250:56ff:fec0:1");
    result += cookie;
    QTest::newRow("IPv6-address-as-domain") << preset << cookie << "http://[fe80::250:56ff:fec0:1]/" << result << true;

    // setting the defaults:
    finalCookie = cookie;
    finalCookie.setPath("/something/");
    finalCookie.setDomain("www.foo.tld");
    cookie.setPath("");
    cookie.setDomain("");
    result.clear();
    result += finalCookie;
    QTest::newRow("defaults-1") << preset << cookie << "http://www.foo.tld/something/" << result << true;

    finalCookie.setPath("/");
    result.clear();
    result += finalCookie;
    QTest::newRow("defaults-2") << preset << cookie << "http://www.foo.tld" << result << true;

    // security test: do not accept cookie domains like ".com" nor ".com." (see RFC 2109 section 4.3.2)
    result.clear();
    preset.clear();
    cookie.setDomain(".com");
    QTest::newRow("rfc2109-4.3.2-ex3") << preset << cookie << "http://x.foo.com" << result << false;

    result.clear();
    preset.clear();
    cookie.setDomain(".com.");
    QTest::newRow("rfc2109-4.3.2-ex3-2") << preset << cookie << "http://x.foo.com" << result << false;
}

void tst_QNetworkCookieJar::setCookiesFromUrl()
{
    QFETCH(QList<QNetworkCookie>, preset);
    QFETCH(QNetworkCookie, newCookie);
    QFETCH(QString, referenceUrl);
    QFETCH(QList<QNetworkCookie>, expectedResult);
    QFETCH(bool, setCookies);

    QList<QNetworkCookie> cookieList;
    cookieList += newCookie;
    MyCookieJar jar;
    jar.setAllCookies(preset);
    QCOMPARE(jar.setCookiesFromUrl(cookieList, referenceUrl), setCookies);

    QList<QNetworkCookie> result = jar.allCookies();
    foreach (QNetworkCookie cookie, expectedResult) {
        QVERIFY2(result.contains(cookie), cookie.toRawForm());
        result.removeAll(cookie);
    }
    QVERIFY2(result.isEmpty(), QTest::toString(result));
}

void tst_QNetworkCookieJar::cookiesForUrl_data()
{
    QTest::addColumn<QList<QNetworkCookie> >("allCookies");
    QTest::addColumn<QString>("url");
    QTest::addColumn<QList<QNetworkCookie> >("expectedResult");

    QList<QNetworkCookie> allCookies;
    QList<QNetworkCookie> result;

    QTest::newRow("no-cookies") << allCookies << "http://foo.bar/" << result;

    QNetworkCookie cookie;
    cookie.setName("a");
    cookie.setPath("/web");
    cookie.setDomain(".qt-project.org");
    allCookies += cookie;

    QTest::newRow("no-match-1") << allCookies << "http://foo.bar/" << result;
    QTest::newRow("no-match-2") << allCookies << "http://foo.bar/web" << result;
    QTest::newRow("no-match-3") << allCookies << "http://foo.bar/web/wiki" << result;
    QTest::newRow("no-match-4") << allCookies << "http://qt-project.org" << result;
    QTest::newRow("no-match-5") << allCookies << "http://qt-project.org" << result;
    QTest::newRow("no-match-6") << allCookies << "http://qt-project.org/webinar" << result;
    QTest::newRow("no-match-7") << allCookies << "http://qt-project.org/webinar" << result;
    QTest::newRow("no-match-8") << allCookies << "http://qt-project.org./web" << result;
    QTest::newRow("no-match-9") << allCookies << "http://qt-project.org./web" << result;

    result = allCookies;
    QTest::newRow("match-1") << allCookies << "http://qt-project.org/web" << result;
    QTest::newRow("match-2") << allCookies << "http://qt-project.org/web/" << result;
    QTest::newRow("match-3") << allCookies << "http://qt-project.org/web/content" << result;
    QTest::newRow("match-4") << allCookies << "http://qt-project.org/web" << result;
    QTest::newRow("match-5") << allCookies << "http://qt-project.org/web/" << result;
    QTest::newRow("match-6") << allCookies << "http://qt-project.org/web/content" << result;

    cookie.setPath("/web/wiki");
    allCookies += cookie;

    // exact same results as before:
    QTest::newRow("one-match-1") << allCookies << "http://qt-project.org/web" << result;
    QTest::newRow("one-match-2") << allCookies << "http://qt-project.org/web/" << result;
    QTest::newRow("one-match-3") << allCookies << "http://qt-project.org/web/content" << result;
    QTest::newRow("one-match-4") << allCookies << "http://qt-project.org/web" << result;
    QTest::newRow("one-match-5") << allCookies << "http://qt-project.org/web/" << result;
    QTest::newRow("one-match-6") << allCookies << "http://qt-project.org/web/content" << result;

    result.prepend(cookie);     // longer path, it must match first
    QTest::newRow("two-matches-1") << allCookies << "http://qt-project.org/web/wiki" << result;
    QTest::newRow("two-matches-2") << allCookies << "http://qt-project.org/web/wiki" << result;

    // invert the order;
    allCookies.clear();
    allCookies << result.at(1) << result.at(0);
    QTest::newRow("two-matches-3") << allCookies << "http://qt-project.org/web/wiki" << result;
    QTest::newRow("two-matches-4") << allCookies << "http://qt-project.org/web/wiki" << result;

    // expired cookie
    allCookies.clear();
    cookie.setExpirationDate(QDateTime::fromString("09-Nov-1999", "dd-MMM-yyyy"));
    allCookies += cookie;
    result.clear();
    QTest::newRow("exp-match-1") << allCookies << "http://qt-project.org/web" << result;
    QTest::newRow("exp-match-2") << allCookies << "http://qt-project.org/web/" << result;
    QTest::newRow("exp-match-3") << allCookies << "http://qt-project.org/web/content" << result;
    QTest::newRow("exp-match-4") << allCookies << "http://qt-project.org/web" << result;
    QTest::newRow("exp-match-5") << allCookies << "http://qt-project.org/web/" << result;
    QTest::newRow("exp-match-6") << allCookies << "http://qt-project.org/web/content" << result;

    // path matching
    allCookies.clear();
    QNetworkCookie anotherCookie;
    anotherCookie.setName("a");
    anotherCookie.setPath("/web");
    anotherCookie.setDomain(".qt-project.org");
    allCookies += anotherCookie;
    result.clear();
    QTest::newRow("path-unmatch-1") << allCookies << "http://qt-project.org/" << result;
    QTest::newRow("path-unmatch-2") << allCookies << "http://qt-project.org/something/else" << result;
    result += anotherCookie;
    QTest::newRow("path-match-1") << allCookies << "http://qt-project.org/web" << result;
    QTest::newRow("path-match-2") << allCookies << "http://qt-project.org/web/" << result;
    QTest::newRow("path-match-3") << allCookies << "http://qt-project.org/web/content" << result;

    // secure cookies
    allCookies.clear();
    result.clear();
    QNetworkCookie secureCookie;
    secureCookie.setName("a");
    secureCookie.setPath("/web");
    secureCookie.setDomain(".qt-project.org");
    secureCookie.setSecure(true);
    allCookies += secureCookie;
    QTest::newRow("no-match-secure-1") << allCookies << "http://qt-project.org/web" << result;
    QTest::newRow("no-match-secure-2") << allCookies << "http://qt-project.org/web" << result;
    result += secureCookie;
    QTest::newRow("match-secure-1") << allCookies << "https://qt-project.org/web" << result;
    QTest::newRow("match-secure-2") << allCookies << "https://qt-project.org/web" << result;

    // domain ending in .
    allCookies.clear();
    result.clear();
    QNetworkCookie cookieDot;
    cookieDot.setDomain(".example.com.");
    cookieDot.setName("a");
    allCookies += cookieDot;
    QTest::newRow("no-match-domain-dot") << allCookies << "http://example.com" << result;
    result += cookieDot;
    QTest::newRow("match-domain-dot") << allCookies << "http://example.com." << result;
}

void tst_QNetworkCookieJar::cookiesForUrl()
{
    QFETCH(QList<QNetworkCookie>, allCookies);
    QFETCH(QString, url);
    QFETCH(QList<QNetworkCookie>, expectedResult);

    MyCookieJar jar;
    jar.setAllCookies(allCookies);

    QList<QNetworkCookie> result = jar.cookiesForUrl(url);
    QCOMPARE(result, expectedResult);
}

// This test requires private API.
#ifdef QT_BUILD_INTERNAL
void tst_QNetworkCookieJar::effectiveTLDs_data()
{
    QTest::addColumn<QString>("domain");
    QTest::addColumn<bool>("isTLD");

    QTest::newRow("yes1") << "com" << true;
    QTest::newRow("yes2") << "de" << true;
    QTest::newRow("yes3") << "ulm.museum" << true;
    QTest::newRow("yes4") << "krodsherad.no" << true;
    QTest::newRow("yes5") << "1.bg" << true;
    QTest::newRow("yes6") << "com.cn" << true;
    QTest::newRow("yes7") << "org.ws" << true;
    QTest::newRow("yes8") << "co.uk" << true;
    QTest::newRow("yes9") << "wallonie.museum" << true;
    QTest::newRow("yes10") << "hk.com" << true;
    QTest::newRow("yes11") << "hk.org" << true;

    QTest::newRow("no1") << "anything.com" << false;
    QTest::newRow("no2") << "anything.de" << false;
    QTest::newRow("no3") << "eselsberg.ulm.museum" << false;
    QTest::newRow("no4") << "noe.krodsherad.no" << false;
    QTest::newRow("no5") << "2.1.bg" << false;
    QTest::newRow("no6") << "foo.com.cn" << false;
    QTest::newRow("no7") << "something.org.ws" << false;
    QTest::newRow("no8") << "teatime.co.uk" << false;
    QTest::newRow("no9") << "bla" << false;
    QTest::newRow("no10") << "bla.bla" << false;
    QTest::newRow("no11") << "mosreg.ru" << false;

    const ushort s1[] = {0x74, 0x72, 0x61, 0x6e, 0xf8, 0x79, 0x2e, 0x6e, 0x6f, 0x00}; // xn--trany-yua.no
    const ushort s2[] = {0x5d9, 0x5e8, 0x5d5, 0x5e9, 0x5dc, 0x5d9, 0x5dd, 0x2e, 0x6d, 0x75, 0x73, 0x65, 0x75, 0x6d, 0x00}; // xn--9dbhblg6di.museum
    const ushort s3[] = {0x7ec4, 0x7e54, 0x2e, 0x68, 0x6b, 0x00}; // xn--mk0axi.hk
    const ushort s4[] = {0x7f51, 0x7edc, 0x2e, 0x63, 0x6e, 0x00}; // xn--io0a7i.cn
    const ushort s5[] = {0x72, 0xe1, 0x68, 0x6b, 0x6b, 0x65, 0x72, 0xe1, 0x76, 0x6a, 0x75, 0x2e, 0x6e, 0x6f, 0x00}; // xn--rhkkervju-01af.no
    const ushort s6[] = {0xb9a, 0xbbf, 0xb99, 0xbcd, 0xb95, 0xbaa, 0xbcd, 0xbaa, 0xbc2, 0xbb0, 0xbcd, 0x00}; // xn--clchc0ea0b2g2a9gcd
    const ushort s7[] = {0x627, 0x644, 0x627, 0x631, 0x62f, 0x646, 0x00}; // xn--mgbayh7gpa
    const ushort s8[] = {0x63, 0x6f, 0x72, 0x72, 0x65, 0x69, 0x6f, 0x73, 0x2d, 0x65, 0x2d, 0x74, 0x65, 0x6c, 0x65,
                         0x63, 0x6f, 0x6d, 0x75, 0x6e, 0x69, 0x63, 0x61, 0xe7, 0xf5, 0x65, 0x73, 0x2e, 0x6d, 0x75,
                         0x73, 0x65, 0x75, 0x6d, 0x00}; // xn--correios-e-telecomunicaes-ghc29a.museum
    QTest::newRow("yes-specialchars1") << QString::fromUtf16(s1) << true;
    QTest::newRow("yes-specialchars2") << QString::fromUtf16(s2) << true;
    QTest::newRow("yes-specialchars3") << QString::fromUtf16(s3) << true;
    QTest::newRow("yes-specialchars4") << QString::fromUtf16(s4) << true;
    QTest::newRow("yes-specialchars5") << QString::fromUtf16(s5) << true;
    QTest::newRow("yes-specialchars6") << QString::fromUtf16(s6) << true;
    QTest::newRow("yes-specialchars7") << QString::fromUtf16(s7) << true;
    QTest::newRow("yes-specialchars8") << QString::fromUtf16(s8) << true;

    QTest::newRow("no-specialchars1") << QString::fromUtf16(s1).prepend("something") << false;
    QTest::newRow("no-specialchars2") << QString::fromUtf16(s2).prepend(QString::fromUtf16(s2)) << false;
    QTest::newRow("no-specialchars2.5") << QString::fromUtf16(s2).prepend("whatever") << false;
    QTest::newRow("no-specialchars3") << QString::fromUtf16(s3).prepend("foo") << false;
    QTest::newRow("no-specialchars4") << QString::fromUtf16(s4).prepend("bar") << false;
    QTest::newRow("no-specialchars5") << QString::fromUtf16(s5).prepend(QString::fromUtf16(s2)) << false;
    QTest::newRow("no-specialchars6") << QString::fromUtf16(s6).prepend(QLatin1Char('.') + QString::fromUtf16(s6)) << false;
    QTest::newRow("no-specialchars7") << QString::fromUtf16(s7).prepend("bla") << false;
    QTest::newRow("no-specialchars8") << QString::fromUtf16(s8).append("foo") << false;

    QTest::newRow("exception1") << "pref.iwate.jp" << false;
    QTest::newRow("exception2") << "omanpost.om" << false;
    QTest::newRow("exception3") << "omantel.om" << false;
    QTest::newRow("exception4") << "gobiernoelectronico.ar" << false;
    QTest::newRow("exception5") << "pref.ishikawa.jp" << false;

    QTest::newRow("yes-wildcard1") << "*.jm" << true;
    QTest::newRow("yes-wildcard1.5") << "anything.jm" << true;
    QTest::newRow("yes-wildcard2") << "something.kh" << true;
    QTest::newRow("no-wildcard3") << "whatever.uk" << false; // was changed at some point
    QTest::newRow("yes-wildcard4") << "anything.sendai.jp" << true;
    QTest::newRow("yes-wildcard5") << "foo.sch.uk" << true;
    QTest::newRow("yes-wildcard6") << "something.platform.sh" << true;
}

void tst_QNetworkCookieJar::effectiveTLDs()
{
    QFETCH(QString, domain);
    QFETCH(bool, isTLD);
    QCOMPARE(qIsEffectiveTLD(domain), isTLD);
}
#endif

void tst_QNetworkCookieJar::rfc6265_data()
{
    QTest::addColumn<QStringList>("received");
    QTest::addColumn<QList<QNetworkCookie> >("sent");
    QTest::addColumn<QString>("sentTo");

    QFile file(QFINDTESTDATA("parser.json"));
    QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
    QJsonDocument document;
    document = QJsonDocument::fromJson(file.readAll());
    QVERIFY(!document.isNull());
    QVERIFY(document.isArray());

    foreach (const QJsonValue& testCase, document.array()) {
        QJsonObject testObject = testCase.toObject();

        //"test" - the test case name
        QString testCaseName = testObject.value("test").toString();
        if (testCaseName.toLower().startsWith("disabled"))
            continue;

        //"received" - the cookies received from the server
        QJsonArray received = testObject.value("received").toArray();
        QStringList receivedList;
        foreach (const QJsonValue& receivedCookie, received)
            receivedList.append(receivedCookie.toString());

        //"sent" - the cookies sent back to the server
        QJsonArray sent = testObject.value("sent").toArray();
        QList<QNetworkCookie> sentList;
        foreach (const QJsonValue& sentCookie, sent) {
            QJsonObject sentCookieObject = sentCookie.toObject();
            QNetworkCookie cookie;
            cookie.setName(sentCookieObject.value("name").toString().toUtf8());
            cookie.setValue(sentCookieObject.value("value").toString().toUtf8());
            sentList.append(cookie);
        }

        //"sent-to" - the relative url where cookies are sent
        QTest::newRow(qPrintable(testCaseName)) << receivedList << sentList << testObject.value("sent-to").toString();
    }
}

void tst_QNetworkCookieJar::rfc6265()
{
    QFETCH(QStringList, received);
    QFETCH(QList<QNetworkCookie>, sent);
    QFETCH(QString, sentTo);

    QUrl receivedUrl("http://home.example.org:8888/cookie-parser");
    QUrl sentUrl("http://home.example.org:8888/cookie-parser-result");
    if (!sentTo.isEmpty())
        sentUrl = receivedUrl.resolved(sentTo);

    QNetworkCookieJar jar;
    QList<QNetworkCookie> receivedCookies;
    foreach (const QString &cookieLine, received)
        receivedCookies.append(QNetworkCookie::parseCookies(cookieLine.toUtf8()));

    jar.setCookiesFromUrl(receivedCookies, receivedUrl);
    QList<QNetworkCookie> cookiesToSend = jar.cookiesForUrl(sentUrl);

    //compare cookies only using name/value, as the metadata isn't sent over the network
    QCOMPARE(cookiesToSend.count(), sent.count());
    bool ok = true;
    for (int i = 0; i < cookiesToSend.count(); i++) {
        if (cookiesToSend.at(i).name() != sent.at(i).name()) {
            ok = false;
            break;
        }
        if (cookiesToSend.at(i).value() != sent.at(i).value()) {
            ok = false;
            break;
        }
    }
    if (!ok) {
        QNetworkRequest r(sentUrl);
        r.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(cookiesToSend));
        QString actual = QString::fromUtf8(r.rawHeader("Cookie"));
        r.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(sent));
        QString expected = QString::fromUtf8(r.rawHeader("Cookie"));

        QVERIFY2(ok, qPrintable(QString("Expected: %1\nActual: %2").arg(expected).arg(actual)));
    }
}

QTEST_MAIN(tst_QNetworkCookieJar)
#include "tst_qnetworkcookiejar.moc"

