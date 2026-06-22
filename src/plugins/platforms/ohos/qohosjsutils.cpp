// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosjsutils.h"
#include <QtCore/private/qohoscommon_p.h>
#include <qohosutils.h>
#include <utility>

using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

namespace QtOhos {

std::shared_ptr<void> registerOnOffMethodsBasedEventHandler(
    QNapi::Object eventSourceObject, const std::string &eventTypeName,
    QNapi::CallbackFuncWrapper eventHandler, OnOffMethodsBasedEventHandlerOptions options)
{
    struct Context
    {
        std::function<QNapi::Value(const CallbackInfo &)> eventHandler;
        std::function<bool(QNapi::Object)> eventSourceAliveCheckFunc;
        QNapi::Reference<QNapi::Value> optExtraOnArg;
        QNapi::Reference<QNapi::Value> optExtraOffArg;
    };

    auto env = eventSourceObject.Env();

    auto sharedContext = moveToSharedPtr(
        Context{
            .eventHandler = std::move(eventHandler.callbackFunc()),
            .eventSourceAliveCheckFunc = options.optEventSourceAliveCheckFunc
                ? std::move(options.optEventSourceAliveCheckFunc)
                : [](QNapi::Object) {
                    return true;
                },
            .optExtraOnArg = options.extraOnArg.has_value()
                ? QNapi::Reference<>::makePersistentFrom(
                    options.extraOnArg.value().mapToValue(env))
                : QNapi::Reference<>::makeEmpty(),
            .optExtraOffArg = options.extraOffArg.has_value()
                ? QNapi::Reference<>::makePersistentFrom(
                    options.extraOffArg.value().mapToValue(env))
                : QNapi::Reference<>::makeEmpty(),
        });

    auto jsEventHandlerRef = moveToSharedPtr(
        QNapi::Reference<>::makePersistentFrom(
            QNapi::Function::New(
                eventSourceObject.Env(),
                [eventTypeName, weakContext = makeWeakPtr(sharedContext)](const CallbackInfo &cbInfo) {
                    auto sharedContext = weakContext.lock();
                    if (sharedContext) {
                        return sharedContext->eventHandler(cbInfo);
                    } else {
                        qOhosPrintfWarning(
                            "%s: got unexpected '%s' event callback call for detached handler",
                            Q_FUNC_INFO, eventTypeName.c_str());
                        return cbInfo.Env().Undefined();
                    }
                })));

    std::vector<QNapi::ValueWrapper> onCallArgs;
    onCallArgs.push_back(eventTypeName);
    if (!sharedContext->optExtraOnArg.IsEmpty())
        onCallArgs.push_back(sharedContext->optExtraOnArg.Value());
    onCallArgs.push_back(jsEventHandlerRef->Value());
    bool onCallSuccessful;
    try {
        eventSourceObject.call("on", onCallArgs);
        onCallSuccessful = true;
    } catch (const Napi::Error &error) {
        onCallSuccessful = false;
        if (options.optOnCallExceptionHandler) {
            options.optOnCallExceptionHandler(error);
        } else {
            throw;
        }
    }

    if (!onCallSuccessful)
        return nullptr;

    auto eventSourceWeakRef = moveToSharedPtr(Napi::Weak(eventSourceObject));

    return makeProxyWithJsThreadDeleter(
        QtOhos::makeDestroyNotifier(
        [eventSourceWeakRef, eventTypeName, sharedContext, jsEventHandlerRef]() {
            auto eventSourceValue = eventSourceWeakRef->Value();
            if (eventSourceValue.IsObject()) {
                auto eventSourceObject = QNapi::checkedCast<QNapi::Object>(eventSourceValue);
                if (sharedContext->eventSourceAliveCheckFunc(eventSourceObject)) {
                    try {
                        std::vector<QNapi::ValueWrapper> offCallArgs;
                        offCallArgs.push_back(eventTypeName);
                        if (!sharedContext->optExtraOffArg.IsEmpty())
                            offCallArgs.push_back(sharedContext->optExtraOffArg.Value());
                        offCallArgs.push_back(jsEventHandlerRef->Value());
                        eventSourceObject.call("off", offCallArgs);
                    } catch (const Napi::Error &e) {
                        qOhosPrintfError(
                            "%s: got exception from off(%s, ...) call (ignoring): %s",
                            Q_FUNC_INFO, eventTypeName.c_str(), e.what());
                    }
                } else {
                    qOhosPrintfDebug(
                        "%s: not calling off(%s, ...), event source 'considered' not alive",
                        Q_FUNC_INFO, eventTypeName.c_str());
                }
            } else {
                qOhosPrintfDebug(
                    "%s: not calling off(%s, ...), event source not alive",
                    Q_FUNC_INFO, eventTypeName.c_str());
            }
        }));
}

std::shared_ptr<void> startDelayedJsThreadTask(
    JsState &jsState, std::function<void(JsState &)> task,
    std::chrono::milliseconds delay)
{
    struct Context
    {
        std::function<void(JsState &)> task;
        QOhosOptional<int> timerId;
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

QOhosOptional<std::uint32_t> tryGetCodeFromJsBusinessError(const Napi::Error &error)
{
    if (!error.Value().IsObject())
        return {};

    auto errorObject = QNapi::checkedCast<QNapi::Object>(error.Value());
    auto optErrorCode = QNapi::getOptionalPropOrEmpty<QNapi::Number>(errorObject, "code");

    return !optErrorCode.IsEmpty()
        ? makeQOhosOptional(optErrorCode.Uint32Value())
        : makeEmptyQOhosOptional();
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
