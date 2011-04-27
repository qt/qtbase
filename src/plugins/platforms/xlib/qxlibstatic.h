/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTESTLITESTATICINFO_H
#define QTESTLITESTATICINFO_H

#include <QtCore/QTextStream>
#include <QtCore/QDataStream>
#include <QtCore/QMetaType>
#include <QtCore/QVariant>

#if defined(_XLIB_H_) // crude hack, but...
#error "cannot include <X11/Xlib.h> before this file"
#endif
#define XRegisterIMInstantiateCallback qt_XRegisterIMInstantiateCallback
#define XUnregisterIMInstantiateCallback qt_XUnregisterIMInstantiateCallback
#define XSetIMValues qt_XSetIMValues
#include <X11/Xlib.h>
#undef XRegisterIMInstantiateCallback
#undef XUnregisterIMInstantiateCallback
#undef XSetIMValues

#include <X11/Xutil.h>
#include <X11/Xos.h>
#ifdef index
#  undef index
#endif
#ifdef rindex
#  undef rindex
#endif
#ifdef Q_OS_VXWORS
#  ifdef open
#    undef open
#  endif
#  ifdef getpid
#    undef getpid
#  endif
#endif // Q_OS_VXWORKS
#include <X11/Xatom.h>

//#define QT_NO_SHAPE
#ifdef QT_NO_SHAPE
#  define XShapeCombineRegion(a,b,c,d,e,f,g)
#  define XShapeCombineMask(a,b,c,d,e,f,g)
#else
#  include <X11/extensions/shape.h>
#endif // QT_NO_SHAPE


#if !defined (QT_NO_TABLET)
#  include <X11/extensions/XInput.h>
#if defined (Q_OS_IRIX)
#  include <X11/extensions/SGIMisc.h>
#  include <wacom.h>
#endif
#endif // QT_NO_TABLET


// #define QT_NO_XINERAMA
#ifndef QT_NO_XINERAMA
// XFree86 does not C++ify Xinerama (at least up to XFree86 4.0.3).
extern "C" {
#  include <X11/extensions/Xinerama.h>
}
#endif // QT_NO_XINERAMA

// #define QT_NO_XRANDR
#ifndef QT_NO_XRANDR
#  include <X11/extensions/Xrandr.h>
#endif // QT_NO_XRANDR

// #define QT_NO_XRENDER
#ifndef QT_NO_XRENDER
#  include <X11/extensions/Xrender.h>
#endif // QT_NO_XRENDER

#ifndef QT_NO_XSYNC
extern "C" {
#  include "X11/extensions/sync.h"
}
#endif

// #define QT_NO_XKB
#ifndef QT_NO_XKB
#  include <X11/XKBlib.h>
#endif // QT_NO_XKB


#if !defined(XlibSpecificationRelease)
#  define X11R4
typedef char *XPointer;
#else
#  undef X11R4
#endif

#ifndef QT_NO_XFIXES
typedef Bool (*PtrXFixesQueryExtension)(Display *, int *, int *);
typedef Status (*PtrXFixesQueryVersion)(Display *, int *, int *);
typedef void (*PtrXFixesSetCursorName)(Display *dpy, Cursor cursor, const char *name);
typedef void (*PtrXFixesSelectSelectionInput)(Display *dpy, Window win, Atom selection, unsigned long eventMask);
#endif // QT_NO_XFIXES

#ifndef QT_NO_XCURSOR
#include <X11/Xcursor/Xcursor.h>
typedef Cursor (*PtrXcursorLibraryLoadCursor)(Display *, const char *);
#endif // QT_NO_XCURSOR

#ifndef QT_NO_XINERAMA
typedef Bool (*PtrXineramaQueryExtension)(Display *dpy, int *event_base, int *error_base);
typedef Bool (*PtrXineramaIsActive)(Display *dpy);
typedef XineramaScreenInfo *(*PtrXineramaQueryScreens)(Display *dpy, int *number);
#endif // QT_NO_XINERAMA

#ifndef QT_NO_XRANDR
typedef void (*PtrXRRSelectInput)(Display *, Window, int);
typedef int (*PtrXRRUpdateConfiguration)(XEvent *);
typedef int (*PtrXRRRootToScreen)(Display *, Window);
typedef Bool (*PtrXRRQueryExtension)(Display *, int *, int *);
#endif // QT_NO_XRANDR

#ifndef QT_NO_XINPUT
typedef int (*PtrXCloseDevice)(Display *, XDevice *);
typedef XDeviceInfo* (*PtrXListInputDevices)(Display *, int *);
typedef XDevice* (*PtrXOpenDevice)(Display *, XID);
typedef void (*PtrXFreeDeviceList)(XDeviceInfo *);
typedef int (*PtrXSelectExtensionEvent)(Display *, Window, XEventClass *, int);
#endif // QT_NO_XINPUT

/*
 * Solaris patch 108652-47 and higher fixes crases in
 * XRegisterIMInstantiateCallback, but the function doesn't seem to
 * work.
 *
 * Instead, we disabled R6 input, and open the input method
 * immediately at application start.
 */

//######### XFree86 has wrong declarations for XRegisterIMInstantiateCallback
//######### and XUnregisterIMInstantiateCallback in at least version 3.3.2.
//######### Many old X11R6 header files lack XSetIMValues.
//######### Therefore, we have to declare these functions ourselves.

extern "C" Bool XRegisterIMInstantiateCallback(
    Display*,
    struct _XrmHashBucketRec*,
    char*,
    char*,
    XIMProc, //XFree86 has XIDProc, which has to be wrong
    XPointer
);

extern "C" Bool XUnregisterIMInstantiateCallback(
    Display*,
    struct _XrmHashBucketRec*,
    char*,
    char*,
    XIMProc, //XFree86 has XIDProc, which has to be wrong
    XPointer
);

#ifndef X11R4
#  include <X11/Xlocale.h>
#endif // X11R4


#ifndef QT_NO_MITSHM
#  include <X11/extensions/XShm.h>
#endif // QT_NO_MITSHM

// rename a couple of X defines to get rid of name clashes
// resolve the conflict between X11's FocusIn and QEvent::FocusIn
enum {
    XFocusOut = FocusOut,
    XFocusIn = FocusIn,
    XKeyPress = KeyPress,
    XKeyRelease = KeyRelease,
    XNone = None,
    XRevertToParent = RevertToParent,
    XGrayScale = GrayScale,
    XCursorShape = CursorShape
};
#undef FocusOut
#undef FocusIn
#undef KeyPress
#undef KeyRelease
#undef None
#undef RevertToParent
#undef GrayScale
#undef CursorShape

#ifdef FontChange
#undef FontChange
#endif


class QXlibStatic
{
public:
    enum X11Atom {
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

    static Atom atom(X11Atom atom);
    static bool isSupportedByWM(Atom atom);

    static bool useXFixes();
    static int xFixesEventBase();

    #ifndef QT_NO_XFIXES
    static PtrXFixesSelectSelectionInput xFixesSelectSelectionInput();
    #endif //QT_NO_XFIXES

    static QImage qimageFromXImage(XImage *xi);


};

#endif // QTESTLITESTATICINFO_H
