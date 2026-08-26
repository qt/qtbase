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
#include <QtGui/private/qwindow_p.h>
#include <QtCore/qmath.h>
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

QInputDevice *registerPointingDevice(std::unique_ptr<QPointingDevice> device)
{
    auto *deviceRaw = device.get();
    QWindowSystemInterface::registerInputDevice(device.release());
    return deviceRaw;
}

QInputDevice *createPointingDevice(QInputDevice::DeviceType deviceType)
{
    qOhosDebug(QtForOhos) << "Creating pointing device! type:" << deviceType;

    // QInputDevice::systemId() is expected to be unique per device; reusing
    // the DeviceType flag value keeps that true across the devices created
    // here without needing a separate ID scheme.
    const qint64 systemId = static_cast<qint64>(deviceType);

    if (deviceType == QInputDevice::DeviceType::Mouse) {
        return registerPointingDevice(std::make_unique<QPointingDevice>(
            "OHOS mouse device"_L1, systemId, deviceType, QPointingDevice::PointerType::Generic,
            QInputDevice::Capability::Position, 1, 3));
    }

    return registerPointingDevice(std::make_unique<QPointingDevice>(
        "OHOS touch device"_L1, systemId, deviceType, QPointingDevice::PointerType::Finger,
        QInputDevice::Capability::Position
            | QInputDevice::Capability::Area
            | QInputDevice::Capability::Pressure
            | QInputDevice::Capability::NormalizedPosition,
        10, 0));
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

std::optional<QEventPoint::State> tryMapXComponentTouchEventTypeToQt(::OH_NativeXComponent_TouchEventType eventType)
{
    switch (eventType) {
    case OH_NATIVEXCOMPONENT_DOWN:
        return QEventPoint::State::Pressed;
    case OH_NATIVEXCOMPONENT_UP:
        return QEventPoint::State::Released;
    case OH_NATIVEXCOMPONENT_MOVE:
        return QEventPoint::State::Updated;
    case OH_NATIVEXCOMPONENT_CANCEL:
    case OH_NATIVEXCOMPONENT_UNKNOWN:
        break;
    }
    return {};
}

QPointF calculateTouchPointNormalPosition(QWindow *targetWindow, const QPointF &clickPoint)
{
    auto *platformScreen = static_cast<QOhosPlatformScreen *>(targetWindow->screen()->handle());

    QSize screenSize = platformScreen->geometry().size();

    QPointF clickPointNormalized(
        clickPoint.x() / screenSize.width(),
        clickPoint.y() / screenSize.height());

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

QPoint determineScreenGlobalDisplayOffset(QWindow *qWindow)
{
    auto *screen = qWindow != nullptr
        ? qWindow->screen()
        : QGuiApplication::primaryScreen();

    return screen != nullptr
        ? screen->handle()->geometry().topLeft()
        : QPoint();
}

QPoint makeWindowLocalPosition(const QPoint &globalPosition, QWindow *qWindow)
{
    auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(qWindow);
    auto platformWindowGeometry = platformWindow != nullptr
        ? platformWindow->geometry()
        : QHighDpi::toNativePixels(qWindow->geometry(), qWindow);

    return globalPosition - platformWindowGeometry.topLeft();
}

// Whether a press on qWindow should make it the focus window. Based on the
// check in QXcbWindow::handleButtonPressEvent(), with the window types and
// states that OHOS must also skip.
bool windowShouldTakeFocusOnPress(QWindow *qWindow)
{
    if (qWindow == nullptr || qWindow == QGuiApplication::focusWindow())
        return false;

    if (QGuiApplicationPrivate::activePopupWindow() != nullptr)
        return false;

    if (QOhosPlatformWindow::fromQWindowOrNull(qWindow) == nullptr)
        return false;

    const auto flags = QOhosPlatformWindow::platformWindowFlagsForQWindow(qWindow);
    if (flags.testFlag(Qt::WindowDoesNotAcceptFocus)
        || flags.testFlag(Qt::WindowTransparentForInput)
        || flags.testFlag(Qt::BypassWindowManagerHint)) {
        return false;
    }

    switch (qWindow->type()) {
    case Qt::ToolTip:
    case Qt::Popup:
    case Qt::SplashScreen:
        return false;
    default:
        break;
    }

    return !QGuiApplicationPrivate::instance()->isWindowBlocked(qWindow);
}

}

QOhosInputMethodEventHandler::QOhosInputMethodEventHandler(
    const std::set<QInputDevice::DeviceType> &deviceTypes)
{
    for (const auto &deviceType : deviceTypes)
        m_pointingDevices.emplace(deviceType, createPointingDevice(deviceType));

    // Activation is asynchronous, so a request is latched until focus changes.
    connect(qGuiApp, &QGuiApplication::focusWindowChanged,
            this, [this](QWindow *) { m_pendingActivationWindow.clear(); });
}

void QOhosInputMethodEventHandler::requestActivationOnPressIfNeeded(QWindow *pressTargetWindow)
{
    if (!m_currentMouseGrabbingWindow.isNull())
        return;

    if (pressTargetWindow == nullptr)
        return;

    // Activate the event receiver, not the hit window: a native child
    // QWidgetWindow must not become the focus window on its own.
    auto *targetWindow =
        static_cast<QWindowPrivate *>(QObjectPrivate::get(pressTargetWindow))->eventReceiver();

    if (!windowShouldTakeFocusOnPress(targetWindow))
        return;

    auto *currentFocusWindow = QGuiApplication::focusWindow();
    const bool embeddedWindowInvolved =
        QOhosPlatformWindow::isEmbeddedWindow(targetWindow)
        || (currentFocusWindow != nullptr
            && QOhosPlatformWindow::isEmbeddedWindow(currentFocusWindow));
    if (!embeddedWindowInvolved)
        return;

    if (m_pendingActivationWindow == targetWindow)
        return;

    m_pendingActivationWindow = targetWindow;
    targetWindow->requestActivate();
}

QOhosInputMethodEventHandler::~QOhosInputMethodEventHandler() = default;

void QOhosInputMethodEventHandler::onTouchEventFromXComponent(
    QWindow *targetWindow, ch::nanoseconds timeStamp,
    const std::vector<QOhosTouchEventTouchPointData> &touchPoints,
    QInputDevice::DeviceType deviceType, QFlags<OhosKeyboardModifier> modifiers)
{
    auto *touchDevice = getPointingDeviceOrCreate(deviceType);

    QList<QWindowSystemInterface::TouchPoint> wsiTouchPoints;

    auto timeStampMs = ch::duration_cast<ch::milliseconds>(timeStamp);

    std::vector<QPoint> activeTouchPointDisplayPositions;

    auto displayOffset = targetWindow != nullptr
        ? determineScreenGlobalDisplayOffset(targetWindow)
        : QPoint(0, 0);

    for (const auto &touchPointData : touchPoints) {
        const auto &touchPoint = touchPointData.touchPoint;
        QPointF clickPoint = touchPointData.displayPosition;

        switch (touchPointData.toolType) {
        case ::OH_NATIVEXCOMPONENT_TOOL_TYPE_FINGER: {
            QEventPoint::State state =
                tryMapXComponentTouchEventTypeToQt(touchPoint.type)
                    .value_or(QEventPoint::State::Stationary);

            if (state != QEventPoint::State::Released)
                activeTouchPointDisplayPositions.push_back(touchPointData.displayPosition.toPoint());

            QWindowSystemInterface::TouchPoint qwsiTouchPoint;
            qwsiTouchPoint.id = touchPoint.id;
            qwsiTouchPoint.pressure = touchPoint.force;
            qwsiTouchPoint.normalPosition =
                calculateTouchPointNormalPosition(targetWindow, clickPoint);
            qwsiTouchPoint.state = state;
            qwsiTouchPoint.area = calculateTouchPointArea(clickPoint + displayOffset);
            wsiTouchPoints.push_back(qwsiTouchPoint);
            break;
        }
        case ::OH_NATIVEXCOMPONENT_TOOL_TYPE_PEN: {
            Qt::MouseButtons buttons = Qt::NoButton;
            switch (touchPoint.type) {
            case OH_NATIVEXCOMPONENT_DOWN:
                requestActivationOnPressIfNeeded(targetWindow);
                buttons = Qt::LeftButton;
                break;
            case OH_NATIVEXCOMPONENT_MOVE:
                buttons = Qt::LeftButton;
                break;
            case OH_NATIVEXCOMPONENT_UP:
            case OH_NATIVEXCOMPONENT_CANCEL:
            case OH_NATIVEXCOMPONENT_UNKNOWN:
                buttons = Qt::NoButton;
                break;
            }
            constexpr float tiltDegreesMin = -60.0f;
            constexpr float tiltDegreesMax = 60.0f;
            const int xTilt = qRound(qBound(tiltDegreesMin, touchPointData.tiltX, tiltDegreesMax));
            const int yTilt = qRound(qBound(tiltDegreesMin, touchPointData.tiltY, tiltDegreesMax));
            constexpr qreal tangentialPressure = 0;
            constexpr qreal rotation = 0;
            constexpr int z = 0;
            QWindowSystemInterface::handleTabletEvent(
                targetWindow, timeStampMs.count(), {touchPoint.x, touchPoint.y},
                clickPoint,
                static_cast<int>(QInputDevice::DeviceType::Stylus),
                static_cast<int>(QPointingDevice::PointerType::Pen),
                buttons, touchPoint.force, xTilt, yTilt, tangentialPressure, rotation,
                z, touchPoint.id, convertOhosToQtKeyboardModifiers(modifiers));
            break;
        }
        case ::OH_NATIVEXCOMPONENT_TOOL_TYPE_RUBBER:
        case ::OH_NATIVEXCOMPONENT_TOOL_TYPE_BRUSH:
        case ::OH_NATIVEXCOMPONENT_TOOL_TYPE_PENCIL:
        case ::OH_NATIVEXCOMPONENT_TOOL_TYPE_AIRBRUSH:
        case ::OH_NATIVEXCOMPONENT_TOOL_TYPE_LENS:
        case ::OH_NATIVEXCOMPONENT_TOOL_TYPE_UNKNOWN:
            qOhosWarning(QtForOhos) << "Skipping unsupported tool type =" << touchPointData.toolType;
            break;
        case ::OH_NATIVEXCOMPONENT_TOOL_TYPE_MOUSE:
            qOhosWarning(QtForOhos) << "Skipping mouse tool type in touch event.";
            break;
        }
    }

    if (!wsiTouchPoints.isEmpty()) {
        auto singleActiveTouchEventGlobalPosition = activeTouchPointDisplayPositions.size() == 1
            ? std::optional(displayOffset + activeTouchPointDisplayPositions.front())
            : std::nullopt;

        QWindowSystemInterfaceTouchEvent touchEvent = {
            .targetWindow = targetWindow,
            .touchPoints = wsiTouchPoints,
            .touchDevice = touchDevice,
            .timestampMs = timeStampMs,
            .modifiers = modifiers,
            .singleTouchPointEventGlobalPosition = singleActiveTouchEventGlobalPosition,
        };

        handleTouchEvent(touchEvent);
    }
}

void QOhosInputMethodEventHandler::onGestureEventFromNativeNode(const QOhosGestureEvent &gestureEvent)
{
    auto *pointingDevice = getPointingDeviceOrCreate(gestureEvent.deviceType);

    // handleGestureEventWithRealValue() takes native positions and applies QHighDpi internally.
    QWindowSystemInterface::handleGestureEventWithRealValue(
        gestureEvent.targetWindow,
        gestureEvent.timestamp,
        static_cast<const QPointingDevice *>(pointingDevice),
        gestureEvent.gestureType,
        gestureEvent.value,
        gestureEvent.localPosition,
        gestureEvent.globalPosition);
}

void QOhosInputMethodEventHandler::onKeyEvent(const QOhosKeyEvent &keyEvent, QWindow *targetWindow)
{
    const auto optQOhosQtKeyEvent = keyEvent.tryConvertToQOhosQtKeyEvent();
    if (!optQOhosQtKeyEvent.has_value())
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
        .deviceType = mouseEvent.deviceType,
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

    static_cast<QOhosPlatformTheme *>(QGuiApplicationPrivate::platformTheme())->setWheelScrollLines(
        static_cast<int>(event.wheelScrollLines));

    auto *platformScreen = static_cast<QOhosPlatformScreen *>(window->screen()->handle());

    if (event.eventToolType == UI_INPUT_EVENT_TOOL_TYPE_MOUSE) {
        angleDelta.setX(qBound(angleXMin, static_cast<int>(event.horizontalValue * xAxisValueMultiplier), angleXMax));
        angleDelta.setY(qBound(angleYMin, static_cast<int>(event.verticalValue * yAxisValueMultiplier), angleYMax));

        auto mousePixelDeltaMultiplier = wheelStepPixel / wheelStepDegree / angleBaseValue;
        pixelDelta.setX(
            qRound(angleDelta.x() * mousePixelDeltaMultiplier * platformScreen->pixelScalingCoefficient()));
        pixelDelta.setY(
            qRound(angleDelta.y() * mousePixelDeltaMultiplier * platformScreen->pixelScalingCoefficient()));
    } else if (event.eventToolType == UI_INPUT_EVENT_TOOL_TYPE_TOUCHPAD) {
        auto touchpadAngleDeltaMultiplier =
            wheelStepDegree / wheelStepPixel / platformScreen->pixelScalingCoefficient() * angleBaseValue * directionMultiplier / event.wheelScrollLines;
        angleDelta.setX(qRound(event.horizontalValue * touchpadAngleDeltaMultiplier));
        angleDelta.setY(qRound(event.verticalValue * touchpadAngleDeltaMultiplier));

        pixelDelta.setX(qRound(event.horizontalValue * directionMultiplier));
        pixelDelta.setY(qRound(event.verticalValue * directionMultiplier));
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

    const QInputDevice::DeviceType wheelDeviceType = event.eventToolType == UI_INPUT_EVENT_TOOL_TYPE_MOUSE
        ? QInputDevice::DeviceType::Mouse
        : QInputDevice::DeviceType::TouchPad;
    const auto *wheelDevice =
        static_cast<const QPointingDevice *>(getPointingDeviceOrCreate(wheelDeviceType));

    QWindowSystemInterface::handleWheelEvent(
        window,
        event.timestamp,
        wheelDevice,
        event.localPoint,
        event.globalPoint,
        pixelDelta,
        angleDelta,
        convertOhosToQtKeyboardModifiers(event.modifiers),
        event.scrollPhase,
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
        // Input_MouseEvent (unlike ArkUI_UIInputEvent) exposes no source type, so
        // we cannot tell a real mouse from a touchpad here. TouchPad is the safe
        // guess: if getPointingDeviceOrCreate() has to lazily register a device
        // for it, that doesn't change what QPointingDevice::primaryPointingDevice()
        // resolves to elsewhere, since TouchPad is already its fallback in the
        // absence of a real Mouse device. Guessing Mouse instead could register a
        // phantom Mouse device and make primaryPointingDevice() prefer it process-wide.
        QOhosMouseEvent qtMouseEvent = {
            .targetWindow = targetWindow,
            .timestampMs = mouseEvent.timestamp,
            .localPosition = mouseEvent.localPosition,
            .globalPosition = mouseEvent.displayPosition,
            .button = mouseEvent.button,
            .eventType = mouseEvent.action,
            .modifiers = {},
            .deviceType = QInputDevice::DeviceType::TouchPad,
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
                return event.action == QEvent::NonClientAreaMouseMove
                    && nextEvent.action == QEvent::NonClientAreaMouseMove;
            }),
        eventBatch.end());

    for (const auto &touchEvent : eventBatch) {
        if (touchEvent.action == QEvent::NonClientAreaMouseButtonPress)
            m_optNonClientAreaTouchPointerId = touchEvent.id;
        else if (m_optNonClientAreaTouchPointerId != touchEvent.id)
            continue;

        if (touchEvent.action == QEvent::NonClientAreaMouseButtonRelease)
            m_optNonClientAreaTouchPointerId.reset();

        QOhosMouseEvent qtMouseEvent = {
            .targetWindow = targetWindow,
            .timestampMs = touchEvent.timestamp,
            .localPosition = touchEvent.localPosition,
            .globalPosition = touchEvent.displayPosition,
            .button = Qt::LeftButton,
            .eventType = touchEvent.action,
            .modifiers = {},
            .deviceType = QInputDevice::DeviceType::TouchScreen,
        };

        if (touchEvent.action == QEvent::NonClientAreaMouseButtonPress)
            registerOnWindowCloseToResetMouseButtonsState(targetWindow);

        handleMouseEvent(qtMouseEvent);
    }
}

QWindow *QOhosInputMethodEventHandler::lastTouchedWindowOrNull() const
{
    auto lastTouchedPair = getLastTouchedWindowWithSeqNoIfPresent();
    return lastTouchedPair.has_value()
        ? lastTouchedPair.value().first
        : nullptr;
}

QInputDevice *QOhosInputMethodEventHandler::getPointingDeviceOrCreate(QInputDevice::DeviceType deviceType)
{
    auto pointingDeviceIter = m_pointingDevices.find(deviceType);
    if (pointingDeviceIter == m_pointingDevices.end()) {
        qOhosWarning(QtForOhos) << "Trying to get pointing device but it isn't registered. Creating and registering one now.";
        std::tie(pointingDeviceIter, std::ignore) = m_pointingDevices.emplace(
            deviceType, createPointingDevice(deviceType));
    }
    return pointingDeviceIter->second;
}

std::optional<std::pair<QWindow *, std::uint64_t>> QOhosInputMethodEventHandler::getLastTouchedWindowWithSeqNoIfPresent() const
{
    auto maxSeqNoEntryIter = std::max_element(
        m_windowsUnderTouchPoints.begin(), m_windowsUnderTouchPoints.end(),
        [](const auto &a, const auto &b) {
            return a.second.second < b.second.second;
        });

    return maxSeqNoEntryIter != m_windowsUnderTouchPoints.end()
        ? std::optional(
            std::make_pair(maxSeqNoEntryIter->first, maxSeqNoEntryIter->second.second))
        : std::nullopt;
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
    if (!m_currentMouseGrabbingWindow.isNull() && m_lastWsiMouseEvent.has_value()) {
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

QPoint QOhosInputMethodEventHandler::cursorPosition() const
{
    if (m_lastWsiMouseEvent.has_value())
        return m_lastWsiMouseEvent.value().globalPosition.toPoint();

    auto optLastTouchPosition = qAndThen(
        m_lastWsiTouchEvent,
        [](const QWindowSystemInterfaceTouchEvent &touchEvent) {
            return touchEvent.singleTouchPointEventGlobalPosition;
        });
    if (optLastTouchPosition.has_value())
        return optLastTouchPosition.value();

    auto lastScaledPositionFromApp = QGuiApplicationPrivate::lastCursorPosition.toPoint();
    auto *screen = qGuiApp->screenAt(lastScaledPositionFromApp);
    return QHighDpi::toNativePixels(
        lastScaledPositionFromApp,
        screen != nullptr
            ? screen
            : QGuiApplication::primaryScreen());
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

    if (eventTypeIsPress)
        requestActivationOnPressIfNeeded(wsiEvent.targetWindow.data());

    QEvent::Type targetEventType;
    QWindow *targetWindow;
    QPointF localPosition;
    if (!m_currentMouseGrabbingWindow.isNull()) {
        targetWindow = m_currentMouseGrabbingWindow;
        localPosition = makeWindowLocalPosition(wsiEvent.globalPosition.toPoint(), targetWindow);
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

    const auto *mouseDevice =
        static_cast<const QPointingDevice *>(getPointingDeviceOrCreate(wsiEvent.deviceType));

    QWindowSystemInterface::handleMouseEvent(
        targetWindow,
        wsiEvent.timestampMs.count(),
        mouseDevice,
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

    bool anyTouchPointPressed = false;
    bool anyTouchPointReleased = false;
    for (const auto &touchPoint : touchEvent.touchPoints) {
        anyTouchPointPressed = anyTouchPointPressed || touchPoint.state == QEventPoint::State::Pressed;
        anyTouchPointReleased = anyTouchPointReleased || touchPoint.state == QEventPoint::State::Released;
    }

    if (anyTouchPointPressed || anyTouchPointReleased) {
        auto *ohosInputContext = qobject_cast<QOhosInputContext *>(QOhosPlatformIntegration::instance()->inputContext());
        if (ohosInputContext != nullptr)
            ohosInputContext->setLastInputTypeToTriggerSoftKeyboard(QOhosInputContext::RequestKeyboardReason::TOUCH);
    }

    if (anyTouchPointPressed)
        requestActivationOnPressIfNeeded(touchEvent.targetWindow);

    updateWindowsUnderTouchPoints(touchEvent);

    m_lastWsiTouchEvent = touchEvent;
    QWindowSystemInterface::handleTouchEvent(
        touchEvent.targetWindow, touchEvent.timestampMs.count(),
        static_cast<const QPointingDevice *>(touchEvent.touchDevice), touchEvent.touchPoints,
        convertOhosToQtKeyboardModifiers(touchEvent.modifiers));
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
        auto nextSeqNo = lastTouchedPair.has_value()
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
