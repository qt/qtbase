// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qopenglcontextwindow.h"
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QGuiApplication>
#include <QtGui/QMatrix4x4>
#include <qpa/qplatformnativeinterface.h>

#include <QtEglSupport/private/qeglconvenience_p.h>

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

    using namespace QNativeInterface;
    auto *eglContext = m_context->nativeInterface<QEGLContext>();
    if (!eglContext)
        qFatal("Not running with EGL backend");

    EGLContext shareCtx = eglContext->nativeContext();
    Q_ASSERT(shareCtx != EGL_NO_CONTEXT);

    EGLDisplay dpy = (EGLDisplay) qGuiApp->platformNativeInterface()->nativeResourceForWindow(
        QByteArrayLiteral("egldisplay"), this);
    Q_ASSERT(dpy != EGL_NO_DISPLAY);

    QSurfaceFormat fmt = format();
    EGLConfig config = q_configFromGLFormat(dpy, fmt);

    QList<EGLint> contextAttrs;
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
    QOpenGLContext *ctxWrap = QEGLContext::fromNative(ctx, dpy, m_context);
    Q_ASSERT(ctxWrap->nativeInterface<QEGLContext>()->nativeContext() == ctx);

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
