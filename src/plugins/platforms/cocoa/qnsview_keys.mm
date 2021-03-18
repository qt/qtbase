/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

// This file is included from qnsview.mm, and only used to organize the code

@implementation QNSView (KeysAPI)

+ (Qt::KeyboardModifiers)convertKeyModifiers:(ulong)modifierFlags
{
    const bool dontSwapCtrlAndMeta = qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
    Qt::KeyboardModifiers qtMods =Qt::NoModifier;
    if (modifierFlags & NSEventModifierFlagShift)
        qtMods |= Qt::ShiftModifier;
    if (modifierFlags & NSEventModifierFlagControl)
        qtMods |= dontSwapCtrlAndMeta ? Qt::ControlModifier : Qt::MetaModifier;
    if (modifierFlags & NSEventModifierFlagOption)
        qtMods |= Qt::AltModifier;
    if (modifierFlags & NSEventModifierFlagCommand)
        qtMods |= dontSwapCtrlAndMeta ? Qt::MetaModifier : Qt::ControlModifier;
    if (modifierFlags & NSEventModifierFlagNumericPad)
        qtMods |= Qt::KeypadModifier;
    return qtMods;
}

@end

@implementation QNSView (Keys)

- (int)convertKeyCode:(QChar)keyChar
{
    return qt_mac_cocoaKey2QtKey(keyChar);
}

- (bool)handleKeyEvent:(NSEvent *)nsevent eventType:(int)eventType
{
    ulong timestamp = [nsevent timestamp] * 1000;
    ulong nativeModifiers = [nsevent modifierFlags];
    Qt::KeyboardModifiers modifiers = [QNSView convertKeyModifiers: nativeModifiers];
    NSString *charactersIgnoringModifiers = [nsevent charactersIgnoringModifiers];
    NSString *characters = [nsevent characters];
    if (m_inputSource != characters) {
        [m_inputSource release];
        m_inputSource = [characters retain];
    }

    // There is no way to get the scan code from carbon/cocoa. But we cannot
    // use the value 0, since it indicates that the event originates from somewhere
    // else than the keyboard.
    quint32 nativeScanCode = 1;
    quint32 nativeVirtualKey = [nsevent keyCode];

    QChar ch = QChar::ReplacementCharacter;
    int keyCode = Qt::Key_unknown;

    // If a dead key occurs as a result of pressing a key combination then
    // characters will have 0 length, but charactersIgnoringModifiers will
    // have a valid character in it. This enables key combinations such as
    // ALT+E to be used as a shortcut with an English keyboard even though
    // pressing ALT+E will give a dead key while doing normal text input.
    if ([characters length] != 0 || [charactersIgnoringModifiers length] != 0) {
        auto ctrlOrMetaModifier = qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta) ? Qt::ControlModifier : Qt::MetaModifier;
        if (((modifiers & ctrlOrMetaModifier) || (modifiers & Qt::AltModifier)) && ([charactersIgnoringModifiers length] != 0))
            ch = QChar([charactersIgnoringModifiers characterAtIndex:0]);
        else if ([characters length] != 0)
            ch = QChar([characters characterAtIndex:0]);
        keyCode = [self convertKeyCode:ch];
    }

    // we will send a key event unless the input method sets m_sendKeyEvent to false
    m_sendKeyEvent = true;
    QString text;
    // ignore text for the U+F700-U+F8FF range. This is used by Cocoa when
    // delivering function keys (e.g. arrow keys, backspace, F1-F35, etc.)
    if (!(modifiers & (Qt::ControlModifier | Qt::MetaModifier)) && (ch.unicode() < 0xf700 || ch.unicode() > 0xf8ff))
        text = QString::fromNSString(characters);

    QWindow *window = [self topLevelWindow];

    // Popups implicitly grab key events; forward to the active popup if there is one.
    // This allows popups to e.g. intercept shortcuts and close the popup in response.
    if (QCocoaWindow *popup = QCocoaIntegration::instance()->activePopupWindow()) {
        if (!popup->window()->flags().testFlag(Qt::ToolTip))
            window = popup->window();
    }

    if (eventType == QEvent::KeyPress) {

        if (m_composingText.isEmpty()) {
            m_sendKeyEvent = !QWindowSystemInterface::handleShortcutEvent(window, timestamp, keyCode,
                modifiers, nativeScanCode, nativeVirtualKey, nativeModifiers, text, [nsevent isARepeat], 1);

            // Handling a shortcut may result in closing the window
            if (!m_platformWindow)
                return true;
        }

        QObject *fo = m_platformWindow->window()->focusObject();
        if (m_sendKeyEvent && fo) {
            QInputMethodQueryEvent queryEvent(Qt::ImEnabled | Qt::ImHints);
            if (QCoreApplication::sendEvent(fo, &queryEvent)) {
                bool imEnabled = queryEvent.value(Qt::ImEnabled).toBool();
                Qt::InputMethodHints hints = static_cast<Qt::InputMethodHints>(queryEvent.value(Qt::ImHints).toUInt());
                if (imEnabled && !(hints & Qt::ImhDigitsOnly || hints & Qt::ImhFormattedNumbersOnly || hints & Qt::ImhHiddenText)) {
                    // pass the key event to the input method. note that m_sendKeyEvent may be set to false during this call
                    m_currentlyInterpretedKeyEvent = nsevent;
                    [self interpretKeyEvents:@[nsevent]];
                    m_currentlyInterpretedKeyEvent = 0;
                }
            }
        }
        if (m_resendKeyEvent)
            m_sendKeyEvent = true;
    }

    bool accepted = true;
    if (m_sendKeyEvent && m_composingText.isEmpty()) {
        QWindowSystemInterface::handleExtendedKeyEvent(window, timestamp, QEvent::Type(eventType), keyCode, modifiers,
                                                       nativeScanCode, nativeVirtualKey, nativeModifiers, text, [nsevent isARepeat], 1, false);
        accepted = QWindowSystemInterface::flushWindowSystemEvents();
    }
    m_sendKeyEvent = false;
    m_resendKeyEvent = false;
    return accepted;
}

- (void)keyDown:(NSEvent *)nsevent
{
    if ([self isTransparentForUserInput])
        return [super keyDown:nsevent];

    const bool accepted = [self handleKeyEvent:nsevent eventType:int(QEvent::KeyPress)];

    // When Qt is used to implement a plugin for a native application we
    // want to propagate unhandled events to other native views. However,
    // Qt does not always set the accepted state correctly (in particular
    // for return key events), so do this for plugin applications only
    // to prevent incorrect forwarding in the general case.
    const bool shouldPropagate = QCoreApplication::testAttribute(Qt::AA_PluginApplication) && !accepted;

    // Track keyDown acceptance/forward state for later acceptance of the keyUp.
    if (!shouldPropagate)
        m_acceptedKeyDowns.insert([nsevent keyCode]);

    if (shouldPropagate)
        [super keyDown:nsevent];
}

- (void)keyUp:(NSEvent *)nsevent
{
    if ([self isTransparentForUserInput])
        return [super keyUp:nsevent];

    const bool keyUpAccepted = [self handleKeyEvent:nsevent eventType:int(QEvent::KeyRelease)];

    // Propagate the keyUp if neither Qt accepted it nor the corresponding KeyDown was
    // accepted. Qt text controls wil often not use and ignore keyUp events, but we
    // want to avoid propagating unmatched keyUps.
    const bool keyDownAccepted = m_acceptedKeyDowns.remove([nsevent keyCode]);
    if (!keyUpAccepted && !keyDownAccepted)
        [super keyUp:nsevent];
}

- (void)cancelOperation:(id)sender
{
    Q_UNUSED(sender);

    NSEvent *currentEvent = [NSApp currentEvent];
    if (!currentEvent || currentEvent.type != NSEventTypeKeyDown)
        return;

    // Handling the key event may recurse back here through interpretKeyEvents
    // (when IM is enabled), so we need to guard against that.
    if (currentEvent == m_currentlyInterpretedKeyEvent)
        return;

    // Send Command+Key_Period and Escape as normal keypresses so that
    // the key sequence is delivered through Qt. That way clients can
    // intercept the shortcut and override its effect.
    [self handleKeyEvent:currentEvent eventType:int(QEvent::KeyPress)];
}

- (void)flagsChanged:(NSEvent *)nsevent
{
    ulong timestamp = [nsevent timestamp] * 1000;
    ulong nativeModifiers = [nsevent modifierFlags];
    Qt::KeyboardModifiers modifiers = [QNSView convertKeyModifiers:nativeModifiers];

    // There is no way to get the scan code from carbon/cocoa. But we cannot
    // use the value 0, since it indicates that the event originates from somewhere
    // else than the keyboard.
    quint32 nativeScanCode = 1;
    quint32 nativeVirtualKey = [nsevent keyCode];

    // calculate the delta and remember the current modifiers for next time
    static ulong m_lastKnownModifiers;
    ulong lastKnownModifiers = m_lastKnownModifiers;
    ulong delta = lastKnownModifiers ^ nativeModifiers;
    m_lastKnownModifiers = nativeModifiers;

    struct qt_mac_enum_mapper
    {
        ulong mac_mask;
        Qt::Key qt_code;
    };
    static qt_mac_enum_mapper modifier_key_symbols[] = {
        { NSEventModifierFlagShift, Qt::Key_Shift },
        { NSEventModifierFlagControl, Qt::Key_Meta },
        { NSEventModifierFlagCommand, Qt::Key_Control },
        { NSEventModifierFlagOption, Qt::Key_Alt },
        { NSEventModifierFlagCapsLock, Qt::Key_CapsLock },
        { 0ul, Qt::Key_unknown } };
    for (int i = 0; modifier_key_symbols[i].mac_mask != 0u; ++i) {
        uint mac_mask = modifier_key_symbols[i].mac_mask;
        if ((delta & mac_mask) == 0u)
            continue;

        Qt::Key qtCode = modifier_key_symbols[i].qt_code;
        if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
            if (qtCode == Qt::Key_Meta)
                qtCode = Qt::Key_Control;
            else if (qtCode == Qt::Key_Control)
                qtCode = Qt::Key_Meta;
        }
        QWindowSystemInterface::handleExtendedKeyEvent(m_platformWindow->window(),
                                                       timestamp,
                                                       (lastKnownModifiers & mac_mask) ? QEvent::KeyRelease
                                                                                       : QEvent::KeyPress,
                                                       qtCode,
                                                       modifiers ^ [QNSView convertKeyModifiers:mac_mask],
                                                       nativeScanCode, nativeVirtualKey,
                                                       nativeModifiers ^ mac_mask);
    }
}

@end
