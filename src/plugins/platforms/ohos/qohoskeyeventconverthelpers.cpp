// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohoskeyeventconverthelpers.h>

#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

namespace QtKeyEventHelpers {

namespace {

const QMap<Qt::Key, QPair<OhosKeyboardModifier, Qt::KeyboardModifier>> qtKeyToModifiersMap = {
    {Qt::Key_Shift, {OhosKeyboardModifier::SHIFT, Qt::ShiftModifier}},
    {Qt::Key_Alt, {OhosKeyboardModifier::ALT, Qt::AltModifier}},
    {Qt::Key_Control, {OhosKeyboardModifier::CTRL, Qt::ControlModifier}},
    {Qt::Key_Meta, {OhosKeyboardModifier::LOGO, Qt::MetaModifier}},
};

bool isNoModificatorPressed(Qt::KeyboardModifiers keyboardModifiers)
{
    return keyboardModifiers.testFlag(Qt::NoModifier);
}

bool isOnlyKeypadModificatorOn(Qt::KeyboardModifiers keyboardModifiers)
{
    return keyboardModifiers == Qt::KeyboardModifiers(Qt::KeypadModifier);
}

char tryConvertKeyEventToKeyCharOrNull(const Qt::Key &key, Qt::KeyboardModifiers qtModifiers)
{
    bool onlyShiftModPressed = isOnlyShiftModificatorPressed(qtModifiers);
    bool onlyKeypadModificatorOn = isOnlyKeypadModificatorOn(qtModifiers);
    bool noModPressed = isNoModificatorPressed(qtModifiers);
    bool useNoModifiedKeyText = noModPressed || onlyKeypadModificatorOn;

    switch (key) {
    case Qt::Key_Space: return ' ';
    case Qt::Key_Backspace: return '\x08';
    case Qt::Key_Tab: return '\x09';
    case Qt::Key_Enter: return '\x0D';
    case Qt::Key_Return: return '\x0D';
    case Qt::Key_Delete: return '\x7F';
    case Qt::Key_Escape: return '\x1B';
    default:
        break;
    }

    if (onlyShiftModPressed || useNoModifiedKeyText) {
        switch (key) {
        case Qt::Key_A: return 'a';
        case Qt::Key_B: return 'b';
        case Qt::Key_C: return 'c';
        case Qt::Key_D: return 'd';
        case Qt::Key_E: return 'e';
        case Qt::Key_F: return 'f';
        case Qt::Key_G: return 'g';
        case Qt::Key_H: return 'h';
        case Qt::Key_I: return 'i';
        case Qt::Key_J: return 'j';
        case Qt::Key_K: return 'k';
        case Qt::Key_L: return 'l';
        case Qt::Key_M: return 'm';
        case Qt::Key_N: return 'n';
        case Qt::Key_O: return 'o';
        case Qt::Key_P: return 'p';
        case Qt::Key_Q: return 'q';
        case Qt::Key_R: return 'r';
        case Qt::Key_S: return 's';
        case Qt::Key_T: return 't';
        case Qt::Key_U: return 'u';
        case Qt::Key_V: return 'v';
        case Qt::Key_W: return 'w';
        case Qt::Key_X: return 'x';
        case Qt::Key_Y: return 'y';
        case Qt::Key_Z: return 'z';
        case Qt::Key_0: return '0';
        case Qt::Key_1: return '1';
        case Qt::Key_2: return '2';
        case Qt::Key_3: return '3';
        case Qt::Key_4: return '4';
        case Qt::Key_5: return '5';
        case Qt::Key_6: return '6';
        case Qt::Key_7: return '7';
        case Qt::Key_8: return '8';
        case Qt::Key_9: return '9';
        case Qt::Key_QuoteLeft: return '`';
        case Qt::Key_Minus: return '-';
        case Qt::Key_Equal: return '=';
        case Qt::Key_BracketLeft: return '[';
        case Qt::Key_BracketRight: return ']';
        case Qt::Key_Backslash: return '\\';
        case Qt::Key_Semicolon: return ';';
        case Qt::Key_Apostrophe: return '\'';
        case Qt::Key_Comma: return ',';
        case Qt::Key_Period: return '.';
        case Qt::Key_Slash: return '/';
        case Qt::Key_ParenRight: return ')';
        case Qt::Key_Exclam: return '!';
        case Qt::Key_At: return '@';
        case Qt::Key_NumberSign: return '#';
        case Qt::Key_Dollar: return '$';
        case Qt::Key_Percent: return '%';
        case Qt::Key_AsciiCircum: return '^';
        case Qt::Key_Ampersand: return '&';
        case Qt::Key_Asterisk: return '*';
        case Qt::Key_ParenLeft: return '(';
        case Qt::Key_AsciiTilde: return '~';
        case Qt::Key_Underscore: return '_';
        case Qt::Key_Plus: return '+';
        case Qt::Key_BraceLeft: return '{';
        case Qt::Key_BraceRight: return '}';
        case Qt::Key_Bar: return '|';
        case Qt::Key_Colon: return ':';
        case Qt::Key_QuoteDbl: return '"';
        case Qt::Key_Less: return '<';
        case Qt::Key_Greater: return '>';
        case Qt::Key_Question: return '?';
        default:
            break;
        }
    }

    return '\0';
}

}

Qt::KeyboardModifiers convertOhosToQtKeyboardModifiersWithNumpad(
    QFlags<OhosKeyboardModifier> ohosKeysModifiers, bool keyFromNumPad)
{
    auto keyboardModifiers = convertOhosToQtKeyboardModifiers(ohosKeysModifiers);
    keyboardModifiers.setFlag(Qt::KeypadModifier, keyFromNumPad);

    return keyboardModifiers;
}

bool isOnlyShiftModificatorPressed(Qt::KeyboardModifiers keyboardModifiers)
{
    return keyboardModifiers.testFlag(Qt::ShiftModifier) &&
          !keyboardModifiers.testFlag(Qt::AltModifier) &&
          !keyboardModifiers.testFlag(Qt::ControlModifier);
}

QString tryConvertQtKeyCodeToKeyText(
    const Qt::Key &key, Qt::KeyboardModifiers qtModifiers, QFlags<OhosKeyboardModifier> ohosKeysModifiers)
{
    auto keyChar = QChar::fromLatin1(tryConvertKeyEventToKeyCharOrNull(key, qtModifiers));
    bool keyCapsLockFlagOn = ohosKeysModifiers.testFlag(OhosKeyboardModifier::CAPS_LOCK);
    auto effectiveKeyChar =
        keyChar.isLetter() && keyCapsLockFlagOn != isOnlyShiftModificatorPressed(qtModifiers)
            ? keyChar.toUpper()
            : keyChar;
    return !effectiveKeyChar.isNull() ? QString(effectiveKeyChar) : QString();
}

Qt::KeyboardModifiers convertCurrentOhosKeyModifiersForQGuiApplication(
    Qt::KeyboardModifiers lastKeyboardModifiers, QFlags<OhosKeyboardModifier> ohosKeysModifiers, Qt::Key qtKey)
{
    auto modifiers = lastKeyboardModifiers;
    if (qtKeyToModifiersMap.contains(qtKey)) {
        const auto actualKeyModifiers = qtKeyToModifiersMap.value(qtKey);
        const auto ohosModifier = actualKeyModifiers.first;
        const auto qtModifier = actualKeyModifiers.second;
        modifiers.setFlag(qtModifier, !ohosKeysModifiers.testFlag(ohosModifier));
    }
    return modifiers;
}

}

QT_END_NAMESPACE
