/****************************************************************************
**
** Copyright (C) 2016 Thiago Macieira <thiago@kde.org>
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QThread>
#include <QtTest/QtTest>

#if defined(Q_OS_UNIX)
#include <sys/resource.h>
#endif

class tst_QGlobalStatic : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();

private Q_SLOTS:
    void beforeInitialization();
    void api();
    void constVolatile();
    void exception();
    void catchExceptionAndRetry();
    void threadStressTest();
    void afterDestruction();
};

void tst_QGlobalStatic::initTestCase()
{
#if defined(Q_OS_UNIX)
    // The tests create a lot of threads, which require file descriptors. On systems like
    // OS X low defaults such as 256 as the limit for the number of simultaneously
    // open files is not sufficient.
    struct rlimit numFiles;
    if (getrlimit(RLIMIT_NOFILE, &numFiles) == 0 && numFiles.rlim_cur < 1024) {
        numFiles.rlim_cur = qMin(rlim_t(1024), numFiles.rlim_max);
        setrlimit(RLIMIT_NOFILE, &numFiles);
    }
#endif
}

Q_GLOBAL_STATIC_WITH_ARGS(const int, constInt, (42))
Q_GLOBAL_STATIC_WITH_ARGS(volatile int, volatileInt, (-47))

void otherFunction()
{
    // never called
    constInt();
    volatileInt();
}

// do not initialize the following Q_GLOBAL_STATIC
Q_GLOBAL_STATIC(int, checkedBeforeInitialization)
void tst_QGlobalStatic::beforeInitialization()
{
    QVERIFY(!checkedBeforeInitialization.exists());
    QVERIFY(!checkedBeforeInitialization.isDestroyed());
}

struct Type {
    int i;
};

Q_GLOBAL_STATIC(Type, checkedAfterInitialization)
void tst_QGlobalStatic::api()
{
    // check the API
    QVERIFY((Type *)checkedAfterInitialization);
    QVERIFY(checkedAfterInitialization());
    *checkedAfterInitialization = Type();
    *checkedAfterInitialization() = Type();

    checkedAfterInitialization()->i = 47;
    checkedAfterInitialization->i = 42;
    QCOMPARE(checkedAfterInitialization()->i, 42);
    checkedAfterInitialization()->i = 47;
    QCOMPARE(checkedAfterInitialization->i, 47);

    QVERIFY(checkedAfterInitialization.exists());
    QVERIFY(!checkedAfterInitialization.isDestroyed());
}

void tst_QGlobalStatic::constVolatile()
{
    QCOMPARE(*constInt(), 42);
    QCOMPARE((int)*volatileInt(), -47);
    QCOMPARE(*constInt(), 42);
    QCOMPARE((int)*volatileInt(), -47);
}

struct ThrowingType
{
    static QBasicAtomicInt constructedCount;
    static QBasicAtomicInt destructedCount;
    ThrowingType()
    {
        throw 0;
    }

    ThrowingType(QBasicAtomicInt &throwControl)
    {
        constructedCount.ref();
        if (throwControl.fetchAndAddRelaxed(-1) != 0)
            throw 0;
    }
    ~ThrowingType() { destructedCount.ref(); }
};

QBasicAtomicInt ThrowingType::constructedCount = Q_BASIC_ATOMIC_INITIALIZER(0);
QBasicAtomicInt ThrowingType::destructedCount = Q_BASIC_ATOMIC_INITIALIZER(0);

Q_GLOBAL_STATIC(ThrowingType, throwingGS)
void tst_QGlobalStatic::exception()
{
    bool exceptionCaught = false;
    try {
        throwingGS();
    } catch (int) {
        exceptionCaught = true;
    }
    QVERIFY(exceptionCaught);
    QCOMPARE(Q_QGS_throwingGS::guard.load(), 0);
    QVERIFY(!throwingGS.exists());
    QVERIFY(!throwingGS.isDestroyed());
}

QBasicAtomicInt exceptionControlVar = Q_BASIC_ATOMIC_INITIALIZER(1);
Q_GLOBAL_STATIC_WITH_ARGS(ThrowingType, exceptionGS, (exceptionControlVar))
void tst_QGlobalStatic::catchExceptionAndRetry()
{
    if (exceptionControlVar.load() != 1)
        QSKIP("This test cannot be run more than once");
    ThrowingType::constructedCount.store(0);
    ThrowingType::destructedCount.store(0);

    bool exceptionCaught = false;
    try {
        exceptionGS();
    } catch (int) {
        exceptionCaught = true;
    }
    QCOMPARE(ThrowingType::constructedCount.load(), 1);
    QVERIFY(exceptionCaught);

    exceptionGS();
    QCOMPARE(ThrowingType::constructedCount.load(), 2);
}

QBasicAtomicInt threadStressTestControlVar = Q_BASIC_ATOMIC_INITIALIZER(5);
Q_GLOBAL_STATIC_WITH_ARGS(ThrowingType, threadStressTestGS, (threadStressTestControlVar))


void tst_QGlobalStatic::threadStressTest()
{
    class ThreadStressTestThread: public QThread
    {
    public:
        QReadWriteLock *lock;
        void run()
        {
            QReadLocker l(lock);
            //usleep(qrand() * 200 / RAND_MAX);
            // thundering herd
            try {
                threadStressTestGS();
            } catch (int) {
            }
        }
    };

    ThrowingType::constructedCount.store(0);
    ThrowingType::destructedCount.store(0);
    int expectedConstructionCount = threadStressTestControlVar.load() + 1;
    if (expectedConstructionCount <= 0)
        QSKIP("This test cannot be run more than once");

    const int numThreads = 200;
    ThreadStressTestThread threads[numThreads];
    QReadWriteLock lock;
    lock.lockForWrite();
    for (int i = 0; i < numThreads; ++i) {
        threads[i].lock = &lock;
        threads[i].start();
    }

    // wait for all threads
    // release the herd
    lock.unlock();

    for (int i = 0; i < numThreads; ++i)
        threads[i].wait();

    QCOMPARE(ThrowingType::constructedCount.loadAcquire(), expectedConstructionCount);
    QCOMPARE(ThrowingType::destructedCount.loadAcquire(), 0);
}

Q_GLOBAL_STATIC(int, checkedAfterDestruction)
void tst_QGlobalStatic::afterDestruction()
{
    // this test will not produce results now
    // it will simply run some code on destruction (after the global statics have been deleted)
    // if that fails, this will cause a crash

    // static destruction is LIFO: so we must add our exit-time code before the
    // global static is used for the first time
    static struct RunAtExit {
        ~RunAtExit() {
            int *ptr = checkedAfterDestruction();
            if (ptr)
                qFatal("Global static is not null as was expected");
        }
    } runAtExit;
    (void) runAtExit;

    *checkedAfterDestruction = 42;
}

QTEST_APPLESS_MAIN(tst_QGlobalStatic);

#include "tst_qglobalstatic.moc"
