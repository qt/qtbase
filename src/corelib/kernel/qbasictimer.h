// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBASICTIMER_H
#define QBASICTIMER_H

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>

#include <chrono>

QT_BEGIN_NAMESPACE


class QObject;

class Q_CORE_EXPORT QBasicTimer
{
    int id;
    Q_DISABLE_COPY(QBasicTimer)

public:
    constexpr QBasicTimer() noexcept : id{0} {}
    inline ~QBasicTimer() { if (id) stop(); }

    QBasicTimer(QBasicTimer &&other) noexcept
        : id{std::exchange(other.id, 0)}
    {}

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QBasicTimer)

    void swap(QBasicTimer &other) noexcept { std::swap(id, other.id); }

    bool isActive() const noexcept { return id != 0; }
    int timerId() const noexcept { return id; }
    QT_CORE_INLINE_SINCE(6, 5)
    void start(int msec, QObject *obj);
    QT_CORE_INLINE_SINCE(6, 5)
    void start(int msec, Qt::TimerType timerType, QObject *obj);
    void start(std::chrono::milliseconds duration, QObject *obj);
    void start(std::chrono::milliseconds duration, Qt::TimerType timerType, QObject *obj);
    void stop();
};
Q_DECLARE_TYPEINFO(QBasicTimer, Q_RELOCATABLE_TYPE);

#if QT_CORE_INLINE_IMPL_SINCE(6, 5)
void QBasicTimer::start(int msec, QObject *obj)
{
    start(std::chrono::milliseconds{msec}, obj);
}

void QBasicTimer::start(int msec, Qt::TimerType t, QObject *obj)
{
    start(std::chrono::milliseconds{msec}, t, obj);
}
#endif

inline void swap(QBasicTimer &lhs, QBasicTimer &rhs) noexcept { lhs.swap(rhs); }

QT_END_NAMESPACE

#endif // QBASICTIMER_H
