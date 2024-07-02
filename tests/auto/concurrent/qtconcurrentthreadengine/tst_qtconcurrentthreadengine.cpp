// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include <qtconcurrentthreadengine.h>
#include <qexception.h>
#include <QThread>
#include <QElapsedTimer>
#include <QTest>
#include <QSet>

using namespace QtConcurrent;

class tst_QtConcurrentThreadEngine: public QObject
{
    Q_OBJECT
private slots:
    void runDirectly();
    void result();
    void runThroughStarter();
    void cancel();
    void throttle();
    void threadCount();
    void multipleResults();
    void stresstest();
    void cancelQueuedSlowUser();
#ifndef QT_NO_EXCEPTIONS
    void exceptions();
#endif
};


class PrintUser : public ThreadEngine<void>
{
public:
    PrintUser() : ThreadEngine(QThreadPool::globalInstance()) {}
    ThreadFunctionResult threadFunction() override
    {
        QTest::qSleep(50);
        QTest::qSleep(100);
        return ThreadFinished;
    }
};

void tst_QtConcurrentThreadEngine::runDirectly()
{
    PrintUser *engine = new PrintUser();
    QFuture<void> f = engine->startAsynchronously();
    f.waitForFinished();
}

class StringResultUser : public ThreadEngine<QString>
{
public:
    typedef QString ResultType;
    StringResultUser()
    : ThreadEngine(QThreadPool::globalInstance())
    , done(false) { }

    bool shouldStartThread() override
    {
        return !done;
    }

    ThreadFunctionResult threadFunction() override
    {
        done = true;
        return ThreadFinished;
    }

    QString *result() override
    {
        foo = "Foo";
        return &foo;
    }
    QString foo;
    bool done;
};

void tst_QtConcurrentThreadEngine::result()
{
    // ThreadEngine will delete 'engine' when it finishes
    auto engine = new StringResultUser();
    auto future = engine->startAsynchronously();
    QCOMPARE(future.result(), QString("Foo"));
}

class VoidResultUser : public ThreadEngine<void>
{
public:
    VoidResultUser() : ThreadEngine(QThreadPool::globalInstance()) {}

    bool shouldStartThread() override
    {
        return !done;
    }

    ThreadFunctionResult threadFunction() override
    {
        done = true;
        return ThreadFinished;
    }

    void *result() override
    {
        return 0;
    }
    bool done = false;
};

void tst_QtConcurrentThreadEngine::runThroughStarter()
{
    ThreadEngineStarter<QString> starter = startThreadEngine(new StringResultUser());
    QFuture<QString> f = starter.startAsynchronously();
    QCOMPARE(f.result(), QString("Foo"));
}

class CancelUser : public ThreadEngine<void>
{
public:
    CancelUser() : ThreadEngine(QThreadPool::globalInstance()) {}

    void *result() override
    {
        return 0;
    }

    ThreadFunctionResult threadFunction() override
    {
        while (this->isCanceled() == false)
        {
            QTest::qSleep(10);
        }
        return ThreadFinished;
    }
};

void tst_QtConcurrentThreadEngine::cancel()
{
    {
        CancelUser *engine = new CancelUser();
        QFuture<void> f = engine->startAsynchronously();
        f.cancel();
        f.waitForFinished();
    }
    {
        CancelUser *engine = new CancelUser();
        QFuture<void> f = engine->startAsynchronously();
        QTest::qSleep(10);
        f.cancel();
        f.waitForFinished();
    }
}

QAtomicInt count;
class ThrottleAlwaysUser : public ThreadEngine<void>
{
public:
    ThrottleAlwaysUser()
    : ThreadEngine(QThreadPool::globalInstance())
    {
        count.storeRelaxed(initialCount = 100);
        finishing = false;
    }

    bool shouldStartThread() override
    {
        return !finishing;
    }

    ThreadFunctionResult threadFunction() override
    {
        forever {
            const int local = count.loadRelaxed();
            if (local == 0) {
                finishing = true;
                return ThreadFinished;
            }

            if (count.testAndSetOrdered(local, local - 1))
                break;
        }
        return ThrottleThread;
    }

    bool finishing;
    int initialCount;
};

// Test that a user task with a thread function that always
// want to be throttled still completes. The thread engine
// should make keep one thread running at all times.
void tst_QtConcurrentThreadEngine::throttle()
{
    const int repeats = 10;
    for (int i = 0; i < repeats; ++i) {
        QFuture<void> f = (new ThrottleAlwaysUser())->startAsynchronously();
        f.waitForFinished();
        QCOMPARE(count.loadRelaxed(), 0);
    }
}

QSet<QThread *> threads;
QMutex mutex;
class ThreadCountUser : public ThreadEngine<void>
{
public:
    ThreadCountUser(bool finishImmediately = false)
    : ThreadEngine(QThreadPool::globalInstance())
    {
        threads.clear();
        finishing = finishImmediately;
    }

    bool shouldStartThread() override
    {
        return !finishing;
    }

    ThreadFunctionResult threadFunction() override
    {
        {
            QMutexLocker lock(&mutex);
            threads.insert(QThread::currentThread());
        }
        QTest::qSleep(10);
        finishing = true;
        return ThreadFinished;
    }

    bool finishing;
};

void tst_QtConcurrentThreadEngine::threadCount()
{
    const int repeats = 10;
    for (int i = 0; i < repeats; ++i) {
        (new ThreadCountUser())->startAsynchronously().waitForFinished();
        const auto count = threads.size();
        const auto maxThreadCount = QThreadPool::globalInstance()->maxThreadCount();
        QVERIFY(count <= maxThreadCount);
        QVERIFY(!threads.contains(QThread::currentThread()));
    }

    // Set the finish flag immediately, this should give us one thread only.
    for (int i = 0; i < repeats; ++i) {
        (new ThreadCountUser(true /*finishImmediately*/))->startAsynchronously().waitForFinished();
        const auto count = threads.size();
        QCOMPARE(count, 1);
        QVERIFY(!threads.contains(QThread::currentThread()));
    }
}

class MultipleResultsUser : public ThreadEngine<int>
{
public:
    MultipleResultsUser() : ThreadEngine(QThreadPool::globalInstance()) {}
    bool shouldStartThread() override
    {
        return false;
    }

    ThreadFunctionResult threadFunction() override
    {
        for (int i = 0; i < 10; ++i)
            this->reportResult(&i);
        return ThreadFinished;
    }
};


void tst_QtConcurrentThreadEngine::multipleResults()
{
    MultipleResultsUser *engine =  new MultipleResultsUser();
    QFuture<int> f = engine->startAsynchronously();
    QCOMPARE(f.results().size() , 10);
    QCOMPARE(f.resultAt(0), 0);
    QCOMPARE(f.resultAt(5), 5);
    QCOMPARE(f.resultAt(9), 9);
    f.waitForFinished();
}


class NoThreadsUser : public ThreadEngine<void>
{
public:
    bool shouldStartThread() override
    {
        return false;
    }

    ThreadFunctionResult threadFunction() override
    {
        return ThreadFinished;
    }

    void *result() override
    {
        return 0;
    }
};

void tst_QtConcurrentThreadEngine::stresstest()
{
    const int times = 20000;

    for (int i = 0; i < times; ++i) {
        VoidResultUser *engine = new VoidResultUser();
        engine->startAsynchronously().waitForFinished();
    }

    for (int i = 0; i < times; ++i) {
        VoidResultUser *engine = new VoidResultUser();
        engine->startAsynchronously();
    }

    for (int i = 0; i < times; ++i) {
        VoidResultUser *engine = new VoidResultUser();
        engine->startAsynchronously().waitForFinished();
    }
}

const int sleepTime = 20;
class SlowUser : public ThreadEngine<void>
{
public:
    SlowUser() : ThreadEngine(QThreadPool::globalInstance()) {}
    bool shouldStartThread() override { return false; }
    ThreadFunctionResult threadFunction() override { QTest::qSleep(sleepTime); return ThreadFinished; }
};

void tst_QtConcurrentThreadEngine::cancelQueuedSlowUser()
{
    const int times = 100;

    QElapsedTimer t;
    t.start();

    {
        QList<QFuture<void> > futures;
        for (int i = 0; i < times; ++i) {
            SlowUser *engine = new SlowUser();
            futures.append(engine->startAsynchronously());
        }

        foreach(QFuture<void> future, futures)
            future.cancel();
    }

    QVERIFY(t.elapsed() < (sleepTime * times) / 2);
}

#ifndef QT_NO_EXCEPTIONS

class QtConcurrentExceptionThrower : public ThreadEngine<void>
{
public:
    QtConcurrentExceptionThrower(QThread *blockThread = nullptr)
    : ThreadEngine(QThreadPool::globalInstance())
    {
        this->blockThread = blockThread;
    }

    ThreadFunctionResult threadFunction() override
    {
        QTest::qSleep(50);
        throw QException();
        return ThreadFinished;
    }
    QThread *blockThread;
};

class IntExceptionThrower : public ThreadEngine<void>
{
public:
    IntExceptionThrower(QThread *blockThread = nullptr)
    : ThreadEngine(QThreadPool::globalInstance())
    {
        this->blockThread = blockThread;
    }

    ThreadFunctionResult threadFunction() override
    {
        QTest::qSleep(50);
        throw int();
        return ThreadFinished;
    }
    QThread *blockThread;
};

void tst_QtConcurrentThreadEngine::exceptions()
{
    {
        bool caught = false;
        try  {
            QtConcurrentExceptionThrower *e = new QtConcurrentExceptionThrower();
            QFuture<void> f = e->startAsynchronously();
            f.waitForFinished();
        } catch (const QException &) {
            caught = true;
        }
        QVERIFY2(caught, "did not get exception");
    }

    {
        bool caught = false;
        try  {
            IntExceptionThrower *e = new IntExceptionThrower();
            QFuture<void> f = e->startAsynchronously();
            f.waitForFinished();
        } catch (const QUnhandledException &ex) {
            // Make sure the exception info is not lost
            try {
                if (ex.exception())
                    std::rethrow_exception(ex.exception());
            } catch (int) {
                caught = true;
            }
        }
        QVERIFY2(caught, "did not get exception");
    }
}

#endif

QTEST_MAIN(tst_QtConcurrentThreadEngine)

#include "tst_qtconcurrentthreadengine.moc"
