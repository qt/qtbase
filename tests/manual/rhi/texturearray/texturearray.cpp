// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/examplefw.h"
#include <QElapsedTimer>

// Creates a texture array object with size 4, uploads a different
// image to each, and cycles through them on-screen.

static const int ARRAY_SIZE = 4;
static const int UBUF_SIZE = 72;

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

struct {
    QList<QRhiResource *> releasePool;
    QRhiTexture *texArr = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QMatrix4x4 winProj;
    QElapsedTimer t;
    int arrayIndex = 0;
} d;

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::TextureArrays))
        qFatal("Texture array objects are not supported by this backend");

    d.texArr = m_r->newTextureArray(QRhiTexture::RGBA8, ARRAY_SIZE, QSize(512, 512), 1,
        // mipmaps will be generated, to exercise that too
        QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips);
    d.releasePool << d.texArr;
    d.texArr->create();

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    QImage img(512, 512, QImage::Format_RGBA8888);
    img.fill(Qt::red);
    d.initialUpdates->uploadTexture(d.texArr, QRhiTextureUploadDescription(QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(img))));
    img.fill(Qt::green);
    d.initialUpdates->uploadTexture(d.texArr, QRhiTextureUploadDescription(QRhiTextureUploadEntry(1, 0, QRhiTextureSubresourceUploadDescription(img))));
    img.fill(Qt::blue);
    d.initialUpdates->uploadTexture(d.texArr, QRhiTextureUploadDescription(QRhiTextureUploadEntry(2, 0, QRhiTextureSubresourceUploadDescription(img))));
    img.fill(Qt::yellow);
    d.initialUpdates->uploadTexture(d.texArr, QRhiTextureUploadDescription(QRhiTextureUploadEntry(3, 0, QRhiTextureSubresourceUploadDescription(img))));

    d.initialUpdates->generateMips(d.texArr);

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->create();

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE);
    d.releasePool << d.ubuf;
    d.ubuf->create();

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf, 0, UBUF_SIZE),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.texArr, d.sampler)
    });
    d.srb->create();

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
    d.releasePool << d.vbuf;
    d.vbuf->create();

    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indexData));
    d.releasePool << d.ibuf;
    d.ibuf->create();

    d.initialUpdates->uploadStaticBuffer(d.vbuf, 0, sizeof(vertexData), vertexData);
    d.initialUpdates->uploadStaticBuffer(d.ibuf, indexData);

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;
    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/texture_arr.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture_arr.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, quint32(2 * sizeof(float)) }
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();

    d.t.start();
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

    if (d.winProj != m_proj) {
        d.winProj = m_proj;
        QMatrix4x4 mvp = m_proj;
        mvp.scale(2);
        u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());
        const qint32 flip = 0;
        u->updateDynamicBuffer(d.ubuf, 64, 4, &flip);
        u->updateDynamicBuffer(d.ubuf, 68, 4, &d.arrayIndex);
    }

    if (d.t.elapsed() > 2000) {
        d.t.restart();
        d.arrayIndex = (d.arrayIndex + 1) % ARRAY_SIZE;
        u->updateDynamicBuffer(d.ubuf, 68, 4, &d.arrayIndex);
    }

    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(6);
    cb->endPass();
}
