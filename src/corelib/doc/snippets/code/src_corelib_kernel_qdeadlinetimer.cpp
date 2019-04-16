/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
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
