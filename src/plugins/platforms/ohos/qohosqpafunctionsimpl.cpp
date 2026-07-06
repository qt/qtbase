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
#include <QtGui/private/qohosimageconversions_p.h>
#include <algorithm>
#include <chrono>
#include <filemanagement/file_uri/oh_file_uri.h>
#include <filemanagement/fileshare/oh_file_share.h>
#include <functional>
#include <info/application_target_sdk_version.h>
#include <memory>
#include <qohosapppermissions_p.h>
#include <qohosenums.h>
#include <qohosjsutils.h>
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

std::optional<QOhosQpaFunctions::WantInfo::LaunchReason> tryMapOhosLaunchReasonToWantInfoEnum(
    enums::ohos::app::ability::AbilityConstant::LaunchReason ohosLaunchReason)
{
    using OhosLaunchReason = enums::ohos::app::ability::AbilityConstant::LaunchReason;
    using WantInfo = QOhosQpaFunctions::WantInfo;

    switch (ohosLaunchReason) {
    case OhosLaunchReason::START_ABILITY:
        return WantInfo::LaunchReason::START_ABILITY;
    case OhosLaunchReason::CONTINUATION:
        return WantInfo::LaunchReason::CONTINUATION;
    case OhosLaunchReason::PREPARE_CONTINUATION:
        return WantInfo::LaunchReason::PREPARE_CONTINUATION;
    case OhosLaunchReason::PRELOAD:
        return WantInfo::LaunchReason::PRELOAD;
    case OhosLaunchReason::UNKNOWN:
    case OhosLaunchReason::CALL:
    case OhosLaunchReason::APP_RECOVERY:
    case OhosLaunchReason::SHARE:
    case OhosLaunchReason::AUTO_STARTUP:
    case OhosLaunchReason::INSIGHT_INTENT:
        return WantInfo::LaunchReason::UNKNOWN;
    }

    return {};
}

QOhosQpaFunctions::WantInfo::LaunchReason mapJsLaunchReasonToWantInfoEnumWithFallback(
    QOhosJsState &jsState, QNapi::Number jsLaunchReason)
{
    auto optLaunchReasonJsEnum =
        jsState.tryMapOhosEnumFromJs<enums::ohos::app::ability::AbilityConstant::LaunchReason>(jsLaunchReason);
    auto optLaunchReason =
        optLaunchReasonJsEnum.has_value()
            ? tryMapOhosLaunchReasonToWantInfoEnum(optLaunchReasonJsEnum.value())
            : std::nullopt;
    return optLaunchReason.value_or(QOhosQpaFunctions::WantInfo::LaunchReason::UNKNOWN);
}

Q_NORETURN void killCurrentProcess()
{
    ::kill(getpid(), SIGKILL);
    std::abort();
}

std::optional<QOhosAbilityOnContinueResult> tryMapAbilityOnContinueResponseStatusToOhos(
    QOhosQpaFunctions::AbilityOnContinueResponseStatus status)
{
    using AbilityOnContinueResponseStatus = QOhosQpaFunctions::AbilityOnContinueResponseStatus;

    switch (status) {
    case AbilityOnContinueResponseStatus::Agree:
        return QOhosAbilityOnContinueResult::AGREE;
    case AbilityOnContinueResponseStatus::Reject:
        return QOhosAbilityOnContinueResult::REJECT;
    case AbilityOnContinueResponseStatus::Mismatch:
        return QOhosAbilityOnContinueResult::MISMATCH;
    }
    return {};
}

std::optional<enums::ohos::app::ability::AbilityConstant::WindowMode> tryMapWindowModeToOhosOrLogWarning(
    QOhosQpaFunctions::StartOptions::WindowMode windowMode)
{
    namespace AbilityConstant = enums::ohos::app::ability::AbilityConstant;
    using StartOptions = QOhosQpaFunctions::StartOptions;

    switch (windowMode) {
    case StartOptions::WindowMode::WINDOW_MODE_SPLIT_PRIMARY:
        return AbilityConstant::WindowMode::WINDOW_MODE_SPLIT_PRIMARY;
    case StartOptions::WindowMode::WINDOW_MODE_SPLIT_SECONDARY:
        return AbilityConstant::WindowMode::WINDOW_MODE_SPLIT_SECONDARY;
    case StartOptions::WindowMode::WINDOW_MODE_FULLSCREEN:
        return AbilityConstant::WindowMode::WINDOW_MODE_FULLSCREEN;
    }

    qCWarning(QtForOhos, "%s: got illegal WindowMode: %d", Q_FUNC_INFO, static_cast<int>(windowMode));

    return {};
}

std::optional<enums::ohos::app::ability::contextConstant::ProcessMode> tryMapProcessModeToOhosOrLogWarning(
    QOhosQpaFunctions::StartOptions::ProcessMode processMode)
{
    namespace contextConstant = enums::ohos::app::ability::contextConstant;
    using StartOptions = QOhosQpaFunctions::StartOptions;

    switch (processMode) {
    case StartOptions::ProcessMode::NEW_PROCESS_ATTACH_TO_PARENT:
        return contextConstant::ProcessMode::NEW_PROCESS_ATTACH_TO_PARENT;
    case StartOptions::ProcessMode::NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM:
        return contextConstant::ProcessMode::NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM;
    }

    qCWarning(QtForOhos, "%s: got illegal ProcessMode: %d", Q_FUNC_INFO, static_cast<int>(processMode));

    return {};
}

std::optional<enums::ohos::app::ability::contextConstant::StartupVisibility> tryMapStartupVisibilityToOhosOrLogWarning(
    QOhosQpaFunctions::StartOptions::StartupVisibility startupVisibility)
{
    namespace contextConstant = enums::ohos::app::ability::contextConstant;
    using StartOptions = QOhosQpaFunctions::StartOptions;

    switch (startupVisibility) {
    case StartOptions::StartupVisibility::STARTUP_HIDE:
        return contextConstant::StartupVisibility::STARTUP_HIDE;
    case StartOptions::StartupVisibility::STARTUP_SHOW:
        return contextConstant::StartupVisibility::STARTUP_SHOW;
    }

    qCWarning(QtForOhos, "%s: got illegal StartupVisibility: %d", Q_FUNC_INFO, static_cast<int>(startupVisibility));

    return {};
}

std::optional<enums::ohos::bundle::bundleManager::SupportWindowMode> tryMapSupportWindowModeToOhosOrLogWarning(
    QOhosQpaFunctions::StartOptions::SupportWindowMode supportWindowMode)
{
    namespace bundleManager = enums::ohos::bundle::bundleManager;
    using StartOptions = QOhosQpaFunctions::StartOptions;

    switch (supportWindowMode) {
    case StartOptions::SupportWindowMode::FULL_SCREEN:
        return bundleManager::SupportWindowMode::FULL_SCREEN;
    case StartOptions::SupportWindowMode::SPLIT:
        return bundleManager::SupportWindowMode::SPLIT;
    case StartOptions::SupportWindowMode::FLOATING:
        return bundleManager::SupportWindowMode::FLOATING;
    }

    qCWarning(QtForOhos, "%s: got illegal SupportWindowMode: %d", Q_FUNC_INFO, static_cast<int>(supportWindowMode));

    return {};
}

QNapi::Array mapSupportWindowModesToJsEnumsArray(
    QOhosJsState &jsState, const QList<QOhosQpaFunctions::StartOptions::SupportWindowMode> &supportWindowModes)
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
    QOhosJsState &jsState, std::shared_ptr<QOhosConsumer<bool, QJsonObject, QString>> qtThreadCompletionHandler)
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

QNapi::Object convertStartOptionsToNapiObject(
    QOhosJsState &jsState, const QOhosQpaFunctions::StartOptions &opts)
{
    auto *env = jsState.env();
    auto napiOptions = QNapi::Object::New(env);

    auto optOhosWindowMode = opts.windowMode.has_value()
        ? tryMapWindowModeToOhosOrLogWarning(opts.windowMode.value())
        : std::nullopt;
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
    auto optOhosProcessMode = opts.processMode.has_value()
        ? tryMapProcessModeToOhosOrLogWarning(opts.processMode.value())
        : std::nullopt;
    if (optOhosProcessMode.has_value())
        napiOptions.set("processMode", jsState.mapOhosEnumToJs(optOhosProcessMode.value()));
    auto optOhosStartupVisibility = opts.startupVisibility.has_value()
        ? tryMapStartupVisibilityToOhosOrLogWarning(opts.startupVisibility.value())
        : std::nullopt;
    if (optOhosStartupVisibility.has_value())
        napiOptions.set("startupVisibility", jsState.mapOhosEnumToJs(optOhosStartupVisibility.value()));
    if (opts.windowIcon.has_value()) {
        auto windowIcon = opts.windowIcon.value().value<QImage>();
        if (!windowIcon.isNull())
            napiOptions.set("startWindowIcon", makeOhosNapiPixelMapFromQImage(jsState, windowIcon));
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

std::optional<QOhosQpaFunctions::FileShare::PolicyErrorCode> tryMapFileSharePolicyErrorCode(
    ::FileShare_PolicyErrorCode errorCode)
{
    using PolicyErrorCode = QOhosQpaFunctions::FileShare::PolicyErrorCode;
    switch (errorCode) {
    case ::FileShare_PolicyErrorCode::PERSISTENCE_FORBIDDEN:
        return PolicyErrorCode::PERSISTENCE_FORBIDDEN;
    case ::FileShare_PolicyErrorCode::INVALID_MODE:
        return PolicyErrorCode::INVALID_MODE;
    case ::FileShare_PolicyErrorCode::INVALID_PATH:
        return PolicyErrorCode::INVALID_PATH;
    case ::FileShare_PolicyErrorCode::PERMISSION_NOT_PERSISTED:
        return PolicyErrorCode::PERMISSION_NOT_PERSISTED;
    }
    return {};
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

std::optional<QOhosQpaFunctions::ShareKit::SharedRecord> tryConvertNapiObjectToSharedRecord(QNapi::Object record)
{
    auto tryGetOptionalStringProp = [](const QNapi::Object &object, const std::string &propName) -> std::optional<QString> {
        auto optProp = getOptionalProperty<QNapi::String>(object, propName);
        return optProp.has_value()
            ? std::make_optional(QString::fromStdString(optProp.value()))
            : std::nullopt;
    };

    auto tryGetOptionalByteArrayProp = [](const QNapi::Object &object, const std::string &propName) -> std::optional<QByteArray> {
        auto optProp = getOptionalProperty<QNapi::TypedArrayOf<std::uint8_t>>(object, propName);
        return optProp.has_value()
            ? std::make_optional(QByteArray(
                  reinterpret_cast<const char *>(optProp.value().Data()),
                  optProp.value().ByteLength()))
            : std::nullopt;
    };

    auto tryGetOptionalJsonObjectProp = [](const QNapi::Object &object, const std::string &propName) -> std::optional<QJsonObject> {
        auto optProp = getOptionalProperty<QNapi::Object>(object, propName);
        return optProp.has_value()
            ? std::make_optional(QOhosJsEnv::fromNapiValue<QJsonObject>(optProp.value()))
            : std::nullopt;
    };

    std::string utd = record.get<QNapi::String>("utd");
    auto optMimeType = utd != UDMF_META_HYPERLINK
        ? tryMapUtdTypeIdToMimeType(utd)
        : QOhosShareKit::mimeTextUriList;
    if (!optMimeType.has_value()) {
        qOhosPrintfWarning(
            "%s: can't map utd '%s' to mimetype, not mapping the record",
            Q_FUNC_INFO, utd.c_str());
        return {};
    }

    auto content = tryGetOptionalStringProp(record, "content");
    auto uri = tryGetOptionalStringProp(record, "uri");
    if (!content.has_value() && !uri.has_value()) {
        qOhosPrintfWarning(
            "%s: cannot create Shared Record, content and uri properties are empty", Q_FUNC_INFO);
        return {};
    }

    auto optExtraDataJson = tryGetOptionalJsonObjectProp(record, "extraData");

    return QOhosQpaFunctions::ShareKit::SharedRecord{
        .mimeType = QString::fromStdString(optMimeType.value()),
        .content = content,
        .filePath = uri,
        .title = tryGetOptionalStringProp(record, "title"),
        .label = tryGetOptionalStringProp(record, "label"),
        .description = tryGetOptionalStringProp(record, "description"),
        .thumbnail = tryGetOptionalByteArrayProp(record, "thumbnail"),
        .thumbnailFilePath = tryGetOptionalStringProp(record, "thumbnailUri"),
        .extraData = optExtraDataJson.has_value()
            ? std::make_optional(optExtraDataJson.value().toVariantMap())
            : std::nullopt,
    };
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

std::optional<std::uint32_t> tryConvertPortNameToSystemPortId(const QString &portName)
{
    constexpr const char *serialPortPrefix = "COM";
    const QString prefix = QLatin1String(serialPortPrefix);

    if (!portName.startsWith(prefix))
        return {};

    bool parsedOk = false;
    const uint parsedValue = portName.mid(prefix.length()).toUInt(&parsedOk);
    if (!parsedOk)
        return {};

    return static_cast<std::uint32_t>(parsedValue);
}

bool hasSerialPortAccessRightJsImpl(QOhosJsState &jsState, std::uint32_t serialPortId)
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
    QOhosJsState &jsState, std::uint32_t serialPortId, QOhosConsumer<bool> resultConsumer)
{
    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.usbManager.serial.requestSerialRight(*)", {serialPortId})
    .withContext(std::move(resultConsumer))
    .onThenWithContext(
        [](const QOhosCallbackInfo &cbInfo, auto &resultConsumer) {
            bool granted = cbInfo.getFirstArg<QNapi::Boolean>(Q_FUNC_INFO);
            resultConsumer(granted);
        })
    .onCatchWithContext(
        [](const QOhosCallbackInfo &cbInfo, auto &resultConsumer) {
            QtOhos::logJsCallbackError(
                cbInfo, "@ohos.usbManager.serial.requestSerialRight() failed");
            resultConsumer(false);
        });
}

void cancelSerialPortAccessRightJsImpl(QOhosJsState &jsState, std::uint32_t serialPortId)
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

    std::optional<QList<QOhosQpaFunctions::ShareKit::SharedRecord>> tryGetSharedDataRecords() const override;

    std::optional<ContactInfo> tryGetContactInfo() const override;

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

std::optional<QList<QOhosQpaFunctions::ShareKit::SharedRecord>> WantInfoImpl::tryGetSharedDataRecords() const
{
    using SharedRecord = QOhosQpaFunctions::ShareKit::SharedRecord;

    return QOhosJsThreadGateway::evalWithPromise<std::optional<QList<SharedRecord>>>(
        [&](QOhosJsState &jsState, QOhosTaskPromise<std::optional<QList<SharedRecord>>> evalPromise) {
            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            jsState.evalToPromiseOrRejectOnThrow(
                "@kit.ShareKit.systemShare.getSharedData(*)", {m_jsScopeData->want.Value()})
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QOhosCallbackInfo &cbInfo) {
                    QNapi::Object sharedData = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

                    auto optRecords = QNapi::getArrayElements<QList<std::optional<SharedRecord>>, QNapi::Object>(
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

                    thenPromise(records);
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QOhosCallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "@kit.ShareKit.systemShare.getSharedData() failed");
                    catchPromise({});
                });
        },
        Q_FUNC_INFO);
}

std::optional<QOhosQpaFunctions::WantInfo::ContactInfo> WantInfoImpl::tryGetContactInfo() const
{
    using ContactInfo = QOhosQpaFunctions::WantInfo::ContactInfo;

    return QOhosJsThreadGateway::evalWithPromise<std::optional<ContactInfo>>(
        [&](QOhosJsState &jsState, QOhosTaskPromise<std::optional<ContactInfo>> evalPromise) {
            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            jsState.evalToPromiseOrRejectOnThrow(
                "@kit.ShareKit.systemShare.getContactInfo(*)", {m_jsScopeData->want.Value()})
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QOhosCallbackInfo &cbInfo) {
                    auto contactInfoObj = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                    ContactInfo contactInfo = {
                        .contactType = QString::fromStdString(
                            contactInfoObj.get<QNapi::String>("contactType")),
                        .contactId = QString::fromStdString(
                            contactInfoObj.get<QNapi::String>("contactId")),
                    };
                    thenPromise(contactInfo);
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QOhosCallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "@kit.ShareKit.systemShare.getContactInfo() failed");
                    catchPromise({});
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

    std::optional<double> tryGetNativeWindowId(QObject *window) override;
    std::optional<double> tryGetScreenDisplayId(QObject *screenObject) override;

    void setOnContinueRequestsHandlerForAbilityInstanceWindow(
        QObject *windowObject, std::function<void(AbilityOnContinueRequest, QOhosConsumer<AbilityOnContinueResponse>)> requestsHandler) override;

    void setAbilityContinuationActive(
        QObject *optInstanceMainWindow, bool continuationActive) override;

    Q_NORETURN void restartApp(std::optional<QJsonObject> want) override;

    QJsonObject getAppLaunchWant() override;
    QSharedPointer<WantInfo> getAppLaunchWantInfo() const override;

    void addNewWantConsumer(QObject *context, QOhosConsumer<QJsonObject> wantConsumer) override;
    void addNewWantConsumer(
        QObject *context, QOhosConsumer<QSharedPointer<WantInfo>> wantConsumer) override;

    void startAppProcess(
        const QString &processId, const QJsonObject &requestWant,
        const std::optional<StartOptions> &optStartOptions) override;

    bool startAbility(const QJsonObject &want, const std::optional<StartOptions> &options) override;

    bool startAbilityByType(const QString &appType, const QJsonObject &wantParameters) override;

    void startAbilityForResult(
        const QJsonObject &want, const std::optional<StartOptions> &options,
        QObject *optInstanceMainWindow, QObject *resultConsumerQtContext,
        QOhosConsumer<std::optional<AbilityResult>> resultConsumer) override;

    void setDestroyAllowedFlagForAbilityInstances(
        std::vector<QObject *> instancesMainWindows, bool destroyEnabled) override;

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

    bool tryOpenLink(QObject *optInstanceMainWindow, const QString &link, std::optional<bool> appLinkingOnly) override;

    void setAudioStreamUsageHintProperty(QObject *qObject, AudioStreamUsage usage) override;
    std::optional<AudioStreamUsage> tryGetAudioStreamUsageHintProperty(QObject *qObject) override;

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
    std::optional<QOhosPlatformWindow::NativeNodeRenderFitPolicy> policy;
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
    std::optional<QOhosPlatformIntegration::WindowGeometryPersistencePolicy> policy;
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

std::optional<double> QOhosQpaFunctionsImpl::tryGetNativeWindowId(QObject *window)
{
    auto *qWindow = qobject_cast<QWindow *>(window);
    if (qWindow == nullptr)
        return {};

    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(qWindow);
    if (platformWindow == nullptr)
        return {};

    auto internalId = platformWindow->internalWindowId();
    auto jsWinId = QWindowProxyRegistry::instance().tryMapInternalWindowIdToJsWindowId(internalId);
    if (!jsWinId.has_value())
        return {};

    qOhosPrintfInfo(
        "PlatformWindow WIID: %s is returning JsWindowId: %f to the user",
        qPrintable(internalId.toString()), jsWinId.value().value());

    return jsWinId.value().value();
}

std::optional<double> QOhosQpaFunctionsImpl::tryGetScreenDisplayId(QObject *screenObject)
{
    auto *qScreen = qobject_cast<QScreen *>(screenObject);
    if (qScreen == nullptr) {
        qOhosPrintfWarning("%s: screenObject argument is not a QScreen", Q_FUNC_INFO);
        return {};
    }
    auto *ohosPlatformScreen = static_cast<QOhosPlatformScreen *>(qScreen->handle());

    return ohosPlatformScreen != nullptr
        ? std::optional(ohosPlatformScreen->displayInfo().id.value())
        : std::nullopt;
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
        QOhosConsumer<QOhosJsState &, QNapi::Number> resultConsumer;
    };

    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
            auto optQAbility = jsState.tryGetQAbilityByQWindow(qWindowRef);
            if (!optQAbility.has_value()) {
                qOhosPrintfError(
                    "%s: no ability for window %s, handler not set",
                    Q_FUNC_INFO, qWindowRef.refName().c_str());
                return;
            }

            jsState.setOnContinueRequestsHandler(
                optQAbility.value(),
                [sharedRequestsHandler](QOhosJsState &, QNapi::Object wantParamsObj, auto resultConsumer) {
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
                                    QOhosJsThreadGateway::invoke(
                                        [jsResultContext, qtResponse](QOhosJsState &jsState) {
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
                                                jsState, jsState.mapOhosEnumToJs(ohosResult.value_or(QOhosAbilityOnContinueResult::REJECT)));
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
           ? std::optional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
           : std::nullopt;

    QOhosJsThreadGateway::invokeAndWaitForContinue(
        [&](QOhosJsState &jsState, QOhosTaskPromise<> taskPromise) {
            auto optUiAbility = optInstanceMainWindowRef.has_value()
                ? jsState.tryGetQAbilityByQWindow(optInstanceMainWindowRef.value())
                : jsState.defaultQAbility();
            if (!optUiAbility.has_value()) {
                taskPromise();
                return;
            }

            auto continueState = continuationActive ? ContinueState::ACTIVE : ContinueState::INACTIVE;
            optUiAbility.value().evalToPromiseOrRejectOnThrow(
                "context.setMissionContinueState(*)", {jsState.mapOhosEnumToJs(continueState)})
            .onCatch(QtOhos::makeErrorLoggingJsCallback("setMissionContinueState()"))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}

Q_NORETURN void QOhosQpaFunctionsImpl::restartApp(std::optional<QJsonObject> want)
{
    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
            auto napiWant = want.has_value()
                ? QNapi::checkedCast<QNapi::Object>(QOhosJsEnv::toNapiValue(jsState.env(), want.value()))
                : jsState.appLaunchWant();

            constexpr auto sleepTimeBeforeRetry = std::chrono::seconds(3);

            unsigned remainingTries = 3;

            while (true) {
                --remainingTries;

                qOhosPrintfInfo(
                    "%s: calling restartApp() using Want: %s",
                    Q_FUNC_INFO, QNapi::toJsonString(napiWant).c_str());

                auto optQAbility = jsState.defaultQAbility();
                if (!optQAbility.has_value())
                    qOhosReportFatalErrorAndAbort("%s: no default UIAbility available to restart the app", Q_FUNC_INFO);

                try {
                    optQAbility.value().call(
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
    return QOhosJsThreadGateway::eval(
        [](QOhosJsState &jsState) {
            return QOhosJsEnv::fromNapiValue<QJsonObject>(jsState.appLaunchWant());
        },
        Q_FUNC_INFO);
}

QSharedPointer<QOhosQpaFunctions::WantInfo> QOhosQpaFunctionsImpl::getAppLaunchWantInfo() const
{
    return QOhosJsThreadGateway::eval(
        [](QOhosJsState &jsState) {
            auto optAppLaunchParam = jsState.optAppLaunchParam();
            auto optAppLaunchReason = optAppLaunchParam.has_value()
                ? std::make_optional(mapJsLaunchReasonToWantInfoEnumWithFallback(
                      jsState, optAppLaunchParam.value().get<QNapi::Number>("launchReason")))
                : std::nullopt;
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
    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
            jsState.addNewWantConsumer(
                [contextRef, sharedWantConsumer](QOhosJsState &jsState, QNapi::Object napiWant, QNapi::Object launchParam) {
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
    const std::optional<StartOptions> &optStartOptions)
{
    QOhosJsThreadGateway::invokeAndWaitForContinue(
        [&](QOhosJsState &jsState, QOhosTaskPromise<> taskPromise) {
            auto startOptions = optStartOptions.has_value()
                ? convertStartOptionsToNapiObject(jsState, optStartOptions.value())
                : QNapi::Object();

            auto sharedTaskPromise = QtOhos::moveToSharedPtr(std::move(taskPromise).makeChained(Q_FUNC_INFO));
            jsState.startAppProcess(
                processId.toStdString(),
                QNapi::checkedCast<QNapi::Object>(
                    QOhosJsEnv::toNapiValue(jsState.env(), requestWant)),
                startOptions,
                [sharedTaskPromise](QOhosJsState &) {
                    (*sharedTaskPromise)();
                });
    },
    Q_FUNC_INFO);
}

bool QOhosQpaFunctionsImpl::startAbility(const QJsonObject &want, const std::optional<QOhosQpaFunctions::StartOptions> &options)
{
    return QOhosJsThreadGateway::eval(
        [&](QOhosJsState &jsState) {
            auto optMainUiAbility = jsState.defaultQAbility();
            if (!optMainUiAbility.has_value())
                return false;
            auto mainUiAbility = optMainUiAbility.value();
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
    return QOhosJsThreadGateway::evalWithPromise<bool>(
        [&](QOhosJsState &jsState, QOhosTaskPromise<bool> evalPromise) {
            auto optQAbility = jsState.defaultQAbility();
            if (!optQAbility.has_value()) {
                evalPromise(false);
                return;
            }

            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            optQAbility.value().evalToPromiseOrRejectOnThrow(
                "context.startAbilityByType(*)",
                {
                    appType.toStdString(),
                    QOhosJsEnv::toNapiValue(jsState.env(), wantParameters),
                    QNapi::makeObject(
                        jsState.env(),
                        {
                            {
                                "onResult",
                                [](const QOhosCallbackInfo&) {
                                    qOhosPrintfDebug("startAbilityByType: onResult called");
                                }
                            },
                            {
                                "onError",
                                [](const QOhosCallbackInfo &cbInfo) {
                                    QtOhos::logJsCallbackError(cbInfo, "startAbilityByType: onError called");
                                }
                            }
                        })
                })
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QOhosCallbackInfo &) {
                    thenPromise(true);
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QOhosCallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "startAbilityByType: failed");
                    catchPromise(false);
                });
        },
        Q_FUNC_INFO);
}

void QOhosQpaFunctionsImpl::startAbilityForResult(
    const QJsonObject &want, const std::optional<StartOptions> &options,
    QObject *optInstanceMainWindow, QObject *resultConsumerQtContext,
    QOhosConsumer<std::optional<AbilityResult>> resultConsumer)
{
    struct Context
    {
        QtOhos::QObjectThreadSafeRef resultConsumerQtContextRef;
        QOhosConsumer<std::optional<AbilityResult>> resultConsumer;
    };

    auto context = QtOhos::moveToSharedPtr(
        Context{
            .resultConsumerQtContextRef = QtOhos::QObjectThreadSafeRef(resultConsumerQtContext),
            .resultConsumer = std::move(resultConsumer),
        });

    auto optInstanceMainWindowRef =
        optInstanceMainWindow != nullptr
           ? std::optional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
           : std::nullopt;

    QOhosJsThreadGateway::invoke(
        [context, want, options, optInstanceMainWindowRef](QOhosJsState &jsState) {
            auto optUiAbility = optInstanceMainWindowRef.has_value()
                ? jsState.tryGetQAbilityByQWindow(optInstanceMainWindowRef.value())
                : jsState.defaultQAbility();
            if (!optUiAbility.has_value()) {
                context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                    [context](auto &) {
                        context->resultConsumer({});
                    });
                return;
            }

            auto arguments = std::vector<QNapi::ValueWrapper>{QOhosJsEnv::toNapiValue(jsState.env(), want)};
            if (options.has_value())
                arguments.push_back(convertStartOptionsToNapiObject(jsState, options.value()));

            optUiAbility.value().evalToPromiseOrRejectOnThrow("context.startAbilityForResult(*)", arguments)
            .onThen(
                [context](const QOhosCallbackInfo &cbInfo) {
                    QNapi::Object abilityResult = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                    int resultCode = abilityResult.get<QNapi::Number>("resultCode");

                    auto wantOrEmpty = QNapi::getOptionalPropOrEmpty<QNapi::Object>(abilityResult, "want");
                    auto jsonWant = !wantOrEmpty.IsEmpty()
                        ? std::optional<QJsonObject>(QOhosJsEnv::fromNapiValue<QJsonObject>(wantOrEmpty))
                        : std::nullopt;

                    context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                        [context, resultCode, jsonWant](auto &) {
                            constexpr int startedAbilityErrorResultCode = -1;
                            context->resultConsumer(
                                resultCode != startedAbilityErrorResultCode
                                    ? std::optional<AbilityResult>({resultCode, jsonWant})
                                    : std::nullopt);
                        });
                })
            .onCatch(
                [context](const QOhosCallbackInfo &cbInfo) {
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

    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
            for (const auto &instanceMainWindowRef : instancesMainWindowsRefs) {
                auto optQAbility = jsState.tryGetQAbilityByQWindow(instanceMainWindowRef);
                if (optQAbility)
                    jsState.setDestroyFromSystemAllowed(optQAbility.value(), destroyEnabled);
            }
        },
        Q_FUNC_INFO);
}

QOhosSupplier<double> QOhosQpaFunctionsImpl::makeOhosConfigFontSizeScaleDataSource(
    QOhosConsumer<double> valueChangedHandler)
{
    auto initFontSizeScale = QOhosPlatformIntegration::instance()->settings()->fontSizeScale();
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
    return QOhosJsThreadGateway::eval(
        [](QOhosJsState &jsState) {
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
    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
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

    return QOhosJsThreadGateway::eval(
        [&](QOhosJsState &jsState) {
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
                QOhosJsThreadGateway::invoke(
                    [serialPortId, weakSelf](QOhosJsState &jsState) {
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
    return QOhosJsThreadGateway::eval(
        [&](QOhosJsState &) {
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
    return QOhosJsThreadGateway::eval(
        [&](QOhosJsState &) {
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
    return QOhosJsThreadGateway::eval(
        [&](QOhosJsState &) {
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
    return QOhosJsThreadGateway::eval(
        [&](QOhosJsState &) {
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
    return QOhosJsThreadGateway::eval(
        [&](QOhosJsState &) {
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
                .content = record.content.has_value()
                    ? std::make_optional(record.content.value().toStdString())
                    : std::nullopt,
                .filePath = record.filePath.has_value()
                    ? std::make_optional(record.filePath.value().toStdString())
                    : std::nullopt,
                .title = record.title.has_value()
                    ? std::make_optional(record.title.value().toStdString())
                    : std::nullopt,
                .label = record.label.has_value()
                    ? std::make_optional(record.label.value().toStdString())
                    : std::nullopt,
                .description = record.description.has_value()
                    ? std::make_optional(record.description.value().toStdString())
                    : std::nullopt,
                .thumbnail = record.thumbnail,
                .thumbnailFilePath = record.thumbnailFilePath.has_value()
                    ? std::make_optional(record.thumbnailFilePath.value().toStdString())
                    : std::nullopt,
                .extraData = record.extraData,
            });
    }

    auto shareKitControllerOptions = QOhosShareKit::ControllerOptions{
        .anchor = controllerOptions.anchorOffset.has_value()
            ? std::make_optional(
                QOhosShareKit::ShareControllerAnchor{
                    .windowOffset = controllerOptions.anchorOffset.value(),
                    .size = controllerOptions.anchorSize,
                })
            : std::nullopt,
        .selectionMode = controllerOptions.useSingleSelectionMode.has_value()
            ? std::make_optional(
                controllerOptions.useSingleSelectionMode.value()
                    ? QOhosShareKit::SelectionMode::SINGLE
                    : QOhosShareKit::SelectionMode::BATCH)
            : std::nullopt,
        .previewMode = controllerOptions.useDefaultPreviewMode.has_value()
            ? std::make_optional(
                controllerOptions.useDefaultPreviewMode.value()
                    ? QOhosShareKit::SharePreviewMode::DEFAULT
                    : QOhosShareKit::SharePreviewMode::DETAIL)
            : std::nullopt,
        .excludedAbilities = controllerOptions.excludedAbilities.has_value()
            ? std::make_optional(
                [&]() {
                    std::vector<QOhosShareKit::ShareAbilityType> outExcludedAbilities;
                    for (auto excludedAbilityType : controllerOptions.excludedAbilities.value()) {
                        outExcludedAbilities.push_back(
                            mapShareAbilityTypeFromQpaFunctionsEnum(excludedAbilityType));
                    }
                    return outExcludedAbilities;
                }())
            : std::nullopt,
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

bool QOhosQpaFunctionsImpl::tryOpenLink(QObject *optInstanceMainWindow, const QString &link, std::optional<bool> appLinkingOnly)
{
    if (optInstanceMainWindow != nullptr && qobject_cast<QWindow *>(optInstanceMainWindow) == nullptr)
        qOhosReportFatalErrorAndAbort("%s: the main window argument is not a QWindow", Q_FUNC_INFO);

    auto optInstanceMainWindowRef = optInstanceMainWindow != nullptr
        ? std::optional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
        : std::nullopt;

    return QOhosJsThreadGateway::evalWithPromise<bool>(
        [&](QOhosJsState &jsState, QOhosTaskPromise<bool> evalPromise) {
            auto optUiAbility = optInstanceMainWindowRef.has_value()
                ? jsState.tryGetQAbilityByQWindow(optInstanceMainWindowRef.value())
                : jsState.defaultQAbility();
            if (!optUiAbility.has_value()) {
                evalPromise(false);
                return;
            }

            std::vector<std::pair<std::string, QNapi::ValueWrapper>> openLinkOptions;
            if (appLinkingOnly.has_value())
                openLinkOptions.emplace_back("appLinkingOnly", appLinkingOnly.value());

            auto thenCatchPromises = std::move(evalPromise).makeThenCatchBranches(Q_FUNC_INFO);
            optUiAbility.value().evalToPromiseOrRejectOnThrow(
                "context.openLink(*)",
                {
                    link.toStdString(),
                    QNapi::makeObject(jsState.env(), openLinkOptions),
                })
            .onThen(
                [thenPromise = std::move(thenCatchPromises.first)](const QOhosCallbackInfo &) {
                    thenPromise(true);
                })
            .onCatch(
                [catchPromise = std::move(thenCatchPromises.second)](const QOhosCallbackInfo &cbInfo) {
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

std::optional<QOhosQpaFunctions::AudioStreamUsage> QOhosQpaFunctionsImpl::tryGetAudioStreamUsageHintProperty(QObject *qObject)
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
                        QOhosJsThreadGateway::runAndWait(
                            [&](QOhosJsState &jsState) {
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
