/****************************************************************************
**
** Copyright (C) 2016 Intel Corporation.
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

#include <QtGui/QGuiApplication>
#include <QtTest/QtTest>
#include <QtCore/QProcess>

class tst_QProcess_and_GuiEventLoop : public QObject
{
    Q_OBJECT
private slots:
    void waitForAndEventLoop();
};


void tst_QProcess_and_GuiEventLoop::waitForAndEventLoop()
{
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_EMBEDDED)
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
    QCOMPARE(spy.count(), 0);

    // ensure the process hasn't died
    QVERIFY(!process.waitForFinished(250));

    // we mustn't have read anything during waitForFinished either
    QCOMPARE(spy.count(), 0);

    // release the child for the second write
    process.write("\n");
    QVERIFY(process.waitForFinished(5000));
    QCOMPARE(int(process.exitStatus()), int(QProcess::NormalExit));
    QCOMPARE(process.exitCode(), 0);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(process.readAll().trimmed(), msg);
#endif
}

QTEST_MAIN(tst_QProcess_and_GuiEventLoop)

#include "tst_qprocess_and_guieventloop.moc"
