// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmwindowclientarea.h"

#include "qwasmdom.h"
#include "qwasmevent.h"
#include "qwasmscreen.h"
#include "qwasmwindow.h"
#include "qwasmdrag.h"

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpointingdevice.h>

#include <QtCore/qassert.h>

QT_BEGIN_NAMESPACE

ClientArea::ClientArea(QWasmWindow *window, QWasmScreen *screen, emscripten::val element)
    : m_screen(screen), m_window(window), m_element(element)
{
    m_pointerDownCallback = std::make_unique<qstdweb::EventCallback>(element, "pointerdown",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerDown, event)); }
    );
    m_pointerMoveCallback = std::make_unique<qstdweb::EventCallback>(element, "pointermove",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerMove, event)); }
    );
    m_pointerUpCallback = std::make_unique<qstdweb::EventCallback>(element, "pointerup",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerUp, event)); }
    );
    m_pointerCancelCallback = std::make_unique<qstdweb::EventCallback>(element, "pointercancel",
        [this](emscripten::val event){ processPointer(PointerEvent(EventType::PointerCancel, event)); }
    );

    element.call<void>("setAttribute", emscripten::val("draggable"), emscripten::val("true"));
    m_dragStartCallback = std::make_unique<qstdweb::EventCallback>(element, "dragstart",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragStart, event, m_window->window());
            QWasmDrag::instance()->onNativeDragStarted(&dragEvent);
        }
    );
    m_dragOverCallback = std::make_unique<qstdweb::EventCallback>(element, "dragover",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragOver, event, m_window->window());
            QWasmDrag::instance()->onNativeDragOver(&dragEvent);
        }
    );
    m_dropCallback = std::make_unique<qstdweb::EventCallback>(element, "drop",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::Drop, event, m_window->window());
            QWasmDrag::instance()->onNativeDrop(&dragEvent);
        }
    );
    m_dragEndCallback = std::make_unique<qstdweb::EventCallback>(element, "dragend",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragEnd, event, m_window->window());
            QWasmDrag::instance()->onNativeDragFinished(&dragEvent);
        }
    );
    m_dragLeaveCallback = std::make_unique<qstdweb::EventCallback>(element, "dragleave",
        [this](emscripten::val event) {
            DragEvent dragEvent(EventType::DragLeave, event, m_window->window());
            QWasmDrag::instance()->onNativeDragLeave(&dragEvent);
        }
    );
}

void ClientArea::processPointer(const PointerEvent &event)
{
    switch (event.type) {
    case EventType::PointerDown:
        m_element.call<void>("setPointerCapture", event.pointerId);
        if ((m_window->window()->flags() & Qt::WindowDoesNotAcceptFocus)
                    != Qt::WindowDoesNotAcceptFocus
            && m_window->window()->isTopLevel())
                m_window->window()->requestActivate();
        break;
    case EventType::PointerUp:
        m_element.call<void>("releasePointerCapture", event.pointerId);
        break;
    default:
        break;
    };

    const bool eventAccepted = deliverEvent(event);
    if (!eventAccepted && event.type == EventType::PointerDown)
        QGuiApplicationPrivate::instance()->closeAllPopups();
    event.webEvent.call<void>("preventDefault");
    event.webEvent.call<void>("stopPropagation");
}

bool ClientArea::deliverEvent(const PointerEvent &event)
{
    const auto pointInScreen = m_screen->mapFromLocal(
        dom::mapPoint(event.target(), m_screen->element(), event.localPoint));

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

    if (event.pointerType == PointerType::Pen) {
        qreal pressure;
        switch (event.type) {
            case EventType::PointerDown :
            case EventType::PointerMove :
                pressure = event.pressure;
                break;
            case EventType::PointerUp :
                pressure = 0.0;
                break;
            default:
                return false;
        }
        // Tilt in the browser is in the range +-90, but QTabletEvent only goes to +-60.
        qreal xTilt = qBound(-60.0, event.tiltX, 60.0);
        qreal yTilt = qBound(-60.0, event.tiltY, 60.0);
        // Barrel rotation is reported as 0 to 359, but QTabletEvent wants a signed value.
        qreal rotation = event.twist > 180.0 ? 360.0 - event.twist : event.twist;
        return QWindowSystemInterface::handleTabletEvent(
            m_window->window(), QWasmIntegration::getTimestamp(), m_screen->tabletDevice(),
            m_window->window()->mapFromGlobal(targetPointClippedToScreen),
            targetPointClippedToScreen, event.mouseButtons, pressure, xTilt, yTilt,
            event.tangentialPressure, rotation, event.modifiers);
    }

    QWindowSystemInterface::TouchPoint *touchPoint;

    QPointF pointInTargetWindowCoords =
            QPointF(m_window->window()->mapFromGlobal(targetPointClippedToScreen));
    QPointF normalPosition(pointInTargetWindowCoords.x() / m_window->window()->width(),
                           pointInTargetWindowCoords.y() / m_window->window()->height());

    const auto tp = m_pointerIdToTouchPoints.find(event.pointerId);
    if (event.pointerType != PointerType::Pen && tp != m_pointerIdToTouchPoints.end()) {
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
