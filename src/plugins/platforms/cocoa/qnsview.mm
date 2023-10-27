// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/qtguiglobal.h>

#include <AppKit/AppKit.h>
#include <MetalKit/MetalKit.h>

#include "qnsview.h"
#include "qcocoawindow.h"
#include "qcocoahelpers.h"
#include "qcocoascreen.h"
#include "qmultitouch_mac_p.h"
#include "qcocoadrag.h"
#include "qcocoainputcontext.h"
#include <qpa/qplatformintegration.h>

#include <qpa/qwindowsysteminterface.h>
#include <QtGui/QTextFormat>
#include <QtCore/QDebug>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/qsysinfo.h>
#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/QAccessible>
#include <QtGui/QImage>
#include <private/qguiapplication_p.h>
#include <private/qcoregraphics_p.h>
#include <private/qwindow_p.h>
#include <private/qpointingdevice_p.h>
#include <private/qhighdpiscaling_p.h>
#include "qcocoabackingstore.h"
#ifndef QT_NO_OPENGL
#include "qcocoaglcontext.h"
#endif
#include "qcocoaintegration.h"
#include <QtGui/private/qmacmimeregistry_p.h>

@interface QNSView (Drawing) <CALayerDelegate>
- (void)initDrawing;
@end

@interface QT_MANGLE_NAMESPACE(QNSViewMouseMoveHelper) : NSObject
- (instancetype)initWithView:(QNSView *)theView;
- (void)mouseMoved:(NSEvent *)theEvent;
- (void)mouseEntered:(NSEvent *)theEvent;
- (void)mouseExited:(NSEvent *)theEvent;
- (void)cursorUpdate:(NSEvent *)theEvent;
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSViewMouseMoveHelper);

@interface QNSView (Mouse)
- (void)initMouse;
- (NSPoint)screenMousePoint:(NSEvent *)theEvent;
- (void)mouseMovedImpl:(NSEvent *)theEvent;
- (void)mouseEnteredImpl:(NSEvent *)theEvent;
- (void)mouseExitedImpl:(NSEvent *)theEvent;
@end

@interface QNSView (Touch)
@end

@interface QNSView (Tablet)
- (bool)handleTabletEvent:(NSEvent *)theEvent;
@end

@interface QNSView (Gestures)
@end

@interface QNSView (Dragging)
-(void)registerDragTypes;
@end

@interface QNSView (Keys)
@end

@interface QNSView (ComplexText) <NSTextInputClient>
@property (readonly) QObject* focusObject;
@end

@interface QT_MANGLE_NAMESPACE(QNSViewMenuHelper) : NSObject
- (instancetype)initWithView:(QNSView *)theView;
@end
QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSViewMenuHelper);

// Private interface
@interface QNSView ()
- (BOOL)isTransparentForUserInput;
@property (assign) NSView* previousSuperview;
@property (assign) NSWindow* previousWindow;
@property (retain) QNSViewMenuHelper* menuHelper;
@end

@implementation QNSView {
    QPointer<QCocoaWindow> m_platformWindow;

    // Mouse
    QNSViewMouseMoveHelper *m_mouseMoveHelper;
    Qt::MouseButtons m_buttons;
    Qt::MouseButtons m_acceptedMouseDowns;
    Qt::MouseButtons m_frameStrutButtons;
    Qt::KeyboardModifiers m_currentWheelModifiers;
    bool m_dontOverrideCtrlLMB;
    bool m_sendUpAsRightButton;
    bool m_scrolling;
    bool m_updatingDrag;

    // Keys
    bool m_lastKeyDead;
    bool m_sendKeyEvent;
    bool m_sendKeyEventWithoutText;
    NSEvent *m_currentlyInterpretedKeyEvent;
    QSet<quint32> m_acceptedKeyDowns;

    // Text
    QString m_composingText;
    QPointer<QObject> m_composingFocusObject;
    NSDraggingContext m_lastSeenContext;
}

- (instancetype)initWithCocoaWindow:(QCocoaWindow *)platformWindow
{
    if ((self = [super initWithFrame:NSZeroRect])) {
        m_platformWindow = platformWindow;

        // NSViews are by default visible, but QWindows are not.
        // We should ideally pick up the actual QWindow state here,
        // but QWindowPrivate::setVisible() expects to control the
        // order of events tightly, so we need to wait for a call
        // to QCocoaWindow::setVisible().
        self.hidden = YES;

        self.focusRingType = NSFocusRingTypeNone;

        self.previousSuperview = nil;
        self.previousWindow = nil;

        [self initDrawing];
        [self initMouse];
        [self registerDragTypes];

        m_updatingDrag = false;

        m_lastKeyDead = false;
        m_sendKeyEvent = false;
        m_currentlyInterpretedKeyEvent = nil;
        m_lastSeenContext = NSDraggingContextWithinApplication;

        self.menuHelper = [[[QNSViewMenuHelper alloc] initWithView:self] autorelease];
    }
    return self;
}

- (void)dealloc
{
    qCDebug(lcQpaWindow) << "Deallocating" << self;

    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [m_mouseMoveHelper release];

    [super dealloc];
}

- (NSString *)description
{
    NSMutableString *description = [NSMutableString stringWithString:[super description]];

#ifndef QT_NO_DEBUG_STREAM
    QString platformWindowDescription;
    QDebug debug(&platformWindowDescription);
    debug.nospace() << "; " << m_platformWindow << ">";

    NSRange lastCharacter = [description rangeOfComposedCharacterSequenceAtIndex:description.length - 1];
    [description replaceCharactersInRange:lastCharacter withString:platformWindowDescription.toNSString()];
#endif

    return description;
}

// ----------------------------- Re-parenting ---------------------------------

- (void)removeFromSuperview
{
    QMacAutoReleasePool pool;
    [super removeFromSuperview];
}

- (void)viewWillMoveToSuperview:(NSView *)newSuperview
{
    Q_ASSERT(!self.previousSuperview);
    self.previousSuperview = self.superview;

    if (newSuperview == self.superview)
        qCDebug(lcQpaWindow) << "Re-ordering" << self << "inside" << self.superview;
    else
        qCDebug(lcQpaWindow) << "Re-parenting" << self << "from" << self.superview << "to" << newSuperview;
}

- (void)viewDidMoveToSuperview
{
    auto cleanup = qScopeGuard([&] { self.previousSuperview = nil; });

    if (self.superview == self.previousSuperview) {
        qCDebug(lcQpaWindow) << "Done re-ordering" << self << "new index:"
            << [self.superview.subviews indexOfObject:self];
        return;
    }

    qCDebug(lcQpaWindow) << "Done re-parenting" << self << "into" << self.superview;

    // Note: at this point the view's window property hasn't been updated to match the window
    // of the new superview. We have to wait for viewDidMoveToWindow for that to be reflected.

    if (!m_platformWindow)
        return;

    if (!m_platformWindow->isEmbedded())
        return;

    if ([self superview]) {
        QWindowSystemInterface::handleGeometryChange(m_platformWindow->window(), m_platformWindow->geometry());
        [self setNeedsDisplay:YES];
        QWindowSystemInterface::flushWindowSystemEvents();
    }
}

- (void)viewWillMoveToWindow:(NSWindow *)newWindow
{
    Q_ASSERT(!self.previousWindow);
    self.previousWindow = self.window;

    // This callback is documented to be called also when a view is just moved between
    // subviews in the same NSWindow, so we're not necessarily moving between NSWindows.
    if (newWindow == self.window)
        return;

    qCDebug(lcQpaWindow) << "Moving" << self << "from" << self.window << "to" << newWindow;

    // Note: at this point the superview has already been updated, so we know which view inside
    // the new window the view will be a child of.
}

- (void)viewDidMoveToWindow
{
    auto cleanup = qScopeGuard([&] { self.previousWindow = nil; });

    // This callback is documented to be called also when a view is just moved between
    // subviews in the same NSWindow, so we're not necessarily moving between NSWindows.
    if (self.window == self.previousWindow)
        return;

    qCDebug(lcQpaWindow) << "Done moving" << self << "to" << self.window;
}

// ----------------------------------------------------------------------------

- (QWindow *)topLevelWindow
{
    if (!m_platformWindow)
        return nullptr;

    QWindow *focusWindow = m_platformWindow->window();

    // For widgets we need to do a bit of trickery as the window
    // to activate is the window of the top-level widget.
    if (qstrcmp(focusWindow->metaObject()->className(), "QWidgetWindow") == 0) {
        while (focusWindow->parent()) {
            focusWindow = focusWindow->parent();
        }
    }

    return focusWindow;
}

/*
    Invoked when the view is hidden, either directly,
    or in response to an ancestor being hidden.
*/
- (void)viewDidHide
{
    qCDebug(lcQpaWindow) << "Did hide" << self;

    if (!m_platformWindow->isExposed())
        return;

    m_platformWindow->handleExposeEvent(QRegion());
}

/*
    Invoked when the view is unhidden, either directly,
    or in response to an ancestor being unhidden.
*/
- (void)viewDidUnhide
{
     qCDebug(lcQpaWindow) << "Did unhide" << self;

     [self setNeedsDisplay:YES];
}

- (BOOL)isTransparentForUserInput
{
    return m_platformWindow->window() &&
        m_platformWindow->window()->flags() & Qt::WindowTransparentForInput;
}

- (BOOL)becomeFirstResponder
{
    if (!m_platformWindow)
        return NO;
    if ([self isTransparentForUserInput])
        return NO;

    if (!m_platformWindow->windowIsPopupType()
            && (!self.window.canBecomeKeyWindow || self.window.keyWindow)) {
        // Calling handleWindowActivated for a QWindow has two effects: first, it
        // will set the QWindow (and all other QWindows in the same hierarchy)
        // as Active. Being Active means that the window should appear active from
        // a style perspective (according to QWindow::isActive()). The second
        // effect is that it will set QQuiApplication::focusWindow() to point to
        // the QWindow. The latter means that the QWindow should have keyboard
        // focus. But those two are not necessarily the same; A tool window could e.g be
        // rendered as Active while the parent window, which is also Active, has
        // input focus. But we currently don't distinguish between that cleanly in Qt.
        // Since we don't want a QWindow to be rendered as Active when the NSWindow
        // it belongs to is not key, we skip calling handleWindowActivated when
        // that is the case. Instead, we wait for the window to become key, and handle
        // QWindow activation from QCocoaWindow::windowDidBecomeKey instead. The only
        // exception is if the window can never become key, in which case we naturally
        // cannot wait for that to happen.
        QWindowSystemInterface::handleWindowActivated<QWindowSystemInterface::SynchronousDelivery>(
            [self topLevelWindow], Qt::ActiveWindowFocusReason);
    }

    return YES;
}

- (BOOL)acceptsFirstResponder
{
    if (!m_platformWindow)
        return NO;
    if (m_platformWindow->shouldRefuseKeyWindowAndFirstResponder())
        return NO;
    if ([self isTransparentForUserInput])
        return NO;
    if ((m_platformWindow->window()->flags() & Qt::ToolTip) == Qt::ToolTip)
        return NO;
    return YES;
}

- (NSView *)hitTest:(NSPoint)aPoint
{
    NSView *candidate = [super hitTest:aPoint];
    if (candidate == self) {
        if ([self isTransparentForUserInput])
            return nil;
    }
    return candidate;
}

- (void)convertFromScreen:(NSPoint)mouseLocation toWindowPoint:(QPointF *)qtWindowPoint andScreenPoint:(QPointF *)qtScreenPoint
{
    // Calculate the mouse position in the QWindow and Qt screen coordinate system,
    // starting from coordinates in the NSWindow coordinate system.
    //
    // This involves translating according to the window location on screen,
    // as well as inverting the y coordinate due to the origin change.
    //
    // Coordinate system overview, outer to innermost:
    //
    // Name             Origin
    //
    // OS X screen      bottom-left
    // Qt screen        top-left
    // NSWindow         bottom-left
    // NSView/QWindow   top-left
    //
    // NSView and QWindow are equal coordinate systems: the QWindow covers the
    // entire NSView, and we've set the NSView's isFlipped property to true.

    NSWindow *window = [self window];
    NSPoint nsWindowPoint;
    NSRect windowRect = [window convertRectFromScreen:NSMakeRect(mouseLocation.x, mouseLocation.y, 1, 1)];
    nsWindowPoint = windowRect.origin;                    // NSWindow coordinates
    NSPoint nsViewPoint = [self convertPoint: nsWindowPoint fromView: nil]; // NSView/QWindow coordinates
    *qtWindowPoint = QPointF(nsViewPoint.x, nsViewPoint.y);                     // NSView/QWindow coordinates
    *qtScreenPoint = QCocoaScreen::mapFromNative(mouseLocation);
}

@end

#include "qnsview_drawing.mm"
#include "qnsview_mouse.mm"
#include "qnsview_touch.mm"
#include "qnsview_gestures.mm"
#include "qnsview_tablet.mm"
#include "qnsview_dragging.mm"
#include "qnsview_keys.mm"
#include "qnsview_complextext.mm"
#include "qnsview_menus.mm"
#if QT_CONFIG(accessibility)
#include "qnsview_accessibility.mm"
#endif

// -----------------------------------------------------

@implementation QNSView (QtExtras)

- (QCocoaWindow*)platformWindow
{
    return m_platformWindow.data();;
}

@end
