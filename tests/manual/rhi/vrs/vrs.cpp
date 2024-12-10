// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define EXAMPLEFW_IMGUI
#define EXAMPLEFW_BEFORE_FRAME
#include "../shared/examplefw.h"

#include "../shared/cube.h"

#include <QPainter>
#include <QRandomGenerator>

#if QT_CONFIG(metal)
void *makeRateMap(QRhi *rhi, const QSize &outputSizeInPixels);
void releaseRateMap(void *map);
#endif

const int CUBE_COUNT = 10;

struct {
    QMatrix4x4 winProj;
    QList<QRhiResource *> releasePool;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTexture *tex = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    bool showDemoWindow = true;
    float rotation = 35.0f;
    bool vrsSupported = false;
    bool vrsMapSupported = false;
    bool vrsMapImageSupported = false;
    QMap<int, QList<QSize>> supportedShadingRates;
    int cps[2] = {};
    bool applyRateMapWithImage = false;
    bool applyRateMapNative = false;
    bool applyRateMapPending = false;
    QRhiShadingRateMap *rateMap = nullptr;
    QRhiRenderPassDescriptor *scRpWithRateMap = nullptr;
    QRhiTexture *rateMapTexture = nullptr;
    QRhiTexture *rateMapTextureForVisualization = nullptr;
    void *nativeRateMap = nullptr;
    QSize nativeRateMapSize;
    QVector<float> tx;
    QVector<float> ty;
    QVector<float> scale;
    quint32 ubufAlignedSize;

    bool textureBased = false;
    QRhiTexture *outTexture = nullptr;
    QRhiTextureRenderTarget *texRt = nullptr;
    QRhiRenderPassDescriptor *texRtRp = nullptr;
    QRhiRenderPassDescriptor *texRtRpWithRateMap = nullptr;
} d;

void Window::customInit()
{
    d.vrsSupported = m_r->isFeatureSupported(QRhi::VariableRateShading);
    d.vrsMapSupported = m_r->isFeatureSupported(QRhi::VariableRateShadingMap);
    d.vrsMapImageSupported = m_r->isFeatureSupported(QRhi::VariableRateShadingMapWithTexture);
    for (int sampleCount : { 1, 2, 4, 8, 16 })
        d.supportedShadingRates.insert(sampleCount, m_r->supportedShadingRates(sampleCount));

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.vbuf->create();
    d.releasePool << d.vbuf;

    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);

    d.ubufAlignedSize = m_r->ubufAligned(68);
    const quint32 ubufSize = d.ubufAlignedSize * CUBE_COUNT;
    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, ubufSize);
    d.ubuf->create();
    d.releasePool << d.ubuf;

    QImage image = QImage(QLatin1String(":/qt256.png")).convertToFormat(QImage::Format_RGBA8888).flipped();
    d.tex = m_r->newTexture(QRhiTexture::RGBA8, QSize(image.width(), image.height()), 1, {});
    d.releasePool << d.tex;
    d.tex->create();
    d.initialUpdates->uploadTexture(d.tex, image);

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->create();

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBufferWithDynamicOffset(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf, 68),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.tex, d.sampler)
    });
    d.srb->create();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;

    d.ps->setFlags(QRhiGraphicsPipeline::UsesShadingRate);

    d.ps->setCullMode(QRhiGraphicsPipeline::Back);
    const QRhiShaderStage stages[] = {
        { QRhiShaderStage::Vertex, getShader(QLatin1String(":/texture.vert.qsb")) },
        { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture.frag.qsb")) }
    };
    d.ps->setShaderStages(stages, stages + 2);
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },
        { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 }
    });
    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();

    // resources for trying out rendering into a texture
    d.outTexture = m_r->newTexture(QRhiTexture::RGBA8, QSize(1024, 1024), 1, QRhiTexture::RenderTarget);
    d.releasePool << d.outTexture;
    d.outTexture->create();

    d.texRt = m_r->newTextureRenderTarget({ d.outTexture });
    d.releasePool << d.texRt;
    d.texRtRp = d.texRt->newCompatibleRenderPassDescriptor();
    d.releasePool << d.texRtRp;
    d.texRt->setRenderPassDescriptor(d.texRtRp);
    d.texRt->create();

    QRandomGenerator *rg = QRandomGenerator::global();
    for (int i = 0; i < CUBE_COUNT; i++) {
        d.tx.append(rg->bounded(-20, 20) / 10.0f);
        d.ty.append(rg->bounded(-20, 20) / 10.0f);
        d.scale.append(rg->bounded(0, 10) / 10.0f);
    }
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();

#if QT_CONFIG(metal)
    if (d.nativeRateMap)
        releaseRateMap(d.nativeRateMap);
#endif
}

void Window::customBeforeFrame()
{
    // This function is invoked before calling rhi->beginFrame().
    // Thus it is suitable to do things that involve rebuilding render target related things.

    if (d.applyRateMapPending) {
        d.applyRateMapPending = false;
        if (d.applyRateMapWithImage || d.applyRateMapNative) {
            if (d.textureBased) {
                QRhiTextureRenderTargetDescription desc = d.texRt->description();
                desc.setShadingRateMap(d.rateMap);
                d.texRt->setDescription(desc);
                if (!d.texRtRpWithRateMap) {
                    d.texRtRpWithRateMap = d.texRt->newCompatibleRenderPassDescriptor();
                    d.releasePool << d.texRtRpWithRateMap;
                }
                d.texRt->setRenderPassDescriptor(d.texRtRpWithRateMap);
                d.texRt->create();
                d.ps->setRenderPassDescriptor(d.texRtRpWithRateMap);
                d.ps->create();
            } else {
                m_sc->setShadingRateMap(d.rateMap);
                if (!d.scRpWithRateMap) {
                    d.scRpWithRateMap = m_sc->newCompatibleRenderPassDescriptor();
                    d.releasePool << d.scRpWithRateMap;
                }
                m_sc->setRenderPassDescriptor(d.scRpWithRateMap);
                m_sc->createOrResize();
                d.ps->setRenderPassDescriptor(d.scRpWithRateMap);
                d.ps->create();
            }
        } else {
             if (d.textureBased) {
                QRhiTextureRenderTargetDescription desc = d.texRt->description();
                desc.setShadingRateMap(nullptr);
                d.texRt->setDescription(desc);
                d.texRt->setRenderPassDescriptor(d.texRtRp);
                d.texRt->create();
                d.ps->setRenderPassDescriptor(d.texRtRp);
                d.ps->create();
            } else {
                m_sc->setShadingRateMap(nullptr);
                m_sc->setRenderPassDescriptor(m_rp);
                m_sc->createOrResize();
                d.ps->setRenderPassDescriptor(m_rp);
                d.ps->create();
            }
        }
    }
}

static void renderCube(QRhiCommandBuffer *cb, const QSize &outputSizeInPixels, quint32 ubufAlignedSize)
{
    cb->setGraphicsPipeline(d.ps);
    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    if (d.vrsSupported) {
        int coarsePixelWidth = 1;
        if (d.cps[0] == 1)
            coarsePixelWidth = 2;
        if (d.cps[0] == 2)
            coarsePixelWidth = 4;
        int coarsePixelHeight = 1;
        if (d.cps[1] == 1)
            coarsePixelHeight = 2;
        if (d.cps[1] == 2)
            coarsePixelHeight = 4;
        const QSize shadingRate(coarsePixelWidth, coarsePixelHeight);
        cb->setShadingRate(shadingRate);
    }
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { d.vbuf, 0 },
        { d.vbuf, quint32(36 * 3 * sizeof(float)) }
    };
    cb->setVertexInput(0, 2, vbufBindings);
    for (int i = 0; i < CUBE_COUNT; ++i) {
        QRhiCommandBuffer::DynamicOffset dynOfs(0, i * ubufAlignedSize);
        cb->setShaderResources(d.srb, 1, &dynOfs);
        cb->draw(36);
    }
}

void Window::customRender()
{
    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    float opacity = 1.0f;
    quint32 uoffset = 0;
    for (int i = 0; i < CUBE_COUNT; ++i) {
        QMatrix4x4 mvp = m_proj;
        mvp.translate(d.tx[i], d.ty[i], 0.0f);
        mvp.rotate(d.rotation, 0, 1, 0);
        mvp.scale(d.scale[i], d.scale[i], d.scale[i]);
        u->updateDynamicBuffer(d.ubuf, uoffset, 64, mvp.constData());
        u->updateDynamicBuffer(d.ubuf, uoffset + 64, 4, &opacity);
        uoffset += d.ubufAlignedSize;
    }
    cb->resourceUpdate(u);

    if (d.textureBased) {
        cb->beginPass(d.texRt, Qt::black, { 1.0f, 0 }, nullptr);
        renderCube(cb, d.texRt->pixelSize(), d.ubufAlignedSize);
        cb->endPass();
    }

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 });
    if (!d.textureBased)
        renderCube(cb, m_sc->currentPixelSize(), d.ubufAlignedSize);

    m_imguiRenderer->render();

    cb->endPass();

    d.rotation += 0.1f;
}

void Window::customGui()
{
    ImGui::ShowDemoWindow(&d.showDemoWindow);

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(620, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Variable Rate Shading Test");
    ImGui::Text("Per-draw VRS supported = %s", d.vrsSupported ? "true" : "false");
    ImGui::Text("Map-based VRS supported = %s", d.vrsMapSupported ? "true" : "false");
    ImGui::Text("Map/Image-based VRS supported = %s", d.vrsMapImageSupported ? "true" : "false");
    const int tileSize = m_r->resourceLimit(QRhi::ShadingRateImageTileSize);
    ImGui::Text("VRS image tile size: %dx%d", tileSize, tileSize);

    if (ImGui::TreeNodeEx("Supported rates", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Columns(2, "ratecols");
        ImGui::Separator();
        ImGui::Text("Sample count"); ImGui::NextColumn();
        ImGui::Text("Rates"); ImGui::NextColumn();
        ImGui::Separator();
        for (int sampleCount : { 1, 2, 4, 8, 16 }) {
            ImGui::Text("%d", sampleCount);
            ImGui::NextColumn();
            QString rateStr;
            for (QSize coarsePixelSize : d.supportedShadingRates[sampleCount])
                rateStr += QString::asprintf(" %dx%d", coarsePixelSize.width(), coarsePixelSize.height());
            ImGui::Text("%s", qPrintable(rateStr));
            ImGui::NextColumn();
        }
        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::TreePop();
    }

    ImGui::Text("Sample count: %d", sampleCount);

    const bool wasThisFrameTextureBased = d.textureBased;
    if (ImGui::Checkbox("Render cubes to texture and apply VRS to that", &d.textureBased)) {
        d.applyRateMapPending = true;
        // this imgui callback is made before customRender(), ensure the pipeline
        // and renderpasses are valid; customBeforeFrame() comes only before the next frame.
        if (d.textureBased) {
            d.ps->setRenderPassDescriptor(d.texRtRp);
            d.ps->create();
        } else {
            d.ps->setRenderPassDescriptor(m_rp);
            d.ps->create();
        }
        m_imguiRenderer->registerCustomTexture(d.outTexture, d.outTexture, QRhiSampler::Nearest, QRhiImguiRenderer::NoCustomTextureOwnership);
    }

    if (d.vrsSupported) {
        ImGui::Text("Coarse pixel size");
        ImGui::PushID("cps_width");
        ImGui::Text("Width"); ImGui::SameLine(); ImGui::RadioButton("1", &d.cps[0], 0); ImGui::SameLine(); ImGui::RadioButton("2", &d.cps[0], 1); ImGui::SameLine(); ImGui::RadioButton("4", &d.cps[0], 2);
        ImGui::PopID();
        ImGui::PushID("cps_height");
        ImGui::Text("Height"); ImGui::SameLine(); ImGui::RadioButton("1", &d.cps[1], 0); ImGui::SameLine(); ImGui::RadioButton("2", &d.cps[1], 1); ImGui::SameLine(); ImGui::RadioButton("4", &d.cps[1], 2);
        ImGui::PopID();
    }

    if (d.vrsMapImageSupported) {
        if (ImGui::Checkbox("Apply R8_UINT texture as shading rate image", &d.applyRateMapWithImage)) {
            // We are recording a frame already (between beginFrame..endFrame), it is too
            // late to attempt to change settings that involve recreating the render
            // targets, because the swapchain is involved here. It can only apply from the
            // next frame (the one after this one).
            d.applyRateMapPending = true;
            if (d.applyRateMapWithImage && tileSize > 0) {
                const QSize outputSizeInPixels = d.textureBased ? d.texRt->pixelSize() : m_sc->currentPixelSize();
                if (d.rateMap && d.rateMapTexture->pixelSize() != outputSizeInPixels)
                    d.rateMap = nullptr;
                if (!d.rateMap) {
                    const QSize rateImageSize(qCeil(outputSizeInPixels.width() / (float)tileSize),
                                                qCeil(outputSizeInPixels.height() / (float)tileSize));
                    qDebug() << "Tile size" << tileSize << "Shading rate texture size" << rateImageSize;
                    d.rateMapTexture = m_r->newTexture(QRhiTexture::R8UI, rateImageSize, 1, QRhiTexture::UsedAsShadingRateMap);
                    d.releasePool << d.rateMapTexture;
                    d.rateMapTexture->create();

                    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
                    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
                    // 1x1 in a certain area, but use 4x4 outside
                    QImage img(rateImageSize, QImage::Format_Grayscale8);
                    img.fill(0xA); // 4x4
                    QPainter pnt(&img);
                    // pnt.setPen(QColor::fromRgb(0, 0, 0)); // 1x1
                    // pnt.setBrush(QColor::fromRgb(0, 0, 0));
                    // pnt.drawEllipse(20, 20, rateImageSize.width() - 40, rateImageSize.height() - 40);
                    pnt.fillRect(20, 20, rateImageSize.width() - 40, rateImageSize.height() - 40, QColor::fromRgb(0, 0, 0));
                    pnt.end();
                    u->uploadTexture(d.rateMapTexture, img);
                    cb->resourceUpdate(u);

                    d.rateMap = m_r->newShadingRateMap();
                    d.releasePool << d.rateMap;
                    d.rateMap->createFrom(d.rateMapTexture);

                    d.rateMapTextureForVisualization = m_r->newTexture(QRhiTexture::RGBA8, rateImageSize, 1);
                    d.releasePool << d.rateMapTextureForVisualization;
                    d.rateMapTextureForVisualization->create();
                    QImage rgbaImg = img.convertToFormat(QImage::Format_RGBA8888);
                    u = m_r->nextResourceUpdateBatch();
                    u->uploadTexture(d.rateMapTextureForVisualization, rgbaImg);
                    cb->resourceUpdate(u);
                    m_imguiRenderer->registerCustomTexture(d.rateMapTextureForVisualization, d.rateMapTextureForVisualization, QRhiSampler::Nearest, QRhiImguiRenderer::NoCustomTextureOwnership);
                }
            }
        }
    } else if (d.vrsMapSupported) {
#if QT_CONFIG(metal)
        if (ImGui::Checkbox("Apply a MTLRasterizationRateMap (no scaling, incomplete!)", &d.applyRateMapNative)) {
            d.applyRateMapPending = true;
            const QSize outputSizeInPixels = d.textureBased ? d.texRt->pixelSize() : m_sc->currentPixelSize();
            if (d.applyRateMapNative && d.nativeRateMap && d.nativeRateMapSize != outputSizeInPixels) {
                releaseRateMap(d.nativeRateMap);
                d.nativeRateMap = nullptr;
            }
            if (d.applyRateMapNative && !d.nativeRateMap) {
                d.nativeRateMap = makeRateMap(m_r, outputSizeInPixels);
                d.nativeRateMapSize = outputSizeInPixels;
                d.rateMap = m_r->newShadingRateMap();
                d.releasePool << d.rateMap;
                // rateMap will not own nativeRateMap as per cross-platform docs,
                // but it does actually do a retain/release in the Metal backend.
                // Regardless, we make sure nativeRateMap lives until the end.
                d.rateMap->createFrom({ quint64(d.nativeRateMap) });
            }
        }
#endif
    }

    ImGui::End();

    if (wasThisFrameTextureBased) {
        QSize s = d.outTexture->pixelSize();
        ImGui::SetNextWindowPos(ImVec2(500, 50), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(s.width() / 2, s.height() / 2), ImGuiCond_FirstUseEver);
        ImGui::Begin("Texture", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::Image(d.outTexture, ImVec2(s.width(), s.height()));
        ImGui::End();

        if (d.applyRateMapWithImage && !d.applyRateMapPending) {
            ImGui::SetNextWindowPos(ImVec2(500, 250), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(s.width() / 2, s.height() / 2), ImGuiCond_FirstUseEver);
            ImGui::Begin("Shading rate image", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
            s = d.rateMapTextureForVisualization->pixelSize();
            const int tileSize = m_r->resourceLimit(QRhi::ShadingRateImageTileSize);
            const float alpha = 0.4f;
            ImGui::Image(d.rateMapTextureForVisualization, ImVec2(s.width() * tileSize, s.height() * tileSize), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, alpha));
            ImGui::End();
        }
    }
}
