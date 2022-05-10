// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/examplefw.h"
#include "../shared/cube.h"

struct {
    QList<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTexture *tex = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
} d;

void Window::customInit()
{
    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
    d.ubuf->create();
    d.releasePool << d.ubuf;

    const QSize cubeMapSize(512, 512);
    d.tex = m_r->newTexture(QRhiTexture::RGBA8, cubeMapSize, 1, QRhiTexture::CubeMap
                            | QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips); // exercise mipmap generation as well
    d.releasePool << d.tex;
    d.tex->create();

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

    QImage img = QImage(":/c.png").mirrored().convertToFormat(QImage::Format_RGBA8888);
    // just use the same image for all faces for now
    QRhiTextureSubresourceUploadDescription subresDesc(img);
    QRhiTextureUploadDescription desc({
                                          { 0, 0, subresDesc },  // +X
                                          { 1, 0, subresDesc },  // -X
                                          { 2, 0, subresDesc },  // +Y
                                          { 3, 0, subresDesc },  // -Y
                                          { 4, 0, subresDesc },  // +Z
                                          { 5, 0, subresDesc }   // -Z
                                      });
    d.initialUpdates->uploadTexture(d.tex, desc);

    d.initialUpdates->generateMips(d.tex);

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::Repeat, QRhiSampler::Repeat);
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

    d.ps->setDepthTest(true);
    d.ps->setDepthWrite(true);
    d.ps->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);

    d.ps->setCullMode(QRhiGraphicsPipeline::Front); // we are inside the cube so cull front, not back
    d.ps->setFrontFace(QRhiGraphicsPipeline::CCW); // front is ccw in the cube data

    QShader vs = getShader(QLatin1String(":/cubemap.vert.qsb"));
    Q_ASSERT(vs.isValid());
    QShader fs = getShader(QLatin1String(":/cubemap.frag.qsb"));
    Q_ASSERT(fs.isValid());
    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 }
    });

    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);

    d.ps->create();
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

    QMatrix4x4 mvp = m_r->clipSpaceCorrMatrix();
    mvp.perspective(90.0f, outputSizeInPixels.width() / (float) outputSizeInPixels.height(), 0.01f, 1000.0f);
    // cube vertices go from -1..1
    mvp.scale(10);
    static float rx = 0;
    mvp.rotate(rx, 1, 0, 0);
    rx += 0.5f;
    // no translation
    u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(36);
    cb->endPass();
}
