// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QThread>
#include <QFile>
#include <QOffscreenSurface>
#include <QPainter>
#include <QSpan>
#include <qrgbafloat.h>
#include <qrgba64.h>

#include <private/qrhi_p.h>

#if QT_CONFIG(opengl)
# include <QOpenGLContext>
# include <QOpenGLFunctions>
# include <QtGui/private/qguiapplication_p.h>
# include <qpa/qplatformintegration.h>
# define TST_GL
#endif

#if QT_CONFIG(vulkan)
# include <QVulkanInstance>
# include <QVulkanFunctions>
# define TST_VK
#endif

#ifdef Q_OS_WIN
# define TST_D3D11
# define TST_D3D12
#endif

#if QT_CONFIG(metal)
# define TST_MTL
#endif

Q_DECLARE_METATYPE(QRhi::Implementation)
Q_DECLARE_METATYPE(QRhiInitParams *)

class tst_QRhi : public QObject
{
    Q_OBJECT

private:
    template<typename T, std::size_t E, typename ParamStringifier>
    void rhiTestDataWithParam(const char *paramName, QSpan<T, E> paramValues, ParamStringifier &&paramStringifier);

private slots:
    void initTestCase();
    void cleanupTestCase();

    void rhiTestData();
    void create_data();
    void create();
    void adapters_data();
    void adapters();
    void stats_data();
    void stats();
    void nativeHandles_data();
    void nativeHandles();
    void nativeHandlesImportVulkan();
    void nativeHandlesImportD3D11();
    void nativeHandlesImportOpenGL();
    void nativeTexture_data();
    void nativeTexture();
    void nativeBuffer_data();
    void nativeBuffer();
    void resourceUpdateBatchBuffer_data();
    void resourceUpdateBatchBuffer();
    void resourceUpdateBatchRGBATextureUpload_data();
    void resourceUpdateBatchRGBATextureUpload();
    void resourceUpdateBatchRGBATextureCopy_data();
    void resourceUpdateBatchRGBATextureCopy();
    void resourceUpdateBatchRGBATextureMip_data();
    void resourceUpdateBatchRGBATextureMip();
    void resourceUpdateBatchTextureRawDataStride_data();
    void resourceUpdateBatchTextureRawDataStride();
    void resourceUpdateBatchLotsOfResources_data();
    void resourceUpdateBatchLotsOfResources();
    void resourceUpdateBatchBetweenFrames_data();
    void resourceUpdateBatchBetweenFrames();
    void invalidPipeline_data();
    void invalidPipeline();
    void srbLayoutCompatibility_data();
    void srbLayoutCompatibility();
    void srbWithNoResource_data();
    void srbWithNoResource();
    void renderPassDescriptorCompatibility_data();
    void renderPassDescriptorCompatibility();
    void renderPassDescriptorClone_data();
    void renderPassDescriptorClone();
    void textureWithSampleCount_data();
    void textureWithSampleCount();
    void textureFormats_data();
    void textureFormats();

    void renderToTextureSimple_data();
    void renderToTextureSimple();
    void renderToTextureMip_data();
    void renderToTextureMip();
    void renderToTextureCubemapFace_data();
    void renderToTextureCubemapFace();
    void renderToTextureTextureArray_data();
    void renderToTextureTextureArray();
    void renderToTextureTexturedQuad_data();
    void renderToTextureTexturedQuad();
    void renderToTextureSampleWithSeparateTextureAndSampler_data();
    void renderToTextureSampleWithSeparateTextureAndSampler();
    void renderToTextureArrayOfTexturedQuad_data();
    void renderToTextureArrayOfTexturedQuad();
    void renderToTextureTexturedQuadAndUniformBuffer_data();
    void renderToTextureTexturedQuadAndUniformBuffer();
    void renderToTextureTexturedQuadAllDynamicBuffers_data();
    void renderToTextureTexturedQuadAllDynamicBuffers();
    void renderToTextureDeferredSrb_data();
    void renderToTextureDeferredSrb();
    void renderToTextureDeferredUpdateSamplerInSrb_data();
    void renderToTextureDeferredUpdateSamplerInSrb();
    void renderToTextureMultipleUniformBuffersAndDynamicOffset_data();
    void renderToTextureMultipleUniformBuffersAndDynamicOffset();
    void renderToTextureSrbReuse_data();
    void renderToTextureSrbReuse();
    void renderToTextureIndexedDraw_data();
    void renderToTextureIndexedDraw();
    void renderToTextureArrayMultiView_data();
    void renderToTextureArrayMultiView();
    void renderToWindowSimple_data();
    void renderToWindowSimple();
    void continuousReadbackFromWindow_data();
    void continuousReadbackFromWindow();
    void finishWithinSwapchainFrame_data();
    void finishWithinSwapchainFrame();
    void resourceUpdateBatchBufferTextureWithSwapchainFrames_data();
    void resourceUpdateBatchBufferTextureWithSwapchainFrames();
    void textureRenderTargetAutoRebuild_data();
    void textureRenderTargetAutoRebuild();
    void renderToMRTPerRenderTargetBlending();
    void renderToMRTPerRenderTargetBlending_data();

    void pipelineCache_data();
    void pipelineCache();
    void textureImportOpenGL();
    void renderbufferImportOpenGL();
    void threeDimTexture_data();
    void threeDimTexture();
    void oneDimTexture_data();
    void oneDimTexture();
    void leakedResourceDestroy_data();
    void leakedResourceDestroy();

    void renderToFloatTexture_data();
    void renderToFloatTexture();
    void renderToRgb10Texture_data();
    void renderToRgb10Texture();

    void tessellation_data();
    void tessellation();

    void tessellationInterfaceBlocks_data();
    void tessellationInterfaceBlocks();

    void storageBuffer_data();
    void storageBuffer();
    void storageBufferRuntimeSizeCompute_data();
    void storageBufferRuntimeSizeCompute();
    void storageBufferRuntimeSizeGraphics_data();
    void storageBufferRuntimeSizeGraphics();

    void halfPrecisionAttributes_data();
    void halfPrecisionAttributes();

private:
    void setWindowType(QWindow *window, QRhi::Implementation impl);
    bool isAndroidOpenGLSwiftShader(QRhi::Implementation impl, const QRhi *rhi);

    struct {
        QRhiNullInitParams null;
#ifdef TST_GL
        QRhiGles2InitParams gl;
#endif
#ifdef TST_VK
        QRhiVulkanInitParams vk;
#endif
#ifdef TST_D3D11
        QRhiD3D11InitParams d3d11;
#endif
#ifdef TST_D3D12
        QRhiD3D12InitParams d3d12;
#endif
#ifdef TST_MTL
        QRhiMetalInitParams mtl;
#endif
    } initParams;

#ifdef TST_VK
    QVulkanInstance vulkanInstance;
#endif
    QOffscreenSurface *fallbackSurface = nullptr;
};

void tst_QRhi::initTestCase()
{
#ifdef TST_GL
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(fmt);

    initParams.gl.format = QSurfaceFormat::defaultFormat();
    fallbackSurface = QRhiGles2InitParams::newFallbackSurface();
    initParams.gl.fallbackSurface = fallbackSurface;
#endif

#ifdef TST_VK
    const QVersionNumber supportedVersion = vulkanInstance.supportedApiVersion();
    if (supportedVersion >= QVersionNumber(1, 2))
        vulkanInstance.setApiVersion(QVersionNumber(1, 2));
    else if (supportedVersion >= QVersionNumber(1, 1))
        vulkanInstance.setApiVersion(QVersionNumber(1, 1));
    vulkanInstance.setLayers({ "VK_LAYER_KHRONOS_validation" });
    vulkanInstance.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
    vulkanInstance.create();
    initParams.vk.inst = &vulkanInstance;
#endif

#ifdef TST_D3D11
    initParams.d3d11.enableDebugLayer = true;
#endif
#ifdef TST_D3D12
    initParams.d3d12.enableDebugLayer = true;
#endif
}

void tst_QRhi::cleanupTestCase()
{
#ifdef TST_VK
    vulkanInstance.destroy();
#endif

    delete fallbackSurface;
}

void tst_QRhi::rhiTestData()
{
    QTest::addColumn<QRhi::Implementation>("impl");
    QTest::addColumn<QRhiInitParams *>("initParams");

// webOS does not support raster (software) pipeline
#ifndef Q_OS_WEBOS
    QTest::newRow("Null") << QRhi::Null << static_cast<QRhiInitParams *>(&initParams.null);
#endif
#ifdef TST_GL
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
        QTest::newRow("OpenGL") << QRhi::OpenGLES2 << static_cast<QRhiInitParams *>(&initParams.gl);
#endif
#ifdef TST_VK
    if (vulkanInstance.isValid())
        QTest::newRow("Vulkan") << QRhi::Vulkan << static_cast<QRhiInitParams *>(&initParams.vk);
#endif
#ifdef TST_D3D11
    QTest::newRow("Direct3D 11") << QRhi::D3D11 << static_cast<QRhiInitParams *>(&initParams.d3d11);
#endif
#ifdef TST_D3D12
    QTest::newRow("Direct3D 12") << QRhi::D3D12 << static_cast<QRhiInitParams *>(&initParams.d3d12);
#endif
#ifdef TST_MTL
    QTest::newRow("Metal") << QRhi::Metal << static_cast<QRhiInitParams *>(&initParams.mtl);
#endif
}

template<typename T, std::size_t E, typename ParamStringifier>
void tst_QRhi::rhiTestDataWithParam(const char *paramName, QSpan<T, E> paramValues, ParamStringifier &&paramStringifier)
{
    QTest::addColumn<QRhi::Implementation>("impl");
    QTest::addColumn<QRhiInitParams *>("initParams");
    QTest::addColumn<T>(paramName);

#ifndef Q_OS_WEBOS
    for (auto &paramValue : paramValues) {
        QTest::newRow(qPrintable(QStringLiteral("Null-") + paramStringifier(paramValue)))
            << QRhi::Null << static_cast<QRhiInitParams *>(&initParams.null) << paramValue;
    }
#endif
#ifdef TST_GL
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL)) {
        for (auto &paramValue : paramValues) {
            QTest::newRow(qPrintable(QStringLiteral("OpenGL-") + paramStringifier(paramValue)))
                << QRhi::OpenGLES2 << static_cast<QRhiInitParams *>(&initParams.gl) << paramValue;
        }
    }
#endif
#ifdef TST_VK
    if (vulkanInstance.isValid()) {
        for (auto &paramValue : paramValues) {
            QTest::newRow(qPrintable(QStringLiteral("Vulkan-") + paramStringifier(paramValue)))
                << QRhi::Vulkan << static_cast<QRhiInitParams *>(&initParams.vk) << paramValue;
        }
    }
#endif
#ifdef TST_D3D11
    for (auto &paramValue : paramValues) {
        QTest::newRow(qPrintable(QStringLiteral("Direct3D11-") + paramStringifier(paramValue)))
            << QRhi::D3D11 << static_cast<QRhiInitParams *>(&initParams.d3d11) << paramValue;
    }
#endif
#ifdef TST_D3D12
    for (auto &paramValue : paramValues) {
        QTest::newRow(qPrintable(QStringLiteral("Direct3D12-") + paramStringifier(paramValue)))
            << QRhi::D3D12 << static_cast<QRhiInitParams *>(&initParams.d3d12) << paramValue;
    }
#endif
#ifdef TST_MTL
    for (auto &paramValue : paramValues) {
        QTest::newRow(qPrintable(QStringLiteral("Metal-") + paramStringifier(paramValue)))
            << QRhi::Metal << static_cast<QRhiInitParams *>(&initParams.mtl) << paramValue;
    }
#endif
}

bool tst_QRhi::isAndroidOpenGLSwiftShader(QRhi::Implementation impl, const QRhi *rhi)
{
#ifdef Q_OS_ANDROID
    if (impl == QRhi::OpenGLES2 && rhi->driverInfo().deviceName.contains("SwiftShader"))
        return true;
#else
    Q_UNUSED(impl);
    Q_UNUSED(rhi);
#endif
    return false;
}

void tst_QRhi::create_data()
{
    rhiTestData();
}

static int aligned(int v, int a)
{
    return (v + a - 1) & ~(a - 1);
}

void tst_QRhi::create()
{
    // Merely attempting to create a QRhi should survive, with an error when
    // not supported. (of course, there is always a chance we encounter a crash
    // due to some random graphics stack...)

    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));

    if (rhi) {
        QVERIFY(QRhi::probe(impl, initParams));

        qDebug() << rhi->driverInfo();

        QCOMPARE(rhi->backend(), impl);
        QVERIFY(strcmp(rhi->backendName(), ""));
        QVERIFY(!strcmp(rhi->backendName(), QRhi::backendName(rhi->backend())));
        QVERIFY(!rhi->driverInfo().deviceName.isEmpty());
        QCOMPARE(rhi->thread(), QThread::currentThread());

        // do a basic smoke test for the apis that do not directly render anything

        int cleanupOk = 0;
        QRhi *rhiPtr = rhi.data();
        auto cleanupFunc = [rhiPtr, &cleanupOk](QRhi *dyingRhi) {
            if (rhiPtr == dyingRhi)
                cleanupOk += 1;
        };
        rhi->addCleanupCallback(cleanupFunc);
        rhi->addCleanupCallback(cleanupFunc);
        rhi->addCleanupCallback(reinterpret_cast<const void *>(quintptr(1234)), cleanupFunc);
        rhi->addCleanupCallback(reinterpret_cast<const void *>(quintptr(12345)), cleanupFunc);
        rhi->removeCleanupCallback(reinterpret_cast<const void *>(quintptr(1234)));

        QRhiResourceUpdateBatch *resUpd = rhi->nextResourceUpdateBatch();
        QVERIFY(resUpd);
        resUpd->release();

        QRhiResourceUpdateBatch *resUpdArray[64];
        for (int i = 0; i < 64; ++i) {
            resUpdArray[i] = rhi->nextResourceUpdateBatch();
            QVERIFY(resUpdArray[i]);
        }
        resUpd = rhi->nextResourceUpdateBatch();
        QVERIFY(!resUpd);
        for (int i = 0; i < 64; ++i)
            resUpdArray[i]->release();
        resUpd = rhi->nextResourceUpdateBatch();
        QVERIFY(resUpd);
        resUpd->release();

        const QVector<int> supportedSampleCounts = rhi->supportedSampleCounts();
        QVERIFY(!supportedSampleCounts.isEmpty());
        QVERIFY(supportedSampleCounts.contains(1));
        for (int i = 1; i < supportedSampleCounts.count(); ++i) {
            // Verify the list is sorted. Internally the backends rely on this.
            QVERIFY(supportedSampleCounts[i] > supportedSampleCounts[i - 1]);
        }

        QVERIFY(rhi->ubufAlignment() > 0);
        QCOMPARE(rhi->ubufAligned(123), aligned(123, rhi->ubufAlignment()));

        QCOMPARE(rhi->mipLevelsForSize(QSize(512, 300)), 10);
        QCOMPARE(rhi->sizeForMipLevel(0, QSize(512, 300)), QSize(512, 300));
        QCOMPARE(rhi->sizeForMipLevel(1, QSize(512, 300)), QSize(256, 150));
        QCOMPARE(rhi->sizeForMipLevel(2, QSize(512, 300)), QSize(128, 75));
        QCOMPARE(rhi->sizeForMipLevel(9, QSize(512, 300)), QSize(1, 1));

        const bool fbUp = rhi->isYUpInFramebuffer();
        const bool ndcUp = rhi->isYUpInNDC();
        const bool d0to1 = rhi->isClipDepthZeroToOne();
        const QMatrix4x4 corrMat = rhi->clipSpaceCorrMatrix();
        if (impl == QRhi::OpenGLES2) {
            QVERIFY(fbUp);
            QVERIFY(ndcUp);
            QVERIFY(!d0to1);
            QVERIFY(corrMat.isIdentity());
        } else if (impl == QRhi::Vulkan) {
            QVERIFY(!fbUp);
            QVERIFY(!ndcUp);
            QVERIFY(d0to1);
            QVERIFY(!corrMat.isIdentity());
        } else if (impl == QRhi::D3D11) {
            QVERIFY(!fbUp);
            QVERIFY(ndcUp);
            QVERIFY(d0to1);
            QVERIFY(!corrMat.isIdentity());
        } else if (impl == QRhi::Metal) {
            QVERIFY(!fbUp);
            QVERIFY(ndcUp);
            QVERIFY(d0to1);
            QVERIFY(!corrMat.isIdentity());
        }

        const int texMin = rhi->resourceLimit(QRhi::TextureSizeMin);
        const int texMax = rhi->resourceLimit(QRhi::TextureSizeMax);
        const int maxAtt = rhi->resourceLimit(QRhi::MaxColorAttachments);
        const int framesInFlight = rhi->resourceLimit(QRhi::FramesInFlight);
        const int texArrayMax = rhi->resourceLimit(QRhi::TextureArraySizeMax);
        const int uniBufRangeMax = rhi->resourceLimit(QRhi::MaxUniformBufferRange);
        const int maxVertexInputs = rhi->resourceLimit(QRhi::MaxVertexInputs);
        const int maxVertexOutputs = rhi->resourceLimit(QRhi::MaxVertexOutputs);

        QVERIFY(texMin >= 1);
        QVERIFY(texMax >= texMin);
        QVERIFY(maxAtt >= 1);
        QVERIFY(framesInFlight >= 1);
        if (rhi->isFeatureSupported(QRhi::TextureArrays))
            QVERIFY(texArrayMax > 1);
        QVERIFY(uniBufRangeMax >= 224 * 4 * 4);
        QVERIFY(maxVertexInputs >= 8);
        QVERIFY(maxVertexOutputs >= 8);

        QVERIFY(rhi->nativeHandles());

        const QRhi::Feature features[] = {
            QRhi::MultisampleTexture,
            QRhi::MultisampleRenderBuffer,
            QRhi::DebugMarkers,
            QRhi::Timestamps,
            QRhi::Instancing,
            QRhi::CustomInstanceStepRate,
            QRhi::PrimitiveRestart,
            QRhi::NonDynamicUniformBuffers,
            QRhi::NonFourAlignedEffectiveIndexBufferOffset,
            QRhi::NPOTTextureRepeat,
            QRhi::RedOrAlpha8IsRed,
            QRhi::ElementIndexUint,
            QRhi::Compute,
            QRhi::WideLines,
            QRhi::VertexShaderPointSize,
            QRhi::BaseVertex,
            QRhi::BaseInstance,
            QRhi::TriangleFanTopology,
            QRhi::ReadBackNonUniformBuffer,
            QRhi::ReadBackNonBaseMipLevel,
            QRhi::TexelFetch,
            QRhi::RenderToNonBaseMipLevel,
            QRhi::IntAttributes,
            QRhi::ScreenSpaceDerivatives,
            QRhi::ReadBackAnyTextureFormat,
            QRhi::PipelineCacheDataLoadSave,
            QRhi::ImageDataStride,
            QRhi::RenderBufferImport,
            QRhi::ThreeDimensionalTextures,
            QRhi::RenderTo3DTextureSlice,
            QRhi::TextureArrays,
            QRhi::Tessellation,
            QRhi::GeometryShader,
            QRhi::TextureArrayRange,
            QRhi::NonFillPolygonMode,
            QRhi::OneDimensionalTextures,
            QRhi::OneDimensionalTextureMipmaps,
            QRhi::HalfAttributes,
            QRhi::RenderToOneDimensionalTexture,
            QRhi::ThreeDimensionalTextureMipmaps,
            QRhi::MultiView,
            QRhi::TextureViewFormat,
            QRhi::ResolveDepthStencil
        };
        for (size_t i = 0; i <sizeof(features) / sizeof(QRhi::Feature); ++i)
            rhi->isFeatureSupported(features[i]);

        QVERIFY(rhi->isTextureFormatSupported(QRhiTexture::RGBA8));

        rhi->releaseCachedResources();

        QVERIFY(!rhi->isDeviceLost());

        rhi.reset();
        QCOMPARE(cleanupOk, 3);
    }
}

void tst_QRhi::adapters_data()
{
    rhiTestData();
}

void tst_QRhi::adapters()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QRhi::AdapterList adapters = QRhi::enumerateAdapters(impl, initParams);
    for (QRhiAdapter *adapter : adapters)
        qDebug() << adapter->info();

    if (!adapters.isEmpty()) {
        QRhi *rhi = QRhi::create(impl, initParams, QRhi::Flags(), nullptr, adapters[0]);
        if (rhi) {
            QCOMPARE(rhi->driverInfo().deviceName, adapters[0]->info().deviceName);
            QCOMPARE(rhi->driverInfo().deviceId, adapters[0]->info().deviceId);
            QCOMPARE(rhi->driverInfo().vendorId, adapters[0]->info().vendorId);
            QCOMPARE(rhi->driverInfo().deviceType, adapters[0]->info().deviceType);
        }
        delete rhi;

        // test filtering based on luid/physdev
        if (impl == QRhi::D3D11) {
#ifdef TST_D3D11
            QRhi *rhi = QRhi::create(impl, initParams);
            if (rhi) {
                QRhiD3D11NativeHandles h = *static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());
                delete rhi;
                if (h.adapterLuidLow || h.adapterLuidHigh) {
                    QRhi::AdapterList filteredList = QRhi::enumerateAdapters(impl, initParams, &h);
                    QCOMPARE(filteredList.count(), 1);
                    qDeleteAll(filteredList);
                }
            }
#endif
        } else if (impl == QRhi::D3D12) {
#ifdef TST_D3D12
            QRhi *rhi = QRhi::create(impl, initParams);
            if (rhi) {
                QRhiD3D12NativeHandles h = *static_cast<const QRhiD3D12NativeHandles *>(rhi->nativeHandles());
                delete rhi;
                if (h.adapterLuidLow || h.adapterLuidHigh) {
                    QRhi::AdapterList filteredList = QRhi::enumerateAdapters(impl, initParams, &h);
                    QCOMPARE(filteredList.count(), 1);
                    qDeleteAll(filteredList);
                }
            }
#endif
        } else if (impl == QRhi::Vulkan) {
#ifdef TST_VK
            QRhi *rhi = QRhi::create(impl, initParams);
            if (rhi) {
                QRhiVulkanNativeHandles h = *static_cast<const QRhiVulkanNativeHandles *>(rhi->nativeHandles());
                delete rhi;
                QVERIFY(h.physDev);
                QRhi::AdapterList filteredList = QRhi::enumerateAdapters(impl, initParams, &h);
                QCOMPARE(filteredList.count(), 1);
                qDeleteAll(filteredList);
            }
#endif
        }
    }

    qDeleteAll(adapters);
    adapters.clear();
}

void tst_QRhi::stats_data()
{
    rhiTestData();
}

void tst_QRhi::stats()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing statistics getter");

    QRhiStats stats = rhi->statistics();
    qDebug() << stats;
    QCOMPARE(stats.totalPipelineCreationTime, 0);

    if (impl == QRhi::Vulkan) {
        QScopedPointer<QRhiBuffer> buf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, 32768));
        QVERIFY(buf->create());
        QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(1024, 1024)));
        QVERIFY(tex->create());

        stats = rhi->statistics();
        qDebug() << stats;
        QVERIFY(stats.allocCount > 0);
        QVERIFY(stats.blockCount > 0);
        QVERIFY(stats.usedBytes > 0);
    }
}

void tst_QRhi::nativeHandles_data()
{
    rhiTestData();
}

void tst_QRhi::nativeHandles()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing native handles");

    // QRhi::nativeHandles()
    {
        const QRhiNativeHandles *rhiHandles = rhi->nativeHandles();
        Q_ASSERT(rhiHandles);

        switch (impl) {
        case QRhi::Null:
            break;
#ifdef TST_VK
        case QRhi::Vulkan:
        {
            const QRhiVulkanNativeHandles *vkHandles = static_cast<const QRhiVulkanNativeHandles *>(rhiHandles);
            QVERIFY(vkHandles->inst);
            QCOMPARE(vkHandles->inst, &vulkanInstance);
            QVERIFY(vkHandles->physDev);
            QVERIFY(vkHandles->dev);
            QVERIFY(vkHandles->gfxQueue);
            QVERIFY(vkHandles->vmemAllocator);
        }
            break;
#endif
#ifdef TST_GL
        case QRhi::OpenGLES2:
        {
            const QRhiGles2NativeHandles *glHandles = static_cast<const QRhiGles2NativeHandles *>(rhiHandles);
            QVERIFY(glHandles->context);
            QVERIFY(glHandles->context->isValid());
            glHandles->context->doneCurrent();
            QVERIFY(!QOpenGLContext::currentContext());
            rhi->makeThreadLocalNativeContextCurrent();
            QVERIFY(QOpenGLContext::currentContext() == glHandles->context);
        }
            break;
#endif
#ifdef TST_D3D11
        case QRhi::D3D11:
        {
            const QRhiD3D11NativeHandles *d3dHandles = static_cast<const QRhiD3D11NativeHandles *>(rhiHandles);
            QVERIFY(d3dHandles->dev);
            QVERIFY(d3dHandles->context);
            QVERIFY(d3dHandles->featureLevel > 0);
            QVERIFY(d3dHandles->adapterLuidLow || d3dHandles->adapterLuidHigh);
        }
            break;
#endif
#ifdef TST_D3D12
        case QRhi::D3D12:
        {
            const QRhiD3D12NativeHandles *d3dHandles = static_cast<const QRhiD3D12NativeHandles *>(rhiHandles);
            QVERIFY(d3dHandles->dev);
            QVERIFY(d3dHandles->minimumFeatureLevel > 0);
            QVERIFY(d3dHandles->adapterLuidLow || d3dHandles->adapterLuidHigh);
            QVERIFY(d3dHandles->commandQueue);
        }
            break;
#endif
#ifdef TST_MTL
        case QRhi::Metal:
        {
            const QRhiMetalNativeHandles *mtlHandles = static_cast<const QRhiMetalNativeHandles *>(rhiHandles);
            QVERIFY(mtlHandles->dev);
            QVERIFY(mtlHandles->cmdQueue);
        }
            break;
#endif
        default:
            Q_ASSERT(false);
        }
    }

    // QRhiCommandBuffer::nativeHandles()
    {
        QRhiCommandBuffer *cb = nullptr;
        QRhi::FrameOpResult result = rhi->beginOffscreenFrame(&cb);
        QVERIFY(result == QRhi::FrameOpSuccess);
        QVERIFY(cb);

        Q_DECL_UNUSED const QRhiNativeHandles *cbHandles = cb->nativeHandles();
        // no null check here, backends where not applicable will return null

        switch (impl) {
        case QRhi::Null:
            break;
#ifdef TST_VK
        case QRhi::Vulkan:
        {
            const QRhiVulkanCommandBufferNativeHandles *vkHandles = static_cast<const QRhiVulkanCommandBufferNativeHandles *>(cbHandles);
            QVERIFY(vkHandles);
            QVERIFY(vkHandles->commandBuffer);
        }
            break;
#endif
#ifdef TST_GL
        case QRhi::OpenGLES2:
            break;
#endif
#ifdef TST_D3D11
        case QRhi::D3D11:
            break;
#endif
#ifdef TST_D3D12
        case QRhi::D3D12:
            break;
#endif
#ifdef TST_MTL
        case QRhi::Metal:
        {
            const QRhiMetalCommandBufferNativeHandles *mtlHandles = static_cast<const QRhiMetalCommandBufferNativeHandles *>(cbHandles);
            QVERIFY(mtlHandles);
            QVERIFY(mtlHandles->commandBuffer);
            QVERIFY(!mtlHandles->encoder);

            QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
            QVERIFY(tex->create());
            QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ tex.data() }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
            QVERIFY(rpDesc);
            rt->setRenderPassDescriptor(rpDesc.data());
            QVERIFY(rt->create());
            cb->beginPass(rt.data(), Qt::red, { 1.0f, 0 });
            QVERIFY(static_cast<const QRhiMetalCommandBufferNativeHandles *>(cb->nativeHandles())->encoder);
            cb->endPass();
        }
            break;
#endif
        default:
            Q_ASSERT(false);
        }

        rhi->endOffscreenFrame();
    }

    // QRhiRenderPassDescriptor::nativeHandles()
    {
        QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
        QVERIFY(tex->create());
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ tex.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        QVERIFY(rpDesc);
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->create());

        switch (impl) {
        case QRhi::Null:
            break;
#ifdef TST_VK
        case QRhi::Vulkan:
        {
            const QRhiNativeHandles *rpHandles = rpDesc->nativeHandles();
            const QRhiVulkanRenderPassNativeHandles *vkHandles = static_cast<const QRhiVulkanRenderPassNativeHandles *>(rpHandles);
            QVERIFY(vkHandles);
            QVERIFY(vkHandles->renderPass);
        }
            break;
#endif
#ifdef TST_GL
        case QRhi::OpenGLES2:
            break;
#endif
#ifdef TST_D3D11
        case QRhi::D3D11:
            break;
#endif
#ifdef TST_D3D12
        case QRhi::D3D12:
            break;
#endif
#ifdef TST_MTL
        case QRhi::Metal:
            break;
#endif
        default:
            Q_ASSERT(false);
        }
    }
}

void tst_QRhi::nativeHandlesImportVulkan()
{
#ifdef TST_VK
    // VkDevice and everything else. For simplicity we'll get QRhi to create one, and then use that with another QRhi.
    {
        QScopedPointer<QRhi> rhi(QRhi::create(QRhi::Vulkan, &initParams.vk, QRhi::Flags(), nullptr));
        if (!rhi)
            QSKIP("Skipping native Vulkan test");

        const QRhiVulkanNativeHandles *nativeHandles = static_cast<const QRhiVulkanNativeHandles *>(rhi->nativeHandles());
        QRhiVulkanNativeHandles h = *nativeHandles;
        // do not pass the rarely used fields, this is useful to test if it creates its own as expected
        h.vmemAllocator = nullptr;

        QScopedPointer<QRhi> adoptingRhi(QRhi::create(QRhi::Vulkan, &initParams.vk, QRhi::Flags(), &h));
        QVERIFY(adoptingRhi);

        const QRhiVulkanNativeHandles *newNativeHandles = static_cast<const QRhiVulkanNativeHandles *>(adoptingRhi->nativeHandles());
        QCOMPARE(newNativeHandles->physDev, nativeHandles->physDev);
        QCOMPARE(newNativeHandles->dev, nativeHandles->dev);
        QCOMPARE(newNativeHandles->gfxQueueFamilyIdx, nativeHandles->gfxQueueFamilyIdx);
        QCOMPARE(newNativeHandles->gfxQueueIdx, nativeHandles->gfxQueueIdx);
        QVERIFY(newNativeHandles->vmemAllocator != nativeHandles->vmemAllocator);
    }

    // Physical device only
    {
        uint32_t physDevCount = 0;
        QVulkanFunctions *f = vulkanInstance.functions();
        f->vkEnumeratePhysicalDevices(vulkanInstance.vkInstance(), &physDevCount, nullptr);
        if (physDevCount < 1)
            QSKIP("No Vulkan physical devices, skip");
        QVarLengthArray<VkPhysicalDevice, 4> physDevs(physDevCount);
        f->vkEnumeratePhysicalDevices(vulkanInstance.vkInstance(), &physDevCount, physDevs.data());

        for (uint32_t i = 0; i < physDevCount; ++i) {
            QRhiVulkanNativeHandles h;
            h.physDev = physDevs[i];
            QScopedPointer<QRhi> rhi(QRhi::create(QRhi::Vulkan, &initParams.vk, QRhi::Flags(), &h));
            // ok if fails, what we want to know is that if it succeeds, it must use that given phys.dev.
            if (!rhi) {
                qWarning("Skipping native Vulkan handle test for physical device %u", i);
                continue;
            }
            const QRhiVulkanNativeHandles *actualNativeHandles = static_cast<const QRhiVulkanNativeHandles *>(rhi->nativeHandles());
            QCOMPARE(actualNativeHandles->physDev, physDevs[i]);
        }
    }

#else
    QSKIP("Skipping Vulkan-specific test");
#endif
}

void tst_QRhi::nativeHandlesImportD3D11()
{
#ifdef TST_D3D11
    QScopedPointer<QRhi> rhi(QRhi::create(QRhi::D3D11, &initParams.d3d11, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing D3D11 native handle import");

    const QRhiD3D11NativeHandles *nativeHandles = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());

    // Case 1: device and context
    {
        QRhiD3D11NativeHandles h = *nativeHandles;
        h.featureLevel = 0; // see if these are queried as expected, even when not provided
        h.adapterLuidLow = 0;
        h.adapterLuidHigh = 0;
        QScopedPointer<QRhi> adoptingRhi(QRhi::create(QRhi::D3D11, &initParams.d3d11, QRhi::Flags(), &h));
        QVERIFY(adoptingRhi);
        const QRhiD3D11NativeHandles *newNativeHandles = static_cast<const QRhiD3D11NativeHandles *>(adoptingRhi->nativeHandles());
        QCOMPARE(newNativeHandles->dev, nativeHandles->dev);
        QCOMPARE(newNativeHandles->context, nativeHandles->context);
        QCOMPARE(newNativeHandles->featureLevel, nativeHandles->featureLevel);
        QCOMPARE(newNativeHandles->adapterLuidLow, nativeHandles->adapterLuidLow);
        QCOMPARE(newNativeHandles->adapterLuidHigh, nativeHandles->adapterLuidHigh);
    }

    // Case 2: adapter and feature level only (hello OpenXR)
    {
        QRhiD3D11NativeHandles h = *nativeHandles;
        h.dev = nullptr;
        h.context = nullptr;
        QScopedPointer<QRhi> adoptingRhi(QRhi::create(QRhi::D3D11, &initParams.d3d11, QRhi::Flags(), &h));
        QVERIFY(adoptingRhi);
        const QRhiD3D11NativeHandles *newNativeHandles = static_cast<const QRhiD3D11NativeHandles *>(adoptingRhi->nativeHandles());
        QVERIFY(newNativeHandles->dev != nativeHandles->dev);
        QVERIFY(newNativeHandles->context != nativeHandles->context);
        QCOMPARE(newNativeHandles->featureLevel, nativeHandles->featureLevel);
        QCOMPARE(newNativeHandles->adapterLuidLow, nativeHandles->adapterLuidLow);
        QCOMPARE(newNativeHandles->adapterLuidHigh, nativeHandles->adapterLuidHigh);
    }

#else
    QSKIP("Skipping D3D11-specific test");
#endif
}

void tst_QRhi::nativeHandlesImportOpenGL()
{
#ifdef TST_GL
    QRhiGles2NativeHandles h;
    QScopedPointer<QOpenGLContext> ctx(new QOpenGLContext);
    if (!ctx->create())
        QSKIP("No OpenGL context, skipping OpenGL-specific test");
    h.context = ctx.data();
    QScopedPointer<QRhi> rhi(QRhi::create(QRhi::OpenGLES2, &initParams.gl, QRhi::Flags(), &h));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing OpenGL native handle import");

    const QRhiGles2NativeHandles *actualNativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    QCOMPARE(actualNativeHandles->context, ctx.data());

    rhi->makeThreadLocalNativeContextCurrent();
    QCOMPARE(QOpenGLContext::currentContext(), ctx.data());
#else
    QSKIP("Skipping OpenGL-specific test");
#endif
}

void tst_QRhi::nativeTexture_data()
{
    rhiTestData();
}

void tst_QRhi::nativeTexture()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing native texture");

    QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 256)));
    QVERIFY(tex->create());

    const QRhiTexture::NativeTexture nativeTex = tex->nativeTexture();

    switch (impl) {
    case QRhi::Null:
        break;
#ifdef TST_VK
    case QRhi::Vulkan:
    {
        auto image = VkImage(nativeTex.object);
        QVERIFY(image);
        QVERIFY(nativeTex.layout >= 1); // VK_IMAGE_LAYOUT_GENERAL
        QVERIFY(nativeTex.layout <= 8); // VK_IMAGE_LAYOUT_PREINITIALIZED
    }
        break;
#endif
#ifdef TST_GL
    case QRhi::OpenGLES2:
    {
        auto textureId = uint(nativeTex.object);
        QVERIFY(textureId);
    }
        break;
#endif
#ifdef TST_D3D11
    case QRhi::D3D11:
    {
        auto *texture = reinterpret_cast<void *>(nativeTex.object);
        QVERIFY(texture);
    }
        break;
#endif
#ifdef TST_D3D12
    case QRhi::D3D12:
    {
        auto *texture = reinterpret_cast<void *>(nativeTex.object);
        QVERIFY(texture);
    }
        break;
#endif
#ifdef TST_MTL
    case QRhi::Metal:
    {
        auto texture = (void *)nativeTex.object;
        QVERIFY(texture);
    }
        break;
#endif
    default:
        Q_ASSERT(false);
    }
}

void tst_QRhi::nativeBuffer_data()
{
    rhiTestData();
}

void tst_QRhi::nativeBuffer()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing native buffer query");

    const QRhiBuffer::Type types[3] = { QRhiBuffer::Immutable, QRhiBuffer::Static, QRhiBuffer::Dynamic };
    const QRhiBuffer::UsageFlags usages[3] = { QRhiBuffer::VertexBuffer, QRhiBuffer::IndexBuffer, QRhiBuffer::UniformBuffer };
    for (int typeUsageIdx = 0; typeUsageIdx < 3; ++typeUsageIdx) {
        QScopedPointer<QRhiBuffer> buf(rhi->newBuffer(types[typeUsageIdx], usages[typeUsageIdx], 256));
        QVERIFY(buf->create());

        const QRhiBuffer::NativeBuffer nativeBuf = buf->nativeBuffer();
        QVERIFY(nativeBuf.slotCount <= rhi->resourceLimit(QRhi::FramesInFlight));

        switch (impl) {
        case QRhi::Null:
            break;
    #ifdef TST_VK
        case QRhi::Vulkan:
        {
            QVERIFY(nativeBuf.slotCount >= 1); // always backed by native buffers
            for (int i = 0; i < nativeBuf.slotCount; ++i) {
                auto *buffer = static_cast<const VkBuffer *>(nativeBuf.objects[i]);
                QVERIFY(buffer);
                QVERIFY(*buffer);
            }
        }
            break;
    #endif
    #ifdef TST_GL
        case QRhi::OpenGLES2:
        {
            QVERIFY(nativeBuf.slotCount >= 0); // UniformBuffers are not backed by native buffers, so 0 is perfectly valid
            for (int i = 0; i < nativeBuf.slotCount; ++i) {
                auto *bufferId = static_cast<const uint *>(nativeBuf.objects[i]);
                QVERIFY(bufferId);
                QVERIFY(*bufferId);
            }
        }
            break;
    #endif
    #ifdef TST_D3D11
        case QRhi::D3D11:
        {
            QVERIFY(nativeBuf.slotCount >= 1); // always backed by native buffers
            for (int i = 0; i < nativeBuf.slotCount; ++i) {
                auto *buffer = static_cast<void * const *>(nativeBuf.objects[i]);
                QVERIFY(buffer);
                QVERIFY(*buffer);
            }
        }
            break;
    #endif
    #ifdef TST_D3D12
        case QRhi::D3D12:
        {
            QVERIFY(nativeBuf.slotCount >= 1); // always backed by native buffers
            for (int i = 0; i < nativeBuf.slotCount; ++i) {
                auto *buffer = static_cast<void * const *>(nativeBuf.objects[i]);
                QVERIFY(buffer);
                QVERIFY(*buffer);
            }
        }
            break;
    #endif
    #ifdef TST_MTL
        case QRhi::Metal:
        {
            QVERIFY(nativeBuf.slotCount >= 1); // always backed by native buffers
            for (int i = 0; i < nativeBuf.slotCount; ++i) {
                void * const * buffer = (void * const *) nativeBuf.objects[i];
                QVERIFY(buffer);
                QVERIFY(*buffer);
            }
        }
            break;
    #endif
        default:
            Q_ASSERT(false);
        }
    }
}

static bool submitResourceUpdates(QRhi *rhi, QRhiResourceUpdateBatch *batch)
{
    QRhiCommandBuffer *cb = nullptr;
    QRhi::FrameOpResult result = rhi->beginOffscreenFrame(&cb);
    if (result != QRhi::FrameOpSuccess) {
        qWarning("beginOffscreenFrame returned %d", result);
        return false;
    }
    if (!cb) {
        qWarning("No command buffer from beginOffscreenFrame");
        return false;
    }
    cb->resourceUpdate(batch);
    rhi->endOffscreenFrame();
    return true;
}

void tst_QRhi::resourceUpdateBatchBuffer_data()
{
    rhiTestData();
}

void tst_QRhi::resourceUpdateBatchBuffer()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing buffer resource updates");

    const int bufferSize = 23;
    const QByteArray a(bufferSize, 'A');
    const QByteArray b(bufferSize, 'B');

    // dynamic buffer, updates, readback
    {
        QScopedPointer<QRhiBuffer> dynamicBuffer(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, bufferSize));
        QVERIFY(dynamicBuffer->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        batch->updateDynamicBuffer(dynamicBuffer.data(), 10, bufferSize - 10, a.constData());
        batch->updateDynamicBuffer(dynamicBuffer.data(), 0, 12, b.constData());

        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };
        batch->readBackBuffer(dynamicBuffer.data(), 5, 10, &readResult);

        QVERIFY(submitResourceUpdates(rhi.data(), batch));

        // Offscreen frames are synchronous, so the readback must have
        // completed at this point. With swapchain frames this would not be the
        // case.
        QVERIFY(readCompleted);
        QVERIFY(readResult.data.size() == 10);
        QCOMPARE(readResult.data.left(7), QByteArrayLiteral("BBBBBBB"));
        QCOMPARE(readResult.data.mid(7), QByteArrayLiteral("AAA"));
    }

    // static buffer, updates, readback
    {
        QScopedPointer<QRhiBuffer> dynamicBuffer(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::VertexBuffer, bufferSize));
        QVERIFY(dynamicBuffer->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        batch->uploadStaticBuffer(dynamicBuffer.data(), 10, bufferSize - 10, a.constData());
        batch->uploadStaticBuffer(dynamicBuffer.data(), 0, 12, b.constData());

        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };

        if (rhi->isFeatureSupported(QRhi::ReadBackNonUniformBuffer))
            batch->readBackBuffer(dynamicBuffer.data(), 5, 10, &readResult);
        else
            qDebug("Skipping verification of buffer data as ReadBackNonUniformBuffer is not supported");

        QVERIFY(submitResourceUpdates(rhi.data(), batch));

        if (rhi->isFeatureSupported(QRhi::ReadBackNonUniformBuffer)) {
            QVERIFY(readCompleted);
            QVERIFY(readResult.data.size() == 10);
            QCOMPARE(readResult.data.left(7), QByteArrayLiteral("BBBBBBB"));
            QCOMPARE(readResult.data.mid(7), QByteArrayLiteral("AAA"));
        } else {
            qDebug("Skipping verifying buffer contents because readback is not supported");
        }
    }
}

inline bool imageRGBAEquals(const QImage &a, const QImage &b, int maxFuzz = 1)
{
    if (a.size() != b.size())
        return false;

    const QImage image0 = a.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    const QImage image1 = b.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

    const int width = image0.width();
    const int height = image0.height();
    for (int y = 0; y < height; ++y) {
        const quint32 *p0 = reinterpret_cast<const quint32 *>(image0.constScanLine(y));
        const quint32 *p1 = reinterpret_cast<const quint32 *>(image1.constScanLine(y));
        int x = width - 1;
        while (x-- >= 0) {
            const QRgb c0(*p0++);
            const QRgb c1(*p1++);
            const int red = qAbs(qRed(c0) - qRed(c1));
            const int green = qAbs(qGreen(c0) - qGreen(c1));
            const int blue = qAbs(qBlue(c0) - qBlue(c1));
            const int alpha = qAbs(qAlpha(c0) - qAlpha(c1));
            if (red > maxFuzz || green > maxFuzz || blue > maxFuzz || alpha > maxFuzz)
                return false;
        }
    }

    return true;
}

void tst_QRhi::resourceUpdateBatchRGBATextureUpload_data()
{
    rhiTestData();
}

void tst_QRhi::resourceUpdateBatchRGBATextureUpload()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing texture resource updates");

    QImage image(234, 123, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::red);
    QPainter painter;
    const QPoint greenRectPos(35, 50);
    const QSize greenRectSize(100, 50);
    painter.begin(&image);
    painter.fillRect(QRect(greenRectPos, greenRectSize), Qt::green);
    painter.end();

    // simple image upload; uploading and reading back RGBA8 is supported by the Null backend even
    {
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, image.size(),
                                                            1, QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        batch->uploadTexture(texture.data(), image);

        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };
        batch->readBackTexture(texture.data(), &readResult);

        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        // like with buffers, the readback is now complete due to endOffscreenFrame()
        QVERIFY(readCompleted);
        QCOMPARE(readResult.format, QRhiTexture::RGBA8);
        QCOMPARE(readResult.pixelSize, image.size());

        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            image.format());

        QVERIFY(imageRGBAEquals(image, wrapperImage));
    }

    // the same with raw data
    {
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, image.size(),
                                                            1, QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();

        QRhiTextureUploadEntry upload(0, 0, { image.constBits(), quint32(image.sizeInBytes()) });
        QRhiTextureUploadDescription uploadDesc(upload);
        batch->uploadTexture(texture.data(), uploadDesc);

        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };
        batch->readBackTexture(texture.data(), &readResult);

        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(readCompleted);
        QCOMPARE(readResult.format, QRhiTexture::RGBA8);
        QCOMPARE(readResult.pixelSize, image.size());

        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            image.format());

        QVERIFY(imageRGBAEquals(image, wrapperImage));
    }

    // partial image upload at a non-zero destination position
    {
        const QSize copySize(30, 40);
        const int gap = 10;
        const QSize fullSize(copySize.width() + gap, copySize.height() + gap);
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, fullSize,
                                                            1, QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();

        QImage clearImage(fullSize, image.format());
        clearImage.fill(Qt::black);
        batch->uploadTexture(texture.data(), clearImage);

        // copy green pixels of copySize to (gap, gap), leaving a black bar of
        // gap pixels on the left and top
        QRhiTextureSubresourceUploadDescription desc;
        desc.setImage(image);
        desc.setSourceSize(copySize);
        desc.setDestinationTopLeft(QPoint(gap, gap));
        desc.setSourceTopLeft(greenRectPos);

        batch->uploadTexture(texture.data(), QRhiTextureUploadDescription({ 0, 0, desc }));

        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };
        batch->readBackTexture(texture.data(), &readResult);

        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(readCompleted);
        QCOMPARE(readResult.format, QRhiTexture::RGBA8);
        QCOMPARE(readResult.pixelSize, clearImage.size());

        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            image.format());

        QVERIFY(!imageRGBAEquals(clearImage, wrapperImage));

        QImage expectedImage = clearImage;
        QPainter painter(&expectedImage);
        painter.fillRect(QRect(QPoint(gap, gap), QSize(copySize)), Qt::green);
        painter.end();

        QVERIFY(imageRGBAEquals(expectedImage, wrapperImage));
    }

    // the same (partial upload) with raw data as source
    {
        const QSize copySize(30, 40);
        const int gap = 10;
        const QSize fullSize(copySize.width() + gap, copySize.height() + gap);
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, fullSize,
                                                            1, QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();

        QImage clearImage(fullSize, image.format());
        clearImage.fill(Qt::black);
        batch->uploadTexture(texture.data(), clearImage);

        // SourceTopLeft is not supported for non-QImage-based uploads.
        const QImage im = image.copy(QRect(greenRectPos, copySize));
        QRhiTextureSubresourceUploadDescription desc;
        desc.setData(QByteArray::fromRawData(reinterpret_cast<const char *>(im.constBits()), im.sizeInBytes()));

        desc.setSourceSize(copySize);
        desc.setDestinationTopLeft(QPoint(gap, gap));

        batch->uploadTexture(texture.data(), QRhiTextureUploadDescription({ 0, 0, desc }));

        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };
        batch->readBackTexture(texture.data(), &readResult);

        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(readCompleted);
        QCOMPARE(readResult.format, QRhiTexture::RGBA8);
        QCOMPARE(readResult.pixelSize, clearImage.size());

        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            image.format());

        QVERIFY(!imageRGBAEquals(clearImage, wrapperImage));

        QImage expectedImage = clearImage;
        QPainter painter(&expectedImage);
        painter.fillRect(QRect(QPoint(gap, gap), QSize(copySize)), Qt::green);
        painter.end();

        QVERIFY(imageRGBAEquals(expectedImage, wrapperImage));
    }

    // now a QImage from an actual file
    {
        QImage inputImage;
        inputImage.load(QLatin1String(":/data/qt256.png"));
        QVERIFY(!inputImage.isNull());
        inputImage = std::move(inputImage).convertToFormat(image.format());

        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(),
                                                            1, QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        batch->uploadTexture(texture.data(), inputImage);

        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };
        batch->readBackTexture(texture.data(), &readResult);

        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(readCompleted);
        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            inputImage.format());

        QVERIFY(imageRGBAEquals(inputImage, wrapperImage));
    }
}

void tst_QRhi::resourceUpdateBatchRGBATextureCopy_data()
{
    rhiTestData();
}

void tst_QRhi::resourceUpdateBatchRGBATextureCopy()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing texture resource updates");

    QImage red(256, 256, QImage::Format_RGBA8888_Premultiplied);
    red.fill(Qt::red);

    QImage green(35, 73, red.format());
    green.fill(Qt::green);

    QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiTexture> redTexture(rhi->newTexture(QRhiTexture::RGBA8, red.size(),
                                                           1, QRhiTexture::UsedAsTransferSource));
    QVERIFY(redTexture->create());
    batch->uploadTexture(redTexture.data(), red);

    QScopedPointer<QRhiTexture> greenTexture(rhi->newTexture(QRhiTexture::RGBA8, green.size(),
                                                             1, QRhiTexture::UsedAsTransferSource));
    QVERIFY(greenTexture->create());
    batch->uploadTexture(greenTexture.data(), green);

    // 1. simple copy red -> texture; 2. subimage copy green -> texture; 3. partial subimage copy green -> texture
    {
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, red.size(),
                                                            1, QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        // 1.
        batch->copyTexture(texture.data(), redTexture.data());

        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };
        batch->readBackTexture(texture.data(), &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(readCompleted);
        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            red.format());
        QVERIFY(imageRGBAEquals(red, wrapperImage));

        batch = rhi->nextResourceUpdateBatch();
        readCompleted = false;

        // 2.
        QRhiTextureCopyDescription copyDesc;
        copyDesc.setDestinationTopLeft(QPoint(15, 23));
        batch->copyTexture(texture.data(), greenTexture.data(), copyDesc);

        batch->readBackTexture(texture.data(), &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(readCompleted);
        wrapperImage = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                              readResult.pixelSize.width(), readResult.pixelSize.height(),
                              red.format());

        QImage expectedImage = red;
        QPainter painter(&expectedImage);
        painter.drawImage(copyDesc.destinationTopLeft(), green);
        painter.end();

        QVERIFY(imageRGBAEquals(expectedImage, wrapperImage));

        batch = rhi->nextResourceUpdateBatch();
        readCompleted = false;

        // 3.
        copyDesc.setDestinationTopLeft(QPoint(125, 89));
        copyDesc.setSourceTopLeft(QPoint(5, 5));
        copyDesc.setPixelSize(QSize(26, 45));
        batch->copyTexture(texture.data(), greenTexture.data(), copyDesc);

        batch->readBackTexture(texture.data(), &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(readCompleted);
        wrapperImage = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                              readResult.pixelSize.width(), readResult.pixelSize.height(),
                              red.format());

        painter.begin(&expectedImage);
        painter.drawImage(copyDesc.destinationTopLeft(), green,
                          QRect(copyDesc.sourceTopLeft(), copyDesc.pixelSize()));
        painter.end();

        QVERIFY(imageRGBAEquals(expectedImage, wrapperImage));
    }
}

void tst_QRhi::resourceUpdateBatchRGBATextureMip_data()
{
    rhiTestData();
}

void tst_QRhi::resourceUpdateBatchRGBATextureMip()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing texture resource updates");


    QImage red(512, 512, QImage::Format_RGBA8888_Premultiplied);
    red.fill(Qt::red);

    const QRhiTexture::Flags textureFlags =
            QRhiTexture::UsedAsTransferSource
            | QRhiTexture::MipMapped
            | QRhiTexture::UsedWithGenerateMips;
    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, red.size(), 1, textureFlags));
    QVERIFY(texture->create());

    QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
    batch->uploadTexture(texture.data(), red);
    batch->generateMips(texture.data());
    QVERIFY(submitResourceUpdates(rhi.data(), batch));

    const int levelCount = rhi->mipLevelsForSize(red.size());
    QCOMPARE(levelCount, 10);
    for (int level = 0; level < levelCount; ++level) {
        batch = rhi->nextResourceUpdateBatch();

        QRhiReadbackDescription readDesc(texture.data());
        readDesc.setLevel(level);
        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };
        batch->readBackTexture(readDesc, &readResult);

        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(readCompleted);

        const QSize expectedSize = rhi->sizeForMipLevel(level, texture->pixelSize());
        QCOMPARE(readResult.pixelSize, expectedSize);

        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            red.format());
        QImage expectedImage;
        if (level == 0 || rhi->isFeatureSupported(QRhi::ReadBackNonBaseMipLevel)) {
            // Compare to a scaled version; we can do this safely only because we
            // only have plain red pixels in the source image.
            expectedImage = red.scaled(expectedSize);
        } else {
            qDebug("Expecting all-zero image for level %d because reading back a level other than 0 is not supported", level);
            expectedImage = QImage(readResult.pixelSize, red.format());
            expectedImage.fill(0);
        }
        QVERIFY(imageRGBAEquals(expectedImage, wrapperImage));
    }
}

void tst_QRhi::resourceUpdateBatchTextureRawDataStride_data()
{
    rhiTestData();
}

void tst_QRhi::resourceUpdateBatchTextureRawDataStride()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing texture resource updates");

    const int WIDTH = 150;
    const int DATA_WIDTH = 180;
    const int HEIGHT = 50;
    QByteArray image;
    image.resize(DATA_WIDTH * HEIGHT * 4);
    for (int y = 0; y < HEIGHT; ++y) {
        char *p = image.data() + y * DATA_WIDTH * 4;
        memset(p, y, DATA_WIDTH * 4);
    }

    {
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(WIDTH, HEIGHT),
                                                            1, QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();

        QRhiTextureSubresourceUploadDescription subresDesc(image.constData(), image.size());
        subresDesc.setDataStride(DATA_WIDTH * 4);
        QRhiTextureUploadEntry upload(0, 0, subresDesc);
        QRhiTextureUploadDescription uploadDesc(upload);
        batch->uploadTexture(texture.data(), uploadDesc);

        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };
        batch->readBackTexture(texture.data(), &readResult);

        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(readCompleted);
        QCOMPARE(readResult.format, QRhiTexture::RGBA8);
        QCOMPARE(readResult.pixelSize, QSize(WIDTH, HEIGHT));

        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888_Premultiplied);
        // wrap the original data, note the bytesPerLine argument
        QImage originalWrapperImage(reinterpret_cast<const uchar *>(image.constData()),
                                    WIDTH, HEIGHT, DATA_WIDTH * 4,
                                    QImage::Format_RGBA8888_Premultiplied);
        QVERIFY(imageRGBAEquals(wrapperImage, originalWrapperImage));
    }
}

void tst_QRhi::resourceUpdateBatchLotsOfResources_data()
{
    rhiTestData();
}

void tst_QRhi::resourceUpdateBatchLotsOfResources()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing resource updates");

    QImage image(128, 128, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::red);
    static const float bufferData[64] = {};

    QRhiResourceUpdateBatch *b = rhi->nextResourceUpdateBatch();
    std::vector<std::unique_ptr<QRhiTexture>> textures;
    std::vector<std::unique_ptr<QRhiBuffer>> buffers;

    // QTBUG-96619
    static const int TEXTURE_COUNT = 3 * QRhiResourceUpdateBatchPrivate::TEXTURE_OPS_STATIC_ALLOC;
    static const int BUFFER_COUNT = 3 * QRhiResourceUpdateBatchPrivate::BUFFER_OPS_STATIC_ALLOC;

    for (int i = 0; i < TEXTURE_COUNT; ++i) {
        std::unique_ptr<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, image.size()));
        QVERIFY(texture->create());
        b->uploadTexture(texture.get(), image);
        textures.push_back(std::move(texture));
    }

    for (int i = 0; i < BUFFER_COUNT; ++i) {
        std::unique_ptr<QRhiBuffer> buffer(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, 256));
        QVERIFY(buffer->create());
        b->uploadStaticBuffer(buffer.get(), bufferData);
        buffers.push_back(std::move(buffer));
    }

    submitResourceUpdates(rhi.data(), b);
}

void tst_QRhi::resourceUpdateBatchBetweenFrames_data()
{
    rhiTestData();
}

void tst_QRhi::resourceUpdateBatchBetweenFrames()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing resource updates");

    QImage image(128, 128, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::red);
    static const float bufferData[64] = {};

    QRhiCommandBuffer *cb = nullptr;
    QRhi::FrameOpResult result = rhi->beginOffscreenFrame(&cb);
    QVERIFY(result == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    static const int TEXTURE_COUNT = 123;
    static const int BUFFER_COUNT = 456;

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    std::vector<std::unique_ptr<QRhiTexture>> textures;
    std::vector<std::unique_ptr<QRhiBuffer>> buffers;

    for (int i = 0; i < TEXTURE_COUNT; ++i) {
        std::unique_ptr<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8,
                                                             image.size(),
                                                             1,
                                                             QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());
        u->uploadTexture(texture.get(), image);
        textures.push_back(std::move(texture));
    }

    for (int i = 0; i < BUFFER_COUNT; ++i) {
        std::unique_ptr<QRhiBuffer> buffer(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, 256));
        QVERIFY(buffer->create());
        u->uploadStaticBuffer(buffer.get(), bufferData);
        buffers.push_back(std::move(buffer));
    }

    rhi->endOffscreenFrame();
    cb = nullptr;

    // 'u' stays valid, commit it in another frame

    result = rhi->beginOffscreenFrame(&cb);
    QVERIFY(result == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    cb->resourceUpdate(u); // this should work

    rhi->endOffscreenFrame();

    u = rhi->nextResourceUpdateBatch();
    QRhiReadbackResult readResult;
    bool readCompleted = false;
    readResult.completed = [&readCompleted] { readCompleted = true; };
    u->readBackTexture(textures[5].get(), &readResult);

    QVERIFY(submitResourceUpdates(rhi.data(), u));
    QVERIFY(readCompleted);
    QCOMPARE(readResult.format, QRhiTexture::RGBA8);
    QCOMPARE(readResult.pixelSize, image.size());

    QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x)
            QCOMPARE(wrapperImage.pixel(x, y), qRgba(255, 0, 0, 255));
    }
}

static QShader loadShader(const char *name)
{
    QFile f(QString::fromUtf8(name));
    if (f.open(QIODevice::ReadOnly)) {
        const QByteArray contents = f.readAll();
        return QShader::fromSerialized(contents);
    }
    return QShader();
}

void tst_QRhi::invalidPipeline_data()
{
    rhiTestData();
}

void tst_QRhi::invalidPipeline()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing empty shader");

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(256, 256), 1, QRhiTexture::RenderTarget));
    QVERIFY(texture->create());
    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 2 * sizeof(float) } });
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float2, 0 } });

    // no stages
    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(!pipeline->create());

    QShader vs;
    QShader fs;

    // no shaders in the stages
    pipeline.reset(rhi->newGraphicsPipeline());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(!pipeline->create());

    vs = loadShader(":/data/simple.vert.qsb");
    QVERIFY(vs.isValid());
    fs = loadShader(":/data/simple.frag.qsb");
    QVERIFY(fs.isValid());

    // no vertex stage
    pipeline.reset(rhi->newGraphicsPipeline());
    pipeline->setShaderStages({ { QRhiShaderStage::Fragment, fs } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(!pipeline->create());

    // no renderpass descriptor
    pipeline.reset(rhi->newGraphicsPipeline());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    QVERIFY(!pipeline->create());

    // no shader resource bindings
    pipeline.reset(rhi->newGraphicsPipeline());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(!pipeline->create());

    // correct
    pipeline.reset(rhi->newGraphicsPipeline());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setRenderPassDescriptor(rpDesc.data());
    pipeline->setShaderResourceBindings(srb.data());
    QVERIFY(pipeline->create());
}

void tst_QRhi::renderToTextureSimple_data()
{
    rhiTestData();
}

static QRhiGraphicsPipeline *createSimplePipeline(QRhi *rhi, QRhiShaderResourceBindings *srb, QRhiRenderPassDescriptor *rpDesc)
{
    std::unique_ptr<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    QShader vs = loadShader(":/data/simple.vert.qsb");
    if (!vs.isValid())
        return nullptr;
    QShader fs = loadShader(":/data/simple.frag.qsb");
    if (!fs.isValid())
        return nullptr;
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 2 * sizeof(float) } });
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float2, 0 } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb);
    pipeline->setRenderPassDescriptor(rpDesc);
    return pipeline->create() ? pipeline.release() : nullptr;
}

static const float triangleVertices[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    0.0f, 1.0f
};

void tst_QRhi::renderToTextureSimple()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    const QSize outputSize(1920, 1080);
    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, outputSize, 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createSimplePipeline(rhi.data(), srb.data(), rpDesc.data()));
    QVERIFY(pipeline);

    cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied); // non-owning, no copy needed because readResult outlives result
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();
    // Offscreen frames are synchronous, so the readback is guaranteed to
    // complete at this point. This would not be the case with swapchain-based
    // frames.
    QCOMPARE(result.size(), texture->pixelSize());

    if (impl == QRhi::Null)
        return;

    // Now we have a red rectangle on blue background.
    const int y = 100;
    const quint32 *p = reinterpret_cast<const quint32 *>(result.constScanLine(y));
    int x = result.width() - 1;
    int redCount = 0;
    int blueCount = 0;
    const int maxFuzz = 1;
    while (x-- >= 0) {
        const QRgb c(*p++);
        if (qRed(c) >= (255 - maxFuzz) && qGreen(c) == 0 && qBlue(c) == 0)
            ++redCount;
        else if (qRed(c) == 0 && qGreen(c) == 0 && qBlue(c) >= (255 - maxFuzz))
            ++blueCount;
        else
            QFAIL("Encountered a pixel that is neither red or blue");
    }

    QCOMPARE(redCount + blueCount, texture->pixelSize().width());

    // The triangle is "pointing up" in the resulting image with OpenGL
    // (because Y is up both in normalized device coordinates and in images)
    // and Vulkan (because Y is down in both and the vertex data was specified
    // with Y up in mind), but "pointing down" with D3D (because Y is up in NDC
    // but down in images).
    if (rhi->isYUpInFramebuffer() == rhi->isYUpInNDC())
        QVERIFY(redCount < blueCount);
    else
        QVERIFY(redCount > blueCount);
}

void tst_QRhi::renderToTextureMip_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureMip()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    if (!rhi->isFeatureSupported(QRhi::RenderToNonBaseMipLevel))
        QSKIP("Rendering to non-base mip levels is not supported on this platform, skipping test");

    const QSize baseLevelSize(1024, 1024);
    const int LEVEL = 3; // render into mip #3 (128x128)
    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, baseLevelSize, 1,
                                                        QRhiTexture::RenderTarget
                                                        | QRhiTexture::UsedAsTransferSource
                                                        | QRhiTexture::MipMapped));
    QVERIFY(texture->create());

    QRhiColorAttachment colorAtt(texture.data());
    colorAtt.setLevel(LEVEL);
    QRhiTextureRenderTargetDescription rtDesc(colorAtt);
    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QCOMPARE(rt->pixelSize(), rhi->sizeForMipLevel(LEVEL, baseLevelSize));
    const QSize mipSize(baseLevelSize.width() >> LEVEL, baseLevelSize.height() >> LEVEL);
    QCOMPARE(rt->pixelSize(), mipSize);

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createSimplePipeline(rhi.data(), srb.data(), rpDesc.data()));
    QVERIFY(pipeline);

    cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(rt->pixelSize().width()), float(rt->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    QRhiReadbackDescription readbackDescription(texture.data());
    readbackDescription.setLevel(LEVEL);
    readbackBatch->readBackTexture(readbackDescription, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    if (!rhi->isFeatureSupported(QRhi::ReadBackNonBaseMipLevel))
        QSKIP("Reading back non-base mip levels is not supported on this platform, skipping readback");

    QCOMPARE(result.size(), mipSize);

    if (impl == QRhi::Null)
        return;

    const int y = 100;
    const quint32 *p = reinterpret_cast<const quint32 *>(result.constScanLine(y));
    int x = result.width() - 1;
    int redCount = 0;
    int blueCount = 0;
    const int maxFuzz = 1;
    while (x-- >= 0) {
        const QRgb c(*p++);
        if (qRed(c) >= (255 - maxFuzz) && qGreen(c) == 0 && qBlue(c) == 0)
            ++redCount;
        else if (qRed(c) == 0 && qGreen(c) == 0 && qBlue(c) >= (255 - maxFuzz))
            ++blueCount;
        else
            QFAIL("Encountered a pixel that is neither red or blue");
    }

    QCOMPARE(redCount + blueCount, mipSize.width());

    if (rhi->isYUpInFramebuffer() == rhi->isYUpInNDC())
        QVERIFY(redCount > blueCount); // 100, 28
    else
        QVERIFY(redCount < blueCount); // 28, 100
}

void tst_QRhi::renderToTextureCubemapFace_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureCubemapFace()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    const QSize outputSize(512, 512); // width must be same as height
    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, outputSize, 1,
                                                        QRhiTexture::RenderTarget
                                                        | QRhiTexture::UsedAsTransferSource
                                                        | QRhiTexture::CubeMap)); // will be a cubemap, so 6 layers
    QVERIFY(texture->create());

    const int LAYER = 1; // render into the layer for face -X
    const int BAD_LAYER = 2; // +Y

    QRhiColorAttachment colorAtt(texture.data());
    colorAtt.setLayer(LAYER);
    QRhiTextureRenderTargetDescription rtDesc(colorAtt);
    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QCOMPARE(rt->pixelSize(), texture->pixelSize());
    QCOMPARE(rt->pixelSize(), outputSize);

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createSimplePipeline(rhi.data(), srb.data(), rpDesc.data()));
    QVERIFY(pipeline);

    cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(rt->pixelSize().width()), float(rt->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    QRhiReadbackDescription readbackDescription(texture.data());
    readbackDescription.setLayer(LAYER);
    readbackBatch->readBackTexture(readbackDescription, &readResult);

    // also read back a layer we did not render into
    QRhiReadbackResult readResult2;
    QImage result2;
    readResult2.completed = [&readResult2, &result2] {
        result2 = QImage(reinterpret_cast<const uchar *>(readResult2.data.constData()),
                         readResult2.pixelSize.width(), readResult2.pixelSize.height(),
                         QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiReadbackDescription readbackDescription2(texture.data());
    readbackDescription2.setLayer(BAD_LAYER);
    readbackBatch->readBackTexture(readbackDescription2, &readResult2);

    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QCOMPARE(result.size(), outputSize);
    QCOMPARE(result2.size(), outputSize);

    if (impl == QRhi::Null)
        return;

    // just want to ensure that we did not read the same thing back twice, i.e.
    // that the 'layer' parameter was not ignored
    QVERIFY(result != result2);

    const int y = 100;
    const quint32 *p = reinterpret_cast<const quint32 *>(result.constScanLine(y));
    int x = result.width() - 1;
    int redCount = 0;
    int blueCount = 0;
    const int maxFuzz = 1;
    while (x-- >= 0) {
        const QRgb c(*p++);
        if (qRed(c) >= (255 - maxFuzz) && qGreen(c) == 0 && qBlue(c) == 0)
            ++redCount;
        else if (qRed(c) == 0 && qGreen(c) == 0 && qBlue(c) >= (255 - maxFuzz))
            ++blueCount;
        else
            QFAIL("Encountered a pixel that is neither red or blue");
    }

    QVERIFY(redCount > 0 && blueCount > 0);
    QCOMPARE(redCount + blueCount, outputSize.width());

    if (rhi->isYUpInFramebuffer() == rhi->isYUpInNDC())
        QVERIFY(redCount < blueCount); // 100, 412
    else
        QVERIFY(redCount > blueCount); // 412, 100
}

void tst_QRhi::renderToTextureTextureArray_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureTextureArray()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    if (!rhi->isFeatureSupported(QRhi::TextureArrays))
        QSKIP("TextureArrays is not supported with this backend, skipping test");

    if (isAndroidOpenGLSwiftShader(impl, rhi.get())) {
        QSKIP("SwiftShader software acceleration is used which does not support this OpenGLES "
              "feature. See QTBUG-132934");
    }

    const QSize outputSize(512, 256);
    const int ARRAY_SIZE = 8;
    QScopedPointer<QRhiTexture> texture(rhi->newTextureArray(QRhiTexture::RGBA8,
                                                             ARRAY_SIZE,
                                                             outputSize,
                                                             1,
                                                             QRhiTexture::RenderTarget
                                                             | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    const int LAYER = 5; // render into element #5

    QRhiColorAttachment colorAtt(texture.data());
    colorAtt.setLayer(LAYER);
    QRhiTextureRenderTargetDescription rtDesc(colorAtt);
    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QCOMPARE(rt->pixelSize(), texture->pixelSize());
    QCOMPARE(rt->pixelSize(), outputSize);

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createSimplePipeline(rhi.data(), srb.data(), rpDesc.data()));
    QVERIFY(pipeline);

    cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(rt->pixelSize().width()), float(rt->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    QRhiReadbackDescription readbackDescription(texture.data());
    readbackDescription.setLayer(LAYER);
    readbackBatch->readBackTexture(readbackDescription, &readResult);

    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QCOMPARE(result.size(), outputSize);

    if (impl == QRhi::Null)
        return;

    const int y = 100;
    const quint32 *p = reinterpret_cast<const quint32 *>(result.constScanLine(y));
    int x = result.width() - 1;
    int redCount = 0;
    int blueCount = 0;
    const int maxFuzz = 1;
    while (x-- >= 0) {
        const QRgb c(*p++);
        if (qRed(c) >= (255 - maxFuzz) && qGreen(c) == 0 && qBlue(c) == 0)
            ++redCount;
        else if (qRed(c) == 0 && qGreen(c) == 0 && qBlue(c) >= (255 - maxFuzz))
            ++blueCount;
        else
            QFAIL("Encountered a pixel that is neither red or blue");
    }

    QVERIFY(redCount > 0 && blueCount > 0);
    QCOMPARE(redCount + blueCount, outputSize.width());

    if (rhi->isYUpInFramebuffer() == rhi->isYUpInNDC())
        QVERIFY(redCount < blueCount); // 100, 412
    else
        QVERIFY(redCount > blueCount); // 412, 100
}

static const float quadVerticesUvs[] = {
    -1.0f, -1.0f,   0.0f, 0.0f,
    1.0f, -1.0f,    1.0f, 0.0f,
    -1.0f, 1.0f,    0.0f, 1.0f,
    1.0f, 1.0f,     1.0f, 1.0f
};

void tst_QRhi::renderToTextureTexturedQuad_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureTexturedQuad()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QImage inputImage;
    inputImage.load(QLatin1String(":/data/qt256.png"));
    QVERIFY(!inputImage.isNull());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVerticesUvs)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), quadVerticesUvs);

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->create());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->create());

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings({
                         QRhiShaderResourceBinding::sampledTexture(0, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
                     });
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = loadShader(":/data/simpletextured.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/simpletextured.frag.qsb");
    QVERIFY(fs.isValid());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
                                  { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                                  { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
                              });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setShaderResources();
    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QVERIFY(!result.isNull());

    if (impl == QRhi::Null)
        return;

    // Flip with D3D and Metal because these have Y down in images. Vulkan does
    // not need this because there Y is down both in images and in NDC, which
    // just happens to give correct results with our OpenGL-targeted vertex and
    // UV data.
    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        result.flip();

    // check a few points that are expected to match regardless of the implementation
    QRgb white = qRgba(255, 255, 255, 255);
    QCOMPARE(result.pixel(79, 77), white);
    QCOMPARE(result.pixel(124, 81), white);
    QCOMPARE(result.pixel(128, 149), white);
    QCOMPARE(result.pixel(120, 189), white);
    QCOMPARE(result.pixel(116, 185), white);

    QRgb empty = qRgba(0, 0, 0, 0);
    QCOMPARE(result.pixel(11, 45), empty);
    QCOMPARE(result.pixel(246, 202), empty);
    QCOMPARE(result.pixel(130, 18), empty);
    QCOMPARE(result.pixel(4, 227), empty);

    QVERIFY(qGreen(result.pixel(32, 52)) > 2 * qRed(result.pixel(32, 52)));
    QVERIFY(qGreen(result.pixel(32, 52)) > 2 * qBlue(result.pixel(32, 52)));
    QVERIFY(qGreen(result.pixel(214, 191)) > 2 * qRed(result.pixel(214, 191)));
    QVERIFY(qGreen(result.pixel(214, 191)) > 2 * qBlue(result.pixel(214, 191)));
}

void tst_QRhi::renderToTextureSampleWithSeparateTextureAndSampler_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureSampleWithSeparateTextureAndSampler()
{
    // Same as renderToTextureTexturedQuad but the fragment shader uses a
    // separate image and sampler. For Vulkan/Metal/D3D11 these are natively
    // supported. For OpenGL this exercises the auto-generated combined sampler
    // in the GLSL code and the mapping table that gets applied at run time by
    // the backend.

    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

#ifdef Q_OS_ANDROID
    if (impl == QRhi::Vulkan) {
        QSKIP("This function fails with Vulkan on Android, see QTQAINFRA-6926 for more info.");
    }
#endif

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QImage inputImage;
    inputImage.load(QLatin1String(":/data/qt256.png"));
    QVERIFY(!inputImage.isNull());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVerticesUvs)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), quadVerticesUvs);

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->create());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->create());

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings({
                         QRhiShaderResourceBinding::texture(3, QRhiShaderResourceBinding::FragmentStage, inputTexture.data()),
                         QRhiShaderResourceBinding::sampler(5, QRhiShaderResourceBinding::FragmentStage, sampler.data())
                     });
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = loadShader(":/data/simpletextured.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/simpletextured_separate.frag.qsb");
    QVERIFY(fs.isValid());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
                                  { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                                  { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
                              });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setShaderResources();
    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QVERIFY(!result.isNull());

    if (impl == QRhi::Null)
        return;

    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        result.flip();

    QRgb white = qRgba(255, 255, 255, 255);
    QCOMPARE(result.pixel(79, 77), white);
    QCOMPARE(result.pixel(124, 81), white);
    QCOMPARE(result.pixel(128, 149), white);
    QCOMPARE(result.pixel(120, 189), white);
    QCOMPARE(result.pixel(116, 185), white);

    QRgb empty = qRgba(0, 0, 0, 0);
    QCOMPARE(result.pixel(11, 45), empty);
    QCOMPARE(result.pixel(246, 202), empty);
    QCOMPARE(result.pixel(130, 18), empty);
    QCOMPARE(result.pixel(4, 227), empty);

    QVERIFY(qGreen(result.pixel(32, 52)) > 2 * qRed(result.pixel(32, 52)));
    QVERIFY(qGreen(result.pixel(32, 52)) > 2 * qBlue(result.pixel(32, 52)));
    QVERIFY(qGreen(result.pixel(214, 191)) > 2 * qRed(result.pixel(214, 191)));
    QVERIFY(qGreen(result.pixel(214, 191)) > 2 * qBlue(result.pixel(214, 191)));
}

void tst_QRhi::renderToTextureArrayOfTexturedQuad_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureArrayOfTexturedQuad()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    if (isAndroidOpenGLSwiftShader(impl, rhi.get())) {
        QSKIP("SwiftShader software acceleration is used which does not support this OpenGLES "
              "feature. See QTBUG-132934");
    }

    QImage inputImage;
    inputImage.load(QLatin1String(":/data/qt256.png"));
    QVERIFY(!inputImage.isNull());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVerticesUvs)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), quadVerticesUvs);

    // In this test we pass 3 textures (and samplers) to the fragment shader in
    // form of an array of combined image samplers.

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->create());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QImage redImage(inputImage.size(), QImage::Format_RGBA8888);
    redImage.fill(Qt::red);

    QScopedPointer<QRhiTexture> redTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(redTexture->create());
    updates->uploadTexture(redTexture.data(), redImage);

    QImage greenImage(inputImage.size(), QImage::Format_RGBA8888);
    greenImage.fill(Qt::green);

    QScopedPointer<QRhiTexture> greenTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(greenTexture->create());
    updates->uploadTexture(greenTexture.data(), greenImage);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->create());

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QRhiShaderResourceBinding::TextureAndSampler texSamplers[3] = {
        { inputTexture.data(), sampler.data() },
        { redTexture.data(), sampler.data() },
        { greenTexture.data(), sampler.data() }
    };
    srb->setBindings({
                         QRhiShaderResourceBinding::sampledTextures(0, QRhiShaderResourceBinding::FragmentStage, 3, texSamplers)
                     });
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = loadShader(":/data/simpletextured.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/simpletextured_array.frag.qsb");
    QVERIFY(fs.isValid());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
                                  { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                                  { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
                              });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setShaderResources();
    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QVERIFY(!result.isNull());

    if (impl == QRhi::Null)
        return;

    // Flip with D3D and Metal because these have Y down in images. Vulkan does
    // not need this because there Y is down both in images and in NDC, which
    // just happens to give correct results with our OpenGL-targeted vertex and
    // UV data.
    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        result.flip();

    // we added the input image + red + green together, so red and green must be all 1
    for (int y = 0; y < result.height(); ++y) {
        for (int x = 0; x < result.width(); ++x) {
            const QRgb pixel = result.pixel(x, y);
            QCOMPARE(qRed(pixel), 255);
            QCOMPARE(qGreen(pixel), 255);
        }
    }
}

void tst_QRhi::renderToTextureTexturedQuadAndUniformBuffer_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureTexturedQuadAndUniformBuffer()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QImage inputImage;
    inputImage.load(QLatin1String(":/data/qt256.png"));
    QVERIFY(!inputImage.isNull());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVerticesUvs)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), quadVerticesUvs);

    // There will be two renderpasses. One renders with no transformation and
    // an opacity of 0.5, the second has a rotation. Bake the uniform data for
    // both into a single buffer.

    const int UNIFORM_BLOCK_SIZE = 64 + 4; // matrix + opacity
    const int secondUbufOffset = rhi->ubufAligned(UNIFORM_BLOCK_SIZE);
    const int UBUF_SIZE = secondUbufOffset + UNIFORM_BLOCK_SIZE;

    QScopedPointer<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE));
    QVERIFY(ubuf->create());

    QMatrix4x4 matrix;
    updates->updateDynamicBuffer(ubuf.data(), 0, 64, matrix.constData());
    float opacity = 0.5f;
    updates->updateDynamicBuffer(ubuf.data(), 64, 4, &opacity);

    // rotation by 45 degrees around the Z axis
    matrix.rotate(45, 0, 0, 1);
    updates->updateDynamicBuffer(ubuf.data(), secondUbufOffset, 64, matrix.constData());
    updates->updateDynamicBuffer(ubuf.data(), secondUbufOffset + 64, 4, &opacity);

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->create());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->create());

    const QRhiShaderResourceBinding::StageFlags commonVisibility = QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    QScopedPointer<QRhiShaderResourceBindings> srb0(rhi->newShaderResourceBindings());
    srb0->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, ubuf.data(), 0, UNIFORM_BLOCK_SIZE),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
                     });
    QVERIFY(srb0->create());

    QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
    srb1->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, ubuf.data(), secondUbufOffset, UNIFORM_BLOCK_SIZE),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
                     });
    QVERIFY(srb1->create());
    QVERIFY(srb1->isLayoutCompatible(srb0.data())); // hence no need for a second pipeline

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = loadShader(":/data/textured.vert.qsb");
    QVERIFY(vs.isValid());
    QShaderDescription shaderDesc = vs.description();
    QVERIFY(!shaderDesc.uniformBlocks().isEmpty());
    QCOMPARE(shaderDesc.uniformBlocks().first().size, UNIFORM_BLOCK_SIZE);

    QShader fs = loadShader(":/data/textured.frag.qsb");
    QVERIFY(fs.isValid());
    shaderDesc = fs.description();
    QVERIFY(!shaderDesc.uniformBlocks().isEmpty());
    QCOMPARE(shaderDesc.uniformBlocks().first().size, UNIFORM_BLOCK_SIZE);

    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
                                  { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                                  { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
                              });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb0.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setShaderResources();
    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult0;
    QImage result0;
    readResult0.completed = [&readResult0, &result0] {
        result0 = QImage(reinterpret_cast<const uchar *>(readResult0.data.constData()),
                        readResult0.pixelSize.width(), readResult0.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult0);
    cb->endPass(readbackBatch);

    // second pass (rotated)
    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 });
    cb->setGraphicsPipeline(pipeline.data());
    cb->setShaderResources(srb1.data()); // sources data from a different offset in ubuf
    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult1;
    QImage result1;
    readResult1.completed = [&readResult1, &result1] {
        result1 = QImage(reinterpret_cast<const uchar *>(readResult1.data.constData()),
                        readResult1.pixelSize.width(), readResult1.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult1);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QVERIFY(!result0.isNull());
    QVERIFY(!result1.isNull());

    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC()) {
        result0.flip();
        result1.flip();
    }

    if (impl == QRhi::Null)
        return;

    // opacity 0.5 (premultiplied)
    static const auto checkSemiWhite = [](const QRgb &c) {
        QRgb semiWhite127 = qPremultiply(qRgba(255, 255, 255, 127));
        QRgb semiWhite128 = qPremultiply(qRgba(255, 255, 255, 128));
        return c == semiWhite127 || c == semiWhite128;
    };
    QVERIFY(checkSemiWhite(result0.pixel(79, 77)));
    QVERIFY(checkSemiWhite(result0.pixel(124, 81)));
    QVERIFY(checkSemiWhite(result0.pixel(128, 149)));
    QVERIFY(checkSemiWhite(result0.pixel(120, 189)));
    QVERIFY(checkSemiWhite(result0.pixel(116, 185)));
    QVERIFY(checkSemiWhite(result0.pixel(191, 172)));

    QRgb empty = qRgba(0, 0, 0, 0);
    QCOMPARE(result0.pixel(11, 45), empty);
    QCOMPARE(result0.pixel(246, 202), empty);
    QCOMPARE(result0.pixel(130, 18), empty);
    QCOMPARE(result0.pixel(4, 227), empty);

    // also rotated 45 degrees around Z
    QRgb black = qRgba(0, 0, 0, 255);
    QCOMPARE(result1.pixel(20, 23), black);
    QCOMPARE(result1.pixel(47, 5), black);
    QCOMPARE(result1.pixel(238, 22), black);
    QCOMPARE(result1.pixel(250, 203), black);
    QCOMPARE(result1.pixel(224, 237), black);
    QCOMPARE(result1.pixel(12, 221), black);

    QVERIFY(checkSemiWhite(result1.pixel(142, 67)));
    QVERIFY(checkSemiWhite(result1.pixel(81, 79)));
    QVERIFY(checkSemiWhite(result1.pixel(79, 168)));
    QVERIFY(checkSemiWhite(result1.pixel(146, 204)));
    QVERIFY(checkSemiWhite(result1.pixel(186, 156)));

    QCOMPARE(result1.pixel(204, 45), empty);
    QCOMPARE(result1.pixel(28, 178), empty);
}

void tst_QRhi::renderToTextureTexturedQuadAllDynamicBuffers_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureTexturedQuadAllDynamicBuffers()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QImage inputImage;
    inputImage.load(QLatin1String(":/data/qt256.png"));
    QVERIFY(!inputImage.isNull());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    // Do like renderToTextureTexturedQuadAndUniformBuffer but only use Dynamic
    // buffers, and do updates with the direct beginFullDynamicBufferUpdate
    // function. (for some backend this is different for UniformBuffer and
    // others, hence useful exercising it also on a VertexBuffer)

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::VertexBuffer, sizeof(quadVerticesUvs)));
    QVERIFY(vbuf->create());
    char *p = vbuf->beginFullDynamicBufferUpdateForCurrentFrame();
    QVERIFY(p);
    memcpy(p, quadVerticesUvs, sizeof(quadVerticesUvs));
    vbuf->endFullDynamicBufferUpdateForCurrentFrame();

    const int UNIFORM_BLOCK_SIZE = 64 + 4; // matrix + opacity
    const int secondUbufOffset = rhi->ubufAligned(UNIFORM_BLOCK_SIZE);
    const int UBUF_SIZE = secondUbufOffset + UNIFORM_BLOCK_SIZE;

    QScopedPointer<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE));
    QVERIFY(ubuf->create());

    p = ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    QVERIFY(p);

    QMatrix4x4 matrix;
    memcpy(p, matrix.constData(), 64);
    float opacity = 0.5f;
    memcpy(p + 64, &opacity, 4);

    // rotation by 45 degrees around the Z axis
    matrix.rotate(45, 0, 0, 1);
    memcpy(p + secondUbufOffset, matrix.constData(), 64);
    memcpy(p + secondUbufOffset + 64, &opacity, 4);

    ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();
    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->create());
    updates->uploadTexture(inputTexture.data(), inputImage);
    cb->resourceUpdate(updates);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->create());

    const QRhiShaderResourceBinding::StageFlags commonVisibility = QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    QScopedPointer<QRhiShaderResourceBindings> srb0(rhi->newShaderResourceBindings());
    srb0->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, ubuf.data(), 0, UNIFORM_BLOCK_SIZE),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
                     });
    QVERIFY(srb0->create());

    QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
    srb1->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, ubuf.data(), secondUbufOffset, UNIFORM_BLOCK_SIZE),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
                     });
    QVERIFY(srb1->create());
    QVERIFY(srb1->isLayoutCompatible(srb0.data())); // hence no need for a second pipeline

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = loadShader(":/data/textured.vert.qsb");
    QVERIFY(vs.isValid());
    QShaderDescription shaderDesc = vs.description();
    QVERIFY(!shaderDesc.uniformBlocks().isEmpty());
    QCOMPARE(shaderDesc.uniformBlocks().first().size, UNIFORM_BLOCK_SIZE);

    QShader fs = loadShader(":/data/textured.frag.qsb");
    QVERIFY(fs.isValid());
    shaderDesc = fs.description();
    QVERIFY(!shaderDesc.uniformBlocks().isEmpty());
    QCOMPARE(shaderDesc.uniformBlocks().first().size, UNIFORM_BLOCK_SIZE);

    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
                                  { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                                  { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
                              });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb0.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 });
    cb->setGraphicsPipeline(pipeline.data());
    cb->setShaderResources();
    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult0;
    QImage result0;
    readResult0.completed = [&readResult0, &result0] {
        result0 = QImage(reinterpret_cast<const uchar *>(readResult0.data.constData()),
                        readResult0.pixelSize.width(), readResult0.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult0);
    cb->endPass(readbackBatch);

    // second pass (rotated)
    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 });
    cb->setGraphicsPipeline(pipeline.data());
    cb->setShaderResources(srb1.data()); // sources data from a different offset in ubuf
    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult1;
    QImage result1;
    readResult1.completed = [&readResult1, &result1] {
        result1 = QImage(reinterpret_cast<const uchar *>(readResult1.data.constData()),
                        readResult1.pixelSize.width(), readResult1.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult1);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QVERIFY(!result0.isNull());
    QVERIFY(!result1.isNull());

    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC()) {
        result0.flip();
        result1.flip();
    }

    if (impl == QRhi::Null)
        return;

    // opacity 0.5 (premultiplied)
    static const auto checkSemiWhite = [](const QRgb &c) {
        QRgb semiWhite127 = qPremultiply(qRgba(255, 255, 255, 127));
        QRgb semiWhite128 = qPremultiply(qRgba(255, 255, 255, 128));
        return c == semiWhite127 || c == semiWhite128;
    };
    QVERIFY(checkSemiWhite(result0.pixel(79, 77)));
    QVERIFY(checkSemiWhite(result0.pixel(124, 81)));
    QVERIFY(checkSemiWhite(result0.pixel(128, 149)));
    QVERIFY(checkSemiWhite(result0.pixel(120, 189)));
    QVERIFY(checkSemiWhite(result0.pixel(116, 185)));
    QVERIFY(checkSemiWhite(result0.pixel(191, 172)));

    QRgb empty = qRgba(0, 0, 0, 0);
    QCOMPARE(result0.pixel(11, 45), empty);
    QCOMPARE(result0.pixel(246, 202), empty);
    QCOMPARE(result0.pixel(130, 18), empty);
    QCOMPARE(result0.pixel(4, 227), empty);

    // also rotated 45 degrees around Z
    QRgb black = qRgba(0, 0, 0, 255);
    QCOMPARE(result1.pixel(20, 23), black);
    QCOMPARE(result1.pixel(47, 5), black);
    QCOMPARE(result1.pixel(238, 22), black);
    QCOMPARE(result1.pixel(250, 203), black);
    QCOMPARE(result1.pixel(224, 237), black);
    QCOMPARE(result1.pixel(12, 221), black);

    QVERIFY(checkSemiWhite(result1.pixel(142, 67)));
    QVERIFY(checkSemiWhite(result1.pixel(81, 79)));
    QVERIFY(checkSemiWhite(result1.pixel(79, 168)));
    QVERIFY(checkSemiWhite(result1.pixel(146, 204)));
    QVERIFY(checkSemiWhite(result1.pixel(186, 156)));

    QCOMPARE(result1.pixel(204, 45), empty);
    QCOMPARE(result1.pixel(28, 178), empty);
}

void tst_QRhi::renderToTextureDeferredSrb_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureDeferredSrb()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QImage inputImage;
    inputImage.load(QLatin1String(":/data/qt256.png"));
    QVERIFY(!inputImage.isNull());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVerticesUvs)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), quadVerticesUvs);

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->create());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->create());

    QScopedPointer<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4));
    QVERIFY(ubuf->create());

    QMatrix4x4 matrix;
    updates->updateDynamicBuffer(ubuf.data(), 0, 64, matrix.constData());
    float opacity = 0.5f;
    updates->updateDynamicBuffer(ubuf.data(), 64, 4, &opacity);

    // this is the specific thing to test here: an srb with null resources
    const QRhiShaderResourceBinding::StageFlags commonVisibility = QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    QScopedPointer<QRhiShaderResourceBindings> layoutOnlySrb(rhi->newShaderResourceBindings());
    layoutOnlySrb->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, nullptr),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, nullptr, nullptr)
                     });
    QVERIFY(layoutOnlySrb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = loadShader(":/data/textured.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/textured.frag.qsb");
    QVERIFY(fs.isValid());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
                                  { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                                  { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
                              });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(layoutOnlySrb.data()); // no resources needed yet
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    // another, layout compatible, srb with the actual resources
    QScopedPointer<QRhiShaderResourceBindings> layoutCompatibleSrbWithResources(rhi->newShaderResourceBindings());
    layoutCompatibleSrbWithResources->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, ubuf.data()),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
                     });
    QVERIFY(layoutCompatibleSrbWithResources->create());

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setShaderResources(layoutCompatibleSrbWithResources.data()); // here we must use the srb referencing the resources
    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QVERIFY(!result.isNull());

    if (impl == QRhi::Null)
        return;

    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        result.flip();

    // opacity 0.5 (premultiplied)
    static const auto checkSemiWhite = [](const QRgb &c) {
        QRgb semiWhite127 = qPremultiply(qRgba(255, 255, 255, 127));
        QRgb semiWhite128 = qPremultiply(qRgba(255, 255, 255, 128));
        return c == semiWhite127 || c == semiWhite128;
    };
    QVERIFY(checkSemiWhite(result.pixel(79, 77)));
    QVERIFY(checkSemiWhite(result.pixel(124, 81)));
    QVERIFY(checkSemiWhite(result.pixel(128, 149)));
    QVERIFY(checkSemiWhite(result.pixel(120, 189)));
    QVERIFY(checkSemiWhite(result.pixel(116, 185)));
    QVERIFY(checkSemiWhite(result.pixel(191, 172)));

    QRgb empty = qRgba(0, 0, 0, 0);
    QCOMPARE(result.pixel(11, 45), empty);
    QCOMPARE(result.pixel(246, 202), empty);
    QCOMPARE(result.pixel(130, 18), empty);
    QCOMPARE(result.pixel(4, 227), empty);
}

void tst_QRhi::renderToTextureDeferredUpdateSamplerInSrb_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureDeferredUpdateSamplerInSrb()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QImage inputImage;
    inputImage.load(QLatin1String(":/data/qt256.png"));
    QVERIFY(!inputImage.isNull());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVerticesUvs)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), quadVerticesUvs);

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->create());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QScopedPointer<QRhiSampler> sampler1(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::Linear,
                                                         QRhiSampler::Repeat, QRhiSampler::Repeat));
    QVERIFY(sampler1->create());
    QScopedPointer<QRhiSampler> sampler2(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                         QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler2->create());

    QScopedPointer<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4));
    QVERIFY(ubuf->create());

    QMatrix4x4 matrix;
    updates->updateDynamicBuffer(ubuf.data(), 0, 64, matrix.constData());
    float opacity = 0.5f;
    updates->updateDynamicBuffer(ubuf.data(), 64, 4, &opacity);

    const QRhiShaderResourceBinding::StageFlags commonVisibility = QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, ubuf.data()),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler1.data())
                     });
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = loadShader(":/data/textured.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/textured.frag.qsb");
    QVERIFY(fs.isValid());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
                                  { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                                  { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
                              });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    // Now update the sampler to a different one, so if the pipeline->create()
    // baked in static samplers somewhere (with 3D APIs where that's a thing),
    // based on sampler1, that's now all invalid.
    srb->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, ubuf.data()),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler2.data())
                     });
    srb->updateResources(); // now it references sampler2, not sampler1

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setShaderResources();
    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QVERIFY(!result.isNull());

    if (impl == QRhi::Null)
        return;

    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        result.flip();

    // opacity 0.5 (premultiplied)
    static const auto checkSemiWhite = [](const QRgb &c) {
        QRgb semiWhite127 = qPremultiply(qRgba(255, 255, 255, 127));
        QRgb semiWhite128 = qPremultiply(qRgba(255, 255, 255, 128));
        return c == semiWhite127 || c == semiWhite128;
    };
    QVERIFY(checkSemiWhite(result.pixel(79, 77)));
    QVERIFY(checkSemiWhite(result.pixel(124, 81)));
    QVERIFY(checkSemiWhite(result.pixel(128, 149)));
    QVERIFY(checkSemiWhite(result.pixel(120, 189)));
    QVERIFY(checkSemiWhite(result.pixel(116, 185)));
    QVERIFY(checkSemiWhite(result.pixel(191, 172)));

    QRgb empty = qRgba(0, 0, 0, 0);
    QCOMPARE(result.pixel(11, 45), empty);
    QCOMPARE(result.pixel(246, 202), empty);
    QCOMPARE(result.pixel(130, 18), empty);
    QCOMPARE(result.pixel(4, 227), empty);
}

void tst_QRhi::renderToTextureMultipleUniformBuffersAndDynamicOffset_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureMultipleUniformBuffersAndDynamicOffset()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QImage inputImage;
    inputImage.load(QLatin1String(":/data/qt256.png"));
    QVERIFY(!inputImage.isNull());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVerticesUvs)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), quadVerticesUvs);

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->create());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->create());

    const int MATRIX_COUNT = 4; // put 4 mat4s into the buffer, will only use one
    const int ubufElemSize = rhi->ubufAligned(64);
    QVERIFY(ubufElemSize >= 64);
    QScopedPointer<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, MATRIX_COUNT * ubufElemSize));
    QVERIFY(ubuf->create());

    float zeroes[16];
    memset(zeroes, 0, sizeof(zeroes));
    updates->updateDynamicBuffer(ubuf.data(), 0, 64, zeroes);
    updates->updateDynamicBuffer(ubuf.data(), ubufElemSize, 64, zeroes);
    // the only correct matrix is the third one
    QMatrix4x4 matrix;
    updates->updateDynamicBuffer(ubuf.data(), ubufElemSize * 2, 64, matrix.constData());
    updates->updateDynamicBuffer(ubuf.data(), ubufElemSize * 3, 64, zeroes);

    const int OPACITY_COUNT = 6; // put 6 floats into the buffer, will only use one
    const int ubuf2ElemSize = rhi->ubufAligned(4);
    QVERIFY(ubuf2ElemSize >= 4);
    QScopedPointer<QRhiBuffer> ubuf2(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, OPACITY_COUNT * ubuf2ElemSize));
    QVERIFY(ubuf2->create());

    updates->updateDynamicBuffer(ubuf2.data(), 0, 4, &zeroes[0]);
    updates->updateDynamicBuffer(ubuf2.data(), ubuf2ElemSize, 4, &zeroes[0]);
    updates->updateDynamicBuffer(ubuf2.data(), ubuf2ElemSize * 2, 4, &zeroes[0]);
    // the only correct opacity value is the fourth one
    float opacity = 0.5f;
    updates->updateDynamicBuffer(ubuf2.data(), ubuf2ElemSize * 3, 4, &opacity);
    updates->updateDynamicBuffer(ubuf2.data(), ubuf2ElemSize * 4, 4, &zeroes[0]);
    updates->updateDynamicBuffer(ubuf2.data(), ubuf2ElemSize * 5, 4, &zeroes[0]);

    const QRhiShaderResourceBinding::StageFlags commonVisibility = QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings({
                         QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, commonVisibility, ubuf.data(), 64),
                         QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(1, commonVisibility, ubuf2.data(), 4),
                         QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
                     });
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = loadShader(":/data/textured_multiubuf.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/textured_multiubuf.frag.qsb");
    QVERIFY(fs.isValid());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
                                  { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                                  { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
                              });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());

    // Now the magic, expose the 3rd matrix and 4th opacity value to the shader.
    // If the handling of dynamic offsets were broken, the shaders would likely
    // "see" an all zero matrix and zero opacity, thus leading to different
    // rendering output. This way we can verify if using dynamic offsets, and
    // more than one at the same time, is functional.
    QVarLengthArray<QPair<int, quint32>, 2> dynamicOffset = {
        { 0, quint32(ubufElemSize * 2) },
        { 1, quint32(ubuf2ElemSize * 3) },
    };
    cb->setShaderResources(srb.data(), 2, dynamicOffset.constData());

    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QVERIFY(!result.isNull());

    if (impl == QRhi::Null)
        return;

    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        result.flip();

    // opacity 0.5 (premultiplied)
    static const auto checkSemiWhite = [](const QRgb &c) {
        QRgb semiWhite127 = qPremultiply(qRgba(255, 255, 255, 127));
        QRgb semiWhite128 = qPremultiply(qRgba(255, 255, 255, 128));
        return c == semiWhite127 || c == semiWhite128;
    };
    QVERIFY(checkSemiWhite(result.pixel(79, 77)));
    QVERIFY(checkSemiWhite(result.pixel(124, 81)));
    QVERIFY(checkSemiWhite(result.pixel(128, 149)));
    QVERIFY(checkSemiWhite(result.pixel(120, 189)));
    QVERIFY(checkSemiWhite(result.pixel(116, 185)));
    QVERIFY(checkSemiWhite(result.pixel(191, 172)));

    QRgb empty = qRgba(0, 0, 0, 0);
    QCOMPARE(result.pixel(11, 45), empty);
    QCOMPARE(result.pixel(246, 202), empty);
    QCOMPARE(result.pixel(130, 18), empty);
    QCOMPARE(result.pixel(4, 227), empty);
}

void tst_QRhi::renderToTextureSrbReuse_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureSrbReuse()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    // Draw a textured quad with opacity 0.5. The difference to the simple tests
    // of the same kind is that there are two (configuration-wise identical)
    // pipeline objects that are bound after each other, with the same one srb,
    // on the command buffer. This exercises, in particular for the OpenGL
    // backend, that the uniforms are set for the pipelines' underlying shader
    // program correctly. (with OpenGL we may not use real uniform buffers,
    // which presents extra pipeline-srb tracking work for the backend)

    QImage inputImage;
    inputImage.load(QLatin1String(":/data/qt256.png"));
    QVERIFY(!inputImage.isNull());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVerticesUvs)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), quadVerticesUvs);

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->create());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->create());

    QScopedPointer<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4));
    QVERIFY(ubuf->create());
    QMatrix4x4 matrix;
    updates->updateDynamicBuffer(ubuf.data(), 0, 64, matrix.constData());
    float opacity = 0.5f;
    updates->updateDynamicBuffer(ubuf.data(), 64, 4, &opacity);

    const QRhiShaderResourceBinding::StageFlags commonVisibility = QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, ubuf.data()),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
        });
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline1(rhi->newGraphicsPipeline());
    pipeline1->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    QShader vs = loadShader(":/data/textured.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/textured.frag.qsb");
    QVERIFY(fs.isValid());
    pipeline1->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes({
                                  { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
                                  { 0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float) }
                              });
    pipeline1->setVertexInputLayout(inputLayout);
    pipeline1->setShaderResourceBindings(srb.data());
    pipeline1->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(pipeline1->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline2(rhi->newGraphicsPipeline());
    pipeline2->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    pipeline2->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    pipeline2->setVertexInputLayout(inputLayout);
    pipeline2->setShaderResourceBindings(srb.data());
    pipeline2->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(pipeline2->create());

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, updates);

    // The key step in this test: set the 1st pipeline, then the 2nd, the
    // srb is the same. This should lead to identical results to just
    // binding one of them.
    cb->setGraphicsPipeline(pipeline1.data());
    cb->setShaderResources(srb.data());
    cb->setGraphicsPipeline(pipeline2.data());
    cb->setShaderResources(srb.data());
    cb->setViewport({ 0, 0, float(texture->pixelSize().width()), float(texture->pixelSize().height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(4);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QVERIFY(!result.isNull());

    if (impl == QRhi::Null)
        return;

    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        result.flip();

    // opacity 0.5 (premultiplied)
    static const auto checkSemiWhite = [](const QRgb &c) {
        QRgb semiWhite127 = qPremultiply(qRgba(255, 255, 255, 127));
        QRgb semiWhite128 = qPremultiply(qRgba(255, 255, 255, 128));
        return c == semiWhite127 || c == semiWhite128;
    };
    QVERIFY(checkSemiWhite(result.pixel(79, 77)));
    QVERIFY(checkSemiWhite(result.pixel(124, 81)));
    QVERIFY(checkSemiWhite(result.pixel(128, 149)));
    QVERIFY(checkSemiWhite(result.pixel(120, 189)));
    QVERIFY(checkSemiWhite(result.pixel(116, 185)));
    QVERIFY(checkSemiWhite(result.pixel(191, 172)));

    QRgb empty = qRgba(0, 0, 0, 0);
    QCOMPARE(result.pixel(11, 45), empty);
    QCOMPARE(result.pixel(246, 202), empty);
    QCOMPARE(result.pixel(130, 18), empty);
    QCOMPARE(result.pixel(4, 227), empty);
}

void tst_QRhi::setWindowType(QWindow *window, QRhi::Implementation impl)
{
    switch (impl) {
#ifdef TST_GL
    case QRhi::OpenGLES2:
        window->setSurfaceType(QSurface::OpenGLSurface);
        break;
#endif
    case QRhi::D3D11:
        window->setSurfaceType(QSurface::Direct3DSurface);
        break;
    case QRhi::Metal:
        window->setSurfaceType(QSurface::MetalSurface);
        break;
#ifdef TST_VK
    case QRhi::Vulkan:
        window->setSurfaceType(QSurface::VulkanSurface);
        window->setVulkanInstance(&vulkanInstance);
        break;
#endif
    default:
        break;
    }
}

void tst_QRhi::renderToTextureIndexedDraw_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureIndexedDraw()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    const QSize outputSize(1920, 1080);
    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, outputSize, 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    static const quint16 indices[] = {
        0, 1, 2
    };

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiBuffer> ibuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(indices)));
    QVERIFY(ibuf->create());
    updates->uploadStaticBuffer(ibuf.data(), indices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createSimplePipeline(rhi.data(), srb.data(), rpDesc.data()));
    QVERIFY(pipeline);

    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);

    // Do three render passes, even though all render the same thing. This is done to
    // verify that QTBUG-89765 is fixed.  One of them specifies ExternalContent which
    // triggers special behavior with some backends (uses a secondary command buffer with
    // Vulkan for example). This way we can see that optimizations, such as keeping track
    // of what index buffer is active, are handled correctly across pass boundaries in the
    // QRhi backends. Without the fix for QTBUG-89765 this test would show validation
    // warnings and even crash when run with Vulkan.

    cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    cb->setVertexInput(0, 1, &vbindings, ibuf.data(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(3);
    cb->endPass();

    cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, nullptr, QRhiCommandBuffer::ExternalContent);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    cb->setVertexInput(0, 1, &vbindings, ibuf.data(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(3);
    cb->endPass();

    cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, nullptr);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    cb->setVertexInput(0, 1, &vbindings, ibuf.data(), 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();
    QCOMPARE(result.size(), texture->pixelSize());

    if (impl == QRhi::Null)
        return;

    // Now we have a red rectangle on blue background.
    const int y = 100;
    const quint32 *p = reinterpret_cast<const quint32 *>(result.constScanLine(y));
    int x = result.width() - 1;
    int redCount = 0;
    int blueCount = 0;
    const int maxFuzz = 1;
    while (x-- >= 0) {
        const QRgb c(*p++);
        if (qRed(c) >= (255 - maxFuzz) && qGreen(c) == 0 && qBlue(c) == 0)
            ++redCount;
        else if (qRed(c) == 0 && qGreen(c) == 0 && qBlue(c) >= (255 - maxFuzz))
            ++blueCount;
        else
            QFAIL("Encountered a pixel that is neither red or blue");
    }

    QCOMPARE(redCount + blueCount, texture->pixelSize().width());

    if (rhi->isYUpInFramebuffer() == rhi->isYUpInNDC())
        QVERIFY(redCount < blueCount);
    else
        QVERIFY(redCount > blueCount);
}

void tst_QRhi::renderToTextureArrayMultiView_data()
{
    rhiTestData();
}

void tst_QRhi::renderToTextureArrayMultiView()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    if (!rhi->isFeatureSupported(QRhi::MultiView))
        QSKIP("Multiview not supported, skipping testing on this backend");

    if (rhi->backend() == QRhi::Vulkan && rhi->driverInfo().deviceType == QRhiDriverInfo::CpuDevice)
        QSKIP("lavapipe does not like multiview, skip for now");

    for (int sampleCount : rhi->supportedSampleCounts()) {
        const QSize outputSize(1920, 1080);
        QRhiTexture::Flags textureFlags = QRhiTexture::RenderTarget;
        if (sampleCount <= 1)
            textureFlags |= QRhiTexture::UsedAsTransferSource;
        QScopedPointer<QRhiTexture> texture(rhi->newTextureArray(QRhiTexture::RGBA8, 2, outputSize, sampleCount, textureFlags));
        QVERIFY(texture->create());

        // exercise a depth-stencil buffer as well, not that the triangle needs it; note that this also needs to be a two-layer texture array
        QScopedPointer<QRhiTexture> ds(rhi->newTextureArray(QRhiTexture::D24S8, 2, outputSize, sampleCount, QRhiTexture::RenderTarget));
        QVERIFY(ds->create());

        QScopedPointer<QRhiTexture> resolveTexture;
        if (sampleCount > 1) {
            resolveTexture.reset(rhi->newTextureArray(QRhiTexture::RGBA8, 2, outputSize, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
            QVERIFY(resolveTexture->create());
        }

        QRhiColorAttachment multiViewAtt(texture.get());
        multiViewAtt.setMultiViewCount(2);
        if (sampleCount > 1)
            multiViewAtt.setResolveTexture(resolveTexture.get());

        QRhiTextureRenderTargetDescription rtDesc(multiViewAtt);
        rtDesc.setDepthTexture(ds.get());

        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->create());

        QRhiCommandBuffer *cb = nullptr;
        QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
        QVERIFY(cb);

        QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

        static float triangleData[] = {
            0.0f,   0.5f,    1.0f, 0.0f, 0.0f,
            -0.5f,  -0.5f,    0.0f, 1.0f, 0.0f,
            0.5f,  -0.5f,    0.0f, 0.0f, 1.0f
        };

        QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleData)));
        QVERIFY(vbuf->create());
        updates->uploadStaticBuffer(vbuf.data(), triangleData);

        QScopedPointer<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 128)); // mat4 mvp[2]
        QVERIFY(ubuf->create());

        QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
        srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf.get())
        });
        QVERIFY(srb->create());

        QScopedPointer<QRhiGraphicsPipeline> ps(rhi->newGraphicsPipeline());
        ps->setShaderStages({
            { QRhiShaderStage::Vertex, loadShader(":/data/multiview.vert.qsb") },
            { QRhiShaderStage::Fragment, loadShader(":/data/multiview.frag.qsb") }
        });
        ps->setMultiViewCount(2); // the view count must be set both on the render target and the pipeline
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 5 * sizeof(float) }
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
            { 0, 1, QRhiVertexInputAttribute::Float3, quint32(2 * sizeof(float)) }
        });
        ps->setDepthTest(true);
        ps->setDepthWrite(true);
        ps->setSampleCount(sampleCount);
        ps->setVertexInputLayout(inputLayout);
        ps->setShaderResourceBindings(srb.get());
        ps->setRenderPassDescriptor(rpDesc.get());
        QVERIFY(ps->create());

        QMatrix4x4 mvp = rhi->clipSpaceCorrMatrix();
        mvp.perspective(45.0f, outputSize.width() / float(outputSize.height()), 0.01f, 1000.0f);
        mvp.translate(0, 0, -2);
        mvp.rotate(90, 0, 0, 1); // point left
        updates->updateDynamicBuffer(ubuf.get(), 0, 64, mvp.constData());
        mvp.rotate(-180, 0, 0, 1); // point right
        updates->updateDynamicBuffer(ubuf.get(), 64, 64, mvp.constData());

        cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, updates);
        cb->setGraphicsPipeline(ps.data());
        cb->setShaderResources();
        cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
        QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
        cb->setVertexInput(0, 1, &vbindings);
        cb->draw(3);

        QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
        QRhiReadbackResult readResult[2];
        QRhiReadbackDescription readbackDesc;
        if (sampleCount > 1)
            readbackDesc.setTexture(resolveTexture.get());
        else
            readbackDesc.setTexture(texture.get());
        readbackDesc.setLayer(0);
        readbackBatch->readBackTexture(readbackDesc, &readResult[0]);
        readbackDesc.setLayer(1);
        readbackBatch->readBackTexture(readbackDesc, &readResult[1]);

        cb->endPass(readbackBatch);

        rhi->endOffscreenFrame();

        if (rhi->backend() == QRhi::Null)
            QSKIP("No real content with Null backend, skipping multiview content check");

        // both readbacks should be finished now due to using offscreen frames

        QImage image0 = QImage(reinterpret_cast<const uchar *>(readResult[0].data.constData()),
                            readResult[0].pixelSize.width(), readResult[0].pixelSize.height(),
                            QImage::Format_RGBA8888);
        if (rhi->isYUpInFramebuffer()) // note that we used clipSpaceCorrMatrix
            image0.flip();

        QImage image1 = QImage(reinterpret_cast<const uchar *>(readResult[1].data.constData()),
                            readResult[1].pixelSize.width(), readResult[1].pixelSize.height(),
                            QImage::Format_RGBA8888);
        if (rhi->isYUpInFramebuffer())
            image1.flip();

        QVERIFY(!image0.isNull());
        QVERIFY(!image1.isNull());

        // image0 should have a triangle rotated so that it points left with the red
        // tip. image1 should have a triangle rotated so that it points right with
        // the red tip. Both are centered, so we will check in range 0..width/2 for
        // image0 and width/2..width-1 for image1 to see if the red-enough pixels
        // are present.

        int y = image0.height() / 2;
        int n = 0;
        for (int x = 0; x < image0.width() / 2; ++x) {
            QRgb c = image0.pixel(x, y);
            if (qRed(c) > 250 && qGreen(c) < 10 && qBlue(c) < 10)
                ++n;
        }
        QVERIFY(n >= 10);

        y = image1.height() / 2;
        n = 0;
        for (int x = image1.width() / 2; x < image1.width(); ++x) {
            QRgb c = image1.pixel(x, y);
            if (qRed(c) > 250 && qGreen(c) < 10 && qBlue(c) < 10)
                ++n;
        }
        QVERIFY(n >= 10);
    }
}

void tst_QRhi::renderToWindowSimple_data()
{
    rhiTestData();
}

void tst_QRhi::renderToWindowSimple()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("offscreen"), Qt::CaseInsensitive))
        QSKIP("Offscreen: This fails.");

    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QScopedPointer<QWindow> window(new QWindow);
    setWindowType(window.data(), impl);

    window->setGeometry(0, 0, 640, 480);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QScopedPointer<QRhiSwapChain> swapChain(rhi->newSwapChain());
    swapChain->setWindow(window.data());
    swapChain->setFlags(QRhiSwapChain::UsedAsTransferSource);
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(swapChain->newCompatibleRenderPassDescriptor());
    swapChain->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(swapChain->createOrResize());

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createSimplePipeline(rhi.data(), srb.data(), rpDesc.data()));
    QVERIFY(pipeline);

    const int asyncReadbackFrames = rhi->resourceLimit(QRhi::MaxAsyncReadbackFrames);
    // one frame issues the readback, then we do MaxAsyncReadbackFrames more to ensure the readback completes
    const int FRAME_COUNT = asyncReadbackFrames + 1;

    bool readCompleted = false;
    QRhiReadbackResult readResult;
    QImage result;
    int readbackWidth = 0;

    bool readCompletedPartial = false;
    QRhiReadbackResult readResultPartial;
    QImage resultPartial;

    for (int frameNo = 0; frameNo < FRAME_COUNT; ++frameNo) {
        QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);
        QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
        QRhiRenderTarget *rt = swapChain->currentFrameRenderTarget();
        QCOMPARE(rt->resourceType(), QRhiResource::SwapChainRenderTarget);
        QVERIFY(rt->renderPassDescriptor());
        QCOMPARE(static_cast<QRhiSwapChainRenderTarget *>(rt)->swapChain(), swapChain.data());
        const QSize outputSize = swapChain->currentPixelSize();
        QCOMPARE(rt->pixelSize(), outputSize);
        QRhiViewport viewport(0, 0, float(outputSize.width()), float(outputSize.height()));

        cb->beginPass(rt, Qt::blue, { 1.0f, 0 }, updates);
        updates = nullptr;
        cb->setGraphicsPipeline(pipeline.data());
        cb->setViewport(viewport);
        QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
        cb->setVertexInput(0, 1, &vbindings);
        cb->draw(3);

        if (frameNo == 0) {
            QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
            readResult.completed = [&readCompleted, &readResult, &result, &rhi] {
                readCompleted = true;
                QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                                    readResult.pixelSize.width(), readResult.pixelSize.height(),
                                    QImage::Format_ARGB32_Premultiplied);
                if (readResult.format == QRhiTexture::RGBA8)
                    wrapperImage = wrapperImage.rgbSwapped();
                if (rhi->isYUpInFramebuffer() == rhi->isYUpInNDC())
                    result = wrapperImage.flipped();
                else
                    result = wrapperImage.copy();
            };
            QRhiReadbackDescription readbackDescription;
            QVERIFY(!readbackDescription.rect().isValid());
            readbackBatch->readBackTexture(readbackDescription, &readResult); // read back the current backbuffer
            readbackWidth = outputSize.width();
            readResultPartial.completed = [&readCompletedPartial, &readResultPartial, &resultPartial, &rhi] {
                readCompletedPartial = true;
                QImage wrapperImage(reinterpret_cast<const uchar *>(readResultPartial.data.constData()),
                                    readResultPartial.pixelSize.width(), readResultPartial.pixelSize.height(),
                                    QImage::Format_ARGB32_Premultiplied);
                if (readResultPartial.format == QRhiTexture::RGBA8)
                    wrapperImage = wrapperImage.rgbSwapped();
                if (rhi->isYUpInFramebuffer() == rhi->isYUpInNDC())
                    resultPartial = wrapperImage.flipped();
                else
                    resultPartial = wrapperImage.copy();
            };
            QRhiReadbackDescription partialReadbackDescription;
            partialReadbackDescription.setRect({100, 100, 1, 1});
            QVERIFY(partialReadbackDescription.rect().isValid());
            readbackBatch->readBackTexture(partialReadbackDescription, &readResultPartial); // read back one pixel at 100,100 of the current backbuffer
            cb->endPass(readbackBatch);
        } else {
            cb->endPass();
        }

        rhi->endFrame(swapChain.data());
    }

    // The readback is asynchronous here. However it is guaranteed that it
    // finished at latest after rendering QRhi::MaxAsyncReadbackFrames frames
    // after the one that enqueues the readback.
    QVERIFY(readCompleted);
    QVERIFY(readbackWidth > 0);

    if (impl == QRhi::Null)
        return;

    // Now we have a red rectangle on blue background.
    const int y = 50;
    const quint32 *p = reinterpret_cast<const quint32 *>(result.constScanLine(y));
    int x = result.width() - 1;
    int redCount = 0;
    int blueCount = 0;
    const int maxFuzz = 1;
    while (x-- >= 0) {
        const QRgb c(*p++);
        if (qRed(c) >= (255 - maxFuzz) && qGreen(c) == 0 && qBlue(c) == 0)
            ++redCount;
        else if (qRed(c) == 0 && qGreen(c) == 0 && qBlue(c) >= (255 - maxFuzz))
            ++blueCount;
        else
            QFAIL("Encountered a pixel that is neither red or blue");
    }

    QCOMPARE(redCount + blueCount, readbackWidth);
    QVERIFY(redCount < blueCount);

    // Verify the backbuffer single-pixel readback
    QVERIFY(readCompletedPartial);
    QCOMPARE(readResultPartial.pixelSize, QSize(1, 1));
    if (rhi->isYUpInFramebuffer() == rhi->isYUpInNDC())
        result.flip();
    QCOMPARE(resultPartial.pixelColor(0, 0), result.pixelColor(100, 100));
}

void tst_QRhi::continuousReadbackFromWindow_data()
{
    rhiTestData();
}

void tst_QRhi::continuousReadbackFromWindow()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("offscreen"), Qt::CaseInsensitive))
        QSKIP("Offscreen: This fails.");

    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QScopedPointer<QWindow> window(new QWindow);
    setWindowType(window.data(), impl);

    window->setGeometry(0, 0, 640, 480);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QScopedPointer<QRhiSwapChain> swapChain(rhi->newSwapChain());
    swapChain->setWindow(window.data());
    swapChain->setFlags(QRhiSwapChain::UsedAsTransferSource);
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(swapChain->newCompatibleRenderPassDescriptor());
    swapChain->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(swapChain->createOrResize());

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createSimplePipeline(rhi.data(), srb.data(), rpDesc.data()));
    QVERIFY(pipeline);

    const int asyncReadbackFrames = rhi->resourceLimit(QRhi::MaxAsyncReadbackFrames);
    const int FRAME_COUNT = asyncReadbackFrames * 10;
    QVector<QRhiReadbackResult> readResults(FRAME_COUNT);
    int readbackCompletedCount = 0;

    for (int frameNo = 0; frameNo < FRAME_COUNT; ++frameNo) {
        QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);
        QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
        QRhiRenderTarget *rt = swapChain->currentFrameRenderTarget();
        const QSize outputSize = swapChain->currentPixelSize();
        QRhiViewport viewport(0, 0, float(outputSize.width()), float(outputSize.height()));

        cb->beginPass(rt, Qt::blue, { 1.0f, 0 }, updates);
        updates = nullptr;
        cb->setGraphicsPipeline(pipeline.data());
        cb->setViewport(viewport);
        QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
        cb->setVertexInput(0, 1, &vbindings);
        cb->draw(3);

        readResults[frameNo].completed = [&readbackCompletedCount] {
            readbackCompletedCount += 1;
        };
        QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
        readbackBatch->readBackTexture({}, &readResults[frameNo]); // read back the current backbuffer
        cb->endPass(readbackBatch);

        rhi->endFrame(swapChain.data());
    }

    QVERIFY(readbackCompletedCount >= FRAME_COUNT - asyncReadbackFrames);
    rhi->finish();
    QCOMPARE(readbackCompletedCount, FRAME_COUNT);
}

void tst_QRhi::finishWithinSwapchainFrame_data()
{
    rhiTestData();
}

void tst_QRhi::finishWithinSwapchainFrame()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("offscreen"), Qt::CaseInsensitive))
        QSKIP("Offscreen: This fails.");

    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QScopedPointer<QWindow> window(new QWindow);
    setWindowType(window.data(), impl);

    window->setGeometry(0, 0, 640, 480);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QScopedPointer<QRhiSwapChain> swapChain(rhi->newSwapChain());
    swapChain->setWindow(window.data());
    swapChain->setFlags(QRhiSwapChain::UsedAsTransferSource);
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(swapChain->newCompatibleRenderPassDescriptor());
    swapChain->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(swapChain->createOrResize());

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createSimplePipeline(rhi.data(), srb.data(), rpDesc.data()));
    QVERIFY(pipeline);

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());

    // exercise begin/endExternal() just a little bit, note ExternalContent for beginPass()
    QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);
    QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
    QRhiRenderTarget *rt = swapChain->currentFrameRenderTarget();
    const QSize outputSize = swapChain->currentPixelSize();

    // repeat a sequence of upload, renderpass, readback, finish a number of
    // times within the same frame
    for (int i = 0; i < 5; ++i) {
        QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();
        updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

        cb->beginPass(rt, Qt::blue, { 1.0f, 0 }, updates, QRhiCommandBuffer::ExternalContent);

        // just have some commands, do not bother with draw calls
        cb->setGraphicsPipeline(pipeline.data());
        QRhiViewport viewport(0, 0, float(outputSize.width()), float(outputSize.height()));
        cb->setViewport(viewport);

        // do a dummy begin/endExternal round: interesting for Vulkan because
        // there this may start end then submit a secondary command buffer
        cb->beginExternal();
        cb->endExternal();

        cb->endPass();

        QRhiReadbackResult readResult;
        bool ok = false;
        readResult.completed = [&readResult, &ok, impl] {
            QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                                readResult.pixelSize.width(), readResult.pixelSize.height(),
                                QImage::Format_ARGB32_Premultiplied);
            if (readResult.format == QRhiTexture::RGBA8)
                wrapperImage = wrapperImage.rgbSwapped();

            if (impl != QRhi::Null)
                ok = qBlue(wrapperImage.pixel(43, 89)) > 250;
            else
                ok = true; // the Null backend does not actually render
        };
        QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
        readbackBatch->readBackTexture({}, &readResult); // read back the current backbuffer
        cb->resourceUpdate(readbackBatch);

        // force submit what we have so far, wait for the queue, and then start
        // a new primary command buffer
        rhi->finish();

        QVERIFY(ok);
    }

    rhi->endFrame(swapChain.data());
}

void tst_QRhi::resourceUpdateBatchBufferTextureWithSwapchainFrames_data()
{
    rhiTestData();
}

void tst_QRhi::resourceUpdateBatchBufferTextureWithSwapchainFrames()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("offscreen"), Qt::CaseInsensitive))
        QSKIP("Offscreen: Skipping onscreen test");

    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing buffer resource updates");

    QScopedPointer<QWindow> window(new QWindow);
    setWindowType(window.data(), impl);

    window->setGeometry(0, 0, 640, 480);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QScopedPointer<QRhiSwapChain> swapChain(rhi->newSwapChain());
    swapChain->setWindow(window.data());
    swapChain->setFlags(QRhiSwapChain::UsedAsTransferSource);
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(swapChain->newCompatibleRenderPassDescriptor());
    swapChain->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(swapChain->createOrResize());

    const int bufferSize = 18;
    const char *a = "123456789";
    const char *b = "abcdefghi";

    bool readCompleted = false;
    QRhiReadbackResult readResult;
    readResult.completed = [&readCompleted] { readCompleted = true; };
    QRhiReadbackResult texReadResult;
    texReadResult.completed = [&readCompleted] { readCompleted = true; };

    {
        QScopedPointer<QRhiBuffer> dynamicBuffer(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, bufferSize));
        QVERIFY(dynamicBuffer->create());

        for (int i = 0; i < bufferSize; ++i) {
            QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);

            QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();

            // One byte every 16.66 ms should be enough for everyone: fill up
            // the buffer with "123456789abcdefghi", one byte in each frame.
            if (i >= bufferSize / 2)
                batch->updateDynamicBuffer(dynamicBuffer.data(), i, 1, b + (i - bufferSize / 2));
            else
                batch->updateDynamicBuffer(dynamicBuffer.data(), i, 1, a + i);

            QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
            // just clear to black, but submit the resource update
            cb->beginPass(swapChain->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, batch);
            cb->endPass();

            rhi->endFrame(swapChain.data());
        }

        {
            QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);

            QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
            readCompleted = false;
            batch->readBackBuffer(dynamicBuffer.data(), 0, bufferSize, &readResult);

            QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
            cb->beginPass(swapChain->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, batch);
            cb->endPass();

            rhi->endFrame(swapChain.data());

            // This is a proper, typically at least double buffered renderer (as
            // a real swapchain is involved). readCompleted may only become true
            // in a future frame.
            while (!readCompleted) {
                QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);
                rhi->endFrame(swapChain.data());
            }

            QVERIFY(readResult.data.size() == bufferSize);
            QCOMPARE(readResult.data.left(bufferSize / 2), QByteArray(a));
            QCOMPARE(readResult.data.mid(bufferSize / 2), QByteArray(b));
        }
    }

    // Repeat for types Immutable and Static, declare Vertex usage.
    // This may not be readable on GLES 2.0 so skip the verification then.
    for (QRhiBuffer::Type type : { QRhiBuffer::Immutable, QRhiBuffer::Static }) {
        QScopedPointer<QRhiBuffer> buffer(rhi->newBuffer(type, QRhiBuffer::VertexBuffer, bufferSize));
        QVERIFY(buffer->create());

        for (int i = 0; i < bufferSize; ++i) {
            QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);

            QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
            if (i >= bufferSize / 2)
                batch->uploadStaticBuffer(buffer.data(), i, 1, b + (i - bufferSize / 2));
            else
                batch->uploadStaticBuffer(buffer.data(), i, 1, a + i);

            QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
            cb->beginPass(swapChain->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, batch);
            cb->endPass();

            rhi->endFrame(swapChain.data());
        }

        if (rhi->isFeatureSupported(QRhi::ReadBackNonUniformBuffer)) {
            QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);

            QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
            readCompleted = false;
            batch->readBackBuffer(buffer.data(), 0, bufferSize, &readResult);

            QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
            cb->beginPass(swapChain->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, batch);
            cb->endPass();

            rhi->endFrame(swapChain.data());

            while (!readCompleted) {
                QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);
                rhi->endFrame(swapChain.data());
            }

            QVERIFY(readResult.data.size() == bufferSize);
            QCOMPARE(readResult.data.left(bufferSize / 2), QByteArray(a));
            QCOMPARE(readResult.data.mid(bufferSize / 2), QByteArray(b));
        } else {
            qDebug("Skipping verification of buffer data as ReadBackNonUniformBuffer is not supported");
        }
    }

    // Now exercise a texture. Internally this is expected (with low level APIs
    // at least) to be similar to what happens with a staic buffer: copy to host
    // visible staging buffer, enqueue buffer-to-buffer (or here
    // buffer-to-image) copy.
    {
        const int w = 234;
        const int h = 8; // use a small height because vsync throttling is active
        const QColor colors[] = { Qt::red, Qt::green, Qt::blue, Qt::gray, Qt::yellow, Qt::black, Qt::white, Qt::magenta };
        QImage image(w, h, QImage::Format_RGBA8888);
        for (int i = 0; i < h; ++i) {
            QRgb c = colors[i].rgb();
            uchar *p = image.scanLine(i);
            int x = w;
            while (x--) {
                *p++ = qRed(c);
                *p++ = qGreen(c);
                *p++ = qBlue(c);
                *p++ = qAlpha(c);
            }
        }

        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(w, h), 1, QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        // fill a texture from the image, two lines at a time
        for (int i = 0; i < h / 2; ++i) {
            QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);
            QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();

            QRhiTextureSubresourceUploadDescription subresDesc(image);
            subresDesc.setSourceSize(QSize(w, 2));
            subresDesc.setSourceTopLeft(QPoint(0, i * 2));
            subresDesc.setDestinationTopLeft(QPoint(0, i * 2));
            QRhiTextureUploadDescription uploadDesc(QRhiTextureUploadEntry(0, 0, subresDesc));
            batch->uploadTexture(texture.data(), uploadDesc);

            QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
            cb->beginPass(swapChain->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, batch);
            cb->endPass();

            rhi->endFrame(swapChain.data());
        }

        {
            QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);

            QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
            readCompleted = false;
            batch->readBackTexture(texture.data(), &texReadResult);

            QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
            cb->beginPass(swapChain->currentFrameRenderTarget(), Qt::black, { 1.0f, 0 }, batch);
            cb->endPass();

            rhi->endFrame(swapChain.data());

            while (!readCompleted) {
                QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);
                rhi->endFrame(swapChain.data());
            }

            QCOMPARE(texReadResult.pixelSize, image.size());
            QImage wrapperImage(reinterpret_cast<const uchar *>(texReadResult.data.constData()),
                                texReadResult.pixelSize.width(), texReadResult.pixelSize.height(),
                                image.format());
            QVERIFY(imageRGBAEquals(image, wrapperImage));
        }
    }
}

void tst_QRhi::textureRenderTargetAutoRebuild_data()
{
    rhiTestData();
}

void tst_QRhi::textureRenderTargetAutoRebuild()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    // case 1: beginPass's implicit create()
    {
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
        QVERIFY(texture->create());
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ { texture.data() } }));
        QScopedPointer<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rp.data());
        QVERIFY(rt->create());

        QRhiCommandBuffer *cb = nullptr;
        QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
        QVERIFY(cb);
        cb->beginPass(rt.data(), Qt::red, { 1.0f, 0 });
        cb->endPass();
        rhi->endOffscreenFrame();

        texture->setPixelSize(QSize(256, 256));
        QVERIFY(texture->create());
        QCOMPARE(texture->pixelSize(), QSize(256, 256));

        QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
        QVERIFY(cb);
        // no rt->create() but beginPass() does it implicitly for us
        cb->beginPass(rt.data(), Qt::red, { 1.0f, 0 });
        QCOMPARE(rt->pixelSize(), QSize(256, 256));
        cb->endPass();
        rhi->endOffscreenFrame();
    }

    // case 2: pixelSize's implicit create()
    {
        QSize sz(512, 512);
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, sz, 1, QRhiTexture::RenderTarget));
        QVERIFY(texture->create());
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ { texture.data() } }));
        QScopedPointer<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rp.data());
        QVERIFY(rt->create());
        QCOMPARE(rt->pixelSize(), sz);

        sz = QSize(256, 256);
        texture->setPixelSize(sz);
        QVERIFY(texture->create());
        QCOMPARE(rt->pixelSize(), sz);
    }
}


void tst_QRhi::renderToMRTPerRenderTargetBlending_data()
{
    rhiTestData();
}

static QRhiGraphicsPipeline *createMRTBLPipeline(QRhi *rhi, QRhiShaderResourceBindings *srb, QRhiRenderPassDescriptor *rpDesc, QRhiGraphicsPipeline::TargetBlend blend[2])
{
    std::unique_ptr<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    QShader vs = loadShader(":/data/mrtbl.vert.qsb");
    if (!vs.isValid())
        return nullptr;
    QShader fs = loadShader(":/data/mrtbl.frag.qsb");
    if (!fs.isValid())
        return nullptr;
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 2 * sizeof(float) } });
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float2, 0 } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb);
    pipeline->setRenderPassDescriptor(rpDesc);
    pipeline->setTargetBlends({blend[0], blend[1]});
    return pipeline->create() ? pipeline.release() : nullptr;
}

void tst_QRhi::renderToMRTPerRenderTargetBlending()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");
    if (!rhi->isFeatureSupported(QRhi::PerRenderTargetBlending))
        QSKIP("QRhi::PerRenderTargetBlending not supported, skipping testing rendering");

    const QSize outputSize(1920, 1080);
    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, outputSize, 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTexture> texture2(rhi->newTexture(QRhiTexture::RGBA8, outputSize, 1,
                                                         QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture2->create());

    QRhiTextureRenderTargetDescription desc;
    desc.setColorAttachments({{texture.data()}, {texture2.data()}});
    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(desc));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QRhiGraphicsPipeline::TargetBlend blend[2];
    blend[0].enable = true;    // Use src-over for first rt
    blend[0].srcColor = QRhiGraphicsPipeline::One;
    blend[0].dstColor = QRhiGraphicsPipeline::Zero;
    blend[0].srcAlpha = QRhiGraphicsPipeline::One;
    blend[0].dstAlpha = QRhiGraphicsPipeline::Zero;
    blend[1].enable = true;    // Use custom blending second rt
    blend[1].srcColor = QRhiGraphicsPipeline::Zero;
    blend[1].dstColor = QRhiGraphicsPipeline::OneMinusSrcColor;
    blend[1].srcAlpha = QRhiGraphicsPipeline::One;
    blend[1].dstAlpha = QRhiGraphicsPipeline::Zero;

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createMRTBLPipeline(rhi.data(), srb.data(), rpDesc.data(), blend));
    QVERIFY(pipeline);

    cb->beginPass(rt.data(), Qt::white, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(3);

    QRhiReadbackResult readResult1;
    QImage result1;
    readResult1.completed = [&readResult1, &result1] {
        result1 = QImage(reinterpret_cast<const uchar *>(readResult1.data.constData()),
                        readResult1.pixelSize.width(), readResult1.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied); // non-owning, no copy needed because readResult outlives result
    };
    QRhiReadbackResult readResult2;
    QImage result2;
    readResult2.completed = [&readResult2, &result2] {
        result2 = QImage(reinterpret_cast<const uchar *>(readResult2.data.constData()),
                        readResult2.pixelSize.width(), readResult2.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied); // non-owning, no copy needed because readResult outlives result
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult1);
    readbackBatch->readBackTexture({ texture2.data() }, &readResult2);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QCOMPARE(result1.size(), texture->pixelSize());
    QCOMPARE(result2.size(), texture2->pixelSize());

    if (impl == QRhi::Null)
        return;

    // Now we have a red triangle in result1 and an 'electric blue' triangle in the result2
    // which are complementary colors. If we sum them up they should return all white.

    int y = result1.size().height() / 2;
    const quint32 *p1 = reinterpret_cast<const quint32 *>(result1.constScanLine(y));
    const quint32 *p2 = reinterpret_cast<const quint32 *>(result2.constScanLine(y));

    const int width = result1.size().width();
    for (int i = 0; i < width; i++) {
        QRgb c1(*p1++);
        QRgb c2(*p2++);
        const bool c1white = (qAbs(qRed(c1) - 255) <= 1 && qAbs(qGreen(c1) - 255) <= 1 && qAbs(qBlue(c1) - 255) <= 1);
        const bool c2white = (qAbs(qRed(c2) - 255) <= 1 && qAbs(qGreen(c2) - 255) <= 1 && qAbs(qBlue(c2) - 255) <= 1);

        // skip if both pixels are white
        if (c1white && c2white)
            continue;

        // remove the chance that either color accounts
        // for all the value in the color sum
        if (c1white || c2white)
            QFAIL("If both colors are not white then neither can be white.");

        if (qAbs((qRed(c1) + qRed(c2) - 255)) > 1
            || qAbs((qGreen(c1) + qGreen(c2) - 255)) > 1
            || qAbs((qBlue(c1) + qBlue(c2) - 255)) > 1) {
            QFAIL("Colors in resulting images are not complementary");
        }
    }
}

void tst_QRhi::srbLayoutCompatibility_data()
{
    rhiTestData();
}

void tst_QRhi::srbLayoutCompatibility()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing texture resource updates");

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512)));
    QVERIFY(texture->create());
    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->create());
    QScopedPointer<QRhiSampler> otherSampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(otherSampler->create());
    QScopedPointer<QRhiBuffer> buf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 1024));
    QVERIFY(buf->create());
    QScopedPointer<QRhiBuffer> otherBuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 256));
    QVERIFY(otherBuf->create());

    // empty (compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        QVERIFY(srb1->create());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        QVERIFY(srb2->create());

        QVERIFY(srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(srb2->isLayoutCompatible(srb1.data()));

        QCOMPARE(srb1->serializedLayoutDescription(), srb2->serializedLayoutDescription());
        QVERIFY(srb1->serializedLayoutDescription().size() == 0);
    }

    // different count (not compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        QVERIFY(srb1->create());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::sampledTexture(0, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb2->create());

        QVERIFY(!srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(!srb2->isLayoutCompatible(srb1.data()));

        QVERIFY(srb1->serializedLayoutDescription() != srb2->serializedLayoutDescription());
        QVERIFY(srb1->serializedLayoutDescription().size() == 0);
        QVERIFY(srb2->serializedLayoutDescription().size() == 1 * QRhiShaderResourceBinding::LAYOUT_DESC_ENTRIES_PER_BINDING);
    }

    // full match (compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb1->create());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb2->create());

        QVERIFY(srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(srb2->isLayoutCompatible(srb1.data()));

        QVERIFY(!srb1->serializedLayoutDescription().isEmpty());
        QVERIFY(!srb2->serializedLayoutDescription().isEmpty());
        QCOMPARE(srb1->serializedLayoutDescription(), srb2->serializedLayoutDescription());
        QVERIFY(srb1->serializedLayoutDescription().size() == 2 * QRhiShaderResourceBinding::LAYOUT_DESC_ENTRIES_PER_BINDING);

        // see what we would get if a binding list got serialized "manually", without pulling it out from the srb after building
        // (the results should be identical)
        QVector<quint32> layoutDesc1;
        QRhiShaderResourceBinding::serializeLayoutDescription(srb1->cbeginBindings(), srb1->cendBindings(), std::back_inserter(layoutDesc1));
        QCOMPARE(layoutDesc1, srb1->serializedLayoutDescription());
        QVector<quint32> layoutDesc2;
        QRhiShaderResourceBinding::serializeLayoutDescription(srb2->cbeginBindings(), srb2->cendBindings(), std::back_inserter(layoutDesc2));
        QCOMPARE(layoutDesc2, srb2->serializedLayoutDescription());

        // exercise with an "output iterator" different from back_inserter
        quint32 layoutDesc3[2 * QRhiShaderResourceBinding::LAYOUT_DESC_ENTRIES_PER_BINDING];
        QRhiShaderResourceBinding::serializeLayoutDescription(srb1->cbeginBindings(), srb1->cendBindings(), layoutDesc3);
        QVERIFY(!memcmp(layoutDesc3, layoutDesc1.constData(), sizeof(quint32) * 2 * QRhiShaderResourceBinding::LAYOUT_DESC_ENTRIES_PER_BINDING));
    }

    // different visibility (not compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, buf.data()),
                         });
        QVERIFY(srb1->create());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                         });
        QVERIFY(srb2->create());

        QVERIFY(!srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(!srb2->isLayoutCompatible(srb1.data()));

        QVERIFY(srb1->serializedLayoutDescription() != srb2->serializedLayoutDescription());
    }

    // different binding points (not compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                         });
        QVERIFY(srb1->create());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::VertexStage, buf.data()),
                         });
        QVERIFY(srb2->create());

        QVERIFY(!srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(!srb2->isLayoutCompatible(srb1.data()));

        QVERIFY(srb1->serializedLayoutDescription() != srb2->serializedLayoutDescription());
    }

    // different buffer region offset and size (compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data(), rhi->ubufAligned(1), 128),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb1->create());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb2->create());

        QVERIFY(srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(srb2->isLayoutCompatible(srb1.data()));

        QCOMPARE(srb1->serializedLayoutDescription(), srb2->serializedLayoutDescription());
    }

    // different resources (compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, otherBuf.data()),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), otherSampler.data())
                         });
        QVERIFY(srb1->create());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb2->create());

        QVERIFY(srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(srb2->isLayoutCompatible(srb1.data()));

        QCOMPARE(srb1->serializedLayoutDescription(), srb2->serializedLayoutDescription());
    }
}

void tst_QRhi::srbWithNoResource_data()
{
    rhiTestData();
}

void tst_QRhi::srbWithNoResource()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing srb");

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512)));
    QVERIFY(texture->create());
    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->create());
    QScopedPointer<QRhiBuffer> buf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 1024));
    QVERIFY(buf->create());

    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                             QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, nullptr),
                             QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, nullptr, nullptr)
                         });
        QVERIFY(srb1->create());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                             QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                             QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb2->create());

        QVERIFY(srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(srb2->isLayoutCompatible(srb1.data()));
    }
}

void tst_QRhi::renderPassDescriptorCompatibility_data()
{
    rhiTestData();
}

void tst_QRhi::renderPassDescriptorCompatibility()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing renderpass descriptors");

    // Note that checking compatibility is only relevant with backends where
    // there is a concept of renderpass descriptions (Vulkan, and partially
    // Metal). It is perfectly fine for isCompatible() to always return true
    // when that is not the case (D3D11, OpenGL). Hence the 'if (Vulkan or
    // Metal)' for all the negative tests. Also note "partial" for Metal:
    // resolve textures for examples have no effect on compatibility with Metal.

    // tex and tex2 have the same format
    QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
    QVERIFY(tex->create());
    QScopedPointer<QRhiTexture> tex2(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
    QVERIFY(tex2->create());

    QScopedPointer<QRhiRenderBuffer> ds(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, QSize(512, 512)));
    QVERIFY(ds->create());

    // two texture rendertargets with tex and tex2 as color0 (compatible)
    {
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ tex.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->create());

        QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ tex2.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
        rt2->setRenderPassDescriptor(rpDesc2.data());
        QVERIFY(rt2->create());

        QVERIFY(rpDesc->isCompatible(rpDesc2.data()));
        QVERIFY(rpDesc2->isCompatible(rpDesc.data()));
        QCOMPARE(rpDesc->serializedFormat(), rpDesc2->serializedFormat());
    }

    // two texture rendertargets with tex and tex2 as color0, and a depth-stencil attachment as well (compatible)
    {
        QRhiTextureRenderTargetDescription desc({ tex.data() }, ds.data());
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(desc));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->create());

        QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget(desc));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
        rt2->setRenderPassDescriptor(rpDesc2.data());
        QVERIFY(rt2->create());

        QVERIFY(rpDesc->isCompatible(rpDesc2.data()));
        QVERIFY(rpDesc2->isCompatible(rpDesc.data()));
        QCOMPARE(rpDesc->serializedFormat(), rpDesc2->serializedFormat());
    }

    // now one of them does not have the ds attachment (not compatible)
    {
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ { tex.data() }, ds.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->create());

        QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ tex.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
        rt2->setRenderPassDescriptor(rpDesc2.data());
        QVERIFY(rt2->create());

        // these backends have a real concept of rp compatibility, with those we
        // know that incompatibility must be reported; verify this
        if (impl == QRhi::Vulkan || impl == QRhi::Metal) {
            QVERIFY(!rpDesc->isCompatible(rpDesc2.data()));
            QVERIFY(!rpDesc2->isCompatible(rpDesc.data()));
            QVERIFY(!rpDesc->serializedFormat().isEmpty());
            QVERIFY(rpDesc->serializedFormat() != rpDesc2->serializedFormat());
        }
    }

    if (rhi->isFeatureSupported(QRhi::MultisampleRenderBuffer)) {
        // resolve attachments (compatible)
        {
            QScopedPointer<QRhiRenderBuffer> msaaRenderBuffer(rhi->newRenderBuffer(QRhiRenderBuffer::Color, QSize(512, 512), 4));
            QVERIFY(msaaRenderBuffer->create());
            QScopedPointer<QRhiRenderBuffer> msaaRenderBuffer2(rhi->newRenderBuffer(QRhiRenderBuffer::Color, QSize(512, 512), 4));
            QVERIFY(msaaRenderBuffer2->create());

            QRhiColorAttachment colorAtt(msaaRenderBuffer.data()); // color0, multisample
            colorAtt.setResolveTexture(tex.data()); // resolved into a non-msaa texture
            QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ colorAtt }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
            rt->setRenderPassDescriptor(rpDesc.data());
            QVERIFY(rt->create());

            QRhiColorAttachment colorAtt2(msaaRenderBuffer2.data()); // color0, multisample
            colorAtt2.setResolveTexture(tex2.data()); // resolved into a non-msaa texture
            QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ colorAtt2 }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
            rt2->setRenderPassDescriptor(rpDesc2.data());
            QVERIFY(rt2->create());

            QVERIFY(rpDesc->isCompatible(rpDesc2.data()));
            QVERIFY(rpDesc2->isCompatible(rpDesc.data()));
            QCOMPARE(rpDesc->serializedFormat(), rpDesc2->serializedFormat());
        }

        // missing resolve for one of them (not compatible)
        {
            QScopedPointer<QRhiRenderBuffer> msaaRenderBuffer(rhi->newRenderBuffer(QRhiRenderBuffer::Color, QSize(512, 512), 4));
            QVERIFY(msaaRenderBuffer->create());
            QScopedPointer<QRhiRenderBuffer> msaaRenderBuffer2(rhi->newRenderBuffer(QRhiRenderBuffer::Color, QSize(512, 512), 4));
            QVERIFY(msaaRenderBuffer2->create());

            QRhiColorAttachment colorAtt(msaaRenderBuffer.data()); // color0, multisample
            colorAtt.setResolveTexture(tex.data()); // resolved into a non-msaa texture
            QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ colorAtt }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
            rt->setRenderPassDescriptor(rpDesc.data());
            QVERIFY(rt->create());

            QRhiColorAttachment colorAtt2(msaaRenderBuffer2.data()); // color0, multisample
            QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ colorAtt2 }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
            rt2->setRenderPassDescriptor(rpDesc2.data());
            QVERIFY(rt2->create());

            if (impl == QRhi::Vulkan) { // no Metal here
                QVERIFY(!rpDesc->isCompatible(rpDesc2.data()));
                QVERIFY(!rpDesc2->isCompatible(rpDesc.data()));
                QVERIFY(!rpDesc->serializedFormat().isEmpty());
                QVERIFY(rpDesc->serializedFormat() != rpDesc2->serializedFormat());
            }
        }
    } else {
        qDebug("Skipping multisample renderbuffer dependent tests");
    }

    if (rhi->isTextureFormatSupported(QRhiTexture::RGBA32F)) {
        QScopedPointer<QRhiTexture> tex3(rhi->newTexture(QRhiTexture::RGBA32F, QSize(512, 512), 1, QRhiTexture::RenderTarget));
        QVERIFY(tex3->create());

        // different texture formats (not compatible)
        {
            QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ tex.data() }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
            rt->setRenderPassDescriptor(rpDesc.data());
            QVERIFY(rt->create());

            QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ tex3.data() }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
            rt2->setRenderPassDescriptor(rpDesc2.data());
            QVERIFY(rt2->create());

            if (impl == QRhi::Vulkan || impl == QRhi::Metal) {
                QVERIFY(!rpDesc->isCompatible(rpDesc2.data()));
                QVERIFY(!rpDesc2->isCompatible(rpDesc.data()));
                QVERIFY(!rpDesc->serializedFormat().isEmpty());
                QVERIFY(rpDesc->serializedFormat() != rpDesc2->serializedFormat());
            }
        }
    } else {
        qDebug("Skipping texture format dependent tests");
    }

    if (rhi->isFeatureSupported(QRhi::MultiView)) {
        {
            QScopedPointer<QRhiTexture> texArr(rhi->newTextureArray(QRhiTexture::RGBA8, 2, QSize(512, 512), 1, QRhiTexture::RenderTarget));
            QVERIFY(texArr->create());
            QRhiColorAttachment multiViewAtt(texArr.data());
            multiViewAtt.setMultiViewCount(2);
            QRhiTextureRenderTargetDescription rtDesc(multiViewAtt);
            QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
            rt->setRenderPassDescriptor(rpDesc.data());
            QVERIFY(rt->create());

            QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget(rtDesc));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
            rt2->setRenderPassDescriptor(rpDesc2.data());
            QVERIFY(rt2->create());

            QVERIFY(rpDesc->isCompatible(rpDesc2.data()));
            QVERIFY(rpDesc2->isCompatible(rpDesc.data()));
            QCOMPARE(rpDesc->serializedFormat(), rpDesc2->serializedFormat());

            QScopedPointer<QRhiRenderPassDescriptor> rpDescClone(rpDesc->newCompatibleRenderPassDescriptor());
            QVERIFY(rpDesc->isCompatible(rpDescClone.data()));
            QVERIFY(rpDesc2->isCompatible(rpDescClone.data()));
            QCOMPARE(rpDesc->serializedFormat(), rpDescClone->serializedFormat());

            // With Vulkan the multiViewCount really matters since it is baked
            // in to underlying native object (VkRenderPass). Verify that the
            // compatibility check fails when the view count differs. Other
            // backends cannot do this test since they will likely report the
            // rps being compatible regardless.
            if (impl == QRhi::Vulkan) {
                QRhiColorAttachment nonMultiViewAtt(texArr.data());
                QRhiTextureRenderTargetDescription rtDesc3(nonMultiViewAtt);
                QScopedPointer<QRhiTextureRenderTarget> rt3(rhi->newTextureRenderTarget(rtDesc3));
                QScopedPointer<QRhiRenderPassDescriptor> rpDesc3(rt3->newCompatibleRenderPassDescriptor());
                rt3->setRenderPassDescriptor(rpDesc3.data());
                QVERIFY(rt3->create());

                QVERIFY(!rpDesc->isCompatible(rpDesc3.data()));
                QVERIFY(!rpDesc2->isCompatible(rpDesc3.data()));
                QVERIFY(rpDesc->serializedFormat() != rpDesc3->serializedFormat());

                QScopedPointer<QRhiRenderPassDescriptor> rpDesc3Clone(rpDesc3->newCompatibleRenderPassDescriptor());
                QVERIFY(!rpDesc->isCompatible(rpDesc3Clone.data()));
                QVERIFY(!rpDesc2->isCompatible(rpDesc3Clone.data()));
                QVERIFY(rpDesc->serializedFormat() != rpDesc3Clone->serializedFormat());
            }
        }
    } else {
        qDebug("Skipping multiview dependent tests");
    }
}

void tst_QRhi::renderPassDescriptorClone_data()
{
    rhiTestData();
}

void tst_QRhi::renderPassDescriptorClone()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing renderpass descriptors");

    // tex and tex2 have the same format
    QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
    QVERIFY(tex->create());
    QScopedPointer<QRhiTexture> tex2(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
    QVERIFY(tex2->create());

    QScopedPointer<QRhiRenderBuffer> ds(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, QSize(512, 512)));
    QVERIFY(ds->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ tex.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QScopedPointer<QRhiRenderPassDescriptor> rpDescClone(rpDesc->newCompatibleRenderPassDescriptor());
    QVERIFY(rpDescClone);
    QVERIFY(rpDesc->isCompatible(rpDescClone.data()));

    // rt and rt2 have the same set of attachments
    QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ tex2.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
    rt2->setRenderPassDescriptor(rpDesc2.data());
    QVERIFY(rt2->create());

    QVERIFY(rpDesc2->isCompatible(rpDescClone.data()));
}

void tst_QRhi::pipelineCache_data()
{
    rhiTestData();
}

void tst_QRhi::pipelineCache()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QByteArray pcd;
    QShader vs = loadShader(":/data/simple.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/simple.frag.qsb");
    QVERIFY(fs.isValid());
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 2 * sizeof(float) } });
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float2, 0 } });

    {
        QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::EnablePipelineCacheDataSave));
        if (!rhi)
            QSKIP("QRhi could not be created, skipping testing (set)pipelineCacheData()");

        if (!rhi->isFeatureSupported(QRhi::PipelineCacheDataLoadSave))
            QSKIP("PipelineCacheDataLoadSave is not supported with this backend, skipping test");

        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(256, 256), 1, QRhiTexture::RenderTarget));
        QVERIFY(texture->create());
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->create());
        QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
        QVERIFY(srb->create());
        QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
        pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
        pipeline->setVertexInputLayout(inputLayout);
        pipeline->setShaderResourceBindings(srb.data());
        pipeline->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(pipeline->create());

        // This cannot be more than a basic smoketest: ensure that passing
        // in the data we retrieve still gives us successful pipeline
        // creation. What happens internally we cannot check.
        pcd = rhi->pipelineCacheData();
        rhi->setPipelineCacheData(pcd);
        QVERIFY(pipeline->create());
    }

    {
        // Now from scratch, with seeding the cache right from the start,
        // presumably leading to a cache hit when creating the pipeline.
        QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::EnablePipelineCacheDataSave));
        QVERIFY(rhi);
        rhi->setPipelineCacheData(pcd);

        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(256, 256), 1, QRhiTexture::RenderTarget));
        QVERIFY(texture->create());
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->create());
        QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
        QVERIFY(srb->create());
        QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
        pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
        pipeline->setVertexInputLayout(inputLayout);
        pipeline->setShaderResourceBindings(srb.data());
        pipeline->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(pipeline->create());
    }
}

void tst_QRhi::textureWithSampleCount_data()
{
    rhiTestData();
}

void tst_QRhi::textureWithSampleCount()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing renderpass descriptors");

    if (!rhi->isFeatureSupported(QRhi::MultisampleTexture))
        QSKIP("No multisample texture support with this backend, skipping");

    {
        QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1));
        QVERIFY(tex->create());
    }

    // Ensure 0 is accepted the same way as 1.
    {
        QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 0));
        QVERIFY(tex->create());
    }

    // Note that we intentionally do not pass in RenderTarget in flags. Where
    // matters for create(), the backend is expected to act as if it was
    // specified whenever samples > 1. (in practice it does not make sense to not
    // have the flag for an msaa texture, but we only care about create() here)

    // Pick the commonly supported sample count of 4.
    {
        QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 4));
        QVERIFY(tex->create());
    }

    // Now a bogus value that is typically in-between the supported values.
    {
        QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 3));
        QVERIFY(tex->create());
    }

    // Now a bogus value that is out of range.
    {
        QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 123));
        QVERIFY(tex->create());
    }
}

void tst_QRhi::textureFormats_data()
{
    static constexpr QRhiTexture::Format textureFormats[] = {
        QRhiTexture::RGBA8,
        QRhiTexture::BGRA8,
        QRhiTexture::R8,
        QRhiTexture::RG8,
        QRhiTexture::R16,
        QRhiTexture::RG16,
        QRhiTexture::RED_OR_ALPHA8,
        QRhiTexture::RGBA16F,
        QRhiTexture::RGBA32F,
        QRhiTexture::R16F,
        QRhiTexture::R32F,
        QRhiTexture::RGB10A2,
        QRhiTexture::R8SI,
        QRhiTexture::R32SI,
        QRhiTexture::RG32SI,
        QRhiTexture::RGBA32SI,
        QRhiTexture::R8UI,
        QRhiTexture::R32UI,
        QRhiTexture::RG32UI,
        QRhiTexture::RGBA32UI,
        QRhiTexture::D16,
        QRhiTexture::D24,
        QRhiTexture::D24S8,
        QRhiTexture::D32F,
        QRhiTexture::D32FS8,
        QRhiTexture::BC1,
        QRhiTexture::BC2,
        QRhiTexture::BC3,
        QRhiTexture::BC4,
        QRhiTexture::BC5,
        QRhiTexture::BC6H,
        QRhiTexture::BC7,
        QRhiTexture::ETC2_RGB8,
        QRhiTexture::ETC2_RGB8A1,
        QRhiTexture::ETC2_RGBA8,
        QRhiTexture::ASTC_4x4,
        QRhiTexture::ASTC_5x4,
        QRhiTexture::ASTC_5x5,
        QRhiTexture::ASTC_6x5,
        QRhiTexture::ASTC_6x6,
        QRhiTexture::ASTC_8x5,
        QRhiTexture::ASTC_8x6,
        QRhiTexture::ASTC_8x8,
        QRhiTexture::ASTC_10x5,
        QRhiTexture::ASTC_10x6,
        QRhiTexture::ASTC_10x8,
        QRhiTexture::ASTC_10x10,
        QRhiTexture::ASTC_12x10,
        QRhiTexture::ASTC_12x12,
    };

    rhiTestDataWithParam("textureFormat", QSpan(textureFormats), [](const QRhiTexture::Format format) {
        return QString::number(format);
    });
}

void tst_QRhi::textureFormats()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);
    QFETCH(QRhiTexture::Format, textureFormat);

    std::unique_ptr<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing texture formats");

    if (!rhi->isTextureFormatSupported(textureFormat))
        QSKIP("Texture format not supported on this backend");

    std::unique_ptr<QRhiTexture> tex(rhi->newTexture(textureFormat, QSize(512, 512), 1));
    QVERIFY(tex->create());
}

void tst_QRhi::textureImportOpenGL()
{
#ifdef TST_GL
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
        QSKIP("Skipping OpenGL-dependent test");

    QScopedPointer<QRhi> rhi(QRhi::create(QRhi::OpenGLES2, &initParams.gl, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing native texture");

    QVERIFY(rhi->makeThreadLocalNativeContextCurrent());
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QVERIFY(ctx);
    QOpenGLFunctions *f = ctx->functions();

    QImage image(320, 200, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::red);

    GLuint t = 0;
    f->glGenTextures(1, &t);
    f->glBindTexture(GL_TEXTURE_2D, t);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width(), image.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image.constBits());

    QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, image.size()));
    QRhiTexture::NativeTexture nativeTex = { t, 0 };
    QVERIFY(tex->createFrom(nativeTex));
    QCOMPARE(tex->nativeTexture().object, nativeTex.object);

    QRhiReadbackResult readResult;
    bool readCompleted = false;
    readResult.completed = [&readCompleted] { readCompleted = true; };
    QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
    batch->readBackTexture(tex.data(), &readResult);
    QVERIFY(submitResourceUpdates(rhi.data(), batch));
    QVERIFY(readCompleted);
    QCOMPARE(readResult.format, QRhiTexture::RGBA8);
    QCOMPARE(readResult.pixelSize, image.size());
    QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        image.format());
    QVERIFY(imageRGBAEquals(image, wrapperImage));

    f->glDeleteTextures(1, &t);
#endif
}

void tst_QRhi::renderbufferImportOpenGL()
{
#ifdef TST_GL
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
        QSKIP("Skipping OpenGL-dependent test");

    QScopedPointer<QRhi> rhi(QRhi::create(QRhi::OpenGLES2, &initParams.gl, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing native texture");

    QVERIFY(rhi->makeThreadLocalNativeContextCurrent());
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QVERIFY(ctx);
    QOpenGLFunctions *f = ctx->functions();

    const QSize size(320, 200);
    GLuint b = 0;
    f->glGenRenderbuffers(1, &b);
    f->glBindRenderbuffer(GL_RENDERBUFFER, b);
    // in a real world use case this would be some extension, e.g. glEGLImageTargetRenderbufferStorageOES instead
    f->glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA4, size.width(), size.height());
    f->glBindRenderbuffer(GL_RENDERBUFFER, 0);

    QScopedPointer<QRhiRenderBuffer> rb(rhi->newRenderBuffer(QRhiRenderBuffer::Color, size));
    QVERIFY(rb->createFrom({ b }));

    QScopedPointer<QRhiRenderBuffer> depthStencil(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size));
    QVERIFY(depthStencil->create());
    QRhiColorAttachment att(rb.data());
    QRhiTextureRenderTargetDescription rtDesc(att);
    rtDesc.setDepthStencilBuffer(depthStencil.data());
    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
    QScopedPointer<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rp.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);
    cb->beginPass(rt.data(), Qt::red, { 1.0f, 0 }, nullptr, QRhiCommandBuffer::ExternalContent);
    cb->beginExternal();
    QByteArray tmpBuf;
    tmpBuf.resize(size.width() * size.height() * 4);
    f->glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE, tmpBuf.data());
    cb->endExternal();
    cb->endPass();
    rhi->endOffscreenFrame();

    f->glDeleteRenderbuffers(1, &b);

    QImage wrapperImage(reinterpret_cast<const uchar *>(tmpBuf.constData()),
                        size.width(), size.height(), QImage::Format_RGBA8888_Premultiplied);

    QImage image(320, 200, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::red);
    QVERIFY(imageRGBAEquals(image, wrapperImage));
#endif
}

void tst_QRhi::threeDimTexture_data()
{
    rhiTestData();
}

void tst_QRhi::threeDimTexture()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing 3D textures");

    if (!rhi->isFeatureSupported(QRhi::ThreeDimensionalTextures))
        QSKIP("Skipping testing 3D textures because they are reported as unsupported");

    if (isAndroidOpenGLSwiftShader(impl, rhi.get())) {
        QSKIP("SwiftShader software acceleration is used which does not support this OpenGLES "
            "feature. See QTBUG-132934");
    }

    const int WIDTH = 512;
    const int HEIGHT = 256;
    const int DEPTH = 128;

    {
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, WIDTH, HEIGHT, DEPTH));
        QVERIFY(texture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        for (int i = 0; i < DEPTH; ++i) {
            QImage img(WIDTH, HEIGHT, QImage::Format_RGBA8888);
            img.fill(QColor::fromRgb(i * 2, 0, 0));
            QRhiTextureUploadEntry sliceUpload(i, 0, QRhiTextureSubresourceUploadDescription(img));
            batch->uploadTexture(texture.data(), sliceUpload);
        }

        QVERIFY(submitResourceUpdates(rhi.data(), batch));
    }

    // mipmaps
    if (rhi->isFeatureSupported(QRhi::ThreeDimensionalTextureMipmaps)) {
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, WIDTH, HEIGHT, DEPTH,
                                                            1, QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips));
        QVERIFY(texture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        for (int i = 0; i < DEPTH; ++i) {
            QImage img(WIDTH, HEIGHT, QImage::Format_RGBA8888);
            img.fill(QColor::fromRgb(i * 2, 0, 0));
            QRhiTextureUploadEntry sliceUpload(i, 0, QRhiTextureSubresourceUploadDescription(img));
            batch->uploadTexture(texture.data(), sliceUpload);
        }

        batch->generateMips(texture.data());

        QVERIFY(submitResourceUpdates(rhi.data(), batch));

        // read back slice 63 of level 1 (256x128, almost red)
        batch = rhi->nextResourceUpdateBatch();
        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readResult, &result] {
            result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        };
        QRhiReadbackDescription readbackDescription(texture.data());
        readbackDescription.setLevel(1);
        readbackDescription.setLayer(63);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        QImage referenceImage(WIDTH / 2, HEIGHT / 2, result.format());
        referenceImage.fill(QColor::fromRgb(253, 0, 0));

        // Now restrict the test a bit. The Null QRhi backend has broken support for
        // mipmap generation of 3D textures (it ignores the depth, effectively behaving as
        // if the 3D texture was a 2D array which is incorrect wrt mipmapping)
        // Some software-based OpenGL implementations, such as Mesa llvmpipe builds that are
        // used both in Qt CI and are shipped with the official Qt binaries also seem to have
        // problems with this.
        if (impl != QRhi::Null && impl != QRhi::OpenGLES2)
            QVERIFY(imageRGBAEquals(result, referenceImage, 2));
    } else {
        qDebug("Skipping 3D texture mipmap generation test because it is reported as unsupported");
    }

    // render target (one slice)
    // NB with Vulkan we require Vulkan 1.1 for this to work.
    {
        const int SLICE = 23;
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, WIDTH, HEIGHT, DEPTH,
                                                            1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        QRhiColorAttachment att(texture.data());
        att.setLayer(SLICE);
        QRhiTextureRenderTargetDescription rtDesc(att);
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
        QScopedPointer<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rp.data());
        QVERIFY(rt->create());

        // render to slice 23
        QRhiCommandBuffer *cb = nullptr;
        QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
        QVERIFY(cb);
        cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 });
        // slice 23 is now blue
        cb->endPass();
        rhi->endOffscreenFrame();

        // Fill all other slices with some color. We should be free to do this
        // step *before* the "render to slice 23" block above as well. However,
        // as QTBUG-111772 shows, some Vulkan implementations have problems
        // then. (or it could be QRhi is doing something wrong, but there is no
        // evidence of that yet) For now, keep the order of first rendering to
        // a slice and then uploading data for the rest.
        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);
        for (int i = 0; i < DEPTH; ++i) {
            if (i != SLICE) {
                QImage img(WIDTH, HEIGHT, QImage::Format_RGBA8888);
                img.fill(QColor::fromRgb(i * 2, 0, 0));
                QRhiTextureUploadEntry sliceUpload(i, 0, QRhiTextureSubresourceUploadDescription(img));
                batch->uploadTexture(texture.data(), sliceUpload);
            }
        }
        QVERIFY(submitResourceUpdates(rhi.data(), batch));

        // read back slice 23 (blue)
        batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);
        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readResult, &result] {
            result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        };
        QRhiReadbackDescription readbackDescription(texture.data());
        readbackDescription.setLayer(23);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        QImage referenceImage(WIDTH, HEIGHT, result.format());
        referenceImage.fill(QColor::fromRgbF(0.0f, 0.0f, 1.0f));
        // the Null backend does not render so skip the verification for that
        if (impl != QRhi::Null)
            QVERIFY(imageRGBAEquals(result, referenceImage));

        // read back slice 0 (black)
        batch = rhi->nextResourceUpdateBatch();
        result = QImage();
        readbackDescription.setLayer(0);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        referenceImage.fill(QColor::fromRgbF(0.0f, 0.0f, 0.0f));
        QVERIFY(imageRGBAEquals(result, referenceImage));

        // read back slice 127 (almost red)
        batch = rhi->nextResourceUpdateBatch();
        result = QImage();
        readbackDescription.setLayer(127);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        referenceImage.fill(QColor::fromRgb(254, 0, 0));
        QVERIFY(imageRGBAEquals(result, referenceImage));
    }
}
void tst_QRhi::oneDimTexture_data()
{
    rhiTestData();
}

void tst_QRhi::oneDimTexture()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing 1D textures");

    if (!rhi->isFeatureSupported(QRhi::OneDimensionalTextures))
        QSKIP("Skipping testing 1D textures because they are reported as unsupported");

    const int WIDTH = 512;
    const int LAYERS = 128;

    {
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, WIDTH, 0, 0));
        QVERIFY(texture->create());

        QVERIFY(texture->flags().testFlag(QRhiTexture::Flag::OneDimensional));

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        QImage img(WIDTH, 1, QImage::Format_RGBA8888);
        img.fill(QColor::fromRgb(255, 0, 0));

        QRhiTextureUploadEntry upload(0, 0, QRhiTextureSubresourceUploadDescription(img));
        batch->uploadTexture(texture.data(), upload);

        QVERIFY(submitResourceUpdates(rhi.data(), batch));
    }

    {
        QScopedPointer<QRhiTexture> texture(
                rhi->newTextureArray(QRhiTexture::RGBA8, LAYERS, QSize(WIDTH, 0)));
        QVERIFY(texture->create());

        QVERIFY(texture->flags().testFlag(QRhiTexture::Flag::OneDimensional));
        QVERIFY(texture->flags().testFlag(QRhiTexture::Flag::TextureArray));

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        for (int i = 0; i < LAYERS; ++i) {
            QImage img(WIDTH, 1, QImage::Format_RGBA8888);
            img.fill(QColor::fromRgb(i * 2, 0, 0));
            QRhiTextureUploadEntry layerUpload(i, 0, QRhiTextureSubresourceUploadDescription(img));
            batch->uploadTexture(texture.data(), layerUpload);
        }

        QVERIFY(submitResourceUpdates(rhi.data(), batch));
    }

    // Copy from 2D texture to 1D texture
    {
        const int WIDTH = 256;
        const int HEIGHT = 256;

        QScopedPointer<QRhiTexture> srcTexture(rhi->newTexture(
                QRhiTexture::RGBA8, WIDTH, HEIGHT, 0, 1, QRhiTexture::Flag::UsedAsTransferSource));
        QVERIFY(srcTexture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        QImage img(WIDTH, HEIGHT, QImage::Format_RGBA8888);
        for (int x = 0; x < WIDTH; ++x) {
            for (int y = 0; y < HEIGHT; ++y) {
                img.setPixelColor(x, y, QColor::fromRgb(x, y, 0));
            }
        }
        QRhiTextureUploadEntry upload(0, 0, QRhiTextureSubresourceUploadDescription(img));
        batch->uploadTexture(srcTexture.data(), upload);

        QScopedPointer<QRhiTexture> dstTexture(rhi->newTexture(
                QRhiTexture::RGBA8, WIDTH, 0, 0, 1, QRhiTexture::Flag::UsedAsTransferSource));
        QVERIFY(dstTexture->create());

        QRhiTextureCopyDescription copy;
        copy.setPixelSize(QSize(WIDTH / 2, 1));
        copy.setDestinationTopLeft(QPoint(WIDTH / 2, 0));
        copy.setSourceTopLeft(QPoint(33, 67));
        batch->copyTexture(dstTexture.data(), srcTexture.data(), copy);

        copy.setDestinationTopLeft(QPoint(0, 0));
        copy.setSourceTopLeft(QPoint(99, 12));
        batch->copyTexture(dstTexture.data(), srcTexture.data(), copy);

        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readResult, &result] {
            result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        };

        QRhiReadbackDescription readbackDescription(dstTexture.data());
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        QImage referenceImage(WIDTH, 1, result.format());
        for (int i = 0; i < WIDTH / 2; ++i) {
            referenceImage.setPixelColor(i, 0, img.pixelColor(99 + i, 12));
            referenceImage.setPixelColor(WIDTH / 2 + i, 0, img.pixelColor(33 + i, 67));
        }

        QVERIFY(imageRGBAEquals(result, referenceImage));
    }

    // Copy from 2D texture to 1D texture array
    {
        const int WIDTH = 256;
        const int HEIGHT = 256;
        const int LAYERS = 64;

        QScopedPointer<QRhiTexture> srcTexture(rhi->newTexture(
                QRhiTexture::RGBA8, WIDTH, HEIGHT, 0, 1, QRhiTexture::Flag::UsedAsTransferSource));
        QVERIFY(srcTexture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        QImage img(WIDTH, HEIGHT, QImage::Format_RGBA8888);
        for (int x = 0; x < WIDTH; ++x) {
            for (int y = 0; y < HEIGHT; ++y) {
                img.setPixelColor(x, y, QColor::fromRgb(x, y, 0));
            }
        }
        QRhiTextureUploadEntry upload(0, 0, QRhiTextureSubresourceUploadDescription(img));
        batch->uploadTexture(srcTexture.data(), upload);

        QScopedPointer<QRhiTexture> dstTexture(
                rhi->newTextureArray(QRhiTexture::RGBA8, LAYERS, QSize(WIDTH, 0), 1,
                                     QRhiTexture::Flag::UsedAsTransferSource));
        QVERIFY(dstTexture->create());

        QRhiTextureCopyDescription copy;
        copy.setPixelSize(QSize(WIDTH / 2, 1));
        copy.setDestinationTopLeft(QPoint(WIDTH / 2, 0));
        copy.setSourceTopLeft(QPoint(33, 67));
        copy.setDestinationLayer(12);
        batch->copyTexture(dstTexture.data(), srcTexture.data(), copy);

        copy.setDestinationTopLeft(QPoint(0, 0));
        copy.setSourceTopLeft(QPoint(99, 12));
        batch->copyTexture(dstTexture.data(), srcTexture.data(), copy);

        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readResult, &result] {
            result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        };

        QRhiReadbackDescription readbackDescription(dstTexture.data());
        readbackDescription.setLayer(12);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        QImage referenceImage(WIDTH, 1, result.format());
        for (int i = 0; i < WIDTH / 2; ++i) {
            referenceImage.setPixelColor(i, 0, img.pixelColor(99 + i, 12));
            referenceImage.setPixelColor(WIDTH / 2 + i, 0, img.pixelColor(33 + i, 67));
        }

        QVERIFY(imageRGBAEquals(result, referenceImage));
    }

    // Copy from 1D texture array to 1D texture
    {
        const int WIDTH = 256;
        const int LAYERS = 256;

        QScopedPointer<QRhiTexture> srcTexture(
                rhi->newTextureArray(QRhiTexture::RGBA8, LAYERS, QSize(WIDTH, 0), 1,
                                     QRhiTexture::Flag::UsedAsTransferSource));
        QVERIFY(srcTexture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        for (int y = 0; y < LAYERS; ++y) {
            QImage img(WIDTH, 1, QImage::Format_RGBA8888);
            for (int x = 0; x < WIDTH; ++x) {
                img.setPixelColor(x, 0, QColor::fromRgb(x, y, 0));
            }
            QRhiTextureUploadEntry upload(y, 0, QRhiTextureSubresourceUploadDescription(img));
            batch->uploadTexture(srcTexture.data(), upload);
        }

        QScopedPointer<QRhiTexture> dstTexture(rhi->newTexture(
                QRhiTexture::RGBA8, WIDTH, 0, 0, 1, QRhiTexture::Flag::UsedAsTransferSource));
        QVERIFY(dstTexture->create());

        QRhiTextureCopyDescription copy;
        copy.setPixelSize(QSize(WIDTH / 2, 1));
        copy.setDestinationTopLeft(QPoint(WIDTH / 2, 0));
        copy.setSourceLayer(67);
        copy.setSourceTopLeft(QPoint(33, 0));
        batch->copyTexture(dstTexture.data(), srcTexture.data(), copy);

        copy.setDestinationTopLeft(QPoint(0, 0));
        copy.setSourceLayer(12);
        copy.setSourceTopLeft(QPoint(99, 0));
        batch->copyTexture(dstTexture.data(), srcTexture.data(), copy);

        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readResult, &result] {
            result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        };

        QRhiReadbackDescription readbackDescription(dstTexture.data());
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        QImage referenceImage(WIDTH, 1, result.format());
        for (int i = 0; i < WIDTH / 2; ++i) {
            referenceImage.setPixelColor(i, 0, QColor::fromRgb(99 + i, 12, 0));
            referenceImage.setPixelColor(WIDTH / 2 + i, 0, QColor::fromRgb(33 + i, 67, 0));
        }

        QVERIFY(imageRGBAEquals(result, referenceImage));
    }

    // Copy from 1D texture to 1D texture array
    {
        const int WIDTH = 256;
        const int LAYERS = 256;

        QScopedPointer<QRhiTexture> srcTexture(rhi->newTexture(
                QRhiTexture::RGBA8, WIDTH, 0, 0, 1, QRhiTexture::Flag::UsedAsTransferSource));
        QVERIFY(srcTexture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        QImage img(WIDTH, 1, QImage::Format_RGBA8888);
        for (int x = 0; x < WIDTH; ++x) {
            img.setPixelColor(x, 0, QColor::fromRgb(x, 0, 0));
        }
        QRhiTextureUploadEntry upload(0, 0, QRhiTextureSubresourceUploadDescription(img));
        batch->uploadTexture(srcTexture.data(), upload);

        QScopedPointer<QRhiTexture> dstTexture(
                rhi->newTextureArray(QRhiTexture::RGBA8, LAYERS, QSize(WIDTH, 0), 1,
                                     QRhiTexture::Flag::UsedAsTransferSource));
        QVERIFY(dstTexture->create());

        QRhiTextureCopyDescription copy;
        copy.setPixelSize(QSize(WIDTH / 2, 1));
        copy.setDestinationTopLeft(QPoint(WIDTH / 2, 0));
        copy.setDestinationLayer(67);
        copy.setSourceTopLeft(QPoint(33, 0));
        batch->copyTexture(dstTexture.data(), srcTexture.data(), copy);

        copy.setDestinationTopLeft(QPoint(0, 0));
        copy.setSourceTopLeft(QPoint(99, 0));
        batch->copyTexture(dstTexture.data(), srcTexture.data(), copy);

        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readResult, &result] {
            result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        };

        QRhiReadbackDescription readbackDescription(dstTexture.data());
        readbackDescription.setLayer(67);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        QImage referenceImage(WIDTH, 1, result.format());
        for (int i = 0; i < WIDTH / 2; ++i) {
            referenceImage.setPixelColor(i, 0, QColor::fromRgb(99 + i, 0, 0));
            referenceImage.setPixelColor(WIDTH / 2 + i, 0, QColor::fromRgb(33 + i, 0, 0));
        }

        QVERIFY(imageRGBAEquals(result, referenceImage));
    }

    // mipmaps and 1D render target
    if (!rhi->isFeatureSupported(QRhi::OneDimensionalTextureMipmaps)
            || !rhi->isFeatureSupported(QRhi::RenderToOneDimensionalTexture))
    {
        QSKIP("Skipping testing 1D texture mipmaps and 1D render target because they are reported as unsupported");
    }

    {
        QScopedPointer<QRhiTexture> texture(
                rhi->newTexture(QRhiTexture::RGBA8, WIDTH, 0, 0, 1,
                                QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips));
        QVERIFY(texture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        QImage img(WIDTH, 1, QImage::Format_RGBA8888);
        img.fill(QColor::fromRgb(128, 0, 0));
        QRhiTextureUploadEntry upload(0, 0, QRhiTextureSubresourceUploadDescription(img));
        batch->uploadTexture(texture.data(), upload);

        batch->generateMips(texture.data());

        QVERIFY(submitResourceUpdates(rhi.data(), batch));

        // read back level 1 (256x1, #800000ff)
        batch = rhi->nextResourceUpdateBatch();
        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readResult, &result] {
            result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        };
        QRhiReadbackDescription readbackDescription(texture.data());
        readbackDescription.setLevel(1);
        readbackDescription.setLayer(0);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        QImage referenceImage(WIDTH / 2, 1, result.format());
        referenceImage.fill(QColor::fromRgb(128, 0, 0));

        QVERIFY(imageRGBAEquals(result, referenceImage, 2));
    }

    {
        QScopedPointer<QRhiTexture> texture(
                rhi->newTextureArray(QRhiTexture::RGBA8, LAYERS, QSize(WIDTH, 0), 1,
                                     QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips));
        QVERIFY(texture->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        for (int i = 0; i < LAYERS; ++i) {
            QImage img(WIDTH, 1, QImage::Format_RGBA8888);
            img.fill(QColor::fromRgb(i * 2, 0, 0));
            QRhiTextureUploadEntry sliceUpload(i, 0, QRhiTextureSubresourceUploadDescription(img));
            batch->uploadTexture(texture.data(), sliceUpload);
        }

        batch->generateMips(texture.data());

        QVERIFY(submitResourceUpdates(rhi.data(), batch));

        // read back slice 63 of level 1 (256x1, #7E0000FF)
        batch = rhi->nextResourceUpdateBatch();
        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readResult, &result] {
            result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        };
        QRhiReadbackDescription readbackDescription(texture.data());
        readbackDescription.setLevel(1);
        readbackDescription.setLayer(63);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        QImage referenceImage(WIDTH / 2, 1, result.format());
        referenceImage.fill(QColor::fromRgb(126, 0, 0));

        // Now restrict the test a bit. The Null QRhi backend has broken support for
        // mipmap generation of 1D texture arrays.
        if (impl != QRhi::Null)
            QVERIFY(imageRGBAEquals(result, referenceImage, 2));
    }

    // 1D texture render target
    // NB with Vulkan we require Vulkan 1.1 for this to work.
    // Metal does not allow 1D texture render targets
    {
        QScopedPointer<QRhiTexture> texture(
                rhi->newTexture(QRhiTexture::RGBA8, WIDTH, 0, 0, 1,
                                QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        QRhiColorAttachment att(texture.data());
        QRhiTextureRenderTargetDescription rtDesc(att);
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
        QScopedPointer<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rp.data());
        QVERIFY(rt->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        QImage img(WIDTH, 1, QImage::Format_RGBA8888);
        img.fill(QColor::fromRgb(128, 0, 0));
        QRhiTextureUploadEntry upload(0, 0, QRhiTextureSubresourceUploadDescription(img));
        batch->uploadTexture(texture.data(), upload);

        QRhiCommandBuffer *cb = nullptr;
        QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
        QVERIFY(cb);
        cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, batch);
        // texture is now blue
        cb->endPass();
        rhi->endOffscreenFrame();

        // read back texture (blue)
        batch = rhi->nextResourceUpdateBatch();
        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readResult, &result] {
            result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        };
        QRhiReadbackDescription readbackDescription(texture.data());
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        QImage referenceImage(WIDTH, 1, result.format());
        referenceImage.fill(QColor::fromRgbF(0.0f, 0.0f, 1.0f));
        // the Null backend does not render so skip the verification for that
        if (impl != QRhi::Null)
            QVERIFY(imageRGBAEquals(result, referenceImage));
    }

    // 1D array texture render target (one slice)
    // NB with Vulkan we require Vulkan 1.1 for this to work.
    // Metal does not allow 1D texture render targets
    {
        const int SLICE = 23;
        QScopedPointer<QRhiTexture> texture(rhi->newTextureArray(
                QRhiTexture::RGBA8, LAYERS, QSize(WIDTH, 0), 1,
                QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->create());

        QRhiColorAttachment att(texture.data());
        att.setLayer(SLICE);
        QRhiTextureRenderTargetDescription rtDesc(att);
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(rtDesc));
        QScopedPointer<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rp.data());
        QVERIFY(rt->create());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        for (int i = 0; i < LAYERS; ++i) {
            QImage img(WIDTH, 1, QImage::Format_RGBA8888);
            img.fill(QColor::fromRgb(i * 2, 0, 0));
            QRhiTextureUploadEntry sliceUpload(i, 0, QRhiTextureSubresourceUploadDescription(img));
            batch->uploadTexture(texture.data(), sliceUpload);
        }

        QRhiCommandBuffer *cb = nullptr;
        QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
        QVERIFY(cb);
        cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, batch);
        // slice 23 is now blue
        cb->endPass();
        rhi->endOffscreenFrame();

        // read back slice 23 (blue)
        batch = rhi->nextResourceUpdateBatch();
        QRhiReadbackResult readResult;
        QImage result;
        readResult.completed = [&readResult, &result] {
            result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        };
        QRhiReadbackDescription readbackDescription(texture.data());
        readbackDescription.setLayer(23);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        QImage referenceImage(WIDTH, 1, result.format());
        referenceImage.fill(QColor::fromRgbF(0.0f, 0.0f, 1.0f));
        // the Null backend does not render so skip the verification for that
        if (impl != QRhi::Null)
            QVERIFY(imageRGBAEquals(result, referenceImage));

        // read back slice 0 (black)
        batch = rhi->nextResourceUpdateBatch();
        result = QImage();
        readbackDescription.setLayer(0);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        referenceImage.fill(QColor::fromRgbF(0.0f, 0.0f, 0.0f));
        QVERIFY(imageRGBAEquals(result, referenceImage));

        // read back slice 127 (almost red)
        batch = rhi->nextResourceUpdateBatch();
        result = QImage();
        readbackDescription.setLayer(127);
        batch->readBackTexture(readbackDescription, &readResult);
        QVERIFY(submitResourceUpdates(rhi.data(), batch));
        QVERIFY(!result.isNull());
        referenceImage.fill(QColor::fromRgb(254, 0, 0));
        QVERIFY(imageRGBAEquals(result, referenceImage));
    }
}

void tst_QRhi::leakedResourceDestroy_data()
{
    rhiTestData();
}

void tst_QRhi::leakedResourceDestroy()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping");

    // Incorrectly destroy the QRhi before the resources created from it.  Attempting to
    // destroy the resources afterwards is pointless, the native resources are leaked.
    // Nonetheless, it should not crash, which is what we are testing here.
    //
    // We do not however have control over other, native and 3rd party components: a
    // validation or debug layer, or a memory allocator may warn, assert, or abort when
    // not releasing all native resources correctly.
#ifndef QT_NO_DEBUG
    // don't want asserts from vkmemalloc, skip the test in debug builds
    if (impl == QRhi::Vulkan)
        QSKIP("Skipping leaked resource destroy test due to Vulkan and debug build");
#endif

    QScopedPointer<QRhiBuffer> buffer(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, 256));
    QVERIFY(buffer->create());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    QVERIFY(rpDesc);
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiRenderBuffer *rb = rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, QSize(512, 512));
    QVERIFY(rb->create());

    QRhiShaderResourceBindings *srb = rhi->newShaderResourceBindings();
    QVERIFY(srb->create());

    if (impl == QRhi::Vulkan)
        qDebug("Vulkan validation layer warnings may be printed below - this is expected");

    if (impl == QRhi::D3D12)
        qDebug("QD3D12CpuDescriptorPool warnings may be printed below - this is expected");

    qDebug("QRhi resource leak check warnings may be printed below - this is expected");

    // make the QRhi go away early
    rhi.reset();

    // see if the internal rhi backpointer got nulled out
    QVERIFY(buffer->rhi() == nullptr);
    QVERIFY(texture->rhi() == nullptr);
    QVERIFY(rt->rhi() == nullptr);
    QVERIFY(rpDesc->rhi() == nullptr);
    QVERIFY(rb->rhi() == nullptr);
    QVERIFY(srb->rhi() == nullptr);

    // test out deleteLater on some of the resources
    rb->deleteLater();
    srb->deleteLater();

    // let the scoped ptr do its job with the rest
}

void tst_QRhi::renderToFloatTexture_data()
{
    rhiTestData();
}

void tst_QRhi::renderToFloatTexture()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    if (!rhi->isTextureFormatSupported(QRhiTexture::RGBA16F))
        QSKIP("RGBA16F is not supported, skipping test");

    if (isAndroidOpenGLSwiftShader(impl, rhi.get())) {
        QSKIP("SwiftShader software acceleration is used which does not support this OpenGLES "
              "feature. See QTBUG-132934");
    }

    const QSize outputSize(1920, 1080);
    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA16F, outputSize, 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createSimplePipeline(rhi.data(), srb.data(), rpDesc.data()));
    QVERIFY(pipeline);

    cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA16FPx4);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();
    QCOMPARE(result.size(), texture->pixelSize());

    if (impl == QRhi::Null)
        return;

    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        result.flip();

    // Now we have a red rectangle on blue background.
    const int y = 100;
    const QRgbaFloat16 *p = reinterpret_cast<const QRgbaFloat16 *>(result.constScanLine(y));
    int redCount = 0;
    int blueCount = 0;
    int x = result.width() - 1;
    while (x-- >= 0) {
        QRgbaFloat16 c = *p++;
        if (c.red() >= 0.95f && qFuzzyIsNull(c.green()) && qFuzzyIsNull(c.blue()))
            ++redCount;
        else if (qFuzzyIsNull(c.red()) && qFuzzyIsNull(c.green()) && c.blue() >= 0.95f)
            ++blueCount;
        else
            QFAIL("Encountered a pixel that is neither red or blue");
    }
    QCOMPARE(redCount + blueCount, texture->pixelSize().width());
    QVERIFY(redCount > blueCount); // 1742 > 178
}

void tst_QRhi::renderToRgb10Texture_data()
{
    rhiTestData();
}

void tst_QRhi::renderToRgb10Texture()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    if (!rhi->isTextureFormatSupported(QRhiTexture::RGB10A2))
        QSKIP("RGB10A2 is not supported, skipping test");

    if (isAndroidOpenGLSwiftShader(impl, rhi.get())) {
        QSKIP("SwiftShader software acceleration is used which does not support this OpenGLES "
              "feature. See QTBUG-132934");
    }

    const QSize outputSize(1920, 1080);
    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGB10A2, outputSize, 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(createSimplePipeline(rhi.data(), srb.data(), rpDesc.data()));
    QVERIFY(pipeline);

    cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_A2BGR30_Premultiplied);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();
    QCOMPARE(result.size(), texture->pixelSize());

    if (impl == QRhi::Null)
        return;

    if (rhi->isYUpInFramebuffer() != rhi->isYUpInNDC())
        result.flip();

    // Now we have a red rectangle on blue background.
    const int y = 100;
    int redCount = 0;
    int blueCount = 0;
    const int maxFuzz = 1;
    for (int x = 0; x < result.width(); ++x) {
        QRgb c = result.pixel(x, y);
        if (qRed(c) >= (255 - maxFuzz) && qGreen(c) == 0 && qBlue(c) == 0)
            ++redCount;
        else if (qRed(c) == 0 && qGreen(c) == 0 && qBlue(c) >= (255 - maxFuzz))
            ++blueCount;
        else
            QFAIL("Encountered a pixel that is neither red or blue");
    }
    QCOMPARE(redCount + blueCount, texture->pixelSize().width());
    QVERIFY(redCount > blueCount); // 1742 > 178
}

void tst_QRhi::tessellation_data()
{
    rhiTestData();
}

void tst_QRhi::tessellation()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-108844)");
#endif
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    if (!rhi->isFeatureSupported(QRhi::Tessellation)) {
        // From a Vulkan or Metal implementation we expect tessellation to work,
        // even though it is optional (as per spec) for Vulkan.
        QVERIFY(rhi->backend() != QRhi::Vulkan);
        QVERIFY(rhi->backend() != QRhi::Metal);
        QSKIP("Tessellation is not supported with this graphics API, skipping test");
    }

    if (rhi->backend() == QRhi::D3D11 || rhi->backend() == QRhi::D3D12)
        QSKIP("Skipping tessellation test on D3D for now, test assets not prepared for HLSL yet");

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(1280, 720), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    static const float triangleVertices[] = {
        0.0f, 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.0f,    0.0f, 1.0f, 0.0f,
    };

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    u->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    QVERIFY(ubuf->create());

    // Use the 3D API specific correction matrix that flips Y, so we can use
    // the OpenGL-targeted vertex data and the tessellation winding order of
    // counter-clockwise to get uniform results.
    QMatrix4x4 mvp = rhi->clipSpaceCorrMatrix();
    u->updateDynamicBuffer(ubuf.data(), 0, 64, mvp.constData());

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::TessellationEvaluationStage, ubuf.data()),
                     });
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());

    pipeline->setTopology(QRhiGraphicsPipeline::Patches);
    pipeline->setPatchControlPointCount(3);

    pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, loadShader(":/data/simpletess.vert.qsb") },
        { QRhiShaderStage::TessellationControl, loadShader(":/data/simpletess.tesc.qsb") },
        { QRhiShaderStage::TessellationEvaluation, loadShader(":/data/simpletess.tese.qsb") },
        { QRhiShaderStage::Fragment, loadShader(":/data/simpletess.frag.qsb") }
    });

    pipeline->setCullMode(QRhiGraphicsPipeline::Back); // to ensure the winding order is correct

    // won't get the wireframe with OpenGL ES
    if (rhi->isFeatureSupported(QRhi::NonFillPolygonMode))
        pipeline->setPolygonMode(QRhiGraphicsPipeline::Line);

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 6 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) }
    });

    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    QRhiCommandBuffer *cb = nullptr;
    QCOMPARE(rhi->beginOffscreenFrame(&cb), QRhi::FrameOpSuccess);

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(rt->pixelSize().width()), float(rt->pixelSize().height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    if (rhi->isYUpInFramebuffer()) // we used clipSpaceCorrMatrix so this is different from many other tests
        result.flip();

    QCOMPARE(result.size(), rt->pixelSize());

    // cannot check rendering results with Null, because there is no rendering there
    if (impl == QRhi::Null)
        return;

    int redCount = 0, greenCount = 0, blueCount = 0;
    for (int y = 0; y < result.height(); ++y) {
        const quint32 *p = reinterpret_cast<const quint32 *>(result.constScanLine(y));
        int x = result.width() - 1;
        while (x-- >= 0) {
            const QRgb c(*p++);
            const int red = qRed(c);
            const int green = qGreen(c);
            const int blue = qBlue(c);
            // just count the color components that are above a certain threshold
            if (red > 240)
                ++redCount;
            if (green > 240)
                ++greenCount;
            if (blue > 240)
                ++blueCount;
        }
    }

    // Line drawing can be different between the 3D APIs. What we will check if
    // the number of strong-enough r/g/b components above a certain threshold.
    // That is good enough to ensure that something got rendered, i.e. that
    // tessellation is not completely broken.
    //
    // For the record the actual values are something like:
    // OpenGL (NVIDIA, Windows) 59 82 82
    // Metal (Intel, macOS 12.5) 59 79 79
    // Vulkan (NVIDIA, Windows) 71 85 85

    QVERIFY(redCount > 50);
    QVERIFY(blueCount > 50);
    QVERIFY(greenCount > 50);
}

void tst_QRhi::tessellationInterfaceBlocks_data()
{
    rhiTestData();
}

void tst_QRhi::tessellationInterfaceBlocks()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-108844)");
#endif
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    // This test is intended for Metal, but will run on other tessellation render pipelines
    //
    // Metal tessellation uses a combination of compute pipelines for the vert and tesc, and a
    // render pipeline for the tese and frag. This test uses input output interface blocks between
    // the tesc and tese, and all tese stage builtin inputs to check that the Metal tese-frag
    // pipeline vertex inputs are correctly configured. The tese writes the values to a storage
    // buffer whose values are checked by the unit test. MSL 2.1 is required for this test.
    // (requires support for writing to a storage buffer in the vertex shader within a render
    // pipeline)

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    if (!rhi->isFeatureSupported(QRhi::Tessellation)) {
        // From a Vulkan or Metal implementation we expect tessellation to work,
        // even though it is optional (as per spec) for Vulkan.
        QVERIFY(rhi->backend() != QRhi::Vulkan);
        QVERIFY(rhi->backend() != QRhi::Metal);
        QSKIP("Tessellation is not supported with this graphics API, skipping test");
    }

    if (rhi->backend() == QRhi::D3D11 || rhi->backend() == QRhi::D3D12)
        QSKIP("Skipping tessellation test on D3D for now, test assets not prepared for HLSL yet");

    if (rhi->backend() == QRhi::OpenGLES2)
        QSKIP("Skipping test on OpenGL as gl_ClipDistance[] support inconsistent");

    QScopedPointer<QRhiTexture> texture(
            rhi->newTexture(QRhiTexture::RGBA8, QSize(1280, 720), 1,
                            QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    static const float triangleVertices[] = {
        0.0f, 0.5f, 0.0f, 0.0f, 0.0f,  1.0f, -0.5f, -0.5f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f,  1.0f,  0.0f,
    };

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                                   sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    u->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiBuffer> ubuf(
            rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    QVERIFY(ubuf->create());

    // Use the 3D API specific correction matrix that flips Y, so we can use
    // the OpenGL-targeted vertex data and the tessellation winding order of
    // counter-clockwise to get uniform results.
    QMatrix4x4 mvp = rhi->clipSpaceCorrMatrix();
    u->updateDynamicBuffer(ubuf.data(), 0, 64, mvp.constData());

    QScopedPointer<QRhiBuffer> buffer(
            rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::UsageFlag::StorageBuffer, 2048));
    QVERIFY(buffer->create());

    // 2048 is a large buffer for RUB
    u->uploadStaticBuffer(buffer.data(), QByteArray(2048, 0));

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings(
            { QRhiShaderResourceBinding::uniformBuffer(
                      0, QRhiShaderResourceBinding::TessellationEvaluationStage, ubuf.data()),
              QRhiShaderResourceBinding::bufferLoadStore(
                      1, QRhiShaderResourceBinding::TessellationEvaluationStage, buffer.data()) });
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());

    pipeline->setTopology(QRhiGraphicsPipeline::Patches);
    pipeline->setPatchControlPointCount(3);

    pipeline->setShaderStages(
            { { QRhiShaderStage::Vertex, loadShader(":/data/tessinterfaceblocks.vert.qsb") },
              { QRhiShaderStage::TessellationControl,
                loadShader(":/data/tessinterfaceblocks.tesc.qsb") },
              { QRhiShaderStage::TessellationEvaluation,
                loadShader(":/data/tessinterfaceblocks.tese.qsb") },
              { QRhiShaderStage::Fragment, loadShader(":/data/tessinterfaceblocks.frag.qsb") } });

    pipeline->setCullMode(QRhiGraphicsPipeline::Back); // to ensure the winding order is correct

    // won't get the wireframe with OpenGL ES
    if (rhi->isFeatureSupported(QRhi::NonFillPolygonMode))
        pipeline->setPolygonMode(QRhiGraphicsPipeline::Line);

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 6 * sizeof(float) } });
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
                                { 0, 1, QRhiVertexInputAttribute::Float3, 3 * sizeof(float) } });

    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    QRhiCommandBuffer *cb = nullptr;
    QCOMPARE(rhi->beginOffscreenFrame(&cb), QRhi::FrameOpSuccess);

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(rt->pixelSize().width()), float(rt->pixelSize().height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);

    QRhiReadbackResult bufferReadResult;
    bufferReadResult.completed = []() {};
    readbackBatch->readBackBuffer(buffer.data(), 0, 1024, &bufferReadResult);

    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    if (rhi->isYUpInFramebuffer()) // we used clipSpaceCorrMatrix so this is different from many
                                   // other tests
        result.flip();

    QCOMPARE(result.size(), rt->pixelSize());

    // cannot check rendering results with Null, because there is no rendering there
    if (impl == QRhi::Null)
        return;

    int redCount = 0, greenCount = 0, blueCount = 0;
    for (int y = 0; y < result.height(); ++y) {
        const quint32 *p = reinterpret_cast<const quint32 *>(result.constScanLine(y));
        int x = result.width() - 1;
        while (x-- >= 0) {
            const QRgb c(*p++);
            const int red = qRed(c);
            const int green = qGreen(c);
            const int blue = qBlue(c);
            // just count the color components that are above a certain threshold
            if (red > 240)
                ++redCount;
            if (green > 240)
                ++greenCount;
            if (blue > 240)
                ++blueCount;
        }
    }

    // make sure we drew something
    QVERIFY(redCount > 50);
    QVERIFY(blueCount > 50);
    QVERIFY(greenCount > 50);

    //    StorageBlock("result" "" knownSize=16 binding=1 set=0 runtimeArrayStride=336 QList(
    //        BlockVariable("int" "count" offset=0 size=4),
    //        BlockVariable("struct" "elements" offset=16 size=0 array=QList(0) structMembers=QList(
    //            BlockVariable("struct" "a" offset=0 size=48 array=QList(3) structMembers=QList(
    //                BlockVariable("vec3" "color" offset=0 size=12),
    //                BlockVariable("int" "id" offset=12 size=4))),
    //            BlockVariable("struct" "b" offset=48 size=144 array=QList(3) structMembers=QList(
    //                BlockVariable("vec2" "some" offset=0 size=8),
    //                BlockVariable("int" "other" offset=8 size=12 array=QList(3)),
    //                BlockVariable("vec3" "variables" offset=32 size=12))),
    //            BlockVariable("struct" "c" offset=192 size=16 structMembers=QList(
    //                BlockVariable("vec3" "stuff" offset=0 size=12),
    //                BlockVariable("float" "more_stuff" offset=12 size=4))),
    //            BlockVariable("vec4" "tesslevelOuter" offset=208 size=16),
    //            BlockVariable("vec2" "tessLevelInner" offset=224 size=8),
    //            BlockVariable("float" "pointSize" offset=232 size=12 array=QList(3)),
    //            BlockVariable("float" "clipDistance" offset=244 size=60 array=QList(5, 3)),
    //            BlockVariable("vec3" "tessCoord" offset=304 size=12),
    //            BlockVariable("int" "patchVerticesIn" offset=316 size=4),
    //            BlockVariable("int" "primitiveID" offset=320 size=4)))))

    // int count
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[0])[0], 1);

    // a[0].color
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 0 + 0])[0], 0.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 0 + 0])[1], 0.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 0 + 0])[2], 1.0f);

    // a[0].id
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 0 + 12])[0], 91);

    // a[1].color
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 16 + 0])[0], 1.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 16 + 0])[1], 0.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 16 + 0])[2], 0.0f);

    // a[1].id
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 16 + 12])[0], 92);

    // a[2].color
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 32 + 0])[0], 0.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 32 + 0])[1], 1.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 32 + 0])[2], 0.0f);

    // a[2].id
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 32 + 12])[0], 93);

    // b[0].some
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 48 + 0])[0], 0.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 48 + 0])[1], 0.0f);

    // b[0].other[0]
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 48 + 8])[0], 10.0f);
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 48 + 8])[1], 20.0f);
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 48 + 8])[2], 30.0f);

    // b[0].variables
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 48 + 32])[0], 3.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 48 + 32])[1], 13.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 48 + 32])[2], 17.0f);

    // b[1].some
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 96 + 0])[0], 1.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 96 + 0])[1], 1.0f);

    // b[1].other[0]
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 96 + 8])[0], 10.0f);
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 96 + 8])[1], 20.0f);
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 96 + 8])[2], 30.0f);

    // b[1].variables
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 96 + 32])[0], 3.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 96 + 32])[1], 14.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 96 + 32])[2], 17.0f);

    // b[2].some
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 144 + 0])[0], 2.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 144 + 0])[1], 2.0f);

    // b[2].other[0]
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 144 + 8])[0], 10.0f);
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 144 + 8])[1], 20.0f);
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 144 + 8])[2], 30.0f);

    // b[2].variables
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 144 + 32])[0], 3.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 144 + 32])[1], 15.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 144 + 32])[2], 17.0f);

    // c.stuff
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 192 + 0])[0], 1.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 192 + 0])[1], 2.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 192 + 0])[2], 3.0f);

    // c.more_stuff
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 192 + 12])[0], 4.0f);

    // tessLevelOuter
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 208 + 0])[0], 1.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 208 + 0])[1], 2.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 208 + 0])[2], 3.0f);

    // tessLevelInner
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 224 + 0])[0], 5.0f);

    // pointSize[0]
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 232 + 0])[0], 10.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 232 + 0])[1], 11.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 232 + 0])[2], 12.0f);

    // clipDistance[0][0]
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 0])[0], 20.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 0])[1], 40.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 0])[2], 60.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 0])[3], 80.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 0])[4], 100.0f);

    // clipDistance[1][0]
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 20])[0], 21.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 20])[1], 41.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 20])[2], 61.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 20])[3], 81.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 20])[4], 101.0f);

    // clipDistance[2][0]
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 40])[0], 22.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 40])[1], 42.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 40])[2], 62.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 40])[3], 82.0f);
    QCOMPARE(reinterpret_cast<const float *>(&bufferReadResult.data.constData()[16 + 244 + 40])[4], 102.0f);

    // patchVerticesIn
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 316 + 0])[0], 3);

    // primitiveID
    QCOMPARE(reinterpret_cast<const int *>(&bufferReadResult.data.constData()[16 + 320 + 0])[0], 0);
}

void tst_QRhi::storageBuffer_data()
{
    rhiTestData();
}

void tst_QRhi::storageBuffer()
{
    // Use a compute shader to copy from one storage buffer of float types to
    // another of int types. We fill the "toGpu" buffer with known float type
    // data generated and uploaded from the CPU, then dispatch a compute shader
    // to copy from the "toGpu" buffer to the "fromGpu" buffer. We then
    // readback the "fromGpu" buffer and verify that the results are as
    // expected.

    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    // we can't test with Null as there is no compute
    if (impl == QRhi::Null)
        return;

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing");

    if (!rhi->isFeatureSupported(QRhi::Feature::Compute))
        QSKIP("Compute is not supported with this graphics API, skipping test");

    QShader s = loadShader(":/data/storagebuffer.comp.qsb");
    QVERIFY(s.isValid());
    QCOMPARE(s.description().storageBlocks().size(), 2);

    QMap<QByteArray, QShaderDescription::StorageBlock> blocks;
    for (const QShaderDescription::StorageBlock &block : s.description().storageBlocks())
        blocks[block.blockName] = block;

    QMap<QByteArray, QShaderDescription::BlockVariable> toGpuMembers;
    for (const QShaderDescription::BlockVariable &member: blocks["toGpu"].members)
        toGpuMembers[member.name] = member;

    QMap<QByteArray, QShaderDescription::BlockVariable> fromGpuMembers;
    for (const QShaderDescription::BlockVariable &member: blocks["fromGpu"].members)
        fromGpuMembers[member.name] = member;

    for (QRhiBuffer::Type type : {QRhiBuffer::Type::Immutable, QRhiBuffer::Type::Static}) {

        QRhiCommandBuffer *cb = nullptr;
        rhi->beginOffscreenFrame(&cb);
        QVERIFY(cb);

        QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
        QVERIFY(u);

        QScopedPointer<QRhiBuffer> toGpuBuffer(rhi->newBuffer(type, QRhiBuffer::UsageFlag::StorageBuffer, blocks["toGpu"].knownSize));
        QVERIFY(toGpuBuffer->create());

        QScopedPointer<QRhiBuffer> fromGpuBuffer(rhi->newBuffer(type, QRhiBuffer::UsageFlag::StorageBuffer, blocks["fromGpu"].knownSize));
        QVERIFY(fromGpuBuffer->create());

        QByteArray toGpuData(blocks["toGpu"].knownSize, 0);
        reinterpret_cast<float *>(&toGpuData.data()[toGpuMembers["_float"].offset])[0] = 1.0f;
        reinterpret_cast<float *>(&toGpuData.data()[toGpuMembers["_vec2"].offset])[0] = 2.0f;
        reinterpret_cast<float *>(&toGpuData.data()[toGpuMembers["_vec2"].offset])[1] = 3.0f;
        reinterpret_cast<float *>(&toGpuData.data()[toGpuMembers["_vec3"].offset])[0] = 4.0f;
        reinterpret_cast<float *>(&toGpuData.data()[toGpuMembers["_vec3"].offset])[1] = 5.0f;
        reinterpret_cast<float *>(&toGpuData.data()[toGpuMembers["_vec3"].offset])[2] = 6.0f;
        reinterpret_cast<float *>(&toGpuData.data()[toGpuMembers["_vec4"].offset])[0] = 7.0f;
        reinterpret_cast<float *>(&toGpuData.data()[toGpuMembers["_vec4"].offset])[1] = 8.0f;
        reinterpret_cast<float *>(&toGpuData.data()[toGpuMembers["_vec4"].offset])[2] = 9.0f;
        reinterpret_cast<float *>(&toGpuData.data()[toGpuMembers["_vec4"].offset])[3] = 10.0f;

        u->uploadStaticBuffer(toGpuBuffer.data(), std::move(toGpuData));
        u->uploadStaticBuffer(fromGpuBuffer.data(), 0, blocks["fromGpu"].knownSize, QByteArray(blocks["fromGpu"].knownSize, 0).constData());

        QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
        srb->setBindings({QRhiShaderResourceBinding::bufferLoad(blocks["toGpu"].binding, QRhiShaderResourceBinding::ComputeStage, toGpuBuffer.data()),
                          QRhiShaderResourceBinding::bufferLoadStore(blocks["fromGpu"].binding, QRhiShaderResourceBinding::ComputeStage, fromGpuBuffer.data())});

        QVERIFY(srb->create());

        QScopedPointer<QRhiComputePipeline> pipeline(rhi->newComputePipeline());
        pipeline->setShaderStage({QRhiShaderStage::Compute, s});
        pipeline->setShaderResourceBindings(srb.data());
        QVERIFY(pipeline->create());

        cb->beginComputePass(u);

        cb->setComputePipeline(pipeline.data());
        cb->setShaderResources();
        cb->dispatch(1, 1, 1);

        u = rhi->nextResourceUpdateBatch();
        QVERIFY(u);

        int readCompletedNotifications = 0;
        QRhiReadbackResult result;
        result.completed = [&readCompletedNotifications]() { readCompletedNotifications++; };
        u->readBackBuffer(fromGpuBuffer.data(), 0, blocks["fromGpu"].knownSize, &result);

        cb->endComputePass(u);

        rhi->endOffscreenFrame();

        QCOMPARE(readCompletedNotifications, 1);

        QCOMPARE(result.data.size(), blocks["fromGpu"].knownSize);
        QCOMPARE(reinterpret_cast<const int *>(&result.data.constData()[fromGpuMembers["_int"].offset])[0], 1);
        QCOMPARE(reinterpret_cast<const int *>(&result.data.constData()[fromGpuMembers["_ivec2"].offset])[0], 2);
        QCOMPARE(reinterpret_cast<const int *>(&result.data.constData()[fromGpuMembers["_ivec2"].offset])[1], 3);
        QCOMPARE(reinterpret_cast<const int *>(&result.data.constData()[fromGpuMembers["_ivec3"].offset])[0], 4);
        QCOMPARE(reinterpret_cast<const int *>(&result.data.constData()[fromGpuMembers["_ivec3"].offset])[1], 5);
        QCOMPARE(reinterpret_cast<const int *>(&result.data.constData()[fromGpuMembers["_ivec3"].offset])[2], 6);
        QCOMPARE(reinterpret_cast<const int *>(&result.data.constData()[fromGpuMembers["_ivec4"].offset])[0], 7);
        QCOMPARE(reinterpret_cast<const int *>(&result.data.constData()[fromGpuMembers["_ivec4"].offset])[1], 8);
        QCOMPARE(reinterpret_cast<const int *>(&result.data.constData()[fromGpuMembers["_ivec4"].offset])[2], 9);
        QCOMPARE(reinterpret_cast<const int *>(&result.data.constData()[fromGpuMembers["_ivec4"].offset])[3], 10);

    }
}

 void tst_QRhi::storageBufferRuntimeSizeCompute_data()
{
     rhiTestData();
}

 void tst_QRhi::storageBufferRuntimeSizeCompute()
{
    // Use a compute shader to copy from one storage buffer with std430 runtime
    // float array to another with std140 runtime int array. We fill the
    // "toGpu" buffer with known float data generated and uploaded from the
    // CPU, then dispatch a compute shader to copy from the "toGpu" buffer to
    // the "fromGpu" buffer. We then readback the "fromGpu" buffer and verify
    // that the results are as expected.  This is primarily to test Metal
    // SPIRV-Cross buffer size buffers.

    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    // we can't test with Null as there is no compute
    if (impl == QRhi::Null)
        return;

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing");

    if (!rhi->isFeatureSupported(QRhi::Feature::Compute))
        QSKIP("Compute is not supported with this graphics API, skipping test");

    QShader s = loadShader(":/data/storagebuffer_runtime.comp.qsb");
    QVERIFY(s.isValid());
    QCOMPARE(s.description().storageBlocks().size(), 2);

    QMap<QByteArray, QShaderDescription::StorageBlock> blocks;
    for (const QShaderDescription::StorageBlock &block : s.description().storageBlocks())
        blocks[block.blockName] = block;

    QMap<QByteArray, QShaderDescription::BlockVariable> toGpuMembers;
    for (const QShaderDescription::BlockVariable &member : blocks["toGpu"].members)
        toGpuMembers[member.name] = member;

    QMap<QByteArray, QShaderDescription::BlockVariable> fromGpuMembers;
    for (const QShaderDescription::BlockVariable &member : blocks["fromGpu"].members)
        fromGpuMembers[member.name] = member;

    for (QRhiBuffer::Type type : { QRhiBuffer::Type::Immutable, QRhiBuffer::Type::Static }) {
        QRhiCommandBuffer *cb = nullptr;

        rhi->beginOffscreenFrame(&cb);
        QVERIFY(cb);

        QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
        QVERIFY(u);

        const int stride430 = sizeof(float);
        const int stride140 = 4 * sizeof(float);
        const int length = 32;

        QScopedPointer<QRhiBuffer> toGpuBuffer(
                    rhi->newBuffer(type, QRhiBuffer::UsageFlag::StorageBuffer,
                                   blocks["toGpu"].knownSize + length * stride430));
        QVERIFY(toGpuBuffer->create());

        QScopedPointer<QRhiBuffer> fromGpuBuffer(
                    rhi->newBuffer(type, QRhiBuffer::UsageFlag::StorageBuffer,
                                   blocks["fromGpu"].knownSize + length * stride140));
        QVERIFY(fromGpuBuffer->create());

        QByteArray toGpuData(toGpuBuffer->size(), 0);
        for (int i = 0; i < length; ++i)
            reinterpret_cast<float &>(toGpuData.data()[toGpuMembers["_float"].offset + i * stride430]) = float(i);

        u->uploadStaticBuffer(toGpuBuffer.data(), 0, toGpuData.size(), toGpuData.constData());
        u->uploadStaticBuffer(fromGpuBuffer.data(), 0, blocks["fromGpu"].knownSize,
                QByteArray(fromGpuBuffer->size(), 0).constData());

        QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
        srb->setBindings(
                    { QRhiShaderResourceBinding::bufferLoadStore(
                      blocks["toGpu"].binding, QRhiShaderResourceBinding::ComputeStage,
                      toGpuBuffer.data()),
                      QRhiShaderResourceBinding::bufferLoadStore(
                      blocks["fromGpu"].binding, QRhiShaderResourceBinding::ComputeStage,
                      fromGpuBuffer.data()) });
        QVERIFY(srb->create());

        QScopedPointer<QRhiComputePipeline> pipeline(rhi->newComputePipeline());
        pipeline->setShaderStage({ QRhiShaderStage::Compute, s });
        pipeline->setShaderResourceBindings(srb.data());
        QVERIFY(pipeline->create());

        cb->beginComputePass(u);

        cb->setComputePipeline(pipeline.data());
        cb->setShaderResources();
        cb->dispatch(1, 1, 1);

        u = rhi->nextResourceUpdateBatch();
        QVERIFY(u);
        int readbackCompleted = 0;
        QRhiReadbackResult result;
        result.completed = [&readbackCompleted]() { readbackCompleted++; };
        u->readBackBuffer(fromGpuBuffer.data(), 0, fromGpuBuffer->size(), &result);

        cb->endComputePass(u);

        rhi->endOffscreenFrame();

        QVERIFY(readbackCompleted > 0);
        QCOMPARE(result.data.size(), fromGpuBuffer->size());

        for (int i = 0; i < length; ++i)
            QCOMPARE(reinterpret_cast<const int &>(result.data.constData()[fromGpuMembers["_int"].offset + i * stride140]), i);

        QCOMPARE(readbackCompleted, 1);

    }

}

void tst_QRhi::storageBufferRuntimeSizeGraphics_data()
{
    rhiTestData();
}

void tst_QRhi::storageBufferRuntimeSizeGraphics()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-108844)");
#endif
    // Draws a tessellated triangle with color determined by the length of
    // buffers bound to shader stages. This is primarily to test Metal
    // SPIRV-Cross buffer size buffers.

    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    if (!rhi->isFeatureSupported(QRhi::Compute)) // also indicates storage buffers and image load/store
        QSKIP("Storage buffers are not supported with this graphics API, skipping test");

    if (!rhi->isFeatureSupported(QRhi::Tessellation)) {
        // From a Vulkan or Metal implementation we expect tessellation to work,
        // even though it is optional (as per spec) for Vulkan.
        QVERIFY(rhi->backend() != QRhi::Vulkan);
        QVERIFY(rhi->backend() != QRhi::Metal);
        QSKIP("Tessellation is not supported with this graphics API, skipping test");
    }

    if (rhi->backend() == QRhi::D3D11 || rhi->backend() == QRhi::D3D12)
        QSKIP("Skipping tessellation test on D3D for now, test assets not prepared for HLSL yet");

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, QSize(64, 64), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    static const float triangleVertices[] = {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
    };

    QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(triangleVertices)));
    QVERIFY(vbuf->create());
    u->uploadStaticBuffer(vbuf.data(), triangleVertices);

    QScopedPointer<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    QVERIFY(ubuf->create());

    QMatrix4x4 mvp = rhi->clipSpaceCorrMatrix();
    u->updateDynamicBuffer(ubuf.data(), 0, 64, mvp.constData());

    QScopedPointer<QRhiBuffer> ssbo5(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, 256));
    QVERIFY(ssbo5->create());

    QScopedPointer<QRhiBuffer> ssbo3(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, 16));
    QVERIFY(ssbo3->create());

    u->uploadStaticBuffer(ssbo3.data(), QVector<float>({ 1.0f, 1.0f, 1.0f, 1.0f }).constData());

    QScopedPointer<QRhiBuffer> ssbo4(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, 128));
    QVERIFY(ssbo4->create());

    const int red = 79;
    const int green = 43;
    const int blue = 251;

    QScopedPointer<QRhiBuffer> ssboR(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, red * sizeof(float)));
    QVERIFY(ssboR->create());

    QScopedPointer<QRhiBuffer> ssboG(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, green * sizeof(float)));
    QVERIFY(ssboG->create());

    QScopedPointer<QRhiBuffer> ssboB(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, blue * sizeof(float)));
    QVERIFY(ssboB->create());

    const int tessOuter0 = 1;
    const int tessOuter1 = 2;
    const int tessOuter2 = 3;
    const int tessInner0 = 4;

    QScopedPointer<QRhiBuffer> ssboTessOuter0(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, tessOuter0 * sizeof(float)));
    QVERIFY(ssboTessOuter0->create());

    QScopedPointer<QRhiBuffer> ssboTessOuter1(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, tessOuter1 * sizeof(float)));
    QVERIFY(ssboTessOuter1->create());

    QScopedPointer<QRhiBuffer> ssboTessOuter2(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, tessOuter2 * sizeof(float)));
    QVERIFY(ssboTessOuter2->create());

    QScopedPointer<QRhiBuffer> ssboTessInner0(rhi->newBuffer(QRhiBuffer::Static, QRhiBuffer::StorageBuffer, tessInner0 * sizeof(float)));
    QVERIFY(ssboTessInner0->create());


    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings({ QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::TessellationEvaluationStage, ubuf.data()),
                       QRhiShaderResourceBinding::bufferLoad(5, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::TessellationEvaluationStage, ssbo5.data()),
                       QRhiShaderResourceBinding::bufferLoad(3, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::TessellationEvaluationStage | QRhiShaderResourceBinding::FragmentStage, ssbo3.data()),
                       QRhiShaderResourceBinding::bufferLoad(4, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::TessellationEvaluationStage, ssbo4.data()),
                       QRhiShaderResourceBinding::bufferLoad(7, QRhiShaderResourceBinding::TessellationControlStage, ssboTessOuter0.data()),
                       QRhiShaderResourceBinding::bufferLoad(8, QRhiShaderResourceBinding::TessellationControlStage | QRhiShaderResourceBinding::TessellationEvaluationStage, ssboTessOuter1.data()),
                       QRhiShaderResourceBinding::bufferLoad(9, QRhiShaderResourceBinding::TessellationControlStage, ssboTessOuter2.data()),
                       QRhiShaderResourceBinding::bufferLoad(10, QRhiShaderResourceBinding::TessellationControlStage, ssboTessInner0.data()),
                       QRhiShaderResourceBinding::bufferLoad(1, QRhiShaderResourceBinding::FragmentStage, ssboG.data()),
                       QRhiShaderResourceBinding::bufferLoad(2, QRhiShaderResourceBinding::FragmentStage, ssboB.data()),
                       QRhiShaderResourceBinding::bufferLoad(6, QRhiShaderResourceBinding::FragmentStage, ssboR.data()) });

    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());

    pipeline->setTopology(QRhiGraphicsPipeline::Patches);
    pipeline->setPatchControlPointCount(3);

    pipeline->setShaderStages({
        { QRhiShaderStage::Vertex, loadShader(":/data/storagebuffer_runtime.vert.qsb") },
        { QRhiShaderStage::TessellationControl, loadShader(":/data/storagebuffer_runtime.tesc.qsb") },
        { QRhiShaderStage::TessellationEvaluation, loadShader(":/data/storagebuffer_runtime.tese.qsb") },
        { QRhiShaderStage::Fragment, loadShader(":/data/storagebuffer_runtime.frag.qsb") }
    });

    pipeline->setCullMode(QRhiGraphicsPipeline::None);

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
    });

    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->create());

    QRhiCommandBuffer *cb = nullptr;
    QCOMPARE(rhi->beginOffscreenFrame(&cb), QRhi::FrameOpSuccess);

    cb->beginPass(rt.data(), Qt::black, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(rt->pixelSize().width()), float(rt->pixelSize().height()) });
    cb->setShaderResources();
    QRhiCommandBuffer::VertexInput vbufBinding(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888);
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();

    QCOMPARE(result.size(), rt->pixelSize());

    // cannot check rendering results with Null, because there is no rendering there
    if (impl == QRhi::Null)
        return;

    QCOMPARE(result.pixel(32, 32), qRgb(red, green, blue));
}

void tst_QRhi::halfPrecisionAttributes_data()
{
    rhiTestData();
}

void tst_QRhi::halfPrecisionAttributes()
{
#ifdef Q_OS_ANDROID
    if (QNativeInterface::QAndroidApplication::sdkVersion() >= 31)
        QSKIP("Fails on Android 12 (QTBUG-108844)");
#endif
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    if (!rhi->isFeatureSupported(QRhi::HalfAttributes)) {
        QVERIFY(rhi->backend() != QRhi::Vulkan);
        QVERIFY(rhi->backend() != QRhi::Metal);
        QVERIFY(rhi->backend() != QRhi::D3D11);
        QVERIFY(rhi->backend() != QRhi::D3D12);
        QSKIP("Half precision vertex attributes are not supported with this graphics API, skipping test");
    }

    const QSize outputSize(1920, 1080);
    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, outputSize, 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->create());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->create());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    //
    // This test uses half3 vertices
    //
    // Note: D3D does not support half3 - rhi passes it through as half4.  Because of this, D3D will
    // report the following warning and error if we don't take precautions:
    //
    // D3D11 WARNING: ID3D11DeviceContext::Draw: Input vertex slot 0 has stride 6 which is less than
    // the minimum stride logically expected from the current Input Layout (8 bytes). This is OK, as
    // hardware is perfectly capable of reading overlapping data. However the developer probably did
    // not intend to make use of this behavior. [ EXECUTION WARNING #355:
    // DEVICE_DRAW_VERTEX_BUFFER_STRIDE_TOO_SMALL]
    //
    // D3D11 ERROR: ID3D11DeviceContext::Draw: Vertex Buffer Stride (6) at the input vertex slot 0
    // is not aligned properly. The current Input Layout imposes an alignment of (4) because of the
    // Formats used with this slot. [ EXECUTION ERROR #367: DEVICE_DRAW_VERTEX_STRIDE_UNALIGNED]
    //
    // The same warning and error are produced for D3D12.  The rendered output is correct despite
    // the warning and error.
    //
    // To avoid these errors, we pad the vertices to 8 byte stride.
    //
    static const qfloat16 vertices[] = {
        qfloat16(-1.0), qfloat16(-1.0), qfloat16(0.0), qfloat16(0.0),
        qfloat16(1.0), qfloat16(-1.0), qfloat16(0.0), qfloat16(0.0),
        qfloat16(0.0), qfloat16(1.0), qfloat16(0.0), qfloat16(0.0),
    };

    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)));
    QVERIFY(vbuf->create());
    updates->uploadStaticBuffer(vbuf.data(), vertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->create());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    QShader vs = loadShader(":/data/half.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/simple.frag.qsb");
    QVERIFY(fs.isValid());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(qfloat16) } }); // 8 byte vertex stride for D3D
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Half3, 0 } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(pipeline->create());

    cb->beginPass(rt.data(), Qt::blue, { 1.0f, 0 }, updates);
    cb->setGraphicsPipeline(pipeline.data());
    cb->setViewport({ 0, 0, float(outputSize.width()), float(outputSize.height()) });
    QRhiCommandBuffer::VertexInput vbindings(vbuf.data(), 0);
    cb->setVertexInput(0, 1, &vbindings);
    cb->draw(3);

    QRhiReadbackResult readResult;
    QImage result;
    readResult.completed = [&readResult, &result] {
        result = QImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                        readResult.pixelSize.width(), readResult.pixelSize.height(),
                        QImage::Format_RGBA8888_Premultiplied); // non-owning, no copy needed because readResult outlives result
    };
    QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
    readbackBatch->readBackTexture({ texture.data() }, &readResult);
    cb->endPass(readbackBatch);

    rhi->endOffscreenFrame();
    // Offscreen frames are synchronous, so the readback is guaranteed to
    // complete at this point. This would not be the case with swapchain-based
    // frames.
    QCOMPARE(result.size(), texture->pixelSize());

    if (impl == QRhi::Null)
        return;

    // Now we have a red rectangle on blue background.
    const int y = 100;
    const quint32 *p = reinterpret_cast<const quint32 *>(result.constScanLine(y));
    int x = result.width() - 1;
    int redCount = 0;
    int blueCount = 0;
    const int maxFuzz = 1;
    while (x-- >= 0) {
        const QRgb c(*p++);
        if (qRed(c) >= (255 - maxFuzz) && qGreen(c) == 0 && qBlue(c) == 0)
            ++redCount;
        else if (qRed(c) == 0 && qGreen(c) == 0 && qBlue(c) >= (255 - maxFuzz))
            ++blueCount;
        else
            QFAIL("Encountered a pixel that is neither red or blue");
    }

    QCOMPARE(redCount + blueCount, texture->pixelSize().width());
    QVERIFY(redCount != 0);
    QVERIFY(blueCount != 0);

    // The triangle is "pointing up" in the resulting image with OpenGL
    // (because Y is up both in normalized device coordinates and in images)
    // and Vulkan (because Y is down in both and the vertex data was specified
    // with Y up in mind), but "pointing down" with D3D (because Y is up in NDC
    // but down in images).
    if (rhi->isYUpInFramebuffer() == rhi->isYUpInNDC())
        QVERIFY(redCount < blueCount);
    else
        QVERIFY(redCount > blueCount);

}

#include <tst_qrhi.moc>
QTEST_MAIN(tst_QRhi)
