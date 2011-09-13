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
#include <qregexp.h>
#include <qstringlist.h>





//TESTED_CLASS=
//TESTED_FILES=

class tst_QStringList : public QObject
{
    Q_OBJECT

public:
    tst_QStringList();
    virtual ~tst_QStringList();

public slots:
    void init();
    void cleanup();
private slots:
    void filter();
    void replaceInStrings();
    void removeDuplicates();
    void removeDuplicates_data();
    void contains();
    void indexOf();
    void lastIndexOf();

    void indexOf_regExp();
    void lastIndexOf_regExp();

    void streamingOperator();
    void join() const;
    void join_data() const;
    void joinEmptiness() const;

    void initializeList() const;
};

extern const char email[];

tst_QStringList::tst_QStringList()
{
}

tst_QStringList::~tst_QStringList()
{
}

void tst_QStringList::init()
{
}

void tst_QStringList::cleanup()
{
}

void tst_QStringList::indexOf_regExp()
{
    QStringList list;
    list << "harald" << "trond" << "vohi" << "harald";

    QRegExp re(".*o.*");

    QCOMPARE(list.indexOf(re), 1);
    QCOMPARE(list.indexOf(re, 2), 2);
    QCOMPARE(list.indexOf(re, 3), -1);

    QCOMPARE(list.indexOf(QRegExp(".*x.*")), -1);
    QCOMPARE(list.indexOf(re, -1), -1);
    QCOMPARE(list.indexOf(re, -3), 1);
    QCOMPARE(list.indexOf(re, -9999), 1);
    QCOMPARE(list.indexOf(re, 9999), -1);
}

void tst_QStringList::lastIndexOf_regExp()
{
    QStringList list;
    list << "harald" << "trond" << "vohi" << "harald";

    QRegExp re(".*o.*");

    QCOMPARE(list.lastIndexOf(re), 2);
    QCOMPARE(list.lastIndexOf(re, 2), 2);
    QCOMPARE(list.lastIndexOf(re, 1), 1);

    QCOMPARE(list.lastIndexOf(QRegExp(".*x.*")), -1);
    QCOMPARE(list.lastIndexOf(re, -1), 2);
    QCOMPARE(list.lastIndexOf(re, -3), 1);
    QCOMPARE(list.lastIndexOf(re, -9999), -1);
    QCOMPARE(list.lastIndexOf(re, 9999), 2);
}

void tst_QStringList::indexOf()
{
    QStringList list;
    list << "harald" << "trond" << "vohi" << "harald";

    QCOMPARE(list.indexOf("harald"), 0);
    QCOMPARE(list.indexOf("trond"), 1);
    QCOMPARE(list.indexOf("vohi"), 2);
    QCOMPARE(list.indexOf("harald", 1), 3);

    QCOMPARE(list.indexOf("hans"), -1);
    QCOMPARE(list.indexOf("trond", 2), -1);
    QCOMPARE(list.indexOf("harald", -1), 3);
    QCOMPARE(list.indexOf("vohi", -3), 2);
}

void tst_QStringList::lastIndexOf()
{
    QStringList list;
    list << "harald" << "trond" << "vohi" << "harald";

    QCOMPARE(list.lastIndexOf("harald"), 3);
    QCOMPARE(list.lastIndexOf("trond"), 1);
    QCOMPARE(list.lastIndexOf("vohi"), 2);
    QCOMPARE(list.lastIndexOf("harald", 2), 0);

    QCOMPARE(list.lastIndexOf("hans"), -1);
    QCOMPARE(list.lastIndexOf("vohi", 1), -1);
    QCOMPARE(list.lastIndexOf("vohi", -1), 2);
    QCOMPARE(list.lastIndexOf("vohi", -3), -1);
}

void tst_QStringList::filter()
{
    QStringList list1, list2;
    list1 << "Bill Gates" << "Joe Blow" << "Bill Clinton";
    list1 = list1.filter( "Bill" );
    list2 << "Bill Gates" << "Bill Clinton";
    QCOMPARE( list1, list2 );

    QStringList list3, list4;
    list3 << "Bill Gates" << "Joe Blow" << "Bill Clinton";
    list3 = list3.filter( QRegExp("[i]ll") );
    list4 << "Bill Gates" << "Bill Clinton";
    QCOMPARE( list3, list4 );
}

void tst_QStringList::replaceInStrings()
{
    QStringList list1, list2;
    list1 << "alpha" << "beta" << "gamma" << "epsilon";
    list1.replaceInStrings( "a", "o" );
    list2 << "olpho" << "beto" << "gommo" << "epsilon";
    QCOMPARE( list1, list2 );

    QStringList list3, list4;
    list3 << "alpha" << "beta" << "gamma" << "epsilon";
    list3.replaceInStrings( QRegExp("^a"), "o" );
    list4 << "olpha" << "beta" << "gamma" << "epsilon";
    QCOMPARE( list3, list4 );

    QStringList list5, list6;
    list5 << "Bill Clinton" << "Gates, Bill";
    list6 << "Bill Clinton" << "Bill Gates";
    list5.replaceInStrings( QRegExp("^(.*), (.*)$"), "\\2 \\1" );
    QCOMPARE( list5, list6 );
}

void tst_QStringList::contains()
{
    QStringList list;
    list << "arthur" << "Arthur" << "arthuR" << "ARTHUR" << "Dent" << "Hans Dent";

    QVERIFY(list.contains("arthur"));
    QVERIFY(!list.contains("ArthuR"));
    QVERIFY(!list.contains("Hans"));
    QVERIFY(list.contains("arthur", Qt::CaseInsensitive));
    QVERIFY(list.contains("ArthuR", Qt::CaseInsensitive));
    QVERIFY(list.contains("ARTHUR", Qt::CaseInsensitive));
    QVERIFY(list.contains("dent", Qt::CaseInsensitive));
    QVERIFY(!list.contains("hans", Qt::CaseInsensitive));
}

void tst_QStringList::removeDuplicates_data()
{
    QTest::addColumn<QString>("before");
    QTest::addColumn<QString>("after");
    QTest::addColumn<int>("count");

    QTest::newRow("empty") << "Hello,Hello" << "Hello" << 1;
    QTest::newRow("empty") << "Hello,World" << "Hello,World" << 0;
}

void tst_QStringList::removeDuplicates()
{
    QFETCH(QString, before);
    QFETCH(QString, after);
    QFETCH(int, count);

    QStringList lbefore = before.split(',');
    QStringList lafter = after.split(',');
    int removed = lbefore.removeDuplicates();

    QCOMPARE(removed, count);
    QCOMPARE(lbefore, lafter);
}

void tst_QStringList::streamingOperator()
{
    QStringList list;
    list << "hei";
    list << list << "hopp" << list;

    QCOMPARE(list, QStringList()
            << "hei" << "hei" << "hopp"
            << "hei" << "hei" << "hopp");

    QStringList list2;
    list2 << "adam";

    QStringList list3;
    list3 << "eva";

    QCOMPARE(list2 << list3, QStringList() << "adam" << "eva");
}

void tst_QStringList::join() const
{
    QFETCH(QStringList, input);
    QFETCH(QString, separator);
    QFETCH(QString, expectedResult);

    QCOMPARE(input.join(separator), expectedResult);
}

void tst_QStringList::join_data() const
{
    QTest::addColumn<QStringList>("input");
    QTest::addColumn<QString>("separator");
    QTest::addColumn<QString>("expectedResult");

    QTest::newRow("")
                << QStringList()
                << QString()
                << QString();

    QTest::newRow("")
                << QStringList()
                << QString(QLatin1String("separator"))
                << QString();

    QTest::newRow("")
                << QStringList("one")
                << QString(QLatin1String("separator"))
                << QString("one");

    QTest::newRow("")
                << QStringList("one")
                << QString(QLatin1String("separator"))
                << QString("one");


    QTest::newRow("")
                << (QStringList()
                        << QLatin1String("a")
                        << QLatin1String("b"))
                << QString(QLatin1String(" "))
                << QString("a b");

    QTest::newRow("")
                << (QStringList()
                        << QLatin1String("a")
                        << QLatin1String("b")
                        << QLatin1String("c"))
                << QString(QLatin1String(" "))
                << QString("a b c");
}

void tst_QStringList::joinEmptiness() const
{
    QStringList list;
    QString string = list.join(QString());

    QVERIFY(string.isEmpty());
    QVERIFY(string.isNull());
}

void tst_QStringList::initializeList() const
{
#ifdef Q_COMPILER_INITIALIZER_LISTS
    QStringList v1{QLatin1String("hello"),"world",QString::fromLatin1("plop")};
    QCOMPARE(v1, (QStringList() << "hello" << "world" << "plop"));
    QCOMPARE(v1, (QStringList{"hello","world","plop"}));
#else
    QSKIP("Require C++0x support, pass the right flag to the compiler", SkipAll);
#endif
}

QTEST_APPLESS_MAIN(tst_QStringList)
#include "tst_qstringlist.moc"
