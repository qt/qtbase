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

#include <cstdlib>
#include <memory>

// This is needed to make Qt compile together with XKB. xkb.h is using a variable
// which is called 'explicit', this is a reserved keyword in c++
#if QT_CONFIG(xkb)
#define explicit dont_use_cxx_explicit
#include <xcb/xkb.h>
#undef explicit
#endif

#if QT_CONFIG(tabletevent)
#include <QTabletEvent>
#endif

#if QT_CONFIG(xinput2)
#include <X11/extensions/XI2.h>
#ifdef XIScrollClass
#define XCB_USE_XINPUT21    // XI 2.1 adds smooth scrolling support
#ifdef XI_TouchBeginMask
#define XCB_USE_XINPUT22    // XI 2.2 adds multi-point touch support
#endif
#endif
#endif // QT_CONFIG(xinput2)

struct xcb_randr_get_output_info_reply_t;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaXInput)
Q_DECLARE_LOGGING_CATEGORY(lcQpaXInputDevices)
Q_DECLARE_LOGGING_CATEGORY(lcQpaXInputEvents)
Q_DECLARE_LOGGING_CATEGORY(lcQpaScreen)
Q_DECLARE_LOGGING_CATEGORY(lcQpaEvents)
Q_DECLARE_LOGGING_CATEGORY(lcQpaXcb)
Q_DECLARE_LOGGING_CATEGORY(lcQpaPeeker)
Q_DECLARE_LOGGING_CATEGORY(lcQpaKeyboard)

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

    void run() override;

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
#if QT_CONFIG(xinput2)
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
    bool isConnected() const;

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

    bool imageNeedsEndianSwap() const
    {
        if (!hasShm())
            return false; // The non-Shm path does its own swapping
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        return m_setup->image_byte_order != XCB_IMAGE_ORDER_MSB_FIRST;
#else
        return m_setup->image_byte_order != XCB_IMAGE_ORDER_LSB_FIRST;
#endif
    }

    QXcbKeyboard *keyboard() const { return m_keyboard; }

#ifndef QT_NO_CLIPBOARD
    QXcbClipboard *clipboard() const { return m_clipboard; }
#endif
#if QT_CONFIG(draganddrop)
    QXcbDrag *drag() const { return m_drag; }
#endif

    QXcbWMSupport *wmSupport() const { return m_wmSupport.data(); }
    xcb_window_t rootWindow();
    xcb_window_t clientLeader();

    bool hasDefaultVisualId() const { return m_defaultVisualId != UINT_MAX; }
    xcb_visualid_t defaultVisualId() const { return m_defaultVisualId; }

#if QT_CONFIG(xcb_xlib)
    void *xlib_display() const;
    void *createVisualInfoForDefaultVisualId() const;
#endif
    void sync();

    void handleXcbError(xcb_generic_error_t *error);
    void printXcbError(const char *message, xcb_generic_error_t *error);
    void handleXcbEvent(xcb_generic_event_t *event);
    void printXcbEvent(const QLoggingCategory &log, const char *message,
                       xcb_generic_event_t *event) const;

    void addWindowEventListener(xcb_window_t id, QXcbWindowEventListener *eventListener);
    void removeWindowEventListener(xcb_window_t id);
    QXcbWindowEventListener *windowEventListenerFromId(xcb_window_t id);
    QXcbWindow *platformWindowFromId(xcb_window_t id);

    template<typename T>
    inline xcb_generic_event_t *checkEvent(T &checker);

    typedef bool (*PeekFunc)(QXcbConnection *, xcb_generic_event_t *);
    void addPeekFunc(PeekFunc f);

    // Peek at all queued events
    qint32 generatePeekerId();
    bool removePeekerId(qint32 peekerId);
    enum PeekOption { PeekDefault = 0, PeekFromCachedIndex = 1 }; // see qx11info_x11.h
    Q_DECLARE_FLAGS(PeekOptions, PeekOption)
    typedef bool (*PeekerCallback)(xcb_generic_event_t *event, void *peekerData);
    bool peekEventQueue(PeekerCallback peeker, void *peekerData = nullptr,
                        PeekOptions option = PeekDefault, qint32 peekerId = -1);

    inline xcb_timestamp_t time() const { return m_time; }
    inline void setTime(xcb_timestamp_t t) { if (t > m_time) m_time = t; }

    inline xcb_timestamp_t netWmUserTime() const { return m_netWmUserTime; }
    inline void setNetWmUserTime(xcb_timestamp_t t) { if (t > m_netWmUserTime) m_netWmUserTime = t; }

    bool hasXFixes() const { return has_xfixes; }
    bool hasXShape() const { return has_shape_extension; }
    bool hasXRandr() const { return has_randr_extension; }
    bool hasInputShape() const { return has_input_shape; }
    bool hasXKB() const { return has_xkb; }
    bool hasXRender(int major = -1, int minor = -1) const
    {
        if (has_render_extension && major != -1 && minor != -1)
            return m_xrenderVersion >= qMakePair(major, minor);

        return has_render_extension;
    }
    bool hasXInput2() const { return m_xi2Enabled; }
    bool hasShm() const { return has_shm; }
    bool hasShmFd() const { return has_shm_fd; }

    bool threadedEventHandling() const { return m_reader->isRunning(); }

    xcb_timestamp_t getTimestamp();
    xcb_window_t getSelectionOwner(xcb_atom_t atom) const;
    xcb_window_t getQtSelectionOwner();

    void setButtonState(Qt::MouseButton button, bool down);
    Qt::MouseButtons buttonState() const { return m_buttonState; }
    Qt::MouseButton button() const { return m_button; }
    Qt::MouseButton translateMouseButton(xcb_button_t s);

    QXcbWindow *focusWindow() const { return m_focusWindow; }
    void setFocusWindow(QWindow *);
    QXcbWindow *mouseGrabber() const { return m_mouseGrabber; }
    void setMouseGrabber(QXcbWindow *);
    QXcbWindow *mousePressWindow() const { return m_mousePressWindow; }
    void setMousePressWindow(QXcbWindow *);

    QByteArray startupId() const { return m_startupId; }
    void setStartupId(const QByteArray &nextId) { m_startupId = nextId; }
    void clearStartupId() { m_startupId.clear(); }

    void grabServer();
    void ungrabServer();

    bool isUnity() const { return m_xdgCurrentDesktop == "unity"; }
    bool isGnome() const { return m_xdgCurrentDesktop == "gnome"; }

    QXcbNativeInterface *nativeInterface() const { return m_nativeInterface; }

    QXcbSystemTrayTracker *systemTrayTracker() const;
    static bool xEmbedSystemTrayAvailable();
    static bool xEmbedSystemTrayVisualHasAlphaChannel();

#if QT_CONFIG(xinput2)
    void xi2SelectStateEvents();
    void xi2SelectDeviceEvents(xcb_window_t window);
    void xi2SelectDeviceEventsCompatibility(xcb_window_t window);
    bool xi2SetMouseGrabEnabled(xcb_window_t w, bool grab);
    bool xi2MouseEventsDisabled() const;
    bool isAtLeastXI21() const { return m_xi2Enabled && m_xi2Minor >= 1; }
    bool isAtLeastXI22() const { return m_xi2Enabled && m_xi2Minor >= 2; }
    Qt::MouseButton xiToQtMouseButton(uint32_t b);
#ifdef XCB_USE_XINPUT21
    void xi2UpdateScrollingDevices();
#endif
#ifdef XCB_USE_XINPUT22
    bool startSystemMoveResizeForTouchBegin(xcb_window_t window, const QPoint &point, int corner);
    void abortSystemMoveResizeForTouch();
    bool isTouchScreen(int id);
#endif
#endif
    QXcbEventReader *eventReader() const { return m_reader; }

    bool canGrab() const { return m_canGrabServer; }

    QXcbGlIntegration *glIntegration() const { return m_glIntegration; }

protected:
    bool event(QEvent *e) override;

public slots:
    void flush() { xcb_flush(m_connection); }

private slots:
    void processXcbEvents();

private:
    void initializeAllAtoms();
    void sendConnectionEvent(QXcbAtom::Atom atom, uint id = 0);
    void initializeShm();
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

    bool m_xi2Enabled = false;
#if QT_CONFIG(xinput2)
    int m_xi2Minor = -1;
    void initializeXInput2();
    void xi2SetupDevice(void *info, bool removeExisting = true);
    void xi2SetupDevices();
    struct TouchDeviceData {
        QTouchDevice *qtTouchDevice = nullptr;
        QHash<int, QWindowSystemInterface::TouchPoint> touchPoints;
        QHash<int, QPointF> pointPressedPosition; // in screen coordinates where each point was pressed
        struct ValuatorClassInfo {
            double min = 0;
            double max = 0;
            int number = -1;
            QXcbAtom::Atom label;
        };
        QVector<ValuatorClassInfo> valuatorInfo;

        // Stuff that is relevant only for touchpads
        QPointF firstPressedPosition;        // in screen coordinates where the first point was pressed
        QPointF firstPressedNormalPosition;  // device coordinates (0 to 1, 0 to 1) where the first point was pressed
        QSizeF size;                         // device size in mm
        bool providesTouchOrientation = false;
    };
    TouchDeviceData *populateTouchDevices(void *info);
    TouchDeviceData *touchDeviceForId(int id);
    void xi2HandleEvent(xcb_ge_event_t *event);
    void xi2HandleHierarchyEvent(void *event);
    void xi2HandleDeviceChangedEvent(void *event);
    int m_xiOpCode, m_xiEventBase, m_xiErrorBase;
#ifdef XCB_USE_XINPUT22
    void xi2ProcessTouch(void *xiDevEvent, QXcbWindow *platformWindow);
#endif // XCB_USE_XINPUT22
#if QT_CONFIG(tabletevent)
    struct TabletData {
        int deviceId = 0;
        QTabletEvent::PointerType pointerType = QTabletEvent::UnknownPointer;
        QTabletEvent::TabletDevice tool = QTabletEvent::Stylus;
        Qt::MouseButtons buttons = 0;
        qint64 serialId = 0;
        bool inProximity = false;
        struct ValuatorClassInfo {
            double minVal = 0;
            double maxVal = 0;
            double curVal = 0;
            int number = -1;
        };
        QHash<int, ValuatorClassInfo> valuatorInfo;
    };
    friend class QTypeInfo<TabletData>;
    friend class QTypeInfo<TabletData::ValuatorClassInfo>;
    bool xi2HandleTabletEvent(const void *event, TabletData *tabletData);
    void xi2ReportTabletEvent(const void *event, TabletData *tabletData);
    QVector<TabletData> m_tabletData;
    TabletData *tabletDataForDevice(int id);
#endif // QT_CONFIG(tabletevent)
    struct ScrollingDevice {
        int deviceId = 0;
        int verticalIndex = 0;
        int horizontalIndex = 0;
        double verticalIncrement = 0;
        double horizontalIncrement = 0;
        Qt::Orientations orientations = 0;
        Qt::Orientations legacyOrientations = 0;
        QPointF lastScrollPosition;
    };
    QHash<int, ScrollingDevice> m_scrollingDevices;
#ifdef XCB_USE_XINPUT21
    void xi2HandleScrollEvent(void *event, ScrollingDevice &scrollingDevice);
    void xi2UpdateScrollingDevice(ScrollingDevice &scrollingDevice);
    ScrollingDevice *scrollingDeviceForId(int id);
#endif

    static bool xi2GetValuatorValueIfSet(const void *event, int valuatorNum, double *value);
    static void xi2PrepareXIGenericDeviceEvent(xcb_ge_event_t *event);
#endif

    xcb_connection_t *m_connection = nullptr;
    const xcb_setup_t *m_setup = nullptr;
    const bool m_canGrabServer;
    const xcb_visualid_t m_defaultVisualId;

    QList<QXcbVirtualDesktop *> m_virtualDesktops;
    QList<QXcbScreen *> m_screens;
    int m_primaryScreenNumber = 0;

    xcb_atom_t m_allAtoms[QXcbAtom::NAtoms];

    xcb_timestamp_t m_time = XCB_CURRENT_TIME;
    xcb_timestamp_t m_netWmUserTime = XCB_CURRENT_TIME;

    QByteArray m_displayName;

    QXcbKeyboard *m_keyboard = nullptr;
#ifndef QT_NO_CLIPBOARD
    QXcbClipboard *m_clipboard = nullptr;
#endif
#if QT_CONFIG(draganddrop)
    QXcbDrag *m_drag = nullptr;
#endif
    QScopedPointer<QXcbWMSupport> m_wmSupport;
    QXcbNativeInterface *m_nativeInterface = nullptr;

#if QT_CONFIG(xcb_xlib)
    void *m_xlib_display = nullptr;
#endif
    QXcbEventReader *m_reader = nullptr;

#if QT_CONFIG(xinput2)
    QHash<int, TouchDeviceData> m_touchDevices;
#ifdef XCB_USE_XINPUT22
    struct StartSystemMoveResizeInfo {
        xcb_window_t window = XCB_NONE;
        uint16_t deviceid;
        uint32_t pointid;
        int corner;
    } m_startSystemMoveResizeInfo;
#endif
#endif
    WindowMapper m_mapper;

    QVector<PeekFunc> m_peekFuncs;

    uint32_t xfixes_first_event = 0;
    uint32_t xrandr_first_event = 0;
    uint32_t xkb_first_event = 0;

    bool has_xfixes = false;
    bool has_xinerama_extension = false;
    bool has_shape_extension = false;
    bool has_randr_extension = false;
    bool has_input_shape;
    bool has_xkb = false;
    bool has_render_extension = false;
    bool has_shm = false;
    bool has_shm_fd = false;

    QPair<int, int> m_xrenderVersion;

    Qt::MouseButtons m_buttonState = 0;
    Qt::MouseButton m_button = Qt::NoButton;

    QXcbWindow *m_focusWindow = nullptr;
    QXcbWindow *m_mouseGrabber = nullptr;
    QXcbWindow *m_mousePressWindow = nullptr;

    xcb_window_t m_clientLeader = 0;
    QByteArray m_startupId;
    QXcbSystemTrayTracker *m_systemTrayTracker = nullptr;
    QXcbGlIntegration *m_glIntegration = nullptr;
    bool m_xiGrab = false;
    QVector<int> m_xiMasterPointerIds;

    xcb_window_t m_qtSelectionOwner = 0;

    bool m_mainEventLoopFlushedQueue = false;
    qint32 m_peekerIdSource = 0;
    bool m_peekerIndexCacheDirty = false;
    QHash<qint32, qint32> m_peekerToCachedIndex;
    friend class QXcbEventReader;

    QByteArray m_xdgCurrentDesktop;
};
#if QT_CONFIG(xinput2)
#if QT_CONFIG(tabletevent)
Q_DECLARE_TYPEINFO(QXcbConnection::TabletData::ValuatorClassInfo, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(QXcbConnection::TabletData, Q_MOVABLE_TYPE);
#endif
#endif

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

#define Q_XCB_REPLY_CONNECTION_ARG(connection, ...) connection

struct QStdFreeDeleter {
    void operator()(void *p) const Q_DECL_NOTHROW { return std::free(p); }
};

#define Q_XCB_REPLY(call, ...) \
    std::unique_ptr<call##_reply_t, QStdFreeDeleter>( \
        call##_reply(Q_XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call(__VA_ARGS__), nullptr) \
    )

#define Q_XCB_REPLY_UNCHECKED(call, ...) \
    std::unique_ptr<call##_reply_t, QStdFreeDeleter>( \
        call##_reply(Q_XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call##_unchecked(__VA_ARGS__), nullptr) \
    )

template <typename T>
union q_padded_xcb_event {
  T event;
  char padding[32];
};

// The xcb_send_event() requires all events to have 32 bytes. It calls memcpy() on the
// passed in event. If the passed in event is less than 32 bytes, memcpy() reaches into
// unrelated memory.
#define Q_DECLARE_XCB_EVENT(event_var, event_type) \
    q_padded_xcb_event<event_type> store = {}; \
    auto &event_var = store.event;

QT_END_NAMESPACE

#endif
