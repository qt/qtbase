// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qcomparehelpers.h"

#define DECLARE_TYPE(Name, Type, RetType, Constexpr, Noex, Suffix, ...) \
class Templated ## Name \
{ \
public: \
    Constexpr Templated ## Name () {} \
\
private: \
    __VA_ARGS__ \
    friend Constexpr bool \
    comparesEqual(const Templated ## Name &lhs, X rhs) noexcept(Noex) \
    { Q_UNUSED(lhs); Q_UNUSED(rhs); return true; } \
    __VA_ARGS__ \
    friend Constexpr RetType \
    compareThreeWay(const Templated ## Name &lhs, X rhs) noexcept(Noex) \
    { Q_UNUSED(lhs); Q_UNUSED(rhs); return RetType::equivalent; } \
    Q_DECLARE_ ## Type ## _ORDERED ## Suffix (Templated ## Name, X, __VA_ARGS__) \
}; \
/* END */

#define DECLARE_AUTO_TYPE(Name, Type, Constexpr, Noex, Suffix, ...) \
class TemplatedAuto ## Name \
{ \
public: \
    Constexpr TemplatedAuto ## Name () {} \
    Constexpr TemplatedAuto ## Name (Type v) : val(v) {} \
\
private: \
    __VA_ARGS__ \
    friend Constexpr bool \
    comparesEqual(const TemplatedAuto ## Name &lhs, X rhs) noexcept(Noex) \
    { return lhs.val == rhs; } \
    __VA_ARGS__ \
    friend Constexpr auto \
    compareThreeWay(const TemplatedAuto ## Name &lhs, X rhs) noexcept(Noex) \
    { using Qt::compareThreeWay; return compareThreeWay(lhs.val, rhs); } \
    Q_DECLARE_ORDERED ## Suffix (TemplatedAuto ## Name, X, __VA_ARGS__) \
\
    Type val = {}; \
}; \
/* END */

#define DECLARE_TYPES_FOR_N_ATTRS(N, ...) \
DECLARE_TYPE(PartialConst ## N, PARTIALLY, Qt::partial_ordering, constexpr, \
             true, _LITERAL_TYPE, __VA_ARGS__) \
DECLARE_TYPE(Partial ## N, PARTIALLY, Qt::partial_ordering, , true, , __VA_ARGS__) \
DECLARE_TYPE(PartialNonNoex ## N, PARTIALLY, Qt::partial_ordering, , false, \
             _NON_NOEXCEPT, __VA_ARGS__) \
DECLARE_TYPE(WeakConst ## N, WEAKLY, Qt::weak_ordering, constexpr, true, \
             _LITERAL_TYPE, __VA_ARGS__) \
DECLARE_TYPE(Weak ## N, WEAKLY, Qt::weak_ordering, , true, , __VA_ARGS__) \
DECLARE_TYPE(WeakNonNoex ## N, WEAKLY, Qt::weak_ordering, , false, \
             _NON_NOEXCEPT, __VA_ARGS__) \
DECLARE_TYPE(StrongConst ## N, STRONGLY, Qt::strong_ordering, constexpr, true, \
             _LITERAL_TYPE, __VA_ARGS__) \
DECLARE_TYPE(Strong ## N, STRONGLY, Qt::strong_ordering, , true, , __VA_ARGS__) \
DECLARE_TYPE(StrongNonNoex ## N, STRONGLY, Qt::strong_ordering, , false, \
             _NON_NOEXCEPT, __VA_ARGS__) \
DECLARE_AUTO_TYPE(Def ## N, int, , true, , __VA_ARGS__) \
DECLARE_AUTO_TYPE(Const ## N, int, constexpr, true, _LITERAL_TYPE, __VA_ARGS__) \
DECLARE_AUTO_TYPE(NonNoex ## N, int, , false, _NON_NOEXCEPT, __VA_ARGS__) \
/* END */

template <typename T>
using if_int = std::enable_if_t<std::is_same_v<T, int>, bool>;

// The code below tries to craft some artificial template constraints that
// would fit into 1-7 macro arguments.
DECLARE_TYPES_FOR_N_ATTRS(1, template <typename X>)
DECLARE_TYPES_FOR_N_ATTRS(2, template <typename X, if_int<X> = true>)
DECLARE_TYPES_FOR_N_ATTRS(3, template <typename X, std::enable_if_t<std::is_integral_v<X>,
                                                                    bool> = true>)
DECLARE_TYPES_FOR_N_ATTRS(4, template <typename X, std::enable_if_t<std::is_same_v<X, int>,
                                                                    bool> = true>)
DECLARE_TYPES_FOR_N_ATTRS(5, template <typename X,
                                       std::enable_if_t<std::disjunction_v<
                                                                std::is_same<X, int>,
                                                                std::is_floating_point<X>>,
                                                        bool> = true>)
DECLARE_TYPES_FOR_N_ATTRS(6, template <typename X,
                                       std::enable_if_t<std::disjunction_v<
                                                                std::is_same<X, int>,
                                                                std::is_same<X, short>>,
                                                        bool> = true>)
DECLARE_TYPES_FOR_N_ATTRS(7, template <typename X,
                                       std::enable_if_t<std::disjunction_v<
                                                                std::is_same<X, int>,
                                                                std::is_same<X, short>,
                                                                std::is_floating_point<X>>,
                                                        bool> = true>)

#undef DECLARE_TYPES_FOR_N_ATTRS
#undef DECLARE_TYPE

void tst_QCompareHelpers::compareWithAttributes()
{
#define COMPARE(ClassName) \
    do { \
        ClassName c; \
        QCOMPARE_EQ(c, 0); \
        QCOMPARE_LE(c, 0); \
        QCOMPARE_GE(0, c); \
    } while (false)

#define COMPARE_SET(N) \
    COMPARE(TemplatedPartialConst ## N); \
    COMPARE(TemplatedPartial ## N); \
    COMPARE(TemplatedPartialNonNoex ## N); \
    COMPARE(TemplatedWeakConst ## N); \
    COMPARE(TemplatedWeak ## N); \
    COMPARE(TemplatedWeakNonNoex ## N); \
    COMPARE(TemplatedStrongConst ## N); \
    COMPARE(TemplatedStrong ## N); \
    COMPARE(TemplatedStrongNonNoex ## N); \
    COMPARE(TemplatedAutoDef ## N); \
    COMPARE(TemplatedAutoConst ## N); \
    COMPARE(TemplatedAutoNonNoex ## N); \
    /* END */

    COMPARE_SET(1)
    COMPARE_SET(2)
    COMPARE_SET(3)
    COMPARE_SET(4)
    COMPARE_SET(5)
    COMPARE_SET(6)
    COMPARE_SET(7)

#undef COMPARE_SET
#undef COMPARE
}

void tst_QCompareHelpers::totallyOrderedWrapperBasics()
{
    Qt::totally_ordered_wrapper<int*> pi; // partially-formed
    pi = nullptr;
    QCOMPARE_EQ(pi.get(), nullptr);

    // Test that we can create a wrapper for void*.
    [[maybe_unused]] constexpr Qt::totally_ordered_wrapper<void*> voidWrp{nullptr};

    // test that operator*() works
    int val = 10;
    Qt::totally_ordered_wrapper<int*> intWrp{&val};
    QCOMPARE_EQ(*intWrp, 10);
    *intWrp = 20;
    QCOMPARE_EQ(*intWrp, 20);
    QCOMPARE_EQ(val, 20);
}

template <typename T>
class AutoComparisonTester
{
public:
    AutoComparisonTester(const T &v) : val(v) {}

private:
    friend bool
    comparesEqual(const AutoComparisonTester &lhs, const AutoComparisonTester &rhs) noexcept
    { return lhs.val == rhs.val; }

    friend auto
    compareThreeWay(const AutoComparisonTester &lhs, const AutoComparisonTester &rhs) noexcept
    { using Qt::compareThreeWay; return compareThreeWay(lhs.val, rhs.val); }

    Q_DECLARE_ORDERED(AutoComparisonTester)

    T val;
};

void tst_QCompareHelpers::compareAutoReturnType()
{
    // strong
    {
        using StrongT = AutoComparisonTester<int>;
        static_assert(std::is_same_v<decltype(compareThreeWay(std::declval<const StrongT &>(),
                                                              std::declval<const StrongT &>())),
                                     Qt::strong_ordering>);
        QTestPrivate::testAllComparisonOperatorsCompile<StrongT>();
    }
    // partial
    {
        using PartialT = AutoComparisonTester<float>;
        static_assert(std::is_same_v<decltype(compareThreeWay(std::declval<const PartialT &>(),
                                                              std::declval<const PartialT &>())),
                                     Qt::partial_ordering>);
        QTestPrivate::testAllComparisonOperatorsCompile<PartialT>();
    }
    // weak
    {
        using WeakT = AutoComparisonTester<QDateTime>;
        static_assert(std::is_same_v<decltype(compareThreeWay(std::declval<const WeakT &>(),
                                                              std::declval<const WeakT &>())),
                                     Qt::weak_ordering>);
        QTestPrivate::testAllComparisonOperatorsCompile<WeakT>();
    }
}

template <typename T>
class LessOnly
{
public:
    using Type = T;

    LessOnly(T v) : val(v) {}
    T value() const { return val; }
private:
    friend bool operator<(LessOnly lhs, LessOnly rhs)
    { return lhs.val < rhs.val; }

    T val;
};

template <typename T>
class ThreeWayCmp
{
public:
    using Type = T;

    ThreeWayCmp(T v) : val(v) {}
private:
    template <typename U = T>
    friend bool operator==(ThreeWayCmp lhs, ThreeWayCmp rhs)
    { return lhs.val == rhs.val; }
    template <typename U = T>
    friend bool operator==(ThreeWayCmp lhs, LessOnly<U> rhs)
    { return lhs.val == rhs.value(); }
    template <typename U = T>
    friend auto compareThreeWay(ThreeWayCmp lhs, ThreeWayCmp rhs)
    {
        using Qt::compareThreeWay;
        return compareThreeWay(lhs.val, rhs.val);
    }
    template <typename U = T>
    friend auto compareThreeWay(ThreeWayCmp lhs, LessOnly<U> rhs)
    {
        using Qt::compareThreeWay;
        return compareThreeWay(lhs.val, rhs.value());
    }

    T val;
};

// A dummy int-based class to implement weak ordering
struct Weak
{
    int val;
private:
    friend constexpr bool comparesEqual(Weak lhs, Weak rhs) noexcept
    { return lhs.val == rhs.val; }
    friend constexpr Qt::weak_ordering compareThreeWay(Weak lhs, Weak rhs) noexcept
    {
        const auto r = Qt::compareThreeWay(lhs.val, rhs.val);
        return Qt::weak_ordering{r};
    }
    Q_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE(Weak)
};

template <typename LeftType, typename RightType, typename OrderingType>
void tst_QCompareHelpers::lexicographicalCompareThreeWayDataImpl()
{
    QTest::addColumn<QList<LeftType>>("lhs");
    QTest::addColumn<QList<RightType>>("rhs");
    QTest::addColumn<OrderingType>("expectedResult");

    using LT = typename LeftType::Type;
    using RT = typename RightType::Type;

    constexpr bool HasOnlyOpLess = std::is_same_v<LeftType, LessOnly<LT>>
                                    && std::is_same_v<RightType, LessOnly<RT>>;

    if constexpr (!HasOnlyOpLess)
        static_assert(std::is_same_v<Qt::if_has_qt_compare_three_way<LeftType, RightType>, bool>);

    QTest::addRow("empty")
            << QList<LeftType>{}
            << QList<RightType>{}
            << OrderingType::equivalent;
    QTest::addRow("same_length_equal")
            << QList<LeftType>{LT{1}, LT{2}, LT{3}}
            << QList<RightType>{RT{1}, RT{2}, RT{3}}
            << OrderingType::equivalent;
    QTest::addRow("greater_val")
            << QList<LeftType>{LT{1}, LT{3}, LT{2}}
            << QList<RightType>{RT{1}, RT{2}, RT{3}, RT{4}}
            << OrderingType::greater;
    QTest::addRow("less_val")
            << QList<LeftType>{LT{1}, LT{2}, LT{3}, LT{4}}
            << QList<RightType>{RT{1}, RT{3}, RT{2}}
            << OrderingType::less;
    QTest::addRow("greater_length")
            << QList<LeftType>{LT{1}, LT{2}, LT{3}}
            << QList<RightType>{RT{1}, RT{2}}
            << OrderingType::greater;
    QTest::addRow("less_length")
            << QList<LeftType>{LT{1}, LT{2}, LT{3}}
            << QList<RightType>{RT{1}, RT{2}, RT{3}, RT{4}}
            << OrderingType::less;

    if constexpr (!HasOnlyOpLess && std::is_same_v<OrderingType, Qt::partial_ordering>) {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        QTest::addRow("unordered")
                << QList<LeftType>{LT{nan}, LT{2}, LT{3}}
                << QList<RightType>{RT{1}, RT{2}, RT{3}}
                << OrderingType::unordered;
        QTest::addRow("unordered_start_with_nan")
                << QList<LeftType>{LT{nan}, LT{2}, LT{3}}
                << QList<RightType>{RT{nan}, RT{3}, RT{2}}
                << OrderingType::unordered;
    }
}

template <typename LeftType, typename RightType, typename OrderingType>
void tst_QCompareHelpers::lexicographicalCompareThreeWayImpl()
{
    QFETCH(const QList<LeftType>, lhs);
    QFETCH(const QList<RightType>, rhs);
    QFETCH(const OrderingType, expectedResult);

    const auto res =
            QtOrderingPrivate::lexicographicalCompareThreeWay(lhs.begin(), lhs.end(),
                                                              rhs.begin(), rhs.end());
    QCOMPARE_EQ(res, expectedResult);

    // check the C-style arrays as well
    const auto rawRes =
            QtOrderingPrivate::lexicographicalCompareThreeWay(lhs.data(), lhs.data() + lhs.size(),
                                                              rhs.data(), rhs.data() + rhs.size());
    QCOMPARE_EQ(rawRes, expectedResult);
}

template <typename LT, typename RT = LT>
Qt::strong_ordering lessOnlyCmp(LT const &lhs, RT const &rhs)
{
    if (std::less<>{}(lhs, rhs))
        return Qt::strong_ordering::less;
    if (std::less<>{}(rhs, lhs))
        return Qt::strong_ordering::greater;
    return Qt::strong_ordering::equivalent;
}

template <typename LeftType, typename RightType, typename OrderingType>
void tst_QCompareHelpers::lexicographicalCompareThreeWayComparatorImpl()
{
    QFETCH(const QList<LeftType>, lhs);
    QFETCH(const QList<RightType>, rhs);
    QFETCH(const OrderingType, expectedResult);

    // free function
    {
        const auto res =
            QtOrderingPrivate::lexicographicalCompareThreeWay(lhs.begin(), lhs.end(),
                                                              rhs.begin(), rhs.end(),
                                                              lessOnlyCmp<LeftType, RightType>);
        QCOMPARE_EQ(res, expectedResult);

        // check the C-style arrays as well
        const auto rawRes =
            QtOrderingPrivate::lexicographicalCompareThreeWay(lhs.data(), lhs.data() + lhs.size(),
                                                              rhs.data(), rhs.data() + rhs.size(),
                                                              lessOnlyCmp<LeftType, RightType>);
        QCOMPARE_EQ(rawRes, expectedResult);
    }
    // lambda
    {
        auto cmp = [](LeftType const &lhs, RightType const &rhs) {
            return lessOnlyCmp(lhs, rhs);
        };
        const auto res =
            QtOrderingPrivate::lexicographicalCompareThreeWay(lhs.begin(), lhs.end(),
                                                              rhs.begin(), rhs.end(),
                                                              cmp);
        QCOMPARE_EQ(res, expectedResult);

        // check the C-style arrays as well
        const auto rawRes =
            QtOrderingPrivate::lexicographicalCompareThreeWay(lhs.data(), lhs.data() + lhs.size(),
                                                              rhs.data(), rhs.data() + rhs.size(),
                                                              std::move(cmp));
        QCOMPARE_EQ(rawRes, expectedResult);
    }
}

void tst_QCompareHelpers::lexicographicalCompareThreeWay_ThreeWayCmp_Int_data()
{
    lexicographicalCompareThreeWayDataImpl<ThreeWayCmp<int>, ThreeWayCmp<int>,
                                           Qt::strong_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_ThreeWayCmp_Int()
{
    lexicographicalCompareThreeWayImpl<ThreeWayCmp<int>, ThreeWayCmp<int>,
                                       Qt::strong_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_ThreeWayCmp_Float_data()
{
    lexicographicalCompareThreeWayDataImpl<ThreeWayCmp<float>, ThreeWayCmp<float>,
                                           Qt::partial_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_ThreeWayCmp_Float()
{
    lexicographicalCompareThreeWayImpl<ThreeWayCmp<float>, ThreeWayCmp<float>,
                                       Qt::partial_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_ThreeWayCmp_Weak_data()
{
    lexicographicalCompareThreeWayDataImpl<ThreeWayCmp<Weak>, ThreeWayCmp<Weak>,
                                           Qt::weak_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_ThreeWayCmp_Weak()
{
    lexicographicalCompareThreeWayImpl<ThreeWayCmp<Weak>, ThreeWayCmp<Weak>,
                                       Qt::weak_ordering>();
}

void tst_QCompareHelpers::lexicographicalCompareThreeWay_Mixed_Int_data()
{
    lexicographicalCompareThreeWayDataImpl<LessOnly<int>, ThreeWayCmp<int>,
                                           Qt::strong_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_Mixed_Int()
{
    lexicographicalCompareThreeWayImpl<LessOnly<int>, ThreeWayCmp<int>,
                                       Qt::strong_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_Mixed_Float_data()
{
    lexicographicalCompareThreeWayDataImpl<LessOnly<float>, ThreeWayCmp<float>,
                                           Qt::partial_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_Mixed_Float()
{
    lexicographicalCompareThreeWayImpl<LessOnly<float>, ThreeWayCmp<float>,
                                       Qt::partial_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_Mixed_Weak_data()
{
    lexicographicalCompareThreeWayDataImpl<LessOnly<Weak>, ThreeWayCmp<Weak>,
                                           Qt::weak_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_Mixed_Weak()
{
    lexicographicalCompareThreeWayImpl<LessOnly<Weak>, ThreeWayCmp<Weak>,
                                       Qt::weak_ordering>();
}

void tst_QCompareHelpers::lexicographicalCompareThreeWay_Comparator_Int_data()
{
    lexicographicalCompareThreeWayDataImpl<LessOnly<int>, LessOnly<int>, Qt::strong_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_Comparator_Int()
{
    lexicographicalCompareThreeWayComparatorImpl<LessOnly<int>, LessOnly<int>,
                                                 Qt::strong_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_Comparator_Float_data()
{
    lexicographicalCompareThreeWayDataImpl<LessOnly<float>, LessOnly<float>,
                                           Qt::partial_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_Comparator_Float()
{
    lexicographicalCompareThreeWayComparatorImpl<LessOnly<float>, LessOnly<float>,
                                                 Qt::partial_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_Comparator_Weak_data()
{
    lexicographicalCompareThreeWayDataImpl<LessOnly<Weak>, LessOnly<Weak>, Qt::weak_ordering>();
}
void tst_QCompareHelpers::lexicographicalCompareThreeWay_Comparator_Weak()
{
    lexicographicalCompareThreeWayComparatorImpl<LessOnly<Weak>, LessOnly<Weak>,
                                                 Qt::weak_ordering>();
}

void tst_QCompareHelpers::lexicographicalCompareThreeWay_Pointers()
{
    constexpr std::array<int, 3> a = {42, 0, 17}; // the actual values do not matter

    {
        const QList<const int*> lhs{&a[0], &a[1], &a[2]};
        const QList<const int*> rhs{&a[0], &a[1], &a[2]};
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(lhs.begin(), lhs.end(),
                                                                           rhs.begin(), rhs.end());
        QCOMPARE_EQ(res, Qt::strong_ordering::equivalent);
    }
    {
        const QList<const int*> lhs{&a[0], &a[1], &a[2]};
        const QList<const int*> rhs{&a[0], &a[2]};
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(lhs.begin(), lhs.end(),
                                                                           rhs.begin(), rhs.end());
        QCOMPARE_EQ(res, Qt::strong_ordering::less);
    }
    {
        const QList<const int*> lhs{&a[0], &a[2], &a[1]};
        const QList<const int*> rhs{&a[0], &a[1], &a[2]};
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(lhs.begin(), lhs.end(),
                                                                           rhs.begin(), rhs.end());
        QCOMPARE_EQ(res, Qt::strong_ordering::greater);
    }
    {
        const QList<const int*> lhs{&a[0], &a[1], &a[2], &a[0]};
        const QList<const int*> rhs{&a[0], &a[1], &a[2]};
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(lhs.begin(), lhs.end(),
                                                                           rhs.begin(), rhs.end());
        QCOMPARE_EQ(res, Qt::strong_ordering::greater);
    }
}

struct NonCopyMove
{
    float val;

    constexpr NonCopyMove(float f) : val(f) {}
    Q_DISABLE_COPY_MOVE(NonCopyMove)

private:
    friend auto compareThreeWay(const NonCopyMove &lhs, const NonCopyMove &rhs)
    {
        return Qt::compareThreeWay(lhs.val, rhs.val);
    }
};

void tst_QCompareHelpers::lexicographicalCompareThreeWay_NonCopyMove()
{
    constexpr std::array a1 = {NonCopyMove{1.f}, NonCopyMove{2.f}, NonCopyMove{3.f}};
    constexpr std::array a2 = {NonCopyMove{1.f}, NonCopyMove{3.f}, NonCopyMove{2.f}};
    constexpr std::array a3 = {NonCopyMove{std::numeric_limits<float>::quiet_NaN()},
                               NonCopyMove{2.f}, NonCopyMove{3.f}};

    {
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(a1.begin(), a1.end(),
                                                                           a1.begin(), a1.end());
        QCOMPARE_EQ(res, Qt::partial_ordering::equivalent);
    }
    {
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(a1.begin(), a1.end(),
                                                                           a2.begin(), a2.end());
        QCOMPARE_EQ(res, Qt::partial_ordering::less);
    }
    {
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(a2.begin(), a2.end(),
                                                                           a1.begin(), a1.end());
        QCOMPARE_EQ(res, Qt::partial_ordering::greater);
    }
    {
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(a1.begin(), a1.end(),
                                                                           a3.begin(), a3.end());
        QCOMPARE_EQ(res, Qt::partial_ordering::unordered);
    }
}

struct WithCustomPointerCompareThreeWayHelper
{
    float val;

private:
    friend auto compareThreeWay(const WithCustomPointerCompareThreeWayHelper *lhs,
                                const WithCustomPointerCompareThreeWayHelper *rhs)
    {
        if (lhs && rhs)
            return Qt::compareThreeWay(lhs->val, rhs->val);
        else if (lhs)
            return Qt::partial_ordering::greater;
        else
            return Qt::partial_ordering::less;
    }
};

static_assert(QtOrderingPrivate::HasCustomCompareThreeWay<WithCustomPointerCompareThreeWayHelper*>::value);
static_assert(!QtOrderingPrivate::HasCustomCompareThreeWay<int*>::value);

void tst_QCompareHelpers::lexicographicalCompareThreeWay_CustomPointerHelper()
{
    const WithCustomPointerCompareThreeWayHelper val1{1.f};
    const WithCustomPointerCompareThreeWayHelper val2{2.f};
    const WithCustomPointerCompareThreeWayHelper val3{3.f};
    const WithCustomPointerCompareThreeWayHelper nan{std::numeric_limits<float>::quiet_NaN()};

    const std::array a1 = {&val1, &val2, &val3};
    const std::array a2 = {&val1, &val3, &val2};
    const std::array a3 = {&nan, &val2, &val3};

    {
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(a1.begin(), a1.end(),
                                                                           a1.begin(), a1.end());
        QCOMPARE_EQ(res, Qt::partial_ordering::equivalent);
    }
    {
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(a1.begin(), a1.end(),
                                                                           a2.begin(), a2.end());
        QCOMPARE_EQ(res, Qt::partial_ordering::less);
    }
    {
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(a2.begin(), a2.end(),
                                                                           a1.begin(), a1.end());
        QCOMPARE_EQ(res, Qt::partial_ordering::greater);
    }
    {
        const auto res = QtOrderingPrivate::lexicographicalCompareThreeWay(a1.begin(), a1.end(),
                                                                           a3.begin(), a3.end());
        QCOMPARE_EQ(res, Qt::partial_ordering::unordered);
    }
}
