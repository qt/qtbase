// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosapppermissions_p.h"
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <optional>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

namespace QOhosAppPermissions {

namespace {

void tryGetBundleAccessTokenIdWithConsumer(
    QtOhos::JsState &jsState, QOhosConsumer<QtOhos::JsState &, std::optional<int>> resultConsumer)
{
    auto bundleFlags = jsState.eval<QNapi::Number>(
        "@ohos.bundle.bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION");

    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.bundle.bundleManager.getBundleInfoForSelf(*)", {bundleFlags})
    .withContext(std::move(resultConsumer))
    .onThenWithContext([](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
        QNapi::Object bundleInfo = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
        resultConsumer(
            cbInfo.jsState(),
            std::optional<int>(bundleInfo.eval<QNapi::Number>("appInfo.accessTokenId")));
    })
    .onCatchWithContext([](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
        QtOhos::logJsCallbackError(cbInfo, "Got error from getBundleInfoForSelf()");
        resultConsumer(cbInfo.jsState(), {});
    });
}

void checkAppPermissionStatusGrantedWithConsumer(
    QtOhos::JsState &jsState, int bundleAccessToken, const std::string &permissionName,
    QOhosConsumer<QtOhos::JsState &, bool> resultConsumer)
{
    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.abilityAccessCtrl.createAtManager().checkAccessToken(*)",
        {bundleAccessToken, permissionName})
    .withContext(std::move(resultConsumer))
    .onThenWithContext([](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
        auto status = cbInfo.getFirstArg<QNapi::Number>(Q_FUNC_INFO);
        auto permissionGrantedStatus = cbInfo.jsState().eval<QNapi::Number>(
            "@ohos.abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED");
        resultConsumer(cbInfo.jsState(), status == permissionGrantedStatus);
    })
    .onCatchWithContext([](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
        QtOhos::logJsCallbackError(cbInfo, "Got error from checkAccessToken()");
        resultConsumer(cbInfo.jsState(), false);
    });
}

}

void checkAppPermissionGrantedWithConsumer(
    QtOhos::JsState &jsState, const std::string &permissionName,
    QOhosConsumer<QtOhos::JsState &, bool> resultConsumer)
{
    tryGetBundleAccessTokenIdWithConsumer(
        jsState,
        [permissionName, resultConsumer = std::move(resultConsumer)](
            QtOhos::JsState &jsState, std::optional<int> bundleAccessTokenId) mutable {
            if (bundleAccessTokenId.has_value()) {
                checkAppPermissionStatusGrantedWithConsumer(
                    jsState, bundleAccessTokenId.value(), permissionName,
                    std::move(resultConsumer));
            } else {
                qOhosPrintfError(
                    "Cannot check permission: '%s': cannot get bundle access token id.",
                    permissionName.c_str());
                resultConsumer(jsState, false);
            }
        });
}

void requestAppPermissionFromUser(
    QtOhos::JsState &jsState, const std::string &permissionName,
    QOhosConsumer<QtOhos::JsState &, bool> resultConsumer)
{
    return requestAppPermissionFromUser(
        jsState, jsState.defaultQAbilityPeer(), permissionName,
        std::move(resultConsumer));
}

void requestAppPermissionFromUser(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> abilityPeer,
    const std::string &permissionName,
    QOhosConsumer<QtOhos::JsState &, bool> resultConsumer)
{
    requestAppPermissionsFromUserWithResult(
        jsState, abilityPeer, {permissionName},
        [resultConsumer = std::move(resultConsumer)](QtOhos::JsState &jsState, std::vector<QOhosAppPermissions::AppPermissionResult> result) {
            resultConsumer(jsState, result.size() == 1 && result.front().permissionGranted);
        });
}

void requestAppPermissionsFromUserWithResult(
    QtOhos::JsState &jsState, const std::vector<std::string> &permissionNames,
    QOhosConsumer<QtOhos::JsState &, std::vector<AppPermissionResult>> resultConsumer)
{
    requestAppPermissionsFromUserWithResult(
        jsState, jsState.defaultQAbilityPeer(), permissionNames,
        std::move(resultConsumer));
}

void requestAppPermissionsFromUserWithResult(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> abilityPeer,
    const std::vector<std::string> &permissionNames,
    QOhosConsumer<QtOhos::JsState &, std::vector<AppPermissionResult>> resultConsumer)
{
    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.abilityAccessCtrl.createAtManager().requestPermissionsFromUser(*)",
        {
            abilityPeer->qAbility().eval<QNapi::Object>("context"),
            QNapi::makeArray(
                jsState.env(),
                std::vector<QNapi::ValueWrapper>(permissionNames.begin(), permissionNames.end()))
        })
    .withContext(std::move(resultConsumer))
    .onThenWithContext(
        [permissionNames](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
            QNapi::Object resultObj = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);

            auto resultPermissionsNames =
                QNapi::getArrayElements<std::vector<std::string>, QNapi::String>(
                    resultObj.get<QNapi::Array>("permissions"));
            auto resultAuthResults =
                QNapi::getArrayElements<std::vector<int>, QNapi::Number>(
                    resultObj.get<QNapi::Array>("authResults"));

            auto dialogShownResultsOrEmpty = QNapi::getOptionalPropOrEmpty<QNapi::Array>(resultObj, "dialogShownResults");
            auto dialogShownResultsVector =
                !dialogShownResultsOrEmpty.IsEmpty()
                    ? QNapi::getArrayElements<std::vector<bool>, QNapi::Boolean>(dialogShownResultsOrEmpty)
                    : std::vector<bool>();

            int permissionGrantedStatus = cbInfo.jsState().eval<QNapi::Number>(
                "@ohos.abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED");

            const std::size_t totalPermissions = permissionNames.size();
            std::vector<AppPermissionResult> appPermissionResults(
                totalPermissions,
                AppPermissionResult {
                    .permissionGranted = false,
                    .dialogShown = false
                });
            if (resultPermissionsNames.size() == totalPermissions && resultAuthResults.size() == totalPermissions
                    && dialogShownResultsVector.size() == totalPermissions) {
                for (const auto &permissionName : permissionNames) {
                    const auto it = std::find(resultPermissionsNames.begin(), resultPermissionsNames.end(), permissionName);
                    if (it != resultPermissionsNames.end()) {
                        const int index = std::distance(resultPermissionsNames.begin(), it);
                        appPermissionResults[index].permissionGranted = (resultAuthResults[index] == permissionGrantedStatus);
                        appPermissionResults[index].dialogShown = dialogShownResultsVector[index];
                    }
                }
            }
            resultConsumer(cbInfo.jsState(), appPermissionResults);
        })
    .onCatchWithContext(
        [permissionNames](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
            QtOhos::logJsCallbackError(cbInfo, "Got error from requestPermissionsFromUser()");
            const std::size_t totalPermissions = permissionNames.size();
            std::vector<AppPermissionResult> appPermissionResults(
                totalPermissions,
                AppPermissionResult {
                    .permissionGranted = false,
                    .dialogShown = false
                });
            resultConsumer(cbInfo.jsState(), appPermissionResults);
        });
}

void requestAppPermissionsOnSetting(
    QtOhos::JsState &jsState, const std::vector<std::string> &permissionNames,
    QOhosConsumer<QtOhos::JsState &, std::vector<bool>> resultConsumer)
{
    requestAppPermissionsOnSetting(
        jsState, jsState.defaultQAbilityPeer(), permissionNames, std::move(resultConsumer));
}

void requestAppPermissionsOnSetting(
    QtOhos::JsState &jsState, std::shared_ptr<QtOhos::QAbilityPeer> abilityPeer,
    const std::vector<std::string> &permissionNames,
    QOhosConsumer<QtOhos::JsState &, std::vector<bool>> resultConsumer)
{
    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.abilityAccessCtrl.createAtManager().requestPermissionOnSetting(*)",
        {
            abilityPeer->qAbility().eval<QNapi::Object>("context"),
            QNapi::makeArray(
                jsState.env(),
                std::vector<QNapi::ValueWrapper>(permissionNames.begin(), permissionNames.end()))
        })
    .withContext(std::move(resultConsumer))
    .onThenWithContext(
        [permissionNames](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
            QNapi::Array resultArray = cbInfo.getFirstArg<QNapi::Array>(Q_FUNC_INFO);

            auto resultAuthResults = QNapi::getArrayElements<std::vector<int>, QNapi::Number>(resultArray);

            int permissionGrantedStatus = cbInfo.jsState().eval<QNapi::Number>(
                "@ohos.abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED");

            const std::size_t totalPermissions = permissionNames.size();
            std::vector<bool> settingsPermissionResults(totalPermissions, false);
            if (resultAuthResults.size() == totalPermissions) {
                for (std::size_t permIndex = 0; permIndex < totalPermissions; permIndex++)
                     settingsPermissionResults[permIndex] = (resultAuthResults[permIndex] == permissionGrantedStatus);
            }

            resultConsumer(cbInfo.jsState(), settingsPermissionResults);
        })
    .onCatchWithContext(
        [permissionNames](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
            QtOhos::logJsCallbackError(cbInfo, "Got error from requestPermissionOnSetting()");
            std::vector<bool> settingsPermissionResults(permissionNames.size(), false);
            resultConsumer(cbInfo.jsState(), settingsPermissionResults);
        });
}

}

QT_END_NAMESPACE
