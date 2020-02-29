/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QElapsedTimer>
#include <QtTest/QtTest>

#if __has_include(<chrono>)
#  include <chrono>
#endif

static const int minResolution = 400; // the minimum resolution for the tests

Q_DECLARE_METATYPE(Qt::TimerType)

class tst_QDeadlineTimer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase_data();
    void basics();
    void foreverness();
    void current();
    void deadlines();
    void setDeadline();
    void overflow();
    void expire();
    void stdchrono();
};

void tst_QDeadlineTimer::initTestCase_data()
{
    qRegisterMetaType<Qt::TimerType>();
    QTest::addColumn<Qt::TimerType>("timerType");
    QTest::newRow("precise") << Qt::PreciseTimer;
    QTest::newRow("coarse") << Qt::CoarseTimer;
}

void tst_QDeadlineTimer::basics()
{
    QDeadlineTimer deadline;
    QCOMPARE(deadline.timerType(), Qt::CoarseTimer);

    QFETCH_GLOBAL(Qt::TimerType, timerType);
    deadline = QDeadlineTimer(timerType);
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline, QDeadlineTimer(timerType));
    QVERIFY(!(deadline != QDeadlineTimer(timerType)));
    QVERIFY(!(deadline < QDeadlineTimer()));
    QVERIFY(deadline <= QDeadlineTimer());
    QVERIFY(deadline >= QDeadlineTimer());
    QVERIFY(!(deadline > QDeadlineTimer()));
    QVERIFY(!(deadline < deadline));
    QVERIFY(deadline <= deadline);
    QVERIFY(deadline >= deadline);
    QVERIFY(!(deadline > deadline));

    // should have expired, but we may be running too early after boot
    QTRY_VERIFY_WITH_TIMEOUT(deadline.hasExpired(), 100);

    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QCOMPARE(deadline.deadline(), qint64(0));
    QCOMPARE(deadline.deadlineNSecs(), qint64(0));

    deadline.setRemainingTime(0, timerType);
    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QVERIFY(deadline.deadline() != 0);
    QVERIFY(deadline.deadline() != std::numeric_limits<qint64>::max());
    QVERIFY(deadline.deadlineNSecs() != 0);
    QVERIFY(deadline.deadlineNSecs() != std::numeric_limits<qint64>::max());

    deadline.setPreciseRemainingTime(0, 0, timerType);
    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QVERIFY(deadline.deadline() != 0);
    QVERIFY(deadline.deadline() != std::numeric_limits<qint64>::max());
    QVERIFY(deadline.deadlineNSecs() != 0);
    QVERIFY(deadline.deadlineNSecs() != std::numeric_limits<qint64>::max());

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
    QFETCH_GLOBAL(Qt::TimerType, timerType);
    // we don't check whether timerType() is our type since it's possible it detects it's forever

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
    QVERIFY(deadline <= deadline);
    QVERIFY(deadline >= deadline);
    QVERIFY(!(deadline > deadline));

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
    QVERIFY(deadline2 <= deadline);
    QVERIFY(deadline2 >= deadline);
    QVERIFY(!(deadline2 > deadline));

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
    QVERIFY(deadline2 <= deadline);
    QVERIFY(deadline2 >= deadline);
    QVERIFY(!(deadline2 > deadline));

    // compare and order against a default-constructed object
    QDeadlineTimer expired;
    QVERIFY(!(deadline == expired));
    QVERIFY(deadline != expired);
    QVERIFY(!(deadline < expired));
    QVERIFY(!(deadline <= expired));
    QVERIFY(deadline >= expired);
    QVERIFY(deadline > expired);
}

void tst_QDeadlineTimer::current()
{
    QFETCH_GLOBAL(Qt::TimerType, timerType);
    auto deadline = QDeadlineTimer::current(timerType);
    QVERIFY(deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QVERIFY(deadline.deadline() != 0);
    QVERIFY(deadline.deadline() != std::numeric_limits<qint64>::max());
    QVERIFY(deadline.deadlineNSecs() != 0);
    QVERIFY(deadline.deadlineNSecs() != std::numeric_limits<qint64>::max());

    // subtracting from current should be "more expired"
    QDeadlineTimer earlierDeadline = deadline - 1;
    QVERIFY(earlierDeadline.hasExpired());
    QVERIFY(!earlierDeadline.isForever());
    QCOMPARE(earlierDeadline.timerType(), timerType);
    QCOMPARE(earlierDeadline.remainingTime(), qint64(0));
    QCOMPARE(earlierDeadline.remainingTimeNSecs(), qint64(0));
    QVERIFY(earlierDeadline.deadline() != 0);
    QVERIFY(earlierDeadline.deadline() != std::numeric_limits<qint64>::max());
    QVERIFY(earlierDeadline.deadlineNSecs() != 0);
    QVERIFY(earlierDeadline.deadlineNSecs() != std::numeric_limits<qint64>::max());
    QCOMPARE(earlierDeadline.deadline(), deadline.deadline() - 1);
    QCOMPARE(earlierDeadline.deadlineNSecs(), deadline.deadlineNSecs() - 1000*1000);

    QCOMPARE(earlierDeadline - deadline, qint64(-1));
    QVERIFY(earlierDeadline != deadline);
    QVERIFY(earlierDeadline < deadline);
    QVERIFY(earlierDeadline <= deadline);
    QVERIFY(!(earlierDeadline >= deadline));
    QVERIFY(!(earlierDeadline > deadline));
}

void tst_QDeadlineTimer::deadlines()
{
    QFETCH_GLOBAL(Qt::TimerType, timerType);

    QDeadlineTimer deadline(4 * minResolution, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(deadline.remainingTime() > (3 * minResolution));
    QVERIFY(deadline.remainingTime() <= (4 * minResolution));
    QVERIFY(deadline.remainingTimeNSecs() > (3000000 * minResolution));
    QVERIFY(deadline.remainingTimeNSecs() <= (4000000 * minResolution));
    QVERIFY(deadline.deadline() != 0);
    QVERIFY(deadline.deadline() != std::numeric_limits<qint64>::max());
    QVERIFY(deadline.deadlineNSecs() != 0);
    QVERIFY(deadline.deadlineNSecs() != std::numeric_limits<qint64>::max());

    deadline.setRemainingTime(4 * minResolution, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(deadline.remainingTime() > (3 * minResolution));
    QVERIFY(deadline.remainingTime() <= (4 * minResolution));
    QVERIFY(deadline.remainingTimeNSecs() > (3000000 * minResolution));
    QVERIFY(deadline.remainingTimeNSecs() <= (4000000 * minResolution));
    QVERIFY(deadline.deadline() != 0);
    QVERIFY(deadline.deadline() != std::numeric_limits<qint64>::max());
    QVERIFY(deadline.deadlineNSecs() != 0);
    QVERIFY(deadline.deadlineNSecs() != std::numeric_limits<qint64>::max());

    deadline.setPreciseRemainingTime(0, 4000000 * minResolution, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(deadline.remainingTime() > (3 * minResolution));
    QVERIFY(deadline.remainingTime() <= (4 * minResolution));
    QVERIFY(deadline.remainingTimeNSecs() > (3000000 * minResolution));
    QVERIFY(deadline.remainingTimeNSecs() <= (4000000 * minResolution));
    QVERIFY(deadline.deadline() != 0);
    QVERIFY(deadline.deadline() != std::numeric_limits<qint64>::max());
    QVERIFY(deadline.deadlineNSecs() != 0);
    QVERIFY(deadline.deadlineNSecs() != std::numeric_limits<qint64>::max());

    deadline.setPreciseRemainingTime(1, 0, timerType); // 1 sec
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(deadline.remainingTime() > (1000 - minResolution));
    QVERIFY(deadline.remainingTime() <= 1000);
    QVERIFY(deadline.remainingTimeNSecs() > (1000 - minResolution)*1000*1000);
    QVERIFY(deadline.remainingTimeNSecs() <= (1000*1000*1000));
    QVERIFY(deadline.deadline() != 0);
    QVERIFY(deadline.deadline() != std::numeric_limits<qint64>::max());
    QVERIFY(deadline.deadlineNSecs() != 0);
    QVERIFY(deadline.deadlineNSecs() != std::numeric_limits<qint64>::max());

    // adding to a future deadline must still be further in the future
    QDeadlineTimer laterDeadline = deadline + 1;
    QVERIFY(!laterDeadline.hasExpired());
    QVERIFY(!laterDeadline.isForever());
    QCOMPARE(laterDeadline.timerType(), timerType);
    QVERIFY(laterDeadline.remainingTime() > (1000 - minResolution));
    QVERIFY(laterDeadline.remainingTime() <= 1001);
    QVERIFY(laterDeadline.remainingTimeNSecs() > (1001 - minResolution)*1000*1000);
    QVERIFY(laterDeadline.remainingTimeNSecs() <= (1001*1000*1000));
    QVERIFY(laterDeadline.deadline() != 0);
    QVERIFY(laterDeadline.deadline() != std::numeric_limits<qint64>::max());
    QVERIFY(laterDeadline.deadlineNSecs() != 0);
    QVERIFY(laterDeadline.deadlineNSecs() != std::numeric_limits<qint64>::max());
    QCOMPARE(laterDeadline.deadline(), deadline.deadline() + 1);
    QCOMPARE(laterDeadline.deadlineNSecs(), deadline.deadlineNSecs() + 1000*1000);

    QCOMPARE(laterDeadline - deadline, qint64(1));
    QVERIFY(laterDeadline != deadline);
    QVERIFY(!(laterDeadline < deadline));
    QVERIFY(!(laterDeadline <= deadline));
    QVERIFY(laterDeadline >= deadline);
    QVERIFY(laterDeadline > deadline);

    // compare and order against a default-constructed object
    QDeadlineTimer expired;
    QVERIFY(!(deadline == expired));
    QVERIFY(deadline != expired);
    QVERIFY(!(deadline < expired));
    QVERIFY(!(deadline <= expired));
    QVERIFY(deadline >= expired);
    QVERIFY(deadline > expired);

    // compare and order against a forever deadline
    QDeadlineTimer forever_(QDeadlineTimer::Forever);
    QVERIFY(!(deadline == forever_));
    QVERIFY(deadline != forever_);
    QVERIFY(deadline < forever_);
    QVERIFY(deadline <= forever_);
    QVERIFY(!(deadline >= forever_));
    QVERIFY(!(deadline > forever_));
}

void tst_QDeadlineTimer::setDeadline()
{
    QFETCH_GLOBAL(Qt::TimerType, timerType);
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
    QVERIFY(deadline.remainingTime() > (3 * minResolution));
    QVERIFY(deadline.remainingTime() <= (4 * minResolution));
    QVERIFY(deadline.remainingTimeNSecs() > (3000000 * minResolution));
    QVERIFY(deadline.remainingTimeNSecs() <= (4000000 * minResolution));
    QCOMPARE(deadline.deadline(), now.deadline() + 4 * minResolution);  // yes, it's exact
    // don't check deadlineNSecs!

    now = QDeadlineTimer::current(timerType);
    qint64 nsec = now.deadlineNSecs() + 4000000 * minResolution;
    deadline.setPreciseDeadline(nsec / (1000 * 1000 * 1000),
                                nsec % (1000 * 1000 * 1000), timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(deadline.remainingTime() > (3 * minResolution));
    QVERIFY(deadline.remainingTime() <= (4 * minResolution));
    QVERIFY(deadline.remainingTimeNSecs() > (3000000 * minResolution));
    QVERIFY(deadline.remainingTimeNSecs() <= (4000000 * minResolution));
    QCOMPARE(deadline.deadline(), nsec / (1000 * 1000));
    QCOMPARE(deadline.deadlineNSecs(), nsec);
}

void tst_QDeadlineTimer::overflow()
{
    QFETCH_GLOBAL(Qt::TimerType, timerType);
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
    QVERIFY(difference >= 0);        // Should always be true, but just in case
    QVERIFY(difference <= callTimer.nsecsElapsed()); // Ideally difference should be 0 exactly

    // Make sure setRemainingTime underflows gracefully
    deadline.setPreciseRemainingTime(std::numeric_limits<qint64>::min() / 10, 0, timerType);
    QVERIFY(!deadline.isForever());     // On Win/macOS the above underflows, make sure we don't saturate to Forever
    QVERIFY(deadline.remainingTime() == 0);
    // If the timer is saturated we don't want to get a valid number of milliseconds
    QVERIFY(deadline.deadline() == std::numeric_limits<qint64>::min());

    // Check that the conversion to milliseconds and nanoseconds underflows gracefully
    deadline.setPreciseDeadline(std::numeric_limits<qint64>::min() / 10, 0, timerType);
    QVERIFY(!deadline.isForever());     // On Win/macOS the above underflows, make sure we don't saturate to Forever
    QVERIFY(deadline.deadline() == std::numeric_limits<qint64>::min());
    QVERIFY(deadline.deadlineNSecs() == std::numeric_limits<qint64>::min());
}

void tst_QDeadlineTimer::expire()
{
    QFETCH_GLOBAL(Qt::TimerType, timerType);

    QDeadlineTimer deadline(minResolution, timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());

    qint64 previousDeadline = deadline.deadlineNSecs();

    QTest::qSleep(2 * minResolution);

    QCOMPARE(deadline.remainingTime(), qint64(0));
    QCOMPARE(deadline.remainingTimeNSecs(), qint64(0));
    QVERIFY(deadline.deadline() != 0);
    QVERIFY(deadline.deadline() != std::numeric_limits<qint64>::max());
    QVERIFY(deadline.deadlineNSecs() != 0);
    QVERIFY(deadline.deadlineNSecs() != std::numeric_limits<qint64>::max());
    QCOMPARE(deadline.deadlineNSecs(), previousDeadline);
}

void tst_QDeadlineTimer::stdchrono()
{
#if !__has_include(<chrono>)
    QSKIP("std::chrono not found on this system");
#else
    using namespace std::chrono;
    QFETCH_GLOBAL(Qt::TimerType, timerType);

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

    /*
        Call QTest::qSleep, and return true if the time actually slept is
        within \a deviationPercent percent of the requested sleep time.
        Otherwise, return false, in which case the test should to abort.
    */
    auto sleepHelper = [](int ms, int deviationPercent = 10) -> bool {
        auto before = steady_clock::now();
        QTest::qSleep(ms);
        auto after = steady_clock::now();
        auto diff = duration_cast<milliseconds>(after - before).count();
        bool inRange = qAbs(diff - ms) < ms * deviationPercent/100.0;
        if (!inRange)
            qWarning() << "sleeping" << diff << "instead of" << ms << inRange;
        return inRange;
    };

    auto steady_before = steady_clock::now();
    auto system_before = system_clock::now();

    if (!sleepHelper(minResolution))
        QSKIP("Slept too long");
    auto now = QDeadlineTimer::current(timerType);
    auto steady_reference = steady_clock::now();
    auto system_reference = system_clock::now();
    if (!sleepHelper(minResolution))
        QSKIP("Slept too long");

    auto sampling_start = steady_clock::now();
    auto steady_deadline = now.deadline<steady_clock>();
    auto system_deadline = now.deadline<system_clock>();
    auto steady_after = steady_clock::now();
    auto system_after = system_clock::now();
    auto sampling_end = steady_clock::now();

    auto sampling_diff = duration_cast<milliseconds>(sampling_end - sampling_start).count();
    if (sampling_diff > minResolution/2) {
        qWarning() << "Sampling clock took" << sampling_diff << "ms";
        QSKIP("Sampling clock took too long, aborting test", Abort);
    }
    auto total_diff = duration_cast<milliseconds>(steady_after - steady_before).count();
    if (total_diff >= 3*minResolution) {
        qWarning() << "Measurement took" << total_diff << "ms";
        QSKIP("Measurement took too long, aborting test", Abort);
    }

    {
        auto reference = duration_cast<milliseconds>(steady_after - steady_reference).count();
        auto diff = duration_cast<milliseconds>(steady_after - steady_deadline).count();
        QVERIFY2(diff > reference * 0.9 && diff < reference*1.1, QByteArray::number(qint64(diff)));
        QDeadlineTimer dt_after(steady_after, timerType);
        QVERIFY2(now < dt_after,
                 ("now = " + QLocale().toString(now.deadlineNSecs()) +
                 "; after = " + QLocale().toString(dt_after.deadlineNSecs())).toLatin1());

        reference = duration_cast<milliseconds>(steady_reference - steady_before).count();
        diff = duration_cast<milliseconds>(steady_deadline - steady_before).count();
        QVERIFY2(diff > reference * 0.9 && diff < reference*1.1, QByteArray::number(qint64(diff)));
        QDeadlineTimer dt_before(steady_before, timerType);
        QVERIFY2(now > dt_before,
                 ("now = " + QLocale().toString(now.deadlineNSecs()) +
                 "; before = " + QLocale().toString(dt_before.deadlineNSecs())).toLatin1());
    }
    {
        auto reference = duration_cast<milliseconds>(system_after - system_reference).count();
        auto diff = duration_cast<milliseconds>(system_after - system_deadline).count();
        QVERIFY2(diff > reference * 0.9 && diff < reference*1.1, QByteArray::number(qint64(diff)));        QDeadlineTimer dt_after(system_after, timerType);
        QVERIFY2(now < dt_after,
                 ("now = " + QLocale().toString(now.deadlineNSecs()) +
                 "; after = " + QLocale().toString(dt_after.deadlineNSecs())).toLatin1());

        reference = duration_cast<milliseconds>(system_reference - system_before).count();
        diff = duration_cast<milliseconds>(steady_deadline - steady_before).count();
        QVERIFY2(diff > reference * 0.9 && diff < reference*1.1, QByteArray::number(qint64(diff)));        QDeadlineTimer dt_before(system_before, timerType);
        QVERIFY2(now > dt_before,
                 ("now = " + QLocale().toString(now.deadlineNSecs()) +
                 "; before = " + QLocale().toString(dt_before.deadlineNSecs())).toLatin1());
    }

    // make it regular
    now = QDeadlineTimer::current(timerType);
    deadline.setRemainingTime(milliseconds(4 * minResolution), timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(deadline.remainingTimeAsDuration() > milliseconds(3 * minResolution));
    QVERIFY(deadline.remainingTimeAsDuration() < milliseconds(5 * minResolution));
    QVERIFY(deadline.remainingTimeAsDuration() > nanoseconds(3000000 * minResolution));
    QVERIFY(deadline.remainingTimeAsDuration() < nanoseconds(5000000 * minResolution));
    QVERIFY(deadline.deadline<steady_clock>() > (steady_clock::now() + milliseconds(3 * minResolution)));
    QVERIFY(deadline.deadline<steady_clock>() < (steady_clock::now() + milliseconds(5 * minResolution)));
    QVERIFY(deadline.deadline<system_clock>() > (system_clock::now() + milliseconds(3 * minResolution)));
    QVERIFY(deadline.deadline<system_clock>() < (system_clock::now() + milliseconds(5 * minResolution)));
    if (timerType == Qt::CoarseTimer) {
        QVERIFY(deadline > (now + milliseconds(3 * minResolution)));
        QVERIFY(deadline < (now + milliseconds(5 * minResolution)));
        QVERIFY(deadline > (now + nanoseconds(3000000 * minResolution)));
        QVERIFY(deadline < (now + nanoseconds(5000000 * minResolution)));
        QVERIFY(deadline > milliseconds(3 * minResolution));
        QVERIFY(deadline < milliseconds(5 * minResolution));
        QVERIFY(deadline > nanoseconds(3000000 * minResolution));
        QVERIFY(deadline < nanoseconds(5000000 * minResolution));
        QVERIFY(deadline >= steady_clock::now());
        QVERIFY(deadline >= system_clock::now());
    }

    now = QDeadlineTimer::current(timerType);
    deadline = QDeadlineTimer(seconds(1), timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(deadline.remainingTimeAsDuration() > (seconds(1) - milliseconds(minResolution)));
    QVERIFY(deadline.remainingTimeAsDuration() <= seconds(1));
    QVERIFY(deadline.deadline<steady_clock>() > (steady_clock::now() + seconds(1) - milliseconds(minResolution)));
    QVERIFY(deadline.deadline<steady_clock>() <= (steady_clock::now() + seconds(1) + milliseconds(minResolution)));
    QVERIFY(deadline.deadline<system_clock>() > (system_clock::now() + seconds(1) - milliseconds(minResolution)));
    QVERIFY(deadline.deadline<system_clock>() <= (system_clock::now() + seconds(1) + milliseconds(minResolution)));
    if (timerType == Qt::CoarseTimer) {
        QVERIFY(deadline > (seconds(1) - milliseconds(minResolution)));
        QVERIFY(deadline <= seconds(1));
    }

    now = QDeadlineTimer::current(timerType);
    deadline.setRemainingTime(hours(1), timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(deadline.remainingTimeAsDuration() > (hours(1) - milliseconds(minResolution)));
    QVERIFY(deadline.remainingTimeAsDuration() <= hours(1));
    QVERIFY(deadline.deadline<steady_clock>() > (steady_clock::now() + hours(1) - milliseconds(minResolution)));
    QVERIFY(deadline.deadline<steady_clock>() <= (steady_clock::now() + hours(1) + milliseconds(minResolution)));
    QVERIFY(deadline.deadline<system_clock>() > (system_clock::now() + hours(1) - milliseconds(minResolution)));
    QVERIFY(deadline.deadline<system_clock>() <= (system_clock::now() + hours(1) + milliseconds(minResolution)));

    now = QDeadlineTimer::current(timerType);
    deadline.setDeadline(system_clock::now() + seconds(1), timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(deadline.remainingTimeAsDuration() > (seconds(1) - milliseconds(minResolution)));
    QVERIFY(deadline.remainingTimeAsDuration() <= seconds(1));
    QVERIFY(deadline.deadline<steady_clock>() > (steady_clock::now() + seconds(1) - milliseconds(minResolution)));
    QVERIFY(deadline.deadline<steady_clock>() <= (steady_clock::now() + seconds(1) + milliseconds(minResolution)));
    QVERIFY(deadline.deadline<system_clock>() > (system_clock::now() + seconds(1) - milliseconds(minResolution)));
    QVERIFY(deadline.deadline<system_clock>() <= (system_clock::now() + seconds(1) + milliseconds(minResolution)));

    now = QDeadlineTimer::current(timerType);
    deadline.setDeadline(steady_clock::now() + seconds(1), timerType);
    QVERIFY(!deadline.hasExpired());
    QVERIFY(!deadline.isForever());
    QCOMPARE(deadline.timerType(), timerType);
    QVERIFY(deadline.remainingTimeAsDuration() > (seconds(1) - milliseconds(minResolution)));
    QVERIFY(deadline.remainingTimeAsDuration() <= seconds(1));
    QVERIFY(deadline.deadline<steady_clock>() > (steady_clock::now() + seconds(1) - milliseconds(minResolution)));
    QVERIFY(deadline.deadline<steady_clock>() <= (steady_clock::now() + seconds(1) + milliseconds(minResolution)));
    QVERIFY(deadline.deadline<system_clock>() > (system_clock::now() + seconds(1) - milliseconds(minResolution)));
    QVERIFY(deadline.deadline<system_clock>() <= (system_clock::now() + seconds(1) + milliseconds(minResolution)));
#endif
}

QTEST_MAIN(tst_QDeadlineTimer)

#include "tst_qdeadlinetimer.moc"
