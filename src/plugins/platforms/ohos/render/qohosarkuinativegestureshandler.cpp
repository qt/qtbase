// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosarkuinativegestureshandler.h"

#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qpoint.h>
#include <QtGui/qwindow.h>
#include <arkui/native_gesture.h>
#include <arkui/native_interface.h>
#include <arkui/native_type.h>
#include <arkui/ui_input_event.h>
#include <chrono>
#include <qarkui/qembeddedwindownode.h>
#include <qarkui/input.h>
#include <qohosplatformintegration.h>
#include <qohosutils.h>
#include <qpa/qwindowsysteminterface.h>
#include <render/qohosnativegestureshandler.h>

QT_BEGIN_NAMESPACE

namespace {

Qt::NativeGestureType getQtGestureType(
    ::ArkUI_GestureEventActionType ohEventActionType, Qt::NativeGestureType defaultGestureType)
{
    switch (ohEventActionType) {
    case ::GESTURE_EVENT_ACTION_ACCEPT:
        return Qt::BeginNativeGesture;
    case ::GESTURE_EVENT_ACTION_END:
        return Qt::EndNativeGesture;
    case ::GESTURE_EVENT_ACTION_CANCEL:
        return Qt::EndNativeGesture;
    case ::GESTURE_EVENT_ACTION_UPDATE:
        return defaultGestureType;
    }
    return defaultGestureType;
}

class QOhosArkUiNativeGesturesHandler final : public QEnableSharedFromThis<QOhosArkUiNativeGesturesHandler>
{
public:
    QOhosArkUiNativeGesturesHandler(QtOhos::QThreadSafeRef<QWindow> qWindowRef);

    void handleNativeGestureEvent(const QArkUi::NativeGestureInfo &nativeGestureInfo);

private:
    void handleRotationGestureEvent(const ::ArkUI_GestureEvent *gestureEvent);
    void handlePinchGestureEvent(const ::ArkUI_GestureEvent *gestureEvent);

    QOhosConsumer<const QOhosNativeGestureEvent &> m_baseGesturesHandler;
    qreal m_lastTotalScale{1.0};
    QtOhos::QThreadSafeRef<QWindow> m_qWindowRef;
};

QOhosArkUiNativeGesturesHandler::QOhosArkUiNativeGesturesHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef)
    : m_baseGesturesHandler(makeQOhosNativeGesturesHandler(qWindowRef))
    , m_qWindowRef(qWindowRef)
{
}

void QOhosArkUiNativeGesturesHandler::handleNativeGestureEvent(
    const QArkUi::NativeGestureInfo &nativeGestureInfo)
{
    switch (nativeGestureInfo.type) {
    case QArkUi::NativeGestureInfo::GestureType::Pinch:
        handlePinchGestureEvent(nativeGestureInfo.event);
        break;
    case QArkUi::NativeGestureInfo::GestureType::Rotation:
        handleRotationGestureEvent(nativeGestureInfo.event);
        break;
    }
}

void QOhosArkUiNativeGesturesHandler::handleRotationGestureEvent(
    const ::ArkUI_GestureEvent *gestureEvent)
{
    const auto timestamp = std::chrono::steady_clock::now();
    const auto actionType = ::OH_ArkUI_GestureEvent_GetActionType(gestureEvent);
    const auto qtGestureType = getQtGestureType(actionType, Qt::RotateNativeGesture);
    const auto delta = ::OH_ArkUI_RotationGesture_GetAngle(gestureEvent);

    const auto *inputEvent = ::OH_ArkUI_GestureEvent_GetRawInputEvent(gestureEvent);
    const auto gestureTimestamp = ::OH_ArkUI_UIInputEvent_GetEventTime(inputEvent);
    const auto localPosition = QArkUi::getPointerEventLocalPosition(inputEvent);
    const auto displayBasedPosition = QArkUi::getPointerEventDisplayPosition(inputEvent);
    const auto deviceType = QArkUi::getTouchDeviceType(inputEvent);

    QOhosNativeGestureEvent newEvent {
        .timestamp = timestamp,
        .gestureTimestamp = gestureTimestamp,
        .value = delta,
        .localPosition = localPosition,
        .displayBasedPosition = displayBasedPosition,
        .gestureType = qtGestureType,
        .deviceType = deviceType,
    };

    m_baseGesturesHandler(newEvent);
}

void QOhosArkUiNativeGesturesHandler::handlePinchGestureEvent(const ::ArkUI_GestureEvent *gestureEvent)
{
    const auto timestamp = std::chrono::steady_clock::now();
    const auto actionType = ::OH_ArkUI_GestureEvent_GetActionType(gestureEvent);
    const auto qtGestureType = getQtGestureType(actionType, Qt::ZoomNativeGesture);

    const auto totalScale = ::OH_ArkUI_PinchGesture_GetScale(gestureEvent);
    const auto localX = ::OH_ArkUI_PinchGesture_GetCenterX(gestureEvent);
    const auto localY = ::OH_ArkUI_PinchGesture_GetCenterY(gestureEvent);
    const QPointF localPosition { localX, localY };

    const auto *inputEvent = ::OH_ArkUI_GestureEvent_GetRawInputEvent(gestureEvent);

    const auto toolType = ::OH_ArkUI_UIInputEvent_GetToolType(inputEvent);
    if (toolType == UI_INPUT_EVENT_TOOL_TYPE_MOUSE || toolType == UI_INPUT_EVENT_TOOL_TYPE_TOUCHPAD)
        return;

    const auto gestureTimestamp = ::OH_ArkUI_UIInputEvent_GetEventTime(inputEvent);
    const auto displayBasedPosition = QArkUi::getPointerEventDisplayPosition(inputEvent);
    const auto deviceType = QArkUi::getTouchDeviceType(inputEvent);

    const auto scaleFactor =
        qtGestureType == Qt::BeginNativeGesture
            ? totalScale
            : totalScale / m_lastTotalScale;
    m_lastTotalScale = totalScale;

    QOhosNativeGestureEvent newEvent {
        .timestamp = timestamp,
        .gestureTimestamp = gestureTimestamp,
        .value = scaleFactor,
        .localPosition = localPosition,
        .displayBasedPosition = displayBasedPosition,
        .gestureType = qtGestureType,
        .deviceType = deviceType,
    };

    m_baseGesturesHandler(newEvent);
}

}

QOhosConsumer<const QArkUi::NativeGestureInfo &> makeQOhosArkUiNativeGesturesHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef)
{
    return [gesturesHandler = QSharedPointer<QOhosArkUiNativeGesturesHandler>::create(qWindowRef)](
        const QArkUi::NativeGestureInfo &nativeGestureInfo) {
            gesturesHandler->handleNativeGestureEvent(nativeGestureInfo);
        };
}

QT_END_NAMESPACE
