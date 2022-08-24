// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    void heapAllocatedPool();
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

void tst_QMacAutoreleasePool::heapAllocatedPool()
{
    // The special case, a pool allocated on the heap, or as a member of a
    // heap allocated object. This is not a supported use of QMacAutoReleasePool,
    // and will result in warnings if the pool is prematurely drained.

    NSObject *allocedObject = nil;
    {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        QMacAutoReleasePool *qtPool = nullptr;
        {
            qtPool = new QMacAutoReleasePool;
            allocedObject = [[[DeallocTracker alloc] init] autorelease];
        }
        [pool drain];
        delete qtPool;
    }
    QCOMPARE(lastDeallocedObject, allocedObject);
}

QTEST_APPLESS_MAIN(tst_QMacAutoreleasePool)

#include "tst_qmacautoreleasepool.moc"
