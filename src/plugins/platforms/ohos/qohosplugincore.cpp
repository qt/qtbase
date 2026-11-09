// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosplugincore.h>

#include "qohosmtblockingcallsgateway_p.h"
#include <QtCore/private/qohosjstools_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/private/qohosmemoizingjsthreadfetcher_p.h>
#include <QtCore/qabstracteventdispatcher.h>
#include <algorithm>
#include <cstdint>
#include <deque>
#include <exception>
#include <future>
#include <map>
#include <mutex>
#include <napi.h>
#include <napi/native_api.h>
#include <optional>
#include <qohosjscallbatchtrace.h>
#include <qohosqabilityinstancesmanager.h>
#include <qohosutils.h>
#include <render/qohosjswindowregistry.h>
#include <tuple>
#include <typeindex>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhos {

namespace {

constexpr const char *napiLoadableModules[] = {
    "@ohos.abilityAccessCtrl",
    "@ohos.accessibility",
    "@ohos.app.ability.AbilityConstant",
    "@ohos.app.ability.ConfigurationConstant",
    "@ohos.app.ability.childProcessManager",
    "@ohos.app.ability.contextConstant",
    "@ohos.app.ability.wantConstant",
    "@ohos.arkui.uiExtension",
    "@ohos.bluetooth.access",
    "@ohos.bluetooth.connection",
    "@ohos.bluetooth.socket",
    "@ohos.bundle.bundleManager",
    "@ohos.commonEventManager",
    "@ohos.data.uniformTypeDescriptor",
    "@ohos.deviceInfo",
    "@ohos.display",
    "@ohos.file.fileuri",
    "@ohos.file.picker",
    "@ohos.font",
    "@ohos.geoLocationManager",
    "@ohos.graphics.text",
    "@ohos.i18n",
    "@ohos.inputMethod",
    "@ohos.intl",
    "@ohos.multimedia.image",
    "@ohos.multimedia.media",
    "@ohos.multimodalInput.inputDevice",
    "@ohos.multimodalInput.pointer",
    "@ohos.net.connection",
    "@ohos.notificationManager",
    "@ohos.pasteboard",
    "@ohos.resourceManager",
    "@ohos.settings",
    "@ohos.web.webview",
    "@ohos.wifiManager",
    "@ohos.window",
};

template<typename Task>
class PreQueuingTasksExecutor
{
public:
    void setUnderlyingExecutor(
        std::function<void(Task)> executor,
        std::function<void(Task)> optSyncFlushExecutor = nullptr);
    void invokeTask(Task task);

private:
    std::recursive_mutex m_executorMutex;
    std::function<void(Task)> m_optUnderlyingExecutor;
    std::deque<Task> m_pendingTasks;
};

template<typename Task>
void PreQueuingTasksExecutor<Task>::setUnderlyingExecutor(
    std::function<void(Task)> executor, std::function<void(Task)> optSyncFlushExecutor)
{
    std::lock_guard<std::recursive_mutex> executorLock(m_executorMutex);
    auto &flushExecutor = optSyncFlushExecutor ? optSyncFlushExecutor : executor;
    while (!m_pendingTasks.empty()) {
        auto task = std::move(m_pendingTasks.front());
        m_pendingTasks.pop_front();
        flushExecutor(std::move(task));
    }
    m_optUnderlyingExecutor = std::move(executor);
}

template<typename Task>
void PreQueuingTasksExecutor<Task>::invokeTask(Task task)
{
    std::lock_guard<std::recursive_mutex> executorLock(m_executorMutex);
    if (m_optUnderlyingExecutor)
        m_optUnderlyingExecutor(std::move(task));
    else
        m_pendingTasks.push_back(std::move(task));
}

using PreQueuingJsTasksExecutor = PreQueuingTasksExecutor<std::function<void(JsState &)>>;

std::function<void(std::function<void(JsState &)>)> makeJsThreadTasksExecutor(JsState *jsState)
{
    struct Context {
        std::mutex pendingTasksMutex;
        bool pendingTasksRunnerActive = false;
        std::vector<std::function<void(JsState &)>> pendingTasks;
        Napi::ThreadSafeFunction executorThreadSafeFunc;
    };
    auto ctx = std::make_shared<Context>();

    ctx->executorThreadSafeFunc = Napi::ThreadSafeFunction::New(
        jsState->env(),
        QNapi::Function::New(
            jsState->env(),
            [ctx = ctx.get()](const CallbackInfo &cbInfo) {
                while (true) {
                    std::vector<std::function<void(JsState &)>> pendingTasks;
                    {
                        std::lock_guard<std::mutex> pendingTasksLock(ctx->pendingTasksMutex);
                        pendingTasks = std::exchange(ctx->pendingTasks, {});
                        if (pendingTasks.empty()) {
                            ctx->pendingTasksRunnerActive = false;
                            break;
                        }
                    }
                    for (auto &task : pendingTasks)
                        task(cbInfo.jsState());
                }
            },
            "QCoreOhosTasksExecutorFunc"),
        "QCoreOhosTasksExecutor", 0, 1);

    return [ctx](std::function<void(JsState &)> task) {
        std::lock_guard<std::mutex> pendingTasksLock(ctx->pendingTasksMutex);
        ctx->pendingTasks.push_back(std::move(task));
        if (!ctx->pendingTasksRunnerActive) {
            ctx->executorThreadSafeFunc.NonBlockingCall();
            ctx->pendingTasksRunnerActive = true;
        }
    };
}

QNapi::Symbol getJsWindowsTrackerIsClosingPropSymbol(JsState &jsState)
{
    struct SymbolTag
    {
    };

    return jsState.getJsSymbolForType<SymbolTag>();
}

std::optional<QNapi::Object> tryLoadJsModuleViaNapi(napi_env env, const std::string &moduleName)
{
    qOhosPrintfDebug("%s: loading napi module '%s' via napi_load_module()", Q_FUNC_INFO, moduleName.c_str());

    napi_value result = nullptr;
    auto status = napi_load_module(env, moduleName.c_str(), &result);
    if (status != napi_ok) {
        Napi::Env napiEnv(env);
        if (napiEnv.IsExceptionPending())
            napiEnv.GetAndClearPendingException();
        qOhosPrintfWarning(
            "%s: napi_load_module failed for module '%s' (status=%d)",
            Q_FUNC_INFO, moduleName.c_str(), static_cast<int>(status));
        return std::nullopt;
    }

    Napi::Value resultValue(env, result);
    if (!resultValue.IsObject()) {
        qOhosPrintfWarning(
            "%s: napi_load_module returned %s instead of an object for module '%s'",
            Q_FUNC_INFO, QNapi::getValueTypeString(resultValue).c_str(), moduleName.c_str());
        return std::nullopt;
    }

    return QNapi::Object(env, result);
}

QNapi::Object loadJsModuleViaNapiOrFail(napi_env env, const std::string &moduleName)
{
    auto optModule = tryLoadJsModuleViaNapi(env, moduleName);
    if (!optModule)
        qOhosReportFatalErrorAndAbort("%s: required module '%s' not loaded", Q_FUNC_INFO, moduleName.c_str());

    return optModule.value();
}

QNapi::Object loadJsModuleViaEtsFactoryOrFail(
    const QNapi::Function &etsModuleFactory, const std::string &moduleName)
{
    qOhosPrintfDebug("%s: loading napi module '%s' via ets factory", Q_FUNC_INFO, moduleName.c_str());

    auto result = etsModuleFactory.Call({});
    if (!result.IsObject()) {
        qOhosReportFatalErrorAndAbort(
            "%s: ets factory for module '%s' did not return an object",
            Q_FUNC_INFO, moduleName.c_str());
    }

    return QNapi::checkedCast<QNapi::Object>(result);
}

template<typename T>
std::function<T(JsState &)> makeMemoizingJsValueSupplier(std::function<T(JsState &)> baseJsSupplier)
{
    auto memoizedValue = moveToSharedPtr(QNapi::Reference<T>());
    return [baseJsSupplier = std::move(baseJsSupplier), memoizedValue](JsState &jsState) mutable {
        if (memoizedValue->IsEmpty()) {
            *memoizedValue = QNapi::Reference<T>::makePersistentFrom(baseJsSupplier(jsState));
            baseJsSupplier = nullptr;
        }
        return memoizedValue->Value();
    };
}

class JsStateImpl : public JsState
{
public:
    JsStateImpl() = default;

    void initInJsThread(
        napi_env env, std::map<std::string, QNapi::Reference<QNapi::Function>> &&etsModulesFactories,
        std::shared_ptr<AppFunctions> appFunctions, QtRunMode qtRunMode);

    bool isJsThread();

    void dispatchNewWantInJsThread(QNapi::Object qAbility, QNapi::Object want, QNapi::Object launchParam);
    void callUiAbilityDelegates(const char *methodName, const std::vector<QNapi::Value> &args);
    std::optional<QNapi::Value> tryCallFirstUiAbilityDelegate(
        const char *methodName, const std::vector<QNapi::Value> &args, QNapi::Value resultOnThrow);

    void invokeTask(std::function<void(JsState &)> &&task);

    napi_env env() override;
    QNapi::Object getModule(const std::string &moduleName);
    std::vector<std::string> predefinedJsModuleNames() const;

    QNapi::Object appLaunchWant() override;
    std::optional<QNapi::Object> optAppLaunchParam() override;
    std::optional<QNapi::Object> tryGetModule(const std::string &moduleName) override;

    QNapi::Object defaultWindowStageOrEmpty() override;
    QNapi::Object defaultUiContextOrEmpty() override;

    std::optional<QNapi::Object> tryGetJsWindowByQWindow(QObjectThreadSafeRef qwindow) override;
    std::optional<QNapi::Object> tryGetQAbilityByQWindow(QObjectThreadSafeRef qwindow) override;
    std::optional<QNapi::Object> defaultQAbility() override;

    void startAppProcess(
        const std::string &processId, QNapi::Object requestWant,
        QNapi::Object optStartOptions, std::function<void(QOhosJsState &)> continueFunc) override;

    std::shared_ptr<void> registerNewWantConsumer(
        QOhosConsumer<QOhosJsState &, QNapi::Object, QNapi::Object, QNapi::Object> wantConsumer) override;

    std::shared_ptr<void> registerUiAbilityDelegate(QNapi::Object delegate) override;

    void setOnContinueRequestsHandler(
        QNapi::Object qAbility,
        std::function<void(QOhosJsState &, QNapi::Object, QOhosConsumer<QOhosJsState &, QNapi::Number>)> requestsHandler) override;

    void setDestroyFromSystemAllowed(QNapi::Object qAbility, bool destroyAllowed) override;

    void startNoUiChildProcess(const std::string &libraryName, const std::vector<std::string> &args) override;

    QtRunMode qtRunMode() override;

private:
    QAbilityInstancesManager &qAbilityInstancesManager();

    void *getAttachedObjectWithLazyCreate(
        const std::type_info &objectTypeInfo, QOhosSupplier<std::shared_ptr<void>> objectFactory) override;

    std::tuple<QNapi::Object, std::string> extractModuleFromEvalExpr(const std::string &expr) override;
    QNapi::Number mapOhosEnumToJs(int enumValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)()) override;
    std::optional<int> tryMapOhosEnumFromJs(QNapi::Number enumJsValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)()) override;
    int mapOhosEnumFromJs(QNapi::Number enumJsValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)()) override;
    QNapi::Symbol getJsSymbolForType(const std::type_info &typeInfo) override;

    const std::vector<std::pair<int, double>> &getOhosEnumEnumerators(
        const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)());
    std::vector<std::pair<int, double>> resolveOhosEnumEnumerators(const OhosEnumInfo &enumInfo);

    pthread_t m_jsThread = {};
    napi_env m_env = nullptr;
    std::map<std::string, std::function<QNapi::Object(JsState &)>> m_jsModulesFactories;
    std::map<std::string, QNapi::Reference<QNapi::Object>> m_onDemandJsModules;
    std::shared_ptr<AppFunctions> m_appFunctions;
    PreQueuingJsTasksExecutor m_tasksExecutor;
    std::map<std::uint64_t, std::shared_ptr<QOhosConsumer<QOhosJsState &, QNapi::Object, QNapi::Object, QNapi::Object>>> m_newWantConsumers;
    std::uint64_t m_nextNewWantConsumerId = 1;
    std::map<std::uint64_t, QNapi::Reference<QNapi::Object>> m_uiAbilityDelegates;
    std::uint64_t m_nextUiAbilityDelegateId = 1;
    std::map<std::type_index, std::vector<std::pair<int, double>>> m_ohosEnumsEnumerators;
    std::map<std::type_index, QNapi::Reference<QNapi::Symbol>> m_jsSymbolsRefs;
    QtRunMode m_qtRunMode;
    std::map<std::type_index, std::shared_ptr<void>> m_attachedObjects;
};

bool JsStateImpl::isJsThread()
{
    return pthread_self() == m_jsThread;
}

void JsStateImpl::initInJsThread(
    napi_env env, std::map<std::string, QNapi::Reference<QNapi::Function>> &&etsModulesFactories,
    std::shared_ptr<AppFunctions> appFunctions, QtRunMode qtRunMode)
{
    m_jsThread = pthread_self();
    m_env = env;
    m_appFunctions = appFunctions;
    m_qtRunMode = qtRunMode;

    for (const char *moduleName : napiLoadableModules) {
        m_jsModulesFactories.emplace(
            moduleName,
            makeMemoizingJsValueSupplier<QNapi::Object>(
                [moduleName](JsState &jsState) {
                    return loadJsModuleViaNapiOrFail(jsState.env(), moduleName);
                }));
    }
    for (auto &etsModulesFactoryEntry : etsModulesFactories) {
        const std::string &moduleName = etsModulesFactoryEntry.first;
        auto etsModuleFactory = moveToSharedPtr(std::move(etsModulesFactoryEntry.second));
        m_jsModulesFactories.emplace(
            moduleName,
            makeMemoizingJsValueSupplier<QNapi::Object>(
                [moduleName, etsModuleFactory](JsState &) {
                    return loadJsModuleViaEtsFactoryOrFail(etsModuleFactory->Value(), moduleName);
                }));
    }
    m_jsModulesFactories.emplace(
        "Global",
        [](JsState &jsState) -> QNapi::Object {
            return Napi::Env(jsState.env()).Global();
        });

    m_tasksExecutor.setUnderlyingExecutor(makeJsThreadTasksExecutor(this));
}

void JsStateImpl::dispatchNewWantInJsThread(
    QNapi::Object qAbility, QNapi::Object want, QNapi::Object launchParam)
{
    std::vector<std::uint64_t> consumersIds;
    for (const auto &consumerEntry : m_newWantConsumers)
        consumersIds.push_back(consumerEntry.first);

    for (auto consumerId : consumersIds) {
        auto consumerIter = m_newWantConsumers.find(consumerId);
        if (consumerIter != m_newWantConsumers.end()) {
            auto consumer = consumerIter->second;
            (*consumer)(*this, qAbility, want, launchParam);
        }
    }
}

std::optional<QNapi::Function> tryGetUiAbilityDelegateMethod(QNapi::Object delegate, const char *methodName)
{
    auto methodValue = QNapi::getPropOrUndefined(delegate, methodName);
    if (methodValue.IsFunction())
        return QNapi::checkedCast<QNapi::Function>(methodValue);

    if (!methodValue.IsUndefined()) {
        qOhosPrintfWarning(
            "%s: the '%s' property of a UIAbility delegate is not a function, ignoring it",
            Q_FUNC_INFO, methodName);
    }
    return std::nullopt;
}

void JsStateImpl::callUiAbilityDelegates(const char *methodName, const std::vector<QNapi::Value> &args)
{
    const auto logContext = std::string("UIAbility delegate ") + methodName + "()";

    std::vector<std::uint64_t> delegatesIds;
    for (const auto &delegateEntry : m_uiAbilityDelegates)
        delegatesIds.push_back(delegateEntry.first);

    for (auto delegateId : delegatesIds) {
        auto delegateIter = m_uiAbilityDelegates.find(delegateId);
        if (delegateIter != m_uiAbilityDelegates.end()) {
            runQOhosJsCodeLoggingExceptions(
                m_env, logContext.c_str(),
                [&]() {
                    auto delegate = delegateIter->second.Value();
                    auto optMethod = tryGetUiAbilityDelegateMethod(delegate, methodName);
                    if (optMethod)
                        optMethod->Call(delegate, args);
                });
        }
    }
}

std::optional<QNapi::Value> JsStateImpl::tryCallFirstUiAbilityDelegate(
    const char *methodName, const std::vector<QNapi::Value> &args, QNapi::Value resultOnThrow)
{
    std::vector<std::uint64_t> delegatesIds;
    for (const auto &delegateEntry : m_uiAbilityDelegates)
        delegatesIds.push_back(delegateEntry.first);

    for (auto delegateId : delegatesIds) {
        auto delegateIter = m_uiAbilityDelegates.find(delegateId);
        if (delegateIter != m_uiAbilityDelegates.end()) {
            try {
                auto delegate = delegateIter->second.Value();
                auto optMethod = tryGetUiAbilityDelegateMethod(delegate, methodName);
                if (optMethod)
                    return optMethod->Call(delegate, args);
            } catch (const Napi::Error &error) {
                qOhosPrintfWarning(
                    "%s: UIAbility delegate %s() threw: %s", Q_FUNC_INFO, methodName, error.what());
                return resultOnThrow;
            }
        }
    }

    return std::nullopt;
}

void JsStateImpl::invokeTask(std::function<void(JsState &)> &&task)
{
    m_tasksExecutor.invokeTask(std::move(task));
}

napi_env JsStateImpl::env()
{
    return m_env;
}

std::vector<std::string> JsStateImpl::predefinedJsModuleNames() const
{
    std::vector<std::string> moduleNames;
    for (const auto &moduleFactoryEntry : m_jsModulesFactories)
        moduleNames.push_back(moduleFactoryEntry.first);
    return moduleNames;
}

QNapi::Object JsStateImpl::getModule(const std::string &moduleName)
{
    using namespace std::string_literals;

    auto moduleFactoryIter = m_jsModulesFactories.find(moduleName);
    if (moduleFactoryIter == m_jsModulesFactories.end())
        throw QNapi::makeLoggedException(m_env, "JS module not found: "s + moduleName);

    return moduleFactoryIter->second(*this);
}

std::optional<QNapi::Object> JsStateImpl::tryGetModule(const std::string &moduleName)
{
    auto moduleFactoryIter = m_jsModulesFactories.find(moduleName);
    if (moduleFactoryIter != m_jsModulesFactories.end())
        return moduleFactoryIter->second(*this);

    auto onDemandModuleIter = m_onDemandJsModules.find(moduleName);
    if (onDemandModuleIter != m_onDemandJsModules.end())
        return onDemandModuleIter->second.Value();

    auto optModule = tryLoadJsModuleViaNapi(m_env, moduleName);
    if (!optModule)
        return std::nullopt;

    m_onDemandJsModules.emplace(moduleName, QNapi::Reference<>::makePersistentFrom(optModule.value()));
    return optModule;
}

QNapi::Object JsStateImpl::appLaunchWant()
{
    return qAbilityInstancesManager().appLaunchWant();
}

std::optional<QNapi::Object> JsStateImpl::optAppLaunchParam()
{
    return qAbilityInstancesManager().optAppLaunchParam();
}

QNapi::Object JsStateImpl::defaultWindowStageOrEmpty()
{
    auto qUiAbilityPeer = QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(qAbilityInstancesManager().defaultQAbilityPeer());
    if (!qUiAbilityPeer)
        return QNapi::Object();

    return qUiAbilityPeer->windowStage();
}

QNapi::Object JsStateImpl::defaultUiContextOrEmpty()
{
    return qAbilityInstancesManager().defaultQAbilityPeer()->uiContext();
}

std::optional<QNapi::Object> JsStateImpl::tryGetJsWindowByQWindow(QObjectThreadSafeRef qwindow)
{
    auto optJsWindowRef = QOhosJsWindowRegistry::instance(*this).tryFindJsWindowByQWindowRef(qwindow);
    return optJsWindowRef ? std::optional(optJsWindowRef->jsObject()) : std::nullopt;
}

std::optional<QNapi::Object> JsStateImpl::tryGetQAbilityByQWindow(QObjectThreadSafeRef qwindow)
{
    auto qAbilityPeer = qAbilityInstancesManager().tryGetQAbilityPeerByQWindow(qwindow);
    if (!qAbilityPeer) {
        auto optJsWindowRef = QOhosJsWindowRegistry::instance(*this).tryFindJsWindowByQWindowRef(qwindow);
        if (optJsWindowRef)
            qAbilityPeer = qAbilityInstancesManager().tryGetQAbilityPeerByInstanceId(
                optJsWindowRef->owningQAbilityInstanceId());
    }
    return qAbilityPeer ? std::optional(qAbilityPeer->qAbility()) : std::nullopt;
}

std::optional<QNapi::Object> JsStateImpl::defaultQAbility()
{
    auto qAbility = qAbilityInstancesManager().defaultQAbilityPeer()->qAbility();
    return !qAbility.IsEmpty() ? std::optional(qAbility) : std::nullopt;
}

void JsStateImpl::setOnContinueRequestsHandler(
    QNapi::Object qAbility,
    std::function<void(QOhosJsState &, QNapi::Object, QOhosConsumer<QOhosJsState &, QNapi::Number>)> requestsHandler)
{
    auto uiAbilityPeer = QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(
        qAbilityInstancesManager().tryGetQAbilityPeerByInstance(qAbility));
    if (!uiAbilityPeer) {
        qOhosPrintfError(
            "%s: no QUiAbilityPeer for the given qAbility, handler not set", Q_FUNC_INFO);
        return;
    }

    uiAbilityPeer->setOnContinueRequestsHandler(
        [requestsHandler = std::move(requestsHandler)](
            JsState &jsState, QNapi::Object wantParams,
            QOhosConsumer<JsState &, QOhosAbilityOnContinueResult> resultConsumer) {
            requestsHandler(
                jsState, wantParams,
                [resultConsumer = std::move(resultConsumer)](QOhosJsState &resultJsState, QNapi::Number result) {
                    auto optResultEnum = resultJsState.tryMapOhosEnumFromJs<QOhosAbilityOnContinueResult>(result);
                    if (!optResultEnum) {
                        qOhosPrintfWarning(
                            "%s: got invalid OnContinueResult value (%f) from requests handler, rejecting the request",
                            Q_FUNC_INFO, result.DoubleValue());
                    }
                    resultConsumer(
                        JsState::castFrom(resultJsState),
                        optResultEnum.value_or(QOhosAbilityOnContinueResult::REJECT));
                });
        });
}

void JsStateImpl::setDestroyFromSystemAllowed(QNapi::Object qAbility, bool destroyAllowed)
{
    auto abilityPeer = qAbilityInstancesManager().tryGetQAbilityPeerByInstance(qAbility);
    if (!abilityPeer) {
        qOhosPrintfWarning(
            "%s: no QAbilityPeer for the given qAbility, flag not set to %s",
            Q_FUNC_INFO, mapBoolToTrueFalseStr(destroyAllowed));
        return;
    }
    abilityPeer->destroyAllowedFlag()->store(destroyAllowed);
}

void JsStateImpl::startAppProcess(
    const std::string &processId, QNapi::Object requestWant,
    QNapi::Object optStartOptions, std::function<void(QOhosJsState &)> continueFunc)
{
    auto optBaseQAbility = defaultQAbility();
    if (optBaseQAbility) {
        m_appFunctions->startAppProcess(
            optBaseQAbility.value(), processId, requestWant, optStartOptions, std::move(continueFunc));
    } else {
        invokeTask(std::move(continueFunc));
    }
}

std::shared_ptr<void> JsStateImpl::registerNewWantConsumer(
    QOhosConsumer<QOhosJsState &, QNapi::Object, QNapi::Object, QNapi::Object> wantConsumer)
{
    const auto consumerId = m_nextNewWantConsumerId++;
    m_newWantConsumers.emplace(consumerId, moveToSharedPtr(std::move(wantConsumer)));
    return makeDestroyNotifier(
        [this, consumerId]() {
            m_newWantConsumers.erase(consumerId);
        });
}

std::shared_ptr<void> JsStateImpl::registerUiAbilityDelegate(QNapi::Object delegate)
{
    const auto delegateId = m_nextUiAbilityDelegateId++;
    m_uiAbilityDelegates.emplace(delegateId, QNapi::Reference<>::makePersistentFrom(delegate));
    return makeDestroyNotifier(
        [this, delegateId]() {
            m_uiAbilityDelegates.erase(delegateId);
        });
}

void JsStateImpl::startNoUiChildProcess(const std::string &libraryName, const std::vector<std::string> &args)
{
    m_appFunctions->startNoUiChildProcess(*this, libraryName, args);
}

QtRunMode JsStateImpl::qtRunMode()
{
    return m_qtRunMode;
}

QAbilityInstancesManager &JsStateImpl::qAbilityInstancesManager()
{
    return QAbilityInstancesManager::instance(*this);
}

void *JsStateImpl::getAttachedObjectWithLazyCreate(
    const std::type_info &objectTypeInfo, QOhosSupplier<std::shared_ptr<void>> objectFactory)
{
    auto objectIter = m_attachedObjects.find(objectTypeInfo);
    if (objectIter == m_attachedObjects.end())
        std::tie(objectIter, std::ignore) = m_attachedObjects.emplace(objectTypeInfo, objectFactory());

    return objectIter->second.get();
}

std::tuple<QNapi::Object, std::string> JsStateImpl::extractModuleFromEvalExpr(const std::string &expr)
{
    using namespace std::string_literals;

    std::size_t longestModulePrefixSize = 0;
    auto considerAsLongestPrefix = [&](const std::string &name) {
        if (name.size() > longestModulePrefixSize
            && expr.compare(0, name.size(), name) == 0
            && (expr.size() == name.size() || expr[name.size()] == '.')) {
            longestModulePrefixSize = name.size();
        }
    };
    for (const auto &moduleFactoryEntry : m_jsModulesFactories)
        considerAsLongestPrefix(moduleFactoryEntry.first);

    if (longestModulePrefixSize == 0) {
        throw QNapi::makeLoggedException(
            env(), "global expression doesn't start with known module path: '"s + expr + "'");
    }

    return std::make_tuple(
        getModule(expr.substr(0, longestModulePrefixSize)),
        longestModulePrefixSize < expr.size()
            ? expr.substr(longestModulePrefixSize + 1)
            : "");
}

QNapi::Number JsStateImpl::mapOhosEnumToJs(
    int enumValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)())
{
    using namespace std::string_literals;

    const auto &enumeratorsValues = getOhosEnumEnumerators(enumTypeInfo, ohosEnumInfoFactory);

    auto valueIter = std::find_if(
        enumeratorsValues.begin(), enumeratorsValues.end(),
        [&](const auto &valuePair) {
            return valuePair.first == enumValue;
        });

    if (valueIter == enumeratorsValues.end()) {
        throw QNapi::makeLoggedException(
            env(),
            "Illegal C++ value of enumerator for enum '"s + enumTypeInfo.name() + "': "
            + std::to_string(enumValue));
    }

    return QNapi::Number::New(env(), valueIter->second);
}

std::optional<int> JsStateImpl::tryMapOhosEnumFromJs(
    QNapi::Number enumJsValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)())
{
    double enumDoubleJsValue = enumJsValue.DoubleValue();

    const auto &enumeratorsValues = getOhosEnumEnumerators(enumTypeInfo, ohosEnumInfoFactory);

    auto valueIter = std::find_if(
        enumeratorsValues.begin(), enumeratorsValues.end(),
        [&](const auto &valuePair) {
            return valuePair.second == enumDoubleJsValue;
        });

    return valueIter != enumeratorsValues.end()
        ? std::optional(valueIter->first)
        : std::nullopt;
}

int JsStateImpl::mapOhosEnumFromJs(
    QNapi::Number enumJsValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)())
{
    using namespace std::string_literals;

    auto optEnumValue = tryMapOhosEnumFromJs(enumJsValue, enumTypeInfo, ohosEnumInfoFactory);
    if (!optEnumValue.has_value()) {
        throw QNapi::makeLoggedException(
            enumJsValue.Env(),
            "Illegal Napi value of enumerator for enum '"s + enumTypeInfo.name() + "': "
            + std::to_string(enumJsValue.DoubleValue()));
    }
    return optEnumValue.value();
}

QNapi::Symbol JsStateImpl::getJsSymbolForType(const std::type_info &typeInfo)
{
    using namespace std::string_literals;

    auto jsSymbolRefIter = m_jsSymbolsRefs.find(typeInfo);
    if (jsSymbolRefIter == m_jsSymbolsRefs.end()) {
        std::tie(jsSymbolRefIter, std::ignore) = m_jsSymbolsRefs.emplace(
            typeInfo,
            QNapi::Reference<>::makePersistentFrom(
                QNapi::Symbol::New(env(), "_io_qt_"s + typeInfo.name())));
    }
    return jsSymbolRefIter->second.Value();
}

const std::vector<std::pair<int, double>> &JsStateImpl::getOhosEnumEnumerators(
    const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)())
{
    auto enumeratorsIter = m_ohosEnumsEnumerators.find(enumTypeInfo);
    if (enumeratorsIter == m_ohosEnumsEnumerators.end()) {
        std::tie(enumeratorsIter, std::ignore) = m_ohosEnumsEnumerators.emplace(
            enumTypeInfo, resolveOhosEnumEnumerators(ohosEnumInfoFactory()));
    }

    return enumeratorsIter->second;
}

std::vector<std::pair<int, double>> JsStateImpl::resolveOhosEnumEnumerators(const OhosEnumInfo &enumInfo)
{
    using namespace std::string_literals;

    QNapi::Object enumObj;
    try {
        enumObj = eval<QNapi::Object>(enumInfo.fullTypeName);
    } catch (const Napi::Error &e) {
        throw QNapi::makeLoggedException(
            env(), "JS enum '"s + enumInfo.fullTypeName + "' not found: "s + e.what());
    }

    std::vector<std::pair<int, double>> enumeratorsValues;

    for (const auto &enumNamePair : enumInfo.enumeratorsNames) {
        const auto *enumeratorName = enumNamePair.second;

        QNapi::Value jsEnumeratorValue = QNapi::getPropOrUndefined(enumObj, enumeratorName);

        if (!jsEnumeratorValue.IsNumber()) {
            throw QNapi::makeLoggedException(
                env(), "JS enum '"s + enumInfo.fullTypeName + "' doesn't contain: "s + enumeratorName);
        }

        enumeratorsValues.emplace_back(
            enumNamePair.first, QNapi::checkedCast<QNapi::Number>(jsEnumeratorValue).DoubleValue());
    }

    return enumeratorsValues;
}

JsStateImpl &getJsStateImpl()
{
    static JsStateImpl jsStateImpl;
    return jsStateImpl;
}

QOhosMtBlockingCallsGateway<JsState> &getQtJsBlockingCallsGateway()
{
    static auto qtJsBlockingCallsGateway = QOhosMtBlockingCallsGateway<JsState>::makeInstance(
        invokeInQtThread,
        [](std::function<void(JsState &)> task) {
            getJsStateImpl().invokeTask(std::move(task));
        });
    return *qtJsBlockingCallsGateway;
}

struct QueuedJsCall
{
    std::function<void(QOhosJsState &, QOhosTaskPromise<>)> task;
    std::string callerContextName;
};

struct JsCallSequence
{
    JsCallSequence(
        std::deque<QueuedJsCall> queuedJsCalls, std::optional<QueuedJsCall> optFinalJsCall,
        QOhosTaskPromise<> donePromise);

    std::deque<QueuedJsCall> remainingCalls;
    std::optional<QueuedJsCall> optFinalJsCall;
    bool currentCallRunning = false;
    bool currentCallSettled = false;
    QOhosTaskPromise<> donePromise;
};

JsCallSequence::JsCallSequence(
    std::deque<QueuedJsCall> queuedJsCalls, std::optional<QueuedJsCall> optFinalJsCall,
    QOhosTaskPromise<> donePromise)
    : remainingCalls(std::move(queuedJsCalls))
    , optFinalJsCall(std::move(optFinalJsCall))
    , donePromise(std::move(donePromise))
{
}

void runJsCallSequence(QOhosJsState &jsState, const std::shared_ptr<JsCallSequence> &jsCallSequence)
{
    const char *funcInfo = Q_FUNC_INFO;

    while (!jsCallSequence->remainingCalls.empty()) {
        auto queuedJsCall = std::move(jsCallSequence->remainingCalls.front());
        jsCallSequence->remainingCalls.pop_front();
        jsCallSequence->currentCallRunning = true;
        jsCallSequence->currentCallSettled = false;

        auto callPromise = QOhosTaskPromise<>(
            [&jsState, jsCallSequence, funcInfo, callerContextName = queuedJsCall.callerContextName]() {
                if (!isJsThread()) {
                    qOhosReportFatalErrorAndAbort(
                        "%s: batched task settled its promise outside the JS thread: %s",
                        funcInfo, callerContextName.c_str());
                }
                jsCallSequence->currentCallSettled = true;
                if (!jsCallSequence->currentCallRunning)
                    runJsCallSequence(jsState, jsCallSequence);
            },
            [funcInfo, callerContextName = queuedJsCall.callerContextName]() {
                if (std::uncaught_exceptions() == 0) {
                    qOhosReportFatalErrorAndAbort(
                        "%s: promise destroyed without notifying the caller: %s",
                        funcInfo, callerContextName.c_str());
                }
            },
            queuedJsCall.callerContextName);

        try {
            queuedJsCall.task(jsState, std::move(callPromise));
        } catch (const std::exception &e) {
            qOhosReportFatalErrorAndAbort(
                "%s: exception from batched task invoked in JS thread (caller: \"%s\"): %s",
                funcInfo, queuedJsCall.callerContextName.c_str(), e.what());
        } catch (...) {
            qOhosReportFatalErrorAndAbort(
                "%s: exception from batched task invoked in JS thread (caller: \"%s\"): <unknown exception>",
                funcInfo, queuedJsCall.callerContextName.c_str());
        }

        jsCallSequence->currentCallRunning = false;
        if (!jsCallSequence->currentCallSettled)
            return;
    }

    if (!jsCallSequence->optFinalJsCall) {
        jsCallSequence->donePromise();
        return;
    }

    auto finalJsCall = std::move(*jsCallSequence->optFinalJsCall);
    try {
        finalJsCall.task(jsState, std::move(jsCallSequence->donePromise));
    } catch (const std::exception &e) {
        qOhosReportFatalErrorAndAbort(
            "%s: exception from task invoked in JS thread (caller: \"%s\"): %s",
            funcInfo, finalJsCall.callerContextName.c_str(), e.what());
    } catch (...) {
        qOhosReportFatalErrorAndAbort(
            "%s: exception from task invoked in JS thread (caller: \"%s\"): <unknown exception>",
            funcInfo, finalJsCall.callerContextName.c_str());
    }
}

std::function<void(QOhosJsState &, QOhosTaskPromise<>)> makeJsCallSequenceTask(
    std::deque<QueuedJsCall> queuedJsCalls, std::optional<QueuedJsCall> optFinalJsCall)
{
    return [queuedJsCalls = std::move(queuedJsCalls), optFinalJsCall = std::move(optFinalJsCall)](
               QOhosJsState &jsState, QOhosTaskPromise<> donePromise) mutable {
        runJsCallSequence(
            jsState,
            std::make_shared<JsCallSequence>(
                std::move(queuedJsCalls), std::move(optFinalJsCall), std::move(donePromise)));
    };
}

class JsThreadOpsImpl : public ::QOhosJsThreadOps
{
public:
    QOhosJsState &jsState() override;

    void invoke(std::function<void(QOhosJsState &)> &&task) override;
    void invokeAndWaitForContinue(
        std::function<void(QOhosJsState &, QOhosTaskPromise<>)> &&task,
        std::string callerContextName) override;
    void runAndWait(
        const std::function<void(QOhosJsState &)> &task,
        std::string callerContextName) override;

    void startBatch(const char *batchContextName);
    void finishBatch(const char *batchContextName);
    void queueCall(std::function<void(QOhosJsState &, QOhosTaskPromise<>)> task, std::string callerContextName);

private:
    void invokeAndWaitForContinueUntraced(
        std::function<void(QOhosJsState &, QOhosTaskPromise<>)> &&task,
        std::string callerContextName);
    void runQueuedCalls();
    std::function<void(QOhosJsState &, QOhosTaskPromise<>)> takeQueuedCallsFollowedBy(
        std::function<void(QOhosJsState &, QOhosTaskPromise<>)> task, std::string callerContextName);

    std::deque<QueuedJsCall> m_qtThreadQueuedCalls;
    JsCallBatchTrace m_batchTrace;
};

QOhosJsState &JsThreadOpsImpl::jsState()
{
    return getJsStateImpl();
}

void JsThreadOpsImpl::invoke(std::function<void(QOhosJsState &)> &&task)
{
    if (getQtState().isQtThread()) {
        m_batchTrace.recordRunForcedBy(m_qtThreadQueuedCalls.size(), "invoke()");
        runQueuedCalls();
    }

    getJsStateImpl().invokeTask(
        [sharedTask = takeFunctionToSharedPtr(task)](JsState &jsState) {
            const auto takenTask = std::exchange(*sharedTask, nullptr);
            takenTask(jsState);
        });
}

void JsThreadOpsImpl::invokeAndWaitForContinue(
    std::function<void(QOhosJsState &, QOhosTaskPromise<>)> &&task,
    std::string callerContextName)
{
    if (getQtState().isQtThread())
        m_batchTrace.recordBlockingCall(m_qtThreadQueuedCalls.size(), callerContextName);

    invokeAndWaitForContinueUntraced(std::move(task), std::move(callerContextName));
}

void JsThreadOpsImpl::invokeAndWaitForContinueUntraced(
    std::function<void(QOhosJsState &, QOhosTaskPromise<>)> &&task,
    std::string callerContextName)
{
    const auto funcInfo = Q_FUNC_INFO;

    struct JsTaskCompletionState
    {
        std::mutex mutex;
        bool outerTaskPromiseSettled = false;
        std::optional<std::string> jsThreadExceptionMsg;
    };
    auto completionState = std::make_shared<JsTaskCompletionState>();

    auto catchingTaskWrapper =
        [task = takeQueuedCallsFollowedBy(std::move(task), callerContextName), completionState,
            callerContextName, funcInfo](
            JsState &jsState, QOhosTaskPromise<> outerTaskPromise) {
            auto sharedOuterTaskPromise =
                std::make_shared<QOhosTaskPromise<>>(std::move(outerTaskPromise));

            auto settleOuterTaskPromise =
                [completionState, sharedOuterTaskPromise](std::optional<std::string> exceptionMsg) {
                    {
                        std::lock_guard<std::mutex> lock(completionState->mutex);
                        if (completionState->outerTaskPromiseSettled)
                            return;
                        completionState->outerTaskPromiseSettled = true;
                        completionState->jsThreadExceptionMsg = std::move(exceptionMsg);
                    }
                    (*sharedOuterTaskPromise)();
                };

            auto innerTaskPromise = QOhosTaskPromise<>(
                [settleOuterTaskPromise]() {
                    settleOuterTaskPromise({});
                },
                [funcInfo, callerContextName]() {
                    if (std::uncaught_exceptions() == 0) {
                        qOhosReportFatalErrorAndAbort(
                            "%s: promise destroyed without notifying the caller: %s",
                            funcInfo, callerContextName.c_str());
                    }
                },
                callerContextName);

            try {
                task(jsState, std::move(innerTaskPromise));
            } catch (const std::exception &e) {
                settleOuterTaskPromise(std::optional<std::string>(e.what()));
            } catch (...) {
                settleOuterTaskPromise(std::optional<std::string>("<unknown exception>"));
            }
        };

    if (getQtState().isQtThread()) {
        getQtJsBlockingCallsGateway().runInSlaveThreadAndWaitForContinue(
            std::move(catchingTaskWrapper), callerContextName);
    } else {
        auto taskReadyPromise = std::make_shared<std::promise<void>>();
        auto taskReadyFuture = taskReadyPromise->get_future();

        auto sharedTaskPromise = std::make_shared<QOhosTaskPromise<>>(
            [taskReadyPromise]() {
                taskReadyPromise->set_value();
            },
            [funcInfo, callerContextName]() {
                qOhosReportFatalErrorAndAbort(
                    "%s: promise destroyed without notifying the caller: %s",
                    funcInfo, callerContextName.c_str());
            },
            callerContextName);

        getJsStateImpl().invokeTask(
            [catchingTaskWrapper = std::move(catchingTaskWrapper), sharedTaskPromise](JsState &jsState) {
                catchingTaskWrapper(jsState, std::move(*sharedTaskPromise));
            });
        taskReadyFuture.wait();
    }

    if (completionState->jsThreadExceptionMsg) {
        qOhosReportFatalErrorAndAbort(
            "%s: exception from task invoked in JS thread (caller: \"%s\"): %s",
            Q_FUNC_INFO, callerContextName.c_str(),
            completionState->jsThreadExceptionMsg.value().c_str());
    }
}

void JsThreadOpsImpl::runAndWait(
    const std::function<void(QOhosJsState &)> &task,
    std::string callerContextName)
{
    if (getJsStateImpl().isJsThread()) {
        task(getJsStateImpl());
    } else {
        invokeAndWaitForContinue(
            [&](auto &jsState, auto taskPromise) {
                task(jsState);
                taskPromise();
            },
            std::move(callerContextName));
    }
}

void JsThreadOpsImpl::startBatch(const char *batchContextName)
{
    m_batchTrace.recordBatchStart(batchContextName);
}

void JsThreadOpsImpl::finishBatch(const char *batchContextName)
{
    m_batchTrace.recordBatchEnd(batchContextName, m_qtThreadQueuedCalls.size());
    runQueuedCalls();
}

void JsThreadOpsImpl::queueCall(
    std::function<void(QOhosJsState &, QOhosTaskPromise<>)> task, std::string callerContextName)
{
    m_batchTrace.recordQueuedCall(callerContextName);

    m_qtThreadQueuedCalls.push_back(
        QueuedJsCall{
            .task = std::move(task),
            .callerContextName = std::move(callerContextName),
        });
}

void JsThreadOpsImpl::runQueuedCalls()
{
    if (m_qtThreadQueuedCalls.empty())
        return;

    invokeAndWaitForContinueUntraced(
        makeJsCallSequenceTask(std::exchange(m_qtThreadQueuedCalls, {}), std::nullopt), Q_FUNC_INFO);
}

std::function<void(QOhosJsState &, QOhosTaskPromise<>)> JsThreadOpsImpl::takeQueuedCallsFollowedBy(
    std::function<void(QOhosJsState &, QOhosTaskPromise<>)> task, std::string callerContextName)
{
    if (!getQtState().isQtThread() || m_qtThreadQueuedCalls.empty())
        return task;

    return makeJsCallSequenceTask(
        std::exchange(m_qtThreadQueuedCalls, {}),
        QueuedJsCall{
            .task = std::move(task),
            .callerContextName = std::move(callerContextName),
        });
}

JsThreadOpsImpl &getJsThreadOpsImpl()
{
    static JsThreadOpsImpl jsThreadOpsImpl;
    return jsThreadOpsImpl;
}

}

JsCallBatch::JsCallBatch(const char *staticContextName)
    : JsCallBatch(getQtState().isQtThread() ? Mode::Deferring : Mode::Immediate, staticContextName)
{
    if (!getQtState().isQtThread()) {
        qOhosPrintfDebug("%s: batch created outside the Qt thread runs each call right away: %s", Q_FUNC_INFO, staticContextName);
        return;
    }

    getJsThreadOpsImpl().startBatch(staticContextName);

    const char *funcInfo = Q_FUNC_INFO;
    if (auto *eventDispatcher = QAbstractEventDispatcher::instance()) {
        m_eventProcessingDetectionConnection = QObject::connect(
            eventDispatcher, &QAbstractEventDispatcher::awake,
            [funcInfo, batchContextName = staticContextName, warned = false]() mutable {
                if (!warned) {
                    warned = true;
                    qCWarning(
                        QtForOhos, "%s: events processed inside batch %s, where application code must not run",
                        funcInfo, batchContextName);
                }
            });
    }
}

JsCallBatch::JsCallBatch(Mode mode, const char *staticContextName)
    : m_mode(mode)
    , m_staticContextName(staticContextName)
{
}

JsCallBatch::~JsCallBatch()
{
    if (m_mode == Mode::Deferring) {
        QObject::disconnect(m_eventProcessingDetectionConnection);
        getJsThreadOpsImpl().finishBatch(m_staticContextName);
    }
}

JsCallBatch &JsCallBatch::immediate()
{
    static JsCallBatch immediateBatch(Mode::Immediate, Q_FUNC_INFO);
    return immediateBatch;
}

void JsCallBatch::submit(
    std::function<void(QOhosJsState &, QOhosTaskPromise<>)> task, std::string callerContextName)
{
    if (m_mode == Mode::Immediate) {
        QOhosJsThreadGateway::invokeAndWaitForContinue(std::move(task), std::move(callerContextName));
        return;
    }

    if (!getQtState().isQtThread()) {
        qOhosReportFatalErrorAndAbort(
            "%s: call submitted to a batch outside the Qt thread: %s", Q_FUNC_INFO, callerContextName.c_str());
    }

    getJsThreadOpsImpl().queueCall(std::move(task), std::move(callerContextName));
}

void JsCallBatch::submit(std::function<void(QOhosJsState &)> task, std::string callerContextName)
{
    if (m_mode == Mode::Immediate) {
        QOhosJsThreadGateway::runAndWait(task, std::move(callerContextName));
        return;
    }

    submit(
        [task = std::move(task)](QOhosJsState &jsState, QOhosTaskPromise<> taskPromise) {
            task(jsState);
            taskPromise();
        },
        std::move(callerContextName));
}

AppFunctions::~AppFunctions() = default;

QAbilityPeer::~QAbilityPeer() = default;

QAbilityPeer::QAbilityPeer() = default;

QUiAbilityPeer::QUiAbilityPeer() = default;

QUiAbilityPeer::~QUiAbilityPeer() = default;

const nullptr_t QUiAbilityPeer::typeIdObject = {};

void *QUiAbilityPeer::tryCastWithTypeIdObject(const void *matchTypeIdObject)
{
    return matchTypeIdObject == &typeIdObject ? this : nullptr;
}

std::shared_ptr<QUiAbilityPeer> QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(
    std::shared_ptr<QAbilityPeer> qAbilityPeer)
{
    if (!qAbilityPeer)
        return nullptr;

    auto qUiAbilityPeer = reinterpret_cast<QUiAbilityPeer *>(
        qAbilityPeer->tryCastWithTypeIdObject(&typeIdObject));

    return qUiAbilityPeer != nullptr
        ? qUiAbilityPeer->shared_from_this()
        : nullptr;
}

JsState::~JsState() = default;

JsState::JsState() = default;

JsState &JsState::castFrom(QOhosJsState &jsState)
{
    if (&jsState != &getJsStateImpl())
        qOhosReportFatalErrorAndAbort("%s: jsState is not the plugin's JsState", Q_FUNC_INFO);

    return static_cast<JsState &>(jsState);
}

JsState &CallbackInfo::jsState() const
{
    return getJsStateImpl();
}

void JsWindowsTracker::tagWindowAsClosing(QNapi::Object jsWindow, const char *logContext)
{
    qOhosPrintfDebug("Tagging JS Window as closing (from: %s)", logContext);
    Napi::HandleScope funcScope{jsWindow.Env()};
    jsWindow.Set(
        getJsWindowsTrackerIsClosingPropSymbol(getJsStateImpl()),
        QNapi::Boolean::New(jsWindow.Env(), true));
}

bool JsWindowsTracker::isWindowClosing(QNapi::Object jsWindow)
{
    Napi::HandleScope funcScope{jsWindow.Env()};
    auto isClosingPropSymbol = getJsWindowsTrackerIsClosingPropSymbol(getJsStateImpl());
    auto isClosingValue = jsWindow.Has(isClosingPropSymbol)
        ? jsWindow.Get(isClosingPropSymbol)
        : QNapi::Value();
    return isClosingValue.IsBoolean() && QNapi::checkedCast<QNapi::Boolean>(isClosingValue).Value();
}

std::vector<std::string> predefinedJsModuleNames()
{
    return getJsStateImpl().predefinedJsModuleNames();
}

void initJsThreadState(
    napi_env env, std::map<std::string, QNapi::Reference<QNapi::Function>> &&jsModulesFactories,
    std::shared_ptr<AppFunctions> appFunctions, QtRunMode qtRunMode)
{
    QOhosJsThreadOps::registerInstance(&getJsThreadOpsImpl());

    getJsStateImpl().initInJsThread(env, std::move(jsModulesFactories), appFunctions, qtRunMode);

    qOhosEnableMemoizingJsThreadFetchers(getJsStateImpl());
}

bool isJsThread()
{
    return getJsStateImpl().isJsThread();
}

void dispatchNewWant(QNapi::Object qAbility, QNapi::Object want, QNapi::Object launchParam)
{
    getJsStateImpl().dispatchNewWantInJsThread(qAbility, want, launchParam);
}

void callUiAbilityDelegates(const char *methodName, const std::vector<QNapi::Value> &args)
{
    getJsStateImpl().callUiAbilityDelegates(methodName, args);
}

std::optional<QNapi::Value> tryCallFirstUiAbilityDelegate(
    const char *methodName, const std::vector<QNapi::Value> &args, QNapi::Value resultOnThrow)
{
    return getJsStateImpl().tryCallFirstUiAbilityDelegate(methodName, args, resultOnThrow);
}

bool tryInvokeInQtThreadAndTryWaitForContinue(
    std::function<void(std::function<void()>)> &&task,
    std::chrono::nanoseconds timeout)
{
    if (getJsStateImpl().isJsThread()) {
        auto result = getQtJsBlockingCallsGateway().tryInvokeInMasterThreadAndTryWaitForContinue(std::move(task), timeout);
        return result == QOhosMtBlockingCallsGateway<JsState>::MasterThreadTaskResult::Finished;
    } else {
        auto taskReadyPromise = std::make_shared<std::promise<void>>();
        auto taskReadyFuture = taskReadyPromise->get_future();

        auto continueFunc = [taskReadyPromise]() {
            taskReadyPromise->set_value();
        };

        invokeInQtThread(
            [task = std::move(task), continueFunc = std::move(continueFunc)]() mutable {
                task(std::move(continueFunc));
            });
        auto waitResult = taskReadyFuture.wait_for(timeout);

        return waitResult == std::future_status::ready;
    }
}

}

QT_END_NAMESPACE
