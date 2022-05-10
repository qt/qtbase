// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
    using namespace std::chrono;
    using namespace std::chrono_literals;

    QDeadlineTimer deadline(30s);
    device->waitForReadyRead(deadline);
    if (deadline.remainingTime<nanoseconds>() > 300ms)
        cleanup();
//! [1]

//! [2]
    using namespace std::chrono;
    using namespace std::chrono_literals;
    auto now = steady_clock::now();
    QDeadlineTimer deadline(now + 1s);
    Q_ASSERT(deadline == now + 1s);
//! [2]

//! [3]
    using namespace std::chrono_literals;
    QDeadlineTimer deadline(250ms);
//! [3]

//! [4]
    using namespace std::chrono_literals;
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
    return d1.deadlineNSecs() == d2.deadlineNSecs();
//! [8]

//! [9]
    return d1.deadlineNSecs() != d2.deadlineNSecs();
//! [9]

//! [10]
    return d1.deadlineNSecs() < d2.deadlineNSecs();
//! [10]

//! [11]
    return d1.deadlineNSecs() <= d2.deadlineNSecs();
//! [11]

//! [12]
    return d1.deadlineNSecs() > d2.deadlineNSecs();
//! [12]

//! [13]
    return d1.deadlineNSecs() >= d2.deadlineNSecs();
//! [13]
