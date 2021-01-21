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

#ifndef QBENCHMARK_H
#define QBENCHMARK_H

#include <QtTest/qttestglobal.h>
#include <QtTest/qbenchmarkmetric.h>

QT_BEGIN_NAMESPACE


namespace QTest
{

//
//  W A R N I N G
//  -------------
//
// The QBenchmarkIterationController class is not a part of the
// Qt Test API. It exists purely as an implementation detail.
//
//
class Q_TESTLIB_EXPORT QBenchmarkIterationController
{
public:
    enum RunMode { RepeatUntilValidMeasurement, RunOnce };
    QBenchmarkIterationController();
    QBenchmarkIterationController(RunMode runMode);
    ~QBenchmarkIterationController();
    bool isDone();
    void next();
    int i;
};

}

// --- BEGIN public API ---

#define QBENCHMARK \
    for (QTest::QBenchmarkIterationController __iteration_controller; \
            __iteration_controller.isDone() == false; __iteration_controller.next())

#define QBENCHMARK_ONCE \
    for (QTest::QBenchmarkIterationController __iteration_controller(QTest::QBenchmarkIterationController::RunOnce); \
            __iteration_controller.isDone() == false; __iteration_controller.next())

namespace QTest
{
    void Q_TESTLIB_EXPORT setBenchmarkResult(qreal result, QBenchmarkMetric metric);
}

// --- END public API ---

QT_END_NAMESPACE

#endif // QBENCHMARK_H
