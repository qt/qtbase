// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QUuid>
#include <QTest>

#include <QtCore/private/qcore_mac_p.h>

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
