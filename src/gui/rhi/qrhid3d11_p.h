// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHID3D11_P_H
#define QRHID3D11_P_H

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
#include <rhi/qshaderdescription.h>
#include <QWindow>

#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <dcomp.h>

QT_BEGIN_NAMESPACE

class QRhiD3D11;

struct QD3D11Buffer : public QRhiBuffer
{
    QD3D11Buffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size);
    ~QD3D11Buffer();
    void destroy() override;
    bool create() override;
    QRhiBuffer::NativeBuffer nativeBuffer() override;
    char *beginFullDynamicBufferUpdateForCurrentFrame() override;
    void endFullDynamicBufferUpdateForCurrentFrame() override;

    ID3D11UnorderedAccessView *unorderedAccessView(quint32 offset);

    ID3D11Buffer *buffer = nullptr;
    char *dynBuf = nullptr;
    bool hasPendingDynamicUpdates = false;
    QHash<quint32, ID3D11UnorderedAccessView *> uavs;
    uint generation = 0;
    friend class QRhiD3D11;
};

struct QD3D11RenderBuffer : public QRhiRenderBuffer
{
    QD3D11RenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                       int sampleCount, QRhiRenderBuffer::Flags flags,
                       QRhiTexture::Format backingFormatHint);
    ~QD3D11RenderBuffer();
    void destroy() override;
    bool create() override;
    QRhiTexture::Format backingFormat() const override;

    ID3D11Texture2D *tex = nullptr;
    ID3D11DepthStencilView *dsv = nullptr;
    ID3D11RenderTargetView *rtv = nullptr;
    DXGI_FORMAT dxgiFormat;
    DXGI_SAMPLE_DESC sampleDesc;
    uint generation = 0;
    friend class QRhiD3D11;
};

struct QD3D11Texture : public QRhiTexture
{
    QD3D11Texture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
                  int arraySize, int sampleCount, Flags flags);
    ~QD3D11Texture();
    void destroy() override;
    bool create() override;
    bool createFrom(NativeTexture src) override;
    NativeTexture nativeTexture() override;

    bool prepareCreate(QSize *adjustedSize = nullptr);
    bool finishCreate();
    ID3D11UnorderedAccessView *unorderedAccessViewForLevel(int level);
    ID3D11Resource *textureResource() const
    {
        if (tex)
            return tex;
        else if (tex1D)
            return tex1D;
        return tex3D;
    }

    ID3D11Texture2D *tex = nullptr;
    ID3D11Texture3D *tex3D = nullptr;
    ID3D11Texture1D *tex1D = nullptr;
    bool owns = true;
    ID3D11ShaderResourceView *srv = nullptr;
    DXGI_FORMAT dxgiFormat;
    uint mipLevelCount = 0;
    DXGI_SAMPLE_DESC sampleDesc;
    ID3D11UnorderedAccessView *perLevelViews[QRhi::MAX_MIP_LEVELS];
    uint generation = 0;
    friend class QRhiD3D11;
};

struct QD3D11Sampler : public QRhiSampler
{
    QD3D11Sampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                  AddressMode u, AddressMode v, AddressMode w);
    ~QD3D11Sampler();
    void destroy() override;
    bool create() override;

    ID3D11SamplerState *samplerState = nullptr;
    uint generation = 0;
    friend class QRhiD3D11;
};

struct QD3D11RenderPassDescriptor : public QRhiRenderPassDescriptor
{
    QD3D11RenderPassDescriptor(QRhiImplementation *rhi);
    ~QD3D11RenderPassDescriptor();
    void destroy() override;
    bool isCompatible(const QRhiRenderPassDescriptor *other) const override;
    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() const override;
    QVector<quint32> serializedFormat() const override;
};

struct QD3D11RenderTargetData
{
    QD3D11RenderTargetData(QRhiImplementation *)
    {
        for (int i = 0; i < MAX_COLOR_ATTACHMENTS; ++i)
            rtv[i] = nullptr;
    }

    QD3D11RenderPassDescriptor *rp = nullptr;
    QSize pixelSize;
    float dpr = 1;
    int sampleCount = 1;
    int colorAttCount = 0;
    int dsAttCount = 0;

    static const int MAX_COLOR_ATTACHMENTS = 8;
    ID3D11RenderTargetView *rtv[MAX_COLOR_ATTACHMENTS];
    ID3D11DepthStencilView *dsv = nullptr;

    QRhiRenderTargetAttachmentTracker::ResIdList currentResIdList;
};

struct QD3D11SwapChainRenderTarget : public QRhiSwapChainRenderTarget
{
    QD3D11SwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain);
    ~QD3D11SwapChainRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QD3D11RenderTargetData d;
};

struct QD3D11TextureRenderTarget : public QRhiTextureRenderTarget
{
    QD3D11TextureRenderTarget(QRhiImplementation *rhi, const QRhiTextureRenderTargetDescription &desc, Flags flags);
    ~QD3D11TextureRenderTarget();
    void destroy() override;

    QSize pixelSize() const override;
    float devicePixelRatio() const override;
    int sampleCount() const override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool create() override;

    QD3D11RenderTargetData d;
    bool ownsRtv[QD3D11RenderTargetData::MAX_COLOR_ATTACHMENTS];
    ID3D11RenderTargetView *rtv[QD3D11RenderTargetData::MAX_COLOR_ATTACHMENTS];
    bool ownsDsv = false;
    ID3D11DepthStencilView *dsv = nullptr;
    friend class QRhiD3D11;
};

struct QD3D11ShaderResourceBindings : public QRhiShaderResourceBindings
{
    QD3D11ShaderResourceBindings(QRhiImplementation *rhi);
    ~QD3D11ShaderResourceBindings();
    void destroy() override;
    bool create() override;
    void updateResources(UpdateFlags flags) override;

    bool hasDynamicOffset = false;
    QVarLengthArray<QRhiShaderResourceBinding, 8> sortedBindings;
    uint generation = 0;

    // Keep track of the generation number of each referenced QRhi* to be able
    // to detect that the batched bindings are out of date.
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

    struct StageUniformBufferBatches {
        bool present = false;
        QRhiBatchedBindings<ID3D11Buffer *> ubufs;
        QRhiBatchedBindings<UINT> ubuforigbindings;
        QRhiBatchedBindings<UINT> ubufoffsets;
        QRhiBatchedBindings<UINT> ubufsizes;
        void finish() {
            present = ubufs.finish();
            ubuforigbindings.finish();
            ubufoffsets.finish();
            ubufsizes.finish();
        }
        void clear() {
            ubufs.clear();
            ubuforigbindings.clear();
            ubufoffsets.clear();
            ubufsizes.clear();
        }
    };

    struct StageSamplerBatches {
        bool present = false;
        QRhiBatchedBindings<ID3D11SamplerState *> samplers;
        QRhiBatchedBindings<ID3D11ShaderResourceView *> shaderresources;
        void finish() {
            present = samplers.finish();
            shaderresources.finish();
        }
        void clear() {
            samplers.clear();
            shaderresources.clear();
        }
    };

    struct StageUavBatches {
        bool present = false;
        QRhiBatchedBindings<ID3D11UnorderedAccessView *> uavs;
        void finish() {
            present = uavs.finish();
        }
        void clear() {
            uavs.clear();
        }
    };

    StageUniformBufferBatches vsUniformBufferBatches;
    StageUniformBufferBatches hsUniformBufferBatches;
    StageUniformBufferBatches dsUniformBufferBatches;
    StageUniformBufferBatches gsUniformBufferBatches;
    StageUniformBufferBatches fsUniformBufferBatches;
    StageUniformBufferBatches csUniformBufferBatches;

    StageSamplerBatches vsSamplerBatches;
    StageSamplerBatches hsSamplerBatches;
    StageSamplerBatches dsSamplerBatches;
    StageSamplerBatches gsSamplerBatches;
    StageSamplerBatches fsSamplerBatches;
    StageSamplerBatches csSamplerBatches;

    StageUavBatches csUavBatches;

    friend class QRhiD3D11;
};

Q_DECLARE_TYPEINFO(QD3D11ShaderResourceBindings::BoundResourceData, Q_RELOCATABLE_TYPE);

struct QD3D11GraphicsPipeline : public QRhiGraphicsPipeline
{
    QD3D11GraphicsPipeline(QRhiImplementation *rhi);
    ~QD3D11GraphicsPipeline();
    void destroy() override;
    bool create() override;

    ID3D11DepthStencilState *dsState = nullptr;
    ID3D11BlendState *blendState = nullptr;
    struct {
        ID3D11VertexShader *shader = nullptr;
        QShader::NativeResourceBindingMap nativeResourceBindingMap;
    } vs;
    struct {
        ID3D11HullShader *shader = nullptr;
        QShader::NativeResourceBindingMap nativeResourceBindingMap;
    } hs;
    struct {
        ID3D11DomainShader *shader = nullptr;
        QShader::NativeResourceBindingMap nativeResourceBindingMap;
    } ds;
    struct {
        ID3D11GeometryShader *shader = nullptr;
        QShader::NativeResourceBindingMap nativeResourceBindingMap;
    } gs;
    struct {
        ID3D11PixelShader *shader = nullptr;
        QShader::NativeResourceBindingMap nativeResourceBindingMap;
    } fs;
    ID3D11InputLayout *inputLayout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    ID3D11RasterizerState *rastState = nullptr;
    uint generation = 0;
    friend class QRhiD3D11;
};

struct QD3D11ComputePipeline : public QRhiComputePipeline
{
    QD3D11ComputePipeline(QRhiImplementation *rhi);
    ~QD3D11ComputePipeline();
    void destroy() override;
    bool create() override;

    struct {
        ID3D11ComputeShader *shader = nullptr;
        QShader::NativeResourceBindingMap nativeResourceBindingMap;
    } cs;
    uint generation = 0;
    friend class QRhiD3D11;
};

struct QD3D11SwapChain;

struct QD3D11CommandBuffer : public QRhiCommandBuffer
{
    QD3D11CommandBuffer(QRhiImplementation *rhi);
    ~QD3D11CommandBuffer();
    void destroy() override;

    // these must be kept at a reasonably low value otherwise sizeof Command explodes
    static const int MAX_DYNAMIC_OFFSET_COUNT = 8;
    static const int MAX_VERTEX_BUFFER_BINDING_COUNT = 8;

    struct Command {
        enum Cmd {
            BeginFrame,
            EndFrame,
            ResetShaderResources,
            SetRenderTarget,
            Clear,
            Viewport,
            Scissor,
            BindVertexBuffers,
            BindIndexBuffer,
            BindGraphicsPipeline,
            BindShaderResources,
            StencilRef,
            BlendConstants,
            Draw,
            DrawIndexed,
            UpdateSubRes,
            CopySubRes,
            ResolveSubRes,
            GenMip,
            DebugMarkBegin,
            DebugMarkEnd,
            DebugMarkMsg,
            BindComputePipeline,
            Dispatch
        };
        enum ClearFlag { Color = 1, Depth = 2, Stencil = 4 };
        Cmd cmd;

        // QRhi*/QD3D11* references should be kept at minimum (so no
        // QRhiTexture/Buffer/etc. pointers).
        union Args {
            struct {
                ID3D11Query *tsQuery;
                ID3D11Query *tsDisjointQuery;
                QD3D11RenderTargetData *swapchainData;
            } beginFrame;
            struct {
                ID3D11Query *tsQuery;
                ID3D11Query *tsDisjointQuery;
            } endFrame;
            struct {
                QRhiRenderTarget *rt;
            } setRenderTarget;
            struct {
                QRhiRenderTarget *rt;
                int mask;
                float c[4];
                float d;
                quint32 s;
            } clear;
            struct {
                float x, y, w, h;
                float d0, d1;
            } viewport;
            struct {
                int x, y, w, h;
            } scissor;
            struct {
                int startSlot;
                int slotCount;
                ID3D11Buffer *buffers[MAX_VERTEX_BUFFER_BINDING_COUNT];
                UINT offsets[MAX_VERTEX_BUFFER_BINDING_COUNT];
                UINT strides[MAX_VERTEX_BUFFER_BINDING_COUNT];
            } bindVertexBuffers;
            struct {
                ID3D11Buffer *buffer;
                quint32 offset;
                DXGI_FORMAT format;
            } bindIndexBuffer;
            struct {
                QD3D11GraphicsPipeline *ps;
            } bindGraphicsPipeline;
            struct {
                QD3D11ShaderResourceBindings *srb;
                bool offsetOnlyChange;
                int dynamicOffsetCount;
                uint dynamicOffsetPairs[MAX_DYNAMIC_OFFSET_COUNT * 2]; // binding, offsetInConstants
            } bindShaderResources;
            struct {
                QD3D11GraphicsPipeline *ps;
                quint32 ref;
            } stencilRef;
            struct {
                QD3D11GraphicsPipeline *ps;
                float c[4];
            } blendConstants;
            struct {
                QD3D11GraphicsPipeline *ps;
                quint32 vertexCount;
                quint32 instanceCount;
                quint32 firstVertex;
                quint32 firstInstance;
            } draw;
            struct {
                QD3D11GraphicsPipeline *ps;
                quint32 indexCount;
                quint32 instanceCount;
                quint32 firstIndex;
                qint32 vertexOffset;
                quint32 firstInstance;
            } drawIndexed;
            struct {
                ID3D11Resource *dst;
                UINT dstSubRes;
                bool hasDstBox;
                D3D11_BOX dstBox;
                const void *src; // must come from retain*()
                UINT srcRowPitch;
            } updateSubRes;
            struct {
                ID3D11Resource *dst;
                UINT dstSubRes;
                UINT dstX;
                UINT dstY;
                UINT dstZ;
                ID3D11Resource *src;
                UINT srcSubRes;
                bool hasSrcBox;
                D3D11_BOX srcBox;
            } copySubRes;
            struct {
                ID3D11Resource *dst;
                UINT dstSubRes;
                ID3D11Resource *src;
                UINT srcSubRes;
                DXGI_FORMAT format;
            } resolveSubRes;
            struct {
                ID3D11ShaderResourceView *srv;
            } genMip;
            struct {
                char s[64];
            } debugMark;
            struct {
                QD3D11ComputePipeline *ps;
            } bindComputePipeline;
            struct {
                UINT x;
                UINT y;
                UINT z;
            } dispatch;
        } args;
    };

    enum PassType {
        NoPass,
        RenderPass,
        ComputePass
    };

    QRhiBackendCommandList<Command> commands;
    PassType recordingPass;
    double lastGpuTime = 0;
    QRhiRenderTarget *currentTarget;
    QRhiGraphicsPipeline *currentGraphicsPipeline;
    QRhiComputePipeline *currentComputePipeline;
    uint currentPipelineGeneration;
    QRhiShaderResourceBindings *currentGraphicsSrb;
    QRhiShaderResourceBindings *currentComputeSrb;
    uint currentSrbGeneration;
    ID3D11Buffer *currentIndexBuffer;
    quint32 currentIndexOffset;
    DXGI_FORMAT currentIndexFormat;
    ID3D11Buffer *currentVertexBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    quint32 currentVertexOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];

    QVarLengthArray<QByteArray, 4> dataRetainPool;
    QVarLengthArray<QRhiBufferData, 4> bufferDataRetainPool;
    QVarLengthArray<QImage, 4> imageRetainPool;

    // relies heavily on implicit sharing (no copies of the actual data will be made)
    const uchar *retainData(const QByteArray &data) {
        dataRetainPool.append(data);
        return reinterpret_cast<const uchar *>(dataRetainPool.last().constData());
    }
    const uchar *retainBufferData(const QRhiBufferData &data) {
        bufferDataRetainPool.append(data);
        return reinterpret_cast<const uchar *>(bufferDataRetainPool.last().constData());
    }
    const uchar *retainImage(const QImage &image) {
        imageRetainPool.append(image);
        return imageRetainPool.last().constBits();
    }
    void resetCommands() {
        commands.reset();
        dataRetainPool.clear();
        bufferDataRetainPool.clear();
        imageRetainPool.clear();
    }
    void resetState() {
        recordingPass = NoPass;
        // do not zero lastGpuTime
        currentTarget = nullptr;
        resetCommands();
        resetCachedState();
    }
    void resetCachedState() {
        currentGraphicsPipeline = nullptr;
        currentComputePipeline = nullptr;
        currentPipelineGeneration = 0;
        currentGraphicsSrb = nullptr;
        currentComputeSrb = nullptr;
        currentSrbGeneration = 0;
        currentIndexBuffer = nullptr;
        currentIndexOffset = 0;
        currentIndexFormat = DXGI_FORMAT_R16_UINT;
        memset(currentVertexBuffers, 0, sizeof(currentVertexBuffers));
        memset(currentVertexOffsets, 0, sizeof(currentVertexOffsets));
    }
};

struct QD3D11SwapChainTimestamps
{
    static const int TIMESTAMP_PAIRS = 2;

    bool active[TIMESTAMP_PAIRS] = {};
    ID3D11Query *disjointQuery[TIMESTAMP_PAIRS] = {};
    ID3D11Query *query[TIMESTAMP_PAIRS * 2] = {};

    bool prepare(QRhiD3D11 *rhiD);
    void destroy();
    bool tryQueryTimestamps(int idx, ID3D11DeviceContext *context, double *elapsedSec);
};

struct QD3D11SwapChain : public QRhiSwapChain
{
    QD3D11SwapChain(QRhiImplementation *rhi);
    ~QD3D11SwapChain();
    void destroy() override;

    QRhiCommandBuffer *currentFrameCommandBuffer() override;
    QRhiRenderTarget *currentFrameRenderTarget() override;
    QRhiRenderTarget *currentFrameRenderTarget(StereoTargetBuffer targetBuffer) override;

    QSize surfacePixelSize() override;
    bool isFormatSupported(Format f) override;
    QRhiSwapChainHdrInfo hdrInfo() override;

    QRhiRenderPassDescriptor *newCompatibleRenderPassDescriptor() override;
    bool createOrResize() override;

    void releaseBuffers();
    bool newColorBuffer(const QSize &size, DXGI_FORMAT format, DXGI_SAMPLE_DESC sampleDesc,
                        ID3D11Texture2D **tex, ID3D11RenderTargetView **rtv) const;

    QWindow *window = nullptr;
    QSize pixelSize;
    QD3D11SwapChainRenderTarget rt;
    QD3D11SwapChainRenderTarget rtRight;
    QD3D11CommandBuffer cb;
    DXGI_FORMAT colorFormat;
    DXGI_FORMAT srgbAdjustedColorFormat;
    IDXGISwapChain *swapChain = nullptr;
    UINT swapChainFlags = 0;
    ID3D11Texture2D *backBufferTex;
    ID3D11RenderTargetView *backBufferRtv;
    ID3D11RenderTargetView *backBufferRtvRight = nullptr;
    static const int BUFFER_COUNT = 2;
    ID3D11Texture2D *msaaTex[BUFFER_COUNT];
    ID3D11RenderTargetView *msaaRtv[BUFFER_COUNT];
    DXGI_SAMPLE_DESC sampleDesc;
    int currentFrameSlot = 0;
    int frameCount = 0;
    QD3D11RenderBuffer *ds = nullptr;
    UINT swapInterval = 1;
    IDCompositionTarget *dcompTarget = nullptr;
    IDCompositionVisual *dcompVisual = nullptr;
    QD3D11SwapChainTimestamps timestamps;
    int currentTimestampPairIndex = 0;
    HANDLE frameLatencyWaitableObject = nullptr;
};

class QRhiD3D11 : public QRhiImplementation
{
public:
    QRhiD3D11(QRhiD3D11InitParams *params, QRhiD3D11NativeHandles *importDevice = nullptr);

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

    QRhiShadingRateMap *createShadingRateMap() override;

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
    void setShadingRate(QRhiCommandBuffer *cb, const QSize &coarsePixelSize) override;

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
    QList<QSize> supportedShadingRates(int sampleCount) const override;
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
    void setQueueSubmitParams(QRhiNativeHandles *params) override;
    void releaseCachedResources() override;
    bool isDeviceLost() const override;

    QByteArray pipelineCacheData() override;
    void setPipelineCacheData(const QByteArray &data) override;

    void enqueueSubresUpload(QD3D11Texture *texD, QD3D11CommandBuffer *cbD,
                             int layer, int level, const QRhiTextureSubresourceUploadDescription &subresDesc);
    void enqueueResourceUpdates(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates);
    void updateShaderResourceBindings(QD3D11ShaderResourceBindings *srbD,
                                      const QShader::NativeResourceBindingMap *nativeResourceBindingMaps[]);
    void executeBufferHostWrites(QD3D11Buffer *bufD);
    void bindShaderResources(QD3D11ShaderResourceBindings *srbD,
                             const uint *dynOfsPairs, int dynOfsPairCount,
                             bool offsetOnlyChange);
    void resetShaderResources();
    void executeCommandBuffer(QD3D11CommandBuffer *cbD);
    DXGI_SAMPLE_DESC effectiveSampleDesc(int sampleCount) const;
    void finishActiveReadbacks();
    void reportLiveObjects(ID3D11Device *device);
    void clearShaderCache();
    QByteArray compileHlslShaderSource(const QShader &shader, QShader::Variant shaderVariant, uint flags,
                                       QString *error, QShaderKey *usedShaderKey);
    bool ensureDirectCompositionDevice();

    QRhi::Flags rhiFlags;
    bool debugLayer = false;
    UINT maxFrameLatency = 2; // 1-3, use 2 to keep CPU-GPU parallelism while reducing lag compared to tripple buffering
    bool importedDeviceAndContext = false;
    ID3D11Device *dev = nullptr;
    ID3D11DeviceContext1 *context = nullptr;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL(0);
    LUID adapterLuid = {};
    ID3DUserDefinedAnnotation *annotations = nullptr;
    IDXGIAdapter1 *activeAdapter = nullptr;
    IDXGIFactory1 *dxgiFactory = nullptr;
    IDCompositionDevice *dcompDevice = nullptr;
    bool supportsAllowTearing = false;
    bool useLegacySwapchainModel = false;
    bool deviceLost = false;
    QRhiD3D11NativeHandles nativeHandlesStruct;
    QRhiDriverInfo driverInfoStruct;

    struct {
        int vsHighestActiveVertexBufferBinding = -1;
        bool vsHasIndexBufferBound = false;
        int vsHighestActiveSrvBinding = -1;
        int hsHighestActiveSrvBinding = -1;
        int dsHighestActiveSrvBinding = -1;
        int gsHighestActiveSrvBinding = -1;
        int fsHighestActiveSrvBinding = -1;
        int csHighestActiveSrvBinding = -1;
        int csHighestActiveUavBinding = -1;
        QD3D11SwapChain *currentSwapChain = nullptr;
    } contextState;

    struct OffscreenFrame {
        OffscreenFrame(QRhiImplementation *rhi) : cbWrapper(rhi) { }
        bool active = false;
        QD3D11CommandBuffer cbWrapper;
        ID3D11Query *tsQueries[2] = {};
        ID3D11Query *tsDisjointQuery = nullptr;
    } ofr;

    struct TextureReadback {
        QRhiReadbackDescription desc;
        QRhiReadbackResult *result;
        ID3D11Texture2D *stagingTex;
        quint32 byteSize;
        quint32 bpl;
        QSize pixelSize;
        QRhiTexture::Format format;
    };
    QVarLengthArray<TextureReadback, 2> activeTextureReadbacks;
    struct BufferReadback {
        QRhiReadbackResult *result;
        quint32 byteSize;
        ID3D11Buffer *stagingBuf;
    };
    QVarLengthArray<BufferReadback, 2> activeBufferReadbacks;

    struct Shader {
        Shader() = default;
        Shader(IUnknown *s, const QByteArray &bytecode, const QShader::NativeResourceBindingMap &rbm)
            : s(s), bytecode(bytecode), nativeResourceBindingMap(rbm) { }
        IUnknown *s;
        QByteArray bytecode;
        QShader::NativeResourceBindingMap nativeResourceBindingMap;
    };
    QHash<QRhiShaderStage, Shader> m_shaderCache;

    // This is what gets exposed as the "pipeline cache", not that that concept
    // applies anyway. Here we are just storing the DX bytecode for a shader so
    // we can skip the HLSL->DXBC compilation when the QShader has HLSL source
    // code and the same shader source has already been compiled before.
    // m_shaderCache seemingly does the same, but this here does not care about
    // the ID3D11*Shader, this is just about the bytecode and about allowing
    // the data to be serialized to persistent storage and then reloaded in
    // future runs of the app, or when creating another QRhi, etc.
    struct BytecodeCacheKey {
        QByteArray sourceHash;
        QByteArray target;
        QByteArray entryPoint;
        uint compileFlags;
    };
    QHash<BytecodeCacheKey, QByteArray> m_bytecodeCache;
};

Q_DECLARE_TYPEINFO(QRhiD3D11::TextureReadback, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QRhiD3D11::BufferReadback, Q_RELOCATABLE_TYPE);

inline bool operator==(const QRhiD3D11::BytecodeCacheKey &a, const QRhiD3D11::BytecodeCacheKey &b) noexcept
{
    return a.sourceHash == b.sourceHash
            && a.target == b.target
            && a.entryPoint == b.entryPoint
            && a.compileFlags == b.compileFlags;
}

inline bool operator!=(const QRhiD3D11::BytecodeCacheKey &a, const QRhiD3D11::BytecodeCacheKey &b) noexcept
{
    return !(a == b);
}

inline size_t qHash(const QRhiD3D11::BytecodeCacheKey &k, size_t seed = 0) noexcept
{
    return qHash(k.sourceHash, seed) ^ qHash(k.target) ^ qHash(k.entryPoint) ^ k.compileFlags;
}

QT_END_NAMESPACE

#endif
