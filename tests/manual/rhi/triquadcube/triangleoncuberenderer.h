// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TRIANGLEONCUBERENDERER_H
#define TRIANGLEONCUBERENDERER_H

#include "trianglerenderer.h"

class TriangleOnCubeRenderer
{
public:
    void setRhi(QRhi *r) { m_r = r; }
    void setSampleCount(int samples) { m_sampleCount = samples; }
    void setTranslation(const QVector3D &v) { m_translation = v; }
    void initResources(QRhiRenderPassDescriptor *rp);
    void releaseResources();
    void resize(const QSize &pixelSize);
    void queueResourceUpdates(QRhiResourceUpdateBatch *resourceUpdates);
    void queueOffscreenPass(QRhiCommandBuffer *cb);
    void queueDraw(QRhiCommandBuffer *cb, const QSize &outputSizeInPixels);

private:
    QRhi *m_r;

    QRhiBuffer *m_vbuf = nullptr;
    bool m_vbufReady = false;
    QRhiBuffer *m_ubuf = nullptr;
    QRhiTexture *m_tex = nullptr;
    QRhiRenderBuffer *m_ds = nullptr;
    QRhiTexture *m_tex2 = nullptr;
    QRhiTexture *m_depthTex = nullptr;
    QRhiSampler *m_sampler = nullptr;
    QRhiTextureRenderTarget *m_rt = nullptr;
    QRhiRenderPassDescriptor *m_rp = nullptr;
    QRhiShaderResourceBindings *m_srb = nullptr;
    QRhiGraphicsPipeline *m_ps = nullptr;

    QVector3D m_translation;
    QMatrix4x4 m_proj;
    float m_rotation = 0;
    int m_sampleCount = 1; // no MSAA by default

    TriangleRenderer m_offscreenTriangle;

    QImage m_image;
};

#endif
