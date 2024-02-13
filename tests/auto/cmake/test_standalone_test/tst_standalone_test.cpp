// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QWindow>

class tst_standalone_test : public QObject
{
    Q_OBJECT

private slots:
    void testLaunched()
    {
        QWindow w;
        w.show();
        QVERIFY(QTest::qWaitForWindowActive(&w));
    }
};

QTEST_MAIN(tst_standalone_test)

#include "tst_standalone_test.moc"
