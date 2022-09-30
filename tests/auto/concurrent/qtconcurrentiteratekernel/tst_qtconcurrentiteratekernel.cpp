// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QThread>
#include <QSet>

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
#include <QTest>

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
    void multipleResults();
};

QAtomicInt iterations;
class PrintFor : public IterateKernel<TestIterator, void>
{
public:
    PrintFor(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, void>(QThreadPool::globalInstance(), begin, end) { iterations.storeRelaxed(0); }
    bool runIterations(TestIterator/*beginIterator*/, int begin, int end, void *) override
    {
        iterations.fetchAndAddRelaxed(end - begin);
#ifdef PRINT
        qDebug() << QThread::currentThread() << "iteration" << begin <<  "to" << end << "(exclusive)";
#endif
        return false;
    }
    bool runIteration(TestIterator it, int index , void *result) override
    {
        return runIterations(it, index, index + 1, result);
    }

};

class SleepPrintFor : public IterateKernel<TestIterator, void>
{
public:
    SleepPrintFor(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, void>(QThreadPool::globalInstance(), begin, end) { iterations.storeRelaxed(0); }
    inline bool runIterations(TestIterator/*beginIterator*/, int begin, int end, void *) override
    {
        QTest::qSleep(200);
        iterations.fetchAndAddRelaxed(end - begin);
#ifdef PRINT
        qDebug() << QThread::currentThread() << "iteration" << begin <<  "to" << end << "(exclusive)";
#endif
        return false;
    }
    bool runIteration(TestIterator it, int index , void *result) override
    {
        return runIterations(it, index, index + 1, result);
    }
};


void tst_QtConcurrentIterateKernel::instantiate()
{
    auto future = startThreadEngine(new PrintFor(0, 40)).startAsynchronously();
    future.waitForFinished();
    QCOMPARE(iterations.loadRelaxed(), 40);
}

void tst_QtConcurrentIterateKernel::cancel()
{
    {
        QFuture<void> f = startThreadEngine(new SleepPrintFor(0, 40)).startAsynchronously();
        f.cancel();
        f.waitForFinished();
        QVERIFY(f.isCanceled());
         // the threads might run one iteration each before they are canceled.
        QVERIFY2(iterations.loadRelaxed() <= QThread::idealThreadCount(),
                 (QByteArray::number(iterations.loadRelaxed()) + ' ' + QByteArray::number(QThread::idealThreadCount())));
    }
}

QAtomicInt counter;
class CountFor : public IterateKernel<TestIterator, void>
{
public:
    CountFor(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, void>(QThreadPool::globalInstance(), begin, end) { iterations.storeRelaxed(0); }
    inline bool runIterations(TestIterator/*beginIterator*/, int begin, int end, void *) override
    {
        counter.fetchAndAddRelaxed(end - begin);
        return false;
    }
    bool runIteration(TestIterator it, int index , void *result) override
    {
        return runIterations(it, index, index + 1, result);
    }
};

void tst_QtConcurrentIterateKernel::stresstest()
{
    const int iterations = 1000;
    const int times = 50;
    for (int i = 0; i < times; ++i) {
        counter.storeRelaxed(0);
        // ThreadEngine will delete f when it finishes
        auto f = new CountFor(0, iterations);
        auto future = f->startAsynchronously();
        future.waitForFinished();
        QCOMPARE(counter.loadRelaxed(), iterations);
    }
}

void tst_QtConcurrentIterateKernel::noIterations()
{
    const int times = 20000;
    for (int i = 0; i < times; ++i) {
        auto future = startThreadEngine(new IterateKernel<TestIterator, void>(
                                                QThreadPool::globalInstance(), 0, 0))
                              .startAsynchronously();
        future.waitForFinished();
    }
}

QMutex threadsMutex;
QSet<QThread *> threads;
class ThrottleFor : public IterateKernel<TestIterator, void>
{
public:
    // this class throttles between iterations 100 and 200,
    // and then records how many threads that run between
    // iterations 140 and 160.
    ThrottleFor(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, void>(QThreadPool::globalInstance(), begin, end) { iterations.storeRelaxed(0); throttling = false; }
    inline bool runIterations(TestIterator/*beginIterator*/, int begin, int end, void *) override
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
    bool runIteration(TestIterator it, int index , void *result) override
    {
        return runIterations(it, index, index + 1, result);
    }

    bool shouldThrottleThread() override
    {
       const int load = iterations.loadRelaxed();
       return (load > 100 && load < 200);
    }
    bool throttling;
};

void tst_QtConcurrentIterateKernel::throttling()
{
    const int totalIterations = 400;
    iterations.storeRelaxed(0);

    threads.clear();

    // ThreadEngine will delete f when it finishes
    auto f = new ThrottleFor(0, totalIterations);
    auto future = f->startAsynchronously();
    future.waitForFinished();

    QCOMPARE(iterations.loadRelaxed(), totalIterations);


    QCOMPARE(threads.size(), 1);
}

class MultipleResultsFor : public IterateKernel<TestIterator, int>
{
public:
    MultipleResultsFor(TestIterator begin, TestIterator end) : IterateKernel<TestIterator, int>(QThreadPool::globalInstance(), begin, end) { }
    inline bool runIterations(TestIterator, int begin, int end, int *results) override
    {
        for (int i = begin; i < end; ++i)
            results[i - begin] = i;
        return true;
    }
};

void tst_QtConcurrentIterateKernel::multipleResults()
{
    QFuture<int> f = startThreadEngine(new MultipleResultsFor(0, 10)).startAsynchronously();
    QCOMPARE(f.results().size() , 10);
    QCOMPARE(f.resultAt(0), 0);
    QCOMPARE(f.resultAt(5), 5);
    QCOMPARE(f.resultAt(9), 9);
    f.waitForFinished();
}

QTEST_MAIN(tst_QtConcurrentIterateKernel)

#include "tst_qtconcurrentiteratekernel.moc"
