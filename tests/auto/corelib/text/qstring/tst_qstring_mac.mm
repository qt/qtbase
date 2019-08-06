/****************************************************************************
**
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

#include <QtCore/QString>
#include <QtTest/QtTest>

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
