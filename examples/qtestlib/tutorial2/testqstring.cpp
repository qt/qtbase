// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QTest>

//! [0]
class TestQString: public QObject
{
    Q_OBJECT

private slots:
    void toUpper_data();
    void toUpper();
};
//! [0]

//! [1]
void TestQString::toUpper_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<QString>("result");

    QTest::newRow("all-lower") << "hello" << "HELLO";
    QTest::newRow("mixed")     << "Hello" << "HELLO";
    QTest::newRow("all-upper") << "HELLO" << "HELLO";
}
//! [1]

//! [2]
void TestQString::toUpper()
{
    QFETCH(QString, string);
    QFETCH(QString, result);

    QCOMPARE(string.toUpper(), result);
}
//! [2]

//! [3]
QTEST_MAIN(TestQString)
#include "testqstring.moc"
//! [3]

