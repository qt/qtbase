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

#include "qxcbwindow.h"

#include <QtDebug>

#include "qxcbconnection.h"
#include "qxcbscreen.h"
#ifdef XCB_USE_DRI2
#include "qdri2context.h"
#endif

#include <xcb/xcb_icccm.h>

#include <private/qguiapplication_p.h>
#include <private/qwindowsurface_p.h>

#include <QtGui/QWindowSystemInterface>

#include <stdio.h>

#ifdef XCB_USE_XLIB
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#if defined(XCB_USE_GLX)
#include "qglxintegration.h"
#include "qglxconvenience.h"
#elif defined(XCB_USE_EGL)
#include "../eglconvenience/qeglplatformcontext.h"
#include "../eglconvenience/qeglconvenience.h"
#include "../eglconvenience/qxlibeglintegration.h"
#endif

#if 0
// Returns true if we should set WM_TRANSIENT_FOR on \a w
static inline bool isTransient(const QWidget *w)
{
    return ((w->windowType() == Qt::Dialog
             || w->windowType() == Qt::Sheet
             || w->windowType() == Qt::Tool
             || w->windowType() == Qt::SplashScreen
             || w->windowType() == Qt::ToolTip
             || w->windowType() == Qt::Drawer
             || w->windowType() == Qt::Popup)
            && !w->testAttribute(Qt::WA_X11BypassTransientForHint));
}
#endif

QXcbWindow::QXcbWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_context(0)
    , m_windowState(Qt::WindowNoState)
{
    m_screen = static_cast<QXcbScreen *>(QGuiApplicationPrivate::platformIntegration()->screens().at(0));

    setConnection(m_screen->connection());

    const quint32 mask = XCB_CW_BACK_PIXMAP | XCB_CW_EVENT_MASK;
    const quint32 values[] = {
        // XCB_CW_BACK_PIXMAP
        XCB_NONE,
        // XCB_CW_EVENT_MASK
        XCB_EVENT_MASK_EXPOSURE
        | XCB_EVENT_MASK_STRUCTURE_NOTIFY
        | XCB_EVENT_MASK_KEY_PRESS
        | XCB_EVENT_MASK_KEY_RELEASE
        | XCB_EVENT_MASK_BUTTON_PRESS
        | XCB_EVENT_MASK_BUTTON_RELEASE
        | XCB_EVENT_MASK_BUTTON_MOTION
        | XCB_EVENT_MASK_ENTER_WINDOW
        | XCB_EVENT_MASK_LEAVE_WINDOW
        | XCB_EVENT_MASK_POINTER_MOTION
        | XCB_EVENT_MASK_PROPERTY_CHANGE
        | XCB_EVENT_MASK_FOCUS_CHANGE
    };

    QRect rect = window->geometry();

    xcb_window_t xcb_parent_id = m_screen->root();
    if (window->parent() && window->parent()->handle())
        xcb_parent_id = static_cast<QXcbWindow *>(window->parent()->handle())->xcb_window();

#if defined(XCB_USE_GLX) || defined(XCB_USE_EGL)
    if (window->surfaceType() == QWindow::OpenGLSurface
        && QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
    {
#if defined(XCB_USE_GLX)
        XVisualInfo *visualInfo = qglx_findVisualInfo(DISPLAY_FROM_XCB(m_screen),m_screen->screenNumber(), window->requestedWindowFormat());
#elif defined(XCB_USE_EGL)
        EGLDisplay eglDisplay = connection()->egl_display();
        EGLConfig eglConfig = q_configFromQPlatformWindowFormat(eglDisplay,window->requestedWindowFormat(),true);
        VisualID id = QXlibEglIntegration::getCompatibleVisualId(DISPLAY_FROM_XCB(this), eglDisplay, eglConfig);

        XVisualInfo visualInfoTemplate;
        memset(&visualInfoTemplate, 0, sizeof(XVisualInfo));
        visualInfoTemplate.visualid = id;

        XVisualInfo *visualInfo;
        int matchingCount = 0;
        visualInfo = XGetVisualInfo(DISPLAY_FROM_XCB(this), VisualIDMask, &visualInfoTemplate, &matchingCount);
#endif //XCB_USE_GLX
        if (visualInfo) {
            Colormap cmap = XCreateColormap(DISPLAY_FROM_XCB(this), xcb_parent_id, visualInfo->visual, AllocNone);

            XSetWindowAttributes a;
            a.colormap = cmap;
            m_window = XCreateWindow(DISPLAY_FROM_XCB(this), xcb_parent_id, rect.x(), rect.y(), rect.width(), rect.height(),
                                      0, visualInfo->depth, InputOutput, visualInfo->visual,
                                      CWColormap, &a);

            printf("created GL window: %d\n", m_window);
        } else {
            qFatal("no window!");
        }
    } else
#endif //defined(XCB_USE_GLX) || defined(XCB_USE_EGL)
    {
        m_window = xcb_generate_id(xcb_connection());

        Q_XCB_CALL(xcb_create_window(xcb_connection(),
                                     XCB_COPY_FROM_PARENT,            // depth -- same as root
                                     m_window,                        // window id
                                     xcb_parent_id,                   // parent window id
                                     rect.x(),
                                     rect.y(),
                                     rect.width(),
                                     rect.height(),
                                     0,                               // border width
                                     XCB_WINDOW_CLASS_INPUT_OUTPUT,   // window class
                                     m_screen->screen()->root_visual, // visual
                                     0,                               // value mask
                                     0));                             // value list

        printf("created regular window: %d\n", m_window);
    }

    connection()->addWindow(m_window, this);

    Q_XCB_CALL(xcb_change_window_attributes(xcb_connection(), m_window, mask, values));

    xcb_atom_t properties[4];
    int propertyCount = 0;
    properties[propertyCount++] = atom(QXcbAtom::WM_DELETE_WINDOW);
    properties[propertyCount++] = atom(QXcbAtom::WM_TAKE_FOCUS);
    properties[propertyCount++] = atom(QXcbAtom::_NET_WM_PING);

    if (m_screen->syncRequestSupported())
        properties[propertyCount++] = atom(QXcbAtom::_NET_WM_SYNC_REQUEST);

    if (window->windowFlags() & Qt::WindowContextHelpButtonHint)
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

    if (m_screen->syncRequestSupported()) {
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

#if 0
    if (tlw && isTransient(tlw) && tlw->parentWidget()) {
        // ICCCM 4.1.2.6
        QWidget *p = tlw->parentWidget()->window();
        xcb_window_t parentWindow = p->winId();
        Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                       XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 32,
                                       1, &parentWindow));

    }
#endif

    // set the PID to let the WM kill the application if unresponsive
    long pid = getpid();
    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::_NET_WM_PID), XCB_ATOM_CARDINAL, 32,
                                   1, &pid));
}

QXcbWindow::~QXcbWindow()
{
    delete m_context;
    if (m_screen->syncRequestSupported())
        Q_XCB_CALL(xcb_sync_destroy_counter(xcb_connection(), m_syncCounter));
    connection()->removeWindow(m_window);
    Q_XCB_CALL(xcb_destroy_window(xcb_connection(), m_window));
}

void QXcbWindow::setGeometry(const QRect &rect)
{
    QPlatformWindow::setGeometry(rect);

    const quint32 mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
    const quint32 values[] = { rect.x(), rect.y(), rect.width(), rect.height() };

    Q_XCB_CALL(xcb_configure_window(xcb_connection(), m_window, mask, values));
}

void QXcbWindow::setVisible(bool visible)
{
    xcb_wm_hints_t hints;
    if (visible) {
        // TODO: QWindow::isMinimized() or similar
        if (window()->windowState() & Qt::WindowMinimized)
            xcb_wm_hints_set_iconic(&hints);
        else
            xcb_wm_hints_set_normal(&hints);
        xcb_set_wm_hints(xcb_connection(), m_window, &hints);
        Q_XCB_CALL(xcb_map_window(xcb_connection(), m_window));
        connection()->sync();
    } else {
        Q_XCB_CALL(xcb_unmap_window(xcb_connection(), m_window));

        // send synthetic UnmapNotify event according to icccm 4.1.4
        xcb_unmap_notify_event_t event;
        event.response_type = XCB_UNMAP_NOTIFY;
        event.sequence = 0; // does this matter?
        event.event = m_screen->root();
        event.window = m_window;
        event.from_configure = false;
        Q_XCB_CALL(xcb_send_event(xcb_connection(), false, m_screen->root(),
                                  XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));

        xcb_flush(xcb_connection());
    }
}

struct QtMWMHints {
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

Qt::WindowFlags QXcbWindow::setWindowFlags(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    setNetWmWindowTypes(flags);

    if (type == Qt::ToolTip)
        flags |= Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint;
    if (type == Qt::Popup)
        flags |= Qt::X11BypassWindowManagerHint;

    bool topLevel = (flags & Qt::Window);
    bool popup = (type == Qt::Popup);
    bool dialog = (type == Qt::Dialog
                   || type == Qt::Sheet);
    bool desktop = (type == Qt::Desktop);
    bool tool = (type == Qt::Tool || type == Qt::SplashScreen
                 || type == Qt::ToolTip || type == Qt::Drawer);

    Q_UNUSED(topLevel);
    Q_UNUSED(dialog);
    Q_UNUSED(desktop);
    Q_UNUSED(tool);

    bool tooltip = (type == Qt::ToolTip);

    QtMWMHints mwmhints;
    mwmhints.flags = 0L;
    mwmhints.functions = 0L;
    mwmhints.decorations = 0;
    mwmhints.input_mode = 0L;
    mwmhints.status = 0L;

    if (type != Qt::SplashScreen) {
        mwmhints.flags |= MWM_HINTS_DECORATIONS;

        bool customize = flags & Qt::CustomizeWindowHint;
        if (!(flags & Qt::FramelessWindowHint) && !(customize && !(flags & Qt::WindowTitleHint))) {
            mwmhints.decorations |= MWM_DECOR_BORDER;
            mwmhints.decorations |= MWM_DECOR_RESIZEH;

            if (flags & Qt::WindowTitleHint)
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

    if (mwmhints.flags != 0l) {
        Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                       XCB_PROP_MODE_REPLACE,
                                       m_window,
                                       atom(QXcbAtom::_MOTIF_WM_HINTS),
                                       atom(QXcbAtom::_MOTIF_WM_HINTS),
                                       32,
                                       5,
                                       &mwmhints));
    } else {
        Q_XCB_CALL(xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::_MOTIF_WM_HINTS)));
    }

    if (popup || tooltip) {
        const quint32 mask = XCB_CW_OVERRIDE_REDIRECT | XCB_CW_SAVE_UNDER;
        const quint32 values[] = { true, true };

        Q_XCB_CALL(xcb_change_window_attributes(xcb_connection(), m_window, mask, values));
    }

    return flags;
}

void QXcbWindow::changeNetWmState(bool set, xcb_atom_t one, xcb_atom_t two)
{
    xcb_client_message_event_t event;

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.window = m_window;
    event.type = atom(QXcbAtom::_NET_WM_STATE);
    event.data.data32[0] = set ? 1 : 0;
    event.data.data32[1] = one;
    event.data.data32[2] = two;
    event.data.data32[3] = 0;
    event.data.data32[4] = 0;

    Q_XCB_CALL(xcb_send_event(xcb_connection(), 0, m_screen->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));
}

Qt::WindowState QXcbWindow::setWindowState(Qt::WindowState state)
{
    if (state == m_windowState)
        return state;

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
            event.window = m_window;
            event.type = atom(QXcbAtom::WM_CHANGE_STATE);
            event.data.data32[0] = XCB_WM_STATE_ICONIC;
            event.data.data32[1] = 0;
            event.data.data32[2] = 0;
            event.data.data32[3] = 0;
            event.data.data32[4] = 0;

            Q_XCB_CALL(xcb_send_event(xcb_connection(), 0, m_screen->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event));
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
    return m_windowState;
}

void QXcbWindow::setNetWmWindowTypes(Qt::WindowFlags flags)
{
    // in order of decreasing priority
    QVector<uint> windowTypes;

    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    switch (type) {
    case Qt::Dialog:
    case Qt::Sheet:
        windowTypes.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_DIALOG));
        break;
    case Qt::Tool:
    case Qt::Drawer:
        windowTypes.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_UTILITY));
        break;
    case Qt::ToolTip:
        windowTypes.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_TOOLTIP));
        break;
    case Qt::SplashScreen:
        windowTypes.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_SPLASH));
        break;
    default:
        break;
    }

    if (flags & Qt::FramelessWindowHint)
        windowTypes.append(atom(QXcbAtom::_KDE_NET_WM_WINDOW_TYPE_OVERRIDE));

    windowTypes.append(atom(QXcbAtom::_NET_WM_WINDOW_TYPE_NORMAL));

    Q_XCB_CALL(xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                   atom(QXcbAtom::_NET_WM_WINDOW_TYPE), XCB_ATOM_ATOM, 32,
                                   windowTypes.count(), windowTypes.constData()));
}

WId QXcbWindow::winId() const
{
    return m_window;
}

void QXcbWindow::setParent(const QPlatformWindow *parent)
{
    QPoint topLeft = geometry().topLeft();
    Q_XCB_CALL(xcb_reparent_window(xcb_connection(), xcb_window(), static_cast<const QXcbWindow *>(parent)->xcb_window(), topLeft.x(), topLeft.y()));
}

void QXcbWindow::setWindowTitle(const QString &title)
{
    QByteArray ba = title.toUtf8();
    Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                   XCB_PROP_MODE_REPLACE,
                                   m_window,
                                   atom(QXcbAtom::_NET_WM_NAME),
                                   atom(QXcbAtom::UTF8_STRING),
                                   8,
                                   ba.length(),
                                   ba.constData()));
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

void QXcbWindow::requestActivateWindow()
{
    Q_XCB_CALL(xcb_set_input_focus(xcb_connection(), XCB_INPUT_FOCUS_PARENT, m_window, XCB_TIME_CURRENT_TIME));
    connection()->sync();
}

QPlatformGLContext *QXcbWindow::glContext() const
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL)) {
        printf("no opengl\n");
        return 0;
    }
    if (!m_context) {
#if defined(XCB_USE_GLX)
        QXcbWindow *that = const_cast<QXcbWindow *>(this);
        that->m_context = new QGLXContext(m_window, m_screen, window()->requestedWindowFormat());
#elif defined(XCB_USE_EGL)
        EGLDisplay display = connection()->egl_display();
        EGLConfig config = q_configFromQPlatformWindowFormat(display,window()->requestedWindowFormat(),true);
        QVector<EGLint> eglContextAttrs;
        eglContextAttrs.append(EGL_CONTEXT_CLIENT_VERSION);
        eglContextAttrs.append(2);
        eglContextAttrs.append(EGL_NONE);

        EGLSurface eglSurface = eglCreateWindowSurface(display,config,(EGLNativeWindowType)m_window,0);
        QXcbWindow *that = const_cast<QXcbWindow *>(this);
        that->m_context = new QEGLPlatformContext(display, config, eglContextAttrs.data(), eglSurface, EGL_OPENGL_ES_API);
#elif defined(XCB_USE_DRI2)
        QXcbWindow *that = const_cast<QXcbWindow *>(this);
        that->m_context = new QDri2Context(that);
#endif
    }
    return m_context;
}

void QXcbWindow::handleExposeEvent(const xcb_expose_event_t *event)
{
    QWindowSurface *surface = window()->surface();
    if (surface) {
        QRect rect(event->x, event->y, event->width, event->height);

        surface->flush(window(), rect, QPoint());
    }
}

void QXcbWindow::handleClientMessageEvent(const xcb_client_message_event_t *event)
{
    if (event->format == 32 && event->type == atom(QXcbAtom::WM_PROTOCOLS)) {
        if (event->data.data32[0] == atom(QXcbAtom::WM_DELETE_WINDOW)) {
            QWindowSystemInterface::handleCloseEvent(window());
        } else if (event->data.data32[0] == atom(QXcbAtom::_NET_WM_PING)) {
            xcb_client_message_event_t reply = *event;

            reply.response_type = XCB_CLIENT_MESSAGE;
            reply.window = m_screen->root();

            xcb_send_event(xcb_connection(), 0, m_screen->root(), XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&reply);
            xcb_flush(xcb_connection());
        } else if (event->data.data32[0] == atom(QXcbAtom::_NET_WM_SYNC_REQUEST)) {
            if (!m_hasReceivedSyncRequest) {
                m_hasReceivedSyncRequest = true;
                printf("Window manager supports _NET_WM_SYNC_REQUEST, syncing resizes\n");
            }
            m_syncValue.lo = event->data.data32[2];
            m_syncValue.hi = event->data.data32[3];
        }
    }
}

void QXcbWindow::handleConfigureNotifyEvent(const xcb_configure_notify_event_t *event)
{
    int xpos = geometry().x();
    int ypos = geometry().y();

    if ((event->width == geometry().width() && event->height == geometry().height()) || event->x != 0 || event->y != 0) {
        xpos = event->x;
        ypos = event->y;
    }

    QRect rect(xpos, ypos, event->width, event->height);

    if (rect == geometry())
        return;

    QPlatformWindow::setGeometry(rect);
    QWindowSystemInterface::handleGeometryChange(window(), rect);

#if XCB_USE_DRI2
    if (m_context)
        static_cast<QDri2Context *>(m_context)->resize(rect.size());
#endif
}

static Qt::MouseButtons translateMouseButtons(int s)
{
    Qt::MouseButtons ret = 0;
    if (s & XCB_BUTTON_MASK_1)
        ret |= Qt::LeftButton;
    if (s & XCB_BUTTON_MASK_2)
        ret |= Qt::MidButton;
    if (s & XCB_BUTTON_MASK_3)
        ret |= Qt::RightButton;
    return ret;
}

static Qt::MouseButton translateMouseButton(xcb_button_t s)
{
    switch (s) {
    case 1:
        return Qt::LeftButton;
    case 2:
        return Qt::MidButton;
    case 3:
        return Qt::RightButton;
    default:
        return Qt::NoButton;
    }
}

void QXcbWindow::handleButtonPressEvent(const xcb_button_press_event_t *event)
{
    QPoint local(event->event_x, event->event_y);
    QPoint global(event->root_x, event->root_y);

    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    if (event->detail >= 4 && event->detail <= 7) {
        //logic borrowed from qapplication_x11.cpp
        int delta = 120 * ((event->detail == 4 || event->detail == 6) ? 1 : -1);
        bool hor = (((event->detail == 4 || event->detail == 5)
                     && (modifiers & Qt::AltModifier))
                    || (event->detail == 6 || event->detail == 7));

        QWindowSystemInterface::handleWheelEvent(window(), event->time,
                                                 local, global, delta, hor ? Qt::Horizontal : Qt::Vertical);
        return;
    }

    handleMouseEvent(event->detail, event->state, event->time, local, global);
}

void QXcbWindow::handleButtonReleaseEvent(const xcb_button_release_event_t *event)
{
    QPoint local(event->event_x, event->event_y);
    QPoint global(event->root_x, event->root_y);

    handleMouseEvent(event->detail, event->state, event->time, local, global);
}

void QXcbWindow::handleMotionNotifyEvent(const xcb_motion_notify_event_t *event)
{
    QPoint local(event->event_x, event->event_y);
    QPoint global(event->root_x, event->root_y);

    handleMouseEvent(event->detail, event->state, event->time, local, global);
}

void QXcbWindow::handleMouseEvent(xcb_button_t detail, uint16_t state, xcb_timestamp_t time, const QPoint &local, const QPoint &global)
{
    Qt::MouseButtons buttons = translateMouseButtons(state);
    Qt::MouseButton button = translateMouseButton(detail);

    buttons ^= button; // X event uses state *before*, Qt uses state *after*

    QWindowSystemInterface::handleMouseEvent(window(), time, local, global, buttons);
}

void QXcbWindow::handleEnterNotifyEvent(const xcb_enter_notify_event_t *event)
{
    if ((event->mode != XCB_NOTIFY_MODE_NORMAL && event->mode != XCB_NOTIFY_MODE_UNGRAB)
        || event->detail == XCB_NOTIFY_DETAIL_VIRTUAL
        || event->detail == XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL)
    {
        return;
    }

    QWindowSystemInterface::handleEnterEvent(window());
}

void QXcbWindow::handleLeaveNotifyEvent(const xcb_leave_notify_event_t *event)
{
    if ((event->mode != XCB_NOTIFY_MODE_NORMAL && event->mode != XCB_NOTIFY_MODE_UNGRAB)
        || event->detail == XCB_NOTIFY_DETAIL_INFERIOR)
    {
        return;
    }

    QWindowSystemInterface::handleLeaveEvent(window());
}

void QXcbWindow::handleFocusInEvent(const xcb_focus_in_event_t *)
{
    QWindowSystemInterface::handleWindowActivated(window());
}

void QXcbWindow::handleFocusOutEvent(const xcb_focus_out_event_t *)
{
    QWindowSystemInterface::handleWindowActivated(0);
}

void QXcbWindow::updateSyncRequestCounter()
{
    if (m_screen->syncRequestSupported() && (m_syncValue.lo != 0 || m_syncValue.hi != 0)) {
        Q_XCB_CALL(xcb_sync_set_counter(xcb_connection(), m_syncCounter, m_syncValue));
        xcb_flush(xcb_connection());
        connection()->sync();

        m_syncValue.lo = 0;
        m_syncValue.hi = 0;
    }
}
