// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/qbenchmark.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>
#include <QtTest/private/qbenchmarktimemeasurers_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qset.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QBenchmarkGlobalData *QBenchmarkGlobalData::current;

QBenchmarkGlobalData::QBenchmarkGlobalData()
{
    setMode(mode_);
}

QBenchmarkGlobalData::~QBenchmarkGlobalData()
{
    delete measurer;
    if (QBenchmarkGlobalData::current == this)
        QBenchmarkGlobalData::current = nullptr;
}

void QBenchmarkGlobalData::setMode(Mode mode)
{
    mode_ = mode;

    delete measurer;
    measurer = createMeasurer();
}

QBenchmarkMeasurerBase * QBenchmarkGlobalData::createMeasurer()
{
    QBenchmarkMeasurerBase *measurer = nullptr;
    if (0) {
#if QT_CONFIG(valgrind)
    } else if (mode_ == CallgrindChildProcess || mode_ == CallgrindParentProcess) {
        measurer = new QBenchmarkCallgrindMeasurer;
#endif
#ifdef QTESTLIB_USE_PERF_EVENTS
    } else if (mode_ == PerfCounter) {
        measurer = new QBenchmarkPerfEventsMeasurer;
#endif
#ifdef HAVE_TICK_COUNTER
    } else if (mode_ == TickCounter) {
        measurer = new QBenchmarkTickMeasurer;
#endif
    } else if (mode_ == EventCounter) {
        measurer = new QBenchmarkEvent;
    } else {
        measurer =  new QBenchmarkTimeMeasurer;
    }
    measurer->init();
    return measurer;
}

int QBenchmarkGlobalData::adjustMedianIterationCount()
{
    return medianIterationCount != -1
        ? medianIterationCount : measurer->adjustMedianCount(1);
}


QBenchmarkTestMethodData *QBenchmarkTestMethodData::current;

QBenchmarkTestMethodData::QBenchmarkTestMethodData() = default;

QBenchmarkTestMethodData::~QBenchmarkTestMethodData()
{
    QBenchmarkTestMethodData::current = nullptr;
}

void QBenchmarkTestMethodData::beginDataRun()
{
    iterationCount = adjustIterationCount(1);
}

void QBenchmarkTestMethodData::endDataRun()
{
}

int QBenchmarkTestMethodData::adjustIterationCount(int suggestion)
{
    // Let the -iterations option override the measurer.
    if (QBenchmarkGlobalData::current->iterationCount != -1) {
        iterationCount = QBenchmarkGlobalData::current->iterationCount;
    } else {
        iterationCount = QBenchmarkGlobalData::current->measurer->adjustIterationCount(suggestion);
    }

    return iterationCount;
}

void QBenchmarkTestMethodData::setResults(const QList<QBenchmarkMeasurerBase::Measurement> &list,
                                          bool setByMacro)
{
    bool accepted = false;
    QBenchmarkMeasurerBase::Measurement firstMeasurement = {};
    if (!list.isEmpty())
        firstMeasurement = list.constFirst();

    // Always accept the result if the iteration count has been
    // specified on the command line with -iterations.
    if (QBenchmarkGlobalData::current->iterationCount != -1)
        accepted = true;

    else if (QBenchmarkTestMethodData::current->runOnce || !setByMacro) {
        iterationCount = 1;
        accepted = true;
    }

    // Test the result directly without calling the measurer if the minimum time
    // has been specified on the command line with -minimumvalue.
    else if (QBenchmarkGlobalData::current->walltimeMinimum != -1)
        accepted = (firstMeasurement.value > QBenchmarkGlobalData::current->walltimeMinimum);
    else
        accepted = QBenchmarkGlobalData::current->measurer->isMeasurementAccepted(firstMeasurement);

    // Accept the result or double the number of iterations.
    if (accepted)
        resultAccepted = true;
    else
        iterationCount *= 2;

    valid = true;
    results.reserve(list.size());
    for (auto m : list)
        results.emplaceBack(QBenchmarkGlobalData::current->context, m, iterationCount, setByMacro);
}

/*!
    \class QTest::QBenchmarkIterationController
    \internal

    The QBenchmarkIterationController class is used by the QBENCHMARK macro to
    drive the benchmarking loop. It is responsible for starting and stopping
    the timing measurements as well as calling the result reporting functions.
*/

/*! \internal
*/
QTest::QBenchmarkIterationController::QBenchmarkIterationController(RunMode runMode)
{
    i = 0;
    if (runMode == RunOnce)
        QBenchmarkTestMethodData::current->runOnce = true;
    QTest::beginBenchmarkMeasurement();
}

QTest::QBenchmarkIterationController::QBenchmarkIterationController()
{
    i = 0;
    QTest::beginBenchmarkMeasurement();
}

/*! \internal
*/
QTest::QBenchmarkIterationController::~QBenchmarkIterationController()
{
    QBenchmarkTestMethodData::current->setResults(QTest::endBenchmarkMeasurement());
}

/*! \internal
*/
bool QTest::QBenchmarkIterationController::isDone()
{
    if (QBenchmarkTestMethodData::current->runOnce)
        return i > 0;
    return i >= QTest::iterationCount();
}

/*! \internal
*/
void QTest::QBenchmarkIterationController::next()
{
    ++i;
}

/*! \internal
*/
int QTest::iterationCount()
{
    return QBenchmarkTestMethodData::current->iterationCount;
}

/*! \internal
*/
void QTest::setIterationCountHint(int count)
{
    QBenchmarkTestMethodData::current->adjustIterationCount(count);
}

/*! \internal
*/
void QTest::setIterationCount(int count)
{
    QBenchmarkTestMethodData::current->iterationCount = count;
    QBenchmarkTestMethodData::current->resultAccepted = true;
}

/*! \internal
*/
void QTest::beginBenchmarkMeasurement()
{
    QBenchmarkGlobalData::current->measurer->start();
    // the clock is ticking after the line above, don't add code here.
}

/*! \internal
*/
QList<QBenchmarkMeasurerBase::Measurement> QTest::endBenchmarkMeasurement()
{
    // the clock is ticking before the line below, don't add code here.
    return QBenchmarkGlobalData::current->measurer->stop();
}

/*!
    Sets the benchmark result for this test function to \a result.

    Use this function if you want to report benchmark results without
    using the QBENCHMARK macro. Use \a metric to specify how Qt Test
    should interpret the results.

    The context for the result will be the test function name and any
    data tag from the _data function. This function can only be called
    once in each test function, subsequent calls will replace the
    earlier reported results.

    Note that the -iterations command line argument has no effect
    on test functions without the QBENCHMARK macro.

    \since 4.7
*/
void QTest::setBenchmarkResult(qreal result, QTest::QBenchmarkMetric metric)
{
    QBenchmarkTestMethodData::current->setResult({ result, metric }, false);
}

template <typename T>
typename T::value_type qAverage(const T &container)
{
    typename T::const_iterator it = container.constBegin();
    typename T::const_iterator end = container.constEnd();
    typename T::value_type acc = typename T::value_type();
    int count = 0;
    while (it != end) {
        acc += *it;
        ++it;
        ++count;
    }
    return acc / count;
}

QT_END_NAMESPACE
