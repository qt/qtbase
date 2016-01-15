/****************************************************************************
**
** Copyright (C) 2014 Samuel Gaist <samuel.gaist@edeltech.ch>
** Copyright (C) 2016 The Qt Company Ltd.
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
        QMacAutoReleasePool pool;
        QByteArray qtByteArray("test bytearray");
        const NSData *nsData = qtByteArray.toNSData();
        QCOMPARE(QByteArray::fromNSData(nsData), qtByteArray);
    }
    {
        QMacAutoReleasePool pool;
        QByteArray qtByteArray("test bytearray");
        const NSData *nsData = qtByteArray.toNSData();
        QByteArray qtByteArrayCopy(qtByteArray);
        qtByteArray = qtByteArray.toUpper(); // modify
        QCOMPARE(QByteArray::fromNSData(nsData), qtByteArrayCopy);
    }
    // QByteArray <-> NSData Raw
    {
        QMacAutoReleasePool pool;
        QByteArray qtByteArray("test bytearray");
        const NSData *nsData = qtByteArray.toRawNSData();
        QCOMPARE([nsData bytes], qtByteArray.constData());
    }
    {
        QMacAutoReleasePool pool;
        const char data[] = "nsdata test";
        const NSData *nsData = [NSData dataWithBytes:data length:sizeof(data)];
        QByteArray qtByteArray = QByteArray::fromRawNSData(nsData);
        QCOMPARE(qtByteArray.constData(), [nsData bytes]);
    }
}
