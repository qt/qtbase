// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/examplefw.h"
#include <QImage>
#include <QPainter>
#include <functional>

static float quadVertexData[] = { // Y up, CCW
    -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  0.5f,  1.0f, 0.0f
};

static quint16 quadIndexData[] = { 0, 1, 2, 0, 2, 3 };

struct
{
    QList<QRhiResource *> releasePool;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;

    QRhiBuffer *vertexBuffer = nullptr;
    QRhiBuffer *indexBuffer = nullptr;
    QRhiBuffer *uniformBuffer = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    int index = 0;
} d;

void readBackCompleted(QRhiReadbackResult *result, const QByteArray &expected)
{
    if (result->data != expected) {
        qFatal("texture readback data did not match expected values");
    }

    delete result;
}

void Window::customInit()
{
    if (!m_r->isFeatureSupported(QRhi::Feature::OneDimensionalTextures))
        qFatal("1D textures are not supported");

    const bool mipmaps = m_r->isFeatureSupported(QRhi::Feature::OneDimensionalTextureMipmaps);

    d.initialUpdates = m_r->nextResourceUpdateBatch();

    QRhiTexture *texture = nullptr;
    QRhiReadbackResult *readbackResult = nullptr;
    QRhiReadbackDescription readbackDescription;
    QByteArray data;

    QList<QRhiShaderResourceBinding> shaderResouceBindings;

    //
    // Create vertex buffer
    //
    d.vertexBuffer =
            m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(quadVertexData));
    d.vertexBuffer->create();
    d.releasePool << d.vertexBuffer;

    d.initialUpdates->uploadStaticBuffer(d.vertexBuffer, 0, sizeof(quadVertexData), quadVertexData);

    //
    // Create index buffer
    //
    d.indexBuffer =
            m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::IndexBuffer, sizeof(quadIndexData));
    d.indexBuffer->create();
    d.releasePool << d.indexBuffer;

    d.initialUpdates->uploadStaticBuffer(d.indexBuffer, quadIndexData);

    //
    // Create uniform buffer
    //
    d.uniformBuffer =
            m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, m_r->ubufAligned(4));
    d.uniformBuffer->create();
    d.releasePool << d.uniformBuffer;

    shaderResouceBindings.append(QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::StageFlag::FragmentStage, d.uniformBuffer, 0, 4));

    //
    // Create samplers
    //
    QRhiSampler *samplerNearest = m_r->newSampler(
            QRhiSampler::Filter::Nearest, QRhiSampler::Filter::Nearest,
            QRhiSampler::Filter::Nearest, QRhiSampler::AddressMode::ClampToEdge,
            QRhiSampler::AddressMode::ClampToEdge, QRhiSampler::AddressMode::ClampToEdge);
    d.releasePool << samplerNearest;
    samplerNearest->create();

    QRhiSampler *samplerNone = m_r->newSampler(
            QRhiSampler::Filter::Nearest, QRhiSampler::Filter::Nearest, QRhiSampler::Filter::None,
            QRhiSampler::AddressMode::ClampToEdge, QRhiSampler::AddressMode::ClampToEdge,
            QRhiSampler::AddressMode::ClampToEdge);
    d.releasePool << samplerNone;
    samplerNone->create();

    //
    // 1D texture with generated mipmaps.  Contains grey colormap
    //
    texture = m_r->newTexture(QRhiTexture::Format::RGBA8, 8, 0, 0, 1,
                              QRhiTexture::Flag::OneDimensional
                                      | QRhiTexture::Flag::UsedAsTransferSource
                                      | (mipmaps ? (QRhiTexture::Flag::MipMapped
                                                    | QRhiTexture::Flag::UsedWithGenerateMips)
                                                 : QRhiTexture::Flag(0)));
    d.releasePool << texture;
    texture->create();

    shaderResouceBindings.append(QRhiShaderResourceBinding::sampledTexture(
            1, QRhiShaderResourceBinding::FragmentStage, texture,
            texture->flags().testFlag(QRhiTexture::Flag::MipMapped) ? samplerNearest
                                                                    : samplerNone));

    for (int i = 0; i < 8; ++i) {
        data.append(char(i) * 32);
        data.append(char(i) * 32);
        data.append(char(i) * 32);
        data.append(char(255));
    }
    d.initialUpdates->uploadTexture(
            texture, QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(data)));
    if (mipmaps)
        d.initialUpdates->generateMips(texture);

    readbackResult = new QRhiReadbackResult;
    readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
    readbackDescription.setTexture(texture);
    readbackDescription.setLayer(0);
    readbackDescription.setLevel(0);
    d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

    //
    // 1D texture array with generated mipmaps. layer 0 contains red colormap, layer 1 contains
    // green colormap
    //
    texture =
            m_r->newTextureArray(QRhiTexture::RGBA8, 2, QSize(8, 0), 1,
                                 QRhiTexture::Flag::TextureArray | QRhiTexture::Flag::OneDimensional
                                         | QRhiTexture::Flag::UsedAsTransferSource
                                         | (mipmaps ? (QRhiTexture::Flag::MipMapped
                                                       | QRhiTexture::Flag::UsedWithGenerateMips)
                                                    : QRhiTexture::Flag(0)));
    d.releasePool << texture;
    texture->create();

    shaderResouceBindings.append(QRhiShaderResourceBinding::sampledTexture(
            2, QRhiShaderResourceBinding::FragmentStage, texture,
            texture->flags().testFlag(QRhiTexture::Flag::MipMapped) ? samplerNearest
                                                                    : samplerNone));

    data.clear();
    for (int i = 0; i < 8; ++i) {
        data.append(char(i * 32));
        data.append(char(0));
        data.append(char(0));
        data.append(char(255));
    }
    d.initialUpdates->uploadTexture(
            texture, QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(data)));

    readbackResult = new QRhiReadbackResult;
    readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
    readbackDescription.setTexture(texture);
    readbackDescription.setLayer(0);
    readbackDescription.setLevel(0);
    d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

    data.clear();
    for (int i = 0; i < 8; ++i) {
        data.append(char(0));
        data.append(char(i * 32));
        data.append(char(0));
        data.append(char(255));
    }
    d.initialUpdates->uploadTexture(
            texture, QRhiTextureUploadEntry(1, 0, QRhiTextureSubresourceUploadDescription(data)));
    if (mipmaps)
        d.initialUpdates->generateMips(texture);

    readbackResult = new QRhiReadbackResult;
    readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
    readbackDescription.setTexture(texture);
    readbackDescription.setLayer(1);
    readbackDescription.setLevel(0);
    d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

    //
    // 1D texture with uploaded mipmaps.  Contains yellow colormap
    //
    texture = m_r->newTexture(
            QRhiTexture::Format::RGBA8, 8, 0, 0, 1,
            QRhiTexture::Flag::OneDimensional | QRhiTexture::Flag::UsedAsTransferSource
                    | (mipmaps ? QRhiTexture::Flag::MipMapped : QRhiTexture::Flag(0)));
    d.releasePool << texture;
    texture->create();

    shaderResouceBindings.append(QRhiShaderResourceBinding::sampledTexture(
            3, QRhiShaderResourceBinding::FragmentStage, texture,
            texture->flags().testFlag(QRhiTexture::Flag::MipMapped) ? samplerNearest
                                                                    : samplerNone));

    data.clear();
    for (int i = 0; i < 8; ++i) {
        data.append(char(i * 32));
        data.append(char(i * 32));
        data.append(char(0));
        data.append(char(255));
    }
    d.initialUpdates->uploadTexture(
            texture, QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(data)));

    readbackResult = new QRhiReadbackResult;
    readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
    readbackDescription.setTexture(texture);
    readbackDescription.setLayer(0);
    readbackDescription.setLevel(0);
    d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

    QRhiTexture *textureSource = texture;
    QByteArray textureCopyData = data.mid(data.size() / 2).append(data.mid(0, data.size() / 2));

    if (mipmaps) {

        data.clear();
        for (int i = 0; i < 4; ++i) {
            data.append(char(i * 64));
            data.append(char(i * 64));
            data.append(char(0));
            data.append(char(255));
        }
        d.initialUpdates->uploadTexture(
                texture,
                QRhiTextureUploadEntry(0, 1, QRhiTextureSubresourceUploadDescription(data)));

        readbackResult = new QRhiReadbackResult;
        readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
        readbackDescription.setTexture(texture);
        readbackDescription.setLayer(0);
        readbackDescription.setLevel(1);
        d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

        data.clear();
        for (int i = 0; i < 2; ++i) {
            data.append(char(i * 128));
            data.append(char(i * 128));
            data.append(char(0));
            data.append(char(255));
        }
        d.initialUpdates->uploadTexture(
                texture,
                QRhiTextureUploadEntry(0, 2, QRhiTextureSubresourceUploadDescription(data)));

        readbackResult = new QRhiReadbackResult;
        readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
        readbackDescription.setTexture(texture);
        readbackDescription.setLayer(0);
        readbackDescription.setLevel(2);
        d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

        data.clear();
        data.append(char(128));
        data.append(char(128));
        data.append(char(0));
        data.append(char(255));
        d.initialUpdates->uploadTexture(
                texture,
                QRhiTextureUploadEntry(0, 3, QRhiTextureSubresourceUploadDescription(data)));

        readbackResult = new QRhiReadbackResult;
        readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
        readbackDescription.setTexture(texture);
        readbackDescription.setLayer(0);
        readbackDescription.setLevel(3);
        d.initialUpdates->readBackTexture(readbackDescription, readbackResult);
    }

    //
    // 1D texture array with uploaded mipmaps. Layer 0 contains blue colormap, layer 1 contains
    // magenta colormap
    //
    texture = m_r->newTextureArray(
            QRhiTexture::Format::RGBA8, 2, QSize(8, 0), 1,
            QRhiTexture::Flag::TextureArray | QRhiTexture::Flag::OneDimensional
                    | QRhiTexture::Flag::UsedAsTransferSource
                    | (mipmaps ? QRhiTexture::Flag::MipMapped : QRhiTexture::Flag(0)));
    d.releasePool << texture;
    texture->create();

    shaderResouceBindings.append(QRhiShaderResourceBinding::sampledTexture(
            4, QRhiShaderResourceBinding::FragmentStage, texture,
            texture->flags().testFlag(QRhiTexture::Flag::MipMapped) ? samplerNearest
                                                                    : samplerNone));

    data.clear();
    for (int i = 0; i < 8; ++i) {
        data.append(char(0));
        data.append(char(0));
        data.append(char(i * 32));
        data.append(char(255));
    }
    d.initialUpdates->uploadTexture(
            texture, QRhiTextureUploadEntry(0, 0, QRhiTextureSubresourceUploadDescription(data)));

    readbackResult = new QRhiReadbackResult;
    readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
    readbackDescription.setTexture(texture);
    readbackDescription.setLayer(0);
    readbackDescription.setLevel(0);
    d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

    QRhiTexture *textureArraySource = texture;
    QList<QByteArray> textureArrayCopyData;
    textureArrayCopyData.append(data.mid(data.size() / 2).append(data.mid(0, data.size() / 2)));

    if (mipmaps) {

        data.clear();
        for (int i = 0; i < 4; ++i) {
            data.append(char(0));
            data.append(char(0));
            data.append(char(i * 64));
            data.append(char(255));
        }
        d.initialUpdates->uploadTexture(
                texture,
                QRhiTextureUploadEntry(0, 1, QRhiTextureSubresourceUploadDescription(data)));

        readbackResult = new QRhiReadbackResult;
        readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
        readbackDescription.setTexture(texture);
        readbackDescription.setLayer(0);
        readbackDescription.setLevel(1);
        d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

        data.clear();
        for (int i = 0; i < 2; ++i) {
            data.append(char(0));
            data.append(char(0));
            data.append(char(i * 128));
            data.append(char(255));
        }
        d.initialUpdates->uploadTexture(
                texture,
                QRhiTextureUploadEntry(0, 2, QRhiTextureSubresourceUploadDescription(data)));

        readbackResult = new QRhiReadbackResult;
        readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
        readbackDescription.setTexture(texture);
        readbackDescription.setLayer(0);
        readbackDescription.setLevel(2);
        d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

        data.clear();
        data.append(char(0));
        data.append(char(0));
        data.append(char(128));
        data.append(char(255));
        d.initialUpdates->uploadTexture(
                texture,
                QRhiTextureUploadEntry(0, 3, QRhiTextureSubresourceUploadDescription(data)));

        readbackResult = new QRhiReadbackResult;
        readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
        readbackDescription.setTexture(texture);
        readbackDescription.setLayer(0);
        readbackDescription.setLevel(3);
        d.initialUpdates->readBackTexture(readbackDescription, readbackResult);
    }

    data.clear();
    for (int i = 0; i < 8; ++i) {
        data.append(char(i * 32));
        data.append(char(0));
        data.append(char(i * 32));
        data.append(char(255));
    }
    d.initialUpdates->uploadTexture(
            texture, QRhiTextureUploadEntry(1, 0, QRhiTextureSubresourceUploadDescription(data)));

    readbackResult = new QRhiReadbackResult;
    readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
    readbackDescription.setTexture(texture);
    readbackDescription.setLayer(1);
    readbackDescription.setLevel(0);
    d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

    textureArrayCopyData.append(data.mid(data.size() / 2).append(data.mid(0, data.size() / 2)));

    if (mipmaps) {

        data.clear();
        for (int i = 0; i < 4; ++i) {
            data.append(char(i * 64));
            data.append(char(0));
            data.append(char(i * 64));
            data.append(char(255));
        }
        d.initialUpdates->uploadTexture(
                texture,
                QRhiTextureUploadEntry(1, 1, QRhiTextureSubresourceUploadDescription(data)));

        readbackResult = new QRhiReadbackResult;
        readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
        readbackDescription.setTexture(texture);
        readbackDescription.setLayer(1);
        readbackDescription.setLevel(1);
        d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

        data.clear();
        for (int i = 0; i < 2; ++i) {
            data.append(char(i * 128));
            data.append(char(0));
            data.append(char(i * 128));
            data.append(char(255));
        }
        d.initialUpdates->uploadTexture(
                texture,
                QRhiTextureUploadEntry(1, 2, QRhiTextureSubresourceUploadDescription(data)));

        readbackResult = new QRhiReadbackResult;
        readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
        readbackDescription.setTexture(texture);
        readbackDescription.setLayer(1);
        readbackDescription.setLevel(2);
        d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

        data.clear();
        data.append(char(128));
        data.append(char(0));
        data.append(char(128));
        data.append(char(255));
        d.initialUpdates->uploadTexture(
                texture,
                QRhiTextureUploadEntry(1, 3, QRhiTextureSubresourceUploadDescription(data)));

        readbackResult = new QRhiReadbackResult;
        readbackResult->completed = std::bind(readBackCompleted, readbackResult, data);
        readbackDescription.setTexture(texture);
        readbackDescription.setLayer(1);
        readbackDescription.setLevel(3);
        d.initialUpdates->readBackTexture(readbackDescription, readbackResult);
    }

    //
    // 1D Texture loaded from image - contains rainbow colormap
    //
    QImage image(256, 1, QImage::Format_RGBA8888);
    for (int i = 0; i < image.width(); ++i) {
        float x = float(i) / float(image.width() - 1);
        if (x < 2.0f / 5.0f) {
            image.setPixelColor(i, 0, QColor::fromRgbF(1.0f, 5.0f / 2.0f * x, 0.0f));
        } else if (x < 3.0f / 5.0f) {
            image.setPixelColor(i, 0, QColor::fromRgbF(-5.0f * x + 3.0f, 1.0f, 0.0f));
        } else if (x < 4.0f / 5.0f) {
            image.setPixelColor(i, 0, QColor::fromRgbF(0.0f, -5.0f * x + 4.0f, 5.0f * x - 3.0f));
        } else {
            image.setPixelColor(i, 0, QColor::fromRgbF(10.0f / 3.0f * x - 8.0f / 3.0f, 0.0f, 1.0f));
        }
    }

    texture = m_r->newTexture(QRhiTexture::Format::RGBA8, image.width(), 0, 0, 1,
                              QRhiTexture::Flag::OneDimensional);
    d.releasePool << texture;
    texture->create();

    shaderResouceBindings.append(QRhiShaderResourceBinding::sampledTexture(
            5, QRhiShaderResourceBinding::FragmentStage, texture,
            texture->flags().testFlag(QRhiTexture::Flag::MipMapped) ? samplerNearest
                                                                    : samplerNone));

    d.initialUpdates->uploadTexture(texture, image);

    //
    // 1D Texture copied
    //
    texture = m_r->newTexture(QRhiTexture::Format::RGBA8, 8, 0, 0, 1,
                              QRhiTexture::Flag::OneDimensional
                                      | QRhiTexture::Flag::UsedAsTransferSource);
    d.releasePool << texture;
    texture->create();

    shaderResouceBindings.append(QRhiShaderResourceBinding::sampledTexture(
            6, QRhiShaderResourceBinding::FragmentStage, texture,
            texture->flags().testFlag(QRhiTexture::Flag::MipMapped) ? samplerNearest
                                                                    : samplerNone));

    QRhiTextureCopyDescription copyDescription;
    copyDescription.setSourceLayer(0);
    copyDescription.setSourceLevel(0);
    copyDescription.setSourceTopLeft(QPoint(4, 0));
    copyDescription.setDestinationLayer(0);
    copyDescription.setDestinationLevel(0);
    copyDescription.setDestinationTopLeft(QPoint(0, 0));
    copyDescription.setPixelSize(QSize(4, 1));

    d.initialUpdates->copyTexture(texture, textureSource, copyDescription);

    copyDescription.setSourceTopLeft(QPoint(0, 0));
    copyDescription.setDestinationTopLeft(QPoint(4, 0));

    d.initialUpdates->copyTexture(texture, textureSource, copyDescription);

    readbackResult = new QRhiReadbackResult;
    readbackResult->completed = std::bind(readBackCompleted, readbackResult, textureCopyData);
    readbackDescription.setTexture(texture);
    readbackDescription.setLayer(0);
    readbackDescription.setLevel(0);
    d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

    //
    // 1D Texture array copied
    //
    texture = m_r->newTextureArray(QRhiTexture::Format::RGBA8, 2, QSize(8, 0), 1,
                                   QRhiTexture::Flag::OneDimensional
                                           | QRhiTexture::Flag::UsedAsTransferSource);
    d.releasePool << texture;
    texture->create();

    shaderResouceBindings.append(QRhiShaderResourceBinding::sampledTexture(
            7, QRhiShaderResourceBinding::FragmentStage, texture,
            texture->flags().testFlag(QRhiTexture::Flag::MipMapped) ? samplerNearest
                                                                    : samplerNone));

    copyDescription.setSourceLayer(1);
    copyDescription.setSourceLevel(0);
    copyDescription.setSourceTopLeft(QPoint(4, 0));
    copyDescription.setDestinationLayer(0);
    copyDescription.setDestinationLevel(0);
    copyDescription.setDestinationTopLeft(QPoint(0, 0));
    copyDescription.setPixelSize(QSize(4, 1));
    d.initialUpdates->copyTexture(texture, textureArraySource, copyDescription);

    copyDescription.setSourceTopLeft(QPoint(0, 0));
    copyDescription.setDestinationTopLeft(QPoint(4, 0));
    d.initialUpdates->copyTexture(texture, textureArraySource, copyDescription);

    copyDescription.setSourceLayer(0);
    copyDescription.setDestinationLayer(1);
    d.initialUpdates->copyTexture(texture, textureArraySource, copyDescription);

    copyDescription.setSourceTopLeft(QPoint(4, 0));
    copyDescription.setDestinationTopLeft(QPoint(0, 0));
    d.initialUpdates->copyTexture(texture, textureArraySource, copyDescription);

    readbackResult = new QRhiReadbackResult;
    readbackResult->completed =
            std::bind(readBackCompleted, readbackResult, textureArrayCopyData[0]);
    readbackDescription.setTexture(texture);
    readbackDescription.setLayer(1);
    readbackDescription.setLevel(0);
    d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

    readbackResult = new QRhiReadbackResult;
    readbackResult->completed =
            std::bind(readBackCompleted, readbackResult, textureArrayCopyData[1]);
    readbackDescription.setTexture(texture);
    readbackDescription.setLayer(0);
    readbackDescription.setLevel(0);
    d.initialUpdates->readBackTexture(readbackDescription, readbackResult);

    //
    // Shader resource bindings
    //
    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;
    d.srb->setBindings(shaderResouceBindings.cbegin(), shaderResouceBindings.cend());
    d.srb->create();

    //
    // Pipeline
    //
    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;
    d.ps->setShaderStages(
            { { QRhiShaderStage::Vertex, getShader(QLatin1String(":/texture1d.vert.qsb")) },
              { QRhiShaderStage::Fragment, getShader(QLatin1String(":/texture1d.frag.qsb")) } });

    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 4 * sizeof(float) } });
    inputLayout.setAttributes(
            { { 0, 0, QRhiVertexInputAttribute::Float2, 0 },
              { 0, 1, QRhiVertexInputAttribute::Float2, quint32(2 * sizeof(float)) } });

    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);
    d.ps->create();
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
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

    u->updateDynamicBuffer(d.uniformBuffer, 0, 4, &d.index);
    d.index++;

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);
    cb->setGraphicsPipeline(d.ps);
    cb->setShaderResources(d.srb);
    const QSize outputSizeInPixels = m_sc->currentPixelSize();
    cb->setViewport(
            { 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    QRhiCommandBuffer::VertexInput vbufBinding(d.vertexBuffer, 0);
    cb->setVertexInput(0, 1, &vbufBinding, d.indexBuffer, 0, QRhiCommandBuffer::IndexUInt16);
    cb->drawIndexed(6);
    cb->endPass();
}
