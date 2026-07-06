// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLUGINCORE_H
#define QOHOSPLUGINCORE_H

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qobject.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <qohosenums.h>
#ifndef QT_NO_EXCEPTIONS
#include <stdexcept>
#endif
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace qohosplugincore_h_detail {

template<typename A, typename B, typename = void>
struct HasEqualityComparatorT : std::false_type {};

template<typename A, typename B>
struct HasEqualityComparatorT<A, B, decltype(void(std::declval<A>() == std::declval<B>()))>
    : std::true_type {};

template<typename A, typename B>
constexpr bool hasEqualityComparator = HasEqualityComparatorT<A, B>::value;

template<typename>
struct IsStdOptional : std::false_type {};

template<typename T>
struct IsStdOptional<std::optional<T>> : std::true_type {};

template<typename T>
constexpr bool isStdOptional = IsStdOptional<T>::value;

}

template<typename T>
using QOhosOptional = std::optional<T>;

template<typename T, typename Func>
std::enable_if_t<
    qohosplugincore_h_detail::isStdOptional<QOhosInvokeResult<Func, T>>,
    QOhosInvokeResult<Func, T>>
qAndThen(const std::optional<T> &opt, Func &&func);

template<typename T, typename Func>
std::optional<std::remove_cv_t<QOhosInvokeResult<Func, T>>> qTransform(const std::optional<T> &opt, Func &&func);

template<typename T>
std::optional<T> makeQOhosOptional(const T &value);

std::nullopt_t makeEmptyQOhosOptional();

namespace QtOhos {

class JsWindowsTracker
{
public:
    static void tagWindowAsClosing(QNapi::Object jsWindow, const char *logContext);
    static bool isWindowClosing(QNapi::Object jsWindow);

    JsWindowsTracker() = delete;
};

class QAbilityPeer
{
public:
    virtual ~QAbilityPeer();

    virtual std::string instanceId() = 0;
    virtual QNapi::Object uiContext() = 0;
    virtual QNapi::Object qAbility() = 0;
    virtual QNapi::Object launchWant() = 0;
    virtual QObjectThreadSafeRef qWindowRef() = 0;
    virtual QOhosOptional<QNapi::Promise> qWindowDestroyPromise() = 0;
    virtual void forceResolveQWindowDestroyPromiseIfPresent(Napi::Env env) = 0;
    virtual std::shared_ptr<std::atomic_bool> destroyAllowedFlag() = 0;
    virtual bool isTerminating() = 0;

    virtual void setQWindow(Napi::Env env, QObjectThreadSafeRef qwindow) = 0;

    virtual void *tryCastWithTypeIdObject(const void *matchTypeIdObject) = 0;

protected:
    QAbilityPeer();
};

struct QAbilityInfo
{
    std::string name;
    std::string bundleName;
    std::string moduleName;
};

class QAbilityEngine
{
public:
    virtual ~QAbilityEngine();

    virtual QAbilityInfo readAbilityInfo(const QNapi::Object &ability) const = 0;

protected:
    QAbilityEngine();
};

class JsState;

class AppFunctions
{
public:
    virtual ~AppFunctions();

    virtual void startQAbilityInstance(
        QNapi::Object baseQAbility, QObjectThreadSafeRef qwindow,
        QNapi::Object optStartOptions,
        std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc) = 0;

    virtual void startAppProcess(
        QNapi::Object baseQAbility, const std::string &processId, QNapi::Object want,
        QNapi::Object optStartOptions, std::function<void(JsState &)> continueFunc) = 0;

    virtual void startNoUiChildProcess(JsState &jsState, const std::string &libraryName, const std::vector<std::string> &args) = 0;

    virtual void tagWidgetOrWindowAsFloatWindow(QObject *widgetOrWindow, bool floatWindowEnabled) = 0;
};

enum class QtRunMode
{
    Normal,
    NoUiChildProcess,
};

class JsState : public QOhosJsState
{
public:
    JsState(const JsState &) = delete;
    JsState &operator=(const JsState &) = delete;

    ~JsState() override;

    QT_DEPRECATED virtual QNapi::Object getModule(const std::string &moduleName) = 0;

    virtual QNapi::Object appLaunchWant() = 0;
    virtual QOhosOptional<QNapi::Object> optAppLaunchParam() = 0;

    virtual std::shared_ptr<QAbilityPeer> defaultQAbilityPeer() = 0;
    virtual std::shared_ptr<QAbilityPeer> tryGetQAbilityPeerByInstanceId(const std::string &instanceId) = 0;
    virtual std::shared_ptr<QAbilityPeer> tryGetQAbilityPeerByInstance(QNapi::Object qAbility) = 0;
    virtual std::shared_ptr<QAbilityPeer> tryGetQAbilityPeerByQWindow(QObjectThreadSafeRef qwindow) = 0;

    virtual void visitEachQAbilityPeer(const std::function<void(std::shared_ptr<QAbilityPeer>)> &visitor) = 0;

    virtual void startNewQAbilityInstance(
        std::shared_ptr<QAbilityPeer> baseQAbilityPeer, QObjectThreadSafeRef qwindow,
        QNapi::Object optStartOptions,
        std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc) = 0;

    virtual void startAppProcess(
        const std::string &processId, QNapi::Object requestWant,
        QNapi::Object optStartOptions, std::function<void(JsState &)> continueFunc) = 0;

    virtual void addNewWantConsumer(QOhosConsumer<JsState &, QNapi::Object, QNapi::Object> wantConsumer) = 0;

    virtual void startNoUiChildProcess(const std::string &libraryName, const std::vector<std::string> &args) = 0;

    virtual QtRunMode qtRunMode() = 0;

    template<typename T>
    std::enable_if_t<std::is_default_constructible<T>::value, T> &getAttachedObjectWithLazyCreate();

    template<typename T>
    QNapi::Symbol getJsSymbolForType();

protected:
    JsState();

private:
    virtual void *getAttachedObjectWithLazyCreate(
        const std::type_info &objectTypeInfo, QOhosSupplier<std::shared_ptr<void>> objectFactory) = 0;

    virtual QNapi::Symbol getJsSymbolForType(const std::type_info &typeInfo) = 0;
};

using QOhosAbilityOnContinueResult = enums::ohos::app::ability::AbilityConstant::OnContinueResult;

class QUiAbilityPeer : public virtual QAbilityPeer, public std::enable_shared_from_this<QUiAbilityPeer>
{
public:
    static std::shared_ptr<QUiAbilityPeer> tryCastFromQAbilityPeerOrNull(std::shared_ptr<QAbilityPeer> qAbilityPeer);

    ~QUiAbilityPeer() override;

    virtual QNapi::Object launchParam() = 0;

    virtual QNapi::Object windowStage() = 0;
    virtual QNapi::Object window() = 0;

    virtual void setOnContinueRequestsHandler(
        std::function<void(JsState &, QNapi::Object, QOhosConsumer<JsState &, QOhosAbilityOnContinueResult>)> requestsHandler) = 0;

protected:
    QUiAbilityPeer();

private:
    static const nullptr_t typeIdObject;

    void *tryCastWithTypeIdObject(const void *matchTypeIdObject) final;
};

class CallbackInfo : public ::QOhosCallbackInfo
{
public:
    using ::QOhosCallbackInfo::QOhosCallbackInfo;

    JsState &jsState() const;
};

// this function should be called once from JS thread at some point during startup
void initJsThreadState(
    napi_env env, std::map<std::string, QNapi::Reference<QNapi::Function>> &&jsModulesFactories,
    std::shared_ptr<AppFunctions> appFunctions, QtRunMode qtRunMode);

// this function should be called from JS thread for each UIAbility when it's ready
void addJsQAbilityPeer(std::shared_ptr<QAbilityPeer> qAbilityPeer);

// this function should be called from JS thread when WindowStage of UIAbility is destroyed
void removeMatchingJsQAbilityPeer(QNapi::Object qAbility);

// this function should be called from JS thread when new Want object is received
void dispatchNewWant(QNapi::Object want, QNapi::Object launchParam);

// invokes the task inside the JS thread, can be called from Qt thread at any time
void invokeInJsThread(std::function<void(JsState &)> task);

// Invokes the task inside the JS thread and blocks the caller's thread until
// the "continue" function (std::function<void()>, passed as second argument to
// the task) is called on the JS side.
// It can be called from the Qt thread at any time, calling it from the JS
// thread is illegal.
void invokeInJsThreadAndWaitForContinue(
    std::function<void(JsState &, QOhosTaskPromise<>)> &&task,
    std::string callerContextName = {});

// Runs the task inside the JS thread and waits until its execution ends.
// When called from the JS thread, it calls the task directly. For other threads
// it behaves like a wrapper around the invokeInJsThreadAndWaitForContinue().
void runInJsThreadAndWait(
    const std::function<void(JsState &)> &task,
    std::string callerContextName = {});

template<typename Func>
auto evalInJsThread(Func &&func, std::string callerContextName = {}) -> decltype(func(std::declval<JsState &>()));

template<typename T>
T evalInJsThreadWithConsumer(std::function<void(QtOhos::JsState &, std::function<void(T)>)> evalFunc);

template<typename T>
T evalInJsThreadWithPromise(
    std::function<void(QtOhos::JsState &, QOhosTaskPromise<T>)> evalFunc,
    std::string callerContextName = {});

// Invokes the task inside the Qt thread and blocks the caller's thread until either:
//  - the "continue" function (std::function<void()>, passed as second argument to
//    the task) is called on the Qt side and returns,
//  - timeout occurs (we don't receive response from the finished task within the
//    specified time limit),
//  - deadlock is detected (both Qt and JS thread use synchronous calls at the same
//    time).
// It can be called from the JS thread. Calling it from the Qt thread is illegal.
//
// Returns true iff the caller receives confirmation about finishing the task within
// the time limit.
//
// Note:
// If the function returns false then:
//   - either the task wasn't started at all (timeout or deadlock)
//   - or the task was still running (and may be still running!) in the Qt thread
// after reaching the timeout.
Q_REQUIRED_RESULT bool tryInvokeInQtThreadAndTryWaitForContinue(
    std::function<void(std::function<void()>)> &&task,
    std::chrono::nanoseconds timeout);

template<typename T>
std::enable_if_t<std::is_default_constructible<T>::value, T> &JsState::getAttachedObjectWithLazyCreate()
{
    auto *objectPtr = reinterpret_cast<T *>(
        getAttachedObjectWithLazyCreate(typeid(T), &std::make_shared<T>));
    return *objectPtr;
}

template<typename T>
QNapi::Symbol JsState::getJsSymbolForType()
{
    return getJsSymbolForType(typeid(T));
}

template<typename Func>
auto evalInJsThread(Func &&func, std::string callerContextName) -> decltype(func(std::declval<JsState &>()))
{
    Q_UNUSED(callerContextName);
    return QOhosJsThreadGateway::eval(
        [&func](QOhosJsState &jsState) {
            return func(static_cast<JsState &>(jsState));
        });
}

template<typename T>
T evalInJsThreadWithConsumer(std::function<void(QtOhos::JsState &, std::function<void(T)>)> evalFunc)
{
    return QOhosJsThreadGateway::evalWithConsumer<T>(
        [evalFunc = std::move(evalFunc)](QOhosJsState &jsState, std::function<void(T)> consumer) {
            evalFunc(static_cast<JsState &>(jsState), std::move(consumer));
        });
}

template<typename T>
T evalInJsThreadWithPromise(
    std::function<void(QtOhos::JsState &, QOhosTaskPromise<T>)> evalFunc,
    std::string callerContextName)
{
    return evalInJsThreadWithConsumer<T>(
        [evalFunc = std::move(evalFunc), callerContextName](QtOhos::JsState &jsState, std::function<void(T)> consumer) {
            evalFunc(jsState, QOhosTaskPromise<T>(std::move(consumer), {}, callerContextName));
        });
}

}

template<typename T, typename Func>
std::enable_if_t<
    qohosplugincore_h_detail::isStdOptional<QOhosInvokeResult<Func, T>>,
    QOhosInvokeResult<Func, T>>
qAndThen(const std::optional<T> &opt, Func &&func)
{
    return opt.has_value() ? func(*opt) : QOhosInvokeResult<Func, T>();
}

template<typename T, typename Func>
std::optional<std::remove_cv_t<QOhosInvokeResult<Func, T>>> qTransform(const std::optional<T> &opt, Func &&func)
{
    using TransformedT = std::remove_cv_t<QOhosInvokeResult<Func, T>>;
    return opt.has_value()
        ? std::optional<TransformedT>(func(*opt))
        : std::optional<TransformedT>();
}

template<typename T>
std::optional<T> makeQOhosOptional(const T &value)
{
    return std::optional<T>(value);
}

inline std::nullopt_t makeEmptyQOhosOptional()
{
    return std::nullopt;
}

QT_END_NAMESPACE

#endif
