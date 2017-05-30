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
#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QFile>
#include <QDir>

#include <stdio.h>

#if defined(Q_OS_UNIX)
#include <sys/types.h>
#include <unistd.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

static void writeStuff(QFile &f)
{
    f.write(QDir::currentPath().toUtf8());
    f.putChar('\n');
#if defined(Q_OS_UNIX)
    f.write(QByteArray::number(quint64(getpid())));
#elif defined(Q_OS_WIN)
    f.write(QByteArray::number(quint64(GetCurrentProcessId())));
#endif
    f.putChar('\n');
    f.write(qgetenv("tst_QProcess"));
    f.putChar('\n');
}

struct Args
{
    int exitCode = 0;
    QByteArray errorMessage;
    QString fileName;
    FILE *channel = nullptr;
    QByteArray channelName;
};

static Args parseArguments(const QStringList &args)
{
    Args result;
    if (args.count() < 2) {
        result.exitCode = 128;
        result.errorMessage = "Usage: testDetached [--out-channel={stdout|stderr}] filename.txt\n";
        return result;
    }
    for (const QString &arg : args) {
        if (arg.startsWith("--")) {
            if (!arg.startsWith("--out-channel=")) {
                result.exitCode = 2;
                result.errorMessage = "Unknown argument " + arg.toLocal8Bit();
                return result;
            }
            result.channelName = arg.mid(14).toLocal8Bit();
            if (result.channelName == "stdout") {
                result.channel = stdout;
            } else if (result.channelName == "stderr") {
                result.channel = stderr;
            } else {
                result.exitCode = 3;
                result.errorMessage = "Unknown channel " + result.channelName;
                return result;
            }
        } else {
            result.fileName = arg;
        }
    }
    return result;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    const Args args = parseArguments(app.arguments());
    if (args.exitCode) {
        fprintf(stderr, "testDetached: %s\n", args.errorMessage.constData());
        return args.exitCode;
    }

    if (args.channel) {
        QFile channel;
        if (!channel.open(args.channel, QIODevice::WriteOnly | QIODevice::Text)) {
            fprintf(stderr, "Cannot open channel %s for writing: %s\n",
                    qPrintable(args.channelName), qPrintable(channel.errorString()));
            return 4;
        }
        writeStuff(channel);
    }

    QFile f(args.fileName);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        fprintf(stderr, "Cannot open %s for writing: %s\n",
                qPrintable(f.fileName()), qPrintable(f.errorString()));
        return 1;
    }

    writeStuff(f);
    f.close();

    return 0;
}
