// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qbenchmarktimemeasurers_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>
#include <QtTest/qbenchmark.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

// QBenchmarkTimeMeasurer implementation

void QBenchmarkTimeMeasurer::start()
{
    time.start();
}

void QBenchmarkTimeMeasurer::updateMeasurement()
{
    const qreal value = std::chrono::duration<qreal, std::milli>(time.durationElapsed()).count();
    accumulator.update(value);
    time.restart();
}

QList<QBenchmarkMeasurerBase::Measurement> QBenchmarkTimeMeasurer::stop()
{
    return { { accumulator.total(),
               accumulator.variance() / QBenchmarkTestMethodData::current->measurementBlockSize,
               QTest::WalltimeMilliseconds } };
}

bool QBenchmarkTimeMeasurer::isMeasurementAccepted(Measurement measurement)
{
    return (measurement.value > 50);
}

int QBenchmarkTimeMeasurer::adjustIterationCount(int suggestion)
{
    return suggestion;
}

bool QBenchmarkTimeMeasurer::needsWarmupIteration()
{
    return true;
}

int QBenchmarkTimeMeasurer::adjustMedianCount(int)
{
    return 1;
}

#ifdef HAVE_TICK_COUNTER // defined in 3rdparty/cycle/cycle_p.h

void QBenchmarkTickMeasurer::start()
{
    startTicks = getticks();
}

void QBenchmarkTickMeasurer::updateMeasurement()
{
    const qreal value = qreal(elapsed(getticks(), startTicks));
    accumulator.update(value);
    startTicks = getticks();
}

QList<QBenchmarkMeasurerBase::Measurement> QBenchmarkTickMeasurer::stop()
{
    return { { accumulator.total(),
               accumulator.variance() / QBenchmarkTestMethodData::current->measurementBlockSize,
               QTest::CPUTicks } };
}

bool QBenchmarkTickMeasurer::isMeasurementAccepted(QBenchmarkMeasurerBase::Measurement)
{
    return true;
}

int QBenchmarkTickMeasurer::adjustIterationCount(int)
{
    return 1;
}

int QBenchmarkTickMeasurer::adjustMedianCount(int)
{
    return 1;
}

bool QBenchmarkTickMeasurer::needsWarmupIteration()
{
    return true;
}

#endif


QT_END_NAMESPACE
