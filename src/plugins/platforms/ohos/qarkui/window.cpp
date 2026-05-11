// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qarkui/window.h>

#include <cstdint>
#include <qarkui/qarkuiutils.h>
#include <qohosutils.h>
#include <window_manager/oh_window.h>
#include <window_manager/oh_window_comm.h>

QT_BEGIN_NAMESPACE

namespace QArkUi {

namespace {

QRect makeQRectFromWindowManagerRect(const ::WindowManager_Rect &wmRect)
{
    return QRect(
        QPoint(wmRect.posX, wmRect.posY),
        QSize(wmRect.width, wmRect.height));
}

}

QOhosOptional<WindowProperties> tryGetWindowProperties(JsWindowId jsWindowId)
{
    ::WindowManager_WindowProperties windowProperties;
    auto errorCode = ::OH_WindowManager_GetWindowProperties(
        static_cast<std::int32_t>(jsWindowId.value()),
        &windowProperties);

    if (errorCode != ::OK)
        return makeEmptyQOhosOptional();

    return makeQOhosOptional(
        WindowProperties {
            .windowRect = makeQRectFromWindowManagerRect(windowProperties.windowRect),
            .drawableRect = makeQRectFromWindowManagerRect(windowProperties.drawableRect),
            .id = JsWindowId(windowProperties.id),
            .displayId = makeQOhosOptional(QOhosDisplayInfo::JsDisplayId(windowProperties.displayId)),
        });
}

JsWindowRef::JsWindowRef(JsWindowId windowId, QNapi::Object jsWindow)
    : m_jsWindowId(windowId)
    , m_jsWindow(Napi::Persistent(jsWindow))
{
}

bool JsWindowRef::isWindowShown() const
{
    bool result = false;
    const auto windowShownErrorCode = callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_WindowManager_IsWindowShown),
        static_cast<std::int32_t>(m_jsWindowId.value()), &result);
    if (windowShownErrorCode == ::WindowManager_ErrorCode::WINDOW_MANAGER_ERRORCODE_INVALID_PARAM)
        qOhosReportFatalErrorAndAbort("%s: error: %d", Q_FUNC_INFO, windowShownErrorCode);
    return windowShownErrorCode == ::WindowManager_ErrorCode::OK
        ? result
        : false;
}

bool JsWindowRef::isFocused() const
{
    if (QtOhos::JsWindowsTracker::isWindowClosing(m_jsWindow.Value()))
        return false;
    return m_jsWindow.call<QNapi::Boolean>("isFocused").Value();
}

JsWindowId JsWindowRef::id() const
{
    return m_jsWindowId;
}

QNapi::Object JsWindowRef::jsObject()
{
    return m_jsWindow.Value();
}

}

QT_END_NAMESPACE

