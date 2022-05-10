// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/examplefw.h"
#include "../shared/cube.h"
#include "../shared/dds_bc1.h"

struct {
    QRhiBuffer *vbuf = nullptr;
    bool vbufReady = false;
    QRhiBuffer *ubuf = nullptr;
    QRhiTexture *tex = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;

    float rotation = 0;

    QByteArrayList compressedData;
} d;

void Window::customInit()
{
    if (!m_r->isTextureFormatSupported(QRhiTexture::BC1))
        qFatal("This backend does not support BC1");

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.vbufReady = false;

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    d.ubuf->create();

    QSize imageSize;
    d.compressedData = loadBC1(QLatin1String(":/qt256_bc1_9mips.dds"), &imageSize);
    qDebug() << d.compressedData.count() << imageSize << m_r->mipLevelsForSize(imageSize);

    d.tex = m_r->newTexture(QRhiTexture::BC1, imageSize, 1, QRhiTexture::MipMapped);
    d.tex->create();

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.sampler->create();

    d.srb = m_r->newShaderResourceBindings();
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.tex, d.sampler)
    });
    d.srb->create();

    d.ps = m_r->newGraphicsPipeline();

    d.ps->setDepthTest(true);
    d.ps->setDepthWrite(true);
    d.ps->setDepthOp(QRhiGraphicsPipeline::Less);

    d.ps->setCullMode(QRhiGraphicsPipeline::Back);
    d.ps->setFrontFace(QRhiGraphicsPipeline::CCW);

    const QShader vs = getShader(QLatin1String(":/texture.vert.qsb"));
    if (!vs.isValid())
        qFatal("Failed to load shader pack (vertex)");
    const QShader fs = getShader(QLatin1String(":/texture.frag.qsb"));
    if (!fs.isValid())
        qFatal("Failed to load shader pack (fragment)");

    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },
        { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 }
    });

    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);

    d.ps->create();
}

void Window::customRelease()
{
    delete d.ps;
    d.ps = nullptr;

    delete d.srb;
    d.srb = nullptr;

    delete d.ubuf;
    d.ubuf = nullptr;

    delete d.vbuf;
    d.vbuf = nullptr;

    delete d.sampler;
    d.sampler = nullptr;

    delete d.tex;
    d.tex = nullptr;
}

void Window::customRender()
{
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (!d.vbufReady) {
        d.vbufReady = true;
        u->uploadStaticBuffer(d.vbuf, cube);
        qint32 flip = 0;
        u->updateDynamicBuffer(d.ubuf, 64, 4, &flip);
    }
    if (!d.compressedData.isEmpty()) {
        QVarLengthArray<QRhiTextureUploadEntry, 16> descEntries;
        for (int i = 0; i < d.compressedData.count(); ++i) {
            QRhiTextureSubresourceUploadDescription image(d.compressedData[i].constData(), d.compressedData[i].size());
            descEntries.append({ 0, i, image });
        }
        QRhiTextureUploadDescription desc;
        desc.setEntries(descEntries.cbegin(), descEntries.cend());
        u->uploadTexture(d.tex, desc);
        d.compressedData.clear();
    }
    d.rotation += 1.0f;
    QMatrix4x4 mvp = m_proj;
    mvp.scale(0.5f);
    mvp.rotate(d.rotation, 0, 1, 0);
    u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());

    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    const QSize outputSizeInPixels = m_sc->currentPixelSize();

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);

    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { d.vbuf, 0 },
        { d.vbuf, quint32(36 * 3 * sizeof(float)) }
    };
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(36);

    cb->endPass();
}
