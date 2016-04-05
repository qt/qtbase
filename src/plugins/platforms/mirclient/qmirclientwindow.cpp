/****************************************************************************
**
** Copyright (C) 2014-2015 Canonical, Ltd.
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


// Local
#include "qmirclientwindow.h"
#include "qmirclientclipboard.h"
#include "qmirclientinput.h"
#include "qmirclientscreen.h"
#include "qmirclientlogging.h"

#include <mir_toolkit/mir_client_library.h>

// Qt
#include <qpa/qwindowsysteminterface.h>
#include <QMutexLocker>
#include <QSize>
#include <QtMath>

// Platform API
#include <ubuntu/application/instance.h>

#include <EGL/egl.h>

namespace
{

// FIXME: this used to be defined by platform-api, but it's been removed in v3. Change ubuntu-keyboard to use
// a different enum for window roles.
enum UAUiWindowRole {
    U_MAIN_ROLE = 1,
    U_DASH_ROLE,
    U_INDICATOR_ROLE,
    U_NOTIFICATIONS_ROLE,
    U_GREETER_ROLE,
    U_LAUNCHER_ROLE,
    U_ON_SCREEN_KEYBOARD_ROLE,
    U_SHUTDOWN_DIALOG_ROLE,
};

struct MirSpecDeleter
{
    void operator()(MirSurfaceSpec *spec) { mir_surface_spec_release(spec); }
};

using Spec = std::unique_ptr<MirSurfaceSpec, MirSpecDeleter>;

EGLNativeWindowType nativeWindowFor(MirSurface *surf)
{
    auto stream = mir_surface_get_buffer_stream(surf);
    return reinterpret_cast<EGLNativeWindowType>(mir_buffer_stream_get_egl_native_window(stream));
}

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

WId makeId()
{
    static int id = 1;
    return id++;
}

MirPixelFormat defaultPixelFormatFor(MirConnection *connection)
{
    MirPixelFormat format;
    unsigned int nformats;
    mir_connection_get_available_surface_formats(connection, &format, 1, &nformats);
    return format;
}

UAUiWindowRole roleFor(QWindow *window)
{
    QVariant roleVariant = window->property("role");
    if (!roleVariant.isValid())
        return U_MAIN_ROLE;

    uint role = roleVariant.toUInt();
    if (role < U_MAIN_ROLE || role > U_SHUTDOWN_DIALOG_ROLE)
        return U_MAIN_ROLE;

    return static_cast<UAUiWindowRole>(role);
}

QMirClientWindow *transientParentFor(QWindow *window)
{
    QWindow *parent = window->transientParent();
    return parent ? static_cast<QMirClientWindow *>(parent->handle()) : nullptr;
}

Spec makeSurfaceSpec(QWindow *window, QMirClientInput *input, MirConnection *connection)
{
   const auto geom = window->geometry();
   const int width = geom.width() > 0 ? geom.width() : 1;
   const int height = geom.height() > 0 ? geom.height() : 1;
   const auto pixelFormat = defaultPixelFormatFor(connection);

   if (U_ON_SCREEN_KEYBOARD_ROLE == roleFor(window)) {
       DLOG("[ubuntumirclient QPA] makeSurfaceSpec(window=%p) - creating input method surface (width=%d, height=%d", window, width, height);
       return Spec{mir_connection_create_spec_for_input_method(connection, width, height, pixelFormat)};
   }

   const Qt::WindowType type = window->type();
   if (type == Qt::Popup) {
       auto parent = transientParentFor(window);
       if (parent == nullptr) {
           //NOTE: We cannot have a parentless popup -
           //try using the last surface to receive input as that will most likely be
           //the one that caused this popup to be created
           parent = input->lastFocusedWindow();
       }
       if (parent) {
           auto pos = geom.topLeft();
           pos -= parent->geometry().topLeft();
           MirRectangle location{pos.x(), pos.y(), 0, 0};
           DLOG("[ubuntumirclient QPA] makeSurfaceSpec(window=%p) - creating menu surface(width:%d, height:%d)", window, width, height);
           return Spec{mir_connection_create_spec_for_menu(
                       connection, width, height, pixelFormat, parent->mirSurface(),
                       &location, mir_edge_attachment_any)};
       } else {
           DLOG("[ubuntumirclient QPA] makeSurfaceSpec(window=%p) - cannot create a menu without a parent!", window);
       }
   } else if (type == Qt::Dialog) {
       auto parent = transientParentFor(window);
       if (parent) {
           // Modal dialog
           DLOG("[ubuntumirclient QPA] makeSurfaceSpec(window=%p) - creating modal dialog (width=%d, height=%d", window, width, height);
           return Spec{mir_connection_create_spec_for_modal_dialog(connection, width, height, pixelFormat, parent->mirSurface())};
       } else {
           // TODO: do Qt parentless dialogs have the same semantics as mir?
           DLOG("[ubuntumirclient QPA] makeSurfaceSpec(window=%p) - creating parentless dialog (width=%d, height=%d)", window, width, height);
           return Spec{mir_connection_create_spec_for_dialog(connection, width, height, pixelFormat)};
       }
   }
   DLOG("[ubuntumirclient QPA] makeSurfaceSpec(window=%p) - creating normal surface(type=0x%x, width=%d, height=%d)", window, type, width, height);
   return Spec{mir_connection_create_spec_for_normal_surface(connection, width, height, pixelFormat)};
}

void setSizingConstraints(MirSurfaceSpec *spec, const QSize& minSize, const QSize& maxSize, const QSize& increment)
{
    mir_surface_spec_set_min_width(spec, minSize.width());
    mir_surface_spec_set_min_height(spec, minSize.height());
    if (maxSize.width() >= minSize.width()) {
        mir_surface_spec_set_max_width(spec, maxSize.width());
    }
    if (maxSize.height() >= minSize.height()) {
        mir_surface_spec_set_max_height(spec, maxSize.height());
    }
    if (increment.width() > 0) {
        mir_surface_spec_set_width_increment(spec, increment.width());
    }
    if (increment.height() > 0) {
        mir_surface_spec_set_height_increment(spec, increment.height());
    }
}

MirSurface *createMirSurface(QWindow *window, QMirClientScreen *screen, QMirClientInput *input, MirConnection *connection)
{
    auto spec = makeSurfaceSpec(window, input, connection);
    const auto title = window->title().toUtf8();
    mir_surface_spec_set_name(spec.get(), title.constData());

    setSizingConstraints(spec.get(), window->minimumSize(), window->maximumSize(), window->sizeIncrement());

    if (window->windowState() == Qt::WindowFullScreen) {
        mir_surface_spec_set_fullscreen_on_output(spec.get(), screen->mirOutputId());
    }

    auto surface = mir_surface_create_sync(spec.get());
    Q_ASSERT(mir_surface_is_valid(surface));
    return surface;
}

// FIXME - in order to work around https://bugs.launchpad.net/mir/+bug/1346633
// we need to guess the panel height (3GU)
int panelHeight()
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
    return gridUnit * 3;
}

} //namespace

class QMirClientSurface
{
public:
    QMirClientSurface(QMirClientWindow *platformWindow, QMirClientScreen *screen, QMirClientInput *input, MirConnection *connection)
        : mWindow(platformWindow->window())
        , mPlatformWindow(platformWindow)
        , mInput(input)
        , mConnection(connection)
        , mMirSurface(createMirSurface(mWindow, screen, input, connection))
        , mEglDisplay(screen->eglDisplay())
        , mEglSurface(eglCreateWindowSurface(mEglDisplay, screen->eglConfig(), nativeWindowFor(mMirSurface), nullptr))
        , mVisible(false)
        , mNeedsRepaint(false)
        , mParented(mWindow->transientParent() || mWindow->parent())
        , mWindowState(mWindow->windowState())

    {
        mir_surface_set_event_handler(mMirSurface, surfaceEventCallback, this);

        // Window manager can give us a final size different from what we asked for
        // so let's check what we ended up getting
        MirSurfaceParameters parameters;
        mir_surface_get_parameters(mMirSurface, &parameters);

        auto geom = mWindow->geometry();
        geom.setWidth(parameters.width);
        geom.setHeight(parameters.height);
        if (mWindowState == Qt::WindowFullScreen) {
            geom.setY(0);
        } else {
            geom.setY(panelHeight());
        }

        // Assume that the buffer size matches the surface size at creation time
        mBufferSize = geom.size();
        platformWindow->QPlatformWindow::setGeometry(geom);
        QWindowSystemInterface::handleGeometryChange(mWindow, geom);

        DLOG("[ubuntumirclient QPA] created surface at (%d, %d) with size (%d, %d), title '%s', role: '%d'\n",
             geom.x(), geom.y(), geom.width(), geom.height(), mWindow->title().toUtf8().constData(), roleFor(mWindow));
    }

    ~QMirClientSurface()
    {
        if (mEglSurface != EGL_NO_SURFACE)
            eglDestroySurface(mEglDisplay, mEglSurface);
        if (mMirSurface)
            mir_surface_release_sync(mMirSurface);
    }

    QMirClientSurface(QMirClientSurface const&) = delete;
    QMirClientSurface& operator=(QMirClientSurface const&) = delete;

    void resize(const QSize& newSize);
    void setState(Qt::WindowState newState);
    void setVisible(bool state);
    void updateTitle(const QString& title);
    void setSizingConstraints(const QSize& minSize, const QSize& maxSize, const QSize& increment);

    void onSwapBuffersDone();
    void handleSurfaceResized(int width, int height);
    int needsRepaint() const;

    EGLSurface eglSurface() const { return mEglSurface; }
    MirSurface *mirSurface() const { return mMirSurface; }

private:
    static void surfaceEventCallback(MirSurface* surface, const MirEvent *event, void* context);
    void postEvent(const MirEvent *event);
    void updateSurface();

    QWindow * const mWindow;
    QMirClientWindow * const mPlatformWindow;
    QMirClientInput * const mInput;
    MirConnection * const mConnection;

    MirSurface * const mMirSurface;
    const EGLDisplay mEglDisplay;
    const EGLSurface mEglSurface;

    bool mVisible;
    bool mNeedsRepaint;
    bool mParented;
    Qt::WindowState mWindowState;
    QSize mBufferSize;

    QMutex mTargetSizeMutex;
    QSize mTargetSize;
};

void QMirClientSurface::resize(const QSize& size)
{
    DLOG("[ubuntumirclient QPA] resize(window=%p, width=%d, height=%d)", mWindow, size.width(), size.height());

    if (mWindowState == Qt::WindowFullScreen || mWindowState == Qt::WindowMaximized) {
        DLOG("[ubuntumirclient QPA] resize(window=%p) - not resizing, window is maximized or fullscreen", mWindow);
        return;
    }

    if (size.isEmpty()) {
        DLOG("[ubuntumirclient QPA] resize(window=%p) - not resizing, size is empty", mWindow);
        return;
    }

    Spec spec{mir_connection_create_spec_for_changes(mConnection)};
    mir_surface_spec_set_width(spec.get(), size.width());
    mir_surface_spec_set_height(spec.get(), size.height());
    mir_surface_apply_spec(mMirSurface, spec.get());
}

void QMirClientSurface::setState(Qt::WindowState newState)
{
    mir_wait_for(mir_surface_set_state(mMirSurface, qtWindowStateToMirSurfaceState(newState)));
    mWindowState = newState;
}

void QMirClientSurface::setVisible(bool visible)
{
    if (mVisible == visible)
        return;

    mVisible = visible;

    if (mVisible)
        updateSurface();

    // TODO: Use the new mir_surface_state_hidden state instead of mir_surface_state_minimized.
    //       Will have to change qtmir and unity8 for that.
    const auto newState = visible ? qtWindowStateToMirSurfaceState(mWindowState) : mir_surface_state_minimized;
    mir_wait_for(mir_surface_set_state(mMirSurface, newState));
}

void QMirClientSurface::updateTitle(const QString& newTitle)
{
    const auto title = newTitle.toUtf8();
    Spec spec{mir_connection_create_spec_for_changes(mConnection)};
    mir_surface_spec_set_name(spec.get(), title.constData());
    mir_surface_apply_spec(mMirSurface, spec.get());
}

void QMirClientSurface::setSizingConstraints(const QSize& minSize, const QSize& maxSize, const QSize& increment)
{
    Spec spec{mir_connection_create_spec_for_changes(mConnection)};
    ::setSizingConstraints(spec.get(), minSize, maxSize, increment);
    mir_surface_apply_spec(mMirSurface, spec.get());
}

void QMirClientSurface::handleSurfaceResized(int width, int height)
{
    QMutexLocker lock(&mTargetSizeMutex);

    // mir's resize event is mainly a signal that we need to redraw our content. We use the
    // width/height as identifiers to figure out if this is the latest surface resize event
    // that has posted, discarding any old ones. This avoids issuing too many redraw events.
    // see TODO in postEvent as the ideal way we should handle this.
    // The actual buffer size may or may have not changed at this point, so let the rendering
    // thread drive the window geometry updates.
    mNeedsRepaint = mTargetSize.width() == width && mTargetSize.height() == height;
}

int QMirClientSurface::needsRepaint() const
{
    if (mNeedsRepaint) {
        if (mTargetSize != mBufferSize) {
            //If the buffer hasn't changed yet, we need at least two redraws,
            //once to get the new buffer size and propagate the geometry changes
            //and the second to redraw the content at the new size
            return 2;
        } else {
            // The buffer size has already been updated so we only need one redraw
            // to render at the new size
            return 1;
        }
    }
    return 0;
}

void QMirClientSurface::onSwapBuffersDone()
{
#if !defined(QT_NO_DEBUG)
    static int sFrameNumber = 0;
    ++sFrameNumber;
#endif

    EGLint eglSurfaceWidth = -1;
    EGLint eglSurfaceHeight = -1;
    eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &eglSurfaceWidth);
    eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &eglSurfaceHeight);

    const bool validSize = eglSurfaceWidth > 0 && eglSurfaceHeight > 0;

    if (validSize && (mBufferSize.width() != eglSurfaceWidth || mBufferSize.height() != eglSurfaceHeight)) {

        DLOG("[ubuntumirclient QPA] onSwapBuffersDone(window=%p) [%d] - size changed (%d, %d) => (%d, %d)",
               mWindow, sFrameNumber, mBufferSize.width(), mBufferSize.height(), eglSurfaceWidth, eglSurfaceHeight);

        mBufferSize.rwidth() = eglSurfaceWidth;
        mBufferSize.rheight() = eglSurfaceHeight;

        QRect newGeometry = mPlatformWindow->geometry();
        newGeometry.setSize(mBufferSize);

        mPlatformWindow->QPlatformWindow::setGeometry(newGeometry);
        QWindowSystemInterface::handleGeometryChange(mWindow, newGeometry);
    } else {
#if 0
        DLOG("[ubuntumirclient QPA] onSwapBuffersDone(window=%p) [%d] - buffer size (%d,%d)",
               mWindow, sFrameNumber, mBufferSize.width(), mBufferSize.height());
#endif
    }
}

void QMirClientSurface::surfaceEventCallback(MirSurface *surface, const MirEvent *event, void* context)
{
    Q_UNUSED(surface);
    Q_ASSERT(context != nullptr);

    auto s = static_cast<QMirClientSurface *>(context);
    s->postEvent(event);
}

void QMirClientSurface::postEvent(const MirEvent *event)
{
    if (mir_event_type_resize == mir_event_get_type(event)) {
        // TODO: The current event queue just accumulates all resize events;
        // It would be nicer if we could update just one event if that event has not been dispatched.
        // As a workaround, we use the width/height as an identifier of this latest event
        // so the event handler (handleSurfaceResized) can discard/ignore old ones.
        const auto resizeEvent = mir_event_get_resize_event(event);
        const auto width =  mir_resize_event_get_width(resizeEvent);
        const auto height =  mir_resize_event_get_height(resizeEvent);
        DLOG("[ubuntumirclient QPA] resizeEvent(window=%p, width=%d, height=%d)", mWindow, width, height);

        QMutexLocker lock(&mTargetSizeMutex);
        mTargetSize.rwidth() = width;
        mTargetSize.rheight() = height;
    }

    mInput->postEvent(mPlatformWindow, event);
}

void QMirClientSurface::updateSurface()
{
    DLOG("[ubuntumirclient QPA] updateSurface(window=%p)", mWindow);

    if (!mParented && mWindow->type() == Qt::Dialog) {
        // The dialog may have been parented after creation time
        // so morph it into a modal dialog
        auto parent = transientParentFor(mWindow);
        if (parent) {
            DLOG("[ubuntumirclient QPA] updateSurface(window=%p) dialog now parented", mWindow);
            mParented = true;
            Spec spec{mir_connection_create_spec_for_changes(mConnection)};
            mir_surface_spec_set_parent(spec.get(), parent->mirSurface());
            mir_surface_apply_spec(mMirSurface, spec.get());
        }
    }
}

QMirClientWindow::QMirClientWindow(QWindow *w, const QSharedPointer<QMirClientClipboard> &clipboard, QMirClientScreen *screen,
                           QMirClientInput *input, MirConnection *connection)
    : QObject(nullptr)
    , QPlatformWindow(w)
    , mId(makeId())
    , mClipboard(clipboard)
    , mSurface(new QMirClientSurface{this, screen, input, connection})
{
    DLOG("[ubuntumirclient QPA] QMirClientWindow(window=%p, screen=%p, input=%p, surf=%p)", w, screen, input, mSurface.get());
}

QMirClientWindow::~QMirClientWindow()
{
    DLOG("[ubuntumirclient QPA] ~QMirClientWindow(window=%p)", this);
}

void QMirClientWindow::handleSurfaceResized(int width, int height)
{
    QMutexLocker lock(&mMutex);
    DLOG("[ubuntumirclient QPA] handleSurfaceResize(window=%p, width=%d, height=%d)", window(), width, height);

    mSurface->handleSurfaceResized(width, height);

    // This resize event could have occurred just after the last buffer swap for this window.
    // This means the client may still be holding a buffer with the older size. The first redraw call
    // will then render at the old size. After swapping the client now will get a new buffer with the
    // updated size but it still needs re-rendering so another redraw may be needed.
    // A mir API to drop the currently held buffer would help here, so that we wouldn't have to redraw twice
    auto const numRepaints = mSurface->needsRepaint();
    DLOG("[ubuntumirclient QPA] handleSurfaceResize(window=%p) redraw %d times", window(), numRepaints);
    for (int i = 0; i < numRepaints; i++) {
        DLOG("[ubuntumirclient QPA] handleSurfaceResize(window=%p) repainting width=%d, height=%d", window(), geometry().size().width(), geometry().size().height());
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
    }
}

void QMirClientWindow::handleSurfaceFocused()
{
    DLOG("[ubuntumirclient QPA] handleSurfaceFocused(window=%p)", window());

    // System clipboard contents might have changed while this window was unfocused and without
    // this process getting notified about it because it might have been suspended (due to
    // application lifecycle policies), thus unable to listen to any changes notified through
    // D-Bus.
    // Therefore let's ensure we are up to date with the system clipboard now that we are getting
    // focused again.
    mClipboard->requestDBusClipboardContents();
}

void QMirClientWindow::setWindowState(Qt::WindowState state)
{
    QMutexLocker lock(&mMutex);
    DLOG("[ubuntumirclient QPA] setWindowState(window=%p, %s)", this, qtWindowStateToStr(state));
    mSurface->setState(state);

    updatePanelHeightHack(state);
}

/*
    FIXME: Mir does not let clients know the position of their windows in the virtual
    desktop space. So we have this ugly hack that assumes a phone situation where the
    window is always on the top-left corner, right below the indicators panel if not
    in fullscreen.
 */
void QMirClientWindow::updatePanelHeightHack(Qt::WindowState state)
{
    if (state == Qt::WindowFullScreen && geometry().y() != 0) {
        QRect newGeometry = geometry();
        newGeometry.setY(0);
        QPlatformWindow::setGeometry(newGeometry);
        QWindowSystemInterface::handleGeometryChange(window(), newGeometry);
    } else if (geometry().y() == 0) {
        QRect newGeometry = geometry();
        newGeometry.setY(panelHeight());
        QPlatformWindow::setGeometry(newGeometry);
        QWindowSystemInterface::handleGeometryChange(window(), newGeometry);
    }
}

void QMirClientWindow::setGeometry(const QRect& rect)
{
    QMutexLocker lock(&mMutex);
    DLOG("[ubuntumirclient QPA] setGeometry (window=%p, x=%d, y=%d, width=%d, height=%d)",
           window(), rect.x(), rect.y(), rect.width(), rect.height());

    //NOTE: mir surfaces cannot be moved by the client so ignore the topLeft coordinates
    const auto newSize = rect.size();
    auto newGeometry = geometry();
    newGeometry.setSize(newSize);
    QPlatformWindow::setGeometry(newGeometry);

    mSurface->resize(newSize);
}

void QMirClientWindow::setVisible(bool visible)
{
    QMutexLocker lock(&mMutex);
    DLOG("[ubuntumirclient QPA] setVisible (window=%p, visible=%s)", window(), visible ? "true" : "false");

    mSurface->setVisible(visible);
    const QRect& exposeRect = visible ? QRect(QPoint(), geometry().size()) : QRect();

    lock.unlock();
    QWindowSystemInterface::handleExposeEvent(window(), exposeRect);
    QWindowSystemInterface::flushWindowSystemEvents();
}

void QMirClientWindow::setWindowTitle(const QString& title)
{
    QMutexLocker lock(&mMutex);
    DLOG("[ubuntumirclient QPA] setWindowTitle(window=%p) title=%s)", window(), title.toUtf8().constData());
    mSurface->updateTitle(title);
}

void QMirClientWindow::propagateSizeHints()
{
    QMutexLocker lock(&mMutex);
    const auto win = window();
    DLOG("[ubuntumirclient QPA] propagateSizeHints(window=%p) min(%d,%d), max(%d,%d) increment(%d, %d)",
           win, win->minimumSize().width(), win->minimumSize().height(),
           win->maximumSize().width(), win->maximumSize().height(),
           win->sizeIncrement().width(), win->sizeIncrement().height());
    mSurface->setSizingConstraints(win->minimumSize(), win->maximumSize(), win->sizeIncrement());
}

void* QMirClientWindow::eglSurface() const
{
    return mSurface->eglSurface();
}

MirSurface *QMirClientWindow::mirSurface() const
{
    return mSurface->mirSurface();
}

WId QMirClientWindow::winId() const
{
    return mId;
}

void QMirClientWindow::onSwapBuffersDone()
{
    QMutexLocker lock(&mMutex);
    mSurface->onSwapBuffersDone();
}
