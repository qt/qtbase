// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "hdc.h"

#include <QtCore/qprocess.h>

#include <cstdio>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static constexpr int startTimeoutMs = 5000;
static constexpr int finishTimeoutMs = 30000;

Hdc::Hdc(QString hdcPath, QString connectKey)
    : m_hdcPath(std::move(hdcPath)), m_connectKey(std::move(connectKey))
{
}

QString Hdc::program() const
{
    return m_hdcPath;
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
    QProcess process;
    const QStringList allArgs = arguments(args);
    process.start(m_hdcPath, allArgs);
    if (!process.waitForStarted(startTimeoutMs)) {
        if (printOnFailure) {
            fprintf(stderr, "harmonyostestrunner: failed to start hdc: %s\n",
                qPrintable(m_hdcPath));
        }
        return {};
    }
    process.waitForFinished(finishTimeoutMs);
    if (printOnFailure) {
        const QByteArray standardError = process.readAllStandardError();
        if (!standardError.isEmpty()) {
            fprintf(stderr, "hdc %s stderr: %s\n", qPrintable(allArgs.join(u' ')),
                standardError.constData());
        }
    }
    return QString::fromUtf8(process.readAllStandardOutput());
}

QT_END_NAMESPACE
