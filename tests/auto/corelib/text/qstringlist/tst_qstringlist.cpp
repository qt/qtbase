// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <qlatin1stringmatcher.h>
#include <qlist.h>
#include <qregularexpression.h>
#include <qstringlist.h>
#include <QScopeGuard>

#include <locale.h>

#include <algorithm>

using namespace Qt::StringLiterals;

class tst_QStringList : public QObject
{
    Q_OBJECT
private slots:
    void constructors();
    void sort();
    void filter();
    void replaceInStrings();
    void removeDuplicates();
    void removeDuplicates_data();
    void contains_data();
    void contains();
    void indexOf_data();
    void indexOf();
    void lastIndexOf_data();
    void lastIndexOf();

    void streamingOperator();
    void assignmentOperator();
    void join() const;
    void join_data() const;
    void joinEmptiness() const;
    void joinChar() const;
    void joinChar_data() const;

    void initializeList() const;
};

extern const char email[];

void tst_QStringList::constructors()
{
    {
        QStringList list;
        QVERIFY(list.isEmpty());
        QCOMPARE(list.size(), 0);
        QVERIFY(list == QStringList());
    }
    {
        QString str = "abc";
        QStringList list(str);
        QVERIFY(!list.isEmpty());
        QCOMPARE(list.size(), 1);
        QCOMPARE(list.at(0), str);
    }
    {
        QStringList list{ "a", "b", "c" };
        QVERIFY(!list.isEmpty());
        QCOMPARE(list.size(), 3);
        QCOMPARE(list.at(0), "a");
        QCOMPARE(list.at(1), "b");
        QCOMPARE(list.at(2), "c");
    }
    {
        const QList<QString> reference{ "a", "b", "c" };
        QCOMPARE(reference.size(), 3);

        QStringList list(reference.cbegin(), reference.cend());
        QCOMPARE(list.size(), reference.size());
        QVERIFY(std::equal(list.cbegin(), list.cend(), reference.cbegin()));
    }
}

void tst_QStringList::indexOf_data()
{
    QTest::addColumn<QStringList>("list");
    QTest::addColumn<QString>("search");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("expectedResult");

    QStringList searchIn{"harald", "trond", "vohi", "harald"};
    QTest::newRow("harald") << searchIn << "harald" << 0 << 0;
    QTest::newRow("trond") << searchIn << "trond" << 0 << 1;
    QTest::newRow("vohi") << searchIn << "vohi" << 0 << 2;
    QTest::newRow("harald-1") << searchIn << "harald" << 1 << 3;

    QTest::newRow("hans") << searchIn << "hans" << 0 << -1;
    QTest::newRow("trond-1") << searchIn << "trond" << 2 << -1;
    QTest::newRow("harald-2") << searchIn << "harald" << -1 << 3;
    QTest::newRow("vohi-1") << searchIn << "vohi" << -3 << 2;

    QTest::newRow("from-bigger-than-size") << searchIn << "harald" << 100 << -1;

    searchIn = {"lost+found", "foo.bar"};
    QTest::newRow("string-with-regex-meta-char1") << searchIn << "lost+found" << 0 << 0;
    QTest::newRow("string-with-regex-meta-char2") << searchIn << "foo.bar" << 0 << 1;
    QTest::newRow("string-with-regex-meta-char3") << searchIn << "foo.bar" << 2 << -1;
}

void tst_QStringList::indexOf()
{
    QFETCH(QStringList, list);
    QFETCH(QString, search);
    QFETCH(int, from);
    QFETCH(int, expectedResult);

    QCOMPARE(list.indexOf(search, from), expectedResult);
    QCOMPARE(list.indexOf(QStringView(search), from), expectedResult);
    QCOMPARE(list.indexOf(QLatin1String(search.toLatin1()), from), expectedResult);
    QCOMPARE(list.indexOf(QRegularExpression(QRegularExpression::escape(search)), from), expectedResult);

    QString searchUpper = search.toUpper();
    QCOMPARE(list.indexOf(searchUpper, from, Qt::CaseInsensitive), expectedResult);
    QCOMPARE(list.indexOf(QStringView(searchUpper), from, Qt::CaseInsensitive), expectedResult);
    QCOMPARE(list.indexOf(QLatin1StringView(searchUpper.toLatin1()), from, Qt::CaseInsensitive),
             expectedResult);
    const QRegularExpression re(QRegularExpression::escape(searchUpper),
                                QRegularExpression::CaseInsensitiveOption);
    QCOMPARE(list.indexOf(re, from), expectedResult);
}

void tst_QStringList::lastIndexOf_data()
{
    QTest::addColumn<QStringList>("list");
    QTest::addColumn<QString>("search");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("expectedResult");

    QStringList list{"harald", "trond", "vohi", "harald"};
    QTest::newRow("harald") << list << "harald" << -1 << 3;
    QTest::newRow("trond") << list << "trond" << -1 << 1;
    QTest::newRow("vohi") << list << "vohi" << -1 << 2;
    QTest::newRow("harald-1") << list << "harald" << 2 << 0;

    QTest::newRow("hans") << list << "hans" << -1 << -1;
    QTest::newRow("vohi-1") << list << "vohi" << 1 << -1;
    QTest::newRow("vohi-2") << list << "vohi" << -1 << 2;
    QTest::newRow("vohi-3") << list << "vohi" << -3 << -1;

    list = {"lost+found", "foo.bar"};
    QTest::newRow("string-with-regex-meta-char1") << list << "lost+found" << -1 << 0;
    QTest::newRow("string-with-regex-meta-char2") << list << "foo.bar" << -1 << 1;
    QTest::newRow("string-with-regex-meta-char3") << list << "foo.bar" << -2 << -1;
}

void tst_QStringList::lastIndexOf()
{
    QFETCH(QStringList, list);
    QFETCH(QString, search);
    QFETCH(int, from);
    QFETCH(int, expectedResult);

    QCOMPARE(list.lastIndexOf(search, from), expectedResult);
    QCOMPARE(list.lastIndexOf(QStringView(search), from), expectedResult);
    QCOMPARE(list.lastIndexOf(QLatin1String(search.toLatin1()), from), expectedResult);
    QCOMPARE(list.lastIndexOf(QRegularExpression(QRegularExpression::escape(search)), from), expectedResult);

    const QString searchUpper = search.toUpper();
    QCOMPARE(list.lastIndexOf(searchUpper, from, Qt::CaseInsensitive), expectedResult);
    QCOMPARE(list.lastIndexOf(QStringView(searchUpper), from, Qt::CaseInsensitive), expectedResult);
    QCOMPARE(list.lastIndexOf(QLatin1String(searchUpper.toLatin1()), from, Qt::CaseInsensitive),
             expectedResult);
    const QRegularExpression re(QRegularExpression::escape(searchUpper),
                                QRegularExpression::CaseInsensitiveOption);
    QCOMPARE(list.lastIndexOf(re, from), expectedResult);
}

void tst_QStringList::filter()
{
    const QStringList list = {u"Bill Gates"_s, u"Joe Blow"_s, u"Bill Clinton"_s, u"bIll"_s};

    { // CaseSensitive
        const QStringList expected{u"Bill Gates"_s, u"Bill Clinton"_s};
        QCOMPARE(list.filter(u"Bill"_s), expected);
        QCOMPARE(list.filter(u"Bill"), expected);
        QCOMPARE(list.filter("Bill"_L1), expected);
        QCOMPARE(list.filter(QRegularExpression(u"[i]ll"_s)), expected);
        QCOMPARE(list.filter(QStringMatcher(u"Bill")), expected);
        QCOMPARE(list.filter(QLatin1StringMatcher("Bill"_L1)), expected);
    }

    { // CaseInsensitive
        const QStringList expected = {u"Bill Gates"_s, u"Bill Clinton"_s, u"bIll"_s};
        QCOMPARE(list.filter(u"bill"_s, Qt::CaseInsensitive), expected);
        QCOMPARE(list.filter(u"bill", Qt::CaseInsensitive), expected);
        QCOMPARE(list.filter("bill"_L1, Qt::CaseInsensitive), expected);
        QCOMPARE(list.filter(QRegularExpression(u"[i]ll"_s, QRegularExpression::CaseInsensitiveOption)),
                             expected);
        QCOMPARE(list.filter(QStringMatcher(u"Bill", Qt::CaseInsensitive)), expected);
        QCOMPARE(list.filter(QLatin1StringMatcher("bill"_L1, Qt::CaseInsensitive)), expected);
    }
}

void tst_QStringList::sort()
{
    QStringList list1, list2;
    list1 << "alpha" << "beta" << "BETA" << "gamma" << "Gamma" << "gAmma" << "epsilon";
    list1.sort();
    list2 << "BETA" << "Gamma" << "alpha" << "beta" << "epsilon" << "gAmma" << "gamma";
    QCOMPARE( list1, list2 );

    const char *const currentLocale = setlocale(LC_ALL, "C.UTF-8");
    if (!currentLocale)
        QSKIP("Failed to set C locale, needed for testing");
    const QScopeGuard restore([currentLocale]() { setlocale(LC_ALL, currentLocale); });
    QStringList list3, list4;
    list3 << "alpha" << "beta" << "BETA" << "gamma" << "Gamma" << "gAmma" << "epsilon";
    list3.sort(Qt::CaseInsensitive);
    list4 << "alpha" << "beta" << "BETA" << "epsilon" << "Gamma" << "gAmma" << "gamma";
    // with this list, case insensitive sorting can give more than one permutation for "equivalent"
    // elements; so we check that the sort gave the formally correct result (list[i] <= list[i+1])
    for (int i = 0; i < list4.size() - 1; ++i)
        QVERIFY2(QString::compare(list4.at(i), list4.at(i + 1), Qt::CaseInsensitive) <= 0, qPrintable(QString("index %1 failed").arg(i)));
    // additional checks
    QCOMPARE(list4.at(0), QString("alpha"));
    QVERIFY(list4.indexOf("epsilon") > 0);
    QVERIFY(list4.indexOf("epsilon") < (list4.size() - 1));
}

void tst_QStringList::replaceInStrings()
{
    QStringList list1, list2;
    list1 << "alpha" << "beta" << "gamma" << "epsilon";
    list1.replaceInStrings( "a", "o" );
    list2 << "olpho" << "beto" << "gommo" << "epsilon";
    QCOMPARE( list1, list2 );

    QStringList list7, list8;
    list7 << "alpha" << "beta" << "gamma" << "epsilon";
    list7.replaceInStrings( QRegularExpression("^a"), "o" );
    list8 << "olpha" << "beta" << "gamma" << "epsilon";
    QCOMPARE( list7, list8 );

    QStringList list9, list10;
    list9 << "Bill Clinton" << "Gates, Bill";
    list10 << "Bill Clinton" << "Bill Gates";
    list9.replaceInStrings( QRegularExpression("^(.*), (.*)$"), "\\2 \\1" );
    QCOMPARE( list9, list10 );

    QStringList list11, list12, list13, list14;
    list11 << "alpha" << "beta" << "gamma" << "epsilon";
    list12 << "alpha" << "beta" << "gamma" << "epsilon";
    list13 << "alpha" << "beta" << "gamma" << "epsilon";
    list11.replaceInStrings( QStringView(QString("a")), QStringView(QString("o")) );
    list12.replaceInStrings( QStringView(QString("a")), QString("o") );
    list13.replaceInStrings( QString("a"), QStringView(QString("o")) );
    list14 << "olpho" << "beto" << "gommo" << "epsilon";
    QCOMPARE( list11, list12 );

    QStringList list{"alpha", "beta", "gamma"};
    QStringList copy = list;
    QVERIFY(!copy.isDetached());

    // No matches, no detach
    copy.replaceInStrings("z", "y");
    QVERIFY(!copy.isDetached());
    QCOMPARE(copy, list);

    copy.replaceInStrings("a", "y");
    QVERIFY(copy.isDetached());
    QCOMPARE(copy, (QStringList{"ylphy", "bety", "gymmy"}));
}

void tst_QStringList::contains_data()
{
    QTest::addColumn<QString>("needle");
    QTest::addColumn<Qt::CaseSensitivity>("cs");
    QTest::addColumn<bool>("expected");

    QTest::newRow("arthur") << u"arthur"_s << Qt::CaseSensitive << true;
    QTest::newRow("ArthuR") << u"ArthuR"_s << Qt::CaseSensitive << false;
    QTest::newRow("arthur") << u"arthur"_s << Qt::CaseInsensitive << true;
    QTest::newRow("ArthuR") << u"ArthuR"_s << Qt::CaseInsensitive << true;
    QTest::newRow("ARTHUR") << u"ARTHUR"_s << Qt::CaseInsensitive << true;
    QTest::newRow("Hans") << u"Hans"_s << Qt::CaseSensitive << false;
    QTest::newRow("hans") << u"hans"_s << Qt::CaseInsensitive << false;
    QTest::newRow("dent") << u"dent"_s << Qt::CaseInsensitive << true;
}

void tst_QStringList::contains()
{
    QFETCH(QString, needle);
    QFETCH(Qt::CaseSensitivity, cs);
    QFETCH(bool, expected);

    const QStringList list = {
        u"arthur"_s, u"Arthur"_s, u"arthuR"_s, u"ARTHUR"_s, u"Dent"_s, u"Hans Dent"_s
    };

    QCOMPARE(list.contains(needle, cs), expected);
    QCOMPARE(list.contains(QStringView(needle), cs), expected);
    QCOMPARE(list.contains(QLatin1StringView(needle.toLatin1()), cs), expected);
}

void tst_QStringList::removeDuplicates_data()
{
    QTest::addColumn<QString>("before");
    QTest::addColumn<QString>("after");
    QTest::addColumn<int>("count");
    QTest::addColumn<bool>("detached");

    QTest::newRow("empty-1") << "Hello,Hello" << "Hello" << 1 << true;
    QTest::newRow("empty-2") << "Hello,World" << "Hello,World" << 0 << false;
    QTest::newRow("middle")  << "Hello,World,Hello" << "Hello,World" << 1 << true;
}

void tst_QStringList::removeDuplicates()
{
    QFETCH(QString, before);
    QFETCH(QString, after);
    QFETCH(int, count);
    QFETCH(bool, detached);

    QStringList lbefore = before.split(',');
    const QStringList oldlbefore = lbefore;
    QStringList lafter = after.split(',');
    int removed = lbefore.removeDuplicates();

    QCOMPARE(removed, count);
    QCOMPARE(lbefore, lafter);
    QCOMPARE(detached, !oldlbefore.isSharedWith(lbefore));
}

void tst_QStringList::streamingOperator()
{
    QStringList list;
    list << "hei";
    list << list << "hopp" << list;

    QList<QString> slist = list;
    list << slist;

    QCOMPARE(list, QStringList()
            << "hei" << "hei" << "hopp"
            << "hei" << "hei" << "hopp"
            << "hei" << "hei" << "hopp"
            << "hei" << "hei" << "hopp");

    QStringList list2;
    list2 << "adam";

    QStringList list3;
    list3 << "eva";

    QCOMPARE(list2 << list3, QStringList() << "adam" << "eva");
}

void tst_QStringList::assignmentOperator()
{
    // compile-only test

    QStringList adam;
    adam << "adam";
    QList<QString> eva;
    eva << "eva";
    QStringList result;
    QStringList &ref1 = (result = adam);
    QStringList &ref2 = (result = eva);
    Q_UNUSED(ref1);
    Q_UNUSED(ref2);
}

void tst_QStringList::join() const
{
    QFETCH(QStringList, input);
    QFETCH(QString, separator);
    QFETCH(QString, expectedResult);

    QCOMPARE(input.join(separator), expectedResult);
    QCOMPARE(input.join(QLatin1String(separator.toLatin1())), expectedResult);
    QCOMPARE(input.join(QStringView(separator)), expectedResult);
}

void tst_QStringList::join_data() const
{
    QTest::addColumn<QStringList>("input");
    QTest::addColumn<QString>("separator");
    QTest::addColumn<QString>("expectedResult");

    QTest::newRow("data1")
                << QStringList()
                << QString()
                << QString();

    QTest::newRow("data2")
                << QStringList()
                << QString(QLatin1String("separator"))
                << QString();

    QTest::newRow("data3")
                << QStringList("one")
                << QString(QLatin1String("separator"))
                << QString("one");

    QTest::newRow("data4")
                << QStringList("one")
                << QString(QLatin1String("separator"))
                << QString("one");


    QTest::newRow("data5")
                << (QStringList()
                        << QLatin1String("a")
                        << QLatin1String("b"))
                << QString(QLatin1String(" "))
                << QString("a b");

    QTest::newRow("data6")
                << (QStringList()
                        << QLatin1String("a")
                        << QLatin1String("b")
                        << QLatin1String("c"))
                << QString(QLatin1String(" "))
                << QString("a b c");
}

void tst_QStringList::joinChar() const
{
    QFETCH(QStringList, input);
    QFETCH(QChar, separator);
    QFETCH(QString, expectedResult);

    QCOMPARE(input.join(separator), expectedResult);
}

void tst_QStringList::joinChar_data() const
{
    QTest::addColumn<QStringList>("input");
    QTest::addColumn<QChar>("separator");
    QTest::addColumn<QString>("expectedResult");

    QTest::newRow("data1")
                << QStringList()
                << QChar(QLatin1Char(' '))
                << QString();

    QTest::newRow("data5")
                << (QStringList()
                        << QLatin1String("a")
                        << QLatin1String("b"))
                << QChar(QLatin1Char(' '))
                << QString("a b");

    QTest::newRow("data6")
                << (QStringList()
                        << QLatin1String("a")
                        << QLatin1String("b")
                        << QLatin1String("c"))
                << QChar(QLatin1Char(' '))
                << QString("a b c");

    QTest::newRow("null separator")
            << QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")}
            << QChar(u'\0')
            << QStringLiteral("a\0b\0c");
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

    QStringList v1{QLatin1String("hello"),"world",QString::fromLatin1("plop")};
    QCOMPARE(v1, (QStringList() << "hello" << "world" << "plop"));
    QCOMPARE(v1, (QStringList{"hello","world","plop"}));
}

QTEST_APPLESS_MAIN(tst_QStringList)
#include "tst_qstringlist.moc"
