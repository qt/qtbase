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
#include "qiostextresponder.h"
#include "qioswindow.h"
#include "quiview.h"

#include <QGuiApplication>
#include <QtGui/private/qwindow_p.h>

static QUIView *focusView()
{
    return qApp->focusWindow() ?
        reinterpret_cast<QUIView *>(qApp->focusWindow()->winId()) : 0;
}

@interface QIOSKeyboardListener : UIGestureRecognizer {
@public
    QIOSInputContext *m_context;
    BOOL m_keyboardVisible;
    BOOL m_keyboardVisibleAndDocked;
    BOOL m_touchPressWhileKeyboardVisible;
    BOOL m_keyboardHiddenByGesture;
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
    self = [super initWithTarget:self action:@selector(gestureTriggered)];
    if (self) {
        m_context = context;
        m_keyboardVisible = NO;
        m_keyboardVisibleAndDocked = NO;
        m_touchPressWhileKeyboardVisible = NO;
        m_keyboardHiddenByGesture = NO;
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
            // keyboard is not visible. Note that we never trigger the gesture the way it is intended
            // since we don't want to cancel touch events and interrupt flicking etc. Instead we use
            // the gesture framework more as an event filter and hide the keyboard silently.
            self.enabled = NO;
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
    self.enabled = YES;
    if (!m_duration) {
        m_duration = [[notification.userInfo objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
        m_curve = UIViewAnimationCurve([[notification.userInfo objectForKey:UIKeyboardAnimationCurveUserInfoKey] integerValue]);
    }
    m_context->scrollToCursor();
}

- (void) keyboardWillHide:(NSNotification *)notification
{
    // Note that UIKeyboardWillHideNotification is also sendt when the keyboard is undocked.
    m_keyboardVisibleAndDocked = NO;
    m_keyboardEndRect = [[[notification userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    if (!m_keyboardHiddenByGesture) {
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

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    CGPoint p = [[touches anyObject] locationInView:m_viewController.view.window];
    if (CGRectContainsPoint(m_keyboardEndRect, p)) {
        m_keyboardHiddenByGesture = YES;
        m_context->hideVirtualKeyboard();
    }

    [super touchesMoved:touches withEvent:event];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    Q_ASSERT(m_keyboardVisibleAndDocked);
    m_touchPressWhileKeyboardVisible = YES;
    [super touchesBegan:touches withEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    m_touchPressWhileKeyboardVisible = NO;
    [self performSelectorOnMainThread:@selector(touchesEndedPostDelivery) withObject:nil waitUntilDone:NO];
    [super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    m_touchPressWhileKeyboardVisible = NO;
    [self performSelectorOnMainThread:@selector(touchesEndedPostDelivery) withObject:nil waitUntilDone:NO];
    [super touchesCancelled:touches withEvent:event];
}

- (void)touchesEndedPostDelivery
{
    // Do some clean-up _after_ touchEnd has been delivered to QUIView
    m_keyboardHiddenByGesture = NO;
    if (!m_keyboardVisibleAndDocked) {
        self.enabled = NO;
        if (qApp->focusObject()) {
            // UI Controls are told to gain focus on touch release. So when the 'hide keyboard' gesture
            // finishes, the final touch end can trigger a control to gain focus. This is in conflict with
            // the gesture, so we clear focus once more as a work-around.
            static_cast<QWindowPrivate *>(QObjectPrivate::get(qApp->focusWindow()))->clearFocusObject();
        }
    } else {
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

    if (qApp && qApp->focusObject())
        QCoreApplication::sendEvent(qApp->focusObject(), &newState);

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
}

void QIOSInputContext::hideInputPanel()
{
    // No-op, keyboard controlled fully by platform based on focus
}

void QIOSInputContext::hideVirtualKeyboard()
{
    static_cast<QWindowPrivate *>(QObjectPrivate::get(qApp->focusWindow()))->clearFocusObject();
}

bool QIOSInputContext::isInputPanelVisible() const
{
    return m_keyboardListener->m_keyboardVisible;
}

void QIOSInputContext::cursorRectangleChanged()
{
    if (!m_keyboardListener->m_keyboardVisibleAndDocked)
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

    if (m_keyboardListener->m_touchPressWhileKeyboardVisible) {
        // Don't scroll to the cursor if the user is touching the screen. This
        // interferes with selection and the 'hide keyboard' gesture. Instead
        // we update scrolling upon touchEnd.
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

            animation.fromValue = [NSValue valueWithCATransform3D:rootView.layer.sublayerTransform];
            animation.toValue = [NSValue valueWithCATransform3D:translationTransform];
            [rootView.layer addAnimation:animation forKey:@"AnimateSubLayerTransform"];
            rootView.layer.sublayerTransform = translationTransform;
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

    reset();

    if (m_keyboardListener->m_keyboardVisibleAndDocked)
        scrollToCursor();
}

void QIOSInputContext::focusWindowChanged(QWindow *focusWindow)
{
    Q_UNUSED(focusWindow);

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

    Qt::InputMethodQueries changedProperties = m_imeState.update(updatedProperties);
    if (changedProperties & (Qt::ImEnabled | Qt::ImHints | Qt::ImPlatformData)) {
        // Changes to enablement or hints require virtual keyboard reconfigure
        [m_textResponder release];
        m_textResponder = [[QIOSTextInputResponder alloc] initWithInputContext:this];
        [m_textResponder reloadInputViews];
    } else {
        [m_textResponder notifyInputDelegate:changedProperties];
    }
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

// -------------------------------------------------------------------------

@interface QUIView (InputMethods)
- (void)reloadInputViews;
@end

@implementation QUIView (InputMethods)
- (void)reloadInputViews
{
    qApp->inputMethod()->reset();
}
@end
