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

#include <xkbcommon/xkbcommon.h>

#include <QEvent>

QT_BEGIN_NAMESPACE

class QWindow;

class QXcbKeyboard : public QXcbObject
{
public:
    QXcbKeyboard(QXcbConnection *connection);

    ~QXcbKeyboard();

    void handleKeyPressEvent(QXcbWindowEventListener *eventListener, const xcb_key_press_event_t *event);
    void handleKeyReleaseEvent(QXcbWindowEventListener *eventListener, const xcb_key_release_event_t *event);

    void handleMappingNotifyEvent(const xcb_xkb_map_notify_event_t *event);

    Qt::KeyboardModifiers translateModifiers(int s) const;

    void updateKeymap();
    void updateXKBState(xcb_xkb_state_notify_event_t *state);
    int coreDeviceId() { return core_device_id; }

protected:
    void handleKeyEvent(QWindow *window, QEvent::Type type, xcb_keycode_t code, quint16 state, xcb_timestamp_t time);

    QString keysymToUnicode(xcb_keysym_t sym) const;

    int keysymToQtKey(xcb_keysym_t keysym) const;
    int keysymToQtKey(xcb_keysym_t keysym, Qt::KeyboardModifiers &modifiers, QString text) const;

    void readXKBConfig(struct xkb_rule_names *names);
    void updateVModMapping();
    void updateVModToRModMapping();

private:
    xcb_keycode_t m_autorepeat_code;

    struct xkb_context *xkb_context;
    struct xkb_keymap *xkb_keymap;
    struct xkb_state *xkb_state;

    struct _mod_masks {
        uint alt;
        uint altgr;
        uint meta;
    };

    _mod_masks vmod_masks;
    _mod_masks rmod_masks;

    int core_device_id;

    bool m_config;
};

QT_END_NAMESPACE

#endif
