// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QTimeZone>
#include <QTest>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

void tst_QTimeZone_darwinTypes()
{
#if QT_CONFIG(timezone)
    // QTimeZone <-> CFTimeZone
    {
        QTimeZone qtTimeZone("America/Los_Angeles");
        const CFTimeZoneRef cfTimeZone = qtTimeZone.toCFTimeZone();
        QCOMPARE(QTimeZone::fromCFTimeZone(cfTimeZone), qtTimeZone);
        CFRelease(cfTimeZone);
    }
    {
        CFTimeZoneRef cfTimeZone = CFTimeZoneCreateWithName(kCFAllocatorDefault,
            CFSTR("America/Los_Angeles"), false);
        const QTimeZone qtTimeZone = QTimeZone::fromCFTimeZone(cfTimeZone);
        QVERIFY(CFEqual(qtTimeZone.toCFTimeZone(), cfTimeZone));
        CFRelease(cfTimeZone);
    }
    // QTimeZone <-> NSTimeZone
    {
        NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
        QTimeZone qtTimeZone("America/Los_Angeles");
        const NSTimeZone *nsTimeZone = qtTimeZone.toNSTimeZone();
        QCOMPARE(QTimeZone::fromNSTimeZone(nsTimeZone), qtTimeZone);
        [autoreleasepool release];
    }
    {
        NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
        NSTimeZone *nsTimeZone = [NSTimeZone timeZoneWithName:@"America/Los_Angeles"];
        const QTimeZone qtTimeZone = QTimeZone::fromNSTimeZone(nsTimeZone);
        QVERIFY([qtTimeZone.toNSTimeZone() isEqual:nsTimeZone]);
        [autoreleasepool release];
    }
#endif // feature timezone
}
