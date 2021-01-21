/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QXKBCOMMON_P_H
#define QXKBCOMMON_P_H

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

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QLoggingCategory>
#include <QtCore/QList>

#include <xkbcommon/xkbcommon.h>

#include <memory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcXkbcommon)

class QEvent;
class QKeyEvent;
class QPlatformInputContext;

class QXkbCommon
{
public:
    static QString lookupString(struct xkb_state *state, xkb_keycode_t code);
    static QString lookupStringNoKeysymTransformations(xkb_keysym_t keysym);

    static QVector<xkb_keysym_t> toKeysym(QKeyEvent *event);

    static int keysymToQtKey(xkb_keysym_t keysym, Qt::KeyboardModifiers modifiers);
    static int keysymToQtKey(xkb_keysym_t keysym, Qt::KeyboardModifiers modifiers,
                             xkb_state *state, xkb_keycode_t code,
                             bool superAsMeta = false, bool hyperAsMeta = false);

    // xkbcommon_* API is part of libxkbcommon internals, with modifications as
    // desribed in the header of the implementation file.
    static void xkbcommon_XConvertCase(xkb_keysym_t sym, xkb_keysym_t *lower, xkb_keysym_t *upper);
    static xkb_keysym_t qxkbcommon_xkb_keysym_to_upper(xkb_keysym_t ks);

    static Qt::KeyboardModifiers modifiers(struct xkb_state *state);

    static QList<int> possibleKeys(xkb_state *state, const QKeyEvent *event,
                                   bool superAsMeta = false, bool hyperAsMeta = false);

    static void verifyHasLatinLayout(xkb_keymap *keymap);
    static xkb_keysym_t lookupLatinKeysym(xkb_state *state, xkb_keycode_t keycode);

    static bool isLatin(xkb_keysym_t sym) {
        return ((sym >= 'a' && sym <= 'z') || (sym >= 'A' && sym <= 'Z'));
    }
    static bool isKeypad(xkb_keysym_t sym) {
        return sym >= XKB_KEY_KP_Space && sym <= XKB_KEY_KP_9;
    }

    static void setXkbContext(QPlatformInputContext *inputContext, struct xkb_context *context);

    struct XKBStateDeleter {
        void operator()(struct xkb_state *state) const { return xkb_state_unref(state); }
    };
    struct XKBKeymapDeleter {
        void operator()(struct xkb_keymap *keymap) const { return xkb_keymap_unref(keymap); }
    };
    struct XKBContextDeleter {
        void operator()(struct xkb_context *context) const { return xkb_context_unref(context); }
    };
    using ScopedXKBState = std::unique_ptr<struct xkb_state, XKBStateDeleter>;
    using ScopedXKBKeymap = std::unique_ptr<struct xkb_keymap, XKBKeymapDeleter>;
    using ScopedXKBContext = std::unique_ptr<struct xkb_context, XKBContextDeleter>;
};

QT_END_NAMESPACE

#endif // QXKBCOMMON_P_H
