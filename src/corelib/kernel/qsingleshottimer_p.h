// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSINGLESHOTTIMER_P_H
#define QSINGLESHOTTIMER_P_H

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

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"
#include "qmetaobject_p.h"
#include "private/qnumeric_p.h"

#include <chrono>

QT_BEGIN_NAMESPACE

class QSingleShotTimer : public QObject
{
    Q_OBJECT

    Qt::TimerId timerId = Qt::TimerId::Invalid;

public:
    // use the same duration type
    using Duration = QAbstractEventDispatcher::Duration;

    explicit QSingleShotTimer(Duration interval, Qt::TimerType timerType,
                              const QObject *r, const char *member);
    explicit QSingleShotTimer(Duration interval, Qt::TimerType timerType,
                              const QObject *r, QtPrivate::QSlotObjectBase *slotObj);
    ~QSingleShotTimer() override;

    void startTimerForReceiver(Duration interval, Qt::TimerType timerType,
                               const QObject *receiver);

    static Duration fromMsecs(std::chrono::milliseconds ms)
    {
        using namespace std::chrono;
        using ratio = std::ratio_divide<std::milli, Duration::period>;
        static_assert(ratio::den == 1);

        Duration::rep r;
        if (qMulOverflow<ratio::num>(ms.count(), &r)) {
            qWarning("QTimer::singleShot(std::chrono::milliseconds, ...): "
                     "interval argument overflowed when converted to nanoseconds.");
            return Duration::max();
        }
        return Duration{r};
    }
Q_SIGNALS:
    void timeout();

private:
    void timerEvent(QTimerEvent *) override;
};

QT_END_NAMESPACE

#endif // QSINGLESHOTTIMER_P_H
