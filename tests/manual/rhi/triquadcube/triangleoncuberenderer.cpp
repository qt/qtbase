// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "triangleoncuberenderer.h"
#include <QFile>
#include <rhi/qshader.h>

// toggle to test the preserved content (no clear) path
const bool IMAGE_UNDER_OFFSCREEN_RENDERING = false;
const bool UPLOAD_UNDERLAY_ON_EVERY_FRAME = false;

const bool DS_ATT = false; // have a depth-stencil attachment for the offscreen pass

const bool DEPTH_TEXTURE = false; // offscreen pass uses a depth texture (verify with renderdoc etc., ignore valid.layer about ps slot 0)
const bool MRT = false; // two textures, the second is just cleared as the shader does not write anything (valid.layer may warn but for testing that's ok)

#include "../shared/cube.h"

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    return QShader();
}

static const QSize OFFSCREEN_SIZE(512, 512);

void TriangleOnCubeRenderer::initResources(QRhiRenderPassDescriptor *rp)
{
    m_vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    m_vbuf->setName(QByteArrayLiteral("Cube vbuf (textured with offscreen)"));
    m_vbuf->create();
    m_vbufReady = false;

    m_ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 64 + 4);
    m_ubuf->setName(QByteArrayLiteral("Cube ubuf (textured with offscreen)"));
    m_ubuf->create();

    if (IMAGE_UNDER_OFFSCREEN_RENDERING) {
        m_image = QImage(QLatin1String(":/qt256.png")).scaled(OFFSCREEN_SIZE).convertToFormat(QImage::Format_RGBA8888);
        if (m_r->isYUpInFramebuffer())
            m_image.flip(); // just cause we'll flip texcoord Y when y up so accommodate our static background image as well
    }

    m_tex = m_r->newTexture(QRhiTexture::RGBA8, OFFSCREEN_SIZE, 1, QRhiTexture::RenderTarget);
    m_tex->setName(QByteArrayLiteral("Texture for offscreen content"));
    m_tex->create();

    if (MRT) {
        m_tex2 = m_r->newTexture(QRhiTexture::RGBA8, OFFSCREEN_SIZE, 1, QRhiTexture::RenderTarget);
        m_tex2->create();
    }

    if (DS_ATT) {
        m_offscreenTriangle.setDepthWrite(true);
        m_ds = m_r->newRenderBuffer(QRhiRenderBuffer::DepthStencil, m_tex->pixelSize());
        m_ds->create();
    }

    if (DEPTH_TEXTURE) {
        m_offscreenTriangle.setDepthWrite(true);
        m_depthTex = m_r->newTexture(QRhiTexture::D32F, OFFSCREEN_SIZE, 1, QRhiTexture::RenderTarget);
        m_depthTex->create();
    }

    m_sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None, QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    m_sampler->create();

    m_srb = m_r->newShaderResourceBindings();
    m_srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_ubuf),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, m_tex, m_sampler)
    });
    m_srb->create();

    m_ps = m_r->newGraphicsPipeline();

    m_ps->setDepthTest(true);
    m_ps->setDepthWrite(true);
    m_ps->setDepthOp(QRhiGraphicsPipeline::Less);

    m_ps->setCullMode(QRhiGraphicsPipeline::Back);
    m_ps->setFrontFace(QRhiGraphicsPipeline::CCW);

    m_ps->setSampleCount(m_sampleCount);

    QShader vs = getShader(QLatin1String(":/texture.vert.qsb"));
    Q_ASSERT(vs.isValid());
    QShader fs = getShader(QLatin1String(":/texture.frag.qsb"));
    Q_ASSERT(fs.isValid());
    m_ps->setShaderStages({
        { QRhiShaderStage::Vertex, vs },
        { QRhiShaderStage::Fragment, fs }
    });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) },
        { 2 * sizeof(float) }
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 1, 1, QRhiVertexInputAttribute::Float2, 0 }
    });

    m_ps->setVertexInputLayout(inputLayout);
    m_ps->setShaderResourceBindings(m_srb);
    m_ps->setRenderPassDescriptor(rp);

    m_ps->create();

    QRhiTextureRenderTarget::Flags rtFlags;
    if (IMAGE_UNDER_OFFSCREEN_RENDERING)
        rtFlags |= QRhiTextureRenderTarget::PreserveColorContents;

    if (DEPTH_TEXTURE) {
        QRhiTextureRenderTargetDescription desc;
        desc.setDepthTexture(m_depthTex);
        m_rt = m_r->newTextureRenderTarget(desc, rtFlags);
    } else {
        QRhiTextureRenderTargetDescription desc;
        QRhiColorAttachment color0 { m_tex };
        if (DS_ATT)
            desc.setDepthStencilBuffer(m_ds);
        if (MRT) {
            m_offscreenTriangle.setColorAttCount(2);
            QRhiColorAttachment color1 { m_tex2 };
            desc.setColorAttachments({ color0, color1 });
        } else {
            desc.setColorAttachments({ color0 });
        }
        m_rt = m_r->newTextureRenderTarget(desc, rtFlags);
    }

    m_rp = m_rt->newCompatibleRenderPassDescriptor();
    m_rt->setRenderPassDescriptor(m_rp);

    m_rt->create();

    m_offscreenTriangle.setRhi(m_r);
    m_offscreenTriangle.initResources(m_rp);
    m_offscreenTriangle.setScale(2);
    // m_tex and the offscreen triangle are never multisample
}

void TriangleOnCubeRenderer::resize(const QSize &pixelSize)
{
    m_proj = m_r->clipSpaceCorrMatrix();
    m_proj.perspective(45.0f, pixelSize.width() / (float) pixelSize.height(), 0.01f, 100.0f);
    m_proj.translate(0, 0, -4);

    m_offscreenTriangle.resize(pixelSize);
}

void TriangleOnCubeRenderer::releaseResources()
{
    m_offscreenTriangle.releaseResources();

    delete m_ps;
    m_ps = nullptr;

    delete m_srb;
    m_srb = nullptr;

    delete m_rt;
    m_rt = nullptr;

    delete m_rp;
    m_rp = nullptr;

    delete m_sampler;
    m_sampler = nullptr;

    delete m_depthTex;
    m_depthTex = nullptr;

    delete m_tex2;
    m_tex2 = nullptr;

    delete m_tex;
    m_tex = nullptr;

    delete m_ds;
    m_ds = nullptr;

    delete m_ubuf;
    m_ubuf = nullptr;

    delete m_vbuf;
    m_vbuf = nullptr;
}

void TriangleOnCubeRenderer::queueResourceUpdates(QRhiResourceUpdateBatch *resourceUpdates)
{
    if (!m_vbufReady) {
        m_vbufReady = true;
        resourceUpdates->uploadStaticBuffer(m_vbuf, cube);
        qint32 flip = m_r->isYUpInFramebuffer() ? 1 : 0;
        resourceUpdates->updateDynamicBuffer(m_ubuf, 64, 4, &flip);
    }

    m_rotation += 1.0f;
    QMatrix4x4 mvp = m_proj;
    mvp.translate(m_translation);
    mvp.scale(0.5f);
    mvp.rotate(m_rotation, 1, 0, 0);
    resourceUpdates->updateDynamicBuffer(m_ubuf, 0, 64, mvp.constData());

    // ###
//    if (DEPTH_TEXTURE) {
//        // m_tex is basically undefined here, be nice and transition the layout properly at least
//        resourceUpdates->prepareTextureForUse(m_tex, QRhiResourceUpdateBatch::TextureRead);
//    }
}

void TriangleOnCubeRenderer::queueOffscreenPass(QRhiCommandBuffer *cb)
{
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();
    m_offscreenTriangle.queueResourceUpdates(u);

    if (IMAGE_UNDER_OFFSCREEN_RENDERING && !m_image.isNull()) {
        u->uploadTexture(m_tex, m_image);
        if (!UPLOAD_UNDERLAY_ON_EVERY_FRAME)
            m_image = QImage();
    }

    cb->beginPass(m_rt, QColor::fromRgbF(0.0f, 0.4f, 0.7f, 1.0f), { 1.0f, 0 }, u);
    m_offscreenTriangle.queueDraw(cb, OFFSCREEN_SIZE);
    cb->endPass();
}

void TriangleOnCubeRenderer::queueDraw(QRhiCommandBuffer *cb, const QSize &outputSizeInPixels)
{
    cb->setGraphicsPipeline(m_ps);
    cb->setViewport(QRhiViewport(0, 0, outputSizeInPixels.width(), outputSizeInPixels.height()));
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { m_vbuf, 0 },
        { m_vbuf, quint32(36 * 3 * sizeof(float)) }
    };
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(36);
}
