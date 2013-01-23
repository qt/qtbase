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
#include <qtconcurrentthreadengine.h>
#include <qexception.h>
#include <QThread>
#include <QtTest/QtTest>

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
    ThreadFunctionResult threadFunction()
    {
        QTest::qSleep(50);
        QTest::qSleep(100);
        return ThreadFinished;
    }
};

void tst_QtConcurrentThreadEngine::runDirectly()
{
    {
        PrintUser engine;
        engine.startSingleThreaded();
        engine.startBlocking();
    }
    {
        PrintUser *engine = new PrintUser();
        QFuture<void> f = engine->startAsynchronously();
        f.waitForFinished();
    }
}

class StringResultUser : public ThreadEngine<QString>
{
public:
    typedef QString ResultType;
    StringResultUser()
    : done(false) { }

    bool shouldStartThread()
    {
        return !done;
    }

    ThreadFunctionResult threadFunction()
    {
        done = true;
        return ThreadFinished;
    }

    QString *result()
    {
        foo = "Foo";
        return &foo;
    }
    QString foo;
    bool done;
};

void tst_QtConcurrentThreadEngine::result()
{
    StringResultUser engine;
    QCOMPARE(*engine.startBlocking(), QString("Foo"));
}

class VoidResultUser : public ThreadEngine<void>
{
public:
    bool shouldStartThread()
    {
        return !done;
    }

    ThreadFunctionResult threadFunction()
    {
        done = true;
        return ThreadFinished;
    }

    void *result()
    {
        return 0;
    }
    bool done;
};

void tst_QtConcurrentThreadEngine::runThroughStarter()
{
    {
        ThreadEngineStarter<QString> starter = startThreadEngine(new StringResultUser());
        QFuture<QString>  f = starter.startAsynchronously();
        QCOMPARE(f.result(), QString("Foo"));
    }

    {
        ThreadEngineStarter<QString> starter = startThreadEngine(new StringResultUser());
        QString str = starter.startBlocking();
        QCOMPARE(str, QString("Foo"));
    }
}

class CancelUser : public ThreadEngine<void>
{
public:
    void *result()
    {
        return 0;
    }

    ThreadFunctionResult threadFunction()
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
    {
        count.store(initialCount = 100);
        finishing = false;
    }

    bool shouldStartThread()
    {
        return !finishing;
    }

    ThreadFunctionResult threadFunction()
    {
        forever {
            const int local = count.load();
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
        QCOMPARE(count.load(), 0);
    }

    for (int i = 0; i < repeats; ++i) {
        ThrottleAlwaysUser t;
        t.startBlocking();
        QCOMPARE(count.load(), 0);
    }
}

QSet<QThread *> threads;
QMutex mutex;
class ThreadCountUser : public ThreadEngine<void>
{
public:
    ThreadCountUser(bool finishImmediately = false)
    {
        threads.clear();
        finishing = finishImmediately;
    }

    bool shouldStartThread()
    {
        return !finishing;
    }

    ThreadFunctionResult threadFunction()
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
   //QTBUG-23333: This test is unstable

    const int repeats = 10;
    for (int i = 0; i < repeats; ++i) {
        ThreadCountUser t;
        t.startBlocking();
        int count = threads.count();
        int count_expected = QThreadPool::globalInstance()->maxThreadCount() + 1; // +1 for the main thread.
        if (count != count_expected)
            QEXPECT_FAIL("", "QTBUG-23333", Abort);
        QCOMPARE(count, count_expected);

        (new ThreadCountUser())->startAsynchronously().waitForFinished();
        count = threads.count();
        count_expected = QThreadPool::globalInstance()->maxThreadCount();
        if (count != count_expected)
            QEXPECT_FAIL("", "QTBUG-23333", Abort);
        QCOMPARE(count, count_expected);
    }

    // Set the finish flag immediately, this should give us one thread only.
    for (int i = 0; i < repeats; ++i) {
        ThreadCountUser t(true /*finishImmediately*/);
        t.startBlocking();
        int count = threads.count();
        if (count != 1)
            QEXPECT_FAIL("", "QTBUG-23333", Abort);
        QCOMPARE(count, 1);

        (new ThreadCountUser(true /*finishImmediately*/))->startAsynchronously().waitForFinished();
        count = threads.count();
        if (count != 1)
            QEXPECT_FAIL("", "QTBUG-23333", Abort);
        QCOMPARE(count, 1);
    }
}

class MultipleResultsUser : public ThreadEngine<int>
{
public:
    bool shouldStartThread()
    {
        return false;
    }

    ThreadFunctionResult threadFunction()
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
    QCOMPARE(f.results().count() , 10);
    QCOMPARE(f.resultAt(0), 0);
    QCOMPARE(f.resultAt(5), 5);
    QCOMPARE(f.resultAt(9), 9);
    f.waitForFinished();
}


class NoThreadsUser : public ThreadEngine<void>
{
public:
    bool shouldStartThread()
    {
        return false;
    }

    ThreadFunctionResult threadFunction()
    {
        return ThreadFinished;
    }

    void *result()
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
    bool shouldStartThread() { return false; }
    ThreadFunctionResult threadFunction() { QTest::qSleep(sleepTime); return ThreadFinished; }
};

void tst_QtConcurrentThreadEngine::cancelQueuedSlowUser()
{
    const int times = 100;

    QTime t;
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
    QtConcurrentExceptionThrower(QThread *blockThread = 0)
    {
        this->blockThread = blockThread;
    }

    ThreadFunctionResult threadFunction()
    {
        QTest::qSleep(50);
        throw QException();
        return ThreadFinished;
    }
    QThread *blockThread;
};

class UnrelatedExceptionThrower : public ThreadEngine<void>
{
public:
    UnrelatedExceptionThrower(QThread *blockThread = 0)
    {
        this->blockThread = blockThread;
    }

    ThreadFunctionResult threadFunction()
    {
        QTest::qSleep(50);
        throw int();
        return ThreadFinished;
    }
    QThread *blockThread;
};

void tst_QtConcurrentThreadEngine::exceptions()
{
    // Asynchronous mode:
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

    // Blocking mode:
    // test throwing the exception from a worker thread.
    {
        bool caught = false;
        try  {
            QtConcurrentExceptionThrower e(QThread::currentThread());
            e.startBlocking();
        } catch (const QException &) {
            caught = true;
        }
        QVERIFY2(caught, "did not get exception");
    }

    // test throwing the exception from the main thread (different code path)
    {
        bool caught = false;
        try  {
            QtConcurrentExceptionThrower e(0);
            e.startBlocking();
        } catch (const QException &) {
            caught = true;
        }
        QVERIFY2(caught, "did not get exception");
    }

    // Asynchronous mode:
    {
        bool caught = false;
        try  {
            UnrelatedExceptionThrower *e = new UnrelatedExceptionThrower();
            QFuture<void> f = e->startAsynchronously();
            f.waitForFinished();
        } catch (const QUnhandledException &) {
            caught = true;
        }
        QVERIFY2(caught, "did not get exception");
    }

    // Blocking mode:
    // test throwing the exception from a worker thread.
    {
        bool caught = false;
        try  {
            UnrelatedExceptionThrower e(QThread::currentThread());
            e.startBlocking();
        } catch (const QUnhandledException &) {
            caught = true;
        }
        QVERIFY2(caught, "did not get exception");
    }

    // test throwing the exception from the main thread (different code path)
    {
        bool caught = false;
        try  {
            UnrelatedExceptionThrower e(0);
            e.startBlocking();
        } catch (const QUnhandledException &) {
            caught = true;
        }
        QVERIFY2(caught, "did not get exception");
    }
}

#endif

QTEST_MAIN(tst_QtConcurrentThreadEngine)

#include "tst_qtconcurrentthreadengine.moc"
