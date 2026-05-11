// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWPROXYREGISTRY_H
#define QWINDOWPROXYREGISTRY_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <map>
#include <qarkui/window.h>
#include <qohosplugincore.h>
#include <render/qohoswindowproxy.h>
#include <string>

QT_BEGIN_NAMESPACE

class QWindowProxyRegistry
{
public:
    static QWindowProxyRegistry &instance();

    QWindowProxyRegistry(const QWindowProxyRegistry &) = delete;
    QWindowProxyRegistry &operator=(const QWindowProxyRegistry &) = delete;

    QWindowProxyRegistry(QWindowProxyRegistry &&) = delete;
    QWindowProxyRegistry &operator=(QWindowProxyRegistry &&) = delete;

    std::shared_ptr<void> registerQWindowWithWindowProxy(
        QWindow *window, const QOhosWindowProxy &windowProxy);

    QOhosOptional<QArkUi::JsWindowId> tryMapInternalWindowIdToJsWindowId(
        QtOhos::InternalWindowId internalWindowId) const;

    QWindow *findQWindowByJsWindowIdOrNull(QArkUi::JsWindowId jsWindowId);
    QOhosOptional<std::string> tryFindQAbilityInstanceIdByInternalWindowId(
        QtOhos::InternalWindowId internalWindowId);

    std::vector<QWindow *> queryWindowsWithSystemWindowAndFocus();
    std::vector<QWindow *> queryWindowsWithVisibleSystemWindow();

    ~QWindowProxyRegistry();

private:
    QWindowProxyRegistry();

    std::vector<QWindow *> querySystemWindows(
        const std::function<bool(QtOhos::JsState &, const QArkUi::JsWindowRef &)> &predicate);

    std::map<QArkUi::JsWindowId, QtOhos::InternalWindowId> m_jsWindowIdMap;
    std::map<QtOhos::InternalWindowId, std::string> m_qAbilityInstanceIdMap;
};

QT_END_NAMESPACE

#endif
