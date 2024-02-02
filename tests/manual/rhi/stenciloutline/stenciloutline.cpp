// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/examplefw.h"
#include "../shared/cube.h"

static const quint32 UBUF_SIZE = 76;
static const float OUTLINE_SIZE = 1.05f;

struct {
    QList<QRhiResource *> releasePool;

    QRhiBuffer *vbuf;
    QRhiBuffer *ubuf;
    QRhiShaderResourceBindings *srb;
    QRhiGraphicsPipeline *psPass1;
    QRhiGraphicsPipeline *psPass2;
    float rot = 0.0f;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;
} d;

void Window::customInit()
{
    d.initialUpdates = m_r->nextResourceUpdateBatch();

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 2 * m_r->ubufAligned(UBUF_SIZE));
    d.ubuf->create();
    d.releasePool << d.ubuf;

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings({
            QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf, UBUF_SIZE)
        });
    d.srb->create();

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
    });

    const QRhiShaderStage stages[] = {
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/material.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/material.frag.qsb")) }
    };

    d.psPass1 = m_r->newGraphicsPipeline();
    d.releasePool << d.psPass1;
    d.psPass1->setShaderStages(stages, stages + 2);
    d.psPass1->setFlags(QRhiGraphicsPipeline::UsesStencilRef);
    d.psPass1->setDepthTest(true);
    d.psPass1->setDepthWrite(true);
    QRhiGraphicsPipeline::StencilOpState stencilPass1 = { QRhiGraphicsPipeline::Keep,
                                                          QRhiGraphicsPipeline::Keep,
                                                          QRhiGraphicsPipeline::Replace,
                                                          QRhiGraphicsPipeline::Always };
    d.psPass1->setStencilFront(stencilPass1);
    d.psPass1->setCullMode(QRhiGraphicsPipeline::Back);
    //d.psPass1->setStencilBack(stencilPass1);
    d.psPass1->setStencilTest(true);
    d.psPass1->setVertexInputLayout(inputLayout);
    d.psPass1->setShaderResourceBindings(d.srb);
    d.psPass1->setRenderPassDescriptor(m_rp);
    d.psPass1->create();

    d.psPass2 = m_r->newGraphicsPipeline();
    d.releasePool << d.psPass2;
    d.psPass2->setShaderStages(stages, stages + 2);
    d.psPass2->setFlags(QRhiGraphicsPipeline::UsesStencilRef);
    d.psPass2->setDepthTest(false);
    d.psPass2->setDepthWrite(false);
    QRhiGraphicsPipeline::StencilOpState stencilPass2 = { QRhiGraphicsPipeline::Keep,
                                                          QRhiGraphicsPipeline::Keep,
                                                          QRhiGraphicsPipeline::Replace,
                                                          QRhiGraphicsPipeline::NotEqual };
    d.psPass2->setStencilFront(stencilPass2);
    d.psPass2->setCullMode(QRhiGraphicsPipeline::Back);
    //d.psPass2->setStencilBack(stencilPass2);
    d.psPass2->setStencilTest(true);
    d.psPass2->setStencilWriteMask(0);
    d.psPass2->setVertexInputLayout(inputLayout);
    d.psPass2->setShaderResourceBindings(d.srb);
    d.psPass2->setRenderPassDescriptor(m_rp);
    d.psPass2->create();
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
    QRhiResourceUpdateBatch *u = nullptr;
    if (d.initialUpdates) {
        u = d.initialUpdates;
        d.initialUpdates = nullptr;
    }

    char *p = d.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    QMatrix4x4 mvp = m_proj;
    mvp.rotate(d.rot, 1, 1, 0);
    mvp.scale(0.5f);

    memcpy(p, mvp.constData(), 64);
    const float color[] = { 1.0f, 0.0f, 0.0f };
    memcpy(p + 64, color, 3 * sizeof(float));

    static const size_t pass2Ofs = m_r->ubufAligned(UBUF_SIZE);
    mvp.scale(OUTLINE_SIZE);
    memcpy(p + pass2Ofs, mvp.constData(), 64);
    const float pass2Color[] = { 0.0f, 0.0f, 1.0f };
    memcpy(p + pass2Ofs + 64, pass2Color, 3 * sizeof(float));

    d.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    const QRhiCommandBuffer::VertexInput vbufBinding[] = {
        { d.vbuf, 0 },
    };

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u, QRhiCommandBuffer::DoNotTrackResourcesForCompute);

    cb->setGraphicsPipeline(d.psPass1);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    QRhiCommandBuffer::DynamicOffset pass1DynOffset = { 0, 0 };
    cb->setShaderResources(d.srb, 1, &pass1DynOffset);
    cb->setVertexInput(0, 1, vbufBinding);
    cb->setStencilRef(1);
    cb->draw(36);

    cb->setGraphicsPipeline(d.psPass2);
    QRhiCommandBuffer::DynamicOffset pass2DynOffset = { 0, m_r->ubufAligned(UBUF_SIZE) };
    cb->setShaderResources(d.srb, 1, &pass2DynOffset);
    cb->setVertexInput(0, 1, vbufBinding);
    cb->setStencilRef(1);
    cb->draw(36);

    cb->endPass();

    d.rot += 1;
}
