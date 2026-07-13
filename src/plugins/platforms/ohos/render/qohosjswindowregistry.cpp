// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosutils.h>
#include <render/qohosjswindowregistry.h>
#include <tuple>

QT_BEGIN_NAMESPACE

QOhosJsWindowRegistry::QOhosJsWindowRegistry() = default;

std::shared_ptr<void> QOhosJsWindowRegistry::registerJsWindow(std::shared_ptr<QArkUi::JsWindowRef> jsWindowRef)
{
    auto jsWindowId = jsWindowRef->id();

    bool added = false;
    std::tie(std::ignore, added) = m_windowRefs.insert({jsWindowRef->id(), jsWindowRef});
    if (!added) {
        qOhosReportFatalErrorAndAbort(
            "%s: Duplicate jsWindow with id: %f", Q_FUNC_INFO, jsWindowId.value());
    }

    return QtOhos::makeProxyWithJsThreadDeleter(
        QtOhos::makeDestroyNotifier([this, jsWindowId]() {
            std::ignore = m_windowRefs.erase(jsWindowId);
        }));
}

std::vector<QArkUi::JsWindowId> QOhosJsWindowRegistry::queryByPredicate(
    QtOhos::JsState &jsState,
    const std::function<bool(QtOhos::JsState &, const QArkUi::JsWindowRef &)> &predicate)
{
    std::vector<QArkUi::JsWindowId> result;
    for (const auto &windowIdRefPair : m_windowRefs) {
        if (predicate(jsState, *(windowIdRefPair.second)))
            result.push_back(windowIdRefPair.first);
    }
    return result;
}

std::shared_ptr<QArkUi::JsWindowRef> QOhosJsWindowRegistry::tryFindJsWindowById(QArkUi::JsWindowId jsWinId) const
{
    auto jsWinIter = m_windowRefs.find(jsWinId);
    return jsWinIter != m_windowRefs.end()
        ? jsWinIter->second
        : nullptr;
}

std::shared_ptr<QArkUi::JsWindowRef> QOhosJsWindowRegistry::tryFindJsWindowByQWindowRef(
    QtOhos::QObjectThreadSafeRef qwindow) const
{
    for (const auto &windowIdRefPair : m_windowRefs) {
        if (windowIdRefPair.second->owningQWindowRef() == qwindow)
            return windowIdRefPair.second;
    }
    return nullptr;
}

QT_END_NAMESPACE
