/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <QTest>

#include <QByteArrayView>

class tst_QByteArrayApiSymmetry : public QObject
{
    Q_OBJECT
private slots:
    void startsWith_QByteArray_QByteArray_data() { startsWith_data(); }
    void startsWith_QByteArray_QByteArray() { startsWith_impl<QByteArray, QByteArray>(); }
    void startsWith_QByteArray_QByteArrayView_data() { startsWith_data(); }
    void startsWith_QByteArray_QByteArrayView() { startsWith_impl<QByteArray, QByteArrayView>(); }
    void startsWith_QByteArrayView_QByteArrayView_data() { startsWith_data(); }
    void startsWith_QByteArrayView_QByteArrayView() { startsWith_impl<QByteArrayView, QByteArrayView>(); }
    void startsWith_QByteArrayView_QByteArray_data() { startsWith_data(); }
    void startsWith_QByteArrayView_QByteArray() { startsWith_impl<QByteArrayView, QByteArray>(); }
    void startsWithChar_QByteArray() { startsWithChar_impl<QByteArray>(); }
    void startsWithChar_QByteArrayView() { startsWithChar_impl<QByteArrayView>(); }

    void endsWith_QByteArray_QByteArray_data() { endsWith_data(); }
    void endsWith_QByteArray_QByteArray() { endsWith_impl<QByteArray, QByteArray>(); }
    void endsWith_QByteArray_QByteArrayView_data() { endsWith_data(); }
    void endsWith_QByteArray_QByteArrayView() { endsWith_impl<QByteArray, QByteArrayView>(); }
    void endsWith_QByteArrayView_QByteArrayView_data() { endsWith_data(); }
    void endsWith_QByteArrayView_QByteArrayView() { endsWith_impl<QByteArrayView, QByteArrayView>(); }
    void endsWith_QByteArrayView_QByteArray_data() { endsWith_data(); }
    void endsWith_QByteArrayView_QByteArray() { endsWith_impl<QByteArrayView, QByteArray>(); }
    void endsWithChar_QByteArray() { endsWithChar_impl<QByteArray>(); }
    void endsWithChar_QByteArrayView() { endsWithChar_impl<QByteArrayView>(); }

    void indexOf_QByteArray_QByteArray_data() { indexOf_data(); }
    void indexOf_QByteArray_QByteArray() { indexOf_impl<QByteArray, QByteArray>(); }
    void indexOf_QByteArray_QByteArrayView_data() { indexOf_data(); }
    void indexOf_QByteArray_QByteArrayView() { indexOf_impl<QByteArray, QByteArrayView>(); }
    void indexOf_QByteArrayView_QByteArrayView_data() { indexOf_data(); }
    void indexOf_QByteArrayView_QByteArrayView() { indexOf_impl<QByteArrayView, QByteArrayView>(); }
    void indexOf_QByteArrayView_QByteArray_data() { indexOf_data(); }
    void indexOf_QByteArrayView_QByteArray() { indexOf_impl<QByteArrayView, QByteArray>(); }

    void lastIndexOf_QByteArray_QByteArray_data() { lastIndexOf_data(); }
    void lastIndexOf_QByteArray_QByteArray() { lastIndexOf_impl<QByteArray, QByteArray>(); }
    void lastIndexOf_QByteArray_QByteArrayView_data() { lastIndexOf_data(); }
    void lastIndexOf_QByteArray_QByteArrayView() { lastIndexOf_impl<QByteArray, QByteArrayView>(); }
    void lastIndexOf_QByteArrayView_QByteArrayView_data() { lastIndexOf_data(); }
    void lastIndexOf_QByteArrayView_QByteArrayView() { lastIndexOf_impl<QByteArrayView, QByteArrayView>(); }
    void lastIndexOf_QByteArrayView_QByteArray_data() { lastIndexOf_data(); }
    void lastIndexOf_QByteArrayView_QByteArray() { lastIndexOf_impl<QByteArrayView, QByteArray>(); }

    void contains_QByteArray_QByteArray_data() { contains_data(); }
    void contains_QByteArray_QByteArray() { contains_impl<QByteArray, QByteArray>(); }
    void contains_QByteArray_QByteArrayView_data() { contains_data(); }
    void contains_QByteArray_QByteArrayView() { contains_impl<QByteArray, QByteArrayView>(); }
    void contains_QByteArrayView_QByteArrayView_data() { contains_data(); }
    void contains_QByteArrayView_QByteArrayView() { contains_impl<QByteArrayView, QByteArrayView>(); }
    void contains_QByteArrayView_QByteArray_data() { contains_data(); }
    void contains_QByteArrayView_QByteArray() { contains_impl<QByteArrayView, QByteArray>(); }

    void count_QByteArray_QByteArray_data() { count_data(); }
    void count_QByteArray_QByteArray() { count_impl<QByteArray, QByteArray>(); }
    void count_QByteArray_QByteArrayView_data() { count_data(); }
    void count_QByteArray_QByteArrayView() { count_impl<QByteArray, QByteArrayView>(); }
    void count_QByteArrayView_QByteArrayView_data() { count_data(); }
    void count_QByteArrayView_QByteArrayView() { count_impl<QByteArrayView, QByteArrayView>(); }
    void count_QByteArrayView_QByteArray_data() { count_data(); }
    void count_QByteArrayView_QByteArray() { count_impl<QByteArrayView, QByteArray>(); }

    void compare_QByteArray_QByteArray_data() { compare_data(); }
    void compare_QByteArray_QByteArray() { compare_impl<QByteArray, QByteArray>(); }
    void compare_QByteArray_QByteArrayView_data() { compare_data(); }
    void compare_QByteArray_QByteArrayView() { compare_impl<QByteArray, QByteArrayView>(); }
    void compare_QByteArrayView_QByteArray_data() { compare_data(); }
    void compare_QByteArrayView_QByteArray() { compare_impl<QByteArrayView, QByteArray>(); }
    void compare_QByteArrayView_QByteArrayView_data() { compare_data(); }
    void compare_QByteArrayView_QByteArrayView() { compare_impl<QByteArrayView, QByteArrayView>(); }

    void sliced_QByteArray_data() { sliced_data(); }
    void sliced_QByteArray() { sliced_impl<QByteArray>(); }
    void sliced_QByteArrayView_data() { sliced_data(); }
    void sliced_QByteArrayView() { sliced_impl<QByteArrayView>(); }

    void first_QByteArray_data() { first_data(); }
    void first_QByteArray() { first_impl<QByteArray>(); }
    void first_QByteArrayView_data() { first_data(); }
    void first_QByteArrayView() { first_impl<QByteArrayView>(); }

    void last_QByteArray_data() { last_data(); }
    void last_QByteArray() { last_impl<QByteArray>(); }
    void last_QByteArrayView_data() { last_data(); }
    void last_QByteArrayView() { last_impl<QByteArrayView>(); }

    void chop_QByteArray_data() { chop_data(); }
    void chop_QByteArray() { chop_impl<QByteArray>(); }
    void chop_QByteArrayView_data() { chop_data(); }
    void chop_QByteArrayView() { chop_impl<QByteArrayView>(); }

private:
    void startsWith_data();
    template<typename Haystack, typename Needle> void startsWith_impl();
    template<typename Haystack> void startsWithChar_impl();

    void endsWith_data();
    template<typename Haystack, typename Needle> void endsWith_impl();
    template<typename Haystack> void endsWithChar_impl();

    void indexOf_data();
    template<typename Haystack, typename Needle> void indexOf_impl();

    void lastIndexOf_data();
    template<typename Haystack, typename Needle> void lastIndexOf_impl();

    void contains_data();
    template<typename Haystack, typename Needle> void contains_impl();

    void count_data();
    template <typename Haystack, typename Needle> void count_impl();

    void compare_data();
    template <typename LHS, typename RHS> void compare_impl();

    void sliced_data();
    template <typename ByteArray> void sliced_impl();

    void first_data();
    template <typename ByteArray> void first_impl();

    void last_data();
    template <typename ByteArray> void last_impl();

    void chop_data();
    template <typename ByteArray> void chop_impl();
};

static const auto empty = QByteArray("");
static const QByteArray null;
// the tests below rely on the fact that these objects' names match their contents:
static const auto a = QByteArrayLiteral("a");
static const auto A = QByteArrayLiteral("A");
static const auto b = QByteArrayLiteral("b");
static const auto B = QByteArrayLiteral("B");
static const auto c = QByteArrayLiteral("c");
static const auto C = QByteArrayLiteral("C");
static const auto ab = QByteArrayLiteral("ab");
static const auto aB = QByteArrayLiteral("aB");
static const auto bc = QByteArrayLiteral("bc");
static const auto bC = QByteArrayLiteral("bC");
static const auto Bc = QByteArrayLiteral("Bc");
static const auto BC = QByteArrayLiteral("BC");
static const auto abc = QByteArrayLiteral("abc");
static const auto aBc = QByteArrayLiteral("aBc");

void tst_QByteArrayApiSymmetry::startsWith_data()
{
    QTest::addColumn<QByteArray>("ba");
    QTest::addColumn<QByteArray>("sw");
    QTest::addColumn<bool>("result");

    QTest::newRow("01") << QByteArray() << QByteArray() << true;
    QTest::newRow("02") << QByteArray() << QByteArray("") << true;
    QTest::newRow("03") << QByteArray() << QByteArray("hallo") << false;

    QTest::newRow("04") << QByteArray("") << QByteArray() << true;
    QTest::newRow("05") << QByteArray("") << QByteArray("") << true;
    QTest::newRow("06") << QByteArray("") << QByteArray("h") << false;

    QTest::newRow("07") << QByteArray("hallo") << QByteArray("h") << true;
    QTest::newRow("08") << QByteArray("hallo") << QByteArray("hallo") << true;
    QTest::newRow("09") << QByteArray("hallo") << QByteArray("") << true;
    QTest::newRow("10") << QByteArray("hallo") << QByteArray("hallohallo") << false;
    QTest::newRow("11") << QByteArray("hallo") << QByteArray() << true;
}

template<typename Haystack, typename Needle>
void tst_QByteArrayApiSymmetry::startsWith_impl()
{
    QFETCH(QByteArray, ba);
    QFETCH(QByteArray, sw);
    QFETCH(bool, result);

    const Haystack haystack(ba.data(), ba.size());
    const Needle needle(sw.data(), sw.size());

    QVERIFY(haystack.startsWith(needle) == result);
    if (needle.isNull()) {
        QVERIFY(haystack.startsWith((char *)0) == result);
    } else {
        QVERIFY(haystack.startsWith(needle.data()) == result);
        if (needle.size() == 1)
            QVERIFY(haystack.startsWith(needle.at(0)) == result);
    }
}

template<typename Haystack>
void tst_QByteArrayApiSymmetry::startsWithChar_impl()
{
    QVERIFY(Haystack("hallo").startsWith('h'));
    QVERIFY(!Haystack("hallo").startsWith('\0'));
    QVERIFY(!Haystack("hallo").startsWith('o'));
    QVERIFY(Haystack("h").startsWith('h'));
    QVERIFY(!Haystack("h").startsWith('\0'));
    QVERIFY(!Haystack("h").startsWith('o'));
    QVERIFY(!Haystack("hallo").startsWith('l'));
    QVERIFY(!Haystack("").startsWith('\0'));
    QVERIFY(!Haystack("").startsWith('a'));
    QVERIFY(!Haystack().startsWith('a'));
    QVERIFY(!Haystack().startsWith('\0'));
}

void tst_QByteArrayApiSymmetry::endsWith_data()
{
    QTest::addColumn<QByteArray>("ba");
    QTest::addColumn<QByteArray>("sw");
    QTest::addColumn<bool>("result");

    QTest::newRow("01") << QByteArray() << QByteArray() << true;
    QTest::newRow("02") << QByteArray() << QByteArray("") << true;
    QTest::newRow("03") << QByteArray() << QByteArray("hallo") << false;

    QTest::newRow("04") << QByteArray("") << QByteArray() << true;
    QTest::newRow("05") << QByteArray("") << QByteArray("") << true;
    QTest::newRow("06") << QByteArray("") << QByteArray("h") << false;

    QTest::newRow("07") << QByteArray("hallo") << QByteArray("o") << true;
    QTest::newRow("08") << QByteArray("hallo") << QByteArray("hallo") << true;
    QTest::newRow("09") << QByteArray("hallo") << QByteArray("") << true;
    QTest::newRow("10") << QByteArray("hallo") << QByteArray("hallohallo") << false;
    QTest::newRow("11") << QByteArray("hallo") << QByteArray() << true;
}

template<typename Haystack, typename Needle>
void tst_QByteArrayApiSymmetry::endsWith_impl()
{
    QFETCH(QByteArray, ba);
    QFETCH(QByteArray, sw);
    QFETCH(bool, result);

    const Haystack haystack(ba.data(), ba.size());
    const Needle needle(sw.data(), sw.size());

    QVERIFY(haystack.endsWith(needle) == result);
    if (needle.isNull()) {
        QVERIFY(haystack.endsWith((char *)0) == result);
    } else {
        QVERIFY(haystack.endsWith(needle.data()) == result);
        if (needle.size() == 1)
            QVERIFY(haystack.endsWith(needle.at(0)) == result);
    }
}

template<typename Haystack>
void tst_QByteArrayApiSymmetry::endsWithChar_impl()
{
    QVERIFY(Haystack("hallo").endsWith('o'));
    QVERIFY(!Haystack("hallo").endsWith('\0'));
    QVERIFY(!Haystack("hallo").endsWith('h'));
    QVERIFY(Haystack("h").endsWith('h'));
    QVERIFY(!Haystack("h").endsWith('\0'));
    QVERIFY(!Haystack("h").endsWith('o'));
    QVERIFY(!Haystack("hallo").endsWith('l'));
    QVERIFY(!Haystack("").endsWith('\0'));
    QVERIFY(!Haystack("").endsWith('a'));
    QVERIFY(!Haystack().endsWith('a'));
    QVERIFY(!Haystack().endsWith('\0'));
}

void tst_QByteArrayApiSymmetry::indexOf_data()
{
    QTest::addColumn<QByteArray>("ba1");
    QTest::addColumn<QByteArray>("ba2");
    QTest::addColumn<int>("startpos");
    QTest::addColumn<int>("expected");

    QTest::newRow("1") << abc << a << 0 << 0;
    QTest::newRow("2") << abc << A << 0 << -1;
    QTest::newRow("3") << abc << a << 1 << -1;
    QTest::newRow("4") << abc << A << 1 << -1;
    QTest::newRow("5") << abc << b << 0 << 1;
    QTest::newRow("6") << abc << B << 0 << -1;
    QTest::newRow("7") << abc << b << 1 << 1;
    QTest::newRow("8") << abc << B << 1 << -1;
    QTest::newRow("9") << abc << b << 2 << -1;
    QTest::newRow("10") << abc << c << 0 << 2;
    QTest::newRow("11") << abc << C << 0 << -1;
    QTest::newRow("12") << abc << c << 1 << 2;
    QTest::newRow("13") << abc << C << 1 << -1;
    QTest::newRow("14") << abc << c << 2 << 2;
    QTest::newRow("15") << aBc << bc << 0 << -1;
    QTest::newRow("16") << aBc << Bc << 0 << 1;
    QTest::newRow("17") << aBc << bC << 0 << -1;
    QTest::newRow("18") << aBc << BC << 0 << -1;

    static const char h19[] = { 'x', 0x00, (char)0xe7, 0x25, 0x1c, 0x0a };
    static const char n19[] = { 0x00, 0x00, 0x01, 0x00 };
    QTest::newRow("19") << QByteArray(h19, sizeof(h19)) << QByteArray(n19, sizeof(n19)) << 0 << -1;

    QTest::newRow("empty from 0") << QByteArray("") << QByteArray("x") << 0 << -1;
    QTest::newRow("empty from -1") << QByteArray("") << QByteArray("x") << -1 << -1;
    QTest::newRow("empty from 1") << QByteArray("") << QByteArray("x") << 1 << -1;
    QTest::newRow("null from 0") << QByteArray() << QByteArray("x") << 0 << -1;
    QTest::newRow("null from -1") << QByteArray() << QByteArray("x") << -1 << -1;
    QTest::newRow("null from 1") << QByteArray() << QByteArray("x") << 1 << -1;
    QTest::newRow("null-in-null") << QByteArray() << QByteArray() << 0 << 0;
    QTest::newRow("empty-in-null") << QByteArray() << QByteArray("") << 0 << 0;
    QTest::newRow("null-in-empty") << QByteArray("") << QByteArray() << 0 << 0;
    QTest::newRow("empty-in-empty") << QByteArray("") << QByteArray("") << 0 << 0;
    QTest::newRow("empty in abc from 0") << abc << QByteArray() << 0 << 0;
    QTest::newRow("empty in abc from 2") << abc << QByteArray() << 2 << 2;
    QTest::newRow("empty in abc from 5") << abc << QByteArray() << 5 << -1;
    QTest::newRow("empty in abc from -1") << abc << QByteArray() << -1 << 2;

    QByteArray veryBigHaystack(500, 'a');
    veryBigHaystack += 'B';
    QTest::newRow("BoyerMooreStressTest") << veryBigHaystack << veryBigHaystack << 0 << 0;
    QTest::newRow("BoyerMooreStressTest2")
            << QByteArray(veryBigHaystack + 'c') << QByteArray(veryBigHaystack) << 0 << 0;
    QTest::newRow("BoyerMooreStressTest3")
            << QByteArray('c' + veryBigHaystack) << QByteArray(veryBigHaystack) << 0 << 1;
    QTest::newRow("BoyerMooreStressTest4")
            << QByteArray(veryBigHaystack) << QByteArray(veryBigHaystack + 'c') << 0 << -1;
    QTest::newRow("BoyerMooreStressTest5")
            << QByteArray(veryBigHaystack) << QByteArray('c' + veryBigHaystack) << 0 << -1;
    QTest::newRow("BoyerMooreStressTest6")
            << QByteArray('d' + veryBigHaystack) << QByteArray('c' + veryBigHaystack) << 0 << -1;
    QTest::newRow("BoyerMooreStressTest7")
            << QByteArray(veryBigHaystack + 'c') << QByteArray('c' + veryBigHaystack) << 0 << -1;
}

template<typename Haystack, typename Needle>
void tst_QByteArrayApiSymmetry::indexOf_impl()
{
    QFETCH(QByteArray, ba1);
    QFETCH(QByteArray, ba2);
    QFETCH(int, startpos);
    QFETCH(int, expected);

    const Haystack haystack(ba1.data(), ba1.size());
    const Needle needle(ba2.data(), ba2.size());

    bool hasNull = needle.contains('\0');

    QCOMPARE(haystack.indexOf(needle, startpos), expected);
    if (!hasNull)
        QCOMPARE(haystack.indexOf(needle.data(), startpos), expected);
    if (needle.size() == 1)
        QCOMPARE(haystack.indexOf(needle.at(0), startpos), expected);

    if (startpos == 0) {
        QCOMPARE(haystack.indexOf(needle), expected);
        if (!hasNull)
            QCOMPARE(haystack.indexOf(needle.data()), expected);
        if (needle.size() == 1)
            QCOMPARE(haystack.indexOf(needle.at(0)), expected);
    }
}

void tst_QByteArrayApiSymmetry::lastIndexOf_data()
{
    QTest::addColumn<QByteArray>("ba1");
    QTest::addColumn<QByteArray>("ba2");
    QTest::addColumn<int>("startpos");
    QTest::addColumn<int>("expected");

    QTest::newRow("1") << abc << a << 0 << 0;
    QTest::newRow("2") << abc << A << 0 << -1;
    QTest::newRow("3") << abc << a << 1 << 0;
    QTest::newRow("4") << abc << A << 1 << -1;
    QTest::newRow("5") << abc << a << -1 << 0;
    QTest::newRow("6") << abc << b << 0 << -1;
    QTest::newRow("7") << abc << B << 0 << -1;
    QTest::newRow("8") << abc << b << 1 << 1;
    QTest::newRow("9") << abc << B << 1 << -1;
    QTest::newRow("10") << abc << b << 2 << 1;
    QTest::newRow("11") << abc << b << -1 << 1;
    QTest::newRow("12") << abc << c << 0 << -1;
    QTest::newRow("13") << abc << C << 0 << -1;
    QTest::newRow("14") << abc << c << 1 << -1;
    QTest::newRow("15") << abc << C << 1 << -1;
    QTest::newRow("16") << abc << c << 2 << 2;
    QTest::newRow("17") << abc << c << -1 << 2;
    QTest::newRow("18") << aBc << bc << 0 << -1;
    QTest::newRow("19") << aBc << Bc << 0 << -1;
    QTest::newRow("20") << aBc << Bc << 2 << 1;
    QTest::newRow("21") << aBc << Bc << 1 << 1;
    QTest::newRow("22") << aBc << Bc << -1 << 1;
    QTest::newRow("23") << aBc << bC << 0 << -1;
    QTest::newRow("24") << aBc << BC << 0 << -1;

    static const char h25[] = { 0x00, (char)0xbc, 0x03, 0x10, 0x0a };
    static const char n25[] = { 0x00, 0x00, 0x01, 0x00 };
    QTest::newRow("25") << QByteArray(h25, sizeof(h25)) << QByteArray(n25, sizeof(n25)) << 0 << -1;

    QTest::newRow("empty from 0") << QByteArray("") << QByteArray("x") << 0 << -1;
    QTest::newRow("empty from -1") << QByteArray("") << QByteArray("x") << -1 << -1;
    QTest::newRow("empty from 1") << QByteArray("") << QByteArray("x") << 1 << -1;
    QTest::newRow("null from 0") << QByteArray() << QByteArray("x") << 0 << -1;
    QTest::newRow("null from -1") << QByteArray() << QByteArray("x") << -1 << -1;
    QTest::newRow("null from 1") << QByteArray() << QByteArray("x") << 1 << -1;
    QTest::newRow("null-in-null-off--1") << QByteArray() << QByteArray() << -1 << -1;
    QTest::newRow("null-in-null-off-0") << QByteArray() << QByteArray() << 0 << 0;
    QTest::newRow("empty-in-null-off--1") << QByteArray() << QByteArray("") << -1 << -1;
    QTest::newRow("empty-in-null-off-0") << QByteArray() << QByteArray("") << 0 << 0;
    QTest::newRow("null-in-empty-off--1") << QByteArray("") << QByteArray() << -1 << -1;
    QTest::newRow("null-in-empty-off-0") << QByteArray("") << QByteArray() << 0 << 0;
    QTest::newRow("empty-in-empty-off--1") << QByteArray("") << QByteArray("") << -1 << -1;
    QTest::newRow("empty-in-empty-off-0") << QByteArray("") << QByteArray("") << 0 << 0;
    QTest::newRow("empty in abc from 0") << abc << QByteArray() << 0 << 0;
    QTest::newRow("empty in abc from 2") << abc << QByteArray() << 2 << 2;
    QTest::newRow("empty in abc from 5")
            << abc << QByteArray() << 5 << -1; // perversely enough, should be 3?
    QTest::newRow("empty in abc from -1") << abc << QByteArray() << -1 << 3;
    QTest::newRow("empty in abc from -5")
            << abc << QByteArray() << -5 << 3; // perversely enough, should be -1?
}

template<typename Haystack, typename Needle>
void tst_QByteArrayApiSymmetry::lastIndexOf_impl()
{
    QFETCH(QByteArray, ba1);
    QFETCH(QByteArray, ba2);
    QFETCH(int, startpos);
    QFETCH(int, expected);

    const Haystack haystack(ba1.data(), ba1.size());
    const Needle needle(ba2.data(), ba2.size());

    bool hasNull = needle.contains('\0');

    QCOMPARE(haystack.lastIndexOf(needle, startpos), expected);
    if (!hasNull)
        QCOMPARE(haystack.lastIndexOf(needle.data(), startpos), expected);
    if (needle.size() == 1)
        QCOMPARE(haystack.lastIndexOf(needle.at(0), startpos), expected);

    if (startpos == haystack.size()) {
        QCOMPARE(haystack.lastIndexOf(needle), expected);
        if (!hasNull)
            QCOMPARE(haystack.lastIndexOf(needle.data()), expected);
        if (needle.size() == 1)
            QCOMPARE(haystack.lastIndexOf(needle.at(0)), expected);
    }
}

void tst_QByteArrayApiSymmetry::contains_data()
{
    QTest::addColumn<QByteArray>("ba1");
    QTest::addColumn<QByteArray>("ba2");
    QTest::addColumn<bool>("result");

    QTest::newRow("1") << abc << a << true;
    QTest::newRow("2") << abc << A << false;
    QTest::newRow("3") << abc << b << true;
    QTest::newRow("4") << abc << B << false;
    QTest::newRow("5") << abc << c << true;
    QTest::newRow("6") << abc << C << false;
    QTest::newRow("7") << aBc << Bc << true;
    QTest::newRow("8") << aBc << bc << false;

    const char withnull[] = "a\0bc";
    QTest::newRow("withnull") << QByteArray(withnull, 4) << QByteArray("bc") << true;

    QTest::newRow("empty") << QByteArray("") << QByteArray("x") << false;
    QTest::newRow("null") << QByteArray() << QByteArray("x") << false;
    QTest::newRow("null-in-null") << QByteArray() << QByteArray() << true;
    QTest::newRow("empty-in-null") << QByteArray() << QByteArray("") << true;
    QTest::newRow("null-in-empty") << QByteArray("") << QByteArray() << true;
    QTest::newRow("empty-in-empty") << QByteArray("") << QByteArray("") << true;
}

template<typename Haystack, typename Needle>
void tst_QByteArrayApiSymmetry::contains_impl()
{
    QFETCH(QByteArray, ba1);
    QFETCH(QByteArray, ba2);
    QFETCH(bool, result);

    const Haystack haystack(ba1.data(), ba1.size());
    const Needle needle(ba2.data(), ba2.size());

    QCOMPARE(haystack.contains(needle), result);
    if (needle.size() == 1)
        QCOMPARE(haystack.contains(needle.at(0)), result);
}


void tst_QByteArrayApiSymmetry::count_data()
{
    QTest::addColumn<QByteArray>("ba1");
    QTest::addColumn<QByteArray>("ba2");
    QTest::addColumn<int>("result");

    QTest::addRow("aaa") << QByteArray("aaa") << QByteArray("a") << 3;
    QTest::addRow("xyzaaaxyz") << QByteArray("xyzaaxyaxyz") << QByteArray("xyz") << 2;
    QTest::addRow("a in null") << QByteArray() << QByteArray("a") << 0;
    QTest::addRow("a in empty") << QByteArray("") << QByteArray("a") << 0;
    QTest::addRow("xyz in null") << QByteArray() << QByteArray("xyz") << 0;
    QTest::addRow("xyz in empty") << QByteArray("") << QByteArray("xyz") << 0;
    QTest::addRow("null in null") << QByteArray() << QByteArray() << 1;
    QTest::addRow("empty in empty") << QByteArray("") << QByteArray("") << 1;
    QTest::addRow("empty in null") << QByteArray() << QByteArray("") << 1;
    QTest::addRow("null in empty") << QByteArray("") << QByteArray() << 1;

    const int len = 500;
    QByteArray longData(len, 'a');
    const QByteArray needle("abcdef");
    longData.insert(0, needle);
    longData.insert(len / 2, needle);
    QTest::addRow("longInput") << longData << needle << 2;
}

template <typename Haystack, typename Needle>
void tst_QByteArrayApiSymmetry::count_impl()
{
    QFETCH(const QByteArray, ba1);
    QFETCH(const QByteArray, ba2);
    QFETCH(int, result);

    const Haystack haystack(ba1.data(), ba1.size());
    const Needle needle(ba2.data(), ba2.size());

    QCOMPARE(haystack.count(needle), result);
    if (needle.size() == 1)
        QCOMPARE(haystack.count(needle.at(0)), result);
}

void tst_QByteArrayApiSymmetry::compare_data()
{
    QTest::addColumn<QByteArray>("ba1");
    QTest::addColumn<QByteArray>("ba2");
    QTest::addColumn<int>("result");

    QTest::newRow("null")      << QByteArray() << QByteArray() << 0;
    QTest::newRow("null-empty")<< QByteArray() << QByteArray("") << 0;
    QTest::newRow("empty-null")<< QByteArray("") << QByteArray() << 0;
    QTest::newRow("null-full") << QByteArray() << QByteArray("abc") << -1;
    QTest::newRow("full-null") << QByteArray("abc") << QByteArray() << +1;
    QTest::newRow("empty-full")<< QByteArray("") << QByteArray("abc") << -1;
    QTest::newRow("full-empty")<< QByteArray("abc") << QByteArray("") << +1;
    QTest::newRow("rawempty-full") << QByteArray::fromRawData("abc", 0) << QByteArray("abc") << -1;
    QTest::newRow("full-rawempty") << QByteArray("abc") << QByteArray::fromRawData("abc", 0) << +1;

    QTest::newRow("equal   1") << QByteArray("abc") << QByteArray("abc") << 0;
    QTest::newRow("equal   2") << QByteArray::fromRawData("abc", 3) << QByteArray("abc") << 0;
    QTest::newRow("equal   3") << QByteArray::fromRawData("abcdef", 3) << QByteArray("abc") << 0;
    QTest::newRow("equal   4") << QByteArray("abc") << QByteArray::fromRawData("abc", 3) << 0;
    QTest::newRow("equal   5") << QByteArray("abc") << QByteArray::fromRawData("abcdef", 3) << 0;
    QTest::newRow("equal   6") << QByteArray("a\0bc", 4) << QByteArray("a\0bc", 4) << 0;
    QTest::newRow("equal   7") << QByteArray::fromRawData("a\0bcdef", 4) << QByteArray("a\0bc", 4) << 0;
    QTest::newRow("equal   8") << QByteArray("a\0bc", 4) << QByteArray::fromRawData("a\0bcdef", 4) << 0;

    QTest::newRow("less    1") << QByteArray("000") << QByteArray("abc") << -1;
    QTest::newRow("less    2") << QByteArray::fromRawData("00", 3) << QByteArray("abc") << -1;
    QTest::newRow("less    3") << QByteArray("000") << QByteArray::fromRawData("abc", 3) << -1;
    QTest::newRow("less    4") << QByteArray("abc", 3) << QByteArray("abc", 4) << -1;
    QTest::newRow("less    5") << QByteArray::fromRawData("abc\0", 3) << QByteArray("abc\0", 4) << -1;
    QTest::newRow("less    6") << QByteArray("a\0bc", 4) << QByteArray("a\0bd", 4) << -1;

    QTest::newRow("greater 1") << QByteArray("abc") << QByteArray("000") << +1;
    QTest::newRow("greater 2") << QByteArray("abc") << QByteArray::fromRawData("00", 3) << +1;
    QTest::newRow("greater 3") << QByteArray("abcd") << QByteArray::fromRawData("abcd", 3) << +1;
    QTest::newRow("greater 4") << QByteArray("a\0bc", 4) << QByteArray("a\0bb", 4) << +1;
}

template <typename LHS, typename RHS>
void tst_QByteArrayApiSymmetry::compare_impl()
{
    QFETCH(QByteArray, ba1);
    QFETCH(QByteArray, ba2);
    QFETCH(int, result);

    const bool isEqual   = result == 0;
    const bool isLess    = result < 0;
    const bool isGreater = result > 0;

    const LHS lhs(ba1);
    const RHS rhs(ba2);

    if constexpr (std::is_same_v<QByteArray, LHS>) {
        int cmp = lhs.compare(rhs);
        if (cmp)
            cmp = (cmp < 0 ? -1 : 1);
        QCOMPARE(cmp, result);
    }

    // basic tests:
    QCOMPARE(lhs == rhs, isEqual);
    QCOMPARE(lhs < rhs, isLess);
    QCOMPARE(lhs > rhs, isGreater);

    // composed tests:
    QCOMPARE(lhs <= rhs, isLess || isEqual);
    QCOMPARE(lhs >= rhs, isGreater || isEqual);
    QCOMPARE(lhs != rhs, !isEqual);

    // inverted tests:
    QCOMPARE(rhs == lhs, isEqual);
    QCOMPARE(rhs < lhs, isGreater);
    QCOMPARE(rhs > lhs, isLess);

    // composed, inverted tests:
    QCOMPARE(rhs <= lhs, isGreater || isEqual);
    QCOMPARE(rhs >= lhs, isLess || isEqual);
    QCOMPARE(rhs != lhs, !isEqual);

    if (isEqual)
        QVERIFY(qHash(lhs) == qHash(rhs));
}

template <typename ByteArray> ByteArray detached(ByteArray b)
{
    if (!b.isNull()) { // detaching loses nullness, but we need to preserve it
        auto d = b.data();
        Q_UNUSED(d);
    }
    return b;
}

void tst_QByteArrayApiSymmetry::sliced_data()
{
    QTest::addColumn<QByteArray>("ba");
    QTest::addColumn<int>("pos");
    QTest::addColumn<int>("n");
    QTest::addColumn<QByteArray>("result1");
    QTest::addColumn<QByteArray>("result2");

    QTest::addRow("empty") << empty << 0 << 0 << empty << empty;
#define ROW(base, p, n, r1, r2) \
    QTest::addRow("%s%d%d", #base, p, n) << base << p << n << r1 << r2

    ROW(a, 0, 0, a, empty);
    ROW(a, 0, 1, a, a);
    ROW(a, 1, 0, empty, empty);

    ROW(ab, 0, 0, ab, empty);
    ROW(ab, 0, 1, ab, a);
    ROW(ab, 0, 2, ab, ab);
    ROW(ab, 1, 0, b,  empty);
    ROW(ab, 1, 1, b,  b);
    ROW(ab, 2, 0, empty, empty);

    ROW(abc, 0, 0, abc, empty);
    ROW(abc, 0, 1, abc, a);
    ROW(abc, 0, 2, abc, ab);
    ROW(abc, 0, 3, abc, abc);
    ROW(abc, 1, 0, bc,  empty);
    ROW(abc, 1, 1, bc,  b);
    ROW(abc, 1, 2, bc,  bc);
    ROW(abc, 2, 0, c,   empty);
    ROW(abc, 2, 1, c,   c);
    ROW(abc, 3, 0, empty, empty);
#undef ROW
}

template <typename ByteArray>
void tst_QByteArrayApiSymmetry::sliced_impl()
{
    QFETCH(const QByteArray, ba);
    QFETCH(const int, pos);
    QFETCH(const int, n);
    QFETCH(const QByteArray, result1);
    QFETCH(const QByteArray, result2);

    const ByteArray b(ba);
    {
        const auto sliced1 = b.sliced(pos);
        const auto sliced2 = b.sliced(pos, n);

        QCOMPARE(sliced1, result1);
        QCOMPARE(sliced1.isNull(), result1.isNull());
        QCOMPARE(sliced1.isEmpty(), result1.isEmpty());

        QCOMPARE(sliced2, result2);
        QCOMPARE(sliced2.isNull(), result2.isNull());
        QCOMPARE(sliced2.isEmpty(), result2.isEmpty());
    }

    {
        const auto sliced1 = detached(b).sliced(pos);
        const auto sliced2 = detached(b).sliced(pos, n);

        QCOMPARE(sliced1, result1);
        QCOMPARE(sliced1.isNull(), result1.isNull());
        QCOMPARE(sliced1.isEmpty(), result1.isEmpty());

        QCOMPARE(sliced2, result2);
        QCOMPARE(sliced2.isNull(), result2.isNull());
        QCOMPARE(sliced2.isEmpty(), result2.isEmpty());
    }
}

void tst_QByteArrayApiSymmetry::first_data()
{
    QTest::addColumn<QByteArray>("ba");
    QTest::addColumn<int>("n");
    QTest::addColumn<QByteArray>("result");

    QTest::addRow("empty") << empty << 0 << empty;

#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << base << n << res

    ROW(a, 0, empty);
    ROW(a, 1, a);

    ROW(ab, 0, empty);
    ROW(ab, 1, a);
    ROW(ab, 2, ab);

    ROW(abc, 0, empty);
    ROW(abc, 1, a);
    ROW(abc, 2, ab);
    ROW(abc, 3, abc);
#undef ROW
}

template <typename ByteArray>
void tst_QByteArrayApiSymmetry::first_impl()
{
    QFETCH(const QByteArray, ba);
    QFETCH(const int, n);
    QFETCH(const QByteArray, result);

    const ByteArray b(ba);
    {
        const auto first = b.first(n);

        QCOMPARE(first, result);
        QCOMPARE(first.isNull(), result.isNull());
        QCOMPARE(first.isEmpty(), result.isEmpty());
    }
    {
        const auto first = detached(b).first(n);

        QCOMPARE(first, result);
        QCOMPARE(first.isNull(), result.isNull());
        QCOMPARE(first.isEmpty(), result.isEmpty());
    }
    {
        auto first = b;
        first.truncate(n);

        QCOMPARE(first, result);
        QCOMPARE(first.isNull(), result.isNull());
        QCOMPARE(first.isEmpty(), result.isEmpty());
    }
}

void tst_QByteArrayApiSymmetry::last_data()
{
    QTest::addColumn<QByteArray>("ba");
    QTest::addColumn<int>("n");
    QTest::addColumn<QByteArray>("result");

    QTest::addRow("empty") << empty << 0 << empty;

#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << base << n << res

    ROW(a, 0, empty);
    ROW(a, 1, a);

    ROW(ab, 0, empty);
    ROW(ab, 1, b);
    ROW(ab, 2, ab);

    ROW(abc, 0, empty);
    ROW(abc, 1, c);
    ROW(abc, 2, bc);
    ROW(abc, 3, abc);
#undef ROW
}

template <typename ByteArray>
void tst_QByteArrayApiSymmetry::last_impl()
{
    QFETCH(const QByteArray, ba);
    QFETCH(const int, n);
    QFETCH(const QByteArray, result);

    const ByteArray b(ba);

    {
        const auto last = b.last(n);

        QCOMPARE(last, result);
        QCOMPARE(last.isNull(), result.isNull());
        QCOMPARE(last.isEmpty(), result.isEmpty());
    }
    {
        const auto last = detached(b).last(n);

        QCOMPARE(last, result);
        QCOMPARE(last.isNull(), result.isNull());
        QCOMPARE(last.isEmpty(), result.isEmpty());
    }
}

void tst_QByteArrayApiSymmetry::chop_data()
{
    QTest::addColumn<QByteArray>("ba");
    QTest::addColumn<int>("n");
    QTest::addColumn<QByteArray>("result");

    QTest::addRow("empty") << empty << 0 << empty;

    // Some classes' truncate() implementations have a wide contract, others a narrow one
    // so only test valid arguents here:
#define ROW(base, n, res) \
    QTest::addRow("%s%d", #base, n) << base << n << res

    ROW(a, 0, a);
    ROW(a, 1, empty);

    ROW(ab, 0, ab);
    ROW(ab, 1, a);
    ROW(ab, 2, empty);

    ROW(abc, 0, abc);
    ROW(abc, 1, ab);
    ROW(abc, 2, a);
    ROW(abc, 3, empty);
#undef ROW
}

template <typename ByteArray>
void tst_QByteArrayApiSymmetry::chop_impl()
{
    QFETCH(const QByteArray, ba);
    QFETCH(const int, n);
    QFETCH(const QByteArray, result);

    const ByteArray b(ba);

    {
        const auto chopped = b.chopped(n);

        QCOMPARE(chopped, result);
        QCOMPARE(chopped.isNull(), result.isNull());
        QCOMPARE(chopped.isEmpty(), result.isEmpty());
    }
    {
        const auto chopped = detached(b).chopped(n);

        QCOMPARE(chopped, result);
        QCOMPARE(chopped.isNull(), result.isNull());
        QCOMPARE(chopped.isEmpty(), result.isEmpty());
    }
    {
        auto chopped = b;
        chopped.chop(n);

        QCOMPARE(chopped, result);
        QCOMPARE(chopped.isNull(), result.isNull());
        QCOMPARE(chopped.isEmpty(), result.isEmpty());
    }
}

QTEST_APPLESS_MAIN(tst_QByteArrayApiSymmetry)
#include "tst_qbytearrayapisymmetry.moc"
