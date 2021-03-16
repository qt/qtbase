/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QRHINULL_P_H
#define QRHINULL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qrhinull_p.h"
#include "qrhi_p_p.h"

QT_BEGIN_NAMESPACE

struct QNullBuffer : public QRhiBuffer
{
    QNullBuffer(QRhiImplementation *rhi, Type type, UsageFlags usage, int size);
    ~QNullBuffer();
    void destroy() override;
    bool create() override;
    char *beginFullDynamicBufferUpdateForCurrentFrame() override;

    char *data = nullptr;
};

struct QNullRenderBuffer : public QRhiRenderBuffer
{
    QNullRenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                      int sampleCount, QRhiRenderBuffer::Flags flags,
                      QRhiTexture::Format backingFormatHint);
    ~QNullRenderBuffer();
    void destroy() override;
    bool create() override;
    QRhiTexture::Format backingFormat() const override;
};

struct QNullTexture : public QRhiTexture
{
    QNullTexture(QRhiImplementation *rhi, Format format, const QSize &pixelSize,
                  int sampleCount, Flags flags);
    ~QNullTexture();
    void destroy() override;
    bool create() override;
    bool createFrom(NativeTexture src) override;

    QImage image[QRhi::MAX_LAYERS][QRhi::MAX_LEVELS];
};

struct QNullSampler : public QRhiSampler
{
    QNullSampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                 AddressMode u, AddressMode v, AddressMode w);
    ~QNullSampler();
    void destroy() override;
    bool create() override;
};

struct QNullRenderPassDescriptor : public QRhiRenderPassDescriptor
{
    QNullRenderPassDescriptor(QRhiImplementation *rhi);
    ~QNullRenderPassDescriptor();
    void destroy() override;
    bool isCompatible(const QRhiRenderPassDescriptor *other) const override;
    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() const override;
};

struct QNullRenderTargetData
{
    QNullRenderTargetData(QRhiImplementation *) { }

    QNullRenderPassDescriptor *rp = nullptr;
    QSize pixelSize;
    float dpr = 1;
};

struct QNullReferenceRenderTarget : public QRhiRenderTarget
{
    QNullReferenceRenderTarget(QRhiImplementation *rhi);
    ~QNullReferenceRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QNullRenderTargetData d;
};

struct QNullTextureRenderTarget : public QRhiTextureRenderTarget
{
    QNullTextureRenderTarget(QRhiImplementation *rhi, const QRhiTextureRenderTargetDescription &desc, Flags flags);
    ~QNullTextureRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool create() override;

    QNullRenderTargetData d;
};

struct QNullShaderResourceBindings : public QRhiShaderResourceBindings
{
    QNullShaderResourceBindings(QRhiImplementation *rhi);
    ~QNullShaderResourceBindings();
    void destroy() override;
    bool create() override;
};

struct QNullGraphicsPipeline : public QRhiGraphicsPipeline
{
    QNullGraphicsPipeline(QRhiImplementation *rhi);
    ~QNullGraphicsPipeline();
    void destroy() override;
    bool create() override;
};

struct QNullComputePipeline : public QRhiComputePipeline
{
    QNullComputePipeline(QRhiImplementation *rhi);
    ~QNullComputePipeline();
    void destroy() override;
    bool create() override;
};

struct QNullCommandBuffer : public QRhiCommandBuffer
{
    QNullCommandBuffer(QRhiImplementation *rhi);
    ~QNullCommandBuffer();
    void destroy() override;
};

struct QNullSwapChain : public QRhiSwapChain
{
    QNullSwapChain(QRhiImplementation *rhi);
    ~QNullSwapChain();
    void destroy() override;

    QRhiCommandBuffer *currentFrameCommandBuffer() override;
    QRhiRenderTarget *currentFrameRenderTarget() override;

    QSize surfacePixelSize() override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool createOrResize() override;

    QNullReferenceRenderTarget rt;
    QNullCommandBuffer cb;
    int frameCount = 0;
};

class QRhiNull : public QRhiImplementation
{
public:
    QRhiNull(QRhiNullInitParams *params);

    bool create(QRhi::Flags flags) override;
    void destroy() override;

    QRhiGraphicsPipeline *createGraphicsPipeline() override;
    QRhiComputePipeline *createComputePipeline() override;
    QRhiShaderResourceBindings *createShaderResourceBindings() override;
    QRhiBuffer *createBuffer(QRhiBuffer::Type type,
                             QRhiBuffer::UsageFlags usage,
                             int size) override;
    QRhiRenderBuffer *createRenderBuffer(QRhiRenderBuffer::Type type,
                                         const QSize &pixelSize,
                                         int sampleCount,
                                         QRhiRenderBuffer::Flags flags,
                                         QRhiTexture::Format backingFormatHint) override;
    QRhiTexture *createTexture(QRhiTexture::Format format,
                               const QSize &pixelSize,
                               int sampleCount,
                               QRhiTexture::Flags flags) override;
    QRhiSampler *createSampler(QRhiSampler::Filter magFilter,
                               QRhiSampler::Filter minFilter,
                               QRhiSampler::Filter mipmapMode,
                               QRhiSampler:: AddressMode u,
                               QRhiSampler::AddressMode v,
                               QRhiSampler::AddressMode w) override;

    QRhiTextureRenderTarget *createTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                       QRhiTextureRenderTarget::Flags flags) override;

    QRhiSwapChain *createSwapChain() override;
    QRhi::FrameOpResult beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags) override;
    QRhi::FrameOpResult endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags) override;
    QRhi::FrameOpResult beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags) override;
    QRhi::FrameOpResult endOffscreenFrame(QRhi::EndFrameFlags flags) override;
    QRhi::FrameOpResult finish() override;

    void resourceUpdate(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) override;

    void beginPass(QRhiCommandBuffer *cb,
                   QRhiRenderTarget *rt,
                   const QColor &colorClearValue,
                   const QRhiDepthStencilClearValue &depthStencilClearValue,
                   QRhiResourceUpdateBatch *resourceUpdates,
                   QRhiCommandBuffer::BeginPassFlags flags) override;
    void endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) override;

    void setGraphicsPipeline(QRhiCommandBuffer *cb,
                             QRhiGraphicsPipeline *ps) override;

    void setShaderResources(QRhiCommandBuffer *cb,
                            QRhiShaderResourceBindings *srb,
                            int dynamicOffsetCount,
                            const QRhiCommandBuffer::DynamicOffset *dynamicOffsets) override;

    void setVertexInput(QRhiCommandBuffer *cb,
                        int startBinding, int bindingCount, const QRhiCommandBuffer::VertexInput *bindings,
                        QRhiBuffer *indexBuf, quint32 indexOffset,
                        QRhiCommandBuffer::IndexFormat indexFormat) override;

    void setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport) override;
    void setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor) override;
    void setBlendConstants(QRhiCommandBuffer *cb, const QColor &c) override;
    void setStencilRef(QRhiCommandBuffer *cb, quint32 refValue) override;

    void draw(QRhiCommandBuffer *cb, quint32 vertexCount,
              quint32 instanceCount, quint32 firstVertex, quint32 firstInstance) override;

    void drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                     quint32 instanceCount, quint32 firstIndex,
                     qint32 vertexOffset, quint32 firstInstance) override;

    void debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name) override;
    void debugMarkEnd(QRhiCommandBuffer *cb) override;
    void debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg) override;

    void beginComputePass(QRhiCommandBuffer *cb,
                          QRhiResourceUpdateBatch *resourceUpdates,
                          QRhiCommandBuffer::BeginPassFlags flags) override;
    void endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates) override;
    void setComputePipeline(QRhiCommandBuffer *cb, QRhiComputePipeline *ps) override;
    void dispatch(QRhiCommandBuffer *cb, int x, int y, int z) override;

    const QRhiNativeHandles *nativeHandles(QRhiCommandBuffer *cb) override;
    void beginExternal(QRhiCommandBuffer *cb) override;
    void endExternal(QRhiCommandBuffer *cb) override;

    QList<int> supportedSampleCounts() const override;
    int ubufAlignment() const override;
    bool isYUpInFramebuffer() const override;
    bool isYUpInNDC() const override;
    bool isClipDepthZeroToOne() const override;
    QMatrix4x4 clipSpaceCorrMatrix() const override;
    bool isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const override;
    bool isFeatureSupported(QRhi::Feature feature) const override;
    int resourceLimit(QRhi::ResourceLimit limit) const override;
    const QRhiNativeHandles *nativeHandles() override;
    QRhiDriverInfo driverInfo() const override;
    void sendVMemStatsToProfiler() override;
    bool makeThreadLocalNativeContextCurrent() override;
    void releaseCachedResources() override;
    bool isDeviceLost() const override;

    QByteArray pipelineCacheData() override;
    void setPipelineCacheData(const QByteArray &data) override;

    void simulateTextureUpload(const QRhiResourceUpdateBatchPrivate::TextureOp &u);
    void simulateTextureCopy(const QRhiResourceUpdateBatchPrivate::TextureOp &u);
    void simulateTextureGenMips(const QRhiResourceUpdateBatchPrivate::TextureOp &u);

    QRhiNullNativeHandles nativeHandlesStruct;
    QRhiSwapChain *currentSwapChain = nullptr;
    QNullCommandBuffer offscreenCommandBuffer;
};

QT_END_NAMESPACE

#endif
