/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QXCBCONNECTION_H
#define QXCBCONNECTION_H

#include <xcb/xcb.h>

#include <QList>
#include <QObject>
#include <QVector>

#define Q_XCB_DEBUG

class QXcbScreen;

namespace QXcbAtom {
    enum Atom {
        // window-manager <-> client protocols
        WM_PROTOCOLS,
        WM_DELETE_WINDOW,
        WM_TAKE_FOCUS,
        _NET_WM_PING,
        _NET_WM_CONTEXT_HELP,
        _NET_WM_SYNC_REQUEST,
        _NET_WM_SYNC_REQUEST_COUNTER,

        // ICCCM window state
        WM_STATE,
        WM_CHANGE_STATE,

        // Session management
        WM_CLIENT_LEADER,
        WM_WINDOW_ROLE,
        SM_CLIENT_ID,

        // Clipboard
        CLIPBOARD,
        INCR,
        TARGETS,
        MULTIPLE,
        TIMESTAMP,
        SAVE_TARGETS,
        CLIP_TEMPORARY,
        _QT_SELECTION,
        _QT_CLIPBOARD_SENTINEL,
        _QT_SELECTION_SENTINEL,
        CLIPBOARD_MANAGER,

        RESOURCE_MANAGER,

        _XSETROOT_ID,

        _QT_SCROLL_DONE,
        _QT_INPUT_ENCODING,

        _MOTIF_WM_HINTS,

        DTWM_IS_RUNNING,
        ENLIGHTENMENT_DESKTOP,
        _DT_SAVE_MODE,
        _SGI_DESKS_MANAGER,

        // EWMH (aka NETWM)
        _NET_SUPPORTED,
        _NET_VIRTUAL_ROOTS,
        _NET_WORKAREA,

        _NET_MOVERESIZE_WINDOW,
        _NET_WM_MOVERESIZE,

        _NET_WM_NAME,
        _NET_WM_ICON_NAME,
        _NET_WM_ICON,

        _NET_WM_PID,

        _NET_WM_WINDOW_OPACITY,

        _NET_WM_STATE,
        _NET_WM_STATE_ABOVE,
        _NET_WM_STATE_BELOW,
        _NET_WM_STATE_FULLSCREEN,
        _NET_WM_STATE_MAXIMIZED_HORZ,
        _NET_WM_STATE_MAXIMIZED_VERT,
        _NET_WM_STATE_MODAL,
        _NET_WM_STATE_STAYS_ON_TOP,
        _NET_WM_STATE_DEMANDS_ATTENTION,

        _NET_WM_USER_TIME,
        _NET_WM_USER_TIME_WINDOW,
        _NET_WM_FULL_PLACEMENT,

        _NET_WM_WINDOW_TYPE,
        _NET_WM_WINDOW_TYPE_DESKTOP,
        _NET_WM_WINDOW_TYPE_DOCK,
        _NET_WM_WINDOW_TYPE_TOOLBAR,
        _NET_WM_WINDOW_TYPE_MENU,
        _NET_WM_WINDOW_TYPE_UTILITY,
        _NET_WM_WINDOW_TYPE_SPLASH,
        _NET_WM_WINDOW_TYPE_DIALOG,
        _NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        _NET_WM_WINDOW_TYPE_POPUP_MENU,
        _NET_WM_WINDOW_TYPE_TOOLTIP,
        _NET_WM_WINDOW_TYPE_NOTIFICATION,
        _NET_WM_WINDOW_TYPE_COMBO,
        _NET_WM_WINDOW_TYPE_DND,
        _NET_WM_WINDOW_TYPE_NORMAL,
        _KDE_NET_WM_WINDOW_TYPE_OVERRIDE,

        _KDE_NET_WM_FRAME_STRUT,

        _NET_STARTUP_INFO,
        _NET_STARTUP_INFO_BEGIN,

        _NET_SUPPORTING_WM_CHECK,

        _NET_WM_CM_S0,

        _NET_SYSTEM_TRAY_VISUAL,

        _NET_ACTIVE_WINDOW,

        // Property formats
        COMPOUND_TEXT,
        TEXT,
        UTF8_STRING,

        // Xdnd
        XdndEnter,
        XdndPosition,
        XdndStatus,
        XdndLeave,
        XdndDrop,
        XdndFinished,
        XdndTypelist,
        XdndActionList,

        XdndSelection,

        XdndAware,
        XdndProxy,

        XdndActionCopy,
        XdndActionLink,
        XdndActionMove,
        XdndActionPrivate,

        // Motif DND
        _MOTIF_DRAG_AND_DROP_MESSAGE,
        _MOTIF_DRAG_INITIATOR_INFO,
        _MOTIF_DRAG_RECEIVER_INFO,
        _MOTIF_DRAG_WINDOW,
        _MOTIF_DRAG_TARGETS,

        XmTRANSFER_SUCCESS,
        XmTRANSFER_FAILURE,

        // Xkb
        _XKB_RULES_NAMES,

        // XEMBED
        _XEMBED,
        _XEMBED_INFO,

        XWacomStylus,
        XWacomCursor,
        XWacomEraser,

        XTabletStylus,
        XTabletEraser,

        NPredefinedAtoms,

        _QT_SETTINGS_TIMESTAMP = NPredefinedAtoms,
        NAtoms
    };
}

class QXcbKeyboard;

class QXcbConnection : public QObject
{
    Q_OBJECT
public:
    QXcbConnection(const char *displayName = 0);
    ~QXcbConnection();

    QXcbConnection *connection() const { return const_cast<QXcbConnection *>(this); }

    QList<QXcbScreen *> screens() const { return m_screens; }
    int primaryScreen() const { return m_primaryScreen; }

    xcb_atom_t atom(QXcbAtom::Atom atom);

    const char *displayName() const { return m_displayName.constData(); }

    xcb_connection_t *xcb_connection() const { return m_connection; }

    QXcbKeyboard *keyboard() const { return m_keyboard; }

#ifdef XCB_USE_XLIB
    void *xlib_display() const { return m_xlib_display; }
#endif

#ifdef XCB_USE_DRI2
    bool hasSupportForDri2() const;
    QByteArray dri2DeviceName() const { return m_dri2_device_name; }
#endif
#ifdef XCB_USE_EGL
    bool hasEgl() const;
#endif
#if defined(XCB_USE_EGL) || defined(XCB_USE_DRI2)
    void *egl_display() const { return m_egl_display; }
#endif

    void sync();
    void handleXcbError(xcb_generic_error_t *error);

private slots:
    void processXcbEvents();

private:
    void initializeAllAtoms();
    void sendConnectionEvent(QXcbAtom::Atom atom, uint id = 0);
#ifdef XCB_USE_DRI2
    void initializeDri2();
#endif

    xcb_connection_t *m_connection;
    const xcb_setup_t *m_setup;

    QList<QXcbScreen *> m_screens;
    int m_primaryScreen;

    xcb_atom_t m_allAtoms[QXcbAtom::NAtoms];

    QByteArray m_displayName;

    QXcbKeyboard *m_keyboard;

#if defined(XCB_USE_XLIB)
    void *m_xlib_display;
#endif

#ifdef XCB_USE_DRI2
    uint32_t m_dri2_major;
    uint32_t m_dri2_minor;
    bool m_dri2_support_probed;
    bool m_has_support_for_dri2;
    QByteArray m_dri2_device_name;
#endif
#if defined(XCB_USE_EGL) || defined(XCB_USE_DRI2)
    void *m_egl_display;
    bool m_has_egl;
#endif
#ifdef Q_XCB_DEBUG
    struct CallInfo {
        int sequence;
        QByteArray file;
        int line;
    };
    QVector<CallInfo> m_callLog;
    void log(const char *file, int line, int sequence);
    template <typename cookie_t>
    friend cookie_t q_xcb_call_template(const cookie_t &cookie, QXcbConnection *connection, const char *file, int line);
#endif
};

#define DISPLAY_FROM_XCB(object) ((Display *)(object->connection()->xlib_display()))

#ifdef Q_XCB_DEBUG
template <typename cookie_t>
cookie_t q_xcb_call_template(const cookie_t &cookie, QXcbConnection *connection, const char *file, int line)
{
    connection->log(file, line, cookie.sequence);
    return cookie;
}
#define Q_XCB_CALL(x) q_xcb_call_template(x, connection(), __FILE__, __LINE__)
#define Q_XCB_CALL2(x, connection) q_xcb_call_template(x, connection, __FILE__, __LINE__)
#define Q_XCB_NOOP(c) q_xcb_call_template(xcb_no_operation(c->xcb_connection()), c, __FILE__, __LINE__);
#else
#define Q_XCB_CALL(x) x
#define Q_XCB_CALL2(x, connection) x
#define Q_XCB_NOOP(c)
#endif


#if defined(XCB_USE_DRI2) || defined(XCB_USE_EGL)
#define EGL_DISPLAY_FROM_XCB(object) ((EGLDisplay)(object->connection()->egl_display()))
#endif //endifXCB_USE_DRI2

#endif
