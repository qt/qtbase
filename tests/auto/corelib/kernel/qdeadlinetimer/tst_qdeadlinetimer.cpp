// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>
#include <QTimer>

#include <chrono>
#include <cmath>
#include <cstdio>

#include <inttypes.h>

static const int minResolution = 400; // the minimum resolution for the tests

QT_BEGIN_NAMESPACE
namespace QTest {
template<> char *toString(const QDeadlineTimer &dt)
{
    if (dt.isForever())
        return qstrdup("QDeadlineTimer::Forever");

    qint64 deadline = dt.deadlineNSecs();
    char *buf = new char[256];
    std::snprintf(buf, 256, "%lld.%09lld%s",
                  deadline / 1000 / 1000 / 1000,
                  std::abs(deadline) % (1000 * 1000 * 1000),
                  dt.hasExpired() ? " (expired)" : "");
    return buf;
}
}
QT_END_NAMESPACE

class tst_QDeadlineTimer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void compareCompiles();
    void basics();
    void foreverness();
    void current();
    void deadlines();
    void setDeadline();
    void overflow();
    void expire();
    void stdchrono();
};

static constexpr auto timerType = Qt::PreciseTimer;

void tst_QDeadlineTimer::compareCompiles()
{
    QTestPrivate::testAllComparisonOperatorsCompile<QDeadlineTimer>();
}

void tst_QDeadlineTimer::basics()
{
    QDeadlineTimer deadline;
    QCOMPARE(deadline.timerType(), Qt::CoarseTimer);

    deadline = QDeadlineTimer(timerType);
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline, QDeadlineTimer(timerType));
    QVERIFY(!(deadline != QDeadlineTimer(timerType)));
    QVERIFY(!(deadline < QDeadlineTimer()));
    QCOMPARE_LE(deadline, QDeadlineTimer());
    QCOMPARE_GE(deadline, QDeadlineTimer());
    QVERIFY(!(deadline > QDeadlineTimer()));
    QVERIFY(!(deadline < deadline));
    QCOMPARE_LE(deadline, deadline);
    QCOMPARE_GE(deadline, deadline);
    QVERIFY(!(deadline > deadline));
    QT_TEST_ALL_COMPARISON_OPS(deadline, QDeadlineTimer(timerType), Qt::strong_ordering::equal);
    QT_TEST_ALL_COMPARISON_OPS(deadline, QDeadlineTimer(), Qt::strong_ordering::equal);
    QT_TEST_ALL_COMPARISON_OPS(QDeadlineTimer(), QDeadlineTimer(), Qt::strong_ordering::equal);

    // should have expired, but we may be running too early after boot
    QTRY_VERIFY_WITH_TIMEOUT(deadline.hasExpired(), 100);

    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE(deadline.deadline(), qint64(0));
    QCOMPARE(deadline.deadlineNSecs(), qint64(0));

    deadline.setRemainingTime(0, timerType);
    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE_NE(deadline.deadline(), 0);
    QCOMPARE_NE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE_NE(deadline.deadlineNSecs(), 0);
    QCOMPARE_NE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline.setPreciseRemainingTime(0, 0, timerType);
    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE_NE(deadline.deadline(), 0);
    QCOMPARE_NE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE_NE(deadline.deadlineNSecs(), 0);
    QCOMPARE_NE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline.setDeadline(0, timerType);
    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE(deadline.deadline(), qint64(0));
    QCOMPARE(deadline.deadlineNSecs(), qint64(0));

    deadline.setPreciseDeadline(0, 0, timerType);
    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE(deadline.deadline(), qint64(0));
    QCOMPARE(deadline.deadlineNSecs(), qint64(0));
}

void tst_QDeadlineTimer::foreverness()
{
    QDeadlineTimer deadline = QDeadlineTimer::Forever;
    QCOMPARE(deadline.timerType(), Qt::CoarseTimer);
    QVERIFY(deadline.isForever());
    QVERIFY(!deadline.hasExpired());
    QCOMPARE(deadline.remainingTime(), qint64(-1));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(-1));
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline = QDeadlineTimer(-1, timerType);
    QVERIFY(deadline.isForever());
    QVERIFY(!deadline.hasExpired());
    QCOMPARE(deadline.remainingTime(), qint64(-1));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(-1));
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline.setRemainingTime(-1, timerType);
    QVERIFY(deadline.isForever());
    QVERIFY(!deadline.hasExpired());
    QCOMPARE(deadline.remainingTime(), qint64(-1));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(-1));
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline.setPreciseRemainingTime(-1, 0, timerType);
    QVERIFY(deadline.isForever());
    QVERIFY(!deadline.hasExpired());
    QCOMPARE(deadline.remainingTime(), qint64(-1));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(-1));
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline.setPreciseRemainingTime(-1, -1, timerType);
    QVERIFY(deadline.isForever());
    QVERIFY(!deadline.hasExpired());
    QCOMPARE(deadline.remainingTime(), qint64(-1));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(-1));
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline.setDeadline(std::numeric_limits<qint64>::max(), timerType);
    QVERIFY(deadline.isForever());
    QVERIFY(!deadline.hasExpired());
    QCOMPARE(deadline.remainingTime(), qint64(-1));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(-1));
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline.setPreciseDeadline(std::numeric_limits<qint64>::max(), 0, timerType);
    QVERIFY(deadline.isForever());
    QVERIFY(!deadline.hasExpired());
    QCOMPARE(deadline.remainingTime(), qint64(-1));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(-1));
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    QCOMPARE(deadline, deadline);
    QVERIFY(!(deadline < deadline));
    QCOMPARE_LE(deadline, deadline);
    QCOMPARE_GE(deadline, deadline);
    QVERIFY(!(deadline > deadline));
    QT_TEST_ALL_COMPARISON_OPS(deadline, deadline, Qt::strong_ordering::equal);

    // adding to forever must still be forever
    QDeadlineTimer deadline2 = deadline + 1;
    QVERIFY(deadline2.isForever());
    QVERIFY(!deadline2.hasExpired());
    QCOMPARE(deadline2.remainingTime(), qint64(-1));
    QCOMPARE(deadline2.remainingTimeNSecs(), qint64(-1));
    QCOMPARE(deadline2.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline2.deadlineNSecs(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline2.timerType(), deadline.timerType());

    QCOMPARE(deadline2 - deadline, qint64(0));
    QCOMPARE(deadline2, deadline);
    QVERIFY(!(deadline2 < deadline));
    QCOMPARE_LE(deadline2, deadline);
    QCOMPARE_GE(deadline2, deadline);
    QVERIFY(!(deadline2 > deadline));
    QT_TEST_ALL_COMPARISON_OPS(deadline2, deadline, Qt::strong_ordering::equal);

    // subtracting from forever is *also* forever
    deadline2 = deadline - 1;
    QVERIFY(deadline2.isForever());
    QVERIFY(!deadline2.hasExpired());
    QCOMPARE(deadline2.remainingTime(), qint64(-1));
    QCOMPARE(deadline2.remainingTimeNSecs(), qint64(-1));
    QCOMPARE(deadline2.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline2.deadlineNSecs(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline2.timerType(), deadline.timerType());

    QCOMPARE(deadline2 - deadline, qint64(0));
    QCOMPARE(deadline2, deadline);
    QVERIFY(!(deadline2 < deadline));
    QCOMPARE_LE(deadline2, deadline);
    QCOMPARE_GE(deadline2, deadline);
    QVERIFY(!(deadline2 > deadline));
    QT_TEST_ALL_COMPARISON_OPS(deadline2, deadline, Qt::strong_ordering::equal);

    // compare and order against a default-constructed object
    QDeadlineTimer expired;
    QVERIFY(!(deadline == expired));
    QCOMPARE_NE(deadline, expired);
    QVERIFY(!(deadline < expired));
    QVERIFY(!(deadline <= expired));
    QCOMPARE_GE(deadline, expired);
    QCOMPARE_GT(deadline, expired);
    QT_TEST_EQUALITY_OPS(deadline, expired, false);
}

void tst_QDeadlineTimer::current()
{
    auto deadline = QDeadlineTimer::current(timerType);
    QVERIFY(deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE_NE(deadline.deadline(), 0);
    QCOMPARE_NE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE_NE(deadline.deadlineNSecs(), 0);
    QCOMPARE_NE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    // subtracting from current should be "more expired"
    QDeadlineTimer earlierDeadline = deadline - 1;
    QVERIFY(earlierDeadline.hasExpired());
    QVERIFY(!earlierDeadline.isForever());
    QCOMPARE(earlierDeadline.timerType(), timerType);
    QCOMPARE(earlierDeadline.remainingTime(), qint64(0));
    QCOMPARE(earlierDeadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE_NE(earlierDeadline.deadline(), 0);
    QCOMPARE_NE(earlierDeadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE_NE(earlierDeadline.deadlineNSecs(), 0);
    QCOMPARE_NE(earlierDeadline.deadlineNSecs(), std::numeric_limits<qint64>::max());
    QCOMPARE(earlierDeadline.deadline(), deadline.deadline() - 1);
    QCOMPARE(earlierDeadline.deadlineNSecs(), deadline.deadlineNSecs() - 1000*1000);

    QCOMPARE(earlierDeadline - deadline, qint64(-1));
    QCOMPARE_NE(earlierDeadline, deadline);
    QCOMPARE_LT(earlierDeadline, deadline);
    QCOMPARE_LE(earlierDeadline, deadline);
    QVERIFY(!(earlierDeadline >= deadline));
    QVERIFY(!(earlierDeadline > deadline));
    QT_TEST_ALL_COMPARISON_OPS(earlierDeadline, deadline, Qt::strong_ordering::less);
}

void tst_QDeadlineTimer::deadlines()
{
    QDeadlineTimer deadline(4 * minResolution, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTime(), (3 * minResolution));
    QCOMPARE_LE(deadline.remainingTime(), (4 * minResolution));
    QCOMPARE_GT(deadline.remainingTimeNSecs(), (3000000 * minResolution));
    QCOMPARE_LE(deadline.remainingTimeNSecs(), (4000000 * minResolution));
    QCOMPARE_NE(deadline.deadline(), 0);
    QCOMPARE_NE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE_NE(deadline.deadlineNSecs(), 0);
    QCOMPARE_NE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline.setRemainingTime(4 * minResolution, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTime(), (3 * minResolution));
    QCOMPARE_LE(deadline.remainingTime(), (4 * minResolution));
    QCOMPARE_GT(deadline.remainingTimeNSecs(), (3000000 * minResolution));
    QCOMPARE_LE(deadline.remainingTimeNSecs(), (4000000 * minResolution));
    QCOMPARE_NE(deadline.deadline(), 0);
    QCOMPARE_NE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE_NE(deadline.deadlineNSecs(), 0);
    QCOMPARE_NE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline.setPreciseRemainingTime(0, 4000000 * minResolution, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTime(), (3 * minResolution));
    QCOMPARE_LE(deadline.remainingTime(), (4 * minResolution));
    QCOMPARE_GT(deadline.remainingTimeNSecs(), (3000000 * minResolution));
    QCOMPARE_LE(deadline.remainingTimeNSecs(), (4000000 * minResolution));
    QCOMPARE_NE(deadline.deadline(), 0);
    QCOMPARE_NE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE_NE(deadline.deadlineNSecs(), 0);
    QCOMPARE_NE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    deadline.setPreciseRemainingTime(1, 0, timerType); // 1 sec
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTime(), (1000 - minResolution));
    QCOMPARE_LE(deadline.remainingTime(), 1000);
    QCOMPARE_GT(deadline.remainingTimeNSecs(), (1000 - minResolution)*1000*1000);
    QCOMPARE_LE(deadline.remainingTimeNSecs(), (1000*1000*1000));
    QCOMPARE_NE(deadline.deadline(), 0);
    QCOMPARE_NE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE_NE(deadline.deadlineNSecs(), 0);
    QCOMPARE_NE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    // adding to a future deadline must still be further in the future
    QDeadlineTimer laterDeadline = deadline + 1;
    QVERIFY(!laterDeadline.hasExpired());
    QVERIFY(!laterDeadline.isForever());
    QCOMPARE(laterDeadline.timerType(), timerType);
    QCOMPARE_GT(laterDeadline.remainingTime(), (1000 - minResolution));
    QCOMPARE_LE(laterDeadline.remainingTime(), 1001);
    QCOMPARE_GT(laterDeadline.remainingTimeNSecs(), (1001 - minResolution)*1000*1000);
    QCOMPARE_LE(laterDeadline.remainingTimeNSecs(), (1001*1000*1000));
    QCOMPARE_NE(laterDeadline.deadline(), 0);
    QCOMPARE_NE(laterDeadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE_NE(laterDeadline.deadlineNSecs(), 0);
    QCOMPARE_NE(laterDeadline.deadlineNSecs(), std::numeric_limits<qint64>::max());
    QCOMPARE(laterDeadline.deadline(), deadline.deadline() + 1);
    QCOMPARE(laterDeadline.deadlineNSecs(), deadline.deadlineNSecs() + 1000*1000);

    QCOMPARE(laterDeadline - deadline, qint64(1));
    QCOMPARE_NE(laterDeadline, deadline);
    QVERIFY(!(laterDeadline < deadline));
    QVERIFY(!(laterDeadline <= deadline));
    QCOMPARE_GE(laterDeadline, deadline);
    QCOMPARE_GT(laterDeadline, deadline);
    QT_TEST_ALL_COMPARISON_OPS(laterDeadline, deadline, Qt::strong_ordering::greater);

    // compare and order against a default-constructed object
    QDeadlineTimer expired;
    QVERIFY(!(deadline == expired));
    QCOMPARE_NE(deadline, expired);
    QVERIFY(!(deadline < expired));
    QVERIFY(!(deadline <= expired));
    QCOMPARE_GE(deadline, expired);
    QCOMPARE_GT(deadline, expired);
    QT_TEST_EQUALITY_OPS(deadline, expired, false);

    // compare and order against a forever deadline
    QDeadlineTimer forever_(QDeadlineTimer::Forever);
    QT_TEST_EQUALITY_OPS(deadline, forever_, false);
    QVERIFY(!(deadline == forever_));
    QCOMPARE_NE(deadline, forever_);
    QCOMPARE_LT(deadline, forever_);
    QCOMPARE_LE(deadline, forever_);
    QVERIFY(!(deadline >= forever_));
    QVERIFY(!(deadline > forever_));
}

void tst_QDeadlineTimer::setDeadline()
{
    auto now = QDeadlineTimer::current(timerType);
    QDeadlineTimer deadline;

    deadline.setDeadline(now.deadline(), timerType);
    QVERIFY(deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE(deadline.deadline(), now.deadline());
    // don't check deadlineNSecs!

    deadline.setPreciseDeadline(now.deadlineNSecs() / (1000 * 1000 * 1000),
                                now.deadlineNSecs() % (1000 * 1000 * 1000), timerType);
    QVERIFY(deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE(deadline.deadline(), now.deadline());
    QCOMPARE(deadline.deadlineNSecs(), now.deadlineNSecs());

    now = QDeadlineTimer::current(timerType);
    deadline.setDeadline(now.deadline() + 4 * minResolution, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTime(), (3 * minResolution));
    QCOMPARE_LE(deadline.remainingTime(), (4 * minResolution));
    QCOMPARE_GT(deadline.remainingTimeNSecs(), (3000000 * minResolution));
    QCOMPARE_LE(deadline.remainingTimeNSecs(), (4000000 * minResolution));
    QCOMPARE(deadline.deadline(), now.deadline() + 4 * minResolution);  // yes, it's exact
    // don't check deadlineNSecs!

    now = QDeadlineTimer::current(timerType);
    qint64 nsec = now.deadlineNSecs() + 4000000 * minResolution;
    deadline.setPreciseDeadline(nsec / (1000 * 1000 * 1000),
                                nsec % (1000 * 1000 * 1000), timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTime(), (3 * minResolution));
    QCOMPARE_LE(deadline.remainingTime(), (4 * minResolution));
    QCOMPARE_GT(deadline.remainingTimeNSecs(), (3000000 * minResolution));
    QCOMPARE_LE(deadline.remainingTimeNSecs(), (4000000 * minResolution));
    QCOMPARE(deadline.deadline(), nsec / (1000 * 1000));
    QCOMPARE(deadline.deadlineNSecs(), nsec);
}

void tst_QDeadlineTimer::overflow()
{
    // Check the constructor for overflows (should also cover saturating the result of the deadline() method if overflowing)
    QDeadlineTimer now = QDeadlineTimer::current(timerType), deadline(std::numeric_limits<qint64>::max() - 1, timerType);
    QVERIFY(deadline.isForever() || deadline.deadline() >= now.deadline());

    // Check the setDeadline with milliseconds (should also cover implicitly setting the nanoseconds as qint64 max)
    deadline.setDeadline(std::numeric_limits<qint64>::max() - 1, timerType);
    QVERIFY(deadline.isForever() || deadline.deadline() >= now.deadline());

    // Check the setRemainingTime with milliseconds (should also cover implicitly setting the nanoseconds as qint64 max)
    deadline.setRemainingTime(std::numeric_limits<qint64>::max() - 1, timerType);
    QVERIFY(deadline.isForever() || deadline.deadline() >= now.deadline());

    // Check that the deadline gets saturated when the arguments of setPreciseDeadline are large
    deadline.setPreciseDeadline(std::numeric_limits<qint64>::max() - 1, std::numeric_limits<qint64>::max() - 1, timerType);
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QVERIFY(deadline.isForever());

    // Check that remainingTime gets saturated if we overflow
    deadline.setPreciseRemainingTime(std::numeric_limits<qint64>::max() - 1, std::numeric_limits<qint64>::max() - 1, timerType);
    QCOMPARE(deadline.remainingTime(), qint64(-1));
    QVERIFY(deadline.isForever());

    // Check that we saturate the getter for nanoseconds
    deadline.setPreciseDeadline(std::numeric_limits<qint64>::max() - 1, 0, timerType);
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    // Check that adding nanoseconds and overflowing is consistent and saturates the timer
    deadline = QDeadlineTimer::addNSecs(deadline, std::numeric_limits<qint64>::max() - 1);
    QVERIFY(deadline.isForever());

    // Make sure forever is forever, regardless of us subtracting time from it
    deadline = QDeadlineTimer(QDeadlineTimer::Forever, timerType);
    deadline = QDeadlineTimer::addNSecs(deadline, -10000);
    QVERIFY(deadline.isForever());

    // Make sure we get the correct result when moving the deadline back and forth in time
    QDeadlineTimer current = QDeadlineTimer::current(timerType);
    QDeadlineTimer takenNSecs = QDeadlineTimer::addNSecs(current, -1000);
    QVERIFY(takenNSecs.deadlineNSecs() - current.deadlineNSecs() == -1000);
    QDeadlineTimer addedNSecs = QDeadlineTimer::addNSecs(current, 1000);
    QVERIFY(addedNSecs.deadlineNSecs() - current.deadlineNSecs() == 1000);

    // Make sure the calculation goes as expected when we need to subtract nanoseconds
    // We make use of an additional timer to be certain that
    // even when the environment is under load we can track the
    // time needed to do the calls
    static constexpr qint64 nsExpected = 1000 * 1000 * 1000 - 1000;     // 1s - 1000ns, what we pass to setPreciseRemainingTime() later

    QElapsedTimer callTimer;
    callTimer.start();

    deadline = QDeadlineTimer::current(timerType);
    qint64 nsDeadline = deadline.deadlineNSecs();
    // We adjust in relation to current() here, so we expect the difference to be a tad over the exact number.
    // However we are tracking the elapsed time, so it shouldn't be a problem.
    deadline.setPreciseRemainingTime(1, -1000, timerType);
    qint64 difference = (deadline.deadlineNSecs() - nsDeadline) - nsExpected;
    QCOMPARE_GE(difference, 0);        // Should always be true, but just in case
    QCOMPARE_LE(difference, callTimer.nsecsElapsed()); // Ideally difference should be 0 exactly

    // Make sure setRemainingTime underflows gracefully
    deadline.setPreciseRemainingTime(std::numeric_limits<qint64>::min() / 10, 0, timerType);
    QVERIFY(deadline.isForever());      // The above could underflow, so make sure we did set to Forever
    QCOMPARE(deadline.remainingTimeNSecs(), -1);
    QCOMPARE(deadline.remainingTime(), -1);
    // If the timer is saturated we don't want to get a valid number of milliseconds
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());

    // Check that the conversion to milliseconds and nanoseconds underflows gracefully
    deadline.setPreciseDeadline(std::numeric_limits<qint64>::min() / 10, 0, timerType);
    QVERIFY(!deadline.isForever());     // The above underflows, make sure we don't saturate to Forever
    QVERIFY(deadline.deadline() == std::numeric_limits<qint64>::min());
    QVERIFY(deadline.deadlineNSecs() == std::numeric_limits<qint64>::min());

    // Check that subtracting max() twice doesn't make it become positive
    deadline.setPreciseDeadline(0);
    deadline -= std::numeric_limits<qint64>::max();
    deadline -= std::numeric_limits<qint64>::max();
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::min());
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::min());

    // Ditto for adding max()
    deadline.setPreciseDeadline(0);
    deadline += std::numeric_limits<qint64>::max();
    deadline += std::numeric_limits<qint64>::max();
    QVERIFY(deadline.isForever());      // it's so far in the future it's effectively forever
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());

    // But we don't un-become forever after saturation
    deadline -= std::numeric_limits<qint64>::max();
    QVERIFY(deadline.isForever());
    QCOMPARE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());
}

void tst_QDeadlineTimer::expire()
{
    QDeadlineTimer deadline(minResolution, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());

    qint64 previousDeadline = deadline.deadlineNSecs();

    QTest::qSleep(2 * minResolution);

    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE_NE(deadline.deadline(), 0);
    QCOMPARE_NE(deadline.deadline(), std::numeric_limits<qint64>::max());
    QCOMPARE_NE(deadline.deadlineNSecs(), 0);
    QCOMPARE_NE(deadline.deadlineNSecs(), std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), previousDeadline);
}

void tst_QDeadlineTimer::stdchrono()
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    // create some forevers
    QDeadlineTimer deadline = milliseconds::max();
    QVERIFY(deadline.isForever());
    deadline = milliseconds::max();
    QVERIFY(deadline.isForever());
    deadline.setRemainingTime(milliseconds::max(), timerType);
    QVERIFY(deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    deadline = nanoseconds::max();
    QVERIFY(deadline.isForever());
    deadline.setRemainingTime(nanoseconds::max(), timerType);
    QVERIFY(deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    deadline = hours::max();
    QVERIFY(deadline.isForever());
    deadline.setRemainingTime(hours::max(), timerType);
    QVERIFY(deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);

    deadline = time_point<system_clock>::max();
    QVERIFY(deadline.isForever());
    deadline.setDeadline(time_point<system_clock>::max(), timerType);
    QVERIFY(deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    deadline = time_point<steady_clock>::max();
    QVERIFY(deadline.isForever());
    deadline.setDeadline(time_point<steady_clock>::max(), timerType);
    QVERIFY(deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);

    QVERIFY(deadline == time_point<steady_clock>::max());
    QVERIFY(deadline == time_point<system_clock>::max());
    QCOMPARE(deadline.remainingTimeAsDuration(), nanoseconds::max());

    // make it expired
    deadline = time_point<system_clock>();
    QVERIFY(deadline.hasExpired());
    deadline.setDeadline(time_point<system_clock>(), timerType);
    QVERIFY(deadline.hasExpired());
    QCOMPARE(deadline.timerType(), timerType);
    deadline = time_point<steady_clock>();
    QVERIFY(deadline.hasExpired());
    deadline.setDeadline(time_point<steady_clock>(), timerType);
    QVERIFY(deadline.hasExpired());
    QCOMPARE(deadline.timerType(), timerType);

    QCOMPARE(deadline.remainingTimeAsDuration(), nanoseconds::zero());

    QDeadlineTimer now;
    bool timersExecuted = false;

    auto steady_before = steady_clock::now();
    auto system_before = system_clock::now();

    decltype(steady_before) steady_after, steady_deadline;
    decltype(system_before) system_after, system_deadline;

    QTimer::singleShot(minResolution, Qt::PreciseTimer, [&]() {
        now = QDeadlineTimer::current(timerType);
        QTimer::singleShot(minResolution, Qt::PreciseTimer, [&]() {
            steady_deadline = now.deadline<steady_clock>();
            system_deadline = now.deadline<system_clock>();
            steady_after = steady_clock::now();
            system_after = system_clock::now();
            timersExecuted = true;
        });
    });

    QTRY_VERIFY2_WITH_TIMEOUT(timersExecuted,
        "Looks like timers didn't fire on time.", 4 * minResolution);

    {
        qint64 before = duration_cast<nanoseconds>(steady_before.time_since_epoch()).count();
        qint64 after = duration_cast<nanoseconds>(steady_after.time_since_epoch()).count();
        QCOMPARE_GT(now.deadlineNSecs(), before);
        QCOMPARE_LT(now.deadlineNSecs(), after);
    }

    {
        auto diff = duration_cast<milliseconds>(steady_after - steady_deadline);
        QCOMPARE_GT(diff.count(), minResolution / 2);
        QCOMPARE_LT(diff.count(), 3 * minResolution / 2);
        QDeadlineTimer dt_after(steady_after, timerType);
        QCOMPARE_LT(now, dt_after);
        QT_TEST_ALL_COMPARISON_OPS(now, dt_after, Qt::strong_ordering::less);

        diff = duration_cast<milliseconds>(steady_deadline - steady_before);
        QCOMPARE_GT(diff.count(), minResolution / 2);
        QCOMPARE_LT(diff.count(), 3 * minResolution / 2);
        QDeadlineTimer dt_before(steady_before, timerType);
        QCOMPARE_GT(now, dt_before);
        QT_TEST_ALL_COMPARISON_OPS(now, dt_before, Qt::strong_ordering::greater);
    }
    {
        auto diff = duration_cast<milliseconds>(system_after - system_deadline);
        QCOMPARE_GT(diff.count(), minResolution / 2);
        QCOMPARE_LT(diff.count(), 3 * minResolution / 2);
        QDeadlineTimer dt_after(system_after, timerType);
        QCOMPARE_LT(now, dt_after);
        QT_TEST_ALL_COMPARISON_OPS(now, dt_after, Qt::strong_ordering::less);

        diff = duration_cast<milliseconds>(system_deadline - system_before);
        QCOMPARE_GT(diff.count(), minResolution / 2);
        QCOMPARE_LT(diff.count(), 3 * minResolution / 2);
        QDeadlineTimer dt_before(system_before, timerType);
        QCOMPARE_GT(now, dt_before);
        QT_TEST_ALL_COMPARISON_OPS(now, dt_before, Qt::strong_ordering::greater);
    }

    // make it regular
    now = QDeadlineTimer::current(timerType);
    deadline.setRemainingTime(4ms * minResolution, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTimeAsDuration(), 3ms * minResolution);
    QCOMPARE_LT(deadline.remainingTimeAsDuration(), 5ms * minResolution);
    QCOMPARE_GT(deadline.remainingTimeAsDuration(), 3'000'000ns * minResolution);
    QCOMPARE_LT(deadline.remainingTimeAsDuration(), 5'000'000ns * minResolution);
    QCOMPARE_GT(deadline.deadline<steady_clock>(), (steady_clock::now() + 3ms * minResolution));
    QCOMPARE_LT(deadline.deadline<steady_clock>(), (steady_clock::now() + 5ms * minResolution));
    QCOMPARE_GT(deadline.deadline<system_clock>(), (system_clock::now() + 3ms * minResolution));
    QCOMPARE_LT(deadline.deadline<system_clock>(), (system_clock::now() + 5ms * minResolution));
    QCOMPARE_GT((deadline.deadline<steady_clock, milliseconds>()),
                steady_clock::now() + 3ms * minResolution);
    QCOMPARE_LT((deadline.deadline<steady_clock, milliseconds>()),
                steady_clock::now() + 5ms * minResolution);
    QCOMPARE_GT((deadline.deadline<system_clock, milliseconds>()),
                system_clock::now() + 3ms * minResolution);
    QCOMPARE_LT((deadline.deadline<system_clock, milliseconds>()),
                system_clock::now() + 5ms * minResolution);
    QCOMPARE_GT(deadline, now + 3ms * minResolution);
    QCOMPARE_LT(deadline, now + 5ms * minResolution);
    QCOMPARE_GT(deadline, now + 3000000ns * minResolution);
    QCOMPARE_LT(deadline, now + 5000000ns * minResolution);
    QCOMPARE_GT(deadline, 3ms * minResolution);
    QCOMPARE_LT(deadline, 5ms * minResolution);
    QCOMPARE_GT(deadline, 3000000ns * minResolution);
    QCOMPARE_LT(deadline, 5000000ns * minResolution);
    QCOMPARE_GE(deadline, steady_clock::now());
    QCOMPARE_GE(deadline, system_clock::now());
    QT_TEST_ALL_COMPARISON_OPS(deadline, now + 3ms * minResolution, Qt::strong_ordering::greater);
    QT_TEST_ALL_COMPARISON_OPS(deadline, now + 5ms * minResolution, Qt::strong_ordering::less);
    QT_TEST_ALL_COMPARISON_OPS(deadline, now + 3000000ns * minResolution, Qt::strong_ordering::greater);
    QT_TEST_ALL_COMPARISON_OPS(deadline, now + 5000000ns * minResolution, Qt::strong_ordering::less);
    QT_TEST_ALL_COMPARISON_OPS(deadline, 3ms * minResolution, Qt::strong_ordering::greater);
    QT_TEST_ALL_COMPARISON_OPS(deadline, 5ms * minResolution, Qt::strong_ordering::less);
    QT_TEST_ALL_COMPARISON_OPS(deadline, steady_clock::now(), Qt::strong_ordering::greater);
    QT_TEST_ALL_COMPARISON_OPS(deadline, system_clock::now(), Qt::strong_ordering::greater);

    now = QDeadlineTimer::current(timerType);
    deadline = QDeadlineTimer(1s, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTimeAsDuration(), 1s - 1ms * minResolution);
    QCOMPARE_LE(deadline.remainingTimeAsDuration(), 1s);
    QCOMPARE_GT(deadline.deadline<steady_clock>(), steady_clock::now() + 1s - 1ms * minResolution);
    QCOMPARE_LE(deadline.deadline<steady_clock>(), steady_clock::now() + 1s + 1ms * minResolution);
    QCOMPARE_GT(deadline.deadline<system_clock>(), system_clock::now() + 1s - 1ms * minResolution);
    QCOMPARE_LE(deadline.deadline<system_clock>(), system_clock::now() + 1s + 1ms * minResolution);
    QCOMPARE_GT(deadline, 1s - 1ms * minResolution);
    QCOMPARE_LE(deadline, 1s);

    now = QDeadlineTimer::current(timerType);
    deadline.setRemainingTime(1h, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTimeAsDuration(), 1h - 1ms * minResolution);
    QCOMPARE_LE(deadline.remainingTimeAsDuration(), 1h);
    QCOMPARE_GT(deadline.deadline<steady_clock>(), steady_clock::now() + 1h - 1ms * minResolution);
    QCOMPARE_LE(deadline.deadline<steady_clock>(), steady_clock::now() + 1h + 1ms * minResolution);
    QCOMPARE_GT(deadline.deadline<system_clock>(), system_clock::now() + 1h - 1ms * minResolution);
    QCOMPARE_LE(deadline.deadline<system_clock>(), system_clock::now() + 1h + 1ms * minResolution);

    now = QDeadlineTimer::current(timerType);
    deadline.setDeadline(system_clock::now() + 1s, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTimeAsDuration(), 1s - 1ms * minResolution);
    QCOMPARE_LE(deadline.remainingTimeAsDuration(), 1s);
    QCOMPARE_GT(deadline.deadline<steady_clock>(), steady_clock::now() + 1s - 1ms * minResolution);
    QCOMPARE_LE(deadline.deadline<steady_clock>(), steady_clock::now() + 1s + 1ms * minResolution);
    QCOMPARE_GT(deadline.deadline<system_clock>(), system_clock::now() + 1s - 1ms * minResolution);
    QCOMPARE_LE(deadline.deadline<system_clock>(), system_clock::now() + 1s + 1ms * minResolution);

    now = QDeadlineTimer::current(timerType);
    deadline.setDeadline(steady_clock::now() + 1s, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE_GT(deadline.remainingTimeAsDuration(), 1s - 1ms * minResolution);
    QCOMPARE_LE(deadline.remainingTimeAsDuration(), 1s);
    QCOMPARE_GT(deadline.deadline<steady_clock>(), steady_clock::now() + 1s - 1ms * minResolution);
    QCOMPARE_LE(deadline.deadline<steady_clock>(), steady_clock::now() + 1s + 1ms * minResolution);
    QCOMPARE_GT(deadline.deadline<system_clock>(), system_clock::now() + 1s - 1ms * minResolution);
    QCOMPARE_LE(deadline.deadline<system_clock>(), system_clock::now() + 1s + 1ms * minResolution);
}

QTEST_MAIN(tst_QDeadlineTimer)

#include "tst_qdeadlinetimer.moc"
