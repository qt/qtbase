// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSBATCHINGREQUESTSHANDLER_H
#define QOHOSBATCHINGREQUESTSHANDLER_H

#include <QtCore/QtGlobal>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qobject.h>
#include <functional>
#include <memory>
#include <mutex>
#include <qohosplugincore.h>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace details_qohosbatchingrequestshandler_h {

inline QOhosConsumer<std::function<void()>> makeQtThreadTasksExecutor(QtOhos::QObjectThreadSafeRef qtContext)
{
    return [qtContext](std::function<void()> task) {
        qtContext.visitInQtThreadIfAlive(
            [task = std::move(task)](auto &) {
                task();
            });
    };
}

template<typename Batch>
class QtOhosBatchingAsyncMTRequestsHandler : public std::enable_shared_from_this<QtOhosBatchingAsyncMTRequestsHandler<Batch>>
{
public:
    QtOhosBatchingAsyncMTRequestsHandler(
        QOhosConsumer<std::function<void()>> tasksExecutor, std::function<void(Batch &&)> targetThreadHandleBatchFunc);

    void updateBatch(const std::function<void(Batch &)> &updateFunc);

private:
    void postRequestTaskIfNeeded();

    std::mutex m_batchMutex;
    Batch m_batch = {};
    bool m_batchPending = false;
    QOhosConsumer<std::function<void()>> m_tasksExecutor;
    std::function<void(Batch &&)> m_targetThreadHandleBatchFunc;
};

template<typename Batch>
QtOhosBatchingAsyncMTRequestsHandler<Batch>::QtOhosBatchingAsyncMTRequestsHandler(
    QOhosConsumer<std::function<void()>> tasksExecutor, std::function<void(Batch &&)> targetThreadHandleBatchFunc)
    : m_tasksExecutor(tasksExecutor)
    , m_targetThreadHandleBatchFunc(std::move(targetThreadHandleBatchFunc))
{
}

template<typename Batch>
void QtOhosBatchingAsyncMTRequestsHandler<Batch>::updateBatch(const std::function<void(Batch &)> &updateFunc)
{
    std::lock_guard<std::mutex> requestLock(m_batchMutex);

    updateFunc(m_batch);

    if (!m_batchPending) {
        auto weakSelf = std::weak_ptr<QtOhosBatchingAsyncMTRequestsHandler<Batch>>(this->shared_from_this());
        m_tasksExecutor(
            [weakSelf]() {
                auto self = weakSelf.lock();
                if (self) {
                    std::unique_ptr<Batch> optRequest;
                    {
                        std::lock_guard<std::mutex> requestLock(self->m_batchMutex);
                        if (self->m_batchPending) {
                            optRequest = std::make_unique<Batch>(std::exchange(self->m_batch, Batch()));
                            self->m_batchPending = false;
                        }
                    }
                    if (optRequest)
                        self->m_targetThreadHandleBatchFunc(std::move(*optRequest));
                }
            });
        m_batchPending = true;
    }
}

}

template<typename Request>
std::function<void(std::function<void(Request &)>)> makeQtOhosBatchingMTRequestsHandler(
    QOhosConsumer<std::function<void()>> tasksExecutor,
    QOhosConsumer<Request &&> targetThreadBatchConsumer);

template<typename QtRequest>
std::function<void(std::function<void(QtRequest &)>)> makeQtOhosBatchingQtRequestsHandler(
    QtOhos::QObjectThreadSafeRef qtContext, std::function<void(QtRequest &&)> qtHandleRequestFunc);

template<typename QtRequest>
QOhosConsumer<QtRequest> makeQtOhosSimpleBatchingQtRequestsHandler(
    QtOhos::QObjectThreadSafeRef qtContext, QOhosConsumer<std::vector<QtRequest>> qtThreadBatchConsumer);

template<typename Request>
QOhosConsumer<Request> makeQtOhosSimpleBatchingMTRequestsHandler(
    QOhosConsumer<std::function<void()>> tasksExecutor,
    QOhosConsumer<std::vector<Request>> targetThreadBatchConsumer);

template<typename Request>
std::function<void(std::function<void(Request &)>)> makeQtOhosBatchingMTRequestsHandler(
    QOhosConsumer<std::function<void()>> tasksExecutor,
    QOhosConsumer<Request &&> targetThreadBatchConsumer)
{
    using namespace details_qohosbatchingrequestshandler_h;
    auto requestsHandler = std::make_shared<QtOhosBatchingAsyncMTRequestsHandler<Request>>(
        std::move(tasksExecutor), std::move(targetThreadBatchConsumer));
    return [requestsHandler](const std::function<void(Request &)> &updateFunc) {
        requestsHandler->updateBatch(updateFunc);
    };
}

template<typename QtRequest>
std::function<void(std::function<void(QtRequest &)>)> makeQtOhosBatchingQtRequestsHandler(
    QtOhos::QObjectThreadSafeRef qtContext, std::function<void(QtRequest &&)> qtHandleRequestFunc)
{
    using namespace details_qohosbatchingrequestshandler_h;
    return makeQtOhosBatchingMTRequestsHandler(
        makeQtThreadTasksExecutor(qtContext), std::move(qtHandleRequestFunc));
}

template<typename QtRequest>
QOhosConsumer<QtRequest> makeQtOhosSimpleBatchingQtRequestsHandler(
    QtOhos::QObjectThreadSafeRef qtContext, QOhosConsumer<std::vector<QtRequest>> qtThreadBatchConsumer)
{
    using namespace details_qohosbatchingrequestshandler_h;
    return makeQtOhosSimpleBatchingMTRequestsHandler(
        makeQtThreadTasksExecutor(qtContext), std::move(qtThreadBatchConsumer));
}

template<typename Request>
QOhosConsumer<Request> makeQtOhosSimpleBatchingMTRequestsHandler(
    QOhosConsumer<std::function<void()>> tasksExecutor,
    QOhosConsumer<std::vector<Request>> targetThreadBatchConsumer)
{
    using namespace details_qohosbatchingrequestshandler_h;
    auto baseRequestsHandler = std::make_shared<QtOhosBatchingAsyncMTRequestsHandler<std::vector<Request>>>(
        std::move(tasksExecutor), std::move(targetThreadBatchConsumer));
    return [baseRequestsHandler = std::move(baseRequestsHandler)](Request request) {
        baseRequestsHandler->updateBatch(
            [&](std::vector<Request> &batch) {
                batch.push_back(std::move(request));
            });
    };
}

QT_END_NAMESPACE

#endif
