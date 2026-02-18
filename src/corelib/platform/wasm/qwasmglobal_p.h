// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QWASMGLOBAL_P_H
#define QWASMGLOBAL_P_H

#include <emscripten/proxying.h>
#include <emscripten/threading.h>

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

QT_BEGIN_NAMESPACE

namespace qwasmglobal {

#if QT_CONFIG(thread)
    template<class T>
    T proxyCall(std::function<T()> task, emscripten::ProxyingQueue *queue)
    {
        T result;
        queue->proxySync(emscripten_main_runtime_thread_id(),
                         [task, result = &result]() { *result = task(); });
        return result;
    }

    template<>
    inline void proxyCall<void>(std::function<void()> task, emscripten::ProxyingQueue *queue)
    {
        queue->proxySync(emscripten_main_runtime_thread_id(), task);
    }

    template<class T>
    T runTaskOnMainThread(std::function<T()> task, emscripten::ProxyingQueue *queue)
    {
        return emscripten_is_main_runtime_thread() ? task() : proxyCall<T>(std::move(task), queue);
    }

    template<class T>
    T runTaskOnMainThread(std::function<T()> task)
    {
        emscripten::ProxyingQueue singleUseQueue;
        return runTaskOnMainThread<T>(task, &singleUseQueue);
    }

#else
    template<class T>
    T runTaskOnMainThread(std::function<T()> task)
    {
        return task();
    }
#endif // QT_CONFIG(thread)

    Q_CORE_EXPORT void runAsync(std::function<void(void)> fn);
    Q_CORE_EXPORT void runOnMainThread(std::function<void(void)> fn);
    Q_CORE_EXPORT void runOnMainThreadAsync(std::function<void(void)> fn);
}

QT_END_NAMESPACE

#endif // QWASMGLOBAL_P_H
