// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAPPLEKEYMAPPER_H
#define QAPPLEKEYMAPPER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifdef Q_OS_MACOS
#include <Carbon/Carbon.h>
#endif

#include <qpa/qplatformkeymapper.h>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtGui/QKeyEvent>

#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QAppleKeyMapper : public QPlatformKeyMapper
{
public:
    Qt::KeyboardModifiers queryKeyboardModifiers() const override;
    QList<QKeyCombination> possibleKeyCombinations(const QKeyEvent *event) const override;

#ifdef Q_OS_MACOS
    static Qt::KeyboardModifiers fromCocoaModifiers(NSEventModifierFlags cocoaModifiers);
    static NSEventModifierFlags toCocoaModifiers(Qt::KeyboardModifiers);

    static QChar toCocoaKey(Qt::Key key);
    static Qt::Key fromCocoaKey(QChar keyCode);
#else
    static Qt::Key fromNSString(Qt::KeyboardModifiers qtMods, NSString *characters,
                            NSString *charactersIgnoringModifiers, QString &text);

    static Qt::Key fromUIKitKey(NSString *keyCode);
    static Qt::KeyboardModifiers fromUIKitModifiers(ulong uikitModifiers);
    static ulong toUIKitModifiers(Qt::KeyboardModifiers);
#endif
private:
#ifdef Q_OS_MACOS
    static constexpr int kNumModifierCombinations = 16;
    struct KeyMap : std::array<char32_t, kNumModifierCombinations>
    {
        // Initialize first element to a sentinel that allows us
        // to distinguish an uninitialized map from an initialized.
        // Using 0 would not allow us to map U+0000 (NUL), however
        // unlikely that is.
        KeyMap() : std::array<char32_t, 16>{Qt::Key_unknown} {}
    };

    bool updateKeyboard();

    using VirtualKeyCode = unsigned short;
    const KeyMap &keyMapForKey(VirtualKeyCode virtualKey) const;

    QCFType<TISInputSourceRef> m_currentInputSource = nullptr;

    enum { NullMode, UnicodeMode, OtherMode } m_keyboardMode = NullMode;
    const UCKeyboardLayout *m_keyboardLayoutFormat = nullptr;
    KeyboardLayoutKind m_keyboardKind = kKLKCHRuchrKind;

    mutable QHash<VirtualKeyCode, KeyMap> m_keyMap;
#endif
};

QT_END_NAMESPACE

#endif

