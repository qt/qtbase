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

qint64 QBenchmarkTimeMeasurer::checkpoint()
{
    return time.elapsed();
}

qint64 QBenchmarkTimeMeasurer::stop()
{
    return time.elapsed();
}

bool QBenchmarkTimeMeasurer::isMeasurementAccepted(qint64 measurement)
{
    return (measurement > 50);
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

QTest::QBenchmarkMetric QBenchmarkTimeMeasurer::metricType()
{
    return QTest::WalltimeMilliseconds;
}

#ifdef HAVE_TICK_COUNTER // defined in 3rdparty/cycle_p.h

void QBenchmarkTickMeasurer::start()
{
    startTicks = getticks();
}

qint64 QBenchmarkTickMeasurer::checkpoint()
{
    CycleCounterTicks now = getticks();
    return qRound64(elapsed(now, startTicks));
}

qint64 QBenchmarkTickMeasurer::stop()
{
    CycleCounterTicks now = getticks();
    return qRound64(elapsed(now, startTicks));
}

bool QBenchmarkTickMeasurer::isMeasurementAccepted(qint64)
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

QTest::QBenchmarkMetric QBenchmarkTickMeasurer::metricType()
{
    return QTest::CPUTicks;
}

#endif


QT_END_NAMESPACE
