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
