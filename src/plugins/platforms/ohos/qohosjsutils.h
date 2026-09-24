// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSJSUTILS_H
#define QOHOSJSUTILS_H

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohosjstools_p.h>
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

template<typename T>
QNapi::Promise adaptAsyncCallResultToJsPromise(
    JsState &jsState, std::function<QNapi::Value(JsState &, T)> promiseValueFactory,
    const QOhosConsumer<JsState &, QOhosConsumer<JsState &, T>> &asyncCallFunc);

Q_REQUIRED_RESULT std::shared_ptr<void> startDelayedJsThreadTask(
    JsState &jsState, std::function<void(JsState &)> task,
    std::chrono::milliseconds delay);

int setJsTimeout(
    JsState &jsState, std::function<void(const CallbackInfo &)> timeoutFunc,
    std::chrono::milliseconds delay);

void clearJsTimeout(JsState &jsState, int timerId);

QNapi::Promise makeResolvedPromise(QNapi::Value valueForResolve);

// Makes a Promise resolved to undefined, the JS way of saying "no particular value".
QNapi::Promise makeResolvedPromise(Napi::Env env);

template<typename T>
std::optional<T> getOptionalProperty(const QNapi::Object &object, const std::string &propName);

std::optional<std::uint32_t> tryGetCodeFromJsBusinessError(const Napi::Error &error);

// On JS business error matching suppressedErrorCode warns and returns, otherwise rethrows.
// Call from a catch block.
void rethrowUnlessJsBusinessErrorIs(
    const Napi::Error &error, std::uint32_t suppressedErrorCode, const char *callerContextName);

// Runs action; on JS business error matching suppressedErrorCode warns and returns, otherwise rethrows.
// Returns true if action completed, false if error was swallowed.
bool runIgnoringJsBusinessError(
    JsState &, std::uint32_t suppressedErrorCode, const char *callerContextName,
    const std::function<void()> &action);

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
std::optional<T> getOptionalProperty(const QNapi::Object &object, const std::string &propName)
{
    auto optPropValue = QNapi::getOptionalPropOrEmpty<T>(object, propName);
    return !optPropValue.IsEmpty()
        ? std::optional(optPropValue)
        : std::nullopt;
}

template<typename ConfigValue>
QOhosSupplier<ConfigValue> makeOhosConfigValueDataSource(
    std::function<ConfigValue(QOhosJsState &)> initValueSupplier,
    std::function<ConfigValue(QOhosJsState &, const QNapi::Object &)> valueFetcher,
    QOhosConsumer<ConfigValue> valueChangedHandler)
{
    return makeQOhosDataSource<ConfigValue>(
        std::move(initValueSupplier),
        [valueFetcher = std::move(valueFetcher)](QOhosJsState &jsState, QOhosConsumer<ConfigValue> valueUpdatesConsumer) mutable {
            return registerOhosAppContextEnvironmentCallback(
                jsState,
                {
                    {
                        "onConfigurationUpdated",
                        [valueFetcher = std::move(valueFetcher), valueUpdatesConsumer = std::move(valueUpdatesConsumer)](const QOhosCallbackInfo &cbInfo) {
                            auto config = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                            valueUpdatesConsumer(valueFetcher(cbInfo.jsState(), config));
                        }
                    },
                });
        },
        std::move(valueChangedHandler),
        QtOhos::invokeInQtThread,
        Q_FUNC_INFO);
}

}

QT_END_NAMESPACE

#endif // QOHOSJSUTILS_H
