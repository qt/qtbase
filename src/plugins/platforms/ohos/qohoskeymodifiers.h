// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSKEYMODIFIERS_H
#define QOHOSKEYMODIFIERS_H

#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <QtCore/qspan.h>
#include <arkui/ui_input_event.h>
#include <multimodalinput/oh_input_manager.h>
#include <vector>

QT_BEGIN_NAMESPACE

enum class OhosKeyboardModifier
{
    CTRL = 1 << 0,
    ALT = 1 << 1,
    SHIFT = 1 << 2,
    LOGO = 1 << 3,
    CAPS_LOCK = 1 << 4,
    NUM_LOCK = 1 << 5,
};

struct OhosKeyToModifier
{
    OhosKeyboardModifier modifier;
    std::vector<::Input_KeyCode> keysToCheck;
    std::int32_t (*getKeyStateActionFunc)(const ::Input_KeyState *);
    ::Input_KeyStateAction keyStateActionKeyActive;
};

QFlags<OhosKeyboardModifier> readKeyModifiersFromKeyState(QSpan<const OhosKeyToModifier> keysToModifiers);
QFlags<OhosKeyboardModifier> readKeyModifiersFromOhosUiInputEvent(::ArkUI_UIInputEvent *uiInputEvent);
Qt::KeyboardModifiers convertOhosToQtKeyboardModifiers(QFlags<OhosKeyboardModifier> ohosKeysModifiers);

QT_END_NAMESPACE

#endif // QOHOSKEYMODIFIERS_H
