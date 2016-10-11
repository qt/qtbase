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

#include "quiview.h"

#include "qiosglobal.h"
#include "qiosintegration.h"
#include "qiosviewcontroller.h"
#include "qiostextresponder.h"
#include "qioswindow.h"
#ifndef Q_OS_TVOS
#include "qiosmenu.h"
#endif

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qwindow_p.h>
#include <qpa/qwindowsysteminterface_p.h>

@implementation QUIView

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (id)initWithQIOSWindow:(QT_PREPEND_NAMESPACE(QIOSWindow) *)window
{
    if (self = [self initWithFrame:window->geometry().toCGRect()])
        m_qioswindow = window;

    m_accessibleElements = [[NSMutableArray alloc] init];
    return self;
}

- (id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        // Set up EAGL layer
        CAEAGLLayer *eaglLayer = static_cast<CAEAGLLayer *>(self.layer);
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithBool:YES], kEAGLDrawablePropertyRetainedBacking,
            kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];

        if (isQtApplication())
            self.hidden = YES;

#ifndef Q_OS_TVOS
        self.multipleTouchEnabled = YES;
#endif

        if (QIOSIntegration::instance()->debugWindowManagement()) {
            static CGFloat hue = 0.0;
            CGFloat lastHue = hue;
            for (CGFloat diff = 0; diff < 0.1 || diff > 0.9; diff = fabs(hue - lastHue))
                hue = drand48();

            #define colorWithBrightness(br) \
                [UIColor colorWithHue:hue saturation:0.5 brightness:br alpha:1.0].CGColor

            self.layer.backgroundColor = colorWithBrightness(0.5);
            self.layer.borderColor = colorWithBrightness(1.0);
            self.layer.borderWidth = 1.0;
        }
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

- (void)setNeedsDisplay
{
    [super setNeedsDisplay];

    // We didn't implement drawRect: so we have to manually
    // mark the layer as needing display.
    [self.layer setNeedsDisplay];
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

    // The original geometry requested by setGeometry() might be different
    // from what we end up with after applying window constraints.
    QRect requestedGeometry = m_qioswindow->geometry();

    QRect actualGeometry = QRectF::fromCGRect(self.frame).toRect();

    // Persist the actual/new geometry so that QWindow::geometry() can
    // be queried on the resize event.
    m_qioswindow->QPlatformWindow::setGeometry(actualGeometry);

    QRect previousGeometry = requestedGeometry != actualGeometry ?
            requestedGeometry : qt_window_private(m_qioswindow->window())->geometry;

    QWindow *window = m_qioswindow->window();
    QWindowSystemInterface::handleGeometryChange<QWindowSystemInterface::SynchronousDelivery>(window, actualGeometry, previousGeometry);

    if (actualGeometry.size() != previousGeometry.size()) {
        // Trigger expose event on resize
        [self setNeedsDisplay];

        // A new size means we also need to resize the FBO's corresponding buffers,
        // but we defer that to when the application calls makeCurrent.
    }
}

- (void)displayLayer:(CALayer *)layer
{
    Q_UNUSED(layer);
    Q_ASSERT(layer == self.layer);

    [self sendUpdatedExposeEvent];
}

- (void)sendUpdatedExposeEvent
{
    QRegion region;

    if (m_qioswindow->isExposed()) {
        QSize bounds = QRectF::fromCGRect(self.layer.bounds).toRect().size();

        Q_ASSERT(m_qioswindow->geometry().size() == bounds);
        Q_ASSERT(self.hidden == !m_qioswindow->window()->isVisible());

        region = QRect(QPoint(), bounds);
    }

    QWindowSystemInterface::handleExposeEvent<QWindowSystemInterface::SynchronousDelivery>(m_qioswindow->window(), region);
}

// -------------------------------------------------------------------------

- (BOOL)canBecomeFirstResponder
{
    return !(m_qioswindow->window()->flags() & Qt::WindowDoesNotAcceptFocus);
}

- (BOOL)becomeFirstResponder
{
    FirstResponderCandidate firstResponderCandidate(self);

    qImDebug() << "win:" << m_qioswindow->window() << "self:" << self
        << "first:" << [UIResponder currentFirstResponder];

    if (![super becomeFirstResponder]) {
        qImDebug() << m_qioswindow->window()
            << "was not allowed to become first responder";
        return NO;
    }

    qImDebug() << m_qioswindow->window() << "became first responder";

    if (qGuiApp->focusWindow() != m_qioswindow->window())
        QWindowSystemInterface::handleWindowActivated<QWindowSystemInterface::SynchronousDelivery>(m_qioswindow->window());
    else
        qImDebug() << m_qioswindow->window() << "already active, not sending window activation";

    return YES;
}

- (BOOL)responderShouldTriggerWindowDeactivation:(UIResponder *)responder
{
    // We don't want to send window deactivation in case the resign
    // was a result of another Qt window becoming first responder.
    if ([responder isKindOfClass:[QUIView class]])
        return NO;

    // Nor do we want to deactivate the Qt window if the new responder
    // is temporarily handling text input on behalf of a Qt window.
    if ([responder isKindOfClass:[QIOSTextInputResponder class]]) {
        while ((responder = [responder nextResponder])) {
            if ([responder isKindOfClass:[QUIView class]])
                return NO;
        }
    }

    return YES;
}

- (BOOL)resignFirstResponder
{
    qImDebug() << "win:" << m_qioswindow->window() << "self:" << self
        << "first:" << [UIResponder currentFirstResponder];

    if (![super resignFirstResponder])
        return NO;

    qImDebug() << m_qioswindow->window() << "resigned first responder";

    UIResponder *newResponder = FirstResponderCandidate::currentCandidate();
    if ([self responderShouldTriggerWindowDeactivation:newResponder])
        QWindowSystemInterface::handleWindowActivated<QWindowSystemInterface::SynchronousDelivery>(0);

    return YES;
}

- (BOOL)isActiveWindow
{
    // Normally this is determined exclusivly by being firstResponder, but
    // since we employ a separate first responder for text input we need to
    // handle both cases as this view being the active Qt window.

    if ([self isFirstResponder])
        return YES;

    UIResponder *firstResponder = [UIResponder currentFirstResponder];
    if ([firstResponder isKindOfClass:[QIOSTextInputResponder class]]
        && [firstResponder nextResponder] == self)
        return YES;

    return NO;
}

// -------------------------------------------------------------------------

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
    [super traitCollectionDidChange: previousTraitCollection];

    QTouchDevice *touchDevice = QIOSIntegration::instance()->touchDevice();
    QTouchDevice::Capabilities touchCapabilities = touchDevice->capabilities();

    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_9_0) {
        if (self.traitCollection.forceTouchCapability == UIForceTouchCapabilityAvailable)
            touchCapabilities |= QTouchDevice::Pressure;
        else
            touchCapabilities &= ~QTouchDevice::Pressure;
    }

    touchDevice->setCapabilities(touchCapabilities);
}

-(BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
    if (m_qioswindow->window()->flags() & Qt::WindowTransparentForInput)
        return NO;
    return [super pointInside:point withEvent:event];
}

- (void)updateTouchList:(NSSet *)touches withState:(Qt::TouchPointState)state
{
    bool supportsPressure = QIOSIntegration::instance()->touchDevice()->capabilities() & QTouchDevice::Pressure;

    foreach (UITouch *uiTouch, m_activeTouches.keys()) {
        QWindowSystemInterface::TouchPoint &touchPoint = m_activeTouches[uiTouch];
        if (![touches containsObject:uiTouch]) {
            touchPoint.state = Qt::TouchPointStationary;
        } else {
            touchPoint.state = state;

            // Touch positions are expected to be in QScreen global coordinates, and
            // as we already have the QWindow positioned at the right place, we can
            // just map from the local view position to global coordinates.
            // tvOS: all touches start at the center of the screen and move from there.
            QPoint localViewPosition = QPointF::fromCGPoint([uiTouch locationInView:self]).toPoint();
            QPoint globalScreenPosition = m_qioswindow->mapToGlobal(localViewPosition);

            touchPoint.area = QRectF(globalScreenPosition, QSize(0, 0));

            // FIXME: Do we really need to support QTouchDevice::NormalizedPosition?
            QSize screenSize = m_qioswindow->screen()->geometry().size();
            touchPoint.normalPosition = QPointF(globalScreenPosition.x() / screenSize.width(),
                                                globalScreenPosition.y() / screenSize.height());

            if (supportsPressure) {
                // Note: iOS  will deliver touchesBegan with a touch force of 0, which
                // we will reflect/propagate as a 0 pressure, but there is no clear
                // alternative, as we don't want to wait for a touchedMoved before
                // sending a touch press event to Qt, just to have a valid pressure.
                touchPoint.pressure = uiTouch.force / uiTouch.maximumPossibleForce;
            } else {
                // We don't claim that our touch device supports QTouchDevice::Pressure,
                // but fill in a meaningfull value in case clients use it anyways.
                touchPoint.pressure = (state == Qt::TouchPointReleased) ? 0.0 : 1.0;
            }
        }
    }
}

- (void)sendTouchEventWithTimestamp:(ulong)timeStamp
{
    QIOSIntegration *iosIntegration = QIOSIntegration::instance();
    QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::SynchronousDelivery>(m_qioswindow->window(), timeStamp, iosIntegration->touchDevice(), m_activeTouches.values());
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

    if (m_qioswindow->shouldAutoActivateWindow() && m_activeTouches.size() == 1) {
        QPlatformWindow *topLevel = m_qioswindow;
        while (QPlatformWindow *p = topLevel->parent())
            topLevel = p;
        if (topLevel->window() != QGuiApplication::focusWindow())
            topLevel->requestActivateWindow();
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
    if (m_activeTouches.isEmpty())
        return;

    // When four-finger swiping, we get a touchesCancelled callback
    // which includes all four touch points. The swipe gesture is
    // then active until all four touches have been released, and
    // we start getting touchesBegan events again.

    // When five-finger pinching, we also get a touchesCancelled
    // callback with all five touch points, but the pinch gesture
    // ends when the second to last finger is released from the
    // screen. The last finger will not emit any more touch
    // events, _but_, will contribute to starting another pinch
    // gesture. That second pinch gesture will _not_ trigger a
    // touchesCancelled event when starting, but as each finger
    // is released, and we may get touchesMoved events for the
    // remaining fingers. [event allTouches] also contains one
    // less touch point than it should, so this behavior is
    // likely a bug in the iOS system gesture recognizer, but we
    // have to take it into account when maintaining the Qt state.
    // We do this by assuming that there are no cases where a
    // sub-set of the active touch events are intentionally cancelled.

    NSInteger count = static_cast<NSInteger>([touches count]);
    if (count != 0 && count != m_activeTouches.count())
        qWarning("Subset of active touches cancelled by UIKit");

    m_activeTouches.clear();
    m_nextTouchId = 0;

    NSTimeInterval timestamp = event ? event.timestamp : [[NSProcessInfo processInfo] systemUptime];

    QIOSIntegration *iosIntegration = static_cast<QIOSIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QWindowSystemInterface::handleTouchCancelEvent<QWindowSystemInterface::SynchronousDelivery>(m_qioswindow->window(), ulong(timestamp * 1000), iosIntegration->touchDevice());
}

- (int)mapPressTypeToKey:(UIPress*)press
{
    switch (press.type) {
    case UIPressTypeUpArrow: return Qt::Key_Up;
    case UIPressTypeDownArrow: return Qt::Key_Down;
    case UIPressTypeLeftArrow: return Qt::Key_Left;
    case UIPressTypeRightArrow: return Qt::Key_Right;
    case UIPressTypeSelect: return Qt::Key_Select;
    case UIPressTypeMenu: return Qt::Key_Menu;
    case UIPressTypePlayPause: return Qt::Key_MediaTogglePlayPause;
    }
    return Qt::Key_unknown;
}

- (bool)processPresses:(NSSet *)presses withType:(QEvent::Type)type {
    // Presses on Menu button will generate a Menu key event. By default, not handling
    // this event will cause the application to return to Headboard (tvOS launcher).
    // When handling the event (for example, as a back button), both press and
    // release events must be handled accordingly.

    bool handled = false;
    for (UIPress* press in presses) {
        int key = [self mapPressTypeToKey:press];
        if (key == Qt::Key_unknown)
            continue;
        if (QWindowSystemInterface::handleKeyEvent<QWindowSystemInterface::SynchronousDelivery>(m_qioswindow->window(), type, key, Qt::NoModifier))
            handled = true;
    }

    return handled;
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event
{
    if (![self processPresses:presses withType:QEvent::KeyPress])
        [super pressesBegan:presses withEvent:event];
}

- (void)pressesChanged:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event
{
    if (![self processPresses:presses withType:QEvent::KeyPress])
        [super pressesChanged:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event
{
    if (![self processPresses:presses withType:QEvent::KeyRelease])
        [super pressesEnded:presses withEvent:event];
}

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender
{
#ifndef Q_OS_TVOS
    // Check first if QIOSMenu should handle the action before continuing up the responder chain
    return [QIOSMenu::menuActionTarget() targetForAction:action withSender:sender] != 0;
#else
    Q_UNUSED(action)
    Q_UNUSED(sender)
    return false;
#endif
}

- (id)forwardingTargetForSelector:(SEL)selector
{
    Q_UNUSED(selector)
#ifndef Q_OS_TVOS
    return QIOSMenu::menuActionTarget();
#else
    return nil;
#endif
}

@end

@implementation UIView (QtHelpers)

- (QWindow *)qwindow
{
    if ([self isKindOfClass:[QUIView class]]) {
        if (QT_PREPEND_NAMESPACE(QIOSWindow) *w = static_cast<QUIView *>(self)->m_qioswindow)
            return w->window();
    }
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

- (QIOSViewController*)qtViewController
{
    UIViewController *vc = self.viewController;
    if ([vc isKindOfClass:QIOSViewController.class])
        return static_cast<QIOSViewController *>(vc);

    return nil;
}

@end

#ifndef QT_NO_ACCESSIBILITY
// Include category as an alternative to using -ObjC (Apple QA1490)
#include "quiview_accessibility.mm"
#endif
