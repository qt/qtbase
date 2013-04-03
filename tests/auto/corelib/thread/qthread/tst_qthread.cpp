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

#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qdatetime.h>
#include <qmutex.h>
#include <qthread.h>
#include <qtimer.h>
#include <qwaitcondition.h>
#include <qdebug.h>
#include <qmetaobject.h>

#ifdef Q_OS_UNIX
#include <pthread.h>
#endif
#if defined(Q_OS_WINCE)
#include <windows.h>
#elif defined(Q_OS_WIN)
#include <process.h>
#include <windows.h>
#endif

class tst_QThread : public QObject
{
    Q_OBJECT
private slots:
    void currentThreadId();
    void currentThread();
    void idealThreadCount();
    void isFinished();
    void isRunning();
    void setPriority();
    void setStackSize();
    void exit();
    void start();
    void terminate();
    void quit();
    void started();
    void finished();
    void terminated(); // Named after a signal that was removed in Qt 5.0
    void exec();
    void sleep();
    void msleep();
    void usleep();

    void nativeThreadAdoption();
    void adoptedThreadAffinity();
    void adoptedThreadSetPriority();
    void adoptedThreadExit();
    void adoptedThreadExec();
    void adoptedThreadFinished();
    void adoptedThreadExecFinished();
    void adoptMultipleThreads();
    void adoptMultipleThreadsOverlap();

    void exitAndStart();
    void exitAndExec();

    void connectThreadFinishedSignalToObjectDeleteLaterSlot();
    void wait2();
    void wait3_slowDestructor();
    void destroyFinishRace();
    void startFinishRace();
    void startAndQuitCustomEventLoop();
    void isRunningInFinished();

    void customEventDispatcher();

#ifndef Q_OS_WINCE
    void stressTest();
#endif

    void quitLock();
};

enum { one_minute = 60 * 1000, five_minutes = 5 * one_minute };

class SignalRecorder : public QObject
{
    Q_OBJECT
public:
    QAtomicInt activationCount;

    inline SignalRecorder()
        : activationCount(0)
    { }

    bool wasActivated()
    { return activationCount.load() > 0; }

public slots:
    void slot();
};

void SignalRecorder::slot()
{ activationCount.ref(); }

class Current_Thread : public QThread
{
public:
    Qt::HANDLE id;
    QThread *thread;

    void run()
    {
        id = QThread::currentThreadId();
        thread = QThread::currentThread();
    }
};

class Simple_Thread : public QThread
{
public:
    QMutex mutex;
    QWaitCondition cond;

    void run()
    {
        QMutexLocker locker(&mutex);
        cond.wakeOne();
    }
};

class Exit_Object : public QObject
{
    Q_OBJECT
public:
    QThread *thread;
    int code;
public slots:
    void slot()
    { thread->exit(code); }
};

class Exit_Thread : public Simple_Thread
{
public:
    Exit_Object *object;
    int code;
    int result;

    void run()
    {
        Simple_Thread::run();
        if (object) {
            object->thread = this;
            object->code = code;
            QTimer::singleShot(100, object, SLOT(slot()));
        }
        result = exec();
    }
};

class Terminate_Thread : public Simple_Thread
{
public:
    void run()
    {
        setTerminationEnabled(false);
        {
            QMutexLocker locker(&mutex);
            cond.wakeOne();
            cond.wait(&mutex, five_minutes);
        }
        setTerminationEnabled(true);
        qFatal("tst_QThread: test case hung");
    }
};

class Quit_Object : public QObject
{
    Q_OBJECT
public:
    QThread *thread;
public slots:
    void slot()
    { thread->quit(); }
};

class Quit_Thread : public Simple_Thread
{
public:
    Quit_Object *object;
    int result;

    void run()
    {
        Simple_Thread::run();
        if (object) {
            object->thread = this;
            QTimer::singleShot(100, object, SLOT(slot()));
        }
        result = exec();
    }
};

class Sleep_Thread : public Simple_Thread
{
public:
    enum SleepType { Second, Millisecond, Microsecond };

    SleepType sleepType;
    int interval;

    int elapsed; // result, in *MILLISECONDS*

    void run()
    {
        QMutexLocker locker(&mutex);

        elapsed = 0;
        QTime time;
        time.start();
        switch (sleepType) {
        case Second:
            sleep(interval);
            break;
        case Millisecond:
            msleep(interval);
            break;
        case Microsecond:
            usleep(interval);
            break;
        }
        elapsed = time.elapsed();

        cond.wakeOne();
    }
};

void tst_QThread::currentThreadId()
{
    Current_Thread thread;
    thread.id = 0;
    thread.thread = 0;
    thread.start();
    QVERIFY(thread.wait(five_minutes));
    QVERIFY(thread.id != 0);
    QVERIFY(thread.id != QThread::currentThreadId());
}

void tst_QThread::currentThread()
{
    QVERIFY(QThread::currentThread() != 0);
    QCOMPARE(QThread::currentThread(), thread());

    Current_Thread thread;
    thread.id = 0;
    thread.thread = 0;
    thread.start();
    QVERIFY(thread.wait(five_minutes));
    QCOMPARE(thread.thread, (QThread *)&thread);
}

void tst_QThread::idealThreadCount()
{
    QVERIFY(QThread::idealThreadCount() > 0);
    qDebug() << "Ideal thread count:" << QThread::idealThreadCount();
}

void tst_QThread::isFinished()
{
    Simple_Thread thread;
    QVERIFY(!thread.isFinished());
    QMutexLocker locker(&thread.mutex);
    thread.start();
    QVERIFY(!thread.isFinished());
    thread.cond.wait(locker.mutex());
    QVERIFY(thread.wait(five_minutes));
    QVERIFY(thread.isFinished());
}

void tst_QThread::isRunning()
{
    Simple_Thread thread;
    QVERIFY(!thread.isRunning());
    QMutexLocker locker(&thread.mutex);
    thread.start();
    QVERIFY(thread.isRunning());
    thread.cond.wait(locker.mutex());
    QVERIFY(thread.wait(five_minutes));
    QVERIFY(!thread.isRunning());
}

void tst_QThread::setPriority()
{
    Simple_Thread thread;

    // cannot change the priority, since the thread is not running
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::IdlePriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::LowestPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::LowPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::NormalPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::HighPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::HighestPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::TimeCriticalPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);

    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QMutexLocker locker(&thread.mutex);
    thread.start();

    // change the priority of a running thread
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    thread.setPriority(QThread::IdlePriority);
    QCOMPARE(thread.priority(), QThread::IdlePriority);
    thread.setPriority(QThread::LowestPriority);
    QCOMPARE(thread.priority(), QThread::LowestPriority);
    thread.setPriority(QThread::LowPriority);
    QCOMPARE(thread.priority(), QThread::LowPriority);
    thread.setPriority(QThread::NormalPriority);
    QCOMPARE(thread.priority(), QThread::NormalPriority);
    thread.setPriority(QThread::HighPriority);
    QCOMPARE(thread.priority(), QThread::HighPriority);
    thread.setPriority(QThread::HighestPriority);
    QCOMPARE(thread.priority(), QThread::HighestPriority);
    thread.setPriority(QThread::TimeCriticalPriority);
    QCOMPARE(thread.priority(), QThread::TimeCriticalPriority);
    thread.cond.wait(locker.mutex());
    QVERIFY(thread.wait(five_minutes));

    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::IdlePriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::LowestPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::LowPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::NormalPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::HighPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::HighestPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
    QTest::ignoreMessage(QtWarningMsg, "QThread::setPriority: Cannot set priority, thread is not running");
    thread.setPriority(QThread::TimeCriticalPriority);
    QCOMPARE(thread.priority(), QThread::InheritPriority);
}

void tst_QThread::setStackSize()
{
    Simple_Thread thread;
    QCOMPARE(thread.stackSize(), 0u);
    thread.setStackSize(8192u);
    QCOMPARE(thread.stackSize(), 8192u);
    thread.setStackSize(0u);
    QCOMPARE(thread.stackSize(), 0u);
}

void tst_QThread::exit()
{
    Exit_Thread thread;
    thread.object = new Exit_Object;
    thread.object->moveToThread(&thread);
    thread.code = 42;
    thread.result = 0;
    QVERIFY(!thread.isFinished());
    QVERIFY(!thread.isRunning());
    QMutexLocker locker(&thread.mutex);
    thread.start();
    QVERIFY(thread.isRunning());
    QVERIFY(!thread.isFinished());
    thread.cond.wait(locker.mutex());
    QVERIFY(thread.wait(five_minutes));
    QVERIFY(thread.isFinished());
    QVERIFY(!thread.isRunning());
    QCOMPARE(thread.result, thread.code);
    delete thread.object;

    Exit_Thread thread2;
    thread2.object = 0;
    thread2.code = 53;
    thread2.result = 0;
    QMutexLocker locker2(&thread2.mutex);
    thread2.start();
    thread2.exit(thread2.code);
    thread2.cond.wait(locker2.mutex());
    QVERIFY(thread2.wait(five_minutes));
    QCOMPARE(thread2.result, thread2.code);
}

void tst_QThread::start()
{
    QThread::Priority priorities[] = {
        QThread::IdlePriority,
        QThread::LowestPriority,
        QThread::LowPriority,
        QThread::NormalPriority,
        QThread::HighPriority,
        QThread::HighestPriority,
        QThread::TimeCriticalPriority,
        QThread::InheritPriority
    };
    const int prio_count = sizeof(priorities) / sizeof(QThread::Priority);

    for (int i = 0; i < prio_count; ++i) {
        Simple_Thread thread;
        QVERIFY(!thread.isFinished());
        QVERIFY(!thread.isRunning());
        QMutexLocker locker(&thread.mutex);
        thread.start(priorities[i]);
        QVERIFY(thread.isRunning());
        QVERIFY(!thread.isFinished());
        thread.cond.wait(locker.mutex());
        QVERIFY(thread.wait(five_minutes));
        QVERIFY(thread.isFinished());
        QVERIFY(!thread.isRunning());
    }
}

void tst_QThread::terminate()
{
    Terminate_Thread thread;
    {
        QMutexLocker locker(&thread.mutex);
        thread.start();
        QVERIFY(thread.cond.wait(locker.mutex(), five_minutes));
        thread.terminate();
        thread.cond.wakeOne();
    }
    QVERIFY(thread.wait(five_minutes));
}

void tst_QThread::quit()
{
    Quit_Thread thread;
    thread.object = new Quit_Object;
    thread.object->moveToThread(&thread);
    thread.result = -1;
    QVERIFY(!thread.isFinished());
    QVERIFY(!thread.isRunning());
    QMutexLocker locker(&thread.mutex);
    thread.start();
    QVERIFY(thread.isRunning());
    QVERIFY(!thread.isFinished());
    thread.cond.wait(locker.mutex());
    QVERIFY(thread.wait(five_minutes));
    QVERIFY(thread.isFinished());
    QVERIFY(!thread.isRunning());
    QCOMPARE(thread.result, 0);
    delete thread.object;

    Quit_Thread thread2;
    thread2.object = 0;
    thread2.result = -1;
    QMutexLocker locker2(&thread2.mutex);
    thread2.start();
    thread2.quit();
    thread2.cond.wait(locker2.mutex());
    QVERIFY(thread2.wait(five_minutes));
    QCOMPARE(thread2.result, 0);
}

void tst_QThread::started()
{
    SignalRecorder recorder;
    Simple_Thread thread;
    connect(&thread, SIGNAL(started()), &recorder, SLOT(slot()), Qt::DirectConnection);
    thread.start();
    QVERIFY(thread.wait(five_minutes));
    QVERIFY(recorder.wasActivated());
}

void tst_QThread::finished()
{
    SignalRecorder recorder;
    Simple_Thread thread;
    connect(&thread, SIGNAL(finished()), &recorder, SLOT(slot()), Qt::DirectConnection);
    thread.start();
    QVERIFY(thread.wait(five_minutes));
    QVERIFY(recorder.wasActivated());
}

void tst_QThread::terminated()
{
    SignalRecorder recorder;
    Terminate_Thread thread;
    connect(&thread, SIGNAL(finished()), &recorder, SLOT(slot()), Qt::DirectConnection);
    {
        QMutexLocker locker(&thread.mutex);
        thread.start();
        thread.cond.wait(locker.mutex());
        thread.terminate();
        thread.cond.wakeOne();
    }
    QVERIFY(thread.wait(five_minutes));
    QVERIFY(recorder.wasActivated());
}

void tst_QThread::exec()
{
    class MultipleExecThread : public QThread
    {
    public:
        int res1, res2;

        MultipleExecThread() : res1(-2), res2(-2) { }

        void run()
        {
            {
                Exit_Object o;
                o.thread = this;
                o.code = 1;
                QTimer::singleShot(100, &o, SLOT(slot()));
                res1 = exec();
            }
            {
                Exit_Object o;
                o.thread = this;
                o.code = 2;
                QTimer::singleShot(100, &o, SLOT(slot()));
                res2 = exec();
            }
        }
    };

    MultipleExecThread thread;
    thread.start();
    QVERIFY(thread.wait());

    QCOMPARE(thread.res1, 1);
    QCOMPARE(thread.res2, 2);
}

void tst_QThread::sleep()
{
    Sleep_Thread thread;
    thread.sleepType = Sleep_Thread::Second;
    thread.interval = 2;
    thread.start();
    QVERIFY(thread.wait(five_minutes));
    QVERIFY(thread.elapsed >= 2000);
}

void tst_QThread::msleep()
{
    Sleep_Thread thread;
    thread.sleepType = Sleep_Thread::Millisecond;
    thread.interval = 120;
    thread.start();
    QVERIFY(thread.wait(five_minutes));
#if defined (Q_OS_WIN)
    // Since the resolution of QTime is so coarse...
    QVERIFY(thread.elapsed >= 100);
#else
    QVERIFY(thread.elapsed >= 120);
#endif
}

void tst_QThread::usleep()
{
    Sleep_Thread thread;
    thread.sleepType = Sleep_Thread::Microsecond;
    thread.interval = 120000;
    thread.start();
    QVERIFY(thread.wait(five_minutes));
#if defined (Q_OS_WIN)
    // Since the resolution of QTime is so coarse...
    QVERIFY(thread.elapsed >= 100);
#else
    QVERIFY(thread.elapsed >= 120);
#endif
}

typedef void (*FunctionPointer)(void *);
void noop(void*) { }

#if defined Q_OS_UNIX
    typedef pthread_t ThreadHandle;
#elif defined Q_OS_WIN
    typedef HANDLE ThreadHandle;
#endif

#ifdef Q_OS_WIN
#define WIN_FIX_STDCALL __stdcall
#else
#define WIN_FIX_STDCALL
#endif

class NativeThreadWrapper
{
public:
    NativeThreadWrapper() : qthread(0), waitForStop(false) {}
    void start(FunctionPointer functionPointer = noop, void *data = 0);
    void startAndWait(FunctionPointer functionPointer = noop, void *data = 0);
    void join();
    void setWaitForStop() { waitForStop = true; }
    void stop();

    ThreadHandle nativeThreadHandle;
    QThread *qthread;
    QWaitCondition startCondition;
    QMutex mutex;
    bool waitForStop;
    QWaitCondition stopCondition;
protected:
    static void *runUnix(void *data);
    static unsigned WIN_FIX_STDCALL runWin(void *data);

    FunctionPointer functionPointer;
    void *data;
};

void NativeThreadWrapper::start(FunctionPointer functionPointer, void *data)
{
    this->functionPointer = functionPointer;
    this->data = data;
#if defined Q_OS_UNIX
    const int state = pthread_create(&nativeThreadHandle, 0, NativeThreadWrapper::runUnix, this);
    Q_UNUSED(state);
#elif defined(Q_OS_WINCE)
        nativeThreadHandle = CreateThread(NULL, 0 , (LPTHREAD_START_ROUTINE)NativeThreadWrapper::runWin , this, 0, NULL);
#elif defined Q_OS_WIN
    unsigned thrdid = 0;
    nativeThreadHandle = (Qt::HANDLE) _beginthreadex(NULL, 0, NativeThreadWrapper::runWin, this, 0, &thrdid);
#endif
}

void NativeThreadWrapper::startAndWait(FunctionPointer functionPointer, void *data)
{
    QMutexLocker locker(&mutex);
    start(functionPointer, data);
    startCondition.wait(locker.mutex());
}

void NativeThreadWrapper::join()
{
#if defined Q_OS_UNIX
    pthread_join(nativeThreadHandle, 0);
#elif defined Q_OS_WIN
    WaitForSingleObject(nativeThreadHandle, INFINITE);
    CloseHandle(nativeThreadHandle);
#endif
}

void *NativeThreadWrapper::runUnix(void *that)
{
    NativeThreadWrapper *nativeThreadWrapper = reinterpret_cast<NativeThreadWrapper*>(that);

    // Adopt thread, create QThread object.
    nativeThreadWrapper->qthread = QThread::currentThread();

    // Release main thread.
    {
        QMutexLocker lock(&nativeThreadWrapper->mutex);
        nativeThreadWrapper->startCondition.wakeOne();
    }

    // Run function.
    nativeThreadWrapper->functionPointer(nativeThreadWrapper->data);

    // Wait for stop.
    {
        QMutexLocker lock(&nativeThreadWrapper->mutex);
        if (nativeThreadWrapper->waitForStop)
            nativeThreadWrapper->stopCondition.wait(lock.mutex());
    }

    return 0;
}

unsigned WIN_FIX_STDCALL NativeThreadWrapper::runWin(void *data)
{
    runUnix(data);
    return 0;
}

void NativeThreadWrapper::stop()
{
    QMutexLocker lock(&mutex);
    waitForStop = false;
    stopCondition.wakeOne();
}

bool threadAdoptedOk = false;
QThread *mainThread;
void testNativeThreadAdoption(void *)
{
    threadAdoptedOk = (QThread::currentThreadId() != 0
                       && QThread::currentThread() != 0
                       && QThread::currentThread() != mainThread);
}
void tst_QThread::nativeThreadAdoption()
{
    threadAdoptedOk = false;
    mainThread = QThread::currentThread();
    NativeThreadWrapper nativeThread;
    nativeThread.setWaitForStop();
    nativeThread.startAndWait(testNativeThreadAdoption);
    QVERIFY(nativeThread.qthread);

    nativeThread.stop();
    nativeThread.join();

    QVERIFY(threadAdoptedOk);
}

void adoptedThreadAffinityFunction(void *arg)
{
    QThread **affinity = reinterpret_cast<QThread **>(arg);
    QThread *current = QThread::currentThread();
    affinity[0] = current;
    affinity[1] = current->thread();
}

void tst_QThread::adoptedThreadAffinity()
{
    QThread *affinity[2] = { 0, 0 };

    NativeThreadWrapper thread;
    thread.startAndWait(adoptedThreadAffinityFunction, affinity);
    thread.join();

    // adopted thread should have affinity to itself
    QCOMPARE(affinity[0], affinity[1]);
}

void tst_QThread::adoptedThreadSetPriority()
{

    NativeThreadWrapper nativeThread;
    nativeThread.setWaitForStop();
    nativeThread.startAndWait();

    // change the priority of a running thread
    QCOMPARE(nativeThread.qthread->priority(), QThread::InheritPriority);
    nativeThread.qthread->setPriority(QThread::IdlePriority);
    QCOMPARE(nativeThread.qthread->priority(), QThread::IdlePriority);
    nativeThread.qthread->setPriority(QThread::LowestPriority);
    QCOMPARE(nativeThread.qthread->priority(), QThread::LowestPriority);
    nativeThread.qthread->setPriority(QThread::LowPriority);
    QCOMPARE(nativeThread.qthread->priority(), QThread::LowPriority);
    nativeThread.qthread->setPriority(QThread::NormalPriority);
    QCOMPARE(nativeThread.qthread->priority(), QThread::NormalPriority);
    nativeThread.qthread->setPriority(QThread::HighPriority);
    QCOMPARE(nativeThread.qthread->priority(), QThread::HighPriority);
    nativeThread.qthread->setPriority(QThread::HighestPriority);
    QCOMPARE(nativeThread.qthread->priority(), QThread::HighestPriority);
    nativeThread.qthread->setPriority(QThread::TimeCriticalPriority);
    QCOMPARE(nativeThread.qthread->priority(), QThread::TimeCriticalPriority);

    nativeThread.stop();
    nativeThread.join();
}

void tst_QThread::adoptedThreadExit()
{
    NativeThreadWrapper nativeThread;
    nativeThread.setWaitForStop();

    nativeThread.startAndWait();
    QVERIFY(nativeThread.qthread);
    QVERIFY(nativeThread.qthread->isRunning());
    QVERIFY(!nativeThread.qthread->isFinished());

    nativeThread.stop();
    nativeThread.join();
}

void adoptedThreadExecFunction(void *)
{
    QThread  * const adoptedThread = QThread::currentThread();
    QEventLoop eventLoop(adoptedThread);

    const int code = 1;
    Exit_Object o;
    o.thread = adoptedThread;
    o.code = code;
    QTimer::singleShot(100, &o, SLOT(slot()));

    const int result = eventLoop.exec();
    QCOMPARE(result, code);
}

void tst_QThread::adoptedThreadExec()
{
    NativeThreadWrapper nativeThread;
    nativeThread.start(adoptedThreadExecFunction);
    nativeThread.join();
}

/*
    Test that you get the finished signal when an adopted thread exits.
*/
void tst_QThread::adoptedThreadFinished()
{
    NativeThreadWrapper nativeThread;
    nativeThread.setWaitForStop();
    nativeThread.startAndWait();

    QObject::connect(nativeThread.qthread, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    nativeThread.stop();
    nativeThread.join();

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

void tst_QThread::adoptedThreadExecFinished()
{
    NativeThreadWrapper nativeThread;
    nativeThread.setWaitForStop();
    nativeThread.startAndWait(adoptedThreadExecFunction);

    QObject::connect(nativeThread.qthread, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    nativeThread.stop();
    nativeThread.join();

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

void tst_QThread::adoptMultipleThreads()
{
#if defined(Q_OS_WIN)
    // Windows CE is not capable of handling that many threads. On the emulator it is dead with 26 threads already.
#  if defined(Q_OS_WINCE)
    const int numThreads = 20;
#  else
    // need to test lots of threads, so that we exceed MAXIMUM_WAIT_OBJECTS in qt_adopted_thread_watcher()
    const int numThreads = 200;
#  endif
#else
    const int numThreads = 5;
#endif
    QVector<NativeThreadWrapper*> nativeThreads;

    SignalRecorder recorder;

    for (int i = 0; i < numThreads; ++i) {
        nativeThreads.append(new NativeThreadWrapper());
        nativeThreads.at(i)->setWaitForStop();
        nativeThreads.at(i)->startAndWait();
        QObject::connect(nativeThreads.at(i)->qthread, SIGNAL(finished()), &recorder, SLOT(slot()));
    }

    QObject::connect(nativeThreads.at(numThreads - 1)->qthread, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    for (int i = 0; i < numThreads; ++i) {
        nativeThreads.at(i)->stop();
        nativeThreads.at(i)->join();
        delete nativeThreads.at(i);
    }

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(recorder.activationCount.load(), numThreads);
}

void tst_QThread::adoptMultipleThreadsOverlap()
{
#if defined(Q_OS_WIN)
    // Windows CE is not capable of handling that many threads. On the emulator it is dead with 26 threads already.
#  if defined(Q_OS_WINCE)
    const int numThreads = 20;
#  else
    // need to test lots of threads, so that we exceed MAXIMUM_WAIT_OBJECTS in qt_adopted_thread_watcher()
    const int numThreads = 200;
#  endif
#else
    const int numThreads = 5;
#endif
    QVector<NativeThreadWrapper*> nativeThreads;

    SignalRecorder recorder;

    for (int i = 0; i < numThreads; ++i) {
        nativeThreads.append(new NativeThreadWrapper());
        nativeThreads.at(i)->setWaitForStop();
        nativeThreads.at(i)->mutex.lock();
        nativeThreads.at(i)->start();
    }
    for (int i = 0; i < numThreads; ++i) {
        nativeThreads.at(i)->startCondition.wait(&nativeThreads.at(i)->mutex);
        QObject::connect(nativeThreads.at(i)->qthread, SIGNAL(finished()), &recorder, SLOT(slot()));
        nativeThreads.at(i)->mutex.unlock();
    }

    QObject::connect(nativeThreads.at(numThreads - 1)->qthread, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    for (int i = 0; i < numThreads; ++i) {
        nativeThreads.at(i)->stop();
        nativeThreads.at(i)->join();
        delete nativeThreads.at(i);
    }

    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(recorder.activationCount.load(), numThreads);
}

#ifndef Q_OS_WINCE
// Disconnects on WinCE
void tst_QThread::stressTest()
{
    QTime t;
    t.start();
    while (t.elapsed() < one_minute) {
        Current_Thread t;
        t.start();
        t.wait(one_minute);
    }
}
#endif

class Syncronizer : public QObject
{ Q_OBJECT
public slots:
    void setProp(int p) {
        if(m_prop != p) {
            m_prop = p;
            emit propChanged(p);
        }
    }
signals:
    void propChanged(int);
public:
    Syncronizer() : m_prop(42) {}
    int m_prop;
};

void tst_QThread::exitAndStart()
{
    QThread thread;
    thread.exit(555); //should do nothing

    thread.start();

    //test that the thread is running by executing queued connected signal there
    Syncronizer sync1;
    sync1.moveToThread(&thread);
    Syncronizer sync2;
    sync2.moveToThread(&thread);
    connect(&sync2, SIGNAL(propChanged(int)), &sync1, SLOT(setProp(int)), Qt::QueuedConnection);
    connect(&sync1, SIGNAL(propChanged(int)), &thread, SLOT(quit()), Qt::QueuedConnection);
    QMetaObject::invokeMethod(&sync2, "setProp", Qt::QueuedConnection , Q_ARG(int, 89));
    QTest::qWait(50);
    while(!thread.wait(10))
        QTest::qWait(10);
    QCOMPARE(sync2.m_prop, 89);
    QCOMPARE(sync1.m_prop, 89);
}

void tst_QThread::exitAndExec()
{
    class Thread : public QThread {
    public:
        QSemaphore sem1;
        QSemaphore sem2;
        volatile int value;
        void run() {
            sem1.acquire();
            value = exec();  //First entrence
            sem2.release();
            value = exec(); // Second loop
        }
    };
    Thread thread;
    thread.value = 0;
    thread.start();
    thread.exit(556);
    thread.sem1.release(); //should exit the first loop
    thread.sem2.acquire();
    int v = thread.value;
    QCOMPARE(v, 556);

    //test that the thread is running by executing queued connected signal there
    Syncronizer sync1;
    sync1.moveToThread(&thread);
    Syncronizer sync2;
    sync2.moveToThread(&thread);
    connect(&sync2, SIGNAL(propChanged(int)), &sync1, SLOT(setProp(int)), Qt::QueuedConnection);
    connect(&sync1, SIGNAL(propChanged(int)), &thread, SLOT(quit()), Qt::QueuedConnection);
    QMetaObject::invokeMethod(&sync2, "setProp", Qt::QueuedConnection , Q_ARG(int, 89));
    QTest::qWait(50);
    while(!thread.wait(10))
        QTest::qWait(10);
    QCOMPARE(sync2.m_prop, 89);
    QCOMPARE(sync1.m_prop, 89);
}

void tst_QThread::connectThreadFinishedSignalToObjectDeleteLaterSlot()
{
    QThread thread;
    QObject *object = new QObject;
    QPointer<QObject> p = object;
    QVERIFY(!p.isNull());
    connect(&thread, SIGNAL(started()), &thread, SLOT(quit()), Qt::DirectConnection);
    connect(&thread, SIGNAL(finished()), object, SLOT(deleteLater()));
    object->moveToThread(&thread);
    thread.start();
    QVERIFY(thread.wait(30000));
    QVERIFY(p.isNull());
}

class Waiting_Thread : public QThread
{
public:
    enum { WaitTime = 800 };
    QMutex mutex;
    QWaitCondition cond1;
    QWaitCondition cond2;

    void run()
    {
        QMutexLocker locker(&mutex);
        cond1.wait(&mutex);
        cond2.wait(&mutex, WaitTime);
    }
};

void tst_QThread::wait2()
{
    QElapsedTimer timer;
    Waiting_Thread thread;
    thread.start();
    timer.start();
    QVERIFY(!thread.wait(Waiting_Thread::WaitTime));
    qint64 elapsed = timer.elapsed(); // On Windows, we sometimes get (WaitTime - 1).
    QVERIFY2(elapsed >= Waiting_Thread::WaitTime - 1, qPrintable(QString::fromLatin1("elapsed: %1").arg(elapsed)));

    timer.start();
    thread.cond1.wakeOne();
    QVERIFY(thread.wait(/*Waiting_Thread::WaitTime * 1.4*/));
    elapsed = timer.elapsed();
    QVERIFY2(elapsed - Waiting_Thread::WaitTime >= -1, qPrintable(QString::fromLatin1("elapsed: %1").arg(elapsed)));
}


class SlowSlotObject : public QObject {
    Q_OBJECT
public:
    QMutex mutex;
    QWaitCondition cond;
public slots:
    void slowSlot() {
        QMutexLocker locker(&mutex);
        cond.wait(&mutex);
    }
};

void tst_QThread::wait3_slowDestructor()
{
    SlowSlotObject slow;
    QThread thread;
    QObject::connect(&thread, SIGNAL(finished()), &slow, SLOT(slowSlot()), Qt::DirectConnection);

    enum { WaitTime = 1800 };
    QElapsedTimer timer;

    thread.start();
    thread.quit();
    //the quit function will cause the thread to finish and enter the slowSlot that is blocking

    timer.start();
    QVERIFY(!thread.wait(Waiting_Thread::WaitTime));
    qint64 elapsed = timer.elapsed();
    QVERIFY2(elapsed >= Waiting_Thread::WaitTime - 1, qPrintable(QString::fromLatin1("elapsed: %1").arg(elapsed)));

    slow.cond.wakeOne();
    //now the thread should finish quickly
    QVERIFY(thread.wait(one_minute));
}

void tst_QThread::destroyFinishRace()
{
    class Thread : public QThread { void run() {} };
    for (int i = 0; i < 15; i++) {
        Thread *thr = new Thread;
        connect(thr, SIGNAL(finished()), thr, SLOT(deleteLater()));
        QPointer<QThread> weak(static_cast<QThread*>(thr));
        thr->start();
        while (weak) {
            qApp->processEvents();
            qApp->processEvents();
            qApp->processEvents();
            qApp->processEvents();
        }
    }
}

void tst_QThread::startFinishRace()
{
    class Thread : public QThread {
    public:
        Thread() : i (50) {}
        void run() {
            i--;
            if (!i) disconnect(this, SIGNAL(finished()), 0, 0);
        }
        int i;
    };
    for (int i = 0; i < 15; i++) {
        Thread thr;
        connect(&thr, SIGNAL(finished()), &thr, SLOT(start()));
        thr.start();
        while (!thr.isFinished() || thr.i != 0) {
            qApp->processEvents();
            qApp->processEvents();
            qApp->processEvents();
            qApp->processEvents();
        }
        QCOMPARE(thr.i, 0);
    }
}

void tst_QThread::startAndQuitCustomEventLoop()
{
    struct Thread : QThread {
        void run() { QEventLoop().exec(); }
    };

   for (int i = 0; i < 5; i++) {
       Thread t;
       t.start();
       t.quit();
       t.wait();
   }
}

class FinishedTestObject : public QObject {
    Q_OBJECT
public:
    FinishedTestObject() : ok(false) {}
    bool ok;
public slots:
    void slotFinished() {
        QThread *t = qobject_cast<QThread *>(sender());
        ok = t && t->isFinished() && !t->isRunning();
    }
};

void tst_QThread::isRunningInFinished()
{
    for (int i = 0; i < 15; i++) {
        QThread thread;
        thread.start();
        FinishedTestObject localObject;
        FinishedTestObject inThreadObject;
        localObject.setObjectName("...");
        inThreadObject.moveToThread(&thread);
        connect(&thread, SIGNAL(finished()), &localObject, SLOT(slotFinished()));
        connect(&thread, SIGNAL(finished()), &inThreadObject, SLOT(slotFinished()));
        QEventLoop loop;
        connect(&thread, SIGNAL(finished()), &loop, SLOT(quit()));
        QMetaObject::invokeMethod(&thread, "quit", Qt::QueuedConnection);
        loop.exec();
        QVERIFY(!thread.isRunning());
        QVERIFY(thread.isFinished());
        QVERIFY(localObject.ok);
        QVERIFY(inThreadObject.ok);
    }
}

QT_BEGIN_NAMESPACE
Q_CORE_EXPORT uint qGlobalPostedEventsCount();
QT_END_NAMESPACE

class DummyEventDispatcher : public QAbstractEventDispatcher {
public:
    DummyEventDispatcher() : QAbstractEventDispatcher() {}
    bool processEvents(QEventLoop::ProcessEventsFlags) {
        visited.store(true);
        emit awake();
        QCoreApplication::sendPostedEvents();
        return false;
    }
    bool hasPendingEvents() {
        return qGlobalPostedEventsCount();
    }
    void registerSocketNotifier(QSocketNotifier *) {}
    void unregisterSocketNotifier(QSocketNotifier *) {}
    void registerTimer(int, int, Qt::TimerType, QObject *) {}
    bool unregisterTimer(int ) { return false; }
    bool unregisterTimers(QObject *) { return false; }
    QList<TimerInfo> registeredTimers(QObject *) const { return QList<TimerInfo>(); }
    int remainingTime(int) { return 0; }
    void wakeUp() {}
    void interrupt() {}
    void flush() {}

#ifdef Q_OS_WIN
    bool registerEventNotifier(QWinEventNotifier *) { return false; }
    void unregisterEventNotifier(QWinEventNotifier *) { }
#endif

    QBasicAtomicInt visited; // bool
};

class ThreadObj : public QObject
{
    Q_OBJECT
public slots:
    void visit() {
        emit visited();
    }
signals:
    void visited();
};

void tst_QThread::customEventDispatcher()
{
    QThread thr;
    // there should be no ED yet
    QVERIFY(!thr.eventDispatcher());
    DummyEventDispatcher *ed = new DummyEventDispatcher;
    thr.setEventDispatcher(ed);
    // the new ED should be set
    QCOMPARE(thr.eventDispatcher(), ed);
    // test the alternative API of QAbstractEventDispatcher
    QCOMPARE(QAbstractEventDispatcher::instance(&thr), ed);
    thr.start();
    // start() should not overwrite the ED
    QCOMPARE(thr.eventDispatcher(), ed);

    ThreadObj obj;
    obj.moveToThread(&thr);
    // move was successful?
    QCOMPARE(obj.thread(), &thr);
    QEventLoop loop;
    connect(&obj, SIGNAL(visited()), &loop, SLOT(quit()), Qt::QueuedConnection);
    QMetaObject::invokeMethod(&obj, "visit", Qt::QueuedConnection);
    loop.exec();
    // test that the ED has really been used
    QVERIFY(ed->visited.load());

    QPointer<DummyEventDispatcher> weak_ed(ed);
    QVERIFY(!weak_ed.isNull());
    thr.quit();
    // wait for thread to be stopped
    QVERIFY(thr.wait(30000));
    // test that ED has been deleted
    QVERIFY(weak_ed.isNull());
}

class Job : public QObject
{
    Q_OBJECT
public:
    Job(QThread *thread, int deleteDelay, bool *flag, QObject *parent = 0)
      : QObject(parent), quitLocker(thread), exitThreadCalled(*flag)
    {
        exitThreadCalled = false;
        moveToThread(thread);
        QTimer::singleShot(deleteDelay, this, SLOT(deleteLater()));
        QTimer::singleShot(1000, this, SLOT(exitThread()));
    }

private slots:
    void exitThread()
    {
        exitThreadCalled = true;
        thread()->exit(1);
    }

private:
    QEventLoopLocker quitLocker;
public:
    bool &exitThreadCalled;
};

void tst_QThread::quitLock()
{
    QThread thread;
    bool exitThreadCalled;

    QEventLoop loop;
    connect(&thread, SIGNAL(finished()), &loop, SLOT(quit()));

    Job *job;

    thread.start();
    job = new Job(&thread, 500, &exitThreadCalled);
    QCOMPARE(job->thread(), &thread);
    loop.exec();
    QVERIFY(!exitThreadCalled);

    thread.start();
    job = new Job(&thread, 2500, &exitThreadCalled);
    QCOMPARE(job->thread(), &thread);
    loop.exec();
    QVERIFY(exitThreadCalled);
}

QTEST_MAIN(tst_QThread)
#include "tst_qthread.moc"
