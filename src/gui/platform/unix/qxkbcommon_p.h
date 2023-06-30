// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QXKBCOMMON_P_H
#define QXKBCOMMON_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/qtguiglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qlist.h>
#include <QtCore/private/qglobal_p.h>

#include <xkbcommon/xkbcommon.h>

#include <memory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcXkbcommon)

class QEvent;
class QKeyEvent;
class QPlatformInputContext;

class Q_GUI_EXPORT QXkbCommon
{
public:
    static QString lookupString(struct xkb_state *state, xkb_keycode_t code);
    static QString lookupStringNoKeysymTransformations(xkb_keysym_t keysym);

    static QList<xkb_keysym_t> toKeysym(QKeyEvent *event);

    static int keysymToQtKey(xkb_keysym_t keysym, Qt::KeyboardModifiers modifiers);
    static int keysymToQtKey(xkb_keysym_t keysym, Qt::KeyboardModifiers modifiers,
                             xkb_state *state, xkb_keycode_t code,
                             bool superAsMeta = true, bool hyperAsMeta = true);

    // xkbcommon_* API is part of libxkbcommon internals, with modifications as
    // described in the header of the implementation file.
    static void xkbcommon_XConvertCase(xkb_keysym_t sym, xkb_keysym_t *lower, xkb_keysym_t *upper);
    static xkb_keysym_t qxkbcommon_xkb_keysym_to_upper(xkb_keysym_t ks);

    static Qt::KeyboardModifiers modifiers(struct xkb_state *state, xkb_keysym_t keysym = XKB_KEY_VoidSymbol);

    static QList<int> possibleKeys(xkb_state *state, const QKeyEvent *event,
                                   bool superAsMeta = false, bool hyperAsMeta = false);

    static void verifyHasLatinLayout(xkb_keymap *keymap);
    static xkb_keysym_t lookupLatinKeysym(xkb_state *state, xkb_keycode_t keycode);

    static bool isLatin1(xkb_keysym_t sym) {
        return sym >= 0x20 && sym <= 0xff;
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
