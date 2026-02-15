// Copyright (C) 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandpipewritehelper_p.h"
#include <QtCore/private/qcore_unix_p.h>

QT_BEGIN_NAMESPACE

QWaylandPipeWriteHelper::SafeWriteResult
QWaylandPipeWriteHelper::safeWriteWithTimeout(int fd,
                                              const char *data,
                                              qsizetype len,
                                              qsizetype chunkSize,
                                              std::chrono::nanoseconds timeout)
{
    if (len == 0)
        return SafeWriteResult::Ok;

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLOUT;

    QDeadlineTimer deadline(timeout);
    qsizetype offset = 0;
    while (offset < len) {
        int ready = qt_safe_poll(&pfd, 1, deadline);
        if (ready < 0) {
            return SafeWriteResult::Error;
        } else if (ready == 0 || deadline.hasExpired()) {
            return SafeWriteResult::Timeout;
        } else {
            const qsizetype toWrite = qMin(chunkSize, len - offset);
            ssize_t n = qt_safe_write_nosignal(fd, data + offset, toWrite);
            if (n > 0) {
                offset += n;
                continue;
            } else if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                if (errno == EPIPE)
                    return SafeWriteResult::Closed;
                return SafeWriteResult::Error;
            }
        }
    }

    return SafeWriteResult::Ok;
}

QT_END_NAMESPACE
