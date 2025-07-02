// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qioswindow.h"

#include "qiosapplicationdelegate.h"
#include "qiosglobal.h"
#include "qiosintegration.h"
#include "qiosscreen.h"
#include "qiosviewcontroller.h"
#include "quiview.h"
#include "qiosinputcontext.h"

#include <QtCore/private/qcore_mac_p.h>

#include <QtGui/private/qwindow_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <qpa/qplatformintegration.h>

#if QT_CONFIG(opengl)
#import <QuartzCore/CAEAGLLayer.h>
#endif

#if QT_CONFIG(metal)
#import <QuartzCore/CAMetalLayer.h>
#endif

#include <QtDebug>

QT_BEGIN_NAMESPACE

enum {
    defaultWindowWidth = 160,
    defaultWindowHeight = 160
};

QIOSWindow::QIOSWindow(QWindow *window, WId nativeHandle)
    : QPlatformWindow(window)
{
    if (nativeHandle) {
        m_view = reinterpret_cast<UIView *>(nativeHandle);
        [m_view retain];
    } else {
#if QT_CONFIG(metal)
        if (window->surfaceType() == QSurface::RasterSurface)
            window->setSurfaceType(QSurface::MetalSurface);

        if (window->surfaceType() == QSurface::MetalSurface)
            m_view = [[QUIMetalView alloc] initWithQIOSWindow:this];
        else
#endif
            m_view = [[QUIView alloc] initWithQIOSWindow:this];
    }

    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &QIOSWindow::applicationStateChanged);

    // Always set parent, even if we don't have a parent window,
    // as we use setParent to reparent top levels into our desktop
    // manager view.
    setParent(QPlatformWindow::parent());

    if (!isForeignWindow()) {
        // Resolve default window geometry in case it was not set before creating the
        // platform window. This picks up eg. minimum-size if set.
        m_normalGeometry = initialGeometry(window, QPlatformWindow::geometry(),
            defaultWindowWidth, defaultWindowHeight);

        setWindowState(window->windowStates());
        setOpacity(window->opacity());
        setMask(QHighDpi::toNativeLocalRegion(window->mask(), window));
    } else {
        // Pick up essential foreign window state
        QPlatformWindow::setGeometry(QRectF::fromCGRect(m_view.frame).toRect());
    }

    Qt::ScreenOrientation initialOrientation = window->contentOrientation();
    if (initialOrientation != Qt::PrimaryOrientation) {
        // Start up in portrait, then apply possible content orientation,
        // as per Apple's documentation.
        dispatch_async(dispatch_get_main_queue(), ^{
            handleContentOrientationChange(initialOrientation);
        });
    }
}

QIOSWindow::~QIOSWindow()
{
    // According to the UIResponder documentation, Cocoa Touch should react to system interruptions
    // that "might cause the view to be removed from the window" by sending touchesCancelled, but in
    // practice this doesn't seem to happen when removing the view from its superview. To ensure that
    // Qt's internal state for touch and mouse handling is kept consistent, we therefore have to force
    // cancellation of all touch events.
    [m_view touchesCancelled:[NSSet set] withEvent:0];

    clearAccessibleCache();

    quiview_cast(m_view).platformWindow = nullptr;

    // Remove from superview, unless we're a foreign window without a
    // Qt window parent, in which case the foreign window is used as
    // a window container for a Qt UI hierarchy inside a native UI.
    if (!(isForeignWindow() && !QPlatformWindow::parent()))
        [m_view removeFromSuperview];

    [m_view release];
}


QSurfaceFormat QIOSWindow::format() const
{
    return window()->requestedFormat();
}


bool QIOSWindow::blockedByModal()
{
    QWindow *modalWindow = QGuiApplication::modalWindow();
    return modalWindow && modalWindow != window();
}

void QIOSWindow::setVisible(bool visible)
{
    m_view.hidden = !visible;
    [m_view setNeedsDisplay];

    if (!isQtApplication() || !window()->isTopLevel())
        return;

    if (blockedByModal()) {
        if (visible)
            raise();
        return;
    }

    if (visible && shouldAutoActivateWindow()) {
        if (!window()->property("_q_showWithoutActivating").toBool())
            requestActivateWindow();
    } else if (!visible && [quiview_cast(m_view) isActiveWindow]) {
        // Our window was active/focus window but now hidden, so relinquish
        // focus to the next possible window in the stack.
        NSArray<UIView *> *subviews = m_view.viewController.view.subviews;
        for (int i = int(subviews.count) - 1; i >= 0; --i) {
            UIView *view = [subviews objectAtIndex:i];
            if (view.hidden)
                continue;

            QWindow *w = view.qwindow;
            if (!w || !w->isTopLevel())
                continue;

            QIOSWindow *iosWindow = static_cast<QIOSWindow *>(w->handle());
            if (!iosWindow->shouldAutoActivateWindow())
                continue;

            iosWindow->requestActivateWindow();
            break;
        }
    }
}

bool QIOSWindow::shouldAutoActivateWindow() const
{
    if (![m_view canBecomeFirstResponder])
        return false;

    // We don't want to do automatic window activation for popup windows
    // that are unlikely to contain editable controls (to avoid hiding
    // the keyboard while the popup is showing)
    const Qt::WindowType type = window()->type();
    return (type != Qt::Popup && type != Qt::ToolTip) || !window()->isActive();
}

void QIOSWindow::setOpacity(qreal level)
{
    m_view.alpha = qBound(0.0, level, 1.0);
}

void QIOSWindow::setGeometry(const QRect &rect)
{
    m_normalGeometry = rect;

    if (window()->windowState() != Qt::WindowNoState) {
        QPlatformWindow::setGeometry(rect);

        // The layout will realize the requested geometry was not applied, and
        // send geometry-change events that match the actual geometry.
        [m_view setNeedsLayout];

        if (window()->inherits("QWidgetWindow")) {
            // QWidget wrongly assumes that setGeometry resets the window
            // state back to Qt::NoWindowState, so we need to inform it that
            // that his is not the case by re-issuing the current window state.
            QWindowSystemInterface::handleWindowStateChanged(window(), window()->windowState());

            // It also needs to be told immediately that the geometry it requested
            // did not apply, otherwise it will continue on as if it did, instead
            // of waiting for a resize event.
            [m_view layoutIfNeeded];
        }

        return;
    }

    applyGeometry(rect);
}

void QIOSWindow::applyGeometry(const QRect &rect)
{
    // Geometry changes are asynchronous, but QWindow::geometry() is
    // expected to report back the 'requested geometry' until we get
    // a callback with the updated geometry from the window system.
    // The baseclass takes care of persisting this for us.
    QPlatformWindow::setGeometry(rect);

    m_view.frame = rect.toCGRect();

    // iOS will automatically trigger -[layoutSubviews:] for resize,
    // but not for move, so we force it just in case.
    [m_view setNeedsLayout];

    if (window()->inherits("QWidgetWindow"))
        [m_view layoutIfNeeded];
}

QMargins QIOSWindow::safeAreaMargins() const
{
    UIEdgeInsets safeAreaInsets = m_view.safeAreaInsets;
    return QMargins(safeAreaInsets.left, safeAreaInsets.top,
        safeAreaInsets.right, safeAreaInsets.bottom);
}

bool QIOSWindow::isExposed() const
{
    return qApp->applicationState() != Qt::ApplicationSuspended
        && window()->isVisible() && !window()->geometry().isEmpty();
}

void QIOSWindow::setWindowState(Qt::WindowStates state)
{
    // Update the QWindow representation straight away, so that
    // we can update the statusbar visibility based on the new
    // state before applying geometry changes.
    qt_window_private(window())->windowState = state;

    if (window()->isTopLevel() && window()->isVisible() && window()->isActive())
        [m_view.qtViewController updateStatusBarProperties];

    if (state & Qt::WindowMinimized) {
        applyGeometry(QRect());
    } else if (state & (Qt::WindowFullScreen | Qt::WindowMaximized)) {
        QRect uiWindowBounds = QRectF::fromCGRect(m_view.window.bounds).toRect();
        if (NSProcessInfo.processInfo.iOSAppOnMac) {
            // iOS apps running as "Designed for iPad" on macOS do not match
            // our current window management implementation where a single
            // UIWindow is tied to a single screen. And even if we're on the
            // right screen, the UIScreen does not account for the 77% scale
            // of the UIUserInterfaceIdiomPad environment, so we can't use
            // it to clamp the window geometry. Instead just use the UIWindow
            // directly, which represents our "screen".
            applyGeometry(uiWindowBounds);
        } else if (isRunningOnVisionOS()) {
            // On visionOS there is no concept of a screen, and hence no concept of
            // screen-relative system UI that we should keep top level windows away
            // from, so don't apply the UIWindow safe area insets to the screen.
            applyGeometry(uiWindowBounds);
        } else {
            QRect fullscreenGeometry = screen()->geometry();
            QRect maximizedGeometry = fullscreenGeometry;

#if !defined(Q_OS_VISIONOS)
            if (!(window()->flags() & Qt::ExpandedClientAreaHint)) {
                // If the safe area margins reflect the screen's outer edges,
                // then reduce the maximized geometry accordingly. Otherwise
                // leave it as is, and assume the client will take the safe
                // are margins into account explicitly.
                UIScreen *uiScreen = m_view.window.windowScene.screen;
                UIEdgeInsets safeAreaInsets = m_view.window.safeAreaInsets;
                if (m_view.window.bounds.size.width == uiScreen.bounds.size.width)
                    maximizedGeometry.adjust(safeAreaInsets.left, 0, -safeAreaInsets.right, 0);
                if (m_view.window.bounds.size.height == uiScreen.bounds.size.height)
                    maximizedGeometry.adjust(0, safeAreaInsets.top, 0, -safeAreaInsets.bottom);
            }
#endif

            if (m_view.window) {
                // On application startup, during main(), we don't have a UIWindow yet (because
                // the UIWindowScene has not been connected yet), but once the scene has been
                // connected and we have a UIWindow we can adjust the maximized/fullscreen size
                // to account for split-view or floating window mode, where the UIWindow is
                // smaller than the screen.
                fullscreenGeometry = fullscreenGeometry.intersected(uiWindowBounds);
                maximizedGeometry = maximizedGeometry.intersected(uiWindowBounds);
            }

            if (state & Qt::WindowFullScreen)
                applyGeometry(fullscreenGeometry);
            else
                applyGeometry(maximizedGeometry);
        }
    } else {
        applyGeometry(m_normalGeometry);
    }
}

void QIOSWindow::setParent(const QPlatformWindow *parentWindow)
{
    UIView *superview = nullptr;
    if (parentWindow)
        superview = reinterpret_cast<UIView *>(parentWindow->winId());
    else if (isQtApplication() && !isForeignWindow())
        superview = rootViewForScreen(window()->screen()->handle());

    if (superview)
        [superview addSubview:m_view];
    else if (quiview_cast(m_view.superview))
        [m_view removeFromSuperview];
}

void QIOSWindow::requestActivateWindow()
{
    // Note that several windows can be active at the same time if they exist in the same
    // hierarchy (transient children). But only one window can be QGuiApplication::focusWindow().
    // Despite the name, 'requestActivateWindow' means raise and transfer focus to the window:
    if (blockedByModal())
        return;

    [m_view.window makeKeyWindow];
    [m_view becomeFirstResponder];

    if (window()->isTopLevel())
        raise();
}

void QIOSWindow::raiseOrLower(bool raise)
{
    if (!isQtApplication())
        return;

    NSArray<UIView *> *subviews = m_view.superview.subviews;
    if (subviews.count == 1)
        return;

    if (m_view.superview == m_view.qtViewController.view) {
        // We're a top level window, so we need to take window
        // levels into account.
        for (int i = int(subviews.count) - 1; i >= 0; --i) {
            UIView *view = static_cast<UIView *>([subviews objectAtIndex:i]);
            if (view.hidden || view == m_view || !view.qwindow)
                continue;
            int level = static_cast<QIOSWindow *>(view.qwindow->handle())->windowLevel();
            if (windowLevel() > level || (raise && windowLevel() == level)) {
                [m_view.superview insertSubview:m_view aboveSubview:view];
                return;
            }
        }
        [m_view.superview insertSubview:m_view atIndex:0];
    } else {
        // Child window, or embedded into a non-Qt view controller
        if (raise)
            [m_view.superview bringSubviewToFront:m_view];
        else
            [m_view.superview sendSubviewToBack:m_view];
    }
}

int QIOSWindow::windowLevel() const
{
    Qt::WindowType type = window()->type();

    int level = 0;

    if (type == Qt::ToolTip)
        level = 120;
    else if (window()->flags() & Qt::WindowStaysOnTopHint)
        level = 100;
    else if (window()->isModal())
        level = 40;
    else if (type == Qt::Popup)
        level = 30;
    else if (type == Qt::SplashScreen)
        level = 20;
    else if (type == Qt::Tool)
        level = 10;
    else
        level = 0;

    // A window should be in at least the same window level as its parent
    QWindow *transientParent = window()->transientParent();
    QIOSWindow *transientParentWindow = transientParent ? static_cast<QIOSWindow *>(transientParent->handle()) : 0;
    if (transientParentWindow)
        level = qMax(transientParentWindow->windowLevel(), level);

    return level;
}

void QIOSWindow::applicationStateChanged(Qt::ApplicationState)
{
    if (isForeignWindow())
        return;

    if (window()->isExposed() != isExposed())
        [quiview_cast(m_view) sendUpdatedExposeEvent];
}

qreal QIOSWindow::devicePixelRatio() const
{
#if !defined(Q_OS_VISIONOS)
    // If the view has not yet been added to a screen, it will not
    // pick up its device pixel ratio, so we need to do so manually
    // based on the screen we think the window will be added to.
    if (!m_view.window.windowScene.screen)
        return screen()->devicePixelRatio();
#endif

    // Otherwise we can rely on the content scale factor
    return m_view.contentScaleFactor;
}

void QIOSWindow::clearAccessibleCache()
{
    if (isForeignWindow())
        return;

    [quiview_cast(m_view) clearAccessibleCache];
}

void QIOSWindow::requestUpdate()
{
    static_cast<QIOSScreen *>(screen())->setUpdatesPaused(false);
}

void QIOSWindow::setMask(const QRegion &region)
{
    if (!region.isEmpty()) {
        QCFType<CGMutablePathRef> maskPath = CGPathCreateMutable();
        for (const QRect &r : region)
            CGPathAddRect(maskPath, nullptr, r.toCGRect());
        CAShapeLayer *maskLayer = [CAShapeLayer layer];
        maskLayer.path = maskPath;
        m_view.layer.mask = maskLayer;
    } else {
        m_view.layer.mask = nil;
    }
}

#if QT_CONFIG(opengl)
CAEAGLLayer *QIOSWindow::eaglLayer() const
{
    Q_ASSERT([m_view.layer isKindOfClass:[CAEAGLLayer class]]);
    return static_cast<CAEAGLLayer *>(m_view.layer);
}
#endif

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QIOSWindow *window)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QIOSWindow(" << (const void *)window;
    if (window)
        debug << ", window=" << window->window();
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

/*!
    Returns the view cast to a QUIview if possible.

    If the view is not a QUIview, nil is returned, which is safe to
    send messages to, effectively making [quiview_cast(view) message]
    a no-op.

    For extra verbosity and clearer code, please consider checking
    that the platform window is not a foreign window before using
    this cast, via QPlatformWindow::isForeignWindow().

    Do not use this method solely to check for foreign windows, as
    that will make the code harder to read for people not working
    primarily on iOS, who do not know the difference between the
    UIView and QUIView cases.
*/
QUIView *quiview_cast(UIView *view)
{
    return qt_objc_cast<QUIView *>(view);
}

bool QIOSWindow::isForeignWindow() const
{
    return ![m_view isKindOfClass:QUIView.class];
}

UIView *QIOSWindow::view() const
{
    return m_view;
}

QT_END_NAMESPACE

#include "moc_qioswindow.cpp"
