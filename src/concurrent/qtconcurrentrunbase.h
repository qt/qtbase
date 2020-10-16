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

#ifndef QTCONCURRENT_RUNBASE_H
#define QTCONCURRENT_RUNBASE_H

#include <QtConcurrent/qtconcurrent_global.h>

#ifndef QT_NO_CONCURRENT

#include <QtCore/qfuture.h>
#include <QtCore/qrunnable.h>
#include <QtCore/qthreadpool.h>

#include <type_traits>
#include <utility>

QT_BEGIN_NAMESPACE


#ifndef Q_QDOC

namespace QtConcurrent {

template <typename T>
struct SelectSpecialization
{
    template <class Normal, class Void>
    struct Type { typedef Normal type; };
};

template <>
struct SelectSpecialization<void>
{
    template <class Normal, class Void>
    struct Type { typedef Void type; };
};

struct TaskStartParameters
{
    QThreadPool *threadPool = QThreadPool::globalInstance();
    int priority = 0;
};

template <typename T>
class RunFunctionTaskBase : public QRunnable
{
public:
    QFuture<T> start()
    {
        return start(TaskStartParameters());
    }

    QFuture<T> start(const TaskStartParameters &parameters)
    {
        promise.setThreadPool(parameters.threadPool);
        promise.setRunnable(this);
        promise.reportStarted();
        QFuture<T> theFuture = promise.future();
        parameters.threadPool->start(this, parameters.priority);
        return theFuture;
    }

    // For backward compatibility
    QFuture<T> start(QThreadPool *pool) { return start({pool, 0});  }

    void run() override
    {
        if (promise.isCanceled()) {
            promise.reportFinished();
            return;
        }
#ifndef QT_NO_EXCEPTIONS
        try {
#endif
            runFunctor();
#ifndef QT_NO_EXCEPTIONS
        } catch (QException &e) {
            promise.reportException(e);
        } catch (...) {
            promise.reportException(QUnhandledException(std::current_exception()));
        }
#endif

        reportResult();

        promise.reportFinished();
    }

protected:
    virtual void runFunctor() = 0;
    virtual void reportResult() {}

    QFutureInterface<T> promise;
};

template <typename T>
class RunFunctionTask : public RunFunctionTaskBase<T>
{
protected:
    void reportResult() override
    {
        if constexpr (std::is_move_constructible_v<T>)
            this->promise.reportAndMoveResult(std::move(result));
        else if constexpr (std::is_copy_constructible_v<T>)
            this->promise.reportResult(result);
    }

    T result;
};

template <>
class RunFunctionTask<void> : public RunFunctionTaskBase<void> {};

} //namespace QtConcurrent

#endif //Q_QDOC

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
