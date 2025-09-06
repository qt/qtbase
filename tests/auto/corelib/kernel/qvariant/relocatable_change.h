// Copyright (C) 2025 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef RELOCATABLE_CHANGE_H
#define RELOCATABLE_CHANGE_H

#ifndef WHICH_TYPE_IS_RELOCATABLE
#  error must define WHICH_TYPE_IS_RELOCATABLE
#endif

#include <qvariant.h>
#include <QtTest/QTest>

#include <type_traits>

// A typical non-trivial type that may change from not relocatable to relocable
template <int I> struct MaybeRelocatableTypeTemplate
{
    static constexpr quint8 Which = I;
    void *ptr = nullptr;
    quintptr value = (I + 1) * quintptr(Q_UINT64_C(0x0001'0002'0008'0010));

    MaybeRelocatableTypeTemplate() = default;
    /* not constexpr */ ~MaybeRelocatableTypeTemplate() {}
    MaybeRelocatableTypeTemplate(const MaybeRelocatableTypeTemplate &other) noexcept
        : value(other.value)
    {}
    MaybeRelocatableTypeTemplate(MaybeRelocatableTypeTemplate &&other) noexcept
        : value(other.value)
    {}
    MaybeRelocatableTypeTemplate &operator=(const MaybeRelocatableTypeTemplate &other) noexcept
    { value = other.value; return *this; }
    MaybeRelocatableTypeTemplate &operator=(MaybeRelocatableTypeTemplate &&other) noexcept
    { value = other.value; return *this; }
};
struct RelocatableInAppType : MaybeRelocatableTypeTemplate<0> {};
struct RelocatableInPluginType : MaybeRelocatableTypeTemplate<1> {};

template <typename Type> QVariant relocatabilityChange_create()
{
    // common to all instantiations
    static_assert(!std::is_trivially_copy_constructible_v<Type>);
    static_assert(!std::is_trivially_move_constructible_v<Type>);
    static_assert(!std::is_trivially_copy_assignable_v<Type>);
    static_assert(!std::is_trivially_move_assignable_v<Type>);
    static_assert(!std::is_trivially_destructible_v<Type>);
    static_assert(QVariant::Private::FitsInInternalSize<sizeof(Type)>);

    static constexpr bool IsRelocatable = std::is_same_v<Type, WHICH_TYPE_IS_RELOCATABLE>;
    static_assert(QTypeInfo<Type>::isRelocatable == IsRelocatable);
    static_assert(QVariant::Private::CanUseInternalSpace<Type> == IsRelocatable);

    Type t;
    t.ptr = reinterpret_cast<void *>(&relocatabilityChange_create<Type>);
    return QVariant::fromValue(t);
}

#endif // RELOCATABLE_CHANGE_H
