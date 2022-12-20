// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmwindowclientarea.h"

#include "qwasmdom.h"
#include "qwasmevent.h"
#include "qwasmscreen.h"
#include "qwasmwindow.h"

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtCore/qassert.h>

QT_BEGIN_NAMESPACE

ClientArea::ClientArea(QWasmWindow *window, QWasmScreen *screen, emscripten::val element)
    : m_screen(screen), m_window(window), m_element(element)
{
    const auto callback = std::function([this](emscripten::val event) {
        if (processPointer(*PointerEvent::fromWeb(event)))
            event.call<void>("preventDefault");
    });

    m_pointerDownCallback =
            std::make_unique<qstdweb::EventCallback>(element, "pointerdown", callback);
    m_pointerMoveCallback =
            std::make_unique<qstdweb::EventCallback>(element, "pointermove", callback);
    m_pointerUpCallback = std::make_unique<qstdweb::EventCallback>(element, "pointerup", callback);
}

bool ClientArea::processPointer(const PointerEvent &event)
{
    if (event.pointerType != PointerType::Mouse)
        return false;

    const auto localScreenPoint =
            dom::mapPoint(event.target, m_screen->element(), event.localPoint);
    const auto pointInScreen = m_screen->mapFromLocal(localScreenPoint);

    const QPoint pointInTargetWindowCoords = m_window->mapFromGlobal(pointInScreen);

    switch (event.type) {
    case EventType::PointerDown: {
        m_element.call<void>("setPointerCapture", event.pointerId);
        m_window->window()->requestActivate();
        break;
    }
    case EventType::PointerUp: {
        m_element.call<void>("releasePointerCapture", event.pointerId);
        break;
    }
    case EventType::PointerEnter:;
        QWindowSystemInterface::handleEnterEvent(
                m_window->window(), pointInTargetWindowCoords, pointInScreen);
        break;
    case EventType::PointerLeave:
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

    const QPoint targetPointClippedToScreen(
            std::max(m_screen->geometry().left(),
                     std::min(m_screen->geometry().right(), pointInScreen.x())),
            std::max(m_screen->geometry().top(),
                     std::min(m_screen->geometry().bottom(), pointInScreen.y())));

    const QEvent::Type eventType =
            MouseEvent::mouseEventTypeFromEventType(event.type, WindowArea::Client);

    return eventType != QEvent::None
            && QWindowSystemInterface::handleMouseEvent(
                    m_window->window(), QWasmIntegration::getTimestamp(),
                    m_window->window()->mapFromGlobal(targetPointClippedToScreen),
                    targetPointClippedToScreen, event.mouseButtons, event.mouseButton, eventType,
                    event.modifiers);
}

QT_END_NAMESPACE
