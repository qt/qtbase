/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifdef QT_GUI_LIB
#  include <QtGui/QGuiApplication>
#else
#  include <QtCore/QCoreApplication>
#endif

#include <QtTest/QtTest>

#include <qtimer.h>
#include <qthread.h>

#if defined Q_OS_UNIX
#include <unistd.h>
#endif

class tst_QTimer : public QObject
{
    Q_OBJECT
private slots:
    void zeroTimer();
    void singleShotTimeout();
    void timeout();
    void remainingTime();
    void livelock_data();
    void livelock();
    void timerInfiniteRecursion_data();
    void timerInfiniteRecursion();
    void recurringTimer_data();
    void recurringTimer();
    void deleteLaterOnQTimer(); // long name, don't want to shadow QObject::deleteLater()
    void moveToThread();
    void restartedTimerFiresTooSoon();
    void timerFiresOnlyOncePerProcessEvents_data();
    void timerFiresOnlyOncePerProcessEvents();
    void timerIdPersistsAfterThreadExit();
    void cancelLongTimer();
    void singleShotStaticFunctionZeroTimeout();
    void recurseOnTimeoutAndStopTimer();

    void dontBlockEvents();
    void postedEventsShouldNotStarveTimers();
};

class TimerHelper : public QObject
{
    Q_OBJECT
public:
    TimerHelper() : QObject(), count(0)
    {
    }

    int count;

public slots:
    void timeout();
};

void TimerHelper::timeout()
{
    ++count;
}

void tst_QTimer::zeroTimer()
{
    TimerHelper helper;
    QTimer timer;
    timer.setInterval(0);
    timer.start();

    connect(&timer, SIGNAL(timeout()), &helper, SLOT(timeout()));

    QCoreApplication::processEvents();

    QCOMPARE(helper.count, 1);
}

void tst_QTimer::singleShotTimeout()
{
    TimerHelper helper;
    QTimer timer;
    timer.setSingleShot(true);

    connect(&timer, SIGNAL(timeout()), &helper, SLOT(timeout()));
    timer.start(100);

    QTest::qWait(500);
    QCOMPARE(helper.count, 1);
    QTest::qWait(500);
    QCOMPARE(helper.count, 1);
}

#define TIMEOUT_TIMEOUT 200

void tst_QTimer::timeout()
{
    TimerHelper helper;
    QTimer timer;

    connect(&timer, SIGNAL(timeout()), &helper, SLOT(timeout()));
    timer.start(100);

    QCOMPARE(helper.count, 0);

    QTest::qWait(TIMEOUT_TIMEOUT);
    QVERIFY(helper.count > 0);
    int oldCount = helper.count;

    QTest::qWait(TIMEOUT_TIMEOUT);
    QVERIFY(helper.count > oldCount);
}

void tst_QTimer::remainingTime()
{
    TimerHelper helper;
    QTimer timer;

    connect(&timer, SIGNAL(timeout()), &helper, SLOT(timeout()));
    timer.start(200);

    QCOMPARE(helper.count, 0);

    QTest::qWait(50);
    QCOMPARE(helper.count, 0);

    int remainingTime = timer.remainingTime();
    QVERIFY2(qAbs(remainingTime - 150) < 50, qPrintable(QString::number(remainingTime)));
}

void tst_QTimer::livelock_data()
{
    QTest::addColumn<int>("interval");
    QTest::newRow("zero timer") << 0;
    QTest::newRow("non-zero timer") << 1;
    QTest::newRow("longer than sleep") << 20;
}

/*!
 *
 * DO NOT "FIX" THIS TEST!  it is written like this for a reason, do
 * not *change it without first dicussing it with its maintainers.
 *
*/
class LiveLockTester : public QObject
{
public:
    LiveLockTester(int i)
        : interval(i),
          timeoutsForFirst(0), timeoutsForExtra(0), timeoutsForSecond(0),
          postEventAtRightTime(false)
    {
        firstTimerId = startTimer(interval);
        extraTimerId = startTimer(interval + 80);
        secondTimerId = -1; // started later
    }

    bool event(QEvent *e) {
        if (e->type() == 4002) {
            // got the posted event
            if (timeoutsForFirst == 1 && timeoutsForSecond == 0)
                postEventAtRightTime = true;
            return true;
        }
        return QObject::event(e);
    }

    void timerEvent(QTimerEvent *te) {
        if (te->timerId() == firstTimerId) {
            if (++timeoutsForFirst == 1) {
                killTimer(extraTimerId);
                extraTimerId = -1;
                QCoreApplication::postEvent(this, new QEvent(static_cast<QEvent::Type>(4002)));
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

    const int interval;
    int firstTimerId;
    int secondTimerId;
    int extraTimerId;
    int timeoutsForFirst;
    int timeoutsForExtra;
    int timeoutsForSecond;
    bool postEventAtRightTime;
};

void tst_QTimer::livelock()
{
    /*
      New timers created in timer event handlers should not be sent
      until the next iteration of the eventloop.  Note: this test
      depends on the fact that we send posted events before timer
      events (since new posted events are not sent until the next
      iteration of the eventloop either).
    */
    QFETCH(int, interval);
    LiveLockTester tester(interval);
    QTest::qWait(180); // we have to use wait here, since we're testing timers with a non-zero timeout
    QTRY_COMPARE(tester.timeoutsForFirst, 1);
    QCOMPARE(tester.timeoutsForExtra, 0);
    QTRY_COMPARE(tester.timeoutsForSecond, 1);
#if defined(Q_OS_WINCE)
    QEXPECT_FAIL("non-zero timer", "Windows CE devices often too slow", Continue);
#endif
    QVERIFY(tester.postEventAtRightTime);
}

class TimerInfiniteRecursionObject : public QObject
{
public:
    bool inTimerEvent;
    bool timerEventRecursed;
    int interval;

    TimerInfiniteRecursionObject(int interval)
        : inTimerEvent(false), timerEventRecursed(false), interval(interval)
    { }

    void timerEvent(QTimerEvent *timerEvent)
    {
        timerEventRecursed = inTimerEvent;
        if (timerEventRecursed) {
            // bug detected!
            return;
        }

        inTimerEvent = true;

        QEventLoop eventLoop;
        QTimer::singleShot(qMax(100, interval * 2), &eventLoop, SLOT(quit()));
        eventLoop.exec();

        inTimerEvent = false;

        killTimer(timerEvent->timerId());
    }
};

void tst_QTimer::timerInfiniteRecursion_data()
{
    QTest::addColumn<int>("interval");
    QTest::newRow("zero timer") << 0;
    QTest::newRow("non-zero timer") << 1;
    QTest::newRow("10ms timer") << 10;
    QTest::newRow("11ms timer") << 11;
    QTest::newRow("100ms timer") << 100;
    QTest::newRow("1s timer") << 1000;
}


void tst_QTimer::timerInfiniteRecursion()
{
    QFETCH(int, interval);
    TimerInfiniteRecursionObject object(interval);
    (void) object.startTimer(interval);

    QEventLoop eventLoop;
    QTimer::singleShot(qMax(100, interval * 2), &eventLoop, SLOT(quit()));
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

    void timerEvent(QTimerEvent *timerEvent)
    {
        if (++times == target) {
            killTimer(timerEvent->timerId());
            emit done();
        } if (recurse) {
            QEventLoop eventLoop;
            QTimer::singleShot(100, &eventLoop, SLOT(quit()));
            eventLoop.exec();
        }
    }

signals:
    void done();
};

void tst_QTimer::recurringTimer_data()
{
    QTest::addColumn<int>("interval");
    QTest::newRow("zero timer") << 0;
    QTest::newRow("non-zero timer") << 1;
}

void tst_QTimer::recurringTimer()
{
    const int target = 5;
    QFETCH(int, interval);

    {
        RecurringTimerObject object(target);
        QObject::connect(&object, SIGNAL(done()), &QTestEventLoop::instance(), SLOT(exitLoop()));
        (void) object.startTimer(interval);
        QTestEventLoop::instance().enterLoop(5);

        QCOMPARE(object.times, target);
    }

    {
        // make sure that eventloop recursion doesn't effect timer recurrance
        RecurringTimerObject object(target);
        object.recurse = true;

        QObject::connect(&object, SIGNAL(done()), &QTestEventLoop::instance(), SLOT(exitLoop()));
        (void) object.startTimer(interval);
        QTestEventLoop::instance().enterLoop(5);

        QCOMPARE(object.times, target);
    }
}

void tst_QTimer::deleteLaterOnQTimer()
{
    QTimer *timer = new QTimer;
    connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()));
    connect(timer, SIGNAL(destroyed()), &QTestEventLoop::instance(), SLOT(exitLoop()));
    timer->setInterval(1);
    timer->setSingleShot(true);
    timer->start();
    QPointer<QTimer> pointer = timer;
    QTestEventLoop::instance().enterLoop(5);
    QVERIFY(pointer.isNull());
}

#define MOVETOTHREAD_TIMEOUT 200
#define MOVETOTHREAD_WAIT 300

void tst_QTimer::moveToThread()
{
    QTimer ti1;
    QTimer ti2;
    ti1.start(MOVETOTHREAD_TIMEOUT);
    ti2.start(MOVETOTHREAD_TIMEOUT);
    QVERIFY((ti1.timerId() & 0xffffff) != (ti2.timerId() & 0xffffff));
    QThread tr;
    ti1.moveToThread(&tr);
    connect(&ti1,SIGNAL(timeout()), &tr, SLOT(quit()));
    tr.start();
    QTimer ti3;
    ti3.start(MOVETOTHREAD_TIMEOUT);
    QVERIFY((ti3.timerId() & 0xffffff) != (ti2.timerId() & 0xffffff));
    QVERIFY((ti3.timerId() & 0xffffff) != (ti1.timerId() & 0xffffff));
    QTest::qWait(MOVETOTHREAD_WAIT);
    QVERIFY(tr.wait());
    ti2.stop();
    QTimer ti4;
    ti4.start(MOVETOTHREAD_TIMEOUT);
    ti3.stop();
    ti2.start(MOVETOTHREAD_TIMEOUT);
    ti3.start(MOVETOTHREAD_TIMEOUT);
    QVERIFY((ti4.timerId() & 0xffffff) != (ti2.timerId() & 0xffffff));
    QVERIFY((ti3.timerId() & 0xffffff) != (ti2.timerId() & 0xffffff));
    QVERIFY((ti3.timerId() & 0xffffff) != (ti1.timerId() & 0xffffff));
}

class RestartedTimerFiresTooSoonObject : public QObject
{
    Q_OBJECT

public:
    QBasicTimer m_timer;

    int m_interval;
    QTime m_startedTime;
    QEventLoop eventLoop;

    inline RestartedTimerFiresTooSoonObject()
        : QObject(), m_interval(0)
    { }

    void timerFired()
    {
        static int interval = 1000;

        m_interval = interval;
        m_startedTime.start();
        m_timer.start(interval, this);

        // alternate between single-shot and 1 sec
        interval = interval ? 0 : 1000;
    }

    void timerEvent(QTimerEvent* ev)
    {
        if (ev->timerId() != m_timer.timerId())
            return;

        m_timer.stop();

        int elapsed = m_startedTime.elapsed();

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

void tst_QTimer::restartedTimerFiresTooSoon()
{
    RestartedTimerFiresTooSoonObject object;
    object.timerFired();
    QVERIFY(object.eventLoop.exec() == 0);
}

class LongLastingSlotClass : public QObject
{
    Q_OBJECT

public:
    LongLastingSlotClass(QTimer *timer) : count(0), timer(timer) {}

public slots:
    void longLastingSlot()
    {
        // Don't use timers for this, because we are testing them.
        QTime time;
        time.start();
        while (time.elapsed() < 200) {
            for (int c = 0; c < 100000; c++) {} // Mindless looping.
        }
        if (++count >= 2) {
            timer->stop();
        }
    }

public:
    int count;
    QTimer *timer;
};

void tst_QTimer::timerFiresOnlyOncePerProcessEvents_data()
{
    QTest::addColumn<int>("interval");
    QTest::newRow("zero timer") << 0;
    QTest::newRow("non-zero timer") << 10;
}

void tst_QTimer::timerFiresOnlyOncePerProcessEvents()
{
    QFETCH(int, interval);

    QTimer t;
    LongLastingSlotClass longSlot(&t);
    t.start(interval);
    connect(&t, SIGNAL(timeout()), &longSlot, SLOT(longLastingSlot()));
    // Loop because there may be other events pending.
    while (longSlot.count == 0) {
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }

    QCOMPARE(longSlot.count, 1);
}

class TimerIdPersistsAfterThreadExitThread : public QThread
{
public:
    QTimer *timer;
    int timerId, returnValue;

    TimerIdPersistsAfterThreadExitThread()
        : QThread(), timer(0), timerId(-1), returnValue(-1)
    { }
    ~TimerIdPersistsAfterThreadExitThread()
    {
        delete timer;
    }

    void run()
    {
        QEventLoop eventLoop;
        timer = new QTimer;
        connect(timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
        timer->start(100);
        timerId = timer->timerId();
        returnValue = eventLoop.exec();
    }
};

void tst_QTimer::timerIdPersistsAfterThreadExit()
{
    TimerIdPersistsAfterThreadExitThread thread;
    thread.start();
    QVERIFY(thread.wait(30000));
    QCOMPARE(thread.returnValue, 0);

    // even though the thread has exited, and the event dispatcher destroyed, the timer is still
    // "active", meaning the timer id should NOT be reused (i.e. the event dispatcher should not
    // have unregistered it)
    int timerId = thread.startTimer(100);
    QVERIFY((timerId & 0xffffff) != (thread.timerId & 0xffffff));
}

void tst_QTimer::cancelLongTimer()
{
    QTimer timer;
    timer.setSingleShot(true);
    timer.start(1000 * 60 * 60); //set timer for 1 hour
    QCoreApplication::processEvents();
    QVERIFY(timer.isActive()); //if the timer completes immediately with an error, then this will fail
    timer.stop();
    QVERIFY(!timer.isActive());
}

void tst_QTimer::singleShotStaticFunctionZeroTimeout()
{
    TimerHelper helper;

    QTimer::singleShot(0, &helper, SLOT(timeout()));
    QTest::qWait(500);
    QCOMPARE(helper.count, 1);
    QTest::qWait(500);
    QCOMPARE(helper.count, 1);
}

class RecursOnTimeoutAndStopTimerTimer : public QObject
{
    Q_OBJECT

public:
    QTimer *one;
    QTimer *two;

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

void tst_QTimer::recurseOnTimeoutAndStopTimer()
{
    QEventLoop eventLoop;
    QTimer::singleShot(1000, &eventLoop, SLOT(quit()));

    RecursOnTimeoutAndStopTimerTimer t;
    t.one = new QTimer(&t);
    t.two = new QTimer(&t);

    QObject::connect(t.one, SIGNAL(timeout()), &t, SLOT(onetrigger()));
    QObject::connect(t.two, SIGNAL(timeout()), &t, SLOT(twotrigger()));

    t.two->setSingleShot(true);

    t.one->start();
    t.two->start();

    (void) eventLoop.exec();

    QVERIFY(!t.one->isActive());
    QVERIFY(!t.two->isActive());
}



class DontBlockEvents : public QObject
{
    Q_OBJECT
public:
    DontBlockEvents();
    void timerEvent(QTimerEvent*);

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

    // need a few unrelated timers running to reproduce the bug.
    (new QTimer(this))->start(2000);
    (new QTimer(this))->start(2500);
    (new QTimer(this))->start(3000);
    (new QTimer(this))->start(5000);
    (new QTimer(this))->start(1000);
    (new QTimer(this))->start(2000);

    m_timer.start(1, this);
}

void DontBlockEvents::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == m_timer.timerId()) {
        QMetaObject::invokeMethod(this, "paintEvent", Qt::QueuedConnection);
        m_timer.start(0, this);
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
void tst_QTimer::dontBlockEvents()
{
    DontBlockEvents t;
    QTest::qWait(60);
    QTRY_VERIFY(t.total > 2);
}

class SlotRepeater : public QObject {
    Q_OBJECT
public:
    SlotRepeater() {}

public slots:
    void repeatThisSlot()
    {
        QMetaObject::invokeMethod(this, "repeatThisSlot", Qt::QueuedConnection);
    }
};

void tst_QTimer::postedEventsShouldNotStarveTimers()
{
    TimerHelper timerHelper;
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &timerHelper, SLOT(timeout()));
    timer.setInterval(0);
    timer.setSingleShot(false);
    timer.start();
    SlotRepeater slotRepeater;
    slotRepeater.repeatThisSlot();
    QTest::qWait(100);
    QVERIFY(timerHelper.count > 5);
}

QTEST_MAIN(tst_QTimer)
#include "tst_qtimer.moc"
