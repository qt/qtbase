/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtConcurrent module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

template <class Function, class ...Args>
[[nodiscard]]
auto run(QThreadPool *pool, Function &&f, Args &&...args)
{
    DecayedTuple<Function, Args...> tuple { std::forward<Function>(f),
                                            std::forward<Args>(args)... };
    return TaskResolver<std::decay_t<Function>, std::decay_t<Args>...>::run(
                std::move(tuple), TaskStartParameters { pool });
}

template <class Function, class ...Args>
[[nodiscard]]
auto run(QThreadPool *pool, std::reference_wrapper<const Function> &&functionWrapper,
         Args &&...args)
{
    return run(pool, std::forward<const Function>(functionWrapper.get()),
               std::forward<Args>(args)...);
}

template <class Function, class ...Args>
[[nodiscard]]
auto run(Function &&f, Args &&...args)
{
    return run(QThreadPool::globalInstance(), std::forward<Function>(f),
               std::forward<Args>(args)...);
}

// overload with a Promise Type hint, takes thread pool
template <class PromiseType, class Function, class ...Args>
[[nodiscard]]
auto run(QThreadPool *pool, Function &&f, Args &&...args)
{
    return (new StoredFunctionCallWithPromise<Function, PromiseType, Args...>(
                std::forward<Function>(f), std::forward<Args>(args)...))->start(pool);
}

// overload with a Promise Type hint, uses global thread pool
template <class PromiseType, class Function, class ...Args>
[[nodiscard]]
auto run(Function &&f, Args &&...args)
{
    return run<PromiseType>(QThreadPool::globalInstance(), std::forward<Function>(f),
                            std::forward<Args>(args)...);
}

} //namespace QtConcurrent

#endif // Q_CLANG_QDOC

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
