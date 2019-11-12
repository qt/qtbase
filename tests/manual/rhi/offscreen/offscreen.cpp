/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QGuiApplication>
#include <QImage>
#include <QFileInfo>
#include <QFile>
#include <QLoggingCategory>
#include <QCommandLineParser>
#include <QtGui/private/qshader_p.h>

#include <QtGui/private/qrhinull_p.h>

#ifndef QT_NO_OPENGL
#include <QtGui/private/qrhigles2_p.h>
#include <QOffscreenSurface>
#endif

#if QT_CONFIG(vulkan)
#include <QLoggingCategory>
#include <QtGui/private/qrhivulkan_p.h>
#endif

#ifdef Q_OS_WIN
#include <QtGui/private/qrhid3d11_p.h>
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
#include <QtGui/private/qrhimetal_p.h>
#endif

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
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
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
    QCommandLineOption d3dOption({ "d", "d3d11" }, QLatin1String("Direct3D 11"));
    cmdLineParser.addOption(d3dOption);
    QCommandLineOption mtlOption({ "m", "metal" }, QLatin1String("Metal"));
    cmdLineParser.addOption(mtlOption);
    QCommandLineOption nullOption({ "n", "null" }, QLatin1String("Null"));
    cmdLineParser.addOption(nullOption);
    cmdLineParser.process(app);
    if (cmdLineParser.isSet(glOption))
        graphicsApi = OpenGL;
    if (cmdLineParser.isSet(vkOption))
        graphicsApi = Vulkan;
    if (cmdLineParser.isSet(d3dOption))
        graphicsApi = D3D11;
    if (cmdLineParser.isSet(mtlOption))
        graphicsApi = Metal;
    if (cmdLineParser.isSet(nullOption))
        graphicsApi = Null;

    qDebug("Selected graphics API is %s", qPrintable(graphicsApiName()));
    qDebug("This is a multi-api example, use command line arguments to override:\n%s", qPrintable(cmdLineParser.helpText()));

    QRhi *r = nullptr;

    if (graphicsApi == Null) {
        QRhiNullInitParams params;
        r = QRhi::create(QRhi::Null, &params);
    }

#if QT_CONFIG(vulkan)
    QVulkanInstance inst;
    if (graphicsApi == Vulkan) {
        QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
#ifndef Q_OS_ANDROID
        inst.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");
#else
        inst.setLayers(QByteArrayList()
                       << "VK_LAYER_GOOGLE_threading"
                       << "VK_LAYER_LUNARG_parameter_validation"
                       << "VK_LAYER_LUNARG_object_tracker"
                       << "VK_LAYER_LUNARG_core_validation"
                       << "VK_LAYER_LUNARG_image"
                       << "VK_LAYER_LUNARG_swapchain"
                       << "VK_LAYER_GOOGLE_unique_objects");
#endif
        if (inst.create()) {
            QRhiVulkanInitParams params;
            params.inst = &inst;
            r = QRhi::create(QRhi::Vulkan, &params);
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
        r = QRhi::create(QRhi::OpenGLES2, &params);
    }
#endif

#ifdef Q_OS_WIN
    if (graphicsApi == D3D11) {
        QRhiD3D11InitParams params;
        params.enableDebugLayer = true;
        r = QRhi::create(QRhi::D3D11, &params);
    }
#endif

#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    if (graphicsApi == Metal) {
        QRhiMetalInitParams params;
        r = QRhi::create(QRhi::Metal, &params);
    }
#endif

    if (!r)
        qFatal("Failed to initialize RHI");

    QRhiTexture *tex = r->newTexture(QRhiTexture::RGBA8, QSize(1280, 720), 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
    tex->build();
    QRhiTextureRenderTarget *rt = r->newTextureRenderTarget({ tex });
    QRhiRenderPassDescriptor *rp = rt->newCompatibleRenderPassDescriptor();
    rt->setRenderPassDescriptor(rp);
    rt->build();

    QMatrix4x4 proj = r->clipSpaceCorrMatrix();
    proj.perspective(45.0f, 1280 / 720.f, 0.01f, 1000.0f);
    proj.translate(0, 0, -4);

    QRhiBuffer *vbuf = r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
    vbuf->build();

    QRhiBuffer *ubuf = r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    ubuf->build();

    QRhiShaderResourceBindings *srb = r->newShaderResourceBindings();
    srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, ubuf)
    });
    srb->build();

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
    ps->build();

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
