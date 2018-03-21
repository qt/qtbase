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

#include <QtTest/QtTest>
#include <qwineventnotifier.h>
#include <qtimer.h>
#include <qvarlengtharray.h>
#include <qvector.h>
#include <qt_windows.h>

#include <algorithm>
#include <memory>

class tst_QWinEventNotifier : public QObject
{
    Q_OBJECT

protected slots:
    void simple_activated();
    void simple_timerSet();
private slots:
    void simple_data();
    void simple();
    void manyNotifiers();
    void disableNotifiersInActivatedSlot_data();
    void disableNotifiersInActivatedSlot();

private:
    HANDLE simpleHEvent;
    bool simpleActivated;
};

void tst_QWinEventNotifier::simple_activated()
{
    simpleActivated = true;
    ResetEvent((HANDLE)simpleHEvent);
    QTestEventLoop::instance().exitLoop();
}

void tst_QWinEventNotifier::simple_timerSet()
{
    SetEvent((HANDLE)simpleHEvent);
}

void tst_QWinEventNotifier::simple_data()
{
    QTest::addColumn<bool>("resetManually");
    QTest::newRow("manual_reset") << true;
    QTest::newRow("auto_reset") << false;
}

void tst_QWinEventNotifier::simple()
{
    QFETCH(bool, resetManually);
    simpleHEvent = CreateEvent(0, resetManually, false, 0);
    QVERIFY(simpleHEvent);

    QWinEventNotifier n(simpleHEvent);
    QObject::connect(&n, SIGNAL(activated(HANDLE)), this, SLOT(simple_activated()));
    simpleActivated = false;

    SetEvent((HANDLE)simpleHEvent);

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout())
        QFAIL("Timed out");

    QVERIFY(simpleActivated);


    simpleActivated = false;

    QTimer::singleShot(3000, this, SLOT(simple_timerSet()));

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout())
        QFAIL("Timed out");

    QVERIFY(simpleActivated);
}

class EventWithNotifier : public QObject
{
    Q_OBJECT
public:
    EventWithNotifier()
    {
        connect(&notifier, &QWinEventNotifier::activated,
                this, &EventWithNotifier::onNotifierActivated);
        notifier.setHandle(CreateEvent(0, TRUE, FALSE, 0));
        notifier.setEnabled(true);
    }

    ~EventWithNotifier()
    {
        notifier.setEnabled(false);
        CloseHandle(notifier.handle());
    }

    HANDLE eventHandle() const { return notifier.handle(); }
    int numberOfTimesActivated() const { return activatedCount; }
    void setEnabled(bool b) { notifier.setEnabled(b); }
    bool isEnabled() const { return notifier.isEnabled(); }

signals:
    void activated();

public slots:
    void onNotifierActivated()
    {
        ResetEvent(notifier.handle());
        activatedCount++;
        emit activated();
    }

private:
    QWinEventNotifier notifier;
    int activatedCount = 0;
};

void tst_QWinEventNotifier::manyNotifiers()
{
    const size_t maxEvents = 100;
    const size_t middleEvenEvent = maxEvents / 2;
    Q_ASSERT(middleEvenEvent % 2 == 0);
    using EventWithNotifierPtr = std::unique_ptr<EventWithNotifier>;
    std::vector<EventWithNotifierPtr> events(maxEvents);
    std::generate(events.begin(), events.end(), [] () {
        return EventWithNotifierPtr(new EventWithNotifier);
    });

    QTestEventLoop loop;
    auto connection = connect(events.at(8).get(), &EventWithNotifier::activated, &loop, &QTestEventLoop::exitLoop);
    for (const auto &ewn : events) {
        connect(ewn.get(), &EventWithNotifier::activated, [&events, &loop] () {
            if (std::all_of(events.cbegin(), events.cend(),
                    [] (const EventWithNotifierPtr &ewn) {
                            return ewn->numberOfTimesActivated() > 0; })) {
                loop.exitLoop();
            }
        });
    }

    // Activate all even events before running the event loop.
    for (size_t i = 0; i < events.size(); i += 2)
        SetEvent(events.at(i)->eventHandle());

    // Wait until event notifier with index 8 has been activated.
    loop.enterLoop(30);
    QObject::disconnect(connection);

    // Activate all odd events after the event loop has run for a bit.
    for (size_t i = 1; i < events.size(); i += 2)
        SetEvent(events.at(i)->eventHandle());

    // Wait until all event notifiers have fired.
    loop.enterLoop(30);

    // All notifiers must have been activated exactly once.
    QVERIFY(std::all_of(events.cbegin(), events.cend(), [] (const EventWithNotifierPtr &ewn) {
        return ewn->numberOfTimesActivated() == 1;
    }));
}

using Indices = QVector<int>;

void tst_QWinEventNotifier::disableNotifiersInActivatedSlot_data()
{
    QTest::addColumn<int>("count");
    QTest::addColumn<Indices>("notifiersToSignal");
    QTest::addColumn<Indices>("notifiersToDisable");
    QTest::addColumn<bool>("deleteNotifiers");
    QTest::newRow("disable_signaled") << 3 << Indices{1} << Indices{1} << false;
    QTest::newRow("disable_signaled2") << 3 << Indices{1, 2} << Indices{1} << false;
    QTest::newRow("disable_before_signaled") << 3 << Indices{1} << Indices{0, 1} << false;
    QTest::newRow("disable_after_signaled") << 3 << Indices{1} << Indices{1, 2} << false;
    QTest::newRow("delete_signaled") << 3 << Indices{1} << Indices{1} << true;
    QTest::newRow("delete_before_signaled1") << 3 << Indices{1} << Indices{0} << true;
    QTest::newRow("delete_before_signaled2") << 3 << Indices{1} << Indices{0, 1} << true;
    QTest::newRow("delete_before_signaled3") << 4 << Indices{3, 1} << Indices{0, 1} << true;
    QTest::newRow("delete_after_signaled1") << 3 << Indices{1} << Indices{1, 2} << true;
    QTest::newRow("delete_after_signaled2") << 4 << Indices{1, 3} << Indices{1, 2} << true;
    QTest::newRow("delete_after_signaled3") << 5 << Indices{1} << Indices{1, 4} << true;
}

void tst_QWinEventNotifier::disableNotifiersInActivatedSlot()
{
    QFETCH(int, count);
    QFETCH(Indices, notifiersToSignal);
    QFETCH(Indices, notifiersToDisable);
    QFETCH(bool, deleteNotifiers);

    QVarLengthArray<std::unique_ptr<EventWithNotifier>, 10> events(count);
    for (int i = 0; i < count; ++i)
        events[i].reset(new EventWithNotifier);

    auto isActivatedOrDisabled = [&events](int i) {
        return !events.at(i) || !events.at(i)->isEnabled()
               || events.at(i)->numberOfTimesActivated() > 0;
    };

    for (auto &e : events) {
        connect(e.get(), &EventWithNotifier::activated, [&]() {
            for (int i : notifiersToDisable) {
                if (deleteNotifiers)
                    events[i].reset();
                else
                    events.at(i)->setEnabled(false);
            }
            if (std::all_of(notifiersToSignal.begin(), notifiersToSignal.end(),
                            isActivatedOrDisabled)) {
                QTimer::singleShot(0, &QTestEventLoop::instance(), SLOT(exitLoop()));
            }
        });
    }
    for (int i : notifiersToSignal)
        SetEvent(events.at(i)->eventHandle());
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

QTEST_MAIN(tst_QWinEventNotifier)

#include "tst_qwineventnotifier.moc"
