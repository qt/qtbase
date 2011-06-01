/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <QtCore/QDebug>

#include <qcoreapplication.h>

#include <qfileinfo.h>
#include <qurl.h>
#include <qtextcodec.h>
#include <qmap.h>
#include "private/qtldurl_p.h"

// For testsuites
#define IDNA_ACE_PREFIX "xn--"
#define IDNA_SUCCESS 1
#define STRINGPREP_NO_UNASSIGNED 1
#define STRINGPREP_CONTAINS_UNASSIGNED 2
#define STRINGPREP_CONTAINS_PROHIBITED 3
#define STRINGPREP_BIDI_BOTH_L_AND_RAL 4
#define STRINGPREP_BIDI_LEADTRAIL_NOT_RAL 5

struct ushortarray {
    ushortarray(unsigned short *array = 0)
    {
        if (array)
            memcpy(points, array, sizeof(points));
    }

    unsigned short points[100];
};

Q_DECLARE_METATYPE(ushortarray)
Q_DECLARE_METATYPE(QUrl::FormattingOptions)

//TESTED_CLASS=
//TESTED_FILES=

class tst_QUrl : public QObject
{
    Q_OBJECT

public:
    tst_QUrl();
    virtual ~tst_QUrl();


public slots:
    void init();
    void cleanup();
private slots:
    void effectiveTLDs_data();
    void effectiveTLDs();
    void getSetCheck();
    void constructing();
    void assignment();
    void comparison();
    void copying();
    void setUrl();
    void i18n_data();
    void i18n();
    void punycode_data();
    void punycode();
    void resolving_data();
    void resolving();
    void toString_data();
    void toString();
    void toString_constructed_data();
    void toString_constructed();
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
    void queryItems();
    void hasQuery_data();
    void hasQuery();
    void hasQueryItem_data();
    void hasQueryItem();
    void nameprep();
    void isValid();
    void schemeValidator_data();
    void schemeValidator();
    void invalidSchemeValidator();
    void tolerantParser();
    void correctEncodedMistakes_data();
    void correctEncodedMistakes();
    void correctDecodedMistakes_data();
    void correctDecodedMistakes();
    void idna_testsuite_data();
    void idna_testsuite();
    void nameprep_testsuite_data();
    void nameprep_testsuite();
    void nameprep_highcodes_data();
    void nameprep_highcodes();
    void ace_testsuite_data();
    void ace_testsuite();
    void std3violations_data();
    void std3violations();
    void std3deviations_data();
    void std3deviations();
    void tldRestrictions_data();
    void tldRestrictions();
    void emptyQueryOrFragment();
    void hasFragment_data();
    void hasFragment();
    void setEncodedFragment_data();
    void setEncodedFragment();
    void fromEncoded();
    void stripTrailingSlash();
    void hosts_data();
    void hosts();
    void setPort();
    void toEncoded_data();
    void toEncoded();
    void setAuthority_data();
    void setAuthority();
    void errorString();
    void clear();
    void resolvedWithAbsoluteSchemes() const;
    void resolvedWithAbsoluteSchemes_data() const;
    void binaryData_data();
    void binaryData();
    void fromUserInput_data();
    void fromUserInput();
    void task_199967();
    void task_240612();
    void taskQTBUG_6962();
    void taskQTBUG_8701();
};

// Testing get/set functions
void tst_QUrl::getSetCheck()
{
    QUrl obj1;
    // int QUrl::port()
    // void QUrl::setPort(int)
    obj1.setPort(0);
    QCOMPARE(0, obj1.port());

    QTest::ignoreMessage(QtWarningMsg, "QUrl::setPort: Out of range");
    obj1.setPort(INT_MIN);
    QCOMPARE(-1, obj1.port()); // Out of range, -1

    QTest::ignoreMessage(QtWarningMsg, "QUrl::setPort: Out of range");
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

tst_QUrl::tst_QUrl()
{
}

tst_QUrl::~tst_QUrl()
{

}

void tst_QUrl::init()
{
}

void tst_QUrl::cleanup()
{
}

void tst_QUrl::constructing()
{
    QUrl url;
    QVERIFY(!url.isValid());
    QVERIFY(url.isEmpty());
    QCOMPARE(url.port(), -1);
    QCOMPARE(url.toString(), QString());

    QList<QPair<QString, QString> > query;
    query += qMakePair(QString("type"), QString("login"));
    query += qMakePair(QString("name"), QString("åge nissemannsen"));
    query += qMakePair(QString("ole&du"), QString("anne+jørgen=sant"));
    query += qMakePair(QString("prosent"), QString("%"));
    url.setQueryItems(query);
    QVERIFY(!url.isEmpty());

    QCOMPARE(url.encodedQuery().constData(),
            QByteArray("type=login&name=%C3%A5ge%20nissemannsen&ole%26du="
                       "anne+j%C3%B8rgen%3Dsant&prosent=%25").constData());

    url.setQueryDelimiters('>', '/');
    url.setQueryItems(query);

    QCOMPARE(url.encodedQuery(),
            QByteArray("type>login/name>%C3%A5ge%20nissemannsen/ole&du>"
                       "anne+j%C3%B8rgen=sant/prosent>%25"));

    url.setFragment(QString::fromLatin1("top"));
    QCOMPARE(url.fragment(), QString::fromLatin1("top"));

    url.setScheme("http");
    url.setHost("qt.nokia.com");

    QCOMPARE(url.toString(),
            QString::fromLatin1("http://qt.nokia.com?type>login/name>åge nissemannsen"
                          "/ole&du>anne+jørgen=sant/prosent>%#top"));

    QUrl justHost("qt.nokia.com");
    QVERIFY(!justHost.isEmpty());
    QVERIFY(justHost.host().isEmpty());
    QCOMPARE(justHost.path(), QString::fromLatin1("qt.nokia.com"));

    QUrl hostWithSlashes("//qt.nokia.com");
    QVERIFY(hostWithSlashes.path().isEmpty());
    QCOMPARE(hostWithSlashes.host(), QString::fromLatin1("qt.nokia.com"));


    QUrl withHashInPath;
    withHashInPath.setPath(QString::fromLatin1("hi#mum.txt"));
    QCOMPARE(withHashInPath.path(), QString::fromLatin1("hi#mum.txt"));
    QCOMPARE(withHashInPath.toEncoded(), QByteArray("hi%23mum.txt"));
    QUrl fromHashInPath = QUrl::fromEncoded(withHashInPath.toEncoded());
    QVERIFY(withHashInPath == fromHashInPath);


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
    QUrl url("http://qt.nokia.com/");
    QVERIFY(url.isValid());

    QUrl copy;
    copy = url;

    QVERIFY(url == copy);
}

void tst_QUrl::comparison()
{
    QUrl url1("http://qt.nokia.com/");
    QVERIFY(url1.isValid());

    QUrl url2("http://qt.nokia.com/");
    QVERIFY(url2.isValid());

    QVERIFY(url1 == url2);

    // 6.2.2 Syntax-based Normalization
    QUrl url3 = QUrl::fromEncoded("example://a/b/c/%7Bfoo%7D");
    QUrl url4 = QUrl::fromEncoded("eXAMPLE://a/./b/../b/%63/%7bfoo%7d");
    QVERIFY(url3 == url4);

    // 6.2.2.1 Make sure hexdecimal characters in percent encoding are
    // treated case-insensitively
    QUrl url5;
    url5.setEncodedQuery("a=%2a");
    QUrl url6;
    url6.setEncodedQuery("a=%2A");
    QVERIFY(url5 == url6);

    // ensure that encoded characters in the query do not match
    QUrl url7;
    url7.setEncodedQuery("a=%63");
    QUrl url8;
    url8.setEncodedQuery("a=c");
    QVERIFY(url7 != url8);
}

void tst_QUrl::copying()
{
    QUrl url("http://qt.nokia.com/");
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
    }

    {
        QUrl url("hTTp://www.foo.bar:80");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString::fromLatin1("hTTp"));
        QCOMPARE(url.path(), QString());
        QVERIFY(url.encodedQuery().isEmpty());
        QVERIFY(url.userInfo().isEmpty());
        QVERIFY(url.fragment().isEmpty());
        QCOMPARE(url.host(), QString::fromLatin1("www.foo.bar"));
        QCOMPARE(url.authority(), QString::fromLatin1("www.foo.bar:80"));
        QCOMPARE(url.port(), 80);

        QUrl url2("//www1.foo.bar");
        QCOMPARE(url.resolved(url2).toString(), QString::fromLatin1("hTTp://www1.foo.bar"));
    }

    {
        QUrl url("http://user:pass@[56::56:56:56:127.0.0.1]:99");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString::fromLatin1("http"));
        QCOMPARE(url.path(), QString());
        QVERIFY(url.encodedQuery().isEmpty());
        QCOMPARE(url.userInfo(), QString::fromLatin1("user:pass"));
        QVERIFY(url.fragment().isEmpty());
        QCOMPARE(url.host(), QString::fromLatin1("56::56:56:56:127.0.0.1"));
        QCOMPARE(url.authority(), QString::fromLatin1("user:pass@[56::56:56:56:127.0.0.1]:99"));
        QCOMPARE(url.port(), 99);
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
        QCOMPARE(url.fragment(), QString::fromLatin1("myref"));
    }

    {
        QString u1 = "file:/home/dfaure/my#myref";
        QUrl url = u1;
        QVERIFY(url.isValid());

        QCOMPARE(url.toString(), QString::fromLatin1("file:///home/dfaure/my#myref"));
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
        QUrl notPretty;
        notPretty.setEncodedUrl("http://ferret.lmh.ox.ac.uk/%7Ekdecvs/");
        QVERIFY(notPretty.isValid());
        QCOMPARE(notPretty.toString(), QString::fromLatin1("http://ferret.lmh.ox.ac.uk/~kdecvs/"));

        QUrl notPretty2;
        notPretty2.setEncodedUrl("file:/home/test/directory%20with%20spaces");
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
        QCOMPARE(url15582.toString(), QString::fromLatin1("http://alain.knaff.linux.lu/bug-reports/kde/percentage%in%url.html"));
        QCOMPARE(url15582.toEncoded(), QByteArray("http://alain.knaff.linux.lu/bug-reports/kde/percentage%25in%25url.html"));
    }

    {
        QUrl carsten;
        carsten.setPath("/home/gis/src/kde/kdelibs/kfile/.#kfiledetailview.cpp.1.18");
        QCOMPARE(carsten.path(), QString::fromLatin1("/home/gis/src/kde/kdelibs/kfile/.#kfiledetailview.cpp.1.18"));

        QUrl charles;
        charles.setPath("/home/charles/foo%20moo");
        QCOMPARE(charles.path(), QString::fromLatin1("/home/charles/foo%20moo"));

        QUrl charles2;
        charles2.setEncodedUrl("file:/home/charles/foo%20moo");
        QCOMPARE(charles2.path(), QString::fromLatin1("/home/charles/foo moo"));
    }

    {
        QUrl udir;
        QCOMPARE(udir.toEncoded(), QByteArray());
        QVERIFY(!udir.isValid());

        udir = QUrl::fromLocalFile("/home/dfaure/file.txt");
        QCOMPARE(udir.path(), QString::fromLatin1("/home/dfaure/file.txt"));
        QCOMPARE(udir.toEncoded(), QByteArray("file:///home/dfaure/file.txt"));
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
        QUrl url;
        url.setEncodedUrl("data:text/javascript,d5%20%3D%20'five\\u0027s'%3B");
        QVERIFY(url.isValid());
        QCOMPARE(url.scheme(), QString("data"));
        QCOMPARE(url.host(), QString());
        QCOMPARE(url.path(), QString("text/javascript,d5 = 'five\\u0027s';"));
        QCOMPARE(url.encodedPath().constData(), "text/javascript,d5%20%3D%20'five%5Cu0027s'%3B");
    }

    { //check it calls detach
        QUrl u1("http://aaa.com");
        QUrl u2 = u1;
        u2.setUrl("http://bbb.com");
        QCOMPARE(u1.host(), QString::fromLatin1("aaa.com"));
        QCOMPARE(u2.host(), QString::fromLatin1("bbb.com"));
    }

/*
   The tests below are copied from kdelibs/kdecore/tests/kurltest.cpp (an old version of)

    Copyright (c) 1999-2005 Waldo Bastian <bastian@kde.org>
    Copyright (c) 2000-2005 David Faure <faure@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 */
/*
    ### File / directory specifics

    KURL u2( QCString("/home/dfaure/") );
    printf("\n* URL is %s\n",u2.url().ascii());
// not ignoring trailing slash
    check("KURL::directory(false,false)", u2.directory(false,false), "/home/dfaure/");
    check("KURL::directory(true,false)", u2.directory(true,false), "/home/dfaure");
// ignoring trailing slash
    check("KURL::directory(false,true)", u2.directory(false,true), "/home/");
    check("KURL::directory(true,true)", u2.directory(true,true), "/home");
    u2.cd("..");
    check("KURL::cd(\"..\")", u2.url(), "file:/home");
    u2.cd("thomas");
    check("KURL::cd(\"thomas\")", u2.url(), "file:/home/thomas");
    u2.cd("/opt/kde/bin/");
    check("KURL::cd(\"/opt/kde/bin/\")", u2.url(), "file:/opt/kde/bin/");
    u2 = "ftp://ftp.kde.org/";
    printf("\n* URL is %s\n",u2.url().ascii());
    u2.cd("pub");
    check("KURL::cd(\"pub\")", u2.url(), "ftp://ftp.kde.org/pub");
    u2 = u2.upURL();
    check("KURL::upURL()", u2.url(), "ftp://ftp.kde.org/");
    u2 = u1;
    printf("\n* URL is %s\n",u2.url().ascii());
// setFileName
    u2.setFileName( "myfile.txt" );
    check("KURL::setFileName()", u2.url(), "file:/home/dfaure/myfile.txt");
    u2.setFileName( "myotherfile.txt" );
    check("KURL::setFileName()", u2.url(), "file:/home/dfaure/myotherfile.txt");
// more tricky, renaming a directory (kpropsdlg.cc, line ~ 238)
    QString tmpurl = "file:/home/dfaure/myolddir/";
    if ( tmpurl.at(tmpurl.length() - 1) == '/')
// It's a directory, so strip the trailing slash first
        tmpurl.truncate( tmpurl.length() - 1);
    KURL newUrl = tmpurl;
    newUrl.setFileName( "mynewdir" );
    check("KURL::setFileName() special", newUrl.url(), "file:/home/dfaure/mynewdir");
// addPath tests
    newUrl.addPath( "subdir" );
    check("KURL::addPath(\"subdir\")", newUrl.url(), "file:/home/dfaure/mynewdir/subdir");
    newUrl.addPath( "/foo/" );
    check("KURL::addPath(\"/foo/\")", newUrl.url(), "file:/home/dfaure/mynewdir/subdir/foo/");
    u2 = "http://www.kde.org"; // no path
    u2.addPath( "subdir" );
    check("KURL::addPath(\"subdir\")", u2.url(), "http://www.kde.org/subdir");
    u2.addPath( "" );
    check("KURL::addPath(\"subdir\")", u2.url(), "http://www.kde.org/subdir"); // unchanged

// even more tricky
    u2 = "print:/specials/Print%20To%20File%20(PDF%2FAcrobat)";
    printf("\n* URL is %s\n",u2.url().ascii());
    check("KURL::path()", u2.path(), "/specials/Print To File (PDF/Acrobat)");
    check("KURL::fileName()", u2.fileName(), "Print To File (PDF/Acrobat)");
    u2.setFileName( "" );
    check("KURL::setFileName()", u2.url(), "print:/specials/");

    u2 = "file:/specials/Print";
    printf("\n* URL is %s\n",u2.url().ascii());
    check("KURL::path()", u2.path(), "/specials/Print");
    check("KURL::fileName()", u2.fileName(), "Print");
    u2.setFileName( "" );
    check("KURL::setFileName()", u2.url(), "file:/specials/");

    const char * u3 = "ftp://host/dir1/dir2/myfile.txt";
    printf("\n* URL is %s\n",u3);
    check("KURL::hasSubURL()", KURL(u3).hasSubURL() ? "yes" : "no", "no");
    lst.clear();
    lst = KURL::split( KURL(u3) );
    check("KURL::split()", lst.count()==1 ? "1" : "error", "1");
    check("KURL::split()", lst.first().url(), "ftp://host/dir1/dir2/myfile.txt");
// cdUp code
    KURL lastUrl = lst.last();
    QString dir = lastUrl.directory( true, true );
    check( "KURL::directory(true,true)", dir, "/dir1/dir2");
*/

    {
        // !!! FAILS
//        QString ucmp1 = "ftp://ftp.de.kde.org/dir";
//        QString ucmp2 = "ftp://ftp.de.kde.org/dir/";
//        QVERIFY(QUrl(ucmp1) == QUrl(ucmp2));
    }


    /*
/// Comparisons
    QString ucmp1 = "ftp://ftp.de.kde.org/dir";
    QString ucmp2 = "ftp://ftp.de.kde.org/dir/";
    check("urlcmp(only slash difference)", urlcmp(ucmp1,ucmp2)?"ko":"ok","ok");
    check("urlcmp(only slash difference, ignore_trailing)", urlcmp(ucmp1,ucmp2,true,false)?"ok":"ko","ok");
    QString ucmp3 = "ftp://ftp.de.kde.org/dir/#";
    check("urlcmp(only hash difference)", urlcmp(ucmp2,ucmp3)?"ko":"ok","ok");
    check("urlcmp(only hash difference, ignore_ref)", urlcmp(ucmp2,ucmp3,false,true)?"ok":"ko","ok");
    check("urlcmp(slash and hash difference, ignore_trailing, ignore_ref)", urlcmp(ucmp2,ucmp3,true,true)?"ok":"ko","ok");
    check("urlcmp(empty, empty)", urlcmp("","",false,true)?"ok":"ko","ok");
    check("urlcmp(empty, empty)", urlcmp("","")?"ok":"ko","ok");
    check("urlcmp(empty, not empty)", urlcmp("",ucmp1)?"ok":"ko","ko");
    check("urlcmp(empty, not empty)", urlcmp("",ucmp1,false,true)?"ok":"ko","ko");
    check("urlcmp(malformed, not empty)", urlcmp("file",ucmp1)?"ok":"ko","ko");
    check("urlcmp(malformed, not empty)", urlcmp("file",ucmp1,false,true)?"ok":"ko","ko");

    KURL ftpUrl ( "ftp://ftp.de.kde.org" );
    printf("\n* URL is %s\n",ftpUrl.url().latin1());
    check("KURL::path()", ftpUrl.path(), QString());
    ftpUrl = "ftp://ftp.de.kde.org/";
    check("KURL::isParentOf()", ftpUrl.isParentOf( "ftp://ftp.de.kde.org/host/subdir/") ? "yes" : "no", "yes");
    ftpUrl = "ftp://ftp/host/subdir/";
    check("KURL::isParentOf()", ftpUrl.isParentOf( "ftp://ftp/host/subdir/") ? "yes" : "no", "yes");
    check("KURL::isParentOf()", ftpUrl.isParentOf( "ftp://ftp/host/subdir") ? "yes" : "no", "yes");
    check("KURL::isParentOf()", ftpUrl.isParentOf( "ftp://ftp/host/subdi") ? "yes" : "no", "no");
    check("KURL::isParentOf()", ftpUrl.isParentOf( "ftp://ftp/host/subdir/blah/") ? "yes" : "no", "yes");
    check("KURL::isParentOf()", ftpUrl.isParentOf( "ftp://ftp/blah/subdir") ? "yes" : "no", "no");
    check("KURL::isParentOf()", ftpUrl.isParentOf( "file://ftp/host/subdir/") ? "yes" : "no", "no");
    check("KURL::isParentOf()", ftpUrl.isParentOf( "ftp://ftp/host/subdir/subsub") ? "yes" : "no", "yes");

// WABA: The following tests are to test the handling of relative URLs as
//       found on web-pages.

    KURL waba1( "http://www.website.com/directory/?hello#ref" );
    {
        KURL waba2( waba1, "relative.html");
        check("http: Relative URL, single file", waba2.url(), "http://www.website.com/directory/relative.html");
    }
    {
        KURL waba2( waba1, "../relative.html");
        check("http: Relative URL, single file, directory up", waba2.url(), "http://www.website.com/relative.html");
    }
    {
        KURL waba2( waba1, "down/relative.html");
        check("http: Relative URL, single file, directory down", waba2.url(), "http://www.website.com/directory/down/relative.html");
    }
    {
        KURL waba2( waba1, "/down/relative.html");
        check("http: Relative URL, full path", waba2.url(), "http://www.website.com/down/relative.html");
    }
    {
        KURL waba2( waba1, "//www.kde.org/relative.html");
        check("http: Relative URL, with host", waba2.url(), "http://www.kde.org/relative.html");
    }
    {
        KURL waba2( waba1, "relative.html?query=test&name=harry");
        check("http: Relative URL, with query", waba2.url(), "http://www.website.com/directory/relative.html?query=test&name=harry");
        waba2.removeQueryItem("query");
        check("http: Removing query item", waba2.url(), "http://www.website.com/directory/relative.html?name=harry");
        waba2.addQueryItem("age", "18");
        check("http: Adding query item", waba2.url(), "http://www.website.com/directory/relative.html?name=harry&age=18");
        waba2.addQueryItem("age", "21");
        check("http: Adding query item", waba2.url(), "http://www.website.com/directory/relative.html?name=harry&age=18&age=21");
        waba2.addQueryItem("fullname", "Harry Potter");
        check("http: Adding query item", waba2.url(), "http://www.website.com/directory/relative.html?name=harry&age=18&age=21&fullname=Harry%20Potter");
    }
    {
        KURL waba2( waba1, "?query=test&name=harry");
        check("http: Relative URL, with query and no filename", waba2.url(), "http://www.website.com/directory/?query=test&name=harry");
    }
    {
        KURL waba2( waba1, "relative.html#with_reference");
        check("http: Relative URL, with reference", waba2.url(), "http://www.website.com/directory/relative.html#with_reference");
    }
    {
        KURL waba2( waba1, "#");
        check("http: Relative URL, with empty reference", waba2.url(), "http://www.website.com/directory/?hello#");
    }
    {
        KURL waba2( waba1, "");
        check("http: Empty relative URL", waba2.url(), "http://www.website.com/directory/?hello#ref");
    }
    {
        KURL base( "http://faure@www.kde.org" ); // no path
        KURL waba2( base, "filename.html");
        check("http: Relative URL, orig URL had no path", waba2.url(), "http://faure@www.kde.org/filename.html");
    }
    {
        KURL base( "http://faure:pass@www.kde.org:81?query" );
        KURL rel1( base, "http://www.kde.org/bleh/"); // same host
        check("http: Relative URL, orig URL had username", rel1.url(), "http://faure:pass@www.kde.org/bleh/");
        KURL rel2( base, "http://www.yahoo.org"); // different host
        check("http: Relative URL, orig URL had username", rel2.url(), "http://www.yahoo.org");
    }

    waba1 = "http://www.website.com/directory/filename?bla#blub";
    {
        KURL waba2( waba1, "relative.html");
        check("http: Relative URL, single file", waba2.url(), "http://www.website.com/directory/relative.html");
    }
    {
        KURL waba2( waba1, "../relative.html");
        check("http: Relative URL, single file, directory up", waba2.url(), "http://www.website.com/relative.html");
    }
    {
        KURL waba2( waba1, "down/relative.html");
        check("http: Relative URL, single file, directory down", waba2.url(), "http://www.website.com/directory/down/relative.html");
    }
    {
        KURL waba2( waba1, "/down/relative.html");
        check("http: Relative URL, full path", waba2.url(), "http://www.website.com/down/relative.html");
    }
    {
        KURL waba2( waba1, "relative.html?query=test&name=harry");
        check("http: Relative URL, with query", waba2.url(), "http://www.website.com/directory/relative.html?query=test&name=harry");
    }
    {
        KURL waba2( waba1, "?query=test&name=harry");
        check("http: Relative URL, with query and no filename", waba2.url(), "http://www.website.com/directory/filename?query=test&name=harry");
    }
    {
        KURL waba2( waba1, "relative.html#with_reference");
        check("http: Relative URL, with reference", waba2.url(), "http://www.website.com/directory/relative.html#with_reference");
    }
    {
        KURL waba2( waba1, "http:/relative.html"); // "rfc 1606 loophole"
        check("http: Strange relative URL", waba2.url(), "http://www.website.com/relative.html");
    }
    waba1.setUser("waldo");
    check("http: Set user", waba1.url(), "http://waldo@www.website.com/directory/filename?bla#blub");
    waba1.setUser("waldo/bastian");
    check("http: Set user with slash in it", waba1.url(), "http://waldo%2Fbastian@www.website.com/directory/filename?bla#blub");
    waba1.setRef( QString() );
    waba1.setPass( "pass" );
    waba1.setDirectory( "/foo" );
    waba1.setProtocol( "https" );
    waba1.setHost( "web.com" );
    waba1.setPort( 881 );
    check("http: setRef/setPass/setDirectory/setHost/setPort", waba1.url(), "https://waldo%2Fbastian:pass@web.com:881/foo/?bla");
    waba1.setDirectory( "/foo/" );
    check("http: setDirectory #2", waba1.url(), "https://waldo%2Fbastian:pass@web.com:881/foo/?bla");

// Empty queries should be preserved!
    waba1 = "http://www.kde.org/cgi/test.cgi?";
    check("http: URL with empty query string", waba1.url(),
          "http://www.kde.org/cgi/test.cgi?");

// Empty references should be preserved
    waba1 = "http://www.kde.org/cgi/test.cgi#";
    check("http: URL with empty reference string", waba1.url(),
          "http://www.kde.org/cgi/test.cgi#");
    check("hasRef()", waba1.hasRef()?"true":"false","true");
    check("hasHTMLRef()", waba1.hasHTMLRef()?"true":"false","true");
    check("encodedHtmlRef()", waba1.encodedHtmlRef(),QString());

// URLs who forgot to encode spaces in the query.
    waba1 = "http://www.kde.org/cgi/test.cgi?hello=My Value";
    check("http: URL with incorrect encoded query", waba1.url(),
          "http://www.kde.org/cgi/test.cgi?hello=My%20Value");

// URL with ':' in query (':' should NOT be encoded!)
    waba1.setQuery("hello:My Value");
    check("http: URL with ':' in query", waba1.url(),
          "http://www.kde.org/cgi/test.cgi?hello:My%20Value");
    check("upURL() removes query", waba1.upURL().url(),
          "http://www.kde.org/cgi/test.cgi");

// URLs who forgot to encode spaces in the query.
    waba1 = "http://www.kde.org/cgi/test.cgi?hello=My Value+20";
    check("http: URL with incorrect encoded query", waba1.url(),
          "http://www.kde.org/cgi/test.cgi?hello=My%20Value+20");

// Urls without path (BR21387)
    waba1 = "http://meine.db24.de?link=home_c_login_login";
    check("http: URL with empty path string", waba1.url(),
          "http://meine.db24.de?link=home_c_login_login");
    check("http: URL with empty path string path", waba1.path(),
          "");
    check("http: URL with empty path string query", waba1.query(),
          "?link=home_c_login_login");

    waba1 = "http://a:389?b=c";
    check( "http: URL with port, query, and empty path; url", waba1.url(), "http://a:389?b=c" );
    check( "http: URL with port, query, and empty path; host", waba1.host(), "a" );
    check( "http: URL with port, query, and empty path; port", QString::number( waba1.port() ), "389" );
    check( "http: URL with port, query, and empty path; path", waba1.path(), "" );
    check( "http: URL with port, query, and empty path; query", waba1.query(), "?b=c" );

// Urls without path (BR21387)
    waba1 = "http://meine.db24.de#link=home_c_login_login";
    check("http: URL with empty path string", waba1.url(),
          "http://meine.db24.de#link=home_c_login_login");
    check("http: URL with empty path string path", waba1.path(),
          "");

    waba1 = "http://a:389#b=c";
    check( "http: URL with port, ref, and empty path; url", waba1.url(), "http://a:389#b=c" );
    check( "http: URL with port, ref, and empty path; host", waba1.host(), "a" );
    check( "http: URL with port, ref, and empty path; port", QString::number( waba1.port() ), "389" );
    check( "http: URL with port, ref, and empty path; path", waba1.path(), "" );
    check( "http: URL with port, ref, and empty path; ref", waba1.ref(), "b=c" );
    check( "http: URL with port, ref, and empty path; query", waba1.query(), "" );

// IPV6
    waba1 = "http://[::FFFF:129.144.52.38]:81/index.html";
    check("http: IPV6 host", waba1.host(),
          "::ffff:129.144.52.38");
    check("http: IPV6 port", QString("%1").arg(waba1.port()),
          "81");

// IPV6
    waba1 = "http://waba:pass@[::FFFF:129.144.52.38]:81/index.html";
    check("http: IPV6 host", waba1.host(),
          "::ffff:129.144.52.38");
    check("http: IPV6 host", waba1.user(),
          "waba");
    check("http: IPV6 host", waba1.pass(),
          "pass");
    check("http: IPV6 port", QString("%1").arg(waba1.port()),
          "81");

// IPV6
    waba1 = "http://www.kde.org/cgi/test.cgi";
    waba1.setHost("::ffff:129.144.52.38");
    check("http: IPV6 host", waba1.url(),
          "http://[::ffff:129.144.52.38]/cgi/test.cgi");
    waba1 = "http://[::ffff:129.144.52.38]/cgi/test.cgi";
    assert( waba1.isValid() );

// IPV6 without path
    waba1 = "http://[::ffff:129.144.52.38]?query";
    assert( waba1.isValid() );
    check("http: IPV6 without path", waba1.url(),
          "http://[::ffff:129.144.52.38]?query");
    check("http: IPV6 without path; query", waba1.query(),
          "?query");
    waba1 = "http://[::ffff:129.144.52.38]#ref";
    assert( waba1.isValid() );
    check("http: IPV6 without path", waba1.url(),
          "http://[::ffff:129.144.52.38]#ref");
    check("http: IPV6 without path; ref", waba1.ref(),
          "ref");
// IPV6 without path but with a port
    waba1 = "http://[::ffff:129.144.52.38]:81?query";
    assert( waba1.isValid() );
    check("http: IPV6 without path", waba1.url(),
          "http://[::ffff:129.144.52.38]:81?query");
    check("http: IPV6 without path; port", QString::number( waba1.port() ), "81" );
    check("http: IPV6 without path; query", waba1.query(), "?query");
    waba1 = "http://[::ffff:129.144.52.38]:81#ref";
    assert( waba1.isValid() );
    check("http: IPV6 without path", waba1.url(),
          "http://[::ffff:129.144.52.38]:81#ref");
    check("http: IPV6 without path; port", QString::number( waba1.port() ), "81" );
    check("http: IPV6 without path; ref", waba1.ref(), "ref");

// Streaming operators
    KURL origURL( "http://www.website.com/directory/?#ref" );
    waba1 = "http://[::ffff:129.144.52.38]:81?query";
    QByteArray buffer;
    {
        QDataStream stream( buffer, QIODevice::WriteOnly );
        stream << origURL
               << KURL( "file:" ) // an invalid one
               << waba1; // the IPv6 one
    }
    {
        QDataStream stream( buffer, QIODevice::ReadOnly );
        KURL restoredURL;
        stream >> restoredURL;
        check( "Streaming valid URL", origURL.url(), restoredURL.url() );
        stream >> restoredURL;
        check( "Streaming invalid URL", restoredURL.isValid()?"valid":"malformed", "malformed" );
        check( "Streaming invalid URL", restoredURL.url(), "file:" );
        stream >> restoredURL;
        check( "Streaming ipv6 URL with query", restoredURL.url(), waba1.url() );
    }

// Broken stuff
    waba1 = "file:a";
    check("Broken stuff #1 path", waba1.path(), "a");
    check("Broken stuff #1 fileName(false)", waba1.fileName(false), "a");
    check("Broken stuff #1 fileName(true)", waba1.fileName(true), "a");
    check("Broken stuff #1 directory(false, false)", waba1.directory(false, false), "");
    check("Broken stuff #1 directory(true, false)", waba1.directory(true, false), "");
    check("Broken stuff #1 directory(false, true)", waba1.directory(true, true), "");

    waba1 = "file:a/";
    check("Broken stuff #2 path", waba1.path(), "a/");
    check("Broken stuff #2 fileName(false)", waba1.fileName(false), "");
    check("Broken stuff #2 fileName(true)", waba1.fileName(true), "a");
    check("Broken stuff #2 directory(false, false)", waba1.directory(false, false), "a/");
    check("Broken stuff #2 directory(true, false)", waba1.directory(true, false), "a");
    check("Broken stuff #2 directory(false, true)", waba1.directory(true, true), "");

    waba1 = "file:";
    check("Broken stuff #3 empty", waba1.isEmpty()?"EMPTY":"NOT", "NOT");
    check("Broken stuff #3 valid", waba1.isValid()?"VALID":"MALFORMED", "MALFORMED");
    check("Broken stuff #3 path", waba1.path(), "");
    check("Broken stuff #3 fileName(false)", waba1.fileName(false), "");
    check("Broken stuff #3 fileName(true)", waba1.fileName(true), "");
    check("Broken stuff #3 directory(false, false)", waba1.directory(false, false), "");
    check("Broken stuff #3 directory(true, false)", waba1.directory(true, false), "");
    check("Broken stuff #3 directory(false, true)", waba1.directory(true, true), "");
    KURL broken;
    broken.setPath( QString() );
    check("Broken stuff #4 empty", broken.isEmpty()?"EMPTY":"NOT", "NOT");
// It's valid: because isValid refers to parsing, not to what happens afterwards.
    check("Broken stuff #4 valid", broken.isValid()?"VALID":"MALFORMED", "VALID");
    check("Broken stuff #4 path", broken.path(), "");
    broken = "file://"; // just because coolo wondered
    check("Broken stuff #5 empty", broken.isEmpty()?"EMPTY":"NOT", "NOT");
    check("Broken stuff #5 valid", broken.isValid()?"VALID":"MALFORMED", "MALFORMED");
    check("Broken stuff #5 path", broken.path(), "");
    broken = "file";
    check("Broken stuff #6 valid", broken.isValid()?"VALID":"MALFORMED", "MALFORMED");

#if 0 // BROKEN?
// UNC like names
    KURL unc1("FILE://localhost/home/root");
    check("UNC, with localhost", unc1.path(), "/home/root");
    check("UNC, with localhost", unc1.url(), "file:/home/root");
#endif
    KURL unc2("file:///home/root");
    check("UNC, with empty host", unc2.path(), "/home/root");
    check("UNC, with empty host", unc2.url(), "file:/home/root");

    {
        KURL unc3("FILE://remotehost/home/root");
#if 0 // BROKEN?
        check("UNC, with remote host", unc3.path(), "//remotehost/home/root");
#endif
        check("UNC, with remote host", unc3.url(), "file://remotehost/home/root");
        KURL url2("file://atlas/dfaure");
        check("KURL::host()", url2.host(), "atlas");
        check("KURL::path()", url2.path(), "/dfaure");
        //check("KURL::path()", url3.path(), "//atlas/dfaure"); // says Waba
        //KURL url3("file:////atlas/dfaure");
        //check("KURL::path()", url3.path(), "//atlas/dfaure"); // says Waba
    }

    KURL umail1 ( "mailto:faure@kde.org" );
    check("mailto: URL, general form", umail1.protocol(), "mailto");
    check("mailto: URL, general form", umail1.path(), "faure@kde.org");
    check("mailto: URL, is relative", KURL::isRelativeURL("mailto:faure@kde.org") ? "true" : "false", "false");
    KURL umail2 ( "mailto:Faure David <faure@kde.org>" );
    check("mailto: URL, general form", umail2.protocol(), "mailto");
    check("mailto: URL, general form", umail2.path(), "Faure David <faure@kde.org>");
    check("isRelativeURL(\"mailto:faure@kde.org\")", KURL::isRelativeURL("mailto:faure@kde.org") ? "yes" : "no", "no");
    KURL umail3 ( "mailto:" );
    check("mailto: invalid URL", umail3.isValid()?"valid":"malformed", "malformed");

    check("man: URL, is relative", KURL::isRelativeURL("man:mmap") ? "true" : "false", "false");
    check("javascript: URL, is relative", KURL::isRelativeURL("javascript:doSomething()") ? "true" : "false", "false");
// more isRelative
    check("file: URL, is relative", KURL::isRelativeURL("file:/blah") ? "true" : "false", "false");
    check("/path, is relative", KURL::isRelativeURL("/path") ? "true" : "false", "true"); // arguable
    check("something, is relative", KURL::isRelativeURL("something") ? "true" : "false", "true");
    KURL about("about:konqueror");
    check("about:",about.path(),"konqueror");

    KURL ulong("https://swww.gad.de:443/servlet/CookieAccepted?MAIL=s@gad.de&VER=25901");
    check("host",ulong.host(),"swww.gad.de");
    check("path",ulong.path(),"/servlet/CookieAccepted");

    QTextCodec::setCodecForLocale( KGlobal::charsets()->codecForName( "iso-8859-1" ) );
// UTF8 tests
    KURL uloc("/home/dfaure/konqtests/Matériel");
    check("locale8bit",uloc.url().latin1(),"file:/home/dfaure/konqtests/Mat%E9riel"); // escaping the letter would be correct too
    check("pretty",uloc.prettyURL(),"file:/home/dfaure/konqtests/Matériel"); // escaping the letter would be correct too
// 106 is MIB for UTF-8
    check("UTF8",uloc.url(0, 106),"file:/home/dfaure/konqtests/Mat%C3%A9riel");
    uloc = KURL("file:/home/dfaure/konqtests/Mat%C3%A9riel", 106);
    check("UTF8 path", uloc.path(), "/home/dfaure/konqtests/Matériel");

// fromPathOrURL tests
    uloc = KURL::fromPathOrURL( "/home/dfaure/konqtests/Mat%E9riel" );
    check("fromPathOrURL path", uloc.path(), "/home/dfaure/konqtests/Mat%E9riel");
    uloc = KURL::fromPathOrURL( "http://www.kde.org" );
    check("fromPathOrURL url", uloc.url(), "http://www.kde.org");
    uloc = KURL::fromPathOrURL( "www.kde.org" );
    check("fromPathOrURL url", uloc.isValid()?"valid":"malformed", "malformed");
    uloc = KURL::fromPathOrURL( "index.html" );
    check("fromPathOrURL url", uloc.isValid()?"valid":"malformed", "malformed");
    uloc = KURL::fromPathOrURL( "" );
    check("fromPathOrURL url", uloc.isValid()?"valid":"malformed", "malformed");

    QTextCodec::setCodecForLocale( KGlobal::charsets()->codecForName( "koi8-r" ) );
    baseURL = "file:/home/coolo";
    KURL russian = baseURL.directory(false, true) + QString::fromLocal8Bit( "ÆÇÎ7" );
    check( "russian", russian.url(), "file:/home/%C6%C7%CE7" );

    KURL tobi1("http://some.host.net/path/to/file#fragmentPrecedes?theQuery");
    check("wrong order of query and hypertext reference #1", tobi1.ref(), "fragmentPrecedes");
    check("wrong order of query and hypertext reference #2", tobi1.query(), "?theQuery");

    tobi1 = "http://host.net/path/?#http://brokenäadsfküpoij31ü029muß2890zupycÜ*!*'O´+ß0i";
    check("zero-length query",tobi1.query(),"?");

    tobi1 = "http://host.net/path/#no-query";
    check("no query", tobi1.query(),"");

    tobi1 = "http://host.net/path?myfirstquery#andsomeReference";
    tobi1.setEncodedPathAndQuery("another/path/?another&query");
    check("setEncodedPathAndQuery test#1", tobi1.query(), "?another&query");
    check("setEncodedPathAndQuery test#2", tobi1.path(), "another/path/");
    tobi1.setEncodedPathAndQuery("another/path?another&query");
    check("setEncodedPathAndQuery test#1", tobi1.query(), "?another&query");
    check("setEncodedPathAndQuery test#2", tobi1.path(), "another/path");

    KURL theKow = "http://www.google.de/search?q=frerich&hlx=xx&hl=de&empty=&lr=lang+de&test=%2B%20%3A%25";
    check("queryItem (first item)", theKow.queryItem("q"), "frerich");
    check("queryItem (middle item)", theKow.queryItem("hl"), "de");
    check("queryItem (last item)", theKow.queryItem("lr"), "lang de");
    check("queryItem (invalid item)", theKow.queryItem("InterstellarCounselor"), QString());
    check("queryItem (empty item)", theKow.queryItem("empty"), "");
    check("queryItem (item with encoded chars)", theKow.queryItem("test"), "+ :%");

// checks for queryItems(), which returns a QMap<QString,QString>:
    KURL queryUrl( "mailto:Marc%20Mutz%20%3cmutz@kde.org%3E?"
                   "Subject=subscribe+me&"
                   "body=subscribe+mutz%40kde.org&"
                   "Cc=majordomo%40lists.kde.org" );
    check("queryItems (c.s. keys)",
          QStringList(queryUrl.queryItems().keys()).join(", "),
          "Cc, Subject, body" );
    check("queryItems (c.i.s. keys)",
          QStringList(queryUrl.queryItems(KURL::CaseInsensitiveKeys).keys()).join(", "),
          "body, cc, subject" );
    check("queryItems (values; c.s. keys)",
          QStringList(queryUrl.queryItems().values()).join(", "),
          "majordomo@lists.kde.org, subscribe me, subscribe mutz@kde.org" );
    check("queryItems (values; c.i.s. keys)",
          QStringList(queryUrl.queryItems(KURL::CaseInsensitiveKeys).values()).join(", "),
          "subscribe mutz@kde.org, majordomo@lists.kde.org, subscribe me" );

    KURL umlaut1("http://www.clever-tanken.de/liste.asp?ort=N%FCrnberg&typ=Diesel");
    check("umlaut1.url()", umlaut1.url(), "http://www.clever-tanken.de/liste.asp?ort=N%FCrnberg&typ=Diesel");

    KURL umlaut2("http://www.clever-tanken.de/liste.asp?ort=N%FCrnberg&typ=Diesel", 106);
    check("umlaut2.url()", umlaut2.url(), "http://www.clever-tanken.de/liste.asp?ort=N%FCrnberg&typ=Diesel");

// Needed for #49616
    check( "encode_string('C++')", KURL::encode_string( "C++" ), "C%2B%2B" );
    check( "decode_string('C%2B%2B')", KURL::decode_string( "C%2B%2B" ), "C++" );
    check( "decode_string('C%00A')", KURL::decode_string( "C%00%A" ), "C" ); // we stop at %00

    check( "encode_string('%')", KURL::encode_string( "%" ), "%25" );
    check( "encode_string(':')", KURL::encode_string( ":" ), "%3A" );

    KURL amantia( "http://%E1.foo" );
    check("amantia.isValid()", amantia.isValid() ? "true" : "false", "true");
#ifdef HAVE_IDNA_H
    check("amantia.url()", amantia.url(), "http://xn--80a.foo");   // Non-ascii is allowed in IDN domain names.
#else
    check("amantia.url()", amantia.url(), "http://?.foo"); // why not
#endif

    KURL smb("smb://domain;username:password@server/share");
    check("smb.isValid()", smb.isValid() ? "true" : "false", "true");
    check("smb.user()", smb.user(), "domain;username");
    smb = "smb:/";
    check("smb:/", smb.isValid()?"VALID":"MALFORMED", "VALID");
    smb = "smb://"; // kurl.cpp rev 1.106
    check("smb://", smb.isValid()?"VALID":"MALFORMED", "MALFORMED");
    smb = "smb://host";
    check("smb://host", smb.isValid()?"VALID":"MALFORMED", "VALID");
    smb = "smb:///";
    check("smb:///", smb.isValid()?"VALID":"MALFORMED", "VALID");

    KURL weird;
    weird = "http://strange<hostname>/";
    check("weird.isValid()", weird.isValid() ? "true" : "false", "false");

    weird = "http://strange<username>@strange<hostname>/";
    check("weird.isValid()", weird.isValid() ? "true" : "false", "false");

    weird = "http://strange<username>@ok_hostname/";
    check("weird.isValid()", weird.isValid() ? "true" : "false", "true");
    check("weird.host()", weird.host(), "ok_hostname");

    weird = "http://strange;hostname/";
    check("weird.isValid()", weird.isValid() ? "true" : "false", "false");

    weird = "http://strange;username@strange;hostname/";
    check("weird.isValid()", weird.isValid() ? "true" : "false", "false");

    weird = "http://strange;username@ok_hostname/";
    check("weird.isValid()", weird.isValid() ? "true" : "false", "true");
    check("weird.host()", weird.host(), "ok_hostname");

    weird = "http://strange;username:password@strange;hostname/";
    check("weird.isValid()", weird.isValid() ? "true" : "false", "false");

    weird = "http://strange;username:password@ok_hostname/";
    check("weird.isValid()", weird.isValid() ? "true" : "false", "true");
    check("weird.host()", weird.host(), "ok_hostname");

    weird = "http://[strange;hostname]/";
    check("weird.isValid()", weird.isValid() ? "true" : "false", "false");

    weird = "http://[::fff:1:23]/";
    check("weird.isValid()", weird.isValid() ? "true" : "false", "true");
    check("weird.host()", weird.host(), "::fff:1:23");

    KURL com1("http://server.com/dir/", ".");
    check("com1.url()", com1.url(), "http://server.com/dir/");

    KURL com2("http://server.com/dir/blubb/", "blah/");
    check("com2.url()", com2.url(), "http://server.com/dir/blubb/blah/");

    KURL utf8_1("audiocd:/By%20Name/15%20Geantra%C3%AE.wav", 106);
    check("utf8_1.fileName()", utf8_1.fileName(), QString::fromLatin1("15 Geantraî.wav"));

    KURL utf8_2("audiocd:/By%20Name/15%2fGeantra%C3%AE.wav", 106);
    check("utf8_2.fileName()", utf8_2.fileName(), QString::fromLatin1("15/Geantraî.wav"));

    KURL url_newline_1("http://www.foo.bar/foo/bar\ngnork");
    check("url_newline_1.url()", url_newline_1.url(), QString::fromLatin1("http://www.foo.bar/foo/bar%0Agnork"));

    KURL url_newline_2("http://www.foo.bar/foo?bar\ngnork");
    check("url_newline_2.url()", url_newline_2.url(), QString::fromLatin1("http://www.foo.bar/foo?bar%0Agnork"));

    KURL local_file_1("file://localhost/my/file");
    check("local_file_1.isLocalFile()", local_file_1.isLocalFile() ? "true" : "false", "true");

    KURL local_file_2("file://www.kde.org/my/file");
    check("local_file_2.isLocalFile()", local_file_2.isLocalFile() ? "true" : "false", "false");

    KURL local_file_3;
    local_file_3.setHost(getenv("HOSTNAME"));
    local_file_3.setPath("/my/file");
    printf("\nURL=%s\n", local_file_3.url().latin1());
    check("local_file_3.isLocalFile()", local_file_3.isLocalFile() ? "true" : "false", "true");

    KURL local_file_4("file:///my/file");
    check("local_file_4.isLocalFile()", local_file_4.isLocalFile() ? "true" : "false", "true");

    KURL local_file_5;
    local_file_5.setPath("/foo?bar");
    check("local_file_5.url()", local_file_5.url(), "file:/foo%3Fbar");

    QString basePath = "/home/bastian";

    check("relativePath(\"/home/bastian\", \"/home/bastian\")", KURL::relativePath(basePath, "/home/bastian"), "./");
    bool b;
    check("relativePath(\"/home/bastian\", \"/home/bastian/src/plugins\")", KURL::relativePath(basePath, "/home/bastian/src/plugins", &b), "./src/plugins");
    check("Is a subdirectory?", b ? "true" : "false", "true");
    check("relativePath(\"/home/bastian\", \"./src/plugins\")", KURL::relativePath(basePath, "./src/plugins"), "./src/plugins");
    check("relativePath(\"/home/bastian\", \"/home/waba/src/plugins\")", KURL::relativePath(basePath, "/home/waba/src/plugins", &b), "../waba/src/plugins");
    check("Is a subdirectory?", b ? "true" : "false", "false");
    check("relativePath(\"/home/bastian\", \"/\")", KURL::relativePath(basePath, "/"), "../../");

    check("relativePath(\"/\", \"/\")", KURL::relativePath("/", "/"), "./");
    check("relativePath(\"/\", \"/home/bastian\")", KURL::relativePath("/", "/home/bastian"), "./home/bastian");
    check("relativePath(\"\", \"/home/bastian\")", KURL::relativePath("", "/home/bastian"), "/home/bastian");

    baseURL = "http://www.kde.org/index.html";
    check("relativeURL(\"http://www.kde.org/index.html\", \"http://www.kde.org/index.html#help\")", KURL::relativeURL(baseURL, "http://www.kde.org/index.html#help"), "#help");
    check("relativeURL(\"http://www.kde.org/index.html\", \"http://www.kde.org/index.html?help=true\")", KURL::relativeURL(baseURL, "http://www.kde.org/index.html?help=true"), "index.html?help=true");
    check("relativeURL(\"http://www.kde.org/index.html\", \"http://www.kde.org/contact.html\")", KURL::relativeURL(baseURL, "http://www.kde.org/contact.html"), "contact.html");
    check("relativeURL(\"http://www.kde.org/index.html\", \"ftp://ftp.kde.org/pub/kde\")", KURL::relativeURL(baseURL, "ftp://ftp.kde.org/pub/kde"), "ftp://ftp.kde.org/pub/kde");
    check("relativeURL(\"http://www.kde.org/index.html\", \"http://www.kde.org/index.html\")", KURL::relativeURL(baseURL, "http://www.kde.org/index.html"), "./");

    baseURL = "http://www.kde.org/info/index.html";
    check("relativeURL(\"http://www.kde.org/info/index.html\", \"http://www.kde.org/bugs/contact.html\")", KURL::relativeURL(baseURL, "http://www.kde.org/bugs/contact.html"), "../bugs/contact.html");

    baseURL = "ptal://mlc:usb:PC_970";
    check("isValid()?", baseURL.isValid() ? "true" : "false", "false");
    check("url()", baseURL.url(), "ptal://mlc:usb:PC_970");

    baseURL = "http://mlc:80/";
    check("isValid()?", baseURL.isValid() ? "true" : "false", "true");
    check("port()?", QString::number(baseURL.port()), "80");
    check("path()?", baseURL.path(), "/");

    baseURL = "ptal://mlc:usb@PC_970"; // User=mlc, password=usb, host=PC_970
    check("isValid()?", baseURL.isValid() ? "true" : "false", "true");
    check("host()?", baseURL.host(), "pc_970");
    check("user()?", baseURL.user(), "mlc");
    check("pass()?", baseURL.pass(), "usb");

    weird = "ftp://user%40host.com@ftp.host.com/var/www/";
    check("user()?", weird.user(), "user@host.com" );
    check("host()?", weird.host(), "ftp.host.com" );
    KURL up = weird.upURL();
    check("KURL::upURL()", up.url(), "ftp://user%40host.com@ftp.host.com/var/");
    up = up.upURL();
    check("KURL::upURL()", up.url(), "ftp://user%40host.com@ftp.host.com/");
    up = up.upURL();
    check("KURL::upURL()", up.url(), "ftp://user%40host.com@ftp.host.com/"); // unchanged

    KURL ldap = "ldap://host.com:6666/o=University%20of%20Michigan,c=US??sub?(cn=Babs%20Jensen)";
    check("host()?", ldap.host(), "host.com");
    check("port()?", QString("%1").arg(ldap.port()), "6666");
    check("path()?", ldap.path(), "/o=University of Michigan,c=US");
    check("query()?", ldap.query(), "??sub?(cn=Babs%20Jensen)");
    check("url()?", ldap.url(), "ldap://host.com:6666/o=University%20of%20Michigan,c=US??sub?(cn=Babs%20Jensen)");
    ldap.setQuery("??sub?(cn=Karl%20Marx)");
    check("query()?", ldap.query(), "??sub?(cn=Karl%20Marx)");
    check("url()?", ldap.url(), "ldap://host.com:6666/o=University%20of%20Michigan,c=US??sub?(cn=Karl%20Marx)");

    KURL leo = "data:text/html,http://www.invalid/";
    check("data URL: isValid", leo.isValid()?"valid":"malformed", "valid" );
    check("data URL: protocol", leo.protocol(), "data" );
    check("data URL: url", leo.url(), "data:text/html,http://www.invalid/" );
    check("data URL: path", leo.path(), "text/html,http://www.invalid/" );

// URI Mode tests
    url1 = "http://www.foobar.com/";
    check("KURL(\"http://www.foobar.com/\").uriMode()", QString::number(url1.uriMode()), QString::number(KURL::URL));
    url1 = "mailto:user@host.com";
    check("KURL(\"mailto:user@host.com\").uriMode()", QString::number(url1.uriMode()), QString::number(KURL::Mailto));
    check("KURL(\"mailto:user@host.com\").url()", url1.url(), "mailto:user@host.com");
    check("KURL(\"mailto:user@host.com\").url(0, 106)", url1.url(0, 106), "mailto:user@host.com");
    url1 = "data:text/plain,foobar?gazonk=flarp";
    check("KURL(\"data:text/plain,foobar?gazonk=flarp\").uriMode()", QString::number(url1.uriMode()), QString::number(KURL::RawURI));
    check("KURL(\"data:text/plain,foobar?gazonk=flarp\").path()", url1.path(), "text/plain,foobar?gazonk=flarp");
    url1 = "mailto:User@Host.COM?subject=Hello";
    check("KURL(\"mailto:User@Host.COM?subject=Hello\").path()", url1.path(), "User@host.com");

    printf("\nTest OK !\n");
    */


        }

void tst_QUrl::i18n_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QByteArray>("punyOutput");

    QTest::newRow("øl") << QString::fromLatin1("http://ole:passord@www.øl.no/index.html?ole=æsemann&ilder gud=hei#top")
                     <<          QByteArray("http://ole:passord@www.xn--l-4ga.no/index.html?ole=%C3%A6semann&ilder%20gud=hei#top");
    QTest::newRow("räksmörgås") << QString::fromLatin1("http://www.räksmörgås.no/")
                             << QByteArray("http://www.xn--rksmrgs-5wao1o.no/");
    QTest::newRow("bühler") << QString::fromLatin1("http://www.bühler.no/")
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
    QTest::addColumn<QString>("relsolvedUrl");

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
    QFETCH(QString, relsolvedUrl);

    QUrl url(baseUrl);
    QCOMPARE(url.resolved(relativeUrl).toString(), relsolvedUrl);
}


void tst_QUrl::toString_data()
{
    QTest::addColumn<QString>("urlString");
    QTest::addColumn<uint>("options");
    QTest::addColumn<QString>("string");

    QTest::newRow("data0")	<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
			<< uint(QUrl::RemoveScheme)
			<< QString::fromLatin1("//ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data2")	<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
			<< uint(QUrl::RemovePassword)
			<< QString::fromLatin1("http://ole@www.troll.no:9090/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data3")	<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
			<< uint(QUrl::RemoveUserInfo)
			<< QString::fromLatin1("http://www.troll.no:9090/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data4")	<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
			<< uint(QUrl::RemovePort)
			<< QString::fromLatin1("http://ole:password@www.troll.no/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data5")	<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
			<< uint(QUrl::RemoveAuthority)
			<< QString::fromLatin1("http:/index.html?ole=semann&gud=hei#top");

    QTest::newRow("data6")	<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
			<< uint(QUrl::RemovePath)
			<< QString::fromLatin1("http://ole:password@www.troll.no:9090?ole=semann&gud=hei#top");

    QTest::newRow("data7")	<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
			<< uint(QUrl::RemoveQuery)
			<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html#top");

    QTest::newRow("data8")	<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
			<< uint(QUrl::RemoveFragment)
			<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei");

    QTest::newRow("data9")	<< QString::fromLatin1("http://ole:password@www.troll.no:9090/index.html?ole=semann&gud=hei#top")
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

    /*
    QTest::newRow("data19") << QString::fromLatin1("http://andreas:hemmelig@www.vg.no/a/../?my=query&your=query#yougotfragged")
                         << uint(QUrl::None)
                         << QString::fromLatin1("http://andreas:hemmelig@www.vg.no/?my=query&your=query#yougotfragged");
    */

    QTest::newRow("nopath_task31320") << QString::fromLatin1("host://protocol")
                                   << uint(QUrl::None)
                                   << QString::fromLatin1("host://protocol");

    QTest::newRow("underscore_QTBUG-7434") << QString::fromLatin1("http://foo_bar.host.com/rss.php")
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

//### more tests ... what do we expect ...
void tst_QUrl::isParentOf_data()
{
    QTest::addColumn<QString>("parent");
    QTest::addColumn<QString>("child");
    QTest::addColumn<bool>("trueFalse");

    QTest::newRow("data0")	<< QString::fromLatin1("http://a.b.c/d")
			<< QString::fromLatin1("http://a.b.c/d/e?f") << true;
    QTest::newRow("data1")	<< QString::fromLatin1("http://a.b.c/d")
			<< QString::fromLatin1("http://a.b.c/d") << false;
    QTest::newRow("data2")	<< QString::fromLatin1("http://a.b.c/d")
			<< QString::fromLatin1("http://a.b.c/de") << false;
    QTest::newRow("data3")	<< QString::fromLatin1("http://a.b.c/d/")
			<< QString::fromLatin1("http://a.b.c/de") << false;
    QTest::newRow("data4")	<< QString::fromLatin1("http://a.b.c/d/")
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

    QString n("");

    QTest::newRow("data1") << n << n << n << QString::fromLatin1("qt.nokia.com") << -1 << QString::fromLatin1("index.html")
	                << QByteArray() << n << QString::fromLatin1("//qt.nokia.com/index.html")
			<< QByteArray("//qt.nokia.com/index.html");
    QTest::newRow("data2") << QString::fromLatin1("file") << n << n << n << -1 << QString::fromLatin1("/root") << QByteArray()
                        << n << QString::fromLatin1("file:///root") << QByteArray("file:///root");
    QTest::newRow("userAndPass") << QString::fromLatin1("http") << QString::fromLatin1("dfaure") << QString::fromLatin1("kde")
                                 << "kde.org" << 443 << QString::fromLatin1("/") << QByteArray() << n
                                 << QString::fromLatin1("http://dfaure:kde@kde.org:443/") << QByteArray("http://dfaure:kde@kde.org:443/");
    QTest::newRow("PassWithoutUser") << QString::fromLatin1("http") << n << QString::fromLatin1("kde")
                                     << "kde.org" << 443 << QString::fromLatin1("/") << QByteArray() << n
                                     << QString::fromLatin1("http://:kde@kde.org:443/") << QByteArray("http://:kde@kde.org:443/");
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
    QCOMPARE(url.toString(), asString);
    QCOMPARE(QString::fromLatin1(url.toEncoded()), QString::fromLatin1(asEncoded)); // readable in case of differences
    QCOMPARE(url.toEncoded(), asEncoded);
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

    QTest::newRow("data0")	<< QString::fromLatin1("file:/a.txt") << QString::fromLatin1("/a.txt");
    QTest::newRow("data4")	<< QString::fromLatin1("file:///a.txt") << QString::fromLatin1("/a.txt");
    QTest::newRow("data5")	<< QString::fromLatin1("file:///c:/a.txt") << QString::fromLatin1("c:/a.txt");
    QTest::newRow("data6")	<< QString::fromLatin1("file://somehost/somedir/somefile") << QString::fromLatin1("//somehost/somedir/somefile");
    QTest::newRow("data7")	<< QString::fromLatin1("file://somehost/") << QString::fromLatin1("//somehost/");
    QTest::newRow("data8")	<< QString::fromLatin1("file://somehost") << QString::fromLatin1("//somehost");
    QTest::newRow("data9")	<< QString::fromLatin1("file:////somehost/somedir/somefile") << QString::fromLatin1("//somehost/somedir/somefile");
    QTest::newRow("data10")	<< QString::fromLatin1("FILE:/a.txt") << QString::fromLatin1("/a.txt");

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
}

void tst_QUrl::fromLocalFile_data()
{
    QTest::addColumn<QString>("theFile");
    QTest::addColumn<QString>("theUrl");
    QTest::addColumn<QString>("thePath");

    QTest::newRow("data0")	<< QString::fromLatin1("/a.txt") << QString::fromLatin1("file:///a.txt") << QString::fromLatin1("/a.txt");
    QTest::newRow("data1")	<< QString::fromLatin1("a.txt") << QString::fromLatin1("file:a.txt") << QString::fromLatin1("a.txt");
    QTest::newRow("data2")	<< QString::fromLatin1("/a/b.txt") << QString::fromLatin1("file:///a/b.txt") << QString::fromLatin1("/a/b.txt");
    QTest::newRow("data3")	<< QString::fromLatin1("c:/a.txt") << QString::fromLatin1("file:///c:/a.txt") << QString::fromLatin1("/c:/a.txt");
    QTest::newRow("data4")	<< QString::fromLatin1("//somehost/somedir/somefile") << QString::fromLatin1("file://somehost/somedir/somefile")
	                << QString::fromLatin1("/somedir/somefile");
    QTest::newRow("data5")	<< QString::fromLatin1("//somehost") << QString::fromLatin1("file://somehost")
	                << QString::fromLatin1("");
    QTest::newRow("data6")	<< QString::fromLatin1("//somehost/") << QString::fromLatin1("file://somehost/")
	                << QString::fromLatin1("/");
}

void tst_QUrl::fromLocalFile()
{
    QFETCH(QString, theFile);
    QFETCH(QString, theUrl);
    QFETCH(QString, thePath);

    QUrl url = QUrl::fromLocalFile(theFile);

    QCOMPARE(url.toString(), theUrl);
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
	QUrl u( "http://qt.nokia.com/images/ban/pgs_front.jpg" );
	QCOMPARE( u.path(), QString("/images/ban/pgs_front.jpg") );
    }
    {
	QUrl tmp( "http://qt.nokia.com/images/ban/" );
	QUrl u = tmp.resolved(QString("pgs_front.jpg"));
	QCOMPARE( u.path(), QString("/images/ban/pgs_front.jpg") );
    }
    {
	QUrl tmp;
	QUrl u = tmp.resolved(QString("http://qt.nokia.com/images/ban/pgs_front.jpg"));
	QCOMPARE( u.path(), QString("/images/ban/pgs_front.jpg") );
    }
    {
	QUrl tmp;
	QUrl u = tmp.resolved(QString("http://qt.nokia.com/images/ban/pgs_front.jpg"));
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
    QTest::newRow( "data2" )  << QString("ftp://ftp.qt.nokia.com/qt/INSTALL") << QString("ftp://ftp.qt.nokia.com/qt/INSTALL");
    QTest::newRow( "data3" )  << QString("ftp://ftp.qt.nokia.com/qt/INSTALL") << QString("ftp://ftp.qt.nokia.com/qt/INSTALL");
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
     * op.copy(QString("ftp://ftp.qt.nokia.com/qt/INSTALL"), ".");
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
    QTest::newRow( "data0" )  << QString("ftp://ftp.qt.nokia.com/qt") << QString("INSTALL") << QString("ftp://ftp.qt.nokia.com/INSTALL");
    QTest::newRow( "data1" )  << QString("ftp://ftp.qt.nokia.com/qt/") << QString("INSTALL") << QString("ftp://ftp.qt.nokia.com/qt/INSTALL");
}

void tst_QUrl::compat_constructor_02()
{
    /* The following should work as expected:
     *
     * QUrlOperator op( "ftp://ftp.qt.nokia.com/qt" );
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
    QTest::newRow( "protocol00" )  << QString( "http://qt.nokia.com/index.html" ) << QString( "http://qt.nokia.com/index.html" );
    QTest::newRow( "protocol01" )  << QString( "http://qt.nokia.com" ) << QString( "http://qt.nokia.com" );
    QTest::newRow( "protocol02" )  << QString( "http://qt.nokia.com/" ) << QString( "http://qt.nokia.com/" );
    QTest::newRow( "protocol03" )  << QString( "http://qt.nokia.com/foo" ) << QString( "http://qt.nokia.com/foo" );
    QTest::newRow( "protocol04" )  << QString( "http://qt.nokia.com/foo/" ) << QString( "http://qt.nokia.com/foo/" );
    QTest::newRow( "protocol05" )  << QString( "ftp://ftp.qt.nokia.com/foo/index.txt" ) << QString( "ftp://ftp.qt.nokia.com/foo/index.txt" );

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

    QTest::newRow("task31280") << QString("protocol://host") << QString("protocol://host");
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

    QTest::newRow( "ok_01" ) << QString("ftp://ftp.qt.nokia.com/qt/INSTALL") << (bool)true;
    QTest::newRow( "ok_02" ) << QString( "file:/foo") << (bool)true;
    QTest::newRow( "ok_03" ) << QString( "file:foo") << (bool)true;

    QTest::newRow( "err_01" ) << QString("#ftp://ftp.qt.nokia.com/qt/INSTALL") << (bool)true;
    QTest::newRow( "err_02" ) << QString( "file:/::foo") << (bool)true;
}

void tst_QUrl::compat_isValid_01()
{
    QFETCH( QString, urlStr );
    QFETCH( bool, res );

    QUrl url( urlStr );
    QVERIFY( url.isValid() == res );
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
    QTest::newRow( "ok_02" ) << QString("ftp") << n     << n     << QString("ftp.qt.nokia.com") << -1 << n      << (bool)true;
    QTest::newRow( "ok_03" ) << QString("ftp") << QString("foo") << n     << QString("ftp.qt.nokia.com") << -1 << n      << (bool)true;
    QTest::newRow( "ok_04" ) << QString("ftp") << QString("foo") << QString("bar") << QString("ftp.qt.nokia.com") << -1 << n      << (bool)true;
    QTest::newRow( "ok_05" ) << QString("ftp") << n     << n     << QString("ftp.qt.nokia.com") << -1 << QString("path")<< (bool)true;
    QTest::newRow( "ok_06" ) << QString("ftp") << QString("foo") << n     << QString("ftp.qt.nokia.com") << -1 << QString("path") << (bool)true;
    QTest::newRow( "ok_07" ) << QString("ftp") << QString("foo") << QString("bar") << QString("ftp.qt.nokia.com") << -1 << QString("path")<< (bool)true;

    QTest::newRow( "err_01" ) << n     << n     << n     << n                   << -1 << n << (bool)false;
    QTest::newRow( "err_02" ) << QString("ftp") << n     << n     << n                   << -1 << n << (bool)true;
    QTest::newRow( "err_03" ) << n     << QString("foo") << n     << n                   << -1 << n << (bool)true;
    QTest::newRow( "err_04" ) << n     << n     << QString("bar") << n                   << -1 << n << (bool)true;
    QTest::newRow( "err_05" ) << n     << n     << n     << QString("ftp.qt.nokia.com") << -1 << n << (bool)true;
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

    QVERIFY( url.isValid() == res );
}

void tst_QUrl::compat_path_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("res");

    QTest::newRow( "protocol00" ) << "http://qt.nokia.com/images/ban/pgs_front.jpg" << "/images/ban/pgs_front.jpg";

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
    QTest::newRow("HTTPUrl") << QByteArray("http://qt.nokia.com") << QString("http://qt.nokia.com");
    QTest::newRow("HTTPUrlEncoded") << QByteArray("http://qt%20nokia%20com") << QString("http://qt nokia com");
    QTest::newRow("EmptyString") << QByteArray("") << QString("");
    QTest::newRow("Task27166") << QByteArray("Fran%C3%A7aise") << QString("Française");
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
    QTest::newRow("HTTPUrl") << QString("http://qt.nokia.com") << QByteArray("http%3A//qt.nokia.com");
    QTest::newRow("HTTPUrlEncoded") << QString("http://qt nokia com") << QByteArray("http%3A//qt%20nokia%20com");
    QTest::newRow("EmptyString") << QString("") << QByteArray("");
    QTest::newRow("Task27166") << QString::fromLatin1("Française") << QByteArray("Fran%C3%A7aise");
}

void tst_QUrl::compat_encode()
{
    QFETCH(QString, decodedString);
    QFETCH(QByteArray, encodedString);

    QCOMPARE(QUrl::toPercentEncoding(decodedString, "/.").constData(), encodedString.constData());
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
    QTest::newRow("test_02") << QString::fromLatin1("æss") << QByteArray("%C3%A6ss");
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
    QUrl u1(QLatin1String("http://qt.nokia.com")), u2(QLatin1String("http://www.kdab.com"));
    u1.swap(u2);
    QCOMPARE(u2.host(),QLatin1String("qt.nokia.com"));
    QCOMPARE(u1.host(),QLatin1String("www.kdab.com"));
}

void tst_QUrl::symmetry()
{
    QUrl url(QString::fromLatin1("http://www.räksmörgås.se/pub?a=b&a=dø&a=f#vræl"));
    QCOMPARE(url.scheme(), QString::fromLatin1("http"));
    QCOMPARE(url.host(), QString::fromLatin1("www.räksmörgås.se"));
    QCOMPARE(url.path(), QString::fromLatin1("/pub"));
    // this will be encoded ...
    QCOMPARE(url.encodedQuery().constData(), QString::fromLatin1("a=b&a=d%C3%B8&a=f").toLatin1().constData());
    // unencoded
    QCOMPARE(url.allQueryItemValues("a").join("").toLatin1().constData(), "bdøf");
    QCOMPARE(url.fragment(), QString::fromLatin1("vræl"));

    QUrl onlyHost("//qt.nokia.com");
    QCOMPARE(onlyHost.toString(), QString::fromLatin1("//qt.nokia.com"));

    {
        QString urlString = QString::fromLatin1("http://desktop:33326/upnp/{32f525a6-6f31-426e-91ca-01c2e6c2c57e}");
        QUrl urlPreviewList(urlString);
        QCOMPARE(urlPreviewList.toString(), urlString);
        QByteArray b = urlPreviewList.toEncoded();
        QCOMPARE(b.constData(), "http://desktop:33326/upnp/%7B32f525a6-6f31-426e-91ca-01c2e6c2c57e%7D");
        QCOMPARE(QUrl::fromEncoded(b).toString(), urlString);

    }{
        QString urlString = QString::fromLatin1("http://desktop:53423/deviceDescription?uuid={7977c17b-00bf-4af9-894e-fed28573c3a9}");
        QUrl urlPreviewList(urlString);
        QCOMPARE(urlPreviewList.toString(), urlString);
        QByteArray b = urlPreviewList.toEncoded();
        QCOMPARE(b.constData(), "http://desktop:53423/deviceDescription?uuid=%7B7977c17b-00bf-4af9-894e-fed28573c3a9%7D");
        QCOMPARE(QUrl::fromEncoded(b).toString(), urlString);
    }
}


void tst_QUrl::ipv6_data()
{
    QTest::addColumn<QString>("ipv6Auth");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("case 1") << QString::fromLatin1("//[56:56:56:56:56:56:56:56]") << true;
    QTest::newRow("case 2") << QString::fromLatin1("//[::56:56:56:56:56:56:56]") << true;
    QTest::newRow("case 3") << QString::fromLatin1("//[56::56:56:56:56:56:56]") << true;
    QTest::newRow("case 4") << QString::fromLatin1("//[56:56::56:56:56:56:56]") << true;
    QTest::newRow("case 5") << QString::fromLatin1("//[56:56:56::56:56:56:56]") << true;
    QTest::newRow("case 6") << QString::fromLatin1("//[56:56:56:56::56:56:56]") << true;
    QTest::newRow("case 7") << QString::fromLatin1("//[56:56:56:56:56::56:56]") << true;
    QTest::newRow("case 8") << QString::fromLatin1("//[56:56:56:56:56:56::56]") << true;
    QTest::newRow("case 9") << QString::fromLatin1("//[56:56:56:56:56:56:56::]") << true;
    QTest::newRow("case 4 with one less") << QString::fromLatin1("//[56::56:56:56:56:56]") << true;
    QTest::newRow("case 4 with less and ip4") << QString::fromLatin1("//[56::56:56:56:127.0.0.1]") << true;
    QTest::newRow("case 7 with one and ip4") << QString::fromLatin1("//[56::255.0.0.0]") << true;
    QTest::newRow("case 2 with ip4") << QString::fromLatin1("//[::56:56:56:56:56:0.0.0.255]") << true;
    QTest::newRow("case 2 with half ip4") << QString::fromLatin1("//[::56:56:56:56:56:56:0.255]") << false;
    QTest::newRow("case 4 with less and ip4 and port and useinfo") << QString::fromLatin1("//user:pass@[56::56:56:56:127.0.0.1]:99") << true;
    QTest::newRow("case :,") << QString::fromLatin1("//[:,]") << false;
    QTest::newRow("case ::bla") << QString::fromLatin1("//[::bla]") << false;
}

void tst_QUrl::ipv6()
{
    QFETCH(QString, ipv6Auth);
    QFETCH(bool, isValid);

    QUrl url(ipv6Auth);

    QCOMPARE(url.isValid(), isValid);
    if (url.isValid()) {
        QCOMPARE(url.toString(), ipv6Auth);
        url.setHost(url.host());
        QCOMPARE(url.toString(), ipv6Auth);
    }
};

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

void tst_QUrl::punycode_data()
{
    QTest::addColumn<QString>("original");
    QTest::addColumn<QByteArray>("encoded");

    QTest::newRow("øl") << QString::fromLatin1("øl") << QByteArray("xn--l-4ga");
    QTest::newRow("Bühler") << QString::fromLatin1("Bühler") << QByteArray("xn--Bhler-kva");
    QTest::newRow("räksmörgås") << QString::fromLatin1("räksmörgås") << QByteArray("xn--rksmrgs-5wao1o");
}

void tst_QUrl::punycode()
{
    QFETCH(QString, original);
    QFETCH(QByteArray, encoded);

    QCOMPARE(QUrl::fromPunycode(encoded), original);
    QCOMPARE(QUrl::fromPunycode(QUrl::toPunycode(original)), original);
    QCOMPARE(QUrl::toPunycode(original).constData(), encoded.constData());
}

void tst_QUrl::isRelative_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("trueFalse");

    QTest::newRow("not") << QString::fromLatin1("http://qt.nokia.com") << false;
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

void tst_QUrl::queryItems()
{
    QUrl url;
    QVERIFY(!url.hasQuery());

    QList<QPair<QString, QString> > newItems;
    newItems += qMakePair(QString("2"), QString("b"));
    newItems += qMakePair(QString("1"), QString("a"));
    newItems += qMakePair(QString("3"), QString("c"));
    newItems += qMakePair(QString("4"), QString("a b"));
    newItems += qMakePair(QString("5"), QString("&"));
    newItems += qMakePair(QString("foo bar"), QString("hello world"));
    newItems += qMakePair(QString("foo+bar"), QString("hello+world"));
    newItems += qMakePair(QString("tex"), QString("a + b = c"));
    url.setQueryItems(newItems);
    QVERIFY(url.hasQuery());

    QList<QPair<QString, QString> > setItems = url.queryItems();
    QVERIFY(newItems == setItems);

    url.addQueryItem("1", "z");

    QVERIFY(url.hasQueryItem("1"));
    QCOMPARE(url.queryItemValue("1").toLatin1().constData(), "a");

    url.addQueryItem("1", "zz");

    QStringList expected;
    expected += "a";
    expected += "z";
    expected += "zz";
    QCOMPARE(expected, url.allQueryItemValues("1"));

    url.removeQueryItem("1");
    QCOMPARE(url.allQueryItemValues("1").size(), 2);
    QCOMPARE(url.queryItemValue("1").toLatin1().constData(), "z");

    url.removeAllQueryItems("1");
    QVERIFY(!url.hasQueryItem("1"));

    QCOMPARE(url.queryItemValue("4").toLatin1().constData(), "a b");
    QCOMPARE(url.queryItemValue("5").toLatin1().constData(), "&");
    QCOMPARE(url.queryItemValue("tex").toLatin1().constData(), "a + b = c");
    QCOMPARE(url.queryItemValue("foo bar").toLatin1().constData(), "hello world");
    url.setUrl("http://www.google.com/search?q=a+b");
    QCOMPARE(url.queryItemValue("q"), QString("a+b"));
    url.setUrl("http://www.google.com/search?q=a=b"); // invalid, but should be tolerated
    QCOMPARE(url.queryItemValue("q"), QString("a=b"));
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

void tst_QUrl::hasQueryItem_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("item");
    QTest::addColumn<bool>("trueFalse");

    QTest::newRow("no query items") << "http://www.foo.bar" << "baz" << false;
    QTest::newRow("query item: hello") << "http://www.foo.bar?hello=world" << "hello" << true;
    QTest::newRow("no query item: world") << "http://www.foo.bar?hello=world" << "world" << false;
    QTest::newRow("query item: qt") << "http://www.foo.bar?hello=world&qt=rocks" << "qt" << true;
}

void tst_QUrl::hasQueryItem()
{
    QFETCH(QString, url);
    QFETCH(QString, item);
    QFETCH(bool, trueFalse);

    QCOMPARE(QUrl(url).hasQueryItem(item), trueFalse);
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
        QCOMPARE(url.path(), QString("A=B"));
    }
    {
        QUrl url = QUrl::fromEncoded("http://strange<username>@ok-hostname/", QUrl::StrictMode);
        QVERIFY(!url.isValid());
        // < and > are not allowed in userinfo in strict mode
        url.setUserName("normal_username");
        QVERIFY(url.isValid());
    }
    {
        QUrl url = QUrl::fromEncoded("http://strange<username>@ok-hostname/");
        QVERIFY(url.isValid());
        // < and > are allowed in tolerant mode
    }
    {
        QUrl url = QUrl::fromEncoded("http://strange;hostname/here");
        QVERIFY(!url.isValid());
        QCOMPARE(url.path(), QString("/here"));
        url.setAuthority("strange;hostname");
        QVERIFY(!url.isValid());
        url.setAuthority("foobar@bar");
        QVERIFY(url.isValid());
        url.setAuthority("strange;hostname");
        QVERIFY(!url.isValid());
        QVERIFY(url.errorString().contains("invalid hostname"));
    }

    {
        QUrl url = QUrl::fromEncoded("foo://stuff;1/g");
        QVERIFY(!url.isValid());
        QCOMPARE(url.path(), QString("/g"));
        url.setHost("stuff;1");
        QVERIFY(!url.isValid());
        url.setHost("stuff-1");
        QVERIFY(url.isValid());
        url.setHost("stuff;1");
        QVERIFY(!url.isValid());
        QVERIFY(url.errorString().contains("invalid hostname"));
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
    QTest::newRow("ftp://ftp.qt.nokia.com")
        << QByteArray("ftp://ftp.qt.nokia.com")
        << true << QString("ftp://ftp.qt.nokia.com");
    QTest::newRow("ftp://ftp.qt.nokia.com/")
        << QByteArray("ftp://ftp.qt.nokia.com/")
        << true << QString("ftp://ftp.qt.nokia.com/");
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
    QCOMPARE(url.isValid(), result);
}

void tst_QUrl::invalidSchemeValidator()
{
    // test that if scheme does not start with an ALPHA, QUrl::isValid() returns false
    {
        QUrl url("1http://qt.nokia.com", QUrl::StrictMode);
        QCOMPARE(url.isValid(), false);
    }
    {
        QUrl url("http://qt.nokia.com");
        url.setScheme("111http://qt.nokia.com");
        QCOMPARE(url.isValid(), false);
    }
    {
        QUrl url = QUrl::fromEncoded("1http://qt.nokia.com", QUrl::StrictMode);
        QCOMPARE(url.isValid(), false);
    }

    // non-ALPHA character at other positions in the scheme are ok
    {
        QUrl url("ht111tp://qt.nokia.com", QUrl::StrictMode);
        QVERIFY(url.isValid());
    }
    {
        QUrl url("http://qt.nokia.com");
        url.setScheme("ht123tp://qt.nokia.com");
        QVERIFY(url.isValid());
    }
    {
        QUrl url = QUrl::fromEncoded("ht321tp://qt.nokia.com", QUrl::StrictMode);
        QVERIFY(url.isValid());
    }
}

void tst_QUrl::tolerantParser()
{
    {
        QUrl url("http://www.example.com/path%20with spaces.html");
        QVERIFY(url.isValid());
        QCOMPARE(url.path(), QString("/path with spaces.html"));
        QCOMPARE(url.toEncoded(), QByteArray("http://www.example.com/path%20with%20spaces.html"));
        url.setUrl("http://www.example.com/path%20with spaces.html", QUrl::StrictMode);
        QVERIFY(url.isValid());
        QCOMPARE(url.toEncoded(), QByteArray("http://www.example.com/path%2520with%20spaces.html"));
    }
    {
        QUrl url = QUrl::fromEncoded("http://www.example.com/path%20with spaces.html");
        QVERIFY(url.isValid());
        QCOMPARE(url.path(), QString("/path with spaces.html"));
        url.setEncodedUrl("http://www.example.com/path%20with spaces.html", QUrl::StrictMode);
        QVERIFY(!url.isValid());
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
        QCOMPARE(webkit22616.toEncoded().constData(),
                 "http://example.com/testya.php?browser-info=s:1400x1050x24:f:9.0%20r152:t:%25u0442%25u0435%25u0441%25u0442");
    }

    {
        QUrl url;
        url.setUrl("http://foo.bar/[image][1].jpg");
        QVERIFY(url.isValid());
        QCOMPARE(url.toEncoded(), QByteArray("http://foo.bar/%5Bimage%5D%5B1%5D.jpg"));

        url.setUrl("[].jpg");
        QCOMPARE(url.toEncoded(), QByteArray("%5B%5D.jpg"));

        url.setUrl("/some/[path]/[]");
        QCOMPARE(url.toEncoded(), QByteArray("/some/%5Bpath%5D/%5B%5D"));

        url.setUrl("//[::56:56:56:56:56:56:56]");
        QCOMPARE(url.toEncoded(), QByteArray("//[::56:56:56:56:56:56:56]"));

        url.setUrl("//[::56:56:56:56:56:56:56]#[]");
        QCOMPARE(url.toEncoded(), QByteArray("//[::56:56:56:56:56:56:56]#%5B%5D"));

        url.setUrl("//[::56:56:56:56:56:56:56]?[]");
        QCOMPARE(url.toEncoded(), QByteArray("//[::56:56:56:56:56:56:56]?%5B%5D"));

        url.setUrl("%hello.com/f%");
        QCOMPARE(url.toEncoded(), QByteArray("%25hello.com/f%25"));

        url.setEncodedUrl("http://www.host.com/foo.php?P0=[2006-3-8]");
        QVERIFY(url.isValid());

        url.setEncodedUrl("http://foo.bar/[image][1].jpg");
        QVERIFY(url.isValid());
        QCOMPARE(url.toEncoded(), QByteArray("http://foo.bar/%5Bimage%5D%5B1%5D.jpg"));

        url.setEncodedUrl("[].jpg");
        QCOMPARE(url.toEncoded(), QByteArray("%5B%5D.jpg"));

        url.setEncodedUrl("/some/[path]/[]");
        QCOMPARE(url.toEncoded(), QByteArray("/some/%5Bpath%5D/%5B%5D"));

        url.setEncodedUrl("//[::56:56:56:56:56:56:56]");
        QCOMPARE(url.toEncoded(), QByteArray("//[::56:56:56:56:56:56:56]"));

        url.setEncodedUrl("//[::56:56:56:56:56:56:56]#[]");
        QCOMPARE(url.toEncoded(), QByteArray("//[::56:56:56:56:56:56:56]#%5B%5D"));

        url.setEncodedUrl("//[::56:56:56:56:56:56:56]?[]");
        QCOMPARE(url.toEncoded(), QByteArray("//[::56:56:56:56:56:56:56]?%5B%5D"));

        url.setEncodedUrl("data:text/css,div%20{%20border-right:%20solid;%20}");
        QCOMPARE(url.toEncoded(), QByteArray("data:text/css,div%20%7B%20border-right:%20solid;%20%7D"));
    }

    {
        // task 243557
        QByteArray tsdgeos("http://google.com/c?c=Translation+%C2%BB+trunk|");
        QUrl tsdgeosQUrl;
        tsdgeosQUrl.setEncodedUrl(tsdgeos, QUrl::TolerantMode);
        QVERIFY(tsdgeosQUrl.isValid()); // failed in Qt-4.4, works in Qt-4.5
        QByteArray tsdgeosExpected("http://google.com/c?c=Translation+%C2%BB+trunk%7C");
        //QCOMPARE(tsdgeosQUrl.toEncoded(), tsdgeosExpected); // unusable output from qtestlib...
        QCOMPARE(QString(tsdgeosQUrl.toEncoded()), QString(tsdgeosExpected));
    }

    {
        QUrl url;
        url.setUrl("http://strange<username>@hostname/", QUrl::TolerantMode);
        QVERIFY(url.isValid());
        QCOMPARE(QString(url.toEncoded()), QString("http://strange%3Cusername%3E@hostname/"));
    }
}

void tst_QUrl::correctEncodedMistakes_data()
{
    QTest::addColumn<QByteArray>("encodedUrl");
    QTest::addColumn<bool>("result");
    QTest::addColumn<QString>("toString");
    QTest::addColumn<QByteArray>("toEncoded");

    QTest::newRow("%") << QByteArray("%") << true << QString("%") << QByteArray("%25");
    QTest::newRow("3%") << QByteArray("3%") << true << QString("3%") << QByteArray("3%25");
    QTest::newRow("13%") << QByteArray("13%") << true << QString("13%") << QByteArray("13%25");
    QTest::newRow("13%!") << QByteArray("13%!") << true << QString("13%!") << QByteArray("13%25!");
    QTest::newRow("13%!!") << QByteArray("13%!!") << true << QString("13%!!") << QByteArray("13%25!!");
    QTest::newRow("13%a") << QByteArray("13%a") << true << QString("13%a") << QByteArray("13%25a");
    QTest::newRow("13%az") << QByteArray("13%az") << true << QString("13%az") << QByteArray("13%25az");
    QTest::newRow("13%25") << QByteArray("13%25") << true << QString("13%") << QByteArray("13%25");
}

void tst_QUrl::correctEncodedMistakes()
{
    QFETCH(QByteArray, encodedUrl);
    QFETCH(bool, result);
    QFETCH(QString, toString);
    QFETCH(QByteArray, toEncoded);

    QUrl url = QUrl::fromEncoded(encodedUrl);
    QCOMPARE(url.isValid(), result);
    if (url.isValid()) {
        QCOMPARE(url.toString(), toString);
        QCOMPARE(url.toEncoded(), toEncoded);
    }
}

void tst_QUrl::correctDecodedMistakes_data()
{
    QTest::addColumn<QString>("decodedUrl");
    QTest::addColumn<bool>("result");
    QTest::addColumn<QString>("toString");
    QTest::addColumn<QByteArray>("toEncoded");

    QTest::newRow("%") << QString("%") << true << QString("%") << QByteArray("%25");
    QTest::newRow("3%") << QString("3%") << true << QString("3%") << QByteArray("3%25");
    QTest::newRow("13%") << QString("13%") << true << QString("13%") << QByteArray("13%25");
    QTest::newRow("13%!") << QString("13%!") << true << QString("13%!") << QByteArray("13%25!");
    QTest::newRow("13%!!") << QString("13%!!") << true << QString("13%!!") << QByteArray("13%25!!");
    QTest::newRow("13%a") << QString("13%a") << true << QString("13%a") << QByteArray("13%25a");
    QTest::newRow("13%az") << QString("13%az") << true << QString("13%az") << QByteArray("13%25az");
    QTest::newRow("13%25") << QString("13%25") << true << QString("13%25") << QByteArray("13%2525");
}

void tst_QUrl::correctDecodedMistakes()
{
    QFETCH(QString, decodedUrl);
    QFETCH(bool, result);
    QFETCH(QString, toString);
    QFETCH(QByteArray, toEncoded);

    QUrl url(decodedUrl);
    QCOMPARE(url.isValid(), result);
    if (url.isValid()) {
        QCOMPARE(url.toString(), toString);
        QCOMPARE(url.toEncoded(), toEncoded);
    }
}

void tst_QUrl::idna_testsuite_data()
{
    QTest::addColumn<int>("numchars");
    QTest::addColumn<ushortarray>("unicode");
    QTest::addColumn<QByteArray>("punycode");
    QTest::addColumn<int>("allowunassigned");
    QTest::addColumn<int>("usestd3asciirules");
    QTest::addColumn<int>("toasciirc");
    QTest::addColumn<int>("tounicoderc");

    unsigned short d1[] = { 0x0644, 0x064A, 0x0647, 0x0645, 0x0627, 0x0628, 0x062A, 0x0643,
                            0x0644, 0x0645, 0x0648, 0x0634, 0x0639, 0x0631, 0x0628, 0x064A,
                            0x061F };
    QTest::newRow("Arabic (Egyptian)") << 17 << ushortarray(d1)
                                    << QByteArray(IDNA_ACE_PREFIX "egbpdaj6bu4bxfgehfvwxn")
                                    << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d2[] = { 0x4ED6, 0x4EEC, 0x4E3A, 0x4EC0, 0x4E48, 0x4E0D, 0x8BF4, 0x4E2D,
                            0x6587 };
    QTest::newRow("Chinese (simplified)") << 9 << ushortarray(d2)
                                       << QByteArray(IDNA_ACE_PREFIX "ihqwcrb4cv8a8dqg056pqjye")
                                       << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d3[] = { 0x4ED6, 0x5011, 0x7232, 0x4EC0, 0x9EBD, 0x4E0D, 0x8AAA, 0x4E2D,
                            0x6587 };
    QTest::newRow("Chinese (traditional)") << 9 << ushortarray(d3)
                                        << QByteArray(IDNA_ACE_PREFIX "ihqwctvzc91f659drss3x8bo0yb")
                                        << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d4[] = { 0x0050, 0x0072, 0x006F, 0x010D, 0x0070, 0x0072, 0x006F, 0x0073,
                            0x0074, 0x011B, 0x006E, 0x0065, 0x006D, 0x006C, 0x0075, 0x0076,
                            0x00ED, 0x010D, 0x0065, 0x0073, 0x006B, 0x0079 };
    QTest::newRow("Czech") << 22 << ushortarray(d4)
                        << QByteArray(IDNA_ACE_PREFIX "Proprostnemluvesky-uyb24dma41a")
                        << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d5[] = { 0x05DC, 0x05DE, 0x05D4, 0x05D4, 0x05DD, 0x05E4, 0x05E9, 0x05D5,
                            0x05D8, 0x05DC, 0x05D0, 0x05DE, 0x05D3, 0x05D1, 0x05E8, 0x05D9,
                            0x05DD, 0x05E2, 0x05D1, 0x05E8, 0x05D9, 0x05EA };
    QTest::newRow("Hebrew") << 22 << ushortarray(d5)
                         << QByteArray(IDNA_ACE_PREFIX "4dbcagdahymbxekheh6e0a7fei0b")
                         << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d6[] = { 0x092F, 0x0939, 0x0932, 0x094B, 0x0917, 0x0939, 0x093F, 0x0928,
                            0x094D, 0x0926, 0x0940, 0x0915, 0x094D, 0x092F, 0x094B, 0x0902,
                            0x0928, 0x0939, 0x0940, 0x0902, 0x092C, 0x094B, 0x0932, 0x0938,
                            0x0915, 0x0924, 0x0947, 0x0939, 0x0948, 0x0902 };
    QTest::newRow("Hindi (Devanagari)") << 30 << ushortarray(d6)
                                     << QByteArray(IDNA_ACE_PREFIX "i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd")
                                     << 0 << 0 << IDNA_SUCCESS;

    unsigned short d7[] = { 0x306A, 0x305C, 0x307F, 0x3093, 0x306A, 0x65E5, 0x672C, 0x8A9E,
                            0x3092, 0x8A71, 0x3057, 0x3066, 0x304F, 0x308C, 0x306A, 0x3044,
                            0x306E, 0x304B };
    QTest::newRow("Japanese (kanji and hiragana)") << 18 << ushortarray(d7)
                                                << QByteArray(IDNA_ACE_PREFIX "n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa")
                                                << 0 << 0 << IDNA_SUCCESS;

    unsigned short d8[] = { 0x043F, 0x043E, 0x0447, 0x0435, 0x043C, 0x0443, 0x0436, 0x0435,
                            0x043E, 0x043D, 0x0438, 0x043D, 0x0435, 0x0433, 0x043E, 0x0432,
                            0x043E, 0x0440, 0x044F, 0x0442, 0x043F, 0x043E, 0x0440, 0x0443,
                            0x0441, 0x0441, 0x043A, 0x0438 };
    QTest::newRow("Russian (Cyrillic)") << 28 << ushortarray(d8)
                                     << QByteArray(IDNA_ACE_PREFIX "b1abfaaepdrnnbgefbadotcwatmq2g4l")
                                     << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d9[] = { 0x0050, 0x006F, 0x0072, 0x0071, 0x0075, 0x00E9, 0x006E, 0x006F,
                            0x0070, 0x0075, 0x0065, 0x0064, 0x0065, 0x006E, 0x0073, 0x0069,
                            0x006D, 0x0070, 0x006C, 0x0065, 0x006D, 0x0065, 0x006E, 0x0074,
                            0x0065, 0x0068, 0x0061, 0x0062, 0x006C, 0x0061, 0x0072, 0x0065,
                            0x006E, 0x0045, 0x0073, 0x0070, 0x0061, 0x00F1, 0x006F, 0x006C };
    QTest::newRow("Spanish") << 40 << ushortarray(d9)
                          << QByteArray(IDNA_ACE_PREFIX "PorqunopuedensimplementehablarenEspaol-fmd56a")
                          << 0 << 0 << IDNA_SUCCESS;

    unsigned short d10[] = { 0x0054, 0x1EA1, 0x0069, 0x0073, 0x0061, 0x006F, 0x0068, 0x1ECD,
                             0x006B, 0x0068, 0x00F4, 0x006E, 0x0067, 0x0074, 0x0068, 0x1EC3,
                             0x0063, 0x0068, 0x1EC9, 0x006E, 0x00F3, 0x0069, 0x0074, 0x0069,
                             0x1EBF, 0x006E, 0x0067, 0x0056, 0x0069, 0x1EC7, 0x0074 };
    QTest::newRow("Vietnamese") << 31 << ushortarray(d10)
                             << QByteArray(IDNA_ACE_PREFIX "TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g")
                             << 0 << 0 << IDNA_SUCCESS;

    unsigned short d11[] = { 0x0033, 0x5E74, 0x0042, 0x7D44, 0x91D1, 0x516B, 0x5148, 0x751F };
    QTest::newRow("Japanese") << 8 << ushortarray(d11)
                           << QByteArray(IDNA_ACE_PREFIX "3B-ww4c5e180e575a65lsy2b")
                           << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d12[] = { 0x5B89, 0x5BA4, 0x5948, 0x7F8E, 0x6075, 0x002D, 0x0077, 0x0069,
                             0x0074, 0x0068, 0x002D, 0x0053, 0x0055, 0x0050, 0x0045, 0x0052,
                             0x002D, 0x004D, 0x004F, 0x004E, 0x004B, 0x0045, 0x0059, 0x0053 };
    QTest::newRow("Japanese2") << 24 << ushortarray(d12)
                            << QByteArray(IDNA_ACE_PREFIX "-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n")
                            << 0 << 0 << IDNA_SUCCESS;

    unsigned short d13[] = { 0x0048, 0x0065, 0x006C, 0x006C, 0x006F, 0x002D, 0x0041, 0x006E,
                             0x006F, 0x0074, 0x0068, 0x0065, 0x0072, 0x002D, 0x0057, 0x0061,
                             0x0079, 0x002D, 0x305D, 0x308C, 0x305E, 0x308C, 0x306E, 0x5834,
                             0x6240 };
    QTest::newRow("Japanese3") << 25 << ushortarray(d13)
                            << QByteArray(IDNA_ACE_PREFIX "Hello-Another-Way--fc4qua05auwb3674vfr0b")
                            << 0 << 0 << IDNA_SUCCESS;

    unsigned short d14[] = { 0x3072, 0x3068, 0x3064, 0x5C4B, 0x6839, 0x306E, 0x4E0B, 0x0032 };
    QTest::newRow("Japanese4") << 8 << ushortarray(d14)
                            << QByteArray(IDNA_ACE_PREFIX "2-u9tlzr9756bt3uc0v")
                            << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d15[] = { 0x004D, 0x0061, 0x006A, 0x0069, 0x3067, 0x004B, 0x006F, 0x0069,
                             0x3059, 0x308B, 0x0035, 0x79D2, 0x524D };
    QTest::newRow("Japanese5") << 13 << ushortarray(d15)
                            << QByteArray(IDNA_ACE_PREFIX "MajiKoi5-783gue6qz075azm5e")
                            << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d16[] = { 0x30D1, 0x30D5, 0x30A3, 0x30FC, 0x0064, 0x0065, 0x30EB, 0x30F3, 0x30D0 };
    QTest::newRow("Japanese6") << 9 << ushortarray(d16)
                            << QByteArray(IDNA_ACE_PREFIX "de-jg4avhby1noc0d")
                            << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d17[] = { 0x305D, 0x306E, 0x30B9, 0x30D4, 0x30FC, 0x30C9, 0x3067 };
    QTest::newRow("Japanese7") << 7 << ushortarray(d17)
                            << QByteArray(IDNA_ACE_PREFIX "d9juau41awczczp")
                            << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d18[] = { 0x03b5, 0x03bb, 0x03bb, 0x03b7, 0x03bd, 0x03b9, 0x03ba, 0x03ac };
    QTest::newRow("Greek") << 8 << ushortarray(d18)
                        << QByteArray(IDNA_ACE_PREFIX "hxargifdar")
                        << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d19[] = { 0x0062, 0x006f, 0x006e, 0x0121, 0x0075, 0x0073, 0x0061, 0x0127,
                             0x0127, 0x0061 };
    QTest::newRow("Maltese (Malti)") << 10 << ushortarray(d19)
                                  << QByteArray(IDNA_ACE_PREFIX "bonusaa-5bb1da")
                                  << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;

    unsigned short d20[] = {0x043f, 0x043e, 0x0447, 0x0435, 0x043c, 0x0443, 0x0436, 0x0435,
                            0x043e, 0x043d, 0x0438, 0x043d, 0x0435, 0x0433, 0x043e, 0x0432,
                            0x043e, 0x0440, 0x044f, 0x0442, 0x043f, 0x043e, 0x0440, 0x0443,
                            0x0441, 0x0441, 0x043a, 0x0438 };
    QTest::newRow("Russian (Cyrillic)") << 28 << ushortarray(d20)
                                     << QByteArray(IDNA_ACE_PREFIX "b1abfaaepdrnnbgefbadotcwatmq2g4l")
                                     << 0 << 0 << IDNA_SUCCESS << IDNA_SUCCESS;
}

void tst_QUrl::idna_testsuite()
{
    QFETCH(int, numchars);
    QFETCH(ushortarray, unicode);
    QFETCH(QByteArray, punycode);

    QString s = QString::fromUtf16(unicode.points, numchars);
    QCOMPARE(punycode, QUrl::toPunycode(s));
}

void tst_QUrl::nameprep_testsuite_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QString>("profile");
    QTest::addColumn<int>("flags");
    QTest::addColumn<int>("rc");

    QTest::newRow("Map to nothing")
        << QString::fromUtf8("foo\xC2\xAD\xCD\x8F\xE1\xA0\x86\xE1\xA0\x8B"
                             "bar""\xE2\x80\x8B\xE2\x81\xA0""baz\xEF\xB8\x80\xEF\xB8\x88"
                             "\xEF\xB8\x8F\xEF\xBB\xBF")
        << QString::fromUtf8("foobarbaz")
        << QString() << 0 << 0;

    QTest::newRow("Case folding ASCII U+0043 U+0041 U+0046 U+0045")
        << QString::fromUtf8("CAFE")
        << QString::fromUtf8("cafe")
        << QString() << 0 << 0;

    QTest::newRow("Case folding 8bit U+00DF (german sharp s)")
        << QString::fromUtf8("\xC3\x9F")
        << QString("ss")
        << QString() << 0 << 0;

    QTest::newRow("Case folding U+0130 (turkish capital I with dot)")
        << QString::fromUtf8("\xC4\xB0")
        << QString::fromUtf8("i\xcc\x87")
        << QString() << 0 << 0;

    QTest::newRow("Case folding multibyte U+0143 U+037A")
        << QString::fromUtf8("\xC5\x83\xCD\xBA")
        << QString::fromUtf8("\xC5\x84 \xCE\xB9")
        << QString() << 0 << 0;

    QTest::newRow("Case folding U+2121 U+33C6 U+1D7BB")
        << QString::fromUtf8("\xE2\x84\xA1\xE3\x8F\x86\xF0\x9D\x9E\xBB")
        << QString::fromUtf8("telc\xE2\x88\x95""kg\xCF\x83")
        << QString() << 0 << 0;

    QTest::newRow("Normalization of U+006a U+030c U+00A0 U+00AA")
        << QString::fromUtf8("\x6A\xCC\x8C\xC2\xA0\xC2\xAA")
        << QString::fromUtf8("\xC7\xB0 a")
        << QString() << 0 << 0;

    QTest::newRow("Case folding U+1FB7 and normalization")
        << QString::fromUtf8("\xE1\xBE\xB7")
        << QString::fromUtf8("\xE1\xBE\xB6\xCE\xB9")
        << QString() << 0 << 0;

    QTest::newRow("Self-reverting case folding U+01F0 and normalization")
//        << QString::fromUtf8("\xC7\xF0") ### typo in the original testsuite
        << QString::fromUtf8("\xC7\xB0")
        << QString::fromUtf8("\xC7\xB0")
        << QString() << 0 << 0;

    QTest::newRow("Self-reverting case folding U+0390 and normalization")
        << QString::fromUtf8("\xCE\x90")
        << QString::fromUtf8("\xCE\x90")
        << QString() << 0 << 0;

    QTest::newRow("Self-reverting case folding U+03B0 and normalization")
        << QString::fromUtf8("\xCE\xB0")
        << QString::fromUtf8("\xCE\xB0")
        << QString() << 0 << 0;

    QTest::newRow("Self-reverting case folding U+1E96 and normalization")
        << QString::fromUtf8("\xE1\xBA\x96")
        << QString::fromUtf8("\xE1\xBA\x96")
        << QString() << 0 << 0;

    QTest::newRow("Self-reverting case folding U+1F56 and normalization")
        << QString::fromUtf8("\xE1\xBD\x96")
        << QString::fromUtf8("\xE1\xBD\x96")
        << QString() << 0 << 0;

    QTest::newRow("ASCII space character U+0020")
        << QString::fromUtf8("\x20")
        << QString::fromUtf8("\x20")
        << QString() << 0 << 0;

    QTest::newRow("Non-ASCII 8bit space character U+00A0")
        << QString::fromUtf8("\xC2\xA0")
        << QString::fromUtf8("\x20")
        << QString() << 0 << 0;

    QTest::newRow("Non-ASCII multibyte space character U+1680")
        << QString::fromUtf8("\xE1\x9A\x80")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-ASCII multibyte space character U+2000")
        << QString::fromUtf8("\xE2\x80\x80")
        << QString::fromUtf8("\x20")
        << QString() << 0 << 0;

    QTest::newRow("Zero Width Space U+200b")
        << QString::fromUtf8("\xE2\x80\x8b")
        << QString()
        << QString() << 0 << 0;

    QTest::newRow("Non-ASCII multibyte space character U+3000")
        << QString::fromUtf8("\xE3\x80\x80")
        << QString::fromUtf8("\x20")
        << QString() << 0 << 0;

    QTest::newRow("ASCII control characters U+0010 U+007F")
        << QString::fromUtf8("\x10\x7F")
        << QString::fromUtf8("\x10\x7F")
        << QString() << 0 << 0;

    QTest::newRow("Non-ASCII 8bit control character U+0085")
        << QString::fromUtf8("\xC2\x85")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-ASCII multibyte control character U+180E")
        << QString::fromUtf8("\xE1\xA0\x8E")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Zero Width No-Break Space U+FEFF")
        << QString::fromUtf8("\xEF\xBB\xBF")
        << QString()
        << QString() << 0 << 0;

    QTest::newRow("Non-ASCII control character U+1D175")
        << QString::fromUtf8("\xF0\x9D\x85\xB5")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Plane 0 private use character U+F123")
        << QString::fromUtf8("\xEF\x84\xA3")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Plane 15 private use character U+F1234")
        << QString::fromUtf8("\xF3\xB1\x88\xB4")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Plane 16 private use character U+10F234")
        << QString::fromUtf8("\xF4\x8F\x88\xB4")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-character code point U+8FFFE")
        << QString::fromUtf8("\xF2\x8F\xBF\xBE")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-character code point U+10FFFF")
        << QString::fromUtf8("\xF4\x8F\xBF\xBF")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Surrogate code U+DF42")
        << QString::fromUtf8("\xED\xBD\x82")
        << QString()
        << QString("Nameprep") << 0 <<  STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Non-plain text character U+FFFD")
        << QString::fromUtf8("\xEF\xBF\xBD")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Ideographic description character U+2FF5")
        << QString::fromUtf8("\xE2\xBF\xB5")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Display property character U+0341")
        << QString::fromUtf8("\xCD\x81")
        << QString::fromUtf8("\xCC\x81")
        << QString() << 0 << 0;

    QTest::newRow("Left-to-right mark U+200E")
        << QString::fromUtf8("\xE2\x80\x8E")
        << QString::fromUtf8("\xCC\x81")
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Deprecated U+202A")
        << QString::fromUtf8("\xE2\x80\xAA")
        << QString::fromUtf8("\xCC\x81")
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Language tagging character U+E0001")
        << QString::fromUtf8("\xF3\xA0\x80\x81")
        << QString::fromUtf8("\xCC\x81")
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Language tagging character U+E0042")
        << QString::fromUtf8("\xF3\xA0\x81\x82")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_CONTAINS_PROHIBITED;

    QTest::newRow("Bidi: RandALCat character U+05BE and LCat characters")
        << QString::fromUtf8("foo\xD6\xBE""bar")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_BIDI_BOTH_L_AND_RAL;

    QTest::newRow("Bidi: RandALCat character U+FD50 and LCat characters")
        << QString::fromUtf8("foo\xEF\xB5\x90""bar")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_BIDI_BOTH_L_AND_RAL;

    QTest::newRow("Bidi: RandALCat character U+FB38 and LCat characters")
        << QString::fromUtf8("foo\xEF\xB9\xB6""bar")
        << QString::fromUtf8("foo \xd9\x8e""bar")
        << QString() << 0 << 0;

    QTest::newRow("Bidi: RandALCat without trailing RandALCat U+0627 U+0031")
        << QString::fromUtf8("\xD8\xA7\x31")
        << QString()
        << QString("Nameprep") << 0 << STRINGPREP_BIDI_LEADTRAIL_NOT_RAL;

    QTest::newRow("Bidi: RandALCat character U+0627 U+0031 U+0628")
        << QString::fromUtf8("\xD8\xA7\x31\xD8\xA8")
        << QString::fromUtf8("\xD8\xA7\x31\xD8\xA8")
        << QString() << 0 << 0;

    QTest::newRow("Unassigned code point U+E0002")
        << QString::fromUtf8("\xF3\xA0\x80\x82")
        << QString()
        << QString("Nameprep") << STRINGPREP_NO_UNASSIGNED << STRINGPREP_CONTAINS_UNASSIGNED;

    QTest::newRow("Larger test (shrinking)")
        << QString::fromUtf8("X\xC2\xAD\xC3\x9F\xC4\xB0\xE2\x84\xA1\x6a\xcc\x8c\xc2\xa0\xc2"
                             "\xaa\xce\xb0\xe2\x80\x80")
        << QString::fromUtf8("xssi\xcc\x87""tel\xc7\xb0 a\xce\xb0 ")
        << QString("Nameprep") << 0 << 0;

    QTest::newRow("Larger test (expanding)")
        << QString::fromUtf8("X\xC3\x9F\xe3\x8c\x96\xC4\xB0\xE2\x84\xA1\xE2\x92\x9F\xE3\x8c\x80")
        << QString::fromUtf8("xss\xe3\x82\xad\xe3\x83\xad\xe3\x83\xa1\xe3\x83\xbc\xe3\x83\x88"
                             "\xe3\x83\xab""i\xcc\x87""tel\x28""d\x29\xe3\x82\xa2\xe3\x83\x91"
                             "\xe3\x83\xbc\xe3\x83\x88")
        << QString() << 0 << 0;
}

#ifdef QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
extern void qt_nameprep(QString *source, int from);
extern bool qt_check_std3rules(const QChar *, int);
QT_END_NAMESPACE
#endif

void tst_QUrl::nameprep_testsuite()
{
#ifdef QT_BUILD_INTERNAL
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QString, profile);

    QEXPECT_FAIL("Left-to-right mark U+200E",
                 "Investigate further", Continue);
    QEXPECT_FAIL("Deprecated U+202A",
                 "Investigate further", Continue);
    QEXPECT_FAIL("Language tagging character U+E0001",
                 "Investigate further", Continue);
    qt_nameprep(&in, 0);
    QCOMPARE(in, out);
#endif
}

void tst_QUrl::nameprep_highcodes_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<QString>("profile");
    QTest::addColumn<int>("flags");
    QTest::addColumn<int>("rc");

    {
        QChar st[] = { '-', 0xd801, 0xdc1d, 'a' };
        QChar se[] = { '-', 0xd801, 0xdc45, 'a' };
        QTest::newRow("highcodes (U+1041D)")
            << QString(st, sizeof(st)/sizeof(st[0]))
            << QString(se, sizeof(se)/sizeof(se[0]))
            << QString() << 0 << 0;
    }
    {
        QChar st[] = { 0x011C, 0xd835, 0xdf6e, 0x0110 };
        QChar se[] = { 0x011D, 0x03C9, 0x0111 };
        QTest::newRow("highcodes (U+1D76E)")
            << QString(st, sizeof(st)/sizeof(st[0]))
            << QString(se, sizeof(se)/sizeof(se[0]))
            << QString() << 0 << 0;
    }
    {
        QChar st[] = { 'D', 0xdb40, 0xdc20, 'o', 0xd834, 0xdd7a, '\'', 0x2060, 'h' };
        QChar se[] = { 'd', 'o', '\'', 'h' };
        QTest::newRow("highcodes (D, U+E0020, o, U+1D17A, ', U+2060, h)")
            << QString(st, sizeof(st)/sizeof(st[0]))
            << QString(se, sizeof(se)/sizeof(se[0]))
            << QString() << 0 << 0;
    }
}

void tst_QUrl::nameprep_highcodes()
{
#ifdef QT_BUILD_INTERNAL
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(QString, profile);

    qt_nameprep(&in, 0);
    QCOMPARE(in, out);
#endif
}

void tst_QUrl::ace_testsuite_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("toace");
    QTest::addColumn<QString>("fromace");
    QTest::addColumn<QString>("unicode");

    QTest::newRow("ascii-lower") << "fluke" << "fluke" << "fluke" << "fluke";
    QTest::newRow("ascii-mixed") << "FLuke" << "fluke" << "fluke" << "fluke";
    QTest::newRow("ascii-upper") << "FLUKE" << "fluke" << "fluke" << "fluke";

    QTest::newRow("asciifolded") << QString::fromLatin1("stra\337e") << "strasse" << "." << "strasse";
    QTest::newRow("asciifolded-dotcom") << QString::fromLatin1("stra\337e.example.com") << "strasse.example.com" << "." << "strasse.example.com";
    QTest::newRow("greek-mu") << QString::fromLatin1("\265V")
                              <<"xn--v-lmb"
                              << "."
                              << QString::fromUtf8("\316\274v");

    QTest::newRow("non-ascii-lower") << QString::fromLatin1("alqualond\353")
                                     << "xn--alqualond-34a"
                                     << "."
                                     << QString::fromLatin1("alqualond\353");
    QTest::newRow("non-ascii-mixed") << QString::fromLatin1("Alqualond\353")
                                     << "xn--alqualond-34a"
                                     << "."
                                     << QString::fromLatin1("alqualond\353");
    QTest::newRow("non-ascii-upper") << QString::fromLatin1("ALQUALOND\313")
                                     << "xn--alqualond-34a"
                                     << "."
                                     << QString::fromLatin1("alqualond\353");

    QTest::newRow("idn-lower") << "xn--alqualond-34a" << "xn--alqualond-34a"
                               << QString::fromLatin1("alqualond\353")
                               << QString::fromLatin1("alqualond\353");
    QTest::newRow("idn-mixed") << "Xn--alqualond-34a" << "xn--alqualond-34a"
                               << QString::fromLatin1("alqualond\353")
                               << QString::fromLatin1("alqualond\353");
    QTest::newRow("idn-mixed2") << "XN--alqualond-34a" << "xn--alqualond-34a"
                                << QString::fromLatin1("alqualond\353")
                                << QString::fromLatin1("alqualond\353");
    QTest::newRow("idn-mixed3") << "xn--ALQUALOND-34a" << "xn--alqualond-34a"
                                << QString::fromLatin1("alqualond\353")
                                << QString::fromLatin1("alqualond\353");
    QTest::newRow("idn-mixed4") << "xn--alqualond-34A" << "xn--alqualond-34a"
                                << QString::fromLatin1("alqualond\353")
                                << QString::fromLatin1("alqualond\353");
    QTest::newRow("idn-upper") << "XN--ALQUALOND-34A" << "xn--alqualond-34a"
                               << QString::fromLatin1("alqualond\353")
                               << QString::fromLatin1("alqualond\353");

    QTest::newRow("separator-3002") << QString::fromUtf8("example\343\200\202com")
                                    << "example.com" << "." << "example.com";

    QString egyptianIDN =
        QString::fromUtf8("\331\210\330\262\330\247\330\261\330\251\055\330\247\331\204\330"
                          "\243\330\252\330\265\330\247\331\204\330\247\330\252.\331\205"
                          "\330\265\330\261");
    QTest::newRow("egyptian-tld-ace")
        << "xn----rmckbbajlc6dj7bxne2c.xn--wgbh1c"
        << "xn----rmckbbajlc6dj7bxne2c.xn--wgbh1c"
        << "."
        << egyptianIDN;
    QTest::newRow("egyptian-tld-unicode")
        << egyptianIDN
        << "xn----rmckbbajlc6dj7bxne2c.xn--wgbh1c"
        << "."
        << egyptianIDN;
    QTest::newRow("egyptian-tld-mix1")
        << QString::fromUtf8("\331\210\330\262\330\247\330\261\330\251\055\330\247\331\204\330"
                             "\243\330\252\330\265\330\247\331\204\330\247\330\252.xn--wgbh1c")
        << "xn----rmckbbajlc6dj7bxne2c.xn--wgbh1c"
        << "."
        << egyptianIDN;
    QTest::newRow("egyptian-tld-mix2")
        << QString::fromUtf8("xn----rmckbbajlc6dj7bxne2c.\331\205\330\265\330\261")
        << "xn----rmckbbajlc6dj7bxne2c.xn--wgbh1c"
        << "."
        << egyptianIDN;
}

void tst_QUrl::ace_testsuite()
{
    static const char canonsuffix[] = ".troll.no";
    QFETCH(QString, in);
    QFETCH(QString, toace);
    QFETCH(QString, fromace);
    QFETCH(QString, unicode);

    const char *suffix = canonsuffix;
    if (toace.contains('.'))
        suffix = 0;

    QString domain = in + suffix;
    QCOMPARE(QString::fromLatin1(QUrl::toAce(domain)), toace + suffix);
    if (fromace != ".")
        QCOMPARE(QUrl::fromAce(domain.toLatin1()), fromace + suffix);
    QCOMPARE(QUrl::fromAce(QUrl::toAce(domain)), unicode + suffix);

    domain = in + (suffix ? ".troll.No" : "");
    QCOMPARE(QString::fromLatin1(QUrl::toAce(domain)), toace + suffix);
    if (fromace != ".")
        QCOMPARE(QUrl::fromAce(domain.toLatin1()), fromace + suffix);
    QCOMPARE(QUrl::fromAce(QUrl::toAce(domain)), unicode + suffix);

    domain = in + (suffix ? ".troll.NO" : "");
    QCOMPARE(QString::fromLatin1(QUrl::toAce(domain)), toace + suffix);
    if (fromace != ".")
        QCOMPARE(QUrl::fromAce(domain.toLatin1()), fromace + suffix);
    QCOMPARE(QUrl::fromAce(QUrl::toAce(domain)), unicode + suffix);
}

void tst_QUrl::std3violations_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<bool>("validUrl");

    QTest::newRow("too-long") << "this-domain-is-far-too-long-for-its-own-good-and-should-have-been-limited-to-63-chars" << false;
    QTest::newRow("dash-begin") << "-x-foo" << false;
    QTest::newRow("dash-end") << "x-foo-" << false;
    QTest::newRow("dash-begin-end") << "-foo-" << false;

    QTest::newRow("control") << "\033foo" << false;
    QTest::newRow("bang") << "foo!" << false;
    QTest::newRow("plus") << "foo+bar" << false;
    QTest::newRow("dot") << "foo.bar";
    QTest::newRow("startingdot") << ".bar" << false;
    QTest::newRow("startingdot2") << ".example.com" << false;
    QTest::newRow("slash") << "foo/bar" << true;
    QTest::newRow("colon") << "foo:80" << true;
    QTest::newRow("question") << "foo?bar" << true;
    QTest::newRow("at") << "foo@bar" << true;
    QTest::newRow("backslash") << "foo\\bar" << false;

    // these characters are transformed by NFKC to non-LDH characters
    QTest::newRow("dot-like") << QString::fromUtf8("foo\342\200\244bar") << false;  // U+2024 ONE DOT LEADER
    QTest::newRow("slash-like") << QString::fromUtf8("foo\357\274\217bar") << false;    // U+FF0F FULLWIDTH SOLIDUS

    // The following should be invalid but isn't
    // the DIVISON SLASH doesn't case-fold to a slash
    // is this a problem with RFC 3490?
    //QTest::newRow("slash-like2") << QString::fromUtf8("foo\342\210\225bar") << false; // U+2215 DIVISION SLASH
}

void tst_QUrl::std3violations()
{
    QFETCH(QString, source);

#ifdef QT_BUILD_INTERNAL
    {
        QString prepped = source;
        qt_nameprep(&prepped, 0);
        QVERIFY(!qt_check_std3rules(prepped.constData(), prepped.length()));
    }
#endif

    if (source.contains('.'))
        return; // this test ends here

    QUrl url;
    url.setHost(source);
    QVERIFY(url.host().isEmpty());

    QFETCH(bool, validUrl);
    if (validUrl)
        return;  // test ends here for these cases

    url = QUrl("http://" + source + "/some/path");
    QVERIFY(!url.isValid());
}

void tst_QUrl::std3deviations_data()
{
    QTest::addColumn<QString>("source");

    QTest::newRow("ending-dot") << "example.com.";
    QTest::newRow("ending-dot3002") << QString("example.com") + QChar(0x3002);
    QTest::newRow("underline") << "foo_bar";  //QTBUG-7434
}

void tst_QUrl::std3deviations()
{
    QFETCH(QString, source);
    QVERIFY(!QUrl::toAce(source).isEmpty());

    QUrl url;
    url.setHost(source);
    QVERIFY(!url.host().isEmpty());
}

void tst_QUrl::tldRestrictions_data()
{
    QTest::addColumn<QString>("tld");
    QTest::addColumn<bool>("encode");

    // current whitelist
    QTest::newRow("ac")  << QString("ac")  << true;
    QTest::newRow("at") << QString("at") << true;
    QTest::newRow("br") << QString("br") << true;
    QTest::newRow("cat")  << QString("cat")  << true;
    QTest::newRow("ch")  << QString("ch")  << true;
    QTest::newRow("cl")  << QString("cl")  << true;
    QTest::newRow("cn") << QString("cn") << true;
    QTest::newRow("de")  << QString("de")  << true;
    QTest::newRow("dk") << QString("dk") << true;
    QTest::newRow("fi") << QString("fi") << true;
    QTest::newRow("hu") << QString("hu") << true;
    QTest::newRow("info")  << QString("info")  << true;
    QTest::newRow("io") << QString("io") << true;
    QTest::newRow("jp") << QString("jp") << true;
    QTest::newRow("kr") << QString("kr") << true;
    QTest::newRow("li")  << QString("li")  << true;
    QTest::newRow("lt") << QString("lt") << true;
    QTest::newRow("museum") << QString("museum") << true;
    QTest::newRow("no") << QString("no") << true;
    QTest::newRow("se")  << QString("se")  << true;
    QTest::newRow("sh") << QString("sh") << true;
    QTest::newRow("th")  << QString("th")  << true;
    QTest::newRow("tm")  << QString("tm")  << true;
    QTest::newRow("tw") << QString("tw") << true;
    QTest::newRow("vn") << QString("vn") << true;

    // known blacklists:
    QTest::newRow("com") << QString("com") << false;
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
        url.setEncodedQuery("abc=def");
        QVERIFY(url.hasQuery());
        QCOMPARE(QString(url.encodedQuery()), QString(QLatin1String("abc=def")));
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz?abc=def")));

        // remove encodedQuery
        url.setEncodedQuery(0);
        QVERIFY(!url.hasQuery());
        QVERIFY(url.encodedQuery().isNull());
        QCOMPARE(url.toString(), QString(QLatin1String("http://www.foo.bar/baz")));

        // add empty encodedQuery
        url.setEncodedQuery("");
        QVERIFY(url.hasQuery());
        QVERIFY(url.encodedQuery().isEmpty());
        QVERIFY(!url.encodedQuery().isNull());
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
    QTest::newRow("empty") << BA("http://www.kde.org") << BA("") << BA("http://www.kde.org#");
    QTest::newRow("basic test") << BA("http://www.kde.org") << BA("abc") << BA("http://www.kde.org#abc");
    QTest::newRow("initial url has fragment") << BA("http://www.kde.org#old") << BA("new") << BA("http://www.kde.org#new");
    QTest::newRow("encoded fragment") << BA("http://www.kde.org") << BA("a%20c") << BA("http://www.kde.org#a%20c");
    QTest::newRow("with #") << BA("http://www.kde.org") << BA("a#b") << BA("http://www.kde.org#a#b");
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
    QVERIFY(u.hasFragment());
    QCOMPARE(QString::fromLatin1(u.toEncoded()), QString::fromLatin1(expected));
}

void tst_QUrl::fromEncoded()
{
    QUrl qurl2 = QUrl::fromEncoded("print:/specials/Print%20To%20File%20(PDF%252FAcrobat)", QUrl::TolerantMode);
    QCOMPARE(qurl2.path(), QString::fromLatin1("/specials/Print To File (PDF%2FAcrobat)"));
    QCOMPARE(QFileInfo(qurl2.path()).fileName(), QString::fromLatin1("Print To File (PDF%2FAcrobat)"));
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

void tst_QUrl::stripTrailingSlash()
{
    QUrl u1( "ftp://ftp.de.kde.org/dir" );
    QUrl u2( "ftp://ftp.de.kde.org/dir/" );
    QUrl::FormattingOptions options = QUrl::None;
    options |= QUrl::StripTrailingSlash;
    QString str1 = u1.toString(options);
    QString str2 = u2.toString(options);
    QCOMPARE( str1, u1.toString() );
    QCOMPARE( str2, u1.toString() );
    bool same = str1 == str2;
    QVERIFY( same );
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

    // numeric hostnames
    QTest::newRow("http://123/") << QString("http://123/") << QString("123");
    QTest::newRow("http://456/") << QString("http://456/") << QString("456");
    QTest::newRow("http://1000/") << QString("http://1000/") << QString("1000");

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
    QTest::newRow("ipv6-literal-v4compat-2") << QString("http://[1000::ffff:127.128.129.1]")
                                             << QString("1000::ffff:127.128.129.1");
    QTest::newRow("long-ipv6-literal-v4compat") << QString("http://[fec0:8000::8002:1000:ffff:200.100.50.250]")
                                                << QString("fec0:8000::8002:1000:ffff:200.100.50.250");
    QTest::newRow("longer-ipv6-literal-v4compat") << QString("http://[fec0:8000:4000:8002:1000:ffff:200.100.50.250]")
                                                  << QString("fec0:8000:4000:8002:1000:ffff:200.100.50.250");

#if 0
    // this is actually invalid
    QTest::newRow("mac-literal") << QString("obex://[00:30:1b:b7:21:fb]")
                                 << QString("00:30:1b:b7:21:fb");
#endif

    // normal hostnames
    QTest::newRow("normal") << QString("http://intern") << QString("intern");
    QTest::newRow("normal2") << QString("http://qt.nokia.com") << QString("qt.nokia.com");

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
        url.setPort(80);
        QCOMPARE(url.port(), 80);
        QCOMPARE(url.toString(), QString::fromLatin1("//:80"));
        url.setPort(-1);
        QCOMPARE(url.port(), -1);
        QVERIFY(url.toString().isEmpty());
        url.setPort(80);
        QTest::ignoreMessage(QtWarningMsg, "QUrl::setPort: Out of range");
        url.setPort(65536);
        QCOMPARE(url.port(), -1);
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

void tst_QUrl::errorString()
{
    QUrl u = QUrl::fromEncoded("http://strange<username>@bad_hostname/", QUrl::StrictMode);
    QVERIFY(!u.isValid());
    QString errorString = "Invalid URL \"http://strange<username>@bad_hostname/\": "
                          "error at position 14: expected end of URL, but found '<'";
    QCOMPARE(u.errorString(), errorString);

    QUrl v;
    errorString = "Invalid URL \"\": ";
    QCOMPARE(v.errorString(), errorString);
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
    QTest::newRow("file-slash") << "http://foo/abc%2f_def";

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

    // QUrl's tolerant parser should already handle this
    QTest::newRow("not-encoded-0") << "http://example.org/test page.html" << QUrl::fromEncoded("http://example.org/test%20page.html");

    // Make sure the :80, i.e. port doesn't screw anything up
    QUrl portUrl("http://example.org");
    portUrl.setPort(80);
    QTest::newRow("port-0") << "example.org:80" << portUrl;
    QTest::newRow("port-1") << "http://example.org:80" << portUrl;
    portUrl.setPath("path");
    QTest::newRow("port-1") << "example.org:80/path" << portUrl;
    QTest::newRow("port-1") << "http://example.org:80/path" << portUrl;

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

    // FYI: The scheme in the resulting url user
    QUrl authUrl("user:pass@domain.com");
    QTest::newRow("misc-1") << "user:pass@domain.com" << authUrl;
}

void tst_QUrl::fromUserInput()
{
    QFETCH(QString, string);
    QFETCH(QUrl, guessUrlFromString);

    QUrl url = QUrl::fromUserInput(string);
    QCOMPARE(url, guessUrlFromString);
}

void tst_QUrl::task_199967()
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

void tst_QUrl::task_240612()
{
    QUrl url;
    url.setEncodedPath("test.txt");
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
}

void tst_QUrl::taskQTBUG_6962()
{
    //bug 6962: empty authority ignored by setAuthority
    QUrl url("http://example.com/something");
    url.setAuthority(QString());
    QCOMPARE(url.authority(), QString());
}

void tst_QUrl::taskQTBUG_8701()
{
    //bug 8701: foo:///bar mangled to foo:/bar
    QString foo_triple_bar("foo:///bar"), foo_uni_bar("foo:/bar");

    QCOMPARE(foo_triple_bar, QUrl(foo_triple_bar).toString());
    QCOMPARE(foo_uni_bar, QUrl(foo_uni_bar).toString());

    QCOMPARE(foo_triple_bar, QUrl(foo_triple_bar, QUrl::StrictMode).toString()); // fails
    QCOMPARE(foo_uni_bar, QUrl(foo_uni_bar, QUrl::StrictMode).toString());
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
}

void tst_QUrl::effectiveTLDs()
{
    QFETCH(QUrl, domain);
    QFETCH(QString, TLD);
    QCOMPARE(domain.topLevelDomain(), TLD);
}

QTEST_MAIN(tst_QUrl)
#include "tst_qurl.moc"
