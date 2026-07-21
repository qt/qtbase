// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohospermissionshelperimpl.h"

#include <QtCore/private/qohoslogger_p.h>
#include <optional>
#include <qohosapppermissions_p.h>
#include <qohosenums.h>
#include <qohosutils.h>
#include <render/qwindowproxyregistry.h>
#include <utility>

QT_BEGIN_NAMESPACE

namespace QtOhos {

namespace {

QWindow *getFocusedWindowOrNull()
{
    auto focusedWindows = QWindowProxyRegistry::instance().queryWindowsWithSystemWindowAndFocus();
    return !focusedWindows.empty() ? focusedWindows.front() : nullptr;
}

std::optional<Qt::PermissionStatus> tryMapPermissionStatusFromOhos(
    QtOhos::enums::ohos::abilityAccessCtrl::PermissionStatus ohosPermissionStatus)
{
    using OhosPermissionStatus = QtOhos::enums::ohos::abilityAccessCtrl::PermissionStatus;

    switch (ohosPermissionStatus) {
    case OhosPermissionStatus::GRANTED:
        return Qt::PermissionStatus::Granted;
    case OhosPermissionStatus::NOT_DETERMINED:
        return Qt::PermissionStatus::Undetermined;
    case OhosPermissionStatus::DENIED:
    case OhosPermissionStatus::INVALID:
    case OhosPermissionStatus::RESTRICTED:
        return Qt::PermissionStatus::Denied;
    }

    return {};
}

std::optional<QtOhos::enums::ohos::abilityAccessCtrl::PermissionStatus> tryGetSelfPermissionStatus(
    QtOhos::JsState &jsState, const std::string &permissionName)
{
    using OhosPermissionStatus = QtOhos::enums::ohos::abilityAccessCtrl::PermissionStatus;

    std::optional<QNapi::Number> optJsPermissionStatusValue;
    try {
        optJsPermissionStatusValue = jsState.eval<QNapi::Number>(
            "@ohos.abilityAccessCtrl.createAtManager().getSelfPermissionStatus(*)",
            {permissionName});
    } catch (const Napi::Error &error) {
        qOhosPrintfError("%s: getSelfPermissionStatus() failed: %s", Q_FUNC_INFO, error.what());
    }

    return qAndThen(
        optJsPermissionStatusValue,
        [&](auto jsPermissionStatusValue) {
            return jsState.tryMapOhosEnumFromJs<OhosPermissionStatus>(jsPermissionStatusValue);
        });
}

class QOhosPermissionsHelperImpl : public QOhosPermissionsHelper
{
public:
    QList<Qt::PermissionStatus> checkStatusesOfPermissions(const QStringList &permissionNames) const override;

    void requestPermissionsFromUserIfNeeded(
        const QStringList &permissionNames, QObject *resultConsumerContext,
        QOhosConsumer<QList<QOhosPermissionsHelper::PermissionRequestResult>> resultConsumer) override;

    void requestPermissionsOnSettingIfNeeded(
        const QStringList &permissionNames, QObject *resultConsumerContext,
        QOhosConsumer<QList<bool>> resultConsumer) override;
};

QList<Qt::PermissionStatus> QOhosPermissionsHelperImpl::checkStatusesOfPermissions(
    const QStringList &permissionNames) const
{
    if (permissionNames.isEmpty())
        return {};

    return QtOhos::evalInJsThread(
        [&](QtOhos::JsState &jsState) {
            QList<Qt::PermissionStatus> statuses;
            for (const auto &permissionName : permissionNames) {
                auto optPermissionStatus = qAndThen(
                    tryGetSelfPermissionStatus(jsState, permissionName.toStdString()),
                    &tryMapPermissionStatusFromOhos);
                statuses.append(optPermissionStatus.value_or(Qt::PermissionStatus::Denied));
            }
            return statuses;
        });
}

void QOhosPermissionsHelperImpl::requestPermissionsFromUserIfNeeded(
    const QStringList &qPermissionNames, QObject *resultConsumerContext,
    QOhosConsumer<QList<QOhosPermissionsHelper::PermissionRequestResult>> resultConsumer)
{
    std::vector<std::string> permissionNames(qPermissionNames.size());
    std::transform(
        qPermissionNames.constBegin(), qPermissionNames.constEnd(),
        permissionNames.begin(), [](const QString &str) { return str.toStdString(); });

    struct Context {
        QtOhos::QObjectThreadSafeRef resultConsumerQtContextRef;
        QOhosConsumer<QList<PermissionRequestResult>> resultConsumer;
    };

    QOhosConsumer<QList<QOhosPermissionsHelper::PermissionRequestResult>> qResultConsumer =
        [resultConsumer = std::move(resultConsumer)](const auto &inputResult) {
            QList<QOhosPermissionsHelper::PermissionRequestResult> result;
            for (const auto &inputEntry : inputResult) {
                result.append(
                    QOhosPermissionsHelper::PermissionRequestResult{
                        .permissionGranted = inputEntry.permissionGranted,
                        .dialogShown = inputEntry.dialogShown,
                    });
                resultConsumer(std::move(result));
            }
        };

    auto context = QtOhos::moveToSharedPtr(
        Context{
            .resultConsumerQtContextRef = QtOhos::QObjectThreadSafeRef(resultConsumerContext),
            .resultConsumer = std::move(qResultConsumer),
        });

    QObject *optInstanceMainWindow = getFocusedWindowOrNull();
    auto optInstanceMainWindowRef =
        optInstanceMainWindow != nullptr
           ? makeQOhosOptional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
           : makeEmptyQOhosOptional();

    QtOhos::invokeInJsThread(
        [context, permissionNames, optInstanceMainWindowRef](QtOhos::JsState &jsState) {
            auto optAbilityPeer = QtOhos::tryMapOptMainWindowToAbilityPeer(jsState, optInstanceMainWindowRef);
            if (!optAbilityPeer) {
                context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                    [context, permissionNames](auto &) {
                        const size_t totalPermissions = permissionNames.size();
                        QList<PermissionRequestResult> appPermissionResults(
                            totalPermissions,
                            PermissionRequestResult {
                                .permissionGranted = false,
                                .dialogShown = false,
                            });
                        context->resultConsumer(appPermissionResults);
                    });
                return;
            }
            QOhosAppPermissions::requestAppPermissionsFromUserWithResult(
                jsState, optAbilityPeer, permissionNames,
                [context](QtOhos::JsState &, std::vector<QOhosPermissionsHelper::PermissionRequestResult> result) {
                    context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                        [context, appPermissionResults = result](auto &) {
                            QList<PermissionRequestResult> appPermResults;
                            for (const auto &appPermResult: appPermissionResults)
                                appPermResults.push_back(
                                    PermissionRequestResult {
                                        .permissionGranted = appPermResult.permissionGranted,
                                        .dialogShown = appPermResult.dialogShown,
                                    });
                            context->resultConsumer(appPermResults);
                        });
                });
        });
}

void QOhosPermissionsHelperImpl::requestPermissionsOnSettingIfNeeded(
    const QStringList &qPermissionNames, QObject *resultConsumerContext,
    QOhosConsumer<QList<bool>> resultConsumer)
{
    std::vector<std::string> permissionNames(qPermissionNames.size());
    std::transform(
        qPermissionNames.constBegin(), qPermissionNames.constEnd(),
        permissionNames.begin(), [](const QString &str) { return str.toStdString(); });

    struct Context {
        QtOhos::QObjectThreadSafeRef resultConsumerQtContextRef;
        QOhosConsumer<QList<bool>> resultConsumer;
    };

    auto context = QtOhos::moveToSharedPtr(
        Context{
            .resultConsumerQtContextRef = QtOhos::QObjectThreadSafeRef(resultConsumerContext),
            .resultConsumer = std::move(resultConsumer),
        });

    QObject *optInstanceMainWindow = getFocusedWindowOrNull();
    auto optInstanceMainWindowRef =
        optInstanceMainWindow != nullptr
           ? makeQOhosOptional(QtOhos::QObjectThreadSafeRef(optInstanceMainWindow))
           : makeEmptyQOhosOptional();

    QtOhos::invokeInJsThread(
        [context, permissionNames, optInstanceMainWindowRef](auto &jsState) {
            auto optAbilityPeer = QtOhos::tryMapOptMainWindowToAbilityPeer(jsState, optInstanceMainWindowRef);
            if (!optAbilityPeer) {
                context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                    [context, permissionNames](auto &) {
                        QList<bool> settingPermissionResults(permissionNames.size(), false);
                        context->resultConsumer(settingPermissionResults);
                    });
                return;
            }
            QOhosAppPermissions::requestAppPermissionsOnSetting(
                jsState, optAbilityPeer ? optAbilityPeer : jsState.defaultQAbilityPeer(),
                permissionNames,
                [context](QtOhos::JsState &, std::vector<bool> permissionsGranted) {
                    context->resultConsumerQtContextRef.visitInQtThreadIfAlive(
                        [context, permissionsGranted](auto &) {
                            QList<bool> qPermissionsGranted(
                                permissionsGranted.begin(), permissionsGranted.end());
                            context->resultConsumer(qPermissionsGranted);
                        });
                });
        });
    }
}

QOhosPermissionsHelper *getQOhosPermissionsHelperImpl()
{
    static QOhosPermissionsHelperImpl permissionsHelperImpl;
    return &permissionsHelperImpl;
}

}

QT_END_NAMESPACE
