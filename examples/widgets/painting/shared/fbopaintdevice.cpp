// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "fbopaintdevice.h"

#include <QOffscreenSurface>
#include <QOpenGLFunctions>

QFboPaintDevice::QFboPaintDevice(const QSize &size, bool flipped, bool clearOnInit,
                                 QOpenGLFramebufferObject::Attachment attachment)
        : QOpenGLPaintDevice(size)
{
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(attachment);
    format.setSamples(4);
    m_framebufferObject = new QOpenGLFramebufferObject(size, format);
    QOffscreenSurface *surface = new QOffscreenSurface();
    surface->create();
    m_surface = surface;
    setPaintFlipped(flipped);
    if (clearOnInit) {
        m_framebufferObject->bind();

        context()->functions()->glClearColor(0, 0, 0, 0);
        context()->functions()->glClear(GL_COLOR_BUFFER_BIT);
    }
    m_resolvedFbo = new QOpenGLFramebufferObject(m_framebufferObject->size(), m_framebufferObject->attachment());
}

QFboPaintDevice::~QFboPaintDevice()
{
    delete m_framebufferObject;
    delete m_resolvedFbo;
    delete m_surface;
}

void QFboPaintDevice::ensureActiveTarget()
{
    if (QOpenGLContext::currentContext() != context())
        context()->makeCurrent(m_surface);

    m_framebufferObject->bind();
}

GLuint QFboPaintDevice::texture()
{
    m_resolvedFbo->bind(); // to get the backing texture recreated if it was taken (in takeTexture) previously
    QOpenGLFramebufferObject::blitFramebuffer(m_resolvedFbo, m_framebufferObject);
    return m_resolvedFbo->texture();
}

GLuint QFboPaintDevice::takeTexture()
{
    m_resolvedFbo->bind(); // to get the backing texture recreated if it was taken (in takeTexture) previously
    // We have multisamples so we can't just forward takeTexture(), have to resolve first.
    QOpenGLFramebufferObject::blitFramebuffer(m_resolvedFbo, m_framebufferObject);
    return m_resolvedFbo->takeTexture();
}

QImage QFboPaintDevice::toImage() const
{
    QOpenGLContext *currentContext = QOpenGLContext::currentContext();
    QSurface *currentSurface = currentContext ? currentContext->surface() : nullptr;

    context()->makeCurrent(m_surface);

    QImage image = m_framebufferObject->toImage(!paintFlipped());

    if (currentContext)
        currentContext->makeCurrent(currentSurface);
    else
        context()->doneCurrent();

    return image;
}
