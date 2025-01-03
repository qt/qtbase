// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QLIBINPUTKEYBOARD_P_H
#define QLIBINPUTKEYBOARD_P_H

#include <QtCore/QPoint>
#include <QtCore/QTimer>

#include <QtGui/private/qtguiglobal_p.h>

#if QT_CONFIG(xkbcommon)
#include <xkbcommon/xkbcommon.h>
#endif

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

struct libinput_event_keyboard;

QT_BEGIN_NAMESPACE

class QLibInputKeyboard : public QObject
{
public:
    QLibInputKeyboard();
    ~QLibInputKeyboard();

    void processKey(libinput_event_keyboard *e);

#if QT_CONFIG(xkbcommon)
    void handleRepeat();

private:
    int keysymToQtKey(xkb_keysym_t key) const;
    int keysymToQtKey(xkb_keysym_t keysym, Qt::KeyboardModifiers *modifiers, const QString &text) const;

    xkb_context *m_ctx = nullptr;
    xkb_keymap *m_keymap = nullptr;
    xkb_state *m_state = nullptr;

    QTimer m_repeatTimer;

    struct {
        int qtkey;
        Qt::KeyboardModifiers mods;
        int nativeScanCode;
        int virtualKey;
        int nativeMods;
        QString unicodeText;
        int repeatCount;
    } m_repeatData;
#endif
};

QT_END_NAMESPACE

#endif
