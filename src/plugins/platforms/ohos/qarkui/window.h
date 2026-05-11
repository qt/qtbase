// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QARKUI__WINDOW_H
#define QARKUI__WINDOW_H

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qrect.h>
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
    QOhosOptional<QOhosDisplayInfo::JsDisplayId> displayId;
};

QOhosOptional<WindowProperties> tryGetWindowProperties(JsWindowId jsWindowId);

class JsWindowRef
{
public:
    explicit JsWindowRef(JsWindowId windowId, QNapi::Object jsWindow);

    JsWindowRef(const JsWindowRef &) = delete;
    JsWindowRef &operator=(const JsWindowRef &) = delete;

    JsWindowRef(JsWindowRef &&) = delete;
    JsWindowRef &operator=(JsWindowRef &&) = delete;

    bool isFocused() const;
    bool isWindowShown() const;
    JsWindowId id() const;

    template<typename Result = QNapi::Value>
    Result call(const std::string &methodName, const std::vector<QNapi::ValueWrapper> &args = {}) const;

    QNapi::Object jsObject();

private:
    JsWindowId m_jsWindowId;
    QNapi::Reference<QNapi::Object> m_jsWindow;
};

template<typename Result>
Result JsWindowRef::call(const std::string &methodName, const std::vector<QNapi::ValueWrapper> &args) const
{
    return m_jsWindow.call<Result>(methodName, args);
}

}

QT_END_NAMESPACE

#endif
