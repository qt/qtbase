/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/private/qbenchmarkevent_p.h>
#include <QtTest/private/qbenchmark_p.h>
#include <QtTest/private/qbenchmarkmetric_p.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

QAbstractEventDispatcher::EventFilter oldEventFilter = 0;
qint64 QBenchmarkEvent::eventCounter = 0;

QBenchmarkEvent::~QBenchmarkEvent()
{
}

void QBenchmarkEvent::start()
{
    QBenchmarkEvent::eventCounter = 0;
    QAbstractEventDispatcher *parent = QAbstractEventDispatcher::instance();
    oldEventFilter = parent->setEventFilter(QBenchmarkEvent::eventCountingMechanism);
}

qint64 QBenchmarkEvent::checkpoint()
{
    return QBenchmarkEvent::eventCounter;
}

qint64 QBenchmarkEvent::stop()
{
    QAbstractEventDispatcher *parent = QAbstractEventDispatcher::instance();
    parent->setEventFilter(oldEventFilter);
    return QBenchmarkEvent::eventCounter;
}

// It's very tempting to simply reject a measurement if 0 events
// where counted, however that is a possible situation and returning
// false here will create a infinite loop. Do not change this.
bool QBenchmarkEvent::isMeasurementAccepted(qint64 measurement)
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

QTest::QBenchmarkMetric QBenchmarkEvent::metricType()
{
    return QTest::Events;
}

// This could be done in a much better way, this is just the beginning.
bool QBenchmarkEvent::eventCountingMechanism(void *message)
{
    Q_UNUSED(message);
    QBenchmarkEvent::eventCounter++;
    return false;
}

QT_END_NAMESPACE
