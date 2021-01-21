/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QCOCOAKEYMAPPER_H
#define QCOCOAKEYMAPPER_H

#include "qcocoahelpers.h"

#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>

#include <QtCore/QList>
#include <QtGui/QKeyEvent>

QT_BEGIN_NAMESPACE

/*
    \internal
    A Mac KeyboardLayoutItem has 8 possible states:
        1. Unmodified
        2. Shift
        3. Control
        4. Control + Shift
        5. Alt
        6. Alt + Shift
        7. Alt + Control
        8. Alt + Control + Shift
        9. Meta
        10. Meta + Shift
        11. Meta + Control
        12. Meta + Control + Shift
        13. Meta + Alt
        14. Meta + Alt + Shift
        15. Meta + Alt + Control
        16. Meta + Alt + Control + Shift
*/
struct KeyboardLayoutItem {
    bool dirty;
    quint32 qtKey[16]; // Can by any Qt::Key_<foo>, or unicode character
};


class QCocoaKeyMapper
{
public:
    QCocoaKeyMapper();
    ~QCocoaKeyMapper();
    static Qt::KeyboardModifiers queryKeyboardModifiers();
    QList<int> possibleKeys(const QKeyEvent *event) const;
    bool updateKeyboard();
    void deleteLayouts();
    void updateKeyMap(unsigned short macVirtualKey, QChar unicodeKey);
    void clearMappings();

private:
    QCFType<TISInputSourceRef> currentInputSource = nullptr;

    enum { NullMode, UnicodeMode, OtherMode } keyboard_mode = NullMode;
    const UCKeyboardLayout *keyboard_layout_format = nullptr;
    KeyboardLayoutKind keyboard_kind = kKLKCHRuchrKind;
    UInt32 keyboard_dead = 0;
    KeyboardLayoutItem *keyLayout[256];
};

QT_END_NAMESPACE

#endif

