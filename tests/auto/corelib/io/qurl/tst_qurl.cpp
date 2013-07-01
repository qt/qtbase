/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Intel Corporation.
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

#define QT_DEPRECATED
#define QT_DISABLE_DEPRECATED_BEFORE 0
#include <qurl.h>
#include <QtTest/QtTest>
#include <QtCore/QDebug>

#include <qcoreapplication.h>

#include <qfileinfo.h>
#include <qtextcodec.h>
#include <qmap.h>

Q_DECLARE_METATYPE(QUrl::FormattingOptions)

class tst_QUrl : public QObject
{
    Q_OBJECT

private slots:
    void effectiveTLDs_data();
    void effectiveTLDs();
    void getSetCheck();
    void constructing();
    void hashInPath();
    void unc();
    void assignment();
    void comparison();
    void comparison2_data();
    void comparison2();
    void copying();
    void setUrl();
    void i18n_data();
    void i18n();
    void resolving_data();
    void resolving();
    void toString_data();
    void toString();
    void toString_constructed_data();
    void toString_constructed();
    void toAndFromStringList_data();
    void toAndFromStringList();
    void isParentOf_data();
    void isParentOf();
    void toLocalFile_data();
    void toLocalFile();
    void fromLocalFile_data();
    void fromLocalFile();
    void relative();
    void compat_legacy();
    void compat_constructor_01_data();
    void compat_constructor_01();
    void compat_constructor_02_data();
    void compat_constructor_02();
    void compat_constructor_03_data();
    void compat_constructor_03();
    void compat_isValid_01_data();
    void compat_isValid_01();
    void compat_isValid_02_data();
    void compat_isValid_02();
    void compat_path_data();
    void compat_path();
    void compat_fileName_data();
    void compat_fileName();
    void compat_decode_data();
    void compat_decode();
    void compat_encode_data();
    void compat_encode();
    void percentEncoding_data();
    void percentEncoding();
    void swap();
    void symmetry();
    void ipv6_data();
    void ipv6();
    void ipv6_2_data();
    void ipv6_2();
    void moreIpv6();
    void toPercentEncoding_data();
    void toPercentEncoding();
    void isRelative_data();
    void isRelative();
    void hasQuery_data();
    void hasQuery();
    void nameprep();
    void isValid();
    void schemeValidator_data();
    void schemeValidator();
    void invalidSchemeValidator();
    void strictParser_data();
    void strictParser();
    void tolerantParser();
    void correctEncodedMistakes_data();
    void correctEncodedMistakes();
    void correctDecodedMistakes_data();
    void correctDecodedMistakes();
    void tldRestrictions_data();
    void tldRestrictions();
    void emptyQueryOrFragment();
    void hasFragment_data();
    void hasFragment();
    void setEncodedFragment_data();
    void setEncodedFragment();
    void fromEncoded();
    void stripTrailingSlash_data();
    void stripTrailingSlash();
    void hosts_data();
    void hosts();
    void setPort();
    void toEncoded_data();
    void toEncoded();
    void setAuthority_data();
    void setAuthority();
    void setEmptyAuthority_data();
    void setEmptyAuthority();
    void clear();
    void resolvedWithAbsoluteSchemes() const;
    void resolvedWithAbsoluteSchemes_data() const;
    void binaryData_data();
    void binaryData();
    void fromUserInput_data();
    void fromUserInput();
    void isEmptyForEncodedUrl();
    void toEncodedNotUsingUninitializedPath();
    void emptyAuthorityRemovesExistingAuthority();
    void acceptEmptyAuthoritySegments();
    void lowercasesScheme();
    void componentEncodings_data();
    void componentEncodings();
    void setComponents_data();
    void setComponents();
    void streaming_data();
    void streaming();
    void detach();
    void testThreading();

private:
    void testThreadingHelper();
};

// Testing get/set functions
void tst_QUrl::getSetCheck()
{
    QUrl obj1;
    // int QUrl::port()
    // void QUrl::setPort(int)
    obj1.setPort(0);
    QCOMPARE(0, obj1.port());

    obj1.setPort(INT_MIN);
    QCOMPARE(-1, obj1.port()); // Out of range, -1

    obj1.setPort(INT_MAX);
    QCOMPARE(-1, obj1.port()); // Out of range, -1

    obj1.setPort(1234);
    QCOMPARE(1234, obj1.port());

    // static QStringList QUrl::idnWhitelist()
    // static void QUrl::setIdnWhitelist(QStringList)
    QStringList original = QUrl::idnWhitelist(); // save for later

    QUrl::setIdnWhitelist(QStringList());
    QCOMPARE(QUrl::idnWhitelist(), QStringList());

    QStringList norway; norway << "no";
    QUrl::setIdnWhitelist(norway);
    QCOMPARE(QUrl::idnWhitelist(), norway);

    QStringList modified = original;
    modified << "foo";
    QUrl::setIdnWhitelist(modified);
    QCOMPARE(QUrl::idnWhitelist(), modified);

    // reset to the original
    QUrl::setIdnWhitelist(original);
    QCOMPARE(QUrl::idnWhitelist(), original);
}

void tst_QUrl::constructing()
{
    QUrl url;
    QVERIFY(!url.isValid());
    QVERIFY(url.isEmpty());
    QCOMPARE(url.port(), -1);
    QCOMPARE(url.toString(), QString());
    QVERIFY(url == url);
    QVERIFY(!(url < url));

    QUrl justHost("qt-project.org");
    QVERIFY(!justHost.isEmpty());
    QVERIFY(justHost.host().isEmpty());
    QCOMPARE(justHost.path(), QString::fromLatin1("qt-project.org"));

    QUrl hostWithSlashes("//qt-project.org");
    QVERIFY(hostWithSlashes.path().isEmpty());
    QCOMPARE(hostWithSlashes.host(), QString::fromLatin1("qt-project.org"));
}

void tst_QUrl::hashInPath()
{
    QUrl withHashInPath;
    withHashInPath.setPath(QString::fromLatin1("hi#mum.txt"));
    QCOMPARE(withHashInPath.path(), QString::fromLatin1("hi#mum.txt"));
    QCOMPARE(withHashInPath.toEncoded(), QByteArray("hi%23mum.txt"));
    QCOMPARE(withHashInPath.toString(), QString("hi%23mum.txt"));
    QCOMPARE(withHashInPath.toDisplayString(QUrl::PreferLocalFile), QString("hi%23mum.txt"));

    QUrl fromHashInPath = QUrl::fromEncoded(withHashInPath.toEncoded());
    QVERIFY(withHashInPath == fromHashInPath);

    const QUrl localWithHash = QUrl::fromLocalFile("/hi#mum.txt");
    QCOMPARE(localWithHash.path(), QString::fromLatin1("/hi#mum.txt"));
    QCOMPARE(localWithHash.toEncoded(), QByteArray("file:///hi%23mum.txt"));
    QCOMPARE(localWithHash.toString(), QString("file:///hi%23mum.txt"));
    QCOMPARE(localWithHash.path(), QString::fromLatin1("/hi#mum.txt"));
    QCOMPARE(localWithHash.toString(QUrl::PreferLocalFile), QString("/hi#mum.txt"));
    QCOMPARE(localWithHash.toDisplayString(QUrl::PreferLocalFile), QString("/hi#mum.txt"));
}

void tst_QUrl::unc()
{
    QUrl buildUNC;
    buildUNC.setScheme(QString::fromLatin1("file"));
    buildUNC.setHost(QString::fromLatin1("somehost"));
    buildUNC.setPath(QString::fromLatin1("somepath"));
    QCOMPARE(buildUNC.toLocalFile(), QString::fromLatin1("//somehost/somepath"));
    buildUNC.toEncoded();
    QVERIFY(!buildUNC.isEmpty());
}

void tst_QUrl::assignment()
{
    QUrl url("http://qt-project.org/");
    QVERIFY(url.isValid());

    QUrl copy;
    copy = url;

    QVERIFY(url == copy);
}

void tst_QUrl::comparison()
{
    QUrl url1("http://qt-project.org/");
    QVERIFY(url1.isValid());

    QUrl url2("http://qt-project.org/");
    QVERIFY(url2.isValid());

    QVERIFY(url1 == url2);
    QVERIFY(!(url1 < url2));
    QVERIFY(!(url2 < url1));

    // 6.2.2 Syntax-based Normalization
    QUrl url3 = QUrl::fromEncoded("example://a/b/c/%7Bfoo%7D");
    QUrl url4 = QUrl::fromEncoded("eXAMPLE://a/./b/../b/%63/%7bfoo%7d");
    QEXPECT_FAIL("", "Normalization not implemented, will probably not be implemented like this", Continue);
    QCOMPARE(url3, url4);

    // 6.2.2.1 Make sure hexdecimal characters in percent encoding are
    // treated case-insensitively
    QUrl url5;
    url5.setEncodedQuery("a=%2a");
    QUrl url6;
    url6.setEncodedQuery("a=%2A");
    QVERIFY(url5 == url6);

    QUrl url7;
    url7.setEncodedQuery("a=C");
    QUrl url8;
    url8.setEncodedQuery("a=c");
    QVERIFY(url7 != url8);
    QVERIFY(url7 < url8);
}

void tst_QUrl::comparison2_data()
{
    QTest::addColumn<QUrl>("url1");
    QTest::addColumn<QUrl>("url2");
    QTest::addColumn<int>("ordering"); // like strcmp

    QTest::newRow("null-null") << QUrl() << QUrl() << 0;

    QUrl empty;
    empty.setPath("/hello"); // ensure it has detached
    empty.setPath(QString());
    QTest::newRow("null-empty") << QUrl() << empty << 0;

    QTest::newRow("scheme-null") << QUrl("x:") << QUrl() << 1;
    QTest::newRow("samescheme") << QUrl("x:") << QUrl("x:") << 0;
    QTest::newRow("no-fragment-empty-fragment") << QUrl("http://kde.org/dir/") << QUrl("http://kde.org/dir/#") << -1;
    QTest::newRow("no-query-empty-query") << QUrl("http://kde.org/dir/") << QUrl("http://kde.org/dir/?") << -1;
    QTest::newRow("simple-file-url") << QUrl("file:///home/dfaure/file") << QUrl("file:///home/dfaure/file") << 0;
    QTest::newRow("fromLocalFile-vs-ctor") << QUrl::fromLocalFile("/home/dfaure/file") << QUrl("file:///home/dfaure/file") << 0;

    // the following three are by choice
    // the order could be the opposite and it would still be correct
    QTest::newRow("scheme-path") << QUrl("x:") << QUrl("/tmp") << +1;
    QTest::newRow("fragment-path") << QUrl("#foo") << QUrl("/tmp") << -1;
    QTest::newRow("fragment-scheme") << QUrl("#foo") << QUrl("x:") << -1;

    QTest::newRow("noport-zeroport") << QUrl("http://example.com") << QUrl("http://example.com:0") << -1;
}

void tst_QUrl::comparison2()
{
    QFETCH(QUrl, url1);
    QFETCH(QUrl, url2);
    QFETCH(int, ordering);

    QCOMPARE(url1.toString() == url2.toString(), ordering == 0);
    QCOMPARE(url1 == url2, ordering == 0);
    QCOMPARE(url1 != url2, ordering != 0);
    if (ordering == 0)
        QCOMPARE(qHash(url1), qHash(url2));

    QCOMPARE(url1 < url2, ordering < 0);
    QCOMPARE(!(url1 < url2), ordering >= 0);

    QCOMPARE(url2 < url1, ordering > 0);
    QCOMPARE(!(url2 < url1), ordering <= 0);

    // redundant checks (the above should catch these)
    QCOMPARE(url1 < url2 || url2 < url1, ordering != 0);
    QVERIFY(!(url1 < url2 && url2 < url1));
    QVERIFY(url1 < url2 || url1 == url2 || url2 < url1);
}

void tst_QUrl::copying()
{
    QUrl url("http://qt-project.org/");
    QVERIFY(url.isValid());

    QUrl copy(url);

    QVERIFY(url == copy);
}

void tst_QUrl::setUrl()
{
    {
        QUrl url("http://0.foo.com");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString::fromLatin1("http"));
        QCOMPARE(url.path(), QString());
        QCOMPARE(url.host(), QString::fromLatin1("0.foo.com"));
    }

    {
        QUrl url("file:/");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString::fromLatin1("file"));
        QCOMPARE(url.path(), QString::fromLatin1("/"));
        QVERIFY(url.encodedQuery().isEmpty());
        QVERIFY(url.userInfo().isEmpty());
        QVERIFY(url.authority().isEmpty());
        QVERIFY(url.fragment().isEmpty());
        QCOMPARE(url.port(), -1);
        QCOMPARE(url.toString(), QString::fromLatin1("file:///"));
        QCOMPARE(url.toDisplayString(), QString::fromLatin1("file:///"));
        QCOMPARE(url.toDisplayString(QUrl::PreferLocalFile), QString::fromLatin1("/"));
    }

    {
        QUrl url("hTTp://www.foo.bar:80");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString::fromLatin1("http"));
        QCOMPARE(url.path(), QString());
        QVERIFY(url.encodedQuery().isEmpty());
        QVERIFY(url.userInfo().isEmpty());
        QVERIFY(url.fragment().isEmpty());
        QCOMPARE(url.host(), QString::fromLatin1("www.foo.bar"));
        QCOMPARE(url.authority(), QString::fromLatin1("www.foo.bar:80"));
        QCOMPARE(url.port(), 80);
        QCOMPARE(url.toString(), QString::fromLatin1("http://www.foo.bar:80"));
        QCOMPARE(url.toDisplayString(), QString::fromLatin1("http://www.foo.bar:80"));
        QCOMPARE(url.toDisplayString(QUrl::PreferLocalFile), QString::fromLatin1("http://www.foo.bar:80"));

        QUrl url2("//www1.foo.bar");
        QCOMPARE(url.resolved(url2).toString(), QString::fromLatin1("http://www1.foo.bar"));
    }

    {
        QUrl url("http://user:pass@[56::56:56:56:127.0.0.1]:99");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString::fromLatin1("http"));
        QCOMPARE(url.path(), QString());
        QVERIFY(url.encodedQuery().isEmpty());
        QCOMPARE(url.userInfo(), QString::fromLatin1("user:pass"));
        QVERIFY(url.fragment().isEmpty());
        QCOMPARE(url.host(), QString::fromLatin1("56::56:56:56:7f00:1"));
        QCOMPARE(url.authority(), QString::fromLatin1("user:pass@[56::56:56:56:7f00:1]:99"));
        QCOMPARE(url.port(), 99);
        QCOMPARE(url.url(), QString::fromLatin1("http://user:pass@[56::56:56:56:7f00:1]:99"));
        QCOMPARE(url.toDisplayString(), QString::fromLatin1("http://user@[56::56:56:56:7f00:1]:99"));
    }

    {
        QUrl url("http://user@localhost:8000/xmlhttprequest/resources/basic-auth-nouserpass/basic-auth-nouserpass.php");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString::fromLatin1("http"));
        QCOMPARE(url.userName(), QString::fromLatin1("user"));
        QCOMPARE(url.password(), QString());
        QCOMPARE(url.userInfo(), QString::fromLatin1("user"));
        QCOMPARE(url.port(), 8000);
    }

    {
        QUrl url("http://www.foo.bar");
        QVERIFY(url.isValid());

        QUrl url2("/top//test/../test1/file.html");
        QCOMPARE(url.resolved(url2).toString(), QString::fromLatin1("http://www.foo.bar/top//test1/file.html"));
    }

    {
        QUrl url("http://www.foo.bar");
        QVERIFY(url.isValid());

        QUrl url2("/top//test/../test1/file.html");
        QCOMPARE(url.resolved(url2).toString(), QString::fromLatin1("http://www.foo.bar/top//test1/file.html"));
    }

    {
        QUrl url("http://www.foo.bar/top//test2/file2.html");
        QVERIFY(url.isValid());

        QCOMPARE(url.toString(), QString::fromLatin1("http://www.foo.bar/top//test2/file2.html"));
    }

    {
        QUrl url("http://www.foo.bar/top//test2/file2.html");
        QVERIFY(url.isValid());

        QCOMPARE(url.toString(), QString::fromLatin1("http://www.foo.bar/top//test2/file2.html"));
    }

    {
        QUrl url("file:/usr/local/src/kde2/////kdelibs/kio");
        QVERIFY(url.isValid());
        QCOMPARE(url.toString(), QString::fromLatin1("file:///usr/local/src/kde2/////kdelibs/kio"));
    }

    {
        QUrl url("http://www.foo.bar");
        QVERIFY(url.isValid());

        QUrl url2("mailto:bastian@kde.org");
        QVERIFY(url2.isValid());
        QCOMPARE(url.resolved(url2).toString(), QString::fromLatin1("mailto:bastian@kde.org"));
    }

    {
        QUrl url("mailto:bastian@kde.org?subject=hello");
        QCOMPARE(url.toString(), QString::fromLatin1("mailto:bastian@kde.org?subject=hello"));
    }

    {
        QUrl url("file:/usr/local/src/kde2/kdelibs/kio/");
        QVERIFY(url.isValid());

        QUrl url2("../../////kdebase/konqueror");
        QCOMPARE(url.resolved(url2).toString(),
                QString::fromLatin1("file:///usr/local/src/kde2/////kdebase/konqueror"));
    }

    {
        QString u1 = "file:/home/dfaure/my#myref";
        QUrl url = u1;
        QVERIFY(url.isValid());
        QCOMPARE(url.toString(), QString::fromLatin1("file:///home/dfaure/my#myref"));
        QCOMPARE(url.toString(QUrl::PreferLocalFile), QString::fromLatin1("file:///home/dfaure/my#myref"));
        QCOMPARE(url.fragment(), QString::fromLatin1("myref"));
    }

    {
        QUrl url("gg:www.kde.org");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString::fromLatin1("gg"));
        QVERIFY(url.host().isEmpty());
        QCOMPARE(url.path(), QString::fromLatin1("www.kde.org"));
    }

    {
        QUrl url("KDE");
        QVERIFY(url.isValid());
        QCOMPARE(url.path(), QString::fromLatin1("KDE"));
        QVERIFY(url.scheme().isEmpty());
    }

    {
        QUrl url("$HOME/.kde/share/config");
        QVERIFY(url.isValid());
        QCOMPARE(url.path(), QString::fromLatin1("$HOME/.kde/share/config"));
        QVERIFY(url.scheme().isEmpty());
    }

    {
        QUrl url("file:/opt/kde2/qt2/doc/html/showimg-main-cpp.html#QObject::connect");
        QVERIFY(url.isValid());
        QCOMPARE(url.fragment(), QString::fromLatin1("QObject::connect"));
    }

    {
        QUrl url("file:/opt/kde2/qt2/doc/html/showimg-main-cpp.html#QObject:connect");
        QVERIFY(url.isValid());
        QCOMPARE(url.fragment(), QString::fromLatin1("QObject:connect"));
    }

    {
        // suburls
        QUrl url("file:/home/dfaure/my%20tar%20file.tgz#gzip:/#tar:/#myref");
        QVERIFY(url.isValid());

        // or simply 'myref?'
        QCOMPARE(url.fragment(), QString::fromLatin1("gzip:/#tar:/#myref"));
    }

    {
        QUrl url("error:/?error=14&errText=Unknown%20host%20asdfu.adgi.sdfgoi#http://asdfu.adgi.sdfgoi");
        QVERIFY(url.isValid());
        QCOMPARE(url.fragment(), QString::fromLatin1("http://asdfu.adgi.sdfgoi"));
    }

    {
        // suburls
        QUrl url("file:/home/dfaure/my%20tar%20file.tgz#gzip:/#tar:/");
        QVERIFY(url.isValid());
    }

    {
        QUrl url("file:/home/dfaure/cdrdao-1.1.5/dao/#CdrDriver.cc#");
        QVERIFY(url.isValid());
    }

    {
        QUrl url("file:/home/dfaure/my%20tar%20file.tgz#gzip:/#tar:/README");
        QVERIFY(url.isValid());
        QCOMPARE(url.toString(), QString::fromLatin1("file:///home/dfaure/my tar file.tgz#gzip:/#tar:/README"));
    }

    {
        QUrl notPretty("http://ferret.lmh.ox.ac.uk/%7Ekdecvs/");
        QVERIFY(notPretty.isValid());
        QCOMPARE(notPretty.toString(), QString::fromLatin1("http://ferret.lmh.ox.ac.uk/~kdecvs/"));

        QUrl notPretty2("file:/home/test/directory%20with%20spaces");
        QVERIFY(notPretty2.isValid());
        QCOMPARE(notPretty2.toString(), QString::fromLatin1("file:///home/test/directory with spaces"));

        QUrl notPretty3("fish://foo/%23README%23");
        QVERIFY(notPretty3.isValid());
        QCOMPARE(notPretty3.toString(), QString::fromLatin1("fish://foo/%23README%23"));

        QUrl url15581;
        url15581.setUrl("http://alain.knaff.linux.lu/bug-reports/kde/spaces in url.html");
        QCOMPARE(url15581.toString(), QString::fromLatin1("http://alain.knaff.linux.lu/bug-reports/kde/spaces in url.html"));
        QCOMPARE(url15581.toEncoded().constData(), QByteArray("http://alain.knaff.linux.lu/bug-reports/kde/spaces%20in%20url.html").constData());

        QUrl url15582("http://alain.knaff.linux.lu/bug-reports/kde/percentage%in%url.html");
        QCOMPARE(url15582.toString(), QString::fromLatin1("http://alain.knaff.linux.lu/bug-reports/kde/percentage%25in%25url.html"));
        QCOMPARE(url15582.toEncoded(), QByteArray("http://alain.knaff.linux.lu/bug-reports/kde/percentage%25in%25url.html"));
        QCOMPARE(url15582.toString(QUrl::FullyEncoded), QString("http://alain.knaff.linux.lu/bug-reports/kde/percentage%25in%25url.html"));
    }

    {
        QUrl carsten;
        carsten.setPath("/home/gis/src/kde/kdelibs/kfile/.#kfiledetailview.cpp.1.18");
        QCOMPARE(carsten.path(), QString::fromLatin1("/home/gis/src/kde/kdelibs/kfile/.#kfiledetailview.cpp.1.18"));

        QUrl charles;
        charles.setPath("/home/charles/foo%20moo");
        QCOMPARE(charles.path(), QString::fromLatin1("/home/charles/foo moo"));
        QCOMPARE(charles.path(QUrl::FullyEncoded), QString::fromLatin1("/home/charles/foo%20moo"));

        QUrl charles2("file:/home/charles/foo%20moo");
        QCOMPARE(charles2.path(), QString::fromLatin1("/home/charles/foo moo"));
        QCOMPARE(charles2.path(QUrl::FullyEncoded), QString::fromLatin1("/home/charles/foo%20moo"));
    }

    {
        QUrl udir;
        QCOMPARE(udir.toEncoded(), QByteArray());
        QCOMPARE(udir.toString(QUrl::FullyEncoded), QString());
        QVERIFY(!udir.isValid());

        udir = QUrl::fromLocalFile("/home/dfaure/file.txt");
        QCOMPARE(udir.path(), QString::fromLatin1("/home/dfaure/file.txt"));
        QCOMPARE(udir.toEncoded(), QByteArray("file:///home/dfaure/file.txt"));
        QCOMPARE(udir.toString(QUrl::FullyEncoded), QString("file:///home/dfaure/file.txt"));
    }

    {
        QUrl url;
        url.setUrl("hello.com#?");
        QVERIFY(url.isValid());
        url.setUrl("hello.com");
        QVERIFY(!url.toString().contains(QLatin1Char('#')));
        QVERIFY(!url.toString().contains(QLatin1Char('?')));
    }

    {
        QUrl url;
        url.setUrl("http://1.2.3.4.example.com");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("http"));
        QCOMPARE(url.host(), QString("1.2.3.4.example.com"));
    }

    {
        QUrl url;
        url.setUrl("http://1.2.3.4");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("http"));
        QCOMPARE(url.host(), QString("1.2.3.4"));
    }
    {
        QUrl url;
        url.setUrl("http://1.2.3.4/");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("http"));
        QCOMPARE(url.host(), QString("1.2.3.4"));
        QCOMPARE(url.path(), QString("/"));
    }
    {
        QUrl url;
        url.setUrl("http://1.2.3.4?foo");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("http"));
        QCOMPARE(url.host(), QString("1.2.3.4"));
        QCOMPARE(url.encodedQuery(), QByteArray("foo"));
    }
    {
        QUrl url;
        url.setUrl("http://1.2.3.4#bar");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("http"));
        QCOMPARE(url.host(), QString("1.2.3.4"));
        QCOMPARE(url.fragment(), QString("bar"));
    }

    {
        QUrl url("data:text/javascript,d5%20%3D%20'five\\u0027s'%3B");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("data"));
        QCOMPARE(url.host(), QString());
        QCOMPARE(url.path(), QString("text/javascript,d5 = 'five\\u0027s';"));
        QCOMPARE(url.encodedPath().constData(), "text/javascript,d5%20=%20'five%5Cu0027s';");
    }

    {
        // invalid port number
        QUrl url;
        url.setEncodedUrl("foo://tel:2147483648");
        QVERIFY(!url.isValid());
    }

    { //check it calls detach
        QUrl u1("http://aaa.com");
        QUrl u2 = u1;
        u2.setUrl("http://bbb.com");
        QCOMPARE(u1.host(), QString::fromLatin1("aaa.com"));
        QCOMPARE(u2.host(), QString::fromLatin1("bbb.com"));
    }
}

void tst_QUrl::i18n_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QByteArray>("punyOutput");

    QTest::newRow("øl") << QString::fromUtf8("http://ole:passord@www.øl.no/index.html?ole=æsemann&ilder gud=hei#top")
                        << QByteArray("http://ole:passord@www.xn--l-4ga.no/index.html?ole=%C3%A6semann&ilder%20gud=hei#top");
    QTest::newRow("räksmörgås") << QString::fromUtf8("http://www.räksmörgås.no/")
                             << QByteArray("http://www.xn--rksmrgs-5wao1o.no/");
    QTest::newRow("bühler") << QString::fromUtf8("http://www.bühler.no/")
                         << QByteArray("http://www.xn--bhler-kva.no/");
    QTest::newRow("non-latin1")
        << QString::fromUtf8("http://www.\316\261\316\270\316\256\316\275\316\261.info")
        << QByteArray("http://www.xn--jxafb0a0a.info");
}

void tst_QUrl::i18n()
{
    QFETCH(QString, input);
    QFETCH(QByteArray, punyOutput);

    QUrl url(input);
    QVERIFY(url.isValid());

    QCOMPARE(url.toEncoded().constData(), punyOutput.constData());
    QCOMPARE(QUrl::fromEncoded(punyOutput), url);
    QCOMPARE(QUrl::fromEncoded(punyOutput).toString(), input);
}


void tst_QUrl::resolving_data()
{
    QTest::addColumn<QString>("baseUrl");
    QTest::addColumn<QString>("relativeUrl");
    QTest::addColumn<QString>("resolvedUrl");

    // 5.4.1 Normal Examples (http://www.ietf.org/rfc/rfc3986.txt)
    QTest::newRow("g:h")       << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g:h")      << QString::fromLatin1("g:h");
    QTest::newRow("g")         << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g")        << QString::fromLatin1("http://a/b/c/g");
    QTest::newRow("./g")       << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("./g")      << QString::fromLatin1("http://a/b/c/g");
    QTest::newRow("g/")        << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g/")       << QString::fromLatin1("http://a/b/c/g/");
    QTest::newRow("/g")        << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("/g")       << QString::fromLatin1("http://a/g");
    QTest::newRow("//g")       << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("//g")      << QString::fromLatin1("http://g");
    QTest::newRow("?y")        << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("?y")       << QString::fromLatin1("http://a/b/c/d;p?y");
    QTest::newRow("g?y")       << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g?y")      << QString::fromLatin1("http://a/b/c/g?y");
    QTest::newRow("#s")        << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("#s")       << QString::fromLatin1("http://a/b/c/d;p?q#s");
    QTest::newRow("g#s")       << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g#s")      << QString::fromLatin1("http://a/b/c/g#s");
    QTest::newRow("g?y#s")     << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g?y#s")    << QString::fromLatin1("http://a/b/c/g?y#s");
    QTest::newRow(";x")        << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1(";x")       << QString::fromLatin1("http://a/b/c/;x");
    QTest::newRow("g;x")       << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g;x")      << QString::fromLatin1("http://a/b/c/g;x");
    QTest::newRow("g;x?y#s")   << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g;x?y#s")  << QString::fromLatin1("http://a/b/c/g;x?y#s");
    QTest::newRow("[empty]")   << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("")         << QString::fromLatin1("http://a/b/c/d;p?q");
    QTest::newRow(".")         << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1(".")        << QString::fromLatin1("http://a/b/c/");
    QTest::newRow("./")        << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("./")       << QString::fromLatin1("http://a/b/c/");
    QTest::newRow("..")        << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("..")       << QString::fromLatin1("http://a/b/");
    QTest::newRow("../")       << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("../")      << QString::fromLatin1("http://a/b/");
    QTest::newRow("../g")      << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("../g")     << QString::fromLatin1("http://a/b/g");
    QTest::newRow("../..")     << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("../..")    << QString::fromLatin1("http://a/");
    QTest::newRow("../../")    << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("../../")   << QString::fromLatin1("http://a/");
    QTest::newRow("../../g")   << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("../../g")  << QString::fromLatin1("http://a/g");

    // 5.4.2  Abnormal Examples (http://www.ietf.org/rfc/rfc3986.txt)

    // Parsers must be careful in handling cases where there are more
    // relative path ".." segments than there are hierarchical levels in the
    // base URI's path.  Note that the ".." syntax cannot be used to change
    // the authority component of a URI.
    QTest::newRow("../../../g")    << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("../../../g")     << QString::fromLatin1("http://a/g");
    QTest::newRow("../../../../g") << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("../../../../g")  << QString::fromLatin1("http://a/g");

    // Similarly, parsers must remove the dot-segments "." and ".." when
    // they are complete components of a path, but not when they are only
    // part of a segment.
    QTest::newRow("/./g")  << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("/./g")  << QString::fromLatin1("http://a/g");
    QTest::newRow("/../g") << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("/../g") << QString::fromLatin1("http://a/g");
    QTest::newRow("g.")    << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g.")    << QString::fromLatin1("http://a/b/c/g.");
    QTest::newRow(".g")    << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1(".g")    << QString::fromLatin1("http://a/b/c/.g");
    QTest::newRow("g..")   << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g..")   << QString::fromLatin1("http://a/b/c/g..");
    QTest::newRow("..g")   << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("..g")   << QString::fromLatin1("http://a/b/c/..g");

    // Less likely are cases where the relative URI reference uses
    // unnecessary or nonsensical forms of the "." and ".." complete path
    // segments.
    QTest::newRow("./../g")     << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("./../g")     << QString::fromLatin1("http://a/b/g");
    QTest::newRow("./g/.")      << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("./g/.")      << QString::fromLatin1("http://a/b/c/g/");
    QTest::newRow("g/./h")      << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g/./h")      << QString::fromLatin1("http://a/b/c/g/h");
    QTest::newRow("g/../h")     << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g/../h")     << QString::fromLatin1("http://a/b/c/h");
    QTest::newRow("g;x=1/./y")  << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g;x=1/./y")  << QString::fromLatin1("http://a/b/c/g;x=1/y");
    QTest::newRow("g;x=1/../y") << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g;x=1/../y") << QString::fromLatin1("http://a/b/c/y");

    // Some applications fail to separate the reference's query and/or
    // fragment components from a relative path before merging it with the
    // base path and removing dot-segments.  This error is rarely noticed,
    // since typical usage of a fragment never includes the hierarchy ("/")
    // character, and the query component is not normally used within
    // relative references.
    QTest::newRow("g?y/./x")  << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g?y/./x")  << QString::fromLatin1("http://a/b/c/g?y/./x");
    QTest::newRow("g?y/../x") << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g?y/../x") << QString::fromLatin1("http://a/b/c/g?y/../x");
    QTest::newRow("g#s/./x")  << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g#s/./x")  << QString::fromLatin1("http://a/b/c/g#s/./x");
    QTest::newRow("g#s/../x") << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("g#s/../x") << QString::fromLatin1("http://a/b/c/g#s/../x");

    // Some parsers allow the scheme name to be present in a relative URI
    // reference if it is the same as the base URI scheme.  This is
    // considered to be a loophole in prior specifications of partial URI
    // [RFC1630]. Its use should be avoided, but is allowed for backward
    // compatibility.
    // For strict parsers :
//    QTest::newRow("http:g [for strict parsers]")         << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("http:g") << QString::fromLatin1("http:g");
    // For backward compatibility :
    QTest::newRow("http:g [for backward compatibility]") << QString::fromLatin1("http://a/b/c/d;p?q") << QString::fromLatin1("http:g") << QString::fromLatin1("http://a/b/c/g");

    // Resolve relative with relative
    QTest::newRow("../a (1)")  << QString::fromLatin1("b") << QString::fromLatin1("../a")  << QString::fromLatin1("a");
    QTest::newRow("../a (2)")  << QString::fromLatin1("b/a") << QString::fromLatin1("../a")  << QString::fromLatin1("a");
    QTest::newRow("../a (3)")  << QString::fromLatin1("b/c/a") << QString::fromLatin1("../a")  << QString::fromLatin1("b/a");
    QTest::newRow("../a (4)")  << QString::fromLatin1("b") << QString::fromLatin1("/a")  << QString::fromLatin1("/a");

    QTest::newRow("../a (5)")  << QString::fromLatin1("/b") << QString::fromLatin1("../a")  << QString::fromLatin1("/a");
    QTest::newRow("../a (6)")  << QString::fromLatin1("/b/a") << QString::fromLatin1("../a")  << QString::fromLatin1("/a");
    QTest::newRow("../a (7)")  << QString::fromLatin1("/b/c/a") << QString::fromLatin1("../a")  << QString::fromLatin1("/b/a");
    QTest::newRow("../a (8)")  << QString::fromLatin1("/b") << QString::fromLatin1("/a")  << QString::fromLatin1("/a");
}

void tst_QUrl::resolving()
{
    QFETCH(QString, baseUrl);
    QFETCH(QString, relativeUrl);
    QFETCH(QString, resolvedUrl);

    QUrl url(baseUrl);
    QCOMPARE(url.resolved(relativeUrl).toString(), resolvedUrl);
}

void tst_QUrl::toString_data()
{
    QTest::addColumn<QString>("urlString");
    QTest::addColumn<uint>("options");
    QTest::addColumn<QString>("string");

    QTest::newRow("data0") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemoveScheme)
                        << QString::fromLatin1("//ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data2") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemovePassword)
                        << QString::fromLatin1("http://ole@www.troll.no:9090/index.html?ole=semann&gud=hei#top");

    // show that QUrl keeps the empty-but-present username if you remove the password
    // see data3-bis for another case
    QTest::newRow("data2-bis") << QString::fromLatin1("http://:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemovePassword)
                        << QString::fromLatin1("http://@www.troll.no:9090/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data3") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemoveUserInfo)
                        << QString::fromLatin1("http://www.troll.no:9090/index.html?ole=semann&gud=hei#top");

    // show that QUrl keeps the empty-but-preset hostname if you remove the userinfo
    QTest::newRow("data3-bis") << QString::fromLatin1("http://ole:password@/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemoveUserInfo)
                        << QString::fromLatin1("http:///index.html?ole=semann&gud=hei#top");

    QTest::newRow("data4") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemovePort)
                        << QString::fromLatin1("http://ole:password@www.troll.no/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data5") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemoveAuthority)
                        << QString::fromLatin1("http:/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data6") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemovePath)
                        << QString::fromLatin1("http://ole:password@www.troll.no:9090?ole=semann&gud=hei#top");

    QTest::newRow("data7") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemoveQuery)
                        << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html#top");

    QTest::newRow("data8") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemoveFragment)
                        << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei");

    QTest::newRow("data9") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                        << uint(QUrl::RemoveScheme | QUrl::RemovePassword)
                        << QString::fromLatin1("//ole@www.troll.no:9090/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data10") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                         << uint(QUrl::RemoveScheme | QUrl::RemoveUserInfo)
                         << QString::fromLatin1("//www.troll.no:9090/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data11") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                         << uint(QUrl::RemoveScheme | QUrl::RemovePort)
                         << QString::fromLatin1("//ole:password@www.troll.no/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data12") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                         << uint(QUrl::RemoveScheme | QUrl::RemoveAuthority)
                         << QString::fromLatin1("/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data13") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                         << uint(QUrl::RemoveScheme | QUrl::RemovePath)
                         << QString::fromLatin1("//ole:password@www.troll.no:9090?ole=semann&gud=hei#top");

    QTest::newRow("data14") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                         << uint(QUrl::RemoveScheme | QUrl::RemoveAuthority | QUrl::RemoveFragment)
                         << QString::fromLatin1("/index.html?ole=semann&gud=hei");

    QTest::newRow("data15") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                         << uint(QUrl::RemoveAuthority | QUrl::RemoveQuery)
                         << QString::fromLatin1("http:/index.html#top");

    QTest::newRow("data16") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                         << uint(QUrl::RemovePassword | QUrl::RemovePort
                            | QUrl::RemovePath | QUrl::RemoveQuery
                            | QUrl::RemoveFragment)
                         << QString::fromLatin1("http://ole@www.troll.no");

    QTest::newRow("data17") << QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
                         << uint(QUrl::RemoveScheme | QUrl::RemovePassword
                            | QUrl::RemovePort | QUrl::RemovePath
                            | QUrl::RemoveQuery | QUrl::RemoveFragment)
                         << QString::fromLatin1("//ole@www.troll.no");

    QTest::newRow("data18") << QString::fromLatin1("http://andreas:hemmelig@www.vg.no/?my=query&your=query#yougotfragged")
                         << uint(QUrl::None)
                         << QString::fromLatin1("http://andreas:hemmelig@www.vg.no/?my=query&your=query#yougotfragged");

    QTest::newRow("nopath") << QString::fromLatin1("host://protocol")
                         << uint(QUrl::None)
                         << QString::fromLatin1("host://protocol");

    QTest::newRow("underscore") << QString::fromLatin1("http://foo_bar.host.com/rss.php")
                         << uint(QUrl::None)
                         << QString::fromLatin1("http://foo_bar.host.com/rss.php");
}

void tst_QUrl::toString()
{
    QFETCH(QString, urlString);
    QFETCH(uint, options);
    QFETCH(QString, string);

    QUrl url(urlString);
    QCOMPARE(url.toString(QUrl::FormattingOptions(options)), string);
}

void tst_QUrl::toAndFromStringList_data()
{
    QTest::addColumn<QStringList>("strings");

    QTest::newRow("empty") << QStringList();
    QTest::newRow("local") << (QStringList() << "file:///tmp" << "file:///");
    QTest::newRow("remote") << (QStringList() << "http://qt-project.org");
}

void tst_QUrl::toAndFromStringList()
{
    QFETCH(QStringList, strings);

    const QList<QUrl> urls = QUrl::fromStringList(strings);
    QCOMPARE(urls.count(), strings.count());
    const QStringList converted = QUrl::toStringList(urls);
    QCOMPARE(converted, strings);
}

//### more tests ... what do we expect ...
void tst_QUrl::isParentOf_data()
{
    QTest::addColumn<QString>("parent");
    QTest::addColumn<QString>("child");
    QTest::addColumn<bool>("trueFalse");

    QTest::newRow("data0") << QString::fromLatin1("http://a.b.c/d")
                        << QString::fromLatin1("http://a.b.c/d/e?f") << true;
    QTest::newRow("data1") << QString::fromLatin1("http://a.b.c/d")
                        << QString::fromLatin1("http://a.b.c/d") << false;
    QTest::newRow("data2") << QString::fromLatin1("http://a.b.c/d")
                        << QString::fromLatin1("http://a.b.c/de") << false;
    QTest::newRow("data3") << QString::fromLatin1("http://a.b.c/d/")
                        << QString::fromLatin1("http://a.b.c/de") << false;
    QTest::newRow("data4") << QString::fromLatin1("http://a.b.c/d/")
                        << QString::fromLatin1("http://a.b.c/d/e") << true;
}

void tst_QUrl::toString_constructed_data()
{
    QTest::addColumn<QString>("scheme");
    QTest::addColumn<QString>("userName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QByteArray>("query");
    QTest::addColumn<QString>("fragment");
    QTest::addColumn<QString>("asString");
    QTest::addColumn<QByteArray>("asEncoded");
    QTest::addColumn<uint>("options");

    QString n("");

    QTest::newRow("data1") << n << n << n << QString::fromLatin1("qt-project.org") << -1 << QString::fromLatin1("/index.html")
                        << QByteArray() << n << QString::fromLatin1("//qt-project.org/index.html")
                        << QByteArray("//qt-project.org/index.html") << 0u;
    QTest::newRow("data2") << QString::fromLatin1("file") << n << n << n << -1 << QString::fromLatin1("/root") << QByteArray()
                        << n << QString::fromLatin1("file:///root") << QByteArray("file:///root") << 0u;
    QTest::newRow("userAndPass") << QString::fromLatin1("http") << QString::fromLatin1("dfaure") << QString::fromLatin1("kde")
                        << "kde.org" << 443 << QString::fromLatin1("/") << QByteArray() << n
                        << QString::fromLatin1("http://dfaure:kde@kde.org:443/") << QByteArray("http://dfaure:kde@kde.org:443/")
                        << 0u;
    QTest::newRow("PassWithoutUser") << QString::fromLatin1("http") << n << QString::fromLatin1("kde")
                        << "kde.org" << 443 << QString::fromLatin1("/") << QByteArray() << n
                        << QString::fromLatin1("http://:kde@kde.org:443/") << QByteArray("http://:kde@kde.org:443/") << 0u;
    QTest::newRow("PassWithoutUser-RemovePassword") << QString::fromLatin1("http") << n << QString::fromLatin1("kde")
                        << "kde.org" << 443 << QString::fromLatin1("/") << QByteArray() << n
                        << QString::fromLatin1("http://kde.org:443/") << QByteArray("http://kde.org:443/")
                        << uint(QUrl::RemovePassword);
}

void tst_QUrl::toString_constructed()
{
    QFETCH(QString, scheme);
    QFETCH(QString, userName);
    QFETCH(QString, password);
    QFETCH(QString, host);
    QFETCH(int, port);
    QFETCH(QString, path);
    QFETCH(QByteArray, query);
    QFETCH(QString, fragment);
    QFETCH(QString, asString);
    QFETCH(QByteArray, asEncoded);
    QFETCH(uint, options);

    QUrl url;
    if (!scheme.isEmpty())
        url.setScheme(scheme);
    if (!userName.isEmpty())
        url.setUserName(userName);
    if (!password.isEmpty())
        url.setPassword(password);
    if (!host.isEmpty())
        url.setHost(host);
    if (port != -1)
        url.setPort(port);
    if (!path.isEmpty())
        url.setPath(path);
    if (!query.isEmpty())
        url.setEncodedQuery(query);
    if (!fragment.isEmpty())
        url.setFragment(fragment);

    QVERIFY(url.isValid());

    QUrl::FormattingOptions formattingOptions(options);
    QCOMPARE(url.toString(formattingOptions), asString);
    QCOMPARE(QString::fromLatin1(url.toEncoded(formattingOptions)), QString::fromLatin1(asEncoded)); // readable in case of differences
    QCOMPARE(url.toEncoded(formattingOptions), asEncoded);
}


void tst_QUrl::isParentOf()
{
    QFETCH(QString, parent);
    QFETCH(QString, child);
    QFETCH(bool, trueFalse);

    QUrl url(parent);
    QCOMPARE(url.isParentOf(QUrl(child)), trueFalse);
}

void tst_QUrl::toLocalFile_data()
{
    QTest::addColumn<QString>("theUrl");
    QTest::addColumn<QString>("theFile");

    QTest::newRow("data0") << QString::fromLatin1("file:/a.txt") << QString::fromLatin1("/a.txt");
    QTest::newRow("data4") << QString::fromLatin1("file:///a.txt") << QString::fromLatin1("/a.txt");
#ifdef Q_OS_WIN
    QTest::newRow("data5") << QString::fromLatin1("file:///c:/a.txt") << QString::fromLatin1("c:/a.txt");
#else
    QTest::newRow("data5") << QString::fromLatin1("file:///c:/a.txt") << QString::fromLatin1("/c:/a.txt");
#endif
    QTest::newRow("data6") << QString::fromLatin1("file://somehost/somedir/somefile") << QString::fromLatin1("//somehost/somedir/somefile");
    QTest::newRow("data7") << QString::fromLatin1("file://somehost/") << QString::fromLatin1("//somehost/");
    QTest::newRow("data8") << QString::fromLatin1("file://somehost") << QString::fromLatin1("//somehost");
    QTest::newRow("data9") << QString::fromLatin1("file:////somehost/somedir/somefile") << QString::fromLatin1("//somehost/somedir/somefile");
    QTest::newRow("data10") << QString::fromLatin1("FILE:/a.txt") << QString::fromLatin1("/a.txt");
    QTest::newRow("data11") << QString::fromLatin1("file:///Mambo <%235>.mp3") << QString::fromLatin1("/Mambo <#5>.mp3");
    QTest::newRow("data12") << QString::fromLatin1("file:///a%25.txt") << QString::fromLatin1("/a%.txt");
    QTest::newRow("data13") << QString::fromLatin1("file:///a%25%25.txt") << QString::fromLatin1("/a%%.txt");
    QTest::newRow("data14") << QString::fromLatin1("file:///a%25a%25.txt") << QString::fromLatin1("/a%a%.txt");
    QTest::newRow("data15") << QString::fromLatin1("file:///a%1f.txt") << QString::fromLatin1("/a\x1f.txt");
    QTest::newRow("data16") << QString::fromLatin1("file:///%2580.txt") << QString::fromLatin1("/%80.txt");

    // and some that result in empty (i.e., not local)
    QTest::newRow("xdata0") << QString::fromLatin1("/a.txt") << QString();
    QTest::newRow("xdata1") << QString::fromLatin1("//a.txt") << QString();
    QTest::newRow("xdata2") << QString::fromLatin1("///a.txt") << QString();
    QTest::newRow("xdata3") << QString::fromLatin1("foo:/a.txt") << QString();
    QTest::newRow("xdata4") << QString::fromLatin1("foo://a.txt") << QString();
    QTest::newRow("xdata5") << QString::fromLatin1("foo:///a.txt") << QString();
}

void tst_QUrl::toLocalFile()
{
    QFETCH(QString, theUrl);
    QFETCH(QString, theFile);

    QUrl url(theUrl);
    QCOMPARE(url.toLocalFile(), theFile);
    QCOMPARE(url.isLocalFile(), !theFile.isEmpty());
}

void tst_QUrl::fromLocalFile_data()
{
    QTest::addColumn<QString>("theFile");
    QTest::addColumn<QString>("theUrl");
    QTest::addColumn<QString>("thePath");

    QTest::newRow("data0") << QString::fromLatin1("/a.txt") << QString::fromLatin1("file:///a.txt") << QString::fromLatin1("/a.txt");
    QTest::newRow("data1") << QString::fromLatin1("a.txt") << QString::fromLatin1("file:a.txt") << QString::fromLatin1("a.txt");
    QTest::newRow("data2") << QString::fromLatin1("/a/b.txt") << QString::fromLatin1("file:///a/b.txt") << QString::fromLatin1("/a/b.txt");
    QTest::newRow("data3") << QString::fromLatin1("c:/a.txt") << QString::fromLatin1("file:///c:/a.txt") << QString::fromLatin1("/c:/a.txt");
    QTest::newRow("data4") << QString::fromLatin1("//somehost/somedir/somefile") << QString::fromLatin1("file://somehost/somedir/somefile")
                        << QString::fromLatin1("/somedir/somefile");
    QTest::newRow("data5") << QString::fromLatin1("//somehost") << QString::fromLatin1("file://somehost")
                        << QString::fromLatin1("");
    QTest::newRow("data6") << QString::fromLatin1("//somehost/") << QString::fromLatin1("file://somehost/")
                        << QString::fromLatin1("/");
    QTest::newRow("data7") << QString::fromLatin1("/Mambo <#5>.mp3") << QString::fromLatin1("file:///Mambo <%235>.mp3")
                           << QString::fromLatin1("/Mambo <#5>.mp3");
    QTest::newRow("data8") << QString::fromLatin1("/a%.txt") << QString::fromLatin1("file:///a%25.txt")
                           << QString::fromLatin1("/a%25.txt");
    QTest::newRow("data9") << QString::fromLatin1("/a%25.txt") << QString::fromLatin1("file:///a%2525.txt")
                           << QString::fromLatin1("/a%2525.txt");
    QTest::newRow("data10") << QString::fromLatin1("/%80.txt") << QString::fromLatin1("file:///%2580.txt")
                            << QString::fromLatin1("/%2580.txt");
}

void tst_QUrl::fromLocalFile()
{
    QFETCH(QString, theFile);
    QFETCH(QString, theUrl);
    QFETCH(QString, thePath);

    QUrl url = QUrl::fromLocalFile(theFile);

    QCOMPARE(url.toString(QUrl::DecodeReserved), theUrl);
    QCOMPARE(url.path(), thePath);
}

void tst_QUrl::compat_legacy()
{
    {
        QUrl u( "file:bar" );
        QCOMPARE( u.toString(QUrl::RemoveScheme), QString("bar") );
    }

    /* others
     */
    {
        QUrl u( "http://qt-project.org/images/ban/pgs_front.jpg" );
        QCOMPARE( u.path(), QString("/images/ban/pgs_front.jpg") );
    }
    {
        QUrl tmp( "http://qt-project.org/images/ban/" );
        QUrl u = tmp.resolved(QString("pgs_front.jpg"));
        QCOMPARE( u.path(), QString("/images/ban/pgs_front.jpg") );
    }
    {
        QUrl tmp;
        QUrl u = tmp.resolved(QString("http://qt-project.org/images/ban/pgs_front.jpg"));
        QCOMPARE( u.path(), QString("/images/ban/pgs_front.jpg") );
    }
    {
        QUrl tmp;
        QUrl u = tmp.resolved(QString("http://qt-project.org/images/ban/pgs_front.jpg"));
        QFileInfo fi(u.path());
        u.setPath(fi.path());
        QCOMPARE( u.path(), QString("/images/ban") );
    }
}

void tst_QUrl::compat_constructor_01_data()
{
    QTest::addColumn<QString>("urlStr");
    QTest::addColumn<QString>("res");

    //next we fill it with data
    QTest::newRow( "data0" )  << QString("Makefile") << QString("Makefile"); // nolonger add file by default
    QTest::newRow( "data1" )  << QString("Makefile") << QString("Makefile");
    QTest::newRow( "data2" )  << QString("ftp://ftp.qt-project.org/qt/INSTALL") << QString("ftp://ftp.qt-project.org/qt/INSTALL");
    QTest::newRow( "data3" )  << QString("ftp://ftp.qt-project.org/qt/INSTALL") << QString("ftp://ftp.qt-project.org/qt/INSTALL");
}

void tst_QUrl::compat_constructor_01()
{
    /* The following should work as expected:
     *
     * QUrlOperator op;
     * op.copy( QString( "Makefile" ),
     *          QString("ftp://rms:grmpf12@nibbler/home/rms/tmp"),
     *          false );
     *
     * as well as the following:
     *
     * QUrlOperator op;
     * op.copy(QString("ftp://ftp.qt-project.org/qt/INSTALL"), ".");
     */
    QFETCH( QString, urlStr );

    {
        QUrl empty;
        QUrl u = empty.resolved(urlStr);

        QTEST( u.toString(), "res" );
    }
    {
        QUrl empty;
        QUrl u = empty.resolved(urlStr);

        QTEST( u.toString(), "res" );
    }
}

void tst_QUrl::compat_constructor_02_data()
{
    QTest::addColumn<QString>("urlStr");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("res");

    //next we fill it with data
    QTest::newRow( "data0" )  << QString("ftp://ftp.qt-project.org/qt") << QString("INSTALL") << QString("ftp://ftp.qt-project.org/INSTALL");
    QTest::newRow( "data1" )  << QString("ftp://ftp.qt-project.org/qt/") << QString("INSTALL") << QString("ftp://ftp.qt-project.org/qt/INSTALL");
}

void tst_QUrl::compat_constructor_02()
{
    /* The following should work as expected:
     *
     * QUrlOperator op( "ftp://ftp.qt-project.org/qt" );
     * op.copy(QString("INSTALL"), ".");
     */
    QFETCH( QString, urlStr );
    QFETCH( QString, fileName );

    QUrl tmp( urlStr );
    QUrl u = tmp.resolved(fileName);

    QTEST( u.toString(), "res" );
}

void tst_QUrl::compat_constructor_03_data()
{
    QTest::addColumn<QString>("urlStr");
    QTest::addColumn<QString>("res");

    //next we fill it with data
    QTest::newRow( "protocol00" )  << QString( "http://qt-project.org/index.html" ) << QString( "http://qt-project.org/index.html" );
    QTest::newRow( "protocol01" )  << QString( "http://qt-project.org" ) << QString( "http://qt-project.org" );
    QTest::newRow( "protocol02" )  << QString( "http://qt-project.org/" ) << QString( "http://qt-project.org/" );
    QTest::newRow( "protocol03" )  << QString( "http://qt-project.org/foo" ) << QString( "http://qt-project.org/foo" );
    QTest::newRow( "protocol04" )  << QString( "http://qt-project.org/foo/" ) << QString( "http://qt-project.org/foo/" );
    QTest::newRow( "protocol05" )  << QString( "ftp://ftp.qt-project.org/foo/index.txt" ) << QString( "ftp://ftp.qt-project.org/foo/index.txt" );

    QTest::newRow( "local00" )  << QString( "/foo" ) << QString( "/foo" );
    QTest::newRow( "local01" )  << QString( "/foo/" ) << QString( "/foo/" );
    QTest::newRow( "local02" )  << QString( "/foo/bar" ) << QString( "/foo/bar" );
    QTest::newRow( "local03" )  << QString( "/foo/bar/" ) << QString( "/foo/bar/" );
    QTest::newRow( "local04" )  << QString( "foo" ) << QString( "foo" );
    QTest::newRow( "local05" )  << QString( "foo/" ) << QString( "foo/" );
    QTest::newRow( "local06" )  << QString( "foo/bar" ) << QString( "foo/bar" );
    QTest::newRow( "local07" )  << QString( "foo/bar/" ) << QString( "foo/bar/" );
    QTest::newRow( "local09" )  << QString( "" ) << QString( "" );

    QTest::newRow( "file00" )  << QString( "file:/foo" ) << QString( "file:///foo" );
    QTest::newRow( "file01" )  << QString( "file:/foo/" ) << QString( "file:///foo/" );
    QTest::newRow( "file02" )  << QString( "file:/foo/bar" ) << QString( "file:///foo/bar" );
    QTest::newRow( "file03" )  << QString( "file:/foo/bar/" ) << QString( "file:///foo/bar/" );
    QTest::newRow( "relProtocol00" )  << QString( "foo:bar" ) << QString( "foo:bar" );
    QTest::newRow( "relProtocol01" )  << QString( "foo:/bar" ) << QString( "foo:/bar" );

    QTest::newRow( "windowsDrive00" )  << QString( "c:/" ) << QString( "c:/" );
    QTest::newRow( "windowsDrive01" )  << QString( "c:" ) << QString( "c:" );
    QTest::newRow( "windowsDrive02" )  << QString( "c:/WinNT/" ) << QString( "c:/WinNT/" );
    QTest::newRow( "windowsDrive03" )  << QString( "c:/autoexec.bat" ) << QString( "c:/autoexec.bat" );
    QTest::newRow( "windowsDrive04" )  << QString( "c:WinNT/" ) << QString( "c:WinNT/" );
    QTest::newRow( "windowsDrive05" )  << QString( "c:autoexec.bat" ) << QString( "c:autoexec.bat" );

    QTest::newRow("nopath") << QString("protocol://host") << QString("protocol://host");
}

void tst_QUrl::compat_constructor_03()
{
    QFETCH( QString, urlStr );

    QUrl u( urlStr );
    QTEST( u.toString(), "res" );
}

void tst_QUrl::compat_isValid_01_data()
{
    QTest::addColumn<QString>("urlStr");
    QTest::addColumn<bool>("res");

    QTest::newRow( "ok_01" ) << QString("ftp://ftp.qt-project.org/qt/INSTALL") << (bool)true;
    QTest::newRow( "ok_02" ) << QString( "file:/foo") << (bool)true;
    QTest::newRow( "ok_03" ) << QString( "file:foo") << (bool)true;

    QTest::newRow( "err_01" ) << QString("#ftp://ftp.qt-project.org/qt/INSTALL") << (bool)true;
    QTest::newRow( "err_02" ) << QString( "file:/::foo") << (bool)true;
}

void tst_QUrl::compat_isValid_01()
{
    QFETCH( QString, urlStr );
    QFETCH( bool, res );

    QUrl url( urlStr );
    QCOMPARE( url.isValid(), res );
    if (!res)
        QVERIFY(url.toString().isEmpty());
}

void tst_QUrl::compat_isValid_02_data()
{
    QTest::addColumn<QString>("protocol");
    QTest::addColumn<QString>("user");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("host");
    QTest::addColumn<int>("port");
    QTest::addColumn<QString>("path");
    QTest::addColumn<bool>("res");

    QString n = "";

    QTest::newRow( "ok_01" ) << n     << n     << n     << n                   << -1 << QString("path") << (bool)true;
    QTest::newRow( "ok_02" ) << QString("ftp") << n     << n     << QString("ftp.qt-project.org") << -1 << n      << (bool)true;
    QTest::newRow( "ok_03" ) << QString("ftp") << QString("foo") << n     << QString("ftp.qt-project.org") << -1 << n      << (bool)true;
    QTest::newRow( "ok_04" ) << QString("ftp") << QString("foo") << QString("bar") << QString("ftp.qt-project.org") << -1 << n      << (bool)true;
    QTest::newRow( "ok_05" ) << QString("ftp") << n     << n     << QString("ftp.qt-project.org") << -1 << QString("/path")<< (bool)true;
    QTest::newRow( "ok_06" ) << QString("ftp") << QString("foo") << n     << QString("ftp.qt-project.org") << -1 << QString("/path") << (bool)true;
    QTest::newRow( "ok_07" ) << QString("ftp") << QString("foo") << QString("bar") << QString("ftp.qt-project.org") << -1 << QString("/path")<< (bool)true;

    QTest::newRow( "err_01" ) << n     << n     << n     << n                   << -1 << n << (bool)false;
    QTest::newRow( "err_02" ) << QString("ftp") << n     << n     << n                   << -1 << n << (bool)true;
    QTest::newRow( "err_03" ) << n     << QString("foo") << n     << n                   << -1 << n << (bool)true;
    QTest::newRow( "err_04" ) << n     << n     << QString("bar") << n                   << -1 << n << (bool)true;
    QTest::newRow( "err_05" ) << n     << n     << n     << QString("ftp.qt-project.org") << -1 << n << (bool)true;
    QTest::newRow( "err_06" ) << n     << n     << n     << n                   << 80 << n << (bool)true;
    QTest::newRow( "err_07" ) << QString("ftp") << QString("foo") << n     << n                   << -1 << n << (bool)true;
    QTest::newRow( "err_08" ) << QString("ftp") << n     << QString("bar") << n                   << -1 << n << (bool)true;
    QTest::newRow( "err_09" ) << QString("ftp") << QString("foo") << QString("bar") << n                   << -1 << n << (bool)true;
}

void tst_QUrl::compat_isValid_02()
{
    QFETCH( QString, protocol );
    QFETCH( QString, user );
    QFETCH( QString, password );
    QFETCH( QString, host );
    QFETCH( int, port );
    QFETCH( QString, path );
    QFETCH( bool, res );

    QUrl url;
    if ( !protocol.isEmpty() )
        url.setScheme( protocol );
    if ( !user.isEmpty() )
        url.setUserName( user );
    if ( !password.isEmpty() )
        url.setPassword( password );
    if ( !host.isEmpty() )
        url.setHost( host );
    if ( port != -1 )
        url.setPort( port );
    if ( !path.isEmpty() )
        url.setPath( path );

    QCOMPARE( url.isValid(), res );
    if (!res)
        QVERIFY(url.toString().isEmpty());
}

void tst_QUrl::compat_path_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("res");

    QTest::newRow( "protocol00" ) << "http://qt-project.org/images/ban/pgs_front.jpg" << "/images/ban/pgs_front.jpg";

#if defined( Q_OS_WIN32 )
    QTest::newRow( "winShare00" ) << "//Anarki/homes" << "/homes";
#endif
}

void tst_QUrl::compat_path()
{
    QFETCH( QString, url );

    QUrl u( url );
    QTEST( u.path(), "res" );
}

void tst_QUrl::compat_fileName_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("fileName");

#ifdef Q_OS_WIN32
    QTest::newRow( "Windows - DrivePathFileName - \\" ) << QString("c:\\windows\\tmp\\filename.txt")<< QString("filename.txt");
    QTest::newRow( "Windows - DrivePathFileName - /" ) << QString("c:/windows/tmp/filename.txt") << QString("filename.txt");
    QTest::newRow( "Windows - DrivePathWithSlash - \\" ) << QString("c:\\windows\\tmp\\") << QString();
    QTest::newRow( "Windows - DrivePathWithSlash - /" ) << QString("c:/windows/tmp/") << QString();
    QTest::newRow( "Windows - DrivePathWithoutSlash - \\" ) << QString("c:/windows/tmp") << QString("tmp");
    QTest::newRow( "Windows - DrivePathWithoutSlash - /" ) << QString("c:/windows/tmp") << QString("tmp");
#endif
    QTest::newRow( "Path00" ) << QString("/") << QString();
    QTest::newRow( "Path01" ) << QString("/home/dev/test/") << QString();
    QTest::newRow( "PathFileName00" ) << QString("/home/dev/test") << QString("test");
}

void tst_QUrl::compat_fileName()
{
    QFETCH( QString, url );
    QFETCH( QString, fileName );
    QUrl fileUrl = QUrl::fromLocalFile(url);
    QFileInfo fi(fileUrl.toLocalFile());
    QCOMPARE( fi.fileName(), fileName );
}

void tst_QUrl::compat_decode_data()
{
    QTest::addColumn<QByteArray>("encodedString");
    QTest::addColumn<QString>("decodedString");

    QTest::newRow("NormalString") << QByteArray("filename") << QString("filename");
    QTest::newRow("NormalStringEncoded") << QByteArray("file%20name") << QString("file name");
    QTest::newRow("JustEncoded") << QByteArray("%20") << QString(" ");
    QTest::newRow("HTTPUrl") << QByteArray("http://qt-project.org") << QString("http://qt-project.org");
    QTest::newRow("HTTPUrlEncoded") << QByteArray("http://qt-project%20org") << QString("http://qt-project org");
    QTest::newRow("EmptyString") << QByteArray("") << QString("");
    QTest::newRow("NulByte") << QByteArray("C%00%0A") << QString::fromLatin1("C\0\n", 3);
    QTest::newRow("Task27166") << QByteArray("Fran%C3%A7aise") << QString::fromUtf8("Française");
}

void tst_QUrl::compat_decode()
{
    QFETCH(QByteArray, encodedString);
    QFETCH(QString, decodedString);

    QCOMPARE(QUrl::fromPercentEncoding(encodedString), decodedString);
}

void tst_QUrl::compat_encode_data()
{
    QTest::addColumn<QString>("decodedString");
    QTest::addColumn<QByteArray>("encodedString");

    QTest::newRow("NormalString") << QString("filename") << QByteArray("filename");
    QTest::newRow("NormalStringEncoded") << QString("file name") << QByteArray("file%20name");
    QTest::newRow("JustEncoded") << QString(" ") << QByteArray("%20");
    QTest::newRow("HTTPUrl") << QString("http://qt-project.org") << QByteArray("http%3A//qt-project.org");
    QTest::newRow("HTTPUrlEncoded") << QString("http://qt-project org") << QByteArray("http%3A//qt-project%20org");
    QTest::newRow("EmptyString") << QString("") << QByteArray("");
    QTest::newRow("NulByte") << QString::fromLatin1("C\0\n", 3) << QByteArray("C%00%0A");
    QTest::newRow("Task27166") << QString::fromUtf8("Française") << QByteArray("Fran%C3%A7aise");
}

void tst_QUrl::compat_encode()
{
    QFETCH(QString, decodedString);
    QFETCH(QByteArray, encodedString);

    QCOMPARE(QUrl::toPercentEncoding(decodedString, "/."), encodedString);
}


void tst_QUrl::relative()
{
    QUrl url("../ole");
    QCOMPARE(url.path(), QString::fromLatin1("../ole"));

    QUrl url2("./");
    QCOMPARE(url2.path(), QString::fromLatin1("./"));

    QUrl url3("..");
    QCOMPARE(url3.path(), QString::fromLatin1(".."));

    QUrl url4("../..");
    QCOMPARE(url4.path(), QString::fromLatin1("../.."));
}

void tst_QUrl::percentEncoding_data()
{
    QTest::addColumn<QString>("original");
    QTest::addColumn<QByteArray>("encoded");

    QTest::newRow("test_01") << QString::fromLatin1("sdfsdf") << QByteArray("sdfsdf");
    QTest::newRow("test_02") << QString::fromUtf8("æss") << QByteArray("%C3%A6ss");
    // not unreserved or reserved
    QTest::newRow("test_03") << QString::fromLatin1("{}") << QByteArray("%7B%7D");
}

void tst_QUrl::percentEncoding()
{
    QFETCH(QString, original);
    QFETCH(QByteArray, encoded);

    QCOMPARE(QUrl(original).toEncoded().constData(), encoded.constData());
    QVERIFY(QUrl::fromEncoded(QUrl(original).toEncoded()) == QUrl(original));
    QCOMPARE(QUrl::fromEncoded(QUrl(original).toEncoded()).toString(), original);
    QVERIFY(QUrl::fromEncoded(encoded) == QUrl(original));
    QCOMPARE(QUrl(QUrl(original).toString()).toString(), original);
}

void tst_QUrl::toPercentEncoding_data()
{
    QTest::addColumn<QString>("original");
    QTest::addColumn<QByteArray>("encoded");
    QTest::addColumn<QByteArray>("excludeInEncoding");
    QTest::addColumn<QByteArray>("includeInEncoding");

    QTest::newRow("test_01") << QString::fromLatin1("abcdevghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345678-._~")
                          << QByteArray("abcdevghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ012345678-._~")
                          << QByteArray("")
                          << QByteArray("");
    QTest::newRow("test_02") << QString::fromLatin1("{\t\n\r^\"abc}")
                          << QByteArray("%7B%09%0A%0D%5E%22abc%7D")
                          << QByteArray("")
                          << QByteArray("");
    QTest::newRow("test_03") << QString::fromLatin1("://?#[]@!$&'()*+,;=")
                          << QByteArray("%3A%2F%2F%3F%23%5B%5D%40%21%24%26%27%28%29%2A%2B%2C%3B%3D")
                          << QByteArray("")
                          << QByteArray("");
    QTest::newRow("test_04") << QString::fromLatin1("://?#[]@!$&'()*+,;=")
                          << QByteArray("%3A%2F%2F%3F%23%5B%5D%40!$&'()*+,;=")
                          << QByteArray("!$&'()*+,;=")
                          << QByteArray("");
    QTest::newRow("test_05") << QString::fromLatin1("abcd")
                          << QByteArray("a%62%63d")
                          << QByteArray("")
                          << QByteArray("bc");
}

void tst_QUrl::toPercentEncoding()
{
    QFETCH(QString, original);
    QFETCH(QByteArray, encoded);
    QFETCH(QByteArray, excludeInEncoding);
    QFETCH(QByteArray, includeInEncoding);

    QByteArray encodedUrl = QUrl::toPercentEncoding(original, excludeInEncoding, includeInEncoding);
    QCOMPARE(encodedUrl.constData(), encoded.constData());
    QCOMPARE(original, QUrl::fromPercentEncoding(encodedUrl));
}

void tst_QUrl::swap()
{
    QUrl u1(QLatin1String("http://qt-project.org")), u2(QLatin1String("http://www.kdab.com"));
    u1.swap(u2);
    QCOMPARE(u2.host(),QLatin1String("qt-project.org"));
    QCOMPARE(u1.host(),QLatin1String("www.kdab.com"));
}

void tst_QUrl::symmetry()
{
    QUrl url(QString::fromUtf8("http://www.räksmörgås.se/pub?a=b&a=dø&a=f#vræl"));
    QCOMPARE(url.scheme(), QString::fromLatin1("http"));
    QCOMPARE(url.host(), QString::fromUtf8("www.räksmörgås.se"));
    QCOMPARE(url.path(), QString::fromLatin1("/pub"));
    // this will be encoded ...
    QCOMPARE(url.encodedQuery().constData(), QString::fromLatin1("a=b&a=d%C3%B8&a=f").toLatin1().constData());
    QCOMPARE(url.fragment(), QString::fromUtf8("vræl"));

    QUrl onlyHost("//qt-project.org");
    QCOMPARE(onlyHost.toString(), QString::fromLatin1("//qt-project.org"));

    {
        QString urlString = QString::fromLatin1("http://desktop:33326/upnp/{32f525a6-6f31-426e-91ca-01c2e6c2c57e}");
        QUrl urlPreviewList(urlString);
        QCOMPARE(urlPreviewList.toString(), urlString);
        QByteArray b = urlPreviewList.toEncoded();
        QCOMPARE(b.constData(), "http://desktop:33326/upnp/%7B32f525a6-6f31-426e-91ca-01c2e6c2c57e%7D");
        QCOMPARE(QUrl::fromEncoded(b).toString(), urlString);
        QCOMPARE(QUrl(b).toString(), urlString);
    }
    {
        QString urlString = QString::fromLatin1("http://desktop:53423/deviceDescription?uuid={7977c17b-00bf-4af9-894e-fed28573c3a9}");
        QUrl urlPreviewList(urlString);
        QCOMPARE(urlPreviewList.toString(), urlString);
        QByteArray b = urlPreviewList.toEncoded();
        QCOMPARE(b.constData(), "http://desktop:53423/deviceDescription?uuid=%7B7977c17b-00bf-4af9-894e-fed28573c3a9%7D");
        QCOMPARE(QUrl::fromEncoded(b).toString(), urlString);
        QCOMPARE(QUrl(b).toString(), urlString);
    }
}


void tst_QUrl::ipv6_data()
{
    QTest::addColumn<QString>("ipv6Auth");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<QString>("output");

    QTest::newRow("case 1") << QString::fromLatin1("//[56:56:56:56:56:56:56:56]") << true
                            << "//[56:56:56:56:56:56:56:56]";
    QTest::newRow("case 2") << QString::fromLatin1("//[::56:56:56:56:56:56:56]") << true
                            << "//[0:56:56:56:56:56:56:56]";
    QTest::newRow("case 3") << QString::fromLatin1("//[56::56:56:56:56:56:56]") << true
                            << "//[56:0:56:56:56:56:56:56]";
    QTest::newRow("case 4") << QString::fromLatin1("//[56:56::56:56:56:56:56]") << true
                            << "//[56:56:0:56:56:56:56:56]";
    QTest::newRow("case 5") << QString::fromLatin1("//[56:56:56::56:56:56:56]") << true
                            << "//[56:56:56:0:56:56:56:56]";
    QTest::newRow("case 6") << QString::fromLatin1("//[56:56:56:56::56:56:56]") << true
                            << "//[56:56:56:56:0:56:56:56]";
    QTest::newRow("case 7") << QString::fromLatin1("//[56:56:56:56:56::56:56]") << true
                            << "//[56:56:56:56:56:0:56:56]";
    QTest::newRow("case 8") << QString::fromLatin1("//[56:56:56:56:56:56::56]") << true
                            << "//[56:56:56:56:56:56:0:56]";
    QTest::newRow("case 9") << QString::fromLatin1("//[56:56:56:56:56:56:56::]") << true
                            << "//[56:56:56:56:56:56:56:0]";
    QTest::newRow("case 4 with one less") << QString::fromLatin1("//[56::56:56:56:56:56]") << true
                                          << "//[56::56:56:56:56:56]";
    QTest::newRow("case 4 with less and ip4") << QString::fromLatin1("//[56::56:56:56:127.0.0.1]") << true
                                              << "//[56::56:56:56:7f00:1]";
    QTest::newRow("case 7 with one and ip4") << QString::fromLatin1("//[56::255.0.0.0]") << true
                                             << "//[56::ff00:0]";
    QTest::newRow("case 2 with ip4") << QString::fromLatin1("//[::56:56:56:56:56:0.0.0.255]") << true
                                     << "//[0:56:56:56:56:56:0:ff]";
    QTest::newRow("case 2 with half ip4") << QString::fromLatin1("//[::56:56:56:56:56:56:0.255]") << false << "";
    QTest::newRow("case 4 with less and ip4 and port and useinfo")
            << QString::fromLatin1("//user:pass@[56::56:56:56:127.0.0.1]:99") << true
            << "//user:pass@[56::56:56:56:7f00:1]:99";
    QTest::newRow("case :,") << QString::fromLatin1("//[:,]") << false << "";
    QTest::newRow("case ::bla") << QString::fromLatin1("//[::bla]") << false << "";
    QTest::newRow("case v4-mapped") << "//[0:0:0:0:0:ffff:7f00:1]" << true << "//[::ffff:127.0.0.1]";
}

void tst_QUrl::ipv6()
{
    QFETCH(QString, ipv6Auth);
    QFETCH(bool, isValid);
    QFETCH(QString, output);

    QUrl url(ipv6Auth);

    QCOMPARE(url.isValid(), isValid);
    QCOMPARE(url.toString().isEmpty(), !isValid);
    if (url.isValid()) {
        QCOMPARE(url.toString(), output);
        url.setHost(url.host());
        QVERIFY(url.isValid());
        QCOMPARE(url.toString(), output);
    }
}

void tst_QUrl::ipv6_2_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");

    QTest::newRow("[::ffff:129.144.52.38]")
        << QString("http://[::ffff:129.144.52.38]/cgi/test.cgi")
        << QString("http://[::ffff:129.144.52.38]/cgi/test.cgi");
    QTest::newRow("[::FFFF:129.144.52.38]")
        << QString("http://[::FFFF:129.144.52.38]/cgi/test.cgi")
        << QString("http://[::ffff:129.144.52.38]/cgi/test.cgi");
}

void tst_QUrl::ipv6_2()
{
    QFETCH(QString, input);
    QFETCH(QString, output);

    QUrl url(input);
    QCOMPARE(url.toString(), output);
    url.setHost(url.host());
    QCOMPARE(url.toString(), output);
}

void tst_QUrl::moreIpv6()
{
    QUrl waba1("http://www.kde.org/cgi/test.cgi");
    waba1.setHost("::ffff:129.144.52.38");
    QCOMPARE(QString::fromLatin1(waba1.toEncoded()), QString::fromLatin1("http://[::ffff:129.144.52.38]/cgi/test.cgi"));
}

void tst_QUrl::isRelative_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("trueFalse");

    QTest::newRow("not") << QString::fromLatin1("http://qt-project.org") << false;
    QTest::newRow("55288") << QString::fromLatin1("node64.html#fig:form:ana") << true;

    // kde
    QTest::newRow("mailto: URL, is relative") << "mailto:faure@kde.org" << false;
    QTest::newRow("man: URL, is relative") << "man:mmap" << false;
    QTest::newRow("javascript: URL, is relative") << "javascript:doSomething()" << false;
    QTest::newRow("file: URL, is relative") << "file:/blah" << false;
    QTest::newRow("/path, is relative") << "/path" << true;
    QTest::newRow("something, is relative") << "something" << true;
    // end kde
}

void tst_QUrl::isRelative()
{
    QFETCH(QString, url);
    QFETCH(bool, trueFalse);

    QCOMPARE(QUrl(url).isRelative(), trueFalse);
}

void tst_QUrl::hasQuery_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("trueFalse");

    QTest::newRow("no query items") << "http://www.foo.bar" << false;

    QTest::newRow("empty query") << "http://www.foo.bar?" << true;
    QTest::newRow("empty query 2") << "http://www.foo.bar/?" << true;

    QTest::newRow("query") << "http://www.foo.bar?query" << true;
    QTest::newRow("query=") << "http://www.foo.bar?query=" << true;
    QTest::newRow("query=value") << "http://www.foo.bar?query=value" << true;

    QTest::newRow("%3f") << "http://www.foo.bar/file%3f" << false;
    QTest::newRow("%3f-query") << "http://www.foo.bar/file%3fquery" << false;
    QTest::newRow("%3f-query=value") << "http://www.foo.bar/file%3fquery=value" << false;
}

void tst_QUrl::hasQuery()
{
    QFETCH(QString, url);
    QFETCH(bool, trueFalse);

    QUrl qurl(url);
    QCOMPARE(qurl.hasQuery(), trueFalse);
    QCOMPARE(qurl.encodedQuery().isNull(), !trueFalse);
}

void tst_QUrl::nameprep()
{
    QUrl url(QString::fromUtf8("http://www.fu""\xc3""\x9f""ball.de/"));
    QCOMPARE(url.toString(), QString::fromLatin1("http://www.fussball.de/"));
}

void tst_QUrl::isValid()
{
    {
        QUrl url(QString("A=B"));
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());
        QCOMPARE(url.path(), QString("A=B"));
    }
    {
        QUrl url = QUrl::fromEncoded("http://strange<username>@ok-hostname/", QUrl::StrictMode);
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
        // < and > are not allowed in userinfo in strict mode
        url.setUserName("normal_username");
        QVERIFY(url.isValid());
    }
    {
        QUrl url = QUrl::fromEncoded("http://strange<username>@ok-hostname/");
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());
        // < and > are allowed in tolerant mode
        QCOMPARE(url.toEncoded(), QByteArray("http://strange%3Cusername%3E@ok-hostname/"));
    }
    {
        QUrl url = QUrl::fromEncoded("http://strange;hostname/here");
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
        QCOMPARE(url.path(), QString("/here"));
        url.setAuthority("strange;hostname");
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
        url.setAuthority("foobar@bar");
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());
        url.setAuthority("strange;hostname");
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
        QVERIFY2(url.errorString().contains("Invalid hostname"),
                 qPrintable(url.errorString()));
    }

    {
        QUrl url = QUrl::fromEncoded("foo://stuff;1/g");
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
        QCOMPARE(url.path(), QString("/g"));
        url.setHost("stuff;1");
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
        url.setHost("stuff-1");
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());
        url.setHost("stuff;1");
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
        QVERIFY2(url.errorString().contains("Invalid hostname"),
                 qPrintable(url.errorString()));
    }

    {
        QUrl url("http://example.com");
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());
        url.setPath("relative");
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
        QVERIFY(url.errorString().contains("Path component is relative and authority is present"));
    }

    {
        QUrl url;
        url.setPath("http://example.com");
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
        QVERIFY(url.errorString().contains("':' before any '/'"));
    }
}

void tst_QUrl::schemeValidator_data()
{
    QTest::addColumn<QByteArray>("encodedUrl");
    QTest::addColumn<bool>("result");
    QTest::addColumn<QString>("toString");

    QTest::newRow("empty") << QByteArray() << false << QString();

    // ftp
    QTest::newRow("ftp:") << QByteArray("ftp:") << true << QString("ftp:");
    QTest::newRow("ftp://ftp.qt-project.org")
        << QByteArray("ftp://ftp.qt-project.org")
        << true << QString("ftp://ftp.qt-project.org");
    QTest::newRow("ftp://ftp.qt-project.org/")
        << QByteArray("ftp://ftp.qt-project.org/")
        << true << QString("ftp://ftp.qt-project.org/");
    QTest::newRow("ftp:/index.html")
        << QByteArray("ftp:/index.html")
        << false << QString();

    // mailto
    QTest::newRow("mailto:") << QByteArray("mailto:") << true << QString("mailto:");
    QTest::newRow("mailto://smtp.trolltech.com/ole@bull.name")
        << QByteArray("mailto://smtp.trolltech.com/ole@bull.name") << false << QString();
    QTest::newRow("mailto:") << QByteArray("mailto:") << true << QString("mailto:");
    QTest::newRow("mailto:ole@bull.name")
        << QByteArray("mailto:ole@bull.name") << true << QString("mailto:ole@bull.name");

    // file
    QTest::newRow("file:") << QByteArray("file:/etc/passwd") << true << QString("file:///etc/passwd");
}

void tst_QUrl::schemeValidator()
{
    QFETCH(QByteArray, encodedUrl);
    QFETCH(bool, result);
    QFETCH(QString, toString);

    QUrl url = QUrl::fromEncoded(encodedUrl);
    QEXPECT_FAIL("ftp:/index.html", "high-level URL validation not reimplemented yet", Abort);
    QEXPECT_FAIL("mailto://smtp.trolltech.com/ole@bull.name", "high-level URL validation not reimplemented yet", Abort);
    QCOMPARE(url.isValid(), result);
    if (!result)
        QVERIFY(url.toString().isEmpty());
}

void tst_QUrl::invalidSchemeValidator()
{
    // test that if scheme does not start with an ALPHA, QUrl::isValid() returns false
    {
        QUrl url("1http://qt-project.org");
        QVERIFY(url.scheme().isEmpty());
        QVERIFY(url.path().startsWith("1http"));
    }
    {
        QUrl url("http://qt-project.org");
        url.setScheme("111http://qt-project.org");
        QCOMPARE(url.isValid(), false);
        QVERIFY(url.toString().isEmpty());
    }
    // non-ALPHA character at other positions in the scheme are ok
    {
        QUrl url("ht111tp://qt-project.org", QUrl::StrictMode);
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("ht111tp"));
        QVERIFY(!url.toString().isEmpty());
    }
    {
        QUrl url("http://qt-project.org");
        url.setScheme("ht123tp://qt-project.org");
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
        url.setScheme("http");
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());
    }
    {
        QUrl url = QUrl::fromEncoded("ht321tp://qt-project.org", QUrl::StrictMode);
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());
    }
}

void tst_QUrl::strictParser_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("needle");

    // QUrl doesn't detect an error in the scheme when parsing because
    // it falls back to parsing as a path. So, these errors are path errors
    QTest::newRow("invalid-scheme") << "ht%://example.com" << "character '%' not permitted";
    QTest::newRow("empty-scheme") << ":/" << "':' before any '/'";

    QTest::newRow("invalid-user1") << "http://bad<user_name>@ok-hostname" << "Invalid user name (character '<' not permitted)";
    QTest::newRow("invalid-user2") << "http://bad%@ok-hostname" << "Invalid user name (character '%' not permitted)";

    QTest::newRow("invalid-password") << "http://user:pass\x7F@ok-hostname" << "Invalid password (character '\x7F' not permitted)";

    QTest::newRow("invalid-regname") << "http://bad<hostname>" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("invalid-regname-2") << "http://b%61d" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("invalid-ipv6") << "http://[:::]" << "Invalid IPv6 address";
    QTest::newRow("invalid-ipvfuture-1") << "http://[v7]" << "Invalid IPvFuture address";
    QTest::newRow("invalid-ipvfuture-2") << "http://[v7.]" << "Invalid IPvFuture address";
    QTest::newRow("invalid-ipvfuture-3") << "http://[v789]" << "Invalid IPvFuture address";
    QTest::newRow("unbalanced-brackets") << "http://[ff02::1" << "Expected ']' to match '[' in hostname";

    // invalid hostnames happen in TolerantMode too
    QTest::newRow("invalid-hostname-leading-dot") << "http://.co.uk" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("invalid-hostname-double-dot") << "http://co..uk" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("invalid-hostname-non-LDH") << "http://foo,bar.example.com" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-space") << "http:// " << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-nbsp") << "http://\xc2\xa0" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-control-1f") << "http://\x1f" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-control-7f") << "http://\x7f" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-control-80") << "http://\xc2\x80" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-private-bmp") << "http://\xee\x80\x80" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-private-plane15") << "http://\xf3\xb0\x80\x80" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-private-plane16") << "http://\xf4\x80\x80\x80" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-ffff") << "http://\xef\xbf\xbf" << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-surrogate-1") << "http://" + QString(QChar(0xD800)) << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-surrogate-2") << "http://" + QString(QChar(0xDC00)) << "Invalid hostname (contains invalid characters)";
    QTest::newRow("idn-prohibited-char-surrogate-3") << "http://" + QString(QChar(0xD800)) + "a" << "Invalid hostname (contains invalid characters)";
    // FIXME: add some tests for prohibited BiDi (RFC 3454 section 6)

    // port errors happen in TolerantMode too
    QTest::newRow("empty-port-1") << "http://example.com:" << "Port field was empty";
    QTest::newRow("empty-port-2") << "http://example.com:/" << "Port field was empty";
    QTest::newRow("invalid-port-1") << "http://example.com:-1" << "Invalid port";
    QTest::newRow("invalid-port-2") << "http://example.com:abc" << "Invalid port";
    QTest::newRow("invalid-port-3") << "http://example.com:9a" << "Invalid port";
    QTest::newRow("port-range") << "http://example.com:65536" << "out of range";

    QTest::newRow("invalid-path") << "foo:/path%\x1F" << "Invalid path (character '%' not permitted)";

    QTest::newRow("invalid-query") << "foo:?\\#" << "Invalid query (character '\\' not permitted)";

    QTest::newRow("invalid-fragment") << "#{}" << "Invalid fragment (character '{' not permitted)";
}

void tst_QUrl::strictParser()
{
    QFETCH(QString, input);
    QFETCH(QString, needle);

    QUrl url(input, QUrl::StrictMode);
    QVERIFY(!url.isValid());
    QVERIFY(url.toString().isEmpty());
    QVERIFY(!url.errorString().isEmpty());
    QVERIFY2(url.errorString().contains(input),
             "Error string does not contain the original input (\"" +
             input.toLatin1() + "\"): " + url.errorString().toLatin1());

    // Note: if the following fails due to changes in the parser, simply update the test data
    QVERIFY2(url.errorString().contains(needle),
             "Error string changed and does not contain \"" +
             needle.toLatin1() + "\" anymore: " + url.errorString().toLatin1());
}

void tst_QUrl::tolerantParser()
{
    {
        QUrl url("http://www.example.com/path%20with spaces.html");
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());
        QCOMPARE(url.path(), QString("/path with spaces.html"));
        QCOMPARE(url.toEncoded(), QByteArray("http://www.example.com/path%20with%20spaces.html"));
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("http://www.example.com/path%20with%20spaces.html"));
        url.setUrl("http://www.example.com/path%20with spaces.html", QUrl::StrictMode);
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
    }
    {
        QUrl url = QUrl::fromEncoded("http://www.example.com/path%20with spaces.html");
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());
        QCOMPARE(url.path(), QString("/path with spaces.html"));
        url.setEncodedUrl("http://www.example.com/path%20with spaces.html", QUrl::StrictMode);
        QVERIFY(!url.isValid());
        QVERIFY(url.toString().isEmpty());
    }

    {
        QUrl url15581("http://alain.knaff.linux.lu/bug-reports/kde/percentage%in%url.htm>");
        QVERIFY(url15581.isValid());
        QCOMPARE(url15581.toEncoded().constData(), "http://alain.knaff.linux.lu/bug-reports/kde/percentage%25in%25url.htm%3E");
    }

    {
        QUrl webkit22616 =
            QUrl::fromEncoded("http://example.com/testya.php?browser-info=s:1400x1050x24:f:9.0%20r152:t:%u0442%u0435%u0441%u0442");
        QVERIFY(webkit22616.isValid());

        // Qt 5 behaviour change: one broken % means all % are considered broken
//      QCOMPARE(webkit22616.toEncoded().constData(),
//               "http://example.com/testya.php?browser-info=s:1400x1050x24:f:9.0%20r152:t:%25u0442%25u0435%25u0441%25u0442");
        QCOMPARE(webkit22616.toEncoded().constData(),
                 "http://example.com/testya.php?browser-info=s:1400x1050x24:f:9.0%2520r152:t:%25u0442%25u0435%25u0441%25u0442");
    }

    {
        QUrl url;
        url.setUrl("http://foo.bar/[image][1].jpg");
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("http://foo.bar/%5Bimage%5D%5B1%5D.jpg"));
        QCOMPARE(url.toEncoded(), QByteArray("http://foo.bar/%5Bimage%5D%5B1%5D.jpg"));
        QCOMPARE(url.toString(), QString("http://foo.bar/[image][1].jpg"));

        url.setUrl("[].jpg");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("%5B%5D.jpg"));
        QCOMPARE(url.toEncoded(), QByteArray("%5B%5D.jpg"));
        QCOMPARE(url.toString(), QString("[].jpg"));

        url.setUrl("/some/[path]/[]");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("/some/%5Bpath%5D/%5B%5D"));
        QCOMPARE(url.toEncoded(), QByteArray("/some/%5Bpath%5D/%5B%5D"));
        QCOMPARE(url.toString(), QString("/some/[path]/[]"));

        url.setUrl("//[::56:56:56:56:56:56:56]");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("//[0:56:56:56:56:56:56:56]"));
        QCOMPARE(url.toEncoded(), QByteArray("//[0:56:56:56:56:56:56:56]"));
        QCOMPARE(url.toString(), QString("//[0:56:56:56:56:56:56:56]"));

        url.setUrl("//[::56:56:56:56:56:56:56]#[]");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("//[0:56:56:56:56:56:56:56]#%5B%5D"));
        QCOMPARE(url.toEncoded(), QByteArray("//[0:56:56:56:56:56:56:56]#%5B%5D"));
        QCOMPARE(url.toString(), QString("//[0:56:56:56:56:56:56:56]#[]"));

        url.setUrl("//[::56:56:56:56:56:56:56]?[]");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("//[0:56:56:56:56:56:56:56]?[]"));
        QCOMPARE(url.toEncoded(), QByteArray("//[0:56:56:56:56:56:56:56]?[]"));
        QCOMPARE(url.toString(), QString("//[0:56:56:56:56:56:56:56]?[]"));

        // invoke the tolerant parser's error correction
        url.setUrl("%hello.com/f%");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("%25hello.com/f%25"));
        QCOMPARE(url.toEncoded(), QByteArray("%25hello.com/f%25"));
        QCOMPARE(url.toString(), QString("%25hello.com/f%25"));

        url.setEncodedUrl("http://www.host.com/foo.php?P0=[2006-3-8]");
        QVERIFY(url.isValid());
        QVERIFY(!url.toString().isEmpty());

        url.setEncodedUrl("http://foo.bar/[image][1].jpg");
        QVERIFY(url.isValid());
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("http://foo.bar/%5Bimage%5D%5B1%5D.jpg"));
        QCOMPARE(url.toEncoded(), QByteArray("http://foo.bar/%5Bimage%5D%5B1%5D.jpg"));
        QCOMPARE(url.toString(), QString("http://foo.bar/[image][1].jpg"));

        url.setEncodedUrl("[].jpg");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("%5B%5D.jpg"));
        QCOMPARE(url.toEncoded(), QByteArray("%5B%5D.jpg"));
        QCOMPARE(url.toString(), QString("[].jpg"));

        url.setEncodedUrl("/some/[path]/[]");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("/some/%5Bpath%5D/%5B%5D"));
        QCOMPARE(url.toEncoded(), QByteArray("/some/%5Bpath%5D/%5B%5D"));
        QCOMPARE(url.toString(), QString("/some/[path]/[]"));

        url.setEncodedUrl("//[::56:56:56:56:56:56:56]");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("//[0:56:56:56:56:56:56:56]"));
        QCOMPARE(url.toEncoded(), QByteArray("//[0:56:56:56:56:56:56:56]"));

        url.setEncodedUrl("//[::56:56:56:56:56:56:56]#[]");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("//[0:56:56:56:56:56:56:56]#%5B%5D"));
        QCOMPARE(url.toEncoded(), QByteArray("//[0:56:56:56:56:56:56:56]#%5B%5D"));
        QCOMPARE(url.toString(), QString("//[0:56:56:56:56:56:56:56]#[]"));

        url.setEncodedUrl("//[::56:56:56:56:56:56:56]?[]");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("//[0:56:56:56:56:56:56:56]?[]"));
        QCOMPARE(url.toEncoded(), QByteArray("//[0:56:56:56:56:56:56:56]?[]"));
        QCOMPARE(url.toString(), QString("//[0:56:56:56:56:56:56:56]?[]"));

        url.setEncodedUrl("data:text/css,div%20{%20border-right:%20solid;%20}");
        QCOMPARE(url.toString(QUrl::FullyEncoded), QString("data:text/css,div%20%7B%20border-right:%20solid;%20%7D"));
        QCOMPARE(url.toEncoded(), QByteArray("data:text/css,div%20%7B%20border-right:%20solid;%20%7D"));
        QCOMPARE(url.toString(), QString("data:text/css,div { border-right: solid; }"));
    }

    {
        QByteArray tsdgeos("http://google.com/c?c=Translation+%C2%BB+trunk|");
        QUrl tsdgeosQUrl;
        tsdgeosQUrl.setEncodedUrl(tsdgeos, QUrl::TolerantMode);
        QVERIFY(tsdgeosQUrl.isValid()); // failed in Qt-4.4, works in Qt-4.5
        QByteArray tsdgeosExpected("http://google.com/c?c=Translation+%C2%BB+trunk%7C");
        QCOMPARE(QString(tsdgeosQUrl.toEncoded()), QString(tsdgeosExpected));
    }

    {
        QUrl url;
        url.setUrl("http://strange<username>@hostname/", QUrl::TolerantMode);
        QVERIFY(url.isValid());
        QCOMPARE(QString(url.toEncoded()), QString("http://strange%3Cusername%3E@hostname/"));
    }

    {
        QUrl url;
        url.setUrl("http://en%63o%64%65%64.hostname/", QUrl::TolerantMode);
        QVERIFY(url.isValid());
        QCOMPARE(url.toString(), QString("http://encoded.hostname/"));
    }
}

void tst_QUrl::correctEncodedMistakes_data()
{
    QTest::addColumn<QByteArray>("encodedUrl");
    QTest::addColumn<bool>("result");
    QTest::addColumn<QString>("toDecoded");

    QTest::newRow("%") << QByteArray("%") << true << QString("%25");
    QTest::newRow("3%") << QByteArray("3%") << true << QString("3%25");
    QTest::newRow("13%") << QByteArray("13%") << true << QString("13%25");
    QTest::newRow("13%!") << QByteArray("13%!") << true << QString("13%25!");
    QTest::newRow("13%!!") << QByteArray("13%!!") << true << QString("13%25!!");
    QTest::newRow("13%a") << QByteArray("13%a") << true << QString("13%25a");
    QTest::newRow("13%az") << QByteArray("13%az") << true << QString("13%25az");
    QTest::newRow("13%25") << QByteArray("13%25") << true << QString("13%25");
}

void tst_QUrl::correctEncodedMistakes()
{
    QFETCH(QByteArray, encodedUrl);
    QFETCH(bool, result);
    QFETCH(QString, toDecoded);

    QUrl url = QUrl::fromEncoded(encodedUrl);
    QCOMPARE(url.isValid(), result);
    if (url.isValid()) {
        QCOMPARE(url.toString(), toDecoded);
    } else {
        QVERIFY(url.toString().isEmpty());
    }
}

void tst_QUrl::correctDecodedMistakes_data()
{
    QTest::addColumn<QString>("decodedUrl");
    QTest::addColumn<bool>("result");
    QTest::addColumn<QString>("toDecoded");

    QTest::newRow("%") << QString("%") << true << QString("%25");
    QTest::newRow("3%") << QString("3%") << true << QString("3%25");
    QTest::newRow("13%") << QString("13%") << true << QString("13%25");
    QTest::newRow("13%!") << QString("13%!") << true << QString("13%25!");
    QTest::newRow("13%!!") << QString("13%!!") << true << QString("13%25!!");
    QTest::newRow("13%a") << QString("13%a") << true << QString("13%25a");
    QTest::newRow("13%az") << QString("13%az") << true << QString("13%25az");
}

void tst_QUrl::correctDecodedMistakes()
{
    QFETCH(QString, decodedUrl);
    QFETCH(bool, result);
    QFETCH(QString, toDecoded);

    QUrl url(decodedUrl);
    QCOMPARE(url.isValid(), result);
    if (url.isValid()) {
        QCOMPARE(url.toString(), toDecoded);
    } else {
        QVERIFY(url.toString().isEmpty());
    }
}

void tst_QUrl::tldRestrictions_data()
{
    QTest::addColumn<QString>("tld");
    QTest::addColumn<bool>("encode");

    // current whitelist
    QTest::newRow("ac")  << QString("ac")  << true;
    QTest::newRow("ar")  << QString("ar")  << true;
    QTest::newRow("asia")  << QString("asia")  << true;
    QTest::newRow("at") << QString("at") << true;
    QTest::newRow("biz")  << QString("biz")  << true;
    QTest::newRow("br") << QString("br") << true;
    QTest::newRow("cat")  << QString("cat")  << true;
    QTest::newRow("ch")  << QString("ch")  << true;
    QTest::newRow("cl")  << QString("cl")  << true;
    QTest::newRow("cn") << QString("cn") << true;
    QTest::newRow("com")  << QString("com")  << true;
    QTest::newRow("de")  << QString("de")  << true;
    QTest::newRow("dk") << QString("dk") << true;
    QTest::newRow("es")  << QString("es")  << true;
    QTest::newRow("fi") << QString("fi") << true;
    QTest::newRow("gr")  << QString("gr")  << true;
    QTest::newRow("hu") << QString("hu") << true;
    QTest::newRow("il")  << QString("il")  << true;
    QTest::newRow("info")  << QString("info")  << true;
    QTest::newRow("io") << QString("io") << true;
    QTest::newRow("is")  << QString("is")  << true;
    QTest::newRow("ir")  << QString("ir")  << true;
    QTest::newRow("jp") << QString("jp") << true;
    QTest::newRow("kr") << QString("kr") << true;
    QTest::newRow("li")  << QString("li")  << true;
    QTest::newRow("lt") << QString("lt") << true;
    QTest::newRow("lu")  << QString("lu")  << true;
    QTest::newRow("lv")  << QString("lv")  << true;
    QTest::newRow("museum") << QString("museum") << true;
    QTest::newRow("name")  << QString("name")  << true;
    QTest::newRow("net")  << QString("name")  << true;
    QTest::newRow("no") << QString("no") << true;
    QTest::newRow("nu")  << QString("nu")  << true;
    QTest::newRow("nz")  << QString("nz")  << true;
    QTest::newRow("org")  << QString("org")  << true;
    QTest::newRow("pl")  << QString("pl")  << true;
    QTest::newRow("pr")  << QString("pr")  << true;
    QTest::newRow("se")  << QString("se")  << true;
    QTest::newRow("sh") << QString("sh") << true;
    QTest::newRow("tel")  << QString("tel")  << true;
    QTest::newRow("th")  << QString("th")  << true;
    QTest::newRow("tm")  << QString("tm")  << true;
    QTest::newRow("tw") << QString("tw") << true;
    QTest::newRow("ua")  << QString("ua")  << true;
    QTest::newRow("vn") << QString("vn") << true;

    // known blacklists:
    QTest::newRow("foo") << QString("foo") << false;
}

void tst_QUrl::tldRestrictions()
{
    QFETCH(QString, tld);

    // www.brød.tld
    QByteArray ascii = "www.xn--brd-1na." + tld.toLatin1();
    QString unicode = QLatin1String("www.br\370d.") + tld;
    QString encoded = QUrl::fromAce(ascii);
    QTEST(!encoded.contains(".xn--"), "encode");
    QTEST(encoded == unicode, "encode");

    QUrl url = QUrl::fromEncoded("http://www.xn--brd-1na." + tld.toLatin1());
    QTEST(!url.host().contains(".xn--"), "encode");
    QTEST(url.host() == unicode, "encode");

    url.setUrl(QLatin1String("http://www.xn--brd-1na.") + tld);
    QTEST(!url.host().contains(".xn--"), "encode");
    QTEST(url.host() == unicode, "encode");

    url.setUrl(QLatin1String("http://www.br\370d.") + tld);
    QTEST(!url.host().contains(".xn--"), "encode");
    QTEST(url.host() == unicode, "encode");

    url = QUrl::fromEncoded("http://www.br%C3%B8d." + tld.toLatin1());
    QTEST(!url.host().contains(".xn--"), "encode");
    QTEST(url.host() == unicode, "encode");
}

void tst_QUrl::emptyQueryOrFragment()
{
    QUrl qurl = QUrl::fromEncoded("http://www.kde.org/cgi/test.cgi?", QUrl::TolerantMode);
    QCOMPARE(qurl.toEncoded().constData(), "http://www.kde.org/cgi/test.cgi?"); // Empty refs should be preserved
    QCOMPARE(qurl.toString(), QString("http://www.kde.org/cgi/test.cgi?"));
    qurl = QUrl::fromEncoded("http://www.kde.org/cgi/test.cgi#", QUrl::TolerantMode);
    QCOMPARE(qurl.toEncoded().constData(), "http://www.kde.org/cgi/test.cgi#");
    QCOMPARE(qurl.toString(), QString("http://www.kde.org/cgi/test.cgi#"));

    {
        // start with an empty one
        QUrl url("http://www.foo.bar/baz");
        QVERIFY(!url.hasFragment());
        QVERIFY(url.fragment().isNull());

        // add fragment
        url.setFragment(QLatin1String("abc"));
        QVERIFY(url.hasFragment());
        QCOMPARE(url.fragment(), QString(QLatin1String("abc")));
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz#abc")));

        // remove fragment
        url.setFragment(QString());
        QVERIFY(!url.hasFragment());
        QVERIFY(url.fragment().isNull());
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz")));

        // add empty fragment
        url.setFragment(QLatin1String(""));
        QVERIFY(url.hasFragment());
        QVERIFY(url.fragment().isEmpty());
        QVERIFY(!url.fragment().isNull());
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz#")));
    }

    {
        // start with an empty one
        QUrl url("http://www.foo.bar/baz");
        QVERIFY(!url.hasQuery());
        QVERIFY(url.encodedQuery().isNull());

        // add encodedQuery
        url.setQuery("abc=def");
        QVERIFY(url.hasQuery());
        QCOMPARE(url.query(), QString(QLatin1String("abc=def")));
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz?abc=def")));
        url.setEncodedQuery("abc=def");
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz?abc=def")));

        // remove encodedQuery
        url.setQuery(QString());
        QVERIFY(!url.hasQuery());
        QVERIFY(url.encodedQuery().isNull());
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz")));
        url.setEncodedQuery(QByteArray());
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz")));

        // add empty encodedQuery
        url.setQuery("");
        QVERIFY(url.hasQuery());
        QVERIFY(url.encodedQuery().isEmpty());
        QVERIFY(!url.encodedQuery().isNull());
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz?")));
        url.setEncodedQuery("");
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz?")));
    }
}

void tst_QUrl::hasFragment_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("trueFalse");

    QTest::newRow("no fragment") << "http://www.foo.bar" << false;

    QTest::newRow("empty fragment") << "http://www.foo.bar#" << true;
    QTest::newRow("empty fragment 2") << "http://www.foo.bar/#" << true;

    QTest::newRow("fragment") << "http://www.foo.bar#baz" << true;
    QTest::newRow("fragment2") << "http://www.foo.bar/#baz" << true;

    QTest::newRow("%23") << "http://www.foo.bar/%23" << false;
    QTest::newRow("%23-and-something") << "http://www.foo.bar/%23baz" << false;
}

void tst_QUrl::hasFragment()
{
    QFETCH(QString, url);
    QFETCH(bool, trueFalse);

    QUrl qurl(url);
    QCOMPARE(qurl.hasFragment(), trueFalse);
    QCOMPARE(qurl.fragment().isNull(), !trueFalse);
}

void tst_QUrl::setEncodedFragment_data()
{
    QTest::addColumn<QByteArray>("base");
    QTest::addColumn<QByteArray>("fragment");
    QTest::addColumn<QByteArray>("expected");

    typedef QByteArray BA;
    QTest::newRow("null") << BA("http://www.kde.org") << BA() << BA("http://www.kde.org");
    QTest::newRow("empty") << BA("http://www.kde.org") << BA("") << BA("http://www.kde.org#");
    QTest::newRow("basic test") << BA("http://www.kde.org") << BA("abc") << BA("http://www.kde.org#abc");
    QTest::newRow("initial url has fragment") << BA("http://www.kde.org#old") << BA("new") << BA("http://www.kde.org#new");
    QTest::newRow("encoded fragment") << BA("http://www.kde.org") << BA("a%20c") << BA("http://www.kde.org#a%20c");
    QTest::newRow("with #") << BA("http://www.kde.org") << BA("a#b") << BA("http://www.kde.org#a#b");
    QTest::newRow("unicode") << BA("http://www.kde.org") << BA("\xc3\xa9") << BA("http://www.kde.org#%C3%A9");
    QTest::newRow("binary") << BA("http://www.kde.org") << BA("\x00\xc0\x80", 3) << BA("http://www.kde.org#%00%C0%80");
}

void tst_QUrl::setEncodedFragment()
{
    QFETCH(QByteArray, base);
    QFETCH(QByteArray, fragment);
    QFETCH(QByteArray, expected);
    QUrl u;
    u.setEncodedUrl(base, QUrl::TolerantMode);
    QVERIFY(u.isValid());
    u.setEncodedFragment(fragment);
    QVERIFY(u.isValid());
    QCOMPARE(!fragment.isNull(), u.hasFragment());
    QCOMPARE(QString::fromLatin1(u.toEncoded()), QString::fromLatin1(expected));
}

void tst_QUrl::fromEncoded()
{
    QUrl qurl2 = QUrl::fromEncoded("print:/specials/Print%20To%20File%20(PDF%252FAcrobat)", QUrl::TolerantMode);
    QCOMPARE(qurl2.path(), QString::fromLatin1("/specials/Print To File (PDF%252FAcrobat)"));
    QCOMPARE(QFileInfo(qurl2.path()).fileName(), QString::fromLatin1("Print To File (PDF%252FAcrobat)"));
    QCOMPARE(qurl2.toEncoded().constData(), "print:/specials/Print%20To%20File%20(PDF%252FAcrobat)");

    QUrl qurl = QUrl::fromEncoded("http://\303\244.de");
    QVERIFY(qurl.isValid());
    QCOMPARE(qurl.toEncoded().constData(), "http://xn--4ca.de");

    QUrl qurltest(QUrl::fromPercentEncoding("http://\303\244.de"));
    QVERIFY(qurltest.isValid());

    QUrl qurl_newline_1 = QUrl::fromEncoded("http://www.foo.bar/foo/bar\ngnork", QUrl::TolerantMode);
    QVERIFY(qurl_newline_1.isValid());
    QCOMPARE(qurl_newline_1.toEncoded().constData(), "http://www.foo.bar/foo/bar%0Agnork");
}

void tst_QUrl::stripTrailingSlash_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expected");

    QTest::newRow("ftp no slash") << "ftp://ftp.de.kde.org/dir" << "ftp://ftp.de.kde.org/dir";
    QTest::newRow("ftp slash") << "ftp://ftp.de.kde.org/dir/" << "ftp://ftp.de.kde.org/dir";
    QTest::newRow("file slash") << "file:///dir/" << "file:///dir";
    QTest::newRow("file no slash") << "file:///dir/" << "file:///dir";
    QTest::newRow("file root") << "file:///" << "file:///";
    QTest::newRow("no path") << "remote://" << "remote://";
}

void tst_QUrl::stripTrailingSlash()
{
    QFETCH(QString, url);
    QFETCH(QString, expected);

    QUrl u(url);
    QCOMPARE(u.toString(QUrl::StripTrailingSlash), expected);
}

void tst_QUrl::hosts_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("host");

    QTest::newRow("empty") << QString("") << QString("");
    QTest::newRow("empty1") << QString("file:///file") << QString("");
    QTest::newRow("empty2") << QString("file:/file") << QString("");
    QTest::newRow("empty3") << QString("http:///file") << QString("");
    QTest::newRow("empty4") << QString("http:/file") << QString("");

    // numeric hostnames -> decoded as IPv4 as per inet_aton(3)
    QTest::newRow("http://123/") << QString("http://123/") << QString("0.0.0.123");
    QTest::newRow("http://456/") << QString("http://456/") << QString("0.0.1.200");
    QTest::newRow("http://1000/") << QString("http://1000/") << QString("0.0.3.232");

    // IP literals
    QTest::newRow("normal-ip-literal") << QString("http://1.2.3.4") << QString("1.2.3.4");
    QTest::newRow("normal-ip-literal-with-port") << QString("http://1.2.3.4:80")
                                                 << QString("1.2.3.4");
    QTest::newRow("ipv6-literal") << QString("http://[::1]") << QString("::1");
    QTest::newRow("ipv6-literal-with-port") << QString("http://[::1]:80") << QString("::1");
    QTest::newRow("long-ipv6-literal") << QString("http://[2001:200:0:8002:203:47ff:fea5:3085]")
                                       << QString("2001:200:0:8002:203:47ff:fea5:3085");
    QTest::newRow("long-ipv6-literal-with-port") << QString("http://[2001:200:0:8002:203:47ff:fea5:3085]:80")
                                                 << QString("2001:200:0:8002:203:47ff:fea5:3085");
    QTest::newRow("ipv6-literal-v4compat") << QString("http://[::255.254.253.252]")
                                           << QString("::255.254.253.252");
    QTest::newRow("ipv6-literal-v4mapped") << QString("http://[::ffff:255.254.253.252]")
                                           << QString("::ffff:255.254.253.252");
    QTest::newRow("ipv6-literal-v4mapped-2") << QString("http://[::ffff:fffe:fdfc]")
                                           << QString("::ffff:255.254.253.252");

    // no embedded v4 unless the cases above
    QTest::newRow("ipv6-literal-v4decoded") << QString("http://[1000::ffff:127.128.129.1]")
                                            << QString("1000::ffff:7f80:8101");
    QTest::newRow("long-ipv6-literal-v4decoded") << QString("http://[fec0:8000::8002:1000:ffff:200.100.50.250]")
                                                 << QString("fec0:8000:0:8002:1000:ffff:c864:32fa");
    QTest::newRow("longer-ipv6-literal-v4decoded") << QString("http://[fec0:8000:4000:8002:1000:ffff:200.100.50.250]")
                                                   << QString("fec0:8000:4000:8002:1000:ffff:c864:32fa");

    // normal hostnames
    QTest::newRow("normal") << QString("http://intern") << QString("intern");
    QTest::newRow("normal2") << QString("http://qt-project.org") << QString("qt-project.org");

    // IDN hostnames
    QTest::newRow("idn") << QString(QLatin1String("http://\345r.no")) << QString(QLatin1String("\345r.no"));
    QTest::newRow("idn-ace") << QString("http://xn--r-1fa.no") << QString(QLatin1String("\345r.no"));
}

void tst_QUrl::hosts()
{
    QFETCH(QString, url);

    QTEST(QUrl(url).host(), "host");
}

void tst_QUrl::setPort()
{
    {
        QUrl url;
        QVERIFY(url.toString().isEmpty());
        url.setHost("a");
        url.setPort(80);
        QCOMPARE(url.port(), 80);
        QCOMPARE(url.toString(), QString::fromLatin1("//a:80"));
        url.setPort(-1);
        url.setHost(QString());
        QCOMPARE(url.port(), -1);
        QCOMPARE(url.toString(), QString());
        url.setPort(80);
        url.setPort(65536);
        QCOMPARE(url.port(), -1);
        QVERIFY(url.errorString().contains("out of range"));
    }
}

void tst_QUrl::toEncoded_data()
{
    QTest::addColumn<QByteArray>("url");
    QTest::addColumn<QUrl::FormattingOptions>("options");
    QTest::addColumn<QByteArray>("encoded");
    QTest::newRow("file:///dir/") << QByteArray("file:///dir/")
                                  << QUrl::FormattingOptions(QUrl::StripTrailingSlash)
                                  << QByteArray("file:///dir");
}

void tst_QUrl::toEncoded()
{
    QFETCH(QByteArray, url);
    QFETCH(QUrl::FormattingOptions, options);
    QFETCH(QByteArray, encoded);

    QCOMPARE(QUrl::fromEncoded(url).toEncoded(options), encoded);
}

void tst_QUrl::setAuthority_data()
{
    QTest::addColumn<QString>("authority");
    QTest::addColumn<QString>("url");
    QTest::newRow("Plain auth") << QString("62.70.27.22:21") << QString("//62.70.27.22:21");
    QTest::newRow("Yet another plain auth") << QString("192.168.1.1:21") << QString("//192.168.1.1:21");
    QTest::newRow("Auth without port") << QString("192.168.1.1") << QString("//192.168.1.1");
    QTest::newRow("Auth w/full hostname without port") << QString("shusaku.troll.no") << QString("//shusaku.troll.no");
    QTest::newRow("Auth w/hostname without port") << QString("shusaku") << QString("//shusaku");
    QTest::newRow("Auth w/full hostname that ends with number, without port") << QString("shusaku.troll.no.2") << QString("//shusaku.troll.no.2");
    QTest::newRow("Auth w/hostname that ends with number, without port") << QString("shusaku2") << QString("//shusaku2");
    QTest::newRow("Empty auth") << QString() << QString();
}

void tst_QUrl::setAuthority()
{
    QUrl u;
    QFETCH(QString, authority);
    QFETCH(QString, url);
    u.setAuthority(authority);
    QCOMPARE(u.toString(), url);
}

void tst_QUrl::setEmptyAuthority_data()
{
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("authority");
    QTest::addColumn<QString>("expectedUrlString");

    QTest::newRow("null host and authority") << QString() << QString() << QString("");
    QTest::newRow("empty host and authority") << QString("") << QString("") << QString("//");
}

void tst_QUrl::setEmptyAuthority()
{
    QFETCH(QString, host);
    QFETCH(QString, authority);
    QFETCH(QString, expectedUrlString);
    QUrl u;
    u.setHost(host);
    QCOMPARE(u.toString(), expectedUrlString);
    u.setAuthority(authority);
    QCOMPARE(u.toString(), expectedUrlString);
}

void tst_QUrl::clear()
{
    QUrl url("a");
    QUrl url2("a");
    QCOMPARE(url, url2);
    url.clear();
    QVERIFY(url != url2);
}

void tst_QUrl::binaryData_data()
{
    QTest::addColumn<QString>("url");
    QTest::newRow("username") << "http://%01%0D%0A%7F@foo/";
    QTest::newRow("username-at") << "http://abc%40_def@foo/";
    QTest::newRow("username-nul") << "http://abc%00_def@foo/";
    QTest::newRow("username-colon") << "http://abc%3A_def@foo/";
    QTest::newRow("username-nonutf8") << "http://abc%E1_def@foo/";

    QTest::newRow("password") << "http://user:%01%0D%0A%7F@foo/";
    QTest::newRow("password-at") << "http://user:abc%40_def@foo/";
    QTest::newRow("password-nul") << "http://user:abc%00_def@foo/";
    QTest::newRow("password-nonutf8") << "http://user:abc%E1_def@foo/";

    QTest::newRow("file") << "http://foo/%01%0D%0A%7F";
    QTest::newRow("file-nul") << "http://foo/abc%00_def";
    QTest::newRow("file-hash") << "http://foo/abc%23_def";
    QTest::newRow("file-question") << "http://foo/abc%3F_def";
    QTest::newRow("file-nonutf8") << "http://foo/abc%E1_def";
    QTest::newRow("file-slash") << "http://foo/abc%2F_def";

    QTest::newRow("ref") << "http://foo/file#a%01%0D%0A%7F";
    QTest::newRow("ref-nul") << "http://foo/file#abc%00_def";
    QTest::newRow("ref-question") << "http://foo/file#abc?_def";
    QTest::newRow("ref-nonutf8") << "http://foo/file#abc%E1_def";

    QTest::newRow("query-value") << "http://foo/query?foo=%01%0D%0A%7F";
    QTest::newRow("query-value-nul") << "http://foo/query?foo=abc%00_def";
    QTest::newRow("query-value-nonutf8") << "http://foo/query?foo=abc%E1_def";

    QTest::newRow("query-name") << "http://foo/query/a%01%0D%0A%7Fz=foo";
    QTest::newRow("query-name-nul") << "http://foo/query/abc%00_def=foo";
    QTest::newRow("query-name-nonutf8") << "http://foo/query/abc%E1_def=foo";
}

void tst_QUrl::binaryData()
{
    QFETCH(QString, url);
    QUrl u = QUrl::fromEncoded(url.toUtf8());

    QVERIFY(u.isValid());
    QVERIFY(!u.isEmpty());

    QString url2 = QString::fromUtf8(u.toEncoded());
    //QCOMPARE(url2.length(), url.length());
    QCOMPARE(url2, url);
}

void tst_QUrl::fromUserInput_data()
{
    //
    // most of this test is:
    //  Copyright (C) Research In Motion Limited 2009. All rights reserved.
    // Distributed under the BSD license.
    // See qurl.cpp
    //

    QTest::addColumn<QString>("string");
    QTest::addColumn<QUrl>("guessUrlFromString");

    // Null
    QTest::newRow("null") << QString() << QUrl();

    // File
    QDirIterator it(QDir::homePath());
    int c = 0;
    while (it.hasNext()) {
        it.next();
        QTest::newRow(QString("file-%1").arg(c++).toLatin1()) << it.filePath() << QUrl::fromLocalFile(it.filePath());
    }

    // basic latin1
    QTest::newRow("unicode-0") << QString::fromUtf8("\xc3\xa5.com/") << QUrl::fromEncoded(QString::fromUtf8("http://\xc3\xa5.com/").toUtf8(), QUrl::TolerantMode);
    QTest::newRow("unicode-0b") << QString::fromUtf8("\xc3\xa5.com/") << QUrl::fromEncoded("http://%C3%A5.com/", QUrl::TolerantMode);
    QTest::newRow("unicode-0c") << QString::fromUtf8("\xc3\xa5.com/") << QUrl::fromEncoded("http://xn--5ca.com/", QUrl::TolerantMode);
    // unicode
    QTest::newRow("unicode-1") << QString::fromUtf8("\xce\xbb.com/") << QUrl::fromEncoded(QString::fromUtf8("http://\xce\xbb.com/").toUtf8(), QUrl::TolerantMode);
    QTest::newRow("unicode-1b") << QString::fromUtf8("\xce\xbb.com/") << QUrl::fromEncoded("http://%CE%BB.com/", QUrl::TolerantMode);
    QTest::newRow("unicode-1c") << QString::fromUtf8("\xce\xbb.com/") << QUrl::fromEncoded("http://xn--wxa.com/", QUrl::TolerantMode);

    // no scheme
    QTest::newRow("add scheme-0") << "example.org" << QUrl("http://example.org");
    QTest::newRow("add scheme-1") << "www.example.org" << QUrl("http://www.example.org");
    QTest::newRow("add scheme-2") << "ftp.example.org" << QUrl("ftp://ftp.example.org");
    QTest::newRow("add scheme-3") << "hostname" << QUrl("http://hostname");

    // no host
    QTest::newRow("nohost-1") << "http://" << QUrl("http://");
    QTest::newRow("nohost-2") << "smb:" << QUrl("smb:");

    // QUrl's tolerant parser should already handle this
    QTest::newRow("not-encoded-0") << "http://example.org/test page.html" << QUrl::fromEncoded("http://example.org/test%20page.html");

    // Make sure the :80, i.e. port doesn't screw anything up
    QUrl portUrl("http://example.org");
    portUrl.setPort(80);
    QTest::newRow("port-0") << "example.org:80" << portUrl;
    QTest::newRow("port-1") << "http://example.org:80" << portUrl;
    portUrl.setPath("/path");
    QTest::newRow("port-2") << "example.org:80/path" << portUrl;
    QTest::newRow("port-3") << "http://example.org:80/path" << portUrl;

    // mailto doesn't have a ://, but is valid
    QUrl mailto("ben@example.net");
    mailto.setScheme("mailto");
    QTest::newRow("mailto") << "mailto:ben@example.net" << mailto;

    // misc
    QTest::newRow("localhost-1") << "localhost:80" << QUrl("http://localhost:80");
    QTest::newRow("spaces-0") << "  http://example.org/test page.html " << QUrl("http://example.org/test%20page.html");
    QTest::newRow("trash-0") << "example.org/test?someData=42%&someOtherData=abcde#anchor" << QUrl::fromEncoded("http://example.org/test?someData=42%25&someOtherData=abcde#anchor");
    QTest::newRow("other-scheme-0") << "spotify:track:0hO542doVbfGDAGQULMORT" << QUrl("spotify:track:0hO542doVbfGDAGQULMORT");
    QTest::newRow("other-scheme-1") << "weirdscheme:80:otherstuff" << QUrl("weirdscheme:80:otherstuff");
    QTest::newRow("number-path-0") << "tel:2147483648" << QUrl("tel:2147483648");

    // FYI: The scheme in the resulting url user
    QUrl authUrl("user:pass@domain.com");
    QTest::newRow("misc-1") << "user:pass@domain.com" << authUrl;

    // FTP with double slashes in path
    QTest::newRow("ftp-double-slash-1") << "ftp.example.com//path" << QUrl("ftp://ftp.example.com/%2Fpath");
    QTest::newRow("ftp-double-slash-1") << "ftp://ftp.example.com//path" << QUrl("ftp://ftp.example.com/%2Fpath");
}

void tst_QUrl::fromUserInput()
{
    QFETCH(QString, string);
    QFETCH(QUrl, guessUrlFromString);

    QUrl url = QUrl::fromUserInput(string);
    QCOMPARE(url, guessUrlFromString);
}

// This is a regression test for a previously fixed bug where isEmpty didn't
// work for an encoded URL that was yet to be decoded.  The test checks that
// isEmpty works for an encoded URL both after and before decoding.
void tst_QUrl::isEmptyForEncodedUrl()
{
    {
        QUrl url;
        url.setEncodedUrl("LABEL=USB_STICK", QUrl::TolerantMode);
        QVERIFY( url.isValid() );
        QCOMPARE( url.path(), QString("LABEL=USB_STICK") );
        QVERIFY( !url.isEmpty() );
    }
    {
        QUrl url;
        url.setEncodedUrl("LABEL=USB_STICK", QUrl::TolerantMode);
        QVERIFY( url.isValid() );
        QVERIFY( !url.isEmpty() );
        QCOMPARE( url.path(), QString("LABEL=USB_STICK") );
    }
}

// This test verifies that the QUrl::toEncoded() does not rely on the
// potentially uninitialized unencoded path.
void tst_QUrl::toEncodedNotUsingUninitializedPath()
{
    QUrl url;
    url.setEncodedPath("/test.txt");
    url.setHost("example.com");

    QCOMPARE(url.toEncoded().constData(), "//example.com/test.txt");

    url.path();
    QCOMPARE(url.toEncoded().constData(), "//example.com/test.txt");
}

void tst_QUrl::resolvedWithAbsoluteSchemes() const
{
    QFETCH(QUrl, base);
    QFETCH(QUrl, relative);
    QFETCH(QUrl, expected);

    /* Check our input. */
    QVERIFY(relative.isValid());
    QVERIFY(base.isValid());
    QVERIFY(expected.isValid());

    const QUrl result(base.resolved(relative));

    QVERIFY(result.isValid());
    QCOMPARE(result, expected);
}

void tst_QUrl::resolvedWithAbsoluteSchemes_data() const
{
    QTest::addColumn<QUrl>("base");
    QTest::addColumn<QUrl>("relative");
    QTest::addColumn<QUrl>("expected");

    QTest::newRow("Absolute file:/// against absolute FTP.")
        << QUrl::fromEncoded("file:///foo/")
        << QUrl::fromEncoded("ftp://example.com/")
        << QUrl::fromEncoded("ftp://example.com/");

    QTest::newRow("Absolute file:/// against absolute HTTP.")
        << QUrl::fromEncoded("file:///foo/")
        << QUrl::fromEncoded("http://example.com/")
        << QUrl::fromEncoded("http://example.com/");

    QTest::newRow("Absolute file:/// against data scheme.")
        << QUrl::fromEncoded("file:///foo/")
        << QUrl::fromEncoded("data:application/xml,%3Ce%2F%3E")
        << QUrl::fromEncoded("data:application/xml,%3Ce%2F%3E");

    QTest::newRow("Resolve with base url and port.")
        << QUrl::fromEncoded("http://www.foo.com:8080/")
        << QUrl::fromEncoded("newfile.html")
        << QUrl::fromEncoded("http://www.foo.com:8080/newfile.html");

    QTest::newRow("Resolve with relative path")
        << QUrl::fromEncoded("http://example.com/")
        << QUrl::fromEncoded("http://andreas:hemmelig@www.vg.no/a/../?my=query&your=query#yougotfragged")
        << QUrl::fromEncoded("http://andreas:hemmelig@www.vg.no/?my=query&your=query#yougotfragged");
}

void tst_QUrl::emptyAuthorityRemovesExistingAuthority()
{
    QUrl url("http://example.com/something");
    url.setAuthority(QString());
    QCOMPARE(url.authority(), QString());
}

void tst_QUrl::acceptEmptyAuthoritySegments()
{
    QCOMPARE(QUrl("remote://").toString(), QString::fromLatin1("remote://"));

    // Verify that foo:///bar is not mangled to foo:/bar
    QString foo_triple_bar("foo:///bar"), foo_uni_bar("foo:/bar");

    QCOMPARE(foo_triple_bar, QUrl(foo_triple_bar).toString());
    QCOMPARE(foo_triple_bar, QString::fromUtf8(QUrl(foo_triple_bar).toEncoded()));

    QCOMPARE(foo_uni_bar, QUrl(foo_uni_bar).toString());
    QCOMPARE(foo_uni_bar, QString::fromUtf8(QUrl(foo_uni_bar).toEncoded()));

    QCOMPARE(foo_triple_bar, QUrl(foo_triple_bar, QUrl::StrictMode).toString());
    QCOMPARE(foo_triple_bar, QString::fromUtf8(QUrl(foo_triple_bar, QUrl::StrictMode).toEncoded()));

    QCOMPARE(foo_uni_bar, QUrl(foo_uni_bar, QUrl::StrictMode).toString());
    QCOMPARE(foo_uni_bar, QString::fromUtf8(QUrl(foo_uni_bar, QUrl::StrictMode).toEncoded()));
}

void tst_QUrl::effectiveTLDs_data()
{
    QTest::addColumn<QUrl>("domain");
    QTest::addColumn<QString>("TLD");

    QTest::newRow("yes0") << QUrl::fromEncoded("http://test.co.uk") << ".co.uk";
    QTest::newRow("yes1") << QUrl::fromEncoded("http://test.com") << ".com";
    QTest::newRow("yes2") << QUrl::fromEncoded("http://www.test.de") << ".de";
    QTest::newRow("yes3") << QUrl::fromEncoded("http://test.ulm.museum") << ".ulm.museum";
    QTest::newRow("yes4") << QUrl::fromEncoded("http://www.com.krodsherad.no") << ".krodsherad.no";
    QTest::newRow("yes5") << QUrl::fromEncoded("http://www.co.uk.1.bg") << ".1.bg";
    QTest::newRow("yes6") << QUrl::fromEncoded("http://www.com.com.cn") << ".com.cn";
    QTest::newRow("yes7") << QUrl::fromEncoded("http://www.test.org.ws") << ".org.ws";
    QTest::newRow("yes9") << QUrl::fromEncoded("http://www.com.co.uk.wallonie.museum") << ".wallonie.museum";
    QTest::newRow("yes10") << QUrl::fromEncoded("http://www.com.evje-og-hornnes.no") << ".evje-og-hornnes.no";
    QTest::newRow("yes11") << QUrl::fromEncoded("http://www.bla.kamijima.ehime.jp") << ".kamijima.ehime.jp";
    QTest::newRow("yes12") << QUrl::fromEncoded("http://www.bla.kakuda.miyagi.jp") << ".kakuda.miyagi.jp";
    QTest::newRow("yes13") << QUrl::fromEncoded("http://mypage.betainabox.com") << ".betainabox.com";
    QTest::newRow("yes14") << QUrl::fromEncoded("http://mypage.rhcloud.com") << ".rhcloud.com";
    QTest::newRow("yes15") << QUrl::fromEncoded("http://mypage.int.az") << ".int.az";
}

void tst_QUrl::effectiveTLDs()
{
    QFETCH(QUrl, domain);
    QFETCH(QString, TLD);
    QCOMPARE(domain.topLevelDomain(), TLD);
}

void tst_QUrl::lowercasesScheme()
{
    QUrl url;
    url.setScheme("HELLO");
    QCOMPARE(url.scheme(), QString("hello"));
}

void tst_QUrl::componentEncodings_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<int>("encoding");
    QTest::addColumn<QString>("userName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("userInfo");
    QTest::addColumn<QString>("host");
    QTest::addColumn<QString>("authority");
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("query");
    QTest::addColumn<QString>("fragment");
    QTest::addColumn<QString>("toString");

    const int MostDecoded = QUrl::DecodeReserved; // the most decoded mode without being fully decoded

    QTest::newRow("empty") << QUrl() << int(QUrl::FullyEncoded)
                           << QString() << QString() << QString()
                           << QString() << QString()
                           << QString() << QString() << QString() << QString();

    // hostname cannot contain spaces
    QTest::newRow("encoded-space") << QUrl("x://user name:pass word@host/path name?query value#fragment value")
                                   << int(QUrl::EncodeSpaces)
                                   << "user%20name" << "pass%20word" << "user%20name:pass%20word"
                                   << "host" << "user%20name:pass%20word@host"
                                   << "/path%20name" << "query%20value" << "fragment%20value"
                                   << "x://user%20name:pass%20word@host/path%20name?query%20value#fragment%20value";

    QTest::newRow("decoded-space") << QUrl("x://user%20name:pass%20word@host/path%20name?query%20value#fragment%20value")
                                   << MostDecoded
                                   << "user name" << "pass word" << "user name:pass word"
                                   << "host" << "user name:pass word@host"
                                   << "/path name" << "query value" << "fragment value"
                                   << "x://user name:pass word@host/path name?query value#fragment value";

    // binary data is always encoded
    // this is also testing non-UTF8 data
    QTest::newRow("binary") << QUrl("x://%c0%00:%c1%01@host/%c2%02?%c3%03#%d4%04")
                            << MostDecoded
                            << "%C0%00" << "%C1%01" << "%C0%00:%C1%01"
                            << "host" << "%C0%00:%C1%01@host"
                            << "/%C2%02" << "%C3%03" << "%D4%04"
                            << "x://%C0%00:%C1%01@host/%C2%02?%C3%03#%D4%04";

    // unicode tests
    // hostnames can participate in this test, but we need a top-level domain that accepts Unicode
    QTest::newRow("encoded-unicode") << QUrl(QString::fromUtf8("x://\xc2\x80:\xc3\x90@smørbrød.example.no/\xe0\xa0\x80?\xf0\x90\x80\x80#é"))
                                     << int(QUrl::EncodeUnicode)
                                     << "%C2%80" << "%C3%90" << "%C2%80:%C3%90"
                                     << "xn--smrbrd-cyad.example.no" << "%C2%80:%C3%90@xn--smrbrd-cyad.example.no"
                                     << "/%E0%A0%80" << "%F0%90%80%80" << "%C3%A9"
                                     << "x://%C2%80:%C3%90@xn--smrbrd-cyad.example.no/%E0%A0%80?%F0%90%80%80#%C3%A9";
    QTest::newRow("decoded-unicode") << QUrl("x://%C2%80:%C3%90@XN--SMRBRD-cyad.example.NO/%E0%A0%80?%F0%90%80%80#%C3%A9")
                                     << MostDecoded
                                     << QString::fromUtf8("\xc2\x80") << QString::fromUtf8("\xc3\x90")
                                     << QString::fromUtf8("\xc2\x80:\xc3\x90")
                                     << QString::fromUtf8("smørbrød.example.no")
                                     << QString::fromUtf8("\xc2\x80:\xc3\x90@smørbrød.example.no")
                                     << QString::fromUtf8("/\xe0\xa0\x80")
                                     << QString::fromUtf8("\xf0\x90\x80\x80") << QString::fromUtf8("é")
                                     << QString::fromUtf8("x://\xc2\x80:\xc3\x90@smørbrød.example.no/\xe0\xa0\x80?\xf0\x90\x80\x80#é");

    //    unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
    // these are always decoded
    QTest::newRow("decoded-unreserved") << QUrl("x://%61:%71@%41%30%2eexample%2ecom/%7e?%5f#%51")
                                        << int(QUrl::FullyEncoded)
                                        << "a" << "q" << "a:q"
                                        << "a0.example.com" << "a:q@a0.example.com"
                                        << "/~" << "_" << "Q"
                                        << "x://a:q@a0.example.com/~?_#Q";

    //    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
    //                  / "*" / "+" / "," / ";" / "="
    // like the unreserved, these are decoded everywhere
    // don't test in query because they might remain encoded
    QTest::newRow("decoded-subdelims") << QUrl("x://%21%24%26:%27%28%29@host/%2a%2b%2c#%3b%3d")
                                       << int(QUrl::FullyEncoded)
                                       << "!$&" << "'()" << "!$&:'()"
                                       << "host" << "!$&:'()@host"
                                       << "/*+," << "" << ";="
                                       << "x://!$&:'()@host/*+,#;=";

    //    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
    // these are the separators between fields
    // they must appear encoded in certain positions, no exceptions
    // in other positions, they can appear decoded, so they always do
    // 1) test the delimiters that must appear encoded
    //    (if they were decoded, they'd would change the URL parsing)
    QTest::newRow("encoded-gendelims-changing") << QUrl("x://%5b%3a%2f%3f%23%40%5d:%5b%2f%3f%23%40%5d@host/%2f%3f%23?%23")
                                                << MostDecoded
                                                << "[:/?#@]" << "[/?#@]" << "[%3A/?#@]:[/?#@]"
                                                << "host" << "%5B%3A/?#%40%5D:%5B/?#%40%5D@host"
                                                << "/%2F?#" << "#" << ""
                                                << "x://%5B%3A%2F%3F%23%40%5D:%5B%2F%3F%23%40%5D@host/%2F%3F%23?%23";

    // 2) test the delimiters that may appear decoded and would not change the meaning
    // and test that %2f is *not* decoded to a slash in the path
    // don't test the query because in this mode it doesn't transform anything
    QTest::newRow("decoded-gendelims-unchanging") << QUrl("x://:%3a@host/%2f%3a%40#%23%3a%2f%3f%40")
                                                  << int(QUrl::FullyEncoded)
                                                  << "" << ":" << "::"
                                                  << "host" << "::@host"
                                                  << "/%2F:@" << "" << "#:/?@"
                                                  << "x://::@host/%2F:@##:/?@";

    // 3) test "[" and "]". Even though they are not ambiguous in the path, query or fragment
    // the RFC does not allow them to appear there decoded. QUrl adheres strictly in FullyEncoded mode
    QTest::newRow("encoded-square-brackets") << QUrl("x:/[]#[]")
                                             << int(QUrl::FullyEncoded)
                                             << "" << "" << ""
                                             << "" << ""
                                             << "/%5B%5D" << "" << "%5B%5D"
                                             << "x:/%5B%5D#%5B%5D";

    // 4) like above, but now decode them, which is allowed
    QTest::newRow("decoded-square-brackets") << QUrl("x:/%5B%5D#%5B%5D")
                                             << MostDecoded
                                             << "" << "" << ""
                                             << "" << ""
                                             << "/[]" << "" << "[]"
                                             << "x:/[]#[]";

    // test the query
    // since QUrl doesn't know what chars the user wants to use for the pair and value delimiters,
    // it keeps the delimiters alone except for "#", which must always be encoded.
    // In the following test, all delimiter characters appear both as encoded and as decoded (except for "#")
    QTest::newRow("unencoded-delims-query") << QUrl("?!$()*+,;=:/?[]@%21%24%26%27%28%29%2a%2b%2c%2f%3a%3b%3d%3f%40%5b%5d")
                                            << int(QUrl::FullyEncoded)
                                            << QString() << QString() << QString()
                                            << QString() << QString()
                                            << QString() << "!$()*+,;=:/?[]@%21%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D" << QString()
                                            << "?!$()*+,;=:/?[]@%21%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D";
    QTest::newRow("undecoded-delims-query") << QUrl("?!$()*+,;=:/?[]@%21%24%26%27%28%29%2a%2b%2c%2f%3a%3b%3d%3f%40%5b%5d")
                                            << MostDecoded
                                            << QString() << QString() << QString()
                                            << QString() << QString()
                                            << QString() << "!$()*+,;=:/?[]@%21%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D" << QString()
                                            << "?!$()*+,;=:/?[]@%21%24%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%40%5B%5D";

    // reserved characters:  '"' / "<" / ">" / "^" / "\" / "{" / "|" "}"
    // the RFC does not allow them undecoded anywhere, but we do
    QTest::newRow("encoded-reserved") << QUrl("x://\"<>^\\{|}:\"<>^\\{|}@host/\"<>^\\{|}?\"<>^\\{|}#\"<>^\\{|}")
                                    << int(QUrl::FullyEncoded)
                                    << "%22%3C%3E%5E%5C%7B%7C%7D" << "%22%3C%3E%5E%5C%7B%7C%7D"
                                    << "%22%3C%3E%5E%5C%7B%7C%7D:%22%3C%3E%5E%5C%7B%7C%7D"
                                    << "host" << "%22%3C%3E%5E%5C%7B%7C%7D:%22%3C%3E%5E%5C%7B%7C%7D@host"
                                    << "/%22%3C%3E%5E%5C%7B%7C%7D" << "%22%3C%3E%5E%5C%7B%7C%7D"
                                    << "%22%3C%3E%5E%5C%7B%7C%7D"
                                    << "x://%22%3C%3E%5E%5C%7B%7C%7D:%22%3C%3E%5E%5C%7B%7C%7D@host/%22%3C%3E%5E%5C%7B%7C%7D"
                                       "?%22%3C%3E%5E%5C%7B%7C%7D#%22%3C%3E%5E%5C%7B%7C%7D";
    QTest::newRow("decoded-reserved") << QUrl("x://%22%3C%3E%5E%5C%7B%7C%7D:%22%3C%3E%5E%5C%7B%7C%7D@host"
                                            "/%22%3C%3E%5E%5C%7B%7C%7D?%22%3C%3E%5E%5C%7B%7C%7D#%22%3C%3E%5E%5C%7B%7C%7D")
                                    << int(QUrl::DecodeReserved)
                                    << "\"<>^\\{|}" << "\"<>^\\{|}" << "\"<>^\\{|}:\"<>^\\{|}"
                                    << "host" << "\"<>^\\{|}:\"<>^\\{|}@host"
                                    << "/\"<>^\\{|}" << "\"<>^\\{|}" << "\"<>^\\{|}"
                                    << "x://\"<>^\\{|}:\"<>^\\{|}@host/\"<>^\\{|}?\"<>^\\{|}#\"<>^\\{|}";


    // Beauty is in the eye of the beholder
    // Test PrettyDecoder against our expectations

    // spaces and unicode are considered pretty and are decoded
    // this includes hostnames
    QTest::newRow("pretty-spaces-unicode") << QUrl("x://%20%c3%a9:%c3%a9%20@XN--SMRBRD-cyad.example.NO/%c3%a9%20?%20%c3%a9#%c3%a9%20")
                                           << int(QUrl::PrettyDecoded)
                                           << QString::fromUtf8(" é") << QString::fromUtf8("é ")
                                           << QString::fromUtf8(" é:é ")
                                           << QString::fromUtf8("smørbrød.example.no")
                                           << QString::fromUtf8(" é:é @smørbrød.example.no")
                                           << QString::fromUtf8("/é ") << QString::fromUtf8(" é")
                                           << QString::fromUtf8("é ")
                                           << QString::fromUtf8("x:// é:é @smørbrød.example.no/é ? é#é ");

    // the pretty form re-encodes the subdelims (except in the query, where they are left alone)
    QTest::newRow("pretty-subdelims") << QUrl("x://%21%24%26:%27%28%29@host/%2a%2b%2c?%26=%26&%3d=%3d#%3b%3d")
                                      << int(QUrl::PrettyDecoded)
                                      << "!$&" << "'()" << "!$&:'()"
                                      << "host" << "!$&:'()@host"
                                      << "/*+," << "%26=%26&%3D=%3D" << ";="
                                      << "x://!$&:'()@host/*+,?%26=%26&%3D=%3D#;=";

    // the pretty form decodes all unambiguous gen-delims
    // (except in query, where they are left alone)
    QTest::newRow("pretty-gendelims") << QUrl("x://%5b%3a%40%2f%5d:%5b%3a%40%2f%5d@host"
                                              "/%3a%40%5b%3f%23%5d?[?%3f%23]%5b:%3a@%40%5d#%23")
                                      << int(QUrl::PrettyDecoded)
                                      << "[:@/]" << "[:@/]" << "[%3A@/]:[:@/]"
                                      << "host" << "%5B%3A%40/%5D:%5B:%40/%5D@host"
                                      << "/:@[?#]" << "[?%3F#]%5B:%3A@%40%5D" << "#"
                                      << "x://%5B%3A%40%2F%5D:%5B:%40%2F%5D@host/:@[%3F%23]?[?%3F%23]%5B:%3A@%40%5D##";

    // the pretty form keeps the other characters decoded everywhere
    // except when rebuilding the full URL, when we only allow "{}" to remain decoded
    QTest::newRow("pretty-reserved") << QUrl("x://\"<>^\\{|}:\"<>^\\{|}@host/\"<>^\\{|}?\"<>^\\{|}#\"<>^\\{|}")
                                     << int(QUrl::PrettyDecoded)
                                     << "\"<>^\\{|}" << "\"<>^\\{|}" << "\"<>^\\{|}:\"<>^\\{|}"
                                     << "host" << "\"<>^\\{|}:\"<>^\\{|}@host"
                                     << "/\"<>^\\{|}" << "\"<>^\\{|}" << "\"<>^\\{|}"
                                     << "x://%22%3C%3E%5E%5C%7B%7C%7D:%22%3C%3E%5E%5C%7B%7C%7D@host/%22%3C%3E%5E%5C{%7C}"
                                        "?%22%3C%3E%5E%5C{%7C}#%22%3C%3E%5E%5C%7B%7C%7D";
}

void tst_QUrl::componentEncodings()
{
    QFETCH(QUrl, url);
    QFETCH(int, encoding);
    QFETCH(QString, userName);
    QFETCH(QString, password);
    QFETCH(QString, userInfo);
    QFETCH(QString, host);
    QFETCH(QString, authority);
    QFETCH(QString, path);
    QFETCH(QString, query);
    QFETCH(QString, fragment);
    QFETCH(QString, toString);

    QUrl::ComponentFormattingOptions formatting(encoding);
    QCOMPARE(url.userName(formatting), userName);
    QCOMPARE(url.password(formatting), password);
    QCOMPARE(url.userInfo(formatting), userInfo);
    QCOMPARE(url.host(formatting), host);
    QCOMPARE(url.authority(formatting), authority);
    QCOMPARE(url.path(formatting), path);
    QCOMPARE(url.query(formatting), query);
    QCOMPARE(url.fragment(formatting), fragment);
    QCOMPARE(url.toString(formatting),
             (((QString(toString ))))); // the weird () and space is to align the output

    if (formatting == QUrl::FullyEncoded) {
        QCOMPARE(url.encodedUserName(), userName.toUtf8());
        QCOMPARE(url.encodedPassword(), password.toUtf8());
        // no encodedUserInfo
        QCOMPARE(url.encodedHost(), host.toUtf8());
        // no encodedAuthority
        QCOMPARE(url.encodedPath(), path.toUtf8());
        QCOMPARE(url.encodedQuery(), query.toUtf8());
        QCOMPARE(url.encodedFragment(), fragment.toUtf8());
    }

    // repeat with the URL we got from toString
    QUrl url2(toString);
    QCOMPARE(url2.userName(formatting), userName);
    QCOMPARE(url2.password(formatting), password);
    QCOMPARE(url2.userInfo(formatting), userInfo);
    QCOMPARE(url2.host(formatting), host);
    QCOMPARE(url2.authority(formatting), authority);
    QCOMPARE(url2.path(formatting), path);
    QCOMPARE(url2.query(formatting), query);
    QCOMPARE(url2.fragment(formatting), fragment);
    QCOMPARE(url2.toString(formatting), toString);

    // and use the comparison operator
    QCOMPARE(url2, url);
}

enum Component {
    Scheme = 0x01,
    UserName = 0x02,
    Password = 0x04,
    UserInfo = UserName | Password,
    Host = 0x08,
    Port = 0x10,
    Authority = UserInfo | Host | Port,
    Path = 0x20,
    Hierarchy = Authority | Path,
    Query = 0x40,
    Fragment = 0x80,
    FullUrl = 0xff
};

void tst_QUrl::setComponents_data()
{
    QTest::addColumn<QUrl>("original");
    QTest::addColumn<int>("component");
    QTest::addColumn<QString>("newValue");
    QTest::addColumn<int>("parsingMode");
    QTest::addColumn<bool>("isValid");
    QTest::addColumn<int>("encoding");
    QTest::addColumn<QString>("output");
    QTest::addColumn<QString>("toString");

    const int Tolerant = QUrl::TolerantMode;
    const int Strict = QUrl::StrictMode;
    const int Decoded = QUrl::DecodedMode;
    const int PrettyDecoded = QUrl::PrettyDecoded;
    const int FullyDecoded = QUrl::FullyDecoded;

    // -- test empty vs null --
    // there's no empty-but-present scheme or path
    // a URL with an empty scheme is a "URI reference"
    // and the path is always non-empty if it's present
    QTest::newRow("scheme-null") << QUrl("http://example.com")
                                 << int(Scheme) << QString() << Tolerant << true
                                 << PrettyDecoded << QString() << "//example.com";
    QTest::newRow("scheme-empty") << QUrl("http://example.com")
                                  << int(Scheme) << "" << Tolerant << true
                                  << PrettyDecoded << "" << "//example.com";
    QTest::newRow("path-null") << QUrl("http://example.com/path")
                               << int(Path) << QString() << Tolerant << true
                               << PrettyDecoded << QString() << "http://example.com";
    QTest::newRow("path-empty") << QUrl("http://example.com/path")
                                << int(Path) << "" << Tolerant << true
                                << PrettyDecoded << "" << "http://example.com";

    // the other fields can be present and be empty
    // that is, their delimiters would be present, but there would be nothing to one side
    QTest::newRow("userinfo-null") << QUrl("http://user:pass@example.com")
                                   << int(UserInfo) << QString() << Tolerant << true
                                   << PrettyDecoded << QString() << "http://example.com";
    QTest::newRow("userinfo-empty") << QUrl("http://user:pass@example.com")
                                    << int(UserInfo) << "" << Tolerant << true
                                    << PrettyDecoded << "" << "http://@example.com";
    QTest::newRow("userinfo-colon") << QUrl("http://user@example.com")
                                    << int(UserInfo) << ":" << Tolerant << true
                                    << PrettyDecoded << ":" << "http://:@example.com";
    QTest::newRow("username-null") << QUrl("http://user@example.com")
                                   << int(UserName) << QString() << Tolerant << true
                                   << PrettyDecoded << QString() << "http://example.com";
    QTest::newRow("username-empty") << QUrl("http://user@example.com")
                                    << int(UserName) << "" << Tolerant << true
                                    << PrettyDecoded << "" << "http://@example.com";
    QTest::newRow("username-empty-password-nonempty") << QUrl("http://user:pass@example.com")
                                                      << int(UserName) << "" << Tolerant << true
                                                      << PrettyDecoded << "" << "http://:pass@example.com";
    QTest::newRow("username-empty-password-empty") << QUrl("http://user:@example.com")
                                                      << int(UserName) << "" << Tolerant << true
                                                      << PrettyDecoded << "" << "http://:@example.com";
    QTest::newRow("password-null") << QUrl("http://user:pass@example.com")
                                   << int(Password) << QString() << Tolerant << true
                                   << PrettyDecoded << QString() << "http://user@example.com";
    QTest::newRow("password-empty") << QUrl("http://user:pass@example.com")
                                    << int(Password) << "" << Tolerant << true
                                    << PrettyDecoded << "" << "http://user:@example.com";
    QTest::newRow("host-null") << QUrl("foo://example.com/path")
                               << int(Host) << QString() << Tolerant << true
                               << PrettyDecoded << QString() << "foo:/path";
    QTest::newRow("host-empty") << QUrl("foo://example.com/path")
                               << int(Host) << "" << Tolerant << true
                               << PrettyDecoded << QString() << "foo:///path";
    QTest::newRow("query-null") << QUrl("http://example.com/?q=foo")
                                   << int(Query) << QString() << Tolerant << true
                                   << PrettyDecoded << QString() << "http://example.com/";
    QTest::newRow("query-empty") << QUrl("http://example.com/?q=foo")
                                   << int(Query) << "" << Tolerant << true
                                   << PrettyDecoded << QString() << "http://example.com/?";
    QTest::newRow("fragment-null") << QUrl("http://example.com/#bar")
                                   << int(Fragment) << QString() << Tolerant << true
                                   << PrettyDecoded << QString() << "http://example.com/";
    QTest::newRow("fragment-empty") << QUrl("http://example.com/#bar")
                                   << int(Fragment) << "" << Tolerant << true
                                   << PrettyDecoded << "" << "http://example.com/#";

    // -- test some non-valid components --
    QTest::newRow("invalid-scheme-1") << QUrl("http://example.com")
                                      << int(Scheme) << "1http" << Tolerant << false
                                      << PrettyDecoded << "" << "";
    QTest::newRow("invalid-scheme-2") << QUrl("http://example.com")
                                      << int(Scheme) << "http%40" << Tolerant << false
                                      << PrettyDecoded << "" << "";
    QTest::newRow("invalid-scheme-3") << QUrl("http://example.com")
                                      << int(Scheme) << "http%61" << Strict << false
                                      << PrettyDecoded << "" << "";

    QTest::newRow("invalid-username-1") << QUrl("http://example.com")
                                        << int(UserName) << "{}" << Strict << false
                                        << PrettyDecoded << "" << "";
    QTest::newRow("invalid-username-2") << QUrl("http://example.com")
                                        << int(UserName) << "foo/bar" << Strict << false
                                        << PrettyDecoded << "" << "";
    QTest::newRow("invalid-username-3") << QUrl("http://example.com")
                                        << int(UserName) << "foo:bar" << Strict << false
                                        << PrettyDecoded << "" << "";
    QTest::newRow("invalid-password-1") << QUrl("http://example.com")
                                        << int(Password) << "{}" << Strict << false
                                        << PrettyDecoded << "" << "";
    QTest::newRow("invalid-password-2") << QUrl("http://example.com")
                                        << int(Password) << "foo/bar" << Strict << false
                                        << PrettyDecoded << "" << "";
    QTest::newRow("invalid-password-3") << QUrl("http://example.com")
                                        << int(Password) << "foo:bar" << Strict << false
                                        << PrettyDecoded << "" << "";
    QTest::newRow("invalid-userinfo-1") << QUrl("http://example.com")
                                        << int(UserInfo) << "{}" << Strict << false
                                        << PrettyDecoded << "" << "";
    QTest::newRow("invalid-userinfo-2") << QUrl("http://example.com")
                                        << int(UserInfo) << "foo/bar" << Strict << false
                                        << PrettyDecoded << "" << "";

    QTest::newRow("invalid-host-1") << QUrl("http://example.com")
                                    << int(Host) << "-not-valid-" << Tolerant << false
                                    << PrettyDecoded << "" << "";
    QTest::newRow("invalid-host-2") << QUrl("http://example.com")
                                    << int(Host) << "%31%30.%30.%30.%31" << Strict << false
                                    << PrettyDecoded << "" << "";
    QTest::newRow("invalid-authority-1") << QUrl("http://example.com")
                                         << int(Authority) << "-not-valid-" << Tolerant << false
                                         << PrettyDecoded << "" << "";
    QTest::newRow("invalid-authority-2") << QUrl("http://example.com")
                                         << int(Authority) << "%31%30.%30.%30.%31" << Strict << false
                                         << PrettyDecoded << "" << "";

    QTest::newRow("invalid-path-0") << QUrl("http://example.com")
                                    << int(Path) << "{}" << Strict << false
                                    << PrettyDecoded << "" << "";
    QTest::newRow("invalid-query-1") << QUrl("http://example.com")
                                     << int(Query) << "{}" << Strict << false
                                     << PrettyDecoded << "" << "";
    QTest::newRow("invalid-fragment-1") << QUrl("http://example.com")
                                        << int(Fragment) << "{}" << Strict << false
                                        << PrettyDecoded << "" << "";

    // these test cases are "compound invalid":
    // they produces isValid == false, but the original is still available
    QTest::newRow("invalid-path-1") << QUrl("/relative")
                                    << int(Path) << "c:/" << Strict << false
                                    << PrettyDecoded << "c:/" << "";
    QTest::newRow("invalid-path-2") << QUrl("http://example.com")
                                    << int(Path) << "relative" << Strict << false
                                    << PrettyDecoded << "relative" << "";

    // -- test bad percent encoding --
    // unnecessary to test the scheme, since percent-decoding is not performed in it;
    // see tests above
    QTest::newRow("bad-percent-username") << QUrl("http://example.com")
                                          << int(UserName) << "bar%foo" << Strict << false
                                          << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-password") << QUrl("http://user@example.com")
                                          << int(Password) << "bar%foo" << Strict << false
                                          << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-userinfo-1") << QUrl("http://example.com")
                                            << int(UserInfo) << "bar%foo" << Strict << false
                                            << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-userinfo-2") << QUrl("http://example.com")
                                            << int(UserInfo) << "bar%:foo" << Strict << false
                                            << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-userinfo-3") << QUrl("http://example.com")
                                            << int(UserInfo) << "bar:%foo" << Strict << false
                                            << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-authority-1") << QUrl("http://example.com")
                                             << int(Authority) << "bar%foo@example.org" << Strict << false
                                             << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-authority-2") << QUrl("http://example.com")
                                             << int(Authority) << "bar%:foo@example.org" << Strict << false
                                             << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-authority-3") << QUrl("http://example.com")
                                             << int(Authority) << "bar:%foo@example.org" << Strict << false
                                             << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-authority-4") << QUrl("http://example.com")
                                             << int(Authority) << "bar:foo@bar%foo" << Strict << false
                                             << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-host") << QUrl("http://example.com")
                                      << int(Host) << "bar%foo" << Strict << false
                                      << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-path") << QUrl("http://example.com")
                                      << int(Path) << "/bar%foo" << Strict << false
                                      << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-query") << QUrl("http://example.com")
                                       << int(Query) << "bar%foo" << Strict << false
                                       << PrettyDecoded << "" << "";
    QTest::newRow("bad-percent-fragment") << QUrl("http://example.com")
                                          << int(Fragment) << "bar%foo" << Strict << false
                                          << PrettyDecoded << "" << "";

    // -- test decoded behaviour --
    // '%' characters are not permitted in the scheme, this tests that it fails to set anything
    QTest::newRow("invalid-scheme-encode") << QUrl("http://example.com")
                                           << int(Scheme) << "http%61" << Decoded << false
                                           << PrettyDecoded << "" << "";
    QTest::newRow("userinfo-encode") << QUrl("http://example.com")
                                     << int(UserInfo) << "h%61llo:world@" << Decoded << true
                                     << PrettyDecoded << "h%2561llo:world@" << "http://h%2561llo:world%40@example.com";
    QTest::newRow("username-encode") << QUrl("http://example.com")
                                     << int(UserName) << "h%61llo:world" << Decoded << true
                                     << PrettyDecoded << "h%2561llo:world" << "http://h%2561llo%3Aworld@example.com";
    QTest::newRow("password-encode") << QUrl("http://example.com")
                                     << int(Password) << "h%61llo:world@" << Decoded << true
                                     << PrettyDecoded << "h%2561llo:world@" << "http://:h%2561llo:world%40@example.com";
    // '%' characters are not permitted in the hostname, these test that it fails to set anything
    QTest::newRow("invalid-host-encode") << QUrl("http://example.com")
                                         << int(Host) << "ex%61mple.com" << Decoded << false
                                         << PrettyDecoded << "" << "";
    QTest::newRow("invalid-authority-encode") << QUrl("http://example.com")
                                              << int(Authority) << "ex%61mple.com" << Decoded << false
                                              << PrettyDecoded << "" << "";
    QTest::newRow("path-encode") << QUrl("http://example.com/foo")
                                 << int(Path) << "/bar%23" << Decoded << true
                                 << PrettyDecoded << "/bar%2523" << "http://example.com/bar%2523";
    QTest::newRow("query-encode") << QUrl("http://example.com/foo?q")
                                  << int(Query) << "bar%23" << Decoded << true
                                  << PrettyDecoded << "bar%2523" << "http://example.com/foo?bar%2523";
    QTest::newRow("fragment-encode") << QUrl("http://example.com/foo#z")
                                     << int(Fragment) << "bar%23" << Decoded << true
                                     << PrettyDecoded << "bar%2523" << "http://example.com/foo#bar%2523";
    // force decoding; note how the userinfo becomes ambiguous
    QTest::newRow("userinfo-decode") << QUrl("http://example.com")
                                     << int(UserInfo) << "hello%3Aworld:}}>b9o%25kR(" << Tolerant << true
                                     << FullyDecoded << "hello:world:}}>b9o%kR("
                                     << "http://hello%3Aworld:%7D%7D%3Eb9o%25kR(@example.com";
    QTest::newRow("username-decode") << QUrl("http://example.com")
                                     << int(UserName) << "hello%3Aworld%25" << Tolerant << true
                                     << FullyDecoded << "hello:world%" << "http://hello%3Aworld%25@example.com";
    QTest::newRow("password-decode") << QUrl("http://example.com")
                                     << int(Password) << "}}>b9o%25kR(" << Tolerant << true
                                     << FullyDecoded << "}}>b9o%kR(" << "http://:%7D%7D%3Eb9o%25kR(@example.com";
    QTest::newRow("path-decode") << QUrl("http://example.com/")
                                 << int(Path) << "/bar%25foo" << Tolerant << true
                                 << FullyDecoded << "/bar%foo" << "http://example.com/bar%25foo";
    QTest::newRow("query-decode") << QUrl("http://example.com/foo?qq")
                                  << int(Query) << "bar%25foo" << Tolerant << true
                                  << FullyDecoded << "bar%foo" << "http://example.com/foo?bar%25foo";
    QTest::newRow("fragment-decode") << QUrl("http://example.com/foo#qq")
                                     << int(Fragment) << "bar%25foo" << Tolerant << true
                                     << FullyDecoded << "bar%foo" << "http://example.com/foo#bar%25foo";
}

void tst_QUrl::setComponents()
{
    QFETCH(QUrl, original);
    QUrl copy(original);

    QFETCH(int, component);
    QFETCH(int, parsingMode);
    QFETCH(QString, newValue);
    QFETCH(int, encoding);
    QFETCH(QString, output);

    switch (component) {
    case Scheme:
        // scheme is only parsed in strict mode
        copy.setScheme(newValue);
        QCOMPARE(copy.scheme(), output);
        break;

    case Path:
        copy.setPath(newValue, QUrl::ParsingMode(parsingMode));
        QCOMPARE(copy.path(QUrl::ComponentFormattingOptions(encoding)), output);
        break;

    case UserInfo:
        copy.setUserInfo(newValue, QUrl::ParsingMode(parsingMode));
        QCOMPARE(copy.userInfo(QUrl::ComponentFormattingOptions(encoding)), output);
        break;

    case UserName:
        copy.setUserName(newValue, QUrl::ParsingMode(parsingMode));
        QCOMPARE(copy.userName(QUrl::ComponentFormattingOptions(encoding)), output);
        break;

    case Password:
        copy.setPassword(newValue, QUrl::ParsingMode(parsingMode));
        QCOMPARE(copy.password(QUrl::ComponentFormattingOptions(encoding)), output);
        break;

    case Host:
        copy.setHost(newValue, QUrl::ParsingMode(parsingMode));
        QCOMPARE(copy.host(QUrl::ComponentFormattingOptions(encoding)), output);
        break;

    case Authority:
        copy.setAuthority(newValue, QUrl::ParsingMode(parsingMode));
        QCOMPARE(copy.authority(QUrl::ComponentFormattingOptions(encoding)), output);
        break;

    case Query:
        copy.setQuery(newValue, QUrl::ParsingMode(parsingMode));
        QCOMPARE(copy.hasQuery(), !newValue.isNull());
        QCOMPARE(copy.query(QUrl::ComponentFormattingOptions(encoding)), output);
        break;

    case Fragment:
        copy.setFragment(newValue, QUrl::ParsingMode(parsingMode));
        QCOMPARE(copy.hasFragment(), !newValue.isNull());
        QCOMPARE(copy.fragment(QUrl::ComponentFormattingOptions(encoding)), output);
        break;
    }

    QFETCH(bool, isValid);
    QCOMPARE(copy.isValid(), isValid);

    if (isValid) {
        QFETCH(QString, toString);
        QCOMPARE(copy.toString(), toString);
    } else {
        QVERIFY(copy.toString().isEmpty());
    }
}

void tst_QUrl::streaming_data()
{
    QTest::addColumn<QString>("urlStr");

    QTest::newRow("origURL") << "http://www.website.com/directory/?#ref";
    QTest::newRow("urlWithPassAndNoUser") << "ftp://:password@ftp.kde.org/path";
    QTest::newRow("accentuated") << QString::fromUtf8("trash:/été");
    QTest::newRow("withPercents") << "http://host/path%25path?%3Fque%25ry#%23ref%25";
    QTest::newRow("empty") << "";
    QVERIFY(!QUrl("ptal://mlc:usb").isValid());
    QTest::newRow("invalid") << "ptal://mlc:usb";
    QTest::newRow("ipv6") << "http://[::ffff:129.144.52.38]:81?query";
}

void tst_QUrl::streaming()
{
    QFETCH(QString, urlStr);
    QUrl url(urlStr);

    QByteArray buffer;
    QDataStream writeStream( &buffer, QIODevice::WriteOnly );
    writeStream << url;

    QDataStream stream( buffer );
    QUrl restored;
    stream >> restored;
    if (url.isValid())
        QCOMPARE(restored.url(), url.url());
    else
        QVERIFY(!restored.isValid());
}

void tst_QUrl::detach()
{
    QUrl empty;
    empty.detach();

    QUrl foo("http://www.kde.org");
    QUrl foo2 = foo;
    foo2.detach(); // not that it's needed, given that setHost detaches, of course. But this increases coverage :)
    foo2.setHost("www.gnome.org");
    QCOMPARE(foo2.host(), QString("www.gnome.org"));
    QCOMPARE(foo.host(), QString("www.kde.org"));
}

// Test accessing the same QUrl from multiple threads concurrently
// To maximize the chances of a race (and of a report from helgrind), we actually use
// 10 urls rather than one.
class UrlStorage
{
public:
    UrlStorage() {
        m_urls.resize(10);
        for (int i = 0 ; i < m_urls.size(); ++i)
            m_urls[i] = QUrl::fromEncoded("http://www.kde.org", QUrl::StrictMode);
    }
    QVector<QUrl> m_urls;
};

static const UrlStorage * s_urlStorage = 0;

void tst_QUrl::testThreadingHelper()
{
    const UrlStorage* storage = s_urlStorage;
    for (int i = 0 ; i < storage->m_urls.size(); ++i ) {
        const QUrl& u = storage->m_urls.at(i);
        // QVERIFY/QCOMPARE trigger race conditions in helgrind
        if (!u.isValid())
            qFatal("invalid url");
        if (u.scheme() != QLatin1String("http"))
            qFatal("invalid scheme");
        if (!u.toString().startsWith('h'))
            qFatal("invalid toString");
        QUrl copy(u);
        copy.setHost("www.new-host.com");
        QUrl copy2(u);
        copy2.setUserName("dfaure");
        QUrl copy3(u);
        copy3.setUrl("http://www.new-host.com");
        QUrl copy4(u);
        copy4.detach();
        QUrl copy5(u);
        QUrl resolved1 = u.resolved(QUrl("index.html"));
        Q_UNUSED(resolved1);
        QUrl resolved2 = QUrl("http://www.kde.org").resolved(u);
        Q_UNUSED(resolved2);
        QString local = u.toLocalFile();
        Q_UNUSED(local);
        QTest::qWait(10); // give time for the other threads to start
    }
}

#include <QThreadPool>
#include <QtConcurrent>

void tst_QUrl::testThreading()
{
    s_urlStorage = new UrlStorage;
    QThreadPool::globalInstance()->setMaxThreadCount(100);
    QFutureSynchronizer<void> sync;
    for (int i = 0; i < 100; ++i)
        sync.addFuture(QtConcurrent::run(this, &tst_QUrl::testThreadingHelper));
    sync.waitForFinished();
    delete s_urlStorage;
}

QTEST_MAIN(tst_QUrl)

#include "tst_qurl.moc"
