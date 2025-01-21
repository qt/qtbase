// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include "qioswindow.h"
#include "quiview.h"

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
    return qMakePair(anchorPos, cursorPos);
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
  ease showing and hiding it correctly.
  */
@interface QIOSEditMenu : NSObject
@property (nonatomic, assign) BOOL visible;
@property (nonatomic, readonly) BOOL isHiding;
@property (nonatomic, readonly) BOOL shownByUs;
@property (nonatomic, assign) BOOL reshowAfterHidden;
@end

@implementation QIOSEditMenu

- (instancetype)init
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
            _shownByUs = NO;
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
        // UIMenuController is a singleton that can be shown (and hidden) from anywhere.
        // Try to keep track of whether or not is was shown by us (the gesture recognizers
        // in this file) to avoid closing it if it was opened from elsewhere.
        _shownByUs = YES;
        // Note that the contents of the edit menu is decided by
        // first responder, which is normally QIOSTextResponder.
        QRectF cr = QPlatformInputContext::cursorRectangle();
        QRectF ar = QPlatformInputContext::anchorRectangle();

        CGRect targetRect = cr.united(ar).toCGRect();
        UIView *focusView = reinterpret_cast<UIView *>(qApp->focusWindow()->winId());
        [[UIMenuController sharedMenuController] setTargetRect:targetRect inView:focusView];
        [[UIMenuController sharedMenuController] setMenuVisible:YES animated:YES];
    } else {
        [[UIMenuController sharedMenuController] setMenuVisible:NO animated:YES];
    }
}

@end

void showEditMenu(UIView *focusView, QPoint touchPos)
{
    const bool mouseTriggered = false;
    const Qt::KeyboardModifiers keyboardModifiers = Qt::NoModifier;
    QWindow *qtWindow = quiview_cast(focusView).platformWindow->window();
    const auto globalTouchPos = qtWindow->mapToGlobal(touchPos);
    const bool contextMenuEventAccepted = QWindowSystemInterface::handleContextMenuEvent<
        QWindowSystemInterface::SynchronousDelivery>(qtWindow, mouseTriggered, touchPos,
                                                     globalTouchPos, keyboardModifiers);

    if (!contextMenuEventAccepted) {
        // Fall back to show the default platform menu, like we did
        // before we started sending context menu events. This is
        // to be backwards compatible with Widgets and Quick items.
        QIOSTextInputOverlay::s_editMenu.visible = YES;
    }
}

// -------------------------------------------------------------------------

@interface QIOSLoupeLayer : CALayer
@property (nonatomic, retain) UIView *targetView;
@property (nonatomic, assign) CGPoint focalPoint;
@property (nonatomic, assign) BOOL visible;
@end

@implementation QIOSLoupeLayer {
    UIView *_snapshotView;
    BOOL _pendingSnapshotUpdate;
    UIView *_loupeImageView;
    CALayer *_containerLayer;
    CGFloat _loupeOffset;
    QTimer _updateTimer;
}

- (instancetype)initWithSize:(CGSize)size cornerRadius:(CGFloat)cornerRadius bottomOffset:(CGFloat)bottomOffset
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

@interface QIOSHandleLayer : CALayer <CAAnimationDelegate>
@property (nonatomic, assign) CGRect cursorRectangle;
@property (nonatomic, assign) CGFloat handleScale;
@property (nonatomic, assign) BOOL visible;
@property (nonatomic, copy) Block onAnimationDidStop;
@end

@implementation QIOSHandleLayer {
    CALayer *_handleCursorLayer;
    CALayer *_handleKnobLayer;
    Qt::Edge _selectionEdge;
}

@dynamic handleScale;

- (instancetype)initWithKnobAtEdge:(Qt::Edge)selectionEdge
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
            animation.values = @[@(0.0f), @(1.3f), @(1.3f), @(1.0f)];
            animation.keyTimes = @[@(0.0f), @(0.3f), @(0.9f), @(1.0f)];
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
@interface QIOSLoupeRecognizer : UIGestureRecognizer <UIGestureRecognizerDelegate>
@property (nonatomic, assign) QPointF focalPoint;
@property (nonatomic, assign) BOOL dragTriggersGesture;
@property (nonatomic, readonly) UIView *focusView;
@end

@implementation QIOSLoupeRecognizer {
    QIOSLoupeLayer *_loupeLayer;
    UIView *_desktopView;
    CGPoint _firstTouchPoint;
    CGPoint _lastTouchPoint;
    QTimer _triggerStateBeganTimer;
    int _originalCursorFlashTime;
}

- (instancetype)init
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
        _desktopView = [presentationWindow(nullptr).rootViewController.view retain];
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
        QIOSTextInputOverlay::s_editMenu.visible = NO;
        break;
    case UIGestureRecognizerStateChanged:
        // Tell the sub class to move the loupe to the correct position
        [self updateFocalPoint:QPointF::fromCGPoint(_lastTouchPoint)];
        break;
    case UIGestureRecognizerStateEnded: {
        // Restore cursor blinking, and hide the loupe
        QGuiApplication::styleHints()->setCursorFlashTime(_originalCursorFlashTime);
        const QPoint touchPos = QPointF::fromCGPoint(_lastTouchPoint).toPoint();
        showEditMenu(_focusView, touchPos);
        _loupeLayer.visible = NO;
        break;
    }
    default:
        _loupeLayer.visible = NO;
        break;
    }
}

- (void)createLoupe
{
    // We magnify the desktop view. But the loupe itself will be added as a child
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
    Q_UNUSED(touchPoint);
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
    Q_UNUSED(touchPoint);
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
    QRectF inputRect = QPlatformInputContext::inputItemRectangle();
    return !hasSelection() && inputRect.contains(touchPoint);
}

- (void)updateFocalPoint:(QPointF)touchPoint
{
    self.focalPoint = touchPoint;

    const int currentCursorPos = QInputMethod::queryFocusObject(Qt::ImCursorPosition, QVariant()).toInt();
    const int newCursorPos = QPlatformInputContext::queryFocusObject(Qt::ImCursorPosition, touchPoint).toInt();
    if (newCursorPos != currentCursorPos)
        QPlatformInputContext::setSelectionOnFocusObject(touchPoint, touchPoint);
}

@end

// -------------------------------------------------------------------------

/**
  This recognizer will watch for selections, and draw handles as overlay
  on the sides. If the user starts dragging on a handle (or do a press and
  hold), it will show a magnifier glass that follows the handle as it moves.
  */
@interface QIOSSelectionRecognizer : QIOSLoupeRecognizer
@end

@implementation QIOSSelectionRecognizer {
    CALayer *_clipRectLayer;
    QIOSHandleLayer *_cursorLayer;
    QIOSHandleLayer *_anchorLayer;
    QPointF _touchOffset;
    bool _dragOnCursor;
    bool _dragOnAnchor;
    bool _multiLine;
    QTimer _updateSelectionTimer;
    QMetaObject::Connection _cursorConnection;
    QMetaObject::Connection _anchorConnection;
    QMetaObject::Connection _clipRectConnection;
}

- (instancetype)init
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

        if (QIOSTextInputOverlay::s_editMenu.shownByUs)
            QIOSTextInputOverlay::s_editMenu.visible = NO;
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
    QPointF cursorCenter = QPlatformInputContext::cursorRectangle().center();
    QPointF anchorCenter = QPlatformInputContext::anchorRectangle().center();
    QPointF cursorOffset = QPointF(cursorCenter.x() - touchPoint.x(), cursorCenter.y() - touchPoint.y());
    QPointF anchorOffset = QPointF(anchorCenter.x() - touchPoint.x(), anchorCenter.y() - touchPoint.y());
    double cursorDist = hypot(cursorOffset.x(), cursorOffset.y());
    double anchorDist = hypot(anchorOffset.x(), anchorOffset.y());

    if (cursorDist > handleRadius && anchorDist > handleRadius)
        return NO;

    if (cursorDist < anchorDist) {
        _touchOffset = cursorOffset;
        _dragOnCursor = YES;
        _dragOnAnchor = NO;
    } else {
        _touchOffset = anchorOffset;
        _dragOnCursor = NO;
        _dragOnAnchor = YES;
    }

    return YES;
}

- (void)updateFocalPoint:(QPointF)touchPoint
{
    touchPoint += _touchOffset;

    // Get the text position under the touch
    SelectionPair selection = querySelection();
    int touchTextPos = QPlatformInputContext::queryFocusObject(Qt::ImCursorPosition, touchPoint).toInt();

    // Ensure that the handles cannot be dragged past each other
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
        QPlatformInputContext::cursorRectangle() :
        QPlatformInputContext::anchorRectangle();
    self.focalPoint = QPointF(touchPoint.x(), handleRect.center().y());
}

- (void)updateSelection
{
    if (!hasSelection()) {
        if (_cursorLayer.visible) {
            _cursorLayer.visible = NO;
            _anchorLayer.visible = NO;
        }
        if (QIOSTextInputOverlay::s_editMenu.shownByUs)
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
    QRectF inputRect = QPlatformInputContext::inputItemClipRectangle();
    CGRect cursorRect = QPlatformInputContext::cursorRectangle().toCGRect();
    CGRect anchorRect = QPlatformInputContext::anchorRectangle().toCGRect();

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
  This recognizer will show the edit menu if the user taps inside the input
  item without changing the cursor position, or hide it if it's already visible
  and the user taps anywhere on the screen.
  */
@interface QIOSTapRecognizer : UITapGestureRecognizer
@end

@implementation QIOSTapRecognizer {
    int _cursorPosOnPress;
    bool _menuShouldBeVisible;
    UIView *_focusView;
}

- (instancetype)init
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

    QRectF inputRect = QPlatformInputContext::inputItemClipRectangle();
    QPointF touchPos = QPointF::fromCGPoint([static_cast<UITouch *>([touches anyObject]) locationInView:_focusView]);
    const bool touchInsideInputArea = inputRect.contains(touchPos);

    if (touchInsideInputArea && hasSelection()) {
        // When we have a selection and the user taps inside the input area, we stop
        // tracking, and let Qt handle the event like normal. Unless the selection
        // recogniser is triggered instead (if the touch is on top of the selection
        // handles) this will typically result in Qt clearing the selection, which in
        // turn will make the selection recogniser hide the menu.
        self.state = UIGestureRecognizerStateFailed;
        return;
    }

    if (QIOSTextInputOverlay::s_editMenu.visible) {
        // When the menu is visible and there is no selection, we should always
        // hide it, regardless of where the user tapped on the screen. We achieve
        // this by continue tracking so that we receive a touchesEnded call.
        // But note, we only want to hide the menu, and not clear the selection.
        // Only when the user taps inside the input area do we want to clear the
        // selection as well. This is different from native behavior, but done so
        // deliberately for cross-platform consistency. This will let the user click on
        // e.g "Bold" and "Italic" buttons elsewhere in the UI to modify the selected text.
        return;
    }

    if (!touchInsideInputArea) {
        // If the menu is not showing, and the touch is outside the input
        // area, there is nothing left for this recogniser to do.
        self.state = UIGestureRecognizerStateFailed;
        return;
    }

    // When no menu is showing, and the touch is inside the input
    // area, we check if we should show it. We want to do so if
    // the tap doesn't result in the cursor changing position.
    _cursorPosOnPress = QInputMethod::queryFocusObject(Qt::ImCursorPosition, QVariant()).toInt();
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    if (QIOSTextInputOverlay::s_editMenu.visible) {
        _menuShouldBeVisible = false;
    } else {
        QPointF touchPos = QPointF::fromCGPoint([static_cast<UITouch *>([touches anyObject]) locationInView:_focusView]);
        int cursorPosOnRelease = QPlatformInputContext::queryFocusObject(Qt::ImCursorPosition, touchPos).toInt();

        if (cursorPosOnRelease == _cursorPosOnPress) {
            // We've recognized a gesture to open the menu, but we don't know
            // whether the user tapped a control that was overlaid our input
            // area, since we don't do any granular hit-testing in touchesBegan.
            // To ensure that the gesture doesn't eat touch events that should
            // have reached another UI control we report the gesture as failed
            // here, and then manually show the menu at the next runloop pass.
            _menuShouldBeVisible = true;
            self.state = UIGestureRecognizerStateFailed;
            dispatch_async(dispatch_get_main_queue(), ^{
                if (_menuShouldBeVisible)
                    showEditMenu(_focusView, touchPos.toPoint());
                else
                    QIOSTextInputOverlay::s_editMenu.visible = false;
            });
        } else {
            // The menu is hidden, and the cursor will change position once
            // Qt receive the touch release. We therefore fail so that we
            // don't block the touch event from further processing.
            self.state = UIGestureRecognizerStateFailed;
        }
    }

    [super touchesEnded:touches withEvent:event];
}

- (void)gestureStateChanged
{
    if (self.state != UIGestureRecognizerStateEnded)
        return;

    QIOSTextInputOverlay::s_editMenu.visible = _menuShouldBeVisible;
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
    // Destroy old recognizers since they were created with
    // dependencies to the old focus object (focus view).
    if (m_cursorRecognizer) {
        m_cursorRecognizer.enabled = NO;
        [m_cursorRecognizer release];
        m_cursorRecognizer = nullptr;
    }
    if (m_selectionRecognizer) {
        m_selectionRecognizer.enabled = NO;
        [m_selectionRecognizer release];
        m_selectionRecognizer = nullptr;
    }
    if (m_openMenuOnTapRecognizer) {
        m_openMenuOnTapRecognizer.enabled = NO;
        [m_openMenuOnTapRecognizer release];
        m_openMenuOnTapRecognizer = nullptr;
    }

    if (s_editMenu) {
        [s_editMenu release];
        s_editMenu = nullptr;
    }

    const QVariant hintsVariant = QGuiApplication::inputMethod()->queryFocusObject(Qt::ImHints, QVariant());
    const Qt::InputMethodHints hints = Qt::InputMethodHints(hintsVariant.toUInt());
    if (hints & Qt::ImhNoTextHandles)
        return;

    // The focus object can emit selection updates (e.g from mouse drag), and
    // accept modifying it through IM when dragging on the handles, even if it
    // doesn't accept text input and IM in general (and hence return false from
    // inputMethodAccepted()). This is the case for read-only text fields.
    // Therefore, listen for selection changes also when the focus object
    // reports that it's ImReadOnly (which we take as a hint that it's actually
    // a text field, and that selections therefore might happen). But since
    // we have no guarantee that the focus object can actually accept new selections
    // through IM (and since we also need to respect if the input accepts selections
    // in the first place), we only support selections started by the text field (e.g from
    // mouse drag), even if we in theory could also start selections from a loupe.

    const bool inputAccepted = platformInputContext()->inputMethodAccepted();
    const bool readOnly = QGuiApplication::inputMethod()->queryFocusObject(Qt::ImReadOnly, QVariant()).toBool();

    if (inputAccepted || readOnly) {
        if (!(hints & Qt::ImhNoEditMenu))
            s_editMenu = [QIOSEditMenu new];
        m_selectionRecognizer = [QIOSSelectionRecognizer new];
        m_openMenuOnTapRecognizer = [QIOSTapRecognizer new];
        m_selectionRecognizer.enabled = YES;
        m_openMenuOnTapRecognizer.enabled = YES;
    }

    if (inputAccepted) {
        m_cursorRecognizer = [QIOSCursorRecognizer new];
        m_cursorRecognizer.enabled = YES;
    }
}

QT_END_NAMESPACE
