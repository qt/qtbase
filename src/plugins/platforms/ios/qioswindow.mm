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
        : m_view(view)
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
        [m_view setContext:aContext];
    }

    ~EAGLPlatformContext() { }

    bool makeCurrent(QPlatformSurface *surface)
    {
        Q_UNUSED(surface);
        qDebug() << __FUNCTION__ << "not implemented";
        //QPlatformOpenGLContext::makeCurrent();
        //[m_view makeCurrent];
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
        //[m_view presentFramebuffer];
    }

    QFunctionPointer getProcAddress(const QByteArray& ) { return 0; }

    QSurfaceFormat format() const
    {
        return mFormat;
    }

private:
    EAGLView *m_view;

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
    if (m_context != newContext)
    {
        [self deleteFramebuffer];
        [m_context release];
        m_context = [newContext retain];
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)presentFramebuffer
{
    if (m_context) {
        [EAGLContext setCurrentContext:m_context];
        glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
        [m_context presentRenderbuffer:GL_RENDERBUFFER];
    }
}

- (void)deleteFramebuffer
{
    if (m_context)
    {
        [EAGLContext setCurrentContext:m_context];
        if (m_framebuffer) {
            glDeleteFramebuffers(1, &m_framebuffer);
            m_framebuffer = 0;
        }
        if (m_colorRenderbuffer) {
            glDeleteRenderbuffers(1, &m_colorRenderbuffer);
            m_colorRenderbuffer = 0;
        }
        if (m_depthRenderbuffer) {
            glDeleteRenderbuffers(1, &m_depthRenderbuffer);
            m_depthRenderbuffer = 0;
        }
    }
}

- (void)createFramebuffer
{
    if (m_context && !m_framebuffer)
    {
        [EAGLContext setCurrentContext:m_context];
        glGenFramebuffers(1, &m_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

        glGenRenderbuffers(1, &m_colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
        [m_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &m_framebufferWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &m_framebufferHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_colorRenderbuffer);

        glGenRenderbuffers(1, &m_depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
        if (stencilBits() > 0) {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, m_framebufferWidth, m_framebufferHeight);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_framebufferWidth, m_framebufferHeight);
        }
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        if (delegate && [delegate respondsToSelector:@selector(eaglView:usesFramebuffer:)]) {
            [delegate eaglView:self usesFramebuffer:m_framebuffer];
        }
    }
}

- (void)makeCurrent
{
    if (m_context)
    {
        [EAGLContext setCurrentContext:m_context];
        if (!m_framebuffer)
            [self createFramebuffer];
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glViewport(0, 0, m_framebufferWidth, m_framebufferHeight);
    }
}

- (GLint)fbo
{
    return m_framebuffer;
}

- (void)setWindow:(QPlatformWindow *)window
{
    m_window = window;
}

- (void)sendMouseEventForTouches:(NSSet *)touches withEvent:(UIEvent *)event fakeButtons:(Qt::MouseButtons)buttons
{
    UITouch *touch = [touches anyObject];
    CGPoint locationInView = [touch locationInView:self];
    CGFloat scaleFactor = [self contentScaleFactor];
    QPoint p(locationInView.x * scaleFactor, locationInView.y * scaleFactor);
    // TODO handle global touch point? for status bar?
    QWindowSystemInterface::handleMouseEvent(m_window->window(), (ulong)(event.timestamp*1000),
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
    m_window(nil),
    m_context(0)
{
    m_screen = static_cast<QIOSScreen *>(QPlatformScreen::platformScreenForWindow(window));
    m_view = [[EAGLView alloc] init];
}

QIOSWindow::~QIOSWindow()
{
    delete m_context; m_context = 0;
    [m_view release];
    [m_window release];
}

void QIOSWindow::setGeometry(const QRect &rect)
{
    // Not supported. Only a single "full screen" window is supported
    QPlatformWindow::setGeometry(rect);
}

UIWindow *QIOSWindow::ensureNativeWindow()
{
    if (!m_window) {
        m_window = [[UIWindow alloc] init];
        updateGeometryAndOrientation();
        // window
        m_window.screen = m_screen->uiScreen();
        // for some reason setting the screen resets frame.origin, so we need to set the frame afterwards
        m_window.frame = m_frame;

        // view
        [m_view deleteFramebuffer];
        m_view.frame = CGRectMake(0, 0, m_window.bounds.size.width, m_window.bounds.size.height); // fill
        [m_view setContentScaleFactor:[m_window.screen scale]];
        [m_view setMultipleTouchEnabled:YES];
        [m_view setWindow:this];
        [m_window addSubview:m_view];
        [m_window setNeedsDisplay];
        [m_window makeKeyAndVisible];
    }
    return m_window;
}

void QIOSWindow::updateGeometryAndOrientation()
{
    if (!m_window)
        return;
    m_frame = [m_screen->uiScreen() applicationFrame];
    CGRect screen = [m_screen->uiScreen() bounds];
    QRect geom;
    CGFloat angle = 0;
    switch ([[UIApplication sharedApplication] statusBarOrientation]) {
    case UIInterfaceOrientationPortrait:
        geom = QRect(m_frame.origin.x, m_frame.origin.y, m_frame.size.width, m_frame.size.height);
        break;
    case UIInterfaceOrientationPortraitUpsideDown:
        geom = QRect(screen.size.width - m_frame.origin.x - m_frame.size.width,
                     screen.size.height - m_frame.origin.y - m_frame.size.height,
                     m_frame.size.width,
                     m_frame.size.height);
        angle = M_PI;
        break;
    case UIInterfaceOrientationLandscapeLeft:
        geom = QRect(screen.size.height - m_frame.origin.y - m_frame.size.height,
                     m_frame.origin.x,
                     m_frame.size.height,
                     m_frame.size.width);
        angle = -M_PI/2.;
        break;
    case UIInterfaceOrientationLandscapeRight:
        geom = QRect(m_frame.origin.y,
                     screen.size.width - m_frame.origin.x - m_frame.size.width,
                     m_frame.size.height,
                     m_frame.size.width);
        angle = +M_PI/2.;
        break;
    }

    CGFloat scale = [m_screen->uiScreen() scale];
    geom = QRect(geom.x() * scale, geom.y() * scale,
                 geom.width() * scale, geom.height() * scale);

    if (angle != 0) {
        [m_view layer].transform = CATransform3DMakeRotation(angle, 0, 0, 1.);
    } else {
        [m_view layer].transform = CATransform3DIdentity;
    }
    [m_view setNeedsDisplay];
    window()->setGeometry(geom);
}

QPlatformOpenGLContext *QIOSWindow::glContext() const
{
    if (!m_context) {
        m_context = new EAGLPlatformContext(m_view);
    }
    return m_context;
}

QT_END_NAMESPACE
