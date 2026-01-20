// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMEVENT_H
#define QWASMEVENT_H

#include "qwasmplatform.h"
#include "qwasmdom.h"

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>
#include <QtGui/qevent.h>
#include <private/qstdweb_p.h>
#include <QPoint>

#include <emscripten/html5.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE
class QWindow;

enum class EventType {
    DragEnd,
    DragOver,
    DragStart,
    DragEnter,
    DragLeave,
    Drop,
    KeyDown,
    KeyUp,
    PointerDown,
    PointerMove,
    PointerUp,
    PointerEnter,
    PointerLeave,
    PointerCancel,
    Wheel,
};

enum class PointerType {
    Mouse,
    Touch,
    Pen,
    Other,
};

enum class WindowArea {
    NonClient,
    Client,
};

enum class DeltaMode { Pixel, Line, Page };

struct Event
{
    Event(EventType type, emscripten::val webEvent);

    bool isTargetedForElement(emscripten::val element) const;

    emscripten::val webEvent;
    EventType type;
    emscripten::val target() const { return webEvent["target"]; }
};

struct KeyEvent : public Event
{
    KeyEvent(EventType type, emscripten::val webEvent);

    Qt::Key key;
    QFlags<Qt::KeyboardModifier> modifiers;
    bool deadKey;
    QString text;
    bool autoRepeat;
    bool isComposing;
    int keyCode;
};

struct MouseEvent : public Event
{
    MouseEvent(EventType type, emscripten::val webEvent);

    static constexpr Qt::MouseButton buttonFromWeb(int webButton) {
        switch (webButton) {
            case 0:
                return Qt::LeftButton;
            case 1:
                return Qt::MiddleButton;
            case 2:
                return Qt::RightButton;
            default:
                return Qt::NoButton;
        }
    }

    static constexpr Qt::MouseButtons buttonsFromWeb(unsigned short webButtons) {
        // Coincidentally, Qt and web bitfields match.
        return Qt::MouseButtons::fromInt(webButtons);
    }

    static constexpr QEvent::Type mouseEventTypeFromEventType(
        EventType eventType, WindowArea windowArea) {
        switch (eventType) {
            case EventType::PointerDown :
                return windowArea == WindowArea::Client ?
                    QEvent::MouseButtonPress : QEvent::NonClientAreaMouseButtonPress;
            case EventType::PointerUp :
                return windowArea == WindowArea::Client ?
                    QEvent::MouseButtonRelease : QEvent::NonClientAreaMouseButtonRelease;
            case EventType::PointerMove :
                return windowArea == WindowArea::Client ?
                    QEvent::MouseMove : QEvent::NonClientAreaMouseMove;
            default:
                return QEvent::None;
        }
    }

    QPointF localPoint;
    QPointF pointInPage;
    QPointF pointInViewport;
    Qt::MouseButton mouseButton;
    Qt::MouseButtons mouseButtons;
    QFlags<Qt::KeyboardModifier> modifiers;
};

struct PointerEvent : public MouseEvent
{
    PointerEvent(EventType type, emscripten::val webEvent);

    PointerType pointerType;
    int pointerId;
    qreal pressure;
    qreal tiltX;
    qreal tiltY;
    qreal tangentialPressure;
    qreal twist;
    qreal width;
    qreal height;
    bool isPrimary;
};

struct DragEvent : public MouseEvent
{
    DragEvent(EventType type, emscripten::val webEvent, QWindow *targetQWindow);

    void cancelDragStart();
    void acceptDragOver();
    void acceptDrop();

    Qt::DropAction dropAction;
    dom::DataTransfer dataTransfer;
    QWindow *targetWindow;
};

struct WheelEvent : public MouseEvent
{
    WheelEvent(EventType type, emscripten::val webEvent);

    DeltaMode deltaMode;
    bool webkitDirectionInvertedFromDevice;
    QPointF delta;
};

QT_END_NAMESPACE

#endif  // QWASMEVENT_H
