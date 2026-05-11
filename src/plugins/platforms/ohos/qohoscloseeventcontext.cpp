// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoscloseeventcontext_p.h"
#include <QtCore/qscopeguard.h>
#include <QtGui/qevent.h>
#include <stack>

QT_BEGIN_NAMESPACE

namespace {

struct EventContextData
{
    QOhosCloseEventContext::CloseRootCause closeRootCause;
    QOhosConsumer<QOhosCloseEventContext::CloseResolution> closeResolutionConsumer;
};

thread_local std::stack<EventContextData> eventContextDataStack;

}

void QOhosCloseEventContext::runWithCloseRootCauseSet(CloseRootCause rootCause, const std::function<void()> &task)
{
    runWithCloseRootCauseAndCloseResolutionConsumerSet(rootCause, makeQOhosNoOpConsumer(), task);
}

void QOhosCloseEventContext::runWithCloseRootCauseAndCloseResolutionConsumerSet(
    CloseRootCause rootCause, QOhosConsumer<CloseResolution> closeResolutionConsumer, const std::function<void()> &task)
{
    eventContextDataStack.push(
        EventContextData{
            .closeRootCause = rootCause,
            .closeResolutionConsumer = std::move(closeResolutionConsumer),
        });
    auto popGuard = qScopeGuard(
        [&]() {
            eventContextDataStack.pop();
        });
    task();
}

QOhosCloseEventContext::CloseRootCause QOhosCloseEventContext::getCloseRootCauseForEventOrDefault(QEvent *event)
{
    Q_UNUSED(event);
    return CloseRootCause::NotSpecified;
}

void QOhosCloseEventContext::notifyCloseResolutionFromEventIfValid(QEvent *event)
{
    Q_UNUSED(event);
}

QT_END_NAMESPACE
