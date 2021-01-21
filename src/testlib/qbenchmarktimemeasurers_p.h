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

#ifndef QBENCHMARKTIMEMEASURERS_P_H
#define QBENCHMARKTIMEMEASURERS_P_H

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

#include <QtTest/private/qbenchmarkmeasurement_p.h>
#include <QtCore/qelapsedtimer.h>
#include <QtTest/private/cycle_p.h>

QT_BEGIN_NAMESPACE

class QBenchmarkTimeMeasurer : public QBenchmarkMeasurerBase
{
public:
    void start() override;
    qint64 checkpoint() override;
    qint64 stop() override;
    bool isMeasurementAccepted(qint64 measurement) override;
    int adjustIterationCount(int sugestion) override;
    int adjustMedianCount(int suggestion) override;
    bool needsWarmupIteration() override;
    QTest::QBenchmarkMetric metricType() override;
private:
    QElapsedTimer time;
};

#ifdef HAVE_TICK_COUNTER // defined in 3rdparty/cycle_p.h

class QBenchmarkTickMeasurer : public QBenchmarkMeasurerBase
{
public:
    void start() override;
    qint64 checkpoint() override;
    qint64 stop() override;
    bool isMeasurementAccepted(qint64 measurement) override;
    int adjustIterationCount(int) override;
    int adjustMedianCount(int suggestion) override;
    bool needsWarmupIteration() override;
    QTest::QBenchmarkMetric metricType() override;
private:
    CycleCounterTicks startTicks;
};

#endif // HAVE_TICK_COUNTER

QT_END_NAMESPACE

#endif // QBENCHMARKTIMEMEASURERS_P_H
