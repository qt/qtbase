/*
 * Copyright (C) 2014-2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Local
#include "clipboard.h"
#include "input.h"
#include "window.h"
#include "screen.h"
#include "logging.h"

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

class UbuntuWindowPrivate
{
public:
    void createEGLSurface(EGLNativeWindowType nativeWindow);
    void destroyEGLSurface();
    int panelHeight();

    UbuntuScreen* screen;
    EGLSurface eglSurface;
    WId id;
    UbuntuInput* input;
    Qt::WindowState state;
    MirConnection *connection;
    MirSurface* surface;
    QSize bufferSize;
    QSize targetBufferSize;
    QMutex mutex;
    QSharedPointer<UbuntuClipboard> clipboard;
};

static void eventCallback(MirSurface* surface, const MirEvent *event, void* context)
{
    (void) surface;
    DASSERT(context != NULL);
    UbuntuWindow* platformWindow = static_cast<UbuntuWindow*>(context);
    platformWindow->priv()->input->postEvent(platformWindow, event);
}

static void surfaceCreateCallback(MirSurface* surface, void* context)
{
    DASSERT(context != NULL);
    UbuntuWindow* platformWindow = static_cast<UbuntuWindow*>(context);
    platformWindow->priv()->surface = surface;

    mir_surface_set_event_handler(surface, eventCallback, context);
}

UbuntuWindow::UbuntuWindow(QWindow* w, QSharedPointer<UbuntuClipboard> clipboard, UbuntuScreen* screen,
                           UbuntuInput* input, MirConnection* connection)
    : QObject(nullptr), QPlatformWindow(w)
{
    DASSERT(screen != NULL);

    d = new UbuntuWindowPrivate;
    d->screen = screen;
    d->eglSurface = EGL_NO_SURFACE;
    d->input = input;
    d->state = window()->windowState();
    d->connection = connection;
    d->clipboard = clipboard;

    static int id = 1;
    d->id = id++;

    // Use client geometry if set explicitly, use available screen geometry otherwise.
    QPlatformWindow::setGeometry(window()->geometry() != screen->geometry() ?
        window()->geometry() : screen->availableGeometry());
    createWindow();
    DLOG("UbuntuWindow::UbuntuWindow (this=%p, w=%p, screen=%p, input=%p)", this, w, screen, input);
}

UbuntuWindow::~UbuntuWindow()
{
    DLOG("UbuntuWindow::~UbuntuWindow");
    d->destroyEGLSurface();

    mir_surface_release_sync(d->surface);

    delete d;
}

void UbuntuWindowPrivate::createEGLSurface(EGLNativeWindowType nativeWindow)
{
  DLOG("UbuntuWindowPrivate::createEGLSurface (this=%p, nativeWindow=%p)",
          this, reinterpret_cast<void*>(nativeWindow));

  eglSurface = eglCreateWindowSurface(screen->eglDisplay(), screen->eglConfig(),
          nativeWindow, nullptr);

  DASSERT(eglSurface != EGL_NO_SURFACE);
}

void UbuntuWindowPrivate::destroyEGLSurface()
{
    DLOG("UbuntuWindowPrivate::destroyEGLSurface (this=%p)", this);
    if (eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(screen->eglDisplay(), eglSurface);
        eglSurface = EGL_NO_SURFACE;
    }
}

// FIXME - in order to work around https://bugs.launchpad.net/mir/+bug/1346633
// we need to guess the panel height (3GU + 2DP)
int UbuntuWindowPrivate::panelHeight()
{
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

void UbuntuWindow::createWindow()
{
    DLOG("UbuntuWindow::createWindow (this=%p)", this);

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
        printf("UbuntuWindow - fullscreen geometry\n");
        geometry = screen()->geometry();
    } else if (d->state == Qt::WindowMaximized) {
        printf("UbuntuWindow - maximized geometry\n");
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
        printf("UbuntuWindow - regular geometry\n");
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

void UbuntuWindow::moveResize(const QRect& rect)
{
    (void) rect;
    // TODO: Not yet supported by mir.
}

void UbuntuWindow::handleSurfaceResize(int width, int height)
{
    QMutexLocker(&d->mutex);
    LOG("UbuntuWindow::handleSurfaceResize(width=%d, height=%d)", width, height);

    // The current buffer size hasn't actually changed. so just render on it and swap
    // buffers until we render on a buffer with the target size.

    d->targetBufferSize.rwidth() = width;
    d->targetBufferSize.rheight() = height;

    if (d->bufferSize != d->targetBufferSize) {
        QWindowSystemInterface::handleExposeEvent(window(), geometry());
    } else {
        qWarning("[ubuntumirclient QPA] UbuntuWindow::handleSurfaceResize"
                 " current buffer already has the target size");
        d->targetBufferSize = QSize();
    }
}

void UbuntuWindow::handleSurfaceFocusChange(bool focused)
{
    LOG("UbuntuWindow::handleSurfaceFocusChange(focused=%s)", focused ? "true" : "false");
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

void UbuntuWindow::setWindowState(Qt::WindowState state)
{
    QMutexLocker(&d->mutex);
    DLOG("UbuntuWindow::setWindowState (this=%p, %s)", this,  qtWindowStateToStr(state));

    if (state == d->state)
        return;

    // TODO: Perhaps we should check if the states are applied?
    mir_wait_for(mir_surface_set_state(d->surface, qtWindowStateToMirSurfaceState(state)));
    d->state = state;
}

void UbuntuWindow::setGeometry(const QRect& rect)
{
    DLOG("UbuntuWindow::setGeometry (this=%p)", this);

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

void UbuntuWindow::setVisible(bool visible)
{
    QMutexLocker(&d->mutex);
    DLOG("UbuntuWindow::setVisible (this=%p, visible=%s)", this, visible ? "true" : "false");

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

void* UbuntuWindow::eglSurface() const
{
    return d->eglSurface;
}

WId UbuntuWindow::winId() const
{
    return d->id;
}

void UbuntuWindow::onBuffersSwapped_threadSafe(int newBufferWidth, int newBufferHeight)
{
    QMutexLocker(&d->mutex);

    bool sizeKnown = newBufferWidth > 0 && newBufferHeight > 0;

    if (sizeKnown && (d->bufferSize.width() != newBufferWidth ||
                d->bufferSize.height() != newBufferHeight)) {

        DLOG("UbuntuWindow::onBuffersSwapped_threadSafe - buffer size changed from (%d,%d) to (%d,%d)",
                d->bufferSize.width(), d->bufferSize.height(), newBufferWidth, newBufferHeight);

        d->bufferSize.rwidth() = newBufferWidth;
        d->bufferSize.rheight() = newBufferHeight;

        QRect newGeometry;

        newGeometry = geometry();
        newGeometry.setWidth(d->bufferSize.width());
        newGeometry.setHeight(d->bufferSize.height());

        QPlatformWindow::setGeometry(newGeometry);
        QWindowSystemInterface::handleGeometryChange(window(), newGeometry, QRect());
        QWindowSystemInterface::handleExposeEvent(window(), newGeometry);

    } else {
        // buffer size hasn't changed
        if (d->targetBufferSize.isValid()) {
            if (d->bufferSize != d->targetBufferSize) {
                // but we still didn't reach the promised buffer size from the mir resize event.
                // thus keep swapping buffers
                QWindowSystemInterface::handleExposeEvent(window(), geometry());
            } else {
                // target met. we have just provided a render with the target size and
                // can therefore finally rest.
                d->targetBufferSize = QSize();
            }
        }
    }
}
