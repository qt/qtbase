// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testoutputforwarder.h"
#include "shellhelpers.h"

#include <QtCore/qdeadlinetimer.h>
#include <QtCore/qthread.h>

#include <chrono>
#include <cstdio>

QT_BEGIN_NAMESPACE

using namespace std::chrono_literals;

constexpr auto finalCopyRetryDelay = 1s;

TestOutputForwarder::TestOutputForwarder(Hdc hdc, QString devicePath, QString localCopyPath)
    : m_hdc(std::move(hdc)),
      m_devicePath(std::move(devicePath)),
      m_localCopyPath(std::move(localCopyPath))
{
}

q23::expected<QByteArray, QString> TestOutputForwarder::forwardAvailable()
{
    const q23::expected<QByteArray, QString> copiedOutput =
            copyDeviceFile(m_hdc, m_devicePath, m_localCopyPath);
    if (!copiedOutput)
        return q23::unexpected(copiedOutput.error());
    if (copiedOutput->size() <= m_forwardedBytes)
        return QByteArray();
    const QByteArray newOutput = copiedOutput->sliced(m_forwardedBytes);
    forward(newOutput);
    return newOutput;
}

bool TestOutputForwarder::forwardRemaining(qxp::function_ref<bool()> isInterrupted)
{
    const QDeadlineTimer deadline(hdcOutageTolerance);
    q23::expected<QByteArray, QString> copiedOutput =
            copyDeviceFile(m_hdc, m_devicePath, m_localCopyPath);
    while (!copiedOutput && !isInterrupted() && !deadline.hasExpired()
           && queryDeviceFileState(m_hdc, m_devicePath) != DeviceFileState::Absent) {
        QThread::sleep(finalCopyRetryDelay);
        copiedOutput = copyDeviceFile(m_hdc, m_devicePath, m_localCopyPath);
    }
    if (!copiedOutput) {
        reportUnreadOutput(copiedOutput.error());
        return false;
    }
    if (copiedOutput->size() < m_forwardedBytes) {
        fprintf(stderr, "harmonyostestrunner: forwarded %lld bytes of test output, but %s holds "
                "only %lld\n", static_cast<long long>(m_forwardedBytes), qPrintable(m_devicePath),
                static_cast<long long>(copiedOutput->size()));
        return false;
    }
    forward(copiedOutput->sliced(m_forwardedBytes));
    return true;
}

void TestOutputForwarder::forward(const QByteArray &output)
{
    if (output.isEmpty())
        return;
    fwrite(output.constData(), 1, static_cast<size_t>(output.size()), stdout);
    fflush(stdout);
    m_forwardedBytes += output.size();
}

void TestOutputForwarder::reportUnreadOutput(const QString &failure) const
{
    fprintf(stderr, "harmonyostestrunner: could not copy the test output from %s (%s), "
            "it is incomplete after %lld bytes\n", qPrintable(m_devicePath),
            qPrintable(failure), static_cast<long long>(m_forwardedBytes));
}

QT_END_NAMESPACE
