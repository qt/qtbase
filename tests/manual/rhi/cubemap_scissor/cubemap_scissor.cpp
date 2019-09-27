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

// This is a test for scissoring. Based on the cubemap test (because there the
// rendering covers the entire viewport which is what we need here). The
// scissor rectangle moves first up, then down, then from the center to the
// left and then to right. The important part is to ensure that the behavior
// identical between all backends, especially when the rectangle is partly or
// fully off window.

#include "../shared/examplefw.h"
#include "../shared/cube.h"

struct {
    QVector<QRhiResource *> releasePool;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTexture *tex = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;

    QPoint scissorBottomLeft;
    QSize scissorSize;
    int scissorAnimState = 0;
    QSize outputSize;
} d;

void Window::customInit()
{
    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->build();
    d.releasePool << d.vbuf;

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
    d.ubuf->build();
    d.releasePool << d.ubuf;

    const QSize cubeMapSize(512, 512);
    d.tex = m_r->newTexture(QRhiTexture::RGBA8, cubeMapSize, 1, QRhiTexture::CubeMap);
    d.releasePool << d.tex;
    d.tex->build();

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

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::Repeat, QRhiSampler::Repeat);
    d.releasePool << d.sampler;
    d.sampler->build();

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.tex, d.sampler)
    });
    d.srb->build();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;

    d.ps->setFlags(QRhiGraphicsPipeline::UsesScissor);

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

    d.ps->build();

    d.scissorAnimState = 0;
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

static void advanceScissor()
{
    switch (d.scissorAnimState) {
    case 1: // up
        d.scissorBottomLeft.setX(d.outputSize.width() / 4);
        d.scissorBottomLeft.ry() += 1;
        if (d.scissorBottomLeft.y() > d.outputSize.height() + 100)
            d.scissorAnimState = 2;
        break;
    case 2: // down
        d.scissorBottomLeft.ry() -= 1;
        if (d.scissorBottomLeft.y() < -d.scissorSize.height() - 100)
            d.scissorAnimState = 3;
        break;
    case 3: // left
        d.scissorBottomLeft.setY(d.outputSize.height() / 4);
        d.scissorBottomLeft.rx() += 1;
        if (d.scissorBottomLeft.x() > d.outputSize.width() + 100)
            d.scissorAnimState = 4;
        break;
    case 4: // right
        d.scissorBottomLeft.rx() -= 1;
        if (d.scissorBottomLeft.x() < -d.scissorSize.width() - 100)
            d.scissorAnimState = 1;
        break;
    }

    qDebug() << "scissor bottom-left" << d.scissorBottomLeft << "size" << d.scissorSize;
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

    d.outputSize = outputSizeInPixels;
    if (d.scissorAnimState == 0) {
        d.scissorBottomLeft = QPoint(outputSizeInPixels.width() / 4, 0);
        d.scissorSize = QSize(outputSizeInPixels.width() / 2, outputSizeInPixels.height() / 2);
        d.scissorAnimState = 1;
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

    // Apply a scissor rectangle that moves around on the screen, also
    // exercising the out of screen (negative x or y) case.
    cb->setScissor(QRhiScissor(d.scissorBottomLeft.x(), d.scissorBottomLeft.y(),
                               d.scissorSize.width(), d.scissorSize.height()));

    cb->setShaderResources();

    const QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(36);
    cb->endPass();

    advanceScissor();
}
