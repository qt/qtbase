/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QTBASE_QTTASKBUILDER_H
#define QTBASE_QTTASKBUILDER_H

#if !defined(QT_NO_CONCURRENT) || defined(Q_CLANG_QDOC)

#include <QtConcurrent/qtconcurrentstoredfunctioncall.h>

QT_BEGIN_NAMESPACE

#ifdef Q_CLANG_QDOC

namespace QtConcurrent {

enum class FutureResult { Ignore };

using InvokeResultType = int;

template <class Task, class ...Args>
class QTaskBuilder
{
public:
    [[nodiscard]]
    QFuture<InvokeResultType> spawn();

    void spawn(FutureResult);

    template <class ...ExtraArgs>
    [[nodiscard]]
    QTaskBuilder<Task, ExtraArgs...> withArguments(ExtraArgs &&...args);

    [[nodiscard]]
    QTaskBuilder<Task, Args...> &onThreadPool(QThreadPool &newThreadPool);

    [[nodiscard]]
    QTaskBuilder<Task, Args...> &withPriority(int newPriority);
};

} // namespace QtConcurrent

#else

namespace QtConcurrent {

enum class FutureResult { Ignore };

template <class Task, class ...Args>
class QTaskBuilder
{
public:
    [[nodiscard]]
    auto spawn()
    {
        return TaskResolver<std::decay_t<Task>, std::decay_t<Args>...>::run(
                    std::move(taskWithArgs), startParameters);
    }

    // We don't want to run with promise when we don't return a QFuture
    void spawn(FutureResult)
    {
        (new StoredFunctionCall<Task, Args...>(std::move(taskWithArgs)))
            ->start(startParameters);
    }

    template <class ...ExtraArgs>
    [[nodiscard]]
    constexpr auto withArguments(ExtraArgs &&...args)
    {
        static_assert(std::tuple_size_v<TaskWithArgs> == 1,
                      "This function cannot be invoked if "
                      "arguments have already been passed.");

        static_assert(sizeof...(ExtraArgs) >= 1,
                      "One or more arguments must be passed.");

        // We have to re-create a builder, because the type has changed
        return QTaskBuilder<Task, ExtraArgs...>(
                   startParameters,
                   std::get<0>(std::move(taskWithArgs)),
                   std::forward<ExtraArgs>(args)...
               );
    }

    [[nodiscard]]
    constexpr auto &onThreadPool(QThreadPool &newThreadPool)
    {
        startParameters.threadPool = &newThreadPool;
        return *this;
    }

    [[nodiscard]]
    constexpr auto &withPriority(int newPriority)
    {
        startParameters.priority = newPriority;
        return *this;
    }

protected: // Methods
    constexpr explicit QTaskBuilder(Task &&task, Args &&...arguments)
        : taskWithArgs{std::forward<Task>(task), std::forward<Args>(arguments)...}
    {}

    constexpr QTaskBuilder(
        const TaskStartParameters &parameters, Task &&task, Args &&...arguments)
        : taskWithArgs{std::forward<Task>(task), std::forward<Args>(arguments)...}
        , startParameters{parameters}
    {}

private: // Methods
    // Required for creating a builder from "task" function
    template <class T>
    friend constexpr auto task(T &&t);

    // Required for creating a new builder from "withArguments" function
    template <class T, class ...A>
    friend class QTaskBuilder;

private: // Data
    using TaskWithArgs = DecayedTuple<Task, Args...>;

    TaskWithArgs taskWithArgs;
    TaskStartParameters startParameters;
};

} // namespace QtConcurrent

#endif // Q_CLANG_QDOC

QT_END_NAMESPACE

#endif // !defined(QT_NO_CONCURRENT)

#endif //QTBASE_QTTASKBUILDER_H
