// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2014 Petroules Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QDateTime>
#include <QTest>

#include <QtCore/private/qcore_mac_p.h>

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
        qtDateTime.setSecsSinceEpoch(10000); // modify
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
        qtDateTime.setSecsSinceEpoch(10000); // modify
        QCOMPARE(QDateTime::fromNSDate(nsDate), qtDateTimeCopy);
    }
}
