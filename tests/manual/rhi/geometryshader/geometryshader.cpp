// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/examplefw.h"

static const float points[] = { 0.0f, 0.0f, 0.0f };

struct
{
    QVector<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    float radius = 0.0f;
} d;

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::GeometryShader))
        qFatal("Geometry shaders are not supported");

    m_clearColor.setRgb(0, 0, 0);

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(points));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 4);
    d.ubuf->create();
    d.releasePool << d.ubuf;

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    const QRhiShaderResourceBinding::StageFlags geom = QRhiShaderResourceBinding::GeometryStage;
    d.srb->setBindings({ QRhiShaderResourceBinding::uniformBuffer(0, geom, d.ubuf) });
    d.srb->create();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;

    d.ps->setTopology(QRhiGraphicsPipeline::Points);

    d.ps->setShaderStages(
            { { QRhiShaderStage::Vertex, getShader(QLatin1String(":/test.vert.qsb")) },
              { QRhiShaderStage::Geometry, getShader(QLatin1String(":/test.geom.qsb")) },
              { QRhiShaderStage::Fragment, getShader(QLatin1String(":/test.frag.qsb")) } });

    d.ps->setCullMode(QRhiGraphicsPipeline::Back);
    d.ps->setDepthTest(true);
    d.ps->setDepthWrite(true);
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 3 * sizeof(float) } });
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 } });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, points);
    d.initialUpdates->updateDynamicBuffer(d.ubuf, 0, 4, &d.radius);
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

    u->updateDynamicBuffer(d.ubuf, 0, 4, &d.radius);
    d.radius = std::fmod(d.radius + 0.01f, 1.0f);

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(1);
    cb->endPass();
}
