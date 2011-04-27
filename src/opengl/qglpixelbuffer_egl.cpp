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

#include <qdebug.h>
#include "qglpixelbuffer.h"
#include "qglpixelbuffer_p.h"
#include "qgl_egl_p.h"

#include <qimage.h>
#include <private/qgl_p.h>

QT_BEGIN_NAMESPACE

#ifdef EGL_BIND_TO_TEXTURE_RGBA
#define QGL_RENDER_TEXTURE 1
#else
#define QGL_RENDER_TEXTURE 0
#endif

bool QGLPixelBufferPrivate::init(const QSize &size, const QGLFormat &f, QGLWidget *shareWidget)
{
    // Create the EGL context.
    ctx = new QEglContext();
    ctx->setApi(QEgl::OpenGL);

    // Find the shared context.
    QEglContext *shareContext = 0;
    if (shareWidget && shareWidget->d_func()->glcx)
        shareContext = shareWidget->d_func()->glcx->d_func()->eglContext;

    // Choose an appropriate configuration.  We use the best format
    // we can find, even if it is greater than the requested format.
    // We try for a pbuffer that is capable of texture rendering if possible.
    textureFormat = EGL_NONE;
    if (shareContext) {
        // Use the same configuration as the widget we are sharing with.
        ctx->setConfig(shareContext->config());
#if QGL_RENDER_TEXTURE
        if (ctx->configAttrib(EGL_BIND_TO_TEXTURE_RGBA) == EGL_TRUE)
            textureFormat = EGL_TEXTURE_RGBA;
        else if (ctx->configAttrib(EGL_BIND_TO_TEXTURE_RGB) == EGL_TRUE)
            textureFormat = EGL_TEXTURE_RGB;
#endif
    } else {
        QEglProperties configProps;
        qt_eglproperties_set_glformat(configProps, f);
        configProps.setDeviceType(QInternal::Pbuffer);
        configProps.setRenderableType(ctx->api());
        bool ok = false;
#if QGL_RENDER_TEXTURE
        textureFormat = EGL_TEXTURE_RGBA;
        configProps.setValue(EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE);
        ok = ctx->chooseConfig(configProps, QEgl::BestPixelFormat);
        if (!ok) {
            // Try again with RGB texture rendering.
            textureFormat = EGL_TEXTURE_RGB;
            configProps.removeValue(EGL_BIND_TO_TEXTURE_RGBA);
            configProps.setValue(EGL_BIND_TO_TEXTURE_RGB, EGL_TRUE);
            ok = ctx->chooseConfig(configProps, QEgl::BestPixelFormat);
            if (!ok) {
                // One last try for a pbuffer with no texture rendering.
                configProps.removeValue(EGL_BIND_TO_TEXTURE_RGB);
                textureFormat = EGL_NONE;
            }
        }
#endif
        if (!ok) {
            if (!ctx->chooseConfig(configProps, QEgl::BestPixelFormat)) {
                delete ctx;
                ctx = 0;
                return false;
            }
        }
    }

    // Retrieve the actual format properties.
    qt_glformat_from_eglconfig(format, ctx->config());

    // Create the attributes needed for the pbuffer.
    QEglProperties attribs;
    attribs.setValue(EGL_WIDTH, size.width());
    attribs.setValue(EGL_HEIGHT, size.height());
#if QGL_RENDER_TEXTURE
    if (textureFormat != EGL_NONE) {
        attribs.setValue(EGL_TEXTURE_FORMAT, textureFormat);
        attribs.setValue(EGL_TEXTURE_TARGET, EGL_TEXTURE_2D);
    }
#endif

    // Create the pbuffer surface.
    pbuf = eglCreatePbufferSurface(ctx->display(), ctx->config(), attribs.properties());
#if QGL_RENDER_TEXTURE
    if (pbuf == EGL_NO_SURFACE && textureFormat != EGL_NONE) {
        // Try again with texture rendering disabled.
        textureFormat = EGL_NONE;
        attribs.removeValue(EGL_TEXTURE_FORMAT);
        attribs.removeValue(EGL_TEXTURE_TARGET);
        pbuf = eglCreatePbufferSurface(ctx->display(), ctx->config(), attribs.properties());
    }
#endif
    if (pbuf == EGL_NO_SURFACE) {
        qWarning() << "QGLPixelBufferPrivate::init(): Unable to create EGL pbuffer surface:" << QEgl::errorString();
        return false;
    }

    // Create a new context for the configuration.
    if (!ctx->createContext(shareContext)) {
        delete ctx;
        ctx = 0;
        return false;
    }

    return true;
}

bool QGLPixelBufferPrivate::cleanup()
{
    // No need to destroy "pbuf" here - it is done in QGLContext::reset().
    return true;
}

bool QGLPixelBuffer::bindToDynamicTexture(GLuint texture_id)
{
#if QGL_RENDER_TEXTURE
    Q_D(QGLPixelBuffer);
    if (d->invalid || d->textureFormat == EGL_NONE || !d->ctx)
        return false;
    glBindTexture(GL_TEXTURE_2D, texture_id);
    return eglBindTexImage(d->ctx->display(), d->pbuf, EGL_BACK_BUFFER);
#else
    Q_UNUSED(texture_id);
    return false;
#endif
}

void QGLPixelBuffer::releaseFromDynamicTexture()
{
#if QGL_RENDER_TEXTURE
    Q_D(QGLPixelBuffer);
    if (d->invalid || d->textureFormat == EGL_NONE || !d->ctx)
        return;
    eglReleaseTexImage(d->ctx->display(), d->pbuf, EGL_BACK_BUFFER);
#endif
}


GLuint QGLPixelBuffer::generateDynamicTexture() const
{
#if QGL_RENDER_TEXTURE
    Q_D(const QGLPixelBuffer);
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    if (d->textureFormat == EGL_TEXTURE_RGB)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, d->req_size.width(), d->req_size.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    else
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, d->req_size.width(), d->req_size.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    return texture;
#else
    return 0;
#endif
}

bool QGLPixelBuffer::hasOpenGLPbuffers()
{
    // See if we have at least 1 configuration that matches the default format.
    EGLDisplay dpy = QEgl::display();
    if (dpy == EGL_NO_DISPLAY)
        return false;
    QEglProperties configProps;
    qt_eglproperties_set_glformat(configProps, QGLFormat::defaultFormat());
    configProps.setDeviceType(QInternal::Pbuffer);
    configProps.setRenderableType(QEgl::OpenGL);
    do {
        EGLConfig cfg = 0;
        EGLint matching = 0;
        if (eglChooseConfig(dpy, configProps.properties(),
                            &cfg, 1, &matching) && matching > 0)
            return true;
    } while (configProps.reduceConfiguration());
    return false;
}

QT_END_NAMESPACE
