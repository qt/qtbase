// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qcomparehelpers.h"

#define DECLARE_TYPE(Name, Type, RetType, Constexpr, Noex, Suffix) \
class Templated ## Name \
{ \
public: \
    Constexpr Templated ## Name () {} \
\
private: \
    template <typename X> \
    friend Constexpr bool \
    comparesEqual(const Templated ## Name &lhs, X rhs) noexcept(Noex); \
    template <typename X> \
    friend Constexpr RetType \
    compareThreeWay(const Templated ## Name &lhs, X rhs) noexcept(Noex); \
    Q_DECLARE_ ## Type ## _ORDERED ## Suffix (Templated ## Name, X, template <typename X>) \
}; \
\
template <typename X> \
Constexpr bool comparesEqual(const Templated ## Name &lhs, X rhs) noexcept(Noex) \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return true; } \
template <typename X> \
Constexpr RetType compareThreeWay(const Templated ## Name &lhs, X rhs) noexcept(Noex) \
{ Q_UNUSED(lhs); Q_UNUSED(rhs); return RetType::equivalent; }

DECLARE_TYPE(PartialConst, PARTIALLY, Qt::partial_ordering, constexpr, true, _LITERAL_TYPE)
DECLARE_TYPE(Partial, PARTIALLY, Qt::partial_ordering, , true, )
DECLARE_TYPE(PartialNonNoex, PARTIALLY, Qt::partial_ordering, , false, _NON_NOEXCEPT)
DECLARE_TYPE(WeakConst, WEAKLY, Qt::weak_ordering, constexpr, true, _LITERAL_TYPE)
DECLARE_TYPE(Weak, WEAKLY, Qt::weak_ordering, , true, )
DECLARE_TYPE(WeakNonNoex, WEAKLY, Qt::weak_ordering, , false, _NON_NOEXCEPT)
DECLARE_TYPE(StrongConst, STRONGLY, Qt::strong_ordering, constexpr, true, _LITERAL_TYPE)
DECLARE_TYPE(Strong, STRONGLY, Qt::strong_ordering, , true, )
DECLARE_TYPE(StrongNonNoex, STRONGLY, Qt::strong_ordering, , false, _NON_NOEXCEPT)

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
    COMPARE(TemplatedPartialNonNoex);
    COMPARE(TemplatedWeakConst);
    COMPARE(TemplatedWeak);
    COMPARE(TemplatedWeakNonNoex);
    COMPARE(TemplatedStrongConst);
    COMPARE(TemplatedStrong);
    COMPARE(TemplatedStrongNonNoex);

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
