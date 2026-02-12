// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qarkui/input.h>

#include <qarkui/qarkuiutils.h>
#include <qohoskeymodifiers.h>

namespace ch = std::chrono;

QT_BEGIN_NAMESPACE

namespace QArkUi {

using JsDisplayId = QOhosDisplayInfo::JsDisplayId;

QOhosOptional<::Input_MouseEventAction> tryMapMouseEventAction(std::int32_t actionValue)
{
    switch (actionValue) {
    case ::MOUSE_ACTION_CANCEL:
    case ::MOUSE_ACTION_MOVE:
    case ::MOUSE_ACTION_BUTTON_DOWN:
    case ::MOUSE_ACTION_BUTTON_UP:
    case ::MOUSE_ACTION_AXIS_BEGIN:
    case ::MOUSE_ACTION_AXIS_UPDATE:
    case ::MOUSE_ACTION_AXIS_END:
        return makeQOhosOptional(static_cast<::Input_MouseEventAction>(actionValue));
    }
    return makeEmptyQOhosOptional();
}

QOhosOptional<::Input_MouseEventButton> tryMapMouseEventButton(std::int32_t buttonValue)
{
    switch (buttonValue) {
    case ::MOUSE_BUTTON_NONE:
    case ::MOUSE_BUTTON_LEFT:
    case ::MOUSE_BUTTON_MIDDLE:
    case ::MOUSE_BUTTON_RIGHT:
    case ::MOUSE_BUTTON_FORWARD:
    case ::MOUSE_BUTTON_BACK:
        return makeQOhosOptional(static_cast<::Input_MouseEventButton>(buttonValue));
    }
    return makeEmptyQOhosOptional();
}

QOhosOptional<::Input_TouchEventAction> tryMapTouchEventAction(std::int32_t actionValue)
{
    switch (actionValue) {
    case ::TOUCH_ACTION_CANCEL:
    case ::TOUCH_ACTION_DOWN:
    case ::TOUCH_ACTION_MOVE:
    case ::TOUCH_ACTION_UP:
        return makeQOhosOptional(static_cast<::Input_TouchEventAction>(actionValue));
    }
    return makeEmptyQOhosOptional();
}

QOhosOptional<::Input_KeyEventAction> tryMapKeyEventAction(std::int32_t actionValue)
{
    switch (actionValue) {
    case ::KEY_ACTION_CANCEL:
    case ::KEY_ACTION_DOWN:
    case ::KEY_ACTION_UP:
        return makeQOhosOptional(static_cast<::Input_KeyEventAction>(actionValue));
    }
    return makeEmptyQOhosOptional();
}

QOhosOptional<MouseEvent> MouseEvent::createFromNativeEvent(const ::Input_MouseEvent *event)
{
    auto jsWindowId = JsWindowId(::OH_Input_GetMouseEventWindowId(event));

    auto actionValue = ::OH_Input_GetMouseEventAction(event);
    auto optMappedAction = tryMapMouseEventAction(actionValue);
    if (!optMappedAction.hasValue()) {
        qOhosPrintfError(
            "%s: Filter for jsWindowId: %f, received unrecognized mouse event action: %d, event will be ignored",
            Q_FUNC_INFO, jsWindowId.value(), actionValue);
        return makeEmptyQOhosOptional();
    }

    auto buttonValue = ::OH_Input_GetMouseEventButton(event);
    auto optMappedButton = tryMapMouseEventButton(buttonValue);
    if (!optMappedButton.hasValue()) {
        qOhosPrintfError(
            "%s: Filter for jsWindowId: %f, received unrecognized mouse event action: %d, event will be ignored",
            Q_FUNC_INFO, jsWindowId.value(), actionValue);
        return makeEmptyQOhosOptional();
    }

    MouseEvent mouseEvent = {
        .jsWindowId = jsWindowId,
        .jsDisplayId = QOhosDisplayInfo::JsDisplayId(::OH_Input_GetMouseEventDisplayId(event)),
        .button = optMappedButton.value(),
        .action = optMappedAction.value(),
        .displayPosition = QPoint(
            ::OH_Input_GetMouseEventDisplayX(event),
            ::OH_Input_GetMouseEventDisplayY(event)),
        .actionTime = std::chrono::microseconds(::OH_Input_GetMouseEventActionTime(event)),
    };

    return makeQOhosOptional(mouseEvent);
}

QOhosOptional<TouchEvent> TouchEvent::createFromNativeEvent(const ::Input_TouchEvent *event)
{
    auto jsWindowId = JsWindowId(::OH_Input_GetTouchEventWindowId(event));

    auto actionValue = ::OH_Input_GetTouchEventAction(event);
    auto optMappedAction = tryMapTouchEventAction(actionValue);
    if (!optMappedAction.hasValue()) {
        qOhosPrintfError(
            "%s: Filter for jsWindowId: %f, received unrecognized touch event action: %d, event will be ignored",
            Q_FUNC_INFO, jsWindowId.value(), actionValue);
        return makeEmptyQOhosOptional();
    }

    TouchEvent keyEvent = {
        .jsWindowId = jsWindowId,
        .jsDisplayId = JsDisplayId(::OH_Input_GetTouchEventDisplayId(event)),
        .displayPosition = QPoint(
            ::OH_Input_GetTouchEventDisplayX(event),
            ::OH_Input_GetTouchEventDisplayY(event)),
        .action = optMappedAction.value(),
        .fingerId = ::OH_Input_GetTouchEventFingerId(event),
        .actionTime = std::chrono::microseconds(::OH_Input_GetTouchEventActionTime(event)),
    };

    return makeQOhosOptional(keyEvent);
}

QOhosOptional<KeyEvent> KeyEvent::createFromNativeEvent(const ::Input_KeyEvent *event)
{
    auto jsWindowId = JsWindowId(::OH_Input_GetKeyEventWindowId(event));

    auto actionValue = ::OH_Input_GetKeyEventAction(event);
    auto optMappedAction = tryMapKeyEventAction(actionValue);
    if (!optMappedAction.hasValue()) {
        qOhosPrintfError(
            "%s: Filter for jsWindowId: %f, received unrecognized key event action: %d, event will be ignored",
            Q_FUNC_INFO, jsWindowId.value(), actionValue);
        return makeEmptyQOhosOptional();
    }

    KeyEvent keyEvent = {
        .jsWindowId = jsWindowId,
        .actionTime = std::chrono::microseconds(::OH_Input_GetKeyEventActionTime(event)),
        .action = optMappedAction.value(),
        .keyCode = ::OH_Input_GetKeyEventKeyCode(event),
    };

    return makeQOhosOptional(keyEvent);
}

QInputDevice::DeviceType getTouchDeviceType(const ::ArkUI_UIInputEvent *inputEvent)
{
    const auto sourceType = ::OH_ArkUI_UIInputEvent_GetSourceType(inputEvent);
    if (sourceType == ::UI_INPUT_EVENT_SOURCE_TYPE_UNKNOWN)
        qOhosWarning(QtForOhos) << "Obtained ArkUI unknown source type for input event";
    return sourceType == ::UI_INPUT_EVENT_SOURCE_TYPE_TOUCH_SCREEN
        ? QInputDevice::DeviceType::TouchScreen
        : QInputDevice::DeviceType::TouchPad;
}

QPointF getPointerEventLocalPosition(const ::ArkUI_UIInputEvent *event)
{
    return QPointF(
        callArkUi(Q_OHOS_NAMED_FUNC(::OH_ArkUI_PointerEvent_GetX), event),
        callArkUi(Q_OHOS_NAMED_FUNC(::OH_ArkUI_PointerEvent_GetY), event));
}

QPointF getPointerEventDisplayPosition(const ::ArkUI_UIInputEvent *event)
{
    return QPointF(
        callArkUi(Q_OHOS_NAMED_FUNC(::OH_ArkUI_PointerEvent_GetDisplayX), event),
        callArkUi(Q_OHOS_NAMED_FUNC(::OH_ArkUI_PointerEvent_GetDisplayY), event));
}

ch::milliseconds getInputEventTimeMs(const ::ArkUI_UIInputEvent *event)
{
    auto eventTime = ch::nanoseconds(callArkUi(Q_OHOS_NAMED_FUNC(::OH_ArkUI_UIInputEvent_GetEventTime), event));
    return ch::duration_cast<ch::milliseconds>(eventTime);
}

NativeNodeMouseEvent NativeNodeMouseEvent::makeFromUiInputEvent(::ArkUI_UIInputEvent *event)
{
    return NativeNodeMouseEvent {
        .timestampMs = getInputEventTimeMs(event),
        .localPosition = getPointerEventLocalPosition(event),
        .displayPosition = getPointerEventDisplayPosition(event),
        .button = callArkUi(Q_OHOS_NAMED_FUNC(::OH_ArkUI_MouseEvent_GetMouseButton), event),
        .action = callArkUi(Q_OHOS_NAMED_FUNC(::OH_ArkUI_MouseEvent_GetMouseAction), event),
        .modifiers = readKeyModifiersFromOhosUiInputEvent(event),
    };
}

}

QT_END_NAMESPACE
