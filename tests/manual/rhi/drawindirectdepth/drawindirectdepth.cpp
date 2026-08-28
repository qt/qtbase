// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define EXAMPLEFW_PREINIT
#include "../shared/examplefw.h"

// Manual test for indirect drawing combined with depth testing, rendering with
// MSAA straight to the window, i.e. to the swapchain, with the depth-stencil
// buffer that QRhiSwapChain requires: a QRhiRenderBuffer.
//
// Three quads, all with depth test and depth write enabled:
//
//   - a near quad on the left, drawn with an ordinary drawIndexed()
//   - a far quad covering the whole window, drawn with drawIndexedIndirect()
//   - a near quad on the right, drawn with an ordinary drawIndexed()
//
// Both near quads are closer than the far one, so both must stay visible on top
// of it. The draw count of the indirect draw alternates every two seconds
// between 128 and 130. On Metal, a count above 128 makes the backend encode the
// draws into an indirect command buffer, which needs a compute pass, so the
// render pass gets interrupted and continued on a new command encoder. The
// contents of a QRhiRenderBuffer depth-stencil buffer do not survive that, so
// the depth buffer is effectively cleared in the middle of the pass.
//
// Expected: two near quads on top of the far quad, unchanging.
//
// What actually happens with Metal: with a draw count of 130 the left quad
// disappears, because the far quad is drawn after the interruption and passes
// the depth test against a depth buffer that lost the left quad's depth values.
// The right quad is unaffected, it is drawn after the interruption. With a draw
// count of 128 everything is correct, since nothing interrupts the pass.
//
// Note that multisampling is not what breaks this, it is only used here because
// that is the interesting case for a window. The same happens without it.
//
// QRhiCommandBuffer::drawIndirectCount() and drawIndexedIndirectCount() always
// take the indirect command buffer path on Metal, regardless of the draw count,
// so they cannot avoid this at all.
//
// Set QT_RHI_TEST_PRESERVABLE_DS=1 in the environment to create the depth-stencil
// buffer with QRhiRenderBuffer::NoTransientBacking, which is one way to solve
// this. Both draw counts then give the correct result.
//
// The other way is not to interrupt the pass in the first place: record the
// draws into a QRhiIndirectCommandBuffer and issue them with executeIndirect().
// See the indirectcommandbuffer manual test, which is this same scene done that
// way, and stays correct at both draw counts with a transient depth-stencil
// buffer.

static const quint32 SAFE_DRAW_COUNT = 128;   // at or below the Metal ICB threshold
static const quint32 BROKEN_DRAW_COUNT = 130; // above it

struct Vertex {
    float pos[3];
    float color[4];
};

// Quad 0: near, left. Quad 1: near, right. Quad 2: far, fullscreen.
static const Vertex vertexData[] = {
    { { -0.9f, -0.6f, -0.5f }, { 0.9f, 0.2f, 0.2f, 1.0f } },
    { { -0.1f, -0.6f, -0.5f }, { 0.9f, 0.2f, 0.2f, 1.0f } },
    { { -0.1f,  0.6f, -0.5f }, { 0.9f, 0.2f, 0.2f, 1.0f } },
    { { -0.9f,  0.6f, -0.5f }, { 0.9f, 0.2f, 0.2f, 1.0f } },

    { {  0.1f, -0.6f, -0.5f }, { 0.2f, 0.9f, 0.2f, 1.0f } },
    { {  0.9f, -0.6f, -0.5f }, { 0.2f, 0.9f, 0.2f, 1.0f } },
    { {  0.9f,  0.6f, -0.5f }, { 0.2f, 0.9f, 0.2f, 1.0f } },
    { {  0.1f,  0.6f, -0.5f }, { 0.2f, 0.9f, 0.2f, 1.0f } },

    { { -1.0f, -1.0f,  0.5f }, { 0.2f, 0.2f, 0.5f, 1.0f } },
    { {  1.0f, -1.0f,  0.5f }, { 0.2f, 0.2f, 0.5f, 1.0f } },
    { {  1.0f,  1.0f,  0.5f }, { 0.2f, 0.2f, 0.5f, 1.0f } },
    { { -1.0f,  1.0f,  0.5f }, { 0.2f, 0.2f, 0.5f, 1.0f } },
};

static const quint16 indexData[] = { 0, 1, 2, 2, 3, 0 };

struct {
    QList<QRhiResource *> releasePool;

    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *argsBuf = nullptr;
    QRhiBuffer *ubuf = nullptr;

    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;

    int frame = 0;
    bool broken = false;
} d;

void preInit()
{
    sampleCount = 4; // MSAA swapchain
    scFlags |= QRhiSwapChain::UsedAsTransferSource; // to check the result below
    if (qEnvironmentVariableIntValue("QT_RHI_TEST_PRESERVABLE_DS")) {
        qDebug("Creating the depth-stencil buffer with NoTransientBacking");
        dsFlags |= QRhiRenderBuffer::NoTransientBacking;
    }
}

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::DrawIndirect))
        qFatal("DrawIndirect is not supported");
    if (!m_r->isFeatureSupported(QRhi::BaseVertex))
        qFatal("BaseVertex is not supported");

    qDebug("Expected: a red quad on the left and a green quad on the right, both\n"
           "  on top of the dark blue background quad, and both staying put.\n"
           "  If the left one disappears every other two seconds, the depth buffer\n"
           "  is losing its contents in the middle of the render pass. The result is\n"
           "  also sampled and printed once per draw count change.");

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
    if (!d.vbuf->create())
        qFatal("failed to create d.vbuf");
    d.releasePool << d.vbuf;

    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indexData));
    if (!d.ibuf->create())
        qFatal("failed to create d.ibuf");
    d.releasePool << d.ibuf;

    // Every command draws the far quad, which starts at vertex 8.
    QList<QRhiIndexedIndirectDrawCommand> args;
    args.reserve(BROKEN_DRAW_COUNT);
    for (quint32 i = 0; i < BROKEN_DRAW_COUNT; ++i) {
        QRhiIndexedIndirectDrawCommand cmd = {};
        cmd.indexCount = 6;
        cmd.instanceCount = 1;
        cmd.firstIndex = 0;
        cmd.vertexOffset = 8;
        cmd.firstInstance = 0;
        args.push_back(cmd);
    }
    d.argsBuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndirectBuffer,
                               quint32(args.size() * sizeof(QRhiIndexedIndirectDrawCommand)));
    if (!d.argsBuf->create())
        qFatal("failed to create d.argsBuf");
    d.releasePool << d.argsBuf;

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
    if (!d.ubuf->create())
        qFatal("failed to create d.ubuf");
    d.releasePool << d.ubuf;

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, vertexData);
    d.initialUpdates->uploadStaticBuffer(d.ibuf, indexData);
    d.initialUpdates->uploadStaticBuffer(d.argsBuf, args.constData());
    const QMatrix4x4 mvp = m_r->clipSpaceCorrMatrix();
    d.initialUpdates->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());

    d.srb = m_r->newShaderResourceBindings();
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.ubuf)
    });
    if (!d.srb->create())
        qFatal("failed to create srb");
    d.releasePool << d.srb;

    d.ps = m_r->newGraphicsPipeline();
    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/quads.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/quads.frag.qsb")) }
    });
    QRhiVertexInputLayout vlayout;
    vlayout.setBindings({ { sizeof(Vertex) } });
    vlayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, offsetof(Vertex, pos) },
        { 0, 1, QRhiVertexInputAttribute::Float4, offsetof(Vertex, color) },
    });
    d.ps->setVertexInputLayout(vlayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->setSampleCount(sampleCount);
    d.ps->setDepthTest(true);
    d.ps->setDepthWrite(true);
    d.ps->setDepthOp(QRhiGraphicsPipeline::Less);
    // Without this the Metal backend never uses indirect command buffers, and
    // the problem does not show up at all.
    d.ps->setFlags(QRhiGraphicsPipeline::UsesIndirectDraws);
    if (!d.ps->create())
        qFatal("failed to create ps");
    d.releasePool << d.ps;
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
    QRhiResourceUpdateBatch *u = nullptr;
    if (d.initialUpdates) {
        u = d.initialUpdates;
        d.initialUpdates = nullptr;
    }

    // Alternate between a draw count that stays below the Metal indirect command
    // buffer threshold and one that goes above it.
    if (d.frame % 120 == 0) {
        d.broken = !d.broken;
        qDebug("draw count %u%s", d.broken ? BROKEN_DRAW_COUNT : SAFE_DRAW_COUNT,
               d.broken ? " (interrupts the render pass on Metal)" : "");
    }
    const quint32 drawCount = d.broken ? BROKEN_DRAW_COUNT : SAFE_DRAW_COUNT;

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setShaderResources();
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    const QRhiCommandBuffer::VertexInput vbinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);

    // Near quad on the left, before the indirect draw.
    cb->drawIndexed(6);

    // Far quad covering everything. Must not overdraw the quad above.
    cb->drawIndexedIndirect(d.argsBuf, 0, drawCount);

    // Near quad on the right, after the indirect draw. Always survives.
    cb->drawIndexed(6, 1, 0, 4);

    // Sample the middle of both near quads, so that the outcome is also visible
    // without looking at the window.
    QRhiResourceUpdateBatch *passEndUpdates = nullptr;
    if (d.frame % 120 == 60) {
        passEndUpdates = m_r->nextResourceUpdateBatch();
        QRhiReadbackDescription rb; // no texture given -> backbuffer
        QRhiReadbackResult *result = new QRhiReadbackResult;
        const bool yUp = m_r->isYUpInFramebuffer();
        const quint32 count = drawCount;
        result->completed = [result, yUp, count] {
            const QImage::Format fmt = result->format == QRhiTexture::BGRA8
                    ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGBA8888_Premultiplied;
            QImage image(reinterpret_cast<const uchar *>(result->data.constData()),
                         result->pixelSize.width(), result->pixelSize.height(), fmt);
            if (yUp)
                image = image.flipped();
            const int y = image.height() / 2;
            const QRgb left = image.pixel(image.width() / 4, y);
            const QRgb right = image.pixel(image.width() * 3 / 4, y);
            const bool leftOk = qRed(left) > qBlue(left);
            const bool rightOk = qGreen(right) > qBlue(right);
            qDebug("draw count %u: left quad %s, right quad %s%s", count,
                   leftOk ? "present" : "MISSING", rightOk ? "present" : "MISSING",
                   (leftOk && rightOk) ? "" : "   <-- depth contents were lost");
            delete result;
        };
        passEndUpdates->readBackTexture(rb, result);
    }

    cb->endPass(passEndUpdates);

    ++d.frame;
}
