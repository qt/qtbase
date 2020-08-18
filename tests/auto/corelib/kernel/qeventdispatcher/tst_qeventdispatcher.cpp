/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifdef QT_GUI_LIB
#  include <QtGui/QGuiApplication>
#  define tst_QEventDispatcher tst_QGuiEventDispatcher
#else
#  include <QtCore/QCoreApplication>
#endif
#include <QtTest/QtTest>

enum {
    PreciseTimerInterval    =   10,
    CoarseTimerInterval     =  200,
    VeryCoarseTimerInterval = 1000
};

class tst_QEventDispatcher : public QObject
{
    Q_OBJECT

    QAbstractEventDispatcher *eventDispatcher;

    int receivedEventType = -1;
    int timerIdFromEvent = -1;
    bool doubleTimer = false;

protected:
    bool event(QEvent *e);

public:
    inline tst_QEventDispatcher()
        : QObject(),
          eventDispatcher(QAbstractEventDispatcher::instance(thread()))
    { }

private slots:
    void initTestCase();
    void registerTimer();
    /* void registerSocketNotifier(); */ // Not implemented here, see tst_QSocketNotifier instead
    /* void registerEventNotifiier(); */ // Not implemented here, see tst_QWinEventNotifier instead
    void sendPostedEvents_data();
    void sendPostedEvents();
    void processEventsOnlySendsQueuedEvents();
    void postedEventsPingPong();
    void eventLoopExit();
};

bool tst_QEventDispatcher::event(QEvent *e)
{
    switch (receivedEventType = e->type()) {
    case QEvent::Timer:
    {
        // sometimes, two timers fire during a single QTRY_xxx wait loop
        if (timerIdFromEvent != -1)
            doubleTimer = true;
        timerIdFromEvent = static_cast<QTimerEvent *>(e)->timerId();
        return true;
    }
    default:
        break;
    }
    return QObject::event(e);
}

// drain the system event queue after the test starts to avoid destabilizing the test functions
void tst_QEventDispatcher::initTestCase()
{
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    while (!elapsedTimer.hasExpired(CoarseTimerInterval) && eventDispatcher->processEvents(QEventLoop::AllEvents)) {
            ;
    }
}

class TimerManager {
    Q_DISABLE_COPY(TimerManager)

public:
    TimerManager(QAbstractEventDispatcher *eventDispatcher, QObject *parent)
        : m_eventDispatcher(eventDispatcher), m_parent(parent)
    {
    }

    ~TimerManager()
    {
        if (!registeredTimers().isEmpty())
            m_eventDispatcher->unregisterTimers(m_parent);
    }

    TimerManager(TimerManager &&) = delete;
    TimerManager &operator=(TimerManager &&) = delete;

    int preciseTimerId() const { return m_preciseTimerId; }
    int coarseTimerId() const { return m_coarseTimerId; }
    int veryCoarseTimerId() const { return m_veryCoarseTimerId; }

    bool foundPrecise() const { return m_preciseTimerId > 0; }
    bool foundCoarse() const { return m_coarseTimerId > 0; }
    bool foundVeryCoarse() const { return m_veryCoarseTimerId > 0; }

    QList<QAbstractEventDispatcher::TimerInfo> registeredTimers() const
    {
        return m_eventDispatcher->registeredTimers(m_parent);
    }

    void registerAll()
    {
        // start 3 timers, each with the different timer types and different intervals
        m_preciseTimerId = m_eventDispatcher->registerTimer(
                    PreciseTimerInterval, Qt::PreciseTimer, m_parent);
        m_coarseTimerId = m_eventDispatcher->registerTimer(
                    CoarseTimerInterval, Qt::CoarseTimer, m_parent);
        m_veryCoarseTimerId = m_eventDispatcher->registerTimer(
                    VeryCoarseTimerInterval, Qt::VeryCoarseTimer, m_parent);
        QVERIFY(m_preciseTimerId > 0);
        QVERIFY(m_coarseTimerId > 0);
        QVERIFY(m_veryCoarseTimerId > 0);
        findTimers();
    }

    void unregister(int timerId)
    {
        m_eventDispatcher->unregisterTimer(timerId);
        findTimers();
    }

    void unregisterAll()
    {
        m_eventDispatcher->unregisterTimers(m_parent);
        findTimers();
    }

private:
    void findTimers()
    {
        bool foundPrecise = false;
        bool foundCoarse = false;
        bool foundVeryCoarse = false;
        const QList<QAbstractEventDispatcher::TimerInfo> timers = registeredTimers();
        for (int i = 0; i < timers.count(); ++i) {
            const QAbstractEventDispatcher::TimerInfo &timerInfo = timers.at(i);
            if (timerInfo.timerId == m_preciseTimerId) {
                QCOMPARE(timerInfo.interval, int(PreciseTimerInterval));
                QCOMPARE(timerInfo.timerType, Qt::PreciseTimer);
                foundPrecise = true;
            } else if (timerInfo.timerId == m_coarseTimerId) {
                QCOMPARE(timerInfo.interval, int(CoarseTimerInterval));
                QCOMPARE(timerInfo.timerType, Qt::CoarseTimer);
                foundCoarse = true;
            } else if (timerInfo.timerId == m_veryCoarseTimerId) {
                QCOMPARE(timerInfo.interval, int(VeryCoarseTimerInterval));
                QCOMPARE(timerInfo.timerType, Qt::VeryCoarseTimer);
                foundVeryCoarse = true;
            }
        }
        if (!foundPrecise)
            m_preciseTimerId = -1;
        if (!foundCoarse)
            m_coarseTimerId = -1;
        if (!foundVeryCoarse)
            m_veryCoarseTimerId = -1;
    }

    QAbstractEventDispatcher *m_eventDispatcher = nullptr;

    int m_preciseTimerId = -1;
    int m_coarseTimerId = -1;
    int m_veryCoarseTimerId = -1;

    QObject *m_parent = nullptr;
};

// test that the eventDispatcher's timer implementation is complete and working
void tst_QEventDispatcher::registerTimer()
{
    TimerManager timers(eventDispatcher, this);
    timers.registerAll();
    if (QTest::currentTestFailed())
        return;

    // check that all 3 are present in the eventDispatcher's registeredTimer() list
    QCOMPARE(timers.registeredTimers().count(), 3);
    QVERIFY(timers.foundPrecise());
    QVERIFY(timers.foundCoarse());
    QVERIFY(timers.foundVeryCoarse());

#ifdef Q_OS_DARWIN
    /*
        We frequently experience flaky failures on macOS. Assumption is that this is
        due to undeterministic VM scheduling, making us process events for significantly
        longer than expected and resulting in timers firing in undefined order.
        To detect this condition, we use a QElapsedTimer, and skip the test.
    */
    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
#endif

    // process events, waiting for the next event... this should only fire the precise timer
    receivedEventType = -1;
    timerIdFromEvent = -1;
    doubleTimer = false;
    QTRY_COMPARE_WITH_TIMEOUT(receivedEventType, int(QEvent::Timer), PreciseTimerInterval * 2);

#ifdef Q_OS_DARWIN
    if (doubleTimer)
        QSKIP("Double timer during a single timeout - aborting test as flaky on macOS");
    if (timerIdFromEvent != timers.preciseTimerId()
        && elapsedTimer.elapsed() > PreciseTimerInterval * 3)
        QSKIP("Ignore flaky test behavior due to VM scheduling on macOS");
#endif

    QCOMPARE(timerIdFromEvent, timers.preciseTimerId());
    // now unregister it and make sure it's gone
    timers.unregister(timers.preciseTimerId());
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(timers.registeredTimers().count(), 2);
    QVERIFY(!timers.foundPrecise());
    QVERIFY(timers.foundCoarse());
    QVERIFY(timers.foundVeryCoarse());

    // do the same again for the coarse timer
    receivedEventType = -1;
    timerIdFromEvent = -1;
    doubleTimer = false;
    QTRY_COMPARE_WITH_TIMEOUT(receivedEventType, int(QEvent::Timer), CoarseTimerInterval * 2);

#ifdef Q_OS_DARWIN
    if (doubleTimer)
        QSKIP("Double timer during a single timeout - aborting test as flaky on macOS");
    if (timerIdFromEvent != timers.coarseTimerId()
        && elapsedTimer.elapsed() > CoarseTimerInterval * 3)
        QSKIP("Ignore flaky test behavior due to VM scheduling on macOS");
#endif

    QCOMPARE(timerIdFromEvent, timers.coarseTimerId());
    // now unregister it and make sure it's gone
    timers.unregister(timers.coarseTimerId());
    if (QTest::currentTestFailed())
        return;
    QCOMPARE(timers.registeredTimers().count(), 1);
    QVERIFY(!timers.foundPrecise());
    QVERIFY(!timers.foundCoarse());
    QVERIFY(timers.foundVeryCoarse());

    // not going to wait for the VeryCoarseTimer, would take too long, just unregister it
    timers.unregisterAll();
    if (QTest::currentTestFailed())
        return;
    QVERIFY(timers.registeredTimers().isEmpty());
}

void tst_QEventDispatcher::sendPostedEvents_data()
{
    QTest::addColumn<int>("processEventsFlagsInt");

    QTest::newRow("WaitForMoreEvents") << int(QEventLoop::WaitForMoreEvents);
    QTest::newRow("AllEvents") << int(QEventLoop::AllEvents);
}

// test that the eventDispatcher sends posted events correctly
void tst_QEventDispatcher::sendPostedEvents()
{
    QFETCH(int, processEventsFlagsInt);
    QEventLoop::ProcessEventsFlags processEventsFlags = QEventLoop::ProcessEventsFlags(processEventsFlagsInt);

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();
    while (!elapsedTimer.hasExpired(200)) {
        receivedEventType = -1;
        QCoreApplication::postEvent(this, new QEvent(QEvent::User));

        // event shouldn't be delivered as a result of posting
        QCOMPARE(receivedEventType, -1);

        // since there is a pending posted event, this should not actually block, it should send the posted event and return
        QVERIFY(eventDispatcher->processEvents(processEventsFlags));
        // event shouldn't be delivered as a result of posting
        QCOMPARE(receivedEventType, int(QEvent::User));
    }
}

class ProcessEventsOnlySendsQueuedEvents : public QObject
{
    Q_OBJECT
public:
    int eventsReceived;

    inline ProcessEventsOnlySendsQueuedEvents() : eventsReceived(0) {}

    bool event(QEvent *event)
    {
        ++eventsReceived;

        if (event->type() == QEvent::User)
             QCoreApplication::postEvent(this, new QEvent(QEvent::Type(QEvent::User + 1)));

        return QObject::event(event);
    }
public slots:
    void timerFired()
    {
        QCoreApplication::postEvent(this, new QEvent(QEvent::Type(QEvent::User + 1)));
    }
};

void tst_QEventDispatcher::processEventsOnlySendsQueuedEvents()
{
    ProcessEventsOnlySendsQueuedEvents object;

    // Posted events during event processing should be handled on
    // the next processEvents iteration.
    QCoreApplication::postEvent(&object, new QEvent(QEvent::User));
    QCoreApplication::processEvents();
    QCOMPARE(object.eventsReceived, 1);
    QCoreApplication::processEvents();
    QCOMPARE(object.eventsReceived, 2);

    // The same goes for posted events during timer processing
    QTimer::singleShot(0, &object, SLOT(timerFired()));
    QCoreApplication::processEvents();
    QCOMPARE(object.eventsReceived, 3);
    QCoreApplication::processEvents();
    QCOMPARE(object.eventsReceived, 4);
}

void tst_QEventDispatcher::postedEventsPingPong()
{
    QEventLoop mainLoop;

    // We need to have at least two levels of nested loops
    // for the posted event to get stuck (QTBUG-85981).
    QMetaObject::invokeMethod(this, [this, &mainLoop]() {
        QMetaObject::invokeMethod(this, [&mainLoop]() {
            // QEventLoop::quit() should be invoked on the next
            // iteration of mainLoop.exec().
            QMetaObject::invokeMethod(&mainLoop, &QEventLoop::quit,
                                      Qt::QueuedConnection);
        }, Qt::QueuedConnection);
        mainLoop.processEvents();
    }, Qt::QueuedConnection);

    // We should use Qt::CoarseTimer on Windows, to prevent event
    // dispatcher from sending a posted event.
    QTimer::singleShot(500, Qt::CoarseTimer, [&mainLoop]() {
        mainLoop.exit(1);
    });

    QCOMPARE(mainLoop.exec(), 0);
}

void tst_QEventDispatcher::eventLoopExit()
{
    // This test was inspired by QTBUG-79477. A particular
    // implementation detail in QCocoaEventDispatcher allowed
    // QEventLoop::exit() to fail to really exit the event loop.
    // Thus this test is a part of the dispatcher auto-test.

    // Imitates QApplication::exec():
    QEventLoop mainLoop;
    // The test itself is a lambda:
    QTimer::singleShot(0, [&mainLoop]() {
        // Two more single shots, both will be posted as events
        // (zero timeout) and supposed to be processes by the
        // mainLoop:

        QTimer::singleShot(0, [&mainLoop]() {
            // wakeUp triggers QCocoaEventDispatcher into incrementing
            // its 'serialNumber':
            mainLoop.wakeUp();
            // QCocoaEventDispatcher::processEvents() will process
            // posted events and execute the second lambda defined below:
            QCoreApplication::processEvents();
        });

        QTimer::singleShot(0, [&mainLoop]() {
            // With QCocoaEventDispatcher this is executed while in the
            // processEvents (see above) and would fail to actually
            // interrupt the loop.
            mainLoop.exit();
        });
    });

    bool timeoutObserved = false;
    QTimer::singleShot(500, [&timeoutObserved, &mainLoop]() {
        // In case the QEventLoop::exit above failed, we have to bail out
        // early, not wasting time:
        mainLoop.exit();
        timeoutObserved = true;
    });

    mainLoop.exec();
    QVERIFY(!timeoutObserved);
}

QTEST_MAIN(tst_QEventDispatcher)
#include "tst_qeventdispatcher.moc"
