// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define EXAMPLEFW_IMGUI
#include "../shared/examplefw.h"
#include "../shared/cube.h"

// Another tessellation test. Compatible with Direct 3D via hand-written hull
// and domain shaders, but this already pushes the limits of what is sensible
// when it comes to injecting hand-written HLSL code to get tessellation
// functional (cbuffer layout, resource registers all need to be figured out
// manually and works only as long as the GLSL source is not changing, etc.).
// Note that the domain shader must use SampleLevel (textureLod), it won't
// compile for ds_5_0 otherwise.

static const quint32 UBUF_SIZE = 80;

struct {
    QList<QRhiResource *> releasePool;

    QRhiBuffer *vbuf;
    QRhiBuffer *ubuf;
    QRhiTexture *tex;
    QRhiSampler *sampler;
    QRhiShaderResourceBindings *srb;
    QRhiGraphicsPipeline *psWire;
    QRhiGraphicsPipeline *psSolid;
    bool rotate = true;
    float rotation = 0.0f;
    float viewZ = 0.0f;
    float displacementAmount = 0.0f;
    int tessInner = 4;
    int tessOuter = 4;
    bool useTex = false;
    bool wireframe = true;

    QRhiResourceUpdateBatch *initialUpdates = nullptr;
} d;

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::Tessellation))
        qFatal("Tessellation is not supported");

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, m_r->ubufAligned(UBUF_SIZE));
    d.ubuf->create();
    d.releasePool << d.ubuf;

    QImage image;
    image.load(":/heightmap.png");
    if (image.isNull())
        qFatal("Failed to load displacement map");

    d.tex = m_r->newTexture(QRhiTexture::RGBA8, image.size());
    d.tex->create();
    d.releasePool << d.tex;

    d.initialUpdates->uploadTexture(d.tex, image);

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::Repeat, QRhiSampler::Repeat);
    d.releasePool << d.sampler;
    d.sampler->create();

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0,
                           QRhiShaderResourceBinding::TessellationControlStage
                           | QRhiShaderResourceBinding::TessellationEvaluationStage,
                           d.ubuf),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::TessellationEvaluationStage, d.tex, d.sampler)
        });
    d.srb->create();

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },
        { 2 * sizeof(float) },
        { 3 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 },
        { 2, 2, QRhiVertexInputAttribute::Float3, 0 }
    });

    const QRhiShaderStage stages[] = {
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/material.vert.qsb")) },
        { QRhiShaderStage::TessellationControl, getShader(QLatin1String(":/material.tesc.qsb")) },
        { QRhiShaderStage::TessellationEvaluation, getShader(QLatin1String(":/material.tese.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/material.frag.qsb")) }
    };

    d.psWire = m_r->newGraphicsPipeline();
    d.releasePool << d.psWire;
    d.psWire->setTopology(QRhiGraphicsPipeline::Patches);
    d.psWire->setPatchControlPointCount(3);
    d.psWire->setShaderStages(stages, stages + 4);
    d.psWire->setDepthTest(true);
    d.psWire->setDepthWrite(true);
    d.psWire->setCullMode(QRhiGraphicsPipeline::Back);
    d.psWire->setPolygonMode(QRhiGraphicsPipeline::Line);
    d.psWire->setVertexInputLayout(inputLayout);
    d.psWire->setShaderResourceBindings(d.srb);
    d.psWire->setRenderPassDescriptor(m_rp);
    d.psWire->create();

    d.psSolid = m_r->newGraphicsPipeline();
    d.releasePool << d.psSolid;
    d.psSolid->setTopology(QRhiGraphicsPipeline::Patches);
    d.psSolid->setPatchControlPointCount(3);
    d.psSolid->setShaderStages(stages, stages + 4);
    d.psSolid->setDepthTest(true);
    d.psSolid->setDepthWrite(true);
    d.psSolid->setCullMode(QRhiGraphicsPipeline::Back);
    d.psSolid->setVertexInputLayout(inputLayout);
    d.psSolid->setShaderResourceBindings(d.srb);
    d.psSolid->setRenderPassDescriptor(m_rp);
    d.psSolid->create();
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = nullptr;
    if (d.initialUpdates) {
        u = d.initialUpdates;
        d.initialUpdates = nullptr;
    }

    char *p = d.ubuf->beginFullDynamicBufferUpdateForCurrentFrame();
    QMatrix4x4 mvp = m_proj;
    mvp.translate(0, 0, d.viewZ);
    mvp.rotate(d.rotation, 1, 1, 0);
    mvp.scale(0.5f);

    memcpy(p, mvp.constData(), 64);
    memcpy(p + 64, &d.displacementAmount, sizeof(float));
    float tessInnerFloat = d.tessInner;
    memcpy(p + 68, &tessInnerFloat, sizeof(float));
    float tessOuterFloat = d.tessOuter;
    memcpy(p + 72, &tessOuterFloat, sizeof(float));
    qint32 useTex = d.useTex ? 1 : 0;
    memcpy(p + 76, &useTex, sizeof(qint32));

    d.ubuf->endFullDynamicBufferUpdateForCurrentFrame();

    const QRhiCommandBuffer::VertexInput vbufBinding[] = {
        { d.vbuf, 0 },
        { d.vbuf, quint32(36 * 3 * sizeof(float)) },
        { d.vbuf, quint32(36 * (3 + 2) * sizeof(float)) }
    };

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u, QRhiCommandBuffer::DoNotTrackResourcesForCompute);

    cb->setGraphicsPipeline(d.wireframe ? d.psWire : d.psSolid);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources(d.srb);
    cb->setVertexInput(0, 3, vbufBinding);
    cb->draw(36);

    m_imguiRenderer->render();

    cb->endPass();

    if (d.rotate)
        d.rotation += 1;
}

void Window::customGui()
{
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin("Test");
    ImGui::SliderInt("Inner", &d.tessInner, 0, 20);
    ImGui::SliderInt("Outer", &d.tessOuter, 0, 20);
    ImGui::SliderFloat("Displacement", &d.displacementAmount, 0.0f, 4.0f);
    ImGui::Checkbox("Use displacement texture", &d.useTex);
    ImGui::SliderFloat("Z", &d.viewZ, -16.0f, 4.0f);
    ImGui::Checkbox("Rotate", &d.rotate);
    ImGui::Checkbox("Wireframe", &d.wireframe);
    ImGui::End();
}
