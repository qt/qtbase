// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmevent.h"

QT_BEGIN_NAMESPACE

namespace KeyboardModifier
{
template <>
QFlags<Qt::KeyboardModifier> getForEvent<EmscriptenKeyboardEvent>(
    const EmscriptenKeyboardEvent& event)
{
    return internal::Helper<EmscriptenKeyboardEvent>::getModifierForEvent(event) |
        (event.location == DOM_KEY_LOCATION_NUMPAD ? Qt::KeypadModifier : Qt::NoModifier);
}
}  // namespace KeyboardModifier

std::optional<PointerEvent> PointerEvent::fromWeb(emscripten::val event)
{
    PointerEvent ret;

    const auto eventType = ([&event]() -> std::optional<EventType> {
        const auto eventTypeString = event["type"].as<std::string>();

        if (eventTypeString == "pointermove")
            return EventType::PointerMove;
        else if (eventTypeString == "pointerup")
            return EventType::PointerUp;
        else if (eventTypeString == "pointerdown")
            return EventType::PointerDown;
        else if (eventTypeString == "pointerenter")
            return EventType::PointerEnter;
        else if (eventTypeString == "pointerleave")
            return EventType::PointerLeave;
        return std::nullopt;
    })();
    if (!eventType)
        return std::nullopt;

    ret.type = *eventType;
    ret.target = event["target"];
    ret.pointerType = event["pointerType"].as<std::string>() == "mouse" ?
        PointerType::Mouse : PointerType::Other;
    ret.mouseButton = MouseEvent::buttonFromWeb(event["button"].as<int>());
    ret.mouseButtons = MouseEvent::buttonsFromWeb(event["buttons"].as<unsigned short>());

    // The current button state (event.buttons) may be out of sync for some PointerDown
    // events where the "down" state is very brief, for example taps on Apple trackpads.
    // Qt expects that the current button state is in sync with the event, so we sync
    // it up here.
    if (*eventType == EventType::PointerDown)
        ret.mouseButtons |= ret.mouseButton;

    ret.localPoint = QPoint(event["offsetX"].as<int>(), event["offsetY"].as<int>());
    ret.pointInPage = QPoint(event["pageX"].as<int>(), event["pageY"].as<int>());
    ret.pointInViewport = QPoint(event["clientX"].as<int>(), event["clientY"].as<int>());
    ret.pointerId = event["pointerId"].as<int>();
    ret.modifiers = KeyboardModifier::getForEvent(event);

    return ret;
}

QT_END_NAMESPACE
