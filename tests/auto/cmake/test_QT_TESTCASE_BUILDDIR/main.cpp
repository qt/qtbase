// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>
#include <QtTest/QTest>
#include <QDebug>

class TestClass : public QObject
{
    Q_OBJECT
public:
    TestClass(QObject *parent = nullptr) { }

private slots:
    void doTest();
};

void TestClass::doTest()
{
    QFile fsrc(QFINDTESTDATA("data/testdata.txt"));
    QVERIFY(fsrc.open(QFile::ReadOnly));
    QCOMPARE(fsrc.readAll().trimmed(),
             QByteArrayLiteral("This is the test data found in QT_TESTCASE_SOURCEDIR."));

    QFile fbuild(QFINDTESTDATA("level2/testdata_build.txt"));
    QVERIFY(fbuild.open(QFile::ReadOnly));
    QCOMPARE(fbuild.readAll().trimmed(),
             QByteArrayLiteral("This is the test data found in custom QT_TESTCASE_BUILDDIR."));
}

QTEST_MAIN(TestClass)
#include "main.moc"
