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

#include "qxcbwindow.h"

#include <QtDebug>
#include <QMetaEnum>
#include <QScreen>
#include <QtGui/QIcon>
#include <QtGui/QRegion>
#include <QtGui/private/qhighdpiscaling_p.h>

#include "qxcbintegration.h"
#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbdrag.h"
#include "qxcbkeyboard.h"
#include "qxcbwmsupport.h"
#include "qxcbimage.h"
#include "qxcbnativeinterface.h"
#include "qxcbsystemtraytracker.h"

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformcursor.h>

#include <algorithm>

// FIXME This workaround can be removed for xcb-icccm > 3.8
#define class class_name
#include <xcb/xcb_icccm.h>
#undef class
#include <xcb/xfixes.h>
#include <xcb/shape.h>

// xcb-icccm 3.8 support
#ifdef XCB_ICCCM_NUM_WM_SIZE_HINTS_ELEMENTS
#define xcb_get_wm_hints_reply xcb_icccm_get_wm_hints_reply
#define xcb_get_wm_hints xcb_icccm_get_wm_hints
#define xcb_get_wm_hints_unchecked xcb_icccm_get_wm_hints_unchecked
#define xcb_set_wm_hints xcb_icccm_set_wm_hints
#define xcb_set_wm_normal_hints xcb_icccm_set_wm_normal_hints
#define xcb_size_hints_set_base_size xcb_icccm_size_hints_set_base_size
#define xcb_size_hints_set_max_size xcb_icccm_size_hints_set_max_size
#define xcb_size_hints_set_min_size xcb_icccm_size_hints_set_min_size
#define xcb_size_hints_set_position xcb_icccm_size_hints_set_position
#define xcb_size_hints_set_resize_inc xcb_icccm_size_hints_set_resize_inc
#define xcb_size_hints_set_size xcb_icccm_size_hints_set_size
#define xcb_size_hints_set_win_gravity xcb_icccm_size_hints_set_win_gravity
#define xcb_wm_hints_set_iconic xcb_icccm_wm_hints_set_iconic
#define xcb_wm_hints_set_normal xcb_icccm_wm_hints_set_normal
#define xcb_wm_hints_set_input xcb_icccm_wm_hints_set_input
#define xcb_wm_hints_t xcb_icccm_wm_hints_t
#define XCB_WM_STATE_ICONIC XCB_ICCCM_WM_STATE_ICONIC
#define XCB_WM_STATE_WITHDRAWN XCB_ICCCM_WM_STATE_WITHDRAWN
#endif

#include <private/qguiapplication_p.h>
#include <private/qwindow_p.h>

#include <qpa/qplatformbackingstore.h>
#include <qpa/qwindowsysteminterface.h>

#include <QTextCodec>
#include <stdio.h>

#ifdef XCB_USE_XLIB
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#if defined(XCB_USE_XINPUT2)
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2proto.h>
#endif

#define XCOORD_MAX 16383
enum {
    defaultWindowWidth = 160,
    defaultWindowHeight = 160
};

//#ifdef NET_WM_STATE_DEBUG

QT_BEGIN_NAMESPACE

Q_DECLARE_TYPEINFO(xcb_rectangle_t, Q_PRIMITIVE_TYPE);

#undef FocusIn

enum QX11EmbedFocusInDetail {
    XEMBED_FOCUS_CURRENT = 0,
    XEMBED_FOCUS_FIRST = 1,
    XEMBED_FOCUS_LAST = 2
};

enum QX11EmbedInfoFlags {
    XEMBED_MAPPED = (1 << 0),
};

enum QX11EmbedMessageType {
    XEMBED_EMBEDDED_NOTIFY = 0,
    XEMBED_WINDOW_ACTIVATE = 1,
    XEMBED_WINDOW_DEACTIVATE = 2,
    XEMBED_REQUEST_FOCUS = 3,
    XEMBED_FOCUS_IN = 4,
    XEMBED_FOCUS_OUT = 5,
    XEMBED_FOCUS_NEXT = 6,
    XEMBED_FOCUS_PREV = 7,
    XEMBED_MODALITY_ON = 10,
    XEMBED_MODALITY_OFF = 11,
    XEMBED_REGISTER_ACCELERATOR = 12,
    XEMBED_UNREGISTER_ACCELERATOR = 13,
    XEMBED_ACTIVATE_ACCELERATOR = 14
};

const quint32 XEMBED_VERSION = 0;

QXcbScreen *QXcbWindow::parentScreen()
{
    return parent() ? static_cast<QXcbWindow*>(parent())->parentScreen() : xcbScreen();
}

// Returns \c true if we should set WM_TRANSIENT_FOR on \a w
static inline bool isTransient(const QWindow *w)
{
    return w->type() == Qt::Dialog
           || w->type() == Qt::Sheet
           || w->type() == Qt::Tool
           || w->type() == Qt::SplashScreen
           || w->type() == Qt::ToolTip
           || w->type() == Qt::Drawer
           || w->type() == Qt::Popup;
}

static inline QImage::Format imageFormatForVisual(int depth, quint32 red_mask, quint32 blue_mask, bool *rgbSwap)
{
    if (rgbSwap)
        *rgbSwap = false;
    switch (depth) {
    case 32:
        if (blue_mask == 0xff)
            return QImage::Format_ARGB32_Premultiplied;
        if (red_mask == 0x3ff)
            return QImage::Format_A2BGR30_Premultiplied;
        if (blue_mask == 0x3ff)
            return QImage::Format_A2RGB30_Premultiplied;
        if (red_mask == 0xff) {
            if (rgbSwap)
                *rgbSwap = true;
            return QImage::Format_ARGB32_Premultiplied;
        }
        break;
    case 30:
        if (red_mask == 0x3ff)
            return QImage::Format_BGR30;
        if (blue_mask == 0x3ff)
            return QImage::Format_RGB30;
        break;
    case 24:
        if (blue_mask == 0xff)
            return QImage::Format_RGB32;
        if (red_mask == 0xff) {
            if (rgbSwap)
                *rgbSwap = true;
            return QImage::Format_RGB32;
        }
        break;
    case 16:
        if (blue_mask == 0x1f)
            return QImage::Format_RGB16;
        if (red_mask == 0x1f) {
            if (rgbSwap)
                *rgbSwap = true;
            return QImage::Format_RGB16;
        }
        break;
    case 15:
        if (blue_mask == 0x1f)
            return QImage::Format_RGB555;
        if (red_mask == 0x1f) {
            if (rgbSwap)
                *rgbSwap = true;
            return QImage::Format_RGB555;
        }
        break;
    default:
        break;
    }
    qWarning("Unsupported screen format: depth: %d, red_mask: %x, blue_mask: %x", depth, red_mask, blue_mask);

    switch (depth) {
    case 24:
        qWarning("Using RGB32 fallback, if this works your X11 server is reporting a bad screen format.");
        return QImage::Format_RGB32;
    case 16:
        qWarning("Using RGB16 fallback, if this works your X11 server is reporting a bad screen format.");
        return QImage::Format_RGB16;
    default:
        break;
    }

    return QImage::Format_Invalid;
}

static inline bool positionIncludesFrame(QWindow *w)
{
    return qt_window_private(w)->positionPolicy == QWindowPrivate::WindowFrameInclusive;
}

#ifdef XCB_USE_XLIB
static inline XTextProperty* qstringToXTP(Display *dpy, const QString& s)
{
    #include <X11/Xatom.h>

    static XTextProperty tp = { 0, 0, 0, 0 };
    static bool free_prop = true; // we can't free tp.value in case it references
                                  // the data of the static QByteArray below.
    if (tp.value) {
        if (free_prop)
            XFree(tp.value);
        tp.value = 0;
        free_prop = true;
    }

#if QT_CONFIG(textcodec)
    static const QTextCodec* mapper = QTextCodec::codecForLocale();
    int errCode = 0;
    if (mapper) {
        QByteArray mapped = mapper->fromUnicode(s);
        char* tl[2];
        tl[0] = mapped.data();
        tl[1] = 0;
        errCode = XmbTextListToTextProperty(dpy, tl, 1, XStdICCTextStyle, &tp);
        if (errCode < 0)
            qDebug("XmbTextListToTextProperty result code %d", errCode);
    }
    if (!mapper || errCode < 0) {
        mapper = QTextCodec::codecForName("latin1");
        if (!mapper || !mapper->canEncode(s))
            return Q_NULLPTR;
#endif
        static QByteArray qcs;
        qcs = s.toLatin1();
        tp.value = (uchar*)qcs.data();
        tp.encoding = XA_STRING;
        tp.format = 8;
        tp.nitems = qcs.length();
        free_prop = false;
#if QT_CONFIG(textcodec)
    }
#endif
    return &tp;
}
#endif // XCB_USE_XLIB

// TODO move this into a utility function in QWindow or QGuiApplication
static QWindow *childWindowAt(QWindow *win, const QPoint &p)
{
    for (QObject *obj : win->children()) {
        if (obj->isWindowType()) {
            QWindow *childWin = static_cast<QWindow *>(obj);
            if (childWin->isVisible()) {
                if (QWindow *recurse = childWindowAt(childWin, p))
                    return recurse;
            }
        }
    }
    if (!win->isTopLevel()
            && !(win->flags() & Qt::WindowTransparentForInput)
            && win->geometry().contains(win->parent()->mapFromGlobal(p))) {
        return win;
    }
    return Q_NULLPTR;
}

static const char *wm_window_type_property_id = "_q_xcb_wm_window_type";
static const char *wm_window_role_property_id = "_q_xcb_wm_window_role";

QXcbWindow::QXcbWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_window(0)
    , m_syncCounter(0)
    , m_gravity(XCB_GRAVITY_STATIC)
    , m_mapped(false)
    , m_transparent(false)
    , m_usingSyncProtocol(false)
    , m_deferredActivation(false)
    , m_embedded(false)
    , m_alertState(false)
    , m_netWmUserTimeWindow(XCB_NONE)
    , m_dirtyFrameMargins(false)
    , m_lastWindowStateEvent(-1)
    , m_syncState(NoSyncNeeded)
    , m_pendingSyncRequest(0)
    , m_currentBitmapCursor(XCB_CURSOR_NONE)
{
    setConnection(xcbScreen()->connection());
}

#ifdef Q_COMPILER_CLASS_ENUM
enum : quint32 {
#else
enum {
#endif
    baseEventMask
        = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY
            | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE,

    defaultEventMask = baseEventMask
            | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
            | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
            | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW
            | XCB_EVENT_MASK_POINTER_MOTION,

    transparentForInputEventMask = baseEventMask
            | XCB_EVENT_MASK_VISIBILITY_CHANGE | XCB_EVENT_MASK_RESIZE_REDIRECT
            | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
            | XCB_EVENT_MASK_COLOR_MAP_CHANGE | XCB_EVENT_MASK_OWNER_GRAB_BUTTON
};

void QXcbWindow::create()
{
    if (window()->type() == Qt::ForeignWindow) {
        m_window = window()->winId();
        return;
    }

    destroy();

    m_windowState = Qt::WindowNoState;

    Qt::WindowType type = window()->type();

    QXcbScreen *currentScreen = xcbScreen();
    QRect rect = windowGeometry();
    QXcbScreen *platformScreen = parent() ? parentScreen() : static_cast<QXcbScreen*>(screenForGeometry(rect));

    if (type == Qt::Desktop) {
        m_window = platformScreen->root();
        m_depth = platformScreen->screen()->root_depth;
        m_visualId = platformScreen->screen()->root_visual;
        const xcb_visualtype_t *visual = 0;
        if (connection()->hasDefaultVisualId()) {
            visual = platformScreen->visualForId(connection()->defaultVisualId());
            if (visual)
                m_visualId = connection()->defaultVisualId();
            if (!visual)
                qWarning("Could not use default visual id. Falling back to root_visual for screen.");
        }
        if (!visual)
            visual = platformScreen->visualForId(m_visualId);
        m_imageFormat = imageFormatForVisual(m_depth, visual->red_mask, visual->blue_mask, &m_imageRgbSwap);
        connection()->addWindowEventListener(m_window, this);
        return;
    }

    // Parameters to XCreateWindow() are frame corner + inner size.
    // This fits in case position policy is frame inclusive. There is
    // currently no way to implement it for frame-exclusive geometries.
    QPlatformWindow::setGeometry(rect);

    if (platformScreen != currentScreen)
        QWindowSystemInterface::handleWindowScreenChanged(window(), platformScreen->QPlatformScreen::screen());

    const QSize minimumSize = windowMinimumSize();
    if (rect.width() > 0 || rect.height() > 0) {
        rect.setWidth(qBound(1, rect.width(), XCOORD_MAX));
        rect.setHeight(qBound(1, rect.height(), XCOORD_MAX));
    } else if (minimumSize.width() > 0 || minimumSize.height() > 0) {
        rect.setSize(minimumSize);
    } else {
        rect.setWidth(QHighDpi::toNativePixels(int(defaultWindowWidth), platformScreen->QPlatformScreen::screen()));
        rect.setHeight(QHighDpi::toNativePixels(int(defaultWindowHeight), platformScreen->QPlatformScreen::screen()));
    }

    xcb_window_t xcb_parent_id = platformScreen->root();
    if (parent()) {
        xcb_parent_id = static_cast<QXcbWindow *>(parent())->xcb_window();
        m_embedded = parent()->window()->type() == Qt::ForeignWindow;

        QSurfaceFormat parentFormat = parent()->window()->requestedFormat();
        if (window()->surfaceType() != QSurface::OpenGLSurface && parentFormat.hasAlpha()) {
            window()->setFormat(parentFormat);
        }
    }

    resolveFormat(platformScreen->surfaceFormatFor(window()->requestedFormat()));

    const xcb_visualtype_t *visual = Q_NULLPTR;

    if (connection()->hasDefaultVisualId()) {
        visual = platformScreen->visualForId(connection()->defaultVisualId());
        if (!visual)
            qWarning() << "Failed to use requested visual id.";
    }

    if (!visual)
        visual = createVisual();

    if (!visual) {
        qWarning() << "Falling back to using screens root_visual.";
        visual = platformScreen->visualForId(platformScreen->screen()->root_visual);
    }

    Q_ASSERT(visual);

    m_visualId = visual->visual_id;
    m_depth = platformScreen->depthOfVisual(m_visualId);
    m_imageFormat = imageFormatForVisual(m_depth, visual->red_mask, visual->blue_mask, &m_imageRgbSwap);
    xcb_colormap_t colormap = 0;

    quint32 mask = XCB_CW_BACK_PIXMAP
                 | XCB_CW_BORDER_PIXEL
                 | XCB_CW_BIT_GRAVITY
                 | XCB_CW_OVERRIDE_REDIRECT
                 | XCB_CW_SAVE_UNDER
                 | XCB_CW_EVENT_MASK;

    static const bool haveOpenGL = QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL);

    if ((window()->supportsOpenGL() && haveOpenGL) || m_format.hasAlpha()) {
        colormap = xcb_generate_id(xcb_connection());
        Q_XCB_CALL(xcb_create_colormap(xcb_connection(),
                                       XCB_COLORMAP_ALLOC_NONE,
                                       colormap,
                                       xcb_parent_id,
                                       m_visualId));

        mask |= XCB_CW_COLORMAP;
    }

    quint32 values[] = {
        XCB_BACK_PIXMAP_NONE,
        platformScreen->screen()->black_pixel,
        XCB_GRAVITY_NORTH_WEST,
        type == Qt::Popup || type == Qt::ToolTip || (window()->flags() & Qt::BypassWindowManagerHint),
        type == Qt::Popup || type == Qt::Tool || type == Qt::SplashScreen || type == Qt::ToolTip || type == Qt::Drawer,
        defaultEventMask,
        colormap
    };

    m_window = xcb_generate_id(xcb_connection());
    Q_XCB_CALL(xcb_create_window(xcb_connection(),
                                 m_depth,
                                 m_window,                        // window id
                                 xcb_parent_id,                   // parent window id
                                 rect.x(),
                                 rect.y(),
                                 rect.width(),
                                 rect.height(),
                                 0,                               // border width
                                 XCB_WINDOW_CLASS_INPUT_OUTPUT,   // window class
                                 m_visualId,                      // visual
                                 mask,
                                 values));

    connection()->addWindowEventListener(m_window, this);

    Q_XCB_CALL(xcb_change_window_attributes(xcb_connection(), m_window, mask, values));

    propagateSizeHints();

    xcb_atom_t properties[5];
    int propertyCount = 0;
    properties[propertyCount++] = atom(QXcbAtom::WM_DELETE_WINDOW);
    properties[propertyCount++] = atom(QXcbAtom::WM_TAKE_FOCUS);
    properties[propertyCount++] = atom(QXcbAtom::_NET_WM_PING);

    m_usingSyncProtocol = platformScreen->syncRequestSupported();

    if (m_usingSyncProtocol)
        properties[propertyCount++] = atom(QXcbAtom::_NET_WM_SYNC_REQUEST);

    if (window()->flags() & Qt::WindowContextHelpButtonHint)
        properties[propertyCount++] = atom(QXcbAtom::_NET_WM_CONTEXT_HELP);

    Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                   XCB_PROP_MODE_REPLACE,
                                   m_window,
                                   atom(QXcbAtom::WM_PROTOCOLS),
                                   XCB_ATOM_ATOM,
                                   32,
                                   propertyCount,
                                   properties));
    m_syncValue.hi = 0;
    m_syncValue.lo = 0;

    const QByteArray wmClass = QXcbIntegration::instance()->wmClass();
    if (!wmClass.isEmpty()) {
        Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE,
                                       m_window, atom(QXcbAtom::WM_CLASS),
                                       XCB_ATOM_STRING, 8, wmClass.size(), wmClass.constData()));
    }

    if (m_usingSyncProtocol) {
        m_syncCounter = xcb_generate_id(xcb_connection());
        Q_XCB_CALL(xcb_sync_create_counter(xcb_connection(), m_syncCounter, m_syncValue));

        Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                       XCB_PROP_MODE_REPLACE,
                                       m_window,
                                       atom(QXcbAtom::_NET_WM_SYNC_REQUEST_COUNTER),
                                       XCB_ATOM_CARDINAL,
                                       32,
                                       1,
                                       &m_syncCounter));
    }

    // set the PID to let the WM kill the application if unresponsive
    quint32 pid = getpid();
    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::_NET_WM_PID), XCB_ATOM_CARDINAL, 32,
                                   1, &pid));

    xcb_wm_hints_t hints;
    memset(&hints, 0, sizeof(hints));
    xcb_wm_hints_set_normal(&hints);

    xcb_wm_hints_set_input(&hints, !(window()->flags() & Qt::WindowDoesNotAcceptFocus));

    xcb_set_wm_hints(xcb_connection(), m_window, &hints);

    xcb_window_t leader = connection()->clientLeader();
    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::WM_CLIENT_LEADER), XCB_ATOM_WINDOW, 32,
                                   1, &leader));

    /* Add XEMBED info; this operation doesn't initiate the embedding. */
    quint32 data[] = { XEMBED_VERSION, XEMBED_MAPPED };
    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::_XEMBED_INFO),
                                   atom(QXcbAtom::_XEMBED_INFO),
                                   32, 2, (void *)data));


#if defined(XCB_USE_XINPUT2)
    connection()->xi2Select(m_window);
#endif

    setWindowState(window()->windowState());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());

    if (window()->flags() & Qt::WindowTransparentForInput)
        setTransparentForMouseEvents(true);

#ifdef XCB_USE_XLIB
    // force sync to read outstanding requests - see QTBUG-29106
    XSync(DISPLAY_FROM_XCB(platformScreen), false);
#endif

#ifndef QT_NO_DRAGANDDROP
    connection()->drag()->dndEnable(this, true);
#endif

    const qreal opacity = qt_window_private(window())->opacity;
    if (!qFuzzyCompare(opacity, qreal(1.0)))
        setOpacity(opacity);
    if (window()->isTopLevel())
        setWindowIcon(window()->icon());

    if (window()->dynamicPropertyNames().contains(wm_window_role_property_id)) {
        QByteArray wmWindowRole = window()->property(wm_window_role_property_id).toByteArray();
        setWmWindowRole(wmWindowRole);
    }
}

QXcbWindow::~QXcbWindow()
{
    if (m_currentBitmapCursor != XCB_CURSOR_NONE) {
        xcb_free_cursor(xcb_connection(), m_currentBitmapCursor);
    }
    if (window()->type() != Qt::ForeignWindow)
        destroy();
    else {
        if (connection()->mouseGrabber() == this)
            connection()->setMouseGrabber(Q_NULLPTR);
        if (connection()->mousePressWindow() == this)
            connection()->setMousePressWindow(Q_NULLPTR);
    }
}

void QXcbWindow::destroy()
{
    if (connection()->focusWindow() == this)
        doFocusOut();
    if (connection()->mouseGrabber() == this)
        connection()->setMouseGrabber(Q_NULLPTR);

    if (m_syncCounter && m_usingSyncProtocol)
        Q_XCB_CALL(xcb_sync_destroy_counter(xcb_connection(), m_syncCounter));
    if (m_window) {
        if (m_netWmUserTimeWindow) {
            xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::_NET_WM_USER_TIME_WINDOW));
            // Some window managers, like metacity, do XSelectInput on the _NET_WM_USER_TIME_WINDOW window,
            // without trapping BadWindow (which crashes when the user time window is destroyed).
            connection()->sync();
            xcb_destroy_window(xcb_connection(), m_netWmUserTimeWindow);
            m_netWmUserTimeWindow = XCB_NONE;
        }
        connection()->removeWindowEventListener(m_window);
        Q_XCB_CALL(xcb_destroy_window(xcb_connection(), m_window));
        m_window = 0;
    }
    m_mapped = false;

    if (m_pendingSyncRequest)
        m_pendingSyncRequest->invalidate();
}

void QXcbWindow::setGeometry(const QRect &rect)
{
    QPlatformWindow::setGeometry(rect);

    propagateSizeHints();

    QXcbScreen *currentScreen = xcbScreen();
    QXcbScreen *newScreen = parent() ? parentScreen() : static_cast<QXcbScreen*>(screenForGeometry(rect));

    if (!newScreen)
        newScreen = xcbScreen();

    const QRect wmGeometry = windowToWmGeometry(rect);

    if (newScreen != currentScreen)
        QWindowSystemInterface::handleWindowScreenChanged(window(), newScreen->QPlatformScreen::screen());

    if (qt_window_private(window())->positionAutomatic) {
        const quint32 mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        const qint32 values[] = {
            qBound<qint32>(1,           wmGeometry.width(),  XCOORD_MAX),
            qBound<qint32>(1,           wmGeometry.height(), XCOORD_MAX),
        };
        Q_XCB_CALL(xcb_configure_window(xcb_connection(), m_window, mask, reinterpret_cast<const quint32*>(values)));
    } else {
        const quint32 mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        const qint32 values[] = {
            qBound<qint32>(-XCOORD_MAX, wmGeometry.x(),      XCOORD_MAX),
            qBound<qint32>(-XCOORD_MAX, wmGeometry.y(),      XCOORD_MAX),
            qBound<qint32>(1,           wmGeometry.width(),  XCOORD_MAX),
            qBound<qint32>(1,           wmGeometry.height(), XCOORD_MAX),
        };
        Q_XCB_CALL(xcb_configure_window(xcb_connection(), m_window, mask, reinterpret_cast<const quint32*>(values)));
    }

    xcb_flush(xcb_connection());
}

QMargins QXcbWindow::frameMargins() const
{
    if (m_dirtyFrameMargins) {
        if (connection()->wmSupport()->isSupportedByWM(atom(QXcbAtom::_NET_FRAME_EXTENTS))) {
            xcb_get_property_cookie_t cookie = xcb_get_property(xcb_connection(), false, m_window,
                                                                atom(QXcbAtom::_NET_FRAME_EXTENTS), XCB_ATOM_CARDINAL, 0, 4);
            QScopedPointer<xcb_get_property_reply_t, QScopedPointerPodDeleter> reply(
                xcb_get_property_reply(xcb_connection(), cookie, NULL));
            if (reply && reply->type == XCB_ATOM_CARDINAL && reply->format == 32 && reply->value_len == 4) {
                quint32 *data = (quint32 *)xcb_get_property_value(reply.data());
                // _NET_FRAME_EXTENTS format is left, right, top, bottom
                m_frameMargins = QMargins(data[0], data[2], data[1], data[3]);
                m_dirtyFrameMargins = false;
                return m_frameMargins;
            }
        }

        // _NET_FRAME_EXTENTS property is not available, so
        // walk up the window tree to get the frame parent
        xcb_window_t window = m_window;
        xcb_window_t parent = m_window;

        bool foundRoot = false;

        const QVector<xcb_window_t> &virtualRoots =
            connection()->wmSupport()->virtualRoots();

        while (!foundRoot) {
            xcb_query_tree_cookie_t cookie = xcb_query_tree_unchecked(xcb_connection(), parent);

            xcb_query_tree_reply_t *reply = xcb_query_tree_reply(xcb_connection(), cookie, NULL);
            if (reply) {
                if (reply->root == reply->parent || virtualRoots.indexOf(reply->parent) != -1 || reply->parent == XCB_WINDOW_NONE) {
                    foundRoot = true;
                } else {
                    window = parent;
                    parent = reply->parent;
                }

                free(reply);
            } else {
                m_dirtyFrameMargins = false;
                m_frameMargins = QMargins();
                return m_frameMargins;
            }
        }

        QPoint offset;

        xcb_translate_coordinates_reply_t *reply =
            xcb_translate_coordinates_reply(
                xcb_connection(),
                xcb_translate_coordinates(xcb_connection(), window, parent, 0, 0),
                NULL);

        if (reply) {
            offset = QPoint(reply->dst_x, reply->dst_y);
            free(reply);
        }

        xcb_get_geometry_reply_t *geom =
            xcb_get_geometry_reply(
                xcb_connection(),
                xcb_get_geometry(xcb_connection(), parent),
                NULL);

        if (geom) {
            // --
            // add the border_width for the window managers frame... some window managers
            // do not use a border_width of zero for their frames, and if we the left and
            // top strut, we ensure that pos() is absolutely correct.  frameGeometry()
            // will still be incorrect though... perhaps i should have foffset as well, to
            // indicate the frame offset (equal to the border_width on X).
            // - Brad
            // -- copied from qwidget_x11.cpp

            int left = offset.x() + geom->border_width;
            int top = offset.y() + geom->border_width;
            int right = geom->width + geom->border_width - geometry().width() - offset.x();
            int bottom = geom->height + geom->border_width - geometry().height() - offset.y();

            m_frameMargins = QMargins(left, top, right, bottom);

            free(geom);
        }

        m_dirtyFrameMargins = false;
    }

    return m_frameMargins;
}

void QXcbWindow::setVisible(bool visible)
{
    if (visible)
        show();
    else
        hide();
}

static inline bool testShowWithoutActivating(const QWindow *window)
{
    // QWidget-attribute Qt::WA_ShowWithoutActivating.
    const QVariant showWithoutActivating = window->property("_q_showWithoutActivating");
    return showWithoutActivating.isValid() && showWithoutActivating.toBool();
}

void QXcbWindow::show()
{
    if (window()->isTopLevel()) {
        xcb_get_property_cookie_t cookie = xcb_get_wm_hints_unchecked(xcb_connection(), m_window);

        xcb_wm_hints_t hints;
        xcb_get_wm_hints_reply(xcb_connection(), cookie, &hints, NULL);

        if (window()->windowState() & Qt::WindowMinimized)
            xcb_wm_hints_set_iconic(&hints);
        else
            xcb_wm_hints_set_normal(&hints);

        xcb_wm_hints_set_input(&hints, !(window()->flags() & Qt::WindowDoesNotAcceptFocus));

        xcb_set_wm_hints(xcb_connection(), m_window, &hints);

        m_gravity = positionIncludesFrame(window()) ?
                    XCB_GRAVITY_NORTH_WEST : XCB_GRAVITY_STATIC;

        // update WM_NORMAL_HINTS
        propagateSizeHints();

        // update WM_TRANSIENT_FOR
        xcb_window_t transientXcbParent = 0;
        if (isTransient(window())) {
            const QWindow *tp = window()->transientParent();
            if (tp && tp->handle())
                transientXcbParent = static_cast<const QXcbWindow *>(tp->handle())->winId();
            // Default to client leader if there is no transient parent, else modal dialogs can
            // be hidden by their parents.
            if (!transientXcbParent)
                transientXcbParent = connection()->clientLeader();
            if (transientXcbParent) { // ICCCM 4.1.2.6
                Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                               XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 32,
                                               1, &transientXcbParent));
            }
        }
        if (!transientXcbParent)
            Q_XCB_CALL(xcb_delete_property(xcb_connection(), m_window, XCB_ATOM_WM_TRANSIENT_FOR));

        // update _MOTIF_WM_HINTS
        updateMotifWmHintsBeforeMap();

        // update _NET_WM_STATE
        updateNetWmStateBeforeMap();
    }

    if (testShowWithoutActivating(window()))
        updateNetWmUserTime(0);
    else if (connection()->time() != XCB_TIME_CURRENT_TIME)
        updateNetWmUserTime(connection()->time());

    if (window()->objectName() == QLatin1String("QSystemTrayIconSysWindow"))
        return; // defer showing until XEMBED_EMBEDDED_NOTIFY

    Q_XCB_CALL(xcb_map_window(xcb_connection(), m_window));

    if (QGuiApplication::modalWindow() == window())
        requestActivateWindow();

    xcbScreen()->windowShown(this);

    connection()->sync();
}

void QXcbWindow::hide()
{
    Q_XCB_CALL(xcb_unmap_window(xcb_connection(), m_window));

    // send synthetic UnmapNotify event according to icccm 4.1.4
    xcb_unmap_notify_event_t event;
    event.response_type = XCB_UNMAP_NOTIFY;
    event.event = xcbScreen()->root();
    event.window = m_window;
    event.from_configure = false;
    Q_XCB_CALL(xcb_send_event(xcb_connection(), false, xcbScreen()->root(),
                              XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));

    xcb_flush(xcb_connection());

    if (connection()->mouseGrabber() == this)
        connection()->setMouseGrabber(Q_NULLPTR);
    if (QPlatformWindow *w = connection()->mousePressWindow()) {
        // Unset mousePressWindow when it (or one of its parents) is unmapped
        while (w) {
            if (w == this) {
                connection()->setMousePressWindow(Q_NULLPTR);
                break;
            }
            w = w->parent();
        }
    }

    m_mapped = false;

    // Hiding a modal window doesn't send an enter event to its transient parent when the
    // mouse is already over the parent window, so the enter event must be emulated.
    if (window()->isModal()) {
        // Get the cursor position at modal window screen
        const QPoint nativePos = xcbScreen()->cursor()->pos();
        const QPoint cursorPos = QHighDpi::fromNativePixels(nativePos, xcbScreen()->screenForPosition(nativePos)->screen());

        // Find the top level window at cursor position.
        // Don't use QGuiApplication::topLevelAt(): search only the virtual siblings of this window's screen
        QWindow *enterWindow = Q_NULLPTR;
        const auto screens = xcbScreen()->virtualSiblings();
        for (QPlatformScreen *screen : screens) {
            if (screen->geometry().contains(cursorPos)) {
                const QPoint devicePosition = QHighDpi::toNativePixels(cursorPos, screen->screen());
                enterWindow = screen->topLevelAt(devicePosition);
                break;
            }
        }

        if (enterWindow && enterWindow != window()) {
            // Find the child window at cursor position, otherwise use the top level window
            if (QWindow *childWindow = childWindowAt(enterWindow, cursorPos))
                enterWindow = childWindow;
            const QPoint localPos = enterWindow->mapFromGlobal(cursorPos);
            QWindowSystemInterface::handleEnterEvent(enterWindow, localPos, cursorPos);
        }
    }
}

static QWindow *tlWindow(QWindow *window)
{
    if (window && window->parent())
        return tlWindow(window->parent());
    return window;
}

bool QXcbWindow::relayFocusToModalWindow() const
{
    QWindow *w = tlWindow(static_cast<QWindowPrivate *>(QObjectPrivate::get(window()))->eventReceiver());
    QWindow *modal_window = 0;
    if (QGuiApplicationPrivate::instance()->isWindowBlocked(w,&modal_window) && modal_window != w) {
        modal_window->requestActivate();
        connection()->flush();
        return true;
    }

    return false;
}

void QXcbWindow::doFocusIn()
{
    if (relayFocusToModalWindow())
        return;
    QWindow *w = static_cast<QWindowPrivate *>(QObjectPrivate::get(window()))->eventReceiver();
    connection()->setFocusWindow(static_cast<QXcbWindow *>(w->handle()));
    QWindowSystemInterface::handleWindowActivated(w, Qt::ActiveWindowFocusReason);
}

static bool focusInPeeker(QXcbConnection *connection, xcb_generic_event_t *event)
{
    if (!event) {
        // FocusIn event is not in the queue, proceed with FocusOut normally.
        QWindowSystemInterface::handleWindowActivated(0, Qt::ActiveWindowFocusReason);
        return true;
    }
    uint response_type = event->response_type & ~0x80;
    if (response_type == XCB_FOCUS_IN) {
        // Ignore focus events that are being sent only because the pointer is over
        // our window, even if the input focus is in a different window.
        xcb_focus_in_event_t *e = (xcb_focus_in_event_t *) event;
        if (e->detail != XCB_NOTIFY_DETAIL_POINTER)
            return true;
    }

    /* We are also interested in XEMBED_FOCUS_IN events */
    if (response_type == XCB_CLIENT_MESSAGE) {
        xcb_client_message_event_t *cme = (xcb_client_message_event_t *)event;
        if (cme->type == connection->atom(QXcbAtom::_XEMBED)
            && cme->data.data32[1] == XEMBED_FOCUS_IN)
            return true;
    }

    return false;
}

void QXcbWindow::doFocusOut()
{
    if (relayFocusToModalWindow())
        return;
    connection()->setFocusWindow(0);
    // Do not set the active window to 0 if there is a FocusIn coming.
    // There is however no equivalent for XPutBackEvent so register a
    // callback for QXcbConnection instead.
    connection()->addPeekFunc(focusInPeeker);
}

struct QtMotifWmHints {
    quint32 flags, functions, decorations;
    qint32 input_mode;
    quint32 status;
};

enum {
    MWM_HINTS_FUNCTIONS   = (1L << 0),

    MWM_FUNC_ALL      = (1L << 0),
    MWM_FUNC_RESIZE   = (1L << 1),
    MWM_FUNC_MOVE     = (1L << 2),
    MWM_FUNC_MINIMIZE = (1L << 3),
    MWM_FUNC_MAXIMIZE = (1L << 4),
    MWM_FUNC_CLOSE    = (1L << 5),

    MWM_HINTS_DECORATIONS = (1L << 1),

    MWM_DECOR_ALL      = (1L << 0),
    MWM_DECOR_BORDER   = (1L << 1),
    MWM_DECOR_RESIZEH  = (1L << 2),
    MWM_DECOR_TITLE    = (1L << 3),
    MWM_DECOR_MENU     = (1L << 4),
    MWM_DECOR_MINIMIZE = (1L << 5),
    MWM_DECOR_MAXIMIZE = (1L << 6),

    MWM_HINTS_INPUT_MODE = (1L << 2),

    MWM_INPUT_MODELESS                  = 0L,
    MWM_INPUT_PRIMARY_APPLICATION_MODAL = 1L,
    MWM_INPUT_FULL_APPLICATION_MODAL    = 3L
};

static QtMotifWmHints getMotifWmHints(QXcbConnection *c, xcb_window_t window)
{
    QtMotifWmHints hints;

    xcb_get_property_cookie_t get_cookie =
        xcb_get_property_unchecked(c->xcb_connection(), 0, window, c->atom(QXcbAtom::_MOTIF_WM_HINTS),
                         c->atom(QXcbAtom::_MOTIF_WM_HINTS), 0, 20);

    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(c->xcb_connection(), get_cookie, NULL);

    if (reply && reply->format == 32 && reply->type == c->atom(QXcbAtom::_MOTIF_WM_HINTS)) {
        hints = *((QtMotifWmHints *)xcb_get_property_value(reply));
    } else {
        hints.flags = 0L;
        hints.functions = MWM_FUNC_ALL;
        hints.decorations = MWM_DECOR_ALL;
        hints.input_mode = 0L;
        hints.status = 0L;
    }

    free(reply);

    return hints;
}

static void setMotifWmHints(QXcbConnection *c, xcb_window_t window, const QtMotifWmHints &hints)
{
    if (hints.flags != 0l) {
        Q_XCB_CALL2(xcb_change_property(c->xcb_connection(),
                                       XCB_PROP_MODE_REPLACE,
                                       window,
                                       c->atom(QXcbAtom::_MOTIF_WM_HINTS),
                                       c->atom(QXcbAtom::_MOTIF_WM_HINTS),
                                       32,
                                       5,
                                       &hints), c);
    } else {
        Q_XCB_CALL2(xcb_delete_property(c->xcb_connection(), window, c->atom(QXcbAtom::_MOTIF_WM_HINTS)), c);
    }
}

QXcbWindow::NetWmStates QXcbWindow::netWmStates()
{
    NetWmStates result(0);

    xcb_get_property_cookie_t get_cookie =
        xcb_get_property_unchecked(xcb_connection(), 0, m_window, atom(QXcbAtom::_NET_WM_STATE),
                         XCB_ATOM_ATOM, 0, 1024);

    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(xcb_connection(), get_cookie, NULL);

    if (reply && reply->format == 32 && reply->type == XCB_ATOM_ATOM) {
        const xcb_atom_t *states = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply));
        const xcb_atom_t *statesEnd = states + reply->length;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_ABOVE)))
            result |= NetWmStateAbove;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_BELOW)))
            result |= NetWmStateBelow;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_FULLSCREEN)))
            result |= NetWmStateFullScreen;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_HORZ)))
            result |= NetWmStateMaximizedHorz;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_VERT)))
            result |= NetWmStateMaximizedVert;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_MODAL)))
            result |= NetWmStateModal;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_STAYS_ON_TOP)))
            result |= NetWmStateStaysOnTop;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::_NET_WM_STATE_DEMANDS_ATTENTION)))
            result |= NetWmStateDemandsAttention;
        free(reply);
    } else {
#ifdef NET_WM_STATE_DEBUG
        printf("getting net wm state (%x), empty\n", m_window);
#endif
    }

    return result;
}

void QXcbWindow::setNetWmStates(NetWmStates states)
{
    QVector<xcb_atom_t> atoms;

    xcb_get_property_cookie_t get_cookie =
        xcb_get_property_unchecked(xcb_connection(), 0, m_window, atom(QXcbAtom::_NET_WM_STATE),
                         XCB_ATOM_ATOM, 0, 1024);

    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(xcb_connection(), get_cookie, NULL);

    if (reply && reply->format == 32 && reply->type == XCB_ATOM_ATOM && reply->value_len > 0) {
        const xcb_atom_t *data = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply));
        atoms.resize(reply->value_len);
        memcpy((void *)&atoms.first(), (void *)data, reply->value_len * sizeof(xcb_atom_t));
    }

    free(reply);

    if (states & NetWmStateAbove && !atoms.contains(atom(QXcbAtom::_NET_WM_STATE_ABOVE)))
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_ABOVE));
    if (states & NetWmStateBelow && !atoms.contains(atom(QXcbAtom::_NET_WM_STATE_BELOW)))
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_BELOW));
    if (states & NetWmStateFullScreen && !atoms.contains(atom(QXcbAtom::_NET_WM_STATE_FULLSCREEN)))
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_FULLSCREEN));
    if (states & NetWmStateMaximizedHorz && !atoms.contains(atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_HORZ)))
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_HORZ));
    if (states & NetWmStateMaximizedVert && !atoms.contains(atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_VERT)))
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_VERT));
    if (states & NetWmStateModal && !atoms.contains(atom(QXcbAtom::_NET_WM_STATE_MODAL)))
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_MODAL));
    if (states & NetWmStateStaysOnTop && !atoms.contains(atom(QXcbAtom::_NET_WM_STATE_STAYS_ON_TOP)))
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_STAYS_ON_TOP));
    if (states & NetWmStateDemandsAttention && !atoms.contains(atom(QXcbAtom::_NET_WM_STATE_DEMANDS_ATTENTION)))
        atoms.push_back(atom(QXcbAtom::_NET_WM_STATE_DEMANDS_ATTENTION));

    if (atoms.isEmpty()) {
        Q_XCB_CALL(xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::_NET_WM_STATE)));
    } else {
        Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                       atom(QXcbAtom::_NET_WM_STATE), XCB_ATOM_ATOM, 32,
                                       atoms.count(), atoms.constData()));
    }
    xcb_flush(xcb_connection());
}

void QXcbWindow::setWindowFlags(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    if (type == Qt::ToolTip)
        flags |= Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint;
    if (type == Qt::Popup)
        flags |= Qt::X11BypassWindowManagerHint;

    const quint32 mask = XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    const quint32 values[] = {
         // XCB_CW_OVERRIDE_REDIRECT
         (flags & Qt::BypassWindowManagerHint) ? 1u : 0,
         // XCB_CW_EVENT_MASK
         (flags & Qt::WindowTransparentForInput) ? transparentForInputEventMask : defaultEventMask
     };

    xcb_change_window_attributes(xcb_connection(), xcb_window(), mask, values);

    QXcbWindowFunctions::WmWindowTypes wmWindowTypes = 0;
    if (window()->dynamicPropertyNames().contains(wm_window_type_property_id)) {
        wmWindowTypes = static_cast<QXcbWindowFunctions::WmWindowTypes>(
            window()->property(wm_window_type_property_id).value<int>());
    }

    setWmWindowType(wmWindowTypes, flags);
    setNetWmStateWindowFlags(flags);
    setMotifWindowFlags(flags);

    setTransparentForMouseEvents(flags & Qt::WindowTransparentForInput);
    updateDoesNotAcceptFocus(flags & Qt::WindowDoesNotAcceptFocus);
}

void QXcbWindow::setMotifWindowFlags(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    QtMotifWmHints mwmhints;
    mwmhints.flags = 0L;
    mwmhints.functions = 0L;
    mwmhints.decorations = 0;
    mwmhints.input_mode = 0L;
    mwmhints.status = 0L;

    if (type != Qt::SplashScreen) {
        mwmhints.flags |= MWM_HINTS_DECORATIONS;

        bool customize = flags & Qt::CustomizeWindowHint;
        if (type == Qt::Window && !customize) {
            const Qt::WindowFlags defaultFlags = Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint;
            if (!(flags & defaultFlags))
                flags |= defaultFlags;
        }
        if (!(flags & Qt::FramelessWindowHint) && !(customize && !(flags & Qt::WindowTitleHint))) {
            mwmhints.decorations |= MWM_DECOR_BORDER;
            mwmhints.decorations |= MWM_DECOR_RESIZEH;
            mwmhints.decorations |= MWM_DECOR_TITLE;

            if (flags & Qt::WindowSystemMenuHint)
                mwmhints.decorations |= MWM_DECOR_MENU;

            if (flags & Qt::WindowMinimizeButtonHint) {
                mwmhints.decorations |= MWM_DECOR_MINIMIZE;
                mwmhints.functions |= MWM_FUNC_MINIMIZE;
            }

            if (flags & Qt::WindowMaximizeButtonHint) {
                mwmhints.decorations |= MWM_DECOR_MAXIMIZE;
                mwmhints.functions |= MWM_FUNC_MAXIMIZE;
            }

            if (flags & Qt::WindowCloseButtonHint)
                mwmhints.functions |= MWM_FUNC_CLOSE;
        }
    } else {
        // if type == Qt::SplashScreen
        mwmhints.decorations = MWM_DECOR_ALL;
    }

    if (mwmhints.functions != 0) {
        mwmhints.flags |= MWM_HINTS_FUNCTIONS;
        mwmhints.functions |= MWM_FUNC_MOVE | MWM_FUNC_RESIZE;
    } else {
        mwmhints.functions = MWM_FUNC_ALL;
    }

    if (!(flags & Qt::FramelessWindowHint)
        && flags & Qt::CustomizeWindowHint
        && flags & Qt::WindowTitleHint
        && !(flags &
             (Qt::WindowMinimizeButtonHint
              | Qt::WindowMaximizeButtonHint
              | Qt::WindowCloseButtonHint)))
    {
        // a special case - only the titlebar without any button
        mwmhints.flags = MWM_HINTS_FUNCTIONS;
        mwmhints.functions = MWM_FUNC_MOVE | MWM_FUNC_RESIZE;
        mwmhints.decorations = 0;
    }

    setMotifWmHints(connection(), m_window, mwmhints);
}

void QXcbWindow::changeNetWmState(bool set, xcb_atom_t one, xcb_atom_t two)
{
    xcb_client_message_event_t event;

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.sequence = 0;
    event.window = m_window;
    event.type = atom(QXcbAtom::_NET_WM_STATE);
    event.data.data32[0] = set ? 1 : 0;
    event.data.data32[1] = one;
    event.data.data32[2] = two;
    event.data.data32[3] = 0;
    event.data.data32[4] = 0;

    Q_XCB_CALL(xcb_send_event(xcb_connection(), 0, xcbScreen()->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));
}

void QXcbWindow::setWindowState(Qt::WindowState state)
{
    if (state == m_windowState)
        return;

    // unset old state
    switch (m_windowState) {
    case Qt::WindowMinimized:
        Q_XCB_CALL(xcb_map_window(xcb_connection(), m_window));
        break;
    case Qt::WindowMaximized:
        changeNetWmState(false,
                         atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_HORZ),
                         atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_VERT));
        break;
    case Qt::WindowFullScreen:
        changeNetWmState(false, atom(QXcbAtom::_NET_WM_STATE_FULLSCREEN));
        break;
    default:
        break;
    }

    // set new state
    switch (state) {
    case Qt::WindowMinimized:
        {
            xcb_client_message_event_t event;

            event.response_type = XCB_CLIENT_MESSAGE;
            event.format = 32;
            event.sequence = 0;
            event.window = m_window;
            event.type = atom(QXcbAtom::WM_CHANGE_STATE);
            event.data.data32[0] = XCB_WM_STATE_ICONIC;
            event.data.data32[1] = 0;
            event.data.data32[2] = 0;
            event.data.data32[3] = 0;
            event.data.data32[4] = 0;

            Q_XCB_CALL(xcb_send_event(xcb_connection(), 0, xcbScreen()->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));
        }
        break;
    case Qt::WindowMaximized:
        changeNetWmState(true,
                         atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_HORZ),
                         atom(QXcbAtom::_NET_WM_STATE_MAXIMIZED_VERT));
        break;
    case Qt::WindowFullScreen:
        changeNetWmState(true, atom(QXcbAtom::_NET_WM_STATE_FULLSCREEN));
        break;
    case Qt::WindowNoState:
        break;
    default:
        break;
    }

    connection()->sync();

    m_windowState = state;
}

void QXcbWindow::updateMotifWmHintsBeforeMap()
{
    QtMotifWmHints mwmhints = getMotifWmHints(connection(), m_window);

    if (window()->modality() != Qt::NonModal) {
        switch (window()->modality()) {
        case Qt::WindowModal:
            mwmhints.input_mode = MWM_INPUT_PRIMARY_APPLICATION_MODAL;
            break;
        case Qt::ApplicationModal:
        default:
            mwmhints.input_mode = MWM_INPUT_FULL_APPLICATION_MODAL;
            break;
        }
        mwmhints.flags |= MWM_HINTS_INPUT_MODE;
    } else {
        mwmhints.input_mode = MWM_INPUT_MODELESS;
        mwmhints.flags &= ~MWM_HINTS_INPUT_MODE;
    }

    if (windowMinimumSize() == windowMaximumSize()) {
        // fixed size, remove the resize handle (since mwm/dtwm
        // isn't smart enough to do it itself)
        mwmhints.flags |= MWM_HINTS_FUNCTIONS;
        if (mwmhints.functions == MWM_FUNC_ALL) {
            mwmhints.functions = MWM_FUNC_MOVE;
        } else {
            mwmhints.functions &= ~MWM_FUNC_RESIZE;
        }

        if (mwmhints.decorations == MWM_DECOR_ALL) {
            mwmhints.flags |= MWM_HINTS_DECORATIONS;
            mwmhints.decorations = (MWM_DECOR_BORDER
                                    | MWM_DECOR_TITLE
                                    | MWM_DECOR_MENU);
        } else {
            mwmhints.decorations &= ~MWM_DECOR_RESIZEH;
        }
    }

    if (window()->flags() & Qt::WindowMinimizeButtonHint) {
        mwmhints.flags |= MWM_HINTS_DECORATIONS;
        mwmhints.decorations |= MWM_DECOR_MINIMIZE;
        mwmhints.functions |= MWM_FUNC_MINIMIZE;
    }
    if (window()->flags() & Qt::WindowMaximizeButtonHint) {
        mwmhints.flags |= MWM_HINTS_DECORATIONS;
        mwmhints.decorations |= MWM_DECOR_MAXIMIZE;
        mwmhints.functions |= MWM_FUNC_MAXIMIZE;
    }
    if (window()->flags() & Qt::WindowCloseButtonHint)
        mwmhints.functions |= MWM_FUNC_CLOSE;

    setMotifWmHints(connection(), m_window, mwmhints);
}

void QXcbWindow::updateNetWmStateBeforeMap()
{
    NetWmStates states(0);

    const Qt::WindowFlags flags = window()->flags();
    if (flags & Qt::WindowStaysOnTopHint) {
        states |= NetWmStateAbove;
        states |= NetWmStateStaysOnTop;
    } else if (flags & Qt::WindowStaysOnBottomHint) {
        states |= NetWmStateBelow;
    }

    if (window()->windowState() & Qt::WindowFullScreen)
        states |= NetWmStateFullScreen;

    if (window()->windowState() & Qt::WindowMaximized) {
        states |= NetWmStateMaximizedHorz;
        states |= NetWmStateMaximizedVert;
    }

    if (window()->modality() != Qt::NonModal)
        states |= NetWmStateModal;

    setNetWmStates(states);
}

void QXcbWindow::setNetWmStateWindowFlags(Qt::WindowFlags flags)
{
    changeNetWmState(flags & Qt::WindowStaysOnTopHint,
                     atom(QXcbAtom::_NET_WM_STATE_ABOVE),
                     atom(QXcbAtom::_NET_WM_STATE_STAYS_ON_TOP));
    changeNetWmState(flags & Qt::WindowStaysOnBottomHint,
                     atom(QXcbAtom::_NET_WM_STATE_BELOW));
}

void QXcbWindow::updateNetWmUserTime(xcb_timestamp_t timestamp)
{
    xcb_window_t wid = m_window;
    // If timestamp == 0, then it means that the window should not be
    // initially activated. Don't update global user time for this
    // special case.
    if (timestamp != 0)
        connection()->setNetWmUserTime(timestamp);

    const bool isSupportedByWM = connection()->wmSupport()->isSupportedByWM(atom(QXcbAtom::_NET_WM_USER_TIME_WINDOW));
    if (m_netWmUserTimeWindow || isSupportedByWM) {
        if (!m_netWmUserTimeWindow) {
            m_netWmUserTimeWindow = xcb_generate_id(xcb_connection());
            Q_XCB_CALL(xcb_create_window(xcb_connection(),
                                         XCB_COPY_FROM_PARENT,            // depth -- same as root
                                         m_netWmUserTimeWindow,                        // window id
                                         m_window,                   // parent window id
                                         -1, -1, 1, 1,
                                         0,                               // border width
                                         XCB_WINDOW_CLASS_INPUT_OUTPUT,   // window class
                                         m_visualId,                      // visual
                                         0,                               // value mask
                                         0));                             // value list
            wid = m_netWmUserTimeWindow;
            xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window, atom(QXcbAtom::_NET_WM_USER_TIME_WINDOW),
                                XCB_ATOM_WINDOW, 32, 1, &m_netWmUserTimeWindow);
            xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::_NET_WM_USER_TIME));
#ifndef QT_NO_DEBUG
            QByteArray ba("Qt NET_WM user time window");
            Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                           XCB_PROP_MODE_REPLACE,
                                           m_netWmUserTimeWindow,
                                           atom(QXcbAtom::_NET_WM_NAME),
                                           atom(QXcbAtom::UTF8_STRING),
                                           8,
                                           ba.length(),
                                           ba.constData()));
#endif
        } else if (!isSupportedByWM) {
            // WM no longer supports it, then we should remove the
            // _NET_WM_USER_TIME_WINDOW atom.
            xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::_NET_WM_USER_TIME_WINDOW));
            xcb_destroy_window(xcb_connection(), m_netWmUserTimeWindow);
            m_netWmUserTimeWindow = XCB_NONE;
        } else {
            wid = m_netWmUserTimeWindow;
        }
    }
    xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, wid, atom(QXcbAtom::_NET_WM_USER_TIME),
                        XCB_ATOM_CARDINAL, 32, 1, &timestamp);
}

void QXcbWindow::setTransparentForMouseEvents(bool transparent)
{
    if (!connection()->hasXFixes() || transparent == m_transparent)
        return;

    xcb_rectangle_t rectangle;

    xcb_rectangle_t *rect = 0;
    int nrect = 0;

    if (!transparent) {
        rectangle.x = 0;
        rectangle.y = 0;
        rectangle.width = geometry().width();
        rectangle.height = geometry().height();
        rect = &rectangle;
        nrect = 1;
    }

    xcb_xfixes_region_t region = xcb_generate_id(xcb_connection());
    xcb_xfixes_create_region(xcb_connection(), region, nrect, rect);
    xcb_xfixes_set_window_shape_region_checked(xcb_connection(), m_window, XCB_SHAPE_SK_INPUT, 0, 0, region);
    xcb_xfixes_destroy_region(xcb_connection(), region);

    m_transparent = transparent;
}

void QXcbWindow::updateDoesNotAcceptFocus(bool doesNotAcceptFocus)
{
    xcb_get_property_cookie_t cookie = xcb_get_wm_hints_unchecked(xcb_connection(), m_window);

    xcb_wm_hints_t hints;
    if (!xcb_get_wm_hints_reply(xcb_connection(), cookie, &hints, NULL)) {
        return;
    }

    xcb_wm_hints_set_input(&hints, !doesNotAcceptFocus);
    xcb_set_wm_hints(xcb_connection(), m_window, &hints);
}

WId QXcbWindow::winId() const
{
    return m_window;
}

void QXcbWindow::setParent(const QPlatformWindow *parent)
{
    QPoint topLeft = geometry().topLeft();

    xcb_window_t xcb_parent_id;
    if (parent) {
        const QXcbWindow *qXcbParent = static_cast<const QXcbWindow *>(parent);
        xcb_parent_id = qXcbParent->xcb_window();
        m_embedded = qXcbParent->window()->type() == Qt::ForeignWindow;
    } else {
        xcb_parent_id = xcbScreen()->root();
        m_embedded = false;
    }
    Q_XCB_CALL(xcb_reparent_window(xcb_connection(), xcb_window(), xcb_parent_id, topLeft.x(), topLeft.y()));
}

void QXcbWindow::setWindowTitle(const QString &title)
{
    const QString fullTitle = formatWindowTitle(title, QString::fromUtf8(" \xe2\x80\x94 ")); // unicode character U+2014, EM DASH
    const QByteArray ba = fullTitle.toUtf8();
    Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                   XCB_PROP_MODE_REPLACE,
                                   m_window,
                                   atom(QXcbAtom::_NET_WM_NAME),
                                   atom(QXcbAtom::UTF8_STRING),
                                   8,
                                   ba.length(),
                                   ba.constData()));

#ifdef XCB_USE_XLIB
    XTextProperty *text = qstringToXTP(DISPLAY_FROM_XCB(this), title);
    if (text)
        XSetWMName(DISPLAY_FROM_XCB(this), m_window, text);
#endif
    xcb_flush(xcb_connection());
}

void QXcbWindow::setWindowIconText(const QString &title)
{
    const QByteArray ba = title.toUtf8();
    Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                   XCB_PROP_MODE_REPLACE,
                                   m_window,
                                   atom(QXcbAtom::_NET_WM_ICON_NAME),
                                   atom(QXcbAtom::UTF8_STRING),
                                   8,
                                   ba.length(),
                                   ba.constData()));
}

void QXcbWindow::setWindowIcon(const QIcon &icon)
{
    QVector<quint32> icon_data;
    if (!icon.isNull()) {
        QList<QSize> availableSizes = icon.availableSizes();
        if (availableSizes.isEmpty()) {
            // try to use default sizes since the icon can be a scalable image like svg.
            availableSizes.push_back(QSize(16,16));
            availableSizes.push_back(QSize(32,32));
            availableSizes.push_back(QSize(64,64));
            availableSizes.push_back(QSize(128,128));
        }
        for (int i = 0; i < availableSizes.size(); ++i) {
            QSize size = availableSizes.at(i);
            QPixmap pixmap = icon.pixmap(size);
            if (!pixmap.isNull()) {
                QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
                int pos = icon_data.size();
                icon_data.resize(pos + 2 + image.width()*image.height());
                icon_data[pos++] = image.width();
                icon_data[pos++] = image.height();
                memcpy(icon_data.data() + pos, image.bits(), image.width()*image.height()*4);
            }
        }
    }

    if (!icon_data.isEmpty()) {
        Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                       XCB_PROP_MODE_REPLACE,
                                       m_window,
                                       atom(QXcbAtom::_NET_WM_ICON),
                                       atom(QXcbAtom::CARDINAL),
                                       32,
                                       icon_data.size(),
                                       (unsigned char *) icon_data.data()));
    } else {
        Q_XCB_CALL(xcb_delete_property(xcb_connection(),
                                       m_window,
                                       atom(QXcbAtom::_NET_WM_ICON)));
    }
}

void QXcbWindow::raise()
{
    const quint32 mask = XCB_CONFIG_WINDOW_STACK_MODE;
    const quint32 values[] = { XCB_STACK_MODE_ABOVE };
    Q_XCB_CALL(xcb_configure_window(xcb_connection(), m_window, mask, values));
}

void QXcbWindow::lower()
{
    const quint32 mask = XCB_CONFIG_WINDOW_STACK_MODE;
    const quint32 values[] = { XCB_STACK_MODE_BELOW };
    Q_XCB_CALL(xcb_configure_window(xcb_connection(), m_window, mask, values));
}

// Adapt the geometry to match the WM expection with regards
// to gravity.
QRect QXcbWindow::windowToWmGeometry(QRect r) const
{
    if (m_dirtyFrameMargins || m_frameMargins.isNull())
        return r;
    const bool frameInclusive = positionIncludesFrame(window());
    // XCB_GRAVITY_STATIC requires the inner geometry, whereas
    // XCB_GRAVITY_NORTH_WEST requires the frame geometry
    if (frameInclusive && m_gravity == XCB_GRAVITY_STATIC) {
        r.translate(m_frameMargins.left(), m_frameMargins.top());
    } else if (!frameInclusive && m_gravity == XCB_GRAVITY_NORTH_WEST) {
        r.translate(-m_frameMargins.left(), -m_frameMargins.top());
    }
    return r;
}

void QXcbWindow::propagateSizeHints()
{
    // update WM_NORMAL_HINTS
    xcb_size_hints_t hints;
    memset(&hints, 0, sizeof(hints));

    const QRect xRect = windowToWmGeometry(geometry());

    QWindow *win = window();

    if (!qt_window_private(win)->positionAutomatic)
        xcb_size_hints_set_position(&hints, true, xRect.x(), xRect.y());
    if (xRect.width() < QWINDOWSIZE_MAX || xRect.height() < QWINDOWSIZE_MAX)
        xcb_size_hints_set_size(&hints, true, xRect.width(), xRect.height());
    xcb_size_hints_set_win_gravity(&hints, m_gravity);

    QSize minimumSize = windowMinimumSize();
    QSize maximumSize = windowMaximumSize();
    QSize baseSize = windowBaseSize();
    QSize sizeIncrement = windowSizeIncrement();

    if (minimumSize.width() > 0 || minimumSize.height() > 0)
        xcb_size_hints_set_min_size(&hints,
                                    qMin(XCOORD_MAX,minimumSize.width()),
                                    qMin(XCOORD_MAX,minimumSize.height()));

    if (maximumSize.width() < QWINDOWSIZE_MAX || maximumSize.height() < QWINDOWSIZE_MAX)
        xcb_size_hints_set_max_size(&hints,
                                    qMin(XCOORD_MAX, maximumSize.width()),
                                    qMin(XCOORD_MAX, maximumSize.height()));

    if (sizeIncrement.width() > 0 || sizeIncrement.height() > 0) {
        xcb_size_hints_set_base_size(&hints, baseSize.width(), baseSize.height());
        xcb_size_hints_set_resize_inc(&hints, sizeIncrement.width(), sizeIncrement.height());
    }

    xcb_set_wm_normal_hints(xcb_connection(), m_window, &hints);
}

void QXcbWindow::requestActivateWindow()
{
    /* Never activate embedded windows; doing that would prevent the container
     * to re-gain the keyboard focus later. */
    if (m_embedded) {
        QPlatformWindow::requestActivateWindow();
        return;
    }

    if (!m_mapped) {
        m_deferredActivation = true;
        return;
    }
    m_deferredActivation = false;

    updateNetWmUserTime(connection()->time());
    QWindow *focusWindow = QGuiApplication::focusWindow();

    if (window()->isTopLevel()
        && !(window()->flags() & Qt::X11BypassWindowManagerHint)
        && (!focusWindow || !window()->isAncestorOf(focusWindow))
        && connection()->wmSupport()->isSupportedByWM(atom(QXcbAtom::_NET_ACTIVE_WINDOW))) {
        xcb_client_message_event_t event;

        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.sequence = 0;
        event.window = m_window;
        event.type = atom(QXcbAtom::_NET_ACTIVE_WINDOW);
        event.data.data32[0] = 1;
        event.data.data32[1] = connection()->time();
        event.data.data32[2] = focusWindow ? focusWindow->winId() : XCB_NONE;
        event.data.data32[3] = 0;
        event.data.data32[4] = 0;

        Q_XCB_CALL(xcb_send_event(xcb_connection(), 0, xcbScreen()->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));
    } else {
        Q_XCB_CALL(xcb_set_input_focus(xcb_connection(), XCB_INPUT_FOCUS_PARENT, m_window, connection()->time()));
    }

    connection()->sync();
}

QSurfaceFormat QXcbWindow::format() const
{
    return m_format;
}

void QXcbWindow::setWmWindowTypeStatic(QWindow *window, QXcbWindowFunctions::WmWindowTypes windowTypes)
{
    window->setProperty(wm_window_type_property_id, QVariant::fromValue(static_cast<int>(windowTypes)));

    if (window->handle())
        static_cast<QXcbWindow *>(window->handle())->setWmWindowType(windowTypes, window->flags());
}

void QXcbWindow::setWindowIconTextStatic(QWindow *window, const QString &text)
{
    if (window->handle())
        static_cast<QXcbWindow *>(window->handle())->setWindowIconText(text);
}

void QXcbWindow::setWmWindowRoleStatic(QWindow *window, const QByteArray &role)
{
    if (window->handle())
        static_cast<QXcbWindow *>(window->handle())->setWmWindowRole(role);
    else
        window->setProperty(wm_window_role_property_id, role);
}

uint QXcbWindow::visualIdStatic(QWindow *window)
{
    if (window && window->handle())
        return static_cast<QXcbWindow *>(window->handle())->visualId();
    return UINT_MAX;
}

QXcbWindowFunctions::WmWindowTypes QXcbWindow::wmWindowTypes() const
{
    QXcbWindowFunctions::WmWindowTypes result(0);

    xcb_get_property_cookie_t get_cookie =
        xcb_get_property_unchecked(xcb_connection(), 0, m_window, atom(QXcbAtom::_NET_WM_WINDOW_TYPE),
                         XCB_ATOM_ATOM, 0, 1024);

    xcb_get_property_reply_t *reply =
        xcb_get_property_reply(xcb_connection(), get_cookie, NULL);

    if (reply && reply->format == 32 && reply->type == XCB_ATOM_ATOM) {
        const xcb_atom_t *types = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply));
        const xcb_atom_t *types_end = types + reply->length;
        for (; types != types_end; types++) {
            QXcbAtom::Atom type = connection()->qatom(*types);
            switch (type) {
            case QXcbAtom::_NET_WM_WINDOW_TYPE_NORMAL:
                result |= QXcbWindowFunctions::Normal;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_DESKTOP:
                result |= QXcbWindowFunctions::Desktop;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_DOCK:
                result |= QXcbWindowFunctions::Dock;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_TOOLBAR:
                result |= QXcbWindowFunctions::Toolbar;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_MENU:
                result |= QXcbWindowFunctions::Menu;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_UTILITY:
                result |= QXcbWindowFunctions::Utility;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_SPLASH:
                result |= QXcbWindowFunctions::Splash;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_DIALOG:
                result |= QXcbWindowFunctions::Dialog;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_DROPDOWN_MENU:
                result |= QXcbWindowFunctions::DropDownMenu;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_POPUP_MENU:
                result |= QXcbWindowFunctions::PopupMenu;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_TOOLTIP:
                result |= QXcbWindowFunctions::Tooltip;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_NOTIFICATION:
                result |= QXcbWindowFunctions::Notification;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_COMBO:
                result |= QXcbWindowFunctions::Combo;
                break;
            case QXcbAtom::_NET_WM_WINDOW_TYPE_DND:
                result |= QXcbWindowFunctions::Dnd;
                break;
            case QXcbAtom::_KDE_NET_WM_WINDOW_TYPE_OVERRIDE:
                result |= QXcbWindowFunctions::KdeOverride;
                break;
            default:
                break;
            }
        }
        free(reply);
    }
    return result;
}

void QXcbWindow::setWmWindowType(QXcbWindowFunctions::WmWindowTypes types, Qt::WindowFlags flags)
{
    QVector<xcb_atom_t> atoms;

    // manual selection 1 (these are never set by Qt and take precedence)
    if (types & QXcbWindowFunctions::Normal)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_NORMAL));
    if (types & QXcbWindowFunctions::Desktop)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_DESKTOP));
    if (types & QXcbWindowFunctions::Dock)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_DOCK));
    if (types & QXcbWindowFunctions::Notification)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_NOTIFICATION));

    // manual selection 2 (Qt uses these during auto selection);
    if (types & QXcbWindowFunctions::Utility)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_UTILITY));
    if (types & QXcbWindowFunctions::Splash)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_SPLASH));
    if (types & QXcbWindowFunctions::Dialog)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_DIALOG));
    if (types & QXcbWindowFunctions::Tooltip)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_TOOLTIP));
    if (types & QXcbWindowFunctions::KdeOverride)
        atoms.append(atom(QXcbAtom::_KDE_NET_WM_WINDOW_TYPE_OVERRIDE));

    // manual selection 3 (these can be set by Qt, but don't have a
    // corresponding Qt::WindowType). note that order of the *MENU
    // atoms is important
    if (types & QXcbWindowFunctions::Menu)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_MENU));
    if (types & QXcbWindowFunctions::DropDownMenu)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_DROPDOWN_MENU));
    if (types & QXcbWindowFunctions::PopupMenu)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_POPUP_MENU));
    if (types & QXcbWindowFunctions::Toolbar)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_TOOLBAR));
    if (types & QXcbWindowFunctions::Combo)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_COMBO));
    if (types & QXcbWindowFunctions::Dnd)
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_DND));

    // automatic selection
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));
    switch (type) {
    case Qt::Dialog:
    case Qt::Sheet:
        if (!(types & QXcbWindowFunctions::Dialog))
            atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_DIALOG));
        break;
    case Qt::Tool:
    case Qt::Drawer:
        if (!(types & QXcbWindowFunctions::Utility))
            atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_UTILITY));
        break;
    case Qt::ToolTip:
        if (!(types & QXcbWindowFunctions::Tooltip))
            atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_TOOLTIP));
        break;
    case Qt::SplashScreen:
        if (!(types & QXcbWindowFunctions::Splash))
            atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_SPLASH));
        break;
    default:
        break;
    }

    if ((flags & Qt::FramelessWindowHint) && !(type & QXcbWindowFunctions::KdeOverride)) {
        // override netwm type - quick and easy for KDE noborder
        atoms.append(atom(QXcbAtom::_KDE_NET_WM_WINDOW_TYPE_OVERRIDE));
    }

    if (atoms.size() == 1 && atoms.first() == atom(QXcbAtom::_NET_WM_WINDOW_TYPE_NORMAL))
        atoms.clear();
    else
        atoms.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_NORMAL));

    if (atoms.isEmpty()) {
        Q_XCB_CALL(xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::_NET_WM_WINDOW_TYPE)));
    } else {
        Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                       atom(QXcbAtom::_NET_WM_WINDOW_TYPE), XCB_ATOM_ATOM, 32,
                                       atoms.count(), atoms.constData()));
    }
    xcb_flush(xcb_connection());
}

void QXcbWindow::setWmWindowRole(const QByteArray &role)
{
    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::WM_WINDOW_ROLE), XCB_ATOM_STRING, 8,
                                   role.size(), role.constData()));
}

void QXcbWindow::setParentRelativeBackPixmapStatic(QWindow *window)
{
    if (window->handle())
        static_cast<QXcbWindow *>(window->handle())->setParentRelativeBackPixmap();
}

void QXcbWindow::setParentRelativeBackPixmap()
{
    const quint32 mask = XCB_CW_BACK_PIXMAP;
    const quint32 values[] = { XCB_BACK_PIXMAP_PARENT_RELATIVE };
    Q_XCB_CALL(xcb_change_window_attributes(xcb_connection(), m_window, mask, values));
}

bool QXcbWindow::requestSystemTrayWindowDockStatic(const QWindow *window)
{
    if (window->handle())
        return static_cast<QXcbWindow *>(window->handle())->requestSystemTrayWindowDock();
    return false;
}

bool QXcbWindow::requestSystemTrayWindowDock() const
{
    if (!connection()->systemTrayTracker())
        return false;
    connection()->systemTrayTracker()->requestSystemTrayWindowDock(m_window);
    return true;
}

QRect QXcbWindow::systemTrayWindowGlobalGeometryStatic(const QWindow *window)
{
    if (window->handle())
        return static_cast<QXcbWindow *>(window->handle())->systemTrayWindowGlobalGeometry();
    return QRect();
}

QRect QXcbWindow::systemTrayWindowGlobalGeometry() const
{
   if (!connection()->systemTrayTracker())
       return QRect();
   return connection()->systemTrayTracker()->systemTrayWindowGlobalGeometry(m_window);
}

class ExposeCompressor
{
public:
    ExposeCompressor(xcb_window_t window, QRegion *region)
        : m_window(window)
        , m_region(region)
        , m_pending(true)
    {
    }

    bool checkEvent(xcb_generic_event_t *event)
    {
        if (!event)
            return false;
        if ((event->response_type & ~0x80) != XCB_EXPOSE)
            return false;
        xcb_expose_event_t *expose = (xcb_expose_event_t *)event;
        if (expose->window != m_window)
            return false;
        if (expose->count == 0)
            m_pending = false;
        *m_region |= QRect(expose->x, expose->y, expose->width, expose->height);
        return true;
    }

    bool pending() const
    {
        return m_pending;
    }

private:
    xcb_window_t m_window;
    QRegion *m_region;
    bool m_pending;
};

bool QXcbWindow::compressExposeEvent(QRegion &exposeRegion)
{
    ExposeCompressor compressor(m_window, &exposeRegion);
    xcb_generic_event_t *filter = 0;
    do {
        filter = connection()->checkEvent(compressor);
        free(filter);
    } while (filter);
    return compressor.pending();
}

bool QXcbWindow::handleGenericEvent(xcb_generic_event_t *event, long *result)
{
    return QWindowSystemInterface::handleNativeEvent(window(),
                                                     connection()->nativeInterface()->genericEventFilterType(),
                                                     event,
                                                     result);
}

void QXcbWindow::handleExposeEvent(const xcb_expose_event_t *event)
{
    QRect rect(event->x, event->y, event->width, event->height);

    if (m_exposeRegion.isEmpty())
        m_exposeRegion = rect;
    else
        m_exposeRegion |= rect;

    bool pending = compressExposeEvent(m_exposeRegion);

    // if count is non-zero there are more expose events pending
    if (event->count == 0 || !pending) {
        QWindowSystemInterface::handleExposeEvent(window(), m_exposeRegion);
        m_exposeRegion = QRegion();
    }
}

void QXcbWindow::handleClientMessageEvent(const xcb_client_message_event_t *event)
{
    if (event->format != 32)
        return;

    if (event->type == atom(QXcbAtom::WM_PROTOCOLS)) {
        if (event->data.data32[0] == atom(QXcbAtom::WM_DELETE_WINDOW)) {
            QWindowSystemInterface::handleCloseEvent(window());
        } else if (event->data.data32[0] == atom(QXcbAtom::WM_TAKE_FOCUS)) {
            connection()->setTime(event->data.data32[1]);
            relayFocusToModalWindow();
            return;
        } else if (event->data.data32[0] == atom(QXcbAtom::_NET_WM_PING)) {
            if (event->window == xcbScreen()->root())
                return;

            xcb_client_message_event_t reply = *event;

            reply.response_type = XCB_CLIENT_MESSAGE;
            reply.window = xcbScreen()->root();

            xcb_send_event(xcb_connection(), 0, xcbScreen()->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&reply);
            xcb_flush(xcb_connection());
        } else if (event->data.data32[0] == atom(QXcbAtom::_NET_WM_SYNC_REQUEST)) {
            connection()->setTime(event->data.data32[1]);
            m_syncValue.lo = event->data.data32[2];
            m_syncValue.hi = event->data.data32[3];
            if (m_usingSyncProtocol)
                m_syncState = SyncReceived;
#ifndef QT_NO_WHATSTHIS
        } else if (event->data.data32[0] == atom(QXcbAtom::_NET_WM_CONTEXT_HELP)) {
            QWindowSystemInterface::handleEnterWhatsThisEvent();
#endif
        } else {
            qWarning() << "QXcbWindow: Unhandled WM_PROTOCOLS message:" << connection()->atomName(event->data.data32[0]);
        }
#ifndef QT_NO_DRAGANDDROP
    } else if (event->type == atom(QXcbAtom::XdndEnter)) {
        connection()->drag()->handleEnter(this, event);
    } else if (event->type == atom(QXcbAtom::XdndPosition)) {
        connection()->drag()->handlePosition(this, event);
    } else if (event->type == atom(QXcbAtom::XdndLeave)) {
        connection()->drag()->handleLeave(this, event);
    } else if (event->type == atom(QXcbAtom::XdndDrop)) {
        connection()->drag()->handleDrop(this, event);
#endif
    } else if (event->type == atom(QXcbAtom::_XEMBED)) {
        handleXEmbedMessage(event);
    } else if (event->type == atom(QXcbAtom::_NET_ACTIVE_WINDOW)) {
        doFocusIn();
    } else if (event->type == atom(QXcbAtom::MANAGER)
               || event->type == atom(QXcbAtom::_NET_WM_STATE)
               || event->type == atom(QXcbAtom::WM_CHANGE_STATE)) {
        // Ignore _NET_WM_STATE, MANAGER which are relate to tray icons
        // and other messages.
    } else if (event->type == atom(QXcbAtom::_COMPIZ_DECOR_PENDING)
            || event->type == atom(QXcbAtom::_COMPIZ_DECOR_REQUEST)
            || event->type == atom(QXcbAtom::_COMPIZ_DECOR_DELETE_PIXMAP)
            || event->type == atom(QXcbAtom::_COMPIZ_TOOLKIT_ACTION)
            || event->type == atom(QXcbAtom::_GTK_LOAD_ICONTHEMES)) {
        //silence the _COMPIZ and _GTK messages for now
    } else {
        qWarning() << "QXcbWindow: Unhandled client message:" << connection()->atomName(event->type);
    }
}

void QXcbWindow::handleConfigureNotifyEvent(const xcb_configure_notify_event_t *event)
{
    bool fromSendEvent = (event->response_type & 0x80);
    QPoint pos(event->x, event->y);
    if (!parent() && !fromSendEvent) {
        // Do not trust the position, query it instead.
        xcb_translate_coordinates_cookie_t cookie = xcb_translate_coordinates(xcb_connection(), xcb_window(),
                                                                              xcbScreen()->root(), 0, 0);
        xcb_translate_coordinates_reply_t *reply = xcb_translate_coordinates_reply(xcb_connection(), cookie, NULL);
        if (reply) {
            pos.setX(reply->dst_x);
            pos.setY(reply->dst_y);
            free(reply);
        }
    }

    const QRect actualGeometry = QRect(pos, QSize(event->width, event->height));
    QPlatformScreen *newScreen = parent() ? parent()->screen() : screenForGeometry(actualGeometry);
    if (!newScreen)
        return;

    // Persist the actual geometry so that QWindow::geometry() can
    // be queried in the resize event.
    QPlatformWindow::setGeometry(actualGeometry);

    // FIXME: In the case of the requestedGeometry not matching the actualGeometry due
    // to e.g. the window manager applying restrictions to the geometry, the application
    // will never see a move/resize event if the actualGeometry is the same as the current
    // geometry, and may think the requested geometry was fulfilled.
    QWindowSystemInterface::handleGeometryChange(window(), actualGeometry);

    // QPlatformScreen::screen() is updated asynchronously, so we can't compare it
    // with the newScreen. Just send the WindowScreenChanged event and QGuiApplication
    // will make the comparison later.
    QWindowSystemInterface::handleWindowScreenChanged(window(), newScreen->screen());

    // Send the synthetic expose event on resize only when the window is shrinked,
    // because the "XCB_GRAVITY_NORTH_WEST" flag doesn't send it automatically.
    if (!m_oldWindowSize.isEmpty()
            && (actualGeometry.width() < m_oldWindowSize.width()
                || actualGeometry.height() < m_oldWindowSize.height())) {
        QWindowSystemInterface::handleExposeEvent(window(), QRegion(0, 0, actualGeometry.width(), actualGeometry.height()));
    }
    m_oldWindowSize = actualGeometry.size();

    if (m_usingSyncProtocol && m_syncState == SyncReceived)
        m_syncState = SyncAndConfigureReceived;

    m_dirtyFrameMargins = true;
}

bool QXcbWindow::isExposed() const
{
    return m_mapped;
}

bool QXcbWindow::isEmbedded(const QPlatformWindow *parentWindow) const
{
    if (!m_embedded)
        return false;

    return parentWindow ? (parentWindow == parent()) : true;
}

QPoint QXcbWindow::mapToGlobal(const QPoint &pos) const
{
    if (!m_embedded)
        return pos;

    QPoint ret;
    xcb_translate_coordinates_cookie_t cookie =
        xcb_translate_coordinates(xcb_connection(), xcb_window(), xcbScreen()->root(),
                                  pos.x(), pos.y());
    xcb_translate_coordinates_reply_t *reply =
        xcb_translate_coordinates_reply(xcb_connection(), cookie, NULL);
    if (reply) {
        ret.setX(reply->dst_x);
        ret.setY(reply->dst_y);
        free(reply);
    }

    return ret;
}

QPoint QXcbWindow::mapFromGlobal(const QPoint &pos) const
{
    if (!m_embedded)
        return pos;

    QPoint ret;
    xcb_translate_coordinates_cookie_t cookie =
        xcb_translate_coordinates(xcb_connection(), xcbScreen()->root(), xcb_window(),
                                  pos.x(), pos.y());
    xcb_translate_coordinates_reply_t *reply =
        xcb_translate_coordinates_reply(xcb_connection(), cookie, NULL);
    if (reply) {
        ret.setX(reply->dst_x);
        ret.setY(reply->dst_y);
        free(reply);
    }

    return ret;
}

void QXcbWindow::handleMapNotifyEvent(const xcb_map_notify_event_t *event)
{
    if (event->window == m_window) {
        m_mapped = true;
        if (m_deferredActivation)
            requestActivateWindow();

        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
    }
}

void QXcbWindow::handleUnmapNotifyEvent(const xcb_unmap_notify_event_t *event)
{
    if (event->window == m_window) {
        m_mapped = false;
        QWindowSystemInterface::handleExposeEvent(window(), QRegion());
    }
}

void QXcbWindow::handleButtonPressEvent(int event_x, int event_y, int root_x, int root_y,
                                        int detail, Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp, Qt::MouseEventSource source)
{
    const bool isWheel = detail >= 4 && detail <= 7;
    if (!isWheel && window() != QGuiApplication::focusWindow()) {
        QWindow *w = static_cast<QWindowPrivate *>(QObjectPrivate::get(window()))->eventReceiver();
        if (!(w->flags() & (Qt::WindowDoesNotAcceptFocus | Qt::BypassWindowManagerHint))
                && w->type() != Qt::ToolTip
                && w->type() != Qt::Popup) {
            w->requestActivate();
        }
    }

    updateNetWmUserTime(timestamp);

    if (m_embedded) {
        if (window() != QGuiApplication::focusWindow()) {
            const QXcbWindow *container = static_cast<const QXcbWindow *>(parent());
            Q_ASSERT(container != 0);

            sendXEmbedMessage(container->xcb_window(), XEMBED_REQUEST_FOCUS);
        }
    }
    QPoint local(event_x, event_y);
    QPoint global(root_x, root_y);

    if (isWheel) {
        if (!connection()->isAtLeastXI21()) {
            // Logic borrowed from qapplication_x11.cpp
            int delta = 120 * ((detail == 4 || detail == 6) ? 1 : -1);
            bool hor = (((detail == 4 || detail == 5)
                         && (modifiers & Qt::AltModifier))
                        || (detail == 6 || detail == 7));

            QWindowSystemInterface::handleWheelEvent(window(), timestamp,
                                                     local, global, delta, hor ? Qt::Horizontal : Qt::Vertical, modifiers);
        }
        return;
    }

    connection()->setMousePressWindow(this);

    handleMouseEvent(timestamp, local, global, modifiers, source);
}

void QXcbWindow::handleButtonReleaseEvent(int event_x, int event_y, int root_x, int root_y,
                                          int detail, Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp, Qt::MouseEventSource source)
{
    QPoint local(event_x, event_y);
    QPoint global(root_x, root_y);

    if (detail >= 4 && detail <= 7) {
        // mouse wheel, handled in handleButtonPressEvent()
        return;
    }

    if (connection()->buttons() == Qt::NoButton)
        connection()->setMousePressWindow(Q_NULLPTR);

    handleMouseEvent(timestamp, local, global, modifiers, source);
}

static inline bool doCheckUnGrabAncestor(QXcbConnection *conn)
{
    /* Checking for XCB_NOTIFY_MODE_GRAB and XCB_NOTIFY_DETAIL_ANCESTOR prevents unwanted
     * enter/leave events on AwesomeWM on mouse button press. It also ignores duplicated
     * enter/leave events on Alt+Tab switching on some WMs with XInput2 events.
     * Without XInput2 events the (Un)grabAncestor cannot be checked when mouse button is
     * not pressed, otherwise (e.g. on Alt+Tab) it can igonre important enter/leave events.
    */
    if (conn) {
        const bool mouseButtonsPressed = (conn->buttons() != Qt::NoButton);
#ifdef XCB_USE_XINPUT22
        return mouseButtonsPressed || (conn->isAtLeastXI22() && conn->xi2MouseEvents());
#else
        return mouseButtonsPressed;
#endif
    }
    return true;
}

static bool ignoreLeaveEvent(quint8 mode, quint8 detail, QXcbConnection *conn = Q_NULLPTR)
{
    return ((doCheckUnGrabAncestor(conn)
             && mode == XCB_NOTIFY_MODE_GRAB && detail == XCB_NOTIFY_DETAIL_ANCESTOR)
            || (mode == XCB_NOTIFY_MODE_UNGRAB && detail == XCB_NOTIFY_DETAIL_INFERIOR)
            || detail == XCB_NOTIFY_DETAIL_VIRTUAL
            || detail == XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL);
}

static bool ignoreEnterEvent(quint8 mode, quint8 detail, QXcbConnection *conn = Q_NULLPTR)
{
    return ((doCheckUnGrabAncestor(conn)
             && mode == XCB_NOTIFY_MODE_UNGRAB && detail == XCB_NOTIFY_DETAIL_ANCESTOR)
            || (mode != XCB_NOTIFY_MODE_NORMAL && mode != XCB_NOTIFY_MODE_UNGRAB)
            || detail == XCB_NOTIFY_DETAIL_VIRTUAL
            || detail == XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL);
}

class EnterEventChecker
{
public:
    bool checkEvent(xcb_generic_event_t *event)
    {
        if (!event)
            return false;
        if ((event->response_type & ~0x80) != XCB_ENTER_NOTIFY)
            return false;

        xcb_enter_notify_event_t *enter = (xcb_enter_notify_event_t *)event;
        if (ignoreEnterEvent(enter->mode, enter->detail))
            return false;

        return true;
    }
};

void QXcbWindow::handleEnterNotifyEvent(int event_x, int event_y, int root_x, int root_y,
                                        quint8 mode, quint8 detail, xcb_timestamp_t timestamp)
{
    connection()->setTime(timestamp);
#ifdef XCB_USE_XINPUT21
    connection()->handleEnterEvent();
#endif

    const QPoint global = QPoint(root_x, root_y);

    if (ignoreEnterEvent(mode, detail, connection()) || connection()->mousePressWindow())
        return;

    const QPoint local(event_x, event_y);
    QWindowSystemInterface::handleEnterEvent(window(), local, global);
}

void QXcbWindow::handleLeaveNotifyEvent(int root_x, int root_y,
                                        quint8 mode, quint8 detail, xcb_timestamp_t timestamp)
{
    connection()->setTime(timestamp);

    if (ignoreLeaveEvent(mode, detail, connection()) || connection()->mousePressWindow())
        return;

    EnterEventChecker checker;
    xcb_enter_notify_event_t *enter = (xcb_enter_notify_event_t *)connection()->checkEvent(checker);
    QXcbWindow *enterWindow = enter ? connection()->platformWindowFromId(enter->event) : 0;

    if (enterWindow) {
        QPoint local(enter->event_x, enter->event_y);
        QPoint global = QPoint(root_x, root_y);
        QWindowSystemInterface::handleEnterLeaveEvent(enterWindow->window(), window(), local, global);
    } else {
        QWindowSystemInterface::handleLeaveEvent(window());
    }

    free(enter);
}

void QXcbWindow::handleMotionNotifyEvent(int event_x, int event_y, int root_x, int root_y,
                                         Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp, Qt::MouseEventSource source)
{
    QPoint local(event_x, event_y);
    QPoint global(root_x, root_y);

    // "mousePressWindow" can be NULL i.e. if a window will be grabbed or unmapped, so set it again here.
    // Unset "mousePressWindow" when mouse button isn't pressed - in some cases the release event won't arrive.
    const bool isMouseButtonPressed = (connection()->buttons() != Qt::NoButton);
    const bool hasMousePressWindow = (connection()->mousePressWindow() != Q_NULLPTR);
    if (isMouseButtonPressed && !hasMousePressWindow)
        connection()->setMousePressWindow(this);
    else if (hasMousePressWindow && !isMouseButtonPressed)
        connection()->setMousePressWindow(Q_NULLPTR);

    handleMouseEvent(timestamp, local, global, modifiers, source);
}

// Handlers for plain xcb events. Used only when XI 2.2 or newer is not available.
void QXcbWindow::handleButtonPressEvent(const xcb_button_press_event_t *event)
{
    Qt::KeyboardModifiers modifiers = connection()->keyboard()->translateModifiers(event->state);
    handleButtonPressEvent(event->event_x, event->event_y, event->root_x, event->root_y, event->detail,
                           modifiers, event->time);
}

void QXcbWindow::handleButtonReleaseEvent(const xcb_button_release_event_t *event)
{
    Qt::KeyboardModifiers modifiers = connection()->keyboard()->translateModifiers(event->state);
    handleButtonReleaseEvent(event->event_x, event->event_y, event->root_x, event->root_y, event->detail,
                             modifiers, event->time);
}

void QXcbWindow::handleMotionNotifyEvent(const xcb_motion_notify_event_t *event)
{
    Qt::KeyboardModifiers modifiers = connection()->keyboard()->translateModifiers(event->state);
    handleMotionNotifyEvent(event->event_x, event->event_y, event->root_x, event->root_y, modifiers, event->time);
}

#ifdef XCB_USE_XINPUT22
static inline int fixed1616ToInt(FP1616 val)
{
    return int((qreal(val >> 16)) + (val & 0xFFFF) / (qreal)0xFFFF);
}

void QXcbWindow::handleXIMouseButtonState(const xcb_ge_event_t *event)
{
    QXcbConnection *conn = connection();
    const xXIDeviceEvent *ev = reinterpret_cast<const xXIDeviceEvent *>(event);
    if (ev->buttons_len > 0) {
        unsigned char *buttonMask = (unsigned char *) &ev[1];
        for (int i = 1; i <= 15; ++i)
            conn->setButton(conn->translateMouseButton(i), XIMaskIsSet(buttonMask, i));
    }
}

// With XI 2.2+ press/release/motion comes here instead of the above handlers.
void QXcbWindow::handleXIMouseEvent(xcb_ge_event_t *event, Qt::MouseEventSource source)
{
    QXcbConnection *conn = connection();
    xXIDeviceEvent *ev = reinterpret_cast<xXIDeviceEvent *>(event);
    const Qt::KeyboardModifiers modifiers = conn->keyboard()->translateModifiers(ev->mods.effective_mods);
    const int event_x = fixed1616ToInt(ev->event_x);
    const int event_y = fixed1616ToInt(ev->event_y);
    const int root_x = fixed1616ToInt(ev->root_x);
    const int root_y = fixed1616ToInt(ev->root_y);

    conn->keyboard()->updateXKBStateFromXI(&ev->mods, &ev->group);

    const Qt::MouseButton button = conn->xiToQtMouseButton(ev->detail);

    const char *sourceName = 0;
    if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled())) {
        const QMetaObject *metaObject = qt_getEnumMetaObject(source);
        const QMetaEnum me = metaObject->enumerator(metaObject->indexOfEnumerator(qt_getEnumName(source)));
        sourceName = me.valueToKey(source);
    }

    switch (ev->evtype) {
    case XI_ButtonPress:
        handleXIMouseButtonState(event);
        if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
            qCDebug(lcQpaXInputEvents, "XI2 mouse press, button %d, time %d, source %s", button, ev->time, sourceName);
        conn->setButton(button, true);
        handleButtonPressEvent(event_x, event_y, root_x, root_y, ev->detail, modifiers, ev->time, source);
        break;
    case XI_ButtonRelease:
        handleXIMouseButtonState(event);
        if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
            qCDebug(lcQpaXInputEvents, "XI2 mouse release, button %d, time %d, source %s", button, ev->time, sourceName);
        conn->setButton(button, false);
        handleButtonReleaseEvent(event_x, event_y, root_x, root_y, ev->detail, modifiers, ev->time, source);
        break;
    case XI_Motion:
        // Here we do NOT call handleXIMouseButtonState because we don't expect button state change to be bundled with motion.
        // When a touchscreen is pressed, an XI_Motion event occurs in which XIMaskIsSet says the left button is pressed,
        // but we don't want QGuiApplicationPrivate::processMouseEvent() to react by generating a mouse press event.
        if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
            qCDebug(lcQpaXInputEvents, "XI2 mouse motion %d,%d, time %d, source %s", event_x, event_y, ev->time, sourceName);
        handleMotionNotifyEvent(event_x, event_y, root_x, root_y, modifiers, ev->time, source);
        break;
    default:
        qWarning() << "Unrecognized XI2 mouse event" << ev->evtype;
        break;
    }
}

// With XI 2.2+ enter/leave comes here and are blocked in plain xcb events
void QXcbWindow::handleXIEnterLeave(xcb_ge_event_t *event)
{
    xXIEnterEvent *ev = reinterpret_cast<xXIEnterEvent *>(event);

    // Compare the window with current mouse grabber to prevent deliver events to any other windows.
    // If leave event occurs and the window is under mouse - allow to deliver the leave event.
    QXcbWindow *mouseGrabber = connection()->mouseGrabber();
    if (mouseGrabber && mouseGrabber != this
            && (ev->evtype != XI_Leave || QGuiApplicationPrivate::currentMouseWindow != window())) {
        return;
    }

    const int root_x = fixed1616ToInt(ev->root_x);
    const int root_y = fixed1616ToInt(ev->root_y);

    switch (ev->evtype) {
    case XI_Enter: {
        const int event_x = fixed1616ToInt(ev->event_x);
        const int event_y = fixed1616ToInt(ev->event_y);
        qCDebug(lcQpaXInput, "XI2 mouse enter %d,%d, mode %d, detail %d, time %d", event_x, event_y, ev->mode, ev->detail, ev->time);
        handleEnterNotifyEvent(event_x, event_y, root_x, root_y, ev->mode, ev->detail, ev->time);
        break;
    }
    case XI_Leave:
        qCDebug(lcQpaXInput, "XI2 mouse leave, mode %d, detail %d, time %d", ev->mode, ev->detail, ev->time);
        connection()->keyboard()->updateXKBStateFromXI(&ev->mods, &ev->group);
        handleLeaveNotifyEvent(root_x, root_y, ev->mode, ev->detail, ev->time);
        break;
    }
}
#endif

QXcbWindow *QXcbWindow::toWindow() { return this; }

void QXcbWindow::handleMouseEvent(xcb_timestamp_t time, const QPoint &local, const QPoint &global,
        Qt::KeyboardModifiers modifiers, Qt::MouseEventSource source)
{
    connection()->setTime(time);
    QWindowSystemInterface::handleMouseEvent(window(), time, local, global, connection()->buttons(), modifiers, source);
}

void QXcbWindow::handleEnterNotifyEvent(const xcb_enter_notify_event_t *event)
{
    handleEnterNotifyEvent(event->event_x, event->event_y, event->root_x, event->root_y, event->mode, event->detail, event->time);
}

void QXcbWindow::handleLeaveNotifyEvent(const xcb_leave_notify_event_t *event)
{
    handleLeaveNotifyEvent(event->root_x, event->root_y, event->mode, event->detail, event->time);
}

void QXcbWindow::handlePropertyNotifyEvent(const xcb_property_notify_event_t *event)
{
    connection()->setTime(event->time);

    const bool propertyDeleted = event->state == XCB_PROPERTY_DELETE;

    if (event->atom == atom(QXcbAtom::_NET_WM_STATE) || event->atom == atom(QXcbAtom::WM_STATE)) {
        if (propertyDeleted)
            return;

        Qt::WindowState newState = Qt::WindowNoState;
        if (event->atom == atom(QXcbAtom::WM_STATE)) { // WM_STATE: Quick check for 'Minimize'.
            const xcb_get_property_cookie_t get_cookie =
            xcb_get_property(xcb_connection(), 0, m_window, atom(QXcbAtom::WM_STATE),
                             XCB_ATOM_ANY, 0, 1024);

            xcb_get_property_reply_t *reply =
                xcb_get_property_reply(xcb_connection(), get_cookie, NULL);

            if (reply && reply->format == 32 && reply->type == atom(QXcbAtom::WM_STATE)) {
                const quint32 *data = (const quint32 *)xcb_get_property_value(reply);
                if (reply->length != 0) {
                    if (data[0] == XCB_WM_STATE_ICONIC
                            || (data[0] == XCB_WM_STATE_WITHDRAWN
                                && m_lastWindowStateEvent == Qt::WindowMinimized)) {
                        newState = Qt::WindowMinimized;
                    }
                }
            }
            free(reply);
        } else { // _NET_WM_STATE can't change minimized state
            if (m_lastWindowStateEvent == Qt::WindowMinimized)
                newState = Qt::WindowMinimized;
        }

        if (newState != Qt::WindowMinimized) { // Something else changed, get _NET_WM_STATE.
            const NetWmStates states = netWmStates();
            if (states & NetWmStateFullScreen)
                newState = Qt::WindowFullScreen;
            else if ((states & NetWmStateMaximizedHorz) && (states & NetWmStateMaximizedVert))
                newState = Qt::WindowMaximized;
        }
        // Send Window state, compress events in case other flags (modality, etc) are changed.
        if (m_lastWindowStateEvent != newState) {
            QWindowSystemInterface::handleWindowStateChanged(window(), newState);
            m_lastWindowStateEvent = newState;
            m_windowState = newState;
            if (m_windowState == Qt::WindowMinimized && connection()->mouseGrabber() == this)
                connection()->setMouseGrabber(Q_NULLPTR);
        }
        return;
    } else if (event->atom == atom(QXcbAtom::_NET_FRAME_EXTENTS)) {
        m_dirtyFrameMargins = true;
    }
}

void QXcbWindow::handleFocusInEvent(const xcb_focus_in_event_t *event)
{
    // Ignore focus events that are being sent only because the pointer is over
    // our window, even if the input focus is in a different window.
    if (event->detail == XCB_NOTIFY_DETAIL_POINTER)
        return;
    doFocusIn();
}


void QXcbWindow::handleFocusOutEvent(const xcb_focus_out_event_t *event)
{
    // Ignore focus events that are being sent only because the pointer is over
    // our window, even if the input focus is in a different window.
    if (event->detail == XCB_NOTIFY_DETAIL_POINTER)
        return;
    doFocusOut();
}

void QXcbWindow::updateSyncRequestCounter()
{
    if (m_syncState != SyncAndConfigureReceived) {
        // window manager does not expect a sync event yet.
        return;
    }
    if (m_usingSyncProtocol && (m_syncValue.lo != 0 || m_syncValue.hi != 0)) {
        Q_XCB_CALL(xcb_sync_set_counter(xcb_connection(), m_syncCounter, m_syncValue));
        xcb_flush(xcb_connection());

        m_syncValue.lo = 0;
        m_syncValue.hi = 0;
        m_syncState = NoSyncNeeded;
    }
}

const xcb_visualtype_t *QXcbWindow::createVisual()
{
    return xcbScreen() ? xcbScreen()->visualForFormat(m_format)
                       : nullptr;
}

bool QXcbWindow::setKeyboardGrabEnabled(bool grab)
{
    if (grab && !connection()->canGrab())
        return false;

    if (!grab) {
        xcb_ungrab_keyboard(xcb_connection(), XCB_TIME_CURRENT_TIME);
        return true;
    }
    xcb_grab_keyboard_cookie_t cookie = xcb_grab_keyboard(xcb_connection(), false,
                                                          m_window, XCB_TIME_CURRENT_TIME,
                                                          XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_grab_keyboard_reply_t *reply = xcb_grab_keyboard_reply(xcb_connection(), cookie, NULL);
    bool result = !(!reply || reply->status != XCB_GRAB_STATUS_SUCCESS);
    free(reply);
    return result;
}

bool QXcbWindow::setMouseGrabEnabled(bool grab)
{
    if (!grab && connection()->mouseGrabber() == this)
        connection()->setMouseGrabber(Q_NULLPTR);
#ifdef XCB_USE_XINPUT22
    if (connection()->isAtLeastXI22() && connection()->xi2MouseEvents()) {
        bool result = connection()->xi2SetMouseGrabEnabled(m_window, grab);
        if (grab && result)
            connection()->setMouseGrabber(this);
        return result;
    }
#endif
    if (grab && !connection()->canGrab())
        return false;

    if (!grab) {
        xcb_ungrab_pointer(xcb_connection(), XCB_TIME_CURRENT_TIME);
        return true;
    }
    xcb_grab_pointer_cookie_t cookie = xcb_grab_pointer(xcb_connection(), false, m_window,
                                                        (XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
                                                         | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_ENTER_WINDOW
                                                         | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION),
                                                        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                                                        XCB_WINDOW_NONE, XCB_CURSOR_NONE,
                                                        XCB_TIME_CURRENT_TIME);
    xcb_grab_pointer_reply_t *reply = xcb_grab_pointer_reply(xcb_connection(), cookie, NULL);
    bool result = !(!reply || reply->status != XCB_GRAB_STATUS_SUCCESS);
    free(reply);
    if (result)
        connection()->setMouseGrabber(this);
    return result;
}

void QXcbWindow::setCursor(xcb_cursor_t cursor, bool isBitmapCursor)
{
    xcb_connection_t *conn = xcb_connection();

    xcb_change_window_attributes(conn, m_window, XCB_CW_CURSOR, &cursor);
    xcb_flush(conn);

    if (m_currentBitmapCursor != XCB_CURSOR_NONE) {
        xcb_free_cursor(conn, m_currentBitmapCursor);
    }

    if (isBitmapCursor) {
        m_currentBitmapCursor = cursor;
    } else {
        m_currentBitmapCursor = XCB_CURSOR_NONE;
    }
}

void QXcbWindow::windowEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::FocusIn:
        if (m_embedded && !event->spontaneous()) {
            QFocusEvent *focusEvent = static_cast<QFocusEvent *>(event);
            switch (focusEvent->reason()) {
            case Qt::TabFocusReason:
            case Qt::BacktabFocusReason:
                {
                const QXcbWindow *container =
                    static_cast<const QXcbWindow *>(parent());
                sendXEmbedMessage(container->xcb_window(),
                                  focusEvent->reason() == Qt::TabFocusReason ?
                                  XEMBED_FOCUS_NEXT : XEMBED_FOCUS_PREV);
                event->accept();
                }
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
    QPlatformWindow::windowEvent(event);
}

bool QXcbWindow::startSystemResize(const QPoint &pos, Qt::Corner corner)
{
    const xcb_atom_t moveResize = connection()->atom(QXcbAtom::_NET_WM_MOVERESIZE);
    if (!connection()->wmSupport()->isSupportedByWM(moveResize))
        return false;
    xcb_client_message_event_t xev;
    xev.response_type = XCB_CLIENT_MESSAGE;
    xev.type = moveResize;
    xev.sequence = 0;
    xev.window = xcb_window();
    xev.format = 32;
    const QPoint globalPos = window()->mapToGlobal(pos);
    xev.data.data32[0] = globalPos.x();
    xev.data.data32[1] = globalPos.y();
    const bool bottom = corner == Qt::BottomRightCorner || corner == Qt::BottomLeftCorner;
    const bool left = corner == Qt::BottomLeftCorner || corner == Qt::TopLeftCorner;
    if (bottom)
        xev.data.data32[2] = left ? 6 : 4; // bottomleft/bottomright
    else
        xev.data.data32[2] = left ? 0 : 2; // topleft/topright
    xev.data.data32[3] = XCB_BUTTON_INDEX_1;
    xev.data.data32[4] = 0;
    xcb_ungrab_pointer(connection()->xcb_connection(), XCB_CURRENT_TIME);
    xcb_send_event(connection()->xcb_connection(), false, xcbScreen()->root(),
                   XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
                   (const char *)&xev);
    return true;
}

// Sends an XEmbed message.
void QXcbWindow::sendXEmbedMessage(xcb_window_t window, quint32 message,
                                   quint32 detail, quint32 data1, quint32 data2)
{
    xcb_client_message_event_t event;

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.sequence = 0;
    event.window = window;
    event.type = atom(QXcbAtom::_XEMBED);
    event.data.data32[0] = connection()->time();
    event.data.data32[1] = message;
    event.data.data32[2] = detail;
    event.data.data32[3] = data1;
    event.data.data32[4] = data2;
    Q_XCB_CALL(xcb_send_event(xcb_connection(), false, window,
                              XCB_EVENT_MASK_NO_EVENT, (const char *)&event));
}

static bool activeWindowChangeQueued(const QWindow *window)
{
    /* Check from window system event queue if the next queued activation
     * targets a window other than @window.
     */
    QWindowSystemInterfacePrivate::ActivatedWindowEvent *systemEvent =
        static_cast<QWindowSystemInterfacePrivate::ActivatedWindowEvent *>
        (QWindowSystemInterfacePrivate::peekWindowSystemEvent(QWindowSystemInterfacePrivate::ActivatedWindow));
    return systemEvent && systemEvent->activated != window;
}

void QXcbWindow::handleXEmbedMessage(const xcb_client_message_event_t *event)
{
    connection()->setTime(event->data.data32[0]);
    switch (event->data.data32[1]) {
    case XEMBED_WINDOW_ACTIVATE:
    case XEMBED_WINDOW_DEACTIVATE:
        break;
    case XEMBED_EMBEDDED_NOTIFY:
        Q_XCB_CALL(xcb_map_window(xcb_connection(), m_window));
        xcbScreen()->windowShown(this);
        // Without Qt::WA_TranslucentBackground, we use a ParentRelative BackPixmap.
        // Clear the whole tray icon window to its background color as early as possible
        // so that we can get a clean result from grabWindow() later.
        Q_XCB_CALL(xcb_clear_area(xcb_connection(), false, m_window, 0, 0, geometry().width(), geometry().height()));
        xcb_flush(xcb_connection());
        break;
    case XEMBED_FOCUS_IN:
        Qt::FocusReason reason;
        switch (event->data.data32[2]) {
        case XEMBED_FOCUS_FIRST:
            reason = Qt::TabFocusReason;
            break;
        case XEMBED_FOCUS_LAST:
            reason = Qt::BacktabFocusReason;
            break;
        case XEMBED_FOCUS_CURRENT:
        default:
            reason = Qt::OtherFocusReason;
            break;
        }
        connection()->setFocusWindow(static_cast<QXcbWindow*>(window()->handle()));
        QWindowSystemInterface::handleWindowActivated(window(), reason);
        break;
    case XEMBED_FOCUS_OUT:
        if (window() == QGuiApplication::focusWindow()
            && !activeWindowChangeQueued(window())) {
            connection()->setFocusWindow(0);
            QWindowSystemInterface::handleWindowActivated(0);
        }
        break;
    }
}

static inline xcb_rectangle_t qRectToXCBRectangle(const QRect &r)
{
    xcb_rectangle_t result;
    result.x = qMax(SHRT_MIN, r.x());
    result.y = qMax(SHRT_MIN, r.y());
    result.width = qMin((int)USHRT_MAX, r.width());
    result.height = qMin((int)USHRT_MAX, r.height());
    return result;
}

void QXcbWindow::setOpacity(qreal level)
{
    if (!m_window)
        return;

    quint32 value = qRound64(qBound(qreal(0), level, qreal(1)) * 0xffffffff);

    Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                   XCB_PROP_MODE_REPLACE,
                                   m_window,
                                   atom(QXcbAtom::_NET_WM_WINDOW_OPACITY),
                                   XCB_ATOM_CARDINAL,
                                   32,
                                   1,
                                   (uchar *)&value));
}

void QXcbWindow::setMask(const QRegion &region)
{
    if (!connection()->hasXShape())
        return;
    if (region.isEmpty()) {
        xcb_shape_mask(connection()->xcb_connection(), XCB_SHAPE_SO_SET,
                       XCB_SHAPE_SK_BOUNDING, xcb_window(), 0, 0, XCB_NONE);
    } else {
        QVector<xcb_rectangle_t> rects;
        rects.reserve(region.rectCount());
        for (const QRect &r : region)
            rects.push_back(qRectToXCBRectangle(r));
        xcb_shape_rectangles(connection()->xcb_connection(), XCB_SHAPE_SO_SET,
                             XCB_SHAPE_SK_BOUNDING, XCB_CLIP_ORDERING_UNSORTED,
                             xcb_window(), 0, 0, rects.size(), &rects[0]);
    }
}

void QXcbWindow::setAlertState(bool enabled)
{
    if (m_alertState == enabled)
        return;

    m_alertState = enabled;

    changeNetWmState(enabled, atom(QXcbAtom::_NET_WM_STATE_DEMANDS_ATTENTION));
}

uint QXcbWindow::visualId() const
{
    return m_visualId;
}

bool QXcbWindow::needsSync() const
{
    return m_syncState == SyncAndConfigureReceived;
}

void QXcbWindow::postSyncWindowRequest()
{
    if (!m_pendingSyncRequest) {
        QXcbSyncWindowRequest *e = new QXcbSyncWindowRequest(this);
        m_pendingSyncRequest = e;
        QCoreApplication::postEvent(xcbScreen()->connection(), e);
    }
}

QXcbScreen *QXcbWindow::xcbScreen() const
{
    return static_cast<QXcbScreen *>(screen());
}

QT_END_NAMESPACE

