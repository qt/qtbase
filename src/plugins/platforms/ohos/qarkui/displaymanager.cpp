// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qarkui/displaymanager.h>

#include <QtCore/private/qcore_ohos_p.h>
#include <memory>
#include <qohosjsutils.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {

namespace {

const std::string displayCallbackNameChangeEvent = "change";
const std::string displayCallbackNameAddEvent = "add";
const std::string displayCallbackNameRemoveEvent = "remove";

}

void QOhosDisplayManager::registerDisplayCallbackListener(
    QNapi::Object displayModule, const std::string &eventName,
    QOhosConsumer<QtOhos::JsState &, JsDisplayId> handleFunction)
{
    m_destroyNotifiers.push_back(
        QtOhos::registerOnOffMethodsBasedEventHandler(
            displayModule, eventName,
            [handleFunction = std::move(handleFunction)](const QtOhos::CallbackInfo &cbInfo) {
                auto changedDisplayIdValue = cbInfo.getFirstArg<QNapi::Number>(Q_FUNC_INFO);
                auto changedDisplayId = JsDisplayId{changedDisplayIdValue};
                handleFunction(cbInfo.jsState(), changedDisplayId);
            }));
}

bool QOhosDisplayManager::tryRegisterDisplay(
    QtOhos::JsState &jsState, JsDisplayId displayId)
{
    auto optDisplay = QOhosDisplayInfo::tryGetDisplayById(jsState, displayId);
    if (!optDisplay.hasValue()) {
        qOhosPrintfError(
            "%s: Display with id: %f went missing during its registration.",
            Q_FUNC_INFO, displayId.value());
        return false;
    }

    auto displayInfo = QOhosDisplayInfo::makeFromOhosDisplayObject(jsState, optDisplay.value());
    if (displayInfo.shouldIgnoreDisplay())
        return false;

    auto availableAreaChangeHandle = QtOhos::registerOnOffMethodsBasedEventHandler(
        optDisplay.value(),
        "availableAreaChange",
        [this, displayId](const QtOhos::CallbackInfo &cbInfo) {
            auto availableArea = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
            auto availableAreaQRectF = QRectF(
                availableArea.get<QNapi::Number>("left"),
                availableArea.get<QNapi::Number>("top"),
                availableArea.get<QNapi::Number>("width"),
                availableArea.get<QNapi::Number>("height"));
            m_availableAreaChangedCb(cbInfo.jsState(), displayId, availableAreaQRectF);
        });

    bool added = false;
    std::tie(std::ignore, added) = m_perDisplayDestroyNotifiers.insert(
        std::make_pair(displayId, std::move(availableAreaChangeHandle)));
    if (!added)
        qOhosPrintfError("Duplicate display added event for display id: %f", displayId.value());

    return added;
}

void QOhosDisplayManager::unregisterDisplay(JsDisplayId displayId)
{
    if (m_perDisplayDestroyNotifiers.erase(displayId) == 0)
        qOhosPrintfError("Attempted to erase unknown display with id: %f", displayId.value());
}

std::shared_ptr<QOhosDisplayManager> QOhosDisplayManager::create(
    QtOhos::JsState &jsState, CreateInfo createInfo)
{
    return std::shared_ptr<QOhosDisplayManager>(new QOhosDisplayManager(jsState, std::move(createInfo)));
}

std::vector<QOhosDisplayInfo> QOhosDisplayManager::getRegisteredDisplayInfos()
{
    return m_registeredDisplayInfos;
}

QOhosDisplayManager::QOhosDisplayManager(QtOhos::JsState &jsState, CreateInfo createInfo)
{
    for (const auto &displayInfo : createInfo.displayInfos) {
        if (tryRegisterDisplay(jsState, displayInfo.id)) {
            m_registeredDisplayInfos.push_back(displayInfo);
        } else {
            qOhosPrintfError(
                "%s: Failed to register display (%f) during display initialization.",
                Q_FUNC_INFO,
                displayInfo.id.value());
        }
    }

    auto displayModule = jsState.eval<QNapi::Object>("@ohos.display");
    m_availableAreaChangedCb = std::move(createInfo.displayAvailableAreaChangedCb);
    registerDisplayCallbackListener(
        displayModule,
        displayCallbackNameChangeEvent,
        std::move(createInfo.displayChangedCb));
    registerDisplayCallbackListener(
        displayModule,
        displayCallbackNameAddEvent,
        [this, displayAddedCb = std::move(createInfo.displayAddedCb)](QtOhos::JsState &jsState, JsDisplayId displayId) {
            if (tryRegisterDisplay(jsState, displayId)) {
                displayAddedCb(jsState, displayId);
            } else {
                qOhosPrintfError(
                    "%s: Failed to register display (%f) during display added callback.",
                    Q_FUNC_INFO,
                    displayId.value());
            }
        });
    registerDisplayCallbackListener(
        displayModule,
        displayCallbackNameRemoveEvent,
        [this, displayRemovedCb = std::move(createInfo.displayRemovedCb)](QtOhos::JsState &jsState, JsDisplayId displayId) {
            unregisterDisplay(displayId);
            displayRemovedCb(jsState, displayId);
        });
}

void QOhosDisplayManager::getAllDisplaysAsync(
    QtOhos::JsState &jsState, QOhosConsumer<std::vector<QOhosDisplayInfo>> resultConsumer)
{
    jsState.eval<QNapi::Promise>("@ohos.display.getAllDisplay()")
        .withContext(std::move(resultConsumer))
        .onThenWithContext(
            [](const QtOhos::CallbackInfo &cbInfo, auto &resultConsumer) {
                auto displayObjectsArray = cbInfo.getFirstArg<QNapi::Array>(Q_FUNC_INFO);
                resultConsumer(
                    QNapi::getArrayElements<std::vector<QOhosDisplayInfo>, QNapi::Object>(
                        displayObjectsArray,
                        [&](QNapi::Object jsDisplay) {
                            return QOhosDisplayInfo::makeFromOhosDisplayObject(cbInfo.jsState(), jsDisplay);
                        }));
            })
        .onCatchWithContext(
            [](auto &resultConsumer) {
                qOhosPrintfError("%s: Failed to enumerate displays", Q_FUNC_INFO);
                resultConsumer({});
            });

}

}

QT_END_NAMESPACE
