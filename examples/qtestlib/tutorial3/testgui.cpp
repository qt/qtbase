// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
#include <QtWidgets>
#include <QTest>

class TestGui: public QObject
{
    Q_OBJECT

private slots:
    void testGui();

};
//! [0]

//! [1]
void TestGui::testGui()
{
    QLineEdit lineEdit;

    QTest::keyClicks(&lineEdit, "hello world");

    QCOMPARE(lineEdit.text(), QString("hello world"));
}
//! [1]

//! [2]
QTEST_MAIN(TestGui)
#include "testgui.moc"
//! [2]

