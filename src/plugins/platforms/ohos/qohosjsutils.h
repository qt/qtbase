// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSJSUTILS_H
#define QOHOSJSUTILS_H

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <string>
#include <type_traits>

#include "qohoscloseeventcontext_p.h"

QT_BEGIN_NAMESPACE

namespace QtOhos {

struct OnOffMethodsBasedEventHandlerOptions
{
    std::function<bool(QNapi::Object)> optEventSourceAliveCheckFunc;
    std::optional<QNapi::ValueWrapper> extraOnArg;
    std::optional<QNapi::ValueWrapper> extraOffArg;
    std::function<void(const Napi::Error &)> optOnCallExceptionHandler;
};

template<typename T>
QNapi::Promise adaptAsyncCallResultToJsPromise(
    JsState &jsState, std::function<QNapi::Value(JsState &, T)> promiseValueFactory,
    const QOhosConsumer<JsState &, QOhosConsumer<JsState &, T>> &asyncCallFunc);

std::shared_ptr<void> registerOnOffMethodsBasedEventHandler(
    QNapi::Object eventSourceObject, const std::string &eventTypeName,
    QNapi::CallbackFuncWrapper handler, OnOffMethodsBasedEventHandlerOptions options = {});

Q_REQUIRED_RESULT std::shared_ptr<void> startDelayedJsThreadTask(
    JsState &jsState, std::function<void(JsState &)> task,
    std::chrono::milliseconds delay);

int setJsTimeout(
    JsState &jsState, std::function<void(const CallbackInfo &)> timeoutFunc,
    std::chrono::milliseconds delay);

void clearJsTimeout(JsState &jsState, int timerId);

QNapi::Promise makeResolvedPromise(QNapi::Value valueForResolve);

template<typename T>
QOhosOptional<T> getOptionalProperty(const QNapi::Object &object, const std::string &propName);

QOhosOptional<std::uint32_t> tryGetCodeFromJsBusinessError(const Napi::Error &error);

// On JS business error matching suppressedErrorCode warns and returns, otherwise rethrows.
// Call from a catch block.
void rethrowUnlessJsBusinessErrorIs(
    const Napi::Error &error, std::uint32_t suppressedErrorCode, const char *callerContextName);

// Runs action; on JS business error matching suppressedErrorCode warns and returns, otherwise rethrows.
// Returns true if action completed, false if error was swallowed.
bool runIgnoringJsBusinessError(
    JsState &, std::uint32_t suppressedErrorCode, const char *callerContextName,
    const std::function<void()> &action);

// Creates a Data Source: an object tracking a value owned by a JS object and mirrored
// onto a caller-chosen target thread. The returned supplier keeps it alive and yields
// the current value; valueChangedHandler is called on each change. Value updates and the
// handler are dispatched onto the target thread via targetThreadExecutor. The two
// std::function parameters provide the JS-thread-side implementation building blocks.
template<typename T>
QOhosSupplier<T> makeDataSource(
    std::function<T(JsState &)> initialValueReader,
    std::function<std::shared_ptr<void>(JsState &, QOhosConsumer<T>)> changeListenerFactory,
    QOhosConsumer<T> valueChangedHandler,
    QOhosConsumer<std::function<void()>> targetThreadExecutor,
    std::string callerContextName = {});

template<typename T>
QNapi::Promise adaptAsyncCallResultToJsPromise(
    JsState &jsState, std::function<QNapi::Value(JsState &, T)> promiseValueFactory,
    const QOhosConsumer<JsState &, QOhosConsumer<JsState &, T>> &asyncCallFunc)
{
    auto promiseDeferred = QNapi::Promise::Deferred::New(jsState.env());

    asyncCallFunc(
        jsState,
        makeCallOnceConsumerWrapper<JsState &, T>(
            [promiseDeferred, promiseValueFactory = std::move(promiseValueFactory)](JsState &jsState, T result) {
                promiseDeferred.Resolve(promiseValueFactory(jsState, result));
            }));

    return promiseDeferred.Promise();
}

template<typename T>
QOhosOptional<T> getOptionalProperty(const QNapi::Object &object, const std::string &propName)
{
    auto optPropValue = QNapi::getOptionalPropOrEmpty<T>(object, propName);
    return !optPropValue.IsEmpty()
        ? makeQOhosOptional(optPropValue)
        : makeEmptyQOhosOptional();
}

template<typename T>
QOhosSupplier<T> makeDataSource(
    std::function<T(JsState &)> initialValueReader,
    std::function<std::shared_ptr<void>(JsState &, QOhosConsumer<T>)> changeListenerFactory,
    QOhosConsumer<T> valueChangedHandler,
    QOhosConsumer<std::function<void()>> targetThreadExecutor,
    std::string callerContextName)
{
    struct Context {
        QOhosConsumer<T> valueChangedHandler;
        T currentValue;
        std::shared_ptr<void> changeListenerHandle;
    };

    auto context = evalInJsThread(
        [&](JsState &jsState) {
            auto context = std::make_shared<Context>();
            context->valueChangedHandler = std::move(valueChangedHandler);
            context->currentValue = initialValueReader(jsState);
            context->changeListenerHandle = makeProxyWithJsThreadDeleter(
                changeListenerFactory(
                    jsState,
                    [weakContext = makeWeakPtr(context), targetThreadExecutor = std::move(targetThreadExecutor)](T newValue) {
                        targetThreadExecutor(
                            [weakContext, newValue = std::move(newValue)]() mutable {
                                auto context = weakContext.lock();
                                if (context && newValue != context->currentValue) {
                                    context->currentValue = std::move(newValue);
                                    context->valueChangedHandler(context->currentValue);
                                }
                            });
                    }));
            return context;
        },
        std::move(callerContextName));

    return [context]() {
        return context->currentValue;
    };
}

namespace details_qohosjsutils_h {

std::shared_ptr<void> registerAppContextEnvironmentCallback(
    QtOhos::JsState &jsState, QNapi::Object environmentCallback);

std::shared_ptr<void> registerAppConfigurationUpdateListener(
    QtOhos::JsState &jsState, std::function<void(QtOhos::JsState &, QNapi::Object)> updateListener);

}

template<typename ConfigValue>
QOhosSupplier<ConfigValue> makeOhosConfigValueDataSource(
    std::function<ConfigValue(QtOhos::JsState &)> initValueSupplier,
    std::function<ConfigValue(QtOhos::JsState &, const QNapi::Object &)> valueFetcher,
    QOhosConsumer<ConfigValue> valueChangedHandler)
{
    using namespace details_qohosjsutils_h;

    return QtOhos::makeDataSource<ConfigValue>(
        std::move(initValueSupplier),
        [valueFetcher = std::move(valueFetcher)](QtOhos::JsState &jsState, QOhosConsumer<ConfigValue> valueUpdatesConsumer) mutable {
            return registerAppConfigurationUpdateListener(
                jsState,
                [valueFetcher = std::move(valueFetcher), valueUpdatesConsumer = std::move(valueUpdatesConsumer)](QtOhos::JsState &jsState, QNapi::Object config) {
                    valueUpdatesConsumer(valueFetcher(jsState, config));
                });
        },
        std::move(valueChangedHandler),
        QtOhos::invokeInQtThread,
        Q_FUNC_INFO);
}

}

QT_END_NAMESPACE

#endif // QOHOSJSUTILS_H
