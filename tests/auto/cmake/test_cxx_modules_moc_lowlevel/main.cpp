// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>

#include "helper.h"

import Mod;
import HelperMod;

// Verifies moc actually generated a correct QMetaObject for each of the three supported
// module unit kinds (primary interface, interface partition, internal partition), not just
// that the generated code happens to compile.
class TestModuleMoc : public QObject
{
    Q_OBJECT

private slots:
    void primaryInterfaceClassName();
    void interfacePartitionClassName();
    void signalDelivery();
    void internalPartitionObject();
};

void TestModuleMoc::primaryInterfaceClassName()
{
    QCOMPARE(PrimaryObject::staticMetaObject.className(), "PrimaryObject");
}

void TestModuleMoc::interfacePartitionClassName()
{
    QCOMPARE(PartObject::staticMetaObject.className(), "PartObject");
}

void TestModuleMoc::signalDelivery()
{
    // HeaderHelper and ModuleHelper aren't re-exported by Mod, so even though PrimaryObject
    // and PartObject are visible here via "import Mod;", we still need to make these
    // parameter types visible ourselves (via #include / import) in order to name them.
    PrimaryObject primary;
    int primaryReceivedCount = 0;
    QObject::connect(&primary, &PrimaryObject::primarySignal,
                      [&primaryReceivedCount] { ++primaryReceivedCount; });
    emit primary.primarySignal(HeaderHelper{42});
    QCOMPARE(primaryReceivedCount, 1);

    PartObject part;
    int partReceivedCount = 0;
    QObject::connect(&part, &PartObject::partSignal,
                      [&partReceivedCount] { ++partReceivedCount; });
    emit part.partSignal(ModuleHelper{43});
    QCOMPARE(partReceivedCount, 1);
}

void TestModuleMoc::internalPartitionObject()
{
    // InternalObject itself isn't reachable from here (it's in a non-exported partition), so
    // its own signal/slot machinery is verified indirectly via this exported helper instead.
    QVERIFY(testInternalObject());
}

QTEST_APPLESS_MAIN(TestModuleMoc)
#include "main.moc"
