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

#include <qabstracteventdispatcher.h>
#include <qcoreapplication.h>
#include <qcoreevent.h>
#include <qeventloop.h>
#include <private/qeventloop_p.h>
#if defined(Q_OS_UNIX)
  #include <private/qeventdispatcher_unix_p.h>
  #include <QtCore/private/qcore_unix_p.h>
  #if defined(HAVE_GLIB)
    #include <private/qeventdispatcher_glib_p.h>
  #endif
#endif
#include <qmutex.h>
#include <qthread.h>
#include <qtimer.h>
#include <qwaitcondition.h>
#include <QTcpServer>
#include <QTcpSocket>

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
    explicit StartStopEvent(int type, QEventLoop *loop = 0)
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

#ifdef QT_GUI_LIB
  #define tst_QEventLoop tst_QGuiEventLoop
#endif

class tst_QEventLoop : public QObject
{
    Q_OBJECT
private slots:
    // This test *must* run first. See the definition for why.
    void processEvents();
    void exec();
    void reexec();
    void execAfterExit();
    void wakeUp();
    void quit();
#if defined(Q_OS_UNIX)
    void processEventsExcludeSocket();
#endif
    void processEventsExcludeTimers();
    void deliverInDefinedOrder();

    // keep this test last:
    void nestedLoops();

    void testQuitLock();

protected:
    void customEvent(QEvent *e);
};

void tst_QEventLoop::processEvents()
{
    QSignalSpy aboutToBlockSpy(QAbstractEventDispatcher::instance(), &QAbstractEventDispatcher::aboutToBlock);
    QSignalSpy awakeSpy(QAbstractEventDispatcher::instance(), &QAbstractEventDispatcher::awake);

    QVERIFY(aboutToBlockSpy.isValid());
    QVERIFY(awakeSpy.isValid());

    QEventLoop eventLoop;

    QCoreApplication::postEvent(&eventLoop, new QEvent(QEvent::User));

    // process posted events, QEventLoop::processEvents() should return
    // true
    QVERIFY(eventLoop.processEvents());
    QCOMPARE(aboutToBlockSpy.count(), 0);
    QCOMPARE(awakeSpy.count(), 1);

    // allow any session manager to complete its handshake, so that
    // there are no pending events left. This tests that we are able
    // to process all events from the queue, otherwise it will hang.
    while (eventLoop.processEvents())
        ;

    // make sure the test doesn't block forever
    int timerId = startTimer(100);

    // wait for more events to process, QEventLoop::processEvents()
    // should return true
    aboutToBlockSpy.clear();
    awakeSpy.clear();
    QVERIFY(eventLoop.processEvents(QEventLoop::WaitForMoreEvents));

    // We should get one awake for each aboutToBlock, plus one awake when
    // processEvents is entered. There is no guarantee that that the
    // processEvents call actually blocked, since the OS may introduce
    // native events at any time.
    QVERIFY(awakeSpy.count() > 0);
    QVERIFY(awakeSpy.count() >= aboutToBlockSpy.count());

    killTimer(timerId);
}

#define EXEC_TIMEOUT 100

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
        QSignalSpy spy(QAbstractEventDispatcher::instance(&thread), &QAbstractEventDispatcher::awake);
        QVERIFY(spy.isValid());
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

    QSignalSpy spy(QAbstractEventDispatcher::instance(&thread), &QAbstractEventDispatcher::awake);
    QVERIFY(spy.isValid());
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

#if defined(Q_OS_UNIX)
class SocketEventsTester: public QObject
{
    Q_OBJECT
public:
    SocketEventsTester()
    {
        socket = 0;
        server = 0;
        dataSent = false;
        dataReadable = false;
        testResult = false;
        dataArrived = false;
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
    bool dataSent;
    bool dataReadable;
    bool testResult;
    bool dataArrived;
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
        QCoreApplication::processEvents();
        serverSocket->write(data, size);
        dataSent = serverSocket->waitForBytesWritten(-1);

        if (dataSent) {
            pollfd pfd = qt_make_pollfd(socket->socketDescriptor(), POLLIN);
            dataReadable = (1 == qt_safe_poll(&pfd, 1, nullptr));
        }

        if (!dataReadable) {
            testResult = dataArrived;
        } else {
            QCoreApplication::processEvents(QEventLoop::ExcludeSocketNotifiers);
            testResult = dataArrived;
            // to check if the deferred event is processed
            QCoreApplication::processEvents();
        }
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
        dataSent = tester->dataSent;
        dataReadable = tester->dataReadable;
        testResult = tester->testResult;
        dataArrived = tester->dataArrived;
        delete tester;
    }
    bool dataSent;
    bool dataReadable;
    bool testResult;
    bool dataArrived;
};

void tst_QEventLoop::processEventsExcludeSocket()
{
    SocketTestThread thread;
    thread.start();
    QVERIFY(thread.wait());
    QVERIFY(thread.dataSent);
    QVERIFY(thread.dataReadable);
  #if defined(HAVE_GLIB)
    QAbstractEventDispatcher *eventDispatcher = QCoreApplication::eventDispatcher();
    if (qobject_cast<QEventDispatcherGlib *>(eventDispatcher))
        QEXPECT_FAIL("", "ExcludeSocketNotifiers is currently broken in the Glib dispatchers", Continue);
  #endif
    QVERIFY(!thread.testResult);
    QVERIFY(thread.dataArrived);
}
#endif

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

    // but not if we exclude timers
    eventLoop.processEvents(QEventLoop::X11ExcludeTimers);

#if defined(Q_OS_UNIX)
    QAbstractEventDispatcher *eventDispatcher = QCoreApplication::eventDispatcher();
    if (!qobject_cast<QEventDispatcherUNIX *>(eventDispatcher)
  #if defined(HAVE_GLIB)
        && !qobject_cast<QEventDispatcherGlib *>(eventDispatcher)
  #endif
        )
#endif
        QEXPECT_FAIL("", "X11ExcludeTimers only supported in the UNIX/Glib dispatchers", Continue);

    QCOMPARE(timerReceiver.gotTimerEvent, -1);
    timerReceiver.gotTimerEvent = -1;

    // resume timer processing
    eventLoop.processEvents();
    QCOMPARE(timerReceiver.gotTimerEvent, timerId);
    timerReceiver.gotTimerEvent = -1;
}

namespace DeliverInDefinedOrder {
    enum { NbThread = 3,  NbObject = 500, NbEventQueue = 5, NbEvent = 50 };

    struct CustomEvent : public QEvent {
        CustomEvent(int q, int v) : QEvent(Type(User + q)), value(v) {}
        int value;
    };

    struct Object : public QObject {
        Q_OBJECT
    public:
        Object() : count(0) {
            for (int i = 0; i < NbEventQueue;  i++)
                lastReceived[i] = -1;
        }
        int lastReceived[NbEventQueue];
        int count;
        virtual void customEvent(QEvent* e) {
            QVERIFY(e->type() >= QEvent::User);
            QVERIFY(e->type() < QEvent::User + 5);
            uint idx = e->type() - QEvent::User;
            int value = static_cast<CustomEvent *>(e)->value;
            QVERIFY(lastReceived[idx] < value);
            lastReceived[idx] = value;
            count++;
        }

    public slots:
        void moveToThread(QThread *t) {
            QObject::moveToThread(t);
        }
    };

}

void tst_QEventLoop::deliverInDefinedOrder()
{
    using namespace DeliverInDefinedOrder;
    qMetaTypeId<QThread*>();
    QThread threads[NbThread];
    Object objects[NbObject];
    for (int t = 0; t < NbThread; t++) {
        threads[t].start();
    }

    int event = 0;

    for (int o = 0; o < NbObject; o++) {
        objects[o].moveToThread(&threads[o % NbThread]);
        for (int e = 0; e < NbEvent; e++) {
            int q = e % NbEventQueue;
            QCoreApplication::postEvent(&objects[o], new CustomEvent(q, ++event) , q);
            if (e % 7)
                QMetaObject::invokeMethod(&objects[o], "moveToThread", Qt::QueuedConnection, Q_ARG(QThread*, &threads[(e+o)%NbThread]));
        }
    }

    for (int o = 0; o < NbObject; o++) {
        QTRY_COMPARE(objects[o].count, int(NbEvent));
    }

    for (int t = 0; t < NbThread; t++) {
        threads[t].quit();
        threads[t].wait();
    }

}

class JobObject : public QObject
{
    Q_OBJECT
public:

    explicit JobObject(QEventLoop *loop, QObject *parent = 0)
        : QObject(parent), locker(loop)
    {
    }

    explicit JobObject(QObject *parent = 0)
        : QObject(parent)
    {
    }

public slots:
    void start(int timeout = 200)
    {
        QTimer::singleShot(timeout, this, SLOT(timeout()));
    }

private slots:
    void timeout()
    {
        emit done();
        deleteLater();
    }

signals:
    void done();

private:
    QEventLoopLocker locker;
};

void tst_QEventLoop::testQuitLock()
{
    QEventLoop eventLoop;

    QEventLoopPrivate* privateClass = static_cast<QEventLoopPrivate*>(QObjectPrivate::get(&eventLoop));

    QCOMPARE(privateClass->quitLockRef.load(), 0);

    JobObject *job1 = new JobObject(&eventLoop, this);
    job1->start(500);

    QCOMPARE(privateClass->quitLockRef.load(), 1);

    eventLoop.exec();

    QCOMPARE(privateClass->quitLockRef.load(), 0);


    job1 = new JobObject(&eventLoop, this);
    job1->start(200);

    JobObject *previousJob = job1;
    for (int i = 0; i < 9; ++i) {
        JobObject *subJob = new JobObject(&eventLoop, this);
        connect(previousJob, SIGNAL(done()), subJob, SLOT(start()));
        previousJob = subJob;
    }

    eventLoop.exec();
}

QTEST_MAIN(tst_QEventLoop)
#include "tst_qeventloop.moc"
