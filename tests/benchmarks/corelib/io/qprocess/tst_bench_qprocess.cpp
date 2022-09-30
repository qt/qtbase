// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QSignalSpy>
#include <QtCore/QProcess>
#include <QtCore/QElapsedTimer>

class tst_QProcess : public QObject
{
    Q_OBJECT

private slots:

    void echoTest_performance();
};

#ifdef Q_OS_WIN
#  define EXE ".exe"
#else
#  define EXE ""
#endif

void tst_QProcess::echoTest_performance()
{
    QProcess process;
    process.start(QFINDTESTDATA("../testProcessLoopback/testProcessLoopback" EXE));

    QByteArray array;
    array.resize(1024 * 1024);
    for (int j = 0; j < array.size(); ++j)
        array[j] = 'a' + (j % 20);

    QVERIFY(process.waitForStarted());

    QElapsedTimer stopWatch;
    stopWatch.start();

    qint64 totalBytes = 0;
    QByteArray dump;
    QSignalSpy readyReadSpy(&process, SIGNAL(readyRead()));
    QVERIFY(readyReadSpy.isValid());
    while (stopWatch.elapsed() < 2000) {
        process.write(array);
        while (process.bytesToWrite() > 0) {
            int readCount = readyReadSpy.size();
            QVERIFY(process.waitForBytesWritten(5000));
            if (readyReadSpy.size() == readCount)
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
