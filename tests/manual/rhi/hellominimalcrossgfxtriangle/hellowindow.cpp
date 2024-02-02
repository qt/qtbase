// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "hellowindow.h"
#include <QFile>
#include <rhi/qshader.h>

static float vertexData[] = {
    // Y up (note clipSpaceCorrMatrix in m_proj), CCW
     0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
    -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f,
};

HelloWindow::HelloWindow(QRhi::Implementation graphicsApi)
    : Window(graphicsApi)
{
}

QShader HelloWindow::getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

void HelloWindow::customInit()
{
    m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData)));
    m_vbuf->create();
    m_vbufReady = false;

    m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68));
    m_ubuf->create();

    m_srb.reset(m_rhi->newShaderResourceBindings());
    m_srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_ubuf.get())
    });
    m_srb->create();

    m_ps.reset(m_rhi->newGraphicsPipeline());

    QRhiGraphicsPipeline::TargetBlend premulAlphaBlend;
    premulAlphaBlend.enable = true;
    m_ps->setTargetBlends({ premulAlphaBlend });

    const QShader vs = getShader(QLatin1String(":/color.vert.qsb"));
    if (!vs.isValid())
        qFatal("Failed to load shader pack (vertex)");
    const QShader fs = getShader(QLatin1String(":/color.frag.qsb"));
    if (!fs.isValid())
        qFatal("Failed to load shader pack (fragment)");

    m_ps->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 5 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float3, 2 * sizeof(float) }
    });

    m_ps->setVertexInputLayout(inputLayout);
    m_ps->setShaderResourceBindings(m_srb.get());
    m_ps->setRenderPassDescriptor(m_rp.get());

    m_ps->create();
}

// called once per frame
void HelloWindow::customRender()
{
    QRhiResourceUpdateBatch *u = m_rhi->nextResourceUpdateBatch();
    if (!m_vbufReady) {
        m_vbufReady = true;
        u->uploadStaticBuffer(m_vbuf.get(), vertexData);
    }
    m_rotation += 1.0f;
    QMatrix4x4 mvp = m_proj;
    mvp.rotate(m_rotation, 0, 1, 0);
    u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());
    m_opacity += m_opacityDir * 0.005f;
    if (m_opacity < 0.0f || m_opacity > 1.0f) {
        m_opacityDir *= -1;
        m_opacity = qBound(0.0f, m_opacity, 1.0f);
    }
    u->updateDynamicBuffer(m_ubuf.get(), 64, 4, &m_opacity);

    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    const QSize outputSizeInPixels = m_sc->currentPixelSize();

    cb->beginPass(m_sc->currentFrameRenderTarget(), QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f), { 1.0f, 0 }, u);

    cb->setGraphicsPipeline(m_ps.get());
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();

    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);

    cb->endPass();
}
