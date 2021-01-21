/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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
