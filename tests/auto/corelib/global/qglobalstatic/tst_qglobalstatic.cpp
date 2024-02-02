// Copyright (C) 2016 Thiago Macieira <thiago@kde.org>
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QThread>
#include <QTest>
#include <QReadWriteLock>

#if defined(Q_OS_UNIX) && !defined(Q_OS_INTEGRITY)
#include <sys/resource.h>
#endif

#include <QtTest/private/qemulationdetector_p.h>

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
#if defined(Q_OS_UNIX) && !defined(Q_OS_INTEGRITY)
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
    QCOMPARE(QtGlobalStatic::Holder<Q_QGS_throwingGS>::guard.loadRelaxed(), 0);
    QVERIFY(!throwingGS.exists());
    QVERIFY(!throwingGS.isDestroyed());
}

QBasicAtomicInt exceptionControlVar = Q_BASIC_ATOMIC_INITIALIZER(1);
Q_GLOBAL_STATIC_WITH_ARGS(ThrowingType, exceptionGS, (exceptionControlVar))
void tst_QGlobalStatic::catchExceptionAndRetry()
{
    if (exceptionControlVar.loadRelaxed() != 1)
        QSKIP("This test cannot be run more than once");
    ThrowingType::constructedCount.storeRelaxed(0);
    ThrowingType::destructedCount.storeRelaxed(0);

    bool exceptionCaught = false;
    try {
        exceptionGS();
    } catch (int) {
        exceptionCaught = true;
    }
    QCOMPARE(ThrowingType::constructedCount.loadRelaxed(), 1);
    QVERIFY(exceptionCaught);

    exceptionGS();
    QCOMPARE(ThrowingType::constructedCount.loadRelaxed(), 2);
}

QBasicAtomicInt threadStressTestControlVar = Q_BASIC_ATOMIC_INITIALIZER(5);
Q_GLOBAL_STATIC_WITH_ARGS(ThrowingType, threadStressTestGS, (threadStressTestControlVar))


void tst_QGlobalStatic::threadStressTest()
{
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("Frequently hangs on QEMU, QTBUG-91423");

    class ThreadStressTestThread: public QThread
    {
    public:
        QReadWriteLock *lock;
        void run() override
        {
            QReadLocker l(lock);
            //usleep(QRandomGenerator::global()->generate(200));
            // thundering herd
            try {
                threadStressTestGS();
            } catch (int) {
            }
        }
    };

    ThrowingType::constructedCount.storeRelaxed(0);
    ThrowingType::destructedCount.storeRelaxed(0);
    int expectedConstructionCount = threadStressTestControlVar.loadRelaxed() + 1;
    if (expectedConstructionCount <= 0)
        QSKIP("This test cannot be run more than once");

#ifdef Q_OS_INTEGRITY
    // OPEN_REALTIME_THREADS = 123 on current INTEGRITY environment
    // if try to create more, app is halted
    const int numThreads = 122;
#else
    const int numThreads = 200;
#endif
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
