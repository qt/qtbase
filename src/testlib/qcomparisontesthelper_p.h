// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPARISONTESTHELPER_P_H
#define QCOMPARISONTESTHELPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/q20type_traits.h>
#include <QtTest/qtest.h>

QT_BEGIN_NAMESPACE

namespace QTestPrivate {

Q_TESTLIB_EXPORT QByteArray formatTypeWithCRefImpl(QMetaType type, bool isConst,
                                                   bool isRef, bool isRvalueRef);

template <typename T>
QByteArray formatTypeWithCRef()
{
    return formatTypeWithCRefImpl(QMetaType::fromType<q20::remove_cvref_t<T>>(),
                                  std::is_const_v<std::remove_reference_t<T>>,
                                  std::is_reference_v<T>,
                                  std::is_rvalue_reference_v<T>);
}

#define FOR_EACH_CREF(Func, Left, Right, Op, Result) \
    Func(Left &, Right &, Op, Result) \
    Func(Left &, Right const &, Op, Result) \
    Func(Left &, Right &&, Op, Result) \
    Func(Left &, Right const &&, Op, Result) \
    Func(Left const &, Right &, Op, Result) \
    Func(Left const &, Right const &, Op, Result) \
    Func(Left const &, Right &&, Op, Result) \
    Func(Left const &, Right const &&, Op, Result) \
    Func(Left &&, Right &, Op, Result) \
    Func(Left &&, Right const &, Op, Result) \
    Func(Left &&, Right &&, Op, Result) \
    Func(Left &&, Right const &&, Op, Result) \
    Func(Left const &&, Right &, Op, Result) \
    Func(Left const &&, Right const &, Op, Result) \
    Func(Left const &&, Right &&, Op, Result) \
    Func(Left const &&, Right const &&, Op, Result) \
    /* END */

#define CHECK_SINGLE_OPERATOR(Left, Right, Op, Result) \
    do { \
        constexpr bool qtest_op_check_isImplNoexcept \
                        = noexcept(std::declval<Left>() Op std::declval<Right>()); \
        if constexpr (!qtest_op_check_isImplNoexcept) { \
            QEXPECT_FAIL("", QByteArray("(" + formatTypeWithCRef<Left>() \
                                        + " " #Op " " + formatTypeWithCRef<Right>() \
                                        + ") is not noexcept").constData(), \
                         Continue); \
            /* Ideally, operators should be noexcept, so warn if they are not. */ \
            /* Do not make it a hard error, because the fix is not always trivial. */ \
            QVERIFY(qtest_op_check_isImplNoexcept); \
        } \
        static_assert(std::is_convertible_v<decltype( \
                        std::declval<Left>() Op std::declval<Right>()), Result>); \
        if constexpr (!std::is_same_v<Left, Right>) { \
            static_assert(std::is_convertible_v<decltype( \
                            std::declval<Right>() Op std::declval<Left>()), Result>); \
        } \
    } while (false); \
    /* END */

/*!
    \internal

    This function checks that the types \c LeftType and \c RightType properly
    define {in}equality operators (== and !=). The checks are performed for
    all combinations of cvref-qualified lvalues and rvalues.
*/
template <typename LeftType, typename RightType = LeftType>
void testEqualityOperatorsCompile()
{
    FOR_EACH_CREF(CHECK_SINGLE_OPERATOR, LeftType, RightType, ==, bool)
    FOR_EACH_CREF(CHECK_SINGLE_OPERATOR, LeftType, RightType, !=, bool)
}

/*!
    \internal

    This function checks that the types \c LeftType and \c RightType properly
    define all comparison operators (==, !=, <, >, <=, >=). The checks are
    performed for all combinations of cvref-qualified lvalues and rvalues.
*/
template <typename LeftType, typename RightType = LeftType>
void testAllComparisonOperatorsCompile()
{
    testEqualityOperatorsCompile<LeftType, RightType>();
    if (QTest::currentTestFailed())
        return;
    FOR_EACH_CREF(CHECK_SINGLE_OPERATOR, LeftType, RightType, >, bool)
    FOR_EACH_CREF(CHECK_SINGLE_OPERATOR, LeftType, RightType, <, bool)
    FOR_EACH_CREF(CHECK_SINGLE_OPERATOR, LeftType, RightType, >=, bool)
    FOR_EACH_CREF(CHECK_SINGLE_OPERATOR, LeftType, RightType, <=, bool)
}

#undef CHECK_SINGLE_OPERATOR
#undef FOR_EACH_CREF

#define CHECK_RUNTIME_LR(Left, Right, Op, Expected) \
    do { \
        QCOMPARE_EQ(Left Op Right, Expected); \
        QCOMPARE_EQ(std::move(Left) Op Right, Expected); \
        QCOMPARE_EQ(Left Op std::move(Right), Expected); \
        QCOMPARE_EQ(std::move(Left) Op std::move(Right), Expected); \
    } while (false); \
    /* END */

#define CHECK_RUNTIME_CREF(Func, Left, Right, Op, Expected) \
    Func(Left, Right, Op, Expected); \
    Func(std::as_const(Left), Right, Op, Expected); \
    Func(Left, std::as_const(Right), Op, Expected); \
    Func(std::as_const(Left), std::as_const(Right), Op, Expected); \
    /* END */

/*!
    \internal
    Basic testing of equality operators.

    The helper function tests {in}equality operators (== and !=) for the \a lhs
    operand of type \c {LeftType} and the \a rhs operand of type \c {RightType}.

    The \a expectedEqual parameter is an expected result for \c {operator==()}.

    \note Any test calling this method will need to check the test state after
    doing so, if there is any later code in the test.

    \code
    QTime early(12, 34, 56, 00);
    QTime later(12, 34, 56, 01);
    QTestPrivate::testEqualityOperators(early, later, false);
    if (QTest:currentTestFailed())
        return;
    \endcode
*/
template <typename LeftType, typename RightType>
void testEqualityOperators(LeftType lhs, RightType rhs, bool expectedEqual)
{
    CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, lhs, rhs, ==, expectedEqual)
    CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, lhs, rhs, !=, !expectedEqual)
    if constexpr (!std::is_same_v<LeftType, RightType>) {
        CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, rhs, lhs, ==, expectedEqual)
        CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, rhs, lhs, !=, !expectedEqual)
    }
}

/*!
    \internal
    Basic testing of equality and relation operators.

    The helper function tests all six relation and equality operators
    (==, !=, <, >, <=, >=) for the \a lhs operand of type \c {LeftType} and
    the \a rhs operand of type \c {RightType}.

    The \c OrderingType must be one of QPartialOrdering, QStrongOrdering, or
    QWeakOrdering.

    The \a expectedOrdering parameter provides the expected
    relation between \a lhs and \a rhs.

    \note Any test calling this method will need to check the test state after
    doing so, if there is any later code in the test.

    \code
    QDateTime now = QDateTime::currentDateTime();
    QDateTime later = now.addMSec(1);
    QTestPrivate::testComparisonOperators(now, later, QWeakOrdering::Less);
    if (QTest:currentTestFailed())
        return;
    \endcode
*/
template <typename LeftType, typename RightType, typename OrderingType>
void testAllComparisonOperators(LeftType lhs, RightType rhs, OrderingType expectedOrdering)
{
    constexpr bool isQOrderingType = std::is_same_v<OrderingType, QPartialOrdering>
            || std::is_same_v<OrderingType, QWeakOrdering>
            || std::is_same_v<OrderingType, QStrongOrdering>;

    static_assert(isQOrderingType,
                  "Please provide, as the expectedOrdering parameter, a value "
                  "of one of the Q{Partial,Weak,Strong}Ordering types.");

    // We have all sorts of operator==() between Q*Ordering and std::*_ordering
    // types, so we can just compare to QPartialOrdering.
    const bool expectedEqual = expectedOrdering == QPartialOrdering::Equivalent;
    const bool expectedLess = expectedOrdering == QPartialOrdering::Less;
    const bool expectedUnordered = expectedOrdering == QPartialOrdering::Unordered;

    CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, lhs, rhs, ==,
                       !expectedUnordered && expectedEqual)
    CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, lhs, rhs, !=,
                       expectedUnordered || !expectedEqual)
    CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, lhs, rhs, <,
                       !expectedUnordered && expectedLess)
    CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, lhs, rhs, >,
                       !expectedUnordered && !expectedLess && !expectedEqual)
    CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, lhs, rhs, <=,
                       !expectedUnordered && (expectedEqual || expectedLess))
    CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, lhs, rhs, >=,
                       !expectedUnordered && !expectedLess)

    if constexpr (!std::is_same_v<LeftType, RightType>) {
        CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, rhs, lhs, ==,
                           !expectedUnordered && expectedEqual)
        CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, rhs, lhs, !=,
                           expectedUnordered || !expectedEqual)
        CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, rhs, lhs, <,
                           !expectedUnordered && !expectedLess && !expectedEqual)
        CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, rhs, lhs, >,
                           !expectedUnordered && expectedLess)
        CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, rhs, lhs, <=,
                           !expectedUnordered && !expectedLess)
        CHECK_RUNTIME_CREF(CHECK_RUNTIME_LR, rhs, lhs, >=,
                           !expectedUnordered && (expectedEqual || expectedLess))
    }
}

#undef CHECK_RUNTIME_CREF
#undef CHECK_RUNTIME_LR

} // namespace QTestPrivate

QT_END_NAMESPACE

#endif // QCOMPARISONTESTHELPER_P_H
