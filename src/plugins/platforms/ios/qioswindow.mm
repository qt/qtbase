/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiosglobal.h"
#include "qioswindow.h"
#include "qioscontext.h"
#include "qiosinputcontext.h"
#include "qiosscreen.h"
#include "qiosapplicationdelegate.h"
#include "qiosviewcontroller.h"
#include "qiosintegration.h"
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qwindow_p.h>
#include <qpa/qplatformintegration.h>

#import <QuartzCore/CAEAGLLayer.h>

#include <QtGui/QKeyEvent>
#include <qpa/qwindowsysteminterface.h>

#include <QtDebug>

@interface QUIView : UIView <UIKeyInput>
{
@public
    UITextAutocapitalizationType autocapitalizationType;
    UITextAutocorrectionType autocorrectionType;
    BOOL enablesReturnKeyAutomatically;
    UIKeyboardAppearance keyboardAppearance;
    UIKeyboardType keyboardType;
    UIReturnKeyType returnKeyType;
    BOOL secureTextEntry;
    QIOSWindow *m_qioswindow;
    QHash<UITouch *, QWindowSystemInterface::TouchPoint> m_activeTouches;
    int m_nextTouchId;
}

@property(nonatomic) UITextAutocapitalizationType autocapitalizationType;
@property(nonatomic) UITextAutocorrectionType autocorrectionType;
@property(nonatomic) BOOL enablesReturnKeyAutomatically;
@property(nonatomic) UIKeyboardAppearance keyboardAppearance;
@property(nonatomic) UIKeyboardType keyboardType;
@property(nonatomic) UIReturnKeyType returnKeyType;
@property(nonatomic, getter=isSecureTextEntry) BOOL secureTextEntry;

@end

@implementation QUIView

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

-(id)initWithQIOSWindow:(QIOSWindow *)window
{
    if (self = [self initWithFrame:toCGRect(window->geometry())])
        m_qioswindow = window;

    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        // Set up EAGL layer
        CAEAGLLayer *eaglLayer = static_cast<CAEAGLLayer *>(self.layer);
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
            kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];

        // Set up text input
        autocapitalizationType = UITextAutocapitalizationTypeNone;
        autocorrectionType = UITextAutocorrectionTypeNo;
        enablesReturnKeyAutomatically = NO;
        keyboardAppearance = UIKeyboardAppearanceDefault;
        keyboardType = UIKeyboardTypeDefault;
        returnKeyType = UIReturnKeyDone;
        secureTextEntry = NO;
        m_nextTouchId = 0;

        if (isQtApplication())
            self.hidden = YES;

        self.multipleTouchEnabled = YES;
    }

    return self;
}

- (void)willMoveToWindow:(UIWindow *)newWindow
{
    // UIKIt will normally set the scale factor of a view to match the corresponding
    // screen scale factor, but views backed by CAEAGLLayers need to do this manually.
    self.contentScaleFactor = newWindow && newWindow.screen ?
        newWindow.screen.scale : [[UIScreen mainScreen] scale];

    // FIXME: Allow the scale factor to be customized through QSurfaceFormat.
}

- (void)didAddSubview:(UIView *)subview
{
    if ([subview isKindOfClass:[QUIView class]])
        self.clipsToBounds = YES;
}

- (void)willRemoveSubview:(UIView *)subview
{
    for (UIView *view in self.subviews) {
        if (view != subview && [view isKindOfClass:[QUIView class]])
            return;
    }

    self.clipsToBounds = NO;
}

- (void)layoutSubviews
{
    // This method is the de facto way to know that view has been resized,
    // or otherwise needs invalidation of its buffers. Note though that we
    // do not get this callback when the view just changes its position, so
    // the position of our QWindow (and platform window) will only get updated
    // when the size is also changed.

    if (!CGAffineTransformIsIdentity(self.transform))
        qWarning() << m_qioswindow->window()
            << "is backed by a UIView that has a transform set. This is not supported.";

    QRect geometry = fromCGRect(self.frame);
    m_qioswindow->QPlatformWindow::setGeometry(geometry);
    QWindowSystemInterface::handleGeometryChange(m_qioswindow->window(), geometry);
    QWindowSystemInterface::handleExposeEvent(m_qioswindow->window(), QRect(QPoint(), geometry.size()));

    // If we have a new size here we need to resize the FBO's corresponding buffers,
    // but we defer that to when the application calls makeCurrent.

    [super layoutSubviews];
}

- (void)updateTouchList:(NSSet *)touches withState:(Qt::TouchPointState)state
{
    // We deliver touch events in global coordinates. But global in this respect
    // means the same coordinate system that we use for describing the geometry
    // of the top level QWindow we're inside. And that would be the coordinate
    // system of the superview of the UIView that backs that window:
    QPlatformWindow *topLevel = m_qioswindow;
    while (QPlatformWindow *topLevelParent = topLevel->parent())
        topLevel = topLevelParent;
    UIView *rootView = reinterpret_cast<UIView *>(topLevel->winId()).superview;
    CGSize rootViewSize = rootView.frame.size;

    foreach (UITouch *uiTouch, m_activeTouches.keys()) {
        QWindowSystemInterface::TouchPoint &touchPoint = m_activeTouches[uiTouch];
        if (![touches containsObject:uiTouch]) {
            touchPoint.state = Qt::TouchPointStationary;
        } else {
            touchPoint.state = state;
            touchPoint.pressure = (state == Qt::TouchPointReleased) ? 0.0 : 1.0;
            QPoint touchPos = fromCGPoint([uiTouch locationInView:rootView]);
            touchPoint.area = QRectF(touchPos, QSize(0, 0));
            touchPoint.normalPosition = QPointF(touchPos.x() / rootViewSize.width, touchPos.y() / rootViewSize.height);
        }
    }
}

- (void) sendTouchEventWithTimestamp:(ulong)timeStamp
{
    // Send touch event synchronously
    QIOSIntegration *iosIntegration = static_cast<QIOSIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QWindowSystemInterface::handleTouchEvent(m_qioswindow->window(), timeStamp, iosIntegration->touchDevice(), m_activeTouches.values());
    QWindowSystemInterface::flushWindowSystemEvents();
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    // UIKit generates [Began -> Moved -> Ended] event sequences for
    // each touch point. Internally we keep a hashmap of active UITouch
    // points to QWindowSystemInterface::TouchPoints, and assigns each TouchPoint
    // an id for use by Qt.
    for (UITouch *touch in touches) {
        Q_ASSERT(!m_activeTouches.contains(touch));
        m_activeTouches[touch].id = m_nextTouchId++;
    }

    [self updateTouchList:touches withState:Qt::TouchPointPressed];
    [self sendTouchEventWithTimestamp:ulong(event.timestamp * 1000)];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self updateTouchList:touches withState:Qt::TouchPointMoved];
    [self sendTouchEventWithTimestamp:ulong(event.timestamp * 1000)];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    QWindow *window = m_qioswindow->window();
    if (window != QGuiApplication::focusWindow() && m_activeTouches.size() == 1) {
        // Activate the touched window if the last touch was released inside it:
        UITouch *touch = static_cast<UITouch *>([[touches allObjects] lastObject]);
        if (CGRectContainsPoint([self bounds], [touch locationInView:self]))
            m_qioswindow->requestActivateWindow();
    }

    [self updateTouchList:touches withState:Qt::TouchPointReleased];
    [self sendTouchEventWithTimestamp:ulong(event.timestamp * 1000)];

    // Remove ended touch points from the active set:
    for (UITouch *touch in touches)
        m_activeTouches.remove(touch);
    if (m_activeTouches.isEmpty())
        m_nextTouchId = 0;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (!touches && m_activeTouches.isEmpty())
        return;

    if (!touches) {
        m_activeTouches.clear();
    } else {
        for (UITouch *touch in touches)
            m_activeTouches.remove(touch);

        Q_ASSERT_X(m_activeTouches.isEmpty(), Q_FUNC_INFO,
            "Subset of active touches cancelled by UIKit");
    }

    m_nextTouchId = 0;

    NSTimeInterval timestamp = event ? event.timestamp : [[NSProcessInfo processInfo] systemUptime];

    // Send cancel touch event synchronously
    QIOSIntegration *iosIntegration = static_cast<QIOSIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QWindowSystemInterface::handleTouchCancelEvent(m_qioswindow->window(), ulong(timestamp * 1000), iosIntegration->touchDevice());
    QWindowSystemInterface::flushWindowSystemEvents();
}

@synthesize autocapitalizationType;
@synthesize autocorrectionType;
@synthesize enablesReturnKeyAutomatically;
@synthesize keyboardAppearance;
@synthesize keyboardType;
@synthesize returnKeyType;
@synthesize secureTextEntry;

- (BOOL)canBecomeFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    // On iOS, a QWindow should only have input focus when the input panel is
    // open. This is to stop cursors and focus rects from being drawn when the
    // user cannot type. And since the keyboard will open when a view becomes
    // the first responder, it's now a good time to inform QPA that the QWindow
    // this view backs became active:
    QWindowSystemInterface::handleWindowActivated(m_qioswindow->window());
    return [super becomeFirstResponder];
}

- (BOOL)resignFirstResponder
{
    // Resigning first responed status means that the virtual keyboard was closed, or
    // some other view became first responder. In either case we clear the focus object to
    // avoid blinking cursors in line edits etc:
    static_cast<QWindowPrivate *>(QObjectPrivate::get(m_qioswindow->window()))->clearFocusObject();
    return [super resignFirstResponder];
}

- (BOOL)hasText
{
    return YES;
}

- (void)insertText:(NSString *)text
{
    QString string = QString::fromUtf8([text UTF8String]);
    int key = 0;
    if ([text isEqualToString:@"\n"])
        key = (int)Qt::Key_Return;

    // Send key event to window system interface
    QWindowSystemInterface::handleKeyEvent(
        0, QEvent::KeyPress, key, Qt::NoModifier, string, false, int(string.length()));
    QWindowSystemInterface::handleKeyEvent(
        0, QEvent::KeyRelease, key, Qt::NoModifier, string, false, int(string.length()));
}

- (void)deleteBackward
{
    // Send key event to window system interface
    QWindowSystemInterface::handleKeyEvent(
        0, QEvent::KeyPress, (int)Qt::Key_Backspace, Qt::NoModifier);
    QWindowSystemInterface::handleKeyEvent(
        0, QEvent::KeyRelease, (int)Qt::Key_Backspace, Qt::NoModifier);
}

@end

@implementation UIView (QIOS)

- (QWindow *)qwindow
{
    if ([self isKindOfClass:[QUIView class]])
        return static_cast<QUIView *>(self)->m_qioswindow->window();
    return nil;
}

- (UIViewController *)viewController
{
    id responder = self;
    while ((responder = [responder nextResponder])) {
        if ([responder isKindOfClass:UIViewController.class])
            return responder;
    }
    return nil;
}

@end

QT_BEGIN_NAMESPACE

QIOSWindow::QIOSWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_view([[QUIView alloc] initWithQIOSWindow:this])
    , m_normalGeometry(QPlatformWindow::geometry())
    , m_windowLevel(0)
{
    setParent(QPlatformWindow::parent());
    setWindowState(window->windowState());
}

QIOSWindow::~QIOSWindow()
{
    // According to the UIResponder documentation, Cocoa Touch should react to system interruptions
    // that "might cause the view to be removed from the window" by sending touchesCancelled, but in
    // practice this doesn't seem to happen when removing the view from its superview. To ensure that
    // Qt's internal state for touch and mouse handling is kept consistent, we therefor have to force
    // cancellation of all touch events.
    [m_view touchesCancelled:0 withEvent:0];

    [m_view removeFromSuperview];
    [m_view release];
}

bool QIOSWindow::blockedByModal()
{
    QWindow *modalWindow = QGuiApplication::modalWindow();
    return modalWindow && modalWindow != window();
}

void QIOSWindow::setVisible(bool visible)
{
    QPlatformWindow::setVisible(visible);
    m_view.hidden = !visible;

    if (!isQtApplication())
        return;

    // Since iOS doesn't do window management the way a Qt application
    // expects, we need to raise and activate windows ourselves:
    if (visible)
        updateWindowLevel();

    if (blockedByModal()) {
        if (visible)
            raise();
        return;
    }

    if (visible) {
        requestActivateWindow();
    } else {
        // Activate top-most visible QWindow:
        NSArray *subviews = m_view.viewController.view.subviews;
        for (int i = int(subviews.count) - 1; i >= 0; --i) {
            UIView *view = [subviews objectAtIndex:i];
            if (!view.hidden) {
                if (QWindow *window = view.qwindow) {
                    static_cast<QIOSWindow *>(window->handle())->requestActivateWindow();
                    break;
                }
            }
        }
    }
}

void QIOSWindow::setGeometry(const QRect &rect)
{
    // If the window is in fullscreen, just bookkeep the requested
    // geometry in case the window goes into Qt::WindowNoState later:
    m_normalGeometry = rect;
    if (window()->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen))
        return;

    applyGeometry(rect);
}

void QIOSWindow::applyGeometry(const QRect &rect)
{
    // Since we don't support transformations on the UIView, we can set the frame
    // directly and let UIKit deal with translating that into bounds and center.
    // Changing the size of the view will end up in a call to -[QUIView layoutSubviews]
    // which will update QWindowSystemInterface with the new size.
    m_view.frame = toCGRect(rect);
}

void QIOSWindow::setWindowState(Qt::WindowState state)
{
    // FIXME: Figure out where or how we should disable/enable the statusbar.
    // Perhaps setting QWindow to maximized should also mean that we'll show
    // the statusbar, and vice versa for fullscreen?

    switch (state) {
    case Qt::WindowNoState:
        applyGeometry(m_normalGeometry);
        break;
    case Qt::WindowMaximized:
        applyGeometry(screen()->availableGeometry());
        break;
    case Qt::WindowFullScreen:
        applyGeometry(screen()->geometry());
        break;
    case Qt::WindowMinimized:
        applyGeometry(QRect());
        break;
    case Qt::WindowActive:
        Q_UNREACHABLE();
    default:
        Q_UNREACHABLE();
    }
}

void QIOSWindow::setParent(const QPlatformWindow *parentWindow)
{
    if (parentWindow) {
        UIView *parentView = reinterpret_cast<UIView *>(parentWindow->winId());
        [parentView addSubview:m_view];
    } else if (isQtApplication()) {
        for (UIWindow *uiWindow in [[UIApplication sharedApplication] windows]) {
            if (uiWindow.screen == static_cast<QIOSScreen *>(screen())->uiScreen()) {
                [uiWindow.rootViewController.view addSubview:m_view];
                break;
            }
        }
    }
}

void QIOSWindow::requestActivateWindow()
{
    // Note that several windows can be active at the same time if they exist in the same
    // hierarchy (transient children). But only one window can be QGuiApplication::focusWindow().
    // Dispite the name, 'requestActivateWindow' means raise and transfer focus to the window:
    if (blockedByModal())
        return;

    [m_view.window makeKeyWindow];

    if (window()->isTopLevel())
        raise();

    QPlatformInputContext *context = QGuiApplicationPrivate::platformIntegration()->inputContext();
    static_cast<QIOSInputContext *>(context)->focusViewChanged(m_view);
    QWindowSystemInterface::handleWindowActivated(window());
}

void QIOSWindow::raiseOrLower(bool raise)
{
    // Re-insert m_view at the correct index among its sibling views
    // (QWindows) according to their current m_windowLevel:
    if (!isQtApplication())
        return;

    NSArray *subviews = m_view.superview.subviews;
    if (subviews.count == 1)
        return;

    for (int i = int(subviews.count) - 1; i >= 0; --i) {
        UIView *view = static_cast<UIView *>([subviews objectAtIndex:i]);
        if (view.hidden || view == m_view || !view.qwindow)
            continue;
        int level = static_cast<QIOSWindow *>(view.qwindow->handle())->m_windowLevel;
        if (m_windowLevel > level || (raise && m_windowLevel == level)) {
            [m_view.superview insertSubview:m_view aboveSubview:view];
            return;
        }
    }
    [m_view.superview insertSubview:m_view atIndex:0];
}

void QIOSWindow::updateWindowLevel()
{
    Qt::WindowType type = windowType();

    if (type == Qt::ToolTip)
        m_windowLevel = 120;
    else if (window()->flags() & Qt::WindowStaysOnTopHint)
        m_windowLevel = 100;
    else if (window()->isModal())
        m_windowLevel = 40;
    else if (type == Qt::Popup)
        m_windowLevel = 30;
    else if (type == Qt::SplashScreen)
        m_windowLevel = 20;
    else if (type == Qt::Tool)
        m_windowLevel = 10;
    else
        m_windowLevel = 0;

    // A window should be in at least the same m_windowLevel as its parent:
    QWindow *transientParent = window()->transientParent();
    QIOSWindow *transientParentWindow = transientParent ? static_cast<QIOSWindow *>(transientParent->handle()) : 0;
    if (transientParentWindow)
        m_windowLevel = qMax(transientParentWindow->m_windowLevel, m_windowLevel);
}

void QIOSWindow::handleContentOrientationChange(Qt::ScreenOrientation orientation)
{
    // Keep the status bar in sync with content orientation. This will ensure
    // that the task bar (and associated gestures) are aligned correctly:
    UIInterfaceOrientation uiOrientation = UIInterfaceOrientation(fromQtScreenOrientation(orientation));
    [[UIApplication sharedApplication] setStatusBarOrientation:uiOrientation animated:NO];
}

qreal QIOSWindow::devicePixelRatio() const
{
    return m_view.contentScaleFactor;
}

#include "moc_qioswindow.cpp"

QT_END_NAMESPACE
