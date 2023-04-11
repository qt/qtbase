// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRUNNABLE_H
#define QRUNNABLE_H

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qdebug.h>

#include <functional>
#include <type_traits>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QRunnable
{
    bool m_autoDelete = true;

    Q_DISABLE_COPY(QRunnable)
public:
    virtual void run() = 0;

    constexpr QRunnable() noexcept = default;
    virtual ~QRunnable();
#if QT_CORE_REMOVED_SINCE(6, 6)
    static QRunnable *create(std::function<void()> functionToRun);
#endif
    template <typename Callable>
    using if_callable = std::enable_if_t<std::is_invocable_r_v<void, Callable>, bool>;

    template <typename Callable, if_callable<Callable> = true>
    static QRunnable *create(Callable &&functionToRun);
    static QRunnable *create(std::nullptr_t) = delete;

    bool autoDelete() const { return m_autoDelete; }
    void setAutoDelete(bool autoDelete) { m_autoDelete = autoDelete; }

protected:
    // Type erasure, to only instantiate a non-virtual class per Callable:
    class QGenericRunnableHelperBase
    {
        using OpFn = void(*)(QGenericRunnableHelperBase *);
        OpFn runFn;
        OpFn destroyFn;
    protected:
        constexpr explicit QGenericRunnableHelperBase(OpFn fn, OpFn del) noexcept : runFn(fn), destroyFn(del) {}
        ~QGenericRunnableHelperBase() = default;
    public:
        void run() { runFn(this); }
        void destroy() { destroyFn(this); }
    };

    template <typename Callable>
    class QGenericRunnableHelper : public QGenericRunnableHelperBase
    {
        Callable m_functionToRun;
    public:
        template <typename UniCallable>
        QGenericRunnableHelper(UniCallable &&functionToRun) noexcept :
              QGenericRunnableHelperBase(
                      [](QGenericRunnableHelperBase *that) { static_cast<QGenericRunnableHelper*>(that)->m_functionToRun(); },
                      [](QGenericRunnableHelperBase *that) { delete static_cast<QGenericRunnableHelper*>(that); }),
              m_functionToRun(std::forward<UniCallable>(functionToRun))
        {
        }
    };
};

class QGenericRunnable : public QRunnable
{
    QGenericRunnableHelperBase *runHelper;
public:
    QGenericRunnable(QGenericRunnableHelperBase *runner) noexcept: runHelper(runner)
    {
    }
    ~QGenericRunnable() override
    {
        runHelper->destroy();
    }
    void run() override
    {
        runHelper->run();
    }
};

namespace QtPrivate {

template <typename T>
using is_function_pointer = std::conjunction<std::is_pointer<T>, std::is_function<std::remove_pointer_t<T>>>;
template <typename T>
struct is_std_function : std::false_type {};
template <typename T>
struct is_std_function<std::function<T>> : std::true_type {};

} // namespace QtPrivate

template <typename Callable, QRunnable::if_callable<Callable>>
QRunnable *QRunnable::create(Callable &&functionToRun)
{
    bool is_null = false;
    if constexpr(QtPrivate::is_std_function<std::decay_t<Callable>>::value)
        is_null = !functionToRun;

    if constexpr(QtPrivate::is_function_pointer<std::decay_t<Callable>>::value) {
        const void *functionPtr = reinterpret_cast<void *>(functionToRun);
        is_null = !functionPtr;
    }
    if (is_null) {
        qWarning() << "Trying to create null QRunnable. This may stop working.";
        return nullptr;
    }

    return new QGenericRunnable(
            new QGenericRunnableHelper<std::decay_t<Callable>>(
                    std::forward<Callable>(functionToRun)));
}

QT_END_NAMESPACE

#endif
