// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

using namespace std::chrono_literals;

//! [0]
    void executeOperation(int msecs)
    {
        QDeadlineTimer deadline(msecs);
        do {
            if (readFromDevice(deadline.remainingTime()))
                break;
            waitForReadyRead(deadline);
        } while (!deadline.hasExpired());
    }
//! [0]

//! [1]
    QDeadlineTimer deadline(30s);
    device->waitForReadyRead(deadline);
    if (deadline.remainingTime<std::chrono::nanoseconds>() > 300ms)
        cleanup();
//! [1]

//! [2]
    auto now = std::chrono::steady_clock::now();
    QDeadlineTimer deadline(now + 1s);
    Q_ASSERT(deadline == now + 1s);
//! [2]

//! [3]
    QDeadlineTimer deadline(250ms);
//! [3]

//! [4]
    deadline.setRemainingTime(250ms);
//! [4]

//! [5]
    mutex.tryLock(deadline.remainingTime());
//! [5]

//! [6]
    qint64 realTimeLeft = deadline.deadline();
    if (realTimeLeft != (std::numeric_limits<qint64>::max)()) {
        realTimeLeft -= QDeadlineTimer::current().deadline();
        // or:
        //QElapsedTimer timer;
        //timer.start();
        //realTimeLeft -= timer.msecsSinceReference();
    }
//! [6]

//! [7]
    qint64 realTimeLeft = deadline.deadlineNSecs();
    if (realTimeLeft != std::numeric_limits<qint64>::max())
        realTimeLeft -= QDeadlineTimer::current().deadlineNSecs();
//! [7]

//! [8]
    return lhs.deadlineNSecs() == rhs.deadlineNSecs();
//! [8]

//! [9]
    return lhs.deadlineNSecs() != rhs.deadlineNSecs();
//! [9]

//! [10]
    return lhs.deadlineNSecs() < rhs.deadlineNSecs();
//! [10]

//! [11]
    return lhs.deadlineNSecs() <= rhs.deadlineNSecs();
//! [11]

//! [12]
    return lhs.deadlineNSecs() > rhs.deadlineNSecs();
//! [12]

//! [13]
    return lhs.deadlineNSecs() >= rhs.deadlineNSecs();
//! [13]
