// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCHRONOTIMER_H
#define QCHRONOTIMER_H

#ifndef QT_NO_QOBJECT

#include <QtCore/qcoreevent.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class QTimerPrivate;
class Q_CORE_EXPORT QChronoTimer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool singleShot READ isSingleShot WRITE setSingleShot
               BINDABLE bindableSingleShot FINAL)
    Q_PROPERTY(std::chrono::nanoseconds interval READ interval WRITE setInterval
               BINDABLE bindableInterval FINAL)
    Q_PROPERTY(std::chrono::nanoseconds remainingTime READ remainingTime FINAL)
    Q_PROPERTY(Qt::TimerType timerType READ timerType WRITE setTimerType
               BINDABLE bindableTimerType FINAL)
    Q_PROPERTY(bool active READ isActive STORED false BINDABLE bindableActive FINAL)

    template <typename Functor>
    using FunctorContext = typename QtPrivate::ContextTypeForFunctor<Functor>::ContextType;

public:
    explicit QChronoTimer(std::chrono::nanoseconds nsec, QObject *parent = nullptr);
    explicit QChronoTimer(QObject *parent = nullptr);
    ~QChronoTimer() override;

    bool isActive() const;
    QBindable<bool> bindableActive();
    Qt::TimerId id() const;

    void setInterval(std::chrono::nanoseconds nsec);
    std::chrono::nanoseconds interval() const;
    QBindable<std::chrono::nanoseconds> bindableInterval();

    std::chrono::nanoseconds remainingTime() const;

    void setTimerType(Qt::TimerType atype);
    Qt::TimerType timerType() const;
    QBindable<Qt::TimerType> bindableTimerType();

    void setSingleShot(bool singleShot);
    bool isSingleShot() const;
    QBindable<bool> bindableSingleShot();

    // singleShot with context
#ifdef Q_QDOC
    template <typename Functor>
    static inline void singleShot(std::chrono::nanoseconds interval,
                                  const QObject *receiver, Functor &&slot);
    template <typename Functor>
    static inline void singleShot(std::chrono::nanoseconds interval interval,
                                  Qt::TimerType timerType,
                                  const QObject *receiver, Functor &&slot);
#else
    template <typename Functor>
    static void singleShot(std::chrono::nanoseconds interval,
                           const FunctorContext<Functor> *receiver, Functor &&slot)
    {
        singleShot(interval, defaultTimerTypeFor(interval), receiver, std::forward<Functor>(slot));
    }
    template <typename Functor>
    static void singleShot(std::chrono::nanoseconds interval, Qt::TimerType timerType,
                           const FunctorContext<Functor> *receiver, Functor &&slot)
    {
        using Prototype = void(*)();
        auto *slotObj = QtPrivate::makeCallableObject<Prototype>(std::forward<Functor>(slot));
        singleShotImpl(interval, timerType, receiver, slotObj);
    }
#endif

    template <typename Functor>
    static void singleShot(std::chrono::nanoseconds interval, Qt::TimerType timerType,
                           Functor &&slot)
    { singleShot(interval, timerType, nullptr, std::forward<Functor>(slot)); }

    template <typename Functor>
    static void singleShot(std::chrono::nanoseconds interval, Functor &&slot)
    {
        singleShot(interval, defaultTimerTypeFor(interval), nullptr, std::forward<Functor>(slot));
    }

    static void singleShot(std::chrono::nanoseconds interval, Qt::TimerType timerType,
                           const QObject *receiver, const char *member);
    static void singleShot(std::chrono::nanoseconds interval, const QObject *receiver,
                           const char *member)
    { singleShot(interval, defaultTimerTypeFor(interval), receiver, member); }

#ifdef Q_QDOC
    template <typename Functor>
    QMetaObject::Connection callOnTimeout(const QObject *context, Functor &&slot,
                                          Qt::ConnectionType connectionType = Qt::AutoConnection);
#else
    template <typename ... Args>
    QMetaObject::Connection callOnTimeout(Args && ...args)
    {
        return QObject::connect(this, &QChronoTimer::timeout, std::forward<Args>(args)... );
    }
#endif

public Q_SLOTS:
    void start();
    void stop();

Q_SIGNALS:
    void timeout(QPrivateSignal);

protected:
    void timerEvent(QTimerEvent *) override;

private:
    Q_DISABLE_COPY(QChronoTimer)

    // QChronoTimer uses QTimerPrivate
    inline QTimerPrivate *d_func() noexcept
    { Q_CAST_IGNORE_ALIGN(return reinterpret_cast<QTimerPrivate *>(qGetPtrHelper(d_ptr));) }
    inline const QTimerPrivate *d_func() const noexcept
    { Q_CAST_IGNORE_ALIGN(return reinterpret_cast<const QTimerPrivate *>(qGetPtrHelper(d_ptr));) }

    // These two functions are inherited from QObject
    int startTimer(std::chrono::nanoseconds) = delete;
    void killTimer(int) = delete;

    static constexpr Qt::TimerType defaultTimerTypeFor(std::chrono::nanoseconds interval) noexcept
    {
        using namespace std::chrono_literals;
        return interval >= 2s ? Qt::CoarseTimer : Qt::PreciseTimer;
    }

    static void singleShotImpl(std::chrono::nanoseconds interval, Qt::TimerType timerType,
                               const QObject *receiver, QtPrivate::QSlotObjectBase *slotObj);
};

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QCHRONOTIMER_H
