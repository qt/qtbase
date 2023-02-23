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

#include "qplatformdefs.h" // _POSIX_MONOTONIC_CLOCK-0

#include <QtCore/private/qglobal_p.h>

// #define QTIMERINFO_DEBUG

#include "qabstracteventdispatcher.h"

#include <sys/time.h> // struct timeval

QT_BEGIN_NAMESPACE

// internal timer info
struct QTimerInfo {
    int id;           // - timer identifier
    Qt::TimerType timerType; // - timer type
    std::chrono::milliseconds interval; // - timer interval
    timespec timeout;  // - when to actually fire
    QObject *obj;     // - object to receive event
    QTimerInfo **activateRef; // - ref from activateTimers

#ifdef QTIMERINFO_DEBUG
    timeval expected; // when timer is expected to fire
    float cumulativeError;
    uint count;
#endif
};

class Q_CORE_EXPORT QTimerInfoList : public QList<QTimerInfo*>
{
    // state variables used by activateTimers()
    QTimerInfo *firstTimerInfo;

public:
    QTimerInfoList();

    timespec currentTime;
    timespec updateCurrentTime();

    bool timerWait(timespec &);
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

    QList::const_iterator findTimerById(int timerId) const
    {
        auto matchesId = [timerId](const QTimerInfo *t) { return t->id == timerId; };
        return std::find_if(cbegin(), cend(), matchesId);
    }
};

QT_END_NAMESPACE

#endif // QTIMERINFO_UNIX_P_H
