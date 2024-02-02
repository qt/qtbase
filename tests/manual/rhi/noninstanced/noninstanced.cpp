// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define EXAMPLEFW_PREINIT
#include "../shared/examplefw.h"
#include "../shared/cube.h"
#include <QRandomGenerator>

// A variation on the instancing test. No instancing here, just
// individual uniform buffer updates and draw calls.

const int INSTANCE_COUNT = 25000;

struct {
    QList<QRhiResource *> releasePool;

    QRhiBuffer *vbuf;
    QRhiBuffer *ubuf[INSTANCE_COUNT];
    QRhiShaderResourceBindings *srb[INSTANCE_COUNT];
    QRhiGraphicsPipeline *ps;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;

    struct {
        float x, y, z;
        float r, g, b;
    } instData[INSTANCE_COUNT];
    float rot = 0.0f;
} d;

void preInit()
{
    debugLayer = false;
}

void Window::customInit()
{
    d.initialUpdates = m_r->nextResourceUpdateBatch();

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

    for (int i = 0; i < INSTANCE_COUNT; ++i) {
        d.ubuf[i] = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 92);
        d.ubuf[i]->create();
        d.releasePool << d.ubuf[i];

        d.srb[i] = m_r->newShaderResourceBindings();
        d.releasePool << d.srb[i];
        d.srb[i]->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, d.ubuf[i])
            });
        d.srb[i]->create();
    }

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
    d.ps->setShaderResourceBindings(d.srb[0]);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();

    QRandomGenerator *rgen = QRandomGenerator::global();
    for (int i = 0; i < INSTANCE_COUNT; ++i) {
        d.instData[i].x = rgen->bounded(8000) / 100.0f - 40.0f;
        d.instData[i].y = rgen->bounded(8000) / 100.0f - 40.0f;
        d.instData[i].z = rgen->bounded(100) / -4.0f;
        d.instData[i].r = i / float(INSTANCE_COUNT);
        d.instData[i].g = 0.0f;
        d.instData[i].b = 0.0f;
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
    QRhiResourceUpdateBatch *u = nullptr;
    if (d.initialUpdates) {
        u = d.initialUpdates;
        d.initialUpdates = nullptr;
    }

    for (int i = 0; i < INSTANCE_COUNT; ++i) {
        char *p = d.ubuf[i]->beginFullDynamicBufferUpdateForCurrentFrame();
        QMatrix4x4 mvp = m_proj;
        mvp.rotate(d.rot, 0, 1, 0);
        mvp.scale(0.05f);
        memcpy(p, mvp.constData(), 64);

        // float *v = reinterpret_cast<float *>(p + 64);
        // v[0] = d.instData[i].x;
        // v[1] = d.instData[i].y;
        // v[2] = d.instData[i].z;
        memcpy(p + 64, &d.instData[i].x, 4);
        memcpy(p + 68, &d.instData[i].y, 4);
        memcpy(p + 72, &d.instData[i].z, 4);

        // v = reinterpret_cast<float *>(p + 80);
        // v[0] = d.instData[i].r;
        // v[1] = d.instData[i].g;
        // v[2] = d.instData[i].b;
        memcpy(p + 80, &d.instData[i].r, 4);
        memcpy(p + 84, &d.instData[i].g, 4);
        memcpy(p + 88, &d.instData[i].b, 4);

        d.ubuf[i]->endFullDynamicBufferUpdateForCurrentFrame();
    }

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u, QRhiCommandBuffer::DoNotTrackResourcesForCompute);
    for (int i = 0; i < INSTANCE_COUNT; ++i) {
        cb->setGraphicsPipeline(d.ps);
        if (i == 0)
            cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
        cb->setShaderResources(d.srb[i]);
        const QRhiCommandBuffer::VertexInput vbufBinding[] = {
            { d.vbuf, 0 },
        };
        cb->setVertexInput(0, 1, vbufBinding);
        cb->draw(36);
    }
    cb->endPass();

    d.rot += 0.1f;
}
