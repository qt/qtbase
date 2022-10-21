// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qbenchmarkevent_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

QBenchmarkEvent::QBenchmarkEvent() = default;

QBenchmarkEvent::~QBenchmarkEvent() = default;

void QBenchmarkEvent::start()
{
    eventCounter = 0;
    QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
}

QList<QBenchmarkMeasurerBase::Measurement> QBenchmarkEvent::stop()
{
    QAbstractEventDispatcher::instance()->removeNativeEventFilter(this);
    return { { qreal(eventCounter), QTest::Events } };
}

// It's very tempting to simply reject a measurement if 0 events
// where counted, however that is a possible situation and returning
// false here will create a infinite loop. Do not change this.
bool QBenchmarkEvent::isMeasurementAccepted(QBenchmarkMeasurerBase::Measurement measurement)
{
    Q_UNUSED(measurement);
    return true;
}

int QBenchmarkEvent::adjustIterationCount(int suggestion)
{
    return suggestion;
}

int QBenchmarkEvent::adjustMedianCount(int suggestion)
{
    Q_UNUSED(suggestion);
    return 1;
}

// This could be done in a much better way, this is just the beginning.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool QBenchmarkEvent::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
#else
bool QBenchmarkEvent::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
#endif
{
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);

    eventCounter++;
    return false;
}

QT_END_NAMESPACE
