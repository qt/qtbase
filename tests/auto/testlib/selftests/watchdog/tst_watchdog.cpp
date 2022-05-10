// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

class tst_Watchdog : public QObject
{
    Q_OBJECT
private slots:
    void delay() const;
};

void tst_Watchdog::delay() const
{
    bool ok = false;
    const int fiveMinutes = 5 * 60 * 1000;
    // Use the same env.var as the watch-dog and add a little to it:
    const int timeout = qEnvironmentVariableIntValue("QTEST_FUNCTION_TIMEOUT", &ok);
    QTest::qSleep(5000 + (ok && timeout > 0 ? timeout : fiveMinutes));
    // The watchdog timer should have interrupted us by now.
    QFAIL("ERROR: this function should be interrupted.");
}

QTEST_APPLESS_MAIN(tst_Watchdog)

#include "tst_watchdog.moc"
