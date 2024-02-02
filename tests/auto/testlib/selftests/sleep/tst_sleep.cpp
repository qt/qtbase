// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


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
    // Subtracting 10ms as a margin for error
    static constexpr auto MarginForError = 10ms;

    QElapsedTimer t;
    t.start();

    // Test qSleep(int) overload, too
    QTest::qSleep(100);
    QCOMPARE_GT(t.durationElapsed(), 100ms - MarginForError);

    QTest::qSleep(1s);
    QCOMPARE_GT(t.durationElapsed(), 1s - MarginForError);

    QTest::qSleep(10s);
    QCOMPARE_GT(t.durationElapsed(), 10s - MarginForError);
}

void tst_Sleep::wait()
{
    QElapsedTimer t;
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
