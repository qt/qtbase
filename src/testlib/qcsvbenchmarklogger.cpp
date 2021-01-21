/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qcsvbenchmarklogger_p.h"
#include "qtestresult_p.h"
#include "qbenchmark_p.h"

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

    const char *metric = QTest::benchmarkMetricName(result.metric);

    char buf[1024];
    // "function","[globaltag:]tag","metric",value_per_iteration,total,iterations
    qsnprintf(buf, sizeof(buf), "\"%s\",\"%s%s%s\",\"%s\",%.13g,%.13g,%u\n",
              fn, gtag, filler, tag, metric,
              result.value / result.iterations, result.value, result.iterations);
    outputString(buf);
}

void QCsvBenchmarkLogger::addMessage(QAbstractTestLogger::MessageTypes, const QString &, const char *, int)
{
    // don't print anything
}
