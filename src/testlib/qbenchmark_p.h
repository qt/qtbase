// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBENCHMARK_P_H
#define QBENCHMARK_P_H

#include <stdlib.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#if defined(Q_OS_LINUX) && !defined(QT_LINUXBASE) && !defined(Q_OS_ANDROID)
#define QTESTLIB_USE_PERF_EVENTS
#else
#undef QTESTLIB_USE_PERF_EVENTS
#endif

#include <QtTest/private/qbenchmarkmeasurement_p.h>
#include <QtCore/QMap>
#include <QtTest/qttestglobal.h>
#if QT_CONFIG(valgrind)
#include <QtTest/private/qbenchmarkvalgrind_p.h>
#endif
#ifdef QTESTLIB_USE_PERF_EVENTS
#include <QtTest/private/qbenchmarkperfevents_p.h>
#endif
#include <QtTest/private/qbenchmarkevent_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>

QT_BEGIN_NAMESPACE

struct QBenchmarkContext
{
    // None of the strings below are assumed to contain commas (see toString() below)
    QString slotName;
    QString tag; // from _data() function

    int checkpointIndex = -1;

    QString toString() const
    {
        return QString::fromLatin1("%1,%2,%3")
               .arg(slotName, tag, QString::number(checkpointIndex));
    }

    QBenchmarkContext()  = default;
};
Q_DECLARE_TYPEINFO(QBenchmarkContext, Q_RELOCATABLE_TYPE);

class QBenchmarkResult
{
public:
    QBenchmarkContext context;
    qreal value = -1;
    int iterations = -1;
    QTest::QBenchmarkMetric metric = QTest::FramesPerSecond;
    bool setByMacro = true;
    bool valid = false;

    QBenchmarkResult() = default;

    QBenchmarkResult(
        const QBenchmarkContext &context, const qreal value, const int iterations,
        QTest::QBenchmarkMetric metric, bool setByMacro)
        : context(context)
        , value(value)
        , iterations(iterations)
        , metric(metric)
        , setByMacro(setByMacro)
        , valid(true)
    { }

    bool operator<(const QBenchmarkResult &other) const
    {
        return (value / iterations) < (other.value / other.iterations);
    }
};
Q_DECLARE_TYPEINFO(QBenchmarkResult, Q_RELOCATABLE_TYPE);

/*
    The QBenchmarkGlobalData class stores global benchmark-related data.
    QBenchmarkGlobalData:current is created at the beginning of qExec()
    and cleared at the end.
*/
class Q_TESTLIB_EXPORT QBenchmarkGlobalData
{
public:
    static QBenchmarkGlobalData *current;

    QBenchmarkGlobalData();
    ~QBenchmarkGlobalData();
    enum Mode { WallTime, CallgrindParentProcess, CallgrindChildProcess, PerfCounter, TickCounter, EventCounter };
    void setMode(Mode mode);
    Mode mode() const { return mode_; }
    QBenchmarkMeasurerBase *createMeasurer();
    int adjustMedianIterationCount();

    QBenchmarkMeasurerBase *measurer = nullptr;
    QBenchmarkContext context;
    int walltimeMinimum = -1;
    int iterationCount = -1;
    int medianIterationCount = -1;
    bool createChart = false;
    bool verboseOutput = false;
    QString callgrindOutFileBase;
    int minimumTotal = -1;
private:
    Mode mode_ = WallTime;
};

/*
    The QBenchmarkTestMethodData class stores all benchmark-related data
    for the current test case. QBenchmarkTestMethodData:current is
    created at the beginning of qInvokeTestMethod() and cleared at
    the end.
*/
class Q_TESTLIB_EXPORT QBenchmarkTestMethodData
{
public:
    static QBenchmarkTestMethodData *current;
    QBenchmarkTestMethodData();
    ~QBenchmarkTestMethodData();

    // Called once for each data row created by the _data function,
    // before and after calling the test function itself.
    void beginDataRun();
    void endDataRun();

    bool isBenchmark() const { return result.valid; }
    bool resultsAccepted() const { return resultAccepted; }
    int adjustIterationCount(int suggestion);
    void setResult(qreal value, QTest::QBenchmarkMetric metric, bool setByMacro = true);

    QBenchmarkResult result;
    bool resultAccepted = false;
    bool runOnce = false;
    int iterationCount = -1;
};

// low-level API:
namespace QTest
{
    int iterationCount();
    void setIterationCountHint(int count);
    void setIterationCount(int count);

    Q_TESTLIB_EXPORT void beginBenchmarkMeasurement();
    Q_TESTLIB_EXPORT quint64 endBenchmarkMeasurement();
}

QT_END_NAMESPACE

#endif // QBENCHMARK_H
