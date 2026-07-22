// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qohoslogger_p.h>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <qohossinglethreadexecutor.h>
#include <queue>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtOhos {

namespace {

std::shared_ptr<::pthread_attr_t> makePthreadAttr()
{
    auto threadAttrStorage = std::make_shared<::pthread_attr_t>();
    auto threadAttr = std::shared_ptr<::pthread_attr_t>(
        threadAttrStorage.get(),
        [threadAttrStorage](::pthread_attr_t *attr) {
            ::pthread_attr_destroy(attr);
        });

    int initResult = ::pthread_attr_init(threadAttr.get());
    if (initResult != 0)
        qOhosReportFatalErrorAndAbort("pthread_attr_init() failed: %s", std::strerror(initResult));

    return threadAttr;
}

std::shared_ptr<void> startNewThread(
    std::function<void()> threadFunction, const ::pthread_attr_t &threadAttributes)
{
    struct Context
    {
        std::function<void()> threadFunction;
        std::optional<::pthread_t> optThreadId;
    };

    auto context = std::make_shared<Context>();
    context->threadFunction = std::move(threadFunction);

    auto threadHandle = makeDestroyNotifier(
        [context]() {
            if (context->optThreadId.has_value())
                ::pthread_join(context->optThreadId.value(), nullptr);
        });

    auto pthreadStartRoutineFunc = [](void *arg) -> void * {
        auto *context = static_cast<Context *>(arg);
        context->threadFunction();
        return nullptr;
    };

    ::pthread_t threadId;
    int createResult = ::pthread_create(&threadId, &threadAttributes, pthreadStartRoutineFunc, context.get());
    if (createResult != 0) {
        qOhosReportFatalErrorAndAbort(
            "%s: pthread_create() failed: %s", Q_FUNC_INFO, std::strerror(createResult));
    }

    context->optThreadId = threadId;

    return threadHandle;
}

std::shared_ptr<::pthread_attr_t> createSingleThreadExecutorThreadAttributes(
    const SingleThreadExecutorConfig &config)
{
    auto threadAttributes = makePthreadAttr();

    if (config.threadPreferredStackSize.has_value()) {
        int setStackSizeResult = ::pthread_attr_setstacksize(
            threadAttributes.get(), config.threadPreferredStackSize.value());
        if (setStackSizeResult != 0) {
            qOhosPrintfWarning(
                "%s: pthread_attr_setstacksize() failed: %s",
                Q_FUNC_INFO, std::strerror(setStackSizeResult));
        }
    }

    return threadAttributes;
}

class SingleThreadExecutor
{
public:
    SingleThreadExecutor(const SingleThreadExecutorConfig &config);

    ~SingleThreadExecutor();

    void enqueueTask(std::function<void()> task);

private:
    std::shared_ptr<void> m_workerThreadHandle;
    std::mutex m_tasksQueueMutex;
    std::queue<std::function<void()>> m_tasksQueue;
    std::condition_variable m_tasksQueueNonEmptyCv;
};

SingleThreadExecutor::SingleThreadExecutor(const SingleThreadExecutorConfig &config)
{
    m_workerThreadHandle = startNewThread(
        [this]() {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> tasksQueueLock(m_tasksQueueMutex);
                    m_tasksQueueNonEmptyCv.wait(
                        tasksQueueLock,
                        [&]() {
                            return !m_tasksQueue.empty();
                        });

                    task = std::move(m_tasksQueue.front());
                    m_tasksQueue.pop();
                }

                if (!task)
                    break;

                task();
            }
        },
        *createSingleThreadExecutorThreadAttributes(config));
}

SingleThreadExecutor::~SingleThreadExecutor()
{
    enqueueTask({});
    m_workerThreadHandle.reset();
}

void SingleThreadExecutor::enqueueTask(std::function<void()> task)
{
    std::lock_guard<std::mutex> tasksQueueLock(m_tasksQueueMutex);
    m_tasksQueue.push(std::move(task));
    m_tasksQueueNonEmptyCv.notify_one();
}

}

QOhosConsumer<std::function<void()>> makeSingleThreadExecutor(const SingleThreadExecutorConfig &config)
{
    auto executor = std::make_shared<SingleThreadExecutor>(config);
    return [executor](std::function<void()> task) {
        executor->enqueueTask(std::move(task));
    };
}

}

QT_END_NAMESPACE
