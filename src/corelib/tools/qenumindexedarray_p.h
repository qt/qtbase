// Copyright (C) 2026 The Qt Company Ltd.
// Copyright (C) 2021 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QENUMINDEXEDARRAY_P_H
#define QENUMINDEXEDARRAY_P_H

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

#include <QtCore/qtclasshelpermacros.h>
#include <QtCore/qttypetraits.h>
#include <QtCore/private/qglobal_p.h>

#include <array>
#include <QtCore/q20utility.h>
#include <QtCore/q20type_traits.h>

QT_BEGIN_NAMESPACE

QT_ENABLE_P0846_SEMANTICS_FOR(get)

/*
    This is a variant of `std::array`, differing in that it uses a custom
    type to index into the array.

    The class is useful as a container indexed by some enumeration type,
    since, apart from the value() method, it doesn't allow passing
    non-enum types as indexes to its functions.

    There are two modes to use this class: QEnumIndexedArray takes the value
    type, the enum type, and the size as three separate template arguments.
    For convenience, the QEnumIndexedArrayFor alias deduces both the
    enum-type and the maximum value with a single template parameter.

    A simple example using Qt:

    struct CursorData {
        ~~~~
    };
    constexpr auto NumQtCursorShapes = Qt::CursorShape(Qt::CustomCursor + 1);
    using CursorIndexedArray = QEnumIndexedArrayFor<
            std::unique_ptr<CursorData>,
            NumQtCursorShapes
        >;
    CursorIndexedArray cursorData;

    ~~~~
    auto& cross = cursorData[Qt::CrossCursor]; // OK
    auto& ibeam = cursorData[4]; // Qt::IBeamCursor; ERROR: no op[] taking int

    For best results, the enumeration should be zero-based, contiguous, and
    closed.
*/

class QEnumIndexedArrayBase
{
protected:
    QEnumIndexedArrayBase() = default;
    QT_DECLARE_RO5_SMF_AS_DEFAULTED(QEnumIndexedArrayBase)

    /* not constexpr! */ static void unsorted()
    { Q_ASSERT_X(false, "QEnumIndexedArray", "initializer is not sorted"); }
};

template <typename T, typename E, std::size_t N>
struct QEnumIndexedArray : private QEnumIndexedArrayBase
{
private:
    // re-export the template arguments (T is value_type below):
    using Enum = E;
    static constexpr inline std::size_t Size = N;

    static constexpr auto map(Enum e) noexcept { return static_cast<std::size_t>(e); }
    using Base = std::array<T, Size>;

    template <typename S>
    using if_cvref_self = std::enable_if_t<
            std::is_same_v<q20::remove_cvref_t<S>, QEnumIndexedArray>
        , bool>;

    template <std::size_t...Is>
    static constexpr Base extract2nd(std::array<std::pair<Enum, T>, Size> &&init,
                                     std::index_sequence<Is...>)
    {
        if (!((map(init[Is].first) == Is) && ...))
            QEnumIndexedArrayBase::unsorted();
        return Base{std::move(init[Is].second)...};
    }

protected: // like std::stack
    Base c;

public:
    using value_type = typename Base::value_type;
    using size_type = typename Base::size_type;
    using index_type = Enum; // new, not on std::array
    using difference_type = typename Base::difference_type;
    using reference = typename Base::reference;
    using const_reference = typename Base::const_reference;
    using pointer = typename Base::pointer;
    using const_pointer = typename Base::const_pointer;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using reverse_iterator = typename Base::reverse_iterator;
    using const_reverse_iterator = typename Base::const_reverse_iterator;

    QEnumIndexedArray() = default;
    Q_IMPLICIT constexpr QEnumIndexedArray(std::array<T, Size> init)
            noexcept(std::is_nothrow_move_constructible_v<T>)
        : c{std::move(init)}
    {}
    Q_IMPLICIT constexpr QEnumIndexedArray(std::array<std::pair<Enum, T>, Size> init)
            noexcept(std::is_nothrow_move_constructible_v<T>)
        : c(extract2nd(std::move(init), std::make_index_sequence<Size>{}))
    {}

    constexpr decltype(auto) value(size_type i) { return c[i]; } // new, not on std::array
    constexpr decltype(auto) value(size_type i) const { return c[i]; } // new, not on std::array
    constexpr decltype(auto) at(Enum i) { return c.at(map(i)); }
    constexpr decltype(auto) at(Enum i) const { return c.at(map(i)); }
    constexpr decltype(auto) operator[](Enum i) { return c[map(i)]; }
    constexpr decltype(auto) operator[](Enum i) const { return c[map(i)]; }

#define QT_FWD_IMPL_0(name, cv) \
    constexpr decltype(auto) name () cv & \
        noexcept(noexcept(c. name ())) \
    { return c. name (); } \
    constexpr decltype(auto) name () cv && \
        noexcept(noexcept(std::move(c). name ())) \
    { return std::move(c). name (); } \
    /*end*/
#define QT_FWD_M_0(name) QT_FWD_IMPL_0(name, )
#define QT_FWD_C_0(name) QT_FWD_IMPL_0(name, const)
#define QT_FWD_0(name) \
    QT_FWD_M_0(name) \
    QT_FWD_C_0(name) \
    /*end*/

    QT_FWD_0(back)
    QT_FWD_0(data)
    QT_FWD_0(front)

    QT_FWD_0(begin)
    QT_FWD_C_0(cbegin)
    QT_FWD_C_0(cend)
    QT_FWD_C_0(crbegin)
    QT_FWD_C_0(crend)
    QT_FWD_0(end)
    QT_FWD_0(rbegin)
    QT_FWD_0(rend)

    QT_FWD_C_0(empty)
    QT_FWD_C_0(max_size)
    QT_FWD_C_0(size)

#undef QT_FWD_0
#undef QT_FWD_C_0
#undef QT_FWD_M_0
#undef QT_FWD_IMPL_0

    constexpr void fill(const value_type& val)
        noexcept(noexcept(c.fill(val)))
    { c.fill(val); }

    constexpr void swap(QEnumIndexedArray &other)
        noexcept(noexcept(c.swap(other.c)))
    { c.swap(other.c); }

private:
    friend constexpr void swap(QEnumIndexedArray &lhs, QEnumIndexedArray &rhs)
        noexcept(noexcept(lhs.swap(rhs)))
    { lhs.swap(rhs); }

#define Q_REL_OP(op) \
    friend constexpr auto operator op(const QEnumIndexedArray &lhs, const QEnumIndexedArray &rhs) \
        noexcept(noexcept(lhs.c op rhs.c)) /* copy from implementation */ \
    { return lhs.c op rhs.c; }

    Q_REL_OP(==)
# ifndef __cpp_impl_three_way_comparison
    Q_REL_OP(!=)
# endif

# ifdef __cpp_lib_three_way_comparison
    Q_REL_OP(<=>)
# else
    Q_REL_OP(<)
    Q_REL_OP(>)
    Q_REL_OP(<=)
    Q_REL_OP(>=)
# endif
#undef Q_REL_OP

    template <std::size_t I, typename EIA,
              std::enable_if_t<I < q20::remove_cvref_t<EIA>::Size, bool> = true,
              if_cvref_self<EIA> = true>
    friend constexpr decltype(auto) get(EIA &&e) noexcept
    {
        using std::get;
        return get<I>(std::forward<EIA>(e).c);
    }
};

template <typename T, auto MAX>
using QEnumIndexedArrayFor = QEnumIndexedArray<T, decltype(MAX), qToUnderlying(MAX)>;

QT_END_NAMESPACE

// Tuple Protocol
namespace std {
    template <typename T, typename Enum, std::size_t N>
    class tuple_size<QT_PREPEND_NAMESPACE(QEnumIndexedArray)<T, Enum, N>>
        : public std::integral_constant<size_t, N> {};
    template <std::size_t I, typename T, typename Enum, std::size_t N>
    class tuple_element<I, QT_PREPEND_NAMESPACE(QEnumIndexedArray)<T, Enum, N>>
    { public: using type = T; };
}

#endif // QENUMINDEXEDARRAY_P_H
