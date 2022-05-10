// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtconcurrenttask.h>

#include <QTest>
#include <QSemaphore>

#include <random>

class tst_QtConcurrentTask : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void taskWithFreeFunction();
    void taskWithClassMethod();
    void taskWithCallableObject();
    void taskWithLambda();
    void taskWithArguments();
    void useCustomThreadPool();
    void setPriority_data();
    void setPriority();
    void adjustAllSettings();
    void ignoreFutureResult();
    void withPromise();
};

using namespace QtConcurrent;

void tst_QtConcurrentTask::taskWithFreeFunction()
{
    QVariant value(42);

    auto result = task(&qvariant_cast<int>)
                      .withArguments(value)
                      .spawn()
                      .result();

    QCOMPARE(result, 42);
}

void tst_QtConcurrentTask::taskWithClassMethod()
{
    QString result("foobar");

    task(&QString::chop).withArguments(&result, 3).spawn().waitForFinished();

    QCOMPARE(result, "foo");
}

void tst_QtConcurrentTask::taskWithCallableObject()
{
    QCOMPARE(task(std::plus<int>())
                 .withArguments(40, 2)
                 .spawn()
                 .result(),
             42);
}

void tst_QtConcurrentTask::taskWithLambda()
{
    QCOMPARE(task([]{ return 42; }).spawn().result(), 42);
}

void tst_QtConcurrentTask::taskWithArguments()
{
    auto result = task([](int arg1, int arg2){ return arg1 + arg2; })
                      .withArguments(40, 2)
                      .spawn()
                      .result();
    QCOMPARE(result, 42);
}

void tst_QtConcurrentTask::useCustomThreadPool()
{
    QThreadPool pool;

    int result = 0;
    task([&]{ result = 42; }).onThreadPool(pool).spawn().waitForFinished();

    QCOMPARE(result, 42);
}

void tst_QtConcurrentTask::setPriority_data()
{
    QTest::addColumn<bool>("runWithPromise");

    QTest::addRow("without promise") << false;
    QTest::addRow("with promise") << true;
}

void tst_QtConcurrentTask::setPriority()
{
    QFETCH(bool, runWithPromise);

    QThreadPool pool;
    pool.setMaxThreadCount(1);

    QSemaphore sem;

    QList<QFuture<void>> futureResults;
    futureResults << task([&]{ sem.acquire(); })
                         .onThreadPool(pool)
                         .spawn();

    const int tasksCount = 10;
    QList<int> priorities(tasksCount);
    std::iota(priorities.begin(), priorities.end(), 1);
    auto seed = std::random_device {}();
    std::shuffle(priorities.begin(), priorities.end(), std::default_random_engine(seed));

    qDebug() << "Generated priorities list" << priorities << "using seed" << seed;

    QList<int> actual;
    for (int priority : priorities) {
        if (runWithPromise) {
            futureResults << task([priority, &actual] (QPromise<void> &) { actual << priority; })
                                 .onThreadPool(pool)
                                 .withPriority(priority)
                                 .spawn();
        } else {
            futureResults << task([priority, &actual] { actual << priority; })
                                 .onThreadPool(pool)
                                 .withPriority(priority)
                                 .spawn();
        }
    }

    sem.release();
    pool.waitForDone();

    for (const auto &f : futureResults)
        QVERIFY(f.isFinished());

    QList<int> expected(priorities);
    std::sort(expected.begin(), expected.end(), std::greater<>());

    QCOMPARE(actual, expected);
}

void tst_QtConcurrentTask::adjustAllSettings()
{
    QThreadPool pool;
    pool.setMaxThreadCount(1);

    const int priority = 10;

    QList<int> result;
    auto append = [&](auto &&...args){ (result << ... << args); };

    task(std::move(append))
        .withArguments(1, 2, 3)
        .onThreadPool(pool)
        .withPriority(priority)
        .spawn()
        .waitForFinished();

    QCOMPARE(result, QList<int>({ 1, 2, 3 }));
}

void tst_QtConcurrentTask::ignoreFutureResult()
{
    QThreadPool pool;

    std::atomic_int value = 0;
    for (std::size_t i = 0; i < 10; ++i)
        task([&value]{ ++value; })
            .onThreadPool(pool)
            .spawn(FutureResult::Ignore);

    pool.waitForDone();

    QCOMPARE(value, 10);
}

void incrementWithPromise(QPromise<int> &promise, int i)
{
    promise.addResult(i + 1);
}

void return0WithPromise(QPromise<int> &promise)
{
    promise.addResult(0);
}

void tst_QtConcurrentTask::withPromise()
{
    QCOMPARE(task(&return0WithPromise).spawn().result(), 0);
    QCOMPARE(task(&return0WithPromise).withPriority(7).spawn().result(), 0);
    QCOMPARE(task(&incrementWithPromise).withArguments(1).spawn().result(), 2);
    QCOMPARE(task(&incrementWithPromise).withArguments(1).withPriority(7).spawn().result(), 2);
}

QTEST_MAIN(tst_QtConcurrentTask)
#include "tst_qtconcurrenttask.moc"
