/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QThread>

struct TestIterator
{
    TestIterator(int i)
    :i(i) { }

    int operator-(const TestIterator &other)
    {
        return i - other.i;
    }

    TestIterator& operator++()
    {
        ++i;
        return *this;
    }

    bool operator!=(const TestIterator &other) const
    {
        return i != other.i;
    }

    int i;
};

#include <qiterator.h>
namespace std {
template <>
struct iterator_traits<TestIterator>
{
    typedef random_access_iterator_tag iterator_category;
};

int distance(TestIterator &a, TestIterator &b)
{
    return b - a;
}

}

#include <qtconcurrentiteratekernel.h>
#include <QtTest/QtTest>

using namespace QtConcurrent;

class tst_QtConcurrentIterateKernel: public QObject
{
    Q_OBJECT
private slots:
    // "for" iteration tests:
    void instantiate();
    void cancel();
    void stresstest();
    void noIterations();
    void throttling();
    void blockSize();
    void multipleResults();
};

QAtomicInt iterations;
class PrintFor : public IterateKernel<TestIterator, void>
{
public:
    PrintFor(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, void>(begin, end) { iterations.store(0); }
    bool runIterations(TestIterator/*beginIterator*/, int begin, int end, void *)
    {
        iterations.fetchAndAddRelaxed(end - begin);
#ifdef PRINT
        qDebug() << QThread::currentThread() << "iteration" << begin <<  "to" << end << "(exclusive)";
#endif
        return false;
    }
    bool runIteration(TestIterator it, int index , void *result)
    {
        return runIterations(it, index, index + 1, result);
    }

};

class SleepPrintFor : public IterateKernel<TestIterator, void>
{
public:
    SleepPrintFor(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, void>(begin, end) { iterations.store(0); }
    inline bool runIterations(TestIterator/*beginIterator*/, int begin, int end, void *)
    {
        QTest::qSleep(200);
        iterations.fetchAndAddRelaxed(end - begin);
#ifdef PRINT
        qDebug() << QThread::currentThread() << "iteration" << begin <<  "to" << end << "(exclusive)";
#endif
        return false;
    }
    bool runIteration(TestIterator it, int index , void *result)
    {
        return runIterations(it, index, index + 1, result);
    }
};


void tst_QtConcurrentIterateKernel::instantiate()
{
    startThreadEngine(new PrintFor(0, 40)).startBlocking();
    QCOMPARE(iterations.load(), 40);
}

void tst_QtConcurrentIterateKernel::cancel()
{
    {
        QFuture<void> f = startThreadEngine(new SleepPrintFor(0, 40)).startAsynchronously();
        f.cancel();
        f.waitForFinished();
        QVERIFY(f.isCanceled());
        QVERIFY(iterations.load() <= QThread::idealThreadCount()); // the threads might run one iteration each before they are canceled.
    }
}

QAtomicInt counter;
class CountFor : public IterateKernel<TestIterator, void>
{
public:
    CountFor(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, void>(begin, end) { iterations.store(0); }
    inline bool runIterations(TestIterator/*beginIterator*/, int begin, int end, void *)
    {
        counter.fetchAndAddRelaxed(end - begin);
        return false;
    }
    bool runIteration(TestIterator it, int index , void *result)
    {
        return runIterations(it, index, index + 1, result);
    }
};

void tst_QtConcurrentIterateKernel::stresstest()
{
    const int iterations = 1000;
    const int times = 50;
    for (int i = 0; i < times; ++i) {
        counter.store(0);
        CountFor f(0, iterations);
        f.startBlocking();
        QCOMPARE(counter.load(), iterations);
    }
}

void tst_QtConcurrentIterateKernel::noIterations()
{
    const int times = 20000;
    for (int i = 0; i < times; ++i)
        startThreadEngine(new IterateKernel<TestIterator, void>(0, 0)).startBlocking();
}

QMutex threadsMutex;
QSet<QThread *> threads;
class ThrottleFor : public IterateKernel<TestIterator, void>
{
public:
    // this class throttles between iterations 100 and 200,
    // and then records how many threads that run between
    // iterations 140 and 160.
    ThrottleFor(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, void>(begin, end) { iterations.store(0); throttling = false; }
    inline bool runIterations(TestIterator/*beginIterator*/, int begin, int end, void *)
    {
        if (200 >= begin && 200 < end) {
            throttling = false;
        }

        iterations.fetchAndAddRelaxed(end - begin);

        QThread *thread = QThread::currentThread();

        if (begin > 140 && end < 160) {
            QMutexLocker locker(&threadsMutex);
            threads.insert(thread);
        }

        if (100 >= begin && 100 < end) {
            throttling = true;
        }

        QTest::qWait(1);

        return false;
    }
    bool runIteration(TestIterator it, int index , void *result)
    {
        return runIterations(it, index, index + 1, result);
    }

    bool shouldThrottleThread()
    {
       const int load = iterations.load();
       return (load > 100 && load < 200);
    }
    bool throttling;
};

void tst_QtConcurrentIterateKernel::throttling()
{
    const int totalIterations = 400;
    iterations.store(0);

    threads.clear();

    ThrottleFor f(0, totalIterations);
    f.startBlocking();

    QCOMPARE(iterations.load(), totalIterations);


    QCOMPARE(threads.count(), 1);
}


int peakBlockSize = 0;
class BlockSizeRecorder : public IterateKernel<TestIterator, void>
{
public:
    BlockSizeRecorder(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, void>(begin, end) { }
    inline bool runIterations(TestIterator, int begin, int end, void *)
    {
        peakBlockSize = qMax(peakBlockSize, end - begin);
        return false;
    }
};

void tst_QtConcurrentIterateKernel::blockSize()
{
    const int expectedMinimumBlockSize = 1024 / QThread::idealThreadCount();
    BlockSizeRecorder(0, 10000).startBlocking();
    if (peakBlockSize < expectedMinimumBlockSize)
        qDebug() << "block size" << peakBlockSize;
    QVERIFY(peakBlockSize >= expectedMinimumBlockSize);
}

class MultipleResultsFor : public IterateKernel<TestIterator, int>
{
public:
    MultipleResultsFor(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, int>(begin, end) { }
    inline bool runIterations(TestIterator, int begin, int end, int *results)
    {
        for (int i = begin; i < end; ++i)
            results[i - begin] = i;
        return true;
    }
};

void tst_QtConcurrentIterateKernel::multipleResults()
{
    QFuture<int> f = startThreadEngine(new MultipleResultsFor(0, 10)).startAsynchronously();
    QCOMPARE(f.results().count() , 10);
    QCOMPARE(f.resultAt(0), 0);
    QCOMPARE(f.resultAt(5), 5);
    QCOMPARE(f.resultAt(9), 9);
    f.waitForFinished();
}

QTEST_MAIN(tst_QtConcurrentIterateKernel)

#include "tst_qtconcurrentiteratekernel.moc"
