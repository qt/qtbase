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

#include <QtCore/qglobal.h>
#include <QtCore/qlogging.h>
#include <QtCore/qdebug.h>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#ifndef QT_NO_EXCEPTIONS
#include <stdexcept>
#endif
#include <type_traits>
#include <utility>

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
    QOhosTaskPromise(F &&callable);

    QOhosTaskPromise(QOhosTaskPromise &&) = default;
    QOhosTaskPromise &operator=(QOhosTaskPromise &&) = default;

    QOhosTaskPromise(const QOhosTaskPromise &) = delete;
    QOhosTaskPromise &operator=(const QOhosTaskPromise &) = delete;

    void operator()(Args... args) const;

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

    std::unique_ptr<Callable> m_callable;
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
QOhosTaskPromise<Args...>::QOhosTaskPromise(F &&callable)
    : m_callable(std::make_unique<CallableImpl<std::decay_t<F>>>(std::forward<F>(callable)))
{
}

template<typename... Args>
void QOhosTaskPromise<Args...>::operator()(Args... args) const
{
    if (!m_callable)
        qOhosReportFatalErrorAndAbort("%s: called on empty QOhosTaskPromise", Q_FUNC_INFO);

    m_callable->call(std::forward<Args>(args)...);
}

template<typename... Args>
QOhosTaskPromise<Args...>::operator std::function<void(Args...)>() &&
{
    if (!m_callable)
        return {};

    return [callable = std::shared_ptr<Callable>(std::move(m_callable))](Args... args) {
        callable->call(std::forward<Args>(args)...);
    };
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

QT_END_NAMESPACE

#endif
