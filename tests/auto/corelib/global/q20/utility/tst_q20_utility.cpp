// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/q20utility.h>
#include <QtCore/qcompare.h>

class tst_q20_utility : public QObject
{
    Q_OBJECT
private:
    template <typename LeftType, typename RightType>
    void dataImpl1();
    template <typename LeftType, typename RightType>
    void dataImpl2();
    template <typename LeftType, typename RightType>
    void compareImpl();

private Q_SLOTS:
    void compareShortInt_data() { dataImpl1<short, ushort>(); }
    void compareShortInt() { compareImpl<short, ushort>(); }
    void compareIntegers_data() { dataImpl1<int, uint>(); }
    void compareIntegers() { compareImpl<int, uint>(); }
    void compareLongInt_data() { dataImpl1<long, ulong>(); }
    void compareLongInt() { compareImpl<long, ulong>(); }
    void compareQLonglongInt_data() { dataImpl1<qlonglong, qulonglong>(); }
    void compareQLonglongInt() { compareImpl<qlonglong, qulonglong>(); }
#ifdef QT_SUPPORTS_INT128
    void compareQInt128_data() { dataImpl1<qint128, quint128>(); }
    void compareQInt128() { compareImpl<qint128, quint128>(); }
    void compareMixedQInt128ULonglong_data() { dataImpl2<qint128, qulonglong>(); }
    void compareMixedQInt128ULonglong() { compareImpl<qint128, qulonglong>(); }
    void compareMixedSignedCharQUint128_data() { dataImpl1<signed char, quint128>(); }
    void compareMixedSignedCharQUint128() { compareImpl<signed char, quint128>(); }
#endif
    void compareCrossSizeTypes_data();
    void compareCrossSizeTypes() { compareImpl<qlonglong, ushort>(); }
    void compareMixedShortUChar_data() { dataImpl2<short, uchar>(); }
    void compareMixedShortUChar() { compareImpl<short, uchar>(); }
    void compareMixedIntUShort_data() { dataImpl2<int, ushort>(); }
    void compareMixedIntUShort() { compareImpl<int, ushort>(); }
    void compareMixedLongUInt_data() { dataImpl2<long, uint>(); }
    void compareMixedLongUInt() { compareImpl<long, uint>(); }
    void compareMixedQLonglongULong_data() { dataImpl1<qlonglong, ulong>(); }
    void compareMixedQLonglongULong() { compareImpl<qlonglong, ulong>(); }
    void inRange();
};

template <typename LeftType, typename RightType>
void tst_q20_utility::dataImpl1()
{
    QTest::addColumn<LeftType>("a");
    QTest::addColumn<RightType>("b");
    QTest::addColumn<Qt::strong_ordering>("ordering");

    const LeftType lmin = (std::numeric_limits<LeftType>::min)();
    const LeftType lmax = (std::numeric_limits<LeftType>::max)();
    const RightType rmax = (std::numeric_limits<RightType>::max)();

    //                           LeftType          RightType          a <=> b Qt::strong_ordering
    QTest::addRow("0_0")      << LeftType(0)    << RightType(0)    << Qt::strong_ordering::equal;
    QTest::addRow("-1_1")     << LeftType(-1)   << RightType(1)    << Qt::strong_ordering::less;
    QTest::addRow("min_0")    << lmin           << RightType(0)    << Qt::strong_ordering::less;
    QTest::addRow("min_max")  << lmin           << RightType(lmax) << Qt::strong_ordering::less;
    QTest::addRow("min_umax") << lmin           << rmax            << Qt::strong_ordering::less;
    QTest::addRow("-1_max")   << LeftType(-1)   << RightType(lmax) << Qt::strong_ordering::less;
    QTest::addRow("-1_umax")  << LeftType(-1)   << rmax            << Qt::strong_ordering::less;
    QTest::addRow("126_126")  << LeftType(126)  << RightType(126)  << Qt::strong_ordering::equal;
}

template <typename LeftType, typename RightType>
void tst_q20_utility::compareImpl()
{
    QFETCH(LeftType, a);
    QFETCH(RightType, b);
    QFETCH(Qt::strong_ordering, ordering);

    Qt::strong_ordering reversed = QtOrderingPrivate::reversed(ordering);
    QCOMPARE(q20::cmp_equal(a, b), is_eq(ordering));
    QCOMPARE(q20::cmp_less_equal(a, b), is_lteq(ordering));
    QCOMPARE(q20::cmp_greater_equal(a, b), is_gteq(ordering));
    QCOMPARE(q20::cmp_not_equal(a, b), !is_eq(ordering));
    QCOMPARE(q20::cmp_less(a, b), is_lt(ordering));
    QCOMPARE(q20::cmp_greater(a, b), is_gt(ordering));

    QCOMPARE(q20::cmp_equal(b, a), is_eq(reversed));
    QCOMPARE(q20::cmp_less_equal(b, a), is_lteq(reversed));
    QCOMPARE(q20::cmp_greater_equal(b, a), is_gteq(reversed));
    QCOMPARE(q20::cmp_not_equal(b, a), !is_eq(reversed));
    QCOMPARE(q20::cmp_less(b, a), is_lt(reversed));
    QCOMPARE(q20::cmp_greater(b, a), is_gt(reversed));
}

template <typename LeftType, typename RightType>
void tst_q20_utility::dataImpl2()
{
    QTest::addColumn<LeftType>("a");
    QTest::addColumn<RightType>("b");
    QTest::addColumn<Qt::strong_ordering>("ordering");

    const LeftType lmin = (std::numeric_limits<LeftType>::min)();
    const LeftType lmax = (std::numeric_limits<LeftType>::max)();
    const RightType rmax = (std::numeric_limits<RightType>::max)();

    //                           LeftType          RightType          a <=> b Qt::strong_ordering
    QTest::addRow("0_0")      << LeftType(0)    << RightType(0)    << Qt::strong_ordering::equal;
    QTest::addRow("-1_1")     << LeftType(-1)   << RightType(1)    << Qt::strong_ordering::less;
    QTest::addRow("min_0")    << lmin           << RightType(0)    << Qt::strong_ordering::less;
    QTest::addRow("min_max")  << lmin           << RightType(lmax) << Qt::strong_ordering::less;
    QTest::addRow("min_umax") << lmin           << rmax            << Qt::strong_ordering::less;
    QTest::addRow("-1_max")   << LeftType(-1)   << RightType(lmax) << Qt::strong_ordering::less;
    QTest::addRow("-1_umax")  << LeftType(-1)   << rmax            << Qt::strong_ordering::less;
    QTest::addRow("126_126")  << LeftType(126)  << RightType(126)  << Qt::strong_ordering::equal;
}

void tst_q20_utility::compareCrossSizeTypes_data()
{
    QTest::addColumn<qlonglong>("a");
    QTest::addColumn<ushort>("b");
    QTest::addColumn<Qt::strong_ordering>("ordering");

    //                           qlonglong              ushort               a <=> b Qt::strong_ordering
    QTest::addRow("0_0")      << qlonglong(0)        << ushort(0)         << Qt::strong_ordering::equal;
    QTest::addRow("-1_1")     << qlonglong(-1)       << ushort(1)         << Qt::strong_ordering::less;
    QTest::addRow("min_0")    << qlonglong(SHRT_MIN) << ushort(0)         << Qt::strong_ordering::less;
    QTest::addRow("min_max")  << qlonglong(SHRT_MIN) << ushort(SHRT_MAX)  << Qt::strong_ordering::less;
    QTest::addRow("min_umax") << qlonglong(SHRT_MIN) << ushort(USHRT_MAX) << Qt::strong_ordering::less;
    QTest::addRow("max_max")  << qlonglong(SHRT_MAX) << ushort(SHRT_MAX)  << Qt::strong_ordering::equal;
    QTest::addRow("-1_max")   << qlonglong(-1)       << ushort(SHRT_MAX)  << Qt::strong_ordering::less;
    QTest::addRow("-1_umax")  << qlonglong(-1)       << ushort(USHRT_MAX) << Qt::strong_ordering::less;
    QTest::addRow("max_umax") << qlonglong(SHRT_MAX) << ushort(USHRT_MAX) << Qt::strong_ordering::less;
}

void tst_q20_utility::inRange()
{
    static_assert(q20::in_range<short>(-1));
    static_assert(!q20::in_range<std::size_t>(-1));
    static_assert(q20::in_range<std::size_t>(10));
    static_assert(!q20::in_range<ushort>(-1));
    static_assert(q20::in_range<ushort>(10));
    static_assert(!q20::in_range<ushort>(LONG_MIN));
    static_assert(!q20::in_range<uint>(-1));
    static_assert(q20::in_range<uint>(10));
    static_assert(!q20::in_range<uint>(LONG_MIN));
    static_assert(q20::in_range<short>(-1));
    static_assert(q20::in_range<short>(10));
    static_assert(!q20::in_range<short>(LONG_MIN));
    static_assert(q20::in_range<int>(-1));
    static_assert(q20::in_range<int>(10));
    static_assert((q20::in_range<ulong>(INT_MAX)));
    static_assert(q20::in_range<ulong>(ULONG_MAX));
    static_assert(q20::in_range<ulong>(INT_MAX));
#ifdef QT_SUPPORTS_INT128
    static_assert(!q20::in_range<quint128>(-1));
    static_assert(q20::in_range<qint128>(-1));
#endif
}

QTEST_MAIN(tst_q20_utility)
#include "tst_q20_utility.moc"
