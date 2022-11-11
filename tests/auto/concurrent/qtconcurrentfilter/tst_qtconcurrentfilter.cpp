// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtconcurrentfilter.h>
#include <QCoreApplication>
#include <QList>
#include <QTest>
#include <QSet>

#include "../testhelper_functions.h"

class tst_QtConcurrentFilter : public QObject
{
    Q_OBJECT

private slots:
    void filter();
    void filterThreadPool();
    void filterWithMoveOnlyCallable();
    void filtered();
    void filteredThreadPool();
    void filteredWithMoveOnlyCallable();
    void filteredReduced();
    void filteredReducedThreadPool();
    void filteredReducedWithMoveOnlyCallables();
    void filteredReducedDifferentType();
    void filteredReducedInitialValue();
    void filteredReducedInitialValueThreadPool();
    void filteredReducedInitialValueWithMoveOnlyCallables();
    void filteredReducedDifferentTypeInitialValue();
    void filteredReduceOptionConvertableToResultType();
    void resultAt();
    void incrementalResults();
    void noDetach();
    void stlContainers();
    void stlContainersLambda();
};

using namespace QtConcurrent;

#define CHECK_FAIL(message) \
do {\
    if (QTest::currentTestFailed())\
        QFAIL("failed one line above on " message);\
} while (false)

template <typename SourceObject,
          typename ResultObject,
          typename FilterObject>
void testFilter(const QList<SourceObject> &sourceObjectList,
                const QList<ResultObject> &expectedResult,
                FilterObject filterObject)
{
    QList<SourceObject> copy1 = sourceObjectList;
    QList<SourceObject> copy2 = sourceObjectList;

    QtConcurrent::filter(copy1, filterObject).waitForFinished();
    QCOMPARE(copy1, expectedResult);

    QtConcurrent::blockingFilter(copy2, filterObject);
    QCOMPARE(copy2, expectedResult);
}

void tst_QtConcurrentFilter::filter()
{
    const QList<int> intList {1, 2, 3, 4};
    const QList<int> intListEven {2, 4};
    const QList<Number> numberList {1, 2, 3, 4};
    const QList<Number> numberListEven {2, 4};

    auto lambdaIsEven = [](const int &x) {
        return (x & 1) == 0;
    };

    testFilter(intList, intListEven, KeepEvenIntegers());
    CHECK_FAIL("functor");
    testFilter(intList, intListEven, keepEvenIntegers);
    CHECK_FAIL("function");
    testFilter(numberList, numberListEven, &Number::isEven);
    CHECK_FAIL("member");
    testFilter(intList, intListEven, lambdaIsEven);
    CHECK_FAIL("lambda");

    // non-template sequences
    {

        NonTemplateSequence list({ 1, 2, 3, 4 });
        QtConcurrent::filter(list, keepEvenNumbers).waitForFinished();
        QCOMPARE(list, NonTemplateSequence({ 2, 4 }));
    }
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        QtConcurrent::blockingFilter(list, keepEvenNumbers);
        QCOMPARE(list, NonTemplateSequence({ 2, 4 }));
    }
}

static QSemaphore semaphore(1);
static QSet<QThread *> workingThreads;

void storeCurrentThread()
{
    semaphore.acquire();
    workingThreads.insert(QThread::currentThread());
    semaphore.release();
}

int threadCount()
{
    semaphore.acquire();
    const int count = workingThreads.size();
    semaphore.release();
    return count;
}

template <typename SourceObject,
          typename ResultObject,
          typename FilterObject>
void testFilterThreadPool(QThreadPool *pool,
                          const QList<SourceObject> &sourceObjectList,
                          const QList<ResultObject> &expectedResult,
                          FilterObject filterObject)
{
    QList<SourceObject> copy1 = sourceObjectList;
    QList<SourceObject> copy2 = sourceObjectList;

    QtConcurrent::filter(pool, copy1, filterObject).waitForFinished();
    QCOMPARE(copy1, expectedResult);
    QCOMPARE(threadCount(), 1); // ensure the only one thread was working

    QtConcurrent::blockingFilter(pool, copy2, filterObject);
    QCOMPARE(copy2, expectedResult);
    QCOMPARE(threadCount(), 1); // ensure the only one thread was working
}

class KeepOddIntegers
{
public:
    bool operator()(const int &x)
    {
        storeCurrentThread();
        return x & 1;
    }
};

bool keepOddIntegers(const int &x)
{
    storeCurrentThread();
    return x & 1;
}

void tst_QtConcurrentFilter::filterThreadPool()
{
    const QList<int> intList {1, 2, 3, 4};
    const QList<int> intListEven {1, 3};

    auto lambdaIsOdd = [](const int &x) {
        storeCurrentThread();
        return x & 1;
    };

    QThreadPool pool;
    pool.setMaxThreadCount(1);
    QCOMPARE(semaphore.available(), 1);
    workingThreads.clear();

    testFilterThreadPool(&pool, intList, intListEven, KeepOddIntegers());
    CHECK_FAIL("functor");
    testFilterThreadPool(&pool, intList, intListEven, keepOddIntegers);
    CHECK_FAIL("function");
    testFilterThreadPool(&pool, intList, intListEven, lambdaIsOdd);
    CHECK_FAIL("lambda");

    // non-template sequences
    {

        NonTemplateSequence list({ 1, 2, 3, 4 });
        QtConcurrent::filter(list, keepEvenIntegers).waitForFinished();
        QCOMPARE(list, NonTemplateSequence({ 2, 4 }));
    }
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        QtConcurrent::blockingFilter(list, keepEvenIntegers);
        QCOMPARE(list, NonTemplateSequence({ 2, 4 }));
    }
}

void tst_QtConcurrentFilter::filterWithMoveOnlyCallable()
{
    const QList<int> intListEven { 2, 4 };
    {
        QList<int> intList { 1, 2, 3, 4 };
        QtConcurrent::filter(intList, KeepEvenIntegersMoveOnly()).waitForFinished();
        QCOMPARE(intList, intListEven);
    }

    {
        QList<int> intList { 1, 2, 3, 4 };
        QtConcurrent::blockingFilter(intList, KeepEvenIntegersMoveOnly());
        QCOMPARE(intList, intListEven);
    }

    QThreadPool pool;
    {
        QList<int> intList { 1, 2, 3, 4 };
        QtConcurrent::filter(&pool, intList, KeepEvenIntegersMoveOnly()).waitForFinished();
        QCOMPARE(intList, intListEven);
    }
    {
        QList<int> intList { 1, 2, 3, 4 };
        QtConcurrent::blockingFilter(&pool, intList, KeepEvenIntegersMoveOnly());
        QCOMPARE(intList, intListEven);
    }
}

template <typename SourceObject,
          typename ResultObject,
          typename FilterObject>
void testFiltered(const QList<SourceObject> &sourceObjectList,
                  const QList<ResultObject> &expectedResult,
                  FilterObject filterObject)
{
    const QList<ResultObject> result1 = QtConcurrent::filtered(
                sourceObjectList, filterObject).results();
    QCOMPARE(result1, expectedResult);

    const QList<ResultObject> result2 = QtConcurrent::filtered(
                sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                filterObject).results();
    QCOMPARE(result2, expectedResult);

    const QList<ResultObject> result3 = QtConcurrent::blockingFiltered(
                sourceObjectList, filterObject);
    QCOMPARE(result3, expectedResult);

    const QList<ResultObject> result4 = QtConcurrent::blockingFiltered<QList<ResultObject>>(
                sourceObjectList.constBegin(), sourceObjectList.constEnd(), filterObject);
    QCOMPARE(result4, expectedResult);
}

void tst_QtConcurrentFilter::filtered()
{
    const QList<int> intList {1, 2, 3, 4};
    const QList<int> intListEven {2, 4};
    const QList<Number> numberList {1, 2, 3, 4};
    const QList<Number> numberListEven {2, 4};

    auto lambdaIsEven = [](const int &x) {
        return (x & 1) == 0;
    };

    testFiltered(intList, intListEven, KeepEvenIntegers());
    CHECK_FAIL("functor");
    testFiltered(intList, intListEven, keepEvenIntegers);
    CHECK_FAIL("function");
    testFiltered(numberList, numberListEven, &Number::isEven);
    CHECK_FAIL("member");
    testFiltered(intList, intListEven, lambdaIsEven);
    CHECK_FAIL("lambda");

    // non-template sequences
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto future = QtConcurrent::filtered(list, keepEvenIntegers);
        QCOMPARE(future.results(), QList({ 2, 4 }));
    }
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto result = QtConcurrent::blockingFiltered(list, keepEvenIntegers);
        QCOMPARE(result, NonTemplateSequence({ 2, 4 }));
    }

    {
        // rvalue sequences
        auto future = QtConcurrent::filtered(std::vector { 1, 2, 3, 4 }, keepEvenIntegers);
        QCOMPARE(future.results(), QList<int>({ 2, 4 }));

        auto result = QtConcurrent::blockingFiltered(std::vector { 1, 2, 3, 4 }, keepEvenIntegers);
        QCOMPARE(result, std::vector<int>({ 2, 4 }));
    }

    {
        // move only types sequences
        auto future = QtConcurrent::filtered(MoveOnlyVector<int>({ 1, 2, 3, 4 }), keepEvenIntegers);
        QCOMPARE(future.results(), QList<int>({ 2, 4 }));

#if 0
        // does not work yet
        auto result = QtConcurrent::blockingFiltered(
                MoveOnlyVector<int>({ 1, 2, 3, 4 }), keepEvenIntegers);
        QCOMPARE(result, std::vector<int>({ 2, 4 }));
#endif
    }
}

template <typename SourceObject,
          typename ResultObject,
          typename FilterObject>
void testFilteredThreadPool(QThreadPool *pool,
                            const QList<SourceObject> &sourceObjectList,
                            const QList<ResultObject> &expectedResult,
                            FilterObject filterObject)
{
    const QList<ResultObject> result1 = QtConcurrent::filtered(
                pool, sourceObjectList, filterObject).results();
    QCOMPARE(result1, expectedResult);
    QCOMPARE(threadCount(), 1); // ensure the only one thread was working

    const QList<ResultObject> result2 = QtConcurrent::filtered(
                pool, sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                filterObject).results();
    QCOMPARE(result2, expectedResult);
    QCOMPARE(threadCount(), 1); // ensure the only one thread was working

    const QList<ResultObject> result3 = QtConcurrent::blockingFiltered(
                pool, sourceObjectList, filterObject);
    QCOMPARE(result3, expectedResult);
    QCOMPARE(threadCount(), 1); // ensure the only one thread was working

    const QList<ResultObject> result4 = QtConcurrent::blockingFiltered<QList<ResultObject>>(
                pool, sourceObjectList.constBegin(), sourceObjectList.constEnd(), filterObject);
    QCOMPARE(result4, expectedResult);
    QCOMPARE(threadCount(), 1); // ensure the only one thread was working
}

void tst_QtConcurrentFilter::filteredThreadPool()
{
    const QList<int> intList {1, 2, 3, 4};
    const QList<int> intListEven {1, 3};

    auto lambdaIsOdd = [](const int &x) {
        storeCurrentThread();
        return x & 1;
    };

    QThreadPool pool;
    pool.setMaxThreadCount(1);
    QCOMPARE(semaphore.available(), 1);
    workingThreads.clear();

    testFilteredThreadPool(&pool, intList, intListEven, KeepOddIntegers());
    CHECK_FAIL("functor");
    testFilteredThreadPool(&pool, intList, intListEven, keepOddIntegers);
    CHECK_FAIL("function");
    testFilteredThreadPool(&pool, intList, intListEven, lambdaIsOdd);
    CHECK_FAIL("lambda");

    // non-template sequences
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto future = QtConcurrent::filtered(&pool, list, keepEvenIntegers);
        QCOMPARE(future.results(), QList({ 2, 4 }));
    }
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto result = QtConcurrent::blockingFiltered(&pool, list, keepEvenIntegers);
        QCOMPARE(result, NonTemplateSequence({ 2, 4 }));
    }

    {
        // rvalue sequences
        auto future = QtConcurrent::filtered(&pool, std::vector { 1, 2, 3, 4 }, keepEvenIntegers);
        QCOMPARE(future.results(), QList<int>({ 2, 4 }));

        auto result =
                QtConcurrent::blockingFiltered(&pool, std::vector { 1, 2, 3, 4 }, keepEvenIntegers);
        QCOMPARE(result, std::vector<int>({ 2, 4 }));
    }

    {
        // move-only sequences
        auto future = QtConcurrent::filtered(&pool, MoveOnlyVector<int>({ 1, 2, 3, 4 }),
                                             keepEvenIntegers);
        QCOMPARE(future.results(), QList<int>({ 2, 4 }));

#if 0
        // does not work yet
        auto result =
                QtConcurrent::blockingFiltered(
                        &pool, MoveOnlyVector<int>({ 1, 2, 3, 4 }), keepEvenIntegers);
        QCOMPARE(result, std::vector<int>({ 2, 4 }));
#endif
    }
}

void tst_QtConcurrentFilter::filteredWithMoveOnlyCallable()
{
    const QList<int> intList { 1, 2, 3, 4 };
    const QList<int> intListEven { 2, 4 };
    {
        const auto result = QtConcurrent::filtered(intList, KeepEvenIntegersMoveOnly()).results();
        QCOMPARE(result, intListEven);
    }
    {
        const auto result = QtConcurrent::filtered(
                    intList.begin(), intList.end(), KeepEvenIntegersMoveOnly()).results();
        QCOMPARE(result, intListEven);
    }
    {
        const auto result = QtConcurrent::blockingFiltered(intList, KeepEvenIntegersMoveOnly());
        QCOMPARE(result, intListEven);
    }
    {
        const auto result = QtConcurrent::blockingFiltered<QList<int>>(
                intList.begin(), intList.end(), KeepEvenIntegersMoveOnly());
        QCOMPARE(result, intListEven);
    }

    QThreadPool pool;
    {
        const auto result =
                QtConcurrent::filtered(&pool, intList, KeepEvenIntegersMoveOnly()).results();
        QCOMPARE(result, intListEven);
    }
    {
        const auto result = QtConcurrent::filtered(&pool, intList.begin(), intList.end(),
                                                   KeepEvenIntegersMoveOnly()).results();
        QCOMPARE(result, intListEven);
    }
    {
        const auto result =
                QtConcurrent::blockingFiltered(&pool, intList, KeepEvenIntegersMoveOnly());
        QCOMPARE(result, intListEven);
    }
    {
        const auto result = QtConcurrent::blockingFiltered<QList<int>>(
                &pool, intList.begin(), intList.end(), KeepEvenIntegersMoveOnly());
        QCOMPARE(result, intListEven);
    }
}

template <typename SourceObject,
          typename ResultObject,
          typename FilterObject,
          typename ReduceObject>
void testFilteredReduced(const QList<SourceObject> &sourceObjectList,
                         const ResultObject &expectedResult,
                         FilterObject filterObject,
                         ReduceObject reduceObject)
{
    // Result type is passed explicitly
    {
        const ResultObject result1 = QtConcurrent::filteredReduced<ResultObject>(
                    sourceObjectList, filterObject, reduceObject).result();
        QCOMPARE(result1, expectedResult);

        const ResultObject result2 = QtConcurrent::filteredReduced<ResultObject>(
                    sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject).result();
        QCOMPARE(result2, expectedResult);

        const ResultObject result3 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                    sourceObjectList, filterObject, reduceObject);
        QCOMPARE(result3, expectedResult);

        const ResultObject result4 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                    sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject);
        QCOMPARE(result4, expectedResult);
    }

    // Result type is deduced
    {
        const ResultObject result1 = QtConcurrent::filteredReduced(
                    sourceObjectList, filterObject, reduceObject).result();
        QCOMPARE(result1, expectedResult);

        const ResultObject result2 = QtConcurrent::filteredReduced(
                    sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject).result();
        QCOMPARE(result2, expectedResult);

        const ResultObject result3 = QtConcurrent::blockingFilteredReduced(
                    sourceObjectList, filterObject, reduceObject);
        QCOMPARE(result3, expectedResult);

        const ResultObject result4 = QtConcurrent::blockingFilteredReduced(
                    sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject);
        QCOMPARE(result4, expectedResult);
    }
}

template <typename SourceObject,
          typename ResultObject,
          typename FilterObject,
          typename ReduceObject>
void testFilteredReduced(const QList<SourceObject> &sourceObjectList,
                         const ResultObject &expectedResult,
                         FilterObject filterObject,
                         ReduceObject reduceObject,
                         QtConcurrent::ReduceOptions options)
{
    const ResultObject result1 = QtConcurrent::filteredReduced(
                sourceObjectList, filterObject, reduceObject, options).result();
    QCOMPARE(result1, expectedResult);

    const ResultObject result2 =
            QtConcurrent::filteredReduced(sourceObjectList.constBegin(),
                                          sourceObjectList.constEnd(),
                                          filterObject, reduceObject, options).result();
    QCOMPARE(result2, expectedResult);

    const ResultObject result3 = QtConcurrent::blockingFilteredReduced(
                sourceObjectList, filterObject, reduceObject, options);
    QCOMPARE(result3, expectedResult);

    const ResultObject result4 = QtConcurrent::blockingFilteredReduced(
                sourceObjectList.constBegin(), sourceObjectList.constEnd(), filterObject,
                reduceObject, options);
    QCOMPARE(result4, expectedResult);
}

void numberSumReduceToNumber(Number &sum, const Number &x)
{
    sum = Number(sum.toInt() + x.toInt());
}

class NumberSumReduceToNumber
{
public:
    void operator()(Number &sum, const Number &x)
    {
        sum = Number(sum.toInt() + x.toInt());
    }
};

void tst_QtConcurrentFilter::filteredReduced()
{
    const QList<int> intList {1, 2, 3, 4};
    const QList<int> intListEven {2, 4};
    const QList<Number> numberList {1, 2, 3, 4};
    const QList<Number> numberListEven {2, 4};
    const int intSum = 6; // sum of even values
    const Number numberSum = 6; // sum of even values

    void (QList<int>::*pushBackInt)(QList<int>::parameter_type) = &QList<int>::push_back;
    void (QList<Number>::*pushBackNumber)(QList<Number>::parameter_type) =
            &QList<Number>::push_back;

    auto lambdaIsEven = [](const int &x) {
        return (x & 1) == 0;
    };
    auto lambdaIntSumReduce = [](int &sum, const int &x) {
        sum += x;
    };
    auto lambdaNumberSumReduce = [](Number &sum, const Number &x) {
        sum = Number(sum.toInt() + x.toInt());
    };

    // FUNCTOR-other
    testFilteredReduced(intList, intSum, KeepEvenIntegers(), IntSumReduce());
    CHECK_FAIL("functor-functor");
    testFilteredReduced(intList, intSum, KeepEvenIntegers(), intSumReduce);
    CHECK_FAIL("functor-function");
    testFilteredReduced(intList, intListEven, KeepEvenIntegers(), pushBackInt, OrderedReduce);
    CHECK_FAIL("functor-member");
    testFilteredReduced(intList, intSum, KeepEvenIntegers(), lambdaIntSumReduce);
    CHECK_FAIL("functor-lambda");

    // FUNCTION-other
    testFilteredReduced(intList, intSum, keepEvenIntegers, IntSumReduce());
    CHECK_FAIL("function-functor");
    testFilteredReduced(intList, intSum, keepEvenIntegers, intSumReduce);
    CHECK_FAIL("function-function");
    testFilteredReduced(intList, intListEven, keepEvenIntegers, pushBackInt, OrderedReduce);
    CHECK_FAIL("function-member");
    testFilteredReduced(intList, intSum, keepEvenIntegers, lambdaIntSumReduce);
    CHECK_FAIL("function-lambda");

    // MEMBER-other
    testFilteredReduced(numberList, numberSum, &Number::isEven, NumberSumReduceToNumber());
    CHECK_FAIL("member-functor");
    testFilteredReduced(numberList, numberSum, &Number::isEven, numberSumReduceToNumber);
    CHECK_FAIL("member-function");
    testFilteredReduced(numberList, numberListEven, &Number::isEven,
                        pushBackNumber, OrderedReduce);
    CHECK_FAIL("member-member");
    testFilteredReduced(numberList, numberSum, &Number::isEven, lambdaNumberSumReduce);
    CHECK_FAIL("member-lambda");

    // LAMBDA-other
    testFilteredReduced(intList, intSum, lambdaIsEven, IntSumReduce());
    CHECK_FAIL("lambda-functor");
    testFilteredReduced(intList, intSum, lambdaIsEven, intSumReduce);
    CHECK_FAIL("lambda-function");
    testFilteredReduced(intList, intListEven, lambdaIsEven, pushBackInt, OrderedReduce);
    CHECK_FAIL("lambda-member");
    testFilteredReduced(intList, intSum, lambdaIsEven, lambdaIntSumReduce);
    CHECK_FAIL("lambda-lambda");

    // non-template sequences
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto future = QtConcurrent::filteredReduced(list, keepEvenIntegers, intSumReduce);
        QCOMPARE(future.result(), intSum);
    }
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto result = QtConcurrent::blockingFilteredReduced(list, keepEvenIntegers, intSumReduce);
        QCOMPARE(result, intSum);
    }

    {
        // rvalue sequences
        auto future = QtConcurrent::filteredReduced(std::vector { 1, 2, 3, 4 }, keepEvenIntegers,
                                                    intSumReduce);
        QCOMPARE(future.result(), intSum);

        auto result = QtConcurrent::blockingFilteredReduced(std::vector { 1, 2, 3, 4 },
                                                            keepEvenIntegers, intSumReduce);
        QCOMPARE(result, intSum);
    }

    {
        // move only sequences
        auto future = QtConcurrent::filteredReduced(MoveOnlyVector<int>({ 1, 2, 3, 4 }),
                                                    keepEvenIntegers, intSumReduce);
        QCOMPARE(future.result(), intSum);

        auto result = QtConcurrent::blockingFilteredReduced(MoveOnlyVector<int>({ 1, 2, 3, 4 }),
                                                            keepEvenIntegers, intSumReduce);
        QCOMPARE(result, intSum);
    }
}

template <typename SourceObject,
          typename ResultObject,
          typename FilterObject,
          typename ReduceObject>
void testFilteredReducedThreadPool(QThreadPool *pool,
                                   const QList<SourceObject> &sourceObjectList,
                                   const ResultObject &expectedResult,
                                   FilterObject filterObject,
                                   ReduceObject reduceObject)
{
    // Result type is passed explicitly
    {
        const ResultObject result1 = QtConcurrent::filteredReduced<ResultObject>(
                    pool, sourceObjectList, filterObject, reduceObject).result();
        QCOMPARE(result1, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result2 =
                QtConcurrent::filteredReduced<ResultObject>(pool, sourceObjectList.constBegin(),
                                                            sourceObjectList.constEnd(), filterObject,
                                                            reduceObject).result();
        QCOMPARE(result2, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result3 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                    pool, sourceObjectList, filterObject, reduceObject);
        QCOMPARE(result3, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result4 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                    pool, sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject);
        QCOMPARE(result4, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working
    }

    // Result type is deduced
    {
        const ResultObject result1 = QtConcurrent::filteredReduced(
                    pool, sourceObjectList, filterObject, reduceObject).result();
        QCOMPARE(result1, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result2 =
                QtConcurrent::filteredReduced(pool, sourceObjectList.constBegin(),
                                              sourceObjectList.constEnd(), filterObject,
                                              reduceObject).result();
        QCOMPARE(result2, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result3 = QtConcurrent::blockingFilteredReduced(
                    pool, sourceObjectList, filterObject, reduceObject);
        QCOMPARE(result3, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result4 = QtConcurrent::blockingFilteredReduced(
                    pool, sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject);
        QCOMPARE(result4, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working
    }
}

void tst_QtConcurrentFilter::filteredReducedThreadPool()
{
    const QList<int> intList {1, 2, 3, 4};
    const int intSum = 4; // sum of even values

    auto lambdaIsOdd = [](const int &x) {
        storeCurrentThread();
        return x & 1;
    };
    auto lambdaSumReduce = [](int &sum, const int &x) {
        sum += x;
    };

    QThreadPool pool;
    pool.setMaxThreadCount(1);
    QCOMPARE(semaphore.available(), 1);
    workingThreads.clear();

    // FUNCTOR-other
    testFilteredReducedThreadPool(&pool, intList, intSum, KeepOddIntegers(), IntSumReduce());
    CHECK_FAIL("functor-functor");
    testFilteredReducedThreadPool(&pool, intList, intSum, KeepOddIntegers(), intSumReduce);
    CHECK_FAIL("functor-function");
    testFilteredReducedThreadPool(&pool, intList, intSum, KeepOddIntegers(), lambdaSumReduce);
    CHECK_FAIL("functor-lambda");

    // FUNCTION-other
    testFilteredReducedThreadPool(&pool, intList, intSum, keepOddIntegers, IntSumReduce());
    CHECK_FAIL("function-functor");
    testFilteredReducedThreadPool(&pool, intList, intSum, keepOddIntegers, intSumReduce);
    CHECK_FAIL("function-function");
    testFilteredReducedThreadPool(&pool, intList, intSum, keepOddIntegers, lambdaSumReduce);
    CHECK_FAIL("function-lambda");

    // LAMBDA-other
    testFilteredReducedThreadPool(&pool, intList, intSum, lambdaIsOdd, IntSumReduce());
    CHECK_FAIL("lambda-functor");
    testFilteredReducedThreadPool(&pool, intList, intSum, lambdaIsOdd, intSumReduce);
    CHECK_FAIL("lambda-function");
    testFilteredReducedThreadPool(&pool, intList, intSum, lambdaIsOdd, lambdaSumReduce);
    CHECK_FAIL("lambda-lambda");

    // non-template sequences
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto future = QtConcurrent::filteredReduced(&pool, list, keepOddIntegers, intSumReduce);
        QCOMPARE(future.result(), intSum);
    }
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto result =
                QtConcurrent::blockingFilteredReduced(&pool, list, keepOddIntegers, intSumReduce);
        QCOMPARE(result, intSum);
    }

    {
        // rvalue sequences
        auto future = QtConcurrent::filteredReduced(&pool, std::vector { 1, 2, 3, 4 },
                                                    keepOddIntegers, intSumReduce);
        QCOMPARE(future.result(), intSum);

        auto result = QtConcurrent::blockingFilteredReduced(&pool, std::vector { 1, 2, 3, 4 },
                                                            keepOddIntegers, intSumReduce);
        QCOMPARE(result, intSum);
    }

    {
        // move only sequences
        auto future = QtConcurrent::filteredReduced(&pool, MoveOnlyVector<int>({ 1, 2, 3, 4 }),
                                                    keepOddIntegers, intSumReduce);
        QCOMPARE(future.result(), intSum);

        auto result = QtConcurrent::blockingFilteredReduced(
                &pool, MoveOnlyVector<int>({ 1, 2, 3, 4 }), keepOddIntegers, intSumReduce);
        QCOMPARE(result, intSum);
    }
}

void tst_QtConcurrentFilter::filteredReducedWithMoveOnlyCallables()
{
    const QList<int> intList { 1, 2, 3, 4 };
    const QList<int> intListEven { 2, 4 };
    const auto sum = 6;
    {
        const auto result =
                QtConcurrent::filteredReduced(intList, KeepEvenIntegersMoveOnly(),
                                              IntSumReduceMoveOnly()).result();
        QCOMPARE(result, sum);
    }
    {
        const auto result =
                QtConcurrent::filteredReduced(intList.begin(), intList.end(),
                                              KeepEvenIntegersMoveOnly(),
                                              IntSumReduceMoveOnly()).result();
        QCOMPARE(result, sum);
    }
    {
        const auto result = QtConcurrent::blockingFilteredReduced(
                    intList, KeepEvenIntegersMoveOnly(), IntSumReduceMoveOnly());
        QCOMPARE(result, sum);
    }
    {
        const auto result = QtConcurrent::blockingFilteredReduced(
                    intList.begin(), intList.end(), KeepEvenIntegersMoveOnly(), IntSumReduceMoveOnly());
        QCOMPARE(result, sum);
    }

    QThreadPool pool;
    {
        const auto result =
                QtConcurrent::filteredReduced(&pool, intList, KeepEvenIntegersMoveOnly(),
                                              IntSumReduceMoveOnly()).result();
        QCOMPARE(result, sum);
    }
    {
        const auto result = QtConcurrent::filteredReduced(
                    &pool, intList.begin(), intList.end(),
                    KeepEvenIntegersMoveOnly(), IntSumReduceMoveOnly()).result();
        QCOMPARE(result, sum);
    }
    {
        const auto result = QtConcurrent::blockingFilteredReduced(
                    &pool, intList, KeepEvenIntegersMoveOnly(), IntSumReduceMoveOnly());
        QCOMPARE(result, sum);
    }
    {
        const auto result = QtConcurrent::blockingFilteredReduced(
                    &pool, intList.begin(), intList.end(), KeepEvenIntegersMoveOnly(),
                    IntSumReduceMoveOnly());
        QCOMPARE(result, sum);
    }
}

void tst_QtConcurrentFilter::filteredReducedDifferentType()
{
    const QList<Number> numberList {1, 2, 3, 4};
    const int sum = 6; // sum of even values

    auto lambdaIsEven = [](const Number &x) {
        return (x.toInt() & 1) == 0;
    };
    auto lambdaSumReduce = [](int &sum, const Number &x) {
        sum += x.toInt();
    };

    // Test the case where reduce function of the form:
    // V function(T &result, const U &intermediate)
    // has T and U types different.

    // FUNCTOR-other
    testFilteredReduced(numberList, sum, KeepEvenNumbers(), NumberSumReduce());
    CHECK_FAIL("functor-functor");
    testFilteredReduced(numberList, sum, KeepEvenNumbers(), numberSumReduce);
    CHECK_FAIL("functor-function");
    testFilteredReduced(numberList, sum, KeepEvenNumbers(), lambdaSumReduce);
    CHECK_FAIL("functor-lambda");

    // FUNCTION-other
    testFilteredReduced(numberList, sum, keepEvenNumbers, NumberSumReduce());
    CHECK_FAIL("function-functor");
    testFilteredReduced(numberList, sum, keepEvenNumbers, numberSumReduce);
    CHECK_FAIL("function-function");
    testFilteredReduced(numberList, sum, keepEvenNumbers, lambdaSumReduce);
    CHECK_FAIL("function-lambda");

    // MEMBER-other
    testFilteredReduced(numberList, sum, &Number::isEven, NumberSumReduce());
    CHECK_FAIL("member-functor");
    testFilteredReduced(numberList, sum, &Number::isEven, numberSumReduce);
    CHECK_FAIL("member-function");
    testFilteredReduced(numberList, sum, &Number::isEven, lambdaSumReduce);
    CHECK_FAIL("member-lambda");

    // LAMBDA-other
    testFilteredReduced(numberList, sum, lambdaIsEven, NumberSumReduce());
    CHECK_FAIL("lambda-functor");
    testFilteredReduced(numberList, sum, lambdaIsEven, numberSumReduce);
    CHECK_FAIL("lambda-function");
    testFilteredReduced(numberList, sum, lambdaIsEven, lambdaSumReduce);
    CHECK_FAIL("lambda-lambda");
}

template <typename SourceObject,
          typename ResultObject,
          typename InitialObject,
          typename FilterObject,
          typename ReduceObject>
void testFilteredReducedInitialValue(const QList<SourceObject> &sourceObjectList,
                                     const ResultObject &expectedResult,
                                     FilterObject filterObject,
                                     ReduceObject reduceObject,
                                     InitialObject &&initialObject)
{
    // Result type is passed explicitly
    {
        const ResultObject result1 = QtConcurrent::filteredReduced<ResultObject>(
                    sourceObjectList, filterObject, reduceObject, initialObject).result();
        QCOMPARE(result1, expectedResult);

        const ResultObject result2 = QtConcurrent::filteredReduced<ResultObject>(
                    sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject, initialObject).result();
        QCOMPARE(result2, expectedResult);

        const ResultObject result3 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                    sourceObjectList, filterObject, reduceObject, initialObject);
        QCOMPARE(result3, expectedResult);

        const ResultObject result4 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                    sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject, initialObject);
        QCOMPARE(result4, expectedResult);
    }

    // Result type is deduced
    {
        const ResultObject result1 = QtConcurrent::filteredReduced(
                    sourceObjectList, filterObject, reduceObject, initialObject).result();
        QCOMPARE(result1, expectedResult);

        const ResultObject result2 = QtConcurrent::filteredReduced(
                    sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject, initialObject).result();
        QCOMPARE(result2, expectedResult);

        const ResultObject result3 = QtConcurrent::blockingFilteredReduced(
                    sourceObjectList, filterObject, reduceObject, initialObject);
        QCOMPARE(result3, expectedResult);

        const ResultObject result4 = QtConcurrent::blockingFilteredReduced(
                    sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject, initialObject);
        QCOMPARE(result4, expectedResult);
    }
}

template <typename SourceObject,
          typename ResultObject,
          typename InitialObject,
          typename FilterObject,
          typename ReduceObject>
void testFilteredReducedInitialValue(const QList<SourceObject> &sourceObjectList,
                                     const ResultObject &expectedResult,
                                     FilterObject filterObject,
                                     ReduceObject reduceObject,
                                     InitialObject &&initialObject,
                                     QtConcurrent::ReduceOptions options)
{
    const ResultObject result1 = QtConcurrent::filteredReduced(
                sourceObjectList, filterObject, reduceObject, initialObject, options).result();
    QCOMPARE(result1, expectedResult);

    const ResultObject result2 =
            QtConcurrent::filteredReduced(sourceObjectList.constBegin(),
                                          sourceObjectList.constEnd(), filterObject, reduceObject,
                                          initialObject, options).result();
    QCOMPARE(result2, expectedResult);

    const ResultObject result3 = QtConcurrent::blockingFilteredReduced(
                sourceObjectList, filterObject, reduceObject, initialObject, options);
    QCOMPARE(result3, expectedResult);

    const ResultObject result4 = QtConcurrent::blockingFilteredReduced(
                sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                filterObject, reduceObject, initialObject, options);
    QCOMPARE(result4, expectedResult);
}

void tst_QtConcurrentFilter::filteredReducedInitialValue()
{
    // This is a copy of tst_QtConcurrentFilter::filteredReduced
    // with the initial value parameter added

    const QList<int> intList {1, 2, 3, 4};
    const QList<int> intListInitial {10};
    const QList<int> intListAppended {10, 2, 4};
    const QList<Number> numberList {1, 2, 3, 4};
    const QList<Number> numberListInitial {10};
    const QList<Number> numberListAppended {10, 2, 4};
    const int intInitial = 10;
    const int intSum = 16; // sum of even values and initial value
    const Number numberSum = 16; // sum of even values and initial value

    void (QList<int>::*pushBackInt)(QList<int>::parameter_type) = &QList<int>::push_back;
    void (QList<Number>::*pushBackNumber)(QList<Number>::parameter_type) =
            &QList<Number>::push_back;

    auto lambdaIsEven = [](const int &x) {
        return (x & 1) == 0;
    };
    auto lambdaIntSumReduce = [](int &sum, const int &x) {
        sum += x;
    };
    auto lambdaNumberSumReduce = [](Number &sum, const Number &x) {
        sum = Number(sum.toInt() + x.toInt());
    };

    // FUNCTOR-other
    testFilteredReducedInitialValue(intList, intSum, KeepEvenIntegers(),
                                    IntSumReduce(), intInitial);
    CHECK_FAIL("functor-functor");
    testFilteredReducedInitialValue(intList, intSum, KeepEvenIntegers(),
                                    intSumReduce, intInitial);
    CHECK_FAIL("functor-function");
    testFilteredReducedInitialValue(intList, intListAppended, KeepEvenIntegers(),
                                    pushBackInt, intListInitial, OrderedReduce);
    CHECK_FAIL("functor-member");
    testFilteredReducedInitialValue(intList, intSum, KeepEvenIntegers(),
                                    lambdaIntSumReduce, intInitial);
    CHECK_FAIL("functor-lambda");

    // FUNCTION-other
    testFilteredReducedInitialValue(intList, intSum, keepEvenIntegers,
                                    IntSumReduce(), intInitial);
    CHECK_FAIL("function-functor");
    testFilteredReducedInitialValue(intList, intSum, keepEvenIntegers,
                                    intSumReduce, intInitial);
    CHECK_FAIL("function-function");
    testFilteredReducedInitialValue(intList, intListAppended, keepEvenIntegers,
                                    pushBackInt, intListInitial, OrderedReduce);
    CHECK_FAIL("function-member");
    testFilteredReducedInitialValue(intList, intSum, keepEvenIntegers,
                                    lambdaIntSumReduce, intInitial);
    CHECK_FAIL("function-lambda");

    // MEMBER-other
    testFilteredReducedInitialValue(numberList, numberSum, &Number::isEven,
                                    NumberSumReduceToNumber(), intInitial);
    CHECK_FAIL("member-functor");
    testFilteredReducedInitialValue(numberList, numberSum, &Number::isEven,
                                    numberSumReduceToNumber, intInitial);
    CHECK_FAIL("member-function");
    testFilteredReducedInitialValue(numberList, numberListAppended, &Number::isEven,
                                    pushBackNumber, numberListInitial, OrderedReduce);
    CHECK_FAIL("member-member");
    testFilteredReducedInitialValue(numberList, numberSum, &Number::isEven,
                                    lambdaNumberSumReduce, intInitial);
    CHECK_FAIL("member-lambda");

    // LAMBDA-other
    testFilteredReducedInitialValue(intList, intSum, lambdaIsEven,
                                    IntSumReduce(), intInitial);
    CHECK_FAIL("lambda-functor");
    testFilteredReducedInitialValue(intList, intSum, lambdaIsEven,
                                    intSumReduce, intInitial);
    CHECK_FAIL("lambda-function");
    testFilteredReducedInitialValue(intList, intListAppended, lambdaIsEven,
                                    pushBackInt, intListInitial, OrderedReduce);
    CHECK_FAIL("lambda-member");
    testFilteredReducedInitialValue(intList, intSum, lambdaIsEven,
                                    lambdaIntSumReduce, intInitial);
    CHECK_FAIL("lambda-lambda");

    // non-template sequences
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto future =
                QtConcurrent::filteredReduced(list, keepEvenIntegers, intSumReduce, intInitial);
        QCOMPARE(future.result(), intSum);
    }
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto result = QtConcurrent::blockingFilteredReduced(list, keepEvenIntegers, intSumReduce,
                                                            intInitial);
        QCOMPARE(result, intSum);
    }

    {
        // rvalue sequences
        auto future = QtConcurrent::filteredReduced(std::vector { 1, 2, 3, 4 }, keepEvenIntegers,
                                                    intSumReduce, intInitial);
        QCOMPARE(future.result(), intSum);

        auto result = QtConcurrent::blockingFilteredReduced(
                std::vector { 1, 2, 3, 4 }, keepEvenIntegers, intSumReduce, intInitial);
        QCOMPARE(result, intSum);
    }

    {
        // move only sequences
        auto future = QtConcurrent::filteredReduced(MoveOnlyVector<int>({ 1, 2, 3, 4 }),
                                                    keepEvenIntegers, intSumReduce, intInitial);
        QCOMPARE(future.result(), intSum);

        auto result = QtConcurrent::blockingFilteredReduced(
                MoveOnlyVector<int>({ 1, 2, 3, 4 }), keepEvenIntegers, intSumReduce, intInitial);
        QCOMPARE(result, intSum);
    }
}

template <typename SourceObject,
          typename ResultObject,
          typename InitialObject,
          typename FilterObject,
          typename ReduceObject>
void testFilteredReducedInitialValueThreadPool(QThreadPool *pool,
                                               const QList<SourceObject> &sourceObjectList,
                                               const ResultObject &expectedResult,
                                               FilterObject filterObject,
                                               ReduceObject reduceObject,
                                               InitialObject &&initialObject)
{
    // Result type is passed explicitly
    {
        const ResultObject result1 = QtConcurrent::filteredReduced<ResultObject>(
                    pool, sourceObjectList, filterObject, reduceObject, initialObject).result();
        QCOMPARE(result1, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result2 =
                QtConcurrent::filteredReduced<ResultObject>(pool, sourceObjectList.constBegin(),
                                                            sourceObjectList.constEnd(), filterObject,
                                                            reduceObject, initialObject).result();
        QCOMPARE(result2, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result3 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                    pool, sourceObjectList, filterObject, reduceObject, initialObject);
        QCOMPARE(result3, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result4 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                    pool, sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject, initialObject);
        QCOMPARE(result4, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working
    }

    // Result type is deduced
    {
        const ResultObject result1 = QtConcurrent::filteredReduced(
                    pool, sourceObjectList, filterObject, reduceObject, initialObject).result();
        QCOMPARE(result1, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result2 =
                QtConcurrent::filteredReduced(pool, sourceObjectList.constBegin(),
                                              sourceObjectList.constEnd(), filterObject,
                                              reduceObject, initialObject).result();
        QCOMPARE(result2, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result3 = QtConcurrent::blockingFilteredReduced(
                    pool, sourceObjectList, filterObject, reduceObject, initialObject);
        QCOMPARE(result3, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working

        const ResultObject result4 = QtConcurrent::blockingFilteredReduced(
                    pool, sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                    filterObject, reduceObject, initialObject);
        QCOMPARE(result4, expectedResult);
        QCOMPARE(threadCount(), 1); // ensure the only one thread was working
    }
}

void tst_QtConcurrentFilter::filteredReducedInitialValueThreadPool()
{
    const QList<int> intList {1, 2, 3, 4};
    const int intInitial = 10;
    const int intSum = 14; // sum of even values and initial value

    auto lambdaIsOdd = [](const int &x) {
        storeCurrentThread();
        return x & 1;
    };
    auto lambdaSumReduce = [](int &sum, const int &x) {
        sum += x;
    };

    QThreadPool pool;
    pool.setMaxThreadCount(1);
    QCOMPARE(semaphore.available(), 1);
    workingThreads.clear();

    // FUNCTOR-other
    testFilteredReducedInitialValueThreadPool(&pool, intList, intSum, KeepOddIntegers(),
                                              IntSumReduce(), intInitial);
    CHECK_FAIL("functor-functor");
    testFilteredReducedInitialValueThreadPool(&pool, intList, intSum, KeepOddIntegers(),
                                              intSumReduce, intInitial);
    CHECK_FAIL("functor-function");
    testFilteredReducedInitialValueThreadPool(&pool, intList, intSum, KeepOddIntegers(),
                                              lambdaSumReduce, intInitial);
    CHECK_FAIL("functor-lambda");

    // FUNCTION-other
    testFilteredReducedInitialValueThreadPool(&pool, intList, intSum, keepOddIntegers,
                                              IntSumReduce(), intInitial);
    CHECK_FAIL("function-functor");
    testFilteredReducedInitialValueThreadPool(&pool, intList, intSum, keepOddIntegers,
                                              intSumReduce, intInitial);
    CHECK_FAIL("function-function");
    testFilteredReducedInitialValueThreadPool(&pool, intList, intSum, keepOddIntegers,
                                              lambdaSumReduce, intInitial);
    CHECK_FAIL("function-lambda");

    // LAMBDA-other
    testFilteredReducedInitialValueThreadPool(&pool, intList, intSum, lambdaIsOdd,
                                              IntSumReduce(), intInitial);
    CHECK_FAIL("lambda-functor");
    testFilteredReducedInitialValueThreadPool(&pool, intList, intSum, lambdaIsOdd,
                                              intSumReduce, intInitial);
    CHECK_FAIL("lambda-function");
    testFilteredReducedInitialValueThreadPool(&pool, intList, intSum, lambdaIsOdd,
                                              lambdaSumReduce, intInitial);
    CHECK_FAIL("lambda-lambda");

    // non-template sequences
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto future =
                QtConcurrent::filteredReduced(list, keepOddIntegers, intSumReduce, intInitial);
        QCOMPARE(future.result(), intSum);
    }
    {
        NonTemplateSequence list({ 1, 2, 3, 4 });
        auto result = QtConcurrent::blockingFilteredReduced(list, keepOddIntegers, intSumReduce,
                                                            intInitial);
        QCOMPARE(result, intSum);
    }

    {
        // rvalue sequences
        auto future = QtConcurrent::filteredReduced(&pool, std::vector { 1, 2, 3, 4 },
                                                    keepOddIntegers, intSumReduce, intInitial);
        QCOMPARE(future.result(), intSum);

        auto result = QtConcurrent::blockingFilteredReduced(
                &pool, std::vector { 1, 2, 3, 4 }, keepOddIntegers, intSumReduce, intInitial);
        QCOMPARE(result, intSum);
    }

    {
        // move only sequences
        auto future = QtConcurrent::filteredReduced(&pool, MoveOnlyVector<int>({ 1, 2, 3, 4 }),
                                                    keepOddIntegers, intSumReduce, intInitial);
        QCOMPARE(future.result(), intSum);

        auto result =
                QtConcurrent::blockingFilteredReduced(&pool, MoveOnlyVector<int>({ 1, 2, 3, 4 }),
                                                      keepOddIntegers, intSumReduce, intInitial);
        QCOMPARE(result, intSum);
    }
}

void tst_QtConcurrentFilter::filteredReducedInitialValueWithMoveOnlyCallables()
{
    const QList<int> intList { 1, 2, 3, 4 };
    const QList<int> intListEven { 2, 4 };
    const auto initial = 10;
    const auto sum = 16;
    {
        const auto result =
                QtConcurrent::filteredReduced(intList, KeepEvenIntegersMoveOnly(),
                                              IntSumReduceMoveOnly(), initial).result();
        QCOMPARE(result, sum);
    }
    {
        const auto result =
                QtConcurrent::filteredReduced(intList.begin(), intList.end(),
                                              KeepEvenIntegersMoveOnly(),
                                              IntSumReduceMoveOnly(), initial).result();
        QCOMPARE(result, sum);
    }
    {
        const auto result = QtConcurrent::blockingFilteredReduced(
                    intList, KeepEvenIntegersMoveOnly(), IntSumReduceMoveOnly(), initial);
        QCOMPARE(result, sum);
    }
    {
        const auto result = QtConcurrent::blockingFilteredReduced(
                    intList.begin(), intList.end(), KeepEvenIntegersMoveOnly(), IntSumReduceMoveOnly(),
                    initial);
        QCOMPARE(result, sum);
    }

    QThreadPool pool;
    {
        const auto result =
                QtConcurrent::filteredReduced(&pool, intList, KeepEvenIntegersMoveOnly(),
                                              IntSumReduceMoveOnly(), initial).result();
        QCOMPARE(result, sum);
    }
    {
        const auto result =
                QtConcurrent::filteredReduced(
                    &pool, intList.begin(), intList.end(),
                    KeepEvenIntegersMoveOnly(), IntSumReduceMoveOnly(), initial).result();
        QCOMPARE(result, sum);
    }
    {
        const auto result = QtConcurrent::blockingFilteredReduced(
                    &pool, intList, KeepEvenIntegersMoveOnly(), IntSumReduceMoveOnly(), initial);
        QCOMPARE(result, sum);
    }
    {
        const auto result = QtConcurrent::blockingFilteredReduced(
                    &pool, intList.begin(), intList.end(), KeepEvenIntegersMoveOnly(),
                    IntSumReduceMoveOnly(), initial);
        QCOMPARE(result, sum);
    }
}

void tst_QtConcurrentFilter::filteredReducedDifferentTypeInitialValue()
{
    const QList<Number> numberList {1, 2, 3, 4};
    const int initial = 10;
    const int sum = 16; // sum of even values and initial value

    auto lambdaIsEven = [](const Number &x) {
        return (x.toInt() & 1) == 0;
    };
    auto lambdaSumReduce = [](int &sum, const Number &x) {
        sum += x.toInt();
    };

    // Test the case where reduce function of the form:
    // V function(T &result, const U &intermediate)
    // has T and U types different.

    // FUNCTOR-other
    testFilteredReducedInitialValue(numberList, sum, KeepEvenNumbers(), NumberSumReduce(), initial);
    CHECK_FAIL("functor-functor");
    testFilteredReducedInitialValue(numberList, sum, KeepEvenNumbers(), numberSumReduce, initial);
    CHECK_FAIL("functor-function");
    testFilteredReducedInitialValue(numberList, sum, KeepEvenNumbers(), lambdaSumReduce, initial);
    CHECK_FAIL("functor-lambda");

    // FUNCTION-other
    testFilteredReducedInitialValue(numberList, sum, keepEvenNumbers, NumberSumReduce(), initial);
    CHECK_FAIL("function-functor");
    testFilteredReducedInitialValue(numberList, sum, keepEvenNumbers, numberSumReduce, initial);
    CHECK_FAIL("function-function");
    testFilteredReducedInitialValue(numberList, sum, keepEvenNumbers, lambdaSumReduce, initial);
    CHECK_FAIL("function-lambda");

    // MEMBER-other
    testFilteredReducedInitialValue(numberList, sum, &Number::isEven, NumberSumReduce(), initial);
    CHECK_FAIL("member-functor");
    testFilteredReducedInitialValue(numberList, sum, &Number::isEven, numberSumReduce, initial);
    CHECK_FAIL("member-function");
    testFilteredReducedInitialValue(numberList, sum, &Number::isEven, lambdaSumReduce, initial);
    CHECK_FAIL("member-lambda");

    // LAMBDA-other
    testFilteredReducedInitialValue(numberList, sum, lambdaIsEven, NumberSumReduce(), initial);
    CHECK_FAIL("lambda-functor");
    testFilteredReducedInitialValue(numberList, sum, lambdaIsEven, numberSumReduce, initial);
    CHECK_FAIL("lambda-function");
    testFilteredReducedInitialValue(numberList, sum, lambdaIsEven, lambdaSumReduce, initial);
    CHECK_FAIL("lambda-lambda");
}

void tst_QtConcurrentFilter::filteredReduceOptionConvertableToResultType()
{
    const QList<int> intList { 1, 2, 3 };
    const int sum = 4;
    QThreadPool p;
    ReduceOption ro = OrderedReduce;

    // With container
    QCOMPARE(QtConcurrent::filteredReduced(intList, keepOddIntegers, intSumReduce, ro).result(),
             sum);
    QCOMPARE(QtConcurrent::blockingFilteredReduced(intList, keepOddIntegers, intSumReduce, ro),
             sum);

    // With iterators
    QCOMPARE(QtConcurrent::filteredReduced(intList.begin(), intList.end(), keepOddIntegers,
                                           intSumReduce, ro).result(), sum);
    QCOMPARE(QtConcurrent::blockingFilteredReduced(intList.begin(), intList.end(), keepOddIntegers,
                                                   intSumReduce, ro), sum);

    // With custom QThreadPool;
    QCOMPARE(QtConcurrent::filteredReduced(&p, intList, keepOddIntegers, intSumReduce, ro).result(),
             sum);
    QCOMPARE(QtConcurrent::blockingFilteredReduced(&p, intList, keepOddIntegers, intSumReduce, ro),
             sum);
    QCOMPARE(QtConcurrent::filteredReduced(&p, intList.begin(), intList.end(), keepOddIntegers,
                                           intSumReduce, ro).result(), sum);
    QCOMPARE(QtConcurrent::blockingFilteredReduced(&p, intList.begin(), intList.end(),
                                                   keepOddIntegers, intSumReduce, ro), sum);

    // The same as above, but specify the result type explicitly (this invokes different overloads)
    QCOMPARE(QtConcurrent::filteredReduced<int>(intList, keepOddIntegers, intSumReduce,
                                                ro).result(), sum);
    QCOMPARE(QtConcurrent::blockingFilteredReduced<int>(intList, keepOddIntegers, intSumReduce, ro),
             sum);

    QCOMPARE(QtConcurrent::filteredReduced<int>(intList.begin(), intList.end(), keepOddIntegers,
                                                intSumReduce, ro).result(), sum);
    QCOMPARE(QtConcurrent::blockingFilteredReduced<int>(intList.begin(), intList.end(),
                                                        keepOddIntegers, intSumReduce, ro), sum);

    QCOMPARE(QtConcurrent::filteredReduced<int>(&p, intList, keepOddIntegers, intSumReduce,
                                                ro).result(), sum);
    QCOMPARE(QtConcurrent::blockingFilteredReduced<int>(&p, intList, keepOddIntegers, intSumReduce,
                                                        ro), sum);
    QCOMPARE(QtConcurrent::filteredReduced<int>(&p, intList.begin(), intList.end(), keepOddIntegers,
                                                intSumReduce, ro).result(),sum);
    QCOMPARE(QtConcurrent::blockingFilteredReduced<int>(&p, intList.begin(), intList.end(),
                                                        keepOddIntegers, intSumReduce, ro), sum);
}

bool filterfn(int i)
{
    return (i % 2);
}

void tst_QtConcurrentFilter::resultAt()
{
    QList<int> ints;
    for (int i = 0; i < 1000; ++i)
        ints << i;

    QFuture<int> future = QtConcurrent::filtered(ints, filterfn);
    future.waitForFinished();

    for (int i = 0; i < future.resultCount(); ++i) {
        QCOMPARE(future.resultAt(i), ints.at(i * 2 + 1));
    }
}

bool waitFilterfn(const int &i)
{
    QTest::qWait(1);
    return (i % 2);
}

void tst_QtConcurrentFilter::incrementalResults()
{
    const int count = 200;
    QList<int> ints;
    for (int i = 0; i < count; ++i)
        ints << i;

    QFuture<int> future = QtConcurrent::filtered(ints, waitFilterfn);

    QList<int> results;

    while (future.isFinished() == false) {
        for (int i = 0; i < future.resultCount(); ++i) {
            results += future.resultAt(i);
        }
        QTest::qWait(1);
    }

    QCOMPARE(future.isFinished(), true);
    QCOMPARE(future.resultCount(), count / 2);
    QCOMPARE(future.results().size(), count / 2);
}

void tst_QtConcurrentFilter::noDetach()
{
    {
        QList<int> l = QList<int>() << 1;
        QVERIFY(l.isDetached());

        QList<int> ll = l;
        QVERIFY(!l.isDetached());

        QtConcurrent::filtered(l, waitFilterfn).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::blockingFiltered(l, waitFilterfn);

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::filteredReduced(l, waitFilterfn, intSumReduce).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::filter(l, waitFilterfn).waitForFinished();
        if (!l.isDetached())
            QEXPECT_FAIL("", "QTBUG-20688: Known unstable failure", Abort);
        QVERIFY(l.isDetached());
        QVERIFY(ll.isDetached());
    }
    {
        const QList<int> l = QList<int>() << 1;
        QVERIFY(l.isDetached());

        const QList<int> ll = l;
        QVERIFY(!l.isDetached());

        QtConcurrent::filtered(l, waitFilterfn).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());

        QtConcurrent::filteredReduced(l, waitFilterfn, intSumReduce).waitForFinished();

        QVERIFY(!l.isDetached());
        QVERIFY(!ll.isDetached());
    }
}

void tst_QtConcurrentFilter::stlContainers()
{
    std::vector<int> vector;
    vector.push_back(1);
    vector.push_back(2);

    std::vector<int> vector2 =  QtConcurrent::blockingFiltered(vector, waitFilterfn);
    QCOMPARE(vector2.size(), (std::vector<int>::size_type)(1));
    QCOMPARE(vector2[0], 1);

    std::list<int> list;
    list.push_back(1);
    list.push_back(2);

    std::list<int> list2 =  QtConcurrent::blockingFiltered(list, waitFilterfn);
    QCOMPARE(list2.size(), (std::list<int>::size_type)(1));
    QCOMPARE(*list2.begin(), 1);

    QtConcurrent::filtered(list, waitFilterfn).waitForFinished();
    QtConcurrent::filtered(vector, waitFilterfn).waitForFinished();
    QtConcurrent::filtered(vector.begin(), vector.end(), waitFilterfn).waitForFinished();

    QtConcurrent::blockingFilter(list, waitFilterfn);
    QCOMPARE(list2.size(), (std::list<int>::size_type)(1));
    QCOMPARE(*list2.begin(), 1);
}

void tst_QtConcurrentFilter::stlContainersLambda()
{
    auto waitFilterLambda = [](const int &i) {
        return waitFilterfn(i);
    };

    std::vector<int> vector;
    vector.push_back(1);
    vector.push_back(2);

    std::vector<int> vector2 = QtConcurrent::blockingFiltered(vector, waitFilterLambda);
    QCOMPARE(vector2.size(), (std::vector<int>::size_type)(1));
    QCOMPARE(vector2[0], 1);

    std::list<int> list;
    list.push_back(1);
    list.push_back(2);

    std::list<int> list2 = QtConcurrent::blockingFiltered(list, waitFilterLambda);
    QCOMPARE(list2.size(), (std::list<int>::size_type)(1));
    QCOMPARE(*list2.begin(), 1);

    QtConcurrent::filtered(list, waitFilterLambda).waitForFinished();
    QtConcurrent::filtered(vector, waitFilterLambda).waitForFinished();
    QtConcurrent::filtered(vector.begin(), vector.end(), waitFilterLambda).waitForFinished();

    QtConcurrent::blockingFilter(list, waitFilterLambda);
    QCOMPARE(list.size(), (std::list<int>::size_type)(1));
    QCOMPARE(*list.begin(), 1);
}

QTEST_MAIN(tst_QtConcurrentFilter)
#include "tst_qtconcurrentfilter.moc"
