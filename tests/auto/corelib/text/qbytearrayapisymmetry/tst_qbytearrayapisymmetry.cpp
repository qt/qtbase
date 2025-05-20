// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QByteArrayView>

#include <limits>

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

    void trimmed_QByteArray_data() { trimmed_data(); }
    void trimmed_QByteArray() { trimmed_impl<QByteArray>(); }
    void trimmed_QByteArrayView_data() { trimmed_data(); }
    void trimmed_QByteArrayView() { trimmed_impl<QByteArrayView>(); }

    void toShort_QByteArray() const { toShort<QByteArray>(); }
    void toShort_QByteArrayView() const { toShort<QByteArrayView>(); }
    void toUShort_QByteArray() const { toUShort<QByteArray>(); }
    void toUShort_QByteArrayView() const { toUShort<QByteArrayView>(); }
    void toInt_QByteArray_data() const { toInt_data(); }
    void toInt_QByteArrayView_data() const { toInt_data(); }
    void toInt_QByteArray() const { toInt<QByteArray>(); }
    void toInt_QByteArrayView() const { toInt<QByteArrayView>(); }
    void toUInt_QByteArray_data() const { toUInt_data(); }
    void toUInt_QByteArrayView_data() const { toUInt_data(); }
    void toUInt_QByteArray() const { toUInt<QByteArray>(); }
    void toUInt_QByteArrayView() const { toUInt<QByteArrayView>(); }
    void toLong_QByteArray_data() const { toLong_data(); }
    void toLong_QByteArrayView_data() const { toLong_data(); }
    void toLong_QByteArray() const { toLong<QByteArray>(); }
    void toLong_QByteArrayView() const { toLong<QByteArrayView>(); }
    void toULong_QByteArray_data() const { toULong_data(); }
    void toULong_QByteArrayView_data() const { toULong_data(); }
    void toULong_QByteArray() const { toULong<QByteArray>(); }
    void toULong_QByteArrayView() const { toULong<QByteArrayView>(); }
    void toLongLong_QByteArray_data() const { toLongLong_data(); }
    void toLongLong_QByteArrayView_data() const { toLongLong_data(); }
    void toLongLong_QByteArray() const { toLongLong<QByteArray>(); }
    void toLongLong_QByteArrayView() const { toLongLong<QByteArrayView>(); }
    void toULongLong_QByteArray_data() const { toULongLong_data(); }
    void toULongLong_QByteArrayView_data() const { toULongLong_data(); }
    void toULongLong_QByteArray() const { toULongLong<QByteArray>(); }
    void toULongLong_QByteArrayView() const { toULongLong<QByteArrayView>(); }
    void toFloat_QByteArray() const { toFloat<QByteArray>(); }
    void toFloat_QByteArrayView() const { toFloat<QByteArrayView>(); }
    void toDouble_QByteArray_data() const { toDouble_data(); }
    void toDouble_QByteArrayView_data() const { toDouble_data(); }
    void toDouble_QByteArray() const { toDouble<QByteArray>(); }
    void toDouble_QByteArrayView() const { toDouble<QByteArrayView>(); }

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

    void trimmed_data();
    template <typename ByteArray> void trimmed_impl();

    template <typename ByteArray> void toShort() const;
    template <typename ByteArray> void toUShort() const;
    void toInt_data() const;
    template <typename ByteArray> void toInt() const;
    void toUInt_data() const;
    template <typename ByteArray> void toUInt() const;
    void toLong_data() const;
    template <typename ByteArray> void toLong() const;
    void toULong_data() const;
    template <typename ByteArray> void toULong() const;
    void toLongLong_data() const;
    template <typename ByteArray> void toLongLong() const;
    void toULongLong_data() const;
    template <typename ByteArray> void toULongLong() const;
    template <typename ByteArray> void toFloat() const;
    void toDouble_data() const;
    template <typename ByteArray> void toDouble() const;
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

void tst_QByteArrayApiSymmetry::trimmed_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("null") << QByteArray() << QByteArray();
    QTest::newRow("empty") << QByteArray("") << QByteArray("");
    QTest::newRow("no end-spaces") << QByteArray("a b\nc\td") << QByteArray("a b\nc\td");
    QTest::newRow("with end-spaces")
        << QByteArray("\t \v a b\r\nc \td\ve   f \r\n\f") << QByteArray("a b\r\nc \td\ve   f");
    QTest::newRow("all spaces") << QByteArray("\t \r \n \v \f") << QByteArray("");
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::trimmed_impl()
{
    QFETCH(QByteArray, source);
    QFETCH(QByteArray, expected);

    QCOMPARE(ByteArray(source).trimmed(), ByteArray(expected));
    ByteArray copy{source};
    QCOMPARE(std::move(copy).trimmed(), ByteArray(expected));

    if constexpr (std::is_same_v<QByteArray, ByteArray>) {
        if (source.isEmpty())
            QVERIFY(!source.isDetached());
    }
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::toShort() const
{
    bool ok = true; // opposite to the first expected result

    QCOMPARE(ByteArray().toShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(ByteArray("").toShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(ByteArray("12345").toShort(&ok), 12345);
    QVERIFY(ok);

    QCOMPARE(ByteArray("-12345").toShort(&ok), -12345);
    QVERIFY(ok);

    QCOMPARE(ByteArray("-12345 and a bit", 5).toShort(&ok), -1234);
    QVERIFY(ok);

    QCOMPARE(ByteArray("-12345 and a bit").sliced(1, 4).toShort(&ok), 1234);
    QVERIFY(ok);

    QCOMPARE(ByteArray("-012345 and a bit", 2).toShort(&ok), 0);
    QVERIFY(ok);

    QCOMPARE(ByteArray("-12345 and a bit", 6).toShort(&ok), -12345);
    QVERIFY(ok);

    QCOMPARE(ByteArray("-12345 and a bit", 7).toShort(&ok), -12345);
    QVERIFY(ok);

    QCOMPARE(ByteArray("12345 and a bit", 10).toShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(ByteArray("32767").toShort(&ok), 32767);
    QVERIFY(ok);

    QCOMPARE(ByteArray("-32768").toShort(&ok), -32768);
    QVERIFY(ok);

    QCOMPARE(ByteArray("32768").toShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(ByteArray("-32769").toShort(&ok), 0);
    QVERIFY(!ok);
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::toUShort() const
{
    bool ok = true; // opposite to the first expected result

    QCOMPARE(ByteArray().toUShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(ByteArray("").toUShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(ByteArray("12345").toUShort(&ok), 12345);
    QVERIFY(ok);

    QCOMPARE(ByteArray("12345 and a bit", 4).toUShort(&ok), 1234);
    QVERIFY(ok);

    QCOMPARE(ByteArray("012345 and a bit").sliced(1, 4).toUShort(&ok), 1234);
    QVERIFY(ok);

    QCOMPARE(ByteArray("012345 and a bit", 1).toUShort(&ok), 0);
    QVERIFY(ok);

    QCOMPARE(ByteArray("12345 and a bit", 5).toUShort(&ok), 12345);
    QVERIFY(ok);

    QCOMPARE(ByteArray("12345 and a bit", 6).toUShort(&ok), 12345);
    QVERIFY(ok);

    QCOMPARE(ByteArray("12345 and a bit", 10).toUShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(ByteArray("-12345").toUShort(&ok), 0);
    QVERIFY(!ok);

    QCOMPARE(ByteArray("32767").toUShort(&ok), 32767);
    QVERIFY(ok);

    QCOMPARE(ByteArray("32768").toUShort(&ok), 32768);
    QVERIFY(ok);

    QCOMPARE(ByteArray("65535").toUShort(&ok), 65535);
    QVERIFY(ok);

    QCOMPARE(ByteArray("65536").toUShort(&ok), 0);
    QVERIFY(!ok);
}

// defined later
extern const char globalChar;

void tst_QByteArrayApiSymmetry::toInt_data() const
{
    QTest::addColumn<QByteArray>("string");
    QTest::addColumn<int>("base");
    QTest::addColumn<int>("expectednumber");
    QTest::addColumn<bool>("expectedok");

    QTest::newRow("null") << QByteArray() << 10 << 0 << false;
    QTest::newRow("empty") << QByteArray("") << 10 << 0 << false;

    QTest::newRow("base 10") << QByteArray("100") << 10 << 100 << true;
    QTest::newRow("base 16-1") << QByteArray("100") << 16 << 256 << true;
    QTest::newRow("base 16-2") << QByteArray("0400") << 16 << 1024 << true;
    QTest::newRow("base 2") << QByteArray("1111") << 2 << 15 << true;
    QTest::newRow("base 8") << QByteArray("100") << 8 << 64 << true;
    QTest::newRow("base 0-1") << QByteArray("0x10") << 0 << 16 << true;
    QTest::newRow("base 0-2") << QByteArray("10") << 0 << 10 << true;
    QTest::newRow("base 0-3") << QByteArray("010") << 0 << 8 << true;
    QTest::newRow("base 0 empty") << QByteArray() << 0 << 0 << false;

    QTest::newRow("leading space") << QByteArray(" 100") << 10 << 100 << true;
    QTest::newRow("trailing space") << QByteArray("100 ") << 10 << 100 << true;
    QTest::newRow("leading junk") << QByteArray("x100") << 10 << 0 << false;
    QTest::newRow("trailing junk") << QByteArray("100x") << 10 << 0 << false;

    // using fromRawData
    QTest::newRow("raw1") << QByteArray::fromRawData("1", 1) << 10 << 1 << true;
    QTest::newRow("raw2") << QByteArray::fromRawData("1foo", 1) << 10 << 1 << true;
    QTest::newRow("raw3") << QByteArray::fromRawData("12", 1) << 10 << 1 << true;
    QTest::newRow("raw4") << QByteArray::fromRawData("123456789", 1) << 10 << 1 << true;
    QTest::newRow("raw5") << QByteArray::fromRawData("123456789", 2) << 10 << 12 << true;

    QTest::newRow("raw-static") << QByteArray::fromRawData(&globalChar, 1) << 10 << 1 << true;
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::toInt() const
{
    QFETCH(QByteArray, string);
    QFETCH(int, base);
    QFETCH(int, expectednumber);
    QFETCH(bool, expectedok);

    bool ok;
    int number = ByteArray(string).toInt(&ok, base);

    QCOMPARE(ok, expectedok);
    QCOMPARE(number, expectednumber);
}

void tst_QByteArrayApiSymmetry::toUInt_data() const
{
    QTest::addColumn<QByteArray>("string");
    QTest::addColumn<int>("base");
    QTest::addColumn<uint>("expectednumber");
    QTest::addColumn<bool>("expectedok");

    QTest::newRow("null") << QByteArray() << 10 << 0u << false;
    QTest::newRow("empty") << QByteArray("") << 10 << 0u << false;

    QTest::newRow("negative value") << QByteArray("-50") << 10 << 0u << false;
    QTest::newRow("more than MAX_INT") << QByteArray("3234567890") << 10 << 3234567890u << true;
    QTest::newRow("2^32 - 1") << QByteArray("4294967295") << 10 << 4294967295u << true;
    if constexpr (sizeof(int) > 4)
        QTest::newRow("2^32") << QByteArray("4294967296") << 10 << (1u << 32) << true;
    else
        QTest::newRow("2^32") << QByteArray("4294967296") << 10 << 0u << false;
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::toUInt() const
{
    QFETCH(QByteArray, string);
    QFETCH(int, base);
    QFETCH(uint, expectednumber);
    QFETCH(bool, expectedok);

    bool ok;
    const uint number = ByteArray(string).toUInt(&ok, base);

    QCOMPARE(ok, expectedok);
    QCOMPARE(number, expectednumber);
}

void tst_QByteArrayApiSymmetry::toLong_data() const
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<long>("result");
    QTest::addColumn<bool>("ok");

    QTest::newRow("null") << QByteArray() << 10 << 0L << false;
    QTest::newRow("empty") << QByteArray("") << 16 << 0L << false;
    QTest::newRow("in range dec") << QByteArray("1608507359") << 10 << 1608507359L << true;
    QTest::newRow("in range dec neg") << QByteArray("-1608507359") << 10 << -1608507359L << true;
    QTest::newRow("in range hex") << QByteArray("12ABCDEF") << 16 << 0x12ABCDEFL << true;
    QTest::newRow("in range hex neg") << QByteArray("-12ABCDEF") << 16 << -0x12ABCDEFL << true;
    QTest::newRow("Fibonacci's last int32")
        << QByteArray("1836311903") << 10 << 1836311903L << true;

    QTest::newRow("leading spaces") << QByteArray(" \r\n\tABC123") << 16 << 0xABC123L << true;
    QTest::newRow("trailing spaces") << QByteArray("1234567\t\r \n") << 10 << 1234567L << true;
    QTest::newRow("leading junk") << QByteArray("q12345") << 10 << 0L << false;
    QTest::newRow("trailing junk") << QByteArray("abc12345t") << 16 << 0L << false;

    QTest::newRow("dec with base 0") << QByteArray("123") << 0 << 123L << true;
    QTest::newRow("neg dec with base 0") << QByteArray("-123") << 0 << -123L << true;
    QTest::newRow("hex with base 0") << QByteArray("0x123") << 0 << 0x123L << true;
    QTest::newRow("neg hex with base 0") << QByteArray("-0x123") << 0 << -0x123L << true;
    QTest::newRow("oct with base 0") << QByteArray("0123") << 0 << 0123L << true;
    QTest::newRow("neg oct with base 0") << QByteArray("-0123") << 0 << -0123L << true;

    QTest::newRow("base 3") << QByteArray("12012") << 3 << 140L << true;
    QTest::newRow("neg base 3") << QByteArray("-201") << 3 << -19L << true;

    using Bounds = std::numeric_limits<long>;
    QTest::newRow("long max") << QByteArray::number(Bounds::max()) << 10 << Bounds::max() << true;
    QTest::newRow("long min") << QByteArray::number(Bounds::min()) << 10 << Bounds::min() << true;

    using B32 = std::numeric_limits<qint32>;
    QTest::newRow("int32 min bin")
        << (QByteArray("-1") + QByteArray(31, '0')) << 2 << long(B32::min()) << true;
    QTest::newRow("int32 max bin") << QByteArray(31, '1') << 2 << long(B32::max()) << true;
    QTest::newRow("int32 min hex") << QByteArray("-80000000") << 16 << long(B32::min()) << true;
    QTest::newRow("int32 max hex") << QByteArray("7fffffff") << 16 << long(B32::max()) << true;
    QTest::newRow("int32 min dec") << QByteArray("-2147483648") << 10 << long(B32::min()) << true;
    QTest::newRow("int32 max dec") << QByteArray("2147483647") << 10 << long(B32::max()) << true;

    if constexpr (sizeof(long) < sizeof(qlonglong)) {
        QT_WARNING_PUSH
        // See: https://github.com/llvm/llvm-project/issues/59448
        QT_WARNING_DISABLE_CLANG("-Winteger-overflow")
        const qlonglong longMaxPlusOne = static_cast<qlonglong>(Bounds::max()) + 1;
        const qlonglong longMinMinusOne = static_cast<qlonglong>(Bounds::min()) - 1;
        QT_WARNING_POP

        QTest::newRow("long max + 1") << QByteArray::number(longMaxPlusOne) << 10 << 0L << false;
        QTest::newRow("long min - 1") << QByteArray::number(longMinMinusOne) << 10 << 0L << false;
    }
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::toLong() const
{
    QFETCH(QByteArray, str);
    QFETCH(int, base);
    QFETCH(long, result);
    QFETCH(bool, ok);

    bool good;
    QCOMPARE(ByteArray(str).toLong(nullptr, base), result);
    QCOMPARE(ByteArray(str).toLong(&good, base), result);
    QCOMPARE(good, ok);
    if (base == 10) {
        // check that by default base is assumed to be 10
        QCOMPARE(ByteArray(str).toLong(&good), result);
        QCOMPARE(good, ok);
    }
}

void tst_QByteArrayApiSymmetry::toULong_data() const
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<ulong>("result");
    QTest::addColumn<bool>("ok");

    ulong LongMaxPlusOne = (ulong)LONG_MAX + 1;
    QTest::newRow("LONG_MAX+1")
        << QString::number(LongMaxPlusOne).toUtf8() << 10 << LongMaxPlusOne << true;
    QTest::newRow("null") << QByteArray() << 10 << 0UL << false;
    QTest::newRow("empty") << QByteArray("") << 10 << 0UL << false;
    QTest::newRow("ulong1") << QByteArray("3234567890") << 10 << 3234567890UL << true;
    QTest::newRow("ulong2") << QByteArray("fFFfFfFf") << 16 << 0xFFFFFFFFUL << true;

    QTest::newRow("leading spaces") << QByteArray(" \n\r\t100") << 10 << 100UL << true;
    QTest::newRow("trailing spaces") << QByteArray("100 \n\r\t") << 10 << 100UL << true;
    QTest::newRow("leading junk") << QByteArray("x100") << 10 << 0UL << false;
    QTest::newRow("trailing junk") << QByteArray("100x") << 10 << 0UL << false;
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::toULong() const
{
    QFETCH(QByteArray, str);
    QFETCH(int, base);
    QFETCH(ulong, result);
    QFETCH(bool, ok);

    bool good;
    QCOMPARE(ByteArray(str).toULong(0, base), result);
    QCOMPARE(ByteArray(str).toULong(&good, base), result);
    QCOMPARE(good, ok);
}

static QByteArray decNext(QByteArray &&big)
{
    // Increments a decimal digit-string (ignoring sign, so decrements if
    // negative); only intended for taking a boundary value just out of range,
    // so big is never a string of only 9s (that'd be one less than a power of
    // ten, which cannot be a power of two, as odd, or one less than one, as the
    // power of ten isn't a power of two).
    int i = big.size() - 1;
    while (big.at(i) == '9')
        big[i--] = '0';
    big[i] += 1;
    return std::move(big);
}

void tst_QByteArrayApiSymmetry::toLongLong_data() const
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<qlonglong>("result");
    QTest::addColumn<bool>("ok");

    QTest::newRow("null") << QByteArray() << 10 << 0LL << false;
    QTest::newRow("empty") << QByteArray("") << 10 << 0LL << false;
    QTest::newRow("out of base bound") << QByteArray("c") << 10 << 0LL << false;

    QTest::newRow("in range dec")
        << QByteArray("7679359922672374856") << 10 << 7679359922672374856LL << true;
    QTest::newRow("in range dec neg")
        << QByteArray("-7679359922672374856") << 10 << -7679359922672374856LL << true;
    QTest::newRow("in range hex")
        << QByteArray("6A929129A5421448") << 16 << 0x6A929129A5421448LL << true;
    QTest::newRow("in range hex prefix")
        << QByteArray("0x6A929129A5421448") << 16 << 0x6A929129A5421448LL << true;
    QTest::newRow("in range hex neg")
        << QByteArray("-6A929129A5421448") << 16 << -0x6A929129A5421448LL << true;
    QTest::newRow("in range hex prefix neg")
        << QByteArray("-0x6A929129A5421448") << 16 << -0x6A929129A5421448LL << true;
    QTest::newRow("Fibonacci's last int64")
        << QByteArray("7540113804746346429") << 10 << 7540113804746346429LL << true;

    QTest::newRow("leading spaces")
        << QByteArray(" \r\n\tABCFFFFFFF123") << 16 << 0xABCFFFFFFF123LL << true;
    QTest::newRow("trailing spaces")
        << QByteArray("9876543210\t\r \n") << 10 << 9876543210LL << true;
    QTest::newRow("space after plus") << QByteArray("+ 12") << 10 << 0LL << false;
    QTest::newRow("space after minus") << QByteArray("- 12") << 10 << 0LL << false;
    QTest::newRow("leading junk") << QByteArray("q12345") << 10 << 0LL << false;
    QTest::newRow("trailing junk") << QByteArray("abc12345t") << 16 << 0LL << false;

    QTest::newRow("dec with base 0") << QByteArray("9876543210") << 0 << 9876543210LL << true;
    QTest::newRow("neg dec with base 0") << QByteArray("-9876543210") << 0 << -9876543210LL << true;
    QTest::newRow("hex with base 0") << QByteArray("0x9876543210") << 0 << 0x9876543210LL << true;
    QTest::newRow("neg hex with base 0")
        << QByteArray("-0x9876543210") << 0 << -0x9876543210LL << true;
    QTest::newRow("oct with base 0")
        << QByteArray("07654321234567") << 0 << 07654321234567LL << true;
    QTest::newRow("neg oct with base 0")
        << QByteArray("-07654321234567") << 0 << -07654321234567LL << true;

    QTest::newRow("base 3") << QByteArray("12012") << 3 << 140LL << true;
    QTest::newRow("neg base 3") << QByteArray("-201") << 3 << -19LL << true;

    // Boundary values, first in every base:
    using LL = std::numeric_limits<qlonglong>;
    for (int b = 0; b <= 36; ++b) {
        if (b == 1) // bases 0 and 2 through 36 are allowed
            ++b;
        QTest::addRow("max base %d", b)
            << QByteArray::number(LL::max(), b ? b : 10) << b << LL::max() << true;
        QTest::addRow("min base %d", b)
            << QByteArray::number(LL::min(), b ? b : 10) << b << LL::min() << true;
    }
    // Check leading zeros don't hit any buffer-too-big problems:
    QTest::newRow("many-0 max dec")
        << (QByteArray(512, '0') + QByteArray::number(LL::max())) << 10 << LL::max() << true;

    // Special bases (and let's include some leading space, too !), first decimal:
    QTest::newRow("max space dec")
        << ("\t\r\n\f\v " + QByteArray::number(LL::max())) << 10 << LL::max() << true;
    QTest::newRow("max space dec, base 0")
        << ("\t\r\n\f\v " + QByteArray::number(LL::max())) << 0 << LL::max() << true;
    QTest::newRow("min space dec")
        << ("\t\r\n\f\v " + QByteArray::number(LL::min())) << 10 << LL::min() << true;
    QTest::newRow("min space dec, base 0")
        << ("\t\r\n\f\v " + QByteArray::number(LL::min())) << 0 << LL::min() << true;

    // Hex with prefix:
    QTest::newRow("max 0x base 0")
        << ("0x" + QByteArray::number(LL::max(), 16)) << 0 << LL::max() << true;
    QTest::newRow("max +0x base 0")
        << ("+0x" + QByteArray::number(LL::max(), 16)) << 0 << LL::max() << true;
    QTest::newRow("max space 0x base 0")
        << ("\t\r\n\f\v 0x" + QByteArray::number(LL::max(), 16)) << 0 << LL::max() << true;
    QTest::newRow("max space +0x base 0")
        << ("\t\r\n\f\v +0x" + QByteArray::number(LL::max(), 16)) << 0 << LL::max() << true;
    QByteArray big = QByteArray::number(LL::min(), 16);
    big.insert(1, "0x"); // after sign
    QTest::newRow("min hex prefix") << big << 16 << LL::min() << true;
    QTest::newRow("min 0x base 0") << big << 0 << LL::min() << true;
    big.prepend("\t\r\n\f\v ");
    QTest::newRow("min space hex prefix") << big << 16 << LL::min() << true;
    QTest::newRow("min space 0x base 0") << big << 0 << LL::min() << true;

    // Octal with prefix:
    QTest::newRow("max octal base 0")
        << ('0' + QByteArray::number(LL::max(), 8)) << 0 << LL::max() << true;
    QTest::newRow("max +octal base 0")
        << ("+0" + QByteArray::number(LL::max(), 8)) << 0 << LL::max() << true;
    QTest::newRow("max space octal base 0")
        << ("\t\r\n\f\v 0" + QByteArray::number(LL::max(), 8)) << 0 << LL::max() << true;
    QTest::newRow("max space +octal base 0")
        << ("\t\r\n\f\v +0" + QByteArray::number(LL::max(), 8)) << 0 << LL::max() << true;
    big = QByteArray::number(LL::min(), 8);
    big.insert(1, '0'); // after sign
    QTest::newRow("min octal prefix") << big << 8 << LL::min() << true;
    QTest::newRow("min octal base 0") << big << 0 << LL::min() << true;
    big.prepend("\t\r\n\f\v ");
    QTest::newRow("min space octal prefix") << big << 8 << LL::min() << true;
    QTest::newRow("min space octal base 0") << big << 0 << LL::min() << true;

    // Values *just* out of range:
    QTest::newRow("max + 1 dec") << decNext(QByteArray::number(LL::max())) << 10 << 0LL << false;
    QTest::newRow("max + 1 dec base 0")
        << decNext(QByteArray::number(LL::max())) << 0 << 0LL << false;
    QTest::newRow("min - 1 dec") << decNext(QByteArray::number(LL::min())) << 10 << 0LL << false;
    QTest::newRow("min - 1 dec base 0")
        << decNext(QByteArray::number(LL::min())) << 0 << 0LL << false;
    // For hex and octal, we know the last digit of min is 0 and skipping its sign gets max+1:
    big = QByteArray::number(LL::min(), 8);
    QTest::newRow("max + 1 oct") << big.sliced(1) << 8 << 0LL << false;
    big[big.size() - 1] = '1';
    QTest::newRow("min - 1 oct") << big << 8 << 0LL << false;
    big.insert(1, '0'); // after minus sign
    QTest::newRow("min - 1 octal base 0") << big << 0 << 0LL << false;
    big = QByteArray::number(LL::min(), 16);
    QTest::newRow("max + 1 hex") << big.sliced(1) << 16 << 0LL << false;
    big[big.size() - 1] = '1';
    QTest::newRow("min - 1 hex") << big << 16 << 0LL << false;
    big.insert(1, "0x"); // after minus sign
    QTest::newRow("min - 1, 0x base 0") << big << 0 << 0LL << false;
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::toLongLong() const
{
    QFETCH(QByteArray, str);
    QFETCH(int, base);
    QFETCH(qlonglong, result);
    QFETCH(bool, ok);

    bool good;
    QCOMPARE(ByteArray(str).toLongLong(nullptr, base), result);
    QCOMPARE(ByteArray(str).toLongLong(&good, base), result);
    QCOMPARE(good, ok);
    if (base == 10) {
        QCOMPARE(ByteArray(str).toLongLong(&good), result);
        QCOMPARE(good, ok);
    }
}

void tst_QByteArrayApiSymmetry::toULongLong_data() const
{
    QTest::addColumn<QByteArray>("str");
    QTest::addColumn<int>("base");
    QTest::addColumn<qulonglong>("result");
    QTest::addColumn<bool>("ok");

    QTest::newRow("null") << QByteArray() << 10 << 0ULL << false;
    QTest::newRow("empty") << QByteArray("") << 10 << 0ULL << false;
    QTest::newRow("out of base bound") << QByteArray("c") << 10 << 0ULL << false;

    QTest::newRow("in range dec")
        << QByteArray("7679359922672374856") << 10 << 7679359922672374856ULL << true;
    QTest::newRow("in range hex")
        << QByteArray("6A929129A5421448") << 16 << 0x6A929129A5421448ULL << true;
    QTest::newRow("in range hex prefix")
        << QByteArray("0x6A929129A5421448") << 16 << 0x6A929129A5421448ULL << true;

    QTest::newRow("leading spaces") << QByteArray(" \n\r\t100") << 10 << 100ULL << true;
    QTest::newRow("trailing spaces") << QByteArray("100 \n\r\t") << 10 << 100ULL << true;
    QTest::newRow("leading plus") << QByteArray("+100") << 10 << 100ULL << true;
    QTest::newRow("space after plus") << QByteArray("+ 12") << 10 << 0ULL << false;
    QTest::newRow("leading minus") << QByteArray("-100") << 10 << 0ULL << false;
    QTest::newRow("leading junk") << QByteArray("x100") << 10 << 0ULL << false;
    QTest::newRow("trailing junk") << QByteArray("100x") << 10 << 0ULL << false;

    QTest::newRow("dec, base 0") << QByteArray("9876543210") << 0 << 9876543210ULL << true;
    QTest::newRow("hex, base 0") << QByteArray("0x9876543210") << 0 << 0x9876543210ULL << true;
    QTest::newRow("oct, base 0") << QByteArray("07654321234567") << 0 << 07654321234567ULL << true;
    QTest::newRow("base 3") << QByteArray("12012") << 3 << 140ULL << true;

    // Boundary values, first in every base:
    using ULL = std::numeric_limits<qulonglong>;
    for (int b = 0; b <= 36; ++b) {
        if (b == 1) // bases 0 and 2 through 36 are allowed
            ++b;
        QTest::addRow("max base %d", b)
            << QByteArray::number(ULL::max(), b ? b : 10) << b << ULL::max() << true;
    }
    // Check leading zeros don't hit any buffer-too-big problems:
    QTest::newRow("many-0 max dec")
        << (QByteArray(512, '0') + QByteArray::number(ULL::max())) << 10 << ULL::max() << true;

    // Special bases (and let's include some leading space, too !), first decimal:
    QTest::newRow("max space dec")
        << ("\t\r\n\f\v " + QByteArray::number(ULL::max())) << 10 << ULL::max() << true;
    QTest::newRow("max space dec, base 0")
        << ("\t\r\n\f\v " + QByteArray::number(ULL::max())) << 0 << ULL::max() << true;

    // Hex with prefix:
    QTest::newRow("max 0x base 0")
        << ("0x" + QByteArray::number(ULL::max(), 16)) << 0 << ULL::max() << true;
    QTest::newRow("max +0x base 0")
        << ("+0x" + QByteArray::number(ULL::max(), 16)) << 0 << ULL::max() << true;
    QTest::newRow("max space 0x base 0")
        << ("\t\r\n\f\v 0x" + QByteArray::number(ULL::max(), 16)) << 0 << ULL::max() << true;
    QTest::newRow("max space +0x base 0")
        << ("\t\r\n\f\v +0x" + QByteArray::number(ULL::max(), 16)) << 0 << ULL::max() << true;

    // Octal with prefix:
    QTest::newRow("max octal base 0")
        << ('0' + QByteArray::number(ULL::max(), 8)) << 0 << ULL::max() << true;
    QTest::newRow("max +octal base 0")
        << ("+0" + QByteArray::number(ULL::max(), 8)) << 0 << ULL::max() << true;
    QTest::newRow("max space octal base 0")
        << ("\t\r\n\f\v 0" + QByteArray::number(ULL::max(), 8)) << 0 << ULL::max() << true;
    QTest::newRow("max space +octal base 0")
        << ("\t\r\n\f\v +0" + QByteArray::number(ULL::max(), 8)) << 0 << ULL::max() << true;

    // Values *just* out of range:
    QTest::newRow("max + 1 dec") << decNext(QByteArray::number(ULL::max())) << 10 << 0ULL << false;
    QTest::newRow("max + 1 dec base 0")
        << decNext(QByteArray::number(ULL::max())) << 0 << 0ULL << false;
    auto big = QByteArray::number(ULL::max(), 8).replace('7', '0');
    // Number of bits is a power of two, so not a multiple of three; so (only)
    // first digit of max wasn't 7:
    big[0] += 1;
    QTest::newRow("max + 1 oct") << big << 8 << 0ULL << false;
    // Number of bits is a multiple of four, so every digit of max is 'f'.
    big = '1' + QByteArray::number(ULL::max(), 16).replace('f', '0');
    QTest::newRow("max + 1 hex") << big << 16 << 0ULL << false;
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::toULongLong() const
{
    QFETCH(QByteArray, str);
    QFETCH(int, base);
    QFETCH(qulonglong, result);
    QFETCH(bool, ok);

    bool good;
    QCOMPARE(ByteArray(str).toULongLong(0, base), result);
    QCOMPARE(ByteArray(str).toULongLong(&good, base), result);
    QCOMPARE(good, ok);
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::toFloat() const
{
    bool ok = true; // opposite to the next expected result

    QCOMPARE(ByteArray().toFloat(&ok), 0.0f);
    QVERIFY(!ok);

    ok = true;
    QCOMPARE(ByteArray("").toFloat(&ok), 0.0f);
    QVERIFY(!ok);

    ok = true;
    QCOMPARE(ByteArray(" ").toFloat(&ok), 0.0f);
    QVERIFY(!ok);

    ok = true;
    QCOMPARE(ByteArray("  ").toFloat(&ok), 0.0f);
    QVERIFY(!ok);

    // NB: floats < 1e-6 are zero as far as QCOMPARE() is concerned !
    const char data[] = "0.0000931322574615478515625";
    const float expectedValue = 9.31322574615478515625e-5f;
    QCOMPARE(ByteArray(data).toFloat(&ok), expectedValue);
    QVERIFY(ok);
    QCOMPARE(ByteArray(data, 6).toFloat(&ok), 0.0f);
    QVERIFY(ok);

    const char crufty[] = "3.14 and a bit";
    QCOMPARE(ByteArray(crufty).toFloat(&ok), 0.0f);
    QVERIFY(!ok);
    QCOMPARE(ByteArray(crufty, 4).toFloat(&ok), 3.14f);
    QVERIFY(ok);
}

void tst_QByteArrayApiSymmetry::toDouble_data() const
{
    QTest::addColumn<QByteArray>("string");
    QTest::addColumn<double>("expectedNumber");
    QTest::addColumn<bool>("expectedOk");

    QTest::newRow("null") << QByteArray() << 0.0 << false;
    QTest::newRow("empty") << QByteArray("") << 0.0 << false;
    QTest::newRow("space-only") << QByteArray(" ") << 0.0 << false;
    QTest::newRow("spaces-only") << QByteArray("  ") << 0.0 << false;

    QTest::newRow("decimal") << QByteArray("1.2345") << 1.2345 << true;
    QTest::newRow("exponent lowercase") << QByteArray("1.2345e+01") << 12.345 << true;
    QTest::newRow("exponent uppercase") << QByteArray("1.2345E+02") << 123.45 << true;
    QTest::newRow("leading spaces") << QByteArray(" \n\r\t1.2345") << 1.2345 << true;
    QTest::newRow("trailing spaces") << QByteArray("1.2345 \n\r\t") << 1.2345 << true;
    QTest::newRow("leading junk") << QByteArray("x1.2345") << 0.0 << false;
    QTest::newRow("trailing junk") << QByteArray("1.2345x") << 0.0 << false;
    QTest::newRow("high precision")
        << QByteArray("0.000000000931322574615478515625") << 9.31322574615478515625e-10 << true;
    QTest::newRow("exponential")
        << QByteArray("9.31322574615478515625e-10") << 9.31322574615478515625e-10 << true;

    QTest::newRow("raw, null plus junk")
        << QByteArray::fromRawData("1.2\0 junk", 9) << 0.0 << false;
    QTest::newRow("raw, null-terminator excluded")
        << QByteArray::fromRawData("2.3", 3) << 2.3 << true;
}

template <typename ByteArray> void tst_QByteArrayApiSymmetry::toDouble() const
{
    QFETCH(QByteArray, string);
    QFETCH(double, expectedNumber);
    QFETCH(bool, expectedOk);

    bool ok;
    const double number = ByteArray(string).toDouble(&ok);

    QCOMPARE(ok, expectedOk);
    QCOMPARE(number, expectedNumber);
}

const char globalChar = '1'; // Used as staic data for a raw byte array

QTEST_APPLESS_MAIN(tst_QByteArrayApiSymmetry)
#include "tst_qbytearrayapisymmetry.moc"
