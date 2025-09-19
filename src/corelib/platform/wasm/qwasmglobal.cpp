// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qglobalstatic.h>
#include "qwasmglobal_p.h"

namespace qwasmglobal {

Q_GLOBAL_STATIC(emscripten::ProxyingQueue, proxyingQueue);

namespace {
    void trampoline(void *context) {

        auto async_fn = [](void *context){
            std::function<void(void)> *fn = reinterpret_cast<std::function<void(void)> *>(context);
            (*fn)();
            delete fn;
        };

        emscripten_async_call(async_fn, context, 0);
    }
}

void runOnMainThread(std::function<void(void)> fn)
{
#if QT_CONFIG(thread)
    runTaskOnMainThread<void>(fn, proxyingQueue());
#else
    runTaskOnMainThread<void>(fn);
#endif
}

// Runs a function asynchronously. Main thread only.
void runAsync(std::function<void(void)> fn)
{
    Q_ASSERT(emscripten_is_main_runtime_thread());
    trampoline(new std::function<void(void)>(fn));
}

// Runs a function on the main thread. The function always runs asynchronously,
// also if the calling thread is the main thread.
void runOnMainThreadAsync(std::function<void(void)> fn)
{
    void *context = new std::function<void(void)>(fn);
#if QT_CONFIG(thread)
    if (!emscripten_is_main_runtime_thread()) {
        proxyingQueue()->proxyAsync(emscripten_main_runtime_thread_id(), [context]{
            trampoline(context);
        });
        return;
    }
#endif
    trampoline(context);
}

}

