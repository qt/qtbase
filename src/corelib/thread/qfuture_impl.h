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
#include <QtCore/qexception.h>
#include <QtCore/qpointer.h>
#include <QtCore/qpromise.h>

QT_BEGIN_NAMESPACE

//
// forward declarations
//
template<class T>
class QFuture;
template<class T>
class QFutureInterface;
template<class T>
class QPromise;

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

// Helpers to remove QPrivateSignal argument from the list of arguments

template<class T, class Enable = void>
inline constexpr bool IsPrivateSignalArg = false;

template<class T>
inline constexpr bool IsPrivateSignalArg<T, typename std::enable_if_t<
        // finds injected-class-name, the 'class' avoids falling into the rules of [class.qual]/2:
        std::is_class_v<class T::QPrivateSignal>
    >> = true;

template<class Tuple, std::size_t... I>
auto cutTuple(Tuple &&t, std::index_sequence<I...>)
{
    return std::make_tuple(std::get<I>(t)...);
}

template<class Arg, class... Args>
auto createTuple(Arg &&arg, Args &&... args)
{
    using TupleType = std::tuple<std::decay_t<Arg>, std::decay_t<Args>...>;
    constexpr auto Size = sizeof...(Args); // One less than the size of all arguments
    if constexpr (QtPrivate::IsPrivateSignalArg<std::tuple_element_t<Size, TupleType>>) {
        if constexpr (Size == 1) {
            return arg;
        } else {
            return cutTuple(std::make_tuple(std::forward<Arg>(arg), std::forward<Args>(args)...),
                            std::make_index_sequence<Size>());
        }
    } else {
        return std::make_tuple(std::forward<Arg>(arg), std::forward<Args>(args)...);
    }
}

// Helpers to resolve argument types of callables.

template<class Arg, class... Args>
using FilterLastPrivateSignalArg =
        std::conditional_t<(sizeof...(Args) > 0),
                           std::invoke_result_t<decltype(createTuple<Arg, Args...>), Arg, Args...>,
                           std::conditional_t<IsPrivateSignalArg<Arg>, void, Arg>>;

template<typename...>
struct ArgsType;

template<typename Arg, typename... Args>
struct ArgsType<Arg, Args...>
{
    using First = Arg;
    using PromiseType = void;
    using IsPromise = std::false_type;
    static const bool HasExtraArgs = (sizeof...(Args) > 0);
    using AllArgs = FilterLastPrivateSignalArg<std::decay_t<Arg>, std::decay_t<Args>...>;

    template<class Class, class Callable>
    static const bool CanInvokeWithArgs = std::is_invocable_v<Callable, Class, Arg, Args...>;
};

template<typename Arg, typename... Args>
struct ArgsType<QPromise<Arg> &, Args...>
{
    using First = QPromise<Arg> &;
    using PromiseType = Arg;
    using IsPromise = std::true_type;
    static const bool HasExtraArgs = (sizeof...(Args) > 0);
    using AllArgs = FilterLastPrivateSignalArg<QPromise<Arg>, std::decay_t<Args>...>;

    template<class Class, class Callable>
    static const bool CanInvokeWithArgs = std::is_invocable_v<Callable, Class, QPromise<Arg> &, Args...>;
};

template<>
struct ArgsType<>
{
    using First = void;
    using PromiseType = void;
    using IsPromise = std::false_type;
    static const bool HasExtraArgs = false;
    using AllArgs = void;

    template<class Class, class Callable>
    static const bool CanInvokeWithArgs = std::is_invocable_v<Callable, Class>;
};

template<typename F>
struct ArgResolver : ArgResolver<decltype(&std::decay_t<F>::operator())>
{
};

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

template<typename R, typename... Args>
struct ArgResolver<R (*&)(Args...)> : public ArgsType<Args...>
{
};

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

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::* const)(Args...) const> : public ArgsType<Args...>
{
};

template<typename Class, typename R, typename... Args>
struct ArgResolver<R (Class::* const)(Args...) const noexcept> : public ArgsType<Args...>
{
};

template<class Class, class Callable>
using EnableIfInvocable = std::enable_if_t<
        QtPrivate::ArgResolver<Callable>::template CanInvokeWithArgs<Class, Callable>>;

template<typename Function, typename ResultType, typename ParentResultType>
class Continuation
{
public:
    template<typename F = Function>
    Continuation(F &&func, const QFuture<ParentResultType> &f, QPromise<ResultType> &&p)
        : promise(std::move(p)), parentFuture(f), function(std::forward<F>(func))
    {
    }
    virtual ~Continuation() = default;

    bool execute();

    template<typename F = Function>
    static void create(F &&func, QFuture<ParentResultType> *f, QFutureInterface<ResultType> &fi,
                       QtFuture::Launch policy);

    template<typename F = Function>
    static void create(F &&func, QFuture<ParentResultType> *f, QFutureInterface<ResultType> &fi,
                       QThreadPool *pool);

    template<typename F = Function>
    static void create(F &&func, QFuture<ParentResultType> *f, QFutureInterface<ResultType> &fi,
                       QObject *context);

private:
    void fulfillPromiseWithResult();
    void fulfillVoidPromise();
    void fulfillPromiseWithVoidResult();

    template<class... Args>
    void fulfillPromise(Args &&... args);

protected:
    virtual void runImpl() = 0;

    void runFunction();

protected:
    QPromise<ResultType> promise;
    QFuture<ParentResultType> parentFuture;
    Function function;
};

template<typename Function, typename ResultType, typename ParentResultType>
class SyncContinuation final : public Continuation<Function, ResultType, ParentResultType>
{
public:
    template<typename F = Function>
    SyncContinuation(F &&func, const QFuture<ParentResultType> &f, QPromise<ResultType> &&p)
        : Continuation<Function, ResultType, ParentResultType>(std::forward<F>(func), f,
                                                               std::move(p))
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
    template<typename F = Function>
    AsyncContinuation(F &&func, const QFuture<ParentResultType> &f, QPromise<ResultType> &&p,
                      QThreadPool *pool = nullptr)
        : Continuation<Function, ResultType, ParentResultType>(std::forward<F>(func), f,
                                                               std::move(p)),
          threadPool(pool)
    {
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

#ifndef QT_NO_EXCEPTIONS

template<class Function, class ResultType>
class FailureHandler
{
public:
    template<typename F = Function>
    static void create(F &&function, QFuture<ResultType> *future,
                       const QFutureInterface<ResultType> &fi);

    template<typename F = Function>
    static void create(F &&function, QFuture<ResultType> *future, QFutureInterface<ResultType> &fi,
                       QObject *context);

    template<typename F = Function>
    FailureHandler(F &&func, const QFuture<ResultType> &f, QPromise<ResultType> &&p)
        : promise(std::move(p)), parentFuture(f), handler(std::forward<F>(func))
    {
    }

public:
    void run();

private:
    template<class ArgType>
    void handleException();
    void handleAllExceptions();

private:
    QPromise<ResultType> promise;
    QFuture<ResultType> parentFuture;
    Function handler;
};

#endif

template<typename Function, typename ResultType, typename ParentResultType>
void Continuation<Function, ResultType, ParentResultType>::runFunction()
{
    promise.start();

    Q_ASSERT(parentFuture.isFinished());

#ifndef QT_NO_EXCEPTIONS
    try {
#endif
        if constexpr (!std::is_void_v<ResultType>) {
            if constexpr (std::is_void_v<ParentResultType>) {
                fulfillPromiseWithVoidResult();
            } else if constexpr (std::is_invocable_v<Function, ParentResultType>) {
                fulfillPromiseWithResult();
            } else {
                // This assert normally should never fail, this is to make sure
                // that nothing unexpected happend.
                static_assert(std::is_invocable_v<Function, QFuture<ParentResultType>>,
                              "The continuation is not invocable with the provided arguments");
                fulfillPromise(parentFuture);
            }
        } else {
            if constexpr (std::is_void_v<ParentResultType>) {
                if constexpr (std::is_invocable_v<Function, QFuture<void>>)
                    function(parentFuture);
                else
                    function();
            } else if constexpr (std::is_invocable_v<Function, ParentResultType>) {
                fulfillVoidPromise();
            } else {
                // This assert normally should never fail, this is to make sure
                // that nothing unexpected happend.
                static_assert(std::is_invocable_v<Function, QFuture<ParentResultType>>,
                              "The continuation is not invocable with the provided arguments");
                function(parentFuture);
            }
        }
#ifndef QT_NO_EXCEPTIONS
    } catch (...) {
        promise.setException(std::current_exception());
    }
#endif
    promise.finish();
}

template<typename Function, typename ResultType, typename ParentResultType>
bool Continuation<Function, ResultType, ParentResultType>::execute()
{
    Q_ASSERT(parentFuture.isFinished());

    if (parentFuture.d.isChainCanceled()) {
#ifndef QT_NO_EXCEPTIONS
        if (parentFuture.d.exceptionStore().hasException()) {
            // If the continuation doesn't take a QFuture argument, propagate the exception
            // to the caller, by reporting it. If the continuation takes a QFuture argument,
            // the user may want to catch the exception inside the continuation, to not
            // interrupt the continuation chain, so don't report anything yet.
            if constexpr (!std::is_invocable_v<std::decay_t<Function>, QFuture<ParentResultType>>) {
                promise.start();
                promise.setException(parentFuture.d.exceptionStore().exception());
                promise.finish();
                return false;
            }
        } else
#endif
        {
            promise.start();
            promise.future().cancel();
            promise.finish();
            return false;
        }
    }

    runImpl();
    return true;
}

// Workaround for keeping move-only lambdas inside std::function
template<class Function>
struct ContinuationWrapper
{
    ContinuationWrapper(Function &&f) : function(std::move(f)) { }
    ContinuationWrapper(const ContinuationWrapper &other)
        : function(std::move(const_cast<ContinuationWrapper &>(other).function))
    {
        Q_ASSERT_X(false, "QFuture", "Continuation shouldn't be copied");
    }
    ContinuationWrapper(ContinuationWrapper &&other) = default;
    ContinuationWrapper &operator=(ContinuationWrapper &&) = default;

    void operator()(const QFutureInterfaceBase &parentData) { function(parentData); }

private:
    Function function;
};

template<typename Function, typename ResultType, typename ParentResultType>
template<typename F>
void Continuation<Function, ResultType, ParentResultType>::create(F &&func,
                                                                  QFuture<ParentResultType> *f,
                                                                  QFutureInterface<ResultType> &fi,
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
            fi.setThreadPool(pool);
        }
    }

    fi.setLaunchAsync(launchAsync);

    auto continuation = [func = std::forward<F>(func), fi, promise = QPromise(fi), pool,
                         launchAsync](const QFutureInterfaceBase &parentData) mutable {
        const auto parent = QFutureInterface<ParentResultType>(parentData).future();
        Continuation<Function, ResultType, ParentResultType> *continuationJob = nullptr;
        if (launchAsync) {
            auto asyncJob = new AsyncContinuation<Function, ResultType, ParentResultType>(
                    std::forward<Function>(func), parent, std::move(promise), pool);
            fi.setRunnable(asyncJob);
            continuationJob = asyncJob;
        } else {
            continuationJob = new SyncContinuation<Function, ResultType, ParentResultType>(
                    std::forward<Function>(func), parent, std::move(promise));
        }

        bool isLaunched = continuationJob->execute();
        // If continuation is successfully launched, AsyncContinuation will be deleted
        // by the QThreadPool which has started it. Synchronous continuation will be
        // executed immediately, so it's safe to always delete it here.
        if (!(launchAsync && isLaunched)) {
            delete continuationJob;
            continuationJob = nullptr;
        }
    };
    f->d.setContinuation(ContinuationWrapper(std::move(continuation)), fi.d);
}

template<typename Function, typename ResultType, typename ParentResultType>
template<typename F>
void Continuation<Function, ResultType, ParentResultType>::create(F &&func,
                                                                  QFuture<ParentResultType> *f,
                                                                  QFutureInterface<ResultType> &fi,
                                                                  QThreadPool *pool)
{
    Q_ASSERT(f);

    fi.setLaunchAsync(true);
    fi.setThreadPool(pool);

    auto continuation = [func = std::forward<F>(func), promise = QPromise(fi),
                         pool](const QFutureInterfaceBase &parentData) mutable {
        const auto parent = QFutureInterface<ParentResultType>(parentData).future();
        auto continuationJob = new AsyncContinuation<Function, ResultType, ParentResultType>(
                std::forward<Function>(func), parent, std::move(promise), pool);
        bool isLaunched = continuationJob->execute();
        // If continuation is successfully launched, AsyncContinuation will be deleted
        // by the QThreadPool which has started it.
        if (!isLaunched) {
            delete continuationJob;
            continuationJob = nullptr;
        }
    };
    f->d.setContinuation(ContinuationWrapper(std::move(continuation)), fi.d);
}

template<typename Function, typename ResultType, typename ParentResultType>
template<typename F>
void Continuation<Function, ResultType, ParentResultType>::create(F &&func,
                                                                  QFuture<ParentResultType> *f,
                                                                  QFutureInterface<ResultType> &fi,
                                                                  QObject *context)
{
    Q_ASSERT(f);

    auto continuation = [func = std::forward<F>(func), fi,
                         context = QPointer<QObject>(context)](
                                const QFutureInterfaceBase &parentData) mutable {
        Q_ASSERT(context);
        const auto parent = QFutureInterface<ParentResultType>(parentData).future();
        QMetaObject::invokeMethod(
                context,
                [func = std::forward<F>(func), promise = QPromise(fi), parent]() mutable {
                    SyncContinuation<Function, ResultType, ParentResultType> continuationJob(
                            std::forward<Function>(func), parent, std::move(promise));
                    continuationJob.execute();
                });
    };
    f->d.setContinuation(ContinuationWrapper(std::move(continuation)), fi.d);
}

template<typename Function, typename ResultType, typename ParentResultType>
void Continuation<Function, ResultType, ParentResultType>::fulfillPromiseWithResult()
{
    if constexpr (std::is_copy_constructible_v<ParentResultType>)
        fulfillPromise(parentFuture.result());
    else
        fulfillPromise(parentFuture.takeResult());
}

template<typename Function, typename ResultType, typename ParentResultType>
void Continuation<Function, ResultType, ParentResultType>::fulfillVoidPromise()
{
    if constexpr (std::is_copy_constructible_v<ParentResultType>)
        function(parentFuture.result());
    else
        function(parentFuture.takeResult());
}

template<typename Function, typename ResultType, typename ParentResultType>
void Continuation<Function, ResultType, ParentResultType>::fulfillPromiseWithVoidResult()
{
    if constexpr (std::is_invocable_v<Function, QFuture<void>>)
        fulfillPromise(parentFuture);
    else
        fulfillPromise();
}

template<typename Function, typename ResultType, typename ParentResultType>
template<class... Args>
void Continuation<Function, ResultType, ParentResultType>::fulfillPromise(Args &&... args)
{
    promise.addResult(std::invoke(function, std::forward<Args>(args)...));
}

template<class T>
void fulfillPromise(QPromise<T> &promise, QFuture<T> &future)
{
    if constexpr (!std::is_void_v<T>) {
        if constexpr (std::is_copy_constructible_v<T>)
            promise.addResult(future.result());
        else
            promise.addResult(future.takeResult());
    }
}

template<class T, class Function>
void fulfillPromise(QPromise<T> &promise, Function &&handler)
{
    if constexpr (std::is_void_v<T>)
        handler();
    else
        promise.addResult(handler());
}

#ifndef QT_NO_EXCEPTIONS

template<class Function, class ResultType>
template<class F>
void FailureHandler<Function, ResultType>::create(F &&function, QFuture<ResultType> *future,
                                                  const QFutureInterface<ResultType> &fi)
{
    Q_ASSERT(future);

    auto failureContinuation = [function = std::forward<F>(function), promise = QPromise(fi)](
                                       const QFutureInterfaceBase &parentData) mutable {
        const auto parent = QFutureInterface<ResultType>(parentData).future();
        FailureHandler<Function, ResultType> failureHandler(std::forward<Function>(function),
                                                            parent, std::move(promise));
        failureHandler.run();
    };

    future->d.setContinuation(ContinuationWrapper(std::move(failureContinuation)));
}

template<class Function, class ResultType>
template<class F>
void FailureHandler<Function, ResultType>::create(F &&function, QFuture<ResultType> *future,
                                                  QFutureInterface<ResultType> &fi,
                                                  QObject *context)
{
    Q_ASSERT(future);

    auto failureContinuation =
            [function = std::forward<F>(function), fi,
             context = QPointer<QObject>(context)](const QFutureInterfaceBase &parentData) mutable {
                Q_ASSERT(context);
                const auto parent = QFutureInterface<ResultType>(parentData).future();
                QMetaObject::invokeMethod(context,
                                          [function = std::forward<F>(function),
                                          promise = QPromise(fi), parent]() mutable {
                    FailureHandler<Function, ResultType> failureHandler(
                                std::forward<Function>(function), parent, std::move(promise));
                    failureHandler.run();
                });
            };

    future->d.setContinuation(ContinuationWrapper(std::move(failureContinuation)));
}

template<class Function, class ResultType>
void FailureHandler<Function, ResultType>::run()
{
    Q_ASSERT(parentFuture.isFinished());

    promise.start();

    if (parentFuture.d.exceptionStore().hasException()) {
        using ArgType = typename QtPrivate::ArgResolver<Function>::First;
        if constexpr (std::is_void_v<ArgType>) {
            handleAllExceptions();
        } else {
            handleException<ArgType>();
        }
    } else {
        QtPrivate::fulfillPromise(promise, parentFuture);
    }
    promise.finish();
}

template<class Function, class ResultType>
template<class ArgType>
void FailureHandler<Function, ResultType>::handleException()
{
    try {
        parentFuture.d.exceptionStore().throwPossibleException();
    } catch (const ArgType &e) {
        try {
            // Handle exceptions matching with the handler's argument type
            if constexpr (std::is_void_v<ResultType>)
                handler(e);
            else
                promise.addResult(handler(e));
        } catch (...) {
            promise.setException(std::current_exception());
        }
    } catch (...) {
        // Exception doesn't match with handler's argument type, propagate
        // the exception to be handled later.
        promise.setException(std::current_exception());
    }
}

template<class Function, class ResultType>
void FailureHandler<Function, ResultType>::handleAllExceptions()
{
    try {
        parentFuture.d.exceptionStore().throwPossibleException();
    } catch (...) {
        try {
            QtPrivate::fulfillPromise(promise, std::forward<Function>(handler));
        } catch (...) {
            promise.setException(std::current_exception());
        }
    }
}

#endif // QT_NO_EXCEPTIONS

template<class Function, class ResultType>
class CanceledHandler
{
public:
    template<class F = Function>
    static void create(F &&handler, QFuture<ResultType> *future, QFutureInterface<ResultType> &fi)
    {
        Q_ASSERT(future);

        auto canceledContinuation = [promise = QPromise(fi), handler = std::forward<F>(handler)](
                                            const QFutureInterfaceBase &parentData) mutable {
            auto parentFuture = QFutureInterface<ResultType>(parentData).future();
            run(std::forward<F>(handler), parentFuture, std::move(promise));
        };
        future->d.setContinuation(ContinuationWrapper(std::move(canceledContinuation)));
    }

    template<class F = Function>
    static void create(F &&handler, QFuture<ResultType> *future, QFutureInterface<ResultType> &fi,
                       QObject *context)
    {
        Q_ASSERT(future);
        auto canceledContinuation = [fi, handler = std::forward<F>(handler),
                                     context = QPointer<QObject>(context)](
                                            const QFutureInterfaceBase &parentData) mutable {
            Q_ASSERT(context);
            auto parentFuture = QFutureInterface<ResultType>(parentData).future();
            QMetaObject::invokeMethod(context,
                                      [promise = QPromise(fi), parentFuture,
                                      handler = std::forward<F>(handler)]() mutable {
                run(std::forward<F>(handler), parentFuture, std::move(promise));
            });
        };

        future->d.setContinuation(ContinuationWrapper(std::move(canceledContinuation)));
    }

    template<class F = Function>
    static void run(F &&handler, QFuture<ResultType> &parentFuture, QPromise<ResultType> &&promise)
    {
        promise.start();

        if (parentFuture.isCanceled()) {
#ifndef QT_NO_EXCEPTIONS
            if (parentFuture.d.exceptionStore().hasException()) {
                // Propagate the exception to the result future
                promise.setException(parentFuture.d.exceptionStore().exception());
            } else {
                try {
#endif
                    QtPrivate::fulfillPromise(promise, std::forward<F>(handler));
#ifndef QT_NO_EXCEPTIONS
                } catch (...) {
                    promise.setException(std::current_exception());
                }
            }
#endif
        } else {
            QtPrivate::fulfillPromise(promise, parentFuture);
        }

        promise.finish();
    }
};

} // namespace QtPrivate

namespace QtFuture {

template<class Signal>
using ArgsType = typename QtPrivate::ArgResolver<Signal>::AllArgs;

template<class Sender, class Signal, typename = QtPrivate::EnableIfInvocable<Sender, Signal>>
static QFuture<ArgsType<Signal>> connect(Sender *sender, Signal signal)
{
    using ArgsType = ArgsType<Signal>;
    QFutureInterface<ArgsType> promise;
    promise.reportStarted();
    if (!sender) {
        promise.reportCanceled();
        promise.reportFinished();
        return promise.future();
    }

    using Connections = std::pair<QMetaObject::Connection, QMetaObject::Connection>;
    auto connections = std::make_shared<Connections>();

    if constexpr (std::is_void_v<ArgsType>) {
        connections->first =
                QObject::connect(sender, signal, sender, [promise, connections]() mutable {
                    QObject::disconnect(connections->first);
                    QObject::disconnect(connections->second);
                    promise.reportFinished();
                });
    } else if constexpr (QtPrivate::ArgResolver<Signal>::HasExtraArgs) {
        connections->first = QObject::connect(sender, signal, sender,
                                              [promise, connections](auto... values) mutable {
                                                  QObject::disconnect(connections->first);
                                                  QObject::disconnect(connections->second);
                                                  promise.reportResult(QtPrivate::createTuple(
                                                          std::move(values)...));
                                                  promise.reportFinished();
                                              });
    } else {
        connections->first = QObject::connect(sender, signal, sender,
                                              [promise, connections](ArgsType value) mutable {
                                                  QObject::disconnect(connections->first);
                                                  QObject::disconnect(connections->second);
                                                  promise.reportResult(value);
                                                  promise.reportFinished();
                                              });
    }

    if (!connections->first) {
        promise.reportCanceled();
        promise.reportFinished();
        return promise.future();
    }

    connections->second =
            QObject::connect(sender, &QObject::destroyed, sender, [promise, connections]() mutable {
                QObject::disconnect(connections->first);
                QObject::disconnect(connections->second);
                promise.reportCanceled();
                promise.reportFinished();
            });

    return promise.future();
}

template<typename T, typename = QtPrivate::EnableForNonVoid<T>>
static QFuture<std::decay_t<T>> makeReadyFuture(T &&value)
{
    QFutureInterface<std::decay_t<T>> promise;
    promise.reportStarted();
    promise.reportResult(std::forward<T>(value));
    promise.reportFinished();

    return promise.future();
}

#if defined(Q_CLANG_QDOC)
static QFuture<void> makeReadyFuture()
#else
template<typename T = void>
static QFuture<T> makeReadyFuture()
#endif
{
    QFutureInterface<T> promise;
    promise.reportStarted();
    promise.reportFinished();

    return promise.future();
}

template<typename T>
static QFuture<T> makeReadyFuture(const QList<T> &values)
{
    QFutureInterface<T> promise;
    promise.reportStarted();
    promise.reportResults(values);
    promise.reportFinished();

    return promise.future();
}

#ifndef QT_NO_EXCEPTIONS

template<typename T = void>
static QFuture<T> makeExceptionalFuture(std::exception_ptr exception)
{
    QFutureInterface<T> promise;
    promise.reportStarted();
    promise.reportException(exception);
    promise.reportFinished();

    return promise.future();
}

template<typename T = void>
static QFuture<T> makeExceptionalFuture(const QException &exception)
{
    try {
        exception.raise();
    } catch (...) {
        return makeExceptionalFuture<T>(std::current_exception());
    }
    Q_UNREACHABLE();
}

#endif // QT_NO_EXCEPTIONS

} // namespace QtFuture

QT_END_NAMESPACE
