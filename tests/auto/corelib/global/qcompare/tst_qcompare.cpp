// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QtCompare>
#include <QtTest/QTest>

#ifdef __cpp_lib_three_way_comparison
#include <compare>
#endif // __cpp_lib_three_way_comparison

class tst_QCompare: public QObject
{
    Q_OBJECT

    enum TestEnum : int {
        Smaller,
        Bigger
    };

#ifdef __cpp_lib_three_way_comparison
    template<typename LeftType, typename RightType, typename OrderingType>
    void compare3WayHelper(LeftType lhs, RightType rhs, OrderingType order);
#endif

private slots:
    void legacyPartialOrdering();
    void legacyConversions();
    void stdQtBinaryCompatibility();
    void partialOrdering();
    void weakOrdering();
    void strongOrdering();
    void threeWayCompareWithLiteralZero();
    void conversions();
    void is_eq_overloads();
    void compareThreeWay();
    void unorderedNeqLiteralZero();
    void compare3WayMacro();
};

void tst_QCompare::legacyPartialOrdering()
{
    static_assert(QPartialOrdering::Unordered ==  QPartialOrdering::unordered);
    static_assert(QPartialOrdering::Less ==       QPartialOrdering::less);
    static_assert(QPartialOrdering::Equivalent == QPartialOrdering::equivalent);
    static_assert(QPartialOrdering::Greater ==    QPartialOrdering::greater);

    static_assert(QPartialOrdering::Unordered == QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Unordered != QPartialOrdering::Less);
    static_assert(QPartialOrdering::Unordered != QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Unordered != QPartialOrdering::Greater);

    static_assert(QPartialOrdering::Less != QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Less == QPartialOrdering::Less);
    static_assert(QPartialOrdering::Less != QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Less != QPartialOrdering::Greater);

    static_assert(QPartialOrdering::Equivalent != QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Equivalent != QPartialOrdering::Less);
    static_assert(QPartialOrdering::Equivalent == QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Equivalent != QPartialOrdering::Greater);

    static_assert(QPartialOrdering::Greater != QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Greater != QPartialOrdering::Less);
    static_assert(QPartialOrdering::Greater != QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Greater == QPartialOrdering::Greater);

    static_assert(!is_eq  (QPartialOrdering::Unordered));
    static_assert( is_neq (QPartialOrdering::Unordered));
    static_assert(!is_lt  (QPartialOrdering::Unordered));
    static_assert(!is_lteq(QPartialOrdering::Unordered));
    static_assert(!is_gt  (QPartialOrdering::Unordered));
    static_assert(!is_gteq(QPartialOrdering::Unordered));

    static_assert(!(QPartialOrdering::Unordered == 0));
    static_assert( (QPartialOrdering::Unordered != 0));
    static_assert(!(QPartialOrdering::Unordered <  0));
    static_assert(!(QPartialOrdering::Unordered <= 0));
    static_assert(!(QPartialOrdering::Unordered >  0));
    static_assert(!(QPartialOrdering::Unordered >= 0));

    static_assert(!(0 == QPartialOrdering::Unordered));
    static_assert( (0 != QPartialOrdering::Unordered));
    static_assert(!(0 <  QPartialOrdering::Unordered));
    static_assert(!(0 <= QPartialOrdering::Unordered));
    static_assert(!(0 >  QPartialOrdering::Unordered));
    static_assert(!(0 >= QPartialOrdering::Unordered));


    static_assert(!is_eq  (QPartialOrdering::Less));
    static_assert( is_neq (QPartialOrdering::Less));
    static_assert( is_lt  (QPartialOrdering::Less));
    static_assert( is_lteq(QPartialOrdering::Less));
    static_assert(!is_gt  (QPartialOrdering::Less));
    static_assert(!is_gteq(QPartialOrdering::Less));

    static_assert(!(QPartialOrdering::Less == 0));
    static_assert( (QPartialOrdering::Less != 0));
    static_assert( (QPartialOrdering::Less <  0));
    static_assert( (QPartialOrdering::Less <= 0));
    static_assert(!(QPartialOrdering::Less >  0));
    static_assert(!(QPartialOrdering::Less >= 0));

    static_assert(!(0 == QPartialOrdering::Less));
    static_assert( (0 != QPartialOrdering::Less));
    static_assert(!(0 <  QPartialOrdering::Less));
    static_assert(!(0 <= QPartialOrdering::Less));
    static_assert( (0 >  QPartialOrdering::Less));
    static_assert( (0 >= QPartialOrdering::Less));


    static_assert( is_eq  (QPartialOrdering::Equivalent));
    static_assert(!is_neq (QPartialOrdering::Equivalent));
    static_assert(!is_lt  (QPartialOrdering::Equivalent));
    static_assert( is_lteq(QPartialOrdering::Equivalent));
    static_assert(!is_gt  (QPartialOrdering::Equivalent));
    static_assert( is_gteq(QPartialOrdering::Equivalent));

    static_assert( (QPartialOrdering::Equivalent == 0));
    static_assert(!(QPartialOrdering::Equivalent != 0));
    static_assert(!(QPartialOrdering::Equivalent <  0));
    static_assert( (QPartialOrdering::Equivalent <= 0));
    static_assert(!(QPartialOrdering::Equivalent >  0));
    static_assert( (QPartialOrdering::Equivalent >= 0));

    static_assert( (0 == QPartialOrdering::Equivalent));
    static_assert(!(0 != QPartialOrdering::Equivalent));
    static_assert(!(0 <  QPartialOrdering::Equivalent));
    static_assert( (0 <= QPartialOrdering::Equivalent));
    static_assert(!(0 >  QPartialOrdering::Equivalent));
    static_assert( (0 >= QPartialOrdering::Equivalent));


    static_assert(!is_eq  (QPartialOrdering::Greater));
    static_assert( is_neq (QPartialOrdering::Greater));
    static_assert(!is_lt  (QPartialOrdering::Greater));
    static_assert(!is_lteq(QPartialOrdering::Greater));
    static_assert( is_gt  (QPartialOrdering::Greater));
    static_assert( is_gteq(QPartialOrdering::Greater));

    static_assert(!(QPartialOrdering::Greater == 0));
    static_assert( (QPartialOrdering::Greater != 0));
    static_assert(!(QPartialOrdering::Greater <  0));
    static_assert(!(QPartialOrdering::Greater <= 0));
    static_assert( (QPartialOrdering::Greater >  0));
    static_assert( (QPartialOrdering::Greater >= 0));

    static_assert(!(0 == QPartialOrdering::Greater));
    static_assert( (0 != QPartialOrdering::Greater));
    static_assert( (0 <  QPartialOrdering::Greater));
    static_assert( (0 <= QPartialOrdering::Greater));
    static_assert(!(0 >  QPartialOrdering::Greater));
    static_assert(!(0 >= QPartialOrdering::Greater));
}

void tst_QCompare::legacyConversions()
{
#define CHECK_CONVERTS(Lhs, Rhs) static_assert(std::is_convertible_v<Lhs, Rhs>)
#define CHECK_ALL(NS) do { \
        CHECK_CONVERTS(QPartialOrdering, NS ::partial_ordering); \
        static_assert(QPartialOrdering::Less == NS ::partial_ordering::less); \
        static_assert(QPartialOrdering::Greater == NS ::partial_ordering::greater); \
        static_assert(QPartialOrdering::Equivalent == NS ::partial_ordering::equivalent); \
        static_assert(QPartialOrdering::Unordered == NS ::partial_ordering::unordered); \
        \
        CHECK_CONVERTS(NS ::partial_ordering, QPartialOrdering); \
        CHECK_CONVERTS(NS ::weak_ordering,    QPartialOrdering); \
        CHECK_CONVERTS(NS ::strong_ordering,  QPartialOrdering); \
    } while (false)

    CHECK_ALL(Qt);
#ifdef __cpp_lib_three_way_comparison
    CHECK_ALL(std);
#endif // __cpp_lib_three_way_comparison

#undef CHECK_ALL
#undef CHECK_CONVERTS
}

void tst_QCompare::stdQtBinaryCompatibility()
{
#ifndef __cpp_lib_three_way_comparison
    QSKIP("This test requires C++20 three-way-comparison support enabled in the stdlib.");
#else
    QCOMPARE_EQ(sizeof(std::partial_ordering), 1U);
    QCOMPARE_EQ(sizeof( Qt::partial_ordering), 1U);
    QCOMPARE_EQ(sizeof(std::   weak_ordering), 1U);
    QCOMPARE_EQ(sizeof( Qt::   weak_ordering), 1U);
    QCOMPARE_EQ(sizeof(std:: strong_ordering), 1U);
    QCOMPARE_EQ(sizeof( Qt:: strong_ordering), 1U);

    auto valueOf = [](auto obj) {
        typename QIntegerForSizeof<decltype(obj)>::Unsigned value;
        memcpy(&value, &obj, sizeof(obj));
        return value;
    };
#define CHECK(type, flag) \
        QCOMPARE_EQ(valueOf( Qt:: type ## _ordering :: flag), \
                    valueOf(std:: type ## _ordering :: flag)) \
        /* end */
    CHECK(partial, unordered);
    CHECK(partial, less);
    CHECK(partial, greater);
    CHECK(partial, equivalent);

    CHECK(weak, less);
    CHECK(weak, greater);
    CHECK(weak, equivalent);

    CHECK(strong, less);
    CHECK(strong, greater);
    CHECK(strong, equivalent);
    CHECK(strong, equal);
#undef CHECK
#endif //__cpp_lib_three_way_comparison
}

void tst_QCompare::partialOrdering()
{
    static_assert(Qt::partial_ordering::unordered == Qt::partial_ordering::unordered);
    static_assert(Qt::partial_ordering::unordered != Qt::partial_ordering::less);
    static_assert(Qt::partial_ordering::unordered != Qt::partial_ordering::equivalent);
    static_assert(Qt::partial_ordering::unordered != Qt::partial_ordering::greater);

    static_assert(Qt::partial_ordering::less != Qt::partial_ordering::unordered);
    static_assert(Qt::partial_ordering::less == Qt::partial_ordering::less);
    static_assert(Qt::partial_ordering::less != Qt::partial_ordering::equivalent);
    static_assert(Qt::partial_ordering::less != Qt::partial_ordering::greater);

    static_assert(Qt::partial_ordering::equivalent != Qt::partial_ordering::unordered);
    static_assert(Qt::partial_ordering::equivalent != Qt::partial_ordering::less);
    static_assert(Qt::partial_ordering::equivalent == Qt::partial_ordering::equivalent);
    static_assert(Qt::partial_ordering::equivalent != Qt::partial_ordering::greater);

    static_assert(Qt::partial_ordering::greater != Qt::partial_ordering::unordered);
    static_assert(Qt::partial_ordering::greater != Qt::partial_ordering::less);
    static_assert(Qt::partial_ordering::greater != Qt::partial_ordering::equivalent);
    static_assert(Qt::partial_ordering::greater == Qt::partial_ordering::greater);

    static_assert(!is_eq  (Qt::partial_ordering::unordered));
    static_assert( is_neq (Qt::partial_ordering::unordered));
    static_assert(!is_lt  (Qt::partial_ordering::unordered));
    static_assert(!is_lteq(Qt::partial_ordering::unordered));
    static_assert(!is_gt  (Qt::partial_ordering::unordered));
    static_assert(!is_gteq(Qt::partial_ordering::unordered));

    static_assert(!(Qt::partial_ordering::unordered == 0));
    static_assert( (Qt::partial_ordering::unordered != 0));
    static_assert(!(Qt::partial_ordering::unordered <  0));
    static_assert(!(Qt::partial_ordering::unordered <= 0));
    static_assert(!(Qt::partial_ordering::unordered >  0));
    static_assert(!(Qt::partial_ordering::unordered >= 0));

    static_assert(!(0 == Qt::partial_ordering::unordered));
    static_assert( (0 != Qt::partial_ordering::unordered));
    static_assert(!(0 <  Qt::partial_ordering::unordered));
    static_assert(!(0 <= Qt::partial_ordering::unordered));
    static_assert(!(0 >  Qt::partial_ordering::unordered));
    static_assert(!(0 >= Qt::partial_ordering::unordered));


    static_assert(!is_eq  (Qt::partial_ordering::less));
    static_assert( is_neq (Qt::partial_ordering::less));
    static_assert( is_lt  (Qt::partial_ordering::less));
    static_assert( is_lteq(Qt::partial_ordering::less));
    static_assert(!is_gt  (Qt::partial_ordering::less));
    static_assert(!is_gteq(Qt::partial_ordering::less));

    static_assert(!(Qt::partial_ordering::less == 0));
    static_assert( (Qt::partial_ordering::less != 0));
    static_assert( (Qt::partial_ordering::less <  0));
    static_assert( (Qt::partial_ordering::less <= 0));
    static_assert(!(Qt::partial_ordering::less >  0));
    static_assert(!(Qt::partial_ordering::less >= 0));

    static_assert(!(0 == Qt::partial_ordering::less));
    static_assert( (0 != Qt::partial_ordering::less));
    static_assert(!(0 <  Qt::partial_ordering::less));
    static_assert(!(0 <= Qt::partial_ordering::less));
    static_assert( (0 >  Qt::partial_ordering::less));
    static_assert( (0 >= Qt::partial_ordering::less));


    static_assert( is_eq  (Qt::partial_ordering::equivalent));
    static_assert(!is_neq (Qt::partial_ordering::equivalent));
    static_assert(!is_lt  (Qt::partial_ordering::equivalent));
    static_assert( is_lteq(Qt::partial_ordering::equivalent));
    static_assert(!is_gt  (Qt::partial_ordering::equivalent));
    static_assert( is_gteq(Qt::partial_ordering::equivalent));

    static_assert( (Qt::partial_ordering::equivalent == 0));
    static_assert(!(Qt::partial_ordering::equivalent != 0));
    static_assert(!(Qt::partial_ordering::equivalent <  0));
    static_assert( (Qt::partial_ordering::equivalent <= 0));
    static_assert(!(Qt::partial_ordering::equivalent >  0));
    static_assert( (Qt::partial_ordering::equivalent >= 0));

    static_assert( (0 == Qt::partial_ordering::equivalent));
    static_assert(!(0 != Qt::partial_ordering::equivalent));
    static_assert(!(0 <  Qt::partial_ordering::equivalent));
    static_assert( (0 <= Qt::partial_ordering::equivalent));
    static_assert(!(0 >  Qt::partial_ordering::equivalent));
    static_assert( (0 >= Qt::partial_ordering::equivalent));


    static_assert(!is_eq  (Qt::partial_ordering::greater));
    static_assert( is_neq (Qt::partial_ordering::greater));
    static_assert(!is_lt  (Qt::partial_ordering::greater));
    static_assert(!is_lteq(Qt::partial_ordering::greater));
    static_assert( is_gt  (Qt::partial_ordering::greater));
    static_assert( is_gteq(Qt::partial_ordering::greater));

    static_assert(!(Qt::partial_ordering::greater == 0));
    static_assert( (Qt::partial_ordering::greater != 0));
    static_assert(!(Qt::partial_ordering::greater <  0));
    static_assert(!(Qt::partial_ordering::greater <= 0));
    static_assert( (Qt::partial_ordering::greater >  0));
    static_assert( (Qt::partial_ordering::greater >= 0));

    static_assert(!(0 == Qt::partial_ordering::greater));
    static_assert( (0 != Qt::partial_ordering::greater));
    static_assert( (0 <  Qt::partial_ordering::greater));
    static_assert( (0 <= Qt::partial_ordering::greater));
    static_assert(!(0 >  Qt::partial_ordering::greater));
    static_assert(!(0 >= Qt::partial_ordering::greater));
}

void tst_QCompare::weakOrdering()
{
    static_assert(Qt::weak_ordering::less == Qt::weak_ordering::less);
    static_assert(Qt::weak_ordering::less != Qt::weak_ordering::equivalent);
    static_assert(Qt::weak_ordering::less != Qt::weak_ordering::greater);

    static_assert(Qt::weak_ordering::equivalent != Qt::weak_ordering::less);
    static_assert(Qt::weak_ordering::equivalent == Qt::weak_ordering::equivalent);
    static_assert(Qt::weak_ordering::equivalent != Qt::weak_ordering::greater);

    static_assert(Qt::weak_ordering::greater != Qt::weak_ordering::less);
    static_assert(Qt::weak_ordering::greater != Qt::weak_ordering::equivalent);
    static_assert(Qt::weak_ordering::greater == Qt::weak_ordering::greater);

    static_assert(!is_eq  (Qt::weak_ordering::less));
    static_assert( is_neq (Qt::weak_ordering::less));
    static_assert( is_lt  (Qt::weak_ordering::less));
    static_assert( is_lteq(Qt::weak_ordering::less));
    static_assert(!is_gt  (Qt::weak_ordering::less));
    static_assert(!is_gteq(Qt::weak_ordering::less));

    static_assert(!(Qt::weak_ordering::less == 0));
    static_assert( (Qt::weak_ordering::less != 0));
    static_assert( (Qt::weak_ordering::less <  0));
    static_assert( (Qt::weak_ordering::less <= 0));
    static_assert(!(Qt::weak_ordering::less >  0));
    static_assert(!(Qt::weak_ordering::less >= 0));

    static_assert(!(0 == Qt::weak_ordering::less));
    static_assert( (0 != Qt::weak_ordering::less));
    static_assert(!(0 <  Qt::weak_ordering::less));
    static_assert(!(0 <= Qt::weak_ordering::less));
    static_assert( (0 >  Qt::weak_ordering::less));
    static_assert( (0 >= Qt::weak_ordering::less));


    static_assert( is_eq  (Qt::weak_ordering::equivalent));
    static_assert(!is_neq (Qt::weak_ordering::equivalent));
    static_assert(!is_lt  (Qt::weak_ordering::equivalent));
    static_assert( is_lteq(Qt::weak_ordering::equivalent));
    static_assert(!is_gt  (Qt::weak_ordering::equivalent));
    static_assert( is_gteq(Qt::weak_ordering::equivalent));

    static_assert( (Qt::weak_ordering::equivalent == 0));
    static_assert(!(Qt::weak_ordering::equivalent != 0));
    static_assert(!(Qt::weak_ordering::equivalent <  0));
    static_assert( (Qt::weak_ordering::equivalent <= 0));
    static_assert(!(Qt::weak_ordering::equivalent >  0));
    static_assert( (Qt::weak_ordering::equivalent >= 0));

    static_assert( (0 == Qt::weak_ordering::equivalent));
    static_assert(!(0 != Qt::weak_ordering::equivalent));
    static_assert(!(0 <  Qt::weak_ordering::equivalent));
    static_assert( (0 <= Qt::weak_ordering::equivalent));
    static_assert(!(0 >  Qt::weak_ordering::equivalent));
    static_assert( (0 >= Qt::weak_ordering::equivalent));


    static_assert(!is_eq  (Qt::weak_ordering::greater));
    static_assert( is_neq (Qt::weak_ordering::greater));
    static_assert(!is_lt  (Qt::weak_ordering::greater));
    static_assert(!is_lteq(Qt::weak_ordering::greater));
    static_assert( is_gt  (Qt::weak_ordering::greater));
    static_assert( is_gteq(Qt::weak_ordering::greater));

    static_assert(!(Qt::weak_ordering::greater == 0));
    static_assert( (Qt::weak_ordering::greater != 0));
    static_assert(!(Qt::weak_ordering::greater <  0));
    static_assert(!(Qt::weak_ordering::greater <= 0));
    static_assert( (Qt::weak_ordering::greater >  0));
    static_assert( (Qt::weak_ordering::greater >= 0));

    static_assert(!(0 == Qt::weak_ordering::greater));
    static_assert( (0 != Qt::weak_ordering::greater));
    static_assert( (0 <  Qt::weak_ordering::greater));
    static_assert( (0 <= Qt::weak_ordering::greater));
    static_assert(!(0 >  Qt::weak_ordering::greater));
    static_assert(!(0 >= Qt::weak_ordering::greater));
}

void tst_QCompare::strongOrdering()
{
    static_assert(Qt::strong_ordering::less == Qt::strong_ordering::less);
    static_assert(Qt::strong_ordering::less != Qt::strong_ordering::equal);
    static_assert(Qt::strong_ordering::less != Qt::strong_ordering::equivalent);
    static_assert(Qt::strong_ordering::less != Qt::strong_ordering::greater);

    static_assert(Qt::strong_ordering::equal != Qt::strong_ordering::less);
    static_assert(Qt::strong_ordering::equal == Qt::strong_ordering::equal);
    static_assert(Qt::strong_ordering::equal == Qt::strong_ordering::equivalent);
    static_assert(Qt::strong_ordering::equal != Qt::strong_ordering::greater);

    static_assert(Qt::strong_ordering::equivalent != Qt::strong_ordering::less);
    static_assert(Qt::strong_ordering::equivalent == Qt::strong_ordering::equal);
    static_assert(Qt::strong_ordering::equivalent == Qt::strong_ordering::equivalent);
    static_assert(Qt::strong_ordering::equivalent != Qt::strong_ordering::greater);

    static_assert(Qt::strong_ordering::greater != Qt::strong_ordering::less);
    static_assert(Qt::strong_ordering::greater != Qt::strong_ordering::equal);
    static_assert(Qt::strong_ordering::greater != Qt::strong_ordering::equivalent);
    static_assert(Qt::strong_ordering::greater == Qt::strong_ordering::greater);

    static_assert(!is_eq  (Qt::strong_ordering::less));
    static_assert( is_neq (Qt::strong_ordering::less));
    static_assert( is_lt  (Qt::strong_ordering::less));
    static_assert( is_lteq(Qt::strong_ordering::less));
    static_assert(!is_gt  (Qt::strong_ordering::less));
    static_assert(!is_gteq(Qt::strong_ordering::less));

    static_assert(!(Qt::strong_ordering::less == 0));
    static_assert( (Qt::strong_ordering::less != 0));
    static_assert( (Qt::strong_ordering::less <  0));
    static_assert( (Qt::strong_ordering::less <= 0));
    static_assert(!(Qt::strong_ordering::less >  0));
    static_assert(!(Qt::strong_ordering::less >= 0));

    static_assert(!(0 == Qt::strong_ordering::less));
    static_assert( (0 != Qt::strong_ordering::less));
    static_assert(!(0 <  Qt::strong_ordering::less));
    static_assert(!(0 <= Qt::strong_ordering::less));
    static_assert( (0 >  Qt::strong_ordering::less));
    static_assert( (0 >= Qt::strong_ordering::less));


    static_assert( is_eq  (Qt::strong_ordering::equal));
    static_assert(!is_neq (Qt::strong_ordering::equal));
    static_assert(!is_lt  (Qt::strong_ordering::equal));
    static_assert( is_lteq(Qt::strong_ordering::equal));
    static_assert(!is_gt  (Qt::strong_ordering::equal));
    static_assert( is_gteq(Qt::strong_ordering::equal));

    static_assert( (Qt::strong_ordering::equal == 0));
    static_assert(!(Qt::strong_ordering::equal != 0));
    static_assert(!(Qt::strong_ordering::equal <  0));
    static_assert( (Qt::strong_ordering::equal <= 0));
    static_assert(!(Qt::strong_ordering::equal >  0));
    static_assert( (Qt::strong_ordering::equal >= 0));

    static_assert( (0 == Qt::strong_ordering::equal));
    static_assert(!(0 != Qt::strong_ordering::equal));
    static_assert(!(0 <  Qt::strong_ordering::equal));
    static_assert( (0 <= Qt::strong_ordering::equal));
    static_assert(!(0 >  Qt::strong_ordering::equal));
    static_assert( (0 >= Qt::strong_ordering::equal));


    static_assert( is_eq  (Qt::strong_ordering::equivalent));
    static_assert(!is_neq (Qt::strong_ordering::equivalent));
    static_assert(!is_lt  (Qt::strong_ordering::equivalent));
    static_assert( is_lteq(Qt::strong_ordering::equivalent));
    static_assert(!is_gt  (Qt::strong_ordering::equivalent));
    static_assert( is_gteq(Qt::strong_ordering::equivalent));

    static_assert( (Qt::strong_ordering::equivalent == 0));
    static_assert(!(Qt::strong_ordering::equivalent != 0));
    static_assert(!(Qt::strong_ordering::equivalent <  0));
    static_assert( (Qt::strong_ordering::equivalent <= 0));
    static_assert(!(Qt::strong_ordering::equivalent >  0));
    static_assert( (Qt::strong_ordering::equivalent >= 0));

    static_assert( (0 == Qt::strong_ordering::equivalent));
    static_assert(!(0 != Qt::strong_ordering::equivalent));
    static_assert(!(0 <  Qt::strong_ordering::equivalent));
    static_assert( (0 <= Qt::strong_ordering::equivalent));
    static_assert(!(0 >  Qt::strong_ordering::equivalent));
    static_assert( (0 >= Qt::strong_ordering::equivalent));


    static_assert(!is_eq  (Qt::strong_ordering::greater));
    static_assert( is_neq (Qt::strong_ordering::greater));
    static_assert(!is_lt  (Qt::strong_ordering::greater));
    static_assert(!is_lteq(Qt::strong_ordering::greater));
    static_assert( is_gt  (Qt::strong_ordering::greater));
    static_assert( is_gteq(Qt::strong_ordering::greater));

    static_assert(!(Qt::strong_ordering::greater == 0));
    static_assert( (Qt::strong_ordering::greater != 0));
    static_assert(!(Qt::strong_ordering::greater <  0));
    static_assert(!(Qt::strong_ordering::greater <= 0));
    static_assert( (Qt::strong_ordering::greater >  0));
    static_assert( (Qt::strong_ordering::greater >= 0));

    static_assert(!(0 == Qt::strong_ordering::greater));
    static_assert( (0 != Qt::strong_ordering::greater));
    static_assert( (0 <  Qt::strong_ordering::greater));
    static_assert( (0 <= Qt::strong_ordering::greater));
    static_assert(!(0 >  Qt::strong_ordering::greater));
    static_assert(!(0 >= Qt::strong_ordering::greater));
}

void tst_QCompare::threeWayCompareWithLiteralZero()
{
#ifndef __cpp_lib_three_way_comparison
    QSKIP("This test requires C++20 <=> support enabled in the compiler and the stdlib.");
#else
    // the result of <=> is _always_ a std::_ordering type:
#define CHECK(O) do { \
        using StdO = typename QtOrderingPrivate::StdOrdering<O>::type; \
        static_assert(std::is_same_v<decltype(0 <=> std::declval<O&>()), StdO>); \
        static_assert(std::is_same_v<decltype(std::declval<O&>() <=> 0), StdO>); \
    } while (false)

    CHECK(Qt::partial_ordering);
    CHECK(Qt::weak_ordering);
    CHECK(Qt::strong_ordering);
    CHECK(QPartialOrdering);
    // API symmetry check:
    CHECK(std::partial_ordering);
    CHECK(std::weak_ordering);
    CHECK(std::strong_ordering);

#undef CHECK

#define CHECK(O, what, reversed) do { \
        using StdO = typename QtOrderingPrivate::StdOrdering<O>::type; \
        static_assert((O :: what <=> 0) == StdO:: what); \
        static_assert((0 <=> O :: what) == StdO:: reversed); \
    } while (false)

    CHECK(Qt::partial_ordering, unordered,  unordered);
    CHECK(Qt::partial_ordering, equivalent, equivalent);
    CHECK(Qt::partial_ordering, less,       greater);
    CHECK(Qt::partial_ordering, greater,    less);

    CHECK(Qt::weak_ordering, equivalent, equivalent);
    CHECK(Qt::weak_ordering, less,       greater);
    CHECK(Qt::weak_ordering, greater,    less);

    CHECK(Qt::strong_ordering, equal,     equal);
    CHECK(Qt::strong_ordering, less,      greater);
    CHECK(Qt::strong_ordering, greater,   less);

    CHECK(QPartialOrdering, unordered,  unordered);
    CHECK(QPartialOrdering, equivalent, equivalent);
    CHECK(QPartialOrdering, less,       greater);
    CHECK(QPartialOrdering, greater,    less);

    // API symmetry check:

    CHECK(std::partial_ordering, unordered,  unordered);
    CHECK(std::partial_ordering, equivalent, equivalent);
    CHECK(std::partial_ordering, less,       greater);
    CHECK(std::partial_ordering, greater,    less);

    CHECK(std::weak_ordering, equivalent, equivalent);
    CHECK(std::weak_ordering, less,       greater);
    CHECK(std::weak_ordering, greater,    less);

    CHECK(std::strong_ordering, equal,     equal);
    CHECK(std::strong_ordering, less,      greater);
    CHECK(std::strong_ordering, greater,   less);

#undef CHECK
#endif // __cpp_lib_three_way_comparisons

}

void tst_QCompare::conversions()
{
    // Qt::weak_ordering -> Qt::partial_ordering
    {
        constexpr Qt::partial_ordering less = Qt::weak_ordering::less;
        static_assert(less == Qt::partial_ordering::less);
        constexpr Qt::partial_ordering equivalent = Qt::weak_ordering::equivalent;
        static_assert(equivalent == Qt::partial_ordering::equivalent);
        constexpr Qt::partial_ordering greater = Qt::weak_ordering::greater;
        static_assert(greater == Qt::partial_ordering::greater);
    }
    // Qt::strong_ordering -> Qt::partial_ordering
    {
        constexpr Qt::partial_ordering less = Qt::strong_ordering::less;
        static_assert(less == Qt::partial_ordering::less);
        constexpr Qt::partial_ordering equal = Qt::strong_ordering::equal;
        static_assert(equal == Qt::partial_ordering::equivalent);
        constexpr Qt::partial_ordering equivalent = Qt::strong_ordering::equivalent;
        static_assert(equivalent == Qt::partial_ordering::equivalent);
        constexpr Qt::partial_ordering greater = Qt::strong_ordering::greater;
        static_assert(greater == Qt::partial_ordering::greater);
    }
    // Qt::strong_ordering -> Qt::weak_ordering
    {
        constexpr Qt::weak_ordering less = Qt::strong_ordering::less;
        static_assert(less == Qt::weak_ordering::less);
        constexpr Qt::weak_ordering equal = Qt::strong_ordering::equal;
        static_assert(equal == Qt::weak_ordering::equivalent);
        constexpr Qt::weak_ordering equivalent = Qt::strong_ordering::equivalent;
        static_assert(equivalent == Qt::weak_ordering::equivalent);
        constexpr Qt::weak_ordering greater = Qt::strong_ordering::greater;
        static_assert(greater == Qt::weak_ordering::greater);
    }
    // Mixed types
    {
        static_assert(Qt::partial_ordering::less == Qt::strong_ordering::less);
        static_assert(Qt::partial_ordering::equivalent != Qt::strong_ordering::less);
        static_assert(Qt::partial_ordering::equivalent == Qt::strong_ordering::equal);
        static_assert(Qt::partial_ordering::greater == Qt::strong_ordering::greater);

        static_assert(Qt::partial_ordering::less == Qt::weak_ordering::less);
        static_assert(Qt::partial_ordering::equivalent == Qt::weak_ordering::equivalent);
        static_assert(Qt::partial_ordering::greater == Qt::weak_ordering::greater);

        static_assert(Qt::weak_ordering::less == Qt::strong_ordering::less);
        static_assert(Qt::weak_ordering::equivalent != Qt::strong_ordering::greater);
        static_assert(Qt::weak_ordering::equivalent == Qt::strong_ordering::equal);
        static_assert(Qt::weak_ordering::greater == Qt::strong_ordering::greater);

        static_assert(Qt::weak_ordering::less == Qt::partial_ordering::less);
        static_assert(Qt::weak_ordering::equivalent == Qt::partial_ordering::equivalent);
        static_assert(Qt::weak_ordering::greater == Qt::partial_ordering::greater);

        static_assert(Qt::strong_ordering::less == Qt::partial_ordering::less);
        static_assert(Qt::strong_ordering::equivalent == Qt::partial_ordering::equivalent);
        static_assert(Qt::strong_ordering::equal == Qt::partial_ordering::equivalent);
        static_assert(Qt::strong_ordering::greater == Qt::partial_ordering::greater);

        static_assert(Qt::strong_ordering::less == Qt::weak_ordering::less);
        static_assert(Qt::strong_ordering::equivalent == Qt::weak_ordering::equivalent);
        static_assert(Qt::strong_ordering::equal == Qt::weak_ordering::equivalent);
        static_assert(Qt::strong_ordering::greater == Qt::weak_ordering::greater);
    }
#ifdef __cpp_lib_three_way_comparison
    // Qt::partial_ordering <-> std::partial_ordering
    {
        static_assert(Qt::partial_ordering::less == std::partial_ordering::less);
        static_assert(Qt::partial_ordering::less != std::partial_ordering::greater);
        static_assert(std::partial_ordering::unordered != Qt::partial_ordering::equivalent);
        static_assert(std::partial_ordering::unordered == Qt::partial_ordering::unordered);

        static_assert((Qt::partial_ordering(std::partial_ordering::less) ==
                       std::partial_ordering::less));
        static_assert((Qt::partial_ordering(std::partial_ordering::equivalent) ==
                       std::partial_ordering::equivalent));
        static_assert((Qt::partial_ordering(std::partial_ordering::greater) ==
                       std::partial_ordering::greater));
        static_assert((Qt::partial_ordering(std::partial_ordering::unordered) ==
                       std::partial_ordering::unordered));
    }
    // Qt::weak_ordering <-> std::weak_ordering
    {
        static_assert(Qt::weak_ordering::less == std::weak_ordering::less);
        static_assert(Qt::weak_ordering::less != std::weak_ordering::equivalent);
        static_assert(std::weak_ordering::greater != Qt::weak_ordering::less);
        static_assert(std::weak_ordering::equivalent == Qt::weak_ordering::equivalent);

        static_assert((Qt::weak_ordering(std::weak_ordering::less) ==
                       std::weak_ordering::less));
        static_assert((Qt::weak_ordering(std::weak_ordering::equivalent) ==
                       std::weak_ordering::equivalent));
        static_assert((Qt::weak_ordering(std::weak_ordering::greater) ==
                       std::weak_ordering::greater));
    }
    // Qt::strong_ordering <-> std::strong_ordering
    {
        static_assert(Qt::strong_ordering::less == std::strong_ordering::less);
        static_assert(Qt::strong_ordering::less != std::strong_ordering::equivalent);
        static_assert(std::strong_ordering::greater != Qt::strong_ordering::less);
        static_assert(std::strong_ordering::equivalent == Qt::strong_ordering::equivalent);

        static_assert((Qt::strong_ordering(std::strong_ordering::less) ==
                       std::strong_ordering::less));
        static_assert((Qt::strong_ordering(std::strong_ordering::equivalent) ==
                       std::strong_ordering::equivalent));
        static_assert((Qt::strong_ordering(std::strong_ordering::greater) ==
                       std::strong_ordering::greater));
    }
    // Mixed Qt::*_ordering <> std::*_ordering types
    {
        static_assert(Qt::strong_ordering::less == std::partial_ordering::less);
        static_assert(Qt::strong_ordering::less != std::partial_ordering::greater);
        static_assert(Qt::strong_ordering::equal == std::weak_ordering::equivalent);
        static_assert(Qt::strong_ordering::equivalent != std::weak_ordering::less);

        static_assert(Qt::weak_ordering::less != std::partial_ordering::greater);
        static_assert(Qt::weak_ordering::less == std::partial_ordering::less);
        static_assert(Qt::weak_ordering::equivalent == std::strong_ordering::equivalent);
        static_assert(Qt::weak_ordering::equivalent != std::strong_ordering::less);

        static_assert(Qt::partial_ordering::less != std::weak_ordering::greater);
        static_assert(Qt::partial_ordering::less == std::weak_ordering::less);
        static_assert(Qt::partial_ordering::equivalent == std::strong_ordering::equivalent);
        static_assert(Qt::partial_ordering::equivalent != std::strong_ordering::less);
    }
#endif

}

void tst_QCompare::is_eq_overloads()
{
#ifndef __cpp_lib_three_way_comparison
    QSKIP("This test requires C++20 three-way-comparison support enabled in the stdlib.");
#else
    constexpr auto u = std::partial_ordering::unordered;
    constexpr auto l = std::weak_ordering::less;
    constexpr auto g = std::strong_ordering::greater;
    constexpr auto e = std::weak_ordering::equivalent;
    constexpr auto s = std::strong_ordering::equal;

    // This is a compile-time check that unqualified name lookup of
    // std::is_eq-like functions isn't ambiguous, so we can recommend it to our
    // users for minimizing porting on the way to C++20.

    // The goal is to check each std::ordering and each is_eq function at least
    // once, not to test all combinations (we're not the stdlib test suite here).

    QVERIFY(is_eq(s));
    QVERIFY(is_neq(u));
    QVERIFY(is_lt(l));
    QVERIFY(is_gt(g));
    QVERIFY(is_lteq(e));
    QVERIFY(is_gteq(s));
#endif // __cpp_lib_three_way_comparison
}

class StringWrapper
{
public:
    explicit StringWrapper() {}
    explicit StringWrapper(const QString &val) : m_val(val) {}
    QString value() const { return m_val; }

private:
    static Qt::weak_ordering compareHelper(const QString &lhs, const QString &rhs) noexcept
    {
        const int res = QString::compare(lhs, rhs, Qt::CaseInsensitive);
        if (res < 0)
            return Qt::weak_ordering::less;
        else if (res > 0)
            return Qt::weak_ordering::greater;
        else
            return Qt::weak_ordering::equivalent;
    }

    friend bool comparesEqual(const StringWrapper &lhs, const StringWrapper &rhs) noexcept
    { return QString::compare(lhs.m_val, rhs.m_val, Qt::CaseInsensitive) == 0; }
    friend Qt::weak_ordering
    compareThreeWay(const StringWrapper &lhs, const StringWrapper &rhs) noexcept
    { return compareHelper(lhs.m_val, rhs.m_val); }
    Q_DECLARE_WEAKLY_ORDERED(StringWrapper)

    // these helper functions are intentionally non-noexcept
    friend bool comparesEqual(const StringWrapper &lhs, int rhs)
    { return comparesEqual(lhs, StringWrapper(QString::number(rhs))); }
    friend Qt::weak_ordering compareThreeWay(const StringWrapper &lhs, int rhs)
    { return compareHelper(lhs.m_val, QString::number(rhs)); }
    Q_DECLARE_WEAKLY_ORDERED_NON_NOEXCEPT(StringWrapper, int)

    QString m_val;
};

void tst_QCompare::compareThreeWay()
{
    // test noexcept

    // for custom types
    static_assert(noexcept(qCompareThreeWay(std::declval<StringWrapper>(),
                                            std::declval<StringWrapper>())));
    static_assert(!noexcept(qCompareThreeWay(std::declval<StringWrapper>(),
                                             std::declval<int>())));
    static_assert(!noexcept(qCompareThreeWay(std::declval<int>(),
                                             std::declval<StringWrapper>())));
    // for built-in types
    static_assert(noexcept(qCompareThreeWay(std::declval<int>(), std::declval<int>())));
    static_assert(noexcept(qCompareThreeWay(std::declval<float>(), std::declval<int>())));
    static_assert(noexcept(qCompareThreeWay(std::declval<double>(), std::declval<float>())));
    static_assert(noexcept(qCompareThreeWay(std::declval<int>(), std::declval<int>())));
    // enums
    static_assert(noexcept(qCompareThreeWay(std::declval<TestEnum>(), std::declval<TestEnum>())));

    // pointers
    using StringWrapperPtr = Qt::totally_ordered_wrapper<StringWrapper *>;
    static_assert(noexcept(qCompareThreeWay(std::declval<StringWrapperPtr>(),
                                            std::declval<StringWrapperPtr>())));
    static_assert(noexcept(qCompareThreeWay(std::declval<StringWrapperPtr>(), nullptr)));

    // Test some actual comparison results

    // for custom types
    QCOMPARE_EQ(qCompareThreeWay(StringWrapper("ABC"), StringWrapper("abc")),
                Qt::weak_ordering::equivalent);
    QVERIFY(StringWrapper("ABC") == StringWrapper("abc"));
    QCOMPARE_EQ(qCompareThreeWay(StringWrapper("ABC"), StringWrapper("qwe")),
                Qt::weak_ordering::less);
    QVERIFY(StringWrapper("ABC") != StringWrapper("qwe"));
    QCOMPARE_EQ(qCompareThreeWay(StringWrapper("qwe"), StringWrapper("ABC")),
                Qt::weak_ordering::greater);
    QVERIFY(StringWrapper("qwe") != StringWrapper("ABC"));
    QCOMPARE_EQ(qCompareThreeWay(StringWrapper("10"), 10), Qt::weak_ordering::equivalent);
    QVERIFY(StringWrapper("10") == 10);
    QCOMPARE_EQ(qCompareThreeWay(StringWrapper("10"), 12), Qt::weak_ordering::less);
    QVERIFY(StringWrapper("10") != 12);
    QCOMPARE_EQ(qCompareThreeWay(StringWrapper("12"), 10), Qt::weak_ordering::greater);
    QVERIFY(StringWrapper("12") != 10);

    // reversed compareThreeWay()
    auto result = qCompareThreeWay(10, StringWrapper("12"));
    QCOMPARE_EQ(result, Qt::weak_ordering::less);
    static_assert(std::is_same_v<decltype(result), Qt::weak_ordering>);
    QVERIFY(10 != StringWrapper("12"));
    result = qCompareThreeWay(12, StringWrapper("10"));
    QCOMPARE_EQ(result, Qt::weak_ordering::greater);
    static_assert(std::is_same_v<decltype(result), Qt::weak_ordering>);
    QVERIFY(12 != StringWrapper("10"));
    result = qCompareThreeWay(10, StringWrapper("10"));
    QCOMPARE_EQ(result, Qt::weak_ordering::equivalent);
    static_assert(std::is_same_v<decltype(result), Qt::weak_ordering>);
    QVERIFY(10 == StringWrapper("10"));

    // built-in types
    QCOMPARE_EQ(qCompareThreeWay(1, 1.0), Qt::partial_ordering::equivalent);
    QCOMPARE_EQ(qCompareThreeWay(1, 2), Qt::strong_ordering::less);
    QCOMPARE_EQ(qCompareThreeWay(2.0f, 1.0), Qt::partial_ordering::greater);

    // enums
    QCOMPARE_EQ(qCompareThreeWay(Smaller, Bigger), Qt::strong_ordering::less);

    // pointers
    std::array<int, 2> arr{1, 0};
#if QT_DEPRECATED_SINCE(6, 8)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    QCOMPARE_EQ(qCompareThreeWay(&arr[1], &arr[0]), Qt::strong_ordering::greater);
    QCOMPARE_EQ(qCompareThreeWay(arr.data(), &arr[0]), Qt::strong_ordering::equivalent);
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 8)
    const auto a0 = Qt::totally_ordered_wrapper(&arr[0]);
    const auto a1 = Qt::totally_ordered_wrapper(&arr[1]);
    QCOMPARE_EQ(qCompareThreeWay(a1, a0), Qt::strong_ordering::greater);
    QCOMPARE_EQ(qCompareThreeWay(arr.data(), a0), Qt::strong_ordering::equivalent);
}

void tst_QCompare::unorderedNeqLiteralZero()
{
    // This test is checking QTBUG-127759
    constexpr auto qtUnordered = Qt::partial_ordering::unordered;
    constexpr auto qtLegacyUnordered = QPartialOrdering::Unordered;
#ifdef __cpp_lib_three_way_comparison
    constexpr auto stdUnordered = std::partial_ordering::unordered;

    QVERIFY(stdUnordered != 0);
    QVERIFY(0 != stdUnordered);
    QVERIFY(is_neq(stdUnordered));

    QCOMPARE_EQ(qtUnordered != 0, stdUnordered != 0);
    QCOMPARE_EQ(0 != qtUnordered, 0 != stdUnordered);
    QCOMPARE_EQ(is_neq(qtUnordered), is_neq(stdUnordered));

    QCOMPARE_EQ(qtLegacyUnordered != 0, stdUnordered != 0);
    QCOMPARE_EQ(0 != qtLegacyUnordered, 0 != stdUnordered);
    QCOMPARE_EQ(is_neq(qtLegacyUnordered), is_neq(stdUnordered));
#endif
    QVERIFY(qtUnordered != 0);
    QVERIFY(0 != qtUnordered);
    QVERIFY(is_neq(qtUnordered));

    QVERIFY(qtLegacyUnordered != 0);
    QVERIFY(0 != qtLegacyUnordered);
    QVERIFY(is_neq(qtLegacyUnordered));
}

#ifdef __cpp_lib_three_way_comparison
template<typename LeftType, typename RightType, typename OrderingType>
void tst_QCompare::compare3WayHelper(LeftType lhs, RightType rhs, OrderingType order)
{
    // check Qt ordering type.
    QCOMPARE_3WAY(lhs, rhs, QtOrderingPrivate::to_Qt(order));
    // Also check std ordering type.
    QCOMPARE_3WAY(lhs, rhs, QtOrderingPrivate::to_std(order));
}
#endif

void tst_QCompare::compare3WayMacro()
{
#ifdef __cpp_lib_three_way_comparison
    constexpr std::array comparison_array = {1, 0};
    // for custom types
    compare3WayHelper(StringWrapper("ABC"), StringWrapper("abc"),
                      Qt::weak_ordering::equivalent);
    compare3WayHelper(StringWrapper("ABC"), StringWrapper("qwe"),
                      Qt::weak_ordering::less);
    compare3WayHelper(StringWrapper("qwe"), StringWrapper("ABC"),
                      Qt::weak_ordering::greater);

    compare3WayHelper(StringWrapper("10"), 10, Qt::weak_ordering::equivalent);
    compare3WayHelper(StringWrapper("10"), 12, Qt::weak_ordering::less);
    compare3WayHelper(StringWrapper("12"), 10, Qt::weak_ordering::greater);

    // reversed compareThreeWay()
    compare3WayHelper(10, StringWrapper("12"), Qt::weak_ordering::less);
    compare3WayHelper(12, StringWrapper("10"), Qt::weak_ordering::greater);
    compare3WayHelper(10, StringWrapper("10"), Qt::weak_ordering::equivalent);

    // built-in types
    compare3WayHelper(1, 1.0, Qt::partial_ordering::equivalent);
    compare3WayHelper(1, 2, Qt::strong_ordering::less);
    compare3WayHelper(2.0f, 1.0, Qt::partial_ordering::greater);

    // enums
    compare3WayHelper(Smaller, Bigger, Qt::strong_ordering::less);

    // pointers
    compare3WayHelper(&comparison_array[1], &comparison_array[0], Qt::strong_ordering::greater);
    compare3WayHelper(comparison_array.data(),
                      &comparison_array[0], Qt::strong_ordering::equivalent);

    const QString lhs = QStringLiteral("abc");

    // We don't want to put those into test data since we want to specifically test comparison
    // against inline string literals.
    compare3WayHelper(lhs, u"", Qt::strong_ordering::greater);
    compare3WayHelper(lhs, u"abc", Qt::strong_ordering::equal);
    compare3WayHelper(lhs, u"abcd", Qt::strong_ordering::less);

    compare3WayHelper(lhs, "", Qt::strong_ordering::greater);
    compare3WayHelper(lhs, "abc", Qt::strong_ordering::equal);
    compare3WayHelper(lhs, "abcd", Qt::strong_ordering::less);
#else
    QSKIP("This test requires C++20 comparison support enabled in the standard library");
#endif
}

QTEST_MAIN(tst_QCompare)
#include "tst_qcompare.moc"
