// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <info/application_target_sdk_version.h>
#include <multimodalinput/oh_input_manager.h>
#include <render/qohosnativexcomponentinputhandler.h>

#include <QtCore/QCoreApplication>
#include <QtCore/qspan.h>
#include <native_window/external_window.h>
#include <qohosinputmethodeventhandler.h>
#include <qohosjsmain.h>
#include <qohosxcomponentkeyevent.h>
#include <qohosplatformintegration.h>
#include <qohosplatformtheme.h>
#include <qohosutils.h>
#include <render/qohosbatchingrequestshandler.h>
#include <render/qohosview.h>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE

namespace
{

constexpr auto mouseMotionEventMinAgeForDrop = std::chrono::milliseconds(20);

constexpr auto touchEventMinAgeForDrop = std::chrono::milliseconds(20);

const std::array<OhosKeyToModifier, 6> keysToModifiers = {{
    {OhosKeyboardModifier::CTRL, {::KEYCODE_CTRL_LEFT, ::KEYCODE_CTRL_RIGHT}, &::OH_Input_GetKeyPressed, ::KEY_PRESSED},
    {OhosKeyboardModifier::ALT, {::KEYCODE_ALT_LEFT, ::KEYCODE_ALT_RIGHT}, &::OH_Input_GetKeyPressed, ::KEY_PRESSED},
    {OhosKeyboardModifier::SHIFT, {::KEYCODE_SHIFT_LEFT, ::KEYCODE_SHIFT_RIGHT}, &::OH_Input_GetKeyPressed, ::KEY_PRESSED},
    {OhosKeyboardModifier::LOGO, {::KEYCODE_META_LEFT, ::KEYCODE_META_RIGHT}, &::OH_Input_GetKeyPressed, ::KEY_PRESSED},
    {OhosKeyboardModifier::CAPS_LOCK, {::KEYCODE_CAPS_LOCK}, &::OH_Input_GetKeySwitch, ::KEY_SWITCH_ON},
    {OhosKeyboardModifier::NUM_LOCK, {::KEYCODE_NUM_LOCK}, &::OH_Input_GetKeySwitch, ::KEY_SWITCH_ON},
}};

bool qtAcceptsEvents()
{
    // FIXME: make sure we use thread-safe check here
    return QOhosPlatformIntegration::instance() != nullptr
        && !QCoreApplication::startingUp()
        && !QCoreApplication::closingDown();
}

std::tuple<QPointF, QPointF> getLocalAndGlobalPointsOrDefault(OH_NativeXComponent *component, void *window)
{
    OH_NativeXComponent_MouseEvent event;
    return OH_NativeXComponent_GetMouseEvent(component, window, &event) == OH_NATIVEXCOMPONENT_RESULT_SUCCESS
        ? std::tuple<QPointF, QPointF>({{event.x, event.y}, {event.screenX, event.screenY}})
        : std::tuple<QPointF, QPointF>({{}, {}});
}

QOhosOptional<QPointF> tryGetTouchPointDisplayPosition(QXComponentRender xComponent, std::int32_t touchPointIndex)
{
    float x;
    float y;

    return
        ::OH_NativeXComponent_GetTouchPointDisplayX(xComponent.handle(), touchPointIndex, &x)
            == ::OH_NATIVEXCOMPONENT_RESULT_SUCCESS
        && ::OH_NativeXComponent_GetTouchPointDisplayY(xComponent.handle(), touchPointIndex, &y)
            == ::OH_NATIVEXCOMPONENT_RESULT_SUCCESS
                ? makeQOhosOptional(QPointF{x, y})
                : makeEmptyQOhosOptional();
}

QOhosOptional<QOhosTouchEventTouchPointData> tryMakeTouchEventPointData(
    QXComponentRender xComponent, const OH_NativeXComponent_TouchEvent &touchEvent,
    std::uint32_t pointIndex)
{
    auto touchDisplayPosition = tryGetTouchPointDisplayPosition(xComponent, pointIndex);
    if (!touchDisplayPosition.hasValue())
        return makeEmptyQOhosOptional();

    ::OH_NativeXComponent_TouchPointToolType toolType = ::OH_NATIVEXCOMPONENT_TOOL_TYPE_UNKNOWN;
    std::int32_t resToolType = ::OH_NativeXComponent_GetTouchPointToolType(xComponent.handle(), pointIndex, &toolType);
    if (resToolType != ::OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        qOhosCritical(QtForOhos)
            << "OH_NativeXComponent_GetTouchPointToolType() failed,"
            << "touchPoint id:" << touchEvent.touchPoints[pointIndex].id << "result:" << resToolType;
        toolType = ::OH_NATIVEXCOMPONENT_TOOL_TYPE_UNKNOWN;
    }

    return makeQOhosOptional(QOhosTouchEventTouchPointData{
        .touchPoint = touchEvent.touchPoints[pointIndex],
        .toolType = toolType,
        .displayPosition = touchDisplayPosition.value(),
    });
}

QInputDevice::DeviceType getTouchDeviceType(::OH_NativeXComponent *component, std::int32_t pointId)
{
    ::OH_NativeXComponent_EventSourceType sourceType = ::OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN;
    std::int32_t res = ::OH_NativeXComponent_GetTouchEventSourceType(component, pointId, &sourceType);
    if (res != ::OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        qOhosCritical(QtForOhos)
            << "OH_NativeXComponent_GetTouchEventSourceType() failed,"
            << "pointId:" << pointId << "result:" << res;
        return QInputDevice::DeviceType::TouchPad;
    }

    return sourceType == ::OH_NATIVEXCOMPONENT_SOURCE_TYPE_TOUCHSCREEN
        ? QInputDevice::DeviceType::TouchScreen
        : QInputDevice::DeviceType::TouchPad;
}

bool isModifierKey(OH_NativeXComponent_KeyCode keyCode)
{
    switch (keyCode) {
    case OH_NativeXComponent_KeyCode::KEY_SHIFT_LEFT:
    case OH_NativeXComponent_KeyCode::KEY_SHIFT_RIGHT:
    case OH_NativeXComponent_KeyCode::KEY_ALT_LEFT:
    case OH_NativeXComponent_KeyCode::KEY_ALT_RIGHT:
    case OH_NativeXComponent_KeyCode::KEY_CTRL_LEFT:
    case OH_NativeXComponent_KeyCode::KEY_CTRL_RIGHT:
    case OH_NativeXComponent_KeyCode::KEY_META_LEFT:
    case OH_NativeXComponent_KeyCode::KEY_META_RIGHT:
    case OH_NativeXComponent_KeyCode::KEY_CAPS_LOCK:
    case OH_NativeXComponent_KeyCode::KEY_NUM_LOCK:
        return true;
    default:
        return false;
    }
}

QOhosOptional<QEvent::Type> tryMapXComponentMouseEventActionToQEventType(::OH_NativeXComponent_MouseEventAction action)
{
    switch (action) {
    case OH_NATIVEXCOMPONENT_MOUSE_PRESS:
        return makeQOhosOptional(QEvent::MouseButtonPress);
    case OH_NATIVEXCOMPONENT_MOUSE_RELEASE:
        return makeQOhosOptional(QEvent::MouseButtonRelease);
    case OH_NATIVEXCOMPONENT_MOUSE_MOVE:
        return makeQOhosOptional(QEvent::MouseMove);
    case OH_NATIVEXCOMPONENT_MOUSE_NONE:
    case OH_NATIVEXCOMPONENT_MOUSE_CANCEL:
        break;
    }
    return makeEmptyQOhosOptional();
}

QOhosOptional<Qt::MouseButton> tryMapXComponentMouseButtonToQt(::OH_NativeXComponent_MouseEventButton button)
{
    switch (button) {
    case OH_NATIVEXCOMPONENT_LEFT_BUTTON:
        return makeQOhosOptional(Qt::LeftButton);
    case OH_NATIVEXCOMPONENT_MIDDLE_BUTTON:
        return makeQOhosOptional(Qt::MiddleButton);
    case OH_NATIVEXCOMPONENT_RIGHT_BUTTON:
        return makeQOhosOptional(Qt::RightButton);
    case OH_NATIVEXCOMPONENT_BACK_BUTTON:
        return makeQOhosOptional(Qt::BackButton);
    case OH_NATIVEXCOMPONENT_FORWARD_BUTTON:
        return makeQOhosOptional(Qt::ForwardButton);
    case OH_NATIVEXCOMPONENT_NONE_BUTTON:
        break;
    }
    return makeEmptyQOhosOptional();
}


}

QOhosNativeXComponentInputHandler::QOhosNativeXComponentInputHandler(
    QXComponentRender xcomponent,
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef)
    : m_xComponent(xcomponent)
    , m_qWindowRef(qWindowRef)
    , m_imEventHandlerRef(imEventHandlerRef)
{
}

void QOhosNativeXComponentInputHandler::handleTouchEvent(void *window)
{
    if (!qtAcceptsEvents())
        return;

    OH_NativeXComponent_TouchEvent touchEvent;
    int32_t retcode = OH_NativeXComponent_GetTouchEvent(m_xComponent.handle(), window, &touchEvent);
    if (retcode != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        qOhosCritical(QtForOhos) << "Unable to obtain TouchEvent from XComponent";
        return;
    }

    std::vector<QOhosTouchEventTouchPointData> validTouchPoints;

    ::OH_NativeXComponent_EventSourceType sourceType = ::OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN;
    std::int32_t resSourceType = ::OH_NativeXComponent_GetTouchEventSourceType(m_xComponent.handle(), touchEvent.id, &sourceType);
    if (resSourceType != ::OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        qOhosCritical(QtForOhos)
            << "OH_NativeXComponent_GetTouchEventSourceType() failed,"
            << "touchEvent id:" << touchEvent.id << "result:" << resSourceType;
        sourceType = ::OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN;
    }

    bool isMouseOrUnknownTouchType =
        sourceType == OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN
        || sourceType == OH_NATIVEXCOMPONENT_SOURCE_TYPE_MOUSE;
    if (isMouseOrUnknownTouchType)
        return;

    for (std::uint32_t pointIndex = 0; pointIndex < touchEvent.numPoints; ++pointIndex) {
        auto pointData = tryMakeTouchEventPointData(m_xComponent, touchEvent, pointIndex);
        if (pointData.hasValue())
            validTouchPoints.push_back(pointData.value());
    }

    if (validTouchPoints.empty())
        return;

    if (!m_optTouchEventsHandler) {
        auto weakSelf = sharedFromThis().toWeakRef();
        m_optTouchEventsHandler = makeQtOhosSimpleBatchingQtRequestsHandler<TouchEvent>(
            m_imEventHandlerRef.toQObjectThreadSafeRef(),
            [weakSelf](std::vector<TouchEvent> &&batch) {
                auto sharedSelf = weakSelf.toStrongRef();
                if (!sharedSelf.isNull())
                    sharedSelf->processTouchEventsInQtThread(std::move(batch));
            });
    }

    m_optTouchEventsHandler(
        TouchEvent{
            .timestamp = std::chrono::steady_clock::now(),
            .touchTimeStamp = std::chrono::nanoseconds(touchEvent.timeStamp),
            .touchPoints = validTouchPoints,
            .deviceType = getTouchDeviceType(m_xComponent.handle(), touchEvent.id),
            .modifiers = readKeyModifiersFromKeyState(
                QSpan(keysToModifiers.data(), keysToModifiers.size())),
        });
}

void QOhosNativeXComponentInputHandler::handleMouseEvent(void *window)
{
    if (!qtAcceptsEvents())
        return;
    OH_NativeXComponent_MouseEvent ev;
    const std::int32_t retcode = OH_NativeXComponent_GetMouseEvent(m_xComponent.handle(), window, &ev);
    if (retcode != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        qOhosCritical(QtForOhos) << "Unable to retrieve MouseEvent from XComponent";
        return;
    }

    if (!m_optMouseEventsHandler) {
        auto weakSelf = sharedFromThis().toWeakRef();
        m_optMouseEventsHandler = makeQtOhosBatchingQtRequestsHandler<std::vector<MouseEvent>>(
            m_imEventHandlerRef.toQObjectThreadSafeRef(),
            [weakSelf](std::vector<MouseEvent> &&batch) {
                auto sharedSelf = weakSelf.toStrongRef();
                if (!sharedSelf.isNull())
                    sharedSelf->processMouseEventsInQtThread(std::move(batch));
            });
    }

    m_optMouseEventsHandler(
        [&](std::vector<MouseEvent> &batch) {
            auto now = std::chrono::steady_clock::now();
            MouseEvent newEvent{now, ev};
            if (!batch.empty() && mayDropMouseEvent(now, batch.back(), newEvent))
                batch.pop_back();
            batch.push_back(newEvent);
        });
}

void QOhosNativeXComponentInputHandler::handleHoverEvent(void *nativeWindow, bool isHover)
{
    if (!qtAcceptsEvents())
        return;
    auto __dbg = make_QCScopedDebug("QOhosNativeXComponentInputDispatcher::dispatchHoverEvent");

    QPointF localPoint;
    QPointF globalPoint;
    std::tie(localPoint, globalPoint) = getLocalAndGlobalPointsOrDefault(m_xComponent.handle(), nativeWindow);

    invokeMethodLaterIfSelfExists(
        m_imEventHandlerRef.toQObjectThreadSafeRef(),
        [isHover, localPoint, globalPoint](QOhosNativeXComponentInputHandler &self) {
            self.m_imEventHandlerRef.data()->onHoverEvent(
                QOhosHoverEvent {
                    .targetWindow = self.m_qWindowRef.data(),
                    .localPosition = localPoint,
                    .globalPosition = globalPoint,
                    .isHover = isHover,
                });
        });
}

void QOhosNativeXComponentInputHandler::processMouseEventsInQtThread(std::vector<MouseEvent> &&batch)
{
    auto now = std::chrono::steady_clock::now();

    QWindow *associatedWindowHandle = m_qWindowRef.data();
    for (std::size_t i = 0; i < batch.size(); ++i) {
        if (i + 1 < batch.size() && mayDropMouseEvent(now, batch[i], batch[i + 1]))
            continue;

        auto event = batch[i].event;

        auto eventType = tryMapXComponentMouseEventActionToQEventType(event.action);
        if (!eventType.hasValue()) {
            qOhosPrintfDebug(
                "%s: got unsupported action in mouse event (%d), ignoring",
                Q_FUNC_INFO, event.action);
            continue;
        }

        auto optButton = tryMapXComponentMouseButtonToQt(event.button);
        if (!optButton.hasValue())
           qOhosWarning(QtForOhos) << "Unexpected mouse button!";

        // HACK / FIXME
        // Temporary globalPos set to in OH window po  - as we still live in single window world, so the
        // window is our screen...
        QPointF globalPos(event.screenX, event.screenY);
        QPointF localPos(event.x, event.y);

        m_imEventHandlerRef.data()->onMouseEvent(
            {
                .targetWindow = associatedWindowHandle,
                .timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::nanoseconds(event.timestamp)),
                .localPosition = localPos,
                .globalPosition = globalPos,
                .button = optButton.valueOr(Qt::NoButton),
                .eventType = eventType.value(),
                .modifiers = readKeyModifiersFromKeyState(
                    QSpan(keysToModifiers.data(), keysToModifiers.size())),
            });
    }
}

void QOhosNativeXComponentInputHandler::processTouchEventsInQtThread(std::vector<TouchEvent> &&batch)
{
    QWindow *window = m_qWindowRef.data();
    auto now = std::chrono::steady_clock::now();
    for (auto &event : batch) {
        if (now - event.timestamp >= touchEventMinAgeForDrop) {
            event.touchPoints.erase(
                QtOhos::removeMatchingWithLookahead(
                    event.touchPoints.begin(), event.touchPoints.end(),
                    [](const auto &touchPointData, const auto &nextTouchPointData) {
                        return
                            touchPointData.touchPoint.type == OH_NATIVEXCOMPONENT_MOVE
                            && nextTouchPointData.touchPoint.type == OH_NATIVEXCOMPONENT_MOVE;
                    }),
                event.touchPoints.end());
        }
        m_imEventHandlerRef.data()->onTouchEventFromXComponent(
            window, event.touchTimeStamp, event.touchPoints, event.deviceType, event.modifiers);
    }
}

void QOhosNativeXComponentInputHandler::invokeMethodLaterIfSelfExists(
    QtOhos::QObjectThreadSafeRef ctxRef, std::function<void(QOhosNativeXComponentInputHandler &)> callback)
{
    auto weakSelf = sharedFromThis().toWeakRef();
    ctxRef.visitInQtThreadIfAlive(
        [weakSelf, callback = std::move(callback)](auto &) {
            auto sharedSelf = weakSelf.toStrongRef();
            if (!sharedSelf.isNull()) {
                callback(*sharedSelf);
            };
        });
}

bool QOhosNativeXComponentInputHandler::mayDropMouseEvent(
    std::chrono::steady_clock::time_point now,
    const MouseEvent &event, const MouseEvent &nextEvent)
{
    return
        now - event.timestamp >= mouseMotionEventMinAgeForDrop
        && event.event.action == OH_NATIVEXCOMPONENT_MOUSE_MOVE
        && nextEvent.event.action == OH_NATIVEXCOMPONENT_MOUSE_MOVE;
}

void QOhosNativeXComponentInputHandler::handleKeyEvent()
{
    if (!qtAcceptsEvents())
        return;
    OH_NativeXComponent_KeyEvent *keyEvent = nullptr;
    OH_NativeXComponent_KeyAction keyAction = {};
    OH_NativeXComponent_KeyCode keyCode = {};

    if (OH_NativeXComponent_GetKeyEvent(m_xComponent.handle(), &keyEvent) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        qOhosCritical(QtForOhos) << "Could not obtain correct KeyEvent!";
        return;
    }

    if (OH_NativeXComponent_GetKeyEventAction(keyEvent, &keyAction) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        qOhosCritical(QtForOhos) << "Could not obtain correct Action for given KeyEvent!";
        return;
    }

    if (OH_NativeXComponent_GetKeyEventCode(keyEvent, &keyCode) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        qOhosCritical(QtForOhos) << "Could not obtain correct KeyCode for given KeyEvent!";
        return;
    }

    const auto keyModifiers = readKeyModifiersFromKeyState(
        QSpan(keysToModifiers.data(), keysToModifiers.size()));

    auto ohosKeyEvent = makeQOhosXComponentKeyEvent(keyAction, keyCode, keyModifiers);

    if (isModifierKey(keyCode) && m_lastKeyEvent && m_lastKeyEvent->equals(*ohosKeyEvent))
        return;
    m_lastKeyEvent = ohosKeyEvent;

    invokeMethodLaterIfSelfExists(
        m_imEventHandlerRef.toQObjectThreadSafeRef(),
        [ohosKeyEvent](QOhosNativeXComponentInputHandler &self) {
            self.m_imEventHandlerRef.data()->onKeyEvent(*ohosKeyEvent, self.m_qWindowRef.data());
    });
}

QT_END_NAMESPACE

