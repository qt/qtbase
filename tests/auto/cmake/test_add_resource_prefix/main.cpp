// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qfile.h>

class TestAddResourcePrefix : public QObject
{
    Q_OBJECT
private slots:
    void resourceInDefaultPathExists();
    void resourceInGivenPathExists();
    void xmlEscaping();
};

void TestAddResourcePrefix::resourceInDefaultPathExists()
{
    QVERIFY(QFile::exists(":/resource_file.txt"));
}

void TestAddResourcePrefix::resourceInGivenPathExists()
{
    QVERIFY(QFile::exists(":/resources/resource_file.txt"));
}


void TestAddResourcePrefix::xmlEscaping()
{
    QVERIFY(QFile::exists(":/&\"'<>/&\"'<>.alias"));
}

QTEST_MAIN(TestAddResourcePrefix)
#include "main.moc"
