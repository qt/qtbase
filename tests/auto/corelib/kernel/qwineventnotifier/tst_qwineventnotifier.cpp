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
#include <qt_windows.h>

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

        static int nextIndex = 0;
        idx = nextIndex++;
    }

    ~EventWithNotifier()
    {
        notifier.setEnabled(false);
        CloseHandle(notifier.handle());
    }

    HANDLE eventHandle() const { return notifier.handle(); }
    int numberOfTimesActivated() const { return activatedCount; }

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
    int idx = 0;
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

QTEST_MAIN(tst_QWinEventNotifier)

#include "tst_qwineventnotifier.moc"
