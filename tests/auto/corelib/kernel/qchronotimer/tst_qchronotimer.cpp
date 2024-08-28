// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifdef QT_GUI_LIB
#  include <QtGui/QGuiApplication>
#else
#  include <QtCore/QCoreApplication>
#endif

#include <QtCore/private/qglobal_p.h>
#include <QTest>
#include <QSignalSpy>
#include <QtTest/private/qpropertytesthelper_p.h>

#include <qbasictimer.h>
#include <qchronotimer.h>
#include <qthread.h>
#include <qtimer.h>
#include <qelapsedtimer.h>
#include <qproperty.h>

#if defined Q_OS_UNIX
#include <unistd.h>
#endif

using namespace std::chrono_literals;

#ifdef DISABLE_GLIB
static bool glibDisabled = []() {
    qputenv("QT_NO_GLIB", "1");
    return true;
}();
#endif

class tst_QChronoTimer : public QObject
{
    Q_OBJECT

private slots:
    void zeroTimer();
    void singleShotTimeout(); // Non-static singleShot()
    void timeout();
    void sequentialTimers_data();
    void sequentialTimers();
    void remainingTime();
    void remainingTimeInitial_data();
    void remainingTimeInitial();
    void remainingTimeDuringActivation_data();
    void remainingTimeDuringActivation();
    void basic_chrono();
    void livelock_data();
    void livelock();
    void timerInfiniteRecursion_data();
    void timerInfiniteRecursion();
    void recurringTimer_data();
    void recurringTimer();
    void deleteLaterOnQChronoTimer(); // long name, don't want to shadow QObject::deleteLater()
    void moveToThread();
    void restartedTimerFiresTooSoon();
    void timerFiresOnlyOncePerProcessEvents_data();
    void timerFiresOnlyOncePerProcessEvents();
    void timerIdPersistsAfterThreadExit();
    void cancelLongTimer();
    void recurseOnTimeoutAndStopTimer();
    void timerOrder();
    void timerOrder_data();
    void timerOrderBackgroundThread();
    void timerOrderBackgroundThread_data() { timerOrder_data(); }
    void timerPrecision();

    void dontBlockEvents();
    void postedEventsShouldNotStarveTimers();
    void callOnTimeout();

    void bindToTimer();
    void bindTimer();
    void automatedBindingTests();

    void negativeInterval();
};

void tst_QChronoTimer::zeroTimer()
{
    QChronoTimer timer;
    QVERIFY(!timer.isSingleShot());
    timer.setInterval(0ns);
    timer.setSingleShot(true);
    QVERIFY(timer.isSingleShot());

    QSignalSpy timeoutSpy(&timer, &QChronoTimer::timeout);
    timer.start();

    // Pass timeout to work round glib issue, see QTBUG-84291.
    QCoreApplication::processEvents(QEventLoop::AllEvents, INT_MAX);

    QCOMPARE(timeoutSpy.size(), 1);
}

void tst_QChronoTimer::singleShotTimeout()
{
    QChronoTimer timer;
    QVERIFY(!timer.isSingleShot());
    timer.setSingleShot(true);
    QVERIFY(timer.isSingleShot());

    QSignalSpy timeoutSpy(&timer, &QChronoTimer::timeout);
    timer.setInterval(100ms);
    timer.start();

    QVERIFY(timeoutSpy.wait(500ms));
    QCOMPARE(timeoutSpy.size(), 1);
    QTest::qWait(500ms);
    QCOMPARE(timeoutSpy.size(), 1);
}

static constexpr auto Timeout_Interval = 200ms;

void tst_QChronoTimer::timeout()
{
    QChronoTimer timer{100ms};
    QSignalSpy timeoutSpy(&timer, &QChronoTimer::timeout);
    timer.start();

    QCOMPARE(timeoutSpy.size(), 0);

    QTRY_VERIFY_WITH_TIMEOUT(timeoutSpy.size() > 0, Timeout_Interval);
    const qsizetype oldCount = timeoutSpy.size();

    QTRY_VERIFY_WITH_TIMEOUT(timeoutSpy.size() > oldCount, Timeout_Interval);
}

void tst_QChronoTimer::sequentialTimers_data()
{
#ifdef Q_OS_WIN
    QSKIP("The API used by QEventDispatcherWin32 doesn't respect the order");
#endif
    QTest::addColumn<QList<std::chrono::milliseconds>>("timeouts");
    auto addRow = [](const QList<std::chrono::milliseconds> &l) {
        Q_ASSERT_X(std::is_sorted(l.begin(), l.end()),
                   "tst_QChronoTimer", "input list must be sorted");
        QByteArray name;
        for (auto msec : l)
            name += QByteArray::number(msec.count()) + ',';
        name.chop(1);
        QTest::addRow("%s", name.constData()) << l;
    };
    // PreciseTimers
    addRow({0ms, 0ms, 0ms, 0ms, 0ms, 0ms});
    addRow({0ms, 1ms, 2ms});
    addRow({1ms, 1ms, 1ms, 2ms, 2ms, 2ms, 2ms});
    addRow({1ms, 2ms, 3ms});
    addRow({19ms, 19ms, 19ms});
    // CoarseTimer for setinterval
    addRow({20ms, 20ms, 20ms, 20ms, 20ms});
    addRow({25ms, 25ms, 25ms, 25ms, 25ms, 25ms, 50ms});
}

void tst_QChronoTimer::sequentialTimers()
{
    QFETCH(const QList<std::chrono::milliseconds>, timeouts);
    QByteArray result, expected;
    std::vector<std::unique_ptr<QChronoTimer>> timers;
    expected.resize(timeouts.size());
    result.reserve(timeouts.size());
    timers.reserve(timeouts.size());
    for (int i = 0; i < timeouts.size(); ++i) {
        auto timer = std::make_unique<QChronoTimer>(timeouts[i]);
        timer->setSingleShot(true);

        char c = 'A' + i;
        expected[i] = c;
        QObject::connect(timer.get(), &QChronoTimer::timeout, this, [&result, c = c]() {
            result.append(c);
        });
        timers.push_back(std::move(timer));
    }

    // start the timers
    for (auto &timer : timers)
        timer->start();

    QTestEventLoop::instance().enterLoop(timeouts.last() * 2 + 10ms);

    QCOMPARE(result, expected);
}

void tst_QChronoTimer::remainingTime()
{
    QChronoTimer tested;
    tested.setTimerType(Qt::PreciseTimer);

    QChronoTimer tester;
    tester.setTimerType(Qt::PreciseTimer);
    tester.setSingleShot(true);

    constexpr auto tested_interval = 200ms;
    constexpr auto tester_interval = 50ms;
    constexpr auto expectedRemainingTime = tested_interval - tester_interval;

    int testIteration = 0;
    const int desiredTestCount = 2;

    // We let tested (which isn't a single-shot) run repeatedly, to verify
    // it *does* repeat, and check that the single-shot tester, starting
    // at the same time, does finish first each time, by about the right duration.
    auto connection = QObject::connect(&tested, &QChronoTimer::timeout,
                                       &tester, &QChronoTimer::start);

    QObject::connect(&tester, &QChronoTimer::timeout, this, [&]() {
        const std::chrono::nanoseconds remainingTime = tested.remainingTime();
        // We expect that remainingTime is at most 150 and not overdue.
        const bool remainingTimeInRange = remainingTime > 0ns
                                       && remainingTime <= expectedRemainingTime;
        if (remainingTimeInRange)
            ++testIteration;
        else
            testIteration = desiredTestCount; // We are going to fail on QVERIFY2()
                                              // below, so we don't want to iterate
                                              // anymore and quickly exit the QTRY_...()
                                              // with this failure.
        if (testIteration == desiredTestCount)
            QObject::disconnect(connection); // Last iteration, don't start tester again.
        QVERIFY2(remainingTimeInRange, qPrintable("Remaining time "
                 + QByteArray::number(remainingTime.count()) + "ms outside expected range (0ns, "
                 + QByteArray::number(expectedRemainingTime.count()) + "ms]"));
    });

    tested.setInterval(tested_interval);
    tested.start();
    tester.setInterval(tester_interval);
    tester.start(); // Start tester for the 1st time.

    // Test it desiredTestCount times, give it reasonable amount of time
    // (twice as much as needed).
    const auto tryTimeout = tested_interval * desiredTestCount * 2;
    QTRY_COMPARE_WITH_TIMEOUT(testIteration, desiredTestCount, tryTimeout);
}

void tst_QChronoTimer::remainingTimeInitial_data()
{
    using namespace std::chrono;

    QTest::addColumn<nanoseconds>("startTimeNs");
    QTest::addColumn<Qt::TimerType>("timerType");

    QTest::addRow("precisetiemr-0ns") << 0ns << Qt::PreciseTimer;
    QTest::addRow("precisetimer-1ms") << nanoseconds{1ms} << Qt::PreciseTimer;
    QTest::addRow("precisetimer-10ms") <<nanoseconds{10ms} << Qt::PreciseTimer;

    QTest::addRow("coarsetimer-0ns") << 0ns << Qt::CoarseTimer;
    QTest::addRow("coarsetimer-1ms") << nanoseconds{1ms} << Qt::CoarseTimer;
    QTest::addRow("coarsetimer-10ms") << nanoseconds{10ms} << Qt::CoarseTimer;
}

void tst_QChronoTimer::remainingTimeInitial()
{
    QFETCH(std::chrono::nanoseconds, startTimeNs);
    QFETCH(Qt::TimerType, timerType);

    QChronoTimer timer;
    QCOMPARE(timer.timerType(), Qt::CoarseTimer);
    timer.setTimerType(timerType);
    QCOMPARE(timer.timerType(), timerType);
    timer.setInterval(startTimeNs);
    timer.start();

    const std::chrono::nanoseconds rt = timer.remainingTime();
    QCOMPARE_GE(rt, 0ns);
    QCOMPARE_LE(rt, startTimeNs);
}

void tst_QChronoTimer::remainingTimeDuringActivation_data()
{
    QTest::addColumn<bool>("singleShot");
    QTest::newRow("repeating") << false;
    QTest::newRow("single-shot") << true;
}

void tst_QChronoTimer::remainingTimeDuringActivation()
{
    QFETCH(bool, singleShot);

    QChronoTimer timer;
    timer.setSingleShot(singleShot);

    auto remainingTime = 0ns; // not the expected value in either case
    connect(&timer, &QChronoTimer::timeout, this, [&]() { remainingTime = timer.remainingTime(); });
    QSignalSpy timeoutSpy(&timer, &QChronoTimer::timeout);
    // 20 ms is short enough and should not round down to 0 in any timer mode
    constexpr auto timeout = 20ms;
    timer.setInterval(timeout);
    timer.start();

    QVERIFY(timeoutSpy.wait());
    if (singleShot)
        QCOMPARE_LT(remainingTime, 0ns); // timer not running
    else {
        QCOMPARE_LE(remainingTime, timeout);
        QCOMPARE_GT(remainingTime, 0ns);
    }

    if (!singleShot) {
        // do it again - see QTBUG-46940
        remainingTime = std::chrono::milliseconds::min();
        QVERIFY(timeoutSpy.wait());
        QCOMPARE_LE(remainingTime, timeout);
        QCOMPARE_GT(remainingTime, 0ns);
    }
}

namespace {
    template <typename T>
    auto to_ms(T t)
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(t);
    }
} // unnamed namespace

void tst_QChronoTimer::basic_chrono()
{
    // duplicates zeroTimer, singleShotTimeout, interval and remainingTime
    using namespace std::chrono;
    QChronoTimer timer;
    QSignalSpy timeoutSpy(&timer, &QChronoTimer::timeout);
    timer.start();
    QCOMPARE(timer.interval(), 0ns);
    QCOMPARE(timer.remainingTime(), 0ns);

    QCoreApplication::processEvents();

    QCOMPARE(timeoutSpy.size(), 1);

    timeoutSpy.clear();
    timer.setInterval(100ms);
    timer.start();
    QCOMPARE(timeoutSpy.size(), 0);

    QVERIFY(timeoutSpy.wait(Timeout_Interval));
    QVERIFY(timeoutSpy.size() > 0);
    const qsizetype oldCount = timeoutSpy.size();

    QVERIFY(timeoutSpy.wait(Timeout_Interval));
    QVERIFY(timeoutSpy.size() > oldCount);

    timeoutSpy.clear();
    timer.setInterval(200ms);
    timer.start();
    QCOMPARE(timer.interval(), 200ms);
    QTest::qWait(50ms);
    QCOMPARE(timeoutSpy.size(), 0);

    nanoseconds rt = timer.remainingTime();
    QCOMPARE_GE(rt, 50ms);
    QCOMPARE_LE(rt, 200ms);

    timeoutSpy.clear();
    timer.setSingleShot(true);
    timer.setInterval(100ms);
    timer.start();
    QVERIFY(timeoutSpy.wait(Timeout_Interval));
    QCOMPARE(timeoutSpy.size(), 1);
    QTest::qWait(500ms);
    QCOMPARE(timeoutSpy.size(), 1);
}

void tst_QChronoTimer::livelock_data()
{
    QTest::addColumn<std::chrono::nanoseconds>("interval");
    QTest::newRow("zero-timer") << 0ns;
    QTest::newRow("non-zero-timer") << std::chrono::nanoseconds{1ms};
    QTest::newRow("longer-than-sleep") << std::chrono::nanoseconds{20ms};
}

/*!
 *
 * DO NOT "FIX" THIS TEST!  it is written like this for a reason, do
 * not *change it without first dicussing it with its maintainers.
 *
*/
class LiveLockTester : public QObject
{
    static constexpr QEvent::Type PostEventType = static_cast<QEvent::Type>(4002);
public:
    LiveLockTester(std::chrono::nanoseconds i)
        : interval(i)
    {
        firstTimerId = startTimer(interval);
        extraTimerId = startTimer(interval + 80ms);
        secondTimerId = -1; // started later
    }

    bool event(QEvent *e) override
    {
        if (e->type() == PostEventType) {
            // got the posted event
            if (timeoutsForFirst == 1 && timeoutsForSecond == 0)
                postEventAtRightTime = true;
            return true;
        }
        return QObject::event(e);
    }

    void timerEvent(QTimerEvent *te) override
    {
        if (te->timerId() == firstTimerId) {
            if (++timeoutsForFirst == 1) {
                killTimer(extraTimerId);
                extraTimerId = -1;
                QCoreApplication::postEvent(this, new QEvent(PostEventType));
                secondTimerId = startTimer(interval);
            }
        } else if (te->timerId() == secondTimerId) {
            ++timeoutsForSecond;
        } else if (te->timerId() == extraTimerId) {
            ++timeoutsForExtra;
        }

        // sleep for 2ms
        QTest::qSleep(2);
        killTimer(te->timerId());
    }

    const std::chrono::nanoseconds interval;
    int firstTimerId = -1;
    int secondTimerId = -1;
    int extraTimerId = -1;
    int timeoutsForFirst = 0;
    int timeoutsForExtra = 0;
    int timeoutsForSecond = 0;
    bool postEventAtRightTime = false;
};

void tst_QChronoTimer::livelock()
{
    /*
      New timers created in timer event handlers should not be sent
      until the next iteration of the eventloop.  Note: this test
      depends on the fact that we send posted events before timer
      events (since new posted events are not sent until the next
      iteration of the eventloop either).
    */
    QFETCH(std::chrono::nanoseconds, interval);
    LiveLockTester tester(interval);
    QTest::qWait(180ms); // we have to use wait here, since we're testing timers with a non-zero timeout
    QTRY_COMPARE(tester.timeoutsForFirst, 1);
    QCOMPARE(tester.timeoutsForExtra, 0);
    QTRY_COMPARE(tester.timeoutsForSecond, 1);
    QVERIFY(tester.postEventAtRightTime);
}

class TimerInfiniteRecursionObject : public QObject
{
public:
    bool inTimerEvent = false;
    bool timerEventRecursed = false;
    std::chrono::nanoseconds interval;

    TimerInfiniteRecursionObject(std::chrono::nanoseconds interval)
        : interval(interval)
    { }

    void timerEvent(QTimerEvent *timerEvent) override
    {
        timerEventRecursed = inTimerEvent;
        if (timerEventRecursed) {
            // bug detected!
            return;
        }

        inTimerEvent = true;

        QEventLoop eventLoop;
        QTimer::singleShot(std::max<std::chrono::nanoseconds>(100ms, interval * 2),
                           &eventLoop, &QEventLoop::quit);
        eventLoop.exec();

        inTimerEvent = false;

        killTimer(timerEvent->timerId());
    }
};

void tst_QChronoTimer::timerInfiniteRecursion_data()
{
    QTest::addColumn<std::chrono::nanoseconds>("interval");
    QTest::newRow("zero timer") << 0ns;
    QTest::newRow("non-zero timer") << std::chrono::nanoseconds{1ms};
    QTest::newRow("10ms timer") << std::chrono::nanoseconds{10ms};
    QTest::newRow("11ms timer") << std::chrono::nanoseconds{11ms};
    QTest::newRow("100ms timer") << std::chrono::nanoseconds{100ms};
    QTest::newRow("1s timer") << std::chrono::nanoseconds{1000ms};
}


void tst_QChronoTimer::timerInfiniteRecursion()
{
    QFETCH(std::chrono::nanoseconds, interval);
    TimerInfiniteRecursionObject object(interval);
    (void) object.startTimer(interval);

    QEventLoop eventLoop;
    QTimer::singleShot(std::max<std::chrono::nanoseconds>(100ms, interval * 2),
                       &eventLoop, &QEventLoop::quit);
    eventLoop.exec();

    QVERIFY(!object.timerEventRecursed);
}

class RecurringTimerObject : public QObject
{
Q_OBJECT
public:
    int times;
    int target;
    bool recurse;

    RecurringTimerObject(int target)
        : times(0), target(target), recurse(false)
    { }

    void timerEvent(QTimerEvent *timerEvent) override
    {
        if (++times == target) {
            killTimer(timerEvent->timerId());
            Q_EMIT done();
        } if (recurse) {
            QEventLoop eventLoop;
            QTimer::singleShot(100ms, &eventLoop, &QEventLoop::quit);
            eventLoop.exec();
        }
    }

signals:
    void done();
};

void tst_QChronoTimer::recurringTimer_data()
{
    QTest::addColumn<std::chrono::nanoseconds>("interval");
    QTest::addColumn<bool>("recurse");
    // make sure that eventloop recursion doesn't affect timer recurrence
    QTest::newRow("zero timer, don't recurse") << 0ns << false;
    QTest::newRow("zero timer, recurse") << 0ns << true;
    QTest::newRow("non-zero timer, don't recurse") << std::chrono::nanoseconds{1ms} << false;
    QTest::newRow("non-zero timer, recurse") << std::chrono::nanoseconds{1ms} << true;
}

void tst_QChronoTimer::recurringTimer()
{
    const int target = 5;
    QFETCH(std::chrono::nanoseconds, interval);
    QFETCH(bool, recurse);

    RecurringTimerObject object(target);
    object.recurse = recurse;
    QSignalSpy doneSpy(&object, &RecurringTimerObject::done);

    (void) object.startTimer(interval);
    QVERIFY(doneSpy.wait());

    QCOMPARE(object.times, target);
}

void tst_QChronoTimer::deleteLaterOnQChronoTimer()
{
    QChronoTimer *timer = new QChronoTimer;
    connect(timer, &QChronoTimer::timeout, timer, &QObject::deleteLater);
    QSignalSpy destroyedSpy(timer, &QObject::destroyed);
    timer->setInterval(1ms);
    timer->setSingleShot(true);
    timer->start();
    QPointer<QChronoTimer> pointer = timer;
    QVERIFY(destroyedSpy.wait());
    QVERIFY(pointer.isNull());
}

namespace {
int operator&(Qt::TimerId id, int i) { return qToUnderlying(id) & i; }
}

static constexpr auto MoveToThread_Timeout = 200ms;
static constexpr auto MoveToThread_Wait = 300ms;

void tst_QChronoTimer::moveToThread()
{
#if defined(Q_OS_WIN32)
    QSKIP("Does not work reliably on Windows :(");
#elif defined(Q_OS_MACOS)
    QSKIP("Does not work reliably on macOS 10.12+ (QTBUG-59679)");
#endif
    QChronoTimer timer1{MoveToThread_Timeout};
    QChronoTimer timer2{MoveToThread_Timeout};
    timer1.setSingleShot(true);
    timer1.start();
    timer2.start();
    QVERIFY((timer1.id() & 0xffffff) != (timer2.id() & 0xffffff));
    QThread tr;
    timer1.moveToThread(&tr);
    connect(&timer1, &QChronoTimer::timeout, &tr, &QThread::quit);
    tr.start();
    QChronoTimer ti3{MoveToThread_Timeout};
    ti3.start();
    QVERIFY((ti3.id() & 0xffffff) != (timer2.id() & 0xffffff));
    QVERIFY((ti3.id() & 0xffffff) != (timer1.id() & 0xffffff));
    QTest::qWait(MoveToThread_Wait);
    QVERIFY(tr.wait());
    timer2.stop();
    QChronoTimer ti4{MoveToThread_Timeout};
    ti4.start();
    ti3.stop();
    timer2.setInterval(MoveToThread_Timeout);
    timer2.start();
    ti3.setInterval(MoveToThread_Timeout);
    ti3.start();
    QVERIFY((ti4.id() & 0xffffff) != (timer2.id() & 0xffffff));
    QVERIFY((ti3.id() & 0xffffff) != (timer2.id() & 0xffffff));
    QVERIFY((ti3.id() & 0xffffff) != (timer1.id() & 0xffffff));
}

class RestartedTimerFiresTooSoonObject : public QObject
{
    Q_OBJECT

public:
    QBasicTimer m_timer;

    std::chrono::milliseconds m_interval = 0ms;
    QElapsedTimer m_elapsedTimer;
    QEventLoop eventLoop;

    RestartedTimerFiresTooSoonObject() = default;

    void timerFired()
    {
        static std::chrono::milliseconds interval = 1s;

        m_interval = interval;
        m_elapsedTimer.start();
        m_timer.start(interval, this);

        // alternate between single-shot and 1 sec
        interval = interval > 0ms ? 0ms : 1s;
    }

    void timerEvent(QTimerEvent* ev) override
    {
        if (ev->timerId() != m_timer.timerId())
            return;

        m_timer.stop();

        std::chrono::nanoseconds elapsed = m_elapsedTimer.durationElapsed();

        if (elapsed < m_interval / 2) {
            // severely too early!
            m_timer.stop();
            eventLoop.exit(-1);
            return;
        }

        timerFired();

        // don't do this forever
        static int count = 0;
        if (count++ > 20) {
            m_timer.stop();
            eventLoop.quit();
            return;
        }
    }
};

void tst_QChronoTimer::restartedTimerFiresTooSoon()
{
    RestartedTimerFiresTooSoonObject object;
    object.timerFired();
    QCOMPARE(object.eventLoop.exec(), 0);
}

class LongLastingSlotClass : public QObject
{
    Q_OBJECT

public:
    LongLastingSlotClass(QChronoTimer *timer) : timer(timer) { }

public slots:
    void longLastingSlot()
    {
        // Don't use QChronoTimer for this, because we are testing it.
        QElapsedTimer control;
        control.start();
        while (control.durationElapsed() < 200ms) {
            for (int c = 0; c < 100'000; c++) {} // Mindless looping.
        }
        if (++count >= 2) {
            timer->stop();
        }
    }

public:
    int count = 0;
    QChronoTimer *timer;
};

void tst_QChronoTimer::timerFiresOnlyOncePerProcessEvents_data()
{
    QTest::addColumn<std::chrono::nanoseconds>("interval");
    QTest::newRow("zero-timer") << 0ns;
    QTest::newRow("non-zero-timer") << std::chrono::nanoseconds{10ms};
}

void tst_QChronoTimer::timerFiresOnlyOncePerProcessEvents()
{
    QFETCH(std::chrono::nanoseconds, interval);

    QChronoTimer t{interval};
    LongLastingSlotClass longSlot(&t);
    t.start();
    connect(&t, &QChronoTimer::timeout, &longSlot, &LongLastingSlotClass::longLastingSlot);
    // Loop because there may be other events pending.
    while (longSlot.count == 0)
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);

    QCOMPARE(longSlot.count, 1);
}

class TimerIdPersistsAfterThreadExitThread : public QThread
{
public:
    std::unique_ptr<QChronoTimer> timer;
    Qt::TimerId timerId = Qt::TimerId::Invalid;
    int returnValue = -1;

    void run() override
    {
        QEventLoop eventLoop;
        timer = std::make_unique<QChronoTimer>();
        connect(timer.get(), &QChronoTimer::timeout, &eventLoop, &QEventLoop::quit);
        timer->setInterval(100ms);
        timer->start();
        timerId = timer->id();
        returnValue = eventLoop.exec();
    }
};

void tst_QChronoTimer::timerIdPersistsAfterThreadExit()
{
    TimerIdPersistsAfterThreadExitThread thread;
    thread.start();
    QVERIFY(thread.wait(30s));
    QCOMPARE(thread.returnValue, 0);

    // even though the thread has exited, and the event dispatcher destroyed, the timer is still
    // "active", meaning the timer id should NOT be reused (i.e. the event dispatcher should not
    // have unregistered it)
    int timerId = thread.startTimer(100ms);
    QVERIFY((timerId & 0xffffff) != (thread.timerId & 0xffffff));
}

void tst_QChronoTimer::cancelLongTimer()
{
    QChronoTimer timer{1h};
    timer.setSingleShot(true);
    timer.start();
    QCoreApplication::processEvents();
    // If the timer completes immediately with an error, then this will fail
    QVERIFY(timer.isActive());
    timer.stop();
    QVERIFY(!timer.isActive());
}

class TimeoutCounter : public QObject
{
    Q_OBJECT
public slots:
    void timeout() { ++count; };
public:
    int count = 0;
};

class RecursOnTimeoutAndStopTimerTimer : public QObject
{
    Q_OBJECT

public:
    QChronoTimer *one;
    QChronoTimer *two;

public slots:
    void onetrigger()
    {
        QCoreApplication::processEvents();
    }

    void twotrigger()
    {
        one->stop();
    }
};

void tst_QChronoTimer::recurseOnTimeoutAndStopTimer()
{
    QEventLoop eventLoop;
    QTimer::singleShot(1s, &eventLoop, &QEventLoop::quit);

    RecursOnTimeoutAndStopTimerTimer t;
    t.one = new QChronoTimer(&t);
    t.two = new QChronoTimer(&t);

    QObject::connect(t.one, SIGNAL(timeout()), &t, SLOT(onetrigger()));
    QObject::connect(t.two, SIGNAL(timeout()), &t, SLOT(twotrigger()));

    t.two->setSingleShot(true);

    t.one->start();
    t.two->start();

    (void) eventLoop.exec();

    QVERIFY(!t.one->isActive());
    QVERIFY(!t.two->isActive());
}

struct CountedStruct
{
    CountedStruct(int *count, QThread *t = nullptr) : count(count), thread(t) { }
    ~CountedStruct() { }
    void operator()() const { ++(*count); if (thread) QCOMPARE(QThread::currentThread(), thread); }

    int *count;
    QThread *thread;
};

static QScopedPointer<QEventLoop> _e;
static QThread *_t = nullptr;

class StaticEventLoop
{
public:
    static void quitEventLoop()
    {
        quitEventLoop_noexcept();
    }

    static void quitEventLoop_noexcept() noexcept
    {
        QVERIFY(!_e.isNull());
        _e->quit();
        if (_t)
            QCOMPARE(QThread::currentThread(), _t);
    }
};

class DontBlockEvents : public QObject
{
    Q_OBJECT
public:
    DontBlockEvents();
    void timerEvent(QTimerEvent*) override;

    int count;
    int total;
    QBasicTimer m_timer;

public slots:
    void paintEvent();

};

DontBlockEvents::DontBlockEvents()
{
    count = 0;
    total = 0;

    const std::chrono::milliseconds intervals[] = {2s, 2500ms, 3s, 5s, 1s, 2s};
    // need a few unrelated timers running to reproduce the bug.
    for (auto dur : intervals) {
        auto *t = new QChronoTimer(dur, this);
        t->start();
    }

    m_timer.start(1ms, this);
}

void DontBlockEvents::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_timer.timerId()) {
        QMetaObject::invokeMethod(this, &DontBlockEvents::paintEvent, Qt::QueuedConnection);
        m_timer.start(0ms, this);
        count++;
        QCOMPARE(count, 1);
        total++;
    }
}

void DontBlockEvents::paintEvent()
{
    count--;
    QCOMPARE(count, 0);
}

// This is a regression test for QTBUG-13633, where a timer with a zero
// timeout that was restarted by the event handler could starve other timers.
void tst_QChronoTimer::dontBlockEvents()
{
    DontBlockEvents t;
    QTest::qWait(60ms);
    QTRY_VERIFY(t.total > 2);
}

class SlotRepeater : public QObject {
    Q_OBJECT
public:
    SlotRepeater() {}

public slots:
    void repeatThisSlot()
    {
        QMetaObject::invokeMethod(this, &SlotRepeater::repeatThisSlot, Qt::QueuedConnection);
    }
};

void tst_QChronoTimer::postedEventsShouldNotStarveTimers()
{
    QChronoTimer timer;
    timer.setInterval(0ns);
    timer.setSingleShot(false);
    QSignalSpy timeoutSpy(&timer, &QChronoTimer::timeout);
    timer.start();
    SlotRepeater slotRepeater;
    slotRepeater.repeatThisSlot();
    QTRY_VERIFY_WITH_TIMEOUT(timeoutSpy.size() > 5, 100);
}

struct DummyFunctor {
    static QThread *callThread;
    void operator()() {
        callThread = QThread::currentThread();
        callThread->quit();
    }
};
QThread *DummyFunctor::callThread = nullptr;

void tst_QChronoTimer::callOnTimeout()
{
    QChronoTimer timer;
    QSignalSpy timeoutSpy(&timer, &QChronoTimer::timeout);
    timer.start();

    auto context = std::make_unique<QObject>();

    int count = 0;
    timer.callOnTimeout([&count] { count++; });
    QMetaObject::Connection connection = timer.callOnTimeout(context.get(), [&count] { count++; });
    timer.callOnTimeout(&timer, &QChronoTimer::stop);


    QTest::qWait(100ms);
    QCOMPARE(count, 2);
    QCOMPARE(timeoutSpy.size(), 1);

    // Test that connection is bound to context lifetime
    QVERIFY(connection);
    context.reset();
    QVERIFY(!connection);
}

void tst_QChronoTimer::bindToTimer()
{
    QChronoTimer timer;

    // singleShot property
    QProperty<bool> singleShot;
    singleShot.setBinding(timer.bindableSingleShot().makeBinding());
    QCOMPARE(timer.isSingleShot(), singleShot);

    timer.setSingleShot(true);
    QVERIFY(singleShot);
    timer.setSingleShot(false);
    QVERIFY(!singleShot);

    // interval property
    QProperty<std::chrono::nanoseconds> interval;
    interval.setBinding([&](){ return timer.interval(); });
    QCOMPARE(timer.interval(), interval.value());

    timer.setInterval(10ms);
    QCOMPARE(interval.value(), 10ms);
    timer.setInterval(100ms);
    QCOMPARE(interval.value(), 100ms);

    // timerType property
    QProperty<Qt::TimerType> timerType;
    timerType.setBinding(timer.bindableTimerType().makeBinding());
    QCOMPARE(timer.timerType(), timerType);

    timer.setTimerType(Qt::PreciseTimer);
    QCOMPARE(timerType, Qt::PreciseTimer);

    timer.setTimerType(Qt::VeryCoarseTimer);
    QCOMPARE(timerType, Qt::VeryCoarseTimer);

    // active property
    QProperty<bool> active;
    active.setBinding([&](){ return timer.isActive(); });
    QCOMPARE(active, timer.isActive());

    timer.setInterval(1s);
    timer.start();
    QVERIFY(active);

    timer.stop();
    QVERIFY(!active);

    // Also test that using negative interval updates the binding correctly
    timer.setInterval(100ms);
    timer.start();
    QVERIFY(active);

    auto ignoreMsg = [] {
        QTest::ignoreMessage(QtWarningMsg,
                             "QObject::startTimer: Timers cannot have negative intervals");
    };

    ignoreMsg();
    timer.setInterval(-100ms);
    ignoreMsg();
    timer.start();
    QVERIFY(!active);

    timer.setInterval(100ms);
    timer.start();
    QVERIFY(active);

    ignoreMsg();
    timer.setInterval(-100ms);
    ignoreMsg();
    timer.start();
    QVERIFY(!active);
}

void tst_QChronoTimer::bindTimer()
{
    QChronoTimer timer;

    // singleShot property
    QVERIFY(!timer.isSingleShot());

    QProperty<bool> singleShot;
    timer.bindableSingleShot().setBinding(Qt::makePropertyBinding(singleShot));

    singleShot = true;
    QVERIFY(timer.isSingleShot());
    singleShot = false;
    QVERIFY(!timer.isSingleShot());

    // interval property
    QCOMPARE(timer.interval(), 0ns);

    QProperty<std::chrono::nanoseconds> interval;
    timer.bindableInterval().setBinding(Qt::makePropertyBinding(interval));

    interval = 10ms;
    QCOMPARE(timer.interval(), 10ms);
    interval = 100ms;
    QCOMPARE(timer.interval(), 100ms);
    timer.setInterval(50ms);
    QCOMPARE(timer.interval(), 50ms);
    interval = 30ms;
    QCOMPARE(timer.interval(), 50ms);

    // timerType property
    QCOMPARE(timer.timerType(), Qt::CoarseTimer);

    QProperty<Qt::TimerType> timerType;
    timer.bindableTimerType().setBinding(Qt::makePropertyBinding(timerType));

    timerType = Qt::PreciseTimer;
    QCOMPARE(timer.timerType(), Qt::PreciseTimer);
    timerType = Qt::VeryCoarseTimer;
    QCOMPARE(timer.timerType(), Qt::VeryCoarseTimer);
}

void tst_QChronoTimer::automatedBindingTests()
{
    QChronoTimer timer;

    QVERIFY(!timer.isSingleShot());
    QTestPrivate::testReadWritePropertyBasics(timer, true, false, "singleShot");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QChronoTimer::singleShot");
        return;
    }

    QCOMPARE_NE(timer.interval(), 10ms);
    using NSec = std::chrono::nanoseconds;
    QTestPrivate::testReadWritePropertyBasics(timer, NSec{10ms}, NSec{20ms}, "interval");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QChronoTimer::interval");
        return;
    }

    QCOMPARE_NE(timer.timerType(), Qt::PreciseTimer);
    QTestPrivate::testReadWritePropertyBasics(timer, Qt::PreciseTimer, Qt::CoarseTimer,
                                              "timerType");
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QChronoTimer::timerType");
        return;
    }

    timer.setInterval(1s);
    timer.start();
    QVERIFY(timer.isActive());
    QTestPrivate::testReadOnlyPropertyBasics(timer, true, false, "active",
                                             [&timer]() { timer.stop(); });
    if (QTest::currentTestFailed()) {
        qDebug("Failed property test for QChronoTimer::active");
        return;
    }
}

void tst_QChronoTimer::negativeInterval()
{
    QChronoTimer timer;

    auto ignoreMsg = [] {
        QTest::ignoreMessage(QtWarningMsg,
                             "QObject::startTimer: Timers cannot have negative intervals");
    };

    ignoreMsg();
    // Setting a negative interval does not change the active state.
    timer.setInterval(-100ms);
    ignoreMsg();
    timer.start();
    QVERIFY(!timer.isActive());

    // Starting a timer that has a positive interval, the active state is changed
    timer.setInterval(100ms);
    timer.start();
    QVERIFY(timer.isActive());

    ignoreMsg();
    // Setting a negative interval on an already running timer...
    timer.setInterval(-100ms);
    // ... the timer is stopped and the active state is changed
    QVERIFY(!timer.isActive());

    // Calling start on a timer that has a negative interval, does not change the active state
    timer.start();
    QVERIFY(!timer.isActive());
}

class OrderHelper : public QObject
{
    Q_OBJECT
public:
    enum CallType
    {
        String,
        PMF,
        Functor,
        FunctorNoCtx
    };
    Q_ENUM(CallType)
    QList<CallType> calls;

    void triggerCall(CallType callType)
    {
        switch (callType)
        {
        case String:
            QTimer::singleShot(0ns, this, SLOT(stringSlot()));
            break;
        case PMF:
            QTimer::singleShot(0ns, this, &OrderHelper::pmfSlot);
            break;
        case Functor:
            QTimer::singleShot(0ns, this, [this]() { functorSlot(); });
            break;
        case FunctorNoCtx:
            QTimer::singleShot(0ns, [this]() { functorNoCtxSlot(); });
            break;
        }
    }

public slots:
    void stringSlot() { calls << String; }
    void pmfSlot() { calls << PMF; }
    void functorSlot() { calls << Functor; }
    void functorNoCtxSlot() { calls << FunctorNoCtx; }
};

Q_DECLARE_METATYPE(OrderHelper::CallType)

void tst_QChronoTimer::timerOrder()
{
    QFETCH(QList<OrderHelper::CallType>, calls);

    OrderHelper helper;

    for (const auto call : calls)
        helper.triggerCall(call);

    QTRY_COMPARE(helper.calls, calls);
}

void tst_QChronoTimer::timerOrder_data()
{
    QTest::addColumn<QList<OrderHelper::CallType>>("calls");

    QList<OrderHelper::CallType> calls = {
        OrderHelper::String, OrderHelper::PMF,
        OrderHelper::Functor, OrderHelper::FunctorNoCtx
    };
    std::sort(calls.begin(), calls.end());

    int permutation = 0;
    do {
        QTest::addRow("permutation=%d", permutation) << calls;
        ++permutation;
    } while (std::next_permutation(calls.begin(), calls.end()));
}

void tst_QChronoTimer::timerOrderBackgroundThread()
{
    auto *thread = QThread::create([this]() { timerOrder(); });
    thread->start();
    QVERIFY(thread->wait());
    delete thread;
}

void tst_QChronoTimer::timerPrecision()
{
    using namespace std::chrono;
    steady_clock::time_point t1{};
    steady_clock::time_point t2{};

    QEventLoop loop;

    QChronoTimer zeroTimer{0ns};
    zeroTimer.setTimerType(Qt::PreciseTimer);
    zeroTimer.setSingleShot(true);
    connect(&zeroTimer, &QChronoTimer::timeout, this, [&t1] { t1 = steady_clock::now(); });

    QChronoTimer oneNSecTimer{1ns};
    oneNSecTimer.setTimerType(Qt::PreciseTimer);
    oneNSecTimer.setSingleShot(true);
    connect(&oneNSecTimer, &QChronoTimer::timeout, this, [&t2, &loop] {
        t2 = steady_clock::now();
        loop.quit();
    });

    zeroTimer.start();
    oneNSecTimer.start();
    loop.exec();
    QCOMPARE_GT(t2, t1);
    // qDebug() << "t2 - t1" << duration<double, std::chrono::milliseconds::period>{t2 - t1};
}

QTEST_MAIN(tst_QChronoTimer)

#include "tst_qchronotimer.moc"
