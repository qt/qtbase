// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qwindowproxyregistry.h>

#include <QtGui/private/qguiapplication_p.h>
#include <algorithm>
#include <qarkui/window.h>
#include <qohosplatformwindow.h>
#include <qohosutils.h>
#include <render/qohosview.h>
#include <render/qohoswindowproxy.h>

QT_BEGIN_NAMESPACE

namespace
{

QWindow *findQWindowByInternalWindowIdOrNull(QtOhos::InternalWindowId internalWindowId)
{
    auto allWindows = qGuiApp->allWindows();
    auto qWindowIt = std::find_if(
        allWindows.begin(), allWindows.end(), [&](QWindow *qWindow) {
            auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(qWindow);
            return platformWindow != nullptr
                && platformWindow->internalWindowId() == internalWindowId;
        });

    return qWindowIt != allWindows.end()
        ? *qWindowIt
        : nullptr;
}

}

QWindowProxyRegistry::QWindowProxyRegistry() = default;
QWindowProxyRegistry::~QWindowProxyRegistry() = default;

std::shared_ptr<void> QWindowProxyRegistry::registerQWindowWithWindowProxy(
    QWindow *qWindow, const QOhosWindowProxy &windowProxy)
{
    auto jsWindowId = windowProxy.getWindowProperties().id;
    auto *platformWindow = QOhosPlatformWindow::fromQWindow(qWindow);
    auto internalWindowId = platformWindow->internalWindowId();

    bool internalWindowIdAdded = false;
    std::tie(std::ignore, internalWindowIdAdded) = m_jsWindowIdMap.insert({jsWindowId, internalWindowId});

    bool qAbilityInstanceIdAdded = false;
    std::tie(std::ignore, qAbilityInstanceIdAdded) = m_qAbilityInstanceIdMap.emplace(
        internalWindowId, windowProxy.qAbilityInstanceId());

    if (internalWindowIdAdded != qAbilityInstanceIdAdded) {
        qOhosReportFatalErrorAndAbort(
            "%s: state inconsistency: internalWindowIdAdded: %d, qAbilityInstanceIdAdded: %d, InternalWindowId='%s', JsWindowId=%f",
            Q_FUNC_INFO, internalWindowIdAdded, qAbilityInstanceIdAdded,
            internalWindowId.toStdString().c_str(), jsWindowId.value());
    }

    return internalWindowIdAdded
        ? QtOhos::makeDestroyNotifier([this, jsWindowId, internalWindowId]() {
            m_jsWindowIdMap.erase(jsWindowId);
            m_qAbilityInstanceIdMap.erase(internalWindowId);
        })
        : nullptr;
}

std::optional<QArkUi::JsWindowId> QWindowProxyRegistry::tryMapInternalWindowIdToJsWindowId(
    QtOhos::InternalWindowId internalWindowId) const
{
    auto foundEntryIter = std::find_if(
        m_jsWindowIdMap.begin(), m_jsWindowIdMap.end(),
        [&](const auto &entry) {
            return entry.second == internalWindowId;
        });

    return foundEntryIter != m_jsWindowIdMap.end()
        ? std::optional(foundEntryIter->first)
        : std::nullopt;
}

QWindow *QWindowProxyRegistry::findQWindowByJsWindowIdOrNull(
    QArkUi::JsWindowId jsWindowId)
{
    auto windowIdMapIt = m_jsWindowIdMap.find(jsWindowId);
    if (windowIdMapIt == m_jsWindowIdMap.end())
        return nullptr;

    auto internalWindowId = windowIdMapIt->second;
    return findQWindowByInternalWindowIdOrNull(internalWindowId);
}

std::optional<std::string> QWindowProxyRegistry::tryFindQAbilityInstanceIdByInternalWindowId(
    QtOhos::InternalWindowId internalWindowId)
{
    auto qAbilityInstanceIdIt = m_qAbilityInstanceIdMap.find(internalWindowId);
    return qAbilityInstanceIdIt != m_qAbilityInstanceIdMap.end()
        ? std::optional(qAbilityInstanceIdIt->second)
        : std::nullopt;
}

QWindowProxyRegistry &QWindowProxyRegistry::instance()
{
    static QWindowProxyRegistry instance;
    return instance;
}

std::vector<QWindow *> QWindowProxyRegistry::queryWindowsWithSystemWindowAndFocus()
{
    return querySystemWindows(
        [](QtOhos::JsState &, const QArkUi::JsWindowRef &jsWindow) {
            return jsWindow.isFocused();
        });
}

std::vector<QWindow *> QWindowProxyRegistry::queryWindowsWithVisibleSystemWindow()
{
    return querySystemWindows(
        [](QtOhos::JsState &, const QArkUi::JsWindowRef &jsWindow) {
            return jsWindow.isWindowShown();
        });
}

std::vector<QWindow *> QWindowProxyRegistry::querySystemWindows(
    const std::function<bool(QtOhos::JsState &, const QArkUi::JsWindowRef &)> &predicate)
{
    auto jsWindowIds = QOhosWindowProxy::queryQtManagedWindowIdsByPredicate(predicate);

    std::vector<QWindow *> qWindowsWithSystemWindow;
    for (const auto &jsWindowId : jsWindowIds) {
        auto *qWindow = findQWindowByJsWindowIdOrNull(jsWindowId);
        if (qWindow != nullptr)
            qWindowsWithSystemWindow.push_back(qWindow);
    }

    return qWindowsWithSystemWindow;
}

QT_END_NAMESPACE
