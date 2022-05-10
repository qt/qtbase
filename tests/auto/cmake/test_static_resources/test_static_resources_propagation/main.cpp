// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTest/QtTest>
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
