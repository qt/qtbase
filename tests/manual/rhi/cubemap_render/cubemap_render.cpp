/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

// Demonstrates rendering to two cubemaps in two different ways:
//   - one by one, to each face,
//   - if the supported max number of color attachments is greater than 4: in
//     one go with all 6 faces attached as render targets.
//
// Finally, show what we got in a skybox-ish thing. Press the arrow keys to
// switch between the two cubemaps. (the only difference should be their
// background clear color)

#define EXAMPLEFW_KEYPRESS_EVENTS
#include "../shared/examplefw.h"
#include "../shared/cube.h"

// each face is 512x512
static const QSize cubemapSize(512, 512);

// each cubemap face gets a 256x256 quad in the center
static float halfQuadVertexData[] =
{ // Y up, CCW
  -0.5f,    0.5f,
  -0.5f,   -0.5f,
   0.5f,   -0.5f,
   0.5f,    0.5f,
};

static quint16 halfQuadIndexData[] =
{
    0, 1, 2, 0, 2, 3
};

struct {
    QVector<QRhiResource *> releasePool;

    QRhiTexture *cubemap1 = nullptr;
    QRhiTexture *cubemap2 = nullptr;
    bool canDoMrt = false;

    QRhiBuffer *half_quad_vbuf = nullptr;
    QRhiBuffer *half_quad_ibuf = nullptr;

    QRhiBuffer *oneface_ubuf = nullptr;
    int ubufSizePerFace;
    QRhiTextureRenderTarget *oneface_rt[6];
    QRhiRenderPassDescriptor *oneface_rp = nullptr;
    QRhiShaderResourceBindings *oneface_srb = nullptr;
    QRhiGraphicsPipeline *oneface_ps = nullptr;

    QRhiBuffer *mrt_ubuf = nullptr;
    QRhiTextureRenderTarget *mrt_rt = nullptr;
    QRhiRenderPassDescriptor *mrt_rp = nullptr;
    QRhiShaderResourceBindings *mrt_srb = nullptr;
    QRhiGraphicsPipeline *mrt_ps = nullptr;

    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QMatrix4x4 winProj;
    float rx = 0;
} d;

void initializePerFaceRendering(QRhi *rhi)
{
    d.cubemap1 = rhi->newTexture(QRhiTexture::RGBA8, cubemapSize, 1, QRhiTexture::CubeMap | QRhiTexture::RenderTarget);
    d.cubemap1->build();
    d.releasePool << d.cubemap1;

    d.ubufSizePerFace = rhi->ubufAligned(64 + 12);
    d.oneface_ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, d.ubufSizePerFace * 6);
    d.oneface_ubuf->build();
    d.releasePool << d.oneface_ubuf;

    for (int face = 0; face < 6; ++face) {
        QRhiColorAttachment att(d.cubemap1);
        att.setLayer(face);
        QRhiTextureRenderTargetDescription rtDesc(att);
        d.oneface_rt[face] = rhi->newTextureRenderTarget(rtDesc);
        if (face == 0) {
            d.oneface_rp = d.oneface_rt[0]->newCompatibleRenderPassDescriptor();
            d.releasePool << d.oneface_rp;
        }
        d.oneface_rt[face]->setRenderPassDescriptor(d.oneface_rp);
        d.oneface_rt[face]->build();
        d.releasePool << d.oneface_rt[face];
    }

    d.oneface_srb = rhi->newShaderResourceBindings();
    const QRhiShaderResourceBinding::StageFlags visibility =
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    d.oneface_srb->setBindings({
        QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, visibility, d.oneface_ubuf, 64 + 12)
    });
    d.oneface_srb->build();
    d.releasePool << d.oneface_srb;

    d.oneface_ps = rhi->newGraphicsPipeline();
    d.oneface_ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/cubemap_oneface.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/cubemap_oneface.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
    });
    d.oneface_ps->setVertexInputLayout(inputLayout);
    d.oneface_ps->setShaderResourceBindings(d.oneface_srb);
    d.oneface_ps->setRenderPassDescriptor(d.oneface_rp);
    d.oneface_ps->build();
    d.releasePool << d.oneface_ps;

    // wasteful to duplicate the mvp as well but will do for now
    for (int face = 0; face < 6; ++face) {
        const int offset = d.ubufSizePerFace * face;
        QMatrix4x4 identity;
        d.initialUpdates->updateDynamicBuffer(d.oneface_ubuf, offset, 64, identity.constData());
        // will use a different color for each face
        QColor c;
        switch (face) {
        case 0:
            c = Qt::red;
            break;
        case 1:
            c = Qt::green;
            break;
        case 2:
            c = Qt::blue;
            break;
        case 3:
            c = Qt::yellow;
            break;
        case 4:
            c = Qt::lightGray;
            break;
        case 5:
            c = Qt::cyan;
            break;
        }
        float color[] = { float(c.redF()), float(c.greenF()), float(c.blueF()) };
        d.initialUpdates->updateDynamicBuffer(d.oneface_ubuf, offset + 64, 12, color);
    }
}

// 6 render passes, 1 draw call each, targeting one cubemap face at a time
void renderPerFace(QRhiCommandBuffer *cb)
{
    for (int face = 0; face < 6; ++face) {
        cb->beginPass(d.oneface_rt[face], Qt::black, { 1.0f, 0 });
        cb->setGraphicsPipeline(d.oneface_ps);
        cb->setViewport({ 0, 0,
                          float(d.oneface_rt[face]->pixelSize().width()),
                          float(d.oneface_rt[face]->pixelSize().height()) });
        const QRhiCommandBuffer::DynamicOffset dynamicOffset(0, face * d.ubufSizePerFace);
        cb->setShaderResources(nullptr, 1, &dynamicOffset);
        QRhiCommandBuffer::VertexInput vbufBinding(d.half_quad_vbuf, 0);
        cb->setVertexInput(0, 1, &vbufBinding, d.half_quad_ibuf, 0, QRhiCommandBuffer::IndexUInt16);
        cb->drawIndexed(6);
        cb->endPass();
    }
}

void initializeMrtRendering(QRhi *rhi)
{
    d.cubemap2 = rhi->newTexture(QRhiTexture::RGBA8, cubemapSize, 1, QRhiTexture::CubeMap | QRhiTexture::RenderTarget);
    d.cubemap2->build();
    d.releasePool << d.cubemap2;

    d.mrt_ubuf = rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 6 * 16); // note that vec3 is aligned to 16 bytes
    d.mrt_ubuf->build();
    d.releasePool << d.mrt_ubuf;

    QVarLengthArray<QRhiColorAttachment, 6> attachments;
    for (int face = 0; face < 6; ++face) {
        QRhiColorAttachment att(d.cubemap2);
        att.setLayer(face);
        attachments.append(att);
    }
    QRhiTextureRenderTargetDescription rtDesc;
    rtDesc.setColorAttachments(attachments.cbegin(), attachments.cend());
    d.mrt_rt = rhi->newTextureRenderTarget(rtDesc);
    d.mrt_rp = d.mrt_rt->newCompatibleRenderPassDescriptor();
    d.releasePool << d.mrt_rp;
    d.mrt_rt->setRenderPassDescriptor(d.mrt_rp);
    d.mrt_rt->build();
    d.releasePool << d.mrt_rt;

    d.mrt_srb = rhi->newShaderResourceBindings();
    const QRhiShaderResourceBinding::StageFlags visibility =
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    d.mrt_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, visibility, d.mrt_ubuf)
    });
    d.mrt_srb->build();
    d.releasePool << d.mrt_srb;

    d.mrt_ps = rhi->newGraphicsPipeline();
    d.mrt_ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/cubemap_mrt.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/cubemap_mrt.frag.qsb")) }
    });
    QVarLengthArray<QRhiGraphicsPipeline::TargetBlend, 6> targetBlends;
    for (int face = 0; face < 6; ++face)
        targetBlends.append({}); // default to blend = false, color write = all, which is good
    d.mrt_ps->setTargetBlends(targetBlends.cbegin(), targetBlends.cend());
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
    });
    d.mrt_ps->setVertexInputLayout(inputLayout);
    d.mrt_ps->setShaderResourceBindings(d.mrt_srb);
    d.mrt_ps->setRenderPassDescriptor(d.mrt_rp);
    d.mrt_ps->build();
    d.releasePool << d.mrt_ps;

    QMatrix4x4 identity;
    d.initialUpdates->updateDynamicBuffer(d.mrt_ubuf, 0, 64, identity.constData());
    for (int face = 0; face < 6; ++face) {
        const int offset = 64 + face * 16;
        // will use a different color for each face
        QColor c;
        switch (face) {
        case 0:
            c = Qt::red;
            break;
        case 1:
            c = Qt::green;
            break;
        case 2:
            c = Qt::blue;
            break;
        case 3:
            c = Qt::yellow;
            break;
        case 4:
            c = Qt::lightGray;
            break;
        case 5:
            c = Qt::cyan;
            break;
        }
        float color[] = { float(c.redF()), float(c.greenF()), float(c.blueF()) };
        d.initialUpdates->updateDynamicBuffer(d.mrt_ubuf, offset, 12, color);
    }
}

// 1 render pass, 1 draw call, with all 6 faces attached and written to
void renderWithMrt(QRhiCommandBuffer *cb)
{
    // use a different clear color to differentiate from cubemap1 (because the
    // results are expected to be identical otherwise)
    cb->beginPass(d.mrt_rt, Qt::magenta, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.mrt_ps);
    cb->setViewport({ 0, 0,
                      float(d.mrt_rt->pixelSize().width()),
                      float(d.mrt_rt->pixelSize().height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(d.half_quad_vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding, d.half_quad_ibuf, 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(6);
    cb->endPass();
}

void Window::customInit()
{
    d.half_quad_vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(halfQuadVertexData));
    d.half_quad_vbuf->build();
    d.releasePool << d.half_quad_vbuf;

    d.half_quad_ibuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(halfQuadIndexData));
    d.half_quad_ibuf->build();
    d.releasePool << d.half_quad_ibuf;

    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.half_quad_vbuf, 0, sizeof(halfQuadVertexData), halfQuadVertexData);
    d.initialUpdates->uploadStaticBuffer(d.half_quad_ibuf, halfQuadIndexData);

    initializePerFaceRendering(m_r);

    d.canDoMrt = m_r->resourceLimit(QRhi::MaxColorAttachments) >= 6;
    if (d.canDoMrt)
        initializeMrtRendering(m_r);
    else
        qWarning("Not enough color attachments (need 6, supports %d)", m_r->resourceLimit(QRhi::MaxColorAttachments));


    // onscreen stuff
    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->build();
    d.releasePool << d.vbuf;
    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
    d.ubuf->build();
    d.releasePool << d.ubuf;

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::Repeat, QRhiSampler::Repeat);
    d.sampler->build();
    d.releasePool << d.sampler;

    d.srb = m_r->newShaderResourceBindings();
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.cubemap1, d.sampler)
    });
    d.srb->build();
    d.releasePool << d.srb;

    d.ps = m_r->newGraphicsPipeline();
    d.ps->setDepthTest(true);
    d.ps->setDepthWrite(true);
    d.ps->setDepthOp(QRhiGraphicsPipeline::LessOrEqual);
    d.ps->setCullMode(QRhiGraphicsPipeline::Front); // we are inside the cube so cull front, not back
    d.ps->setFrontFace(QRhiGraphicsPipeline::CCW); // front is ccw in the cube data
    QShader vs = getShader(QLatin1String(":/cubemap_sample.vert.qsb"));
    Q_ASSERT(vs.isValid());
    QShader fs = getShader(QLatin1String(":/cubemap_sample.frag.qsb"));
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
    d.ps->build();
    d.releasePool << d.ps;

    if (d.canDoMrt)
        qDebug("Use the arrow keys to switch between the two generated cubemaps");
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
    mvp.scale(10);
    mvp.rotate(d.rx, 1, 0, 0);
    d.rx += 0.5f;
    u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());

    cb->resourceUpdate(u);

    renderPerFace(cb);

    if (d.canDoMrt)
        renderWithMrt(cb);

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 });
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(36);
    cb->endPass();
}

void Window::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Left:
    case Qt::Key_Up:
        qDebug("Showing first cubemap (generated by rendering to the faces one by one; black background)");
        d.srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.cubemap1, d.sampler)
        });
        d.srb->build();
        break;
    case Qt::Key_Right:
    case Qt::Key_Down:
        if (d.canDoMrt) {
            qDebug("Showing second cubemap (generated with multiple render targets; magenta background)");
            d.srb->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.cubemap2, d.sampler)
            });
            d.srb->build();
        }
        break;
    default:
        e->ignore();
        break;
    }
}
