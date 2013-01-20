/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
    void length_data();
    void length();
    void isEmpty();
    void compare_data();
    void compare();
    void operator_eqeq_nullstring();
    void trimmed();
};

static QStringRef emptyRef()
{
    static const QString empty("");
    return empty.midRef(0);
}

#define CREATE_REF(string)                                              \
    const QString padded = QString::fromLatin1(" %1 ").arg(string);     \
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
    s2.prepend("C");
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

    const QString haystackPadded = QString::fromLatin1(" %1 ").arg(haystack);
    const QString needlePadded = QString::fromLatin1(" %1 ").arg(needle);
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

    const QString haystackPadded = QString::fromLatin1(" %1 ").arg(haystack);
    const QString needlePadded = QString::fromLatin1(" %1 ").arg(needle);
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

    const QString haystackPadded = QString::fromLatin1(" %1 ").arg(haystack);
    const QString needlePadded = QString::fromLatin1(" %1 ").arg(needle);
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

    // different length
    QTest::newRow("data6") << QString("abcdef") << QString("abc") << 1 << 1;
    QTest::newRow("data7") << QString("abCdef") << QString("abc") << -1 << 1;
    QTest::newRow("data8") << QString("abc") << QString("abcdef") << -1 << -1;

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

QTEST_APPLESS_MAIN(tst_QStringRef)

#include "tst_qstringref.moc"
