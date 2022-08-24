// Copyright (C) 2014 Samuel Gaist <samuel.gaist@edeltech.ch>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QByteArray>
#include <QTest>

#include <QtCore/private/qcore_mac_p.h>

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
