// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qohoswindowproxydatafactory.h>

#include <QtCore/qscopeguard.h>
#include <cstdint>
#include <qarkui/qxcomponentregistry.h>
#include <qohosutils.h>
#include <render/qohoswindowproxy.h>
#include <render/qxcomponent.h>

QT_BEGIN_NAMESPACE

namespace {

std::string makeOhosUniqueSystemWindowName(QtOhos::InternalWindowId internalWindowId)
{
    static std::uint64_t uniqueIdSuffixCounter = 0;
    auto result = QStringLiteral("QtWindow_%1_%2")
        .arg(internalWindowId.toString())
        .arg(QString::number(uniqueIdSuffixCounter));
    ++uniqueIdSuffixCounter;
    return result.toStdString();
}

struct OnWindowCreatedLoadWindowContentsContext
{
    bool disableWindowFocusableBeforeLoadContentHack;
    std::string contentPagePath;
    QNapi::Object localStorage;
};

struct SubWindowOptions
{
    std::string windowTitle;
    bool decorEnabled;
    bool isModal;
};

struct SubWindowOnAppearContext
{
    QXComponentId xComponentId;
    QNapi::Reference<QNapi::Object> localStorageObj;
    QNapi::Reference<QNapi::Object> windowObject;
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer;
    std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer;
    WindowProxyType windowProxyType;
};

struct LocalStorageForWindowCreateInfo
{
    QXComponentId xComponentId;
    QNapi::Object windowObject;
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer;
    std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer;
    WindowProxyType windowProxyType;
};

std::shared_ptr<QtOhos::QAbilityPeer> getQAbilityPeerByInstanceIdOrFail(
    QtOhos::JsState &jsState, const std::string &qAbilityInstanceId)
{
    auto qAbilityPeer = jsState.tryGetQAbilityPeerByInstanceId(qAbilityInstanceId);
    if (!qAbilityPeer) {
        qOhosReportFatalErrorAndAbort(
            "Failed to find QAbilityPeer for qAbilityInstanceId: %s", qAbilityInstanceId.c_str());
    }
    return qAbilityPeer;
}

QXComponentNode takeNodeXComponentFromRegistryOrFail(const QXComponentId &xComponentId)
{
    auto &registry = QArkUi::QXComponentRegistry::instance();
    auto nativeNodeXComponentOpt = registry.tryTakeNodeByXComponentId(xComponentId);
    if (!nativeNodeXComponentOpt.hasValue()) {
        qOhosReportFatalErrorAndAbort(
            "Failed to fetch native node xcomponent with id: %s",
            xComponentId.stringId().c_str());
    }
    return nativeNodeXComponentOpt.value();
}

QNapi::Object makeLocalStorage(QtOhos::JsState &jsState)
{
    return jsState.eval<QNapi::Object>("LocalStorage.makeNewLocalStorage()");
}

QNapi::Promise createSubWindowWithOptions(
    QNapi::Object windowStageOrWindowObject,
    const std::string &windowName, const SubWindowOptions &subWindowOptions)
{
    auto subWindowOptionsObject =
        QNapi::makeObject(
            windowStageOrWindowObject.Env(),
            {
                {"title", subWindowOptions.windowTitle},
                {"decorEnabled", subWindowOptions.decorEnabled},
                {"isModal", subWindowOptions.isModal},
            });

    return windowStageOrWindowObject.call<QNapi::Promise>(
        "createSubWindowWithOptions",
        {windowName, subWindowOptionsObject});
}

std::function<void(const QtOhos::CallbackInfo &)>
makeSubWindowOnAppearCallbackHandler(SubWindowOnAppearContext subWindowOnAppearCbCtx)
{
    auto sharedContext = QtOhos::moveToSharedPtr(std::move(subWindowOnAppearCbCtx));
    return [sharedContext](const QtOhos::CallbackInfo &cbInfo) mutable {
        if (!sharedContext)
            return;

        auto xComponent = takeNodeXComponentFromRegistryOrFail(sharedContext->xComponentId);

        sharedContext->resultConsumer(
            cbInfo.jsState(),
            QOhosWindowProxyData {
                .qAbilityPeer = sharedContext->qAbilityPeer,
                .jsWindow = std::move(sharedContext->windowObject),
                .windowProxyType = sharedContext->windowProxyType,
                .nodeXComponent = std::make_shared<QXComponentNode>(xComponent),
                .jsKeepAliveData = QtOhos::moveToSharedPtr(std::move(sharedContext->localStorageObj)),
            });

        sharedContext.reset();
    };
}

QNapi::Promise loadWindowContents(QNapi::Object window, QNapi::Object localStorage, const std::string &contentPagePath)
{
    return window.call<QNapi::Promise>("loadContent", {contentPagePath, localStorage});
}

QNapi::Object makeLocalStorageForWindow(
    QtOhos::JsState &jsState, LocalStorageForWindowCreateInfo &&createInfo)
{
    auto *env = jsState.env();

    auto localStorage = makeLocalStorage(jsState);
    auto subWindowNativeNodeCreateInfo = QNapi::makeObject(
        env,
        {
            {"xComponentId", createInfo.xComponentId.toNapiValue(env)},
            {
                "onDisAppear",
                [xComponentId = createInfo.xComponentId.stringId()]() {
                    qOhosPrintfDebug(
                        "WindowNativeNodeCreateInfo.onDisAppear() called from JS for xComponentId='%s'",
                        xComponentId.c_str());
                }
            },
            {
                "onAppear",
                makeSubWindowOnAppearCallbackHandler(SubWindowOnAppearContext {
                    .xComponentId = createInfo.xComponentId,
                    .localStorageObj = Napi::Persistent(localStorage),
                    .windowObject = Napi::Persistent(createInfo.windowObject),
                    .resultConsumer = std::move(createInfo.resultConsumer),
                    .qAbilityPeer = std::move(createInfo.qAbilityPeer),
                    .windowProxyType = createInfo.windowProxyType,
                }),
            }
        });

    localStorage.call("setOrCreate", {"createInfo", subWindowNativeNodeCreateInfo});
    return localStorage;
}

QNapi::Promise onWindowCreatedLoadWindowContents(
    QtOhos::JsState &, const QNapi::Object &windowObject,
    OnWindowCreatedLoadWindowContentsContext context)
{
    struct LoadWindowContentsArgs
    {
        QNapi::Reference<QNapi::Object> window;
        QNapi::Reference<QNapi::Object> localStorage;
        std::string contentPagePath;
    };

    return !context.disableWindowFocusableBeforeLoadContentHack
        ? loadWindowContents(windowObject, context.localStorage, context.contentPagePath)
        : windowObject.call<QNapi::Promise>("setWindowFocusable", {false})
            .withContext(LoadWindowContentsArgs {
                .window = Napi::Persistent(windowObject),
                .localStorage = Napi::Persistent(context.localStorage),
                .contentPagePath = context.contentPagePath,
            })
            .onThenWithContext([](LoadWindowContentsArgs &windowLocalStoragePair) {
                return loadWindowContents(
                    windowLocalStoragePair.window.Value(),
                    windowLocalStoragePair.localStorage.Value(),
                    windowLocalStoragePair.contentPagePath);
            });
}

}

void makeWindowProxyDataForMainWindowInJsThread(
    QtOhos::JsState &jsState,
    const QOhosWindowProxyMainWindowCreateInfo &createInfo,
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer)
{
    auto abilityStartupOptions =
        QNapi::makeObject(
            jsState.env(),
            {
                {"windowLeft", createInfo.frameGeometry.x()},
                {"windowTop", createInfo.frameGeometry.y()},
                {"windowWidth", createInfo.frameGeometry.width()},
                {"windowHeight", createInfo.frameGeometry.height()},
            });

    if (createInfo.fullscreen) {
        auto fullscreenWindowMode = jsState.eval<QNapi::Number>(
            "@ohos.app.ability.AbilityConstant.WindowMode.WINDOW_MODE_FULLSCREEN");
        abilityStartupOptions.set("windowMode", fullscreenWindowMode);
    }

    if (createInfo.displayId.hasValue())
        abilityStartupOptions.set("displayId", createInfo.displayId.value().value());

    jsState.startNewQAbilityInstance(
        jsState.defaultQAbilityPeer(), createInfo.qWindowRef,
        abilityStartupOptions,
        [qWindowRef = createInfo.qWindowRef,
            windowId = createInfo.windowId,
            resultConsumer = std::move(resultConsumer)](QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer) {
                auto createInfo = QOhosWindowProxyExistingMainWindowCreateInfo {
                    .qWindowRef = qWindowRef,
                    .qAbilityInstanceId = qAbilityPeer->instanceId(),
                    .windowId = windowId,
                };

                makeWindowProxyDataForExistingMainWindowInJsThread(
                    jsState, createInfo, std::move(resultConsumer));
        });
}

void makeWindowProxyDataForExistingMainWindowInJsThread(
    QtOhos::JsState &jsState,
    const QOhosWindowProxyExistingMainWindowCreateInfo &createInfo,
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer)
{
    auto optQUiAbilityPeer = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(
        getQAbilityPeerByInstanceIdOrFail(jsState, createInfo.qAbilityInstanceId));
    if (!optQUiAbilityPeer) {
        qOhosReportFatalErrorAndAbort(
            "%s Attempting to make window proxy for main window for ability without windowStage. This is most likely a programming error. Aborting...",
            Q_FUNC_INFO);
    }
    optQUiAbilityPeer->setQWindow(jsState.env(), createInfo.qWindowRef);

    auto window = optQUiAbilityPeer->windowStage().call<QNapi::Object>("getMainWindowSync");
    auto nativeNodeXComponentId = QXComponentId::createForNativeNodeMainWindow(optQUiAbilityPeer->instanceId());
    auto nodeXComponent = takeNodeXComponentFromRegistryOrFail(nativeNodeXComponentId);

    resultConsumer(
        jsState,
        QOhosWindowProxyData {
            .qAbilityPeer = optQUiAbilityPeer,
            .jsWindow = Napi::Persistent(window),
            .windowProxyType = WindowProxyType::MainWindow,
            .nodeXComponent = std::make_shared<QXComponentNode>(nodeXComponent),
            .jsKeepAliveData = nullptr,
        });
}

void makeWindowProxyDataForSubWindowInJsThread(
    QtOhos::JsState &jsState,
    const QOhosWindowProxySubWindowCreateInfo &createInfo,
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer)
{
    auto qAbilityPeer = getQAbilityPeerByInstanceIdOrFail(jsState, createInfo.qAbilityInstanceId);
    auto optQUiAbilityPeer = QtOhos::QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(qAbilityPeer);
    if (!optQUiAbilityPeer) {
        qOhosReportFatalErrorAndAbort(
            "%s Attempting to make window proxy for sub window for ability without windowStage. This is most likely a programming error. Aborting...",
            Q_FUNC_INFO);
    }
    makeWindowProxyDataForSubWindowInJsThread(
        jsState, optQUiAbilityPeer->windowStage(), createInfo, std::move(resultConsumer));
}

void makeWindowProxyDataForSubWindowInJsThread(
    QtOhos::JsState &jsState,
    QNapi::Object windowStageOrWindowObject,
    const QOhosWindowProxySubWindowCreateInfo &createInfo,
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer)
{
    auto xComponentId = QXComponentId::createForNativeNodeSubWindow(createInfo.windowId);
    struct Context
    {
        bool disableWindowFocusableBeforeLoadContentHack;
        QXComponentId xComponentId;
        std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer;
    };

    auto qAbilityPeer = getQAbilityPeerByInstanceIdOrFail(jsState, createInfo.qAbilityInstanceId);

    createSubWindowWithOptions(
        windowStageOrWindowObject,
        makeOhosUniqueSystemWindowName(createInfo.windowId),
        SubWindowOptions {
            .windowTitle = createInfo.windowTitle,
            .decorEnabled = createInfo.decorEnabled,
            .isModal = createInfo.modal,
        })
        .withContext(Context {
            .disableWindowFocusableBeforeLoadContentHack = createInfo.disableWindowFocusableBeforeLoadContentHack,
            .xComponentId = xComponentId,
            .qAbilityPeer = qAbilityPeer,
        })
        .onThenWithContext([resultConsumer = std::move(resultConsumer)](const QtOhos::CallbackInfo &cbInfo, Context &context) mutable {
            auto windowObject = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

            auto localStorage = makeLocalStorageForWindow(
                cbInfo.jsState(),
                LocalStorageForWindowCreateInfo {
                    .xComponentId = context.xComponentId,
                    .windowObject = windowObject,
                    .resultConsumer = std::move(resultConsumer),
                    .qAbilityPeer = context.qAbilityPeer,
                    .windowProxyType = WindowProxyType::SubWindow,
                });

            return onWindowCreatedLoadWindowContents(
                cbInfo.jsState(), windowObject,
                OnWindowCreatedLoadWindowContentsContext {
                    .disableWindowFocusableBeforeLoadContentHack = context.disableWindowFocusableBeforeLoadContentHack,
                    .contentPagePath = "pages/SubWindowNativeNode",
                    .localStorage = localStorage,
                });
        })
        .onCatch([windowId = createInfo.windowId](const QtOhos::CallbackInfo &cbInfo) {
            QtOhos::logJsCallbackError(cbInfo, "createSubWindowWithOptions() failed");
            qOhosReportFatalErrorAndAbort(
                "Failed to create subwindow for windowId='%s'",
                windowId.toStdString().c_str());
        });
}

void makeWindowProxyDataForFloatWindowInJsThread(
    QtOhos::JsState &jsState, const QOhosWindowProxyFloatWindowCreateInfo &createInfo,
    QOhosConsumer<QtOhos::JsState &, QOhosWindowProxyData> resultConsumer)
{
    auto xComponentId = QXComponentId::createForNativeNodeFloatWindow(createInfo.internalWindowId);
    auto qAbilityPeer = jsState.defaultQAbilityPeer();

    auto configurationObject = QNapi::makeObject(
        jsState.env(),
        {
            // NOTE - The parameter name is misleading, what it refers to actually is (system) window id
            {"name", makeOhosUniqueSystemWindowName(createInfo.internalWindowId)},
            {"windowType", jsState.eval<QNapi::Number>("@ohos.window.WindowType.TYPE_FLOAT")},
            {"ctx", qAbilityPeer->qAbility().get<QNapi::Object>("context")},
        });

    if (createInfo.displayId.hasValue())
        configurationObject.set("displayId", createInfo.displayId.value().value());

    struct Context
    {
        QXComponentId xComponentId;
        std::shared_ptr<QtOhos::QAbilityPeer> qAbilityPeer;
    };

    jsState
        .eval<QNapi::Promise>("@ohos.window.createWindow(*)", {configurationObject})
        .withContext(Context {
            .xComponentId = xComponentId,
            .qAbilityPeer = qAbilityPeer,
        })
        .onThenWithContext([resultConsumer = std::move(resultConsumer)](const QtOhos::CallbackInfo &cbInfo, Context &context) mutable {
            auto windowObject = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

            auto localStorage = makeLocalStorageForWindow(
                cbInfo.jsState(),
                LocalStorageForWindowCreateInfo {
                    .xComponentId = context.xComponentId,
                    .windowObject = windowObject,
                    .resultConsumer = std::move(resultConsumer),
                    .qAbilityPeer = context.qAbilityPeer,
                    .windowProxyType = WindowProxyType::FloatWindow,
                });

            return onWindowCreatedLoadWindowContents(
                cbInfo.jsState(), windowObject,
                OnWindowCreatedLoadWindowContentsContext {
                    .disableWindowFocusableBeforeLoadContentHack = false,
                    .contentPagePath = "pages/FloatWindowNativeNode",
                    .localStorage = localStorage,
                });
        })
        .onCatch([internalWindowId = createInfo.internalWindowId](const QtOhos::CallbackInfo &cbInfo) {
            QtOhos::logJsCallbackError(cbInfo, "Failed to create TYPE_FLOAT window");
            qOhosReportFatalErrorAndAbort(
                "Failed to create TYPE_FLOAT window for windowId='%s'",
                internalWindowId.toStdString().c_str());
        });
}


QT_END_NAMESPACE
