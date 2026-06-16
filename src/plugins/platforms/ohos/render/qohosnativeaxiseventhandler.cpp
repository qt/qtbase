// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <render/qohosnativeaxiseventhandler.h>

#include <qarkui/input.h>
#include <qarkui/qarkuiutils.h>
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
    void handleUiCoastingAxisEvent(::ArkUI_UIInputEvent *event);

private:
    QtOhos::QThreadSafeRef<QWindow> m_qWindowRef;
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> m_imEventHandlerRef;

    QOhosOptional<QPointF> m_localPosition;
    QOhosOptional<QPointF> m_globalPosition;
    QOhosOptional<std::int32_t> m_wheelScrollLines;

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

Qt::ScrollPhase convertArkUiAxisEventActionToQtScrollPhase(std::int32_t arkUiAxisEventAction)
{
    switch (arkUiAxisEventAction) {
    case UI_AXIS_EVENT_ACTION_NONE:
        return Qt::NoScrollPhase;
    case UI_AXIS_EVENT_ACTION_BEGIN:
        return Qt::ScrollBegin;
    case UI_AXIS_EVENT_ACTION_UPDATE:
        return Qt::ScrollUpdate;
    case UI_AXIS_EVENT_ACTION_END:
        return Qt::ScrollEnd;
    case UI_AXIS_EVENT_ACTION_CANCEL:
        return Qt::ScrollEnd;
    }

    qOhosReportFatalErrorAndAbort(
        "Received unsupported UI_AXIS_EVENT_ACTION: %d", arkUiAxisEventAction);
}

Qt::ScrollPhase convertArkUiCoastingAxisEventActionToQtScrollPhase(::ArkUI_CoastingAxisEventPhase phase)
{
    switch (phase) {
    case ::ARKUI_COASTING_AXIS_EVENT_PHASE_NONE:
        return Qt::NoScrollPhase;
    case ::ARKUI_COASTING_AXIS_EVENT_PHASE_BEGIN:
        return Qt::ScrollBegin;
    case ::ARKUI_COASTING_AXIS_EVENT_PHASE_UPDATE:
        return Qt::ScrollMomentum;
    case ::ARKUI_COASTING_AXIS_EVENT_PHASE_END:
        return Qt::ScrollEnd;
    }

    qOhosReportFatalErrorAndAbort("Received unsupported ArkUI_CoastingAxisEventPhase: %d", phase);
};

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
    auto globalPosition = QArkUi::getPointerEventGlobalPosition(event);
    double horizontalAxisValue = OH_ArkUI_AxisEvent_GetHorizontalAxisValue(event);
    double verticalAxisValue = OH_ArkUI_AxisEvent_GetVerticalAxisValue(event);
    auto eventToolType = OH_ArkUI_UIInputEvent_GetToolType(event);
    auto eventAxisAction = OH_ArkUI_AxisEvent_GetAxisAction(event);
    std::int32_t wheelScrollLines;
    wheelScrollLines = OH_ArkUI_AxisEvent_GetScrollStep(event);

    m_localPosition = makeQOhosOptional(localPosition);
    m_globalPosition = makeQOhosOptional(globalPosition);
    m_wheelScrollLines = makeQOhosOptional(wheelScrollLines);

    QOhosWheelEvent ohosWheelEvent = {
        .timestamp = eventTimestamp,
        .localPoint = localPosition,
        .globalPoint = globalPosition,
        .horizontalValue = horizontalAxisValue,
        .verticalValue = verticalAxisValue,
        .eventToolType = eventToolType,
        .scrollPhase = convertArkUiAxisEventActionToQtScrollPhase(eventAxisAction),
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
            static_cast<InputEvent_AxisAction>(eventAxisAction)).value_or(Qt::ZoomNativeGesture);

        QOhosNativeGestureEvent newEvent {
            .timestamp = now,
            .gestureTimestamp = eventTimestamp,
            .value = totalScale,
            .localPosition = localPosition,
            .globalPosition = globalPosition,
            .gestureType = gestureType,
            .deviceType = deviceType,
        };

        m_nativeGesturesHandler(newEvent);
    }
}

void QOhosAxisEventHandler::handleUiCoastingAxisEvent(::ArkUI_UIInputEvent *event)
{
    if (!m_localPosition.has_value() || !m_globalPosition.has_value() || !m_wheelScrollLines.has_value()) {
        qOhosPrintfWarning(
            "%s: Cannot create wheel event - incomplete data, ignoring coasting event.", Q_FUNC_INFO);
        return;
    }

    auto *coastingAxisEvent = QArkUi::callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_UIInputEvent_GetCoastingAxisEvent), event);
    auto eventTime = QArkUi::callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_CoastingAxisEvent_GetEventTime), coastingAxisEvent);
    auto phase = QArkUi::callArkUi(
        Q_OHOS_NAMED_FUNC(::OH_ArkUI_CoastingAxisEvent_GetPhase), coastingAxisEvent);
    auto delta = QPointF(
        QArkUi::callArkUi(Q_OHOS_NAMED_FUNC(::OH_ArkUI_CoastingAxisEvent_GetDeltaX), coastingAxisEvent),
        QArkUi::callArkUi(Q_OHOS_NAMED_FUNC(::OH_ArkUI_CoastingAxisEvent_GetDeltaY), coastingAxisEvent));

    const QFlags<OhosKeyboardModifier> coastingAxisEventModifiersFallback = {};

    QOhosWheelEvent ohosWheelEvent = {
        .timestamp = eventTime,
        .localPoint = m_localPosition.value(),
        .globalPoint = m_globalPosition.value(),
        .horizontalValue = delta.x(),
        .verticalValue = delta.y(),
        .eventToolType = ::UI_INPUT_EVENT_TOOL_TYPE_TOUCHPAD,
        .scrollPhase = convertArkUiCoastingAxisEventActionToQtScrollPhase(phase),
        .wheelScrollLines = m_wheelScrollLines.value(),
        .modifiers = coastingAxisEventModifiersFallback,
    };

    auto weakSelf = QtOhos::makeWeakPtr(shared_from_this());
    m_imEventHandlerRef.visitInQtThreadIfAlive(
        [weakSelf, ohosWheelEvent, qWindowRef = m_qWindowRef](auto &eventHandler) {
            auto sharedSelf = weakSelf.lock();
            QWindow *qWindow = qWindowRef.data();
            if (!sharedSelf || qWindow == nullptr)
                return;

            eventHandler.onMouseWheelEvent(ohosWheelEvent, qWindow);
        });
}

}

QOhosConsumer<QOhosAxisEventType, ::ArkUI_UIInputEvent *> makeQOhosNativeAxisEventHandler(
    QtOhos::QThreadSafeRef<QWindow> qWindowRef,
    QtOhos::QThreadSafeRef<QOhosInputMethodEventHandler> imEventHandlerRef)
{
    auto handler = std::make_shared<QOhosAxisEventHandler>(qWindowRef, imEventHandlerRef);
    return [handler](QOhosAxisEventType eventType, ::ArkUI_UIInputEvent *inputEvent) {
        switch (eventType) {
        case QOhosAxisEventType::AxisEvent:
            handler->handleUiAxisEvent(inputEvent);
            break;
        case QOhosAxisEventType::CoastingAxisEvent:
            handler->handleUiCoastingAxisEvent(inputEvent);
            break;
        }
    };
}

QT_END_NAMESPACE
