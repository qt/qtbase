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
    ret.pointerType = event["pointerType"].as<std::string>() == "mouse" ?
        PointerType::Mouse : PointerType::Other;
    ret.mouseButton = MouseEvent::buttonFromWeb(event["button"].as<int>());
    ret.mouseButtons = MouseEvent::buttonsFromWeb(event["buttons"].as<unsigned short>());
    ret.point = QPoint(event["offsetX"].as<int>(), event["offsetY"].as<int>());
    ret.pointerId = event["pointerId"].as<int>();
    ret.modifiers = KeyboardModifier::getForEvent(event);

    return ret;
}

QT_END_NAMESPACE
