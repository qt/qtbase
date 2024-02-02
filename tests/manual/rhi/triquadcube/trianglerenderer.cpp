// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "trianglerenderer.h"
#include <QFile>
#include <rhi/qshader.h>

//#define VBUF_IS_DYNAMIC

static float vertexData[] = { // Y up (note m_proj), CCW
     0.0f,   0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
    -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f
};

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

void TriangleRenderer::initResources(QRhiRenderPassDescriptor *rp)
{
#ifdef VBUF_IS_DYNAMIC
    m_vbuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, sizeof(vertexData));
#else
    m_vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
#endif
    m_vbuf->setName(QByteArrayLiteral("Triangle vbuf"));
    m_vbuf->create();
    m_vbufReady = false;

    m_ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    m_ubuf->setName(QByteArrayLiteral("Triangle ubuf"));
    m_ubuf->create();

    m_srb = m_r->newShaderResourceBindings();
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_ubuf)
    });
    m_srb->create();

    m_ps = m_r->newGraphicsPipeline();

    QRhiGraphicsPipeline::TargetBlend premulAlphaBlend; // convenient defaults...
    premulAlphaBlend.enable = true;
    QVarLengthArray<QRhiGraphicsPipeline::TargetBlend, 4> rtblends;
    for (int i = 0; i < m_colorAttCount; ++i)
        rtblends << premulAlphaBlend;

    m_ps->setTargetBlends(rtblends.cbegin(), rtblends.cend());
    m_ps->setSampleCount(m_sampleCount);

    if (m_depthWrite) { // TriangleOnCube may want to exercise this
        m_ps->setDepthTest(true);
        m_ps->setDepthOp(QRhiGraphicsPipeline::Always);
        m_ps->setDepthWrite(true);
    }

    QShader vs = getShader(QLatin1String(":/color.vert.qsb"));
    Q_ASSERT(vs.isValid());
    QShader fs = getShader(QLatin1String(":/color.frag.qsb"));
    Q_ASSERT(fs.isValid());
    m_ps->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 7 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float3, 2 * sizeof(float) }
    });

    m_ps->setVertexInputLayout(inputLayout);
    m_ps->setShaderResourceBindings(m_srb);
    m_ps->setRenderPassDescriptor(rp);

    m_ps->create();
}

void TriangleRenderer::resize(const QSize &pixelSize)
{
    m_proj = m_r->clipSpaceCorrMatrix();
    m_proj.perspective(45.0f, pixelSize.width() / (float) pixelSize.height(), 0.01f, 100.0f);
    m_proj.translate(0, 0, -4);
}

void TriangleRenderer::releaseResources()
{
    delete m_ps;
    m_ps = nullptr;

    delete m_srb;
    m_srb = nullptr;

    delete m_ubuf;
    m_ubuf = nullptr;

    delete m_vbuf;
    m_vbuf = nullptr;
}

void TriangleRenderer::queueResourceUpdates(QRhiResourceUpdateBatch *resourceUpdates)
{
#if 0
    static int messWithBufferTrigger = 0;
    // recreate the underlying VkBuffer every second frame
    // to exercise setShaderResources' built-in smartness
    if (!(messWithBufferTrigger & 1)) {
        m_ubuf->release();
        m_ubuf->create();
    }
    ++messWithBufferTrigger;
#endif

    if (!m_vbufReady) {
        m_vbufReady = true;
#ifdef VBUF_IS_DYNAMIC
        resourceUpdates->updateDynamicBuffer(m_vbuf, 0, m_vbuf->size(), vertexData);
#else
        resourceUpdates->uploadStaticBuffer(m_vbuf, vertexData);
#endif
    }

    m_rotation += 1.0f;
    QMatrix4x4 mvp = m_proj;
    mvp.translate(m_translation);
    mvp.scale(m_scale);
    mvp.rotate(m_rotation, 0, 1, 0);
    resourceUpdates->updateDynamicBuffer(m_ubuf, 0, 64, mvp.constData());

    m_opacity += m_opacityDir * 0.005f;
    if (m_opacity < 0.0f || m_opacity > 1.0f) {
        m_opacityDir *= -1;
        m_opacity = qBound(0.0f, m_opacity, 1.0f);
    }
    resourceUpdates->updateDynamicBuffer(m_ubuf, 64, 4, &m_opacity);
}

void TriangleRenderer::queueDraw(QRhiCommandBuffer *cb, const QSize &outputSizeInPixels)
{
    cb->setGraphicsPipeline(m_ps);
    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);
}
