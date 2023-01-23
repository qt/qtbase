// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QString>
#include <QTest>

#include <QtCore/private/qcore_mac_p.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

using namespace Qt::StringLiterals;

void tst_QString_macTypes()
{
    const QLatin1StringView testString("test string");
    // QString <-> CFString
    {
        QString qtString = testString;
        const CFStringRef cfString = qtString.toCFString();
        QCOMPARE(QString::fromCFString(cfString), qtString);
        CFRelease(cfString);
    }
    {
        QString qtString = testString;
        const CFStringRef cfString = qtString.toCFString();
        QString qtStringCopy(qtString);
        qtString = qtString.toUpper(); // modify
        QCOMPARE(QString::fromCFString(cfString), qtStringCopy);
    }
    // QString <-> NSString
    {
        QMacAutoReleasePool pool;

        QString qtString = testString;
        const NSString *nsString = qtString.toNSString();
        QCOMPARE(QString::fromNSString(nsString), qtString);
    }
    {
        QMacAutoReleasePool pool;

        QString qtString = testString;
        const NSString *nsString = qtString.toNSString();
        QString qtStringCopy(qtString);
        qtString = qtString.toUpper(); // modify
        QCOMPARE(QString::fromNSString(nsString), qtStringCopy);
    }
}
