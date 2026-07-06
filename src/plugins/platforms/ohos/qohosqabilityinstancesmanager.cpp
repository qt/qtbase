// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosqabilityinstancesmanager.h"
#include <QtCore/private/qnapi_p.h>
#include <qohosjsenv_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qcryptographichash.h>
#include <QtGui/qwindow.h>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <qohosdeviceinfo_p.h>
#include <qohosjsutils.h>
#include <qohosplatformwindow.h>
#include <render/qohosview.h>
#include <typeindex>
#include <unistd.h>

#include "qohoscloseeventcontext_p.h"

QT_BEGIN_NAMESPACE

using namespace std::chrono_literals;

namespace QtOhos {

namespace {

const auto qtInternalRequestIdWantParamKey = "io.qt.private.internalRequestId";
const auto qAbilityInstanceIdWantParamKey = "io.qt.private.abilityInstanceId";
const auto callerAbilityNameWantParamKey = "ohos.aafwk.param.callerAbilityName";
const auto callerBundleNameWantParamKey = "ohos.aafwk.param.callerBundleName";
const auto callerPidWantParamKey = "ohos.aafwk.param.callerPid";

std::string getUtf8QtInternalRequestIdForThisProcess()
{
    static const auto internalRequestIdCommonData =
        QByteArray::fromHex("eba08604c86e45ae3690621757b3f4e9");

    std::int32_t thisProcessPid = ::getpid();

    QCryptographicHash hash{QCryptographicHash::Sha1};
    hash.addData(internalRequestIdCommonData);
    hash.addData(QByteArray::number(thisProcessPid));

    return hash.result().toHex().toStdString();
}

QNapi::Symbol getAbilityLaunchParamPropSymbol(JsState &jsState)
{
    struct SymbolTag
    {
    };

    return jsState.getJsSymbolForType<SymbolTag>();
}

void sendWantToSelfQAbility(
    std::shared_ptr<QAbilityEngine> abilityEngine,
    QNapi::Object baseQAbility, QNapi::Object optStartOptions, const std::string &instanceId,
    std::function<void(JsState &)> continueFunc)
{
    auto qAbilityInfo = abilityEngine->readAbilityInfo(baseQAbility);

    std::vector<QNapi::ValueWrapper> startAbilityArgs = {
        QNapi::makeObject(
            baseQAbility.Env(),
            {
                {"bundleName", qAbilityInfo.bundleName},
                {"moduleName", qAbilityInfo.moduleName},
                {"abilityName", qAbilityInfo.name},
                {
                    "parameters",
                    QNapi::makeObject(
                        baseQAbility.Env(),
                        {
                            {qtInternalRequestIdWantParamKey, getUtf8QtInternalRequestIdForThisProcess()},
                            {qAbilityInstanceIdWantParamKey, instanceId},
                        })
                },
            }),
    };

    if (!optStartOptions.IsEmpty())
        startAbilityArgs.push_back(optStartOptions);

    baseQAbility.evalToPromiseOrRejectOnThrow("context.startAbility(*)", startAbilityArgs)
    .onCatch(
        [instanceId](const CallbackInfo &cbInfo) {
            logJsCallbackError(cbInfo, "context.startAbility()");
            qOhosReportFatalErrorAndAbort("Failed to start the Ability with instance id: %s", instanceId.c_str());
        })
    .onFinally(
        [continueFunc = std::move(continueFunc)](const CallbackInfo &cbInfo) {
            continueFunc(cbInfo.jsState());
        });
}

QOhosSupplier<std::string> makeAbilityInstanceIdsGenerator()
{
    return [idsCounter = std::uint64_t(0)]() mutable {
        auto instanceId = std::to_string(idsCounter);
        ++idsCounter;
        return instanceId;
    };
}

class QAbilityPeerImpl : public virtual QAbilityPeer
{
public:
    QAbilityPeerImpl(
        JsState &jsState, std::shared_ptr<QAbilityEngine> abilityEngine,
        std::string instanceId, QObjectThreadSafeRef qwindow,
        QNapi::Object uiAbility);

    ~QAbilityPeerImpl() = default;

    std::string instanceId() final;
    QNapi::Object qAbility() final;
    QObjectThreadSafeRef qWindowRef() final;
    QOhosOptional<QNapi::Promise> qWindowDestroyPromise() final;
    void forceResolveQWindowDestroyPromiseIfPresent(Napi::Env env) final;
    std::shared_ptr<std::atomic_bool> destroyAllowedFlag() final;

    void setQWindow(Napi::Env env, QObjectThreadSafeRef qwindow) final;

private:
    struct QWindowDestroyPromiseData
    {
        std::optional<QNapi::Promise::Deferred> deferred;
        QNapi::Reference<QNapi::Promise> promise;
    };

    std::shared_ptr<QAbilityEngine> m_abilityEngine;
    std::string m_instanceId;
    QObjectThreadSafeRef m_qwindow;
    std::shared_ptr<QWindowDestroyPromiseData> m_optQWindowDestroyPromiseData;
    std::shared_ptr<std::atomic_bool> m_destroyAllowedFlag;
    QNapi::Reference<QNapi::Object> m_uiAbility;
};

QAbilityPeerImpl::QAbilityPeerImpl(
    JsState &, std::shared_ptr<QAbilityEngine> abilityEngine,
    std::string instanceId, QObjectThreadSafeRef qwindow,
    QNapi::Object uiAbility)
    : m_abilityEngine(abilityEngine)
    , m_instanceId(std::move(instanceId))
    , m_destroyAllowedFlag(std::make_shared<std::atomic_bool>(false))
    , m_uiAbility(Napi::Persistent(uiAbility))
{
    setQWindow(uiAbility.Env(), qwindow);
}

std::string QAbilityPeerImpl::instanceId()
{
    return m_instanceId;
}

QNapi::Object QAbilityPeerImpl::qAbility()
{
    return m_uiAbility.Value();
}

QObjectThreadSafeRef QAbilityPeerImpl::qWindowRef()
{
    return m_qwindow;
}

QOhosOptional<QNapi::Promise> QAbilityPeerImpl::qWindowDestroyPromise()
{
    if (!m_optQWindowDestroyPromiseData || m_optQWindowDestroyPromiseData->promise.IsEmpty())
        return {};

    auto promiseValue = m_optQWindowDestroyPromiseData->promise.Value();
    m_optQWindowDestroyPromiseData->promise.Reset();

    return makeQOhosOptional(promiseValue);
}

void QAbilityPeerImpl::forceResolveQWindowDestroyPromiseIfPresent(Napi::Env env)
{
    if (!m_optQWindowDestroyPromiseData || !m_optQWindowDestroyPromiseData->deferred)
        return;

    qOhosPrintfInfo(
        "%s: force-resolving QWindow destroy Promise for id='%s'",
        Q_FUNC_INFO, m_instanceId.c_str());
    std::exchange(m_optQWindowDestroyPromiseData->deferred, std::nullopt)->Resolve(env.Undefined());
}

std::shared_ptr<std::atomic_bool> QAbilityPeerImpl::destroyAllowedFlag()
{
    return m_destroyAllowedFlag;
}

void QAbilityPeerImpl::setQWindow(Napi::Env env, QObjectThreadSafeRef qwindow)
{
    if (qwindow == m_qwindow)
        return;

    qOhosPrintfInfo(
        "%s: setting QAbilityPeer's QWindow: %s / %s",
        Q_FUNC_INFO, m_instanceId.c_str(), m_qwindow.refName().c_str());

    if (m_qwindow != QObjectThreadSafeRef()) {
        qOhosReportFatalErrorAndAbort(
            "QAbilityPeerImpl: overwriting previously set qwindow: %s => %s",
            m_qwindow.refName().c_str(), qwindow.refName().c_str());
    }

    m_qwindow = qwindow;

    auto qWindowDestroyPromiseDeferred = QNapi::Promise::Deferred::New(env);
    m_optQWindowDestroyPromiseData = std::make_shared<QWindowDestroyPromiseData>(
        QWindowDestroyPromiseData{
            .deferred = qWindowDestroyPromiseDeferred,
            .promise = QNapi::Reference<QNapi::Promise>::makePersistentFrom(
                qWindowDestroyPromiseDeferred.Promise()),
        });

    auto resolveQWindowDestroyPromiseFunc =
        [qWindowRef = qwindow, weakPromiseData = QtOhos::makeWeakPtr(m_optQWindowDestroyPromiseData)]() {
            qOhosPrintfDebug(
                "%s: sending request to resolve QWindow destroy notify Promise if needed: %s",
                Q_FUNC_INFO, qWindowRef.refName().c_str());
            QtOhos::invokeInJsThread(
                [qWindowRef, weakPromiseData](JsState &jsState) {
                    auto promiseData = weakPromiseData.lock();
                    if (promiseData && promiseData->deferred) {
                        qOhosPrintfDebug(
                            "%s: resolving QWindow destroy notify Promise: %s",
                            Q_FUNC_INFO, qWindowRef.refName().c_str());
                        auto napiUndefined = Napi::Env(jsState.env()).Undefined();
                        std::exchange(promiseData->deferred, std::nullopt)->Resolve(napiUndefined);
                    } else {
                        qOhosPrintfDebug(
                            "%s: QAbilityPeer already removed on QWindow's destroy notification: %s",
                            Q_FUNC_INFO, qWindowRef.refName().c_str());
                    }
                });
        };

    QtOhos::invokeInQtThread(
        [qWindowRef = qwindow, resolveQWindowDestroyPromiseFunc]() {
            auto *qWindow = static_cast<QWindow *>(qWindowRef.data().data());
            auto *platformWindow = qWindow != nullptr ? QOhosPlatformWindow::fromQWindowOrNull(qWindow) : nullptr;
            auto *view = platformWindow != nullptr ? platformWindow->ownedViewOrNull() : nullptr;

            if (view != nullptr) {
                qCDebug(
                    QtForOhos, "%s: connecting to QOhosView's 'destroyed', win: %s",
                    Q_FUNC_INFO, qWindowRef.refName().c_str());
                QObject::connect(
                    view, &QObject::destroyed,
                    [qWindowRef, resolveQWindowDestroyPromiseFunc](QObject *) {
                        qCDebug(
                            QtForOhos, "%s: got QOhosView's 'destroyed' signal, win: %s",
                            Q_FUNC_INFO, qWindowRef.refName().c_str());
                        resolveQWindowDestroyPromiseFunc();
                    });
            } else {
                qCDebug(
                    QtForOhos, "%s: QOhosView already destroyed, win: %s",
                    Q_FUNC_INFO, qWindowRef.refName().c_str());
                resolveQWindowDestroyPromiseFunc();
            }
        });
}

class QUiAbilityPeerImpl final : public virtual QUiAbilityPeer, public QAbilityPeerImpl, public QUiAbilityPeerBackend
{
public:
    QUiAbilityPeerImpl(
        JsState &jsState, std::shared_ptr<QAbilityEngine> abilityEngine,
        std::string instanceId, QObjectThreadSafeRef qwindow,
        QNapi::Object uiAbility, QNapi::Object windowStage);

    QNapi::Object uiContext() override;
    QNapi::Object launchWant() override;
    QNapi::Object launchParam() override;
    QNapi::Object windowStage() override;
    QNapi::Object window() override;
    bool isTerminating() override;

    QNapi::Promise handleCloseRequestFromSystem(
        JsState &jsState, const std::string &logContextStr, CloseAbilityRequestSource requestSource,
        std::function<QNapi::Value(JsState &, CloseAbilityRequestResolution)> promiseValueFactory) override;

    void handleOnContinueRequestFromSystem(
        JsState &jsState, QNapi::Object wantParamsObj,
        QOhosConsumer<JsState &, QOhosAbilityOnContinueResult> resultConsumer) override;

    void setOnContinueRequestsHandler(
        std::function<void(JsState &, QNapi::Object, QOhosConsumer<JsState &, QOhosAbilityOnContinueResult>)> requestsHandler) override;

private:
    QNapi::Reference<QNapi::Object> m_windowStage;
    QNapi::Reference<QNapi::Object> m_window;
    QNapi::Reference<QNapi::Object> m_launchParam;
    std::shared_ptr<void> m_windowWillCloseCallbackHandle;
    std::function<void(JsState &, QNapi::Object, QOhosConsumer<JsState &, QOhosAbilityOnContinueResult>)> m_onContinueRequestsHandler;
};

QUiAbilityPeerImpl::QUiAbilityPeerImpl(
    JsState &jsState, std::shared_ptr<QAbilityEngine> abilityEngine,
    std::string instanceId, QObjectThreadSafeRef qwindow,
    QNapi::Object uiAbility, QNapi::Object windowStage)
    : QAbilityPeerImpl(jsState, abilityEngine, instanceId, qwindow, uiAbility)
    , m_windowStage(Napi::Persistent(windowStage))
    , m_window(Napi::Persistent(windowStage.call<QNapi::Object>("getMainWindowSync")))
{
    auto optLaunchParam = QNapi::getOptionalPropOrEmpty<QNapi::Object>(
        uiAbility, getAbilityLaunchParamPropSymbol(jsState));
    if (optLaunchParam.IsEmpty()) {
        qOhosReportFatalErrorAndAbort(
            "Ability with instanceId='%s' doesn't have 'launchParam' set. It shouldn't happen!",
            instanceId.c_str());
    }
    m_launchParam = QNapi::Reference<>::makePersistentFrom(optLaunchParam);

    m_windowWillCloseCallbackHandle = registerQOhosOnOffMethodsBasedEventHandler(
        window(), "windowWillClose",
        [this](const CallbackInfo &cbInfo) {
            JsState &jsState = cbInfo.jsState();
            return handleCloseRequestFromSystem(
                jsState, "Window.windowWillClose",
                CloseAbilityRequestSource::WindowWillClose,
                [](JsState &jsState, CloseAbilityRequestResolution ohosRequestResolution) {
                    return QNapi::Boolean::New(
                        jsState.env(),
                        ohosRequestResolution == CloseAbilityRequestResolution::DontClose);
                });
        },
        {
            .optEventSourceAliveCheckFunc =
                [qAbilityWeakRef = moveToSharedPtr(Napi::Weak(uiAbility))](QNapi::Object window) {
                    bool isWindowClosing = evalInJsThread(
                        [&](JsState &) {
                            return JsWindowsTracker::isWindowClosing(window);
                        },
                        Q_FUNC_INFO);
                    auto qAbilityValue = qAbilityWeakRef->Value();
                    return
                        !isWindowClosing
                        && qAbilityValue.IsObject()
                        && !QNapi::checkedCast<QNapi::Object>(qAbilityValue).eval<QNapi::Boolean>("context.isTerminating()");
                },
            .optOnCallExceptionHandler = [](const Napi::Error &error) {
                constexpr auto capabilityNotSupportedErrorCode = 801;
                QtOhos::rethrowUnlessJsBusinessErrorIs(
                    error, capabilityNotSupportedErrorCode, "on('windowWillClose')");
            },
        });
}

QNapi::Object QUiAbilityPeerImpl::uiContext()
{
    return window().call<QNapi::Object>("getUIContext");
}

QNapi::Object QUiAbilityPeerImpl::launchWant()
{
    return qAbility().get<QNapi::Object>("launchWant");
}

QNapi::Object QUiAbilityPeerImpl::launchParam()
{
    return m_launchParam.Value();
}

QNapi::Object QUiAbilityPeerImpl::windowStage()
{
    return m_windowStage.Value();
}

QNapi::Object QUiAbilityPeerImpl::window()
{
    return m_window.Value();
}

bool QUiAbilityPeerImpl::isTerminating()
{
    return qAbility().call<QNapi::Boolean>("context.isTerminating");
}

QOhosCloseEventContext::CloseRootCause mapCloseAbilityRequestSourceToRootCause(
    QUiAbilityPeerBackend::CloseAbilityRequestSource requestSource)
{
    using CloseAbilityRequestSource = QUiAbilityPeerBackend::CloseAbilityRequestSource;
    using CloseRootCause = QOhosCloseEventContext::CloseRootCause;

    switch (requestSource) {
    case CloseAbilityRequestSource::OnPrepareToTerminate:
        return CloseRootCause::OnPrepareToTerminate;
    case CloseAbilityRequestSource::WindowWillClose:
        return CloseRootCause::WindowStageClose;
    }

    qOhosReportFatalErrorAndAbort("Unsupported CloseAbilityRequestSource: %d", static_cast<int>(requestSource));
}

QNapi::Promise QUiAbilityPeerImpl::handleCloseRequestFromSystem(
    JsState &jsState, const std::string &logContextStr, CloseAbilityRequestSource requestSource,
    std::function<QNapi::Value(JsState &, CloseAbilityRequestResolution)> promiseValueFactory)
{
    auto instanceId = this->instanceId();

    qOhosPrintfInfo(
        "%s: got close request from source %d, id: '%s'",
        logContextStr.c_str(), static_cast<int>(requestSource), instanceId.c_str());

    if (destroyAllowedFlag()->load()) {
        qOhosPrintfDebug(
            "%s: resolving immediately with 'false', id: '%s'",
            Q_FUNC_INFO, instanceId.c_str());
        JsWindowsTracker::tagWindowAsClosing(
            window(), printfToString("%s => false", logContextStr.c_str()).c_str());
        return makeResolvedPromise(
            promiseValueFactory(jsState, CloseAbilityRequestResolution::Close));
    }

    struct PromiseResolveFuncContext
    {
        std::string logContextStr;
        std::weak_ptr<QUiAbilityPeer> weakQAbilityPeer;
        std::shared_ptr<QNapi::Promise::Deferred> promiseDeferred;
        std::string instanceId;
        std::function<QNapi::Value(JsState &, CloseAbilityRequestResolution)> promiseValueFactory;
    };

    auto promiseResolveFuncContext = moveToSharedPtr(
        PromiseResolveFuncContext{
            .logContextStr = logContextStr,
            .weakQAbilityPeer = makeWeakPtr(shared_from_this()),
            .promiseDeferred = std::make_shared<QNapi::Promise::Deferred>(jsState.env()),
            .instanceId = instanceId,
            .promiseValueFactory = std::move(promiseValueFactory),
        });

    auto promiseResolveFunc = moveToSharedPtr(
        makeCallOnceConsumerWrapper<JsState &, CloseAbilityRequestResolution>(
            [context = promiseResolveFuncContext](JsState &jsState, CloseAbilityRequestResolution ohosRequestResolution) {
                auto qAbilityPeer = context->weakQAbilityPeer.lock();
                if (qAbilityPeer && ohosRequestResolution == CloseAbilityRequestResolution::Close) {
                    JsWindowsTracker::tagWindowAsClosing(
                        qAbilityPeer->window(), printfToString("%s => false", context->logContextStr.c_str()).c_str());
                }
                context->promiseDeferred->Resolve(
                    context->promiseValueFactory(jsState, ohosRequestResolution));
                qOhosPrintfInfo(
                    "%s: promise resolved as %d, id: '%s'",
                    context->logContextStr.c_str(), static_cast<int>(ohosRequestResolution), context->instanceId.c_str());
            }));

    constexpr auto promiseAutoresolveTimeout = 9s;
    setJsTimeout(
        jsState,
        [logContextStr, promiseResolveFunc, instanceId](const CallbackInfo &cbInfo) {
            if ((*promiseResolveFunc)(cbInfo.jsState(), CloseAbilityRequestResolution::DontClose)) {
                qOhosPrintfInfo(
                    "%s: promise resolved by timeout, id: '%s'",
                    logContextStr.c_str(), instanceId.c_str());
            }
        },
        promiseAutoresolveTimeout);

    qWindowRef().visitInQtThreadIfAlive(
        [logContextStr, requestSource, promiseResolveFunc, instanceId](auto &qWindowObj) {
            qOhosPrintfInfo(
                "%s: calling close(), id: '%s'",
                logContextStr.c_str(), instanceId.c_str());
            QOhosCloseEventContext::runWithCloseRootCauseAndCloseResolutionConsumerSet(
                mapCloseAbilityRequestSourceToRootCause(requestSource),
                [logContextStr, promiseResolveFunc, instanceId](QOhosCloseEventContext::CloseResolution closeResolution) {
                    auto ohosRequestResolution =
                        closeResolution == QOhosCloseEventContext::CloseResolution::Close
                            ? CloseAbilityRequestResolution::Close
                            : CloseAbilityRequestResolution::DontClose;
                    qOhosPrintfInfo(
                        "%s: requesting promise resolve to %d from Qt thread, id: '%s'",
                        logContextStr.c_str(), static_cast<int>(ohosRequestResolution), instanceId.c_str());
                    QtOhos::invokeInJsThread(
                        [logContextStr, promiseResolveFunc, ohosRequestResolution, instanceId](JsState &jsState) {
                            bool resolveCalled = (*promiseResolveFunc)(jsState, ohosRequestResolution);
                            qOhosPrintfInfo(
                                "%s: promise resolve to %d, called: %s, id: '%s'",
                                logContextStr.c_str(), static_cast<int>(ohosRequestResolution),
                                mapBoolToTrueFalseStr(resolveCalled), instanceId.c_str());
                        });
                },
                [&]() {
                    static_cast<QWindow &>(qWindowObj).close();
                });
        });

    qOhosPrintfInfo("%s: returning Promise, id: '%s'", logContextStr.c_str(), instanceId.c_str());

    return promiseResolveFuncContext->promiseDeferred->Promise();
}

void QUiAbilityPeerImpl::handleOnContinueRequestFromSystem(
    JsState &jsState, QNapi::Object wantParamsObj, QOhosConsumer<JsState &, QOhosAbilityOnContinueResult> resultConsumer)
{
    if (m_onContinueRequestsHandler) {
        m_onContinueRequestsHandler(jsState, wantParamsObj, std::move(resultConsumer));
    } else {
        qOhosPrintfDebug("%s: no handler set, rejecting", Q_FUNC_INFO);
        QtOhos::invokeInJsThread(
            [resultConsumer = std::move(resultConsumer)](JsState &jsState) {
                resultConsumer(jsState, QOhosAbilityOnContinueResult::REJECT);
            });
    }
}

void QUiAbilityPeerImpl::setOnContinueRequestsHandler(
    std::function<void(JsState &, QNapi::Object, QOhosConsumer<JsState &, QOhosAbilityOnContinueResult>)> requestsHandler)
{
    if (m_onContinueRequestsHandler) {
        qOhosReportFatalErrorAndAbort(
            "%s: overwriting previously set requests handler, id='%s'",
            Q_FUNC_INFO, instanceId().c_str());
    }

    m_onContinueRequestsHandler = std::move(requestsHandler);
}

class QAbilityInstancesManagerImpl final : public QAbilityInstancesManager
{
public:
    explicit QAbilityInstancesManagerImpl(
        std::shared_ptr<QAbilityEngine> abilityEngine,
        std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> autoStartedInstanceStartupNotifyFunc);

    std::shared_ptr<QAbilityEngine> abilityEngine() override;

    bool isWantFromThisApp(QNapi::Object appQAbility, QNapi::Object want) const override;

    QOhosOptional<std::string> tryGetQAbilityInstanceIdFromWant(QNapi::Object appQAbility, QNapi::Object want) const override;
    std::string getQAbilityInstanceIdOrPendingAutoStartedId(QNapi::Object qAbility) const override;

    QOhosOptional<std::string> pendingAutoStartedInstanceId() const override;

    void registerPendingAutoStartedInstance() override;

    void startNewInstance(
        QNapi::Object baseQAbility, QObjectThreadSafeRef qwindow,
        QNapi::Object optStartOptions,
        std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc) override;

    void handleStartedUiInstance(JsState &jsState, QNapi::Object qAbility, QNapi::Object windowStage) override;

    std::shared_ptr<QUiAbilityPeerBackend> getAbilityPeerBackend(std::shared_ptr<QUiAbilityPeer> uiAbilityPeer) override;

private:
    struct InstanceStartParams
    {
        QObjectThreadSafeRef qwindow;
        std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc;
    };

    void handleStartedInstance(
        JsState &jsState, const std::string &abilityInstanceId,
        const std::function<std::shared_ptr<QAbilityPeer>(const std::string &, const InstanceStartParams &)> &qAbilityPeerFactory);

    std::shared_ptr<QAbilityEngine> m_abilityEngine;
    std::shared_ptr<std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)>> m_autoStartedInstanceStartupNotifyFunc;
    std::shared_ptr<std::map<QUiAbilityPeer *, std::weak_ptr<QUiAbilityPeerImpl>>> m_ownedUiAbilityPeers;
    QOhosSupplier<std::string> m_abilityInstanceIdsGenerator;
    std::map<std::string, InstanceStartParams> m_pendingInstancesStartParams;
    QOhosOptional<std::string> m_pendingAutoStartedInstanceId;
};

QAbilityInstancesManagerImpl::QAbilityInstancesManagerImpl(
    std::shared_ptr<QAbilityEngine> abilityEngine,
    std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> autoStartedInstanceStartupNotifyFunc)
    : m_abilityEngine(abilityEngine)
    , m_autoStartedInstanceStartupNotifyFunc(moveToSharedPtr(std::move(autoStartedInstanceStartupNotifyFunc)))
    , m_ownedUiAbilityPeers(std::make_shared<std::map<QUiAbilityPeer *, std::weak_ptr<QUiAbilityPeerImpl>>>())
    , m_abilityInstanceIdsGenerator(makeAbilityInstanceIdsGenerator())
{
    registerPendingAutoStartedInstance();
}

QOhosOptional<std::string> QAbilityInstancesManagerImpl::tryGetQAbilityInstanceIdFromWant(
    QNapi::Object appQAbility, QNapi::Object want) const
{
    auto wantParameters = QNapi::getPropOrUndefined(want, "parameters");
    if (wantParameters.IsUndefined())
        return {};

    auto instanceIdParam = QNapi::getPropOrUndefined(wantParameters, qAbilityInstanceIdWantParamKey);

    bool validInstanceId = isWantFromThisApp(appQAbility, want) && instanceIdParam.IsString();

    return validInstanceId
        ? makeQOhosOptional(QNapi::checkedCast<QNapi::String>(instanceIdParam).Utf8Value())
        : makeEmptyQOhosOptional();
}

std::string QAbilityInstancesManagerImpl::getQAbilityInstanceIdOrPendingAutoStartedId(QNapi::Object qAbility) const
{
    auto optInstanceId = tryGetQAbilityInstanceIdFromWant(
        qAbility, qAbility.get<QNapi::Object>("launchWant"));

    if (!optInstanceId.has_value() && !m_pendingAutoStartedInstanceId.has_value()) {
        qOhosReportFatalErrorAndAbort(
            "Got Ability without '%s' param and no pending auto-started instance",
            qAbilityInstanceIdWantParamKey);
    }

    return optInstanceId.has_value()
        ? optInstanceId.value()
        : m_pendingAutoStartedInstanceId.value();
}

std::shared_ptr<QAbilityEngine> QAbilityInstancesManagerImpl::abilityEngine()
{
    return m_abilityEngine;
}

bool QAbilityInstancesManagerImpl::isWantFromThisApp(QNapi::Object appQAbility, QNapi::Object want) const
{
    auto wantParameters = QNapi::getPropOrUndefined(want, "parameters");
    if (wantParameters.IsUndefined())
        return false;

    auto callerAbilityNameParam = QNapi::getPropOrUndefined(wantParameters, callerAbilityNameWantParamKey);
    auto callerBundleNameParam = QNapi::getPropOrUndefined(wantParameters, callerBundleNameWantParamKey);

    auto appAbilityInfo = m_abilityEngine->readAbilityInfo(appQAbility);

    return
        callerAbilityNameParam.IsString()
        && QNapi::checkedCast<QNapi::String>(callerAbilityNameParam).Utf8Value() == appAbilityInfo.name
        && callerBundleNameParam.IsString()
        && QNapi::checkedCast<QNapi::String>(callerBundleNameParam).Utf8Value() == appAbilityInfo.bundleName;
}

QOhosOptional<std::string> QAbilityInstancesManagerImpl::pendingAutoStartedInstanceId() const
{
    return m_pendingAutoStartedInstanceId;
}

void QAbilityInstancesManagerImpl::registerPendingAutoStartedInstance()
{
    if (m_pendingAutoStartedInstanceId.has_value()) {
        qOhosReportFatalErrorAndAbort(
            "%s: pending auto-started instance already registered (id='%s')",
            Q_FUNC_INFO, m_pendingAutoStartedInstanceId.value().c_str());
    }

    m_pendingAutoStartedInstanceId = m_abilityInstanceIdsGenerator();

    qOhosPrintfInfo(
        "Registered pending auto-started Ability instance, id='%s'",
        m_pendingAutoStartedInstanceId.value().c_str());
}

void QAbilityInstancesManagerImpl::startNewInstance(
    QNapi::Object baseQAbility, QObjectThreadSafeRef qwindow,
    QNapi::Object optStartOptions,
    std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc)
{
    struct Context
    {
        std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc;
        bool sendWantFinished = false;
        std::shared_ptr<QAbilityPeer> optAbilityPeer;

        void callNotifyIfReady(JsState &jsState)
        {
            qOhosPrintfDebug(
                "%s: sendWantFinished=%s, startedAbilityPeer=%s",
                Q_FUNC_INFO, mapBoolToTrueFalseStr(sendWantFinished), mapBoolToTrueFalseStr(!!optAbilityPeer));
            if (sendWantFinished && optAbilityPeer)
                startupNotifyFunc(jsState, optAbilityPeer);
        }
    };

    auto context = std::make_shared<Context>();
    context->startupNotifyFunc = std::move(startupNotifyFunc);

    auto instanceId = m_abilityInstanceIdsGenerator();

    InstanceStartParams startParams;
    startParams.qwindow = qwindow;
    startParams.startupNotifyFunc = [context](JsState &jsState, std::shared_ptr<QAbilityPeer> abilityPeer) {
        context->optAbilityPeer = abilityPeer;
        context->callNotifyIfReady(jsState);
    };
    m_pendingInstancesStartParams.emplace(instanceId, std::move(startParams));

    sendWantToSelfQAbility(
        m_abilityEngine, baseQAbility, optStartOptions, instanceId,
        [context](JsState &jsState) {
            context->sendWantFinished = true;
            context->callNotifyIfReady(jsState);
        });
}

void QAbilityInstancesManagerImpl::handleStartedUiInstance(
    JsState &jsState, QNapi::Object qAbility, QNapi::Object windowStage)
{
    handleStartedInstance(
        jsState, getQAbilityInstanceIdOrPendingAutoStartedId(qAbility),
        [&](const std::string &instanceId, const InstanceStartParams &startParams) {
            auto uiAbilityPeer = std::make_shared<QUiAbilityPeerImpl>(
                jsState, m_abilityEngine, instanceId, startParams.qwindow, qAbility, windowStage);
            QUiAbilityPeer *uiAbilityPeerKey = uiAbilityPeer.get();
            m_ownedUiAbilityPeers->emplace(uiAbilityPeerKey, uiAbilityPeer);
            return makeSharedPtrWithAttachedExtraData(
                uiAbilityPeer,
                makeDestroyNotifier(
                    [ownedUiAbilityPeers = m_ownedUiAbilityPeers, uiAbilityPeerKey]() {
                        ownedUiAbilityPeers->erase(uiAbilityPeerKey);
                    }));
        });
}

std::shared_ptr<QUiAbilityPeerBackend> QAbilityInstancesManagerImpl::getAbilityPeerBackend(
    std::shared_ptr<QUiAbilityPeer> uiAbilityPeer)
{
    auto ownedPeerEntryIter = m_ownedUiAbilityPeers->find(uiAbilityPeer.get());
    auto ownedPeerImpl =
        ownedPeerEntryIter != m_ownedUiAbilityPeers->end()
            ? ownedPeerEntryIter->second.lock()
            : nullptr;

    if (!ownedPeerImpl) {
        qOhosReportFatalErrorAndAbort(
            "%s: got unknown QUiAbilityPeer %p with id='%s'",
            Q_FUNC_INFO, uiAbilityPeer.get(), uiAbilityPeer->instanceId().c_str());
    }

    return ownedPeerImpl;
}

void QAbilityInstancesManagerImpl::handleStartedInstance(
    JsState &jsState, const std::string &abilityInstanceId,
    const std::function<std::shared_ptr<QAbilityPeer>(const std::string &, const InstanceStartParams &)> &qAbilityPeerFactory)
{
    auto startParamsIter = m_pendingInstancesStartParams.find(abilityInstanceId);
    if (startParamsIter != m_pendingInstancesStartParams.end()) {
        auto startParams = std::move(startParamsIter->second);
        m_pendingInstancesStartParams.erase(startParamsIter);

        auto qAbilityPeer = qAbilityPeerFactory(abilityInstanceId, startParams);
        QtOhos::addJsQAbilityPeer(qAbilityPeer);
        startParams.startupNotifyFunc(jsState, qAbilityPeer);
    } else if (abilityInstanceId == m_pendingAutoStartedInstanceId) {
        m_pendingAutoStartedInstanceId.reset();

        InstanceStartParams startParams = {
            .qwindow = QObjectThreadSafeRef(),
            .startupNotifyFunc = [startupNotifyFunc = m_autoStartedInstanceStartupNotifyFunc](JsState &jsState, std::shared_ptr<QAbilityPeer> qAbilityPeer) {
                (*startupNotifyFunc)(jsState, qAbilityPeer);
            },
        };
        auto qAbilityPeer = qAbilityPeerFactory(abilityInstanceId, startParams);
        QtOhos::addJsQAbilityPeer(qAbilityPeer);
        startParams.startupNotifyFunc(jsState, qAbilityPeer);
    } else {
        qOhosPrintfError(
            "%s: got unexpected started instance notification with id='%s', ignoring",
            Q_FUNC_INFO, abilityInstanceId.c_str());
    }
}

}

QUiAbilityPeerBackend::QUiAbilityPeerBackend() = default;

QAbilityInstancesManager::QAbilityInstancesManager() = default;

QAbilityInstancesManager::~QAbilityInstancesManager() = default;

bool QAbilityInstancesManager::isQtInternalWantFromThisProcess(QNapi::Object want)
{
    auto optWantParameters = QNapi::getOptionalPropOrEmpty<QNapi::Object>(want, "parameters");

    auto optCallerPid = QNapi::getOptionalPropOrEmpty<QNapi::Number>(
        optWantParameters, callerPidWantParamKey);
    auto optQtInternalRequestId = QNapi::getOptionalPropOrEmpty<QNapi::String>(
        optWantParameters, qtInternalRequestIdWantParamKey);

    std::int32_t thisProcessPid = ::getpid();

    return
        !optCallerPid.IsEmpty() && optCallerPid.Int32Value() == thisProcessPid
        && !optQtInternalRequestId.IsEmpty()
        && optQtInternalRequestId.Utf8Value() == getUtf8QtInternalRequestIdForThisProcess();
}

void QAbilityInstancesManager::setLaunchParamOnAbilityObject(
    JsState &jsState, QNapi::Object ability, QNapi::Object launchParam)
{
    ability.set(getAbilityLaunchParamPropSymbol(jsState), launchParam);
}

std::shared_ptr<QAbilityInstancesManager> makeQAbilityInstancesManager(
    std::shared_ptr<QAbilityEngine> abilityEngine,
    std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> autoStartedInstanceStartupNotifyFunc)
{
    return std::make_shared<QAbilityInstancesManagerImpl>(abilityEngine, std::move(autoStartedInstanceStartupNotifyFunc));
}

}

QT_END_NAMESPACE
