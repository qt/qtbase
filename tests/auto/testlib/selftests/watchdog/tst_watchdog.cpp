// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

using namespace std::chrono_literals;

class tst_Watchdog : public QObject
{
    Q_OBJECT
private slots:
    void delay() const;
};

void tst_Watchdog::delay() const
{
    bool ok = false;
    // Use the same env.var as the watch-dog and add a little to it:
    const int timeout = qEnvironmentVariableIntValue("QTEST_FUNCTION_TIMEOUT", &ok);
    QTest::qSleep(5s + (ok && timeout > 0 ? timeout * 1ms : 5min));
    // The watchdog timer should have interrupted us by now.
    QFAIL("ERROR: this function should be interrupted.");
}

QTEST_APPLESS_MAIN(tst_Watchdog)

#include "tst_watchdog.moc"
