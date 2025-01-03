// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// This file is included from qnsview.mm, and only used to organize the code

/*
    Determines if the text represents one of the "special keys" on macOS

    As a legacy from OpenStep, macOS reserves the range 0xF700-0xF8FF of the
    Unicode private use area for representing function keys on the keyboard:

      http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/CORPCHAR.TXT

      https://developer.apple.com/documentation/appkit/nsevent/specialkey

    These code points are not supposed to have any glyphs associated with them,
    but since we can't guarantee that the system doesn't have a font that does
    provide glyphs for this range (Arial Unicode MS e.g.) we need to filter
    the text of our key events up front.
*/
static bool isSpecialKey(const QString &text)
{
    if (text.length() != 1)
        return false;

    const char16_t unicode = text.at(0).unicode();
    if (unicode >= 0xF700 && unicode <= 0xF8FF)
        return true;

    return false;
}

@implementation QNSView (Keys)

- (bool)handleKeyEvent:(NSEvent *)nsevent
{
    qCDebug(lcQpaKeys) << "Handling" << nsevent;
    KeyEvent keyEvent(nsevent);

    // FIXME: Why is this the top level window and not m_platformWindow?
    QWindow *window = [self topLevelWindow];

    // We will send a key event unless the input method handles it
    QBoolBlocker sendKeyEventGuard(m_sendKeyEvent, true);

    // Assume we should send key events with text, unless told
    // otherwise by doCommandBySelector.
    m_sendKeyEventWithoutText = false;

    bool didInterpretKeyEvent = false;

    if (keyEvent.type == QEvent::KeyPress) {

        if (m_composingText.isEmpty()) {
            KeyEvent shortcutEvent = keyEvent;
            shortcutEvent.type = QEvent::Shortcut;
            qCDebug(lcQpaKeys) << "Trying potential shortcuts in" << window
                               << "for" << shortcutEvent;

            if (shortcutEvent.sendWindowSystemEvent(window)) {
                qCDebug(lcQpaKeys) << "Found matching shortcut; will not send as key event";
                return true;
            } else {
                qCDebug(lcQpaKeys) << "No matching shortcuts; continuing with key event delivery";
            }
        }

        QObject *focusObject = m_platformWindow ? m_platformWindow->window()->focusObject() : nullptr;
        if (m_sendKeyEvent && focusObject) {
            if (auto queryResult = queryInputMethod(focusObject, Qt::ImHints)) {
                auto hints = static_cast<Qt::InputMethodHints>(queryResult.value(Qt::ImHints).toUInt());

                // Make sure we send dead keys and the next key to the input method for composition
                const bool isDeadKey = !nsevent.characters.length;
                const bool ignoreHidden = (hints & Qt::ImhHiddenText) && !isDeadKey && !m_lastKeyDead;

                if (!(hints & Qt::ImhDigitsOnly || hints & Qt::ImhFormattedNumbersOnly || ignoreHidden)) {
                    // Pass the key event to the input method, and assume it handles the event,
                    // unless we explicit set m_sendKeyEvent to deliver as a normal key event.
                    m_sendKeyEvent = false;

                    // Match NSTextView's keyDown behavior of hiding the cursor before
                    // interpreting key events. Shortcuts should not trigger this though.
                    // Unfortunately many of our controls handle shortcuts by accepting
                    // the ShortcutOverride event and then handling the shortcut in the
                    // following key event, and QWSI::handleShortcutEvent doesn't reveal
                    // whether this will be the case. For NSTextView this is not an issue
                    // as shortcuts are handled via performKeyEquivalent, which happens
                    // prior to keyDown. To work around this until we can get the info
                    // we need from handleShortcutEvent we match AppKit and assume that
                    // any key press with a command or control modifier is a shortcut.
                    if (!(nsevent.modifierFlags & (NSEventModifierFlagCommand | NSEventModifierFlagControl)))
                        [NSCursor setHiddenUntilMouseMoves:YES];

                    qCDebug(lcQpaKeys) << "Interpreting key event for focus object" << focusObject;
                    m_currentlyInterpretedKeyEvent = nsevent;
                    [self interpretKeyEvents:@[nsevent]];
                    m_currentlyInterpretedKeyEvent = 0;
                    didInterpretKeyEvent = true;

                    // If the last key we sent was dead, then pass the next
                    // key to the IM as well to complete composition.
                    m_lastKeyDead = isDeadKey;
                }

            }
        }
    }

    bool accepted = true;
    if (m_sendKeyEvent && m_composingText.isEmpty()) {
        // Trust text input system on whether to send the event with text or not,
        // or otherwise apply heuristics to filter out private use symbols.
        if (didInterpretKeyEvent ? m_sendKeyEventWithoutText : isSpecialKey(keyEvent.text))
            keyEvent.text = {};
        qCDebug(lcQpaKeys) << "Sending as" << keyEvent;
        accepted = keyEvent.sendWindowSystemEvent(window);
    }
    return accepted;
}

- (void)keyDown:(NSEvent *)nsevent
{
    if ([self isTransparentForUserInput])
        return [super keyDown:nsevent];

    const bool accepted = [self handleKeyEvent:nsevent];

    // When Qt is used to implement a plugin for a native application we
    // want to propagate unhandled events to other native views. However,
    // Qt does not always set the accepted state correctly (in particular
    // for return key events), so do this for plugin applications only
    // to prevent incorrect forwarding in the general case.
    const bool shouldPropagate = QCoreApplication::testAttribute(Qt::AA_PluginApplication) && !accepted;

    // Track keyDown acceptance/forward state for later acceptance of the keyUp.
    if (!shouldPropagate)
        m_acceptedKeyDowns.insert(nsevent.keyCode);

    if (shouldPropagate)
        [super keyDown:nsevent];
}

- (void)keyUp:(NSEvent *)nsevent
{
    if ([self isTransparentForUserInput])
        return [super keyUp:nsevent];

    const bool keyUpAccepted = [self handleKeyEvent:nsevent];

    // Propagate the keyUp if neither Qt accepted it nor the corresponding KeyDown was
    // accepted. Qt text controls will often not use and ignore keyUp events, but we
    // want to avoid propagating unmatched keyUps.
    const bool keyDownAccepted = m_acceptedKeyDowns.remove(nsevent.keyCode);
    if (!keyUpAccepted && !keyDownAccepted)
        [super keyUp:nsevent];
}

- (void)cancelOperation:(id)sender
{
    Q_UNUSED(sender);

    NSEvent *currentEvent = NSApp.currentEvent;
    if (!currentEvent || currentEvent.type != NSEventTypeKeyDown)
        return;

    // Handling the key event may recurse back here through interpretKeyEvents
    // (when IM is enabled), so we need to guard against that.
    if (currentEvent == m_currentlyInterpretedKeyEvent) {
        m_sendKeyEvent = true;
        return;
    }

    // Send Command+Key_Period and Escape as normal keypresses so that
    // the key sequence is delivered through Qt. That way clients can
    // intercept the shortcut and override its effect.
    [self handleKeyEvent:currentEvent];
}

- (void)flagsChanged:(NSEvent *)nsevent
{
    // FIXME: Why are we not checking isTransparentForUserInput here?

    KeyEvent keyEvent(nsevent);
    qCDebug(lcQpaKeys) << "Flags changed resulting in" << keyEvent.modifiers;

    // Calculate the delta and remember the current modifiers for next time
    static NSEventModifierFlags m_lastKnownModifiers;
    NSEventModifierFlags lastKnownModifiers = m_lastKnownModifiers;
    NSEventModifierFlags newModifiers = lastKnownModifiers ^ keyEvent.nativeModifiers;
    m_lastKnownModifiers = keyEvent.nativeModifiers;

    static constexpr std::tuple<NSEventModifierFlags, Qt::Key> modifierMap[] = {
        { NSEventModifierFlagShift, Qt::Key_Shift },
        { NSEventModifierFlagControl, Qt::Key_Meta },
        { NSEventModifierFlagCommand, Qt::Key_Control },
        { NSEventModifierFlagOption, Qt::Key_Alt },
        { NSEventModifierFlagCapsLock, Qt::Key_CapsLock }
    };

    for (auto [macModifier, qtKey] : modifierMap) {
        if (!(newModifiers & macModifier))
            continue;

        // FIXME: Use QAppleKeyMapper helper
        if (qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta)) {
            if (qtKey == Qt::Key_Meta)
                qtKey = Qt::Key_Control;
            else if (qtKey == Qt::Key_Control)
                qtKey = Qt::Key_Meta;
        }

        KeyEvent modifierEvent = keyEvent;
        modifierEvent.type = lastKnownModifiers & macModifier
                           ? QEvent::KeyRelease : QEvent::KeyPress;

        modifierEvent.key = qtKey;

        // FIXME: Shouldn't this be based on lastKnownModifiers?
        modifierEvent.modifiers ^= QAppleKeyMapper::fromCocoaModifiers(macModifier);
        modifierEvent.nativeModifiers ^= macModifier;

        // FIXME: Why are we sending to m_platformWindow here, but not for key events?
        QWindow *window = m_platformWindow->window();

        qCDebug(lcQpaKeys) << "Sending" << modifierEvent;
        modifierEvent.sendWindowSystemEvent(window);
    }
}

@end

// -------------------------------------------------------------------------

KeyEvent::KeyEvent(NSEvent *nsevent)
{
    timestamp = nsevent.timestamp * 1000;
    nativeModifiers = nsevent.modifierFlags;
    modifiers = QAppleKeyMapper::fromCocoaModifiers(nativeModifiers);

    switch (nsevent.type) {
    case NSEventTypeKeyDown: type = QEvent::KeyPress; break;
    case NSEventTypeKeyUp: type = QEvent::KeyRelease; break;
    default: break; // Must be manually set
    }

    switch (nsevent.type) {
    case NSEventTypeKeyDown:
    case NSEventTypeKeyUp:
    case NSEventTypeFlagsChanged:
        nativeVirtualKey = nsevent.keyCode;
    default:
        break;
    }

    if (nsevent.type == NSEventTypeKeyDown || nsevent.type == NSEventTypeKeyUp) {
        NSString *charactersIgnoringModifiers = nsevent.charactersIgnoringModifiers;
        NSString *characters = nsevent.characters;

        QChar character = QChar::ReplacementCharacter;

        // If a dead key occurs as a result of pressing a key combination then
        // characters will have 0 length, but charactersIgnoringModifiers will
        // have a valid character in it. This enables key combinations such as
        // ALT+E to be used as a shortcut with an English keyboard even though
        // pressing ALT+E will give a dead key while doing normal text input.
        if (characters.length || charactersIgnoringModifiers.length) {
            if (nativeModifiers & (NSEventModifierFlagControl | NSEventModifierFlagOption)
                && charactersIgnoringModifiers.length)
                character = QChar([charactersIgnoringModifiers characterAtIndex:0]);
            else if (characters.length)
                character = QChar([characters characterAtIndex:0]);
            key = QAppleKeyMapper::fromCocoaKey(character);
        }

        text = QString::fromNSString(characters);

        isRepeat = nsevent.ARepeat;
    }
}

bool KeyEvent::sendWindowSystemEvent(QWindow *window) const
{
    switch (type) {
    case QEvent::Shortcut: {
        return QWindowSystemInterface::handleShortcutEvent(window, timestamp,
            key, modifiers, nativeScanCode, nativeVirtualKey, nativeModifiers,
            text, isRepeat);
    }
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        static const int count = 1;
        QWindowSystemInterface::handleExtendedKeyEvent(window, timestamp,
            type, key, modifiers, nativeScanCode, nativeVirtualKey, nativeModifiers,
            text, isRepeat, count);
        // FIXME: Make handleExtendedKeyEvent synchronous
        return QWindowSystemInterface::flushWindowSystemEvents();
    }
    default:
        qCritical() << "KeyEvent can not send event type" << type;
        return false;
    }
}

QDebug operator<<(QDebug debug, const KeyEvent &e)
{
    QDebugStateSaver saver(debug);
    debug.nospace().verbosity(0) << "KeyEvent("
        << e.type << ", timestamp=" << e.timestamp
        << ", key=" << e.key << ", modifiers=" << e.modifiers
        << ", text="<< e.text << ", isRepeat=" << e.isRepeat
        << ", nativeVirtualKey=" << e.nativeVirtualKey
        << ", nativeModifiers=" << e.nativeModifiers
    << ")";
    return debug;
}
