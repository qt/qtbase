// Copyright (C) 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWAYLANDPIPEWRITEHELPER_H
#define QWAYLANDPIPEWRITEHELPER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <chrono>

QT_BEGIN_NAMESPACE

class QWaylandPipeWriteHelper
{
public:
    enum class SafeWriteResult {
        Ok,
        Timeout,
        Closed,
        Error
    };
    static SafeWriteResult safeWriteWithTimeout(int fd, const char *data, qsizetype len, qsizetype chunkSize, std::chrono::nanoseconds timeout);
};

QT_END_NAMESPACE

#endif
