// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtCore/QThread>

#include <stdlib.h>

static bool waitForDoneFileWritten(const QString &filePath, int msecs = 30000)
{
    QDeadlineTimer t(msecs);
    do {
        QThread::sleep(std::chrono::milliseconds{250});
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            continue;
        if (file.readAll() == "That's all folks!")
            return true;
    } while (!t.hasExpired());
    return false;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    if (argc < 4)
        return 13;

    QProcess process;

    QProcess::ProcessChannelMode mode = (QProcess::ProcessChannelMode)atoi(argv[1]);
    process.setProcessChannelMode(mode);
    if (process.processChannelMode() != mode)
        return 1;

    QProcess::InputChannelMode inmode = (QProcess::InputChannelMode)atoi(argv[2]);
    process.setInputChannelMode(inmode);
    if (process.inputChannelMode() != inmode)
        return 11;

    if (atoi(argv[3])) {
        QTemporaryFile doneFile("testForwarding_XXXXXX.txt");
        if (!doneFile.open())
            return 12;
        doneFile.close();

        process.setProgram("testForwardingHelper/testForwardingHelper");
        process.setArguments(QStringList(doneFile.fileName()));
        if (!process.startDetached())
            return 13;
        if (!waitForDoneFileWritten(doneFile.fileName()))
            return 14;
    } else {
        process.start("testProcessEcho2/testProcessEcho2");

        if (!process.waitForStarted(5000))
            return 2;

        if (inmode == QProcess::ManagedInputChannel && process.write("forwarded") != 9)
            return 3;

        process.closeWriteChannel();
        if (!process.waitForFinished(5000))
            return 4;

        if ((mode == QProcess::ForwardedOutputChannel || mode == QProcess::ForwardedChannels)
            && !process.readAllStandardOutput().isEmpty())
            return 5;
        if ((mode == QProcess::ForwardedErrorChannel || mode == QProcess::ForwardedChannels)
            && !process.readAllStandardError().isEmpty())
            return 6;
    }
    return 0;
}
