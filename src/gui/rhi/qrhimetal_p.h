// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include "qrhi_p.h"
#include <QWindow>

QT_BEGIN_NAMESPACE

static const int QMTL_FRAMES_IN_FLIGHT = 2;

// have to hide the ObjC stuff, this header cannot contain MTL* at all
struct QMetalBufferData;

struct QMetalBuffer : public QRhiBuffer
{
    QMetalBuffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size);
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

    static constexpr int WorkBufPoolUsage = 1 << 8;
    static_assert(WorkBufPoolUsage > QRhiBuffer::StorageBuffer);
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
    QMetalTexture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
                  int arraySize, int sampleCount, Flags flags);
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
    QVector<quint32> serializedFormat() const override;

    void updateSerializedFormat();

    // there is no MTLRenderPassDescriptor here as one will be created for each pass in beginPass()

    // but the things needed for the render pipeline descriptor have to be provided
    static const int MAX_COLOR_ATTACHMENTS = 8;
    int colorAttachmentCount = 0;
    bool hasDepthStencil = false;
    int colorFormat[MAX_COLOR_ATTACHMENTS];
    int dsFormat;
    QVector<quint32> serializedFormatData;
};

struct QMetalRenderTargetData;

struct QMetalSwapChainRenderTarget : public QRhiSwapChainRenderTarget
{
    QMetalSwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain);
    ~QMetalSwapChainRenderTarget();
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
    void updateResources(UpdateFlags flags) override;

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
struct QMetalCommandBuffer;

struct QMetalGraphicsPipeline : public QRhiGraphicsPipeline
{
    QMetalGraphicsPipeline(QRhiImplementation *rhi);
    ~QMetalGraphicsPipeline();
    void destroy() override;
    bool create() override;

    void makeActiveForCurrentRenderPassEncoder(QMetalCommandBuffer *cbD);
    void setupAttachmentsInMetalRenderPassDescriptor(void *metalRpDesc, QMetalRenderPassDescriptor *rpD);
    void setupMetalDepthStencilDescriptor(void *metalDsDesc);
    void mapStates();
    bool createVertexFragmentPipeline();
    bool createTessellationPipelines(const QShader &tessVert, const QShader &tesc, const QShader &tese, const QShader &tessFrag);

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
    QMetalGraphicsPipeline *currentGraphicsPipeline;
    QMetalComputePipeline *currentComputePipeline;
    uint currentPipelineGeneration;
    QMetalShaderResourceBindings *currentGraphicsSrb;
    QMetalShaderResourceBindings *currentComputeSrb;
    uint currentSrbGeneration;
    int currentResSlot;
    QMetalBuffer *currentIndexBuffer;
    quint32 currentIndexOffset;
    QRhiCommandBuffer::IndexFormat currentIndexFormat;
    int currentCullMode;
    int currentTriangleFillMode;
    int currentFrontFaceWinding;
    QPair<float, float> currentDepthBiasValues;

    const QRhiNativeHandles *nativeHandles();
    void resetState(double lastGpuTime = 0);
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
    bool isFormatSupported(Format f) override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;

    bool createOrResize() override;

    virtual QRhiSwapChainHdrInfo hdrInfo() override;

    void chooseFormats();
    void waitUntilCompleted(int slot);

    QWindow *window = nullptr;
    QSize pixelSize;
    int currentFrameSlot = 0; // 0..QMTL_FRAMES_IN_FLIGHT-1
    int frameCount = 0;
    int samples = 1;
    QMetalSwapChainRenderTarget rtWrapper;
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

    static bool probe(QRhiMetalInitParams *params);
    static QRhiSwapChainProxyData updateSwapChainProxyData(QWindow *window);

    bool create(QRhi::Flags flags) override;
    void destroy() override;

    QRhiGraphicsPipeline *createGraphicsPipeline() override;
    QRhiComputePipeline *createComputePipeline() override;
    QRhiShaderResourceBindings *createShaderResourceBindings() override;
    QRhiBuffer *createBuffer(QRhiBuffer::Type type,
                             QRhiBuffer::UsageFlags usage,
                             quint32 size) override;
    QRhiRenderBuffer *createRenderBuffer(QRhiRenderBuffer::Type type,
                                         const QSize &pixelSize,
                                         int sampleCount,
                                         QRhiRenderBuffer::Flags flags,
                                         QRhiTexture::Format backingFormatHint) override;
    QRhiTexture *createTexture(QRhiTexture::Format format,
                               const QSize &pixelSize,
                               int depth,
                               int arraySize,
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
    double lastCompletedGpuTime(QRhiCommandBuffer *cb) override;

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
    QRhiStats statistics() override;
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
    static const int SUPPORTED_STAGES = 5;
    void enqueueShaderResourceBindings(QMetalShaderResourceBindings *srbD,
                                       QMetalCommandBuffer *cbD,
                                       int dynamicOffsetCount,
                                       const QRhiCommandBuffer::DynamicOffset *dynamicOffsets,
                                       bool offsetOnlyChange,
                                       const QShader::NativeResourceBindingMap *nativeResourceBindingMaps[SUPPORTED_STAGES]);
    struct TessDrawArgs {
        QMetalCommandBuffer *cbD;
        enum {
            NonIndexed,
            U16Indexed,
            U32Indexed
        } type;
        struct NonIndexedArgs {
            quint32 vertexCount;
            quint32 instanceCount;
            quint32 firstVertex;
            quint32 firstInstance;
        };
        struct IndexedArgs {
            quint32 indexCount;
            quint32 instanceCount;
            quint32 firstIndex;
            qint32 vertexOffset;
            quint32 firstInstance;
            void *indexBuffer;
        };
        union {
            NonIndexedArgs draw;
            IndexedArgs drawIndexed;
        };
    };
    void tessellatedDraw(const TessDrawArgs &args);
    void adjustForMultiViewDraw(quint32 *instanceCount, QRhiCommandBuffer *cb);

    QRhi::Flags rhiFlags;
    bool importedDevice = false;
    bool importedCmdQueue = false;
    QMetalSwapChain *currentSwapChain = nullptr;
    QSet<QMetalSwapChain *> swapchains;
    QRhiMetalNativeHandles nativeHandlesStruct;
    QRhiDriverInfo driverInfoStruct;
    quint32 osMajor = 0;
    quint32 osMinor = 0;

    struct {
        int maxTextureSize = 4096;
        bool baseVertexAndInstance = true;
        QVector<int> supportedSampleCounts;
        bool isAppleGPU = false;
        int maxThreadGroupSize = 512;
        bool multiView = false;
    } caps;

    QRhiMetalData *d = nullptr;
};

QT_END_NAMESPACE

#endif
