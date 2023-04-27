// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QImage>
#include <QFileInfo>
#include <QFile>
#include <QLoggingCategory>
#include <QCommandLineParser>
#include <QLoggingCategory>
#include <QOffscreenSurface>

#include <rhi/qrhi.h>

//#define TEST_FINISH

static float vertexData[] = { // Y up (note m_proj), CCW
     0.0f,   0.5f,   1.0f, 0.0f, 0.0f,
    -0.5f,  -0.5f,   0.0f, 1.0f, 0.0f,
     0.5f,  -0.5f,   0.0f, 0.0f, 1.0f,
};

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

enum GraphicsApi
{
    OpenGL,
    Vulkan,
    D3D11,
    D3D12,
    Metal,
    Null
};

GraphicsApi graphicsApi;

QString graphicsApiName()
{
    switch (graphicsApi) {
    case OpenGL:
        return QLatin1String("OpenGL 2.x");
    case Vulkan:
        return QLatin1String("Vulkan");
    case D3D11:
        return QLatin1String("Direct3D 11");
    case D3D12:
        return QLatin1String("Direct3D 12");
    case Metal:
        return QLatin1String("Metal");
    case Null:
        return QLatin1String("Null");
    default:
        break;
    }
    return QString();
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

#if defined(Q_OS_WIN)
    graphicsApi = D3D11;
#elif defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    graphicsApi = Metal;
#elif QT_CONFIG(vulkan)
    graphicsApi = Vulkan;
#else
    graphicsApi = OpenGL;
#endif

    QCommandLineParser cmdLineParser;
    cmdLineParser.addHelpOption();
    QCommandLineOption glOption({ "g", "opengl" }, QLatin1String("OpenGL (2.x)"));
    cmdLineParser.addOption(glOption);
    QCommandLineOption vkOption({ "v", "vulkan" }, QLatin1String("Vulkan"));
    cmdLineParser.addOption(vkOption);
    QCommandLineOption d3d11Option({ "d", "d3d11" }, QLatin1String("Direct3D 11"));
    cmdLineParser.addOption(d3d11Option);
    QCommandLineOption d3d12Option({ "D", "d3d12" }, QLatin1String("Direct3D 12"));
    cmdLineParser.addOption(d3d12Option);
    QCommandLineOption mtlOption({ "m", "metal" }, QLatin1String("Metal"));
    cmdLineParser.addOption(mtlOption);
    QCommandLineOption nullOption({ "n", "null" }, QLatin1String("Null"));
    cmdLineParser.addOption(nullOption);
    cmdLineParser.process(app);
    if (cmdLineParser.isSet(glOption))
        graphicsApi = OpenGL;
    if (cmdLineParser.isSet(vkOption))
        graphicsApi = Vulkan;
    if (cmdLineParser.isSet(d3d11Option))
        graphicsApi = D3D11;
    if (cmdLineParser.isSet(d3d12Option))
        graphicsApi = D3D12;
    if (cmdLineParser.isSet(mtlOption))
        graphicsApi = Metal;
    if (cmdLineParser.isSet(nullOption))
        graphicsApi = Null;

    qDebug("Selected graphics API is %s", qPrintable(graphicsApiName()));
    qDebug("This is a multi-api example, use command line arguments to override:\n%s", qPrintable(cmdLineParser.helpText()));

    QRhi *r = nullptr;
    QRhi::Flags rhiFlags = QRhi::EnableTimestamps;

    if (graphicsApi == Null) {
        QRhiNullInitParams params;
        r = QRhi::create(QRhi::Null, &params, rhiFlags);
    }

#if QT_CONFIG(vulkan)
    QVulkanInstance inst;
    if (graphicsApi == Vulkan) {
        QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
        inst.setLayers({ "VK_LAYER_KHRONOS_validation" });
        inst.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
        if (inst.create()) {
            QRhiVulkanInitParams params;
            params.inst = &inst;
            r = QRhi::create(QRhi::Vulkan, &params, rhiFlags);
        } else {
            qWarning("Failed to create Vulkan instance, switching to OpenGL");
            graphicsApi = OpenGL;
        }
    }
#endif

#ifndef QT_NO_OPENGL
    QScopedPointer<QOffscreenSurface> offscreenSurface;
    if (graphicsApi == OpenGL) {
        offscreenSurface.reset(QRhiGles2InitParams::newFallbackSurface());
        QRhiGles2InitParams params;
        params.fallbackSurface = offscreenSurface.data();
        r = QRhi::create(QRhi::OpenGLES2, &params, rhiFlags);
    }
#endif

#ifdef Q_OS_WIN
    if (graphicsApi == D3D11) {
        QRhiD3D11InitParams params;
        params.enableDebugLayer = true;
        r = QRhi::create(QRhi::D3D11, &params, rhiFlags);
    } else if (graphicsApi == D3D12) {
        QRhiD3D12InitParams params;
        params.enableDebugLayer = true;
        r = QRhi::create(QRhi::D3D12, &params, rhiFlags);
    }
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    if (graphicsApi == Metal) {
        QRhiMetalInitParams params;
        r = QRhi::create(QRhi::Metal, &params, rhiFlags);
    }
#endif

    if (!r)
        qFatal("Failed to initialize RHI");

    QRhiTexture *tex = r->newTexture(QRhiTexture::RGBA8, QSize(1280, 720), 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
    tex->create();
    QRhiTextureRenderTarget *rt = r->newTextureRenderTarget({ tex });
    QRhiRenderPassDescriptor *rp = rt->newCompatibleRenderPassDescriptor();
    rt->setRenderPassDescriptor(rp);
    rt->create();

    QMatrix4x4 proj = r->clipSpaceCorrMatrix();
    proj.perspective(45.0f, 1280 / 720.f, 0.01f, 1000.0f);
    proj.translate(0, 0, -4);

    QRhiBuffer *vbuf = r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
    vbuf->create();

    QRhiBuffer *ubuf = r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    ubuf->create();

    QRhiShaderResourceBindings *srb = r->newShaderResourceBindings();
    srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf)
    });
    srb->create();

    QRhiGraphicsPipeline *ps = r->newGraphicsPipeline();

    QRhiGraphicsPipeline::TargetBlend premulAlphaBlend;
    premulAlphaBlend.enable = true;
    ps->setTargetBlends({ premulAlphaBlend });

    const QShader vs = getShader(QLatin1String(":/color.vert.qsb"));
    if (!vs.isValid())
        qFatal("Failed to load shader pack (vertex)");
    const QShader fs = getShader(QLatin1String(":/color.frag.qsb"));
    if (!fs.isValid())
        qFatal("Failed to load shader pack (fragment)");

    ps->setShaderStages({
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

    ps->setVertexInputLayout(inputLayout);
    ps->setShaderResourceBindings(srb);
    ps->setRenderPassDescriptor(rp);
    ps->create();

    int frame = 0;
    for (; frame < 20; ++frame) {
        QRhiCommandBuffer *cb;
        if (r->beginOffscreenFrame(&cb) != QRhi::FrameOpSuccess)
            break;

        qDebug("Generating offscreen frame %d", frame);
        QRhiResourceUpdateBatch *u = r->nextResourceUpdateBatch();
        if (frame == 0)
            u->uploadStaticBuffer(vbuf, vertexData);

        static float rotation = 0.0f;
        QMatrix4x4 mvp = proj;
        mvp.rotate(rotation, 0, 1, 0);
        u->updateDynamicBuffer(ubuf, 0, 64, mvp.constData());
        rotation += 5.0f;
        static float opacity = 1.0f;
        static int opacityDir= 1;
        u->updateDynamicBuffer(ubuf, 64, 4, &opacity);
        opacity += opacityDir * 0.005f;
        if (opacity < 0.0f || opacity > 1.0f) {
            opacityDir *= -1;
            opacity = qBound(0.0f, opacity, 1.0f);
        }

        cb->beginPass(rt, QColor::fromRgbF(0.0f, 1.0f, 0.0f, 1.0f), { 1, 0 }, u);
        cb->setGraphicsPipeline(ps);
        cb->setViewport({ 0, 0, 1280, 720 });
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput vbufBinding(vbuf, 0);
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->draw(3);

        u = r->nextResourceUpdateBatch();
        QRhiReadbackDescription rb(tex);
        QRhiReadbackResult rbResult;
        rbResult.completed = [frame] { qDebug("  - readback %d completed", frame); };
        u->readBackTexture(rb, &rbResult);

        cb->endPass(u);

        qDebug("Submit and wait");
#ifdef TEST_FINISH
        r->finish();
#else
        r->endOffscreenFrame();
#endif
        // The data should be ready either because endOffscreenFrame() waits
        // for completion or because finish() did.
        if (!rbResult.data.isEmpty()) {
            const uchar *p = reinterpret_cast<const uchar *>(rbResult.data.constData());
            QImage image(p, rbResult.pixelSize.width(), rbResult.pixelSize.height(), QImage::Format_RGBA8888);
            QString fn = QString::asprintf("frame%d.png", frame);
            fn = QFileInfo(fn).absoluteFilePath();
            qDebug("Saving into %s", qPrintable(fn));
            if (r->isYUpInFramebuffer())
                image.mirrored().save(fn);
            else
                image.save(fn);
        } else {
            qWarning("Readback failed!");
        }
#ifdef TEST_FINISH
        r->endOffscreenFrame();
#endif
        if (r->isFeatureSupported(QRhi::Timestamps))
            qDebug() << "GPU time:" << cb->lastCompletedGpuTime() << "seconds (may refer to a previous frame)";
    }

    delete ps;
    delete srb;
    delete ubuf;
    delete vbuf;

    delete rt;
    delete rp;
    delete tex;

    delete r;

    qDebug("\nRendered and read back %d frames using %s", frame, qPrintable(graphicsApiName()));

    return 0;
}
