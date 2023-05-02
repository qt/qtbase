// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qcompare.h>
#include <QtTest/qtest.h>
#include <QtTest/private/qcomparisontesthelper_p.h>

class IntWrapper
{
public:
    // implicit constructor and operator int() to simulate the case that
    // triggers a bug on MSVC < 19.36.
    IntWrapper(int val) : m_val(val) {}
    operator int() const noexcept { return m_val; }

    int value() const { return m_val; }

private:
    friend bool comparesEqual(const IntWrapper &lhs, const IntWrapper &rhs) noexcept
    { return lhs.m_val == rhs.m_val; }
    friend Qt::strong_ordering
    compareThreeWay(const IntWrapper &lhs, const IntWrapper &rhs) noexcept
    {
        // ### Qt::compareThreeWay
        if (lhs.m_val < rhs.m_val)
            return Qt::strong_ordering::less;
        else if (lhs.m_val > rhs.m_val)
            return Qt::strong_ordering::greater;
        else
            return Qt::strong_ordering::equal;
    }
    friend bool comparesEqual(const IntWrapper &lhs, int rhs) noexcept
    { return lhs.m_val == rhs; }
    friend Qt::strong_ordering compareThreeWay(const IntWrapper &lhs, int rhs) noexcept
    { return compareThreeWay(lhs, IntWrapper(rhs)); }

    Q_DECLARE_STRONGLY_ORDERED(IntWrapper)
    Q_DECLARE_STRONGLY_ORDERED(IntWrapper, int)

    int m_val = 0;
};

class DoubleWrapper
{
public:
    explicit DoubleWrapper(double val) : m_val(val) {}
    double value() const { return m_val; }

private:
    friend bool comparesEqual(const DoubleWrapper &lhs, const DoubleWrapper &rhs) noexcept
    { return lhs.m_val == rhs.m_val; }
    friend Qt::partial_ordering
    compareThreeWay(const DoubleWrapper &lhs, const DoubleWrapper &rhs) noexcept
    {
        // ### Qt::compareThreeWay
        if (qIsNaN(lhs.m_val) || qIsNaN(rhs.m_val))
            return Qt::partial_ordering::unordered;

        if (lhs.m_val < rhs.m_val)
            return Qt::partial_ordering::less;
        else if (lhs.m_val > rhs.m_val)
            return Qt::partial_ordering::greater;
        else
            return Qt::partial_ordering::equivalent;
    }
    friend bool comparesEqual(const DoubleWrapper &lhs, const IntWrapper &rhs) noexcept
    { return comparesEqual(lhs, DoubleWrapper(rhs.value())); }
    friend Qt::partial_ordering
    compareThreeWay(const DoubleWrapper &lhs, const IntWrapper &rhs) noexcept
    { return compareThreeWay(lhs, DoubleWrapper(rhs.value())); }
    friend bool comparesEqual(const DoubleWrapper &lhs, double rhs) noexcept
    { return lhs.m_val == rhs; }
    friend Qt::partial_ordering compareThreeWay(const DoubleWrapper &lhs, double rhs) noexcept
    {
        // ### Qt::compareThreeWay
        if (qIsNaN(lhs.m_val) || qIsNaN(rhs))
            return Qt::partial_ordering::unordered;

        if (lhs.m_val < rhs)
            return Qt::partial_ordering::less;
        else if (lhs.m_val > rhs)
            return Qt::partial_ordering::greater;
        else
            return Qt::partial_ordering::equivalent;
    }

    Q_DECLARE_PARTIALLY_ORDERED(DoubleWrapper)
    Q_DECLARE_PARTIALLY_ORDERED(DoubleWrapper, IntWrapper)
    Q_DECLARE_PARTIALLY_ORDERED(DoubleWrapper, double)

    double m_val = 0.0;
};

template <typename String>
class StringWrapper
{
public:
    explicit StringWrapper(String val) : m_val(val) {}
    String value() const { return m_val; }

private:
    // Some of the helper functions are intentionally NOT marked as noexcept
    // to test the conditional noexcept in the macros.
    template <typename T>
    friend bool comparesEqual(const StringWrapper<T> &, const StringWrapper<T> &) noexcept;
    template <typename T>
    friend Qt::weak_ordering
    compareThreeWay(const StringWrapper<T> &, const StringWrapper<T> &) noexcept;
    template <typename T>
    friend bool comparesEqual(const StringWrapper<T> &, QAnyStringView);
    template <typename T>
    friend Qt::weak_ordering compareThreeWay(const StringWrapper<T> &, QAnyStringView);

    Q_DECLARE_WEAKLY_ORDERED(StringWrapper)
    Q_DECLARE_WEAKLY_ORDERED(StringWrapper, QAnyStringView)

    String m_val;
};

// StringWrapper comparison helper functions

bool equalsHelper(QAnyStringView lhs, QAnyStringView rhs) noexcept
{
    return QAnyStringView::compare(lhs, rhs, Qt::CaseInsensitive) == 0;
}

template <typename T>
bool comparesEqual(const StringWrapper<T> &lhs, const StringWrapper<T> &rhs) noexcept
{
    return equalsHelper(lhs.m_val, rhs.m_val);
}

Qt::weak_ordering compareHelper(QAnyStringView lhs, QAnyStringView rhs) noexcept
{
    const int res = QAnyStringView::compare(lhs, rhs, Qt::CaseInsensitive);
    if (res < 0)
        return Qt::weak_ordering::less;
    else if (res > 0)
        return Qt::weak_ordering::greater;
    else
        return Qt::weak_ordering::equivalent;
}

template <typename T>
Qt::weak_ordering compareThreeWay(const StringWrapper<T> &lhs, const StringWrapper<T> &rhs) noexcept
{
    return compareHelper(lhs.m_val, rhs.m_val);
}

template <typename T>
bool comparesEqual(const StringWrapper<T> &lhs, QAnyStringView rhs)
{
    return equalsHelper(lhs.m_val, rhs);
}

template <typename T>
Qt::weak_ordering compareThreeWay(const StringWrapper<T> &lhs, QAnyStringView rhs)
{
    return compareHelper(lhs.m_val, rhs);
}

// Test class

class tst_QCompareHelpers : public QObject
{
    Q_OBJECT

private:
    template <typename LeftType, typename RightType, typename OrderingType>
    void compareImpl();

    template <typename LeftType, typename RightType>
    void compareIntData();

    template <typename LeftType, typename RightType>
    void compareFloatData();

    template <typename LeftType, typename RightType>
    void compareStringData();

private slots:
    void comparisonCompiles();

    void compare_IntWrapper_data() { compareIntData<IntWrapper, IntWrapper>(); }
    void compare_IntWrapper() { compareImpl<IntWrapper, IntWrapper, Qt::strong_ordering>(); }

    void compare_IntWrapper_int_data() { compareIntData<IntWrapper, int>(); }
    void compare_IntWrapper_int() { compareImpl<IntWrapper, int, Qt::strong_ordering>(); }

    void compare_DoubleWrapper_data() { compareFloatData<DoubleWrapper, DoubleWrapper>(); }
    void compare_DoubleWrapper()
    { compareImpl<DoubleWrapper, DoubleWrapper, Qt::partial_ordering>(); }

    void compare_DoubleWrapper_double_data() { compareFloatData<DoubleWrapper, double>(); }
    void compare_DoubleWrapper_double()
    { compareImpl<DoubleWrapper, double, Qt::partial_ordering>(); }

    void compare_IntWrapper_DoubleWrapper_data();
    void compare_IntWrapper_DoubleWrapper()
    { compareImpl<IntWrapper, DoubleWrapper, Qt::partial_ordering>(); }

    void compare_StringWrapper_data()
    { compareStringData<StringWrapper<QString>, StringWrapper<QString>>(); }
    void compare_StringWrapper()
    { compareImpl<StringWrapper<QString>, StringWrapper<QString>, Qt::weak_ordering>(); }

    void compare_StringWrapper_AnyStringView_data()
    { compareStringData<StringWrapper<QString>, QAnyStringView>(); }
    void compare_StringWrapper_AnyStringView()
    { compareImpl<StringWrapper<QString>, QAnyStringView, Qt::weak_ordering>(); }

    void generatedClasses();
};

template<typename LeftType, typename RightType, typename OrderingType>
void tst_QCompareHelpers::compareImpl()
{
    QFETCH(LeftType, lhs);
    QFETCH(RightType, rhs);
    QFETCH(OrderingType, expectedOrdering);

    QTestPrivate::testAllComparisonOperators(lhs, rhs, expectedOrdering);
    if (QTest::currentTestFailed())
        return;
#ifdef __cpp_lib_three_way_comparison
    // Also check std types.

    // if Ordering == Qt::strong_ordering -> std::strong_ordering
    // else if Ordering == Qt::weak_ordering -> std::weak_ordering
    // else std::partial_ordering
    using StdType = std::conditional_t<
                        std::is_same_v<OrderingType, Qt::strong_ordering>,
                            std::strong_ordering,
                            std::conditional_t<std::is_same_v<OrderingType, Qt::weak_ordering>,
                                std::weak_ordering,
                                std::partial_ordering>>;

    QTestPrivate::testAllComparisonOperators(lhs, rhs, static_cast<StdType>(expectedOrdering));
    if (QTest::currentTestFailed())
        return;
#endif // __cpp_lib_three_way_comparison
}

template<typename LeftType, typename RightType>
void tst_QCompareHelpers::compareIntData()
{
    QTest::addColumn<LeftType>("lhs");
    QTest::addColumn<RightType>("rhs");
    QTest::addColumn<Qt::strong_ordering>("expectedOrdering");

    auto createRow = [](auto lhs, auto rhs, Qt::strong_ordering ordering) {
        QTest::addRow("%d vs %d", lhs, rhs) << LeftType(lhs) << RightType(rhs) << ordering;
    };

    createRow(0, 0, Qt::strong_ordering::equivalent);
    createRow(-1, 0, Qt::strong_ordering::less);
    createRow(1, 0, Qt::strong_ordering::greater);
    constexpr int max = std::numeric_limits<int>::max();
    constexpr int min = std::numeric_limits<int>::min();
    createRow(max, max, Qt::strong_ordering::equivalent);
    createRow(min, min, Qt::strong_ordering::equivalent);
    createRow(max, min, Qt::strong_ordering::greater);
    createRow(min, max, Qt::strong_ordering::less);
}

template<typename LeftType, typename RightType>
void tst_QCompareHelpers::compareFloatData()
{
    QTest::addColumn<LeftType>("lhs");
    QTest::addColumn<RightType>("rhs");
    QTest::addColumn<Qt::partial_ordering>("expectedOrdering");

    auto createRow = [](auto lhs, auto rhs, Qt::partial_ordering ordering) {
        QTest::addRow("%f vs %f", lhs, rhs) << LeftType(lhs) << RightType(rhs) << ordering;
    };

    createRow(0.0, 0.0, Qt::partial_ordering::equivalent);
    createRow(-0.000001, 0.0, Qt::partial_ordering::less);
    createRow(0.000001, 0.0, Qt::partial_ordering::greater);

    const double nan = qQNaN();
    createRow(nan, 0.0, Qt::partial_ordering::unordered);
    createRow(0.0, nan, Qt::partial_ordering::unordered);
    createRow(nan, nan, Qt::partial_ordering::unordered);

    const double inf = qInf();
    createRow(inf, 0.0, Qt::partial_ordering::greater);
    createRow(0.0, inf, Qt::partial_ordering::less);
    createRow(-inf, 0.0, Qt::partial_ordering::less);
    createRow(0.0, -inf, Qt::partial_ordering::greater);
    createRow(inf, inf, Qt::partial_ordering::equivalent);
    createRow(-inf, -inf, Qt::partial_ordering::equivalent);
    createRow(-inf, inf, Qt::partial_ordering::less);
    createRow(inf, -inf, Qt::partial_ordering::greater);

    createRow(nan, inf, Qt::partial_ordering::unordered);
    createRow(inf, nan, Qt::partial_ordering::unordered);
    createRow(nan, -inf, Qt::partial_ordering::unordered);
    createRow(-inf, nan, Qt::partial_ordering::unordered);
}

template<typename LeftType, typename RightType>
void tst_QCompareHelpers::compareStringData()
{
    QTest::addColumn<LeftType>("lhs");
    QTest::addColumn<RightType>("rhs");
    QTest::addColumn<Qt::weak_ordering>("expectedOrdering");

    auto createRow = [](auto lhs, auto rhs, Qt::weak_ordering ordering) {
        QTest::addRow("'%s' vs '%s'", lhs, rhs) << LeftType(lhs) << RightType(rhs) << ordering;
    };

    createRow("", "", Qt::weak_ordering::equivalent);
    createRow("Ab", "abc", Qt::weak_ordering::less);
    createRow("aBc", "AB", Qt::weak_ordering::greater);
    createRow("ab", "AB", Qt::weak_ordering::equivalent);
    createRow("ABC", "abc", Qt::weak_ordering::equivalent);
}

void tst_QCompareHelpers::comparisonCompiles()
{
    QTestPrivate::testAllComparisonOperatorsCompile<IntWrapper>();
    if (QTest::currentTestFailed())
        return;

    QTestPrivate::testAllComparisonOperatorsCompile<IntWrapper, int>();
    if (QTest::currentTestFailed())
        return;

    QTestPrivate::testAllComparisonOperatorsCompile<DoubleWrapper>();
    if (QTest::currentTestFailed())
        return;

    QTestPrivate::testAllComparisonOperatorsCompile<DoubleWrapper, double>();
    if (QTest::currentTestFailed())
        return;

    QTestPrivate::testAllComparisonOperatorsCompile<DoubleWrapper, IntWrapper>();
    if (QTest::currentTestFailed())
        return;

    QTestPrivate::testAllComparisonOperatorsCompile<StringWrapper<QString>>();
    if (QTest::currentTestFailed())
        return;

    QTestPrivate::testAllComparisonOperatorsCompile<StringWrapper<QString>, QAnyStringView>();
    if (QTest::currentTestFailed())
        return;
}

void tst_QCompareHelpers::compare_IntWrapper_DoubleWrapper_data()
{
    QTest::addColumn<IntWrapper>("lhs");
    QTest::addColumn<DoubleWrapper>("rhs");
    QTest::addColumn<Qt::partial_ordering>("expectedOrdering");

    auto createRow = [](auto lhs, auto rhs, Qt::partial_ordering ordering) {
        QTest::addRow("%d vs %f", lhs, rhs) << IntWrapper(lhs) << DoubleWrapper(rhs) << ordering;
    };

    createRow(0, 0.0, Qt::partial_ordering::equivalent);
    createRow(-1, 0.0, Qt::partial_ordering::less);
    createRow(1, 0.0, Qt::partial_ordering::greater);
    createRow(0, -0.000001, Qt::partial_ordering::greater);
    createRow(0, 0.000001, Qt::partial_ordering::less);

    constexpr int max = std::numeric_limits<int>::max();
    constexpr int min = std::numeric_limits<int>::min();
    const double nan = qQNaN();
    createRow(0, nan, Qt::partial_ordering::unordered);
    createRow(max, nan, Qt::partial_ordering::unordered);
    createRow(min, nan, Qt::partial_ordering::unordered);

    const double inf = qInf();
    createRow(0, inf, Qt::partial_ordering::less);
    createRow(0, -inf, Qt::partial_ordering::greater);
    createRow(max, inf, Qt::partial_ordering::less);
    createRow(max, -inf, Qt::partial_ordering::greater);
    createRow(min, inf, Qt::partial_ordering::less);
    createRow(min, -inf, Qt::partial_ordering::greater);
}

#define DECLARE_TYPE(Name, Type, Attrs, RetType, Constexpr, Suffix) \
class Dummy ## Name \
{ \
public: \
    Constexpr Dummy ## Name () {} \
\
private: \
    friend Attrs Constexpr bool \
    comparesEqual(const Dummy ## Name &lhs, const Dummy ## Name &rhs) noexcept; \
    friend Attrs Constexpr RetType \
    compareThreeWay(const Dummy ## Name &lhs, const Dummy ## Name &rhs) noexcept; \
    friend Attrs Constexpr bool \
    comparesEqual(const Dummy ## Name &lhs, int rhs) noexcept; \
    friend Attrs Constexpr RetType \
    compareThreeWay(const Dummy ## Name &lhs, int rhs) noexcept; \
    Q_DECLARE_ ## Type ##_ORDERED ## Suffix (Dummy ## Name) \
    Q_DECLARE_ ## Type ##_ORDERED ## Suffix (Dummy ## Name, int) \
}; \
\
Attrs Constexpr bool comparesEqual(const Dummy ## Name &lhs, const Dummy ## Name &rhs) noexcept \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return true; } \
Attrs Constexpr RetType \
compareThreeWay(const Dummy ## Name &lhs, const Dummy ## Name &rhs) noexcept \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return RetType::equivalent; } \
Attrs Constexpr bool comparesEqual(const Dummy ## Name &lhs, int rhs) noexcept \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return true; } \
Attrs Constexpr RetType compareThreeWay(const Dummy ## Name &lhs, int rhs) noexcept \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return RetType::equivalent; }

DECLARE_TYPE(PartialConstAttr, PARTIALLY, Q_DECL_PURE_FUNCTION, Qt::partial_ordering, constexpr,
             _LITERAL_TYPE)
DECLARE_TYPE(PartialConst, PARTIALLY, /* no attrs */, Qt::partial_ordering, constexpr, _LITERAL_TYPE)
DECLARE_TYPE(PartialAttr, PARTIALLY, Q_DECL_CONST_FUNCTION, Qt::partial_ordering, , )
DECLARE_TYPE(Partial, PARTIALLY, /* no attrs */, Qt::partial_ordering, , )

DECLARE_TYPE(WeakConstAttr, WEAKLY, Q_DECL_PURE_FUNCTION, Qt::weak_ordering, constexpr, _LITERAL_TYPE)
DECLARE_TYPE(WeakConst, WEAKLY, /* no attrs */, Qt::weak_ordering, constexpr, _LITERAL_TYPE)
DECLARE_TYPE(WeakAttr, WEAKLY, Q_DECL_CONST_FUNCTION, Qt::weak_ordering, , )
DECLARE_TYPE(Weak, WEAKLY, /* no attrs */, Qt::weak_ordering, , )

DECLARE_TYPE(StrongConstAttr, STRONGLY, Q_DECL_PURE_FUNCTION, Qt::strong_ordering, constexpr,
             _LITERAL_TYPE)
DECLARE_TYPE(StrongConst, STRONGLY, /* no attrs */, Qt::strong_ordering, constexpr, _LITERAL_TYPE)
DECLARE_TYPE(StrongAttr, STRONGLY, Q_DECL_CONST_FUNCTION, Qt::strong_ordering, , )
DECLARE_TYPE(Strong, STRONGLY, /* no attrs */, Qt::strong_ordering, , )

#define DECLARE_EQUALITY_COMPARABLE(Name, Attrs, Constexpr, Suffix) \
class Dummy ## Name \
{ \
public: \
    Constexpr Dummy ## Name (int) {} \
\
private: \
    friend Attrs Constexpr bool \
    comparesEqual(const Dummy ## Name &lhs, const Dummy ## Name &rhs) noexcept; \
    friend Attrs Constexpr bool comparesEqual(const Dummy ## Name &lhs, int rhs) noexcept; \
    Q_DECLARE_EQUALITY_COMPARABLE ## Suffix (Dummy ## Name) \
    Q_DECLARE_EQUALITY_COMPARABLE ## Suffix (Dummy ## Name, int) \
}; \
\
Attrs Constexpr bool comparesEqual(const Dummy ## Name &lhs, const Dummy ## Name &rhs) noexcept \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return true; } \
Attrs Constexpr bool comparesEqual(const Dummy ## Name &lhs, int rhs) noexcept \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return true; } \

DECLARE_EQUALITY_COMPARABLE(ConstAttr, Q_DECL_PURE_FUNCTION, constexpr, _LITERAL_TYPE)
DECLARE_EQUALITY_COMPARABLE(Const, /* no attrs */, constexpr, _LITERAL_TYPE)
DECLARE_EQUALITY_COMPARABLE(Attr, Q_DECL_CONST_FUNCTION, , )
DECLARE_EQUALITY_COMPARABLE(None, /* no attrs */, , )

void tst_QCompareHelpers::generatedClasses()
{
#define COMPARE(ClassName) \
    do { \
        QTestPrivate::testAllComparisonOperatorsCompile<ClassName>(); \
        QTestPrivate::testAllComparisonOperatorsCompile<ClassName, int>(); \
    } while (0)

    COMPARE(DummyPartialConstAttr);
    COMPARE(DummyPartialConst);
    COMPARE(DummyPartialAttr);
    COMPARE(DummyPartial);

    COMPARE(DummyWeakConstAttr);
    COMPARE(DummyWeakConst);
    COMPARE(DummyWeakAttr);
    COMPARE(DummyWeak);

    COMPARE(DummyStrongConstAttr);
    COMPARE(DummyStrongConst);
    COMPARE(DummyStrongAttr);
    COMPARE(DummyStrong);
#undef COMPARE

    QTestPrivate::testEqualityOperatorsCompile<DummyConstAttr>();
    QTestPrivate::testEqualityOperatorsCompile<DummyConstAttr, int>();

    QTestPrivate::testEqualityOperatorsCompile<DummyConst>();
    QTestPrivate::testEqualityOperatorsCompile<DummyConst, int>();

    QTestPrivate::testEqualityOperatorsCompile<DummyAttr>();
    QTestPrivate::testEqualityOperatorsCompile<DummyAttr, int>();

    QTestPrivate::testEqualityOperatorsCompile<DummyNone>();
    QTestPrivate::testEqualityOperatorsCompile<DummyNone, int>();
}

QTEST_MAIN(tst_QCompareHelpers)
#include "tst_qcomparehelpers.moc"
