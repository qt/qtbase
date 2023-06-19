// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/examplefw.h"

// Multiview rendering. Renders the same geometry (a triangle) with two
// different transforms into two layers of a texture array object in a *single*
// draw call. (NB under the hood it is at the hardware/driver's discretion what
// happens; it may very well map to some simple looping and still drawing
// twice, whereas with modern hardware it can be expected to be implemented
// more efficiently, but that's hidden from us)

static float quadVertexData[] =
{ // Y up, CCW
  -0.5f,   0.5f,   0.0f, 0.0f,
  -0.5f,  -0.5f,   0.0f, 1.0f,
  0.5f,   -0.5f,   1.0f, 1.0f,
  0.5f,   0.5f,    1.0f, 0.0f
};

static quint16 quadIndexData[] =
{
    0, 1, 2, 0, 2, 3
};

static float triangleData[] =
{ // Y up, CCW
     0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
    -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f,
};

struct {
    QList<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTextureRenderTarget *rt = nullptr;
    QRhiRenderPassDescriptor *rtRp = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QMatrix4x4 winProj;
    QRhiTexture *tex = nullptr;
    QRhiShaderResourceBindings *srb[2] = {};

    QRhiBuffer *triUbuf = nullptr;
    QRhiShaderResourceBindings *triSrb = nullptr;
    QRhiGraphicsPipeline *triPs = nullptr;
    QMatrix4x4 triBaseMvp;
} d;

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::MultiView))
        qFatal("Multiview is not supported");

    // texture array with 2 elements, e.g. 0 is left eye, 1 is right
    d.tex = m_r->newTextureArray(QRhiTexture::RGBA8, 2, QSize(512, 512), 1, QRhiTexture::RenderTarget);
    d.releasePool << d.tex;
    d.tex->create();

    // set up the multiview render target
    QRhiColorAttachment multiViewAtt(d.tex);
    // using array elements 0 and 1
    multiViewAtt.setLayer(0);
    multiViewAtt.setMultiViewCount(2);
    QRhiTextureRenderTargetDescription rtDesc(multiViewAtt);
    d.rt = m_r->newTextureRenderTarget(rtDesc);
    d.releasePool << d.rt;
    d.rtRp = d.rt->newCompatibleRenderPassDescriptor();
    d.releasePool << d.rtRp;
    d.rt->setRenderPassDescriptor(d.rtRp);
    d.rt->create();

    // vertex buffer used by both passes
    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVertexData) + sizeof(triangleData));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    // resources for the on-screen visualizer
    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(quadIndexData));
    d.ibuf->create();
    d.releasePool << d.ibuf;

    const int oneRoundedUniformBlockSize = m_r->ubufAligned(72);
    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, oneRoundedUniformBlockSize * 2);
    d.ubuf->create();
    d.releasePool << d.ubuf;

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->create();

    // two srbs, just for the quad positioning on-screen
    for (int i = 0; i < 2; ++i) {
        QRhiShaderResourceBindings *srb = m_r->newShaderResourceBindings();
        d.releasePool << srb;
        srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                             d.ubuf, i * oneRoundedUniformBlockSize, 72),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.tex, d.sampler)
        });
        srb->create();
        d.srb[i] = srb;
    }

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;
    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/texture.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 4 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, quint32(2 * sizeof(float)) }
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb[0]); // all of them are layout-compatible
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, 0, sizeof(quadVertexData), quadVertexData);
    d.initialUpdates->uploadStaticBuffer(d.vbuf, sizeof(quadVertexData), sizeof(triangleData), triangleData);
    d.initialUpdates->uploadStaticBuffer(d.ibuf, quadIndexData);

    qint32 flip = m_r->isYUpInFramebuffer() ? 1 : 0;
    for (int i = 0; i < 2; ++i) {
        d.initialUpdates->updateDynamicBuffer(d.ubuf, i * oneRoundedUniformBlockSize + 64, 4, &flip);
        float layer = i;
        d.initialUpdates->updateDynamicBuffer(d.ubuf, i * oneRoundedUniformBlockSize + 68, 4, &layer);
    }

    // create resources for the multiview render pass
    d.triUbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 128); // mat4 mvp[2]
    d.releasePool << d.triUbuf;
    d.triUbuf->create();

    d.triSrb = m_r->newShaderResourceBindings();
    d.releasePool << d.triSrb;
    d.triSrb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.triUbuf)
    });
    d.triSrb->create();

    d.triPs = m_r->newGraphicsPipeline();
    d.releasePool << d.triPs;
    d.triPs->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/multiview.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/multiview.frag.qsb")) }
    });

    inputLayout.setBindings({
        { 5 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float3, quint32(2 * sizeof(float)) }
    });
    d.triPs->setVertexInputLayout(inputLayout);
    d.triPs->setShaderResourceBindings(d.triSrb);
    d.triPs->setRenderPassDescriptor(d.rtRp);
    d.triPs->create();

    d.triBaseMvp = m_r->clipSpaceCorrMatrix();
    d.triBaseMvp.perspective(45.0f, d.rt->pixelSize().width() / float(d.rt->pixelSize().height()), 0.01f, 1000.0f);
    d.triBaseMvp.translate(0, 0, -2);
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, quint32(sizeof(quadVertexData)));

    QMatrix4x4 triMvp = d.triBaseMvp;
    // let's say this is the left eye, make the triangle point left for now
    triMvp.rotate(90, 0, 0, 1);
    u->updateDynamicBuffer(d.triUbuf, 0, 64, triMvp.constData());
    triMvp = d.triBaseMvp;
    // right for the right eye
    triMvp.rotate(270, 0, 0, 1);
    u->updateDynamicBuffer(d.triUbuf, 64, 64, triMvp.constData());

    cb->beginPass(d.rt, QColor::fromRgbF(0.5f, 0.2f, 0.0f, 1.0f), { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.triPs);
    cb->setViewport({ 0, 0, float(d.rt->pixelSize().width()), float(d.rt->pixelSize().height()) });
    cb->setShaderResources();
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);
    cb->endPass();

    cb->resourceUpdate(u);

    // "blit" the two texture layers on-screen just to visualize the contents
    u = m_r->nextResourceUpdateBatch();
    if (d.winProj != m_proj) {
        d.winProj = m_proj;
        const int oneRoundedUniformBlockSize = m_r->ubufAligned(72);
        for (int i = 0; i < 2; ++i) {
            QMatrix4x4 mvp = m_proj;
            mvp.translate(0, 0, 1);
            if (i == 0)
                mvp.translate(-1.0f, 0, 0);
            else
                mvp.translate(1.0f, 0, 0);
            u->updateDynamicBuffer(d.ubuf, i * oneRoundedUniformBlockSize, 64, mvp.constData());
        }
    }
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    vbufBinding.second = 0;
    cb->setVertexInput(0, 1, &vbufBinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    for (int i = 0; i < 2; ++i) {
        cb->setShaderResources(d.srb[i]);
        cb->drawIndexed(6);
    }
    cb->endPass();
}
