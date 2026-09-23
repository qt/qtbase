// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosapppermissions_p.h"
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QOhosAppPermissions {

namespace {

void tryGetBundleAccessTokenIdWithConsumer(
    QOhosJsState &jsState, QOhosConsumer<QOhosJsState &, std::optional<int>> resultConsumer)
{
    auto bundleFlags = jsState.eval<QNapi::Number>(
        "@ohos.bundle.bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION");

    auto sharedResultConsumer = QtOhos::moveToSharedPtr(std::move(resultConsumer));
    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.bundle.bundleManager.getBundleInfoForSelf(*)", {bundleFlags})
    .onThen([sharedResultConsumer](const QOhosCallbackInfo &cbInfo) {
        QNapi::Object bundleInfo = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
        (*sharedResultConsumer)(
            cbInfo.jsState(),
            std::optional<int>(bundleInfo.eval<QNapi::Number>("appInfo.accessTokenId")));
    })
    .onCatch([sharedResultConsumer](const QOhosCallbackInfo &cbInfo) {
        QtOhos::logJsCallbackError(cbInfo, "Got error from getBundleInfoForSelf()");
        (*sharedResultConsumer)(cbInfo.jsState(), {});
    });
}

void checkAppPermissionStatusGrantedWithConsumer(
    QOhosJsState &jsState, int bundleAccessToken, const std::string &permissionName,
    QOhosConsumer<QOhosJsState &, bool> resultConsumer)
{
    auto sharedResultConsumer = QtOhos::moveToSharedPtr(std::move(resultConsumer));
    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.abilityAccessCtrl.createAtManager().checkAccessToken(*)",
        {bundleAccessToken, permissionName})
    .onThen([sharedResultConsumer](const QOhosCallbackInfo &cbInfo) {
        auto status = cbInfo.getFirstArg<QNapi::Number>(Q_FUNC_INFO);
        auto permissionGrantedStatus = cbInfo.jsState().eval<QNapi::Number>(
            "@ohos.abilityAccessCtrl.GrantStatus.PERMISSION_GRANTED");
        (*sharedResultConsumer)(cbInfo.jsState(), status == permissionGrantedStatus);
    })
    .onCatch([sharedResultConsumer](const QOhosCallbackInfo &cbInfo) {
        QtOhos::logJsCallbackError(cbInfo, "Got error from checkAccessToken()");
        (*sharedResultConsumer)(cbInfo.jsState(), false);
    });
}

}

void checkAppPermissionGrantedWithConsumer(
    QOhosJsState &jsState, const std::string &permissionName,
    QOhosConsumer<QOhosJsState &, bool> resultConsumer)
{
    tryGetBundleAccessTokenIdWithConsumer(
        jsState,
        [permissionName, resultConsumer = std::move(resultConsumer)](
            QOhosJsState &jsState, std::optional<int> bundleAccessTokenId) mutable {
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
    QOhosJsState &jsState, const std::string &permissionName,
    QOhosConsumer<QOhosJsState &, bool> resultConsumer)
{
    auto optQAbility = jsState.defaultQAbility();
    if (!optQAbility) {
        qOhosPrintfError(
            "Cannot request permission: '%s': no default QAbility.", permissionName.c_str());
        resultConsumer(jsState, false);
        return;
    }

    requestAppPermissionFromUser(
        jsState, optQAbility.value(), permissionName, std::move(resultConsumer));
}

void requestAppPermissionFromUser(
    QOhosJsState &jsState, QNapi::Object qAbility, const std::string &permissionName,
    QOhosConsumer<QOhosJsState &, bool> resultConsumer)
{
    requestAppPermissionsFromUserWithResult(
        jsState, qAbility, {permissionName},
        [resultConsumer = std::move(resultConsumer)](QOhosJsState &jsState, std::vector<QOhosAppPermissions::AppPermissionResult> result) {
            resultConsumer(jsState, result.size() == 1 && result.front().permissionGranted);
        });
}

void requestAppPermissionsFromUserWithResult(
    QOhosJsState &jsState, const std::vector<std::string> &permissionNames,
    QOhosConsumer<QOhosJsState &, std::vector<AppPermissionResult>> resultConsumer)
{
    auto optQAbility = jsState.defaultQAbility();
    if (!optQAbility) {
        qOhosPrintfError("Cannot request permissions from user: no default QAbility.");
        resultConsumer(
            jsState,
            std::vector<AppPermissionResult>(
                permissionNames.size(),
                AppPermissionResult {
                    .permissionGranted = false,
                    .dialogShown = false
                }));
        return;
    }

    requestAppPermissionsFromUserWithResult(
        jsState, optQAbility.value(), permissionNames, std::move(resultConsumer));
}

void requestAppPermissionsFromUserWithResult(
    QOhosJsState &jsState, QNapi::Object qAbility,
    const std::vector<std::string> &permissionNames,
    QOhosConsumer<QOhosJsState &, std::vector<AppPermissionResult>> resultConsumer)
{
    auto sharedResultConsumer = QtOhos::moveToSharedPtr(std::move(resultConsumer));
    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.abilityAccessCtrl.createAtManager().requestPermissionsFromUser(*)",
        {
            qAbility.eval<QNapi::Object>("context"),
            QNapi::makeArray(
                jsState.env(),
                std::vector<QNapi::ValueWrapper>(permissionNames.begin(), permissionNames.end()))
        })
    .onThen(
        [permissionNames, sharedResultConsumer](const QOhosCallbackInfo &cbInfo) {
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
            (*sharedResultConsumer)(cbInfo.jsState(), appPermissionResults);
        })
    .onCatch(
        [permissionNames, sharedResultConsumer](const QOhosCallbackInfo &cbInfo) {
            QtOhos::logJsCallbackError(cbInfo, "Got error from requestPermissionsFromUser()");
            const std::size_t totalPermissions = permissionNames.size();
            std::vector<AppPermissionResult> appPermissionResults(
                totalPermissions,
                AppPermissionResult {
                    .permissionGranted = false,
                    .dialogShown = false
                });
            (*sharedResultConsumer)(cbInfo.jsState(), appPermissionResults);
        });
}

void requestAppPermissionsOnSetting(
    QOhosJsState &jsState, const std::vector<std::string> &permissionNames,
    QOhosConsumer<QOhosJsState &, std::vector<bool>> resultConsumer)
{
    auto optQAbility = jsState.defaultQAbility();
    if (!optQAbility) {
        qOhosPrintfError("Cannot request permissions on setting: no default QAbility.");
        resultConsumer(jsState, std::vector<bool>(permissionNames.size(), false));
        return;
    }

    requestAppPermissionsOnSetting(
        jsState, optQAbility.value(), permissionNames, std::move(resultConsumer));
}

void requestAppPermissionsOnSetting(
    QOhosJsState &jsState, QNapi::Object qAbility,
    const std::vector<std::string> &permissionNames,
    QOhosConsumer<QOhosJsState &, std::vector<bool>> resultConsumer)
{
    auto sharedResultConsumer = QtOhos::moveToSharedPtr(std::move(resultConsumer));
    jsState.evalToPromiseOrRejectOnThrow(
        "@ohos.abilityAccessCtrl.createAtManager().requestPermissionOnSetting(*)",
        {
            qAbility.eval<QNapi::Object>("context"),
            QNapi::makeArray(
                jsState.env(),
                std::vector<QNapi::ValueWrapper>(permissionNames.begin(), permissionNames.end()))
        })
    .onThen(
        [permissionNames, sharedResultConsumer](const QOhosCallbackInfo &cbInfo) {
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

            (*sharedResultConsumer)(cbInfo.jsState(), settingsPermissionResults);
        })
    .onCatch(
        [permissionNames, sharedResultConsumer](const QOhosCallbackInfo &cbInfo) {
            QtOhos::logJsCallbackError(cbInfo, "Got error from requestPermissionOnSetting()");
            std::vector<bool> settingsPermissionResults(permissionNames.size(), false);
            (*sharedResultConsumer)(cbInfo.jsState(), settingsPermissionResults);
        });
}

}

QT_END_NAMESPACE
