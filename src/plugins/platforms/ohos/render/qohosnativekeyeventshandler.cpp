// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qohosnativekeyeventshandler.h>

#include <arkui/native_key_event.h>
#include <qarkui/qarkuiutils.h>
#include <qohoskeymodifiers.h>
#include <qohosnativenodekeyevent.h>

QT_BEGIN_NAMESPACE

namespace {

bool isModifierKey(::ArkUI_KeyCode keyCode)
{
    switch (keyCode) {
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_SHIFT_LEFT:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_SHIFT_RIGHT:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_ALT_LEFT:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_ALT_RIGHT:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_CTRL_LEFT:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_CTRL_RIGHT:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_META_LEFT:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_META_RIGHT:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_CAPS_LOCK:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUM_LOCK:
        return true;
    default:
        return false;
    }
}

}

QOhosConsumer<::ArkUI_UIInputEvent *> makeQOhosNativeKeyEventsHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef)
{
    auto lastNativeKeyEvent = makeEmptyQOhosNativeNodeKeyEvent();

    return [qWindowRef, imEventHandlerRef, lastNativeKeyEvent](::ArkUI_UIInputEvent *uiInputEvent) mutable {
        const auto keyType = QArkUi::callArkUi(
            Q_OHOS_NAMED_FUNC(::OH_ArkUI_KeyEvent_GetType), uiInputEvent);
        const auto keyCode = QArkUi::callArkUi(
            Q_OHOS_NAMED_FUNC(::OH_ArkUI_KeyEvent_GetKeyCode), uiInputEvent);

        const auto arkuiKeyCode = static_cast<::ArkUI_KeyCode>(keyCode);
        auto ohosKeyEvent = makeQOhosNativeNodeKeyEvent(keyType, arkuiKeyCode, readKeyModifiersFromOhosUiInputEvent(uiInputEvent));
        if (isModifierKey(arkuiKeyCode) && lastNativeKeyEvent->equals(*ohosKeyEvent))
            return;
        lastNativeKeyEvent = ohosKeyEvent;

        imEventHandlerRef.visitInQtThreadIfAlive(
            [ohosKeyEvent, qWindowRef](QOhosInputMethodEventHandler &imEventHandler) {
                imEventHandler.onKeyEvent(*ohosKeyEvent, qWindowRef.data());
            });
    };
}

QT_END_NAMESPACE
