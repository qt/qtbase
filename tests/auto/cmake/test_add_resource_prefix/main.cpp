// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QFile>
#include <QtTest>

class TestAddResourcePrefix : public QObject
{
    Q_OBJECT
private slots:
    void resourceInDefaultPathExists();
    void resourceInGivenPathExists();
};

void TestAddResourcePrefix::resourceInDefaultPathExists()
{
    QVERIFY(QFile::exists(":/resource_file.txt"));
}

void TestAddResourcePrefix::resourceInGivenPathExists()
{
    QVERIFY(QFile::exists(":/resources/resource_file.txt"));
}

QTEST_MAIN(TestAddResourcePrefix)
#include "main.moc"
