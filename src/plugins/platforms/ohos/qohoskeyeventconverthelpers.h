// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSKEYEVENTCONVERTHELPERS_H
#define QOHOSKEYEVENTCONVERTHELPERS_H

#include <QtCore/qflags.h>
#include <QtCore/qglobal.h>
#include <qarkui/input.h>
#include <qohoskeymodifiers.h>

QT_BEGIN_NAMESPACE

namespace QtKeyEventHelpers {

Qt::KeyboardModifiers convertOhosToQtKeyboardModifiersWithNumpad(
    QFlags<OhosKeyboardModifier> ohosKeysModifiers, bool keyFromNumPad);

bool isOnlyShiftModificatorPressed(Qt::KeyboardModifiers keyboardModifiers);

QString tryConvertQtKeyCodeToKeyText(
    const Qt::Key &key, Qt::KeyboardModifiers qtModifiers, QFlags<OhosKeyboardModifier> ohosKeysModifiers);

Qt::KeyboardModifiers convertCurrentOhosKeyModifiersForQGuiApplication(
    Qt::KeyboardModifiers lastKeyboardModifiers, QFlags<OhosKeyboardModifier> ohosKeysModifiers, Qt::Key qtKey);

}

QT_END_NAMESPACE

#endif // QOHOSKEYEVENTCONVERTHELPERS_H
