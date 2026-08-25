// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "shellhelpers.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

void forceStopBundle(const Hdc &hdc, const QString &bundleName)
{
    hdc.shell({ u"aa"_s, u"force-stop"_s, bundleName });
}

bool isProcessAlive(const Hdc &hdc, const QString &bundleName)
{
    return !hdc.shell({u"pidof"_s, bundleName}).trimmed().isEmpty();
}

// wc -c + tail -c +N runs device-side; both use plain read() syscalls (SELinux
// permits, unlike inotify), and one persistent hdc connection avoids per-poll
// reconnect overhead.
std::unique_ptr<QProcess> streamDeviceFileWhileAppRuns(
    const Hdc &hdc, const QString &devicePath, const QString &bundleName)
{
    static constexpr int startTimeoutMs = 5000;

    const QString followFileFromStart = u"sz=0; f='"_s + devicePath + u"';"_s;
    const QString definePrintNewBytes =
        u" dump() { new=$(wc -c < \"$f\" 2>/dev/null);"
         " if [ \"${new:-0}\" -gt \"$sz\" ]; then"
         " tail -c +$((sz+1)) \"$f\" 2>/dev/null | head -c $((new-sz)); sz=$new; fi; };"_s;
    const QString printNewBytesUntilAppExits =
        u" while true; do dump;"
         " pidof "_s + bundleName + u" >/dev/null 2>&1 || { dump; break; };"
         " sleep 0.1; done"_s;
    const QString streamUntilAppExits =
        followFileFromStart + definePrintNewBytes + printNewBytesUntilAppExits;

    auto process = std::make_unique<QProcess>();
    // SeparateChannels so we can scan stdout for PASS/FAIL while forwarding it.
    process->setProcessChannelMode(QProcess::SeparateChannels);
    process->start(hdc.program(), hdc.shellArguments({streamUntilAppExits}));
    if (!process->waitForStarted(startTimeoutMs))
        return {};
    return process;
}

QString readDeviceFile(const Hdc &hdc, const QString &devicePath)
{
    return hdc.shell({ u"cat"_s, devicePath });
}

QT_END_NAMESPACE
