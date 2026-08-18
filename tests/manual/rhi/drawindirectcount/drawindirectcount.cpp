// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/examplefw.h"

// Manual test for QRhiCommandBuffer::drawIndexedIndirectCount().
//
// QUAD_COUNT colored quads laid out in a row. The args buffer holds
// QUAD_COUNT QRhiIndexedIndirectDrawCommand entries (one per quad). Each
// frame a 1-thread compute writes the desired count into the count buffer
// (sinusoidal between 1 and QUAD_COUNT), then drawIndexedIndirectCount()
// issues only the first N commands.
//
// Expected visual: a row of colored quads that grows from the left and
// shrinks back. Broken implementations show a fixed width or 0/QUAD_COUNT.

static constexpr int QUAD_COUNT = 16;

struct Vertex {
    float pos[3];
    float color[4];
};

// std140-packed
struct CountUbuf {
    quint32 value;
    quint32 _pad[3];
};

struct {
    QList<QRhiResource *> releasePool;

    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *argsBuf = nullptr;
    QRhiBuffer *countBuf = nullptr;
    QRhiBuffer *countUbuf = nullptr;
    QRhiBuffer *gfxUbuf = nullptr;

    QRhiShaderResourceBindings *graphicsSrb = nullptr;
    QRhiGraphicsPipeline *graphicsPipeline = nullptr;

    QRhiShaderResourceBindings *countSrb = nullptr;
    QRhiComputePipeline *countPipeline = nullptr;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;

    int frame = 0;
} d;

static QList<Vertex> makeQuadRow()
{
    QList<Vertex> verts;
    verts.reserve(QUAD_COUNT * 4);
    // Lay QUAD_COUNT quads horizontally between x=-0.95 and x=0.95.
    const float margin = 0.02f;
    const float totalWidth = 1.9f;
    const float quadWidth = (totalWidth - margin * (QUAD_COUNT - 1)) / QUAD_COUNT;
    const float yTop = 0.5f;
    const float yBot = -0.5f;
    for (int i = 0; i < QUAD_COUNT; ++i) {
        const float x0 = -0.95f + i * (quadWidth + margin);
        const float x1 = x0 + quadWidth;
        // Distinct color per quad (HSV ramp).
        QColor c = QColor::fromHsvF(float(i) / float(QUAD_COUNT), 0.8f, 1.0f);
        const float r = float(c.redF());
        const float g = float(c.greenF());
        const float b = float(c.blueF());
        verts.push_back({ { x0, yBot, 0.0f }, { r, g, b, 1.0f } });
        verts.push_back({ { x1, yBot, 0.0f }, { r, g, b, 1.0f } });
        verts.push_back({ { x1, yTop, 0.0f }, { r, g, b, 1.0f } });
        verts.push_back({ { x0, yTop, 0.0f }, { r, g, b, 1.0f } });
    }
    return verts;
}

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::Compute))
        qFatal("Compute is not supported");
    if (!m_r->isFeatureSupported(QRhi::DrawIndirectCount))
        qFatal("DrawIndirectCount is not supported on this backend");
    if (!m_r->isFeatureSupported(QRhi::BaseVertex))
        qFatal("BaseVertex is not supported on this backend");

    const QList<Vertex> verts = makeQuadRow();
    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                            quint32(verts.size() * sizeof(Vertex)));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    static constexpr quint16 oneQuadIndices[6] = { 0, 1, 2, 2, 3, 0 };
    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(oneQuadIndices));
    d.ibuf->create();
    d.releasePool << d.ibuf;

    // Args buffer: one indexed-indirect-draw command per quad. All share the
    // same 6 indices, with vertexOffset selecting which quad's vertices the
    // indices refer to.
    QList<QRhiIndexedIndirectDrawCommand> argsList;
    argsList.reserve(QUAD_COUNT);
    for (int i = 0; i < QUAD_COUNT; ++i) {
        QRhiIndexedIndirectDrawCommand cmd = {};
        cmd.indexCount = 6;
        cmd.instanceCount = 1;
        cmd.firstIndex = 0;
        cmd.vertexOffset = i * 4;
        cmd.firstInstance = 0;
        argsList.push_back(cmd);
    }
    d.argsBuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndirectBuffer,
                               quint32(argsList.size() * sizeof(QRhiIndexedIndirectDrawCommand)));
    d.argsBuf->create();
    d.releasePool << d.argsBuf;

    // Count buffer: written by the compute pass each frame, read by the
    // device when drawIndexedIndirectCount executes.
    static constexpr quint32 zero = 0u;
    d.countBuf = m_r->newBuffer(QRhiBuffer::Static,
                                QRhiBuffer::StorageBuffer | QRhiBuffer::IndirectBuffer,
                                sizeof(quint32));
    d.countBuf->create();
    d.releasePool << d.countBuf;

    // CountUbuf: host-updated per frame. Tells the compute shader what
    // count value to write.
    d.countUbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(CountUbuf));
    d.countUbuf->create();
    d.releasePool << d.countUbuf;

    // Graphics MVP.
    d.gfxUbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
    d.gfxUbuf->create();
    d.releasePool << d.gfxUbuf;

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, verts.constData());
    d.initialUpdates->uploadStaticBuffer(d.ibuf, oneQuadIndices);
    d.initialUpdates->uploadStaticBuffer(d.argsBuf, argsList.constData());
    d.initialUpdates->uploadStaticBuffer(d.countBuf, &zero);

    const QMatrix4x4 mvp = m_r->clipSpaceCorrMatrix();
    d.initialUpdates->updateDynamicBuffer(d.gfxUbuf, 0, 64, mvp.constData());

    // Graphics SRB + pipeline.
    d.graphicsSrb = m_r->newShaderResourceBindings();
    d.graphicsSrb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.gfxUbuf)
    });
    d.graphicsSrb->create();
    d.releasePool << d.graphicsSrb;

    d.graphicsPipeline = m_r->newGraphicsPipeline();
    d.graphicsPipeline->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/quads.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/quads.frag.qsb")) }
    });
    QRhiVertexInputLayout vlayout;
    vlayout.setBindings({ { sizeof(Vertex) } });
    vlayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, pos) },
        { 0, 1, QRhiVertexInputAttribute::Float4, offsetof(Vertex, color) },
    });
    d.graphicsPipeline->setVertexInputLayout(vlayout);
    d.graphicsPipeline->setShaderResourceBindings(d.graphicsSrb);
    d.graphicsPipeline->setRenderPassDescriptor(m_rp);
    // Required by the Metal backend, which implements the count variants via
    // indirect command buffers. No effect on other backends.
    d.graphicsPipeline->setFlags(QRhiGraphicsPipeline::UsesIndirectDraws);
    d.graphicsPipeline->create();
    d.releasePool << d.graphicsPipeline;

    // Count-writer compute SRB + pipeline.
    d.countSrb = m_r->newShaderResourceBindings();
    d.countSrb->setBindings({
        QRhiShaderResourceBinding::bufferLoadStore(0, QRhiShaderResourceBinding::ComputeStage, d.countBuf),
        QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::ComputeStage, d.countUbuf)
    });
    d.countSrb->create();
    d.releasePool << d.countSrb;

    d.countPipeline = m_r->newComputePipeline();
    d.countPipeline->setShaderResourceBindings(d.countSrb);
    d.countPipeline->setShaderStage({ QRhiShaderStage::Compute, getShader(QLatin1String(":/count_writer.comp.qsb")) });
    d.countPipeline->create();
    d.releasePool << d.countPipeline;
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    const QSize outputSize = m_sc->currentPixelSize();
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    // Pick the active draw count for this frame. Oscillates between 1 and
    // QUAD_COUNT with a ~5s period.
    const float t = float(d.frame) * 0.02f;
    const quint32 activeCount = 1u + quint32((std::sin(t) * 0.5f + 0.5f) * (QUAD_COUNT - 1));

    CountUbuf cu = { activeCount, {0, 0, 0} };
    u->updateDynamicBuffer(d.countUbuf, 0, sizeof(CountUbuf), &cu);

    // Pass #1: write the count into the count buffer on the GPU.
    cb->beginComputePass(u);
    cb->setComputePipeline(d.countPipeline);
    cb->setShaderResources();
    cb->dispatch(1, 1, 1);
    cb->endComputePass();

    // Pass #2: render, consuming the freshly written count.
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.graphicsPipeline);
    cb->setShaderResources();
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    const QRhiCommandBuffer::VertexInput vbinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexedIndirectCount(d.argsBuf, 0,
                                 d.countBuf, 0,
                                 quint32(QUAD_COUNT));
    cb->endPass();

    ++d.frame;
}
