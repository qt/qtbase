// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtWidgets/QRhiWidget>
#include <QtGui/QPainter>
#include <QTest>
#include <QSignalSpy>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <rhi/qrhi.h>

#include <QApplication>
#include <QFile>
#include <QVBoxLayout>
#include <QScrollArea>

#if QT_CONFIG(vulkan)
#include <private/qvulkandefaultinstance_p.h>
#endif

class tst_QRhiWidget : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void create_data();
    void create();
    void noCreate();
    void simple_data();
    void simple();
    void msaa_data();
    void msaa();
    void fixedSize_data();
    void fixedSize();
    void autoRt_data();
    void autoRt();
    void reparent_data();
    void reparent();
    void grabFramebufferWhileStillInvisible_data();
    void grabFramebufferWhileStillInvisible();
    void grabViaQWidgetGrab_data();
    void grabViaQWidgetGrab();
    void mirror_data();
    void mirror();

private:
    void testData();
};

void tst_QRhiWidget::initTestCase()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RhiBasedRendering))
        QSKIP("RhiBasedRendering capability is reported as unsupported on this platform.");

    qputenv("QT_RHI_LEAK_CHECK", "1");
}

void tst_QRhiWidget::testData()
{
    QTest::addColumn<QRhiWidget::Api>("api");

#ifndef Q_OS_WEBOS
    QTest::newRow("Null") << QRhiWidget::Api::Null;
#endif

#if QT_CONFIG(opengl) && QT_CONFIG(run_opengl_tests)
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::OpenGL))
        QTest::newRow("OpenGL") << QRhiWidget::Api::OpenGL;
#endif

#if QT_CONFIG(vulkan)
    // Have to probe to be sure Vulkan is actually working (the test cases
    // themselves will assume QRhi init succeeds).
    if (QVulkanDefaultInstance::instance()) {
        QRhiVulkanInitParams vulkanInitParams;
        vulkanInitParams.inst = QVulkanDefaultInstance::instance();
        if (QRhi::probe(QRhi::Vulkan, &vulkanInitParams))
            QTest::newRow("Vulkan") << QRhiWidget::Api::Vulkan;
    }
#endif

#if QT_CONFIG(metal)
    QRhiMetalInitParams metalInitParams;
    if (QRhi::probe(QRhi::Metal, &metalInitParams))
        QTest::newRow("Metal") << QRhiWidget::Api::Metal;
#endif

#ifdef Q_OS_WIN
    QTest::newRow("D3D11") << QRhiWidget::Api::Direct3D11;
    // D3D12 needs to be probed too due to being disabled if the SDK headers
    // are too old (clang, mingw).
    QRhiD3D12InitParams d3d12InitParams;
    if (QRhi::probe(QRhi::D3D12, &d3d12InitParams))
        QTest::newRow("D3D12") << QRhiWidget::Api::Direct3D12;
#endif
}

void tst_QRhiWidget::create_data()
{
    testData();
}

void tst_QRhiWidget::create()
{
    QFETCH(QRhiWidget::Api, api);

    {
        QRhiWidget w;
        w.setApi(api);
        w.resize(320, 240);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
    }

    {
        QWidget topLevel;
        topLevel.resize(320, 240);
        QRhiWidget *w = new QRhiWidget(&topLevel);
        w->setApi(api);
        w->resize(100, 100);
        topLevel.show();
        QVERIFY(QTest::qWaitForWindowExposed(&topLevel));
    }
}

void tst_QRhiWidget::noCreate()
{
    // Now try something that is guaranteed to fail.
    // E.g. try using Metal on Windows.
    // The error signal should be emitted. The frame signal should not.
#ifdef Q_OS_WIN
    qDebug("Warnings will be printed below, this is as expected");
    QRhiWidget rhiWidget;
    rhiWidget.setApi(QRhiWidget::Api::Metal);
    QSignalSpy frameSpy(&rhiWidget, &QRhiWidget::frameSubmitted);
    QSignalSpy errorSpy(&rhiWidget, &QRhiWidget::renderFailed);
    rhiWidget.resize(320, 240);
    rhiWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&rhiWidget));
    QTRY_VERIFY(errorSpy.count() > 0);
    QCOMPARE(frameSpy.count(), 0);
#endif
}

static QShader getShader(const QString &name)
{
    QFile f(name);
    return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
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

class SimpleRhiWidget : public QRhiWidget
{
public:
    SimpleRhiWidget(int sampleCount = 1, QWidget *parent = nullptr)
        : QRhiWidget(parent),
          m_sampleCount(sampleCount)
    { }

    ~SimpleRhiWidget()
    {
        delete m_rt;
        delete m_rp;
    }

    void initialize(QRhiCommandBuffer *cb) override;
    void render(QRhiCommandBuffer *cb) override;
    void releaseResources() override;

    int m_sampleCount;
    QRhi *m_rhi = nullptr;
    std::unique_ptr<QRhiBuffer> m_vbuf;
    std::unique_ptr<QRhiBuffer> m_ubuf;
    std::unique_ptr<QRhiShaderResourceBindings> m_srb;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    QRhiTextureRenderTarget *m_rt = nullptr;   // used when autoRenderTarget is off
    QRhiRenderPassDescriptor *m_rp = nullptr;  // used when autoRenderTarget is off

    friend class tst_QRhiWidget;
};

void SimpleRhiWidget::initialize(QRhiCommandBuffer *cb)
{
    if (m_rhi != rhi()) {
        m_pipeline.reset();
        m_rhi = rhi();
    }

    if (!m_pipeline) {
        if (!isAutoRenderTargetEnabled()) {
            delete m_rt;
            delete m_rp;
            QRhiTextureRenderTargetDescription rtDesc;
            if (colorTexture()) {
                rtDesc.setColorAttachments({ colorTexture() });
            } else if (msaaColorBuffer()) {
                QRhiColorAttachment att;
                att.setRenderBuffer(msaaColorBuffer());
                rtDesc.setColorAttachments({ att });
            }
            m_rt = m_rhi->newTextureRenderTarget(rtDesc);
            m_rp = m_rt->newCompatibleRenderPassDescriptor();
            m_rt->setRenderPassDescriptor(m_rp);
            m_rt->create();
        }

        static float vertexData[] = {
             0,  1,
            -1, -1,
             1, -1
        };

        m_vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData)));
        m_vbuf->create();

        m_ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
        m_ubuf->create();

        m_srb.reset(m_rhi->newShaderResourceBindings());
        m_srb->create();

        m_pipeline.reset(m_rhi->newGraphicsPipeline());
        m_pipeline->setShaderStages({
            { QRhiShaderStage::Vertex, getShader(QLatin1String(":/data/simple.vert.qsb")) },
            { QRhiShaderStage::Fragment, getShader(QLatin1String(":/data/simple.frag.qsb")) }
        });
        QRhiVertexInputLayout inputLayout;
        inputLayout.setBindings({
            { 2 * sizeof(float) }
        });
        inputLayout.setAttributes({
            { 0, 0, QRhiVertexInputAttribute::Float2, 0 }
        });
        m_pipeline->setSampleCount(m_sampleCount);
        m_pipeline->setVertexInputLayout(inputLayout);
        m_pipeline->setShaderResourceBindings(m_srb.get());
        m_pipeline->setRenderPassDescriptor(renderTarget() ? renderTarget()->renderPassDescriptor() : m_rp);
        m_pipeline->create();

        QRhiResourceUpdateBatch *resourceUpdates = m_rhi->nextResourceUpdateBatch();
        resourceUpdates->uploadStaticBuffer(m_vbuf.get(), vertexData);
        cb->resourceUpdate(resourceUpdates);
    }
}

void SimpleRhiWidget::render(QRhiCommandBuffer *cb)
{
    const QSize outputSize = colorTexture() ? colorTexture()->pixelSize() : msaaColorBuffer()->pixelSize();
    if (renderTarget()) {
        QCOMPARE(outputSize, renderTarget()->pixelSize());
        if (rhi()->backend() != QRhi::Null && rhi()->supportedSampleCounts().contains(m_sampleCount))
            QCOMPARE(m_sampleCount, renderTarget()->sampleCount());
    }

    const QColor clearColor = QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f);
    cb->beginPass(renderTarget() ? renderTarget() : m_rt, clearColor, { 1.0f, 0 });
    cb->setGraphicsPipeline(m_pipeline.get());
    cb->setViewport(QRhiViewport(0, 0, outputSize.width(), outputSize.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf.get(), 0);
    cb->setVertexInput(0, 1, &vbufBinding);
    cb->draw(3);
    cb->endPass();
}

void SimpleRhiWidget::releaseResources()
{
    m_pipeline.reset();
    m_srb.reset();
    m_ubuf.reset();
    m_vbuf.reset();

}

void tst_QRhiWidget::simple_data()
{
    testData();
}

void tst_QRhiWidget::simple()
{
    QFETCH(QRhiWidget::Api, api);

    SimpleRhiWidget *rhiWidget = new SimpleRhiWidget;
    rhiWidget->setApi(api);
    QSignalSpy frameSpy(rhiWidget, &QRhiWidget::frameSubmitted);
    QSignalSpy errorSpy(rhiWidget, &QRhiWidget::renderFailed);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(rhiWidget);

    QWidget w;
    w.setLayout(layout);
    w.resize(1280, 720);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);

    QCOMPARE(rhiWidget->sampleCount(), 1);
    QCOMPARE(rhiWidget->colorBufferFormat(), QRhiWidget::TextureFormat::RGBA8);
    QVERIFY(rhiWidget->isAutoRenderTargetEnabled());

    // Pull out the QRhiTexture (we know colorTexture() and rhi() and friends
    // are all there even outside initialize() and render(), even though this
    // is not quite documented), and read it back.
    QRhiTexture *backingTexture = rhiWidget->colorTexture();
    QVERIFY(backingTexture);
    QCOMPARE(backingTexture->format(), QRhiTexture::RGBA8);
    QVERIFY(rhiWidget->depthStencilBuffer());
    QVERIFY(rhiWidget->renderTarget());
    QVERIFY(!rhiWidget->resolveTexture());
    QRhi *rhi = rhiWidget->rhi();
    QVERIFY(rhi);

    switch (api) {
    case QRhiWidget::Api::OpenGL:
        QCOMPARE(rhi->backend(), QRhi::OpenGLES2);
        break;
    case QRhiWidget::Api::Metal:
        QCOMPARE(rhi->backend(), QRhi::Metal);
        break;
    case QRhiWidget::Api::Vulkan:
        QCOMPARE(rhi->backend(), QRhi::Vulkan);
        break;
    case QRhiWidget::Api::Direct3D11:
        QCOMPARE(rhi->backend(), QRhi::D3D11);
        break;
    case QRhiWidget::Api::Direct3D12:
        QCOMPARE(rhi->backend(), QRhi::D3D12);
        break;
    case QRhiWidget::Api::Null:
        QCOMPARE(rhi->backend(), QRhi::Null);
        break;
    default:
        break;
    }

    const int maxFuzz = 1;
    QImage resultOne;
    if (rhi->backend() != QRhi::Null) {
        QRhiReadbackResult readResult;
        bool readCompleted = false;
        readResult.completed = [&readCompleted] { readCompleted = true; };
        QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
        rub->readBackTexture(backingTexture, &readResult);
        QVERIFY(submitResourceUpdates(rhi, rub));
        QVERIFY(readCompleted);

        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        if (rhi->isYUpInFramebuffer())
            resultOne = wrapperImage.flipped();
        else
            resultOne = wrapperImage.copy();

        // result is now a red triangle upon greenish background, where the
        // triangle's edges are (0, 1), (-1, -1), and (1, -1).
        // It's upside down with Vulkan (Y is not corrected, clipSpaceCorrMatrix() is not used),
        // but that won't matter for the test.

        // Check that the center is a red pixel.
        QRgb c = resultOne.pixel(resultOne.width() / 2, resultOne.height() / 2);
        QVERIFY(qRed(c) >= 255 - maxFuzz);
        QVERIFY(qGreen(c) <= maxFuzz);
        QVERIFY(qBlue(c) <= maxFuzz);
    }

    // Now through grabFramebuffer().
    QImage resultTwo;
    if (rhi->backend() != QRhi::Null) {
        resultTwo = rhiWidget->grabFramebuffer();
        QCOMPARE(errorSpy.count(), 0);
        QVERIFY(!resultTwo.isNull());
        QRgb c = resultTwo.pixel(resultTwo.width() / 2, resultTwo.height() / 2);
        QVERIFY(qRed(c) >= 255 - maxFuzz);
        QVERIFY(qGreen(c) <= maxFuzz);
        QVERIFY(qBlue(c) <= maxFuzz);
    }

    // Check we got the same result from our manual readback and when the
    // texture was rendered to again and grabFramebuffer() was called.
    QVERIFY(imageRGBAEquals(resultOne, resultTwo, maxFuzz));
}

void tst_QRhiWidget::msaa_data()
{
    testData();
}

void tst_QRhiWidget::msaa()
{
    QFETCH(QRhiWidget::Api, api);

    const int SAMPLE_COUNT = 4;
    SimpleRhiWidget *rhiWidget = new SimpleRhiWidget(SAMPLE_COUNT);
    rhiWidget->setApi(api);
    rhiWidget->setSampleCount(SAMPLE_COUNT);
    QSignalSpy frameSpy(rhiWidget, &QRhiWidget::frameSubmitted);
    QSignalSpy errorSpy(rhiWidget, &QRhiWidget::renderFailed);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(rhiWidget);

    QWidget w;
    w.setLayout(layout);
    w.resize(1280, 720);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);

    QCOMPARE(rhiWidget->sampleCount(), 4);
    QCOMPARE(rhiWidget->colorBufferFormat(), QRhiWidget::TextureFormat::RGBA8);
    QVERIFY(!rhiWidget->colorTexture());
    QVERIFY(rhiWidget->msaaColorBuffer());
    QVERIFY(rhiWidget->depthStencilBuffer());
    QVERIFY(rhiWidget->renderTarget());
    QVERIFY(rhiWidget->resolveTexture());
    QCOMPARE(rhiWidget->resolveTexture()->format(), QRhiTexture::RGBA8);
    QRhi *rhi = rhiWidget->rhi();
    QVERIFY(rhi);

    if (rhi->backend() != QRhi::Null) {
        QRhiReadbackResult readResult;
        QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
        rub->readBackTexture(rhiWidget->resolveTexture(), &readResult);
        QVERIFY(submitResourceUpdates(rhi, rub));

        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        QImage result;
        if (rhi->isYUpInFramebuffer())
            result = wrapperImage.flipped();
        else
            result = wrapperImage.copy();

        // Check that the center is a red pixel.
        const int maxFuzz = 1;
        QRgb c = result.pixel(result.width() / 2, result.height() / 2);
        QVERIFY(qRed(c) >= 255 - maxFuzz);
        QVERIFY(qGreen(c) <= maxFuzz);
        QVERIFY(qBlue(c) <= maxFuzz);
    }

    // See if switching back and forth works.
    frameSpy.clear();
    rhiWidget->m_pipeline.reset();
    rhiWidget->m_sampleCount = 1;
    rhiWidget->setSampleCount(1);
    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);
    QVERIFY(rhiWidget->colorTexture());
    QVERIFY(!rhiWidget->msaaColorBuffer());

    frameSpy.clear();
    rhiWidget->m_pipeline.reset();
    rhiWidget->m_sampleCount = SAMPLE_COUNT;
    rhiWidget->setSampleCount(SAMPLE_COUNT);
    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);
    QVERIFY(!rhiWidget->colorTexture());
    QVERIFY(rhiWidget->msaaColorBuffer());
}

void tst_QRhiWidget::fixedSize_data()
{
    testData();
}

void tst_QRhiWidget::fixedSize()
{
    QFETCH(QRhiWidget::Api, api);

    SimpleRhiWidget *rhiWidget = new SimpleRhiWidget;
    rhiWidget->setApi(api);
    QSignalSpy frameSpy(rhiWidget, &QRhiWidget::frameSubmitted);
    QSignalSpy errorSpy(rhiWidget, &QRhiWidget::renderFailed);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(rhiWidget);

    rhiWidget->setFixedColorBufferSize(QSize(320, 200));

    QWidget w;
    w.setLayout(layout);
    w.resize(1280, 720);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);

    QVERIFY(rhiWidget->rhi());
    QVERIFY(rhiWidget->colorTexture());
    QCOMPARE(rhiWidget->colorTexture()->pixelSize(), QSize(320, 200));
    QVERIFY(rhiWidget->depthStencilBuffer());
    QCOMPARE(rhiWidget->depthStencilBuffer()->pixelSize(), QSize(320, 200));
    QVERIFY(rhiWidget->renderTarget());
    QVERIFY(!rhiWidget->resolveTexture());

    frameSpy.clear();
    rhiWidget->setFixedColorBufferSize(640, 480); // should also trigger update()
    QTRY_VERIFY(frameSpy.count() > 0);

    QVERIFY(rhiWidget->colorTexture());
    QCOMPARE(rhiWidget->colorTexture()->pixelSize(), QSize(640, 480));
    QVERIFY(rhiWidget->depthStencilBuffer());
    QCOMPARE(rhiWidget->depthStencilBuffer()->pixelSize(), QSize(640, 480));

    frameSpy.clear();
    rhiWidget->setFixedColorBufferSize(QSize());
    QTRY_VERIFY(frameSpy.count() > 0);

    QVERIFY(rhiWidget->colorTexture());
    QVERIFY(rhiWidget->colorTexture()->pixelSize() != QSize(640, 480));
    QVERIFY(rhiWidget->depthStencilBuffer());
    QVERIFY(rhiWidget->depthStencilBuffer()->pixelSize() != QSize(640, 480));
}

void tst_QRhiWidget::autoRt_data()
{
    testData();
}

void tst_QRhiWidget::autoRt()
{
    QFETCH(QRhiWidget::Api, api);

    SimpleRhiWidget *rhiWidget = new SimpleRhiWidget;
    rhiWidget->setApi(api);
    QVERIFY(rhiWidget->isAutoRenderTargetEnabled());
    rhiWidget->setAutoRenderTarget(false);
    QVERIFY(!rhiWidget->isAutoRenderTargetEnabled());
    QSignalSpy frameSpy(rhiWidget, &QRhiWidget::frameSubmitted);
    QSignalSpy errorSpy(rhiWidget, &QRhiWidget::renderFailed);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(rhiWidget);

    QWidget w;
    w.setLayout(layout);
    w.resize(1280, 720);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);

    QVERIFY(rhiWidget->rhi());
    QVERIFY(rhiWidget->colorTexture());
    QVERIFY(!rhiWidget->depthStencilBuffer());
    QVERIFY(!rhiWidget->renderTarget());
    QVERIFY(!rhiWidget->resolveTexture());

    QVERIFY(rhiWidget->m_rt);
    QVERIFY(rhiWidget->m_rp);
    QCOMPARE(rhiWidget->m_rt->description().cbeginColorAttachments()->texture(), rhiWidget->colorTexture());

    frameSpy.clear();
    // do something that triggers creating a new backing texture
    rhiWidget->setFixedColorBufferSize(QSize(320, 200));
    QTRY_VERIFY(frameSpy.count() > 0);

    QVERIFY(rhiWidget->colorTexture());
    QCOMPARE(rhiWidget->m_rt->description().cbeginColorAttachments()->texture(), rhiWidget->colorTexture());
}

void tst_QRhiWidget::reparent_data()
{
    testData();
}

void tst_QRhiWidget::reparent()
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MultipleWindows))
        QSKIP("MultipleWindows capability is reported as unsupported, skipping reparenting test.");

    QFETCH(QRhiWidget::Api, api);

    QWidget *windowOne = new QWidget;
    windowOne->resize(1280, 720);

    SimpleRhiWidget *rhiWidget = new SimpleRhiWidget(1);
    rhiWidget->setApi(api);
    rhiWidget->resize(800, 600);
    QSignalSpy frameSpy(rhiWidget, &QRhiWidget::frameSubmitted);
    QSignalSpy errorSpy(rhiWidget, &QRhiWidget::renderFailed);

    rhiWidget->show();
    QVERIFY(QTest::qWaitForWindowExposed(rhiWidget));
    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);

    frameSpy.clear();
    rhiWidget->setParent(windowOne);
    windowOne->show();
    QVERIFY(QTest::qWaitForWindowExposed(windowOne));
    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);

    frameSpy.clear();
    QWidget windowTwo;
    windowTwo.resize(1280, 720);

    rhiWidget->setParent(&windowTwo);

    // There's nothing saying the old top-level parent is going to be around,
    // which is interesting wrt to its QRhi and resources created with that;
    // exercise this.
    delete windowOne;

    windowTwo.show();
    QVERIFY(QTest::qWaitForWindowExposed(&windowTwo));
    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);

    // now reparent after show() has already been called
    frameSpy.clear();
    QWidget windowThree;
    windowThree.resize(1280, 720);
    windowThree.show();
    QVERIFY(QTest::qWaitForWindowExposed(&windowThree));

    rhiWidget->setParent(&windowThree);
    // this case needs a show() on rhiWidget
    rhiWidget->show();

    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);
}

void tst_QRhiWidget::grabFramebufferWhileStillInvisible_data()
{
    testData();
}

void tst_QRhiWidget::grabFramebufferWhileStillInvisible()
{
    QFETCH(QRhiWidget::Api, api);

    const int maxFuzz = 1;

    SimpleRhiWidget w;
    w.setApi(api);
    w.resize(1280, 720);
    QSignalSpy errorSpy(&w, &QRhiWidget::renderFailed);

    QImage image = w.grabFramebuffer(); // creates its own QRhi just to render offscreen
    QVERIFY(!image.isNull());
    QVERIFY(w.rhi());
    QVERIFY(w.colorTexture());
    QCOMPARE(errorSpy.count(), 0);
    if (api != QRhiWidget::Api::Null) {
        QRgb c = image.pixel(image.width() / 2, image.height() / 2);
        QVERIFY(qRed(c) >= 255 - maxFuzz);
        QVERIFY(qGreen(c) <= maxFuzz);
        QVERIFY(qBlue(c) <= maxFuzz);
    }

    // Make the window visible, this under the hood drops the QRhiWidget's
    // own QRhi and attaches to the backingstore's.
    QSignalSpy frameSpy(&w, &QRhiWidget::frameSubmitted);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QTRY_VERIFY(frameSpy.count() > 0);

    QCOMPARE(errorSpy.count(), 0);

    if (api != QRhiWidget::Api::Null) {
        QRhiReadbackResult readResult;
        QRhiResourceUpdateBatch *rub = w.rhi()->nextResourceUpdateBatch();
        rub->readBackTexture(w.colorTexture(), &readResult);
        QVERIFY(submitResourceUpdates(w.rhi(), rub));
        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        if (w.rhi()->isYUpInFramebuffer())
            image = wrapperImage.flipped();
        else
            image = wrapperImage.copy();
        QRgb c = image.pixel(image.width() / 2, image.height() / 2);
        QVERIFY(qRed(c) >= 255 - maxFuzz);
        QVERIFY(qGreen(c) <= maxFuzz);
        QVERIFY(qBlue(c) <= maxFuzz);
    }
}

void tst_QRhiWidget::grabViaQWidgetGrab_data()
{
    testData();
}

void tst_QRhiWidget::grabViaQWidgetGrab()
{
    QFETCH(QRhiWidget::Api, api);

    SimpleRhiWidget w;
    w.setApi(api);
    w.resize(1280, 720);
    QSignalSpy frameSpy(&w, &QRhiWidget::frameSubmitted);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QTRY_VERIFY(frameSpy.count() > 0);

    QImage image = w.grab().toImage();

    if (w.rhi()->backend() != QRhi::Null) {
        // It's upside down with Vulkan (Y is not corrected, clipSpaceCorrMatrix() is not used),
        // but that won't matter for the test.
        QRgb c = image.pixel(image.width() / 2, image.height() / 2);
        const int maxFuzz = 1;
        QVERIFY(qRed(c) >= 255 - maxFuzz);
        QVERIFY(qGreen(c) <= maxFuzz);
        QVERIFY(qBlue(c) <= maxFuzz);
    }
}

void tst_QRhiWidget::mirror_data()
{
    testData();
}

void tst_QRhiWidget::mirror()
{
    QFETCH(QRhiWidget::Api, api);

    SimpleRhiWidget *rhiWidget = new SimpleRhiWidget;
    rhiWidget->setApi(api);
    QVERIFY(!rhiWidget->isMirrorVerticallyEnabled());

    QSignalSpy frameSpy(rhiWidget, &QRhiWidget::frameSubmitted);
    QSignalSpy errorSpy(rhiWidget, &QRhiWidget::renderFailed);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(rhiWidget);
    QWidget w;
    w.setLayout(layout);
    w.resize(1280, 720);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);

    frameSpy.clear();
    rhiWidget->setMirrorVertically(true);
    QVERIFY(rhiWidget->isMirrorVerticallyEnabled());
    QTRY_VERIFY(frameSpy.count() > 0);
    QCOMPARE(errorSpy.count(), 0);

    if (api != QRhiWidget::Api::Null) {
        QRhi *rhi = rhiWidget->rhi();
        QRhiReadbackResult readResult;
        QRhiResourceUpdateBatch *rub = rhi->nextResourceUpdateBatch();
        rub->readBackTexture(rhiWidget->colorTexture(), &readResult);
        QVERIFY(submitResourceUpdates(rhi, rub));
        QImage wrapperImage(reinterpret_cast<const uchar *>(readResult.data.constData()),
                            readResult.pixelSize.width(), readResult.pixelSize.height(),
                            QImage::Format_RGBA8888);
        QImage image;
        if (rhi->isYUpInFramebuffer())
            image = wrapperImage.flipped();
        else
            image = wrapperImage.copy();

        const int maxFuzz = 1;
        QRgb c = image.pixel(50, 5);
        if (api != QRhiWidget::Api::Vulkan) {
            // this should be the background (greenish), not the red triangle
            QVERIFY(qGreen(c) > qRed(c));
        } else {
            // remember that Vulkan is upside down due to not correcting for Y down in NDC
            // hence this is red
            QVERIFY(qRed(c) >= 255 - maxFuzz);
            QVERIFY(qGreen(c) <= maxFuzz);
        }
        QVERIFY(qBlue(c) <= maxFuzz);
    }
}

QTEST_MAIN(tst_QRhiWidget)

#include "tst_qrhiwidget.moc"
