// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmwindowclientarea.h"

#include "qwasmdom.h"
#include "qwasmevent.h"
#include "qwasmscreen.h"
#include "qwasmwindow.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpointingdevice.h>

#include <QtCore/qassert.h>

QT_BEGIN_NAMESPACE

ClientArea::ClientArea(QWasmWindow *window, QWasmScreen *screen, emscripten::val element)
    : m_screen(screen), m_window(window), m_element(element)
{
    const auto callback = std::function([this](emscripten::val event) {
        processPointer(*PointerEvent::fromWeb(event));
        event.call<void>("preventDefault");
        event.call<void>("stopPropagation");
    });

    m_pointerDownCallback =
            std::make_unique<qstdweb::EventCallback>(element, "pointerdown", callback);
    m_pointerMoveCallback =
            std::make_unique<qstdweb::EventCallback>(element, "pointermove", callback);
    m_pointerUpCallback = std::make_unique<qstdweb::EventCallback>(element, "pointerup", callback);
    m_pointerCancelCallback =
            std::make_unique<qstdweb::EventCallback>(element, "pointercancel", callback);
}

bool ClientArea::processPointer(const PointerEvent &event)
{
    const auto localScreenPoint =
            dom::mapPoint(event.target, m_screen->element(), event.localPoint);
    const auto pointInScreen = m_screen->mapFromLocal(localScreenPoint);

    const QPointF pointInTargetWindowCoords = m_window->window()->mapFromGlobal(pointInScreen);

    switch (event.type) {
    case EventType::PointerDown:
        m_element.call<void>("setPointerCapture", event.pointerId);
        m_window->window()->requestActivate();
        break;
    case EventType::PointerUp:
        m_element.call<void>("releasePointerCapture", event.pointerId);
        break;
    case EventType::PointerEnter:
        if (event.isPrimary) {
            QWindowSystemInterface::handleEnterEvent(m_window->window(), pointInTargetWindowCoords,
                                                     pointInScreen);
        }
        break;
    case EventType::PointerLeave:
        if (event.isPrimary)
            QWindowSystemInterface::handleLeaveEvent(m_window->window());
        break;
    default:
        break;
    };

    const bool eventAccepted = deliverEvent(event);
    if (!eventAccepted && event.type == EventType::PointerDown)
        QGuiApplicationPrivate::instance()->closeAllPopups();
    return eventAccepted;
}

bool ClientArea::deliverEvent(const PointerEvent &event)
{
    const auto pointInScreen = m_screen->mapFromLocal(
            dom::mapPoint(event.target, m_screen->element(), event.localPoint));

    const auto geometryF = m_screen->geometry().toRectF();
    const QPointF targetPointClippedToScreen(
            qBound(geometryF.left(), pointInScreen.x(), geometryF.right()),
            qBound(geometryF.top(), pointInScreen.y(), geometryF.bottom()));

    if (event.pointerType == PointerType::Mouse) {
        const QEvent::Type eventType =
                MouseEvent::mouseEventTypeFromEventType(event.type, WindowArea::Client);

        return eventType != QEvent::None
                && QWindowSystemInterface::handleMouseEvent(
                        m_window->window(), QWasmIntegration::getTimestamp(),
                        m_window->window()->mapFromGlobal(targetPointClippedToScreen),
                        targetPointClippedToScreen, event.mouseButtons, event.mouseButton,
                        eventType, event.modifiers);
    }

    QWindowSystemInterface::TouchPoint *touchPoint;

    QPointF pointInTargetWindowCoords =
            QPointF(m_window->window()->mapFromGlobal(targetPointClippedToScreen));
    QPointF normalPosition(pointInTargetWindowCoords.x() / m_window->window()->width(),
                           pointInTargetWindowCoords.y() / m_window->window()->height());

    const auto tp = m_pointerIdToTouchPoints.find(event.pointerId);
    if (tp != m_pointerIdToTouchPoints.end()) {
        touchPoint = &tp.value();
    } else {
        touchPoint = &m_pointerIdToTouchPoints
                              .insert(event.pointerId, QWindowSystemInterface::TouchPoint())
                              .value();

        // Assign touch point id. TouchPoint::id is int, but QGuiApplicationPrivate::processTouchEvent()
        // will not synthesize mouse events for touch points with negative id; use the absolute value for
        // the touch point id.
        touchPoint->id = qAbs(event.pointerId);

        touchPoint->state = QEventPoint::State::Pressed;
    }

    const bool stationaryTouchPoint = (normalPosition == touchPoint->normalPosition);
    touchPoint->normalPosition = normalPosition;
    touchPoint->area = QRectF(targetPointClippedToScreen, QSizeF(event.width, event.height))
                                   .translated(-event.width / 2, -event.height / 2);
    touchPoint->pressure = event.pressure;

    switch (event.type) {
    case EventType::PointerUp:
        touchPoint->state = QEventPoint::State::Released;
        break;
    case EventType::PointerMove:
        touchPoint->state = (stationaryTouchPoint ? QEventPoint::State::Stationary
                                                  : QEventPoint::State::Updated);
        break;
    default:
        break;
    }

    QList<QWindowSystemInterface::TouchPoint> touchPointList;
    touchPointList.reserve(m_pointerIdToTouchPoints.size());
    std::transform(m_pointerIdToTouchPoints.begin(), m_pointerIdToTouchPoints.end(),
                   std::back_inserter(touchPointList),
                   [](const QWindowSystemInterface::TouchPoint &val) { return val; });

    if (event.type == EventType::PointerUp)
        m_pointerIdToTouchPoints.remove(event.pointerId);

    return event.type == EventType::PointerCancel
            ? QWindowSystemInterface::handleTouchCancelEvent(
                    m_window->window(), QWasmIntegration::getTimestamp(), m_screen->touchDevice(),
                    event.modifiers)
            : QWindowSystemInterface::handleTouchEvent(
                    m_window->window(), QWasmIntegration::getTimestamp(), m_screen->touchDevice(),
                    touchPointList, event.modifiers);
}

QT_END_NAMESPACE
