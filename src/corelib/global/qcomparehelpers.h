// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPARE_H
#error "Do not include qcomparehelpers.h directly. Use qcompare.h instead."
#endif

#ifndef QCOMPAREHELPERS_H
#define QCOMPAREHELPERS_H

#if 0
#pragma qt_no_master_include
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qoverload.h>
#include <QtCore/qttypetraits.h>
#include <QtCore/qtypes.h>

#ifdef __cpp_lib_three_way_comparison
#include <compare>
#endif
#include <QtCore/q20type_traits.h>

#include <functional> // std::less

QT_BEGIN_NAMESPACE

class QPartialOrdering;

namespace QtOrderingPrivate {
#ifdef __cpp_lib_three_way_comparison

template <typename QtOrdering> struct StdOrdering;
template <typename StdOrdering> struct QtOrdering;

#define QT_STD_MAP(x) \
    template <> struct StdOrdering< Qt::x##_ordering> : q20::type_identity<std::x##_ordering> {};\
    template <> struct StdOrdering<std::x##_ordering> : q20::type_identity<std::x##_ordering> {};\
    template <> struct  QtOrdering<std::x##_ordering> : q20::type_identity< Qt::x##_ordering> {};\
    template <> struct  QtOrdering< Qt::x##_ordering> : q20::type_identity< Qt::x##_ordering> {};\
    /* end */
QT_STD_MAP(partial)
QT_STD_MAP(weak)
QT_STD_MAP(strong)
#undef QT_STD_MAP

template <> struct StdOrdering<QPartialOrdering> : q20::type_identity<std::partial_ordering> {};
template <> struct  QtOrdering<QPartialOrdering> : q20::type_identity< Qt::partial_ordering> {};

template <typename In> constexpr auto to_std(In in) noexcept
    -> typename QtOrderingPrivate::StdOrdering<In>::type
{ return in; }

template <typename In> constexpr auto to_Qt(In in) noexcept
    -> typename QtOrderingPrivate::QtOrdering<In>::type
{ return in; }

#endif // __cpp_lib_three_way_comparison
} // namespace QtOrderingPrivate

/*
    For all the macros these parameter names are used:
    * LeftType - the type of the left operand of the comparison
    * RightType - the type of the right operand of the comparison
    * Constexpr - must be either constexpr or empty. Defines whether the
                  operator is constexpr or not
    * Attributes - an optional list of attributes. For example, pass
                   \c QT_ASCII_CAST_WARN when defining comparisons between
                   C-style string and an encoding-aware string type.

    The macros require two helper functions. For operators to be constexpr,
    these must be constexpr, too. Additionally, other attributes (like
    Q_<Module>_EXPORT, Q_DECL_CONST_FUNCTION, etc) can be applied to them.
    Aside from that, their declaration should match:
        bool comparesEqual(LeftType, RightType) noexcept;
        ReturnType compareThreeWay(LeftType, RightType) noexcept;

    The ReturnType can be one of Qt::{partial,weak,strong}_ordering. The actual
    type depends on the macro being used.
    It makes sense to define the helper functions as hidden friends of the
    class, so that they could be found via ADL, and don't participate in
    unintended implicit conversions.
*/

// Seems that qdoc uses C++20 even when Qt is compiled in C++17 mode.
// Or at least it defines __cpp_lib_three_way_comparison.
// Let qdoc see only the C++17 operators for now, because that's what our docs
// currently describe.
#if defined(__cpp_lib_three_way_comparison) && !defined(Q_QDOC)
// C++20 - provide operator==() for equality, and operator<=>() for ordering

#define QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, Constexpr, Attributes) \
    Attributes \
    friend Constexpr bool operator==(LeftType const &lhs, RightType const &rhs) \
        noexcept(noexcept(comparesEqual(lhs, rhs))) \
    { return comparesEqual(lhs, rhs); }

#define QT_DECLARE_3WAY_HELPER_STRONG(LeftType, RightType, Constexpr, Attributes) \
    Attributes \
    friend Constexpr std::strong_ordering \
    operator<=>(LeftType const &lhs, RightType const &rhs) \
        noexcept(noexcept(compareThreeWay(lhs, rhs))) \
    { \
        return compareThreeWay(lhs, rhs); \
    }

#define QT_DECLARE_3WAY_HELPER_WEAK(LeftType, RightType, Constexpr, Attributes) \
    Attributes \
    friend Constexpr std::weak_ordering \
    operator<=>(LeftType const &lhs, RightType const &rhs) \
        noexcept(noexcept(compareThreeWay(lhs, rhs))) \
    { \
        return compareThreeWay(lhs, rhs); \
    }

#define QT_DECLARE_3WAY_HELPER_PARTIAL(LeftType, RightType, Constexpr, Attributes) \
    Attributes \
    friend Constexpr std::partial_ordering \
    operator<=>(LeftType const &lhs, RightType const &rhs) \
        noexcept(noexcept(compareThreeWay(lhs, rhs))) \
    { \
        return compareThreeWay(lhs, rhs); \
    }

#define QT_DECLARE_ORDERING_OPERATORS_HELPER(OrderingType, LeftType, RightType, Constexpr, \
                                             Attributes) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, Constexpr, Attributes) \
    QT_DECLARE_3WAY_HELPER_ ## OrderingType (LeftType, RightType, Constexpr, Attributes)

#ifdef Q_COMPILER_LACKS_THREE_WAY_COMPARE_SYMMETRY

// define reversed versions of the operators manually, because buggy MSVC versions do not do it
#define QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, Constexpr, Attributes) \
    Attributes \
    friend Constexpr bool operator==(RightType const &lhs, LeftType const &rhs) \
        noexcept(noexcept(comparesEqual(rhs, lhs))) \
    { return comparesEqual(rhs, lhs); }

#define QT_DECLARE_REVERSED_3WAY_HELPER_STRONG(LeftType, RightType, Constexpr, Attributes) \
    Attributes \
    friend Constexpr std::strong_ordering \
    operator<=>(RightType const &lhs, LeftType const &rhs) \
        noexcept(noexcept(compareThreeWay(rhs, lhs))) \
    { \
        const auto r = compareThreeWay(rhs, lhs); \
        if (is_gt(r)) return std::strong_ordering::less; \
        if (is_lt(r)) return std::strong_ordering::greater; \
        return r; \
    }

#define QT_DECLARE_REVERSED_3WAY_HELPER_WEAK(LeftType, RightType, Constexpr, Attributes) \
    Attributes \
    friend Constexpr std::weak_ordering \
    operator<=>(RightType const &lhs, LeftType const &rhs) \
        noexcept(noexcept(compareThreeWay(rhs, lhs))) \
    { \
        const auto r = compareThreeWay(rhs, lhs); \
        if (is_gt(r)) return std::weak_ordering::less; \
        if (is_lt(r)) return std::weak_ordering::greater; \
        return r; \
    }

#define QT_DECLARE_REVERSED_3WAY_HELPER_PARTIAL(LeftType, RightType, Constexpr, Attributes) \
    Attributes \
    friend Constexpr std::partial_ordering \
    operator<=>(RightType const &lhs, LeftType const &rhs) \
        noexcept(noexcept(compareThreeWay(rhs, lhs))) \
    { \
        const auto r = compareThreeWay(rhs, lhs); \
        if (is_gt(r)) return std::partial_ordering::less; \
        if (is_lt(r)) return std::partial_ordering::greater; \
        return r; \
    }

#define QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(OrderingString, LeftType, RightType, \
                                                      Constexpr, Attributes) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, Constexpr, Attributes) \
    QT_DECLARE_REVERSED_3WAY_HELPER_ ## OrderingString (LeftType, RightType, Constexpr, Attributes)

#else

// dummy macros for C++17 compatibility, reversed operators are generated by the compiler
#define QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, Constexpr, Attributes)
#define QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(OrderingString, LeftType, RightType, \
                                                      Constexpr, Attributes)

#endif // Q_COMPILER_LACKS_THREE_WAY_COMPARE_SYMMETRY

#else
// C++17 - provide operator==() and operator!=() for equality,
// and all 4 comparison operators for ordering

#define QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, Constexpr, Attributes) \
    Attributes \
    friend Constexpr bool operator==(LeftType const &lhs, RightType const &rhs) \
        noexcept(noexcept(comparesEqual(lhs, rhs))) \
    { return comparesEqual(lhs, rhs); } \
    Attributes \
    friend Constexpr bool operator!=(LeftType const &lhs, RightType const &rhs) \
        noexcept(noexcept(comparesEqual(lhs, rhs))) \
    { return !comparesEqual(lhs, rhs); }

// Helpers for reversed comparison, using the existing comparesEqual() function.
#define QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, Constexpr, Attributes) \
    Attributes \
    friend Constexpr bool operator==(RightType const &lhs, LeftType const &rhs) \
        noexcept(noexcept(comparesEqual(rhs, lhs))) \
    { return comparesEqual(rhs, lhs); } \
    Attributes \
    friend Constexpr bool operator!=(RightType const &lhs, LeftType const &rhs) \
        noexcept(noexcept(comparesEqual(rhs, lhs))) \
    { return !comparesEqual(rhs, lhs); }

#define QT_DECLARE_ORDERING_HELPER_TEMPLATE(OrderingType, LeftType, RightType, Constexpr, \
                                            Attributes) \
    Attributes \
    friend Constexpr bool operator<(LeftType const &lhs, RightType const &rhs) \
        noexcept(noexcept(compareThreeWay(lhs, rhs))) \
    { return is_lt(compareThreeWay(lhs, rhs)); }      \
    Attributes \
    friend Constexpr bool operator>(LeftType const &lhs, RightType const &rhs) \
        noexcept(noexcept(compareThreeWay(lhs, rhs))) \
    { return is_gt(compareThreeWay(lhs, rhs)); }      \
    Attributes \
    friend Constexpr bool operator<=(LeftType const &lhs, RightType const &rhs) \
        noexcept(noexcept(compareThreeWay(lhs, rhs))) \
    { return is_lteq(compareThreeWay(lhs, rhs)); }    \
    Attributes \
    friend Constexpr bool operator>=(LeftType const &lhs, RightType const &rhs) \
        noexcept(noexcept(compareThreeWay(lhs, rhs))) \
    { return is_gteq(compareThreeWay(lhs, rhs)); }

#define QT_DECLARE_ORDERING_HELPER_PARTIAL(LeftType, RightType, Constexpr, Attributes) \
    QT_DECLARE_ORDERING_HELPER_TEMPLATE(Qt::partial_ordering, LeftType, RightType, Constexpr, \
                                        Attributes)

#define QT_DECLARE_ORDERING_HELPER_WEAK(LeftType, RightType, Constexpr, Attributes) \
    QT_DECLARE_ORDERING_HELPER_TEMPLATE(Qt::weak_ordering, LeftType, RightType, Constexpr, \
                                        Attributes)

#define QT_DECLARE_ORDERING_HELPER_STRONG(LeftType, RightType, Constexpr, Attributes) \
    QT_DECLARE_ORDERING_HELPER_TEMPLATE(Qt::strong_ordering, LeftType, RightType, Constexpr, \
                                        Attributes)

#define QT_DECLARE_ORDERING_OPERATORS_HELPER(OrderingString, LeftType, RightType, Constexpr, \
                                             Attributes) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, Constexpr, Attributes) \
    QT_DECLARE_ORDERING_HELPER_ ## OrderingString (LeftType, RightType, Constexpr, Attributes)

// Helpers for reversed ordering, using the existing compareThreeWay() function.
#define QT_DECLARE_REVERSED_ORDERING_HELPER_TEMPLATE(OrderingType, LeftType, RightType, Constexpr, \
                                                     Attributes) \
    Attributes \
    friend Constexpr bool operator<(RightType const &lhs, LeftType const &rhs) \
        noexcept(noexcept(compareThreeWay(rhs, lhs))) \
    { return is_gt(compareThreeWay(rhs, lhs)); }      \
    Attributes \
    friend Constexpr bool operator>(RightType const &lhs, LeftType const &rhs) \
        noexcept(noexcept(compareThreeWay(rhs, lhs))) \
    { return is_lt(compareThreeWay(rhs, lhs)); }      \
    Attributes \
    friend Constexpr bool operator<=(RightType const &lhs, LeftType const &rhs) \
        noexcept(noexcept(compareThreeWay(rhs, lhs))) \
    { return is_gteq(compareThreeWay(rhs, lhs)); }    \
    Attributes \
    friend Constexpr bool operator>=(RightType const &lhs, LeftType const &rhs) \
        noexcept(noexcept(compareThreeWay(rhs, lhs))) \
    { return is_lteq(compareThreeWay(rhs, lhs)); }

#define QT_DECLARE_REVERSED_ORDERING_HELPER_PARTIAL(LeftType, RightType, Constexpr, Attributes) \
    QT_DECLARE_REVERSED_ORDERING_HELPER_TEMPLATE(Qt::partial_ordering, LeftType, RightType, \
                                                 Constexpr, Attributes)

#define QT_DECLARE_REVERSED_ORDERING_HELPER_WEAK(LeftType, RightType, Constexpr, Attributes) \
    QT_DECLARE_REVERSED_ORDERING_HELPER_TEMPLATE(Qt::weak_ordering, LeftType, RightType, \
                                                 Constexpr, Attributes)

#define QT_DECLARE_REVERSED_ORDERING_HELPER_STRONG(LeftType, RightType, Constexpr, Attributes) \
    QT_DECLARE_REVERSED_ORDERING_HELPER_TEMPLATE(Qt::strong_ordering, LeftType, RightType, \
                                                 Constexpr, Attributes)

#define QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(OrderingString, LeftType, RightType, \
                                                      Constexpr, Attributes) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, Constexpr, Attributes) \
    QT_DECLARE_REVERSED_ORDERING_HELPER_ ## OrderingString (LeftType, RightType, Constexpr, \
                                                            Attributes)

#endif // __cpp_lib_three_way_comparison

/* Public API starts here */

// Equality operators
#define QT_DECLARE_EQUALITY_COMPARABLE_1(Type) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(Type, Type, /* non-constexpr */, /* no attributes */)

#define QT_DECLARE_EQUALITY_COMPARABLE_2(LeftType, RightType) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, /* non-constexpr */, \
                                         /* no attributes */) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, /* non-constexpr */, \
                                                  /* no attributes */)

#define QT_DECLARE_EQUALITY_COMPARABLE_3(LeftType, RightType, Attributes) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, /* non-constexpr */, Attributes) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, /* non-constexpr */, \
                                                  Attributes)

#define Q_DECLARE_EQUALITY_COMPARABLE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_EQUALITY_COMPARABLE, __VA_ARGS__)

#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_1(Type) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(Type, Type, constexpr, /* no attributes */)

#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_2(LeftType, RightType) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, constexpr, /* no attributes */) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, constexpr, \
                                                  /* no attributes */)

#define QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE_3(LeftType, RightType, Attributes) \
    QT_DECLARE_EQUALITY_OPERATORS_HELPER(LeftType, RightType, constexpr, Attributes) \
    QT_DECLARE_EQUALITY_OPERATORS_REVERSED_HELPER(LeftType, RightType, constexpr, Attributes)

#define Q_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_EQUALITY_COMPARABLE_LITERAL_TYPE, __VA_ARGS__)

// Partial ordering operators
#define QT_DECLARE_PARTIALLY_ORDERED_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, Type, Type, /* non-constexpr */, \
                                         /* no attributes */)

#define QT_DECLARE_PARTIALLY_ORDERED_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, LeftType, RightType, /* non-constexpr */, \
                                         /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(PARTIAL, LeftType, RightType, \
                                                  /* non-constexpr */, /* no attributes */)

#define QT_DECLARE_PARTIALLY_ORDERED_3(LeftType, RightType, Attributes) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, LeftType, RightType, /* non-constexpr */, \
                                         Attributes) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(PARTIAL, LeftType, RightType, \
                                                  /* non-constexpr */, Attributes)

#define Q_DECLARE_PARTIALLY_ORDERED(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_PARTIALLY_ORDERED, __VA_ARGS__)

#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, Type, Type, constexpr, /* no attributes */)

#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, LeftType, RightType, constexpr, \
                                         /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(PARTIAL, LeftType, RightType, constexpr, \
                                                  /* no attributes */)

#define QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE_3(LeftType, RightType, Attributes) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(PARTIAL, LeftType, RightType, constexpr, Attributes) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(PARTIAL, LeftType, RightType, constexpr, \
                                                  Attributes)

#define Q_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_PARTIALLY_ORDERED_LITERAL_TYPE, __VA_ARGS__)

// Weak ordering operators
#define QT_DECLARE_WEAKLY_ORDERED_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, Type, Type, /* non-constexpr */, /* no attributes */)

#define QT_DECLARE_WEAKLY_ORDERED_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                         /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                                  /* no attributes */)

#define QT_DECLARE_WEAKLY_ORDERED_3(LeftType, RightType, Attributes) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                         Attributes) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(WEAK, LeftType, RightType, /* non-constexpr */, \
                                                  Attributes)

#define Q_DECLARE_WEAKLY_ORDERED(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_WEAKLY_ORDERED, __VA_ARGS__)

#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, Type, Type, constexpr, /* no attributes */)

#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, LeftType, RightType, constexpr, \
                                         /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(WEAK, LeftType, RightType, constexpr, \
                                                  /* no attributes */)

#define QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE_3(LeftType, RightType, Attributes) \
QT_DECLARE_ORDERING_OPERATORS_HELPER(WEAK, LeftType, RightType, constexpr, Attributes) \
        QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(WEAK, LeftType, RightType, constexpr, \
                                                      Attributes)

#define Q_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_WEAKLY_ORDERED_LITERAL_TYPE, __VA_ARGS__)

// Strong ordering operators
#define QT_DECLARE_STRONGLY_ORDERED_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, Type, Type, /* non-constexpr */, \
                                         /* no attributes */)

#define QT_DECLARE_STRONGLY_ORDERED_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, LeftType, RightType, /* non-constexpr */, \
                                         /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(STRONG, LeftType, RightType, \
                                                  /* non-constexpr */, /* no attributes */)

#define QT_DECLARE_STRONGLY_ORDERED_3(LeftType, RightType, Attributes) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, LeftType, RightType, /* non-constexpr */, \
                                         Attributes) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(STRONG, LeftType, RightType, \
                                                  /* non-constexpr */, Attributes)

#define Q_DECLARE_STRONGLY_ORDERED(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_STRONGLY_ORDERED, __VA_ARGS__)

#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_1(Type) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, Type, Type, constexpr, /* no attributes */)

#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_2(LeftType, RightType) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, LeftType, RightType, constexpr, \
                                         /* no attributes */) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(STRONG, LeftType, RightType, constexpr, \
                                                  /* no attributes */)

#define QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE_3(LeftType, RightType, Attributes) \
    QT_DECLARE_ORDERING_OPERATORS_HELPER(STRONG, LeftType, RightType, constexpr, Attributes) \
    QT_DECLARE_ORDERING_OPERATORS_REVERSED_HELPER(STRONG, LeftType, RightType, constexpr, \
                                                  Attributes)

#define Q_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE(...) \
    QT_OVERLOADED_MACRO(QT_DECLARE_STRONGLY_ORDERED_LITERAL_TYPE, __VA_ARGS__)

namespace QtPrivate {

template <typename T>
constexpr bool IsIntegralType_v = std::numeric_limits<std::remove_const_t<T>>::is_specialized
                                  && std::numeric_limits<std::remove_const_t<T>>::is_integer;

template <typename T>
constexpr bool IsFloatType_v = std::is_floating_point_v<T>;

#if QFLOAT16_IS_NATIVE
template <>
constexpr bool IsFloatType_v<QtPrivate::NativeFloat16Type> = true;
#endif

} // namespace QtPrivate

namespace Qt {

template <typename T>
using if_integral = std::enable_if_t<QtPrivate::IsIntegralType_v<T>, bool>;

template <typename T>
using if_floating_point = std::enable_if_t<QtPrivate::IsFloatType_v<T>, bool>;

template <typename T, typename U>
using if_compatible_pointers =
        std::enable_if_t<std::disjunction_v<std::is_same<T, U>,
                                            std::is_base_of<T, U>,
                                            std::is_base_of<U, T>>,
                         bool>;

template <typename Enum>
using if_enum = std::enable_if_t<std::is_enum_v<Enum>, bool>;

template <typename LeftInt, typename RightInt,
          if_integral<LeftInt> = true,
          if_integral<RightInt> = true>
constexpr Qt::strong_ordering compareThreeWay(LeftInt lhs, RightInt rhs) noexcept
{
    static_assert(std::is_signed_v<LeftInt> == std::is_signed_v<RightInt>,
                  "Qt::compareThreeWay() does not allow mixed-sign comparison.");

#ifdef __cpp_lib_three_way_comparison
    return lhs <=> rhs;
#else
    if (lhs == rhs)
        return Qt::strong_ordering::equivalent;
    else if (lhs < rhs)
        return Qt::strong_ordering::less;
    else
        return Qt::strong_ordering::greater;
#endif // __cpp_lib_three_way_comparison
}

template <typename LeftFloat, typename RightFloat,
          if_floating_point<LeftFloat> = true,
          if_floating_point<RightFloat> = true>
constexpr Qt::partial_ordering compareThreeWay(LeftFloat lhs, RightFloat rhs) noexcept
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_FLOAT_COMPARE
#ifdef __cpp_lib_three_way_comparison
    return lhs <=> rhs;
#else
    if (lhs < rhs)
        return Qt::partial_ordering::less;
    else if (lhs > rhs)
        return Qt::partial_ordering::greater;
    else if (lhs == rhs)
        return Qt::partial_ordering::equivalent;
    else
        return Qt::partial_ordering::unordered;
#endif // __cpp_lib_three_way_comparison
QT_WARNING_POP
}

template <typename IntType, typename FloatType,
          if_integral<IntType> = true,
          if_floating_point<FloatType> = true>
constexpr Qt::partial_ordering compareThreeWay(IntType lhs, FloatType rhs) noexcept
{
    return compareThreeWay(FloatType(lhs), rhs);
}

template <typename FloatType, typename IntType,
          if_floating_point<FloatType> = true,
          if_integral<IntType> = true>
constexpr Qt::partial_ordering compareThreeWay(FloatType lhs, IntType rhs) noexcept
{
    return compareThreeWay(lhs, FloatType(rhs));
}

template <typename LeftType, typename RightType,
          if_compatible_pointers<LeftType, RightType> = true>
constexpr Qt::strong_ordering compareThreeWay(const LeftType *lhs, const RightType *rhs) noexcept
{
#ifdef __cpp_lib_three_way_comparison
    return std::compare_three_way{}(lhs, rhs);
#else
    if (lhs == rhs)
        return Qt::strong_ordering::equivalent;
    else if (std::less<>{}(lhs, rhs))
        return Qt::strong_ordering::less;
    else
        return Qt::strong_ordering::greater;
#endif // __cpp_lib_three_way_comparison
}

template <typename T>
constexpr Qt::strong_ordering compareThreeWay(const T *lhs, std::nullptr_t rhs) noexcept
{
    return compareThreeWay(lhs, static_cast<const T *>(rhs));
}

template <typename T>
constexpr Qt::strong_ordering compareThreeWay(std::nullptr_t lhs, const T *rhs) noexcept
{
    return compareThreeWay(static_cast<const T *>(lhs), rhs);
}

template <class Enum, if_enum<Enum> = true>
constexpr Qt::strong_ordering compareThreeWay(Enum lhs, Enum rhs) noexcept
{
    return compareThreeWay(qToUnderlying(lhs), qToUnderlying(rhs));
}
} // namespace Qt

namespace QtOrderingPrivate {

template <typename Head, typename...Tail, std::size_t...Is>
constexpr std::tuple<Tail...> qt_tuple_pop_front_impl(const std::tuple<Head, Tail...> &t,
                                                      std::index_sequence<Is...>) noexcept
{
    return std::tuple<Tail...>(std::get<Is + 1>(t)...);
}

template <typename Head, typename...Tail>
constexpr std::tuple<Tail...> qt_tuple_pop_front(const std::tuple<Head, Tail...> &t) noexcept
{
    return qt_tuple_pop_front_impl(t, std::index_sequence_for<Tail...>{});
}

template <typename LhsHead, typename...LhsTail, typename RhsHead, typename...RhsTail>
constexpr auto compareThreeWayMulti(const std::tuple<LhsHead, LhsTail...> &lhs, // ie. not empty
                                    const std::tuple<RhsHead, RhsTail...> &rhs) noexcept
{
    static_assert(sizeof...(LhsTail) == sizeof...(RhsTail),
                                                            // expanded together below, but provide a nicer error message:
                  "The tuple arguments have to have the same size.");

    using Qt::compareThreeWay;
    using R = std::common_type_t<
            decltype(compareThreeWay(std::declval<LhsHead>(), std::declval<RhsHead>())),
            decltype(compareThreeWay(std::declval<LhsTail>(), std::declval<RhsTail>()))...
            >;

    const auto &l = std::get<0>(lhs);
    const auto &r = std::get<0>(rhs);
    static_assert(noexcept(compareThreeWay(l, r)),
                  "This function requires all relational operators to be noexcept.");
    const auto res = compareThreeWay(l, r);
    if constexpr (sizeof...(LhsTail) > 0) {
        if (is_eq(res))
            return R{compareThreeWayMulti(qt_tuple_pop_front(lhs), qt_tuple_pop_front(rhs))};
    }
    return R{res};
}

} //QtOrderingPrivate

namespace Qt {
// A wrapper class that adapts the wrappee to use the strongly-ordered
// <functional> function objects for implementing the relational operators.
// Mostly useful to avoid UB on pointers (which it currently mandates P to be),
// because all the comparison helpers (incl. std::compare_three_way on
// std::tuple<T*>!) will use the language-level operators.
//
template <typename P>
class totally_ordered_wrapper
{
    static_assert(std::is_pointer_v<P>);
    using T = std::remove_pointer_t<P>;

    P ptr;
public:
    totally_ordered_wrapper() noexcept = default;
    explicit constexpr totally_ordered_wrapper(P p) noexcept : ptr(p) {}

    constexpr P get() const noexcept { return ptr; }
    constexpr P operator->() const noexcept { return get(); }
    constexpr T& operator*() const noexcept { return *get(); }

    explicit constexpr operator bool() const noexcept { return get(); }

private:
    friend constexpr auto compareThreeWay(const totally_ordered_wrapper &lhs, const totally_ordered_wrapper &rhs) noexcept
    { return Qt::compareThreeWay(lhs.ptr, rhs.ptr); }
#define MAKE_RELOP(Ret, op, Op) \
    friend constexpr Ret operator op (const totally_ordered_wrapper &lhs, const totally_ordered_wrapper &rhs) noexcept \
    { return std:: Op {}(lhs.ptr, rhs.ptr); } \
    friend constexpr Ret operator op (const totally_ordered_wrapper &lhs, const P &rhs) noexcept \
    { return std:: Op {}(lhs.ptr, rhs    ); } \
    friend constexpr Ret operator op (const P &lhs, const totally_ordered_wrapper &rhs) noexcept \
    { return std:: Op {}(lhs,     rhs.ptr); } \
    friend constexpr Ret operator op (const totally_ordered_wrapper &lhs, std::nullptr_t) noexcept \
    { return std:: Op {}(lhs.ptr, nullptr); } \
    friend constexpr Ret operator op (std::nullptr_t, const totally_ordered_wrapper &rhs) noexcept \
    { return std:: Op {}(nullptr, rhs.ptr); } \
    /* end */
    MAKE_RELOP(bool, ==, equal_to<P>)
    MAKE_RELOP(bool, !=, not_equal_to<P>)
    MAKE_RELOP(bool, < , less<P>)
    MAKE_RELOP(bool, <=, less_equal<P>)
    MAKE_RELOP(bool, > , greater<P>)
    MAKE_RELOP(bool, >=, greater_equal<P>)
#ifdef __cpp_lib_three_way_comparison
    MAKE_RELOP(auto, <=>, compare_three_way)
#endif
#undef MAKE_RELOP
};

} //Qt

QT_END_NAMESPACE

#endif // QCOMPAREHELPERS_H
