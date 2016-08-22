/****************************************************************************
**
** Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
** Contact: http://www.qt.io/licensing/
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
#include <mutex>
#if QT_HAS_INCLUDE(<shared_mutex>)
#if __cplusplus > 201103L
#include <shared_mutex>
#endif
#endif

// Wrapers that take pointers instead of reference to have the same interface as Qt
template <typename T>
struct LockerWrapper : T
{
    LockerWrapper(typename T::mutex_type *mtx)
        : T(*mtx)
    {
    }
};

int threadCount;

class tst_QReadWriteLock : public QObject
{
    Q_OBJECT
public:
    tst_QReadWriteLock()
    {
        // at least 2 threads, even on single cpu/core machines
        threadCount = qMax(2, QThread::idealThreadCount());
        qDebug("thread count: %d", threadCount);
    }

private slots:
    void uncontended_data();
    void uncontended();
    void readOnly_data();
    void readOnly();
    // void readWrite();
};

struct FunctionPtrHolder
{
    FunctionPtrHolder(QFunctionPointer value = nullptr)
        : value(value)
    {
    }
    QFunctionPointer value;
};
Q_DECLARE_METATYPE(FunctionPtrHolder)

struct FakeLock
{
    FakeLock(volatile int *i) { *i = 0; }
};

enum { Iterations = 1000000 };

template <typename Mutex, typename Locker>
void testUncontended()
{
    Mutex lock;
    QBENCHMARK {
        for (int i = 0; i < Iterations; ++i) {
            Locker locker(&lock);
        }
    }
}

void tst_QReadWriteLock::uncontended_data()
{
    QTest::addColumn<FunctionPtrHolder>("holder");

    QTest::newRow("nothing") << FunctionPtrHolder(testUncontended<int, FakeLock>);
    QTest::newRow("QMutex") << FunctionPtrHolder(testUncontended<QMutex, QMutexLocker>);
    QTest::newRow("QReadWriteLock, read")
        << FunctionPtrHolder(testUncontended<QReadWriteLock, QReadLocker>);
    QTest::newRow("QReadWriteLock, write")
        << FunctionPtrHolder(testUncontended<QReadWriteLock, QWriteLocker>);
    QTest::newRow("std::mutex") << FunctionPtrHolder(
        testUncontended<std::mutex, LockerWrapper<std::unique_lock<std::mutex>>>);
#if defined __cpp_lib_shared_timed_mutex
    QTest::newRow("std::shared_timed_mutex, read") << FunctionPtrHolder(
        testUncontended<std::shared_timed_mutex,
                        LockerWrapper<std::shared_lock<std::shared_timed_mutex>>>);
    QTest::newRow("std::shared_timed_mutex, write") << FunctionPtrHolder(
        testUncontended<std::shared_timed_mutex,
                        LockerWrapper<std::unique_lock<std::shared_timed_mutex>>>);
#endif
}

void tst_QReadWriteLock::uncontended()
{
    QFETCH(FunctionPtrHolder, holder);
    holder.value();
}

static QHash<QString, QString> global_hash;

template <typename Mutex, typename Locker>
void testReadOnly()
{
    struct Thread : QThread
    {
        Mutex *lock;
        void run()
        {
            for (int i = 0; i < Iterations; ++i) {
                QString s = QString::number(i); // Do something outside the lock
                Locker locker(lock);
                global_hash.contains(s);
            }
        }
    };
    Mutex lock;
    QVector<QThread *> threads;
    for (int i = 0; i < threadCount; ++i) {
        auto t = new Thread;
        t->lock = &lock;
        threads.append(t);
    }
    QBENCHMARK {
        for (auto t : threads) {
            t->start();
        }
        for (auto t : threads) {
            t->wait();
        }
    }
    qDeleteAll(threads);
}

void tst_QReadWriteLock::readOnly_data()
{
    QTest::addColumn<FunctionPtrHolder>("holder");

    QTest::newRow("nothing") << FunctionPtrHolder(testReadOnly<int, FakeLock>);
    QTest::newRow("QMutex") << FunctionPtrHolder(testReadOnly<QMutex, QMutexLocker>);
    QTest::newRow("QReadWriteLock") << FunctionPtrHolder(testReadOnly<QReadWriteLock, QReadLocker>);
    QTest::newRow("std::mutex") << FunctionPtrHolder(
        testReadOnly<std::mutex, LockerWrapper<std::unique_lock<std::mutex>>>);
#if defined __cpp_lib_shared_timed_mutex
    QTest::newRow("std::shared_timed_mutex") << FunctionPtrHolder(
        testReadOnly<std::shared_timed_mutex,
                     LockerWrapper<std::shared_lock<std::shared_timed_mutex>>>);
#endif
}

void tst_QReadWriteLock::readOnly()
{
    QFETCH(FunctionPtrHolder, holder);
    holder.value();
}

QTEST_MAIN(tst_QReadWriteLock)
#include "tst_qreadwritelock.moc"
