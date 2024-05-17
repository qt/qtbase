// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QThread>
#include <QTest>

class tst_QProcessNoApplication : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initializationDeadlock();
};

void tst_QProcessNoApplication::initializationDeadlock()
{
    // see QTBUG-27260
    // QProcess on Unix uses (or used to, at the time of the writing of this test)
    // a global class called QProcessManager.
    // This class is instantiated (or was) only in the main thread, which meant that
    // blocking the main thread while waiting for QProcess could mean a deadlock.

    struct MyThread : public QThread
    {
        void run() override
        {
            // what we execute does not matter, as long as we try to
            // and that the process exits
            QProcess::execute("true");
        }
    };

    char *argv[] = { const_cast<char*>(QTest::currentAppName()), 0 };
    int argc = 1;
    QCoreApplication app(argc, argv);
    MyThread thread;
    thread.start();
    QVERIFY(thread.wait(10000));
}

QTEST_APPLESS_MAIN(tst_QProcessNoApplication)

#include "tst_qprocessnoapplication.moc"
