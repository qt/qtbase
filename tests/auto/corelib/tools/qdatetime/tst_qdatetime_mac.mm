/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2014 Petroules Corporation.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    {
        QDateTime qtDateTime = QDateTime::fromMSecsSinceEpoch(0);
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
    {
        QMacAutoReleasePool pool;
        QDateTime qtDateTime = QDateTime::fromMSecsSinceEpoch(0);
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
