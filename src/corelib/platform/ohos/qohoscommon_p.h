// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSCOMMON_H
#define QOHOSCOMMON_H

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

#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlogging.h>
#include <QtCore/qdebug.h>
#include <array>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#ifndef QT_NO_EXCEPTIONS
#include <stdexcept>
#endif
#include <type_traits>

#define Q_OHOS_NAMED_FUNC(func) (QT_PREPEND_NAMESPACE(makeQOhosNamedFunc)<decltype(func)*, func>)(QT_STRINGIFY(func))

#define qOhosReportFatalErrorAndAbort(...) \
    do { \
        QT_PREPEND_NAMESPACE(QMessageLogger)(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC).fatal(__VA_ARGS__); \
        std::abort(); \
    } while (false)

QT_BEGIN_NAMESPACE

template<typename Func, typename... FuncArgs>
using QOhosInvokeResult = decltype(std::declval<Func>()(std::declval<FuncArgs>()...));

template<typename ...Ts>
using QOhosConsumer = std::function<void(Ts...)>;

template<typename T>
using QOhosSupplier = std::function<T()>;

template<typename... Args>
class QOhosTaskPromise
{
public:
    template<typename F, typename = std::enable_if_t<!std::is_same<std::decay_t<F>, QOhosTaskPromise>::value>>
    QOhosTaskPromise(
        F &&callable, std::function<void()> optOnDestroyedWithoutCall,
        std::string callerContextName = {});

    ~QOhosTaskPromise();

    QOhosTaskPromise(QOhosTaskPromise &&) = default;
    QOhosTaskPromise &operator=(QOhosTaskPromise &&) = default;

    QOhosTaskPromise(const QOhosTaskPromise &) = delete;
    QOhosTaskPromise &operator=(const QOhosTaskPromise &) = delete;

    void operator()(Args... args) const;

    QOhosTaskPromise makeChained(std::string callerContextName) &&;

    template<typename... ContextNames>
    std::array<QOhosTaskPromise, sizeof...(ContextNames)> makeBranched(ContextNames &&...branchContextNames) &&;

    std::pair<QOhosTaskPromise, QOhosTaskPromise> makeThenCatchBranches(std::string callerContextName) &&;

    operator std::function<void(Args...)>() &&;

private:
    class Callable
    {
    public:
        virtual ~Callable() = default;
        virtual void call(Args &&...args) = 0;
    };

    template<typename Func>
    class CallableImpl : public Callable
    {
    public:
        template<typename F>
        explicit CallableImpl(F &&f);

        void call(Args &&...args) override;

    private:
        Func m_func;
    };

    struct TrackingUsageState
    {
        TrackingUsageState(
            std::size_t activeCount,
            std::shared_ptr<std::function<void()>> onDestroyedWithoutCall);

        std::shared_ptr<std::function<void()>> onDestroyedWithoutCall;
        bool used = false;
        std::size_t activeCount = 1;
    };

    struct TrackingNode
    {
        TrackingNode(
            std::string callerContext, std::shared_ptr<TrackingNode> optParent,
            std::shared_ptr<TrackingUsageState> usage);

        void markUsed(const char *callerContextString);
        std::string rootCallerContextOrUnknown() const;
        void logCallerContextChainAsErrors(const char *messagePrefix) const;

        std::string callerContext;
        std::shared_ptr<TrackingNode> optParent;
        std::shared_ptr<TrackingUsageState> usage;
    };

    QOhosTaskPromise(std::shared_ptr<Callable> callable, std::shared_ptr<TrackingNode> tracking);

    std::shared_ptr<Callable> m_callable;
    std::shared_ptr<TrackingNode> m_tracking;
};

template<typename Func, Func func>
class QOhosNamedFunc
{
public:
    static constexpr Func funcPtr = func;

    QOhosNamedFunc(const char *funcName);

    const char *name() const;
    Func ptr() const;

    operator Func () const;

private:
    static_assert(std::is_function_v<std::remove_pointer_t<Func>>);

    const char *m_funcName;
};

template<typename Func, Func func>
QOhosNamedFunc<Func, func> makeQOhosNamedFunc(const char *funcName)
{
    return {funcName};
}

class QOhosNoOpConsumer
{
public:
    template<typename ...Ts>
    void operator()(const Ts &...) const;
};

template<typename ...Ts>
QOhosConsumer<Ts...> makeQOhosNoOpConsumer();

QOhosNoOpConsumer makeQOhosNoOpConsumer();

template<typename T>
class QOhosMutexProtectedValue
{
public:
    QOhosMutexProtectedValue();

    QOhosMutexProtectedValue(const QOhosMutexProtectedValue<T> &other) = delete;
    QOhosMutexProtectedValue &operator=(const QOhosMutexProtectedValue<T> &other) = delete;

    QOhosMutexProtectedValue(QOhosMutexProtectedValue<T> &&other) = delete;
    QOhosMutexProtectedValue &operator=(QOhosMutexProtectedValue<T> &&other) = delete;

    template<typename ProcessFunc>
    void processValue(ProcessFunc &&processFunc);

    template<typename EvalFunc>
    auto evalWithValue(EvalFunc &&evalFunc) const -> decltype(evalFunc(std::declval<const T &>()));

private:
    mutable std::mutex m_valueMutex;
    T m_value{};
};

namespace QtOhos {

template<typename T>
std::shared_ptr<T> moveToSharedPtr(T &&obj);

template<typename T>
std::shared_ptr<T> makeSharedPtrWithAttachedExtraData(
    std::shared_ptr<T> baseSharedPtr, std::shared_ptr<void> extraData);

template<typename T>
std::weak_ptr<T> makeWeakPtr(const std::shared_ptr<T> &obj);

std::shared_ptr<void> makeDestroyNotifier(std::function<void()> callOnDestroy);

}

template<typename Func, Func func>
QOhosNamedFunc<Func, func>::QOhosNamedFunc(const char *funcName)
    : m_funcName(funcName)
{
}

template<typename Func, Func func>
const char *QOhosNamedFunc<Func, func>::name() const
{
    return m_funcName;
}

template<typename Func, Func func>
Func QOhosNamedFunc<Func, func>::ptr() const
{
    return func;
}

template<typename Func, Func func>
QOhosNamedFunc<Func, func>::operator Func () const
{
    return func;
}

template<typename ...Ts>
void QOhosNoOpConsumer::operator()(const Ts &...) const
{
}

template<typename ...Ts>
QOhosConsumer<Ts...> makeQOhosNoOpConsumer()
{
    return [](const Ts &...) {
    };
}

inline QOhosNoOpConsumer makeQOhosNoOpConsumer()
{
    return QOhosNoOpConsumer();
}

template<typename T>
QOhosMutexProtectedValue<T>::QOhosMutexProtectedValue() = default;

template<typename T>
template<typename ProcessFunc>
void QOhosMutexProtectedValue<T>::processValue(ProcessFunc &&processFunc)
{
    std::lock_guard<std::mutex> valueLock(m_valueMutex);
    std::forward<ProcessFunc>(processFunc)(m_value);
}

template<typename T>
template<typename EvalFunc>
auto QOhosMutexProtectedValue<T>::evalWithValue(EvalFunc &&evalFunc) const -> decltype(evalFunc(std::declval<const T &>()))
{
    std::lock_guard<std::mutex> valueLock(m_valueMutex);
    return std::forward<EvalFunc>(evalFunc)(m_value);
}

namespace QtOhos {

template<typename T>
std::shared_ptr<T> moveToSharedPtr(T &&obj)
{
    return std::make_shared<T>(std::forward<T>(obj));
}

template<typename T>
std::shared_ptr<T> makeSharedPtrWithAttachedExtraData(
    std::shared_ptr<T> baseSharedPtr, std::shared_ptr<void> extraData)
{
    auto *baseRawPtr = baseSharedPtr.get();
    return std::shared_ptr<T>(
        moveToSharedPtr(
            std::make_pair(
                std::move(baseSharedPtr),
                std::move(extraData))),
        baseRawPtr);
}

template<typename T>
std::weak_ptr<T> makeWeakPtr(const std::shared_ptr<T> &obj)
{
    return obj;
}

inline std::shared_ptr<void> makeDestroyNotifier(std::function<void()> callOnDestroy)
{
    class DestroyNotifier
    {
    public:
        explicit DestroyNotifier(std::function<void()> callOnDestroy)
            : callOnDestroy(std::move(callOnDestroy))
        {
        }

        DestroyNotifier(const DestroyNotifier &) = delete;
        DestroyNotifier(DestroyNotifier &&) = delete;
        DestroyNotifier &operator=(const DestroyNotifier &) = delete;
        DestroyNotifier &operator=(DestroyNotifier &&) = delete;

        ~DestroyNotifier()
        {
            callOnDestroy();
        };

    private:
        std::function<void()> callOnDestroy;
    };

    return std::make_shared<DestroyNotifier>(std::move(callOnDestroy));
}

}

template<typename... Args>
template<typename F, typename>
QOhosTaskPromise<Args...>::QOhosTaskPromise(
    F &&callable, std::function<void()> optOnDestroyedWithoutCall, std::string callerContextName)
    : m_callable(std::make_shared<CallableImpl<std::decay_t<F>>>(std::forward<F>(callable)))
    , m_tracking(
        std::make_shared<TrackingNode>(
            std::move(callerContextName), nullptr,
            std::make_shared<TrackingUsageState>(
                1,
                QtOhos::moveToSharedPtr(
                    optOnDestroyedWithoutCall
                        ? std::move(optOnDestroyedWithoutCall)
                        : makeQOhosNoOpConsumer<>()))))
{
}

template<typename... Args>
QOhosTaskPromise<Args...>::~QOhosTaskPromise()
{
    if (!m_tracking || m_tracking->usage->used)
        return;

    --m_tracking->usage->activeCount;

    if (m_tracking->usage->activeCount == 0) {
        m_tracking->logCallerContextChainAsErrors(Q_FUNC_INFO);
        qOhosPrintfError(
            "%s: promise destroyed without notifying the caller: %s",
            Q_FUNC_INFO, m_tracking->rootCallerContextOrUnknown().c_str());
        (*m_tracking->usage->onDestroyedWithoutCall)();
    }
}

template<typename... Args>
QOhosTaskPromise<Args...>::QOhosTaskPromise(
    std::shared_ptr<Callable> callable, std::shared_ptr<TrackingNode> tracking)
    : m_callable(std::move(callable))
    , m_tracking(std::move(tracking))
{
}

template<typename... Args>
void QOhosTaskPromise<Args...>::operator()(Args... args) const
{
    if (!m_tracking)
        qOhosReportFatalErrorAndAbort("%s: called on moved-from instance", Q_FUNC_INFO);

    if (!m_callable)
        qOhosReportFatalErrorAndAbort("%s: called on empty QOhosTaskPromise", Q_FUNC_INFO);

    m_tracking->markUsed(Q_FUNC_INFO);

    m_callable->call(std::forward<Args>(args)...);
}

template<typename... Args>
QOhosTaskPromise<Args...>::operator std::function<void(Args...)>() &&
{
    const auto funcInfo = Q_FUNC_INFO;

    if (!m_tracking)
        qOhosReportFatalErrorAndAbort("%s: called on moved-from instance", funcInfo);

    if (!m_callable)
        return {};

    m_tracking->markUsed(funcInfo);

    auto callable = std::move(m_callable);
    auto tracking = std::move(m_tracking);

    auto sharedCalledFlag = std::make_shared<bool>(false);
    auto destroyNotifier = QtOhos::makeDestroyNotifier(
        [sharedCalledFlag, tracking, funcInfo]() {
            if (!*sharedCalledFlag) {
                tracking->logCallerContextChainAsErrors(funcInfo);
                qOhosPrintfError(
                    "%s: promise (as std::function) destroyed without notifying the caller: %s",
                    funcInfo, tracking->rootCallerContextOrUnknown().c_str());
                (*tracking->usage->onDestroyedWithoutCall)();
            }
        });

    return [callable, sharedCalledFlag, tracking, funcInfo, destroyNotifier](Args... args) {
        if (*sharedCalledFlag) {
            qOhosReportFatalErrorAndAbort(
                "%s: promise (as std::function) called more than once for: %s",
                funcInfo, tracking->rootCallerContextOrUnknown().c_str());
        }
        *sharedCalledFlag = true;
        callable->call(std::forward<Args>(args)...);
    };
}

template<typename... Args>
QOhosTaskPromise<Args...> QOhosTaskPromise<Args...>::makeChained(std::string callerContextName) &&
{
    if (!m_tracking)
        qOhosReportFatalErrorAndAbort("%s: called on moved-from instance", Q_FUNC_INFO);

    m_tracking->markUsed(Q_FUNC_INFO);

    auto newUsage = std::make_shared<TrackingUsageState>(1, m_tracking->usage->onDestroyedWithoutCall);

    return QOhosTaskPromise(
        std::move(m_callable),
        std::make_shared<TrackingNode>(
            std::move(callerContextName), std::move(m_tracking), newUsage));
}

template<typename... Args>
template<typename... ContextNames>
std::array<QOhosTaskPromise<Args...>, sizeof...(ContextNames)>
QOhosTaskPromise<Args...>::makeBranched(ContextNames &&...branchContextNames) &&
{
    if (!m_tracking)
        qOhosReportFatalErrorAndAbort("%s: called on moved-from instance", Q_FUNC_INFO);

    m_tracking->markUsed(Q_FUNC_INFO);

    auto sharedUsage = std::make_shared<TrackingUsageState>(
        sizeof...(ContextNames), m_tracking->usage->onDestroyedWithoutCall);
    return {{
        QOhosTaskPromise(
            m_callable,
            std::make_shared<TrackingNode>(
                std::forward<ContextNames>(branchContextNames),
                m_tracking, sharedUsage))...
    }};
}

template<typename... Args>
std::pair<QOhosTaskPromise<Args...>, QOhosTaskPromise<Args...>>
QOhosTaskPromise<Args...>::makeThenCatchBranches(std::string callerContextName) &&
{
    auto branches = std::move(*this).makeChained(std::move(callerContextName)).makeBranched("then", "catch");
    return {std::move(branches[0]), std::move(branches[1])};
}

template<typename... Args>
template<typename Func>
template<typename F>
QOhosTaskPromise<Args...>::CallableImpl<Func>::CallableImpl(F &&f)
    : m_func(std::forward<F>(f))
{
}

template<typename... Args>
template<typename Func>
void QOhosTaskPromise<Args...>::CallableImpl<Func>::call(Args &&...args)
{
    m_func(std::forward<Args>(args)...);
}

template<typename... Args>
QOhosTaskPromise<Args...>::TrackingUsageState::TrackingUsageState(
    std::size_t activeCount, std::shared_ptr<std::function<void()>> onDestroyedWithoutCall)
    : onDestroyedWithoutCall(onDestroyedWithoutCall)
    , activeCount(activeCount)
{
}

template<typename... Args>
QOhosTaskPromise<Args...>::TrackingNode::TrackingNode(
    std::string callerContext, std::shared_ptr<TrackingNode> optParent,
    std::shared_ptr<TrackingUsageState> usage)
    : callerContext(std::move(callerContext))
    , optParent(std::move(optParent))
    , usage(std::move(usage))
{
}

template<typename... Args>
void QOhosTaskPromise<Args...>::TrackingNode::markUsed(const char *callerContextString)
{
    if (usage->used) {
        logCallerContextChainAsErrors(callerContextString);
        qOhosReportFatalErrorAndAbort(
            "%s: using a dead promise branch (%s) from this caller: %s",
            callerContextString, callerContext.c_str(), rootCallerContextOrUnknown().c_str());
    }
    usage->used = true;
}

template<typename... Args>
std::string QOhosTaskPromise<Args...>::TrackingNode::rootCallerContextOrUnknown() const
{
    const TrackingNode *rootContextNode = nullptr;
    for (auto currentNode = this; currentNode != nullptr; currentNode = currentNode->optParent.get()) {
        if (!currentNode->callerContext.empty())
            rootContextNode = currentNode;
    }
    return rootContextNode != nullptr ? rootContextNode->callerContext : "<unknown>";
}

template<typename... Args>
void QOhosTaskPromise<Args...>::TrackingNode::logCallerContextChainAsErrors(const char *messagePrefix) const
{
    int index = 0;
    for (auto currentNode = this; currentNode; currentNode = currentNode->optParent.get()) {
        if (!currentNode->callerContext.empty()) {
            qOhosPrintfError(
                "%s [caller context #%d]: %s",
                messagePrefix, index, currentNode->callerContext.c_str());
            ++index;
        }
    }
}

QT_END_NAMESPACE

#endif
