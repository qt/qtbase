// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosxcomponentkeyevent.h>

#include <QtCore/qmap.h>
#include <optional>

QT_BEGIN_NAMESPACE

namespace
{

const QMap<OH_NativeXComponent_KeyCode, Qt::Key> ohosXComponentNumpadSpecialQtKeyMap = {
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_0, Qt::Key_Insert},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_1, Qt::Key_End},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_2, Qt::Key_Down},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_3, Qt::Key_PageDown},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_4, Qt::Key_Left},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_5, Qt::Key_Clear},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_6, Qt::Key_Right},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_7, Qt::Key_Home},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_8, Qt::Key_Up},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_9, Qt::Key_PageUp},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_COMMA, Qt::Key_Delete},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_DOT, Qt::Key_Delete},
};

const QMap<OH_NativeXComponent_KeyCode, Qt::Key> ohosXComponentQtKeyMap = {
    {OH_NativeXComponent_KeyCode::KEY_HOME, Qt::Key_Home},
    {OH_NativeXComponent_KeyCode::KEY_BACK, Qt::Key_Back},
    {OH_NativeXComponent_KeyCode::KEY_MEDIA_PLAY_PAUSE, Qt::Key_MediaTogglePlayPause},
    {OH_NativeXComponent_KeyCode::KEY_MEDIA_STOP, Qt::Key_MediaStop},
    {OH_NativeXComponent_KeyCode::KEY_MEDIA_NEXT, Qt::Key_MediaNext},
    {OH_NativeXComponent_KeyCode::KEY_MEDIA_PREVIOUS, Qt::Key_MediaPrevious},
    {OH_NativeXComponent_KeyCode::KEY_MEDIA_REWIND, Qt::Key_AudioRewind},
    {OH_NativeXComponent_KeyCode::KEY_VOLUME_UP, Qt::Key_VolumeUp},
    {OH_NativeXComponent_KeyCode::KEY_VOLUME_DOWN, Qt::Key_VolumeDown},
    {OH_NativeXComponent_KeyCode::KEY_POWER, Qt::Key_PowerDown},
    {OH_NativeXComponent_KeyCode::KEY_CAMERA, Qt::Key_Camera},
    {OH_NativeXComponent_KeyCode::KEY_VOLUME_MUTE, Qt::Key_VolumeMute},
    {OH_NativeXComponent_KeyCode::KEY_MUTE, Qt::Key_VolumeMute},
    {OH_NativeXComponent_KeyCode::KEY_BRIGHTNESS_UP, Qt::Key_MonBrightnessUp},
    {OH_NativeXComponent_KeyCode::KEY_BRIGHTNESS_DOWN, Qt::Key_MonBrightnessDown},
    {OH_NativeXComponent_KeyCode::KEY_0, Qt::Key_0},
    {OH_NativeXComponent_KeyCode::KEY_1, Qt::Key_1},
    {OH_NativeXComponent_KeyCode::KEY_2, Qt::Key_2},
    {OH_NativeXComponent_KeyCode::KEY_3, Qt::Key_3},
    {OH_NativeXComponent_KeyCode::KEY_4, Qt::Key_4},
    {OH_NativeXComponent_KeyCode::KEY_5, Qt::Key_5},
    {OH_NativeXComponent_KeyCode::KEY_6, Qt::Key_6},
    {OH_NativeXComponent_KeyCode::KEY_7, Qt::Key_7},
    {OH_NativeXComponent_KeyCode::KEY_8, Qt::Key_8},
    {OH_NativeXComponent_KeyCode::KEY_9, Qt::Key_9},
    {OH_NativeXComponent_KeyCode::KEY_DPAD_UP, Qt::Key_Up},
    {OH_NativeXComponent_KeyCode::KEY_DPAD_DOWN, Qt::Key_Down},
    {OH_NativeXComponent_KeyCode::KEY_DPAD_LEFT, Qt::Key_Left},
    {OH_NativeXComponent_KeyCode::KEY_DPAD_RIGHT, Qt::Key_Right},
    {OH_NativeXComponent_KeyCode::KEY_DPAD_CENTER, Qt::Key_Enter},
    {OH_NativeXComponent_KeyCode::KEY_A, Qt::Key_A},
    {OH_NativeXComponent_KeyCode::KEY_B, Qt::Key_B},
    {OH_NativeXComponent_KeyCode::KEY_C, Qt::Key_C},
    {OH_NativeXComponent_KeyCode::KEY_D, Qt::Key_D},
    {OH_NativeXComponent_KeyCode::KEY_E, Qt::Key_E},
    {OH_NativeXComponent_KeyCode::KEY_F, Qt::Key_F},
    {OH_NativeXComponent_KeyCode::KEY_G, Qt::Key_G},
    {OH_NativeXComponent_KeyCode::KEY_H, Qt::Key_H},
    {OH_NativeXComponent_KeyCode::KEY_I, Qt::Key_I},
    {OH_NativeXComponent_KeyCode::KEY_J, Qt::Key_J},
    {OH_NativeXComponent_KeyCode::KEY_K, Qt::Key_K},
    {OH_NativeXComponent_KeyCode::KEY_L, Qt::Key_L},
    {OH_NativeXComponent_KeyCode::KEY_M, Qt::Key_M},
    {OH_NativeXComponent_KeyCode::KEY_N, Qt::Key_N},
    {OH_NativeXComponent_KeyCode::KEY_O, Qt::Key_O},
    {OH_NativeXComponent_KeyCode::KEY_P, Qt::Key_P},
    {OH_NativeXComponent_KeyCode::KEY_Q, Qt::Key_Q},
    {OH_NativeXComponent_KeyCode::KEY_R, Qt::Key_R},
    {OH_NativeXComponent_KeyCode::KEY_S, Qt::Key_S},
    {OH_NativeXComponent_KeyCode::KEY_T, Qt::Key_T},
    {OH_NativeXComponent_KeyCode::KEY_U, Qt::Key_U},
    {OH_NativeXComponent_KeyCode::KEY_V, Qt::Key_V},
    {OH_NativeXComponent_KeyCode::KEY_W, Qt::Key_W},
    {OH_NativeXComponent_KeyCode::KEY_X, Qt::Key_X},
    {OH_NativeXComponent_KeyCode::KEY_Y, Qt::Key_Y},
    {OH_NativeXComponent_KeyCode::KEY_Z, Qt::Key_Z},
    {OH_NativeXComponent_KeyCode::KEY_COMMA, Qt::Key_Comma},
    {OH_NativeXComponent_KeyCode::KEY_PERIOD, Qt::Key_Period},
    {OH_NativeXComponent_KeyCode::KEY_ALT_LEFT, Qt::Key_Alt},
    {OH_NativeXComponent_KeyCode::KEY_ALT_RIGHT, Qt::Key_Alt},
    {OH_NativeXComponent_KeyCode::KEY_SHIFT_LEFT, Qt::Key_Shift},
    {OH_NativeXComponent_KeyCode::KEY_SHIFT_RIGHT, Qt::Key_Shift},
    {OH_NativeXComponent_KeyCode::KEY_TAB, Qt::Key_Tab},
    {OH_NativeXComponent_KeyCode::KEY_SPACE, Qt::Key_Space},
    {OH_NativeXComponent_KeyCode::KEY_EXPLORER, Qt::Key_Explorer},
    {OH_NativeXComponent_KeyCode::KEY_ENVELOPE, Qt::Key_LaunchMail},
    {OH_NativeXComponent_KeyCode::KEY_ENTER, Qt::Key_Return},
    {OH_NativeXComponent_KeyCode::KEY_DEL, Qt::Key_Backspace},
    {OH_NativeXComponent_KeyCode::KEY_GRAVE, Qt::Key_QuoteLeft},
    {OH_NativeXComponent_KeyCode::KEY_MINUS, Qt::Key_Minus},
    {OH_NativeXComponent_KeyCode::KEY_EQUALS, Qt::Key_Equal},
    {OH_NativeXComponent_KeyCode::KEY_LEFT_BRACKET, Qt::Key_BracketLeft},
    {OH_NativeXComponent_KeyCode::KEY_RIGHT_BRACKET, Qt::Key_BracketRight},
    {OH_NativeXComponent_KeyCode::KEY_BACKSLASH, Qt::Key_Backslash},
    {OH_NativeXComponent_KeyCode::KEY_SEMICOLON, Qt::Key_Semicolon},
    {OH_NativeXComponent_KeyCode::KEY_APOSTROPHE, Qt::Key_Apostrophe},
    {OH_NativeXComponent_KeyCode::KEY_SLASH, Qt::Key_Slash},
    {OH_NativeXComponent_KeyCode::KEY_AT, Qt::Key_At},
    {OH_NativeXComponent_KeyCode::KEY_PLUS, Qt::Key_Plus},
    {OH_NativeXComponent_KeyCode::KEY_MENU, Qt::Key_Menu},
    {OH_NativeXComponent_KeyCode::KEY_PAGE_UP, Qt::Key_PageUp},
    {OH_NativeXComponent_KeyCode::KEY_PAGE_DOWN, Qt::Key_PageDown},
    {OH_NativeXComponent_KeyCode::KEY_ESCAPE, Qt::Key_Escape},
    {OH_NativeXComponent_KeyCode::KEY_FORWARD_DEL, Qt::Key_Delete},
    {OH_NativeXComponent_KeyCode::KEY_CTRL_LEFT, Qt::Key_Control},
    {OH_NativeXComponent_KeyCode::KEY_CTRL_RIGHT, Qt::Key_Control},
    {OH_NativeXComponent_KeyCode::KEY_CAPS_LOCK, Qt::Key_CapsLock},
    {OH_NativeXComponent_KeyCode::KEY_SCROLL_LOCK, Qt::Key_ScrollLock},
    {OH_NativeXComponent_KeyCode::KEY_META_LEFT, Qt::Key_Meta},
    {OH_NativeXComponent_KeyCode::KEY_META_RIGHT, Qt::Key_Meta},
    {OH_NativeXComponent_KeyCode::KEY_SYSRQ, Qt::Key_SysReq},
    {OH_NativeXComponent_KeyCode::KEY_MOVE_HOME, Qt::Key_Home},
    {OH_NativeXComponent_KeyCode::KEY_MOVE_END, Qt::Key_End},
    {OH_NativeXComponent_KeyCode::KEY_INSERT, Qt::Key_Insert},
    {OH_NativeXComponent_KeyCode::KEY_FORWARD, Qt::Key_Forward},
    {OH_NativeXComponent_KeyCode::KEY_MEDIA_PLAY, Qt::Key_MediaPlay},
    {OH_NativeXComponent_KeyCode::KEY_MEDIA_PAUSE, Qt::Key_MediaPause},
    {OH_NativeXComponent_KeyCode::KEY_MEDIA_EJECT, Qt::Key_Eject},
    {OH_NativeXComponent_KeyCode::KEY_MEDIA_RECORD, Qt::Key_MediaRecord},
    {OH_NativeXComponent_KeyCode::KEY_F1, Qt::Key_F1},
    {OH_NativeXComponent_KeyCode::KEY_F2, Qt::Key_F2},
    {OH_NativeXComponent_KeyCode::KEY_F3, Qt::Key_F3},
    {OH_NativeXComponent_KeyCode::KEY_F4, Qt::Key_F4},
    {OH_NativeXComponent_KeyCode::KEY_F5, Qt::Key_F5},
    {OH_NativeXComponent_KeyCode::KEY_F6, Qt::Key_F6},
    {OH_NativeXComponent_KeyCode::KEY_F7, Qt::Key_F7},
    {OH_NativeXComponent_KeyCode::KEY_F8, Qt::Key_F8},
    {OH_NativeXComponent_KeyCode::KEY_F9, Qt::Key_F9},
    {OH_NativeXComponent_KeyCode::KEY_F10, Qt::Key_F10},
    {OH_NativeXComponent_KeyCode::KEY_F11, Qt::Key_F11},
    {OH_NativeXComponent_KeyCode::KEY_F12, Qt::Key_F12},
    {OH_NativeXComponent_KeyCode::KEY_NUM_LOCK, Qt::Key_NumLock},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_0, Qt::Key_0},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_1, Qt::Key_1},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_2, Qt::Key_2},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_3, Qt::Key_3},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_4, Qt::Key_4},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_5, Qt::Key_5},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_6, Qt::Key_6},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_7, Qt::Key_7},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_8, Qt::Key_8},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_9, Qt::Key_9},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_DIVIDE, Qt::Key_Slash},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_MULTIPLY, Qt::Key_Asterisk},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_SUBTRACT, Qt::Key_Minus},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_ADD, Qt::Key_Plus},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_DOT, Qt::Key_Period},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_COMMA, Qt::Key_Comma},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_ENTER, Qt::Key_Enter},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_EQUALS, Qt::Key_Equal},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_LEFT_PAREN, Qt::Key_ParenLeft},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_RIGHT_PAREN, Qt::Key_ParenRight},
    {OH_NativeXComponent_KeyCode::KEY_SLEEP, Qt::Key_Sleep},
    {OH_NativeXComponent_KeyCode::KEY_ZENKAKU_HANKAKU, Qt::Key_Zenkaku_Hankaku},
    {OH_NativeXComponent_KeyCode::KEY_KATAKANA, Qt::Key_Katakana},
    {OH_NativeXComponent_KeyCode::KEY_HIRAGANA, Qt::Key_Hiragana},
    {OH_NativeXComponent_KeyCode::KEY_HENKAN, Qt::Key_Henkan},
    {OH_NativeXComponent_KeyCode::KEY_KATAKANA_HIRAGANA, Qt::Key_Hiragana_Katakana},
    {OH_NativeXComponent_KeyCode::KEY_MUHENKAN, Qt::Key_Muhenkan},
    {OH_NativeXComponent_KeyCode::KEY_NUMPAD_PLUSMINUS, Qt::Key_plusminus},
    {OH_NativeXComponent_KeyCode::KEY_YEN, Qt::Key_yen},
    {OH_NativeXComponent_KeyCode::KEY_STOP, Qt::Key_Stop},
    {OH_NativeXComponent_KeyCode::KEY_UNDO, Qt::Key_Undo},
    {OH_NativeXComponent_KeyCode::KEY_COPY, Qt::Key_Copy},
    {OH_NativeXComponent_KeyCode::KEY_OPEN, Qt::Key_Open},
    {OH_NativeXComponent_KeyCode::KEY_PASTE, Qt::Key_Paste},
    {OH_NativeXComponent_KeyCode::KEY_FIND, Qt::Key_Find},
    {OH_NativeXComponent_KeyCode::KEY_CUT, Qt::Key_Cut},
    {OH_NativeXComponent_KeyCode::KEY_HELP, Qt::Key_Help},
    {OH_NativeXComponent_KeyCode::KEY_CALC, Qt::Key_Calculator},
    {OH_NativeXComponent_KeyCode::KEY_BOOKMARKS, Qt::Key_Book},
    {OH_NativeXComponent_KeyCode::KEY_NEXT, Qt::Key_MediaNext},
    {OH_NativeXComponent_KeyCode::KEY_PLAYPAUSE, Qt::Key_MediaTogglePlayPause},
    {OH_NativeXComponent_KeyCode::KEY_PREVIOUS, Qt::Key_MediaPrevious},
    {OH_NativeXComponent_KeyCode::KEY_STOPCD, Qt::Key_Stop},
    {OH_NativeXComponent_KeyCode::KEY_REFRESH, Qt::Key_Refresh},
    {OH_NativeXComponent_KeyCode::KEY_EXIT, Qt::Key_Exit},
    {OH_NativeXComponent_KeyCode::KEY_NEW, Qt::Key_New},
    {OH_NativeXComponent_KeyCode::KEY_REDO, Qt::Key_Redo},
    {OH_NativeXComponent_KeyCode::KEY_CLOSE, Qt::Key_Close},
    {OH_NativeXComponent_KeyCode::KEY_PLAY, Qt::Key_Play},
    {OH_NativeXComponent_KeyCode::KEY_BASSBOOST, Qt::Key_BassBoost},
    {OH_NativeXComponent_KeyCode::KEY_PRINT, Qt::Key_Print},
    {OH_NativeXComponent_KeyCode::KEY_FINANCE, Qt::Key_Finance},
    {OH_NativeXComponent_KeyCode::KEY_CANCEL, Qt::Key_Cancel},
    {OH_NativeXComponent_KeyCode::KEY_SEND, Qt::Key_Send},
    {OH_NativeXComponent_KeyCode::KEY_REPLY, Qt::Key_Reply},
    {OH_NativeXComponent_KeyCode::KEY_FORWARDMAIL, Qt::Key_MailForward},
    {OH_NativeXComponent_KeyCode::KEY_SAVE, Qt::Key_Save},
    {OH_NativeXComponent_KeyCode::KEY_DOCUMENTS, Qt::Key_Documents},
    {OH_NativeXComponent_KeyCode::KEY_VIDEO_NEXT, Qt::Key_MediaNext},
    {OH_NativeXComponent_KeyCode::KEY_VIDEO_PREV, Qt::Key_MediaPrevious},
    {OH_NativeXComponent_KeyCode::KEY_INFO, Qt::Key_Info},
    {OH_NativeXComponent_KeyCode::KEY_SUBTITLE, Qt::Key_Subtitle},
    {OH_NativeXComponent_KeyCode::KEY_CD, Qt::Key_CD},
    {OH_NativeXComponent_KeyCode::KEY_VIDEO, Qt::Key_Video},
    {OH_NativeXComponent_KeyCode::KEY_MEMO, Qt::Key_Memo},
    {OH_NativeXComponent_KeyCode::KEY_CALENDAR, Qt::Key_Calendar},
    {OH_NativeXComponent_KeyCode::KEY_RED, Qt::Key_Red},
    {OH_NativeXComponent_KeyCode::KEY_GREEN, Qt::Key_Green},
    {OH_NativeXComponent_KeyCode::KEY_YELLOW, Qt::Key_Yellow},
    {OH_NativeXComponent_KeyCode::KEY_BLUE, Qt::Key_Blue},
    {OH_NativeXComponent_KeyCode::KEY_CHANNELUP, Qt::Key_ChannelUp},
    {OH_NativeXComponent_KeyCode::KEY_CHANNELDOWN, Qt::Key_ChannelDown},
    {OH_NativeXComponent_KeyCode::KEY_ZOOMIN, Qt::Key_ZoomIn},
    {OH_NativeXComponent_KeyCode::KEY_ZOOMOUT, Qt::Key_ZoomOut},
    {OH_NativeXComponent_KeyCode::KEY_NEWS, Qt::Key_News},
    {OH_NativeXComponent_KeyCode::KEY_MESSENGER, Qt::Key_Messenger},
    {OH_NativeXComponent_KeyCode::KEY_SCREENSAVER, Qt::Key_ScreenSaver},
    {OH_NativeXComponent_KeyCode::KEY_WAKEUP, Qt::Key_WakeUp},
    {OH_NativeXComponent_KeyCode::KEY_XFER, Qt::Key_Xfer},
    {OH_NativeXComponent_KeyCode::KEY_F13, Qt::Key_F13},
    {OH_NativeXComponent_KeyCode::KEY_F14, Qt::Key_F14},
    {OH_NativeXComponent_KeyCode::KEY_F15, Qt::Key_F15},
    {OH_NativeXComponent_KeyCode::KEY_F16, Qt::Key_F16},
    {OH_NativeXComponent_KeyCode::KEY_F17, Qt::Key_F17},
    {OH_NativeXComponent_KeyCode::KEY_F18, Qt::Key_F18},
    {OH_NativeXComponent_KeyCode::KEY_F19, Qt::Key_F19},
    {OH_NativeXComponent_KeyCode::KEY_F20, Qt::Key_F20},
    {OH_NativeXComponent_KeyCode::KEY_F21, Qt::Key_F21},
    {OH_NativeXComponent_KeyCode::KEY_F22, Qt::Key_F22},
    {OH_NativeXComponent_KeyCode::KEY_F23, Qt::Key_F23},
    {OH_NativeXComponent_KeyCode::KEY_F24, Qt::Key_F24},
    {OH_NativeXComponent_KeyCode::KEY_SUSPEND, Qt::Key_Suspend},
    {OH_NativeXComponent_KeyCode::KEY_QUESTION, Qt::Key_Question},
    {OH_NativeXComponent_KeyCode::KEY_SHOP, Qt::Key_Shop},
    {OH_NativeXComponent_KeyCode::KEY_BATTERY, Qt::Key_Battery},
    {OH_NativeXComponent_KeyCode::KEY_BLUETOOTH, Qt::Key_Bluetooth},
    {OH_NativeXComponent_KeyCode::KEY_WLAN, Qt::Key_WLAN},
    {OH_NativeXComponent_KeyCode::KEY_UWB, Qt::Key_UWB},
};

const QMap<OH_NativeXComponent_KeyCode, Qt::Key> ohosXComponentQtKeyShiftPressedMap = {
    {OH_NativeXComponent_KeyCode::KEY_0, Qt::Key_ParenRight},
    {OH_NativeXComponent_KeyCode::KEY_1, Qt::Key_Exclam},
    {OH_NativeXComponent_KeyCode::KEY_2, Qt::Key_At},
    {OH_NativeXComponent_KeyCode::KEY_3, Qt::Key_NumberSign},
    {OH_NativeXComponent_KeyCode::KEY_4, Qt::Key_Dollar},
    {OH_NativeXComponent_KeyCode::KEY_5, Qt::Key_Percent},
    {OH_NativeXComponent_KeyCode::KEY_6, Qt::Key_AsciiCircum},
    {OH_NativeXComponent_KeyCode::KEY_7, Qt::Key_Ampersand},
    {OH_NativeXComponent_KeyCode::KEY_8, Qt::Key_Asterisk},
    {OH_NativeXComponent_KeyCode::KEY_9, Qt::Key_ParenLeft},
    {OH_NativeXComponent_KeyCode::KEY_LEFT_BRACKET, Qt::Key_BraceLeft},
    {OH_NativeXComponent_KeyCode::KEY_RIGHT_BRACKET, Qt::Key_BraceRight},
    {OH_NativeXComponent_KeyCode::KEY_BACKSLASH, Qt::Key_Bar},
    {OH_NativeXComponent_KeyCode::KEY_SEMICOLON, Qt::Key_Colon},
    {OH_NativeXComponent_KeyCode::KEY_APOSTROPHE, Qt::Key_QuoteDbl},
    {OH_NativeXComponent_KeyCode::KEY_SLASH, Qt::Key_Question},
    {OH_NativeXComponent_KeyCode::KEY_COMMA, Qt::Key_Less},
    {OH_NativeXComponent_KeyCode::KEY_PERIOD, Qt::Key_Greater},
    {OH_NativeXComponent_KeyCode::KEY_MINUS, Qt::Key_Underscore},
    {OH_NativeXComponent_KeyCode::KEY_EQUALS, Qt::Key_Plus},
    {OH_NativeXComponent_KeyCode::KEY_GRAVE, Qt::Key_AsciiTilde},
};

class QOhosXComponentKeyEvent final : public QOhosKeyEvent
{
public:
    QOhosXComponentKeyEvent(
        ::OH_NativeXComponent_KeyAction keyAction,
        ::OH_NativeXComponent_KeyCode keyCode,
        QFlags<OhosKeyboardModifier> keysFlags);
    ~QOhosXComponentKeyEvent() override;

    std::optional<QOhosQtKeyEvent> tryConvertToQOhosQtKeyEvent() const override;
    bool equals(const QOhosKeyEvent &other) const override;

private:
    QEvent::Type convertKeyAction() const;
    bool isKeyFromNumPad() const;
    Qt::Key convertKeyCode(Qt::KeyboardModifiers qtModifiers) const;

    OH_NativeXComponent_KeyAction m_keyAction;
    OH_NativeXComponent_KeyCode m_keyCode;
    QFlags<OhosKeyboardModifier> m_keysFlags;
};

QOhosXComponentKeyEvent::QOhosXComponentKeyEvent(
    ::OH_NativeXComponent_KeyAction keyAction,
    ::OH_NativeXComponent_KeyCode keyCode,
    QFlags<OhosKeyboardModifier> keysFlags)
    : m_keyAction(keyAction), m_keyCode(keyCode), m_keysFlags(keysFlags)
{
}

QOhosXComponentKeyEvent::~QOhosXComponentKeyEvent() = default;

std::optional<QOhosQtKeyEvent> QOhosXComponentKeyEvent::tryConvertToQOhosQtKeyEvent() const
{
    const auto qtModifiers = QtKeyEventHelpers::convertOhosToQtKeyboardModifiersWithNumpad(
        m_keysFlags, isKeyFromNumPad());
    const auto qtKeyAction = convertKeyAction();
    const auto qtKeyCode = convertKeyCode(qtModifiers);
    const auto qGuiApplicationKeyboardModifiers = QtKeyEventHelpers::convertCurrentOhosKeyModifiersForQGuiApplication(
        qtModifiers, m_keysFlags, qtKeyCode);
    const auto qtKeyText = QtKeyEventHelpers::tryConvertQtKeyCodeToKeyText(qtKeyCode, qtModifiers, m_keysFlags);

    return QOhosQtKeyEvent{
        qtKeyAction, qtKeyCode, qtKeyText, qtModifiers, qGuiApplicationKeyboardModifiers,
        static_cast<quint32>(m_keyCode)};
}

bool QOhosXComponentKeyEvent::equals(const QOhosKeyEvent &other) const
{
    if (typeid(other) != typeid(QOhosXComponentKeyEvent))
        return false;

    const auto &otherXComponentKeyEvent = static_cast<const QOhosXComponentKeyEvent &>(other);

    return m_keyAction == otherXComponentKeyEvent.m_keyAction
        && m_keyCode == otherXComponentKeyEvent.m_keyCode
        && m_keysFlags == otherXComponentKeyEvent.m_keysFlags;
}

QEvent::Type QOhosXComponentKeyEvent::convertKeyAction() const
{
    return m_keyAction == OH_NativeXComponent_KeyAction::OH_NATIVEXCOMPONENT_KEY_ACTION_DOWN
        ? QEvent::KeyPress
        : QEvent::KeyRelease;
}

bool QOhosXComponentKeyEvent::isKeyFromNumPad() const
{
    switch (m_keyCode) {
    case KEY_NUMPAD_0:
    case KEY_NUMPAD_1:
    case KEY_NUMPAD_2:
    case KEY_NUMPAD_3:
    case KEY_NUMPAD_4:
    case KEY_NUMPAD_5:
    case KEY_NUMPAD_6:
    case KEY_NUMPAD_7:
    case KEY_NUMPAD_8:
    case KEY_NUMPAD_9:
    case KEY_NUMPAD_DIVIDE:
    case KEY_NUMPAD_MULTIPLY:
    case KEY_NUMPAD_SUBTRACT:
    case KEY_NUMPAD_ADD:
    case KEY_NUMPAD_DOT:
    case KEY_NUMPAD_COMMA:
    case KEY_NUMPAD_ENTER:
    case KEY_NUMPAD_EQUALS:
    case KEY_NUMPAD_LEFT_PAREN:
    case KEY_NUMPAD_RIGHT_PAREN:
    case KEY_NUMPAD_PLUSMINUS:
        return true;
    default:
        return false;
    }
}

Qt::Key QOhosXComponentKeyEvent::convertKeyCode(Qt::KeyboardModifiers qtModifiers) const
{
    const bool keyNumLockFlagOn = m_keysFlags.testFlag(OhosKeyboardModifier::NUM_LOCK);

    if (QtKeyEventHelpers::isOnlyShiftModificatorPressed(qtModifiers)
        && ohosXComponentQtKeyShiftPressedMap.contains(m_keyCode)) {
        return ohosXComponentQtKeyShiftPressedMap.value(m_keyCode);
    }

    if ((!keyNumLockFlagOn || QtKeyEventHelpers::isOnlyShiftModificatorPressed(qtModifiers))
        && ohosXComponentNumpadSpecialQtKeyMap.contains(m_keyCode)) {
        return ohosXComponentNumpadSpecialQtKeyMap.value(m_keyCode);
    }

    if (ohosXComponentQtKeyMap.contains(m_keyCode))
        return ohosXComponentQtKeyMap.value(m_keyCode);

    qOhosWarning(QtForOhos) << "Cannot find mapping for OH_NativeXComponent_KeyCode =" << m_keyCode;
    return Qt::Key_unknown;
}

}

std::shared_ptr<QOhosKeyEvent> makeQOhosXComponentKeyEvent(
    ::OH_NativeXComponent_KeyAction keyAction, ::OH_NativeXComponent_KeyCode keyCode,
    QFlags<OhosKeyboardModifier> keysFlags)
{
    return std::make_shared<QOhosXComponentKeyEvent>(keyAction, keyCode, keysFlags);
}

QT_END_NAMESPACE
