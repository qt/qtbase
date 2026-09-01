// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QOVERLOAD_H
#define QOVERLOAD_H

#include <QtCore/qtconfigmacros.h>
#include <QtCore/qtpreprocessorsupport.h>

#if 0
#pragma qt_class(QOverload)
#endif

QT_BEGIN_NAMESPACE

#ifdef Q_QDOC
// Just for documentation generation
template<typename T>
auto qOverload(T functionPointer);
template<typename T>
auto qConstOverload(T memberFunctionPointer);
template<typename T>
auto qNonConstOverload(T memberFunctionPointer);
#else
template <typename... Args>
struct QNonConstOverload
{
    template <typename R, typename T>
    constexpr auto operator()(R (T::*ptr)(Args...)) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename T>
    static constexpr auto of(R (T::*ptr)(Args...)) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args>
struct QConstOverload
{
    template <typename R, typename T>
    constexpr auto operator()(R (T::*ptr)(Args...) const) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R, typename T>
    static constexpr auto of(R (T::*ptr)(Args...) const) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args>
struct QOverload : QConstOverload<Args...>, QNonConstOverload<Args...>
{
    using QConstOverload<Args...>::of;
    using QConstOverload<Args...>::operator();
    using QNonConstOverload<Args...>::of;
    using QNonConstOverload<Args...>::operator();

    template <typename R>
    constexpr auto operator()(R (*ptr)(Args...)) const noexcept -> decltype(ptr)
    { return ptr; }

    template <typename R>
    static constexpr auto of(R (*ptr)(Args...)) noexcept -> decltype(ptr)
    { return ptr; }
};

template <typename... Args> constexpr inline QOverload<Args...> qOverload = {};
template <typename... Args> constexpr inline QConstOverload<Args...> qConstOverload = {};
template <typename... Args> constexpr inline QNonConstOverload<Args...> qNonConstOverload = {};

#endif // Q_QDOC

QT_END_NAMESPACE

#endif /* QOVERLOAD_H */
