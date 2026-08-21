// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define Q_NO_THREAD_STORAGE_TRIVIAL_WARNING

#include <QTest>
#if QT_CONFIG(process)
#include <QProcess>
#endif
#include <QTestEventLoop>

#include <qcoreapplication.h>
#include <qmutex.h>
#include <qthread.h>
#include <qwaitcondition.h>
#include <qthreadstorage.h>
#include <qdir.h>
#include <qfileinfo.h>

#include <array>
#include <QtCore/qxpfunctional.h>

#ifdef Q_OS_UNIX
#include <pthread.h>
#endif
#ifdef Q_OS_WIN
#  include <process.h>
#  include <qt_windows.h>
#endif

#ifdef QTEST_THROW_ON_FAIL
# error QVERIFY/QCOMPARE in QThread::run()
#endif

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

class Pointer
{
public:
    static int count;
    inline Pointer() { ++count; }
    inline ~Pointer() { --count; }
};
int Pointer::count = 0;

class tst_QThreadStorage : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void hasLocalData();
    void localData();
    void localData_const();
    void setLocalData();
    void autoDelete();
    void adoptedPThreads();
    void adoptedWinThreads();
    void ensureCleanupOrder();
    void noWarningOnExitForQGlobalStatic();
    void crashOnExit();
    void leakInDestructor();
    void resetInDestructor();
    void valueBased();
    void otherStorageInDestructor();

private:
    void adoptedThreads_impl(qxp::function_ref<void(QThreadStorage<Pointer *> &) const>);
};

void tst_QThreadStorage::hasLocalData()
{
    QThreadStorage<Pointer *> pointers;
    QVERIFY(!pointers.hasLocalData());
    pointers.setLocalData(new Pointer);
    QVERIFY(pointers.hasLocalData());
    pointers.setLocalData(nullptr);
    QVERIFY(!pointers.hasLocalData());
}

void tst_QThreadStorage::localData()
{
    QThreadStorage<Pointer*> pointers;
    {
        // the mere creation of a Pointer doesn't cause localData() to spring into existence:
        QVERIFY(!pointers.hasLocalData());
        [[maybe_unused]] Pointer p;
        QVERIFY(!pointers.hasLocalData());
    }
    Pointer *p = new Pointer;
    pointers.setLocalData(p);
    QVERIFY(pointers.hasLocalData());
    QCOMPARE(pointers.localData(), p);
    pointers.setLocalData(nullptr);
    QCOMPARE(pointers.localData(), nullptr);
    QVERIFY(!pointers.hasLocalData());
}

void tst_QThreadStorage::localData_const()
{
    QThreadStorage<Pointer *> pointers;
    const QThreadStorage<Pointer *> &const_pointers = pointers;
    QVERIFY(!pointers.hasLocalData());
    Pointer *p = new Pointer;
    pointers.setLocalData(p);
    QVERIFY(pointers.hasLocalData());
    QCOMPARE(const_pointers.localData(), p);
    pointers.setLocalData(nullptr);
    QCOMPARE(const_pointers.localData(), nullptr);
    QVERIFY(!pointers.hasLocalData());
}

void tst_QThreadStorage::setLocalData()
{
    QThreadStorage<Pointer *> pointers;
    QVERIFY(!pointers.hasLocalData());
    pointers.setLocalData(new Pointer);
    QVERIFY(pointers.hasLocalData());
    pointers.setLocalData(nullptr);
    QVERIFY(!pointers.hasLocalData());
}

class Thread : public QThread
{
    Q_OBJECT
public:
    QThreadStorage<Pointer *> &pointers;

    QMutex mutex;
    QWaitCondition cond;

    Thread(QThreadStorage<Pointer *> &p)
        : pointers(p)
    { }

    void run() override
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

static bool threadStorageOk;
void testAdoptedThreadStorage(void *p)
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

void tst_QThreadStorage::adoptedThreads_impl(qxp::function_ref<void(QThreadStorage<Pointer *> &) const> run)
{
    QTestEventLoop::instance(); // Make sure the instance is created in this thread.
    QThreadStorage<Pointer *> pointers;
    int c = Pointer::count;
    threadStorageOk = true;
    run(pointers);
    if (QTest::currentTestResolved())
        return;
    QVERIFY(threadStorageOk);

    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QTRY_COMPARE(Pointer::count, c);
}

void tst_QThreadStorage::adoptedPThreads()
{
#ifdef Q_OS_UNIX
    adoptedThreads_impl([] (QThreadStorage<Pointer *> &pointers) {
        const auto returnValueAdded = [](void *pointers) -> void* {
            testAdoptedThreadStorage(pointers);
            return nullptr; // pthread wants a void* return
        };
        pthread_t thread;
        const int state = pthread_create(&thread, nullptr, returnValueAdded, &pointers);
        QCOMPARE(state, 0);
        pthread_join(thread, nullptr);
    });
#else
    QSKIP("This is a Unix test");
#endif
}

void tst_QThreadStorage::adoptedWinThreads()
{
#ifdef Q_OS_WIN
    adoptedThreads_impl([] (QThreadStorage<Pointer *> &pointers) {
        HANDLE thread;
        thread = (HANDLE)_beginthread(testAdoptedThreadStorage, 0, &pointers);
        QVERIFY(thread);
        WaitForSingleObject(thread, INFINITE);
    });
#else
    QSKIP("This is a Windows test.");
#endif

}

static QBasicAtomicInt cleanupOrder = Q_BASIC_ATOMIC_INITIALIZER(0);

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
    class Thread : public QThread // clazy:exclude=missing-qobject-macro
    {
    public:
        QThreadStorage<First *> &first;
        QThreadStorage<Second *> &second;

        Thread(QThreadStorage<First *> &first,
               QThreadStorage<Second *> &second)
            : first(first), second(second)
        { }

        void run() override
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

void tst_QThreadStorage::noWarningOnExitForQGlobalStatic()
{
#ifdef Q_OS_ANDROID
    QSKIP("Can't start QProcess to run a custom user binary on Android");
#endif
#if !QT_CONFIG(process)
    QSKIP("No QProcess support");
#else
#  ifdef Q_OS_WIN
    QSKIP("Not able to detect a call to exit() in Windows.");
    QString binary = QFINDTESTDATA("tst_qthreadstorage_warnonexit.exe");
#  else
    QString binary = QFINDTESTDATA("tst_qthreadstorage_warnonexit");
#  endif
    QProcess process;
    process.start(binary);
    QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
    QVERIFY2(process.waitForFinished(), qPrintable(process.errorString()));
    QCOMPARE(process.readAllStandardError(), QByteArray());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
#endif
}

#if QT_CONFIG(process)
static inline bool runCrashOnExit(const QString &binary, QString *errorMessage)
{
    const int timeout = 60000;
    QProcess process;
    process.start(binary);
    if (!process.waitForStarted()) {
        *errorMessage = "Could not start '%1': %2"_L1.arg(binary, process.errorString());
        return false;
    }
    if (!process.waitForFinished(timeout)) {
        process.kill();
        *errorMessage = u"Timeout (%1ms) waiting for %2."_s.arg(timeout).arg(binary);
        return false;
    }
    if (process.exitStatus() != QProcess::NormalExit) {
        *errorMessage = binary + " crashed."_L1;
        return false;
    }
    return true;
}
#endif

void tst_QThreadStorage::crashOnExit()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_HARMONY)
    QSKIP("Can't start QProcess to run a custom user binary on Android");
#endif
#if !QT_CONFIG(process)
    QSKIP("No qprocess support");
#else
    QString errorMessage;
    QVERIFY2(runCrashOnExit(u"./crashOnExit_helper"_s, &errorMessage),
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
    class Thread : public QThread // clazy:exclude=missing-qobject-macro
    {
    public:
        QThreadStorage<ThreadStorageLocalDataTester *> &tls;

        Thread(QThreadStorage<ThreadStorageLocalDataTester *> &t) : tls(t) { }

        void run() override
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
    class Thread : public QThread // clazy:exclude=missing-qobject-macro
    {
    public:
        void run() override
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
    struct Thread : QThread // clazy:exclude=missing-qobject-macro
    {
        QThreadStorage<SPointer> &tlsSPointer;
        QThreadStorage<QString> &tlsString;
        QThreadStorage<int> &tlsInt;

        int someNumber;
        QString someString;
        Thread(QThreadStorage<SPointer> &t1, QThreadStorage<QString> &t2, QThreadStorage<int> &t3)
        : tlsSPointer(t1), tlsString(t2), tlsInt(t3) { }

        void run()  override
        {
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
    t1.someString = u"hello"_s;
    t2.someString = u"australia"_s;
    t3.someString = u"nokia"_s;

    t1.start();
    t2.start();
    t3.start();

    QVERIFY(t1.wait());
    QVERIFY(t2.wait());
    QVERIFY(t3.wait());

    QCOMPARE(c, int(SPointer::count.loadRelaxed()));

}


// ---------------------------------------------------------------------
// QThreadStorage reentrancy contracts.
//
// The destructor of the previous payload runs from within setLocalData():
//
//  - otherStorageInDestructor(): the destructor may use other
//    QThreadStorage objects, including ones the current thread has never
//    used before.
//
// Running these under ASan/LSan additionally verifies the memory safety
// of the above (no use-after-free, no leaks).
// ---------------------------------------------------------------------

namespace {

// A payload whose destructor makes first use of a different QThreadStorage
// (with a much higher id) on the current thread:
class OtherStorageUsingPayload
{
public:
    explicit OtherStorageUsingPayload(QThreadStorage<int> *other) : other(other) {}
    ~OtherStorageUsingPayload() { other->setLocalData(42); }

private:
    QThreadStorage<int> *other;
};

} // unnamed namespace

void tst_QThreadStorage::otherStorageInDestructor()
{
    struct Thread : public QThread // clazy:exclude=missing-qobject-macro
    {
        QThreadStorage<OtherStorageUsingPayload *> &primary;
        QThreadStorage<int> &other;

        explicit Thread(QThreadStorage<OtherStorageUsingPayload *> &p, QThreadStorage<int> &o)
            : primary(p), other(o)
        { }

        void run() override
        {
            primary.setLocalData(new OtherStorageUsingPayload(&other));
            QVERIFY(!other.hasLocalData());
            primary.setLocalData(nullptr);
            // the destructor made first use of `other` on this thread:
            QVERIFY(!primary.hasLocalData());
            QVERIFY(other.hasLocalData());
            QCOMPARE(other.localData(), 42);
        }
    };

    QThreadStorage<OtherStorageUsingPayload *> primary;

    // Burn through a large block of ids, so `other`'s id is far higher
    // than `primary`'s, and higher than anything a freshly-started thread
    // has used; its first use, from within the payload's destructor, then
    // grows the thread's storage while setLocalData(nullptr) is still
    // executing.
    std::array<QThreadStorage<int>, 256> others;

    Thread t(primary, others.back());
    t.start();
    QVERIFY(t.wait(10s));
}

QTEST_MAIN(tst_QThreadStorage)
#include "tst_qthreadstorage.moc"
