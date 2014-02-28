/****************************************************************************
**
** Copyright (C) 2014 Samuel Gaist <samuel.gaist@edeltech.ch>
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtCore/QByteArray>
#include <QtTest/QtTest>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

void tst_QByteArray_macTypes()
{
    // QByteArray <-> CFData
    {
        QByteArray qtByteArray("test bytearray");
        const CFDataRef cfData = qtByteArray.toCFData();
        QCOMPARE(QByteArray::fromCFData(cfData), qtByteArray);
        CFRelease(cfData);
    }
    {
        QByteArray qtByteArray("test bytearray");
        const CFDataRef cfData = qtByteArray.toCFData();
        QByteArray qtByteArrayCopy(qtByteArray);
        qtByteArray = qtByteArray.toUpper(); // modify
        QCOMPARE(QByteArray::fromCFData(cfData), qtByteArrayCopy);
    }
    // QByteArray <-> CFData Raw
    {
        QByteArray qtByteArray("test bytearray");
        const CFDataRef cfData = qtByteArray.toRawCFData();
        const UInt8 * cfDataPtr = CFDataGetBytePtr(cfData);
        QCOMPARE(reinterpret_cast<const UInt8*>(qtByteArray.constData()), cfDataPtr);
        CFRelease(cfData);
    }
    {
        const UInt8 data[] = "cfdata test";
        const CFDataRef cfData = CFDataCreate(kCFAllocatorDefault, data, sizeof(data));
        const UInt8 * cfDataPtr = CFDataGetBytePtr(cfData);
        QByteArray qtByteArray = QByteArray::fromRawCFData(cfData);
        QCOMPARE(reinterpret_cast<const UInt8*>(qtByteArray.constData()), cfDataPtr);
        CFRelease(cfData);
    }
    // QByteArray <-> NSData
    {
        NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
        QByteArray qtByteArray("test bytearray");
        const NSData *nsData = qtByteArray.toNSData();
        QCOMPARE(QByteArray::fromNSData(nsData), qtByteArray);
        [autoreleasepool release];
    }
    {
        NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
        QByteArray qtByteArray("test bytearray");
        const NSData *nsData = qtByteArray.toNSData();
        QByteArray qtByteArrayCopy(qtByteArray);
        qtByteArray = qtByteArray.toUpper(); // modify
        QCOMPARE(QByteArray::fromNSData(nsData), qtByteArrayCopy);
        [autoreleasepool release];
    }
    // QByteArray <-> NSData Raw
    {
        NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
        QByteArray qtByteArray("test bytearray");
        const NSData *nsData = qtByteArray.toRawNSData();
        QCOMPARE([nsData bytes], qtByteArray.constData());
        [autoreleasepool release];
    }
    {
        NSAutoreleasePool *autoreleasepool = [[NSAutoreleasePool alloc] init];
        const char data[] = "nsdata test";
        const NSData *nsData = [NSData dataWithBytes:data length:sizeof(data)];
        QByteArray qtByteArray = QByteArray::fromRawNSData(nsData);
        QCOMPARE(qtByteArray.constData(), [nsData bytes]);
        [autoreleasepool release];
    }
}
