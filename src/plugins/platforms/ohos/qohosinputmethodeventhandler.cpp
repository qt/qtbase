// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosinputmethodeventhandler.h"
#include "qohosinputcontext.h"
#include "qohosjsmain.h"
#include "qohosplatformscreen.h"
#include "qohosplatformtheme.h"
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qmap.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtMath>
#include <algorithm>
#include <arkui/ui_input_event.h>
#include <chrono>
#include <render/qohosview.h>
#include <typeinfo>

using namespace Qt::Literals::StringLiterals;

namespace ch = std::chrono;

QT_BEGIN_NAMESPACE

namespace {

constexpr double fingerAreaWidth = 50.0;
constexpr double fingerAreaHeight = 50.0;

QInputDevice *createTouchDevice(QInputDevice::DeviceType deviceType)
{
    qOhosDebug(QtForOhos) << "Creating touchDevice!";
    auto touchDevice =
        std::make_unique<QPointingDevice>("OHOS touch device"_L1, 1,
                                          deviceType, QPointingDevice::PointerType::Finger,
                                          QInputDevice::Capability::Position
                                            | QInputDevice::Capability::Area
                                            | QInputDevice::Capability::Pressure
                                            | QInputDevice::Capability::NormalizedPosition,
                                          10, 0);

    auto *touchDeviceRaw = touchDevice.get();
    QWindowSystemInterface::registerInputDevice(touchDevice.release());
    return touchDeviceRaw;
}

std::shared_ptr<void> registerObjectDestroyedSignalHandler(
    QObject *object, QObject *context, std::function<void()> signalHandler)
{
    auto objDestroyedConnection = QObject::connect(object, &QObject::destroyed, context, std::move(signalHandler));
    return QtOhos::makeDestroyNotifier(
        [objDestroyedConnection = std::move(objDestroyedConnection)] () mutable {
            QObject::disconnect(objDestroyedConnection);
        });
}

QOhosOptional<QEventPoint::State> tryMapXComponentTouchEventTypeToQt(::OH_NativeXComponent_TouchEventType eventType)
{
    switch (eventType) {
    case OH_NATIVEXCOMPONENT_DOWN:
        return makeQOhosOptional(QEventPoint::State::Pressed);
    case OH_NATIVEXCOMPONENT_UP:
        return makeQOhosOptional(QEventPoint::State::Released);
    case OH_NATIVEXCOMPONENT_MOVE:
        return makeQOhosOptional(QEventPoint::State::Updated);
    case OH_NATIVEXCOMPONENT_CANCEL:
    case OH_NATIVEXCOMPONENT_UNKNOWN:
        break;
    }
    return makeEmptyQOhosOptional();
}

QPointF calculateTouchPointNormalPosition(QWindow *targetWindow, const QPointF &clickPoint)
{
    auto *platformScreen = static_cast<QOhosPlatformScreen *>(targetWindow->screen()->handle());

    QSize screenSizeScaled = QHighDpi::fromNative(
        platformScreen->geometry().size(),
        platformScreen->pixelScalingCoefficient());

    QPointF clickPointNormalized(
        clickPoint.x() / screenSizeScaled.width(),
        clickPoint.y() / screenSizeScaled.height());

    return clickPointNormalized;
}

QRectF calculateTouchPointArea(const QPointF &clickPoint)
{
    return QRectF(
        clickPoint.x() - static_cast<double>(fingerAreaWidth/2),
        clickPoint.y() - static_cast<double>(fingerAreaHeight/2),
        fingerAreaWidth,
        fingerAreaHeight);
}

}

QOhosInputMethodEventHandler::QOhosInputMethodEventHandler(
    const std::set<QInputDevice::DeviceType> &deviceTypes)
{
    for (const auto &deviceType : deviceTypes)
        m_touchDevices.emplace(deviceType, createTouchDevice(deviceType));
}

QOhosInputMethodEventHandler::~QOhosInputMethodEventHandler() = default;

void QOhosInputMethodEventHandler::onTouchEventFromXComponent(
    QWindow *targetWindow, ch::nanoseconds timeStamp,
    const std::vector<QOhosTouchEventTouchPointData> &touchPoints,
    QInputDevice::DeviceType deviceType)
{
    auto *touchDevice = getTouchDeviceOrCreateIfNeeded(deviceType);

    QList<QWindowSystemInterface::TouchPoint> wsiTouchPoints;

    for (const auto &touchPointData : touchPoints) {
        const auto &touchPoint = touchPointData.touchPoint;
        QEventPoint::State state =
            tryMapXComponentTouchEventTypeToQt(touchPoint.type)
                .valueOr(QEventPoint::State::Stationary);

        QPointF clickPoint = touchPointData.displayPosition;

        QWindowSystemInterface::TouchPoint qwsiTouchPoint;
        qwsiTouchPoint.id = touchPoint.id;
        qwsiTouchPoint.pressure = touchPoint.force;
        qwsiTouchPoint.normalPosition =
            calculateTouchPointNormalPosition(targetWindow, clickPoint);
        qwsiTouchPoint.state = state;
        qwsiTouchPoint.area = calculateTouchPointArea(clickPoint);
        wsiTouchPoints.push_back(qwsiTouchPoint);
    }

    QWindowSystemInterfaceTouchEvent touchEvent = {
        .targetWindow = targetWindow,
        .touchPoints = wsiTouchPoints,
        .touchDevice = touchDevice,
        .timestampMs = ch::duration_cast<ch::milliseconds>(timeStamp),
    };

    handleTouchEvent(touchEvent);
}

void QOhosInputMethodEventHandler::onGestureEventFromNativeNode(const QOhosGestureEvent &gestureEvent)
{
    auto *touchDevice = getTouchDeviceOrCreateIfNeeded(gestureEvent.deviceType);
    auto localPosition = gestureEvent.localPosition;
    auto screenPosition = gestureEvent.screenPosition;

    QWindowSystemInterface::handleGestureEventWithRealValue(
        gestureEvent.targetWindow,
        gestureEvent.timestamp,
        static_cast<const QPointingDevice *>(touchDevice),
        gestureEvent.gestureType,
        gestureEvent.value,
        localPosition,
        screenPosition);
}

void QOhosInputMethodEventHandler::onKeyEvent(const QOhosKeyEvent &keyEvent, QWindow *targetWindow)
{
    const auto optQOhosQtKeyEvent = keyEvent.tryConvertToQOhosQtKeyEvent();
    if (!optQOhosQtKeyEvent.hasValue())
        return;
    const auto qOhosQtKeyEvent = optQOhosQtKeyEvent.value();

    constexpr quint32 nativeScanCode = 0;
    constexpr quint32 nativeModifiers = 0;

    auto *ohosInputContext = qobject_cast<QOhosInputContext *>(QOhosPlatformIntegration::instance()->inputContext());
    if (ohosInputContext != nullptr)
        ohosInputContext->setLastInputTypeToTriggerSoftKeyboard(QOhosInputContext::RequestKeyboardReason::NONE);

    if (qOhosQtKeyEvent.keyAction == QEvent::KeyPress) {
        if (m_autoRepeatCountMap[qOhosQtKeyEvent.keyCode] < std::numeric_limits<ushort>::max()) {
            ++m_autoRepeatCountMap[qOhosQtKeyEvent.keyCode];
        }
    } else {
        m_autoRepeatCountMap.remove(qOhosQtKeyEvent.keyCode);
    }
    const auto count = m_autoRepeatCountMap.value(qOhosQtKeyEvent.keyCode, 1);

    QWindowSystemInterface::handleExtendedKeyEvent(
        !m_currentKeyboardGrabbingWindow.isNull()
            ? m_currentKeyboardGrabbingWindow.data()
            : targetWindow,
        qOhosQtKeyEvent.keyAction, qOhosQtKeyEvent.keyCode,
        qOhosQtKeyEvent.guiApplicationKeyboardModifiers, nativeScanCode,
        qOhosQtKeyEvent.nativeKeyCode, nativeModifiers, qOhosQtKeyEvent.keyText, count > 1, count);
}

void QOhosInputMethodEventHandler::onMouseEvent(const QOhosMouseEvent &mouseEvent)
{
    Qt::MouseButton button = Qt::NoButton;

    if (mouseEvent.eventType == QEvent::MouseButtonPress || mouseEvent.eventType == QEvent::MouseButtonRelease) {
        button = mouseEvent.button;

        // HACK
        // Destructing QOhosView means loosing QNativeNode, JsStateData and QXComponentCallbackReceiver.
        // It means no more events will come from the destroyed window.
        // There is a case when closing window was done via mouse double-click event and second release
        // button event is not caught due to QNativeNode destruction. It causes issues in
        // QOhosInputMethodEventHandler state machine after switching to different window.
        // This workaround allows to clear buttons state.
        registerOnWindowCloseToResetMouseButtonsState(mouseEvent.targetWindow);
    }

    QOhosMouseEvent wsiEvent {
        .targetWindow = mouseEvent.targetWindow,
        .timestampMs = mouseEvent.timestampMs,
        .localPosition = mouseEvent.localPosition,
        .globalPosition = mouseEvent.globalPosition,
        .button = button,
        .eventType = mouseEvent.eventType,
        .modifiers = mouseEvent.modifiers,
    };

    handleMouseEvent(wsiEvent);
}

void QOhosInputMethodEventHandler::onHoverEvent(const QOhosHoverEvent &hoverEvent)
{
    bool isHover = hoverEvent.isHover;
    auto local = hoverEvent.localPosition;
    auto global = hoverEvent.globalPosition;
    QWindow *window = hoverEvent.targetWindow;
    if (!m_currentMouseGrabbingWindow.isNull() && m_currentMouseGrabbingWindow != window)
        return;

    if (isHover)
        QWindowSystemInterface::handleEnterEvent(window, local, global);
    else
        QWindowSystemInterface::handleLeaveEvent(window);
}

void QOhosInputMethodEventHandler::onMouseWheelEvent(const QOhosWheelEvent &event, QWindow *window)
{
    constexpr int angleXMin = -120;
    constexpr int angleXMax = 120;
    constexpr int xAxisValueMultiplier = -10;

    constexpr int angleYMin = angleXMin;
    constexpr int angleYMax = angleXMax;
    constexpr int yAxisValueMultiplier = xAxisValueMultiplier;

    constexpr double wheelStepDegree = 15.0;
    constexpr double wheelStepPixel = 21.0;

    constexpr double angleBaseValue = 8.0;
    constexpr double directionMultiplier = -1.0;

    QPoint pixelDelta;
    QPoint angleDelta;

    if (event.axisAction != UI_AXIS_EVENT_ACTION_UPDATE)
        return;

    static_cast<QOhosPlatformTheme *>(QGuiApplicationPrivate::platformTheme())->setWheelScrollLines(
        static_cast<int>(event.wheelScrollLines));

    auto *platformScreen = static_cast<QOhosPlatformScreen *>(window->screen()->handle());

    if (event.eventToolType == UI_INPUT_EVENT_TOOL_TYPE_MOUSE) {
        angleDelta.setX(qBound(angleXMin, static_cast<int>(event.horizontalValue * xAxisValueMultiplier), angleXMax));
        angleDelta.setY(qBound(angleYMin, static_cast<int>(event.verticalValue * yAxisValueMultiplier), angleYMax));

        auto mousePixelDeltaMultiplier = wheelStepPixel / wheelStepDegree / angleBaseValue;
        pixelDelta.setX(
            qRound(qFabs(angleDelta.x() * mousePixelDeltaMultiplier) * platformScreen->pixelScalingCoefficient()));
        pixelDelta.setY(
            qRound(qFabs(angleDelta.y() * mousePixelDeltaMultiplier) * platformScreen->pixelScalingCoefficient()));
    } else if (event.eventToolType == UI_INPUT_EVENT_TOOL_TYPE_TOUCHPAD) {
        auto touchpadAngleDeltaMultiplier =
            wheelStepDegree / wheelStepPixel / platformScreen->pixelScalingCoefficient() * angleBaseValue * directionMultiplier / event.wheelScrollLines;
        angleDelta.setX(qRound(event.horizontalValue * touchpadAngleDeltaMultiplier));
        angleDelta.setY(qRound(event.verticalValue * touchpadAngleDeltaMultiplier));

        pixelDelta.setX(qRound(qFabs(event.horizontalValue)));
        pixelDelta.setY(qRound(qFabs(event.verticalValue)));
    } else {
        qOhosWarning(QtForOhos)
            << Q_FUNC_INFO
            << "Received unsupported input event tool type =" << event.eventToolType << "skipping...";
        return;
    }

    Qt::MouseEventSource source = event.eventToolType == ::UI_INPUT_EVENT_TOOL_TYPE_TOUCHPAD
        ? Qt::MouseEventSynthesizedBySystem
        : Qt::MouseEventNotSynthesized;
    bool inverted = false;

    QWindowSystemInterface::handleWheelEvent(
        window,
        event.timestamp,
        event.localPoint,
        event.globalPoint,
        pixelDelta,
        angleDelta,
        convertOhosToQtKeyboardModifiers(event.modifiers),
        Qt::NoScrollPhase,
        source,
        inverted);
}

void QOhosInputMethodEventHandler::onNonClientAreaMouseEvents(
    QWindow *targetWindow, std::vector<QOhosWindowProxy::NonClientAreaMouseEvent> eventBatch)
{
    using NonClientAreaMouseEvent = QOhosWindowProxy::NonClientAreaMouseEvent;

    eventBatch.erase(
        QtOhos::removeMatchingWithLookahead(
            eventBatch.begin(), eventBatch.end(),
            [](const NonClientAreaMouseEvent &event, const NonClientAreaMouseEvent &nextEvent) {
                return event.action == QEvent::NonClientAreaMouseMove
                    && nextEvent.action == QEvent::NonClientAreaMouseMove;
            }),
        eventBatch.end());

    for (const auto &mouseEvent : eventBatch) {
        QOhosMouseEvent qtMouseEvent = {
            .targetWindow = targetWindow,
            .timestampMs = mouseEvent.timestamp,
            .localPosition = mouseEvent.localPosition,
            .globalPosition = mouseEvent.displayPosition,
            .button = mouseEvent.button,
            .eventType = mouseEvent.action,
        };

        // HACK
        // When a window is being closed by clicking close button on title bar we should receive two events:
        // NonClientAreaMouseButtonPress and NonClientAreaMouseButtonRelease, but sometimes we receive only
        // NonClientAreaMouseButtonPress event without NonClientAreaMouseButtonRelease, because the window
        // sending this event is alredy destroyed. It leaves us with invalid mouse state with a button
        // that has not been released.
        // This workaround resets mouse buttons state when a window is closed.
        if (mouseEvent.action == QEvent::NonClientAreaMouseButtonPress)
            registerOnWindowCloseToResetMouseButtonsState(targetWindow);

        handleMouseEvent(qtMouseEvent);
    }
}

void QOhosInputMethodEventHandler::onNonClientAreaTouchEvents(
    QWindow *targetWindow, std::vector<QOhosWindowProxy::NonClientAreaTouchEvent> eventBatch)
{
    using NonClientAreaTouchEvent = QOhosWindowProxy::NonClientAreaTouchEvent;

    eventBatch.erase(
        QtOhos::removeMatchingWithLookahead(
            eventBatch.begin(), eventBatch.end(),
            [](const NonClientAreaTouchEvent &event, const NonClientAreaTouchEvent &nextEvent) {
                return
                    event.state == QEventPoint::State::Updated
                    && nextEvent.state == QEventPoint::State::Updated;
            }),
        eventBatch.end());

    for (const auto &touchEvent : eventBatch) {
        QPointF clickPoint = touchEvent.displayPosition;

        QWindowSystemInterface::TouchPoint qwsiTouchPoint;
        qwsiTouchPoint.id = touchEvent.id;
        qwsiTouchPoint.pressure = 1.0;
        qwsiTouchPoint.normalPosition = calculateTouchPointNormalPosition(targetWindow, clickPoint);
        qwsiTouchPoint.state = touchEvent.state;
        qwsiTouchPoint.area = calculateTouchPointArea(clickPoint);

        QWindowSystemInterfaceTouchEvent qwsiTouchEvent = {
            .targetWindow = targetWindow,
            .touchPoints = {qwsiTouchPoint},
            .touchDevice = getTouchDeviceOrCreateIfNeeded(QInputDevice::DeviceType::TouchScreen),
            .timestampMs = touchEvent.timestamp,
        };

        handleTouchEvent(qwsiTouchEvent);
    }
}

QWindow *QOhosInputMethodEventHandler::lastTouchedWindowOrNull() const
{
    auto lastTouchedPair = getLastTouchedWindowWithSeqNoIfPresent();
    return lastTouchedPair.hasValue()
        ? lastTouchedPair.value().first
        : nullptr;
}

QInputDevice *QOhosInputMethodEventHandler::getTouchDeviceOrCreateIfNeeded(QInputDevice::DeviceType deviceType)
{
    auto touchDeviceIter = m_touchDevices.find(deviceType);
    if (touchDeviceIter == m_touchDevices.end()) {
        qOhosWarning(QtForOhos) << "Trying to get touch device but it isn't registered. Creating and registering one now.";
        std::tie(touchDeviceIter, std::ignore) = m_touchDevices.emplace(
            deviceType, createTouchDevice(deviceType));
    }
    return touchDeviceIter->second;
}

QOhosOptional<std::pair<QWindow *, std::uint64_t>> QOhosInputMethodEventHandler::getLastTouchedWindowWithSeqNoIfPresent() const
{
    auto maxSeqNoEntryIter = std::max_element(
        m_windowsUnderTouchPoints.begin(), m_windowsUnderTouchPoints.end(),
        [](const auto &a, const auto &b) {
            return a.second.second < b.second.second;
        });

    return maxSeqNoEntryIter != m_windowsUnderTouchPoints.end()
        ? makeQOhosOptional(
            std::make_pair(maxSeqNoEntryIter->first, maxSeqNoEntryIter->second.second))
        : makeEmptyQOhosOptional();
}

void QOhosInputMethodEventHandler::grabMouse(QWindow *window)
{
    qCDebug(QtForOhos) << Q_FUNC_INFO << "window:" << window;
    m_currentMouseGrabbingWindow = window;
}

void QOhosInputMethodEventHandler::grabKeyboard(QWindow *window)
{
    qCDebug(QtForOhos) << Q_FUNC_INFO << "window:" << window;
    m_currentKeyboardGrabbingWindow = window;
}

void QOhosInputMethodEventHandler::stopAnyMouseGrab()
{
    if (!m_currentMouseGrabbingWindow.isNull() && m_lastWsiMouseEvent.hasValue()) {
        auto lastWsiMouseEventValue = m_lastWsiMouseEvent.value();
        auto *previousCaptureWindow = m_currentMouseGrabbingWindow.data();
        auto *optLastWindowUnderCursor = lastWsiMouseEventValue.targetWindow.data();
        auto *optCurrentWindowUnderCursor = qGuiApp->topLevelAt(
            QHighDpi::fromNativePixels(
                lastWsiMouseEventValue.globalPosition.toPoint(),
                lastWsiMouseEventValue.targetWindow.data()));

        if (optLastWindowUnderCursor != nullptr
            && optCurrentWindowUnderCursor != nullptr
            && optLastWindowUnderCursor != previousCaptureWindow) {
            QWindowSystemInterface::handleEnterEvent(
                optLastWindowUnderCursor, lastWsiMouseEventValue.localPosition,
                lastWsiMouseEventValue.globalPosition);
        }
    }
    m_currentMouseGrabbingWindow.clear();
}

void QOhosInputMethodEventHandler::stopAnyKeyboardGrab()
{
    m_currentKeyboardGrabbingWindow.clear();
}

void QOhosInputMethodEventHandler::handleMouseEvent(const QOhosMouseEvent &wsiEvent)
{
    static const QSet<QEvent::Type> mouseButtonPressEventTypes = {
        QEvent::MouseButtonPress,
        QEvent::NonClientAreaMouseButtonPress,
    };
    static const QSet<QEvent::Type> mouseButtonReleaseEventTypes = {
        QEvent::MouseButtonRelease,
        QEvent::NonClientAreaMouseButtonRelease,
    };

    bool eventTypeIsPress = mouseButtonPressEventTypes.contains(wsiEvent.eventType);
    bool eventTypeIsRelease = mouseButtonReleaseEventTypes.contains(wsiEvent.eventType);

    if (eventTypeIsPress || eventTypeIsRelease) {
        m_mouseButtonsState.setFlag(wsiEvent.button, eventTypeIsPress);
        auto *ohosInputContext = qobject_cast<QOhosInputContext *>(QOhosPlatformIntegration::instance()->inputContext());
        if (ohosInputContext != nullptr)
            ohosInputContext->setLastInputTypeToTriggerSoftKeyboard(QOhosInputContext::RequestKeyboardReason::MOUSE);
    }

    QEvent::Type targetEventType;
    QWindow *targetWindow;
    QPointF localPosition;
    if (!m_currentMouseGrabbingWindow.isNull()) {
        targetWindow = m_currentMouseGrabbingWindow;
        localPosition =
            QHighDpi::toNativeLocalPosition(
                targetWindow->mapFromGlobal(
                    wsiEvent.targetWindow->mapToGlobal(
                        QHighDpi::fromNativePixels(
                            wsiEvent.localPosition.toPoint(), wsiEvent.targetWindow.data()))),
                targetWindow);
        switch (wsiEvent.eventType) {
        case QEvent::NonClientAreaMouseButtonRelease:
            targetEventType = QEvent::MouseButtonRelease;
            break;
        case QEvent::NonClientAreaMouseButtonPress:
            targetEventType = QEvent::MouseButtonPress;
            break;
        case QEvent::NonClientAreaMouseMove:
            targetEventType = QEvent::MouseMove;
            break;
        default:
            targetEventType = wsiEvent.eventType;
            break;
        }
    } else {
        targetWindow = wsiEvent.targetWindow;
        localPosition = wsiEvent.localPosition;
        targetEventType = wsiEvent.eventType;
    }

    m_lastWsiMouseEvent = wsiEvent;

    if (targetEventType == QEvent::None) {
        qOhosPrintfDebug("%s: targetEventType is QEvent::None!", Q_FUNC_INFO);
        return;
    }

    QWindowSystemInterface::handleMouseEvent(
        targetWindow,
        wsiEvent.timestampMs.count(),
        localPosition,
        wsiEvent.globalPosition,
        m_mouseButtonsState,
        wsiEvent.button,
        targetEventType,
        convertOhosToQtKeyboardModifiers(wsiEvent.modifiers));
}

void QOhosInputMethodEventHandler::handleTouchEvent(const QWindowSystemInterfaceTouchEvent &touchEvent)
{
    if (touchEvent.touchPoints.isEmpty()) {
        qOhosCritical(QtForOhos) << "TouchPoints list is empty, nothing to do";
        return;
    }

    m_lastWsiMouseEvent.reset();

    bool anyEventPressedOrReleased =
        std::any_of(
            touchEvent.touchPoints.begin(),
            touchEvent.touchPoints.end(),
            [](const QWindowSystemInterface::TouchPoint &touchPoint) {
                return touchPoint.state == QEventPoint::State::Pressed || touchPoint.state == QEventPoint::State::Released;
            });
    if (anyEventPressedOrReleased) {
        auto *ohosInputContext = qobject_cast<QOhosInputContext *>(QOhosPlatformIntegration::instance()->inputContext());
        if (ohosInputContext != nullptr)
            ohosInputContext->setLastInputTypeToTriggerSoftKeyboard(QOhosInputContext::RequestKeyboardReason::TOUCH);
    }

    updateWindowsUnderTouchPoints(touchEvent);

    QWindowSystemInterface::handleTouchEvent(
        touchEvent.targetWindow, touchEvent.timestampMs.count(),
        static_cast<const QPointingDevice *>(touchEvent.touchDevice), touchEvent.touchPoints);
}

void QOhosInputMethodEventHandler::updateWindowsUnderTouchPoints(const QWindowSystemInterfaceTouchEvent &touchEvent)
{
    const auto &touchPoints = touchEvent.touchPoints;
    auto *targetWindow = touchEvent.targetWindow;

    const bool allTouchPointsUp = std::all_of(
        touchPoints.begin(), touchPoints.end(),
        [](const auto &touchPointData) {
            return touchPointData.state == QEventPoint::State::Released;
        });

    const bool anyTouchPointDown = std::any_of(
        touchPoints.begin(), touchPoints.end(),
        [](const auto &touchPointData) {
            return touchPointData.state == QEventPoint::State::Pressed;
        });

    if (allTouchPointsUp) {
        std::ignore = m_windowsUnderTouchPoints.erase(targetWindow);
    } else if (anyTouchPointDown) {
        auto lastTouchedPair = getLastTouchedWindowWithSeqNoIfPresent();
        auto nextSeqNo = lastTouchedPair.hasValue()
            ? lastTouchedPair.value().second + 1
            : 0;
        m_windowsUnderTouchPoints[targetWindow] = std::make_pair(
            registerObjectDestroyedSignalHandler(
                targetWindow, this,
                [this, targetWindow]() {
                    std::ignore = m_windowsUnderTouchPoints.erase(targetWindow);
                }),
            nextSeqNo);
    }
}

void QOhosInputMethodEventHandler::registerOnWindowCloseToResetMouseButtonsState(QWindow *window)
{
    auto *eventView = QOhosPlatformWindow::fromQWindow(window)->ownedViewOrNull();
    if (eventView != nullptr) {
        m_lastMouseEventViewLifetimeTrackerHandle = registerObjectDestroyedSignalHandler(
            eventView, this,
            [this]() {
                m_mouseButtonsState = Qt::NoButton;
            });
    }
}

QT_END_NAMESPACE
