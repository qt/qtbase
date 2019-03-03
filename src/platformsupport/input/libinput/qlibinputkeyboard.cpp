/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlibinputkeyboard_p.h"
#include <QtCore/QTextCodec>
#include <QtCore/QLoggingCategory>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qinputdevicemanager_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <libinput.h>
#if QT_CONFIG(xkbcommon)
#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon-names.h>
#include <QtXkbCommonSupport/private/qxkbcommon_p.h>
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

    Qt::KeyboardModifiers modifiersAfterStateChange = QXkbCommon::modifiers(m_state);
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
