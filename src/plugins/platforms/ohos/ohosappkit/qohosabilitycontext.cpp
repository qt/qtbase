// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosabilitycontext.h"
#include "qohosenums_p.h"
#include "qohosjsenv_p.h"
#include "qohoswantinfo_p.h"
#include "qohoswantutils_p.h"
#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qhash.h>
#include <QtCore/qmap.h>
#include <QtCore/qrandom.h>
#include <QtOhosAppKit/private/qohosoperationstatus_p.h>
#include <QtOhosAppKit/private/qohossharekit_p.h>
#include <QtOhosAppKit/private/qohosstartoptions_p.h>
#include <QtOhosAppKit/private/qohosstartrequest_p.h>
#include <QtOhosAppKit/private/qohoswantutils_p.h>
#include <QtGui/qwindow.h>
#include <QtGui/private/qohosimageconversions_p.h>
#include <QtGui/qimage.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

namespace {

constexpr const char *qtOnContinueMigrationDataPropertyName = "__io_qt_on_continue_migration_data";

QOhosSupplier<QByteArray> makeUniqueIdsGenerator()
{
    return [sequenceNumber = std::uint64_t(0)]() mutable {
        QByteArray idBytes(32, Qt::Uninitialized);
        std::memcpy(idBytes.data(), &sequenceNumber, sizeof(sequenceNumber));
        QRandomGenerator::global()->generate(idBytes.begin() + sizeof(sequenceNumber), idBytes.end());
        ++sequenceNumber;
        return idBytes;
    };
}

class QOhosOpenLinkOptionsImpl : public QOhosOpenLinkOptions
{
public:
    QOhosOpenLinkOptionsImpl();

    void setAppLinkingOnly(bool appLinkingOnly) override;

    std::optional<bool> appLinkingOnly() const;

private:
    std::optional<bool> m_appLinkingOnly;
};

QOhosOpenLinkOptionsImpl::QOhosOpenLinkOptionsImpl() = default;

void QOhosOpenLinkOptionsImpl::setAppLinkingOnly(bool appLinkingOnly)
{
    m_appLinkingOnly = appLinkingOnly;
}

std::optional<bool> QOhosOpenLinkOptionsImpl::appLinkingOnly() const
{
    return m_appLinkingOnly;
}

using OnContinueResult = QtOhos::enums::ohos::app::ability::AbilityConstant::OnContinueResult;

enum class AbilityOnContinueResponseStatus
{
    Agree,
    Reject,
    Mismatch,
};

struct AbilityOnContinueRequest
{
    int sourceApplicationVersionCode;
};

struct AbilityOnContinueResponse
{
    AbilityOnContinueResponseStatus status;
    QMap<QString, QString> wantObjectParams;
    std::optional<bool> exitAppOnSourceDeviceAfterMigration;
};

std::optional<OnContinueResult> tryMapAbilityOnContinueResponseStatusToOhos(
    AbilityOnContinueResponseStatus status)
{
    switch (status) {
    case AbilityOnContinueResponseStatus::Agree:
        return OnContinueResult::AGREE;
    case AbilityOnContinueResponseStatus::Reject:
        return OnContinueResult::REJECT;
    case AbilityOnContinueResponseStatus::Mismatch:
        return OnContinueResult::MISMATCH;
    }
    return std::nullopt;
}

void setOnContinueRequestsHandlerForAbilityInstanceWindow(
    QObject *windowObject,
    std::function<void(AbilityOnContinueRequest, QOhosConsumer<AbilityOnContinueResponse>)> requestsHandler)
{
    auto *qWindow = qobject_cast<QWindow *>(windowObject);
    if (qWindow == nullptr)
        qOhosReportFatalErrorAndAbort("%s: windowObject argument is null or not a QWindow", Q_FUNC_INFO);

    auto qWindowRef = QtOhos::QObjectThreadSafeRef(qWindow);
    auto sharedRequestsHandler = QtOhos::moveToSharedPtr(std::move(requestsHandler));

    struct JsResultContext
    {
        QNapi::Reference<QNapi::Object> wantParamsReference;
        QOhosConsumer<QOhosJsState &, QNapi::Number> resultConsumer;
    };

    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
            auto optQAbility = jsState.tryGetQAbilityByQWindow(qWindowRef);
            if (!optQAbility) {
                qOhosPrintfError(
                    "%s: no ability for window %s, handler not set",
                    Q_FUNC_INFO, qWindowRef.refName().c_str());
                return;
            }

            jsState.setOnContinueRequestsHandler(
                optQAbility.value(),
                [sharedRequestsHandler](QOhosJsState &, QNapi::Object wantParamsObj, auto resultConsumer) {
                    int sourceVersionCode = wantParamsObj.get<QNapi::Number>("version");
                    auto jsResultContext = QtOhos::makeProxyWithJsThreadDeleter(std::make_shared<JsResultContext>());
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
                                            auto optResultEnum = tryMapAbilityOnContinueResponseStatusToOhos(qtResponse.status);
                                            if (!optResultEnum) {
                                                qOhosPrintfWarning(
                                                    "%s: got illegal status (%d) from requests handler, rejecting the request",
                                                    Q_FUNC_INFO, static_cast<int>(qtResponse.status));
                                            }
                                            jsResultContext->resultConsumer(
                                                jsState, jsState.mapOhosEnumToJs(optResultEnum.value_or(OnContinueResult::REJECT)));
                                        });
                                });
                        });
                });
        },
        Q_FUNC_INFO);
}

void setDestroyAllowedFlagForAbilityInstances(
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

class QOhosOnContinueContextImpl : public QOhosOnContinueContext
{
public:
    QOhosOnContinueContextImpl(int sourceApplicationVersionCode);

    void setAgreeResponse(const QByteArray &responseData) override;
    void setRejectResponse() override;
    void setMismatchResponse() override;

    void setExitAppOnSourceDeviceAfterMigration(bool exitAfterMigration) override;

    int sourceApplicationVersionCode() const override;

    AbilityOnContinueResponse response() const;

private:
    int m_sourceApplicationVersionCode;
    AbilityOnContinueResponse m_baseResponse;
    std::optional<bool> m_exitAppOnSourceDeviceAfterMigration;
};

QOhosOnContinueContextImpl::QOhosOnContinueContextImpl(int sourceApplicationVersionCode)
    : QOhosOnContinueContext()
    , m_sourceApplicationVersionCode(sourceApplicationVersionCode)
{
    setRejectResponse();
}

/*!
    \fn void QtOhosAppKit::QOhosOnContinueContext::setAgreeResponse(const QByteArray &responseData)

    Sets On Continue action as agreed with a given \a responseData.
    Agreed response means that on continuation process is accepted.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-abilityconstant-V13#oncontinueresult}
    {OnContinueResult}
*/
void QOhosOnContinueContextImpl::setAgreeResponse(const QByteArray &responseData)
{
    m_baseResponse = AbilityOnContinueResponse{
        .status = AbilityOnContinueResponseStatus::Agree,
        .wantObjectParams = {
            {
                QString::fromUtf8(qtOnContinueMigrationDataPropertyName),
                QString::fromUtf8(responseData.toBase64())
            },
        },
    };
}

/*!
    \fn void QtOhosAppKit::QOhosOnContinueContext::setRejectResponse()

    Sets On Continue action as rejected.
    Rejected responses means that on continuation process should not be continued.
    This is typically used when the target application cannot handle the continuation
    request for any reason.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-abilityconstant-V13#oncontinueresult}
    {OnContinueResult}
*/
void QOhosOnContinueContextImpl::setRejectResponse()
{
    m_baseResponse = AbilityOnContinueResponse{
        .status = AbilityOnContinueResponseStatus::Reject,
        .wantObjectParams = {},
    };
}

/*!
    \fn void QOhosOnContinueContext::setMismatchResponse()

    Sets On Continue action as mismatched.
    Mismatched responses means that on continuation process should not be continued - most probably due to
    source and target application version code difference.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-abilityconstant-V13#oncontinueresult}
    {OnContinueResult}

    \sa sourceApplicationVersionCode()
*/
void QOhosOnContinueContextImpl::setMismatchResponse()
{
    m_baseResponse = AbilityOnContinueResponse{
        .status = AbilityOnContinueResponseStatus::Mismatch,
        .wantObjectParams = {},
    };
}

/*!
    \fn void QOhosOnContinueContext::setExitAppOnSourceDeviceAfterMigration(bool exitAfterMigration)

    Decides whether the application should automatically exit on the source device after successful
    migration to the target device.

    The default setting is determined by the platform. As of API17 it's set to "true" by default.
*/
void QOhosOnContinueContextImpl::setExitAppOnSourceDeviceAfterMigration(bool exitAfterMigration)
{
    m_exitAppOnSourceDeviceAfterMigration = exitAfterMigration;
}

AbilityOnContinueResponse QOhosOnContinueContextImpl::response() const
{
    auto response = m_baseResponse;
    response.exitAppOnSourceDeviceAfterMigration = m_exitAppOnSourceDeviceAfterMigration;
    return response;
}

/*!
    \fn int QtOhosAppKit::QOhosOnContinueContext::sourceApplicationVersionCode() const

    Returns source application version code. Source application is the one which has started
    the continuation process. This version code is devlivered as "version" property in
    onContinue wantParam.
*/
int QOhosOnContinueContextImpl::sourceApplicationVersionCode() const
{
    return m_sourceApplicationVersionCode;
}

std::optional<QtOhos::enums::ohos::app::ability::AbilityConstant::WindowMode> tryMapWindowModeToOhosOrLogWarning(
    QOhosStartOptionsData::WindowMode windowMode)
{
    namespace AbilityConstant = QtOhos::enums::ohos::app::ability::AbilityConstant;
    using StartOptions = QOhosStartOptionsData;

    switch (windowMode) {
    case StartOptions::WindowMode::WINDOW_MODE_SPLIT_PRIMARY:
        return std::make_optional(AbilityConstant::WindowMode::WINDOW_MODE_SPLIT_PRIMARY);
    case StartOptions::WindowMode::WINDOW_MODE_SPLIT_SECONDARY:
        return std::make_optional(AbilityConstant::WindowMode::WINDOW_MODE_SPLIT_SECONDARY);
    case StartOptions::WindowMode::WINDOW_MODE_FULLSCREEN:
        return std::make_optional(AbilityConstant::WindowMode::WINDOW_MODE_FULLSCREEN);
    }

    qCWarning(QtForOhos, "%s: got illegal WindowMode: %d", Q_FUNC_INFO, static_cast<int>(windowMode));

    return {};
}

std::optional<QtOhos::enums::ohos::app::ability::contextConstant::ProcessMode> tryMapProcessModeToOhosOrLogWarning(
    QOhosStartOptionsData::ProcessMode processMode)
{
    namespace contextConstant = QtOhos::enums::ohos::app::ability::contextConstant;
    using StartOptions = QOhosStartOptionsData;

    switch (processMode) {
    case StartOptions::ProcessMode::NEW_PROCESS_ATTACH_TO_PARENT:
        return std::make_optional(contextConstant::ProcessMode::NEW_PROCESS_ATTACH_TO_PARENT);
    case StartOptions::ProcessMode::NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM:
        return std::make_optional(contextConstant::ProcessMode::NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM);
    }

    qCWarning(QtForOhos, "%s: got illegal ProcessMode: %d", Q_FUNC_INFO, static_cast<int>(processMode));

    return {};
}

std::optional<QtOhos::enums::ohos::app::ability::contextConstant::StartupVisibility> tryMapStartupVisibilityToOhosOrLogWarning(
    QOhosStartOptionsData::StartupVisibility startupVisibility)
{
    namespace contextConstant = QtOhos::enums::ohos::app::ability::contextConstant;
    using StartOptions = QOhosStartOptionsData;

    switch (startupVisibility) {
    case StartOptions::StartupVisibility::STARTUP_HIDE:
        return std::make_optional(contextConstant::StartupVisibility::STARTUP_HIDE);
    case StartOptions::StartupVisibility::STARTUP_SHOW:
        return std::make_optional(contextConstant::StartupVisibility::STARTUP_SHOW);
    }

    qCWarning(QtForOhos, "%s: got illegal StartupVisibility: %d", Q_FUNC_INFO, static_cast<int>(startupVisibility));

    return {};
}

std::optional<QtOhos::enums::ohos::bundle::bundleManager::SupportWindowMode> tryMapSupportWindowModeToOhosOrLogWarning(
    QOhosStartOptionsData::SupportWindowMode supportWindowMode)
{
    namespace bundleManager = QtOhos::enums::ohos::bundle::bundleManager;
    using StartOptions = QOhosStartOptionsData;

    switch (supportWindowMode) {
    case StartOptions::SupportWindowMode::FULL_SCREEN:
        return std::make_optional(bundleManager::SupportWindowMode::FULL_SCREEN);
    case StartOptions::SupportWindowMode::SPLIT:
        return std::make_optional(bundleManager::SupportWindowMode::SPLIT);
    case StartOptions::SupportWindowMode::FLOATING:
        return std::make_optional(bundleManager::SupportWindowMode::FLOATING);
    }

    qCWarning(QtForOhos, "%s: got illegal SupportWindowMode: %d", Q_FUNC_INFO, static_cast<int>(supportWindowMode));

    return {};
}

QNapi::Array mapSupportWindowModesToJsEnumsArray(
    QOhosJsState &jsState, const QList<QOhosStartOptionsData::SupportWindowMode> &supportWindowModes)
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
    QOhosJsState &jsState, const QOhosStartOptionsData &opts)
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
                                QtOhos::enums::ohos::window::AnimationType::FADE_IN_OUT),
                        }
                    }));
        }
        napiOptions.set("windowCreateParams", QNapi::makeObject(env, windowCreateParamsProps));
    }

    return napiOptions;
}

void requestStartAbilityForResult(
    const QOhosWant &want, std::optional<QOhosStartOptionsData> options,
    QWindow *optInstanceMainWindow, QObject *callerContext,
    std::function<void(std::optional<QOhosStartAbilityResult>)> resultCallback)
{
    struct Context
    {
        std::function<void(std::optional<QOhosStartAbilityResult>)> resultCallback;
        QtOhos::QObjectThreadSafeRef resultConsumerQtContextRef;
    };

    auto context = QtOhos::moveToSharedPtr(
        Context{
            .resultCallback = std::move(resultCallback),
            .resultConsumerQtContextRef = QtOhos::QObjectThreadSafeRef(callerContext),
        });

    auto resultConsumer = [context](std::optional<QOhosAbilityResult> optAbilityResult) {
        if (optAbilityResult.has_value()) {
            auto abilityResult = optAbilityResult.value();
            auto want = abilityResult.want.has_value()
                ? QSharedPointer<QOhosWant>::create(convertWantFromJsonObject(abilityResult.want.value()))
                : nullptr;
            context->resultCallback(QOhosStartAbilityResult{abilityResult.resultCode, want});
        } else {
            context->resultCallback(std::nullopt);
        }
    };

    auto wantJson = convertWantToJsonObject(want);
    auto optInstanceMainWindowRef =
        optInstanceMainWindow != nullptr
           ? std::make_optional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
           : std::nullopt;

    QOhosJsThreadGateway::invoke(
        [context, resultConsumer, wantJson, options, optInstanceMainWindowRef](QOhosJsState &jsState) {
            auto optUiAbility = optInstanceMainWindowRef.has_value()
                ? jsState.tryGetQAbilityByQWindow(optInstanceMainWindowRef.value())
                : jsState.defaultQAbility();
            if (!optUiAbility.has_value()) {
                context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                    [resultConsumer](auto &) {
                        resultConsumer({});
                    });
                return;
            }

            auto arguments = std::vector<QNapi::ValueWrapper>{QOhosJsEnv::toNapiValue(jsState.env(), wantJson)};
            if (options.has_value())
                arguments.push_back(convertStartOptionsToNapiObject(jsState, options.value()));

            optUiAbility.value().evalToPromiseOrRejectOnThrow("context.startAbilityForResult(*)", arguments)
            .onThen(
                [context, resultConsumer](const QOhosCallbackInfo &cbInfo) {
                    QNapi::Object abilityResult = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                    int resultCode = abilityResult.get<QNapi::Number>("resultCode");

                    auto wantOrEmpty = QNapi::getOptionalPropOrEmpty<QNapi::Object>(abilityResult, "want");
                    auto jsonWant = !wantOrEmpty.IsEmpty()
                        ? std::optional<QJsonObject>(QOhosJsEnv::fromNapiValue<QJsonObject>(wantOrEmpty))
                        : std::nullopt;

                    context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                        [resultConsumer, resultCode, jsonWant](auto &) {
                            constexpr int startedAbilityErrorResultCode = -1;
                            resultConsumer(
                                resultCode != startedAbilityErrorResultCode
                                    ? std::optional<QOhosAbilityResult>({resultCode, jsonWant})
                                    : std::nullopt);
                        });
                })
            .onCatch(
                [context, resultConsumer](const QOhosCallbackInfo &cbInfo) {
                    QtOhos::logJsCallbackError(cbInfo, "Got error from startAbilityForResult()");
                    context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                        [resultConsumer](auto &) {
                            resultConsumer({});
                        });
                });
        });
}

class QOhosBaseAbilityContextImpl : public QOhosAbilityContext
{
protected:
    QOhosBaseAbilityContextImpl();

    QByteArray generateUniqueId();

    void startAbilityForResultImpl(
        const QOhosWant &want, QWindow *optInstanceMainWindow,
        std::optional<QOhosStartOptionsData> qpaStartOptions,
        QObject *context, std::function<void(std::optional<QOhosStartAbilityResult>)> callback);

    QByteArray shareDataWithShareKitImpl(
        QWindow *optMainWindow, const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
        QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions);

private:
    QOhosSupplier<QByteArray> m_uniqueIdsGenerator;
    QHash<QByteArray, std::shared_ptr<void>> m_shareDataRequestsHandles;
};

class QOhosDefaultAbilityContextImpl : public QOhosBaseAbilityContextImpl
{
public:
    QOhosDefaultAbilityContextImpl();

    void setDestroyFromSystemEnabled(bool destroyEnabled) override;

    void startAbilityForResult(
        const QOhosWant &want, QObject *context,
        std::function<void(std::optional<QOhosStartAbilityResult>)> callback) override;
    void startAbilityForResult(
        const QOhosWant &want, const QOhosStartOptions &options, QObject *context,
        std::function<void(std::optional<QOhosStartAbilityResult>)> callback) override;
    void startAbilityForResult(
        const QOhosWant &want, const QOhosStartRequest &startRequest, QObject *context,
        std::function<void(std::optional<QOhosStartAbilityResult>)> callback) override;

    QByteArray shareDataWithShareKit(
        const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
        QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions) override;

    bool tryOpenLink(const QString &link) override;
    bool tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options) override;

    void setContinuationActive(bool continuationActive) override;
};

class QOhosAbilityContextImpl : public QOhosBaseAbilityContextImpl
{
public:
    QOhosAbilityContextImpl(QWindow *instanceMainWindow);

    void setDestroyFromSystemEnabled(bool destroyEnabled) override;

    void startAbilityForResult(
        const QOhosWant &want, QObject *context,
        std::function<void(std::optional<QOhosStartAbilityResult>)> callback) override;
    void startAbilityForResult(
        const QOhosWant &want, const QOhosStartOptions &options, QObject *context,
        std::function<void(std::optional<QOhosStartAbilityResult>)> callback) override;
    void startAbilityForResult(
        const QOhosWant &want, const QOhosStartRequest &startRequest, QObject *context,
        std::function<void(std::optional<QOhosStartAbilityResult>)> callback) override;

    QByteArray shareDataWithShareKit(
        const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
        QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions) override;

    bool tryOpenLink(const QString &link) override;
    bool tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options) override;

    void setContinuationActive(bool continuationActive) override;

private:
    QPointer<QWindow> m_instanceMainWindow;
};

std::map<QWindow *, QSharedPointer<QOhosAbilityContextImpl>> abilityContextsMap;

QOhosBaseAbilityContextImpl::QOhosBaseAbilityContextImpl()
    : m_uniqueIdsGenerator(makeUniqueIdsGenerator())
{
}

QByteArray QOhosBaseAbilityContextImpl::generateUniqueId()
{
    return m_uniqueIdsGenerator();
}

void QOhosBaseAbilityContextImpl::startAbilityForResultImpl(
    const QOhosWant &want, QWindow *optInstanceMainWindow,
    std::optional<QOhosStartOptionsData> qpaStartOptions,
    QObject *context, std::function<void(std::optional<QOhosStartAbilityResult>)> callback)
{
    requestStartAbilityForResult(
        want, std::move(qpaStartOptions), optInstanceMainWindow, context,
        std::move(callback));
}

QByteArray QOhosBaseAbilityContextImpl::shareDataWithShareKitImpl(
    QWindow *optMainWindow, const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
    QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions)
{
    auto selfPtr = QPointer<QOhosBaseAbilityContextImpl>(this);
    auto requestId = generateUniqueId();
    auto requestHandle = ShareKit::shareData(
        optMainWindow, records, controllerOptions,
        [selfPtr, requestId]() {
            if (!selfPtr.isNull()) {
                selfPtr->m_shareDataRequestsHandles.remove(requestId);
                Q_EMIT selfPtr->shareKitPanelClosed(requestId);
            }
        },
        [selfPtr, requestId](auto shareOperationResult) {
            if (!selfPtr.isNull())
                Q_EMIT selfPtr->shareKitCompleted(requestId, shareOperationResult);
        }
    );
    m_shareDataRequestsHandles.insert(requestId, requestHandle);

    return requestId;
}

QOhosDefaultAbilityContextImpl::QOhosDefaultAbilityContextImpl() = default;

void QOhosDefaultAbilityContextImpl::setDestroyFromSystemEnabled(bool destroyEnabled)
{
    qCDebug(QtForOhos, "%s: setting destroyEnabled=%d for all instances", Q_FUNC_INFO, destroyEnabled);

    std::vector<QObject *> instancesMainWindows;
    for (const auto &contextEntry : abilityContextsMap)
        instancesMainWindows.push_back(contextEntry.first);

    setDestroyAllowedFlagForAbilityInstances(instancesMainWindows, destroyEnabled);
}

void QOhosDefaultAbilityContextImpl::startAbilityForResult(
    const QOhosWant &want, QObject *context, std::function<void(std::optional<QOhosStartAbilityResult>)> callback)
{
    startAbilityForResultImpl(want, nullptr, std::nullopt, context, std::move(callback));
}

void QOhosDefaultAbilityContextImpl::startAbilityForResult(
    const QOhosWant &want, const QOhosStartOptions &options, QObject *context,
    std::function<void(std::optional<QOhosStartAbilityResult>)> callback)
{
    startAbilityForResultImpl(
        want, nullptr, tryConvertStartOptionsToQpaFunctionsStruct(options), context, std::move(callback));
}

void QOhosDefaultAbilityContextImpl::startAbilityForResult(
    const QOhosWant &want, const QOhosStartRequest &startRequest, QObject *context,
    std::function<void(std::optional<QOhosStartAbilityResult>)> callback)
{
    startAbilityForResultImpl(
        want, nullptr, tryConvertStartRequestToQpaFunctionsStruct(startRequest), context, std::move(callback));
}

QByteArray QOhosDefaultAbilityContextImpl::shareDataWithShareKit(
    const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
    QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions)
{
    return shareDataWithShareKitImpl(nullptr, records, controllerOptions);
}

bool startAbilityByTypeImpl(const QString &appType, const QJsonObject &wantParameters)
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

bool tryOpenLinkImpl(QObject *optInstanceMainWindow, const QString &link, std::optional<bool> appLinkingOnly)
{
    if (optInstanceMainWindow != nullptr && qobject_cast<QWindow *>(optInstanceMainWindow) == nullptr)
        qOhosReportFatalErrorAndAbort("%s: the main window argument is not a QWindow", Q_FUNC_INFO);

    auto optInstanceMainWindowRef = optInstanceMainWindow != nullptr
        ? std::make_optional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
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

void setContinuationActiveImpl(QObject *optInstanceMainWindow, bool continuationActive)
{
    using ContinueState = QtOhos::enums::ohos::app::ability::AbilityConstant::ContinueState;

    auto optInstanceMainWindowRef = optInstanceMainWindow != nullptr
        ? std::make_optional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
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

bool QOhosDefaultAbilityContextImpl::tryOpenLink(const QString &link)
{
    return tryOpenLinkImpl(nullptr, link, {});
}

bool QOhosDefaultAbilityContextImpl::tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options)
{
    const auto &optionsImpl = static_cast<const QOhosOpenLinkOptionsImpl &>(options);
    return tryOpenLinkImpl(nullptr, link, optionsImpl.appLinkingOnly());
}

void QOhosDefaultAbilityContextImpl::setContinuationActive(bool continuationActive)
{
    setContinuationActiveImpl(nullptr, continuationActive);
}

QOhosAbilityContextImpl::QOhosAbilityContextImpl(QWindow *instanceMainWindow)
    : m_instanceMainWindow(instanceMainWindow)
{
    setOnContinueRequestsHandlerForAbilityInstanceWindow(
        instanceMainWindow,
        [self = QPointer<QOhosAbilityContextImpl>(this)](auto request, auto responseConsumer) {
            auto context = QSharedPointer<QOhosOnContinueContextImpl>::create(request.sourceApplicationVersionCode);

            if (!self.isNull())
                Q_EMIT self->continueRequestReceived(context);
            else
                context->setRejectResponse();

            QtOhos::invokeInQtThread(
                [response = context->response(), responseConsumer = std::move(responseConsumer)]() {
                    responseConsumer(response);
                });
        });
}

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::setDestroyFromSystemEnabled(bool destroyEnabled)

    Sets whether the Ability can be automatically destroyed by the system when the user clicks on
    the window's "close" button. If \a destroyEnabled is \c true, the system destroys the Ability
    automatically (and Qt needs to adapt to this). If \a destroyEnabled = \c false, the window is
    not automatically destroyed, but instead standard Qt path for window close is triggered, i.e.
    QWindow::close().

    By default, the flag is set to \c false.

    When called on the default QOhosAbilityContext instance it sets the flag to \a destroyEnabled
    for all Ability instances.
*/
void QOhosAbilityContextImpl::setDestroyFromSystemEnabled(bool destroyEnabled)
{
    if (m_instanceMainWindow.isNull()) {
        qCWarning(QtForOhos, "%s: called on destroyed instance, ignoring", Q_FUNC_INFO);
        return;
    }

    qCDebug(
        QtForOhos, "%s: setting destroyEnabled=%d for window %p",
        Q_FUNC_INFO, destroyEnabled, m_instanceMainWindow.data());

    setDestroyAllowedFlagForAbilityInstances(
        {m_instanceMainWindow.data()}, destroyEnabled);
}

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::startAbilityForResult(const QtOhosAppKit::QOhosWant &want, QObject *context, std::function<void(std::optional<QtOhosAppKit::QOhosStartAbilityResult>)> callback)

    Starts a UIAbility with a given \a want and delivers the result by invoking \a callback on the thread of \a context;
    if \a context is destroyed before the result arrives, \a callback is not invoked. To start the UIAbility, at least bundleName and abilityName properties must be set.
    On success \a callback receives the ability result; on failure (for example, the UIAbility was killed) it receives an empty std::optional.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#startabilityforresult-2}
    {UIAbilityContext.startAbilityForResult}
*/
void QOhosAbilityContextImpl::startAbilityForResult(
    const QOhosWant &want, QObject *context, std::function<void(std::optional<QOhosStartAbilityResult>)> callback)
{
    startAbilityForResultImpl(want, m_instanceMainWindow, std::nullopt, context, std::move(callback));
}

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::startAbilityForResult(const QtOhosAppKit::QOhosWant &want, const QtOhosAppKit::QOhosStartOptions &options, QObject *context, std::function<void(std::optional<QtOhosAppKit::QOhosStartAbilityResult>)> callback)

    Starts a UIAbility with a given \a want and \a options and delivers the result by invoking \a callback on the thread of \a context;
    if \a context is destroyed before the result arrives, \a callback is not invoked. To start the UIAbility, at least bundleName and abilityName properties must be set.
    On success \a callback receives the ability result; on failure (for example, the UIAbility was killed) it receives an empty std::optional.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#startabilityforresult-2}
    {UIAbilityContext.startAbilityForResult}
*/
void QOhosAbilityContextImpl::startAbilityForResult(
    const QOhosWant &want, const QOhosStartOptions &options, QObject *context,
    std::function<void(std::optional<QOhosStartAbilityResult>)> callback)
{
    startAbilityForResultImpl(
        want, m_instanceMainWindow, tryConvertStartOptionsToQpaFunctionsStruct(options), context, std::move(callback));
}

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::startAbilityForResult(const QtOhosAppKit::QOhosWant &want, const QtOhosAppKit::QOhosStartRequest &startRequest, QObject *context, std::function<void(std::optional<QtOhosAppKit::QOhosStartAbilityResult>)> callback)

    Starts a UIAbility with a given \a want and \a startRequest and delivers the result by invoking \a callback on the thread of \a context;
    if \a context is destroyed before the result arrives, \a callback is not invoked. To start the UIAbility, at least bundleName and abilityName properties must be set.
    On success \a callback receives the ability result; on failure (for example, the UIAbility was killed) it receives an empty std::optional.

    The \a startRequest carries the completion handler from start options. Connect to
    QOhosStartRequest::requestSucceeded() and QOhosStartRequest::requestFailed() to receive
    completion handler results.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#startabilityforresult-2}
    {UIAbilityContext.startAbilityForResult}
*/
void QOhosAbilityContextImpl::startAbilityForResult(
    const QOhosWant &want, const QOhosStartRequest &startRequest, QObject *context,
    std::function<void(std::optional<QOhosStartAbilityResult>)> callback)
{
    startAbilityForResultImpl(
        want, m_instanceMainWindow, tryConvertStartRequestToQpaFunctionsStruct(startRequest), context, std::move(callback));
}

/*!
    \fn virtual void QtOhosAppKit::QOhosAbilityContext::shareDataWithShareKit(const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records, QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions) = 0

    Share provided \a records with other applications using an inter-application mechanism called ShareKit. Share Kit panel can be controlled
    with a given \a controllerOptions. When called on the default QOhosAbilityContext instance, it shares \a records using the default UiAbility.

    Returns request identifier which will be passed as an argument of the corresponding \sa shareKitPanelClosed() or \sa shareKitCompleted() signal.
    shareKitPanelClosed() signal is emitted when the sharing panel is closed.
    shareKitCompleted() signal is called when User selects application for sharing (can be called multiple times).

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-guides-V5/share-introduction-V5}{Share Kit}
*/
QByteArray QOhosAbilityContextImpl::shareDataWithShareKit(
    const QList<QSharedPointer<ShareKit::QOhosSharedRecord>> &records,
    QSharedPointer<ShareKit::QOhosShareControllerOptions> controllerOptions)
{
    return shareDataWithShareKitImpl(m_instanceMainWindow, records, controllerOptions);
}

/*!
    \fn virtual bool QtOhosAppKit::QOhosAbilityContext::tryOpenLink(const QString &link) = 0

    Use provided \a link to open application with a deep link.
    Returns true if successful, false otherwise.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-guides/app-linking-startup}{App Linking}
*/
bool QOhosAbilityContextImpl::tryOpenLink(const QString &link)
{
    return tryOpenLinkImpl(m_instanceMainWindow, link, {});
}

/*!
    \fn virtual bool QtOhosAppKit::QOhosAbilityContext::tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options) = 0

    Use provided \a link and \a options to open application with a deep link.
    Returns true if successful, false otherwise.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-guides/app-linking-startup}{App Linking}
*/
bool QOhosAbilityContextImpl::tryOpenLink(const QString &link, const QOhosOpenLinkOptions &options)
{
    const auto &optionsImpl = static_cast<const QOhosOpenLinkOptionsImpl &>(options);
    return tryOpenLinkImpl(m_instanceMainWindow, link, optionsImpl.appLinkingOnly());
}

/*!
    \fn virtual void QtOhosAppKit::QOhosAbilityContext::setContinuationActive(bool continuationActive)

    Sets the mission continuation state of the underlying Ability instance as per the
    \a continuationActive parameter (\c true = active, \c false = inactive).

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/js-apis-inner-application-uiabilitycontext#setmissioncontinuestate10-1}{setMissionContinueState}
*/
void QOhosAbilityContextImpl::setContinuationActive(bool continuationActive)
{
    setContinuationActiveImpl(m_instanceMainWindow, continuationActive);
}

QSharedPointer<QOhosOperationStatus> startAbilityImpl(
    const QOhosWant &want, std::optional<QOhosStartOptionsData> qpaStartOptions)
{
    auto wantJson = convertWantToJsonObject(want);
    bool success = QOhosJsThreadGateway::eval(
        [&](QOhosJsState &jsState) {
            auto optMainUiAbility = jsState.defaultQAbility();
            if (!optMainUiAbility.has_value())
                return false;
            auto mainUiAbility = optMainUiAbility.value();
            if (mainUiAbility.IsEmpty())
                return false;

            auto arguments = std::vector<QNapi::ValueWrapper>{QOhosJsEnv::toNapiValue(jsState.env(), wantJson)};
            if (qpaStartOptions.has_value())
                arguments.push_back(convertStartOptionsToNapiObject(jsState, qpaStartOptions.value()));

            mainUiAbility.call("context.startAbility", arguments);

            // FIXME:
            // * there should be error code taken from a call to JS `startAbility` function
            // * error code should be checked and provided to the returned `operationStatus`
            return true;
        },
        Q_FUNC_INFO);
    return createOperationStatus(success);
}

void startAppProcessImpl(
    const QString &processId, const QOhosWant &requestWant,
    std::optional<QOhosStartOptionsData> qpaStartOptions)
{
    auto requestWantJson = convertWantToJsonObject(requestWant);
    QOhosJsThreadGateway::invokeAndWaitForContinue(
        [&](QOhosJsState &jsState, QOhosTaskPromise<> taskPromise) {
            auto startOptions = qpaStartOptions.has_value()
                ? convertStartOptionsToNapiObject(jsState, qpaStartOptions.value())
                : QNapi::Object();

            auto sharedTaskPromise = QtOhos::moveToSharedPtr(std::move(taskPromise).makeChained(Q_FUNC_INFO));
            jsState.startAppProcess(
                processId.toStdString(),
                QNapi::checkedCast<QNapi::Object>(
                    QOhosJsEnv::toNapiValue(jsState.env(), requestWantJson)),
                startOptions,
                [sharedTaskPromise](QOhosJsState &) {
                    (*sharedTaskPromise)();
                });
        },
        Q_FUNC_INFO);
}

}

/*!
    \class QtOhosAppKit::QOhosAbilityContext
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The QOhosAbilityContext class is to manage native UI Ability context. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5}
    {UIAbilityContext}.
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::newWantInfoReceived(QSharedPointer<QtOhosAppKit::QOhosWantInfo> wantInfo)

    Singal is emitted when an ability gets new \a want. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-app-ability-uiability-V5#uiabilityonnewwant}
    {On New Want}.
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::continueRequestReceived(QSharedPointer<QtOhosAppKit::QOhosOnContinueContext> onContinueContext)

    Signal emitted when the ability receives onContinue request. The signal delivers \a onContinueContext on which User
    can set onContinue response.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-uiability-V13#uiabilityoncontinue}
    {UIAbility onContinue}.

    \sa setAgreeResponse(), setMismatchResponse(), setRejectResponse()
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::shareKitPanelClosed(QByteArray requestId)

    Signal emitted when the sharing panel (invoked via shareDataWithShareKit) closes.
    It corresponds to OH ShareController's "dismiss" event.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section147001858124512}{ShareController}
*/

/*!
    \fn void QtOhosAppKit::QOhosAbilityContext::shareKitCompleted(QByteArray requestId, QSharedPointer<ShareKit::QOhosShareOperationResult> shareOperationResult)

    Signal emitted when share operation is completed.
    Please note that the shareKitCompleted() can be called multiple times as the User can change the target application.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references/share-system-share#section238319917154}{ShareController}
*/

QOhosAbilityContext::QOhosAbilityContext()
{
    addNewWantConsumer(
        this,
        [this](QSharedPointer<detail::WantInfo> wantInfo) {
            Q_EMIT newWantInfoReceived(convertToOhosAppKitWantInfo(wantInfo));
        });
}

/*!
    \fn QSharedPointer<QOhosAbilityContext> QtOhosAppKit::QOhosAbilityContext::getDefaultInstance()

    Returns instance of the class which is not connected to any specific Ability instance. It should
    be used when the application needs to perform some operations without selecting specific
    Ability instance (via the corresponding main window).

    See descriptions of specific class methods for information about their behavior for the default
    instance.
*/
QSharedPointer<QOhosAbilityContext> QOhosAbilityContext::getDefaultInstance()
{
    static auto instance = QSharedPointer<QOhosDefaultAbilityContextImpl>::create();
    return instance;
}

/*!
    \fn QSharedPointer<QOhosAbilityContext> QtOhosAppKit::QOhosAbilityContext::getInstanceForMainWindow(QWindow *instanceMainWindow)

    Returns instance of the class which is connected to Ability instance identified by the
    \a instanceMainWindow. Methods called on the returned object will only affect the
    corresponding Ability instance.
*/
QSharedPointer<QOhosAbilityContext> QOhosAbilityContext::getInstanceForMainWindow(QWindow *instanceMainWindow)
{
    if (instanceMainWindow == nullptr) {
        qCWarning(QtForOhos, "%s: got null QWindow", Q_FUNC_INFO);
        return QSharedPointer<QOhosAbilityContextImpl>::create(nullptr);
    }

    auto abilityContextIter = abilityContextsMap.find(instanceMainWindow);
    if (abilityContextIter == abilityContextsMap.end()) {
        std::tie(abilityContextIter, std::ignore) = abilityContextsMap.emplace(
            instanceMainWindow, QSharedPointer<QOhosAbilityContextImpl>::create(instanceMainWindow));
            QObject::connect(
                instanceMainWindow, &QObject::destroyed,
                [instanceMainWindow](QObject *) {
                    abilityContextsMap.erase(instanceMainWindow);
                });
    }

    return abilityContextIter->second;
}

/*!
    \fn QSharedPointer<QOhosOperationStatus> QtOhosAppKit::startAbility(const QOhosWant &want)

    Starts an Ability for a given \a want. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5#uiabilitycontextstartability}
    {Start Ability}.

    \warning Currently, operation status result is hardcoded as "successful" (even if ability were
    not started).
*/
QSharedPointer<QOhosOperationStatus> startAbility(const QOhosWant &want)
{
    return startAbilityImpl(want, std::nullopt);
}

/*!
    \fn QSharedPointer<QOhosOperationStatus> QtOhosAppKit::startAbility(const QOhosWant &want,
    const QOhosStartOptions &options)

    Starts an Ability for a given \a want and \a options. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5#uiabilitycontextstartability-1}
    {Start Ability}.

    \warning Currently, operation status result is hardcoded as "successful" (even if ability were
    not started).
*/
QSharedPointer<QOhosOperationStatus> startAbility(const QOhosWant &want, const QOhosStartOptions &options)
{
    return startAbilityImpl(want, tryConvertStartOptionsToQpaFunctionsStruct(options));
}

/*!
    \fn QSharedPointer<QOhosOperationStatus> QtOhosAppKit::startAbility(const QOhosWant &want,
    const QOhosStartRequest &startRequest)

    Starts an Ability for a given \a want and \a startRequest.

    The \a startRequest carries the completion handler from start options. Connect to
    QOhosStartRequest::requestSucceeded() and QOhosStartRequest::requestFailed() to receive
    completion handler results. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5#uiabilitycontextstartability-1}
    {Start Ability}.
*/
QSharedPointer<QOhosOperationStatus> startAbility(
    const QOhosWant &want, const QOhosStartRequest &startRequest)
{
    return startAbilityImpl(want, tryConvertStartRequestToQpaFunctionsStruct(startRequest));
}

/*!
    \fn QSharedPointer<QOhosOperationStatus> QtOhosAppKit::startAbilityByType(const QString &appType,
    const QJsonObject &wantParameters)

    Starts an Ability for a given \a appType and \a wantParameters. See
    \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5#uiabilitycontextstartabilitybytype11-1}
    {Start Ability}.

    \return true on success
*/
QSharedPointer<QOhosOperationStatus> startAbilityByType(const QString &appType, const QJsonObject &wantParameters)
{
    bool success = startAbilityByTypeImpl(appType, wantParameters);
    return createOperationStatus(success);
}

/*!
    Starts another instance of the UIAbility used by the Qt app with specified widget inside

    The caller should pass newly created QWidget \a instanceWidget, without any setting any parent, calling show() or winId() on it.
*/
void startNewAbilityInstance(QWidget *instanceWidget)
{
    instanceWidget->show();
}

/*!
    \fn void QtOhosAppKit::startAppProcess(const QString &processId, const QOhosWant &requestWant)

    Starts application process for a given \a processId and \a requestWant.
*/
void startAppProcess(const QString &processId, const QOhosWant &requestWant)
{
    startAppProcessImpl(processId, requestWant, std::nullopt);
}

/*!
    \fn void QtOhosAppKit::startAppProcess(const QString &processId, const QOhosWant &requestWant, const QOhosStartOptions &options)

    Starts application process for a given \a processId, \a requestWant and \a options.
*/
void startAppProcess(const QString &processId, const QOhosWant &requestWant, const QOhosStartOptions &options)
{
    startAppProcessImpl(processId, requestWant, tryConvertStartOptionsToQpaFunctionsStruct(options));
}

/*!
    \fn void QtOhosAppKit::startAppProcess(const QString &processId, const QOhosWant &requestWant, const QOhosStartRequest &startRequest)

    Starts application process for a given \a processId, \a requestWant and \a startRequest.
    The \a startRequest carries the completion handler from start options. Connect to
    QOhosStartRequest::requestSucceeded() and QOhosStartRequest::requestFailed() to receive
    completion handler results.

    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V5/js-apis-inner-application-uiabilitycontext-V5#uiabilitycontextstartability-1}
    {Start Ability}.

    \sa QOhosStartRequest
*/
void startAppProcess(
    const QString &processId, const QOhosWant &requestWant, const QOhosStartRequest &startRequest)
{
    startAppProcessImpl(
        processId, requestWant, tryConvertStartRequestToQpaFunctionsStruct(startRequest));
}

/*!
    \fn void QtOhosAppKit::setAbilityInstanceDestroyEnabled(QWindow *instanceWindow, bool destroyEnabled)

    Sets whether the Ability related with \a instanceWindow can be automatically destroyed by the
    system when the user clicks on the window's "close" button. If \a destroyEnabled is \c true,
    the system destroys the Ability automatically (and Qt needs to adapt to this).
    If \a destroyEnabled = \c false, the window is not automatically destroyed, but instead standard
    Qt path for window close is triggered, i.e. QWindow::close().

    By default, the flag is set to \c false.

    \sa QtOhosAppKit::QOhosAbilityContext::setDestroyFromSystemEnabled()
*/
void setAbilityInstanceDestroyEnabled(QWindow *instanceWindow, bool destroyEnabled)
{
    QOhosAbilityContext::getInstanceForMainWindow(instanceWindow)->setDestroyFromSystemEnabled(destroyEnabled);
}

/*!
    \fn QSharedPointer<QByteArray> tryGetOnContinueData(const QOhosWant &want)

    Tries to get continuation / migration related data that was provided on the source device.
    Returns \c nullptr if no such data found. The data is expected to be stored in the \a want parameters.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-guides/app-continuation-guide}
    {Application Continuation}.
*/
QSharedPointer<QByteArray> tryGetOnContinueData(const QOhosWant &want)
{
    auto key = QString::fromUtf8(qtOnContinueMigrationDataPropertyName);
    if (want.parameters.contains(key)) {
        auto base64String = want.parameters.value(key).toString();
        auto decodedData = QByteArray::fromBase64(base64String.toUtf8());
        return QSharedPointer<QByteArray>::create(decodedData);
    } else {
        return nullptr;
    }
}

/*!
    \class QtOhosAppKit::QOhosOnContinueContext
    \inmodule QtOhosAppKit
    \since 5.12.12
    \brief The QOhosOnContinueContext class manages onContinue context. It provides system
    data, like source application version code and set the onContinue result that is requested by the system.
    See \l {https://developer.huawei.com/consumer/en/doc/harmonyos-references-V13/js-apis-app-ability-uiability-V13#uiabilityoncontinue}
    {UIAbility onContinue}.
*/
QtOhosAppKit::QOhosOnContinueContext::QOhosOnContinueContext() = default;
QtOhosAppKit::QOhosOnContinueContext::~QOhosOnContinueContext() = default;

QOhosOpenLinkOptions::QOhosOpenLinkOptions() = default;
QOhosOpenLinkOptions::~QOhosOpenLinkOptions() = default;

QSharedPointer<QOhosOpenLinkOptions> createOpenLinkOptions()
{
    return QSharedPointer<QOhosOpenLinkOptionsImpl>::create();
}

}

QT_END_NAMESPACE
