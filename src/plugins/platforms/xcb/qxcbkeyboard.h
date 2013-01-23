/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QXCBKEYBOARD_H
#define QXCBKEYBOARD_H

#include "qxcbobject.h"

#include "xcb/xcb_keysyms.h"

#include <QEvent>

QT_BEGIN_NAMESPACE

class QWindow;

class QXcbKeyboard : public QXcbObject
{
public:
    QXcbKeyboard(QXcbConnection *connection);
    ~QXcbKeyboard();

    void handleKeyPressEvent(QXcbWindow *window, const xcb_key_press_event_t *event);
    void handleKeyReleaseEvent(QXcbWindow *window, const xcb_key_release_event_t *event);

    void handleMappingNotifyEvent(const xcb_mapping_notify_event_t *event);

    Qt::KeyboardModifiers translateModifiers(int s);

private:
    void handleKeyEvent(QWindow *window, QEvent::Type type, xcb_keycode_t code, quint16 state, xcb_timestamp_t time);

    int translateKeySym(uint key) const;
    QString translateKeySym(xcb_keysym_t keysym, uint xmodifiers,
                            int &code, Qt::KeyboardModifiers &modifiers,
                            QByteArray &chars, int &count);
    void setupModifiers();
    void setMask(uint sym, uint mask);
    xcb_keysym_t lookupString(QWindow *window, uint state, xcb_keycode_t code,
                              QEvent::Type type, QByteArray *chars);

    uint m_alt_mask;
    uint m_super_mask;
    uint m_hyper_mask;
    uint m_meta_mask;
    uint m_mode_switch_mask;
    uint m_num_lock_mask;
    uint m_caps_lock_mask;

    xcb_key_symbols_t *m_key_symbols;
    xcb_keycode_t m_autorepeat_code;
};

QT_END_NAMESPACE

#endif
