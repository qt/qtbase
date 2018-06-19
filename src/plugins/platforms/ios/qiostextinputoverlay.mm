/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#import <UIKit/UIGestureRecognizerSubclass.h>
#import <UIKit/UITextView.h>

#include <QtGui/QGuiApplication>
#include <QtGui/QInputMethod>
#include <QtGui/QStyleHints>

#include <QtGui/private/qinputmethod_p.h>
#include <QtCore/private/qobject_p.h>
#include <QtCore/private/qcore_mac_p.h>

#include "qiosglobal.h"
#include "qiostextinputoverlay.h"

typedef QPair<int, int> SelectionPair;
typedef void (^Block)(void);

static const CGFloat kKnobWidth = 10;

static QPlatformInputContext *platformInputContext()
{
    return static_cast<QInputMethodPrivate *>(QObjectPrivate::get(QGuiApplication::inputMethod()))->platformInputContext();
}

static SelectionPair querySelection()
{
    QInputMethodQueryEvent query(Qt::ImAnchorPosition | Qt::ImCursorPosition);
    QGuiApplication::sendEvent(QGuiApplication::focusObject(), &query);
    int anchorPos = query.value(Qt::ImAnchorPosition).toInt();
    int cursorPos = query.value(Qt::ImCursorPosition).toInt();
    return qMakePair<int, int>(anchorPos, cursorPos);
}

static bool hasSelection()
{
    SelectionPair selection = querySelection();
    return selection.first != selection.second;
}

static void executeBlockWithoutAnimation(Block block)
{
    [CATransaction begin];
    [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
    block();
    [CATransaction commit];
}

// -------------------------------------------------------------------------
/**
  QIOSEditMenu is just a wrapper class around UIMenuController to
  ease showing and hiding it correcly.
  */
@interface QIOSEditMenu : NSObject
@property (nonatomic, assign) BOOL visible;
@property (nonatomic, readonly) BOOL isHiding;
@property (nonatomic, assign) BOOL reshowAfterHidden;
@end

@implementation QIOSEditMenu

- (id)init
{
    if (self = [super init]) {
        NSNotificationCenter *center = [NSNotificationCenter defaultCenter];

        [center addObserverForName:UIMenuControllerWillHideMenuNotification
            object:nil queue:nil usingBlock:^(NSNotification *) {
            _isHiding = YES;
        }];

        [center addObserverForName:UIMenuControllerDidHideMenuNotification
            object:nil queue:nil usingBlock:^(NSNotification *) {
            _isHiding = NO;
            if (self.reshowAfterHidden) {
                // To not abort an ongoing hide transition when showing the menu, you can set
                // reshowAfterHidden to wait until the transition finishes before reshowing it.
                self.reshowAfterHidden = NO;
                dispatch_async(dispatch_get_main_queue (), ^{ self.visible = YES; });
            }
        }];
        [center addObserverForName:UIKeyboardDidHideNotification object:nil queue:nil
            usingBlock:^(NSNotification *) {
                self.visible = NO;
        }];

    }

    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:nil object:nil];
    [super dealloc];
}

- (BOOL)visible
{
    return [UIMenuController sharedMenuController].menuVisible;
}

- (void)setVisible:(BOOL)visible
{
    if (visible == self.visible)
        return;

    if (visible) {
        // Note that the contents of the edit menu is decided by
        // first responder, which is normally QIOSTextResponder.
        QRectF cr = qApp->inputMethod()->cursorRectangle();
        QRectF ar = qApp->inputMethod()->anchorRectangle();
        CGRect targetRect = cr.united(ar).toCGRect();
        UIView *focusView = reinterpret_cast<UIView *>(qApp->focusWindow()->winId());
        [[UIMenuController sharedMenuController] setTargetRect:targetRect inView:focusView];
        [[UIMenuController sharedMenuController] setMenuVisible:YES animated:YES];
    } else {
        [[UIMenuController sharedMenuController] setMenuVisible:NO animated:YES];
    }
}

@end

// -------------------------------------------------------------------------

@interface QIOSLoupeLayer : CALayer {
    UIView *_snapshotView;
    BOOL _pendingSnapshotUpdate;
    UIView *_loupeImageView;
    CALayer *_containerLayer;
    CGFloat _loupeOffset;
    QTimer _updateTimer;
}
@property (nonatomic, retain) UIView *targetView;
@property (nonatomic, assign) CGPoint focalPoint;
@property (nonatomic, assign) BOOL visible;
@end

@implementation QIOSLoupeLayer

- (id)initWithSize:(CGSize)size cornerRadius:(CGFloat)cornerRadius bottomOffset:(CGFloat)bottomOffset
{
    if (self = [super init]) {
        _loupeOffset = bottomOffset + (size.height / 2);
        _snapshotView = nil;
        _pendingSnapshotUpdate = YES;
        _updateTimer.setInterval(100);
        _updateTimer.setSingleShot(true);
        QObject::connect(&_updateTimer, &QTimer::timeout, [self](){ [self updateSnapshot]; });

        // Create own geometry and outer shadow
        self.frame = CGRectMake(0, 0, size.width, size.height);
        self.cornerRadius = cornerRadius;
        self.shadowColor = [[UIColor grayColor] CGColor];
        self.shadowOffset = CGSizeMake(0, 1);
        self.shadowRadius = 2.0;
        self.shadowOpacity = 0.75;
        self.transform = CATransform3DMakeScale(0, 0, 0);

        // Create container view for the snapshots
        _containerLayer = [[CALayer new] autorelease];
        _containerLayer.frame = self.bounds;
        _containerLayer.cornerRadius = cornerRadius;
        _containerLayer.masksToBounds = YES;
        [self addSublayer:_containerLayer];

        // Create inner loupe shadow
        const CGFloat inset = 30;
        CALayer *topShadeLayer = [[CALayer new] autorelease];
        topShadeLayer.frame = CGRectOffset(CGRectInset(self.bounds, -inset, -inset), 0, inset / 2);
        topShadeLayer.borderWidth = inset / 2;
        topShadeLayer.cornerRadius = cornerRadius;
        topShadeLayer.borderColor = [[UIColor blackColor] CGColor];
        topShadeLayer.shadowColor = [[UIColor blackColor] CGColor];
        topShadeLayer.shadowOffset = CGSizeMake(0, 0);
        topShadeLayer.shadowRadius = 15.0;
        topShadeLayer.shadowOpacity = 0.6;
        // Keep the shadow inside the loupe
        CALayer *mask = [[CALayer new] autorelease];
        mask.frame = CGRectOffset(self.bounds, inset, inset / 2);
        mask.backgroundColor = [[UIColor blackColor] CGColor];
        mask.cornerRadius = cornerRadius;
        topShadeLayer.mask = mask;
        [self addSublayer:topShadeLayer];

        // Create border around the loupe. We need to do this in a separate
        // layer (as opposed to on self) to not draw the border on top of
        // overlapping external children (arrow).
        CALayer *borderLayer = [[CALayer new] autorelease];
        borderLayer.frame = self.bounds;
        borderLayer.borderWidth = 0.75;
        borderLayer.cornerRadius = cornerRadius;
        borderLayer.borderColor = [[UIColor lightGrayColor] CGColor];
        [self addSublayer:borderLayer];
    }

    return self;
}

- (void)dealloc
{
    _targetView = nil;
    [super dealloc];
}

- (void)setVisible:(BOOL)visible
{
    if (_visible == visible)
        return;

    _visible = visible;

    dispatch_async(dispatch_get_main_queue (), ^{
        // Setting transform later, since CA will not perform an animation if
        // changing values directly after init, and if the scale ends up empty.
        self.transform = _visible ? CATransform3DMakeScale(1, 1, 1) : CATransform3DMakeScale(0.0, 0.0, 1);
    });
}

- (void)updateSnapshot
{
    _pendingSnapshotUpdate = YES;
    [self setNeedsDisplay];
}

- (void)setFocalPoint:(CGPoint)point
{
    _focalPoint = point;
    [self updateSnapshot];

    // Schedule a delayed update as well to ensure that we end up with a correct
    // snapshot of the cursor, since QQuickRenderThread lags a bit behind
    _updateTimer.start();
}

- (void)display
{
     // Take a snapshow of the target view, magnify the area around the focal
     // point, and add the snapshow layer as a child of the container layer
     // to make it look like a loupe. Then place this layer at the position of
     // the focal point with the requested offset.
     executeBlockWithoutAnimation(^{
         if (_pendingSnapshotUpdate) {
             UIView *newSnapshot = [_targetView snapshotViewAfterScreenUpdates:NO];
             [_snapshotView.layer removeFromSuperlayer];
             [_snapshotView release];
             _snapshotView = [newSnapshot retain];
             [_containerLayer addSublayer:_snapshotView.layer];
             _pendingSnapshotUpdate = NO;
         }

         self.position = CGPointMake(_focalPoint.x, _focalPoint.y - _loupeOffset);

         const CGFloat loupeScale = 1.5;
         CGFloat x = -(_focalPoint.x * loupeScale) + self.frame.size.width / 2;
         CGFloat y = -(_focalPoint.y * loupeScale) + self.frame.size.height / 2;
         CGFloat w = _targetView.frame.size.width * loupeScale;
         CGFloat h = _targetView.frame.size.height * loupeScale;
         _snapshotView.layer.frame = CGRectMake(x, y, w, h);
     });
}

@end

// -------------------------------------------------------------------------

#if QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_10_0)
@interface QIOSHandleLayer : CALayer <CAAnimationDelegate> {
#else
@interface QIOSHandleLayer : CALayer {
#endif
    CALayer *_handleCursorLayer;
    CALayer *_handleKnobLayer;
    Qt::Edge _selectionEdge;
}
@property (nonatomic, assign) CGRect cursorRectangle;
@property (nonatomic, assign) CGFloat handleScale;
@property (nonatomic, assign) BOOL visible;
@property (nonatomic, copy) Block onAnimationDidStop;
@end

@implementation QIOSHandleLayer

@dynamic handleScale;

- (id)initWithKnobAtEdge:(Qt::Edge)selectionEdge
{
    if (self = [super init]) {
        CGColorRef bgColor = [UIColor colorWithRed:0.1 green:0.4 blue:0.9 alpha:1].CGColor;
        _selectionEdge = selectionEdge;
        self.handleScale = 0;

        _handleCursorLayer = [[CALayer new] autorelease];
        _handleCursorLayer.masksToBounds = YES;
        _handleCursorLayer.backgroundColor = bgColor;
        [self addSublayer:_handleCursorLayer];

        _handleKnobLayer = [[CALayer new] autorelease];
        _handleKnobLayer.masksToBounds = YES;
        _handleKnobLayer.backgroundColor = bgColor;
        _handleKnobLayer.cornerRadius = kKnobWidth / 2;
        [self addSublayer:_handleKnobLayer];
    }
    return self;
}

+ (BOOL)needsDisplayForKey:(NSString *)key
{
    if ([key isEqualToString:@"handleScale"])
        return YES;
    return [super needsDisplayForKey:key];
}

- (id<CAAction>)actionForKey:(NSString *)key
{
    if ([key isEqualToString:@"handleScale"]) {
        if (_visible) {
            // The handle should "bounce" in when becoming visible
            CAKeyframeAnimation * animation = [CAKeyframeAnimation animationWithKeyPath:key];
            [animation setDuration:0.5];
            animation.values = [NSArray arrayWithObjects:
                [NSNumber numberWithFloat:0],
                [NSNumber numberWithFloat:1.3],
                [NSNumber numberWithFloat:1.3],
                [NSNumber numberWithFloat:1], nil];
            animation.keyTimes = [NSArray arrayWithObjects:
                [NSNumber numberWithFloat:0],
                [NSNumber numberWithFloat:0.3],
                [NSNumber numberWithFloat:0.9],
                [NSNumber numberWithFloat:1], nil];
            return animation;
        } else {
            CABasicAnimation *animation = [CABasicAnimation animationWithKeyPath:key];
            [animation setDelegate:self];
            animation.fromValue = [self valueForKey:key];
            [animation setDuration:0.2];
            return animation;
        }
    }
    return [super actionForKey:key];
}

- (void)animationDidStop:(CAAnimation *)animation finished:(BOOL)flag
{
    Q_UNUSED(animation);
    Q_UNUSED(flag);
    if (self.onAnimationDidStop)
        self.onAnimationDidStop();
}

- (void)setVisible:(BOOL)visible
{
    if (visible == _visible)
        return;

    _visible = visible;

    self.handleScale = visible ? 1 : 0;
}

- (void)setCursorRectangle:(CGRect)cursorRect
{
    if (CGRectEqualToRect(_cursorRectangle, cursorRect))
        return;

    _cursorRectangle = cursorRect;

    executeBlockWithoutAnimation(^{
        [self setNeedsDisplay];
        [self displayIfNeeded];
    });
}

- (void)display
{
    CGFloat cursorWidth = 2;
    CGPoint origin = _cursorRectangle.origin;
    CGSize size = _cursorRectangle.size;
    CGFloat scale = ((QIOSHandleLayer *)[self presentationLayer]).handleScale;
    CGFloat edgeAdjustment = (_selectionEdge == Qt::LeftEdge) ? 0.5 - cursorWidth : -0.5;

    CGFloat cursorX = origin.x + (size.width / 2) + edgeAdjustment;
    CGFloat cursorY = origin.y;
    CGFloat knobX = cursorX - (kKnobWidth - cursorWidth) / 2;
    CGFloat knobY = origin.y + ((_selectionEdge == Qt::LeftEdge) ? -kKnobWidth : size.height);

    _handleCursorLayer.frame = CGRectMake(cursorX, cursorY, cursorWidth, size.height);
    _handleKnobLayer.frame = CGRectMake(knobX, knobY, kKnobWidth, kKnobWidth);
    _handleCursorLayer.transform = CATransform3DMakeScale(1, scale, scale);
    _handleKnobLayer.transform = CATransform3DMakeScale(scale, scale, scale);
}

@end

// -------------------------------------------------------------------------

/**
  QIOSLoupeRecognizer is only a base class from which other recognisers
  below will inherit. It takes care of creating and showing a magnifier
  glass depending on the current gesture state.
  */
@interface QIOSLoupeRecognizer : UIGestureRecognizer <UIGestureRecognizerDelegate> {
@public
    QIOSLoupeLayer *_loupeLayer;
    UIView *_desktopView;
    CGPoint _firstTouchPoint;
    CGPoint _lastTouchPoint;
    QTimer _triggerStateBeganTimer;
    int _originalCursorFlashTime;
}
@property (nonatomic, assign) QPointF focalPoint;
@property (nonatomic, assign) BOOL dragTriggersGesture;
@property (nonatomic, readonly) UIView *focusView;
@end

@implementation QIOSLoupeRecognizer

- (id)init
{
    if (self = [super initWithTarget:self action:@selector(gestureStateChanged)]) {
        self.enabled = NO;
        _triggerStateBeganTimer.setInterval(QGuiApplication::styleHints()->startDragTime());
        _triggerStateBeganTimer.setSingleShot(true);
        QObject::connect(&_triggerStateBeganTimer, &QTimer::timeout, [=](){
            self.state = UIGestureRecognizerStateBegan;
        });
    }

    return self;
}

- (void)setEnabled:(BOOL)enabled
{
    if (enabled == self.enabled)
        return;

    [super setEnabled:enabled];

    if (enabled) {
        _focusView = [reinterpret_cast<UIView *>(qApp->focusWindow()->winId()) retain];
        _desktopView = [qt_apple_sharedApplication().keyWindow.rootViewController.view retain];
        Q_ASSERT(_focusView && _desktopView && _desktopView.superview);
        [_desktopView addGestureRecognizer:self];
    } else {
        [_desktopView removeGestureRecognizer:self];
        [_desktopView release];
        _desktopView = nil;
        [_focusView release];
        _focusView = nil;
        _triggerStateBeganTimer.stop();
        if (_loupeLayer) {
            [_loupeLayer removeFromSuperlayer];
            [_loupeLayer release];
            _loupeLayer = nil;
        }
    }
}

- (void)gestureStateChanged
{
    switch (self.state) {
    case UIGestureRecognizerStateBegan:
        // Stop cursor blinking, and show the loupe
        _originalCursorFlashTime = QGuiApplication::styleHints()->cursorFlashTime();
        QGuiApplication::styleHints()->setCursorFlashTime(0);
        if (!_loupeLayer)
            [self createLoupe];
        [self updateFocalPoint:QPointF::fromCGPoint(_lastTouchPoint)];
        _loupeLayer.visible = YES;
        break;
    case UIGestureRecognizerStateChanged:
        // Tell the sub class to move the loupe to the correct position
        [self updateFocalPoint:QPointF::fromCGPoint(_lastTouchPoint)];
        break;
    case UIGestureRecognizerStateEnded:
        // Restore cursor blinking, and hide the loupe
        QGuiApplication::styleHints()->setCursorFlashTime(_originalCursorFlashTime);
        QIOSTextInputOverlay::s_editMenu.visible = YES;
        _loupeLayer.visible = NO;
        break;
    default:
        _loupeLayer.visible = NO;
        break;
    }
}

- (void)createLoupe
{
    // We magnify the the desktop view. But the loupe itself will be added as a child
    // of the desktop view's parent, so it doesn't become a part of what we magnify.
    _loupeLayer = [[self createLoupeLayer] retain];
    _loupeLayer.targetView = _desktopView;
    [_desktopView.superview.layer addSublayer:_loupeLayer];
}

- (QPointF)focalPoint
{
    return QPointF::fromCGPoint([_loupeLayer.targetView convertPoint:_loupeLayer.focalPoint toView:_focusView]);
}

- (void)setFocalPoint:(QPointF)point
{
    _loupeLayer.focalPoint = [_loupeLayer.targetView convertPoint:point.toCGPoint() fromView:_focusView];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesBegan:touches withEvent:event];
    if ([event allTouches].count > 1) {
        // We only support text selection with one finger
        self.state = UIGestureRecognizerStateFailed;
        return;
    }

    _firstTouchPoint = [static_cast<UITouch *>([touches anyObject]) locationInView:_focusView];
    _lastTouchPoint = _firstTouchPoint;

    // If the touch point is accepted by the sub class (e.g touch on cursor), we start a
    // press'n'hold timer that eventually will move the state to UIGestureRecognizerStateBegan.
    if ([self acceptTouchesBegan:QPointF::fromCGPoint(_firstTouchPoint)])
        _triggerStateBeganTimer.start();
    else
        self.state = UIGestureRecognizerStateFailed;
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesMoved:touches withEvent:event];
    _lastTouchPoint = [static_cast<UITouch *>([touches anyObject]) locationInView:_focusView];

    if (self.state == UIGestureRecognizerStatePossible) {
        // If the touch was moved too far before the timer triggered (meaning that this
        // is a drag, not a press'n'hold), we should either fail, or trigger the gesture
        // immediately, depending on self.dragTriggersGesture.
        int startDragDistance = QGuiApplication::styleHints()->startDragDistance();
        int dragDistance = hypot(_firstTouchPoint.x - _lastTouchPoint.x, _firstTouchPoint.y - _lastTouchPoint.y);
        if (dragDistance > startDragDistance) {
            _triggerStateBeganTimer.stop();
            self.state = self.dragTriggersGesture ? UIGestureRecognizerStateBegan : UIGestureRecognizerStateFailed;
        }
    } else {
        self.state = UIGestureRecognizerStateChanged;
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];
    _triggerStateBeganTimer.stop();
    _lastTouchPoint = [static_cast<UITouch *>([touches anyObject]) locationInView:_focusView];
    self.state = self.state == UIGestureRecognizerStatePossible ? UIGestureRecognizerStateFailed : UIGestureRecognizerStateEnded;
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
    _triggerStateBeganTimer.stop();
    _lastTouchPoint = [static_cast<UITouch *>([touches anyObject]) locationInView:_focusView];
    self.state = UIGestureRecognizerStateCancelled;
}

// Methods implemented by subclasses:

- (BOOL)acceptTouchesBegan:(QPointF)touchPoint
{
    Q_UNUSED(touchPoint)
    Q_UNREACHABLE();
    return NO;
}

- (QIOSLoupeLayer *)createLoupeLayer
{
    Q_UNREACHABLE();
    return nullptr;
}

- (void)updateFocalPoint:(QPointF)touchPoint
{
    Q_UNUSED(touchPoint)
    Q_UNREACHABLE();
}

@end

// -------------------------------------------------------------------------

/**
  This recognizer will be active when there's no selection. It will trigger if
  the user does a press and hold, which will start a session where the user can move
  the cursor around with his finger together with a magnifier glass.
  */
@interface QIOSCursorRecognizer : QIOSLoupeRecognizer
@end

@implementation QIOSCursorRecognizer

- (QIOSLoupeLayer *)createLoupeLayer
{
    return [[[QIOSLoupeLayer alloc] initWithSize:CGSizeMake(120, 120) cornerRadius:60 bottomOffset:4] autorelease];
}

- (BOOL)acceptTouchesBegan:(QPointF)touchPoint
{
    QRectF inputRect = QGuiApplication::inputMethod()->inputItemClipRectangle();
    return !hasSelection() && inputRect.contains(touchPoint);
}

- (void)updateFocalPoint:(QPointF)touchPoint
{
    platformInputContext()->setSelectionOnFocusObject(touchPoint, touchPoint);
    self.focalPoint = touchPoint;
}

@end

// -------------------------------------------------------------------------

/**
  This recognizer will watch for selections, and draw handles as overlay
  on the sides. If the user starts dragging on a handle (or do a press and
  hold), it will show a magnifier glass that follows the handle as it moves.
  */
@interface QIOSSelectionRecognizer : QIOSLoupeRecognizer {
    CALayer *_clipRectLayer;
    QIOSHandleLayer *_cursorLayer;
    QIOSHandleLayer *_anchorLayer;
    QPointF _touchOffset;
    bool _dragOnCursor;
    bool _multiLine;
    QTimer _updateSelectionTimer;
    QMetaObject::Connection _cursorConnection;
    QMetaObject::Connection _anchorConnection;
    QMetaObject::Connection _clipRectConnection;
}
@end

@implementation QIOSSelectionRecognizer

- (id)init
{
    if (self = [super init]) {
        self.delaysTouchesBegan = YES;
        self.dragTriggersGesture = YES;
        _multiLine = QInputMethod::queryFocusObject(Qt::ImHints, QVariant()).toUInt() & Qt::ImhMultiLine;
        _updateSelectionTimer.setInterval(1);
        _updateSelectionTimer.setSingleShot(true);
        QObject::connect(&_updateSelectionTimer, &QTimer::timeout, [self](){ [self updateSelection]; });
    }

    return self;
}

- (void)setEnabled:(BOOL)enabled
{
    if (enabled == self.enabled)
        return;

    [super setEnabled:enabled];

    if (enabled) {
        // Create a layer that clips the handles inside the input field
        _clipRectLayer = [CALayer new];
        _clipRectLayer.masksToBounds = YES;
        [self.focusView.layer addSublayer:_clipRectLayer];

        // Create the handle layers, and add them to the clipped input rect layer
        _cursorLayer = [[[QIOSHandleLayer alloc] initWithKnobAtEdge:Qt::RightEdge] autorelease];
        _anchorLayer = [[[QIOSHandleLayer alloc] initWithKnobAtEdge:Qt::LeftEdge] autorelease];
        bool selection = hasSelection();
        _cursorLayer.visible = selection;
        _anchorLayer.visible = selection;
        [_clipRectLayer addSublayer:_cursorLayer];
        [_clipRectLayer addSublayer:_anchorLayer];

        // iOS text input will sometimes set a temporary text selection to perform operations
        // such as backspace (select last character + cut selection). To avoid briefly showing
        // the selection handles for such cases, and to avoid calling updateSelection when
        // both handles and clip rectangle change, we use a timer to wait a cycle before we update.
        // (Note that since QTimer::start is overloaded, we need some extra syntax for the connections).
        QInputMethod *im = QGuiApplication::inputMethod();
        void(QTimer::*start)(void) = &QTimer::start;
        _cursorConnection = QObject::connect(im, &QInputMethod::cursorRectangleChanged, &_updateSelectionTimer, start);
        _anchorConnection = QObject::connect(im, &QInputMethod::anchorRectangleChanged, &_updateSelectionTimer, start);
        _clipRectConnection = QObject::connect(im, &QInputMethod::inputItemClipRectangleChanged, &_updateSelectionTimer, start);

        [self updateSelection];
    } else {
        // Fade out the handles by setting visible to NO, and wait for the animations
        // to finish before removing the clip rect layer, including the handles.
        // Create a local variable to hold the clipRectLayer while the animation is
        // ongoing to ensure that any subsequent calls to setEnabled does not interfere.
        // Also, declare it as __block to stop it from being automatically retained, which
        // would cause a cyclic dependency between clipRectLayer and the block.
        __block CALayer *clipRectLayer = _clipRectLayer;
        __block int handleCount = 2;
        Block block = ^{
            if (--handleCount == 0) {
                [clipRectLayer removeFromSuperlayer];
                [clipRectLayer release];
            }
        };

        _cursorLayer.onAnimationDidStop = block;
        _anchorLayer.onAnimationDidStop = block;
        _cursorLayer.visible = NO;
        _anchorLayer.visible = NO;

        _clipRectLayer = 0;
        _cursorLayer = 0;
        _anchorLayer = 0;
        _updateSelectionTimer.stop();

        QObject::disconnect(_cursorConnection);
        QObject::disconnect(_anchorConnection);
        QObject::disconnect(_clipRectConnection);
    }
}

- (QIOSLoupeLayer *)createLoupeLayer
{
    CGSize loupeSize = CGSizeMake(123, 33);
    CGSize arrowSize = CGSizeMake(25, 12);
    CGFloat loupeOffset = arrowSize.height + 20;

    // Create loupe and arrow layers
    QIOSLoupeLayer *loupeLayer = [[[QIOSLoupeLayer alloc] initWithSize:loupeSize cornerRadius:5 bottomOffset:loupeOffset] autorelease];
    CAShapeLayer *arrowLayer = [[[CAShapeLayer alloc] init] autorelease];

    // Build a triangular path to both draw and mask the arrow layer as a triangle
    UIBezierPath *path = [[UIBezierPath new] autorelease];
    [path moveToPoint:CGPointMake(0, 0)];
    [path addLineToPoint:CGPointMake(arrowSize.width / 2, arrowSize.height)];
    [path addLineToPoint:CGPointMake(arrowSize.width, 0)];

    arrowLayer.frame = CGRectMake((loupeSize.width - arrowSize.width) / 2, loupeSize.height - 1, arrowSize.width, arrowSize.height);
    arrowLayer.path = path.CGPath;
    arrowLayer.backgroundColor = [[UIColor whiteColor] CGColor];
    arrowLayer.strokeColor = [[UIColor lightGrayColor] CGColor];
    arrowLayer.lineWidth = 0.75 * 2;
    arrowLayer.fillColor = nil;

    CAShapeLayer *mask = [[CAShapeLayer new] autorelease];
    mask.frame = arrowLayer.bounds;
    mask.path = path.CGPath;
    arrowLayer.mask = mask;

    [loupeLayer addSublayer:arrowLayer];

    return loupeLayer;
}

- (BOOL)acceptTouchesBegan:(QPointF)touchPoint
{
    if (!hasSelection())
        return NO;

    // Accept the touch if it "overlaps" with any of the handles
    const int handleRadius = 50;
    QPointF cursorCenter = qApp->inputMethod()->cursorRectangle().center();
    QPointF anchorCenter = qApp->inputMethod()->anchorRectangle().center();
    QPointF cursorOffset = QPointF(cursorCenter.x() - touchPoint.x(), cursorCenter.y() - touchPoint.y());
    QPointF anchorOffset = QPointF(anchorCenter.x() - touchPoint.x(), anchorCenter.y() - touchPoint.y());
    double cursorDist = hypot(cursorOffset.x(), cursorOffset.y());
    double anchorDist = hypot(anchorOffset.x(), anchorOffset.y());

    if (cursorDist > handleRadius && anchorDist > handleRadius)
        return NO;

    if (cursorDist < anchorDist) {
        _touchOffset = cursorOffset;
        _dragOnCursor = YES;
    } else {
        _touchOffset = anchorOffset;
        _dragOnCursor = NO;
    }

    return YES;
}

- (void)updateFocalPoint:(QPointF)touchPoint
{
    touchPoint += _touchOffset;

    // Get the text position under the touch
    SelectionPair selection = querySelection();
    const QTransform mapToLocal = QGuiApplication::inputMethod()->inputItemTransform().inverted();
    int touchTextPos = QInputMethod::queryFocusObject(Qt::ImCursorPosition, touchPoint * mapToLocal).toInt();

    // Ensure that the handels cannot be dragged past each other
    if (_dragOnCursor)
        selection.second = (touchTextPos > selection.first) ? touchTextPos : selection.first + 1;
    else
        selection.first = (touchTextPos < selection.second) ? touchTextPos : selection.second - 1;

    // Set new selection
    QList<QInputMethodEvent::Attribute> imAttributes;
    imAttributes.append(QInputMethodEvent::Attribute(
        QInputMethodEvent::Selection, selection.first, selection.second - selection.first, QVariant()));
    QInputMethodEvent event(QString(), imAttributes);
    QGuiApplication::sendEvent(qApp->focusObject(), &event);

    // Move loupe to new position
    QRectF handleRect = _dragOnCursor ?
        qApp->inputMethod()->cursorRectangle() :
        qApp->inputMethod()->anchorRectangle();
    self.focalPoint = QPointF(touchPoint.x(), handleRect.center().y());
}

- (void)updateSelection
{
    if (!hasSelection()) {
        _cursorLayer.visible = NO;
        _anchorLayer.visible = NO;
        QIOSTextInputOverlay::s_editMenu.visible = NO;
        return;
    }

    if (!_cursorLayer.visible && QIOSTextInputOverlay::s_editMenu.isHiding) {
        // Since the edit menu is hiding and this is the first selection thereafter, we
        // assume that the selection came from the user tapping on a menu item. In that
        // case, we reshow the menu after it has closed (but then with selection based
        // menu items, as specified by first responder).
        QIOSTextInputOverlay::s_editMenu.reshowAfterHidden = YES;
    }

    // Adjust handles and input rect to match the new selection
    QRectF inputRect = QGuiApplication::inputMethod()->inputItemClipRectangle();
    CGRect cursorRect = QGuiApplication::inputMethod()->cursorRectangle().toCGRect();
    CGRect anchorRect = QGuiApplication::inputMethod()->anchorRectangle().toCGRect();

    if (!_multiLine) {
        // Resize the layer a bit bigger to ensure that the handles are
        // not cut if if they are otherwise visible inside the clip rect.
        int margin = kKnobWidth + 5;
        inputRect.adjust(-margin / 2, -margin, margin / 2, margin);
    }

    executeBlockWithoutAnimation(^{ _clipRectLayer.frame = inputRect.toCGRect(); });
    _cursorLayer.cursorRectangle = [self.focusView.layer convertRect:cursorRect toLayer:_clipRectLayer];
    _anchorLayer.cursorRectangle = [self.focusView.layer convertRect:anchorRect toLayer:_clipRectLayer];
    _cursorLayer.visible = YES;
    _anchorLayer.visible = YES;
}

@end

// -------------------------------------------------------------------------

/**
  This recognizer will trigger if the user taps inside the edit rectangle.
  If there's no selection, and the tap doesn't change the cursor position, the
  visibility of the edit menu will be toggled. Otherwise, if there's a selection, a
  first tap will close the edit menu (if any), and a second tap will remove the selection.
  */
@interface QIOSTapRecognizer : UITapGestureRecognizer {
    int _cursorPosOnPress;
    UIView *_focusView;
}
@end

@implementation QIOSTapRecognizer

- (id)init
{
    if (self = [super initWithTarget:self action:@selector(gestureStateChanged)]) {
        self.enabled = NO;
    }

    return self;
}

- (void)setEnabled:(BOOL)enabled
{
    if (enabled == self.enabled)
        return;

    [super setEnabled:enabled];

    if (enabled) {
        _focusView = [reinterpret_cast<UIView *>(qApp->focusWindow()->winId()) retain];
        [_focusView addGestureRecognizer:self];
    } else {
        [_focusView removeGestureRecognizer:self];
        [_focusView release];
        _focusView = nil;
    }
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesBegan:touches withEvent:event];

    if (hasSelection() && !QIOSTextInputOverlay::s_editMenu.isHiding) {
        // If there's a selection and the menu is visible, UIKit will hide the menu on the
        // first tap. But if we get a second tap while the menu is hidden, we choose to diverge
        // a bit from native behavior and instead fail the tap and forward the touch
        // to Qt. This will effectively move the cursor (and remove the selection).
        // This is needed to ensure that the user can remove the selection at any time, but
        // at the same time, also be able to tap on other items in the UI while keeping the
        // selection (e.g make the selection bold by tapping on a bold button in the UI).
        self.state = UIGestureRecognizerStateFailed;
        return;
    }

    QRectF inputRect = QGuiApplication::inputMethod()->inputItemClipRectangle();
    QPointF touchPos = QPointF::fromCGPoint([static_cast<UITouch *>([touches anyObject]) locationInView:_focusView]);
    if (!inputRect.contains(touchPos))
        self.state = UIGestureRecognizerStateFailed;

    _cursorPosOnPress = QInputMethod::queryFocusObject(Qt::ImCursorPosition, QVariant()).toInt();
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    QPointF touchPos = QPointF::fromCGPoint([static_cast<UITouch *>([touches anyObject]) locationInView:_focusView]);
    const QTransform mapToLocal = QGuiApplication::inputMethod()->inputItemTransform().inverted();
    int cursorPosOnRelease = QInputMethod::queryFocusObject(Qt::ImCursorPosition, touchPos * mapToLocal).toInt();

    if (!QIOSTextInputOverlay::s_editMenu.isHiding && cursorPosOnRelease != _cursorPosOnPress) {
        // We also want to track if the user taps on the screen to close the edit menu. And
        // the way we detect that is to check if the edit menu is hiding when we receive this
        // call. If that's the case, we leave the state as-is to allow a tap to be recognized.
        // Otherwise, if we also see that the cursor will change position, we fail, so that
        // touch events for Qt are not cancelled.
        self.state = UIGestureRecognizerStateFailed;
    }

    [super touchesEnded:touches withEvent:event];
}

- (void)gestureStateChanged
{
    if (self.state != UIGestureRecognizerStateEnded)
        return;

    if (QIOSTextInputOverlay::s_editMenu.isHiding) {
        // Closing the menu is what we want for the first tap, so just return
        return;
    }

    QIOSTextInputOverlay::s_editMenu.visible = !QIOSTextInputOverlay::s_editMenu.visible;
}

@end

// -------------------------------------------------------------------------

QT_BEGIN_NAMESPACE

QIOSEditMenu *QIOSTextInputOverlay::s_editMenu = nullptr;

QIOSTextInputOverlay::QIOSTextInputOverlay()
    : m_cursorRecognizer(nullptr)
    , m_selectionRecognizer(nullptr)
    , m_openMenuOnTapRecognizer(nullptr)
{
    if (qt_apple_isApplicationExtension()) {
        qWarning() << "text input overlays disabled in application extensions";
        return;
    }

    connect(qApp, &QGuiApplication::focusObjectChanged, this, &QIOSTextInputOverlay::updateFocusObject);
}

QIOSTextInputOverlay::~QIOSTextInputOverlay()
{
    if (qApp)
        disconnect(qApp, 0, this, 0);
}

void QIOSTextInputOverlay::updateFocusObject()
{
    if (m_cursorRecognizer) {
        // Destroy old recognizers since they were created with
        // dependencies to the old focus object (focus view).
        m_cursorRecognizer.enabled = NO;
        m_selectionRecognizer.enabled = NO;
        m_openMenuOnTapRecognizer.enabled = NO;
        [m_cursorRecognizer release];
        [m_selectionRecognizer release];
        [m_openMenuOnTapRecognizer release];
        [s_editMenu release];
        m_cursorRecognizer = nullptr;
        m_selectionRecognizer = nullptr;
        m_openMenuOnTapRecognizer = nullptr;
        s_editMenu = nullptr;
    }

    if (platformInputContext()->inputMethodAccepted()) {
        s_editMenu = [QIOSEditMenu new];
        m_cursorRecognizer = [QIOSCursorRecognizer new];
        m_selectionRecognizer = [QIOSSelectionRecognizer new];
        m_openMenuOnTapRecognizer = [QIOSTapRecognizer new];
        m_cursorRecognizer.enabled = YES;
        m_selectionRecognizer.enabled = YES;
        m_openMenuOnTapRecognizer.enabled = YES;
    }
}

QT_END_NAMESPACE
