// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qcomparehelpers.h"

#define DECLARE_TYPE(Name, Type, RetType, Constexpr, Suffix) \
class Deprecated ## Name \
{ \
public: \
    Constexpr Deprecated ## Name () {} \
\
private: \
    friend Constexpr bool \
    comparesEqual(const Deprecated ## Name &lhs, int rhs) noexcept; \
    friend Constexpr RetType \
    compareThreeWay(const Deprecated ## Name &lhs, int rhs) noexcept; \
    Q_DECLARE_ ## Type ## _ORDERED ## Suffix (Deprecated ## Name, int, \
                                              Q_DECL_DEPRECATED_X("This op is deprecated")) \
}; \
\
Constexpr bool comparesEqual(const Deprecated ## Name &lhs, int rhs) noexcept \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return true; } \
Constexpr RetType compareThreeWay(const Deprecated ## Name &lhs, int rhs) noexcept \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return RetType::equivalent; }

DECLARE_TYPE(PartialConst, PARTIALLY, Qt::partial_ordering, constexpr, _LITERAL_TYPE)
DECLARE_TYPE(Partial, PARTIALLY, Qt::partial_ordering, , )
DECLARE_TYPE(WeakConst, WEAKLY, Qt::weak_ordering, constexpr, _LITERAL_TYPE)
DECLARE_TYPE(Weak, WEAKLY, Qt::weak_ordering, , )
DECLARE_TYPE(StrongConst, STRONGLY, Qt::strong_ordering, constexpr, _LITERAL_TYPE)
DECLARE_TYPE(Strong, STRONGLY, Qt::strong_ordering, , )

#undef DECLARE_TYPE

void tst_QCompareHelpers::compareWithAttributes()
{
    // All these comparisons would trigger deprecation warnings.
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED

#define COMPARE(ClassName) \
    do { \
        ClassName c; \
        QCOMPARE_EQ(c, 0); \
        QCOMPARE_LE(c, 0); \
        QCOMPARE_GE(0, c); \
    } while (false)

    COMPARE(DeprecatedPartialConst);
    COMPARE(DeprecatedPartial);
    COMPARE(DeprecatedWeakConst);
    COMPARE(DeprecatedWeak);
    COMPARE(DeprecatedStrongConst);
    COMPARE(DeprecatedStrong);

#undef COMPARE

QT_WARNING_POP
}
