// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSMTBLOCKINGCALLSGATEWAY_H
#define QOHOSMTBLOCKINGCALLSGATEWAY_H

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

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtOhos {

template<typename SlaveContext>
class QOhosMtBlockingCallsGateway : public std::enable_shared_from_this<QOhosMtBlockingCallsGateway<SlaveContext>>
{
public:
    enum class MasterThreadTaskResult
    {
        CancelledByDeadlock,
        TimeoutedBeforeStart,
        TimeoutedAfterStart,
        Finished,
    };

    static std::shared_ptr<QOhosMtBlockingCallsGateway<SlaveContext>> makeInstance(
        QOhosConsumer<std::function<void()>> masterThreadTasksExecutor,
        QOhosConsumer<std::function<void(SlaveContext &)>> slaveThreadTasksExecutor);

    void invokeInMasterThread(std::function<void()> &&task);
    void invokeInSlaveThread(std::function<void(SlaveContext &)> &&task);

    void runInSlaveThreadAndWaitForContinue(
        std::function<void(SlaveContext &, QOhosTaskPromise<>)> &&task,
        const std::string &callerContextName = std::string());
    Q_REQUIRED_RESULT MasterThreadTaskResult tryInvokeInMasterThreadAndTryWaitForContinue(
        QOhosConsumer<std::function<void()>> &&task,
        std::chrono::nanoseconds timeout);

private:
    struct MasterThreadTaskState
    {
        bool started = false;
        std::optional<MasterThreadTaskResult> result;
        std::condition_variable resultSetCondVar;
    };

    QOhosMtBlockingCallsGateway(
        QOhosConsumer<std::function<void()>> masterThreadTasksExecutor,
        QOhosConsumer<std::function<void(SlaveContext &)>> slaveThreadTasksExecutor);

    QOhosConsumer<std::function<void()>> m_masterThreadTasksExecutor;
    QOhosConsumer<std::function<void(SlaveContext &)>> m_slaveThreadTasksExecutor;
    std::mutex m_waitStateMutex;
    bool m_masterWaiting = false;
    std::shared_ptr<MasterThreadTaskState> m_masterThreadTaskState;
};

template<typename SlaveContext>
std::shared_ptr<QOhosMtBlockingCallsGateway<SlaveContext>> QOhosMtBlockingCallsGateway<SlaveContext>::makeInstance(
    QOhosConsumer<std::function<void()>> masterThreadTasksExecutor,
    QOhosConsumer<std::function<void(SlaveContext &)>> slaveThreadTasksExecutor)
{
    return std::shared_ptr<QOhosMtBlockingCallsGateway<SlaveContext>>(
        new QOhosMtBlockingCallsGateway<SlaveContext>(
            std::move(masterThreadTasksExecutor), std::move(slaveThreadTasksExecutor)));
}

template<typename SlaveContext>
QOhosMtBlockingCallsGateway<SlaveContext>::QOhosMtBlockingCallsGateway(
    QOhosConsumer<std::function<void()>> masterThreadTasksExecutor,
    QOhosConsumer<std::function<void(SlaveContext &)>> slaveThreadTasksExecutor)
    : m_masterThreadTasksExecutor(std::move(masterThreadTasksExecutor))
    , m_slaveThreadTasksExecutor(std::move(slaveThreadTasksExecutor))
{
}

template<typename SlaveContext>
void QOhosMtBlockingCallsGateway<SlaveContext>::invokeInMasterThread(std::function<void()> &&task)
{
    m_masterThreadTasksExecutor(std::move(task));
}

template<typename SlaveContext>
void QOhosMtBlockingCallsGateway<SlaveContext>::invokeInSlaveThread(std::function<void(SlaveContext &)> &&task)
{
    m_slaveThreadTasksExecutor(std::move(task));
}

template<typename SlaveContext>
void QOhosMtBlockingCallsGateway<SlaveContext>::runInSlaveThreadAndWaitForContinue(
    std::function<void(SlaveContext &, QOhosTaskPromise<>)> &&task,
    const std::string &callerContextName)
{
    const auto funcInfo = Q_FUNC_INFO;

    {
        std::lock_guard<std::mutex> waitStateLock(m_waitStateMutex);
        if (m_masterThreadTaskState) {
            auto taskState = std::exchange(m_masterThreadTaskState, nullptr);
            taskState->result = MasterThreadTaskResult::CancelledByDeadlock;
            taskState->resultSetCondVar.notify_all();
        }
        m_masterWaiting = true;
    }

    auto taskFinishedPromise = std::make_shared<std::promise<void>>();
    auto taskFinishedFuture = taskFinishedPromise->get_future();

    auto sharedTaskPromise = std::make_shared<QOhosTaskPromise<>>(
        [taskFinishedPromise]() {
            taskFinishedPromise->set_value();
        },
        [funcInfo, callerContextName]() {
            qOhosReportFatalErrorAndAbort(
                "%s: promise destroyed without notifying the caller: %s",
                funcInfo, callerContextName.c_str());
        },
        callerContextName);

    invokeInSlaveThread(
        [task = std::move(task), sharedTaskPromise](SlaveContext &context) {
            task(context, std::move(*sharedTaskPromise));
        });
    taskFinishedFuture.wait();

    {
        std::lock_guard<std::mutex> waitStateLock(m_waitStateMutex);
        m_masterWaiting = false;
    }
}

template<typename SlaveContext>
typename QOhosMtBlockingCallsGateway<SlaveContext>::MasterThreadTaskResult
QOhosMtBlockingCallsGateway<SlaveContext>::tryInvokeInMasterThreadAndTryWaitForContinue(
    QOhosConsumer<std::function<void()>> &&task,
    std::chrono::nanoseconds timeout)
{
    auto taskState = std::make_shared<MasterThreadTaskState>();

    {
        std::lock_guard<std::mutex> waitStateLock(m_waitStateMutex);
        if (m_masterWaiting)
            return MasterThreadTaskResult::CancelledByDeadlock;
        m_masterThreadTaskState = taskState;
    }

    auto weakSelf = std::weak_ptr<QOhosMtBlockingCallsGateway<SlaveContext>>(this->shared_from_this());

    auto continueFunc = [weakSelf, taskState]() {
        auto self = weakSelf.lock();
        if (self) {
            std::lock_guard<std::mutex> waitStateLock(self->m_waitStateMutex);
            if (self->m_masterThreadTaskState == taskState) {
                self->m_masterThreadTaskState.reset();
                taskState->result = MasterThreadTaskResult::Finished;
                taskState->resultSetCondVar.notify_all();
            }
        }
    };

    invokeInMasterThread(
        [weakSelf, taskState, task = std::move(task), continueFunc = std::move(continueFunc)]() mutable {
            auto self = weakSelf.lock();
            if (self) {
                bool upToDate;
                {
                    std::lock_guard<std::mutex> waitStateLock(self->m_waitStateMutex);
                    if (self->m_masterThreadTaskState == taskState)
                        self->m_masterThreadTaskState->started = true;
                    upToDate = self->m_masterThreadTaskState == taskState;
                }
                if (upToDate)
                    task(std::move(continueFunc));
            }
        });

    MasterThreadTaskResult result;
    {
        std::unique_lock<std::mutex> waitStateLock(m_waitStateMutex);
        taskState->resultSetCondVar.wait_for(
            waitStateLock, timeout,
            [&]() {
                return taskState->result.has_value();
            });
        result = taskState->result.value_or(
            taskState->started
                ? MasterThreadTaskResult::TimeoutedAfterStart
                : MasterThreadTaskResult::TimeoutedBeforeStart);
    }

    return result;
}

}

QT_END_NAMESPACE

#endif
