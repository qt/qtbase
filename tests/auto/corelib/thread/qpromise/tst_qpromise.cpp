// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QCoreApplication>
#include <QDebug>

#define QPROMISE_TEST

#include <QTest>
#include <qfuture.h>
#include <qfuturewatcher.h>
#include <qpromise.h>
#include <QtCore/qsemaphore.h>

#include <algorithm>
#include <memory>
#include <QtCore/q20type_traits.h>
#include <chrono>

using namespace std::chrono_literals;

template <typename T>
struct promise_type {};
template <typename T>
struct promise_type<QPromise<T>> : q20::type_identity<T> {};
template <typename T>
using promise_type_t = typename promise_type<T>::type;

class tst_QPromise : public QObject
{
    Q_OBJECT
private slots:
    // simple test cases
    void promise();
    void futureFromPromise();
    void addResult();
    void addResultWithBracedInitializer();
    void addResultOutOfOrder();
#ifndef QT_NO_EXCEPTIONS
    void setException();
    void setExceptionAfterCancelDoesNothing(); // QTBUG-128405
    void setExceptionAfterFinishDoesNothing();
#endif
    void cancel();
    void progress();

    // complicated test cases
    void addInThread();
    void addInThreadMoveOnlyObject();  // separate test case - QTBUG-84736
    void reportFromMultipleThreads();
    void reportFromMultipleThreadsByMovedPromise();
    void doNotCancelWhenFinished();
#ifndef QT_NO_EXCEPTIONS
    void cancelWhenDestroyed();
#endif
    void cancelWhenReassigned();
    void cancelWhenDestroyedWithoutStarting();
    void cancelWhenDestroyedRunsContinuations();
    void cancelWhenDestroyedWithFailureHandler();  // QTBUG-114606
    void continuationsRunWhenFinished();
    void finishWhenSwapped();
    void cancelWhenMoved();
    void waitUntilResumed();
    void waitUntilCanceled();

    // snippets (external):
    void snippet_basicExample();
    void snippet_multithreadExample();
    void snippet_suspendExample();
};

struct TrivialType { int field = 0; };
struct CopyOnlyType {
    constexpr CopyOnlyType(int field = 0) noexcept : field(field) {}
    CopyOnlyType(const CopyOnlyType &) = default;
    CopyOnlyType& operator=(const CopyOnlyType &) = default;
    ~CopyOnlyType() = default;

    int field;
};
struct MoveOnlyType {
    Q_DISABLE_COPY(MoveOnlyType)
    constexpr MoveOnlyType(int field = 0) noexcept : field(field) {}
    MoveOnlyType(MoveOnlyType &&) = default;
    MoveOnlyType& operator=(MoveOnlyType &&) = default;
    ~MoveOnlyType() = default;

    int field;
};
bool operator==(const CopyOnlyType &a, const CopyOnlyType &b) { return a.field == b.field; }
bool operator==(const MoveOnlyType &a, const MoveOnlyType &b) { return a.field == b.field; }

// A wrapper for a test function, calls the function, if it fails, reports failure
#define RUN_TEST_FUNC(test, ...) \
do { \
    test(__VA_ARGS__); \
    if (QTest::currentTestFailed()) \
        QFAIL("Test case " #test "(" #__VA_ARGS__ ") failed"); \
} while (false)

// std::thread-like wrapper that ensures that the thread is joined at the end of
// a scope to prevent potential std::terminate
struct ThreadWrapper
{
    std::unique_ptr<QThread> t;
    template<typename Function>
    ThreadWrapper(Function &&f) : t(QThread::create(std::forward<Function>(f)))
    {
        t->start();
    }
    void join() { QVERIFY(t->wait(60s)); }
    ~ThreadWrapper() noexcept(false)
    {
        join();
    }
};

struct FutureWatcher
{
    bool thenCalled = false;
    bool onFailedCalled = false;
    bool onCanceledCalled = false;

    FutureWatcher() = default;
    Q_DISABLE_COPY_MOVE(FutureWatcher)

    template <typename T>
    explicit FutureWatcher(QFuture<T> &f) { setFuture(f); }

    template <typename T>
    void setFuture(QFuture<T> &f)
    {
        f.onFailed([&]{
                       onFailedCalled = true;
                       if constexpr (!std::is_void_v<T>)
                           return T{};
                   })
         .then([&](auto&&...) { thenCalled = true; })
         .onCanceled([&]{ onCanceledCalled = true; });
    }
};

void tst_QPromise::promise()
{
    const auto testCanCreatePromise = [] (auto promise) {
        promise.start();
        promise.suspendIfRequested();  // should not block on its own
        promise.finish();
    };

    RUN_TEST_FUNC(testCanCreatePromise, QPromise<void>());
    RUN_TEST_FUNC(testCanCreatePromise, QPromise<int>());
    RUN_TEST_FUNC(testCanCreatePromise, QPromise<QList<float>>());
    RUN_TEST_FUNC(testCanCreatePromise, QPromise<TrivialType>());
    RUN_TEST_FUNC(testCanCreatePromise, QPromise<CopyOnlyType>());
    RUN_TEST_FUNC(testCanCreatePromise, QPromise<MoveOnlyType>());
}

void tst_QPromise::futureFromPromise()
{
    const auto testCanCreateFutureFromPromise = [] (auto promise) {
        auto future = promise.future();
        QVERIFY(!future.isValid());

        promise.start();
        QCOMPARE(future.isStarted(), true);
        QVERIFY(future.isValid());

        promise.finish();
        QCOMPARE(future.isFinished(), true);
        QVERIFY(future.isValid());

        future.waitForFinished();
    };

    RUN_TEST_FUNC(testCanCreateFutureFromPromise, QPromise<void>());
    RUN_TEST_FUNC(testCanCreateFutureFromPromise, QPromise<double>());
    RUN_TEST_FUNC(testCanCreateFutureFromPromise, QPromise<QList<int>>());
    RUN_TEST_FUNC(testCanCreateFutureFromPromise, QPromise<TrivialType>());
    RUN_TEST_FUNC(testCanCreateFutureFromPromise, QPromise<CopyOnlyType>());
    RUN_TEST_FUNC(testCanCreateFutureFromPromise, QPromise<MoveOnlyType>());
}

void tst_QPromise::addResult()
{
    QPromise<int> promise;
    auto f = promise.future();

    // add as lvalue
    int resultAt0 = 456;
    {
        QVERIFY(promise.addResult(resultAt0));
        QCOMPARE(f.resultCount(), 1);
        QCOMPARE(f.result(), resultAt0);
        QCOMPARE(f.resultAt(0), resultAt0);
    }
    // add as rvalue
    {
        int result = 789;
        QVERIFY(promise.addResult(789));
        QCOMPARE(f.resultCount(), 2);
        QCOMPARE(f.resultAt(1), result);
    }
    // add at position
    {
        int result = 56238;
        QVERIFY(promise.addResult(result, 2));
        QCOMPARE(f.resultCount(), 3);
        QCOMPARE(f.resultAt(2), result);
    }
    // add multiple results in one go:
    {
        QList results = {42, 4242, 424242};
        QVERIFY(promise.addResults(results));
        QCOMPARE(f.resultCount(), 6);
        QCOMPARE(f.resultAt(3), 42);
        QCOMPARE(f.resultAt(4), 4242);
        QCOMPARE(f.resultAt(5), 424242);
    }
    // add as lvalue at position and overwrite
    {
        int result = -1;
        const auto originalCount = f.resultCount();
        QVERIFY(!promise.addResult(result, 0));
        QCOMPARE(f.resultCount(), originalCount);
        QCOMPARE(f.resultAt(0), resultAt0); // overwrite does not work
    }
    // add as rvalue at position and overwrite
    {
        const auto originalCount = f.resultCount();
        QVERIFY(!promise.addResult(-1, 0));
        QCOMPARE(f.resultCount(), originalCount);
        QCOMPARE(f.resultAt(0), resultAt0); // overwrite does not work
    }
}

void tst_QPromise::addResultWithBracedInitializer() // QTBUG-111826
{
    struct MyClass
    {
        QString strValue;
        int intValue = 0;
#ifndef __cpp_aggregate_paren_init // make emplacement work with MyClass
        MyClass(QString s, int i) : strValue(std::move(s)), intValue(i) {}
#endif
    };

    {
        QPromise<MyClass> myPromise;
        myPromise.addResult({"bar", 1});
    }

    {
        QPromise<MyClass> myPromise;
        myPromise.emplaceResult("bar", 1);
    }
}

void tst_QPromise::addResultOutOfOrder()
{
    // Compare results available in QFuture to expected results
    const auto compareResults = [] (const auto &future, auto expected) {
        QCOMPARE(future.resultCount(), expected.size());
        // index based loop
        for (int i = 0; i < future.resultCount(); ++i)
            QCOMPARE(future.resultAt(i), expected.at(i));
        // iterator based loop
        QVERIFY(std::equal(future.begin(), future.end(), expected.begin()));
    };

    // out of order results without a gap
    {
        QPromise<int> promise;
        auto f = promise.future();
        QVERIFY(promise.addResult(456, 1));
        QCOMPARE(f.resultCount(), 0);
        QVERIFY(promise.addResult(123, 0));

        QList<int> expected({123, 456});
        RUN_TEST_FUNC(compareResults, f, expected);
        QCOMPARE(f.results(), expected);
    }

    // out of order results with a gap that is closed "later"
    {
        QPromise<int> promise;
        auto f = promise.future();
        QVERIFY(promise.addResult(0, 0));
        QVERIFY(promise.addResult(1, 1));
        QVERIFY(promise.addResult(3, 3));  // intentional gap here

        QList<int> expectedWhenGapExists({0, 1});
        RUN_TEST_FUNC(compareResults, f, expectedWhenGapExists);
        QCOMPARE(f.resultAt(3), 3);

        QList<int> expectedWhenNoGap({0, 1, 2, 3});
        QVERIFY(promise.addResult(2, 2));  // fill a gap with a value
        RUN_TEST_FUNC(compareResults, f, expectedWhenNoGap);
        QCOMPARE(f.results(), expectedWhenNoGap);
    }
}

#ifndef QT_NO_EXCEPTIONS
void tst_QPromise::setException()
{
    struct TestException {};  // custom exception class
    const auto testExceptionCaught = [] (auto promise, const auto& exception) {
        auto f = promise.future();
        promise.start();
        promise.setException(exception);
        promise.finish();

        bool caught = false;
        try {
            f.waitForFinished();
        } catch (const QException&) {
            caught = true;
        } catch (const TestException&) {
            caught = true;
        }
        QVERIFY(caught);
    };

    RUN_TEST_FUNC(testExceptionCaught, QPromise<void>(), QException());
    RUN_TEST_FUNC(testExceptionCaught, QPromise<int>(), QException());
    RUN_TEST_FUNC(testExceptionCaught, QPromise<void>(),
                       std::make_exception_ptr(TestException()));
    RUN_TEST_FUNC(testExceptionCaught, QPromise<int>(),
                       std::make_exception_ptr(TestException()));
    RUN_TEST_FUNC(testExceptionCaught, QPromise<CopyOnlyType>(),
                       std::make_exception_ptr(TestException()));
    RUN_TEST_FUNC(testExceptionCaught, QPromise<MoveOnlyType>(),
                       std::make_exception_ptr(TestException()));
}

void tst_QPromise::setExceptionAfterCancelDoesNothing()
{
    struct TestException {};

    auto test = [](auto promise, auto exception) {
        auto f = promise.future();
        FutureWatcher r(f);
        QSemaphore sem;
        QSemaphoreReleaser rel(sem);
        ThreadWrapper t([&] {
            auto p = std::move(promise);
            p.start();
            sem.acquire(); // wait for cancel() in main thread
            p.setException(std::move(exception));
        });

        f.cancel();
        rel.cancel()->release(); // give a go for setException() in worker thread

        t.join();

        QVERIFY(f.isCanceled());
        QVERIFY(!r.thenCalled);
        QVERIFY(!r.onFailedCalled);
        QVERIFY(r.onCanceledCalled);
    };

    RUN_TEST_FUNC(test, QPromise<void>(), QException());
    RUN_TEST_FUNC(test, QPromise<int>(),  QException());
    RUN_TEST_FUNC(test, QPromise<void>(), std::make_exception_ptr(TestException()));
    RUN_TEST_FUNC(test, QPromise<int>(), std::make_exception_ptr(TestException()));
    RUN_TEST_FUNC(test, QPromise<CopyOnlyType>(), std::make_exception_ptr(TestException()));
    RUN_TEST_FUNC(test, QPromise<MoveOnlyType>(), std::make_exception_ptr(TestException()));
}

void tst_QPromise::setExceptionAfterFinishDoesNothing()
{
    struct TestException {};

    auto test = [](auto promise, auto exception) {
        auto f = promise.future();
        FutureWatcher r(f);
        promise.start();
        using T = promise_type_t<decltype(promise)>;
        if constexpr (!std::is_void_v<T>)
            promise.addResult(T());
        promise.finish();
        promise.setException(std::move(exception));

        QVERIFY(f.isFinished());
        QVERIFY(r.thenCalled);
        QVERIFY(!r.onFailedCalled);
        QVERIFY(!r.onCanceledCalled);
    };

    RUN_TEST_FUNC(test, QPromise<void>(), QException());
    RUN_TEST_FUNC(test, QPromise<int>(),  QException());
    RUN_TEST_FUNC(test, QPromise<void>(), std::make_exception_ptr(TestException()));
    RUN_TEST_FUNC(test, QPromise<int>(), std::make_exception_ptr(TestException()));
    RUN_TEST_FUNC(test, QPromise<CopyOnlyType>(), std::make_exception_ptr(TestException()));
    RUN_TEST_FUNC(test, QPromise<MoveOnlyType>(), std::make_exception_ptr(TestException()));
}
#endif

void tst_QPromise::cancel()
{
    const auto testCancel = [] (auto promise) {
        auto f = promise.future();
        f.cancel();
        QCOMPARE(promise.isCanceled(), true);
    };

    RUN_TEST_FUNC(testCancel, QPromise<void>());
    RUN_TEST_FUNC(testCancel, QPromise<int>());
    RUN_TEST_FUNC(testCancel, QPromise<CopyOnlyType>());
    RUN_TEST_FUNC(testCancel, QPromise<MoveOnlyType>());
}

void tst_QPromise::progress()
{
    const auto testProgress = [] (auto promise) {
        auto f = promise.future();

        promise.setProgressRange(0, 2);
        QCOMPARE(f.progressMinimum(), 0);
        QCOMPARE(f.progressMaximum(), 2);

        QCOMPARE(f.progressValue(), 0);
        promise.setProgressValue(1);
        QCOMPARE(f.progressValue(), 1);
        promise.setProgressValue(0);  // decrement
        QCOMPARE(f.progressValue(), 1);
        promise.setProgressValue(10);  // out of range
        QCOMPARE(f.progressValue(), 1);

        promise.setProgressRange(0, 100);
        promise.setProgressValueAndText(50, u8"50%");
        QCOMPARE(f.progressValue(), 50);
        QCOMPARE(f.progressText(), u8"50%");
    };

    RUN_TEST_FUNC(testProgress, QPromise<void>());
    RUN_TEST_FUNC(testProgress, QPromise<int>());
    RUN_TEST_FUNC(testProgress, QPromise<CopyOnlyType>());
    RUN_TEST_FUNC(testProgress, QPromise<MoveOnlyType>());
}

void tst_QPromise::addInThread()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires C++11 std threading enabled (and working) in the standard library.");
#endif
    const auto testAddResult = [] (auto promise, const auto &result) {
        promise.start();
        auto f = promise.future();
        // move construct QPromise
        ThreadWrapper thr([&] {
            auto p = std::move(promise);
            p.addResult(result);
        });
        // Waits for result first
        const auto actual = f.result();
        QCOMPARE(actual, result);
        QCOMPARE(f.resultAt(0), result);
    };

    RUN_TEST_FUNC(testAddResult, QPromise<int>(), 42);
    RUN_TEST_FUNC(testAddResult, QPromise<QString>(), u8"42");
    RUN_TEST_FUNC(testAddResult, QPromise<CopyOnlyType>(), CopyOnlyType{99});
}

void tst_QPromise::addInThreadMoveOnlyObject()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires C++11 std threading enabled (and working) in the standard library.");
#endif
    QPromise<MoveOnlyType> promise;
    promise.start();
    auto f = promise.future();

    ThreadWrapper thr([&] {
        auto p = std::move(promise);
        p.addResult(MoveOnlyType{-11});
    });

    // Iterators wait for result first
    for (auto& result : f)
        QCOMPARE(result, MoveOnlyType{-11});
}

void tst_QPromise::reportFromMultipleThreads()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires C++11 std threading enabled (and working) in the standard library.");
#endif
    QPromise<int> promise;
    auto f = promise.future();
    promise.start();

    ThreadWrapper threads[] = {
        ThreadWrapper([&promise] { promise.addResult(42); }),
        ThreadWrapper([&promise] { promise.addResult(43); }),
        ThreadWrapper([&promise] { promise.addResult(44); }),
    };
    for (auto& t : threads)
        t.join();
    promise.finish();

    QList<int> expected = {42, 43, 44};
    for (auto actual : f.results()) {
        QVERIFY(std::find(expected.begin(), expected.end(), actual) != expected.end());
        expected.removeOne(actual);
    }
}

void tst_QPromise::reportFromMultipleThreadsByMovedPromise()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires C++11 std threading enabled (and working) in the standard library.");
#endif
    QPromise<int> initialPromise;
    auto f = initialPromise.future();
    {
        // Move QPromise into local scope: local QPromise (as being
        // move-constructed) must be able to set results, QFuture must still
        // hold correct references to results.
        auto promise = std::move(initialPromise);
        promise.start();
        ThreadWrapper threads[] = {
            ThreadWrapper([&promise] { promise.addResult(42); }),
            ThreadWrapper([&promise] { promise.addResult(43); }),
            ThreadWrapper([&promise] { promise.addResult(44); }),
        };
        for (auto& t : threads)
            t.join();
        promise.finish();
    }

    QCOMPARE(f.isFinished(), true);
    QCOMPARE(f.isValid(), true);

    QList<int> expected = {42, 43, 44};
    for (auto actual : f.results()) {
        QVERIFY(std::find(expected.begin(), expected.end(), actual) != expected.end());
        expected.removeOne(actual);
    }
}

void tst_QPromise::doNotCancelWhenFinished()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires C++11 std threading enabled (and working) in the standard library.");
#endif
    const auto testFinishedPromise = [] (auto promise) {
        auto f = promise.future();
        promise.start();

        // Finish QPromise inside thread, destructor must not call cancel()
        ThreadWrapper([&] { auto p = std::move(promise); p.finish(); }).join();

        f.waitForFinished();

        QCOMPARE(f.isFinished(), true);
        QCOMPARE(f.isCanceled(), false);
    };

    RUN_TEST_FUNC(testFinishedPromise, QPromise<void>());
    RUN_TEST_FUNC(testFinishedPromise, QPromise<int>());
    RUN_TEST_FUNC(testFinishedPromise, QPromise<QString>());
    RUN_TEST_FUNC(testFinishedPromise, QPromise<CopyOnlyType>());
    RUN_TEST_FUNC(testFinishedPromise, QPromise<MoveOnlyType>());
}

#ifndef QT_NO_EXCEPTIONS
void tst_QPromise::cancelWhenDestroyed()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires C++11 std threading enabled (and working) in the standard library.");
#endif
    QPromise<int> initialPromise;
    auto f = initialPromise.future();

    try {
        // Move QPromise to local scope. On destruction, it must call cancel().
        auto promise = std::move(initialPromise);
        promise.start();
        ThreadWrapper threads[] = {
            ThreadWrapper([&promise] { promise.addResult(42); }),
            ThreadWrapper([&promise] { promise.addResult(43); }),
            ThreadWrapper([&promise] { promise.addResult(44); }),
        };
        for (auto& t : threads)
            t.join();
        throw "Throw in the middle, we lose our promise here, finish() not called!";
        promise.finish();
    } catch (...) {}

    QCOMPARE(f.isFinished(), true);
    QCOMPARE(f.isCanceled(), true);

    // Results are still available despite throw
    QList<int> expected = {42, 43, 44};
    for (auto actual : f.results()) {
        QVERIFY(std::find(expected.begin(), expected.end(), actual) != expected.end());
        expected.removeOne(actual);
    }
}
#endif

void tst_QPromise::cancelWhenReassigned()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires C++11 std threading enabled (and working) in the standard library.");
#endif
    QPromise<int> promise;
    auto f = promise.future();
    promise.start();

    ThreadWrapper thr([&] {
        auto p = std::move(promise);
        QThread::sleep(100ms);
        p = QPromise<int>();  // assign new promise, old must be correctly destroyed
    });

    f.waitForFinished();  // wait for the old promise

    QCOMPARE(f.isFinished(), true);
    QCOMPARE(f.isCanceled(), true);
}

template <typename T>
static inline void testCancelWhenDestroyedWithoutStarting()
{
    QFuture<T> future;
    {
        QPromise<T> promise;
        future = promise.future();
    }
    future.waitForFinished();
    QVERIFY(!future.isStarted());
    QVERIFY(future.isCanceled());
    QVERIFY(future.isFinished());
}

void tst_QPromise::cancelWhenDestroyedWithoutStarting()
{
    RUN_TEST_FUNC(testCancelWhenDestroyedWithoutStarting<void>);
    RUN_TEST_FUNC(testCancelWhenDestroyedWithoutStarting<int>);
    RUN_TEST_FUNC(testCancelWhenDestroyedWithoutStarting<CopyOnlyType>);
    RUN_TEST_FUNC(testCancelWhenDestroyedWithoutStarting<MoveOnlyType>);
}

template <typename T>
static inline void testCancelWhenDestroyedRunsContinuations()
{
    QFuture<T> future;
    FutureWatcher r;
    {
        QPromise<T> promise;
        future = promise.future();
        r.setFuture(future);
    }
    QVERIFY(future.isFinished());
    QVERIFY(!r.thenCalled);
    QVERIFY(r.onCanceledCalled);
}

void tst_QPromise::cancelWhenDestroyedRunsContinuations()
{
    RUN_TEST_FUNC(testCancelWhenDestroyedRunsContinuations<void>);
    RUN_TEST_FUNC(testCancelWhenDestroyedRunsContinuations<int>);
    RUN_TEST_FUNC(testCancelWhenDestroyedRunsContinuations<CopyOnlyType>);
    RUN_TEST_FUNC(testCancelWhenDestroyedRunsContinuations<MoveOnlyType>);
}

template <typename T>
static inline void testCancelWhenDestroyedWithFailureHandler()
{
    QFuture<T> future;
    FutureWatcher r;
    {
        QPromise<T> promise;
        future = promise.future();
        r.setFuture(future);
    }
    QVERIFY(future.isFinished());
    QVERIFY(!r.onFailedCalled);
    QVERIFY(!r.thenCalled);
}

void tst_QPromise::cancelWhenDestroyedWithFailureHandler()
{
#ifndef QT_NO_EXCEPTIONS
    RUN_TEST_FUNC(testCancelWhenDestroyedWithFailureHandler<void>);
    RUN_TEST_FUNC(testCancelWhenDestroyedWithFailureHandler<int>);
    RUN_TEST_FUNC(testCancelWhenDestroyedWithFailureHandler<CopyOnlyType>);
    RUN_TEST_FUNC(testCancelWhenDestroyedWithFailureHandler<MoveOnlyType>);
#else
    QSKIP("Exceptions are disabled, skipping the test");
#endif
}

template <typename T>
static inline void testContinuationsRunWhenFinished()
{
    QPromise<T> promise;
    QFuture<T> future = promise.future();

    FutureWatcher r(future);

    promise.start();
    if constexpr (!std::is_void_v<T>) {
        promise.addResult(T{});
    }
    promise.finish();

    QVERIFY(r.thenCalled);
}

void tst_QPromise::continuationsRunWhenFinished()
{
    RUN_TEST_FUNC(testContinuationsRunWhenFinished<void>);
    RUN_TEST_FUNC(testContinuationsRunWhenFinished<int>);
    RUN_TEST_FUNC(testContinuationsRunWhenFinished<CopyOnlyType>);
    RUN_TEST_FUNC(testContinuationsRunWhenFinished<MoveOnlyType>);
}

void tst_QPromise::finishWhenSwapped()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires C++11 std threading enabled (and working) in the standard library.");
#endif
    QPromise<int> promise1;
    auto f1 = promise1.future();
    promise1.start();

    QPromise<int> promise2;
    auto f2 = promise2.future();
    promise2.start();

    ThreadWrapper thr([&promise1, &promise2] () mutable {
        QThread::sleep(100ms);
        promise1.addResult(0);
        promise2.addResult(1);
        swap(promise1, promise2);  // ADL must resolve this
        promise1.addResult(2);
        promise2.addResult(3);
        promise1.finish();  // this finish is for future #2
        promise2.finish();  // this finish is for future #1
    });

    f1.waitForFinished();
    f2.waitForFinished();

    // Future #1 and #2 are finished inside thread
    QCOMPARE(f1.isFinished(), true);
    QCOMPARE(f1.isCanceled(), false);

    QCOMPARE(f2.isFinished(), true);
    QCOMPARE(f2.isCanceled(), false);

    QCOMPARE(f1.resultAt(0), 0);
    QCOMPARE(f1.resultAt(1), 3);

    QCOMPARE(f2.resultAt(0), 1);
    QCOMPARE(f2.resultAt(1), 2);
}

template <typename T>
void testCancelWhenMoved()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires C++11 std threading enabled (and working) in the standard library.");
#endif
    QPromise<T> promise1;
    auto f1 = promise1.future();
    promise1.start();

    QPromise<T> promise2;
    auto f2 = promise2.future();
    promise2.start();

    // Move promises to local scope to test cancellation behavior
    ThreadWrapper thr([&] {
        auto p1 = std::move(promise1);
        auto p2 = std::move(promise2);
        QThread::sleep(100ms);
        p1 = std::move(p2);
        p1.finish();  // this finish is for future #2
    });

    f1.waitForFinished();
    f2.waitForFinished();

    // Future #1 is implicitly cancelled inside thread
    QCOMPARE(f1.isFinished(), true);
    QCOMPARE(f1.isCanceled(), true);

    // Future #2 is explicitly finished inside thread
    QCOMPARE(f2.isFinished(), true);
    QCOMPARE(f2.isCanceled(), false);
}

void tst_QPromise::cancelWhenMoved()
{
    RUN_TEST_FUNC(testCancelWhenMoved<void>);
    RUN_TEST_FUNC(testCancelWhenMoved<int>);
    RUN_TEST_FUNC(testCancelWhenMoved<CopyOnlyType>);
    RUN_TEST_FUNC(testCancelWhenMoved<MoveOnlyType>);
}

void tst_QPromise::waitUntilResumed()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires QThread::create");
#else
    QPromise<int> promise;
    promise.start();
    auto f = promise.future();
    f.suspend();

    ThreadWrapper thr([&] {
        auto p = std::move(promise);
        p.suspendIfRequested();
        p.addResult(42);  // result added after suspend
        p.finish();
    });

    while (!f.isSuspended()) {  // busy wait until worker thread suspends
        QCOMPARE(f.isFinished(), false);  // exit condition in case of failure
        QThread::sleep(50ms);  // allow another thread to actually carry on
    }

    f.resume();
    f.waitForFinished();

    QCOMPARE(f.resultCount(), 1);
    QCOMPARE(f.result(), 42);
#endif
}

void tst_QPromise::waitUntilCanceled()
{
#if !QT_CONFIG(cxx11_future)
    QSKIP("This test requires C++11 std threading enabled (and working) in the standard library.");
#endif
    QPromise<int> promise;
    promise.start();
    auto f = promise.future();
    f.suspend();

    ThreadWrapper thr([&] {
        auto p = std::move(promise);
        p.suspendIfRequested();
        p.addResult(42);  // result not added due to QFuture::cancel()
        p.finish();
    });

    while (!f.isSuspended()) {  // busy wait until worker thread suspends
        QCOMPARE(f.isFinished(), false);  // exit condition in case of failure
        QThread::sleep(50ms);  // allow another thread to actually carry on
    }

    f.cancel();
    f.waitForFinished();

    QCOMPARE(f.resultCount(), 0);
}

// Below is a quick and dirty hack to make snippets a part of a test suite
#include "snippet_qpromise.cpp"
void tst_QPromise::snippet_basicExample()
{
    snippet_QPromise::basicExample();
}

void tst_QPromise::snippet_multithreadExample()
{
    snippet_QPromise::multithreadExample();
}

void tst_QPromise::snippet_suspendExample()
{
    snippet_QPromise::suspendExample();
}

QTEST_MAIN(tst_QPromise)
#include "tst_qpromise.moc"
