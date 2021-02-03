/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtCore/QList>
#include <QtGui/QKeyEvent>

#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QAppleKeyMapper
{
public:
    static Qt::KeyboardModifiers queryKeyboardModifiers();
    QList<int> possibleKeys(const QKeyEvent *event) const;
    static Qt::Key fromNSString(Qt::KeyboardModifiers qtMods, NSString *characters,
                                NSString *charactersIgnoringModifiers, QString &text);
#ifdef Q_OS_MACOS
    static Qt::KeyboardModifiers fromCocoaModifiers(NSEventModifierFlags cocoaModifiers);
    static NSEventModifierFlags toCocoaModifiers(Qt::KeyboardModifiers);

    static QChar toCocoaKey(Qt::Key key);
    static Qt::Key fromCocoaKey(QChar keyCode);
#else
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
    const KeyMap &keyMapForKey(VirtualKeyCode virtualKey, QChar unicodeKey) const;

    QCFType<TISInputSourceRef> m_currentInputSource = nullptr;

    enum { NullMode, UnicodeMode, OtherMode } m_keyboardMode = NullMode;
    const UCKeyboardLayout *m_keyboardLayoutFormat = nullptr;
    KeyboardLayoutKind m_keyboardKind = kKLKCHRuchrKind;
    mutable UInt32 m_deadKeyState = 0; // Maintains dead key state beween calls to UCKeyTranslate

    mutable QHash<VirtualKeyCode, KeyMap> m_keyMap;
#endif
};

QT_END_NAMESPACE

#endif

