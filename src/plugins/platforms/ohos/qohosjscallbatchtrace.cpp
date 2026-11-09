// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosjscallbatchtrace.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QtOhos {

Q_STATIC_LOGGING_CATEGORY(lcJsCallBatch, "qt.ohos.jscallbatch", QtWarningMsg)

void JsCallBatchTrace::recordBatchStart(const char *batchContextName)
{
    ++m_liveBatchCount;
    if (m_liveBatchCount == 1 && (lcJsCallBatch().isInfoEnabled() || lcJsCallBatch().isDebugEnabled())) {
        m_optTracedBatch = TracedBatch{
            .contextName = batchContextName,
        };
    }
}

void JsCallBatchTrace::recordBatchEnd(const char *batchContextName, std::size_t queuedCallCount)
{
    --m_liveBatchCount;
    if (m_optTracedBatch) {
        if (m_liveBatchCount > 0) {
            recordRunForcedBy(queuedCallCount, std::string("end of nested batch ") + batchContextName);
        } else {
            recordRunAtEnd(queuedCallCount);
            logSummary();
            m_optTracedBatch.reset();
        }
    }
}

void JsCallBatchTrace::recordQueuedCall(const std::string &callerContextName)
{
    if (!m_optTracedBatch)
        return;

    auto &tracedBatch = *m_optTracedBatch;
    if (tracedBatch.optPendingSplitCause)
        tracedBatch.splitCauses.push_back(*std::exchange(tracedBatch.optPendingSplitCause, std::nullopt));

    ++tracedBatch.callCount;
    qCDebug(
        lcJsCallBatch, "batch %s: call %d queued: %s",
        tracedBatch.contextName, tracedBatch.callCount, callerContextName.c_str());
}

void JsCallBatchTrace::recordBlockingCall(std::size_t queuedCallCount, const std::string &callerContextName)
{
    if (!m_optTracedBatch)
        return;

    auto &tracedBatch = *m_optTracedBatch;
    ++tracedBatch.roundTripCount;
    ++tracedBatch.callCount;
    if (queuedCallCount == 0) {
        qCDebug(
            lcJsCallBatch, "batch %s: round trip %d runs only call %d: %s",
            tracedBatch.contextName, tracedBatch.roundTripCount, tracedBatch.callCount, callerContextName.c_str());
    } else {
        tracedBatch.optPendingSplitCause = callerContextName;
        qCDebug(
            lcJsCallBatch, "batch %s: round trip %d runs %s and then call %d: %s",
            tracedBatch.contextName, tracedBatch.roundTripCount, describeQueuedCalls(queuedCallCount).c_str(),
            tracedBatch.callCount, callerContextName.c_str());
    }
    tracedBatch.firstQueuedCall = tracedBatch.callCount + 1;
}

void JsCallBatchTrace::recordRunForcedBy(std::size_t queuedCallCount, const std::string &runCause)
{
    if (!m_optTracedBatch || queuedCallCount == 0)
        return;

    auto &tracedBatch = *m_optTracedBatch;
    ++tracedBatch.roundTripCount;
    tracedBatch.optPendingSplitCause = runCause;
    qCDebug(
        lcJsCallBatch, "batch %s: round trip %d runs %s, forced by %s",
        tracedBatch.contextName, tracedBatch.roundTripCount, describeQueuedCalls(queuedCallCount).c_str(),
        runCause.c_str());
    tracedBatch.firstQueuedCall = tracedBatch.callCount + 1;
}

void JsCallBatchTrace::recordRunAtEnd(std::size_t queuedCallCount)
{
    if (queuedCallCount == 0)
        return;

    auto &tracedBatch = *m_optTracedBatch;
    ++tracedBatch.roundTripCount;
    qCDebug(
        lcJsCallBatch, "batch %s: round trip %d runs %s at the end of the batch",
        tracedBatch.contextName, tracedBatch.roundTripCount, describeQueuedCalls(queuedCallCount).c_str());
}

void JsCallBatchTrace::logSummary() const
{
    const auto &tracedBatch = *m_optTracedBatch;
    if (tracedBatch.callCount == 0) {
        qCInfo(lcJsCallBatch, "summary of batch %s: no calls", tracedBatch.contextName);
        return;
    }

    auto summary = describeCount(tracedBatch.callCount, "call") + " in "
        + describeCount(tracedBatch.roundTripCount, "round trip");
    for (const auto &splitCause : tracedBatch.splitCauses)
        summary += "; split by " + splitCause;

    qCInfo(lcJsCallBatch, "summary of batch %s: %s", tracedBatch.contextName, summary.c_str());
}

std::string JsCallBatchTrace::describeQueuedCalls(std::size_t queuedCallCount) const
{
    const auto firstCall = m_optTracedBatch->firstQueuedCall;
    if (queuedCallCount == 1)
        return "call " + std::to_string(firstCall);

    const auto lastCall = firstCall + static_cast<int>(queuedCallCount) - 1;
    return "calls " + std::to_string(firstCall) + "-" + std::to_string(lastCall);
}

std::string JsCallBatchTrace::describeCount(int count, const char *noun)
{
    return std::to_string(count) + " " + noun + (count == 1 ? "" : "s");
}

}

QT_END_NAMESPACE
