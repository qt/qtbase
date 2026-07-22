// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSQABILITYINSTANCESMANAGER_H
#define QOHOSQABILITYINSTANCESMANAGER_H

#include <QtCore/private/qnapi_p.h>
#include <QtCore/qobject.h>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <qohosplugincore.h>
#include <string>

QT_BEGIN_NAMESPACE

namespace QtOhos {

class QUiAbilityPeerBackend
{
public:
    enum class CloseAbilityRequestSource
    {
        OnPrepareToTerminate,
        WindowWillClose,
    };

    enum class CloseAbilityRequestResolution
    {
        Close,
        DontClose,
    };

    virtual QNapi::Promise handleCloseRequestFromSystem(
        JsState &jsState, const std::string &logContextStr, CloseAbilityRequestSource requestSource,
        std::function<QNapi::Value(JsState &, CloseAbilityRequestResolution)> promiseValueFactory) = 0;

    virtual void handleOnContinueRequestFromSystem(
        JsState &jsState, QNapi::Object wantParamsObj,
        QOhosConsumer<JsState &, QOhosAbilityOnContinueResult> resultConsumer) = 0;

protected:
    QUiAbilityPeerBackend();
};

struct QAbilityInstancesManager
{
public:
    virtual ~QAbilityInstancesManager();

    static bool isQtInternalWantFromThisProcess(QNapi::Object want);

    static void setLaunchParamOnAbilityObject(JsState &jsState, QNapi::Object ability, QNapi::Object launchParam);

    virtual std::shared_ptr<QAbilityEngine> abilityEngine() = 0;

    virtual bool isWantFromThisApp(QNapi::Object appQAbility, QNapi::Object want) const = 0;

    virtual std::optional<std::string> tryGetQAbilityInstanceIdFromWant(QNapi::Object appQAbility, QNapi::Object want) const = 0;
    virtual std::string getQAbilityInstanceIdOrPendingAutoStartedId(QNapi::Object qAbility) const = 0;

    virtual std::optional<std::string> pendingAutoStartedInstanceId() const = 0;

    virtual void registerPendingAutoStartedInstance() = 0;

    virtual void startNewInstance(
        QNapi::Object baseQAbility, QObjectThreadSafeRef qwindow,
        QNapi::Object optStartOptions,
        std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> startupNotifyFunc) = 0;

    virtual void handleStartedUiInstance(JsState &jsState, QNapi::Object qAbility, QNapi::Object windowStage) = 0;

    virtual std::shared_ptr<QUiAbilityPeerBackend> getAbilityPeerBackend(std::shared_ptr<QUiAbilityPeer> uiAbilityPeer) = 0;

protected:
    QAbilityInstancesManager();
};

std::shared_ptr<QAbilityInstancesManager> makeQAbilityInstancesManager(
    std::shared_ptr<QAbilityEngine> abilityEngine,
    std::function<void(JsState &, std::shared_ptr<QAbilityPeer>)> autoStartedInstanceStartupNotifyFunc);

}

QT_END_NAMESPACE

#endif // QOHOSQABILITYINSTANCESMANAGER_H
