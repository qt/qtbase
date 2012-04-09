/****************************************************************************
**
** Copyright (C) 2013 Intel Corporation.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qbenchmarkperfevents_p.h"
#include "qbenchmark_p.h"

#ifdef QTESTLIB_USE_PERF_EVENTS

#include <sys/types.h>
#include <errno.h>

#include <sys/syscall.h>

#include "3rdparty/linux_perf_event_p.h"

QT_BEGIN_NAMESPACE


static int perf_event_open(perf_event_attr *attr, pid_t pid, int cpu, int group_fd, unsigned long flags)
{
    return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

bool QBenchmarkPerfEventsMeasurer::isAvailable()
{
    // this generates an EFAULT because attr == NULL if perf_event_open is available
    // if the kernel is too old, it generates ENOSYS
    return perf_event_open(0, 0, 0, 0, 0) == -1 && errno != ENOSYS;
}

QBenchmarkPerfEventsMeasurer::QBenchmarkPerfEventsMeasurer()
{
}

QBenchmarkPerfEventsMeasurer::~QBenchmarkPerfEventsMeasurer()
{
}

void QBenchmarkPerfEventsMeasurer::init()
{
}

void QBenchmarkPerfEventsMeasurer::start()
{
}

qint64 QBenchmarkPerfEventsMeasurer::checkpoint()
{
}

qint64 QBenchmarkPerfEventsMeasurer::stop()
{
}

bool QBenchmarkPerfEventsMeasurer::isMeasurementAccepted(qint64)
{
    return true;
}

int QBenchmarkPerfEventsMeasurer::adjustIterationCount(int)
{
    return 1;
}

int QBenchmarkPerfEventsMeasurer::adjustMedianCount(int)
{
    return 1;
}

QTest::QBenchmarkMetric QBenchmarkPerfEventsMeasurer::metricType()
{
    return QTest::Events;
}

#endif

QT_END_NAMESPACE
