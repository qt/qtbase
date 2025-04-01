// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qglobal.h>

#ifdef Q_OS_MACOS
#include <AppKit/AppKit.h>
#endif

#if defined(QT_PLATFORM_UIKIT)
#include <UIKit/UIKit.h>
#endif

#include "qapplekeymapper_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/QGuiApplication>

QT_BEGIN_NAMESPACE

#ifdef Q_OS_MACOS
Q_STATIC_LOGGING_CATEGORY(lcQpaKeyMapperKeys, "qt.qpa.keymapper.keys");
#endif

static Qt::KeyboardModifiers swapModifiersIfNeeded(const Qt::KeyboardModifiers modifiers)
{
    if (QCoreApplication::testAttribute(Qt::AA_MacDontSwapCtrlAndMeta))
        return modifiers;

    Qt::KeyboardModifiers swappedModifiers = modifiers;
    swappedModifiers &= ~(Qt::MetaModifier | Qt::ControlModifier);

    if (modifiers & Qt::ControlModifier)
        swappedModifiers |= Qt::MetaModifier;
    if (modifiers & Qt::MetaModifier)
        swappedModifiers |= Qt::ControlModifier;

    return swappedModifiers;
}

#ifdef Q_OS_MACOS
static constexpr std::tuple<NSEventModifierFlags, Qt::KeyboardModifier> cocoaModifierMap[] = {
    { NSEventModifierFlagShift, Qt::ShiftModifier },
    { NSEventModifierFlagControl, Qt::ControlModifier },
    { NSEventModifierFlagCommand, Qt::MetaModifier },
    { NSEventModifierFlagOption, Qt::AltModifier },
    { NSEventModifierFlagNumericPad, Qt::KeypadModifier }
};

Qt::KeyboardModifiers QAppleKeyMapper::fromCocoaModifiers(NSEventModifierFlags cocoaModifiers)
{
    Qt::KeyboardModifiers qtModifiers = Qt::NoModifier;
    for (const auto &[cocoaModifier, qtModifier] : cocoaModifierMap) {
        if (cocoaModifiers & cocoaModifier)
            qtModifiers |= qtModifier;
    }

    return swapModifiersIfNeeded(qtModifiers);
}

NSEventModifierFlags QAppleKeyMapper::toCocoaModifiers(Qt::KeyboardModifiers qtModifiers)
{
    qtModifiers = swapModifiersIfNeeded(qtModifiers);

    NSEventModifierFlags cocoaModifiers = 0;
    for (const auto &[cocoaModifier, qtModifier] : cocoaModifierMap) {
        if (qtModifiers & qtModifier)
            cocoaModifiers |= cocoaModifier;
    }

    return cocoaModifiers;
}

using CarbonModifiers = UInt32; // As opposed to EventModifiers which is UInt16

static CarbonModifiers toCarbonModifiers(Qt::KeyboardModifiers qtModifiers)
{
    qtModifiers = swapModifiersIfNeeded(qtModifiers);

    static constexpr std::tuple<int, Qt::KeyboardModifier> carbonModifierMap[] = {
        { shiftKey, Qt::ShiftModifier },
        { controlKey, Qt::ControlModifier },
        { cmdKey, Qt::MetaModifier },
        { optionKey, Qt::AltModifier },
        { kEventKeyModifierNumLockMask, Qt::KeypadModifier }
    };

    CarbonModifiers carbonModifiers = 0;
    for (const auto &[carbonModifier, qtModifier] : carbonModifierMap) {
        if (qtModifiers & qtModifier)
            carbonModifiers |= carbonModifier;
    }

    return carbonModifiers;
}

// Keyboard keys (non-modifiers)
static QHash<char16_t, Qt::Key> standardKeys = {
    { kHomeCharCode, Qt::Key_Home },
    { kEnterCharCode, Qt::Key_Enter },
    { kEndCharCode, Qt::Key_End },
    { kBackspaceCharCode, Qt::Key_Backspace },
    { kTabCharCode, Qt::Key_Tab },
    { kPageUpCharCode, Qt::Key_PageUp },
    { kPageDownCharCode, Qt::Key_PageDown },
    { kReturnCharCode, Qt::Key_Return },
    { kEscapeCharCode, Qt::Key_Escape },
    { kLeftArrowCharCode, Qt::Key_Left },
    { kRightArrowCharCode, Qt::Key_Right },
    { kUpArrowCharCode, Qt::Key_Up },
    { kDownArrowCharCode, Qt::Key_Down },
    { kHelpCharCode, Qt::Key_Help },
    { kDeleteCharCode, Qt::Key_Delete },
    // ASCII maps, for debugging
    { ':', Qt::Key_Colon },
    { ';', Qt::Key_Semicolon },
    { '<', Qt::Key_Less },
    { '=', Qt::Key_Equal },
    { '>', Qt::Key_Greater },
    { '?', Qt::Key_Question },
    { '@', Qt::Key_At },
    { ' ', Qt::Key_Space },
    { '!', Qt::Key_Exclam },
    { '"', Qt::Key_QuoteDbl },
    { '#', Qt::Key_NumberSign },
    { '$', Qt::Key_Dollar },
    { '%', Qt::Key_Percent },
    { '&', Qt::Key_Ampersand },
    { '\'', Qt::Key_Apostrophe },
    { '(', Qt::Key_ParenLeft },
    { ')', Qt::Key_ParenRight },
    { '*', Qt::Key_Asterisk },
    { '+', Qt::Key_Plus },
    { ',', Qt::Key_Comma },
    { '-', Qt::Key_Minus },
    { '.', Qt::Key_Period },
    { '/', Qt::Key_Slash },
    { '[', Qt::Key_BracketLeft },
    { ']', Qt::Key_BracketRight },
    { '\\', Qt::Key_Backslash },
    { '_', Qt::Key_Underscore },
    { '`', Qt::Key_QuoteLeft },
    { '{', Qt::Key_BraceLeft },
    { '}', Qt::Key_BraceRight },
    { '|', Qt::Key_Bar },
    { '~', Qt::Key_AsciiTilde },
    { '^', Qt::Key_AsciiCircum }
};

static QHash<char16_t, Qt::Key> virtualKeys = {
    { kVK_F1, Qt::Key_F1 },
    { kVK_F2, Qt::Key_F2 },
    { kVK_F3, Qt::Key_F3 },
    { kVK_F4, Qt::Key_F4 },
    { kVK_F5, Qt::Key_F5 },
    { kVK_F6, Qt::Key_F6 },
    { kVK_F7, Qt::Key_F7 },
    { kVK_F8, Qt::Key_F8 },
    { kVK_F9, Qt::Key_F9 },
    { kVK_F10, Qt::Key_F10 },
    { kVK_F11, Qt::Key_F11 },
    { kVK_F12, Qt::Key_F12 },
    { kVK_F13, Qt::Key_F13 },
    { kVK_F14, Qt::Key_F14 },
    { kVK_F15, Qt::Key_F15 },
    { kVK_F16, Qt::Key_F16 },
    { kVK_Return, Qt::Key_Return },
    { kVK_Tab, Qt::Key_Tab },
    { kVK_Escape, Qt::Key_Escape },
    { kVK_Help, Qt::Key_Help },
    { kVK_UpArrow, Qt::Key_Up },
    { kVK_DownArrow, Qt::Key_Down },
    { kVK_LeftArrow, Qt::Key_Left },
    { kVK_RightArrow, Qt::Key_Right },
    { kVK_PageUp, Qt::Key_PageUp },
    { kVK_PageDown, Qt::Key_PageDown }
};

static QHash<char16_t, Qt::Key> functionKeys = {
    { NSUpArrowFunctionKey, Qt::Key_Up },
    { NSDownArrowFunctionKey, Qt::Key_Down },
    { NSLeftArrowFunctionKey, Qt::Key_Left },
    { NSRightArrowFunctionKey, Qt::Key_Right },
    // F1-35 function keys handled manually below
    { NSInsertFunctionKey, Qt::Key_Insert },
    { NSDeleteFunctionKey, Qt::Key_Delete },
    { NSHomeFunctionKey, Qt::Key_Home },
    { NSEndFunctionKey, Qt::Key_End },
    { NSPageUpFunctionKey, Qt::Key_PageUp },
    { NSPageDownFunctionKey, Qt::Key_PageDown },
    { NSPrintScreenFunctionKey, Qt::Key_Print },
    { NSScrollLockFunctionKey, Qt::Key_ScrollLock },
    { NSPauseFunctionKey, Qt::Key_Pause },
    { NSSysReqFunctionKey, Qt::Key_SysReq },
    { NSMenuFunctionKey, Qt::Key_Menu },
    { NSPrintFunctionKey, Qt::Key_Printer },
    { NSClearDisplayFunctionKey, Qt::Key_Clear },
    { NSInsertCharFunctionKey, Qt::Key_Insert },
    { NSDeleteCharFunctionKey, Qt::Key_Delete },
    { NSSelectFunctionKey, Qt::Key_Select },
    { NSExecuteFunctionKey, Qt::Key_Execute },
    { NSUndoFunctionKey, Qt::Key_Undo },
    { NSRedoFunctionKey, Qt::Key_Redo },
    { NSFindFunctionKey, Qt::Key_Find },
    { NSHelpFunctionKey, Qt::Key_Help },
    { NSModeSwitchFunctionKey, Qt::Key_Mode_switch }
};

static int toKeyCode(const QChar &key, int virtualKey, int modifiers)
{
    qCDebug(lcQpaKeyMapperKeys, "Mapping key: %d (0x%04x) / vk %d (0x%04x)",
        key.unicode(), key.unicode(), virtualKey, virtualKey);

    if (key == char16_t(kClearCharCode) && virtualKey == 0x47)
        return Qt::Key_Clear;

    if (key.isDigit()) {
        qCDebug(lcQpaKeyMapperKeys, "Got digit key: %d", key.digitValue());
        return key.digitValue() + Qt::Key_0;
    }

    if (key.isLetter()) {
        qCDebug(lcQpaKeyMapperKeys, "Got letter key: %d", (key.toUpper().unicode() - 'A'));
        return (key.toUpper().unicode() - 'A') + Qt::Key_A;
    }
    if (key.isSymbol()) {
        qCDebug(lcQpaKeyMapperKeys, "Got symbol key: %d", (key.unicode()));
        return key.unicode();
    }

    if (auto qtKey = standardKeys.value(key.unicode())) {
        // To work like Qt for X11 we issue Backtab when Shift + Tab are pressed
        if (qtKey == Qt::Key_Tab && (modifiers & Qt::ShiftModifier)) {
            qCDebug(lcQpaKeyMapperKeys, "Got key: Qt::Key_Backtab");
            return Qt::Key_Backtab;
        }

        qCDebug(lcQpaKeyMapperKeys) << "Got" << qtKey;
        return qtKey;
    }

    // Last ditch try to match the scan code
    if (auto qtKey = virtualKeys.value(virtualKey)) {
        qCDebug(lcQpaKeyMapperKeys) << "Got scancode" << qtKey;
        return qtKey;
    }

    // Check if they belong to key codes in private unicode range
    if (key >= char16_t(NSUpArrowFunctionKey) && key <= char16_t(NSModeSwitchFunctionKey)) {
        if (auto qtKey = functionKeys.value(key.unicode())) {
            qCDebug(lcQpaKeyMapperKeys) << "Got" << qtKey;
            return qtKey;
        } else if (key >= char16_t(NSF1FunctionKey) && key <= char16_t(NSF35FunctionKey)) {
            auto functionKey = Qt::Key_F1 + (key.unicode() - NSF1FunctionKey) ;
            qCDebug(lcQpaKeyMapperKeys) << "Got" << functionKey;
            return functionKey;
        }
    }

    qCDebug(lcQpaKeyMapperKeys, "Unknown case.. %d[%d] %d", key.unicode(), key.toLatin1(), virtualKey);
    return Qt::Key_unknown;
}

// --------- Cocoa key mapping moved from Qt Core ---------

static const int NSEscapeCharacter = 27; // not defined by Cocoa headers

static const QHash<char16_t, Qt::Key> cocoaKeys = {
    { NSEnterCharacter, Qt::Key_Enter },
    { NSBackspaceCharacter, Qt::Key_Backspace },
    { NSTabCharacter, Qt::Key_Tab },
    { NSNewlineCharacter, Qt::Key_Return },
    { NSCarriageReturnCharacter, Qt::Key_Return },
    { NSBackTabCharacter, Qt::Key_Backtab },
    { NSEscapeCharacter, Qt::Key_Escape },
    { NSDeleteCharacter, Qt::Key_Backspace },
    { NSUpArrowFunctionKey, Qt::Key_Up },
    { NSDownArrowFunctionKey, Qt::Key_Down },
    { NSLeftArrowFunctionKey, Qt::Key_Left },
    { NSRightArrowFunctionKey, Qt::Key_Right },
    { NSF1FunctionKey, Qt::Key_F1 },
    { NSF2FunctionKey, Qt::Key_F2 },
    { NSF3FunctionKey, Qt::Key_F3 },
    { NSF4FunctionKey, Qt::Key_F4 },
    { NSF5FunctionKey, Qt::Key_F5 },
    { NSF6FunctionKey, Qt::Key_F6 },
    { NSF7FunctionKey, Qt::Key_F7 },
    { NSF8FunctionKey, Qt::Key_F8 },
    { NSF9FunctionKey, Qt::Key_F9 },
    { NSF10FunctionKey, Qt::Key_F10 },
    { NSF11FunctionKey, Qt::Key_F11 },
    { NSF12FunctionKey, Qt::Key_F12 },
    { NSF13FunctionKey, Qt::Key_F13 },
    { NSF14FunctionKey, Qt::Key_F14 },
    { NSF15FunctionKey, Qt::Key_F15 },
    { NSF16FunctionKey, Qt::Key_F16 },
    { NSF17FunctionKey, Qt::Key_F17 },
    { NSF18FunctionKey, Qt::Key_F18 },
    { NSF19FunctionKey, Qt::Key_F19 },
    { NSF20FunctionKey, Qt::Key_F20 },
    { NSF21FunctionKey, Qt::Key_F21 },
    { NSF22FunctionKey, Qt::Key_F22 },
    { NSF23FunctionKey, Qt::Key_F23 },
    { NSF24FunctionKey, Qt::Key_F24 },
    { NSF25FunctionKey, Qt::Key_F25 },
    { NSF26FunctionKey, Qt::Key_F26 },
    { NSF27FunctionKey, Qt::Key_F27 },
    { NSF28FunctionKey, Qt::Key_F28 },
    { NSF29FunctionKey, Qt::Key_F29 },
    { NSF30FunctionKey, Qt::Key_F30 },
    { NSF31FunctionKey, Qt::Key_F31 },
    { NSF32FunctionKey, Qt::Key_F32 },
    { NSF33FunctionKey, Qt::Key_F33 },
    { NSF34FunctionKey, Qt::Key_F34 },
    { NSF35FunctionKey, Qt::Key_F35 },
    { NSInsertFunctionKey, Qt::Key_Insert },
    { NSDeleteFunctionKey, Qt::Key_Delete },
    { NSHomeFunctionKey, Qt::Key_Home },
    { NSEndFunctionKey, Qt::Key_End },
    { NSPageUpFunctionKey, Qt::Key_PageUp },
    { NSPageDownFunctionKey, Qt::Key_PageDown },
    { NSPrintScreenFunctionKey, Qt::Key_Print },
    { NSScrollLockFunctionKey, Qt::Key_ScrollLock },
    { NSPauseFunctionKey, Qt::Key_Pause },
    { NSSysReqFunctionKey, Qt::Key_SysReq },
    { NSMenuFunctionKey, Qt::Key_Menu },
    { NSHelpFunctionKey, Qt::Key_Help },
};

QChar QAppleKeyMapper::toCocoaKey(Qt::Key key)
{
    // Prioritize overloaded keys
    if (key == Qt::Key_Return)
        return char16_t(NSCarriageReturnCharacter);
    if (key == Qt::Key_Backspace)
        return char16_t(NSBackspaceCharacter);

    Q_CONSTINIT static QHash<Qt::Key, char16_t> reverseCocoaKeys;
    if (reverseCocoaKeys.isEmpty()) {
        reverseCocoaKeys.reserve(cocoaKeys.size());
        for (auto it = cocoaKeys.begin(); it != cocoaKeys.end(); ++it)
            reverseCocoaKeys.insert(it.value(), it.key());
    }

    return reverseCocoaKeys.value(key);
}

Qt::Key QAppleKeyMapper::fromCocoaKey(QChar keyCode)
{
    if (auto key = cocoaKeys.value(keyCode.unicode()))
        return key;

    return Qt::Key(keyCode.toUpper().unicode());
}

// ------------------------------------------------

Qt::KeyboardModifiers QAppleKeyMapper::queryKeyboardModifiers() const
{
    return fromCocoaModifiers(NSEvent.modifierFlags);
}

bool QAppleKeyMapper::updateKeyboard()
{
    QCFType<TISInputSourceRef> source = TISCopyInputMethodKeyboardLayoutOverride();
    if (!source)
        source = TISCopyCurrentKeyboardInputSource();

    if (m_keyboardMode != NullMode && source == m_currentInputSource)
        return false;

    Q_ASSERT(source);
    m_currentInputSource = source;
    m_keyboardKind = LMGetKbdType();

    m_keyMap.clear();

    if (auto data = CFDataRef(TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData))) {
        const UCKeyboardLayout *uchrData = reinterpret_cast<const UCKeyboardLayout *>(CFDataGetBytePtr(data));
        Q_ASSERT(uchrData);
        m_keyboardLayoutFormat = uchrData;
        m_keyboardMode = UnicodeMode;
    } else {
        m_keyboardLayoutFormat = nullptr;
        m_keyboardMode = NullMode;
    }

    qCDebug(lcQpaKeyMapper) << "Updated keyboard to"
        << QString::fromCFString(CFStringRef(TISGetInputSourceProperty(
            m_currentInputSource, kTISPropertyLocalizedName)));

    return true;
}

static constexpr Qt::KeyboardModifiers modifierCombinations[] = {
    Qt::NoModifier,                                             // 0
    Qt::ShiftModifier,                                          // 1
    Qt::ControlModifier,                                        // 2
    Qt::ControlModifier | Qt::ShiftModifier,                    // 3
    Qt::AltModifier,                                            // 4
    Qt::AltModifier | Qt::ShiftModifier,                        // 5
    Qt::AltModifier | Qt::ControlModifier,                      // 6
    Qt::AltModifier | Qt::ShiftModifier | Qt::ControlModifier,  // 7
    Qt::MetaModifier,                                           // 8
    Qt::MetaModifier | Qt::ShiftModifier,                       // 9
    Qt::MetaModifier | Qt::ControlModifier,                     // 10
    Qt::MetaModifier | Qt::ControlModifier | Qt::ShiftModifier, // 11
    Qt::MetaModifier | Qt::AltModifier,                         // 12
    Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier,     // 13
    Qt::MetaModifier | Qt::AltModifier | Qt::ControlModifier,   // 14
    Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::ControlModifier,  // 15
};

/*
    Returns a key map for the given \virtualKey based on all
    possible modifier combinations.
*/
const QAppleKeyMapper::KeyMap &QAppleKeyMapper::keyMapForKey(VirtualKeyCode virtualKey) const
{
    static_assert(sizeof(modifierCombinations) / sizeof(Qt::KeyboardModifiers) == kNumModifierCombinations);

    const_cast<QAppleKeyMapper *>(this)->updateKeyboard();

    auto &keyMap = m_keyMap[virtualKey];
    if (keyMap[Qt::NoModifier] != Qt::Key_unknown)
        return keyMap; // Already filled

    qCDebug(lcQpaKeyMapper, "Updating key map for virtual key 0x%02x", (uint)virtualKey);

    // Key mapping via [NSEvent charactersByApplyingModifiers:] only works for key down
    // events, but we might (wrongly) get into this code path for other key events such
    // as NSEventTypeFlagsChanged.
    const bool canMapCocoaEvent = NSApp.currentEvent.type == NSEventTypeKeyDown;

    if (!canMapCocoaEvent)
        qCWarning(lcQpaKeyMapper) << "Could not map key to character for event" << NSApp.currentEvent;

    for (int i = 0; i < kNumModifierCombinations; ++i) {
        Q_ASSERT(!i || keyMap[i] == 0);

        auto qtModifiers = modifierCombinations[i];
        auto carbonModifiers = toCarbonModifiers(qtModifiers);
        const UInt32 modifierKeyState = (carbonModifiers >> 8) & 0xFF;

        UInt32 deadKeyState = 0;
        static const UniCharCount maxStringLength = 10;
        static UniChar unicodeString[maxStringLength];
        UniCharCount actualStringLength = 0;
        OSStatus err = UCKeyTranslate(m_keyboardLayoutFormat, virtualKey,
            kUCKeyActionDown, modifierKeyState, m_keyboardKind,
            kUCKeyTranslateNoDeadKeysMask, &deadKeyState,
            maxStringLength, &actualStringLength,
            unicodeString);

        // Use translated Unicode key if valid
        QChar carbonUnicodeKey;
        if (err == noErr && actualStringLength)
            carbonUnicodeKey = QChar(unicodeString[0]);

        if (canMapCocoaEvent) {
            // Until we've verified that the Cocoa API works as expected
            // we first run the event through the Carbon APIs and then
            // compare the results to Cocoa.
            auto cocoaModifiers = toCocoaModifiers(qtModifiers);
            auto *charactersWithModifiers = [NSApp.currentEvent charactersByApplyingModifiers:cocoaModifiers];

            QChar cocoaUnicodeKey;
            if (charactersWithModifiers.length > 0)
                cocoaUnicodeKey = QChar([charactersWithModifiers characterAtIndex:0]);

            if (cocoaUnicodeKey != carbonUnicodeKey) {
                qCWarning(lcQpaKeyMapper) << "Mismatch between Cocoa" << cocoaUnicodeKey
                    << "and Carbon" << carbonUnicodeKey << "for virtual key" << virtualKey
                    << "with" << qtModifiers;
            }
        }

        int qtKey = toKeyCode(carbonUnicodeKey, virtualKey, qtModifiers);
        if (qtKey == Qt::Key_unknown)
            qtKey = carbonUnicodeKey.unicode();

        keyMap[i] = qtKey;

        qCDebug(lcQpaKeyMapper).verbosity(0) << "\t" << qtModifiers
            << "+" << qUtf8Printable(QString::asprintf("0x%02x", virtualKey))
            << "=" << qUtf8Printable(QString::asprintf("%d / 0x%02x /", qtKey, qtKey))
                   << QKeySequence(qtKey).toString();
    }

    return keyMap;
}

/*
    Compute the possible key combinations that can map to the event's
    virtual key and modifiers, in the current keyboard layout.

    For example, given a normal US keyboard layout, the virtual key
    23 combined with the Alt (⌥) and Shift (⇧) modifiers, can map
    to the following key combinations:

        - Alt+Shift+5
        - Alt+%
        - Shift+∞
        - ﬁ

    The function builds on a key map produced by keyMapForKey(),
    where each modifier-key combination has been mapped to the
    key it will produce.
*/
QList<QKeyCombination> QAppleKeyMapper::possibleKeyCombinations(const QKeyEvent *event) const
{
    QList<QKeyCombination> ret;

    const auto nativeVirtualKey = event->nativeVirtualKey();
    if (!nativeVirtualKey)
        return ret;

    auto keyMap = keyMapForKey(nativeVirtualKey);

    auto unmodifiedKey = keyMap[Qt::NoModifier];
    Q_ASSERT(unmodifiedKey != Qt::Key_unknown);

    auto eventModifiers = event->modifiers();

    int startingModifierLayer = 0;
    if (toCocoaModifiers(eventModifiers) & NSEventModifierFlagCommand) {
        // When the Command key is pressed AppKit seems to do key equivalent
        // matching using a Latin/Roman interpretation of the current keyboard
        // layout. For example, for a Greek layout, pressing Option+Command+C
        // produces a key event with chars="ç" and unmodchars="ψ", but AppKit
        // still treats this as a match for a key equivalent of Option+Command+C.
        // We can't do the same by just applying the modifiers to our key map,
        // as that too contains "ψ" for the Option+Command combination. What we
        // can do instead is take advantage of the fact that the Command
        // modifier layer in all/most keyboard layouts contains a Latin
        // layer. We then combine that with the modifiers of the event
        // to produce the resulting "Latin" key combination.

        // Depending on whether Qt::AA_MacDontSwapCtrlAndMeta is set or not
        // the index of the Command layer in modifierCombinations will differ.
        const int commandLayer = qApp->testAttribute(Qt::AA_MacDontSwapCtrlAndMeta) ? 8 : 2;
        // FIXME: We don't clear the key map when Qt::AA_MacDontSwapCtrlAndMeta
        // is set, so changing Qt::AA_MacDontSwapCtrlAndMeta at runtime will
        // fail once we've built the key map.
        ret << QKeyCombination::fromCombined(int(eventModifiers) + int(keyMap[commandLayer]));

        // If the unmodified key is outside of Latin1, we also treat
        // that as a valid key combination, even if AppKit natively
        // does not. For example, for a Greek layout, we still want
        // to support Option+Command+ψ as a key combination, as it's
        // unlikely to clash with the Latin key combination we added
        // above.

        // However, if the unmodified key is within Latin1, we skip
        // it, to avoid these types of conflicts. For example, in
        // the same Greek layout, pressing the key next to Tab will
        // produce a Latin ';' symbol, but we've already treated that
        // as 'q' above, thanks to the Command modifier, so we skip
        // the potential Command+; key combination. This is also in
        // line with what AppKit natively does.

        // Skipping Latin1 unmodified keys also handles the case of
        // a Latin layout, where the unmodified and modified keys
        // are the same.

        if (unmodifiedKey <= 0xff)
            startingModifierLayer = 1;
    }

    // FIXME: We only compute the first 8 combinations. Why?
    for (int i = startingModifierLayer; i < 15; ++i) {
        auto keyAfterApplyingModifiers = keyMap[i];
        if (!keyAfterApplyingModifiers)
             continue;

        // Include key if the event modifiers match exactly,
        // or are a superset of the current candidate modifiers.
        auto candidateModifiers = modifierCombinations[i];
        if ((eventModifiers & candidateModifiers) == candidateModifiers) {
            // If the event includes more modifiers than the candidate they
            // will need to be included in the resulting key combination.
            auto additionalModifiers = eventModifiers & ~candidateModifiers;

            auto keyCombination = QKeyCombination::fromCombined(
                int(additionalModifiers) + int(keyAfterApplyingModifiers));

            // If there's an existing key combination with the same key,
            // but a different set of modifiers, we want to choose only
            // one of them, by priority (see below).
            const auto existingCombination = std::find_if(
                ret.begin(), ret.end(), [&](auto existingCombination) {
                    return existingCombination.key() == keyAfterApplyingModifiers;
                });

            if (existingCombination != ret.end()) {
                // We prioritize the combination with the more specific
                // modifiers. In the case where the number of modifiers
                // are the same, we want to prioritize Command over Option
                // over Control over Shift. Unfortunately the order (and
                // hence value) of the modifiers in Qt::KeyboardModifier
                // does not match our preferred order when Control and
                // Meta is switched, but we can work around that by
                // explicitly swapping the modifiers and using that
                // for the comparison. This also works when the
                // Qt::AA_MacDontSwapCtrlAndMeta application attribute
                // is set, as the incoming modifiers are then left
                // as is, and we can still trust the order.
                auto existingModifiers = swapModifiersIfNeeded(existingCombination->keyboardModifiers());
                auto replacementModifiers = swapModifiersIfNeeded(additionalModifiers);
                if (replacementModifiers > existingModifiers)
                    *existingCombination = keyCombination;
            } else {
                // All is good, no existing combination has this key
                ret << keyCombination;
            }
        }
    }

    return ret;
}



#else // iOS

Qt::Key QAppleKeyMapper::fromNSString(Qt::KeyboardModifiers qtModifiers, NSString *characters,
                                      NSString *charactersIgnoringModifiers, QString &text)
{
    if ([characters isEqualToString:@"\t"]) {
        if (qtModifiers & Qt::ShiftModifier)
            return Qt::Key_Backtab;
        return Qt::Key_Tab;
    } else if ([characters isEqualToString:@"\r"]) {
        if (qtModifiers & Qt::KeypadModifier)
            return Qt::Key_Enter;
        return Qt::Key_Return;
    }
    if ([characters length] != 0 || [charactersIgnoringModifiers length] != 0) {
        QChar ch;
        if (((qtModifiers & Qt::MetaModifier) || (qtModifiers & Qt::AltModifier)) &&
            ([charactersIgnoringModifiers length] != 0)) {
            ch = QChar([charactersIgnoringModifiers characterAtIndex:0]);
        } else if ([characters length] != 0) {
            ch = QChar([characters characterAtIndex:0]);
        }
        if (!(qtModifiers & (Qt::ControlModifier | Qt::MetaModifier)) &&
            (ch.unicode() < 0xf700 || ch.unicode() > 0xf8ff)) {
            text = QString::fromNSString(characters);
        }
        if (!ch.isNull())
            return Qt::Key(ch.toUpper().unicode());
    }
    return Qt::Key_unknown;
}

// Keyboard keys (non-modifiers)
API_AVAILABLE(ios(13.4)) Qt::Key QAppleKeyMapper::fromUIKitKey(NSString *keyCode)
{
    static QHash<NSString *, Qt::Key> uiKitKeys = {
        { UIKeyInputF1, Qt::Key_F1 },
        { UIKeyInputF2, Qt::Key_F2 },
        { UIKeyInputF3, Qt::Key_F3 },
        { UIKeyInputF4, Qt::Key_F4 },
        { UIKeyInputF5, Qt::Key_F5 },
        { UIKeyInputF6, Qt::Key_F6 },
        { UIKeyInputF7, Qt::Key_F7 },
        { UIKeyInputF8, Qt::Key_F8 },
        { UIKeyInputF9, Qt::Key_F9 },
        { UIKeyInputF10, Qt::Key_F10 },
        { UIKeyInputF11, Qt::Key_F11 },
        { UIKeyInputF12, Qt::Key_F12 },
        { UIKeyInputHome, Qt::Key_Home },
        { UIKeyInputEnd, Qt::Key_End },
        { UIKeyInputPageUp, Qt::Key_PageUp },
        { UIKeyInputPageDown, Qt::Key_PageDown },
        { UIKeyInputEscape, Qt::Key_Escape },
        { UIKeyInputUpArrow, Qt::Key_Up },
        { UIKeyInputDownArrow, Qt::Key_Down },
        { UIKeyInputLeftArrow, Qt::Key_Left },
        { UIKeyInputRightArrow, Qt::Key_Right }
    };

    if (auto key = uiKitKeys.value(keyCode))
        return key;

    return Qt::Key_unknown;
}

static constexpr std::tuple<ulong, Qt::KeyboardModifier> uiKitModifierMap[] = {
    { UIKeyModifierShift, Qt::ShiftModifier },
    { UIKeyModifierControl, Qt::ControlModifier },
    { UIKeyModifierCommand, Qt::MetaModifier },
    { UIKeyModifierAlternate, Qt::AltModifier },
    { UIKeyModifierNumericPad, Qt::KeypadModifier }
};

ulong QAppleKeyMapper::toUIKitModifiers(Qt::KeyboardModifiers qtModifiers)
{
    qtModifiers = swapModifiersIfNeeded(qtModifiers);

    ulong nativeModifiers = 0;
    for (const auto &[nativeModifier, qtModifier] : uiKitModifierMap) {
        if (qtModifiers & qtModifier)
            nativeModifiers |= nativeModifier;
    }

    return nativeModifiers;
}

Qt::KeyboardModifiers QAppleKeyMapper::fromUIKitModifiers(ulong nativeModifiers)
{
    Qt::KeyboardModifiers qtModifiers = Qt::NoModifier;
    for (const auto &[nativeModifier, qtModifier] : uiKitModifierMap) {
        if (nativeModifiers & nativeModifier)
            qtModifiers |= qtModifier;
    }

    return swapModifiersIfNeeded(qtModifiers);
}
#endif

QT_END_NAMESPACE
