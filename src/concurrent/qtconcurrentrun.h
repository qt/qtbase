/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtConcurrent module of the Qt Toolkit.
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

#ifndef QTCONCURRENT_RUN_H
#define QTCONCURRENT_RUN_H

#include <QtConcurrent/qtconcurrentcompilertest.h>

#if !defined(QT_NO_CONCURRENT) || defined(Q_CLANG_QDOC)

#include <QtConcurrent/qtconcurrentrunbase.h>
#include <QtConcurrent/qtconcurrentstoredfunctioncall.h>

QT_BEGIN_NAMESPACE

#ifdef Q_CLANG_QDOC

typedef int Function;

namespace QtConcurrent {

    template <typename T>
    QFuture<T> run(Function function, ...);

    template <typename T>
    QFuture<T> run(QThreadPool *pool, Function function, ...);

    template <typename T>
    QFuture<T> runWithPromise(Function function, ...);

    template <typename T>
    QFuture<T> runWithPromise(QThreadPool *pool, Function function, ...);

} // namespace QtConcurrent

#else

namespace QtConcurrent {

// Note: It's a copy taken from qfuture_impl.h with some specialization added
// TODO: Get rid of the code repetition and unify this for both purposes, see QTBUG-83331
template<typename...>
struct ArgsType;

template<typename Arg, typename... Args>
struct ArgsType<Arg, Args...>
{
    using PromiseType = void;
    static const bool IsPromise = false;
};

// Note: this specialization was added
template<typename Arg, typename... Args>
struct ArgsType<QPromise<Arg> &, Args...>
{
    using PromiseType = Arg;
    static const bool IsPromise = true;
};

template<>
struct ArgsType<>
{
    using PromiseType = void;
    static const bool IsPromise = false;
};

template<typename F>
struct ArgResolver : ArgResolver<decltype(&std::decay_t<F>::operator())>
{
};

// Note: this specialization was added, see callableObjectWithState() test in qtconcurrentrun
template<typename F>
struct ArgResolver<std::reference_wrapper<F>> : ArgResolver<decltype(&std::decay_t<F>::operator())>
{
};

template<typename R, typename... Args>
struct ArgResolver<R(Args...)> : public ArgsType<Args...>
{
};

template<typename R, typename... Args>
struct ArgResolver<R (*)(Args...)> : public ArgsType<Args...>
{
};

// Note: this specialization was added, see light() test in qtconcurrentrun
template<typename R, typename... Args>
struct ArgResolver<R (*&)(Args...)> : public ArgsType<Args...>
{
};

// Note: this specialization was added, see light() test in qtconcurrentrun
template<typename R, typename... Args>
struct ArgResolver<R (* const)(Args...)> : public ArgsType<Args...>
{
};

template<typename R, typename... Args>
struct ArgResolver<R (&)(Args...)> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::*)(Args...)> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::*)(Args...) noexcept> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::*)(Args...) const> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::*)(Args...) const noexcept> : public ArgsType<Args...>
{
};

// Note: this specialization was added, see crefFunction() test in qtconcurrentrun
template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::* const)(Args...) const> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::* const)(Args...) const noexcept> : public ArgsType<Args...>
{
};

template <class Function, class ...Args>
[[nodiscard]]
auto run(QThreadPool *pool, Function &&f, Args &&...args)
{
    return (new StoredFunctionCall<Function, Args...>(
                std::forward<Function>(f), std::forward<Args>(args)...))->start(pool);
}

template <class Function, class ...Args>
[[nodiscard]]
auto run(Function &&f, Args &&...args)
{
    return run(QThreadPool::globalInstance(), std::forward<Function>(f), std::forward<Args>(args)...);
}

template <class PromiseType, class Function, class ...Args>
[[nodiscard]]
auto runWithPromise(QThreadPool *pool, Function &&f, Args &&...args)
{
    return (new StoredFunctionCallWithPromise<Function, PromiseType, Args...>(
                std::forward<Function>(f), std::forward<Args>(args)...))->start(pool);
}

template <class Function, class ...Args>
[[nodiscard]]
auto runWithPromise(QThreadPool *pool, Function &&f, Args &&...args)
{
    static_assert(ArgResolver<Function>::IsPromise, "The first argument of passed callable object isn't a QPromise<T> & type.");
    using PromiseType = typename ArgResolver<Function>::PromiseType;
    return runWithPromise<PromiseType>(pool, std::forward<Function>(f), std::forward<Args>(args)...);
}

template <class Function, class ...Args>
[[nodiscard]]
auto runWithPromise(QThreadPool *pool, std::reference_wrapper<const Function> &&functionWrapper, Args &&...args)
{
    static_assert(ArgResolver<const Function>::IsPromise, "The first argument of passed callable object isn't a QPromise<T> & type.");
    using PromiseType = typename ArgResolver<const Function>::PromiseType;
    return runWithPromise<PromiseType>(pool, std::forward<const Function>(functionWrapper.get()), std::forward<Args>(args)...);
}

template <class PromiseType, class Function, class ...Args>
[[nodiscard]]
auto runWithPromise(Function &&f, Args &&...args)
{
    return runWithPromise<PromiseType>(QThreadPool::globalInstance(), std::forward<Function>(f), std::forward<Args>(args)...);
}

template <class Function, class ...Args>
[[nodiscard]]
auto runWithPromise(Function &&f, Args &&...args)
{
    return runWithPromise(QThreadPool::globalInstance(), std::forward<Function>(f), std::forward<Args>(args)...);
}

} //namespace QtConcurrent

#endif // Q_CLANG_QDOC

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
