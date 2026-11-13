// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "hdc.h"

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

QString Hdc::run(const QStringList &args, bool printOnFailure) const
{
    static constexpr std::chrono::milliseconds startTimeout = 5s;
    static constexpr std::chrono::milliseconds finishTimeout = 30s;

    QProcess process;
    const QStringList allArgs = arguments(args);
    process.start(m_hdcPath, allArgs);
    if (!process.waitForStarted(int(startTimeout.count()))) {
        if (printOnFailure) {
            fprintf(stderr, "harmonyostestrunner: failed to start hdc: %s\n",
                qPrintable(m_hdcPath));
        }
        return {};
    }
    process.waitForFinished(int(finishTimeout.count()));
    if (printOnFailure) {
        const QByteArray standardError = process.readAllStandardError();
        if (!standardError.isEmpty()) {
            fprintf(stderr, "hdc %s stderr: %s\n", qPrintable(allArgs.join(u' ')),
                standardError.constData());
        }
    }
    return QString::fromUtf8(process.readAllStandardOutput());
}

QString Hdc::shell(const QStringList &command, bool printOnFailure) const
{
    return run(shellCommandArgs(command), printOnFailure);
}

QString Hdc::receiveFile(const QString &devicePath, const QString &localPath) const
{
    return run({ u"file"_s, u"recv"_s, devicePath, localPath }, true);
}

QT_END_NAMESPACE
