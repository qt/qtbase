// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "hdc.h"

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qprocess.h>

#include <chrono>
#include <cstdio>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace std::chrono_literals;

static QStringList shellCommandArgs(const QStringList &command)
{
    return QStringList{ u"shell"_s } + command;
}

Hdc::Hdc(QString hdcPath, QString connectKey)
    : m_hdcPath(std::move(hdcPath)), m_connectKey(std::move(connectKey))
{
}

QString Hdc::connectKey() const
{
    return m_connectKey;
}

QStringList Hdc::arguments(const QStringList &args) const
{
    QStringList allArgs;
    if (!m_connectKey.isEmpty())
        allArgs << u"-t"_s << m_connectKey;
    return allArgs + args;
}

QString Hdc::run(const QStringList &args) const
{
    static constexpr std::chrono::milliseconds startTimeout = 5s;
    static constexpr std::chrono::milliseconds finishTimeout = 30s;
    static constexpr std::chrono::milliseconds killTimeout = 1s;
    static constexpr std::chrono::milliseconds slowCall = 5s;
    static constexpr qsizetype loggedStdoutBytes = 80;

    QElapsedTimer timer;
    timer.start();

    QProcess process;
    const QStringList allArgs = arguments(args);
    process.start(m_hdcPath, allArgs);
    if (!process.waitForStarted(int(startTimeout.count()))) {
        fprintf(stderr, "harmonyostestrunner: failed to start hdc '%s' after %lld ms: %s\n",
            qPrintable(m_hdcPath), static_cast<long long>(timer.durationElapsed() / 1ms),
            qPrintable(process.errorString()));
        return {};
    }
    const std::chrono::nanoseconds startedAfter = timer.durationElapsed();
    const bool finished = process.waitForFinished(int(finishTimeout.count()));
    const std::chrono::nanoseconds callDuration = timer.durationElapsed();
    if (!finished) {
        process.kill();
        process.waitForFinished(int(killTimeout.count()));
    }
    const QByteArray standardOutput = process.readAllStandardOutput();
    const QByteArray standardError = process.readAllStandardError();
    if (!finished || callDuration >= slowCall || !standardError.isEmpty()) {
        fprintf(stderr, "harmonyostestrunner: hdc %s %s after %lld ms (started after %lld ms), "
            "stdout %lld bytes [%s], stderr: %s\n", qPrintable(allArgs.join(u' ')),
            finished ? "finished" : "timed out", static_cast<long long>(callDuration / 1ms),
            static_cast<long long>(startedAfter / 1ms),
            static_cast<long long>(standardOutput.size()),
            standardOutput.left(loggedStdoutBytes).simplified().constData(),
            standardError.trimmed().constData());
    }
    return QString::fromUtf8(standardOutput);
}

QString Hdc::shell(const QStringList &command) const
{
    return run(shellCommandArgs(command));
}

QString Hdc::receiveFile(const QString &devicePath, const QString &localPath) const
{
    return run({ u"file"_s, u"recv"_s, devicePath, localPath });
}

QT_END_NAMESPACE
