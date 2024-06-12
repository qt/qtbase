// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qcomparehelpers.h"

#define DECLARE_TYPE(Name, Type, RetType, Constexpr, Suffix) \
class Templated ## Name \
{ \
public: \
    Constexpr Templated ## Name () {} \
\
private: \
    template <typename X> \
    friend Constexpr bool \
    comparesEqual(const Templated ## Name &lhs, X rhs) noexcept; \
    template <typename X> \
    friend Constexpr RetType \
    compareThreeWay(const Templated ## Name &lhs, X rhs) noexcept; \
    Q_DECLARE_ ## Type ## _ORDERED ## Suffix (Templated ## Name, X, template <typename X>) \
}; \
\
template <typename X> \
Constexpr bool comparesEqual(const Templated ## Name &lhs, X rhs) noexcept \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return true; } \
template <typename X> \
Constexpr RetType compareThreeWay(const Templated ## Name &lhs, X rhs) noexcept \
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
#define COMPARE(ClassName) \
    do { \
        ClassName c; \
        QCOMPARE_EQ(c, 0); \
        QCOMPARE_LE(c, 0); \
        QCOMPARE_GE(0, c); \
    } while (false)

    COMPARE(TemplatedPartialConst);
    COMPARE(TemplatedPartial);
    COMPARE(TemplatedWeakConst);
    COMPARE(TemplatedWeak);
    COMPARE(TemplatedStrongConst);
    COMPARE(TemplatedStrong);

#undef COMPARE
}

void tst_QCompareHelpers::totallyOrderedWrapperBasics()
{
    Qt::totally_ordered_wrapper<int*> pi; // partially-formed
    pi = nullptr;
    QCOMPARE_EQ(pi.get(), nullptr);
}
