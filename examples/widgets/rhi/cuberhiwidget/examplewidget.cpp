// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "examplewidget.h"
#include "cube.h"

#include <QFile>
#include <QPainter>
#include <QtGui/qquaternion.h>

static const QSize CUBE_TEX_SIZE(512, 512);

ExampleRhiWidget::ExampleRhiWidget(QWidget *parent)
    : QRhiWidget(parent)
{
}

//![init-1]
void ExampleRhiWidget::initialize(QRhiCommandBuffer *)
{
    if (m_rhi != rhi()) {
        m_rhi = rhi();
        scene = {};
        emit rhiChanged(QString::fromUtf8(m_rhi->backendName()));
    }
    if (m_pixelSize != renderTarget()->pixelSize()) {
        m_pixelSize = renderTarget()->pixelSize();
        emit resized();
    }
    if (m_sampleCount != renderTarget()->sampleCount()) {
        m_sampleCount = renderTarget()->sampleCount();
        scene = {};
    }
//![init-1]

//![init-2]
    if (!scene.vbuf) {
        initScene();
        updateCubeTexture();
    }

    scene.mvp = m_rhi->clipSpaceCorrMatrix();
    scene.mvp.perspective(45.0f, m_pixelSize.width() / (float) m_pixelSize.height(), 0.01f, 1000.0f);
    scene.mvp.translate(0, 0, -4);
    updateMvp();
}
//![init-2]

//![rotation-update]
void ExampleRhiWidget::updateMvp()
{
    QMatrix4x4 mvp = scene.mvp * QMatrix4x4(QQuaternion::fromEulerAngles(QVector3D(30, itemData.cubeRotation, 0)).toRotationMatrix());
    if (!scene.resourceUpdates)
        scene.resourceUpdates = m_rhi->nextResourceUpdateBatch();
    scene.resourceUpdates->updateDynamicBuffer(scene.ubuf.get(), 0, 64, mvp.constData());
}
//![rotation-update]

//![texture-update]
void ExampleRhiWidget::updateCubeTexture()
{
    QImage image(CUBE_TEX_SIZE, QImage::Format_RGBA8888);
    const QRect r(QPoint(0, 0), CUBE_TEX_SIZE);
    QPainter p(&image);
    p.fillRect(r, QGradient::DeepBlue);
    QFont font;
    font.setPointSize(24);
    p.setFont(font);
    p.drawText(r, itemData.cubeText);
    p.end();

    if (!scene.resourceUpdates)
        scene.resourceUpdates = m_rhi->nextResourceUpdateBatch();
    scene.resourceUpdates->uploadTexture(scene.cubeTex.get(), image);
}
//![texture-update]

static QShader getShader(const QString &name)
{
    QFile f(name);
    return f.open(QIODevice::ReadOnly) ? QShader::fromSerialized(f.readAll()) : QShader();
}

void ExampleRhiWidget::initScene()
{
//![setup-scene]
    scene.vbuf.reset(m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube)));
    scene.vbuf->create();

    scene.resourceUpdates = m_rhi->nextResourceUpdateBatch();
    scene.resourceUpdates->uploadStaticBuffer(scene.vbuf.get(), cube);

    scene.ubuf.reset(m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64));
    scene.ubuf->create();

    scene.cubeTex.reset(m_rhi->newTexture(QRhiTexture::RGBA8, CUBE_TEX_SIZE));
    scene.cubeTex->create();

    scene.sampler.reset(m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                               QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
    scene.sampler->create();

    scene.srb.reset(m_rhi->newShaderResourceBindings());
    scene.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage, scene.ubuf.get()),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, scene.cubeTex.get(), scene.sampler.get())
    });
    scene.srb->create();

    scene.ps.reset(m_rhi->newGraphicsPipeline());
    scene.ps->setDepthTest(true);
    scene.ps->setDepthWrite(true);
    scene.ps->setCullMode(QRhiGraphicsPipeline::Back);
    scene.ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/shader_assets/texture.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/shader_assets/texture.frag.qsb")) }
    });
    QRhiVertexInputLayout inputLayout;
    // The cube is provided as non-interleaved sets of positions, UVs, normals.
    // Normals are not interesting here, only need the positions and UVs.
    inputLayout.setBindings({
        { 3 * sizeof(float) },
        { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 }
    });
    scene.ps->setSampleCount(m_sampleCount);
    scene.ps->setVertexInputLayout(inputLayout);
    scene.ps->setShaderResourceBindings(scene.srb.get());
    scene.ps->setRenderPassDescriptor(renderTarget()->renderPassDescriptor());
    scene.ps->create();
//![setup-scene]
}

//![render]
void ExampleRhiWidget::render(QRhiCommandBuffer *cb)
{
    if (itemData.cubeRotationDirty) {
        itemData.cubeRotationDirty = false;
        updateMvp();
    }

    if (itemData.cubeTextDirty) {
        itemData.cubeTextDirty = false;
        updateCubeTexture();
    }

    QRhiResourceUpdateBatch *resourceUpdates = scene.resourceUpdates;
    if (resourceUpdates)
        scene.resourceUpdates = nullptr;

    const QColor clearColor = QColor::fromRgbF(0.4f, 0.7f, 0.0f, 1.0f);
    cb->beginPass(renderTarget(), clearColor, { 1.0f, 0 }, resourceUpdates);

    cb->setGraphicsPipeline(scene.ps.get());
    cb->setViewport(QRhiViewport(0, 0, m_pixelSize.width(), m_pixelSize.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { scene.vbuf.get(), 0 },
        { scene.vbuf.get(), quint32(36 * 3 * sizeof(float)) }
    };
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(36);

    cb->endPass();
}
//![render]

void ExampleRhiWidget::releaseResources()
{
    scene = {}; // a subsequent initialize() will recreate everything
}
