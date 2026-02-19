// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qohosnativeaxiseventhandler.h>

#include <qarkui/input.h>
#include <render/qohosnativegestureshandler.h>

QT_BEGIN_NAMESPACE

namespace {

class QOhosAxisEventHandler final : public std::enable_shared_from_this<QOhosAxisEventHandler>
{
public:
    QOhosAxisEventHandler(
        QtOhos::QThreadSafeRef<QWindow> qWindowRef,
        QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef);

    void handleUiAxisEvent(ArkUI_UIInputEvent *event);

private:
    QtOhos::QThreadSafeRef<QWindow> m_qWindowRef;
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> m_imEventHandlerRef;

    QOhosConsumer<const QOhosNativeGestureEvent &> m_nativeGesturesHandler;
};

QOhosOptional<Qt::NativeGestureType> getQtGestureType(::InputEvent_AxisAction ohAxisActionType)
{
    switch (ohAxisActionType) {
    case ::AXIS_ACTION_BEGIN:
        return makeQOhosOptional(Qt::BeginNativeGesture);
    case ::AXIS_ACTION_END :
    case ::AXIS_ACTION_CANCEL:
        return makeQOhosOptional(Qt::EndNativeGesture);
    case ::AXIS_ACTION_UPDATE:
        return makeQOhosOptional(Qt::ZoomNativeGesture);
    }
    return makeEmptyQOhosOptional();
}

QOhosAxisEventHandler::QOhosAxisEventHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef)
    : m_qWindowRef(qWindowRef)
    , m_imEventHandlerRef(imEventHandlerRef)
    , m_nativeGesturesHandler(
        makeQOhosNativeGesturesHandler(
            m_qWindowRef,
            [lastTotalScale = 1.0](QOhosNativeGestureEvent &event) mutable {
                const auto totalScale =
                    event.gestureType == Qt::BeginNativeGesture
                        ? 1.0
                        : event.value;
                const auto scaleFactor =
                    event.gestureType == Qt::BeginNativeGesture
                        ? totalScale
                        : totalScale / lastTotalScale;
                lastTotalScale = totalScale;
                event.value = scaleFactor;
            }))
{
}

void QOhosAxisEventHandler::handleUiAxisEvent(ArkUI_UIInputEvent *event)
{
    const auto now = std::chrono::steady_clock::now();

    auto eventTimestamp = OH_ArkUI_UIInputEvent_GetEventTime(event);
    auto localPosition = QArkUi::getPointerEventLocalPosition(event);
    auto screenPosition = QArkUi::getPointerEventDisplayPosition(event);
    double horizontalAxisValue = OH_ArkUI_AxisEvent_GetHorizontalAxisValue(event);
    double verticalAxisValue = OH_ArkUI_AxisEvent_GetVerticalAxisValue(event);
    auto eventToolType = OH_ArkUI_UIInputEvent_GetToolType(event);
    auto eventAxisAction = OH_ArkUI_AxisEvent_GetAxisAction(event);
    std::int32_t wheelScrollLines;
    wheelScrollLines = OH_ArkUI_AxisEvent_GetScrollStep(event);

    QOhosWheelEvent ohosWheelEvent = {
        .timestamp = eventTimestamp,
        .localPoint = localPosition,
        .globalPoint = screenPosition,
        .horizontalValue = horizontalAxisValue,
        .verticalValue = verticalAxisValue,
        .eventToolType = eventToolType,
        .axisAction = eventAxisAction,
        .wheelScrollLines = wheelScrollLines,
        .modifiers = readKeyModifiersFromOhosUiInputEvent(event),
    };

    auto weakSelf = QtOhos::makeWeakPtr(shared_from_this());
    m_imEventHandlerRef.visitInQtThreadIfAlive(
        [weakSelf, ohosWheelEvent, qWindowRef = m_qWindowRef](auto &eventHandler) {
            auto sharedSelf = weakSelf.lock();
            if (!sharedSelf) {
                return;
            };
            eventHandler.onMouseWheelEvent(ohosWheelEvent, qWindowRef.data());
        });

    const auto totalScale = OH_ArkUI_AxisEvent_GetPinchAxisScaleValue(event);

    if (qFuzzyIsNull(horizontalAxisValue) && qFuzzyIsNull(verticalAxisValue) && !qFuzzyIsNull(totalScale)) {
        const auto deviceType = QArkUi::getTouchDeviceType(event);
        const auto gestureType = getQtGestureType(
            static_cast<InputEvent_AxisAction>(eventAxisAction)).valueOr(Qt::ZoomNativeGesture);

        QOhosNativeGestureEvent newEvent {
            .timestamp = now,
            .gestureTimestamp = eventTimestamp,
            .value = totalScale,
            .localPosition = localPosition,
            .displayBasedPosition = screenPosition,
            .gestureType = gestureType,
            .deviceType = deviceType,
        };

        m_nativeGesturesHandler(newEvent);
    }
}

}

QOhosConsumer<::ArkUI_UIInputEvent *> makeQOhosNativeAxisEventHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef)
{
    auto handler = std::make_shared<QOhosAxisEventHandler>(qWindowRef, imEventHandlerRef);
    return [handler](::ArkUI_UIInputEvent *inputEvent) {
        handler->handleUiAxisEvent(inputEvent);
    };
}

QT_END_NAMESPACE
