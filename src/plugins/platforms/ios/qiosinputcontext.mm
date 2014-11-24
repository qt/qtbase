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

#include "qiosinputcontext.h"

#import <UIKit/UIGestureRecognizerSubclass.h>

#include "qiosglobal.h"
#include "qiosintegration.h"
#include "qiostextresponder.h"
#include "qiosviewcontroller.h"
#include "qioswindow.h"
#include "quiview.h"

#include <QGuiApplication>
#include <QtGui/private/qwindow_p.h>

// -------------------------------------------------------------------------

static QUIView *focusView()
{
    return qApp->focusWindow() ?
        reinterpret_cast<QUIView *>(qApp->focusWindow()->winId()) : 0;
}

// -------------------------------------------------------------------------

@interface QIOSKeyboardListener : UIGestureRecognizer <UIGestureRecognizerDelegate> {
@public
    QIOSInputContext *m_context;
    BOOL m_keyboardVisible;
    BOOL m_keyboardVisibleAndDocked;
    QRectF m_keyboardRect;
    CGRect m_keyboardEndRect;
    NSTimeInterval m_duration;
    UIViewAnimationCurve m_curve;
    UIViewController *m_viewController;
}
@end

@implementation QIOSKeyboardListener

- (id)initWithQIOSInputContext:(QIOSInputContext *)context
{
    self = [super initWithTarget:self action:@selector(gestureStateChanged:)];
    if (self) {
        m_context = context;
        m_keyboardVisible = NO;
        m_keyboardVisibleAndDocked = NO;
        m_duration = 0;
        m_curve = UIViewAnimationCurveEaseOut;
        m_viewController = 0;

        if (isQtApplication()) {
            // Get the root view controller that is on the same screen as the keyboard:
            for (UIWindow *uiWindow in [[UIApplication sharedApplication] windows]) {
                if (uiWindow.screen == [UIScreen mainScreen]) {
                    m_viewController = [uiWindow.rootViewController retain];
                    break;
                }
            }
            Q_ASSERT(m_viewController);

            // Attach 'hide keyboard' gesture to the window, but keep it disabled when the
            // keyboard is not visible.
            self.enabled = NO;
            self.cancelsTouchesInView = NO;
            self.delaysTouchesEnded = NO;
            [m_viewController.view.window addGestureRecognizer:self];
        }

        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(keyboardWillShow:)
            name:@"UIKeyboardWillShowNotification" object:nil];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(keyboardWillHide:)
            name:@"UIKeyboardWillHideNotification" object:nil];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(keyboardDidChangeFrame:)
            name:@"UIKeyboardDidChangeFrameNotification" object:nil];
    }
    return self;
}

- (void) dealloc
{
    [m_viewController.view.window removeGestureRecognizer:self];
    [m_viewController release];

    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:@"UIKeyboardWillShowNotification" object:nil];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:@"UIKeyboardWillHideNotification" object:nil];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:@"UIKeyboardDidChangeFrameNotification" object:nil];
    [super dealloc];
}

- (void) keyboardDidChangeFrame:(NSNotification *)notification
{
    Q_UNUSED(notification);
    [self handleKeyboardRectChanged];

    // If the keyboard was visible and docked from before, this is just a geometry
    // change (normally caused by an orientation change). In that case, update scroll:
    if (m_keyboardVisibleAndDocked)
        m_context->scrollToCursor();
}

- (void) keyboardWillShow:(NSNotification *)notification
{
    // Note that UIKeyboardWillShowNotification is only sendt when the keyboard is docked.
    m_keyboardVisibleAndDocked = YES;
    m_keyboardEndRect = [[[notification userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];

    if (!m_duration) {
        m_duration = [[notification.userInfo objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
        m_curve = UIViewAnimationCurve([[notification.userInfo objectForKey:UIKeyboardAnimationCurveUserInfoKey] integerValue]);
    }

    UIResponder *firstResponder = [UIResponder currentFirstResponder];
    if (![firstResponder isKindOfClass:[QIOSTextInputResponder class]])
        return;

    // Enable hide-keyboard gesture
    self.enabled = YES;

    m_context->scrollToCursor();
}

- (void) keyboardWillHide:(NSNotification *)notification
{
    // Note that UIKeyboardWillHideNotification is also sendt when the keyboard is undocked.
    m_keyboardVisibleAndDocked = NO;
    m_keyboardEndRect = [[[notification userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    if (self.state != UIGestureRecognizerStateBegan) {
        // Only disable the gesture if the hiding of the keyboard was not caused by it.
        // Otherwise we need to await the final touchEnd callback for doing some clean-up.
        self.enabled = NO;
    }
    m_context->scroll(0);
}

- (void) handleKeyboardRectChanged
{
    // QInputmethod::keyboardRectangle() is documented to be in window coordinates.
    // If there is no focus window, we return an empty rectangle
    UIView *view = focusView();
    QRectF convertedRect = fromCGRect([view convertRect:m_keyboardEndRect fromView:nil]);

    // Set height to zero if keyboard is hidden. Otherwise the rect will not change
    // when the keyboard hides on a scrolled screen (since the keyboard will already
    // be at the bottom of the 'screen' in that case)
    if (!m_keyboardVisibleAndDocked)
        convertedRect.setHeight(0);

    if (convertedRect != m_keyboardRect) {
        m_keyboardRect = convertedRect;
        m_context->emitKeyboardRectChanged();
    }

    BOOL visible = CGRectIntersectsRect(m_keyboardEndRect, [UIScreen mainScreen].bounds);
    if (m_keyboardVisible != visible) {
        m_keyboardVisible = visible;
        m_context->emitInputPanelVisibleChanged();
    }
}

// -------------------------------------------------------------------------

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesBegan:touches withEvent:event];

    Q_ASSERT(m_keyboardVisibleAndDocked);

    if ([touches count] != 1)
        self.state = UIGestureRecognizerStateFailed;
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesMoved:touches withEvent:event];

    if (self.state != UIGestureRecognizerStatePossible)
        return;

    CGPoint touchPoint = [[touches anyObject] locationInView:m_viewController.view.window];
    if (CGRectContainsPoint(m_keyboardEndRect, touchPoint))
        self.state = UIGestureRecognizerStateBegan;
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];

    [self touchesEndedOrCancelled];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];

    [self touchesEndedOrCancelled];
}

- (void)touchesEndedOrCancelled
{
    // Defer final state change until next runloop iteration, so that Qt
    // has a chance to process the final touch events first, before we eg.
    // scroll the view.
    dispatch_async(dispatch_get_main_queue (), ^{
        // iOS will transition from began to changed by itself
        Q_ASSERT(self.state != UIGestureRecognizerStateBegan);

        if (self.state == UIGestureRecognizerStateChanged)
            self.state = UIGestureRecognizerStateEnded;
        else
            self.state = UIGestureRecognizerStateFailed;
    });
}

- (void)gestureStateChanged:(id)sender
{
    Q_UNUSED(sender);

    if (self.state == UIGestureRecognizerStateBegan) {
        qImDebug() << "hide keyboard gesture was triggered";
        UIResponder *firstResponder = [UIResponder currentFirstResponder];
        Q_ASSERT([firstResponder isKindOfClass:[QIOSTextInputResponder class]]);
        [firstResponder resignFirstResponder];
    }
}

- (void)reset
{
    [super reset];

    if (!m_keyboardVisibleAndDocked) {
        qImDebug() << "keyboard was hidden, disabling hide-keyboard gesture";
        self.enabled = NO;
    } else {
        qImDebug() << "gesture completed without triggering, scrolling view to cursor";
        m_context->scrollToCursor();
    }
}

@end

// -------------------------------------------------------------------------

Qt::InputMethodQueries ImeState::update(Qt::InputMethodQueries properties)
{
    if (!properties)
        return 0;

    QInputMethodQueryEvent newState(properties);

    // Update the focus object that the new state is based on
    focusObject = qApp ? qApp->focusObject() : 0;

    if (focusObject)
        QCoreApplication::sendEvent(focusObject, &newState);

    Qt::InputMethodQueries updatedProperties;
    for (uint i = 0; i < (sizeof(Qt::ImQueryAll) * CHAR_BIT); ++i) {
        if (Qt::InputMethodQuery property = Qt::InputMethodQuery(int(properties & (1 << i)))) {
            if (newState.value(property) != currentState.value(property)) {
                updatedProperties |= property;
                currentState.setValue(property, newState.value(property));
            }
        }
    }

    return updatedProperties;
}

// -------------------------------------------------------------------------

QIOSInputContext *QIOSInputContext::instance()
{
    return static_cast<QIOSInputContext *>(QIOSIntegration::instance()->inputContext());
}

QIOSInputContext::QIOSInputContext()
    : QPlatformInputContext()
    , m_keyboardListener([[QIOSKeyboardListener alloc] initWithQIOSInputContext:this])
    , m_textResponder(0)
{
    if (isQtApplication())
        connect(qGuiApp->inputMethod(), &QInputMethod::cursorRectangleChanged, this, &QIOSInputContext::cursorRectangleChanged);
    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &QIOSInputContext::focusWindowChanged);
}

QIOSInputContext::~QIOSInputContext()
{
    [m_keyboardListener release];
    [m_textResponder release];
}

QRectF QIOSInputContext::keyboardRect() const
{
    return m_keyboardListener->m_keyboardRect;
}

void QIOSInputContext::showInputPanel()
{
    // No-op, keyboard controlled fully by platform based on focus
    qImDebug() << "can't show virtual keyboard without a focus object, ignoring";
}

void QIOSInputContext::hideInputPanel()
{
    if (![m_textResponder isFirstResponder]) {
        qImDebug() << "QIOSTextInputResponder is not first responder, ignoring";
        return;
    }

    if (qGuiApp->focusObject() != m_imeState.focusObject) {
        qImDebug() << "current focus object does not match IM state, likely hiding from focusOut event, so ignoring";
        return;
    }

    qImDebug() << "hiding VKB as requested by QInputMethod::hide()";
    [m_textResponder resignFirstResponder];
}

void QIOSInputContext::clearCurrentFocusObject()
{
    if (QWindow *focusWindow = qApp->focusWindow())
        static_cast<QWindowPrivate *>(QObjectPrivate::get(focusWindow))->clearFocusObject();
}

bool QIOSInputContext::isInputPanelVisible() const
{
    return m_keyboardListener->m_keyboardVisible;
}

void QIOSInputContext::cursorRectangleChanged()
{
    if (!m_keyboardListener->m_keyboardVisibleAndDocked || !qApp->focusObject())
        return;

    // Check if the cursor has changed position inside the input item. Since
    // qApp->inputMethod()->cursorRectangle() will also change when the input item
    // itself moves, we need to ask the focus object for ImCursorRectangle:
    static QPoint prevCursor;
    QInputMethodQueryEvent queryEvent(Qt::ImCursorRectangle);
    QCoreApplication::sendEvent(qApp->focusObject(), &queryEvent);
    QPoint cursor = queryEvent.value(Qt::ImCursorRectangle).toRect().topLeft();
    if (cursor != prevCursor)
        scrollToCursor();
    prevCursor = cursor;
}

void QIOSInputContext::scrollToCursor()
{
    if (!isQtApplication())
        return;

    if (m_keyboardListener.state == UIGestureRecognizerStatePossible && m_keyboardListener.numberOfTouches == 1) {
        // Don't scroll to the cursor if the user is touching the screen and possibly
        // trying to trigger the hide-keyboard gesture.
        qImDebug() << "preventing scrolling to cursor as we're still waiting for a possible gesture";
        return;
    }

    UIView *view = m_keyboardListener->m_viewController.view;
    if (view.window != focusView().window)
        return;

    const int margin = 20;
    QRectF translatedCursorPos = qApp->inputMethod()->cursorRectangle();
    translatedCursorPos.translate(focusView().qwindow->geometry().topLeft());

    qreal keyboardY = [view convertRect:m_keyboardListener->m_keyboardEndRect fromView:nil].origin.y;
    int statusBarY = qGuiApp->primaryScreen()->availableGeometry().y();

    scroll((translatedCursorPos.bottomLeft().y() < keyboardY - margin) ? 0
        : qMin(view.bounds.size.height - keyboardY, translatedCursorPos.y() - statusBarY - margin));
}

void QIOSInputContext::scroll(int y)
{
    UIView *rootView = m_keyboardListener->m_viewController.view;

    CATransform3D translationTransform = CATransform3DMakeTranslation(0.0, -y, 0.0);
    if (CATransform3DEqualToTransform(translationTransform, rootView.layer.sublayerTransform))
        return;

    QPointer<QIOSInputContext> self = this;
    [UIView animateWithDuration:m_keyboardListener->m_duration delay:0
        options:(m_keyboardListener->m_curve << 16) | UIViewAnimationOptionBeginFromCurrentState
        animations:^{
            // The sublayerTransform property of CALayer is not implicitly animated for a
            // layer-backed view, even inside a UIView animation block, so we need to set up
            // an explicit CoreAnimation animation. Since there is no predefined media timing
            // function that matches the custom keyboard animation curve we cheat by asking
            // the view for an animation of another property, which will give us an animation
            // that matches the parameters we passed to [UIView animateWithDuration] above.
            // The reason we ask for the animation of 'backgroundColor' is that it's a simple
            // property that will not return a compound animation, like eg. bounds will.
            NSObject *action = (NSObject*)[rootView actionForLayer:rootView.layer forKey:@"backgroundColor"];

            CABasicAnimation *animation;
            if ([action isKindOfClass:[CABasicAnimation class]]) {
                animation = static_cast<CABasicAnimation*>(action);
                animation.keyPath = @"sublayerTransform"; // Instead of backgroundColor
            } else {
                animation = [CABasicAnimation animationWithKeyPath:@"sublayerTransform"];
            }

            CATransform3D currentSublayerTransform = static_cast<CALayer *>([rootView.layer presentationLayer]).sublayerTransform;
            animation.fromValue = [NSValue valueWithCATransform3D:currentSublayerTransform];
            animation.toValue = [NSValue valueWithCATransform3D:translationTransform];
            [rootView.layer addAnimation:animation forKey:@"AnimateSubLayerTransform"];
            rootView.layer.sublayerTransform = translationTransform;

            [rootView.qtViewController updateProperties];
        }
        completion:^(BOOL){
            if (self)
                [m_keyboardListener handleKeyboardRectChanged];
        }
    ];
}

// -------------------------------------------------------------------------

void QIOSInputContext::setFocusObject(QObject *focusObject)
{
    Q_UNUSED(focusObject);

    qImDebug() << "new focus object =" << focusObject;

    if (m_keyboardListener.state == UIGestureRecognizerStateChanged) {
        // A new focus object may be set as part of delivering touch events to
        // application during the hide-keyboard gesture, but we don't want that
        // to result in a new object getting focus and bringing the keyboard up
        // again.
        qImDebug() << "clearing focus object" << focusObject << "as hide-keyboard gesture is active";
        clearCurrentFocusObject();
        return;
    }

    reset();

    if (m_keyboardListener->m_keyboardVisibleAndDocked)
        scrollToCursor();
}

void QIOSInputContext::focusWindowChanged(QWindow *focusWindow)
{
    Q_UNUSED(focusWindow);

    qImDebug() << "new focus window =" << focusWindow;

    reset();

    [m_keyboardListener handleKeyboardRectChanged];
    if (m_keyboardListener->m_keyboardVisibleAndDocked)
        scrollToCursor();
}

/*!
    Called by the input item to inform the platform input methods when there has been
    state changes in editor's input method query attributes. When calling the function
    \a queries parameter has to be used to tell what has changes, which input method
    can use to make queries for attributes it's interested with QInputMethodQueryEvent.
*/
void QIOSInputContext::update(Qt::InputMethodQueries updatedProperties)
{
    // Mask for properties that we are interested in and see if any of them changed
    updatedProperties &= (Qt::ImEnabled | Qt::ImHints | Qt::ImQueryInput | Qt::ImPlatformData);

    if (updatedProperties & Qt::ImEnabled) {
        // Switching on and off input-methods needs a re-fresh of hints and platform
        // data when we turn them on again, as the IM state we have may have been
        // invalidated when IM was switched off. We could defer this until we know
        // if IM was turned on, to limit the extra query parameters, but for simplicity
        // we always do the update.
        updatedProperties |= (Qt::ImHints | Qt::ImPlatformData);
    }

    qImDebug() << "fw =" << qApp->focusWindow() << "fo =" << qApp->focusObject();

    Qt::InputMethodQueries changedProperties = m_imeState.update(updatedProperties);
    if (changedProperties & (Qt::ImEnabled | Qt::ImHints | Qt::ImPlatformData)) {
        // Changes to enablement or hints require virtual keyboard reconfigure

        qImDebug() << "changed IM properties" << changedProperties << "require keyboard reconfigure";

        if (inputMethodAccepted()) {
            qImDebug() << "replacing text responder with new text responder";
            [m_textResponder autorelease];
            m_textResponder = [[QIOSTextInputResponder alloc] initWithInputContext:this];
            [m_textResponder becomeFirstResponder];
        } else if ([UIResponder currentFirstResponder] == m_textResponder) {
            qImDebug() << "IM not enabled, resigning text responder as first responder";
            [m_textResponder resignFirstResponder];
        } else {
            qImDebug() << "IM not enabled. Text responder not first responder. Nothing to do";
        }
    } else {
        [m_textResponder notifyInputDelegate:changedProperties];
    }
}

bool QIOSInputContext::inputMethodAccepted() const
{
    // The IM enablement state is based on the last call to update()
    bool lastKnownImEnablementState = m_imeState.currentState.value(Qt::ImEnabled).toBool();

#if !defined(QT_NO_DEBUG)
    // QPlatformInputContext keeps a cached value of the current IM enablement state that is
    // updated by QGuiApplication when the current focus object changes, or by QInputMethod's
    // update() function. If the focus object changes, but the change is not propagated as
    // a signal to QGuiApplication due to bugs in the widget/graphicsview/qml stack, we'll
    // end up with a stale value for QPlatformInputContext::inputMethodAccepted(). To be on
    // the safe side we always use our own cached value to decide if IM is enabled, and try
    // to detect the case where the two values are out of sync.
    if (lastKnownImEnablementState != QPlatformInputContext::inputMethodAccepted())
        qWarning("QPlatformInputContext::inputMethodAccepted() does not match actual focus object IM enablement!");
#endif

    return lastKnownImEnablementState;
}

/*!
    Called by the input item to reset the input method state.
*/
void QIOSInputContext::reset()
{
    update(Qt::ImQueryAll);

    [m_textResponder setMarkedText:@"" selectedRange:NSMakeRange(0, 0)];
    [m_textResponder notifyInputDelegate:Qt::ImQueryInput];
}

/*!
    Commits the word user is currently composing to the editor. The function is
    mostly needed by the input methods with text prediction features and by the
    methods where the script used for typing characters is different from the
    script that actually gets appended to the editor. Any kind of action that
    interrupts the text composing needs to flush the composing state by calling the
    commit() function, for example when the cursor is moved elsewhere.
*/
void QIOSInputContext::commit()
{
    [m_textResponder unmarkText];
    [m_textResponder notifyInputDelegate:Qt::ImSurroundingText];
}
