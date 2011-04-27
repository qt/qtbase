/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#import <QuartzCore/CAEAGLLayer.h>

#include "quikitwindow.h"

#include "quikitscreen.h"

#include <QtDebug>
#include <QtGui/QApplication>
#include <QtGui/QKeyEvent>
#include <QtGui/QPlatformGLContext>
#include <QtGui/QWindowSystemInterface>

#include <QtDebug>

class EAGLPlatformContext : public QPlatformGLContext
{
public:
    EAGLPlatformContext(EAGLView *view)
        : mView(view)
    {
        mFormat.setWindowApi(QPlatformWindowFormat::OpenGL);
        mFormat.setDepthBufferSize(24);
        mFormat.setAccumBufferSize(0);
        mFormat.setRedBufferSize(8);
        mFormat.setGreenBufferSize(8);
        mFormat.setBlueBufferSize(8);
        mFormat.setAlphaBufferSize(8);
        mFormat.setStencilBufferSize(8);
        mFormat.setSampleBuffers(false);
        mFormat.setSamples(1);
//        mFormat.setSwapInterval(?)
        mFormat.setDoubleBuffer(true);
        mFormat.setDepth(true);
        mFormat.setRgba(true);
        mFormat.setAlpha(true);
        mFormat.setAccum(false);
        mFormat.setStencil(true);
        mFormat.setStereo(false);
        mFormat.setDirectRendering(false);

        EAGLContext *aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
        [mView setContext:aContext];
    }

    ~EAGLPlatformContext() { }

    void makeCurrent()
    {
        QPlatformGLContext::makeCurrent();
        [mView makeCurrent];
    }

    void doneCurrent()
    {
        QPlatformGLContext::doneCurrent();
    }

    void swapBuffers()
    {
        [mView presentFramebuffer];
    }

    void* getProcAddress(const QString& ) { return 0; }

    QPlatformWindowFormat platformWindowFormat() const
    {
        return mFormat;
    }

private:
    EAGLView *mView;

    QPlatformWindowFormat mFormat;
};

@implementation EAGLView

+ (Class)layerClass
{
    return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame])) {
        CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
        eaglLayer.opaque = TRUE;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:YES], kEAGLDrawablePropertyRetainedBacking,
                                        kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                        nil];
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

- (void)setContext:(EAGLContext *)newContext
{
    if (mContext != newContext)
    {
        [self deleteFramebuffer];
        [mContext release];
        mContext = [newContext retain];
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)presentFramebuffer
{
    if (mContext) {
        [EAGLContext setCurrentContext:mContext];
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, mColorRenderbuffer);
        [mContext presentRenderbuffer:GL_RENDERBUFFER_OES];
    }
}

- (void)deleteFramebuffer
{
    if (mContext)
    {
        [EAGLContext setCurrentContext:mContext];
        if (mFramebuffer) {
            glDeleteFramebuffersOES(1, &mFramebuffer);
            mFramebuffer = 0;
        }
        if (mColorRenderbuffer) {
            glDeleteRenderbuffersOES(1, &mColorRenderbuffer);
            mColorRenderbuffer = 0;
        }
        if (mDepthRenderbuffer) {
            glDeleteRenderbuffersOES(1, &mDepthRenderbuffer);
            mDepthRenderbuffer = 0;
        }
    }
}

- (void)createFramebuffer
{
    if (mContext && !mFramebuffer)
    {
        [EAGLContext setCurrentContext:mContext];
        glGenFramebuffersOES(1, &mFramebuffer);
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, mFramebuffer);

        glGenRenderbuffersOES(1, &mColorRenderbuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, mColorRenderbuffer);
        [mContext renderbufferStorage:GL_RENDERBUFFER_OES fromDrawable:(CAEAGLLayer *)self.layer];
        glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &mFramebufferWidth);
        glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &mFramebufferHeight);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, mColorRenderbuffer);

        glGenRenderbuffersOES(1, &mDepthRenderbuffer);
        glBindRenderbufferOES(GL_RENDERBUFFER_OES, mDepthRenderbuffer);
        glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH24_STENCIL8_OES, mFramebufferWidth, mFramebufferHeight);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, mDepthRenderbuffer);
        glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_STENCIL_ATTACHMENT_OES, GL_RENDERBUFFER_OES, mDepthRenderbuffer);

        if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES)
            NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
    }
}

- (void)makeCurrent
{
    if (mContext)
    {
        [EAGLContext setCurrentContext:mContext];
        if (!mFramebuffer)
            [self createFramebuffer];
        glBindFramebufferOES(GL_FRAMEBUFFER_OES, mFramebuffer);
        glViewport(0, 0, mFramebufferWidth, mFramebufferHeight);
    }
}

- (void)setWindow:(QPlatformWindow *)window
{
    mWindow = window;
}

- (void)sendMouseEventForTouches:(NSSet *)touches withEvent:(UIEvent *)event fakeButtons:(Qt::MouseButtons)buttons
{
    UITouch *touch = [touches anyObject];
    CGPoint locationInView = [touch locationInView:self];
    QPoint p(locationInView.x, locationInView.y);
    // TODO handle global touch point? for status bar?
    QWindowSystemInterface::handleMouseEvent(mWindow->widget(), (ulong)(event.timestamp*1000),
        p, p, buttons);
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

// ------- Text Input ----------

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
    QKeyEvent *ev;
    int key = 0;
    if ([text isEqualToString:@"\n"])
        key = (int)Qt::Key_Return;
    ev = new QKeyEvent(QEvent::KeyPress,
                       key,
                       Qt::NoModifier,
                       QString::fromUtf8([text UTF8String])
                       );
    qApp->postEvent(qApp->focusWidget(), ev);
    ev = new QKeyEvent(QEvent::KeyRelease,
                       key,
                       Qt::NoModifier,
                       QString::fromUtf8([text UTF8String])
                       );
    qApp->postEvent(qApp->focusWidget(), ev);
}

- (void)deleteBackward
{
    QKeyEvent *ev;
    ev = new QKeyEvent(QEvent::KeyPress,
                       (int)Qt::Key_Backspace,
                       Qt::NoModifier
                       );
    qApp->postEvent(qApp->focusWidget(), ev);
    ev = new QKeyEvent(QEvent::KeyRelease,
                       (int)Qt::Key_Backspace,
                       Qt::NoModifier
                       );
    qApp->postEvent(qApp->focusWidget(), ev);
}

@end

QT_BEGIN_NAMESPACE

QUIKitWindow::QUIKitWindow(QWidget *tlw) :
    QPlatformWindow(tlw),
    mWindow(nil),
    mContext(0)
{
    mScreen = static_cast<QUIKitScreen *>(QPlatformScreen::platformScreenForWidget(tlw));
    CGRect screenBounds = [mScreen->uiScreen() bounds];
    QRect geom(screenBounds.origin.x, screenBounds.origin.y, screenBounds.size.width, screenBounds.size.height);
    setGeometry(geom);
    // TODO ensure the native window if the application is already running
}

QUIKitWindow::~QUIKitWindow()
{
    delete mContext; mContext = 0;
    [mView release];
    [mWindow release];
}

void QUIKitWindow::setGeometry(const QRect &rect)
{
    if (mWindow) {
        mWindow.frame = CGRectMake(rect.x(), rect.y(), rect.width(), rect.height());
        mView.frame = CGRectMake(0, 0, rect.width(), rect.height());
        [mView deleteFramebuffer];
        [mWindow setNeedsDisplay];
    }
    QPlatformWindow::setGeometry(rect);
}

UIWindow *QUIKitWindow::ensureNativeWindow()
{
    if (!mWindow) {
        // window
        QRect geom = geometry();
        CGRect frame = CGRectMake(geom.x(), geom.y(), geom.width(), geom.height());
        mWindow = [[UIWindow alloc] initWithFrame:frame];
        mWindow.screen = mScreen->uiScreen();
        mWindow.frame = frame; // for some reason setting the screen resets frame.origin

        // view
        mView = [[EAGLView alloc] initWithFrame:CGRectMake(0, 0, geom.width(), geom.height())];
        [mView setMultipleTouchEnabled:YES];
        [mView setWindow:this];
        [mWindow addSubview:mView];
        [mWindow setNeedsDisplay];
        [mWindow makeKeyAndVisible];
    }
    return mWindow;
}

QPlatformGLContext *QUIKitWindow::glContext() const
{
    if (!mContext) {
        mContext = new EAGLPlatformContext(mView);
    }
    return mContext;
}

QT_END_NAMESPACE
