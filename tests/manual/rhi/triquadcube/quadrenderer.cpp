/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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

#include "quadrenderer.h"
#include <QFile>
#include <QtGui/private/qshader_p.h>

// Renders a quad using indexed drawing. No QRhiGraphicsPipeline is created, it
// expects to reuse the one created by TriangleRenderer. A separate
// QRhiShaderResourceBindings is still needed, this will override the one the
// QRhiGraphicsPipeline references.

static float vertexData[] =
{ // Y up (note m_proj), CCW
  -0.5f,   0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
  -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
  0.5f,   -0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
  0.5f,   0.5f,    1.0f, 0.0f, 0.0f,   1.0f, 1.0f
};

static quint16 indexData[] =
{
    0, 1, 2, 0, 2, 3
};

void QuadRenderer::initResources(QRhiRenderPassDescriptor *)
{
    m_vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
    m_vbuf->build();
    m_vbufReady = false;

    m_ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indexData));
    m_ibuf->build();

    m_ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    m_ubuf->build();

    m_srb = m_r->newShaderResourceBindings();
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_ubuf)
    });
    m_srb->build();
}

void QuadRenderer::setPipeline(QRhiGraphicsPipeline *ps)
{
    m_ps = ps;
}

void QuadRenderer::resize(const QSize &pixelSize)
{
    m_proj = m_r->clipSpaceCorrMatrix();
    m_proj.perspective(45.0f, pixelSize.width() / (float) pixelSize.height(), 0.01f, 100.0f);
    m_proj.translate(0, 0, -4);
}

void QuadRenderer::releaseResources()
{
    delete m_srb;
    m_srb = nullptr;

    delete m_ubuf;
    m_ubuf = nullptr;

    delete m_ibuf;
    m_ibuf = nullptr;

    delete m_vbuf;
    m_vbuf = nullptr;
}

void QuadRenderer::queueResourceUpdates(QRhiResourceUpdateBatch *resourceUpdates)
{
    if (!m_vbufReady) {
        m_vbufReady = true;
        resourceUpdates->uploadStaticBuffer(m_vbuf, vertexData);
        resourceUpdates->uploadStaticBuffer(m_ibuf, indexData);
    }

    m_rotation += 1.0f;
    QMatrix4x4 mvp = m_proj;
    mvp.translate(m_translation);
    mvp.rotate(m_rotation, 0, 1, 0);
    resourceUpdates->updateDynamicBuffer(m_ubuf, 0, 64, mvp.constData());

    if (!m_opacityReady) {
        m_opacityReady = true;
        const float opacity = 1.0f;
        resourceUpdates->updateDynamicBuffer(m_ubuf, 64, 4, &opacity);
    }
}

void QuadRenderer::queueDraw(QRhiCommandBuffer *cb, const QSize &/*outputSizeInPixels*/)
{
    cb->setGraphicsPipeline(m_ps);
    //cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    cb->setShaderResources(m_srb);
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding, m_ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(6);
}
