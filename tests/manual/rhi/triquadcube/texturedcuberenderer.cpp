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

#include "texturedcuberenderer.h"
#include <QFile>
#include <QtGui/private/qshader_p.h>

#include "../shared/cube.h"

const bool MIPMAP = true;
const bool AUTOGENMIPMAP = true;

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

void TexturedCubeRenderer::initResources(QRhiRenderPassDescriptor *rp)
{
    m_vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    m_vbuf->setName(QByteArrayLiteral("Cube vbuf (textured)"));
    m_vbuf->build();
    m_vbufReady = false;

    m_ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4);
    m_ubuf->setName(QByteArrayLiteral("Cube ubuf (textured)"));
    m_ubuf->build();

    m_image = QImage(QLatin1String(":/qt256.png")).convertToFormat(QImage::Format_RGBA8888);
    QRhiTexture::Flags texFlags;
    if (MIPMAP)
        texFlags |= QRhiTexture::MipMapped;
    if (AUTOGENMIPMAP)
        texFlags |= QRhiTexture::UsedWithGenerateMips;
    m_tex = m_r->newTexture(QRhiTexture::RGBA8, QSize(m_image.width(), m_image.height()), 1, texFlags);
    m_tex->setName(QByteArrayLiteral("Qt texture"));
    m_tex->build();

    m_sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, MIPMAP ? QRhiSampler::Linear : QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    m_sampler->build();

    m_srb = m_r->newShaderResourceBindings();
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_tex, m_sampler)
    });
    m_srb->build();

    m_ps = m_r->newGraphicsPipeline();

    // No blending but the texture has alpha which we do not want to write out.
    // Be nice. (would not matter for an onscreen window but makes a difference
    // when reading back and saving into image files f.ex.)
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.colorWrite = QRhiGraphicsPipeline::R | QRhiGraphicsPipeline::G | QRhiGraphicsPipeline::B;
    m_ps->setTargetBlends({ blend });

    m_ps->setDepthTest(true);
    m_ps->setDepthWrite(true);
    m_ps->setDepthOp(QRhiGraphicsPipeline::Less);

    m_ps->setCullMode(QRhiGraphicsPipeline::Back);
    m_ps->setFrontFace(QRhiGraphicsPipeline::CCW);

    m_ps->setSampleCount(m_sampleCount);

    QShader vs = getShader(QLatin1String(":/texture.vert.qsb"));
    Q_ASSERT(vs.isValid());
    QShader fs = getShader(QLatin1String(":/texture.frag.qsb"));
    Q_ASSERT(fs.isValid());
    m_ps->setShaderStages({
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

    m_ps->setVertexInputLayout(inputLayout);
    m_ps->setShaderResourceBindings(m_srb);
    m_ps->setRenderPassDescriptor(rp);

    m_ps->build();
}

void TexturedCubeRenderer::resize(const QSize &pixelSize)
{
    m_proj = m_r->clipSpaceCorrMatrix();
    m_proj.perspective(45.0f, pixelSize.width() / (float) pixelSize.height(), 0.01f, 100.0f);
    m_proj.translate(0, 0, -4);
}

void TexturedCubeRenderer::releaseResources()
{
    delete m_ps;
    m_ps = nullptr;

    delete m_srb;
    m_srb = nullptr;

    delete m_sampler;
    m_sampler = nullptr;

    delete m_tex;
    m_tex = nullptr;

    delete m_ubuf;
    m_ubuf = nullptr;

    delete m_vbuf;
    m_vbuf = nullptr;
}

void TexturedCubeRenderer::queueResourceUpdates(QRhiResourceUpdateBatch *resourceUpdates)
{
    if (!m_vbufReady) {
        m_vbufReady = true;
        resourceUpdates->uploadStaticBuffer(m_vbuf, cube);
        qint32 flip = 0;
        resourceUpdates->updateDynamicBuffer(m_ubuf, 64, 4, &flip);
    }

    if (!m_image.isNull()) {
        if (MIPMAP) {
            QVarLengthArray<QRhiTextureUploadEntry, 16> descEntries;
            if (!AUTOGENMIPMAP) {
                // the ghetto mipmap generator...
                for (int i = 0, ie = m_r->mipLevelsForSize(m_image.size()); i != ie; ++i) {
                    QImage image = m_image.scaled(m_r->sizeForMipLevel(i, m_image.size()));
                    descEntries.append({ 0, i, image });
                }
            } else {
                descEntries.append({ 0, 0, m_image });
            }
            QRhiTextureUploadDescription desc;
            desc.setEntries(descEntries.cbegin(), descEntries.cend());
            resourceUpdates->uploadTexture(m_tex, desc);
            if (AUTOGENMIPMAP)
                resourceUpdates->generateMips(m_tex);
        } else {
            resourceUpdates->uploadTexture(m_tex, m_image);
        }
        m_image = QImage();
    }

    m_rotation += 1.0f;
    QMatrix4x4 mvp = m_proj;
    mvp.translate(m_translation);
    mvp.scale(0.5f);
    mvp.rotate(m_rotation, 0, 1, 0);
    resourceUpdates->updateDynamicBuffer(m_ubuf, 0, 64, mvp.constData());
}

void TexturedCubeRenderer::queueDraw(QRhiCommandBuffer *cb, const QSize &outputSizeInPixels)
{
    cb->setGraphicsPipeline(m_ps);
    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { m_vbuf, 0 },
        { m_vbuf, 36 * 3 * sizeof(float) }
    };
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(36);
}
