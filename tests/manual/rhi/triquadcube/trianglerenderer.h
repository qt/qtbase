// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TRIANGLERENDERER_H
#define TRIANGLERENDERER_H

#include <rhi/qrhi.h>

class TriangleRenderer
{
public:
    void setRhi(QRhi *r) { m_r = r; }
    void setSampleCount(int samples) { m_sampleCount = samples; }
    int sampleCount() const { return m_sampleCount; }
    void setTranslation(const QVector3D &v) { m_translation = v; }
    void setScale(float f) { m_scale = f; }
    void setDepthWrite(bool enable) { m_depthWrite = enable; }
    void setColorAttCount(int count) { m_colorAttCount = count; }
    QRhiGraphicsPipeline *pipeline() const { return m_ps; }
    void initResources(QRhiRenderPassDescriptor *rp);
    void releaseResources();
    void resize(const QSize &pixelSize);
    void queueResourceUpdates(QRhiResourceUpdateBatch *resourceUpdates);
    void queueDraw(QRhiCommandBuffer *cb, const QSize &outputSizeInPixels);

private:
    QRhi *m_r;

    QRhiBuffer *m_vbuf = nullptr;
    bool m_vbufReady = false;
    QRhiBuffer *m_ubuf = nullptr;
    QRhiShaderResourceBindings *m_srb = nullptr;
    QRhiGraphicsPipeline *m_ps = nullptr;

    QVector3D m_translation;
    float m_scale = 1;
    bool m_depthWrite = false;
    int m_colorAttCount = 1;
    QMatrix4x4 m_proj;
    float m_rotation = 0;
    float m_opacity = 1;
    int m_opacityDir = -1;
    int m_sampleCount = 1; // no MSAA by default
};

#endif
