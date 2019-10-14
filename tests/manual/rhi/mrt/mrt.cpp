/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "../shared/examplefw.h"

// Exercises MRT, by attaching 4 textures to a single QRhiTextureRenderTarget.
// The fragment shader outputs to all four targets.
// The textures are then used to render 4 quads, so their contents can be confirmed.

const int ATTCOUNT = 4;

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
    QVector<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTextureRenderTarget *rt = nullptr;
    QRhiRenderPassDescriptor *rtRp = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QMatrix4x4 winProj;
    struct PerColorBuffer {
        QRhiTexture *tex = nullptr;
        QRhiShaderResourceBindings *srb = nullptr;
    };
    PerColorBuffer colData[ATTCOUNT];

    QRhiBuffer *triUbuf = nullptr;
    QRhiShaderResourceBindings *triSrb = nullptr;
    QRhiGraphicsPipeline *triPs = nullptr;
    float triRot = 0;
    QMatrix4x4 triBaseMvp;
} d;

void Window::customInit()
{
    qDebug("Max color attachments: %d", m_r->resourceLimit(QRhi::MaxColorAttachments));
    if (m_r->resourceLimit(QRhi::MaxColorAttachments) < 4)
        qWarning("MRT is not supported");

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVertexData) + sizeof(triangleData));
    d.vbuf->build();
    d.releasePool << d.vbuf;

    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(quadIndexData));
    d.ibuf->build();
    d.releasePool << d.ibuf;

    const int oneRoundedUniformBlockSize = m_r->ubufAligned(68);
    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, oneRoundedUniformBlockSize * ATTCOUNT);
    d.ubuf->build();
    d.releasePool << d.ubuf;

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->build();

    for (int i = 0; i < ATTCOUNT; ++i) {
        QRhiTexture *tex = m_r->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget);
        d.releasePool << tex;
        tex->build();

        QRhiShaderResourceBindings *srb = m_r->newShaderResourceBindings();
        d.releasePool << srb;
        srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                             d.ubuf, i * oneRoundedUniformBlockSize, 68),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, tex, d.sampler)
        });
        srb->build();

        d.colData[i].tex = tex;
        d.colData[i].srb = srb;
    }

    QRhiTextureRenderTargetDescription rtDesc;
    QRhiColorAttachment att[ATTCOUNT];
    for (int i = 0; i < ATTCOUNT; ++i)
        att[i] = QRhiColorAttachment(d.colData[i].tex);
    rtDesc.setColorAttachments(att, att + ATTCOUNT);
    d.rt = m_r->newTextureRenderTarget(rtDesc);
    d.releasePool << d.rt;
    d.rtRp = d.rt->newCompatibleRenderPassDescriptor();
    d.releasePool << d.rtRp;
    d.rt->setRenderPassDescriptor(d.rtRp);
    d.rt->build();

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
        { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.colData[0].srb); // all of them are layout-compatible
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->build();

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, 0, sizeof(quadVertexData), quadVertexData);
    d.initialUpdates->uploadStaticBuffer(d.vbuf, sizeof(quadVertexData), sizeof(triangleData), triangleData);
    d.initialUpdates->uploadStaticBuffer(d.ibuf, quadIndexData);

    qint32 flip = m_r->isYUpInFramebuffer() ? 1 : 0;
    for (int i = 0; i < ATTCOUNT; ++i)
        d.initialUpdates->updateDynamicBuffer(d.ubuf, i * oneRoundedUniformBlockSize + 64, 4, &flip);

    // triangle
    d.triUbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    d.releasePool << d.triUbuf;
    d.triUbuf->build();

    d.triSrb = m_r->newShaderResourceBindings();
    d.releasePool << d.triSrb;
    d.triSrb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.triUbuf)
    });
    d.triSrb->build();

    d.triPs = m_r->newGraphicsPipeline();
    d.releasePool << d.triPs;
    d.triPs->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/mrt.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/mrt.frag.qsb")) }
    });

    QRhiGraphicsPipeline::TargetBlend blends[ATTCOUNT]; // defaults to blending == false
    d.triPs->setTargetBlends(blends, blends + ATTCOUNT);

    inputLayout.setBindings({
        { 5 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float3, 2 * sizeof(float) }
    });
    d.triPs->setVertexInputLayout(inputLayout);
    d.triPs->setShaderResourceBindings(d.triSrb);
    d.triPs->setRenderPassDescriptor(d.rtRp);
    d.triPs->build();

    d.triBaseMvp = m_r->clipSpaceCorrMatrix();
    d.triBaseMvp.perspective(45.0f, d.rt->pixelSize().width() / float(d.rt->pixelSize().height()), 0.01f, 1000.0f);
    d.triBaseMvp.translate(0, 0, -2);
    float opacity = 1.0f;
    d.initialUpdates->updateDynamicBuffer(d.triUbuf, 64, 4, &opacity);
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

    cb->beginPass(d.rt, QColor::fromRgbF(0.5f, 0.2f, 0.0f, 1.0f), { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.triPs);
    cb->setViewport({ 0, 0, float(d.rt->pixelSize().width()), float(d.rt->pixelSize().height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, sizeof(quadVertexData));
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);
    cb->endPass();

    u = m_r->nextResourceUpdateBatch();
    if (d.winProj != m_proj) {
        d.winProj = m_proj;
        const int oneRoundedUniformBlockSize = m_r->ubufAligned(68);
        for (int i = 0; i < ATTCOUNT; ++i) {
            QMatrix4x4 mvp = m_proj;
            switch (i) {
            case 0:
                mvp.translate(-2.0f, 0, 0);
                break;
            case 1:
                mvp.translate(-0.8f, 0, 0);
                break;
            case 2:
                mvp.translate(0.4f, 0, 0);
                break;
            case 3:
                mvp.translate(1.6f, 0, 0);
                break;
            default:
                Q_UNREACHABLE();
                break;
            }
            u->updateDynamicBuffer(d.ubuf, i * oneRoundedUniformBlockSize, 64, mvp.constData());
        }
    }

    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    vbufBinding.second = 0;
    cb->setVertexInput(0, 1, &vbufBinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    for (int i = 0; i < ATTCOUNT; ++i) {
        cb->setShaderResources(d.colData[i].srb);
        cb->drawIndexed(6);
    }
    cb->endPass();
}
