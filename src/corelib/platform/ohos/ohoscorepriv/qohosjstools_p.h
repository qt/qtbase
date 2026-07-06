// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSJSTOOLS_P_H
#define QOHOSJSTOOLS_P_H

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

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <functional>
#include <memory>
#include <optional>
#include <string>

QT_BEGIN_NAMESPACE

struct QOhosOnOffMethodsBasedEventHandlerOptions
{
    std::function<bool(QNapi::Object)> optEventSourceAliveCheckFunc;
    std::optional<QNapi::ValueWrapper> extraOnArg;
    std::optional<QNapi::ValueWrapper> extraOffArg;
    std::function<void(const Napi::Error &)> optOnCallExceptionHandler;
};

Q_CORE_EXPORT std::shared_ptr<void> registerQOhosOnOffMethodsBasedEventHandler(
    QNapi::Object eventSourceObject, const std::string &eventTypeName,
    QNapi::CallbackFuncWrapper handler, QOhosOnOffMethodsBasedEventHandlerOptions options = {});

// Creates a Data Source: an object tracking a value owned by a JS object and mirrored
// onto a caller-chosen target thread. The returned supplier keeps it alive and yields
// the current value; valueChangedHandler is called on each change. Value updates and the
// handler are dispatched onto the target thread via targetThreadExecutor. The two
// std::function parameters provide the JS-thread-side implementation building blocks.
template<typename T>
QOhosSupplier<T> makeQOhosDataSource(
    std::function<T(QOhosJsState &)> initialValueReader,
    std::function<std::shared_ptr<void>(QOhosJsState &, QOhosConsumer<T>)> changeListenerFactory,
    QOhosConsumer<T> valueChangedHandler,
    QOhosConsumer<std::function<void()>> targetThreadExecutor,
    std::string callerContextName = {});

template<typename T>
QOhosSupplier<T> makeQOhosDataSource(
    std::function<T(QOhosJsState &)> initialValueReader,
    std::function<std::shared_ptr<void>(QOhosJsState &, QOhosConsumer<T>)> changeListenerFactory,
    QOhosConsumer<T> valueChangedHandler,
    QOhosConsumer<std::function<void()>> targetThreadExecutor,
    std::string callerContextName)
{
    Q_UNUSED(callerContextName);

    struct Context {
        QOhosConsumer<T> valueChangedHandler;
        T currentValue;
        std::shared_ptr<void> changeListenerHandle;
    };

    auto context = QOhosJsThreadGateway::eval(
        [&](QOhosJsState &jsState) {
            auto context = std::make_shared<Context>();
            context->valueChangedHandler = std::move(valueChangedHandler);
            context->currentValue = initialValueReader(jsState);
            context->changeListenerHandle = QtOhos::makeProxyWithJsThreadDeleter(
                changeListenerFactory(
                    jsState,
                    [weakContext = QtOhos::makeWeakPtr(context), targetThreadExecutor = std::move(targetThreadExecutor)](T newValue) {
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
        });

    return [context]() {
        return context->currentValue;
    };
}

QT_END_NAMESPACE

#endif // QOHOSJSTOOLS_P_H
