// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindow.h"

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformintegration.h>
#ifndef QT_NO_CONTEXTMENU
#include <qpa/qplatformtheme.h>
#endif
#include "qsurfaceformat.h"
#ifndef QT_NO_OPENGL
#include <qpa/qplatformopenglcontext.h>
#include "qopenglcontext.h"
#include "qopenglcontext_p.h"
#endif
#include "qscreen.h"

#include "qwindow_p.h"
#include "qguiapplication_p.h"
#if QT_CONFIG(accessibility)
#  include "qaccessible.h"
#endif
#include "qhighdpiscaling_p.h"
#if QT_CONFIG(draganddrop)
#include "qshapedpixmapdndwindow_p.h"
#endif // QT_CONFIG(draganddrop)

#include <private/qevent_p.h>

#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include <QStyleHints>
#include <qpa/qplatformcursor.h>
#include <qpa/qplatformwindow_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindow
    \inmodule QtGui
    \since 5.0
    \brief The QWindow class represents a window in the underlying windowing system.

    A window that is supplied a parent becomes a native child window of
    their parent window.

    An application will typically use QWidget or QQuickView for its UI, and not
    QWindow directly. Still, it is possible to render directly to a QWindow
    with QBackingStore or QOpenGLContext, when wanting to keep dependencies to
    a minimum or when wanting to use OpenGL directly. The
    \l{Raster Window Example} and \l{OpenGL Window Example}
    are useful reference examples for how to render to a QWindow using
    either approach.

    \section1 Resource Management

    Windows can potentially use a lot of memory. A usual measurement is
    width times height times color depth. A window might also include multiple
    buffers to support double and triple buffering, as well as depth and stencil
    buffers. To release a window's memory resources, call the destroy() function.

    \section1 Content Orientation

    QWindow has reportContentOrientationChange() that can be used to specify
    the layout of the window contents in relation to the screen. The content
    orientation is simply a hint to the windowing system about which
    orientation the window contents are in.  It's useful when you wish to keep
    the same window size, but rotate the contents instead, especially when
    doing rotation animations between different orientations. The windowing
    system might use this value to determine the layout of system popups or
    dialogs.

    \section1 Visibility and Windowing System Exposure

    By default, the window is not visible, and you must call setVisible(true),
    or show() or similar to make it visible. To make a window hidden again,
    call setVisible(false) or hide(). The visible property describes the state
    the application wants the window to be in. Depending on the underlying
    system, a visible window might still not be shown on the screen. It could,
    for instance, be covered by other opaque windows or moved outside the
    physical area of the screen. On windowing systems that have exposure
    notifications, the isExposed() accessor describes whether the window should
    be treated as directly visible on screen. The exposeEvent() function is
    called whenever an area of the window is invalidated, for example due to the
    exposure in the windowing system changing. On windowing systems that do not
    make this information visible to the application, isExposed() will simply
    return the same value as isVisible().

    QWindow::Visibility queried through visibility() is a convenience API
    combining the functions of visible() and windowStates().

    \section1 Rendering

    There are two Qt APIs that can be used to render content into a window,
    QBackingStore for rendering with a QPainter and flushing the contents
    to a window with type QSurface::RasterSurface, and QOpenGLContext for
    rendering with OpenGL to a window with type QSurface::OpenGLSurface.

    The application can start rendering as soon as isExposed() returns \c true,
    and can keep rendering until it isExposed() returns \c false. To find out when
    isExposed() changes, reimplement exposeEvent(). The window will always get
    a resize event before the first expose event.

    \section1 Initial Geometry

    If the window's width and height are left uninitialized, the window will
    get a reasonable default geometry from the platform window. If the position
    is left uninitialized, then the platform window will allow the windowing
    system to position the window. For example on X11, the window manager
    usually does some kind of smart positioning to try to avoid having new
    windows completely obscure existing windows. However setGeometry()
    initializes both the position and the size, so if you want a fixed size but
    an automatic position, you should call resize() or setWidth() and
    setHeight() instead.
*/

/*!
    Creates a window as a top level on the \a targetScreen.

    The window is not shown until setVisible(true), show(), or similar is called.

    \sa setScreen()
*/
QWindow::QWindow(QScreen *targetScreen)
    : QObject(*new QWindowPrivate(), nullptr)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->init(targetScreen);
}

static QWindow *nonDesktopParent(QWindow *parent)
{
    if (parent && parent->type() == Qt::Desktop) {
        qWarning("QWindows cannot be reparented into desktop windows");
        return nullptr;
    }

    return parent;
}

/*!
    Creates a window as a child of the given \a parent window.

    The window will be embedded inside the parent window, its coordinates
    relative to the parent.

    The screen is inherited from the parent.

    \sa setParent()
*/
QWindow::QWindow(QWindow *parent)
    : QWindow(*new QWindowPrivate(), parent)
{
}

/*!
    Creates a window as a child of the given \a parent window with the \a dd
    private implementation.

    The window will be embedded inside the parent window, its coordinates
    relative to the parent.

    The screen is inherited from the parent.

    \internal
    \sa setParent()
*/
QWindow::QWindow(QWindowPrivate &dd, QWindow *parent)
    : QObject(dd, nonDesktopParent(parent))
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->init();
}

/*!
    Destroys the window.
*/
QWindow::~QWindow()
{
    Q_D(QWindow);
    d->destroy();
    QGuiApplicationPrivate::window_list.removeAll(this);
    if (!QGuiApplicationPrivate::is_app_closing)
        QGuiApplicationPrivate::instance()->modalWindowList.removeOne(this);

    // thse are normally cleared in destroy(), but the window may in
    // some cases end up becoming the focus window again, or receive an enter
    // event. Clear it again here as a workaround. See QTBUG-75326.
    if (QGuiApplicationPrivate::focus_window == this)
        QGuiApplicationPrivate::focus_window = nullptr;
    if (QGuiApplicationPrivate::currentMouseWindow == this)
        QGuiApplicationPrivate::currentMouseWindow = nullptr;
    if (QGuiApplicationPrivate::currentMousePressWindow == this)
        QGuiApplicationPrivate::currentMousePressWindow = nullptr;

    d->isWindow = false;
}

QWindowPrivate::QWindowPrivate()
    = default;

QWindowPrivate::~QWindowPrivate()
    = default;

void QWindowPrivate::init(QScreen *targetScreen)
{
    Q_Q(QWindow);

    isWindow = true;
    parentWindow = static_cast<QWindow *>(q->QObject::parent());

    QScreen *connectScreen = targetScreen ? targetScreen : QGuiApplication::primaryScreen();

    if (!parentWindow)
        connectToScreen(connectScreen);

    // If your application aborts here, you are probably creating a QWindow
    // before the screen list is populated.
    if (Q_UNLIKELY(!parentWindow && !topLevelScreen)) {
        qFatal("Cannot create window: no screens available");
    }
    QGuiApplicationPrivate::window_list.prepend(q);

    requestedFormat = QSurfaceFormat::defaultFormat();
    devicePixelRatio = connectScreen->devicePixelRatio();

    QObject::connect(q, &QWindow::screenChanged, q, [q, this](QScreen *){
        // We may have changed scaling; trigger resize event if needed,
        // except on Windows, where we send resize events during WM_DPICHANGED
        // event handling. FIXME: unify DPI change handling across all platforms.
#ifndef Q_OS_WIN
        if (q->handle()) {
            QWindowSystemInterfacePrivate::GeometryChangeEvent gce(q, QHighDpi::fromNativePixels(q->handle()->geometry(), q));
            QGuiApplicationPrivate::processGeometryChangeEvent(&gce);
        }
#else
        Q_UNUSED(q);
#endif
        updateDevicePixelRatio();
    });
}

/*!
    \enum QWindow::Visibility
    \since 5.1

    This enum describes what part of the screen the window occupies or should
    occupy.

    \value Windowed The window occupies part of the screen, but not necessarily
    the entire screen. This state will occur only on windowing systems which
    support showing multiple windows simultaneously. In this state it is
    possible for the user to move and resize the window manually, if
    WindowFlags permit it and if it is supported by the windowing system.

    \value Minimized The window is reduced to an entry or icon on the task bar,
    dock, task list or desktop, depending on how the windowing system handles
    minimized windows.

    \value Maximized The window occupies one entire screen, and the titlebar is
    still visible. On most windowing systems this is the state achieved by
    clicking the maximize button on the toolbar.

    \value FullScreen The window occupies one entire screen, is not resizable,
    and there is no titlebar. On some platforms which do not support showing
    multiple simultaneous windows, this can be the usual visibility when the
    window is not hidden.

    \value AutomaticVisibility This means to give the window a default visible
    state, which might be fullscreen or windowed depending on the platform.
    It can be given as a parameter to setVisibility but will never be
    read back from the visibility accessor.

    \value Hidden The window is not visible in any way, however it may remember
    a latent visibility which can be restored by setting AutomaticVisibility.
*/

/*!
    \property QWindow::visibility
    \brief the screen-occupation state of the window
    \since 5.1

    Visibility is whether the window should appear in the windowing system as
    normal, minimized, maximized, fullscreen or hidden.

    To set the visibility to AutomaticVisibility means to give the window
    a default visible state, which might be fullscreen or windowed depending on
    the platform.
    When reading the visibility property you will always get the actual state,
    never AutomaticVisibility.
*/
QWindow::Visibility QWindow::visibility() const
{
    Q_D(const QWindow);
    return d->visibility;
}

void QWindow::setVisibility(Visibility v)
{
    switch (v) {
    case Hidden:
        hide();
        break;
    case AutomaticVisibility:
        show();
        break;
    case Windowed:
        showNormal();
        break;
    case Minimized:
        showMinimized();
        break;
    case Maximized:
        showMaximized();
        break;
    case FullScreen:
        showFullScreen();
        break;
    default:
        Q_ASSERT(false);
    }
}

/*
    Subclasses may override this function to run custom setVisible
    logic. Subclasses that do so must call the base class implementation
    at some point to make the native window visible, and must not
    call QWindow::setVisble() since that will recurse back here.
*/
void QWindowPrivate::setVisible(bool visible)
{
    Q_Q(QWindow);

    if (this->visible != visible) {
        this->visible = visible;
        emit q->visibleChanged(visible);
        updateVisibility();
    } else if (platformWindow) {
        // Visibility hasn't changed, and the platform window is in sync
        return;
    }

    if (!platformWindow) {
        // If we have a parent window, but the parent hasn't been created yet, we
        // can defer creation until the parent is created or we're re-parented.
        if (parentWindow && !parentWindow->handle())
            return;

        // We only need to create the window if it's being shown
        if (visible) {
            // FIXME: At this point we've already updated the visible state of
            // the QWindow, so if the platform layer reads the window state during
            // creation, and reflects that in the native window, it will end up
            // with a visible window. This may in turn result in resize or expose
            // events from the platform before we have sent the show event below.
            q->create();
        }
    }

    if (visible) {
        // remove posted quit events when showing a new window
        QCoreApplication::removePostedEvents(qApp, QEvent::Quit);

        if (q->type() == Qt::Window) {
            QGuiApplicationPrivate *app_priv = QGuiApplicationPrivate::instance();
            QString &firstWindowTitle = app_priv->firstWindowTitle;
            if (!firstWindowTitle.isEmpty()) {
                q->setTitle(firstWindowTitle);
                firstWindowTitle = QString();
            }
            if (!app_priv->forcedWindowIcon.isNull())
                q->setIcon(app_priv->forcedWindowIcon);

            // Handling of the -qwindowgeometry, -geometry command line arguments
            static bool geometryApplied = false;
            if (!geometryApplied) {
                geometryApplied = true;
                QGuiApplicationPrivate::applyWindowGeometrySpecificationTo(q);
            }
        }

        QShowEvent showEvent;
        QGuiApplication::sendEvent(q, &showEvent);
    }

    if (q->isModal()) {
        if (visible)
            QGuiApplicationPrivate::showModalWindow(q);
        else
            QGuiApplicationPrivate::hideModalWindow(q);
    // QShapedPixmapWindow is used on some platforms for showing a drag pixmap, so don't block
    // input to this window as it is performing a drag - QTBUG-63846
    } else if (visible && QGuiApplication::modalWindow()
#if QT_CONFIG(draganddrop)
               && !qobject_cast<QShapedPixmapWindow *>(q)
#endif // QT_CONFIG(draganddrop)
              ) {
        QGuiApplicationPrivate::updateBlockedStatus(q);
    }

#ifndef QT_NO_CURSOR
    if (visible && (hasCursor || QGuiApplication::overrideCursor()))
        applyCursor();
#endif

    if (platformWindow)
        platformWindow->setVisible(visible);

    if (!visible) {
        QHideEvent hideEvent;
        QGuiApplication::sendEvent(q, &hideEvent);
    }
}

void QWindowPrivate::updateVisibility()
{
    Q_Q(QWindow);

    QWindow::Visibility old = visibility;

    if (!visible)
        visibility = QWindow::Hidden;
    else if (windowState & Qt::WindowMinimized)
        visibility = QWindow::Minimized;
    else if (windowState & Qt::WindowFullScreen)
        visibility = QWindow::FullScreen;
    else if (windowState & Qt::WindowMaximized)
        visibility = QWindow::Maximized;
    else
        visibility = QWindow::Windowed;

    if (visibility != old)
        emit q->visibilityChanged(visibility);
}

void QWindowPrivate::updateSiblingPosition(SiblingPosition position)
{
    Q_Q(QWindow);

    if (!q->parent())
        return;

    QObjectList &siblings = q->parent()->d_ptr->children;

    const qsizetype siblingCount = siblings.size() - 1;
    if (siblingCount == 0)
        return;

    const qsizetype currentPosition = siblings.indexOf(q);
    Q_ASSERT(currentPosition >= 0);

    const qsizetype targetPosition = position == PositionTop ? siblingCount : 0;

    if (currentPosition == targetPosition)
        return;

    siblings.move(currentPosition, targetPosition);
}

bool QWindowPrivate::windowRecreationRequired(QScreen *newScreen) const
{
    Q_Q(const QWindow);
    const QScreen *oldScreen = q->screen();
    return oldScreen != newScreen && (platformWindow || !oldScreen)
        && !(oldScreen && oldScreen->virtualSiblings().contains(newScreen));
}

void QWindowPrivate::disconnectFromScreen()
{
    if (topLevelScreen)
        topLevelScreen = nullptr;
}

void QWindowPrivate::connectToScreen(QScreen *screen)
{
    disconnectFromScreen();
    topLevelScreen = screen;
}

void QWindowPrivate::emitScreenChangedRecursion(QScreen *newScreen)
{
    Q_Q(QWindow);
    emit q->screenChanged(newScreen);
    for (QObject *child : q->children()) {
        if (child->isWindowType())
            static_cast<QWindow *>(child)->d_func()->emitScreenChangedRecursion(newScreen);
    }
}

void QWindowPrivate::setTopLevelScreen(QScreen *newScreen, bool recreate)
{
    Q_Q(QWindow);
    if (parentWindow) {
        qWarning() << q << '(' << newScreen << "): Attempt to set a screen on a child window.";
        return;
    }
    if (newScreen != topLevelScreen) {
        const bool shouldRecreate = recreate && windowRecreationRequired(newScreen);
        const bool shouldShow = visibilityOnDestroy && !topLevelScreen;
        if (shouldRecreate && platformWindow)
            q->destroy();
        connectToScreen(newScreen);
        if (shouldShow)
            q->setVisible(true);
        else if (newScreen && shouldRecreate)
            create(true);
        emitScreenChangedRecursion(newScreen);
    }
}

void QWindowPrivate::create(bool recursive, WId nativeHandle)
{
    Q_Q(QWindow);
    if (platformWindow)
        return;

    // avoid losing update requests when re-creating
    const bool needsUpdate = updateRequestPending;
    // the platformWindow, if there was one, is now gone, so make this flag reflect reality now
    updateRequestPending = false;

    if (q->parent())
        q->parent()->create();

    if (platformWindow) {
        // Creating the parent window will end up creating any child window
        // that was already visible, via setVisible. If this applies to us,
        // we will already have a platform window at this point.
        return;
    }

    // QPlatformWindow will poll geometry() during construction below. Set the
    // screen here so that high-dpi scaling will use the correct scale factor.
    if (q->isTopLevel()) {
        if (QScreen *screen = screenForGeometry(geometry))
            setTopLevelScreen(screen, false);
    }

    QPlatformIntegration *platformIntegration = QGuiApplicationPrivate::platformIntegration();
    platformWindow = nativeHandle ? platformIntegration->createForeignWindow(q, nativeHandle)
        : platformIntegration->createPlatformWindow(q);
    Q_ASSERT(platformWindow);

    if (!platformWindow) {
        qWarning() << "Failed to create platform window for" << q << "with flags" << q->flags();
        return;
    }

    platformWindow->initialize();

    QObjectList childObjects = q->children();
    for (int i = 0; i < childObjects.size(); i ++) {
        QObject *object = childObjects.at(i);
        if (!object->isWindowType())
            continue;

        QWindow *childWindow = static_cast<QWindow *>(object);
        if (recursive)
            childWindow->d_func()->create(recursive);

        // The child may have had deferred creation due to this window not being created
        // at the time setVisible was called, so we re-apply the visible state, which
        // may result in creating the child, and emitting the appropriate signals.
        if (childWindow->isVisible())
            childWindow->setVisible(true);

        if (QPlatformWindow *childPlatformWindow = childWindow->d_func()->platformWindow)
            childPlatformWindow->setParent(this->platformWindow);
    }

    QPlatformSurfaceEvent e(QPlatformSurfaceEvent::SurfaceCreated);
    QGuiApplication::sendEvent(q, &e);

    updateDevicePixelRatio();

    if (needsUpdate)
        q->requestUpdate();
}

void QWindowPrivate::clearFocusObject()
{
}

// Allows for manipulating the suggested geometry before a resize/move
// event in derived classes for platforms that support it, for example to
// implement heightForWidth().
QRectF QWindowPrivate::closestAcceptableGeometry(const QRectF &rect) const
{
    Q_UNUSED(rect);
    return QRectF();
}

void QWindowPrivate::setMinOrMaxSize(QSize *oldSizeMember, const QSize &size,
                                     qxp::function_ref<void()> funcWidthChanged,
                                     qxp::function_ref<void()> funcHeightChanged)
{
    Q_Q(QWindow);
    Q_ASSERT(oldSizeMember);
    const QSize adjustedSize =
            size.expandedTo(QSize(0, 0)).boundedTo(QSize(QWINDOWSIZE_MAX, QWINDOWSIZE_MAX));
    if (*oldSizeMember == adjustedSize)
        return;
    const bool widthChanged = adjustedSize.width() != oldSizeMember->width();
    const bool heightChanged = adjustedSize.height() != oldSizeMember->height();
    *oldSizeMember = adjustedSize;

    if (platformWindow && q->isTopLevel())
        platformWindow->propagateSizeHints();

    if (widthChanged)
        funcWidthChanged();
    if (heightChanged)
        funcHeightChanged();

    // resize window if current size is outside of min and max limits
    if (minimumSize.width() <= maximumSize.width()
        || minimumSize.height() <= maximumSize.height()) {
        const QSize currentSize = q->size();
        const QSize boundedSize = currentSize.expandedTo(minimumSize).boundedTo(maximumSize);
        if (currentSize != boundedSize)
            q->resize(boundedSize);
    }
}

/*!
    Sets the \a surfaceType of the window.

    Specifies whether the window is meant for raster rendering with
    QBackingStore, or OpenGL rendering with QOpenGLContext.

    The surfaceType will be used when the native surface is created
    in the create() function. Calling this function after the native
    surface has been created requires calling destroy() and create()
    to release the old native surface and create a new one.

    \sa QBackingStore, QOpenGLContext, create(), destroy()
*/
void QWindow::setSurfaceType(SurfaceType surfaceType)
{
    Q_D(QWindow);
    d->surfaceType = surfaceType;
}

/*!
    Returns the surface type of the window.

    \sa setSurfaceType()
*/
QWindow::SurfaceType QWindow::surfaceType() const
{
    Q_D(const QWindow);
    return d->surfaceType;
}

/*!
    \property QWindow::visible
    \brief whether the window is visible or not

    This property controls the visibility of the window in the windowing system.

    By default, the window is not visible, you must call setVisible(true), or
    show() or similar to make it visible.

    \note Hiding a window does not remove the window from the windowing system,
    it only hides it. On windowing systems that give full screen applications a
    dedicated desktop (such as macOS), hiding a full screen window will not remove
    that desktop, but leave it blank. Another window from the same application
    might be shown full screen, and will fill that desktop. Use QWindow::close to
    completely remove a window from the windowing system.

    \sa show()
*/
void QWindow::setVisible(bool visible)
{
    Q_D(QWindow);

    d->setVisible(visible);
}

bool QWindow::isVisible() const
{
    Q_D(const QWindow);

    return d->visible;
}

/*!
    Allocates the platform resources associated with the window.

    It is at this point that the surface format set using setFormat() gets resolved
    into an actual native surface. However, the window remains hidden until setVisible() is called.

    Note that it is not usually necessary to call this function directly, as it will be implicitly
    called by show(), setVisible(), winId(), and other functions that require access to the platform
    resources.

    Call destroy() to free the platform resources if necessary.

    \sa destroy()
*/
void QWindow::create()
{
    Q_D(QWindow);
    d->create(false);
}

/*!
    Returns the window's platform id.

    \note This function will cause the platform window to be created if it is not already.
    Returns 0, if the platform window creation failed.

    For platforms where this id might be useful, the value returned
    will uniquely represent the window inside the corresponding screen.

    \sa screen()
*/
WId QWindow::winId() const
{
    Q_D(const QWindow);

    if (!d->platformWindow)
        const_cast<QWindow *>(this)->create();

    if (!d->platformWindow)
        return 0;

    return d->platformWindow->winId();
}

 /*!
    Returns the parent window, if any.

    If \a mode is IncludeTransients, then the transient parent is returned
    if there is no parent.

    A window without a parent is known as a top level window.

    \since 5.9
*/
QWindow *QWindow::parent(AncestorMode mode) const
{
    Q_D(const QWindow);
    return d->parentWindow ? d->parentWindow : (mode == IncludeTransients ? transientParent() : nullptr);
}

/*!
    Sets the \a parent Window. This will lead to the windowing system managing
    the clip of the window, so it will be clipped to the \a parent window.

    Setting \a parent to be \nullptr will make the window become a top level
    window.

    If \a parent is a window created by fromWinId(), then the current window
    will be embedded inside \a parent, if the platform supports it.
*/
void QWindow::setParent(QWindow *parent)
{
    parent = nonDesktopParent(parent);

    Q_D(QWindow);
    if (d->parentWindow == parent)
        return;

    QScreen *newScreen = parent ? parent->screen() : screen();
    if (d->windowRecreationRequired(newScreen)) {
        qWarning() << this << '(' << parent << "): Cannot change screens (" << screen() << newScreen << ')';
        return;
    }

    QObject::setParent(parent);
    d->parentWindow = parent;

    if (parent)
        d->disconnectFromScreen();
    else
        d->connectToScreen(newScreen);

    // If we were set visible, but not created because we were a child, and we're now
    // re-parented into a created parent, or to being a top level, we need re-apply the
    // visibility state, which will also create.
    if (isVisible() && (!parent || parent->handle()))
        setVisible(true);

    if (d->platformWindow) {
        if (parent)
            parent->create();

        d->platformWindow->setParent(parent ? parent->d_func()->platformWindow : nullptr);
    }

    QGuiApplicationPrivate::updateBlockedStatus(this);
}

/*!
    Returns whether the window is top level, i.e. has no parent window.
*/
bool QWindow::isTopLevel() const
{
    Q_D(const QWindow);
    return d->parentWindow == nullptr;
}

/*!
    Returns whether the window is modal.

    A modal window prevents other windows from getting any input.

    \sa QWindow::modality
*/
bool QWindow::isModal() const
{
    Q_D(const QWindow);
    return d->modality != Qt::NonModal;
}

/*! \property QWindow::modality
    \brief the modality of the window

    A modal window prevents other windows from receiving input events. Qt
    supports two types of modality: Qt::WindowModal and Qt::ApplicationModal.

    By default, this property is Qt::NonModal

    \sa Qt::WindowModality
*/

Qt::WindowModality QWindow::modality() const
{
    Q_D(const QWindow);
    return d->modality;
}

void QWindow::setModality(Qt::WindowModality modality)
{
    Q_D(QWindow);
    if (d->modality == modality)
        return;
    d->modality = modality;
    emit modalityChanged(modality);
}

/*! \fn void QWindow::modalityChanged(Qt::WindowModality modality)

    This signal is emitted when the Qwindow::modality property changes to \a modality.
*/

/*!
    Sets the window's surface \a format.

    The format determines properties such as color depth, alpha, depth and
    stencil buffer size, etc. For example, to give a window a transparent
    background (provided that the window system supports compositing, and
    provided that other content in the window does not make it opaque again):

    \code
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    window.setFormat(format);
    \endcode

    The surface format will be resolved in the create() function. Calling
    this function after create() has been called will not re-resolve the
    surface format of the native surface.

    When the format is not explicitly set via this function, the format returned
    by QSurfaceFormat::defaultFormat() will be used. This means that when having
    multiple windows, individual calls to this function can be replaced by one
    single call to QSurfaceFormat::setDefaultFormat() before creating the first
    window.

    \sa create(), destroy(), QSurfaceFormat::setDefaultFormat()
*/
void QWindow::setFormat(const QSurfaceFormat &format)
{
    Q_D(QWindow);
    d->requestedFormat = format;
}

/*!
    Returns the requested surface format of this window.

    If the requested format was not supported by the platform implementation,
    the requestedFormat will differ from the actual window format.

    This is the value set with setFormat().

    \sa setFormat(), format()
 */
QSurfaceFormat QWindow::requestedFormat() const
{
    Q_D(const QWindow);
    return d->requestedFormat;
}

/*!
    Returns the actual format of this window.

    After the window has been created, this function will return the actual surface format
    of the window. It might differ from the requested format if the requested format could
    not be fulfilled by the platform. It might also be a superset, for example certain
    buffer sizes may be larger than requested.

    \note Depending on the platform, certain values in this surface format may still
    contain the requested values, that is, the values that have been passed to
    setFormat(). Typical examples are the OpenGL version, profile and options. These may
    not get updated during create() since these are context specific and a single window
    may be used together with multiple contexts over its lifetime. Use the
    QOpenGLContext's format() instead to query such values.

    \sa create(), requestedFormat(), QOpenGLContext::format()
*/
QSurfaceFormat QWindow::format() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->format();
    return d->requestedFormat;
}

/*!
    \property QWindow::flags
    \brief the window flags of the window

    The window flags control the window's appearance in the windowing system,
    whether it's a dialog, popup, or a regular window, and whether it should
    have a title bar, etc.

    The actual window flags might differ from the flags set with setFlags()
    if the requested flags could not be fulfilled.

    \sa setFlag()
*/
void QWindow::setFlags(Qt::WindowFlags flags)
{
    Q_D(QWindow);
    if (d->windowFlags == flags)
        return;

    if (d->platformWindow)
        d->platformWindow->setWindowFlags(flags);
    d->windowFlags = flags;
}

Qt::WindowFlags QWindow::flags() const
{
    Q_D(const QWindow);
    Qt::WindowFlags flags = d->windowFlags;

    if (d->platformWindow && d->platformWindow->isForeignWindow())
        flags |= Qt::ForeignWindow;

    return flags;
}

/*!
    \since 5.9

    Sets the window flag \a flag on this window if \a on is true;
    otherwise clears the flag.

    \sa setFlags(), flags(), type()
*/
void QWindow::setFlag(Qt::WindowType flag, bool on)
{
    Q_D(QWindow);
    if (on)
        setFlags(d->windowFlags | flag);
    else
        setFlags(d->windowFlags & ~flag);
}

/*!
    Returns the type of the window.

    This returns the part of the window flags that represents
    whether the window is a dialog, tooltip, popup, regular window, etc.

    \sa flags(), setFlags()
*/
Qt::WindowType QWindow::type() const
{
    return static_cast<Qt::WindowType>(int(flags() & Qt::WindowType_Mask));
}

/*!
    \property QWindow::title
    \brief the window's title in the windowing system

    The window title might appear in the title area of the window decorations,
    depending on the windowing system and the window flags. It might also
    be used by the windowing system to identify the window in other contexts,
    such as in the task switcher.

    \sa flags()
*/
void QWindow::setTitle(const QString &title)
{
    Q_D(QWindow);
    bool changed = false;
    if (d->windowTitle != title) {
        d->windowTitle = title;
        changed = true;
    }
    if (d->platformWindow && type() != Qt::Desktop)
        d->platformWindow->setWindowTitle(title);
    if (changed)
        emit windowTitleChanged(title);
}

QString QWindow::title() const
{
    Q_D(const QWindow);
    return d->windowTitle;
}

/*!
    \brief set the file name this window is representing.

    The windowing system might use \a filePath to display the
    path of the document this window is representing in the tile bar.

*/
void QWindow::setFilePath(const QString &filePath)
{
    Q_D(QWindow);
    d->windowFilePath = filePath;
    if (d->platformWindow)
        d->platformWindow->setWindowFilePath(filePath);
}

/*!
    \brief the file name this window is representing.

    \sa setFilePath()
*/
QString QWindow::filePath() const
{
    Q_D(const QWindow);
    return d->windowFilePath;
}

/*!
    \brief Sets the window's \a icon in the windowing system

    The window icon might be used by the windowing system for example to
    decorate the window, and/or in the task switcher.

    \note On \macos, the window title bar icon is meant for windows representing
    documents, and will only show up if a file path is also set.

    \sa setFilePath()
*/
void QWindow::setIcon(const QIcon &icon)
{
    Q_D(QWindow);
    d->windowIcon = icon;
    if (d->platformWindow)
        d->platformWindow->setWindowIcon(icon);
    QEvent e(QEvent::WindowIconChange);
    QCoreApplication::sendEvent(this, &e);
}

/*!
    \brief Returns the window's icon in the windowing system

    \sa setIcon()
*/
QIcon QWindow::icon() const
{
    Q_D(const QWindow);
    if (d->windowIcon.isNull())
        return QGuiApplication::windowIcon();
    return d->windowIcon;
}

/*!
    Raise the window in the windowing system.

    Requests that the window be raised to appear above other windows.
*/
void QWindow::raise()
{
    Q_D(QWindow);

    d->updateSiblingPosition(QWindowPrivate::PositionTop);

    if (d->platformWindow)
        d->platformWindow->raise();
}

/*!
    Lower the window in the windowing system.

    Requests that the window be lowered to appear below other windows.
*/
void QWindow::lower()
{
    Q_D(QWindow);

    d->updateSiblingPosition(QWindowPrivate::PositionBottom);

    if (d->platformWindow)
        d->platformWindow->lower();
}

/*!
    \brief Start a system-specific resize operation
    \since 5.15

    Calling this will start an interactive resize operation on the window by platforms
    that support it. The actual behavior may vary depending on the platform. Usually,
    it will make the window resize so that its edge follows the mouse cursor.

    On platforms that support it, this method of resizing windows is preferred over
    \c setGeometry, because it allows a more native look and feel of resizing windows, e.g.
    letting the window manager snap this window against other windows, or special resizing
    behavior with animations when dragged to the edge of the screen.

    \a edges should either be a single edge, or two adjacent edges (a corner). Other values
    are not allowed.

    Returns true if the operation was supported by the system.
*/
bool QWindow::startSystemResize(Qt::Edges edges)
{
    Q_D(QWindow);
    if (Q_UNLIKELY(!isVisible() || !d->platformWindow || d->maximumSize == d->minimumSize))
        return false;

    const bool isSingleEdge = edges == Qt::TopEdge || edges == Qt::RightEdge || edges == Qt::BottomEdge || edges == Qt::LeftEdge;
    const bool isCorner =
            edges == (Qt::TopEdge | Qt::LeftEdge) ||
            edges == (Qt::TopEdge | Qt::RightEdge) ||
            edges == (Qt::BottomEdge | Qt::RightEdge) ||
            edges == (Qt::BottomEdge | Qt::LeftEdge);

    if (Q_UNLIKELY(!isSingleEdge && !isCorner)) {
        qWarning() << "Invalid edges" << edges << "passed to QWindow::startSystemResize, ignoring.";
        return false;
    }

    return d->platformWindow->startSystemResize(edges);
}

/*!
    \brief Start a system-specific move operation
    \since 5.15

    Calling this will start an interactive move operation on the window by platforms
    that support it. The actual behavior may vary depending on the platform. Usually,
    it will make the window follow the mouse cursor until a mouse button is released.

    On platforms that support it, this method of moving windows is preferred over
    \c setPosition, because it allows a more native look-and-feel of moving windows, e.g.
    letting the window manager snap this window against other windows, or special tiling
    or resizing behavior with animations when dragged to the edge of the screen.
    Furthermore, on some platforms such as Wayland, \c setPosition is not supported, so
    this is the only way the application can influence its position.

    Returns true if the operation was supported by the system.
*/
bool QWindow::startSystemMove()
{
    Q_D(QWindow);
    if (Q_UNLIKELY(!isVisible() || !d->platformWindow))
        return false;

    return d->platformWindow->startSystemMove();
}

/*!
    \property QWindow::opacity
    \brief The opacity of the window in the windowing system.
    \since 5.1

    If the windowing system supports window opacity, this can be used to fade the
    window in and out, or to make it semitransparent.

    A value of 1.0 or above is treated as fully opaque, whereas a value of 0.0 or below
    is treated as fully transparent. Values inbetween represent varying levels of
    translucency between the two extremes.

    The default value is 1.0.
*/
void QWindow::setOpacity(qreal level)
{
    Q_D(QWindow);
    if (level == d->opacity)
        return;
    d->opacity = level;
    if (d->platformWindow) {
        d->platformWindow->setOpacity(level);
        emit opacityChanged(level);
    }
}

qreal QWindow::opacity() const
{
    Q_D(const QWindow);
    return d->opacity;
}

/*!
    Sets the mask of the window.

    The mask is a hint to the windowing system that the application does not
    want to receive mouse or touch input outside the given \a region.

    The window manager may or may not choose to display any areas of the window
    not included in the mask, thus it is the application's responsibility to
    clear to transparent the areas that are not part of the mask.
*/
void QWindow::setMask(const QRegion &region)
{
    Q_D(QWindow);
    if (d->platformWindow)
        d->platformWindow->setMask(QHighDpi::toNativeLocalRegion(region, this));
    d->mask = region;
}

/*!
    Returns the mask set on the window.

    The mask is a hint to the windowing system that the application does not
    want to receive mouse or touch input outside the given region.
*/
QRegion QWindow::mask() const
{
    Q_D(const QWindow);
    return d->mask;
}

/*!
    Requests the window to be activated, i.e. receive keyboard focus.

    \sa isActive(), QGuiApplication::focusWindow()
*/
void QWindow::requestActivate()
{
    Q_D(QWindow);
    if (flags() & Qt::WindowDoesNotAcceptFocus) {
        qWarning() << "requestActivate() called for " << this << " which has Qt::WindowDoesNotAcceptFocus set.";
        return;
    }
    if (d->platformWindow)
        d->platformWindow->requestActivateWindow();
}

/*!
    Returns if this window is exposed in the windowing system.

    When the window is not exposed, it is shown by the application
    but it is still not showing in the windowing system, so the application
    should minimize animations and other graphical activities.

    An exposeEvent() is sent every time this value changes.

    \sa exposeEvent()
*/
bool QWindow::isExposed() const
{
    Q_D(const QWindow);
    return d->exposed;
}

/*!
    \property QWindow::active
    \brief the active status of the window
    \since 5.1

    \sa requestActivate()
*/

/*!
    Returns \c true if the window is active.

    This is the case for the window that has input focus as well as windows
    that are in the same parent / transient parent chain as the focus window.

    Typically active windows should appear active from a style perspective.

    To get the window that currently has focus, use QGuiApplication::focusWindow().

    \sa requestActivate()
*/
bool QWindow::isActive() const
{
    Q_D(const QWindow);
    if (!d->platformWindow)
        return false;

    QWindow *focus = QGuiApplication::focusWindow();

    // Means the whole application lost the focus
    if (!focus)
        return false;

    if (focus == this)
        return true;

    if (QWindow *p = parent(IncludeTransients))
        return p->isActive();
    else
        return isAncestorOf(focus);
}

/*!
    \property QWindow::contentOrientation
    \brief the orientation of the window's contents

    This is a hint to the window manager in case it needs to display
    additional content like popups, dialogs, status bars, or similar
    in relation to the window.

    The recommended orientation is QScreen::orientation() but
    an application doesn't have to support all possible orientations,
    and thus can opt to ignore the current screen orientation.

    The difference between the window and the content orientation
    determines how much to rotate the content by. QScreen::angleBetween(),
    QScreen::transformBetween(), and QScreen::mapBetween() can be used
    to compute the necessary transform.

    The default value is Qt::PrimaryOrientation
*/
void QWindow::reportContentOrientationChange(Qt::ScreenOrientation orientation)
{
    Q_D(QWindow);
    if (d->contentOrientation == orientation)
        return;
    if (d->platformWindow)
        d->platformWindow->handleContentOrientationChange(orientation);
    d->contentOrientation = orientation;
    emit contentOrientationChanged(orientation);
}

Qt::ScreenOrientation QWindow::contentOrientation() const
{
    Q_D(const QWindow);
    return d->contentOrientation;
}

/*!
    Returns the ratio between physical pixels and device-independent pixels
    for the window. This value is dependent on the screen the window is on,
    and may change when the window is moved.

    Common values are 1.0 on normal displays and 2.0 on Apple "retina" displays.

    \note For windows not backed by a platform window, meaning that create() was not
    called, the function will fall back to the associated QScreen's device pixel ratio.

    \sa QScreen::devicePixelRatio()
*/
qreal QWindow::devicePixelRatio() const
{
    Q_D(const QWindow);
    return d->devicePixelRatio;
}

/*
    Updates the cached devicePixelRatio value by polling for a new value.
    Sends QEvent::DevicePixelRatioChange to the window if the DPR has changed.
    Returns true if the DPR was changed.
*/
bool QWindowPrivate::updateDevicePixelRatio()
{
    Q_Q(QWindow);

    // If there is no platform window use the associated screen's devicePixelRatio,
    // which typically is the primary screen and will be correct for single-display
    // systems (a very common case).
    const qreal newDevicePixelRatio = platformWindow ?
        platformWindow->devicePixelRatio() * QHighDpiScaling::factor(q) : q->screen()->devicePixelRatio();

    if (newDevicePixelRatio == devicePixelRatio)
        return false;

    devicePixelRatio = newDevicePixelRatio;
    QEvent dprChangeEvent(QEvent::DevicePixelRatioChange);
    QGuiApplication::sendEvent(q, &dprChangeEvent);
    return true;
}

Qt::WindowState QWindowPrivate::effectiveState(Qt::WindowStates state)
{
    if (state & Qt::WindowMinimized)
        return Qt::WindowMinimized;
    else if (state & Qt::WindowFullScreen)
        return Qt::WindowFullScreen;
    else if (state & Qt::WindowMaximized)
        return Qt::WindowMaximized;
    return Qt::WindowNoState;
}

/*!
    \brief set the screen-occupation state of the window

    The window \a state represents whether the window appears in the
    windowing system as maximized, minimized, fullscreen, or normal.

    The enum value Qt::WindowActive is not an accepted parameter.

    \sa showNormal(), showFullScreen(), showMinimized(), showMaximized(), setWindowStates()
*/
void QWindow::setWindowState(Qt::WindowState state)
{
    setWindowStates(state);
}

/*!
    \brief set the screen-occupation state of the window
    \since 5.10

    The window \a state represents whether the window appears in the
    windowing system as maximized, minimized and/or fullscreen.

    The window can be in a combination of several states. For example, if
    the window is both minimized and maximized, the window will appear
    minimized, but clicking on the task bar entry will restore it to the
    maximized state.

    The enum value Qt::WindowActive should not be set.

    \sa showNormal(), showFullScreen(), showMinimized(), showMaximized()
 */
void QWindow::setWindowStates(Qt::WindowStates state)
{
    Q_D(QWindow);
    if (state & Qt::WindowActive) {
        qWarning("QWindow::setWindowStates does not accept Qt::WindowActive");
        state &= ~Qt::WindowActive;
    }

    if (d->platformWindow)
        d->platformWindow->setWindowState(state);

    auto originalEffectiveState = QWindowPrivate::effectiveState(d->windowState);
    d->windowState = state;
    auto newEffectiveState = QWindowPrivate::effectiveState(d->windowState);
    if (newEffectiveState != originalEffectiveState)
        emit windowStateChanged(newEffectiveState);

    d->updateVisibility();
}

/*!
    \brief the screen-occupation state of the window

    \sa setWindowState(), windowStates()
*/
Qt::WindowState QWindow::windowState() const
{
    Q_D(const QWindow);
    return QWindowPrivate::effectiveState(d->windowState);
}

/*!
    \brief the screen-occupation state of the window
    \since 5.10

    The window can be in a combination of several states. For example, if
    the window is both minimized and maximized, the window will appear
    minimized, but clicking on the task bar entry will restore it to
    the maximized state.

    \sa setWindowStates()
*/
Qt::WindowStates QWindow::windowStates() const
{
    Q_D(const QWindow);
    return d->windowState;
}

/*!
    \fn QWindow::windowStateChanged(Qt::WindowState windowState)

    This signal is emitted when the \a windowState changes, either
    by being set explicitly with setWindowStates(), or automatically when
    the user clicks one of the titlebar buttons or by other means.
*/

/*!
    \property QWindow::transientParent
    \brief the window for which this window is a transient pop-up
    \since 5.13

    This is a hint to the window manager that this window is a dialog or pop-up
    on behalf of the transient parent.

    In order to cause the window to be centered above its transient \a parent by
    default, depending on the window manager, it may also be necessary to call
    setFlags() with a suitable \l Qt::WindowType (such as \c Qt::Dialog).

    \sa parent()
*/
void QWindow::setTransientParent(QWindow *parent)
{
    Q_D(QWindow);
    if (parent && !parent->isTopLevel()) {
        qWarning() << parent << "must be a top level window.";
        return;
    }
    if (parent == this) {
        qWarning() << "transient parent" << parent << "cannot be same as window";
        return;
    }

    d->transientParent = parent;

    QGuiApplicationPrivate::updateBlockedStatus(this);
    emit transientParentChanged(parent);
}

QWindow *QWindow::transientParent() const
{
    Q_D(const QWindow);
    return d->transientParent.data();
}

/*
    The setter for the QWindow::transientParent property.
    The only reason this exists is to set the transientParentPropertySet flag
    so that Qt Quick knows whether it was set programmatically (because of
    Window declaration context) or because the user set the property.
*/
void QWindowPrivate::setTransientParent(QWindow *parent)
{
    Q_Q(QWindow);
    q->setTransientParent(parent);
    transientParentPropertySet = true;
}

/*!
    \enum QWindow::AncestorMode

    This enum is used to control whether or not transient parents
    should be considered ancestors.

    \value ExcludeTransients Transient parents are not considered ancestors.
    \value IncludeTransients Transient parents are considered ancestors.
*/

/*!
    Returns \c true if the window is an ancestor of the given \a child. If \a mode
    is IncludeTransients, then transient parents are also considered ancestors.
*/
bool QWindow::isAncestorOf(const QWindow *child, AncestorMode mode) const
{
    if (child->parent() == this || (mode == IncludeTransients && child->transientParent() == this))
        return true;

    if (QWindow *parent = child->parent(mode)) {
        if (isAncestorOf(parent, mode))
            return true;
    } else if (handle() && child->handle()) {
        if (handle()->isAncestorOf(child->handle()))
            return true;
    }

    return false;
}

/*!
    Returns the minimum size of the window.

    \sa setMinimumSize()
*/
QSize QWindow::minimumSize() const
{
    Q_D(const QWindow);
    return d->minimumSize;
}

/*!
    Returns the maximum size of the window.

    \sa setMaximumSize()
*/
QSize QWindow::maximumSize() const
{
    Q_D(const QWindow);
    return d->maximumSize;
}

/*!
    Returns the base size of the window.

    \sa setBaseSize()
*/
QSize QWindow::baseSize() const
{
    Q_D(const QWindow);
    return d->baseSize;
}

/*!
    Returns the size increment of the window.

    \sa setSizeIncrement()
*/
QSize QWindow::sizeIncrement() const
{
    Q_D(const QWindow);
    return d->sizeIncrement;
}

/*!
    Sets the minimum size of the window.

    This is a hint to the window manager to prevent resizing below the specified \a size.

    \sa setMaximumSize(), minimumSize()
*/
void QWindow::setMinimumSize(const QSize &size)
{
    Q_D(QWindow);
    d->setMinOrMaxSize(
            &d->minimumSize, size, [this, d]() { emit minimumWidthChanged(d->minimumSize.width()); },
            [this, d]() { emit minimumHeightChanged(d->minimumSize.height()); });
}

/*!
    \property QWindow::x
    \brief the x position of the window's geometry
*/
void QWindow::setX(int arg)
{
    Q_D(QWindow);
    if (x() != arg)
        setGeometry(QRect(arg, y(), width(), height()));
    else
        d->positionAutomatic = false;
}

/*!
    \property QWindow::y
    \brief the y position of the window's geometry
*/
void QWindow::setY(int arg)
{
    Q_D(QWindow);
    if (y() != arg)
        setGeometry(QRect(x(), arg, width(), height()));
    else
        d->positionAutomatic = false;
}

/*!
    \property QWindow::width
    \brief the width of the window's geometry
*/
void QWindow::setWidth(int arg)
{
    if (width() != arg)
        resize(arg, height());
}

/*!
    \property QWindow::height
    \brief the height of the window's geometry
*/
void QWindow::setHeight(int arg)
{
    if (height() != arg)
        resize(width(), arg);
}

/*!
    \property QWindow::minimumWidth
    \brief the minimum width of the window's geometry
*/
void QWindow::setMinimumWidth(int w)
{
    setMinimumSize(QSize(w, minimumHeight()));
}

/*!
    \property QWindow::minimumHeight
    \brief the minimum height of the window's geometry
*/
void QWindow::setMinimumHeight(int h)
{
    setMinimumSize(QSize(minimumWidth(), h));
}

/*!
    Sets the maximum size of the window.

    This is a hint to the window manager to prevent resizing above the specified \a size.

    \sa setMinimumSize(), maximumSize()
*/
void QWindow::setMaximumSize(const QSize &size)
{
    Q_D(QWindow);
    d->setMinOrMaxSize(
            &d->maximumSize, size, [this, d]() { emit maximumWidthChanged(d->maximumSize.width()); },
            [this, d]() { emit maximumHeightChanged(d->maximumSize.height()); });
}

/*!
    \property QWindow::maximumWidth
    \brief the maximum width of the window's geometry
*/
void QWindow::setMaximumWidth(int w)
{
    setMaximumSize(QSize(w, maximumHeight()));
}

/*!
    \property QWindow::maximumHeight
    \brief the maximum height of the window's geometry
*/
void QWindow::setMaximumHeight(int h)
{
    setMaximumSize(QSize(maximumWidth(), h));
}

/*!
    Sets the base \a size of the window.

    The base size is used to calculate a proper window size if the
    window defines sizeIncrement().

    \sa setMinimumSize(), setMaximumSize(), setSizeIncrement(), baseSize()
*/
void QWindow::setBaseSize(const QSize &size)
{
    Q_D(QWindow);
    if (d->baseSize == size)
        return;
    d->baseSize = size;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

/*!
    Sets the size increment (\a size) of the window.

    When the user resizes the window, the size will move in steps of
    sizeIncrement().width() pixels horizontally and
    sizeIncrement().height() pixels vertically, with baseSize() as the
    basis.

    By default, this property contains a size with zero width and height.

    The windowing system might not support size increments.

    \sa setBaseSize(), setMinimumSize(), setMaximumSize()
*/
void QWindow::setSizeIncrement(const QSize &size)
{
    Q_D(QWindow);
    if (d->sizeIncrement == size)
        return;
    d->sizeIncrement = size;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

/*!
    Sets the geometry of the window, excluding its window frame, to a
    rectangle constructed from \a posx, \a posy, \a w and \a h.

    The geometry is in relation to the virtualGeometry() of its screen.

    \sa geometry()
*/
void QWindow::setGeometry(int posx, int posy, int w, int h)
{
    setGeometry(QRect(posx, posy, w, h));
}

/*!
    \brief Sets the geometry of the window, excluding its window frame, to \a rect.

    The geometry is in relation to the virtualGeometry() of its screen.

    \sa geometry()
*/
void QWindow::setGeometry(const QRect &rect)
{
    Q_D(QWindow);
    d->positionAutomatic = false;
    const QRect oldRect = geometry();
    if (rect == oldRect)
        return;

    d->positionPolicy = QWindowPrivate::WindowFrameExclusive;
    if (d->platformWindow) {
        QScreen *newScreen = d->screenForGeometry(rect);
        if (newScreen && isTopLevel())
            d->setTopLevelScreen(newScreen, true);
        d->platformWindow->setGeometry(QHighDpi::toNativeWindowGeometry(rect, this));
    } else {
        d->geometry = rect;

        if (rect.x() != oldRect.x())
            emit xChanged(rect.x());
        if (rect.y() != oldRect.y())
            emit yChanged(rect.y());
        if (rect.width() != oldRect.width())
            emit widthChanged(rect.width());
        if (rect.height() != oldRect.height())
            emit heightChanged(rect.height());
    }
}

/*
  This is equivalent to QPlatformWindow::screenForGeometry, but in platform
  independent coordinates. The duplication is unfortunate, but there is a
  chicken and egg problem here: we cannot convert to native coordinates
  before we know which screen we are on.
*/
QScreen *QWindowPrivate::screenForGeometry(const QRect &newGeometry) const
{
    Q_Q(const QWindow);
    QScreen *currentScreen = q->screen();
    QScreen *fallback = currentScreen;
    QPoint center = newGeometry.center();
    if (!q->parent() && currentScreen && !currentScreen->geometry().contains(center)) {
        const auto screens = currentScreen->virtualSiblings();
        for (QScreen* screen : screens) {
            if (screen->geometry().contains(center))
                return screen;
            if (screen->geometry().intersects(newGeometry))
                fallback = screen;
        }
    }
    return fallback;
}


/*!
    Returns the geometry of the window, excluding its window frame.

    The geometry is in relation to the virtualGeometry() of its screen.

    \sa frameMargins(), frameGeometry()
*/
QRect QWindow::geometry() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        const auto nativeGeometry = d->platformWindow->geometry();
        return QHighDpi::fromNativeWindowGeometry(nativeGeometry, this);
    }
    return d->geometry;
}

/*!
    Returns the window frame margins surrounding the window.

    \sa geometry(), frameGeometry()
*/
QMargins QWindow::frameMargins() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return QHighDpi::fromNativePixels(d->platformWindow->frameMargins(), this);
    return QMargins();
}

/*!
    Returns the geometry of the window, including its window frame.

    The geometry is in relation to the virtualGeometry() of its screen.

    \sa geometry(), frameMargins()
*/
QRect QWindow::frameGeometry() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        QMargins m = frameMargins();
        return QHighDpi::fromNativeWindowGeometry(d->platformWindow->geometry(), this).adjusted(-m.left(), -m.top(), m.right(), m.bottom());
    }
    return d->geometry;
}

/*!
    Returns the top left position of the window, including its window frame.

    This returns the same value as frameGeometry().topLeft().

    \sa geometry(), frameGeometry()
*/
QPoint QWindow::framePosition() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        QMargins margins = frameMargins();
        return QHighDpi::fromNativeWindowGeometry(d->platformWindow->geometry().topLeft(), this) - QPoint(margins.left(), margins.top());
    }
    return d->geometry.topLeft();
}

/*!
    Sets the upper left position of the window (\a point) including its window frame.

    The position is in relation to the virtualGeometry() of its screen.

    \sa setGeometry(), frameGeometry()
*/
void QWindow::setFramePosition(const QPoint &point)
{
    Q_D(QWindow);
    d->positionPolicy = QWindowPrivate::WindowFrameInclusive;
    d->positionAutomatic = false;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(QHighDpi::toNativeWindowGeometry(QRect(point, size()), this));
    } else {
        d->geometry.moveTopLeft(point);
    }
}

/*!
    \brief set the position of the window on the desktop to \a pt

    The position is in relation to the virtualGeometry() of its screen.

    For interactively moving windows, see startSystemMove(). For interactively
    resizing windows, see startSystemResize().

    \note Not all windowing systems support setting or querying top level window positions.
    On such a system, programmatically moving windows may not have any effect, and artificial
    values may be returned for the current positions, such as \c QPoint(0, 0).

    \sa position(), startSystemMove()
*/
void QWindow::setPosition(const QPoint &pt)
{
    setGeometry(QRect(pt, size()));
}

/*!
    \brief set the position of the window on the desktop to \a posx, \a posy

    The position is in relation to the virtualGeometry() of its screen.

    \sa position()
*/
void QWindow::setPosition(int posx, int posy)
{
    setPosition(QPoint(posx, posy));
}

/*!
    \fn QPoint QWindow::position() const
    \brief Returns the position of the window on the desktop excluding any window frame

    \note Not all windowing systems support setting or querying top level window positions.
    On such a system, programmatically moving windows may not have any effect, and artificial
    values may be returned for the current positions, such as \c QPoint(0, 0).

    \sa setPosition()
*/

/*!
    \fn QSize QWindow::size() const
    \brief Returns the size of the window excluding any window frame

    \sa resize()
*/

/*!
    set the size of the window, excluding any window frame, to a QSize
    constructed from width \a w and height \a h

    For interactively resizing windows, see startSystemResize().

    \sa size(), geometry()
*/
void QWindow::resize(int w, int h)
{
    resize(QSize(w, h));
}

/*!
    \brief set the size of the window, excluding any window frame, to \a newSize

    \sa size(), geometry()
*/
void QWindow::resize(const QSize &newSize)
{
    Q_D(QWindow);
    d->positionPolicy = QWindowPrivate::WindowFrameExclusive;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(
            QHighDpi::toNativeWindowGeometry(QRect(position(), newSize), this));
    } else {
        const QSize oldSize = d->geometry.size();
        d->geometry.setSize(newSize);
        if (newSize.width() != oldSize.width())
            emit widthChanged(newSize.width());
        if (newSize.height() != oldSize.height())
            emit heightChanged(newSize.height());
    }
}

/*!
    Releases the native platform resources associated with this window.

    \sa create()
*/
void QWindow::destroy()
{
    Q_D(QWindow);
    if (!d->platformWindow)
        return;

    if (d->platformWindow->isForeignWindow())
        return;

    d->destroy();
}

void QWindowPrivate::destroy()
{
    if (!platformWindow)
        return;

    Q_Q(QWindow);
    QObjectList childrenWindows = q->children();
    for (int i = 0; i < childrenWindows.size(); i++) {
        QObject *object = childrenWindows.at(i);
        if (object->isWindowType()) {
            QWindow *w = static_cast<QWindow*>(object);
            qt_window_private(w)->destroy();
        }
    }

    bool wasVisible = q->isVisible();
    visibilityOnDestroy = wasVisible && platformWindow;

    q->setVisible(false);

    // Let subclasses act, typically by doing graphics resource cleaup, when
    // the window, to which graphics resource may be tied, is going away.
    //
    // NB! This is dysfunctional when destroy() is invoked from the dtor since
    // a reimplemented event() will not get called in the subclasses at that
    // stage. However, the typical QWindow cleanup involves either close() or
    // going through QWindowContainer, both of which will do an explicit, early
    // destroy(), which is good here.

    QPlatformSurfaceEvent e(QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed);
    QGuiApplication::sendEvent(q, &e);

    // Unset platformWindow before deleting, so that the destructor of the
    // platform window does not recurse back into the platform window via
    // this window during destruction (e.g. as a result of platform events).
    delete std::exchange(platformWindow, nullptr);

    if (QGuiApplicationPrivate::focus_window == q)
        QGuiApplicationPrivate::focus_window = q->parent();
    if (QGuiApplicationPrivate::currentMouseWindow == q)
        QGuiApplicationPrivate::currentMouseWindow = q->parent();
    if (QGuiApplicationPrivate::currentMousePressWindow == q)
        QGuiApplicationPrivate::currentMousePressWindow = q->parent();

    for (int i = 0; i < QGuiApplicationPrivate::tabletDevicePoints.size(); ++i)
        if (QGuiApplicationPrivate::tabletDevicePoints.at(i).target == q)
            QGuiApplicationPrivate::tabletDevicePoints[i].target = q->parent();

    resizeEventPending = true;
    receivedExpose = false;
    exposed = false;
}

/*!
    Returns the platform window corresponding to the window.

    \internal
*/
QPlatformWindow *QWindow::handle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}

/*!
    Returns the platform surface corresponding to the window.

    \internal
*/
QPlatformSurface *QWindow::surfaceHandle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}

/*!
    Sets whether keyboard grab should be enabled or not (\a grab).

    If the return value is true, the window receives all key events until
    setKeyboardGrabEnabled(false) is called; other windows get no key events at
    all. Mouse events are not affected. Use setMouseGrabEnabled() if you want
    to grab that.

    \sa setMouseGrabEnabled()
*/
bool QWindow::setKeyboardGrabEnabled(bool grab)
{
    Q_D(QWindow);
    if (d->platformWindow)
        return d->platformWindow->setKeyboardGrabEnabled(grab);
    return false;
}

/*!
    Sets whether mouse grab should be enabled or not (\a grab).

    If the return value is true, the window receives all mouse events until setMouseGrabEnabled(false) is
    called; other windows get no mouse events at all. Keyboard events are not affected.
    Use setKeyboardGrabEnabled() if you want to grab that.

    \sa setKeyboardGrabEnabled()
*/
bool QWindow::setMouseGrabEnabled(bool grab)
{
    Q_D(QWindow);
    if (d->platformWindow)
        return d->platformWindow->setMouseGrabEnabled(grab);
    return false;
}

/*!
    Returns the screen on which the window is shown, or null if there is none.

    For child windows, this returns the screen of the corresponding top level window.

    \sa setScreen(), QScreen::virtualSiblings()
*/
QScreen *QWindow::screen() const
{
    Q_D(const QWindow);
    return d->parentWindow ? d->parentWindow->screen() : d->topLevelScreen.data();
}

/*!
    Sets the screen on which the window should be shown.

    If the window has been created, it will be recreated on the \a newScreen.

    \note If the screen is part of a virtual desktop of multiple screens,
    the window will not move automatically to \a newScreen. To place the
    window relative to the screen, use the screen's topLeft() position.

    This function only works for top level windows.

    \sa screen(), QScreen::virtualSiblings()
*/
void QWindow::setScreen(QScreen *newScreen)
{
    Q_D(QWindow);
    if (!newScreen)
        newScreen = QGuiApplication::primaryScreen();
    d->setTopLevelScreen(newScreen, newScreen != nullptr);
}

/*!
    \fn QWindow::screenChanged(QScreen *screen)

    This signal is emitted when a window's \a screen changes, either
    by being set explicitly with setScreen(), or automatically when
    the window's screen is removed.
*/

/*!
  Returns the accessibility interface for the object that the window represents
  \internal
  \sa QAccessible
  */
QAccessibleInterface *QWindow::accessibleRoot() const
{
    return nullptr;
}

/*!
    \fn QWindow::focusObjectChanged(QObject *object)

    This signal is emitted when the final receiver of events tied to focus
    is changed to \a object.

    \sa focusObject()
*/

/*!
    Returns the QObject that will be the final receiver of events tied focus, such
    as key events.
*/
QObject *QWindow::focusObject() const
{
    return const_cast<QWindow *>(this);
}

/*!
    Shows the window.

    This is equivalent to calling showFullScreen(), showMaximized(), or showNormal(),
    depending on the platform's default behavior for the window type and flags.

    \sa showFullScreen(), showMaximized(), showNormal(), hide(), QStyleHints::showIsFullScreen(), flags()
*/
void QWindow::show()
{
    Qt::WindowState defaultState = QGuiApplicationPrivate::platformIntegration()->defaultWindowState(d_func()->windowFlags);
    if (defaultState == Qt::WindowFullScreen)
        showFullScreen();
    else if (defaultState == Qt::WindowMaximized)
        showMaximized();
    else
        showNormal();
}

/*!
    Hides the window.

    Equivalent to calling setVisible(false).

    \sa show(), setVisible()
*/
void QWindow::hide()
{
    setVisible(false);
}

/*!
    Shows the window as minimized.

    Equivalent to calling setWindowStates(Qt::WindowMinimized) and then
    setVisible(true).

    \sa setWindowStates(), setVisible()
*/
void QWindow::showMinimized()
{
    setWindowStates(Qt::WindowMinimized);
    setVisible(true);
}

/*!
    Shows the window as maximized.

    Equivalent to calling setWindowStates(Qt::WindowMaximized) and then
    setVisible(true).

    \sa setWindowStates(), setVisible()
*/
void QWindow::showMaximized()
{
    setWindowStates(Qt::WindowMaximized);
    setVisible(true);
}

/*!
    Shows the window as fullscreen.

    Equivalent to calling setWindowStates(Qt::WindowFullScreen) and then
    setVisible(true).

    See the \l{QWidget::showFullScreen()} documentation for platform-specific
    considerations and limitations.

    \sa setWindowStates(), setVisible()
*/
void QWindow::showFullScreen()
{
    setWindowStates(Qt::WindowFullScreen);
    setVisible(true);
#if !defined Q_OS_QNX // On QNX this window will be activated anyway from libscreen
                      // activating it here before libscreen activates it causes problems
    requestActivate();
#endif
}

/*!
    Shows the window as normal, i.e. neither maximized, minimized, nor fullscreen.

    Equivalent to calling setWindowStates(Qt::WindowNoState) and then
    setVisible(true).

    \sa setWindowStates(), setVisible()
*/
void QWindow::showNormal()
{
    setWindowStates(Qt::WindowNoState);
    setVisible(true);
}

/*!
    Close the window.

    This closes the window, effectively calling destroy(), and potentially
    quitting the application. Returns \c true on success, false if it has a parent
    window (in which case the top level window should be closed instead).

    \sa destroy(), QGuiApplication::quitOnLastWindowClosed(), closeEvent()
*/
bool QWindow::close()
{
    Q_D(QWindow);
    if (d->inClose)
        return true;

    // Do not close non top level windows
    if (!isTopLevel())
        return false;

    if (!d->platformWindow)
        return true;

    // The window might be deleted during close,
    // as a result of delivering the close event.
    QPointer guard(this);
    d->inClose = true;
    bool success = d->platformWindow->close();
    if (guard)
        d->inClose = false;

    return success;
}

bool QWindowPrivate::participatesInLastWindowClosed() const
{
    Q_Q(const QWindow);

    if (!q->isTopLevel())
        return false;

    // Tool-tip widgets do not normally have Qt::WA_QuitOnClose,
    // but since we do not have a similar flag for non-widget
    // windows we need an explicit exclusion here as well.
    if (q->type() == Qt::ToolTip)
        return false;

    // A window with a transient parent is not a primary window,
    // it's a secondary window.
    if (q->transientParent())
        return false;

    return true;
}

bool QWindowPrivate::treatAsVisible() const
{
    Q_Q(const QWindow);
    return q->isVisible();
}

/*!
    The expose event (\a ev) is sent by the window system when a window moves
    between the un-exposed and exposed states.

    An exposed window is potentially visible to the user. If the window is moved
    off screen, is made totally obscured by another window, is minimized, or
    similar, this function might be called and the value of isExposed() might
    change to false. You may use this event to limit expensive operations such
    as animations to only run when the window is exposed.

    This event should not be used to paint. To handle painting implement
    paintEvent() instead.

    A resize event will always be sent before the expose event the first time
    a window is shown.

    \sa paintEvent(), isExposed()
*/
void QWindow::exposeEvent(QExposeEvent *ev)
{
    ev->ignore();
}

/*!
    The paint event (\a ev) is sent by the window system whenever an area of
    the window needs a repaint, for example when initially showing the window,
    or due to parts of the window being uncovered by moving another window.

    The application is expected to render into the window in response to the
    paint event, regardless of the exposed state of the window. For example,
    a paint event may be sent before the window is exposed, to prepare it for
    showing to the user.

    \since 6.0

    \sa exposeEvent()
*/
void QWindow::paintEvent(QPaintEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle window move events (\a ev).
*/
void QWindow::moveEvent(QMoveEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle resize events (\a ev).

    The resize event is called whenever the window is resized in the windowing system,
    either directly through the windowing system acknowledging a setGeometry() or resize() request,
    or indirectly through the user resizing the window manually.
*/
void QWindow::resizeEvent(QResizeEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle show events (\a ev).

    The function is called when the window has requested becoming visible.

    If the window is successfully shown by the windowing system, this will
    be followed by a resize and an expose event.
*/
void QWindow::showEvent(QShowEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle hide events (\a ev).

    The function is called when the window has requested being hidden in the
    windowing system.
*/
void QWindow::hideEvent(QHideEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle close events (\a ev).

    The function is called when the window is requested to close. Call \l{QEvent::ignore()}
    on the event if you want to prevent the window from being closed.

    \sa close()
*/
void QWindow::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev);
}

/*!
    Override this to handle any event (\a ev) sent to the window.
    Return \c true if the event was recognized and processed.

    Remember to call the base class version if you wish for mouse events,
    key events, resize events, etc to be dispatched as usual.
*/
bool QWindow::event(QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        touchEvent(static_cast<QTouchEvent *>(ev));
        break;

    case QEvent::Move:
        moveEvent(static_cast<QMoveEvent*>(ev));
        break;

    case QEvent::Resize:
        resizeEvent(static_cast<QResizeEvent*>(ev));
        break;

    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent *>(ev));
        break;

    case QEvent::KeyRelease:
        keyReleaseEvent(static_cast<QKeyEvent *>(ev));
        break;

    case QEvent::FocusIn: {
        focusInEvent(static_cast<QFocusEvent *>(ev));
#if QT_CONFIG(accessibility)
        QAccessible::State state;
        state.active = true;
        QAccessibleStateChangeEvent event(this, state);
        QAccessible::updateAccessibility(&event);
#endif
        break; }

    case QEvent::FocusOut: {
        focusOutEvent(static_cast<QFocusEvent *>(ev));
#if QT_CONFIG(accessibility)
        QAccessible::State state;
        state.active = true;
        QAccessibleStateChangeEvent event(this, state);
        QAccessible::updateAccessibility(&event);
#endif
        break; }

#if QT_CONFIG(wheelevent)
    case QEvent::Wheel:
        wheelEvent(static_cast<QWheelEvent*>(ev));
        break;
#endif

    case QEvent::Close: {

        Q_D(QWindow);
        const bool wasVisible = d->treatAsVisible();
        const bool participatesInLastWindowClosed = d->participatesInLastWindowClosed();

        // The window might be deleted in the close event handler
        QPointer<QWindow> deletionGuard(this);
        closeEvent(static_cast<QCloseEvent*>(ev));

        if (ev->isAccepted()) {
            if (deletionGuard)
                destroy();
            if (wasVisible && participatesInLastWindowClosed)
                QGuiApplicationPrivate::instance()->maybeLastWindowClosed();
        }

        break;
    }

    case QEvent::Expose:
        exposeEvent(static_cast<QExposeEvent *>(ev));
        break;

    case QEvent::Paint:
        paintEvent(static_cast<QPaintEvent *>(ev));
        break;

    case QEvent::Show:
        showEvent(static_cast<QShowEvent *>(ev));
        break;

    case QEvent::Hide:
        hideEvent(static_cast<QHideEvent *>(ev));
        break;

    case QEvent::ApplicationWindowIconChange:
        setIcon(icon());
        break;

#if QT_CONFIG(tabletevent)
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
        tabletEvent(static_cast<QTabletEvent *>(ev));
        break;
#endif

    case QEvent::PlatformSurface: {
        if ((static_cast<QPlatformSurfaceEvent *>(ev))->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed) {
#ifndef QT_NO_OPENGL
            QOpenGLContext *context = QOpenGLContext::currentContext();
            if (context && context->surface() == static_cast<QSurface *>(this))
                context->doneCurrent();
#endif
        }
        break;
    }

    default:
        return QObject::event(ev);
    }

#ifndef QT_NO_CONTEXTMENU
    /*
        QGuiApplicationPrivate::processContextMenuEvent blocks mouse-triggered
        context menu events that the QPA plugin might generate. In practice that
        never happens, as even on Windows WM_CONTEXTMENU is never generated by
        the OS (we never call the default window procedure that would do that in
        response to unhandled WM_RBUTTONUP).

        So, we always have to syntheize QContextMenuEvent for mouse events anyway.
        QWidgetWindow synthesizes QContextMenuEvent similar to this code, and
        never calls QWindow::event, so we have to do it here as well.

        This logic could be simplified by always synthesizing events in
        QGuiApplicationPrivate, or perhaps even in each QPA plugin. See QTBUG-93486.
    */
    static const QEvent::Type contextMenuTrigger =
        QGuiApplicationPrivate::platformTheme()->themeHint(QPlatformTheme::ContextMenuOnMouseRelease).toBool() ?
        QEvent::MouseButtonRelease : QEvent::MouseButtonPress;
    auto asMouseEvent = [](QEvent *ev) {
        const auto t = ev->type();
        return t == QEvent::MouseButtonPress || t == QEvent::MouseButtonRelease
                ? static_cast<QMouseEvent *>(ev) : nullptr ;
    };
    if (QMouseEvent *me = asMouseEvent(ev); me &&
        ev->type() == contextMenuTrigger && me->button() == Qt::RightButton) {
        QContextMenuEvent e(QContextMenuEvent::Mouse, me->position().toPoint(),
                            me->globalPosition().toPoint(), me->modifiers());
        QGuiApplication::sendEvent(this, &e);
    }
#endif
    return true;
}

/*!
    Schedules a QEvent::UpdateRequest event to be delivered to this window.

    The event is delivered in sync with the display vsync on platforms where
    this is possible. Otherwise, the event is delivered after a delay of at
    most 5 ms. If the window's associated screen reports a
    \l{QScreen::refreshRate()}{refresh rate} higher than 60 Hz, the interval is
    scaled down to a value smaller than 5. The additional time is there to give
    the event loop a bit of idle time to gather system events, and can be
    overridden using the QT_QPA_UPDATE_IDLE_TIME environment variable.

    When driving animations, this function should be called once after drawing
    has completed. Calling this function multiple times will result in a single
    event being delivered to the window.

    Subclasses of QWindow should reimplement event(), intercept the event and
    call the application's rendering code, then call the base class
    implementation.

    \note The subclass' reimplementation of event() must invoke the base class
    implementation, unless it is absolutely sure that the event does not need to
    be handled by the base class. For example, the default implementation of
    this function relies on QEvent::Timer events. Filtering them away would
    therefore break the delivery of the update events.

    \since 5.5
*/
void QWindow::requestUpdate()
{
    Q_ASSERT_X(QThread::currentThread() == QCoreApplication::instance()->thread(),
        "QWindow", "Updates can only be scheduled from the GUI (main) thread");

    Q_D(QWindow);
    if (d->updateRequestPending || !d->platformWindow)
        return;
    d->updateRequestPending = true;
    d->platformWindow->requestUpdate();
}

/*!
    Override this to handle key press events (\a ev).

    \sa keyReleaseEvent()
*/
void QWindow::keyPressEvent(QKeyEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle key release events (\a ev).

    \sa keyPressEvent()
*/
void QWindow::keyReleaseEvent(QKeyEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle focus in events (\a ev).

    Focus in events are sent when the window receives keyboard focus.

    \sa focusOutEvent()
*/
void QWindow::focusInEvent(QFocusEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle focus out events (\a ev).

    Focus out events are sent when the window loses keyboard focus.

    \sa focusInEvent()
*/
void QWindow::focusOutEvent(QFocusEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse press events (\a ev).

    \sa mouseReleaseEvent()
*/
void QWindow::mousePressEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse release events (\a ev).

    \sa mousePressEvent()
*/
void QWindow::mouseReleaseEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse double click events (\a ev).

    \sa mousePressEvent(), QStyleHints::mouseDoubleClickInterval()
*/
void QWindow::mouseDoubleClickEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse move events (\a ev).
*/
void QWindow::mouseMoveEvent(QMouseEvent *ev)
{
    ev->ignore();
}

#if QT_CONFIG(wheelevent)
/*!
    Override this to handle mouse wheel or other wheel events (\a ev).
*/
void QWindow::wheelEvent(QWheelEvent *ev)
{
    ev->ignore();
}
#endif // QT_CONFIG(wheelevent)

/*!
    Override this to handle touch events (\a ev).
*/
void QWindow::touchEvent(QTouchEvent *ev)
{
    ev->ignore();
}

#if QT_CONFIG(tabletevent)
/*!
    Override this to handle tablet press, move, and release events (\a ev).

    Proximity enter and leave events are not sent to windows, they are
    delivered to the application instance.
*/
void QWindow::tabletEvent(QTabletEvent *ev)
{
    ev->ignore();
}
#endif

/*!
    Override this to handle platform dependent events.
    Will be given \a eventType, \a message and \a result.

    This might make your application non-portable.

    Should return true only if the event was handled.
*/

bool QWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}

/*!
    \fn QPointF QWindow::mapToGlobal(const QPointF &pos) const

    Translates the window coordinate \a pos to global screen
    coordinates. For example, \c{mapToGlobal(QPointF(0,0))} would give
    the global coordinates of the top-left pixel of the window.

    \sa mapFromGlobal()
    \since 6.0
*/
QPointF QWindow::mapToGlobal(const QPointF &pos) const
{
    Q_D(const QWindow);
    // QTBUG-43252, prefer platform implementation for foreign windows.
    if (d->platformWindow
        && (d->platformWindow->isForeignWindow() || d->platformWindow->isEmbedded())) {
        return QHighDpi::fromNativeGlobalPosition(d->platformWindow->mapToGlobalF(QHighDpi::toNativeLocalPosition(pos, this)), this);
    }

    if (!QHighDpiScaling::isActive())
        return pos + d->globalPosition();

    // The normal pos + windowGlobalPos calculation may give a point which is outside
    // screen geometry for windows which span multiple screens, due to the way QHighDpiScaling
    // creates gaps between screens in the the device indendent cooordinate system.
    //
    // Map the position (and the window's global position) to native coordinates, perform
    // the addition, and then map back to device independent coordinates.
    QPointF nativeLocalPos = QHighDpi::toNativeLocalPosition(pos, this);
    QPointF nativeWindowGlobalPos = QHighDpi::toNativeGlobalPosition(QPointF(d->globalPosition()), this);
    QPointF nativeGlobalPos = nativeLocalPos + nativeWindowGlobalPos;
    QPointF deviceIndependentGlobalPos = QHighDpi::fromNativeGlobalPosition(nativeGlobalPos, this);
    return deviceIndependentGlobalPos;
}

/*!
    \overload
*/
QPoint QWindow::mapToGlobal(const QPoint &pos) const
{
    return mapToGlobal(QPointF(pos)).toPoint();
}

/*!
    \fn QPointF QWindow::mapFromGlobal(const QPointF &pos) const

    Translates the global screen coordinate \a pos to window
    coordinates.

    \sa mapToGlobal()
    \since 6.0
*/
QPointF QWindow::mapFromGlobal(const QPointF &pos) const
{
    Q_D(const QWindow);
    // QTBUG-43252, prefer platform implementation for foreign windows.
    if (d->platformWindow
        && (d->platformWindow->isForeignWindow() || d->platformWindow->isEmbedded())) {
        return QHighDpi::fromNativeLocalPosition(d->platformWindow->mapFromGlobalF(QHighDpi::toNativeGlobalPosition(pos, this)), this);
    }

    if (!QHighDpiScaling::isActive())
        return pos - d->globalPosition();

    // Calculate local position in the native coordinate system. (See comment for the
    // corresponding mapToGlobal() code above).
    QPointF nativeGlobalPos = QHighDpi::toNativeGlobalPosition(pos, this);
    QPointF nativeWindowGlobalPos = QHighDpi::toNativeGlobalPosition(QPointF(d->globalPosition()), this);
    QPointF nativeLocalPos = nativeGlobalPos - nativeWindowGlobalPos;
    QPointF deviceIndependentLocalPos = QHighDpi::fromNativeLocalPosition(nativeLocalPos, this);
    return deviceIndependentLocalPos;
}

/*!
    \overload
*/
QPoint QWindow::mapFromGlobal(const QPoint &pos) const
{
    return QWindow::mapFromGlobal(QPointF(pos)).toPoint();
}

QPoint QWindowPrivate::globalPosition() const
{
    Q_Q(const QWindow);
    QPoint offset = q->position();
    for (const QWindow *p = q->parent(); p; p = p->parent()) {
        QPlatformWindow *pw = p->handle();
        if (pw && (pw->isForeignWindow() || pw->isEmbedded())) {
            // Use mapToGlobal() for foreign windows
            offset += p->mapToGlobal(QPoint(0, 0));
            break;
        } else {
            offset += p->position();
        }
    }
    return offset;
}

Q_GUI_EXPORT QWindowPrivate *qt_window_private(QWindow *window)
{
    return window->d_func();
}

QWindow *QWindowPrivate::topLevelWindow(QWindow::AncestorMode mode) const
{
    Q_Q(const QWindow);

    QWindow *window = const_cast<QWindow *>(q);

    while (window) {
        QWindow *parent = window->parent(mode);
        if (!parent)
            break;

        window = parent;
    }

    return window;
}

/*!
    Creates a local representation of a window created by another process or by
    using native libraries below Qt.

    Given the handle \a id to a native window, this method creates a QWindow
    object which can be used to represent the window when invoking methods like
    setParent() and setTransientParent().

    This can be used, on platforms which support it, to embed a QWindow inside a
    native window, or to embed a native window inside a QWindow.

    If foreign windows are not supported or embedding the native window
    failed in the platform plugin, this function returns \nullptr.

    \note The resulting QWindow should not be used to manipulate the underlying
    native window (besides re-parenting), or to observe state changes of the
    native window. Any support for these kind of operations is incidental, highly
    platform dependent and untested.

    \sa setParent()
*/
QWindow *QWindow::fromWinId(WId id)
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ForeignWindows)) {
        qWarning("QWindow::fromWinId(): platform plugin does not support foreign windows.");
        return nullptr;
    }

    QWindow *window = new QWindow;
    qt_window_private(window)->create(false, id);

    if (!window->handle()) {
        delete window;
        return nullptr;
    }

    return window;
}

/*!
    Causes an alert to be shown for \a msec milliseconds. If \a msec is \c 0 (the
    default), then the alert is shown indefinitely until the window becomes
    active again. This function has no effect on an active window.

    In alert state, the window indicates that it demands attention, for example by
    flashing or bouncing the taskbar entry.

    \since 5.1
*/

void QWindow::alert(int msec)
{
    Q_D(QWindow);
    if (!d->platformWindow || d->platformWindow->isAlertState() || isActive())
        return;
    d->platformWindow->setAlertState(true);
    if (d->platformWindow->isAlertState() && msec)
        QTimer::singleShot(msec, this, SLOT(_q_clearAlert()));
}

void QWindowPrivate::_q_clearAlert()
{
    if (platformWindow && platformWindow->isAlertState())
        platformWindow->setAlertState(false);
}

#ifndef QT_NO_CURSOR
/*!
    \brief set the cursor shape for this window

    The mouse \a cursor will assume this shape when it is over this
    window, unless an override cursor is set.
    See the \l{Qt::CursorShape}{list of predefined cursor objects} for a
    range of useful shapes.

    If no cursor has been set, or after a call to unsetCursor(), the
    parent window's cursor is used.

    By default, the cursor has the Qt::ArrowCursor shape.

    Some underlying window implementations will reset the cursor if it
    leaves a window even if the mouse is grabbed. If you want to have
    a cursor set for all windows, even when outside the window, consider
    QGuiApplication::setOverrideCursor().

    \sa QGuiApplication::setOverrideCursor()
*/
void QWindow::setCursor(const QCursor &cursor)
{
    Q_D(QWindow);
    d->setCursor(&cursor);
}

/*!
  \brief Restores the default arrow cursor for this window.
 */
void QWindow::unsetCursor()
{
    Q_D(QWindow);
    d->setCursor(nullptr);
}

/*!
    \brief the cursor shape for this window

    \sa setCursor(), unsetCursor()
*/
QCursor QWindow::cursor() const
{
    Q_D(const QWindow);
    return d->cursor;
}

void QWindowPrivate::setCursor(const QCursor *newCursor)
{

    Q_Q(QWindow);
    if (newCursor) {
        const Qt::CursorShape newShape = newCursor->shape();
        if (newShape <= Qt::LastCursor && hasCursor && newShape == cursor.shape())
            return; // Unchanged and no bitmap/custom cursor.
        cursor = *newCursor;
        hasCursor = true;
    } else {
        if (!hasCursor)
            return;
        cursor = QCursor(Qt::ArrowCursor);
        hasCursor = false;
    }
    // Only attempt to emit signal if there is an actual platform cursor
    if (applyCursor()) {
        QEvent event(QEvent::CursorChange);
        QGuiApplication::sendEvent(q, &event);
    }
}

// Apply the cursor and returns true iff the platform cursor exists
bool QWindowPrivate::applyCursor()
{
    Q_Q(QWindow);
    if (QScreen *screen = q->screen()) {
        if (QPlatformCursor *platformCursor = screen->handle()->cursor()) {
            if (!platformWindow)
                return true;
            QCursor *c = QGuiApplication::overrideCursor();
            if (c != nullptr && platformCursor->capabilities().testFlag(QPlatformCursor::OverrideCursor))
                return true;
            if (!c && hasCursor)
                c = &cursor;
            platformCursor->changeCursor(c, q);
            return true;
        }
    }
    return false;
}
#endif // QT_NO_CURSOR

void *QWindow::resolveInterface(const char *name, int revision) const
{
    using namespace QNativeInterface::Private;

    auto *platformWindow = handle();
    Q_UNUSED(platformWindow);
    Q_UNUSED(name);
    Q_UNUSED(revision);

#if defined(Q_OS_WIN)
    QT_NATIVE_INTERFACE_RETURN_IF(QWindowsWindow, platformWindow);
#endif

#if QT_CONFIG(xcb)
    QT_NATIVE_INTERFACE_RETURN_IF(QXcbWindow, platformWindow);
#endif

#if defined(Q_OS_MACOS)
    QT_NATIVE_INTERFACE_RETURN_IF(QCocoaWindow, platformWindow);
#endif

#if defined(Q_OS_UNIX)
    QT_NATIVE_INTERFACE_RETURN_IF(QWaylandWindow, platformWindow);
#endif

#if defined(Q_OS_WASM)
    QT_NATIVE_INTERFACE_RETURN_IF(QWasmWindow, platformWindow);
#endif

    return nullptr;
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QWindow *window)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    if (window) {
        debug << window->metaObject()->className() << '(' << (const void *)window;
        if (!window->objectName().isEmpty())
            debug << ", name=" << window->objectName();
        if (debug.verbosity() > 2) {
            const QRect geometry = window->geometry();
            if (window->isVisible())
                debug << ", visible";
            if (window->isExposed())
                debug << ", exposed";
            debug << ", state=" << window->windowState()
                << ", type=" << window->type() << ", flags=" << window->flags()
                << ", surface type=" << window->surfaceType();
            if (window->isTopLevel())
                debug << ", toplevel";
            debug << ", " << geometry.width() << 'x' << geometry.height()
                << Qt::forcesign << geometry.x() << geometry.y() << Qt::noforcesign;
            const QMargins margins = window->frameMargins();
            if (!margins.isNull())
                debug << ", margins=" << margins;
            debug << ", devicePixelRatio=" << window->devicePixelRatio();
            if (const QPlatformWindow *platformWindow = window->handle())
                debug << ", winId=0x" << Qt::hex << platformWindow->winId() << Qt::dec;
            if (const QScreen *screen = window->screen())
                debug << ", on " << screen->name();
        }
        debug << ')';
    } else {
        debug << "QWindow(0x0)";
    }
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

#if QT_CONFIG(vulkan) || defined(Q_QDOC)

/*!
    Associates this window with the specified Vulkan \a instance.

    \a instance must stay valid as long as this QWindow instance exists.
 */
void QWindow::setVulkanInstance(QVulkanInstance *instance)
{
    Q_D(QWindow);
    d->vulkanInstance = instance;
}

/*!
    \return the associated Vulkan instance if any was set, otherwise \nullptr.
 */
QVulkanInstance *QWindow::vulkanInstance() const
{
    Q_D(const QWindow);
    return d->vulkanInstance;
}

#endif // QT_CONFIG(vulkan)

QT_END_NAMESPACE

#include "moc_qwindow.cpp"
