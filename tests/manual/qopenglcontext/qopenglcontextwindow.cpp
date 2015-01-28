/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qopenglcontextwindow.h"
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QGuiApplication>
#include <QtGui/QMatrix4x4>
#include <qpa/qplatformnativeinterface.h>

#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformHeaders/QEGLNativeContext>

QOpenGLContextWindow::QOpenGLContextWindow()
    : m_blitter(0)
{
    setSurfaceType(OpenGLSurface);

    m_context = new QOpenGLContext(this);
    m_context->setFormat(requestedFormat());
    m_context->create();

    m_image = QImage(QStringLiteral("qticon64.png")).convertToFormat(QImage::Format_RGBA8888);
    Q_ASSERT(!m_image.isNull());

    create(); // to make sure format() returns something real
    createForeignContext();
}

QOpenGLContextWindow::~QOpenGLContextWindow()
{
    if (m_blitter) {
        m_blitter->destroy(); // the dtor does not call this for some reason
        delete m_blitter;
    }
}

void QOpenGLContextWindow::render()
{
    if (!m_context->makeCurrent(this))
        qFatal("makeCurrent() failed");

    QOpenGLFunctions *f = m_context->functions();
    f->glViewport(0, 0, dWidth(), dHeight());
    f->glClearColor(0, 0, 0, 1);
    f->glClear(GL_COLOR_BUFFER_BIT);

    if (!m_blitter) {
        m_blitter = new QOpenGLTextureBlitter;
        m_blitter->create();
    }

    // Draw the image. If nothing gets shown, then something went wrong with the context
    // adoption or sharing was not successfully enabled.
    m_blitter->bind();
    QRectF r(0, 0, dWidth(), dHeight());
    QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(QRectF(100, 100, 100, 100), r.toRect());
    m_blitter->blit(m_textureId, target, QOpenGLTextureBlitter::OriginTopLeft);
    m_blitter->release();

    m_context->swapBuffers(this);
}

void QOpenGLContextWindow::exposeEvent(QExposeEvent *)
{
    if (isExposed())
        render();
}

void QOpenGLContextWindow::createForeignContext()
{
    // Here a context will be created manually. This context will share with m_context's
    // underlying native context.  This way the texture, that belongs to the context
    // created here, will be accessible from m_context too.

    EGLContext shareCtx = m_context->nativeHandle().value<QEGLNativeContext>().context();
    Q_ASSERT(shareCtx != EGL_NO_CONTEXT);

    EGLDisplay dpy = (EGLDisplay) qGuiApp->platformNativeInterface()->nativeResourceForWindow(
        QByteArrayLiteral("egldisplay"), this);
    Q_ASSERT(dpy != EGL_NO_DISPLAY);

    QSurfaceFormat fmt = format();
    EGLConfig config = q_configFromGLFormat(dpy, fmt);

    QVector<EGLint> contextAttrs;
    contextAttrs.append(EGL_CONTEXT_CLIENT_VERSION);
    contextAttrs.append(fmt.majorVersion());
    contextAttrs.append(EGL_NONE);
    switch (fmt.renderableType()) {
#ifdef EGL_VERSION_1_4
    case QSurfaceFormat::OpenGL:
        eglBindAPI(EGL_OPENGL_API);
        break;
#endif // EGL_VERSION_1_4
    default:
        eglBindAPI(EGL_OPENGL_ES_API);
        break;
    }

    EGLContext ctx = eglCreateContext(dpy, config, shareCtx, contextAttrs.constData());
    Q_ASSERT(ctx != EGL_NO_CONTEXT);

    // Wrap ctx into a QOpenGLContext.
    QOpenGLContext *ctxWrap = new QOpenGLContext;
    ctxWrap->setNativeHandle(QVariant::fromValue<QEGLNativeContext>(QEGLNativeContext(ctx, dpy)));
    ctxWrap->setShareContext(m_context); // only needed for correct bookkeeping
    if (!ctxWrap->create())
        qFatal("Failed to created wrapping context");
    Q_ASSERT(ctxWrap->nativeHandle().value<QEGLNativeContext>().context() == ctx);

    QOffscreenSurface surface;
    surface.setFormat(fmt);
    surface.create();

    if (!ctxWrap->makeCurrent(&surface))
        qFatal("Failed to make pbuffer surface current");

    // Create the texture.
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    GLuint textureId = 0;
    f->glGenTextures(1, &textureId);
    f->glBindTexture(GL_TEXTURE_2D, textureId);
    f->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_image.width(), m_image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                    m_image.constBits());
    Q_ASSERT(f->glGetError() == GL_NO_ERROR);

    ctxWrap->doneCurrent();
    delete ctxWrap; // ctx is not destroyed
    eglDestroyContext(dpy, ctx); // resources like the texture stay alive until any context on the share list is alive
    Q_ASSERT(eglGetError() == EGL_SUCCESS);

    m_textureId = textureId;
}
