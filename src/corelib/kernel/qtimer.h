// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTIMER_H
#define QTIMER_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_QOBJECT

#include <QtCore/qbasictimer.h> // conceptual inheritance
#include <QtCore/qobject.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class QTimerPrivate;
class Q_CORE_EXPORT QTimer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool singleShot READ isSingleShot WRITE setSingleShot BINDABLE bindableSingleShot)
    Q_PROPERTY(int interval READ interval WRITE setInterval BINDABLE bindableInterval)
    Q_PROPERTY(int remainingTime READ remainingTime)
    Q_PROPERTY(Qt::TimerType timerType READ timerType WRITE setTimerType BINDABLE bindableTimerType)
    Q_PROPERTY(bool active READ isActive STORED false BINDABLE bindableActive)
public:
    explicit QTimer(QObject *parent = nullptr);
    ~QTimer();

    bool isActive() const;
    QBindable<bool> bindableActive();
    int timerId() const;

    void setInterval(int msec);
    int interval() const;
    QBindable<int> bindableInterval();

    int remainingTime() const;

    void setTimerType(Qt::TimerType atype);
    Qt::TimerType timerType() const;
    QBindable<Qt::TimerType> bindableTimerType();

    void setSingleShot(bool singleShot);
    bool isSingleShot() const;
    QBindable<bool> bindableSingleShot();

    static void singleShot(int msec, const QObject *receiver, const char *member);
    static void singleShot(int msec, Qt::TimerType timerType, const QObject *receiver, const char *member);

    // singleShot with context
#ifdef Q_QDOC
    template <typename Duration, typename Functor>
    static inline void singleShot(Duration interval, const QObject *receiver, Functor &&slot);
    template <typename Duration, typename Functor>
    static inline void singleShot(Duration interval, Qt::TimerType timerType,
                                  const QObject *receiver, Functor &&slot);
#else
    template <typename Duration, typename Functor>
    static inline void singleShot(Duration interval,
                                  const typename QtPrivate::ContextTypeForFunctor<Functor>::ContextType *receiver,
                                  Functor &&slot)
    {
        singleShot(interval, defaultTypeFor(interval), receiver, std::forward<Functor>(slot));
    }
    template <typename Duration, typename Functor>
    static inline void singleShot(Duration interval, Qt::TimerType timerType,
                                  const typename QtPrivate::ContextTypeForFunctor<Functor>::ContextType *receiver,
                                  Functor &&slot)
    {
        using Prototype = void(*)();
        singleShotImpl(interval, timerType, receiver,
                       QtPrivate::makeCallableObject<Prototype>(std::forward<Functor>(slot)));
    }
#endif

    // singleShot without context
    template <typename Duration, typename Functor>
    static inline void singleShot(Duration interval, Functor &&slot)
    {
        singleShot(interval, defaultTypeFor(interval), nullptr, std::forward<Functor>(slot));
    }
    template <typename Duration, typename Functor>
    static inline void singleShot(Duration interval, Qt::TimerType timerType, Functor &&slot)
    {
        singleShot(interval, timerType, nullptr, std::forward<Functor>(slot));
    }

#ifdef Q_QDOC
    template <typename Functor>
    QMetaObject::Connection callOnTimeout(Functor &&slot);
    template <typename Functor>
    QMetaObject::Connection callOnTimeout(const QObject *context, Functor &&slot, Qt::ConnectionType connectionType = Qt::AutoConnection);
#else
    template <typename ... Args>
    QMetaObject::Connection callOnTimeout(Args && ...args)
    {
        return QObject::connect(this, &QTimer::timeout, std::forward<Args>(args)... );
    }

#endif

public Q_SLOTS:
    void start(int msec);

    void start();
    void stop();

Q_SIGNALS:
    void timeout(QPrivateSignal);

public:
    void setInterval(std::chrono::milliseconds value)
    {
        setInterval(int(value.count()));
    }

    std::chrono::milliseconds intervalAsDuration() const
    {
        return std::chrono::milliseconds(interval());
    }

    std::chrono::milliseconds remainingTimeAsDuration() const
    {
        return std::chrono::milliseconds(remainingTime());
    }

    static void singleShot(std::chrono::milliseconds value, const QObject *receiver, const char *member)
    {
        singleShot(int(value.count()), receiver, member);
    }

    static void singleShot(std::chrono::milliseconds value, Qt::TimerType timerType, const QObject *receiver, const char *member)
    {
        singleShot(int(value.count()), timerType, receiver, member);
    }

    void start(std::chrono::milliseconds value)
    {
        start(int(value.count()));
    }

protected:
    void timerEvent(QTimerEvent *) override;

private:
    Q_DISABLE_COPY(QTimer)
    Q_DECLARE_PRIVATE(QTimer)

    inline int startTimer(int){ return -1;}
    inline void killTimer(int){}

    static constexpr Qt::TimerType defaultTypeFor(int msecs) noexcept
    { return defaultTypeFor(std::chrono::milliseconds{msecs}); }

    static constexpr Qt::TimerType defaultTypeFor(std::chrono::milliseconds interval) noexcept
    {
        // coarse timers are worst in their first firing
        // so we prefer a high precision timer for something that happens only once
        // unless the timeout is too big, in which case we go for coarse anyway
        using namespace std::chrono_literals;
        return interval >= 2s ? Qt::CoarseTimer : Qt::PreciseTimer;
    }

    static void singleShotImpl(int msec, Qt::TimerType timerType,
                               const QObject *receiver, QtPrivate::QSlotObjectBase *slotObj);

    static void singleShotImpl(std::chrono::milliseconds interval, Qt::TimerType timerType,
                               const QObject *receiver, QtPrivate::QSlotObjectBase *slotObj)
    {
        singleShotImpl(int(interval.count()),
                       timerType, receiver, slotObj);
    }
};

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QTIMER_H
