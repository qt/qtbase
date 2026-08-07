// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosjstools_p.h"
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <iterator>
#include <vector>

QT_BEGIN_NAMESPACE

std::shared_ptr<void> registerQOhosOnOffMethodsBasedEventHandler(
    QNapi::Object eventSourceObject, const std::string &eventTypeName,
    QNapi::CallbackFuncWrapper eventHandler, QOhosOnOffMethodsBasedEventHandlerOptions options)
{
    struct Context
    {
        std::function<QNapi::Value(const QNapi::CallbackInfo &)> eventHandler;
        std::function<bool(QNapi::Object)> eventSourceAliveCheckFunc;
        QNapi::Reference<QNapi::Value> optExtraOnArg;
        QNapi::Reference<QNapi::Value> optExtraOffArg;
    };

    auto env = eventSourceObject.Env();

    auto sharedContext = QtOhos::moveToSharedPtr(
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

    auto jsEventHandlerRef = QtOhos::moveToSharedPtr(
        QNapi::Reference<>::makePersistentFrom(
            QNapi::Function::New(
                eventSourceObject.Env(),
                [eventTypeName, weakContext = QtOhos::makeWeakPtr(sharedContext)](const QNapi::CallbackInfo &cbInfo) {
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
        eventSourceObject.eval("on(*)", onCallArgs);
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

    auto eventSourceWeakRef = QtOhos::moveToSharedPtr(Napi::Weak(eventSourceObject));

    return QtOhos::makeDestroyNotifier(
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
                        eventSourceObject.eval("off(*)", offCallArgs);
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
        });
}

std::shared_ptr<void> registerOhosAppContextEnvironmentCallback(
    QOhosJsState &jsState,
    std::vector<std::pair<std::string, QNapi::CallbackFuncWrapper>> environmentCallbackMethods)
{
    auto optQAbility = jsState.defaultQAbility();
    if (!optQAbility.has_value())
        return {};

    auto environmentCallback = QNapi::makeObject(
        jsState.env(),
        std::vector<std::pair<std::string, QNapi::ValueWrapper>>(
            std::make_move_iterator(environmentCallbackMethods.begin()),
            std::make_move_iterator(environmentCallbackMethods.end())));

    auto appContextRefPtr = QtOhos::moveToSharedPtr(
        QNapi::Reference<>::makePersistentFrom(
            optQAbility->eval<QNapi::Object>("context.getApplicationContext()")));

    double environmentCallbackId = appContextRefPtr->eval<QNapi::Number>(
        "on(*)",
        {"environment", environmentCallback});

    return std::shared_ptr<void>(
        nullptr,
        [environmentCallbackId, appContextRefPtr](auto) {
            QOhosJsThreadGateway::runAndWait(
                [&](QOhosJsState &) {
                    auto appContextRef = std::move(*appContextRefPtr);
                    appContextRef.eval(
                        "off(*)",
                        {"environment", environmentCallbackId});
                },
                Q_FUNC_INFO);
        });
}

QT_END_NAMESPACE
