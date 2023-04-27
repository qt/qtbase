// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "quadrenderer.h"
#include <QFile>
#include <rhi/qshader.h>

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
    m_vbuf->create();
    m_vbufReady = false;

    m_ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indexData));
    m_ibuf->create();

    m_ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    m_ubuf->create();

    m_srb = m_r->newShaderResourceBindings();
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_ubuf)
    });
    m_srb->create();
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
