// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QXCBATOM_H
#define QXCBATOM_H

#include <xcb/xcb.h>

class QXcbAtom
{
public:
    enum Atom {
        // window-manager <-> client protocols
        AtomWM_PROTOCOLS,
        AtomWM_DELETE_WINDOW,
        AtomWM_TAKE_FOCUS,
        Atom_NET_WM_PING,
        Atom_NET_WM_CONTEXT_HELP,
        Atom_NET_WM_SYNC_REQUEST,
        Atom_NET_WM_SYNC_REQUEST_COUNTER,
        AtomMANAGER, // System tray notification
        Atom_NET_SYSTEM_TRAY_OPCODE, // System tray operation

        // ICCCM window state
        AtomWM_STATE,
        AtomWM_CHANGE_STATE,
        AtomWM_CLASS,
        AtomWM_NAME,

        // Session management
        AtomWM_CLIENT_LEADER,
        AtomWM_WINDOW_ROLE,
        AtomSM_CLIENT_ID,
        AtomWM_CLIENT_MACHINE,

        // Clipboard
        AtomCLIPBOARD,
        AtomINCR,
        AtomTARGETS,
        AtomMULTIPLE,
        AtomTIMESTAMP,
        AtomSAVE_TARGETS,
        AtomCLIP_TEMPORARY,
        Atom_QT_SELECTION,
        Atom_QT_CLIPBOARD_SENTINEL,
        Atom_QT_SELECTION_SENTINEL,
        AtomCLIPBOARD_MANAGER,

        AtomRESOURCE_MANAGER,

        Atom_XSETROOT_ID,

        Atom_QT_SCROLL_DONE,
        Atom_QT_INPUT_ENCODING,

        // Qt/XCB specific
        Atom_QT_CLOSE_CONNECTION,

        Atom_QT_GET_TIMESTAMP,

        Atom_MOTIF_WM_HINTS,

        AtomDTWM_IS_RUNNING,
        AtomENLIGHTENMENT_DESKTOP,
        Atom_DT_SAVE_MODE,
        Atom_SGI_DESKS_MANAGER,

        // EWMH (aka NETWM)
        Atom_NET_SUPPORTED,
        Atom_NET_VIRTUAL_ROOTS,
        Atom_NET_WORKAREA,

        Atom_NET_MOVERESIZE_WINDOW,
        Atom_NET_WM_MOVERESIZE,

        Atom_NET_WM_NAME,
        Atom_NET_WM_ICON_NAME,
        Atom_NET_WM_ICON,

        Atom_NET_WM_PID,

        Atom_NET_WM_WINDOW_OPACITY,

        Atom_NET_WM_STATE,
        Atom_NET_WM_STATE_ABOVE,
        Atom_NET_WM_STATE_BELOW,
        Atom_NET_WM_STATE_FULLSCREEN,
        Atom_NET_WM_STATE_MAXIMIZED_HORZ,
        Atom_NET_WM_STATE_MAXIMIZED_VERT,
        Atom_NET_WM_STATE_MODAL,
        Atom_NET_WM_STATE_STAYS_ON_TOP,
        Atom_NET_WM_STATE_DEMANDS_ATTENTION,
        Atom_NET_WM_STATE_HIDDEN,

        Atom_NET_WM_USER_TIME,
        Atom_NET_WM_USER_TIME_WINDOW,
        Atom_NET_WM_FULL_PLACEMENT,

        Atom_NET_WM_WINDOW_TYPE,
        Atom_NET_WM_WINDOW_TYPE_DESKTOP,
        Atom_NET_WM_WINDOW_TYPE_DOCK,
        Atom_NET_WM_WINDOW_TYPE_TOOLBAR,
        Atom_NET_WM_WINDOW_TYPE_MENU,
        Atom_NET_WM_WINDOW_TYPE_UTILITY,
        Atom_NET_WM_WINDOW_TYPE_SPLASH,
        Atom_NET_WM_WINDOW_TYPE_DIALOG,
        Atom_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        Atom_NET_WM_WINDOW_TYPE_POPUP_MENU,
        Atom_NET_WM_WINDOW_TYPE_TOOLTIP,
        Atom_NET_WM_WINDOW_TYPE_NOTIFICATION,
        Atom_NET_WM_WINDOW_TYPE_COMBO,
        Atom_NET_WM_WINDOW_TYPE_DND,
        Atom_NET_WM_WINDOW_TYPE_NORMAL,
        Atom_KDE_NET_WM_WINDOW_TYPE_OVERRIDE,

        Atom_KDE_NET_WM_FRAME_STRUT,
        Atom_NET_FRAME_EXTENTS,

        Atom_NET_STARTUP_INFO,
        Atom_NET_STARTUP_INFO_BEGIN,
        Atom_NET_STARTUP_ID,

        Atom_NET_SUPPORTING_WM_CHECK,

        Atom_NET_WM_CM_S0,

        Atom_NET_SYSTEM_TRAY_VISUAL,

        Atom_NET_ACTIVE_WINDOW,

        // Property formats
        AtomTEXT,
        AtomUTF8_STRING,
        AtomCARDINAL,

        // Xdnd
        AtomXdndEnter,
        AtomXdndPosition,
        AtomXdndStatus,
        AtomXdndLeave,
        AtomXdndDrop,
        AtomXdndFinished,
        AtomXdndTypelist,
        AtomXdndActionList,

        AtomXdndSelection,

        AtomXdndAware,
        AtomXdndProxy,

        AtomXdndActionCopy,
        AtomXdndActionLink,
        AtomXdndActionMove,
        AtomXdndActionAsk,
        AtomXdndActionPrivate,

        // Xkb
        Atom_XKB_RULES_NAMES,

        // XEMBED
        Atom_XEMBED,
        Atom_XEMBED_INFO,

        // XInput2
        AtomButtonLeft,
        AtomButtonMiddle,
        AtomButtonRight,
        AtomButtonWheelUp,
        AtomButtonWheelDown,
        AtomButtonHorizWheelLeft,
        AtomButtonHorizWheelRight,
        AtomAbsMTPositionX,
        AtomAbsMTPositionY,
        AtomAbsMTTouchMajor,
        AtomAbsMTTouchMinor,
        AtomAbsMTOrientation,
        AtomAbsMTPressure,
        AtomAbsMTTrackingID,
        AtomMaxContacts,
        AtomRelX,
        AtomRelY,
        // XInput2 tablet
        AtomAbsX,
        AtomAbsY,
        AtomAbsPressure,
        AtomAbsTiltX,
        AtomAbsTiltY,
        AtomAbsWheel,
        AtomAbsDistance,
        AtomWacomSerialIDs,
        AtomINTEGER,
        AtomRelHorizWheel,
        AtomRelVertWheel,
        AtomRelHorizScroll,
        AtomRelVertScroll,

        Atom_XSETTINGS_SETTINGS,

        Atom_COMPIZ_DECOR_PENDING,
        Atom_COMPIZ_DECOR_REQUEST,
        Atom_COMPIZ_DECOR_DELETE_PIXMAP,
        Atom_COMPIZ_TOOLKIT_ACTION,
        Atom_GTK_LOAD_ICONTHEMES,

        AtomAT_SPI_BUS,

        AtomEDID,
        AtomEDID_DATA,
        AtomXFree86_DDC_EDID1_RAWDATA,

        Atom_ICC_PROFILE,

        NAtoms
    };

    QXcbAtom();
    void initialize(xcb_connection_t *connection);

    inline xcb_atom_t atom(QXcbAtom::Atom atom) const { return m_allAtoms[atom]; }
    QXcbAtom::Atom qatom(xcb_atom_t atom) const;

protected:
    void initializeAllAtoms(xcb_connection_t *connection);

private:
    xcb_atom_t m_allAtoms[QXcbAtom::NAtoms];
};

#endif // QXCBATOM_H
