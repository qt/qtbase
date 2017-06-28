/****************************************************************************
**
** Copyright (C) 2014-2016 Canonical, Ltd.
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
#include "qmirclientdebugextension.h"
#include "qmirclientnativeinterface.h"
#include "qmirclientinput.h"
#include "qmirclientintegration.h"
#include "qmirclientscreen.h"
#include "qmirclientlogging.h"

#include <mir_toolkit/mir_client_library.h>
#include <mir_toolkit/version.h>

// Qt
#include <qpa/qwindowsysteminterface.h>
#include <QMutexLocker>
#include <QSize>
#include <QtMath>
#include <QtEglSupport/private/qeglconvenience_p.h>

// Platform API
#include <ubuntu/application/instance.h>

#include <EGL/egl.h>

Q_LOGGING_CATEGORY(mirclientBufferSwap, "qt.qpa.mirclient.bufferSwap", QtWarningMsg)

namespace
{
const Qt::WindowType LowChromeWindowHint = (Qt::WindowType)0x00800000;

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
    case Qt::WindowActive:
        return "Active";
    }
    Q_UNREACHABLE();
}

const char *mirSurfaceStateToStr(MirSurfaceState surfaceState)
{
    switch (surfaceState) {
    case mir_surface_state_unknown: return "unknown";
    case mir_surface_state_restored: return "restored";
    case mir_surface_state_minimized: return "minimized";
    case mir_surface_state_maximized: return "vertmaximized";
    case mir_surface_state_vertmaximized: return "vertmaximized";
    case mir_surface_state_fullscreen: return "fullscreen";
    case mir_surface_state_horizmaximized: return "horizmaximized";
    case mir_surface_state_hidden: return "hidden";
    case mir_surface_states: Q_UNREACHABLE();
    }
    Q_UNREACHABLE();
}

const char *mirPixelFormatToStr(MirPixelFormat pixelFormat)
{
    switch (pixelFormat) {
    case mir_pixel_format_invalid:   return "invalid";
    case mir_pixel_format_abgr_8888: return "ABGR8888";
    case mir_pixel_format_xbgr_8888: return "XBGR8888";
    case mir_pixel_format_argb_8888: return "ARGB8888";
    case mir_pixel_format_xrgb_8888: return "XRGB8888";
    case mir_pixel_format_bgr_888:   return "BGR888";
    case mir_pixel_format_rgb_888:   return "RGB888";
    case mir_pixel_format_rgb_565:   return "RGB565";
    case mir_pixel_format_rgba_5551: return "RGBA5551";
    case mir_pixel_format_rgba_4444: return "RGBA4444";
    case mir_pixel_formats:          Q_UNREACHABLE();
    }
    Q_UNREACHABLE();
}

const char *mirSurfaceTypeToStr(MirSurfaceType type)
{
    switch (type) {
    case mir_surface_type_normal:       return "Normal";        /**< AKA "regular"                   */
    case mir_surface_type_utility:      return "Utility";       /**< AKA "floating regular"          */
    case mir_surface_type_dialog:       return "Dialog";
    case mir_surface_type_gloss:        return "Gloss";
    case mir_surface_type_freestyle:    return "Freestyle";
    case mir_surface_type_menu:         return "Menu";
    case mir_surface_type_inputmethod:  return "Input Method";  /**< AKA "OSK" or handwriting etc.   */
    case mir_surface_type_satellite:    return "Satellite";     /**< AKA "toolbox"/"toolbar"         */
    case mir_surface_type_tip:          return "Tip";           /**< AKA "tooltip"                   */
    case mir_surface_types:             Q_UNREACHABLE();
    }
    return "";
}

MirSurfaceState qtWindowStateToMirSurfaceState(Qt::WindowState state)
{
    switch (state) {
    case Qt::WindowNoState:
    case Qt::WindowActive:
        return mir_surface_state_restored;
    case Qt::WindowFullScreen:
        return mir_surface_state_fullscreen;
    case Qt::WindowMaximized:
        return mir_surface_state_maximized;
    case Qt::WindowMinimized:
        return mir_surface_state_minimized;
    }
    return mir_surface_state_unknown; // should never be reached
}

MirSurfaceType qtWindowTypeToMirSurfaceType(Qt::WindowType type)
{
    switch (type & Qt::WindowType_Mask) {
    case Qt::Dialog:
        return mir_surface_type_dialog;
    case Qt::Sheet:
    case Qt::Drawer:
        return mir_surface_type_utility;
    case Qt::Popup:
    case Qt::Tool:
        return mir_surface_type_menu;
    case Qt::ToolTip:
        return mir_surface_type_tip;
    case Qt::SplashScreen:
        return mir_surface_type_freestyle;
    case Qt::Window:
    default:
        return mir_surface_type_normal;
    }
}

WId makeId()
{
    static int id = 1;
    return id++;
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

bool requiresParent(const MirSurfaceType type)
{
    switch (type) {
    case mir_surface_type_dialog: //FIXME - not quite what the specification dictates, but is what Mir's api dictates
    case mir_surface_type_utility:
    case mir_surface_type_gloss:
    case mir_surface_type_menu:
    case mir_surface_type_satellite:
    case mir_surface_type_tip:
        return true;
    default:
        return false;
    }
}

bool requiresParent(const Qt::WindowType type)
{
    return requiresParent(qtWindowTypeToMirSurfaceType(type));
}

bool isMovable(const Qt::WindowType type)
{
    auto mirType = qtWindowTypeToMirSurfaceType(type);
    switch (mirType) {
    case mir_surface_type_menu:
    case mir_surface_type_tip:
        return true;
    default:
        return false;
    }
}

Spec makeSurfaceSpec(QWindow *window, MirPixelFormat pixelFormat, QMirClientWindow *parentWindowHandle,
                     MirConnection *connection)
{
    const auto geometry = window->geometry();
    const int width = geometry.width() > 0 ? geometry.width() : 1;
    const int height = geometry.height() > 0 ? geometry.height() : 1;
    auto type = qtWindowTypeToMirSurfaceType(window->type());

    if (U_ON_SCREEN_KEYBOARD_ROLE == roleFor(window)) {
        type = mir_surface_type_inputmethod;
    }

    MirRectangle location{geometry.x(), geometry.y(), 0, 0};
    MirSurface *parent = nullptr;
    if (parentWindowHandle) {
        parent = parentWindowHandle->mirSurface();
        // Qt uses absolute positioning, but Mir positions surfaces relative to parent.
        location.top  -= parentWindowHandle->geometry().top();
        location.left -= parentWindowHandle->geometry().left();
    }

    Spec spec;

    switch (type) {
    case mir_surface_type_menu:
        spec = Spec{mir_connection_create_spec_for_menu(connection, width, height, pixelFormat, parent,
                    &location, mir_edge_attachment_any)};
        break;
    case mir_surface_type_dialog:
        spec = Spec{mir_connection_create_spec_for_modal_dialog(connection, width, height, pixelFormat, parent)};
        break;
    case mir_surface_type_utility:
        spec = Spec{mir_connection_create_spec_for_dialog(connection, width, height, pixelFormat)};
        break;
    case mir_surface_type_tip:
#if MIR_CLIENT_VERSION < MIR_VERSION_NUMBER(3, 4, 0)
        spec = Spec{mir_connection_create_spec_for_tooltip(connection, width, height, pixelFormat, parent,
                    &location)};
#else
        spec = Spec{mir_connection_create_spec_for_tip(connection, width, height, pixelFormat, parent,
                    &location, mir_edge_attachment_any)};
#endif
        break;
    case mir_surface_type_inputmethod:
        spec = Spec{mir_connection_create_spec_for_input_method(connection, width, height, pixelFormat)};
        break;
    default:
        spec = Spec{mir_connection_create_spec_for_normal_surface(connection, width, height, pixelFormat)};
        break;
    }

    qCDebug(mirclient, "makeSurfaceSpec(window=%p): %s spec (type=0x%x, position=(%d, %d)px, size=(%dx%d)px)",
            window, mirSurfaceTypeToStr(type), window->type(), location.left, location.top, width, height);

     return std::move(spec);
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

MirSurface *createMirSurface(QWindow *window, int mirOutputId, QMirClientWindow *parentWindowHandle,
                             MirPixelFormat pixelFormat, MirConnection *connection,
                             mir_surface_event_callback inputCallback, void *inputContext)
{
    auto spec = makeSurfaceSpec(window, pixelFormat, parentWindowHandle, connection);

    // Install event handler as early as possible
    mir_surface_spec_set_event_handler(spec.get(), inputCallback, inputContext);

    const auto title = window->title().toUtf8();
    mir_surface_spec_set_name(spec.get(), title.constData());

    setSizingConstraints(spec.get(), window->minimumSize(), window->maximumSize(), window->sizeIncrement());

    if (window->windowState() == Qt::WindowFullScreen) {
        mir_surface_spec_set_fullscreen_on_output(spec.get(), mirOutputId);
    }

    if (window->flags() & LowChromeWindowHint) {
        mir_surface_spec_set_shell_chrome(spec.get(), mir_shell_chrome_low);
    }

    if (!window->isVisible()) {
        mir_surface_spec_set_state(spec.get(), mir_surface_state_hidden);
    }

    auto surface = mir_surface_create_sync(spec.get());
    Q_ASSERT(mir_surface_is_valid(surface));
    return surface;
}

QMirClientWindow *getParentIfNecessary(QWindow *window, QMirClientInput *input)
{
    QMirClientWindow *parentWindowHandle = nullptr;
    if (requiresParent(window->type())) {
        parentWindowHandle = transientParentFor(window);
        if (parentWindowHandle == nullptr) {
            // NOTE: Mir requires this surface have a parent. Try using the last surface to receive input as that will
            // most likely be the one that caused this surface to be created
            parentWindowHandle = input->lastInputWindow();
        }
    }
    return parentWindowHandle;
}

MirPixelFormat disableAlphaBufferIfPossible(MirPixelFormat pixelFormat)
{
    switch (pixelFormat) {
    case mir_pixel_format_abgr_8888:
        return mir_pixel_format_xbgr_8888;
    case mir_pixel_format_argb_8888:
        return mir_pixel_format_xrgb_8888;
    default: // can do nothing, leave it alone
        return pixelFormat;
    }
}
} //namespace



class UbuntuSurface
{
public:
    UbuntuSurface(QMirClientWindow *platformWindow, EGLDisplay display, QMirClientInput *input, MirConnection *connection);
    ~UbuntuSurface();

    UbuntuSurface(const UbuntuSurface &) = delete;
    UbuntuSurface& operator=(const UbuntuSurface &) = delete;

    void updateGeometry(const QRect &newGeometry);
    void updateTitle(const QString& title);
    void setSizingConstraints(const QSize& minSize, const QSize& maxSize, const QSize& increment);

    void onSwapBuffersDone();
    void handleSurfaceResized(int width, int height);
    int needsRepaint() const;

    MirSurfaceState state() const { return mir_surface_get_state(mMirSurface); }
    void setState(MirSurfaceState state);

    MirSurfaceType type() const { return mir_surface_get_type(mMirSurface); }

    void setShellChrome(MirShellChrome shellChrome);

    EGLSurface eglSurface() const { return mEglSurface; }
    MirSurface *mirSurface() const { return mMirSurface; }

    void setSurfaceParent(MirSurface*);
    bool hasParent() const { return mParented; }

    QSurfaceFormat format() const { return mFormat; }

    bool mNeedsExposeCatchup;

    QString persistentSurfaceId();

private:
    static void surfaceEventCallback(MirSurface* surface, const MirEvent *event, void* context);
    void postEvent(const MirEvent *event);

    QWindow * const mWindow;
    QMirClientWindow * const mPlatformWindow;
    QMirClientInput * const mInput;
    MirConnection * const mConnection;
    QMirClientWindow * mParentWindowHandle{nullptr};

    MirSurface* mMirSurface;
    const EGLDisplay mEglDisplay;
    EGLSurface mEglSurface;

    bool mNeedsRepaint;
    bool mParented;
    QSize mBufferSize;
    QSurfaceFormat mFormat;
    MirPixelFormat mPixelFormat;

    QMutex mTargetSizeMutex;
    QSize mTargetSize;
    MirShellChrome mShellChrome;
    QString mPersistentIdStr;
};

UbuntuSurface::UbuntuSurface(QMirClientWindow *platformWindow, EGLDisplay display, QMirClientInput *input, MirConnection *connection)
    : mWindow(platformWindow->window())
    , mPlatformWindow(platformWindow)
    , mInput(input)
    , mConnection(connection)
    , mEglDisplay(display)
    , mNeedsRepaint(false)
    , mParented(mWindow->transientParent() || mWindow->parent())
    , mFormat(mWindow->requestedFormat())
    , mShellChrome(mWindow->flags() & LowChromeWindowHint ? mir_shell_chrome_low : mir_shell_chrome_normal)
{
    // Have Qt choose most suitable EGLConfig for the requested surface format, and update format to reflect it
    EGLConfig config = q_configFromGLFormat(display, mFormat, true);
    if (config == 0) {
        // Older Intel Atom-based devices only support OpenGL 1.4 compatibility profile but by default
        // QML asks for at least OpenGL 2.0. The XCB GLX backend ignores this request and returns a
        // 1.4 context, but the XCB EGL backend tries to honor it, and fails. The 1.4 context appears to
        // have sufficient capabilities on MESA (i915) to render correctly however. So reduce the default
        // requested OpenGL version to 1.0 to ensure EGL will give us a working context (lp:1549455).
        static const bool isMesa = QString(eglQueryString(display, EGL_VENDOR)).contains(QStringLiteral("Mesa"));
        if (isMesa) {
            qCDebug(mirclientGraphics, "Attempting to choose OpenGL 1.4 context which may suit Mesa");
            mFormat.setMajorVersion(1);
            mFormat.setMinorVersion(4);
            config = q_configFromGLFormat(display, mFormat, true);
        }
    }
    if (config == 0) {
        qCritical() << "Qt failed to choose a suitable EGLConfig to suit the surface format" << mFormat;
    }

    mFormat = q_glFormatFromConfig(display, config, mFormat);

    // Have Mir decide the pixel format most suited to the chosen EGLConfig. This is the only way
    // Mir will know what EGLConfig has been chosen - it cannot deduce it from the buffers.
    mPixelFormat = mir_connection_get_egl_pixel_format(connection, display, config);
    // But the chosen EGLConfig might have an alpha buffer enabled, even if not requested by the client.
    // If that's the case, try to edit the chosen pixel format in order to disable the alpha buffer.
    // This is an optimization for the compositor, as it can avoid blending this surface.
    if (mWindow->requestedFormat().alphaBufferSize() < 0) {
        mPixelFormat = disableAlphaBufferIfPossible(mPixelFormat);
    }

    const auto outputId = static_cast<QMirClientScreen *>(mWindow->screen()->handle())->mirOutputId();

    mParentWindowHandle = getParentIfNecessary(mWindow, input);

    mMirSurface = createMirSurface(mWindow, outputId, mParentWindowHandle, mPixelFormat, connection, surfaceEventCallback, this);
    mEglSurface = eglCreateWindowSurface(mEglDisplay, config, nativeWindowFor(mMirSurface), nullptr);

    mNeedsExposeCatchup = mir_surface_get_visibility(mMirSurface) == mir_surface_visibility_occluded;

    // Window manager can give us a final size different from what we asked for
    // so let's check what we ended up getting
    MirSurfaceParameters parameters;
    mir_surface_get_parameters(mMirSurface, &parameters);

    auto geom = mWindow->geometry();
    geom.setWidth(parameters.width);
    geom.setHeight(parameters.height);

    // Assume that the buffer size matches the surface size at creation time
    mBufferSize = geom.size();
    QWindowSystemInterface::handleGeometryChange(mWindow, geom);

    qCDebug(mirclient) << "Created surface with geometry:" << geom << "title:" << mWindow->title()
                       << "role:" << roleFor(mWindow);
    qCDebug(mirclientGraphics)
                       << "Requested format:" << mWindow->requestedFormat()
                       << "\nActual format:" << mFormat
                       << "with associated Mir pixel format:" << mirPixelFormatToStr(mPixelFormat);
}

UbuntuSurface::~UbuntuSurface()
{
    if (mEglSurface != EGL_NO_SURFACE)
        eglDestroySurface(mEglDisplay, mEglSurface);
    if (mMirSurface) {
        mir_surface_release_sync(mMirSurface);
    }
}

void UbuntuSurface::updateGeometry(const QRect &newGeometry)
{
    qCDebug(mirclient,"updateGeometry(window=%p, width=%d, height=%d)", mWindow,
            newGeometry.width(), newGeometry.height());

    Spec spec;
    if (isMovable(mWindow->type())) {
        spec = Spec{makeSurfaceSpec(mWindow, mPixelFormat, mParentWindowHandle, mConnection)};
    } else {
        spec = Spec{mir_connection_create_spec_for_changes(mConnection)};
        mir_surface_spec_set_width(spec.get(), newGeometry.width());
        mir_surface_spec_set_height(spec.get(), newGeometry.height());
    }
    mir_surface_apply_spec(mMirSurface, spec.get());
}

void UbuntuSurface::updateTitle(const QString& newTitle)
{
    const auto title = newTitle.toUtf8();
    Spec spec{mir_connection_create_spec_for_changes(mConnection)};
    mir_surface_spec_set_name(spec.get(), title.constData());
    mir_surface_apply_spec(mMirSurface, spec.get());
}

void UbuntuSurface::setSizingConstraints(const QSize& minSize, const QSize& maxSize, const QSize& increment)
{
    Spec spec{mir_connection_create_spec_for_changes(mConnection)};
    ::setSizingConstraints(spec.get(), minSize, maxSize, increment);
    mir_surface_apply_spec(mMirSurface, spec.get());
}

void UbuntuSurface::handleSurfaceResized(int width, int height)
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

int UbuntuSurface::needsRepaint() const
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

void UbuntuSurface::setState(MirSurfaceState state)
{
    mir_wait_for(mir_surface_set_state(mMirSurface, state));
}

void UbuntuSurface::setShellChrome(MirShellChrome chrome)
{
    if (chrome != mShellChrome) {
        auto spec = Spec{mir_connection_create_spec_for_changes(mConnection)};
        mir_surface_spec_set_shell_chrome(spec.get(), chrome);
        mir_surface_apply_spec(mMirSurface, spec.get());

        mShellChrome = chrome;
    }
}

void UbuntuSurface::onSwapBuffersDone()
{
    static int sFrameNumber = 0;
    ++sFrameNumber;

    EGLint eglSurfaceWidth = -1;
    EGLint eglSurfaceHeight = -1;
    eglQuerySurface(mEglDisplay, mEglSurface, EGL_WIDTH, &eglSurfaceWidth);
    eglQuerySurface(mEglDisplay, mEglSurface, EGL_HEIGHT, &eglSurfaceHeight);

    const bool validSize = eglSurfaceWidth > 0 && eglSurfaceHeight > 0;

    if (validSize && (mBufferSize.width() != eglSurfaceWidth || mBufferSize.height() != eglSurfaceHeight)) {

        qCDebug(mirclientBufferSwap, "onSwapBuffersDone(window=%p) [%d] - size changed (%d, %d) => (%d, %d)",
               mWindow, sFrameNumber, mBufferSize.width(), mBufferSize.height(), eglSurfaceWidth, eglSurfaceHeight);

        mBufferSize.rwidth() = eglSurfaceWidth;
        mBufferSize.rheight() = eglSurfaceHeight;

        QRect newGeometry = mPlatformWindow->geometry();
        newGeometry.setSize(mBufferSize);

        QWindowSystemInterface::handleGeometryChange(mWindow, newGeometry);
    } else {
        qCDebug(mirclientBufferSwap, "onSwapBuffersDone(window=%p) [%d] - buffer size (%d,%d)",
               mWindow, sFrameNumber, mBufferSize.width(), mBufferSize.height());
    }
}

void UbuntuSurface::surfaceEventCallback(MirSurface *surface, const MirEvent *event, void* context)
{
    Q_UNUSED(surface);
    Q_ASSERT(context != nullptr);

    auto s = static_cast<UbuntuSurface *>(context);
    s->postEvent(event);
}

void UbuntuSurface::postEvent(const MirEvent *event)
{
    if (mir_event_type_resize == mir_event_get_type(event)) {
        // TODO: The current event queue just accumulates all resize events;
        // It would be nicer if we could update just one event if that event has not been dispatched.
        // As a workaround, we use the width/height as an identifier of this latest event
        // so the event handler (handleSurfaceResized) can discard/ignore old ones.
        const auto resizeEvent = mir_event_get_resize_event(event);
        const auto width =  mir_resize_event_get_width(resizeEvent);
        const auto height =  mir_resize_event_get_height(resizeEvent);
        qCDebug(mirclient, "resizeEvent(window=%p, width=%d, height=%d)", mWindow, width, height);

        QMutexLocker lock(&mTargetSizeMutex);
        mTargetSize.rwidth() = width;
        mTargetSize.rheight() = height;
    }

    mInput->postEvent(mPlatformWindow, event);
}

void UbuntuSurface::setSurfaceParent(MirSurface* parent)
{
    qCDebug(mirclient, "setSurfaceParent(window=%p)", mWindow);

    mParented = true;
    Spec spec{mir_connection_create_spec_for_changes(mConnection)};
    mir_surface_spec_set_parent(spec.get(), parent);
    mir_surface_apply_spec(mMirSurface, spec.get());
}

QString UbuntuSurface::persistentSurfaceId()
{
    if (mPersistentIdStr.isEmpty()) {
        MirPersistentId* mirPermaId = mir_surface_request_persistent_id_sync(mMirSurface);
        mPersistentIdStr = mir_persistent_id_as_string(mirPermaId);
        mir_persistent_id_release(mirPermaId);
    }
    return mPersistentIdStr;
}

QMirClientWindow::QMirClientWindow(QWindow *w, QMirClientInput *input, QMirClientNativeInterface *native,
                                   QMirClientAppStateController *appState, EGLDisplay eglDisplay,
                                   MirConnection *mirConnection, QMirClientDebugExtension *debugExt)
    : QObject(nullptr)
    , QPlatformWindow(w)
    , mId(makeId())
    , mWindowState(w->windowState())
    , mWindowFlags(w->flags())
    , mWindowVisible(false)
    , mAppStateController(appState)
    , mDebugExtention(debugExt)
    , mNativeInterface(native)
    , mSurface(new UbuntuSurface{this, eglDisplay, input, mirConnection})
    , mScale(1.0)
    , mFormFactor(mir_form_factor_unknown)
{
    mWindowExposed = mSurface->mNeedsExposeCatchup == false;

    qCDebug(mirclient, "QMirClientWindow(window=%p, screen=%p, input=%p, surf=%p) with title '%s', role: '%d'",
            w, w->screen()->handle(), input, mSurface.get(), qPrintable(window()->title()), roleFor(window()));
}

QMirClientWindow::~QMirClientWindow()
{
    qCDebug(mirclient, "~QMirClientWindow(window=%p)", this);
}

void QMirClientWindow::handleSurfaceResized(int width, int height)
{
    QMutexLocker lock(&mMutex);
    qCDebug(mirclient, "handleSurfaceResize(window=%p, size=(%dx%d)px", window(), width, height);

    mSurface->handleSurfaceResized(width, height);

    // This resize event could have occurred just after the last buffer swap for this window.
    // This means the client may still be holding a buffer with the older size. The first redraw call
    // will then render at the old size. After swapping the client now will get a new buffer with the
    // updated size but it still needs re-rendering so another redraw may be needed.
    // A mir API to drop the currently held buffer would help here, so that we wouldn't have to redraw twice
    auto const numRepaints = mSurface->needsRepaint();
    lock.unlock();
    qCDebug(mirclient, "handleSurfaceResize(window=%p) redraw %d times", window(), numRepaints);
    for (int i = 0; i < numRepaints; i++) {
        qCDebug(mirclient, "handleSurfaceResize(window=%p) repainting size=(%dx%d)dp", window(), geometry().size().width(), geometry().size().height());
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
    }
}

void QMirClientWindow::handleSurfaceExposeChange(bool exposed)
{
    QMutexLocker lock(&mMutex);
    qCDebug(mirclient, "handleSurfaceExposeChange(window=%p, exposed=%s)", window(), exposed ? "true" : "false");

    mSurface->mNeedsExposeCatchup = false;
    if (mWindowExposed == exposed) return;
    mWindowExposed = exposed;

    lock.unlock();
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
}

void QMirClientWindow::handleSurfaceFocusChanged(bool focused)
{
    qCDebug(mirclient, "handleSurfaceFocusChanged(window=%p, focused=%d)", window(), focused);
    if (focused) {
        mAppStateController->setWindowFocused(true);
        QWindowSystemInterface::handleWindowActivated(window(), Qt::ActiveWindowFocusReason);
    } else {
        mAppStateController->setWindowFocused(false);
    }
}

void QMirClientWindow::handleSurfaceVisibilityChanged(bool visible)
{
    qCDebug(mirclient, "handleSurfaceVisibilityChanged(window=%p, visible=%d)", window(), visible);

    if (mWindowVisible == visible) return;
    mWindowVisible = visible;

    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
}

void QMirClientWindow::handleSurfaceStateChanged(Qt::WindowState state)
{
    qCDebug(mirclient, "handleSurfaceStateChanged(window=%p, %s)", window(), qtWindowStateToStr(state));

    if (mWindowState == state) return;
    mWindowState = state;

    QWindowSystemInterface::handleWindowStateChanged(window(), state);
}

void QMirClientWindow::setWindowState(Qt::WindowStates states)
{
    Qt::WindowState state = Qt::WindowNoState;
    if (states & Qt::WindowMinimized)
        state = Qt::WindowMinimized;
    else if (states & Qt::WindowFullScreen)
        state = Qt::WindowFullScreen;
    else if (states & Qt::WindowMaximized)
        state = Qt::WindowMaximized;

    QMutexLocker lock(&mMutex);
    qCDebug(mirclient, "setWindowState(window=%p, %s)", this, qtWindowStateToStr(state));

    if (mWindowState == state) return;
    mWindowState = state;

    lock.unlock();
    updateSurfaceState();
}

void QMirClientWindow::setWindowFlags(Qt::WindowFlags flags)
{
    QMutexLocker lock(&mMutex);
    qCDebug(mirclient, "setWindowFlags(window=%p, 0x%x)", this, (int)flags);

    if (mWindowFlags == flags) return;
    mWindowFlags = flags;

    mSurface->setShellChrome(mWindowFlags & LowChromeWindowHint ? mir_shell_chrome_low : mir_shell_chrome_normal);
}

QRect QMirClientWindow::geometry() const
{
    if (mDebugExtention) {
        auto geom = QPlatformWindow::geometry();
        geom.moveTopLeft(mDebugExtention->mapSurfacePointToScreen(mSurface->mirSurface(), QPoint(0,0)));
        return geom;
    } else {
        return QPlatformWindow::geometry();
    }
}

void QMirClientWindow::setGeometry(const QRect& rect)
{
    QMutexLocker lock(&mMutex);

    if (window()->windowState() == Qt::WindowFullScreen || window()->windowState() == Qt::WindowMaximized) {
        qCDebug(mirclient, "setGeometry(window=%p) - not resizing, window is maximized or fullscreen", window());
        return;
    }

    qCDebug(mirclient, "setGeometry (window=%p, position=(%d, %d)dp, size=(%dx%d)dp)",
            window(), rect.x(), rect.y(), rect.width(), rect.height());
    // Immediately update internal geometry so Qt believes position updated
    QRect newPosition(geometry());
    newPosition.moveTo(rect.topLeft());
    QPlatformWindow::setGeometry(newPosition);

    mSurface->updateGeometry(rect);
    // Note: don't call handleGeometryChange here, wait to see what Mir replies with.
}

void QMirClientWindow::setVisible(bool visible)
{
    QMutexLocker lock(&mMutex);
    qCDebug(mirclient, "setVisible (window=%p, visible=%s)", window(), visible ? "true" : "false");

    if (mWindowVisible == visible) return;
    mWindowVisible = visible;

    if (visible) {
        if (!mSurface->hasParent() && window()->type() == Qt::Dialog) {
            // The dialog may have been parented after creation time
            // so morph it into a modal dialog
            auto parent = transientParentFor(window());
            if (parent) {
                mSurface->setSurfaceParent(parent->mirSurface());
            }
        }
    }

    lock.unlock();
    updateSurfaceState();
    QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
}

void QMirClientWindow::setWindowTitle(const QString& title)
{
    QMutexLocker lock(&mMutex);
    qCDebug(mirclient, "setWindowTitle(window=%p) title=%s)", window(), title.toUtf8().constData());
    mSurface->updateTitle(title);
}

void QMirClientWindow::propagateSizeHints()
{
    QMutexLocker lock(&mMutex);
    const auto win = window();
    qCDebug(mirclient, "propagateSizeHints(window=%p) min(%d,%d), max(%d,%d) increment(%d, %d)",
            win, win->minimumSize().width(), win->minimumSize().height(),
            win->maximumSize().width(), win->maximumSize().height(),
            win->sizeIncrement().width(), win->sizeIncrement().height());
    mSurface->setSizingConstraints(win->minimumSize(), win->maximumSize(), win->sizeIncrement());
}

bool QMirClientWindow::isExposed() const
{
    // mNeedsExposeCatchup because we need to render a frame to get the expose surface event from mir.
    return mWindowVisible && (mWindowExposed || (mSurface && mSurface->mNeedsExposeCatchup));
}

QSurfaceFormat QMirClientWindow::format() const
{
    return mSurface->format();
}

QPoint QMirClientWindow::mapToGlobal(const QPoint &pos) const
{
    if (mDebugExtention) {
        return mDebugExtention->mapSurfacePointToScreen(mSurface->mirSurface(), pos);
    } else {
        return pos;
    }
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

    if (mSurface->mNeedsExposeCatchup) {
        mSurface->mNeedsExposeCatchup = false;
        mWindowExposed = false;

        lock.unlock();
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(), geometry().size()));
    }
}

void QMirClientWindow::handleScreenPropertiesChange(MirFormFactor formFactor, float scale)
{
    // Update the scale & form factor native-interface properties for the windows affected
    // as there is no convenient way to emit signals for those custom properties on a QScreen
    if (formFactor != mFormFactor) {
        mFormFactor = formFactor;
        Q_EMIT mNativeInterface->windowPropertyChanged(this, QStringLiteral("formFactor"));
    }

    if (!qFuzzyCompare(scale, mScale)) {
        mScale = scale;
        Q_EMIT mNativeInterface->windowPropertyChanged(this, QStringLiteral("scale"));
    }
}

void QMirClientWindow::updateSurfaceState()
{
    QMutexLocker lock(&mMutex);
    MirSurfaceState newState = mWindowVisible ? qtWindowStateToMirSurfaceState(mWindowState) :
                                                mir_surface_state_hidden;
    qCDebug(mirclient, "updateSurfaceState (window=%p, surfaceState=%s)", window(), mirSurfaceStateToStr(newState));
    if (newState != mSurface->state()) {
        mSurface->setState(newState);
    }
}

QString QMirClientWindow::persistentSurfaceId()
{
    return mSurface->persistentSurfaceId();
}
