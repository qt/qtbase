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

#include <QtGui/QOpenGlContext>

#import <OpenGLES/EAGL.h>
#import <QuartzCore/CAEAGLLayer.h>

QIOSContext::QIOSContext(QOpenGLContext *context)
    : QPlatformOpenGLContext()
    , m_eaglContext([[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2])
{
    // Start out with the requested format
    QSurfaceFormat format = context->format();

    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setMajorVersion(2);
    format.setMinorVersion(0);

    // Even though iOS internally double-buffers its rendering, we
    // report single-buffered here since the buffer remains unchanged
    // when swapping unlesss you manually clear it yourself.
    format.setSwapBehavior(QSurfaceFormat::SingleBuffer);

    m_format = format;
}

QIOSContext::~QIOSContext()
{
    if ([EAGLContext currentContext] == m_eaglContext)
        doneCurrent();

    [m_eaglContext release];
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

    [EAGLContext setCurrentContext:m_eaglContext];

    GLint renderbuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject(surface));
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);

    [m_eaglContext presentRenderbuffer:GL_RENDERBUFFER];
}

GLuint QIOSContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    return static_cast<QIOSWindow *>(surface)->framebufferObject(*const_cast<const QIOSContext*>(this));
}

QFunctionPointer QIOSContext::getProcAddress(const QByteArray& functionName)
{
    return reinterpret_cast<QFunctionPointer>(dlsym(RTLD_NEXT, functionName.constData()));
}

EAGLContext *QIOSContext::nativeContext() const
{
    return m_eaglContext;
}
