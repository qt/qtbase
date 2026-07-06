// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCORE_OHOS_P_H
#define QCORE_OHOS_P_H

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

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtCore/qmutex.h>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <pthread.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhos {

class QObjectThreadSafeRef;

template<typename Enum>
struct OhosEnumMeta
{
};

}

class Q_CORE_EXPORT QOhosJsState
{
public:
    QOhosJsState(const QOhosJsState &) = delete;
    QOhosJsState &operator=(const QOhosJsState &) = delete;

    virtual ~QOhosJsState();

    virtual napi_env env() = 0;

    virtual QNapi::Object defaultWindowStageOrEmpty() = 0;
    virtual QNapi::Object defaultUiContextOrEmpty() = 0;

    virtual std::optional<QNapi::Object> tryGetQAbilityByQWindow(QtOhos::QObjectThreadSafeRef qwindow) = 0;
    virtual std::optional<QNapi::Object> defaultQAbility() = 0;

    virtual QNapi::Object appLaunchWant() = 0;
    virtual std::optional<QNapi::Object> optAppLaunchParam() = 0;

    virtual void startAppProcess(
        const std::string &processId, QNapi::Object requestWant,
        QNapi::Object optStartOptions, std::function<void(QOhosJsState &)> continueFunc) = 0;

    virtual void startNoUiChildProcess(const std::string &libraryName, const std::vector<std::string> &args) = 0;

    virtual void addNewWantConsumer(
        QOhosConsumer<QOhosJsState &, QNapi::Object, QNapi::Object> wantConsumer) = 0;

    // requestsHandler's parameters: jsState, wantParams, resultConsumer
    virtual void setOnContinueRequestsHandler(
        QNapi::Object qAbility,
        std::function<void(QOhosJsState &, QNapi::Object, QOhosConsumer<QOhosJsState &, QNapi::Number>)> requestsHandler) = 0;

    virtual void setDestroyFromSystemAllowed(QNapi::Object qAbility, bool destroyAllowed) = 0;

    template<typename T = QNapi::Value>
    T eval(const std::string &expr, const std::vector<QNapi::ValueWrapper> &exprArgs = {});

    QNapi::Promise evalToPromiseOrRejectOnThrow(
        const std::string &expr, const std::vector<QNapi::ValueWrapper> &exprArgs = {});

    template<typename Enum>
    QNapi::Number mapOhosEnumToJs(Enum enumValue);

    template<typename Enum>
    std::optional<Enum> tryMapOhosEnumFromJs(QNapi::Number enumJsValue);

    template<typename Enum>
    Enum mapOhosEnumFromJs(QNapi::Number enumJsValue);

protected:
    struct OhosEnumInfo
    {
        std::string fullTypeName;
        std::vector<std::pair<int, const char *>> enumeratorsNames;
    };

    QOhosJsState();

private:
    template<typename Enum, typename = void>
    struct OhosEnumFullTypeNameFetcher
    {
        static std::string fullTypeName()
        {
            return QtOhos::OhosEnumMeta<Enum>::moduleName + std::string(".") + QtOhos::OhosEnumMeta<Enum>::typeName;
        }
    };

    template<typename Enum>
    struct OhosEnumFullTypeNameFetcher<Enum, decltype(static_cast<void>(&QtOhos::OhosEnumMeta<Enum>::fullTypeName))>
    {
        static std::string fullTypeName()
        {
            return QtOhos::OhosEnumMeta<Enum>::fullTypeName;
        }
    };

    template<typename Enum>
    static OhosEnumInfo makeOhosEnumInfo();

    virtual std::tuple<QNapi::Object, std::string> extractModuleFromEvalExpr(const std::string &expr) = 0;
    virtual QNapi::Number mapOhosEnumToJs(int enumValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)()) = 0;
    virtual std::optional<int> tryMapOhosEnumFromJs(QNapi::Number enumJsValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)()) = 0;
    virtual int mapOhosEnumFromJs(QNapi::Number enumJsValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)()) = 0;
};

class Q_CORE_EXPORT QOhosCallbackInfo : public QNapi::CallbackInfo
{
public:
    using QNapi::CallbackInfo::CallbackInfo;

    QOhosJsState &jsState() const;
};

namespace QOhosJsThreadGateway {

Q_CORE_EXPORT void invoke(std::function<void(QOhosJsState &)> task);

Q_CORE_EXPORT void invokeAndWaitForContinue(
    std::function<void(QOhosJsState &, QOhosTaskPromise<>)> &&task,
    std::string callerContextName = {});

Q_CORE_EXPORT void runAndWait(
    const std::function<void(QOhosJsState &)> &task,
    std::string callerContextName = {});

template<typename Func>
auto eval(Func &&func, std::string callerContextName = {})
    -> decltype(func(std::declval<QOhosJsState &>()));

template<typename T>
T evalWithPromise(
    std::function<void(QOhosJsState &, QOhosTaskPromise<T>)> evalFunc,
    std::string callerContextName = {});

// Backward-compatible alias for evalWithPromise(); kept so existing callers
// passing a result consumer keep compiling.
template<typename T>
T evalWithConsumer(std::function<void(QOhosJsState &, std::function<void(T)>)> evalFunc);

}

class Q_CORE_EXPORT QOhosJsThreadOps
{
public:
    virtual ~QOhosJsThreadOps();

    virtual QOhosJsState &jsState() = 0;

    virtual void invoke(std::function<void(QOhosJsState &)> task) = 0;
    virtual void invokeAndWaitForContinue(
        std::function<void(QOhosJsState &, QOhosTaskPromise<>)> &&task,
        std::string callerContextName) = 0;
    virtual void runAndWait(
        const std::function<void(QOhosJsState &)> &task,
        std::string callerContextName) = 0;

    static void registerInstance(QOhosJsThreadOps *ops);
    static QOhosJsThreadOps &instance();

protected:
    QOhosJsThreadOps();
};

namespace QtOhos {

class Q_CORE_EXPORT QObjectThreadSafeRef
{
public:
    QObjectThreadSafeRef();
    QObjectThreadSafeRef(QPointer<QObject> obj);

    QObjectThreadSafeRef(const QObjectThreadSafeRef &other);
    QObjectThreadSafeRef &operator=(const QObjectThreadSafeRef &other);

    bool operator==(const QObjectThreadSafeRef &other) const;
    bool operator!=(const QObjectThreadSafeRef &other) const;

    std::string refName() const;

    // it may be called only inside Qt thread that created the reference
    QPointer<QObject> data() const;

    void visitInQtThreadIfAlive(std::function<void(QObject &)> visitFunc) const;

private:
    struct QObjectRef
    {
        QPointer<QObject> obj;
        std::string refName;
    };

    Q_CONSTINIT inline static QBasicMutex refsMapMutex;
    Q_CONSTINIT inline static std::uint64_t refsMapInsertCounter = 0;

    pthread_t m_creatorThread;
    std::string m_refName;
    std::weak_ptr<QObjectRef> m_weakObjRef;

public:
    using ObjectRefsMap = std::map<QObject *, std::shared_ptr<QObjectRef>>;
};

template<typename T>
class QThreadSafeRef : private QObjectThreadSafeRef
{
public:
    static_assert(std::is_base_of<QObject, T>::value, "The class supports QObject subtypes only");

    QThreadSafeRef();
    QThreadSafeRef(QPointer<T> obj);

    QThreadSafeRef(const QThreadSafeRef<T> &other);
    QThreadSafeRef &operator=(const QThreadSafeRef<T> &other);

    QObjectThreadSafeRef toQObjectThreadSafeRef() const;

    std::string refName() const;

    // it may be called only inside Qt thread that created the reference
    QPointer<T> data() const;

    void visitInQtThreadIfAlive(std::function<void(T &)> visitFunc) const;

private:
    QObjectThreadSafeRef m_ref;
};

class QtState
{
public:
    QtState(const QtState &) = delete;
    QtState &operator=(const QtState &) = delete;

    virtual ~QtState();

    virtual bool isQtThread() const = 0;

    virtual void invokeTask(std::function<void()> &&task) = 0;

protected:
    QtState();
};

template<typename T>
QThreadSafeRef<T> makeQThreadSafeRef(T *obj);

// this function should be called once from Qt thread at some point during startup
Q_CORE_EXPORT void initQtThreadState();

// Creates a proxy for the base shared_ptr which uses custom deleter to ensure
// that the underlying shared_ptr is destroyed (synchronously) in the JS thread.
// This ensures that the managed object will be also destroyed in the JS thread,
// as long as the caller passes the only existing shared_ptr instance managing
// that object to the function. This is the caller's responsibility to do this
// (preferably by using a call to some std::shared_ptr<T> factory function as
// the function's argument).
template<typename T>
std::shared_ptr<T> makeProxyWithJsThreadDeleter(std::shared_ptr<T> &&baseSharedPtr);

// invokes the task inside the Qt thread, can be called from any thread at any time
Q_CORE_EXPORT void invokeInQtThread(std::function<void()> task);

Q_CORE_EXPORT QtState &getQtState();

// logs error message with supplied prefix and error details extracted from
// JS error object passed as an argument via CallbackInfo
Q_CORE_EXPORT void logJsCallbackError(const QOhosCallbackInfo &cbInfo, const char *errorMessagePrefix);

// Creates JS callback that expects JS error object as a JS argument
// and logs details of the error. The logged message includes provided
// call context string (usually the name of called method).
Q_CORE_EXPORT std::function<void(const QOhosCallbackInfo &)> makeErrorLoggingJsCallback(std::string callContext);

template<typename T>
QThreadSafeRef<T> makeQThreadSafeRef(T *obj)
{
    return QThreadSafeRef<T>(obj);
}

template<typename T>
QThreadSafeRef<T>::QThreadSafeRef() = default;

template<typename T>
QThreadSafeRef<T>::QThreadSafeRef(QPointer<T> obj)
    : QObjectThreadSafeRef(obj.data())
{
}

template<typename T>
QThreadSafeRef<T>::QThreadSafeRef(const QThreadSafeRef<T> &other) = default;

template<typename T>
QThreadSafeRef<T> &QThreadSafeRef<T>::operator=(const QThreadSafeRef<T> &other) = default;

template<typename T>
std::string QThreadSafeRef<T>::refName() const
{
    return QObjectThreadSafeRef::refName();
}

template<typename T>
QObjectThreadSafeRef QThreadSafeRef<T>::toQObjectThreadSafeRef() const
{
    return static_cast<const QObjectThreadSafeRef &>(*this);
}

template<typename T>
QPointer<T> QThreadSafeRef<T>::data() const
{
    return static_cast<T *>(
        static_cast<QObject *>(
            QObjectThreadSafeRef::data()));
}

template<typename T>
void QThreadSafeRef<T>::visitInQtThreadIfAlive(std::function<void(T &)> visitFunc) const
{
    QObjectThreadSafeRef::visitInQtThreadIfAlive(
        [visitFunc = std::move(visitFunc)](QObject &obj) {
            visitFunc(static_cast<T &>(obj));
        });
}

template<typename T>
std::shared_ptr<T> makeProxyWithJsThreadDeleter(std::shared_ptr<T> &&baseSharedPtr)
{
    auto *baseRawPtr = baseSharedPtr.get();
    return std::shared_ptr<T>(
        baseRawPtr,
        [baseSharedPtr = std::move(baseSharedPtr)](T *) mutable {
            QOhosJsThreadGateway::runAndWait(
                [&](QOhosJsState &) {
                    baseSharedPtr.reset();
                },
                Q_FUNC_INFO);
        });
}

}

template<typename T>
T QOhosJsState::eval(const std::string &expr, const std::vector<QNapi::ValueWrapper> &exprArgs)
{
    using namespace std::string_literals;

    QNapi::Object module;
    std::string subExpr;
    std::tie(module, subExpr) = extractModuleFromEvalExpr(expr);

    return !subExpr.empty()
        ? module.eval<T>(subExpr, exprArgs)
        : QNapi::checkedCast<T>(
            module,
            [&]() {
                return "module '"s + expr + "'"s;
            });
}

inline QNapi::Promise QOhosJsState::evalToPromiseOrRejectOnThrow(
    const std::string &expr, const std::vector<QNapi::ValueWrapper> &exprArgs)
{
    QNapi::Object module;
    std::string subExpr;
    std::tie(module, subExpr) = extractModuleFromEvalExpr(expr);

    return module.evalToPromiseOrRejectOnThrow(subExpr, exprArgs);
}

template<typename Enum>
QNapi::Number QOhosJsState::mapOhosEnumToJs(Enum enumValue)
{
    return mapOhosEnumToJs(static_cast<int>(enumValue), typeid(Enum), &makeOhosEnumInfo<Enum>);
}

template<typename Enum>
std::optional<Enum> QOhosJsState::tryMapOhosEnumFromJs(QNapi::Number enumJsValue)
{
    auto intValue = tryMapOhosEnumFromJs(enumJsValue, typeid(Enum), &makeOhosEnumInfo<Enum>);
    return intValue.has_value()
        ? std::optional(static_cast<Enum>(intValue.value()))
        : std::nullopt;
}

template<typename Enum>
Enum QOhosJsState::mapOhosEnumFromJs(QNapi::Number enumJsValue)
{
    return static_cast<Enum>(mapOhosEnumFromJs(enumJsValue, typeid(Enum), &makeOhosEnumInfo<Enum>));
}

template<typename Enum>
QOhosJsState::OhosEnumInfo QOhosJsState::makeOhosEnumInfo()
{
    static const auto enumEnumeratorsNames = QtOhos::OhosEnumMeta<Enum>::enumeratorsNames;

    std::vector<std::pair<int, const char *>> intEnumeratorsNames;
    std::transform(
        enumEnumeratorsNames.begin(), enumEnumeratorsNames.end(),
        std::back_inserter(intEnumeratorsNames),
        [](const auto &valueNamePair) {
            return std::pair<int, const char *>(static_cast<int>(valueNamePair.first), valueNamePair.second);
        });

    return OhosEnumInfo {
        .fullTypeName = OhosEnumFullTypeNameFetcher<Enum>::fullTypeName(),
        .enumeratorsNames = std::move(intEnumeratorsNames),
    };
}

template<typename Func>
auto QOhosJsThreadGateway::eval(Func &&func, std::string callerContextName)
    -> decltype(func(std::declval<QOhosJsState &>()))
{
    using Result = decltype(func(std::declval<QOhosJsState &>()));
    static_assert(
        !(std::is_class<Result>::value
            && (
                std::is_convertible<Result, napi_value>::value
                || std::is_convertible<Result, napi_ref>::value)),
        "NAPI values/references must not be accessed outside the JS thread");

    std::unique_ptr<Result> result;
    runAndWait(
        [&](QOhosJsState &jsState) {
            result = std::make_unique<Result>(func(jsState));
        },
        std::move(callerContextName));
    return std::move(*result);
}

template<typename T>
T QOhosJsThreadGateway::evalWithPromise(
    std::function<void(QOhosJsState &, QOhosTaskPromise<T>)> evalFunc,
    std::string callerContextName)
{
    static_assert(
        !(std::is_class<T>::value
            && (
                std::is_convertible<T, napi_value>::value
                || std::is_convertible<T, napi_ref>::value)),
        "NAPI values/references must not be accessed outside the JS thread");

    std::unique_ptr<T> result;
    invokeAndWaitForContinue(
        [&](QOhosJsState &jsState, QOhosTaskPromise<> taskPromise) {
            evalFunc(
                jsState,
                QOhosTaskPromise<T>(
                    [&result, taskPromise = std::move(taskPromise)](T value) {
                        result = std::make_unique<T>(std::move(value));
                        taskPromise();
                    },
                    {},
                    std::move(callerContextName)));
        },
        callerContextName);

    return std::move(*result);
}

template<typename T>
T QOhosJsThreadGateway::evalWithConsumer(std::function<void(QOhosJsState &, std::function<void(T)>)> evalFunc)
{
    return evalWithPromise<T>(
        [evalFunc = std::move(evalFunc)](QOhosJsState &jsState, QOhosTaskPromise<T> resultPromise) {
            evalFunc(jsState, std::move(resultPromise));
        });
}

QT_END_NAMESPACE

#endif
