// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosplugincore.h>

#include "qohosmtblockingcallsgateway_p.h"
#include <QtCore/private/qohoslogger_p.h>
#include <algorithm>
#include <deque>
#include <future>
#include <mutex>
#include <napi.h>
#include <napi/native_api.h>
#include <optional>
#include <tuple>
#include <typeindex>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhos {

namespace {

constexpr const char *forceLoadJsModulesEnvVariableName = "IO__QT__OHOS__FORCE_LOAD_JS_MODULES";

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
    "@ohos.notificationManager",
    "@ohos.pasteboard",
    "@ohos.settings",
    "@ohos.web.webview",
    "@ohos.wifiManager",
    "@ohos.window",
};

class DummyQAbilityPeer : public QAbilityPeer
{
public:
    DummyQAbilityPeer() = default;

    std::string instanceId() override;
    QNapi::Object uiContext() override;
    QNapi::Object qAbility() override;
    QNapi::Object launchWant() override;
    QObjectThreadSafeRef qWindowRef() override;
    QOhosOptional<QNapi::Promise> qWindowDestroyPromise() override;
    std::shared_ptr<std::atomic_bool> destroyAllowedFlag() override;
    bool isTerminating() override;

    void setQWindow(Napi::Env env, QObjectThreadSafeRef) override;

    void *tryCastWithTypeIdObject(const void *matchTypeIdObject) override;
};

std::string DummyQAbilityPeer::instanceId()
{
    return {};
}

QNapi::Object DummyQAbilityPeer::uiContext()
{
    return QNapi::Object();
}

QNapi::Object DummyQAbilityPeer::qAbility()
{
    return QNapi::Object();
}

QNapi::Object DummyQAbilityPeer::launchWant()
{
    return QNapi::Object();
}

QObjectThreadSafeRef DummyQAbilityPeer::qWindowRef()
{
    return {};
}

QOhosOptional<QNapi::Promise> DummyQAbilityPeer::qWindowDestroyPromise()
{
    return {};
}

std::shared_ptr<std::atomic_bool> DummyQAbilityPeer::destroyAllowedFlag()
{
    return std::make_shared<std::atomic_bool>(false);
}

bool DummyQAbilityPeer::isTerminating()
{
    return false;
}

void DummyQAbilityPeer::setQWindow(Napi::Env, QObjectThreadSafeRef)
{
}

void *DummyQAbilityPeer::tryCastWithTypeIdObject(const void *)
{
    return nullptr;
}

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

QNapi::Object loadJsModuleViaNapiOrFail(napi_env env, const std::string &moduleName)
{
    qOhosPrintfDebug("%s: loading napi module '%s' via napi_load_module()", Q_FUNC_INFO, moduleName.c_str());

    napi_value result = nullptr;
    auto status = napi_load_module(env, moduleName.c_str(), &result);
    if (status != napi_ok) {
        qOhosReportFatalErrorAndAbort(
            "%s: napi_load_module failed for module '%s' (status=%d)",
            Q_FUNC_INFO, moduleName.c_str(), static_cast<int>(status));
    }

    return QNapi::Object(env, result);
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

    void addQAbilityPeerInJsThread(std::shared_ptr<QAbilityPeer> qAbilityPeer);
    void removeMatchingQAbilityPeerInJsThread(QNapi::Object qAbility);

    void dispatchNewWantInJsThread(QNapi::Object want, QNapi::Object launchParam);

    void invokeTask(std::function<void(JsState &)> &&task);

    napi_env env() override;
    QNapi::Object getModule(const std::string &moduleName) override;

    QNapi::Object appLaunchWant() override;
    QOhosOptional<QNapi::Object> optAppLaunchParam() override;

    QNapi::Object defaultWindowStageOrEmpty() override;
    QNapi::Object defaultUiContextOrEmpty() override;

    std::shared_ptr<QAbilityPeer> defaultQAbilityPeer() override;
    std::shared_ptr<QAbilityPeer> tryGetQAbilityPeerByInstanceId(const std::string &instanceId) override;
    std::shared_ptr<QAbilityPeer> tryGetQAbilityPeerByInstance(QNapi::Object qAbility) override;
    std::shared_ptr<QAbilityPeer> tryGetQAbilityPeerByQWindow(QObjectThreadSafeRef qwindow) override;

    void visitEachQAbilityPeer(const std::function<void(std::shared_ptr<QAbilityPeer>)> &visitor) override;

    void startNewQAbilityInstance(
        std::shared_ptr<QAbilityPeer> baseQAbilityPeer, QObjectThreadSafeRef qwindow,
        QNapi::Object optStartOptions,
        std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc) override;

    void startAppProcess(
        const std::string &processId, QNapi::Object requestWant,
        QNapi::Object optStartOptions) override;

    void startAppProcess(
        const std::string &processId, QNapi::Object requestWant,
        QNapi::Object optStartOptions, std::function<void(JsState &)> continueFunc) override;

    void addNewWantConsumer(QOhosConsumer<JsState &, QNapi::Object, QNapi::Object> wantConsumer) override;

    void startNoUiChildProcess(const std::string &libraryName, const std::vector<std::string> &args) override;

    QtRunMode qtRunMode() override;

private:
    template<typename PeerMatchFunc>
    std::shared_ptr<QAbilityPeer> tryFindMatchingQAbilityPeer(PeerMatchFunc &&matchFunc);

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

    void forceLoadAllJsModulesIfRequested();

    pthread_t m_jsThread = {};
    napi_env m_env = nullptr;
    QNapi::Reference<QNapi::Object> m_appLaunchWant;
    QNapi::Reference<QNapi::Object> m_optAppLaunchParam;
    std::shared_ptr<QAbilityPeer> m_defaultQAbilityPeer;
    std::map<std::string, std::shared_ptr<QAbilityPeer>> m_qAbilityPeers;
    std::map<std::string, std::function<QNapi::Object(JsState &)>> m_jsModulesFactories;
    std::shared_ptr<AppFunctions> m_appFunctions;
    PreQueuingJsTasksExecutor m_tasksExecutor;
    std::vector<QOhosConsumer<JsState &, QNapi::Object, QNapi::Object>> m_newWantConsumers;
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
    m_defaultQAbilityPeer = std::make_shared<DummyQAbilityPeer>();
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

void JsStateImpl::addQAbilityPeerInJsThread(std::shared_ptr<QAbilityPeer> qAbilityPeer)
{
    forceLoadAllJsModulesIfRequested();

    if (m_appLaunchWant.IsEmpty())
        m_appLaunchWant = Napi::Persistent(qAbilityPeer->launchWant());

    auto optQUiAbilityPeer = QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(qAbilityPeer);
    if (m_optAppLaunchParam.IsEmpty() && optQUiAbilityPeer) {
        m_optAppLaunchParam = QNapi::Reference<>::makePersistentFrom(optQUiAbilityPeer->launchParam());
        qOhosPrintfInfo(
            "JsStateImpl: appLaunchParam set from Ability with instanceId='%s'",
            qAbilityPeer->instanceId().c_str());
    }

    if (m_qAbilityPeers.empty())
        m_defaultQAbilityPeer = qAbilityPeer;

    m_qAbilityPeers.emplace(qAbilityPeer->instanceId(), qAbilityPeer);

    qOhosPrintfDebug(
        "JsStateImpl: added QAbilityPeer, instanceId: %s, qwindow: %s",
        qAbilityPeer->instanceId().c_str(), qAbilityPeer->qWindowRef().refName().c_str());
}

void JsStateImpl::removeMatchingQAbilityPeerInJsThread(QNapi::Object qAbility)
{
    Napi::HandleScope funcScope{qAbility.Env()};

    auto foundPeerIter = std::find_if(
        m_qAbilityPeers.begin(), m_qAbilityPeers.end(),
        [&](const auto &entry) {
            return entry.second->qAbility() == qAbility;
        });

    if (foundPeerIter != m_qAbilityPeers.end()) {
        auto removedPeer = foundPeerIter->second;
        m_qAbilityPeers.erase(foundPeerIter);
        if (m_defaultQAbilityPeer == removedPeer) {
            m_defaultQAbilityPeer =
                !m_qAbilityPeers.empty()
                    ? m_qAbilityPeers.begin()->second
                    : std::make_shared<DummyQAbilityPeer>();
        }
        qOhosPrintfDebug(
            "JsStateImpl: removed QAbilityPeer, instanceId: %s, qwindow: %s",
            removedPeer->instanceId().c_str(), removedPeer->qWindowRef().refName().c_str());
    }
}

void JsStateImpl::dispatchNewWantInJsThread(QNapi::Object want, QNapi::Object launchParam)
{
    const auto consumersCount = m_newWantConsumers.size();
    for (std::size_t i = 0; i < consumersCount; ++i)
        m_newWantConsumers[i](*this, want, launchParam);
}

void JsStateImpl::invokeTask(std::function<void(JsState &)> &&task)
{
    m_tasksExecutor.invokeTask(std::move(task));
}

napi_env JsStateImpl::env()
{
    return m_env;
}

void JsStateImpl::forceLoadAllJsModulesIfRequested()
{
    if (qEnvironmentVariableIntValue(forceLoadJsModulesEnvVariableName) == 0)
        return;

    qOhosPrintfDebug("%s: forcibly loading all JS modules", Q_FUNC_INFO);

    for (const auto &moduleFactoryEntry : m_jsModulesFactories)
        getModule(moduleFactoryEntry.first);
}

QNapi::Object JsStateImpl::getModule(const std::string &moduleName)
{
    using namespace std::string_literals;

    auto moduleFactoryIter = m_jsModulesFactories.find(moduleName);
    if (moduleFactoryIter == m_jsModulesFactories.end())
        throw QNapi::makeLoggedException(m_env, "JS module not found: "s + moduleName);

    return moduleFactoryIter->second(*this);
}

QNapi::Object JsStateImpl::appLaunchWant()
{
    return m_appLaunchWant.Value();
}

QOhosOptional<QNapi::Object> JsStateImpl::optAppLaunchParam()
{
    return !m_optAppLaunchParam.IsEmpty()
        ? makeQOhosOptional(m_optAppLaunchParam.Value())
        : makeEmptyQOhosOptional();
}

QNapi::Object JsStateImpl::defaultWindowStageOrEmpty()
{
    auto qUiAbilityPeer = QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(m_defaultQAbilityPeer);
    if (!qUiAbilityPeer)
        return QNapi::Object();

    return qUiAbilityPeer->windowStage();
}

QNapi::Object JsStateImpl::defaultUiContextOrEmpty()
{
    return m_defaultQAbilityPeer->uiContext();
}

std::shared_ptr<QAbilityPeer> JsStateImpl::defaultQAbilityPeer()
{
    return m_defaultQAbilityPeer;
}

std::shared_ptr<QAbilityPeer> JsStateImpl::tryGetQAbilityPeerByInstanceId(const std::string &instanceId)
{
    auto foundIter = m_qAbilityPeers.find(instanceId);
    return foundIter != m_qAbilityPeers.end() ? foundIter->second : nullptr;
}

std::shared_ptr<QAbilityPeer> JsStateImpl::tryGetQAbilityPeerByInstance(QNapi::Object qAbility)
{
    Napi::HandleScope findPeerScope(qAbility.Env());
    return tryFindMatchingQAbilityPeer(
        [&](const auto &peer) {
            return peer->qAbility() == qAbility;
        });
}

std::shared_ptr<QAbilityPeer> JsStateImpl::tryGetQAbilityPeerByQWindow(QObjectThreadSafeRef qwindow)
{
    return tryFindMatchingQAbilityPeer(
        [&](const auto &peer) {
            return peer->qWindowRef() == qwindow;
        });
}

void JsStateImpl::visitEachQAbilityPeer(const std::function<void(std::shared_ptr<QAbilityPeer>)> &visitor)
{
    for (const auto &qAbilityPeerEntry : m_qAbilityPeers)
        visitor(qAbilityPeerEntry.second);
}

void JsStateImpl::startNewQAbilityInstance(
    std::shared_ptr<QAbilityPeer> baseQAbilityPeer, QObjectThreadSafeRef qwindow,
    QNapi::Object optStartOptions,
    std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc)
{
    auto baseQAbility = baseQAbilityPeer->qAbility();
    if (!baseQAbility.IsEmpty()) {
        m_appFunctions->startQAbilityInstance(
            baseQAbility, qwindow, optStartOptions, std::move(startupNotifyFunc));
    }
}

void JsStateImpl::startAppProcess(
    const std::string &processId, QNapi::Object requestWant, QNapi::Object optStartOptions)
{
    auto baseQAbility = defaultQAbilityPeer()->qAbility();
    if (!baseQAbility.IsEmpty())
        m_appFunctions->startAppProcess(baseQAbility, processId, requestWant, optStartOptions);
}

void JsStateImpl::startAppProcess(
    const std::string &processId, QNapi::Object requestWant,
    QNapi::Object optStartOptions, std::function<void(JsState &)> continueFunc)
{
    auto baseQAbility = defaultQAbilityPeer()->qAbility();
    if (!baseQAbility.IsEmpty()) {
        m_appFunctions->startAppProcess(
            baseQAbility, processId, requestWant, optStartOptions, std::move(continueFunc));
    } else {
        invokeTask(std::move(continueFunc));
    }
}

void JsStateImpl::addNewWantConsumer(QOhosConsumer<JsState &, QNapi::Object, QNapi::Object> wantConsumer)
{
    m_newWantConsumers.push_back(std::move(wantConsumer));
}

void JsStateImpl::startNoUiChildProcess(const std::string &libraryName, const std::vector<std::string> &args)
{
    m_appFunctions->startNoUiChildProcess(*this, libraryName, args);
}

QtRunMode JsStateImpl::qtRunMode()
{
    return m_qtRunMode;
}

template<typename PeerMatchFunc>
std::shared_ptr<QAbilityPeer> JsStateImpl::tryFindMatchingQAbilityPeer(PeerMatchFunc &&matchFunc)
{
    auto foundIter = std::find_if(
        m_qAbilityPeers.begin(), m_qAbilityPeers.end(),
        [&](const auto &entry) {
            return matchFunc(entry.second);
        });
    return foundIter != m_qAbilityPeers.end() ? foundIter->second : nullptr;
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
        invokeInQtThread, invokeInJsThread);
    return *qtJsBlockingCallsGateway;
}

class JsThreadOpsImpl : public ::QOhosJsThreadOps
{
public:
    QOhosJsState &jsState() override;

    void invoke(std::function<void(QOhosJsState &)> task) override;
    void invokeAndWaitForContinue(
        std::function<void(QOhosJsState &, std::function<void()>)> &&task) override;
    void runAndWait(const std::function<void(QOhosJsState &)> &task) override;
};

QOhosJsState &JsThreadOpsImpl::jsState()
{
    return getJsStateImpl();
}

void JsThreadOpsImpl::invoke(std::function<void(QOhosJsState &)> task)
{
    QtOhos::invokeInJsThread(std::move(task));
}

void JsThreadOpsImpl::invokeAndWaitForContinue(
    std::function<void(QOhosJsState &, std::function<void()>)> &&task)
{
    QtOhos::invokeInJsThreadAndWaitForContinue(std::move(task));
}

void JsThreadOpsImpl::runAndWait(const std::function<void(QOhosJsState &)> &task)
{
    QtOhos::runInJsThreadAndWait(task);
}

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

QAbilityEngine::~QAbilityEngine() = default;

QAbilityEngine::QAbilityEngine() = default;

JsState::~JsState() = default;

JsState::JsState() = default;

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

void initJsThreadState(
    napi_env env, std::map<std::string, QNapi::Reference<QNapi::Function>> &&jsModulesFactories,
    std::shared_ptr<AppFunctions> appFunctions, QtRunMode qtRunMode)
{
    static JsThreadOpsImpl jsThreadOpsImpl;
    QOhosJsThreadOps::registerInstance(&jsThreadOpsImpl);

    getJsStateImpl().initInJsThread(env, std::move(jsModulesFactories), appFunctions, qtRunMode);
}

void addJsQAbilityPeer(std::shared_ptr<QAbilityPeer> qAbilityPeer)
{
    getJsStateImpl().addQAbilityPeerInJsThread(qAbilityPeer);
}

void removeMatchingJsQAbilityPeer(QNapi::Object qAbility)
{
    getJsStateImpl().removeMatchingQAbilityPeerInJsThread(qAbility);
}

void dispatchNewWant(QNapi::Object want, QNapi::Object launchParam)
{
    getJsStateImpl().dispatchNewWantInJsThread(want, launchParam);
}

void invokeInJsThread(std::function<void(JsState &)> task)
{
    getJsStateImpl().invokeTask(std::move(task));
}

void invokeInJsThreadAndWaitForContinue(std::function<void(JsState &, std::function<void()>)> &&task)
{
    if (getQtState().isQtThread()) {
        getQtJsBlockingCallsGateway().runInSlaveThreadAndWaitForContinue(std::move(task));
    } else {
        auto taskReadyPromise = std::make_shared<std::promise<void>>();
        auto taskReadyFuture = taskReadyPromise->get_future();

        auto continueFunc = [taskReadyPromise]() {
            taskReadyPromise->set_value();
        };

        invokeInJsThread(
            [task = std::move(task), continueFunc = std::move(continueFunc)](JsState &jsState) mutable {
                task(jsState, std::move(continueFunc));
            });
        taskReadyFuture.wait();
    }
}

void runInJsThreadAndWait(const std::function<void(JsState &)> &task)
{
    if (getJsStateImpl().isJsThread()) {
        task(getJsStateImpl());
    } else {
        invokeInJsThreadAndWaitForContinue(
            [&](auto &jsState, auto continueFunc) {
                task(jsState);
                continueFunc();
            });
    }
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
