/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#include "qglpixelbuffer.h"
#include "qglpixelbuffer_p.h"

#ifndef QT_MAC_USE_COCOA
#include <AGL/agl.h>
#endif

#include <qimage.h>
#include <private/qgl_p.h>
#include <qdebug.h>

QT_BEGIN_NAMESPACE

#ifndef GL_TEXTURE_RECTANGLE_EXT
#define GL_TEXTURE_RECTANGLE_EXT 0x84F5
#endif

static int nearest_gl_texture_size(int v)
{
    int n = 0, last = 0;
    for (int s = 0; s < 32; ++s) {
        if (((v>>s) & 1) == 1) {
            ++n;
            last = s;
        }
    }
    if (n > 1)
        return 1 << (last+1);
    return 1 << last;
}

bool QGLPixelBufferPrivate::init(const QSize &size, const QGLFormat &f, QGLWidget *shareWidget)
{
#ifdef QT_MAC_USE_COCOA
    Q_Q(QGLPixelBuffer);
    // create a dummy context
    QGLContext context(f, q);
    context.create(shareWidget ? shareWidget->context() : 0);

    if (context.isSharing())
        share_ctx = shareWidget->context()->d_func()->cx;

    // steal the NSOpenGLContext and update the format
    ctx = context.d_func()->cx;
    context.d_func()->cx = 0;
    // d->cx will be set to ctx later in
    // QGLPixelBufferPrivate::common_init, so we need to retain it:
    [static_cast<NSOpenGLContext *>(ctx) retain];

    format = context.format();

    GLenum target = GL_TEXTURE_2D;

    if ((QGLExtensions::glExtensions() & QGLExtensions::TextureRectangle)
        && (size.width() != nearest_gl_texture_size(size.width())
            || size.height() != nearest_gl_texture_size(size.height())))
    {
        target = GL_TEXTURE_RECTANGLE_EXT;
    }

    pbuf = [[NSOpenGLPixelBuffer alloc] initWithTextureTarget:target
                                        textureInternalFormat:GL_RGBA
                                        textureMaxMipMapLevel:0
                                                  pixelsWide:size.width()
                                                  pixelsHigh:size.height()];
    if (!pbuf) {
        qWarning("QGLPixelBuffer: Cannot create a pbuffer");
        return false;
    }

    [static_cast<NSOpenGLContext *>(ctx) setPixelBuffer:static_cast<NSOpenGLPixelBuffer *>(pbuf)
                                            cubeMapFace:0
                                            mipMapLevel:0
                                   currentVirtualScreen:0];
    return true;
#else
    GLint attribs[40], i=0;
    attribs[i++] = AGL_RGBA;
    attribs[i++] = AGL_BUFFER_SIZE;
    attribs[i++] = 32;
    attribs[i++] = AGL_LEVEL;
    attribs[i++] = f.plane();
    if (f.redBufferSize() != -1) {
        attribs[i++] = AGL_RED_SIZE;
        attribs[i++] = f.redBufferSize();
    }
    if (f.greenBufferSize() != -1) {
        attribs[i++] = AGL_GREEN_SIZE;
        attribs[i++] = f.greenBufferSize();
    }
    if (f.blueBufferSize() != -1) {
        attribs[i++] = AGL_BLUE_SIZE;
        attribs[i++] = f.blueBufferSize();
    }
    if (f.stereo())
        attribs[i++] = AGL_STEREO;
    if (f.alpha()) {
        attribs[i++] = AGL_ALPHA_SIZE;
        attribs[i++] = f.alphaBufferSize() == -1 ? 8 : f.alphaBufferSize();
    }
    if (f.stencil()) {
        attribs[i++] = AGL_STENCIL_SIZE;
        attribs[i++] = f.stencilBufferSize() == -1 ? 8 : f.stencilBufferSize();
    }
    if (f.depth()) {
        attribs[i++] = AGL_DEPTH_SIZE;
        attribs[i++] = f.depthBufferSize() == -1 ? 32 : f.depthBufferSize();
    }
    if (f.accum()) {
        attribs[i++] = AGL_ACCUM_RED_SIZE;
        attribs[i++] = f.accumBufferSize() == -1 ? 16 : f.accumBufferSize();
        attribs[i++] = AGL_ACCUM_BLUE_SIZE;
        attribs[i++] = f.accumBufferSize() == -1 ? 16 : f.accumBufferSize();
        attribs[i++] = AGL_ACCUM_GREEN_SIZE;
        attribs[i++] = f.accumBufferSize() == -1 ? 16 : f.accumBufferSize();
        attribs[i++] = AGL_ACCUM_ALPHA_SIZE;
        attribs[i++] = f.accumBufferSize() == -1 ? 16 : f.accumBufferSize();
    }

    if (f.sampleBuffers()) {
        attribs[i++] = AGL_SAMPLE_BUFFERS_ARB;
        attribs[i++] = 1;
        attribs[i++] = AGL_SAMPLES_ARB;
        attribs[i++] = f.samples() == -1 ? 4 : f.samples();
    }
    attribs[i] = AGL_NONE;

    AGLPixelFormat format = aglChoosePixelFormat(0, 0, attribs);
    if (!format) {
	qWarning("QGLPixelBuffer: Unable to find a pixel format (AGL error %d).",
		 (int) aglGetError());
        return false;
    }

    GLint res;
    aglDescribePixelFormat(format, AGL_LEVEL, &res);
    this->format.setPlane(res);
    aglDescribePixelFormat(format, AGL_DOUBLEBUFFER, &res);
    this->format.setDoubleBuffer(res);
    aglDescribePixelFormat(format, AGL_DEPTH_SIZE, &res);
    this->format.setDepth(res);
    if (this->format.depth())
	this->format.setDepthBufferSize(res);
    aglDescribePixelFormat(format, AGL_RGBA, &res);
    this->format.setRgba(res);
    aglDescribePixelFormat(format, AGL_RED_SIZE, &res);
    this->format.setRedBufferSize(res);
    aglDescribePixelFormat(format, AGL_GREEN_SIZE, &res);
    this->format.setGreenBufferSize(res);
    aglDescribePixelFormat(format, AGL_BLUE_SIZE, &res);
    this->format.setBlueBufferSize(res);
    aglDescribePixelFormat(format, AGL_ALPHA_SIZE, &res);
    this->format.setAlpha(res);
    if (this->format.alpha())
	this->format.setAlphaBufferSize(res);
    aglDescribePixelFormat(format, AGL_ACCUM_RED_SIZE, &res);
    this->format.setAccum(res);
    if (this->format.accum())
	this->format.setAccumBufferSize(res);
    aglDescribePixelFormat(format, AGL_STENCIL_SIZE, &res);
    this->format.setStencil(res);
    if (this->format.stencil())
	this->format.setStencilBufferSize(res);
    aglDescribePixelFormat(format, AGL_STEREO, &res);
    this->format.setStereo(res);
    aglDescribePixelFormat(format, AGL_SAMPLE_BUFFERS_ARB, &res);
    this->format.setSampleBuffers(res);
    if (this->format.sampleBuffers()) {
        aglDescribePixelFormat(format, AGL_SAMPLES_ARB, &res);
        this->format.setSamples(res);
    }

    AGLContext share = 0;
    if (shareWidget)
	share = share_ctx = static_cast<AGLContext>(shareWidget->d_func()->glcx->d_func()->cx);
    ctx = aglCreateContext(format, share);
    if (!ctx) {
	qWarning("QGLPixelBuffer: Unable to create a context (AGL error %d).",
		 (int) aglGetError());
	return false;
    }

    GLenum target = GL_TEXTURE_2D;

    if ((QGLExtensions::glExtensions() & QGLExtensions::TextureRectangle)
        && (size.width() != nearest_gl_texture_size(size.width())
            || size.height() != nearest_gl_texture_size(size.height())))
    {
        target = GL_TEXTURE_RECTANGLE_EXT;
    }

    if (!aglCreatePBuffer(size.width(), size.height(), target, GL_RGBA, 0, &pbuf)) {
	qWarning("QGLPixelBuffer: Unable to create a pbuffer (AGL error %d).",
		 (int) aglGetError());
	return false;
    }

    if (!aglSetPBuffer(ctx, pbuf, 0, 0, 0)) {
	qWarning("QGLPixelBuffer: Unable to set pbuffer (AGL error %d).",
		 (int) aglGetError());
	return false;
    }

    aglDestroyPixelFormat(format);
    return true;

#endif
}

bool QGLPixelBufferPrivate::cleanup()
{
#ifdef QT_MAC_USE_COCOA
    [static_cast<NSOpenGLPixelBuffer *>(pbuf) release];
    pbuf = 0;
    [static_cast<NSOpenGLContext *>(ctx) release];
    ctx = 0;
#else
    aglDestroyPBuffer(pbuf);
#endif
    return true;
}

bool QGLPixelBuffer::bindToDynamicTexture(GLuint texture_id)
{
    Q_D(QGLPixelBuffer);
    if (d->invalid || !d->share_ctx)
        return false;

#ifdef QT_MAC_USE_COCOA
    NSOpenGLContext *oldContext = [NSOpenGLContext currentContext];
    if (d->share_ctx != oldContext)
        [static_cast<NSOpenGLContext *>(d->share_ctx) makeCurrentContext];
    glBindTexture(GL_TEXTURE_2D, texture_id);
    [static_cast<NSOpenGLContext *>(d->share_ctx)
            setTextureImageToPixelBuffer:static_cast<NSOpenGLPixelBuffer *>(d->pbuf)
            colorBuffer:GL_FRONT];
    if (oldContext && oldContext != d->share_ctx)
        [oldContext makeCurrentContext];
    return true;
#else
    aglSetCurrentContext(d->share_ctx);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    aglTexImagePBuffer(d->share_ctx, d->pbuf, GL_FRONT);
    return true;
#endif
}

#ifdef Q_MAC_COMPAT_GL_FUNCTIONS
bool QGLPixelBuffer::bindToDynamicTexture(QMacCompatGLuint texture_id)
{
    return bindToDynamicTexture(GLuint(texture_id));
}
#endif

void QGLPixelBuffer::releaseFromDynamicTexture()
{
}

GLuint QGLPixelBuffer::generateDynamicTexture() const
{
#ifdef QT_MAC_USE_COCOA
    Q_D(const QGLPixelBuffer);
    NSOpenGLContext *oldContext = [NSOpenGLContext currentContext];
    if (d->share_ctx != oldContext)
        [static_cast<NSOpenGLContext *>(d->share_ctx) makeCurrentContext];
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    if (oldContext && oldContext != d->share_ctx)
        [oldContext makeCurrentContext];
    return texture;
#else
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return texture;
#endif
}

bool QGLPixelBuffer::hasOpenGLPbuffers()
{
    return true;
}

QT_END_NAMESPACE
