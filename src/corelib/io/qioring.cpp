// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qioring_p.h"

#include <QtCore/q26numeric.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQIORing, "qt.core.ioring", QtCriticalMsg)

QIORing *QIORing::sharedInstance()
{
    thread_local QIORing instance;
    if (!instance.initializeIORing())
        return nullptr;
    return &instance;
}

QIORing::QIORing(quint32 submissionQueueSize, quint32 completionQueueSize)
    : sqEntries(submissionQueueSize), cqEntries(completionQueueSize)
{
    // Destructor in respective _<platform>.cpp
}

auto QIORing::queueRequestInternal(GenericRequestType &request) -> QueuedRequestStatus
{
    if (!ensureInitialized() || preparingRequests) { // preparingRequests protects against recursing
                                                     // inside callbacks of synchronous completions.
        finishRequestWithError(request, QFileDevice::ResourceError);
        addrItMap.remove(&request);
        return QueuedRequestStatus::CompletedImmediately;
    }
    if (!lastUnqueuedIterator) {
        lastUnqueuedIterator.emplace(addrItMap[&request]);
    } else if (request.operation() == QtPrivate::Operation::Cancel) {
        // We want to fast-track cancellations because they may be cancelling
        // unqueued things, so we push it up front in the queue:
        auto &it = addrItMap[&request];
        const auto where = *lastUnqueuedIterator;
        pendingRequests.splice(where, pendingRequests, it);
        it = std::prev(where);
        lastUnqueuedIterator.emplace(it);
    }

    qCDebug(lcQIORing) << "Trying to submit request" << request.operation()
                       << "user data:" << std::addressof(request);
    prepareRequests();
    // If this is now true we have, in some way, fulfilled the request:
    const bool requestCompleted = !addrItMap.contains(&request);
    const QueuedRequestStatus requestQueuedState = requestCompleted
            ? QueuedRequestStatus::CompletedImmediately
            : QueuedRequestStatus::Pending;
    // We want to avoid notifying the kernel too often of tasks, so only do it if the queue is full,
    // otherwise do it when we return to the event loop.
    if (unstagedRequests == sqEntries && inFlightRequests <= cqEntries) {
        submitRequests();
        return requestQueuedState;
    }
    if (stagePending || unstagedRequests == 0)
        return requestQueuedState;
    stagePending = true;
    // We are not a QObject, but we always have the notifier, so use that for context:
    QMetaObject::invokeMethod(
            std::addressof(*notifier), [this] { submitRequests(); }, Qt::QueuedConnection);
    return requestQueuedState;
}

bool QIORing::waitForRequest(RequestHandle handle, QDeadlineTimer deadline)
{
    if (!handle || !addrItMap.contains(handle))
        return true; // : It was never there to begin with (so it is finished)
    if (unstagedRequests)
        submitRequests();
    completionReady(); // Try to process some pending completions
    while (!deadline.hasExpired() && addrItMap.contains(handle)) {
        if (!waitForCompletions(deadline))
            return false;
        completionReady();
    }
    return !addrItMap.contains(handle);
}

namespace QtPrivate {
template <typename T>
using DetectResult = decltype(std::declval<const T &>().result);

template <typename T>
constexpr bool HasResultMember = qxp::is_detected_v<DetectResult, T>;
}

void QIORing::setFileErrorResult(QIORing::GenericRequestType &req, QFileDevice::FileError error)
{
    invokeOnOp(req, [error](auto *concreteRequest) {
        if constexpr (QtPrivate::HasResultMember<decltype(*concreteRequest)>)
            setFileErrorResult(*concreteRequest, error);
    });
}

void QIORing::finishRequestWithError(QIORing::GenericRequestType &req, QFileDevice::FileError error)
{
    invokeOnOp(req, [error](auto *concreteRequest) {
        if constexpr (QtPrivate::HasResultMember<decltype(*concreteRequest)>)
            setFileErrorResult(*concreteRequest, error);
        invokeCallback(*concreteRequest);
    });
}

QIORing::ReadWriteStatus QIORing::handleReadCompletion(size_t value, QSpan<std::byte> *destinations,
                                                       void *voidExtra, SetResultFn setResultFn)
{
    if (value == 0) {
        // Since we are reading, presumably this indicates EOF.
        // In case this is our only callback, notify that it at least wasn't a
        // failure:
        setResultFn(qint64(0));
        return ReadWriteStatus::Finished;
    }
    if (auto *extra = static_cast<QtPrivate::ReadWriteExtra *>(voidExtra)) {
        const qsizetype bytesRead = q26::saturate_cast<qsizetype>(value);
        qCDebug(lcQIORing) << "Partial read of" << bytesRead << "bytes completed";
        extra->totalProcessed = setResultFn(bytesRead);
        // [0/1] Add the number of bytes processed to the spanOffset - we use this to test how many
        // spans were fully processed, and/or how far into the last span we have data.
        extra->spanOffset += bytesRead;
        qCDebug(lcQIORing) << "Read operation progress: span" << extra->spanIndex << "offset"
                           << extra->spanOffset << "of" << destinations[extra->spanIndex].size()
                           << "bytes. Total read:" << extra->totalProcessed << "bytes";
        // [1/1] We subtract the size of the spans, in order, here. When a span's size is larger
        // than the "spanOffset" it means we have a partially-used span. Otherwise it stops
        // processing when we run out of spans to check, or we hit spanOffset == 0.
        while (extra->spanOffset >= destinations[extra->spanIndex].size()) {
            extra->spanOffset -= destinations[extra->spanIndex].size();
            // Move to next span
            if (++extra->spanIndex == extra->numSpans)
                return ReadWriteStatus::Finished;
        }
        return ReadWriteStatus::MoreToDo;
    }
    setResultFn(q26::saturate_cast<qsizetype>(value));
    return ReadWriteStatus::Finished;
}

QIORing::ReadWriteStatus QIORing::handleWriteCompletion(size_t value,
                                                        const QSpan<const std::byte> *sources,
                                                        void *voidExtra, SetResultFn setResultFn)
{
    if (auto *extra = static_cast<QtPrivate::ReadWriteExtra *>(voidExtra)) {
        const qsizetype bytesWritten = q26::saturate_cast<qsizetype>(value);
        qCDebug(lcQIORing) << "Partial write of" << bytesWritten << "bytes completed";
        extra->totalProcessed = setResultFn(bytesWritten);
        // [0/1] Add the number of bytes processed to the spanOffset - we use this to test how many
        // spans were fully processed, and/or how far into the last span we have data.
        extra->spanOffset += bytesWritten;
        qCDebug(lcQIORing) << "Write operation progress: span" << extra->spanIndex << "offset"
                           << extra->spanOffset << "of" << sources[extra->spanIndex].size()
                           << "bytes. Total written:" << extra->totalProcessed << "bytes";
        // [1/1] We subtract the size of the spans, in order, here. When a span's size is larger
        // than the "spanOffset" it means we have a partially-used span. Otherwise it stops
        // processing when we run out of spans to check, or we hit spanOffset == 0.
        while (extra->spanOffset >= sources[extra->spanIndex].size()) {
            extra->spanOffset -= sources[extra->spanIndex].size();
            // Move to next span
            if (++extra->spanIndex == extra->numSpans)
                return ReadWriteStatus::Finished;
        }
        return ReadWriteStatus::MoreToDo;
    }
    setResultFn(q26::saturate_cast<qsizetype>(value));
    return ReadWriteStatus::Finished;
}

void QIORing::finalizeReadWriteCompletion(GenericRequestType *request, ReadWriteStatus rwstatus)
{
    switch (rwstatus) {
    case ReadWriteStatus::Finished:
        if (request->getExtra<void>())
            --ongoingSplitOperations;
        break;
    case ReadWriteStatus::MoreToDo: {
        // Move the request such that it is next in the list to be processed:
        auto &it = addrItMap[request];
        const auto where = lastUnqueuedIterator.value_or(pendingRequests.end());
        pendingRequests.splice(where, pendingRequests, it);
        it = std::prev(where);
        lastUnqueuedIterator = it;
        break;
    }
    }
}

QT_END_NAMESPACE

#include "moc_qioring_p.cpp"
