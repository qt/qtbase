/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QBASICTIMER_H
#define QBASICTIMER_H

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>

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
        : id{qExchange(other.id, 0)}
    {}

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QBasicTimer)

    void swap(QBasicTimer &other) noexcept { std::swap(id, other.id); }

    bool isActive() const noexcept { return id != 0; }
    int timerId() const noexcept { return id; }

    void start(int msec, QObject *obj);
    void start(int msec, Qt::TimerType timerType, QObject *obj);
    void stop();
};
Q_DECLARE_TYPEINFO(QBasicTimer, Q_RELOCATABLE_TYPE);

inline void swap(QBasicTimer &lhs, QBasicTimer &rhs) noexcept { lhs.swap(rhs); }

QT_END_NAMESPACE

#endif // QBASICTIMER_H
