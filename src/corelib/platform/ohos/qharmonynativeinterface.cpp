// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qcoreapplication_platform.h>

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnativeinterface_p.h>
#include <QtCore/private/qohosjstools_p.h>
#include <functional>

QT_BEGIN_NAMESPACE

namespace {

void runTaskLoggingExceptions(
    QOhosJsState &jsState, const char *callerContextName,
    const std::function<void(QHarmonyJsContext &)> &task)
{
    runQOhosJsCodeLoggingExceptions(
        jsState.env(), callerContextName,
        [&]() {
            task(QHarmonyJsContext::fromEnv(jsState.env()));
        });
}

}

/*!
    \class QNativeInterface::QHarmonyApplication
    \brief Native interface to a core application on HarmonyOS.

    Accessed through QCoreApplication::nativeInterface().

    On HarmonyOS, the ArkTS runtime and the platform APIs run on the main
    thread of the process, called the JS thread. The Qt application runs on
    a separate thread, called the Qt thread. This is the thread of the
    QCoreApplication object. This interface runs application code on the JS
    thread, where the code receives a QHarmonyJsContext. It also brings
    results back to the Qt thread.

    Code that runs on the JS thread must follow these rules:
    \list
    \li It must not wait for the Qt thread, which may itself be waiting in
        runOnJsThreadAndWait(). Send results back asynchronously, with
        runOnQtThread() or through QPromise.
    \li It must not access Qt-thread objects directly, not even through a
        QPointer. A QPointer is created, checked, and dereferenced only on
        the thread of its object. The JS thread only passes copies of it
        along. The functor posted to the Qt thread checks the QPointer
        there.
    \li A JS callback may run after the Qt event loop has finished. Tasks
        that it queues with runOnQtThread() then never run.
    \li A \c napi_value that it obtains is valid until the task returns, and
        must not leave the JS thread. To keep a JS object longer, use a
        QHarmonyJsObjectRef.
    \li Resources bound to the JS engine, such as references and listeners,
        must be released on the JS thread.
    \li The JS thread also runs the platform UI, so tasks must be short.
    \endlist

    Each task runs inside its own Node-API handle scope. A C++ exception
    that a task throws and a JS exception that it leaves pending are caught
    and logged on the JS thread. A task that needs to react to a failure
    must handle it itself.

    \inmodule QtCore
    \inheaderfile QCoreApplication
    \ingroup native-interfaces
    \ingroup native-interfaces-qcoreapplication
*/
QT_DEFINE_NATIVE_INTERFACE(QHarmonyApplication);

/*!
    Queues \a task for execution on the JS thread and returns immediately.
    You can call this function from any thread. This is the primary way to
    reach the platform APIs. The task, with everything it captures, is
    destroyed on the JS thread.

    Tasks queued with runOnJsThread() and runOnJsThreadAndWait() run one at a
    time, in the order in which they were queued.
*/
void QNativeInterface::QHarmonyApplication::runOnJsThread(std::function<void(QHarmonyJsContext &)> task)
{
    const char *funcInfo = Q_FUNC_INFO;

    QOhosJsThreadGateway::invoke(
        [task = std::move(task), funcInfo](QOhosJsState &jsState) {
            runTaskLoggingExceptions(jsState, funcInfo, task);
        });
}

/*!
    Runs \a task on the JS thread and blocks the calling thread until
    \a task returns. You can call this function from any thread. \a task
    runs after the tasks queued before it with runOnJsThread() or
    runOnJsThreadAndWait(). When called on the JS thread, it runs \a task
    directly, before any task that is still queued. \a task must not wait for
    the calling thread.
*/
void QNativeInterface::QHarmonyApplication::runOnJsThreadAndWait(const std::function<void(QHarmonyJsContext &)> &task)
{
    const char *funcInfo = Q_FUNC_INFO;

    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
            runTaskLoggingExceptions(jsState, funcInfo, task);
        },
        funcInfo);
}

/*!
    \fn template <typename Func, typename Result> Result QNativeInterface::QHarmonyApplication::runOnJsThreadAndWait(Func &&task)

    Runs \a task on the JS thread, blocks the calling thread until \a task
    returns, and returns its result. \a task is ordered like in the overload
    without a result. If \a task throws an exception or leaves a JS
    exception pending, the exception is logged on the JS thread and a
    default-constructed value is returned. The result must not be a
    \c napi_value or a \c napi_ref. To bring JS data back, convert it with
    the QtHarmony functions or keep it in a QHarmonyJsObjectRef.
*/

/*!
    Queues \a task for execution on the Qt thread, the thread of the
    QCoreApplication object, and returns immediately. You can call this
    function from any thread at any time. Use it to bring results back from
    the JS thread. Tasks queued with runOnQtThread() run one at a time, in
    the order in which they were queued. A task queued after the Qt event
    loop has finished for good never runs.
*/
void QNativeInterface::QHarmonyApplication::runOnQtThread(std::function<void()> task)
{
    QtOhos::invokeInQtThread(std::move(task));
}

QT_END_NAMESPACE
