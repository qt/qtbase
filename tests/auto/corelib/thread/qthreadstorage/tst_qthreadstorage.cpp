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
#include <qmutex.h>
#include <qthread.h>
#include <qwaitcondition.h>
#include <qthreadstorage.h>
#include <qdir.h>
#include <qfileinfo.h>

#ifdef Q_OS_UNIX
#include <pthread.h>
#endif
#ifdef Q_OS_WIN
#  include <process.h>
#  include <qt_windows.h>
#endif

class tst_QThreadStorage : public QObject
{
    Q_OBJECT
private slots:
    void hasLocalData();
    void localData();
    void localData_const();
    void setLocalData();
    void autoDelete();
    void adoptedThreads();
    void ensureCleanupOrder();
    void crashOnExit();
    void leakInDestructor();
    void resetInDestructor();
    void valueBased();
};

class Pointer
{
public:
    static int count;
    inline Pointer() { ++count; }
    inline ~Pointer() { --count; }
};
int Pointer::count = 0;

void tst_QThreadStorage::hasLocalData()
{
    QThreadStorage<Pointer *> pointers;
    QVERIFY(!pointers.hasLocalData());
    pointers.setLocalData(new Pointer);
    QVERIFY(pointers.hasLocalData());
    pointers.setLocalData(0);
    QVERIFY(!pointers.hasLocalData());
}

void tst_QThreadStorage::localData()
{
    QThreadStorage<Pointer*> pointers;
    Pointer *p = new Pointer;
    QVERIFY(!pointers.hasLocalData());
    pointers.setLocalData(p);
    QVERIFY(pointers.hasLocalData());
    QCOMPARE(pointers.localData(), p);
    pointers.setLocalData(0);
    QCOMPARE(pointers.localData(), (Pointer *)0);
    QVERIFY(!pointers.hasLocalData());
}

void tst_QThreadStorage::localData_const()
{
    QThreadStorage<Pointer *> pointers;
    const QThreadStorage<Pointer *> &const_pointers = pointers;
    Pointer *p = new Pointer;
    QVERIFY(!pointers.hasLocalData());
    pointers.setLocalData(p);
    QVERIFY(pointers.hasLocalData());
    QCOMPARE(const_pointers.localData(), p);
    pointers.setLocalData(0);
    QCOMPARE(const_pointers.localData(), (Pointer *)0);
    QVERIFY(!pointers.hasLocalData());
}

void tst_QThreadStorage::setLocalData()
{
    QThreadStorage<Pointer *> pointers;
    QVERIFY(!pointers.hasLocalData());
    pointers.setLocalData(new Pointer);
    QVERIFY(pointers.hasLocalData());
    pointers.setLocalData(0);
    QVERIFY(!pointers.hasLocalData());
}

class Thread : public QThread
{
public:
    QThreadStorage<Pointer *> &pointers;

    QMutex mutex;
    QWaitCondition cond;

    Thread(QThreadStorage<Pointer *> &p)
        : pointers(p)
    { }

    void run()
    {
        pointers.setLocalData(new Pointer);

        QMutexLocker locker(&mutex);
        cond.wakeOne();
        cond.wait(&mutex);
    }
};

void tst_QThreadStorage::autoDelete()
{
    QThreadStorage<Pointer *> pointers;
    QVERIFY(!pointers.hasLocalData());

    Thread thread(pointers);
    int c = Pointer::count;
    {
        QMutexLocker locker(&thread.mutex);
        thread.start();
        thread.cond.wait(&thread.mutex);
        // QCOMPARE(Pointer::count, c + 1);
        thread.cond.wakeOne();
    }
    thread.wait();
    QCOMPARE(Pointer::count, c);
}

bool threadStorageOk;
void testAdoptedThreadStorageWin(void *p)
{
    QThreadStorage<Pointer *>  *pointers = reinterpret_cast<QThreadStorage<Pointer *> *>(p);
    if (pointers->hasLocalData()) {
        threadStorageOk = false;
        return;
    }

    Pointer *pointer = new Pointer();
    pointers->setLocalData(pointer);

    if (pointers->hasLocalData() == false) {
        threadStorageOk = false;
        return;
    }

    if (pointers->localData() != pointer) {
        threadStorageOk = false;
        return;
    }
    QObject::connect(QThread::currentThread(), SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));
}
#ifdef Q_OS_WINRT
unsigned __stdcall testAdoptedThreadStorageWinRT(void *p)
{
    testAdoptedThreadStorageWin(p);
    return 0;
}
#endif
void *testAdoptedThreadStorageUnix(void *pointers)
{
    testAdoptedThreadStorageWin(pointers);
    return 0;
}
void tst_QThreadStorage::adoptedThreads()
{
    QTestEventLoop::instance(); // Make sure the instance is created in this thread.
    QThreadStorage<Pointer *> pointers;
    int c = Pointer::count;
    threadStorageOk = true;
    {
#ifdef Q_OS_UNIX
        pthread_t thread;
        const int state = pthread_create(&thread, 0, testAdoptedThreadStorageUnix, &pointers);
        QCOMPARE(state, 0);
        pthread_join(thread, 0);
#elif defined Q_OS_WINRT
        HANDLE thread;
        thread = (HANDLE) _beginthreadex(NULL, 0, testAdoptedThreadStorageWinRT, &pointers, 0, 0);
        QVERIFY(thread);
        WaitForSingleObjectEx(thread, INFINITE, FALSE);
#elif defined Q_OS_WIN
        HANDLE thread;
        thread = (HANDLE)_beginthread(testAdoptedThreadStorageWin, 0, &pointers);
        QVERIFY(thread);
        WaitForSingleObject(thread, INFINITE);
#endif
    }
    QVERIFY(threadStorageOk);

    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QTRY_COMPARE(Pointer::count, c);
}

QBasicAtomicInt cleanupOrder = Q_BASIC_ATOMIC_INITIALIZER(0);

class First
{
public:
    ~First()
    {
        order = cleanupOrder.fetchAndAddRelaxed(1);
    }
    static int order;
};
int First::order = -1;

class Second
{
public:
    ~Second()
    {
        order = cleanupOrder.fetchAndAddRelaxed(1);
    }
    static int order;
};
int Second::order = -1;

void tst_QThreadStorage::ensureCleanupOrder()
{
    class Thread : public QThread
    {
    public:
        QThreadStorage<First *> &first;
        QThreadStorage<Second *> &second;

        Thread(QThreadStorage<First *> &first,
               QThreadStorage<Second *> &second)
            : first(first), second(second)
        { }

        void run()
        {
            // set in reverse order, but shouldn't matter, the data
            // will be deleted in the order the thread storage objects
            // were created
            second.setLocalData(new Second);
            first.setLocalData(new First);
        }
    };

    QThreadStorage<Second *> second;
    QThreadStorage<First *> first;
    Thread thread(first, second);
    thread.start();
    thread.wait();

    QVERIFY(First::order < Second::order);
}

#if QT_CONFIG(process)
static inline bool runCrashOnExit(const QString &binary, QString *errorMessage)
{
    const int timeout = 60000;
    QProcess process;
    process.start(binary);
    if (!process.waitForStarted()) {
        *errorMessage = QString::fromLatin1("Could not start '%1': %2").arg(binary, process.errorString());
        return false;
    }
    if (!process.waitForFinished(timeout)) {
        process.kill();
        *errorMessage = QString::fromLatin1("Timeout (%1ms) waiting for %2.").arg(timeout).arg(binary);
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        *errorMessage = binary + QStringLiteral(" crashed.");
        return false;
    }
    return true;
}
#endif

void tst_QThreadStorage::crashOnExit()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
    QString errorMessage;
    QVERIFY2(runCrashOnExit("crashOnExit_helper", &errorMessage),
             qPrintable(errorMessage));
#endif
}

// S stands for thread Safe.
class SPointer
{
public:
    static QBasicAtomicInt count;
    inline SPointer() { count.ref(); }
    inline ~SPointer() { count.deref(); }
    inline SPointer(const SPointer & /* other */) { count.ref(); }
};
QBasicAtomicInt SPointer::count = Q_BASIC_ATOMIC_INITIALIZER(0);

Q_GLOBAL_STATIC(QThreadStorage<SPointer *>, threadStoragePointers1)
Q_GLOBAL_STATIC(QThreadStorage<SPointer *>, threadStoragePointers2)

class ThreadStorageLocalDataTester
{
public:
    SPointer member;
    inline ~ThreadStorageLocalDataTester() {
        QVERIFY(!threadStoragePointers1()->hasLocalData());
        QVERIFY(!threadStoragePointers2()->hasLocalData());
        threadStoragePointers2()->setLocalData(new SPointer);
        threadStoragePointers1()->setLocalData(new SPointer);
        QVERIFY(threadStoragePointers1()->hasLocalData());
        QVERIFY(threadStoragePointers2()->hasLocalData());
    }
};


void tst_QThreadStorage::leakInDestructor()
{
    class Thread : public QThread
    {
    public:
        QThreadStorage<ThreadStorageLocalDataTester *> &tls;

        Thread(QThreadStorage<ThreadStorageLocalDataTester *> &t) : tls(t) { }

        void run()
        {
            QVERIFY(!tls.hasLocalData());
            tls.setLocalData(new ThreadStorageLocalDataTester);
            QVERIFY(tls.hasLocalData());
        }
    };
    int c = SPointer::count.loadRelaxed();

    QThreadStorage<ThreadStorageLocalDataTester *> tls;

    QVERIFY(!threadStoragePointers1()->hasLocalData());
    QThreadStorage<int *> tls2; //add some more tls to make sure ids are not following each other too much
    QThreadStorage<int *> tls3;
    QVERIFY(!tls2.hasLocalData());
    QVERIFY(!tls3.hasLocalData());
    QVERIFY(!tls.hasLocalData());

    Thread t1(tls);
    Thread t2(tls);
    Thread t3(tls);

    t1.start();
    t2.start();
    t3.start();

    QVERIFY(t1.wait());
    QVERIFY(t2.wait());
    QVERIFY(t3.wait());

    //check all the constructed things have been destructed
    QCOMPARE(int(SPointer::count.loadRelaxed()), c);
}

class ThreadStorageResetLocalDataTester {
public:
    SPointer member;
    ~ThreadStorageResetLocalDataTester();
};

Q_GLOBAL_STATIC(QThreadStorage<ThreadStorageResetLocalDataTester *>, ThreadStorageResetLocalDataTesterTls)

ThreadStorageResetLocalDataTester::~ThreadStorageResetLocalDataTester() {
    //Quite stupid, but WTF::ThreadSpecific<T>::destroy does it.
    ThreadStorageResetLocalDataTesterTls()->setLocalData(this);
}

void tst_QThreadStorage::resetInDestructor()
{
    class Thread : public QThread
    {
    public:
        void run()
        {
            QVERIFY(!ThreadStorageResetLocalDataTesterTls()->hasLocalData());
            ThreadStorageResetLocalDataTesterTls()->setLocalData(new ThreadStorageResetLocalDataTester);
            QVERIFY(ThreadStorageResetLocalDataTesterTls()->hasLocalData());
        }
    };
    int c = SPointer::count.loadRelaxed();

    Thread t1;
    Thread t2;
    Thread t3;
    t1.start();
    t2.start();
    t3.start();
    QVERIFY(t1.wait());
    QVERIFY(t2.wait());
    QVERIFY(t3.wait());

    //check all the constructed things have been destructed
    QCOMPARE(int(SPointer::count.loadRelaxed()), c);
}


void tst_QThreadStorage::valueBased()
{
    struct Thread : QThread {
        QThreadStorage<SPointer> &tlsSPointer;
        QThreadStorage<QString> &tlsString;
        QThreadStorage<int> &tlsInt;

        int someNumber;
        QString someString;
        Thread(QThreadStorage<SPointer> &t1, QThreadStorage<QString> &t2, QThreadStorage<int> &t3)
        : tlsSPointer(t1), tlsString(t2), tlsInt(t3) { }

        void run() {
            /*QVERIFY(!tlsSPointer.hasLocalData());
            QVERIFY(!tlsString.hasLocalData());
            QVERIFY(!tlsInt.hasLocalData());*/
            SPointer pointercopy = tlsSPointer.localData();

            //Default constructed values
            QVERIFY(tlsString.localData().isNull());
            QCOMPARE(tlsInt.localData(), 0);

            //setting
            tlsString.setLocalData(someString);
            tlsInt.setLocalData(someNumber);

            QCOMPARE(tlsString.localData(), someString);
            QCOMPARE(tlsInt.localData(), someNumber);

            //changing
            tlsSPointer.setLocalData(SPointer());
            tlsInt.localData() += 42;
            tlsString.localData().append(QLatin1String(" world"));

            QCOMPARE(tlsString.localData(), (someString + QLatin1String(" world")));
            QCOMPARE(tlsInt.localData(), (someNumber + 42));

            // operator=
            tlsString.localData() = QString::number(someNumber);
            QCOMPARE(tlsString.localData().toInt(), someNumber);
        }
    };

    QThreadStorage<SPointer> tlsSPointer;
    QThreadStorage<QString> tlsString;
    QThreadStorage<int> tlsInt;

    int c = SPointer::count.loadRelaxed();

    Thread t1(tlsSPointer, tlsString, tlsInt);
    Thread t2(tlsSPointer, tlsString, tlsInt);
    Thread t3(tlsSPointer, tlsString, tlsInt);
    t1.someNumber = 42;
    t2.someNumber = -128;
    t3.someNumber = 78;
    t1.someString = "hello";
    t2.someString = "australia";
    t3.someString = "nokia";

    t1.start();
    t2.start();
    t3.start();

    QVERIFY(t1.wait());
    QVERIFY(t2.wait());
    QVERIFY(t3.wait());

    QCOMPARE(c, int(SPointer::count.loadRelaxed()));

}


QTEST_MAIN(tst_QThreadStorage)
#include "tst_qthreadstorage.moc"
