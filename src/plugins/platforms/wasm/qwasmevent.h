// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMEVENT_H
#define QWASMEVENT_H

#include "qwasmplatform.h"

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>
#include <QtGui/qevent.h>

#include <QPoint>

#include <emscripten/html5.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

class QWasmDeadKeySupport;

enum class EventType {
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
    Other,
};

enum class WindowArea {
    NonClient,
    Client,
};

enum class DeltaMode { Pixel, Line, Page };

namespace KeyboardModifier {
namespace internal
{
    // Check for the existence of shiftKey, ctrlKey, altKey and metaKey in a type.
    // Based on that, we can safely assume we are dealing with an emscripten event type.
    template<typename T>
    struct IsEmscriptenEvent
    {
        template<typename U, EM_BOOL U::*, EM_BOOL U::*, EM_BOOL U::*, EM_BOOL U::*>
        struct SFINAE {};
        template<typename U> static char Test(
            SFINAE<U, &U::shiftKey, &U::ctrlKey, &U::altKey, &U::metaKey>*);
        template<typename U> static int Test(...);
        static const bool value = sizeof(Test<T>(0)) == sizeof(char);
    };

    template<class T, typename Enable = void>
    struct Helper;

    template<class T>
    struct Helper<T, std::enable_if_t<IsEmscriptenEvent<T>::value>>
    {
        static QFlags<Qt::KeyboardModifier> getModifierForEvent(const T& event) {
            QFlags<Qt::KeyboardModifier> keyModifier = Qt::NoModifier;
            if (event.shiftKey)
                keyModifier |= Qt::ShiftModifier;
            if (event.ctrlKey)
                keyModifier |= platform() == Platform::MacOS ? Qt::MetaModifier : Qt::ControlModifier;
            if (event.altKey)
                keyModifier |= Qt::AltModifier;
            if (event.metaKey)
                keyModifier |= platform() == Platform::MacOS ? Qt::ControlModifier : Qt::MetaModifier;

            return keyModifier;
        }
    };

    template<>
    struct Helper<emscripten::val>
    {
        static QFlags<Qt::KeyboardModifier> getModifierForEvent(const emscripten::val& event) {
            QFlags<Qt::KeyboardModifier> keyModifier = Qt::NoModifier;
            if (event["shiftKey"].as<bool>())
                keyModifier |= Qt::ShiftModifier;
            if (event["ctrlKey"].as<bool>())
                keyModifier |= platform() == Platform::MacOS ? Qt::MetaModifier : Qt::ControlModifier;
            if (event["altKey"].as<bool>())
                keyModifier |= Qt::AltModifier;
            if (event["metaKey"].as<bool>())
                keyModifier |= platform() == Platform::MacOS ? Qt::ControlModifier : Qt::MetaModifier;
            if (event["constructor"]["name"].as<std::string>() == "KeyboardEvent" &&
                event["location"].as<unsigned int>() == DOM_KEY_LOCATION_NUMPAD) {
                keyModifier |= Qt::KeypadModifier;
            }

            return keyModifier;
        }
    };
}  // namespace internal

template <typename Event>
QFlags<Qt::KeyboardModifier> getForEvent(const Event& event)
{
    return internal::Helper<Event>::getModifierForEvent(event);
}

template <>
QFlags<Qt::KeyboardModifier> getForEvent<EmscriptenKeyboardEvent>(
    const EmscriptenKeyboardEvent& event);

}  // namespace KeyboardModifier

struct Event
{
    Event(EventType type, emscripten::val target);
    ~Event();
    Event(const Event &other);
    Event(Event &&other);
    Event &operator=(const Event &other);
    Event &operator=(Event &&other);

    EventType type;
    emscripten::val target = emscripten::val::undefined();
};

struct KeyEvent : public Event
{
    static std::optional<KeyEvent>
    fromWebWithDeadKeyTranslation(emscripten::val webEvent, QWasmDeadKeySupport *deadKeySupport);

    KeyEvent(EventType type, emscripten::val webEvent);
    ~KeyEvent();
    KeyEvent(const KeyEvent &other);
    KeyEvent(KeyEvent &&other);
    KeyEvent &operator=(const KeyEvent &other);
    KeyEvent &operator=(KeyEvent &&other);

    Qt::Key key;
    QFlags<Qt::KeyboardModifier> modifiers;
    bool deadKey;
    QString text;
};

struct MouseEvent : public Event
{
    MouseEvent(EventType type, emscripten::val webEvent);
    ~MouseEvent();
    MouseEvent(const MouseEvent &other);
    MouseEvent(MouseEvent &&other);
    MouseEvent &operator=(const MouseEvent &other);
    MouseEvent &operator=(MouseEvent &&other);

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
    static std::optional<PointerEvent> fromWeb(emscripten::val webEvent);

    PointerEvent(EventType type, emscripten::val webEvent);
    ~PointerEvent();
    PointerEvent(const PointerEvent &other);
    PointerEvent(PointerEvent &&other);
    PointerEvent &operator=(const PointerEvent &other);
    PointerEvent &operator=(PointerEvent &&other);

    PointerType pointerType;
    int pointerId;
    qreal pressure;
    qreal width;
    qreal height;
    bool isPrimary;
};

struct DragEvent : public MouseEvent
{
    static std::optional<DragEvent> fromWeb(emscripten::val webEvent);

    DragEvent(EventType type, emscripten::val webEvent);
    ~DragEvent();
    DragEvent(const DragEvent &other);
    DragEvent(DragEvent &&other);
    DragEvent &operator=(const DragEvent &other);
    DragEvent &operator=(DragEvent &&other);

    Qt::DropAction dropAction;
    emscripten::val dataTransfer;
};

struct WheelEvent : public MouseEvent
{
    static std::optional<WheelEvent> fromWeb(emscripten::val webEvent);

    WheelEvent(EventType type, emscripten::val webEvent);
    ~WheelEvent();
    WheelEvent(const WheelEvent &other);
    WheelEvent(WheelEvent &&other);
    WheelEvent &operator=(const WheelEvent &other);
    WheelEvent &operator=(WheelEvent &&other);

    DeltaMode deltaMode;
    bool webkitDirectionInvertedFromDevice;
    QPointF delta;
};

QT_END_NAMESPACE

#endif  // QWASMEVENT_H
