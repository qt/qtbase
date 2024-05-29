// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTTYPETRAITS_H
#define QTTYPETRAITS_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtdeprecationmarkers.h>

#include <type_traits>
#include <utility>

#if 0
#pragma qt_class(QtTypeTraits)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

// like std::to_underlying
template <typename Enum>
constexpr std::underlying_type_t<Enum> qToUnderlying(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

#ifndef QT_NO_QASCONST
#if QT_DEPRECATED_SINCE(6, 6)

// this adds const to non-const objects (like std::as_const)
template <typename T>
QT_DEPRECATED_VERSION_X_6_6("Use std::as_const() instead.")
constexpr typename std::add_const<T>::type &qAsConst(T &t) noexcept { return t; }
// prevent rvalue arguments:
template <typename T>
void qAsConst(const T &&) = delete;

#endif // QT_DEPRECATED_SINCE(6, 6)
#endif // QT_NO_QASCONST

#ifndef QT_NO_QEXCHANGE

// like std::exchange
template <typename T, typename U = T>
constexpr T qExchange(T &t, U &&newValue)
noexcept(std::conjunction_v<std::is_nothrow_move_constructible<T>,
                            std::is_nothrow_assignable<T &, U>>)
{
    T old = std::move(t);
    t = std::forward<U>(newValue);
    return old;
}

#endif // QT_NO_QEXCHANGE

namespace QtPrivate {
// helper to be used to trigger a "dependent static_assert(false)"
// (for instance, in a final `else` branch of a `if constexpr`.)
template <typename T> struct type_dependent_false : std::false_type {};
template <auto T> struct value_dependent_false : std::false_type {};
}

namespace QTypeTraits {

namespace detail {
template<typename T, typename U,
         typename = std::enable_if_t<std::is_arithmetic_v<T> && std::is_arithmetic_v<U> &&
                                     std::is_floating_point_v<T> == std::is_floating_point_v<U> &&
                                     std::is_signed_v<T> == std::is_signed_v<U> &&
                                     !std::is_same_v<T, bool> && !std::is_same_v<U, bool> &&
                                     !std::is_same_v<T, char> && !std::is_same_v<U, char>>>
struct Promoted
{
    using type = decltype(T() + U());
};
}

template <typename T, typename U>
using Promoted = typename detail::Promoted<T, U>::type;

} // namespace QTypeTraits

QT_END_NAMESPACE

#endif // QTTYPETRAITS_H
