// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
#include <QGuiApplication>
#include <QImage>
#include <QFile>
#include <rhi/qrhi.h>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

#if QT_CONFIG(vulkan)
    QVulkanInstance inst;
#endif
    std::unique_ptr<QRhi> rhi;
#if defined(Q_OS_WIN)
    QRhiD3D12InitParams params;
    rhi.reset(QRhi::create(QRhi::D3D12, &params));
#elif QT_CONFIG(metal)
    QRhiMetalInitParams params;
    rhi.reset(QRhi::create(QRhi::Metal, &params));
#elif QT_CONFIG(vulkan)
    inst.setExtensions(QRhiVulkanInitParams::preferredInstanceExtensions());
    if (inst.create()) {
        QRhiVulkanInitParams params;
        params.inst = &inst;
        rhi.reset(QRhi::create(QRhi::Vulkan, &params));
    } else {
        qFatal("Failed to create Vulkan instance");
    }
#endif
    if (rhi)
        qDebug() << rhi->backendName() << rhi->driverInfo();
    else
        qFatal("Failed to initialize RHI");

    float rotation = 0.0f;
    float opacity = 1.0f;
    int opacityDir = 1;

    std::unique_ptr<QRhiTexture> tex(rhi->newTexture(QRhiTexture::RGBA8,
                                                     QSize(1280, 720),
                                                     1,
                                                     QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    tex->create();
    std::unique_ptr<QRhiTextureRenderTarget> rt(rhi->newTextureRenderTarget({ tex.get() }));
    std::unique_ptr<QRhiRenderPassDescriptor> rp(rt->newCompatibleRenderPassDescriptor());
    rt->setRenderPassDescriptor(rp.get());
    rt->create();

    QMatrix4x4 viewProjection = rhi->clipSpaceCorrMatrix();
    viewProjection.perspective(45.0f, 1280 / 720.f, 0.01f, 1000.0f);
    viewProjection.translate(0, 0, -4);

    static float vertexData[] = { // Y up, CCW
        0.0f,   0.5f,     1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f,     0.0f, 1.0f, 0.0f,
        0.5f,  -0.5f,     0.0f, 0.0f, 1.0f,
    };

    std::unique_ptr<QRhiBuffer> vbuf(rhi->newBuffer(QRhiBuffer::Immutable,
                                                    QRhiBuffer::VertexBuffer,
                                                    sizeof(vertexData)));
    vbuf->create();

    std::unique_ptr<QRhiBuffer> ubuf(rhi->newBuffer(QRhiBuffer::Dynamic,
                                                    QRhiBuffer::UniformBuffer,
                                                    64 + 4));
    ubuf->create();

    std::unique_ptr<QRhiShaderResourceBindings> srb(rhi->newShaderResourceBindings());
    srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0,
                                                 QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                                                 ubuf.get())
    });
    srb->create();

    std::unique_ptr<QRhiGraphicsPipeline> ps(rhi->newGraphicsPipeline());
    QRhiGraphicsPipeline::TargetBlend premulAlphaBlend;
    premulAlphaBlend.enable = true;
    ps->setTargetBlends({ premulAlphaBlend });
    static auto getShader = [](const QString &name) {
        QFile f(name);
        return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
    };
    ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String("color.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String("color.frag.qsb")) }
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
    ps->setShaderResourceBindings(srb.get());
    ps->setRenderPassDescriptor(rp.get());
    ps->create();

    QRhiCommandBuffer *cb;
    for (int frame = 0; frame < 20; ++frame) {
        rhi->beginOffscreenFrame(&cb);

        QRhiResourceUpdateBatch *u = rhi->nextResourceUpdateBatch();
        if (frame == 0)
            u->uploadStaticBuffer(vbuf.get(), vertexData);

        QMatrix4x4 mvp = viewProjection;
        mvp.rotate(rotation, 0, 1, 0);
        u->updateDynamicBuffer(ubuf.get(), 0, 64, mvp.constData());
        rotation += 5.0f;

        u->updateDynamicBuffer(ubuf.get(), 64, 4, &opacity);
        opacity += opacityDir * 0.2f;
        if (opacity < 0.0f || opacity > 1.0f) {
            opacityDir *= -1;
            opacity = qBound(0.0f, opacity, 1.0f);
        }

        cb->beginPass(rt.get(), Qt::green, { 1.0f, 0 }, u);
        cb->setGraphicsPipeline(ps.get());
        cb->setViewport({ 0, 0, 1280, 720 });
        cb->setShaderResources();
        const QRhiCommandBuffer::VertexInput vbufBinding(vbuf.get(), 0);
        cb->setVertexInput(0, 1, &vbufBinding);
        cb->draw(3);
        QRhiReadbackResult readbackResult;
        u = rhi->nextResourceUpdateBatch();
        u->readBackTexture({ tex.get() }, &readbackResult);
        cb->endPass(u);

        rhi->endOffscreenFrame();

        QImage image(reinterpret_cast<const uchar *>(readbackResult.data.constData()),
                     readbackResult.pixelSize.width(),
                     readbackResult.pixelSize.height(),
                     QImage::Format_RGBA8888_Premultiplied);
        if (rhi->isYUpInFramebuffer())
            image.flip();
        image.save(QString::asprintf("frame%d.png", frame));
    }

    return 0;
}
//! [0]
