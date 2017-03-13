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
#include <QtCore/QProcess>

class tst_QProcess : public QObject
{
    Q_OBJECT

private slots:

    void echoTest_performance();
};

void tst_QProcess::echoTest_performance()
{
    QProcess process;
    process.start("testProcessLoopback/testProcessLoopback");

    QByteArray array;
    array.resize(1024 * 1024);
    for (int j = 0; j < array.size(); ++j)
        array[j] = 'a' + (j % 20);

    QVERIFY(process.waitForStarted());

    QTime stopWatch;
    stopWatch.start();

    qint64 totalBytes = 0;
    QByteArray dump;
    QSignalSpy readyReadSpy(&process, SIGNAL(readyRead()));
    QVERIFY(readyReadSpy.isValid());
    while (stopWatch.elapsed() < 2000) {
        process.write(array);
        while (process.bytesToWrite() > 0) {
            int readCount = readyReadSpy.count();
            QVERIFY(process.waitForBytesWritten(5000));
            if (readyReadSpy.count() == readCount)
                QVERIFY(process.waitForReadyRead(5000));
        }

        while (process.bytesAvailable() < array.size())
            QVERIFY2(process.waitForReadyRead(5000), qPrintable(process.errorString()));
        dump = process.readAll();
        totalBytes += dump.size();
    }

    qDebug() << "Elapsed time:" << stopWatch.elapsed() << "ms;"
             << "transfer rate:" << totalBytes / (1048.576) / stopWatch.elapsed()
             << "MB/s";

    for (int j = 0; j < array.size(); ++j)
        QCOMPARE(char(dump.at(j)), char('a' + (j % 20)));

    process.closeWriteChannel();
    QVERIFY(process.waitForFinished());
}

QTEST_MAIN(tst_QProcess)
#include "tst_bench_qprocess.moc"
