// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSJSCALLBATCHTRACE_H
#define QOHOSJSCALLBATCHTRACE_H

#include <QtCore/qglobal.h>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhos {

// logs what QtOhos::JsCallBatch batches in the qt.ohos.jscallbatch category, used on the Qt thread only
class JsCallBatchTrace
{
public:
    void recordBatchStart(const char *batchContextName);
    void recordBatchEnd(const char *batchContextName, std::size_t queuedCallCount);
    void recordQueuedCall(const std::string &callerContextName);
    void recordBlockingCall(std::size_t queuedCallCount, const std::string &callerContextName);
    void recordRunForcedBy(std::size_t queuedCallCount, const std::string &runCause);

private:
    struct TracedBatch
    {
        const char *contextName;
        int callCount = 0;
        int firstQueuedCall = 1;
        int roundTripCount = 0;
        std::optional<std::string> optPendingSplitCause;
        std::vector<std::string> splitCauses;
    };

    void recordRunAtEnd(std::size_t queuedCallCount);
    void logSummary() const;
    std::string describeQueuedCalls(std::size_t queuedCallCount) const;
    static std::string describeCount(int count, const char *noun);

    int m_liveBatchCount = 0;
    std::optional<TracedBatch> m_optTracedBatch;
};

}

QT_END_NAMESPACE

#endif
