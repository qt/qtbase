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

#include "qioswindow.h"
#include "qioscontext.h"
#include "qiosscreen.h"
#include "qiosapplicationdelegate.h"
#include "qiosorientationlistener.h"
#include "qiosviewcontroller.h"

#import <QuartzCore/CAEAGLLayer.h>

#include <QtGui/QKeyEvent>
#include <qpa/qwindowsysteminterface.h>

#include <QtDebug>

static CGRect toCGRect(const QRect &rect)
{
    return CGRectMake(rect.x(), rect.y(), rect.width(), rect.height());
}

static QRect fromCGRect(const CGRect &rect)
{
    return QRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

@implementation EAGLView

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
            [NSNumber numberWithBool:YES], kEAGLDrawablePropertyRetainedBacking,
            kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];

        // Set up text input
        autocapitalizationType = UITextAutocapitalizationTypeNone;
        autocorrectionType = UITextAutocorrectionTypeNo;
        enablesReturnKeyAutomatically = NO;
        keyboardAppearance = UIKeyboardAppearanceDefault;
        keyboardType = UIKeyboardTypeDefault;
        returnKeyType = UIReturnKeyDone;
        secureTextEntry = NO;
    }

    return self;
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

    QWindowSystemInterface::handleGeometryChange(m_qioswindow->window(), fromCGRect(self.frame));
    [super layoutSubviews];
}

- (void)sendMouseEventForTouches:(NSSet *)touches withEvent:(UIEvent *)event fakeButtons:(Qt::MouseButtons)buttons
{
    UITouch *touch = [touches anyObject];
    CGPoint locationInView = [touch locationInView:self];
    CGFloat scaleFactor = [self contentScaleFactor];
    QPoint p(locationInView.x * scaleFactor, locationInView.y * scaleFactor);

    // TODO handle global touch point? for status bar?
    QWindowSystemInterface::handleMouseEvent(m_qioswindow->window(), (ulong)(event.timestamp*1000), p, p, buttons);
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self sendMouseEventForTouches:touches withEvent:event fakeButtons:Qt::LeftButton];
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self sendMouseEventForTouches:touches withEvent:event fakeButtons:Qt::LeftButton];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self sendMouseEventForTouches:touches withEvent:event fakeButtons:Qt::NoButton];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self sendMouseEventForTouches:touches withEvent:event fakeButtons:Qt::NoButton];
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


QT_BEGIN_NAMESPACE

QIOSWindow::QIOSWindow(QWindow *window)
    : QPlatformWindow(window)
    , m_view([[EAGLView alloc] initWithQIOSWindow:this])
{
    if ([[UIApplication sharedApplication].delegate isKindOfClass:[QIOSApplicationDelegate class]])
        [[UIApplication sharedApplication].delegate.window.rootViewController.view addSubview:m_view];

    setWindowState(window->windowState());
}

QIOSWindow::~QIOSWindow()
{
    [m_view release];
}

void QIOSWindow::setGeometry(const QRect &rect)
{
    // If the window is in fullscreen, just bookkeep the requested
    // geometry in case the window goes into Qt::WindowNoState later:
    QPlatformWindow::setGeometry(rect);
    if (window()->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen))
        return;

    // Since we don't support transformations on the UIView, we can set the frame
    // directly and let UIKit deal with translating that into bounds and center.
    // Changing the size of the view will end up in a call to -[EAGLView layoutSubviews]
    // which will update QWindowSystemInterface with the new size.
    m_view.frame = toCGRect(rect);
}

void QIOSWindow::setWindowState(Qt::WindowState state)
{
    // FIXME: Figure out where or how we should disable/enable the statusbar.
    // Perhaps setting QWindow to maximized should also mean that we'll show
    // the statusbar, and vice versa for fullscreen?

    switch (state) {
    case Qt::WindowMaximized:
    case Qt::WindowFullScreen:
        m_view.frame = toCGRect(QRect(QPoint(0, 0), window()->screen()->availableSize()));
        m_view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        break;
    default:
        m_view.frame = toCGRect(geometry());
        m_view.autoresizingMask = UIViewAutoresizingNone;
        break;
    }
}

void QIOSWindow::handleContentOrientationChange(Qt::ScreenOrientation orientation)
{
    // Keep the status bar in sync with content orientation. This will ensure
    // that the task bar (and associated gestures) are aligned correctly:
    UIDeviceOrientation uiOrientation = convertToUIOrientation(orientation);
    [[UIApplication sharedApplication] setStatusBarOrientation:uiOrientation animated:NO];
}

GLuint QIOSWindow::framebufferObject(const QIOSContext &context) const
{
    static GLuint framebuffer = 0;

    // FIXME: Cache context and recreate framebuffer if window
    // is used with a different context then last time.

    if (!framebuffer) {
        EAGLContext* eaglContext = context.nativeContext();

        [EAGLContext setCurrentContext:eaglContext];

        // Create the framebuffer and bind it
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        GLint width;
        GLint height;

        // Create a color renderbuffer, allocate storage for it,
        // and attach it to the framebuffer’s color attachment point.
        GLuint colorRenderbuffer;
        glGenRenderbuffers(1, &colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
        [eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:static_cast<CAEAGLLayer *>(m_view.layer)];
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);

        QSurfaceFormat requestedFormat = context.format();
        if (requestedFormat.depthBufferSize() > 0 || requestedFormat.stencilBufferSize() > 0) {
            // Create a depth or depth/stencil renderbuffer, allocate storage for it,
            // and attach it to the framebuffer’s depth attachment point.
            GLuint depthRenderbuffer;
            glGenRenderbuffers(1, &depthRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);

            // FIXME: Support more fine grained control over depth/stencil buffer sizes
            if (requestedFormat.stencilBufferSize() > 0) {
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, width, height);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
            } else {
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
            }
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }

    return framebuffer;
}

QT_END_NAMESPACE
