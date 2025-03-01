// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMPTR_P_H
#define QCOMPTR_P_H

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

#include <QtCore/private/qglobal_p.h>
#include <QtCore/qtconfigmacros.h>
#include <type_traits>
#if defined(Q_OS_WIN) || defined(Q_QDOC)

// clang-format off
#include <QtCore/qt_windows.h>
#include <wrl/client.h>
// clang-format on

QT_BEGIN_NAMESPACE

namespace Detail {

template <typename, typename, typename = void>
struct has_equal_to_operator : std::false_type
{
};

template <typename T, typename U>
struct has_equal_to_operator<
        T, U, std::void_t<decltype(operator==(std::declval<T>(), std::declval<U>()))>>
    : std::true_type
{
};

template <typename T, typename U>
using EnableIfMissingEqualToOperator = std::enable_if_t<!has_equal_to_operator<T, U>::value, bool>;

template <typename, typename, typename = void>
struct has_not_equal_to_operator : std::false_type
{
};

template <typename T, typename U>
struct has_not_equal_to_operator<
        T, U, std::void_t<decltype(operator!=(std::declval<T>(), std::declval<U>()))>>
    : std::true_type
{
};

template <typename T, typename U>
using EnableIfMissingNotEqualToOperator =
        std::enable_if_t<!has_not_equal_to_operator<T, U>::value, bool>;

template <typename, typename, typename = void>
struct has_less_than_operator : std::false_type
{
};

template <typename T, typename U>
struct has_less_than_operator<
        T, U, std::void_t<decltype(operator<(std::declval<T>(), std::declval<U>()))>>
    : std::true_type
{
};

template <typename T, typename U>
using EnableIfMissingLessThanOperator =
        std::enable_if_t<!has_less_than_operator<T, U>::value, bool>;

} // namespace Detail

QT_END_NAMESPACE

namespace Microsoft {
namespace WRL {

// Add missing comparison operators if MINGW does not provide them

template <typename T, typename U,
          QT_PREPEND_NAMESPACE(Detail)::EnableIfMissingEqualToOperator<ComPtr<T>, ComPtr<U>> = true>
bool operator==(const ComPtr<T> &lhs, const ComPtr<U> &rhs) noexcept
{
    static_assert(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>);
    return lhs.Get() == rhs.Get();
}

template <typename T,
          QT_PREPEND_NAMESPACE(Detail)::EnableIfMissingEqualToOperator<ComPtr<T>, std::nullptr_t> =
                  true>
bool operator==(const ComPtr<T> &lhs, std::nullptr_t) noexcept
{
    return lhs.Get() == nullptr;
}

template <typename T,
          QT_PREPEND_NAMESPACE(Detail)::EnableIfMissingEqualToOperator<std::nullptr_t, ComPtr<T>> =
                  true>
bool operator==(std::nullptr_t, const ComPtr<T> &rhs) noexcept
{
    return rhs.Get() == nullptr;
}

template <typename T, typename U,
          QT_PREPEND_NAMESPACE(Detail)::EnableIfMissingNotEqualToOperator<ComPtr<T>, ComPtr<U>> =
                  true>
bool operator!=(const ComPtr<T> &a, const ComPtr<U> &b) noexcept
{
    static_assert(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>);
    return a.Get() != b.Get();
}

template <class T,
          QT_PREPEND_NAMESPACE(
                  Detail)::EnableIfMissingNotEqualToOperator<ComPtr<T>, std::nullptr_t> = true>
bool operator!=(const ComPtr<T> &a, std::nullptr_t) noexcept
{
    return a.Get() != nullptr;
}

template <class T,
          QT_PREPEND_NAMESPACE(
                  Detail)::EnableIfMissingNotEqualToOperator<std::nullptr_t, ComPtr<T>> = true>
bool operator!=(std::nullptr_t, const ComPtr<T> &a) noexcept
{
    return a.Get() != nullptr;
}

// MSVC WRL only defines operator<, we do not add other variants such as <=, > or >=
template <
        class T, class U,
        QT_PREPEND_NAMESPACE(Detail)::EnableIfMissingLessThanOperator<ComPtr<T>, ComPtr<U>> = true>
bool operator<(const ComPtr<T> &a, const ComPtr<U> &b) noexcept
{
    static_assert(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>);
    return a.Get() < b.Get();
}

} // namespace WRL
} // namespace Microsoft

QT_BEGIN_NAMESPACE

using Microsoft::WRL::ComPtr;

template <typename T, typename... Args>
ComPtr<T> makeComObject(Args &&...args)
{
    ComPtr<T> p;
    // Don't use Attach because of MINGW64 bug
    // #892 Microsoft::WRL::ComPtr::Attach leaks references
    *p.GetAddressOf() = new T(std::forward<Args>(args)...);
    return p;
}

QT_END_NAMESPACE

#endif // Q_OS_WIN

#endif // QCOMPTR_P_H
