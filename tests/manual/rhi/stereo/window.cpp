// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"
#include <QPlatformSurfaceEvent>
#include <QTimer>
#include <QFile>
#include <rhi/qshader.h>
#include "../shared/cube.h"

Window::Window()
{
    setSurfaceType(OpenGLSurface);
}

void Window::exposeEvent(QExposeEvent *)
{
    if (isExposed() && !m_running) {
        m_running = true;
        init();
        resizeSwapChain();
    }

    const QSize surfaceSize = m_hasSwapChain ? m_sc->surfacePixelSize() : QSize();
    if ((!isExposed() || (m_hasSwapChain && surfaceSize.isEmpty())) && m_running && !m_notExposed)
        m_notExposed = true;

    if (isExposed() && m_running && m_notExposed && !surfaceSize.isEmpty()) {
        m_notExposed = false;
        m_newlyExposed = true;
    }

    if (isExposed() && !surfaceSize.isEmpty())
        render();
}

bool Window::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        render();
        break;

    case QEvent::PlatformSurface:
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            releaseSwapChain();
        break;

    default:
        break;
    }

    return QWindow::event(e);
}

void Window::init()
{
    QRhi::Flags rhiFlags = QRhi::EnableDebugMarkers;

    m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
    QRhiGles2InitParams params;
    params.fallbackSurface = m_fallbackSurface.get();
    params.window = this;
    m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params, rhiFlags));

    m_sc.reset(m_rhi->newSwapChain());
    m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                                      QSize(),
                                      1,
                                      QRhiRenderBuffer::UsedWithSwapChainOnly));
    m_sc->setWindow(this);
    m_sc->setDepthStencil(m_ds.get());
    m_rp.reset(m_sc->newCompatibleRenderPassDescriptor());
    m_sc->setRenderPassDescriptor(m_rp.get());

    m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube)));
    m_vbuf->setName(QByteArrayLiteral("vbuf"));
    m_vbuf->create();
    m_vbufReady = false;

    m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4));
    m_ubuf->setName(QByteArrayLiteral("Left eye"));
    m_ubuf->create();

    m_srb.reset(m_rhi->newShaderResourceBindings());
    m_srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_ubuf.get())
    });
    m_srb->create();


    m_ubuf2.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4));
    m_ubuf2->setName(QByteArrayLiteral("Right eye"));
    m_ubuf2->create();

    m_srb2.reset(m_rhi->newShaderResourceBindings());
    m_srb2->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                     m_ubuf2.get())
    });
    m_srb2->create();

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
            { 3 * sizeof(float) },
            { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
            { 1, 1, QRhiVertexInputAttribute::Float2, 0 }
    });

    m_ps->setVertexInputLayout(inputLayout);
    m_ps->setShaderResourceBindings(m_srb.get());
    m_ps->setRenderPassDescriptor(m_rp.get());

    m_ps->create();

    m_ps->setDepthTest(true);
    m_ps->setDepthWrite(true);
    m_ps->setDepthOp(QRhiGraphicsPipeline::Less);

    m_ps->setCullMode(QRhiGraphicsPipeline::Back);
    m_ps->setFrontFace(QRhiGraphicsPipeline::CCW);
}

void Window::resizeSwapChain()
{
    m_hasSwapChain = m_sc->createOrResize(); // also handles m_ds

    const QSize outputSize = m_sc->currentPixelSize();
    m_proj = m_rhi->clipSpaceCorrMatrix();
    m_proj.perspective(45.0f, outputSize.width() / (float) outputSize.height(), 0.01f, 1000.0f);
    m_proj.translate(0, 0, -4);
}

void Window::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_sc->destroy();
    }
}

void Window::render()
{
    if (!m_hasSwapChain || m_notExposed)
        return;

    if (m_sc->currentPixelSize() != m_sc->surfacePixelSize() || m_newlyExposed) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        m_newlyExposed = false;
    }

    QRhi::FrameOpResult r = m_rhi->beginFrame(m_sc.get());
    if (r == QRhi::FrameOpSwapChainOutOfDate) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        r = m_rhi->beginFrame(m_sc.get());
    }
    if (r != QRhi::FrameOpSuccess) {
        qDebug("beginFrame failed with %d, retry", r);
        requestUpdate();
        return;
    }

    recordFrame();

    m_rhi->endFrame(m_sc.get());

    QTimer::singleShot(0, this, [this] { render(); });
}

QShader Window::getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

// called once per frame
void Window::recordFrame()
{
    QRhiResourceUpdateBatch *u = m_rhi->nextResourceUpdateBatch();
    if (!m_vbufReady) {
        m_vbufReady = true;
        u->uploadStaticBuffer(m_vbuf.get(), cube);
    }

    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    const QSize outputSizeInPixels = m_sc->currentPixelSize();

    const QColor clearColor = QColor::fromRgbF(0.15f, 0.15f, 0.15f, 1.0f);
    const QRhiDepthStencilClearValue depthStencil = { 1.0f, 0 };
    const QRhiViewport viewPort = { 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) };
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { m_vbuf.get(), 0 },
        { m_vbuf.get(), quint32(36 * 3 * sizeof(float)) }
    };

    QMatrix4x4 mvp = m_rhi->clipSpaceCorrMatrix();
    mvp.perspective(45.0f, outputSizeInPixels.width() / (float) outputSizeInPixels.height(), 0.01f, 100.0f);

    m_translation += m_translationDir * 0.05f;
    if (m_translation < -10.0f || m_translation > -5.0f) {
        m_translationDir *= -1;
        m_translation = qBound(-10.0f, m_translation, -5.0f);
    }
    mvp.translate(0, 0, m_translation);
    m_rotation += .5f;
    mvp.rotate(m_rotation, 0, 1, 0);
    u->updateDynamicBuffer(m_ubuf.get(), 0, 64, mvp.constData());

    float opacity = 1.0f;
    u->updateDynamicBuffer(m_ubuf.get(), 64, 4, &opacity);

    QMatrix4x4 mvp2 = mvp;
    mvp2.translate(-2.f, 0, 0);

    u->updateDynamicBuffer(m_ubuf2.get(), 0, 64, mvp2.constData());
    u->updateDynamicBuffer(m_ubuf2.get(), 64, 4, &opacity);

    cb->resourceUpdate(u);

    cb->beginPass(m_sc->currentFrameRenderTarget(QRhiSwapChain::LeftBuffer), clearColor, depthStencil);
    cb->setGraphicsPipeline(m_ps.get());
    cb->setViewport(viewPort);
    cb->setShaderResources(m_srb.get());
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(36);
    cb->endPass();

    cb->beginPass(m_sc->currentFrameRenderTarget(QRhiSwapChain::RightBuffer), clearColor, depthStencil);
    cb->setGraphicsPipeline(m_ps.get());
    cb->setViewport(viewPort);
    cb->setShaderResources(m_srb2.get());
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(36);
    cb->endPass();
}
