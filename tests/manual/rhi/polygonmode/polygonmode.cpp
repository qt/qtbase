// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/examplefw.h"

static const float geom[] = {
    -0.5f, 0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
    0.0f, 0.5f, 0.0f,    0.0f, 0.0f, 1.0f,
    0.0f, -0.5f, 0.0f,   1.0f, 0.0f, 1.0f,
    0.5f, 0.5f, 0.0f,    1.0f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 1.0f,

};

struct {
    QVector<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    int count = 0;
} d;

void Window::customInit()
{
    m_clearColor = QColor("black");

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(geom));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->create();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;

    d.ps->setTopology(QRhiGraphicsPipeline::TriangleStrip);

    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/test.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/test.frag.qsb")) }
    });

    d.ps->setCullMode(QRhiGraphicsPipeline::None);
    d.ps->setPolygonMode(QRhiGraphicsPipeline::Line);
    d.ps->setDepthTest(true);
    d.ps->setDepthWrite(true);
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 6 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) }
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, geom);
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    QRhiGraphicsPipeline::PolygonMode polygonMode =
            (++d.count / 60) % 2 ? QRhiGraphicsPipeline::Fill : QRhiGraphicsPipeline::Line;

    if (d.ps->polygonMode() != polygonMode) {
        d.ps->setPolygonMode(polygonMode);
        d.ps->create();
    }

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(6);
    cb->endPass();
}
