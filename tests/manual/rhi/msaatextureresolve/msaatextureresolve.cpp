// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/examplefw.h"

// Uses a multisample texture both for color and depth-stencil, renders into
// those, and then resolves into non-multisample textures. Also the
// depth-stencil, in order to exercise that rarely used path. If that is
// functional, will not be visible on-screen. Frame captures with tools such as
// RenderDoc can be used to verify that there is indeed a non-multisample depth
// texture generated.

static float vertexData[] =
{ // Y up, CCW
  -0.5f,   0.5f,   0.0f, 0.0f,
  -0.5f,  -0.5f,   0.0f, 1.0f,
  0.5f,   -0.5f,   1.0f, 1.0f,
  0.5f,   0.5f,    1.0f, 0.0f
};

static quint16 indexData[] =
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
    QRhiTexture *msaaColorTexture = nullptr;
    QRhiTexture *msaaDepthTexture = nullptr;
    QRhiTextureRenderTarget *rt = nullptr;
    QRhiRenderPassDescriptor *rtRp = nullptr;
    QRhiTexture *tex = nullptr;
    QRhiTexture *depthTex = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiBuffer *triUbuf = nullptr;
    QRhiShaderResourceBindings *triSrb = nullptr;
    QRhiGraphicsPipeline *triPs = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QMatrix4x4 triBaseMvp;
    float triRot = 0;
    QMatrix4x4 winProj;
} d;

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::MultisampleTexture))
        qFatal("Multisample textures not supported by this backend");

    // Skip the check for ResolveDepthStencil, and let it run regardless. When
    // not supported, it won't be functional, but should be handled somewhat
    // gracefully.

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData) + sizeof(triangleData));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indexData));
    d.ibuf->create();
    d.releasePool << d.ibuf;

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    d.ubuf->create();
    d.releasePool << d.ubuf;

    d.msaaColorTexture = m_r->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 4, QRhiTexture::RenderTarget); // 4x MSAA
    d.msaaColorTexture->create();
    d.releasePool << d.msaaColorTexture;

    d.msaaDepthTexture = m_r->newTexture(QRhiTexture::D24S8, QSize(512, 512), 4, QRhiTexture::RenderTarget); // 4x MSAA
    d.msaaDepthTexture->create();
    d.releasePool << d.msaaDepthTexture;

    // the non-msaa texture that will be the destination in the resolve
    d.tex = m_r->newTexture(QRhiTexture::RGBA8, d.msaaColorTexture->pixelSize(), 1, QRhiTexture::RenderTarget);
    d.releasePool << d.tex;
    d.tex->create();

    d.depthTex = m_r->newTexture(QRhiTexture::D24S8, d.msaaDepthTexture->pixelSize(), 1, QRhiTexture::RenderTarget);
    d.releasePool << d.depthTex;
    d.depthTex->create();

    QRhiTextureRenderTargetDescription rtDesc;
    QRhiColorAttachment rtAtt(d.msaaColorTexture);
    rtAtt.setResolveTexture(d.tex);
    rtDesc.setColorAttachments({ rtAtt });
    rtDesc.setDepthTexture(d.msaaDepthTexture);
    rtDesc.setDepthResolveTexture(d.depthTex);

    d.rt = m_r->newTextureRenderTarget(rtDesc);
    d.releasePool << d.rt;
    d.rtRp = d.rt->newCompatibleRenderPassDescriptor();
    d.releasePool << d.rtRp;
    d.rt->setRenderPassDescriptor(d.rtRp);
    d.rt->create();

    d.triUbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
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
    d.triPs->setDepthTest(true);
    d.triPs->setDepthWrite(true);
    d.triPs->setSampleCount(4); // must match the render target
    d.triPs->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/color.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/color.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
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

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->create();

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.tex, d.sampler)
    });
    d.srb->create();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;
    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/texture.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture.frag.qsb")) }
    });
    inputLayout.setBindings({
        { 4 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, quint32(2 * sizeof(float)) }
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, 0, sizeof(vertexData), vertexData);
    d.initialUpdates->uploadStaticBuffer(d.vbuf, sizeof(vertexData), sizeof(triangleData), triangleData);
    d.initialUpdates->uploadStaticBuffer(d.ibuf, indexData);

    d.triBaseMvp = m_r->clipSpaceCorrMatrix();
    d.triBaseMvp.perspective(45.0f, d.msaaColorTexture->pixelSize().width() / float(d.msaaColorTexture->pixelSize().height()), 0.01f, 1000.0f);
    d.triBaseMvp.translate(0, 0, -2);
    float opacity = 1.0f;
    d.initialUpdates->updateDynamicBuffer(d.triUbuf, 64, 4, &opacity);

    qint32 flip = m_r->isYUpInFramebuffer() ? 1 : 0;
    d.initialUpdates->updateDynamicBuffer(d.ubuf, 64, 4, &flip);
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
    triMvp.rotate(d.triRot, 0, 1, 0);
    d.triRot += 1;
    u->updateDynamicBuffer(d.triUbuf, 0, 64, triMvp.constData());

    if (d.winProj != m_proj) {
        d.winProj = m_proj;
        QMatrix4x4 mvp = m_proj;
        mvp.scale(2.5f);
        u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());
    }

    // offscreen (triangle, msaa)
    cb->beginPass(d.rt, QColor::fromRgbF(0.5f, 0.2f, 0.0f, 1.0f), { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.triPs);
    cb->setViewport({ 0, 0, float(d.msaaColorTexture->pixelSize().width()), float(d.msaaColorTexture->pixelSize().height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, quint32(sizeof(vertexData)));
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);
    cb->endPass();

    // onscreen (quad)
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    vbufBinding.second = 0;
    cb->setVertexInput(0, 1, &vbufBinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(6);
    cb->endPass();
}
