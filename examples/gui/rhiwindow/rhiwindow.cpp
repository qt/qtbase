// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "rhiwindow.h"
#include <QPlatformSurfaceEvent>
#include <QPainter>
#include <QFile>
#include <rhi/qshader.h>

//! [rhiwindow-ctor]
RhiWindow::RhiWindow(QRhi::Implementation graphicsApi)
    : m_graphicsApi(graphicsApi)
{
    switch (graphicsApi) {
    case QRhi::OpenGLES2:
        setSurfaceType(OpenGLSurface);
        break;
    case QRhi::Vulkan:
        setSurfaceType(VulkanSurface);
        break;
    case QRhi::D3D11:
    case QRhi::D3D12:
        setSurfaceType(Direct3DSurface);
        break;
    case QRhi::Metal:
        setSurfaceType(MetalSurface);
        break;
    case QRhi::Null:
        break; // RasterSurface
    }
}
//! [rhiwindow-ctor]

QString RhiWindow::graphicsApiName() const
{
    switch (m_graphicsApi) {
    case QRhi::Null:
        return QLatin1String("Null (no output)");
    case QRhi::OpenGLES2:
        return QLatin1String("OpenGL");
    case QRhi::Vulkan:
        return QLatin1String("Vulkan");
    case QRhi::D3D11:
        return QLatin1String("Direct3D 11");
    case QRhi::D3D12:
        return QLatin1String("Direct3D 12");
    case QRhi::Metal:
        return QLatin1String("Metal");
    }
    return QString();
}

//! [expose]
void RhiWindow::exposeEvent(QExposeEvent *)
{
    // initialize and start rendering when the window becomes usable for graphics purposes
    if (isExposed() && !m_initialized) {
        init();
        resizeSwapChain();
        m_initialized = true;
    }

    const QSize surfaceSize = m_hasSwapChain ? m_sc->surfacePixelSize() : QSize();

    // stop pushing frames when not exposed (or size is 0)
    if ((!isExposed() || (m_hasSwapChain && surfaceSize.isEmpty())) && m_initialized && !m_notExposed)
        m_notExposed = true;

    // Continue when exposed again and the surface has a valid size. Note that
    // surfaceSize can be (0, 0) even though size() reports a valid one, hence
    // trusting surfacePixelSize() and not QWindow.
    if (isExposed() && m_initialized && m_notExposed && !surfaceSize.isEmpty()) {
        m_notExposed = false;
        m_newlyExposed = true;
    }

    // always render a frame on exposeEvent() (when exposed) in order to update
    // immediately on window resize.
    if (isExposed() && !surfaceSize.isEmpty())
        render();
}
//! [expose]

//! [event]
bool RhiWindow::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::UpdateRequest:
        render();
        break;

    case QEvent::PlatformSurface:
        // this is the proper time to tear down the swapchain (while the native window and surface are still around)
        if (static_cast<QPlatformSurfaceEvent *>(e)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            releaseSwapChain();
        break;

    default:
        break;
    }

    return QWindow::event(e);
}
//! [event]

//! [rhi-init]
void RhiWindow::init()
{
    if (m_graphicsApi == QRhi::Null) {
        QRhiNullInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Null, &params));
    }

#if QT_CONFIG(opengl)
    if (m_graphicsApi == QRhi::OpenGLES2) {
        m_fallbackSurface.reset(QRhiGles2InitParams::newFallbackSurface());
        QRhiGles2InitParams params;
        params.fallbackSurface = m_fallbackSurface.get();
        params.window = this;
        m_rhi.reset(QRhi::create(QRhi::OpenGLES2, &params));
    }
#endif

#if QT_CONFIG(vulkan)
    if (m_graphicsApi == QRhi::Vulkan) {
        QRhiVulkanInitParams params;
        params.inst = vulkanInstance();
        params.window = this;
        m_rhi.reset(QRhi::create(QRhi::Vulkan, &params));
    }
#endif

#ifdef Q_OS_WIN
    if (m_graphicsApi == QRhi::D3D11) {
        QRhiD3D11InitParams params;
        // Enable the debug layer, if available. This is optional
        // and should be avoided in production builds.
        params.enableDebugLayer = true;
        m_rhi.reset(QRhi::create(QRhi::D3D11, &params));
    } else if (m_graphicsApi == QRhi::D3D12) {
        QRhiD3D12InitParams params;
        // Enable the debug layer, if available. This is optional
        // and should be avoided in production builds.
        params.enableDebugLayer = true;
        m_rhi.reset(QRhi::create(QRhi::D3D12, &params));
    }
#endif

#if QT_CONFIG(metal)
    if (m_graphicsApi == QRhi::Metal) {
        QRhiMetalInitParams params;
        m_rhi.reset(QRhi::create(QRhi::Metal, &params));
    }
#endif

    if (!m_rhi)
        qFatal("Failed to create RHI backend");
//! [rhi-init]

//! [swapchain-init]
    m_sc.reset(m_rhi->newSwapChain());
    m_ds.reset(m_rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil,
                                      QSize(), // no need to set the size here, due to UsedWithSwapChainOnly
                                      1,
                                      QRhiRenderBuffer::UsedWithSwapChainOnly));
    m_sc->setWindow(this);
    m_sc->setDepthStencil(m_ds.get());
    m_rp.reset(m_sc->newCompatibleRenderPassDescriptor());
    m_sc->setRenderPassDescriptor(m_rp.get());
//! [swapchain-init]

    customInit();
}

//! [swapchain-resize]
void RhiWindow::resizeSwapChain()
{
    m_hasSwapChain = m_sc->createOrResize(); // also handles m_ds

    const QSize outputSize = m_sc->currentPixelSize();
    m_viewProjection = m_rhi->clipSpaceCorrMatrix();
    m_viewProjection.perspective(45.0f, outputSize.width() / (float) outputSize.height(), 0.01f, 1000.0f);
    m_viewProjection.translate(0, 0, -4);
}
//! [swapchain-resize]

void RhiWindow::releaseSwapChain()
{
    if (m_hasSwapChain) {
        m_hasSwapChain = false;
        m_sc->destroy();
    }
}

//! [render-precheck]
void RhiWindow::render()
{
    if (!m_hasSwapChain || m_notExposed)
        return;
//! [render-precheck]

//! [render-resize]
    // If the window got resized or newly exposed, resize the swapchain. (the
    // newly-exposed case is not actually required by some platforms, but is
    // here for robustness and portability)
    //
    // This (exposeEvent + the logic here) is the only safe way to perform
    // resize handling. Note the usage of the RHI's surfacePixelSize(), and
    // never QWindow::size(). (the two may or may not be the same under the hood,
    // depending on the backend and platform)
    //
    if (m_sc->currentPixelSize() != m_sc->surfacePixelSize() || m_newlyExposed) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        m_newlyExposed = false;
    }
//! [render-resize]

//! [beginframe]
    QRhi::FrameOpResult result = m_rhi->beginFrame(m_sc.get());
    if (result == QRhi::FrameOpSwapChainOutOfDate) {
        resizeSwapChain();
        if (!m_hasSwapChain)
            return;
        result = m_rhi->beginFrame(m_sc.get());
    }
    if (result != QRhi::FrameOpSuccess) {
        qWarning("beginFrame failed with %d, will retry", result);
        requestUpdate();
        return;
    }

    customRender();
//! [beginframe]

//! [request-update]
    m_rhi->endFrame(m_sc.get());

    // Always request the next frame via requestUpdate(). On some platforms this is backed
    // by a platform-specific solution, e.g. CVDisplayLink on macOS, which is potentially
    // more efficient than a timer, queued metacalls, etc.
    requestUpdate();
}
//! [request-update]

static float vertexData[] = {
    // Y up (note clipSpaceCorrMatrix in m_viewProjection), CCW
     0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
    -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f,
};

//! [getshader]
static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}
//! [getshader]

HelloWindow::HelloWindow(QRhi::Implementation graphicsApi)
    : RhiWindow(graphicsApi)
{
}

//! [ensure-texture]
void HelloWindow::ensureFullscreenTexture(const QSize &pixelSize, QRhiResourceUpdateBatch *u)
{
    if (m_texture && m_texture->pixelSize() == pixelSize)
        return;

    if (!m_texture)
        m_texture.reset(m_rhi->newTexture(QRhiTexture::RGBA8, pixelSize));
    else
        m_texture->setPixelSize(pixelSize);

    m_texture->create();

    QImage image(pixelSize, QImage::Format_RGBA8888_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatio());
//! [ensure-texture]
    QPainter painter(&image);
    painter.fillRect(QRectF(QPointF(0, 0), size()), QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f));
    painter.setPen(Qt::transparent);
    painter.setBrush({ QGradient(QGradient::DeepBlue) });
    painter.drawRoundedRect(QRectF(QPointF(20, 20), size() - QSize(40, 40)), 16, 16);
    painter.setPen(Qt::black);
    QFont font;
    font.setPixelSize(0.05 * qMin(width(), height()));
    painter.setFont(font);
    painter.drawText(QRectF(QPointF(60, 60), size() - QSize(120, 120)), 0,
                     QLatin1String("Rendering with QRhi to a resizable QWindow.\nThe 3D API is %1.\nUse the command-line options to choose a different API.")
                     .arg(graphicsApiName()));
    painter.end();

    if (m_rhi->isYUpInNDC())
        image.flip();

//! [ensure-texture-2]
    u->uploadTexture(m_texture.get(), image);
//! [ensure-texture-2]
}

//! [render-init-1]
void HelloWindow::customInit()
{
    m_initialUpdates = m_rhi->nextResourceUpdateBatch();

    m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData)));
    m_vbuf->create();
    m_initialUpdates->uploadStaticBuffer(m_vbuf.get(), vertexData);

    static const quint32 UBUF_SIZE = 68;
    m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE));
    m_ubuf->create();
//! [render-init-1]

    ensureFullscreenTexture(m_sc->surfacePixelSize(), m_initialUpdates);

    m_sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                      QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    m_sampler->create();

 //! [render-init-2]
    m_colorTriSrb.reset(m_rhi->newShaderResourceBindings());
    static const QRhiShaderResourceBinding::StageFlags visibility =
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    m_colorTriSrb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, visibility, m_ubuf.get())
    });
    m_colorTriSrb->create();

    m_colorPipeline.reset(m_rhi->newGraphicsPipeline());
    // Enable depth testing; not quite needed for a simple triangle, but we
    // have a depth-stencil buffer so why not.
    m_colorPipeline->setDepthTest(true);
    m_colorPipeline->setDepthWrite(true);
    // Blend factors default to One, OneOneMinusSrcAlpha, which is convenient.
    QRhiGraphicsPipeline::TargetBlend premulAlphaBlend;
    premulAlphaBlend.enable = true;
    m_colorPipeline->setTargetBlends({ premulAlphaBlend });
    m_colorPipeline->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/color.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/color.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 5 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float3, 2 * sizeof(float) }
    });
    m_colorPipeline->setVertexInputLayout(inputLayout);
    m_colorPipeline->setShaderResourceBindings(m_colorTriSrb.get());
    m_colorPipeline->setRenderPassDescriptor(m_rp.get());
    m_colorPipeline->create();
//! [render-init-2]

    m_fullscreenQuadSrb.reset(m_rhi->newShaderResourceBindings());
    m_fullscreenQuadSrb->setBindings({
            QRhiShaderResourceBinding::sampledTexture(0, QRhiShaderResourceBinding::FragmentStage,
                                       m_texture.get(), m_sampler.get())
    });
    m_fullscreenQuadSrb->create();

    m_fullscreenQuadPipeline.reset(m_rhi->newGraphicsPipeline());
    m_fullscreenQuadPipeline->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/quad.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/quad.frag.qsb")) }
    });
    m_fullscreenQuadPipeline->setVertexInputLayout({});
    m_fullscreenQuadPipeline->setShaderResourceBindings(m_fullscreenQuadSrb.get());
    m_fullscreenQuadPipeline->setRenderPassDescriptor(m_rp.get());
    m_fullscreenQuadPipeline->create();
}

//! [render-1]
void HelloWindow::customRender()
{
    QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();

    if (m_initialUpdates) {
        resourceUpdates->merge(m_initialUpdates);
        m_initialUpdates->release();
        m_initialUpdates = nullptr;
    }
//! [render-1]

//! [render-rotation]
    m_rotation += 1.0f;
    QMatrix4x4 modelViewProjection = m_viewProjection;
    modelViewProjection.rotate(m_rotation, 0, 1, 0);
    resourceUpdates->updateDynamicBuffer(m_ubuf.get(), 0, 64, modelViewProjection.constData());
//! [render-rotation]

//! [render-opacity]
    m_opacity += m_opacityDir * 0.005f;
    if (m_opacity < 0.0f || m_opacity > 1.0f) {
        m_opacityDir *= -1;
        m_opacity = qBound(0.0f, m_opacity, 1.0f);
    }
    resourceUpdates->updateDynamicBuffer(m_ubuf.get(), 64, 4, &m_opacity);
//! [render-opacity]

//! [render-cb]
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
//! [render-cb]

    // (re)create the texture with a size matching the output surface size, when necessary.
    ensureFullscreenTexture(outputSizeInPixels, resourceUpdates);

//! [render-pass]
    cb->beginPass(m_sc->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, resourceUpdates);
//! [render-pass]

    cb->setGraphicsPipeline(m_fullscreenQuadPipeline.get());
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    cb->draw(3);

//! [render-pass-record]
    cb->setGraphicsPipeline(m_colorPipeline.get());
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);

    cb->endPass();
//! [render-pass-record]
}
