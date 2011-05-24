/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>


#include <qabstracteventdispatcher.h>
#include <qcoreapplication.h>
#include <qcoreevent.h>
#include <qeventloop.h>
#include <qmutex.h>
#include <qthread.h>
#include <qtimer.h>
#include <qwaitcondition.h>
#include <QTcpServer>
#include <QTcpSocket>

#ifdef Q_OS_SYMBIAN
#include <e32base.h>
#include <unistd.h>
#endif

//TESTED_CLASS=
//TESTED_FILES=

class EventLoopExiter : public QObject
{
    Q_OBJECT
    QEventLoop *eventLoop;
public:
    inline EventLoopExiter(QEventLoop *el)
        : eventLoop(el)
    { }
public slots:
    void exit();
    void exit1();
    void exit2();
};

void EventLoopExiter::exit()
{ eventLoop->exit(); }

void EventLoopExiter::exit1()
{ eventLoop->exit(1); }

void EventLoopExiter::exit2()
{ eventLoop->exit(2); }

class EventLoopThread : public QThread
{
    Q_OBJECT
signals:
    void checkPoint();
public:
    QEventLoop *eventLoop;
    void run();
};

void EventLoopThread::run()
{
    eventLoop = new QEventLoop;
    emit checkPoint();
    (void) eventLoop->exec();
    delete eventLoop;
    eventLoop = 0;
}

class MultipleExecThread : public QThread
{
    Q_OBJECT
signals:
    void checkPoint();
public:
    QMutex mutex;
    QWaitCondition cond;
    volatile int result1;
    volatile int result2;
    MultipleExecThread() : result1(0xdead), result2(0xbeef) {}

    void run()
    {
        QMutexLocker locker(&mutex);
        // this exec should work

        cond.wakeOne();
        cond.wait(&mutex);

        QTimer timer;
        connect(&timer, SIGNAL(timeout()), SLOT(quit()), Qt::DirectConnection);
        timer.setInterval(1000);
        timer.start();
        result1 = exec();

        // this should return immediately, since exit() has been called
        cond.wakeOne();
        cond.wait(&mutex);
        QEventLoop eventLoop;
        result2 = eventLoop.exec();
    }
};

class StartStopEvent: public QEvent
{
public:
    StartStopEvent(int type, QEventLoop *loop = 0)
        : QEvent(Type(type)), el(loop)
    { }

    QEventLoop *el;
};

class EventLoopExecutor : public QObject
{
    Q_OBJECT
    QEventLoop *eventLoop;
public:
    int returnCode;
    EventLoopExecutor(QEventLoop *eventLoop)
        : QObject(), eventLoop(eventLoop), returnCode(-42)
    {
    }
public slots:
    void exec()
    {
        QTimer::singleShot(100, eventLoop, SLOT(quit()));
        // this should return immediately, and the timer event should be delivered to
        // tst_QEventLoop::exec() test, letting the test complete
        returnCode = eventLoop->exec();
    }
};

#ifndef QT_NO_EXCEPTIONS
class QEventLoopTestException { };

class ExceptionThrower : public QObject
{
    Q_OBJECT
public:
    ExceptionThrower() : QObject() { }
public slots:
    void throwException()
    {
        QEventLoopTestException e;
        throw e;
    }
};
#endif

class tst_QEventLoop : public QObject
{
    Q_OBJECT
public:
    tst_QEventLoop();
    ~tst_QEventLoop();
public slots:
    void init();
    void cleanup();
private slots:
    // This test *must* run first. See the definition for why.
    void onlySymbianActiveScheduler();
    void symbianNestedActiveSchedulerLoop_data();
    void symbianNestedActiveSchedulerLoop();
    void processEvents();
    void exec();
    void reexec();
    void exit();
    void execAfterExit();
    void wakeUp();
    void quit();
    void processEventsExcludeSocket();
    void processEventsExcludeTimers();

    // keep this test last:
    void nestedLoops();

protected:
    void customEvent(QEvent *e);
};

tst_QEventLoop::tst_QEventLoop()
{ }

tst_QEventLoop::~tst_QEventLoop()
{ }

void tst_QEventLoop::init()
{ }

void tst_QEventLoop::cleanup()
{ }

#ifdef Q_OS_SYMBIAN
class OnlySymbianActiveScheduler_helper : public QObject
{
    Q_OBJECT

public:
    OnlySymbianActiveScheduler_helper(int fd, QTimer *zeroTimer)
        : fd(fd),
          timerCount(0),
          zeroTimer(zeroTimer),
          zeroTimerCount(0),
          notifierCount(0)
    {
    }
    ~OnlySymbianActiveScheduler_helper() {}

public slots:
    void timerSlot()
    {
        // Let all the events occur twice so we know they reactivated after
        // each occurrence.
        if (++timerCount >= 2) {
            // This will hopefully run last, so stop the active scheduler.
            CActiveScheduler::Stop();
        }
    }
    void zeroTimerSlot()
    {
        if (++zeroTimerCount >= 2) {
            zeroTimer->stop();
        }
    }
    void notifierSlot()
    {
        if (++notifierCount >= 2) {
            char dummy;
            ::read(fd, &dummy, 1);
        }
    }

private:
    int fd;
    int timerCount;
    QTimer *zeroTimer;
    int zeroTimerCount;
    int notifierCount;
};
#endif

void tst_QEventLoop::onlySymbianActiveScheduler() {
#ifndef Q_OS_SYMBIAN
    QSKIP("This is a Symbian-only test.", SkipAll);
#else
    // In here we try to use timers and sockets exclusively using the Symbian
    // active scheduler and no processEvents().
    // This test should therefore be run first, so that we can verify that
    // the first occurrence of processEvents does not do any initalization that
    // we depend on.

    // Open up a pipe so we can test socket notifiers.
    int pipeEnds[2];
    if (::pipe(pipeEnds) != 0) {
        QFAIL("Could not open pipe");
    }
    QSocketNotifier notifier(pipeEnds[0], QSocketNotifier::Read);
    QSignalSpy notifierSpy(&notifier, SIGNAL(activated(int)));
    char dummy = 1;
    ::write(pipeEnds[1], &dummy, 1);

    QTimer zeroTimer;
    QSignalSpy zeroTimerSpy(&zeroTimer, SIGNAL(timeout()));
    zeroTimer.setInterval(0);
    zeroTimer.start();

    QTimer timer;
    QSignalSpy timerSpy(&timer, SIGNAL(timeout()));
    timer.setInterval(2000); // Generous timeout or this test will fail if there is high load
    timer.start();

    OnlySymbianActiveScheduler_helper helper(pipeEnds[0], &zeroTimer);
    connect(&notifier, SIGNAL(activated(int)), &helper, SLOT(notifierSlot()));
    connect(&zeroTimer, SIGNAL(timeout()), &helper, SLOT(zeroTimerSlot()));
    connect(&timer, SIGNAL(timeout()), &helper, SLOT(timerSlot()));

    CActiveScheduler::Start();

    ::close(pipeEnds[1]);
    ::close(pipeEnds[0]);

    QCOMPARE(notifierSpy.count(), 2);
    QCOMPARE(zeroTimerSpy.count(), 2);
    QCOMPARE(timerSpy.count(), 2);
#endif
}

void tst_QEventLoop::processEvents()
{
    QSignalSpy spy1(QAbstractEventDispatcher::instance(), SIGNAL(aboutToBlock()));
    QSignalSpy spy2(QAbstractEventDispatcher::instance(), SIGNAL(awake()));

    QEventLoop eventLoop;

    QCoreApplication::postEvent(&eventLoop, new QEvent(QEvent::User));

    // process posted events, QEventLoop::processEvents() should return
    // true
    QVERIFY(eventLoop.processEvents());
    QCOMPARE(spy1.count(), 0);
    QCOMPARE(spy2.count(), 1);

    // allow any session manager to complete its handshake, so that
    // there are no pending events left.
    while (eventLoop.processEvents())
        ;

    // On mac we get application started events at this point,
    // so process events one more time just to be sure.
    eventLoop.processEvents();

    // no events to process, QEventLoop::processEvents() should return
    // false
    spy1.clear();
    spy2.clear();
    QVERIFY(!eventLoop.processEvents());
    QCOMPARE(spy1.count(), 0);
    QCOMPARE(spy2.count(), 1);

    // make sure the test doesn't block forever
    int timerId = startTimer(100);

    // wait for more events to process, QEventLoop::processEvents()
    // should return true
    spy1.clear();
    spy2.clear();
    QVERIFY(eventLoop.processEvents(QEventLoop::WaitForMoreEvents));

    // Verify that the eventloop has blocked and woken up. Some eventloops
    // may block and wake up multiple times.
    QVERIFY(spy1.count() > 0);
    QVERIFY(spy2.count() > 0);
    // We should get one awake for each aboutToBlock, plus one awake when
    // processEvents is entered.
    QVERIFY(spy2.count() >= spy1.count());

    killTimer(timerId);
}

#if defined(Q_OS_SYMBIAN) && defined(Q_CC_NOKIAX86)
// Symbian needs bit longer timeout for emulator, as emulator startup causes additional delay
#  define EXEC_TIMEOUT 1000
#else
#  define EXEC_TIMEOUT 100
#endif


void tst_QEventLoop::exec()
{
    {
        QEventLoop eventLoop;
        EventLoopExiter exiter(&eventLoop);
        int returnCode;

        QTimer::singleShot(EXEC_TIMEOUT, &exiter, SLOT(exit()));
        returnCode = eventLoop.exec();
        QCOMPARE(returnCode, 0);

        QTimer::singleShot(EXEC_TIMEOUT, &exiter, SLOT(exit1()));
        returnCode = eventLoop.exec();
        QCOMPARE(returnCode, 1);

        QTimer::singleShot(EXEC_TIMEOUT, &exiter, SLOT(exit2()));
        returnCode = eventLoop.exec();
        QCOMPARE(returnCode, 2);
    }

    {
        // calling QEventLoop::exec() after a thread loop has exit()ed should return immediately
        // Note: this behaviour differs from QCoreApplication and QEventLoop
        // see tst_QCoreApplication::eventLoopExecAfterExit, tst_QEventLoop::reexec
        MultipleExecThread thread;

        // start thread and wait for checkpoint
        thread.mutex.lock();
        thread.start();
        thread.cond.wait(&thread.mutex);

        // make sure the eventloop runs
        QSignalSpy spy(QAbstractEventDispatcher::instance(&thread), SIGNAL(awake()));
        thread.cond.wakeOne();
        thread.cond.wait(&thread.mutex);
        QVERIFY(spy.count() > 0);
        int v = thread.result1;
        QCOMPARE(v, 0);

        // exec should return immediately
        spy.clear();
        thread.cond.wakeOne();
        thread.mutex.unlock();
        thread.wait();
        QCOMPARE(spy.count(), 0);
        v = thread.result2;
        QCOMPARE(v, -1);
    }

    {
        // a single instance of QEventLoop should not be allowed to recurse into exec()
        QEventLoop eventLoop;
        EventLoopExecutor executor(&eventLoop);

        QTimer::singleShot(EXEC_TIMEOUT, &executor, SLOT(exec()));
        int returnCode = eventLoop.exec();
        QCOMPARE(returnCode, 0);
        QCOMPARE(executor.returnCode, -1);
    }

#if !defined(QT_NO_EXCEPTIONS) && !defined(Q_OS_WINCE_WM) && !defined(Q_OS_SYMBIAN) && !defined(NO_EVENTLOOP_EXCEPTIONS)
    // Windows Mobile cannot handle cross library exceptions
    // qobject.cpp will try to rethrow the exception after handling
    // which causes gwes.exe to crash

    // Symbian doesn't propagate exceptions from eventloop, but converts them to
    // CActiveScheduler errors instead -> this test will hang.
    {
        // QEventLoop::exec() is exception safe
        QEventLoop eventLoop;
        int caughtExceptions = 0;

        try {
            ExceptionThrower exceptionThrower;
            QTimer::singleShot(EXEC_TIMEOUT, &exceptionThrower, SLOT(throwException()));
            (void) eventLoop.exec();
        } catch (...) {
            ++caughtExceptions;
        }
        try {
            ExceptionThrower exceptionThrower;
            QTimer::singleShot(EXEC_TIMEOUT, &exceptionThrower, SLOT(throwException()));
            (void) eventLoop.exec();
        } catch (...) {
            ++caughtExceptions;
        }
        QCOMPARE(caughtExceptions, 2);
    }
#endif
}

void tst_QEventLoop::reexec()
{
    QEventLoop loop;

    // exec once
    QMetaObject::invokeMethod(&loop, "quit", Qt::QueuedConnection);
    QCOMPARE(loop.exec(), 0);

    // and again
    QMetaObject::invokeMethod(&loop, "quit", Qt::QueuedConnection);
    QCOMPARE(loop.exec(), 0);
}

void tst_QEventLoop::exit()
{ DEPENDS_ON(exec()); }

void tst_QEventLoop::execAfterExit()
{
    QEventLoop loop;
    EventLoopExiter obj(&loop);

    QMetaObject::invokeMethod(&obj, "exit", Qt::QueuedConnection);
    loop.exit(1);
    QCOMPARE(loop.exec(), 0);
}

void tst_QEventLoop::wakeUp()
{
    EventLoopThread thread;
    QEventLoop eventLoop;
    connect(&thread, SIGNAL(checkPoint()), &eventLoop, SLOT(quit()));
    connect(&thread, SIGNAL(finished()), &eventLoop, SLOT(quit()));

    thread.start();
    (void) eventLoop.exec();

    QSignalSpy spy(QAbstractEventDispatcher::instance(&thread), SIGNAL(awake()));
    thread.eventLoop->wakeUp();

    // give the thread time to wake up
    QTimer::singleShot(1000, &eventLoop, SLOT(quit()));
    (void) eventLoop.exec();

    QVERIFY(spy.count() > 0);

    thread.quit();
    (void) eventLoop.exec();
}

void tst_QEventLoop::quit()
{
    QEventLoop eventLoop;
    int returnCode;

    QTimer::singleShot(100, &eventLoop, SLOT(quit()));
    returnCode = eventLoop.exec();
    QCOMPARE(returnCode, 0);
}


void tst_QEventLoop::nestedLoops()
{
    QCoreApplication::postEvent(this, new StartStopEvent(QEvent::User));
    QCoreApplication::postEvent(this, new StartStopEvent(QEvent::User));
    QCoreApplication::postEvent(this, new StartStopEvent(QEvent::User));

    // without the fix, this will *wedge* and never return
    QTest::qWait(1000);
}

void tst_QEventLoop::customEvent(QEvent *e)
{
    if (e->type() == QEvent::User) {
        QEventLoop loop;
        QCoreApplication::postEvent(this, new StartStopEvent(int(QEvent::User) + 1, &loop));
        loop.exec();
    } else {
        static_cast<StartStopEvent *>(e)->el->exit();
    }
}

class SocketEventsTester: public QObject
{
    Q_OBJECT
public:
    SocketEventsTester()
    {
        socket = 0;
        server = 0;
        dataArrived = false;
        testResult = false;
    }
    ~SocketEventsTester()
    {
        delete socket;
        delete server;
    }
    bool init()
    {
        bool ret = false;
        server = new QTcpServer();
        socket = new QTcpSocket();
        connect(server, SIGNAL(newConnection()), this, SLOT(sendHello()));
        connect(socket, SIGNAL(readyRead()), this, SLOT(sendAck()), Qt::DirectConnection);
        if((ret = server->listen(QHostAddress::LocalHost, 0))) {
            socket->connectToHost(server->serverAddress(), server->serverPort());
            socket->waitForConnected();
        }
        return ret;
    }

    QTcpSocket *socket;
    QTcpServer *server;
    bool dataArrived;
    bool testResult;
public slots:
    void sendAck()
    {
        dataArrived = true;
    }
    void sendHello()
    {
        char data[10] ="HELLO";
        qint64 size = sizeof(data);

        QTcpSocket *serverSocket = server->nextPendingConnection();
        serverSocket->write(data, size);
        serverSocket->flush();
        QTest::qSleep(200); //allow the TCP/IP stack time to loopback the data, so our socket is ready to read
        QCoreApplication::processEvents(QEventLoop::ExcludeSocketNotifiers);
        testResult = dataArrived;
        QCoreApplication::processEvents(); //check the deferred event is processed
        serverSocket->close();
        QThread::currentThread()->exit(0);
    }
};

class SocketTestThread : public QThread
{
    Q_OBJECT
public:
    SocketTestThread():QThread(0),testResult(false){};
    void run()
    {
        SocketEventsTester *tester = new SocketEventsTester();
        if (tester->init())
            exec();
        testResult = tester->testResult;
        dataArrived = tester->dataArrived;
        delete tester;
    }
     bool testResult;
     bool dataArrived;
};

void tst_QEventLoop::processEventsExcludeSocket()
{
    SocketTestThread thread;
    thread.start();
    QVERIFY(thread.wait());
    QVERIFY(!thread.testResult);
    QVERIFY(thread.dataArrived);
}

class TimerReceiver : public QObject
{
public:
    int gotTimerEvent;

    TimerReceiver()
        : QObject(), gotTimerEvent(-1)
    { }

    void timerEvent(QTimerEvent *event)
    {
        gotTimerEvent = event->timerId();
    }
};

void tst_QEventLoop::processEventsExcludeTimers()
{
    TimerReceiver timerReceiver;
    int timerId = timerReceiver.startTimer(0);

    QEventLoop eventLoop;

    // normal process events will send timers
    eventLoop.processEvents();
    QCOMPARE(timerReceiver.gotTimerEvent, timerId);
    timerReceiver.gotTimerEvent = -1;

    // normal process events will send timers
    eventLoop.processEvents(QEventLoop::X11ExcludeTimers);
#if !defined(Q_OS_UNIX) || defined(Q_OS_SYMBIAN)
    QEXPECT_FAIL("", "X11ExcludeTimers only works on UN*X", Continue);
#endif
    QCOMPARE(timerReceiver.gotTimerEvent, -1);
    timerReceiver.gotTimerEvent = -1;

    // resume timer processing
    eventLoop.processEvents();
    QCOMPARE(timerReceiver.gotTimerEvent, timerId);
    timerReceiver.gotTimerEvent = -1;
}

#ifdef Q_OS_SYMBIAN
class DummyActiveObject : public CActive
{
public:
    DummyActiveObject(int levels);
    ~DummyActiveObject();

    void Start();

protected:
    void DoCancel();
    void RunL();

public:
    bool succeeded;

private:
    RTimer m_rTimer;
    int remainingLevels;
};

class ActiveSchedulerLoop : public QObject
{
public:
    ActiveSchedulerLoop(int levels) : succeeded(false), timerId(-1), remainingLevels(levels) {}
    ~ActiveSchedulerLoop() {}

    void timerEvent(QTimerEvent *e);

public:
    bool succeeded;
    int timerId;
    int remainingLevels;
};

DummyActiveObject::DummyActiveObject(int levels)
    : CActive(CActive::EPriorityStandard),
      succeeded(false),
      remainingLevels(levels)
{
    m_rTimer.CreateLocal();
}

DummyActiveObject::~DummyActiveObject()
{
    Cancel();
    m_rTimer.Close();
}

void DummyActiveObject::DoCancel()
{
    m_rTimer.Cancel();
}

void DummyActiveObject::RunL()
{
    if (remainingLevels - 1 <= 0) {
        ActiveSchedulerLoop loop(remainingLevels - 1);
        loop.timerId = loop.startTimer(0);
        QCoreApplication::processEvents();

        succeeded = loop.succeeded;
    } else {
        succeeded = true;
    }
    CActiveScheduler::Stop();
}

void DummyActiveObject::Start()
{
    m_rTimer.After(iStatus, 100000); // 100 ms
    SetActive();
}

void ActiveSchedulerLoop::timerEvent(QTimerEvent *e)
{
    Q_UNUSED(e);
    DummyActiveObject *dummy = new(ELeave) DummyActiveObject(remainingLevels);
    CActiveScheduler::Add(dummy);

    dummy->Start();

    CActiveScheduler::Start();

    succeeded = dummy->succeeded;

    delete dummy;

    killTimer(timerId);
}

// We cannot trap panics when the test case fails, so run it in a different thread instead.
class ActiveSchedulerThread : public QThread
{
public:
    ActiveSchedulerThread(QEventLoop::ProcessEventsFlag flags);
    ~ActiveSchedulerThread();

protected:
    void run();

public:
    volatile bool succeeded;

private:
    QEventLoop::ProcessEventsFlag m_flags;
};

ActiveSchedulerThread::ActiveSchedulerThread(QEventLoop::ProcessEventsFlag flags)
    : succeeded(false),
      m_flags(flags)
{
}

ActiveSchedulerThread::~ActiveSchedulerThread()
{
}

void ActiveSchedulerThread::run()
{
    ActiveSchedulerLoop loop(2);
    loop.timerId = loop.startTimer(0);
    // It may panic in here if the active scheduler and the Qt loop don't go together.
    QCoreApplication::processEvents(m_flags);

    succeeded = loop.succeeded;
}
#endif // ifdef Q_OS_SYMBIAN

void tst_QEventLoop::symbianNestedActiveSchedulerLoop_data()
{
    QTest::addColumn<int>("processEventFlags");

    QTest::newRow("AllEvents") << (int)QEventLoop::AllEvents;
    QTest::newRow("WaitForMoreEvents") << (int)QEventLoop::WaitForMoreEvents;
}

/*
  Before you start fiddling with this test, you should have a good understanding of how
  Symbian active objects work. What the test does is to try to screw up the semaphore count
  in the active scheduler to cause stray signals, by running the Qt event loop and the
  active scheduler inside each other. Naturally, its attempts to do this should be futile!
*/
void tst_QEventLoop::symbianNestedActiveSchedulerLoop()
{
#ifndef Q_OS_SYMBIAN
    QSKIP("This is a Symbian only test.", SkipAll);
#else
    QFETCH(int, processEventFlags);

    ActiveSchedulerThread thread((QEventLoop::ProcessEventsFlag)processEventFlags);
    thread.start();
    thread.wait(2000);

    QVERIFY(thread.succeeded);
#endif
}

QTEST_MAIN(tst_QEventLoop)
#include "tst_qeventloop.moc"
