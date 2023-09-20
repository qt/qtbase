// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformkeymapper.h"

#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaKeyMapper, "qt.qpa.keymapper")

QPlatformKeyMapper::~QPlatformKeyMapper()
{
}

/*
    Should return a list of possible key combinations for the given key event.

    For example, given a US English keyboard layout, the key event Shift+5
    can represent both a "Shift+5" key combination, as well as just "%".
*/
QList<QKeyCombination> QPlatformKeyMapper::possibleKeyCombinations(const QKeyEvent *event) const
{
    auto *platformIntegration = QGuiApplicationPrivate::platformIntegration();
    QList<int> possibleKeys = platformIntegration->possibleKeys(event);
    QList<QKeyCombination> combinations;
    for (int key : possibleKeys)
        combinations << QKeyCombination::fromCombined(key);
    return combinations;
}

Qt::KeyboardModifiers QPlatformKeyMapper::queryKeyboardModifiers() const
{
    return QGuiApplicationPrivate::platformIntegration()->queryKeyboardModifiers();
}

QT_END_NAMESPACE
