// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "intermediate_lib.h"

#include <QtCore/qfile.h>
#include <QtTest/QtTest>

class TestAddResourcesBigResources : public QObject
{
    Q_OBJECT
private slots:
    void resourceInApplicationExists();
    void resourceInIntermediateLibExists();
    void resourceInLeafLibExists();
};

void TestAddResourcesBigResources::resourceInApplicationExists()
{
    QVERIFY(QFile::exists(":/resource1.txt"));
}

void TestAddResourcesBigResources::resourceInIntermediateLibExists()
{
    QVERIFY(intermediate_lib::isResourceAvailable());
}

void TestAddResourcesBigResources::resourceInLeafLibExists()
{
    QVERIFY(intermediate_lib::isLeafLibResourceAvailable());
}

QTEST_MAIN(TestAddResourcesBigResources)
#include "main.moc"

