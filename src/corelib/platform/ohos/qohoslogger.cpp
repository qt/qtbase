// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qohoslogger_p.h>
#include <array>
#include <cstdio>
#include <cstring>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QtForOhos, "qt.ohos", QtDebugMsg)

void qOhosLogMessage(LogLevel logLevel, const char *tag, const char *message)
{
    OH_LOG_Print(LOG_APP, logLevel, LOG_DOMAIN, tag, "%{public}s", message);
}

void qOhosVPrintf(LogLevel logLevel, const char *format, std::va_list ap)
{
    std::array<char, 4096> entryBuffer;

    int prefixVsnprintfRes = std::snprintf(
        entryBuffer.data(), entryBuffer.size(),
        "[QtForOhos]: T: 0x%llx, M: ",
        reinterpret_cast<unsigned long long>(QThread::currentThreadId()));

    Q_ASSERT(prefixVsnprintfRes >= 0);

    auto prefixSize = std::min(static_cast<size_t>(prefixVsnprintfRes), entryBuffer.size() - 1);

    auto *msgBuffer = entryBuffer.data() + prefixSize;
    auto msgBufferSize = entryBuffer.size() - prefixSize;

    int vsnprintfRes = std::vsnprintf(msgBuffer, msgBufferSize, format, ap);
    if (vsnprintfRes < 0) {
        std::snprintf(msgBuffer, msgBufferSize, "[error formatting log msg: %s]", format);
    } else if (static_cast<unsigned>(vsnprintfRes) >= msgBufferSize) {
        constexpr auto *ellipsis = "...";
        std::strcpy(msgBuffer + msgBufferSize - (std::strlen(ellipsis) + 1), ellipsis);
    }

    qOhosLogMessage(logLevel, "OHOS plugin", entryBuffer.data());
}

QT_END_NAMESPACE
