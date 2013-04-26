/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QProcess>

class tst_QProcess : public QObject
{
    Q_OBJECT

#if !defined(QT_NO_PROCESS) && !defined(Q_OS_WINCE)
private slots:

    void echoTest_performance();

#endif // QT_NO_PROCESS
};

#if !defined(QT_NO_PROCESS) && !defined(Q_OS_WINCE)
// Reading and writing to a process is not supported on Qt/CE
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

#endif // QT_NO_PROCESS && Q_OS_WINCE

QTEST_MAIN(tst_QProcess)
#include "tst_bench_qprocess.moc"
