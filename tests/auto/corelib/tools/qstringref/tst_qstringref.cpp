/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include <qstringlist.h>
#include <qvariant.h>

#include <qlocale.h>
#include <locale.h>


class tst_QStringRef : public QObject
{
    Q_OBJECT
public slots:
    void cleanup();
private slots:
    void at();
    void endsWith();
    void startsWith();
    void contains();
    void count();
    void lastIndexOf_data();
    void lastIndexOf();
    void indexOf_data();
    void indexOf();
    void indexOf2_data();
    void indexOf2();
    void iteration();
    void length_data();
    void length();
    void isEmpty();
    void compare_data();
    void compare();
    void compare2_data();
    void compare2();
    void operator_eqeq_nullstring();
    void toNum();
    void toDouble_data();
    void toDouble();
    void toFloat();
    void toLong_data();
    void toLong();
    void toULong_data();
    void toULong();
    void toLongLong();
    void toULongLong();
    void toUInt();
    void toInt();
    void toShort();
    void toUShort();
    void double_conversion_data();
    void double_conversion();
    void integer_conversion_data();
    void integer_conversion();
    void trimmed();
    void truncate();
    void left();
    void right();
    void mid();
    void split_data();
    void split();
};

static QStringRef emptyRef()
{
    static const QString empty("");
    return empty.midRef(0);
}

#define CREATE_REF(string)                                              \
    const QString padded = QLatin1Char(' ') + string + QLatin1Char(' ');     \
    QStringRef ref = padded.midRef(1, padded.size() - 2);

typedef QList<int> IntList;

// This next bit is needed for the NAN and INF in string -> number conversion tests
#include <float.h>
#include <limits.h>
#include <math.h>
#if defined(Q_OS_WIN)
#   include <windows.h>
// mingw defines NAN and INFINITY to 0/0 and x/0
#   if defined(Q_CC_GNU)
#      undef NAN
#      undef INFINITY
#   else
#      define isnan(d) _isnan(d)
#   endif
#endif
#if defined(Q_OS_MAC) && !defined isnan
#define isnan(d) __isnand(d)
#endif
#if defined(Q_OS_SOLARIS)
#   include <ieeefp.h>
#endif
#if defined(Q_OS_OSF) && (defined(__DECC) || defined(__DECCXX))
#   define INFINITY DBL_INFINITY
#   define NAN DBL_QNAN
#endif
#if defined(Q_OS_IRIX) && defined(Q_CC_GNU)
#   include <ieeefp.h>
#   define isnan(d) isnand(d)
#endif

enum {
    LittleEndian,
    BigEndian
#ifdef Q_BYTE_ORDER
#  if Q_BYTE_ORDER == Q_BIG_ENDIAN
    , ByteOrder = BigEndian
#  elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    , ByteOrder = LittleEndian
#  else
#    error "undefined byte order"
#  endif
};
#else
};
static const unsigned int one = 1;
static const bool ByteOrder = ((*((unsigned char *) &one) == 0) ? BigEndian : LittleEndian);
#endif
#if !defined(INFINITY)
static const unsigned char be_inf_bytes[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0,0 };
static const unsigned char le_inf_bytes[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
static inline double inf()
{
    if (ByteOrder == BigEndian)
        return *reinterpret_cast<const double *>(be_inf_bytes);
    return *reinterpret_cast<const double *>(le_inf_bytes);
}
#   define INFINITY (::inf())
#endif
#if !defined(NAN)
static const unsigned char be_nan_bytes[] = { 0x7f, 0xf8, 0, 0, 0, 0, 0,0 };
static const unsigned char le_nan_bytes[] = { 0, 0, 0, 0, 0, 0, 0xf8, 0x7f };
static inline double nan()
{
    if (ByteOrder == BigEndian)
        return *reinterpret_cast<const double *>(be_nan_bytes);
    return *reinterpret_cast<const double *>(le_nan_bytes);
}
#   define NAN (::nan())
#endif

void tst_QStringRef::cleanup()
{
    QLocale::setDefault(QString(QLatin1Char('C')));
}

void tst_QStringRef::at()
{
    const QString hw = QStringLiteral("Hello World");
    const QStringRef ref = hw.midRef(6);
    QCOMPARE(ref.at(0), QChar('W'));
    QCOMPARE(ref.at(4), QChar('d'));
    QCOMPARE(ref[0], QChar('W'));
    QCOMPARE(ref[4], QChar('d'));
}

void tst_QStringRef::length_data()
{
    QTest::addColumn<QString>("s1");
    QTest::addColumn<int>("res");

    QTest::newRow("data0") << QString("Test") << 4;
    QTest::newRow("data1") << QString("The quick brown fox jumps over the lazy dog") << 43;
    QTest::newRow("data2") << QString() << 0;
    QTest::newRow("data3") << QString("A") << 1;
    QTest::newRow("data4") << QString("AB") << 2;
    QTest::newRow("data5") << QString("AB\n") << 3;
    QTest::newRow("data6") << QString("AB\nC") << 4;
    QTest::newRow("data7") << QString("\n") << 1;
    QTest::newRow("data8") << QString("\nA") << 2;
    QTest::newRow("data9") << QString("\nAB") << 3;
    QTest::newRow("data10") << QString("\nAB\nCDE") << 7;
    QTest::newRow("data11") << QString("shdnftrheid fhgnt gjvnfmd chfugkh bnfhg thgjf vnghturkf chfnguh bjgnfhvygh hnbhgutjfv dhdnjds dcjs d") << 100;
    QTest::newRow("data12") << QString("") << 0;
}


void tst_QStringRef::length()
{
    QFETCH(QString, s1);
    CREATE_REF(s1);
    QTEST(ref.length(), "res");
}


void tst_QStringRef::isEmpty()
{
    QStringRef a;
    QVERIFY(a.isEmpty());
    QVERIFY(emptyRef().isEmpty());
    CREATE_REF("Not empty");
    QVERIFY(!ref.isEmpty());
}

void tst_QStringRef::indexOf_data()
{
    QTest::addColumn<QString>("haystack");
    QTest::addColumn<QString>("needle");
    QTest::addColumn<int>("startpos");
    QTest::addColumn<bool>("bcs");
    QTest::addColumn<int>("resultpos");

    QTest::newRow("data0") << QString("abc") << QString("a") << 0 << true << 0;
    QTest::newRow("data1") << QString("abc") << QString("a") << 0 << false << 0;
    QTest::newRow("data2") << QString("abc") << QString("A") << 0 << true << -1;
    QTest::newRow("data3") << QString("abc") << QString("A") << 0 << false << 0;
    QTest::newRow("data4") << QString("abc") << QString("a") << 1 << true << -1;
    QTest::newRow("data5") << QString("abc") << QString("a") << 1 << false << -1;
    QTest::newRow("data6") << QString("abc") << QString("A") << 1 << true << -1;
    QTest::newRow("data7") << QString("abc") << QString("A") << 1 << false << -1;
    QTest::newRow("data8") << QString("abc") << QString("b") << 0 << true << 1;
    QTest::newRow("data9") << QString("abc") << QString("b") << 0 << false << 1;
    QTest::newRow("data10") << QString("abc") << QString("B") << 0 << true << -1;
    QTest::newRow("data11") << QString("abc") << QString("B") << 0 << false << 1;
    QTest::newRow("data12") << QString("abc") << QString("b") << 1 << true << 1;
    QTest::newRow("data13") << QString("abc") << QString("b") << 1 << false << 1;
    QTest::newRow("data14") << QString("abc") << QString("B") << 1 << true << -1;
    QTest::newRow("data15") << QString("abc") << QString("B") << 1 << false << 1;
    QTest::newRow("data16") << QString("abc") << QString("b") << 2 << true << -1;
    QTest::newRow("data17") << QString("abc") << QString("b") << 2 << false << -1;

    QTest::newRow("data20") << QString("ABC") << QString("A") << 0 << true << 0;
    QTest::newRow("data21") << QString("ABC") << QString("A") << 0 << false << 0;
    QTest::newRow("data22") << QString("ABC") << QString("a") << 0 << true << -1;
    QTest::newRow("data23") << QString("ABC") << QString("a") << 0 << false << 0;
    QTest::newRow("data24") << QString("ABC") << QString("A") << 1 << true << -1;
    QTest::newRow("data25") << QString("ABC") << QString("A") << 1 << false << -1;
    QTest::newRow("data26") << QString("ABC") << QString("a") << 1 << true << -1;
    QTest::newRow("data27") << QString("ABC") << QString("a") << 1 << false << -1;
    QTest::newRow("data28") << QString("ABC") << QString("B") << 0 << true << 1;
    QTest::newRow("data29") << QString("ABC") << QString("B") << 0 << false << 1;
    QTest::newRow("data30") << QString("ABC") << QString("b") << 0 << true << -1;
    QTest::newRow("data31") << QString("ABC") << QString("b") << 0 << false << 1;
    QTest::newRow("data32") << QString("ABC") << QString("B") << 1 << true << 1;
    QTest::newRow("data33") << QString("ABC") << QString("B") << 1 << false << 1;
    QTest::newRow("data34") << QString("ABC") << QString("b") << 1 << true << -1;
    QTest::newRow("data35") << QString("ABC") << QString("b") << 1 << false << 1;
    QTest::newRow("data36") << QString("ABC") << QString("B") << 2 << true << -1;
    QTest::newRow("data37") << QString("ABC") << QString("B") << 2 << false << -1;

    QTest::newRow("data40") << QString("aBc") << QString("bc") << 0 << true << -1;
    QTest::newRow("data41") << QString("aBc") << QString("Bc") << 0 << true << 1;
    QTest::newRow("data42") << QString("aBc") << QString("bC") << 0 << true << -1;
    QTest::newRow("data43") << QString("aBc") << QString("BC") << 0 << true << -1;
    QTest::newRow("data44") << QString("aBc") << QString("bc") << 0 << false << 1;
    QTest::newRow("data45") << QString("aBc") << QString("Bc") << 0 << false << 1;
    QTest::newRow("data46") << QString("aBc") << QString("bC") << 0 << false << 1;
    QTest::newRow("data47") << QString("aBc") << QString("BC") << 0 << false << 1;
    QTest::newRow("data48") << QString("AbC") << QString("bc") << 0 << true << -1;
    QTest::newRow("data49") << QString("AbC") << QString("Bc") << 0 << true << -1;
    QTest::newRow("data50") << QString("AbC") << QString("bC") << 0 << true << 1;
    QTest::newRow("data51") << QString("AbC") << QString("BC") << 0 << true << -1;
    QTest::newRow("data52") << QString("AbC") << QString("bc") << 0 << false << 1;
    QTest::newRow("data53") << QString("AbC") << QString("Bc") << 0 << false << 1;

    QTest::newRow("data54") << QString("AbC") << QString("bC") << 0 << false << 1;
    QTest::newRow("data55") << QString("AbC") << QString("BC") << 0 << false << 1;
    QTest::newRow("data56") << QString("AbC") << QString("BC") << 1 << false << 1;
    QTest::newRow("data57") << QString("AbC") << QString("BC") << 2 << false << -1;
#if 0
    QTest::newRow("null-in-null") << QString() << QString() << 0 << false << 0;
    QTest::newRow("empty-in-null") << QString() << QString("") << 0 << false << 0;
    QTest::newRow("null-in-empty") << QString("") << QString() << 0 << false << 0;
    QTest::newRow("empty-in-empty") << QString("") << QString("") << 0 << false << 0;
#endif


    QString s1 = "abc";
    s1 += QChar(0xb5);
    QString s2;
    s2 += QChar(0x3bc);
    QTest::newRow("data58") << QString(s1) << QString(s2) << 0 << false << 3;
    s2.prepend(QLatin1Char('C'));
    QTest::newRow("data59") << QString(s1) << QString(s2) << 0 << false << 2;

    QString veryBigHaystack(500, 'a');
    veryBigHaystack += 'B';
    QTest::newRow("BoyerMooreStressTest") << veryBigHaystack << veryBigHaystack << 0 << true << 0;
    QTest::newRow("BoyerMooreStressTest2") << veryBigHaystack + 'c' << veryBigHaystack << 0 << true << 0;
    QTest::newRow("BoyerMooreStressTest3") << 'c' + veryBigHaystack << veryBigHaystack << 0 << true << 1;
    QTest::newRow("BoyerMooreStressTest4") << veryBigHaystack << veryBigHaystack + 'c' << 0 << true << -1;
    QTest::newRow("BoyerMooreStressTest5") << veryBigHaystack << 'c' + veryBigHaystack << 0 << true << -1;
    QTest::newRow("BoyerMooreStressTest6") << 'd' + veryBigHaystack << 'c' + veryBigHaystack << 0 << true << -1;
    QTest::newRow("BoyerMooreStressTest7") << veryBigHaystack + 'c' << 'c' + veryBigHaystack << 0 << true << -1;

    QTest::newRow("BoyerMooreInsensitiveStressTest") << veryBigHaystack << veryBigHaystack << 0 << false << 0;

}

void tst_QStringRef::indexOf()
{
    QFETCH(QString, haystack);
    QFETCH(QString, needle);
    QFETCH(int, startpos);
    QFETCH(bool, bcs);
    QFETCH(int, resultpos);

    const QString haystackPadded = QLatin1Char(' ') + haystack + QLatin1Char(' ');
    const QString needlePadded = QLatin1Char(' ') + needle + QLatin1Char(' ');
    const QStringRef haystackRef(&haystackPadded, 1, haystack.size());
    const QStringRef needleRef(&needlePadded, 1, needle.size());

    Qt::CaseSensitivity cs = bcs ? Qt::CaseSensitive : Qt::CaseInsensitive;

    QCOMPARE(haystack.indexOf(needle, startpos, cs), resultpos);
    QCOMPARE(haystackRef.indexOf(needle, startpos, cs), resultpos);
    QCOMPARE(haystackRef.indexOf(needleRef, startpos, cs), resultpos);
    QCOMPARE(haystack.indexOf(needleRef, startpos, cs), resultpos);

    if (cs == Qt::CaseSensitive) {
        QCOMPARE(haystack.indexOf(needle, startpos), resultpos);
        QCOMPARE(haystackRef.indexOf(needle, startpos), resultpos);
        QCOMPARE(haystackRef.indexOf(needleRef, startpos), resultpos);
        QCOMPARE(haystack.indexOf(needleRef, startpos), resultpos);
        if (startpos == 0) {
            QCOMPARE(haystack.indexOf(needle), resultpos);
            QCOMPARE(haystackRef.indexOf(needle), resultpos);
            QCOMPARE(haystackRef.indexOf(needleRef), resultpos);
            QCOMPARE(haystack.indexOf(needleRef), resultpos);
        }
    }
    if (needle.size() == 1) {
        QCOMPARE(needle.at(0), needleRef.at(0));
        QCOMPARE(haystack.indexOf(needleRef.at(0), startpos, cs), resultpos);
        QCOMPARE(haystackRef.indexOf(needle.at(0), startpos, cs), resultpos);
        QCOMPARE(haystackRef.indexOf(needleRef.at(0), startpos, cs), resultpos);
        QCOMPARE(haystack.indexOf(needleRef.at(0), startpos ,cs), resultpos);
    }
}

void tst_QStringRef::indexOf2_data()
{
    QTest::addColumn<QString>("haystack");
    QTest::addColumn<QString>("needle");
    QTest::addColumn<int>("resultpos");

    QTest::newRow("data0") << QString() << QString() << 0;
    QTest::newRow("data1") << QString() << QString("") << 0;
    QTest::newRow("data2") << QString("") << QString() << 0;
    QTest::newRow("data3") << QString("") << QString("") << 0;
    QTest::newRow("data4") << QString() << QString("a") << -1;
    QTest::newRow("data5") << QString() << QString("abcdefg") << -1;
    QTest::newRow("data6") << QString("") << QString("a") << -1;
    QTest::newRow("data7") << QString("") << QString("abcdefg") << -1;

    QTest::newRow("data8") << QString("a") << QString() << 0;
    QTest::newRow("data9") << QString("a") << QString("") << 0;
    QTest::newRow("data10") << QString("a") << QString("a") << 0;
    QTest::newRow("data11") << QString("a") << QString("b") << -1;
    QTest::newRow("data12") << QString("a") << QString("abcdefg") << -1;
    QTest::newRow("data13") << QString("ab") << QString() << 0;
    QTest::newRow("data14") << QString("ab") << QString("") << 0;
    QTest::newRow("data15") << QString("ab") << QString("a") << 0;
    QTest::newRow("data16") << QString("ab") << QString("b") << 1;
    QTest::newRow("data17") << QString("ab") << QString("ab") << 0;
    QTest::newRow("data18") << QString("ab") << QString("bc") << -1;
    QTest::newRow("data19") << QString("ab") << QString("abcdefg") << -1;

    QTest::newRow("data30") << QString("abc") << QString("a") << 0;
    QTest::newRow("data31") << QString("abc") << QString("b") << 1;
    QTest::newRow("data32") << QString("abc") << QString("c") << 2;
    QTest::newRow("data33") << QString("abc") << QString("d") << -1;
    QTest::newRow("data34") << QString("abc") << QString("ab") << 0;
    QTest::newRow("data35") << QString("abc") << QString("bc") << 1;
    QTest::newRow("data36") << QString("abc") << QString("cd") << -1;
    QTest::newRow("data37") << QString("abc") << QString("ac") << -1;

    // sizeof(whale) > 32
    QString whale = "a5zby6cx7dw8evf9ug0th1si2rj3qkp4lomn";
    QString minnow = "zby";
    QTest::newRow("data40") << whale << minnow << 2;
    QTest::newRow("data41") << (whale + whale) << minnow << 2;
    QTest::newRow("data42") << (minnow + whale) << minnow << 0;
    QTest::newRow("data43") << whale << whale << 0;
    QTest::newRow("data44") << (whale + whale) << whale << 0;
    QTest::newRow("data45") << whale << (whale + whale) << -1;
    QTest::newRow("data46") << (whale + whale) << (whale + whale) << 0;
    QTest::newRow("data47") << (whale + whale) << (whale + minnow) << -1;
    QTest::newRow("data48") << (minnow + whale) << whale << (int)minnow.length();
}

void tst_QStringRef::indexOf2()
{
    QFETCH(QString, haystack);
    QFETCH(QString, needle);
    QFETCH(int, resultpos);

    const QString haystackPadded = QLatin1Char(' ') + haystack + QLatin1Char(' ');
    const QString needlePadded = QLatin1Char(' ') + needle + QLatin1Char(' ');
    const QStringRef haystackRef(&haystackPadded, 1, haystack.size());
    const QStringRef needleRef(&needlePadded, 1, needle.size());


    int got;

    QCOMPARE(haystack.indexOf(needleRef, 0, Qt::CaseSensitive), resultpos);
    QCOMPARE(haystackRef.indexOf(needle, 0, Qt::CaseSensitive), resultpos);
    QCOMPARE(haystackRef.indexOf(needleRef, 0, Qt::CaseSensitive), resultpos);
    QCOMPARE(haystack.indexOf(needleRef, 0, Qt::CaseInsensitive), resultpos);
    QCOMPARE(haystackRef.indexOf(needle, 0, Qt::CaseInsensitive), resultpos);
    QCOMPARE(haystackRef.indexOf(needleRef, 0, Qt::CaseInsensitive), resultpos);
    if (needle.length() > 0) {
        got = haystackRef.lastIndexOf(needle, -1, Qt::CaseSensitive);
        QVERIFY(got == resultpos || (resultpos >= 0 && got >= resultpos));
        got = haystackRef.lastIndexOf(needle, -1, Qt::CaseInsensitive);
        QVERIFY(got == resultpos || (resultpos >= 0 && got >= resultpos));

        got = haystack.lastIndexOf(needleRef, -1, Qt::CaseSensitive);
        QVERIFY(got == resultpos || (resultpos >= 0 && got >= resultpos));
        got = haystack.lastIndexOf(needleRef, -1, Qt::CaseInsensitive);
        QVERIFY(got == resultpos || (resultpos >= 0 && got >= resultpos));

        got = haystackRef.lastIndexOf(needleRef, -1, Qt::CaseSensitive);
        QVERIFY(got == resultpos || (resultpos >= 0 && got >= resultpos));
        got = haystackRef.lastIndexOf(needleRef, -1, Qt::CaseInsensitive);
        QVERIFY(got == resultpos || (resultpos >= 0 && got >= resultpos));
    }
}

void tst_QStringRef::iteration()
{
    QString hello = "Hello";
    QString olleh = "olleH";

    QStringRef ref(&hello);
    QStringRef rref(&olleh);

    const QStringRef &cref = ref;
    const QStringRef &crref = rref;

    QVERIFY(std::equal(  ref.begin(),   ref.end(), hello.begin()));
    QVERIFY(std::equal( rref.begin(),  rref.end(), olleh.begin()));
    QVERIFY(std::equal( cref.begin(),  cref.end(), hello.begin()));
    QVERIFY(std::equal(crref.begin(), crref.end(), olleh.begin()));

    QVERIFY(std::equal(  ref.cbegin(),   ref.cend(), hello.begin()));
    QVERIFY(std::equal( rref.cbegin(),  rref.cend(), olleh.begin()));
    QVERIFY(std::equal( cref.cbegin(),  cref.cend(), hello.begin()));
    QVERIFY(std::equal(crref.cbegin(), crref.cend(), olleh.begin()));

    QVERIFY(std::equal(  ref.rbegin(),   ref.rend(), hello.rbegin()));
    QVERIFY(std::equal( rref.rbegin(),  rref.rend(), olleh.rbegin()));
    QVERIFY(std::equal( cref.rbegin(),  cref.rend(), hello.rbegin()));
    QVERIFY(std::equal(crref.rbegin(), crref.rend(), olleh.rbegin()));

    QVERIFY(std::equal(  ref.crbegin(),   ref.crend(), hello.rbegin()));
    QVERIFY(std::equal( rref.crbegin(),  rref.crend(), olleh.rbegin()));
    QVERIFY(std::equal( cref.crbegin(),  cref.crend(), hello.rbegin()));
    QVERIFY(std::equal(crref.crbegin(), crref.crend(), olleh.rbegin()));
}

void tst_QStringRef::lastIndexOf_data()
{
    QTest::addColumn<QString>("haystack");
    QTest::addColumn<QString>("needle");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("expected");
    QTest::addColumn<bool>("caseSensitive");

    QString a = "ABCDEFGHIEfGEFG";

    QTest::newRow("-1") << a << "G" << a.size() - 1 << 14 << true;
    QTest::newRow("1") << a << "G" << - 1 << 14 << true;
    QTest::newRow("2") << a << "G" << -3 << 11 << true;
    QTest::newRow("3") << a << "G" << -5 << 6 << true;
    QTest::newRow("4") << a << "G" << 14 << 14 << true;
    QTest::newRow("5") << a << "G" << 13 << 11 << true;
    QTest::newRow("6") << a << "B" << a.size() - 1 << 1 << true;
    QTest::newRow("7") << a << "B" << - 1 << 1 << true;
    QTest::newRow("8") << a << "B" << 1 << 1 << true;
    QTest::newRow("9") << a << "B" << 0 << -1 << true;

    QTest::newRow("10") << a << "G" <<  -1 <<  a.size()-1 << true;
    QTest::newRow("11") << a << "G" <<  a.size()-1 <<  a.size()-1 << true;
    QTest::newRow("12") << a << "G" <<  a.size() <<  -1 << true;
    QTest::newRow("13") << a << "A" <<  0 <<  0 << true;
    QTest::newRow("14") << a << "A" <<  -1*a.size() <<  0 << true;

    QTest::newRow("15") << a << "efg" << 0 << -1 << false;
    QTest::newRow("16") << a << "efg" << a.size() << -1 << false;
    QTest::newRow("17") << a << "efg" << -1 * a.size() << -1 << false;
    QTest::newRow("19") << a << "efg" << a.size() - 1 << 12 << false;
    QTest::newRow("20") << a << "efg" << 12 << 12 << false;
    QTest::newRow("21") << a << "efg" << -12 << -1 << false;
    QTest::newRow("22") << a << "efg" << 11 << 9 << false;

    QTest::newRow("24") << "" << "asdf" << -1 << -1 << false;
    QTest::newRow("25") << "asd" << "asdf" << -1 << -1 << false;
    QTest::newRow("26") << "" << QString() << -1 << -1 << false;

    QTest::newRow("27") << a << "" << a.size() << a.size() << false;
    QTest::newRow("28") << a << "" << a.size() + 10 << -1 << false;
}

void tst_QStringRef::lastIndexOf()
{
    QFETCH(QString, haystack);
    QFETCH(QString, needle);
    QFETCH(int, from);
    QFETCH(int, expected);
    QFETCH(bool, caseSensitive);

    const QString haystackPadded = QLatin1Char(' ') + haystack + QLatin1Char(' ');
    const QString needlePadded = QLatin1Char(' ') + needle + QLatin1Char(' ');
    const QStringRef haystackRef(&haystackPadded, 1, haystack.size());
    const QStringRef needleRef(&needlePadded, 1, needle.size());

    Qt::CaseSensitivity cs = (caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);

    QCOMPARE(haystack.lastIndexOf(needleRef, from, cs), expected);
    QCOMPARE(haystackRef.lastIndexOf(needle, from, cs), expected);
    QCOMPARE(haystackRef.lastIndexOf(needleRef, from, cs), expected);


    if (cs == Qt::CaseSensitive) {
        QCOMPARE(haystack.lastIndexOf(needleRef, from), expected);
        QCOMPARE(haystackRef.lastIndexOf(needle, from), expected);
        QCOMPARE(haystackRef.lastIndexOf(needleRef, from), expected);

        if (from == -1) {
            QCOMPARE(haystack.lastIndexOf(needleRef), expected);
            QCOMPARE(haystackRef.lastIndexOf(needle), expected);
            QCOMPARE(haystackRef.lastIndexOf(needleRef), expected);

        }
    }
    if (needle.size() == 1) {
        QCOMPARE(haystack.lastIndexOf(needleRef.at(0), from), expected);
        QCOMPARE(haystackRef.lastIndexOf(needle.at(0), from), expected);
        QCOMPARE(haystackRef.lastIndexOf(needleRef.at(0), from), expected);
    }
}

void tst_QStringRef::count()
{
    const QString a = QString::fromLatin1("ABCDEFGHIEfGEFG"); // 15 chars
    CREATE_REF(a);
    QCOMPARE(ref.count('A'),1);
    QCOMPARE(ref.count('Z'),0);
    QCOMPARE(ref.count('E'),3);
    QCOMPARE(ref.count('F'),2);
    QCOMPARE(ref.count('F',Qt::CaseInsensitive),3);
    QCOMPARE(ref.count("FG"),2);
    QCOMPARE(ref.count("FG",Qt::CaseInsensitive),3);
    QCOMPARE(ref.count(QString(), Qt::CaseInsensitive), 16);
    QCOMPARE(ref.count("", Qt::CaseInsensitive), 16);
}

void tst_QStringRef::contains()
{
    const QString a = QString::fromLatin1("ABCDEFGHIEfGEFG"); // 15 chars
    CREATE_REF(a);
    QVERIFY(ref.contains('A'));
    QVERIFY(!ref.contains('Z'));
    QVERIFY(ref.contains('E'));
    QVERIFY(ref.contains('F'));
    QVERIFY(ref.contains('F',Qt::CaseInsensitive));
    QVERIFY(ref.contains("FG"));
    QVERIFY(ref.contains(QString("FG").midRef(0)));
    const QString ref2 = QString::fromLatin1(" FG ");
    QVERIFY(ref.contains(ref2.midRef(1, 2),Qt::CaseInsensitive));
    QVERIFY(ref.contains(QString(), Qt::CaseInsensitive));
    QVERIFY(ref.contains("", Qt::CaseInsensitive)); // apparently
}

void tst_QStringRef::startsWith()
{
    {
        const QString a = QString::fromLatin1("AB");
        CREATE_REF(a);
        QVERIFY(ref.startsWith("A"));
        QVERIFY(ref.startsWith("AB"));
        QVERIFY(!ref.startsWith("C"));
        QVERIFY(!ref.startsWith("ABCDEF"));
        QVERIFY(ref.startsWith(""));
        QVERIFY(ref.startsWith(QString::null));
        QVERIFY(ref.startsWith('A'));
        QVERIFY(ref.startsWith(QLatin1Char('A')));
        QVERIFY(ref.startsWith(QChar('A')));
        QVERIFY(!ref.startsWith('C'));
        QVERIFY(!ref.startsWith(QChar()));
        QVERIFY(!ref.startsWith(QLatin1Char(0)));

        QVERIFY(ref.startsWith(QLatin1String("A")));
        QVERIFY(ref.startsWith(QLatin1String("AB")));
        QVERIFY(!ref.startsWith(QLatin1String("C")));
        QVERIFY(!ref.startsWith(QLatin1String("ABCDEF")));
        QVERIFY(ref.startsWith(QLatin1String("")));
        QVERIFY(ref.startsWith(QLatin1String(0)));

        QVERIFY(ref.startsWith("A", Qt::CaseSensitive));
        QVERIFY(ref.startsWith("A", Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith("a", Qt::CaseSensitive));
        QVERIFY(ref.startsWith("a", Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith("aB", Qt::CaseSensitive));
        QVERIFY(ref.startsWith("aB", Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith("C", Qt::CaseSensitive));
        QVERIFY(!ref.startsWith("C", Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith("c", Qt::CaseSensitive));
        QVERIFY(!ref.startsWith("c", Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith("abcdef", Qt::CaseInsensitive));
        QVERIFY(ref.startsWith("", Qt::CaseInsensitive));
        QVERIFY(ref.startsWith(QString::null, Qt::CaseInsensitive));
        QVERIFY(ref.startsWith('a', Qt::CaseInsensitive));
        QVERIFY(ref.startsWith('A', Qt::CaseInsensitive));
        QVERIFY(ref.startsWith(QLatin1Char('a'), Qt::CaseInsensitive));
        QVERIFY(ref.startsWith(QChar('a'), Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith('c', Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith(QChar(), Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith(QLatin1Char(0), Qt::CaseInsensitive));

        QVERIFY(ref.startsWith(QLatin1String("A"), Qt::CaseSensitive));
        QVERIFY(ref.startsWith(QLatin1String("A"), Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith(QLatin1String("a"), Qt::CaseSensitive));
        QVERIFY(ref.startsWith(QLatin1String("a"), Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith(QLatin1String("aB"), Qt::CaseSensitive));
        QVERIFY(ref.startsWith(QLatin1String("aB"), Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith(QLatin1String("C"), Qt::CaseSensitive));
        QVERIFY(!ref.startsWith(QLatin1String("C"), Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith(QLatin1String("c"), Qt::CaseSensitive));
        QVERIFY(!ref.startsWith(QLatin1String("c"), Qt::CaseInsensitive));
        QVERIFY(!ref.startsWith(QLatin1String("abcdef"), Qt::CaseInsensitive));
        QVERIFY(ref.startsWith(QLatin1String(""), Qt::CaseInsensitive));
        QVERIFY(ref.startsWith(QLatin1String(0), Qt::CaseInsensitive));
        QVERIFY(ref.startsWith('A', Qt::CaseSensitive));
        QVERIFY(ref.startsWith(QLatin1Char('A'), Qt::CaseSensitive));
        QVERIFY(ref.startsWith(QChar('A'), Qt::CaseSensitive));
        QVERIFY(!ref.startsWith('a', Qt::CaseSensitive));
        QVERIFY(!ref.startsWith(QChar(), Qt::CaseSensitive));
        QVERIFY(!ref.startsWith(QLatin1Char(0), Qt::CaseSensitive));
    }
    {
        const QString a = QString::fromLatin1("");
        CREATE_REF(a);
        QVERIFY(ref.startsWith(""));
        QVERIFY(ref.startsWith(QString::null));
        QVERIFY(!ref.startsWith("ABC"));

        QVERIFY(ref.startsWith(QLatin1String("")));
        QVERIFY(ref.startsWith(QLatin1String(0)));
        QVERIFY(!ref.startsWith(QLatin1String("ABC")));

        QVERIFY(!ref.startsWith(QLatin1Char(0)));
        QVERIFY(!ref.startsWith(QLatin1Char('x')));
        QVERIFY(!ref.startsWith(QChar()));
    }
    {
        const QStringRef ref;
        QVERIFY(!ref.startsWith(""));
        QVERIFY(ref.startsWith(QString::null));
        QVERIFY(!ref.startsWith("ABC"));

        QVERIFY(!ref.startsWith(QLatin1String("")));
        QVERIFY(ref.startsWith(QLatin1String(0)));
        QVERIFY(!ref.startsWith(QLatin1String("ABC")));

        QVERIFY(!ref.startsWith(QLatin1Char(0)));
        QVERIFY(!ref.startsWith(QLatin1Char('x')));
        QVERIFY(!ref.startsWith(QChar()));
    }
}

void tst_QStringRef::endsWith()
{
    {
        const QString a = QString::fromLatin1("AB");
        CREATE_REF(a);
        QVERIFY(ref.endsWith("B"));
        QVERIFY(ref.endsWith("AB"));
        QVERIFY(!ref.endsWith("C"));
        QVERIFY(!ref.endsWith("ABCDEF"));
        QVERIFY(ref.endsWith(""));
        QVERIFY(ref.endsWith(QString::null));
        QVERIFY(ref.endsWith('B'));
        QVERIFY(ref.endsWith(QLatin1Char('B')));
        QVERIFY(ref.endsWith(QChar('B')));
        QVERIFY(!ref.endsWith('C'));
        QVERIFY(!ref.endsWith(QChar()));
        QVERIFY(!ref.endsWith(QLatin1Char(0)));

        QVERIFY(ref.endsWith(QLatin1String("B")));
        QVERIFY(ref.endsWith(QLatin1String("AB")));
        QVERIFY(!ref.endsWith(QLatin1String("C")));
        QVERIFY(!ref.endsWith(QLatin1String("ABCDEF")));
        QVERIFY(ref.endsWith(QLatin1String("")));
        QVERIFY(ref.endsWith(QLatin1String(0)));

        QVERIFY(ref.endsWith("B", Qt::CaseSensitive));
        QVERIFY(ref.endsWith("B", Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith("b", Qt::CaseSensitive));
        QVERIFY(ref.endsWith("b", Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith("aB", Qt::CaseSensitive));
        QVERIFY(ref.endsWith("aB", Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith("C", Qt::CaseSensitive));
        QVERIFY(!ref.endsWith("C", Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith("c", Qt::CaseSensitive));
        QVERIFY(!ref.endsWith("c", Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith("abcdef", Qt::CaseInsensitive));
        QVERIFY(ref.endsWith("", Qt::CaseInsensitive));
        QVERIFY(ref.endsWith(QString::null, Qt::CaseInsensitive));
        QVERIFY(ref.endsWith('b', Qt::CaseInsensitive));
        QVERIFY(ref.endsWith('B', Qt::CaseInsensitive));
        QVERIFY(ref.endsWith(QLatin1Char('b'), Qt::CaseInsensitive));
        QVERIFY(ref.endsWith(QChar('b'), Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith('c', Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith(QChar(), Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith(QLatin1Char(0), Qt::CaseInsensitive));

        QVERIFY(ref.endsWith(QLatin1String("B"), Qt::CaseSensitive));
        QVERIFY(ref.endsWith(QLatin1String("B"), Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith(QLatin1String("b"), Qt::CaseSensitive));
        QVERIFY(ref.endsWith(QLatin1String("b"), Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith(QLatin1String("aB"), Qt::CaseSensitive));
        QVERIFY(ref.endsWith(QLatin1String("aB"), Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith(QLatin1String("C"), Qt::CaseSensitive));
        QVERIFY(!ref.endsWith(QLatin1String("C"), Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith(QLatin1String("c"), Qt::CaseSensitive));
        QVERIFY(!ref.endsWith(QLatin1String("c"), Qt::CaseInsensitive));
        QVERIFY(!ref.endsWith(QLatin1String("abcdef"), Qt::CaseInsensitive));
        QVERIFY(ref.endsWith(QLatin1String(""), Qt::CaseInsensitive));
        QVERIFY(ref.endsWith(QLatin1String(0), Qt::CaseInsensitive));
        QVERIFY(ref.endsWith('B', Qt::CaseSensitive));
        QVERIFY(ref.endsWith(QLatin1Char('B'), Qt::CaseSensitive));
        QVERIFY(ref.endsWith(QChar('B'), Qt::CaseSensitive));
        QVERIFY(!ref.endsWith('b', Qt::CaseSensitive));
        QVERIFY(!ref.endsWith(QChar(), Qt::CaseSensitive));
        QVERIFY(!ref.endsWith(QLatin1Char(0), Qt::CaseSensitive));

    }
    {
        const QString a = QString::fromLatin1("");
        CREATE_REF(a);
        QVERIFY(ref.endsWith(""));
        QVERIFY(ref.endsWith(QString::null));
        QVERIFY(!ref.endsWith("ABC"));
        QVERIFY(!ref.endsWith(QLatin1Char(0)));
        QVERIFY(!ref.endsWith(QLatin1Char('x')));
        QVERIFY(!ref.endsWith(QChar()));

        QVERIFY(ref.endsWith(QLatin1String("")));
        QVERIFY(ref.endsWith(QLatin1String(0)));
        QVERIFY(!ref.endsWith(QLatin1String("ABC")));
    }

    {
        QStringRef ref;
        QVERIFY(!ref.endsWith(""));
        QVERIFY(ref.endsWith(QString::null));
        QVERIFY(!ref.endsWith("ABC"));

        QVERIFY(!ref.endsWith(QLatin1String("")));
        QVERIFY(ref.endsWith(QLatin1String(0)));
        QVERIFY(!ref.endsWith(QLatin1String("ABC")));

        QVERIFY(!ref.endsWith(QLatin1Char(0)));
        QVERIFY(!ref.endsWith(QLatin1Char('x')));
        QVERIFY(!ref.endsWith(QChar()));
    }
}

void tst_QStringRef::operator_eqeq_nullstring()
{
    /* Some of these might not be all that logical but it's the behaviour we've had since 3.0.0
       so we should probably stick with it. */

    QVERIFY(QStringRef() == "");
    QVERIFY("" == QStringRef());

    QVERIFY(QString("") == "");
    QVERIFY("" == QString(""));

    QVERIFY(QStringRef().size() == 0);

    QVERIFY(QString("").size() == 0);

    QVERIFY(QStringRef() == QString(""));
    QVERIFY(QString("") == QString());
}

static inline int sign(int x)
{
    return x == 0 ? 0 : (x < 0 ? -1 : 1);
}

void tst_QStringRef::compare_data()
{
    QTest::addColumn<QString>("s1");
    QTest::addColumn<QString>("s2");
    QTest::addColumn<int>("csr"); // case sensitive result
    QTest::addColumn<int>("cir"); // case insensitive result


    // null strings
    QTest::newRow("data0") << QString("") << QString("") << 0 << 0;
    QTest::newRow("data1") << QString("a") << QString("") << 1 << 1;
    QTest::newRow("data2") << QString("") << QString("a") << -1 << -1;

    // equal length
    QTest::newRow("data3") << QString("abc") << QString("abc") << 0 << 0;
    QTest::newRow("data4") << QString("abC") << QString("abc") << -1 << 0;
    QTest::newRow("data5") << QString("abc") << QString("abC") << 1 << 0;
    QTest::newRow("data10") << QString("abcdefgh") << QString("abcdefgh") << 0 << 0;
    QTest::newRow("data11") << QString("abcdefgh") << QString("abCdefgh") << 1 << 0;
    QTest::newRow("data12") << QString("0123456789012345") << QString("0123456789012345") << 0 << 0;
    QTest::newRow("data13") << QString("0123556789012345") << QString("0123456789012345") << 1 << 1;

    // different length
    QTest::newRow("data6") << QString("abcdef") << QString("abc") << 1 << 1;
    QTest::newRow("data7") << QString("abCdef") << QString("abc") << -1 << 1;
    QTest::newRow("data8") << QString("abc") << QString("abcdef") << -1 << -1;
    QTest::newRow("data14") << QString("abcdefgh") << QString("abcdefghi") << -1 << -1;
    QTest::newRow("data15") << QString("01234567890123456") << QString("0123456789012345") << 1 << 1;

    QString upper;
    upper += QChar(QChar::highSurrogate(0x10400));
    upper += QChar(QChar::lowSurrogate(0x10400));
    QString lower;
    lower += QChar(QChar::highSurrogate(0x10428));
    lower += QChar(QChar::lowSurrogate(0x10428));
    QTest::newRow("data9") << upper << lower << -1 << 0;
}

static bool isLatin(const QString &s)
{
    for (int i = 0; i < s.length(); ++i)
        if (s.at(i).unicode() > 0xff)
            return false;
    return true;
}

void tst_QStringRef::compare()
{
    QFETCH(QString, s1);
    QFETCH(QString, s2);
    QFETCH(int, csr);
    QFETCH(int, cir);

    QStringRef r1(&s1, 0, s1.length());
    QStringRef r2(&s2, 0, s2.length());

    QCOMPARE(sign(QString::compare(s1, s2)), csr);
    QCOMPARE(sign(QStringRef::compare(r1, r2)), csr);
    QCOMPARE(sign(s1.compare(s2)), csr);
    QCOMPARE(sign(s1.compare(r2)), csr);
    QCOMPARE(sign(r1.compare(r2)), csr);

    QCOMPARE(sign(s1.compare(s2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(s1.compare(s2, Qt::CaseInsensitive)), cir);
    QCOMPARE(sign(s1.compare(r2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(s1.compare(r2, Qt::CaseInsensitive)), cir);
    QCOMPARE(sign(r1.compare(r2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(r1.compare(r2, Qt::CaseInsensitive)), cir);

    QCOMPARE(sign(QString::compare(s1, s2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(QString::compare(s1, s2, Qt::CaseInsensitive)), cir);
    QCOMPARE(sign(QString::compare(s1, r2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(QString::compare(s1, r2, Qt::CaseInsensitive)), cir);
    QCOMPARE(sign(QStringRef::compare(r1, r2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(QStringRef::compare(r1, r2, Qt::CaseInsensitive)), cir);

    if (!cir) {
        QCOMPARE(s1.toCaseFolded(), s2.toCaseFolded());
    }

    if (isLatin(s2)) {
        QCOMPARE(sign(QString::compare(s1, QLatin1String(s2.toLatin1()))), csr);
        QCOMPARE(sign(QString::compare(s1, QLatin1String(s2.toLatin1()), Qt::CaseInsensitive)), cir);
        QCOMPARE(sign(QStringRef::compare(r1, QLatin1String(s2.toLatin1()))), csr);
        QCOMPARE(sign(QStringRef::compare(r1, QLatin1String(s2.toLatin1()), Qt::CaseInsensitive)), cir);
    }

    if (isLatin(s1)) {
        QCOMPARE(sign(QString::compare(QLatin1String(s1.toLatin1()), s2)), csr);
        QCOMPARE(sign(QString::compare(QLatin1String(s1.toLatin1()), s2, Qt::CaseInsensitive)), cir);
    }
}

void tst_QStringRef::compare2_data()
{
    compare_data();
}

void tst_QStringRef::compare2()
{
    QFETCH(QString, s1);
    QFETCH(QString, s2);
    QFETCH(int, csr);
    QFETCH(int, cir);

    // prepend and append data
    // we only use Latin1 here so isLatin1 still results true
    s1.prepend("xyz").append("zyx");
    s2.prepend("foobar").append("raboof");

    QStringRef r1(&s1, 3, s1.length() - 6);
    QStringRef r2(&s2, 6, s2.length() - 12);

    QCOMPARE(sign(QStringRef::compare(r1, r2)), csr);
    QCOMPARE(sign(r1.compare(r2)), csr);

    QCOMPARE(sign(r1.compare(r2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(r1.compare(r2, Qt::CaseInsensitive)), cir);

    QCOMPARE(sign(QStringRef::compare(r1, r2, Qt::CaseSensitive)), csr);
    QCOMPARE(sign(QStringRef::compare(r1, r2, Qt::CaseInsensitive)), cir);

    if (isLatin(s2)) {
        QCOMPARE(sign(QStringRef::compare(r1, QLatin1String(r2.toLatin1()))), csr);
        QCOMPARE(sign(QStringRef::compare(r1, QLatin1String(r2.toLatin1()), Qt::CaseInsensitive)), cir);
    }

    if (isLatin(s1)) {
        QCOMPARE(sign(QStringRef::compare(r2, QLatin1String(r1.toLatin1()))), -csr);
        QCOMPARE(sign(QStringRef::compare(r2, QLatin1String(r1.toLatin1()), Qt::CaseInsensitive)), -cir);
    }
}

void tst_QStringRef::toNum()
{
#define TEST_TO_INT(num, func, type) \
    a = #num; \
    b = a.leftRef(-1); \
    QCOMPARE(b.func(&ok), type(Q_INT64_C(num))); \
    QVERIFY2(ok, "Failed: num=" #num);

    QString a;
    QStringRef b;
    bool ok = false;

    TEST_TO_INT(0, toInt, int)
    TEST_TO_INT(-1, toInt, int)
    TEST_TO_INT(1, toInt, int)
    TEST_TO_INT(2147483647, toInt, int)
    TEST_TO_INT(-2147483648, toInt, int)

    TEST_TO_INT(0, toShort, short)
    TEST_TO_INT(-1, toShort, short)
    TEST_TO_INT(1, toShort, short)
    TEST_TO_INT(32767, toShort, short)
    TEST_TO_INT(-32768, toShort, short)

    TEST_TO_INT(0, toLong, long)
    TEST_TO_INT(-1, toLong, long)
    TEST_TO_INT(1, toLong, long)
    TEST_TO_INT(2147483647, toLong, long)
    TEST_TO_INT(-2147483648, toLong, long)
    TEST_TO_INT(0, toLongLong, (long long))
    TEST_TO_INT(-1, toLongLong, (long long))
    TEST_TO_INT(1, toLongLong, (long long))
    TEST_TO_INT(9223372036854775807, toLongLong, (long long))
    TEST_TO_INT(-9223372036854775807, toLongLong, (long long))

#undef TEST_TO_INT

#define TEST_TO_UINT(num, func, type) \
    a = #num; \
    b = a.leftRef(-1); \
    QCOMPARE(b.func(&ok), type(Q_UINT64_C(num))); \
    QVERIFY2(ok, "Failed: num=" #num);

    TEST_TO_UINT(0, toUInt, (unsigned int))
    TEST_TO_UINT(1, toUInt, (unsigned int))
    TEST_TO_UINT(4294967295, toUInt, (unsigned int))

    TEST_TO_UINT(0, toUShort, (unsigned short))
    TEST_TO_UINT(1, toUShort, (unsigned short))
    TEST_TO_UINT(65535, toUShort, (unsigned short))

    TEST_TO_UINT(0, toULong, (unsigned long))
    TEST_TO_UINT(1, toULong, (unsigned long))
    TEST_TO_UINT(4294967295, toULong, (unsigned long))

    TEST_TO_UINT(0, toULongLong, (unsigned long long))
    TEST_TO_UINT(1, toULongLong, (unsigned long long))
    TEST_TO_UINT(18446744073709551615, toULongLong, (unsigned long long))

#undef TEST_TO_UINT

#define TEST_BASE(str, base, num) \
    a = str; \
    b = a.leftRef(-1); \
    QCOMPARE(b.toInt(&ok,base), int(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= " #base " num=" #num ", func=toInt"); \
    QCOMPARE(b.toUInt(&ok, base), (unsigned int)(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= " #base " num=" #num ", func=toUInt"); \
    QCOMPARE(b.toShort(&ok, base), short(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= " #base " num=" #num ", func=toShort"); \
    QCOMPARE(b.toUShort(&ok, base), (unsigned short)(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= " #base " num=" #num ", func=toUShort"); \
    QCOMPARE(b.toLong(&ok, base), long(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= " #base " num=" #num ", func=toLong"); \
    QCOMPARE(b.toULong(&ok, base), (unsigned long)(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= " #base " num=" #num ", func=toULong"); \
    QCOMPARE(b.toLongLong(&ok, base), (long long)(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= " #base " num=" #num ", func=toLongLong"); \
    QCOMPARE(b.toULongLong(&ok, base), (unsigned long long)(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= " #base " num=" #num ", func=toULongLong");

    TEST_BASE("FF", 16, 255)
    TEST_BASE("0xFF", 16, 255)
    TEST_BASE("77", 8, 63)
    TEST_BASE("077", 8, 63)

    TEST_BASE("0xFF", 0, 255)
    TEST_BASE("077", 0, 63)
    TEST_BASE("255", 0, 255)

    TEST_BASE(" FF", 16, 255)
    TEST_BASE(" 0xFF", 16, 255)
    TEST_BASE(" 77", 8, 63)
    TEST_BASE(" 077", 8, 63)

    TEST_BASE(" 0xFF", 0, 255)
    TEST_BASE(" 077", 0, 63)
    TEST_BASE(" 255", 0, 255)

    TEST_BASE("\tFF\t", 16, 255)
    TEST_BASE("\t0xFF  ", 16, 255)
    TEST_BASE("   77   ", 8, 63)
    TEST_BASE("77  ", 8, 63)

#undef TEST_BASE

#define TEST_NEG_BASE(str, base, num) \
    a = str; \
    b = a.leftRef(-1); \
    QCOMPARE(b.toInt(&ok, base), int(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toInt"); \
    QCOMPARE(b.toShort(&ok,base), short(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toShort"); \
    QCOMPARE(b.toLong(&ok, base), long(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toLong"); \
    QCOMPARE(b.toLongLong(&ok, base), (long long)(num)); \
    QVERIFY2(ok, "Failed: str=" #str " base= "#base " num=" #num ", func=toLongLong");

    TEST_NEG_BASE("-FE", 16, -254)
    TEST_NEG_BASE("-0xFE", 16, -254)
    TEST_NEG_BASE("-77", 8, -63)
    TEST_NEG_BASE("-077", 8, -63)

    TEST_NEG_BASE("-0xFE", 0, -254)
    TEST_NEG_BASE("-077", 0, -63)
    TEST_NEG_BASE("-254", 0, -254)

#undef TEST_NEG_BASE

#define TEST_DOUBLE(num, str) \
    a = str; \
    b = a.leftRef(-1); \
    QCOMPARE(b.toDouble(&ok), num); \
    QVERIFY(ok);

    TEST_DOUBLE(1.2345, "1.2345")
    TEST_DOUBLE(12.345, "1.2345e+01")
    TEST_DOUBLE(12.345, "1.2345E+01")
    TEST_DOUBLE(12345.6, "12345.6")

#undef TEST_DOUBLE

#define TEST_BAD(str, func) \
    a = str; \
    b = a.leftRef(-1); \
    b.func(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str " func=" #func);

    TEST_BAD("32768", toShort)
    TEST_BAD("-32769", toShort)
    TEST_BAD("65536", toUShort)
    TEST_BAD("2147483648", toInt)
    TEST_BAD("-2147483649", toInt)
    TEST_BAD("4294967296", toUInt)
    if (sizeof(long) == 4) {
        TEST_BAD("2147483648", toLong)
        TEST_BAD("-2147483649", toLong)
        TEST_BAD("4294967296", toULong)
    }
    TEST_BAD("9223372036854775808", toLongLong)
    TEST_BAD("-9223372036854775809", toLongLong)
    TEST_BAD("18446744073709551616", toULongLong)
    TEST_BAD("-1", toUShort)
    TEST_BAD("-1", toUInt)
    TEST_BAD("-1", toULong)
    TEST_BAD("-1", toULongLong)

#undef TEST_BAD

#define TEST_BAD_ALL(str) \
    a = str; \
    b = a.leftRef(-1); \
    b.toShort(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    b.toUShort(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    b.toInt(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    b.toUInt(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    b.toLong(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    b.toULong(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    b.toLongLong(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    b.toULongLong(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    b.toFloat(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str); \
    b.toDouble(&ok); \
    QVERIFY2(!ok, "Failed: str=" #str);

    TEST_BAD_ALL((const char*)0);
    TEST_BAD_ALL("");
    TEST_BAD_ALL(" ");
    TEST_BAD_ALL(".");
    TEST_BAD_ALL("-");
    TEST_BAD_ALL("hello");
    TEST_BAD_ALL("1.2.3");
    TEST_BAD_ALL("0x0x0x");
    TEST_BAD_ALL("123-^~<");
    TEST_BAD_ALL("123ThisIsNotANumber");

#undef TEST_BAD_ALL

    a = "FF";
    b = a.leftRef(-1);
    b.toULongLong(&ok, 10);
    QVERIFY(!ok);

    a = "FF";
    b = a.leftRef(-1);
    b.toULongLong(&ok, 0);
    QVERIFY(!ok);

#ifdef QT_NO_FPU
    double d = 3.40282346638528e+38; // slightly off FLT_MAX when using hardfloats
#else
    double d = 3.4028234663852886e+38; // FLT_MAX
#endif
    QString::number(d, 'e', 17).leftRef(-1).toFloat(&ok);
    QVERIFY(ok);
    QString::number(d + 1e32, 'e', 17).leftRef(-1).toFloat(&ok);
    QVERIFY(!ok);
    a = QString::number(-d, 'e', 17).leftRef(-1).toFloat(&ok);
    QVERIFY(ok);
    QString::number(-d - 1e32, 'e', 17).leftRef(-1).toFloat(&ok);
    QVERIFY(!ok);
    QString::number(d + 1e32, 'e', 17).leftRef(-1).toDouble(&ok);
    QVERIFY(ok);
    QString::number(-d - 1e32, 'e', 17).leftRef(-1).toDouble(&ok);
    QVERIFY(ok);
}

void tst_QStringRef::toUShort()
{
    QString a;
    QStringRef b;
    bool ok;
    QCOMPARE(b.toUShort(), ushort(0));
    QCOMPARE(b.toUShort(&ok), ushort(0));
    QVERIFY(!ok);

    a = "";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(0));
    QCOMPARE(b.toUShort(&ok), ushort(0));
    QVERIFY(!ok);

    a = "COMPARE";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(0));
    QCOMPARE(b.toUShort(&ok), ushort(0));
    QVERIFY(!ok);

    a = "123";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(123));
    QCOMPARE(b.toUShort(&ok), ushort(123));
    QVERIFY(ok);

    a = "123A";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(0));
    QCOMPARE(b.toUShort(&ok), ushort(0));
    QVERIFY(!ok);

    a = "1234567";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(0));
    QCOMPARE(b.toUShort(&ok), ushort(0));
    QVERIFY(!ok);

    a = "aaa123aaa";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(0));
    QCOMPARE(b.toUShort(&ok), ushort(0));
    QVERIFY(!ok);

    a = "aaa123";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(0));
    QCOMPARE(b.toUShort(&ok), ushort(0));
    QVERIFY(!ok);

    a = "123aaa";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(0));
    QCOMPARE(b.toUShort(&ok), ushort(0));
    QVERIFY(!ok);

    a = "32767";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(32767));
    QCOMPARE(b.toUShort(&ok), ushort(32767));
    QVERIFY(ok);

    a = "-32767";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(0));
    QCOMPARE(b.toUShort(&ok), ushort(0));
    QVERIFY(!ok);

    a = "65535";
    b = a.leftRef(-1);
    QCOMPARE(b.toUShort(), ushort(65535));
    QCOMPARE(b.toUShort(&ok), ushort(65535));
    QVERIFY(ok);

    if (sizeof(short) == 2) {
        a = "65536";
        b = a.leftRef(-1);
        QCOMPARE(b.toUShort(), ushort(0));
        QCOMPARE(b.toUShort(&ok), ushort(0));
        QVERIFY(!ok);

        a = "123456";
        b = a.leftRef(-1);
        QCOMPARE(b.toUShort(), ushort(0));
        QCOMPARE(b.toUShort(&ok), ushort(0));
        QVERIFY(!ok);
    }
}

void tst_QStringRef::toShort()
{
    QString a;
    QStringRef b;
    bool ok;
    QCOMPARE(b.toShort(), short(0));
    QCOMPARE(b.toShort(&ok), short(0));
    QVERIFY(!ok);

    a = "";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(0));
    QCOMPARE(b.toShort(&ok), short(0));
    QVERIFY(!ok);

    a = "COMPARE";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(0));
    QCOMPARE(b.toShort(&ok), short(0));
    QVERIFY(!ok);

    a = "123";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(123));
    QCOMPARE(b.toShort(&ok), short(123));
    QVERIFY(ok);

    a = "123A";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(0));
    QCOMPARE(b.toShort(&ok), short(0));
    QVERIFY(!ok);

    a = "1234567";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(0));
    QCOMPARE(b.toShort(&ok), short(0));
    QVERIFY(!ok);

    a = "aaa123aaa";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(0));
    QCOMPARE(b.toShort(&ok), short(0));
    QVERIFY(!ok);

    a = "aaa123";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(0));
    QCOMPARE(b.toShort(&ok), short(0));
    QVERIFY(!ok);

    a = "123aaa";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(0));
    QCOMPARE(b.toShort(&ok), short(0));
    QVERIFY(!ok);

    a = "32767";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(32767));
    QCOMPARE(b.toShort(&ok), short(32767));
    QVERIFY(ok);

    a = "-32767";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(-32767));
    QCOMPARE(b.toShort(&ok), short(-32767));
    QVERIFY(ok);

    a = "-32768";
    b = a.leftRef(-1);
    QCOMPARE(b.toShort(), short(-32768));
    QCOMPARE(b.toShort(&ok), short(-32768));
    QVERIFY(ok);

    if (sizeof(short) == 2) {
        a = "32768";
        b = a.leftRef(-1);
        QCOMPARE(b.toShort(), short(0));
        QCOMPARE(b.toShort(&ok), short(0));
        QVERIFY(!ok);

        a = "-32769";
        b = a.leftRef(-1);
        QCOMPARE(b.toShort(), short(0));
        QCOMPARE(b.toShort(&ok), short(0));
        QVERIFY(!ok);
    }
}

void tst_QStringRef::toInt()
{
    QString a;
    QStringRef b;
    bool ok;
    QCOMPARE(b.toInt(), 0);
    QCOMPARE(b.toInt(&ok), 0);
    QVERIFY(!ok);

    a = "";
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 0);
    QCOMPARE(b.toInt(&ok), 0);
    QVERIFY(!ok);

    a = "COMPARE";
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 0);
    QCOMPARE(b.toInt(&ok), 0);
    QVERIFY(!ok);

    a = "123";
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 123);
    QCOMPARE(b.toInt(&ok), 123);
    QVERIFY(ok);

    a = "123A";
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 0);
    QCOMPARE(b.toInt(&ok), 0);
    QVERIFY(!ok);

    a = "1234567";
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 1234567);
    QCOMPARE(b.toInt(&ok), 1234567);
    QVERIFY(ok);

    a = "12345678901234";
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 0);
    QCOMPARE(b.toInt(&ok), 0);
    QVERIFY(!ok);

    a = "3234567890";
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 0);
    QCOMPARE(b.toInt(&ok), 0);
    QVERIFY(!ok);

    a = "aaa12345aaa";
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 0);
    QCOMPARE(b.toInt(&ok), 0);
    QVERIFY(!ok);

    a = "aaa12345";
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 0);
    QCOMPARE(b.toInt(&ok), 0);
    QVERIFY(!ok);

    a = "12345aaa";
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 0);
    QCOMPARE(b.toInt(&ok), 0);
    QVERIFY(!ok);

    a = "2147483647"; // 2**31 - 1
    b = a.leftRef(-1);
    QCOMPARE(b.toInt(), 2147483647);
    QCOMPARE(b.toInt(&ok), 2147483647);
    QVERIFY(ok);

    if (sizeof(int) == 4) {
        a = "-2147483647"; // -(2**31 - 1)
        b = a.leftRef(-1);
        QCOMPARE(b.toInt(), -2147483647);
        QCOMPARE(b.toInt(&ok), -2147483647);
        QVERIFY(ok);

        a = "2147483648"; // 2**31
        b = a.leftRef(-1);
        QCOMPARE(b.toInt(), 0);
        QCOMPARE(b.toInt(&ok), 0);
        QVERIFY(!ok);

        a = "-2147483648"; // -2**31
        b = a.leftRef(-1);
        QCOMPARE(b.toInt(), -2147483647 - 1);
        QCOMPARE(b.toInt(&ok), -2147483647 - 1);
        QVERIFY(ok);

        a = "2147483649"; // 2**31 + 1
        b = a.leftRef(-1);
        QCOMPARE(b.toInt(), 0);
        QCOMPARE(b.toInt(&ok), 0);
        QVERIFY(!ok);
    }
}

void tst_QStringRef::toUInt()
{
    bool ok;
    QString a;
    QStringRef b;
    a = "3234567890";
    b = a.leftRef(-1);
    QCOMPARE(b.toUInt(&ok), 3234567890u);
    QVERIFY(ok);

    a = "-50";
    b = a.leftRef(-1);
    QCOMPARE(b.toUInt(), 0u);
    QCOMPARE(b.toUInt(&ok), 0u);
    QVERIFY(!ok);

    a = "4294967295"; // 2**32 - 1
    b = a.leftRef(-1);
    QCOMPARE(b.toUInt(), 4294967295u);
    QCOMPARE(b.toUInt(&ok), 4294967295u);
    QVERIFY(ok);

    if (sizeof(int) == 4) {
        a = "4294967296"; // 2**32
        b = a.leftRef(-1);
        QCOMPARE(b.toUInt(), 0u);
        QCOMPARE(b.toUInt(&ok), 0u);
        QVERIFY(!ok);
    }
}

///////////////////////////// to*Long //////////////////////////////////////

void tst_QStringRef::toULong_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<ulong>("result");
    QTest::addColumn<bool>("ok");

    QTest::newRow("default") << QString() << 10 << 0UL << false;
    QTest::newRow("empty") << QString("") << 10 << 0UL << false;
    QTest::newRow("ulong1") << QString("3234567890") << 10 << 3234567890UL << true;
    QTest::newRow("ulong2") << QString("fFFfFfFf") << 16 << 0xFFFFFFFFUL << true;
}

void tst_QStringRef::toULong()
{
    QFETCH(QString, str);
    QFETCH(int, base);
    QFETCH(ulong, result);
    QFETCH(bool, ok);
    QStringRef strRef = str.leftRef(-1);

    bool b;
    QCOMPARE(strRef.toULong(0, base), result);
    QCOMPARE(strRef.toULong(&b, base), result);
    QCOMPARE(b, ok);
}

void tst_QStringRef::toLong_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<long>("result");
    QTest::addColumn<bool>("ok");

    QTest::newRow("default") << QString() << 10 << 0L << false;
    QTest::newRow("empty") << QString("") << 10 << 0L << false;
    QTest::newRow("normal") << QString("7fFFfFFf") << 16 << 0x7fFFfFFfL << true;
    QTest::newRow("long_max") << QString("2147483647") << 10 << 2147483647L << true;
    if (sizeof(long) == 4) {
        QTest::newRow("long_max+1") << QString("2147483648") << 10 << 0L << false;
        QTest::newRow("long_min-1") << QString("-80000001") << 16 << 0L << false;
    }
    QTest::newRow("negative") << QString("-7fffffff") << 16 << -0x7fffffffL << true;
//    QTest::newRow("long_min") << QString("-80000000") << 16 << 0x80000000uL << true;
}

void tst_QStringRef::toLong()
{
    QFETCH(QString, str);
    QFETCH(int, base);
    QFETCH(long, result);
    QFETCH(bool, ok);
    QStringRef strRef = str.leftRef(-1);

    bool b;
    QCOMPARE(strRef.toLong(0, base), result);
    QCOMPARE(strRef.toLong(&b, base), result);
    QCOMPARE(b, ok);
}


////////////////////////// to*LongLong //////////////////////////////////////

void tst_QStringRef::toULongLong()
{
    QString str;
    QStringRef strRef;
    bool ok;
    str = "18446744073709551615"; // ULLONG_MAX
    strRef = str.leftRef(-1);
    QCOMPARE(strRef.toULongLong(0), Q_UINT64_C(18446744073709551615));
    QCOMPARE(strRef.toULongLong(&ok), Q_UINT64_C(18446744073709551615));
    QVERIFY(ok);

    str = "18446744073709551616"; // ULLONG_MAX + 1
    strRef = str.leftRef(-1);
    QCOMPARE(strRef.toULongLong(0), Q_UINT64_C(0));
    QCOMPARE(strRef.toULongLong(&ok), Q_UINT64_C(0));
    QVERIFY(!ok);

    str = "-150";
    strRef = str.leftRef(-1);
    QCOMPARE(strRef.toULongLong(0), Q_UINT64_C(0));
    QCOMPARE(strRef.toULongLong(&ok), Q_UINT64_C(0));
    QVERIFY(!ok);
}

void tst_QStringRef::toLongLong()
{
    QString str;
    QStringRef strRef;
    bool ok;

    str = "9223372036854775807"; // LLONG_MAX
    strRef = str.leftRef(-1);
    QCOMPARE(strRef.toLongLong(0), Q_INT64_C(9223372036854775807));
    QCOMPARE(strRef.toLongLong(&ok), Q_INT64_C(9223372036854775807));
    QVERIFY(ok);

    str = "-9223372036854775808"; // LLONG_MIN
    strRef = str.leftRef(-1);
    QCOMPARE(strRef.toLongLong(0),
             -Q_INT64_C(9223372036854775807) - Q_INT64_C(1));
    QCOMPARE(strRef.toLongLong(&ok),
             -Q_INT64_C(9223372036854775807) - Q_INT64_C(1));
    QVERIFY(ok);

    str = "aaaa9223372036854775807aaaa";
    strRef = str.leftRef(-1);
    QCOMPARE(strRef.toLongLong(0), Q_INT64_C(0));
    QCOMPARE(strRef.toLongLong(&ok), Q_INT64_C(0));
    QVERIFY(!ok);

    str = "9223372036854775807aaaa";
    strRef = str.leftRef(-1);
    QCOMPARE(strRef.toLongLong(0), Q_INT64_C(0));
    QCOMPARE(strRef.toLongLong(&ok), Q_INT64_C(0));
    QVERIFY(!ok);

    str = "aaaa9223372036854775807";
    strRef = str.leftRef(-1);
    QCOMPARE(strRef.toLongLong(0), Q_INT64_C(0));
    QCOMPARE(strRef.toLongLong(&ok), Q_INT64_C(0));
    QVERIFY(!ok);

    static char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    for (int i = 0; i < 36; ++i) {
        for (int j = 0; j < 36; ++j) {
            for (int k = 0; k < 36; ++k) {
                QString str;
                str += QChar(digits[i]);
                str += QChar(digits[j]);
                str += QChar(digits[k]);
                strRef = str.leftRef(-1);
                qlonglong value = (((i * 36) + j) * 36) + k;
                QVERIFY(strRef.toLongLong(0, 36) == value);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////

void tst_QStringRef::toFloat()
{
    QString a;
    QStringRef b;
    bool ok;
    a = "0.000000000931322574615478515625";
    b = a.leftRef(-1);
    QCOMPARE(b.toFloat(&ok), float(0.000000000931322574615478515625));
    QVERIFY(ok);
}

void tst_QStringRef::toDouble_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<double>("result");
    QTest::addColumn<bool>("result_ok");

    QTest::newRow("ok00") << QString("0.000000000931322574615478515625") << 0.000000000931322574615478515625 << true;
    QTest::newRow("ok01") << QString(" 123.45") << 123.45 << true;

    QTest::newRow("ok02") << QString("0.1e10") << 0.1e10 << true;
    QTest::newRow("ok03") << QString("0.1e-10") << 0.1e-10 << true;

    QTest::newRow("ok04") << QString("1e10") << 1.0e10 << true;
    QTest::newRow("ok05") << QString("1e+10") << 1.0e10 << true;
    QTest::newRow("ok06") << QString("1e-10") << 1.0e-10 << true;

    QTest::newRow("ok07") << QString(" 1e10") << 1.0e10 << true;
    QTest::newRow("ok08") << QString("  1e+10") << 1.0e10 << true;
    QTest::newRow("ok09") << QString("   1e-10") << 1.0e-10 << true;

    QTest::newRow("ok10") << QString("1.") << 1.0 << true;
    QTest::newRow("ok11") << QString(".1") << 0.1 << true;

    QTest::newRow("wrong00") << QString("123.45 ") << 123.45 << true;
    QTest::newRow("wrong01") << QString(" 123.45 ") << 123.45 << true;

    QTest::newRow("wrong02") << QString("aa123.45aa") << 0.0 << false;
    QTest::newRow("wrong03") << QString("123.45aa") << 0.0 << false;
    QTest::newRow("wrong04") << QString("123erf") << 0.0 << false;

    QTest::newRow("wrong05") << QString("abc") << 0.0 << false;
    QTest::newRow("wrong06") << QString() << 0.0 << false;
    QTest::newRow("wrong07") << QString("") << 0.0 << false;
}

void tst_QStringRef::toDouble()
{
    QFETCH(QString, str);
    QFETCH(bool, result_ok);
    QStringRef strRef = str.leftRef(-1);
    bool ok;
    double d = strRef.toDouble(&ok);
    if (result_ok) {
        QTEST(d, "result");
        QVERIFY(ok);
    } else {
        QVERIFY(!ok);
    }
}

void tst_QStringRef::integer_conversion_data()
{
    QTest::addColumn<QString>("num_str");
    QTest::addColumn<int>("base");
    QTest::addColumn<bool>("good");
    QTest::addColumn<qlonglong>("num");

    QTest::newRow("C empty 0")      << QString("")         << 0  << false << (qlonglong)0;
    QTest::newRow("C empty 8")      << QString("")         << 8  << false << (qlonglong)0;
    QTest::newRow("C empty 10")     << QString("")         << 10 << false << (qlonglong)0;
    QTest::newRow("C empty 16")     << QString("")         << 16 << false << (qlonglong)0;

    QTest::newRow("C null 0")       << QString()           << 0  << false << (qlonglong)0;
    QTest::newRow("C null 8")       << QString()           << 8  << false << (qlonglong)0;
    QTest::newRow("C null 10")      << QString()           << 10 << false << (qlonglong)0;
    QTest::newRow("C null 16")      << QString()           << 16 << false << (qlonglong)0;

    QTest::newRow("C   -0xf 0")     << QString("  -0xf")   << 0  << true  << (qlonglong)-15;
    QTest::newRow("C -0xf   0")     << QString("-0xf  ")   << 0  << true  << (qlonglong)-15;
    QTest::newRow("C \t0xf\t 0")    << QString("\t0xf\t")  << 0  << true  << (qlonglong)15;
    QTest::newRow("C   -010 0")     << QString("  -010")   << 0  << true  << (qlonglong)-8;
    QTest::newRow("C 010   0")      << QString("010  ")    << 0  << true  << (qlonglong)8;
    QTest::newRow("C \t-010\t 0")   << QString("\t-010\t") << 0  << true  << (qlonglong)-8;
    QTest::newRow("C   123 10")     << QString("  123")    << 10 << true  << (qlonglong)123;
    QTest::newRow("C 123   10")     << QString("123  ")    << 10 << true  << (qlonglong)123;
    QTest::newRow("C \t123\t 10")   << QString("\t123\t")  << 10 << true  << (qlonglong)123;
    QTest::newRow("C   -0xf 16")    << QString("  -0xf")   << 16 << true  << (qlonglong)-15;
    QTest::newRow("C -0xf   16")    << QString("-0xf  ")   << 16 << true  << (qlonglong)-15;
    QTest::newRow("C \t0xf\t 16")   << QString("\t0xf\t")  << 16 << true  << (qlonglong)15;

    QTest::newRow("C -0 0")         << QString("-0")       << 0   << true << (qlonglong)0;
    QTest::newRow("C -0 8")         << QString("-0")       << 8   << true << (qlonglong)0;
    QTest::newRow("C -0 10")        << QString("-0")       << 10  << true << (qlonglong)0;
    QTest::newRow("C -0 16")        << QString("-0")       << 16  << true << (qlonglong)0;

    QTest::newRow("C 1.234 10")     << QString("1.234")    << 10 << false << (qlonglong)0;
    QTest::newRow("C 1,234 10")     << QString("1,234")    << 10 << false << (qlonglong)0;

    QTest::newRow("C 0x 0")         << QString("0x")       << 0  << false << (qlonglong)0;
    QTest::newRow("C 0x 16")        << QString("0x")       << 16 << false << (qlonglong)0;

    QTest::newRow("C 10 0")         << QString("10")       << 0  << true  << (qlonglong)10;
    QTest::newRow("C 010 0")        << QString("010")      << 0  << true  << (qlonglong)8;
    QTest::newRow("C 0x10 0")       << QString("0x10")     << 0  << true  << (qlonglong)16;
    QTest::newRow("C 10 8")         << QString("10")       << 8  << true  << (qlonglong)8;
    QTest::newRow("C 010 8")        << QString("010")      << 8  << true  << (qlonglong)8;
    QTest::newRow("C 0x10 8")       << QString("0x10")     << 8  << false << (qlonglong)0;
    QTest::newRow("C 10 10")        << QString("10")       << 10 << true  << (qlonglong)10;
    QTest::newRow("C 010 10")       << QString("010")      << 10 << true  << (qlonglong)10;
    QTest::newRow("C 0x10 10")      << QString("0x10")     << 10 << false << (qlonglong)0;
    QTest::newRow("C 10 16")        << QString("10")       << 16 << true  << (qlonglong)16;
    QTest::newRow("C 010 16")       << QString("010")      << 16 << true  << (qlonglong)16;
    QTest::newRow("C 0x10 16")      << QString("0x10")     << 16 << true  << (qlonglong)16;

    QTest::newRow("C -10 0")        << QString("-10")      << 0  << true  << (qlonglong)-10;
    QTest::newRow("C -010 0")       << QString("-010")     << 0  << true  << (qlonglong)-8;
    QTest::newRow("C -0x10 0")      << QString("-0x10")    << 0  << true  << (qlonglong)-16;
    QTest::newRow("C -10 8")        << QString("-10")      << 8  << true  << (qlonglong)-8;
    QTest::newRow("C -010 8")       << QString("-010")     << 8  << true  << (qlonglong)-8;
    QTest::newRow("C -0x10 8")      << QString("-0x10")    << 8  << false << (qlonglong)0;
    QTest::newRow("C -10 10")       << QString("-10")      << 10 << true  << (qlonglong)-10;
    QTest::newRow("C -010 10")      << QString("-010")     << 10 << true  << (qlonglong)-10;
    QTest::newRow("C -0x10 10")     << QString("-0x10")    << 10 << false << (qlonglong)0;
    QTest::newRow("C -10 16")       << QString("-10")      << 16 << true  << (qlonglong)-16;
    QTest::newRow("C -010 16")      << QString("-010")     << 16 << true  << (qlonglong)-16;
    QTest::newRow("C -0x10 16")     << QString("-0x10")    << 16 << true  << (qlonglong)-16;

    // Let's try some Arabic
    const quint16 arabic_str[] = { 0x0661, 0x0662, 0x0663, 0x0664, 0x0000 }; // "1234"
    QTest::newRow("ar_SA 1234 0")  << QString::fromUtf16(arabic_str)  << 0  << false << (qlonglong)0;
}

void tst_QStringRef::integer_conversion()
{
    QFETCH(QString, num_str);
    QFETCH(int, base);
    QFETCH(bool, good);
    QFETCH(qlonglong, num);
    QStringRef num_strRef = num_str.leftRef(-1);

    bool ok;
    qlonglong d = num_strRef.toLongLong(&ok, base);
    QCOMPARE(ok, good);

    if (ok) {
        QCOMPARE(d, num);
    }
}

void tst_QStringRef::double_conversion_data()
{
    QTest::addColumn<QString>("num_str");
    QTest::addColumn<bool>("good");
    QTest::addColumn<double>("num");

    // The good...

    QTest::newRow("C 1")             << QString("1")          << true  << 1.0;
    QTest::newRow("C 1.0")           << QString("1.0")        << true  << 1.0;
    QTest::newRow("C 1.234")         << QString("1.234")      << true  << 1.234;
    QTest::newRow("C 1.234e-10")     << QString("1.234e-10")  << true  << 1.234e-10;
    QTest::newRow("C 1.234E10")      << QString("1.234E10")   << true  << 1.234e10;
    QTest::newRow("C 1e10")          << QString("1e10")       << true  << 1.0e10;

    // The bad...

    QTest::newRow("C empty")         << QString("")           << false << 0.0;
    QTest::newRow("C null")          << QString()             << false << 0.0;
    QTest::newRow("C .")             << QString(".")          << false << 0.0;
    QTest::newRow("C 1e")            << QString("1e")         << false << 0.0;
    QTest::newRow("C 1,")            << QString("1,")         << false << 0.0;
    QTest::newRow("C 1,0")           << QString("1,0")        << false << 0.0;
    QTest::newRow("C 1,000")         << QString("1,000")      << false << 0.0;
    QTest::newRow("C 1e1.0")         << QString("1e1.0")      << false << 0.0;
    QTest::newRow("C 1e+")           << QString("1e+")        << false << 0.0;
    QTest::newRow("C 1e-")           << QString("1e-")        << false << 0.0;
    QTest::newRow("de_DE 1,0")       << QString("1,0")        << false << 0.0;
    QTest::newRow("de_DE 1,234")     << QString("1,234")      << false << 0.0;
    QTest::newRow("de_DE 1,234e-10") << QString("1,234e-10")  << false << 0.0;
    QTest::newRow("de_DE 1,234E10")  << QString("1,234E10")   << false << 0.0;

    // And the ugly...

    QTest::newRow("C .1")            << QString(".1")         << true  << 0.1;
    QTest::newRow("C -.1")           << QString("-.1")        << true  << -0.1;
    QTest::newRow("C 1.")            << QString("1.")         << true  << 1.0;
    QTest::newRow("C 1.E10")         << QString("1.E10")      << true  << 1.0e10;
    QTest::newRow("C 1e+10")         << QString("1e+10")      << true  << 1.0e+10;
    QTest::newRow("C   1")           << QString("  1")        << true  << 1.0;
    QTest::newRow("C 1  ")           << QString("1  ")        << true  << 1.0;

    // Let's try some Arabic
    const quint16 arabic_str[] = { 0x0660, 0x066B, 0x0661, 0x0662,
                                    0x0663, 0x0664, 0x0065, 0x0662,
                                    0x0000 };                            // "0.1234e2"
    QTest::newRow("ar_SA") << QString::fromUtf16(arabic_str) << false << 0.0;
}

void tst_QStringRef::double_conversion()
{
#define MY_DOUBLE_EPSILON (2.22045e-16)

    QFETCH(QString, num_str);
    QFETCH(bool, good);
    QFETCH(double, num);
    QStringRef num_strRef = num_str.leftRef(-1);

    bool ok;
    double d = num_strRef.toDouble(&ok);
    QCOMPARE(ok, good);

    if (ok) {
        double diff = d - num;
        if (diff < 0)
            diff = -diff;
        QVERIFY(diff <= MY_DOUBLE_EPSILON);
    }
}

void tst_QStringRef::trimmed()
{
    QString a;
    QStringRef b;
    a = "Text";
    b = a.leftRef(-1);
    QCOMPARE(b.compare(QStringLiteral("Text")), 0);
    QCOMPARE(b.trimmed().compare(QStringLiteral("Text")), 0);
    a = " ";
    b = a.leftRef(-1);
    QCOMPARE(b.compare(QStringLiteral(" ")), 0);
    QCOMPARE(b.trimmed().compare(QStringLiteral("")), 0);
    a = " a   ";
    b = a.leftRef(-1);
    QCOMPARE(b.trimmed().compare(QStringLiteral("a")), 0);
    a = "Text a   ";
    b = a.midRef(4);
    QCOMPARE(b.compare(QStringLiteral(" a   ")), 0);
    QCOMPARE(b.trimmed().compare(QStringLiteral("a")), 0);
}

void tst_QStringRef::truncate()
{
    const QString str = "OriginalString~";
    const QStringRef cref = str.midRef(0);
    {
        QStringRef ref = cref;
        ref.truncate(1000);
        QCOMPARE(ref, cref);
        for (int i = str.size(); i >= 0; --i) {
            ref.truncate(i);
            QCOMPARE(ref.size(), i);
            QCOMPARE(ref, cref.left(i));
        }
        QVERIFY(ref.isEmpty());
    }

    {
        QStringRef ref = cref;
        QVERIFY(!ref.isEmpty());
        ref.truncate(-1);
        QVERIFY(ref.isEmpty());
    }
}

void tst_QStringRef::left()
{
    QString originalString = "OrginalString~";
    QStringRef ref = originalString.leftRef(originalString.size() - 1);
    QCOMPARE(ref.toString(), QStringLiteral("OrginalString"));

    QVERIFY(ref.left(0).toString().isEmpty());
    QCOMPARE(ref.left(ref.size()).toString(), QStringLiteral("OrginalString"));

    QStringRef nullRef;
    QVERIFY(nullRef.isNull());
    QVERIFY(nullRef.left(3).toString().isEmpty());
    QVERIFY(nullRef.left(0).toString().isEmpty());
    QVERIFY(nullRef.left(-1).toString().isEmpty());

    QStringRef emptyRef(&originalString, 0, 0);
    QVERIFY(emptyRef.isEmpty());
    QVERIFY(emptyRef.left(3).toString().isEmpty());
    QVERIFY(emptyRef.left(0).toString().isEmpty());
    QVERIFY(emptyRef.left(-1).toString().isEmpty());

    QCOMPARE(ref.left(-1), ref);
    QCOMPARE(ref.left(100), ref);
}

void tst_QStringRef::right()
{
    QString originalString = "~OrginalString";
    QStringRef ref = originalString.rightRef(originalString.size() - 1);
    QCOMPARE(ref.toString(), QLatin1String("OrginalString"));

    QCOMPARE(ref.right(6).toString(), QLatin1String("String"));
    QCOMPARE(ref.right(ref.size()).toString(), QLatin1String("OrginalString"));
    QCOMPARE(ref.right(0).toString(), QLatin1String(""));

    QStringRef nullRef;
    QVERIFY(nullRef.isNull());
    QVERIFY(nullRef.right(3).toString().isEmpty());
    QVERIFY(nullRef.right(0).toString().isEmpty());
    QVERIFY(nullRef.right(-1).toString().isEmpty());

    QStringRef emptyRef(&originalString, 0, 0);
    QVERIFY(emptyRef.isEmpty());
    QVERIFY(emptyRef.right(3).toString().isEmpty());
    QVERIFY(emptyRef.right(0).toString().isEmpty());
    QVERIFY(emptyRef.right(-1).toString().isEmpty());

    QCOMPARE(ref.right(-1), ref);
    QCOMPARE(ref.right(100), ref);
}

void tst_QStringRef::mid()
{
    QString orig = QStringLiteral("~ABCDEFGHIEfGEFG~"); // 15 + 2 chars
    QStringRef a = orig.midRef(1, 15);
    QCOMPARE(a.size(), orig.size() - 2);

    QCOMPARE(a.mid(3,3).toString(),(QString)"DEF");
    QCOMPARE(a.mid(0,0).toString(),(QString)"");
    QVERIFY(!a.mid(15,0).toString().isNull());
    QVERIFY(a.mid(15,0).toString().isEmpty());
    QVERIFY(!a.mid(15,1).toString().isNull());
    QVERIFY(a.mid(15,1).toString().isEmpty());
    QVERIFY(a.mid(9999).toString().isEmpty());
    QVERIFY(a.mid(9999,1).toString().isEmpty());

    QCOMPARE(a.mid(-1, 6), a.mid(0, 5));
    QVERIFY(a.mid(-100, 6).isEmpty());
    QVERIFY(a.mid(INT_MIN, 0).isEmpty());
    QCOMPARE(a.mid(INT_MIN, -1), a);
    QVERIFY(a.mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(a.mid(INT_MIN + 1, INT_MAX).isEmpty());
    QCOMPARE(a.mid(INT_MIN + 2, INT_MAX), a.left(1));
    QCOMPARE(a.mid(INT_MIN + a.size() + 1, INT_MAX), a);
    QVERIFY(a.mid(INT_MAX).isNull());
    QVERIFY(a.mid(INT_MAX, INT_MAX).isNull());
    QCOMPARE(a.mid(-5, INT_MAX), a);
    QCOMPARE(a.mid(-1, INT_MAX), a);
    QCOMPARE(a.mid(0, INT_MAX), a);
    QCOMPARE(a.mid(1, INT_MAX).toString(), QString("BCDEFGHIEfGEFG"));
    QCOMPARE(a.mid(5, INT_MAX).toString(), QString("FGHIEfGEFG"));
    QVERIFY(a.mid(20, INT_MAX).isNull());
    QCOMPARE(a.mid(-1, -1), a);

    QStringRef nullRef;
    QVERIFY(nullRef.mid(3,3).toString().isEmpty());
    QVERIFY(nullRef.mid(0,0).toString().isEmpty());
    QVERIFY(nullRef.mid(9999,0).toString().isEmpty());
    QVERIFY(nullRef.mid(9999,1).toString().isEmpty());

    QVERIFY(nullRef.mid(-1, 6).isNull());
    QVERIFY(nullRef.mid(-100, 6).isNull());
    QVERIFY(nullRef.mid(INT_MIN, 0).isNull());
    QVERIFY(nullRef.mid(INT_MIN, -1).isNull());
    QVERIFY(nullRef.mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(nullRef.mid(INT_MIN + 1, INT_MAX).isNull());
    QVERIFY(nullRef.mid(INT_MIN + 2, INT_MAX).isNull());
    QVERIFY(nullRef.mid(INT_MIN + nullRef.size() + 1, INT_MAX).isNull());
    QVERIFY(nullRef.mid(INT_MAX).isNull());
    QVERIFY(nullRef.mid(INT_MAX, INT_MAX).isNull());
    QVERIFY(nullRef.mid(-5, INT_MAX).isNull());
    QVERIFY(nullRef.mid(-1, INT_MAX).isNull());
    QVERIFY(nullRef.mid(0, INT_MAX).isNull());
    QVERIFY(nullRef.mid(1, INT_MAX).isNull());
    QVERIFY(nullRef.mid(5, INT_MAX).isNull());
    QVERIFY(nullRef.mid(20, INT_MAX).isNull());
    QVERIFY(nullRef.mid(-1, -1).isNull());

    QString ninePineapples = "~Nine pineapples~";
    QStringRef x = ninePineapples.midRef(1, ninePineapples.size() - 1);
    QCOMPARE(x.mid(5, 4).toString(), QString("pine"));
    QCOMPARE(x.mid(5).toString(), QString("pineapples~"));

    QCOMPARE(x.mid(-1, 6), x.mid(0, 5));
    QVERIFY(x.mid(-100, 6).isEmpty());
    QVERIFY(x.mid(INT_MIN, 0).isEmpty());
    QCOMPARE(x.mid(INT_MIN, -1).toString(), x.toString());
    QVERIFY(x.mid(INT_MIN, INT_MAX).isNull());
    QVERIFY(x.mid(INT_MIN + 1, INT_MAX).isEmpty());
    QCOMPARE(x.mid(INT_MIN + 2, INT_MAX), x.left(1));
    QCOMPARE(x.mid(INT_MIN + x.size() + 1, INT_MAX).toString(), x.toString());
    QVERIFY(x.mid(INT_MAX).isNull());
    QVERIFY(x.mid(INT_MAX, INT_MAX).isNull());
    QCOMPARE(x.mid(-5, INT_MAX).toString(), x.toString());
    QCOMPARE(x.mid(-1, INT_MAX).toString(), x.toString());
    QCOMPARE(x.mid(0, INT_MAX), x);
    QCOMPARE(x.mid(1, INT_MAX).toString(), QString("ine pineapples~"));
    QCOMPARE(x.mid(5, INT_MAX).toString(), QString("pineapples~"));
    QVERIFY(x.mid(20, INT_MAX).isNull());
    QCOMPARE(x.mid(-1, -1), x);

    QStringRef emptyRef(&ninePineapples, 0, 0);
    QVERIFY(emptyRef.mid(1).isEmpty());
    QVERIFY(emptyRef.mid(-1).isEmpty());
    QVERIFY(emptyRef.mid(0).isEmpty());
    QVERIFY(emptyRef.mid(0, 3).isEmpty());
    QVERIFY(emptyRef.mid(-10, 3).isEmpty());
}

static bool operator ==(const QStringList &left, const QVector<QStringRef> &right)
{
    if (left.size() != right.size())
        return false;

    QStringList::const_iterator iLeft = left.constBegin();
    QVector<QStringRef>::const_iterator iRight = right.constBegin();
    for (; iLeft != left.end(); ++iLeft, ++iRight) {
        if (*iLeft != *iRight)
            return false;
    }
    return true;
}
static inline bool operator ==(const QVector<QStringRef> &left, const QStringList &right) { return right == left; }

void tst_QStringRef::split_data()
{
    QTest::addColumn<QString>("str");
    QTest::addColumn<QString>("sep");
    QTest::addColumn<QStringList>("result");

    QTest::newRow("a,b,c") << "a,b,c" << "," << (QStringList() << "a" << "b" << "c");
    QTest::newRow("a,b,c,a,b,c") << "a,b,c,a,b,c" << "," << (QStringList() << "a" << "b" << "c" << "a" << "b" << "c");
    QTest::newRow("a,b,c,,a,b,c") << "a,b,c,,a,b,c" << "," << (QStringList() << "a" << "b" << "c" << "" << "a" << "b" << "c");
    QTest::newRow("2") << QString("-rw-r--r--  1 0  0  519240 Jul  9  2002 bigfile")
                       << " "
                       << (QStringList() << "-rw-r--r--" << "" << "1" << "0" << "" << "0" << ""
                           << "519240" << "Jul" << "" << "9" << "" << "2002" << "bigfile");
    QTest::newRow("one-empty") << "" << " " << (QStringList() << "");
    QTest::newRow("two-empty") << " " << " " << (QStringList() << "" << "");
    QTest::newRow("three-empty") << "  " << " " << (QStringList() << "" << "" << "");

    QTest::newRow("all-empty") << "" << "" << (QStringList() << "" << "");
    QTest::newRow("all-null") << QString() << QString() << (QStringList() << QString() << QString());
    QTest::newRow("sep-empty") << "abc" << "" << (QStringList() << "" << "a" << "b" << "c" << "");
}

void tst_QStringRef::split()
{
    QFETCH(QString, str);
    QFETCH(QString, sep);
    QFETCH(QStringList, result);

    QVector<QStringRef> list;
    // we construct a bigger valid string to check
    // if ref.split is using the right size
    QString source = str + str + str;
    QStringRef ref = source.midRef(str.size(), str.size());
    QCOMPARE(ref.size(), str.size());

    list = ref.split(sep);
    QVERIFY(list == result);
    if (sep.size() == 1) {
        list = ref.split(sep.at(0));
        QVERIFY(list == result);
    }

    list = ref.split(sep, QString::KeepEmptyParts);
    QVERIFY(list == result);
    if (sep.size() == 1) {
        list = ref.split(sep.at(0), QString::KeepEmptyParts);
        QVERIFY(list == result);
    }

    result.removeAll("");
    list = ref.split(sep, QString::SkipEmptyParts);
    QVERIFY(list == result);
    if (sep.size() == 1) {
        list = ref.split(sep.at(0), QString::SkipEmptyParts);
        QVERIFY(list == result);
    }
}

QTEST_APPLESS_MAIN(tst_QStringRef)

#include "tst_qstringref.moc"
