// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/examplefw.h"

// Multiview rendering. Renders the same geometry (a triangle) with two
// different transforms into two layers of a texture array object in a *single*
// draw call. (NB under the hood it is at the hardware/driver's discretion what
// happens; it may very well map to some simple looping and still drawing
// twice, whereas with modern hardware it can be expected to be implemented
// more efficiently, but that's hidden from us)

// Toggle this to exercise 4x MSAA for the texture array that is the render
// target of the multiview render pass. The elements written by the multiview
// render pass get resolved to a non-multisample texture array at the end of
// the pass.
static bool MSAA = false;

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
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f
};

static const int INSTANCE_COUNT = 5;

static float instanceData[INSTANCE_COUNT * 3] =
{
    0.4f,  0.0f, 0.0f,
    0.2f,  0.0f, 0.1f,
    0.0f,  0.0f, 0.2f,
    -0.2f, 0.0f, 0.3f,
    -0.4f, 0.0f, 0.4f
};

struct {
    QList<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *instanceBuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTextureRenderTarget *rt = nullptr;
    QRhiRenderPassDescriptor *rtRp = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QMatrix4x4 winProj;
    QRhiTexture *tex = nullptr;
    QRhiTexture *resolveTex = nullptr; // only if MSAA is true
    QRhiTexture *ds = nullptr;
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

    int sampleCount = 1;
    if (MSAA) {
        qDebug("Using 4x MSAA for the multiview render pass");
        sampleCount = 4;
    }

    // texture array with 2 elements, e.g. 0 is left eye, 1 is right
    d.tex = m_r->newTextureArray(QRhiTexture::RGBA8, 2, QSize(512, 512), sampleCount, QRhiTexture::RenderTarget);
    d.releasePool << d.tex;
    d.tex->create();

    if (MSAA) {
        d.resolveTex = m_r->newTextureArray(QRhiTexture::RGBA8, 2, QSize(512, 512), 1, QRhiTexture::RenderTarget);
        d.releasePool << d.resolveTex;
        d.resolveTex->create();
    }

    // Have a depth-stencil buffer, just to exercise it, the triangles will be
    // rendered with depth test/write enabled. The catch here is that we must
    // use a texture array for depth/stencil as well, so QRhiRenderBuffer is
    // not an option anymore.
    d.ds = m_r->newTextureArray(QRhiTexture::D24S8, 2, QSize(512, 512), sampleCount, QRhiTexture::RenderTarget);
    d.releasePool << d.ds;
    d.ds->create();

    // set up the multiview render target
    QRhiColorAttachment multiViewAtt(d.tex);
    // using array elements 0 and 1
    multiViewAtt.setLayer(0);
    multiViewAtt.setMultiViewCount(2); // the view count must be set both on the render target and the pipeline

    // On-screen we work with a non-MSAA texture array, so the fragment shader
    // does not need to deal with sampler2DMSArray, but can use sampler2DArray
    // regardless of using multisampling or not. This means using an extra
    // non-MSAA 2D texture array into which both array elements get resolved at
    // the end of the multiview render pass.
    QRhiTexture *textureForOnscreenView = d.tex;
    if (MSAA) {
        multiViewAtt.setResolveTexture(d.resolveTex);
        textureForOnscreenView = d.resolveTex;
    }

    QRhiTextureRenderTargetDescription rtDesc(multiViewAtt);
    rtDesc.setDepthTexture(d.ds);

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

    // data for the instanced translation attribute
    d.instanceBuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(instanceData));
    d.instanceBuf->create();
    d.releasePool << d.instanceBuf;

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
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                      textureForOnscreenView, d.sampler)
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
    d.initialUpdates->uploadStaticBuffer(d.instanceBuf, instanceData);
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
    d.triPs->setMultiViewCount(2); // the view count must be set both on the render target and the pipeline
    inputLayout.setBindings({
        { 5 * sizeof(float) },
        { 3 * sizeof(float), QRhiVertexInputBinding::PerInstance }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float3, quint32(2 * sizeof(float)) },
        { 1, 2, QRhiVertexInputAttribute::Float3, 0 }
    });
    d.triPs->setDepthTest(true);
    d.triPs->setDepthWrite(true);
    d.triPs->setSampleCount(sampleCount);
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
    const QRhiCommandBuffer::VertexInput multiViewPassVbufBindings[] = {
        { d.vbuf, quint32(sizeof(quadVertexData)) },
        { d.instanceBuf, 0 }
    };
    cb->setVertexInput(0, 2, multiViewPassVbufBindings);
    cb->draw(3, INSTANCE_COUNT);
    cb->endPass();

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
    const QRhiCommandBuffer::VertexInput quadPassVBufBindings[] = { { d.vbuf, 0 } };
    cb->setVertexInput(0, 1, quadPassVBufBindings, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    for (int i = 0; i < 2; ++i) {
        cb->setShaderResources(d.srb[i]);
        cb->drawIndexed(6);
    }
    cb->endPass();
}
