/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QtTest/QtTest>

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
