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

#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qthread.h>
#include <qsemaphore.h>

class tst_QSemaphore : public QObject
{
    Q_OBJECT
private slots:
    void acquire();
    void tryAcquire();
    void tryAcquireWithTimeout_data();
    void tryAcquireWithTimeout();
    void tryAcquireWithTimeoutStarvation();
    void producerConsumer();
};

static QSemaphore *semaphore = 0;

class ThreadOne : public QThread
{
public:
    ThreadOne() {}

protected:
    void run()
    {
        int i = 0;
        while ( i < 100 ) {
            semaphore->acquire();
            i++;
            semaphore->release();
        }
    }
};

class ThreadN : public QThread
{
    int N;

public:
    ThreadN(int n) :N(n) { }

protected:
    void run()
    {
        int i = 0;
        while ( i < 100 ) {
            semaphore->acquire(N);
            i++;
            semaphore->release(N);
        }
    }
};

void tst_QSemaphore::acquire()
{
    {
        // old incrementOne() test
        QVERIFY(!semaphore);
        semaphore = new QSemaphore;
        // make some "thing" available
        semaphore->release();

        ThreadOne t1;
        ThreadOne t2;

        t1.start();
        t2.start();

        QVERIFY(t1.wait(4000));
        QVERIFY(t2.wait(4000));

        delete semaphore;
        semaphore = 0;
    }

    // old incrementN() test
    {
        QVERIFY(!semaphore);
        semaphore = new QSemaphore;
        // make 4 "things" available
        semaphore->release(4);

        ThreadN t1(2);
        ThreadN t2(3);

        t1.start();
        t2.start();

        QVERIFY(t1.wait(4000));
        QVERIFY(t2.wait(4000));

        delete semaphore;
        semaphore = 0;
    }

    QSemaphore semaphore;

    QCOMPARE(semaphore.available(), 0);
    semaphore.release();
    QCOMPARE(semaphore.available(), 1);
    semaphore.release();
    QCOMPARE(semaphore.available(), 2);
    semaphore.release(10);
    QCOMPARE(semaphore.available(), 12);
    semaphore.release(10);
    QCOMPARE(semaphore.available(), 22);

    semaphore.acquire();
    QCOMPARE(semaphore.available(), 21);
    semaphore.acquire();
    QCOMPARE(semaphore.available(), 20);
    semaphore.acquire(10);
    QCOMPARE(semaphore.available(), 10);
    semaphore.acquire(10);
    QCOMPARE(semaphore.available(), 0);
}

void tst_QSemaphore::tryAcquire()
{
    QSemaphore semaphore;

    QCOMPARE(semaphore.available(), 0);

    semaphore.release();
    QCOMPARE(semaphore.available(), 1);
    QVERIFY(!semaphore.tryAcquire(2));
    QCOMPARE(semaphore.available(), 1);

    semaphore.release();
    QCOMPARE(semaphore.available(), 2);
    QVERIFY(!semaphore.tryAcquire(3));
    QCOMPARE(semaphore.available(), 2);

    semaphore.release(10);
    QCOMPARE(semaphore.available(), 12);
    QVERIFY(!semaphore.tryAcquire(100));
    QCOMPARE(semaphore.available(), 12);

    semaphore.release(10);
    QCOMPARE(semaphore.available(), 22);
    QVERIFY(!semaphore.tryAcquire(100));
    QCOMPARE(semaphore.available(), 22);

    QVERIFY(semaphore.tryAcquire());
    QCOMPARE(semaphore.available(), 21);

    QVERIFY(semaphore.tryAcquire());
    QCOMPARE(semaphore.available(), 20);

    QVERIFY(semaphore.tryAcquire(10));
    QCOMPARE(semaphore.available(), 10);

    QVERIFY(semaphore.tryAcquire(10));
    QCOMPARE(semaphore.available(), 0);

    // should not be able to acquire more
    QVERIFY(!semaphore.tryAcquire());
    QCOMPARE(semaphore.available(), 0);

    QVERIFY(!semaphore.tryAcquire());
    QCOMPARE(semaphore.available(), 0);

    QVERIFY(!semaphore.tryAcquire(10));
    QCOMPARE(semaphore.available(), 0);

    QVERIFY(!semaphore.tryAcquire(10));
    QCOMPARE(semaphore.available(), 0);
}

void tst_QSemaphore::tryAcquireWithTimeout_data()
{
    QTest::addColumn<int>("timeout");

    QTest::newRow("1s") << 1000;
    QTest::newRow("10s") << 10000;
}

void tst_QSemaphore::tryAcquireWithTimeout()
{
    QFETCH(int, timeout);

    // timers are not guaranteed to be accurate down to the last millisecond,
    // so we permit the elapsed times to be up to this far from the expected value.
    int fuzz = 50;

    QSemaphore semaphore;
    QElapsedTimer time;

#define FUZZYCOMPARE(a,e) \
    do { \
        int a1 = a; \
        int e1 = e; \
        QVERIFY2(qAbs(a1-e1) < fuzz, \
            qPrintable(QString("(%1=%2) is more than %3 milliseconds different from (%4=%5)") \
                        .arg(#a).arg(a1).arg(fuzz).arg(#e).arg(e1))); \
    } while (0)

    QCOMPARE(semaphore.available(), 0);

    semaphore.release();
    QCOMPARE(semaphore.available(), 1);
    time.start();
    QVERIFY(!semaphore.tryAcquire(2, timeout));
    FUZZYCOMPARE(time.elapsed(), timeout);
    QCOMPARE(semaphore.available(), 1);

    semaphore.release();
    QCOMPARE(semaphore.available(), 2);
    time.start();
    QVERIFY(!semaphore.tryAcquire(3, timeout));
    FUZZYCOMPARE(time.elapsed(), timeout);
    QCOMPARE(semaphore.available(), 2);

    semaphore.release(10);
    QCOMPARE(semaphore.available(), 12);
    time.start();
    QVERIFY(!semaphore.tryAcquire(100, timeout));
    FUZZYCOMPARE(time.elapsed(), timeout);
    QCOMPARE(semaphore.available(), 12);

    semaphore.release(10);
    QCOMPARE(semaphore.available(), 22);
    time.start();
    QVERIFY(!semaphore.tryAcquire(100, timeout));
    FUZZYCOMPARE(time.elapsed(), timeout);
    QCOMPARE(semaphore.available(), 22);

    time.start();
    QVERIFY(semaphore.tryAcquire(1, timeout));
    FUZZYCOMPARE(time.elapsed(), 0);
    QCOMPARE(semaphore.available(), 21);

    time.start();
    QVERIFY(semaphore.tryAcquire(1, timeout));
    FUZZYCOMPARE(time.elapsed(), 0);
    QCOMPARE(semaphore.available(), 20);

    time.start();
    QVERIFY(semaphore.tryAcquire(10, timeout));
    FUZZYCOMPARE(time.elapsed(), 0);
    QCOMPARE(semaphore.available(), 10);

    time.start();
    QVERIFY(semaphore.tryAcquire(10, timeout));
    FUZZYCOMPARE(time.elapsed(), 0);
    QCOMPARE(semaphore.available(), 0);

    // should not be able to acquire more
    time.start();
    QVERIFY(!semaphore.tryAcquire(1, timeout));
    FUZZYCOMPARE(time.elapsed(), timeout);
    QCOMPARE(semaphore.available(), 0);

    time.start();
    QVERIFY(!semaphore.tryAcquire(1, timeout));
    FUZZYCOMPARE(time.elapsed(), timeout);
    QCOMPARE(semaphore.available(), 0);

    time.start();
    QVERIFY(!semaphore.tryAcquire(10, timeout));
    FUZZYCOMPARE(time.elapsed(), timeout);
    QCOMPARE(semaphore.available(), 0);

    time.start();
    QVERIFY(!semaphore.tryAcquire(10, timeout));
    FUZZYCOMPARE(time.elapsed(), timeout);
    QCOMPARE(semaphore.available(), 0);

#undef FUZZYCOMPARE
}

void tst_QSemaphore::tryAcquireWithTimeoutStarvation()
{
    class Thread : public QThread
    {
    public:
        QSemaphore startup;
        QSemaphore *semaphore;
        int amountToConsume, timeout;

        void run()
        {
            startup.release();
            forever {
                if (!semaphore->tryAcquire(amountToConsume, timeout))
                    break;
                semaphore->release(amountToConsume);
            }
        }
    };

    QSemaphore semaphore;
    semaphore.release(1);

    Thread consumer;
    consumer.semaphore = &semaphore;
    consumer.amountToConsume = 1;
    consumer.timeout = 1000;

    // start the thread and wait for it to start consuming
    consumer.start();
    consumer.startup.acquire();

    // try to consume more than the thread we started is, and provide a longer
    // timeout... we should timeout, not wait indefinitely
    QVERIFY(!semaphore.tryAcquire(consumer.amountToConsume * 2, consumer.timeout * 2));

    // the consumer should still be running
    QVERIFY(consumer.isRunning() && !consumer.isFinished());

    // acquire, and wait for smallConsumer to timeout
    semaphore.acquire();
    QVERIFY(consumer.wait());
}

const char alphabet[] = "ACGTH";
const int AlphabetSize = sizeof(alphabet) - 1;

const int BufferSize = 4096; // GCD of BufferSize and alphabet size must be 1
char buffer[BufferSize];

#ifndef Q_OS_WINCE
const int ProducerChunkSize = 3;
const int ConsumerChunkSize = 7;
const int Multiplier = 10;
#else
const int ProducerChunkSize = 2;
const int ConsumerChunkSize = 5;
const int Multiplier = 3;
#endif

// note: the code depends on the fact that DataSize is a multiple of
// ProducerChunkSize, ConsumerChunkSize, and BufferSize
const int DataSize = ProducerChunkSize * ConsumerChunkSize * BufferSize * Multiplier;

QSemaphore freeSpace(BufferSize);
QSemaphore usedSpace;

class Producer : public QThread
{
public:
    void run();
};

void Producer::run()
{
    for (int i = 0; i < DataSize; ++i) {
        freeSpace.acquire();
        buffer[i % BufferSize] = alphabet[i % AlphabetSize];
        usedSpace.release();
    }
    for (int i = 0; i < DataSize; ++i) {
        if ((i % ProducerChunkSize) == 0)
            freeSpace.acquire(ProducerChunkSize);
        buffer[i % BufferSize] = alphabet[i % AlphabetSize];
        if ((i % ProducerChunkSize) == (ProducerChunkSize - 1))
            usedSpace.release(ProducerChunkSize);
    }
}

class Consumer : public QThread
{
public:
    void run();
};

void Consumer::run()
{
    for (int i = 0; i < DataSize; ++i) {
        usedSpace.acquire();
        QCOMPARE(buffer[i % BufferSize], alphabet[i % AlphabetSize]);
        freeSpace.release();
    }
    for (int i = 0; i < DataSize; ++i) {
        if ((i % ConsumerChunkSize) == 0)
            usedSpace.acquire(ConsumerChunkSize);
        QCOMPARE(buffer[i % BufferSize], alphabet[i % AlphabetSize]);
        if ((i % ConsumerChunkSize) == (ConsumerChunkSize - 1))
            freeSpace.release(ConsumerChunkSize);
    }
}

void tst_QSemaphore::producerConsumer()
{
    Producer producer;
    Consumer consumer;
    producer.start();
    consumer.start();
    producer.wait();
    consumer.wait();
}

QTEST_MAIN(tst_QSemaphore)
#include "tst_qsemaphore.moc"
