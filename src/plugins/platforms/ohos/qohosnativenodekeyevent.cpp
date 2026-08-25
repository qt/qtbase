// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosnativenodekeyevent.h>

#include <QtCore/qmap.h>
#include <optional>

QT_BEGIN_NAMESPACE

namespace
{

const QMap<::ArkUI_KeyCode, Qt::Key> ohosNativeNodeNumpadSpecialQtKeyMap = {
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_0, Qt::Key_Insert},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_1, Qt::Key_End},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_2, Qt::Key_Down},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_3, Qt::Key_PageDown},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_4, Qt::Key_Left},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_5, Qt::Key_Clear},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_6, Qt::Key_Right},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_7, Qt::Key_Home},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_8, Qt::Key_Up},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_9, Qt::Key_PageUp},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_COMMA, Qt::Key_Delete},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_DOT, Qt::Key_Delete},
};

QMap<::ArkUI_KeyCode, Qt::Key> ohosNativeNodeQtKeyMap = {
    {::ArkUI_KeyCode::ARKUI_KEYCODE_UNKNOWN, Qt::Key_unknown},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_VOLUME_UP, Qt::Key_VolumeUp},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_VOLUME_DOWN, Qt::Key_VolumeDown},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_POWER, Qt::Key_PowerDown},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_CAMERA, Qt::Key_Camera},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_VOLUME_MUTE, Qt::Key_VolumeMute},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MUTE, Qt::Key_VolumeMute},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_BRIGHTNESS_UP, Qt::Key_MonBrightnessUp},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_BRIGHTNESS_DOWN, Qt::Key_MonBrightnessDown},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_0, Qt::Key_0},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_1, Qt::Key_1},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_2, Qt::Key_2},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_3, Qt::Key_3},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_4, Qt::Key_4},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_5, Qt::Key_5},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_6, Qt::Key_6},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_7, Qt::Key_7},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_8, Qt::Key_8},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_9, Qt::Key_9},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_STAR, Qt::Key_Asterisk},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_POUND, Qt::Key_NumberSign},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_DPAD_UP, Qt::Key_Up},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_DPAD_DOWN, Qt::Key_Down},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_DPAD_LEFT, Qt::Key_Left},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_DPAD_RIGHT, Qt::Key_Right},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_DPAD_CENTER, Qt::Key_Enter},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_A, Qt::Key_A},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_B, Qt::Key_B},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_C, Qt::Key_C},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_D, Qt::Key_D},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_E, Qt::Key_E},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F, Qt::Key_F},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_G, Qt::Key_G},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_H, Qt::Key_H},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_I, Qt::Key_I},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_J, Qt::Key_J},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_K, Qt::Key_K},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_L, Qt::Key_L},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_M, Qt::Key_M},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_N, Qt::Key_N},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_O, Qt::Key_O},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_P, Qt::Key_P},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_Q, Qt::Key_Q},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_R, Qt::Key_R},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_S, Qt::Key_S},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_T, Qt::Key_T},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_U, Qt::Key_U},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_V, Qt::Key_V},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_W, Qt::Key_W},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_X, Qt::Key_X},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_Y, Qt::Key_Y},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_Z, Qt::Key_Z},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_COMMA, Qt::Key_Comma},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_PERIOD, Qt::Key_Period},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_ALT_LEFT, Qt::Key_Alt},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_ALT_RIGHT, Qt::Key_Alt},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_SHIFT_LEFT, Qt::Key_Shift},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_SHIFT_RIGHT, Qt::Key_Shift},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_TAB, Qt::Key_Tab},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_SPACE, Qt::Key_Space},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_EXPLORER, Qt::Key_Explorer},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_ENVELOPE, Qt::Key_LaunchMail},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_ENTER, Qt::Key_Return},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_DEL, Qt::Key_Backspace},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_GRAVE, Qt::Key_QuoteLeft},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MINUS, Qt::Key_Minus},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_EQUALS, Qt::Key_Equal},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_LEFT_BRACKET, Qt::Key_BracketLeft},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_RIGHT_BRACKET, Qt::Key_BracketRight},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_BACKSLASH, Qt::Key_Backslash},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_SEMICOLON, Qt::Key_Semicolon},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_APOSTROPHE, Qt::Key_Apostrophe},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_SLASH, Qt::Key_Slash},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MENU, Qt::Key_Menu},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_PAGE_UP, Qt::Key_PageUp},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_PAGE_DOWN, Qt::Key_PageDown},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_ESCAPE, Qt::Key_Escape},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_FORWARD_DEL, Qt::Key_Delete},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_CTRL_LEFT, Qt::Key_Control},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_CTRL_RIGHT, Qt::Key_Control},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_CAPS_LOCK, Qt::Key_CapsLock},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_SCROLL_LOCK, Qt::Key_ScrollLock},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_META_LEFT, Qt::Key_Meta},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_META_RIGHT, Qt::Key_Meta},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_SYSRQ, Qt::Key_SysReq},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MOVE_HOME, Qt::Key_Home},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MOVE_END, Qt::Key_End},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_INSERT, Qt::Key_Insert},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_FORWARD, Qt::Key_Forward},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MEDIA_PLAY, Qt::Key_MediaPlay},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MEDIA_PAUSE, Qt::Key_MediaPause},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MEDIA_CLOSE, Qt::Key_MediaStop},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MEDIA_EJECT, Qt::Key_Eject},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MEDIA_RECORD, Qt::Key_MediaRecord},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F1, Qt::Key_F1},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F2, Qt::Key_F2},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F3, Qt::Key_F3},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F4, Qt::Key_F4},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F5, Qt::Key_F5},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F6, Qt::Key_F6},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F7, Qt::Key_F7},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F8, Qt::Key_F8},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F9, Qt::Key_F9},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F10, Qt::Key_F10},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F11, Qt::Key_F11},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_F12, Qt::Key_F12},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUM_LOCK, Qt::Key_NumLock},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_0, Qt::Key_0},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_1, Qt::Key_1},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_2, Qt::Key_2},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_3, Qt::Key_3},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_4, Qt::Key_4},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_5, Qt::Key_5},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_6, Qt::Key_6},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_7, Qt::Key_7},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_8, Qt::Key_8},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_9, Qt::Key_9},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_DIVIDE, Qt::Key_Slash},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_MULTIPLY, Qt::Key_Asterisk},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_SUBTRACT, Qt::Key_Minus},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_ADD, Qt::Key_Plus},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_DOT, Qt::Key_Period},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_COMMA, Qt::Key_Comma},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_ENTER, Qt::Key_Enter},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_EQUALS, Qt::Key_Equal},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_LEFT_PAREN, Qt::Key_ParenLeft},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_RIGHT_PAREN, Qt::Key_ParenRight},
};

const QMap<::ArkUI_KeyCode, Qt::Key> ohosNativeNodeQtKeyShiftPressedMap = {
    {::ArkUI_KeyCode::ARKUI_KEYCODE_0, Qt::Key_ParenRight},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_1, Qt::Key_Exclam},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_2, Qt::Key_At},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_3, Qt::Key_NumberSign},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_4, Qt::Key_Dollar},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_5, Qt::Key_Percent},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_6, Qt::Key_AsciiCircum},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_7, Qt::Key_Ampersand},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_8, Qt::Key_Asterisk},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_9, Qt::Key_ParenLeft},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_LEFT_BRACKET, Qt::Key_BraceLeft},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_RIGHT_BRACKET, Qt::Key_BraceRight},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_BACKSLASH, Qt::Key_Bar},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_SEMICOLON, Qt::Key_Colon},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_APOSTROPHE, Qt::Key_QuoteDbl},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_SLASH, Qt::Key_Question},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_COMMA, Qt::Key_Less},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_PERIOD, Qt::Key_Greater},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_MINUS, Qt::Key_Underscore},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_EQUALS, Qt::Key_Plus},
    {::ArkUI_KeyCode::ARKUI_KEYCODE_GRAVE, Qt::Key_AsciiTilde},
};

class QOhosNativeNodeKeyEvent final : public QOhosKeyEvent
{
public:
    QOhosNativeNodeKeyEvent(
        ::ArkUI_KeyEventType keyEventType,
        ::ArkUI_KeyCode keyCode,
        QFlags<OhosKeyboardModifier> keysFlags);
    ~QOhosNativeNodeKeyEvent() override;

    std::optional<QOhosQtKeyEvent> tryConvertToQOhosQtKeyEvent() const override;
    bool equals(const QOhosKeyEvent &other) const override;

private:
    QEvent::Type convertKeyEventType() const;
    bool isKeyFromNumPad() const;
    Qt::Key convertKeyCode(Qt::KeyboardModifiers qtModifiers) const;

    ::ArkUI_KeyEventType m_keyEventType;
    ::ArkUI_KeyCode m_keyCode;
    QFlags<OhosKeyboardModifier> m_keysFlags;
};

QOhosNativeNodeKeyEvent::QOhosNativeNodeKeyEvent(
    ::ArkUI_KeyEventType keyEventType,
    ::ArkUI_KeyCode keyCode,
    QFlags<OhosKeyboardModifier> keysFlags)
    : m_keyEventType(keyEventType), m_keyCode(keyCode), m_keysFlags(keysFlags)
{
}

QOhosNativeNodeKeyEvent::~QOhosNativeNodeKeyEvent() = default;

std::optional<QOhosQtKeyEvent> QOhosNativeNodeKeyEvent::tryConvertToQOhosQtKeyEvent() const
{
    if (m_keyEventType == ::ArkUI_KeyEventType::ARKUI_KEY_EVENT_UNKNOWN) {
        qOhosWarning(QtForOhos) << "Cannot convert to QOhosQtKeyEvent - key action unknown";
        return {};
    }

    const auto qtModifiers = QtKeyEventHelpers::convertOhosToQtKeyboardModifiersWithNumpad(
        m_keysFlags, isKeyFromNumPad());
    const auto qtKeyAction = convertKeyEventType();
    const auto qtKeyCode = convertKeyCode(qtModifiers);
    const auto qGuiApplicationKeyboardModifiers = QtKeyEventHelpers::convertCurrentOhosKeyModifiersForQGuiApplication(
        qtModifiers, m_keysFlags, qtKeyCode);
    const auto qtKeyText = QtKeyEventHelpers::tryConvertQtKeyCodeToKeyText(qtKeyCode, qtModifiers, m_keysFlags);

    return QOhosQtKeyEvent{
        qtKeyAction, qtKeyCode, qtKeyText, qtModifiers, qGuiApplicationKeyboardModifiers,
        static_cast<quint32>(m_keyCode)};
}

bool QOhosNativeNodeKeyEvent::equals(const QOhosKeyEvent &other) const
{
    if (typeid(other) != typeid(QOhosNativeNodeKeyEvent))
        return false;

    const auto &otherNativeNodeKeyEvent = static_cast<const QOhosNativeNodeKeyEvent &>(other);

    return m_keyEventType == otherNativeNodeKeyEvent.m_keyEventType
        && m_keyCode == otherNativeNodeKeyEvent.m_keyCode
        && m_keysFlags == otherNativeNodeKeyEvent.m_keysFlags;
}

QEvent::Type QOhosNativeNodeKeyEvent::convertKeyEventType() const
{
    return m_keyEventType == ::ArkUI_KeyEventType::ARKUI_KEY_EVENT_DOWN
        ? QEvent::KeyPress
        : m_keyEventType == ::ArkUI_KeyEventType::ARKUI_KEY_EVENT_UP
            ? QEvent::KeyRelease
            : QEvent::None;
}

bool QOhosNativeNodeKeyEvent::isKeyFromNumPad() const
{
    switch (m_keyCode) {
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_0:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_1:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_2:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_3:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_4:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_5:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_6:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_7:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_8:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_9:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_DIVIDE:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_MULTIPLY:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_SUBTRACT:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_ADD:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_DOT:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_COMMA:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_ENTER:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_EQUALS:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_LEFT_PAREN:
    case ::ArkUI_KeyCode::ARKUI_KEYCODE_NUMPAD_RIGHT_PAREN:
        return true;
    default:
        return false;
    }
}

Qt::Key QOhosNativeNodeKeyEvent::convertKeyCode(Qt::KeyboardModifiers qtModifiers) const
{
    if (QtKeyEventHelpers::isOnlyShiftModificatorPressed(qtModifiers)
        && ohosNativeNodeQtKeyShiftPressedMap.contains(m_keyCode)) {
        return ohosNativeNodeQtKeyShiftPressedMap.value(m_keyCode);
    }

    const bool keyNumLockFlagOn = m_keysFlags.testFlag(OhosKeyboardModifier::NUM_LOCK);
    if ((!keyNumLockFlagOn || QtKeyEventHelpers::isOnlyShiftModificatorPressed(qtModifiers))
        && ohosNativeNodeNumpadSpecialQtKeyMap.contains(m_keyCode)) {
        return ohosNativeNodeNumpadSpecialQtKeyMap.value(m_keyCode);
    }

    if (ohosNativeNodeQtKeyMap.contains(m_keyCode))
        return ohosNativeNodeQtKeyMap.value(m_keyCode);

    qOhosWarning(QtForOhos) << "Cannot find mapping for ::ArkUI_KeyCode =" << m_keyCode;
    return Qt::Key_unknown;
}

}

std::shared_ptr<QOhosKeyEvent> makeQOhosNativeNodeKeyEvent(
    ::ArkUI_KeyEventType keyEventType, ::ArkUI_KeyCode keyCode, QFlags<OhosKeyboardModifier> keysFlags)
{
    return std::make_shared<QOhosNativeNodeKeyEvent>(keyEventType, keyCode, keysFlags);
}

std::shared_ptr<QOhosKeyEvent> makeEmptyQOhosNativeNodeKeyEvent()
{
    return makeQOhosNativeNodeKeyEvent(
        ::ArkUI_KeyEventType::ARKUI_KEY_EVENT_UNKNOWN, ::ArkUI_KeyCode::ARKUI_KEYCODE_UNKNOWN, {});
}

QT_END_NAMESPACE
