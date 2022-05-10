// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBENCHMARKPERFEVENTS_P_H
#define QBENCHMARKPERFEVENTS_P_H

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

QT_BEGIN_NAMESPACE

class QBenchmarkPerfEventsMeasurer : public QBenchmarkMeasurerBase
{
public:
    QBenchmarkPerfEventsMeasurer();
    ~QBenchmarkPerfEventsMeasurer();
    void init() override;
    void start() override;
    qint64 checkpoint() override;
    qint64 stop() override;
    bool isMeasurementAccepted(qint64 measurement) override;
    int adjustIterationCount(int suggestion) override;
    int adjustMedianCount(int suggestion) override;
    bool repeatCount() override { return true; }
    bool needsWarmupIteration() override { return true; }
    QTest::QBenchmarkMetric metricType() override;

    static bool isAvailable();
    static QTest::QBenchmarkMetric metricForEvent(quint32 type, quint64 event_id);
    static void setCounter(const char *name);
    static void listCounters();
private:
    int fd = -1;

    qint64 readValue();
};

QT_END_NAMESPACE

#endif // QBENCHMARKPERFEVENTS_P_H
