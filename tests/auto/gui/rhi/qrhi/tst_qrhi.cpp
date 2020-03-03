/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QThread>
#include <QFile>
#include <QOffscreenSurface>
#include <QPainter>

#include <QtGui/private/qrhi_p.h>
#include <QtGui/private/qrhinull_p.h>

#if QT_CONFIG(opengl)
# include <QOpenGLContext>
# include <QtGui/private/qrhigles2_p.h>
# define TST_GL
#endif

#if QT_CONFIG(vulkan)
# include <QVulkanInstance>
# include <QtGui/private/qrhivulkan_p.h>
# define TST_VK
#endif

#ifdef Q_OS_WIN
#include <QtGui/private/qrhid3d11_p.h>
# define TST_D3D11
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
# include <QtGui/private/qrhimetal_p.h>
# define TST_MTL
#endif

Q_DECLARE_METATYPE(QRhi::Implementation)
Q_DECLARE_METATYPE(QRhiInitParams *)

class tst_QRhi : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void rhiTestData();
    void create_data();
    void create();
    void nativeHandles_data();
    void nativeHandles();
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
    void invalidPipeline_data();
    void invalidPipeline();
    void renderToTextureSimple_data();
    void renderToTextureSimple();
    void renderToTextureTexturedQuad_data();
    void renderToTextureTexturedQuad();
    void renderToTextureArrayOfTexturedQuad_data();
    void renderToTextureArrayOfTexturedQuad();
    void renderToTextureTexturedQuadAndUniformBuffer_data();
    void renderToTextureTexturedQuadAndUniformBuffer();
    void renderToWindowSimple_data();
    void renderToWindowSimple();
    void srbLayoutCompatibility_data();
    void srbLayoutCompatibility();
    void renderPassDescriptorCompatibility_data();
    void renderPassDescriptorCompatibility();

private:
    struct {
        QRhiNullInitParams null;
#ifdef TST_GL
        QRhiGles2InitParams gl;
#endif
#ifdef TST_VK
        QRhiVulkanInitParams vk;
#endif
#ifdef TST_D3D11
        QRhiD3D11InitParams d3d;
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
    fallbackSurface = QRhiGles2InitParams::newFallbackSurface();
    initParams.gl.fallbackSurface = fallbackSurface;
#endif

#ifdef TST_VK
#ifndef Q_OS_ANDROID
    vulkanInstance.setLayers({ QByteArrayLiteral("VK_LAYER_LUNARG_standard_validation") });
#else
    vulkanInstance.setLayers({ QByteArrayLiteral("VK_LAYER_GOOGLE_threading"),
                               QByteArrayLiteral("VK_LAYER_LUNARG_parameter_validation"),
                               QByteArrayLiteral("VK_LAYER_LUNARG_object_tracker"),
                               QByteArrayLiteral("VK_LAYER_LUNARG_core_validation"),
                               QByteArrayLiteral("VK_LAYER_LUNARG_image"),
                               QByteArrayLiteral("VK_LAYER_LUNARG_swapchain"),
                               QByteArrayLiteral("VK_LAYER_GOOGLE_unique_objects") });
#endif
    vulkanInstance.setExtensions(QByteArrayList()
                                 << "VK_KHR_get_physical_device_properties2");
    vulkanInstance.create();
    initParams.vk.inst = &vulkanInstance;
#endif

#ifdef TST_D3D11
    initParams.d3d.enableDebugLayer = true;
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

    QTest::newRow("Null") << QRhi::Null << static_cast<QRhiInitParams *>(&initParams.null);
#ifdef TST_GL
    QTest::newRow("OpenGL") << QRhi::OpenGLES2 << static_cast<QRhiInitParams *>(&initParams.gl);
#endif
#ifdef TST_VK
    if (vulkanInstance.isValid())
        QTest::newRow("Vulkan") << QRhi::Vulkan << static_cast<QRhiInitParams *>(&initParams.vk);
#endif
#ifdef TST_D3D11
    QTest::newRow("Direct3D 11") << QRhi::D3D11 << static_cast<QRhiInitParams *>(&initParams.d3d);
#endif
#ifdef TST_MTL
    QTest::newRow("Metal") << QRhi::Metal << static_cast<QRhiInitParams *>(&initParams.mtl);
#endif
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
        QCOMPARE(rhi->backend(), impl);
        QCOMPARE(rhi->thread(), QThread::currentThread());

        // do a basic smoke test for the apis that do not directly render anything

        int cleanupOk = 0;
        QRhi *rhiPtr = rhi.data();
        auto cleanupFunc = [rhiPtr, &cleanupOk](QRhi *dyingRhi) {
            if (rhiPtr == dyingRhi)
                cleanupOk += 1;
        };
        rhi->addCleanupCallback(cleanupFunc);
        rhi->runCleanup();
        QCOMPARE(cleanupOk, 1);
        cleanupOk = 0;
        rhi->addCleanupCallback(cleanupFunc);

        QRhiResourceUpdateBatch *resUpd = rhi->nextResourceUpdateBatch();
        QVERIFY(resUpd);
        resUpd->release();

        QVERIFY(!rhi->supportedSampleCounts().isEmpty());
        QVERIFY(rhi->supportedSampleCounts().contains(1));

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
        QVERIFY(texMin >= 1);
        QVERIFY(texMax >= texMin);
        QVERIFY(maxAtt >= 1);
        QVERIFY(framesInFlight >= 1);

        QVERIFY(rhi->nativeHandles());
        QVERIFY(rhi->profiler());

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
            QRhi::TexelFetch
        };
        for (size_t i = 0; i <sizeof(features) / sizeof(QRhi::Feature); ++i)
            rhi->isFeatureSupported(features[i]);

        QVERIFY(rhi->isTextureFormatSupported(QRhiTexture::RGBA8));

        rhi->releaseCachedResources();

        QVERIFY(!rhi->isDeviceLost());

        rhi.reset();
        QCOMPARE(cleanupOk, 1);
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
            QVERIFY(vkHandles->physDev);
            QVERIFY(vkHandles->dev);
            QVERIFY(vkHandles->gfxQueueFamilyIdx >= 0);
            QVERIFY(vkHandles->gfxQueue);
            QVERIFY(vkHandles->cmdPool);
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

        const QRhiNativeHandles *cbHandles = cb->nativeHandles();
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
#ifdef TST_MTL
        case QRhi::Metal:
        {
            const QRhiMetalCommandBufferNativeHandles *mtlHandles = static_cast<const QRhiMetalCommandBufferNativeHandles *>(cbHandles);
            QVERIFY(mtlHandles);
            QVERIFY(mtlHandles->commandBuffer);
            QVERIFY(!mtlHandles->encoder);

            QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
            QVERIFY(tex->build());
            QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ tex.data() }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
            QVERIFY(rpDesc);
            rt->setRenderPassDescriptor(rpDesc.data());
            QVERIFY(rt->build());
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
        QVERIFY(tex->build());
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ tex.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        QVERIFY(rpDesc);
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->build());

        const QRhiNativeHandles *rpHandles = rpDesc->nativeHandles();
        switch (impl) {
        case QRhi::Null:
            break;
#ifdef TST_VK
        case QRhi::Vulkan:
        {
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
#ifdef TST_MTL
        case QRhi::Metal:
            break;
#endif
        default:
            Q_ASSERT(false);
        }
    }
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
    QVERIFY(tex->build());

    const QRhiTexture::NativeTexture nativeTex = tex->nativeTexture();

    switch (impl) {
    case QRhi::Null:
        break;
#ifdef TST_VK
    case QRhi::Vulkan:
    {
        auto *image = static_cast<const VkImage *>(nativeTex.object);
        QVERIFY(image);
        QVERIFY(*image);
        QVERIFY(nativeTex.layout >= 1); // VK_IMAGE_LAYOUT_GENERAL
        QVERIFY(nativeTex.layout <= 8); // VK_IMAGE_LAYOUT_PREINITIALIZED
    }
        break;
#endif
#ifdef TST_GL
    case QRhi::OpenGLES2:
    {
        auto *textureId = static_cast<const uint *>(nativeTex.object);
        QVERIFY(textureId);
        QVERIFY(*textureId);
    }
        break;
#endif
#ifdef TST_D3D11
    case QRhi::D3D11:
    {
        auto *texture = static_cast<void * const *>(nativeTex.object);
        QVERIFY(texture);
        QVERIFY(*texture);
    }
        break;
#endif
#ifdef TST_MTL
    case QRhi::Metal:
    {
        void * const * texture = (void * const *)nativeTex.object;
        QVERIFY(texture);
        QVERIFY(*texture);
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
        QVERIFY(buf->build());

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
        QVERIFY(dynamicBuffer->build());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        batch->updateDynamicBuffer(dynamicBuffer.data(), 10, bufferSize - 10, a.constData());
        batch->updateDynamicBuffer(dynamicBuffer.data(), 0, 12, b.constData());

        QRhiBufferReadbackResult readResult;
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
        QVERIFY(dynamicBuffer->build());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
        QVERIFY(batch);

        batch->uploadStaticBuffer(dynamicBuffer.data(), 10, bufferSize - 10, a.constData());
        batch->uploadStaticBuffer(dynamicBuffer.data(), 0, 12, b.constData());

        QRhiBufferReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };

        if (rhi->isFeatureSupported(QRhi::ReadBackNonUniformBuffer))
            batch->readBackBuffer(dynamicBuffer.data(), 5, 10, &readResult);

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

inline bool imageRGBAEquals(const QImage &a, const QImage &b)
{
    const int maxFuzz = 1;

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
        QVERIFY(texture->build());

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
        QVERIFY(texture->build());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();

        QRhiTextureUploadEntry upload(0, 0, { image.constBits(), int(image.sizeInBytes()) });
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
        QVERIFY(texture->build());

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
        QVERIFY(texture->build());

        QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();

        QImage clearImage(fullSize, image.format());
        clearImage.fill(Qt::black);
        batch->uploadTexture(texture.data(), clearImage);

        // SourceTopLeft is not supported for non-QImage-based uploads.
        const QImage im = image.copy(QRect(greenRectPos, copySize));
        QRhiTextureSubresourceUploadDescription desc;
        desc.setData(QByteArray::fromRawData(reinterpret_cast<const char *>(im.constBits()),
                                             int(im.sizeInBytes())));
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
        QVERIFY(texture->build());

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
    QVERIFY(redTexture->build());
    batch->uploadTexture(redTexture.data(), red);

    QScopedPointer<QRhiTexture> greenTexture(rhi->newTexture(QRhiTexture::RGBA8, green.size(),
                                                             1, QRhiTexture::UsedAsTransferSource));
    QVERIFY(greenTexture->build());
    batch->uploadTexture(greenTexture.data(), green);

    // 1. simple copy red -> texture; 2. subimage copy green -> texture; 3. partial subimage copy green -> texture
    {
        QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, red.size(),
                                                            1, QRhiTexture::UsedAsTransferSource));
        QVERIFY(texture->build());

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
    QVERIFY(texture->build());

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
    QVERIFY(texture->build());
    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->build());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->build());

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 2 * sizeof(float) } });
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float2, 0 } });

    // no stages
    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(!pipeline->build());

    QShader vs;
    QShader fs;

    // no shaders in the stages
    pipeline.reset(rhi->newGraphicsPipeline());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(!pipeline->build());

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
    QVERIFY(!pipeline->build());

    // no vertex inputs
    pipeline.reset(rhi->newGraphicsPipeline());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    pipeline->setRenderPassDescriptor(rpDesc.data());
    pipeline->setShaderResourceBindings(srb.data());
    QVERIFY(!pipeline->build());

    // no renderpass descriptor
    pipeline.reset(rhi->newGraphicsPipeline());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    QVERIFY(!pipeline->build());

    // no shader resource bindings
    pipeline.reset(rhi->newGraphicsPipeline());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(!pipeline->build());

    // correct
    pipeline.reset(rhi->newGraphicsPipeline());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setRenderPassDescriptor(rpDesc.data());
    pipeline->setShaderResourceBindings(srb.data());
    QVERIFY(pipeline->build());
}

void tst_QRhi::renderToTextureSimple_data()
{
    rhiTestData();
}

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
    QVERIFY(texture->build());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->build());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    static const float vertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        0.0f, 1.0f
    };
    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)));
    QVERIFY(vbuf->build());
    updates->uploadStaticBuffer(vbuf.data(), vertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->build());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    QShader vs = loadShader(":/data/simple.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/simple.frag.qsb");
    QVERIFY(fs.isValid());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 2 * sizeof(float) } });
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float2, 0 } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->build());

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
    QVERIFY(texture->build());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->build());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    static const float verticesUvs[] = {
        -1.0f, -1.0f,   0.0f, 0.0f,
        1.0f, -1.0f,    1.0f, 0.0f,
        -1.0f, 1.0f,    0.0f, 1.0f,
        1.0f, 1.0f,     1.0f, 1.0f
    };
    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(verticesUvs)));
    QVERIFY(vbuf->build());
    updates->uploadStaticBuffer(vbuf.data(), verticesUvs);

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->build());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->build());

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings({
                         QRhiShaderResourceBinding::sampledTexture(0, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
                     });
    QVERIFY(srb->build());

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

    QVERIFY(pipeline->build());

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
        result = std::move(result).mirrored();

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

    QImage inputImage;
    inputImage.load(QLatin1String(":/data/qt256.png"));
    QVERIFY(!inputImage.isNull());

    QScopedPointer<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size(), 1,
                                                        QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    QVERIFY(texture->build());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->build());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    static const float verticesUvs[] = {
        -1.0f, -1.0f,   0.0f, 0.0f,
        1.0f, -1.0f,    1.0f, 0.0f,
        -1.0f, 1.0f,    0.0f, 1.0f,
        1.0f, 1.0f,     1.0f, 1.0f
    };
    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(verticesUvs)));
    QVERIFY(vbuf->build());
    updates->uploadStaticBuffer(vbuf.data(), verticesUvs);

    // In this test we pass 3 textures (and samplers) to the fragment shader in
    // form of an array of combined image samplers.

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->build());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QImage redImage(inputImage.size(), QImage::Format_RGBA8888);
    redImage.fill(Qt::red);

    QScopedPointer<QRhiTexture> redTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(redTexture->build());
    updates->uploadTexture(redTexture.data(), redImage);

    QImage greenImage(inputImage.size(), QImage::Format_RGBA8888);
    greenImage.fill(Qt::green);

    QScopedPointer<QRhiTexture> greenTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(greenTexture->build());
    updates->uploadTexture(greenTexture.data(), greenImage);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->build());

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QRhiShaderResourceBinding::TextureAndSampler texSamplers[3] = {
        { inputTexture.data(), sampler.data() },
        { redTexture.data(), sampler.data() },
        { greenTexture.data(), sampler.data() }
    };
    srb->setBindings({
                         QRhiShaderResourceBinding::sampledTextures(0, QRhiShaderResourceBinding::FragmentStage, 3, texSamplers)
                     });
    QVERIFY(srb->build());

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

    QVERIFY(pipeline->build());

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
        result = std::move(result).mirrored();

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
    QVERIFY(texture->build());

    QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ texture.data() }));
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(rt->build());

    QRhiCommandBuffer *cb = nullptr;
    QVERIFY(rhi->beginOffscreenFrame(&cb) == QRhi::FrameOpSuccess);
    QVERIFY(cb);

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    static const float verticesUvs[] = {
        -1.0f, -1.0f,   0.0f, 0.0f,
        1.0f, -1.0f,    1.0f, 0.0f,
        -1.0f, 1.0f,    0.0f, 1.0f,
        1.0f, 1.0f,     1.0f, 1.0f
    };
    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(verticesUvs)));
    QVERIFY(vbuf->build());
    updates->uploadStaticBuffer(vbuf.data(), verticesUvs);

    // There will be two renderpasses. One renders with no transformation and
    // an opacity of 0.5, the second has a rotation. Bake the uniform data for
    // both into a single buffer.

    const int UNIFORM_BLOCK_SIZE = 64 + 4; // matrix + opacity
    const int secondUbufOffset = rhi->ubufAligned(UNIFORM_BLOCK_SIZE);
    const int UBUF_SIZE = secondUbufOffset + UNIFORM_BLOCK_SIZE;

    QScopedPointer<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE));
    QVERIFY(ubuf->build());

    QMatrix4x4 matrix;
    updates->updateDynamicBuffer(ubuf.data(), 0, 64, matrix.constData());
    float opacity = 0.5f;
    updates->updateDynamicBuffer(ubuf.data(), 64, 4, &opacity);

    // rotation by 45 degrees around the Z axis
    matrix.rotate(45, 0, 0, 1);
    updates->updateDynamicBuffer(ubuf.data(), secondUbufOffset, 64, matrix.constData());
    updates->updateDynamicBuffer(ubuf.data(), secondUbufOffset + 64, 4, &opacity);

    QScopedPointer<QRhiTexture> inputTexture(rhi->newTexture(QRhiTexture::RGBA8, inputImage.size()));
    QVERIFY(inputTexture->build());
    updates->uploadTexture(inputTexture.data(), inputImage);

    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->build());

    const QRhiShaderResourceBinding::StageFlags commonVisibility = QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage;
    QScopedPointer<QRhiShaderResourceBindings> srb0(rhi->newShaderResourceBindings());
    srb0->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, ubuf.data(), 0, UNIFORM_BLOCK_SIZE),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
                     });
    QVERIFY(srb0->build());

    QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
    srb1->setBindings({
                         QRhiShaderResourceBinding::uniformBuffer(0, commonVisibility, ubuf.data(), secondUbufOffset, UNIFORM_BLOCK_SIZE),
                         QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, inputTexture.data(), sampler.data())
                     });
    QVERIFY(srb1->build());
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

    QVERIFY(pipeline->build());

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
        result0 = std::move(result0).mirrored();
        result1 = std::move(result1).mirrored();
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

void tst_QRhi::renderToWindowSimple_data()
{
    rhiTestData();
}

void tst_QRhi::renderToWindowSimple()
{
    QFETCH(QRhi::Implementation, impl);
    QFETCH(QRhiInitParams *, initParams);

#ifdef Q_OS_WINRT
    if (impl == QRhi::D3D11)
        QSKIP("Skipping window-based QRhi rendering on WinRT as the platform and the D3D11 backend are not prepared for this yet");
#endif

    QScopedPointer<QRhi> rhi(QRhi::create(impl, initParams, QRhi::Flags(), nullptr));
    if (!rhi)
        QSKIP("QRhi could not be created, skipping testing rendering");

    QScopedPointer<QWindow> window(new QWindow);
    switch (impl) {
    case QRhi::OpenGLES2:
#if QT_CONFIG(opengl)
        window->setFormat(QRhiGles2InitParams::adjustedFormat());
#endif
        Q_FALLTHROUGH();
    case QRhi::D3D11:
        window->setSurfaceType(QSurface::OpenGLSurface);
        break;
    case QRhi::Metal:
        window->setSurfaceType(QSurface::MetalSurface);
        break;
    case QRhi::Vulkan:
        window->setSurfaceType(QSurface::VulkanSurface);
#if QT_CONFIG(vulkan)
        window->setVulkanInstance(&vulkanInstance);
#endif
        break;
    default:
        break;
    }

    window->setGeometry(0, 0, 640, 480);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QScopedPointer<QRhiSwapChain> swapChain(rhi->newSwapChain());
    swapChain->setWindow(window.data());
    swapChain->setFlags(QRhiSwapChain::UsedAsTransferSource);
    QScopedPointer<QRhiRenderPassDescriptor> rpDesc(swapChain->newCompatibleRenderPassDescriptor());
    swapChain->setRenderPassDescriptor(rpDesc.data());
    QVERIFY(swapChain->buildOrResize());

    QRhiResourceUpdateBatch *updates = rhi->nextResourceUpdateBatch();

    static const float vertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        0.0f, 1.0f
    };
    QScopedPointer<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertices)));
    QVERIFY(vbuf->build());
    updates->uploadStaticBuffer(vbuf.data(), vertices);

    QScopedPointer<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    QVERIFY(srb->build());

    QScopedPointer<QRhiGraphicsPipeline> pipeline(rhi->newGraphicsPipeline());
    QShader vs = loadShader(":/data/simple.vert.qsb");
    QVERIFY(vs.isValid());
    QShader fs = loadShader(":/data/simple.frag.qsb");
    QVERIFY(fs.isValid());
    pipeline->setShaderStages({ { QRhiShaderStage::Vertex, vs }, { QRhiShaderStage::Fragment, fs } });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 2 * sizeof(float) } });
    inputLayout.setAttributes({ { 0, 0, QRhiVertexInputAttribute::Float2, 0 } });
    pipeline->setVertexInputLayout(inputLayout);
    pipeline->setShaderResourceBindings(srb.data());
    pipeline->setRenderPassDescriptor(rpDesc.data());

    QVERIFY(pipeline->build());

    const int asyncReadbackFrames = rhi->resourceLimit(QRhi::MaxAsyncReadbackFrames);
    // one frame issues the readback, then we do MaxAsyncReadbackFrames more to ensure the readback completes
    const int FRAME_COUNT = asyncReadbackFrames + 1;
    bool readCompleted = false;
    QRhiReadbackResult readResult;
    QImage result;
    int readbackWidth = 0;

    for (int frameNo = 0; frameNo < FRAME_COUNT; ++frameNo) {
        QVERIFY(rhi->beginFrame(swapChain.data()) == QRhi::FrameOpSuccess);
        QRhiCommandBuffer *cb = swapChain->currentFrameCommandBuffer();
        QRhiRenderTarget *rt = swapChain->currentFrameRenderTarget();
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
            readResult.completed = [&readCompleted, &readResult, &result, &rhi] {
                readCompleted = true;
                QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                                    readResult.pixelSize.width(), readResult.pixelSize.height(),
                                    QImage::Format_ARGB32_Premultiplied);
                if (readResult.format == QRhiTexture::RGBA8)
                    wrapperImage = wrapperImage.rgbSwapped();
                if (rhi->isYUpInFramebuffer() == rhi->isYUpInNDC())
                    result = wrapperImage.mirrored();
                else
                    result = wrapperImage.copy();
            };
            QRhiResourceUpdateBatch *readbackBatch = rhi->nextResourceUpdateBatch();
            readbackBatch->readBackTexture({}, &readResult); // read back the current backbuffer
            readbackWidth = outputSize.width();
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
    QVERIFY(texture->build());
    QScopedPointer<QRhiSampler> sampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(sampler->build());
    QScopedPointer<QRhiSampler> otherSampler(rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    QVERIFY(otherSampler->build());
    QScopedPointer<QRhiBuffer> buf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 1024));
    QVERIFY(buf->build());
    QScopedPointer<QRhiBuffer> otherBuf(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 256));
    QVERIFY(otherBuf->build());

    // empty (compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        QVERIFY(srb1->build());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        QVERIFY(srb2->build());

        QVERIFY(srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(srb2->isLayoutCompatible(srb1.data()));
    }

    // different count (not compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        QVERIFY(srb1->build());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::sampledTexture(0, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb2->build());

        QVERIFY(!srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(!srb2->isLayoutCompatible(srb1.data()));
    }

    // full match (compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb1->build());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb2->build());

        QVERIFY(srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(srb2->isLayoutCompatible(srb1.data()));
    }

    // different visibility (not compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, buf.data()),
                         });
        QVERIFY(srb1->build());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                         });
        QVERIFY(srb2->build());

        QVERIFY(!srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(!srb2->isLayoutCompatible(srb1.data()));
    }

    // different binding points (not compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                         });
        QVERIFY(srb1->build());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(1, QRhiShaderResourceBinding::VertexStage, buf.data()),
                         });
        QVERIFY(srb2->build());

        QVERIFY(!srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(!srb2->isLayoutCompatible(srb1.data()));
    }

    // different buffer region offset and size (compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data(), rhi->ubufAligned(1), 128),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb1->build());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb2->build());

        QVERIFY(srb1->isLayoutCompatible(srb2.data()));
        QVERIFY(srb2->isLayoutCompatible(srb1.data()));
    }

    // different resources (compatible)
    {
        QScopedPointer<QRhiShaderResourceBindings> srb1(rhi->newShaderResourceBindings());
        srb1->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, otherBuf.data()),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), otherSampler.data())
                         });
        QVERIFY(srb1->build());

        QScopedPointer<QRhiShaderResourceBindings> srb2(rhi->newShaderResourceBindings());
        srb2->setBindings({
                              QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, buf.data()),
                              QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture.data(), sampler.data())
                         });
        QVERIFY(srb2->build());

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
        QSKIP("QRhi could not be created, skipping testing texture resource updates");

    // Note that checking compatibility is only relevant with backends where
    // there is a concept of renderpass descriptions (Vulkan, and partially
    // Metal). It is perfectly fine for isCompatible() to always return true
    // when that is not the case (D3D11, OpenGL). Hence the 'if (Vulkan or
    // Metal)' for all the negative tests. Also note "partial" for Metal:
    // resolve textures for examples have no effect on compatibility with Metal.

    // tex and tex2 have the same format
    QScopedPointer<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
    QVERIFY(tex->build());
    QScopedPointer<QRhiTexture> tex2(rhi->newTexture(QRhiTexture::RGBA8, QSize(512, 512), 1, QRhiTexture::RenderTarget));
    QVERIFY(tex2->build());

    QScopedPointer<QRhiRenderBuffer> ds(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, QSize(512, 512)));
    QVERIFY(ds->build());

    // two texture rendertargets with tex and tex2 as color0 (compatible)
    {
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ tex.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->build());

        QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ tex2.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
        rt2->setRenderPassDescriptor(rpDesc2.data());
        QVERIFY(rt2->build());

        QVERIFY(rpDesc->isCompatible(rpDesc2.data()));
        QVERIFY(rpDesc2->isCompatible(rpDesc.data()));
    }

    // two texture rendertargets with tex and tex2 as color0, and a depth-stencil attachment as well (compatible)
    {
        QRhiTextureRenderTargetDescription desc({ tex.data() }, ds.data());
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget(desc));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->build());

        QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget(desc));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
        rt2->setRenderPassDescriptor(rpDesc2.data());
        QVERIFY(rt2->build());

        QVERIFY(rpDesc->isCompatible(rpDesc2.data()));
        QVERIFY(rpDesc2->isCompatible(rpDesc.data()));
    }

    // now one of them does not have the ds attachment (not compatible)
    {
        QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ { tex.data() }, ds.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
        rt->setRenderPassDescriptor(rpDesc.data());
        QVERIFY(rt->build());

        QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ tex.data() }));
        QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
        rt2->setRenderPassDescriptor(rpDesc2.data());
        QVERIFY(rt2->build());

        if (impl == QRhi::Vulkan || impl == QRhi::Metal) {
            QVERIFY(!rpDesc->isCompatible(rpDesc2.data()));
            QVERIFY(!rpDesc2->isCompatible(rpDesc.data()));
        }
    }

    if (rhi->isFeatureSupported(QRhi::MultisampleRenderBuffer)) {
        // resolve attachments (compatible)
        {
            QScopedPointer<QRhiRenderBuffer> msaaRenderBuffer(rhi->newRenderBuffer(QRhiRenderBuffer::Color, QSize(512, 512), 4));
            QVERIFY(msaaRenderBuffer->build());
            QScopedPointer<QRhiRenderBuffer> msaaRenderBuffer2(rhi->newRenderBuffer(QRhiRenderBuffer::Color, QSize(512, 512), 4));
            QVERIFY(msaaRenderBuffer2->build());

            QRhiColorAttachment colorAtt(msaaRenderBuffer.data()); // color0, multisample
            colorAtt.setResolveTexture(tex.data()); // resolved into a non-msaa texture
            QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ colorAtt }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
            rt->setRenderPassDescriptor(rpDesc.data());
            QVERIFY(rt->build());

            QRhiColorAttachment colorAtt2(msaaRenderBuffer2.data()); // color0, multisample
            colorAtt2.setResolveTexture(tex2.data()); // resolved into a non-msaa texture
            QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ colorAtt2 }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
            rt2->setRenderPassDescriptor(rpDesc2.data());
            QVERIFY(rt2->build());

            QVERIFY(rpDesc->isCompatible(rpDesc2.data()));
            QVERIFY(rpDesc2->isCompatible(rpDesc.data()));
        }

        // missing resolve for one of them (not compatible)
        {
            QScopedPointer<QRhiRenderBuffer> msaaRenderBuffer(rhi->newRenderBuffer(QRhiRenderBuffer::Color, QSize(512, 512), 4));
            QVERIFY(msaaRenderBuffer->build());
            QScopedPointer<QRhiRenderBuffer> msaaRenderBuffer2(rhi->newRenderBuffer(QRhiRenderBuffer::Color, QSize(512, 512), 4));
            QVERIFY(msaaRenderBuffer2->build());

            QRhiColorAttachment colorAtt(msaaRenderBuffer.data()); // color0, multisample
            colorAtt.setResolveTexture(tex.data()); // resolved into a non-msaa texture
            QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ colorAtt }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
            rt->setRenderPassDescriptor(rpDesc.data());
            QVERIFY(rt->build());

            QRhiColorAttachment colorAtt2(msaaRenderBuffer2.data()); // color0, multisample
            QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ colorAtt2 }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
            rt2->setRenderPassDescriptor(rpDesc2.data());
            QVERIFY(rt2->build());

            if (impl == QRhi::Vulkan) { // no Metal here
                QVERIFY(!rpDesc->isCompatible(rpDesc2.data()));
                QVERIFY(!rpDesc2->isCompatible(rpDesc.data()));
            }
        }
    } else {
        qDebug("Skipping multisample renderbuffer dependent tests");
    }

    if (rhi->isTextureFormatSupported(QRhiTexture::RGBA32F)) {
        QScopedPointer<QRhiTexture> tex3(rhi->newTexture(QRhiTexture::RGBA32F, QSize(512, 512), 1, QRhiTexture::RenderTarget));
        QVERIFY(tex3->build());

        // different texture formats (not compatible)
        {
            QScopedPointer<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ tex.data() }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc(rt->newCompatibleRenderPassDescriptor());
            rt->setRenderPassDescriptor(rpDesc.data());
            QVERIFY(rt->build());

            QScopedPointer<QRhiTextureRenderTarget> rt2(rhi->newTextureRenderTarget({ tex3.data() }));
            QScopedPointer<QRhiRenderPassDescriptor> rpDesc2(rt2->newCompatibleRenderPassDescriptor());
            rt2->setRenderPassDescriptor(rpDesc2.data());
            QVERIFY(rt2->build());

            if (impl == QRhi::Vulkan || impl == QRhi::Metal) {
                QVERIFY(!rpDesc->isCompatible(rpDesc2.data()));
                QVERIFY(!rpDesc2->isCompatible(rpDesc.data()));
            }
        }
    } else {
        qDebug("Skipping texture format dependent tests");
    }
}

#include <tst_qrhi.moc>
QTEST_MAIN(tst_QRhi)
