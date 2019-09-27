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
#include "../shared/cube.h"

// Depth texture / shadow sampler / shadow map example.
// Not available on GLES 2.0.

static float quadVertexData[] =
{ // Y up, CCW, x-y-z
  -0.5f,   0.5f, 0.0f,
  -0.5f,  -0.5f, 0.0f,
  0.5f,   -0.5f, 0.0f,
  0.5f,   0.5f,  0.0f,
};

static quint16 quadIndexData[] =
{
    0, 1, 2, 0, 2, 3
};

struct {
    QVector<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ibuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiSampler *shadowSampler = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QMatrix4x4 winProj;
    float cubeRot = 0;

    QRhiTextureRenderTarget *rt = nullptr;
    QRhiRenderPassDescriptor *rtRp = nullptr;
    QRhiTexture *shadowMap = nullptr;
    QRhiShaderResourceBindings *shadowSrb = nullptr;
    QRhiGraphicsPipeline *shadowPs = nullptr;
} d;

const int UBLOCK_SIZE = 64 * 3 + 4;
const int SHADOW_UBLOCK_SIZE = 64 * 1;
const int UBUF_SLOTS = 4; // 2 objects * 2 passes with different cameras

void Window::customInit()
{
    if (!m_r->isTextureFormatSupported(QRhiTexture::D32F))
        qFatal("Depth texture is not supported");

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVertexData) + sizeof(cube));
    d.vbuf->build();
    d.releasePool << d.vbuf;

    d.ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(quadIndexData));
    d.ibuf->build();
    d.releasePool << d.ibuf;

    const int oneRoundedUniformBlockSize = m_r->ubufAligned(UBLOCK_SIZE);
    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SLOTS * oneRoundedUniformBlockSize);
    d.ubuf->build();
    d.releasePool << d.ubuf;

    d.shadowMap = m_r->newTexture(QRhiTexture::D32F, QSize(1024, 1024), 1, QRhiTexture::RenderTarget);
    d.releasePool << d.shadowMap;
    d.shadowMap->build();

    d.shadowSampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.shadowSampler;
    d.shadowSampler->setTextureCompareOp(QRhiSampler::Less);
    d.shadowSampler->build();

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    const QRhiShaderResourceBinding::StageFlags stages = QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    d.srb->setBindings({ QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, stages, d.ubuf, UBLOCK_SIZE),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.shadowMap, d.shadowSampler) });
    d.srb->build();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;
    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/main.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/main.frag.qsb")) }
    });
    d.ps->setDepthTest(true);
    d.ps->setDepthWrite(true);
    // fits both the quad and cube vertex data
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->build();

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, 0, sizeof(quadVertexData), quadVertexData);
    d.initialUpdates->uploadStaticBuffer(d.vbuf, sizeof(quadVertexData), sizeof(cube), cube);
    d.initialUpdates->uploadStaticBuffer(d.ibuf, quadIndexData);

    QRhiTextureRenderTargetDescription rtDesc;
    rtDesc.setDepthTexture(d.shadowMap);
    d.rt = m_r->newTextureRenderTarget(rtDesc);
    d.releasePool << d.rt;
    d.rtRp = d.rt->newCompatibleRenderPassDescriptor();
    d.releasePool << d.rtRp;
    d.rt->setRenderPassDescriptor(d.rtRp);
    d.rt->build();

    d.shadowSrb = m_r->newShaderResourceBindings();
    d.releasePool << d.shadowSrb;
    d.shadowSrb->setBindings({ QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, stages, d.ubuf, SHADOW_UBLOCK_SIZE) });
    d.shadowSrb->build();

    d.shadowPs = m_r->newGraphicsPipeline();
    d.releasePool << d.shadowPs;
    d.shadowPs->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/shadowmap.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/shadowmap.frag.qsb")) }
    });
    d.shadowPs->setDepthTest(true);
    d.shadowPs->setDepthWrite(true);
    inputLayout.setBindings({
        { 3 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 }
    });
    d.shadowPs->setVertexInputLayout(inputLayout);
    d.shadowPs->setShaderResourceBindings(d.shadowSrb);
    d.shadowPs->setRenderPassDescriptor(d.rtRp);
    d.shadowPs->build();
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

static void enqueueScene(QRhiCommandBuffer *cb, QRhiShaderResourceBindings *srb, int oneRoundedUniformBlockSize, int firstUbufSlot)
{
    QRhiCommandBuffer::DynamicOffset ubufOffset(0, quint32(firstUbufSlot * oneRoundedUniformBlockSize));
    // draw the ground (the quad)
    cb->setShaderResources(srb, 1, &ubufOffset);
    QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding, d.ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(6);

    // Draw the object (the cube). Both vertex and uniform data are in the same
    // buffer, right after the quad's.
    ubufOffset.second += oneRoundedUniformBlockSize;
    cb->setShaderResources(srb, 1, &ubufOffset);
    vbufBinding.second = sizeof(quadVertexData);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(36);
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

    const int oneRoundedUniformBlockSize = m_r->ubufAligned(UBLOCK_SIZE);

    QMatrix4x4 shadowBias;
    // fill it in column-major order to keep our sanity (ctor would take row-major)
    float *sbp = shadowBias.data();
    if (m_r->isClipDepthZeroToOne()) {
        // convert x, y [-1, 1] -> [0, 1]
        *sbp++ = 0.5f; *sbp++ = 0.0f; *sbp++ = 0.0f; *sbp++ = 0.0f;
        *sbp++ = 0.0f; *sbp++ = m_r->isYUpInNDC() ? -0.5f : 0.5f; *sbp++ = 0.0f; *sbp++ = 0.0f;
        *sbp++ = 0.0f; *sbp++ = 0.0f; *sbp++ = 1.0f; *sbp++ = 0.0f;
        *sbp++ = 0.5f; *sbp++ = 0.5f; *sbp++ = 0.0f; *sbp++ = 1.0f;
    } else {
        // convert x, y, z [-1, 1] -> [0, 1]
        *sbp++ = 0.5f; *sbp++ = 0.0f; *sbp++ = 0.0f; *sbp++ = 0.0f;
        *sbp++ = 0.0f; *sbp++ = 0.5f; *sbp++ = 0.0f; *sbp++ = 0.0f;
        *sbp++ = 0.0f; *sbp++ = 0.0f; *sbp++ = 0.5f; *sbp++ = 0.0f;
        *sbp++ = 0.5f; *sbp++ = 0.5f; *sbp++ = 0.5f; *sbp++ = 1.0f;
    }

    const QVector3D lightPos(5, 10, 10);
    QMatrix4x4 lightViewProj = m_r->clipSpaceCorrMatrix();
    lightViewProj.perspective(45.0f, 1, 0.1f, 100.0f);
    lightViewProj.lookAt(lightPos, QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    // uniform data for the ground
    if (d.winProj != m_proj) {
        d.winProj = m_proj;

        QMatrix4x4 m;
        m.scale(4.0f);
        m.rotate(-60, 1, 0, 0);

        // for the main pass
        const QMatrix4x4 mvp = m_proj * m; // m_proj is in fact projection * view
        u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());
        const QMatrix4x4 shadowMvp = lightViewProj * m;
        u->updateDynamicBuffer(d.ubuf, 64, 64, shadowMvp.constData());
        u->updateDynamicBuffer(d.ubuf, 128, 64, shadowBias.constData());
        qint32 useShadows = 1;
        u->updateDynamicBuffer(d.ubuf, 192, 4, &useShadows);

        // for the shadow pass
        u->updateDynamicBuffer(d.ubuf, 2 * oneRoundedUniformBlockSize, 64, shadowMvp.constData());
    }

    // uniform data for the rotating cube
    QMatrix4x4 m;
    m.translate(0, 0.5f, 2);
    m.scale(0.2f);
    m.rotate(d.cubeRot, 0, 1, 0);
    m.rotate(45, 1, 0, 0);
    d.cubeRot += 1;

    // for the main pass
    const QMatrix4x4 mvp = m_proj * m;
    u->updateDynamicBuffer(d.ubuf, oneRoundedUniformBlockSize, 64, mvp.constData());
    const QMatrix4x4 shadowMvp = lightViewProj * m;
    u->updateDynamicBuffer(d.ubuf, oneRoundedUniformBlockSize + 64, 64, shadowMvp.constData());
    u->updateDynamicBuffer(d.ubuf, oneRoundedUniformBlockSize + 128, 64, shadowBias.constData());
    qint32 useShadows = 0;
    u->updateDynamicBuffer(d.ubuf, oneRoundedUniformBlockSize + 192, 4, &useShadows);

    // for the shadow pass
    u->updateDynamicBuffer(d.ubuf, 3 * oneRoundedUniformBlockSize, 64, shadowMvp.constData());

    cb->resourceUpdate(u);

    // shadow pass
    const QSize shadowMapSize = d.shadowMap->pixelSize();
    cb->beginPass(d.rt, QColor(), { 1.0f, 0 });
    cb->setGraphicsPipeline(d.shadowPs);
    cb->setViewport({ 0, 0, float(shadowMapSize.width()), float(shadowMapSize.height()) });
    enqueueScene(cb, d.shadowSrb, oneRoundedUniformBlockSize, 2);
    cb->endPass();

    // main pass
    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    enqueueScene(cb, d.srb, oneRoundedUniformBlockSize, 0);
    cb->endPass();
}
