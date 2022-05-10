// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QString>
#include <QTest>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

void tst_QString_macTypes()
{
    // QString <-> CFString
    {
        QString qtString("test string");
        const CFStringRef cfString = qtString.toCFString();
        QCOMPARE(QString::fromCFString(cfString), qtString);
        CFRelease(cfString);
    }
    {
        QString qtString("test string");
        const CFStringRef cfString = qtString.toCFString();
        QString qtStringCopy(qtString);
        qtString = qtString.toUpper(); // modify
        QCOMPARE(QString::fromCFString(cfString), qtStringCopy);
    }
    // QString <-> NSString
    {
        QMacAutoReleasePool pool;

        QString qtString("test string");
        const NSString *nsString = qtString.toNSString();
        QCOMPARE(QString::fromNSString(nsString), qtString);
    }
    {
        QMacAutoReleasePool pool;

        QString qtString("test string");
        const NSString *nsString = qtString.toNSString();
        QString qtStringCopy(qtString);
        qtString = qtString.toUpper(); // modify
        QCOMPARE(QString::fromNSString(nsString), qtStringCopy);
    }
}
