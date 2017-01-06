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

#include <QtCore/QTimeZone>
#include <QtTest/QtTest>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

void tst_QTimeZone_darwinTypes()
{
#if !defined(QT_NO_SYSTEMLOCALE)
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
#endif
}
