// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosjsutils.h"
#include <QtCore/private/qohoscommon_p.h>
#include <qohosutils.h>
#include <utility>

using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

namespace QtOhos {

std::shared_ptr<void> startDelayedJsThreadTask(
    JsState &jsState, std::function<void(JsState &)> task,
    std::chrono::milliseconds delay)
{
    struct Context
    {
        std::function<void(JsState &)> task;
        std::optional<int> timerId;
    };

    auto context = std::make_shared<Context>();
    context->task = std::move(task);

    int timerId = jsState.eval<QNapi::Number>(
        "Global.setTimeout(*)",
        {
            [context](const CallbackInfo &cbInfo) {
                if (context->task) {
                    auto task = std::exchange(context->task, nullptr);
                    context->timerId.reset();
                    task(cbInfo.jsState());
                }
            },
            std::max(delay, std::chrono::milliseconds(0)).count(),
        });
    context->timerId = timerId;

    return QtOhos::makeDestroyNotifier(
        [context]() {
            if (context->timerId.has_value()) {
                runInJsThreadAndWait(
                    [&](JsState &jsState) {
                        jsState.eval("Global.clearTimeout(*)", {context->timerId.value()});
                    },
                    Q_FUNC_INFO);
                context->task = nullptr;
                context->timerId.reset();
            }
        });
}

int setJsTimeout(
    JsState &jsState, std::function<void(const CallbackInfo &)> timeoutFunc,
    std::chrono::milliseconds delay)
{
    int timerId = jsState.eval<QNapi::Number>(
        "Global.setTimeout(*)", {std::move(timeoutFunc), delay.count()});
    return timerId;
}

void clearJsTimeout(JsState &jsState, int timerId)
{
    jsState.eval("Global.clearTimeout(*)", {timerId});
}

QNapi::Promise makeResolvedPromise(QNapi::Value valueForResolve)
{
    auto promiseDeferred = QNapi::Promise::Deferred::New(valueForResolve.Env());
    promiseDeferred.Resolve(valueForResolve);
    return promiseDeferred.Promise();
}

std::optional<std::uint32_t> tryGetCodeFromJsBusinessError(const Napi::Error &error)
{
    if (!error.Value().IsObject())
        return {};

    auto errorObject = QNapi::checkedCast<QNapi::Object>(error.Value());
    auto optErrorCode = QNapi::getOptionalPropOrEmpty<QNapi::Number>(errorObject, "code");

    return !optErrorCode.IsEmpty()
        ? std::optional(optErrorCode.Uint32Value())
        : std::nullopt;
}

void rethrowUnlessJsBusinessErrorIs(
    const Napi::Error &error, std::uint32_t suppressedErrorCode, const char *callerContextName)
{
    if (tryGetCodeFromJsBusinessError(error) != suppressedErrorCode)
        throw;

    qOhosPrintfWarning(
        "%s: ignored expected JS business error %u",
        callerContextName, suppressedErrorCode);
}

bool runIgnoringJsBusinessError(
    JsState &, std::uint32_t suppressedErrorCode, const char *callerContextName,
    const std::function<void()> &action)
{
    try {
        action();
        return true;
    } catch (const Napi::Error &error) {
        rethrowUnlessJsBusinessErrorIs(error, suppressedErrorCode, callerContextName);
        return false;
    }
}

}

QT_END_NAMESPACE
