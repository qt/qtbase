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
#include "qbenchmarkmetric.h"
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

static quint32 event_type = PERF_TYPE_HARDWARE;
static quint64 event_id = PERF_COUNT_HW_CPU_CYCLES;

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

/* Event list structure
   The following table provides the list of supported events

   Event type   Event counter           Unit            Name and aliases
   HARDWARE     CPU_CYCLES              CPUCycles       cycles  cpu-cycles
   HARDWARE     INSTRUCTIONS            Instructions    instructions
   HARDWARE     CACHE_REFERENCES        CacheReferences cache-references
   HARDWARE     CACHE_MISSES            CacheMisses     cache-misses
   HARDWARE     BRANCH_INSTRUCTIONS     BranchInstructions branch-instructions branches
   HARDWARE     BRANCH_MISSES           BranchMisses    branch-misses
   HARDWARE     BUS_CYCLES              BusCycles       bus-cycles
   HARDWARE     STALLED_CYCLES_FRONTEND StalledCycles   stalled-cycles-frontend idle-cycles-frontend
   HARDWARE     STALLED_CYCLES_BACKEND  StalledCycles   stalled-cycles-backend idle-cycles-backend
   SOFTWARE     CPU_CLOCK               WalltimeMilliseconds cpu-clock
   SOFTWARE     TASK_CLOCK              WalltimeMilliseconds task-clock
   SOFTWARE     PAGE_FAULTS             PageFaults      page-faults faults
   SOFTWARE     PAGE_FAULTS_MAJ         MajorPageFaults major-faults
   SOFTWARE     PAGE_FAULTS_MIN         MinorPageFaults minor-faults
   SOFTWARE     CONTEXT_SWITCHES        ContextSwitches context-switches cs
   SOFTWARE     CPU_MIGRATIONS          CPUMigrations   cpu-migrations migrations
   SOFTWARE     ALIGNMENT_FAULTS        AlignmentFaults alignment-faults
   SOFTWARE     EMULATION_FAULTS        EmulationFaults emulation-faults

   Use the following Perl script to re-generate the list
=== cut perl ===
#!/usr/bin/env perl
# Load all entries into %map
while (<STDIN>) {
    m/^\s*(.*)\s*$/;
    @_ = split /\s+/, $1;
    $type = shift @_;
    $id = ($type eq "HARDWARE" ? "PERF_COUNT_HW_" :
       $type eq "SOFTWARE" ? "PERF_COUNT_SW_" :
       $type eq "HW_CACHE" ? "CACHE_" : "") . shift @_;
    $unit = shift @_;

    for $string (@_) {
    die "$string was already seen!" if defined($map{$string});
    $map{$string} = [-1, $type, $id, $unit];
    push @strings, $string;
    }
}

# sort the map and print the string list
@strings = sort @strings;
print "static const char eventlist_strings[] = \n";
$counter = 0;
for $entry (@strings) {
    print "    \"$entry\\0\"\n";
    $map{$entry}[0] = $counter;
    $counter += 1 + length $entry;
}

# print the table
print "    \"\\0\";\n\nstatic const Events eventlist[] = {\n";
for $entry (sort @strings) {
    printf "    { %3d, PERF_TYPE_%s, %s, QTest::%s },\n",
        $map{$entry}[0],
    $map{$entry}[1],
        $map{$entry}[2],
        $map{$entry}[3];
}
print "    {   0, PERF_TYPE_MAX, 0, QTest::Events }\n};\n";
=== cut perl ===
*/

struct Events {
    unsigned offset;
    quint32 type;
    quint64 event_id;
    QTest::QBenchmarkMetric metric;
};

/* -- BEGIN GENERATED CODE -- */
static const char eventlist_strings[] =
    "alignment-faults\0"
    "branch-instructions\0"
    "branch-misses\0"
    "branches\0"
    "bus-cycles\0"
    "cache-misses\0"
    "cache-references\0"
    "context-switches\0"
    "cpu-clock\0"
    "cpu-cycles\0"
    "cpu-migrations\0"
    "cs\0"
    "cycles\0"
    "emulation-faults\0"
    "faults\0"
    "idle-cycles-backend\0"
    "idle-cycles-frontend\0"
    "instructions\0"
    "major-faults\0"
    "migrations\0"
    "minor-faults\0"
    "page-faults\0"
    "stalled-cycles-backend\0"
    "stalled-cycles-frontend\0"
    "task-clock\0"
    "\0";

static const Events eventlist[] = {
    {   0, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_ALIGNMENT_FAULTS, QTest::AlignmentFaults },
    {  17, PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS, QTest::BranchInstructions },
    {  37, PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES, QTest::BranchMisses },
    {  51, PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS, QTest::BranchInstructions },
    {  60, PERF_TYPE_HARDWARE, PERF_COUNT_HW_BUS_CYCLES, QTest::BusCycles },
    {  71, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES, QTest::CacheMisses },
    {  84, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES, QTest::CacheReferences },
    { 101, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES, QTest::ContextSwitches },
    { 118, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_CLOCK, QTest::WalltimeMilliseconds },
    { 128, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, QTest::CPUCycles },
    { 139, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS, QTest::CPUMigrations },
    { 154, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CONTEXT_SWITCHES, QTest::ContextSwitches },
    { 157, PERF_TYPE_HARDWARE, PERF_COUNT_HW_CPU_CYCLES, QTest::CPUCycles },
    { 164, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_EMULATION_FAULTS, QTest::EmulationFaults },
    { 181, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS, QTest::PageFaults },
    { 188, PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND, QTest::StalledCycles },
    { 208, PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND, QTest::StalledCycles },
    { 229, PERF_TYPE_HARDWARE, PERF_COUNT_HW_INSTRUCTIONS, QTest::Instructions },
    { 242, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MAJ, QTest::MajorPageFaults },
    { 255, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_CPU_MIGRATIONS, QTest::CPUMigrations },
    { 266, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS_MIN, QTest::MinorPageFaults },
    { 279, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_PAGE_FAULTS, QTest::PageFaults },
    { 291, PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_BACKEND, QTest::StalledCycles },
    { 314, PERF_TYPE_HARDWARE, PERF_COUNT_HW_STALLED_CYCLES_FRONTEND, QTest::StalledCycles },
    { 338, PERF_TYPE_SOFTWARE, PERF_COUNT_SW_TASK_CLOCK, QTest::WalltimeMilliseconds },
    {   0, PERF_TYPE_MAX, 0, QTest::Events }
};
/* -- END GENERATED CODE -- */

QTest::QBenchmarkMetric QBenchmarkPerfEventsMeasurer::metricForEvent(quint32 type, quint64 event_id)
{
    const Events *ptr = eventlist;
    for ( ; ptr->type != PERF_TYPE_MAX; ++ptr) {
        if (ptr->type == type && ptr->event_id == event_id)
            return ptr->metric;
    }
    return QTest::Events;
}

void QBenchmarkPerfEventsMeasurer::setCounter(const char *name)
{
    const Events *ptr = eventlist;
    for ( ; ptr->type != PERF_TYPE_MAX; ++ptr) {
        int c = strcmp(name, eventlist_strings + ptr->offset);
        if (c == 0)
            break;
        if (c < 0) {
            fprintf(stderr, "ERROR: Performance counter type '%s' is unknown\n", name);
            exit(1);
        }
    }

    ::event_type = ptr->type;
    ::event_id = ptr->event_id;
}

void QBenchmarkPerfEventsMeasurer::listCounters()
{
    if (!isAvailable()) {
        printf("Performance counters are not available on this system\n");
        return;
    }

    printf("The following performance counters are available:\n");
    const Events *ptr = eventlist;
    for ( ; ptr->type != PERF_TYPE_MAX; ++ptr) {
        printf("  %-30s [%s]\n", eventlist_strings + ptr->offset,
               ptr->type == PERF_TYPE_HARDWARE ? "hardware" :
               ptr->type == PERF_TYPE_SOFTWARE ? "software" :
               ptr->type == PERF_TYPE_HW_CACHE ? "cache" : "other");
    }
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
}

void QBenchmarkPerfEventsMeasurer::start()
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
    attr.type = ::event_type;
    attr.config = ::event_id;

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
    return metricForEvent(event_type, event_id);
}

static quint64 rawReadValue(int fd)
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

qint64 QBenchmarkPerfEventsMeasurer::readValue()
{
    quint64 raw = rawReadValue(fd);
    if (metricType() == QTest::WalltimeMilliseconds) {
        // perf returns nanoseconds
        return raw / 1000000;
    }
    return raw;
}

QT_END_NAMESPACE

#endif
