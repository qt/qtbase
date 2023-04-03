// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// An example exercising more than a single feature. Enables profiling
// (resource logging to a file) and inserts debug markers and sets some
// object names. Can also be used to test MSAA swapchains, swapchain image
// readback, requesting an sRGB swapchain, and some texture features.

#define EXAMPLEFW_PREINIT
#include "../shared/examplefw.h"
#include "trianglerenderer.h"
#include "quadrenderer.h"
#include "texturedcuberenderer.h"
#include "triangleoncuberenderer.h"

#include <QFileInfo>
#include <QFile>
#include <QElapsedTimer>

//#define SKIP_PRESENT
//#define USE_MSAA
//#define USE_SRGB_SWAPCHAIN
//#define READBACK_SWAPCHAIN
//#define NO_VSYNC
//#define USE_MIN_SWAPCHAIN_BUFFERS
//#define DECLARE_EXT_CONTENTS

struct {
    TriangleRenderer triRenderer;
    QuadRenderer quadRenderer;
    TexturedCubeRenderer cubeRenderer;
    TriangleOnCubeRenderer liveTexCubeRenderer;
    bool onScreenOnly = false;
    bool triangleOnly = false;
    QSize lastOutputSize;
    int frameCount = 0;
    QFile profOut;
    QElapsedTimer gpuFrameTimePrintTimer;
} d;

void preInit()
{
#ifdef SKIP_PRESENT
    endFrameFlags |= QRhi::SkipPresent;
#endif

#ifdef USE_MSAA
    sampleCount = 4; // enable 4x MSAA (except for the render-to-texture pass)
#endif

#ifdef READBACK_SWAPCHAIN
    scFlags |= QRhiSwapChain::UsedAsTransferSource;
#endif

#ifdef USE_SRGB_SWAPCHAIN
    scFlags |= QRhiSwapChain::sRGB;
#endif

#ifdef NO_VSYNC
    scFlags |= QRhiSwapChain::NoVSync;
#endif

#ifdef USE_MIN_SWAPCHAIN_BUFFERS
    scFlags |= QRhiSwapChain::MinimalBufferCount;
#endif

#ifdef DECLARE_EXT_CONTENTS
    beginFrameFlags |= QRhi::ExternalContentsInPass;
#endif

    // For OpenGL some of these are incorporated into the QSurfaceFormat by
    // examplefw.h after returning from here as that is out of the RHI's control.
}

void Window::customInit()
{
    d.triRenderer.setRhi(m_r);
    d.triRenderer.setSampleCount(sampleCount);
    d.triRenderer.initResources(m_rp);

    if (!d.triangleOnly) {
        d.triRenderer.setTranslation(QVector3D(0, 0.5f, 0));

        d.quadRenderer.setRhi(m_r);
        d.quadRenderer.setSampleCount(sampleCount);
        d.quadRenderer.setPipeline(d.triRenderer.pipeline());
        d.quadRenderer.initResources(m_rp);
        d.quadRenderer.setTranslation(QVector3D(1.5f, -0.5f, 0));

        d.cubeRenderer.setRhi(m_r);
        d.cubeRenderer.setSampleCount(sampleCount);
        d.cubeRenderer.initResources(m_rp);
        d.cubeRenderer.setTranslation(QVector3D(0, -0.5f, 0));
    }

    if (!d.onScreenOnly) {
        d.liveTexCubeRenderer.setRhi(m_r);
        d.liveTexCubeRenderer.setSampleCount(sampleCount);
        d.liveTexCubeRenderer.initResources(m_rp);
        d.liveTexCubeRenderer.setTranslation(QVector3D(-2.0f, 0, 0));
    }

    // Check some features/limits.
    qDebug("isFeatureSupported(MultisampleTexture): %d", m_r->isFeatureSupported(QRhi::MultisampleTexture));
    qDebug("isFeatureSupported(MultisampleRenderBuffer): %d", m_r->isFeatureSupported(QRhi::MultisampleRenderBuffer));
    qDebug("isFeatureSupported(DebugMarkers): %d", m_r->isFeatureSupported(QRhi::DebugMarkers));
    qDebug("isFeatureSupported(Timestamps): %d", m_r->isFeatureSupported(QRhi::Timestamps));
    qDebug("isFeatureSupported(Instancing): %d", m_r->isFeatureSupported(QRhi::Instancing));
    qDebug("isFeatureSupported(CustomInstanceStepRate): %d", m_r->isFeatureSupported(QRhi::CustomInstanceStepRate));
    qDebug("isFeatureSupported(PrimitiveRestart): %d", m_r->isFeatureSupported(QRhi::PrimitiveRestart));
    qDebug("isFeatureSupported(NonDynamicUniformBuffers): %d", m_r->isFeatureSupported(QRhi::NonDynamicUniformBuffers));
    qDebug("isFeatureSupported(NonFourAlignedEffectiveIndexBufferOffset): %d", m_r->isFeatureSupported(QRhi::NonFourAlignedEffectiveIndexBufferOffset));
    qDebug("isFeatureSupported(NPOTTextureRepeat): %d", m_r->isFeatureSupported(QRhi::NPOTTextureRepeat));
    qDebug("isFeatureSupported(RedOrAlpha8IsRed): %d", m_r->isFeatureSupported(QRhi::RedOrAlpha8IsRed));
    qDebug("isFeatureSupported(ElementIndexUint): %d", m_r->isFeatureSupported(QRhi::ElementIndexUint));
    qDebug("isFeatureSupported(Compute): %d", m_r->isFeatureSupported(QRhi::Compute));
    qDebug("isFeatureSupported(WideLines): %d", m_r->isFeatureSupported(QRhi::WideLines));
    qDebug("isFeatureSupported(VertexShaderPointSize): %d", m_r->isFeatureSupported(QRhi::VertexShaderPointSize));
    qDebug("isFeatureSupported(BaseVertex): %d", m_r->isFeatureSupported(QRhi::BaseVertex));
    qDebug("isFeatureSupported(BaseInstance): %d", m_r->isFeatureSupported(QRhi::BaseInstance));
    qDebug("isFeatureSupported(PipelineCacheDataLoadSave): %d", m_r->isFeatureSupported(QRhi::PipelineCacheDataLoadSave));
    qDebug("Min 2D texture width/height: %d", m_r->resourceLimit(QRhi::TextureSizeMin));
    qDebug("Max 2D texture width/height: %d", m_r->resourceLimit(QRhi::TextureSizeMax));
    qDebug("Max color attachment count: %d", m_r->resourceLimit(QRhi::MaxColorAttachments));
    qDebug("MaxThreadGroupsPerDimension: %d", m_r->resourceLimit(QRhi::MaxThreadGroupsPerDimension));
    qDebug("MaxThreadsPerThreadGroup: %d", m_r->resourceLimit(QRhi::MaxThreadsPerThreadGroup));
    qDebug("MaxThreadGroupX: %d", m_r->resourceLimit(QRhi::MaxThreadGroupX));
    qDebug("MaxThreadGroupY: %d", m_r->resourceLimit(QRhi::MaxThreadGroupY));
    qDebug("MaxThreadGroupZ: %d", m_r->resourceLimit(QRhi::MaxThreadGroupZ));
    qDebug("TextureArraySizeMax: %d", m_r->resourceLimit(QRhi::TextureArraySizeMax));
    qDebug("MaxUniformBufferRange: %d", m_r->resourceLimit(QRhi::MaxUniformBufferRange));
    qDebug("MaxVertexInputs: %d", m_r->resourceLimit(QRhi::MaxVertexInputs));
    qDebug("MaxVertexOutputs: %d", m_r->resourceLimit(QRhi::MaxVertexOutputs));

    // With Vulkan at least we should see some details from the memory allocator.
    qDebug() << m_r->statistics();

    // Every two seconds try printing last known gpu frame time.
    d.gpuFrameTimePrintTimer.start();
}

void Window::customRelease()
{
    d.triRenderer.releaseResources();

    if (!d.triangleOnly) {
        d.quadRenderer.releaseResources();
        d.cubeRenderer.releaseResources();
    }

    if (!d.onScreenOnly)
        d.liveTexCubeRenderer.releaseResources();
}

void Window::customRender()
{
    const QSize outputSize = m_sc->currentPixelSize();
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();

    if (d.gpuFrameTimePrintTimer.elapsed() > 2000) {
        qDebug() << "Last completed GPU frame time" << cb->lastCompletedGpuTime() << "seconds";
        d.gpuFrameTimePrintTimer.restart();
    }

    if (outputSize != d.lastOutputSize) {
        d.triRenderer.resize(outputSize);
        if (!d.triangleOnly) {
            d.quadRenderer.resize(outputSize);
            d.cubeRenderer.resize(outputSize);
        }
        if (!d.onScreenOnly)
            d.liveTexCubeRenderer.resize(outputSize);
        d.lastOutputSize = outputSize;
    }

    if (!d.onScreenOnly) {
        cb->debugMarkBegin("Offscreen triangle pass");
        d.liveTexCubeRenderer.queueOffscreenPass(cb);
        cb->debugMarkEnd();
    }

    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    d.triRenderer.queueResourceUpdates(u);
    if (!d.triangleOnly) {
        d.quadRenderer.queueResourceUpdates(u);
        d.cubeRenderer.queueResourceUpdates(u);
    }
    if (!d.onScreenOnly)
        d.liveTexCubeRenderer.queueResourceUpdates(u);

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->debugMarkBegin(QByteArrayLiteral("Triangle"));
    d.triRenderer.queueDraw(cb, outputSize);
    cb->debugMarkEnd();
    if (!d.triangleOnly) {
        cb->debugMarkMsg(QByteArrayLiteral("More stuff"));
        cb->debugMarkBegin(QByteArrayLiteral("Quad"));
        d.quadRenderer.queueDraw(cb, outputSize);
        cb->debugMarkEnd();
        cb->debugMarkBegin(QByteArrayLiteral("Cube"));
        d.cubeRenderer.queueDraw(cb, outputSize);
        cb->debugMarkEnd();
    }
    if (!d.onScreenOnly) {
        cb->debugMarkMsg(QByteArrayLiteral("Even more stuff"));
        cb->debugMarkBegin(QByteArrayLiteral("Cube with offscreen triangle"));
        d.liveTexCubeRenderer.queueDraw(cb, outputSize);
        cb->debugMarkEnd();
    }

    QRhiResourceUpdateBatch *passEndUpdates = nullptr;
#ifdef READBACK_SWAPCHAIN
    passEndUpdates = m_r->nextResourceUpdateBatch();
    QRhiReadbackDescription rb; // no texture given -> backbuffer
    QRhiReadbackResult *rbResult = new QRhiReadbackResult;
    int frameNo = d.frameCount;
    rbResult->completed = [this, rbResult, frameNo] {
        {
            QImage::Format fmt = rbResult->format == QRhiTexture::BGRA8 ? QImage::Format_ARGB32_Premultiplied
                                                                        : QImage::Format_RGBA8888_Premultiplied;
            const uchar *p = reinterpret_cast<const uchar *>(rbResult->data.constData());
            QImage image(p, rbResult->pixelSize.width(), rbResult->pixelSize.height(), fmt);
            QString fn = QString::asprintf("frame%d.png", frameNo);
            fn = QFileInfo(fn).absoluteFilePath();
            qDebug("Saving into %s", qPrintable(fn));
            if (m_r->isYUpInFramebuffer())
                image.mirrored().save(fn);
            else
                image.save(fn);
        }
        delete rbResult;
    };
    passEndUpdates->readBackTexture(rb, rbResult);
#endif

    cb->endPass(passEndUpdates);

    d.frameCount += 1;
}
