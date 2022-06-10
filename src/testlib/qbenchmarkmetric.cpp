// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/private/qbenchmarkmetric_p.h>

QT_BEGIN_NAMESPACE

namespace QTest {

struct QBenchmarkMetricKey {
    int metric;
    const char * name;
    const char * unit;
};

static const QBenchmarkMetricKey entries[] = {
    { FramesPerSecond, "FramesPerSecond", "fps" },
    { BitsPerSecond, "BitsPerSecond", "bits/s" },
    { BytesPerSecond, "BytesPerSecond", "bytes/s" },
    { WalltimeMilliseconds, "WalltimeMilliseconds", "msecs" },
    { CPUTicks, "CPUTicks", "CPU ticks" },
    { InstructionReads, "InstructionReads", "instruction reads" },
    { Events, "Events", "events" },
    { WalltimeNanoseconds, "WalltimeNanoseconds", "nsecs" },
    { BytesAllocated, "BytesAllocated", "bytes" },
    { CPUMigrations, "CPUMigrations", "CPU migrations" },
    { CPUCycles, "CPUCycles", "CPU cycles" },
    { BusCycles, "BusCycles", "bus cycles" },
    { StalledCycles, "StalledCycles", "stalled cycles" },
    { Instructions, "Instructions", "instructions" },
    { BranchInstructions, "BranchInstructions", "branch instructions" },
    { BranchMisses, "BranchMisses", "branch misses" },
    { CacheReferences, "CacheReferences", "cache references" },
    { CacheReads, "CacheReads", "cache loads" },
    { CacheWrites, "CacheWrites", "cache stores" },
    { CachePrefetches, "CachePrefetches", "cache prefetches" },
    { CacheMisses, "CacheMisses", "cache misses" },
    { CacheReadMisses, "CacheReadMisses", "cache load misses" },
    { CacheWriteMisses, "CacheWriteMisses", "cache store misses" },
    { CachePrefetchMisses, "CachePrefetchMisses", "cache prefetch misses" },
    { ContextSwitches, "ContextSwitches", "context switches" },
    { PageFaults, "PageFaults", "page faults" },
    { MinorPageFaults, "MinorPageFaults", "minor page faults" },
    { MajorPageFaults, "MajorPageFaults", "major page faults" },
    { AlignmentFaults, "AlignmentFaults", "alignment faults" },
    { EmulationFaults, "EmulationFaults", "emulation faults" },
    { RefCPUCycles, "RefCPUCycles", "Reference CPU cycles" },
};
static const int NumEntries = sizeof(entries) / sizeof(entries[0]);

}

/*!
  \enum QTest::QBenchmarkMetric
  \since 4.7

  This enum lists all the things that can be benchmarked.

  \value FramesPerSecond        Frames per second
  \value BitsPerSecond          Bits per second
  \value BytesPerSecond         Bytes per second
  \value WalltimeMilliseconds   Clock time in milliseconds
  \value WalltimeNanoseconds    Clock time in nanoseconds
  \value BytesAllocated         Memory usage in bytes
  \value Events                 Event count
  \value CPUTicks               CPU time
  \value CPUMigrations          Process migrations between CPUs
  \value CPUCycles              CPU cycles
  \value RefCPUCycles           Reference CPU cycles
  \value BusCycles              Bus cycles
  \value StalledCycles          Cycles stalled
  \value InstructionReads       Instruction reads
  \value Instructions           Instructions executed
  \value BranchInstructions     Branch-type instructions
  \value BranchMisses           Branch instructions that were mispredicted
  \value CacheReferences        Cache accesses of any type
  \value CacheMisses            Cache misses of any type
  \value CacheReads             Cache reads / loads
  \value CacheReadMisses        Cache read / load misses
  \value CacheWrites            Cache writes / stores
  \value CacheWriteMisses       Cache write / store misses
  \value CachePrefetches        Cache prefetches
  \value CachePrefetchMisses    Cache prefetch misses
  \value ContextSwitches        Context switches
  \value PageFaults             Page faults of any type
  \value MinorPageFaults        Minor page faults
  \value MajorPageFaults        Major page faults
  \value AlignmentFaults        Faults caused due to misalignment
  \value EmulationFaults        Faults that needed software emulation

  \sa QTest::benchmarkMetricName(), QTest::benchmarkMetricUnit()

  Note that \c WalltimeNanoseconds and \c BytesAllocated are
  only provided for use via \l setBenchmarkResult(), and results
  in those metrics are not able to be provided automatically
  by the QTest framework.
 */

/*!
  \since 4.7
  Returns the enum value \a metric as a character string.
 */
const char * QTest::benchmarkMetricName(QBenchmarkMetric metric)
{
    if (unsigned(metric) < unsigned(QTest::NumEntries))
        return entries[metric].name;

    return "";
}

/*!
  \since 4.7
  Returns the units of measure for the specified \a metric.
 */
const char * QTest::benchmarkMetricUnit(QBenchmarkMetric metric)
{
    if (unsigned(metric) < unsigned(QTest::NumEntries))
        return entries[metric].unit;

    return "";
}

QT_END_NAMESPACE
