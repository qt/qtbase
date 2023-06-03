// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QTest>

#ifdef Q_OS_UNIX
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/qsystemdetection.h>

#include <time.h>
#endif

using namespace std::chrono_literals;

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
    QCOMPARE_GE(t.durationElapsed(), 90ms);

    QTest::qSleep(1000);
    QCOMPARE_GE(t.durationElapsed(), 1s);

    QTest::qSleep(1000 * 10); // 10 seconds
    QCOMPARE_GE(t.durationElapsed(), 10s);
}

void tst_Sleep::wait()
{
    QElapsedTimer t;
    t.start();

    t.start();
    QTest::qWait(1);
    QCOMPARE_GE(t.durationElapsed(), 1ms);

    QTest::qWait(10);
    QCOMPARE_GE(t.durationElapsed(), 11ms);

    QTest::qWait(100);
    QCOMPARE_GE(t.durationElapsed(), 111ms);

    QTest::qWait(1000);
    QCOMPARE_GE(t.durationElapsed(), 1111ms);
}

QTEST_MAIN(tst_Sleep)

#include "tst_sleep.moc"
