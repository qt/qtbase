/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QXCBCONNECTION_H
#define QXCBCONNECTION_H

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include <QtGui/private/qtguiglobal_p.h>
#include "qxcbexport.h"
#include <QHash>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QThread>
#include <QVector>
#include <QVarLengthArray>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/QLoggingCategory>
#include <QtCore/private/qglobal_p.h>

// This is needed to make Qt compile together with XKB. xkb.h is using a variable
// which is called 'explicit', this is a reserved keyword in c++
#if QT_CONFIG(xkb)
#define explicit dont_use_cxx_explicit
#include <xcb/xkb.h>
#undef explicit
#endif

#ifndef QT_NO_TABLETEVENT
#include <QTabletEvent>
#endif

#if XCB_USE_XINPUT2
#include <X11/extensions/XI2.h>
#ifdef XIScrollClass
#define XCB_USE_XINPUT21    // XI 2.1 adds smooth scrolling support
#ifdef XI_TouchBeginMask
#define XCB_USE_XINPUT22    // XI 2.2 adds multi-point touch support
#endif
#endif
struct XInput2TouchDeviceData;
#endif // XCB_USE_XINPUT2

struct xcb_randr_get_output_info_reply_t;

//#define Q_XCB_DEBUG

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaXInput)
Q_DECLARE_LOGGING_CATEGORY(lcQpaXInputDevices)
Q_DECLARE_LOGGING_CATEGORY(lcQpaXInputEvents)
Q_DECLARE_LOGGING_CATEGORY(lcQpaScreen)

class QXcbVirtualDesktop;
class QXcbScreen;
class QXcbWindow;
class QXcbDrag;
class QXcbKeyboard;
class QXcbClipboard;
class QXcbWMSupport;
class QXcbNativeInterface;
class QXcbSystemTrayTracker;
class QXcbGlIntegration;

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
        MANAGER, // System tray notification
        _NET_SYSTEM_TRAY_OPCODE, // System tray operation

        // ICCCM window state
        WM_STATE,
        WM_CHANGE_STATE,
        WM_CLASS,
        WM_NAME,

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

        // Qt/XCB specific
        _QT_CLOSE_CONNECTION,

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
        _NET_FRAME_EXTENTS,

        _NET_STARTUP_INFO,
        _NET_STARTUP_INFO_BEGIN,

        _NET_SUPPORTING_WM_CHECK,

        _NET_WM_CM_S0,

        _NET_SYSTEM_TRAY_VISUAL,

        _NET_ACTIVE_WINDOW,

        // Property formats
        TEXT,
        UTF8_STRING,
        CARDINAL,

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

        // XInput2
        ButtonLeft,
        ButtonMiddle,
        ButtonRight,
        ButtonWheelUp,
        ButtonWheelDown,
        ButtonHorizWheelLeft,
        ButtonHorizWheelRight,
        AbsMTPositionX,
        AbsMTPositionY,
        AbsMTTouchMajor,
        AbsMTTouchMinor,
        AbsMTOrientation,
        AbsMTPressure,
        AbsMTTrackingID,
        MaxContacts,
        RelX,
        RelY,
        // XInput2 tablet
        AbsX,
        AbsY,
        AbsPressure,
        AbsTiltX,
        AbsTiltY,
        AbsWheel,
        AbsDistance,
        WacomSerialIDs,
        INTEGER,
        RelHorizWheel,
        RelVertWheel,
        RelHorizScroll,
        RelVertScroll,

        _XSETTINGS_SETTINGS,

        _COMPIZ_DECOR_PENDING,
        _COMPIZ_DECOR_REQUEST,
        _COMPIZ_DECOR_DELETE_PIXMAP,
        _COMPIZ_TOOLKIT_ACTION,
        _GTK_LOAD_ICONTHEMES,

        NPredefinedAtoms,

        _QT_SETTINGS_TIMESTAMP = NPredefinedAtoms,
        NAtoms
    };
}

typedef QVarLengthArray<xcb_generic_event_t *, 64> QXcbEventArray;

class QXcbConnection;
class QXcbEventReader : public QThread
{
    Q_OBJECT
public:
    QXcbEventReader(QXcbConnection *connection);

    void run() Q_DECL_OVERRIDE;

    QXcbEventArray *lock();
    void unlock();

    void start();

    void registerEventDispatcher(QAbstractEventDispatcher *dispatcher);

signals:
    void eventPending();

private slots:
    void registerForEvents();

private:
    void addEvent(xcb_generic_event_t *event);

    QMutex m_mutex;
    QXcbEventArray m_events;
    QXcbConnection *m_connection;
};

class QXcbWindowEventListener
{
public:
    virtual ~QXcbWindowEventListener() {}
    virtual bool handleGenericEvent(xcb_generic_event_t *, long *) { return false; }

    virtual void handleExposeEvent(const xcb_expose_event_t *) {}
    virtual void handleClientMessageEvent(const xcb_client_message_event_t *) {}
    virtual void handleConfigureNotifyEvent(const xcb_configure_notify_event_t *) {}
    virtual void handleMapNotifyEvent(const xcb_map_notify_event_t *) {}
    virtual void handleUnmapNotifyEvent(const xcb_unmap_notify_event_t *) {}
    virtual void handleDestroyNotifyEvent(const xcb_destroy_notify_event_t *) {}
    virtual void handleButtonPressEvent(const xcb_button_press_event_t *) {}
    virtual void handleButtonReleaseEvent(const xcb_button_release_event_t *) {}
    virtual void handleMotionNotifyEvent(const xcb_motion_notify_event_t *) {}
    virtual void handleEnterNotifyEvent(const xcb_enter_notify_event_t *) {}
    virtual void handleLeaveNotifyEvent(const xcb_leave_notify_event_t *) {}
    virtual void handleFocusInEvent(const xcb_focus_in_event_t *) {}
    virtual void handleFocusOutEvent(const xcb_focus_out_event_t *) {}
    virtual void handlePropertyNotifyEvent(const xcb_property_notify_event_t *) {}
#ifdef XCB_USE_XINPUT22
    virtual void handleXIMouseEvent(xcb_ge_event_t *, Qt::MouseEventSource = Qt::MouseEventNotSynthesized) {}
    virtual void handleXIEnterLeave(xcb_ge_event_t *) {}
#endif
    virtual QXcbWindow *toWindow() { return 0; }
};

typedef QHash<xcb_window_t, QXcbWindowEventListener *> WindowMapper;

class QXcbSyncWindowRequest : public QEvent
{
public:
    QXcbSyncWindowRequest(QXcbWindow *w) : QEvent(QEvent::Type(QEvent::User + 1)), m_window(w) { }

    QXcbWindow *window() const { return m_window; }
    void invalidate();

private:
    QXcbWindow *m_window;
};

class QAbstractEventDispatcher;
class Q_XCB_EXPORT QXcbConnection : public QObject
{
    Q_OBJECT
public:
    QXcbConnection(QXcbNativeInterface *nativeInterface, bool canGrabServer, xcb_visualid_t defaultVisualId, const char *displayName = 0);
    ~QXcbConnection();

    QXcbConnection *connection() const { return const_cast<QXcbConnection *>(this); }

    const QList<QXcbVirtualDesktop *> &virtualDesktops() const { return m_virtualDesktops; }
    const QList<QXcbScreen *> &screens() const { return m_screens; }
    int primaryScreenNumber() const { return m_primaryScreenNumber; }
    QXcbVirtualDesktop *primaryVirtualDesktop() const { return m_virtualDesktops.value(m_primaryScreenNumber); }
    QXcbScreen *primaryScreen() const;

    inline xcb_atom_t atom(QXcbAtom::Atom atom) const { return m_allAtoms[atom]; }
    QXcbAtom::Atom qatom(xcb_atom_t atom) const;
    xcb_atom_t internAtom(const char *name);
    QByteArray atomName(xcb_atom_t atom);

    const char *displayName() const { return m_displayName.constData(); }
    xcb_connection_t *xcb_connection() const { return m_connection; }
    const xcb_setup_t *setup() const { return m_setup; }
    const xcb_format_t *formatForDepth(uint8_t depth) const;

    QXcbKeyboard *keyboard() const { return m_keyboard; }

#ifndef QT_NO_CLIPBOARD
    QXcbClipboard *clipboard() const { return m_clipboard; }
#endif
#ifndef QT_NO_DRAGANDDROP
    QXcbDrag *drag() const { return m_drag; }
#endif

    QXcbWMSupport *wmSupport() const { return m_wmSupport.data(); }
    xcb_window_t rootWindow();
    xcb_window_t clientLeader();

    bool hasDefaultVisualId() const { return m_defaultVisualId != UINT_MAX; }
    xcb_visualid_t defaultVisualId() const { return m_defaultVisualId; }

#ifdef XCB_USE_XLIB
    void *xlib_display() const;
    void *createVisualInfoForDefaultVisualId() const;
#endif

#if defined(XCB_USE_XINPUT2)
    void xi2Select(xcb_window_t window);
#endif
#ifdef XCB_USE_XINPUT21
    bool isAtLeastXI21() const { return m_xi2Enabled && m_xi2Minor >= 1; }
#else
    bool isAtLeastXI21() const { return false; }
#endif
#ifdef XCB_USE_XINPUT22
    bool isAtLeastXI22() const { return m_xi2Enabled && m_xi2Minor >= 2; }
#else
    bool isAtLeastXI22() const { return false; }
#endif

    void sync();

    void handleXcbError(xcb_generic_error_t *error);
    void handleXcbEvent(xcb_generic_event_t *event);

    void addWindowEventListener(xcb_window_t id, QXcbWindowEventListener *eventListener);
    void removeWindowEventListener(xcb_window_t id);
    QXcbWindowEventListener *windowEventListenerFromId(xcb_window_t id);
    QXcbWindow *platformWindowFromId(xcb_window_t id);

    template<typename T>
    inline xcb_generic_event_t *checkEvent(T &checker);

    typedef bool (*PeekFunc)(QXcbConnection *, xcb_generic_event_t *);
    void addPeekFunc(PeekFunc f);

    inline xcb_timestamp_t time() const { return m_time; }
    inline void setTime(xcb_timestamp_t t) { if (t > m_time) m_time = t; }

    inline xcb_timestamp_t netWmUserTime() const { return m_netWmUserTime; }
    inline void setNetWmUserTime(xcb_timestamp_t t) { if (t > m_netWmUserTime) m_netWmUserTime = t; }

    bool hasXFixes() const { return xfixes_first_event > 0; }
    bool hasXShape() const { return has_shape_extension; }
    bool hasXRandr() const { return has_randr_extension; }
    bool hasInputShape() const { return has_input_shape; }
    bool hasXKB() const { return has_xkb; }

    bool supportsThreadedRendering() const { return m_reader->isRunning(); }
    bool threadedEventHandling() const { return m_reader->isRunning(); }

    xcb_timestamp_t getTimestamp();
    xcb_window_t getSelectionOwner(xcb_atom_t atom) const;
    xcb_window_t getQtSelectionOwner();

    void setButton(Qt::MouseButton button, bool down) { m_buttons.setFlag(button, down); }
    Qt::MouseButtons buttons() const { return m_buttons; }
    Qt::MouseButton translateMouseButton(xcb_button_t s);

    QXcbWindow *focusWindow() const { return m_focusWindow; }
    void setFocusWindow(QXcbWindow *);
    QXcbWindow *mouseGrabber() const { return m_mouseGrabber; }
    void setMouseGrabber(QXcbWindow *);
    QXcbWindow *mousePressWindow() const { return m_mousePressWindow; }
    void setMousePressWindow(QXcbWindow *);

    QByteArray startupId() const { return m_startupId; }
    void setStartupId(const QByteArray &nextId) { m_startupId = nextId; }
    void clearStartupId() { m_startupId.clear(); }

    void grabServer();
    void ungrabServer();

    QXcbNativeInterface *nativeInterface() const { return m_nativeInterface; }

    QXcbSystemTrayTracker *systemTrayTracker() const;
    static bool xEmbedSystemTrayAvailable();
    static bool xEmbedSystemTrayVisualHasAlphaChannel();

#ifdef XCB_USE_XINPUT21
    void handleEnterEvent();
#endif

#ifdef XCB_USE_XINPUT22
    bool xi2SetMouseGrabEnabled(xcb_window_t w, bool grab);
#endif
    Qt::MouseButton xiToQtMouseButton(uint32_t b);

    QXcbEventReader *eventReader() const { return m_reader; }

    bool canGrab() const { return m_canGrabServer; }

    QXcbGlIntegration *glIntegration() const { return m_glIntegration; }

#ifdef XCB_USE_XINPUT22
    bool xi2MouseEvents() const;
#endif

protected:
    bool event(QEvent *e) Q_DECL_OVERRIDE;

public slots:
    void flush() { xcb_flush(m_connection); }

private slots:
    void processXcbEvents();

private:
    void initializeAllAtoms();
    void sendConnectionEvent(QXcbAtom::Atom atom, uint id = 0);
    void initializeXFixes();
    void initializeXRender();
    void initializeXRandr();
    void initializeXinerama();
    void initializeXShape();
    void initializeXKB();
    void handleClientMessageEvent(const xcb_client_message_event_t *event);
    QXcbScreen* findScreenForCrtc(xcb_window_t rootWindow, xcb_randr_crtc_t crtc) const;
    QXcbScreen* findScreenForOutput(xcb_window_t rootWindow, xcb_randr_output_t output) const;
    QXcbVirtualDesktop* virtualDesktopForRootWindow(xcb_window_t rootWindow) const;
    void updateScreens(const xcb_randr_notify_event_t *event);
    bool checkOutputIsPrimary(xcb_window_t rootWindow, xcb_randr_output_t output);
    void updateScreen(QXcbScreen *screen, const xcb_randr_output_change_t &outputChange);
    QXcbScreen *createScreen(QXcbVirtualDesktop *virtualDesktop,
                             const xcb_randr_output_change_t &outputChange,
                             xcb_randr_get_output_info_reply_t *outputInfo);
    void destroyScreen(QXcbScreen *screen);
    void initializeScreens();
    bool compressEvent(xcb_generic_event_t *event, int currentIndex, QXcbEventArray *eventqueue) const;

#ifdef XCB_USE_XINPUT2
    bool m_xi2Enabled;
    int m_xi2Minor;
    void initializeXInput2();
    void finalizeXInput2();
    void xi2SetupDevices();
    XInput2TouchDeviceData *touchDeviceForId(int id);
    void xi2HandleEvent(xcb_ge_event_t *event);
    void xi2HandleHierachyEvent(void *event);
    void xi2HandleDeviceChangedEvent(void *event);
    int m_xiOpCode, m_xiEventBase, m_xiErrorBase;
#ifdef XCB_USE_XINPUT22
    void xi2ProcessTouch(void *xiDevEvent, QXcbWindow *platformWindow);
#endif // XCB_USE_XINPUT22
#ifndef QT_NO_TABLETEVENT
    struct TabletData {
        TabletData() : deviceId(0), pointerType(QTabletEvent::UnknownPointer),
            tool(QTabletEvent::Stylus), buttons(0), serialId(0), inProximity(false) { }
        int deviceId;
        QTabletEvent::PointerType pointerType;
        QTabletEvent::TabletDevice tool;
        Qt::MouseButtons buttons;
        qint64 serialId;
        bool inProximity;
        struct ValuatorClassInfo {
            ValuatorClassInfo() : minVal(0.), maxVal(0.), curVal(0.) { }
            double minVal;
            double maxVal;
            double curVal;
            int number;
        };
        QHash<int, ValuatorClassInfo> valuatorInfo;
    };
    friend class QTypeInfo<TabletData>;
    friend class QTypeInfo<TabletData::ValuatorClassInfo>;
    bool xi2HandleTabletEvent(const void *event, TabletData *tabletData);
    void xi2ReportTabletEvent(const void *event, TabletData *tabletData);
    QVector<TabletData> m_tabletData;
    TabletData *tabletDataForDevice(int id);
#endif // !QT_NO_TABLETEVENT
    struct ScrollingDevice {
        ScrollingDevice() : deviceId(0), verticalIndex(0), horizontalIndex(0), orientations(0), legacyOrientations(0) { }
        int deviceId;
        int verticalIndex, horizontalIndex;
        double verticalIncrement, horizontalIncrement;
        Qt::Orientations orientations;
        Qt::Orientations legacyOrientations;
        QPointF lastScrollPosition;
    };
    void updateScrollingDevice(ScrollingDevice& scrollingDevice, int num_classes, void *classes);
    void xi2HandleScrollEvent(void *event, ScrollingDevice &scrollingDevice);
    QHash<int, ScrollingDevice> m_scrollingDevices;

    static bool xi2GetValuatorValueIfSet(const void *event, int valuatorNum, double *value);
    static void xi2PrepareXIGenericDeviceEvent(xcb_ge_event_t *event);
#endif

    xcb_connection_t *m_connection;
    const xcb_setup_t *m_setup;
    bool m_canGrabServer;
    xcb_visualid_t m_defaultVisualId;

    QList<QXcbVirtualDesktop *> m_virtualDesktops;
    QList<QXcbScreen *> m_screens;
    int m_primaryScreenNumber;

    xcb_atom_t m_allAtoms[QXcbAtom::NAtoms];

    xcb_timestamp_t m_time;
    xcb_timestamp_t m_netWmUserTime;

    QByteArray m_displayName;

    QXcbKeyboard *m_keyboard;
#ifndef QT_NO_CLIPBOARD
    QXcbClipboard *m_clipboard;
#endif
#ifndef QT_NO_DRAGANDDROP
    QXcbDrag *m_drag;
#endif
    QScopedPointer<QXcbWMSupport> m_wmSupport;
    QXcbNativeInterface *m_nativeInterface;

#if defined(XCB_USE_XLIB)
    void *m_xlib_display;
#endif
    QXcbEventReader *m_reader;
#if defined(XCB_USE_XINPUT2)
    QHash<int, XInput2TouchDeviceData*> m_touchDevices;
#endif
#ifdef Q_XCB_DEBUG
    struct CallInfo {
        int sequence;
        QByteArray file;
        int line;
    };
    QVector<CallInfo> m_callLog;
    QMutex m_callLogMutex;
    void log(const char *file, int line, int sequence);
    template <typename cookie_t>
    friend cookie_t q_xcb_call_template(const cookie_t &cookie, QXcbConnection *connection,
                                        const char *file, int line);
    template <typename reply_t>
    friend reply_t *q_xcb_call_template(reply_t *reply, QXcbConnection *connection,
                                        const char *file, int line);
#endif

    WindowMapper m_mapper;

    QVector<PeekFunc> m_peekFuncs;

    uint32_t xfixes_first_event;
    uint32_t xrandr_first_event;
    uint32_t xkb_first_event;

    bool has_xinerama_extension;
    bool has_shape_extension;
    bool has_randr_extension;
    bool has_input_shape;
    bool has_xkb;

    Qt::MouseButtons m_buttons;

    QXcbWindow *m_focusWindow;
    QXcbWindow *m_mouseGrabber;
    QXcbWindow *m_mousePressWindow;

    xcb_window_t m_clientLeader;
    QByteArray m_startupId;
    QXcbSystemTrayTracker *m_systemTrayTracker;
    QXcbGlIntegration *m_glIntegration;
    bool m_xiGrab;

    xcb_window_t m_qtSelectionOwner;

    friend class QXcbEventReader;
};
#ifdef XCB_USE_XINPUT2
#ifndef QT_NO_TABLETEVENT
Q_DECLARE_TYPEINFO(QXcbConnection::TabletData::ValuatorClassInfo, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(QXcbConnection::TabletData, Q_MOVABLE_TYPE);
#endif
#endif

#define DISPLAY_FROM_XCB(object) ((Display *)(object->connection()->xlib_display()))
#define CREATE_VISUALINFO_FROM_DEFAULT_VISUALID(object) ((XVisualInfo *)(object->connection()->createVisualInfoForDefaultVisualId()))

template<typename T>
xcb_generic_event_t *QXcbConnection::checkEvent(T &checker)
{
    QXcbEventArray *eventqueue = m_reader->lock();

    for (int i = 0; i < eventqueue->size(); ++i) {
        xcb_generic_event_t *event = eventqueue->at(i);
        if (checker.checkEvent(event)) {
            (*eventqueue)[i] = 0;
            m_reader->unlock();
            return event;
        }
    }
    m_reader->unlock();
    return 0;
}

class QXcbConnectionGrabber
{
public:
    QXcbConnectionGrabber(QXcbConnection *connection);
    ~QXcbConnectionGrabber();
    void release();
private:
    QXcbConnection *m_connection;
};

#ifdef Q_XCB_DEBUG
template <typename cookie_t>
cookie_t q_xcb_call_template(const cookie_t &cookie, QXcbConnection *connection, const char *file,
                             int line)
{
    connection->log(file, line, cookie.sequence);
    return cookie;
}

template <typename reply_t>
reply_t *q_xcb_call_template(reply_t *reply, QXcbConnection *connection, const char *file, int line)
{
    connection->log(file, line, reply->sequence);
    return reply;
}
#define Q_XCB_CALL(x) q_xcb_call_template(x, connection(), __FILE__, __LINE__)
#define Q_XCB_CALL2(x, connection) q_xcb_call_template(x, connection, __FILE__, __LINE__)
#define Q_XCB_NOOP(c) q_xcb_call_template(xcb_no_operation(c->xcb_connection()), c, __FILE__, __LINE__);
#else
#define Q_XCB_CALL(x) x
#define Q_XCB_CALL2(x, connection) x
#define Q_XCB_NOOP(c) (void)c;
#endif

QT_END_NAMESPACE

#endif
