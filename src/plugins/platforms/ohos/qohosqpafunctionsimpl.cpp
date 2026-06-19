// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <qohosjsenv_p.h>
#include <QtCore/qobject.h>
#include <QtCore/qscopeguard.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qcolor.h>
#include <QtGui/qimage.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <algorithm>
#include <chrono>
#include <filemanagement/file_uri/oh_file_uri.h>
#include <filemanagement/fileshare/oh_file_share.h>
#include <functional>
#include <info/application_target_sdk_version.h>
#include <memory>
#include <qohosapppermissions_p.h>
#include <qohosenums.h>
#include <qohosjsmain.h>
#include <qohosjsutils.h>
#include <qohospixelmapconversions.h>
#include <qohosplatformclipboard.h>
#include <qohosplatformintegration.h>
#include <qohosplatformservices.h>
#include <qohosplatformwindow.h>
#include <qohosplugincore.h>
#include <qohosqpafunctions_p.h>
#include <qohossettings.h>
#include <qohossharekit.h>
#include <qohosudmfconversions.h>
#include <qohosutils.h>
#include <qohoswindowmanager.h>
#include <qohoswindowproperty.h>
#include <render/qwindowproxyregistry.h>
#include <signal.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE

namespace QtOhos {

namespace {

const QOhosPropertyDescriptor<QOhosQpaFunctions::AudioStreamUsage> audioStreamUsageProperty{};

QOhosOptional<QOhosQpaFunctions::WantInfo::LaunchReason> tryMapOhosLaunchReasonToWantInfoEnum(
    enums::ohos::app::ability::AbilityConstant::LaunchReason ohosLaunchReason)
{
    using OhosLaunchReason = enums::ohos::app::ability::AbilityConstant::LaunchReason;
    using WantInfo = QOhosQpaFunctions::WantInfo;

    switch (ohosLaunchReason) {
    case OhosLaunchReason::START_ABILITY:
        return makeQOhosOptional(WantInfo::LaunchReason::START_ABILITY);
    case OhosLaunchReason::CONTINUATION:
        return makeQOhosOptional(WantInfo::LaunchReason::CONTINUATION);
    case OhosLaunchReason::PREPARE_CONTINUATION:
        return makeQOhosOptional(WantInfo::LaunchReason::PREPARE_CONTINUATION);
    case OhosLaunchReason::PRELOAD:
        return makeQOhosOptional(WantInfo::LaunchReason::PRELOAD);
    case OhosLaunchReason::UNKNOWN:
    case OhosLaunchReason::CALL:
    case OhosLaunchReason::APP_RECOVERY:
    case OhosLaunchReason::SHARE:
    case OhosLaunchReason::AUTO_STARTUP:
    case OhosLaunchReason::INSIGHT_INTENT:
        return makeQOhosOptional(WantInfo::LaunchReason::UNKNOWN);
    }

    return {};
}

QOhosQpaFunctions::WantInfo::LaunchReason mapJsLaunchReasonToWantInfoEnumWithFallback(
    QtOhos::JsState &jsState, QNapi::Number jsLaunchReason)
{
    auto optLaunchReasonJsEnum =
        jsState.tryMapOhosEnumFromJs<enums::ohos::app::ability::AbilityConstant::LaunchReason>(jsLaunchReason);
    auto optLaunchReason =
        optLaunchReasonJsEnum.has_value()
            ? tryMapOhosLaunchReasonToWantInfoEnum(optLaunchReasonJsEnum.value())
            : makeEmptyQOhosOptional();
    return optLaunchReason.value_or(QOhosQpaFunctions::WantInfo::LaunchReason::UNKNOWN);
}

Q_NORETURN void killCurrentProcess()
{
    ::kill(getpid(), SIGKILL);
    std::abort();
}

QOhosOptional<QOhosAbilityOnContinueResult> tryMapAbilityOnContinueResponseStatusToOhos(
    QOhosQpaFunctions::AbilityOnContinueResponseStatus status)
{
    using AbilityOnContinueResponseStatus = QOhosQpaFunctions::AbilityOnContinueResponseStatus;

    switch (status) {
    case AbilityOnContinueResponseStatus::Agree:
        return makeQOhosOptional(QOhosAbilityOnContinueResult::AGREE);
    case AbilityOnContinueResponseStatus::Reject:
        return makeQOhosOptional(QOhosAbilityOnContinueResult::REJECT);
    case AbilityOnContinueResponseStatus::Mismatch:
        return makeQOhosOptional(QOhosAbilityOnContinueResult::MISMATCH);
    }
    return {};
}

QOhosOptional<enums::ohos::app::ability::AbilityConstant::WindowMode> tryMapWindowModeToOhosOrLogWarning(
    QOhosQpaFunctions::StartOptions::WindowMode windowMode)
{
    namespace AbilityConstant = enums::ohos::app::ability::AbilityConstant;
    using StartOptions = QOhosQpaFunctions::StartOptions;

    switch (windowMode) {
    case StartOptions::WindowMode::WINDOW_MODE_SPLIT_PRIMARY:
        return makeQOhosOptional(AbilityConstant::WindowMode::WINDOW_MODE_SPLIT_PRIMARY);
    case StartOptions::WindowMode::WINDOW_MODE_SPLIT_SECONDARY:
        return makeQOhosOptional(AbilityConstant::WindowMode::WINDOW_MODE_SPLIT_SECONDARY);
    case StartOptions::WindowMode::WINDOW_MODE_FULLSCREEN:
        return makeQOhosOptional(AbilityConstant::WindowMode::WINDOW_MODE_FULLSCREEN);
    }

    qCWarning(QtForOhos, "%s: got illegal WindowMode: %d", Q_FUNC_INFO, static_cast<int>(windowMode));

    return {};
}

QOhosOptional<enums::ohos::app::ability::contextConstant::ProcessMode> tryMapProcessModeToOhosOrLogWarning(
    QOhosQpaFunctions::StartOptions::ProcessMode processMode)
{
    namespace contextConstant = enums::ohos::app::ability::contextConstant;
    using StartOptions = QOhosQpaFunctions::StartOptions;

    switch (processMode) {
    case StartOptions::ProcessMode::NEW_PROCESS_ATTACH_TO_PARENT:
        return makeQOhosOptional(contextConstant::ProcessMode::NEW_PROCESS_ATTACH_TO_PARENT);
    case StartOptions::ProcessMode::NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM:
        return makeQOhosOptional(contextConstant::ProcessMode::NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM);
    }

    qCWarning(QtForOhos, "%s: got illegal ProcessMode: %d", Q_FUNC_INFO, static_cast<int>(processMode));

    return {};
}

QOhosOptional<enums::ohos::app::ability::contextConstant::StartupVisibility> tryMapStartupVisibilityToOhosOrLogWarning(
    QOhosQpaFunctions::StartOptions::StartupVisibility startupVisibility)
{
    namespace contextConstant = enums::ohos::app::ability::contextConstant;
    using StartOptions = QOhosQpaFunctions::StartOptions;

    switch (startupVisibility) {
    case StartOptions::StartupVisibility::STARTUP_HIDE:
        return makeQOhosOptional(contextConstant::StartupVisibility::STARTUP_HIDE);
    case StartOptions::StartupVisibility::STARTUP_SHOW:
        return makeQOhosOptional(contextConstant::StartupVisibility::STARTUP_SHOW);
    }

    qCWarning(QtForOhos, "%s: got illegal StartupVisibility: %d", Q_FUNC_INFO, static_cast<int>(startupVisibility));

    return {};
}

QOhosOptional<enums::ohos::bundle::bundleManager::SupportWindowMode> tryMapSupportWindowModeToOhosOrLogWarning(
    QOhosQpaFunctions::StartOptions::SupportWindowMode supportWindowMode)
{
    namespace bundleManager = enums::ohos::bundle::bundleManager;
    using StartOptions = QOhosQpaFunctions::StartOptions;

    switch (supportWindowMode) {
    case StartOptions::SupportWindowMode::FULL_SCREEN:
        return makeQOhosOptional(bundleManager::SupportWindowMode::FULL_SCREEN);
    case StartOptions::SupportWindowMode::SPLIT:
        return makeQOhosOptional(bundleManager::SupportWindowMode::SPLIT);
    case StartOptions::SupportWindowMode::FLOATING:
        return makeQOhosOptional(bundleManager::SupportWindowMode::FLOATING);
    }

    qCWarning(QtForOhos, "%s: got illegal SupportWindowMode: %d", Q_FUNC_INFO, static_cast<int>(supportWindowMode));

    return {};
}

QNapi::Array mapSupportWindowModesToJsEnumsArray(
    QtOhos::JsState &jsState, const QList<QOhosQpaFunctions::StartOptions::SupportWindowMode> &supportWindowModes)
{
    std::vector<QNapi::ValueWrapper> jsSupportWindowModes;
    for (auto supportWindowMode : supportWindowModes) {
        auto optOhosSupportWindowMode = tryMapSupportWindowModeToOhosOrLogWarning(supportWindowMode);
        if (optOhosSupportWindowMode.has_value())
            jsSupportWindowModes.push_back(jsState.mapOhosEnumToJs(optOhosSupportWindowMode.value()));
    }

    return QNapi::makeArray(jsState.env(), jsSupportWindowModes);
}

QNapi::Object makeJsCompletionHandler(
    QtOhos::JsState &jsState, std::shared_ptr<QOhosConsumer<bool, QJsonObject, QString>> qtThreadCompletionHandler)
{
    auto makeCompletionCallback = [qtThreadCompletionHandler](bool requestSuccess) {
        return [qtThreadCompletionHandler, requestSuccess](const QNapi::CallbackInfo &cbInfo) {
            QNapi::Object elementNameObj;
            QNapi::String messageValue;
            cbInfo.getLeadingArgs(Q_FUNC_INFO, elementNameObj, messageValue);

            const QJsonObject elementName = QOhosJsEnv::fromNapiValue<QJsonObject>(elementNameObj);
            const QString message = QString::fromStdString(messageValue);

            QtOhos::invokeInQtThread(
                [qtThreadCompletionHandler, requestSuccess, elementName, message]() {
                    (*qtThreadCompletionHandler)(requestSuccess, elementName, message);
                });
        };
    };

    return QNapi::makeObject(
        jsState.env(),
        {
            {"onRequestSuccess", makeCompletionCallback(true)},
            {"onRequestFailure", makeCompletionCallback(false)},
        });
}

using OhosConfigurationColorMode = QtOhos::enums::ohos::app::ability::ConfigurationConstant::ColorMode;

QNapi::Object convertStartOptionsToNapiObject(
    QtOhos::JsState &jsState, const QOhosQpaFunctions::StartOptions &opts)
{
    auto *env = jsState.env();
    auto napiOptions = QNapi::Object::New(env);

    auto optOhosWindowMode = qAndThen(opts.windowMode, &tryMapWindowModeToOhosOrLogWarning);
    if (optOhosWindowMode.has_value())
        napiOptions.set("windowMode", jsState.mapOhosEnumToJs(optOhosWindowMode.value()));
    if (opts.displayId.has_value())
        napiOptions.set("displayId", opts.displayId.value());
    if (opts.withAnimation.has_value())
        napiOptions.set("withAnimation", opts.withAnimation.value());
    if (opts.windowLeft.has_value())
        napiOptions.set("windowLeft", opts.windowLeft.value());
    if (opts.windowTop.has_value())
        napiOptions.set("windowTop", opts.windowTop.value());
    if (opts.windowWidth.has_value())
        napiOptions.set("windowWidth", opts.windowWidth.value());
    if (opts.windowHeight.has_value())
        napiOptions.set("windowHeight", opts.windowHeight.value());
    auto optOhosProcessMode = qAndThen(opts.processMode, &tryMapProcessModeToOhosOrLogWarning);
    if (optOhosProcessMode.has_value())
        napiOptions.set("processMode", jsState.mapOhosEnumToJs(optOhosProcessMode.value()));
    auto optOhosStartupVisibility = qAndThen(opts.startupVisibility, &tryMapStartupVisibilityToOhosOrLogWarning);
    if (optOhosStartupVisibility.has_value())
        napiOptions.set("startupVisibility", jsState.mapOhosEnumToJs(optOhosStartupVisibility.value()));
    if (opts.windowIcon.has_value()) {
        auto windowIcon = opts.windowIcon.value().value<QImage>();
        if (!windowIcon.isNull())
            napiOptions.set("startWindowIcon", createNapiPixelMapFromQImage(jsState, windowIcon));
    }
    if (opts.windowBackgroundColorHex.has_value())
        napiOptions.set("startWindowBackgroundColor", opts.windowBackgroundColorHex.value().toStdString());
    if (opts.supportWindowModes.has_value()) {
        auto jsSupportWindowModes = mapSupportWindowModesToJsEnumsArray(jsState, opts.supportWindowModes.value());
        if (jsSupportWindowModes.Length() != 0)
            napiOptions.set("supportWindowModes", jsSupportWindowModes);
        else
            qCWarning(QtForOhos, "%s: OHOS doesn't support empty supportWindowModes, skipping", Q_FUNC_INFO);
    }
    if (opts.minWindowWidth.has_value())
        napiOptions.set("minWindowWidth", opts.minWindowWidth.value());
    if (opts.minWindowHeight.has_value())
        napiOptions.set("minWindowHeight", opts.minWindowHeight.value());
    if (opts.maxWindowWidth.has_value())
        napiOptions.set("maxWindowWidth", opts.maxWindowWidth.value());
    if (opts.maxWindowHeight.has_value())
        napiOptions.set("maxWindowHeight", opts.maxWindowHeight.value());
    if (opts.optCompletionHandler)
        napiOptions.set("completionHandler", makeJsCompletionHandler(jsState, opts.optCompletionHandler));
    if (opts.hideStartWindow.has_value())
        napiOptions.set("hideStartWindow", opts.hideStartWindow.value());
    if (opts.windowCreateParams.has_value()) {
        const auto &windowCreateParams = opts.windowCreateParams.value();
        std::vector<std::pair<std::string, QNapi::ValueWrapper>> windowCreateParamsProps;
        if (windowCreateParams.setWindowFadeInOutAnimation) {
            windowCreateParamsProps.emplace_back(
                "animationParams",
                QNapi::makeObject(
                    env,
                    {
                        {
                            "type",
                            jsState.mapOhosEnumToJs(
                                enums::ohos::window::AnimationType::FADE_IN_OUT),
                        }
                    }));
        }
        napiOptions.set("windowCreateParams", QNapi::makeObject(env, windowCreateParamsProps));
    }

    return napiOptions;
}

std::shared_ptr<void> registerAppContextEnvironmentCallback(
    QtOhos::JsState &jsState, QNapi::Object environmentCallback)
{
    auto appContextRefPtr = QtOhos::moveToSharedPtr(
        QNapi::Reference<>::makePersistentFrom(
            jsState.defaultQAbilityPeer()->qAbility().eval<QNapi::Object>(
                "context.getApplicationContext()")));

    double environmentCallbackId = appContextRefPtr->call<QNapi::Number>(
        "on",
        {"environment", environmentCallback});

    return std::shared_ptr<void>(
        nullptr,
        [environmentCallbackId, appContextRefPtr](auto) {
            QtOhos::runInJsThreadAndWait(
                [&](QtOhos::JsState &) {
                    auto appContextRef = std::move(*appContextRefPtr);
                    appContextRef.call(
                        "off",
                        {"environment", environmentCallbackId});
                },
                Q_FUNC_INFO);
        });
}

std::shared_ptr<void> registerAppConfigurationUpdateListener(
    QtOhos::JsState &jsState, std::function<void(QtOhos::JsState &, QNapi::Object)> updateListener)
{
    return registerAppContextEnvironmentCallback(
        jsState,
        QNapi::makeObject(
            jsState.env(),
            {
                {
                    "onConfigurationUpdated",
                    [updateListener = std::move(updateListener)](const QtOhos::CallbackInfo &cbInfo) {
                        auto config = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                        updateListener(cbInfo.jsState(), config);
                    }
                },
            }));
}

OhosConfigurationColorMode mapOhosConfigurationColorModeFromJs(QtOhos::JsState &jsState, QNapi::Number colorModeJsEnum)
{
    constexpr auto fallbackColorMode = OhosConfigurationColorMode::COLOR_MODE_NOT_SET;
    auto optColorMode = jsState.tryMapOhosEnumFromJs<OhosConfigurationColorMode>(colorModeJsEnum);
    return optColorMode.value_or(fallbackColorMode);
};

void setOhosConfigColorMode(OhosConfigurationColorMode colorMode)
{
    if (QtOhos::isOhosNoUiChildMode()) {
        qCWarning(QtForOhos, "%s: cannot set a color mode in 'no UI child mode'", Q_FUNC_INFO);
        return;
    }

    QtOhos::runInJsThreadAndWait(
        [&](QtOhos::JsState &jsState) {
            auto qAbility = jsState.defaultQAbilityPeer()->qAbility();
            const auto jsColorMode = jsState.mapOhosEnumToJs(colorMode);
            qAbility.call("context.getApplicationContext().setColorMode", {jsColorMode});
        },
        Q_FUNC_INFO);
}

template<typename ConfigValue>
QOhosSupplier<ConfigValue> makeOhosConfigValueDataSource(
    std::function<ConfigValue(QtOhos::JsState &)> initValueSupplier,
    std::function<ConfigValue(QtOhos::JsState &, const QNapi::Object &)> valueFetcher,
    QOhosConsumer<ConfigValue> valueChangedHandler)
{
    return QtOhos::makeDataSource<ConfigValue>(
        std::move(initValueSupplier),
        [valueFetcher = std::move(valueFetcher)](QtOhos::JsState &jsState, QOhosConsumer<ConfigValue> valueUpdatesConsumer) mutable {
            return registerAppConfigurationUpdateListener(
                jsState,
                [valueFetcher = std::move(valueFetcher), valueUpdatesConsumer = std::move(valueUpdatesConsumer)](QtOhos::JsState &jsState, QNapi::Object config) {
                    valueUpdatesConsumer(valueFetcher(jsState, config));
                });
        },
        std::move(valueChangedHandler),
        Q_FUNC_INFO);
}

QOhosSupplier<OhosConfigurationColorMode> makeOhosConfigColorModeDataSource(
    QOhosConsumer<OhosConfigurationColorMode> valueChangedHandler)
{
    return makeOhosConfigValueDataSource<OhosConfigurationColorMode>(
        [](QtOhos::JsState &jsState) {
            return mapOhosConfigurationColorModeFromJs(
                jsState, jsState.defaultQAbilityPeer()->qAbility().eval<QNapi::Number>("context.config.colorMode"));
        },
        [](QtOhos::JsState &jsState, const QNapi::Object &config) {
            return mapOhosConfigurationColorModeFromJs(jsState, config.get<QNapi::Number>("colorMode"));
        },
        std::move(valueChangedHandler));
}

QOhosOptional<bool> mapOhosConfigurationColorModeToDarkModeFlag(OhosConfigurationColorMode colorMode)
{
    switch (colorMode) {
    case OhosConfigurationColorMode::COLOR_MODE_NOT_SET:
        return makeEmptyQOhosOptional();
    case OhosConfigurationColorMode::COLOR_MODE_LIGHT:
        return makeQOhosOptional(false);
    case OhosConfigurationColorMode::COLOR_MODE_DARK:
        return makeQOhosOptional(true);
    }

    return makeEmptyQOhosOptional();
}

std::shared_ptr<char> makeSharedNullTerminatedString(std::string str)
{
    auto sharedStrData = QtOhos::moveToSharedPtr(std::move(str) + '\0');
    return std::shared_ptr<char>(sharedStrData, &sharedStrData->front());
}

std::shared_ptr<char> makeSharedNullTerminatedString(const char *str)
{
    return makeSharedNullTerminatedString(std::string(str != nullptr ? str : ""));
}

template<typename ConvFunc>
std::string callOhFileUriConversionFunc(
    ConvFunc convFunc, const std::string &input)
{
    char *outputPtr = nullptr;
    auto outputPtrGuard = qScopeGuard(std::bind(::free, outputPtr));
    auto convFuncRetVal = convFunc(input.c_str(), input.size(), &outputPtr);

    std::string outputString;
    if (convFuncRetVal == ::FileManagement_ErrCode::ERR_OK && outputPtr != nullptr) {
        outputString = outputPtr;
    } else {
        qOhosPrintfWarning(
            "OH FileUri conversion function '%s' failed for input '%s', retval: %d",
            convFunc.name(), input.c_str(), static_cast<int>(convFuncRetVal));
    }

    return outputString;
}

std::string mapPathToOhosUriInJsThread(const std::string &path)
{
    return callOhFileUriConversionFunc(Q_OHOS_NAMED_FUNC(::OH_FileUri_GetUriFromPath), path);
}

std::string mapOhosFileUriToPathInJsThread(const std::string &ohosFileUri)
{
    return callOhFileUriConversionFunc(Q_OHOS_NAMED_FUNC(::OH_FileUri_GetPathFromUri), ohosFileUri);
}

std::shared_ptr<::FileShare_PolicyInfo> makeFileSharePolicyInfo(
    std::string uri, unsigned operationMode)
{
    auto sharedUri = makeSharedNullTerminatedString(std::move(uri));

    auto policyInfo = QtOhos::moveToSharedPtr(
        ::FileShare_PolicyInfo{
            .uri = sharedUri.get(),
            .length = static_cast<unsigned>(std::strlen(sharedUri.get())),
            .operationMode = operationMode,
        });

    return QtOhos::makeSharedPtrWithAttachedExtraData(
        policyInfo, sharedUri);
}

std::vector<std::shared_ptr<::FileShare_PolicyInfo>> convertToFileSharePolicyInfos(
    const QList<QOhosQpaFunctions::FileShare::PolicyInfo> &policyInfos)
{
    std::vector<std::shared_ptr<::FileShare_PolicyInfo>> fileSharePolicies;

    for (const auto &policyInfo : policyInfos) {
        unsigned ohosOperationModes = 0;
        for (auto operationMode : policyInfo.operationModes)
            ohosOperationModes |= static_cast<unsigned>(operationMode);
        fileSharePolicies.push_back(
            makeFileSharePolicyInfo(
                mapPathToOhosUriInJsThread(policyInfo.path.toStdString()),
                ohosOperationModes));
    }

    return fileSharePolicies;
}

std::shared_ptr<::FileShare_PolicyErrorResult> makeFileSharePolicyErrorResultFromRawStruct(
    const ::FileShare_PolicyErrorResult &inputStruct)
{
    auto sharedUri = makeSharedNullTerminatedString(inputStruct.uri);
    auto sharedMessage = makeSharedNullTerminatedString(inputStruct.message);

    auto policyErrorResult = QtOhos::moveToSharedPtr(
        ::FileShare_PolicyErrorResult{
            .uri = sharedUri.get(),
            .code = inputStruct.code,
            .message = sharedMessage.get(),
        });

    return QtOhos::makeSharedPtrWithAttachedExtraData(
        policyErrorResult,
        QtOhos::moveToSharedPtr(std::make_tuple(sharedUri, sharedMessage)));
};

std::vector<::FileShare_PolicyInfo> makePoliciesRawVectorView(
    const std::vector<std::shared_ptr<::FileShare_PolicyInfo>> &policies)
{
    std::vector<::FileShare_PolicyInfo> rawVectorView;
    for (const auto &policyPtr : policies)
        rawVectorView.push_back(*policyPtr);

    return rawVectorView;
}

template<typename PermissionActionFunc>
::FileManagement_ErrCode callFileSharePermissionActionFunc(
    PermissionActionFunc permissionActionFunc,
    const std::vector<std::shared_ptr<::FileShare_PolicyInfo>> &policies,
    std::vector<std::shared_ptr<::FileShare_PolicyErrorResult>> &outResult)
{
    auto policiesRawVectorView = makePoliciesRawVectorView(policies);
    ::FileShare_PolicyErrorResult *resultParam = nullptr;
    unsigned resultNumParam = 0;
    auto resultParamReleaseGuard = qScopeGuard(
        [&]() {
            if (resultParam != nullptr && resultNumParam != 0)
                ::OH_FileShare_ReleasePolicyErrorResult(resultParam, resultNumParam);
        });

    auto errCode = permissionActionFunc(
        policiesRawVectorView.data(), policiesRawVectorView.size(),
        &resultParam, &resultNumParam);

    outResult.clear();
    if (resultParam != nullptr) {
        for (unsigned i = 0; i < resultNumParam; ++i) {
            outResult.push_back(
                makeFileSharePolicyErrorResultFromRawStruct(resultParam[i]));
        }
    }

    return errCode;
}

::FileManagement_ErrCode fileShareCheckPersistentPermission(
    const std::vector<std::shared_ptr<::FileShare_PolicyInfo>> &policies,
    std::vector<bool> &outResult)
{
    auto policiesRawVectorView = makePoliciesRawVectorView(policies);
    bool *resultParam = nullptr;
    auto resultParamReleaseGuard = qScopeGuard(
        [&]() {
            ::free(resultParam);
        });
    unsigned resultNumParam = 0;

    auto errCode = ::OH_FileShare_CheckPersistentPermission(
        policiesRawVectorView.data(), policiesRawVectorView.size(),
        &resultParam, &resultNumParam);

    outResult.clear();
    if (resultParam != nullptr) {
        for (unsigned i = 0; i < resultNumParam; ++i)
            outResult.push_back(resultParam[i]);
    }

    return errCode;
}

::FileManagement_ErrCode fileSharePersistPermission(
    const std::vector<std::shared_ptr<::FileShare_PolicyInfo>> &policies,
    std::vector<std::shared_ptr<::FileShare_PolicyErrorResult>> &outResult)
{
    return callFileSharePermissionActionFunc(
        Q_OHOS_NAMED_FUNC(::OH_FileShare_PersistPermission),
        policies, outResult);
}

::FileManagement_ErrCode fileShareRevokePermission(
    const std::vector<std::shared_ptr<::FileShare_PolicyInfo>> &policies,
    std::vector<std::shared_ptr<::FileShare_PolicyErrorResult>> &outResult)
{
    return callFileSharePermissionActionFunc(
        Q_OHOS_NAMED_FUNC(::OH_FileShare_RevokePermission),
        policies, outResult);
}

::FileManagement_ErrCode fileShareActivatePermission(
    const std::vector<std::shared_ptr<::FileShare_PolicyInfo>> &policies,
    std::vector<std::shared_ptr<::FileShare_PolicyErrorResult>> &outResult)
{
    return callFileSharePermissionActionFunc(
        Q_OHOS_NAMED_FUNC(::OH_FileShare_ActivatePermission),
        policies, outResult);
}

::FileManagement_ErrCode fileShareDeactivatePermission(
    const std::vector<std::shared_ptr<::FileShare_PolicyInfo>> &policies,
    std::vector<std::shared_ptr<::FileShare_PolicyErrorResult>> &outResult)
{
    return callFileSharePermissionActionFunc(
        Q_OHOS_NAMED_FUNC(::OH_FileShare_DeactivatePermission),
        policies, outResult);
}

QOhosOptional<QOhosQpaFunctions::FileShare::PolicyErrorCode> tryMapFileSharePolicyErrorCode(
    ::FileShare_PolicyErrorCode errorCode)
{
    using PolicyErrorCode = QOhosQpaFunctions::FileShare::PolicyErrorCode;
    switch (errorCode) {
    case ::FileShare_PolicyErrorCode::PERSISTENCE_FORBIDDEN:
        return makeQOhosOptional(PolicyErrorCode::PERSISTENCE_FORBIDDEN);
    case ::FileShare_PolicyErrorCode::INVALID_MODE:
        return makeQOhosOptional(PolicyErrorCode::INVALID_MODE);
    case ::FileShare_PolicyErrorCode::INVALID_PATH:
        return makeQOhosOptional(PolicyErrorCode::INVALID_PATH);
    case ::FileShare_PolicyErrorCode::PERMISSION_NOT_PERSISTED:
        return makeQOhosOptional(PolicyErrorCode::PERMISSION_NOT_PERSISTED);
    }
    return makeEmptyQOhosOptional();
}

QList<QOhosQpaFunctions::FileShare::PolicyErrorResult> convertToPolicyErrorResults(
    const std::vector<std::shared_ptr<::FileShare_PolicyErrorResult>> &policyErrorResults)
{
    QList<QOhosQpaFunctions::FileShare::PolicyErrorResult> result;
    for (const auto &policyErrorResult : policyErrorResults) {
        result.push_back({
            .path = policyErrorResult->uri != nullptr
                ? QString::fromStdString(mapOhosFileUriToPathInJsThread(policyErrorResult->uri))
                : QString(),
            .error =
                tryMapFileSharePolicyErrorCode(policyErrorResult->code),
            .errorMessage = QLatin1String(
                policyErrorResult->message != nullptr ? policyErrorResult->message : ""),
        });
    }

    return result;
}

bool isSuccessErrorCode(::FileManagement_ErrCode errorCode)
{
    return errorCode == ::FileManagement_ErrCode::ERR_OK;
}

QOhosOptional<QOhosQpaFunctions::ShareKit::SharedRecord> tryConvertNapiObjectToSharedRecord(QNapi::Object record)
{
    auto tryGetOptionalStringProp = [](const QNapi::Object &object, const std::string &propName) {
        return qTransform(getOptionalProperty<QNapi::String>(object, propName), &QString::fromStdString);
    };

    auto tryGetOptionalByteArrayProp = [](const QNapi::Object &object, const std::string &propName) {
        return qTransform(
            getOptionalProperty<QNapi::TypedArrayOf<std::uint8_t>>(object, propName),
            [](const auto &napiArray) {
                return QByteArray(
                    reinterpret_cast<const char *>(napiArray.Data()),
                    napiArray.ByteLength());
            });
    };

    auto tryGetOptionalJsonObjectProp = [](const QNapi::Object &object, const std::string &propName) {
        return qTransform(
            getOptionalProperty<QNapi::Object>(object, propName),
            [](const auto &napiObject) {
                return QOhosJsEnv::fromNapiValue<QJsonObject>(napiObject);
            });
    };

    std::string utd = record.get<QNapi::String>("utd");
    auto optMimeType = utd != QOhosUdsMeta<::OH_UdsHyperlink>::udmfMetaId
        ? tryMapUtdTypeIdToMimeType(utd)
        : QOhosOptional<std::string>(QOhosShareKit::mimeTextUriList);
    if (!optMimeType.has_value()) {
        qOhosPrintfWarning(
            "%s: can't map utd '%s' to mimetype, not mapping the record",
            Q_FUNC_INFO, utd.c_str());
        return makeEmptyQOhosOptional();
    }

    auto content = tryGetOptionalStringProp(record, "content");
    auto uri = tryGetOptionalStringProp(record, "uri");
    if (!content.has_value() && !uri.has_value()) {
        qOhosPrintfWarning(
            "%s: cannot create Shared Record, content and uri properties are empty", Q_FUNC_INFO);
        return makeEmptyQOhosOptional();
    }

    return makeQOhosOptional(
        QOhosQpaFunctions::ShareKit::SharedRecord{
            .mimeType = QString::fromStdString(optMimeType.value()),
            .content = content,
            .filePath = uri,
            .title = tryGetOptionalStringProp(record, "title"),
            .label = tryGetOptionalStringProp(record, "label"),
            .description = tryGetOptionalStringProp(record, "description"),
            .thumbnail = tryGetOptionalByteArrayProp(record, "thumbnail"),
            .thumbnailFilePath = tryGetOptionalStringProp(record, "thumbnailUri"),
            .extraData = qTransform(tryGetOptionalJsonObjectProp(record, "extraData"), std::mem_fn(&QJsonObject::toVariantMap)),
        });
}

QOhosShareKit::ShareAbilityType mapShareAbilityTypeFromQpaFunctionsEnum(
    QOhosQpaFunctions::ShareKit::ShareAbilityType abilityType)
{
    switch (abilityType) {
    case QOhosQpaFunctions::ShareKit::ShareAbilityType::COPY_TO_PASTEBOARD:
        return QOhosShareKit::ShareAbilityType::COPY_TO_PASTEBOARD;
    case QOhosQpaFunctions::ShareKit::ShareAbilityType::SAVE_TO_MEDIA_ASSET:
        return QOhosShareKit::ShareAbilityType::SAVE_TO_MEDIA_ASSET;
    case QOhosQpaFunctions::ShareKit::ShareAbilityType::SAVE_AS_FILE:
        return QOhosShareKit::ShareAbilityType::SAVE_AS_FILE;
    case QOhosQpaFunctions::ShareKit::ShareAbilityType::PRINT:
        return QOhosShareKit::ShareAbilityType::PRINT;
    case QOhosQpaFunctions::ShareKit::ShareAbilityType::SAVE_TO_SUPERHUB:
        return QOhosShareKit::ShareAbilityType::SAVE_TO_SUPERHUB;
    }

    qOhosReportFatalErrorAndAbort(
        "%s: unsupported ShareAbilityType value: %d",
        Q_FUNC_INFO, static_cast<int>(abilityType));
}

QOhosOptional<std::uint32_t> tryConvertPortNameToSystemPortId(const QString &portName)
{
    constexpr const char *serialPortPrefix = "COM";
    const QString prefix = QLatin1String(serialPortPrefix);

    if (!portName.startsWith(prefix))
        return {};

    return QtOhos::tryParseStringAsUnsignedInteger<std::uint32_t>(portName.mid(prefix.length()).toStdString());
}

bool hasSerialPortAccessRightJsImpl(QtOhos::JsState &jsState, std::uint32_t serialPortId)
{
    try {
        return jsState.eval<QNapi::Boolean>("@ohos.usbManager.serial.hasSerialRight(*)", {serialPortId});
    } catch (const Napi::Error &error) {
        qOhosPrintfError(
            "%s: hasSerialRight for port %d failed with error: %s",
            Q_FUNC_INFO, serialPortId, error.what());
        return false;
    }
}

void requestSerialPortAccessRightJsImpl(
    QtOhos::JsState &jsState, std::uint32_t serialPortId, QOhosConsumer<bool> resultConsumer)
{
    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.usbManager.serial.requestSerialRight(*)", {serialPortId})
    .withContext(std::move(resultConsumer))
    .onThenWithContext(
        [](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
            bool granted = cbInfo.getFirstArg<QNapi::Boolean>(Q_FUNC_INFO);
            resultConsumer(granted);
        })
    .onCatchWithContext(
        [](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
            QtOhos::logJsCallbackError(
                cbInfo, "@ohos.usbManager.serial.requestSerialRight() failed");
            resultConsumer(false);
        });
}

void cancelSerialPortAccessRightJsImpl(QtOhos::JsState &jsState, std::uint32_t serialPortId)
{
    if (!hasSerialPortAccessRightJsImpl(jsState, serialPortId))
        return;

    try {
        jsState.eval("@ohos.usbManager.serial.cancelSerialRight(*)", {serialPortId});
    } catch (const Napi::Error &error) {
        qOhosPrintfError(
            "%s: cancelSerialRight(%u) failed with error (ignoring): %s",
            Q_FUNC_INFO, serialPortId, error.what());
    }
}

class WantInfoImpl : public QOhosQpaFunctions::WantInfo
{
public:
    WantInfoImpl(QNapi::Object want, LaunchReason launchReason);

    QJsonObject jsonObject() const override;

    QOhosOptional<QList<QOhosQpaFunctions::ShareKit::SharedRecord>> tryGetSharedDataRecords() const override;

    QOhosOptional<ContactInfo> tryGetContactInfo() const override;

    LaunchReason launchReason() const override;

private:
    struct JsScopeData
    {
        QNapi::Reference<QNapi::Object> want;
    };

    std::shared_ptr<JsScopeData> m_jsScopeData;
    QJsonObject m_jsonObject;
    LaunchReason m_launchReason;
};

WantInfoImpl::WantInfoImpl(QNapi::Object want, LaunchReason launchReason)
    : WantInfo()
    , m_jsScopeData(
        QtOhos::makeProxyWithJsThreadDeleter(
            QtOhos::moveToSharedPtr(
                JsScopeData {
                    .want = QNapi::Reference<>::makePersistentFrom(want),
                })))
    , m_jsonObject(QOhosJsEnv::fromNapiValue<QJsonObject>(want))
    , m_launchReason(launchReason)
{
}

QJsonObject WantInfoImpl::jsonObject() const
{
    return m_jsonObject;
}

QOhosOptional<QList<QOhosQpaFunctions::ShareKit::SharedRecord>> WantInfoImpl::tryGetSharedDataRecords() const
{
    using SharedRecord = QOhosQpaFunctions::ShareKit::SharedRecord;

    return QtOhos::evalInJsThreadWithPromise<QOhosOptional<QList<SharedRecord>>>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<QOhosOptional<QList<SharedRecord>>> evalPromise) {
            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            jsState.evalToPromiseOrRejectOnThrow(
                "@kit.ShareKit.systemShare.getSharedData(*)", {m_jsScopeData->want.Value()})
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QtOhos::CallbackInfo &cbInfo) {
                    QNapi::Object sharedData = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

                    auto optRecords = QNapi::getArrayElements<QList<QOhosOptional<SharedRecord>>, QNapi::Object>(
                        sharedData.call<QNapi::Array>("getRecords"), &tryConvertNapiObjectToSharedRecord);

                    QList<SharedRecord> records;
                    for (const auto &optRecord : optRecords) {
                        if (optRecord.has_value())
                            records.append(optRecord.value());
                    }

                    std::size_t unconvertedRecordsCount = optRecords.size() - records.size();
                    if (unconvertedRecordsCount != 0) {
                        qOhosPrintfWarning(
                            "%s: can't convert %zu Shared Records, ignoring them",
                            Q_FUNC_INFO, unconvertedRecordsCount);
                    }

                    thenPromise(makeQOhosOptional(records));
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QtOhos::CallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "@kit.ShareKit.systemShare.getSharedData() failed");
                    catchPromise(makeEmptyQOhosOptional());
                });
        },
        Q_FUNC_INFO);
}

QOhosOptional<QOhosQpaFunctions::WantInfo::ContactInfo> WantInfoImpl::tryGetContactInfo() const
{
    using ContactInfo = QOhosQpaFunctions::WantInfo::ContactInfo;

    return QtOhos::evalInJsThreadWithPromise<QOhosOptional<ContactInfo>>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<QOhosOptional<ContactInfo>> evalPromise) {
            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            jsState.evalToPromiseOrRejectOnThrow(
                "@kit.ShareKit.systemShare.getContactInfo(*)", {m_jsScopeData->want.Value()})
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QtOhos::CallbackInfo &cbInfo) {
                    auto contactInfoObj = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                    ContactInfo contactInfo = {
                        .contactType = QString::fromStdString(
                            contactInfoObj.get<QNapi::String>("contactType")),
                        .contactId = QString::fromStdString(
                            contactInfoObj.get<QNapi::String>("contactId")),
                    };
                    thenPromise(makeQOhosOptional(contactInfo));
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QtOhos::CallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "@kit.ShareKit.systemShare.getContactInfo() failed");
                    catchPromise(makeEmptyQOhosOptional());
                });
        },
        Q_FUNC_INFO);
}

QOhosQpaFunctions::WantInfo::LaunchReason WantInfoImpl::launchReason() const
{
    return m_launchReason;
}

class QOhosQpaFunctionsImpl : public QOhosQpaFunctions, public std::enable_shared_from_this<QOhosQpaFunctionsImpl>
{
public:
    void setWindowPrivacyMode(QObject *window, bool privacyModeEnabled) override;
    void setWindowCornerRadius(QObject *window, double radius) override;
    void tagWindowOrWidgetAsFloatWindow(QObject *windowOrWidget, bool floatWindow) override;

    void setInAppOnlyPasteboardShareOption(bool shareInAppOnly) override;
    QVariant getImageDataFromPasteboard() const override;
    QString getTextDataFromPasteboard() const override;

    void setWindowOrWidgetNativeNodeRenderFitPolicyHint(QObject *windowOrWidget, NativeNodeRenderFitPolicy renderFitPolicy) override;

    void setSurfaceBackgroundColor(QObject *windowOrWidget, const QColor &color) override;

    void setMainWindowGeometryPersistencePolicy(WindowGeometryPersistencePolicy policy) override;

    void setWindowKeepScreenOn(QObject *windowOrWidget, bool keepScreenOn) override;

    void setWindowDragResizable(QObject *windowOrWidget, bool dragResizable) override;

    QOhosOptional<double> tryGetNativeWindowId(QObject *window) override;
    QOhosOptional<double> tryGetScreenDisplayId(QObject *screenObject) override;

    void setOnContinueRequestsHandlerForAbilityInstanceWindow(
        QObject *windowObject, std::function<void(AbilityOnContinueRequest, QOhosConsumer<AbilityOnContinueResponse>)> requestsHandler) override;

    void setAbilityContinuationActive(
        QObject *optInstanceMainWindow, bool continuationActive) override;

    Q_NORETURN void restartApp(QOhosOptional<QJsonObject> want) override;

    QJsonObject getAppLaunchWant() override;
    QSharedPointer<WantInfo> getAppLaunchWantInfo() const override;

    void addNewWantConsumer(QObject *context, QOhosConsumer<QJsonObject> wantConsumer) override;
    void addNewWantConsumer(
        QObject *context, QOhosConsumer<QSharedPointer<WantInfo>> wantConsumer) override;

    void startAppProcess(
        const QString &processId, const QJsonObject &requestWant,
        const QOhosOptional<StartOptions> &optStartOptions) override;

    bool startAbility(const QJsonObject &want, const QOhosOptional<StartOptions> &options) override;

    bool startAbilityByType(const QString &appType, const QJsonObject &wantParameters) override;

    void startAbilityForResult(
        const QJsonObject &want, const QOhosOptional<StartOptions> &options,
        QObject *optInstanceMainWindow, QObject *resultConsumerQtContext,
        QOhosConsumer<QOhosOptional<AbilityResult>> resultConsumer) override;

    void setDestroyAllowedFlagForAbilityInstances(
        std::vector<QObject *> instancesMainWindows, bool destroyEnabled) override;

    void setOhosConfigDarkModeFlag(QOhosOptional<bool> darkModeFlag) override;

    QOhosSupplier<QOhosOptional<bool>> makeOhosConfigDarkModeFlagDataSource(
        QOhosConsumer<QOhosOptional<bool>> darkModeFlagChangedHandler) override;

    QOhosSupplier<double> makeOhosConfigFontSizeScaleDataSource(
        QOhosConsumer<double> valueChangedHandler) override;

    int getCurrentApplicationVersionCode() override;

    bool readOhosNoUiChildMode() override;

    void startNoUiChildProcess(QString libraryName, QStringList args) override;

    bool hasSerialPortAccessRight(const QString &portName) override;

    void requestSerialPortAccessRight(
        const QString &portName, QObject *resultConsumerQtContext,
        QOhosConsumer<std::shared_ptr<void>> resultConsumer) override;

    std::pair<bool, QList<FileShare::PolicyErrorResult>> persistPermission(
        const QList<FileShare::PolicyInfo> &policyInfos) override;

    std::pair<bool, QList<FileShare::PolicyErrorResult>> revokePermission(
        const QList<FileShare::PolicyInfo> &policyInfos) override;

    std::pair<bool, QList<FileShare::PolicyErrorResult>> activatePermission(
        const QList<FileShare::PolicyInfo> &policyInfos) override;

    std::pair<bool, QList<FileShare::PolicyErrorResult>> deactivatePermission(
        const QList<FileShare::PolicyInfo> &policyInfos) override;

    std::pair<bool, std::vector<bool>> checkPersistent(const QList<FileShare::PolicyInfo> &policyInfos) override;

    bool showFileDialogToAuthorizeFilePath(QObject *parentWindow, const QString &filePath) override;

    void setWindowBrightness(QObject *window, int brightness) override;
    void setWindowContrast(QObject *window, int contrast) override;
    void setWindowSaturation(QObject *window, int saturation) override;

    std::shared_ptr<void> shareDataUsingShareKit(
        QObject *optWindowObject, const QList<ShareKit::SharedRecord> &recordsToShare,
        const ShareKit::ShareControllerOptions &controllerOptions,
        std::function<void()> panelClosedCallback,
        QOhosConsumer<ShareKit::ShareOperationResult> optShareCompletedCallback) override;

    bool tryOpenLink(QObject *optInstanceMainWindow, const QString &link, QOhosOptional<bool> appLinkingOnly) override;

    void setAudioStreamUsageHintProperty(QObject *qObject, AudioStreamUsage usage) override;
    QOhosOptional<AudioStreamUsage> tryGetAudioStreamUsageHintProperty(QObject *qObject) override;

private:
    void processSerialPortPermissionResponse(std::uint32_t serialPortId, bool granted);

    std::unordered_map<std::uint32_t, std::vector<QOhosConsumer<std::shared_ptr<void>>>> m_pendingSerialPortsPermissionRequestsConsumers;
    std::unordered_map<std::uint32_t, std::weak_ptr<void>> m_grantedSerialPortsPermissionContexts;
};

void QOhosQpaFunctionsImpl::setWindowPrivacyMode(QObject *window, bool privacyModeEnabled)
{
    QOhosPlatformWindow::setWindowPrivacyMode(window, privacyModeEnabled);
}

void QOhosQpaFunctionsImpl::setInAppOnlyPasteboardShareOption(bool shareInAppOnly)
{
    QOhosPlatformClipboard::setInAppOnlyPasteboardShareOption(shareInAppOnly);
}

QVariant QOhosQpaFunctionsImpl::getImageDataFromPasteboard() const
{
    return QOhosPlatformIntegration::instance()->clipboard()->getPasteboardDataWithLazyFetchOrLocalIfOwner()->imageData();
}

QString QOhosQpaFunctionsImpl::getTextDataFromPasteboard() const
{
    return QOhosPlatformIntegration::instance()->clipboard()->getPasteboardDataWithLazyFetchOrLocalIfOwner()->text();
}

void QOhosQpaFunctionsImpl::setWindowCornerRadius(QObject *windowOrWidget, double radius)
{
    QOhosPlatformWindow::setWindowCornerRadius(windowOrWidget, radius);
}

void QOhosQpaFunctionsImpl::tagWindowOrWidgetAsFloatWindow(
    QObject *windowOrWidget, bool floatWindow)
{
    QOhosPlatformWindow::tagWindowOrWidgetAsFloatWindow(windowOrWidget, floatWindow);
}

void QOhosQpaFunctionsImpl::setWindowOrWidgetNativeNodeRenderFitPolicyHint(
    QObject *windowOrWidget, QOhosQpaFunctionsImpl::NativeNodeRenderFitPolicy renderFitPolicyHint)
{
    QOhosOptional<QOhosPlatformWindow::NativeNodeRenderFitPolicy> policy;
    switch (renderFitPolicyHint) {
    case QOhosQpaFunctions::NativeNodeRenderFitPolicy::TopLeft:
        policy = QOhosPlatformWindow::NativeNodeRenderFitPolicy::TopLeft;
        break;
    case QOhosQpaFunctions::NativeNodeRenderFitPolicy::Fill:
        policy = QOhosPlatformWindow::NativeNodeRenderFitPolicy::Fill;
        break;
    }

    if (policy.has_value()) {
        QOhosPlatformWindow::setWindowOrWidgetNativeNodeRenderFitPolicyHint(windowOrWidget, policy.value());
    } else {
        qOhosReportFatalErrorAndAbort(
            "%s: Failed to convert render fit policy hint to QOhosPlatformWindow enum",
            Q_FUNC_INFO);
    }
}

void QOhosQpaFunctionsImpl::setSurfaceBackgroundColor(QObject *windowOrWidget, const QColor &color)
{
    QOhosPlatformWindow::setSurfaceBackgroundColor(windowOrWidget, color);
}

void QOhosQpaFunctionsImpl::setMainWindowGeometryPersistencePolicy(
    WindowGeometryPersistencePolicy geometryPolicyHint)
{
    QOhosOptional<QOhosPlatformIntegration::WindowGeometryPersistencePolicy> policy;
    switch (geometryPolicyHint) {
    case QOhosQpaFunctions::WindowGeometryPersistencePolicy::Disabled:
        policy = QOhosPlatformIntegration::WindowGeometryPersistencePolicy::Disabled;
        break;
    case QOhosQpaFunctions::WindowGeometryPersistencePolicy::Enabled:
        policy = QOhosPlatformIntegration::WindowGeometryPersistencePolicy::Enabled;
        break;
    case QOhosQpaFunctions::WindowGeometryPersistencePolicy::FollowSystemSetting:
        policy = QOhosPlatformIntegration::WindowGeometryPersistencePolicy::FollowSystemSetting;
        break;
    }

    if (policy.has_value()) {
        QOhosPlatformIntegration::setMainWindowGeometryPersistencePolicy(policy.value());
    } else {
        qOhosReportFatalErrorAndAbort(
            "%s: Failed to convert persistence geometry policy hint to QOhosPlatformIntegration enum",
            Q_FUNC_INFO);
    }
}

void QOhosQpaFunctionsImpl::setWindowKeepScreenOn(QObject *windowOrWidget, bool keepScreenOn)
{
    QOhosPlatformWindow::setWindowKeepScreenOn(windowOrWidget, keepScreenOn);
}

void QOhosQpaFunctionsImpl::setWindowDragResizable(QObject *windowOrWidget, bool dragResizable)
{
    QOhosPlatformWindow::setWindowDragResizable(windowOrWidget, dragResizable);
}

QOhosOptional<double> QOhosQpaFunctionsImpl::tryGetNativeWindowId(QObject *window)
{
    auto *qWindow = qobject_cast<QWindow *>(window);
    if (qWindow == nullptr)
        return makeEmptyQOhosOptional();

    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(qWindow);
    if (platformWindow == nullptr)
        return makeEmptyQOhosOptional();

    auto internalId = platformWindow->internalWindowId();
    auto jsWinId = QWindowProxyRegistry::instance().tryMapInternalWindowIdToJsWindowId(internalId);
    if (!jsWinId.has_value())
        return makeEmptyQOhosOptional();

    qOhosPrintfInfo(
        "PlatformWindow WIID: %s is returning JsWindowId: %f to the user",
        qPrintable(internalId.toString()), jsWinId.value().value());

    return makeQOhosOptional(jsWinId.value().value());
}

QOhosOptional<double> QOhosQpaFunctionsImpl::tryGetScreenDisplayId(QObject *screenObject)
{
    auto *qScreen = qobject_cast<QScreen *>(screenObject);
    if (qScreen == nullptr) {
        qOhosPrintfWarning("%s: screenObject argument is not a QScreen", Q_FUNC_INFO);
        return makeEmptyQOhosOptional();
    }
    auto *ohosPlatformScreen = static_cast<QOhosPlatformScreen *>(qScreen->handle());

    return ohosPlatformScreen != nullptr
        ? makeQOhosOptional(ohosPlatformScreen->displayInfo().id.value())
        : makeEmptyQOhosOptional();
}

void QOhosQpaFunctionsImpl::setOnContinueRequestsHandlerForAbilityInstanceWindow(
    QObject *windowObject, std::function<void(AbilityOnContinueRequest, QOhosConsumer<AbilityOnContinueResponse>)> requestsHandler)
{
    auto *qWindow = qobject_cast<QWindow *>(windowObject);
    if (qWindow == nullptr)
        qOhosReportFatalErrorAndAbort("%s: windowObject argument is null or not a QWindow", Q_FUNC_INFO);

    auto qWindowRef = QObjectThreadSafeRef(qWindow);
    auto sharedRequestsHandler = moveToSharedPtr(std::move(requestsHandler));

    struct JsResultContext
    {
        QNapi::Reference<QNapi::Object> wantParamsReference;
        QOhosConsumer<JsState &, QOhosAbilityOnContinueResult> resultConsumer;
    };

    QtOhos::runInJsThreadAndWait(
        [&](JsState &jsState) {
            auto uiAbilityPeer = QUiAbilityPeer::tryCastFromQAbilityPeerOrNull(
                jsState.tryGetQAbilityPeerByQWindow(qWindowRef));
            if (!uiAbilityPeer) {
                qOhosPrintfError(
                    "%s: no QUiAbilityPeer for window %s, handler not set",
                    Q_FUNC_INFO, qWindowRef.refName().c_str());
                return;
            }

            uiAbilityPeer->setOnContinueRequestsHandler(
                [sharedRequestsHandler](JsState &, QNapi::Object wantParamsObj, auto resultConsumer) {
                    int sourceVersionCode = wantParamsObj.get<QNapi::Number>("version");
                    auto jsResultContext = makeProxyWithJsThreadDeleter(std::make_shared<JsResultContext>());
                    jsResultContext->wantParamsReference = QNapi::Reference<>::makePersistentFrom(wantParamsObj);
                    jsResultContext->resultConsumer = std::move(resultConsumer);
                    QtOhos::invokeInQtThread(
                        [sharedRequestsHandler, sourceVersionCode, jsResultContext]() {
                            (*sharedRequestsHandler)(
                                AbilityOnContinueRequest{
                                    .sourceApplicationVersionCode = sourceVersionCode,
                                },
                                [jsResultContext](AbilityOnContinueResponse qtResponse) {
                                    QtOhos::invokeInJsThread(
                                        [jsResultContext, qtResponse](JsState &jsState) {
                                            if (qtResponse.status == AbilityOnContinueResponseStatus::Agree) {
                                                auto wantParamsObj = jsResultContext->wantParamsReference.Value();
                                                auto newWantParamsIter = qtResponse.wantObjectParams.constKeyValueBegin();
                                                while (newWantParamsIter != qtResponse.wantObjectParams.constKeyValueEnd()) {
                                                    wantParamsObj.set(
                                                        newWantParamsIter->first.toStdString(),
                                                        newWantParamsIter->second.toStdString());
                                                    ++newWantParamsIter;
                                                }
                                                if (qtResponse.exitAppOnSourceDeviceAfterMigration.has_value()) {
                                                    wantParamsObj.set(
                                                        jsState.eval<QNapi::String>(
                                                            "@ohos.app.ability.wantConstant.Params.SUPPORT_CONTINUE_SOURCE_EXIT_KEY"),
                                                        qtResponse.exitAppOnSourceDeviceAfterMigration.value());
                                                }
                                            }
                                            auto ohosResult = tryMapAbilityOnContinueResponseStatusToOhos(qtResponse.status);
                                            if (!ohosResult.has_value()) {
                                                qOhosPrintfWarning(
                                                    "%s: got illegal status (%d) from request handler, rejecting",
                                                    Q_FUNC_INFO, static_cast<int>(qtResponse.status));
                                            }
                                            jsResultContext->resultConsumer(
                                                jsState, ohosResult.value_or(QOhosAbilityOnContinueResult::REJECT));
                                        });
                                });
                        });
                });
        },
        Q_FUNC_INFO);
}

void QOhosQpaFunctionsImpl::setAbilityContinuationActive(
    QObject *optInstanceMainWindow, bool continuationActive)
{
    using ContinueState = enums::ohos::app::ability::AbilityConstant::ContinueState;

    auto optInstanceMainWindowRef =
        optInstanceMainWindow != nullptr
           ? makeQOhosOptional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
           : makeEmptyQOhosOptional();

    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](JsState &jsState, QOhosTaskPromise<> taskPromise) {
            auto optAbilityPeer = tryMapOptMainWindowToAbilityPeer(jsState, optInstanceMainWindowRef);
            if (!optAbilityPeer) {
                taskPromise();
                return;
            }

            auto continueState = continuationActive ? ContinueState::ACTIVE : ContinueState::INACTIVE;
            optAbilityPeer->qAbility().evalToPromiseOrRejectOnThrow(
                "context.setMissionContinueState(*)", {jsState.mapOhosEnumToJs(continueState)})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setMissionContinueState()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

Q_NORETURN void QOhosQpaFunctionsImpl::restartApp(QOhosOptional<QJsonObject> want)
{
    QtOhos::runInJsThreadAndWait(
        [&](JsState &jsState) {
            auto napiWant = want.has_value()
                ? QNapi::checkedCast<QNapi::Object>(QOhosJsEnv::toNapiValue(jsState.env(), want.value()))
                : jsState.appLaunchWant();

            constexpr auto sleepTimeBeforeRetry = 3s;

            unsigned remainingTries = 3;

            while (true) {
                --remainingTries;

                qOhosPrintfInfo(
                    "%s: calling restartApp() using Want: %s",
                    Q_FUNC_INFO, QNapi::toJsonString(napiWant).c_str());

                try {
                    jsState.defaultQAbilityPeer()->qAbility().call(
                        "context.getApplicationContext().restartApp", {napiWant});

                    qOhosPrintfWarning("%s: restartApp() call unexpectedly returned, killing self", Q_FUNC_INFO);
                    killCurrentProcess();
                } catch (const Napi::Error &error) {
                    constexpr std::uint32_t restartTooFrequentlyErrorCode = 16000064;

                    auto errorCode = QtOhos::tryGetCodeFromJsBusinessError(error);

                    if (errorCode == restartTooFrequentlyErrorCode && remainingTries != 0) {
                        qOhosPrintfWarning(
                            "%s: restartApp() returned with error %u, sleeping before retry",
                            Q_FUNC_INFO, restartTooFrequentlyErrorCode);

                        std::this_thread::sleep_for(sleepTimeBeforeRetry);
                    } else {
                        auto errorCodeStr = errorCode.has_value()
                            ? std::to_string(errorCode.value())
                            : "?";
                        qOhosPrintfWarning(
                            "%s: restartApp() returned with error %s, killing self",
                            Q_FUNC_INFO, errorCodeStr.c_str());

                        killCurrentProcess();
                    }
                }
            }
        },
        Q_FUNC_INFO);

    qOhosReportFatalErrorAndAbort("%s: unexpected return from the JS thread call", Q_FUNC_INFO);
}

QJsonObject QOhosQpaFunctionsImpl::getAppLaunchWant()
{
    return getAppLaunchWantInfo()->jsonObject();
}

QSharedPointer<QOhosQpaFunctions::WantInfo> QOhosQpaFunctionsImpl::getAppLaunchWantInfo() const
{
    return QtOhos::evalInJsThread(
        [](auto &jsState) {
            auto optAppLaunchReason = qTransform(
                jsState.optAppLaunchParam(),
                [&](QNapi::Object appLaunchParam) {
                    return mapJsLaunchReasonToWantInfoEnumWithFallback(
                        jsState, appLaunchParam.get<QNapi::Number>("launchReason"));
                });
            auto appLaunchReason = optAppLaunchReason.value_or(QOhosQpaFunctions::WantInfo::LaunchReason::UNKNOWN);
            return QSharedPointer<WantInfoImpl>::create(jsState.appLaunchWant(), appLaunchReason);
        },
        Q_FUNC_INFO);
}

void QOhosQpaFunctionsImpl::addNewWantConsumer(QObject *context, QOhosConsumer<QJsonObject> wantConsumer)
{
    auto sharedWantConsumer = QtOhos::moveToSharedPtr(std::move(wantConsumer));
    addNewWantConsumer(
        context,
        [sharedWantConsumer](QSharedPointer<WantInfo> wantInfo) {
            (*sharedWantConsumer)(wantInfo->jsonObject());
        });
}

void QOhosQpaFunctionsImpl::addNewWantConsumer(
    QObject *context, QOhosConsumer<QSharedPointer<WantInfo>> wantConsumer)
{
    auto contextRef = QtOhos::makeQThreadSafeRef(context);
    auto sharedWantConsumer = QtOhos::moveToSharedPtr(std::move(wantConsumer));
    QtOhos::runInJsThreadAndWait(
        [&](auto &jsState) {
            jsState.addNewWantConsumer(
                [contextRef, sharedWantConsumer](QtOhos::JsState &jsState, QNapi::Object napiWant, QNapi::Object launchParam) {
                    auto launchReason = mapJsLaunchReasonToWantInfoEnumWithFallback(
                        jsState, launchParam.get<QNapi::Number>("launchReason"));
                    auto wantInfo = QSharedPointer<WantInfoImpl>::create(napiWant, launchReason);
                    contextRef.visitInQtThreadIfAlive(
                        [sharedWantConsumer, wantInfo](auto &) {
                            (*sharedWantConsumer)(wantInfo);
                        });
                });
        },
        Q_FUNC_INFO);
}

void QOhosQpaFunctionsImpl::startAppProcess(
    const QString &processId, const QJsonObject &requestWant,
    const QOhosOptional<StartOptions> &optStartOptions)
{
    QtOhos::invokeInJsThreadAndWaitForContinue(
        [&](auto &jsState, QOhosTaskPromise<> taskPromise) {
            auto startOptions = optStartOptions.has_value()
                ? convertStartOptionsToNapiObject(jsState, optStartOptions.value())
                : QNapi::Object();

            auto sharedTaskPromise = QtOhos::moveToSharedPtr(std::move(taskPromise).makeChained(Q_FUNC_INFO));
            jsState.startAppProcess(
                processId.toStdString(),
                QNapi::checkedCast<QNapi::Object>(
                    QOhosJsEnv::toNapiValue(jsState.env(), requestWant)),
                startOptions,
                [sharedTaskPromise](QtOhos::JsState &) {
                    (*sharedTaskPromise)();
                });
    },
    Q_FUNC_INFO);
}

bool QOhosQpaFunctionsImpl::startAbility(const QJsonObject &want, const QOhosOptional<QOhosQpaFunctions::StartOptions> &options)
{
    return QtOhos::evalInJsThread(
        [&](auto &jsState) {
            auto mainUiAbility = jsState.defaultQAbilityPeer()->qAbility();
            if (mainUiAbility.IsEmpty())
                return false;

            auto arguments = std::vector<QNapi::ValueWrapper>{QOhosJsEnv::toNapiValue(jsState.env(), want)};
            if (options.has_value())
                arguments.push_back(convertStartOptionsToNapiObject(jsState, options.value()));

            mainUiAbility.call("context.startAbility", arguments);

            // FIXME:
            // * there should be error code taken from a call to JS `startAbility` function
            // * error code should be checked and provided to the returned `operationStatus`
            return true;
        },
        Q_FUNC_INFO);
}

bool QOhosQpaFunctionsImpl::startAbilityByType(const QString &appType, const QJsonObject &wantParameters)
{
    // The call result of "context.startAbilityByType" will be synced and returned.
    // However, the started ability result won't be synced here.
    return QtOhos::evalInJsThreadWithPromise<bool>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<bool> evalPromise) {
            auto qAbility = jsState.defaultQAbilityPeer()->qAbility();
            if (qAbility.IsEmpty()) {
                evalPromise(false);
                return;
            }

            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            qAbility.evalToPromiseOrRejectOnThrow(
                "context.startAbilityByType(*)",
                {
                    appType.toStdString(),
                    QOhosJsEnv::toNapiValue(jsState.env(), wantParameters),
                    QNapi::makeObject(
                        jsState.env(),
                        {
                            {
                                "onResult",
                                [](const QtOhos::CallbackInfo&) {
                                    qOhosPrintfDebug("startAbilityByType: onResult called");
                                }
                            },
                            {
                                "onError",
                                [](const QtOhos::CallbackInfo &cbInfo) {
                                    QtOhos::logJsCallbackError(cbInfo, "startAbilityByType: onError called");
                                }
                            }
                        })
                })
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QtOhos::CallbackInfo &) {
                    thenPromise(true);
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QtOhos::CallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "startAbilityByType: failed");
                    catchPromise(false);
                });
        },
        Q_FUNC_INFO);
}

void QOhosQpaFunctionsImpl::startAbilityForResult(
    const QJsonObject &want, const QOhosOptional<StartOptions> &options,
    QObject *optInstanceMainWindow, QObject *resultConsumerQtContext,
    QOhosConsumer<QOhosOptional<AbilityResult>> resultConsumer)
{
    struct Context
    {
        QtOhos::QObjectThreadSafeRef resultConsumerQtContextRef;
        QOhosConsumer<QOhosOptional<AbilityResult>> resultConsumer;
    };

    auto context = QtOhos::moveToSharedPtr(
        Context{
            .resultConsumerQtContextRef = QtOhos::QObjectThreadSafeRef(resultConsumerQtContext),
            .resultConsumer = std::move(resultConsumer),
        });

    auto optInstanceMainWindowRef =
        optInstanceMainWindow != nullptr
           ? makeQOhosOptional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
           : makeEmptyQOhosOptional();

    QtOhos::invokeInJsThread(
        [context, want, options, optInstanceMainWindowRef](QtOhos::JsState &jsState) {
            auto optAbilityPeer = tryMapOptMainWindowToAbilityPeer(jsState, optInstanceMainWindowRef);
            if (!optAbilityPeer) {
                context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                    [context](auto &) {
                        context->resultConsumer({});
                    });
                return;
            }

            auto arguments = std::vector<QNapi::ValueWrapper>{QOhosJsEnv::toNapiValue(jsState.env(), want)};
            if (options.has_value())
                arguments.push_back(convertStartOptionsToNapiObject(jsState, options.value()));

            optAbilityPeer->qAbility().evalToPromiseOrRejectOnThrow("context.startAbilityForResult(*)", arguments)
            .onThen(
                [context](const QtOhos::CallbackInfo &cbInfo) {
                    QNapi::Object abilityResult = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                    int resultCode = abilityResult.get<QNapi::Number>("resultCode");

                    auto wantOrEmpty = QNapi::getOptionalPropOrEmpty<QNapi::Object>(abilityResult, "want");
                    auto jsonWant = !wantOrEmpty.IsEmpty()
                        ? QOhosOptional<QJsonObject>(QOhosJsEnv::fromNapiValue<QJsonObject>(wantOrEmpty))
                        : makeEmptyQOhosOptional();

                    context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                        [context, resultCode, jsonWant](auto &) {
                            constexpr int startedAbilityErrorResultCode = -1;
                            context->resultConsumer(
                                resultCode != startedAbilityErrorResultCode
                                    ? QOhosOptional<AbilityResult>({resultCode, jsonWant})
                                    : makeEmptyQOhosOptional());
                        });
                })
            .onCatch(
                [context](const QtOhos::CallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "Got error from startAbilityForResult()");
                    context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                        [context](auto &) {
                            context->resultConsumer({});
                        });
                });
        });
}

void QOhosQpaFunctionsImpl::setDestroyAllowedFlagForAbilityInstances(
    std::vector<QObject *> instancesMainWindows, bool destroyEnabled)
{
    std::vector<QtOhos::QObjectThreadSafeRef> instancesMainWindowsRefs;
    for (auto *instanceMainWindow : instancesMainWindows)
        instancesMainWindowsRefs.emplace_back(instanceMainWindow);

    QtOhos::runInJsThreadAndWait(
        [&](auto &jsState) {
            for (const auto &instanceMainWindowRef : instancesMainWindowsRefs) {
                auto abilityPeer = jsState.tryGetQAbilityPeerByQWindow(instanceMainWindowRef);
                if (abilityPeer)
                    abilityPeer->destroyAllowedFlag()->store(destroyEnabled);
            }
        },
        Q_FUNC_INFO);
}

void QOhosQpaFunctionsImpl::setOhosConfigDarkModeFlag(QOhosOptional<bool> darkModeFlag)
{
    setOhosConfigColorMode(
        darkModeFlag.has_value()
            ? darkModeFlag.value()
                ? OhosConfigurationColorMode::COLOR_MODE_DARK
                : OhosConfigurationColorMode::COLOR_MODE_LIGHT
            : OhosConfigurationColorMode::COLOR_MODE_NOT_SET);
}

QOhosSupplier<QOhosOptional<bool>> QOhosQpaFunctionsImpl::makeOhosConfigDarkModeFlagDataSource(
    QOhosConsumer<QOhosOptional<bool>> darkModeFlagChangedHandler)
{
    auto colorModeDataSource = makeOhosConfigColorModeDataSource(
        [darkModeFlagChangedHandler = std::move(darkModeFlagChangedHandler)](OhosConfigurationColorMode newColorMode) {
            darkModeFlagChangedHandler(
                mapOhosConfigurationColorModeToDarkModeFlag(newColorMode));
        });
    return [colorModeDataSource = std::move(colorModeDataSource)]() {
        return mapOhosConfigurationColorModeToDarkModeFlag(colorModeDataSource());
    };
}

QOhosSupplier<double> QOhosQpaFunctionsImpl::makeOhosConfigFontSizeScaleDataSource(
    QOhosConsumer<double> valueChangedHandler)
{
    auto initFontSizeScale = QOhosSettings::fontSizeScale();
    return makeOhosConfigValueDataSource<double>(
        [initFontSizeScale](QtOhos::JsState &) {
            return initFontSizeScale;
        },
        [](QtOhos::JsState &, const QNapi::Object &config) {
            return config.get<QNapi::Number>("fontSizeScale").DoubleValue();
        },
        std::move(valueChangedHandler));
}

int QOhosQpaFunctionsImpl::getCurrentApplicationVersionCode()
{
    return QtOhos::evalInJsThread(
        [](QtOhos::JsState &jsState) {
            auto applicationInfoFlag = jsState.eval<QNapi::Number>(
                "@ohos.bundle.bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION");
            auto bundleInfo = jsState.eval<QNapi::Object>(
                "@ohos.bundle.bundleManager.getBundleInfoForSelfSync(*)", {applicationInfoFlag});
            int versionCode = bundleInfo.get<QNapi::Number>("versionCode");

            return versionCode;
        },
        Q_FUNC_INFO);
}

bool QOhosQpaFunctionsImpl::readOhosNoUiChildMode()
{
    return QtOhos::evalInJsThread(
        [&](auto &jsState) {
            return jsState.defaultQAbilityPeer()->instanceId().empty();
        },
        Q_FUNC_INFO);
}

void QOhosQpaFunctionsImpl::startNoUiChildProcess(QString libraryName, QStringList args)
{
    QtOhos::runInJsThreadAndWait(
        [&](auto &jsState) {
            std::vector<std::string> argsVector;
            std::transform(
                args.begin(), args.end(), std::back_inserter(argsVector),
                std::mem_fn(&QString::toStdString));
            jsState.startNoUiChildProcess(libraryName.toStdString(), argsVector);
        },
        Q_FUNC_INFO);
}

bool QOhosQpaFunctionsImpl::hasSerialPortAccessRight(const QString &portName)
{
    const auto optSerialPortId = tryConvertPortNameToSystemPortId(portName);
    if (!optSerialPortId.has_value()) {
        qOhosPrintfError(
            "%s: cannot convert serial port name '%s' to port id.",
            Q_FUNC_INFO, portName.toStdString().c_str());
        return false;
    }

    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &jsState) {
            return hasSerialPortAccessRightJsImpl(jsState, optSerialPortId.value());
        },
        Q_FUNC_INFO);
}

void QOhosQpaFunctionsImpl::requestSerialPortAccessRight(
    const QString &portName, QObject *resultConsumerQtContext,
    QOhosConsumer<std::shared_ptr<void>> resultConsumer)
{
    auto resultConsumerQtContextRef = QtOhos::makeQThreadSafeRef(resultConsumerQtContext);
    auto asyncResultConsumer = [resultConsumerQtContextRef, resultConsumer = std::move(resultConsumer)](std::shared_ptr<void> permissionContext) {
        resultConsumerQtContextRef.visitInQtThreadIfAlive(
            [resultConsumer = std::move(resultConsumer), permissionContext](auto &resultConsumerQtContext) {
                QMetaObject::invokeMethod(
                    &resultConsumerQtContext,
                    [resultConsumer = std::move(resultConsumer), permissionContext]() {
                        resultConsumer(permissionContext);
                    },
                    Qt::QueuedConnection);
            });
    };

    const auto optSerialPortId = tryConvertPortNameToSystemPortId(portName);
    if (!optSerialPortId.has_value()) {
        qOhosPrintfError(
            "%s: cannot convert serial port name '%s' to port id.",
            Q_FUNC_INFO, portName.toStdString().c_str());

        asyncResultConsumer(nullptr);
        return;
    }

    QtOhos::invokeInQtThread(
        [serialPortId = optSerialPortId.value(), weakSelf = QtOhos::makeWeakPtr(shared_from_this()), asyncResultConsumer = std::move(asyncResultConsumer)]() {
            auto self = weakSelf.lock();
            if (!self)
                return;

            auto alreadyGrantedPermissionContextIt =
                self->m_grantedSerialPortsPermissionContexts.find(serialPortId);
            auto optAlreadyGrantedPermissionContext =
                alreadyGrantedPermissionContextIt != self->m_grantedSerialPortsPermissionContexts.end()
                    ? alreadyGrantedPermissionContextIt->second.lock()
                    : nullptr;

            if (optAlreadyGrantedPermissionContext) {
                asyncResultConsumer(optAlreadyGrantedPermissionContext);
                return;
            }

            self->m_pendingSerialPortsPermissionRequestsConsumers[serialPortId].push_back(
                std::move(asyncResultConsumer));

            if (self->m_pendingSerialPortsPermissionRequestsConsumers[serialPortId].size() == 1) {
                QtOhos::invokeInJsThread(
                    [serialPortId, weakSelf](QtOhos::JsState &jsState) {
                        requestSerialPortAccessRightJsImpl(
                            jsState,
                            serialPortId,
                            [serialPortId, weakSelf](bool granted) {
                                QtOhos::invokeInQtThread(
                                    [serialPortId, weakSelf, granted]() {
                                        auto self = weakSelf.lock();
                                        if (self)
                                            self->processSerialPortPermissionResponse(serialPortId, granted);
                                    });
                        });
                    });
            }
        });
}

std::pair<bool, QList<QOhosQpaFunctions::FileShare::PolicyErrorResult>> QOhosQpaFunctionsImpl::persistPermission(
    const QList<FileShare::PolicyInfo> &policyInfos)
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            std::vector<std::shared_ptr<::FileShare_PolicyErrorResult>> outResults;
            auto retCode = fileSharePersistPermission(
                convertToFileSharePolicyInfos(policyInfos), outResults);

            return std::make_pair(isSuccessErrorCode(retCode), convertToPolicyErrorResults(outResults));
        },
        Q_FUNC_INFO);
}

std::pair<bool, QList<QOhosQpaFunctions::FileShare::PolicyErrorResult>> QOhosQpaFunctionsImpl::revokePermission(
    const QList<FileShare::PolicyInfo> &policyInfos)
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            std::vector<std::shared_ptr<::FileShare_PolicyErrorResult>> outResults;
            auto retCode = fileShareRevokePermission(
                convertToFileSharePolicyInfos(policyInfos), outResults);

            return std::make_pair(isSuccessErrorCode(retCode), convertToPolicyErrorResults(outResults));
        },
        Q_FUNC_INFO);
}

std::pair<bool, QList<QOhosQpaFunctions::FileShare::PolicyErrorResult>> QOhosQpaFunctionsImpl::activatePermission(
    const QList<FileShare::PolicyInfo> &policyInfos)
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            std::vector<std::shared_ptr<::FileShare_PolicyErrorResult>> outResults;
            auto retCode = fileShareActivatePermission(
                convertToFileSharePolicyInfos(policyInfos), outResults);

            return std::make_pair(isSuccessErrorCode(retCode), convertToPolicyErrorResults(outResults));
        },
        Q_FUNC_INFO);
}

std::pair<bool, QList<QOhosQpaFunctions::FileShare::PolicyErrorResult>> QOhosQpaFunctionsImpl::deactivatePermission(
    const QList<FileShare::PolicyInfo> &policyInfos)
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            std::vector<std::shared_ptr<::FileShare_PolicyErrorResult>> outResults;
            auto retCode = fileShareDeactivatePermission(
                convertToFileSharePolicyInfos(policyInfos), outResults);

            return std::make_pair(isSuccessErrorCode(retCode), convertToPolicyErrorResults(outResults));
        },
        Q_FUNC_INFO);
}

std::pair<bool, std::vector<bool>> QOhosQpaFunctionsImpl::checkPersistent(
    const QList<FileShare::PolicyInfo> &policyInfos)
{
    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &) {
            std::vector<bool> outResults;
            auto retCode = fileShareCheckPersistentPermission(
                convertToFileSharePolicyInfos(policyInfos), outResults);

            return std::make_pair(isSuccessErrorCode(retCode), outResults);
        },
        Q_FUNC_INFO);
}

bool QOhosQpaFunctionsImpl::showFileDialogToAuthorizeFilePath(QObject *parentWindow, const QString &filePath)
{
    auto *qWindow = qobject_cast<QWindow *>(parentWindow);
    if (qWindow == nullptr)
        qOhosReportFatalErrorAndAbort("%s: window argument is null or not a QWindow", Q_FUNC_INFO);

    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(qWindow);
    if (platformWindow == nullptr)
        qOhosReportFatalErrorAndAbort("%s: failed to get platform window", Q_FUNC_INFO);

    auto eventLoop = std::make_shared<QEventLoop>();
    auto filePathAuthorized = std::make_shared<bool>(false);

    QOhosWindowManager::showFileDialogAuthorization(
       platformWindow->internalWindowId(), filePath,
       [filePathAuthorized, eventLoop](bool result) {
            *filePathAuthorized = result;
            eventLoop->quit();
       });

    eventLoop->exec();

    return *filePathAuthorized;
}

void QOhosQpaFunctionsImpl::setWindowBrightness(QObject *window, int brightness)
{
    auto *qWindow = qobject_cast<QWindow *>(window);
    if (qWindow == nullptr)
        qOhosReportFatalErrorAndAbort("%s: window argument is null or not a QWindow", Q_FUNC_INFO);

    QOhosPlatformWindow::setBrightness(qWindow, brightness);
}

void QOhosQpaFunctionsImpl::setWindowContrast(QObject *window, int contrast)
{
    auto *qWindow = qobject_cast<QWindow *>(window);
    if (qWindow == nullptr)
        qOhosReportFatalErrorAndAbort("%s: window argument is null or not a QWindow", Q_FUNC_INFO);

    QOhosPlatformWindow::setContrast(qWindow, contrast);
}

void QOhosQpaFunctionsImpl::setWindowSaturation(QObject *window, int saturation)
{
    auto *qWindow = qobject_cast<QWindow *>(window);
    if (qWindow == nullptr)
        qOhosReportFatalErrorAndAbort("%s: window argument is null or not a QWindow", Q_FUNC_INFO);

    QOhosPlatformWindow::setSaturation(qWindow, saturation);
}

std::shared_ptr<void> QOhosQpaFunctionsImpl::shareDataUsingShareKit(
    QObject *optWindowObject, const QList<ShareKit::SharedRecord> &recordsToShare,
    const ShareKit::ShareControllerOptions &controllerOptions,
    std::function<void()> panelClosedCallback,
    QOhosConsumer<ShareKit::ShareOperationResult> optShareCompletedCallback)
{
    auto *optQWindow = qobject_cast<QWindow *>(optWindowObject);
    if (optWindowObject != nullptr && optQWindow == nullptr)
        qOhosReportFatalErrorAndAbort("%s: window argument is not a QWindow", Q_FUNC_INFO);

    std::vector<QOhosShareKit::SharedRecord> shareKitRecords;
    for (const auto &record : recordsToShare) {
        shareKitRecords.push_back(
            QOhosShareKit::SharedRecord{
                .mimeType = record.mimeType.toStdString(),
                .content = qTransform(record.content, std::mem_fn(&QString::toStdString)),
                .filePath = qTransform(record.filePath, std::mem_fn(&QString::toStdString)),
                .title = qTransform(record.title, std::mem_fn(&QString::toStdString)),
                .label = qTransform(record.label, std::mem_fn(&QString::toStdString)),
                .description = qTransform(record.description, std::mem_fn(&QString::toStdString)),
                .thumbnail = record.thumbnail,
                .thumbnailFilePath = qTransform(record.thumbnailFilePath, std::mem_fn(&QString::toStdString)),
                .extraData = record.extraData,
            });
    }

    auto shareKitControllerOptions = QOhosShareKit::ControllerOptions{
        .anchor = qTransform(
            controllerOptions.anchorOffset,
            [&](auto anchorOffset) {
                return QOhosShareKit::ShareControllerAnchor{
                    .windowOffset = anchorOffset,
                    .size = controllerOptions.anchorSize,
                };
            }),
        .selectionMode = qTransform(
            controllerOptions.useSingleSelectionMode,
            [](auto singleSelectionMode) {
                return singleSelectionMode
                    ? QOhosShareKit::SelectionMode::SINGLE
                    : QOhosShareKit::SelectionMode::BATCH;
            }),
        .previewMode = qTransform(
            controllerOptions.useDefaultPreviewMode,
            [](auto defaultPreviewMode) {
                return defaultPreviewMode
                    ? QOhosShareKit::SharePreviewMode::DEFAULT
                    : QOhosShareKit::SharePreviewMode::DETAIL;
            }),
        .excludedAbilities = qTransform(
            controllerOptions.excludedAbilities,
            [](const auto &excludedAbilities) {
                std::vector<QOhosShareKit::ShareAbilityType> outExcludedAbilities;
                for (auto excludedAbilityType : excludedAbilities) {
                    outExcludedAbilities.push_back(
                        mapShareAbilityTypeFromQpaFunctionsEnum(excludedAbilityType));
                }
                return outExcludedAbilities;
            }),
    };

    auto shareCompletedCallback = optShareCompletedCallback
        ? std::move(optShareCompletedCallback)
        : makeQOhosNoOpConsumer();

    return QOhosShareKit::shareData(
        optQWindow, shareKitRecords, shareKitControllerOptions, std::move(panelClosedCallback),
        [shareCompletedCallback = std::move(shareCompletedCallback)](auto shareOperationResult) {
            shareCompletedCallback(
                ShareKit::ShareOperationResult{
                    .targetAbilityName = QString::fromStdString(shareOperationResult.targetAbilityName),
                });
        });
}

bool QOhosQpaFunctionsImpl::tryOpenLink(QObject *optInstanceMainWindow, const QString &link, QOhosOptional<bool> appLinkingOnly)
{
    if (optInstanceMainWindow != nullptr && qobject_cast<QWindow *>(optInstanceMainWindow) == nullptr)
        qOhosReportFatalErrorAndAbort("%s: the main window argument is not a QWindow", Q_FUNC_INFO);

    auto optInstanceMainWindowRef = optInstanceMainWindow != nullptr
        ? makeQOhosOptional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
        : makeEmptyQOhosOptional();

    return QtOhos::evalInJsThreadWithPromise<bool>(
        [&](QtOhos::JsState &jsState, QOhosTaskPromise<bool> evalPromise) {
            auto optAbilityPeer = tryMapOptMainWindowToAbilityPeer(jsState, optInstanceMainWindowRef);
            if (!optAbilityPeer) {
                evalPromise(false);
                return;
            }

            std::vector<std::pair<std::string, QNapi::ValueWrapper>> openLinkOptions;
            if (appLinkingOnly.has_value())
                openLinkOptions.emplace_back("appLinkingOnly", appLinkingOnly.value());

            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            optAbilityPeer->qAbility().evalToPromiseOrRejectOnThrow(
                "context.openLink(*)",
                {
                    link.toStdString(),
                    QNapi::makeObject(jsState.env(), openLinkOptions),
                })
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QtOhos::CallbackInfo &) {
                    thenPromise(true);
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QtOhos::CallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "Got error from openLink()");
                    catchPromise(false);
                });
        },
        Q_FUNC_INFO);
}

void QOhosQpaFunctionsImpl::setAudioStreamUsageHintProperty(QObject *qObject, AudioStreamUsage usage)
{
    setQOhosPropertyOnQObject<QOhosQpaFunctions::AudioStreamUsage, &audioStreamUsageProperty>(qObject, usage);
}

QOhosOptional<QOhosQpaFunctions::AudioStreamUsage> QOhosQpaFunctionsImpl::tryGetAudioStreamUsageHintProperty(QObject *qObject)
{
    return tryGetQOhosPropertyFromQObject<QOhosQpaFunctions::AudioStreamUsage, &audioStreamUsageProperty>(qObject);
}

void QOhosQpaFunctionsImpl::processSerialPortPermissionResponse(std::uint32_t serialPortId, bool granted)
{
    auto permissionContext = granted
        ? QtOhos::makeDestroyNotifier(
            [serialPortId, weakSelf = QtOhos::makeWeakPtr(shared_from_this())]() {
                QtOhos::invokeInQtThread(
                    [serialPortId, weakSelf]() {
                        QtOhos::runInJsThreadAndWait(
                            [&](QtOhos::JsState &jsState) {
                                cancelSerialPortAccessRightJsImpl(jsState, serialPortId);
                            },
                            Q_FUNC_INFO);

                        auto self = weakSelf.lock();
                        if (self)
                            self->m_grantedSerialPortsPermissionContexts.erase(serialPortId);
                    });
            })
        : nullptr;

    if (permissionContext)
        m_grantedSerialPortsPermissionContexts[serialPortId] = permissionContext;

    for (const auto &asyncPermissionRequestConsumer : m_pendingSerialPortsPermissionRequestsConsumers[serialPortId])
        asyncPermissionRequestConsumer(permissionContext);

    m_pendingSerialPortsPermissionRequestsConsumers.erase(serialPortId);
}

}

QOhosQpaFunctions::QOhosQpaFunctions() = default;

QOhosQpaFunctions::~QOhosQpaFunctions() = default;

QOhosQpaFunctions::WantInfo::WantInfo() = default;

QOhosQpaFunctions::WantInfo::~WantInfo() = default;

QOhosQpaFunctions &getQOhosQpaFunctions()
{
    static QOhosQpaFunctionsImpl qpaFunctions;
    return qpaFunctions;
}

}

QT_END_NAMESPACE
