// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosuiabilitydelegate_p.h"

#include <QtHarmonyExtras/private/qohosharmonyextrasenums_p.h>

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>

#include <unordered_map>

QT_BEGIN_NAMESPACE

namespace QtHarmonyExtras::Private {

namespace {

using OnContinueResult = QtOhosHarmonyExtras::enums::ohos::app::ability::AbilityConstant::OnContinueResult;

struct UiAbilityDelegateState
{
    std::shared_ptr<void> delegateHandle;
    std::unordered_map<QHarmonyJsObjectRef, std::shared_ptr<std::function<napi_value(QHarmonyAppJsContext &, napi_value)>>> onContinueHandlers;
};

UiAbilityDelegateState &uiAbilityDelegateState()
{
    return QOhosJsThreadOps::instance().jsState().getAttachedObjectWithLazyCreate<UiAbilityDelegateState>();
}

Napi::Value callOnContinueHandler(const QOhosCallbackInfo &cbInfo)
{
    QNapi::Object ability;
    QNapi::Object wantParams;
    cbInfo.getLeadingArgs(Q_FUNC_INFO, ability, wantParams);

    auto &appJsContext = QHarmonyAppJsContext::fromEnv(cbInfo.Env());
    auto &state = uiAbilityDelegateState();

    auto handlerIter = state.onContinueHandlers.find(QHarmonyJsObjectRef::fromValue(appJsContext, ability));
    if (handlerIter == state.onContinueHandlers.end()) {
        qOhosPrintfDebug("%s: no continuation handler for the ability, rejecting", Q_FUNC_INFO);
        return cbInfo.jsState().mapOhosEnumToJs(OnContinueResult::REJECT);
    }

    auto handler = handlerIter->second;
    return Napi::Value(cbInfo.Env(), (*handler)(appJsContext, wantParams));
}

}

std::shared_ptr<void> setUiAbilityOnContinueHandler(
    QHarmonyAppJsContext &appJsContext, QHarmonyJsObjectRef abilityRef,
    std::function<napi_value(QHarmonyAppJsContext &, napi_value)> handler)
{
    auto &state = uiAbilityDelegateState();
    if (state.onContinueHandlers.count(abilityRef) != 0) {
        qOhosPrintfError("%s: a continuation handler is already set for the ability, not setting another", Q_FUNC_INFO);
        return nullptr;
    }

    if (state.onContinueHandlers.empty()) {
        auto delegate = QNapi::Object::New(appJsContext.env());
        delegate.set(
            "onContinue",
            [](const QOhosCallbackInfo &cbInfo) {
                return callOnContinueHandler(cbInfo);
            });
        state.delegateHandle = appJsContext.addUiAbilityDelegate(delegate);
    }

    state.onContinueHandlers.emplace(abilityRef, QtOhos::moveToSharedPtr(std::move(handler)));
    return QtOhos::makeDestroyNotifier(
        [abilityRef]() {
            auto &state = uiAbilityDelegateState();
            state.onContinueHandlers.erase(abilityRef);
            if (state.onContinueHandlers.empty())
                state.delegateHandle.reset();
        });
}

}

QT_END_NAMESPACE
