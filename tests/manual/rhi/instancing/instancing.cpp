// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/examplefw.h"
#include "../shared/cube.h"
#include <QRandomGenerator>

// Instanced draw example. When running with OpenGL, at least 3.3 or ES 3.0 is
// needed.

const int INSTANCE_COUNT = 1024;

struct {
    QList<QRhiResource *> releasePool;

    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *instBuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QMatrix4x4 winProj;
} d;

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::Instancing))
        qFatal("Instanced drawing is not supported");

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

    // transform + color (mat4 + vec3), interleaved, for each instance
    d.instBuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, INSTANCE_COUNT * 19 * sizeof(float));
    d.instBuf->create();
    d.releasePool << d.instBuf;

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 12);
    d.ubuf->create();
    d.releasePool << d.ubuf;

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.ubuf)
    });
    d.srb->create();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;
    d.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/inst.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/inst.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },                                        // cube vertices
        { 19 * sizeof(float), QRhiVertexInputBinding::PerInstance },  // per-instance transform and color
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },                     // position
        { 1, 1, QRhiVertexInputAttribute::Float4, 0, 0 },                  // instMat
        { 1, 2, QRhiVertexInputAttribute::Float4, 4 * sizeof(float), 1 },
        { 1, 3, QRhiVertexInputAttribute::Float4, 8 * sizeof(float), 2 },
        { 1, 4, QRhiVertexInputAttribute::Float4, 12 * sizeof(float), 3 },
        { 1, 5, QRhiVertexInputAttribute::Float3, 16 * sizeof(float) },     // instColor
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();

    QByteArray instData;
    instData.resize(INSTANCE_COUNT * 19 * sizeof(float));
    float *p = reinterpret_cast<float *>(instData.data());
    QRandomGenerator *rgen = QRandomGenerator::global();
    for (int i = 0; i < INSTANCE_COUNT; ++i) {
        QMatrix4x4 m;
        m.translate(rgen->bounded(8000) / 100.0f - 40.0f,
                    rgen->bounded(8000) / 100.0f - 40.0f,
                    0.0f);
        memcpy(p, m.constData(), 16 * sizeof(float));
        p += 16;
        // color
        *p++ = i / float(INSTANCE_COUNT);
        *p++ = 0.0f;
        *p++ = 0.0f;
    }
    d.initialUpdates->uploadStaticBuffer(d.instBuf, instData.constData());
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

    if (d.winProj != m_proj) {
        d.winProj = m_proj;
        QMatrix4x4 mvp = m_proj;
        mvp.scale(0.05f);
        u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());
    }

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding[] = {
        { d.vbuf, 0 },
        { d.instBuf, 0 }
    };
    cb->setVertexInput(0, 2, vbufBinding);
    cb->draw(36, INSTANCE_COUNT);
    cb->endPass();
}
