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
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkCookie>


class tst_QNetworkCookie: public QObject
{
    Q_OBJECT

private slots:
    void getterSetter();

    void parseSingleCookie_data();
    void parseSingleCookie();

    void parseMultipleCookies_data();
    void parseMultipleCookies();
};

void tst_QNetworkCookie::getterSetter()
{
    QNetworkCookie cookie;
    QNetworkCookie otherCookie;

    QCOMPARE(cookie, otherCookie);
    QCOMPARE(cookie, otherCookie);
    QVERIFY(!(cookie != otherCookie));

    QVERIFY(!cookie.isSecure());
    QVERIFY(cookie.isSessionCookie());
    QVERIFY(!cookie.expirationDate().isValid());
    QVERIFY(cookie.domain().isEmpty());
    QVERIFY(cookie.path().isEmpty());
    QVERIFY(cookie.name().isEmpty());
    QVERIFY(cookie.value().isEmpty());

    // change something
    cookie.setName("foo");
    QVERIFY(!(cookie == otherCookie));
    QVERIFY(cookie != otherCookie);

    // test getters and setters:
    QCOMPARE(cookie.name(), QByteArray("foo"));
    cookie.setName(0);
    QVERIFY(cookie.name().isEmpty());

    cookie.setValue("bar");
    QCOMPARE(cookie.value(), QByteArray("bar"));
    cookie.setValue(0);
    QVERIFY(cookie.value().isEmpty());

    cookie.setPath("/");
    QCOMPARE(cookie.path(), QString("/"));
    cookie.setPath(QString());
    QVERIFY(cookie.path().isEmpty());

    cookie.setDomain(".tld");
    QCOMPARE(cookie.domain(), QString(".tld"));
    cookie.setDomain(QString());
    QVERIFY(cookie.domain().isEmpty());

    QDateTime now = QDateTime::currentDateTime();
    cookie.setExpirationDate(now);
    QCOMPARE(cookie.expirationDate(), now);
    QVERIFY(!cookie.isSessionCookie());
    cookie.setExpirationDate(QDateTime());
    QVERIFY(!cookie.expirationDate().isValid());
    QVERIFY(cookie.isSessionCookie());

    cookie.setSecure(true);
    QVERIFY(cookie.isSecure());
    cookie.setSecure(false);
    QVERIFY(!cookie.isSecure());

    QCOMPARE(cookie, otherCookie);
}

void tst_QNetworkCookie::parseSingleCookie_data()
{
    QTest::addColumn<QString>("cookieString");
    QTest::addColumn<QNetworkCookie>("expectedCookie");

    QNetworkCookie cookie;
    cookie.setName("a");
    QTest::newRow("basic") << "a=" << cookie;
    QTest::newRow("basic2") << " a=" << cookie;
    QTest::newRow("basic3") << "a= " << cookie;
    QTest::newRow("basic4") << " a= " << cookie;
    QTest::newRow("basic5") << " a= ;" << cookie;
    QTest::newRow("basic6") << " a=; " << cookie;
    QTest::newRow("basic7") << " a =" << cookie;
    QTest::newRow("basic8") << " a = " << cookie;

    cookie.setValue("b");
    QTest::newRow("with-value") << "a=b" << cookie;
    QTest::newRow("with-value2") << " a=b" << cookie;
    QTest::newRow("with-value3") << "a=b " << cookie;
    QTest::newRow("with-value4") << " a=b " << cookie;
    QTest::newRow("with-value5") << " a=b ;" << cookie;
    QTest::newRow("with-value6") << "a =b" << cookie;
    QTest::newRow("with-value7") << "a= b" << cookie;
    QTest::newRow("with-value8") << "a = b" << cookie;
    QTest::newRow("with-value9") << "a = b " << cookie;

    cookie.setValue("\",\"");
    QTest::newRow("with-value-with-special1") << "a = \",\" " << cookie;
    cookie.setValue("\"");
    QTest::newRow("with-value-with-special2") << "a = \";\" " << cookie;
    cookie.setValue("\" \"");
    QTest::newRow("with-value-with-special3") << "a = \" \" " << cookie;
    cookie.setValue("\"\\\"\"");
    QTest::newRow("with-value-with-special4") << "a = \"\\\"\" " << cookie;
    cookie.setValue("\"\\\"a, b");
    QTest::newRow("with-value-with-special5") << "a = \"\\\"a, b; c\\\"\"" << cookie;
    // RFC6265 states that cookie values shouldn't contain commas, but we still allow them
    // since they are only reserved for future compatibility and other browsers do the same.
    cookie.setValue(",");
    QTest::newRow("with-value-with-special6") << "a = ," << cookie;
    cookie.setValue(",b");
    QTest::newRow("with-value-with-special7") << "a = ,b" << cookie;
    cookie.setValue("b,");
    QTest::newRow("with-value-with-special8") << "a = b," << cookie;

    cookie.setValue("b c");
    QTest::newRow("with-value-with-whitespace") << "a = b c" << cookie;

    cookie.setValue("\"b\"");
    QTest::newRow("quoted-value") << "a = \"b\"" << cookie;
    cookie.setValue("\"b c\"");
    QTest::newRow("quoted-value-with-whitespace") << "a = \"b c\"" << cookie;

    cookie.setValue("b");
    cookie.setSecure(true);
    QTest::newRow("secure") << "a=b;secure" << cookie;
    QTest::newRow("secure2") << "a=b;secure " << cookie;
    QTest::newRow("secure3") << "a=b; secure" << cookie;
    QTest::newRow("secure4") << "a=b; secure " << cookie;
    QTest::newRow("secure5") << "a=b ;secure" << cookie;
    QTest::newRow("secure6") << "a=b ;secure " << cookie;
    QTest::newRow("secure7") << "a=b ; secure " << cookie;
    QTest::newRow("secure8") << "a=b; Secure" << cookie;

    cookie.setSecure(false);
    cookie.setHttpOnly(true);
    QTest::newRow("httponly") << "a=b;httponly" << cookie;
    QTest::newRow("httponly2") << "a=b;HttpOnly " << cookie;
    QTest::newRow("httponly3") << "a=b; httpOnly" << cookie;
    QTest::newRow("httponly4") << "a=b; HttpOnly " << cookie;
    QTest::newRow("httponly5") << "a=b ;HttpOnly" << cookie;
    QTest::newRow("httponly6") << "a=b ;httponly " << cookie;
    QTest::newRow("httponly7") << "a=b ; HttpOnly " << cookie;
    QTest::newRow("httponly8") << "a=b; Httponly" << cookie;

    cookie.setHttpOnly(false);
    cookie.setPath("/");
    QTest::newRow("path1") << "a=b;path=/" << cookie;
    QTest::newRow("path2") << "a=b; path=/" << cookie;
    QTest::newRow("path3") << "a=b;path=/ " << cookie;
    QTest::newRow("path4") << "a=b;path =/ " << cookie;
    QTest::newRow("path5") << "a=b;path= / " << cookie;
    QTest::newRow("path6") << "a=b;path = / " << cookie;
    QTest::newRow("path7") << "a=b;Path = / " << cookie;
    QTest::newRow("path8") << "a=b; PATH = / " << cookie;

    cookie.setPath("/foo");
    QTest::newRow("path9") << "a=b;path=/foo" << cookie;

    // some weird paths:
    cookie.setPath("/with%20spaces");
    QTest::newRow("path-with-spaces") << "a=b;path=/with%20spaces" << cookie;
    QTest::newRow("path-with-spaces2") << "a=b; path=/with%20spaces " << cookie;
    cookie.setPath(QString());
    QTest::newRow("invalid-path-with-spaces3") << "a=b; path=\"/with spaces\"" << cookie;
    QTest::newRow("invalid-path-with-spaces4") << "a=b; path = \"/with spaces\" " << cookie;
    cookie.setPath("/with spaces");
    QTest::newRow("path-with-spaces5") << "a=b; path=/with spaces" << cookie;
    QTest::newRow("path-with-spaces6") << "a=b; path = /with spaces " << cookie;

    cookie.setPath("/with\"Quotes");
    QTest::newRow("path-with-quotes") << "a=b; path = /with\"Quotes" << cookie;
    cookie.setPath(QString());
    QTest::newRow("invalid-path-with-quotes2") << "a=b; path = \"/with\\\"Quotes\"" << cookie;

    cookie.setPath(QString::fromUtf8("/R\303\251sum\303\251"));
    QTest::newRow("path-with-utf8") << QString::fromUtf8("a=b;path=/R\303\251sum\303\251") << cookie;
    cookie.setPath("/R%C3%A9sum%C3%A9");
    QTest::newRow("path-with-utf8-2") << "a=b;path=/R%C3%A9sum%C3%A9" << cookie;

    cookie.setPath(QString());
    cookie.setDomain("qt-project.org");
    QTest::newRow("plain-domain1") << "a=b;domain=qt-project.org" << cookie;
    QTest::newRow("plain-domain2") << "a=b; domain=qt-project.org " << cookie;
    QTest::newRow("plain-domain3") << "a=b;domain=QT-PROJECT.ORG" << cookie;
    QTest::newRow("plain-domain4") << "a=b;DOMAIN = QT-PROJECT.ORG" << cookie;

    cookie.setDomain(".qt-project.org");
    QTest::newRow("dot-domain1") << "a=b;domain=.qt-project.org" << cookie;
    QTest::newRow("dot-domain2") << "a=b; domain=.qt-project.org" << cookie;
    QTest::newRow("dot-domain3") << "a=b; domain=.QT-PROJECT.ORG" << cookie;
    QTest::newRow("dot-domain4") << "a=b; Domain = .QT-PROJECT.ORG" << cookie;

    cookie.setDomain(QString::fromUtf8(".d\303\270gn\303\245pent.troll.no"));
    QTest::newRow("idn-domain1") << "a=b;domain=.xn--dgnpent-gxa2o.troll.no" << cookie;
    QTest::newRow("idn-domain2") << QString::fromUtf8("a=b;domain=.d\303\270gn\303\245pent.troll.no") << cookie;
    QTest::newRow("idn-domain3") << "a=b;domain=.XN--DGNPENT-GXA2O.TROLL.NO" << cookie;
    QTest::newRow("idn-domain4") << QString::fromUtf8("a=b;domain=.D\303\230GN\303\205PENT.troll.NO") << cookie;

    cookie.setDomain(".qt-project.org");
    cookie.setPath("/");
    QTest::newRow("two-fields") << "a=b;domain=.qt-project.org;path=/" << cookie;
    QTest::newRow("two-fields2") << "a=b; domain=.qt-project.org; path=/" << cookie;
    QTest::newRow("two-fields3") << "a=b;   domain=.qt-project.org ; path=/ " << cookie;
    QTest::newRow("two-fields4") << "a=b;path=/; domain=.qt-project.org" << cookie;
    QTest::newRow("two-fields5") << "a=b; path=/  ;   domain=.qt-project.org" << cookie;
    QTest::newRow("two-fields6") << "a=b; path= /  ;   domain =.qt-project.org" << cookie;

    cookie.setSecure(true);
    QTest::newRow("three-fields") << "a=b;domain=.qt-project.org;path=/;secure" << cookie;
    QTest::newRow("three-fields2") << "a=b;secure;path=/;domain=.qt-project.org" << cookie;
    QTest::newRow("three-fields3") << "a=b;secure;domain=.qt-project.org; path=/" << cookie;
    QTest::newRow("three-fields4") << "a = b;secure;domain=.qt-project.org; path=/" << cookie;

    cookie = QNetworkCookie();
    cookie.setName("a");
    cookie.setValue("b");
    cookie.setExpirationDate(QDateTime(QDate(2012, 1, 29), QTime(23, 59, 59), Qt::UTC));
    QTest::newRow("broken-expiration1") << "a=b; expires=Sun, 29-Jan-2012 23:59:59;" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1999, 11, 9), QTime(23, 12, 40), Qt::UTC));
    QTest::newRow("expiration1") << "a=b;expires=Wednesday, 09-Nov-1999 23:12:40 GMT" << cookie;
    QTest::newRow("expiration2") << "a=b;expires=Wed, 09-Nov-1999 23:12:40 GMT" << cookie;
    QTest::newRow("expiration3") << "a=b; expires=Wednesday, 09-Nov-1999 23:12:40 GMT " << cookie;
    QTest::newRow("expiration-utc") << "a=b;expires=Wednesday, 09-Nov-1999 23:12:40 UTC" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 14), QTime(3, 20, 0, 0), Qt::UTC));
    QTest::newRow("time-0") << "a=b;expires=14 Apr 89 03:20" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 14), QTime(3, 20, 12, 0), Qt::UTC));
    QTest::newRow("time-1") << "a=b;expires=14 Apr 89 03:20:12" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 14), QTime(3, 20, 12, 88), Qt::UTC));
    QTest::newRow("time-2") << "a=b;expires=14 Apr 89 03:20:12.88" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 14), QTime(3, 20, 12, 88), Qt::UTC));
    QTest::newRow("time-3") << "a=b;expires=14 Apr 89 03:20:12.88am" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 14), QTime(15, 20, 12, 88), Qt::UTC));
    QTest::newRow("time-4") << "a=b;expires=14 Apr 89 03:20:12.88pm" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 14), QTime(3, 20, 12, 88), Qt::UTC));
    QTest::newRow("time-5") << "a=b;expires=14 Apr 89 03:20:12.88 Am" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 14), QTime(15, 20, 12, 88), Qt::UTC));
    QTest::newRow("time-6") << "a=b;expires=14 Apr 89 03:20:12.88 PM" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 14), QTime(15, 20, 12, 88), Qt::UTC));
    QTest::newRow("time-7") << "a=b;expires=14 Apr 89 3:20:12.88 PM" << cookie;

    // normal months
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-1") << "a=b;expires=Jan 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 2, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-2") << "a=b;expires=Feb 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 3, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-3") << "a=b;expires=mar 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-4") << "a=b;expires=Apr 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 5, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-5") << "a=b;expires=May 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 6, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-6") << "a=b;expires=Jun 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 7, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-7") << "a=b;expires=Jul 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 8, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-8") << "a=b;expires=Aug 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 9, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-9") << "a=b;expires=Sep 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 10, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-10") << "a=b;expires=Oct 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 11, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-11") << "a=b;expires=Nov 1 89 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 12, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-12") << "a=b;expires=Dec 1 89 1:1" << cookie;

    // extra months
    cookie.setExpirationDate(QDateTime(QDate(1989, 12, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-13") << "a=b;expires=December 1 89 1:1" << cookie;
    QTest::newRow("months-14") << "a=b;expires=1 89 1:1 Dec" << cookie;
    //cookie.setExpirationDate(QDateTime());
    //QTest::newRow("months-15") << "a=b;expires=1 89 1:1 De" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(2024, 2, 29), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-16") << "a=b;expires=2024 29 Feb 1:1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(2024, 2, 29), QTime(1, 1), Qt::UTC));
    QTest::newRow("months-17") << "a=b;expires=Fri, 29-Feb-2024 01:01:00 GMT" << cookie;
    QTest::newRow("months-18") << "a=b;expires=2024 29 Feb 1:1 GMT" << cookie;

    // normal offsets
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-0") << "a=b;expires=Jan 1 89 8:0 PST" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-1") << "a=b;expires=Jan 1 89 8:0 PDT" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-2") << "a=b;expires=Jan 1 89 7:0 MST" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-3") << "a=b;expires=Jan 1 89 7:0 MDT" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-4") << "a=b;expires=Jan 1 89 6:0 CST" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-5") << "a=b;expires=Jan 1 89 6:0 CDT" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-6") << "a=b;expires=Jan 1 89 5:0 EST" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-7") << "a=b;expires=Jan 1 89 5:0 EDT" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-8") << "a=b;expires=Jan 1 89 4:0 AST" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-9") << "a=b;expires=Jan 1 89 3:0 NST" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-10") << "a=b;expires=Jan 1 89 0:0 GMT" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-11") << "a=b;expires=Jan 1 89 0:0 BST" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 2), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-12") << "a=b;expires=Jan 1 89 23:0 MET" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 2), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-13") << "a=b;expires=Jan 1 89 22:0 EET" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 2), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-14") << "a=b;expires=Jan 1 89 15:0 JST" << cookie;

    // extra offsets
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 2), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-15") << "a=b;expires=Jan 1 89 15:0 JST+1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(1, 0), Qt::UTC));
    QTest::newRow("zoneoffset-16") << "a=b;expires=Jan 1 89 0:0 GMT+1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-17") << "a=b;expires=Jan 1 89 1:0 GMT-1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(1, 0), Qt::UTC));
    QTest::newRow("zoneoffset-18") << "a=b;expires=Jan 1 89 0:0 GMT+01" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(1, 5), Qt::UTC));
    QTest::newRow("zoneoffset-19") << "a=b;expires=Jan 1 89 0:0 GMT+0105" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-20") << "a=b;expires=Jan 1 89 0:0 GMT+015" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-21") << "a=b;expires=Jan 1 89 0:0 GM" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-22") << "a=b;expires=Jan 1 89 0:0 GMT" << cookie;

    // offsets from gmt
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(1, 0), Qt::UTC));
    QTest::newRow("zoneoffset-23") << "a=b;expires=Jan 1 89 0:0 +1" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(1, 0), Qt::UTC));
    QTest::newRow("zoneoffset-24") << "a=b;expires=Jan 1 89 0:0 +01" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(1, 1), Qt::UTC));
    QTest::newRow("zoneoffset-25") << "a=b;expires=Jan 1 89 0:0 +0101" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("zoneoffset-26") << "a=b;expires=Jan 1 89 1:0 -1" << cookie;

    // Y2k
    cookie.setExpirationDate(QDateTime(QDate(2000, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("year-0") << "a=b;expires=Jan 1 00 0:0" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1970, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("year-1") << "a=b;expires=Jan 1 70 0:0" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1971, 1, 1), QTime(0, 0), Qt::UTC));
    QTest::newRow("year-2") << "a=b;expires=Jan 1 71 0:0" << cookie;

    // Day, month, year
    cookie.setExpirationDate(QDateTime(QDate(2013, 1, 2), QTime(0, 0), Qt::UTC));
    QTest::newRow("date-0") << "a=b;expires=Jan 2 13 0:0" << cookie;
    QTest::newRow("date-1") << "a=b;expires=1-2-13 0:0" << cookie;
    QTest::newRow("date-2") << "a=b;expires=1/2/13 0:0" << cookie;
    QTest::newRow("date-3") << "a=b;expires=Jan 2 13 0:0" << cookie;
    QTest::newRow("date-4") << "a=b;expires=Jan 2, 13 0:0" << cookie;
    QTest::newRow("date-5") << "a=b;expires=1-2-13 0:0" << cookie;
    QTest::newRow("date-6") << "a=b;expires=1/2/13 0:0" << cookie;

    // Known Year, determine month and day
    cookie.setExpirationDate(QDateTime(QDate(1995, 1, 13), QTime(0, 0), Qt::UTC));
    QTest::newRow("knownyear-0") << "a=b;expires=13/1/95 0:0" << cookie;
    QTest::newRow("knownyear-1") << "a=b;expires=95/13/1 0:0" << cookie;
    QTest::newRow("knownyear-2") << "a=b;expires=1995/1/13 0:0" << cookie;
    QTest::newRow("knownyear-3") << "a=b;expires=1995/13/1 0:0" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1995, 1, 2), QTime(0, 0), Qt::UTC));
    QTest::newRow("knownyear-4") << "a=b;expires=1/2/95 0:0" << cookie;
    QTest::newRow("knownyear-5") << "a=b;expires=95/1/2 0:0" << cookie;

    // Known Year, Known day, determining month
    cookie.setExpirationDate(QDateTime(QDate(1995, 1, 13), QTime(0, 0), Qt::UTC));
    QTest::newRow("knownYD-0") << "a=b;expires=13/1/95 0:0" << cookie;
    QTest::newRow("knownYD-1") << "a=b;expires=1/13/95 0:0" << cookie;
    QTest::newRow("knownYD-2") << "a=b;expires=95/13/1 0:0" << cookie;
    QTest::newRow("knownYD-3") << "a=b;expires=95/1/13 0:0" << cookie;

    // Month comes before Year
    cookie.setExpirationDate(QDateTime(QDate(2021, 03, 26), QTime(0, 0), Qt::UTC));
    QTest::newRow("month-0") << "a=b;expires=26/03/21 0:0" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(2015, 12, 30), QTime(16, 25, 0, 0), Qt::UTC));
    QTest::newRow("month-1") << "a=b;expires=wed 16:25pm December 2015 30" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(2031, 11, 11), QTime(16, 25, 0, 0), Qt::UTC));
    QTest::newRow("month-2") << "a=b;expires=16:25 11 31 11" << cookie;

    // The very ambiguous cases
    // Matching Firefox's behavior of guessing month, day, year in those cases
    cookie.setExpirationDate(QDateTime(QDate(2013, 10, 2), QTime(0, 0), Qt::UTC));
    QTest::newRow("ambiguousd-0") << "a=b;expires=10/2/13 0:0" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(2013, 2, 10), QTime(0, 0), Qt::UTC));
    QTest::newRow("ambiguousd-1") << "a=b;expires=2/10/13 0:0" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(2010, 2, 3), QTime(0, 0), Qt::UTC));
    QTest::newRow("ambiguousd-2") << "a=b;expires=2/3/10 0:0" << cookie;

    // FYI If you try these in Firefox it won't set a cookie for the following two string
    // because 03 is turned into the year at which point it is expired
    cookie.setExpirationDate(QDateTime(QDate(2003, 2, 10), QTime(0, 0), Qt::UTC));
    QTest::newRow("ambiguousd-3") << "a=b;expires=2/10/3 0:0" << cookie;
    cookie.setExpirationDate(QDateTime(QDate(2003, 10, 2), QTime(0, 0), Qt::UTC));
    QTest::newRow("ambiguousd-4") << "a=b;expires=10/2/3 0:0" << cookie;

    // These are the cookies that firefox's source says it can parse
    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 14), QTime(3, 20, 0, 0), Qt::UTC));
    QTest::newRow("firefox-0") << "a=b;expires=14 Apr 89 03:20" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1989, 4, 14), QTime(3, 20, 0, 0), Qt::UTC));
    QTest::newRow("firefox-1") << "a=b;expires=14 Apr 89 03:20 GMT" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1989, 3, 17), QTime(4, 1, 33, 0), Qt::UTC));
    QTest::newRow("firefox-2") << "a=b;expires=Fri, 17 Mar 89 4:01:33" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1989, 3, 17), QTime(4, 1, 0, 0), Qt::UTC));
    QTest::newRow("firefox-3") << "a=b;expires=Fri, 17 Mar 89 4:01 GMT" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 16), QTime(16-8, 12, 0, 0), Qt::UTC));
    QTest::newRow("firefox-4") << "a=b;expires=Mon Jan 16 16:12 PDT 1989" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1989, 1, 16), QTime(17, 42, 0, 0), Qt::UTC));
    QTest::newRow("firefox-5") << "a=b;expires=Mon Jan 16 16:12 +0130 1989" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1992, 5, 6), QTime(16-9, 41, 0, 0), Qt::UTC));
    QTest::newRow("firefox-6") << "a=b;expires=6 May 1992 16:41-JST (Wednesday)" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1993, 8, 22), QTime(10, 59, 12, 82), Qt::UTC));
    QTest::newRow("firefox-7") << "a=b;expires=22-AUG-1993 10:59:12.82" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1993, 8, 22), QTime(22, 59, 0, 0), Qt::UTC));
    QTest::newRow("firefox-8") << "a=b;expires=22-AUG-1993 10:59pm" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1993, 8, 22), QTime(12, 59, 0, 0), Qt::UTC));
    QTest::newRow("firefox-9") << "a=b;expires=22-AUG-1993 12:59am" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1993, 8, 22), QTime(12, 59, 0, 0), Qt::UTC));
    QTest::newRow("firefox-10") << "a=b;expires=22-AUG-1993 12:59 PM" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1995, 8, 4), QTime(15, 54, 0, 0), Qt::UTC));
    QTest::newRow("firefox-11") << "a=b;expires=Friday, August 04, 1995 3:54 PM" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1995, 6, 21), QTime(16, 24, 34, 0), Qt::UTC));
    QTest::newRow("firefox-12") << "a=b;expires=06/21/95 04:24:34 PM" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1995, 6, 20), QTime(21, 7, 0, 0), Qt::UTC));
    QTest::newRow("firefox-13") << "a=b;expires=20/06/95 21:07" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1995, 6, 8), QTime(19-5, 32, 48, 0), Qt::UTC));
    QTest::newRow("firefox-14") << "a=b;expires=95-06-08 19:32:48 EDT" << cookie;

    // Edge cases caught by fuzzing
    // These are about the default cause creates dates that don't exits
    cookie.setExpirationDate(QDateTime(QDate(2030, 2, 25), QTime(1, 1, 0, 0), Qt::UTC));
    QTest::newRow("fuzz-0") << "a=b; expires=30 -000002  1:1 25;" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(2031, 11, 20), QTime(1, 1, 0, 0), Qt::UTC));
    QTest::newRow("fuzz-1") << "a=b; expires=31 11 20 1:1;" << cookie;

    // April only has 30 days
    cookie.setExpirationDate(QDateTime(QDate(2031, 4, 30), QTime(1, 1, 0, 0), Qt::UTC));
    QTest::newRow("fuzz-2") << "a=b; expires=31 30 4 1:1" << cookie;

    // 9 must be the month so 31 can't be the day
    cookie.setExpirationDate(QDateTime(QDate(2031, 9, 21), QTime(1, 1, 0, 0), Qt::UTC));
    QTest::newRow("fuzz-3") << "a=b; expires=31 21 9 1:1" << cookie;

    // Year is known, then fallback to defaults of filling in month and day
    cookie.setExpirationDate(QDateTime(QDate(2031, 11, 1), QTime(1, 1, 0, 0), Qt::UTC));
    QTest::newRow("fuzz-4") << "a=b; expires=31 11 01 1:1" << cookie;

    // 2 must be the month so 30 can't be the day
    cookie.setExpirationDate(QDateTime(QDate(2030, 2, 20), QTime(1, 1, 0, 0), Qt::UTC));
    QTest::newRow("fuzz-5") << "a=b; expires=30 02 20 1:1" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(2021, 12, 22), QTime(1, 1, 0, 0), Qt::UTC));
    QTest::newRow("fuzz-6") << "a=b; expires=2021 12 22 1:1" << cookie;

    cookie.setExpirationDate(QDateTime(QDate(2029, 2, 23), QTime(1, 1, 0, 0), Qt::UTC));
    QTest::newRow("fuzz-7") << "a=b; expires=29 23 Feb 1:1" << cookie;

    // 11 and 6 don't have 31 days
    cookie.setExpirationDate(QDateTime(QDate(2031, 11, 06), QTime(1, 1, 0, 0), Qt::UTC));
    QTest::newRow("fuzz-8") << "a=b; expires=31 11 06 1:1" << cookie;

    // two-digit years:
    // from 70 until 99, we assume 20th century
    cookie.setExpirationDate(QDateTime(QDate(1999, 11, 9), QTime(23, 12, 40), Qt::UTC));
    QTest::newRow("expiration-2digit1") << "a=b; expires=Wednesday, 09-Nov-99 23:12:40 GMT " << cookie;
    cookie.setExpirationDate(QDateTime(QDate(1970, 1, 1), QTime(23, 12, 40), Qt::UTC));
    QTest::newRow("expiration-2digit2") << "a=b; expires=Thursday, 01-Jan-70 23:12:40 GMT " << cookie;
    // from 00 until 69, we assume 21st century
    cookie.setExpirationDate(QDateTime(QDate(2000, 1, 1), QTime(23, 12, 40), Qt::UTC));
    QTest::newRow("expiration-2digit3") << "a=b; expires=Saturday, 01-Jan-00 23:12:40 GMT " << cookie;
    cookie.setExpirationDate(QDateTime(QDate(2020, 1, 1), QTime(23, 12, 40), Qt::UTC));
    QTest::newRow("expiration-2digit4") << "a=b; expires=Wednesday, 01-Jan-20 23:12:40 GMT " << cookie;
    cookie.setExpirationDate(QDateTime(QDate(2069, 1, 1), QTime(23, 12, 40), Qt::UTC));
    QTest::newRow("expiration-2digit5") << "a=b; expires=Wednesday, 01-Jan-69 23:12:40 GMT " << cookie;

    cookie.setExpirationDate(QDateTime(QDate(1999, 11, 9), QTime(23, 12, 40), Qt::UTC));

    cookie.setPath("/");
    QTest::newRow("expires+path") << "a=b; expires=Wed, 09-Nov-1999 23:12:40 GMT; path=/" << cookie;
    QTest::newRow("path+expires") << "a=b; path=/;expires=Wed, 09-Nov-1999 23:12:40 GMT " << cookie;

    cookie.setDomain(".qt-project.org");
    QTest::newRow("full") << "a=b; domain=.qt-project.org;expires=Wed, 09-Nov-1999 23:12:40 GMT;path=/" << cookie;
    QTest::newRow("full2") << "a=b;path=/; expires=Wed, 09-Nov-1999 23:12:40 GMT ;domain=.qt-project.org" << cookie;

    // cookies obtained from the network:
    cookie = QNetworkCookie("__siteid", "1");
    cookie.setPath("/");
    cookie.setExpirationDate(QDateTime(QDate(9999, 12, 31), QTime(23, 59, 59), Qt::UTC));
    QTest::newRow("network2") << "__siteid=1; expires=Fri, 31-Dec-9999 23:59:59 GMT; path=/" << cookie;

    cookie = QNetworkCookie("YM.LC", "v=2&m=9993_262838_159_1558_1063_0_5649_4012_3776161073,9426_260205_549_1295_1336_0_5141_4738_3922731647,6733_258196_952_1364_643_0_3560_-1_0,3677_237633_1294_1294_19267_0_3244_29483_4102206176,1315_235149_1693_1541_941_0_3224_1691_1861378060,1858_214311_2100_1298_19538_0_2873_30900_716411652,6258_212007_2506_1285_1017_0_2868_3606_4288540264,3743_207884_2895_1362_2759_0_2545_7114_3388520216,2654_205253_3257_1297_1332_0_2504_4682_3048534803,1891_184881_3660_1291_19079_0_978_29178_2592538685&f=1&n=20&s=date&o=down&e=1196548712&b=Inbox&u=removed");
    cookie.setPath("/");
    cookie.setDomain("mail.yahoo.com");
    QTest::newRow("network3") << "YM.LC=v=2&m=9993_262838_159_1558_1063_0_5649_4012_3776161073,9426_260205_549_1295_1336_0_5141_4738_3922731647,6733_258196_952_1364_643_0_3560_-1_0,3677_237633_1294_1294_19267_0_3244_29483_4102206176,1315_235149_1693_1541_941_0_3224_1691_1861378060,1858_214311_2100_1298_19538_0_2873_30900_716411652,6258_212007_2506_1285_1017_0_2868_3606_4288540264,3743_207884_2895_1362_2759_0_2545_7114_3388520216,2654_205253_3257_1297_1332_0_2504_4682_3048534803,1891_184881_3660_1291_19079_0_978_29178_2592538685&f=1&n=20&s=date&o=down&e=1196548712&b=Inbox&u=removed; path=/; domain=mail.yahoo.com" << cookie;

    cookie = QNetworkCookie("__ac", "\"c2hhdXNtYW46U2FTYW80Wm8%3D\"");
    cookie.setPath("/");
    cookie.setExpirationDate(QDateTime(QDate(2008, 8, 30), QTime(20, 21, 49), Qt::UTC));
    QTest::newRow("network4") << "__ac=\"c2hhdXNtYW46U2FTYW80Wm8%3D\"; Path=/; Expires=Sat, 30 Aug 2008 20:21:49 +0000" << cookie;

    // linkedin.com sends cookies in quotes and expects the cookie in quotes
    cookie = QNetworkCookie("leo_auth_token", "\"GST:UroVXaxYA3sVSkoVjMNH9bj4dZxVzK2yekgrAUxMfUsyLTNyPjoP60:1298974875:b675566ae32ab36d7a708c0efbf446a5c22b9fca\"");
    cookie.setPath("/");
    cookie.setExpirationDate(QDateTime(QDate(2011, 3, 1), QTime(10, 51, 14), Qt::UTC));
    QTest::newRow("network5") << "leo_auth_token=\"GST:UroVXaxYA3sVSkoVjMNH9bj4dZxVzK2yekgrAUxMfUsyLTNyPjoP60:1298974875:b675566ae32ab36d7a708c0efbf446a5c22b9fca\"; Version=1; Max-Age=1799; Expires=Tue, 01-Mar-2011 10:51:14 GMT; Path=/" << cookie;

    // cookie containing JSON data (illegal for server, client should accept) - QTBUG-26002
    cookie = QNetworkCookie("xploreCookies", "{\"desktopReportingUrl\":\"null\",\"userIds\":\"1938850\",\"contactEmail\":\"NA\",\"contactName\":\"NA\",\"enterpriseLicenseId\":\"0\",\"openUrlTxt\":\"NA\",\"customerSurvey\":\"NA\",\"standardsLicenseId\":\"0\",\"openUrl\":\"NA\",\"smallBusinessLicenseId\":\"0\", \"instImage\":\"1938850_univ skovde.gif\",\"isMember\":\"false\",\"products\":\"IEL|VDE|\",\"openUrlImgLoc\":\"NA\",\"isIp\":\"true\",\"instName\": \"University of XXXXXX\",\"oldSessionKey\":\"LmZ8hlXo5a9uZx2Fnyw1564T1ZOWMnf3Dk*oDx2FQHwbg6RYefyrhC8PL2wx3Dx3D-18x2d8723DyqXRnkILyGpmx2Fh9wgx3Dx3Dc2lAOhHqGSKT78xxGwXZxxCgx3Dx3D-XrL4FnIlW2OPkqtVJq0LkQx3Dx3D-tujOLwhFqtX7Pa7HGqmCXQx3Dx3D\", \"isChargebackUser\":\"false\",\"isInst\":\"true\"}");
    cookie.setDomain(".ieee.org");
    cookie.setPath("/");
    QTest::newRow("network6") << "xploreCookies={\"desktopReportingUrl\":\"null\",\"userIds\":\"1938850\",\"contactEmail\":\"NA\",\"contactName\":\"NA\",\"enterpriseLicenseId\":\"0\",\"openUrlTxt\":\"NA\",\"customerSurvey\":\"NA\",\"standardsLicenseId\":\"0\",\"openUrl\":\"NA\",\"smallBusinessLicenseId\":\"0\", \"instImage\":\"1938850_univ skovde.gif\",\"isMember\":\"false\",\"products\":\"IEL|VDE|\",\"openUrlImgLoc\":\"NA\",\"isIp\":\"true\",\"instName\": \"University of XXXXXX\",\"oldSessionKey\":\"LmZ8hlXo5a9uZx2Fnyw1564T1ZOWMnf3Dk*oDx2FQHwbg6RYefyrhC8PL2wx3Dx3D-18x2d8723DyqXRnkILyGpmx2Fh9wgx3Dx3Dc2lAOhHqGSKT78xxGwXZxxCgx3Dx3D-XrL4FnIlW2OPkqtVJq0LkQx3Dx3D-tujOLwhFqtX7Pa7HGqmCXQx3Dx3D\", \"isChargebackUser\":\"false\",\"isInst\":\"true\"}; domain=.ieee.org; path=/" << cookie;
}

void tst_QNetworkCookie::parseSingleCookie()
{
    QFETCH(QString, cookieString);
    QFETCH(QNetworkCookie, expectedCookie);

    QList<QNetworkCookie> result = QNetworkCookie::parseCookies(cookieString.toUtf8());

    //QEXPECT_FAIL("network2", "QDateTime parsing problem: the date is beyond year 8000", Abort);
    QCOMPARE(result.count(), 1);
    QCOMPARE(result.at(0), expectedCookie);

    result = QNetworkCookie::parseCookies(result.at(0).toRawForm());
    QCOMPARE(result.count(), 1);

    // Drop any millisecond information, if there's any
    QDateTime dt = expectedCookie.expirationDate();
    if (dt.isValid()) {
        QTime t = dt.time();
        dt.setTime(t.addMSecs(-t.msec()));
        expectedCookie.setExpirationDate(dt);
    }

    QCOMPARE(result.at(0), expectedCookie);
}

void tst_QNetworkCookie::parseMultipleCookies_data()
{
    QTest::addColumn<QString>("cookieString");
    QTest::addColumn<QList<QNetworkCookie> >("expectedCookies");

    QList<QNetworkCookie> list;
    QTest::newRow("empty") << "" << list;

    // these are technically empty cookies:
    QTest::newRow("invalid-01") << ";" << list;
    QTest::newRow("invalid-02") << " " << list;
    QTest::newRow("invalid-03") << " ," << list;
    QTest::newRow("invalid-04") << ";;,, ; ; , , ; , ;" << list;

    // these are really invalid:
    // reason: malformed NAME=VALUE pair
    QTest::newRow("invalid-05") << "foo" << list;
    QTest::newRow("invalid-06") << "=b" << list;
    QTest::newRow("invalid-07") << ";path=/" << list;

    // these should be accepted by RFC6265 but ignoring the expires field
    // reason: malformed expiration date string
    QNetworkCookie datelessCookie;
    datelessCookie.setName("a");
    datelessCookie.setValue("b");
    list << datelessCookie;
    QTest::newRow("expiration-empty") << "a=b;expires=" << list;
    QTest::newRow("expiration-invalid-01") << "a=b;expires=foobar" << list;
    QTest::newRow("expiration-invalid-02") << "a=b;expires=foobar, abc" << list;
    QTest::newRow("expiration-invalid-03") << "a=b; expires=123" << list; // used to ASSERT
    datelessCookie.setPath("/");
    list.clear();
    list << datelessCookie;
    QTest::newRow("expiration-invalid-04") << "a=b;expires=foobar, dd-mmm-yyyy hh:mm:ss GMT; path=/" << list;
    QTest::newRow("expiration-invalid-05") << "a=b;expires=foobar, 32-Caz-1999 24:01:60 GMT; path=/" << list;

    // cookies obtained from the network:
    QNetworkCookie cookie;
    cookie = QNetworkCookie("id", "51706646077999719");
    cookie.setDomain(".bluestreak.com");
    cookie.setPath("/");
    cookie.setExpirationDate(QDateTime(QDate(2017, 12, 05), QTime(9, 11, 7), Qt::UTC));
    list << cookie;
    cookie.setName("bb");
    cookie.setValue("\\\"K14144t\\\"_AAQ\\\"ototrK_A_ttot44AQ4KwoRQtoto|");
    list << cookie;
    cookie.setName("adv");
    cookie.setValue(QByteArray());
    list << cookie;
    QTest::newRow("network1") << "id=51706646077999719 bb=\"K14144t\"_AAQ\"ototrK_A_ttot44AQ4KwoRQtoto| adv=; Domain=.bluestreak.com; expires=Tuesday 05-Dec-2017 09:11:07 GMT; path=/;" << list;

    QNetworkCookie cookieA;
    cookieA.setName("a");
    cookieA.setValue("b");

    QNetworkCookie cookieB;
    cookieB.setName("c");
    cookieB.setValue("d");

    // NewLine
    cookieA.setExpirationDate(QDateTime(QDate(2009, 3, 10), QTime(7, 0, 0, 0), Qt::UTC));
    cookieB.setExpirationDate(QDateTime(QDate(2009, 3, 20), QTime(7, 0, 0, 0), Qt::UTC));
    list = QList<QNetworkCookie>() << cookieA << cookieB;
    QTest::newRow("real-0") << "a=b; expires=Tue Mar 10 07:00:00 2009 GMT\nc=d; expires=Fri Mar 20 07:00:00 2009 GMT" << list;
    QTest::newRow("real-1") << "a=b; expires=Tue Mar 10 07:00:00 2009 GMT\n\nc=d; expires=Fri Mar 20 07:00:00 2009 GMT" << list;
    QTest::newRow("real-2") << "a=b; expires=Mar 10 07:00:00 2009 GMT, Tue\nc=d; expires=Fri Mar 20 07:00:00 2009 GMT" << list;

    // Match firefox's behavior
    cookieA.setPath("/foo");
    list = QList<QNetworkCookie>() << cookieA << cookieB;
    QTest::newRow("real-3") << "a=b; expires=Mar 10 07:00:00 2009 GMT, Tue; path=/foo\nc=d; expires=Fri Mar 20 07:00:00 2009 GMT" << list;

    // do not accept cookies with non-alphanumeric characters in domain field (QTBUG-11029)
    cookie = QNetworkCookie("NonAlphNumDomName", "NonAlphNumDomValue");
    cookie.setDomain("!@#$%^&*();:."); // the ';' is actually problematic, because it is a separator
    list = QList<QNetworkCookie>();
    QTest::newRow("domain-non-alpha-numeric") << "NonAlphNumDomName=NonAlphNumDomValue; domain=!@#$%^&*()" << list;
}

void tst_QNetworkCookie::parseMultipleCookies()
{
    QFETCH(QString, cookieString);
    QFETCH(QList<QNetworkCookie>, expectedCookies);

    QList<QNetworkCookie> result = QNetworkCookie::parseCookies(cookieString.toUtf8());

    QEXPECT_FAIL("network1", "Apparently multiple cookies set in one request (and an invalid date)", Abort);
    QCOMPARE(result, expectedCookies);
}

QTEST_MAIN(tst_QNetworkCookie)
#include "tst_qnetworkcookie.moc"
