// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QTest>

class tst_Sleep: public QObject
{
    Q_OBJECT

private slots:
    void sleep();
    void wait();
};

void tst_Sleep::sleep()
{
    QElapsedTimer t;
    t.start();

    QTest::qSleep(100);
    QVERIFY(t.elapsed() > 90);

    QTest::qSleep(1000);
    QVERIFY(t.elapsed() > 1000);

    QTest::qSleep(1000 * 10); // 10 seconds
    QVERIFY(t.elapsed() > 1000 * 10);
}

void tst_Sleep::wait()
{
    QElapsedTimer t;
    t.start();

    QTest::qWait(1);
    QVERIFY(t.elapsed() >= 1);

    QTest::qWait(10);
    QVERIFY(t.elapsed() >= 11);

    QTest::qWait(100);
    QVERIFY(t.elapsed() >= 111);

    QTest::qWait(1000);
    QVERIFY(t.elapsed() >= 1111);
}

QTEST_MAIN(tst_Sleep)

#include "tst_sleep.moc"
