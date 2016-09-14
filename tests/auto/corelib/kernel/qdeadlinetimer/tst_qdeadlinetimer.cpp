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
#include <QtTest/QtTest>

#if QT_HAS_INCLUDE(<chrono>)
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
#if !QT_HAS_INCLUDE(<chrono>)
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

    auto steady_before = steady_clock::now();
    auto system_before = system_clock::now();

    QTest::qSleep(minResolution);
    auto now = QDeadlineTimer::current(timerType);
    QTest::qSleep(minResolution);

    auto steady_after = steady_clock::now();
    auto system_after = system_clock::now();

    {
        auto diff = duration_cast<milliseconds>(steady_after - now.deadline<steady_clock>());
        QVERIFY2(diff.count() > minResolution/2, QByteArray::number(qint64(diff.count())));
        QVERIFY2(diff.count() < 3*minResolution/2, QByteArray::number(qint64(diff.count())));
        QDeadlineTimer dt_after(steady_after, timerType);
        QVERIFY2(now < dt_after,
                 ("now = " + QLocale().toString(now.deadlineNSecs()) +
                 "; after = " + QLocale().toString(dt_after.deadlineNSecs())).toLatin1());

        diff = duration_cast<milliseconds>(now.deadline<steady_clock>() - steady_before);
        QVERIFY2(diff.count() > minResolution/2, QByteArray::number(qint64(diff.count())));
        QVERIFY2(diff.count() < 3*minResolution/2, QByteArray::number(qint64(diff.count())));
        QDeadlineTimer dt_before(steady_before, timerType);
        QVERIFY2(now > dt_before,
                 ("now = " + QLocale().toString(now.deadlineNSecs()) +
                 "; before = " + QLocale().toString(dt_before.deadlineNSecs())).toLatin1());
    }
    {
        auto diff = duration_cast<milliseconds>(system_after - now.deadline<system_clock>());
        QVERIFY2(diff.count() > minResolution/2, QByteArray::number(qint64(diff.count())));
        QVERIFY2(diff.count() < 3*minResolution/2, QByteArray::number(qint64(diff.count())));
        QDeadlineTimer dt_after(system_after, timerType);
        QVERIFY2(now < dt_after,
                 ("now = " + QLocale().toString(now.deadlineNSecs()) +
                 "; after = " + QLocale().toString(dt_after.deadlineNSecs())).toLatin1());

        diff = duration_cast<milliseconds>(now.deadline<system_clock>() - system_before);
        QVERIFY2(diff.count() > minResolution/2, QByteArray::number(qint64(diff.count())));
        QVERIFY2(diff.count() < 3*minResolution/2, QByteArray::number(qint64(diff.count())));
        QDeadlineTimer dt_before(system_before, timerType);
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
