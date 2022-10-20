/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QENDIAN_P_H
#define QENDIAN_P_H

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

#include <QtCore/qendian.h>

QT_BEGIN_NAMESPACE

// Note if using multiple of these bitfields in a union; the underlying storage type must
// match. Since we always use an unsigned storage type, unsigned and signed versions may
// be used together, but different bit-widths may not.
template<class S, int pos, int width>
class QSpecialIntegerBitfield
{
protected:
    typedef typename S::StorageType T;
    typedef typename std::make_unsigned<T>::type UT;

    static constexpr UT mask()
    {
        return ((UT(1) << width) - 1) << pos;
    }
public:
    // FIXME: val is public until qtdeclarative is fixed to not access it directly.
    UT val;

    QSpecialIntegerBitfield &operator=(T t)
    {
        UT i = S::fromSpecial(val);
        i &= ~mask();
        i |= (UT(t) << pos) & mask();
        val = S::toSpecial(i);
        return *this;
    }
    operator T() const
    {
        if (std::is_signed<T>::value) {
            UT i = S::fromSpecial(val);
            i <<= (sizeof(T) * 8) - width - pos;
            T t = T(i);
            t >>= (sizeof(T) * 8) - width;
            return t;
        }
        return (S::fromSpecial(val) & mask()) >> pos;
    }

    bool operator!() const { return !(val & S::toSpecial(mask())); }
    bool operator==(QSpecialIntegerBitfield<S, pos, width> i) const
    {
        return ((val ^ i.val) & S::toSpecial(mask())) == 0;
    }
    bool operator!=(QSpecialIntegerBitfield<S, pos, width> i) const
    {
        return ((val ^ i.val) & S::toSpecial(mask())) != 0;
    }

    QSpecialIntegerBitfield &operator+=(T i) { return (*this = (T(*this) + i)); }
    QSpecialIntegerBitfield &operator-=(T i) { return (*this = (T(*this) - i)); }
    QSpecialIntegerBitfield &operator*=(T i) { return (*this = (T(*this) * i)); }
    QSpecialIntegerBitfield &operator/=(T i) { return (*this = (T(*this) / i)); }
    QSpecialIntegerBitfield &operator%=(T i) { return (*this = (T(*this) % i)); }
    QSpecialIntegerBitfield &operator|=(T i) { return (*this = (T(*this) | i)); }
    QSpecialIntegerBitfield &operator&=(T i) { return (*this = (T(*this) & i)); }
    QSpecialIntegerBitfield &operator^=(T i) { return (*this = (T(*this) ^ i)); }
    QSpecialIntegerBitfield &operator>>=(T i) { return (*this = (T(*this) >> i)); }
    QSpecialIntegerBitfield &operator<<=(T i) { return (*this = (T(*this) << i)); }
};

template<typename T, int pos, int width>
using QLEIntegerBitfield = QSpecialIntegerBitfield<QLittleEndianStorageType<T>, pos, width>;

template<typename T, int pos, int width>
using QBEIntegerBitfield = QSpecialIntegerBitfield<QBigEndianStorageType<T>, pos, width>;

template<int pos, int width>
using qint32_le_bitfield = QLEIntegerBitfield<int, pos, width>;
template<int pos, int width>
using quint32_le_bitfield = QLEIntegerBitfield<uint, pos, width>;
template<int pos, int width>
using qint32_be_bitfield = QBEIntegerBitfield<int, pos, width>;
template<int pos, int width>
using quint32_be_bitfield = QBEIntegerBitfield<uint, pos, width>;

enum class QSpecialIntegerBitfieldInitializer {};
constexpr QSpecialIntegerBitfieldInitializer QSpecialIntegerBitfieldZero{};

template<class S>
class QSpecialIntegerStorage
{
public:
    using UnsignedStorageType = std::make_unsigned_t<typename S::StorageType>;

    constexpr QSpecialIntegerStorage() = default;
    constexpr QSpecialIntegerStorage(QSpecialIntegerBitfieldInitializer) : val(0) {}
    constexpr QSpecialIntegerStorage(UnsignedStorageType initial) : val(initial) {}

    UnsignedStorageType val;
};

template<class S, int pos, int width, class T = typename S::StorageType>
class QSpecialIntegerAccessor;

template<class S, int pos, int width, class T = typename S::StorageType>
class QSpecialIntegerConstAccessor
{
    Q_DISABLE_COPY_MOVE(QSpecialIntegerConstAccessor)
public:
    using Storage = const QSpecialIntegerStorage<S>;
    using Type = T;
    using UnsignedType = std::make_unsigned_t<T>;

    operator Type() const noexcept
    {
        if constexpr (std::is_signed_v<Type>) {
            UnsignedType i = S::fromSpecial(storage->val);
            i <<= (sizeof(Type) * 8) - width - pos;
            Type t = Type(i);
            t >>= (sizeof(Type) * 8) - width;
            return t;
        }
        return (S::fromSpecial(storage->val) & mask()) >> pos;
    }

    bool operator!() const noexcept { return !(storage->val & S::toSpecial(mask())); }

    static constexpr UnsignedType mask() noexcept
    {
        return ((UnsignedType(1) << width) - 1) << pos;
    }

private:
    template<class Storage, typename... Accessors>
    friend class QSpecialIntegerBitfieldUnion;
    friend class QSpecialIntegerAccessor<S, pos, width, T>;

    explicit QSpecialIntegerConstAccessor(Storage *storage) : storage(storage) {}

    friend bool operator==(const QSpecialIntegerConstAccessor<S, pos, width, T> &i,
                           const QSpecialIntegerConstAccessor<S, pos, width, T> &j) noexcept
    {
        return ((i.storage->val ^ j.storage->val) & S::toSpecial(mask())) == 0;
    }

    friend bool operator!=(const QSpecialIntegerConstAccessor<S, pos, width, T> &i,
                           const QSpecialIntegerConstAccessor<S, pos, width, T> &j) noexcept
    {
        return ((i.storage->val ^ j.storage->val) & S::toSpecial(mask())) != 0;
    }

    Storage *storage;
};

template<class S, int pos, int width, class T>
class QSpecialIntegerAccessor
{
    Q_DISABLE_COPY_MOVE(QSpecialIntegerAccessor)
public:
    using Const = QSpecialIntegerConstAccessor<S, pos, width, T>;
    using Storage = QSpecialIntegerStorage<S>;
    using Type = T;
    using UnsignedType = std::make_unsigned_t<T>;

    QSpecialIntegerAccessor &operator=(Type t)
    {
        UnsignedType i = S::fromSpecial(storage->val);
        i &= ~Const::mask();
        i |= (UnsignedType(t) << pos) & Const::mask();
        storage->val = S::toSpecial(i);
        return *this;
    }

    operator Const() { return Const(storage); }

private:
    template<class Storage, typename... Accessors>
    friend class QSpecialIntegerBitfieldUnion;

    explicit QSpecialIntegerAccessor(Storage *storage) : storage(storage) {}

    Storage *storage;
};

template<class S, typename... Accessors>
class QSpecialIntegerBitfieldUnion
{
public:
    constexpr QSpecialIntegerBitfieldUnion() = default;
    constexpr QSpecialIntegerBitfieldUnion(QSpecialIntegerBitfieldInitializer initial)
        : storage(initial)
    {}

    constexpr QSpecialIntegerBitfieldUnion(
            typename QSpecialIntegerStorage<S>::UnsignedStorageType initial)
        : storage(initial)
    {}

    template<typename A>
    void set(typename A::Type value)
    {
        member<A>() = value;
    }

    template<typename A>
    typename A::Type get() const
    {
        return member<A>();
    }

    typename QSpecialIntegerStorage<S>::UnsignedStorageType data() const
    {
        return storage.val;
    }

private:
    template<typename A>
    static constexpr bool isAccessor = std::disjunction_v<std::is_same<A, Accessors>...>;

    template<typename A>
    A member()
    {
        static_assert(isAccessor<A>);
        return A(&storage);
    }

    template<typename A>
    typename A::Const member() const
    {
        static_assert(isAccessor<A>);
        return typename A::Const(&storage);
    }

    QSpecialIntegerStorage<S> storage;
};

template<typename T, typename... Accessors>
using QLEIntegerBitfieldUnion
        = QSpecialIntegerBitfieldUnion<QLittleEndianStorageType<T>, Accessors...>;

template<typename T, typename... Accessors>
using QBEIntegerBitfieldUnion
        = QSpecialIntegerBitfieldUnion<QBigEndianStorageType<T>, Accessors...>;

template<typename... Accessors>
using qint32_le_bitfield_union = QLEIntegerBitfieldUnion<int, Accessors...>;
template<typename... Accessors>
using quint32_le_bitfield_union = QLEIntegerBitfieldUnion<uint, Accessors...>;
template<typename... Accessors>
using qint32_be_bitfield_union = QBEIntegerBitfieldUnion<int, Accessors...>;
template<typename... Accessors>
using quint32_be_bitfield_union = QBEIntegerBitfieldUnion<uint, Accessors...>;

template<int pos, int width, typename T = int>
using qint32_le_bitfield_member
        = QSpecialIntegerAccessor<QLittleEndianStorageType<int>, pos, width, T>;
template<int pos, int width, typename T = uint>
using quint32_le_bitfield_member
        = QSpecialIntegerAccessor<QLittleEndianStorageType<uint>, pos, width, T>;
template<int pos, int width, typename T = int>
using qint32_be_bitfield_member
        = QSpecialIntegerAccessor<QBigEndianStorageType<int>, pos, width, T>;
template<int pos, int width, typename T = uint>
using quint32_be_bitfield_member
        = QSpecialIntegerAccessor<QBigEndianStorageType<uint>, pos, width, T>;

QT_END_NAMESPACE

#endif // QENDIAN_P_H
