// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QVarLengthArray>

#include <qhash.h>
#include <qfloat16.h>

#include <iterator>
#include <sstream>
#include <algorithm>

#include <unordered_set>

class tst_QHashFunctions : public QObject
{
    Q_OBJECT
public:
    // random values
    static constexpr quint64 ZeroSeed = 0;
    static constexpr quint64 RandomSeed32 = 1045982819;
    static constexpr quint64 RandomSeed64 = QtPrivate::QHashCombine{}(RandomSeed32, RandomSeed32);
    size_t seed;

    template <typename T1, typename T2> void stdPair_template(const T1 &t1, const T2 &t2);

public slots:
    void initTestCase();
    void init();

private Q_SLOTS:
    void consistent();
    void qhash();
    void qhash_of_empty_and_null_qstring();
    void qhash_of_empty_and_null_qbytearray();
    void qhash_of_zero_floating_points();
    void qthash_data();
    void qthash();
    void range();
    void rangeCommutative();

    void stdHash();

    void stdPair_int_int()          { stdPair_template(1, 2); }
    void stdPair_ulong_llong()      { stdPair_template(1UL, -2LL); }
    void stdPair_ullong_long()      { stdPair_template(1ULL, -2L); }
    void stdPair_string_int()       { stdPair_template(QString("Hello"), 2); }
    void stdPair_int_string()       { stdPair_template(1, QString("Hello")); }
    void stdPair_bytearray_string() { stdPair_template(QByteArray("Hello"), QString("World")); }
    void stdPair_string_bytearray() { stdPair_template(QString("Hello"), QByteArray("World")); }
    void stdPair_int_pairIntInt()   { stdPair_template(1, std::make_pair(2, 3)); }
    void stdPair_2x_pairIntInt()    { stdPair_template(std::make_pair(1, 2), std::make_pair(2, 3)); }
    void stdPair_string_pairIntInt()    { stdPair_template(QString("Hello"), std::make_pair(42, -47)); } // QTBUG-92910
    void stdPair_int_pairIntPairIntInt() { stdPair_template(1, std::make_pair(2, std::make_pair(3, 4))); }

    void enum_int_consistent_hash_qtbug108032();

#if QT_DEPRECATED_SINCE(6, 6)
    void setGlobalQHashSeed();
#endif
};

void tst_QHashFunctions::consistent()
{
    // QString-like
    const QString s = QStringLiteral("abcdefghijklmnopqrstuvxyz").repeated(16);
    QCOMPARE(qHash(s, seed), qHash(QStringView(s), seed));

    // unsigned integers
    {
        constexpr unsigned char ae = 0xE4; // LATIN SMALL LETTER A WITH DIAERESIS
        const auto h8   = qHash(quint8(ae), seed);
        const auto h16  = qHash(quint16(ae), seed);
        const auto h32  = qHash(quint32(ae), seed);
        const auto h64  = qHash(quint64(ae), seed);
        QCOMPARE(h8, h16);
        QCOMPARE(h16, h32);
        QCOMPARE(h32, h64);
       // there are a few more unsigned types:
#ifdef __cpp_char8_t
        const auto hc8 = qHash(char8_t(ae), seed);
#endif
        const auto hc16  = qHash(char16_t(ae), seed);
        const auto hc32  = qHash(char32_t(ae), seed);
#ifdef __cpp_char8_t
        QCOMPARE(hc8, h8);
#endif
        QCOMPARE(hc16, h16);
        QCOMPARE(hc32, h32);
    }

    // signed integers
    {
        constexpr signed char ae = 0xE4; // LATIN SMALL LETTER A WITH DIAERESIS
        const auto h8   = qHash(qint8(ae), seed);
        const auto h16  = qHash(qint16(ae), seed);
        const auto h32  = qHash(qint32(ae), seed);
        const auto h64  = qHash(qint64(ae), seed);
        QCOMPARE(h8, h16);
        QCOMPARE(h16, h32);
        if constexpr (sizeof(size_t) == sizeof(int)) // 32-bit
            QEXPECT_FAIL("", "QTBUG-116080", Continue);
        QCOMPARE(h32, h64);
    }

    // floats
    {
        const/*expr broken: QTBUG-116079*/ qfloat16 f16 = qfloat16(-42.f);
        const auto h16 = qHash(f16, seed);
        const auto h32 = qHash(float(f16), seed);
        const auto h64 = qHash(double(f16), seed);
        QCOMPARE(h16, h32);
        QEXPECT_FAIL("", "QTBUG-116077", Continue);
        QCOMPARE(h32, h64);
    }
}

void tst_QHashFunctions::initTestCase()
{
    QTest::addColumn<quint64>("seedValue");

    QTest::newRow("zero-seed") << ZeroSeed;
    QTest::newRow("zero-seed-negated") << ~ZeroSeed;
    QTest::newRow("non-zero-seed-32bit") << RandomSeed32;
    QTest::newRow("non-zero-seed-32bit-negated")
            << quint64{~quint32(RandomSeed32)}; // ensure this->seed gets same value on 32/64-bit
    if constexpr (sizeof(size_t) == sizeof(quint64)) {
        QTest::newRow("non-zero-seed-64bit") << RandomSeed64;
        QTest::newRow("non-zero-seed-64bit-negated") << ~RandomSeed64;
    }
}

void tst_QHashFunctions::init()
{
    QFETCH_GLOBAL(quint64, seedValue);
    seed = size_t(seedValue);
}

void tst_QHashFunctions::qhash()
{
    {
        QBitArray a1;
        QBitArray a2;

        a1.resize(1);
        a1.setBit(0, true);

        a2.resize(1);
        a2.setBit(0, false);

        size_t h1 = qHash(a1, seed);
        size_t h2 = qHash(a2, seed);

        QVERIFY(h1 != h2);  // not guaranteed

        a2.setBit(0, true);
        QVERIFY(h1 == qHash(a2, seed));

        a1.fill(true, 8);
        a1.resize(7);

        h1 = qHash(a1, seed);

        a2.fill(true, 7);
        h2 = qHash(a2, seed);

        QVERIFY(h1 == h2);

        a2.setBit(0, false);
        size_t h3 = qHash(a2, seed);
        QVERIFY(h2 != h3);  // not guaranteed

        a2.setBit(0, true);
        QVERIFY(h2 == qHash(a2, seed));

        a2.setBit(6, false);
        size_t h4 = qHash(a2, seed);
        QVERIFY(h2 != h4);  // not guaranteed

        a2.setBit(6, true);
        QVERIFY(h2 == qHash(a2, seed));

        QVERIFY(h3 != h4);  // not guaranteed
    }

    {
        QPair<int, int> p12(1, 2);
        QPair<int, int> p21(2, 1);

        QVERIFY(qHash(p12, seed) == qHash(p12, seed));
        QVERIFY(qHash(p21, seed) == qHash(p21, seed));
        QVERIFY(qHash(p12, seed) != qHash(p21, seed));  // not guaranteed

        QPair<int, int> pA(0x12345678, 0x12345678);
        QPair<int, int> pB(0x12345675, 0x12345675);

        QVERIFY(qHash(pA, seed) != qHash(pB, seed));    // not guaranteed
    }

    {
        std::pair<int, int> p12(1, 2);
        std::pair<int, int> p21(2, 1);

        using QT_PREPEND_NAMESPACE(qHash);

        QVERIFY(qHash(p12, seed) == qHash(p12, seed));
        QVERIFY(qHash(p21, seed) == qHash(p21, seed));
        QVERIFY(qHash(p12, seed) != qHash(p21, seed));  // not guaranteed

        std::pair<int, int> pA(0x12345678, 0x12345678);
        std::pair<int, int> pB(0x12345675, 0x12345675);

        QVERIFY(qHash(pA, seed) != qHash(pB, seed));    // not guaranteed
    }
}

void tst_QHashFunctions::qhash_of_empty_and_null_qstring()
{
    QString null, empty("");
    QCOMPARE(null, empty);
    QCOMPARE(qHash(null, seed), qHash(empty, seed));

    QStringView nullView, emptyView(empty);
    QCOMPARE(nullView, emptyView);
    QCOMPARE(qHash(nullView, seed), qHash(emptyView, seed));
}

void tst_QHashFunctions::qhash_of_empty_and_null_qbytearray()
{
    QByteArray null, empty("");
    QCOMPARE(null, empty);
    QCOMPARE(qHash(null, seed), qHash(empty, seed));
}

void tst_QHashFunctions::qhash_of_zero_floating_points()
{
    QCOMPARE(qHash(-0.0f, seed), qHash(0.0f, seed));
    QCOMPARE(qHash(-0.0 , seed), qHash(0.0 , seed));
#ifndef Q_OS_DARWIN
    QCOMPARE(qHash(-0.0L, seed), qHash(0.0L, seed));
#endif
}

void tst_QHashFunctions::qthash_data()
{
    QTest::addColumn<QString>("key");
    QTest::addColumn<uint>("hash");

    QTest::newRow("null") << QString() << 0u;
    QTest::newRow("empty") << QStringLiteral("") << 0u;
    QTest::newRow("abcdef") << QStringLiteral("abcdef") << 108567222u;
    QTest::newRow("tqbfjotld") << QStringLiteral("The quick brown fox jumps over the lazy dog") << 140865879u;
    QTest::newRow("42") << QStringLiteral("42") << 882u;
}

void tst_QHashFunctions::qthash()
{
    QFETCH(QString, key);
    const uint result = qt_hash(key);
    QTEST(result, "hash");
}

namespace SomeNamespace {
    struct Hashable { int i; };
    inline size_t qHash(Hashable h, size_t seed = 0)
    { return QT_PREPEND_NAMESPACE(qHash)(h.i, seed); }

    struct AdlHashable {
        int i;
    private:
        friend size_t qHash(AdlHashable h, size_t seed = 0)
        { return QT_PREPEND_NAMESPACE(qHash)(h.i, seed); }
    };
}
void tst_QHashFunctions::range()
{
    static const int ints[] = {0, 1, 2, 3, 4, 5};
    static const size_t numInts = sizeof ints / sizeof *ints;

    // empty range just gives the seed:
    QCOMPARE(qHashRange(ints, ints, seed), seed);
    // verify that order matters (test not guaranteed):
    QVERIFY(qHashRange(ints, ints + numInts, seed) !=
            qHashRange(std::reverse_iterator<const int*>(ints + numInts), std::reverse_iterator<const int*>(ints), seed));

    {
        // verify that the input iterator category suffices:
        std::stringstream sstream;
        static_assert((std::is_same<std::input_iterator_tag, std::istream_iterator<int>::iterator_category>::value));
        std::copy(ints, ints + numInts, std::ostream_iterator<int>(sstream, " "));
        sstream.seekg(0);
        std::istream_iterator<int> it(sstream), end;
        QCOMPARE(qHashRange(ints, ints + numInts, seed), qHashRange(it, end, seed));
    }

    {
        SomeNamespace::Hashable hashables[] = {{0}, {1}, {2}, {3}, {4}, {5}};
        // compile check: is qHash() found using ADL?
        [[maybe_unused]] auto r = qHashRange(std::begin(hashables), std::end(hashables), seed);
    }
    {
        SomeNamespace::AdlHashable hashables[] = {{0}, {1}, {2}, {3}, {4}, {5}};
        // compile check: is qHash() found as a hidden friend?
        [[maybe_unused]] auto r = qHashRange(std::begin(hashables), std::end(hashables), seed);
    }
}

void tst_QHashFunctions::rangeCommutative()
{
    int ints[] = {0, 1, 2, 3, 4, 5};
    static const size_t numInts = sizeof ints / sizeof *ints;

    // empty range just gives the seed:
    QCOMPARE(qHashRangeCommutative(ints, ints, seed), seed);
    // verify that order doesn't matter (test not guaranteed):
    QCOMPARE(qHashRangeCommutative(ints, ints + numInts, seed),
             qHashRangeCommutative(std::reverse_iterator<int*>(ints + numInts), std::reverse_iterator<int*>(ints), seed));

    {
        // verify that the input iterator category suffices:
        std::stringstream sstream;
        std::copy(ints, ints + numInts, std::ostream_iterator<int>(sstream, " "));
        sstream.seekg(0);
        std::istream_iterator<int> it(sstream), end;
        QCOMPARE(qHashRangeCommutative(ints, ints + numInts, seed), qHashRangeCommutative(it, end, seed));
    }

    {
        SomeNamespace::Hashable hashables[] = {{0}, {1}, {2}, {3}, {4}, {5}};
        // compile check: is qHash() found using ADL?
        [[maybe_unused]] auto r = qHashRangeCommutative(std::begin(hashables), std::end(hashables), seed);
    }
    {
        SomeNamespace::AdlHashable hashables[] = {{0}, {1}, {2}, {3}, {4}, {5}};
        // compile check: is qHash() found as a hidden friend?
        [[maybe_unused]] auto r = qHashRangeCommutative(std::begin(hashables), std::end(hashables), seed);
    }
}

// QVarLengthArray these days has a qHash() as a hidden friend.
// This checks that QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH can deal with that:

QT_BEGIN_NAMESPACE
QT_SPECIALIZE_STD_HASH_TO_CALL_QHASH_BY_CREF(QVarLengthArray<QVector<int>>)
QT_END_NAMESPACE

void tst_QHashFunctions::stdHash()
{
    {
        std::unordered_set<QVarLengthArray<QVector<int>>> s = {
            {
                {0, 1, 2},
                {42, 43, 44},
                {},
            }, {
                {11, 12, 13},
                {},
            },
        };
        QCOMPARE(s.size(), 2UL);
        s.insert({
                     {11, 12, 13},
                     {},
                 });
        QCOMPARE(s.size(), 2UL);
    }

    {
        std::unordered_set<QString> s = {QStringLiteral("Hello"), QStringLiteral("World")};
        QCOMPARE(s.size(), 2UL);
        s.insert(QStringLiteral("Hello"));
        QCOMPARE(s.size(), 2UL);
    }

    {
        std::unordered_set<QStringView> s = {QStringLiteral("Hello"), QStringLiteral("World")};
        QCOMPARE(s.size(), 2UL);
        s.insert(QStringLiteral("Hello"));
        QCOMPARE(s.size(), 2UL);
    }

    {
        std::unordered_set<QLatin1String> s = {QLatin1String("Hello"), QLatin1String("World")};
        QCOMPARE(s.size(), 2UL);
        s.insert(QLatin1String("Hello"));
        QCOMPARE(s.size(), 2UL);
    }

    {
        std::unordered_set<QByteArray> s = {QByteArrayLiteral("Hello"), QByteArrayLiteral("World")};
        QCOMPARE(s.size(), 2UL);
        s.insert(QByteArray("Hello"));
        QCOMPARE(s.size(), 2UL);
    }

    {
        std::unordered_set<QChar> s = {u'H', u'W'};
        QCOMPARE(s.size(), 2UL);
        s.insert(u'H');
        QCOMPARE(s.size(), 2UL);
    }

}

template <typename T1, typename T2>
void tst_QHashFunctions::stdPair_template(const T1 &t1, const T2 &t2)
{
    std::pair<T1, T2> dpair{};
    std::pair<T1, T2> vpair{t1, t2};

    // confirm proper working of the pair and of the underlying types
    QVERIFY(t1 == t1);
    QVERIFY(t2 == t2);
    QCOMPARE(qHash(t1, seed), qHash(t1, seed));
    QCOMPARE(qHash(t2, seed), qHash(t2, seed));

    QVERIFY(dpair == dpair);
    QVERIFY(vpair == vpair);

    // therefore their hashes should be equal
    QCOMPARE(qHash(dpair, seed), qHash(dpair, seed));
    QCOMPARE(qHash(vpair, seed), qHash(vpair, seed));
}

void tst_QHashFunctions::enum_int_consistent_hash_qtbug108032()
{
    enum E { E1, E2, E3 };

    static_assert(QHashPrivate::HasQHashSingleArgOverload<E>);

    QCOMPARE(qHash(E1, seed), qHash(int(E1), seed));
    QCOMPARE(qHash(E2, seed), qHash(int(E2), seed));
    QCOMPARE(qHash(E3, seed), qHash(int(E3), seed));
}

#if QT_DEPRECATED_SINCE(6, 6)
void tst_QHashFunctions::setGlobalQHashSeed()
{
QT_WARNING_PUSH QT_WARNING_DISABLE_DEPRECATED
    // Setter works as advertised
    qSetGlobalQHashSeed(0);
    QCOMPARE(qGlobalQHashSeed(), 0);

    // Creating a new QHash doesn't reset the seed
    QHash<QString, int> someHash;
    someHash.insert("foo", 42);
    QCOMPARE(qGlobalQHashSeed(), 0);

    // Reset works as advertised
    qSetGlobalQHashSeed(-1);
    QVERIFY(qGlobalQHashSeed() > 0);
QT_WARNING_POP
}
#endif // QT_DEPRECATED_SINCE(6, 6)

QTEST_APPLESS_MAIN(tst_QHashFunctions)
#include "tst_qhashfunctions.moc"
