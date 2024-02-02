// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/QGuiApplication>
#include <QTest>
#include <QSignalSpy>
#include <QtCore/QProcess>

class tst_QProcess_and_GuiEventLoop : public QObject
{
    Q_OBJECT
private slots:
    void waitForAndEventLoop();
};


void tst_QProcess_and_GuiEventLoop::waitForAndEventLoop()
{
#ifdef Q_OS_ANDROID
    QSKIP("Not supported on Android");
#else

    // based on testcase provided in QTBUG-39488
    QByteArray msg = "Hello World";

    QProcess process;
    process.start("write-read-write/write-read-write", QStringList() << msg);
    QVERIFY(process.waitForStarted(5000));
    QVERIFY(process.waitForReadyRead(5000));
    QCOMPARE(process.readAll().trimmed(), msg);

    // run the GUI event dispatcher once
    QSignalSpy spy(&process, SIGNAL(readyRead()));
    qApp->processEvents(QEventLoop::AllEvents, 100);

    // we mustn't have read anything in the event loop
    QCOMPARE(spy.size(), 0);

    // ensure the process hasn't died
    QVERIFY(!process.waitForFinished(250));

    // we mustn't have read anything during waitForFinished either
    QCOMPARE(spy.size(), 0);

    // release the child for the second write
    process.write("\n");
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(int(process.exitStatus()), int(QProcess::NormalExit));
    QCOMPARE(process.exitCode(), 0);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(process.readAll().trimmed(), msg);
#endif
}

QTEST_MAIN(tst_QProcess_and_GuiEventLoop)

#include "tst_qprocess_and_guieventloop.moc"
