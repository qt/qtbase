// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
    void boolIntegerConsistency();
    void unsignedIntegerConsistency_data();
    void unsignedIntegerConsistency();
    void signedIntegerConsistency_data();
    void signedIntegerConsistency();
    void floatingPointConsistency_data();
    void floatingPointConsistency();
    void stringConsistency_data();
    void stringConsistency();
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

void tst_QHashFunctions::boolIntegerConsistency()
{
    if (seed) QEXPECT_FAIL("", "QTBUG-126674", Continue);
    QCOMPARE(qHash(0, seed), qHash(false, seed));
    if (seed) QEXPECT_FAIL("", "QTBUG-126674", Continue);
    QCOMPARE(qHash(1, seed), qHash(true, seed));
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    // check consistency with pre-6.9 incidental implementation:
    QCOMPARE(qHash(true,  seed), qHash(int(true)) ^ seed);
    QCOMPARE(qHash(false, seed), qHash(int(false)) ^ seed);
#endif
}

template <typename T> static void addPositiveCommonRows()
{
    QTest::addRow("zero") << T(0);
    QTest::addRow("positive_7bit") << T(42);
    QTest::addRow("positive_15bit") << T(0x1f3f);
    QTest::addRow("positive_31bit") << T(0x4b3d'93c4);
    QTest::addRow("positive_63bit") << T(Q_INT64_C(0x39df'7338'4b14'fcb0));

    QTest::addRow("SCHAR_MAX") << T(SCHAR_MAX);
    QTest::addRow("SHRT_MAX") << T(SHRT_MAX);
    QTest::addRow("INT_MAX") << T(INT_MAX);
    QTest::addRow("LLONG_MAX") << T(LLONG_MAX);
}

void tst_QHashFunctions::signedIntegerConsistency_data()
{
    QTest::addColumn<qint64>("value");
    addPositiveCommonRows<qint64>();
    QTest::addRow("negative_7bit") << Q_INT64_C(-28);
    QTest::addRow("negative_15bit") << Q_INT64_C(-0x387c);
    QTest::addRow("negative_31bit") << qint64(-0x7713'30f9);

    QTest::addRow("SCHAR_MIN") << qint64(SCHAR_MIN);
    QTest::addRow("SHRT_MIN") << qint64(SHRT_MIN);
    QTest::addRow("INT_MIN") << qint64(INT_MIN);
    QTest::addRow("LLONG_MIN") << LLONG_MIN;
}

void tst_QHashFunctions::unsignedIntegerConsistency_data()
{
    QTest::addColumn<quint64>("value");
    addPositiveCommonRows<quint64>();

    QTest::addRow("positive_8bit") << Q_UINT64_C(0xE4);
    QTest::addRow("positive_16bit") << Q_UINT64_C(0xcafe);
    QTest::addRow("positive_32bit") << quint64(0xcafe'babe);

    QTest::addRow("UCHAR_MAX") << quint64(UCHAR_MAX);
    QTest::addRow("UHRT_MAX") << quint64(USHRT_MAX);
    QTest::addRow("UINT_MAX") << quint64(UINT_MAX);
    QTest::addRow("ULLONG_MAX") << ULLONG_MAX;
}

static void unsignedIntegerConsistency(quint64 value, size_t seed)
{
    quint8 v8 = quint8(value);
    quint16 v16 = quint16(value);
    quint32 v32 = quint32(value);

    const auto hu8  = qHash(v8, seed);
    const auto hu16 = qHash(v16, seed);
    const auto hu32 = qHash(v32, seed);
    const auto hu64 = qHash(value, seed);

    if (v8 == value)
        QCOMPARE(hu8, hu32);
    if (v16 == value)
        QCOMPARE(hu16, hu32);
    if (v32 == value)
        QCOMPARE(hu64, hu32);

    // there are a few more unsigned types:
#ifdef __cpp_char8_t
     const auto hc8 = qHash(char8_t(value), seed);
#endif
     const auto hc16  = qHash(char16_t(value), seed);
     const auto hc32  = qHash(char32_t(value), seed);
#ifdef __cpp_char8_t
     QCOMPARE(hc8, hu8);
#endif
     QCOMPARE(hc16, hu16);
     QCOMPARE(hc32, hu32);
}

void tst_QHashFunctions::unsignedIntegerConsistency()
{
    QFETCH(quint64, value);
    ::unsignedIntegerConsistency(value, seed);
}

void tst_QHashFunctions::signedIntegerConsistency()
{
    QFETCH(qint64, value);
    qint8 v8 = qint8(value);
    qint16 v16 = qint16(value);
    qint32 v32 = qint32(value);

    const auto hs8  = qHash(v8, seed);
    const auto hs16 = qHash(v16, seed);
    const auto hs32 = qHash(v32, seed);
    const auto hs64 = qHash(value, seed);

    if (v8 == value)
        QCOMPARE(hs8, hs32);
    if (v16 == value)
        QCOMPARE(hs16, hs32);
    if (v32 == value) {
        // because of QTBUG-116080, this may not match, but we can't guarantee
        // it mismatches 100% of the time either
        if constexpr (sizeof(size_t) > sizeof(int))
            QCOMPARE(hs64, hs32);
    }

    if (value > 0) {
        quint64 u64 = quint64(value);
        const auto hu64 = qHash(u64, seed);
        QCOMPARE(hu64, hs64);
        ::unsignedIntegerConsistency(u64, seed);
        // by A == B && B == C -> A == C, we've shown hsXX == huXX for all XX
    }
}

void tst_QHashFunctions::floatingPointConsistency_data()
{
    QTest::addColumn<double>("value");
    QTest::addRow("zero") << 0.0;

    QTest::addRow("1.0") << 1.0;
    QTest::addRow("infinity") << std::numeric_limits<double>::infinity();

    QTest::addRow("fp16_epsilon") << double(std::numeric_limits<qfloat16>::epsilon());
    QTest::addRow("fp16_min") << double(std::numeric_limits<qfloat16>::min());
    QTest::addRow("fp16_max") << double(std::numeric_limits<qfloat16>::max());

    QTest::addRow("float_epsilon") << double(std::numeric_limits<float>::epsilon());
    QTest::addRow("float_min") << double(std::numeric_limits<float>::min());
    QTest::addRow("float_max") << double(std::numeric_limits<float>::max());

    QTest::addRow("double_epsilon") << double(std::numeric_limits<double>::epsilon());
    QTest::addRow("double_min") << double(std::numeric_limits<double>::min());
    QTest::addRow("double_max") << double(std::numeric_limits<double>::max());
}

void tst_QHashFunctions::floatingPointConsistency()
{
    QFETCH(double, value);
    long double lvalue = value;
    float fp32 = float(value);
    qfloat16 fp16 = qfloat16(value);

    const auto hfld = qHash(lvalue, seed);
    const auto hf64 = qHash(value, seed);
    const auto hf32 = qHash(fp32, seed);
    const auto hf16 = qHash(fp16, seed);

    const auto hnfld = qHash(-lvalue, seed);
    const auto hnf64 = qHash(-value, seed);
    const auto hnf32 = qHash(-fp32, seed);
    const auto hnf16 = qHash(-fp16, seed);

    if (fp16 == fp32) {
        QCOMPARE(hf16, hf32);
        QCOMPARE(hnf16, hnf32);
    }

    // See QTBUG-116077; the rest isn't guaranteed to match (but we can't
    // guarantee it will mismatch either).
    return;

    if (fp32 == value) {
        QCOMPARE(hf32, hf64);
        QCOMPARE(hnf32, hnf64);
    }

    QCOMPARE(hfld, hf64);
    QCOMPARE(hnfld, hnf64);
}

void tst_QHashFunctions::stringConsistency_data()
{
    QTest::addColumn<QString>("value");
    QTest::newRow("null") << QString();
    QTest::newRow("empty") << "";
    QTest::newRow("withnull") << QStringLiteral("A\0z");
    QTest::newRow("short-ascii") << "Hello";
    QTest::newRow("long-ascii") << QStringLiteral("abcdefghijklmnopqrstuvxyz").repeated(16);

    QTest::newRow("short-latin1") << "Bokmål";
    QTest::newRow("long-latin1")
            << R"(Alle mennesker er født frie og med samme menneskeverd og menneskerettigheter.
 De er utstyrt med fornuft og samvittighet og bør handle mot hverandre i brorskapets ånd.)";

    QTest::newRow("short-nonlatin1") << "Ελληνικά";
    QTest::newRow("long-nonlatin1")
            << R"('Ολοι οι άνθρωποι γεννιούνται ελεύθεροι και ίσοι στην αξιοπρέπεια και τα
 δικαιώματα. Είναι προικισμένοι με λογική και συνείδηση, και οφείλουν να συμπεριφέρονται μεταξύ
 τους με πνεύμα αδελφοσύνης.)";
}

void tst_QHashFunctions::stringConsistency()
{
    QFETCH(QString, value);
    QStringView sv = value;
    QByteArray u8ba = value.toUtf8();
    QByteArray u8bav = u8ba;

    // sanity checking:
    QCOMPARE(sv.isNull(), value.isNull());
    QCOMPARE(sv.isEmpty(), value.isEmpty());
    QCOMPARE(u8ba.isNull(), value.isNull());
    QCOMPARE(u8ba.isEmpty(), value.isEmpty());
    QCOMPARE(u8bav.isNull(), value.isNull());
    QCOMPARE(u8bav.isEmpty(), value.isEmpty());

    QCOMPARE(qHash(sv, seed), qHash(value, seed));
    QCOMPARE(qHash(u8bav, seed), qHash(u8ba, seed));
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
    QCOMPARE(qHash(-0.0L, seed), qHash(0.0L, seed));
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
