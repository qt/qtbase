// Copyright (C) 2016 Stephen Kelly <steveire@gmail,com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QCoreApplication>
#include <QtTest/QTest>

class TestClass : public QObject
{
  Q_OBJECT
public:
  TestClass(QObject* parent = nullptr) {}

private slots:
  void doTest();
};

void TestClass::doTest()
{
    QFile f(QFINDTESTDATA("testdata.txt"));
    QVERIFY(f.open(QFile::ReadOnly));
    QCOMPARE(f.readAll().trimmed(), QByteArrayLiteral("This is a test."));
}

QTEST_MAIN(TestClass)
#include "main.moc"
