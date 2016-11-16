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
#include "qcocoawindow.h"
#include "qcocoaintegration.h"
#include "qnswindowdelegate.h"
#include "qcocoaeventdispatcher.h"
#ifndef QT_NO_OPENGL
#include "qcocoaglcontext.h"
#endif
#include "qcocoahelpers.h"
#include "qcocoanativeinterface.h"
#include "qnsview.h"
#include <QtCore/qfileinfo.h>
#include <QtCore/private/qcore_mac_p.h>
#include <qwindow.h>
#include <private/qwindow_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformscreen.h>
#include <QtGui/private/qcoregraphics_p.h>

#include <AppKit/AppKit.h>

#include <QDebug>

enum {
    defaultWindowWidth = 160,
    defaultWindowHeight = 160
};

static bool isMouseEvent(NSEvent *ev)
{
    switch ([ev type]) {
    case NSLeftMouseDown:
    case NSLeftMouseUp:
    case NSRightMouseDown:
    case NSRightMouseUp:
    case NSMouseMoved:
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
        return true;
    default:
        return false;
    }
}

static void qt_closePopups()
{
    while (QCocoaWindow *popup = QCocoaIntegration::instance()->popPopupWindow()) {
        QWindowSystemInterface::handleCloseEvent(popup->window());
        QWindowSystemInterface::flushWindowSystemEvents();
    }
}

@implementation QNSWindowHelper

@synthesize window = _window;
@synthesize grabbingMouse = _grabbingMouse;
@synthesize releaseOnMouseUp = _releaseOnMouseUp;

- (QCocoaWindow *)platformWindow
{
    return _platformWindow.data();
}

- (id)initWithNSWindow:(QCocoaNSWindow *)window platformWindow:(QCocoaWindow *)platformWindow
{
    self = [super init];
    if (self) {
        _window = window;
        _platformWindow = platformWindow;

        _window.delegate = [[QNSWindowDelegate alloc] initWithQCocoaWindow:_platformWindow];

        // Prevent Cocoa from releasing the window on close. Qt
        // handles the close event asynchronously and we want to
        // make sure that m_nsWindow stays valid until the
        // QCocoaWindow is deleted by Qt.
        [_window setReleasedWhenClosed:NO];
    }

    return self;
}

- (void)handleWindowEvent:(NSEvent *)theEvent
{
    QCocoaWindow *pw = self.platformWindow;
    if (pw && pw->m_forwardWindow) {
        if (theEvent.type == NSLeftMouseUp || theEvent.type == NSLeftMouseDragged) {
            QNSView *forwardView = qnsview_cast(pw->view());
            if (theEvent.type == NSLeftMouseUp) {
                [forwardView mouseUp:theEvent];
                pw->m_forwardWindow.clear();
            } else {
                [forwardView mouseDragged:theEvent];
            }
        }

        if (!pw->m_isNSWindowChild && theEvent.type == NSLeftMouseDown) {
            pw->m_forwardWindow.clear();
        }
    }

    if (theEvent.type == NSLeftMouseDown) {
        self.grabbingMouse = YES;
    } else if (theEvent.type == NSLeftMouseUp) {
        self.grabbingMouse = NO;
        if (self.releaseOnMouseUp) {
            [self detachFromPlatformWindow];
            [self.window release];
            return;
        }
    }

    // The call to -[NSWindow sendEvent] may result in the window being deleted
    // (e.g., when closing the window by pressing the title bar close button).
    [self retain];
    [self.window superSendEvent:theEvent];
    bool windowStillAlive = self.window != nil; // We need to read before releasing
    [self release];
    if (!windowStillAlive)
        return;

    if (!self.window.delegate)
        return; // Already detached, pending NSAppKitDefined event

    if (pw && pw->frameStrutEventsEnabled() && isMouseEvent(theEvent)) {
        NSPoint loc = [theEvent locationInWindow];
        NSRect windowFrame = [self.window convertRectFromScreen:[self.window frame]];
        NSRect contentFrame = [[self.window contentView] frame];
        if (NSMouseInRect(loc, windowFrame, NO) && !NSMouseInRect(loc, contentFrame, NO))
            [qnsview_cast(pw->view()) handleFrameStrutMouseEvent:theEvent];
    }
}

- (void)detachFromPlatformWindow
{
    _platformWindow.clear();
    [self.window.delegate release];
    self.window.delegate = nil;
}

- (void)clearWindow
{
    if (_window) {
        QCocoaEventDispatcher *cocoaEventDispatcher = qobject_cast<QCocoaEventDispatcher *>(QGuiApplication::instance()->eventDispatcher());
        if (cocoaEventDispatcher) {
            QCocoaEventDispatcherPrivate *cocoaEventDispatcherPrivate = static_cast<QCocoaEventDispatcherPrivate *>(QObjectPrivate::get(cocoaEventDispatcher));
            cocoaEventDispatcherPrivate->removeQueuedUserInputEvents([_window windowNumber]);
        }

        _window = nil;
    }
}

- (void)dealloc
{
    _window = nil;
    _platformWindow.clear();
    [super dealloc];
}

@end

@implementation QNSWindow

@synthesize helper = _helper;

- (id)initWithContentRect:(NSRect)contentRect
      styleMask:(NSUInteger)windowStyle
      qPlatformWindow:(QCocoaWindow *)qpw
{
    self = [super initWithContentRect:contentRect
            styleMask:windowStyle
            backing:NSBackingStoreBuffered
            defer:NO]; // Deferring window creation breaks OpenGL (the GL context is
                       // set up before the window is shown and needs a proper window)

    if (self) {
        _helper = [[QNSWindowHelper alloc] initWithNSWindow:self platformWindow:qpw];
    }
    return self;
}

- (BOOL)canBecomeKeyWindow
{
    // Prevent child NSWindows from becoming the key window in
    // order keep the active apperance of the top-level window.
    QCocoaWindow *pw = self.helper.platformWindow;
    if (!pw || pw->m_isNSWindowChild)
        return NO;

    if (pw->shouldRefuseKeyWindowAndFirstResponder())
        return NO;

    // The default implementation returns NO for title-bar less windows,
    // override and return yes here to make sure popup windows such as
    // the combobox popup can become the key window.
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    BOOL canBecomeMain = YES; // By default, windows can become the main window

    // Windows with a transient parent (such as combobox popup windows)
    // cannot become the main window:
    QCocoaWindow *pw = self.helper.platformWindow;
    if (!pw || pw->m_isNSWindowChild || pw->window()->transientParent())
        canBecomeMain = NO;

    return canBecomeMain;
}

- (void) sendEvent: (NSEvent*) theEvent
{
    [self.helper handleWindowEvent:theEvent];
}

- (void)superSendEvent:(NSEvent *)theEvent
{
    [super sendEvent:theEvent];
}

- (void)closeAndRelease
{
    [self close];

    if (self.helper.grabbingMouse) {
        self.helper.releaseOnMouseUp = YES;
    } else {
        [self.helper detachFromPlatformWindow];
        [self release];
    }
}

- (void)dealloc
{
    [_helper clearWindow];
    [_helper release];
    _helper = nil;
    [super dealloc];
}

@end

@implementation QNSPanel

@synthesize helper = _helper;

- (id)initWithContentRect:(NSRect)contentRect
      styleMask:(NSUInteger)windowStyle
      qPlatformWindow:(QCocoaWindow *)qpw
{
    self = [super initWithContentRect:contentRect
            styleMask:windowStyle
            backing:NSBackingStoreBuffered
            defer:NO]; // Deferring window creation breaks OpenGL (the GL context is
                       // set up before the window is shown and needs a proper window)

    if (self) {
        _helper = [[QNSWindowHelper alloc] initWithNSWindow:self platformWindow:qpw];
    }
    return self;
}

- (BOOL)canBecomeKeyWindow
{
    QCocoaWindow *pw = self.helper.platformWindow;
    if (!pw)
        return NO;

    if (pw->shouldRefuseKeyWindowAndFirstResponder())
        return NO;

    // Only tool or dialog windows should become key:
    Qt::WindowType type = pw->window()->type();
    if (type == Qt::Tool || type == Qt::Dialog)
        return YES;

    return NO;
}

- (void) sendEvent: (NSEvent*) theEvent
{
    [self.helper handleWindowEvent:theEvent];
}

- (void)superSendEvent:(NSEvent *)theEvent
{
    [super sendEvent:theEvent];
}

- (void)closeAndRelease
{
    [self.helper detachFromPlatformWindow];
    [self close];
    [self release];
}

- (void)dealloc
{
    [_helper clearWindow];
    [_helper release];
    _helper = nil;
    [super dealloc];
}

@end

const int QCocoaWindow::NoAlertRequest = -1;

QCocoaWindow::QCocoaWindow(QWindow *tlw)
    : QPlatformWindow(tlw)
    , m_view(nil)
    , m_nsWindow(0)
    , m_viewIsEmbedded(false)
    , m_viewIsToBeEmbedded(false)
    , m_parentCocoaWindow(0)
    , m_isNSWindowChild(false)
    , m_effectivelyMaximized(false)
    , m_synchedWindowState(Qt::WindowActive)
    , m_windowModality(Qt::NonModal)
    , m_windowUnderMouse(false)
    , m_inConstructor(true)
    , m_inSetVisible(false)
    , m_inSetGeometry(false)
    , m_inSetStyleMask(false)
#ifndef QT_NO_OPENGL
    , m_glContext(0)
#endif
    , m_menubar(0)
    , m_windowCursor(0)
    , m_hasModalSession(false)
    , m_frameStrutEventsEnabled(false)
    , m_geometryUpdateExposeAllowed(false)
    , m_isExposed(false)
    , m_registerTouchCount(0)
    , m_resizableTransientParent(false)
    , m_hiddenByClipping(false)
    , m_hiddenByAncestor(false)
    , m_alertRequest(NoAlertRequest)
    , monitor(nil)
    , m_drawContentBorderGradient(false)
    , m_topContentBorderThickness(0)
    , m_bottomContentBorderThickness(0)
    , m_normalGeometry(QRect(0,0,-1,-1))
    , m_hasWindowFilePath(false)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::QCocoaWindow" << window();

    QMacAutoReleasePool pool;

    if (tlw->type() == Qt::ForeignWindow) {
        m_view = (NSView *)WId(tlw->property("_q_foreignWinId").value<WId>());
    } else {
        m_view = [[QNSView alloc] initWithCocoaWindow:this];
        // Enable high-dpi OpenGL for retina displays. Enabling has the side
        // effect that Cocoa will start calling glViewport(0, 0, width, height),
        // overriding any glViewport calls in application code. This is usually not a
        // problem, except if the appilcation wants to have a "custom" viewport.
        // (like the hellogl example)
        if (tlw->supportsOpenGL()) {
            BOOL enable = qt_mac_resolveOption(YES, tlw, "_q_mac_wantsBestResolutionOpenGLSurface",
                                                          "QT_MAC_WANTS_BEST_RESOLUTION_OPENGL_SURFACE");
            [m_view setWantsBestResolutionOpenGLSurface:enable];
        }
        BOOL enable = qt_mac_resolveOption(NO, tlw, "_q_mac_wantsLayer",
                                                     "QT_MAC_WANTS_LAYER");
        [m_view setWantsLayer:enable];
    }
    setGeometry(tlw->geometry());
    recreateWindow(QPlatformWindow::parent());
    tlw->setGeometry(geometry());
    if (tlw->isTopLevel())
        setWindowIcon(tlw->icon());
    m_inConstructor = false;
}

QCocoaWindow::~QCocoaWindow()
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::~QCocoaWindow" << window();

    QMacAutoReleasePool pool;
    [m_nsWindow makeFirstResponder:nil];
    [m_nsWindow setContentView:nil];
    [m_nsWindow.helper detachFromPlatformWindow];
    if (m_isNSWindowChild) {
        if (m_parentCocoaWindow)
            m_parentCocoaWindow->removeChildWindow(this);
    } else if ([m_view superview]) {
        [m_view removeFromSuperview];
    }

    removeMonitor();

    // Make sure to disconnect observer in all case if view is valid
    // to avoid notifications received when deleting when using Qt::AA_NativeWindows attribute
    if (window()->type() != Qt::ForeignWindow)
        [[NSNotificationCenter defaultCenter] removeObserver:m_view];

    // While it is unlikely that this window will be in the popup stack
    // during deletetion we clear any pointers here to make sure.
    if (QCocoaIntegration::instance()) {
        QCocoaIntegration::instance()->popupWindowStack()->removeAll(this);
    }

    foreach (QCocoaWindow *child, m_childWindows) {
       [m_nsWindow removeChildWindow:child->m_nsWindow];
        child->m_parentCocoaWindow = 0;
    }

    [m_view release];
    [m_nsWindow release];
    [m_windowCursor release];
}

QSurfaceFormat QCocoaWindow::format() const
{
    QSurfaceFormat format = window()->requestedFormat();

    // Upgrade the default surface format to include an alpha channel. The default RGB format
    // causes Cocoa to spend an unreasonable amount of time converting it to RGBA internally.
    if (format == QSurfaceFormat())
        format.setAlphaBufferSize(8);
    return format;
}

void QCocoaWindow::setGeometry(const QRect &rectIn)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setGeometry" << window() << rectIn;

    QBoolBlocker inSetGeometry(m_inSetGeometry, true);

    QRect rect = rectIn;
    // This means it is a call from QWindow::setFramePosition() and
    // the coordinates include the frame (size is still the contents rectangle).
    if (qt_window_private(const_cast<QWindow *>(window()))->positionPolicy
            == QWindowPrivate::WindowFrameInclusive) {
        const QMargins margins = frameMargins();
        rect.moveTopLeft(rect.topLeft() + QPoint(margins.left(), margins.top()));
    }
    if (geometry() == rect)
        return;

    setCocoaGeometry(rect);
}

QRect QCocoaWindow::geometry() const
{
    // QWindows that are embedded in a NSView hiearchy may be considered
    // top-level from Qt's point of view but are not from Cocoa's point
    // of view. Embedded QWindows get global (screen) geometry.
    if (m_viewIsEmbedded) {
        NSPoint windowPoint = [m_view convertPoint:NSMakePoint(0, 0) toView:nil];
        NSRect screenRect = [[m_view window] convertRectToScreen:NSMakeRect(windowPoint.x, windowPoint.y, 1, 1)];
        NSPoint screenPoint = screenRect.origin;
        QPoint position = qt_mac_flipPoint(screenPoint).toPoint();
        QSize size = QRectF::fromCGRect([m_view bounds]).toRect().size();
        return QRect(position, size);
    }

    return QPlatformWindow::geometry();
}

void QCocoaWindow::setCocoaGeometry(const QRect &rect)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setCocoaGeometry" << window() << rect;
    QMacAutoReleasePool pool;

    if (m_viewIsEmbedded) {
        if (window()->type() != Qt::ForeignWindow) {
            [m_view setFrame:NSMakeRect(0, 0, rect.width(), rect.height())];
        } else {
            QPlatformWindow::setGeometry(rect);
        }
        return;
    }

    if (m_isNSWindowChild) {
        QPlatformWindow::setGeometry(rect);
        NSWindow *parentNSWindow = m_parentCocoaWindow->m_nsWindow;
        NSRect parentWindowFrame = [parentNSWindow contentRectForFrameRect:parentNSWindow.frame];
        clipWindow(parentWindowFrame);

        // call this here: updateGeometry in qnsview.mm is a no-op for this case
        QWindowSystemInterface::handleGeometryChange(window(), rect);
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), rect.size()));
    } else if (m_nsWindow) {
        NSRect bounds = qt_mac_flipRect(rect);
        [m_nsWindow setFrame:[m_nsWindow frameRectForContentRect:bounds] display:YES animate:NO];
    } else {
        [m_view setFrame:NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height())];
    }

    if (window()->type() == Qt::ForeignWindow)
        QPlatformWindow::setGeometry(rect);

    // will call QPlatformWindow::setGeometry(rect) during resize confirmation (see qnsview.mm)
}

void QCocoaWindow::clipChildWindows()
{
    foreach (QCocoaWindow *childWindow, m_childWindows) {
        childWindow->clipWindow(m_nsWindow.frame);
    }
}

void QCocoaWindow::clipWindow(const NSRect &clipRect)
{
    if (!m_isNSWindowChild)
        return;

    NSRect clippedWindowRect = NSZeroRect;
    if (!NSIsEmptyRect(clipRect)) {
        NSRect windowFrame = qt_mac_flipRect(QRect(window()->mapToGlobal(QPoint(0, 0)), geometry().size()));
        clippedWindowRect = NSIntersectionRect(windowFrame, clipRect);
        // Clipping top/left offsets the content. Move it back.
        NSPoint contentViewOffset = NSMakePoint(qMax(CGFloat(0), NSMinX(clippedWindowRect) - NSMinX(windowFrame)),
                                                qMax(CGFloat(0), NSMaxY(windowFrame) - NSMaxY(clippedWindowRect)));
        [m_view setBoundsOrigin:contentViewOffset];
    }

    if (NSIsEmptyRect(clippedWindowRect)) {
        if (!m_hiddenByClipping) {
            // We dont call hide() here as we will recurse further down
            [m_nsWindow orderOut:nil];
            m_hiddenByClipping = true;
        }
    } else {
        [m_nsWindow setFrame:clippedWindowRect display:YES animate:NO];
        if (m_hiddenByClipping) {
            m_hiddenByClipping = false;
            if (!m_hiddenByAncestor) {
                [m_nsWindow orderFront:nil];
                m_parentCocoaWindow->reinsertChildWindow(this);
            }
        }
    }

    // recurse
    foreach (QCocoaWindow *childWindow, m_childWindows) {
        childWindow->clipWindow(clippedWindowRect);
    }
}

void QCocoaWindow::hide(bool becauseOfAncestor)
{
    bool visible = [m_nsWindow isVisible];

    if (!m_hiddenByAncestor && !visible) // Already explicitly hidden
        return;
    if (m_hiddenByAncestor && becauseOfAncestor) // Trying to hide some child again
        return;

    m_hiddenByAncestor = becauseOfAncestor;

    if (!visible) // Could have been clipped before
        return;

    foreach (QCocoaWindow *childWindow, m_childWindows)
        childWindow->hide(true);

    [m_nsWindow orderOut:nil];
}

void QCocoaWindow::show(bool becauseOfAncestor)
{
    if ([m_nsWindow isVisible])
        return;

    if (m_parentCocoaWindow && ![m_parentCocoaWindow->m_nsWindow isVisible]) {
        m_hiddenByAncestor = true; // Parent still hidden, don't show now
    } else if ((becauseOfAncestor == m_hiddenByAncestor) // Was NEITHER explicitly hidden
               && !m_hiddenByClipping) { // ... NOR clipped
        if (m_isNSWindowChild) {
            m_hiddenByAncestor = false;
            setCocoaGeometry(windowGeometry());
        }
        if (!m_hiddenByClipping) { // setCocoaGeometry() can change the clipping status
            [m_nsWindow orderFront:nil];
            if (m_isNSWindowChild)
                m_parentCocoaWindow->reinsertChildWindow(this);
            foreach (QCocoaWindow *childWindow, m_childWindows)
                childWindow->show(true);
        }
    }
}

void QCocoaWindow::setVisible(bool visible)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setVisible" << window() << visible;

    if (m_isNSWindowChild && m_hiddenByClipping)
        return;

    m_inSetVisible = true;

    QMacAutoReleasePool pool;
    QCocoaWindow *parentCocoaWindow = 0;
    if (window()->transientParent())
        parentCocoaWindow = static_cast<QCocoaWindow *>(window()->transientParent()->handle());

    if (visible) {
        // We need to recreate if the modality has changed as the style mask will need updating
        if (m_windowModality != window()->modality() || isNativeWindowTypeInconsistent())
            recreateWindow(QPlatformWindow::parent());

        // Register popup windows. The Cocoa platform plugin will forward mouse events
        // to them and close them when needed.
        if (window()->type() == Qt::Popup || window()->type() == Qt::ToolTip)
            QCocoaIntegration::instance()->pushPopupWindow(this);

        if (parentCocoaWindow) {
            // The parent window might have moved while this window was hidden,
            // update the window geometry if there is a parent.
            setGeometry(windowGeometry());

            if (window()->type() == Qt::Popup) {
                // QTBUG-30266: a window should not be resizable while a transient popup is open
                // Since this isn't a native popup, the window manager doesn't close the popup when you click outside
                NSUInteger parentStyleMask = [parentCocoaWindow->m_nsWindow styleMask];
                if ((m_resizableTransientParent = (parentStyleMask & NSResizableWindowMask))
                    && !([parentCocoaWindow->m_nsWindow styleMask] & NSFullScreenWindowMask))
                    [parentCocoaWindow->m_nsWindow setStyleMask:parentStyleMask & ~NSResizableWindowMask];
            }

        }

        // This call is here to handle initial window show correctly:
        // - top-level windows need to have backing store content ready when the
        //   window is shown, sendin the expose event here makes that more likely.
        // - QNSViews for child windows are initialy not hidden and won't get the
        //   viewDidUnhide message.
        exposeWindow();

        if (m_nsWindow) {
            QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);

            // setWindowState might have been called while the window was hidden and
            // will not change the NSWindow state in that case. Sync up here:
            syncWindowState(window()->windowState());

            if (window()->windowState() != Qt::WindowMinimized) {
                if ((window()->modality() == Qt::WindowModal
                     || window()->type() == Qt::Sheet)
                        && parentCocoaWindow) {
                    // show the window as a sheet
                    [NSApp beginSheet:m_nsWindow modalForWindow:parentCocoaWindow->m_nsWindow modalDelegate:nil didEndSelector:nil contextInfo:nil];
                } else if (window()->modality() != Qt::NonModal) {
                    // show the window as application modal
                    QCocoaEventDispatcher *cocoaEventDispatcher = qobject_cast<QCocoaEventDispatcher *>(QGuiApplication::instance()->eventDispatcher());
                    Q_ASSERT(cocoaEventDispatcher != 0);
                    QCocoaEventDispatcherPrivate *cocoaEventDispatcherPrivate = static_cast<QCocoaEventDispatcherPrivate *>(QObjectPrivate::get(cocoaEventDispatcher));
                    cocoaEventDispatcherPrivate->beginModalSession(window());
                    m_hasModalSession = true;
                } else if ([m_nsWindow canBecomeKeyWindow]) {
                    QCocoaEventDispatcher *cocoaEventDispatcher = qobject_cast<QCocoaEventDispatcher *>(QGuiApplication::instance()->eventDispatcher());
                    QCocoaEventDispatcherPrivate *cocoaEventDispatcherPrivate = 0;
                    if (cocoaEventDispatcher)
                        cocoaEventDispatcherPrivate = static_cast<QCocoaEventDispatcherPrivate *>(QObjectPrivate::get(cocoaEventDispatcher));

                    if (!(cocoaEventDispatcherPrivate && cocoaEventDispatcherPrivate->currentModalSession()))
                        [m_nsWindow makeKeyAndOrderFront:nil];
                    else
                        [m_nsWindow orderFront:nil];

                    foreach (QCocoaWindow *childWindow, m_childWindows)
                        childWindow->show(true);
                } else {
                    show();
                }

                // We want the events to properly reach the popup, dialog, and tool
                if ((window()->type() == Qt::Popup || window()->type() == Qt::Dialog || window()->type() == Qt::Tool)
                    && [m_nsWindow isKindOfClass:[NSPanel class]]) {
                    [(NSPanel *)m_nsWindow setWorksWhenModal:YES];
                    if (!(parentCocoaWindow && window()->transientParent()->isActive()) && window()->type() == Qt::Popup) {
                        removeMonitor();
                        monitor = [NSEvent addGlobalMonitorForEventsMatchingMask:NSLeftMouseDownMask|NSRightMouseDownMask|NSOtherMouseDownMask|NSMouseMovedMask handler:^(NSEvent *e) {
                            QPointF localPoint = qt_mac_flipPoint([NSEvent mouseLocation]);
                            QWindowSystemInterface::handleMouseEvent(window(), window()->mapFromGlobal(localPoint.toPoint()), localPoint,
                                                                     cocoaButton2QtButton([e buttonNumber]));
                        }];
                    }
                }
            }
        }
        // In some cases, e.g. QDockWidget, the content view is hidden before moving to its own
        // Cocoa window, and then shown again. Therefore, we test for the view being hidden even
        // if it's attached to an NSWindow.
        if ([m_view isHidden])
            [m_view setHidden:NO];
    } else {
        // qDebug() << "close" << this;
#ifndef QT_NO_OPENGL
        if (m_glContext)
            m_glContext->windowWasHidden();
#endif
        QCocoaEventDispatcher *cocoaEventDispatcher = qobject_cast<QCocoaEventDispatcher *>(QGuiApplication::instance()->eventDispatcher());
        QCocoaEventDispatcherPrivate *cocoaEventDispatcherPrivate = 0;
        if (cocoaEventDispatcher)
            cocoaEventDispatcherPrivate = static_cast<QCocoaEventDispatcherPrivate *>(QObjectPrivate::get(cocoaEventDispatcher));
        if (m_nsWindow) {
            if (m_hasModalSession) {
                if (cocoaEventDispatcherPrivate)
                    cocoaEventDispatcherPrivate->endModalSession(window());
                m_hasModalSession = false;
            } else {
                if ([m_nsWindow isSheet])
                    [NSApp endSheet:m_nsWindow];
            }

            hide();
            if (m_nsWindow == [NSApp keyWindow]
                && !(cocoaEventDispatcherPrivate && cocoaEventDispatcherPrivate->currentModalSession())) {
                // Probably because we call runModalSession: outside [NSApp run] in QCocoaEventDispatcher
                // (e.g., when show()-ing a modal QDialog instead of exec()-ing it), it can happen that
                // the current NSWindow is still key after being ordered out. Then, after checking we
                // don't have any other modal session left, it's safe to make the main window key again.
                NSWindow *mainWindow = [NSApp mainWindow];
                if (mainWindow && [mainWindow canBecomeKeyWindow])
                    [mainWindow makeKeyWindow];
            }
        } else {
            [m_view setHidden:YES];
        }
        removeMonitor();

        if (window()->type() == Qt::Popup || window()->type() == Qt::ToolTip)
            QCocoaIntegration::instance()->popupWindowStack()->removeAll(this);

        if (parentCocoaWindow && window()->type() == Qt::Popup) {
            if (m_resizableTransientParent
                && !([parentCocoaWindow->m_nsWindow styleMask] & NSFullScreenWindowMask))
                // QTBUG-30266: a window should not be resizable while a transient popup is open
                [parentCocoaWindow->m_nsWindow setStyleMask:[parentCocoaWindow->m_nsWindow styleMask] | NSResizableWindowMask];
        }
    }

    m_inSetVisible = false;
}

NSInteger QCocoaWindow::windowLevel(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    NSInteger windowLevel = NSNormalWindowLevel;

    if (type == Qt::Tool)
        windowLevel = NSFloatingWindowLevel;
    else if ((type & Qt::Popup) == Qt::Popup)
        windowLevel = NSPopUpMenuWindowLevel;

    // StayOnTop window should appear above Tool windows.
    if (flags & Qt::WindowStaysOnTopHint)
        windowLevel = NSModalPanelWindowLevel;
    // Tooltips should appear above StayOnTop windows.
    if (type == Qt::ToolTip)
        windowLevel = NSScreenSaverWindowLevel;

    // Any "special" window should be in at least the same level as its parent.
    if (type != Qt::Window) {
        const QWindow * const transientParent = window()->transientParent();
        const QCocoaWindow * const transientParentWindow = transientParent ? static_cast<QCocoaWindow *>(transientParent->handle()) : 0;
        if (transientParentWindow)
            windowLevel = qMax([transientParentWindow->m_nsWindow level], windowLevel);
    }

    return windowLevel;
}

NSUInteger QCocoaWindow::windowStyleMask(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));
    NSInteger styleMask = NSBorderlessWindowMask;
    if (flags & Qt::FramelessWindowHint)
        return styleMask;
    if ((type & Qt::Popup) == Qt::Popup) {
        if (!windowIsPopupType(type)) {
            styleMask = NSUtilityWindowMask | NSResizableWindowMask;
            if (!(flags & Qt::CustomizeWindowHint)) {
                styleMask |= NSClosableWindowMask | NSMiniaturizableWindowMask | NSTitledWindowMask;
            } else {
                if (flags & Qt::WindowTitleHint)
                    styleMask |= NSTitledWindowMask;
                if (flags & Qt::WindowCloseButtonHint)
                    styleMask |= NSClosableWindowMask;
                if (flags & Qt::WindowMinimizeButtonHint)
                    styleMask |= NSMiniaturizableWindowMask;
            }
        }
    } else {
        if (type == Qt::Window && !(flags & Qt::CustomizeWindowHint)) {
            styleMask = (NSResizableWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSTitledWindowMask);
        } else if (type == Qt::Dialog) {
            if (flags & Qt::CustomizeWindowHint) {
                if (flags & Qt::WindowMaximizeButtonHint)
                    styleMask = NSResizableWindowMask;
                if (flags & Qt::WindowTitleHint)
                    styleMask |= NSTitledWindowMask;
                if (flags & Qt::WindowCloseButtonHint)
                    styleMask |= NSClosableWindowMask;
                if (flags & Qt::WindowMinimizeButtonHint)
                    styleMask |= NSMiniaturizableWindowMask;
            } else {
                styleMask = NSResizableWindowMask | NSClosableWindowMask | NSTitledWindowMask;
            }
        } else {
            if (flags & Qt::WindowMaximizeButtonHint)
                styleMask |= NSResizableWindowMask;
            if (flags & Qt::WindowTitleHint)
                styleMask |= NSTitledWindowMask;
            if (flags & Qt::WindowCloseButtonHint)
                styleMask |= NSClosableWindowMask;
            if (flags & Qt::WindowMinimizeButtonHint)
                styleMask |= NSMiniaturizableWindowMask;
        }
    }

    if (m_drawContentBorderGradient)
        styleMask |= NSTexturedBackgroundWindowMask;

    return styleMask;
}

void QCocoaWindow::setWindowShadow(Qt::WindowFlags flags)
{
    bool keepShadow = !(flags & Qt::NoDropShadowWindowHint);
    [m_nsWindow setHasShadow:(keepShadow ? YES : NO)];
}

void QCocoaWindow::setWindowZoomButton(Qt::WindowFlags flags)
{
    // Disable the zoom (maximize) button for fixed-sized windows and customized
    // no-WindowMaximizeButtonHint windows. From a Qt perspective it migth be expected
    // that the button would be removed in the latter case, but disabling it is more
    // in line with the platform style guidelines.
    bool fixedSizeNoZoom = (windowMinimumSize().isValid() && windowMaximumSize().isValid()
                            && windowMinimumSize() == windowMaximumSize());
    bool customizeNoZoom = ((flags & Qt::CustomizeWindowHint) && !(flags & Qt::WindowMaximizeButtonHint));
    [[m_nsWindow standardWindowButton:NSWindowZoomButton] setEnabled:!(fixedSizeNoZoom || customizeNoZoom)];
}

void QCocoaWindow::setWindowFlags(Qt::WindowFlags flags)
{
    if (m_nsWindow && !m_isNSWindowChild) {
        NSUInteger styleMask = windowStyleMask(flags);
        NSInteger level = this->windowLevel(flags);
        // While setting style mask we can have -updateGeometry calls on a content
        // view with null geometry, reporting an invalid coordinates as a result.
        m_inSetStyleMask = true;
        [m_nsWindow setStyleMask:styleMask];
        m_inSetStyleMask = false;
        [m_nsWindow setLevel:level];
        setWindowShadow(flags);
        if (!(flags & Qt::FramelessWindowHint)) {
            setWindowTitle(window()->title());
        }

        Qt::WindowType type = window()->type();
        if ((type & Qt::Popup) != Qt::Popup && (type & Qt::Dialog) != Qt::Dialog) {
            NSWindowCollectionBehavior behavior = [m_nsWindow collectionBehavior];
            if (flags & Qt::WindowFullscreenButtonHint) {
                behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
                behavior &= ~NSWindowCollectionBehaviorFullScreenAuxiliary;
            } else {
                behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
                behavior &= ~NSWindowCollectionBehaviorFullScreenPrimary;
            }
            [m_nsWindow setCollectionBehavior:behavior];
        }
        setWindowZoomButton(flags);
    }

    m_windowFlags = flags;
}

void QCocoaWindow::setWindowState(Qt::WindowState state)
{
    if (window()->isVisible())
        syncWindowState(state);  // Window state set for hidden windows take effect when show() is called.
}

void QCocoaWindow::setWindowTitle(const QString &title)
{
    QMacAutoReleasePool pool;
    if (!m_nsWindow)
        return;

    CFStringRef windowTitle = title.toCFString();
    [m_nsWindow setTitle: const_cast<NSString *>(reinterpret_cast<const NSString *>(windowTitle))];
    CFRelease(windowTitle);
}

void QCocoaWindow::setWindowFilePath(const QString &filePath)
{
    QMacAutoReleasePool pool;
    if (!m_nsWindow)
        return;

    QFileInfo fi(filePath);
    [m_nsWindow setRepresentedFilename:fi.exists() ? filePath.toNSString() : @""];
    m_hasWindowFilePath = fi.exists();
}

void QCocoaWindow::setWindowIcon(const QIcon &icon)
{
    QMacAutoReleasePool pool;

    NSButton *iconButton = [m_nsWindow standardWindowButton:NSWindowDocumentIconButton];
    if (iconButton == nil) {
        if (icon.isNull())
            return;
        NSString *title = window()->title().toNSString();
        [m_nsWindow setRepresentedURL:[NSURL fileURLWithPath:title]];
        iconButton = [m_nsWindow standardWindowButton:NSWindowDocumentIconButton];
    }
    if (icon.isNull()) {
        [iconButton setImage:nil];
    } else {
        QPixmap pixmap = icon.pixmap(QSize(22, 22));
        NSImage *image = static_cast<NSImage *>(qt_mac_create_nsimage(pixmap));
        [iconButton setImage:image];
        [image release];
    }
}

void QCocoaWindow::setAlertState(bool enabled)
{
    if (m_alertRequest == NoAlertRequest && enabled) {
        m_alertRequest = [NSApp requestUserAttention:NSCriticalRequest];
    } else if (m_alertRequest != NoAlertRequest && !enabled) {
        [NSApp cancelUserAttentionRequest:m_alertRequest];
        m_alertRequest = NoAlertRequest;
    }
}

bool QCocoaWindow::isAlertState() const
{
    return m_alertRequest != NoAlertRequest;
}

void QCocoaWindow::raise()
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::raise" << window();

    // ### handle spaces (see Qt 4 raise_sys in qwidget_mac.mm)
    if (!m_nsWindow)
        return;
    if (m_isNSWindowChild) {
        QList<QCocoaWindow *> &siblings = m_parentCocoaWindow->m_childWindows;
        siblings.removeOne(this);
        siblings.append(this);
        if (m_hiddenByClipping)
            return;
    }
    if ([m_nsWindow isVisible]) {
        if (m_isNSWindowChild) {
            // -[NSWindow orderFront:] doesn't work with attached windows.
            // The only solution is to remove and add the child window.
            // This will place it on top of all the other NSWindows.
            NSWindow *parentNSWindow = m_parentCocoaWindow->m_nsWindow;
            [parentNSWindow removeChildWindow:m_nsWindow];
            [parentNSWindow addChildWindow:m_nsWindow ordered:NSWindowAbove];
        } else {
            {
                // Clean up autoreleased temp objects from orderFront immediately.
                // Failure to do so has been observed to cause leaks also beyond any outer
                // autorelease pool (for example around a complete QWindow
                // construct-show-raise-hide-delete cyle), counter to expected autoreleasepool
                // behavior.
                QMacAutoReleasePool pool;
                [m_nsWindow orderFront: m_nsWindow];
            }
            static bool raiseProcess = qt_mac_resolveOption(true, "QT_MAC_SET_RAISE_PROCESS");
            if (raiseProcess) {
                [NSApp activateIgnoringOtherApps:YES];
            }
        }
    }
}

void QCocoaWindow::lower()
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::lower" << window();
    if (!m_nsWindow)
        return;
    if (m_isNSWindowChild) {
        QList<QCocoaWindow *> &siblings = m_parentCocoaWindow->m_childWindows;
        siblings.removeOne(this);
        siblings.prepend(this);
        if (m_hiddenByClipping)
            return;
    }
    if ([m_nsWindow isVisible]) {
        if (m_isNSWindowChild) {
            // -[NSWindow orderBack:] doesn't work with attached windows.
            // The only solution is to remove and add all the child windows except this one.
            // This will keep the current window at the bottom while adding the others on top of it,
            // hopefully in the same order (this is not documented anywhere in the Cocoa documentation).
            NSWindow *parentNSWindow = m_parentCocoaWindow->m_nsWindow;
            NSArray *children = [parentNSWindow.childWindows copy];
            for (NSWindow *child in children)
                if (m_nsWindow != child) {
                    [parentNSWindow removeChildWindow:child];
                    [parentNSWindow addChildWindow:child ordered:NSWindowAbove];
                }
        } else {
            [m_nsWindow orderBack: m_nsWindow];
        }
    }
}

bool QCocoaWindow::isExposed() const
{
    return m_isExposed;
}

bool QCocoaWindow::isOpaque() const
{
    // OpenGL surfaces can be ordered either above(default) or below the NSWindow.
    // When ordering below the window must be tranclucent.
    static GLint openglSourfaceOrder = qt_mac_resolveOption(1, "QT_MAC_OPENGL_SURFACE_ORDER");

    bool translucent = window()->format().alphaBufferSize() > 0
                        || window()->opacity() < 1
                        || [qnsview_cast(m_view) hasMask]
                        || (surface()->supportsOpenGL() && openglSourfaceOrder == -1);
    return !translucent;
}

void QCocoaWindow::propagateSizeHints()
{
    QMacAutoReleasePool pool;
    if (!m_nsWindow)
        return;

    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::propagateSizeHints" << window() << "\n"
                              << "       min/max" << windowMinimumSize() << windowMaximumSize()
                              << "size increment" << windowSizeIncrement()
                              << "      basesize" << windowBaseSize()
                              << "      geometry" << windowGeometry();

    // Set the minimum content size.
    const QSize minimumSize = windowMinimumSize();
    if (!minimumSize.isValid()) // minimumSize is (-1, -1) when not set. Make that (0, 0) for Cocoa.
        [m_nsWindow setContentMinSize : NSMakeSize(0.0, 0.0)];
    [m_nsWindow setContentMinSize : NSMakeSize(minimumSize.width(), minimumSize.height())];

    // Set the maximum content size.
    const QSize maximumSize = windowMaximumSize();
    [m_nsWindow setContentMaxSize : NSMakeSize(maximumSize.width(), maximumSize.height())];

    // The window may end up with a fixed size; in this case the zoom button should be disabled.
    setWindowZoomButton(m_windowFlags);

    // sizeIncrement is observed to take values of (-1, -1) and (0, 0) for windows that should be
    // resizable and that have no specific size increment set. Cocoa expects (1.0, 1.0) in this case.
    QSize sizeIncrement = windowSizeIncrement();
    if (sizeIncrement.isEmpty())
        sizeIncrement = QSize(1, 1);
    [m_nsWindow setResizeIncrements:sizeIncrement.toCGSize()];

    QRect rect = geometry();
    QSize baseSize = windowBaseSize();
    if (!baseSize.isNull() && baseSize.isValid()) {
        [m_nsWindow setFrame:NSMakeRect(rect.x(), rect.y(), baseSize.width(), baseSize.height()) display:YES];
    }
}

void QCocoaWindow::setOpacity(qreal level)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setOpacity" << level;
    if (m_nsWindow) {
        [m_nsWindow setAlphaValue:level];
        [m_nsWindow setOpaque: isOpaque()];
    }
}

void QCocoaWindow::setMask(const QRegion &region)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setMask" << window() << region;
    if (m_nsWindow)
        [m_nsWindow setBackgroundColor:[NSColor clearColor]];

    [qnsview_cast(m_view) setMaskRegion:&region];
    [m_nsWindow setOpaque:isOpaque()];
}

bool QCocoaWindow::setKeyboardGrabEnabled(bool grab)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setKeyboardGrabEnabled" << window() << grab;
    if (!m_nsWindow)
        return false;

    if (grab && ![m_nsWindow isKeyWindow])
        [m_nsWindow makeKeyWindow];

    return true;
}

bool QCocoaWindow::setMouseGrabEnabled(bool grab)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setMouseGrabEnabled" << window() << grab;
    if (!m_nsWindow)
        return false;

    if (grab && ![m_nsWindow isKeyWindow])
        [m_nsWindow makeKeyWindow];

    return true;
}

WId QCocoaWindow::winId() const
{
    return WId(m_view);
}

void QCocoaWindow::setParent(const QPlatformWindow *parentWindow)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setParent" << window() << (parentWindow ? parentWindow->window() : 0);

    // recreate the window for compatibility
    bool unhideAfterRecreate = parentWindow && !m_viewIsToBeEmbedded && ![m_view isHidden];
    recreateWindow(parentWindow);
    if (unhideAfterRecreate)
        [m_view setHidden:NO];
    setCocoaGeometry(geometry());
}

NSView *QCocoaWindow::view() const
{
    return m_view;
}

NSWindow *QCocoaWindow::nativeWindow() const
{
    return m_nsWindow;
}

void QCocoaWindow::setEmbeddedInForeignView(bool embedded)
{
    m_viewIsToBeEmbedded = embedded;
    // Release any previosly created NSWindow.
    [m_nsWindow closeAndRelease];
    m_nsWindow = 0;
}

void QCocoaWindow::windowWillMove()
{
    // Close any open popups on window move
    qt_closePopups();
}

void QCocoaWindow::windowDidMove()
{
    if (m_isNSWindowChild)
        return;

    [qnsview_cast(m_view) updateGeometry];
}

void QCocoaWindow::windowDidResize()
{
    if (!m_nsWindow)
        return;

    if (m_isNSWindowChild)
        return;

    clipChildWindows();
    [qnsview_cast(m_view) updateGeometry];
}

void QCocoaWindow::windowDidEndLiveResize()
{
    if (m_synchedWindowState == Qt::WindowMaximized && ![m_nsWindow isZoomed]) {
        m_effectivelyMaximized = false;
        [qnsview_cast(m_view) notifyWindowStateChanged:Qt::WindowNoState];
    }
}

bool QCocoaWindow::windowShouldClose()
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::windowShouldClose" << window();
   // This callback should technically only determine if the window
   // should (be allowed to) close, but since our QPA API to determine
   // that also involves actually closing the window we do both at the
   // same time, instead of doing the latter in windowWillClose.
    bool accepted = false;
    QWindowSystemInterface::handleCloseEvent(window(), &accepted);
    QWindowSystemInterface::flushWindowSystemEvents();
    return accepted;
}

void QCocoaWindow::windowWillClose()
{
    // Close any open popups on window closing.
    if (window() && !windowIsPopupType(window()->type()))
        qt_closePopups();
}

void QCocoaWindow::setSynchedWindowStateFromWindow()
{
    if (QWindow *w = window())
        m_synchedWindowState = w->windowState();
}

bool QCocoaWindow::windowIsPopupType(Qt::WindowType type) const
{
    if (type == Qt::Widget)
        type = window()->type();
    if (type == Qt::Tool)
        return false; // Qt::Tool has the Popup bit set but isn't, at least on Mac.

    return ((type & Qt::Popup) == Qt::Popup);
}

#ifndef QT_NO_OPENGL
void QCocoaWindow::setCurrentContext(QCocoaGLContext *context)
{
    m_glContext = context;
}

QCocoaGLContext *QCocoaWindow::currentContext() const
{
    return m_glContext;
}
#endif

void QCocoaWindow::recreateWindow(const QPlatformWindow *parentWindow)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::recreateWindow" << window()
                              << "parent" << (parentWindow ? parentWindow->window() : 0);

    bool wasNSWindowChild = m_isNSWindowChild;
    BOOL requestNSWindowChild = qt_mac_resolveOption(NO, window(), "_q_platform_MacUseNSWindow",
                                                                   "QT_MAC_USE_NSWINDOW");
    m_isNSWindowChild = parentWindow && requestNSWindowChild;
    bool needsNSWindow = m_isNSWindowChild || !parentWindow;

    QCocoaWindow *oldParentCocoaWindow = m_parentCocoaWindow;
    m_parentCocoaWindow = const_cast<QCocoaWindow *>(static_cast<const QCocoaWindow *>(parentWindow));
    if (m_parentCocoaWindow && m_isNSWindowChild) {
        QWindow *parentQWindow = m_parentCocoaWindow->window();
        if (!parentQWindow->property("_q_platform_MacUseNSWindow").toBool()) {
            parentQWindow->setProperty("_q_platform_MacUseNSWindow", QVariant(true));
            m_parentCocoaWindow->recreateWindow(m_parentCocoaWindow->m_parentCocoaWindow);
        }
    }

    bool usesNSPanel = [m_nsWindow isKindOfClass:[QNSPanel class]];

    // No child QNSWindow should notify its QNSView
    if (m_nsWindow && (window()->type() != Qt::ForeignWindow) && m_parentCocoaWindow && !oldParentCocoaWindow)
        [[NSNotificationCenter defaultCenter] removeObserver:m_view
                                              name:nil object:m_nsWindow];

    // Remove current window (if any)
    if ((m_nsWindow && !needsNSWindow) || (usesNSPanel != shouldUseNSPanel())) {
        [m_nsWindow closeAndRelease];
        if (wasNSWindowChild && oldParentCocoaWindow)
            oldParentCocoaWindow->removeChildWindow(this);
        m_nsWindow = 0;
    }

    if (needsNSWindow) {
        bool noPreviousWindow = m_nsWindow == 0;
        if (noPreviousWindow)
            m_nsWindow = createNSWindow();

        // Only non-child QNSWindows should notify their QNSViews
        // (but don't register more than once).
        if ((window()->type() != Qt::ForeignWindow) && (noPreviousWindow || (wasNSWindowChild && !m_isNSWindowChild)))
            [[NSNotificationCenter defaultCenter] addObserver:m_view
                                                  selector:@selector(windowNotification:)
                                                  name:nil // Get all notifications
                                                  object:m_nsWindow];

        if (oldParentCocoaWindow) {
            if (!m_isNSWindowChild || oldParentCocoaWindow != m_parentCocoaWindow)
                oldParentCocoaWindow->removeChildWindow(this);
            m_forwardWindow = oldParentCocoaWindow;
        }

        setNSWindow(m_nsWindow);
    }

    if (m_viewIsToBeEmbedded) {
        // An embedded window doesn't have its own NSWindow.
    } else if (!parentWindow) {
        // QPlatformWindow subclasses must sync up with QWindow on creation:
        propagateSizeHints();
        setWindowFlags(window()->flags());
        setWindowTitle(window()->title());
        setWindowState(window()->windowState());
    } else if (m_isNSWindowChild) {
        m_nsWindow.styleMask = NSBorderlessWindowMask;
        m_nsWindow.hasShadow = NO;
        m_nsWindow.level = NSNormalWindowLevel;
        NSWindowCollectionBehavior collectionBehavior =
                NSWindowCollectionBehaviorManaged | NSWindowCollectionBehaviorIgnoresCycle
                | NSWindowCollectionBehaviorFullScreenAuxiliary;
        m_nsWindow.animationBehavior = NSWindowAnimationBehaviorNone;
        m_nsWindow.collectionBehavior = collectionBehavior;
        setCocoaGeometry(windowGeometry());

        QList<QCocoaWindow *> &siblings = m_parentCocoaWindow->m_childWindows;
        if (siblings.contains(this)) {
            if (!m_hiddenByClipping)
                m_parentCocoaWindow->reinsertChildWindow(this);
        } else {
            if (!m_hiddenByClipping)
                [m_parentCocoaWindow->m_nsWindow addChildWindow:m_nsWindow ordered:NSWindowAbove];
            siblings.append(this);
        }
    } else {
        // Child windows have no NSWindow, link the NSViews instead.
        if ([m_view superview])
            [m_view removeFromSuperview];

        [m_parentCocoaWindow->m_view addSubview:m_view];
        QRect rect = windowGeometry();
        // Prevent setting a (0,0) window size; causes opengl context
        // "Invalid Drawable" warnings.
        if (rect.isNull())
            rect.setSize(QSize(1, 1));
        NSRect frame = NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height());
        [m_view setFrame:frame];
        [m_view setHidden:!window()->isVisible()];
    }

    m_nsWindow.ignoresMouseEvents =
        (window()->flags() & Qt::WindowTransparentForInput) == Qt::WindowTransparentForInput;

    const qreal opacity = qt_window_private(window())->opacity;
    if (!qFuzzyCompare(opacity, qreal(1.0)))
        setOpacity(opacity);

    // top-level QWindows may have an attached NSToolBar, call
    // update function which will attach to the NSWindow.
    if (!parentWindow)
        updateNSToolbar();
}

void QCocoaWindow::reinsertChildWindow(QCocoaWindow *child)
{
    int childIndex = m_childWindows.indexOf(child);
    Q_ASSERT(childIndex != -1);

    for (int i = childIndex; i < m_childWindows.size(); i++) {
        NSWindow *nsChild = m_childWindows[i]->m_nsWindow;
        if (i != childIndex)
            [m_nsWindow removeChildWindow:nsChild];
        [m_nsWindow addChildWindow:nsChild ordered:NSWindowAbove];
    }
}

void QCocoaWindow::requestActivateWindow()
{
    NSWindow *window = [m_view window];
    [window makeFirstResponder:m_view];
    [window makeKeyWindow];
}

bool QCocoaWindow::shouldUseNSPanel()
{
    Qt::WindowType type = window()->type();

    return !m_isNSWindowChild &&
           ((type & Qt::Popup) == Qt::Popup || (type & Qt::Dialog) == Qt::Dialog);
}

QCocoaNSWindow * QCocoaWindow::createNSWindow()
{
    QMacAutoReleasePool pool;

    QRect rect = initialGeometry(window(), windowGeometry(), defaultWindowWidth, defaultWindowHeight);
    NSRect frame = qt_mac_flipRect(rect);

    Qt::WindowType type = window()->type();
    Qt::WindowFlags flags = window()->flags();

    NSUInteger styleMask;
    if (m_isNSWindowChild) {
        styleMask = NSBorderlessWindowMask;
    } else {
        styleMask = windowStyleMask(flags);
    }
    QCocoaNSWindow *createdWindow = 0;

    // Use NSPanel for popup-type windows. (Popup, Tool, ToolTip, SplashScreen)
    // and dialogs
    if (shouldUseNSPanel()) {
        QNSPanel *window;
        window  = [[QNSPanel alloc] initWithContentRect:frame
                                    styleMask: styleMask
                                    qPlatformWindow:this];
        if ((type & Qt::Popup) == Qt::Popup)
            [window setHasShadow:YES];

        // Qt::Tool windows hide on app deactivation, unless Qt::WA_MacAlwaysShowToolWindow is set.
        QVariant showWithoutActivating = QPlatformWindow::window()->property("_q_macAlwaysShowToolWindow");
        bool shouldHideOnDeactivate = ((type & Qt::Tool) == Qt::Tool) &&
                                      !(showWithoutActivating.isValid() && showWithoutActivating.toBool());
        [window setHidesOnDeactivate: shouldHideOnDeactivate];

        // Make popup windows show on the same desktop as the parent full-screen window.
        [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];
        if ((type & Qt::Popup) == Qt::Popup)
            [window setAnimationBehavior:NSWindowAnimationBehaviorUtilityWindow];

        createdWindow = window;
    } else {
        QNSWindow *window;
        window  = [[QNSWindow alloc] initWithContentRect:frame
                                     styleMask: styleMask
                                     qPlatformWindow:this];
        createdWindow = window;
    }

    if ([createdWindow respondsToSelector:@selector(setRestorable:)])
        [createdWindow setRestorable: NO];

    NSInteger level = windowLevel(flags);
    [createdWindow setLevel:level];

    // OpenGL surfaces can be ordered either above(default) or below the NSWindow.
    // When ordering below the window must be tranclucent and have a clear background color.
    static GLint openglSourfaceOrder = qt_mac_resolveOption(1, "QT_MAC_OPENGL_SURFACE_ORDER");

    bool isTranslucent = window()->format().alphaBufferSize() > 0
                         || (surface()->supportsOpenGL() && openglSourfaceOrder == -1);
    if (isTranslucent) {
        [createdWindow setBackgroundColor:[NSColor clearColor]];
        [createdWindow setOpaque:NO];
    }

    m_windowModality = window()->modality();

    applyContentBorderThickness(createdWindow);

    return createdWindow;
}

void QCocoaWindow::setNSWindow(QCocoaNSWindow *window)
{
    if (window.contentView != m_view) {
        [m_view setPostsFrameChangedNotifications:NO];
        [m_view retain];
        if (m_view.superview) // m_view comes from another NSWindow
            [m_view removeFromSuperview];
        [window setContentView:m_view];
        [m_view release];
        [m_view setPostsFrameChangedNotifications:YES];
    }
}

void QCocoaWindow::removeChildWindow(QCocoaWindow *child)
{
    m_childWindows.removeOne(child);
    [m_nsWindow removeChildWindow:child->m_nsWindow];
}

bool QCocoaWindow::isNativeWindowTypeInconsistent()
{
    if (!m_nsWindow)
        return false;

    const bool isPanel = [m_nsWindow isKindOfClass:[QNSPanel class]];
    const bool usePanel = shouldUseNSPanel();

    return isPanel != usePanel;
}

void QCocoaWindow::removeMonitor()
{
    if (!monitor)
        return;
    [NSEvent removeMonitor:monitor];
    monitor = nil;
}

// Returns the current global screen geometry for the nswindow associated with this window.
QRect QCocoaWindow::nativeWindowGeometry() const
{
    if (!m_nsWindow || m_isNSWindowChild)
        return geometry();

    NSRect rect = [m_nsWindow frame];
    QPlatformScreen *onScreen = QPlatformScreen::platformScreenForWindow(window());
    int flippedY = onScreen->geometry().height() - rect.origin.y - rect.size.height;  // account for nswindow inverted y.
    QRect qRect = QRect(rect.origin.x, flippedY, rect.size.width, rect.size.height);
    return qRect;
}

// Returns a pointer to the parent QCocoaWindow for this window, or 0 if there is none.
QCocoaWindow *QCocoaWindow::parentCocoaWindow() const
{
    if (window() && window()->transientParent()) {
        return static_cast<QCocoaWindow*>(window()->transientParent()->handle());
    }
    return 0;
}

// Syncs the NSWindow minimize/maximize/fullscreen state with the current QWindow state
void QCocoaWindow::syncWindowState(Qt::WindowState newState)
{
    if (!m_nsWindow)
        return;
    // if content view width or height is 0 then the window animations will crash so
    // do nothing except set the new state
    NSRect contentRect = m_view.frame;
    if (contentRect.size.width <= 0 || contentRect.size.height <= 0) {
        qWarning("invalid window content view size, check your window geometry");
        m_synchedWindowState = newState;
        return;
    }

    Qt::WindowState predictedState = newState;
    if ((m_synchedWindowState & Qt::WindowMaximized) != (newState & Qt::WindowMaximized)) {
        const int styleMask = [m_nsWindow styleMask];
        const bool usePerform = styleMask & NSResizableWindowMask;
        [m_nsWindow setStyleMask:styleMask | NSResizableWindowMask];
        if (usePerform)
            [m_nsWindow performZoom : m_nsWindow]; // toggles
        else
            [m_nsWindow zoom : m_nsWindow]; // toggles
        [m_nsWindow setStyleMask:styleMask];
    }

    if ((m_synchedWindowState & Qt::WindowMinimized) != (newState & Qt::WindowMinimized)) {
        if (newState & Qt::WindowMinimized) {
            if ([m_nsWindow styleMask] & NSMiniaturizableWindowMask)
                [m_nsWindow performMiniaturize : m_nsWindow];
            else
                [m_nsWindow miniaturize : m_nsWindow];
        } else {
            [m_nsWindow deminiaturize : m_nsWindow];
        }
    }

    const bool effMax = m_effectivelyMaximized;
    if ((m_synchedWindowState & Qt::WindowMaximized) != (newState & Qt::WindowMaximized) || (m_effectivelyMaximized && newState == Qt::WindowNoState)) {
        if ((m_synchedWindowState & Qt::WindowFullScreen) == (newState & Qt::WindowFullScreen)) {
            [m_nsWindow zoom : m_nsWindow]; // toggles
            m_effectivelyMaximized = !effMax;
        } else if (!(newState & Qt::WindowMaximized)) {
            // it would be nice to change the target geometry that toggleFullScreen will animate toward
            // but there is no known way, so the maximized state is not possible at this time
            predictedState = static_cast<Qt::WindowState>(static_cast<int>(newState) | Qt::WindowMaximized);
            m_effectivelyMaximized = true;
        }
    }

    if ((m_synchedWindowState & Qt::WindowFullScreen) != (newState & Qt::WindowFullScreen)) {
        if (window()->flags() & Qt::WindowFullscreenButtonHint) {
            if (m_effectivelyMaximized && m_synchedWindowState == Qt::WindowFullScreen)
                predictedState = Qt::WindowMaximized;
            [m_nsWindow toggleFullScreen : m_nsWindow];
        } else {
            if (newState & Qt::WindowFullScreen) {
                QScreen *screen = window()->screen();
                if (screen) {
                    if (m_normalGeometry.width() < 0) {
                        m_oldWindowFlags = m_windowFlags;
                        window()->setFlags(window()->flags() | Qt::FramelessWindowHint);
                        m_normalGeometry = nativeWindowGeometry();
                        setGeometry(screen->geometry());
                        m_presentationOptions = [NSApp presentationOptions];
                        [NSApp setPresentationOptions : m_presentationOptions | NSApplicationPresentationAutoHideMenuBar | NSApplicationPresentationAutoHideDock];
                    }
                }
            } else {
                window()->setFlags(m_oldWindowFlags);
                setGeometry(m_normalGeometry);
                m_normalGeometry.setRect(0, 0, -1, -1);
                [NSApp setPresentationOptions : m_presentationOptions];
            }
        }
    }

    // New state is now the current synched state
    m_synchedWindowState = predictedState;
}

bool QCocoaWindow::setWindowModified(bool modified)
{
    if (!m_nsWindow)
        return false;
    [m_nsWindow setDocumentEdited:(modified?YES:NO)];
    return true;
}

void QCocoaWindow::setMenubar(QCocoaMenuBar *mb)
{
    m_menubar = mb;
}

QCocoaMenuBar *QCocoaWindow::menubar() const
{
    return m_menubar;
}

// Finds the effective cursor for this window by walking up the
// ancestor chain (including this window) until a set cursor is
// found. Returns nil if there is not set cursor.
NSCursor *QCocoaWindow::effectiveWindowCursor() const
{

    if (m_windowCursor)
        return m_windowCursor;
    if (!QPlatformWindow::parent())
        return nil;
    return static_cast<QCocoaWindow *>(QPlatformWindow::parent())->effectiveWindowCursor();
}

// Applies the cursor as returned by effectiveWindowCursor(), handles
// the special no-cursor-set case by setting the arrow cursor.
void QCocoaWindow::applyEffectiveWindowCursor()
{
    NSCursor *effectiveCursor = effectiveWindowCursor();
    if (effectiveCursor) {
        [effectiveCursor set];
    } else {
        // We wold like to _unset_ the cursor here; but there is no such
        // API. Fall back to setting the default arrow cursor.
        [[NSCursor arrowCursor] set];
    }
}

void QCocoaWindow::setWindowCursor(NSCursor *cursor)
{
    if (m_windowCursor == cursor)
        return;

    // Setting a cursor in a foregin view is not supported.
    if (window()->type() == Qt::ForeignWindow)
        return;

    [m_windowCursor release];
    m_windowCursor = cursor;
    [m_windowCursor retain];

    // The installed view tracking area (see QNSView updateTrackingAreas) will
    // handle cursor updates on mouse enter/leave. Handle the case where the
    // mouse is on the this window by changing the cursor immediately.
    if (m_windowUnderMouse)
        applyEffectiveWindowCursor();
}

void QCocoaWindow::registerTouch(bool enable)
{
    m_registerTouchCount += enable ? 1 : -1;
    if (enable && m_registerTouchCount == 1)
        [m_view setAcceptsTouchEvents:YES];
    else if (m_registerTouchCount == 0)
        [m_view setAcceptsTouchEvents:NO];
}

void QCocoaWindow::setContentBorderThickness(int topThickness, int bottomThickness)
{
    m_topContentBorderThickness = topThickness;
    m_bottomContentBorderThickness = bottomThickness;
    bool enable = (topThickness > 0 || bottomThickness > 0);
    m_drawContentBorderGradient = enable;

    applyContentBorderThickness(m_nsWindow);
}

void QCocoaWindow::registerContentBorderArea(quintptr identifier, int upper, int lower)
{
    m_contentBorderAreas.insert(identifier, BorderRange(identifier, upper, lower));
    applyContentBorderThickness(m_nsWindow);
}

void QCocoaWindow::setContentBorderAreaEnabled(quintptr identifier, bool enable)
{
    m_enabledContentBorderAreas.insert(identifier, enable);
    applyContentBorderThickness(m_nsWindow);
}

void QCocoaWindow::setContentBorderEnabled(bool enable)
{
    m_drawContentBorderGradient = enable;
    applyContentBorderThickness(m_nsWindow);
}

void QCocoaWindow::applyContentBorderThickness(NSWindow *window)
{
    if (!window)
        return;

    if (!m_drawContentBorderGradient) {
        [window setStyleMask:[window styleMask] & ~NSTexturedBackgroundWindowMask];
        [[[window contentView] superview] setNeedsDisplay:YES];
        return;
    }

    // Find consecutive registered border areas, starting from the top.
    QList<BorderRange> ranges = m_contentBorderAreas.values();
    std::sort(ranges.begin(), ranges.end());
    int effectiveTopContentBorderThickness = m_topContentBorderThickness;
    foreach (BorderRange range, ranges) {
        // Skip disiabled ranges (typically hidden tool bars)
        if (!m_enabledContentBorderAreas.value(range.identifier, false))
            continue;

        // Is this sub-range adjacent to or overlaping the
        // existing total border area range? If so merge
        // it into the total range,
        if (range.upper <= (effectiveTopContentBorderThickness + 1))
            effectiveTopContentBorderThickness = qMax(effectiveTopContentBorderThickness, range.lower);
        else
            break;
    }

    int effectiveBottomContentBorderThickness = m_bottomContentBorderThickness;

    [window setStyleMask:[window styleMask] | NSTexturedBackgroundWindowMask];

    [window setContentBorderThickness:effectiveTopContentBorderThickness forEdge:NSMaxYEdge];
    [window setAutorecalculatesContentBorderThickness:NO forEdge:NSMaxYEdge];

    [window setContentBorderThickness:effectiveBottomContentBorderThickness forEdge:NSMinYEdge];
    [window setAutorecalculatesContentBorderThickness:NO forEdge:NSMinYEdge];

    [[[window contentView] superview] setNeedsDisplay:YES];
}

void QCocoaWindow::updateNSToolbar()
{
    if (!m_nsWindow)
        return;

    NSToolbar *toolbar = QCocoaIntegration::instance()->toolbar(window());

    if ([m_nsWindow toolbar] == toolbar)
       return;

    [m_nsWindow setToolbar: toolbar];
    [m_nsWindow setShowsToolbarButton:YES];
}

bool QCocoaWindow::testContentBorderAreaPosition(int position) const
{
    return m_nsWindow && m_drawContentBorderGradient &&
            0 <= position && position < [m_nsWindow contentBorderThicknessForEdge: NSMaxYEdge];
}

qreal QCocoaWindow::devicePixelRatio() const
{
    // The documented way to observe the relationship between device-independent
    // and device pixels is to use one for the convertToBacking functions. Other
    // methods such as [NSWindow backingScaleFacor] might not give the correct
    // result, for example if setWantsBestResolutionOpenGLSurface is not set or
    // or ignored by the OpenGL driver.
    NSSize backingSize = [m_view convertSizeToBacking:NSMakeSize(1.0, 1.0)];
    return backingSize.height;
}

// Returns whether the window can be expose, which it can
// if it is on screen and has a valid geometry.
bool QCocoaWindow::isWindowExposable()
{
    QSize size = geometry().size();
    bool validGeometry = (size.width() > 0 && size.height() > 0);
    bool validScreen = ([[m_view window] screen] != 0);
    bool nonHiddenSuperView = ![[m_view superview] isHidden];
    return (validGeometry && validScreen && nonHiddenSuperView);
}

// Exposes the window by posting an expose event to QWindowSystemInterface
void QCocoaWindow::exposeWindow()
{
    m_geometryUpdateExposeAllowed = true;

    if (!isWindowExposable())
        return;

    // Update the QWindow's screen property. This property is set
    // to QGuiApplication::primaryScreen() at QWindow construciton
    // time, and we won't get a NSWindowDidChangeScreenNotification
    // on show. The case where the window is initially displayed
    // on a non-primary screen needs special handling here.
    NSUInteger screenIndex = [[NSScreen screens] indexOfObject:m_nsWindow.screen];
    if (screenIndex != NSNotFound) {
        QCocoaScreen *cocoaScreen = QCocoaIntegration::instance()->screenAtIndex(screenIndex);
        if (cocoaScreen)
            window()->setScreen(cocoaScreen->screen());
    }

    if (!m_isExposed) {
        m_isExposed = true;
        m_exposedGeometry = geometry();
        m_exposedDevicePixelRatio = devicePixelRatio();
        QRect geometry(QPoint(0, 0), m_exposedGeometry.size());
        qCDebug(lcQpaCocoaWindow) << "QCocoaWindow: exposeWindow" << window() << geometry;
        QWindowSystemInterface::handleExposeEvent(window(), geometry);
    }
}

// Obscures the window by posting an empty expose event to QWindowSystemInterface
void QCocoaWindow::obscureWindow()
{
    if (m_isExposed) {
        m_geometryUpdateExposeAllowed = false;
        m_isExposed = false;

        qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::obscureWindow" << window();
        QWindowSystemInterface::handleExposeEvent(window(), QRegion());
    }
}

// Updates window geometry by posting an expose event to QWindowSystemInterface
void QCocoaWindow::updateExposedGeometry()
{
    // updateExposedGeometry is not allowed to send the initial expose. If you want
    // that call exposeWindow();
    if (!m_geometryUpdateExposeAllowed)
        return;

    // Do not send incorrect exposes in case the window is not even visible yet.
    // We might get here as a result of a resize() from QWidget's show(), for instance.
    if (!window()->isVisible())
        return;

    if (!isWindowExposable())
        return;

    if (m_exposedGeometry.size() == geometry().size() && m_exposedDevicePixelRatio == devicePixelRatio())
        return;

    m_isExposed = true;
    m_exposedGeometry = geometry();
    m_exposedDevicePixelRatio = devicePixelRatio();

    QRect geometry(QPoint(0, 0), m_exposedGeometry.size());
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::updateExposedGeometry" << window() << geometry;
    QWindowSystemInterface::handleExposeEvent(window(), geometry);
}

QWindow *QCocoaWindow::childWindowAt(QPoint windowPoint)
{
    QWindow *targetWindow = window();
    foreach (QObject *child, targetWindow->children())
        if (QWindow *childWindow = qobject_cast<QWindow *>(child))
            if (QPlatformWindow *handle = childWindow->handle())
                if (handle->isExposed() && childWindow->geometry().contains(windowPoint))
                    targetWindow = static_cast<QCocoaWindow*>(handle)->childWindowAt(windowPoint - childWindow->position());

    return targetWindow;
}

bool QCocoaWindow::shouldRefuseKeyWindowAndFirstResponder()
{
    // This function speaks up if there's any reason
    // to refuse key window or first responder state.

    if (window()->flags() & Qt::WindowDoesNotAcceptFocus)
        return true;

    if (m_inSetVisible) {
        QVariant showWithoutActivating = window()->property("_q_showWithoutActivating");
        if (showWithoutActivating.isValid() && showWithoutActivating.toBool())
            return true;
    }

    return false;
}

QPoint QCocoaWindow::bottomLeftClippedByNSWindowOffsetStatic(QWindow *window)
{
    if (window->handle())
        return static_cast<QCocoaWindow *>(window->handle())->bottomLeftClippedByNSWindowOffset();
    return QPoint();
}

QPoint QCocoaWindow::bottomLeftClippedByNSWindowOffset() const
{
    if (!m_view)
        return QPoint();
    const NSPoint origin = [m_view isFlipped] ? NSMakePoint(0, [m_view frame].size.height)
                                                     : NSMakePoint(0,                                 0);
    const NSRect visibleRect = [m_view visibleRect];

    return QPoint(visibleRect.origin.x, -visibleRect.origin.y + (origin.y - visibleRect.size.height));
}

QMargins QCocoaWindow::frameMargins() const
{
    NSRect frameW = [m_nsWindow frame];
    NSRect frameC = [m_nsWindow contentRectForFrameRect:frameW];

    return QMargins(frameW.origin.x - frameC.origin.x,
        (frameW.origin.y + frameW.size.height) - (frameC.origin.y + frameC.size.height),
        (frameW.origin.x + frameW.size.width) - (frameC.origin.x + frameC.size.width),
        frameC.origin.y - frameW.origin.y);
}

void QCocoaWindow::setFrameStrutEventsEnabled(bool enabled)
{
    m_frameStrutEventsEnabled = enabled;
}

#include "moc_qcocoawindow.cpp"
