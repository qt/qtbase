/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
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

#include "qioswindow.h"

#include "qiosscreen.h"

#include <QtDebug>
#include <QtGui/QKeyEvent>
#include <qpa/qplatformopenglcontext.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtDebug>

static GLint stencilBits()
{
    static GLint bits;
    static bool initialized = false;
    if (!initialized) {
        glGetIntegerv(GL_STENCIL_BITS, &bits);
        initialized = true;
    }
    return bits;
}

/*
static GLint depthBits()
{
    // we can choose between GL_DEPTH24_STENCIL8_OES and GL_DEPTH_COMPONENT16
    return stencilBits() > 0 ? 24 : 16;
}
*/

class EAGLPlatformContext : public QPlatformOpenGLContext
{
public:
    EAGLPlatformContext(EAGLView *view)
        : mView(view)
    {
        /*
        mFormat.setWindowApi(QPlatformWindowFormat::OpenGL);
        mFormat.setDepthBufferSize(depthBits());
        mFormat.setAccumBufferSize(0);
        mFormat.setRedBufferSize(8);
        mFormat.setGreenBufferSize(8);
        mFormat.setBlueBufferSize(8);
        mFormat.setAlphaBufferSize(8);
        mFormat.setStencilBufferSize(stencilBits());
        mFormat.setSamples(0);
        mFormat.setSampleBuffers(false);
        mFormat.setDoubleBuffer(true);
        mFormat.setDepth(true);
        mFormat.setRgba(true);
        mFormat.setAlpha(true);
        mFormat.setAccum(false);
        mFormat.setStencil(stencilBits() > 0);
        mFormat.setStereo(false);
        mFormat.setDirectRendering(false);
        */

#if defined(QT_OPENGL_ES_2)
        EAGLContext *aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
#else
        EAGLContext *aContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
#endif
        [mView setContext:aContext];
    }

    ~EAGLPlatformContext() { }

    bool makeCurrent(QPlatformSurface *surface)
    {
        Q_UNUSED(surface);
        qDebug() << __FUNCTION__ << "not implemented";
        //QPlatformOpenGLContext::makeCurrent();
        //[mView makeCurrent];
        return false;
    }

    void doneCurrent()
    {
        qDebug() << __FUNCTION__ << "not implemented";
        //QPlatformOpenGLContext::doneCurrent();
    }

    void swapBuffers(QPlatformSurface *surface)
    {
        Q_UNUSED(surface);
        qDebug() << __FUNCTION__ << "not implemented";
        //[mView presentFramebuffer];
    }

    QFunctionPointer getProcAddress(const QByteArray& ) { return 0; }

    QSurfaceFormat format() const
    {
        return mFormat;
    }

private:
    EAGLView *mView;

    QSurfaceFormat mFormat;
};

@implementation EAGLView

@synthesize delegate;

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
        glBindRenderbuffer(GL_RENDERBUFFER, mColorRenderbuffer);
        [mContext presentRenderbuffer:GL_RENDERBUFFER];
    }
}

- (void)deleteFramebuffer
{
    if (mContext)
    {
        [EAGLContext setCurrentContext:mContext];
        if (mFramebuffer) {
            glDeleteFramebuffers(1, &mFramebuffer);
            mFramebuffer = 0;
        }
        if (mColorRenderbuffer) {
            glDeleteRenderbuffers(1, &mColorRenderbuffer);
            mColorRenderbuffer = 0;
        }
        if (mDepthRenderbuffer) {
            glDeleteRenderbuffers(1, &mDepthRenderbuffer);
            mDepthRenderbuffer = 0;
        }
    }
}

- (void)createFramebuffer
{
    if (mContext && !mFramebuffer)
    {
        [EAGLContext setCurrentContext:mContext];
        glGenFramebuffers(1, &mFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);

        glGenRenderbuffers(1, &mColorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, mColorRenderbuffer);
        [mContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &mFramebufferWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &mFramebufferHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mColorRenderbuffer);

        glGenRenderbuffers(1, &mDepthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, mDepthRenderbuffer);
        if (stencilBits() > 0) {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, mFramebufferWidth, mFramebufferHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbuffer);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, mFramebufferWidth, mFramebufferHeight);
        }
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthRenderbuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        if (delegate && [delegate respondsToSelector:@selector(eaglView:usesFramebuffer:)]) {
            [delegate eaglView:self usesFramebuffer:mFramebuffer];
        }
    }
}

- (void)makeCurrent
{
    if (mContext)
    {
        [EAGLContext setCurrentContext:mContext];
        if (!mFramebuffer)
            [self createFramebuffer];
        glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
        glViewport(0, 0, mFramebufferWidth, mFramebufferHeight);
    }
}

- (GLint)fbo
{
    return mFramebuffer;
}

- (void)setWindow:(QPlatformWindow *)window
{
    mWindow = window;
}

- (void)sendMouseEventForTouches:(NSSet *)touches withEvent:(UIEvent *)event fakeButtons:(Qt::MouseButtons)buttons
{
    UITouch *touch = [touches anyObject];
    CGPoint locationInView = [touch locationInView:self];
    CGFloat scaleFactor = [self contentScaleFactor];
    QPoint p(locationInView.x * scaleFactor, locationInView.y * scaleFactor);
    // TODO handle global touch point? for status bar?
    QWindowSystemInterface::handleMouseEvent(mWindow->window(), (ulong)(event.timestamp*1000),
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

QIOSWindow::QIOSWindow(QWindow *window) :
    QPlatformWindow(window),
    mWindow(nil),
    mContext(0)
{
    mScreen = static_cast<QIOSScreen *>(QPlatformScreen::platformScreenForWindow(window));
    mView = [[EAGLView alloc] init];
}

QIOSWindow::~QIOSWindow()
{
    delete mContext; mContext = 0;
    [mView release];
    [mWindow release];
}

void QIOSWindow::setGeometry(const QRect &rect)
{
    // Not supported. Only a single "full screen" window is supported
    QPlatformWindow::setGeometry(rect);
}

UIWindow *QIOSWindow::ensureNativeWindow()
{
    if (!mWindow) {
        mWindow = [[UIWindow alloc] init];
        updateGeometryAndOrientation();
        // window
        mWindow.screen = mScreen->uiScreen();
        // for some reason setting the screen resets frame.origin, so we need to set the frame afterwards
        mWindow.frame = mFrame;

        // view
        [mView deleteFramebuffer];
        mView.frame = CGRectMake(0, 0, mWindow.bounds.size.width, mWindow.bounds.size.height); // fill
        [mView setContentScaleFactor:[mWindow.screen scale]];
        [mView setMultipleTouchEnabled:YES];
        [mView setWindow:this];
        [mWindow addSubview:mView];
        [mWindow setNeedsDisplay];
        [mWindow makeKeyAndVisible];
    }
    return mWindow;
}

void QIOSWindow::updateGeometryAndOrientation()
{
    if (!mWindow)
        return;
    mFrame = [mScreen->uiScreen() applicationFrame];
    CGRect screen = [mScreen->uiScreen() bounds];
    QRect geom;
    CGFloat angle = 0;
    switch ([[UIApplication sharedApplication] statusBarOrientation]) {
    case UIInterfaceOrientationPortrait:
        geom = QRect(mFrame.origin.x, mFrame.origin.y, mFrame.size.width, mFrame.size.height);
        break;
    case UIInterfaceOrientationPortraitUpsideDown:
        geom = QRect(screen.size.width - mFrame.origin.x - mFrame.size.width,
                     screen.size.height - mFrame.origin.y - mFrame.size.height,
                     mFrame.size.width,
                     mFrame.size.height);
        angle = M_PI;
        break;
    case UIInterfaceOrientationLandscapeLeft:
        geom = QRect(screen.size.height - mFrame.origin.y - mFrame.size.height,
                     mFrame.origin.x,
                     mFrame.size.height,
                     mFrame.size.width);
        angle = -M_PI/2.;
        break;
    case UIInterfaceOrientationLandscapeRight:
        geom = QRect(mFrame.origin.y,
                     screen.size.width - mFrame.origin.x - mFrame.size.width,
                     mFrame.size.height,
                     mFrame.size.width);
        angle = +M_PI/2.;
        break;
    }

    CGFloat scale = [mScreen->uiScreen() scale];
    geom = QRect(geom.x() * scale, geom.y() * scale,
                 geom.width() * scale, geom.height() * scale);

    if (angle != 0) {
        [mView layer].transform = CATransform3DMakeRotation(angle, 0, 0, 1.);
    } else {
        [mView layer].transform = CATransform3DIdentity;
    }
    [mView setNeedsDisplay];
    window()->setGeometry(geom);
}

QPlatformOpenGLContext *QIOSWindow::glContext() const
{
    if (!mContext) {
        mContext = new EAGLPlatformContext(mView);
    }
    return mContext;
}

QT_END_NAMESPACE
