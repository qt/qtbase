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

#include "../shared/examplefw.h"
#include "../shared/cube.h"
#include <QPainter>

struct {
    QRhiBuffer *vbuf = nullptr;
    QRhiBuffer *ubuf = nullptr;
    QRhiTexture *tex = nullptr;
    QRhiSampler *sampler = nullptr;
    QRhiShaderResourceBindings *srb = nullptr;
    QRhiGraphicsPipeline *ps = nullptr;
    QVector<QRhiResource *> releasePool;

    float rotation = 0;
    QRhiResourceUpdateBatch *initialUpdates = nullptr;
    int frameCount = 0;
    QImage customImage;
    QRhiTexture *newTex = nullptr;
    QRhiTexture *importedTex = nullptr;
    int testStage = 0;

    QRhiShaderResourceBinding bindings[2];
} d;

void Window::customInit()
{
    d.vbuf = m_r->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(cube));
    d.releasePool << d.vbuf;
    d.vbuf->build();

    d.ubuf = m_r->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, 68);
    d.releasePool << d.ubuf;
    d.ubuf->build();

    QImage baseImage(QLatin1String(":/qt256.png"));
    d.tex = m_r->newTexture(QRhiTexture::RGBA8, baseImage.size(), 1, QRhiTexture::UsedAsTransferSource);
    d.releasePool << d.tex;
    d.tex->build();

    // As an alternative to what some of the other examples do, prepare an
    // update batch right here instead of relying on vbufReady and similar flags.
    d.initialUpdates = m_r->nextResourceUpdateBatch();
    d.initialUpdates->uploadStaticBuffer(d.vbuf, cube);
    qint32 flip = 0;
    d.initialUpdates->updateDynamicBuffer(d.ubuf, 64, 4, &flip);
    d.initialUpdates->uploadTexture(d.tex, baseImage);

    d.sampler = m_r->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
    d.releasePool << d.sampler;
    d.sampler->build();

    d.srb = m_r->newShaderResourceBindings();
    d.releasePool << d.srb;

    d.bindings[0] = QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf);
    d.bindings[1] = QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.tex, d.sampler);
    d.srb->setBindings(d.bindings, d.bindings + 2);
    d.srb->build();

    d.ps = m_r->newGraphicsPipeline();
    d.releasePool << d.ps;

    d.ps->setDepthTest(true);
    d.ps->setDepthWrite(true);
    d.ps->setDepthOp(QRhiGraphicsPipeline::Less);

    d.ps->setCullMode(QRhiGraphicsPipeline::Back);
    d.ps->setFrontFace(QRhiGraphicsPipeline::CCW);

    const QShader vs = getShader(QLatin1String(":/texture.vert.qsb"));
    if (!vs.isValid())
        qFatal("Failed to load shader pack (vertex)");
    const QShader fs = getShader(QLatin1String(":/texture.frag.qsb"));
    if (!fs.isValid())
        qFatal("Failed to load shader pack (fragment)");

    d.ps->setShaderStages({
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

    d.ps->setVertexInputLayout(inputLayout);
    d.ps->setShaderResourceBindings(d.srb);
    d.ps->setRenderPassDescriptor(m_rp);

    d.ps->build();

    d.customImage = QImage(128, 64, QImage::Format_RGBA8888);
    d.customImage.fill(Qt::red);
    QPainter painter(&d.customImage);
    // the text may look different on different platforms, so no guarantee the
    // output on the screen will be identical everywhere
    painter.drawText(5, 25, "Hello world");
    painter.end();
}

void Window::customRelease()
{
    qDeleteAll(d.releasePool);
    d.releasePool.clear();
}

void Window::customRender()
{
    QRhiResourceUpdateBatch *u = m_r->nextResourceUpdateBatch();

    // take the initial set of updates, if this is the first frame
    if (d.initialUpdates) {
        u->merge(d.initialUpdates);
        d.initialUpdates->release();
        d.initialUpdates = nullptr;
    }

    d.rotation += 1.0f;
    QMatrix4x4 mvp = m_proj;
    mvp.scale(0.5f);
    mvp.rotate(d.rotation, 0, 1, 0);
    u->updateDynamicBuffer(d.ubuf, 0, 64, mvp.constData());

    if (d.frameCount > 0 && (d.frameCount % 100) == 0) {
        d.testStage += 1;
        qDebug("testStage = %d", d.testStage);

        // Partially change the texture.
        if (d.testStage == 1) {
            QRhiTextureSubresourceUploadDescription mipDesc(d.customImage);
            // The image here is smaller than the original. Use a non-zero position
            // to make it more interesting.
            mipDesc.setDestinationTopLeft(QPoint(100, 20));
            QRhiTextureUploadDescription desc({ 0, 0, mipDesc });
            u->uploadTexture(d.tex, desc);
        }

        // Exercise image copying.
        if (d.testStage == 2) {
            const QSize sz = d.tex->pixelSize();
            d.newTex = m_r->newTexture(QRhiTexture::RGBA8, sz);
            d.releasePool << d.newTex;
            d.newTex->build();

            QImage empty(sz.width(), sz.height(), QImage::Format_RGBA8888);
            empty.fill(Qt::blue);
            u->uploadTexture(d.newTex, empty);

            // Copy the left-half of tex to the right-half of newTex, while
            // leaving the left-half of newTex blue. Keep a 20 pixel gap at
            // the top.
            QRhiTextureCopyDescription desc;
            desc.setSourceTopLeft(QPoint(0, 20));
            desc.setPixelSize(QSize(sz.width() / 2, sz.height() - 20));
            desc.setDestinationTopLeft(QPoint(sz.width() / 2, 20));
            u->copyTexture(d.newTex, d.tex, desc);

            // Now replace d.tex with d.newTex as the shader resource.
            d.bindings[1] = QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.newTex, d.sampler);
            d.srb->setBindings(d.bindings, d.bindings + 2);
            // "rebuild", whatever that means for a given backend. This srb is
            // already live as the ps in the setGraphicsPipeline references it,
            // but that's fine. Changes will be picked up automatically.
            d.srb->build();
        }

        // Exercise simple, full texture copy.
        if (d.testStage == 4)
            u->copyTexture(d.newTex, d.tex);

        // Now again upload customImage but this time only a part of it.
        if (d.testStage == 5) {
            QRhiTextureSubresourceUploadDescription mipDesc(d.customImage);
            mipDesc.setDestinationTopLeft(QPoint(10, 120));
            mipDesc.setSourceSize(QSize(50, 40));
            mipDesc.setSourceTopLeft(QPoint(20, 10));
            QRhiTextureUploadDescription desc({ 0, 0, mipDesc });
            u->uploadTexture(d.newTex, desc);
        }

        // Exercise texture object export/import.
        if (d.testStage == 6) {
            const QRhiTexture::NativeTexture nativeTexture = d.tex->nativeTexture();
            if (nativeTexture.object) {
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
                if (graphicsApi == Metal) {
                    qDebug() << "Metal texture: " << *(void**)nativeTexture.object;
                    // Now could cast to id<MTLTexture> and do something with
                    // it, keeping in mind that copy operations are only done
                    // in beginPass, while rendering into a texture may only
                    // have proper results in current_frame + 2, or after a
                    // finish(). The QRhiTexture still owns the native object.
                }
#endif
                // omit for other backends, the idea is the same

                d.importedTex = m_r->newTexture(QRhiTexture::RGBA8, d.tex->pixelSize());
                d.releasePool << d.importedTex;
                if (!d.importedTex->buildFrom(nativeTexture))
                    qWarning("Texture import failed");

                // now d.tex and d.importedTex use the same MTLTexture
                // underneath (owned by d.tex)

                // switch to showing d.importedTex
                d.bindings[1] = QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.importedTex, d.sampler);
                d.srb->setBindings(d.bindings, d.bindings + 2);
                d.srb->build();
            } else {
                qWarning("Accessing native texture object is not supported");
            }
        }

        // Exercise uploading uncompressed data without a QImage.
        if (d.testStage == 7) {
            d.bindings[1] = QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, d.newTex, d.sampler);
            d.srb->setBindings(d.bindings, d.bindings + 2);
            d.srb->build();

            const QSize sz(221, 139);
            QByteArray data;
            data.resize(sz.width() * sz.height() * 4);
            for (int y = 0; y < sz.height(); ++y) {
                uchar *p = reinterpret_cast<uchar *>(data.data()) + y * sz.width() * 4;
                for (int x = 0; x < sz.width(); ++x) {
                    *p++ = 0; *p++ = 0; *p++ = y * (255 / sz.height());
                    *p++ = 255;
                }
            }
            QRhiTextureSubresourceUploadDescription mipDesc(data.constData(), data.size());
            mipDesc.setSourceSize(sz);
            mipDesc.setDestinationTopLeft(QPoint(5, 25));
            QRhiTextureUploadDescription desc({ 0, 0, mipDesc });
            u->uploadTexture(d.newTex, desc);
        }
    }

    QRhiCommandBuffer *cb = m_sc->currentFrameCommandBuffer();
    const QSize outputSizeInPixels = m_sc->currentPixelSize();

    cb->beginPass(m_sc->currentFrameRenderTarget(), m_clearColor, { 1.0f, 0 }, u);

    cb->setGraphicsPipeline(d.ps);
    cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
    cb->setShaderResources();
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { d.vbuf, 0 },
        { d.vbuf, 36 * 3 * sizeof(float) }
    };
    cb->setVertexInput(0, 2, vbufBindings);
    cb->draw(36);

    cb->endPass();

    d.frameCount += 1;
}
