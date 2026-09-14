// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define EXAMPLEFW_PREINIT
#include "../shared/examplefw.h"
#include "../shared/cube.h"
#include <QRandomGenerator>

// The same scene as the noninstanced test, but the per-object data is a push
// constant block instead of a uniform buffer and a QRhiShaderResourceBindings
// per object, so the pass binds its resources once and then varies only the
// push constants between the draws.

const int OBJECT_COUNT = 25000;

struct {
    QList<QRhiResource *> releasePool;

    QRhiBuffer *vbuf;
    QRhiBuffer *ubuf;
    QRhiShaderResourceBindings *srb;
    QRhiGraphicsPipeline *ps;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;

    struct {
        float translation[4];
        float color[4];
    } objData[OBJECT_COUNT];
    float rot = 0.0f;
} d;

void preInit()
{
    debugLayer = false;
}

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::PushConstants))
        qFatal("Push constants are not supported by this backend");

    qDebug("MaxPushConstantsSize is %d bytes", m_r->resourceLimit(QRhi::MaxPushConstantsSize));

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64);
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
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/material.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/material.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();

    QRandomGenerator *rgen = QRandomGenerator::global();
    for (int i = 0; i < OBJECT_COUNT; ++i) {
        d.objData[i].translation[0] = rgen->bounded(8000) / 100.0f - 40.0f;
        d.objData[i].translation[1] = rgen->bounded(8000) / 100.0f - 40.0f;
        d.objData[i].translation[2] = rgen->bounded(100) / -4.0f;
        d.objData[i].translation[3] = 0.0f;
        d.objData[i].color[0] = i / float(OBJECT_COUNT);
        d.objData[i].color[1] = 0.0f;
        d.objData[i].color[2] = 0.0f;
        d.objData[i].color[3] = 1.0f;
    }
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

    QMatrix4x4 mvp = m_proj;
    mvp.rotate(d.rot, 0, 1, 0);
    mvp.scale(0.05f);
    u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(d.vbuf, 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    for (int i = 0; i < OBJECT_COUNT; ++i) {
        // Two calls, so that updating a part of the block at a non-zero offset
        // gets exercised as well.
        cb->setPushConstants(0, 16, d.objData[i].translation);
        cb->setPushConstants(16, 16, d.objData[i].color);
        cb->draw(36);
    }
    cb->endPass();

    d.rot += 0.1f;
}
