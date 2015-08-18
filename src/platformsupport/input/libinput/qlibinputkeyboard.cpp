/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlibinputkeyboard_p.h"
#include <QtCore/QTextCodec>
#include <QtCore/QLoggingCategory>
#include <qpa/qwindowsysteminterface.h>
#include <libinput.h>
#ifndef QT_NO_XKBCOMMON_EVDEV
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon-names.h>
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcLibInput)

const int REPEAT_DELAY = 500;
const int REPEAT_RATE = 100;

#ifndef QT_NO_XKBCOMMON_EVDEV
struct KeyTabEntry {
    int xkbkey;
    int qtkey;
};

static inline bool operator==(const KeyTabEntry &a, const KeyTabEntry &b)
{
    return a.xkbkey == b.xkbkey;
}

static const KeyTabEntry keyTab[] = {
    { XKB_KEY_Escape,             Qt::Key_Escape },
    { XKB_KEY_Tab,                Qt::Key_Tab },
    { XKB_KEY_ISO_Left_Tab,       Qt::Key_Backtab },
    { XKB_KEY_BackSpace,          Qt::Key_Backspace },
    { XKB_KEY_Return,             Qt::Key_Return },
    { XKB_KEY_Insert,             Qt::Key_Insert },
    { XKB_KEY_Delete,             Qt::Key_Delete },
    { XKB_KEY_Clear,              Qt::Key_Delete },
    { XKB_KEY_Pause,              Qt::Key_Pause },
    { XKB_KEY_Print,              Qt::Key_Print },

    { XKB_KEY_Home,               Qt::Key_Home },
    { XKB_KEY_End,                Qt::Key_End },
    { XKB_KEY_Left,               Qt::Key_Left },
    { XKB_KEY_Up,                 Qt::Key_Up },
    { XKB_KEY_Right,              Qt::Key_Right },
    { XKB_KEY_Down,               Qt::Key_Down },
    { XKB_KEY_Prior,              Qt::Key_PageUp },
    { XKB_KEY_Next,               Qt::Key_PageDown },

    { XKB_KEY_Shift_L,            Qt::Key_Shift },
    { XKB_KEY_Shift_R,            Qt::Key_Shift },
    { XKB_KEY_Shift_Lock,         Qt::Key_Shift },
    { XKB_KEY_Control_L,          Qt::Key_Control },
    { XKB_KEY_Control_R,          Qt::Key_Control },
    { XKB_KEY_Meta_L,             Qt::Key_Meta },
    { XKB_KEY_Meta_R,             Qt::Key_Meta },
    { XKB_KEY_Alt_L,              Qt::Key_Alt },
    { XKB_KEY_Alt_R,              Qt::Key_Alt },
    { XKB_KEY_Caps_Lock,          Qt::Key_CapsLock },
    { XKB_KEY_Num_Lock,           Qt::Key_NumLock },
    { XKB_KEY_Scroll_Lock,        Qt::Key_ScrollLock },
    { XKB_KEY_Super_L,            Qt::Key_Super_L },
    { XKB_KEY_Super_R,            Qt::Key_Super_R },
    { XKB_KEY_Menu,               Qt::Key_Menu },
    { XKB_KEY_Hyper_L,            Qt::Key_Hyper_L },
    { XKB_KEY_Hyper_R,            Qt::Key_Hyper_R },
    { XKB_KEY_Help,               Qt::Key_Help },

    { XKB_KEY_KP_Space,           Qt::Key_Space },
    { XKB_KEY_KP_Tab,             Qt::Key_Tab },
    { XKB_KEY_KP_Enter,           Qt::Key_Enter },
    { XKB_KEY_KP_Home,            Qt::Key_Home },
    { XKB_KEY_KP_Left,            Qt::Key_Left },
    { XKB_KEY_KP_Up,              Qt::Key_Up },
    { XKB_KEY_KP_Right,           Qt::Key_Right },
    { XKB_KEY_KP_Down,            Qt::Key_Down },
    { XKB_KEY_KP_Prior,           Qt::Key_PageUp },
    { XKB_KEY_KP_Next,            Qt::Key_PageDown },
    { XKB_KEY_KP_End,             Qt::Key_End },
    { XKB_KEY_KP_Begin,           Qt::Key_Clear },
    { XKB_KEY_KP_Insert,          Qt::Key_Insert },
    { XKB_KEY_KP_Delete,          Qt::Key_Delete },
    { XKB_KEY_KP_Equal,           Qt::Key_Equal },
    { XKB_KEY_KP_Multiply,        Qt::Key_Asterisk },
    { XKB_KEY_KP_Add,             Qt::Key_Plus },
    { XKB_KEY_KP_Separator,       Qt::Key_Comma },
    { XKB_KEY_KP_Subtract,        Qt::Key_Minus },
    { XKB_KEY_KP_Decimal,         Qt::Key_Period },
    { XKB_KEY_KP_Divide,          Qt::Key_Slash },
};
#endif

QLibInputKeyboard::QLibInputKeyboard()
#ifndef QT_NO_XKBCOMMON_EVDEV
    : m_ctx(0),
      m_keymap(0),
      m_state(0)
#endif
{
#ifndef QT_NO_XKBCOMMON_EVDEV
    qCDebug(qLcLibInput) << "Using xkbcommon for key mapping";
    m_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!m_ctx) {
        qWarning("Failed to create xkb context");
        return;
    }
    m_keymap = xkb_keymap_new_from_names(m_ctx, Q_NULLPTR, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!m_keymap) {
        qWarning("Failed to compile keymap");
        return;
    }
    m_state = xkb_state_new(m_keymap);
    if (!m_state) {
        qWarning("Failed to create xkb state");
        return;
    }
    m_modindex[0] = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_CTRL);
    m_modindex[1] = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_ALT);
    m_modindex[2] = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_SHIFT);
    m_modindex[3] = xkb_keymap_mod_get_index(m_keymap, XKB_MOD_NAME_LOGO);

    m_repeatTimer.setSingleShot(true);
    connect(&m_repeatTimer, &QTimer::timeout, this, &QLibInputKeyboard::handleRepeat);
#else
    qCWarning(qLcLibInput) << "X-less xkbcommon not available, not performing key mapping";
#endif
}

QLibInputKeyboard::~QLibInputKeyboard()
{
#ifndef QT_NO_XKBCOMMON_EVDEV
    if (m_state)
        xkb_state_unref(m_state);
    if (m_keymap)
        xkb_keymap_unref(m_keymap);
    if (m_ctx)
        xkb_context_unref(m_ctx);
#endif
}

void QLibInputKeyboard::processKey(libinput_event_keyboard *e)
{
#ifndef QT_NO_XKBCOMMON_EVDEV
    if (!m_ctx || !m_keymap || !m_state)
        return;

    const uint32_t k = libinput_event_keyboard_get_key(e) + 8;
    const bool pressed = libinput_event_keyboard_get_key_state(e) == LIBINPUT_KEY_STATE_PRESSED;

    QVarLengthArray<char, 32> chars(32);
    const int size = xkb_state_key_get_utf8(m_state, k, chars.data(), chars.size());
    if (Q_UNLIKELY(size + 1 > chars.size())) { // +1 for NUL
        chars.resize(size + 1);
        xkb_state_key_get_utf8(m_state, k, chars.data(), chars.size());
    }
    const QString text = QString::fromUtf8(chars.constData(), size);

    const xkb_keysym_t sym = xkb_state_key_get_one_sym(m_state, k);

    Qt::KeyboardModifiers mods = Qt::NoModifier;
    const int qtkey = keysymToQtKey(sym, &mods, text);

    xkb_state_component modtype = xkb_state_component(XKB_STATE_MODS_DEPRESSED | XKB_STATE_MODS_LATCHED);
    if (xkb_state_mod_index_is_active(m_state, m_modindex[0], modtype) && (qtkey != Qt::Key_Control || !pressed))
        mods |= Qt::ControlModifier;
    if (xkb_state_mod_index_is_active(m_state, m_modindex[1], modtype) && (qtkey != Qt::Key_Alt || !pressed))
        mods |= Qt::AltModifier;
    if (xkb_state_mod_index_is_active(m_state, m_modindex[2], modtype) && (qtkey != Qt::Key_Shift || !pressed))
        mods |= Qt::ShiftModifier;
    if (xkb_state_mod_index_is_active(m_state, m_modindex[3], modtype) && (qtkey != Qt::Key_Meta || !pressed))
        mods |= Qt::MetaModifier;

    xkb_state_update_key(m_state, k, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);

    QWindowSystemInterface::handleExtendedKeyEvent(Q_NULLPTR,
                                                   pressed ? QEvent::KeyPress : QEvent::KeyRelease,
                                                   qtkey, mods, k, sym, mods, text);

    if (pressed && xkb_keymap_key_repeats(m_keymap, k)) {
        m_repeatData.qtkey = qtkey;
        m_repeatData.mods = mods;
        m_repeatData.nativeScanCode = k;
        m_repeatData.virtualKey = sym;
        m_repeatData.nativeMods = mods;
        m_repeatData.unicodeText = text;
        m_repeatData.repeatCount = 1;
        m_repeatTimer.setInterval(REPEAT_DELAY);
        m_repeatTimer.start();
    } else if (m_repeatTimer.isActive()) {
        m_repeatTimer.stop();
    }

#else
    Q_UNUSED(e);
#endif
}

#ifndef QT_NO_XKBCOMMON_EVDEV
void QLibInputKeyboard::handleRepeat()
{
    QWindowSystemInterface::handleExtendedKeyEvent(Q_NULLPTR, QEvent::KeyPress,
                                                   m_repeatData.qtkey, m_repeatData.mods,
                                                   m_repeatData.nativeScanCode, m_repeatData.virtualKey, m_repeatData.nativeMods,
                                                   m_repeatData.unicodeText, true, m_repeatData.repeatCount);
    m_repeatData.repeatCount += 1;
    m_repeatTimer.setInterval(REPEAT_RATE);
    m_repeatTimer.start();
}

int QLibInputKeyboard::keysymToQtKey(xkb_keysym_t key) const
{
    const size_t elemCount = sizeof(keyTab) / sizeof(KeyTabEntry);
    KeyTabEntry e;
    e.xkbkey = key;
    const KeyTabEntry *result = std::find(keyTab, keyTab + elemCount, e);
    return result != keyTab + elemCount ? result->qtkey : 0;
}

int QLibInputKeyboard::keysymToQtKey(xkb_keysym_t keysym, Qt::KeyboardModifiers *modifiers, const QString &text) const
{
    int code = 0;
#ifndef QT_NO_TEXTCODEC
    QTextCodec *systemCodec = QTextCodec::codecForLocale();
#endif
    if (keysym < 128 || (keysym < 256
#ifndef QT_NO_TEXTCODEC
                         && systemCodec->mibEnum() == 4
#endif
                         )) {
        // upper-case key, if known
        code = isprint((int)keysym) ? toupper((int)keysym) : 0;
    } else if (keysym >= XKB_KEY_F1 && keysym <= XKB_KEY_F35) {
        // function keys
        code = Qt::Key_F1 + ((int)keysym - XKB_KEY_F1);
    } else if (keysym >= XKB_KEY_KP_Space && keysym <= XKB_KEY_KP_9) {
        if (keysym >= XKB_KEY_KP_0) {
            // numeric keypad keys
            code = Qt::Key_0 + ((int)keysym - XKB_KEY_KP_0);
        } else {
            code = keysymToQtKey(keysym);
        }
        *modifiers |= Qt::KeypadModifier;
    } else if (text.length() == 1 && text.unicode()->unicode() > 0x1f
                                  && text.unicode()->unicode() != 0x7f
                                  && !(keysym >= XKB_KEY_dead_grave && keysym <= XKB_KEY_dead_currency)) {
        code = text.unicode()->toUpper().unicode();
    } else {
        // any other keys
        code = keysymToQtKey(keysym);
    }
    return code;
}
#endif

QT_END_NAMESPACE
