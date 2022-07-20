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

enum class EventType {
    PointerDown,
    PointerMove,
    PointerUp,
    PointerEnter,
    PointerLeave,
};

enum class PointerType {
    Mouse,
    Other,
};

enum class WindowArea {
    NonClient,
    Client,
};

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

struct Q_CORE_EXPORT Event
{
    EventType type;
};

struct Q_CORE_EXPORT MouseEvent : public Event
{
    QPoint point;
    Qt::MouseButton mouseButton;
    Qt::MouseButtons mouseButtons;
    QFlags<Qt::KeyboardModifier> modifiers;

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
};

struct Q_CORE_EXPORT PointerEvent : public MouseEvent
{
    static std::optional<PointerEvent> fromWeb(emscripten::val webEvent);

    PointerType pointerType;
    int pointerId;
};

QT_END_NAMESPACE

#endif  // QWASMEVENT_H
