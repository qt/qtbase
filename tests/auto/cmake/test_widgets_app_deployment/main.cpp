// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QtTest>
#include <QMainWindow>

class test_widgets_app_deployment : public QObject
{
    Q_OBJECT
private slots:
    void canRun();
};

void test_widgets_app_deployment::canRun()
{
    QMainWindow mw;
    mw.show();
    QVERIFY(QTest::qWaitForWindowActive(&mw));
}

QTEST_MAIN(test_widgets_app_deployment)

#include "main.moc"
