// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosscreenmanager.h>
#include <cstdint>
#include <memory>
#include <utility>
#include <qohosdisplayinfo.h>
#include <qohosjsutils.h>
#include <qohosplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

namespace
{

QOhosDisplayInfo getDisplayInfoForPrimaryDisplay(QtOhos::JsState &jsState)
{
    auto primaryDisplay = jsState.eval<QNapi::Object>("@ohos.display.getPrimaryDisplaySync()");
    return QOhosDisplayInfo::makeFromOhosDisplayObject(jsState, primaryDisplay);
}

QOhosOptional<QNapi::Object> tryGetDisplayById(QtOhos::JsState &jsState, QOhosDisplayInfo::JsDisplayId displayId)
{
    QOhosOptional<QNapi::Object> result;
    try {
        result = jsState.eval<QNapi::Object>(
            "@ohos.display.getDisplayByIdSync(*)", {displayId.value()});
    } catch (const Napi::Error &error) {
        qOhosPrintfError("%s: Failed to retrieve display with id: %f", Q_FUNC_INFO, displayId.value());
    }
    return result;
}

const std::string displayCallbackNameChangeEvent = "change";
const std::string displayCallbackNameAddEvent = "add";
const std::string displayCallbackNameRemoveEvent = "remove";

QDebug &operator<<(QDebug &outputStream, const QOhosDisplayInfo &displayInfo)
{
    outputStream << "id:" << displayInfo.id.value()
        << "name:" << displayInfo.name
        << "sizePixels:" << displayInfo.sizePixels
        << "densityDPI:" << displayInfo.densityDPI
        << "densityPixels:" << displayInfo.densityPixels
        << "densityScaled:" << displayInfo.densityScaled
        << "dpi:" << displayInfo.dpi;
    return outputStream;
}

bool shouldIgnoreDisplay(const QOhosDisplayInfo &displayInfo)
{
    constexpr int virtualDisplayBaseId = 1000;
    static const QOhosDisplayInfo::DisplaySourceMode sourceModesToIgnore[] = {
       QOhosDisplayInfo::DisplaySourceMode::NONE,
       QOhosDisplayInfo::DisplaySourceMode::MIRROR,
       QOhosDisplayInfo::DisplaySourceMode::ALONE,
    };

    const auto *sourceModeToIgnoreIter = std::find(
        std::begin(sourceModesToIgnore), std::end(sourceModesToIgnore),
        displayInfo.sourceMode);
    bool ignoreBySoureMode = sourceModeToIgnoreIter != std::end(sourceModesToIgnore);

    return displayInfo.sourceMode.hasValue()
        ? ignoreBySoureMode
        : displayInfo.id.value() >= virtualDisplayBaseId;
}

}

QOhosScreenManager::QOhosScreenManager()
{
    QOhosDisplayInfo primaryDisplayInfo;
    auto selfRef = QtOhos::QThreadSafeRef<QOhosScreenManager>(this);

    auto displayInfos = QtOhos::evalInJsThreadWithConsumer<std::vector<QOhosDisplayInfo>>(
        [](QtOhos::JsState &jsState, auto resultConsumer) {
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
        });

    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &jsState) {
        primaryDisplayInfo = getDisplayInfoForPrimaryDisplay(jsState);

        m_jsScopeData = QtOhos::makeProxyWithJsThreadDeleter(
            QOhosDisplayManager::create(
                jsState, QOhosDisplayManager::CreateInfo{
                    .displayInfos = displayInfos,
                    .displayChangedCb = [selfRef](QtOhos::JsState &jsState, JsDisplayId changedDisplayId) {
                        auto displayObject = tryGetDisplayById(jsState, changedDisplayId);
                        if (!displayObject.hasValue()) {
                            qOhosPrintfError(
                                "%s: Failed to retrieve display with id: %f during display changed callback. Ignoring the event...",
                                Q_FUNC_INFO, changedDisplayId.value());
                            return;
                        }

                        auto displayInfo = QOhosDisplayInfo::makeFromOhosDisplayObject(jsState, displayObject.value());
                        selfRef.visitInQtThreadIfAlive([displayInfo](QOhosScreenManager &self) {
                            self.handleDisplayChangedCallbackInQtThread(displayInfo);
                        });
                    },
                    .displayAddedCb = [selfRef](QtOhos::JsState &jsState, JsDisplayId displayId) {
                        auto displayObject = tryGetDisplayById(jsState, displayId);
                        if (!displayObject.hasValue()) {
                            qOhosPrintfError(
                                "%s: Failed to retrieve display with id: %f during display added callback. Ignoring the event ...",
                                Q_FUNC_INFO, displayId.value());
                            return;
                        }

                        auto displayInfo = QOhosDisplayInfo::makeFromOhosDisplayObject(jsState, displayObject.value());
                        selfRef.visitInQtThreadIfAlive([displayInfo](QOhosScreenManager &self) {
                            self.handleDisplayAdded(displayInfo);
                        });
                    },
                    .displayRemovedCb = [selfRef](QtOhos::JsState &, JsDisplayId displayId) {
                        selfRef.visitInQtThreadIfAlive([displayId](QOhosScreenManager &self) {
                            self.handleDisplayRemoved(displayId);
                        });
                    },
                    .displayAvailableAreaChangedCb = [selfRef](QtOhos::JsState &, JsDisplayId displayId, QRectF availableArea) {
                        selfRef.visitInQtThreadIfAlive([displayId, availableArea](QOhosScreenManager &self) {
                            self.handleDisplayAvailableAreaChanged(displayId, availableArea);
                        });
                    },
                }));
    });

    m_primaryDisplayId = primaryDisplayInfo.id;
    for (const auto &displayInfo : m_jsScopeData->getRegisteredDisplayInfos())
        addScreen(displayInfo);
}

void QOhosScreenManager::handleDisplayChangedCallbackInQtThread(const QOhosDisplayInfo &displayInfo)
{
    if (shouldIgnoreDisplay(displayInfo))
        return;
    auto *platformScreen = platformScreenForDisplayIdOrNull(displayInfo.id);
    if (platformScreen == nullptr) {
        qCWarning(QtForOhos) << "Received display 'change' event for unknown display with id:"
            << displayInfo.id.value();
        return;
    }

    qCDebug(QtForOhos) << "DisplayChanged:" << displayInfo;

    platformScreen->setDisplayInfo(displayInfo);
    updatePrimaryPlatformScreenIfNeeded();
}

void QOhosScreenManager::QOhosDisplayManager::registerDisplayCallbackListener(
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

bool QOhosScreenManager::QOhosDisplayManager::tryRegisterDisplay(
    QtOhos::JsState &jsState, JsDisplayId displayId)
{
    auto optDisplay = tryGetDisplayById(jsState, displayId);
    if (!optDisplay.hasValue()) {
        qOhosPrintfError(
            "%s: Display with id: %f went missing during its registration.",
            Q_FUNC_INFO, displayId.value());
        return false;
    }

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

void QOhosScreenManager::QOhosDisplayManager::unregisterDisplay(JsDisplayId displayId)
{
    if (m_perDisplayDestroyNotifiers.erase(displayId) == 0)
        qOhosPrintfError("Attempted to erase unknown display with id: %f", displayId.value());
}


QOhosScreenManager::QOhosPlatformScreenHolder::QOhosPlatformScreenHolder(
    const QOhosDisplayInfo &displayInfo)
{
    auto screen = std::make_unique<QOhosPlatformScreen>(displayInfo);
    m_platformScreen = screen.get();
    QWindowSystemInterface::handleScreenAdded(screen.release());
}

QOhosPlatformScreen *QOhosScreenManager::QOhosPlatformScreenHolder::platformScreenOrNull() const
{
    return m_platformScreen;
}

QOhosOptional<QOhosDisplayInfo> QOhosScreenManager::QOhosPlatformScreenHolder::displayInfoOrEmpty() const
{
    return m_platformScreen != nullptr
        ? makeQOhosOptional(m_platformScreen->displayInfo())
        : makeEmptyQOhosOptional();
}

QOhosScreenManager::QOhosPlatformScreenHolder::~QOhosPlatformScreenHolder()
{
    if (m_platformScreen != nullptr)
        QWindowSystemInterface::handleScreenRemoved(m_platformScreen);
}

QOhosScreenManager::QOhosPlatformScreenHolder *
QOhosScreenManager::platformScreenHolderForDisplayIdOrNull(JsDisplayId displayId) const
{
    auto it = m_displays.find(displayId);
    return it != m_displays.end()
        ? it->second.get()
        : nullptr;
}

QOhosPlatformScreen *QOhosScreenManager::platformScreenForDisplayIdOrNull(JsDisplayId displayId) const
{
    auto *platformScreenHolder = platformScreenHolderForDisplayIdOrNull(displayId);
    return platformScreenHolder != nullptr
        ? platformScreenHolder->platformScreenOrNull()
        : nullptr;
}

QOhosPlatformScreen *QOhosScreenManager::platformScreenForDisplayIdOrFail(JsDisplayId displayId) const
{
    auto *platformScreen = platformScreenForDisplayIdOrNull(displayId);
    if (platformScreen == nullptr)
        qOhosReportFatalErrorAndAbort("Failed to find platform screen for display id: %f", displayId.value());
    return platformScreen;
}

void QOhosScreenManager::addScreen(QOhosDisplayInfo displayInfo)
{
    auto it = m_displays.find(displayInfo.id);
    if (it != m_displays.end()) {
        qCWarning(QtForOhos) << "Attemting to add display with non unique id:" << displayInfo.id.value()
            << "previous display with that id will be removed";
    }
    m_displays[displayInfo.id] = std::make_unique<QOhosPlatformScreenHolder>(displayInfo);
}

void QOhosScreenManager::removeScreenIfExists(JsDisplayId displayId)
{
    std::ignore = m_displays.erase(displayId);
}

std::shared_ptr<QOhosScreenManager::QOhosDisplayManager> QOhosScreenManager::QOhosDisplayManager::create(
    QtOhos::JsState &jsState, CreateInfo createInfo)
{
   auto jsScopeData = std::shared_ptr<QOhosDisplayManager>(new QOhosDisplayManager(jsState));
   jsScopeData->initialize(jsState, std::move(createInfo));
   return jsScopeData;
}

std::vector<QOhosDisplayInfo> QOhosScreenManager::QOhosDisplayManager::getRegisteredDisplayInfos()
{
    return m_registeredDisplayInfos;
}

QOhosScreenManager::QOhosDisplayManager::QOhosDisplayManager(QtOhos::JsState &)
{
}

void QOhosScreenManager::QOhosDisplayManager::initialize(QtOhos::JsState &jsState, CreateInfo createInfo)
{
    for (const auto &displayInfo : createInfo.displayInfos) {
        if (!shouldIgnoreDisplay(displayInfo) && tryRegisterDisplay(jsState, displayInfo.id)) {
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

void QOhosScreenManager::handleDisplayAdded(const QOhosDisplayInfo &displayInfo)
{
    if (shouldIgnoreDisplay(displayInfo)) {
        qCWarning(QtForOhos) << "Display add ignored (based on display id):" << displayInfo.id.value();
        return;
    }
    qCWarning(QtForOhos) << "Display added:" << displayInfo;
    addScreen(displayInfo);
    updatePrimaryPlatformScreenIfNeeded();
}

void QOhosScreenManager::handleDisplayRemoved(JsDisplayId displayId)
{
    qCWarning(QtForOhos) << "Display removed: id:" << displayId.value();
    updatePrimaryPlatformScreenIfNeeded();

    if (m_primaryDisplayId == displayId)
        qOhosReportFatalErrorAndAbort("Primary display removed - this is not supported");

    removeScreenIfExists(displayId);
}

void QOhosScreenManager::handleDisplayAvailableAreaChanged(
    JsDisplayId jsDisplayId, QRectF availableArea)
{
    auto *platformScreen = platformScreenForDisplayIdOrNull(jsDisplayId);
    if (platformScreen != nullptr)
        platformScreen->setAvailableGeometry(availableArea.toRect());
}

void QOhosScreenManager::updatePrimaryPlatformScreenIfNeeded()
{
    auto primaryDisplayInfo = QtOhos::evalInJsThread(&getDisplayInfoForPrimaryDisplay);

    if (m_primaryDisplayId == primaryDisplayInfo.id)
        return;

    qCDebug(QtForOhos)
        << "Primary display changed from:" << m_primaryDisplayId.value()
        << "to:" << primaryDisplayInfo.id.value();

    auto *primaryPlatformScreen = platformScreenForDisplayIdOrNull(primaryDisplayInfo.id);
    if (primaryPlatformScreen == nullptr) {
        qCWarning(QtForOhos)
            << Q_FUNC_INFO
            << "Primary screen changed, but it was not registered previously"
            << "adding the screen with display id:" << primaryDisplayInfo.id.value();
        addScreen(primaryDisplayInfo);
        primaryPlatformScreen = platformScreenForDisplayIdOrFail(primaryDisplayInfo.id);
    }

    m_primaryDisplayId = primaryDisplayInfo.id;
    QWindowSystemInterface::handlePrimaryScreenChanged(primaryPlatformScreen);
}

QT_END_NAMESPACE
