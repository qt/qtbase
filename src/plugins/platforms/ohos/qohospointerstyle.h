
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPOINTERSTYLE_H
#define QOHOSPOINTERSTYLE_H

#include <array>
#include <qohosplugincore.h>
#include <utility>

QT_BEGIN_NAMESPACE

// QOhosPointerStyle enum and enumerators correspond to OHOS PointerStyle enum.
// PointerStyle is declared in '@ohos.multimodalInput.pointer' module.
enum class QOhosPointerStyle {
    DEFAULT,
    EAST,
    WEST,
    SOUTH,
    NORTH,
    WEST_EAST,
    NORTH_SOUTH,
    NORTH_EAST,
    NORTH_WEST,
    SOUTH_EAST,
    SOUTH_WEST,
    NORTH_EAST_SOUTH_WEST,
    NORTH_WEST_SOUTH_EAST,
    CROSS,
    CURSOR_COPY,
    CURSOR_FORBID,
    COLOR_SUCKER,
    HAND_GRABBING,
    HAND_OPEN,
    HAND_POINTING,
    HELP,
    MOVE,
    RESIZE_LEFT_RIGHT,
    RESIZE_UP_DOWN,
    SCREENSHOT_CHOOSE,
    SCREENSHOT_CURSOR,
    TEXT_CURSOR,
    ZOOM_IN,
    ZOOM_OUT,
    MIDDLE_BTN_EAST,
    MIDDLE_BTN_WEST,
    MIDDLE_BTN_SOUTH,
    MIDDLE_BTN_NORTH,
    MIDDLE_BTN_NORTH_SOUTH,
    MIDDLE_BTN_NORTH_EAST,
    MIDDLE_BTN_NORTH_WEST,
    MIDDLE_BTN_SOUTH_EAST,
    MIDDLE_BTN_SOUTH_WEST,
    MIDDLE_BTN_NORTH_SOUTH_WEST_EAST,
    HORIZONTAL_TEXT_CURSOR,
    CURSOR_CROSS,
    CURSOR_CIRCLE,
    LOADING,
    RUNNING,
};

namespace QtOhos
{

template<>
struct OhosEnumMeta<QOhosPointerStyle>
{
    static constexpr const char *fullTypeName = "@ohos.multimodalInput.pointer.PointerStyle";
    static constexpr std::array<std::pair<QOhosPointerStyle, const char *>, 44> enumeratorsNames = {{
        {QOhosPointerStyle::DEFAULT, "DEFAULT"},
        {QOhosPointerStyle::EAST, "EAST"},
        {QOhosPointerStyle::WEST, "WEST"},
        {QOhosPointerStyle::SOUTH, "SOUTH"},
        {QOhosPointerStyle::NORTH, "NORTH"},
        {QOhosPointerStyle::WEST_EAST, "WEST_EAST"},
        {QOhosPointerStyle::NORTH_SOUTH, "NORTH_SOUTH"},
        {QOhosPointerStyle::NORTH_EAST, "NORTH_EAST"},
        {QOhosPointerStyle::NORTH_WEST, "NORTH_WEST"},
        {QOhosPointerStyle::SOUTH_EAST, "SOUTH_EAST"},
        {QOhosPointerStyle::SOUTH_WEST, "SOUTH_WEST"},
        {QOhosPointerStyle::NORTH_EAST_SOUTH_WEST, "NORTH_EAST_SOUTH_WEST"},
        {QOhosPointerStyle::NORTH_WEST_SOUTH_EAST, "NORTH_WEST_SOUTH_EAST"},
        {QOhosPointerStyle::CROSS, "CROSS"},
        {QOhosPointerStyle::CURSOR_COPY, "CURSOR_COPY"},
        {QOhosPointerStyle::CURSOR_FORBID, "CURSOR_FORBID"},
        {QOhosPointerStyle::COLOR_SUCKER, "COLOR_SUCKER"},
        {QOhosPointerStyle::HAND_GRABBING, "HAND_GRABBING"},
        {QOhosPointerStyle::HAND_OPEN, "HAND_OPEN"},
        {QOhosPointerStyle::HAND_POINTING, "HAND_POINTING"},
        {QOhosPointerStyle::HELP, "HELP"},
        {QOhosPointerStyle::MOVE, "MOVE"},
        {QOhosPointerStyle::RESIZE_LEFT_RIGHT, "RESIZE_LEFT_RIGHT"},
        {QOhosPointerStyle::RESIZE_UP_DOWN, "RESIZE_UP_DOWN"},
        {QOhosPointerStyle::SCREENSHOT_CHOOSE, "SCREENSHOT_CHOOSE"},
        {QOhosPointerStyle::SCREENSHOT_CURSOR, "SCREENSHOT_CURSOR"},
        {QOhosPointerStyle::TEXT_CURSOR, "TEXT_CURSOR"},
        {QOhosPointerStyle::ZOOM_IN, "ZOOM_IN"},
        {QOhosPointerStyle::ZOOM_OUT, "ZOOM_OUT"},
        {QOhosPointerStyle::MIDDLE_BTN_EAST, "MIDDLE_BTN_EAST"},
        {QOhosPointerStyle::MIDDLE_BTN_WEST, "MIDDLE_BTN_WEST"},
        {QOhosPointerStyle::MIDDLE_BTN_SOUTH, "MIDDLE_BTN_SOUTH"},
        {QOhosPointerStyle::MIDDLE_BTN_NORTH, "MIDDLE_BTN_NORTH"},
        {QOhosPointerStyle::MIDDLE_BTN_NORTH_SOUTH, "MIDDLE_BTN_NORTH_SOUTH"},
        {QOhosPointerStyle::MIDDLE_BTN_NORTH_EAST, "MIDDLE_BTN_NORTH_EAST"},
        {QOhosPointerStyle::MIDDLE_BTN_NORTH_WEST, "MIDDLE_BTN_NORTH_WEST"},
        {QOhosPointerStyle::MIDDLE_BTN_SOUTH_EAST, "MIDDLE_BTN_SOUTH_EAST"},
        {QOhosPointerStyle::MIDDLE_BTN_SOUTH_WEST, "MIDDLE_BTN_SOUTH_WEST"},
        {QOhosPointerStyle::MIDDLE_BTN_NORTH_SOUTH_WEST_EAST, "MIDDLE_BTN_NORTH_SOUTH_WEST_EAST"},
        {QOhosPointerStyle::HORIZONTAL_TEXT_CURSOR, "HORIZONTAL_TEXT_CURSOR"},
        {QOhosPointerStyle::CURSOR_CROSS, "CURSOR_CROSS"},
        {QOhosPointerStyle::CURSOR_CIRCLE, "CURSOR_CIRCLE"},
        {QOhosPointerStyle::LOADING, "LOADING"},
        {QOhosPointerStyle::RUNNING, "RUNNING"},
    }};
};

}

QT_END_NAMESPACE

#endif // QOHOSPOINTERSTYLE_H
