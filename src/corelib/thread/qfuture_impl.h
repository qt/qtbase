/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef QFUTURE_H
#error Do not include qfuture_impl.h directly
#endif

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#include <QtCore/qglobal.h>
#include <QtCore/qfutureinterface.h>
#include <QtCore/qthreadpool.h>

QT_BEGIN_NAMESPACE

//
// forward declarations
//
template<class T>
class QFuture;
template<class T>
class QFutureInterface;

namespace QtFuture {
enum class Launch { Sync, Async, Inherit };
}

namespace QtPrivate {

template<class T>
using EnableForVoid = std::enable_if_t<std::is_same_v<T, void>>;

template<class T>
using EnableForNonVoid = std::enable_if_t<!std::is_same_v<T, void>>;

template<typename F, typename Arg, typename Enable = void>
struct ResultTypeHelper
{
};

// The callable takes an argument of type Arg
template<typename F, typename Arg>
struct ResultTypeHelper<
        F, Arg, typename std::enable_if_t<!std::is_invocable_v<std::decay_t<F>, QFuture<Arg>>>>
{
    using ResultType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Arg>>;
};

// The callable takes an argument of type QFuture<Arg>
template<class F, class Arg>
struct ResultTypeHelper<
        F, Arg, typename std::enable_if_t<std::is_invocable_v<std::decay_t<F>, QFuture<Arg>>>>
{
    using ResultType = std::invoke_result_t<std::decay_t<F>, QFuture<Arg>>;
};

// The callable takes an argument of type QFuture<void>
template<class F>
struct ResultTypeHelper<
        F, void, typename std::enable_if_t<std::is_invocable_v<std::decay_t<F>, QFuture<void>>>>
{
    using ResultType = std::invoke_result_t<std::decay_t<F>, QFuture<void>>;
};

// The callable doesn't take argument
template<class F>
struct ResultTypeHelper<
        F, void, typename std::enable_if_t<!std::is_invocable_v<std::decay_t<F>, QFuture<void>>>>
{
    using ResultType = std::invoke_result_t<std::decay_t<F>>;
};

template<typename Function, typename ResultType, typename ParentResultType>
class Continuation
{
public:
    Continuation(Function &&func, const QFuture<ParentResultType> &f,
                 const QFutureInterface<ResultType> &p)
        : promise(p), parentFuture(f), function(std::forward<Function>(func))
    {
    }
    virtual ~Continuation() = default;

    bool execute();

    static void create(Function &&func, QFuture<ParentResultType> *f,
                       QFutureInterface<ResultType> &p, QtFuture::Launch policy);

    static void create(Function &&func, QFuture<ParentResultType> *f,
                       QFutureInterface<ResultType> &p, QThreadPool *pool);

protected:
    virtual void runImpl() = 0;

    void runFunction();

protected:
    QFutureInterface<ResultType> promise;
    const QFuture<ParentResultType> parentFuture;
    Function function;
};

template<typename Function, typename ResultType, typename ParentResultType>
class SyncContinuation final : public Continuation<Function, ResultType, ParentResultType>
{
public:
    SyncContinuation(Function &&func, const QFuture<ParentResultType> &f,
                     const QFutureInterface<ResultType> &p)
        : Continuation<Function, ResultType, ParentResultType>(std::forward<Function>(func), f, p)
    {
    }

    ~SyncContinuation() override = default;

private:
    void runImpl() override { this->runFunction(); }
};

template<typename Function, typename ResultType, typename ParentResultType>
class AsyncContinuation final : public QRunnable,
                                public Continuation<Function, ResultType, ParentResultType>
{
public:
    AsyncContinuation(Function &&func, const QFuture<ParentResultType> &f,
                      const QFutureInterface<ResultType> &p, QThreadPool *pool = nullptr)
        : Continuation<Function, ResultType, ParentResultType>(std::forward<Function>(func), f, p),
          threadPool(pool)
    {
        this->promise.setRunnable(this);
    }

    ~AsyncContinuation() override = default;

private:
    void runImpl() override // from Continuation
    {
        QThreadPool *pool = threadPool ? threadPool : QThreadPool::globalInstance();
        pool->start(this);
    }

    void run() override // from QRunnable
    {
        this->runFunction();
    }

private:
    QThreadPool *threadPool;
};

template<typename Function, typename ResultType, typename ParentResultType>
void Continuation<Function, ResultType, ParentResultType>::runFunction()
{
    promise.reportStarted();

    Q_ASSERT(parentFuture.isFinished());

#ifndef QT_NO_EXCEPTIONS
    try {
#endif
        if constexpr (!std::is_void_v<ResultType>) {
            if constexpr (std::is_invocable_v<std::decay_t<Function>, QFuture<ParentResultType>>) {
                promise.reportResult(function(parentFuture));
            } else if constexpr (std::is_void_v<ParentResultType>) {
                promise.reportResult(function());
            } else {
                // This assert normally should never fail, this is to make sure
                // that nothing unexpected happend.
                static_assert(
                        std::is_invocable_v<std::decay_t<Function>, std::decay_t<ParentResultType>>,
                        "The continuation is not invocable with the provided arguments");

                promise.reportResult(function(parentFuture.result()));
            }
        } else {
            if constexpr (std::is_invocable_v<std::decay_t<Function>, QFuture<ParentResultType>>) {
                function(parentFuture);
            } else if constexpr (std::is_void_v<ParentResultType>) {
                function();
            } else {
                // This assert normally should never fail, this is to make sure
                // that nothing unexpected happend.
                static_assert(
                        std::is_invocable_v<std::decay_t<Function>, std::decay_t<ParentResultType>>,
                        "The continuation is not invocable with the provided arguments");

                function(parentFuture.result());
            }
        }
#ifndef QT_NO_EXCEPTIONS
    } catch (...) {
        promise.reportException(std::current_exception());
    }
#endif
    promise.reportFinished();
}

template<typename Function, typename ResultType, typename ParentResultType>
bool Continuation<Function, ResultType, ParentResultType>::execute()
{
    Q_ASSERT(parentFuture.isFinished());

    if (parentFuture.isCanceled()) {
#ifndef QT_NO_EXCEPTIONS
        if (parentFuture.d.exceptionStore().hasException()) {
            // If the continuation doesn't take a QFuture argument, propagate the exception
            // to the caller, by reporting it. If the continuation takes a QFuture argument,
            // the user may want to catch the exception inside the continuation, to not
            // interrupt the continuation chain, so don't report anything yet.
            if constexpr (!std::is_invocable_v<std::decay_t<Function>, QFuture<ParentResultType>>) {
                promise.reportStarted();
                promise.reportException(parentFuture.d.exceptionStore().exception());
                promise.reportFinished();
                return false;
            }
        } else
#endif
        {
            promise.reportStarted();
            promise.reportCanceled();
            promise.reportFinished();
            return false;
        }
    }

    runImpl();
    return true;
}

template<typename Function, typename ResultType, typename ParentResultType>
void Continuation<Function, ResultType, ParentResultType>::create(Function &&func,
                                                                  QFuture<ParentResultType> *f,
                                                                  QFutureInterface<ResultType> &p,
                                                                  QtFuture::Launch policy)
{
    Q_ASSERT(f);

    QThreadPool *pool = nullptr;

    bool launchAsync = (policy == QtFuture::Launch::Async);
    if (policy == QtFuture::Launch::Inherit) {
        launchAsync = f->d.launchAsync();

        // If the parent future was using a custom thread pool, inherit it as well.
        if (launchAsync && f->d.threadPool()) {
            pool = f->d.threadPool();
            p.setThreadPool(pool);
        }
    }

    Continuation<Function, ResultType, ParentResultType> *continuationJob = nullptr;
    if (launchAsync) {
        continuationJob = new AsyncContinuation<Function, ResultType, ParentResultType>(
                std::forward<Function>(func), *f, p, pool);
    } else {
        continuationJob = new SyncContinuation<Function, ResultType, ParentResultType>(
                std::forward<Function>(func), *f, p);
    }

    p.setLaunchAsync(launchAsync);

    auto continuation = [continuationJob, policy, launchAsync]() mutable {
        bool isLaunched = continuationJob->execute();
        // If continuation is successfully launched, AsyncContinuation will be deleted
        // by the QThreadPool which has started it. Synchronous continuation will be
        // executed immediately, so it's safe to always delete it here.
        if (!(launchAsync && isLaunched)) {
            delete continuationJob;
            continuationJob = nullptr;
        }
    };

    f->d.setContinuation(std::move(continuation));
}

template<typename Function, typename ResultType, typename ParentResultType>
void Continuation<Function, ResultType, ParentResultType>::create(Function &&func,
                                                                  QFuture<ParentResultType> *f,
                                                                  QFutureInterface<ResultType> &p,
                                                                  QThreadPool *pool)
{
    Q_ASSERT(f);

    auto continuationJob = new AsyncContinuation<Function, ResultType, ParentResultType>(
            std::forward<Function>(func), *f, p, pool);
    p.setLaunchAsync(true);
    p.setThreadPool(pool);

    auto continuation = [continuationJob]() mutable {
        bool isLaunched = continuationJob->execute();
        // If continuation is successfully launched, AsyncContinuation will be deleted
        // by the QThreadPool which has started it.
        if (!isLaunched) {
            delete continuationJob;
            continuationJob = nullptr;
        }
    };

    f->d.setContinuation(continuation);
}

} // namespace QtPrivate

QT_END_NAMESPACE
