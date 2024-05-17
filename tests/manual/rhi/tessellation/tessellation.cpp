// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/examplefw.h"

static const float tri[] = {
    0.0f, 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,
};

static const bool INDEXED = false;
static const quint32 indices[] = { 0, 1, 2 };

struct {
    QVector<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QMatrix4x4 winProj;
    float time = 0.0f;
} d;

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::Tessellation))
        qFatal("Tessellation is not supported");

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(tri));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    if (INDEXED) {
        d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indices));
        d.ibuf->create();
        d.releasePool << d.ibuf;
    }

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4 + 4);
    d.ubuf->create();
    d.releasePool << d.ubuf;

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    const QRhiShaderResourceBinding::StageFlags tese = QRhiShaderResourceBinding::TessellationEvaluationStage;
    d.srb->setBindings({ QRhiShaderResourceBinding::uniformBuffer(0, tese, d.ubuf) });
    d.srb->create();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;

    d.ps->setTopology(QRhiGraphicsPipeline::Patches);
    d.ps->setPatchControlPointCount(3);

    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/test.vert.qsb")) },
        { QRhiShaderStage::TessellationControl, getShader(QLatin1String(":/test.tesc.qsb")) },
        { QRhiShaderStage::TessellationEvaluation, getShader(QLatin1String(":/test.tese.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/test.frag.qsb")) }
    });

    d.ps->setCullMode(QRhiGraphicsPipeline::Back);
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
    d.initialUpdates->uploadStaticBuffer(d.vbuf, tri);

    const float amplitude = 0.5f;
    d.initialUpdates->updateDynamicBuffer(d.ubuf, 68, 4, &amplitude);

    if (INDEXED)
        d.initialUpdates->uploadStaticBuffer(d.ibuf, indices);
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
    if (d.winProj != m_proj) {
        d.winProj = m_proj;
        u->updateDynamicBuffer(d.ubuf, 0, 64, d.winProj.constData());
    }
    u->updateDynamicBuffer(d.ubuf, 64, 4, &d.time);
    d.time += 0.01f;

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    if (INDEXED) {
        cb->setVertexInput(0, 1, &vbufBinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt32);
        cb->drawIndexed(3);
    } else {
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->draw(3);
    }

    cb->endPass();
}
