/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QtTest/QtTest>

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
