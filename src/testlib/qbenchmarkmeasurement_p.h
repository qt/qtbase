// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBENCHMARKMEASUREMENT_P_H
#define QBENCHMARKMEASUREMENT_P_H

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

#include <QtTest/qbenchmark.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class QBenchmarkMeasurerBase
{
public:
    virtual ~QBenchmarkMeasurerBase() = default;
    virtual void init() {}
    virtual void start() = 0;
    virtual qint64 checkpoint() = 0;
    virtual qint64 stop() = 0;
    virtual bool isMeasurementAccepted(qint64 measurement) = 0;
    virtual int adjustIterationCount(int suggestion) = 0;
    virtual int adjustMedianCount(int suggestion) = 0;
    virtual bool repeatCount() { return true; }
    virtual bool needsWarmupIteration() { return false; }
    virtual QTest::QBenchmarkMetric metricType() = 0;
};

QT_END_NAMESPACE

#endif // QBENCHMARKMEASUREMENT_P_H
