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
#include "qiosinputcontext.h"
#include "qioswindow.h"
#include <QGuiApplication>

@interface QIOSKeyboardListener : NSObject {
@public
    QIOSInputContext *m_context;
    BOOL m_keyboardVisible;
    BOOL m_keyboardVisibleAndDocked;
    QRectF m_keyboardRect;
    QRectF m_keyboardEndRect;
    NSTimeInterval m_duration;
    UIViewAnimationCurve m_curve;
    UIViewController *m_viewController;
}
@end

@implementation QIOSKeyboardListener

- (id)initWithQIOSInputContext:(QIOSInputContext *)context
{
    self = [super init];
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

- (QRectF) getKeyboardRect:(NSNotification *)notification
{
    // For Qt applications we rotate the keyboard rect to align with the screen
    // orientation (which is the interface orientation of the root view controller).
    // For hybrid apps we follow native behavior, and return the rect unmodified:
    CGRect keyboardFrame = [[notification userInfo][UIKeyboardFrameEndUserInfoKey] CGRectValue];
    if (isQtApplication()) {
        UIView *view = m_viewController.view;
        return fromCGRect(CGRectOffset([view convertRect:keyboardFrame fromView:view.window], 0, -view.bounds.origin.y));
    } else {
        return fromCGRect(keyboardFrame);
    }
}

- (void) keyboardDidChangeFrame:(NSNotification *)notification
{
    m_keyboardRect = [self getKeyboardRect:notification];
    m_context->emitKeyboardRectChanged();

    BOOL visible = m_keyboardRect.intersects(fromCGRect([UIScreen mainScreen].bounds));
    if (m_keyboardVisible != visible) {
        m_keyboardVisible = visible;
        m_context->emitInputPanelVisibleChanged();
    }

    // If the keyboard was visible and docked from before, this is just a geometry
    // change (normally caused by an orientation change). In that case, update scroll:
    if (m_keyboardVisibleAndDocked)
        m_context->scrollRootView();
}

- (void) keyboardWillShow:(NSNotification *)notification
{
    // Note that UIKeyboardWillShowNotification is only sendt when the keyboard is docked.
    m_keyboardVisibleAndDocked = YES;
    m_keyboardEndRect = [self getKeyboardRect:notification];
    if (!m_duration) {
        m_duration = [notification.userInfo[UIKeyboardAnimationDurationUserInfoKey] doubleValue];
        m_curve = [notification.userInfo[UIKeyboardAnimationCurveUserInfoKey] integerValue] << 16;
    }
    m_context->scrollRootView();
}

- (void) keyboardWillHide:(NSNotification *)notification
{
    // Note that UIKeyboardWillHideNotification is also sendt when the keyboard is undocked.
    m_keyboardVisibleAndDocked = NO;
    m_keyboardEndRect = [self getKeyboardRect:notification];
    m_context->scrollRootView();
}

@end

QIOSInputContext::QIOSInputContext()
    : QPlatformInputContext()
    , m_keyboardListener([[QIOSKeyboardListener alloc] initWithQIOSInputContext:this])
    , m_focusView(0)
    , m_hasPendingHideRequest(false)
    , m_inSetFocusObject(false)
{
    if (isQtApplication())
        connect(qGuiApp->inputMethod(), &QInputMethod::cursorRectangleChanged, this, &QIOSInputContext::scrollRootView);
    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &QIOSInputContext::focusWindowChanged);
}

QIOSInputContext::~QIOSInputContext()
{
    [m_keyboardListener release];
    [m_focusView release];
}

QRectF QIOSInputContext::keyboardRect() const
{
    return m_keyboardListener->m_keyboardRect;
}

void QIOSInputContext::showInputPanel()
{
    // Documentation tells that one should call (and recall, if necessary) becomeFirstResponder/resignFirstResponder
    // to show/hide the keyboard. This is slightly inconvenient, since there exist no API to get the current first
    // responder. Rather than searching for it from the top, we let the active QIOSWindow tell us which view to use.
    // Note that Qt will forward keyevents to whichever QObject that needs it, regardless of which UIView the input
    // actually came from. So in this respect, we're undermining iOS' responder chain.
    m_hasPendingHideRequest = false;
    [m_focusView becomeFirstResponder];
}

void QIOSInputContext::hideInputPanel()
{
    // Delay hiding the keyboard for cases where the user is transferring focus between
    // 'line edits'. In that case the 'line edit' that lost focus will close the input
    // panel, just to see that the new 'line edit' will open it again:
    m_hasPendingHideRequest = true;
    dispatch_async(dispatch_get_main_queue(), ^{
        if (m_hasPendingHideRequest)
            [m_focusView resignFirstResponder];
    });
}

bool QIOSInputContext::isInputPanelVisible() const
{
    return m_keyboardListener->m_keyboardVisible;
}

void QIOSInputContext::setFocusObject(QObject *)
{
    m_inputItemTransform = qApp->inputMethod()->inputItemTransform();

    if (!m_focusView || !m_focusView.isFirstResponder)
        return;

    // Since m_focusView is the first responder, it means that the keyboard is open and we
    // should update keyboard layout. But there seem to be no way to tell it to reread the
    // UITextInputTraits from m_focusView. To work around that, we quickly resign first
    // responder status just to reassign it again. To not remove the focusObject in the same
    // go, we need to call the super implementation of resignFirstResponder. Since the call
    // will cause a 'keyboardWillHide' notification to be sendt, we also block scrollRootView
    // to avoid artifacts:
    m_inSetFocusObject = true;
    SEL sel = @selector(resignFirstResponder);
    [[m_focusView superclass] instanceMethodForSelector:sel](m_focusView, sel);
    m_inSetFocusObject = false;
    [m_focusView becomeFirstResponder];
}

void QIOSInputContext::focusWindowChanged(QWindow *focusWindow)
{
    UIView<UIKeyInput> *view = reinterpret_cast<UIView<UIKeyInput> *>(focusWindow->handle()->winId());
    if ([m_focusView isFirstResponder])
        [view becomeFirstResponder];
    [m_focusView release];
    m_focusView = [view retain];
}

void QIOSInputContext::scrollRootView()
{
    // Scroll the root view (screen) if:
    // - our backend controls the root view controller on the main screen (no hybrid app)
    // - the focus object is on the same screen as the keyboard.
    // - the first responder is a QUIView, and not some other foreign UIView.
    // - the keyboard is docked. Otherwise the user can move the keyboard instead.
    // - the inputItem has not been moved/scrolled
    if (!isQtApplication() || !m_focusView || m_inSetFocusObject)
        return;

    if (m_inputItemTransform != qApp->inputMethod()->inputItemTransform()) {
        // The inputItem has moved since the last scroll update. To avoid competing
        // with the application where the cursor/inputItem should be, we bail:
        return;
    }

    UIView *view = m_keyboardListener->m_viewController.view;
    qreal scrollTo = 0;

    if (m_focusView.isFirstResponder
            && m_keyboardListener->m_keyboardVisibleAndDocked
            && m_focusView.window == view.window) {
        QRectF cursorRect = qGuiApp->inputMethod()->cursorRectangle();
        cursorRect.translate(m_focusView.qwindow->geometry().topLeft());
        qreal keyboardY = m_keyboardListener->m_keyboardEndRect.y();
        int statusBarY = qGuiApp->primaryScreen()->availableGeometry().y();
        const int margin = 20;

        if (cursorRect.bottomLeft().y() > keyboardY - margin)
            scrollTo = qMin(view.bounds.size.height - keyboardY, cursorRect.y() - statusBarY - margin);
    }

    if (scrollTo != view.bounds.origin.y) {
        // Scroll the view the same way a UIScrollView works: by changing bounds.origin:
        CGRect newBounds = view.bounds;
        newBounds.origin.y = scrollTo;
        [UIView animateWithDuration:m_keyboardListener->m_duration delay:0
            options:m_keyboardListener->m_curve
            animations:^{ view.bounds = newBounds; }
            completion:0];
    }
}
