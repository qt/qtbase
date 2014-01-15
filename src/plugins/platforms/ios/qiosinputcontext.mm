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
#include "quiview.h"
#include <QGuiApplication>

@interface QIOSKeyboardListener : NSObject {
@public
    QIOSInputContext *m_context;
    BOOL m_keyboardVisible;
    BOOL m_keyboardVisibleAndDocked;
    BOOL m_ignoreKeyboardChanges;
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
        m_ignoreKeyboardChanges = NO;
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
    if (m_ignoreKeyboardChanges)
        return;
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
        m_context->scrollToCursor();
}

- (void) keyboardWillShow:(NSNotification *)notification
{
    if (m_ignoreKeyboardChanges)
        return;
    // Note that UIKeyboardWillShowNotification is only sendt when the keyboard is docked.
    m_keyboardVisibleAndDocked = YES;
    m_keyboardEndRect = [self getKeyboardRect:notification];
    if (!m_duration) {
        m_duration = [notification.userInfo[UIKeyboardAnimationDurationUserInfoKey] doubleValue];
        m_curve = UIViewAnimationCurve([notification.userInfo[UIKeyboardAnimationCurveUserInfoKey] integerValue] << 16);
    }
    m_context->scrollToCursor();
}

- (void) keyboardWillHide:(NSNotification *)notification
{
    if (m_ignoreKeyboardChanges)
        return;
    // Note that UIKeyboardWillHideNotification is also sendt when the keyboard is undocked.
    m_keyboardVisibleAndDocked = NO;
    m_keyboardEndRect = [self getKeyboardRect:notification];
    m_context->scroll(0);
}

@end

QIOSInputContext::QIOSInputContext()
    : QPlatformInputContext()
    , m_keyboardListener([[QIOSKeyboardListener alloc] initWithQIOSInputContext:this])
    , m_focusView(0)
    , m_hasPendingHideRequest(false)
    , m_focusObject(0)
{
    if (isQtApplication())
        connect(qGuiApp->inputMethod(), &QInputMethod::cursorRectangleChanged, this, &QIOSInputContext::cursorRectangleChanged);
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

void QIOSInputContext::setFocusObject(QObject *focusObject)
{
    m_focusObject = focusObject;

    if (!focusObject || !m_focusView || !m_focusView.isFirstResponder) {
        scroll(0);
        return;
    }

    reset();

    if (m_keyboardListener->m_keyboardVisibleAndDocked)
        scrollToCursor();
}

void QIOSInputContext::focusWindowChanged(QWindow *focusWindow)
{
    QUIView *view = focusWindow ? reinterpret_cast<QUIView *>(focusWindow->handle()->winId()) : 0;
    if ([m_focusView isFirstResponder])
        [view becomeFirstResponder];
    [m_focusView release];
    m_focusView = [view retain];

    if (view.window != m_keyboardListener->m_viewController.view)
        scroll(0);
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
    QCoreApplication::sendEvent(m_focusObject, &queryEvent);
    QPoint cursor = queryEvent.value(Qt::ImCursorRectangle).toRect().topLeft();
    if (cursor != prevCursor)
        scrollToCursor();
    prevCursor = cursor;
}

void QIOSInputContext::scrollToCursor()
{
    if (!isQtApplication() || !m_focusView)
        return;

    UIView *view = m_keyboardListener->m_viewController.view;
    if (view.window != m_focusView.window)
        return;

    const int margin = 20;
    QRectF translatedCursorPos = qApp->inputMethod()->cursorRectangle();
    translatedCursorPos.translate(m_focusView.qwindow->geometry().topLeft());
    qreal keyboardY = m_keyboardListener->m_keyboardEndRect.y();
    int statusBarY = qGuiApp->primaryScreen()->availableGeometry().y();

    scroll((translatedCursorPos.bottomLeft().y() < keyboardY - margin) ? 0
        : qMin(view.bounds.size.height - keyboardY, translatedCursorPos.y() - statusBarY - margin));
}

void QIOSInputContext::scroll(int y)
{
    // Scroll the view the same way a UIScrollView
    // works: by changing bounds.origin:
    UIView *view = m_keyboardListener->m_viewController.view;
    if (y == view.bounds.origin.y)
        return;

    CGRect newBounds = view.bounds;
    newBounds.origin.y = y;
    [UIView animateWithDuration:m_keyboardListener->m_duration delay:0
        options:m_keyboardListener->m_curve
        animations:^{ view.bounds = newBounds; }
        completion:0];
}

void QIOSInputContext::update(Qt::InputMethodQueries query)
{
    [m_focusView updateInputMethodWithQuery:query];
}

void QIOSInputContext::reset()
{
    // Since the call to reset will cause a 'keyboardWillHide'
    // notification to be sendt, we block keyboard nofifications to avoid artifacts:
    m_keyboardListener->m_ignoreKeyboardChanges = true;
    [m_focusView reset];
    m_keyboardListener->m_ignoreKeyboardChanges = false;
}

void QIOSInputContext::commit()
{
    [m_focusView commit];
}

