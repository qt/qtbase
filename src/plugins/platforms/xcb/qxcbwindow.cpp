// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbwindow.h"

#include <QtDebug>
#include <QMetaEnum>
#include <QScreen>
#include <QtCore/QFileInfo>
#include <QtGui/QIcon>
#include <QtGui/QRegion>
#include <QtGui/private/qhighdpiscaling_p.h>

#include "qxcbintegration.h"
#include "qxcbconnection.h"
#include "qxcbscreen.h"
#if QT_CONFIG(draganddrop)
#include "qxcbdrag.h"
#endif
#include "qxcbkeyboard.h"
#include "qxcbimage.h"
#include "qxcbwmsupport.h"
#include "qxcbimage.h"
#include "qxcbnativeinterface.h"
#include "qxcbsystemtraytracker.h"

#include <qpa/qplatformintegration.h>
#include <qpa/qplatformcursor.h>

#include <algorithm>

#include <xcb/xcb_icccm.h>
#include <xcb/xfixes.h>
#include <xcb/shape.h>
#include <xcb/xinput.h>

#include <private/qguiapplication_p.h>
#include <private/qwindow_p.h>

#include <qpa/qplatformbackingstore.h>
#include <qpa/qwindowsysteminterface.h>

#include <stdio.h>

#if QT_CONFIG(xcb_xlib)
#define register        /* C++17 deprecated register */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef register
#endif

#define XCOORD_MAX 32767
enum {
    defaultWindowWidth = 160,
    defaultWindowHeight = 160
};

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcQpaWindow, "qt.qpa.window");

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

QXcbScreen *QXcbWindow::initialScreen() const
{
    // Resolve initial screen via QWindowPrivate::screenForGeometry(),
    // which works in platform independent coordinates, as opposed to
    // QPlatformWindow::screenForGeometry() that uses native coordinates.
    QWindowPrivate *windowPrivate = qt_window_private(window());
    QScreen *screen = windowPrivate->screenForGeometry(window()->geometry());
    return static_cast<QXcbScreen*>(screen->handle());
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

void QXcbWindow::setImageFormatForVisual(const xcb_visualtype_t *visual)
{
    if (qt_xcb_imageFormatForVisual(connection(), m_depth, visual, &m_imageFormat, &m_imageRgbSwap))
        return;

    switch (m_depth) {
    case 32:
    case 24:
        qWarning("Using RGB32 fallback, if this works your X11 server is reporting a bad screen format.");
        m_imageFormat = QImage::Format_RGB32;
        break;
    case 16:
        qWarning("Using RGB16 fallback, if this works your X11 server is reporting a bad screen format.");
        m_imageFormat = QImage::Format_RGB16;
    default:
        break;
    }
}

#if QT_CONFIG(xcb_xlib)
static inline XTextProperty* qstringToXTP(Display *dpy, const QString& s)
{
    #include <X11/Xatom.h>

    static XTextProperty tp = { nullptr, 0, 0, 0 };
    static bool free_prop = true; // we can't free tp.value in case it references
                                  // the data of the static QByteArray below.
    if (tp.value) {
        if (free_prop)
            XFree(tp.value);
        tp.value = nullptr;
        free_prop = true;
    }

    int errCode = 0;
    QByteArray mapped = s.toLocal8Bit(); // should always be utf-8
    char* tl[2];
    tl[0] = mapped.data();
    tl[1] = nullptr;
    errCode = XmbTextListToTextProperty(dpy, tl, 1, XStdICCTextStyle, &tp);
    if (errCode < 0)
        qCDebug(lcQpaXcb, "XmbTextListToTextProperty result code %d", errCode);

    if (errCode < 0) {
        static QByteArray qcs;
        qcs = s.toLatin1();
        tp.value = (uchar*)qcs.data();
        tp.encoding = XA_STRING;
        tp.format = 8;
        tp.nitems = qcs.size();
        free_prop = false;
    }
    return &tp;
}
#endif // QT_CONFIG(xcb_xlib)

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
    return nullptr;
}

static const char *wm_window_type_property_id = "_q_xcb_wm_window_type";
static const char *wm_window_role_property_id = "_q_xcb_wm_window_role";

QXcbWindow::QXcbWindow(QWindow *window)
    : QPlatformWindow(window)
{
    setConnection(xcbScreen()->connection());
}

enum : quint32 {
    baseEventMask
        = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_STRUCTURE_NOTIFY
            | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_FOCUS_CHANGE,

    defaultEventMask = baseEventMask
            | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
            | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
            | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW
            | XCB_EVENT_MASK_POINTER_MOTION,

    transparentForInputEventMask = baseEventMask
            | XCB_EVENT_MASK_VISIBILITY_CHANGE
            | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
            | XCB_EVENT_MASK_COLOR_MAP_CHANGE | XCB_EVENT_MASK_OWNER_GRAB_BUTTON
};

void QXcbWindow::create()
{
    destroy();

    m_windowState = Qt::WindowNoState;
    m_trayIconWindow = isTrayIconWindow(window());

    Qt::WindowType type = window()->type();

    QXcbScreen *currentScreen = xcbScreen();
    QXcbScreen *platformScreen = parent() ? parentScreen() : initialScreen();
    QRect rect = parent()
        ? QHighDpi::toNativeLocalPosition(window()->geometry(), platformScreen)
        : QHighDpi::toNativePixels(window()->geometry(), platformScreen);

    if (type == Qt::Desktop) {
        m_window = platformScreen->root();
        m_depth = platformScreen->screen()->root_depth;
        m_visualId = platformScreen->screen()->root_visual;
        const xcb_visualtype_t *visual = nullptr;
        if (connection()->hasDefaultVisualId()) {
            visual = platformScreen->visualForId(connection()->defaultVisualId());
            if (visual)
                m_visualId = connection()->defaultVisualId();
            if (!visual)
                qWarning("Could not use default visual id. Falling back to root_visual for screen.");
        }
        if (!visual)
            visual = platformScreen->visualForId(m_visualId);
        setImageFormatForVisual(visual);
        connection()->addWindowEventListener(m_window, this);
        return;
    }

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

    QPlatformWindow::setGeometry(rect);

    if (platformScreen != currentScreen)
        QWindowSystemInterface::handleWindowScreenChanged(window(), platformScreen->QPlatformScreen::screen());

    xcb_window_t xcb_parent_id = platformScreen->root();
    if (parent()) {
        xcb_parent_id = static_cast<QXcbWindow *>(parent())->xcb_window();
        m_embedded = parent()->isForeignWindow();

        QSurfaceFormat parentFormat = parent()->window()->requestedFormat();
        if (window()->surfaceType() != QSurface::OpenGLSurface && parentFormat.hasAlpha()) {
            window()->setFormat(parentFormat);
        }
    }

    resolveFormat(platformScreen->surfaceFormatFor(window()->requestedFormat()));

    const xcb_visualtype_t *visual = nullptr;

    if (m_trayIconWindow && connection()->systemTrayTracker()) {
        visual = platformScreen->visualForId(connection()->systemTrayTracker()->visualId());
    } else if (connection()->hasDefaultVisualId()) {
        visual = platformScreen->visualForId(connection()->defaultVisualId());
        if (!visual)
            qWarning() << "Failed to use requested visual id.";
    }

    if (parent()) {
        // When using a Vulkan QWindow via QWidget::createWindowContainer() we
        // must make sure the visuals are compatible. Now, the parent will be
        // of RasterGLSurface which typically chooses a GLX/EGL compatible
        // visual which may not be what the Vulkan window would choose.
        // Therefore, take the parent's visual.
        if (window()->surfaceType() == QSurface::VulkanSurface
                && parent()->window()->surfaceType() != QSurface::VulkanSurface)
        {
            visual = platformScreen->visualForId(static_cast<QXcbWindow *>(parent())->visualId());
        }
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
    setImageFormatForVisual(visual);

    quint32 mask = XCB_CW_BACK_PIXMAP
                 | XCB_CW_BORDER_PIXEL
                 | XCB_CW_BIT_GRAVITY
                 | XCB_CW_OVERRIDE_REDIRECT
                 | XCB_CW_SAVE_UNDER
                 | XCB_CW_EVENT_MASK
                 | XCB_CW_COLORMAP;

    quint32 values[] = {
        XCB_BACK_PIXMAP_NONE,
        platformScreen->screen()->black_pixel,
        XCB_GRAVITY_NORTH_WEST,
        type == Qt::Popup || type == Qt::ToolTip || (window()->flags() & Qt::BypassWindowManagerHint),
        type == Qt::Popup || type == Qt::Tool || type == Qt::SplashScreen || type == Qt::ToolTip || type == Qt::Drawer,
        defaultEventMask,
        platformScreen->colormapForVisual(m_visualId)
    };

    m_window = xcb_generate_id(xcb_connection());
    xcb_create_window(xcb_connection(),
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
                      values);

    connection()->addWindowEventListener(m_window, this);

    propagateSizeHints();

    xcb_atom_t properties[5];
    int propertyCount = 0;
    properties[propertyCount++] = atom(QXcbAtom::AtomWM_DELETE_WINDOW);
    properties[propertyCount++] = atom(QXcbAtom::AtomWM_TAKE_FOCUS);
    properties[propertyCount++] = atom(QXcbAtom::Atom_NET_WM_PING);

    if (connection()->hasXSync())
        properties[propertyCount++] = atom(QXcbAtom::Atom_NET_WM_SYNC_REQUEST);

    if (window()->flags() & Qt::WindowContextHelpButtonHint)
        properties[propertyCount++] = atom(QXcbAtom::Atom_NET_WM_CONTEXT_HELP);

    xcb_change_property(xcb_connection(),
                        XCB_PROP_MODE_REPLACE,
                        m_window,
                        atom(QXcbAtom::AtomWM_PROTOCOLS),
                        XCB_ATOM_ATOM,
                        32,
                        propertyCount,
                        properties);
    m_syncValue.hi = 0;
    m_syncValue.lo = 0;

    const QByteArray wmClass = QXcbIntegration::instance()->wmClass();
    if (!wmClass.isEmpty()) {
        xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE,
                            m_window, atom(QXcbAtom::AtomWM_CLASS),
                            XCB_ATOM_STRING, 8, wmClass.size(), wmClass.constData());
    }

    QString desktopFileName = QGuiApplication::desktopFileName();
    if (QGuiApplication::desktopFileName().isEmpty()) {
        QFileInfo fi = QFileInfo(QCoreApplication::instance()->applicationFilePath());
        QStringList domainName =
                QCoreApplication::instance()->organizationDomain().split(QLatin1Char('.'),
                                                                         Qt::SkipEmptyParts);

        if (domainName.isEmpty()) {
            desktopFileName = fi.baseName();
        } else {
            for (int i = 0; i < domainName.size(); ++i)
                desktopFileName.prepend(QLatin1Char('.')).prepend(domainName.at(i));
            desktopFileName.append(fi.baseName());
        }
    }
    if (!desktopFileName.isEmpty()) {
        const QByteArray dfName = desktopFileName.toUtf8();
        xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE,
                            m_window, atom(QXcbAtom::Atom_KDE_NET_WM_DESKTOP_FILE),
                            atom(QXcbAtom::AtomUTF8_STRING), 8, dfName.size(), dfName.constData());
        xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE,
                            m_window, atom(QXcbAtom::Atom_GTK_APPLICATION_ID),
                            atom(QXcbAtom::AtomUTF8_STRING), 8, dfName.size(), dfName.constData());
    }

    if (connection()->hasXSync()) {
        m_syncCounter = xcb_generate_id(xcb_connection());
        xcb_sync_create_counter(xcb_connection(), m_syncCounter, m_syncValue);

        xcb_change_property(xcb_connection(),
                            XCB_PROP_MODE_REPLACE,
                            m_window,
                            atom(QXcbAtom::Atom_NET_WM_SYNC_REQUEST_COUNTER),
                            XCB_ATOM_CARDINAL,
                            32,
                            1,
                            &m_syncCounter);
    }

    // set the PID to let the WM kill the application if unresponsive
    quint32 pid = getpid();
    xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                        atom(QXcbAtom::Atom_NET_WM_PID), XCB_ATOM_CARDINAL, 32,
                        1, &pid);

    const QByteArray clientMachine = QSysInfo::machineHostName().toLocal8Bit();
    if (!clientMachine.isEmpty()) {
        xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                            atom(QXcbAtom::AtomWM_CLIENT_MACHINE), XCB_ATOM_STRING, 8,
                            clientMachine.size(), clientMachine.constData());
    }

    // Create WM_HINTS property on the window, so we can xcb_icccm_get_wm_hints*()
    // from various setter functions for adjusting the hints.
    xcb_icccm_wm_hints_t hints;
    memset(&hints, 0, sizeof(hints));
    hints.flags = XCB_ICCCM_WM_HINT_WINDOW_GROUP;
    hints.window_group = connection()->clientLeader();
    xcb_icccm_set_wm_hints(xcb_connection(), m_window, &hints);

    xcb_window_t leader = connection()->clientLeader();
    xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                        atom(QXcbAtom::AtomWM_CLIENT_LEADER), XCB_ATOM_WINDOW, 32,
                        1, &leader);

    /* Add XEMBED info; this operation doesn't initiate the embedding. */
    quint32 data[] = { XEMBED_VERSION, XEMBED_MAPPED };
    xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                        atom(QXcbAtom::Atom_XEMBED_INFO),
                        atom(QXcbAtom::Atom_XEMBED_INFO),
                        32, 2, (void *)data);

    if (connection()->hasXInput2())
        connection()->xi2SelectDeviceEvents(m_window);

    setWindowState(window()->windowStates());
    setWindowFlags(window()->flags());
    setWindowTitle(window()->title());

    // force sync to read outstanding requests - see QTBUG-29106
    connection()->sync();

#if QT_CONFIG(draganddrop)
    connection()->drag()->dndEnable(this, true);
#endif

    const qreal opacity = qt_window_private(window())->opacity;
    if (!qFuzzyCompare(opacity, qreal(1.0)))
        setOpacity(opacity);

    setMask(QHighDpi::toNativeLocalRegion(window()->mask(), window()));

    if (window()->isTopLevel())
        setWindowIcon(window()->icon());

    if (window()->dynamicPropertyNames().contains(wm_window_role_property_id)) {
        QByteArray wmWindowRole = window()->property(wm_window_role_property_id).toByteArray();
        setWindowRole(QString::fromLatin1(wmWindowRole));
    }

    if (m_trayIconWindow)
        m_embedded = requestSystemTrayWindowDock();
}

QXcbWindow::~QXcbWindow()
{
    destroy();
}

QXcbForeignWindow::QXcbForeignWindow(QWindow *window, WId nativeHandle)
    : QXcbWindow(window)
{
    m_window = nativeHandle;

    // Reflect the foreign window's geometry as our own
    if (auto geometry = Q_XCB_REPLY(xcb_get_geometry, xcb_connection(), m_window)) {
        QRect nativeGeometry(geometry->x, geometry->y, geometry->width, geometry->height);
        QPlatformWindow::setGeometry(nativeGeometry);
    }
}

QXcbForeignWindow::~QXcbForeignWindow()
{
    // Clear window so that destroy() does not affect it
    m_window = 0;

    if (connection()->mouseGrabber() == this)
        connection()->setMouseGrabber(nullptr);
    if (connection()->mousePressWindow() == this)
        connection()->setMousePressWindow(nullptr);
}

void QXcbWindow::destroy()
{
    if (connection()->focusWindow() == this)
        doFocusOut();
    if (connection()->mouseGrabber() == this)
        connection()->setMouseGrabber(nullptr);

    if (m_syncCounter && connection()->hasXSync())
        xcb_sync_destroy_counter(xcb_connection(), m_syncCounter);
    if (m_window) {
        if (m_netWmUserTimeWindow) {
            xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::Atom_NET_WM_USER_TIME_WINDOW));
            // Some window managers, like metacity, do XSelectInput on the _NET_WM_USER_TIME_WINDOW window,
            // without trapping BadWindow (which crashes when the user time window is destroyed).
            connection()->sync();
            xcb_destroy_window(xcb_connection(), m_netWmUserTimeWindow);
            m_netWmUserTimeWindow = XCB_NONE;
        }
        connection()->removeWindowEventListener(m_window);
        xcb_destroy_window(xcb_connection(), m_window);
        m_window = 0;
    }

    m_mapped = false;
    m_recreationReasons = RecreationNotNeeded;

    if (m_pendingSyncRequest)
        m_pendingSyncRequest->invalidate();
}

void QXcbWindow::setGeometry(const QRect &rect)
{
    setWindowState(Qt::WindowNoState);

    QPlatformWindow::setGeometry(rect);

    propagateSizeHints();

    QXcbScreen *currentScreen = xcbScreen();
    QXcbScreen *newScreen = parent() ? parentScreen() : static_cast<QXcbScreen*>(screenForGeometry(rect));

    if (!newScreen)
        newScreen = xcbScreen();

    if (newScreen != currentScreen)
        QWindowSystemInterface::handleWindowScreenChanged(window(), newScreen->QPlatformScreen::screen());

    if (qt_window_private(window())->positionAutomatic) {
        const quint32 mask = XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        const qint32 values[] = {
            qBound<qint32>(1,           rect.width(),  XCOORD_MAX),
            qBound<qint32>(1,           rect.height(), XCOORD_MAX),
        };
        xcb_configure_window(xcb_connection(), m_window, mask, reinterpret_cast<const quint32*>(values));
    } else {
        const quint32 mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;
        const qint32 values[] = {
            qBound<qint32>(-XCOORD_MAX, rect.x(),      XCOORD_MAX),
            qBound<qint32>(-XCOORD_MAX, rect.y(),      XCOORD_MAX),
            qBound<qint32>(1,           rect.width(),  XCOORD_MAX),
            qBound<qint32>(1,           rect.height(), XCOORD_MAX),
        };
        xcb_configure_window(xcb_connection(), m_window, mask, reinterpret_cast<const quint32*>(values));
        if (window()->parent() && !window()->transientParent()) {
            // Wait for server reply for parented windows to ensure that a few window
            // moves will come as a one event. This is important when native widget is
            // moved a few times in X and Y directions causing native scroll. Widget
            // must get single event to not trigger unwanted widget position changes
            // and then expose events causing backingstore flushes with incorrect
            // offset causing image crruption.
            connection()->sync();
        }
    }

    xcb_flush(xcb_connection());
}

QMargins QXcbWindow::frameMargins() const
{
    if (m_dirtyFrameMargins) {
        if (connection()->wmSupport()->isSupportedByWM(atom(QXcbAtom::Atom_NET_FRAME_EXTENTS))) {
            auto reply = Q_XCB_REPLY(xcb_get_property, xcb_connection(), false, m_window,
                                     atom(QXcbAtom::Atom_NET_FRAME_EXTENTS), XCB_ATOM_CARDINAL, 0, 4);
            if (reply && reply->type == XCB_ATOM_CARDINAL && reply->format == 32 && reply->value_len == 4) {
                quint32 *data = (quint32 *)xcb_get_property_value(reply.get());
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

        const QList<xcb_window_t> &virtualRoots =
            connection()->wmSupport()->virtualRoots();

        while (!foundRoot) {
            auto reply = Q_XCB_REPLY_UNCHECKED(xcb_query_tree, xcb_connection(), parent);
            if (reply) {
                if (reply->root == reply->parent || virtualRoots.indexOf(reply->parent) != -1 || reply->parent == XCB_WINDOW_NONE) {
                    foundRoot = true;
                } else {
                    window = parent;
                    parent = reply->parent;
                }
            } else {
                m_dirtyFrameMargins = false;
                m_frameMargins = QMargins();
                return m_frameMargins;
            }
        }

        QPoint offset;

        auto reply = Q_XCB_REPLY(xcb_translate_coordinates, xcb_connection(), window, parent, 0, 0);
        if (reply) {
            offset = QPoint(reply->dst_x, reply->dst_y);
        }

        auto geom = Q_XCB_REPLY(xcb_get_geometry, xcb_connection(), parent);
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

void QXcbWindow::show()
{
    if (window()->isTopLevel()) {
        if (m_recreationReasons != RecreationNotNeeded) {
            qCDebug(lcQpaWindow) << "QXcbWindow: need to recreate window" << window() << m_recreationReasons;
            create();
            m_recreationReasons = RecreationNotNeeded;
        }

        // update WM_NORMAL_HINTS
        propagateSizeHints();

        // update WM_TRANSIENT_FOR
        xcb_window_t transientXcbParent = 0;
        if (isTransient(window())) {
            const QWindow *tp = window()->transientParent();
            if (tp && tp->handle())
                transientXcbParent = tp->handle()->winId();
            // Default to client leader if there is no transient parent, else modal dialogs can
            // be hidden by their parents.
            if (!transientXcbParent)
                transientXcbParent = connection()->clientLeader();
            if (transientXcbParent) { // ICCCM 4.1.2.6
                xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                                    XCB_ATOM_WM_TRANSIENT_FOR, XCB_ATOM_WINDOW, 32,
                                    1, &transientXcbParent);
            }
        }
        if (!transientXcbParent)
            xcb_delete_property(xcb_connection(), m_window, XCB_ATOM_WM_TRANSIENT_FOR);

        // update _NET_WM_STATE
        setNetWmStateOnUnmappedWindow();
    }

    // QWidget-attribute Qt::WA_ShowWithoutActivating.
    const auto showWithoutActivating = window()->property("_q_showWithoutActivating");
    if (showWithoutActivating.isValid() && showWithoutActivating.toBool())
        updateNetWmUserTime(0);
    else if (connection()->time() != XCB_TIME_CURRENT_TIME)
        updateNetWmUserTime(connection()->time());

    if (m_trayIconWindow)
        return; // defer showing until XEMBED_EMBEDDED_NOTIFY

    xcb_map_window(xcb_connection(), m_window);

    if (QGuiApplication::modalWindow() == window())
        requestActivateWindow();

    xcbScreen()->windowShown(this);

    connection()->sync();
}

void QXcbWindow::hide()
{
    xcb_unmap_window(xcb_connection(), m_window);

    // send synthetic UnmapNotify event according to icccm 4.1.4
    q_padded_xcb_event<xcb_unmap_notify_event_t> event = {};
    event.response_type = XCB_UNMAP_NOTIFY;
    event.event = xcbScreen()->root();
    event.window = m_window;
    event.from_configure = false;
    xcb_send_event(xcb_connection(), false, xcbScreen()->root(),
                   XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, (const char *)&event);

    xcb_flush(xcb_connection());

    if (connection()->mouseGrabber() == this)
        connection()->setMouseGrabber(nullptr);
    if (QPlatformWindow *w = connection()->mousePressWindow()) {
        // Unset mousePressWindow when it (or one of its parents) is unmapped
        while (w) {
            if (w == this) {
                connection()->setMousePressWindow(nullptr);
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
        QWindow *enterWindow = nullptr;
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
            QWindowSystemInterface::handleEnterEvent(enterWindow,
                                                     localPos * QHighDpiScaling::factor(enterWindow),
                                                     nativePos);
        }
    }
}

bool QXcbWindow::relayFocusToModalWindow() const
{
    QWindow *w = static_cast<QWindowPrivate *>(QObjectPrivate::get(window()))->eventReceiver();
    // get top-level window
    while (w && w->parent())
        w = w->parent();

    QWindow *modalWindow = nullptr;
    const bool blocked = QGuiApplicationPrivate::instance()->isWindowBlocked(w, &modalWindow);
    if (blocked && modalWindow != w) {
        modalWindow->requestActivate();
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
    connection()->setFocusWindow(w);
    QWindowSystemInterface::handleWindowActivated(w, Qt::ActiveWindowFocusReason);
}

void QXcbWindow::doFocusOut()
{
    connection()->setFocusWindow(nullptr);
    relayFocusToModalWindow();
    // Do not set the active window to nullptr if there is a FocusIn coming.
    connection()->focusInTimer().start();
}

struct QtMotifWmHints {
    quint32 flags, functions, decorations;
    qint32 input_mode; // unused
    quint32 status; // unused
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
};

QXcbWindow::NetWmStates QXcbWindow::netWmStates()
{
    NetWmStates result;

    auto reply = Q_XCB_REPLY_UNCHECKED(xcb_get_property, xcb_connection(),
                                       0, m_window, atom(QXcbAtom::Atom_NET_WM_STATE),
                                       XCB_ATOM_ATOM, 0, 1024);

    if (reply && reply->format == 32 && reply->type == XCB_ATOM_ATOM) {
        const xcb_atom_t *states = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply.get()));
        const xcb_atom_t *statesEnd = states + reply->length;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::Atom_NET_WM_STATE_ABOVE)))
            result |= NetWmStateAbove;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::Atom_NET_WM_STATE_BELOW)))
            result |= NetWmStateBelow;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::Atom_NET_WM_STATE_FULLSCREEN)))
            result |= NetWmStateFullScreen;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::Atom_NET_WM_STATE_MAXIMIZED_HORZ)))
            result |= NetWmStateMaximizedHorz;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::Atom_NET_WM_STATE_MAXIMIZED_VERT)))
            result |= NetWmStateMaximizedVert;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::Atom_NET_WM_STATE_MODAL)))
            result |= NetWmStateModal;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::Atom_NET_WM_STATE_STAYS_ON_TOP)))
            result |= NetWmStateStaysOnTop;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::Atom_NET_WM_STATE_DEMANDS_ATTENTION)))
            result |= NetWmStateDemandsAttention;
        if (statesEnd != std::find(states, statesEnd, atom(QXcbAtom::Atom_NET_WM_STATE_HIDDEN)))
            result |= NetWmStateHidden;
    } else {
        qCDebug(lcQpaXcb, "getting net wm state (%x), empty\n", m_window);
    }

    return result;
}

void QXcbWindow::setWindowFlags(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    if (type == Qt::ToolTip)
        flags |= Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint;
    if (type == Qt::Popup)
        flags |= Qt::X11BypassWindowManagerHint;

    Qt::WindowFlags oldflags = window()->flags();
    if ((oldflags & Qt::WindowStaysOnTopHint) != (flags & Qt::WindowStaysOnTopHint))
        m_recreationReasons |= WindowStaysOnTopHintChanged;
    if ((oldflags & Qt::WindowStaysOnBottomHint) != (flags & Qt::WindowStaysOnBottomHint))
        m_recreationReasons |= WindowStaysOnBottomHintChanged;

    const quint32 mask = XCB_CW_OVERRIDE_REDIRECT | XCB_CW_EVENT_MASK;
    const quint32 values[] = {
         // XCB_CW_OVERRIDE_REDIRECT
         (flags & Qt::BypassWindowManagerHint) ? 1u : 0,
         // XCB_CW_EVENT_MASK
         (flags & Qt::WindowTransparentForInput) ? transparentForInputEventMask : defaultEventMask
     };

    xcb_change_window_attributes(xcb_connection(), xcb_window(), mask, values);

    WindowTypes wmWindowTypes;
    if (window()->dynamicPropertyNames().contains(wm_window_type_property_id)) {
        wmWindowTypes = static_cast<WindowTypes>(
            qvariant_cast<int>(window()->property(wm_window_type_property_id)));
    }

    setWmWindowType(wmWindowTypes, flags);
    setNetWmState(flags);
    setMotifWmHints(flags);

    setTransparentForMouseEvents(flags & Qt::WindowTransparentForInput);
    updateDoesNotAcceptFocus(flags & Qt::WindowDoesNotAcceptFocus);
}

void QXcbWindow::setMotifWmHints(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    QtMotifWmHints mwmhints;
    memset(&mwmhints, 0, sizeof(mwmhints));

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

    if (mwmhints.flags) {
        xcb_change_property(xcb_connection(),
                            XCB_PROP_MODE_REPLACE,
                            m_window,
                            atom(QXcbAtom::Atom_MOTIF_WM_HINTS),
                            atom(QXcbAtom::Atom_MOTIF_WM_HINTS),
                            32,
                            5,
                            &mwmhints);
    } else {
        xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::Atom_MOTIF_WM_HINTS));
    }
}

void QXcbWindow::setNetWmState(bool set, xcb_atom_t one, xcb_atom_t two)
{
    xcb_client_message_event_t event;

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.sequence = 0;
    event.window = m_window;
    event.type = atom(QXcbAtom::Atom_NET_WM_STATE);
    event.data.data32[0] = set ? 1 : 0;
    event.data.data32[1] = one;
    event.data.data32[2] = two;
    event.data.data32[3] = 0;
    event.data.data32[4] = 0;

    xcb_send_event(xcb_connection(), 0, xcbScreen()->root(),
                   XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                   (const char *)&event);
}

void QXcbWindow::setNetWmState(Qt::WindowStates state)
{
    if ((m_windowState ^ state) & Qt::WindowMaximized) {
        setNetWmState(state & Qt::WindowMaximized,
                      atom(QXcbAtom::Atom_NET_WM_STATE_MAXIMIZED_HORZ),
                      atom(QXcbAtom::Atom_NET_WM_STATE_MAXIMIZED_VERT));
    }

    if ((m_windowState ^ state) & Qt::WindowFullScreen)
        setNetWmState(state & Qt::WindowFullScreen, atom(QXcbAtom::Atom_NET_WM_STATE_FULLSCREEN));
}

void QXcbWindow::setNetWmState(Qt::WindowFlags flags)
{
    setNetWmState(flags & Qt::WindowStaysOnTopHint,
                  atom(QXcbAtom::Atom_NET_WM_STATE_ABOVE),
                  atom(QXcbAtom::Atom_NET_WM_STATE_STAYS_ON_TOP));
    setNetWmState(flags & Qt::WindowStaysOnBottomHint, atom(QXcbAtom::Atom_NET_WM_STATE_BELOW));
}

void QXcbWindow::setNetWmStateOnUnmappedWindow()
{
    if (Q_UNLIKELY(m_mapped))
        qCDebug(lcQpaXcb()) << "internal info: " << Q_FUNC_INFO << "called on mapped window";

    NetWmStates states;
    const Qt::WindowFlags flags = window()->flags();
    if (flags & Qt::WindowStaysOnTopHint) {
        states |= NetWmStateAbove;
        states |= NetWmStateStaysOnTop;
    } else if (flags & Qt::WindowStaysOnBottomHint) {
        states |= NetWmStateBelow;
    }

    if (window()->windowStates() & Qt::WindowMinimized)
        states |= NetWmStateHidden;

    if (window()->windowStates() & Qt::WindowFullScreen)
        states |= NetWmStateFullScreen;

    if (window()->windowStates() & Qt::WindowMaximized) {
        states |= NetWmStateMaximizedHorz;
        states |= NetWmStateMaximizedVert;
    }

    if (window()->modality() != Qt::NonModal)
        states |= NetWmStateModal;

    // According to EWMH:
    //    "The Window Manager should remove _NET_WM_STATE whenever a window is withdrawn".
    // Which means that we don't have to read this property before changing it on a withdrawn
    // window. But there are situations where users want to adjust this property as well
    // (e4cea305ed2ba3c9f580bf9d16c59a1048af0e8a), so instead of overwriting the property
    // we first read it and then merge our hints with the existing values, allowing a user
    // to set custom hints.

    QList<xcb_atom_t> atoms;
    auto reply = Q_XCB_REPLY_UNCHECKED(xcb_get_property, xcb_connection(),
                                       0, m_window, atom(QXcbAtom::Atom_NET_WM_STATE),
                                       XCB_ATOM_ATOM, 0, 1024);
    if (reply && reply->format == 32 && reply->type == XCB_ATOM_ATOM && reply->value_len > 0) {
        const xcb_atom_t *data = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply.get()));
        atoms.resize(reply->value_len);
        memcpy((void *)&atoms.first(), (void *)data, reply->value_len * sizeof(xcb_atom_t));
    }

    if (states & NetWmStateAbove && !atoms.contains(atom(QXcbAtom::Atom_NET_WM_STATE_ABOVE)))
        atoms.push_back(atom(QXcbAtom::Atom_NET_WM_STATE_ABOVE));
    if (states & NetWmStateBelow && !atoms.contains(atom(QXcbAtom::Atom_NET_WM_STATE_BELOW)))
        atoms.push_back(atom(QXcbAtom::Atom_NET_WM_STATE_BELOW));
    if (states & NetWmStateHidden && !atoms.contains(atom(QXcbAtom::Atom_NET_WM_STATE_HIDDEN)))
        atoms.push_back(atom(QXcbAtom::Atom_NET_WM_STATE_HIDDEN));
    if (states & NetWmStateFullScreen && !atoms.contains(atom(QXcbAtom::Atom_NET_WM_STATE_FULLSCREEN)))
        atoms.push_back(atom(QXcbAtom::Atom_NET_WM_STATE_FULLSCREEN));
    if (states & NetWmStateMaximizedHorz && !atoms.contains(atom(QXcbAtom::Atom_NET_WM_STATE_MAXIMIZED_HORZ)))
        atoms.push_back(atom(QXcbAtom::Atom_NET_WM_STATE_MAXIMIZED_HORZ));
    if (states & NetWmStateMaximizedVert && !atoms.contains(atom(QXcbAtom::Atom_NET_WM_STATE_MAXIMIZED_VERT)))
        atoms.push_back(atom(QXcbAtom::Atom_NET_WM_STATE_MAXIMIZED_VERT));
    if (states & NetWmStateModal && !atoms.contains(atom(QXcbAtom::Atom_NET_WM_STATE_MODAL)))
        atoms.push_back(atom(QXcbAtom::Atom_NET_WM_STATE_MODAL));
    if (states & NetWmStateStaysOnTop && !atoms.contains(atom(QXcbAtom::Atom_NET_WM_STATE_STAYS_ON_TOP)))
        atoms.push_back(atom(QXcbAtom::Atom_NET_WM_STATE_STAYS_ON_TOP));
    if (states & NetWmStateDemandsAttention && !atoms.contains(atom(QXcbAtom::Atom_NET_WM_STATE_DEMANDS_ATTENTION)))
        atoms.push_back(atom(QXcbAtom::Atom_NET_WM_STATE_DEMANDS_ATTENTION));

    if (atoms.isEmpty()) {
        xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::Atom_NET_WM_STATE));
    } else {
        xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                            atom(QXcbAtom::Atom_NET_WM_STATE), XCB_ATOM_ATOM, 32,
                            atoms.size(), atoms.constData());
    }
    xcb_flush(xcb_connection());
}

void QXcbWindow::setWindowState(Qt::WindowStates state)
{
    if (state == m_windowState)
        return;

    Qt::WindowStates unsetState = m_windowState & ~state;
    Qt::WindowStates newState =  state & ~m_windowState;

    // unset old state
    if (unsetState & Qt::WindowMinimized)
        xcb_map_window(xcb_connection(), m_window);
    if (unsetState & Qt::WindowMaximized)
        setNetWmState(false,
                      atom(QXcbAtom::Atom_NET_WM_STATE_MAXIMIZED_HORZ),
                      atom(QXcbAtom::Atom_NET_WM_STATE_MAXIMIZED_VERT));
    if (unsetState & Qt::WindowFullScreen)
        setNetWmState(false, atom(QXcbAtom::Atom_NET_WM_STATE_FULLSCREEN));

    // set new state
    if (newState & Qt::WindowMinimized) {
        {
            xcb_client_message_event_t event;

            event.response_type = XCB_CLIENT_MESSAGE;
            event.format = 32;
            event.sequence = 0;
            event.window = m_window;
            event.type = atom(QXcbAtom::AtomWM_CHANGE_STATE);
            event.data.data32[0] = XCB_ICCCM_WM_STATE_ICONIC;
            event.data.data32[1] = 0;
            event.data.data32[2] = 0;
            event.data.data32[3] = 0;
            event.data.data32[4] = 0;

            xcb_send_event(xcb_connection(), 0, xcbScreen()->root(),
                           XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                           (const char *)&event);
        }
        m_minimized = true;
    }

    // set Maximized && FullScreen state if need
    setNetWmState(state);

    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_hints_unchecked(xcb_connection(), m_window);
    xcb_icccm_wm_hints_t hints;
    if (xcb_icccm_get_wm_hints_reply(xcb_connection(), cookie, &hints, nullptr)) {
        if (state & Qt::WindowMinimized)
            xcb_icccm_wm_hints_set_iconic(&hints);
        else
            xcb_icccm_wm_hints_set_normal(&hints);
        xcb_icccm_set_wm_hints(xcb_connection(), m_window, &hints);
    }

    connection()->sync();
    m_windowState = state;
}

void QXcbWindow::updateNetWmUserTime(xcb_timestamp_t timestamp)
{
    xcb_window_t wid = m_window;
    // If timestamp == 0, then it means that the window should not be
    // initially activated. Don't update global user time for this
    // special case.
    if (timestamp != 0)
        connection()->setNetWmUserTime(timestamp);

    const bool isSupportedByWM = connection()->wmSupport()->isSupportedByWM(atom(QXcbAtom::Atom_NET_WM_USER_TIME_WINDOW));
    if (m_netWmUserTimeWindow || isSupportedByWM) {
        if (!m_netWmUserTimeWindow) {
            m_netWmUserTimeWindow = xcb_generate_id(xcb_connection());
            xcb_create_window(xcb_connection(),
                              XCB_COPY_FROM_PARENT,            // depth -- same as root
                              m_netWmUserTimeWindow,           // window id
                              m_window,                        // parent window id
                              -1, -1, 1, 1,
                              0,                               // border width
                              XCB_WINDOW_CLASS_INPUT_OUTPUT,   // window class
                              m_visualId,                      // visual
                              0,                               // value mask
                              nullptr);                        // value list
            wid = m_netWmUserTimeWindow;
            xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window, atom(QXcbAtom::Atom_NET_WM_USER_TIME_WINDOW),
                                XCB_ATOM_WINDOW, 32, 1, &m_netWmUserTimeWindow);
            xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::Atom_NET_WM_USER_TIME));

            QXcbWindow::setWindowTitle(connection(), m_netWmUserTimeWindow,
                                       QStringLiteral("Qt NET_WM User Time Window"));

        } else if (!isSupportedByWM) {
            // WM no longer supports it, then we should remove the
            // _NET_WM_USER_TIME_WINDOW atom.
            xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::Atom_NET_WM_USER_TIME_WINDOW));
            xcb_destroy_window(xcb_connection(), m_netWmUserTimeWindow);
            m_netWmUserTimeWindow = XCB_NONE;
        } else {
            wid = m_netWmUserTimeWindow;
        }
    }
    xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, wid, atom(QXcbAtom::Atom_NET_WM_USER_TIME),
                        XCB_ATOM_CARDINAL, 32, 1, &timestamp);
}

void QXcbWindow::setTransparentForMouseEvents(bool transparent)
{
    if (!connection()->hasXFixes() || transparent == m_transparent)
        return;

    xcb_rectangle_t rectangle;

    xcb_rectangle_t *rect = nullptr;
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
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_hints_unchecked(xcb_connection(), m_window);

    xcb_icccm_wm_hints_t hints;
    if (!xcb_icccm_get_wm_hints_reply(xcb_connection(), cookie, &hints, nullptr))
        return;

    xcb_icccm_wm_hints_set_input(&hints, !doesNotAcceptFocus);
    xcb_icccm_set_wm_hints(xcb_connection(), m_window, &hints);
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
        m_embedded = qXcbParent->isForeignWindow();
    } else {
        xcb_parent_id = xcbScreen()->root();
        m_embedded = false;
    }
    xcb_reparent_window(xcb_connection(), xcb_window(), xcb_parent_id, topLeft.x(), topLeft.y());
}

void QXcbWindow::setWindowTitle(const QString &title)
{
    setWindowTitle(connection(), m_window, title);
}

void QXcbWindow::setWindowIconText(const QString &title)
{
    const QByteArray ba = title.toUtf8();
    xcb_change_property(xcb_connection(),
                        XCB_PROP_MODE_REPLACE,
                        m_window,
                        atom(QXcbAtom::Atom_NET_WM_ICON_NAME),
                        atom(QXcbAtom::AtomUTF8_STRING),
                        8,
                        ba.size(),
                        ba.constData());
}

void QXcbWindow::setWindowIcon(const QIcon &icon)
{
    QList<quint32> icon_data;
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
        // Ignore icon exceeding maximum xcb request length
        if (quint64(icon_data.size()) > quint64(xcb_get_maximum_request_length(xcb_connection()))) {
            qWarning() << "Ignoring window icon" << icon_data.size()
                       << "exceeds maximum xcb request length"
                       << xcb_get_maximum_request_length(xcb_connection());
            return;
        }
        xcb_change_property(xcb_connection(),
                            XCB_PROP_MODE_REPLACE,
                            m_window,
                            atom(QXcbAtom::Atom_NET_WM_ICON),
                            atom(QXcbAtom::AtomCARDINAL),
                            32,
                            icon_data.size(),
                            (unsigned char *) icon_data.data());
    } else {
        xcb_delete_property(xcb_connection(),
                            m_window,
                            atom(QXcbAtom::Atom_NET_WM_ICON));
    }
}

void QXcbWindow::raise()
{
    const quint32 mask = XCB_CONFIG_WINDOW_STACK_MODE;
    const quint32 values[] = { XCB_STACK_MODE_ABOVE };
    xcb_configure_window(xcb_connection(), m_window, mask, values);
}

void QXcbWindow::lower()
{
    const quint32 mask = XCB_CONFIG_WINDOW_STACK_MODE;
    const quint32 values[] = { XCB_STACK_MODE_BELOW };
    xcb_configure_window(xcb_connection(), m_window, mask, values);
}

void QXcbWindow::propagateSizeHints()
{
    // update WM_NORMAL_HINTS
    xcb_size_hints_t hints;
    memset(&hints, 0, sizeof(hints));

    const QRect rect = geometry();
    QWindowPrivate *win = qt_window_private(window());

    if (!win->positionAutomatic)
        xcb_icccm_size_hints_set_position(&hints, true, rect.x(), rect.y());
    if (rect.width() < QWINDOWSIZE_MAX || rect.height() < QWINDOWSIZE_MAX)
        xcb_icccm_size_hints_set_size(&hints, true, rect.width(), rect.height());

    /* Gravity describes how to interpret x and y values the next time
       window needs to be positioned on a screen.
       XCB_GRAVITY_STATIC     : the left top corner of the client window
       XCB_GRAVITY_NORTH_WEST : the left top corner of the frame window */
    auto gravity = win->positionPolicy == QWindowPrivate::WindowFrameInclusive
                   ? XCB_GRAVITY_NORTH_WEST : XCB_GRAVITY_STATIC;

    xcb_icccm_size_hints_set_win_gravity(&hints, gravity);

    QSize minimumSize = windowMinimumSize();
    QSize maximumSize = windowMaximumSize();
    QSize baseSize = windowBaseSize();
    QSize sizeIncrement = windowSizeIncrement();

    if (minimumSize.width() > 0 || minimumSize.height() > 0)
        xcb_icccm_size_hints_set_min_size(&hints,
                                          qMin(XCOORD_MAX,minimumSize.width()),
                                          qMin(XCOORD_MAX,minimumSize.height()));

    if (maximumSize.width() < QWINDOWSIZE_MAX || maximumSize.height() < QWINDOWSIZE_MAX)
        xcb_icccm_size_hints_set_max_size(&hints,
                                          qMin(XCOORD_MAX, maximumSize.width()),
                                          qMin(XCOORD_MAX, maximumSize.height()));

    if (sizeIncrement.width() > 0 || sizeIncrement.height() > 0) {
        xcb_icccm_size_hints_set_base_size(&hints, baseSize.width(), baseSize.height());
        xcb_icccm_size_hints_set_resize_inc(&hints, sizeIncrement.width(), sizeIncrement.height());
    }

    xcb_icccm_set_wm_normal_hints(xcb_connection(), m_window, &hints);

    m_sizeHintsScaleFactor = QHighDpiScaling::factor(screen());
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
        && connection()->wmSupport()->isSupportedByWM(atom(QXcbAtom::Atom_NET_ACTIVE_WINDOW))) {
        xcb_client_message_event_t event;

        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.sequence = 0;
        event.window = m_window;
        event.type = atom(QXcbAtom::Atom_NET_ACTIVE_WINDOW);
        event.data.data32[0] = 1;
        event.data.data32[1] = connection()->time();
        event.data.data32[2] = focusWindow ? focusWindow->winId() : XCB_NONE;
        event.data.data32[3] = 0;
        event.data.data32[4] = 0;

        xcb_send_event(xcb_connection(), 0, xcbScreen()->root(),
                       XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                       (const char *)&event);
    } else {
        xcb_set_input_focus(xcb_connection(), XCB_INPUT_FOCUS_PARENT, m_window, connection()->time());
    }

    connection()->sync();
}

QSurfaceFormat QXcbWindow::format() const
{
    return m_format;
}

QXcbWindow::WindowTypes QXcbWindow::wmWindowTypes() const
{
    WindowTypes result;

    auto reply = Q_XCB_REPLY_UNCHECKED(xcb_get_property, xcb_connection(),
                                       0, m_window, atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE),
                                       XCB_ATOM_ATOM, 0, 1024);
    if (reply && reply->format == 32 && reply->type == XCB_ATOM_ATOM) {
        const xcb_atom_t *types = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply.get()));
        const xcb_atom_t *types_end = types + reply->length;
        for (; types != types_end; types++) {
            QXcbAtom::Atom type = connection()->qatom(*types);
            switch (type) {
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_NORMAL:
                result |= WindowType::Normal;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DESKTOP:
                result |= WindowType::Desktop;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DOCK:
                result |= WindowType::Dock;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_TOOLBAR:
                result |= WindowType::Toolbar;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_MENU:
                result |= WindowType::Menu;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_UTILITY:
                result |= WindowType::Utility;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_SPLASH:
                result |= WindowType::Splash;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DIALOG:
                result |= WindowType::Dialog;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DROPDOWN_MENU:
                result |= WindowType::DropDownMenu;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_POPUP_MENU:
                result |= WindowType::PopupMenu;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_TOOLTIP:
                result |= WindowType::Tooltip;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_NOTIFICATION:
                result |= WindowType::Notification;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_COMBO:
                result |= WindowType::Combo;
                break;
            case QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DND:
                result |= WindowType::Dnd;
                break;
            case QXcbAtom::Atom_KDE_NET_WM_WINDOW_TYPE_OVERRIDE:
                result |= WindowType::KdeOverride;
                break;
            default:
                break;
            }
        }
    }
    return result;
}

void QXcbWindow::setWmWindowType(WindowTypes types, Qt::WindowFlags flags)
{
    QList<xcb_atom_t> atoms;

    // manual selection 1 (these are never set by Qt and take precedence)
    if (types & WindowType::Normal)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_NORMAL));
    if (types & WindowType::Desktop)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DESKTOP));
    if (types & WindowType::Dock)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DOCK));
    if (types & WindowType::Notification)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_NOTIFICATION));

    // manual selection 2 (Qt uses these during auto selection);
    if (types & WindowType::Utility)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_UTILITY));
    if (types & WindowType::Splash)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_SPLASH));
    if (types & WindowType::Dialog)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DIALOG));
    if (types & WindowType::Tooltip)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_TOOLTIP));
    if (types & WindowType::KdeOverride)
        atoms.append(atom(QXcbAtom::Atom_KDE_NET_WM_WINDOW_TYPE_OVERRIDE));

    // manual selection 3 (these can be set by Qt, but don't have a
    // corresponding Qt::WindowType). note that order of the *MENU
    // atoms is important
    if (types & WindowType::Menu)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_MENU));
    if (types & WindowType::DropDownMenu)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DROPDOWN_MENU));
    if (types & WindowType::PopupMenu)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_POPUP_MENU));
    if (types & WindowType::Toolbar)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_TOOLBAR));
    if (types & WindowType::Combo)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_COMBO));
    if (types & WindowType::Dnd)
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DND));

    // automatic selection
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));
    switch (type) {
    case Qt::Dialog:
    case Qt::Sheet:
        if (!(types & WindowType::Dialog))
            atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_DIALOG));
        break;
    case Qt::Tool:
    case Qt::Drawer:
        if (!(types & WindowType::Utility))
            atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_UTILITY));
        break;
    case Qt::ToolTip:
        if (!(types & WindowType::Tooltip))
            atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_TOOLTIP));
        break;
    case Qt::SplashScreen:
        if (!(types & WindowType::Splash))
            atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_SPLASH));
        break;
    default:
        break;
    }

    if ((flags & Qt::FramelessWindowHint) && !(types & WindowType::KdeOverride)) {
        // override netwm type - quick and easy for KDE noborder
        atoms.append(atom(QXcbAtom::Atom_KDE_NET_WM_WINDOW_TYPE_OVERRIDE));
    }

    if (atoms.size() == 1 && atoms.first() == atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_NORMAL))
        atoms.clear();
    else
        atoms.append(atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE_NORMAL));

    if (atoms.isEmpty()) {
        xcb_delete_property(xcb_connection(), m_window, atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE));
    } else {
        xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                            atom(QXcbAtom::Atom_NET_WM_WINDOW_TYPE), XCB_ATOM_ATOM, 32,
                            atoms.size(), atoms.constData());
    }
    xcb_flush(xcb_connection());
}

void QXcbWindow::setWindowRole(const QString &role)
{
    QByteArray roleData = role.toLatin1();
    xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, m_window,
                        atom(QXcbAtom::AtomWM_WINDOW_ROLE), XCB_ATOM_STRING, 8,
                        roleData.size(), roleData.constData());
}

void QXcbWindow::setParentRelativeBackPixmap()
{
    const quint32 mask = XCB_CW_BACK_PIXMAP;
    const quint32 values[] = { XCB_BACK_PIXMAP_PARENT_RELATIVE };
    xcb_change_window_attributes(xcb_connection(), m_window, mask, values);
}

bool QXcbWindow::requestSystemTrayWindowDock()
{
    if (!connection()->systemTrayTracker())
        return false;
    connection()->systemTrayTracker()->requestSystemTrayWindowDock(m_window);
    return true;
}

bool QXcbWindow::handleNativeEvent(xcb_generic_event_t *event)
{
    auto eventType = connection()->nativeInterface()->nativeEventType();
    qintptr result = 0; // Used only by MS Windows
    return QWindowSystemInterface::handleNativeEvent(window(), eventType, event, &result);
}

void QXcbWindow::handleExposeEvent(const xcb_expose_event_t *event)
{
    QRect rect(event->x, event->y, event->width, event->height);
    m_exposeRegion |= rect;

    bool pending = true;

    connection()->eventQueue()->peek(QXcbEventQueue::PeekConsumeMatchAndContinue,
                                     [this, &pending](xcb_generic_event_t *event, int type) {
        if (type != XCB_EXPOSE)
            return false;
        auto expose = reinterpret_cast<xcb_expose_event_t *>(event);
        if (expose->window != m_window)
            return false;
        if (expose->count == 0)
            pending = false;
        m_exposeRegion |= QRect(expose->x, expose->y, expose->width, expose->height);
        free(expose);
        return true;
    });

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

    if (event->type == atom(QXcbAtom::AtomWM_PROTOCOLS)) {
        xcb_atom_t protocolAtom = event->data.data32[0];
        if (protocolAtom == atom(QXcbAtom::AtomWM_DELETE_WINDOW)) {
            QWindowSystemInterface::handleCloseEvent(window());
        } else if (protocolAtom == atom(QXcbAtom::AtomWM_TAKE_FOCUS)) {
            connection()->setTime(event->data.data32[1]);
            relayFocusToModalWindow();
            return;
        } else if (protocolAtom == atom(QXcbAtom::Atom_NET_WM_PING)) {
            if (event->window == xcbScreen()->root())
                return;

            xcb_client_message_event_t reply = *event;

            reply.response_type = XCB_CLIENT_MESSAGE;
            reply.window = xcbScreen()->root();

            xcb_send_event(xcb_connection(), 0, xcbScreen()->root(),
                           XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                           (const char *)&reply);
            xcb_flush(xcb_connection());
        } else if (protocolAtom == atom(QXcbAtom::Atom_NET_WM_SYNC_REQUEST)) {
            connection()->setTime(event->data.data32[1]);
            m_syncValue.lo = event->data.data32[2];
            m_syncValue.hi = event->data.data32[3];
            if (connection()->hasXSync())
                m_syncState = SyncReceived;
#ifndef QT_NO_WHATSTHIS
        } else if (protocolAtom == atom(QXcbAtom::Atom_NET_WM_CONTEXT_HELP)) {
            QWindowSystemInterface::handleEnterWhatsThisEvent();
#endif
        } else {
            qCWarning(lcQpaXcb, "Unhandled WM_PROTOCOLS (%s)",
                      connection()->atomName(protocolAtom).constData());
        }
#if QT_CONFIG(draganddrop)
    } else if (event->type == atom(QXcbAtom::AtomXdndEnter)) {
        connection()->drag()->handleEnter(this, event);
    } else if (event->type == atom(QXcbAtom::AtomXdndPosition)) {
        connection()->drag()->handlePosition(this, event);
    } else if (event->type == atom(QXcbAtom::AtomXdndLeave)) {
        connection()->drag()->handleLeave(this, event);
    } else if (event->type == atom(QXcbAtom::AtomXdndDrop)) {
        connection()->drag()->handleDrop(this, event);
#endif
    } else if (event->type == atom(QXcbAtom::Atom_XEMBED)) {
        handleXEmbedMessage(event);
    } else if (event->type == atom(QXcbAtom::Atom_NET_ACTIVE_WINDOW)) {
        doFocusIn();
    } else if (event->type == atom(QXcbAtom::AtomMANAGER)
               || event->type == atom(QXcbAtom::Atom_NET_WM_STATE)
               || event->type == atom(QXcbAtom::AtomWM_CHANGE_STATE)) {
        // Ignore _NET_WM_STATE, MANAGER which are relate to tray icons
        // and other messages.
    } else if (event->type == atom(QXcbAtom::Atom_COMPIZ_DECOR_PENDING)
            || event->type == atom(QXcbAtom::Atom_COMPIZ_DECOR_REQUEST)
            || event->type == atom(QXcbAtom::Atom_COMPIZ_DECOR_DELETE_PIXMAP)
            || event->type == atom(QXcbAtom::Atom_COMPIZ_TOOLKIT_ACTION)
            || event->type == atom(QXcbAtom::Atom_GTK_LOAD_ICONTHEMES)) {
        //silence the _COMPIZ and _GTK messages for now
    } else {
        qCWarning(lcQpaXcb) << "Unhandled client message: " << connection()->atomName(event->type);
    }
}

void QXcbWindow::handleConfigureNotifyEvent(const xcb_configure_notify_event_t *event)
{
    bool fromSendEvent = (event->response_type & 0x80);
    QPoint pos(event->x, event->y);
    if (!parent() && !fromSendEvent) {
        // Do not trust the position, query it instead.
        auto reply = Q_XCB_REPLY(xcb_translate_coordinates, xcb_connection(),
                                 xcb_window(), xcbScreen()->root(), 0, 0);
        if (reply) {
            pos.setX(reply->dst_x);
            pos.setY(reply->dst_y);
        }
    }

    const QRect actualGeometry = QRect(pos, QSize(event->width, event->height));
    QPlatformScreen *newScreen = parent() ? parent()->screen() : screenForGeometry(actualGeometry);
    if (!newScreen)
        return;

    QWindowSystemInterface::handleGeometryChange(window(), actualGeometry);

    // QPlatformScreen::screen() is updated asynchronously, so we can't compare it
    // with the newScreen. Just send the WindowScreenChanged event and QGuiApplication
    // will make the comparison later.
    QWindowSystemInterface::handleWindowScreenChanged(window(), newScreen->screen());

    if (!qFuzzyCompare(QHighDpiScaling::factor(newScreen), m_sizeHintsScaleFactor))
        propagateSizeHints();

    // Send the synthetic expose event on resize only when the window is shrunk,
    // because the "XCB_GRAVITY_NORTH_WEST" flag doesn't send it automatically.
    if (!m_oldWindowSize.isEmpty()
            && (actualGeometry.width() < m_oldWindowSize.width()
                || actualGeometry.height() < m_oldWindowSize.height())) {
        QWindowSystemInterface::handleExposeEvent(window(), QRegion(0, 0, actualGeometry.width(), actualGeometry.height()));
    }
    m_oldWindowSize = actualGeometry.size();

    if (connection()->hasXSync() && m_syncState == SyncReceived)
        m_syncState = SyncAndConfigureReceived;

    m_dirtyFrameMargins = true;
}

bool QXcbWindow::isExposed() const
{
    return m_mapped;
}

bool QXcbWindow::isEmbedded() const
{
    return m_embedded;
}

QPoint QXcbWindow::mapToGlobal(const QPoint &pos) const
{
    if (!m_embedded)
        return QPlatformWindow::mapToGlobal(pos);

    QPoint ret;
    auto reply = Q_XCB_REPLY(xcb_translate_coordinates, xcb_connection(),
                             xcb_window(), xcbScreen()->root(),
                             pos.x(), pos.y());
    if (reply) {
        ret.setX(reply->dst_x);
        ret.setY(reply->dst_y);
    }

    return ret;
}

QPoint QXcbWindow::mapFromGlobal(const QPoint &pos) const
{
    if (!m_embedded)
        return QPlatformWindow::mapFromGlobal(pos);

    QPoint ret;
    auto reply = Q_XCB_REPLY(xcb_translate_coordinates, xcb_connection(),
                             xcbScreen()->root(), xcb_window(),
                             pos.x(), pos.y());
    if (reply) {
        ret.setX(reply->dst_x);
        ret.setY(reply->dst_y);
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
                                        int detail, Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp,
                                        QEvent::Type type, Qt::MouseEventSource source)
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

    if (m_embedded && !m_trayIconWindow) {
        if (window() != QGuiApplication::focusWindow()) {
            const QXcbWindow *container = static_cast<const QXcbWindow *>(parent());
            Q_ASSERT(container != nullptr);

            sendXEmbedMessage(container->xcb_window(), XEMBED_REQUEST_FOCUS);
        }
    }
    QPoint local(event_x, event_y);
    QPoint global(root_x, root_y);

    if (isWheel) {
        if (!connection()->isAtLeastXI21()) {
            QPoint angleDelta;
            if (detail == 4)
                angleDelta.setY(120);
            else if (detail == 5)
                angleDelta.setY(-120);
            else if (detail == 6)
                angleDelta.setX(120);
            else if (detail == 7)
                angleDelta.setX(-120);
            if (modifiers & Qt::AltModifier)
                angleDelta = angleDelta.transposed();
            QWindowSystemInterface::handleWheelEvent(window(), timestamp, local, global, QPoint(), angleDelta, modifiers);
        }
        return;
    }

    connection()->setMousePressWindow(this);

    handleMouseEvent(timestamp, local, global, modifiers, type, source);
}

void QXcbWindow::handleButtonReleaseEvent(int event_x, int event_y, int root_x, int root_y,
                                          int detail, Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp,
                                          QEvent::Type type, Qt::MouseEventSource source)
{
    QPoint local(event_x, event_y);
    QPoint global(root_x, root_y);

    if (detail >= 4 && detail <= 7) {
        // mouse wheel, handled in handleButtonPressEvent()
        return;
    }

    if (connection()->buttonState() == Qt::NoButton)
        connection()->setMousePressWindow(nullptr);

    handleMouseEvent(timestamp, local, global, modifiers, type, source);
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
        const bool mouseButtonsPressed = (conn->buttonState() != Qt::NoButton);
        return mouseButtonsPressed || conn->hasXInput2();
    }
    return true;
}

static bool ignoreLeaveEvent(quint8 mode, quint8 detail, QXcbConnection *conn = nullptr)
{
    return ((doCheckUnGrabAncestor(conn)
             && mode == XCB_NOTIFY_MODE_GRAB && detail == XCB_NOTIFY_DETAIL_ANCESTOR)
            || (mode == XCB_NOTIFY_MODE_UNGRAB && detail == XCB_NOTIFY_DETAIL_INFERIOR)
            || detail == XCB_NOTIFY_DETAIL_VIRTUAL
            || detail == XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL);
}

static bool ignoreEnterEvent(quint8 mode, quint8 detail, QXcbConnection *conn = nullptr)
{
    return ((doCheckUnGrabAncestor(conn)
             && mode == XCB_NOTIFY_MODE_UNGRAB && detail == XCB_NOTIFY_DETAIL_ANCESTOR)
            || (mode != XCB_NOTIFY_MODE_NORMAL && mode != XCB_NOTIFY_MODE_UNGRAB)
            || detail == XCB_NOTIFY_DETAIL_VIRTUAL
            || detail == XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL);
}

void QXcbWindow::handleEnterNotifyEvent(int event_x, int event_y, int root_x, int root_y,
                                        quint8 mode, quint8 detail, xcb_timestamp_t timestamp)
{
    connection()->setTime(timestamp);

    const QPoint global = QPoint(root_x, root_y);

    if (ignoreEnterEvent(mode, detail, connection()) || connection()->mousePressWindow())
        return;

    // Updates scroll valuators, as user might have done some scrolling outside our X client.
    connection()->xi2UpdateScrollingDevices();

    const QPoint local(event_x, event_y);
    QWindowSystemInterface::handleEnterEvent(window(), local, global);
}

void QXcbWindow::handleLeaveNotifyEvent(int root_x, int root_y,
                                        quint8 mode, quint8 detail, xcb_timestamp_t timestamp)
{
    connection()->setTime(timestamp);

    if (ignoreLeaveEvent(mode, detail, connection()) || connection()->mousePressWindow())
        return;

    // check if enter event is buffered
    auto event = connection()->eventQueue()->peek([](xcb_generic_event_t *event, int type) {
        if (type != XCB_ENTER_NOTIFY)
            return false;
        auto enter = reinterpret_cast<xcb_enter_notify_event_t *>(event);
        return !ignoreEnterEvent(enter->mode, enter->detail);
    });
    auto enter = reinterpret_cast<xcb_enter_notify_event_t *>(event);
    QXcbWindow *enterWindow = enter ? connection()->platformWindowFromId(enter->event) : nullptr;

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
                                         Qt::KeyboardModifiers modifiers, xcb_timestamp_t timestamp,
                                         QEvent::Type type, Qt::MouseEventSource source)
{
    QPoint local(event_x, event_y);
    QPoint global(root_x, root_y);

    // "mousePressWindow" can be NULL i.e. if a window will be grabbed or unmapped, so set it again here.
    // Unset "mousePressWindow" when mouse button isn't pressed - in some cases the release event won't arrive.
    const bool isMouseButtonPressed = (connection()->buttonState() != Qt::NoButton);
    const bool hasMousePressWindow = (connection()->mousePressWindow() != nullptr);
    if (isMouseButtonPressed && !hasMousePressWindow)
        connection()->setMousePressWindow(this);
    else if (hasMousePressWindow && !isMouseButtonPressed)
        connection()->setMousePressWindow(nullptr);

    handleMouseEvent(timestamp, local, global, modifiers, type, source);
}

void QXcbWindow::handleButtonPressEvent(const xcb_button_press_event_t *event)
{
    Qt::KeyboardModifiers modifiers = connection()->keyboard()->translateModifiers(event->state);
    handleButtonPressEvent(event->event_x, event->event_y, event->root_x, event->root_y, event->detail,
                           modifiers, event->time, QEvent::MouseButtonPress);
}

void QXcbWindow::handleButtonReleaseEvent(const xcb_button_release_event_t *event)
{
    Qt::KeyboardModifiers modifiers = connection()->keyboard()->translateModifiers(event->state);
    handleButtonReleaseEvent(event->event_x, event->event_y, event->root_x, event->root_y, event->detail,
                             modifiers, event->time, QEvent::MouseButtonRelease);
}

void QXcbWindow::handleMotionNotifyEvent(const xcb_motion_notify_event_t *event)
{
    Qt::KeyboardModifiers modifiers = connection()->keyboard()->translateModifiers(event->state);
    handleMotionNotifyEvent(event->event_x, event->event_y, event->root_x, event->root_y, modifiers,
                            event->time, QEvent::MouseMove);
}

static inline int fixed1616ToInt(xcb_input_fp1616_t val)
{
    return int(qreal(val) / 0x10000);
}

#define qt_xcb_mask_is_set(ptr, event) (((unsigned char*)(ptr))[(event)>>3] & (1 << ((event) & 7)))

void QXcbWindow::handleXIMouseEvent(xcb_ge_event_t *event, Qt::MouseEventSource source)
{
    QXcbConnection *conn = connection();
    auto *ev = reinterpret_cast<xcb_input_button_press_event_t *>(event);

    if (ev->buttons_len > 0) {
        unsigned char *buttonMask = (unsigned char *) &ev[1];
        // There is a bug in the evdev driver which leads to receiving mouse events without
        // XIPointerEmulated being set: https://bugs.freedesktop.org/show_bug.cgi?id=98188
        // Filter them out by other attributes: when their source device is a touch screen
        // and the LMB is pressed.
        if (qt_xcb_mask_is_set(buttonMask, 1) && conn->isTouchScreen(ev->sourceid)) {
            if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
                qCDebug(lcQpaXInput, "XI2 mouse event from touch device %d was ignored", ev->sourceid);
            return;
        }
        for (int i = 1; i <= 15; ++i)
            conn->setButtonState(conn->translateMouseButton(i), qt_xcb_mask_is_set(buttonMask, i));
    }

    const Qt::KeyboardModifiers modifiers = conn->keyboard()->translateModifiers(ev->mods.effective);
    const int event_x = fixed1616ToInt(ev->event_x);
    const int event_y = fixed1616ToInt(ev->event_y);
    const int root_x = fixed1616ToInt(ev->root_x);
    const int root_y = fixed1616ToInt(ev->root_y);

    conn->keyboard()->updateXKBStateFromXI(&ev->mods, &ev->group);

    const Qt::MouseButton button = conn->xiToQtMouseButton(ev->detail);

    const char *sourceName = nullptr;
    if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled())) {
        const QMetaObject *metaObject = qt_getEnumMetaObject(source);
        const QMetaEnum me = metaObject->enumerator(metaObject->indexOfEnumerator(qt_getEnumName(source)));
        sourceName = me.valueToKey(source);
    }

    switch (ev->event_type) {
    case XCB_INPUT_BUTTON_PRESS:
        if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
            qCDebug(lcQpaXInputEvents, "XI2 mouse press, button %d, time %d, source %s", button, ev->time, sourceName);
        conn->setButtonState(button, true);
        handleButtonPressEvent(event_x, event_y, root_x, root_y, ev->detail, modifiers, ev->time, QEvent::MouseButtonPress, source);
        break;
    case XCB_INPUT_BUTTON_RELEASE:
        if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
            qCDebug(lcQpaXInputEvents, "XI2 mouse release, button %d, time %d, source %s", button, ev->time, sourceName);
        conn->setButtonState(button, false);
        handleButtonReleaseEvent(event_x, event_y, root_x, root_y, ev->detail, modifiers, ev->time, QEvent::MouseButtonRelease, source);
        break;
    case XCB_INPUT_MOTION:
        if (Q_UNLIKELY(lcQpaXInputEvents().isDebugEnabled()))
            qCDebug(lcQpaXInputEvents, "XI2 mouse motion %d,%d, time %d, source %s", event_x, event_y, ev->time, sourceName);
        handleMotionNotifyEvent(event_x, event_y, root_x, root_y, modifiers, ev->time, QEvent::MouseMove, source);
        break;
    default:
        qWarning() << "Unrecognized XI2 mouse event" << ev->event_type;
        break;
    }
}

void QXcbWindow::handleXIEnterLeave(xcb_ge_event_t *event)
{
    auto *ev = reinterpret_cast<xcb_input_enter_event_t *>(event);

    // Compare the window with current mouse grabber to prevent deliver events to any other windows.
    // If leave event occurs and the window is under mouse - allow to deliver the leave event.
    QXcbWindow *mouseGrabber = connection()->mouseGrabber();
    if (mouseGrabber && mouseGrabber != this
            && (ev->event_type != XCB_INPUT_LEAVE || QGuiApplicationPrivate::currentMouseWindow != window())) {
        return;
    }

    const int root_x = fixed1616ToInt(ev->root_x);
    const int root_y = fixed1616ToInt(ev->root_y);

    switch (ev->event_type) {
    case XCB_INPUT_ENTER: {
        const int event_x = fixed1616ToInt(ev->event_x);
        const int event_y = fixed1616ToInt(ev->event_y);
        qCDebug(lcQpaXInputEvents, "XI2 mouse enter %d,%d, mode %d, detail %d, time %d",
                event_x, event_y, ev->mode, ev->detail, ev->time);
        handleEnterNotifyEvent(event_x, event_y, root_x, root_y, ev->mode, ev->detail, ev->time);
        break;
    }
    case XCB_INPUT_LEAVE:
        qCDebug(lcQpaXInputEvents, "XI2 mouse leave, mode %d, detail %d, time %d",
                ev->mode, ev->detail, ev->time);
        connection()->keyboard()->updateXKBStateFromXI(&ev->mods, &ev->group);
        handleLeaveNotifyEvent(root_x, root_y, ev->mode, ev->detail, ev->time);
        break;
    }
}

QXcbWindow *QXcbWindow::toWindow() { return this; }

void QXcbWindow::handleMouseEvent(xcb_timestamp_t time, const QPoint &local, const QPoint &global,
        Qt::KeyboardModifiers modifiers, QEvent::Type type, Qt::MouseEventSource source)
{
    m_lastPointerPosition = local;
    m_lastPointerGlobalPosition = global;
    connection()->setTime(time);
    Qt::MouseButton button = type == QEvent::MouseMove ? Qt::NoButton : connection()->button();
    QWindowSystemInterface::handleMouseEvent(window(), time, local, global,
                                             connection()->buttonState(), button,
                                             type, modifiers, source);
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

    if (event->atom == atom(QXcbAtom::Atom_NET_WM_STATE) || event->atom == atom(QXcbAtom::AtomWM_STATE)) {
        if (propertyDeleted)
            return;

        Qt::WindowStates newState = Qt::WindowNoState;

        if (event->atom == atom(QXcbAtom::AtomWM_STATE)) { // WM_STATE: Quick check for 'Minimize'.
            auto reply = Q_XCB_REPLY(xcb_get_property, xcb_connection(),
                                     0, m_window, atom(QXcbAtom::AtomWM_STATE),
                                     XCB_ATOM_ANY, 0, 1024);
            if (reply && reply->format == 32 && reply->type == atom(QXcbAtom::AtomWM_STATE)) {
                const quint32 *data = (const quint32 *)xcb_get_property_value(reply.get());
                if (reply->length != 0)
                    m_minimized = (data[0] == XCB_ICCCM_WM_STATE_ICONIC
                                   || (data[0] == XCB_ICCCM_WM_STATE_WITHDRAWN && m_minimized));
            }
        }

        const NetWmStates states = netWmStates();
        // _NET_WM_STATE_HIDDEN should be set by the Window Manager to indicate that a window would
        // not be visible on the screen if its desktop/viewport were active and its coordinates were
        // within the screen bounds. The canonical example is that minimized windows should be in
        // the _NET_WM_STATE_HIDDEN state.
        if (m_minimized && (!connection()->wmSupport()->isSupportedByWM(NetWmStateHidden)
                            || states.testFlag(NetWmStateHidden)))
            newState = Qt::WindowMinimized;

        if (states & NetWmStateFullScreen)
            newState |= Qt::WindowFullScreen;
        if ((states & NetWmStateMaximizedHorz) && (states & NetWmStateMaximizedVert))
            newState |= Qt::WindowMaximized;
        // Send Window state, compress events in case other flags (modality, etc) are changed.
        if (m_lastWindowStateEvent != newState) {
            QWindowSystemInterface::handleWindowStateChanged(window(), newState);
            m_lastWindowStateEvent = newState;
            m_windowState = newState;
            if ((m_windowState & Qt::WindowMinimized) && connection()->mouseGrabber() == this)
                connection()->setMouseGrabber(nullptr);
        }
        return;
    } else if (event->atom == atom(QXcbAtom::Atom_NET_FRAME_EXTENTS)) {
        m_dirtyFrameMargins = true;
    }
}

void QXcbWindow::handleFocusInEvent(const xcb_focus_in_event_t *event)
{
    // Ignore focus events that are being sent only because the pointer is over
    // our window, even if the input focus is in a different window.
    if (event->detail == XCB_NOTIFY_DETAIL_POINTER)
        return;

    connection()->focusInTimer().stop();
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
    if (connection()->hasXSync() && (m_syncValue.lo != 0 || m_syncValue.hi != 0)) {
        xcb_sync_set_counter(xcb_connection(), m_syncCounter, m_syncValue);
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

    auto reply = Q_XCB_REPLY(xcb_grab_keyboard, xcb_connection(), false,
                             m_window, XCB_TIME_CURRENT_TIME,
                             XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    return reply && reply->status == XCB_GRAB_STATUS_SUCCESS;
}

bool QXcbWindow::setMouseGrabEnabled(bool grab)
{
    if (!grab && connection()->mouseGrabber() == this)
        connection()->setMouseGrabber(nullptr);

    if (grab && !connection()->canGrab())
        return false;

    if (connection()->hasXInput2()) {
        bool result = connection()->xi2SetMouseGrabEnabled(m_window, grab);
        if (grab && result)
            connection()->setMouseGrabber(this);
        return result;
    }

    if (!grab) {
        xcb_ungrab_pointer(xcb_connection(), XCB_TIME_CURRENT_TIME);
        return true;
    }

    auto reply = Q_XCB_REPLY(xcb_grab_pointer, xcb_connection(),
                             false, m_window,
                             (XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
                              | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_ENTER_WINDOW
                              | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION),
                             XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
                             XCB_WINDOW_NONE, XCB_CURSOR_NONE,
                             XCB_TIME_CURRENT_TIME);
    bool result = reply && reply->status == XCB_GRAB_STATUS_SUCCESS;
    if (result)
        connection()->setMouseGrabber(this);
    return result;
}

bool QXcbWindow::windowEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::FocusIn:
        if (m_embedded && !m_trayIconWindow && !event->spontaneous()) {
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
    return QPlatformWindow::windowEvent(event);
}

bool QXcbWindow::startSystemResize(Qt::Edges edges)
{
    return startSystemMoveResize(m_lastPointerPosition, edges);
}

bool QXcbWindow::startSystemMove()
{
    return startSystemMoveResize(m_lastPointerPosition, 16);
}

bool QXcbWindow::startSystemMoveResize(const QPoint &pos, int edges)
{
    const xcb_atom_t moveResize = connection()->atom(QXcbAtom::Atom_NET_WM_MOVERESIZE);
    if (!connection()->wmSupport()->isSupportedByWM(moveResize))
        return false;

    // ### FIXME QTBUG-53389
    bool startedByTouch = connection()->startSystemMoveResizeForTouch(m_window, edges);
    if (startedByTouch) {
        const QString wmname = connection()->windowManagerName();
        if (wmname != "kwin"_L1 && wmname != "openbox"_L1) {
            qCDebug(lcQpaXInputDevices) << "only KDE and OpenBox support startSystemMove/Resize which is triggered from touch events: XDG_CURRENT_DESKTOP="
                                        << qgetenv("XDG_CURRENT_DESKTOP");
            connection()->abortSystemMoveResize(m_window);
            return false;
        }
        // KWin, Openbox, AwesomeWM and Gnome have been tested to work with _NET_WM_MOVERESIZE.
    } else { // Started by mouse press.
        doStartSystemMoveResize(mapToGlobal(pos), edges);
    }

    return true;
}

static uint qtEdgesToXcbMoveResizeDirection(Qt::Edges edges)
{
    if (edges == (Qt::TopEdge | Qt::LeftEdge))
        return 0;
    if (edges == Qt::TopEdge)
        return 1;
    if (edges == (Qt::TopEdge | Qt::RightEdge))
        return 2;
    if (edges == Qt::RightEdge)
        return 3;
    if (edges == (Qt::RightEdge | Qt::BottomEdge))
        return 4;
    if (edges == Qt::BottomEdge)
        return 5;
    if (edges == (Qt::BottomEdge | Qt::LeftEdge))
        return 6;
    if (edges == Qt::LeftEdge)
        return 7;

    qWarning() << "Cannot convert " << edges << "to _NET_WM_MOVERESIZE direction.";
    return 0;
}

void QXcbWindow::doStartSystemMoveResize(const QPoint &globalPos, int edges)
{
    qCDebug(lcQpaXInputDevices) << "triggered system move or resize via sending _NET_WM_MOVERESIZE client message";
    const xcb_atom_t moveResize = connection()->atom(QXcbAtom::Atom_NET_WM_MOVERESIZE);
    xcb_client_message_event_t xev;
    xev.response_type = XCB_CLIENT_MESSAGE;
    xev.type = moveResize;
    xev.sequence = 0;
    xev.window = xcb_window();
    xev.format = 32;
    xev.data.data32[0] = globalPos.x();
    xev.data.data32[1] = globalPos.y();
    if (edges == 16)
        xev.data.data32[2] = 8; // move
    else
        xev.data.data32[2] = qtEdgesToXcbMoveResizeDirection(Qt::Edges(edges));
    xev.data.data32[3] = XCB_BUTTON_INDEX_1;
    xev.data.data32[4] = 0;
    xcb_ungrab_pointer(connection()->xcb_connection(), XCB_CURRENT_TIME);
    xcb_send_event(connection()->xcb_connection(), false, xcbScreen()->root(),
                   XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY,
                   (const char *)&xev);

    connection()->setDuringSystemMoveResize(true);
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
    event.type = atom(QXcbAtom::Atom_XEMBED);
    event.data.data32[0] = connection()->time();
    event.data.data32[1] = message;
    event.data.data32[2] = detail;
    event.data.data32[3] = data1;
    event.data.data32[4] = data2;
    xcb_send_event(xcb_connection(), false, window, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
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
        xcb_map_window(xcb_connection(), m_window);
        xcbScreen()->windowShown(this);
        break;
    case XEMBED_FOCUS_IN:
        connection()->focusInTimer().stop();
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
        connection()->setFocusWindow(window());
        QWindowSystemInterface::handleWindowActivated(window(), reason);
        break;
    case XEMBED_FOCUS_OUT:
        if (window() == QGuiApplication::focusWindow()
            && !activeWindowChangeQueued(window())) {
            connection()->setFocusWindow(nullptr);
            QWindowSystemInterface::handleWindowActivated(nullptr);
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

    xcb_change_property(xcb_connection(),
                        XCB_PROP_MODE_REPLACE,
                        m_window,
                        atom(QXcbAtom::Atom_NET_WM_WINDOW_OPACITY),
                        XCB_ATOM_CARDINAL,
                        32,
                        1,
                        (uchar *)&value);
}

QList<xcb_rectangle_t> qRegionToXcbRectangleList(const QRegion &region)
{
    QList<xcb_rectangle_t> rects;
    rects.reserve(region.rectCount());
    for (const QRect &r : region)
        rects.push_back(qRectToXCBRectangle(r));
    return rects;
}

void QXcbWindow::setMask(const QRegion &region)
{
    if (!connection()->hasXShape())
        return;
    if (region.isEmpty()) {
        xcb_shape_mask(connection()->xcb_connection(), XCB_SHAPE_SO_SET,
                       XCB_SHAPE_SK_BOUNDING, xcb_window(), 0, 0, XCB_NONE);
    } else {
        const auto rects = qRegionToXcbRectangleList(region);
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

    setNetWmState(enabled, atom(QXcbAtom::Atom_NET_WM_STATE_DEMANDS_ATTENTION));
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

void QXcbWindow::setWindowTitle(const QXcbConnection *conn, xcb_window_t window, const QString &title)
{
    QString fullTitle = formatWindowTitle(title, QString::fromUtf8(" \xe2\x80\x94 ")); // unicode character U+2014, EM DASH
    const QByteArray ba = std::move(fullTitle).toUtf8();
    xcb_change_property(conn->xcb_connection(),
                        XCB_PROP_MODE_REPLACE,
                        window,
                        conn->atom(QXcbAtom::Atom_NET_WM_NAME),
                        conn->atom(QXcbAtom::AtomUTF8_STRING),
                        8,
                        ba.size(),
                        ba.constData());

#if QT_CONFIG(xcb_xlib)
    Display *dpy = static_cast<Display *>(conn->xlib_display());
    XTextProperty *text = qstringToXTP(dpy, title);
    if (text)
        XSetWMName(dpy, window, text);
#endif
    xcb_flush(conn->xcb_connection());
}

QString QXcbWindow::windowTitle(const QXcbConnection *conn, xcb_window_t window)
{
    const xcb_atom_t utf8Atom = conn->atom(QXcbAtom::AtomUTF8_STRING);
    auto reply = Q_XCB_REPLY_UNCHECKED(xcb_get_property, conn->xcb_connection(),
                                       false, window, conn->atom(QXcbAtom::Atom_NET_WM_NAME),
                                       utf8Atom, 0, 1024);
    if (reply && reply->format == 8 && reply->type == utf8Atom) {
        const char *name = reinterpret_cast<const char *>(xcb_get_property_value(reply.get()));
        return QString::fromUtf8(name, xcb_get_property_value_length(reply.get()));
    }

    reply = Q_XCB_REPLY_UNCHECKED(xcb_get_property, conn->xcb_connection(),
                                  false, window, conn->atom(QXcbAtom::AtomWM_NAME),
                                  XCB_ATOM_STRING, 0, 1024);
    if (reply && reply->format == 8 && reply->type == XCB_ATOM_STRING) {
        const char *name = reinterpret_cast<const char *>(xcb_get_property_value(reply.get()));
        return QString::fromLatin1(name, xcb_get_property_value_length(reply.get()));
    }

    return QString();
}

QT_END_NAMESPACE

