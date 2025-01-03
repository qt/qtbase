// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "quiview.h"

#include "qiosglobal.h"
#include "qiosintegration.h"
#include "qiosviewcontroller.h"
#include "qiostextresponder.h"
#include "qiosscreen.h"
#include "qioswindow.h"
#include "qiosinputcontext.h"
#ifndef Q_OS_TVOS
#include "qiosmenu.h"
#endif

#include <QtCore/qmath.h>
#include <QtGui/qpointingdevice.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qwindow_p.h>
#include <QtGui/private/qapplekeymapper_p.h>
#include <qpa/qwindowsysteminterface_p.h>

Q_LOGGING_CATEGORY(lcQpaTablet, "qt.qpa.input.tablet")
Q_LOGGING_CATEGORY(lcQpaInputEvents, "qt.qpa.input.events")

namespace {
inline ulong getTimeStamp(UIEvent *event)
{
#if TARGET_OS_SIMULATOR == 1
    // We currently build Qt for simulator using X86_64, even on ARM based macs.
    // This results in the simulator running on ARM, while the app is running
    // inside it using Rosetta. And with this combination, the event.timestamp, which is
    // documented to be in seconds, looks to be something else, and is not progressing
    // in sync with a normal clock.
    // Sending out mouse events with a timestamp that doesn't follow normal clock time
    // will cause problems for mouse-, and pointer handlers that uses them to e.g calculate
    // the time between a press and release, and to decide if the user is performing a tap
    // or a drag.
    // For that reason, we choose to ignore UIEvent.timestamp under the mentioned condition, and
    // instead rely on NSProcessInfo. Note that if we force the whole simulator to use Rosetta
    // (and not only the Qt app), the timestamps will progress normally.
#if defined(Q_PROCESSOR_ARM)
    #warning The timestamp work-around for x86_64 can (probably) be removed when building for ARM
#endif
    return ulong(NSProcessInfo.processInfo.systemUptime * 1000);
#endif

    return ulong(event.timestamp * 1000);
}
}

@implementation QUIView {
    QHash<NSUInteger, QWindowSystemInterface::TouchPoint> m_activeTouches;
    UITouch *m_activePencilTouch;
    NSMutableArray<UIAccessibilityElement *> *m_accessibleElements;
    UIPanGestureRecognizer *m_scrollGestureRecognizer;
    CGPoint m_lastScrollCursorPos;
    CGPoint m_lastScrollDelta;
}

+ (void)load
{
#ifndef Q_OS_TVOS
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion(QOperatingSystemVersion::IOS, 11)) {
        // iOS 11 handles this though [UIView safeAreaInsetsDidChange], but there's no signal for
        // the corresponding top and bottom layout guides that we use on earlier versions. Note
        // that we use the _will_ change version of the notification, because we want to react
        // to the change as early was possible. But since the top and bottom layout guides have
        // not been updated at this point we use asynchronous delivery of the event, so that the
        // event is processed by QtGui just after iOS has updated the layout margins.
        [[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationWillChangeStatusBarFrameNotification
            object:nil queue:[NSOperationQueue mainQueue] usingBlock:^(NSNotification *) {
                for (QWindow *window : QGuiApplication::allWindows())
                    QWindowSystemInterface::handleSafeAreaMarginsChanged<QWindowSystemInterface::AsynchronousDelivery>(window);
            }
        ];
    }
#endif
}

+ (Class)layerClass
{
#if QT_CONFIG(opengl)
    return [CAEAGLLayer class];
#endif
    return [super layerClass];
}

- (instancetype)initWithQIOSWindow:(QT_PREPEND_NAMESPACE(QIOSWindow) *)window
{
    if (self = [self initWithFrame:window->geometry().toCGRect()]) {
        self.platformWindow = window;
        m_accessibleElements = [[NSMutableArray<UIAccessibilityElement *> alloc] init];
        m_scrollGestureRecognizer = [[UIPanGestureRecognizer alloc]
                                      initWithTarget:self
                                      action:@selector(handleScroll:)];
        // The gesture recognizer should only care about scroll gestures (for now)
        // Set allowedTouchTypes to empty array here to not interfere with touch events
        // handled by the UIView. Scroll gestures, even those coming from touch devices,
        // such as trackpads will still be received as they are not touch events
        m_scrollGestureRecognizer.allowedTouchTypes = [NSArray array];
        if (@available(ios 13.4, *)) {
            m_scrollGestureRecognizer.allowedScrollTypesMask = UIScrollTypeMaskAll;
        }
        m_scrollGestureRecognizer.maximumNumberOfTouches = 0;
        m_lastScrollDelta = CGPointZero;
        m_lastScrollCursorPos = CGPointZero;
        [self addGestureRecognizer:m_scrollGestureRecognizer];

        if ([self.layer isKindOfClass:CAMetalLayer.class]) {
            QWindow *window = self.platformWindow->window();
            if (QColorSpace colorSpace = window->format().colorSpace(); colorSpace.isValid()) {
                QCFType<CFDataRef> iccData = colorSpace.iccProfile().toCFData();
                QCFType<CGColorSpaceRef> cgColorSpace = CGColorSpaceCreateWithICCData(iccData);
                CAMetalLayer *metalLayer = static_cast<CAMetalLayer *>(self.layer);
                metalLayer.colorspace = cgColorSpace;
                qCDebug(lcQpaWindow) << "Set" << self << "color space to" << metalLayer.colorspace;
            }
        }
    }

    return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
#if QT_CONFIG(opengl)
        if ([self.layer isKindOfClass:[CAEAGLLayer class]]) {
            // Set up EAGL layer
            CAEAGLLayer *eaglLayer = static_cast<CAEAGLLayer *>(self.layer);
            eaglLayer.opaque = TRUE;
            eaglLayer.drawableProperties = @{
                kEAGLDrawablePropertyRetainedBacking: @(YES),
                kEAGLDrawablePropertyColorFormat: kEAGLColorFormatRGBA8
            };
        }
#endif

        if (isQtApplication())
            self.hidden = YES;

#ifndef Q_OS_TVOS
        self.multipleTouchEnabled = YES;
#endif

        if (qEnvironmentVariableIntValue("QT_IOS_DEBUG_WINDOW_MANAGEMENT")) {
            static CGFloat hue = 0.0;
            CGFloat lastHue = hue;
            for (CGFloat diff = 0; diff < 0.1 || diff > 0.9; diff = fabs(hue - lastHue))
                hue = drand48();

            #define colorWithBrightness(br) \
                [UIColor colorWithHue:hue saturation:0.5 brightness:br alpha:1.0].CGColor

            self.layer.borderColor = colorWithBrightness(1.0);
            self.layer.borderWidth = 1.0;
        }

        if (qEnvironmentVariableIsSet("QT_IOS_DEBUG_WINDOW_SAFE_AREAS")) {
            UIView *safeAreaOverlay = [[UIView alloc] initWithFrame:CGRectZero];
            [safeAreaOverlay setBackgroundColor:[UIColor colorWithRed:0.3 green:0.7 blue:0.9 alpha:0.3]];
            [self addSubview:safeAreaOverlay];

            safeAreaOverlay.translatesAutoresizingMaskIntoConstraints = NO;
            [safeAreaOverlay.topAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.topAnchor].active = YES;
            [safeAreaOverlay.leftAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.leftAnchor].active = YES;
            [safeAreaOverlay.rightAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.rightAnchor].active = YES;
            [safeAreaOverlay.bottomAnchor constraintEqualToAnchor:self.safeAreaLayoutGuide.bottomAnchor].active = YES;
        }
    }

    return self;
}

- (void)dealloc
{
    [m_accessibleElements release];
    [m_scrollGestureRecognizer release];

    [super dealloc];
}

- (NSString *)description
{
    NSMutableString *description = [NSMutableString stringWithString:[super description]];

#ifndef QT_NO_DEBUG_STREAM
    QString platformWindowDescription;
    QDebug debug(&platformWindowDescription);
    debug.nospace() << "; " << self.platformWindow << ">";
    NSRange lastCharacter = [description rangeOfComposedCharacterSequenceAtIndex:description.length - 1];
    [description replaceCharactersInRange:lastCharacter withString:platformWindowDescription.toNSString()];
#endif

    return description;
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
        qWarning() << self << "has a transform set. This is not supported.";

    QWindow *window = self.platformWindow->window();
    QRect lastReportedGeometry = qt_window_private(window)->geometry;
    QRect currentGeometry = QRectF::fromCGRect(self.frame).toRect();
    qCDebug(lcQpaWindow) << self.platformWindow << "new geometry is" << currentGeometry;
    QWindowSystemInterface::handleGeometryChange(window, currentGeometry);

    if (currentGeometry.size() != lastReportedGeometry.size()) {
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

    if (self.platformWindow->isExposed()) {
        QSize bounds = QRectF::fromCGRect(self.layer.bounds).toRect().size();

        Q_ASSERT(self.platformWindow->geometry().size() == bounds);
        Q_ASSERT(self.hidden == !self.platformWindow->window()->isVisible());

        region = QRect(QPoint(), bounds);
    }

    qCDebug(lcQpaWindow) << self.platformWindow << region << "isExposed" << self.platformWindow->isExposed();
    QWindowSystemInterface::handleExposeEvent(self.platformWindow->window(), region);
}

- (void)safeAreaInsetsDidChange
{
    QWindowSystemInterface::handleSafeAreaMarginsChanged(self.platformWindow->window());
}

// -------------------------------------------------------------------------

- (BOOL)canBecomeFirstResponder
{
    return !(self.platformWindow->window()->flags() & (Qt::WindowDoesNotAcceptFocus
                                                     | Qt::WindowTransparentForInput));
}

- (BOOL)becomeFirstResponder
{
    {
        // Scope for the duration of becoming first responder only, as the window
        // activation event may trigger new responders, which we don't want to be
        // blocked by this guard.
        FirstResponderCandidate firstResponderCandidate(self);

        qImDebug() << "self:" << self << "first:" << [UIResponder currentFirstResponder];

        if (![super becomeFirstResponder]) {
            qImDebug() << self << "was not allowed to become first responder";
            return NO;
        }

        qImDebug() << self << "became first responder";
    }

    if (qGuiApp->focusWindow() != self.platformWindow->window())
        QWindowSystemInterface::handleWindowActivated(self.platformWindow->window(), Qt::ActiveWindowFocusReason);
    else
        qImDebug() << self.platformWindow->window() << "already active, not sending window activation";

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
    if ([responder isKindOfClass:[QIOSTextResponder class]]) {
        while ((responder = [responder nextResponder])) {
            if ([responder isKindOfClass:[QUIView class]])
                return NO;
        }
    }

    return YES;
}

- (BOOL)resignFirstResponder
{
    qImDebug() << "self:" << self << "first:" << [UIResponder currentFirstResponder];

    if (![super resignFirstResponder])
        return NO;

    qImDebug() << self << "resigned first responder";

    UIResponder *newResponder = FirstResponderCandidate::currentCandidate();
    if ([self responderShouldTriggerWindowDeactivation:newResponder])
        QWindowSystemInterface::handleWindowActivated(nullptr, Qt::ActiveWindowFocusReason);

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

    QPointingDevice *touchDevice = QIOSIntegration::instance()->touchDevice();
    QPointingDevice::Capabilities touchCapabilities = touchDevice->capabilities();

    touchCapabilities.setFlag(QPointingDevice::Capability::Pressure,
                              (self.traitCollection.forceTouchCapability == UIForceTouchCapabilityAvailable));

    touchDevice->setCapabilities(touchCapabilities);
}

-(BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event
{
    if (self.platformWindow->window()->flags() & Qt::WindowTransparentForInput)
        return NO;
    return [super pointInside:point withEvent:event];
}

- (void)handleTouches:(NSSet *)touches withEvent:(UIEvent *)event withState:(QEventPoint::State)state withTimestamp:(ulong)timeStamp
{
    QIOSIntegration *iosIntegration = QIOSIntegration::instance();
    bool supportsPressure = QIOSIntegration::instance()->touchDevice()->capabilities() & QPointingDevice::Capability::Pressure;

#if QT_CONFIG(tabletevent)
    if (m_activePencilTouch && [touches containsObject:m_activePencilTouch]) {
        NSArray<UITouch *> *cTouches = [event coalescedTouchesForTouch:m_activePencilTouch];
        int i = 0;
        for (UITouch *cTouch in cTouches) {
            QPointF localViewPosition = QPointF::fromCGPoint([cTouch preciseLocationInView:self]);
            QPoint localViewPositionI = localViewPosition.toPoint();
            QPointF globalScreenPosition = self.platformWindow->mapToGlobal(localViewPositionI) +
                    (localViewPosition - localViewPositionI);
            qreal pressure = cTouch.force / cTouch.maximumPossibleForce;
            // azimuth unit vector: +x to the right, +y going downwards
            CGVector azimuth = [cTouch azimuthUnitVectorInView: self];
            // azimuthAngle given in radians, zero when the stylus points towards +x axis; converted to degrees with 0 pointing straight up
            qreal azimuthAngle = qRadiansToDegrees([cTouch azimuthAngleInView: self]) + 90;
            // altitudeAngle given in radians, pi / 2 is with the stylus perpendicular to the iPad, smaller values mean more tilted, but never negative.
            // Convert to degrees with zero being perpendicular.
            qreal altitudeAngle = 90 - qRadiansToDegrees(cTouch.altitudeAngle);
            qCDebug(lcQpaTablet) << i << ":" << timeStamp << localViewPosition << pressure << state << "azimuth" << azimuth.dx << azimuth.dy
                     << "angle" << azimuthAngle << "altitude" << cTouch.altitudeAngle
                     << "xTilt" << qBound(-60.0, altitudeAngle * azimuth.dx, 60.0) << "yTilt" << qBound(-60.0, altitudeAngle * azimuth.dy, 60.0);
            QWindowSystemInterface::handleTabletEvent(self.platformWindow->window(), timeStamp, localViewPosition, globalScreenPosition,
                    // device, pointerType, buttons
                    int(QInputDevice::DeviceType::Stylus), int(QPointingDevice::PointerType::Pen), state == QEventPoint::State::Released ? Qt::NoButton : Qt::LeftButton,
                    // pressure, xTilt, yTilt
                    pressure, qBound(-60.0, altitudeAngle * azimuth.dx, 60.0), qBound(-60.0, altitudeAngle * azimuth.dy, 60.0),
                    // tangentialPressure, rotation, z, uid, modifiers
                    0, azimuthAngle, 0, 0, Qt::NoModifier);
            ++i;
        }
    }
#endif

    if (m_activeTouches.isEmpty())
        return;
    for (auto it = m_activeTouches.begin(); it != m_activeTouches.end(); ++it) {
        auto hash = it.key();
        QWindowSystemInterface::TouchPoint &touchPoint = it.value();
        UITouch *uiTouch = nil;
        for (UITouch *touch in touches) {
            if (touch.hash == hash) {
                uiTouch = touch;
                break;
            }
        }
        if (!uiTouch) {
            touchPoint.state = QEventPoint::State::Stationary;
        } else {
            touchPoint.state = state;

            // Touch positions are expected to be in QScreen global coordinates, and
            // as we already have the QWindow positioned at the right place, we can
            // just map from the local view position to global coordinates.
            // tvOS: all touches start at the center of the screen and move from there.
            QPoint localViewPosition = QPointF::fromCGPoint([uiTouch locationInView:self]).toPoint();
            QPoint globalScreenPosition = self.platformWindow->mapToGlobal(localViewPosition);

            touchPoint.area = QRectF(globalScreenPosition, QSize(0, 0));

            // FIXME: Do we really need to support QPointingDevice::Capability::NormalizedPosition?
            QSize screenSize = self.platformWindow->screen()->geometry().size();
            touchPoint.normalPosition = QPointF(globalScreenPosition.x() / screenSize.width(),
                                                globalScreenPosition.y() / screenSize.height());

            if (supportsPressure) {
                // Note: iOS  will deliver touchesBegan with a touch force of 0, which
                // we will reflect/propagate as a 0 pressure, but there is no clear
                // alternative, as we don't want to wait for a touchedMoved before
                // sending a touch press event to Qt, just to have a valid pressure.
                touchPoint.pressure = uiTouch.force / uiTouch.maximumPossibleForce;
            } else {
                // We don't claim that our touch device supports QPointingDevice::Capability::Pressure,
                // but fill in a meaningful value in case clients use it anyway.
                touchPoint.pressure = (state == QEventPoint::State::Released) ? 0.0 : 1.0;
            }
        }
    }

    if ([self.window isKindOfClass:[QUIWindow class]] &&
            !static_cast<QUIWindow *>(self.window).sendingEvent) {
        // The event is likely delivered as part of delayed touch delivery, via
        // _UIGestureEnvironmentSortAndSendDelayedTouches, due to one of the two
        // _UISystemGestureGateGestureRecognizer instances on the top level window
        // having its delaysTouchesBegan set to YES. During this delivery, it's not
        // safe to spin up a recursive event loop, as our calling function is not
        // reentrant, so any gestures used by the recursive code, e.g. a native
        // alert dialog, will fail to recognize. To be on the safe side, we deliver
        // the event asynchronously.
        QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::AsynchronousDelivery>(
            self.platformWindow->window(), timeStamp, iosIntegration->touchDevice(), m_activeTouches.values());
    } else {
        // Send the touch event asynchronously, as the application might spin a recursive
        // event loop in response to the touch event (a dialog e.g.), which will deadlock
        // the UIKit event delivery system (QTBUG-98651).
        QWindowSystemInterface::handleTouchEvent<QWindowSystemInterface::AsynchronousDelivery>(
            self.platformWindow->window(), timeStamp, iosIntegration->touchDevice(), m_activeTouches.values());
    }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    // UIKit generates [Began -> Moved -> Ended] event sequences for
    // each touch point. Internally we keep a hashmap of active UITouch
    // points to QWindowSystemInterface::TouchPoints, and assigns each TouchPoint
    // an id for use by Qt.
    for (UITouch *touch in touches) {
#if QT_CONFIG(tabletevent)
        if (touch.type == UITouchTypeStylus) {
            if (Q_UNLIKELY(m_activePencilTouch)) {
                qWarning("ignoring additional Pencil while first is still active");
                continue;
            }
            m_activePencilTouch = touch;
        } else
        {
            Q_ASSERT(!m_activeTouches.contains(touch.hash));
#endif
            // Use window-independent touch identifiers, so that
            // multi-touch works across windows.
            static quint16 nextTouchId = 0;
            m_activeTouches[touch.hash].id = nextTouchId++;
#if QT_CONFIG(tabletevent)
        }
#endif
    }

    if (self.platformWindow->shouldAutoActivateWindow() && m_activeTouches.size() == 1) {
        QPlatformWindow *topLevel = self.platformWindow;
        while (QPlatformWindow *p = topLevel->parent())
            topLevel = p;
        if (topLevel->window() != QGuiApplication::focusWindow())
            topLevel->requestActivateWindow();
    }

    [self handleTouches:touches withEvent:event withState:QEventPoint::State::Pressed withTimestamp:getTimeStamp(event)];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self handleTouches:touches withEvent:event withState:QEventPoint::State::Updated withTimestamp:getTimeStamp(event)];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self handleTouches:touches withEvent:event withState:QEventPoint::State::Released withTimestamp:getTimeStamp(event)];

    // Remove ended touch points from the active set:
#ifndef Q_OS_TVOS
    for (UITouch *touch in touches) {
#if QT_CONFIG(tabletevent)
        if (touch.type == UITouchTypeStylus) {
            m_activePencilTouch = nil;
        } else
#endif
        {
            m_activeTouches.remove(touch.hash);
        }
    }
#else
    // tvOS only supports single touch
    m_activeTouches.clear();
#endif
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (m_activeTouches.isEmpty() && !m_activePencilTouch)
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
    if (count != 0 && count != m_activeTouches.count() && !m_activePencilTouch)
        qWarning("Subset of active touches cancelled by UIKit");

    m_activeTouches.clear();
    m_activePencilTouch = nil;

    ulong timestamp = event ? getTimeStamp(event) : ([[NSProcessInfo processInfo] systemUptime] * 1000);

    QIOSIntegration *iosIntegration = static_cast<QIOSIntegration *>(QGuiApplicationPrivate::platformIntegration());

    // Send the touch event asynchronously, as the application might spin a recursive
    // event loop in response to the touch event (a dialog e.g.), which will deadlock
    // the UIKit event delivery system (QTBUG-98651).
    QWindowSystemInterface::handleTouchCancelEvent<QWindowSystemInterface::AsynchronousDelivery>(
        self.platformWindow->window(), timestamp, iosIntegration->touchDevice());
}

- (int)mapPressTypeToKey:(UIPress*)press withModifiers:(Qt::KeyboardModifiers)qtModifiers text:(QString &)text
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
    if (@available(ios 13.4, *)) {
        NSString *charactersIgnoringModifiers = press.key.charactersIgnoringModifiers;
        Qt::Key key = QAppleKeyMapper::fromUIKitKey(charactersIgnoringModifiers);
        if (key != Qt::Key_unknown)
            return key;
        return QAppleKeyMapper::fromNSString(qtModifiers, press.key.characters,
                                             charactersIgnoringModifiers, text);
    }
    return Qt::Key_unknown;
}

- (bool)isControlKey:(Qt::Key)key
{
    switch (key) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Left:
    case Qt::Key_Right:
        return true;
    default:
        break;
    }

    return false;
}

- (bool)handlePresses:(NSSet<UIPress *> *)presses eventType:(QEvent::Type)type
{
    // Presses on Menu button will generate a Menu key event. By default, not handling
    // this event will cause the application to return to Headboard (tvOS launcher).
    // When handling the event (for example, as a back button), both press and
    // release events must be handled accordingly.
    if (!qApp->focusWindow())
        return false;

    bool eventHandled = false;
    const bool imEnabled = QIOSInputContext::instance()->inputMethodAccepted();

    for (UIPress* press in presses) {
        Qt::KeyboardModifiers qtModifiers = Qt::NoModifier;
        if (@available(ios 13.4, *))
            qtModifiers = QAppleKeyMapper::fromUIKitModifiers(press.key.modifierFlags);
        QString text;
        int key = [self mapPressTypeToKey:press withModifiers:qtModifiers text:text];
        if (key == Qt::Key_unknown)
            continue;
        if (imEnabled && ![self isControlKey:Qt::Key(key)])
            continue;

        bool keyHandled = QWindowSystemInterface::handleKeyEvent(
                    self.platformWindow->window(), type, key, qtModifiers, text);
        eventHandled = eventHandled || keyHandled;
    }

    return eventHandled;
}

- (void)pressesBegan:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event
{
    if (![self handlePresses:presses eventType:QEvent::KeyPress])
        [super pressesBegan:presses withEvent:event];
}

- (void)pressesChanged:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event
{
    if (![self handlePresses:presses eventType:QEvent::KeyPress])
        [super pressesChanged:presses withEvent:event];
    [super pressesChanged:presses withEvent:event];
}

- (void)pressesEnded:(NSSet<UIPress *> *)presses withEvent:(UIPressesEvent *)event
{
    if (![self handlePresses:presses eventType:QEvent::KeyRelease])
        [super pressesEnded:presses withEvent:event];
    [super pressesEnded:presses withEvent:event];
}

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender
{
#ifndef Q_OS_TVOS
    // Check first if QIOSMenu should handle the action before continuing up the responder chain
    return [QIOSMenu::menuActionTarget() targetForAction:action withSender:sender] != 0;
#else
    Q_UNUSED(action);
    Q_UNUSED(sender);
    return false;
#endif
}

- (id)forwardingTargetForSelector:(SEL)selector
{
    Q_UNUSED(selector);
#ifndef Q_OS_TVOS
    return QIOSMenu::menuActionTarget();
#else
    return nil;
#endif
}

- (void)addInteraction:(id<UIInteraction>)interaction
{
    if ([NSStringFromClass(interaction.class) isEqualToString:@"UITextInteraction"])
        return;

    [super addInteraction:interaction];
}

- (UIEditingInteractionConfiguration)editingInteractionConfiguration
{
    // We only want the three-finger-tap edit menu to be available when there's
    // actually something to edit. Otherwise the OS will cause a slight delay
    // before delivering the release of three finger touch input. Note that we
    // do not do any hit testing here to check that the focus object is the one
    // being tapped, as the behavior of native iOS apps is to trigger the menu
    // regardless of where the gesture is being made.
    return QIOSInputContext::instance()->inputMethodAccepted() ?
        UIEditingInteractionConfigurationDefault : UIEditingInteractionConfigurationNone;
}

#if QT_CONFIG(wheelevent)
- (void)handleScroll:(UIPanGestureRecognizer *)recognizer
{
    if (!self.platformWindow->window())
        return;

    if (!self.canBecomeFirstResponder)
        return;

    CGPoint translation = [recognizer translationInView:self];
    CGFloat deltaX = translation.x - m_lastScrollDelta.x;
    CGFloat deltaY = translation.y - m_lastScrollDelta.y;

    QPoint angleDelta;
    // From QNSView implementation:
    // "Since deviceDelta is delivered as pixels rather than degrees, we need to
    // convert from pixels to degrees in a sensible manner.
    // It looks like 1/4 degrees per pixel behaves most native.
    // (NB: Qt expects the unit for delta to be 8 per degree):"
    const int pixelsToDegrees = 2; // 8 * 1/4
    angleDelta.setX(deltaX * pixelsToDegrees);
    angleDelta.setY(deltaY * pixelsToDegrees);

    QPoint pixelDelta;
    pixelDelta.setX(deltaX);
    pixelDelta.setY(deltaY);

    NSTimeInterval time_stamp = [[NSProcessInfo processInfo] systemUptime];
    ulong qt_timestamp = time_stamp * 1000;

    Qt::KeyboardModifiers qt_modifierFlags = Qt::NoModifier;
    if (@available(ios 13.4, *))
        qt_modifierFlags = QAppleKeyMapper::fromUIKitModifiers(recognizer.modifierFlags);

    if (recognizer.state == UIGestureRecognizerStateBegan)
        // locationInView: doesn't return the cursor position at the time of the wheel event,
        // but rather gives us the position with the deltas applied, so we need to save the
        // cursor position at the beginning of the gesture
        m_lastScrollCursorPos = [recognizer locationInView:self];

    if (recognizer.state != UIGestureRecognizerStateEnded) {
        m_lastScrollDelta.x = translation.x;
        m_lastScrollDelta.y = translation.y;
    } else {
        m_lastScrollDelta = CGPointZero;
    }

    QPoint qt_local = QPointF::fromCGPoint(m_lastScrollCursorPos).toPoint();
    QPoint qt_global = self.platformWindow->mapToGlobal(qt_local);

    qCInfo(lcQpaInputEvents).nospace() << "wheel event" << " at " << qt_local
    << " pixelDelta=" << pixelDelta << " angleDelta=" << angleDelta;

    QWindowSystemInterface::handleWheelEvent(self.platformWindow->window(), qt_timestamp, qt_local, qt_global, pixelDelta, angleDelta, qt_modifierFlags);
}
#endif // QT_CONFIG(wheelevent)

@end

@implementation UIView (QtHelpers)

- (QWindow *)qwindow
{
    if ([self isKindOfClass:[QUIView class]]) {
        if (QT_PREPEND_NAMESPACE(QIOSWindow) *w = static_cast<QUIView *>(self).platformWindow)
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

- (UIEdgeInsets)qt_safeAreaInsets
{
    return self.safeAreaInsets;
}

@end

#ifdef Q_OS_IOS
@implementation QUIMetalView

+ (Class)layerClass
{
    return [CAMetalLayer class];
}

@end
#endif

#if QT_CONFIG(accessibility)
// Include category as an alternative to using -ObjC (Apple QA1490)
#include "quiview_accessibility.mm"
#endif
