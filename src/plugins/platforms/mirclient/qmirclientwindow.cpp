/****************************************************************************
**
** Copyright (C) 2014-2015 Canonical, Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


// Local
#include "qmirclientclipboard.h"
#include "qmirclientinput.h"
#include "qmirclientwindow.h"
#include "qmirclientscreen.h"
#include "qmirclientlogging.h"

// Qt
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface.h>
#include <QMutex>
#include <QMutexLocker>
#include <QSize>
#include <QtMath>

// Platform API
#include <ubuntu/application/instance.h>

#include <EGL/egl.h>

#define IS_OPAQUE_FLAG 1

namespace
{
MirSurfaceState qtWindowStateToMirSurfaceState(Qt::WindowState state)
{
    switch (state) {
    case Qt::WindowNoState:
        return mir_surface_state_restored;

    case Qt::WindowFullScreen:
        return mir_surface_state_fullscreen;

    case Qt::WindowMaximized:
        return mir_surface_state_maximized;

    case Qt::WindowMinimized:
        return mir_surface_state_minimized;

    default:
        LOG("Unexpected Qt::WindowState: %d", state);
        return mir_surface_state_restored;
    }
}

#if !defined(QT_NO_DEBUG)
const char *qtWindowStateToStr(Qt::WindowState state)
{
    switch (state) {
    case Qt::WindowNoState:
        return "NoState";

    case Qt::WindowFullScreen:
        return "FullScreen";

    case Qt::WindowMaximized:
        return "Maximized";

    case Qt::WindowMinimized:
        return "Minimized";

    default:
        return "!?";
    }
}
#endif

} // anonymous namespace

class QMirClientWindowPrivate
{
public:
    void createEGLSurface(EGLNativeWindowType nativeWindow);
    void destroyEGLSurface();
    int panelHeight();

    QMirClientScreen* screen;
    EGLSurface eglSurface;
    WId id;
    QMirClientInput* input;
    Qt::WindowState state;
    MirConnection *connection;
    MirSurface* surface;
    QSize bufferSize;
    QMutex mutex;
    QSharedPointer<QMirClientClipboard> clipboard;
};

static void eventCallback(MirSurface* surface, const MirEvent *event, void* context)
{
    (void) surface;
    DASSERT(context != NULL);
    QMirClientWindow* platformWindow = static_cast<QMirClientWindow*>(context);
    platformWindow->priv()->input->postEvent(platformWindow, event);
}

static void surfaceCreateCallback(MirSurface* surface, void* context)
{
    DASSERT(context != NULL);
    QMirClientWindow* platformWindow = static_cast<QMirClientWindow*>(context);
    platformWindow->priv()->surface = surface;

    mir_surface_set_event_handler(surface, eventCallback, context);
}

QMirClientWindow::QMirClientWindow(QWindow* w, QSharedPointer<QMirClientClipboard> clipboard, QMirClientScreen* screen,
                           QMirClientInput* input, MirConnection* connection)
    : QObject(nullptr), QPlatformWindow(w)
{
    DASSERT(screen != NULL);

    d = new QMirClientWindowPrivate;
    d->screen = screen;
    d->eglSurface = EGL_NO_SURFACE;
    d->input = input;
    d->state = window()->windowState();
    d->connection = connection;
    d->clipboard = clipboard;

    static int id = 1;
    d->id = id++;

    // Use client geometry if set explicitly, use available screen geometry otherwise.
    QPlatformWindow::setGeometry(window()->geometry().isValid() &&  window()->geometry() != screen->geometry() ?
        window()->geometry() : screen->availableGeometry());
    createWindow();
    DLOG("QMirClientWindow::QMirClientWindow (this=%p, w=%p, screen=%p, input=%p)", this, w, screen, input);
}

QMirClientWindow::~QMirClientWindow()
{
    DLOG("QMirClientWindow::~QMirClientWindow");
    d->destroyEGLSurface();

    mir_surface_release_sync(d->surface);

    delete d;
}

void QMirClientWindowPrivate::createEGLSurface(EGLNativeWindowType nativeWindow)
{
  DLOG("QMirClientWindowPrivate::createEGLSurface (this=%p, nativeWindow=%p)",
          this, reinterpret_cast<void*>(nativeWindow));

  eglSurface = eglCreateWindowSurface(screen->eglDisplay(), screen->eglConfig(),
          nativeWindow, nullptr);

  DASSERT(eglSurface != EGL_NO_SURFACE);
}

void QMirClientWindowPrivate::destroyEGLSurface()
{
    DLOG("QMirClientWindowPrivate::destroyEGLSurface (this=%p)", this);
    if (eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(screen->eglDisplay(), eglSurface);
        eglSurface = EGL_NO_SURFACE;
    }
}

// FIXME - in order to work around https://bugs.launchpad.net/mir/+bug/1346633
// we need to guess the panel height (3GU + 2DP)
int QMirClientWindowPrivate::panelHeight()
{
    if (qEnvironmentVariableIsSet("QT_MIRCLIENT_IGNORE_PANEL"))
        return 0;
    const int defaultGridUnit = 8;
    int gridUnit = defaultGridUnit;
    QByteArray gridUnitString = qgetenv("GRID_UNIT_PX");
    if (!gridUnitString.isEmpty()) {
        bool ok;
        gridUnit = gridUnitString.toInt(&ok);
        if (!ok) {
            gridUnit = defaultGridUnit;
        }
    }
    qreal densityPixelRatio = static_cast<qreal>(gridUnit) / defaultGridUnit;
    return gridUnit * 3 + qFloor(densityPixelRatio) * 2;
}

namespace
{
static MirPixelFormat
mir_choose_default_pixel_format(MirConnection *connection)
{
    MirPixelFormat format[mir_pixel_formats];
    unsigned int nformats;

    mir_connection_get_available_surface_formats(connection,
        format, mir_pixel_formats, &nformats);

    return format[0];
}
}

void QMirClientWindow::createWindow()
{
    DLOG("QMirClientWindow::createWindow (this=%p)", this);

    // FIXME: remove this remnant of an old platform-api enum - needs ubuntu-keyboard update
    const int SCREEN_KEYBOARD_ROLE = 7;
    // Get surface role and flags.
    QVariant roleVariant = window()->property("role");
    int role = roleVariant.isValid() ? roleVariant.toUInt() : 1;  // 1 is the default role for apps.
    QVariant opaqueVariant = window()->property("opaque");
    uint flags = opaqueVariant.isValid() ?
        opaqueVariant.toUInt() ? static_cast<uint>(IS_OPAQUE_FLAG) : 0 : 0;

    // FIXME(loicm) Opaque flag is forced for now for non-system sessions (applications) for
    //     performance reasons.
    flags |= static_cast<uint>(IS_OPAQUE_FLAG);

    const QByteArray title = (!window()->title().isNull()) ? window()->title().toUtf8() : "Window 1"; // legacy title
    const int panelHeight = d->panelHeight();

#if !defined(QT_NO_DEBUG)
    LOG("panelHeight: '%d'", panelHeight);
    LOG("role: '%d'", role);
    LOG("flags: '%s'", (flags & static_cast<uint>(1)) ? "Opaque" : "NotOpaque");
    LOG("title: '%s'", title.constData());
#endif

    // Get surface geometry.
    QRect geometry;
    if (d->state == Qt::WindowFullScreen) {
        printf("QMirClientWindow - fullscreen geometry\n");
        geometry = screen()->geometry();
    } else if (d->state == Qt::WindowMaximized) {
        printf("QMirClientWindow - maximized geometry\n");
        geometry = screen()->availableGeometry();
        /*
         * FIXME: Autopilot relies on being able to convert coordinates relative of the window
         * into absolute screen coordinates. Mir does not allow this, see bug lp:1346633
         * Until there's a correct way to perform this transformation agreed, this horrible hack
         * guesses the transformation heuristically.
         *
         * Assumption: this method only used on phone devices!
         */
        geometry.setY(panelHeight);
    } else {
        printf("QMirClientWindow - regular geometry\n");
        geometry = this->geometry();
        geometry.setY(panelHeight);
    }

    DLOG("[ubuntumirclient QPA] creating surface at (%d, %d) with size (%d, %d) with title '%s'\n",
            geometry.x(), geometry.y(), geometry.width(), geometry.height(), title.data());

    MirSurfaceSpec *spec;
    if (role == SCREEN_KEYBOARD_ROLE)
    {
        spec = mir_connection_create_spec_for_input_method(d->connection, geometry.width(),
            geometry.height(), mir_choose_default_pixel_format(d->connection));
    }
    else
    {
        spec = mir_connection_create_spec_for_normal_surface(d->connection, geometry.width(),
            geometry.height(), mir_choose_default_pixel_format(d->connection));
    }
    mir_surface_spec_set_name(spec, title.data());

    // Create platform window
    mir_wait_for(mir_surface_create(spec, surfaceCreateCallback, this));
    mir_surface_spec_release(spec);

    DASSERT(d->surface != NULL);
    d->createEGLSurface((EGLNativeWindowType)mir_buffer_stream_get_egl_native_window(mir_surface_get_buffer_stream(d->surface)));

    if (d->state == Qt::WindowFullScreen) {
    // TODO: We could set this on creation once surface spec supports it (mps already up)
        mir_wait_for(mir_surface_set_state(d->surface, mir_surface_state_fullscreen));
    }

    // Window manager can give us a final size different from what we asked for
    // so let's check what we ended up getting
    {
        MirSurfaceParameters parameters;
        mir_surface_get_parameters(d->surface, &parameters);

        geometry.setWidth(parameters.width);
        geometry.setHeight(parameters.height);
    }

    DLOG("[ubuntumirclient QPA] created surface has size (%d, %d)",
            geometry.width(), geometry.height());

    // Assume that the buffer size matches the surface size at creation time
    d->bufferSize = geometry.size();

    // Tell Qt about the geometry.
    QWindowSystemInterface::handleGeometryChange(window(), geometry);
    QPlatformWindow::setGeometry(geometry);
}

void QMirClientWindow::moveResize(const QRect& rect)
{
    (void) rect;
    // TODO: Not yet supported by mir.
}

void QMirClientWindow::handleSurfaceResize(int width, int height)
{
    QMutexLocker(&d->mutex);
    LOG("QMirClientWindow::handleSurfaceResize(width=%d, height=%d)", width, height);

    // The current buffer size hasn't actually changed. so just render on it and swap
    // buffers in the hope that the next buffer will match the surface size advertised
    // in this event.
    // But since this event is processed by a thread different from the one that swaps
    // buffers, you can never know if this information is already outdated as there's
    // no synchronicity whatsoever between the processing of resize events and the
    // consumption of buffers.
    if (d->bufferSize.width() != width || d->bufferSize.height() != height) {
        QWindowSystemInterface::handleExposeEvent(window(), geometry());
        QWindowSystemInterface::flushWindowSystemEvents();
    }
}

void QMirClientWindow::handleSurfaceFocusChange(bool focused)
{
    LOG("QMirClientWindow::handleSurfaceFocusChange(focused=%s)", focused ? "true" : "false");
    QWindow *activatedWindow = focused ? window() : nullptr;

    // System clipboard contents might have changed while this window was unfocused and wihtout
    // this process getting notified about it because it might have been suspended (due to
    // application lifecycle policies), thus unable to listen to any changes notified through
    // D-Bus.
    // Therefore let's ensure we are up to date with the system clipboard now that we are getting
    // focused again.
    if (focused) {
        d->clipboard->requestDBusClipboardContents();
    }

    QWindowSystemInterface::handleWindowActivated(activatedWindow, Qt::ActiveWindowFocusReason);
}

void QMirClientWindow::setWindowState(Qt::WindowState state)
{
    QMutexLocker(&d->mutex);
    DLOG("QMirClientWindow::setWindowState (this=%p, %s)", this,  qtWindowStateToStr(state));

    if (state == d->state)
        return;

    // TODO: Perhaps we should check if the states are applied?
    mir_wait_for(mir_surface_set_state(d->surface, qtWindowStateToMirSurfaceState(state)));
    d->state = state;
}

void QMirClientWindow::setGeometry(const QRect& rect)
{
    DLOG("QMirClientWindow::setGeometry (this=%p)", this);

    bool doMoveResize;

    {
        QMutexLocker(&d->mutex);
        QPlatformWindow::setGeometry(rect);
        doMoveResize = d->state != Qt::WindowFullScreen && d->state != Qt::WindowMaximized;
    }

    if (doMoveResize) {
        moveResize(rect);
    }
}

void QMirClientWindow::setVisible(bool visible)
{
    QMutexLocker(&d->mutex);
    DLOG("QMirClientWindow::setVisible (this=%p, visible=%s)", this, visible ? "true" : "false");

    if (visible) {
        mir_wait_for(mir_surface_set_state(d->surface, qtWindowStateToMirSurfaceState(d->state)));

        QWindowSystemInterface::handleExposeEvent(window(), QRect());
        QWindowSystemInterface::flushWindowSystemEvents();
    } else {
        // TODO: Use the new mir_surface_state_hidden state instead of mir_surface_state_minimized.
        //       Will have to change qtmir and unity8 for that.
        mir_wait_for(mir_surface_set_state(d->surface, mir_surface_state_minimized));
    }
}

void* QMirClientWindow::eglSurface() const
{
    return d->eglSurface;
}

WId QMirClientWindow::winId() const
{
    return d->id;
}

void QMirClientWindow::onBuffersSwapped_threadSafe(int newBufferWidth, int newBufferHeight)
{
    QMutexLocker(&d->mutex);

    bool sizeKnown = newBufferWidth > 0 && newBufferHeight > 0;

    if (sizeKnown && (d->bufferSize.width() != newBufferWidth ||
                d->bufferSize.height() != newBufferHeight)) {

        DLOG("QMirClientWindow::onBuffersSwapped_threadSafe - buffer size changed from (%d,%d) to (%d,%d)",
                d->bufferSize.width(), d->bufferSize.height(), newBufferWidth, newBufferHeight);

        d->bufferSize.rwidth() = newBufferWidth;
        d->bufferSize.rheight() = newBufferHeight;

        QRect newGeometry;

        newGeometry = geometry();
        newGeometry.setWidth(d->bufferSize.width());
        newGeometry.setHeight(d->bufferSize.height());

        QPlatformWindow::setGeometry(newGeometry);
        QWindowSystemInterface::handleGeometryChange(window(), newGeometry, QRect());
    }
}
