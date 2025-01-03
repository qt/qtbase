// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibinputkeyboard_p.h"
#include <QtCore/QLoggingCategory>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qinputdevicemanager_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <libinput.h>
#if QT_CONFIG(xkbcommon)
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon-names.h>
#include <QtGui/private/qxkbcommon_p.h>
#endif

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qLcLibInput)

const int REPEAT_DELAY = 500;
const int REPEAT_RATE = 100;

QLibInputKeyboard::QLibInputKeyboard()
{
#if QT_CONFIG(xkbcommon)
    qCDebug(qLcLibInput) << "Using xkbcommon for key mapping";
    m_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!m_ctx) {
        qWarning("Failed to create xkb context");
        return;
    }
    m_keymap = xkb_keymap_new_from_names(m_ctx, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS);
    if (!m_keymap) {
        qCWarning(qLcLibInput, "Failed to compile keymap");
        return;
    }
    m_state = xkb_state_new(m_keymap);
    if (!m_state) {
        qCWarning(qLcLibInput, "Failed to create xkb state");
        return;
    }

    m_repeatTimer.setSingleShot(true);
    connect(&m_repeatTimer, &QTimer::timeout, this, &QLibInputKeyboard::handleRepeat);
#else
    qCWarning(qLcLibInput) << "xkbcommon not available, not performing key mapping";
#endif
}

QLibInputKeyboard::~QLibInputKeyboard()
{
#if QT_CONFIG(xkbcommon)
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
#if QT_CONFIG(xkbcommon)
    if (!m_ctx || !m_keymap || !m_state)
        return;

    const uint32_t keycode = libinput_event_keyboard_get_key(e) + 8;
    const xkb_keysym_t sym = xkb_state_key_get_one_sym(m_state, keycode);
    const bool pressed = libinput_event_keyboard_get_key_state(e) == LIBINPUT_KEY_STATE_PRESSED;

    // Modifiers here is the modifier state before the event, i.e. not
    // including the current key in case it is a modifier. See the XOR
    // logic in QKeyEvent::modifiers(). ### QTBUG-73826
    Qt::KeyboardModifiers modifiers = QXkbCommon::modifiers(m_state);

    const QString text = QXkbCommon::lookupString(m_state, keycode);
    const int qtkey = QXkbCommon::keysymToQtKey(sym, modifiers, m_state, keycode);

    xkb_state_update_key(m_state, keycode, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);

    Qt::KeyboardModifiers modifiersAfterStateChange = QXkbCommon::modifiers(m_state, sym);
    QGuiApplicationPrivate::inputDeviceManager()->setKeyboardModifiers(modifiersAfterStateChange);

    QWindowSystemInterface::handleExtendedKeyEvent(nullptr,
                                                   pressed ? QEvent::KeyPress : QEvent::KeyRelease,
                                                   qtkey, modifiers, keycode, sym, modifiers, text);

    if (pressed && xkb_keymap_key_repeats(m_keymap, keycode)) {
        m_repeatData.qtkey = qtkey;
        m_repeatData.mods = modifiers;
        m_repeatData.nativeScanCode = keycode;
        m_repeatData.virtualKey = sym;
        m_repeatData.nativeMods = modifiers;
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

#if QT_CONFIG(xkbcommon)
void QLibInputKeyboard::handleRepeat()
{
    QWindowSystemInterface::handleExtendedKeyEvent(nullptr, QEvent::KeyPress,
                                                   m_repeatData.qtkey, m_repeatData.mods,
                                                   m_repeatData.nativeScanCode, m_repeatData.virtualKey, m_repeatData.nativeMods,
                                                   m_repeatData.unicodeText, true, m_repeatData.repeatCount);
    m_repeatData.repeatCount += 1;
    m_repeatTimer.setInterval(REPEAT_RATE);
    m_repeatTimer.start();
}
#endif

QT_END_NAMESPACE
