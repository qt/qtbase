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

#include <QtCore/QUuid>
#include <QtTest/QtTest>

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

void tst_QUuid_darwinTypes()
{
    // QUuid <-> CFUUID
    {
        const auto qtUuid = QUuid::fromString(QLatin1String("0f7169cc-5711-4af9-99d9-fecb2329fdef"));
        const CFUUIDRef cfuuid = qtUuid.toCFUUID();
        QCOMPARE(QUuid::fromCFUUID(cfuuid), qtUuid);
        CFStringRef cfstring = CFUUIDCreateString(0, cfuuid);
        QCOMPARE(QString::fromCFString(cfstring), qtUuid.toString().mid(1, 36).toUpper());
        CFRelease(cfstring);
        CFRelease(cfuuid);
    }
    {
        auto qtUuid = QUuid::fromString(QLatin1String("0f7169cc-5711-4af9-99d9-fecb2329fdef"));
        const CFUUIDRef cfuuid = qtUuid.toCFUUID();
        QUuid qtUuidCopy(qtUuid);
        qtUuid = QUuid::fromString(QLatin1String("93eec131-13c5-4d13-aaea-e456b4c57efa")); // modify
        QCOMPARE(QUuid::fromCFUUID(cfuuid), qtUuidCopy);
        CFStringRef cfstring = CFUUIDCreateString(0, cfuuid);
        QCOMPARE(QString::fromCFString(cfstring), qtUuidCopy.toString().mid(1, 36).toUpper());
        CFRelease(cfstring);
        CFRelease(cfuuid);
    }
    // QUuid <-> NSUUID
    {
        QMacAutoReleasePool pool;

        const auto qtUuid = QUuid::fromString(QLatin1String("0f7169cc-5711-4af9-99d9-fecb2329fdef"));
        const NSUUID *nsuuid = qtUuid.toNSUUID();
        QCOMPARE(QUuid::fromNSUUID(nsuuid), qtUuid);
        QCOMPARE(QString::fromNSString([nsuuid UUIDString]), qtUuid.toString().mid(1, 36).toUpper());
    }
    {
        QMacAutoReleasePool pool;

        auto qtUuid = QUuid::fromString(QLatin1String("0f7169cc-5711-4af9-99d9-fecb2329fdef"));
        const NSUUID *nsuuid = qtUuid.toNSUUID();
        QUuid qtUuidCopy(qtUuid);
        qtUuid = QUuid::fromString(QLatin1String("93eec131-13c5-4d13-aaea-e456b4c57efa")); // modify
        QCOMPARE(QUuid::fromNSUUID(nsuuid), qtUuidCopy);
        QCOMPARE(QString::fromNSString([nsuuid UUIDString]), qtUuidCopy.toString().mid(1, 36).toUpper());
    }
}
