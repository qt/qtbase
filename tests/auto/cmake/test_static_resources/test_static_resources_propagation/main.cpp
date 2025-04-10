// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtCore/qfile.h>
#include <QtCore/qobject.h>
#include <QtPlugin>

class TestStaticResourcePropagation : public QObject
{
    Q_OBJECT
private slots:
    void resourceFilesExist();
};

void TestStaticResourcePropagation::resourceFilesExist()
{
    bool result = QFile::exists(":/teststaticmodule1/testfile1.txt");
    QVERIFY(result);
}

QTEST_MAIN(TestStaticResourcePropagation)
#include "main.moc"
