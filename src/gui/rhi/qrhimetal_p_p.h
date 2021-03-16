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

#ifndef QRHIMETAL_P_H
#define QRHIMETAL_P_H

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

#include "qrhimetal_p.h"
#include "qrhi_p_p.h"
#include <QWindow>

QT_BEGIN_NAMESPACE

static const int QMTL_FRAMES_IN_FLIGHT = 2;

// have to hide the ObjC stuff, this header cannot contain MTL* at all
struct QMetalBufferData;

struct QMetalBuffer : public QRhiBuffer
{
    QMetalBuffer(QRhiImplementation *rhi, Type type, UsageFlags usage, int size);
    ~QMetalBuffer();
    void destroy() override;
    bool create() override;
    QRhiBuffer::NativeBuffer nativeBuffer() override;
    char *beginFullDynamicBufferUpdateForCurrentFrame() override;
    void endFullDynamicBufferUpdateForCurrentFrame() override;

    QMetalBufferData *d;
    uint generation = 0;
    int lastActiveFrameSlot = -1;
    friend class QRhiMetal;
    friend struct QMetalShaderResourceBindings;
};

struct QMetalRenderBufferData;

struct QMetalRenderBuffer : public QRhiRenderBuffer
{
    QMetalRenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                       int sampleCount, QRhiRenderBuffer::Flags flags,
                       QRhiTexture::Format backingFormatHint);
    ~QMetalRenderBuffer();
    void destroy() override;
    bool create() override;
    QRhiTexture::Format backingFormat() const override;

    QMetalRenderBufferData *d;
    int samples = 1;
    uint generation = 0;
    int lastActiveFrameSlot = -1;
    friend class QRhiMetal;
};

struct QMetalTextureData;

struct QMetalTexture : public QRhiTexture
{
    QMetalTexture(QRhiImplementation *rhi, Format format, const QSize &pixelSize,
                  int sampleCount, Flags flags);
    ~QMetalTexture();
    void destroy() override;
    bool create() override;
    bool createFrom(NativeTexture src) override;
    NativeTexture nativeTexture() override;

    bool prepareCreate(QSize *adjustedSize = nullptr);

    QMetalTextureData *d;
    int mipLevelCount = 0;
    int samples = 1;
    uint generation = 0;
    int lastActiveFrameSlot = -1;
    friend class QRhiMetal;
    friend struct QMetalShaderResourceBindings;
    friend struct QMetalTextureData;
};

struct QMetalSamplerData;

struct QMetalSampler : public QRhiSampler
{
    QMetalSampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                  AddressMode u, AddressMode v, AddressMode w);
    ~QMetalSampler();
    void destroy() override;
    bool create() override;

    QMetalSamplerData *d;
    uint generation = 0;
    int lastActiveFrameSlot = -1;
    friend class QRhiMetal;
    friend struct QMetalShaderResourceBindings;
};

struct QMetalRenderPassDescriptor : public QRhiRenderPassDescriptor
{
    QMetalRenderPassDescriptor(QRhiImplementation *rhi);
    ~QMetalRenderPassDescriptor();
    void destroy() override;
    bool isCompatible(const QRhiRenderPassDescriptor *other) const override;
    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() const override;

    // there is no MTLRenderPassDescriptor here as one will be created for each pass in beginPass()

    // but the things needed for the render pipeline descriptor have to be provided
    static const int MAX_COLOR_ATTACHMENTS = 8;
    int colorAttachmentCount = 0;
    bool hasDepthStencil = false;
    int colorFormat[MAX_COLOR_ATTACHMENTS];
    int dsFormat;
};

struct QMetalRenderTargetData;

struct QMetalReferenceRenderTarget : public QRhiRenderTarget
{
    QMetalReferenceRenderTarget(QRhiImplementation *rhi);
    ~QMetalReferenceRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QMetalRenderTargetData *d;
};

struct QMetalTextureRenderTarget : public QRhiTextureRenderTarget
{
    QMetalTextureRenderTarget(QRhiImplementation *rhi, const QRhiTextureRenderTargetDescription &desc, Flags flags);
    ~QMetalTextureRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool create() override;

    QMetalRenderTargetData *d;
    friend class QRhiMetal;
};

struct QMetalShaderResourceBindings : public QRhiShaderResourceBindings
{
    QMetalShaderResourceBindings(QRhiImplementation *rhi);
    ~QMetalShaderResourceBindings();
    void destroy() override;
    bool create() override;

    QVarLengthArray<QRhiShaderResourceBinding, 8> sortedBindings;
    int maxBinding = -1;

    struct BoundUniformBufferData {
        quint64 id;
        uint generation;
    };
    struct BoundSampledTextureData {
        int count;
        struct {
            quint64 texId;
            uint texGeneration;
            quint64 samplerId;
            uint samplerGeneration;
        } d[QRhiShaderResourceBinding::Data::MAX_TEX_SAMPLER_ARRAY_SIZE];
    };
    struct BoundStorageImageData {
        quint64 id;
        uint generation;
    };
    struct BoundStorageBufferData {
        quint64 id;
        uint generation;
    };
    struct BoundResourceData {
        union {
            BoundUniformBufferData ubuf;
            BoundSampledTextureData stex;
            BoundStorageImageData simage;
            BoundStorageBufferData sbuf;
        };
    };
    QVarLengthArray<BoundResourceData, 8> boundResourceData;

    uint generation = 0;
    friend class QRhiMetal;
};

struct QMetalGraphicsPipelineData;

struct QMetalGraphicsPipeline : public QRhiGraphicsPipeline
{
    QMetalGraphicsPipeline(QRhiImplementation *rhi);
    ~QMetalGraphicsPipeline();
    void destroy() override;
    bool create() override;

    QMetalGraphicsPipelineData *d;
    uint generation = 0;
    int lastActiveFrameSlot = -1;
    friend class QRhiMetal;
};

struct QMetalComputePipelineData;

struct QMetalComputePipeline : public QRhiComputePipeline
{
    QMetalComputePipeline(QRhiImplementation *rhi);
    ~QMetalComputePipeline();
    void destroy() override;
    bool create() override;

    QMetalComputePipelineData *d;
    uint generation = 0;
    int lastActiveFrameSlot = -1;
    friend class QRhiMetal;
};

struct QMetalCommandBufferData;
struct QMetalSwapChain;

struct QMetalCommandBuffer : public QRhiCommandBuffer
{
    QMetalCommandBuffer(QRhiImplementation *rhi);
    ~QMetalCommandBuffer();
    void destroy() override;

    QMetalCommandBufferData *d = nullptr;
    QRhiMetalCommandBufferNativeHandles nativeHandlesStruct;

    enum PassType {
        NoPass,
        RenderPass,
        ComputePass
    };

    // per-pass (render or compute command encoder) persistent state
    PassType recordingPass;
    QRhiRenderTarget *currentTarget;

    // per-pass (render or compute command encoder) volatile (cached) state
    QRhiGraphicsPipeline *currentGraphicsPipeline;
    QRhiComputePipeline *currentComputePipeline;
    uint currentPipelineGeneration;
    QRhiShaderResourceBindings *currentGraphicsSrb;
    QRhiShaderResourceBindings *currentComputeSrb;
    uint currentSrbGeneration;
    int currentResSlot;
    QRhiBuffer *currentIndexBuffer;
    quint32 currentIndexOffset;
    QRhiCommandBuffer::IndexFormat currentIndexFormat;
    int currentCullMode;
    int currentFrontFaceWinding;
    QPair<float, float> currentDepthBiasValues;

    const QRhiNativeHandles *nativeHandles();
    void resetState();
    void resetPerPassState();
    void resetPerPassCachedState();
};

struct QMetalSwapChainData;

struct QMetalSwapChain : public QRhiSwapChain
{
    QMetalSwapChain(QRhiImplementation *rhi);
    ~QMetalSwapChain();
    void destroy() override;

    QRhiCommandBuffer *currentFrameCommandBuffer() override;
    QRhiRenderTarget *currentFrameRenderTarget() override;
    QSize surfacePixelSize() override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;

    bool createOrResize() override;

    void chooseFormats();

    QWindow *window = nullptr;
    QSize pixelSize;
    int currentFrameSlot = 0; // 0..QMTL_FRAMES_IN_FLIGHT-1
    int frameCount = 0;
    int samples = 1;
    QMetalReferenceRenderTarget rtWrapper;
    QMetalCommandBuffer cbWrapper;
    QMetalRenderBuffer *ds = nullptr;
    QMetalSwapChainData *d = nullptr;
};

struct QRhiMetalData;

class QRhiMetal : public QRhiImplementation
{
public:
    QRhiMetal(QRhiMetalInitParams *params, QRhiMetalNativeHandles *importDevice = nullptr);
    ~QRhiMetal();

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

    void executeDeferredReleases(bool forced = false);
    void finishActiveReadbacks(bool forced = false);
    qsizetype subresUploadByteSize(const QRhiTextureSubresourceUploadDescription &subresDesc) const;
    void enqueueSubresUpload(QMetalTexture *texD, void *mp, void *blitEncPtr,
                             int layer, int level, const QRhiTextureSubresourceUploadDescription &subresDesc,
                             qsizetype *curOfs);
    void enqueueResourceUpdates(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates);
    void executeBufferHostWritesForSlot(QMetalBuffer *bufD, int slot);
    void executeBufferHostWritesForCurrentFrame(QMetalBuffer *bufD);
    static const int SUPPORTED_STAGES = 3;
    void enqueueShaderResourceBindings(QMetalShaderResourceBindings *srbD,
                                       QMetalCommandBuffer *cbD,
                                       int dynamicOffsetCount,
                                       const QRhiCommandBuffer::DynamicOffset *dynamicOffsets,
                                       bool offsetOnlyChange,
                                       const QShader::NativeResourceBindingMap *nativeResourceBindingMaps[SUPPORTED_STAGES]);
    int effectiveSampleCount(int sampleCount) const;

    bool importedDevice = false;
    bool importedCmdQueue = false;
    QMetalSwapChain *currentSwapChain = nullptr;
    QSet<QMetalSwapChain *> swapchains;
    QRhiMetalNativeHandles nativeHandlesStruct;
    QRhiDriverInfo driverInfoStruct;

    struct {
        int maxTextureSize = 4096;
    } caps;

    QRhiMetalData *d = nullptr;
};

QT_END_NAMESPACE

#endif
