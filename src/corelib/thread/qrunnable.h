// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRUNNABLE_H
#define QRUNNABLE_H

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qfunctionaltools_impl.h>

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
    protected:
        enum class Op {
            Run,
            Destroy,
        };
        using OpFn = void* (*)(Op, QGenericRunnableHelperBase *, void*);
        OpFn fn;
    protected:
        constexpr explicit QGenericRunnableHelperBase(OpFn f) noexcept : fn(f) {}
        ~QGenericRunnableHelperBase() = default;
    public:
        void run() { fn(Op::Run, this, nullptr); }
        void destroy() { fn(Op::Destroy, this, nullptr); }
    };

    template <typename Callable>
    class QGenericRunnableHelper : public QGenericRunnableHelperBase,
                                   private QtPrivate::CompactStorage<Callable>
    {
        using Storage = QtPrivate::CompactStorage<Callable>;
        static void *impl(Op op, QGenericRunnableHelperBase *that, [[maybe_unused]] void *arg)
        {
            const auto _this = static_cast<QGenericRunnableHelper*>(that);
            switch (op) {
            case Op::Run:     _this->object()(); break;
            case Op::Destroy: delete _this; break;
            }
            return nullptr;
        }
    public:
        template <typename UniCallable>
        explicit QGenericRunnableHelper(UniCallable &&functionToRun) noexcept
            : QGenericRunnableHelperBase(&impl),
              Storage{std::forward<UniCallable>(functionToRun)}
        {
        }
    };

private:
    static Q_DECL_COLD_FUNCTION QRunnable *warnNullCallable();
};

class QGenericRunnable : public QRunnable
{
    QGenericRunnableHelperBase *runHelper;
public:
    template <typename Callable, if_callable<Callable> = true>
    explicit QGenericRunnable(Callable &&c)
        : runHelper(new QGenericRunnableHelper<std::decay_t<Callable>>(std::forward<Callable>(c)))
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
    if (is_null)
        return warnNullCallable();

    return new QGenericRunnable(std::forward<Callable>(functionToRun));
}

QT_END_NAMESPACE

#endif
