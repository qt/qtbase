// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qioring_p.h"

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

QT_END_NAMESPACE

#include "moc_qioring_p.cpp"
