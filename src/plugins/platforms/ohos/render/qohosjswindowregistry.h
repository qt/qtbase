// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSJSWINDOWREGISTRY_H
#define QOHOSJSWINDOWREGISTRY_H

#include <QtCore/qglobal.h>
#include <functional>
#include <map>
#include <memory>
#include <qarkui/window.h>
#include <qohosplugincore.h>
#include <vector>

QT_BEGIN_NAMESPACE

class QOhosJsWindowRegistry
{
public:
    QOhosJsWindowRegistry();

    QOhosJsWindowRegistry(const QOhosJsWindowRegistry &) = delete;
    QOhosJsWindowRegistry &operator=(const QOhosJsWindowRegistry &) = delete;
    QOhosJsWindowRegistry(QOhosJsWindowRegistry &&) = delete;
    QOhosJsWindowRegistry &operator=(QOhosJsWindowRegistry &&) = delete;

    std::shared_ptr<void> registerJsWindow(std::shared_ptr<QArkUi::JsWindowRef> jsWindowRef);
    std::vector<QArkUi::JsWindowId> queryByPredicate(
        QtOhos::JsState &jsState,
        const std::function<bool(QtOhos::JsState &, const QArkUi::JsWindowRef &)> &predicate);

    std::shared_ptr<QArkUi::JsWindowRef> tryFindJsWindowById(QArkUi::JsWindowId jsWinId) const;
    std::shared_ptr<QArkUi::JsWindowRef> tryFindJsWindowByQWindowRef(QtOhos::QObjectThreadSafeRef qwindow) const;

private:
    std::map<QArkUi::JsWindowId, std::shared_ptr<QArkUi::JsWindowRef>> m_windowRefs;
};

QT_END_NAMESPACE

#endif
