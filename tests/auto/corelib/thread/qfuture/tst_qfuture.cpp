// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#define QFUTURE_TEST

#include <QCoreApplication>
#include <QDebug>
#include <QSemaphore>
#include <QTestEventLoop>
#include <QTimer>
#include <QSignalSpy>
#include <QVarLengthArray>
#include <QSet>
#include <QList>
#include <private/qobject_p.h>

#include <QTest>
#include <qfuture.h>
#include <qfuturewatcher.h>
#include <qresultstore.h>
#include <qthreadpool.h>
#include <qexception.h>
#include <qrandom.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include <private/qfutureinterface_p.h>

#include <forward_list>
#include <list>
#include <vector>
#include <memory>
#include <set>

// COM interface macro.
#if defined(Q_OS_WIN) && defined(interface)
#  undef interface
#endif

using namespace std::chrono_literals;
static constexpr auto DefaultWaitTime = 2s;

using namespace Qt::StringLiterals;

class SenderObject : public QObject
{
    Q_OBJECT

public:
    void emitNoArg() { emit noArgSignal(); }
    void emitIntArg(int value) { emit intArgSignal(value); }
    void emitConstRefArg(const QString &value) { emit constRefArg(value); }
    void emitMultipleArgs(int value1, double value2, const QString &value3)
    {
        emit multipleArgs(value1, value2, value3);
    }
    void emitTupleArgSignal(const std::tuple<int, double, QString> &t) { emit tupleArgSignal(t); }
    void emitMultiArgsWithTupleSignal1(int value, const std::tuple<int, double, QString> &t)
    {
        emit multiArgsWithTupleSignal1(value, t);
    }
    void emitMultiArgsWithTupleSignal2(const std::tuple<int, double, QString> &t, int value)
    {
        emit multiArgsWithTupleSignal2(t, value);
    }
    void emitMultiArgsWithPairSignal1(int value, const std::pair<int, double> &p)
    {
        emit multiArgsWithPairSignal1(value, p);
    }
    void emitMultiArgsWithPairSignal2(const std::pair<int, double> &p, int value)
    {
        emit multiArgsWithPairSignal2(p, value);
    }

    void emitNoArgPrivateSignal() { emit noArgPrivateSignal(QPrivateSignal()); }
    void emitIntArgPrivateSignal(int value) { emit intArgPrivateSignal(value, QPrivateSignal()); }
    void emitMultiArgsPrivateSignal(int value1, double value2, const QString &value3)
    {
        emit multiArgsPrivateSignal(value1, value2, value3, QPrivateSignal());
    }
    void emitTupleArgPrivateSignal(const std::tuple<int, double, QString> &t)
    {
        emit tupleArgPrivateSignal(t, QPrivateSignal());
    }
    void emitMultiArgsWithTuplePrivateSignal1(int value, const std::tuple<int, double, QString> &t)
    {
        emit multiArgsWithTuplePrivateSignal1(value, t, QPrivateSignal());
    }
    void emitMultiArgsWithTuplePrivateSignal2(const std::tuple<int, double, QString> &t, int value)
    {
        emit multiArgsWithTuplePrivateSignal2(t, value, QPrivateSignal());
    }
    void emitMultiArgsWithPairPrivateSignal1(int value, const std::pair<int, double> &p)
    {
        emit multiArgsWithPairPrivateSignal1(value, p, QPrivateSignal());
    }
    void emitMultiArgsWithPairPrivateSignal2(const std::pair<int, double> &p, int value)
    {
        emit multiArgsWithPairPrivateSignal2(p, value, QPrivateSignal());
    }

signals:
    void noArgSignal();
    void intArgSignal(int value);
    void constRefArg(const QString &value);
    void multipleArgs(int value1, double value2, const QString &value3);
    void tupleArgSignal(const std::tuple<int, double, QString> &t);
    void multiArgsWithTupleSignal1(int value, const std::tuple<int, double, QString> &t);
    void multiArgsWithTupleSignal2(const std::tuple<int, double, QString> &t, int value);
    void multiArgsWithPairSignal1(int value, const std::pair<int, double> &p);
    void multiArgsWithPairSignal2(const std::pair<int, double> &p, int value);

    // Private signals
    void noArgPrivateSignal(QPrivateSignal);
    void intArgPrivateSignal(int value, QPrivateSignal);
    void multiArgsPrivateSignal(int value1, double value2, const QString &value3, QPrivateSignal);
    void tupleArgPrivateSignal(const std::tuple<int, double, QString> &t, QPrivateSignal);
    void multiArgsWithTuplePrivateSignal1(int value, const std::tuple<int, double, QString> &t,
                                          QPrivateSignal);
    void multiArgsWithTuplePrivateSignal2(const std::tuple<int, double, QString> &t, int value,
                                          QPrivateSignal);
    void multiArgsWithPairPrivateSignal1(int value, const std::pair<int, double> &p,
                                         QPrivateSignal);
    void multiArgsWithPairPrivateSignal2(const std::pair<int, double> &p, int value,
                                         QPrivateSignal);
};

class LambdaThread : public QThread
{
public:
    LambdaThread(std::function<void ()> fn)
    :m_fn(fn)
    {

    }

    void run() override
    {
        m_fn();
    }

private:
    std::function<void ()> m_fn;
};

// Emulates QWidget behavior by deleting its children early in the destructor
// instead of leaving it to ~QObject()
class FakeQWidget : public QObject
{
    Q_OBJECT
public:
    ~FakeQWidget() override {
        auto *d = QObjectPrivate::get(this);
        d->deleteChildren();
    }
};

using UniquePtr = std::unique_ptr<int>;

class tst_QFuture: public QObject
{
    Q_OBJECT
private slots:
    void resultStore();
    void future();
    void futureToVoid();
    void futureInterface();
    void refcounting();
    void cancel();
    void cancelAndFinish();
    void statePropagation();
    void multipleResults();
    void indexedResults();
    void progress();
    void setProgressRange();
    void progressWithRange();
    void progressText();
    void resultsAfterFinished();
    void resultsAsList();
    void iterators();
    void iteratorsThread();
#if QT_DEPRECATED_SINCE(6, 0)
    void pause();
    void suspendCheckPaused();
#endif
    void suspend();
    void throttling();
    void voidConversions();
#ifndef QT_NO_EXCEPTIONS
    void exceptions();
    void nestedExceptions();
#endif
    void nonGlobalThreadPool();

    void then();
    void thenForMoveOnlyTypes();
    void thenOnCanceledFuture();
#ifndef QT_NO_EXCEPTIONS
    void thenOnExceptionFuture();
    void thenThrows();
    void onFailed();
    void onFailedTestCallables();
    void onFailedForMoveOnlyTypes();
#endif
    void onCanceled();
    void cancelContinuations();
    void continuationsWithContext();
    void continuationsWithMoveOnlyLambda();
#if 0
    // TODO: enable when QFuture::takeResults() is enabled
    void takeResults();
#endif
    void takeResult();
    void runAndTake();
    void resultsReadyAt_data();
    void resultsReadyAt();
    void takeResultWorksForTypesWithoutDefaultCtor();
    void canceledFutureIsNotValid();
    void signalConnect();
    void waitForFinished();

    void rejectResultOverwrite_data();
    void rejectResultOverwrite();
    void rejectPendingResultOverwrite_data() { rejectResultOverwrite_data(); }
    void rejectPendingResultOverwrite();

    void createReadyFutures();
    void continuationsAfterReadyFutures();

    void getFutureInterface();
    void convertQMetaType();

    void whenAllIterators();
    void whenAllIteratorsWithCanceled();
    void whenAllIteratorsWithFailed();
    void whenAllDifferentTypes();
    void whenAllDifferentTypesWithCanceled();
    void whenAllDifferentTypesWithFailed();
    void whenAnyIterators();
    void whenAnyIteratorsWithCanceled();
    void whenAnyIteratorsWithFailed();
    void whenAnyDifferentTypes();
    void whenAnyDifferentTypesWithCanceled();
    void whenAnyDifferentTypesWithFailed();

    void continuationOverride();
    void continuationsDontLeak();
    void cancelAfterFinishWithContinuations();

    void unwrap();

private:
    using size_type = std::vector<int>::size_type;

    static void testSingleResult(const UniquePtr &p);
    static void testSingleResult(const std::vector<int> &v);
    template<class T>
    static void testSingleResult(const T &unknown);
    template<class T>
    static void testFutureTaken(QFuture<T> &noMoreFuture);
    template<class T>
    static  void testTakeResults(QFuture<T> future, size_type resultCount);
};

class IntResultsCleaner
{
public:
    IntResultsCleaner(QtPrivate::ResultStoreBase &s) : store(s) { }
    ~IntResultsCleaner() { store.clear<int>(); }

private:
    QtPrivate::ResultStoreBase &store;
};

void tst_QFuture::resultStore()
{
    int int0 = 0;
    int int1 = 1;
    int int2 = 2;

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        QCOMPARE(store.begin(), store.end());
        QCOMPARE(store.resultAt(0), store.end());
        QCOMPARE(store.resultAt(1), store.end());
    }


    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(-1, &int0);
        store.addResult(1, &int1);
        QtPrivate::ResultIteratorBase it = store.begin();
        QCOMPARE(it.resultIndex(), 0);
        QVERIFY(it == store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 1);
        QVERIFY(it != store.begin());
        QVERIFY(it != store.end());

        ++it;
        QVERIFY(it != store.begin());
        QVERIFY(it == store.end());
    }

    QList<int> vec0 = QList<int>() << 2 << 3;
    QList<int> vec1 = QList<int>() << 4 << 5;

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResults(-1, &vec0, 2);
        store.addResults(-1, &vec1, 2);
        QtPrivate::ResultIteratorBase it = store.begin();
        QCOMPARE(it.resultIndex(), 0);
        QCOMPARE(it, store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 1);
        QVERIFY(it != store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 2);

        ++it;
        QCOMPARE(it.resultIndex(), 3);

        ++it;
        QCOMPARE(it, store.end());
    }
    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(-1, &int0);
        store.addResults(-1, &vec1, 2);
        store.addResult(-1, &int1);

        QtPrivate::ResultIteratorBase it = store.begin();
        QCOMPARE(it.resultIndex(), 0);
        QVERIFY(it == store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 1);
        QVERIFY(it != store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 2);
        QVERIFY(it != store.end());
        ++it;
        QCOMPARE(it.resultIndex(), 3);
        QVERIFY(it != store.end());
        ++it;
        QVERIFY(it == store.end());

        QCOMPARE(store.resultAt(0).resultIndex(), 0);
        QCOMPARE(store.resultAt(1).resultIndex(), 1);
        QCOMPARE(store.resultAt(2).resultIndex(), 2);
        QCOMPARE(store.resultAt(3).resultIndex(), 3);
        QCOMPARE(store.resultAt(4), store.end());
    }
    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(-1, &int0);
        store.addResults(-1, &vec0);
        store.addResult(-1, &int1);

        QtPrivate::ResultIteratorBase it = store.begin();
        QCOMPARE(it.resultIndex(), 0);
        QVERIFY(it == store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 1);
        QVERIFY(it != store.begin());
        QVERIFY(it != store.end());

        ++it;
        QCOMPARE(it.resultIndex(), 2);
        QVERIFY(it != store.end());
        ++it;
        QCOMPARE(it.resultIndex(), 3);
        QVERIFY(it != store.end());
        ++it;
        QVERIFY(it == store.end());

        QCOMPARE(store.resultAt(0).value<int>(), int0);
        QCOMPARE(store.resultAt(1).value<int>(), vec0[0]);
        QCOMPARE(store.resultAt(2).value<int>(), vec0[1]);
        QCOMPARE(store.resultAt(3).value<int>(), int1);
    }
    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(-1, &int0);
        store.addResults(-1, &vec0);
        store.addResult(200, &int1);

        QCOMPARE(store.resultAt(0).value<int>(), int0);
        QCOMPARE(store.resultAt(1).value<int>(), vec0[0]);
        QCOMPARE(store.resultAt(2).value<int>(), vec0[1]);
        QCOMPARE(store.resultAt(200).value<int>(), int1);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(1, &int1);
        store.addResult(0, &int0);
        store.addResult(-1, &int2);

        QCOMPARE(store.resultAt(0).value<int>(), int0);
        QCOMPARE(store.resultAt(1).value<int>(), int1);
        QCOMPARE(store.resultAt(2).value<int>(), int2);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        QCOMPARE(store.contains(0), false);
        QCOMPARE(store.contains(1), false);
        QCOMPARE(store.contains(INT_MAX), false);
    }

    {
        // Test filter mode, where "gaps" in the result array aren't allowed.
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);

        store.addResult(0, &int0);
        QCOMPARE(store.contains(0), true);

        store.addResult(2, &int2); // add result at index 2
        QCOMPARE(store.contains(2), false); // but 1 is missing, so this 2 won't be reported yet.

        store.addResult(1, &int1);
        QCOMPARE(store.contains(1), true);
        QCOMPARE(store.contains(2), true); // 2 should be visible now.

        store.addResult(4, &int0);
        store.addResult(5, &int0);
        store.addResult(7, &int0);
        QCOMPARE(store.contains(4), false);
        QCOMPARE(store.contains(5), false);
        QCOMPARE(store.contains(7), false);

        store.addResult(3, &int0);  // adding 3 makes 4 and 5 visible
        QCOMPARE(store.contains(4), true);
        QCOMPARE(store.contains(5), true);
        QCOMPARE(store.contains(7), false);

        store.addResult(6, &int0);  // adding 6 makes 7 visible

        QCOMPARE(store.contains(6), true);
        QCOMPARE(store.contains(7), true);
        QCOMPARE(store.contains(8), false);
    }

    {
        // test canceled results
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);

        store.addResult(0, &int0);
        QCOMPARE(store.contains(0), true);

        store.addResult(2, &int0);
        QCOMPARE(store.contains(2), false);

        store.addCanceledResult(1); // report no result at 1

        QCOMPARE(store.contains(0), true);
        QCOMPARE(store.contains(1), true); // 2 gets renamed to 1
        QCOMPARE(store.contains(2), false);

        store.addResult(3, &int0);
        QCOMPARE(store.contains(2), true); //3 gets renamed to 2

        store.addResult(6, &int0);
        store.addResult(7, &int0);
        QCOMPARE(store.contains(3), false);

        store.addCanceledResult(4);
        store.addCanceledResult(5);

        QCOMPARE(store.contains(3), true); //6 gets renamed to 3
        QCOMPARE(store.contains(4), true); //7 gets renamed to 4

        store.addResult(8, &int0);
        QCOMPARE(store.contains(5), true); //8 gets renamed to 4

        QCOMPARE(store.contains(6), false);
        QCOMPARE(store.contains(7), false);
    }

    {
        // test addResult return value
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);

        store.addResult(0, &int0);
        QCOMPARE(store.count(), 1); // result 0 becomes available
        QCOMPARE(store.contains(0), true);

        store.addResult(2, &int0);
        QCOMPARE(store.count(), 1);
        QCOMPARE(store.contains(2), false);

        store.addCanceledResult(1);
        QCOMPARE(store.count(), 2); // result 2 is renamed to 1 and becomes available

        QCOMPARE(store.contains(0), true);
        QCOMPARE(store.contains(1), true);
        QCOMPARE(store.contains(2), false);

        store.addResult(3, &int0);
        QCOMPARE(store.count(), 3);
        QCOMPARE(store.contains(2), true);

        store.addResult(6, &int0);
        QCOMPARE(store.count(), 3);
        store.addResult(7, &int0);
        QCOMPARE(store.count(), 3);
        QCOMPARE(store.contains(3), false);

        store.addCanceledResult(4);
        store.addCanceledResult(5);
        QCOMPARE(store.count(), 5); // 6 and 7 is renamed to 3 and 4 and becomes available

        QCOMPARE(store.contains(3), true);
        QCOMPARE(store.contains(4), true);

        store.addResult(8, &int0);
        QCOMPARE(store.contains(5), true);
        QCOMPARE(store.count(), 6);

        QCOMPARE(store.contains(6), false);
        QCOMPARE(store.contains(7), false);
    }

    {
        // test resultCount in non-filtered mode. It should always be possible
        // to iterate through the results 0 to resultCount.
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(0, &int0);

        QCOMPARE(store.count(), 1);

        store.addResult(2, &int0);

        QCOMPARE(store.count(), 1);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 3);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(2, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(0, &int0);
        QCOMPARE(store.count(), 3);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResults(2, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResult(1, &int0);
        QCOMPARE(store.count(), 0);

        store.addResult(0, &int0);
        QCOMPARE(store.count(), 4);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResults(2, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 4);
    }
    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 2);

        store.addResult(2, &int0);
        QCOMPARE(store.count(), 5);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addResults(0, &vec0);
        QCOMPARE(store.count(), 2);

        store.addCanceledResult(2);
        QCOMPARE(store.count(), 4);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addCanceledResults<int>(0, 3);
        QCOMPARE(store.count(), 2);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);
        store.addResults(3, &vec1);
        QCOMPARE(store.count(), 0);

        store.addCanceledResults<int>(0, 3);
        QCOMPARE(store.count(), 2);  // results at 3 and 4 become available at index 0, 1

        store.addResult(5, &int0);
        QCOMPARE(store.count(), 3);// result 5 becomes available at index 2
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.addResult(1, &int0);
        store.addResult(3, &int0);
        store.addResults(6, &vec0);
        QCOMPARE(store.contains(0), false);
        QCOMPARE(store.contains(1), true);
        QCOMPARE(store.contains(2), false);
        QCOMPARE(store.contains(3), true);
        QCOMPARE(store.contains(4), false);
        QCOMPARE(store.contains(5), false);
        QCOMPARE(store.contains(6), true);
        QCOMPARE(store.contains(7), true);
    }

    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);
        store.addResult(1, &int0);
        store.addResult(3, &int0);
        store.addResults(6, &vec0);
        QCOMPARE(store.contains(0), false);
        QCOMPARE(store.contains(1), false);
        QCOMPARE(store.contains(2), false);
        QCOMPARE(store.contains(3), false);
        QCOMPARE(store.contains(4), false);
        QCOMPARE(store.contains(5), false);
        QCOMPARE(store.contains(6), false);
        QCOMPARE(store.contains(7), false);

        store.addCanceledResult(0);
        store.addCanceledResult(2);
        store.addCanceledResults<int>(4, 2);

        QCOMPARE(store.contains(0), true);
        QCOMPARE(store.contains(1), true);
        QCOMPARE(store.contains(2), true);
        QCOMPARE(store.contains(3), true);
        QCOMPARE(store.contains(4), false);
        QCOMPARE(store.contains(5), false);
        QCOMPARE(store.contains(6), false);
        QCOMPARE(store.contains(7), false);
    }
    {
        QtPrivate::ResultStoreBase store;
        IntResultsCleaner cleanGuard(store);

        store.setFilterMode(true);
        store.addCanceledResult(0);
        QCOMPARE(store.contains(0), false);

        store.addResult(1, &int0);
        QCOMPARE(store.contains(0), true);
        QCOMPARE(store.contains(1), false);
    }
}

void tst_QFuture::future()
{
    // default constructors
    QFuture<int> intFuture;
    intFuture.waitForFinished();
    QFuture<QString> stringFuture;
    stringFuture.waitForFinished();
    QFuture<void> voidFuture;
    voidFuture.waitForFinished();
    QFuture<void> defaultVoidFuture;
    defaultVoidFuture.waitForFinished();

    // copy constructor
    QFuture<int> intFuture2(intFuture);
    QFuture<void> voidFuture2(defaultVoidFuture);

    // assigmnent operator
    intFuture2 = QFuture<int>();
    voidFuture2 = QFuture<void>();

    // state
    QCOMPARE(intFuture2.isStarted(), true);
    QCOMPARE(intFuture2.isFinished(), true);
}

void tst_QFuture::futureToVoid()
{
    QPromise<int> p;
    QFuture<int> future = p.future();

    p.start();
    p.setProgressValue(42);
    p.finish();

    QFuture<void> voidFuture = QFuture<void>(future);
    QCOMPARE(voidFuture.progressValue(), 42);
}

class IntResult : public QFutureInterface<int>
{
public:
    QFuture<int> run()
    {
        this->reportStarted();
        QFuture<int> future = QFuture<int>(this);

        int res = 10;
        reportFinished(&res);
        return future;
    }
};

int value = 10;

class VoidResult : public QFutureInterfaceBase
{
public:
    QFuture<void> run()
    {
        this->reportStarted();
        QFuture<void> future = QFuture<void>(this);
        reportFinished();
        return future;
    }
};

void tst_QFuture::futureInterface()
{
    {
        QFuture<void> future;
        {
            QFutureInterface<void> i;
            i.reportStarted();
            future = i.future();
            i.reportFinished();
        }
    }
    {
        QFuture<int> future;
        {
            QFutureInterface<int> i;
            i.reportStarted();
            QVERIFY(i.reportResult(10));
            future = i.future();
            i.reportFinished();
        }
        QCOMPARE(future.resultAt(0), 10);
    }

    {
        QFuture<int> intFuture;

        QCOMPARE(intFuture.isStarted(), true);
        QCOMPARE(intFuture.isFinished(), true);

        IntResult result;

        result.reportStarted();
        intFuture = result.future();

        QCOMPARE(intFuture.isStarted(), true);
        QCOMPARE(intFuture.isFinished(), false);

        QVERIFY(result.reportFinished(&value));

        QCOMPARE(intFuture.isStarted(), true);
        QCOMPARE(intFuture.isFinished(), true);

        int e = intFuture.result();

        QCOMPARE(intFuture.isStarted(), true);
        QCOMPARE(intFuture.isFinished(), true);
        QCOMPARE(intFuture.isCanceled(), false);

        QCOMPARE(e, value);
        intFuture.waitForFinished();

        IntResult intAlgo;
        intFuture = intAlgo.run();
        QFuture<int> intFuture2(intFuture);
        QCOMPARE(intFuture.result(), value);
        QCOMPARE(intFuture2.result(), value);
        intFuture.waitForFinished();

        VoidResult a;
        a.run().waitForFinished();
    }

    {
        QFutureInterface<int> i1;
        QVERIFY(i1.reportResult(1));
        QFutureInterface<int> i2;
        QVERIFY(i2.reportResult(2));
        swap(i1, i2);  // ADL must resolve this
        QCOMPARE(i1.resultReference(0), 2);
        QCOMPARE(i2.resultReference(0), 1);
    }

    {
        QFutureInterface<int> fi;
        fi.reportStarted();
        QVERIFY(!fi.reportResults(QList<int> {}));
        fi.reportFinished();

        QVERIFY(fi.results().empty());
    }

    {
        QFutureInterface<int> fi;
        fi.reportStarted();
        QList<int> values = { 1, 2, 3 };
        QVERIFY(fi.reportResults(values));
        QVERIFY(!fi.reportResults(QList<int> {}));
        fi.reportFinished();

        QCOMPARE(fi.results(), values);
    }
}

template <typename T>
void testRefCounting()
{
    QFutureInterface<T> interface;
    QCOMPARE(interface.d->refCount.load(), 1);

    {
        interface.reportStarted();

        QFuture<T> f = interface.future();
        QCOMPARE(interface.d->refCount.load(), 2);

        QFuture<T> f2(f);
        QCOMPARE(interface.d->refCount.load(), 3);

        QFuture<T> f3;
        f3 = f2;
        QCOMPARE(interface.d->refCount.load(), 4);

        interface.reportFinished(0);
        QCOMPARE(interface.d->refCount.load(), 4);
    }

    QCOMPARE(interface.d->refCount.load(), 1);
}

void tst_QFuture::refcounting()
{
    testRefCounting<int>();
}

void tst_QFuture::cancel()
{
    {
        QFuture<void> f;
        QFutureInterface<void> result;

        result.reportStarted();
        f = result.future();
        QVERIFY(!f.isCanceled());
        result.reportCanceled();
        QVERIFY(f.isCanceled());
        result.reportFinished();
        QVERIFY(f.isCanceled());
        f.waitForFinished();
        QVERIFY(f.isCanceled());
    }

    // Cancel from the QFuture side and test if the result
    // interface detects it.
    {
        QFutureInterface<void> result;

        QFuture<void> f;
        QVERIFY(f.isStarted());

        result.reportStarted();
        f = result.future();

        QVERIFY(f.isStarted());

        QVERIFY(!result.isCanceled());
        f.cancel();

        QVERIFY(result.isCanceled());

        result.reportFinished();
    }

    // Test that finished futures can be canceled.
    {
        QFutureInterface<void> result;

        QFuture<void> f;
        QVERIFY(f.isStarted());

        result.reportStarted();
        f = result.future();

        QVERIFY(f.isStarted());

        result.reportFinished();

        f.cancel();

        QVERIFY(result.isCanceled());
        QVERIFY(f.isCanceled());
    }

    // Results reported after canceled is called should not be propagated.
    {

        QFutureInterface<int> futureInterface;
        futureInterface.reportStarted();
        QFuture<int> f = futureInterface.future();

        int result = 0;
        futureInterface.reportResult(&result);
        result = 1;
        futureInterface.reportResult(&result);
        f.cancel();
        result = 2;
        futureInterface.reportResult(&result);
        result = 3;
        futureInterface.reportResult(&result);
        futureInterface.reportFinished();
        QVERIFY(f.results().isEmpty());
    }
}

void tst_QFuture::cancelAndFinish()
{
    {
        QFutureInterface<void> fi;

        fi.reportStarted();
        fi.cancelAndFinish();

        QVERIFY(fi.isStarted());
        QVERIFY(!fi.isRunning());
        QVERIFY(!fi.isSuspended());
        QVERIFY(!fi.isSuspending());
        QVERIFY(fi.isCanceled());
        QVERIFY(fi.isFinished());
    }

    // The same with suspended state
    {
        QFutureInterface<void> fi;

        fi.reportStarted();
        fi.setSuspended(true);
        fi.cancelAndFinish();

        QVERIFY(fi.isStarted());
        QVERIFY(!fi.isRunning());
        QVERIFY(!fi.isSuspended());
        QVERIFY(!fi.isSuspending());
        QVERIFY(fi.isCanceled());
        QVERIFY(fi.isFinished());
    }
}

void tst_QFuture::statePropagation()
{
    QFuture<void> f1;
    QFuture<void> f2;

    QCOMPARE(f1.isStarted(), true);

    QFutureInterface<void> result;
    result.reportStarted();
    f1 = result.future();

    f2 = f1;

    QCOMPARE(f2.isStarted(), true);

    result.reportCanceled();

    QCOMPARE(f2.isStarted(), true);
    QCOMPARE(f2.isCanceled(), true);

    QFuture<void> f3 = f2;

    QCOMPARE(f3.isStarted(), true);
    QCOMPARE(f3.isCanceled(), true);

    result.reportFinished();

    QCOMPARE(f2.isStarted(), true);
    QCOMPARE(f2.isCanceled(), true);

    QCOMPARE(f3.isStarted(), true);
    QCOMPARE(f3.isCanceled(), true);
}

/*
    Tests that a QFuture can return multiple results.
*/
void tst_QFuture::multipleResults()
{
    IntResult a;
    a.reportStarted();
    QFuture<int> f = a.future();

    QFuture<int> copy = f;
    int result;

    result = 1;
    QVERIFY(a.reportResult(&result));
    QCOMPARE(f.resultAt(0), 1);

    result = 2;
    QVERIFY(a.reportResult(&result));
    QCOMPARE(f.resultAt(1), 2);

    result = 3;
    QVERIFY(a.reportResult(&result));

    result = 4;
    a.reportFinished(&result);

    QCOMPARE(f.results(), QList<int>() << 1 << 2 << 3 << 4);

    // test foreach
    QList<int> fasit = QList<int>() << 1 << 2 << 3 << 4;
    {
        QList<int> results;
        for (int result : std::as_const(f))
            results.append(result);
        QCOMPARE(results, fasit);
    }
    {
        QList<int> results;
        for (int result : std::as_const(copy))
            results.append(result);
        QCOMPARE(results, fasit);
    }
}

/*
    Test out-of-order result reporting using indexes
*/
void tst_QFuture::indexedResults()
{
    {
        QFutureInterface<QChar> Interface;
        QFuture<QChar> f;
        QVERIFY(f.isStarted());

        Interface.reportStarted();
        f = Interface.future();

        QVERIFY(f.isStarted());

        QChar result;

        result = 'B';
        QVERIFY(Interface.reportResult(&result, 1));

        QCOMPARE(f.resultAt(1), result);

        result = 'A';
        QVERIFY(Interface.reportResult(&result, 0));
        QCOMPARE(f.resultAt(0), result);

        result = 'C';
        QVERIFY(Interface.reportResult(&result)); // no index
        QCOMPARE(f.resultAt(2), result);

        Interface.reportFinished();

        QCOMPARE(f.results(), QList<QChar>() << 'A' << 'B' << 'C');
    }

    {
        // Test result reporting with a missing result in the middle
        QFutureInterface<int> Interface;
        Interface.reportStarted();
        QFuture<int> f = Interface.future();
        int result;

        result = 0;
        QVERIFY(Interface.reportResult(&result, 0));
        QVERIFY(f.isResultReadyAt(0));
        QCOMPARE(f.resultAt(0), 0);

        result = 3;
        QVERIFY(Interface.reportResult(&result, 3));
        QVERIFY(f.isResultReadyAt(3));
        QCOMPARE(f.resultAt(3), 3);

        result = 2;
        QVERIFY(Interface.reportResult(&result, 2));
        QVERIFY(f.isResultReadyAt(2));
        QCOMPARE(f.resultAt(2), 2);

        result = 4;
        QVERIFY(Interface.reportResult(&result)); // no index
        QVERIFY(f.isResultReadyAt(4));
        QCOMPARE(f.resultAt(4), 4);

        Interface.reportFinished();

        QCOMPARE(f.results(), QList<int>() << 0 << 2 << 3 << 4);
    }
}

void tst_QFuture::progress()
{
    QFutureInterface<QChar> result;
    QFuture<QChar> f;

    QCOMPARE (f.progressValue(), 0);

    result.reportStarted();
    f = result.future();

    QCOMPARE (f.progressValue(), 0);

    result.setProgressValue(50);

    QCOMPARE (f.progressValue(), 50);

    result.reportFinished();

    QCOMPARE (f.progressValue(), 50);
}

void tst_QFuture::setProgressRange()
{
    QFutureInterface<int> i;

    QCOMPARE(i.progressMinimum(), 0);
    QCOMPARE(i.progressMaximum(), 0);

    i.setProgressRange(10, 5);

    QCOMPARE(i.progressMinimum(), 10);
    QCOMPARE(i.progressMaximum(), 10);

    i.setProgressRange(5, 10);

    QCOMPARE(i.progressMinimum(), 5);
    QCOMPARE(i.progressMaximum(), 10);
}

void tst_QFuture::progressWithRange()
{
    QFutureInterface<int> i;
    QFuture<int> f;

    i.reportStarted();
    f = i.future();

    QCOMPARE(i.progressValue(), 0);

    i.setProgressRange(5, 10);

    QCOMPARE(i.progressValue(), 5);

    i.setProgressValue(20);

    QCOMPARE(i.progressValue(), 5);

    i.setProgressValue(9);

    QCOMPARE(i.progressValue(), 9);

    i.setProgressRange(5, 7);

    QCOMPARE(i.progressValue(), 5);

    i.reportFinished();

    QCOMPARE(f.progressValue(), 5);
}

void tst_QFuture::progressText()
{
    QFutureInterface<void> i;
    i.reportStarted();
    QFuture<void> f = i.future();

    QCOMPARE(f.progressText(), QLatin1String(""));
    i.setProgressValueAndText(1, QLatin1String("foo"));
    QCOMPARE(f.progressText(), QLatin1String("foo"));
    i.reportFinished();
}

/*
    Test that results reported after finished are ignored.
*/
void tst_QFuture::resultsAfterFinished()
{
    {
        IntResult a;
        a.reportStarted();
        QFuture<int> f =  a.future();
        int result;

        QCOMPARE(f.resultCount(), 0);

        result = 1;
        QVERIFY(a.reportResult(&result));
        QCOMPARE(f.resultAt(0), 1);

        a.reportFinished();

        QCOMPARE(f.resultAt(0), 1);
        QCOMPARE(f.resultCount(), 1);
        result = 2;
        QVERIFY(!a.reportResult(&result));
        QCOMPARE(f.resultCount(), 1);
    }
    // cancel it
    {
        IntResult a;
        a.reportStarted();
        QFuture<int> f =  a.future();
        int result;

        QCOMPARE(f.resultCount(), 0);

        result = 1;
        QVERIFY(a.reportResult(&result));
        QCOMPARE(f.resultAt(0), 1);
        QCOMPARE(f.resultCount(), 1);

        a.reportCanceled();

        QCOMPARE(f.resultAt(0), 1);
        QCOMPARE(f.resultCount(), 1);

        result = 2;
        QVERIFY(!a.reportResult(&result));
        a.reportFinished();
    }
}

void tst_QFuture::resultsAsList()
{
    IntResult a;
    a.reportStarted();
    QFuture<int> f = a.future();

    int result;
    result = 1;
    QVERIFY(a.reportResult(&result));
    result = 2;
    QVERIFY(a.reportResult(&result));

    a.reportFinished();

    QList<int> results = f.results();
    QCOMPARE(results, QList<int>() << 1 << 2);
}

void tst_QFuture::iterators()
{
    {
        QFutureInterface<int> e;
        e.reportStarted();
        QFuture<int> f = e.future();

        int result;
        result = 1;
        e.reportResult(&result);
        result = 2;
        e.reportResult(&result);
        result = 3;
        e.reportResult(&result);
        e.reportFinished();

        QList<int> results;
        QFutureIterator<int> i(f);
        while (i.hasNext()) {
            results.append(i.next());
        }

        QCOMPARE(results, f.results());

        QFuture<int>::const_iterator i1 = f.begin(), i2 = i1 + 1;
        QFuture<int>::const_iterator c1 = i1, c2 = c1 + 1;

        QCOMPARE(i1, i1);
        QCOMPARE(i1, c1);
        QCOMPARE(c1, i1);
        QCOMPARE(c1, c1);
        QCOMPARE(i2, i2);
        QCOMPARE(i2, c2);
        QCOMPARE(c2, i2);
        QCOMPARE(c2, c2);
        QCOMPARE(1 + i1, i1 + 1);
        QCOMPARE(1 + c1, c1 + 1);

        QVERIFY(i1 != i2);
        QVERIFY(i1 != c2);
        QVERIFY(c1 != i2);
        QVERIFY(c1 != c2);
        QVERIFY(i2 != i1);
        QVERIFY(i2 != c1);
        QVERIFY(c2 != i1);
        QVERIFY(c2 != c1);

        int x1 = *i1;
        Q_UNUSED(x1)
        int x2 = *i2;
        Q_UNUSED(x2)
        int y1 = *c1;
        Q_UNUSED(y1)
        int y2 = *c2;
        Q_UNUSED(y2)
    }

    {
        QFutureInterface<QString> e;
        e.reportStarted();
        QFuture<QString> f =  e.future();

        e.reportResult(QString("one"));
        e.reportResult(QString("two"));
        e.reportResult(QString("three"));
        e.reportFinished();

        QList<QString> results;
        QFutureIterator<QString> i(f);
        while (i.hasNext()) {
            results.append(i.next());
        }

        QCOMPARE(results, f.results());

        QFuture<QString>::const_iterator i1 = f.begin(), i2 = i1 + 1;
        QFuture<QString>::const_iterator c1 = i1, c2 = c1 + 1;

        QCOMPARE(i1, i1);
        QCOMPARE(i1, c1);
        QCOMPARE(c1, i1);
        QCOMPARE(c1, c1);
        QCOMPARE(i2, i2);
        QCOMPARE(i2, c2);
        QCOMPARE(c2, i2);
        QCOMPARE(c2, c2);
        QCOMPARE(1 + i1, i1 + 1);
        QCOMPARE(1 + c1, c1 + 1);

        QVERIFY(i1 != i2);
        QVERIFY(i1 != c2);
        QVERIFY(c1 != i2);
        QVERIFY(c1 != c2);
        QVERIFY(i2 != i1);
        QVERIFY(i2 != c1);
        QVERIFY(c2 != i1);
        QVERIFY(c2 != c1);

        QString x1 = *i1;
        QString x2 = *i2;
        QString y1 = *c1;
        QString y2 = *c2;

        QCOMPARE(x1, y1);
        QCOMPARE(x2, y2);

        auto i1Size = i1->size();
        auto i2Size = i2->size();
        auto c1Size = c1->size();
        auto c2Size = c2->size();

        QCOMPARE(i1Size, c1Size);
        QCOMPARE(i2Size, c2Size);
    }

    {
        const int resultCount = 20;

        QFutureInterface<int> e;
        e.reportStarted();
        QFuture<int> f =  e.future();

        for (int i = 0; i < resultCount; ++i) {
            e.reportResult(i);
        }

        e.reportFinished();

        {
            QFutureIterator<int> it(f);
            QFutureIterator<int> it2(it);
        }

        {
            QFutureIterator<int> it(f);

            for (int i = 0; i < resultCount - 1; ++i) {
                QVERIFY(it.hasNext());
                QCOMPARE(it.peekNext(), i);
                QCOMPARE(it.next(), i);
            }

            QVERIFY(it.hasNext());
            QCOMPARE(it.peekNext(), resultCount - 1);
            QCOMPARE(it.next(), resultCount - 1);
            QVERIFY(!it.hasNext());
        }

        {
            QFutureIterator<int> it(f);
            QVERIFY(it.hasNext());
            it.toBack();
            QVERIFY(!it.hasNext());
            it.toFront();
            QVERIFY(it.hasNext());
        }
    }
}
void tst_QFuture::iteratorsThread()
{
    const int expectedResultCount = 10;
    QFutureInterface<int> futureInterface;

    // Create result producer thread. The results are
    // produced with delays in order to make the consumer
    // wait.
    QSemaphore sem;
    LambdaThread thread = {[=, &futureInterface, &sem](){
        for (int i = 1; i <= expectedResultCount; i += 2) {
            int result = i;
            futureInterface.reportResult(&result);
            result = i + 1;
            futureInterface.reportResult(&result);
        }

        sem.acquire(2);
        futureInterface.reportFinished();
    }};

    futureInterface.reportStarted();
    QFuture<int> future = futureInterface.future();

    // Iterate over results while the thread is producing them.
    thread.start();
    int resultCount = 0;
    int resultSum = 0;
    for (int result : future) {
        sem.release();
        ++resultCount;
        resultSum += result;
    }
    thread.wait();

    QCOMPARE(resultCount, expectedResultCount);
    QCOMPARE(resultSum, expectedResultCount * (expectedResultCount + 1) / 2);

    // Reverse iterate
    resultSum = 0;
    QFutureIterator<int> it(future);
    it.toBack();
    while (it.hasPrevious())
        resultSum += it.previous();

    QCOMPARE(resultSum, expectedResultCount * (expectedResultCount + 1) / 2);
}

class SignalSlotObject : public QObject
{
Q_OBJECT
public:
    SignalSlotObject()
    : finishedCalled(false),
      canceledCalled(false),
      rangeBegin(0),
      rangeEnd(0) { }

public slots:
    void finished()
    {
        finishedCalled = true;
    }

    void canceled()
    {
        canceledCalled = true;
    }

    void resultReady(int index)
    {
        results.insert(index);
    }

    void progressRange(int begin, int end)
    {
        rangeBegin = begin;
        rangeEnd = end;
    }

    void progress(int progress)
    {
        reportedProgress.insert(progress);
    }
public:
    bool finishedCalled;
    bool canceledCalled;
    QSet<int> results;
    int rangeBegin;
    int rangeEnd;
    QSet<int> reportedProgress;
};

#if QT_DEPRECATED_SINCE(6, 0)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
void tst_QFuture::pause()
{
    QFutureInterface<void> Interface;

    Interface.reportStarted();
    QFuture<void> f = Interface.future();

    QVERIFY(!Interface.isPaused());
    f.pause();
    QVERIFY(Interface.isPaused());
    f.resume();
    QVERIFY(!Interface.isPaused());
    f.togglePaused();
    QVERIFY(Interface.isPaused());
    f.togglePaused();
    QVERIFY(!Interface.isPaused());

    Interface.reportFinished();
}

void tst_QFuture::suspendCheckPaused()
{
    QFutureInterface<void> interface;

    interface.reportStarted();
    QFuture<void> f = interface.future();
    QVERIFY(!f.isSuspended());

    interface.reportSuspended();
    QVERIFY(!f.isSuspended());

    f.pause();
    QVERIFY(!f.isSuspended());
    QVERIFY(f.isPaused());

    // resume when still pausing
    f.resume();
    QVERIFY(!f.isSuspended());
    QVERIFY(!f.isPaused());

    // pause again
    f.pause();
    QVERIFY(!f.isSuspended());
    QVERIFY(f.isPaused());

    interface.reportSuspended();
    QVERIFY(f.isSuspended());
    QVERIFY(f.isPaused());

    // resume after suspended
    f.resume();
    QVERIFY(!f.isSuspended());
    QVERIFY(!f.isPaused());

    // pause again and cancel
    f.pause();
    interface.reportSuspended();

    interface.reportCanceled();
    QVERIFY(!f.isSuspended());
    QVERIFY(!f.isPaused());
    QVERIFY(f.isCanceled());

    interface.reportFinished();
}

QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 0)

void tst_QFuture::suspend()
{
    QFutureInterface<void> interface;

    interface.reportStarted();
    QFuture<void> f = interface.future();
    QVERIFY(!f.isSuspended());

    interface.reportSuspended();
    QVERIFY(!f.isSuspended());
    QVERIFY(!f.isSuspending());

    f.suspend();
    QVERIFY(f.isSuspending());
    QVERIFY(!f.isSuspended());

    // resume when still suspending
    f.resume();
    QVERIFY(!f.isSuspending());
    QVERIFY(!f.isSuspended());

    // suspend again
    f.suspend();
    QVERIFY(f.isSuspending());
    QVERIFY(!f.isSuspended());

    interface.reportSuspended();
    QVERIFY(!f.isSuspending());
    QVERIFY(f.isSuspended());

    // resume after suspended
    f.resume();
    QVERIFY(!f.isSuspending());
    QVERIFY(!f.isSuspended());

    // suspend again and cancel
    f.suspend();
    interface.reportSuspended();

    interface.reportCanceled();
    QVERIFY(!f.isSuspending());
    QVERIFY(!f.isSuspended());
    QVERIFY(f.isCanceled());

    interface.reportFinished();
}

class ResultObject : public QObject
{
Q_OBJECT
public slots:
    void resultReady(int)
    {

    }
public:
};

// Test that that the isPaused() on future result interface returns true
// if we report a lot of results that are not handled.
void tst_QFuture::throttling()
{
    {
        QFutureInterface<void> i;

        i.reportStarted();
        QFuture<void> f = i.future();

        QVERIFY(!i.isThrottled());

        i.setThrottled(true);
        QVERIFY(i.isThrottled());

        i.setThrottled(false);
        QVERIFY(!i.isThrottled());

        i.setThrottled(true);
        QVERIFY(i.isThrottled());

        i.reportFinished();
    }
}

void tst_QFuture::voidConversions()
{
    {
        QFutureInterface<int> iface;
        iface.reportStarted();

        QFuture<int> intFuture(&iface);
        int value = 10;
        QVERIFY(iface.reportFinished(&value));

        QFuture<void> voidFuture(intFuture);
        voidFuture = intFuture;
    }

    {
        QFuture<void> voidFuture;
        {
            QFutureInterface<QList<int> > iface;
            iface.reportStarted();

            QFuture<QList<int> > listFuture(&iface);
            QVERIFY(iface.reportResult(QList<int>() << 1 << 2 << 3));
            voidFuture = listFuture;
        }
        QCOMPARE(voidFuture.resultCount(), 0);
    }
}


#ifndef QT_NO_EXCEPTIONS

QFuture<void> createExceptionFuture()
{
    QFutureInterface<void> i;
    i.reportStarted();
    QFuture<void> f = i.future();

    QException e;
    i.reportException(e);
    i.reportFinished();
    return f;
}

QFuture<int> createExceptionResultFuture()
{
    QFutureInterface<int> i;
    i.reportStarted();
    QFuture<int> f = i.future();
    int r = 0;
    i.reportResult(r);

    QException e;
    i.reportException(e);
    i.reportFinished();
    return f;
}

class DerivedException : public QException
{
public:
    void raise() const override { throw *this; }
    DerivedException *clone() const override { return new DerivedException(*this); }
};

QFuture<void> createDerivedExceptionFuture()
{
    QFutureInterface<void> i;
    i.reportStarted();
    QFuture<void> f = i.future();

    DerivedException e;
    i.reportException(e);
    i.reportFinished();
    return f;
}

struct TestException
{
};

QFuture<int> createCustomExceptionFuture()
{
    QFutureInterface<int> i;
    i.reportStarted();
    QFuture<int> f = i.future();
    int r = 0;
    i.reportResult(r);
    auto exception = std::make_exception_ptr(TestException());
    i.reportException(exception);
    i.reportFinished();
    return f;
}

void tst_QFuture::exceptions()
{
    // test throwing from waitForFinished
    {
        QFuture<void> f = createExceptionFuture();
        bool caught = false;
        try {
            f.waitForFinished();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // test result()
    {
        QFuture<int> f = createExceptionResultFuture();
        bool caught = false;
        try {
            f.result();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // test result() and destroy
    {
        bool caught = false;
        try {
            createExceptionResultFuture().result();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // test results()
    {
        QFuture<int> f = createExceptionResultFuture();
        bool caught = false;
        try {
            f.results();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // test foreach
    {
        QFuture<int> f = createExceptionResultFuture();
        bool caught = false;
        try {
            foreach (int e, f.results()) {
                Q_UNUSED(e)
                QFAIL("did not get exception");
            }
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // catch derived exceptions
    {
        bool caught = false;
        try {
            createDerivedExceptionFuture().waitForFinished();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    {
        bool caught = false;
        try {
            createDerivedExceptionFuture().waitForFinished();
        } catch (DerivedException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // Custom exceptions
    {
        QFuture<int> f = createCustomExceptionFuture();
        bool caught = false;
        try {
            f.result();
        } catch (const TestException &) {
            caught = true;
        }
        QVERIFY(caught);
    }
}

class MyClass
{
public:
    ~MyClass()
    {
        QFuture<void> f = createExceptionFuture();
        try {
            f.waitForFinished();
        } catch (QException &) {
            caught = true;
        }
    }
    static bool caught;
};

bool MyClass::caught = false;

// This is a regression test for QTBUG-18149. where QFuture did not throw
// exceptions if called from destructors when the stack was already unwinding
// due to an exception having been thrown.
void tst_QFuture::nestedExceptions()
{
    try {
        MyClass m;
        Q_UNUSED(m)
        throw 0;
    } catch (int) {}

    QVERIFY(MyClass::caught);
}

#endif // QT_NO_EXCEPTIONS

void tst_QFuture::nonGlobalThreadPool()
{
    static constexpr int Answer = 42;

    struct UselessTask : QRunnable, QFutureInterface<int>
    {
        QFuture<int> start(QThreadPool *pool)
        {
            setRunnable(this);
            setThreadPool(pool);
            reportStarted();
            QFuture<int> f = future();
            pool->start(this);
            return f;
        }

        void run() override
        {
            const int ms = 100 + (QRandomGenerator::global()->bounded(100) - 100/2);
            QThread::sleep(std::chrono::milliseconds{ms});
            reportResult(Answer);
            reportFinished();
        }
    };

    QThreadPool pool;

    const int numTasks = QThread::idealThreadCount();

    QList<QFuture<int>> futures;
    futures.reserve(numTasks);

    for (int i = 0; i < numTasks; ++i)
        futures.push_back((new UselessTask)->start(&pool));

    QVERIFY(!pool.waitForDone(0)); // pool is busy (meaning our tasks did end up executing there)

    QVERIFY(pool.waitForDone(10000)); // max sleep time in UselessTask::run is 150ms, so 10s should be enough
                                      // (and the call returns as soon as all tasks finished anyway, so the
                                      // maximum wait time only matters when the test fails)

    Q_FOREACH (const QFuture<int> &future, futures) {
        QVERIFY(future.isFinished());
        QCOMPARE(future.result(), Answer);
    }
}

void tst_QFuture::then()
{
    {
        struct Add
        {

            static int addTwo(int arg) { return arg + 2; }

            int operator()(int arg) const { return arg + 3; }
        };

        QFutureInterface<int> promise;
        QFuture<int> then = promise.future()
                                    .then([](int res) { return res + 1; }) // lambda
                                    .then(Add::addTwo) // function
                                    .then(Add()); // functor

        promise.reportStarted();
        QVERIFY(!then.isStarted());
        QVERIFY(!then.isFinished());

        const int result = 0;
        promise.reportResult(result);
        promise.reportFinished();

        then.waitForFinished();

        QVERIFY(then.isStarted());
        QVERIFY(then.isFinished());
        QCOMPARE(then.result(), result + 6);
    }

    // then() on a ready future
    {
        QFutureInterface<int> promise;
        promise.reportStarted();

        const int result = 0;
        promise.reportResult(result);
        promise.reportFinished();

        QFuture<int> then = promise.future()
                                    .then([](int res1) { return res1 + 1; })
                                    .then([](int res2) { return res2 + 2; })
                                    .then([](int res3) { return res3 + 3; });

        then.waitForFinished();

        QVERIFY(then.isStarted());
        QVERIFY(then.isFinished());
        QCOMPARE(then.result(), result + 6);
    }

    // Continuation of QFuture<void>
    {
        int result = 0;
        QFutureInterface<void> promise;
        QFuture<void> then = promise.future()
                                     .then([&]() { result += 1; })
                                     .then([&]() { result += 2; })
                                     .then([&]() { result += 3; });

        promise.reportStarted();
        QVERIFY(!then.isStarted());
        QVERIFY(!then.isFinished());
        promise.reportFinished();

        then.waitForFinished();

        QVERIFY(then.isStarted());
        QVERIFY(then.isFinished());
        QCOMPARE(result, 6);
    }

    // Continuation returns QFuture<void>
    {
        QFutureInterface<int> promise;
        int value;
        QFuture<void> then =
                promise.future().then([](int res) { return res * 2; }).then([&](int prevResult) {
                    value = prevResult;
                });

        promise.reportStarted();
        QVERIFY(!then.isStarted());
        QVERIFY(!then.isFinished());

        const int result = 5;
        promise.reportResult(result);
        promise.reportFinished();

        then.waitForFinished();

        QVERIFY(then.isStarted());
        QVERIFY(then.isFinished());
        QCOMPARE(value, result * 2);
    }

    // Continuations taking a QFuture argument.
    {
        int value = 0;
        QFutureInterface<int> promise;
        QFuture<void> then = promise.future()
                                     .then([](QFuture<int> f1) { return f1.result() + 1; })
                                     .then([&](QFuture<int> f2) { value = f2.result() + 2; })
                                     .then([&](QFuture<void> f3) {
                                         QVERIFY(f3.isFinished());
                                         value += 3;
                                     });

        promise.reportStarted();
        QVERIFY(!then.isStarted());
        QVERIFY(!then.isFinished());

        const int result = 0;
        promise.reportResult(result);
        promise.reportFinished();

        then.waitForFinished();

        QVERIFY(then.isStarted());
        QVERIFY(then.isFinished());
        QCOMPARE(value, 6);
    }

    // Continuations use a new thread
    {
        Qt::HANDLE threadId1 = nullptr;
        Qt::HANDLE threadId2 = nullptr;
        QFutureInterface<void> promise;
        QFuture<void> then = promise.future()
                                     .then(QtFuture::Launch::Async,
                                           [&]() { threadId1 = QThread::currentThreadId(); })
                                     .then([&]() { threadId2 = QThread::currentThreadId(); });

        promise.reportStarted();
        QVERIFY(!then.isStarted());
        QVERIFY(!then.isFinished());

        promise.reportFinished();

        then.waitForFinished();

        QVERIFY(then.isStarted());
        QVERIFY(then.isFinished());
        QVERIFY(threadId1 != QThread::currentThreadId());
        QVERIFY(threadId2 != QThread::currentThreadId());
        QVERIFY(threadId1 == threadId2);
    }

    // Continuation inherits the launch policy of its parent (QtFuture::Launch::Sync)
    {
        Qt::HANDLE threadId1 = nullptr;
        Qt::HANDLE threadId2 = nullptr;
        QFutureInterface<void> promise;
        QFuture<void> then = promise.future()
                                     .then(QtFuture::Launch::Sync,
                                           [&]() { threadId1 = QThread::currentThreadId(); })
                                     .then(QtFuture::Launch::Inherit,
                                           [&]() { threadId2 = QThread::currentThreadId(); });

        promise.reportStarted();
        QVERIFY(!then.isStarted());
        QVERIFY(!then.isFinished());

        promise.reportFinished();

        then.waitForFinished();

        QVERIFY(then.isStarted());
        QVERIFY(then.isFinished());
        QVERIFY(threadId1 == QThread::currentThreadId());
        QVERIFY(threadId2 == QThread::currentThreadId());
        QVERIFY(threadId1 == threadId2);
    }

    // Continuation inherits the launch policy of its parent (QtFuture::Launch::Async)
    {
        Qt::HANDLE threadId1 = nullptr;
        Qt::HANDLE threadId2 = nullptr;
        QFutureInterface<void> promise;
        QFuture<void> then = promise.future()
                                     .then(QtFuture::Launch::Async,
                                           [&]() { threadId1 = QThread::currentThreadId(); })
                                     .then(QtFuture::Launch::Inherit,
                                           [&]() { threadId2 = QThread::currentThreadId(); });

        promise.reportStarted();
        QVERIFY(!then.isStarted());
        QVERIFY(!then.isFinished());

        promise.reportFinished();

        then.waitForFinished();

        QVERIFY(then.isStarted());
        QVERIFY(then.isFinished());
        QVERIFY(threadId1 != QThread::currentThreadId());
        QVERIFY(threadId2 != QThread::currentThreadId());
    }

    // Continuations use a custom thread pool
    {
        QFutureInterface<void> promise;
        QThreadPool pool;
        QVERIFY(pool.waitForDone(0)); // pool is not busy yet
        QSemaphore semaphore;
        QFuture<void> then = promise.future().then(&pool, [&]() { semaphore.acquire(); });

        promise.reportStarted();
        promise.reportFinished();

        // Make sure the custom thread pool is busy on running the continuation
        QVERIFY(!pool.waitForDone(0));
        semaphore.release();
        then.waitForFinished();

        QVERIFY(then.isStarted());
        QVERIFY(then.isFinished());
        QCOMPARE(then.d.threadPool(), &pool);
    }

    // Continuation inherits parent's thread pool
    {
        Qt::HANDLE threadId1 = nullptr;
        Qt::HANDLE threadId2 = nullptr;
        QFutureInterface<void> promise;

        QThreadPool pool;
        QFuture<void> then1 = promise.future().then(&pool, [&]() {
            threadId1 = QThread::currentThreadId();
        });

        promise.reportStarted();
        promise.reportFinished();

        then1.waitForFinished();
        QVERIFY(pool.waitForDone()); // The pool is not busy after the first continuation is done

        QSemaphore semaphore;
        QFuture<void> then2 = then1.then(QtFuture::Launch::Inherit, [&]() {
            semaphore.acquire();
            threadId2 = QThread::currentThreadId();
        });

        QVERIFY(!pool.waitForDone(0)); // The pool is busy running the 2nd continuation

        semaphore.release();
        then2.waitForFinished();

        QVERIFY(then2.isStarted());
        QVERIFY(then2.isFinished());
        QCOMPARE(then1.d.threadPool(), then2.d.threadPool());
        QCOMPARE(then2.d.threadPool(), &pool);
        QVERIFY(threadId1 != QThread::currentThreadId());
        QVERIFY(threadId2 != QThread::currentThreadId());
    }

    // QTBUG-106083 & QTBUG-105182
    {
        QThread thread;
        thread.start();

        QObject context;
        context.moveToThread(&thread);

        auto future = QtConcurrent::run([] {
            return 42;
        }).then([] (int result) {
            return result + 1;
        }).then(&context, [] (int result) {
            return result + 1;
        });
        QCOMPARE(future.result(), 44);
        thread.quit();
        thread.wait();
    }
}

template<class Type, class Callable>
bool runThenForMoveOnly(Callable &&callable)
{
    QFutureInterface<Type> promise;
    auto future = promise.future();

    auto then = future.then(std::forward<Callable>(callable));

    promise.reportStarted();
    if constexpr (!std::is_same_v<Type, void>)
        promise.reportAndMoveResult(std::make_unique<int>(42));
    promise.reportFinished();
    then.waitForFinished();

    bool success = true;
    if constexpr (!std::is_same_v<decltype(then), QFuture<void>>)
        success &= *then.takeResult() == 42;

    if constexpr (!std::is_same_v<Type, void>)
        success &= !future.isValid();

    return success;
}

void tst_QFuture::thenForMoveOnlyTypes()
{
    QVERIFY(runThenForMoveOnly<UniquePtr>([](UniquePtr res) { return res; }));
    QVERIFY(runThenForMoveOnly<UniquePtr>([](UniquePtr res) { Q_UNUSED(res); }));
    QVERIFY(runThenForMoveOnly<UniquePtr>([](QFuture<UniquePtr> res) { return res.takeResult(); }));
    QVERIFY(runThenForMoveOnly<void>([] { return std::make_unique<int>(42); }));
}

template<class T>
QFuture<T> createCanceledFuture()
{
    QFutureInterface<T> promise;
    promise.reportStarted();
    promise.reportCanceled();
    promise.reportFinished();
    return promise.future();
}

void tst_QFuture::thenOnCanceledFuture()
{
    // Continuations on a canceled future
    {
        int thenResult = 0;
        QFuture<void> then = createCanceledFuture<void>().then([&]() { ++thenResult; }).then([&]() {
            ++thenResult;
        });

        QVERIFY(then.isCanceled());
        QCOMPARE(thenResult, 0);
    }

    // QFuture gets canceled after continuations are set
    {
        QFutureInterface<void> promise;

        int thenResult = 0;
        QFuture<void> then =
                promise.future().then([&]() { ++thenResult; }).then([&]() { ++thenResult; });

        promise.reportStarted();
        promise.reportCanceled();
        promise.reportFinished();

        QVERIFY(then.isCanceled());
        QCOMPARE(thenResult, 0);
    }

    // Same with QtFuture::Launch::Async

    // Continuations on a canceled future
    {
        int thenResult = 0;
        QFuture<void> then = createCanceledFuture<void>()
                                     .then(QtFuture::Launch::Async, [&]() { ++thenResult; })
                                     .then([&]() { ++thenResult; });

        QVERIFY(then.isCanceled());
        QCOMPARE(thenResult, 0);
    }

    // QFuture gets canceled after continuations are set
    {
        QFutureInterface<void> promise;

        int thenResult = 0;
        QFuture<void> then =
                promise.future().then(QtFuture::Launch::Async, [&]() { ++thenResult; }).then([&]() {
                    ++thenResult;
                });

        promise.reportStarted();
        promise.reportCanceled();
        promise.reportFinished();

        QVERIFY(then.isCanceled());
        QCOMPARE(thenResult, 0);
    }
}

#ifndef QT_NO_EXCEPTIONS
void tst_QFuture::thenOnExceptionFuture()
{
    {
        QFutureInterface<int> promise;

        int thenResult = 0;
        QFuture<void> then = promise.future().then([&](int res) { thenResult = res; });

        promise.reportStarted();
        QException e;
        promise.reportException(e);
        promise.reportFinished();

        bool caught = false;
        try {
            then.waitForFinished();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
        QCOMPARE(thenResult, 0);
    }

    // Exception handled inside the continuation
    {
        QFutureInterface<int> promise;

        bool caught = false;
        bool caughtByContinuation = false;
        bool success = false;
        int thenResult = 0;
        QFuture<void> then = promise.future()
                                     .then([&](QFuture<int> res) {
                                         try {
                                             thenResult = res.result();
                                         } catch (QException &) {
                                             caughtByContinuation = true;
                                         }
                                     })
                                     .then([&]() { success = true; });

        promise.reportStarted();
        QException e;
        promise.reportException(e);
        promise.reportFinished();

        try {
            then.waitForFinished();
        } catch (QException &) {
            caught = true;
        }

        QCOMPARE(thenResult, 0);
        QVERIFY(!caught);
        QVERIFY(caughtByContinuation);
        QVERIFY(success);
    }

    // Exception future
    {
        QFutureInterface<int> promise;
        promise.reportStarted();
        QException e;
        promise.reportException(e);
        promise.reportFinished();

        int thenResult = 0;
        QFuture<void> then = promise.future().then([&](int res) { thenResult = res; });

        bool caught = false;
        try {
            then.waitForFinished();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
        QCOMPARE(thenResult, 0);
    }

    // Same with QtFuture::Launch::Async
    {
        QFutureInterface<int> promise;

        int thenResult = 0;
        QFuture<void> then =
                promise.future().then(QtFuture::Launch::Async, [&](int res) { thenResult = res; });

        promise.reportStarted();
        QException e;
        promise.reportException(e);
        promise.reportFinished();

        bool caught = false;
        try {
            then.waitForFinished();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
        QCOMPARE(thenResult, 0);
    }

    // Exception future
    {
        QFutureInterface<int> promise;
        promise.reportStarted();
        QException e;
        promise.reportException(e);
        promise.reportFinished();

        int thenResult = 0;
        QFuture<void> then =
                promise.future().then(QtFuture::Launch::Async, [&](int res) { thenResult = res; });

        bool caught = false;
        try {
            then.waitForFinished();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
        QCOMPARE(thenResult, 0);
    }
}

template<class Exception, bool hasTestMsg = false>
QFuture<void> createExceptionContinuation(QtFuture::Launch policy = QtFuture::Launch::Sync)
{
    QFutureInterface<void> promise;

    auto then = promise.future().then(policy, [] {
        if constexpr (hasTestMsg)
            throw Exception("TEST");
        else
            throw Exception();
    });

    promise.reportStarted();
    promise.reportFinished();

    return then;
}

void tst_QFuture::thenThrows()
{
    // Continuation throws a QException
    {
        auto future = createExceptionContinuation<QException>();

        bool caught = false;
        try {
            future.waitForFinished();
        } catch (const QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // Continuation throws an exception derived from QException
    {
        auto future = createExceptionContinuation<DerivedException>();

        bool caught = false;
        try {
            future.waitForFinished();
        } catch (const QException &) {
            caught = true;
        } catch (const std::exception &) {
            QFAIL("The exception should be caught by the above catch block.");
        }

        QVERIFY(caught);
    }

    // Continuation throws std::exception
    {
        auto future = createExceptionContinuation<std::runtime_error, true>();

        bool caught = false;
        try {
            future.waitForFinished();
        } catch (const QException &) {
            QFAIL("The exception should be caught by the below catch block.");
        } catch (const std::exception &e) {
            QCOMPARE(e.what(), "TEST");
            caught = true;
        }

        QVERIFY(caught);
    }

    // Same with QtFuture::Launch::Async
    {
        auto future = createExceptionContinuation<QException>(QtFuture::Launch::Async);

        bool caught = false;
        try {
            future.waitForFinished();
        } catch (const QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }
}

void tst_QFuture::onFailed()
{
    // Ready exception void future
    {
        int checkpoint = 0;
        auto future = createExceptionFuture().then([&] { checkpoint = 1; }).onFailed([&] {
            checkpoint = 2;
        });

        try {
            future.waitForFinished();
        } catch (...) {
            checkpoint = 3;
        }
        QCOMPARE(checkpoint, 2);
    }

    // std::exception handler
    {
        QFutureInterface<int> promise;

        int checkpoint = 0;
        auto then = promise.future()
                            .then([&](int res) {
                                throw std::exception();
                                return res;
                            })
                            .then([&](int res) { return res + 1; })
                            .onFailed([&](const QException &) {
                                checkpoint = 1;
                                return -1;
                            })
                            .onFailed([&](const std::exception &) {
                                checkpoint = 2;
                                return -1;
                            })
                            .onFailed([&] {
                                checkpoint = 3;
                                return -1;
                            });

        promise.reportStarted();
        promise.reportResult(1);
        promise.reportFinished();

        int res = 0;
        try {
            res = then.result();
        } catch (...) {
            checkpoint = 4;
        }
        QCOMPARE(checkpoint, 2);
        QCOMPARE(res, -1);
    }

    // then() throws an exception derived from QException
    {
        QFutureInterface<int> promise;

        int checkpoint = 0;
        auto then = promise.future()
                            .then([&](int res) {
                                throw DerivedException();
                                return res;
                            })
                            .then([&](int res) { return res + 1; })
                            .onFailed([&](const QException &) {
                                checkpoint = 1;
                                return -1;
                            })
                            .onFailed([&](const std::exception &) {
                                checkpoint = 2;
                                return -1;
                            })
                            .onFailed([&] {
                                checkpoint = 3;
                                return -1;
                            });

        promise.reportStarted();
        promise.reportResult(1);
        promise.reportFinished();

        int res = 0;
        try {
            res = then.result();
        } catch (...) {
            checkpoint = 4;
        }
        QCOMPARE(checkpoint, 1);
        QCOMPARE(res, -1);
    }

    // then() throws a custom exception
    {
        QFutureInterface<int> promise;

        int checkpoint = 0;
        auto then = promise.future()
                            .then([&](int res) {
                                throw TestException();
                                return res;
                            })
                            .then([&](int res) { return res + 1; })
                            .onFailed([&](const QException &) {
                                checkpoint = 1;
                                return -1;
                            })
                            .onFailed([&](const std::exception &) {
                                checkpoint = 2;
                                return -1;
                            })
                            .onFailed([&] {
                                checkpoint = 3;
                                return -1;
                            });

        promise.reportStarted();
        promise.reportResult(1);
        promise.reportFinished();

        int res = 0;
        try {
            res = then.result();
        } catch (...) {
            checkpoint = 4;
        }
        QCOMPARE(checkpoint, 3);
        QCOMPARE(res, -1);
    }

    // Custom exception handler
    {
        struct TestException
        {
        };

        QFutureInterface<int> promise;

        int checkpoint = 0;
        auto then = promise.future()
                            .then([&](int res) {
                                throw TestException();
                                return res;
                            })
                            .then([&](int res) { return res + 1; })
                            .onFailed([&](const QException &) {
                                checkpoint = 1;
                                return -1;
                            })
                            .onFailed([&](const TestException &) {
                                checkpoint = 2;
                                return -1;
                            })
                            .onFailed([&] {
                                checkpoint = 3;
                                return -1;
                            });

        promise.reportStarted();
        promise.reportResult(1);
        promise.reportFinished();

        int res = 0;
        try {
            res = then.result();
        } catch (...) {
            checkpoint = 4;
        }
        QCOMPARE(checkpoint, 2);
        QCOMPARE(res, -1);
    }

    // Handle all exceptions
    {
        QFutureInterface<int> promise;

        int checkpoint = 0;
        auto then = promise.future()
                            .then([&](int res) {
                                throw QException();
                                return res;
                            })
                            .then([&](int res) { return res + 1; })
                            .onFailed([&] {
                                checkpoint = 1;
                                return -1;
                            })
                            .onFailed([&](const QException &) {
                                checkpoint = 2;
                                return -1;
                            });

        promise.reportStarted();
        promise.reportResult(1);
        promise.reportFinished();

        int res = 0;
        try {
            res = then.result();
        } catch (...) {
            checkpoint = 3;
        }
        QCOMPARE(checkpoint, 1);
        QCOMPARE(res, -1);
    }

    // Handler throws exception
    {
        QFutureInterface<int> promise;

        int checkpoint = 0;
        auto then = promise.future()
                            .then([&](int res) {
                                throw QException();
                                return res;
                            })
                            .then([&](int res) { return res + 1; })
                            .onFailed([&](const QException &) {
                                checkpoint = 1;
                                throw QException();
                                return -1;
                            })
                            .onFailed([&] {
                                checkpoint = 2;
                                return -1;
                            });

        promise.reportStarted();
        promise.reportResult(1);
        promise.reportFinished();

        int res = 0;
        try {
            res = then.result();
        } catch (...) {
            checkpoint = 3;
        }
        QCOMPARE(checkpoint, 2);
        QCOMPARE(res, -1);
    }

    // No handler for exception
    {
        QFutureInterface<int> promise;

        int checkpoint = 0;
        auto then = promise.future()
                            .then([&](int res) {
                                throw QException();
                                return res;
                            })
                            .then([&](int res) { return res + 1; })
                            .onFailed([&](const std::exception &) {
                                checkpoint = 1;
                                throw std::exception();
                                return -1;
                            })
                            .onFailed([&](QException &) {
                                checkpoint = 2;
                                return -1;
                            });

        promise.reportStarted();
        promise.reportResult(1);
        promise.reportFinished();

        int res = 0;
        try {
            res = then.result();
        } catch (...) {
            checkpoint = 3;
        }
        QCOMPARE(checkpoint, 3);
        QCOMPARE(res, 0);
    }

    // onFailed on a canceled future
    {
        auto future = createCanceledFuture<int>()
                              .then([](int) { return 42; })
                              .onCanceled([] { return -1; })
                              .onFailed([] { return -2; });
        QCOMPARE(future.result(), -1);
    }
}

template<class Callable>
bool runForCallable(Callable &&handler)
{
    QFuture<int> future = createExceptionResultFuture()
                                  .then([&](int) { return 1; })
                                  .onFailed(std::forward<Callable>(handler));

    int res = 0;
    try {
        res = future.result();
    } catch (...) {
        return false;
    }
    return res == -1;
}

int foo()
{
    return -1;
}

void tst_QFuture::onFailedTestCallables()
{
    QVERIFY(runForCallable([&] { return -1; }));
    QVERIFY(runForCallable(foo));
    QVERIFY(runForCallable(&foo));

    std::function<int()> func = foo;
    QVERIFY(runForCallable(func));

    struct Functor1
    {
        int operator()() { return -1; }
        static int foo() { return -1; }
    };
    QVERIFY(runForCallable(Functor1()));
    QVERIFY(runForCallable(Functor1::foo));

    struct Functor2
    {
        int operator()() const { return -1; }
        static int foo() { return -1; }
    };
    QVERIFY(runForCallable(Functor2()));

    struct Functor3
    {
        int operator()() const noexcept { return -1; }
        static int foo() { return -1; }
    };
    QVERIFY(runForCallable(Functor3()));
}

template<class Callable>
bool runOnFailedForMoveOnly(Callable &&callable)
{
    QFutureInterface<UniquePtr> promise;
    auto future = promise.future();

    auto failedFuture = future.onFailed(std::forward<Callable>(callable));

    promise.reportStarted();
    QException e;
    promise.reportException(e);
    promise.reportFinished();

    return *failedFuture.takeResult() == -1;
}

void tst_QFuture::onFailedForMoveOnlyTypes()
{
    QVERIFY(runOnFailedForMoveOnly([](const QException &) { return std::make_unique<int>(-1); }));
    QVERIFY(runOnFailedForMoveOnly([] { return std::make_unique<int>(-1); }));
}

#endif // QT_NO_EXCEPTIONS

void tst_QFuture::onCanceled()
{
    // Canceled int future
    {
        auto future = createCanceledFuture<int>().then([](int) { return 42; }).onCanceled([] {
            return -1;
        });
        QCOMPARE(future.result(), -1);
    }

    // Canceled void future
    {
        int checkpoint = 0;
        auto future = createCanceledFuture<void>().then([&] { checkpoint = 42; }).onCanceled([&] {
            checkpoint = -1;
        });
        QCOMPARE(checkpoint, -1);
    }

    // onCanceled propagates result
    {
        QFutureInterface<int> promise;
        auto future =
                promise.future().then([](int res) { return res; }).onCanceled([] { return -1; });

        promise.reportStarted();
        promise.reportResult(42);
        promise.reportFinished();
        QCOMPARE(future.result(), 42);
    }

    // onCanceled propagates move-only result
    {
        QFutureInterface<UniquePtr> promise;
        auto future = promise.future().then([](UniquePtr res) { return res; }).onCanceled([] {
            return std::make_unique<int>(-1);
        });

        promise.reportStarted();
        promise.reportAndMoveResult(std::make_unique<int>(42));
        promise.reportFinished();
        QCOMPARE(*future.takeResult(), 42);
    }

#ifndef QT_NO_EXCEPTIONS
    // onCanceled propagates exceptions
    {
        QFutureInterface<int> promise;
        auto future = promise.future()
                              .then([](int res) {
                                  throw std::runtime_error("error");
                                  return res;
                              })
                              .onCanceled([] { return 2; })
                              .onFailed([] { return 3; });

        promise.reportStarted();
        promise.reportResult(1);
        promise.reportFinished();
        QCOMPARE(future.result(), 3);
    }

    // onCanceled throws
    {
        auto future = createCanceledFuture<int>()
                              .then([](int) { return 42; })
                              .onCanceled([] {
                                  throw std::runtime_error("error");
                                  return -1;
                              })
                              .onFailed([] { return -2; });

        QCOMPARE(future.result(), -2);
    }

#endif // QT_NO_EXCEPTIONS
}

void tst_QFuture::cancelContinuations()
{
    // The chain is cancelled in the middle of execution of continuations
    {
        QPromise<int> promise;

        int checkpoint = 0;
        auto future = promise.future().then([&](int value) {
            ++checkpoint;
            return value + 1;
        }).then([&](int value) {
            ++checkpoint;
            promise.future().cancel();
            return value + 1;
        }).then([&](int value) {
            ++checkpoint;
            return value + 1;
        }).onCanceled([] {
            return -1;
        });

        promise.start();
        promise.addResult(42);
        promise.finish();

        QCOMPARE(future.result(), -1);
        QCOMPARE(checkpoint, 2);
    }

    // The chain is cancelled before the execution of continuations
    {
        auto f = QtFuture::makeReadyValueFuture(42);
        f.cancel();

        int checkpoint = 0;
        auto future = f.then([&](int value) {
            ++checkpoint;
            return value + 1;
        }).then([&](int value) {
            ++checkpoint;
            return value + 1;
        }).then([&](int value) {
            ++checkpoint;
            return value + 1;
        }).onCanceled([] {
            return -1;
        });

        QCOMPARE(future.result(), -1);
        QCOMPARE(checkpoint, 0);
    }

    // The chain is canceled partially, through an intermediate future
    {
        QPromise<int> promise;

        int checkpoint = 0;
        auto intermediate = promise.future().then([&](int value) {
            ++checkpoint;
            return value + 1;
        });

        auto future = intermediate.then([&](int value) {
            ++checkpoint;
            return value + 1;
        }).then([&](int value) {
            ++checkpoint;
            return value + 1;
        }).onCanceled([] {
            return -1;
        });

        promise.start();
        promise.addResult(42);

        // This should cancel only the chain starting from intermediate
        intermediate.cancel();

        promise.finish();

        QCOMPARE(future.result(), -1);
        QCOMPARE(checkpoint, 1);
    }

#ifndef QT_NO_EXCEPTIONS
    // The chain is cancelled in the middle of execution of continuations,
    // while there's an exception in the chain, which is handled inside
    // the continuations.
    {
        QPromise<int> promise;

        int checkpoint = 0;
        auto future = promise.future().then([&](int value) {
            ++checkpoint;
            throw QException();
            return value + 1;
        }).then([&](QFuture<int> future) {
            try {
                auto res = future.result();
                Q_UNUSED(res);
            } catch (const QException &) {
                ++checkpoint;
            }
            return 2;
        }).then([&](int value) {
            ++checkpoint;
            promise.future().cancel();
            return value + 1;
        }).then([&](int value) {
            ++checkpoint;
            return value + 1;
        }).onCanceled([] {
            return -1;
        });

        promise.start();
        promise.addResult(42);
        promise.finish();

        QCOMPARE(future.result(), -1);
        QCOMPARE(checkpoint, 3);
    }
#endif // QT_NO_EXCEPTIONS

    // Check notifications from QFutureWatcher
    {
        QPromise<void> p;
        auto f = p.future();

        auto f1 = f.then([] {});
        auto f2 = f1.then([] {});

        QFutureWatcher<void> watcher1, watcher2;
        int state = 0;
        QObject::connect(&watcher1, &QFutureWatcher<void>::started, [&] {
            QCOMPARE(state, 0);
            ++state;
        });
        QObject::connect(&watcher1, &QFutureWatcher<void>::canceled, [&] {
            QCOMPARE(state, 1);
            ++state;
        });
        QObject::connect(&watcher1, &QFutureWatcher<void>::finished, [&] {
            QCOMPARE(state, 2);
            ++state;
        });
        QObject::connect(&watcher2, &QFutureWatcher<void>::started, [&] {
            QCOMPARE(state, 3);
            ++state;
        });
        QObject::connect(&watcher2, &QFutureWatcher<void>::canceled, [&] {
            QCOMPARE(state, 4);
            ++state;
        });
        QObject::connect(&watcher2, &QFutureWatcher<int>::finished, [&] {
            QCOMPARE(state, 5);
            ++state;
        });

        watcher1.setFuture(f1);
        watcher2.setFuture(f2);

        p.start();
        f.cancel();
        p.finish();

        qApp->processEvents();

        QCOMPARE(state, 6);
        QVERIFY(watcher1.isFinished());
        QVERIFY(watcher1.isCanceled());
        QVERIFY(watcher2.isFinished());
        QVERIFY(watcher2.isCanceled());
    }

    // Cancel continuations with context (QTBUG-108790)
    {
        // This test should pass with ASan
        auto future = QtConcurrent::run([] {});
        future.then(this, [] {});
        future.waitForFinished();
        future.cancel();
    }
}

void tst_QFuture::continuationsWithContext()
{
    QThread thread;
    thread.start();

    auto context = new QObject();
    context->moveToThread(&thread);

    auto tstThread = QThread::currentThread();

    // .then()
    {
        QPromise<int> promise;
        auto future = promise.future()
                              .then([&](int val) {
                                  if (QThread::currentThread() != tstThread)
                                      return 0;
                                  return val + 1;
                              })
                              .then(context,
                                    [&](int val) {
                                        if (QThread::currentThread() != &thread)
                                            return 0;
                                        return val + 1;
                                    })
                              .then([&](int val) {
                                  if (QThread::currentThread() != &thread)
                                      return 0;
                                  return val + 1;
                              });
        promise.start();
        promise.addResult(0);
        promise.finish();
        QCOMPARE(future.result(), 3);
    }

    // .onCanceled
    {
        QPromise<int> promise;
        auto future = promise.future()
                              .onCanceled(context,
                                          [&] {
                                              if (QThread::currentThread() != &thread)
                                                  return 0;
                                              return 1;
                                          })
                              .then([&](int val) {
                                  if (QThread::currentThread() != &thread)
                                      return 0;
                                  return val + 1;
                              });
        promise.start();
        promise.future().cancel();
        promise.finish();
        QCOMPARE(future.result(), 2);
    }

    // Cancellation when the context object is destroyed
    {
        // Use something like QWidget which deletes its children early, i.e.
        // before ~QObject() runs. This behavior can lead to side-effects
        // like QPointers to the parent not being set to nullptr during child
        // object destruction.
        QPointer shortLivedContext = new FakeQWidget();
        shortLivedContext->moveToThread(&thread);

        QPromise<int> promise;
        auto future = promise.future()
                              .then(shortLivedContext, [&](int val) {
                                  if (QThread::currentThread() != &thread)
                                      return 0;
                                  return val + 1000;
                              })
                              .onCanceled([&, ptr=QPointer(shortLivedContext)] {
                                  if (QThread::currentThread() != &thread)
                                      return 0;
                                  if (ptr)
                                      return 1;
                                  return 2;
                              });
        promise.start();

        QMetaObject::invokeMethod(shortLivedContext, [&]() {
            delete shortLivedContext;
        }, Qt::BlockingQueuedConnection);

        promise.finish();

        QCOMPARE(future.result(), 2);
    }

#ifndef QT_NO_EXCEPTIONS
    // .onFaled()
    {
        QPromise<void> promise;
        auto future = promise.future()
                              .then([&] {
                                  if (QThread::currentThread() != tstThread)
                                      return 0;
                                  throw std::runtime_error("error");
                              })
                              .onFailed(context,
                                        [&] {
                                            if (QThread::currentThread() != &thread)
                                                return 0;
                                            return 1;
                                        })
                              .then([&](int val) {
                                  if (QThread::currentThread() != &thread)
                                      return 0;
                                  return val + 1;
                              });
        promise.start();
        promise.finish();
        QCOMPARE(future.result(), 2);
    }
#endif // QT_NO_EXCEPTIONS

    context->deleteLater();

    thread.quit();
    thread.wait();
}

void tst_QFuture::continuationsWithMoveOnlyLambda()
{
    // .then()
    {
        std::unique_ptr<int> uniquePtr(new int(42));
        auto future = QtFuture::makeReadyVoidFuture()
                .then([p = std::move(uniquePtr)] { return *p; });
        QCOMPARE(future.result(), 42);
    }
    // .then() with thread pool
    {
        QThreadPool pool;

        std::unique_ptr<int> uniquePtr(new int(42));
        auto future = QtFuture::makeReadyVoidFuture()
                .then(&pool, [p = std::move(uniquePtr)] { return *p; });
        QCOMPARE(future.result(), 42);
    }
    // .then() with context
    {
        QObject object;

        std::unique_ptr<int> uniquePtr(new int(42));
        auto future = QtFuture::makeReadyVoidFuture()
                .then(&object, [p = std::move(uniquePtr)] { return *p; });
        QCOMPARE(future.result(), 42);
    }

    // .onCanceled()
    {
        std::unique_ptr<int> uniquePtr(new int(42));
        auto future =
                createCanceledFuture<int>().onCanceled([p = std::move(uniquePtr)] { return *p; });
        QCOMPARE(future.result(), 42);
    }

    // .onCanceled() with context
    {
        QObject object;

        std::unique_ptr<int> uniquePtr(new int(42));
        auto future = createCanceledFuture<int>().onCanceled(
                &object, [p = std::move(uniquePtr)] { return *p; });
        QCOMPARE(future.result(), 42);
    }

#ifndef QT_NO_EXCEPTIONS
    // .onFailed()
    {
        std::unique_ptr<int> uniquePtr(new int(42));
        auto future = QtFuture::makeExceptionalFuture<int>(QException())
                              .onFailed([p = std::move(uniquePtr)] { return *p; });
        QCOMPARE(future.result(), 42);
    }
    // .onFailed() with context
    {
        QObject object;

        std::unique_ptr<int> uniquePtr(new int(42));
        auto future = QtFuture::makeExceptionalFuture<int>(QException())
                              .onFailed(&object, [p = std::move(uniquePtr)] { return *p; });
        QCOMPARE(future.result(), 42);
    }
#endif // QT_NO_EXCEPTIONS
}

void tst_QFuture::testSingleResult(const UniquePtr &p)
{
    QVERIFY(p.get() != nullptr);
}

void tst_QFuture::testSingleResult(const std::vector<int> &v)
{
    QVERIFY(!v.empty());
}

template<class T>
void tst_QFuture::testSingleResult(const T &unknown)
{
    Q_UNUSED(unknown)
}


template<class T>
void tst_QFuture::testFutureTaken(QFuture<T> &noMoreFuture)
{
    QCOMPARE(noMoreFuture.isValid(), false);
    QCOMPARE(noMoreFuture.resultCount(), 0);
    QCOMPARE(noMoreFuture.progressValue(), 0);
}

template<class T>
void tst_QFuture::testTakeResults(QFuture<T> future, size_type resultCount)
{
    auto copy = future;
    QVERIFY(future.isFinished());
    QVERIFY(future.isValid());
    QCOMPARE(size_type(future.resultCount()), resultCount);
    QVERIFY(copy.isFinished());
    QVERIFY(copy.isValid());
    QCOMPARE(size_type(copy.resultCount()), resultCount);

    auto vec = future.takeResults();
    QCOMPARE(vec.size(), resultCount);

    for (const auto &r : vec) {
        testSingleResult(r);
        if (QTest::currentTestFailed())
            return;
    }

    testFutureTaken(future);
    if (QTest::currentTestFailed())
        return;
    testFutureTaken(copy);
}

#if 0
void tst_QFuture::takeResults()
{
    // Test takeResults() for movable types (whether or not copyable).

    // std::unique_ptr<int> supports only move semantics:
    QFutureInterface<UniquePtr> moveIface;
    moveIface.reportStarted();

    // std::vector<int> supports both copy and move:
    QFutureInterface<std::vector<int>> copyIface;
    copyIface.reportStarted();

    const int expectedCount = 10;

    for (int i = 0; i < expectedCount; ++i) {
        QVERIFY(moveIface.reportAndMoveResult(UniquePtr{new int(0b101010)}, i));
        QVERIFY(copyIface.reportAndMoveResult(std::vector<int>{1,2,3,4,5}, i));
    }

    moveIface.reportFinished();
    copyIface.reportFinished();

    testTakeResults(moveIface.future(), size_type(expectedCount));
    if (QTest::currentTestFailed())
        return;

    testTakeResults(copyIface.future(), size_type(expectedCount));
}
#endif

void tst_QFuture::takeResult()
{
    QFutureInterface<UniquePtr> iface;
    iface.reportStarted();
    QVERIFY(iface.reportAndMoveResult(UniquePtr{new int(0b101010)}, 0));
    iface.reportFinished();

    auto future = iface.future();
    QVERIFY(future.isFinished());
    QVERIFY(future.isValid());
    QCOMPARE(future.resultCount(), 1);

    auto result = future.takeResult();
    testFutureTaken(future);
    if (QTest::currentTestFailed())
        return;
    testSingleResult(result);
}

void tst_QFuture::runAndTake()
{
    // Test if a 'moving' future can be used by
    // QtConcurrent::run.

    auto rabbit = [](){
        // Let's wait a bit to give the test below some time
        // to sync up with us with its watcher.
        QThread::currentThread()->sleep(std::chrono::milliseconds{100});
        return UniquePtr(new int(10));
    };

    QTestEventLoop loop;
    QFutureWatcher<UniquePtr> watcha;
    connect(&watcha, &QFutureWatcher<UniquePtr>::finished, [&loop](){
        loop.exitLoop();
    });

    auto gotcha = QtConcurrent::run(rabbit);
    watcha.setFuture(gotcha);

    loop.enterLoop(500ms);
    if (loop.timeout())
        QSKIP("Failed to run the task, nothing to test");

    gotcha = watcha.future();
#if 0
    // TODO: enable when QFuture::takeResults() is enabled
    testTakeResults(gotcha, size_type(1));
#endif
}

void tst_QFuture::resultsReadyAt_data()
{
    QTest::addColumn<bool>("testMove");

    QTest::addRow("reportResult") << false;
    QTest::addRow("reportAndMoveResult") << true;
}

void tst_QFuture::resultsReadyAt()
{
    QFETCH(const bool, testMove);

    QFutureInterface<int> iface;
    QFutureWatcher<int> watcher;
    watcher.setFuture(iface.future());

    QTestEventLoop eventProcessor;
    connect(&watcher, &QFutureWatcher<int>::finished, &eventProcessor, &QTestEventLoop::exitLoop);

    const int nExpectedResults = 4;
    int reported = 0;
    int taken = 0;
    connect(&watcher, &QFutureWatcher<int>::resultsReadyAt,
            [&iface, &reported, &taken](int begin, int end)
    {
        auto future = iface.future();
        QVERIFY(end - begin > 0);
        for (int i = begin; i < end; ++i, ++reported) {
            QVERIFY(future.isResultReadyAt(i));
            taken |= 1 << i;
        }
    });

    auto report = [&iface, testMove](int index)
    {
        int dummyResult = 0b101010;
        if (testMove)
            QVERIFY(iface.reportAndMoveResult(std::move(dummyResult), index));
        else
            QVERIFY(iface.reportResult(&dummyResult, index));
    };

    const QSignalSpy readyCounter(&watcher, &QFutureWatcher<int>::resultsReadyAt);
    QTimer::singleShot(0, [&iface, &report]{
        // With filter mode == true, the result may go into the pending results.
        // Reporting it as ready will allow an application to try and access the
        // result, crashing on invalid (store.end()) iterator dereferenced.
        iface.setFilterMode(true);
        iface.reportStarted();
        report(0);
        report(1);
        // This one - should not be reported (it goes into pending):
        report(3);
        // Let's close the 'gap' and make them all ready:
        report(-1);
        iface.reportFinished();
    });

    // Run event loop, QCoreApplication::postEvent is in use
    // in QFutureInterface:
    eventProcessor.enterLoop(DefaultWaitTime);
    QVERIFY(!eventProcessor.timeout());
    if (QTest::currentTestFailed()) // Failed in our lambda observing 'ready at'
        return;

    QCOMPARE(reported, nExpectedResults);
    QCOMPARE(nExpectedResults, iface.future().resultCount());
    QCOMPARE(readyCounter.size(), 3);
    QCOMPARE(taken, 0b1111);
}

template <class T>
auto makeFutureInterface(T &&result)
{
    QFutureInterface<T> f;

    f.reportStarted();
    f.reportResult(std::forward<T>(result));
    f.reportFinished();

    return f;
}

void tst_QFuture::takeResultWorksForTypesWithoutDefaultCtor()
{
    struct Foo
    {
        Foo() = delete;
        explicit Foo(int i) : _i(i) {}

        int _i = -1;
    };

    auto f = makeFutureInterface(Foo(42));

    QCOMPARE(f.takeResult()._i, 42);
}
void tst_QFuture::canceledFutureIsNotValid()
{
    auto f = makeFutureInterface(42);

    f.cancel();

    QVERIFY(!f.isValid());
}

void tst_QFuture::signalConnect()
{
    const int intValue = 42;
    const double doubleValue = 42.5;
    const QString stringValue = "42";

    using TupleType = std::tuple<int, double, QString>;
    const TupleType tuple(intValue, doubleValue, stringValue);

    using PairType = std::pair<int, double>;
    const PairType pair(intValue, doubleValue);

    // No arg
    {
        SenderObject sender;
        auto future =
                QtFuture::connect(&sender, &SenderObject::noArgSignal).then([] { return true; });
        sender.emitNoArg();
        QCOMPARE(future.result(), true);
    }

    // One arg
    {
        SenderObject sender;
        auto future = QtFuture::connect(&sender, &SenderObject::intArgSignal).then([](int value) {
            return value;
        });
        sender.emitIntArg(42);
        QCOMPARE(future.result(), 42);
    }

    // Const ref arg
    {
        SenderObject sender;
        auto future =
                QtFuture::connect(&sender, &SenderObject::constRefArg).then([](QString value) {
                    return value;
                });
        sender.emitConstRefArg(QString("42"));
        QCOMPARE(future.result(), "42");
    }

    // Multiple args
    {
        SenderObject sender;
        auto future =
                QtFuture::connect(&sender, &SenderObject::multipleArgs).then([](TupleType values) {
                    return values;
                });
        sender.emitMultipleArgs(intValue, doubleValue, stringValue);
        auto result = future.result();
        QCOMPARE(result, tuple);
    }

    // Single std::tuple arg
    {
        SenderObject sender;
        QFuture<TupleType> future = QtFuture::connect(&sender, &SenderObject::tupleArgSignal);
        sender.emitTupleArgSignal(tuple);
        auto result = future.result();
        QCOMPARE(result, tuple);
    }

    // Multi-args signal(int, std::tuple)
    {
        SenderObject sender;
        QFuture<std::tuple<int, TupleType>> future =
                QtFuture::connect(&sender, &SenderObject::multiArgsWithTupleSignal1);
        sender.emitMultiArgsWithTupleSignal1(142, tuple);
        const auto [v, t] = future.result();
        QCOMPARE(v, 142);
        QCOMPARE(t, tuple);
    }

    // Multi-args signal(std::tuple, int)
    {
        SenderObject sender;
        QFuture<std::tuple<TupleType, int>> future =
                QtFuture::connect(&sender, &SenderObject::multiArgsWithTupleSignal2);
        sender.emitMultiArgsWithTupleSignal2(tuple, 142);
        const auto [t, v] = future.result();
        QCOMPARE(v, 142);
        QCOMPARE(t, tuple);
    }

    // Multi-args signal(int, std::pair)
    {
        SenderObject sender;
        QFuture<std::tuple<int, PairType>> future =
                QtFuture::connect(&sender, &SenderObject::multiArgsWithPairSignal1);
        sender.emitMultiArgsWithPairSignal1(142, pair);
        const auto [v, p] = future.result();
        QCOMPARE(v, 142);
        QCOMPARE(p, pair);
    }

    // Multi-args signal(std::pair, int)
    {
        SenderObject sender;
        QFuture<std::tuple<PairType, int>> future =
                QtFuture::connect(&sender, &SenderObject::multiArgsWithPairSignal2);
        sender.emitMultiArgsWithPairSignal2(pair, 142);
        const auto [p, v] = future.result();
        QCOMPARE(v, 142);
        QCOMPARE(p, pair);
    }

    // No arg private signal
    {
        SenderObject sender;
        auto future = QtFuture::connect(&sender, &SenderObject::noArgPrivateSignal).then([] {
            return true;
        });
        sender.emitNoArgPrivateSignal();
        QCOMPARE(future.result(), true);
    }

    // One arg private signal
    {
        SenderObject sender;
        auto future =
                QtFuture::connect(&sender, &SenderObject::intArgPrivateSignal).then([](int value) {
                    return value;
                });
        sender.emitIntArgPrivateSignal(42);
        QCOMPARE(future.result(), 42);
    }

    // Multi-args private signal
    {
        SenderObject sender;
        auto future = QtFuture::connect(&sender, &SenderObject::multiArgsPrivateSignal)
                              .then([](TupleType values) { return values; });
        sender.emitMultiArgsPrivateSignal(intValue, doubleValue, stringValue);
        auto result = future.result();
        QCOMPARE(result, tuple);
    }

    // Single std::tuple arg private signal
    {
        SenderObject sender;
        QFuture<TupleType> future =
                QtFuture::connect(&sender, &SenderObject::tupleArgPrivateSignal);
        sender.emitTupleArgPrivateSignal(tuple);
        auto result = future.result();
        QCOMPARE(result, tuple);
    }

    // Multi-args private signal(int, std::tuple)
    {
        SenderObject sender;
        QFuture<std::tuple<int, TupleType>> future =
                QtFuture::connect(&sender, &SenderObject::multiArgsWithTuplePrivateSignal1);
        sender.emitMultiArgsWithTuplePrivateSignal1(142, tuple);
        const auto [v, t] = future.result();
        QCOMPARE(v, 142);
        QCOMPARE(t, tuple);
    }

    // Multi-args private signal(std::tuple, int)
    {
        SenderObject sender;
        QFuture<std::tuple<TupleType, int>> future =
                QtFuture::connect(&sender, &SenderObject::multiArgsWithTuplePrivateSignal2);
        sender.emitMultiArgsWithTuplePrivateSignal2(tuple, 142);
        const auto [t, v] = future.result();
        QCOMPARE(v, 142);
        QCOMPARE(t, tuple);
    }

    // Multi-args private signal(int, std::pair)
    {
        SenderObject sender;
        QFuture<std::tuple<int, PairType>> future =
                QtFuture::connect(&sender, &SenderObject::multiArgsWithPairPrivateSignal1);
        sender.emitMultiArgsWithPairPrivateSignal1(142, pair);
        const auto [v, p] = future.result();
        QCOMPARE(v, 142);
        QCOMPARE(p, pair);
    }

    // Multi-args private signal(std::pair, int)
    {
        SenderObject sender;
        QFuture<std::tuple<PairType, int>> future =
                QtFuture::connect(&sender, &SenderObject::multiArgsWithPairPrivateSignal2);
        sender.emitMultiArgsWithPairPrivateSignal2(pair, 142);
        const auto [p, v] = future.result();
        QCOMPARE(v, 142);
        QCOMPARE(p, pair);
    }

    // Sender destroyed
    {
        SenderObject *sender = new SenderObject();

        auto future = QtFuture::connect(sender, &SenderObject::intArgSignal);

        QSignalSpy spy(sender, &QObject::destroyed);
        sender->deleteLater();

        spy.wait();

        QVERIFY(future.isCanceled());
        QVERIFY(!future.isValid());
    }

    // Signal emitted, causing Sender to be destroyed
    {
        SenderObject *sender = new SenderObject();

        auto future = QtFuture::connect(sender, &SenderObject::intArgSignal);
        future.then([sender](int) {
            // Scenario: Sender no longer needed, so it's deleted
            delete sender;
        });

        QSignalSpy spy(sender, &SenderObject::destroyed);
        emit sender->intArgSignal(5);
        spy.wait();

        QVERIFY(future.isFinished());
        QVERIFY(!future.isCanceled());
        QVERIFY(future.isValid());
    }

    // Connect to nullptr
    {
        SenderObject *sender = nullptr;
        auto future = QtFuture::connect(sender, &SenderObject::intArgSignal);
        QVERIFY(future.isFinished());
        QVERIFY(future.isCanceled());
        QVERIFY(!future.isValid());
    }

    // Connect to non-signal
    {
        SenderObject sender;

#if defined(Q_CC_MSVC) && !defined(Q_CC_CLANG)
#define EXPECT_FUTURE_CONNECT_FAIL() QEXPECT_FAIL("", "QTBUG-101761, test fails on Windows/MSVC", Continue)
#else
        QTest::ignoreMessage(QtWarningMsg, "QObject::connect: signal not found in SenderObject");
#define EXPECT_FUTURE_CONNECT_FAIL()
#endif

        auto future = QtFuture::connect(&sender, &SenderObject::emitNoArg);
        EXPECT_FUTURE_CONNECT_FAIL();
        QVERIFY(future.isFinished());
        EXPECT_FUTURE_CONNECT_FAIL();
        QVERIFY(future.isCanceled());
        EXPECT_FUTURE_CONNECT_FAIL();
        QVERIFY(!future.isValid());
#undef EXPECT_FUTURE_CONNECT_FAIL
    }
}

void tst_QFuture::waitForFinished()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires QThread::create");
#else
    QFutureInterface<void> fi;
    auto future = fi.future();

    QScopedPointer<QThread> waitingThread (QThread::create([&] {
        future.waitForFinished();
    }));

    waitingThread->start();

    QVERIFY(!waitingThread->wait(200));
    QVERIFY(!waitingThread->isFinished());

    fi.reportStarted();
    QVERIFY(!waitingThread->wait(200));
    QVERIFY(!waitingThread->isFinished());

    fi.reportFinished();

    QVERIFY(waitingThread->wait());
    QVERIFY(waitingThread->isFinished());
#endif
}

void tst_QFuture::rejectResultOverwrite_data()
{
    QTest::addColumn<bool>("filterMode");
    QTest::addColumn<QList<int>>("initResults");

    QTest::addRow("filter-mode-on-1-result") << true << QList<int>({ 456 });
    QTest::addRow("filter-mode-on-N-results") << true << QList<int>({ 456, 789 });
    QTest::addRow("filter-mode-off-1-result") << false << QList<int>({ 456 });
    QTest::addRow("filter-mode-off-N-results") << false << QList<int>({ 456, 789 });
}

void tst_QFuture::rejectResultOverwrite()
{
    QFETCH(bool, filterMode);
    QFETCH(QList<int>, initResults);

    QFutureInterface<int> iface;
    iface.setFilterMode(filterMode);
    auto f = iface.future();
    QFutureWatcher<int> watcher;
    watcher.setFuture(f);

    QTestEventLoop eventProcessor;
    // control the loop by suspend
    connect(&watcher, &QFutureWatcher<int>::suspending, &eventProcessor, &QTestEventLoop::exitLoop);
    // internal machinery always emits resultsReadyAt
    QSignalSpy resultCounter(&watcher, &QFutureWatcher<int>::resultsReadyAt);

    // init
    if (initResults.size() == 1)
        QVERIFY(iface.reportResult(initResults[0]));
    else
        QVERIFY(iface.reportResults(initResults));
    QCOMPARE(f.resultCount(), initResults.size());
    QCOMPARE(f.resultAt(0), initResults[0]);
    QCOMPARE(f.results(), initResults);

    QTimer::singleShot(50, [&f]() {
        f.suspend(); // should exit the loop
    });
    // Run event loop, QCoreApplication::postEvent is in use
    // in QFutureInterface:
    eventProcessor.enterLoop(DefaultWaitTime);
    QVERIFY(!eventProcessor.timeout());
    QCOMPARE(resultCounter.size(), 1);
    f.resume();

    // overwrite with lvalue
    {
        int result = -1;
        const auto originalCount = f.resultCount();
        QVERIFY(!iface.reportResult(result, 0));
        QCOMPARE(f.resultCount(), originalCount);
        QCOMPARE(f.resultAt(0), initResults[0]);
    }
    // overwrite with rvalue
    {
        const auto originalCount = f.resultCount();
        QVERIFY(!iface.reportResult(-1, 0));
        QCOMPARE(f.resultCount(), originalCount);
        QCOMPARE(f.resultAt(0), initResults[0]);
    }
    // overwrite with array
    {
        const auto originalCount = f.resultCount();
        QVERIFY(!iface.reportResults(QList<int> { -1, -2, -3 }, 0));
        QCOMPARE(f.resultCount(), originalCount);
        QCOMPARE(f.resultAt(0), initResults[0]);
    }

    // special case: add result by different index, overlapping with the vector
    if (initResults.size() > 1) {
        const auto originalCount = f.resultCount();
        QVERIFY(!iface.reportResult(-1, 1));
        QCOMPARE(f.resultCount(), originalCount);
        QCOMPARE(f.resultAt(1), initResults[1]);
    }

    QTimer::singleShot(50, [&f]() {
        f.suspend(); // should exit the loop
    });
    eventProcessor.enterLoop(DefaultWaitTime);
    QVERIFY(!eventProcessor.timeout());
    QCOMPARE(resultCounter.size(), 1);
    f.resume();
    QCOMPARE(f.results(), initResults);
}

void tst_QFuture::rejectPendingResultOverwrite()
{
    QFETCH(bool, filterMode);
    QFETCH(QList<int>, initResults);

    QFutureInterface<int> iface;
    iface.setFilterMode(filterMode);
    auto f = iface.future();
    QFutureWatcher<int> watcher;
    watcher.setFuture(f);

    QTestEventLoop eventProcessor;
    // control the loop by suspend
    connect(&watcher, &QFutureWatcher<int>::suspending, &eventProcessor, &QTestEventLoop::exitLoop);
    // internal machinery always emits resultsReadyAt
    QSignalSpy resultCounter(&watcher, &QFutureWatcher<int>::resultsReadyAt);

    // init
    if (initResults.size() == 1)
        QVERIFY(iface.reportResult(initResults[0], 1));
    else
        QVERIFY(iface.reportResults(initResults, 1));
    QCOMPARE(f.resultCount(), 0); // not visible yet
    if (!filterMode) {
        QCOMPARE(f.resultAt(1), initResults[0]);
        QCOMPARE(f.results(), initResults);

        QTimer::singleShot(50, [&f]() {
            f.suspend(); // should exit the loop
        });
        // Run event loop, QCoreApplication::postEvent is in use
        // in QFutureInterface:
        eventProcessor.enterLoop(DefaultWaitTime);
        QVERIFY(!eventProcessor.timeout());
        QCOMPARE(resultCounter.size(), 1);
        f.resume();
    }

    // overwrite with lvalue
    {
        int result = -1;
        const auto originalCount = f.resultCount();
        QVERIFY(!iface.reportResult(result, 1));
        QCOMPARE(f.resultCount(), originalCount);
        if (!filterMode)
            QCOMPARE(f.resultAt(1), initResults[0]);
    }
    // overwrite with rvalue
    {
        const auto originalCount = f.resultCount();
        QVERIFY(!iface.reportResult(-1, 1));
        QCOMPARE(f.resultCount(), originalCount);
        if (!filterMode)
            QCOMPARE(f.resultAt(1), initResults[0]);
    }
    // overwrite with array
    {
        const auto originalCount = f.resultCount();
        QVERIFY(!iface.reportResults(QList<int> { -1, -2 }, 1));
        QCOMPARE(f.resultCount(), originalCount);
        if (!filterMode)
            QCOMPARE(f.resultAt(1), initResults[0]);
    }
    // special case: add result by different index, overlapping with the vector
    if (initResults.size() > 1) {
        const auto originalCount = f.resultCount();
        QVERIFY(!iface.reportResult(-1, 2));
        QCOMPARE(f.resultCount(), originalCount);
        if (!filterMode)
            QCOMPARE(f.resultAt(2), initResults[1]);
    }

    if (!filterMode) {
        QTimer::singleShot(50, [&f]() {
            f.suspend(); // should exit the loop
        });
        eventProcessor.enterLoop(DefaultWaitTime);
        QVERIFY(!eventProcessor.timeout());
        QCOMPARE(resultCounter.size(), 1);
        f.resume();
    }

    QVERIFY(iface.reportResult(123, 0)); // make results at 0 and 1 accessible
    QCOMPARE(f.resultCount(), initResults.size() + 1);
    QCOMPARE(f.resultAt(1), initResults[0]);
    initResults.prepend(123);
    QCOMPARE(f.results(), initResults);
}

void tst_QFuture::createReadyFutures()
{
#if QT_DEPRECATED_SINCE(6, 10)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    // using const T &
    {
        const int val = 42;
        QFuture<int> f = QtFuture::makeReadyFuture(val);
        QCOMPARE(f.result(), val);
    }

    // using T
    {
        int val = 42;
        QFuture<int> f = QtFuture::makeReadyFuture(val);
        QCOMPARE(f.result(), val);
    }

    // using T &&
    {
        auto f = QtFuture::makeReadyFuture(std::make_unique<int>(42));
        QCOMPARE(*f.takeResult(), 42);
    }

    // using void
    {
        auto f = QtFuture::makeReadyFuture();
        QVERIFY(f.isStarted());
        QVERIFY(!f.isRunning());
        QVERIFY(f.isFinished());
    }

    // using const QList<T> &
    {
        const QList<int> values { 1, 2, 3 };
        auto f = QtFuture::makeReadyFuture(values);
        QCOMPARE(f.resultCount(), 3);
        QCOMPARE(f.results(), values);
    }
QT_WARNING_POP
#endif // QT_DEPRECATED_SINCE(6, 10)

    // test makeReadyValueFuture<T>()
    {
        const int val = 42;
        auto f = QtFuture::makeReadyValueFuture(val);
        QCOMPARE_EQ(f.result(), val);

        int otherVal = 42;
        f = QtFuture::makeReadyValueFuture(otherVal);
        QCOMPARE_EQ(f.result(), otherVal);
    }
    {
        auto f = QtFuture::makeReadyValueFuture(std::make_unique<int>(42));
        QCOMPARE(*f.takeResult(), 42);
    }
    // test makeReadyVoidFuture()
    {
        auto f = QtFuture::makeReadyVoidFuture();
        QVERIFY(f.isStarted());
        QVERIFY(!f.isRunning());
        QVERIFY(f.isFinished());
    }

#ifndef QT_NO_EXCEPTIONS
    // using QException
    {
        QException e;
        auto f = QtFuture::makeExceptionalFuture<int>(e);
        bool caught = false;
        try {
            f.result();
        } catch (QException &) {
            caught = true;
        }
        QVERIFY(caught);
    }

    // using std::exception_ptr and QFuture<void>
    {
        auto exception = std::make_exception_ptr(TestException());
        auto f = QtFuture::makeExceptionalFuture(exception);
        bool caught = false;
        try {
            f.waitForFinished();
        } catch (TestException &) {
            caught = true;
        }
        QVERIFY(caught);
    }
#endif

    // testing makeReadyRangeFuture with various containers
    {
        const QList<int> expectedResult{1, 2, 3};

        const QList<int> list{1, 2, 3};
        auto f = QtFuture::makeReadyRangeFuture(list);
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);

        QVarLengthArray<int> varArray{1, 2, 3};
        f = QtFuture::makeReadyRangeFuture(varArray);
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);

        std::vector<int> vec{1, 2, 3};
        f = QtFuture::makeReadyRangeFuture(std::move(vec));
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);

        f = QtFuture::makeReadyRangeFuture(std::array<int, 3>{1, 2, 3});
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);

        f = QtFuture::makeReadyRangeFuture(std::list<int>{1, 2, 3});
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);

        std::forward_list<int> fwdlist{1, 2, 3};
        f = QtFuture::makeReadyRangeFuture(fwdlist);
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);

        const QSet<int> qset{1, 2, 3};
        f = QtFuture::makeReadyRangeFuture(qset);
        QCOMPARE_EQ(f.resultCount(), 3);
        auto result = f.results();
        std::sort(result.begin(), result.end());
        QCOMPARE_EQ(result, expectedResult);

        const QMap<QString, int> qmap{
            {"one", 1},
            {"two", 2},
            {"three", 3}
        };
        f = QtFuture::makeReadyRangeFuture(qmap);
        QCOMPARE_EQ(f.resultCount(), 3);
        result = f.results();
        std::sort(result.begin(), result.end());
        QCOMPARE_EQ(result, expectedResult);

        std::set<int> stdset{1, 2, 3};
        f = QtFuture::makeReadyRangeFuture(stdset);
        QCOMPARE_EQ(f.resultCount(), 3);
        result = f.results();
        std::sort(result.begin(), result.end());
        QCOMPARE_EQ(result, expectedResult);

        // testing ValueType[N] overload
        const int c_array[] = {1, 2, 3};
        f = QtFuture::makeReadyRangeFuture(c_array);
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);

        f = QtFuture::makeReadyRangeFuture({1, 2, 3});
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);
    }
    // testing makeReadyRangeFuture with a more complex underlying type
    {
        QObject obj1;
        QObject obj2;
        QObject obj3;

        const QList<QObject*> expectedResult{&obj1, &obj2, &obj3};

        const QList<QObject*> list{&obj1, &obj2, &obj3};
        auto f = QtFuture::makeReadyRangeFuture(list);
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);

        std::list<QObject*> stdlist{&obj1, &obj2, &obj3};
        f = QtFuture::makeReadyRangeFuture(std::move(stdlist));
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);

        QObject* const c_array[] = {&obj1, &obj2, &obj3};
        f = QtFuture::makeReadyRangeFuture(c_array);
        QCOMPARE_EQ(f.resultCount(), 3);
        QCOMPARE_EQ(f.results(), expectedResult);
    }
}

void tst_QFuture::continuationsAfterReadyFutures()
{
    // continuations without a context
    {
        QFuture<int> f = QtFuture::makeReadyValueFuture(42)
                .then([](int val) {
                    return val + 10;
                })
                .onCanceled([]() {
                    return -1;
                });
        QCOMPARE(f.result(), 52);
    }
    {
        auto rangeF = QtFuture::makeReadyRangeFuture({1, 2, 3});
        QFuture<int> f = rangeF
                .then([vals = rangeF.results()](auto) {
                    return vals.last();
                })
                .onCanceled([]() {
                    return -1;
                });
        QCOMPARE(f.result(), 3);
    }
    {
        QFuture<int> f = QtFuture::makeReadyVoidFuture()
                .then([]() {
                    return 1;
                })
                .onCanceled([]() {
                    return -1;
                });
        QCOMPARE(f.result(), 1);
    }
#ifndef QT_NO_EXCEPTIONS
    {
        QException e;
        QFuture<int> f = QtFuture::makeExceptionalFuture<int>(e)
                .then([](int) {
                    return 1;
                })
                .onCanceled([]() {
                    return -1;
                })
                .onFailed([](const QException &) {
                    return -2;
                });
        QCOMPARE(f.result(), -2);
    }
#endif

    // continuations with a context
    QObject context;
    {
        QFuture<int> f = QtFuture::makeReadyValueFuture(42)
                .then(&context, [](int val) {
                    return val + 10;
                })
                .onCanceled([]() {
                    return -1;
                });
        QCOMPARE(f.result(), 52);
    }
    {
        auto rangeF = QtFuture::makeReadyRangeFuture({1, 2, 3});
        QFuture<int> f = rangeF
                .then(&context, [vals = rangeF.results()](auto) {
                    return vals.last();
                })
                .onCanceled([]() {
                    return -1;
                });
        QCOMPARE(f.result(), 3);
    }
    {
        QFuture<int> f = QtFuture::makeReadyVoidFuture()
                .then(&context, []() {
                    return 1;
                })
                .onCanceled([]() {
                    return -1;
                });
        QCOMPARE(f.result(), 1);
    }
#ifndef QT_NO_EXCEPTIONS
    {
        QException e;
        QFuture<int> f = QtFuture::makeExceptionalFuture<int>(e)
                .then(&context, [](int) {
                    return 1;
                })
                .onCanceled([]() {
                    return -1;
                })
                .onFailed([](const QException &) {
                    return -2;
                });
        QCOMPARE(f.result(), -2);
    }
#endif
}

void tst_QFuture::getFutureInterface()
{
    const int val = 42;
    QFuture<int> f = QtFuture::makeReadyValueFuture(val);

    auto interface = QFutureInterfaceBase::get(f);
    QCOMPARE(interface.resultCount(), 1);
}

void tst_QFuture::convertQMetaType()
{
    const auto intType = QMetaType::fromType<QFuture<int>>();
    const auto voidType = QMetaType::fromType<QFuture<void>>();

    QVERIFY(QMetaType::canConvert(intType, voidType));

    const int val = 42;
    QFuture<int> f = QtFuture::makeReadyValueFuture(val);
    auto variant = QVariant::fromValue(f);
    QVERIFY(variant.convert(voidType));

    const auto voidFuture = variant.value<QFuture<void>>();
    QVERIFY(voidFuture.isValid());
    QVERIFY(voidFuture.isFinished());
}

template<class OutputContainer>
void testWhenAllIterators()
{
    QPromise<int> p0;
    QPromise<int> p1;
    QPromise<int> p2;
    QList<QFuture<int>> futures = { p0.future(), p1.future(), p2.future() };

    bool finished = false;
    QFuture<OutputContainer> whenAll;
    if constexpr (std::is_same_v<QList<QFuture<int>>, OutputContainer>)
        whenAll = QtFuture::whenAll(futures.begin(), futures.end());
    else
        whenAll = QtFuture::whenAll<OutputContainer>(futures.begin(), futures.end());
    whenAll.then([&](const OutputContainer &output) {
        QCOMPARE(output.size(), 3u);
        QCOMPARE(output[0].result(), 0);
        QCOMPARE(output[1].result(), 1);
        QCOMPARE(output[2].result(), 2);
        finished = true;
    });
    QVERIFY(whenAll.isRunning());

    p0.start();
    p0.addResult(0);
    p0.finish();
    QVERIFY(whenAll.isRunning());

    p2.start();
    p2.addResult(2);
    p2.finish();
    QVERIFY(whenAll.isRunning());

    p1.start();
    p1.addResult(1);
    p1.finish();
    QVERIFY(!whenAll.isRunning());
    QVERIFY(finished);

    // Try with empty sequence
    QFuture<OutputContainer> whenAllEmpty;
    if constexpr (std::is_same_v<QList<QFuture<int>>, OutputContainer>)
        whenAllEmpty = QtFuture::whenAll(futures.end(), futures.end());
    else
        whenAllEmpty = QtFuture::whenAll<OutputContainer>(futures.end(), futures.end());
    QVERIFY(whenAllEmpty.isStarted());
    QVERIFY(whenAllEmpty.isFinished());
    QVERIFY(whenAllEmpty.result().empty());
}

void tst_QFuture::whenAllIterators()
{
    // Try with different output containers
    testWhenAllIterators<QList<QFuture<int>>>();
    if (QTest::currentTestFailed())
        QSKIP("testWhenAllIterators() with QList failed!");

    testWhenAllIterators<std::vector<QFuture<int>>>();
    if (QTest::currentTestFailed())
        QSKIP("testWhenAllIterators() with std::vector failed!");

    testWhenAllIterators<QVarLengthArray<QFuture<int>>>();
    if (QTest::currentTestFailed())
        QSKIP("testWhenAllIterators() with QVarLengthArray failed!");
}

void tst_QFuture::whenAllIteratorsWithCanceled()
{
    QPromise<int> p0;
    QPromise<int> p1;
    QList<QFuture<int>> futures = { p0.future(), p1.future() };
    bool finished = false;
    auto whenAll = QtFuture::whenAll(futures.begin(), futures.end())
                           .then([&](const QList<QFuture<int>> &results) {
                               QCOMPARE(results.size(), 2);
                               QVERIFY(results[0].isCanceled());
                               QVERIFY(!results[1].isCanceled());
                               QCOMPARE(results[1].result(), 1);
                               finished = true;
                           });

    p0.start();
    p0.future().cancel();
    p0.finish();
    QVERIFY(!finished);

    p1.start();
    p1.addResult(1);
    p1.finish();
    QVERIFY(finished);
}

void tst_QFuture::whenAllIteratorsWithFailed()
{
#ifndef QT_NO_EXCEPTIONS
    QPromise<int> p0;
    QPromise<int> p1;
    QList<QFuture<int>> futures = { p0.future(), p1.future() };
    bool finished = false;
    auto whenAll = QtFuture::whenAll(futures.begin(), futures.end())
                           .then([&](QList<QFuture<int>> results) {
                               QCOMPARE(results.size(), 2);
                               QCOMPARE(results[1].result(), 1);
                               // A shorter way of handling the exception
                               results[0].onFailed([&](const QException &) {
                                   finished = true;
                                   return 0;
                               });
                           });

    p0.start();
    p0.setException(QException());
    p0.finish();
    QVERIFY(!finished);

    p1.start();
    p1.addResult(1);
    p1.finish();
    QVERIFY(finished);
#else
    QSKIP("Exceptions are disabled, skipping the test")
#endif
}

// A helper for std::visit, see https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts>
struct overloaded : public Ts...
{
    using Ts::operator()...;
};

// explicit deduction guide
template<class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

template<class OutputContainer>
void testWhenAllDifferentTypes()
{
    QPromise<int> pInt1;
    QPromise<int> pInt2;
    QPromise<void> pVoid;

    using Futures = std::variant<QFuture<int>, QFuture<int>, QFuture<void>>;

    QFuture<OutputContainer> whenAll;
    if constexpr (std::is_same_v<QList<Futures>, OutputContainer>) {
        whenAll = QtFuture::whenAll(pInt1.future(), pInt2.future(), pVoid.future());
    } else {
        whenAll =
                QtFuture::whenAll<OutputContainer>(pInt1.future(), pInt2.future(), pVoid.future());
    }

    int sumOfInts = 0;
    whenAll.then([&](const OutputContainer &results) {
        for (auto future : results) {
            std::visit(overloaded {
                               [&](const QFuture<int> &f) {
                                   QVERIFY(f.isFinished());
                                   sumOfInts += f.result();
                               },
                               [](const QFuture<void> &f) { QVERIFY(f.isFinished()); },
                       },
                       future);
        }
    });

    pVoid.start();
    pVoid.finish();
    QVERIFY(whenAll.isRunning());

    pInt2.start();
    pInt2.addResult(2);
    pInt2.finish();
    QVERIFY(whenAll.isRunning());
    QCOMPARE(sumOfInts, 0);

    pInt1.start();
    pInt1.addResult(1);
    pInt1.finish();
    QVERIFY(!whenAll.isRunning());
    QCOMPARE(sumOfInts, 3);
}

void tst_QFuture::whenAllDifferentTypes()
{
    using Futures = std::variant<QFuture<int>, QFuture<int>, QFuture<void>>;
    testWhenAllDifferentTypes<QList<Futures>>();
    if (QTest::currentTestFailed())
        QSKIP("testWhenAllDifferentTypes() with QList failed!");

    testWhenAllDifferentTypes<std::vector<Futures>>();
    if (QTest::currentTestFailed())
        QSKIP("testWhenAllDifferentTypes() with std::vector failed!");

    testWhenAllDifferentTypes<QVarLengthArray<Futures>>();
    if (QTest::currentTestFailed())
        QSKIP("testWhenAllDifferentTypes() with QVarLengthArray failed!");
}

void tst_QFuture::whenAllDifferentTypesWithCanceled()
{
    QPromise<int> pInt;
    QPromise<QString> pString;

    const QString someValue = u"some value"_s;

    bool finished = false;
    using Futures = std::variant<QFuture<int>, QFuture<QString>>;
    auto whenAll = QtFuture::whenAll(pInt.future(), pString.future())
                           .then([&](const QList<Futures> &results) {
                               finished = true;
                               for (auto future : results) {
                                   std::visit(overloaded {
                                                      [](const QFuture<int> &f) {
                                                          QVERIFY(f.isFinished());
                                                          QVERIFY(f.isCanceled());
                                                      },
                                                      [&](const QFuture<QString> &f) {
                                                          QVERIFY(f.isFinished());
                                                          QCOMPARE(f.result(), someValue);
                                                      },
                                              },
                                              future);
                               }
                           });

    pString.start();
    pString.addResult(someValue);
    pString.finish();
    QVERIFY(!finished);

    pInt.start();
    pInt.future().cancel();
    pInt.finish();
    QVERIFY(finished);
}

void tst_QFuture::whenAllDifferentTypesWithFailed()
{
#ifndef QT_NO_EXCEPTIONS
    QPromise<int> pInt;
    QPromise<QString> pString;

    const QString someValue = u"some value"_s;

    bool finished = false;
    using Futures = std::variant<QFuture<int>, QFuture<QString>>;
    auto whenAll = QtFuture::whenAll(pInt.future(), pString.future())
                           .then([&](const QList<Futures> &results) {
                               finished = true;
                               for (auto future : results) {
                                   std::visit(overloaded {
                                                      [](QFuture<int> f) {
                                                          QVERIFY(f.isFinished());
                                                          bool failed = false;
                                                          // A shorter way of handling the exception
                                                          f.onFailed([&](const QException &) {
                                                              failed = true;
                                                              return -1;
                                                          });
                                                          QVERIFY(failed);
                                                      },
                                                      [&](const QFuture<QString> &f) {
                                                          QVERIFY(f.isFinished());
                                                          QCOMPARE(f.result(), someValue);
                                                      },
                                              },
                                              future);
                               }
                           });

    pInt.start();
    pInt.setException(QException());
    pInt.finish();
    QVERIFY(!finished);

    pString.start();
    pString.addResult(someValue);
    pString.finish();
    QVERIFY(finished);
#else
    QSKIP("Exceptions are disabled, skipping the test")
#endif
}

void tst_QFuture::whenAnyIterators()
{
    QPromise<int> p0;
    QPromise<int> p1;
    QPromise<int> p2;
    QList<QFuture<int>> futures = { p0.future(), p1.future(), p2.future() };

    auto whenAny = QtFuture::whenAny(futures.begin(), futures.end());
    int count = 0;
    whenAny.then([&](const QtFuture::WhenAnyResult<int> &result) {
        QCOMPARE(result.index, 1);
        QCOMPARE(result.future.result(), 1);
        QVERIFY(!futures[0].isFinished());
        QVERIFY(futures[1].isFinished());
        QVERIFY(!futures[2].isFinished());
        ++count;
    });

    p0.start();
    p1.start();
    p2.start();
    p0.addResult(0);
    p1.addResult(1);
    p2.addResult(2);
    QVERIFY(!whenAny.isFinished());
    QCOMPARE(count, 0);

    p1.finish();
    QVERIFY(whenAny.isFinished());
    QCOMPARE(count, 1);

    p0.finish();
    QCOMPARE(count, 1);

    p2.finish();
    QCOMPARE(count, 1);

    auto whenAnyEmpty = QtFuture::whenAny(futures.end(), futures.end());
    QVERIFY(whenAnyEmpty.isStarted());
    QVERIFY(whenAnyEmpty.isFinished());
    QCOMPARE(whenAnyEmpty.result().index, -1);
    auto whenAnyEmptyResult = whenAnyEmpty.result().future;
    QVERIFY(whenAnyEmptyResult.isStarted());
    QVERIFY(whenAnyEmptyResult.isFinished());
    QVERIFY(whenAnyEmptyResult.isCanceled());
}

void tst_QFuture::whenAnyIteratorsWithCanceled()
{
    QPromise<int> p0;
    QPromise<int> p1;
    QList<QFuture<int>> futures = { p0.future(), p1.future() };
    int count = 0;
    auto whenAny = QtFuture::whenAny(futures.begin(), futures.end())
                           .then([&](const QtFuture::WhenAnyResult<int> &result) {
                               QCOMPARE(result.index, 1);
                               QVERIFY(result.future.isCanceled());
                               QVERIFY(!futures[0].isFinished());
                               QVERIFY(futures[1].isFinished());
                               ++count;
                           });

    p1.start();
    p1.future().cancel();
    p1.finish();
    QVERIFY(whenAny.isFinished());
    QCOMPARE(count, 1);

    p0.start();
    p0.addResult(0);
    p0.finish();
    QCOMPARE(count, 1);
}

void tst_QFuture::whenAnyIteratorsWithFailed()
{
#ifndef QT_NO_EXCEPTIONS
    QPromise<int> p0;
    QPromise<int> p1;
    QList<QFuture<int>> futures = { p0.future(), p1.future() };
    int count = 0;
    auto whenAny = QtFuture::whenAny(futures.begin(), futures.end())
                           .then([&](QtFuture::WhenAnyResult<int> result) {
                               QCOMPARE(result.index, 1);
                               QVERIFY(p1.future().isFinished());
                               QVERIFY(!p0.future().isFinished());
                               // A shorter way of handling the exception
                               result.future.onFailed([&](const QException &) {
                                   ++count;
                                   return 0;
                               });
                           });

    p1.start();
    p1.setException(QException());
    p1.finish();
    QCOMPARE(count, 1);

    p0.start();
    p0.addResult(0);
    p0.finish();
    QCOMPARE(count, 1);
#else
    QSKIP("Exceptions are disabled, skipping the test")
#endif
}

void tst_QFuture::whenAnyDifferentTypes()
{
    QPromise<int> pInt1;
    QPromise<int> pInt2;
    QPromise<void> pVoid;

    auto whenAny = QtFuture::whenAny(pInt1.future(), pInt2.future(), pVoid.future());
    int count = 0;
    whenAny.then([&](const std::variant<QFuture<int>, QFuture<int>, QFuture<void>> &result) {
        QCOMPARE(result.index(), 1u);
        std::visit(overloaded { [&](const QFuture<int> &future) {
                                    QVERIFY(future.isFinished());
                                    QCOMPARE(future.result(), 2);
                                    ++count;
                                },
                                [](auto) { QFAIL("The wrong future completed."); }
                   },
                   result);
    });

    pInt2.start();
    pInt1.start();
    pVoid.start();
    pInt1.addResult(1);
    pInt2.addResult(2);

    QVERIFY(!whenAny.isFinished());
    QCOMPARE(count, 0);

    pInt2.finish();
    QVERIFY(whenAny.isFinished());
    QCOMPARE(count, 1);

    pInt1.finish();
    QCOMPARE(count, 1);

    pVoid.finish();
    QCOMPARE(count, 1);
}

void tst_QFuture::whenAnyDifferentTypesWithCanceled()
{
    QPromise<int> pInt;
    QPromise<void> pVoid;

    int count = 0;
    auto whenAny = QtFuture::whenAny(pInt.future(), pVoid.future())
                           .then([&](const std::variant<QFuture<int>, QFuture<void>> &result) {
                               QCOMPARE(result.index(), 0u);
                               std::visit(overloaded { [&](const QFuture<int> &future) {
                                                          QVERIFY(future.isFinished());
                                                          QVERIFY(future.isCanceled());
                                                          ++count;
                                                       },
                                                       [](auto) {
                                                           QFAIL("The wrong future completed.");
                                                       }
                                          },
                                          result);
                           });

    pInt.start();
    pInt.future().cancel();
    pInt.finish();
    QCOMPARE(count, 1);

    pVoid.start();
    pVoid.finish();
    QCOMPARE(count, 1);
}

void tst_QFuture::whenAnyDifferentTypesWithFailed()
{
#ifndef QT_NO_EXCEPTIONS
    QPromise<int> pInt;
    QPromise<void> pVoid;

    int count = 0;
    auto whenAny = QtFuture::whenAny(pInt.future(), pVoid.future())
                           .then([&](const std::variant<QFuture<int>, QFuture<void>> &result) {
                               QCOMPARE(result.index(), 0u);
                               std::visit(overloaded { [&](QFuture<int> future) {
                                                          QVERIFY(future.isFinished());
                                                          // A shorter way of handling the exception
                                                          future.onFailed([&](const QException &) {
                                                              ++count;
                                                              return -1;
                                                          });
                                                       },
                                                       [](auto) {
                                                           QFAIL("The wrong future completed.");
                                                       }
                                          },
                                          result);
                           });

    pInt.start();
    pInt.setException(QException());
    pInt.finish();
    QCOMPARE(count, 1);

    pVoid.start();
    pVoid.finish();
    QCOMPARE(count, 1);
#else
    QSKIP("Exceptions are disabled, skipping the test")
#endif
}

void tst_QFuture::continuationOverride()
{
    QPromise<int> p;
    bool firstExecuted = false;
    bool secondExecuted = false;

    QTest::ignoreMessage(QtWarningMsg,
                         "Adding a continuation to a future which already has a continuation. "
                         "The existing continuation is overwritten.");

    QFuture<int> f1 = p.future();
    f1.then([&firstExecuted](int) {
        firstExecuted = true;
    });

    QFuture<int> f2 = p.future();
    f2.then([&secondExecuted](int) {
        secondExecuted = true;
    });

    p.start();
    p.addResult(42);
    p.finish();

    QVERIFY(p.future().isFinished());
    QVERIFY(!firstExecuted);
    QVERIFY(secondExecuted);
}

struct InstanceCounter
{
    InstanceCounter() { ++count; }
    InstanceCounter(const InstanceCounter &) { ++count; }
    ~InstanceCounter() { --count; }
    static int count;
};
int InstanceCounter::count = 0;

void tst_QFuture::continuationsDontLeak()
{
    {
        // QFuture isn't started and isn't finished (has no state)
        QPromise<InstanceCounter> promise;
        auto future = promise.future();

        bool continuationIsRun = false;
        future.then([future, &continuationIsRun](InstanceCounter) { continuationIsRun = true; });

        promise.addResult(InstanceCounter {});

        QVERIFY(!continuationIsRun);
    }
    QCOMPARE(InstanceCounter::count, 0);

    {
        // QFuture is started, but not finished
        QPromise<InstanceCounter> promise;
        auto future = promise.future();

        bool continuationIsRun = false;
        future.then([future, &continuationIsRun](InstanceCounter) { continuationIsRun = true; });

        promise.start();
        promise.addResult(InstanceCounter {});

        QVERIFY(!continuationIsRun);
    }
    QCOMPARE(InstanceCounter::count, 0);

    {
        // QFuture is started and finished, the continuation is run
        QPromise<InstanceCounter> promise;
        auto future = promise.future();

        bool continuationIsRun = false;
        future.then([future, &continuationIsRun](InstanceCounter) {
            QVERIFY(future.isFinished());
            continuationIsRun = true;
        });

        promise.start();
        promise.addResult(InstanceCounter {});
        promise.finish();

        QVERIFY(continuationIsRun);
    }
    QCOMPARE(InstanceCounter::count, 0);

    {
        // QTBUG-116731: Must pass with ASan enabled
        bool continuationIsRun = false;
        auto f = QtFuture::makeReadyValueFuture(42);
        QtFuture::whenAll(f).then([&](auto) { continuationIsRun = true; });
        QVERIFY(continuationIsRun);
    }

    {
        // QTBUG-116731: Must pass with ASan enabled
        bool continuationIsRun = false;
        auto f = QtFuture::makeReadyValueFuture(42);
        QList fs{f};
        QtFuture::whenAll(fs.begin(), fs.end()).then([&](auto) { continuationIsRun = true; });
        QVERIFY(continuationIsRun);
    }

    {
        // QTBUG-116731: Must pass with ASan enabled
        bool continuationIsRun = false;
        auto f = QtFuture::makeReadyValueFuture(42);
        QtFuture::whenAny(f).then([&](auto) { continuationIsRun = true; });
        QVERIFY(continuationIsRun);
    }

    {
        // QTBUG-116731: Must pass with ASan enabled
        bool continuationIsRun = false;
        auto f = QtFuture::makeReadyValueFuture(42);
        QList fs{f};
        QtFuture::whenAny(fs.begin(), fs.end()).then([&](auto) { continuationIsRun = true; });
        QVERIFY(continuationIsRun);
    }
}

// This test checks that we do not get use-after-free
void tst_QFuture::cancelAfterFinishWithContinuations()
{
    QFuture<void> future;
    bool continuationIsRun = false;
    bool cancelCalled = false;
    {
        QPromise<void> promise;
        future = promise.future();

        future.then([&continuationIsRun]() {
            continuationIsRun = true;
        }).onCanceled([&cancelCalled]() {
            cancelCalled = true;
        });

        promise.start();
        promise.finish();
    }

    QVERIFY(continuationIsRun);
    future.cancel();
    QVERIFY(!cancelCalled);
}

void tst_QFuture::unwrap()
{
    // The nested future succeeds
    {
        QPromise<int> p;
        QFuture<QFuture<int>> f = p.future().then([] (int value) {
            QFuture<int> nested = QtConcurrent::run([value] {
                return value + 1;
            });
            return nested;
        });

        QFuture<int> unwrapped = f.unwrap();
        QVERIFY(!unwrapped.isStarted());
        QVERIFY(!unwrapped.isFinished());

        p.start();
        p.addResult(42);
        p.finish();

        unwrapped.waitForFinished();

        QVERIFY(unwrapped.isStarted());
        QVERIFY(unwrapped.isFinished());
        QCOMPARE(unwrapped.result(), 43);
    }

    // The nested future succeeds with multiple results
    {
        QPromise<int> p;
        QFuture<QFuture<int>> f = p.future().then([] (int value) {
            QPromise<int> nested;
            nested.start();
            nested.addResult(++value);
            nested.addResult(++value);
            nested.addResult(++value);
            nested.finish();
            return nested.future();
        });

        QFuture<int> unwrapped = f.unwrap();
        QVERIFY(!unwrapped.isStarted());
        QVERIFY(!unwrapped.isFinished());

        p.start();
        p.addResult(42);
        p.finish();

        f.waitForFinished();

        QVERIFY(unwrapped.isStarted());
        QVERIFY(unwrapped.isFinished());
        QCOMPARE(unwrapped.results(), QList<int>() << 43 << 44 << 45);
    }

    // The chain is canceled, check that unwrap() propagates the cancellation.
    {
        QPromise<int> p;
        QFuture<int> f = p.future().then([] (int value) {
            QFuture<int> nested = QtConcurrent::run([value] {
                return value + 1;
            });
            return nested;
        }).unwrap().then([] (int result) {
            return result;
        }).onCanceled([] {
            return -1;
        });

        p.start();
        p.future().cancel();
        p.finish();

        f.waitForFinished();

        QVERIFY(f.isStarted());
        QVERIFY(f.isFinished());
        QCOMPARE(f.result(), -1);
    }

#ifndef QT_NO_EXCEPTIONS
    // The chain has an exception, check that unwrap() propagates it.
    {
        QPromise<int> p;
        QFuture<int> f = p.future().then([] (int value) {
            QFuture<int> nested = QtConcurrent::run([value] {
                return value + 1;
            });
            return nested;
        }).unwrap().then([] (int result) {
            return result;
        }).onFailed([] (QException &) {
            return -1;
        });

        p.start();
        p.setException(QException());
        p.finish();

        f.waitForFinished();

        QVERIFY(f.isStarted());
        QVERIFY(f.isFinished());
        QCOMPARE(f.result(), -1);
    }

#endif // QT_NO_EXCEPTIONS

    // The nested future is canceled
    {
        QPromise<int> p;
        QFuture<int> f = p.future().then([] (int value) {
            QFuture<int> nested = QtConcurrent::run([value] {
                return value + 1;
            });
            nested.cancel();
            return nested;
        }).unwrap().then([] (int result) {
            return result;
        }).onCanceled([] {
            return -1;
        });

        p.start();
        p.addResult(42);
        p.finish();

        f.waitForFinished();

        QVERIFY(f.isStarted());
        QVERIFY(f.isFinished());
        QCOMPARE(f.result(), -1);
    }

#ifndef QT_NO_EXCEPTIONS
    // The nested future fails with an exception
    {
        QPromise<int> p;
        QFuture<int> f = p.future().then([] (int value) {
            QFuture<int> nested = QtConcurrent::run([value] {
                throw QException();
                return value + 1;
            });
            return nested;
        }).unwrap().then([] (int result) {
            return result;
        }).onFailed([] (QException &) {
            return -1;
        });

        p.start();
        p.addResult(42);
        p.finish();

        f.waitForFinished();

        QVERIFY(f.isStarted());
        QVERIFY(f.isFinished());
        QCOMPARE(f.result(), -1);
    }
#endif // QT_NO_EXCEPTIONS

    // Check that continuations are called in the right order
    {
        QPromise<void> p;

        std::atomic<bool> firstThenInvoked = false;
        std::atomic<bool> secondThenInvoked = false;
        std::atomic<bool> nestedThenInvoked = false;
        auto f = p.future().then([&] {
            if (!firstThenInvoked && !secondThenInvoked && !nestedThenInvoked)
                firstThenInvoked = true;
            QFuture<void> nested = QtConcurrent::run([&] {
                QVERIFY(firstThenInvoked);
                QVERIFY(!nestedThenInvoked);
                QVERIFY(!secondThenInvoked);
                nestedThenInvoked = true;
            });
            return nested;
        }).unwrap().then([&] {
            QVERIFY(firstThenInvoked);
            QVERIFY(nestedThenInvoked);
            QVERIFY(!secondThenInvoked);
            secondThenInvoked = true;
        });

        QVERIFY(!firstThenInvoked);
        QVERIFY(!nestedThenInvoked);
        QVERIFY(!secondThenInvoked);

        p.start();
        p.finish();

        f.waitForFinished();

        if (QTest::currentTestFailed())
            return;

        QVERIFY(firstThenInvoked);
        QVERIFY(nestedThenInvoked);
        QVERIFY(secondThenInvoked);
    }

    // Unwrap multiple nested futures
    {
        QPromise<int> p;
        QFuture<QFuture<QFuture<int>>> f = p.future().then([] (int value) {
            QFuture<QFuture<int>> nested = QtConcurrent::run([value] {
                QFuture<int> doubleNested = QtConcurrent::run([value] {
                    return value + 1;
                });
                return doubleNested;
            });
            return nested;
        });

        QFuture<int> unwrapped = f.unwrap();
        QVERIFY(!unwrapped.isStarted());
        QVERIFY(!unwrapped.isFinished());

        p.start();
        p.addResult(42);
        p.finish();

        unwrapped.waitForFinished();

        QVERIFY(unwrapped.isStarted());
        QVERIFY(unwrapped.isFinished());
        QCOMPARE(unwrapped.result(), 43);
    }

    // Unwrap multiple nested void futures
    {
        QPromise<void> p;
        std::atomic<bool> nestedInvoked = false;
        std::atomic<bool> doubleNestedInvoked = false;
        QFuture<QFuture<QFuture<void>>> f = p.future().then([&] {
            QFuture<QFuture<void>> nested = QtConcurrent::run([&] {
                QFuture<void> doubleNested = QtConcurrent::run([&] {
                    doubleNestedInvoked = true;
                });
                nestedInvoked = true;
                return doubleNested;
            });
            return nested;
        });

        QFuture<void> unwrapped = f.unwrap();
        QVERIFY(!nestedInvoked);
        QVERIFY(!doubleNestedInvoked);
        QVERIFY(!unwrapped.isStarted());
        QVERIFY(!unwrapped.isFinished());

        p.start();
        p.finish();

        unwrapped.waitForFinished();

        QVERIFY(unwrapped.isStarted());
        QVERIFY(unwrapped.isFinished());
        QVERIFY(nestedInvoked);
        QVERIFY(doubleNestedInvoked);
    }
}

QTEST_MAIN(tst_QFuture)
#include "tst_qfuture.moc"
