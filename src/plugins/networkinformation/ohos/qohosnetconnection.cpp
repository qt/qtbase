// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <optional>
#include <qohosnetconnection_p.h>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhosNetConnection {

namespace {

NetState parseNetCapabilities(QOhosJsState &jsState, const QNapi::Object &netCapabilities)
{
    NetState state = {};
    state.reachability = NetworkReachability::Local;

    const auto bearerTypes = QNapi::getFilteredArrayElements<std::vector<NetBearType>, QNapi::Number>(
        netCapabilities.get<QNapi::Array>("bearerTypes"),
        [&](QNapi::Number bearerType) {
            return jsState.tryMapOhosEnumFromJs<NetBearType>(bearerType);
        });
    if (!bearerTypes.empty())
        state.transport = bearerTypes.front();

    auto networkCapArray = QNapi::getOptionalPropOrEmpty<QNapi::Array>(netCapabilities, "networkCap");
    if (!networkCapArray.IsEmpty()) {
        const auto networkCap = QNapi::getFilteredArrayElements<std::vector<NetCap>, QNapi::Number>(
            networkCapArray,
            [&](QNapi::Number capability) {
                return jsState.tryMapOhosEnumFromJs<NetCap>(capability);
            });
        bool internetCapable = false;
        bool validated = false;
        bool checkingConnectivity = false;
        for (const auto capability : networkCap) {
            if (capability == NetCap::NET_CAPABILITY_INTERNET)
                internetCapable = true;
            else if (capability == NetCap::NET_CAPABILITY_VALIDATED)
                validated = true;
            else if (capability == NetCap::NET_CAPABILITY_CHECKING_CONNECTIVITY)
                checkingConnectivity = true;
            else if (capability == NetCap::NET_CAPABILITY_PORTAL)
                state.behindCaptivePortal = true;
        }
        if (checkingConnectivity)
            state.reachability = NetworkReachability::Local;
        else if (validated)
            state.reachability = NetworkReachability::Online;
        else if (internetCapable)
            state.reachability = NetworkReachability::Site;
    }

    return state;
}

NetState readDefaultNetState(QOhosJsState &jsState)
{
    try {
        auto netHandle = jsState.eval<QNapi::Object>("@ohos.net.connection.getDefaultNetSync()");
        constexpr qint32 invalidNetId = 0;
        if (netHandle.get<QNapi::Number>("netId").Int32Value() == invalidNetId)
            return NetState{};
        auto netCapabilities = jsState.eval<QNapi::Object>(
            "@ohos.net.connection.getNetCapabilitiesSync(*)", {netHandle});
        return parseNetCapabilities(jsState, netCapabilities);
    } catch (const Napi::Error &error) {
        qOhosPrintfError(
            "%s: failed to read default network state: %s", Q_FUNC_INFO, error.what());
        return NetState{};
    }
}

std::shared_ptr<void> registerNetConnectionEventHandlers(
    QNapi::Object netConnection,
    std::vector<std::pair<std::string, QNapi::CallbackFuncWrapper>> eventHandlers)
{
    for (auto &[eventTypeName, eventHandler] : eventHandlers)
        netConnection.eval("on(*)", {eventTypeName, std::move(eventHandler)});

    netConnection.eval(
        "register(*)",
        {
            [](const QOhosCallbackInfo &cbInfo) {
                if (cbInfo.Length() != 0 && cbInfo.getFirstArg<QNapi::Value>(Q_FUNC_INFO).IsObject())
                    QtOhos::logJsCallbackError(cbInfo, "@ohos.net.connection.NetConnection.register()");
            }
        });

    auto netConnectionRef = QtOhos::moveToSharedPtr(
        QNapi::Reference<>::makePersistentFrom(netConnection));

    return QtOhos::makeDestroyNotifier(
        [netConnectionRef]() {
            try {
                netConnectionRef->eval(
                    "unregister(*)",
                    {
                        [](const QOhosCallbackInfo &cbInfo) {
                            if (cbInfo.Length() != 0 && cbInfo.getFirstArg<QNapi::Value>(Q_FUNC_INFO).IsObject())
                                QtOhos::logJsCallbackError(cbInfo, "@ohos.net.connection.NetConnection.unregister()");
                        }
                    });
            } catch (const Napi::Error &error) {
                qOhosPrintfError(
                    "%s: net connection unregister failed (ignoring): %s",
                    Q_FUNC_INFO, error.what());
            }
        });
}

std::shared_ptr<void> registerNetConnectionListener(
    QOhosJsState &jsState, QOhosConsumer<NetState> updateListener)
{
    auto sharedUpdateListener = QtOhos::moveToSharedPtr(std::move(updateListener));

    return registerNetConnectionEventHandlers(
        jsState.eval<QNapi::Object>("@ohos.net.connection.createNetConnection()"),
        {
            {
                "netAvailable",
                [sharedUpdateListener](const QOhosCallbackInfo &cbInfo) {
                    (*sharedUpdateListener)(readDefaultNetState(cbInfo.jsState()));
                }
            },
            {
                "netCapabilitiesChange",
                [sharedUpdateListener](const QOhosCallbackInfo &cbInfo) {
                    auto netCapabilityInfo = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                    (*sharedUpdateListener)(
                        parseNetCapabilities(cbInfo.jsState(), netCapabilityInfo.get<QNapi::Object>("netCap")));
                }
            },
            {
                "netLost",
                [sharedUpdateListener]() {
                    (*sharedUpdateListener)(NetState{});
                }
            },
            {
                "netUnavailable",
                [sharedUpdateListener]() {
                    (*sharedUpdateListener)(NetState{});
                }
            },
        });
}

}

QOhosSupplier<NetState> makeOhosNetStateDataSource(QOhosConsumer<NetState> stateChangeConsumer)
{
    struct Context {
        QOhosConsumer<NetState> stateChangedHandler;
        NetState currentState;
        std::shared_ptr<void> listenerHandle;
    };

    auto context = QOhosJsThreadGateway::eval(
        [&](QOhosJsState &jsState) {
            auto context = std::make_shared<Context>();
            context->stateChangedHandler = std::move(stateChangeConsumer);
            context->currentState = readDefaultNetState(jsState);

            auto onStateChanged = [weakContext = QtOhos::makeWeakPtr(context)](NetState state) {
                QtOhos::invokeInQtThread(
                    [weakContext, state]() {
                        auto context = weakContext.lock();
                        if (context) {
                            context->currentState = state;
                            context->stateChangedHandler(context->currentState);
                        }
                    });
            };

            try {
                context->listenerHandle = QtOhos::makeProxyWithJsThreadDeleter(
                    registerNetConnectionListener(jsState, std::move(onStateChanged)));
            } catch (const Napi::Error &error) {
                qOhosPrintfError(
                    "%s: failed to register @ohos.net.connection listener: %s", Q_FUNC_INFO, error.what());
            }

            return context;
        });

    return [context]() {
        return context->currentState;
    };
}

}

QT_END_NAMESPACE
