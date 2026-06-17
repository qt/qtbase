// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLUGINCORE_H
#define QOHOSPLUGINCORE_H

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qobject.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#ifndef QT_NO_EXCEPTIONS
#include <stdexcept>
#endif
#include <string>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

template<typename T>
class QOhosOptional;

namespace qohosplugincore_h_detail {

template<typename A, typename B, typename = void>
struct HasEqualityComparatorT : std::false_type {};

template<typename A, typename B>
struct HasEqualityComparatorT<A, B, decltype(void(std::declval<A>() == std::declval<B>()))>
    : std::true_type {};

template<typename A, typename B>
constexpr bool hasEqualityComparator = HasEqualityComparatorT<A, B>::value;

template<typename>
struct IsQOhosOptional : std::false_type {};

template<typename T>
struct IsQOhosOptional<QOhosOptional<T>> : std::true_type {};

template<typename T>
constexpr bool isQOhosOptional = IsQOhosOptional<T>::value;

template<typename>
struct IsStdOptional : std::false_type {};

template<typename T>
struct IsStdOptional<std::optional<T>> : std::true_type {};

template<typename T>
constexpr bool isStdOptional = IsStdOptional<T>::value;

}

template<typename T>
class QOhosOptional
{
    static_assert(
        std::is_copy_constructible<T>::value && std::is_copy_assignable<T>::value,
        "Only copyable types are supported");

public:
    explicit QOhosOptional(const T &value);
    QOhosOptional(const QOhosOptional<void> &empty);
    template<typename U, std::enable_if_t<std::is_same_v<U, T>, int> = 0>
    QOhosOptional(const std::optional<U> &other);

    QOhosOptional() = default;
    QOhosOptional(const QOhosOptional<T> &other);
    QOhosOptional<T> &operator=(const QOhosOptional<T> &other);
    QOhosOptional<T> &operator=(const T &value);
    QOhosOptional<T> &operator=(const QOhosOptional<void> &empty);
    template<typename U, std::enable_if_t<std::is_same_v<U, T>, int> = 0>
    QOhosOptional<T> &operator=(const std::optional<U> &other);

    ~QOhosOptional();

    bool hasValue() const;
    T valueOr(const T &fallback) const;

    const T &value() const;
    T &value();

    void reset();

    template<typename... Args>
    T &emplace(Args &&...args);

private:
    T &storedValueRef();
    const T &storedValueRef() const;
    template<typename... InitArgs>
    void initializeStoredValue(InitArgs &&...initArgs);

    std::aligned_storage_t<sizeof(T), alignof(T)> m_rawStoredValue;
    bool m_hasValue = false;
};

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator==(const QOhosOptional<T> &lhs, const QOhosOptional<U> &rhs);

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator!=(const QOhosOptional<T> &lhs, const QOhosOptional<U> &rhs);

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator==(const QOhosOptional<T> &lhs, const U &rhs);

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator!=(const QOhosOptional<T> &lhs, const U &rhs);

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator==(const T &lhs, const QOhosOptional<U> &rhs);

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator!=(const T &lhs, const QOhosOptional<U> &rhs);

template<typename T, typename Func>
std::enable_if_t<
    qohosplugincore_h_detail::isQOhosOptional<QOhosInvokeResult<Func, T>>,
    QOhosInvokeResult<Func, T>>
qAndThen(const QOhosOptional<T> &opt, Func &&func);

template<typename T, typename Func>
QOhosOptional<QOhosInvokeResult<Func, T>> qTransform(const QOhosOptional<T> &opt, Func &&func);

template<typename T, typename Func>
std::enable_if_t<
    qohosplugincore_h_detail::isStdOptional<QOhosInvokeResult<Func, T>>,
    QOhosInvokeResult<Func, T>>
qAndThen(const std::optional<T> &opt, Func &&func);

template<typename T, typename Func>
std::optional<std::remove_cv_t<QOhosInvokeResult<Func, T>>> qTransform(const std::optional<T> &opt, Func &&func);

template<>
class QOhosOptional<void>
{
};

template<typename T>
QOhosOptional<T> makeQOhosOptional(const T &value);

QOhosOptional<void> makeEmptyQOhosOptional();

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
        QNapi::Object optStartOptions) = 0;

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
        QNapi::Object optStartOptions = {}) = 0;

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

enum class QOhosAbilityOnContinueResult
{
    AGREE,
    REJECT,
    MISMATCH,
};

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

template<>
struct OhosEnumMeta<QOhosAbilityOnContinueResult>
{
    static constexpr const char *fullTypeName = "@ohos.app.ability.AbilityConstant.OnContinueResult";
    static constexpr std::array<std::pair<QOhosAbilityOnContinueResult, const char *>, 3> enumeratorsNames = {{
        {QOhosAbilityOnContinueResult::AGREE, "AGREE"},
        {QOhosAbilityOnContinueResult::REJECT, "REJECT"},
        {QOhosAbilityOnContinueResult::MISMATCH, "MISMATCH"},
    }};
};

}

template<typename T>
QOhosOptional<T>::QOhosOptional(const T &value)
{
    initializeStoredValue(value);
}

template<typename T>
QOhosOptional<T>::QOhosOptional(const QOhosOptional<void> &)
{
}

template<typename T>
QOhosOptional<T>::QOhosOptional(const QOhosOptional<T> &other)
{
    if (other.m_hasValue)
        initializeStoredValue(other.storedValueRef());
}

template<typename T>
template<typename U, std::enable_if_t<std::is_same_v<U, T>, int>>
QOhosOptional<T>::QOhosOptional(const std::optional<U> &other)
{
    if (other.has_value())
        initializeStoredValue(*other);
}

template<typename T>
QOhosOptional<T> &QOhosOptional<T>::operator=(const QOhosOptional<T> &other)
{
    if (&other == this)
        return *this;

    if (other.m_hasValue) {
        if (m_hasValue)
            storedValueRef() = other.storedValueRef();
        else
            initializeStoredValue(other.storedValueRef());
    } else {
        reset();
    }

    return *this;
}

template<typename T>
QOhosOptional<T> &QOhosOptional<T>::operator=(const T &value)
{
    *this = QOhosOptional<T>(value);
    return *this;
}

template<typename T>
QOhosOptional<T> &QOhosOptional<T>::operator=(const QOhosOptional<void> &)
{
    reset();
    return *this;
}

template<typename T>
template<typename U, std::enable_if_t<std::is_same_v<U, T>, int>>
QOhosOptional<T> &QOhosOptional<T>::operator=(const std::optional<U> &other)
{
    if (other.has_value()) {
        if (m_hasValue)
            storedValueRef() = *other;
        else
            initializeStoredValue(*other);
    } else {
        reset();
    }
    return *this;
}

template<typename T>
QOhosOptional<T>::~QOhosOptional()
{
    reset();
}

template<typename T>
void QOhosOptional<T>::reset()
{
    if (m_hasValue) {
        m_hasValue = false;
        storedValueRef().~T();
    }
}

template<typename T>
T &QOhosOptional<T>::storedValueRef()
{
    return *reinterpret_cast<T *>(&m_rawStoredValue);
}

template<typename T>
const T &QOhosOptional<T>::storedValueRef() const
{
    return *reinterpret_cast<const T *>(&m_rawStoredValue);
}

template<typename T>
template<typename... InitArgs>
void QOhosOptional<T>::initializeStoredValue(InitArgs &&...initArgs)
{
    new (&m_rawStoredValue) T(std::forward<InitArgs...>(initArgs...));
    m_hasValue = true;
}

template<typename T>
bool QOhosOptional<T>::hasValue() const
{
    return m_hasValue;
}

template<typename T>
T QOhosOptional<T>::valueOr(const T &fallback) const
{
    return hasValue() ? storedValueRef() : fallback;
}

template<typename T>
const T &QOhosOptional<T>::value() const
{
    if (!m_hasValue) {
#ifndef QT_NO_EXCEPTIONS
        throw std::runtime_error("Can't access value inside empty QOhosOptional<>");
#else
        qFatal("Can't access value inside empty QOhosOptional<>");
        std::abort();
#endif
    }

    return storedValueRef();
}

template<typename T>
T &QOhosOptional<T>::value()
{
    if (!m_hasValue) {
#ifndef QT_NO_EXCEPTIONS
        throw std::runtime_error("Can't access value inside empty QOhosOptional<>");
#else
        qFatal("Can't access value inside empty QOhosOptional<>");
        std::abort();
#endif
    }

    return storedValueRef();
}

template<typename T>
template<typename... Args>
T &QOhosOptional<T>::emplace(Args &&...args)
{
    reset();
    initializeStoredValue(std::forward<Args...>(args)...);
    return storedValueRef();
}

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator==(const QOhosOptional<T> &lhs, const QOhosOptional<U> &rhs)
{
    return (lhs.hasValue() && rhs.hasValue())
        ? lhs.value() == rhs.value()
        : (!lhs.hasValue() && !rhs.hasValue());
}

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator!=(const QOhosOptional<T> &lhs, const QOhosOptional<U> &rhs)
{
    return !(lhs == rhs);
}

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator==(const QOhosOptional<T> &lhs, const U &rhs)
{
    return lhs.hasValue() && lhs.value() == rhs;
}

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator!=(const QOhosOptional<T> &lhs, const U &rhs)
{
    return !(lhs == rhs);
}

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator==(const T &lhs, const QOhosOptional<U> &rhs)
{
    return rhs.hasValue() && lhs == rhs.value();
}

template<typename T, typename U>
std::enable_if_t<qohosplugincore_h_detail::hasEqualityComparator<T, U>, bool>
operator!=(const T &lhs, const QOhosOptional<U> &rhs)
{
    return !(lhs == rhs);
}

template<typename T, typename Func>
std::enable_if_t<
    qohosplugincore_h_detail::isQOhosOptional<QOhosInvokeResult<Func, T>>,
    QOhosInvokeResult<Func, T>>
qAndThen(const QOhosOptional<T> &opt, Func &&func)
{
    return opt.hasValue() ? func(opt.value()) : QOhosInvokeResult<Func, T>();
}

template<typename T, typename Func>
QOhosOptional<QOhosInvokeResult<Func, T>> qTransform(const QOhosOptional<T> &opt, Func &&func)
{
    using TransformedT = QOhosInvokeResult<Func, T>;
    return opt.hasValue()
        ? QOhosOptional<TransformedT>(func(opt.value()))
        : QOhosOptional<TransformedT>();
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
QOhosOptional<T> makeQOhosOptional(const T &value)
{
    return QOhosOptional<T>(value);
}

inline QOhosOptional<void> makeEmptyQOhosOptional()
{
    return {};
}

QT_END_NAMESPACE

#endif
