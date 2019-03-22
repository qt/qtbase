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
#include <QtGui/private/qrhi_p.h>
#include <QtGui/private/qrhinull_p.h>

#if QT_CONFIG(opengl)
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

#ifdef Q_OS_DARWIN
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

    void create_data();
    void create();

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
    vulkanInstance.create();
    initParams.vk.inst = &vulkanInstance;
#endif
}

void tst_QRhi::cleanupTestCase()
{
#ifdef TST_VK
    vulkanInstance.destroy();
#endif

    delete fallbackSurface;
}

void tst_QRhi::create_data()
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
        QVERIFY(texMin >= 1);
        QVERIFY(texMax >= texMin);
        QVERIFY(maxAtt >= 1);

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
            QRhi::ElementIndexUint
        };
        for (size_t i = 0; i <sizeof(features) / sizeof(QRhi::Feature); ++i)
            rhi->isFeatureSupported(features[i]);

        QVERIFY(rhi->isTextureFormatSupported(QRhiTexture::RGBA8));

        rhi.reset();
        QCOMPARE(cleanupOk, 1);
    }
}

#include <tst_qrhi.moc>
QTEST_MAIN(tst_QRhi)
