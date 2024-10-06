// Copyright (C) 2023 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/qtypes.h>
#include <QtCore/qcompare.h>

#include <memory>

#include <q20chrono.h>

using ToStringFunction = std::function<char *()>;
class tst_toString : public QObject
{
    Q_OBJECT
private:
    void addColumns();
    void testRows();
private slots:
    void int128();

    void chrono_duration_data();
    void chrono_duration() { testRows(); }

    void orderingTypeValue_data();
    void orderingTypeValue() { testRows(); }
};

void tst_toString::addColumns()
{
    QTest::addColumn<ToStringFunction>("fn");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<QByteArrayView>("expr");
    QTest::addColumn<QByteArrayView>("file");
    QTest::addColumn<int>("line");
}

void tst_toString::testRows()
{
    QFETCH(ToStringFunction, fn);
    QFETCH(const QByteArray, expected);
    QFETCH(QByteArrayView, expr);
    QFETCH(QByteArrayView, file);
    QFETCH(int, line);

    std::unique_ptr<char []> ptr{fn()};
    const auto len = qstrlen(ptr.get());
    QTest::qCompare(ptr.get(), expected, expr.data(), expected.data(), file.data(), line);
    if (QTest::currentTestFailed()) {
        qDebug("tail diff:\n"
               "   actual:%s\n"
               " expected:%s",
               ptr.get() + len - std::min(size_t{40}, len),
               expected.data() + expected.size() - std::min(qsizetype{40}, expected.size()));
    }
}

template <typename T> void addRow(QByteArrayView name, T &&value, QByteArrayView expression,
                                  const QByteArray &expected, QByteArrayView file, int line)
{
    ToStringFunction fn = [v = std::move(value)]() { return QTest::toString(v); };
    QTest::newRow(name.data()) << fn << expected << expression << file << line;
}

#define ADD_ROW(name, expr, expected)         \
    ::addRow(name, expr, #expr, expected, __FILE__, __LINE__)

void tst_toString::int128()
{
#ifndef QT_SUPPORTS_INT128
    QSKIP("This test requires int128 support enabled in the compiler.");
#else
    // ### port to data-driven once QVariant has support for qint128/quint128
    std::unique_ptr<char[]> s;

    {
        // build Q_INT128_MIN without using Q_INT128_ macros,
        // because we use Q_INT128_MIN in the impl
        qint128 accu = 1701411834604692317LL;
        accu *= 1000000000000000000LL;
        accu +=  316873037158841057LL;
        accu *= -100;
        accu -= 28;
        QCOMPARE_EQ(accu, Q_INT128_MIN);
        s.reset(QTest::toString(accu));
        QCOMPARE(s.get(), "-170141183460469231731687303715884105728");
    }

    // now test with the macro, too:
    s.reset(QTest::toString(Q_INT128_MIN));
    QCOMPARE(s.get(), "-170141183460469231731687303715884105728");

    s.reset(QTest::toString(Q_INT128_MIN + 1));
    QCOMPARE(s.get(), "-170141183460469231731687303715884105727");

    s.reset(QTest::toString(Q_INT128_MAX));
    QCOMPARE(s.get(), "170141183460469231731687303715884105727");

    s.reset(QTest::toString(Q_INT128_MAX - 1));
    QCOMPARE(s.get(), "170141183460469231731687303715884105726");

    s.reset(QTest::toString(Q_UINT128_MAX));
    QCOMPARE(s.get(), "340282366920938463463374607431768211455");

    s.reset(QTest::toString(Q_UINT128_MAX - 1));
    QCOMPARE(s.get(), "340282366920938463463374607431768211454");

    s.reset(QTest::toString(quint128{0}));
    QCOMPARE(s.get(), "0");

    s.reset(QTest::toString(qint128{0}));
    QCOMPARE(s.get(), "0");

    s.reset(QTest::toString(qint128{-1}));
    QCOMPARE(s.get(), "-1");
#endif // QT_SUPPORTS_INT128
}

void tst_toString::chrono_duration_data()
{
    addColumns();

    using namespace std::chrono;
    using namespace q20::chrono;

    using attoseconds = duration<int64_t, std::atto>;
    using femtoseconds = duration<int64_t, std::femto>;
    using picoseconds = duration<int64_t, std::pico>;
    using centiseconds = duration<int64_t, std::centi>;
    using deciseconds = duration<int64_t, std::deci>;
    using kiloseconds = duration<int64_t, std::kilo>;
    using decades = duration<int, std::ratio_multiply<years::period, std::deca>>; // decayears
    using centuries = duration<int16_t, std::ratio_multiply<years::period, std::hecto>>; // hectoyears
    using millennia = duration<int16_t, std::ratio_multiply<years::period, std::kilo>>; // kiloyears
    using gigayears [[maybe_unused]] = duration<int8_t, std::ratio_multiply<years::period, std::giga>>;
    using fortnights = duration<int, std::ratio_multiply<days::period, std::ratio<14>>>;
    using microfortnights = duration<int64_t, std::ratio_multiply<fortnights::period, std::micro>>;
    using meter_per_light = duration<int64_t, std::ratio<1, 299'792'458>>;
    using kilometer_per_light = duration<int64_t, std::ratio<1000, 299'792'458>>;
    using AU_per_light = duration<int64_t, std::ratio<149'597'871'800, 299'792'458>>;
    using pstn_rate = duration<int64_t, std::ratio<1, 8000>>; // PSTN sampling rate (8 kHz)
    using hyperfine = duration<int64_t, std::ratio<1, 9'192'631'770>>;  // definition of second

    ADD_ROW("1as", attoseconds{1}, "1as (1e-18s)");   // from Norwegian "atten" (18)
    ADD_ROW("1fs", femtoseconds{1}, "1fs (1e-15s)");  // from Norwegian "femten" (15)
    ADD_ROW("1ps", picoseconds{1}, "1ps (1e-12s)");   // from Italian piccolo?
    ADD_ROW("0ns", 0ns, "0ns (0s)");
    ADD_ROW("1000ns", 1000ns, "1000ns (1e-06s)");
    ADD_ROW("1us", 1us, "1us (1e-06s)");
    ADD_ROW("125us", 125us, "125us (0.000125s)");
    ADD_ROW("0ms", 0ms, "0ms (0s)");
    ADD_ROW("-1s", -1s, "-1s");
    ADD_ROW("0s", 0s, "0s");
    ADD_ROW("1cs", centiseconds{1}, "1cs (0.01s)");
    ADD_ROW("2ds", deciseconds{2}, "2ds (0.2s)");
    ADD_ROW("1s", 1s, "1s");
    ADD_ROW("60s", 60s, "60s");
    ADD_ROW("1min", 1min, "1min (60s)");
    ADD_ROW("1h", 1h, "1h (3600s)");
    ADD_ROW("1days", days{1}, "1d (86400s)");
    ADD_ROW("7days", days{7}, "7d (604800s)");
    ADD_ROW("1weeks", weeks{1}, "1wk (604800s)");
    ADD_ROW("365days", days{365}, "365d (31536000s)");
    ADD_ROW("1years", years{1}, "1yr (31556952s)"); // 365.2425 days

    ADD_ROW("2ks", kiloseconds{2}, "2[1000]s (2000s)");
    ADD_ROW("1fortnights", fortnights{1}, "1[2]wk (1209600s)");
    ADD_ROW("1decades", decades{1}, "1[10]yr (315569520s)");
    ADD_ROW("1centuries", centuries{1}, "1[100]yr (3.1556952e+09s)");
    ADD_ROW("1millennia", millennia{1}, "1[1000]yr (3.1556952e+10s)");
#if defined(Q_OS_LINUX) || defined(Q_OS_DARWIN) || defined(__GLIBC__)
    // some OSes print the exponent differently
    ADD_ROW("13gigayears", gigayears{13}, "13[1e+09]yr (4.10240376e+17s)");
#endif

    // months are one twelfth of a Gregorian year, not 30 days
    ADD_ROW("1months", months{1}, "1[2629746]s (2629746s)");
    ADD_ROW("12months", months{12}, "12[2629746]s (31556952s)");

    // weird units
    ADD_ROW("2microfortnights", microfortnights{2}, "2[756/625]s (2.4192s)");
    ADD_ROW("1pstn_rate", pstn_rate{1}, "1[1/8000]s (0.000125s)");   // 125µs
    ADD_ROW("10m/c", meter_per_light{10}, "10[1/299792458]s (3.33564095e-08s)");
    ADD_ROW("10km/c", kilometer_per_light{10}, "10[500/149896229]s (3.33564095e-05s)");
    ADD_ROW("1AU/c", AU_per_light{1}, "1[74798935900/149896229]s (499.004788s)");
    ADD_ROW("Cs133-hyperfine", hyperfine{1}, "1[1/9192631770]s (1.08782776e-10s)");
    ADD_ROW("1sec-definition", hyperfine{9'192'631'770}, "9192631770[1/9192631770]s (1s)");
    ADD_ROW("8000pstn_rate", pstn_rate{8000}, "8000[1/8000]s (1s)");

    // real floting point
    // current (2023) best estimate is 13.813 ± 0.038 billion years (Plank Collaboration)
    using universe [[maybe_unused]] = duration<double, std::ratio_multiply<std::ratio<13'813'000'000>, years::period>>;
    using fpksec = duration<double, std::kilo>;
    using fpsec = duration<double>;
    using fpmsec = duration<double, std::milli>;
    using fpnsec = duration<double, std::nano>;
    using fpGyr [[maybe_unused]] = duration<double, std::ratio_multiply<years::period, std::giga>>;

    ADD_ROW("1.0s", fpsec{1}, "1s");
    ADD_ROW("1.5s", fpsec{1.5}, "1.5s");
    ADD_ROW("-1.0ms", fpmsec{-1}, "-1ms (-0.001s)");
    ADD_ROW("1.5ms", fpmsec{1.5}, "1.5ms (0.0015s)");
    ADD_ROW("1.0ns", fpnsec{1}, "1ns (1e-09s)");
    ADD_ROW("-1.5ns", fpnsec{-1.5}, "-1.5ns (-1.5e-09s)");
    ADD_ROW("1.0ks", fpksec{1}, "1[1000]s (1000s)");
    ADD_ROW("-1.5ks", fpksec{-1.5}, "-1.5[1000]s (-1500s)");
    ADD_ROW("1.0zs", fpsec{1e-21}, "1e-21s");  // zeptosecond
    ADD_ROW("1.0ys", fpsec{1e-24}, "1e-24s");  // yoctosecond
    ADD_ROW("planck-time", fpsec(5.39124760e-44), "5.3912476e-44s");
#if defined(Q_OS_LINUX) || defined(Q_OS_DARWIN) || defined(__GLIBC__)
    // some OSes print the exponent differently
    ADD_ROW("13.813Gyr", fpGyr(13.813), "13.813[1e+09]yr (4.35896178e+17s)");
    ADD_ROW("1universe", universe{1}, "1[1.3813e+10]yr (4.35896178e+17s)");
#endif
}

void tst_toString::orderingTypeValue_data()
{
    addColumns();
#define CHECK(x) ADD_ROW(#x, x, #x)
    CHECK(Qt::strong_ordering::equal);
    CHECK(Qt::strong_ordering::less);
    CHECK(Qt::strong_ordering::greater);

    CHECK(Qt::partial_ordering::equivalent);
    CHECK(Qt::partial_ordering::less);
    CHECK(Qt::partial_ordering::greater);
    CHECK(Qt::partial_ordering::unordered);

    CHECK(Qt::weak_ordering::equivalent);
    CHECK(Qt::weak_ordering::less);
    CHECK(Qt::weak_ordering::greater);
#ifdef __cpp_lib_three_way_comparison
    CHECK(std::strong_ordering::equal);
    CHECK(std::strong_ordering::less);
    CHECK(std::strong_ordering::greater);

    CHECK(std::partial_ordering::equivalent);
    CHECK(std::partial_ordering::less);
    CHECK(std::partial_ordering::greater);
    CHECK(std::partial_ordering::unordered);

    CHECK(std::weak_ordering::equivalent);
    CHECK(std::weak_ordering::less);
    CHECK(std::weak_ordering::greater);
#endif // __cpp_lib_three_way_comparison
#undef CHECK
}

QTEST_APPLESS_MAIN(tst_toString)
#include "tst_tostring.moc"
