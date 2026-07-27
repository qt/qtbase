// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosjsmain.h"

#include <qplugin.h>
#include <dlfcn.h>
#include <node_api.h>
#include <napi/native_api.h>
#include <hilog/log.h>
#include <pthread.h>
#include <qos/qos.h>
#include <QtCore/qdebug.h>
#include <QtCore/qglobal.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qmap.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qdir.h>
#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/qvariant.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohosappcontext_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohospermissionshelper_p.h>
#include <QtCore/private/qcoreapplication_p.h>
#include "qohoscloseeventcontext_p.h"
#include "qohosplatformfontdatabase_p.h"
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <limits>
#include <qohossinglethreadexecutor.h>
#include <qpa/qwindowsysteminterface.h>
#include <map>
#include <qohosapppermissions_p.h>
#include <qohosdeviceinfo_p.h>
#include <qohosenums.h>
#include <qohospermissionshelperimpl.h>
#include <qohosplugincore.h>
#include <signal.h>
#include <string>
#include <sys/resource.h>
#include <type_traits>
#include <cstdlib>
#include <optional>
#include <unordered_map>

#include "private/qohosplatformtheme_p.h"
#include "qarkui/qxcomponentregistry.h"
#include "qohoseventdispatcher.h"
#include "qohosinputcontext.h"
#include "qohosjsutils.h"
#include "qohosnouichildprocess.h"
#include "qohosplatformintegration.h"
#include "qohosplatformwindow.h"
#include "qohosqabilityinstancesmanager.h"
#include "qohosutils.h"
#include "qohoswatchdog.h"
#include "qohosplatformdialoghelper.h"
#include "qohossystemlocale.h"
#include "render/qxcomponent.h"

QT_BEGIN_NAMESPACE

using namespace std::chrono_literals;

static QList<QByteArray> s_applicationParams;

QOhosConsumer<std::vector<std::string>> s_qtAppThreadMainFuncLauncher;
struct {
    std::optional<std::uint64_t> lastRequestedInJsThread;
    std::optional<std::uint64_t> activeInQtThread;
} s_hotStartIteration;
std::function<void()> s_qtAppThreadIdleStateWaitFunc;
extern "C" typedef int (*Main)(int, char **); //use the standard main method to start the application

static bool s_autoStartedAbilityInstanceWaitingForQtWindow = true;

static std::string s_appSharedLibsDirPath;
static std::string s_appSharedLibName;
static std::unique_ptr<std::vector<std::string>> s_appArgs;
static std::vector<QNapi::Reference<QNapi::Object>> foregroundAbilities;
static int s_appExitCode = 0;
static std::string s_exitCodeFilePath;

static bool s_hotStartEnabled = false;

namespace QtOhos {

namespace {

constexpr const char *enableHotStartEnvVariableName = "IO__QT__OHOS__ENABLE_HOT_START";

constexpr const char *qtMainThreadStackSizeEnvVariableName = "IO__QT__OHOS__QT_MAIN_THREAD_STACK_SIZE";

constexpr std::size_t defaultQtThreadStackSize = 8 * 1024 * 1024;

constexpr auto minSupportedOhosSdkApiVersion = 23;
constexpr auto defaultColorMode = enums::ohos::app::ability::ConfigurationConstant::ColorMode::COLOR_MODE_NOT_SET;

std::atomic<bool> experimentalEnableGlBackinStore{false};
std::atomic<bool> debugUseBasicStyleAndTheme{false};
std::atomic<bool> debugDrawQtRasterBackingStoreFlushedRegion{false};
std::atomic<bool> vsyncOnSoftwareBackingStoreEnabled{true};
std::atomic<bool> enableNativeNodeApiKeyEvents{true};
std::atomic<bool> enableNativeNodeApiMouseEvents{true};
QtRunMode currentQtRunMode = QtRunMode::Normal;

const auto callerPidWantArgName = "ohos.aafwk.param.callerPid";
const auto qtAppProcessIdWantArgName = "io.qt.private.appProcessId";

const char *mapQtRunModeToString(QtRunMode qtRunMode)
{
    switch (qtRunMode) {
    case QtRunMode::Normal:
        return "Normal";
    case QtRunMode::NoUiChildProcess:
        return "NoUiChildProcess";
    }

    qOhosReportFatalErrorAndAbort(
        "%s: got unknown QtRunMode value: %d",
        Q_FUNC_INFO, static_cast<int>(qtRunMode));
}

struct QtAppStartConfig
{
    std::string appLibraryPath;
    bool watchdogEnabled = true;
};

class AppContextDirs
{
public:
    static AppContextDirs mapFromNapiObject(QNapi::Object appContextObj);
    static AppContextDirs mapFromQOhosAppContextProperties(const QMap<QOhosAppContext::Type, QString> &props);

    QMap<QOhosAppContext::Type, QString> mapToQOhosAppContextProperties() const;
    QJsonObject mapToQJsonObject() const;

    std::string bundleCodeDir;
    std::string cacheDir;
    std::string filesDir;
    std::string preferencesDir;
    std::string tempDir;
    std::string databaseDir;
    std::string distributedFilesDir;
    std::string resourceDir;

private:
    static const std::pair<const char *, std::string AppContextDirs::*> propsNames[];
    static const std::pair<QOhosAppContext::Type, std::string AppContextDirs::*> qOhosAppContextPropsMap[];
};

const std::pair<const char *, std::string AppContextDirs::*> AppContextDirs::propsNames[] = {
    {"bundleCodeDir", &AppContextDirs::bundleCodeDir},
    {"cacheDir", &AppContextDirs::cacheDir},
    {"filesDir", &AppContextDirs::filesDir},
    {"preferencesDir", &AppContextDirs::preferencesDir},
    {"tempDir", &AppContextDirs::tempDir},
    {"databaseDir", &AppContextDirs::databaseDir},
    {"distributedFilesDir", &AppContextDirs::distributedFilesDir},
    {"resourceDir", &AppContextDirs::resourceDir},
};

const std::pair<QOhosAppContext::Type, std::string AppContextDirs::*> AppContextDirs::qOhosAppContextPropsMap[] = {
    {QOhosAppContext::Type::bundleCodeDir, &AppContextDirs::bundleCodeDir},
    {QOhosAppContext::Type::cacheDir, &AppContextDirs::cacheDir},
    {QOhosAppContext::Type::filesDir, &AppContextDirs::filesDir},
    {QOhosAppContext::Type::preferencesDir, &AppContextDirs::preferencesDir},
    {QOhosAppContext::Type::tempDir, &AppContextDirs::tempDir},
    {QOhosAppContext::Type::databaseDir, &AppContextDirs::databaseDir},
    {QOhosAppContext::Type::distributedFilesDir, &AppContextDirs::distributedFilesDir},
    {QOhosAppContext::Type::resourceDir, &AppContextDirs::resourceDir},
};

AppContextDirs AppContextDirs::mapFromNapiObject(QNapi::Object appContextObj)
{
    AppContextDirs appContextDirs;
    for (const auto &propEntry : propsNames)
        appContextDirs.*propEntry.second = appContextObj.get<QNapi::String>(propEntry.first);

    return appContextDirs;
}

AppContextDirs AppContextDirs::mapFromQOhosAppContextProperties(
    const QMap<QOhosAppContext::Type, QString> &props)
{
    AppContextDirs appContextDirs;
    for (const auto &propEntry : qOhosAppContextPropsMap)
        appContextDirs.*propEntry.second = props[propEntry.first].toStdString();
    return appContextDirs;
}

QMap<QOhosAppContext::Type, QString> AppContextDirs::mapToQOhosAppContextProperties() const
{
    QMap<QOhosAppContext::Type, QString> qOhosAppContextProps;
    for (const auto &propEntry : qOhosAppContextPropsMap)
        qOhosAppContextProps[propEntry.first] = QString::fromStdString(this->*propEntry.second);
    return qOhosAppContextProps;
}

QJsonObject AppContextDirs::mapToQJsonObject() const
{
    QJsonObject json;
    for (const auto &propEntry : propsNames)
        json[QString::fromUtf8(propEntry.first)] = QString::fromStdString(this->*propEntry.second);
    return json;
}

std::vector<std::string> splitString(const std::string &inputStr, char separator)
{
    constexpr auto separatorSize = 1;

    std::vector<std::string> result;
    std::size_t currentPos = 0;
    while (currentPos < inputStr.size()) {
        auto separatorPos = inputStr.find(separator, currentPos);
        result.push_back(inputStr.substr(currentPos, separatorPos));
        currentPos = separatorPos != std::string::npos
            ? separatorPos + separatorSize
            : inputStr.size();
    }

    return result;
}

std::pair<QOhosConsumer<bool>, std::function<void()>> makeConditionFlagMTAccessors(
    std::string conditionName)
{
    struct Context
    {
        std::string conditionName;
        std::mutex conditionMutex;
        std::condition_variable conditionCv;
        bool condition = false;
    };

    auto context = std::make_shared<Context>();
    context->conditionName = std::move(conditionName);

    return {
        [context](bool condition) {
            qOhosPrintfDebug(
                "%s: setting condition '%s' to %s",
                Q_FUNC_INFO, context->conditionName.c_str(), mapBoolToTrueFalseStr(condition));
            {
                std::lock_guard<std::mutex> conditionLock(context->conditionMutex);
                if (condition != context->condition) {
                    context->condition = condition;
                    context->conditionCv.notify_all();
                }
            }
        },
        [context]() {
            qOhosPrintfDebug("%s: waiting for condition '%s'", Q_FUNC_INFO, context->conditionName.c_str());
            {
                std::unique_lock<std::mutex> conditionLock(context->conditionMutex);
                context->conditionCv.wait(
                    conditionLock,
                    [&]() {
                        return context->condition;
                    });
            }
            qOhosPrintfDebug("%s: condition '%s' met", Q_FUNC_INFO, context->conditionName.c_str());
        },
    };
}

template<typename FuncResult, typename ...FuncArgs, typename FuncFactory>
auto makeLazyInitFunc(FuncFactory funcFactory) -> std::function<FuncResult(FuncArgs...)>
{
    using Func = std::function<FuncResult(FuncArgs...)>;
    return [func = Func(), factory = QOhosSupplier<Func>(std::move(funcFactory))](FuncArgs ...args) mutable {
        if (!func)
            func = std::exchange(factory, nullptr)();
        return func(std::forward<FuncArgs>(args)...);
    };
}

std::function<int(std::vector<std::string>)> openLibraryWithMainFunctionOrFail(const std::string &libraryPath)
{
    void *mainLibraryHnd = dlopen(libraryPath.c_str(), RTLD_LAZY);
    if (Q_UNLIKELY(!mainLibraryHnd)) {
        qOhosReportFatalErrorAndAbort(
            "%s: dlopen() failed to open library '%s': %s",
            Q_FUNC_INFO, libraryPath.c_str(), dlerror());
    }

    auto *mainFunc = reinterpret_cast<Main>(dlsym(mainLibraryHnd, "main"));
    if (Q_UNLIKELY(!mainFunc)) {
        qOhosReportFatalErrorAndAbort(
            "%s: dlsym() failed to find 'main' symbol in library '%s': %s",
            Q_FUNC_INFO, libraryPath.c_str(), dlerror());
    }

    qOhosPrintfDebug("%s: opened library '%s' with main function", Q_FUNC_INFO, libraryPath.c_str());

    return [libraryPath, mainFunc](std::vector<std::string> mainArgs) {
        auto mainArgsPointers = std::vector<char *>();
        for (auto &arg : mainArgs)
            mainArgsPointers.push_back(&arg[0]);
        mainArgsPointers.push_back(nullptr);

        int argc = mainArgs.size();
        char **argv = &mainArgsPointers[0];

        qOhosPrintfDebug(
            "%s: calling 'main' function in library '%s' (argc=%d)",
            Q_FUNC_INFO, libraryPath.c_str(), argc);

        for (int i = 0; i < argc; ++i)
            qOhosPrintfDebug("%s: 'main' function argv[%d]='%s'", Q_FUNC_INFO, i, argv[i]);

        int mainResult = mainFunc(argc, argv);

        qOhosPrintfDebug(
            "%s: 'main' function in library '%s' returned %d",
            Q_FUNC_INFO, libraryPath.c_str(), mainResult);

        return mainResult;
    };
}

class QUiAbilityEngine : public QAbilityEngine
{
public:
    QUiAbilityEngine();
    ~QUiAbilityEngine();

    QAbilityInfo readAbilityInfo(const QNapi::Object &ability) const override;
};

QUiAbilityEngine::QUiAbilityEngine() = default;

QUiAbilityEngine::~QUiAbilityEngine() = default;

QAbilityInfo QUiAbilityEngine::readAbilityInfo(const QNapi::Object &ability) const
{
    auto abilityInfo = ability.eval<QNapi::Object>("context.abilityInfo");

    return {
        .name = abilityInfo.get<QNapi::String>("name"),
        .bundleName = abilityInfo.get<QNapi::String>("bundleName"),
        .moduleName = abilityInfo.get<QNapi::String>("moduleName"),
    };
}

void redirectStandardDescriptorsToFile(const std::string &redirectedStdoutPath)
{
    int openResult = qt_safe_open(redirectedStdoutPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    auto openErrno = errno;
    if (openResult < 0) {
        // Non-fatal: the test still needs to write its exit code.
        qOhosPrintfWarning("%s: error opening file '%s' for redirected stdout: %s",
                           Q_FUNC_INFO, redirectedStdoutPath.c_str(), std::strerror(openErrno));
        return;
    }

    ::fflush(stdout);

    if (qt_safe_dup2(openResult, STDOUT_FILENO) < 0) {
        auto dup2Errno = errno;
        qOhosPrintfWarning("%s: dup2() failed on redirecting stdout to '%s': %s",
                           Q_FUNC_INFO, redirectedStdoutPath.c_str(), std::strerror(dup2Errno));
    }

    qt_safe_close(openResult);
}

QOhosConsumer<std::vector<std::string>> makeAppMainFuncLauncher(
    const QtAppStartConfig &appStartConfig, QOhosConsumer<int> funcExitHandler)
{
    auto appLibraryMainFunc = makeLazyInitFunc<int, std::vector<std::string>>(
        [appLibraryPath = appStartConfig.appLibraryPath]() {
            return openLibraryWithMainFunctionOrFail(appLibraryPath);
        });

    return [appLibraryMainFunc = std::move(appLibraryMainFunc), funcExitHandler = std::move(funcExitHandler), appStartConfig](std::vector<std::string> appArgs) {
        auto __dbg = make_QCScopedDebugJS("startApplicationMainFunction");
        std::vector<std::string> mainArgs;
        mainArgs.push_back(appStartConfig.appLibraryPath);
        mainArgs.insert(mainArgs.end(), appArgs.begin(), appArgs.end());

        auto qtWatchdog =
            appStartConfig.watchdogEnabled
                ? QtOhosWatchdog::makeWatchdog()
                : std::shared_ptr<void>();

        int exitCode = appLibraryMainFunc(std::move(mainArgs));

        funcExitHandler(exitCode);
    };
}

std::optional<std::vector<std::string>> tryMapJsonArrayToStrings(const std::string &inputJson)
{
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(inputJson));
    if (doc.isNull()) {
        qOhosPrintfWarning("%s: input is not valid JSON string", Q_FUNC_INFO);
        return std::nullopt;
    }
    if (!doc.isArray()) {
        qOhosPrintfWarning("%s: input JSON does not contain an array", Q_FUNC_INFO);
        return std::nullopt;
    }

    const auto inputArray = doc.array();

    std::vector<std::string> result;
    for (const auto &elem : inputArray) {
        if (!elem.isString()) {
            qOhosPrintfWarning("%s: input array's element is not a string", Q_FUNC_INFO);
            return std::nullopt;
        }
        result.push_back(elem.toString().toStdString());
    }

    return result;
}

template<typename T>
std::enable_if_t<std::is_base_of<QNapi::Value, T>::value, T>
getWantParamOrEmptyIfNotPresent(QNapi::Object want, const std::string &paramName)
{
    return QNapi::getOptionalPropOrEmpty<T>(
        QNapi::getOptionalPropOrEmpty<QNapi::Object>(want, "parameters"),
        paramName, "parameters of Want");
}

std::vector<std::string> getQtAppArgsFromWant(QNapi::Object want)
{
    using namespace std::string_literals;

    const auto *qtUseUriAsArgPropName = "io.qt.useUriAsArg";
    const auto *qtAppArgsPropName = "io.qt.appArgs";
    const auto *qtAppArgsJsonPropName = "io.qt.appArgsJson";

    Napi::HandleScope getArgsScope(want.Env());

    std::vector<std::string> result;

    auto optQtUseUriAsArg = getWantParamOrEmptyIfNotPresent<QNapi::Boolean>(want, qtUseUriAsArgPropName);
    if (optQtUseUriAsArg.IsEmpty() || optQtUseUriAsArg.Value()) {
        auto uri = QNapi::getPropOrUndefined(want, "uri");
        result.push_back(
            uri.IsString()
                ? QNapi::checkedCast<QNapi::String>(uri)
                : std::string());
    }

    auto optQtAppArgs = getWantParamOrEmptyIfNotPresent<QNapi::Array>(want, qtAppArgsPropName);
    auto optQtAppArgsJson = getWantParamOrEmptyIfNotPresent<QNapi::String>(want, qtAppArgsJsonPropName);

    if (!optQtAppArgs.IsEmpty()) {
        if (!QNapi::arrayElementTypesMatch<QNapi::String>(optQtAppArgs)) {
            throw QNapi::makeLoggedException(
                want.Env(), "Want parameter '"s + qtAppArgsPropName + "' is not an array of strings"s);
        }

        auto qtAppArgsStrings = QNapi::getArrayElements<std::vector<std::string>, QNapi::String>(optQtAppArgs);
        result.insert(result.end(), qtAppArgsStrings.begin(), qtAppArgsStrings.end());
    } else if (!optQtAppArgsJson.IsEmpty()) {
        auto optQtAppArgsJsonStrings = tryMapJsonArrayToStrings(optQtAppArgsJson);
        if (!optQtAppArgsJsonStrings) {
            throw QNapi::makeLoggedException(
                want.Env(), "Want parameter '"s + qtAppArgsJsonPropName + "' is invalid"s);
        }
        result.insert(result.end(), optQtAppArgsJsonStrings->begin(), optQtAppArgsJsonStrings->end());
    }

    return result;
}

void requestAppPermissionsInBackground(JsState &jsState, const std::vector<std::string> &permissionsNames)
{
    for (const auto &permissionName : permissionsNames) {
        qOhosPrintfInfo(
            "Qt: automatically requesting application permission: '%s'",
            permissionName.c_str());

        QOhosAppPermissions::requestAppPermissionFromUser(
            jsState, permissionName,
            [permissionName](JsState &, bool permissionGranted) {
                if (permissionGranted) {
                    qOhosPrintfInfo(
                        "Qt: automatically requested application permission granted: '%s'",
                        permissionName.c_str());
                } else {
                    qOhosPrintfWarning(
                        "Qt: automatically requested application permission rejected: '%s'",
                        permissionName.c_str());
                }
            });
    }
}

struct AppProcessLaunchOptions
{
    QNapi::Boolean useDefaultUiAbilityInstanceInQt;
    QNapi::String appSharedLibNameOverride;
    QNapi::Boolean experimentalGlBackingStore;
    QNapi::Boolean debugDrawQtRasterBackingStoreFlushedRegion;
    QNapi::Boolean debugUseBasicStyleAndTheme;
    QNapi::Boolean enableVsyncOnSoftwareBackingStore;
    QNapi::Boolean watchdogEnabled;
    QNapi::String redirectStdoutToFile;
    QNapi::String exitCodeFile;
    QNapi::String autoRequestPermissions;
    QNapi::Boolean enableNativeNodeApiKeyEvents;
    QNapi::Boolean enableNativeNodeApiMouseEvents;
};

AppProcessLaunchOptions getProcessLaunchOptionsFromWant(QNapi::Object launchWant)
{
    auto assignWantParamIfPresent = [&](auto &outputValue, const char *paramName) {
        using Param = std::remove_reference_t<decltype(outputValue)>;
        outputValue = getWantParamOrEmptyIfNotPresent<Param>(launchWant, paramName);
    };

    AppProcessLaunchOptions launchOpts;

    assignWantParamIfPresent(
        launchOpts.useDefaultUiAbilityInstanceInQt,
        "io.qt.useDefaultUiAbilityInstanceInQt");
    assignWantParamIfPresent(
        launchOpts.appSharedLibNameOverride,
        "io.qt.appSharedLibNameOverride");
    assignWantParamIfPresent(
        launchOpts.experimentalGlBackingStore,
        "io.qt.experimental.enableGlBackingStore");
    assignWantParamIfPresent(
        launchOpts.debugDrawQtRasterBackingStoreFlushedRegion,
        "io.qt.debug.drawQtRasterBackingStoreFlushedRegion");
    assignWantParamIfPresent(
        launchOpts.debugUseBasicStyleAndTheme,
        "io.qt.debug.useBasicStyleAndTheme");
    assignWantParamIfPresent(
        launchOpts.enableVsyncOnSoftwareBackingStore,
        "io.qt.experimental.enableVsyncOnSoftwareBackingStore");
    assignWantParamIfPresent(
        launchOpts.watchdogEnabled,
        "io.qt.watchdogEnabled");
    assignWantParamIfPresent(
        launchOpts.redirectStdoutToFile,
        "io.qt.debug.redirectedStdoutPath");
    assignWantParamIfPresent(
        launchOpts.exitCodeFile,
        "io.qt.debug.exitCodePath");
    assignWantParamIfPresent(
        launchOpts.autoRequestPermissions,
        "io.qt.debug.autoRequestPermissions");
    assignWantParamIfPresent(
        launchOpts.enableNativeNodeApiKeyEvents,
        "io.qt.experimental.enableNativeNodeApiKeyEvents");
    assignWantParamIfPresent(
        launchOpts.enableNativeNodeApiMouseEvents,
        "io.qt.experimental.enableNativeNodeApiMouseEvents");

    return launchOpts;
}

void setGlobalFlagsFromAppProcessLaunchOptions(const AppProcessLaunchOptions &launchOpts)
{
    if (!launchOpts.useDefaultUiAbilityInstanceInQt.IsEmpty())
        s_autoStartedAbilityInstanceWaitingForQtWindow = launchOpts.useDefaultUiAbilityInstanceInQt.Value();

    if (!launchOpts.experimentalGlBackingStore.IsEmpty())
        experimentalEnableGlBackinStore = launchOpts.experimentalGlBackingStore.Value();

    if (!launchOpts.debugUseBasicStyleAndTheme.IsEmpty())
        debugUseBasicStyleAndTheme = launchOpts.debugUseBasicStyleAndTheme.Value();

    if (!launchOpts.enableVsyncOnSoftwareBackingStore.IsEmpty())
        vsyncOnSoftwareBackingStoreEnabled = launchOpts.enableVsyncOnSoftwareBackingStore.Value();

    debugDrawQtRasterBackingStoreFlushedRegion =
        launchOpts.debugDrawQtRasterBackingStoreFlushedRegion.IsEmpty()
            ? false
            : launchOpts.debugDrawQtRasterBackingStoreFlushedRegion.Value();

    if (!launchOpts.enableNativeNodeApiKeyEvents.IsEmpty())
        enableNativeNodeApiKeyEvents = launchOpts.enableNativeNodeApiKeyEvents.Value();

    if (!launchOpts.enableNativeNodeApiMouseEvents.IsEmpty())
        enableNativeNodeApiMouseEvents = launchOpts.enableNativeNodeApiMouseEvents.Value();
}

void terminateAllAbilityInstances(JsState &jsState, const char *logContext)
{
    jsState.visitEachQAbilityPeer(
        [&](auto qAbilityPeer) {
            qOhosPrintfInfo(
                "Qt: terminating QAbility with instanceId='%s'",
                qAbilityPeer->instanceId().c_str());
            auto optQUiAbilityPeer = QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(qAbilityPeer);
            if (optQUiAbilityPeer)
                JsWindowsTracker::tagWindowAsClosing(optQUiAbilityPeer->window(), logContext);
            qAbilityPeer->qAbility().eval("context.terminateSelf()");
        });
}

std::optional<std::size_t> tryGetMaxStackSizeHardLimit()
{
    struct ::rlimit limit;
    if (::getrlimit(RLIMIT_STACK, &limit) != 0) {
        auto getrlimitErrno = errno;
        qOhosPrintfWarning(
            "%s: error reading stack size hard limit (assuming no limit): %s",
            Q_FUNC_INFO, std::strerror(getrlimitErrno));
        return {};
    }

    return limit.rlim_max != RLIM_INFINITY
        ? std::optional<std::size_t>(limit.rlim_max)
        : std::nullopt;
}

std::optional<std::size_t> tryGetQtThreadStackSizeFromEnv()
{
    int stackSizeFromEnv = qEnvironmentVariableIntValue(qtMainThreadStackSizeEnvVariableName);
    return stackSizeFromEnv > 0
        ? std::optional(static_cast<std::size_t>(stackSizeFromEnv))
        : std::nullopt;
}

std::size_t getPreferredStackSizeForQtThread()
{
    constexpr std::size_t qtRequiredMinStackSize = 40960;
    std::size_t pthreadStackMin = PTHREAD_STACK_MIN;
    auto minStackSize = std::max({qtRequiredMinStackSize, pthreadStackMin});
    auto maxStackSize = tryGetMaxStackSizeHardLimit().value_or(std::numeric_limits<std::size_t>::max());

    auto optRequestedStackSize = tryGetQtThreadStackSizeFromEnv();

    if (optRequestedStackSize.has_value()) {
        auto requestedStackSize = optRequestedStackSize.value();
        if (requestedStackSize < minStackSize) {
            qOhosPrintfWarning(
                "%s: requested stack size (%zu) is below minimum (pthread min: %zu, Qt min: %zu), increasing",
                Q_FUNC_INFO, requestedStackSize, pthreadStackMin, qtRequiredMinStackSize);
        }
        if (requestedStackSize > maxStackSize) {
            qOhosPrintfWarning(
                "%s: requested stack size (%zu) is above maximum (hard limit: %zu), decreasing",
                Q_FUNC_INFO, requestedStackSize, maxStackSize);
        }
    }

    auto preferredStackSize = qBound(
        minStackSize, optRequestedStackSize.value_or(defaultQtThreadStackSize), maxStackSize);

    qOhosPrintfInfo("%s: preferred stack size for Qt Thread: %zu", Q_FUNC_INFO, preferredStackSize);

    return preferredStackSize;
}

QOhosConsumer<std::vector<std::string>> makeQtThreadWithMainFuncLauncher(
    QOhosConsumer<std::vector<std::string>> baseMainFuncLauncher)
{
    SingleThreadExecutorConfig qtThreadExecutorConfig = {
        .threadPreferredStackSize = getPreferredStackSizeForQtThread(),
    };
    auto qtThreadExecutor = makeSingleThreadExecutor(qtThreadExecutorConfig);

    struct InitContext
    {
        std::mutex initializedMutex;
        std::condition_variable initializedCv;
        bool initialized = false;
    };

    auto initContext = std::make_shared<InitContext>();

    qtThreadExecutor(
        [initContext] {
            pthread_setname_np(pthread_self(), "QtMainThread");
            //
            // Following call to QThread::currentThread() forces this thread to be
            // the main Qt/GUI thread. It sets the QCoreApplication::theMainThread field
            // if this call is the first one.
            //
            auto *currentThread = QThread::currentThread();
            QThread *mainThread = QCoreApplicationPrivate::theMainThread;
            if (mainThread != currentThread) {
                qOhosReportFatalErrorAndAbort(
                    "%s: mainThread (%p) != currentThread (%p). Qt API was likely used before Qt initialization. Aborting.",
                    Q_FUNC_INFO, mainThread, currentThread);
            }

            qt_setQOhosPermissionsHelper(getQOhosPermissionsHelperImpl());

            QtOhos::initQtThreadState();

            {
                std::lock_guard<std::mutex> initializedLock(initContext->initializedMutex);
                initContext->initialized = true;
                initContext->initializedCv.notify_one();
            }
        });

    {
        std::unique_lock<std::mutex> initializedLock(initContext->initializedMutex);
        initContext->initializedCv.wait(
            initializedLock,
            [&]() {
                return initContext->initialized;
            });
    }

    auto sharedBaseMainFuncLauncher = moveToSharedPtr(std::move(baseMainFuncLauncher));

    return [qtThreadExecutor = std::move(qtThreadExecutor), sharedBaseMainFuncLauncher](std::vector<std::string> appArgs) {
        qtThreadExecutor(
            [sharedBaseMainFuncLauncher, appArgs = std::move(appArgs)]() mutable {
                (*sharedBaseMainFuncLauncher)(std::move(appArgs));
            });
    };
}

std::shared_ptr<QAbilityInstancesManager> &getQAbilityInstancesManagerPtr()
{
    static std::shared_ptr<QAbilityInstancesManager> instancePtr;
    return instancePtr;
}

QAbilityInstancesManager &getQAbilityInstancesManager()
{
    return *getQAbilityInstancesManagerPtr();
}

void handleDefaultQAbilityInstanceStartup(JsState &jsState, std::shared_ptr<QAbilityPeer> qAbilityPeer)
{
    QNapi::Object launchWant = qAbilityPeer->launchWant();

    if (!s_qtAppThreadMainFuncLauncher) {
        auto qtAppThreadIdleSetFunc = std::make_shared<QOhosConsumer<bool>>();
        std::function<void()> qtAppThreadIdleStateWaitFunc;
        std::tie(*qtAppThreadIdleSetFunc, qtAppThreadIdleStateWaitFunc) = makeConditionFlagMTAccessors("Qt thread idle");

        auto launchOpts = getProcessLaunchOptionsFromWant(launchWant);

        setGlobalFlagsFromAppProcessLaunchOptions(launchOpts);

        if (!launchOpts.redirectStdoutToFile.IsEmpty())
            redirectStandardDescriptorsToFile(launchOpts.redirectStdoutToFile.Utf8Value());

        if (!launchOpts.exitCodeFile.IsEmpty())
            s_exitCodeFilePath = launchOpts.exitCodeFile.Utf8Value();

        if (!launchOpts.autoRequestPermissions.IsEmpty())
            requestAppPermissionsInBackground(jsState, splitString(launchOpts.autoRequestPermissions, ','));

        std::string appSharedLibName =
            !launchOpts.appSharedLibNameOverride.IsEmpty()
                ? launchOpts.appSharedLibNameOverride
                : s_appSharedLibName;

        auto appMainFuncLauncher = makeAppMainFuncLauncher(
            {
                .appLibraryPath = s_appSharedLibsDirPath + "/" + appSharedLibName,
                .watchdogEnabled = !launchOpts.watchdogEnabled.IsEmpty()
                    ? launchOpts.watchdogEnabled.Value()
                    : true,
            },
            [qtAppThreadIdleSetFunc](int exitCode) {
                if (s_hotStartEnabled)
                    s_autoStartedAbilityInstanceWaitingForQtWindow = true;
                else
                    s_appExitCode = exitCode;

                qOhosPrintfInfo("Qt: asynchronously terminating remaining QAbility instances, if any");
                QtOhos::invokeInJsThread(
                    [](QtOhos::JsState &jsState) {
                        if (s_hotStartEnabled)
                            getQAbilityInstancesManager().registerPendingAutoStartedInstance();

                        qOhosPrintfInfo(
                            "Qt: force-resolving pending QWindow destroy Promises before termination if needed");
                        jsState.visitEachQAbilityPeer(
                            [&](std::shared_ptr<QtOhos::QAbilityPeer> peer) {
                                peer->forceResolveQWindowDestroyPromiseIfPresent(
                                    Napi::Env(jsState.env()));
                            });

                        terminateAllAbilityInstances(jsState, "Qt main() exit");
                    });

                (*qtAppThreadIdleSetFunc)(true);
            });

        auto mainFuncLauncher = moveToSharedPtr(
            makeQtThreadWithMainFuncLauncher(std::move(appMainFuncLauncher)));
        s_qtAppThreadMainFuncLauncher = [mainFuncLauncher, qtAppThreadIdleSetFunc](std::vector<std::string> appArgs) {
            (*qtAppThreadIdleSetFunc)(false);
            s_hotStartIteration.activeInQtThread = s_hotStartIteration.activeInQtThread.value_or(0) + 1;
            (*mainFuncLauncher)(std::move(appArgs));
        };
        s_qtAppThreadIdleStateWaitFunc = std::move(qtAppThreadIdleStateWaitFunc);
    }

    s_hotStartIteration.lastRequestedInJsThread = s_hotStartIteration.lastRequestedInJsThread.value_or(0) + 1;
    s_qtAppThreadMainFuncLauncher(
        s_appArgs
            ? *s_appArgs
            : getQtAppArgsFromWant(launchWant));
}

std::map<std::string, QNapi::Reference<QNapi::Function>> makeJsModulesFactoriesMap(
    const QNapi::Object &jsModulesFactoriesObj)
{
    std::map<std::string, QNapi::Reference<QNapi::Function>> jsModulesFactoriesMap;
    for (const auto &prop : jsModulesFactoriesObj) {
        if (prop.first.IsString()) {
            auto propName = QNapi::checkedCast<QNapi::String>(prop.first);
            QNapi::Value propValue = prop.second;
            if (propValue.IsFunction()) {
                jsModulesFactoriesMap.emplace(
                    propName.Utf8Value(),
                    QNapi::Reference<QNapi::Function>::makePersistentFrom(
                        QNapi::checkedCast<QNapi::Function>(propValue)));
            }
        }
    }

    return jsModulesFactoriesMap;
}

class AppFunctionsImpl : public AppFunctions
{
public:
    void startQAbilityInstance(
        QNapi::Object baseQAbility, QObjectThreadSafeRef qwindow,
        QNapi::Object optStartOptions,
        std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc) override;

    void startAppProcess(
        QNapi::Object baseQAbility, const std::string &processId, QNapi::Object requestWant,
        QNapi::Object optStartOptions, std::function<void(JsState &)> continueFunc) override;

    void startNoUiChildProcess(JsState &jsState, const std::string &libraryName, const std::vector<std::string> &args) override;

    void tagWidgetOrWindowAsFloatWindow(QObject *widgetOrWindow, bool floatWindowEnabled) override;
};

void AppFunctionsImpl::startQAbilityInstance(
    QNapi::Object baseQAbility, QObjectThreadSafeRef qwindow,
    QNapi::Object optStartOptions,
    std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc)
{
    getQAbilityInstancesManager().startNewInstance(
        baseQAbility, qwindow, optStartOptions, std::move(startupNotifyFunc));
}

void AppFunctionsImpl::startAppProcess(
    QNapi::Object baseQAbility, const std::string &processId, QNapi::Object requestWant,
    QNapi::Object optStartOptions, std::function<void(JsState &)> continueFunc)
{
    auto __dbg = make_QCScopedDebugJS("AppFunctionsImpl::startAppProcess");

    static const char * const clonedWantPropsNames[] = {
        "uri",
        "type",
        "action",
        "flags",
        "entities",
    };

    auto env = baseQAbility.Env();

    auto qAbilityInfo = getQAbilityInstancesManager().abilityEngine()->readAbilityInfo(baseQAbility);

    auto startWantParams = QNapi::Object::New(env);
    auto requestWantParams = QNapi::getOptionalPropOrEmpty<QNapi::Object>(requestWant, "parameters");
    if (!requestWantParams.IsEmpty()) {
        for (const auto &requestWantParamEntry : requestWantParams) {
            startWantParams.Set(
                requestWantParamEntry.first, static_cast<QNapi::Value>(requestWantParamEntry.second));
        }
    }
    startWantParams.Set(qtAppProcessIdWantArgName, processId);

    auto startWant = QNapi::makeObject(
        env,
        {
            {"bundleName", qAbilityInfo.bundleName},
            {"moduleName", qAbilityInfo.moduleName},
            {"abilityName", qAbilityInfo.name},
            {"parameters", startWantParams},
        });

    for (const auto &propName : clonedWantPropsNames) {
        auto optProp = QNapi::getOptionalPropOrEmpty<QNapi::Value>(requestWant, propName);
        if (!optProp.IsEmpty())
            startWant.Set(propName, optProp);
    }

    std::vector<QNapi::ValueWrapper> startAbilityArgs = {startWant};
    if (!optStartOptions.IsEmpty())
        startAbilityArgs.push_back(optStartOptions);

    baseQAbility.evalToPromiseOrRejectOnThrow("context.startAbility(*)", startAbilityArgs)
    .onCatch(QtOhos::makeErrorLoggingJsCallback("startAbility()"))
    .onFinally(
        [continueFunc = std::move(continueFunc)](const CallbackInfo &cbInfo) {
            continueFunc(cbInfo.jsState());
        });
}

void AppFunctionsImpl::startNoUiChildProcess(
    JsState &jsState, const std::string &libraryName, const std::vector<std::string> &args)
{
    const auto *childProcessSrcEntry = "./ets/process/QChildProcess.ets";

    // FIXME:
    // We want to use childProcessManager.StartMode.APP_SPAWN_FORK here, but the "StartMode"
    // is defined as "const enum" in the TS code, which makes it a compile-time-only thing
    // (contrary to non-const enums, which are real objects, const enums don't exist at runtime).
    // We should consider adding a separate mechanism for handling const enums in the code
    // if we have more of them in the future.
    constexpr int startModeAppSpawnFork = 1;

    auto appContextDirs = AppContextDirs::mapFromQOhosAppContextProperties(QOhosAppContext::getAllProperties());

    QJsonArray argsArray;
    std::transform(args.begin(), args.end(), std::back_inserter(argsArray), QString::fromStdString);

    QJsonObject childSetupJson = {
        {QString::fromUtf8("appContext"), appContextDirs.mapToQJsonObject()},
        {QString::fromUtf8("appName"), QString::fromStdString(libraryName)},
        {QString::fromUtf8("appArgs"), argsArray},
    };

    jsState.eval(
        "@ohos.app.ability.childProcessManager.startChildProcess(*)",
        {
            childProcessSrcEntry,
            startModeAppSpawnFork,
            [childSetupJson](const CallbackInfo &cbInfo) {
                QNapi::Value error;
                QNapi::Value data;
                cbInfo.getLeadingArgs(Q_FUNC_INFO, error, data);

                int childPid = data.IsNumber()
                    ? QNapi::checkedCast<QNapi::Number>(data)
                    : -1;

                if (childPid > 0) {
                    qOhosPrintfDebug("%s: child started: %d", Q_FUNC_INFO, childPid);
                    sendChildProcessSetupData(childPid, childSetupJson);
                } else {
                    qOhosPrintfError("%s: child NOT started", Q_FUNC_INFO);
                }
            },
        });
}

void AppFunctionsImpl::tagWidgetOrWindowAsFloatWindow(QObject *widgetOrWindow, bool floatWindowEnabled)
{
    QOhosPlatformWindow::tagWindowOrWidgetAsFloatWindow(widgetOrWindow, floatWindowEnabled);
}

void handleAbilityOnForeground(const CallbackInfo &cbInfo)
{
    const auto qAbility = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

    if (foregroundAbilities.empty()) {
        QtOhos::invokeInQtThread([]() {
            const auto setQosRes = OH_QoS_SetThreadQoS(QoS_Level::QOS_USER_INTERACTIVE);
            if (setQosRes != 0) {
                qOhosWarning(QtForOhos)
                    << "Setting QoS level of Qt thread to USER_INTERACTIVE failed, error code:"
                    << setQosRes;
            }
            updateApplicationState(Qt::ApplicationActive);
        });
    }

    foregroundAbilities.push_back(Napi::Persistent(qAbility));
}

void handleAbilityOnBackground(const CallbackInfo &cbInfo)
{
    const auto qAbility = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

    const auto qAbilityRef = Napi::Persistent(qAbility);
    foregroundAbilities.erase(
        std::remove(foregroundAbilities.begin(), foregroundAbilities.end(), qAbilityRef),
        foregroundAbilities.end());

    if (foregroundAbilities.empty()) {
        QtOhos::invokeInQtThread([]() {
            const auto resetQosRes = OH_QoS_ResetThreadQoS();
            if (resetQosRes != 0) {
                qOhosWarning(QtForOhos)
                    << "Resetting QoS level of Qt thread failed, error code:"
                    << resetQosRes;
            }
            updateApplicationState(Qt::ApplicationHidden);
            updateApplicationState(Qt::ApplicationInactive);
        });
    }
}

QNapi::Value handleAbilityOnContinue(const CallbackInfo &cbInfo)
{
    QNapi::Object qAbility;
    QNapi::Object wantParamsObj;
    cbInfo.getLeadingArgs(Q_FUNC_INFO, qAbility, wantParamsObj);

    auto uiAbilityPeer = QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(
        cbInfo.jsState().tryGetQAbilityPeerByInstance(qAbility));
    if (!uiAbilityPeer) {
        qOhosPrintfWarning("%s: got unknown Ability, rejecting", Q_FUNC_INFO);
        return makeResolvedPromise(
            cbInfo.jsState().mapOhosEnumToJs(
                QOhosAbilityOnContinueResult::REJECT));
    }

    return adaptAsyncCallResultToJsPromise<QOhosAbilityOnContinueResult>(
        cbInfo.jsState(),
        [](JsState &jsState, auto result) {
            return jsState.mapOhosEnumToJs(result);
        },
        [&](JsState &jsState, auto resultConsumer) {
            getQAbilityInstancesManager().getAbilityPeerBackend(uiAbilityPeer)->handleOnContinueRequestFromSystem(
                jsState, wantParamsObj, std::move(resultConsumer));
        });
}

std::string targetLibraryDirectory() {
    #if defined(Q_PROCESSOR_ARM_64)
        return "/libs/arm64";
    #elif defined(Q_PROCESSOR_ARM_32)
        return "/libs/arm";
    #elif defined(Q_PROCESSOR_X86_64)
        return "/libs/x86_64";
    #else
        #error "Unknown system architecture, aborting!"
    #endif
}

void tryDetectBrokenWant(JsState &jsState, QNapi::Object want)
{
    Napi::HandleScope checkScope(want.Env());

    auto defaultQAbility = jsState.defaultQAbilityPeer()->qAbility();

    if (!defaultQAbility.IsEmpty()) {
        bool fromThisApp = getQAbilityInstancesManager().isWantFromThisApp(defaultQAbility, want);
        auto optCallerPid = getWantParamOrEmptyIfNotPresent<QNapi::Number>(want, callerPidWantArgName);
        if (fromThisApp && !optCallerPid.IsEmpty() && ::kill(optCallerPid.Int64Value(), 0) != 0) {
            qOhosPrintfError(
                "%s: got Want from non-existing app process (pid: %lld), which most likely means that we received"
                " broken Want (platform bug). That's fatal error for us!",
                Q_FUNC_INFO, static_cast<long long>(optCallerPid.Int64Value()));
            std::abort();
        }
    }
}

std::string readInitialBytesOfFile(const std::string &filePath, std::size_t maxReadSize)
{
    FILE *inputFile = std::fopen(filePath.c_str(), "rb");
    if (inputFile == nullptr) {
        auto fopenErrno = errno;
        qOhosReportFatalErrorAndAbort(
            "%s: can't open file '%s': %s",
            Q_FUNC_INFO, filePath.c_str(), std::strerror(fopenErrno));
    }

    std::unique_ptr<FILE, decltype(&std::fclose)> inputFileCloseGuard(inputFile, &std::fclose);

    auto readBuffer = std::string(maxReadSize, '\0');

    auto bytesRead = std::fread(&readBuffer[0], 1, maxReadSize, inputFile);
    if (std::ferror(inputFile)) {
        qOhosReportFatalErrorAndAbort(
            "%s: error reading '%s': (%zu bytes read)",
            Q_FUNC_INFO, filePath.c_str(), bytesRead);
    }

    readBuffer.resize(bytesRead);
    readBuffer.shrink_to_fit();

    return readBuffer;
}

std::string readCurrentProcessNameFromProcFs()
{
    const auto *procCmdlinePath = "/proc/self/cmdline";
    auto procCmdlineData = readInitialBytesOfFile(procCmdlinePath, 64 * 1024);

    auto processNameTerminatorPos = procCmdlineData.find('\0');
    if (processNameTerminatorPos == std::string::npos)
        qOhosReportFatalErrorAndAbort("%s: found unexpected content in '%s'", Q_FUNC_INFO, procCmdlinePath);

    procCmdlineData.resize(processNameTerminatorPos);
    procCmdlineData.shrink_to_fit();

    return procCmdlineData;
}

bool isThisEmbeddedUIExtensionProcess(const QNapi::Object &appContext)
{
    std::string appName = appContext.eval<QNapi::String>("applicationInfo.name");
    return readCurrentProcessNameFromProcFs() == appName + ":embeddedUI";
}

void handleAbilityStageOnCreate(const CallbackInfo &cbInfo)
{
    auto abilityStage = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

    if (qEnvironmentVariableIntValue(enableHotStartEnvVariableName) != 0) {
        auto appContext = abilityStage.eval<QNapi::Object>("context.getApplicationContext()");
        if (!isThisEmbeddedUIExtensionProcess(appContext)) {
            try {
                appContext.eval<QNapi::Value>("setSupportedProcessCache(*)", {true});
                s_hotStartEnabled = true;
            } catch (const Napi::Error &error) {
                qOhosPrintfError("setSupportedProcessCache() failed with error: %s", error.what());
            }
        }
    }

    qOhosPrintfInfo(
        "AbilityStage::onCreate: hot start enabled: %s",
        mapBoolToTrueFalseStr(s_hotStartEnabled));
}

QNapi::Value handleAbilityStageOnNewProcessRequest(const CallbackInfo &cbInfo)
{
    auto __dbg = make_QCScopedDebugJS(Q_FUNC_INFO);

    QNapi::Object qAbilityStage;
    QNapi::Object want;
    cbInfo.getLeadingArgs(Q_FUNC_INFO, qAbilityStage, want);

    qOhosPrintfInfo("AbilityStage::onNewProcessRequest: input Want: %s", QNapi::toJsonString(want).c_str());

    tryDetectBrokenWant(cbInfo.jsState(), want);

    auto optProcessIdParam = getWantParamOrEmptyIfNotPresent<QNapi::String>(want, qtAppProcessIdWantArgName);

    std::string processId = !optProcessIdParam.IsEmpty() ? optProcessIdParam : std::string();

    qOhosPrintfInfo("AbilityStage::onNewProcessRequest: returning processId='%s'", processId.c_str());

    return QNapi::String::New(cbInfo.Env(), processId);
}

QNapi::Value handleAbilityStageOnAcceptWant(const CallbackInfo &cbInfo)
{
    using namespace std::string_literals;

    QNapi::Object qAbilityStage;
    QNapi::Object want;
    cbInfo.getLeadingArgs(Q_FUNC_INFO, qAbilityStage, want);

    qOhosPrintfInfo("AbilityStage::onAcceptWant: input Want: %s", QNapi::toJsonString(want).c_str());

    tryDetectBrokenWant(cbInfo.jsState(), want);

    auto optProcessIdParam = getWantParamOrEmptyIfNotPresent<QNapi::String>(want, qtAppProcessIdWantArgName);
    if (!optProcessIdParam.IsEmpty()) {
        std::string processIdParamStr = QNapi::checkedCast<QNapi::String>(optProcessIdParam);
        auto instanceId = "//"s + processIdParamStr;
        qOhosPrintfInfo("AbilityStage::onAcceptWant: returning instanceId='%s'", instanceId.c_str());
        return QNapi::String::New(cbInfo.Env(), instanceId);
    }

    std::shared_ptr<QAbilityPeer> defaultQAbilityPeer = cbInfo.jsState().defaultQAbilityPeer();

    auto defaultQAbility = defaultQAbilityPeer->qAbility();
    auto receivedQAbilityInstanceId =
        !defaultQAbility.IsEmpty()
            ? getQAbilityInstancesManager().tryGetQAbilityInstanceIdFromWant(defaultQAbility, want)
            : std::nullopt;

    auto qAbilityInstanceId =
        receivedQAbilityInstanceId.has_value()
            ? receivedQAbilityInstanceId.value()
            : getQAbilityInstancesManager().pendingAutoStartedInstanceId().value_or(
                defaultQAbilityPeer->instanceId());

    qOhosPrintfInfo("AbilityStage::onAcceptWant: returning instanceId='%s'", qAbilityInstanceId.c_str());

    return QNapi::String::New(cbInfo.Env(), qAbilityInstanceId);
}

void asyncRunTaskInTemporaryThread(std::function<void()> task, std::function<void(JsState &)> continueFunc)
{
    auto taskRunnerThread = std::thread(
        [task = std::move(task), continueFunc = std::move(continueFunc)]() {
            task();
            QtOhos::invokeInJsThread(std::move(continueFunc));
        });
    taskRunnerThread.detach();
}

void asyncRunTaskInTemporaryThreadWithTimeout(
    JsState &jsState, std::function<void()> task, std::chrono::milliseconds waitTimeout,
    QOhosConsumer<JsState &, bool> successConsumer)
{
    auto successNotifyFunc = moveToSharedPtr(
        makeCallOnceConsumerWrapper<JsState &, bool>(
            std::move(successConsumer)));

    setJsTimeout(
        jsState,
        [successNotifyFunc](const CallbackInfo &cbInfo) {
            (*successNotifyFunc)(cbInfo.jsState(), false);
        },
        waitTimeout);

    asyncRunTaskInTemporaryThread(
        std::move(task),
        [successNotifyFunc](JsState &jsState) {
            (*successNotifyFunc)(jsState, true);
        });
}

QNapi::Value handleAbilityStageOnPrepareTerminationAsync(const CallbackInfo &cbInfo)
{
    qOhosPrintfInfo("%s", Q_FUNC_INFO);

    return makeResolvedPromise(
        cbInfo.jsState().eval<QNapi::Number>(
            "@ohos.app.ability.AbilityConstant.PrepareTermination.TERMINATE_IMMEDIATELY"));
}

void handleAbilityStageOnDestroy(const CallbackInfo &)
{
    qOhosPrintfInfo("AbilityStage::onDestroy: start");

    qOhosPrintfInfo("AbilityStage::onDestroy: destroying the Qt thread object");
    s_qtAppThreadMainFuncLauncher = nullptr;

    if (!s_exitCodeFilePath.empty()) {
        if (FILE *f = fopen(s_exitCodeFilePath.c_str(), "w")) {
            fprintf(f, "%d\n", s_appExitCode);
            fclose(f);
        }
    }

    qOhosPrintfInfo("AbilityStage::onDestroy: calling _Exit(%d)", s_appExitCode);
    std::_Exit(s_appExitCode);
}

void handleAbilityOnNewWant(const CallbackInfo &cbInfo)
{
    QNapi::Object qAbility;
    QNapi::Object want;
    QNapi::Object launchParam;
    cbInfo.getLeadingArgs(Q_FUNC_INFO, qAbility, want, launchParam);

    qOhosPrintfDebug(
        "%s: input Want: %s, launchParam: %s",
        Q_FUNC_INFO, QNapi::toJsonString(want).c_str(), QNapi::toJsonString(launchParam).c_str());

    tryDetectBrokenWant(cbInfo.jsState(), want);

    if (!QAbilityInstancesManager::isQtInternalWantFromThisProcess(want))
        dispatchNewWant(want, launchParam);
    else
        qOhosPrintfDebug("%s: received qt-internal Want from current process, nothing to do", Q_FUNC_INFO);
}

void loadWindowStageContentPage(JsState &jsState, QNapi::Object &qAbility, const QNapi::Object &windowStage)
{
    auto *jsEnv = jsState.env();

    auto qAbilityInstanceId = getQAbilityInstancesManager().getQAbilityInstanceIdOrPendingAutoStartedId(qAbility);
    auto xComponentId = QXComponentId::createForNativeNodeMainWindow(qAbilityInstanceId);

    auto qAbilityRef = moveToSharedPtr(QNapi::Reference<>::makePersistentFrom(qAbility));
    auto windowStageRef = moveToSharedPtr(QNapi::Reference<>::makePersistentFrom(windowStage));

    auto localStorage = jsState.eval<QNapi::Object>("LocalStorage.makeNewLocalStorage()");
    localStorage.eval(
        "setOrCreate(*)",
        {
            "createInfo",
            QNapi::makeObject(
                jsEnv,
                {
                    {"xComponentId", xComponentId.toNapiValue(jsEnv)},
                    {"onDisAppear", []() {}},
                    {
                        "onAttach",
                        [qAbilityRef, windowStageRef](const QtOhos::CallbackInfo &cbInfo) {
                            getQAbilityInstancesManager().handleStartedUiInstance(
                                cbInfo.jsState(), qAbilityRef->Value(), windowStageRef->Value());
                        }},
                    {
                        "onAppear",
                        [xComponentId]() {
                            qOhosPrintfDebug("XComponentId: %s onAppear", xComponentId.stringId().c_str());
                        }
                    },
                }),
        });
    qAbility.set("localStorage", localStorage);

    const std::string mainWindowNativeNodePagePath = "pages/MainWindowNativeNode";
    windowStage.evalToPromiseOrRejectOnThrow("loadContent(*)", {mainWindowNativeNodePagePath, localStorage})
    .onThen([qAbilityRef](const CallbackInfo &) {
        auto launchWant = qAbilityRef->eval<QNapi::Object>("launchWant");
        qOhosPrintfDebug("%s: launchWant: %s", Q_FUNC_INFO, QNapi::toJsonString(launchWant).c_str());
    })
    .onCatch([qAbilityInstanceId](const CallbackInfo &cbInfo) {
        QtOhos::logJsCallbackError(cbInfo, "windowStage.loadContent failed");
        qOhosReportFatalErrorAndAbort(
            "%s: Failed to loadContent for QAbility instance(qAbilityInstanceId: %s)", Q_FUNC_INFO, qAbilityInstanceId.c_str());
    });
}

QNapi::Symbol getAbilityWindowStageCreatedOrRestoredPropSymbol(JsState &jsState)
{
    struct Symbol
    {
    };
    return jsState.getJsSymbolForType<Symbol>();
}

void handleAbilityOnWindowStageCreate(const CallbackInfo &cbInfo)
{
    QNapi::Object qAbility;
    QNapi::Object windowStage;
    cbInfo.getLeadingArgs(Q_FUNC_INFO, qAbility, windowStage);

    qAbility.set(getAbilityWindowStageCreatedOrRestoredPropSymbol(cbInfo.jsState()), true);
    loadWindowStageContentPage(cbInfo.jsState(), qAbility, windowStage);
}

void handleAbilityOnWindowStageRestore(const CallbackInfo &cbInfo)
{
    QNapi::Object qAbility;
    QNapi::Object windowStage;
    cbInfo.getLeadingArgs(Q_FUNC_INFO, qAbility, windowStage);

    auto optCreatedOrRestoredProp = QNapi::getOptionalPropOrEmpty<QNapi::Boolean>(
        qAbility, getAbilityWindowStageCreatedOrRestoredPropSymbol(cbInfo.jsState()));
    bool createdOrRestoredProp = !optCreatedOrRestoredProp.IsEmpty()
        ? optCreatedOrRestoredProp.Value()
        : false;

    if (!createdOrRestoredProp) {
        qAbility.set(getAbilityWindowStageCreatedOrRestoredPropSymbol(cbInfo.jsState()), true);
        loadWindowStageContentPage(cbInfo.jsState(), qAbility, windowStage);
    } else {
        qOhosPrintfDebug(
            "%s: window page already created or restored. Skip page reloading", Q_FUNC_INFO);
    }
}

void handleAbilityOnWindowStageDestroy(const CallbackInfo &)
{
}

QNapi::Value handleAbilityOnPrepareToTerminate(const CallbackInfo &cbInfo)
{
    auto __dbg = make_QCScopedDebugJS(Q_FUNC_INFO);
    auto ability = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

    auto abilityPeer = cbInfo.jsState().tryGetQAbilityPeerByInstance(ability);
    if (!abilityPeer) {
        qOhosPrintfWarning("%s: unrecognized ability, returning immediately", Q_FUNC_INFO);
        return QNapi::Boolean::New(cbInfo.Env(), false);
    }

    bool destroyAllowed = abilityPeer->destroyAllowedFlag()->load();
    qOhosPrintfDebug(
        "%s: ability id: '%s', destroyAllowed=%s",
        Q_FUNC_INFO, abilityPeer->instanceId().c_str(), mapBoolToTrueFalseStr(destroyAllowed));

    if (!destroyAllowed) {
        QtOhos::invokeInQtThread(
            [qwindowRef = abilityPeer->qWindowRef()]() {
                auto *qwindow = qobject_cast<QWindow *>(qwindowRef.data());
                if (qwindow != nullptr) {
                    qOhosPrintfInfo("handleAbilityOnPrepareToTerminate: calling QWindow::close()");
                    QOhosCloseEventContext::runWithCloseRootCauseSet(
                        QOhosCloseEventContext::CloseRootCause::OnPrepareToTerminate,
                        [&]() {
                            qwindow->close();
                        });
                } else {
                    qOhosPrintfDebug("%s: QWindow is null", Q_FUNC_INFO);
                }
            });
    }

    return QNapi::Boolean::New(cbInfo.Env(), !destroyAllowed);
}

QNapi::Value handleAbilityOnPrepareToTerminateAsync(const CallbackInfo &cbInfo)
{
    auto __dbg = make_QCScopedDebugJS(Q_FUNC_INFO);

    auto qAbility = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

    JsState &jsState = cbInfo.jsState();

    auto optQUiAbilityPeer = QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(
        jsState.tryGetQAbilityPeerByInstance(qAbility));

    if (optQUiAbilityPeer) {
        return getQAbilityInstancesManager().getAbilityPeerBackend(optQUiAbilityPeer)->handleCloseRequestFromSystem(
            jsState, "UIAbility::onPrepareToTerminateAsync",
            QUiAbilityPeerBackend::CloseAbilityRequestSource::OnPrepareToTerminate,
            [](JsState &jsState, QUiAbilityPeerBackend::CloseAbilityRequestResolution ohosRequestResolution) {
                return QNapi::Boolean::New(
                    jsState.env(),
                    ohosRequestResolution == QUiAbilityPeerBackend::CloseAbilityRequestResolution::DontClose);
            });
    } else {
        qOhosPrintfWarning("%s: no matching QAbilityPeer, resolving immediately with 'false'", Q_FUNC_INFO);
        return makeResolvedPromise(QNapi::Boolean::New(cbInfo.Env(), false));
    }
}

void handleAbilityOnCreate(const CallbackInfo &cbInfo)
{
    auto __dbg = make_QCScopedDebugJS(Q_FUNC_INFO);

    QNapi::Object ability;
    QNapi::Object want;
    QNapi::Object launchParam;
    cbInfo.getLeadingArgs(Q_FUNC_INFO, ability, want, launchParam);

    QAbilityInstancesManager::setLaunchParamOnAbilityObject(cbInfo.jsState(), ability, launchParam);
}

QNapi::Value handleAbilityOnDestroy(const CallbackInfo &cbInfo)
{
    auto __dbg = make_QCScopedDebugJS(Q_FUNC_INFO);

    auto qAbility = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

    auto qAbilityPeer = cbInfo.jsState().tryGetQAbilityPeerByInstance(qAbility);
    if (!qAbilityPeer) {
        qOhosPrintfDebug("%s: no matching QAbilityPeer, returning resolved Promise", Q_FUNC_INFO);
        return makeResolvedPromise(cbInfo.Env().Undefined());
    }

    auto optQWindowDestroyPromise = qAbilityPeer->qWindowDestroyPromise();

    auto initialPromise = optQWindowDestroyPromise.has_value()
        ? optQWindowDestroyPromise.value()
        : makeResolvedPromise(cbInfo.Env().Undefined());

    auto resultPromiseDeferred = std::make_shared<QNapi::Promise::Deferred>(cbInfo.Env());

    initialPromise.onFinally(
        [qAbilityPeer, resultPromiseDeferred](const CallbackInfo &cbInfo) {
            qOhosPrintfDebug("%s: initial Promise resolved for id='%s'", Q_FUNC_INFO, qAbilityPeer->instanceId().c_str());

            QtOhos::removeMatchingJsQAbilityPeer(qAbilityPeer->qAbility());

            if (cbInfo.jsState().defaultQAbilityPeer()->qAbility().IsEmpty()) {
                QtOhos::quitApplicationFromJsThread();

                qOhosPrintfInfo("Qt: requested Qt app quit, waiting for the main() function to return");

                constexpr auto resultPromiseAutoresolveTimeout = 5s;

                asyncRunTaskInTemporaryThreadWithTimeout(
                    cbInfo.jsState(),
                    []() {
                        s_qtAppThreadIdleStateWaitFunc();
                    },
                    resultPromiseAutoresolveTimeout,
                    [instanceId = qAbilityPeer->instanceId(), resultPromiseDeferred](JsState &jsState, bool threadExited) {
                        qOhosPrintfInfo(
                            "Qt: end waiting for Qt app's main() function, returned: %s",
                            mapBoolToTrueFalseStr(threadExited));
                        qOhosPrintfDebug("%s: resolving result Promise for id='%s'", Q_FUNC_INFO, instanceId.c_str());
                        resultPromiseDeferred->Resolve(Napi::Env(jsState.env()).Undefined());
                    });
            } else {
                resultPromiseDeferred->Resolve(cbInfo.Env().Undefined());
            }
        });

    qOhosPrintfDebug("%s: returning Promise for id='%s'", Q_FUNC_INFO, qAbilityPeer->instanceId().c_str());

    return resultPromiseDeferred->Promise();
}

void initDeviceInfo(JsState &jsState)
{
    using Type = QOhosDeviceInfo::Type;

    static const std::pair<Type, const char *> strPropertiesMap[] = {
        {Type::deviceType, "deviceType"},
        {Type::manufacture, "manufacture"},
        {Type::brand, "brand"},
        {Type::marketName, "marketName"},
        {Type::productSeries, "productSeries"},
        {Type::productModel, "productModel"},
        {Type::softwareModel, "softwareModel"},
        {Type::hardwareModel, "hardwareModel"},
        {Type::hardwareProfile, "hardwareProfile"},
        {Type::serial, "serial"},
        {Type::bootloaderVersion, "bootloaderVersion"},
        {Type::abiList, "abiList"},
        {Type::securityPatchTag, "securityPatchTag"},
        {Type::displayVersion, "displayVersion"},
        {Type::incrementalVersion, "incrementalVersion"},
        {Type::osReleaseType, "osReleaseType"},
        {Type::osFullName, "osFullName"},
        {Type::versionId, "versionId"},
        {Type::buildType, "buildType"},
        {Type::buildUser, "buildUser"},
        {Type::buildHost, "buildHost"},
        {Type::buildTime, "buildTime"},
        {Type::buildRootHash, "buildRootHash"},
        {Type::udid, "udid"},
        {Type::distributionOSName, "distributionOSName"},
        {Type::distributionOSVersion, "distributionOSVersion"},
        {Type::distributionOSReleaseType, "distributionOSReleaseType"},
    };

    static const std::pair<Type, const char *> intPropertiesMap[] = {
        {Type::majorVersion, "majorVersion"},
        {Type::seniorVersion, "seniorVersion"},
        {Type::featureVersion, "featureVersion"},
        {Type::buildVersion, "buildVersion"},
        {Type::sdkApiVersion, "sdkApiVersion"},
        {Type::firstApiVersion, "firstApiVersion"},
        {Type::distributionOSApiVersion, "distributionOSApiVersion"},
    };

    auto deviceInfoObj = jsState.eval<QNapi::Object>("@ohos.deviceInfo");

    QMap<Type, QVariant> deviceInfo;
    for (const auto &propEntry : strPropertiesMap)
        deviceInfo[propEntry.first] = QString::fromStdString(deviceInfoObj.get<QNapi::String>(propEntry.second));
    for (const auto &propEntry : intPropertiesMap)
        deviceInfo[propEntry.first] = static_cast<int>(deviceInfoObj.get<QNapi::Number>(propEntry.second));

    QOhosDeviceInfo::init(std::move(deviceInfo));
}

void initAppData(JsState &jsState, QNapi::Object appContext)
{
    initDeviceInfo(jsState);

    auto systemLocaleId = QString::fromStdString(
        jsState.eval<QNapi::String>("@ohos.intl.Locale<new>().toString()"));
    auto systemPreferredLanguages = QNapi::getArrayElements<QStringList, QNapi::String>(
        jsState.eval<QNapi::Array>("@ohos.i18n.System.getPreferredLanguageList()"),
        &QString::fromStdString);
    QtOhos::invokeInQtThread(
        [systemLocaleId, systemPreferredLanguages]() {
            QOhosPlatformIntegration::setSystemLocale(new QOhosSystemLocale(systemLocaleId, systemPreferredLanguages));
        });

    if (!isOhosNoUiChildMode())
        appContext.eval("setColorMode(*)", {jsState.mapOhosEnumToJs(defaultColorMode)});
}

std::string buildFcLangEnvVariableValue(JsState &jsState)
{
    std::string language = jsState.eval<QNapi::String>("@ohos.intl.Locale<new>().language");
    std::string region = jsState.eval<QNapi::String>("@ohos.intl.Locale<new>().region");

    return language + "_" + region + ".UTF-8";
}

std::shared_ptr<QAbilityEngine> makeAbilityEngineForQtRunMode(QtRunMode qtRunMode)
{
    switch (qtRunMode) {
    case QtRunMode::Normal:
    case QtRunMode::NoUiChildProcess:
        return std::make_shared<QUiAbilityEngine>();
    }

    qOhosReportFatalErrorAndAbort("%s: Invalid qtRunMode: %d", Q_FUNC_INFO, static_cast<int>(qtRunMode));
}

void setupQtApplicationImpl(JsState &jsState, QNapi::Object appStartupObj, QtRunMode qtRunMode)
{
    auto appContext = appStartupObj.get<QNapi::Object>("appContext");
    auto appContextDirs = AppContextDirs::mapFromNapiObject(appContext);
    if (appContextDirs.resourceDir.empty()) {
        auto optResourceDirProp = QNapi::getOptionalPropOrEmpty<QNapi::String>(appStartupObj, "resourceDir");
        std::string resourceDir = !optResourceDirProp.IsEmpty() ? optResourceDirProp : std::string();
        appContextDirs.resourceDir = !resourceDir.empty()
            ? resourceDir
            : appContextDirs.bundleCodeDir + "/entry/resources/resfile";
    }

    auto jsModulesFactories = appStartupObj.get<QNapi::Object>("modulesFactories");

    qOhosPrintfDebug("%s: setting up Qt in %s mode", Q_FUNC_INFO, mapQtRunModeToString(qtRunMode));

    getQAbilityInstancesManagerPtr() =
        makeQAbilityInstancesManager(
            makeAbilityEngineForQtRunMode(qtRunMode),
            &handleDefaultQAbilityInstanceStartup);

    currentQtRunMode = qtRunMode;
    QtOhos::initJsThreadState(
        jsModulesFactories.Env(), makeJsModulesFactoriesMap(jsModulesFactories),
        std::make_shared<AppFunctionsImpl>(), qtRunMode);

    QOhosAppContext::init(appContextDirs.mapToQOhosAppContextProperties());
    initAppData(jsState, appContext);

    const auto ohosSdkApiVersion = QOhosDeviceInfo::sdkApiVersion();
    if (ohosSdkApiVersion < minSupportedOhosSdkApiVersion) {
        qOhosReportFatalErrorAndAbort(
            "%s: unsupported OHOS version! Current API version: %d, minimum supported version: %d. Aborting.",
            Q_FUNC_INFO, ohosSdkApiVersion, minSupportedOhosSdkApiVersion);
    }

    const auto recognizedDeviceType = QOhosDeviceInfo::tryGetRecognizedDeviceType();
    if (!recognizedDeviceType.has_value()) {
        qOhosReportFatalErrorAndAbort(
            "%s: Unrecognized device type: %s. Qt does not support unrecognized devices. Aborting.",
            Q_FUNC_INFO, qPrintable(QOhosDeviceInfo::getProperty(QOhosDeviceInfo::Type::deviceType).toString()));
    }

    bool allowUnsupportedDevices =
        qEnvironmentVariable("QT_IO_EXPERIMENTAL_ALLOW_UNSUPPORTED_DEVICES", {})
        == QLatin1String("true");
    if (!QOhosDeviceInfo::isCurrentDeviceSupported() && !allowUnsupportedDevices) {
        qOhosReportFatalErrorAndAbort(
            "%s: Unsupported device type: %s. Aborting.",
            Q_FUNC_INFO, qPrintable(QOhosDeviceInfo::getProperty(QOhosDeviceInfo::Type::deviceType).toString()));
    }

    s_appSharedLibName = appStartupObj.get<QNapi::String>("appName");

    qOhosPrintfDebug("setupQtApplication() - sharedLibraryName: %s", s_appSharedLibName.c_str());
    qOhosPrintfDebug("setupQtApplication() - bundleCodeDir: %s", appContextDirs.bundleCodeDir.c_str());

    s_appSharedLibsDirPath = appContextDirs.bundleCodeDir + targetLibraryDirectory();
    qOhosPrintfDebug("setupQtApplication() - Shared libraries directory: %s", s_appSharedLibsDirPath.c_str());

    QByteArrayList qmls = { QByteArray::fromStdString(appContextDirs.resourceDir + "/qml") };

    auto jsAppArgs = QNapi::getPropOrUndefined(appStartupObj, "appArgs");
    if (jsAppArgs.IsArray()) {
        s_appArgs = std::make_unique<std::vector<std::string>>(
            QNapi::getArrayElements<std::vector<std::string>, QNapi::String>(
                QNapi::checkedCast<QNapi::Array>(jsAppArgs)));
    }

    struct {
        const char *variable;
        std::string value;
    } env_variables[] = {
        {"QT_QPA_PLATFORM_PLUGIN_PATH", s_appSharedLibsDirPath },
        {"QT_QPA_PLATFORMTHEME", ohosThemeName},
        {"QT_QPA_PLATFORM", "ohos"},
        {"QML_DISABLE_DISK_CACHE", "1"},
        {"QT_PLUGIN_PATH", s_appSharedLibsDirPath },
        {"QML2_IMPORT_PATH", qmls.join(":").toStdString() },
        // FIXME: temporary measure for preventing QtQuick2-based apps from crashing
        {"QV4_FORCE_INTERPRETER", "1"},
        {"QT_PRINTER_MODULE", "ohosprintersupport"},
        {"TMPDIR", QOhosAppContext::getProperty(QOhosAppContext::Type::tempDir).toStdString()},
        {"HOME", QOhosAppContext::getProperty(QOhosAppContext::Type::filesDir).toStdString()},
        {"FC_LANG", buildFcLangEnvVariableValue(jsState)},
    };

    for (const auto &e : env_variables) {
        if (::setenv(e.variable, e.value.c_str(), 1) != 0) {
            throw std::runtime_error(
                QString::fromUtf8("Cannot set %1 environment variable").arg(QString::fromUtf8(e.variable)).toStdString());
        }
    }

    if (::chdir(appContextDirs.filesDir.c_str()) != 0) {
        auto chdirErrno = errno;
        qOhosPrintfWarning(
            "%s: failed to change current directory to '%s': %s",
            Q_FUNC_INFO, appContextDirs.filesDir.c_str(), std::strerror(chdirErrno));
    }
}

QtRunMode getQtRunModeFromAppStartupObj(QNapi::Object appStartupObj)
{
    std::unordered_map<std::string, QtRunMode> abilityClassNameToQtRunModeMap = {
        {"QAbility", QtRunMode::Normal},
    };

    const auto optAbilityClassName = QNapi::getOptionalPropOrEmpty<QNapi::String>(appStartupObj, "abilityClassName");
    if (!optAbilityClassName.IsEmpty()) {
        const std::string abilityClassName = optAbilityClassName;
        if (abilityClassNameToQtRunModeMap.find(abilityClassName) != abilityClassNameToQtRunModeMap.end())
            return abilityClassNameToQtRunModeMap[abilityClassName];

        qOhosReportFatalErrorAndAbort(
            "%s: got unsupported name of the Ability class: '%s'", Q_FUNC_INFO, abilityClassName.c_str());
    }

    return QtRunMode::Normal;
}

void setupQtApplication(const CallbackInfo &cbInfo)
{
    auto appStartupObj = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
    auto qtRunMode = getQtRunModeFromAppStartupObj(appStartupObj);
    setupQtApplicationImpl(cbInfo.jsState(), appStartupObj, qtRunMode);
}

void runQtChildProcess(const CallbackInfo &cbInfo)
{
    auto __dbg = make_QCScopedDebugJS(Q_FUNC_INFO);

    auto paramsObj = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

    QOhosPlatformFontDatabase::setOhosNoUiChildMode();

    auto childSetupObj = readChildProcessSetupData(cbInfo.Env());
    childSetupObj["modulesFactories"] = paramsObj.get<QNapi::Object>("modulesFactories");

    setupQtApplicationImpl(cbInfo.jsState(), childSetupObj, QtRunMode::NoUiChildProcess);

    auto appArgs = s_appArgs ? *s_appArgs : std::vector<std::string>();

    QThread::currentThread();
    QtOhos::initQtThreadState();
    auto appMainFuncLauncher = makeAppMainFuncLauncher(
        {
            .appLibraryPath = s_appSharedLibsDirPath + "/" + s_appSharedLibName,
        },
        [](int) {
        });
    appMainFuncLauncher(appArgs);
}

QNapi::Value makeXComponentIdForMainWindowWithQAbilityInstanceId(const CallbackInfo &cbInfo)
{
    std::string qAbilityInstanceId = cbInfo.getFirstArg<QNapi::String>(Q_FUNC_INFO);
    return QXComponentId::createForNativeNodeMainWindow(qAbilityInstanceId).toNapiValue(cbInfo.Env());
}

QNapi::Value checkIsAdapterCApiSupported(const CallbackInfo &cbInfo)
{
    constexpr bool adapterCApiSupported = true;
    return QNapi::Boolean::New(cbInfo.Env(), adapterCApiSupported);
}

}

bool isOhosNoUiChildMode()
{
    return currentQtRunMode == QtRunMode::NoUiChildProcess;
}

bool isVsyncOnSoftwareBackingStoreEnabled()
{
    return vsyncOnSoftwareBackingStoreEnabled;
}

void quitApplicationFromJsThread()
{
    auto __dbg = make_QCScopedDebugJS("quitApplicationFromJsThread");
    auto hotStartIterationToQuit = s_hotStartIteration.lastRequestedInJsThread;
    QtOhos::invokeInQtThread(
        [hotStartIterationToQuit]() {
            if (s_hotStartIteration.activeInQtThread == hotStartIterationToQuit)
                QCoreApplication::quit();
        });
}

void updateApplicationState(int state)
{
    qOhosDebug(QtForOhos) << "QOhos updateApplicationState" << state;

    if (QOhosPlatformIntegration::instance() == nullptr)
        return;

    if (state <= Qt::ApplicationInactive) {
        // NOTE: sometimes we will receive two consecutive suspended notifications,
        // In the second suspended notification, QWindowSystemInterface::flushWindowSystemEvents()
        // will deadlock since the dispatcher has been stopped in the first suspended notification.
        // To avoid the deadlock we simply return if we found the event dispatcher has been stopped.
        if (QOhosEventDispatcherStopper::stopped())
            return;

        // Don't send timers and sockets events anymore if we are going to hide all windows
        QOhosEventDispatcherStopper::instance()->goingToStop(true);
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationState(state));
    } else {
        QOhosEventDispatcherStopper::instance()->startAll();
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationState(state));
        QOhosEventDispatcherStopper::instance()->goingToStop(false);
    }
}

bool blockEventLoopsWhenSuspended()
{
    static bool block = qEnvironmentVariableIntValue("QT_BLOCK_EVENT_LOOPS_WHEN_SUSPENDED") != 0;
    return block;
}

bool isGlBackingStoreDefaultEnabled()
{
    return experimentalEnableGlBackinStore;
}

bool isDebugDrawQtRasterBackingStoreFlushedRegionEnabled()
{
    return debugDrawQtRasterBackingStoreFlushedRegion;
}

bool isDebugUseBasicStyleAndThemeEnabled()
{
    return debugUseBasicStyleAndTheme;
}

bool isNativeNodeApiKeyEventsEnabled()
{
    return enableNativeNodeApiKeyEvents;
}

bool isNativeNodeApiMouseEventsEnabled()
{
    return enableNativeNodeApiMouseEvents;
}

bool acquireAndCleanPendingAutoStartedInstanceWindowFlag()
{
    return std::exchange(s_autoStartedAbilityInstanceWaitingForQtWindow, false);
}

}

QT_END_NAMESPACE

EXTERN_C_START

static napi_value Init(napi_env env, napi_value exports)
{
    auto __dbg = make_QCScopedDebugJS("qohosjsmain Init");

    QNapi::Object(env, exports).DefineProperties(
        {
            Napi::PropertyDescriptor::Function("handleAbilityStageOnCreate", QtOhos::handleAbilityStageOnCreate),
            Napi::PropertyDescriptor::Function("handleAbilityStageOnNewProcessRequest", QtOhos::handleAbilityStageOnNewProcessRequest),
            Napi::PropertyDescriptor::Function("handleAbilityStageOnAcceptWant", QtOhos::handleAbilityStageOnAcceptWant),
            Napi::PropertyDescriptor::Function("handleAbilityStageOnPrepareTerminationAsync", QtOhos::handleAbilityStageOnPrepareTerminationAsync),
            Napi::PropertyDescriptor::Function("handleAbilityStageOnDestroy", QtOhos::handleAbilityStageOnDestroy),
            Napi::PropertyDescriptor::Function("handleAbilityOnNewWant", QtOhos::handleAbilityOnNewWant),
            Napi::PropertyDescriptor::Function("handleAbilityOnWindowStageCreate", QtOhos::handleAbilityOnWindowStageCreate),
            Napi::PropertyDescriptor::Function("handleAbilityOnWindowStageRestore", QtOhos::handleAbilityOnWindowStageRestore),
            Napi::PropertyDescriptor::Function("handleAbilityOnWindowStageDestroy", QtOhos::handleAbilityOnWindowStageDestroy),
            Napi::PropertyDescriptor::Function("onStageDestroy", QtOhos::handleAbilityOnWindowStageDestroy),
            Napi::PropertyDescriptor::Function("handleAbilityOnPrepareToTerminate", QtOhos::handleAbilityOnPrepareToTerminate),
            Napi::PropertyDescriptor::Function("handleAbilityOnPrepareToTerminateAsync", QtOhos::handleAbilityOnPrepareToTerminateAsync),
            Napi::PropertyDescriptor::Function("handleAbilityOnCreate", QtOhos::handleAbilityOnCreate),
            Napi::PropertyDescriptor::Function("handleAbilityOnDestroy", QtOhos::handleAbilityOnDestroy),
            Napi::PropertyDescriptor::Function("handleAbilityOnBackground", QtOhos::handleAbilityOnBackground),
            Napi::PropertyDescriptor::Function("handleAbilityOnContinue", QtOhos::handleAbilityOnContinue),
            Napi::PropertyDescriptor::Function("onBackground", QtOhos::handleAbilityOnBackground),
            Napi::PropertyDescriptor::Function("handleAbilityOnForeground", QtOhos::handleAbilityOnForeground),
            Napi::PropertyDescriptor::Function("onForeground", QtOhos::handleAbilityOnForeground),
            Napi::PropertyDescriptor::Function("setupQtApplication", QtOhos::setupQtApplication),
            Napi::PropertyDescriptor::Function("runQtChildProcess", QtOhos::runQtChildProcess),
            Napi::PropertyDescriptor::Function(
                "makeXComponentIdForMainWindowWithQAbilityInstanceId",
                QtOhos::makeXComponentIdForMainWindowWithQAbilityInstanceId),
            Napi::PropertyDescriptor::Function("checkIsAdapterCApiSupported", QtOhos::checkIsAdapterCApiSupported),
        });

    // Here put elements that
    QArkUi::QXComponentRegistry::Init(env, exports);

    return exports;
}

EXTERN_C_END

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    static napi_module qtMainModule = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = Init,
        .nm_modname = "qohos",
        .nm_priv = nullptr,
        .reserved = {nullptr},
    };

    napi_module_register(&qtMainModule);
}
