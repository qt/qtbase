/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#include <QtTest/private/qbenchmarkmetric_p.h>

/*!
  \enum QTest::QBenchmarkMetric
  \since 4.7

  This enum lists all the things that can be benchmarked.

  \value FramesPerSecond        Frames per second
  \value BitsPerSecond          Bits per second
  \value BytesPerSecond         Bytes per second
  \value WalltimeMilliseconds   Clock time in milliseconds
  \value CPUTicks               CPU time
  \value InstructionReads       Instruction reads
  \value Events                 Event count
  \value WalltimeNanoseconds    Clock time in nanoseconds
  \value BytesAllocated         Memory usage in bytes

  Note that \c WalltimeNanoseconds and \c BytesAllocated are
  only provided for use via \l setBenchmarkResult(), and results
  in those metrics are not able to be provided automatically
  by the QTest framework.

  \sa QTest::benchmarkMetricName(), QTest::benchmarkMetricUnit()

 */

/*!
  \relates QTest
  \since 4.7
  Returns the enum value \a metric as a character string.
 */
const char * QTest::benchmarkMetricName(QBenchmarkMetric metric)
{
    switch (metric) {
    case FramesPerSecond:
        return "FramesPerSecond";
    case BitsPerSecond:
        return "BitsPerSecond";
    case BytesPerSecond:
        return "BytesPerSecond";
    case WalltimeMilliseconds:
        return "WalltimeMilliseconds";
    case CPUTicks:
        return "CPUTicks";
    case InstructionReads:
        return "InstructionReads";
    case Events:
        return "Events";
    case WalltimeNanoseconds:
        return "WalltimeNanoseconds";
    case BytesAllocated:
        return "BytesAllocated";
    default:
        return "";
    }
};

/*!
  \relates QTest
  \since 4.7
  Retuns the units of measure for the specified \a metric.
 */
const char * QTest::benchmarkMetricUnit(QBenchmarkMetric metric)
{
    switch (metric) {
    case FramesPerSecond:
        return "fps";
    case BitsPerSecond:
        return "bits/s";
    case BytesPerSecond:
        return "bytes/s";
    case WalltimeMilliseconds:
        return "msecs";
    case CPUTicks:
        return "CPU ticks";
    case InstructionReads:
        return "instruction reads";
    case Events:
        return "events";
    case WalltimeNanoseconds:
        return "nsecs";
    case BytesAllocated:
        return "bytes";
    default:
        return "";
    }
}

