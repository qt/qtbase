// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qarkui/displaymanager.h>

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/qspan.h>
#include <memory>
#include <qarkui/qarkuiutils.h>
#include <qohosdisplayinfo.h>
#include <qohosjsutils.h>
#include <window_manager/oh_display_info.h>
#include <window_manager/oh_display_manager.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {

namespace {

const std::string displayCallbackNameChangeEvent = "change";
const std::string displayCallbackNameAddEvent = "add";
const std::string displayCallbackNameRemoveEvent = "remove";

std::shared_ptr<::NativeDisplayManager_DisplaysInfo> enumerateAllDisplaysOrFail()
{
    ::NativeDisplayManager_DisplaysInfo *displayListPtr = nullptr;
    callArkUiOrFailOnErrorResult(
        Q_OHOS_NAMED_FUNC(::OH_NativeDisplayManager_CreateAllDisplays),
        &displayListPtr);
    if (displayListPtr == nullptr) {
        qOhosReportFatalErrorAndAbort(
            "%s: OH_NativeDisplayManager_CreateAllDisplays returned empty displayListPtr",
            Q_FUNC_INFO);
    }

    return std::shared_ptr<::NativeDisplayManager_DisplaysInfo>(
        displayListPtr,
        [](::NativeDisplayManager_DisplaysInfo *displayListPtr) {
            callArkUi(
                Q_OHOS_NAMED_FUNC(::OH_NativeDisplayManager_DestroyAllDisplays),
                displayListPtr);
        });
}

QPoint getGlobalDisplayOffsetOfDisplay(QOhosDisplayInfo::JsDisplayId displayId)
{
    std::int32_t x = 0;
    std::int32_t y = 0;
    auto getDisplayPositionResult = callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_NativeDisplayManager_GetDisplayPosition),
        static_cast<std::uint64_t>(displayId.value()),
        &x, &y);

    QPoint result;
    if (getDisplayPositionResult == ::DISPLAY_MANAGER_OK) {
        result = QPoint(x, y);
    } else if (getDisplayPositionResult == ::DISPLAY_MANAGER_ERROR_ILLEGAL_PARAM) {
        // NOTE: According to the documentation Queries for other screens return DISPLAY_MANAGER_ERROR_ILLEGAL_PARAM.
        // 0, 0 offset is a default for those cases as they are not part of the virtual desktop
        result = QPoint();
    } else {
        qOhosReportFatalErrorAndAbort(
            "OH_NativeDisplayManager_GetDisplayPosition returned unexpected error: %d", getDisplayPositionResult);
    }

    return result;
}

}

void QOhosDisplayManager::registerDisplayCallbackListener(
    QNapi::Object displayModule, const std::string &eventName,
    QOhosConsumer<QtOhos::JsState &, JsDisplayId> handleFunction)
{
    m_destroyNotifiers.push_back(
        registerQOhosOnOffMethodsBasedEventHandler(
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
    if (!optDisplay.has_value()) {
        qOhosPrintfError(
            "%s: Display with id: %f went missing during its registration.",
            Q_FUNC_INFO, displayId.value());
        return false;
    }

    auto displayInfo = QOhosDisplayInfo::makeFromOhosDisplayObject(jsState, optDisplay.value());
    if (displayInfo.shouldIgnoreDisplay())
        return false;

    auto availableAreaChangeHandle = registerQOhosOnOffMethodsBasedEventHandler(
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
    if (added)
        m_registeredDisplayInfos.push_back(displayInfo);
    else
        qOhosPrintfError("Duplicate display added event for display id: %f", displayId.value());

    return added;
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
    rebuildRegisteredDisplayList(jsState);

    m_availableAreaChangedCb = std::move(createInfo.displayAvailableAreaChangedCb);

    const std::string displayEventNames[] = {
        displayCallbackNameChangeEvent,
        displayCallbackNameAddEvent,
        displayCallbackNameRemoveEvent,
    };

    auto displaysUpdatedCb = QtOhos::moveToSharedPtr(std::move(createInfo.displaysUpdatedCb));

    auto displayModule = jsState.eval<QNapi::Object>("@ohos.display");
    for (const auto &eventName: displayEventNames) {
        registerDisplayCallbackListener(
            displayModule,
            eventName,
            [this, displaysUpdatedCb](QtOhos::JsState &jsState, JsDisplayId) {
                rebuildRegisteredDisplayList(jsState);
                (*displaysUpdatedCb)(jsState, m_registeredDisplayInfos);
            });
    }
}

void QOhosDisplayManager::rebuildRegisteredDisplayList(QtOhos::JsState &jsState)
{
    m_perDisplayDestroyNotifiers = {};
    m_registeredDisplayInfos = {};

    auto displayListPtr = enumerateAllDisplaysOrFail();
    for (const auto &nativeDisplayInfo : QSpan(displayListPtr->displaysInfo, displayListPtr->displaysLength)) {
        if (!tryRegisterDisplay(jsState, JsDisplayId(nativeDisplayInfo.id))) {
            qOhosPrintfError(
                "%s: Failed to register display (%d) during display initialization.",
                Q_FUNC_INFO,
                nativeDisplayInfo.id);
        }
    }
}

QPoint mapFromDisplayToGlobal(const QPoint &displayOffset, QOhosDisplayInfo::JsDisplayId jsDisplayId)
{
    return getGlobalDisplayOffsetOfDisplay(jsDisplayId) + displayOffset;
}

}

QT_END_NAMESPACE
