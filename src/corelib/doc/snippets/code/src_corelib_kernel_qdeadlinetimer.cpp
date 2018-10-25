/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
    void executeOperation(int msecs)
    {
        QDeadlineTimer deadline(msecs);
        do {
            if (readFromDevice(deadline.remainingTime())
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
