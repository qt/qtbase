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

// include the qcore_unix_p.h without core-private
// we only use inline functions anyway
#include "../corelib/kernel/qcore_unix_p.h"

#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <sys/syscall.h>
#include <sys/ioctl.h>

#include "3rdparty/linux_perf_event_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QBenchmarkPerfEvents
    \brief The Linux perf events benchmark backend

    This benchmark backend uses the Linux Performance Counters interface,
    introduced with the Linux kernel v2.6.31. The interface is done by one
    system call (perf_event_open) which takes an attribute structure and
    returns a file descriptor.

    More information:
     \li design docs: tools/perf/design.txt <http://lxr.linux.no/linux/tools/perf/design.txt>
     \li sample tool: tools/perf/builtin-stat.c <http://lxr.linux.no/linux/tools/perf/builtin-stat.c>
    (note: as of v3.3.1, the documentation is out-of-date with the kernel
    interface, so reading the source code of existing tools is necessary)

    This benchlib backend monitors the current process as well as child process
    launched. We do not try to benchmark in kernel or hypervisor mode, as that
    usually requires elevated privileges.
 */

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
    : fd(-1)
{
}

QBenchmarkPerfEventsMeasurer::~QBenchmarkPerfEventsMeasurer()
{
    qt_safe_close(fd);
}

void QBenchmarkPerfEventsMeasurer::init()
{
    perf_event_attr attr;
    memset(&attr, 0, sizeof attr);

    // common init
    attr.size = sizeof attr;
    attr.sample_period = 0;
    attr.sample_type = 0;
    attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    attr.disabled = true; // start disabled, we'll enable later
    attr.inherit = true; // let children inherit, if the benchmark has child processes
    attr.pinned = true; // keep it running on the PMU
    attr.inherit_stat = true; // collapse all the info from child processes
    attr.task = true; // trace fork and exit

    // our event type
    // ### FIXME hardcoded for now
    attr.type = PERF_TYPE_HARDWARE;
    attr.config = PERF_COUNT_HW_CPU_CYCLES;

    // pid == 0 -> attach to the current process
    // cpu == -1 -> monitor on all CPUs
    // group_fd == -1 -> this is the group leader
    // flags == 0 -> reserved, must be zero
    fd = perf_event_open(&attr, 0, -1, -1, 0);
    if (fd == -1) {
        perror("QBenchmarkPerfEventsMeasurer::start: perf_event_open");
        exit(1);
    } else {
        ::fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
}

void QBenchmarkPerfEventsMeasurer::start()
{
    // enable the counter
    ::ioctl(fd, PERF_EVENT_IOC_RESET);
    ::ioctl(fd, PERF_EVENT_IOC_ENABLE);
}

qint64 QBenchmarkPerfEventsMeasurer::checkpoint()
{
    ::ioctl(fd, PERF_EVENT_IOC_DISABLE);
    qint64 value = readValue();
    ::ioctl(fd, PERF_EVENT_IOC_ENABLE);
    return value;
}

qint64 QBenchmarkPerfEventsMeasurer::stop()
{
    // disable the counter
    ::ioctl(fd, PERF_EVENT_IOC_DISABLE);
    return readValue();
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

qint64 QBenchmarkPerfEventsMeasurer::readValue()
{
    /* from the kernel docs:
     * struct read_format {
     *  { u64           value;
     *    { u64         time_enabled; } && PERF_FORMAT_TOTAL_TIME_ENABLED
     *    { u64         time_running; } && PERF_FORMAT_TOTAL_TIME_RUNNING
     *    { u64         id;           } && PERF_FORMAT_ID
     *  } && !PERF_FORMAT_GROUP
     */

    struct read_format {
        quint64 value;
        quint64 time_enabled;
        quint64 time_running;
    } results;

    size_t nread = 0;
    while (nread < sizeof results) {
        char *ptr = reinterpret_cast<char *>(&results);
        qint64 r = qt_safe_read(fd, ptr + nread, sizeof results - nread);
        if (r == -1) {
            perror("QBenchmarkPerfEventsMeasurer::readValue: reading the results");
            exit(1);
        }
        nread += quint64(r);
    }

    if (results.time_running == results.time_enabled)
        return results.value;

    // scale the results, though this shouldn't happen!
    return results.value * (double(results.time_running) / double(results.time_enabled));
}

QT_END_NAMESPACE

#endif
