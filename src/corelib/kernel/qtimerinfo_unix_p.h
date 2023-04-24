// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTIMERINFO_UNIX_P_H
#define QTIMERINFO_UNIX_P_H

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

#include <QtCore/private/qglobal_p.h>

#include "qabstracteventdispatcher.h"

#include <sys/time.h> // struct timespec
#include <chrono>

QT_BEGIN_NAMESPACE

// internal timer info
struct QTimerInfo {
    QTimerInfo(int timerId, std::chrono::milliseconds msecs, Qt::TimerType type, QObject *obj)
        : interval(msecs), id(timerId), timerType(type), obj(obj)
    {
    }

    std::chrono::steady_clock::time_point timeout; // - when to actually fire
    std::chrono::milliseconds interval = std::chrono::milliseconds{-1}; // - timer interval
    int id = -1; // - timer identifier
    Qt::TimerType timerType; // - timer type
    QObject *obj = nullptr; // - object to receive event
    QTimerInfo **activateRef = nullptr; // - ref from activateTimers
};

class Q_CORE_EXPORT QTimerInfoList
{
public:
    QTimerInfoList();

    std::chrono::steady_clock::time_point currentTime;

    std::optional<std::chrono::milliseconds> timerWait();
    void timerInsert(QTimerInfo *);

    qint64 timerRemainingTime(int timerId);
    std::chrono::milliseconds remainingDuration(int timerId);

    void registerTimer(int timerId, qint64 interval, Qt::TimerType timerType, QObject *object);
    void registerTimer(int timerId, std::chrono::milliseconds interval, Qt::TimerType timerType,
                       QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<QAbstractEventDispatcher::TimerInfo> registeredTimers(QObject *object) const;

    int activateTimers();
    bool hasPendingTimers();

    void clearTimers()
    {
        qDeleteAll(timers);
        timers.clear();
    }

    bool isEmpty() const { return timers.empty(); }

    qsizetype size() const { return timers.size(); }

    auto findTimerById(int timerId)
    {
        auto matchesId = [timerId](const auto &t) { return t->id == timerId; };
        return std::find_if(timers.cbegin(), timers.cend(), matchesId);
    }

private:
    std::chrono::steady_clock::time_point updateCurrentTime();

    // state variables used by activateTimers()
    QTimerInfo *firstTimerInfo = nullptr;
    QList<QTimerInfo *> timers;
};

QT_END_NAMESPACE

#endif // QTIMERINFO_UNIX_P_H
