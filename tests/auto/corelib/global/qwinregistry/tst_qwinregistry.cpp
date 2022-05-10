// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include <QtCore/qoperatingsystemversion.h>

#include <QtCore/private/qwinregistry_p.h>

class tst_QWinRegistry : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();

private Q_SLOTS:
    void values();
};

void tst_QWinRegistry::initTestCase()
{
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows10)
        QSKIP("This test requires registry values present in Windows 10");
}

void tst_QWinRegistry::values()
{
    QWinRegistryKey key(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)");
    QVERIFY(key.isValid());
    QVERIFY(!key.stringValue(L"ProductName").isEmpty());
    QVERIFY(key.stringValue(L"NonExistingKey").isEmpty());
    auto majorVersion = key.dwordValue(L"CurrentMajorVersionNumber");
    QVERIFY(majorVersion.second);
    QVERIFY(majorVersion.first > 0);
    auto nonExistingValue = key.dwordValue(L"NonExistingKey");
    QVERIFY(!nonExistingValue.second);
    QCOMPARE(nonExistingValue.first, 0u);
}

QTEST_APPLESS_MAIN(tst_QWinRegistry);

#include "tst_qwinregistry.moc"
