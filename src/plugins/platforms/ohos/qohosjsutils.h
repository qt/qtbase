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

// Creates a Data Source: a Qt-thread object tracking a value owned by a JS object.
// The returned supplier keeps it alive and yields the current value, valueChangedHandler
// is called on each change (everything in the Qt thread).
// The two std::function parameters provide JS-thread side implementation of the necessary
// building blocks.
template<typename T>
QOhosSupplier<T> makeDataSource(
    std::function<T(JsState &)> initialValueReader,
    std::function<std::shared_ptr<void>(JsState &, QOhosConsumer<T>)> changeListenerFactory,
    QOhosConsumer<T> valueChangedHandler,
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
                    [weakContext = makeWeakPtr(context)](T newValue) {
                        invokeInQtThread(
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

}

QT_END_NAMESPACE

#endif // QOHOSJSUTILS_H
