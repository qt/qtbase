// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/examplefw.h"

// Manual test for QRhiCommandBuffer::dispatchIndirect().
//
// Per frame:
//   0. reset: clear all particles to their grid position + white.
//   1. args: a 1-thread compute writes (groupsX, 1, 1) into an
//      IndirectBuffer/StorageBuffer.
//   2. consume: indirect compute dispatch using args from step 1, paints the
//      first groupsX rows red and gives them a sine wiggle.
//   3. draw all particles as points.
//
// One work group per row, LOCAL_SIZE_X = COLS, so groupsX is the row count.
// groupsX oscillates between MIN_ACTIVE_GROUPS and MAX_ACTIVE_GROUPS.
//
// Expected visual: a red band at the top whose height oscillates between 8
// and 128 rows. If broken the band is fixed-height, absent, or lagged.

static constexpr int COLS = 256;
static constexpr int ROWS = 128;
static constexpr quint32 PARTICLE_COUNT = quint32(COLS * ROWS); // 32768

static constexpr quint32 LOCAL_SIZE_X = quint32(COLS);
static constexpr quint32 MIN_ACTIVE_GROUPS = 8u;           // 8 rows animated
static constexpr quint32 MAX_ACTIVE_GROUPS = quint32(ROWS); // all rows animated

struct {
    QList<QRhiResource *> releasePool;

    QRhiBuffer *particles = nullptr;
    QRhiBuffer *indirectBuf = nullptr;
    QRhiBuffer *argsUbuf = nullptr;
    QRhiBuffer *frameUbuf = nullptr;
    QRhiBuffer *gfxUbuf = nullptr;

    QRhiShaderResourceBindings *writeBindings = nullptr;
    QRhiComputePipeline *writePipeline = nullptr;

    QRhiShaderResourceBindings *resetBindings = nullptr;
    QRhiComputePipeline *resetPipeline = nullptr;

    QRhiShaderResourceBindings *consumeBindings = nullptr;
    QRhiComputePipeline *consumePipeline = nullptr;

    QRhiShaderResourceBindings *graphicsBindings = nullptr;
    QRhiGraphicsPipeline *graphicsPipeline = nullptr;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;

    int frame = 0;
} d;

// std430: vec4 pos (16 B) + vec4 colour (16 B) -> 32 B. Match on the CPU.
struct Particle {
    float pos[4];
    float color[4];
};

// std140: each scalar is padded to its own alignment, but we pack as vec4s.
struct ArgsUbuf {
    quint32 groupsX;
    quint32 groupsY;
    quint32 groupsZ;
    quint32 _pad;
};

struct FrameUbuf {
    float time;
    float _pad[3];
};

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::Compute))
        qFatal("Compute is not supported");
    if (!m_r->isFeatureSupported(QRhi::DispatchIndirect))
        qFatal("Indirect compute dispatch is not supported on this backend");

    d.particles = m_r->newBuffer(QRhiBuffer::Static,
                                 QRhiBuffer::StorageBuffer | QRhiBuffer::VertexBuffer,
                                 sizeof(Particle) * PARTICLE_COUNT);
    d.particles->create();
    d.releasePool << d.particles;

    d.indirectBuf = m_r->newBuffer(QRhiBuffer::Static,
                                   QRhiBuffer::StorageBuffer | QRhiBuffer::IndirectBuffer,
                                   sizeof(ArgsUbuf));
    d.indirectBuf->create();
    d.releasePool << d.indirectBuf;

    d.argsUbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(ArgsUbuf));
    d.argsUbuf->create();
    d.releasePool << d.argsUbuf;

    d.frameUbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(FrameUbuf));
    d.frameUbuf->create();
    d.releasePool << d.frameUbuf;

    d.gfxUbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
    d.gfxUbuf->create();
    d.releasePool << d.gfxUbuf;

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    const QMatrix4x4 mvp = m_r->clipSpaceCorrMatrix();
    d.initialUpdates->updateDynamicBuffer(d.gfxUbuf, 0, 64, mvp.constData());

    // Lay particles on a fixed grid: row r is at y = 1 - 2*(r+0.5)/ROWS
    // (so row 0 is at the top, row ROWS-1 at the bottom). Column c is at
    // x = -1 + 2*(c+0.5)/COLS.
    QByteArray particleData;
    particleData.resize(int(sizeof(Particle) * PARTICLE_COUNT));
    Particle *pp = reinterpret_cast<Particle *>(particleData.data());
    for (int r = 0; r < ROWS; ++r) {
        const float y = 1.0f - 2.0f * (float(r) + 0.5f) / float(ROWS);
        for (int c = 0; c < COLS; ++c) {
            const int i = r * COLS + c;
            pp[i].pos[0] = -1.0f + 2.0f * (float(c) + 0.5f) / float(COLS);
            pp[i].pos[1] = y;
            pp[i].pos[2] = 0.0f;
            pp[i].pos[3] = 1.0f;
            // Initial colour: white (static).
            pp[i].color[0] = 1.0f;
            pp[i].color[1] = 1.0f;
            pp[i].color[2] = 1.0f;
            pp[i].color[3] = 1.0f;
        }
    }
    d.initialUpdates->uploadStaticBuffer(d.particles, particleData.constData());

    static constexpr ArgsUbuf zeroArgs = {};
    d.initialUpdates->uploadStaticBuffer(d.indirectBuf, &zeroArgs);

    // -- args-writer pipeline ---------------------------------------------
    d.writeBindings = m_r->newShaderResourceBindings();
    d.writeBindings->setBindings({
        QRhiShaderResourceBinding::bufferLoadStore(0, QRhiShaderResourceBinding::ComputeStage, d.indirectBuf),
        QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::ComputeStage, d.argsUbuf)
    });
    d.writeBindings->create();
    d.releasePool << d.writeBindings;

    d.writePipeline = m_r->newComputePipeline();
    d.writePipeline->setShaderResourceBindings(d.writeBindings);
    d.writePipeline->setShaderStage({ QRhiShaderStage::Compute, getShader(QLatin1String(":/dispatch_args.comp.qsb")) });
    d.writePipeline->create();
    d.releasePool << d.writePipeline;

    // -- reset pipeline (direct dispatch; clears all particles) ------------
    d.resetBindings = m_r->newShaderResourceBindings();
    d.resetBindings->setBindings({
        QRhiShaderResourceBinding::bufferLoadStore(0, QRhiShaderResourceBinding::ComputeStage, d.particles)
    });
    d.resetBindings->create();
    d.releasePool << d.resetBindings;

    d.resetPipeline = m_r->newComputePipeline();
    d.resetPipeline->setShaderResourceBindings(d.resetBindings);
    d.resetPipeline->setShaderStage({ QRhiShaderStage::Compute, getShader(QLatin1String(":/dispatch_reset.comp.qsb")) });
    d.resetPipeline->create();
    d.releasePool << d.resetPipeline;

    // -- consumer pipeline (the indirect-dispatched one) -------------------
    d.consumeBindings = m_r->newShaderResourceBindings();
    d.consumeBindings->setBindings({
        QRhiShaderResourceBinding::bufferLoadStore(0, QRhiShaderResourceBinding::ComputeStage, d.particles),
        QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::ComputeStage, d.frameUbuf)
    });
    d.consumeBindings->create();
    d.releasePool << d.consumeBindings;

    d.consumePipeline = m_r->newComputePipeline();
    d.consumePipeline->setShaderResourceBindings(d.consumeBindings);
    d.consumePipeline->setShaderStage({ QRhiShaderStage::Compute, getShader(QLatin1String(":/dispatch_consume.comp.qsb")) });
    d.consumePipeline->create();
    d.releasePool << d.consumePipeline;

    // -- graphics pipeline -------------------------------------------------
    d.graphicsBindings = m_r->newShaderResourceBindings();
    d.graphicsBindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.gfxUbuf)
    });
    d.graphicsBindings->create();
    d.releasePool << d.graphicsBindings;

    d.graphicsPipeline = m_r->newGraphicsPipeline();
    d.graphicsPipeline->setTopology(QRhiGraphicsPipeline::Points);
    d.graphicsPipeline->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/main.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/main.frag.qsb")) }
    });
    QRhiVertexInputLayout layout;
    layout.setBindings({ { sizeof(Particle) } });
    layout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float4, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float4, 4 * sizeof(float) },
    });
    d.graphicsPipeline->setVertexInputLayout(layout);
    d.graphicsPipeline->setShaderResourceBindings(d.graphicsBindings);
    d.graphicsPipeline->setRenderPassDescriptor(m_rp);
    d.graphicsPipeline->create();
    d.releasePool << d.graphicsPipeline;
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

    const float t = float(d.frame) * 0.02f;
    const quint32 range = MAX_ACTIVE_GROUPS - MIN_ACTIVE_GROUPS;
    const quint32 activeGroups = MIN_ACTIVE_GROUPS + quint32((std::sin(t) * 0.5f + 0.5f) * range);

    ArgsUbuf args = { activeGroups, 1u, 1u, 0u };
    u->updateDynamicBuffer(d.argsUbuf, 0, sizeof(ArgsUbuf), &args);

    FrameUbuf frame = { t, {0, 0, 0} };
    u->updateDynamicBuffer(d.frameUbuf, 0, sizeof(FrameUbuf), &frame);

    // Pass #0: reset every particle to white + grid position so only rows
    // touched by the indirect dispatch this frame end up red.
    cb->beginComputePass(u);
    cb->setComputePipeline(d.resetPipeline);
    cb->setShaderResources();
    cb->dispatch(ROWS, 1, 1);
    cb->endComputePass();

    // Pass #1: write indirect args from the GPU.
    cb->beginComputePass();
    cb->setComputePipeline(d.writePipeline);
    cb->setShaderResources();
    cb->dispatch(1, 1, 1);
    cb->endComputePass();

    // Pass #2: actually dispatch using the args produced above.
    cb->beginComputePass();
    cb->setComputePipeline(d.consumePipeline);
    cb->setShaderResources();
    cb->dispatchIndirect(d.indirectBuf, 0);
    cb->endComputePass();

    // Pass #3: draw all particles.
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.graphicsPipeline);
    cb->setShaderResources();
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    QRhiCommandBuffer::VertexInput vbufBinding(d.particles, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(PARTICLE_COUNT);
    cb->endPass();

    ++d.frame;
}
