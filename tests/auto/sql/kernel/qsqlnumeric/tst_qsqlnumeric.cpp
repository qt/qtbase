// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtSql/private/qsqlnumeric_p.h>

#include <limits>

using namespace Qt::StringLiterals;
using namespace QtPrivate;

/*
    Unit tests for the scaled-integer numeric helpers in qsqlnumeric_p.h,
    used by the QIBASE and QFIREBIRD drivers. No database is needed: every
    function is pure. The 128-bit cases run only where QSQL_HAS_INT128 is
    defined (a compiler with __int128, or MSVC's std::_Signed128).
*/
class tst_QSqlNumeric : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void scaledToString_data();
    void scaledToString();
    void decimalToScaled_data();
    void decimalToScaled();
    void isPlainDecimal_data();
    void isPlainDecimal();
    void numericInputString_data();
    void numericInputString();
    void applyScale();
    void scaledFromVariant();

    void scaledToString128();
    void decimalToScaled128();
    void applyScale128();
    void scaledFromVariant128();
    void int128Words();
};

#ifdef QSQL_HAS_INT128
static qinternalint128 int128FromString(const QByteArray &decimal)
{
    qinternalint128 v = 0;
    const bool ok = sqlDecimalToScaled(decimal, 0, &v);
    Q_ASSERT(ok);
    return v;
}

static const char int128MinStr[] = "-170141183460469231731687303715884105728";
static const char int128MaxStr[] = "170141183460469231731687303715884105727";
#endif

void tst_QSqlNumeric::initTestCase()
{
#ifdef QSQL_HAS_INT128
    qDebug("128-bit coverage: enabled");
#else
    qDebug("128-bit coverage: not available on this toolchain - 128-bit cases are compiled out");
#endif
}

void tst_QSqlNumeric::scaledToString_data()
{
    QTest::addColumn<qint64>("value");
    QTest::addColumn<int>("scale");
    QTest::addColumn<QString>("expected");

    QTest::newRow("zero") << Q_INT64_C(0) << 0 << u"0"_s;
    QTest::newRow("plain") << Q_INT64_C(12345) << 0 << u"12345"_s;
    QTest::newRow("negative plain") << Q_INT64_C(-42) << 0 << u"-42"_s;
    QTest::newRow("positive scale appends zeros") << Q_INT64_C(12) << 2 << u"1200"_s;
    QTest::newRow("zero at positive scale") << Q_INT64_C(0) << 2 << u"0"_s;
    QTest::newRow("zero keeps its decimals") << Q_INT64_C(0) << -3 << u"0.000"_s;
    QTest::newRow("two decimals") << Q_INT64_C(12345) << -2 << u"123.45"_s;
    QTest::newRow("needs leading zeros") << Q_INT64_C(5) << -3 << u"0.005"_s;
    QTest::newRow("negative two decimals") << Q_INT64_C(-12345) << -2 << u"-123.45"_s;
    QTest::newRow("small negative") << Q_INT64_C(-4) << -3 << u"-0.004"_s;
    QTest::newRow("int64 max") << (std::numeric_limits<qint64>::max)() << 0
                               << u"9223372036854775807"_s;
    QTest::newRow("int64 min") << (std::numeric_limits<qint64>::min)() << 0
                               << u"-9223372036854775808"_s;
    QTest::newRow("int64 min scaled") << (std::numeric_limits<qint64>::min)() << -2
                                      << u"-92233720368547758.08"_s;
}

void tst_QSqlNumeric::scaledToString()
{
    QFETCH(qint64, value);
    QFETCH(int, scale);
    QFETCH(QString, expected);

    QCOMPARE(sqlScaledToString(value, scale), expected);
}

void tst_QSqlNumeric::decimalToScaled_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<int>("scale");
    QTest::addColumn<bool>("ok");
    QTest::addColumn<qint64>("expected");

    // Grammar
    QTest::newRow("empty") << QByteArray() << 0 << false << Q_INT64_C(0);
    QTest::newRow("lone dot") << QByteArray(".") << 0 << false << Q_INT64_C(0);
    QTest::newRow("lone sign") << QByteArray("+") << 0 << false << Q_INT64_C(0);
    QTest::newRow("double dot") << QByteArray("1.2.3") << 0 << false << Q_INT64_C(0);
    QTest::newRow("exponent") << QByteArray("1e5") << 0 << false << Q_INT64_C(0);
    QTest::newRow("junk") << QByteArray("abc") << 0 << false << Q_INT64_C(0);
    QTest::newRow("inner space") << QByteArray("1 2") << 0 << false << Q_INT64_C(0);
    QTest::newRow("whitespace trimmed") << QByteArray("  42  ") << 0 << true << Q_INT64_C(42);
    QTest::newRow("plus sign") << QByteArray("+42") << 0 << true << Q_INT64_C(42);

    // Point shifting and rounding (half away from zero)
    QTest::newRow("shift right") << QByteArray("123.45") << -2 << true << Q_INT64_C(12345);
    QTest::newRow("pad fraction") << QByteArray("1.5") << -3 << true << Q_INT64_C(1500);
    QTest::newRow("round up") << QByteArray("1.005") << -2 << true << Q_INT64_C(101);
    QTest::newRow("round down") << QByteArray("2.4949") << -2 << true << Q_INT64_C(249);
    QTest::newRow("half negative") << QByteArray("-0.5") << 0 << true << Q_INT64_C(-1);
    QTest::newRow("below half negative") << QByteArray("-0.4") << 0 << true << Q_INT64_C(0);
    QTest::newRow("positive scale") << QByteArray("1200") << 2 << true << Q_INT64_C(12);

    // Exact overflow boundaries
    QTest::newRow("max") << QByteArray("9223372036854775807") << 0 << true
                         << (std::numeric_limits<qint64>::max)();
    QTest::newRow("max + 1") << QByteArray("9223372036854775808") << 0 << false << Q_INT64_C(0);
    QTest::newRow("min") << QByteArray("-9223372036854775808") << 0 << true
                         << (std::numeric_limits<qint64>::min)();
    QTest::newRow("min - 1") << QByteArray("-9223372036854775809") << 0 << false << Q_INT64_C(0);
    QTest::newRow("max via scale") << QByteArray("92233720368547758.07") << -2 << true
                                   << (std::numeric_limits<qint64>::max)();
    QTest::newRow("max via rounding") << QByteArray("92233720368547758.075") << -2 << false
                                      << Q_INT64_C(0);
}

void tst_QSqlNumeric::decimalToScaled()
{
    QFETCH(QByteArray, input);
    QFETCH(int, scale);
    QFETCH(bool, ok);
    QFETCH(qint64, expected);

    qint64 out = 0;
    QCOMPARE(sqlDecimalToScaled(input, scale, &out), ok);
    if (ok)
        QCOMPARE(out, expected);
}

void tst_QSqlNumeric::isPlainDecimal_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<bool>("plain");

    QTest::newRow("integer") << QByteArray("42") << true;
    QTest::newRow("signed fraction") << QByteArray("-12.34") << true;
    QTest::newRow("plus and spaces") << QByteArray(" +7.0 ") << true;
    QTest::newRow("exponent") << QByteArray("1e5") << false;
    QTest::newRow("lone dot") << QByteArray(".") << false;
    QTest::newRow("empty") << QByteArray() << false;
    QTest::newRow("junk") << QByteArray("true") << false;
}

void tst_QSqlNumeric::isPlainDecimal()
{
    QFETCH(QByteArray, input);
    QFETCH(bool, plain);

    QCOMPARE(sqlIsPlainDecimal(input), plain);
}

void tst_QSqlNumeric::numericInputString_data()
{
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("int") << QVariant(42) << QByteArray("42");
    // A Bool is its value, not "true"/"false": the text path (Firebird's string
    // interface, or a scale beyond the integral fast path) binds it as 1/0
    QTest::newRow("bool true") << QVariant(true) << QByteArray("1");
    QTest::newRow("bool false") << QVariant(false) << QByteArray("0");
    QTest::newRow("string") << QVariant(u"12.34"_s) << QByteArray("12.34");
    QTest::newRow("bytearray") << QVariant(QByteArray("7.5")) << QByteArray("7.5");
    QTest::newRow("plain double") << QVariant(1.5) << QByteArray("1.5");
    QTest::newRow("large double loses exponent") << QVariant(1e20)
                                                 << QByteArray("100000000000000000000");
    QTest::newRow("small double loses exponent") << QVariant(-2.5e-5)
                                                 << QByteArray("-0.000025");
    // The exponent form is expanded digit for digit, however far the point
    // moves (a fixed 18-decimal reformat used to turn 1.5e-19 into "0")
    QTest::newRow("tiny double keeps every digit") << QVariant(1.5e-19)
                                                   << QByteArray("0.00000000000000000015");
    QTest::newRow("wide double keeps every digit") << QVariant(1.23456789e38)
                                                   << QByteArray("123456789" + QByteArray(30, '0'));
}

void tst_QSqlNumeric::numericInputString()
{
    QFETCH(QVariant, value);
    QFETCH(QByteArray, expected);

    QCOMPARE(sqlNumericInputString(value), expected);
}

void tst_QSqlNumeric::applyScale()
{
    // scale == 0: the value comes back as is (usual integral promotion)
    QCOMPARE(sqlApplyScale<qint16>(7, 0, QSql::HighPrecision).metaType().id(), QMetaType::Int);
    QCOMPARE(sqlApplyScale<qint32>(7, 0, QSql::LowPrecisionDouble).metaType().id(), QMetaType::Int);
    QCOMPARE(sqlApplyScale<qint64>(7, 0, QSql::LowPrecisionInt64).metaType().id(),
             QMetaType::LongLong);
    QCOMPARE(sqlApplyScale<qint64>(7, 0, QSql::HighPrecision).toLongLong(), Q_INT64_C(7));

    // The four policies on an INT64-backed NUMERIC(x,2)
    const qint64 stored = Q_INT64_C(123456789012345678); // 1234567890123456.78, > 2^53
    QVariant v = sqlApplyScale(stored, -2, QSql::HighPrecision);
    QCOMPARE(v.metaType().id(), QMetaType::QString);
    QCOMPARE(v.toString(), u"1234567890123456.78"_s);

    v = sqlApplyScale(stored, -2, QSql::LowPrecisionInt64);
    QCOMPARE(v.metaType().id(), QMetaType::LongLong);
    QCOMPARE(v.toLongLong(), Q_INT64_C(1234567890123456)); // exact, no double round-trip

    v = sqlApplyScale(Q_INT64_C(-123499), -2, QSql::LowPrecisionInt64);
    QCOMPARE(v.toLongLong(), Q_INT64_C(-1234)); // truncation toward zero

    bool overflow = true;
    v = sqlApplyScale(Q_INT64_C(123456), -2, QSql::LowPrecisionInt32, &overflow);
    QCOMPARE(v.metaType().id(), QMetaType::Int);
    QCOMPARE(v.toInt(), 1234);
    QVERIFY(!overflow);

    // An integer part beyond the Int32 target comes back as a double, flagged,
    // for the 64-bit width just like for 128 bits - never wrapped
    v = sqlApplyScale(stored, -2, QSql::LowPrecisionInt32, &overflow);
    QCOMPARE(v.metaType().id(), QMetaType::Double);
    QCOMPARE(v.toDouble(), 1234567890123456.78);
    QVERIFY(overflow);
    v = sqlApplyScale(stored, -2, QSql::LowPrecisionInt32); // flag is optional
    QCOMPARE(v.metaType().id(), QMetaType::Double);

    v = sqlApplyScale(Q_INT64_C(150), -2, QSql::LowPrecisionDouble);
    QCOMPARE(v.metaType().id(), QMetaType::Double);
    QCOMPARE(v.toDouble(), 1.5);

    // LowPrecisionDouble is correctly rounded (exact comparisons: QCOMPARE on
    // doubles is fuzzy). 998 * 0.1 would give 99.80000000000001; dividing by
    // the exact 10 gives the nearest double, for every width alike.
    v = sqlApplyScale(Q_INT64_C(998), -1, QSql::LowPrecisionDouble);
    QVERIFY(v.toDouble() == 99.8);
    QVERIFY(sqlApplyScale<qint16>(998, -1, QSql::LowPrecisionDouble).toDouble() == 99.8);
    QVERIFY(sqlApplyScale<qint32>(998, -1, QSql::LowPrecisionDouble).toDouble() == 99.8);
    // Beyond 2^53 the value takes the decimal-text route, which rounds the
    // same way as parsing the text does
    v = sqlApplyScale(Q_INT64_C(9007199254740993), -1, QSql::LowPrecisionDouble);
    QVERIFY(v.toDouble() == u"900719925474099.3"_s.toDouble());
}

void tst_QSqlNumeric::scaledFromVariant()
{
    qint64 out = 0;
    QString error;

    // Integral fast path
    QVERIFY(sqlScaledFromVariant(QVariant(42), -2, &out, &error));
    QCOMPARE(out, Q_INT64_C(4200));
    QVERIFY(sqlScaledFromVariant(QVariant(true), -2, &out, &error));
    QCOMPARE(out, Q_INT64_C(100));
    QVERIFY(sqlScaledFromVariant(QVariant(quint64(9223372036854775807ULL)), 0, &out, &error));
    QCOMPARE(out, (std::numeric_limits<qint64>::max)());

    // ULongLong beyond the signed range is rejected, not wrapped
    QVERIFY(!sqlScaledFromVariant(QVariant(quint64(9223372036854775808ULL)), 0, &out, &error));
    QVERIFY(error.contains(u"out of range"_s));

    // Fast-path overflow through the scale factor
    QVERIFY(!sqlScaledFromVariant(QVariant(Q_INT64_C(1234567890)), -10, &out, &error));
    QVERIFY(error.contains(u"out of range"_s));

    // 10^18 is the widest fast-path factor for qint64; 5 * 10^18 still fits
    QVERIFY(sqlScaledFromVariant(QVariant(5), -18, &out, &error));
    QCOMPARE(out, Q_INT64_C(5000000000000000000));

    // String path: exact, rounded half away from zero
    QVERIFY(sqlScaledFromVariant(QVariant(u"9.995"_s), -2, &out, &error));
    QCOMPARE(out, Q_INT64_C(1000));
    QVERIFY(sqlScaledFromVariant(QVariant(u"-123456789012345.678"_s), -3, &out, &error));
    QCOMPARE(out, Q_INT64_C(-123456789012345678));

    // Malformed input fails with a message; a null errorText must not crash
    QVERIFY(!sqlScaledFromVariant(QVariant(u"1e5"_s), -2, &out, &error));
    QVERIFY(error.contains(u"Cannot encode"_s));
    QVERIFY(!sqlScaledFromVariant<qint64>(QVariant(u"abc"_s), 0, &out, nullptr));

    // Narrow instantiations reject what T cannot hold instead of wrapping
    qint16 s16 = 0;
    QVERIFY(sqlScaledFromVariant(QVariant(1234), 0, &s16, &error));
    QCOMPARE(s16, qint16(1234));
    QVERIFY(sqlScaledFromVariant(QVariant(-42), -2, &s16, &error));
    QCOMPARE(s16, qint16(-4200));
    QVERIFY(!sqlScaledFromVariant(QVariant(65536), 0, &s16, &error)); // qint16(65536) == 0
    QVERIFY(error.contains(u"out of range"_s));
    QVERIFY(!sqlScaledFromVariant(QVariant(100000), 0, &s16, &error)); // would wrap to -31072
    QVERIFY(error.contains(u"out of range"_s));
    qint32 s32 = 0;
    QVERIFY(sqlScaledFromVariant(QVariant((std::numeric_limits<qint32>::min)()), 0, &s32, &error));
    QCOMPARE(s32, (std::numeric_limits<qint32>::min)());
    QVERIFY(!sqlScaledFromVariant(QVariant(Q_INT64_C(5000000000)), 0, &s32, &error));
    QVERIFY(error.contains(u"out of range"_s));
}

void tst_QSqlNumeric::scaledToString128()
{
#ifdef QSQL_HAS_INT128
    QCOMPARE(sqlScaledToString(int128FromString(int128MinStr), 0),
             QString::fromLatin1(int128MinStr));
    QCOMPARE(sqlScaledToString(int128FromString(int128MaxStr), 0),
             QString::fromLatin1(int128MaxStr));
    QCOMPARE(sqlScaledToString(int128FromString(int128MinStr), -2),
             u"-1701411834604692317316873037158841057.28"_s);
    QCOMPARE(sqlScaledToString(qinternalint128(-4), -3), u"-0.004"_s);

    // Chunk boundaries of the base-10^19 formatting underneath (covered in
    // detail by tst_QLocale::int128ToBasicLatin) pass through unchanged
    QCOMPARE(sqlScaledToString(sqlIntPow10<qinternalint128>(19), 0),
             u"10000000000000000000"_s);
    QCOMPARE(sqlScaledToString(int128FromString("20000000000000000000"), 0),
             u"20000000000000000000"_s);
    QCOMPARE(sqlScaledToString(int128FromString("100000000000000000000000000000000000001"), 0),
             u"100000000000000000000000000000000000001"_s);
#else
    QSKIP("No 128-bit integer type on this toolchain");
#endif
}

void tst_QSqlNumeric::decimalToScaled128()
{
#ifdef QSQL_HAS_INT128
    qinternalint128 out = 0;

    // Exact boundaries
    QVERIFY(sqlDecimalToScaled(QByteArray(int128MinStr), 0, &out));
    QCOMPARE(sqlScaledToString(out, 0), QString::fromLatin1(int128MinStr));
    QVERIFY(sqlDecimalToScaled(QByteArray(int128MaxStr), 0, &out));
    QCOMPARE(sqlScaledToString(out, 0), QString::fromLatin1(int128MaxStr));
    QVERIFY(!sqlDecimalToScaled(QByteArray("170141183460469231731687303715884105728"), 0, &out));
    QVERIFY(!sqlDecimalToScaled(QByteArray("-170141183460469231731687303715884105729"), 0, &out));
    QVERIFY(!sqlDecimalToScaled(QByteArray("1234567890123456789012345678901234567890"), 0, &out));

    // Wider than 64 bits, scaled, round-trips exactly
    QVERIFY(sqlDecimalToScaled(QByteArray("123456789012345678901234.56"), -2, &out));
    QCOMPARE(sqlScaledToString(out, -2), u"123456789012345678901234.56"_s);
#else
    QSKIP("No 128-bit integer type on this toolchain");
#endif
}

void tst_QSqlNumeric::applyScale128()
{
#ifdef QSQL_HAS_INT128
    bool overflow = true;

    // In range: exact integers, flag stays clear
    const qinternalint128 small = int128FromString("123499");
    QVariant v = sqlApplyScale(small, -2, QSql::LowPrecisionInt64, &overflow);
    QVERIFY(!overflow);
    QCOMPARE(v.metaType().id(), QMetaType::LongLong);
    QCOMPARE(v.toLongLong(), Q_INT64_C(1234));
    v = sqlApplyScale(small, -2, QSql::LowPrecisionInt32, &overflow);
    QVERIFY(!overflow);
    QCOMPARE(v.metaType().id(), QMetaType::Int);
    QCOMPARE(v.toInt(), 1234);

    // Integer part exceeding the target: Double + flag
    const qinternalint128 wide = int128FromString("12345678901234567890150"); // 123456789012345678901.50
    v = sqlApplyScale(wide, -2, QSql::LowPrecisionInt64, &overflow);
    QVERIFY(overflow);
    QCOMPARE(v.metaType().id(), QMetaType::Double);
    QCOMPARE(v.toDouble(), 123456789012345678901.50);

    // HighPrecision: the exact string
    v = sqlApplyScale(wide, -2, QSql::HighPrecision, &overflow);
    QVERIFY(!overflow);
    QCOMPARE(v.metaType().id(), QMetaType::QString);
    QCOMPARE(v.toString(), u"123456789012345678901.50"_s);

    // LowPrecisionDouble
    v = sqlApplyScale(int128FromString("150"), -2, QSql::LowPrecisionDouble, &overflow);
    QVERIFY(!overflow);
    QCOMPARE(v.metaType().id(), QMetaType::Double);
    QCOMPARE(v.toDouble(), 1.5);
    // ... correctly rounded, and identical to what the narrower widths yield
    v = sqlApplyScale(int128FromString("998"), -1, QSql::LowPrecisionDouble, &overflow);
    QVERIFY(v.toDouble() == 99.8);
    QVERIFY(v.toDouble() == sqlApplyScale(Q_INT64_C(998), -1, QSql::LowPrecisionDouble).toDouble());
#else
    QSKIP("No 128-bit integer type on this toolchain");
#endif
}

void tst_QSqlNumeric::scaledFromVariant128()
{
#ifdef QSQL_HAS_INT128
    qinternalint128 out = 0;
    QString error;

    // ULongLong above INT64_MAX takes the fast path unwrapped
    QVERIFY(sqlScaledFromVariant(QVariant(quint64(18446744073709551615ULL)), 0, &out, &error));
    QCOMPARE(sqlScaledToString(out, 0), u"18446744073709551615"_s);

    // Widest fast-path factors: 10^19 fits, 10^20 overflows for INT64_MIN
    QVERIFY(sqlScaledFromVariant(QVariant((std::numeric_limits<qlonglong>::min)()), -19,
                                 &out, &error));
    QCOMPARE(sqlScaledToString(out, -19), u"-9223372036854775808.0000000000000000000"_s);
    QVERIFY(!sqlScaledFromVariant(QVariant((std::numeric_limits<qlonglong>::min)()), -20,
                                  &out, &error));
    QVERIFY(error.contains(u"out of range"_s));

    // String path: min/max and beyond
    QVERIFY(sqlScaledFromVariant(QVariant(QString::fromLatin1(int128MinStr)), 0, &out, &error));
    QCOMPARE(sqlScaledToString(out, 0), QString::fromLatin1(int128MinStr));
    QVERIFY(!sqlScaledFromVariant(QVariant(u"170141183460469231731687303715884105728"_s), 0,
                                  &out, &error));
    QVERIFY(error.contains(u"Cannot encode"_s));

    // A double whose digits lie beyond the 18th decimal binds exactly at a
    // wider scale (it used to be cut to 18 decimals and bound as 0)
    QVERIFY(sqlScaledFromVariant(QVariant(1.5e-19), -20, &out, &error));
    QCOMPARE(sqlScaledToString(out, -20), u"0.00000000000000000015"_s);
    // ... and rounds at a narrower one, like the same text would
    QVERIFY(sqlScaledFromVariant(QVariant(1.5e-19), -19, &out, &error));
    QCOMPARE(sqlScaledToString(out, -19), u"0.0000000000000000002"_s);
#else
    QSKIP("No 128-bit integer type on this toolchain");
#endif
}

void tst_QSqlNumeric::int128Words()
{
#ifdef QSQL_HAS_INT128
    const struct {
        const char *name;
        quint64 low;
        quint64 high;
    } rows[] = {
        { "zero", 0, 0 },
        { "minus one", ~quint64(0), ~quint64(0) },
        { "min", 0, Q_UINT64_C(0x8000000000000000) },
        { "max", ~quint64(0), Q_UINT64_C(0x7fffffffffffffff) },
        { "mixed", Q_UINT64_C(0x0123456789abcdef), Q_UINT64_C(0x0fedcba987654321) },
    };
    for (const auto &row : rows) {
        const qinternalint128 v = sqlInt128FromWords(row.low, row.high);
        quint64 low = 1, high = 1;
        sqlInt128ToWords(v, &low, &high);
        QCOMPARE_EQ(low, row.low);
        QCOMPARE_EQ(high, row.high);
    }

    // The words really are the value: 2^64 + 5
    const qinternalint128 v = sqlInt128FromWords(5, 1);
    QCOMPARE(sqlScaledToString(v, 0), u"18446744073709551621"_s);
    QCOMPARE(sqlInt128FromWords(Q_UINT64_C(0x8000000000000000), 0),
             qinternalint128(1) << 63);
#else
    QSKIP("No 128-bit integer type on this toolchain");
#endif
}

QTEST_APPLESS_MAIN(tst_QSqlNumeric)
#include "tst_qsqlnumeric.moc"
