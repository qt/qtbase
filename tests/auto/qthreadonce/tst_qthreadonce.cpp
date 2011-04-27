/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
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
#include "qthreadonce.h"

//TESTED_CLASS=
//TESTED_FILES=corelib/thread/qthreadonce.h corelib/thread/qthreadonce.cpp

class tst_QThreadOnce : public QObject
{
    Q_OBJECT

private slots:
    void sameThread();
    void sameThread_data();
    void multipleThreads();

    void nesting();
    void reentering();
    void exception();
};

class SingletonObject: public QObject
{
    Q_OBJECT
public:
    static int runCount;
    SingletonObject() { val = 42; ++runCount; }
    ~SingletonObject() { }

    QBasicAtomicInt val;
};

class IncrementThread: public QThread
{
public:
    static QBasicAtomicInt runCount;
    static QSingleton<SingletonObject> singleton;
    QSemaphore &sem1, &sem2;
    int &var;

    IncrementThread(QSemaphore *psem1, QSemaphore *psem2, int *pvar, QObject *parent)
        : QThread(parent), sem1(*psem1), sem2(*psem2), var(*pvar)
        { start(); }

    ~IncrementThread() { wait(); }

protected:
    void run()
        {
            sem2.release();
            sem1.acquire();             // synchronize

            Q_ONCE {
                ++var;
            }
            runCount.ref();
            singleton->val.ref();
        }
};
int SingletonObject::runCount = 0;
QBasicAtomicInt IncrementThread::runCount = Q_BASIC_ATOMIC_INITIALIZER(0);
QSingleton<SingletonObject> IncrementThread::singleton;

void tst_QThreadOnce::sameThread_data()
{
    SingletonObject::runCount = 0;
    QTest::addColumn<int>("expectedValue");

    QTest::newRow("first") << 42;
    QTest::newRow("second") << 43;
}

void tst_QThreadOnce::sameThread()
{
    static int controlVariable = 0;
    Q_ONCE {
        QCOMPARE(controlVariable, 0);
        ++controlVariable;
    }
    QCOMPARE(controlVariable, 1);

    static QSingleton<SingletonObject> s;
    QTEST((int)s->val, "expectedValue");
    s->val.ref();

    QCOMPARE(SingletonObject::runCount, 1);
}

void tst_QThreadOnce::multipleThreads()
{
#if defined(Q_OS_WINCE) || defined(Q_OS_VXWORKS) || defined(Q_OS_SYMBIAN)
    const int NumberOfThreads = 20;
#else
    const int NumberOfThreads = 100;
#endif
    int controlVariable = 0;
    QSemaphore sem1, sem2(NumberOfThreads);

    QObject *parent = new QObject;
    for (int i = 0; i < NumberOfThreads; ++i)
        new IncrementThread(&sem1, &sem2, &controlVariable, parent);

    QCOMPARE(controlVariable, 0); // nothing must have set them yet
    SingletonObject::runCount = 0;
    IncrementThread::runCount = 0;

    // wait for all of them to be ready
    sem2.acquire(NumberOfThreads);
    // unleash the threads
    sem1.release(NumberOfThreads);

    // wait for all of them to terminate:
    delete parent;

    QCOMPARE(controlVariable, 1);
    QCOMPARE((int)IncrementThread::runCount, NumberOfThreads);
    QCOMPARE(SingletonObject::runCount, 1);
}

void tst_QThreadOnce::nesting()
{
    int variable = 0;
    Q_ONCE {
        Q_ONCE {
            ++variable;
        }
    }

    QVERIFY(variable == 1);
}

static void reentrant(int control, int &counter)
{
    Q_ONCE {
        if (counter)
            reentrant(--control, counter);
        ++counter;
    }
    static QSingleton<SingletonObject> s;
    s->val.ref();
}

void tst_QThreadOnce::reentering()
{
    const int WantedRecursions = 5;
    int count = 0;
    SingletonObject::runCount = 0;
    reentrant(WantedRecursions, count);

    // reentrancy is undefined behavior:
    QVERIFY(count == 1 || count == WantedRecursions);
    QCOMPARE(SingletonObject::runCount, 1);
}

#if !defined(QT_NO_EXCEPTIONS)
static void exception_helper(int &val)
{
    Q_ONCE {
        if (val++ == 0) throw 0;
    }
}
#endif

void tst_QThreadOnce::exception()
{
#if defined(QT_NO_EXCEPTIONS)
    QSKIP("Compiled without exceptions, skipping test", SkipAll);
#else
    int count = 0;

    try {
        exception_helper(count);
    } catch (...) {
        // nothing
    }
    QCOMPARE(count, 1);

    try {
        exception_helper(count);
    } catch (...) {
        QVERIFY2(false, "Exception shouldn't have been thrown...");
    }
    QCOMPARE(count, 2);
#endif
}

QTEST_MAIN(tst_QThreadOnce)
#include "tst_qthreadonce.moc"
