// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSINPUTMETHODEVENTHANDLER_H
#define QOHOSINPUTMETHODEVENTHANDLER_H

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <qohoskeyevent.h>
#include <qohoskeymodifiers.h>
#include <render/qohoswindowproxy.h>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "qohosplatformintegration.h"

QT_BEGIN_NAMESPACE

class QOhosNativeXComponent;

struct QOhosTouchEventTouchPointData
{
    ::OH_NativeXComponent_TouchPoint touchPoint;
    ::OH_NativeXComponent_TouchPointToolType toolType = ::OH_NATIVEXCOMPONENT_TOOL_TYPE_UNKNOWN;
    QPointF displayPosition;
    float tiltX = 0.0f;
    float tiltY = 0.0f;
};

struct QOhosWheelEvent
{
    int64_t timestamp;
    QPointF localPoint;
    QPointF globalPoint;
    double horizontalValue;
    double verticalValue;
    int32_t eventToolType;
    Qt::ScrollPhase scrollPhase;
    int32_t wheelScrollLines;
    QFlags<OhosKeyboardModifier> modifiers;
};

struct QOhosMouseEvent
{
    QPointer<QWindow> targetWindow;
    std::chrono::milliseconds timestampMs;
    QPointF localPosition;
    QPointF globalPosition;
    Qt::MouseButton button;
    QEvent::Type eventType;
    QFlags<OhosKeyboardModifier> modifiers;
    QInputDevice::DeviceType deviceType = QInputDevice::DeviceType::Mouse;
};

struct QOhosHoverEvent
{
    QPointer<QWindow> targetWindow;
    QPointF localPosition;
    QPointF globalPosition;
    bool isHover = false;
};

struct QOhosGestureEvent
{
    QPointer<QWindow> targetWindow;
    std::int64_t timestamp;
    qreal value;
    QPointF localPosition;
    QPointF globalPosition;
    Qt::NativeGestureType gestureType;
    QInputDevice::DeviceType deviceType;
};

class QOhosInputMethodEventHandler : public QObject
{
    Q_OBJECT

public:
    QOhosInputMethodEventHandler(const std::set<QInputDevice::DeviceType> &deviceTypes);
    ~QOhosInputMethodEventHandler();

    void onTouchEventFromXComponent(
        QWindow *targetWindow, std::chrono::nanoseconds timeStamp,
        const std::vector<QOhosTouchEventTouchPointData> &touchPoints,
        QInputDevice::DeviceType deviceType, QFlags<OhosKeyboardModifier> modifiers);
    void onTouchEventFromJsWindow(
        QWindow *optTargetWindow,
        const QList<QWindowSystemInterface::TouchPoint> &touchPoints);
    void onGestureEventFromNativeNode(const QOhosGestureEvent &gestureEvent);

    void onKeyEvent(const QOhosKeyEvent &keyEvent, QWindow *targetWindow);
    void onMouseEvent(const QOhosMouseEvent &mouseEvent);
    void onHoverEvent(const QOhosHoverEvent &hoverEvent);
    void onMouseWheelEvent(const QOhosWheelEvent &event, QWindow *window);
    void onNonClientAreaMouseEvents(QWindow *targetWindow, std::vector<QOhosWindowProxy::NonClientAreaMouseEvent> eventBatch);
    void onNonClientAreaTouchEvents(QWindow *targetWindow, std::vector<QOhosWindowProxy::NonClientAreaTouchEvent> eventBatch);

    QWindow *lastTouchedWindowOrNull() const;

    void grabMouse(QWindow *window);
    void stopAnyMouseGrab();
    void grabKeyboard(QWindow *window);
    void stopAnyKeyboardGrab();

    QPoint cursorPosition() const;

private:
    struct QWindowSystemInterfaceTouchEvent
    {
        QWindow *targetWindow;
        QList<QWindowSystemInterface::TouchPoint> touchPoints;
        QInputDevice *touchDevice;
        std::chrono::milliseconds timestampMs;
        QFlags<OhosKeyboardModifier> modifiers;
        std::optional<QPoint> singleTouchPointEventGlobalPosition;
    };

    QInputDevice *getPointingDeviceOrCreate(QInputDevice::DeviceType deviceType);

    std::optional<std::pair<QWindow *, std::uint64_t>> getLastTouchedWindowWithSeqNoIfPresent() const;
    void handleMouseEvent(const QOhosMouseEvent &wsiEvent);
    void handleTouchEvent(const QWindowSystemInterfaceTouchEvent &touchEvent);
    void updateWindowsUnderTouchPoints(const QWindowSystemInterfaceTouchEvent &touchEvent);
    void registerOnWindowCloseToResetMouseButtonsState(QWindow *window);

    std::unordered_map<QInputDevice::DeviceType, QInputDevice *> m_pointingDevices;

    Qt::MouseButtons m_mouseButtonsState = Qt::NoButton;
    std::shared_ptr<void> m_lastMouseEventViewLifetimeTrackerHandle;

    std::map<QWindow *, std::pair<std::shared_ptr<void>, std::uint64_t>> m_windowsUnderTouchPoints;
    QPointer<QWindow> m_currentMouseGrabbingWindow;
    QPointer<QWindow> m_currentKeyboardGrabbingWindow;
    QMap<Qt::Key, ushort> m_autoRepeatCountMap;
    std::optional<QOhosMouseEvent> m_lastWsiMouseEvent;
    std::optional<QWindowSystemInterfaceTouchEvent> m_lastWsiTouchEvent;
    std::optional<std::int32_t> m_optNonClientAreaTouchPointerId;
};

QT_END_NAMESPACE

#endif // QOHOSINPUTMETHODEVENTHANDLER_H
