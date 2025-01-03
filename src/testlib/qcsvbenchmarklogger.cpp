// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcsvbenchmarklogger_p.h"
#include "qtestresult_p.h"
#include "qbenchmark_p.h"

/*! \internal
    \class QCsvBenchmarkLogger
    \inmodule QtTest

    QCsvBenchmarkLogger implements a comma-separated value format for benchmarks.

    This is intended to be suitable for import into spreadsheets.
    It does not print test failures, debug messages, warnings or any other details.
*/

QCsvBenchmarkLogger::QCsvBenchmarkLogger(const char *filename)
    : QAbstractTestLogger(filename)
{
}

QCsvBenchmarkLogger::~QCsvBenchmarkLogger() = default;

void QCsvBenchmarkLogger::startLogging()
{
    // don't print anything
}

void QCsvBenchmarkLogger::stopLogging()
{
    // don't print anything
}

void QCsvBenchmarkLogger::enterTestFunction(const char *)
{
    // don't print anything
}

void QCsvBenchmarkLogger::leaveTestFunction()
{
    // don't print anything
}

void QCsvBenchmarkLogger::addIncident(QAbstractTestLogger::IncidentTypes, const char *, const char *, int)
{
    // don't print anything
}

void QCsvBenchmarkLogger::addBenchmarkResult(const QBenchmarkResult &result)
{
    const char *fn = QTestResult::currentTestFunction() ? QTestResult::currentTestFunction()
        : "UnknownTestFunc";
    const char *tag = QTestResult::currentDataTag() ? QTestResult::currentDataTag() : "";
    const char *gtag = QTestResult::currentGlobalDataTag()
                     ? QTestResult::currentGlobalDataTag()
                     : "";
    const char *filler = (tag[0] && gtag[0]) ? ":" : "";

    const char *metric = QTest::benchmarkMetricName(result.measurement.metric);

    char buf[1024];
    // "function","[globaltag:]tag","metric",value_per_iteration,total,iterations
    qsnprintf(buf, sizeof(buf), "\"%s\",\"%s%s%s\",\"%s\",%.13g,%.13g,%u\n",
              fn, gtag, filler, tag, metric,
              result.measurement.value / result.iterations,
              result.measurement.value, result.iterations);
    outputString(buf);
}

void QCsvBenchmarkLogger::addMessage(QAbstractTestLogger::MessageTypes, const QString &, const char *, int)
{
    // don't print anything
}
