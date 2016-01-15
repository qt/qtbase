/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QtCore>
#include <QtTest/QtTest>

#include <math.h>


class tst_QWaitCondition : public QObject
{
    Q_OBJECT

public:
    tst_QWaitCondition()
    {
    }

private slots:
    void oscillate_mutex_data();
    void oscillate_mutex();
    void oscillate_writelock_data();
    void oscillate_writelock();
};


int turn;
const int threadCount = 10;
QWaitCondition cond;

template <class Mutex, class Locker>
class OscillateThread : public QThread
{
public:
    Mutex *mutex;
    int m_threadid;
    int timeout;

    void run()
    {
        for (int count = 0; count < 5000; ++count) {

            Locker lock(mutex);
            while (m_threadid != turn) {
                cond.wait(mutex, timeout);
            }
            turn = (turn+1) % threadCount;
            cond.wakeAll();
        }
    }
};

template <class Mutex, class Locker>
void oscillate(unsigned long timeout) {

    OscillateThread<Mutex, Locker> thrd[threadCount];
    Mutex m;
    for (int i = 0; i < threadCount; ++i) {
        thrd[i].mutex = &m;
        thrd[i].m_threadid = i;
        thrd[i].timeout = timeout;
    }

    QBENCHMARK {
        for (int i = 0; i < threadCount; ++i) {
            thrd[i].start();
        }
        for (int i = 0; i < threadCount; ++i) {
            thrd[i].wait();
        }
    }

}

void tst_QWaitCondition::oscillate_mutex_data()
{
    QTest::addColumn<unsigned long>("timeout");

    QTest::newRow("0") << 0ul;
    QTest::newRow("1") << 1ul;
    QTest::newRow("1000") << 1000ul;
    QTest::newRow("forever") << ULONG_MAX;
}

void tst_QWaitCondition::oscillate_mutex()
{
    QFETCH(unsigned long, timeout);
    oscillate<QMutex, QMutexLocker>(timeout);
}

void tst_QWaitCondition::oscillate_writelock_data()
{
    oscillate_mutex_data();
}

void tst_QWaitCondition::oscillate_writelock()
{
    QFETCH(unsigned long, timeout);
    oscillate<QReadWriteLock, QWriteLocker>(timeout);
}


QTEST_MAIN(tst_QWaitCondition)
#include "tst_qwaitcondition.moc"
