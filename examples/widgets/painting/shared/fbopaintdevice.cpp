/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
}

QFboPaintDevice::~QFboPaintDevice()
{
    delete m_framebufferObject;
    delete m_surface;
}

void QFboPaintDevice::ensureActiveTarget()
{
    if (QOpenGLContext::currentContext() != context())
        context()->makeCurrent(m_surface);

    m_framebufferObject->bind();
}

GLuint QFboPaintDevice::takeTexture()
{
    // We have multisamples so we can't just forward takeTexture().
    QOpenGLFramebufferObject resolvedFbo(m_framebufferObject->size(), m_framebufferObject->attachment());
    QOpenGLFramebufferObject::blitFramebuffer(&resolvedFbo, m_framebufferObject);
    return resolvedFbo.takeTexture();
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
