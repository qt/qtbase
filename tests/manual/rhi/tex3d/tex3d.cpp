// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/examplefw.h"
#include <QImage>
#include <QPainter>

// Create a 3D texture of 256x256x3, where we will render to one of the slices
// in a render pass, while providing data to two other slices in texture
// uploads. Finally we use the texture to in the on-screen rendering: texture 3
// quads with the same volume texture, in each draw the shader will sample with
// a w that selects slice 0, 1, or 2, respectively.

const int TEXTURE_DEPTH = 3;

// Tests mipmap generation and linear-mipmap filtering when true. Note: Be
// aware what a mipmap chain for a 3D texture is and isn't, do not confuse with
// 2D (array) textures. The expected result on-screen is the black-red (slice
// 1) content mixed in to slice 0 and 2 on lower mip levels (resize the window
// to a smaller size -> the effect becomes more apparent, with totally taking
// over at small sizes)
// See https://docs.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-textures-intro
//
const bool MIPMAP = true;

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

struct {
    QList<QRhiResource *> releasePool;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QRhiTexture *tex = nullptr;
    QRhiTextureRenderTarget *rt = nullptr;
    QRhiRenderPassDescriptor *rtRp = nullptr;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QMatrix4x4 winProj;
    QVector3D slice1ClearColor;
} d;

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::ThreeDimensionalTextures))
        qWarning("3D textures are not supported");

    QRhiTexture::Flags texFlags = QRhiTexture::RenderTarget;
    if (MIPMAP)
        texFlags |= QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips;
    d.tex = m_r->newTexture(QRhiTexture::RGBA8,
                            256, 256, TEXTURE_DEPTH,
                            1, texFlags);
    d.releasePool << d.tex;
    d.tex->create();

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    QImage imgWhiteText(256, 256, QImage::Format_RGBA8888);
    imgWhiteText.fill(Qt::blue);
    QPainter p(&imgWhiteText);
    p.setPen(Qt::white);
    p.drawText(10, 10, "Slice 2:\nWhite text on blue background");
    p.end();

    // imgWhiteText -> slice 2 of the 3D texture
    QRhiTextureUploadEntry uploadSlice2(2, 0, QRhiTextureSubresourceUploadDescription(imgWhiteText));
    d.initialUpdates->uploadTexture(d.tex, uploadSlice2);

    QImage imgYellowText(256, 256, QImage::Format_RGBA8888);
    imgYellowText.fill(Qt::green);
    p.begin(&imgYellowText);
    p.setPen(Qt::yellow);
    p.drawText(10, 10, "Slice 0: Yellow text on green background");
    p.end();

    // imgYellowText -> slice 0 of the 3D texture
    QRhiTextureUploadEntry uploadSlice0(0, 0, QRhiTextureSubresourceUploadDescription(imgYellowText));
    d.initialUpdates->uploadTexture(d.tex, uploadSlice0);

    // slice 1 -> clear to some color in a render pass
    QRhiColorAttachment att(d.tex);
    att.setLayer(1);
    QRhiTextureRenderTargetDescription rtDesc(att);
    d.rt = m_r->newTextureRenderTarget(rtDesc);
    d.releasePool << d.rt;
    d.rtRp = d.rt->newCompatibleRenderPassDescriptor();
    d.releasePool << d.rtRp;
    d.rt->setRenderPassDescriptor(d.rtRp);
    d.rt->create();

    // set up resources needed to render a quad
    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVertexData));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(quadIndexData));
    d.ibuf->create();
    d.releasePool << d.ibuf;

    d.initialUpdates->uploadStaticBuffer(d.vbuf, 0, sizeof(quadVertexData), quadVertexData);
    d.initialUpdates->uploadStaticBuffer(d.ibuf, quadIndexData);

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, m_r->ubufAligned(72) * TEXTURE_DEPTH);
    d.ubuf->create();
    d.releasePool << d.ubuf;

    for (int i = 0; i < TEXTURE_DEPTH; ++i) {
        qint32 flip = 0;
        if (i == 1) // this slice will be rendered to in a render pass so needs flipping with OpenGL when on screen
            flip = m_r->isYUpInFramebuffer() ? 1 : 0;
        d.initialUpdates->updateDynamicBuffer(d.ubuf, m_r->ubufAligned(72) * i + 64, 4, &flip);
        qint32 idx = i;
        d.initialUpdates->updateDynamicBuffer(d.ubuf, m_r->ubufAligned(72) * i + 68, 4, &idx);
    }

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, MIPMAP ? QRhiSampler::Linear : QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->create();

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf, 72),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.tex, d.sampler)
    });
    d.srb->create();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;
    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/texture3d.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture3d.frag.qsb")) }
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
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    if (d.winProj != m_proj) {
        d.winProj = m_proj;
        for (int i = 0; i < TEXTURE_DEPTH; ++i) {
            QMatrix4x4 mvp = m_proj;
            switch (i) {
            case 0:
                mvp.translate(-2.0f, 0.0f, 0.0f);
                break;
            case 2:
                mvp.translate(2.0f, 0.0f, 0.0f);
                break;
            default:
                break;
            }
            u->updateDynamicBuffer(d.ubuf, m_r->ubufAligned(72) * i, 64, mvp.constData());
        }
    }

    const QColor slice1ClearColor = QColor::fromRgbF(d.slice1ClearColor.x(), d.slice1ClearColor.y(), d.slice1ClearColor.z());
    d.slice1ClearColor.setX(d.slice1ClearColor.x() + 0.01f);
    if (d.slice1ClearColor.x() > 1.0f)
        d.slice1ClearColor.setX(0.0f);
    cb->beginPass(d.rt, slice1ClearColor, { 1.0f, 0 }, u);
    cb->endPass();

    // now all 3 slices have something, regenerate the mipmaps
    u = m_r->nextResourceUpdateBatch();
    if (MIPMAP)
        u->generateMips(d.tex);

    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    QRhiCommandBuffer::DynamicOffset dynOfs = { 0, 0 };
    cb->setShaderResources(d.srb, 1, &dynOfs);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(6);

    dynOfs.second = m_r->ubufAligned(72);
    cb->setShaderResources(d.srb, 1, &dynOfs);
    cb->drawIndexed(6);

    dynOfs.second = m_r->ubufAligned(72) * 2;
    cb->setShaderResources(d.srb, 1, &dynOfs);
    cb->drawIndexed(6);

    cb->endPass();
}
