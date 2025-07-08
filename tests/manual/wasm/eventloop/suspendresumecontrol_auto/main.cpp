// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qwasmsuspendresumecontrol_p.h>
#include <qtwasmtestlib.h>

using namespace emscripten;

const std::chrono::milliseconds timerTimeout{10};

// Test QWasmSuspendResumeControl suspend/resume and event processing,
// via QWasmTimer native timer events.
class WasmSuspendResumeControlTest: public QObject
{
    Q_OBJECT
private slots:
    void timer();
    void multipleTimers();
    void reuseTimer();
    void cancelTimer();
    void deleteTimer();
};

// Verify that a single timer fires
void WasmSuspendResumeControlTest::timer()
{
    QWasmSuspendResumeControl suspendResume;
    bool timerFired = false;

    QWasmTimer timer(&suspendResume, [&timerFired](){
        timerFired = true;
    });
    timer.setTimeout(timerTimeout);

    while (!timerFired) {
        suspendResume.suspend();
        suspendResume.sendPendingEvents();
    }

    QWASMSUCCESS();
}

// Verify that multiple parallel timers fire
void WasmSuspendResumeControlTest::multipleTimers()
{
    QWasmSuspendResumeControl suspendResume;
    const int expectedTimers = 10;
    int compledtedTimers = 0;

    std::unique_ptr<QWasmTimer> timers[expectedTimers];
    for (int i = 0; i < expectedTimers; ++i) {
        timers[i] = std::make_unique<QWasmTimer>(&suspendResume, [&compledtedTimers](){
            ++compledtedTimers;
        });
        timers[i]->setTimeout(timerTimeout * i);
    }

    while (compledtedTimers < expectedTimers) {
        suspendResume.suspend();
        suspendResume.sendPendingEvents();
    }

    QWASMSUCCESS();
}

// Verify that a reused timer fires again
void WasmSuspendResumeControlTest::reuseTimer()
{
    QWasmSuspendResumeControl suspendResume;
    const int expectedTimers = 10;
    int compledtedTimers = 0;

    QWasmTimer timer(&suspendResume, [&compledtedTimers](){
        ++compledtedTimers;
    });

    while (compledtedTimers < expectedTimers) {
        timer.setTimeout(timerTimeout);
        suspendResume.suspend();
        suspendResume.sendPendingEvents();
    }

    QWASMSUCCESS();
}

// Verify that a cancelled timer does not fire
void WasmSuspendResumeControlTest::cancelTimer()
{
    QWasmSuspendResumeControl suspendResume;

    // controlTimer checks that the cancelled testTimer didn't fire
    bool controlFired = false;
    QWasmTimer controlTimer(&suspendResume, [&controlFired](){
        controlFired = true;
    });

    QWasmTimer testTimer(&suspendResume, [](){
        QWASMFAIL("Cancelled timer did fire");
    });

    controlTimer.setTimeout(timerTimeout * 4);
    testTimer.setTimeout(timerTimeout);
    testTimer.clearTimeout();

    while (!controlFired) {
        suspendResume.suspend();
        suspendResume.sendPendingEvents();
    }

    QWASMSUCCESS();
}

// Verify that a deleted timer does not fire
void WasmSuspendResumeControlTest::deleteTimer()
{
    QWasmSuspendResumeControl suspendResume;

    // controlTimer checks that the deleted testTimer didn't fire
    bool controlFired = false;
    QWasmTimer controlTimer(&suspendResume, [&controlFired](){
        controlFired = true;
    });
    controlTimer.setTimeout(timerTimeout * 4);

    {
        QWasmTimer testTimer(&suspendResume, [](){
            QWASMFAIL("Deleted timer did fire");
        });
        testTimer.setTimeout(timerTimeout);
    }

    while (!controlFired) {
        suspendResume.suspend();
        suspendResume.sendPendingEvents();
    }

    QWASMSUCCESS();
}

int main(int argc, char **argv)
{
    auto testObject = std::make_shared<WasmSuspendResumeControlTest>();
    QtWasmTest::initTestCase<QCoreApplication>(argc, argv, testObject);
    return 0;
}

#include "main.moc"
