// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/private/qcore_mac_p.h>

#include <Foundation/Foundation.h>

class tst_QMacAutoreleasePool : public QObject
{
    Q_OBJECT
private slots:
    void noPool();
    void rootLevelPool();
    void stackAllocatedPool();
};

static id lastDeallocedObject = nil;

@interface DeallocTracker : NSObject @end
@implementation DeallocTracker
-(void)dealloc
{
    lastDeallocedObject = self;
    [super dealloc];
}
@end

void tst_QMacAutoreleasePool::noPool()
{
    // No pool, will not be released, but should not crash

    [[[DeallocTracker alloc] init] autorelease];
}

void tst_QMacAutoreleasePool::rootLevelPool()
{
    // The root level case, no NSAutoreleasePool since we're not in the main
    // runloop, and objects autoreleased as part of main.

    NSObject *allocedObject = nil;
    {
        QMacAutoReleasePool qtPool;
        allocedObject = [[[DeallocTracker alloc] init] autorelease];
    }
    QCOMPARE(lastDeallocedObject, allocedObject);
}

void tst_QMacAutoreleasePool::stackAllocatedPool()
{
    // The normal case, other pools surrounding our pool, draining
    // our pool before any other pool.

    NSObject *allocedObject = nil;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    {
        QMacAutoReleasePool qtPool;
        allocedObject = [[[DeallocTracker alloc] init] autorelease];
    }
    QCOMPARE(lastDeallocedObject, allocedObject);
    [pool drain];
}

QTEST_APPLESS_MAIN(tst_QMacAutoreleasePool)

#include "tst_qmacautoreleasepool.moc"
