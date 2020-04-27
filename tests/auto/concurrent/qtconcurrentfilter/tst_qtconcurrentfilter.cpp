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
#include <qtconcurrentfilter.h>
#include <QCoreApplication>
#include <QList>
#include <QtTest/QtTest>

#include "../qtconcurrentmap/functions.h"

class tst_QtConcurrentFilter : public QObject
{
    Q_OBJECT

private slots:
    void filter();
    void filtered();
    void filteredReduced();
    void filteredReducedDifferentType();
    void filteredReducedInitialValue();
    void filteredReducedDifferentTypeInitialValue();
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
    const ResultObject result1 = QtConcurrent::filteredReduced<ResultObject>(
                sourceObjectList, filterObject, reduceObject);
    QCOMPARE(result1, expectedResult);

    const ResultObject result2 = QtConcurrent::filteredReduced<ResultObject>(
                sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                filterObject, reduceObject);
    QCOMPARE(result2, expectedResult);

    const ResultObject result3 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                sourceObjectList, filterObject, reduceObject);
    QCOMPARE(result3, expectedResult);

    const ResultObject result4 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                filterObject, reduceObject);
    QCOMPARE(result4, expectedResult);
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
                sourceObjectList, filterObject, reduceObject, options);
    QCOMPARE(result1, expectedResult);

    const ResultObject result2 = QtConcurrent::filteredReduced(
                sourceObjectList.constBegin(), sourceObjectList.constEnd(), filterObject,
                reduceObject, options);
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

    void (QVector<int>::*pushBackInt)(const int &) = &QVector<int>::push_back;
    void (QVector<Number>::*pushBackNumber)(const Number &) = &QVector<Number>::push_back;

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
    const ResultObject result1 = QtConcurrent::filteredReduced<ResultObject>(
                sourceObjectList, filterObject, reduceObject, initialObject);
    QCOMPARE(result1, expectedResult);

    const ResultObject result2 = QtConcurrent::filteredReduced<ResultObject>(
                sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                filterObject, reduceObject, initialObject);
    QCOMPARE(result2, expectedResult);

    const ResultObject result3 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                sourceObjectList, filterObject, reduceObject, initialObject);
    QCOMPARE(result3, expectedResult);

    const ResultObject result4 = QtConcurrent::blockingFilteredReduced<ResultObject>(
                sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                filterObject, reduceObject, initialObject);
    QCOMPARE(result4, expectedResult);
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
                sourceObjectList, filterObject, reduceObject, initialObject, options);
    QCOMPARE(result1, expectedResult);

    const ResultObject result2 = QtConcurrent::filteredReduced(
                sourceObjectList.constBegin(), sourceObjectList.constEnd(),
                filterObject, reduceObject, initialObject, options);
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

    void (QVector<int>::*pushBackInt)(const int &) = &QVector<int>::push_back;
    void (QVector<Number>::*pushBackNumber)(const Number &) = &QVector<Number>::push_back;

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
    QCOMPARE(future.results().count(), count / 2);
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
