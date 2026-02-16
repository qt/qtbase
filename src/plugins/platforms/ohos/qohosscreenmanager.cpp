// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosscreenmanager.h>
#include <cstdint>
#include <memory>
#include <utility>
#include <qarkui/displaymanager.h>
#include <qohosdisplayinfo.h>
#include <qohosjsutils.h>
#include <qohosplatformscreen.h>
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

QOhosScreenManager::QOhosScreenManager()
{
    auto selfRef = QtOhos::QThreadSafeRef<QOhosScreenManager>(this);

    std::vector<QOhosDisplayInfo> registeredDisplays;
    QtOhos::runInJsThreadAndWait([&](QtOhos::JsState &jsState) {
        auto rebuildScreenListFunc = QtOhos::moveToSharedPtr(
            [selfRef](QtOhos::JsState &jsState) {
                QArkUi::QOhosDisplayManager::getAllDisplaysAsync(
                    jsState,
                    [selfRef](auto displayInfos) {
                        selfRef.visitInQtThreadIfAlive(
                            [displayInfos](QOhosScreenManager &self) {
                                self.rebuildScreenList(displayInfos);
                            });
                    });
            });

        m_jsScopeData = QtOhos::makeProxyWithJsThreadDeleter(
            QArkUi::QOhosDisplayManager::create(
                jsState, QArkUi::QOhosDisplayManager::CreateInfo{
                    .displayChangedCb = [rebuildScreenListFunc](QtOhos::JsState &jsState, JsDisplayId) {
                        (*rebuildScreenListFunc)(jsState);
                    },
                    .displayAddedCb = [rebuildScreenListFunc](QtOhos::JsState &jsState, JsDisplayId) {
                        (*rebuildScreenListFunc)(jsState);
                    },
                    .displayRemovedCb = [rebuildScreenListFunc](QtOhos::JsState &jsState, JsDisplayId) {
                        (*rebuildScreenListFunc)(jsState);
                    },
                    .displayAvailableAreaChangedCb = [selfRef](QtOhos::JsState &, JsDisplayId displayId, QRectF availableArea) {
                        selfRef.visitInQtThreadIfAlive([displayId, availableArea](QOhosScreenManager &self) {
                            self.handleDisplayAvailableAreaChanged(displayId, availableArea);
                        });
                    },
                }));
        registeredDisplays = m_jsScopeData->getRegisteredDisplayInfos();
    });

    rebuildScreenList(registeredDisplays);
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

QOhosOptional<QOhosDisplayInfo::JsDisplayId> QOhosScreenManager::QOhosPlatformScreenHolder::displayIdOrEmpty() const
{
    return displayInfoOrEmpty().transform(
        [](const QOhosDisplayInfo &displayInfo) {
            return displayInfo.id;
        });
}

QOhosScreenManager::QOhosPlatformScreenHolder::~QOhosPlatformScreenHolder()
{
    if (m_platformScreen != nullptr)
        QWindowSystemInterface::handleScreenRemoved(m_platformScreen);
}

QOhosScreenManager::QOhosPlatformScreenHolder *
QOhosScreenManager::platformScreenHolderForDisplayIdOrNull(JsDisplayId displayId) const
{
    auto it = std::find_if(
        m_displays.begin(), m_displays.end(),
        [&](const std::unique_ptr<QOhosPlatformScreenHolder> &platformScreenHolder) {
            return platformScreenHolder->displayIdOrEmpty() == displayId;
        });

    return it != m_displays.end()
        ? it->get()
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

void QOhosScreenManager::handleDisplayAvailableAreaChanged(
    JsDisplayId jsDisplayId, QRectF availableArea)
{
    auto *platformScreen = platformScreenForDisplayIdOrNull(jsDisplayId);
    if (platformScreen != nullptr)
        platformScreen->setAvailableGeometry(availableArea.toRect());
}

void QOhosScreenManager::rebuildScreenList(std::vector<QOhosDisplayInfo> updatedDisplayInfos)
{
    const auto primaryDisplayId = JsDisplayId(0);

    if (updatedDisplayInfos.empty())
        qOhosReportFatalErrorAndAbort("Empty display list. This should not happen");

    auto currentDisplays = std::exchange(m_displays, {});

    QOhosPlatformScreen *primaryScreen = nullptr;
    for (const auto &displayInfo: updatedDisplayInfos) {
        auto availablePlatformScreenHolderIt = std::find_if(
            currentDisplays.begin(), currentDisplays.end(),
            [](const std::unique_ptr<QOhosPlatformScreenHolder> &platformScreenHolder) {
                return platformScreenHolder && platformScreenHolder->platformScreenOrNull() != nullptr;
            });

        auto platformScreenHolder = availablePlatformScreenHolderIt != currentDisplays.end()
            ? std::move(*availablePlatformScreenHolderIt)
            : std::make_unique<QOhosPlatformScreenHolder>(displayInfo);

        auto *platformScreen = platformScreenHolder->platformScreenOrNull();
        if (Q_UNLIKELY(platformScreen == nullptr))
            qOhosReportFatalErrorAndAbort("platformScreenHolder->platformScreenOrNull() == nullptr. This should not happen.");

        platformScreen->setDisplayInfo(displayInfo);
        if (displayInfo.id == primaryDisplayId)
            primaryScreen = platformScreen;

        m_displays.push_back(std::move(platformScreenHolder));
    }

    if (primaryScreen != nullptr) {
        QWindowSystemInterface::handlePrimaryScreenChanged(primaryScreen);
    } else {
        qCCritical(QtForOhos) << Q_FUNC_INFO << "no primary screen was found in the updated list";
    }

}

QT_END_NAMESPACE
