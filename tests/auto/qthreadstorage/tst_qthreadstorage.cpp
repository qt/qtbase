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

#include <qcoreapplication.h>
#include <qmutex.h>
#include <qthread.h>
#include <qwaitcondition.h>
#include <qthreadstorage.h>

#ifdef Q_OS_UNIX
#include <pthread.h>
#endif
#ifdef Q_OS_WIN
#ifndef Q_OS_WINCE
#include <process.h>
#endif
#include <windows.h>
#endif

//TESTED_CLASS=
//TESTED_FILES=

class tst_QThreadStorage : public QObject
{
    Q_OBJECT

public:
    tst_QThreadStorage();

private slots:
    void hasLocalData();
    void localData();
    void localData_const();
    void setLocalData();
    void autoDelete();
    void adoptedThreads();
    void ensureCleanupOrder();
    void QTBUG13877_crashOnExit();
    void QTBUG14579_leakInDestructor();
    void QTBUG14579_resetInDestructor();
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

tst_QThreadStorage::tst_QThreadStorage()

{ }

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
#elif defined Q_OS_WIN
        HANDLE thread;
#if defined(Q_OS_WINCE)
        thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)testAdoptedThreadStorageWin, &pointers, 0, NULL);
#else
        thread = (HANDLE)_beginthread(testAdoptedThreadStorageWin, 0, &pointers);
#endif
        QVERIFY(thread);
        WaitForSingleObject(thread, INFINITE);
#endif
    }
    QVERIFY(threadStorageOk);

    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(Pointer::count, c);
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

void tst_QThreadStorage::QTBUG13877_crashOnExit()
{
    QProcess process;
#ifdef Q_OS_WIN
# ifdef QT_NO_DEBUG
    process.start("release/crashOnExit");
# else
    process.start("debug/crashOnExit");
# endif
#else
    process.start("./crashOnExit");
#endif
    QVERIFY(process.waitForFinished());
    QVERIFY(process.exitStatus() != QProcess::CrashExit);
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

Q_GLOBAL_STATIC(QThreadStorage<SPointer *>, QTBUG14579_pointers1)
Q_GLOBAL_STATIC(QThreadStorage<SPointer *>, QTBUG14579_pointers2)

class QTBUG14579_class
{
public:
    SPointer member;
    inline ~QTBUG14579_class() {
        QVERIFY(!QTBUG14579_pointers1()->hasLocalData());
        QVERIFY(!QTBUG14579_pointers2()->hasLocalData());
        QTBUG14579_pointers2()->setLocalData(new SPointer);
        QTBUG14579_pointers1()->setLocalData(new SPointer);
        QVERIFY(QTBUG14579_pointers1()->hasLocalData());
        QVERIFY(QTBUG14579_pointers2()->hasLocalData());
    }
};


void tst_QThreadStorage::QTBUG14579_leakInDestructor()
{
    class Thread : public QThread
    {
    public:
        QThreadStorage<QTBUG14579_class *> &tls;

        Thread(QThreadStorage<QTBUG14579_class *> &t) : tls(t) { }

        void run()
        {
            QVERIFY(!tls.hasLocalData());
            tls.setLocalData(new QTBUG14579_class);
            QVERIFY(tls.hasLocalData());
        }
    };
    int c = SPointer::count;

    QThreadStorage<QTBUG14579_class *> tls;

    QVERIFY(!QTBUG14579_pointers1()->hasLocalData());
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
    QCOMPARE(int(SPointer::count), c);
}

class QTBUG14579_reset {
public:
    SPointer member;
    ~QTBUG14579_reset();
};

Q_GLOBAL_STATIC(QThreadStorage<QTBUG14579_reset *>, QTBUG14579_resetTls)

QTBUG14579_reset::~QTBUG14579_reset() {
    //Quite stupid, but WTF::ThreadSpecific<T>::destroy does it.
    QTBUG14579_resetTls()->setLocalData(this);
}

void tst_QThreadStorage::QTBUG14579_resetInDestructor()
{
    class Thread : public QThread
    {
    public:
        void run()
        {
            QVERIFY(!QTBUG14579_resetTls()->hasLocalData());
            QTBUG14579_resetTls()->setLocalData(new QTBUG14579_reset);
            QVERIFY(QTBUG14579_resetTls()->hasLocalData());
        }
    };
    int c = SPointer::count;

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
    QCOMPARE(int(SPointer::count), c);
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

    int c = SPointer::count;

    Thread t1(tlsSPointer, tlsString, tlsInt);
    Thread t2(tlsSPointer, tlsString, tlsInt);
    Thread t3(tlsSPointer, tlsString, tlsInt);
    t1.someNumber = 42;
    t2.someNumber = -128;
    t3.someNumber = 78;
    t1.someString = "hello";
    t2.someString = "trolltech";
    t3.someString = "nokia";

    t1.start();
    t2.start();
    t3.start();

    QVERIFY(t1.wait());
    QVERIFY(t2.wait());
    QVERIFY(t3.wait());

    QCOMPARE(c, int(SPointer::count));

}


QTEST_MAIN(tst_QThreadStorage)
#include "tst_qthreadstorage.moc"
