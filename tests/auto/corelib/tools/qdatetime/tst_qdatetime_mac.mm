/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2014 Petroules Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QDateTime>
#include <QtTest/QtTest>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

void tst_QDateTime_macTypes()
{
    // QDateTime <-> CFDate

    static const int kMsPerSecond = 1000;

    for (int i = 0; i < kMsPerSecond; ++i) {
        QDateTime qtDateTime = QDateTime::fromMSecsSinceEpoch(i);
        const CFDateRef cfDate = qtDateTime.toCFDate();
        QCOMPARE(QDateTime::fromCFDate(cfDate), qtDateTime);
        CFRelease(cfDate);
    }
    {
        QDateTime qtDateTime = QDateTime::fromMSecsSinceEpoch(0);
        const CFDateRef cfDate = qtDateTime.toCFDate();
        QDateTime qtDateTimeCopy(qtDateTime);
        qtDateTime.setTime_t(10000); // modify
        QCOMPARE(QDateTime::fromCFDate(cfDate), qtDateTimeCopy);
    }
    // QDateTime <-> NSDate
    for (int i = 0; i < kMsPerSecond; ++i) {
        QMacAutoReleasePool pool;
        QDateTime qtDateTime = QDateTime::fromMSecsSinceEpoch(i);
        const NSDate *nsDate = qtDateTime.toNSDate();
        QCOMPARE(QDateTime::fromNSDate(nsDate), qtDateTime);
    }
    {
        QMacAutoReleasePool pool;
        QDateTime qtDateTime = QDateTime::fromMSecsSinceEpoch(0);
        const NSDate *nsDate = qtDateTime.toNSDate();
        QDateTime qtDateTimeCopy(qtDateTime);
        qtDateTime.setTime_t(10000); // modify
        QCOMPARE(QDateTime::fromNSDate(nsDate), qtDateTimeCopy);
    }
}
