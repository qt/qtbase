// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
#include <QTest>

class TestQString: public QObject
{
    Q_OBJECT
private slots:
    void toUpper();
};
//! [0]

//! [1]
void TestQString::toUpper()
{
    QString str = "Hello";
    QCOMPARE(str.toUpper(), QString("HELLO"));
}
//! [1]

//! [2]
QTEST_MAIN(TestQString)
#include "testqstring.moc"
//! [2]

