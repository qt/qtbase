// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "shellhelpers.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

bool isProcessAlive(const Hdc &hdc, const QString &bundleName)
{
    return !hdc.shell({u"pidof"_s, bundleName}).trimmed().isEmpty();
}

// wc -c + tail -c +N runs device-side; both use plain read() syscalls (SELinux
// permits, unlike inotify), and one persistent hdc connection avoids per-poll
// reconnect overhead.
bool setupStdoutLogger(QProcess &stdoutLogger, const Hdc &hdc,
                              const QString &shellStdoutPath)
{
    const QString loop =
        u"sz=0; f='"_s + shellStdoutPath
        + u"'; while true; do"
           " new=$(wc -c < \"$f\" 2>/dev/null);"
           " if [ \"${new:-0}\" -gt \"$sz\" ]; then"
           " tail -c +$((sz+1)) \"$f\" 2>/dev/null; sz=$new;"
           " fi; sleep 0.1; done"_s;

    // SeparateChannels so we can scan stdout for PASS/FAIL while forwarding it.
    stdoutLogger.setProcessChannelMode(QProcess::SeparateChannels);
    stdoutLogger.start(hdc.program(), hdc.shellArguments({loop}));
    return stdoutLogger.waitForStarted(5000);
}

QString readDeviceFile(const Hdc &hdc, const QString &devicePath)
{
    return hdc.shell({ u"cat"_s, devicePath });
}

QT_END_NAMESPACE
