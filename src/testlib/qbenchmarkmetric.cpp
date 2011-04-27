/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
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
    default:
        return "";
    }
}

