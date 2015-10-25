/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
        QUuid qtUuid(QByteArrayLiteral("0f7169cc-5711-4af9-99d9-fecb2329fdef"));
        const CFUUIDRef cfuuid = qtUuid.toCFUUID();
        QCOMPARE(QUuid::fromCFUUID(cfuuid), qtUuid);
        CFStringRef cfstring = CFUUIDCreateString(0, cfuuid);
        QCOMPARE(QString::fromCFString(cfstring), qtUuid.toString().mid(1, 36).toUpper());
        CFRelease(cfstring);
        CFRelease(cfuuid);
    }
    {
        QUuid qtUuid(QByteArrayLiteral("0f7169cc-5711-4af9-99d9-fecb2329fdef"));
        const CFUUIDRef cfuuid = qtUuid.toCFUUID();
        QUuid qtUuidCopy(qtUuid);
        qtUuid = QUuid(QByteArrayLiteral("93eec131-13c5-4d13-aaea-e456b4c57efa")); // modify
        QCOMPARE(QUuid::fromCFUUID(cfuuid), qtUuidCopy);
        CFStringRef cfstring = CFUUIDCreateString(0, cfuuid);
        QCOMPARE(QString::fromCFString(cfstring), qtUuidCopy.toString().mid(1, 36).toUpper());
        CFRelease(cfstring);
        CFRelease(cfuuid);
    }
    // QUuid <-> NSUUID
    {
        QMacAutoReleasePool pool;

        QUuid qtUuid(QByteArrayLiteral("0f7169cc-5711-4af9-99d9-fecb2329fdef"));
        const NSUUID *nsuuid = qtUuid.toNSUUID();
        QCOMPARE(QUuid::fromNSUUID(nsuuid), qtUuid);
        QCOMPARE(QString::fromNSString([nsuuid UUIDString]), qtUuid.toString().mid(1, 36).toUpper());
    }
    {
        QMacAutoReleasePool pool;

        QUuid qtUuid(QByteArrayLiteral("0f7169cc-5711-4af9-99d9-fecb2329fdef"));
        const NSUUID *nsuuid = qtUuid.toNSUUID();
        QUuid qtUuidCopy(qtUuid);
        qtUuid = QUuid(QByteArrayLiteral("93eec131-13c5-4d13-aaea-e456b4c57efa")); // modify
        QCOMPARE(QUuid::fromNSUUID(nsuuid), qtUuidCopy);
        QCOMPARE(QString::fromNSString([nsuuid UUIDString]), qtUuidCopy.toString().mid(1, 36).toUpper());
    }
}
