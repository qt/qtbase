/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
