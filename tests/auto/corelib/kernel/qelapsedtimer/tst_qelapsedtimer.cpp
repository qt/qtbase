// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QDateTime>
#include <QtCore/QString>
#include <QtCore/QElapsedTimer>
#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>
#include <QTimer>

static const int minResolution = 100; // the minimum resolution for the tests

QT_BEGIN_NAMESPACE
QDebug operator<<(QDebug s, const QElapsedTimer &t)
{
    s.nospace() << "(" << t.msecsSinceReference() << ")";
    return s.space();
}
QT_END_NAMESPACE

class tst_QElapsedTimer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void compareCompiles();
    void statics();
    void validity();
    void basics();
    void elapsed();
    void msecsTo();
};

void tst_QElapsedTimer::compareCompiles()
{
    QTestPrivate::testAllComparisonOperatorsCompile<QElapsedTimer>();
}

void tst_QElapsedTimer::statics()
{
    // these have been required since Qt 6.6
    QCOMPARE(QElapsedTimer::clockType(), QElapsedTimer::MonotonicClock);
    QVERIFY(QElapsedTimer::isMonotonic());

    QElapsedTimer t;
    t.start();
    qint64 system_now = QDateTime::currentMSecsSinceEpoch();

    auto setprecision = +[](QTextStream &s) -> QTextStream & {
        s.setRealNumberNotation(QTextStream::FixedNotation);
        s.setRealNumberPrecision(3);
        return s;
    };
    qDebug() << setprecision
             << "Current monotonic time is" << (t.msecsSinceReference() / 1000.)
             << "s and current system time is" << (system_now / 1000.) << 's';
    if (qAbs(system_now - t.msecsSinceReference()) < 5 * minResolution)
        qWarning() << "The monotonic clock is awfully close to the system clock"
                      " (it may not be monotonic at all!)";
}

void tst_QElapsedTimer::validity()
{
    QElapsedTimer t;

    QVERIFY(!t.isValid()); // non-POD now, it should always start invalid

    t.start();
    QVERIFY(t.isValid());

    t.invalidate();
    QVERIFY(!t.isValid());
}

void tst_QElapsedTimer::basics()
{
    QElapsedTimer t1;
    t1.start();

    QVERIFY(t1.msecsSinceReference() != 0);

    QCOMPARE(t1, t1);
    QVERIFY(!(t1 != t1));
    QVERIFY(!(t1 < t1));
    QCOMPARE(t1.msecsTo(t1), qint64(0));
    QCOMPARE(t1.secsTo(t1), qint64(0));
    QT_TEST_ALL_COMPARISON_OPS(t1, t1, Qt::strong_ordering::equal);

    quint64 value1 = t1.msecsSinceReference();
    qDebug() << "value1:" << value1 << "t1:" << t1;
    qint64 nsecs = t1.nsecsElapsed();
    qint64 elapsed = t1.restart();
    QVERIFY(elapsed < minResolution);
    QVERIFY(nsecs / 1000000 < minResolution);

    quint64 value2 = t1.msecsSinceReference();
    qDebug() << "value2:" << value2 << "t1:" << t1
             << "elapsed:" << elapsed << "nsecs:" << nsecs;
    // in theory, elapsed == value2 - value1

    // However, since QElapsedTimer keeps internally the full resolution,
    // we have here a rounding error due to integer division
    QVERIFY(qAbs(elapsed - qint64(value2 - value1)) <= 1);
}

void tst_QElapsedTimer::elapsed()
{
    qint64 nsecs = 0;
    qint64 msecs = 0;
    bool expired1 = false;
    bool expired8 = false;
    bool expiredInf = false;
    qint64 elapsed = 0;
    bool timerExecuted = false;

    QElapsedTimer t1;
    t1.start();

    QTimer::singleShot(2 * minResolution, Qt::PreciseTimer, [&](){
        nsecs = t1.nsecsElapsed();
        msecs = t1.elapsed();
        expired1 = t1.hasExpired(minResolution);
        expired8 = t1.hasExpired(8 * minResolution);
        expiredInf = t1.hasExpired(-1);
        elapsed = t1.restart();
        timerExecuted = true;
    });

    QTRY_VERIFY2_WITH_TIMEOUT(timerExecuted,
        "Looks like timer didn't fire on time.", 4 * minResolution);

    QVERIFY(nsecs > 0);
    QVERIFY(msecs > 0);
    // the number of elapsed nanoseconds and milliseconds should match
    QVERIFY(nsecs - msecs * 1000000 < 1000000);

    QVERIFY(expired1);
    QVERIFY(!expired8);
    QVERIFY(!expiredInf);

    QVERIFY(elapsed >= msecs);
    QVERIFY(elapsed < msecs + 3 * minResolution);
}

void tst_QElapsedTimer::msecsTo()
{
    QElapsedTimer t1;
    t1.start();
    QTest::qSleep(minResolution);
    QElapsedTimer t2;
    t2.start();
    QTest::qSleep(minResolution);
    QElapsedTimer t3;
    t3.start();

    QT_TEST_EQUALITY_OPS(t1, t2, false);
    QT_TEST_EQUALITY_OPS(QElapsedTimer(), QElapsedTimer(), true);
    QT_TEST_EQUALITY_OPS(QElapsedTimer(), t2, false);
    QT_TEST_ALL_COMPARISON_OPS(t1, t2, Qt::strong_ordering::less);
    QT_TEST_ALL_COMPARISON_OPS(t3, t2, Qt::strong_ordering::greater);
    QT_TEST_ALL_COMPARISON_OPS(t3, QElapsedTimer(), Qt::strong_ordering::greater);

    auto diff = t1.msecsTo(t2);
    QVERIFY2(diff > 0, QString("difference t1 and t2 is %1").arg(diff).toLatin1());
    diff = t2.msecsTo(t1);
    QVERIFY2(diff < 0, QString("difference t2 and t1 is %1").arg(diff).toLatin1());
}

QTEST_MAIN(tst_QElapsedTimer);

#include "tst_qelapsedtimer.moc"
