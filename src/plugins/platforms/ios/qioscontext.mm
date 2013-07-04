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

#include "qioscontext.h"
#include "qioswindow.h"

#include <dlfcn.h>

#include <QtGui/QOpenGLContext>

#import <OpenGLES/EAGL.h>
#import <QuartzCore/CAEAGLLayer.h>

QIOSContext::QIOSContext(QOpenGLContext *context)
    : QPlatformOpenGLContext()
    , m_eaglContext([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2])
    , m_format(context->format())
{
    m_format.setRenderableType(QSurfaceFormat::OpenGLES);
    m_format.setMajorVersion(2);
    m_format.setMinorVersion(0);

    // iOS internally double-buffers its rendering using copy instead of flipping,
    // so technically we could report that we are single-buffered so that clients
    // could take advantage of the unchanged buffer, but this means clients (and Qt)
    // will also assume that swapBufferes() is not needed, which is _not_ the case.
    m_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
}

QIOSContext::~QIOSContext()
{
    [EAGLContext setCurrentContext:m_eaglContext];

    foreach (const FramebufferObject &framebufferObject, m_framebufferObjects)
        deleteBuffers(framebufferObject);

    [EAGLContext setCurrentContext:nil];
    [m_eaglContext release];
}

void QIOSContext::deleteBuffers(const FramebufferObject &framebufferObject)
{
    if (framebufferObject.handle)
        glDeleteFramebuffers(1, &framebufferObject.handle);
    if (framebufferObject.colorRenderbuffer)
        glDeleteRenderbuffers(1, &framebufferObject.colorRenderbuffer);
    if (framebufferObject.depthRenderbuffer)
        glDeleteRenderbuffers(1, &framebufferObject.depthRenderbuffer);
}

QSurfaceFormat QIOSContext::format() const
{
    return m_format;
}

bool QIOSContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface && surface->surface()->surfaceType() == QSurface::OpenGLSurface);

    [EAGLContext setCurrentContext:m_eaglContext];
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject(surface));

    return true;
}

void QIOSContext::doneCurrent()
{
    [EAGLContext setCurrentContext:nil];
}

void QIOSContext::swapBuffers(QPlatformSurface *surface)
{
    Q_ASSERT(surface && surface->surface()->surfaceType() == QSurface::OpenGLSurface);
    Q_ASSERT(surface->surface()->surfaceClass() == QSurface::Window);
    QWindow *window = static_cast<QWindow *>(surface->surface());
    Q_ASSERT(m_framebufferObjects.contains(window));

    [EAGLContext setCurrentContext:m_eaglContext];
    glBindRenderbuffer(GL_RENDERBUFFER, m_framebufferObjects[window].colorRenderbuffer);
    [m_eaglContext presentRenderbuffer:GL_RENDERBUFFER];
}

GLuint QIOSContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    Q_ASSERT(surface && surface->surface()->surfaceClass() == QSurface::Window);
    QWindow *window = static_cast<QWindow *>(surface->surface());

    FramebufferObject &framebufferObject = m_framebufferObjects[window];

    // Set up an FBO for the window if it hasn't been created yet
    if (!framebufferObject.handle) {
        [EAGLContext setCurrentContext:m_eaglContext];

        glGenFramebuffers(1, &framebufferObject.handle);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject.handle);

        glGenRenderbuffers(1, &framebufferObject.colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.colorRenderbuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
            framebufferObject.colorRenderbuffer);

        if (m_format.depthBufferSize() > 0 || m_format.stencilBufferSize() > 0) {
            glGenRenderbuffers(1, &framebufferObject.depthRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.depthRenderbuffer);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                framebufferObject.depthRenderbuffer);

            if (m_format.stencilBufferSize() > 0)
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                    framebufferObject.depthRenderbuffer);
        }

        connect(window, SIGNAL(destroyed(QObject*)), this, SLOT(windowDestroyed(QObject*)));
    }

    // Ensure that the FBO's buffers match the size of the window
    QIOSWindow *platformWindow = static_cast<QIOSWindow *>(surface);
    if (framebufferObject.renderbufferWidth != platformWindow->effectiveWidth() ||
        framebufferObject.renderbufferHeight != platformWindow->effectiveHeight()) {

        [EAGLContext setCurrentContext:m_eaglContext];
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject.handle);

        glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.colorRenderbuffer);
        UIView *view = reinterpret_cast<UIView *>(platformWindow->winId());
        CAEAGLLayer *layer = static_cast<CAEAGLLayer *>(view.layer);
        [m_eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];

        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &framebufferObject.renderbufferWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &framebufferObject.renderbufferHeight);

        if (framebufferObject.depthRenderbuffer) {
            glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.depthRenderbuffer);

            // FIXME: Support more fine grained control over depth/stencil buffer sizes
            if (m_format.stencilBufferSize() > 0)
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES,
                    framebufferObject.renderbufferWidth, framebufferObject.renderbufferHeight);
            else
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                    framebufferObject.renderbufferWidth, framebufferObject.renderbufferHeight);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }

    return framebufferObject.handle;
}

void QIOSContext::windowDestroyed(QObject *object)
{
    QWindow *window = static_cast<QWindow *>(object);
    if (m_framebufferObjects.contains(window)) {
        EAGLContext *originalContext = [EAGLContext currentContext];
        [EAGLContext setCurrentContext:m_eaglContext];
        deleteBuffers(m_framebufferObjects[window]);
        m_framebufferObjects.remove(window);
        [EAGLContext setCurrentContext:originalContext];
    }
}

QFunctionPointer QIOSContext::getProcAddress(const QByteArray& functionName)
{
    return reinterpret_cast<QFunctionPointer>(dlsym(RTLD_NEXT, functionName.constData()));
}

#include "moc_qioscontext.cpp"

