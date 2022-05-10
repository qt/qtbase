// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QtCore>
#include <QTest>

#include <math.h>
#include <condition_variable>
#include <mutex>

#include <limits.h>

using namespace std::chrono_literals;

class tst_QWaitCondition : public QObject
{
    Q_OBJECT

public:
    tst_QWaitCondition()
    {
    }

private slots:
    void oscillate_QWaitCondition_QMutex_data() { oscillate_mutex_data(); }
    void oscillate_QWaitCondition_QMutex();
    void oscillate_QWaitCondition_QReadWriteLock_data() { oscillate_mutex_data(); }
    void oscillate_QWaitCondition_QReadWriteLock();
    void oscillate_std_condition_variable_std_mutex_data() { oscillate_mutex_data(); }
    void oscillate_std_condition_variable_std_mutex();
    void oscillate_std_condition_variable_any_QMutex_data() { oscillate_mutex_data(); }
    void oscillate_std_condition_variable_any_QMutex();
    void oscillate_std_condition_variable_any_QReadWriteLock_data() { oscillate_mutex_data(); }
    void oscillate_std_condition_variable_any_QReadWriteLock();

private:
    void oscillate_mutex_data();
};


int turn;
const int threadCount = 10;
QWaitCondition cond;
std::condition_variable cv;
std::condition_variable_any cva;

template <typename Cond>
Cond *get();

template <> std::condition_variable     *get() { return &cv; }
template <> std::condition_variable_any *get() { return &cva; }

template <class Cond, class Mutex, class Locker>
class OscillateThread : public QThread
{
public:
    Mutex *mutex;
    int m_threadid;
    unsigned long timeout;

    void run() override
    {
        for (int count = 0; count < 5000; ++count) {
            Locker lock(*mutex);
            while (m_threadid != turn) {
                if (timeout == ULONG_MAX)
                    get<Cond>()->wait(lock);
                else if (timeout == 0) // Windows doesn't unlock the mutex with a zero timeout
                    get<Cond>()->wait_for(lock, 1ns);
                else
                    get<Cond>()->wait_for(lock, timeout * 1ms);
            }
            turn = (turn+1) % threadCount;
            get<Cond>()->notify_all();
        }
    }
};

template <class Mutex, class Locker>
class OscillateThread<QWaitCondition, Mutex, Locker> : public QThread
{
public:
    Mutex *mutex;
    int m_threadid;
    unsigned long timeout;

    void run() override
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

template <class Cond, class Mutex, class Locker>
void oscillate(unsigned long timeout) {

    OscillateThread<Cond, Mutex, Locker> thrd[threadCount];
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

void tst_QWaitCondition::oscillate_QWaitCondition_QMutex()
{
    QFETCH(unsigned long, timeout);
    oscillate<QWaitCondition, QMutex, QMutexLocker<QMutex>>(timeout);
}

void tst_QWaitCondition::oscillate_QWaitCondition_QReadWriteLock()
{
    QFETCH(unsigned long, timeout);
    oscillate<QWaitCondition, QReadWriteLock, QWriteLocker>(timeout);
}

void tst_QWaitCondition::oscillate_std_condition_variable_std_mutex()
{
    QFETCH(unsigned long, timeout);
    oscillate<std::condition_variable, std::mutex, std::unique_lock<std::mutex>>(timeout);
}


void tst_QWaitCondition::oscillate_std_condition_variable_any_QMutex()
{
    QFETCH(unsigned long, timeout);
    oscillate<std::condition_variable_any, QMutex, std::unique_lock<QMutex>>(timeout);
}


void tst_QWaitCondition::oscillate_std_condition_variable_any_QReadWriteLock()
{
    QFETCH(unsigned long, timeout);

    struct WriteLocker : QWriteLocker {
        // adapt to BasicLockable
        explicit WriteLocker(QReadWriteLock &m) : QWriteLocker{&m} {}
        void lock() { relock(); }
    };

    oscillate<std::condition_variable_any, QReadWriteLock, WriteLocker>(timeout);
}

QTEST_MAIN(tst_QWaitCondition)

#include "tst_bench_qwaitcondition.moc"
