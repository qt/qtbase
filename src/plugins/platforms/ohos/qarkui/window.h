// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QARKUI__WINDOW_H
#define QARKUI__WINDOW_H

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qrect.h>
#include <optional>
#include <qohosdisplayinfo.h>
#include <qohosplugincore.h>
#include <qohosutils.h>
#include <string>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QArkUi {

using JsWindowId = QtOhos::TypedId<double, struct JsWindowIdTag>;

struct WindowProperties
{
    QRect windowRect;
    QRect drawableRect;
    JsWindowId id;
    std::optional<QOhosDisplayInfo::JsDisplayId> displayId;
};

std::optional<WindowProperties> tryGetWindowProperties(JsWindowId jsWindowId);

class JsWindowRef
{
public:
    explicit JsWindowRef(
        std::string owningQAbilityInstanceId, JsWindowId windowId,
        QNapi::Object jsWindow, QtOhos::QObjectThreadSafeRef owningQWindowRef);

    JsWindowRef(const JsWindowRef &) = delete;
    JsWindowRef &operator=(const JsWindowRef &) = delete;

    JsWindowRef(JsWindowRef &&) = delete;
    JsWindowRef &operator=(JsWindowRef &&) = delete;

    bool isFocused() const;
    bool isWindowShown() const;
    const std::string &owningQAbilityInstanceId() const;
    JsWindowId id() const;
    QtOhos::QObjectThreadSafeRef owningQWindowRef() const;

    template<typename Result = QNapi::Value>
    Result eval(const std::string &expr, const std::vector<QNapi::ValueWrapper> &exprArgs = {}) const;

    QNapi::Promise evalToPromiseOrRejectOnThrow(
        const std::string &expr, const std::vector<QNapi::ValueWrapper> &exprArgs = {}) const;

    QNapi::Object jsObject();

private:
    std::string m_owningQAbilityInstanceId;
    JsWindowId m_jsWindowId;
    QNapi::Reference<QNapi::Object> m_jsWindow;
    QtOhos::QObjectThreadSafeRef m_owningQWindowRef;
};

template<typename Result>
Result JsWindowRef::eval(const std::string &expr, const std::vector<QNapi::ValueWrapper> &exprArgs) const
{
    return m_jsWindow.eval<Result>(expr, exprArgs);
}

inline QNapi::Promise JsWindowRef::evalToPromiseOrRejectOnThrow(
    const std::string &expr, const std::vector<QNapi::ValueWrapper> &exprArgs) const
{
    return m_jsWindow.evalToPromiseOrRejectOnThrow(expr, exprArgs);
}

}

QT_END_NAMESPACE

#endif
