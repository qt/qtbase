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

#include <vector>

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

@interface NSWindow (FullScreenProperty)
@property(readonly) BOOL qt_fullScreen;
@end

@implementation NSWindow (FullScreenProperty)

+ (void)load
{
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserverForName:NSWindowDidEnterFullScreenNotification object:nil queue:nil
        usingBlock:^(NSNotification *notification) {
            objc_setAssociatedObject(notification.object, @selector(qt_fullScreen),
                [NSNumber numberWithBool:YES], OBJC_ASSOCIATION_RETAIN);
        }
    ];
    [center addObserverForName:NSWindowDidExitFullScreenNotification object:nil queue:nil
        usingBlock:^(NSNotification *notification) {
            objc_setAssociatedObject(notification.object, @selector(qt_fullScreen),
                nil, OBJC_ASSOCIATION_RETAIN);
        }
    ];
}

- (BOOL)qt_fullScreen
{
    NSNumber *number = objc_getAssociatedObject(self, @selector(qt_fullScreen));
    return [number boolValue];
}
@end

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
        // make sure that NSWindow stays valid until the
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
        if (pw->window()->isTopLevel() && theEvent.type == NSLeftMouseDown) {
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

    if (!self.platformWindow)
        return; // Platform window went away while processing event

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
      screen:(NSScreen*)screen
      styleMask:(NSUInteger)windowStyle
      qPlatformWindow:(QCocoaWindow *)qpw
{
    self = [super initWithContentRect:contentRect
            styleMask:windowStyle
            backing:NSBackingStoreBuffered
            defer:NO screen:screen]; // Deferring window creation breaks OpenGL (the GL context is
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
    if (!pw || !pw->window()->isTopLevel())
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
    if (!pw || !pw->window()->isTopLevel() || pw->window()->transientParent())
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
    qCDebug(lcQpaCocoaWindow) << "closeAndRelease" << self;

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

+ (void)applicationActivationChanged:(NSNotification*)notification
{
    const id sender = self;
    NSEnumerator<NSWindow*> *windowEnumerator = nullptr;
    NSApplication *application = [NSApplication sharedApplication];

#if QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_12)
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSSierra) {
        // Unfortunately there's no NSWindowListOrderedBackToFront,
        // so we have to manually reverse the order using an array.
        NSMutableArray *windows = [[[NSMutableArray alloc] init] autorelease];
        [application enumerateWindowsWithOptions:NSWindowListOrderedFrontToBack
            usingBlock:^(NSWindow *window, BOOL *) {
                // For some reason AppKit will give us nil-windows, skip those
                if (!window)
                    return;

                [(NSMutableArray*)windows addObject:window];
            }
        ];

        windowEnumerator = windows.reverseObjectEnumerator;
    } else
#endif
    {
        // No way to get ordered list of windows, so fall back to unordered,
        // list, which typically corresponds to window creation order.
        windowEnumerator = application.windows.objectEnumerator;
    }

    for (NSWindow *window in windowEnumerator) {
        // We're meddling with normal and floating windows, so leave others alone
        if (!(window.level == NSNormalWindowLevel || window.level == NSFloatingWindowLevel))
            continue;

        // Windows that hide automatically will keep their NSFloatingWindowLevel,
        // and hence be on top of the window stack. We don't want to affect these
        // windows, as otherwise we might end up with key windows being ordered
        // behind these auto-hidden windows when activating the application by
        // clicking on a new tool window.
        if (window.hidesOnDeactivate)
            continue;

        if ([window conformsToProtocol:@protocol(QNSWindowProtocol)]) {
            QCocoaWindow *cocoaWindow = static_cast<id<QNSWindowProtocol>>(window).helper.platformWindow;
            window.level = notification.name == NSApplicationWillResignActiveNotification ?
                NSNormalWindowLevel : cocoaWindow->windowLevel(cocoaWindow->window()->flags());
        }

        // The documentation says that "when a window enters a new level, it’s ordered
        // in front of all its peers in that level", but that doesn't seem to be the
        // case in practice. To keep the order correct after meddling with the window
        // levels, we explicitly order each window to the front. Since we are iterating
        // the windows in back-to-front order, this is okey. The call also triggers AppKit
        // to re-evaluate the level in relation to windows from other applications,
        // working around an issue where our tool windows would stay on top of other
        // application windows if activation was transferred to another application by
        // clicking on it instead of via the application switcher or Dock. Finally, we
        // do this re-ordering for all windows (except auto-hiding ones), otherwise we would
        // end up triggering a bug in AppKit where the tool windows would disappear behind
        // the application window.
        [window orderFront:sender];
    }
}

- (id)initWithContentRect:(NSRect)contentRect
      screen:(NSScreen*)screen
      styleMask:(NSUInteger)windowStyle
      qPlatformWindow:(QCocoaWindow *)qpw
{
    self = [super initWithContentRect:contentRect
            styleMask:windowStyle
            backing:NSBackingStoreBuffered
            defer:NO screen:screen]; // Deferring window creation breaks OpenGL (the GL context is
                       // set up before the window is shown and needs a proper window)

    if (self) {
        _helper = [[QNSWindowHelper alloc] initWithNSWindow:self platformWindow:qpw];

        if (qpw->alwaysShowToolWindow()) {
            static dispatch_once_t onceToken;
            dispatch_once(&onceToken, ^{
                NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
                [center addObserver:[self class] selector:@selector(applicationActivationChanged:)
                    name:NSApplicationWillResignActiveNotification object:nil];
                [center addObserver:[self class] selector:@selector(applicationActivationChanged:)
                    name:NSApplicationWillBecomeActiveNotification object:nil];
            });
        }
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
    qCDebug(lcQpaCocoaWindow) << "closeAndRelease" << self;

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

static void qRegisterNotificationCallbacks()
{
    static const QLatin1String notificationHandlerPrefix(Q_NOTIFICATION_PREFIX);

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];

    const QMetaObject *metaObject = QMetaType::metaObjectForType(qRegisterMetaType<QCocoaWindow*>());
    Q_ASSERT(metaObject);

    for (int i = 0; i < metaObject->methodCount(); ++i) {
        QMetaMethod method = metaObject->method(i);
        const QString methodTag = QString::fromLatin1(method.tag());
        if (!methodTag.startsWith(notificationHandlerPrefix))
            continue;

        const QString notificationName = methodTag.mid(notificationHandlerPrefix.size());
        [center addObserverForName:notificationName.toNSString() object:nil queue:nil
            usingBlock:^(NSNotification *notification) {

            NSView *view = nullptr;
            if ([notification.object isKindOfClass:[NSWindow class]]) {
                NSWindow *window = notification.object;
                // Only top level NSWindows should notify their QNSViews
                if (window.parentWindow)
                    return;

                if (!window.contentView)
                    return;

                view = window.contentView;
            } else if ([notification.object isKindOfClass:[NSView class]]) {
                view = notification.object;
            } else {
                qCWarning(lcQpaCocoaWindow) << "Unhandled notifcation"
                    << notification.name << "for" << notification.object;
                return;
            }
            Q_ASSERT(view);

            QCocoaWindow *cocoaWindow = nullptr;
            if (QNSView *qnsView = qnsview_cast(view))
                cocoaWindow = qnsView.platformWindow;

            // FIXME: Could be a foreign window, look up by iterating top level QWindows

            if (!cocoaWindow)
                return;

            if (!method.invoke(cocoaWindow, Qt::DirectConnection)) {
                qCWarning(lcQpaCocoaWindow) << "Failed to invoke NSNotification callback for"
                    << notification.name << "on" << cocoaWindow;
            }
        }];
    }
}
Q_CONSTRUCTOR_FUNCTION(qRegisterNotificationCallbacks)

const int QCocoaWindow::NoAlertRequest = -1;

QCocoaWindow::QCocoaWindow(QWindow *tlw, WId nativeHandle)
    : QPlatformWindow(tlw)
    , m_view(nil)
    , m_nsWindow(0)
    , m_viewIsEmbedded(false)
    , m_viewIsToBeEmbedded(false)
    , m_lastReportedWindowState(Qt::WindowNoState)
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
    , m_hasWindowFilePath(false)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::QCocoaWindow" << window();

    QMacAutoReleasePool pool;

    if (nativeHandle) {
        m_view = reinterpret_cast<NSView *>(nativeHandle);
        [m_view retain];
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

    setGeometry(initialGeometry(window(), windowGeometry(), defaultWindowWidth, defaultWindowHeight));

    recreateWindowIfNeeded();
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
    if (m_view.window.parentWindow)
        [m_view.window.parentWindow removeChildWindow:m_view.window];
    else if ([m_view superview])
        [m_view removeFromSuperview];

    removeMonitor();

    // Make sure to disconnect observer in all case if view is valid
    // to avoid notifications received when deleting when using Qt::AA_NativeWindows attribute
    if (!isForeignWindow())
        [[NSNotificationCenter defaultCenter] removeObserver:m_view];

    // While it is unlikely that this window will be in the popup stack
    // during deletetion we clear any pointers here to make sure.
    if (QCocoaIntegration::instance()) {
        QCocoaIntegration::instance()->popupWindowStack()->removeAll(this);
    }

    foreachChildNSWindow(^(QCocoaWindow *childWindow) {
        [m_view.window removeChildWindow:childWindow->nativeWindow()];
    });

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

bool QCocoaWindow::isForeignWindow() const
{
    return ![m_view isKindOfClass:[QNSView class]];
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
        QSize size = QRectF::fromCGRect(NSRectToCGRect([m_view bounds])).toRect().size();
        return QRect(position, size);
    }

    return QPlatformWindow::geometry();
}

void QCocoaWindow::setCocoaGeometry(const QRect &rect)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setCocoaGeometry" << window() << rect;
    QMacAutoReleasePool pool;

    if (m_viewIsEmbedded) {
        if (!isForeignWindow()) {
            [m_view setFrame:NSMakeRect(0, 0, rect.width(), rect.height())];
        } else {
            QPlatformWindow::setGeometry(rect);
        }
        return;
    }

    if (isChildNSWindow()) {
        QPlatformWindow::setGeometry(rect);
        NSWindow *parentNSWindow = m_view.window.parentWindow;
        NSRect parentWindowFrame = [parentNSWindow contentRectForFrameRect:parentNSWindow.frame];
        clipWindow(parentWindowFrame);

        // call this here: updateGeometry in qnsview.mm is a no-op for this case
        QWindowSystemInterface::handleGeometryChange(window(), rect);
        QWindowSystemInterface::handleExposeEvent(window(), QRect(QPoint(0, 0), rect.size()));
    } else if (isContentView()) {
        NSRect bounds = qt_mac_flipRect(rect);
        [m_view.window setFrame:[m_view.window frameRectForContentRect:bounds] display:YES animate:NO];
    } else {
        [m_view setFrame:NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height())];
    }

    if (isForeignWindow())
        QPlatformWindow::setGeometry(rect);

    // will call QPlatformWindow::setGeometry(rect) during resize confirmation (see qnsview.mm)
}

void QCocoaWindow::clipChildWindows()
{
    foreachChildNSWindow(^(QCocoaWindow *childWindow) {
        childWindow->clipWindow(m_view.window.frame);
    });
}

void QCocoaWindow::clipWindow(const NSRect &clipRect)
{
    if (!isChildNSWindow())
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
            [m_view.window orderOut:nil];
            m_hiddenByClipping = true;
        }
    } else {
        [m_view.window setFrame:clippedWindowRect display:YES animate:NO];
        if (m_hiddenByClipping) {
            m_hiddenByClipping = false;
            if (!m_hiddenByAncestor) {
                [m_view.window orderFront:nil];
                static_cast<QCocoaWindow *>(QPlatformWindow::parent())->reinsertChildWindow(this);
            }
        }
    }

    // recurse
    foreachChildNSWindow(^(QCocoaWindow *childWindow) {
        childWindow->clipWindow(clippedWindowRect);
    });
}

void QCocoaWindow::hide(bool becauseOfAncestor)
{
    Q_ASSERT(isContentView());

    bool visible = m_view.window.visible;

    if (!m_hiddenByAncestor && !visible) // Already explicitly hidden
        return;
    if (m_hiddenByAncestor && becauseOfAncestor) // Trying to hide some child again
        return;

    m_hiddenByAncestor = becauseOfAncestor;

    if (!visible) // Could have been clipped before
        return;

    foreachChildNSWindow(^(QCocoaWindow *childWindow) {
        childWindow->hide(true);
    });

    [m_view.window orderOut:nil];
}

void QCocoaWindow::show(bool becauseOfAncestor)
{
    Q_ASSERT(isContentView());

    if (m_view.window.visible)
        return;

    if (m_view.window.parentWindow && !m_view.window.parentWindow.visible) {
        m_hiddenByAncestor = true; // Parent still hidden, don't show now
    } else if ((becauseOfAncestor == m_hiddenByAncestor) // Was NEITHER explicitly hidden
               && !m_hiddenByClipping) { // ... NOR clipped
        if (isChildNSWindow()) {
            m_hiddenByAncestor = false;
            setCocoaGeometry(windowGeometry());
        }
        if (!m_hiddenByClipping) { // setCocoaGeometry() can change the clipping status
            [m_view.window orderFront:nil];
            if (isChildNSWindow())
                static_cast<QCocoaWindow *>(QPlatformWindow::parent())->reinsertChildWindow(this);
            foreachChildNSWindow(^(QCocoaWindow *childWindow) {
                childWindow->show(true);
            });
        }
    }
}

void QCocoaWindow::setVisible(bool visible)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setVisible" << window() << visible;

    if (isChildNSWindow() && m_hiddenByClipping)
        return;

    m_inSetVisible = true;

    QMacAutoReleasePool pool;
    QCocoaWindow *parentCocoaWindow = 0;
    if (window()->transientParent())
        parentCocoaWindow = static_cast<QCocoaWindow *>(window()->transientParent()->handle());

    if (visible) {
        // We need to recreate if the modality has changed as the style mask will need updating
        recreateWindowIfNeeded();

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
                NSWindow *nativeParentWindow = parentCocoaWindow->nativeWindow();
                NSUInteger parentStyleMask = nativeParentWindow.styleMask;
                if ((m_resizableTransientParent = (parentStyleMask & NSResizableWindowMask))
                    && !(nativeParentWindow.styleMask & NSFullScreenWindowMask))
                    nativeParentWindow.styleMask &= ~NSResizableWindowMask;
            }

        }

        // This call is here to handle initial window show correctly:
        // - top-level windows need to have backing store content ready when the
        //   window is shown, sendin the expose event here makes that more likely.
        // - QNSViews for child windows are initialy not hidden and won't get the
        //   viewDidUnhide message.
        exposeWindow();

        if (isContentView()) {
            QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);

            // setWindowState might have been called while the window was hidden and
            // will not change the NSWindow state in that case. Sync up here:
            applyWindowState(window()->windowStates());

            if (window()->windowState() != Qt::WindowMinimized) {
                if ((window()->modality() == Qt::WindowModal
                     || window()->type() == Qt::Sheet)
                        && parentCocoaWindow) {
                    // show the window as a sheet
                    [parentCocoaWindow->nativeWindow() beginSheet:m_view.window completionHandler:nil];
                } else if (window()->modality() != Qt::NonModal) {
                    // show the window as application modal
                    QCocoaEventDispatcher *cocoaEventDispatcher = qobject_cast<QCocoaEventDispatcher *>(QGuiApplication::instance()->eventDispatcher());
                    Q_ASSERT(cocoaEventDispatcher != 0);
                    QCocoaEventDispatcherPrivate *cocoaEventDispatcherPrivate = static_cast<QCocoaEventDispatcherPrivate *>(QObjectPrivate::get(cocoaEventDispatcher));
                    cocoaEventDispatcherPrivate->beginModalSession(window());
                    m_hasModalSession = true;
                } else if ([m_view.window canBecomeKeyWindow]) {
                    QCocoaEventDispatcher *cocoaEventDispatcher = qobject_cast<QCocoaEventDispatcher *>(QGuiApplication::instance()->eventDispatcher());
                    QCocoaEventDispatcherPrivate *cocoaEventDispatcherPrivate = 0;
                    if (cocoaEventDispatcher)
                        cocoaEventDispatcherPrivate = static_cast<QCocoaEventDispatcherPrivate *>(QObjectPrivate::get(cocoaEventDispatcher));

                    if (cocoaEventDispatcherPrivate && cocoaEventDispatcherPrivate->cocoaModalSessionStack.isEmpty())
                        [m_view.window makeKeyAndOrderFront:nil];
                    else
                        [m_view.window orderFront:nil];

                    foreachChildNSWindow(^(QCocoaWindow *childWindow) {
                        childWindow->show(true);
                    });
                } else {
                    show();
                }

                // We want the events to properly reach the popup, dialog, and tool
                if ((window()->type() == Qt::Popup || window()->type() == Qt::Dialog || window()->type() == Qt::Tool)
                    && [m_view.window isKindOfClass:[NSPanel class]]) {
                    ((NSPanel *)m_view.window).worksWhenModal = YES;
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
        if (isContentView()) {
            if (m_hasModalSession) {
                if (cocoaEventDispatcherPrivate)
                    cocoaEventDispatcherPrivate->endModalSession(window());
                m_hasModalSession = false;
            } else {
                if ([m_view.window isSheet]) {
                    Q_ASSERT_X(parentCocoaWindow, "QCocoaWindow", "Window modal dialog has no transient parent.");
                    [parentCocoaWindow->nativeWindow() endSheet:m_view.window];
                }
            }

            hide();
            if (m_view.window == [NSApp keyWindow]
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
            NSWindow *nativeParentWindow = parentCocoaWindow->nativeWindow();
            if (m_resizableTransientParent
                && !(nativeParentWindow.styleMask & NSFullScreenWindowMask))
                // A window should not be resizable while a transient popup is open
                nativeParentWindow.styleMask |= NSResizableWindowMask;
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
            windowLevel = qMax([transientParentWindow->nativeWindow() level], windowLevel);
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

    // Don't wipe fullscreen state
    if (m_view.window.styleMask & NSFullScreenWindowMask)
        styleMask |= NSFullScreenWindowMask;

    return styleMask;
}

void QCocoaWindow::setWindowShadow(Qt::WindowFlags flags)
{
    if (!isContentView())
        return;

    bool keepShadow = !(flags & Qt::NoDropShadowWindowHint);
    m_view.window.hasShadow = keepShadow;
}

void QCocoaWindow::setWindowZoomButton(Qt::WindowFlags flags)
{
    if (!isContentView())
        return;

    // Disable the zoom (maximize) button for fixed-sized windows and customized
    // no-WindowMaximizeButtonHint windows. From a Qt perspective it migth be expected
    // that the button would be removed in the latter case, but disabling it is more
    // in line with the platform style guidelines.
    bool fixedSizeNoZoom = (windowMinimumSize().isValid() && windowMaximumSize().isValid()
                            && windowMinimumSize() == windowMaximumSize());
    bool customizeNoZoom = ((flags & Qt::CustomizeWindowHint)
        && !(flags & (Qt::WindowMaximizeButtonHint | Qt::WindowFullscreenButtonHint)));
    [[m_view.window standardWindowButton:NSWindowZoomButton] setEnabled:!(fixedSizeNoZoom || customizeNoZoom)];
}

void QCocoaWindow::setWindowFlags(Qt::WindowFlags flags)
{
    if (isContentView() && !isChildNSWindow()) {
        // While setting style mask we can have -updateGeometry calls on a content
        // view with null geometry, reporting an invalid coordinates as a result.
        m_inSetStyleMask = true;
        m_view.window.styleMask = windowStyleMask(flags);
        m_inSetStyleMask = false;
        m_view.window.level = this->windowLevel(flags);
        setWindowShadow(flags);
        if (!(flags & Qt::FramelessWindowHint))
            setWindowTitle(window()->title());

        Qt::WindowType type = window()->type();
        if ((type & Qt::Popup) != Qt::Popup && (type & Qt::Dialog) != Qt::Dialog) {
            NSWindowCollectionBehavior behavior = m_view.window.collectionBehavior;
            if (flags & Qt::WindowFullscreenButtonHint) {
                behavior |= NSWindowCollectionBehaviorFullScreenPrimary;
                behavior &= ~NSWindowCollectionBehaviorFullScreenAuxiliary;
            } else {
                behavior |= NSWindowCollectionBehaviorFullScreenAuxiliary;
                behavior &= ~NSWindowCollectionBehaviorFullScreenPrimary;
            }
            m_view.window.collectionBehavior = behavior;
        }
        setWindowZoomButton(flags);
    }

    if (isContentView())
        m_view.window.ignoresMouseEvents = flags & Qt::WindowTransparentForInput;

    m_windowFlags = flags;
}

void QCocoaWindow::setWindowState(Qt::WindowStates state)
{
    if (window()->isVisible())
        applyWindowState(state); // Window state set for hidden windows take effect when show() is called
}

void QCocoaWindow::setWindowTitle(const QString &title)
{
    if (!isContentView())
        return;

    QMacAutoReleasePool pool;
    m_view.window.title = title.toNSString();
}

void QCocoaWindow::setWindowFilePath(const QString &filePath)
{
    if (!isContentView())
        return;

    QMacAutoReleasePool pool;
    QFileInfo fi(filePath);
    [m_view.window setRepresentedFilename:fi.exists() ? filePath.toNSString() : @""];
    m_hasWindowFilePath = fi.exists();
}

void QCocoaWindow::setWindowIcon(const QIcon &icon)
{
    if (!isContentView())
        return;

    QMacAutoReleasePool pool;

    NSButton *iconButton = [m_view.window standardWindowButton:NSWindowDocumentIconButton];
    if (iconButton == nil) {
        if (icon.isNull())
            return;
        NSString *title = window()->title().toNSString();
        [m_view.window setRepresentedURL:[NSURL fileURLWithPath:title]];
        iconButton = [m_view.window standardWindowButton:NSWindowDocumentIconButton];
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
    if (!isContentView())
        return;

    if (isChildNSWindow() && m_hiddenByClipping)
        return;

    if (m_view.window.visible) {
        if (isChildNSWindow()) {
            // -[NSWindow orderFront:] doesn't work with attached windows.
            // The only solution is to remove and add the child window.
            // This will place it on top of all the other NSWindows.
            NSWindow *parentNSWindow = m_view.window.parentWindow;
            [parentNSWindow removeChildWindow:m_view.window];
            [parentNSWindow addChildWindow:m_view.window ordered:NSWindowAbove];
        } else {
            {
                // Clean up autoreleased temp objects from orderFront immediately.
                // Failure to do so has been observed to cause leaks also beyond any outer
                // autorelease pool (for example around a complete QWindow
                // construct-show-raise-hide-delete cyle), counter to expected autoreleasepool
                // behavior.
                QMacAutoReleasePool pool;
                [m_view.window orderFront:m_view.window];
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
    if (!isContentView())
        return;

    if (isChildNSWindow() && m_hiddenByClipping)
        return;

    if (m_view.window.visible) {
        if (isChildNSWindow()) {
            // -[NSWindow orderBack:] doesn't work with attached windows.
            // The only solution is to remove and add all the child windows except this one.
            // This will keep the current window at the bottom while adding the others on top of it,
            // hopefully in the same order (this is not documented anywhere in the Cocoa documentation).
            NSWindow *parentNSWindow = m_view.window.parentWindow;
            NSArray *children = [parentNSWindow.childWindows copy];
            for (NSWindow *child in children)
                if (m_view.window != child) {
                    [parentNSWindow removeChildWindow:child];
                    [parentNSWindow addChildWindow:child ordered:NSWindowAbove];
                }
        } else {
            [m_view.window orderBack:m_view.window];
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
    if (!isContentView())
        return;

    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::propagateSizeHints" << window() << "\n"
                              << "       min/max" << windowMinimumSize() << windowMaximumSize()
                              << "size increment" << windowSizeIncrement()
                              << "      basesize" << windowBaseSize()
                              << "      geometry" << windowGeometry();

    const NSWindow *window = m_view.window;

    // Set the minimum content size.
    QSize minimumSize = windowMinimumSize();
    if (!minimumSize.isValid()) // minimumSize is (-1, -1) when not set. Make that (0, 0) for Cocoa.
        minimumSize = QSize(0, 0);
    window.contentMinSize = NSSizeFromCGSize(minimumSize.toCGSize());

    // Set the maximum content size.
    window.contentMaxSize = NSSizeFromCGSize(windowMaximumSize().toCGSize());

    // The window may end up with a fixed size; in this case the zoom button should be disabled.
    setWindowZoomButton(m_windowFlags);

    // sizeIncrement is observed to take values of (-1, -1) and (0, 0) for windows that should be
    // resizable and that have no specific size increment set. Cocoa expects (1.0, 1.0) in this case.
    QSize sizeIncrement = windowSizeIncrement();
    if (sizeIncrement.isEmpty())
        sizeIncrement = QSize(1, 1);
    window.resizeIncrements = NSSizeFromCGSize(sizeIncrement.toCGSize());

    QRect rect = geometry();
    QSize baseSize = windowBaseSize();
    if (!baseSize.isNull() && baseSize.isValid())
        [window setFrame:NSMakeRect(rect.x(), rect.y(), baseSize.width(), baseSize.height()) display:YES];
}

void QCocoaWindow::setOpacity(qreal level)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setOpacity" << level;
    if (!isContentView())
        return;

    m_view.window.alphaValue = level;
    m_view.window.opaque = isOpaque();
}

void QCocoaWindow::setMask(const QRegion &region)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setMask" << window() << region;
    if (isContentView())
        m_view.window.backgroundColor = [NSColor clearColor];

    [qnsview_cast(m_view) setMaskRegion:&region];
    m_view.window.opaque = isOpaque();
}

bool QCocoaWindow::setKeyboardGrabEnabled(bool grab)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setKeyboardGrabEnabled" << window() << grab;
    if (!isContentView())
        return false;

    if (grab && ![m_view.window isKeyWindow])
        [m_view.window makeKeyWindow];

    return true;
}

bool QCocoaWindow::setMouseGrabEnabled(bool grab)
{
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::setMouseGrabEnabled" << window() << grab;
    if (!isContentView())
        return false;

    if (grab && ![m_view.window isKeyWindow])
        [m_view.window makeKeyWindow];

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
    recreateWindowIfNeeded();
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
    return m_view.window;
}

void QCocoaWindow::setEmbeddedInForeignView(bool embedded)
{
    m_viewIsToBeEmbedded = embedded;
    // Release any previosly created NSWindow.
    [m_nsWindow closeAndRelease];
    m_nsWindow = 0;
}

// ----------------------- NSWindow notifications -----------------------

void QCocoaWindow::windowWillMove()
{
    // Close any open popups on window move
    qt_closePopups();
}

void QCocoaWindow::windowDidMove()
{
    if (isChildNSWindow())
        return;

    [qnsview_cast(m_view) updateGeometry];

    // Moving a window might bring it out of maximized state
    reportCurrentWindowState();
}

void QCocoaWindow::windowDidResize()
{
    if (!isContentView())
        return;

    if (isChildNSWindow())
        return;

    clipChildWindows();
    [qnsview_cast(m_view) updateGeometry];

    if (!m_view.inLiveResize)
        reportCurrentWindowState();
}

void QCocoaWindow::viewDidChangeFrame()
{
    [qnsview_cast(m_view) updateGeometry];
}

/*!
    Callback for NSViewGlobalFrameDidChangeNotification.

    Posted whenever an NSView object that has attached surfaces (that is,
    NSOpenGLContext objects) moves to a different screen, or other cases
    where the NSOpenGLContext object needs to be updated.
*/
void QCocoaWindow::viewDidChangeGlobalFrame()
{
    updateExposedGeometry();
}

void QCocoaWindow::windowDidEndLiveResize()
{
    reportCurrentWindowState();
}

void QCocoaWindow::windowDidBecomeKey()
{
    if (isForeignWindow())
        return;

    if (m_windowUnderMouse) {
        QPointF windowPoint;
        QPointF screenPoint;
        [qnsview_cast(m_view) convertFromScreen:[NSEvent mouseLocation] toWindowPoint:&windowPoint andScreenPoint:&screenPoint];
        QWindowSystemInterface::handleEnterEvent(m_enterLeaveTargetWindow, windowPoint, screenPoint);
    }

    if (!windowIsPopupType() && !qnsview_cast(m_view).isMenuView)
        QWindowSystemInterface::handleWindowActivated(window());
}

void QCocoaWindow::windowDidResignKey()
{
    if (isForeignWindow())
        return;

    // Key window will be non-nil if another window became key, so do not
    // set the active window to zero here -- the new key window's
    // NSWindowDidBecomeKeyNotification hander will change the active window.
    NSWindow *keyWindow = [NSApp keyWindow];
    if (!keyWindow || keyWindow == m_view.window) {
        // No new key window, go ahead and set the active window to zero
        if (!windowIsPopupType() && !qnsview_cast(m_view).isMenuView)
            QWindowSystemInterface::handleWindowActivated(0);
    }
}

void QCocoaWindow::windowDidMiniaturize()
{
    reportCurrentWindowState();
}

void QCocoaWindow::windowDidDeminiaturize()
{
    reportCurrentWindowState();
}

void QCocoaWindow::windowWillEnterFullScreen()
{
    // The NSWindow needs to be resizable, otherwise we'll end up with
    // the normal window geometry, centered in the middle of the screen
    // on a black background. The styleMask will be reset below.
    m_view.window.styleMask |= NSResizableWindowMask;
}

void QCocoaWindow::windowDidEnterFullScreen()
{
    Q_ASSERT_X(m_view.window.qt_fullScreen, "QCocoaWindow",
        "FullScreen category processes window notifications first");

    // Reset to original styleMask
    setWindowFlags(m_windowFlags);

    reportCurrentWindowState();
}

void QCocoaWindow::windowWillExitFullScreen()
{
    // The NSWindow needs to be resizable, otherwise we'll end up with
    // a weird zoom animation. The styleMask will be reset below.
    m_view.window.styleMask |= NSResizableWindowMask;
}

void QCocoaWindow::windowDidExitFullScreen()
{
    Q_ASSERT_X(!m_view.window.qt_fullScreen, "QCocoaWindow",
        "FullScreen category processes window notifications first");

    // Reset to original styleMask
    setWindowFlags(m_windowFlags);

    Qt::WindowState requestedState = window()->windowState();

    // Deliver update of QWindow state
    reportCurrentWindowState();

    if (requestedState != windowState() && requestedState != Qt::WindowFullScreen) {
        // We were only going out of full screen as an intermediate step before
        // progressing into the final step, so re-sync the desired state.
       applyWindowState(requestedState);
    }
}

void QCocoaWindow::windowDidOrderOffScreen()
{
    obscureWindow();
}

void QCocoaWindow::windowDidOrderOnScreen()
{
    exposeWindow();
}

void QCocoaWindow::windowDidChangeOcclusionState()
{
    // Several unit tests expect paint and/or expose events for windows that are
    // sometimes (unpredictably) occluded and some unit tests depend on QWindow::isExposed.
    // Don't send Expose/Obscure events when running under QTestLib.
    static const bool onTestLib = qt_mac_resolveOption(false, "QT_QTESTLIB_RUNNING");
    if (!onTestLib) {
        if ((NSUInteger)[m_view.window occlusionState] & NSWindowOcclusionStateVisible) {
            exposeWindow();
        } else {
            // Send Obscure events on window occlusion to stop animations.
            obscureWindow();
        }
    }
}

void QCocoaWindow::windowDidChangeScreen()
{
    if (!window())
        return;

    if (QCocoaScreen *cocoaScreen = QCocoaIntegration::instance()->screenForNSScreen(m_view.window.screen))
        QWindowSystemInterface::handleWindowScreenChanged(window(), cocoaScreen->screen());

    updateExposedGeometry();
}

void QCocoaWindow::windowWillClose()
{
    // Close any open popups on window closing.
    if (window() && !windowIsPopupType(window()->type()))
        qt_closePopups();
}

// ----------------------- NSWindowDelegate callbacks -----------------------

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

// --------------------------------------------------------------------------

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

/*!
    Checks if the window is a non-top level QWindow with a NSWindow.

    \sa _q_platform_MacUseNSWindow, QT_MAC_USE_NSWINDOW
*/
bool QCocoaWindow::isChildNSWindow() const
{
    return m_view.window.parentWindow != nil;
}

/*!
    Checks if the window is the content view of its immediate NSWindow.

    Being the content view of a NSWindow means the QWindow is
    the highest accessible NSView object in the window's view
    hierarchy.

    This can only happen in two cases, either if the QWindow is
    itself a top level window, or if it's a child NSWindow.

    \sa isChildNSWindow
*/
bool QCocoaWindow::isContentView() const
{
    return m_view.window.contentView == m_view;
}

/*!
    Iterates child NSWindows that have a corresponding QCocoaWindow.
*/
void QCocoaWindow::foreachChildNSWindow(void (^block)(QCocoaWindow *))
{
    NSArray *windows = m_view.window.childWindows;
    [windows enumerateObjectsUsingBlock:^(NSWindow *window, NSUInteger index, BOOL *stop) {
        Q_UNUSED(index);
        Q_UNUSED(stop);
        if (QNSView *view = qnsview_cast(window.contentView))
            block(view.platformWindow);
    }];
}

/*!
    Recreates (or removes) the NSWindow for this QWindow, if needed.

    A QWindow may need a corresponding NSWindow, depending on whether
    or not it's a top level or not (or explicitly set to be a child
    NSWindow), whether it is a NSPanel or not, etc.
*/
void QCocoaWindow::recreateWindowIfNeeded()
{
    QMacAutoReleasePool pool;

    QPlatformWindow *parentWindow = QPlatformWindow::parent();
    qCDebug(lcQpaCocoaWindow) << "QCocoaWindow::recreateWindowIfNeeded" << window()
                              << "parent" << (parentWindow ? parentWindow->window() : 0);

    RecreationReasons recreateReason = RecreationNotNeeded;

    QCocoaWindow *oldParentCocoaWindow = nullptr;
    if (QNSView *qnsView = qnsview_cast(m_view.superview))
        oldParentCocoaWindow = qnsView.platformWindow;

    if (parentWindow != oldParentCocoaWindow)
         recreateReason |= ParentChanged;

    if (!m_view.window)
        recreateReason |= MissingWindow;

    // If the modality has changed the style mask will need updating
    if (m_windowModality != window()->modality())
        recreateReason |= WindowModalityChanged;

    const bool shouldBeChildNSWindow = parentWindow && qt_mac_resolveOption(NO,
        window(), "_q_platform_MacUseNSWindow", "QT_MAC_USE_NSWINDOW");

    if (isChildNSWindow() != shouldBeChildNSWindow)
        recreateReason |= ChildNSWindowChanged;

    const bool shouldBeContentView = (!parentWindow && !m_viewIsEmbedded) || shouldBeChildNSWindow;
    if (isContentView() != shouldBeContentView)
        recreateReason |= ContentViewChanged;

    Qt::WindowType type = window()->type();
    const bool isPanel = isContentView() && [m_view.window isKindOfClass:[QNSPanel class]];
    const bool shouldBePanel = shouldBeContentView && !shouldBeChildNSWindow &&
        ((type & Qt::Popup) == Qt::Popup || (type & Qt::Dialog) == Qt::Dialog);

    if (isPanel != shouldBePanel)
         recreateReason |= PanelChanged;

    if (recreateReason == RecreationNotNeeded) {
        qCDebug(lcQpaCocoaWindow) << "No need to recreate NSWindow";
        return;
    }

    qCDebug(lcQpaCocoaWindow) << "Reconfiguring NSWindow due to" << recreateReason;

    QCocoaWindow *parentCocoaWindow = static_cast<QCocoaWindow *>(parentWindow);

    if (shouldBeChildNSWindow) {
        QWindow *parentQWindow = parentWindow->window();
        // Ensure that all parents in the hierarchy are also child NSWindows
        if (!parentQWindow->property("_q_platform_MacUseNSWindow").toBool()) {
            parentQWindow->setProperty("_q_platform_MacUseNSWindow", QVariant(true));
            parentCocoaWindow->recreateWindowIfNeeded();
        }
    }

    // Remove current window (if any)
    if ((isContentView() && !shouldBeContentView) || (recreateReason & PanelChanged)) {
        qCDebug(lcQpaCocoaWindow) << "Getting rid of existing window" << m_nsWindow;
        [m_nsWindow closeAndRelease];
        if (isChildNSWindow())
            [m_view.window.parentWindow removeChildWindow:m_view.window];
        if (isContentView()) {
            // We explicitly disassociate m_view from the window's contentView,
            // as AppKit does not automatically do this in response to removing
            // the view from the NSThemeFrame subview list, so we might end up
            // with a NSWindow contentView pointing to a deallocated NSView.
            m_view.window.contentView = nil;
        }
        m_nsWindow = 0;
    }

    if (shouldBeContentView) {
        bool noPreviousWindow = m_nsWindow == 0;
        QCocoaNSWindow *newWindow = nullptr;
        if (noPreviousWindow)
            newWindow = createNSWindow(shouldBeChildNSWindow, shouldBePanel);

        if (m_view.window.parentWindow) {
            if (!shouldBeChildNSWindow || (recreateReason & ParentChanged))
                [m_view.window.parentWindow removeChildWindow:m_view.window];
            m_forwardWindow = oldParentCocoaWindow;
        }

        // Move view to new NSWindow if needed
        if (newWindow) {
            qCDebug(lcQpaCocoaWindow) << "Ensuring that view is content view for" << m_nsWindow;
            [m_view setPostsFrameChangedNotifications:NO];
            [newWindow setContentView:m_view];
            [m_view setPostsFrameChangedNotifications:YES];

            m_nsWindow = newWindow;
            Q_ASSERT(m_view.window == m_nsWindow);
        }
    }

    if (m_viewIsToBeEmbedded) {
        // An embedded window doesn't have its own NSWindow.
    } else if (!parentWindow) {
        // QPlatformWindow subclasses must sync up with QWindow on creation:
        propagateSizeHints();
        setWindowFlags(window()->flags());
        setWindowTitle(window()->title());
        setWindowState(window()->windowState());
    } else if (shouldBeChildNSWindow) {
        if (!m_hiddenByClipping) {
            [parentCocoaWindow->nativeWindow() addChildWindow:m_view.window ordered:NSWindowAbove];
            parentCocoaWindow->reinsertChildWindow(this);
        }

        // Set properties after the window has been made a child NSWindow
        setCocoaGeometry(windowGeometry());
        setWindowFlags(window()->flags());
    } else {
        // Child windows have no NSWindow, link the NSViews instead.
        [parentCocoaWindow->m_view addSubview:m_view];
        QRect rect = windowGeometry();
        // Prevent setting a (0,0) window size; causes opengl context
        // "Invalid Drawable" warnings.
        if (rect.isNull())
            rect.setSize(QSize(1, 1));
        NSRect frame = NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height());
        [m_view setFrame:frame];
        [m_view setHidden:!window()->isVisible()];
    }

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
    const QObjectList &childWindows = window()->children();
    int childIndex = childWindows.indexOf(child->window());
    Q_ASSERT(childIndex != -1);

    for (int i = childIndex; i < childWindows.size(); ++i) {
        QWindow *window = static_cast<QWindow *>(childWindows.at(i));
        QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
        if (!cocoaWindow)
            continue;

        NSWindow *nsChild = cocoaWindow->nativeWindow();
        if (i != childIndex)
            [m_view.window removeChildWindow:nsChild];
        [m_view.window addChildWindow:nsChild ordered:NSWindowAbove];
    }
}

void QCocoaWindow::requestActivateWindow()
{
    NSWindow *window = [m_view window];
    [window makeFirstResponder:m_view];
    [window makeKeyWindow];
}

QCocoaNSWindow *QCocoaWindow::createNSWindow(bool shouldBeChildNSWindow, bool shouldBePanel)
{
    qCDebug(lcQpaCocoaWindow) << "createNSWindow" << shouldBeChildNSWindow << shouldBePanel;

    QMacAutoReleasePool pool;

    QRect rect = geometry();

    QScreen *targetScreen = nullptr;
    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen->geometry().contains(rect.topLeft())) {
            targetScreen = screen;
            break;
        }
    }

    if (!targetScreen) {
        qCWarning(lcQpaCocoaWindow) << "Window position outside any known screen, using primary screen";
        targetScreen = QGuiApplication::primaryScreen();
    }

    rect.translate(-targetScreen->geometry().topLeft());
    QCocoaScreen *cocoaScreen = static_cast<QCocoaScreen *>(targetScreen->handle());
    NSRect frame = NSRectFromCGRect(cocoaScreen->mapToNative(rect).toCGRect());

    // Note: The macOS window manager has a bug, where if a screen is rotated, it will not allow
    // a window to be created within the area of the screen that has a Y coordinate (I quadrant)
    // higher than the height of the screen  in its non-rotated state, unless the window is
    // created with the NSWindowStyleMaskBorderless style mask.

    Qt::WindowType type = window()->type();
    Qt::WindowFlags flags = window()->flags();

    // Create NSWindow
    Class windowClass = shouldBePanel ? [QNSPanel class] : [QNSWindow class];
    NSUInteger styleMask = shouldBeChildNSWindow ? NSBorderlessWindowMask : windowStyleMask(flags);
    QCocoaNSWindow *window = [[windowClass alloc] initWithContentRect:frame
        screen:cocoaScreen->nativeScreen() styleMask:styleMask qPlatformWindow:this];

    window.restorable = NO;
    window.level = shouldBeChildNSWindow ? NSNormalWindowLevel : windowLevel(flags);

    if (!isOpaque()) {
        window.backgroundColor = [NSColor clearColor];
        window.opaque = NO;
    }

    Q_ASSERT(!(shouldBePanel && shouldBeChildNSWindow));

    if (shouldBePanel) {
        // Qt::Tool windows hide on app deactivation, unless Qt::WA_MacAlwaysShowToolWindow is set
        window.hidesOnDeactivate = ((type & Qt::Tool) == Qt::Tool) && !alwaysShowToolWindow();

        // Make popup windows show on the same desktop as the parent full-screen window
        window.collectionBehavior = NSWindowCollectionBehaviorFullScreenAuxiliary;

        if ((type & Qt::Popup) == Qt::Popup) {
            window.hasShadow = YES;
            window.animationBehavior = NSWindowAnimationBehaviorUtilityWindow;
        }
    } else if (shouldBeChildNSWindow) {
        window.collectionBehavior =
              NSWindowCollectionBehaviorManaged
            | NSWindowCollectionBehaviorIgnoresCycle
            | NSWindowCollectionBehaviorFullScreenAuxiliary;
        window.hasShadow = NO;
        window.animationBehavior = NSWindowAnimationBehaviorNone;
    }

    // Persist modality so we can detect changes later on
    m_windowModality = QPlatformWindow::window()->modality();

    applyContentBorderThickness(window);

    return window;
}

bool QCocoaWindow::alwaysShowToolWindow() const
{
    return qt_mac_resolveOption(false, window(), "_q_macAlwaysShowToolWindow", "");
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
    if (!isContentView() || isChildNSWindow())
        return geometry();

    NSRect rect = m_view.window.frame;
    QPlatformScreen *onScreen = QPlatformScreen::platformScreenForWindow(window());
    int flippedY = onScreen->geometry().height() - rect.origin.y - rect.size.height;  // account for nswindow inverted y.
    QRect qRect = QRect(rect.origin.x, flippedY, rect.size.width, rect.size.height);
    return qRect;
}

/*!
    Applies the given state to the NSWindow, going in/out of minimize/zoomed/fullscreen

    When this is called from QWindow::setWindowState(), the QWindow state has not been
    updated yet, so window()->windowState() will reflect the previous state that was
    reported to QtGui.
*/
void QCocoaWindow::applyWindowState(Qt::WindowStates requestedState)
{
    const Qt::WindowState currentState = windowState();
    const Qt::WindowState newState = QWindowPrivate::effectiveState(requestedState);

    if (newState == currentState)
        return;

    if (!isContentView())
        return;

    const NSSize contentSize = m_view.frame.size;
    if (contentSize.width <= 0 || contentSize.height <= 0) {
        // If content view width or height is 0 then the window animations will crash so
        // do nothing. We report the current state back to reflect the failed operation.
        qWarning("invalid window content view size, check your window geometry");
        reportCurrentWindowState(true);
        return;
    }

    const NSWindow *nsWindow = m_view.window;

    if (nsWindow.styleMask & NSUtilityWindowMask) {
        // Utility panels cannot be fullscreen
        qWarning() << window()->type() << "windows can not be made full screen";
        reportCurrentWindowState(true);
        return;
    }

    const id sender = nsWindow;

    // First we need to exit states that can't transition directly to other states
    switch (currentState) {
    case Qt::WindowMinimized:
        [nsWindow deminiaturize:sender];
        Q_ASSERT_X(windowState() != Qt::WindowMinimized, "QCocoaWindow",
            "[NSWindow deminiaturize:] is synchronous");
        break;
    case Qt::WindowFullScreen: {
        toggleFullScreen();
        // Exiting fullscreen is not synchronous, so we need to wait for the
        // NSWindowDidExitFullScreenNotification before continuing to apply
        // the new state.
        return;
    }
    default:;
    }

    // Then we apply the new state if needed
    if (newState == windowState())
        return;

    switch (newState) {
    case Qt::WindowFullScreen:
        toggleFullScreen();
        break;
    case Qt::WindowMaximized:
        toggleMaximized();
        break;
    case Qt::WindowMinimized:
        [nsWindow miniaturize:sender];
        break;
    case Qt::WindowNoState:
        if (windowState() == Qt::WindowMaximized)
            toggleMaximized();
        break;
    default:
        Q_UNREACHABLE();
    }
}

void QCocoaWindow::toggleMaximized()
{
    const NSWindow *window = m_view.window;

    // The NSWindow needs to be resizable, otherwise the window will
    // not be possible to zoom back to non-zoomed state.
    const bool wasResizable = window.styleMask & NSResizableWindowMask;
    window.styleMask |= NSResizableWindowMask;

    const id sender = window;
    [window zoom:sender];

    if (!wasResizable)
        window.styleMask &= ~NSResizableWindowMask;
}

void QCocoaWindow::toggleFullScreen()
{
    const NSWindow *window = m_view.window;

    // The window needs to have the correct collection behavior for the
    // toggleFullScreen call to have an effect. The collection behavior
    // will be reset in windowDidEnterFullScreen/windowDidLeaveFullScreen.
    window.collectionBehavior |= NSWindowCollectionBehaviorFullScreenPrimary;

    const id sender = window;
    [window toggleFullScreen:sender];
}

bool QCocoaWindow::isTransitioningToFullScreen() const
{
    NSWindow *window = m_view.window;
    return window.styleMask & NSFullScreenWindowMask && !window.qt_fullScreen;
}

Qt::WindowState QCocoaWindow::windowState() const
{
    // FIXME: Support compound states (Qt::WindowStates)

    NSWindow *window = m_view.window;
    if (window.miniaturized)
        return Qt::WindowMinimized;
    if (window.qt_fullScreen)
        return Qt::WindowFullScreen;
    if ((window.zoomed && !isTransitioningToFullScreen())
        || (m_lastReportedWindowState == Qt::WindowMaximized && isTransitioningToFullScreen()))
        return Qt::WindowMaximized;

    // Note: We do not report Qt::WindowActive, even if isActive()
    // is true, as QtGui does not expect this window state to be set.

    return Qt::WindowNoState;
}

void QCocoaWindow::reportCurrentWindowState(bool unconditionally)
{
    Qt::WindowState currentState = windowState();
    if (!unconditionally && currentState == m_lastReportedWindowState)
        return;

    QWindowSystemInterface::handleWindowStateChanged<QWindowSystemInterface::SynchronousDelivery>(
        window(), currentState, m_lastReportedWindowState);
    m_lastReportedWindowState = currentState;
}

bool QCocoaWindow::setWindowModified(bool modified)
{
    if (!isContentView())
        return false;

    m_view.window.documentEdited = modified;
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
    if (isForeignWindow())
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

    applyContentBorderThickness();
}

void QCocoaWindow::registerContentBorderArea(quintptr identifier, int upper, int lower)
{
    m_contentBorderAreas.insert(identifier, BorderRange(identifier, upper, lower));
    applyContentBorderThickness();
}

void QCocoaWindow::setContentBorderAreaEnabled(quintptr identifier, bool enable)
{
    m_enabledContentBorderAreas.insert(identifier, enable);
    applyContentBorderThickness();
}

void QCocoaWindow::setContentBorderEnabled(bool enable)
{
    m_drawContentBorderGradient = enable;
    applyContentBorderThickness();
}

void QCocoaWindow::applyContentBorderThickness(NSWindow *window)
{
    if (!window && isContentView())
        window = m_view.window;

    if (!window)
        return;

    if (!m_drawContentBorderGradient) {
        window.styleMask = window.styleMask & ~NSTexturedBackgroundWindowMask;
        [window.contentView.superview setNeedsDisplay:YES];
        return;
    }

    // Find consecutive registered border areas, starting from the top.
    std::vector<BorderRange> ranges(m_contentBorderAreas.cbegin(), m_contentBorderAreas.cend());
    std::sort(ranges.begin(), ranges.end());
    int effectiveTopContentBorderThickness = m_topContentBorderThickness;
    for (BorderRange range : ranges) {
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
    if (!isContentView())
        return;

    NSToolbar *toolbar = QCocoaIntegration::instance()->toolbar(window());
    const NSWindow *window = m_view.window;

    if (window.toolbar == toolbar)
       return;

    window.toolbar = toolbar;
    window.showsToolbarButton = YES;
}

bool QCocoaWindow::testContentBorderAreaPosition(int position) const
{
    return isContentView() && m_drawContentBorderGradient &&
            0 <= position && position < [m_view.window contentBorderThicknessForEdge:NSMaxYEdge];
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

    if (window()->isTopLevel()) {
        // Update the QWindow's screen property. This property is set
        // to QGuiApplication::primaryScreen() at QWindow construciton
        // time, and we won't get a NSWindowDidChangeScreenNotification
        // on show. The case where the window is initially displayed
        // on a non-primary screen needs special handling here.
        if (QCocoaScreen *cocoaScreen = QCocoaIntegration::instance()->screenForNSScreen(m_view.window.screen))
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
    if (!isContentView())
        return QMargins();

    NSRect frameW = m_view.window.frame;
    NSRect frameC = [m_view.window contentRectForFrameRect:frameW];

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
