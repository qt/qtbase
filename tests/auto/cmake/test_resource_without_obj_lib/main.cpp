// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qtresource.h>
#include <QtTest/QTest>

class TestManualResourceInit : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void resourceExistsAfterManualInit();
};

void TestManualResourceInit::initTestCase()
{
    // Manually initialize the resource like we used to do it in qt5 + qmake times.
    Q_INIT_RESOURCE(helper_res);
}

void TestManualResourceInit::resourceExistsAfterManualInit()
{
    QVERIFY(QFile::exists(":/resource.txt"));
}

QTEST_MAIN(TestManualResourceInit)
#include "main.moc"

