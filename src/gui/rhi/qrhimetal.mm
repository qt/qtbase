/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qrhimetal_p_p.h"
#include <QGuiApplication>
#include <QWindow>
#include <qmath.h>

#ifdef Q_OS_MACOS
#include <AppKit/AppKit.h>
#else
#include <UIKit/UIKit.h>
#endif

#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

QT_BEGIN_NAMESPACE

/*
    Metal backend. Double buffers and throttles to vsync. "Dynamic" buffers are
    Shared (host visible) and duplicated (to help having 2 frames in flight),
    "static" and "immutable" are Managed on macOS and Shared on iOS/tvOS.
    Textures are Private (device local) and a host visible staging buffer is
    used to upload data to them. Does not rely on strong objects refs from
    command buffers but does rely on the automatic resource tracking of the
    command encoders. Assumes that an autorelease pool (ideally per frame) is
    available on the thread on which QRhi is used.
*/

#if __has_feature(objc_arc)
#error ARC not supported
#endif

// Note: we expect everything here pass the Metal API validation when running
// in Debug mode in XCode. Some of the issues that break validation are not
// obvious and not visible when running outside XCode.
//
// An exception is the nextDrawable Called Early blah blah warning, which is
// plain and simply false.

/*!
    \class QRhiMetalInitParams
    \inmodule QtRhi
    \internal
    \brief Metal specific initialization parameters.

    A Metal-based QRhi needs no special parameters for initialization.

    \badcode
        QRhiMetalInitParams params;
        rhi = QRhi::create(QRhi::Metal, &params);
    \endcode

    \note Metal API validation cannot be enabled by the application. Instead,
    run the debug build of the application in XCode. Generating a
    \c{.xcodeproj} file via \c{qmake -spec macx-xcode} provides a convenient
    way to enable this.

    \note QRhiSwapChain can only target QWindow instances that have their
    surface type set to QSurface::MetalSurface.

    \section2 Working with existing Metal devices

    When interoperating with another graphics engine, it may be necessary to
    get a QRhi instance that uses the same Metal device. This can be achieved
    by passing a pointer to a QRhiMetalNativeHandles to QRhi::create(). The
    device must be set to a non-null value then. Optionally, a command queue
    object can be specified as well.

    The QRhi does not take ownership of any of the external objects.
 */

/*!
    \class QRhiMetalNativeHandles
    \inmodule QtRhi
    \internal
    \brief Holds the Metal device used by the QRhi.

    \note The class uses \c{void *} as the type since including the Objective C
    headers is not acceptable here. The actual types are \c{id<MTLDevice>} and
    \c{id<MTLCommandQueue>}.
 */

/*!
    \class QRhiMetalCommandBufferNativeHandles
    \inmodule QtRhi
    \internal
    \brief Holds the MTLCommandBuffer and MTLRenderCommandEncoder objects that are backing a QRhiCommandBuffer.

    \note The command buffer object is only guaranteed to be valid while
    recording a frame, that is, between a \l{QRhi::beginFrame()}{beginFrame()}
    - \l{QRhi::endFrame()}{endFrame()} or
    \l{QRhi::beginOffscreenFrame()}{beginOffscreenFrame()} -
    \l{QRhi::endOffscreenFrame()}{endOffsrceenFrame()} pair.

    \note The command encoder is only valid while recording a pass, that is,
    between \l{QRhiCommandBuffer::beginPass()} -
    \l{QRhiCommandBuffer::endPass()}.
 */

struct QMetalShader
{
    id<MTLLibrary> lib = nil;
    id<MTLFunction> func = nil;
    std::array<uint, 3> localSize;
    QShader::NativeResourceBindingMap nativeResourceBindingMap;

    void release() {
        nativeResourceBindingMap.clear();
        [lib release];
        lib = nil;
        [func release];
        func = nil;
    }
};

struct QRhiMetalData
{
    QRhiMetalData(QRhiImplementation *rhi) : ofr(rhi) { }

    id<MTLDevice> dev = nil;
    id<MTLCommandQueue> cmdQueue = nil;

    MTLRenderPassDescriptor *createDefaultRenderPass(bool hasDepthStencil,
                                                     const QColor &colorClearValue,
                                                     const QRhiDepthStencilClearValue &depthStencilClearValue,
                                                     int colorAttCount);
    id<MTLLibrary> createMetalLib(const QShader &shader, QShader::Variant shaderVariant,
                                  QString *error, QByteArray *entryPoint, QShaderKey *activeKey);
    id<MTLFunction> createMSLShaderFunction(id<MTLLibrary> lib, const QByteArray &entryPoint);

    struct DeferredReleaseEntry {
        enum Type {
            Buffer,
            RenderBuffer,
            Texture,
            Sampler,
            StagingBuffer
        };
        Type type;
        int lastActiveFrameSlot; // -1 if not used otherwise 0..FRAMES_IN_FLIGHT-1
        union {
            struct {
                id<MTLBuffer> buffers[QMTL_FRAMES_IN_FLIGHT];
            } buffer;
            struct {
                id<MTLTexture> texture;
            } renderbuffer;
            struct {
                id<MTLTexture> texture;
                id<MTLBuffer> stagingBuffers[QMTL_FRAMES_IN_FLIGHT];
                id<MTLTexture> views[QRhi::MAX_LEVELS];
            } texture;
            struct {
                id<MTLSamplerState> samplerState;
            } sampler;
            struct {
                id<MTLBuffer> buffer;
            } stagingBuffer;
        };
    };
    QVector<DeferredReleaseEntry> releaseQueue;

    struct OffscreenFrame {
        OffscreenFrame(QRhiImplementation *rhi) : cbWrapper(rhi) { }
        bool active = false;
        QMetalCommandBuffer cbWrapper;
    } ofr;

    struct TextureReadback {
        int activeFrameSlot = -1;
        QRhiReadbackDescription desc;
        QRhiReadbackResult *result;
        id<MTLBuffer> buf;
        quint32 bufSize;
        QSize pixelSize;
        QRhiTexture::Format format;
    };
    QVector<TextureReadback> activeTextureReadbacks;

    API_AVAILABLE(macos(10.13), ios(11.0)) MTLCaptureManager *captureMgr;
    API_AVAILABLE(macos(10.13), ios(11.0)) id<MTLCaptureScope> captureScope = nil;

    static const int TEXBUF_ALIGN = 256; // probably not accurate

    QHash<QRhiShaderStage, QMetalShader> shaderCache;
};

Q_DECLARE_TYPEINFO(QRhiMetalData::DeferredReleaseEntry, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(QRhiMetalData::TextureReadback, Q_MOVABLE_TYPE);

struct QMetalBufferData
{
    bool managed;
    bool slotted;
    id<MTLBuffer> buf[QMTL_FRAMES_IN_FLIGHT];
    QVarLengthArray<QRhiResourceUpdateBatchPrivate::BufferOp, 16> pendingUpdates[QMTL_FRAMES_IN_FLIGHT];
};

struct QMetalRenderBufferData
{
    MTLPixelFormat format;
    id<MTLTexture> tex = nil;
};

struct QMetalTextureData
{
    QMetalTextureData(QMetalTexture *t) : q(t) { }

    QMetalTexture *q;
    MTLPixelFormat format;
    id<MTLTexture> tex = nil;
    id<MTLBuffer> stagingBuf[QMTL_FRAMES_IN_FLIGHT];
    bool owns = true;
    id<MTLTexture> perLevelViews[QRhi::MAX_LEVELS];

    id<MTLTexture> viewForLevel(int level);
};

struct QMetalSamplerData
{
    id<MTLSamplerState> samplerState = nil;
};

struct QMetalCommandBufferData
{
    id<MTLCommandBuffer> cb;
    id<MTLRenderCommandEncoder> currentRenderPassEncoder;
    id<MTLComputeCommandEncoder> currentComputePassEncoder;
    MTLRenderPassDescriptor *currentPassRpDesc;
    int currentFirstVertexBinding;
    QRhiBatchedBindings<id<MTLBuffer> > currentVertexInputsBuffers;
    QRhiBatchedBindings<NSUInteger> currentVertexInputOffsets;
};

struct QMetalRenderTargetData
{
    QSize pixelSize;
    float dpr = 1;
    int sampleCount = 1;
    int colorAttCount = 0;
    int dsAttCount = 0;

    struct ColorAtt {
        bool needsDrawableForTex = false;
        id<MTLTexture> tex = nil;
        int layer = 0;
        int level = 0;
        bool needsDrawableForResolveTex = false;
        id<MTLTexture> resolveTex = nil;
        int resolveLayer = 0;
        int resolveLevel = 0;
    };

    struct {
        ColorAtt colorAtt[QMetalRenderPassDescriptor::MAX_COLOR_ATTACHMENTS];
        id<MTLTexture> dsTex = nil;
        bool hasStencil = false;
        bool depthNeedsStore = false;
    } fb;
};

struct QMetalGraphicsPipelineData
{
    id<MTLRenderPipelineState> ps = nil;
    id<MTLDepthStencilState> ds = nil;
    MTLPrimitiveType primitiveType;
    MTLWinding winding;
    MTLCullMode cullMode;
    float depthBias;
    float slopeScaledDepthBias;
    QMetalShader vs;
    QMetalShader fs;
};

struct QMetalComputePipelineData
{
    id<MTLComputePipelineState> ps = nil;
    QMetalShader cs;
    MTLSize localSize;
};

struct QMetalSwapChainData
{
    // The iOS simulator's headers mark CAMetalLayer as iOS 13.0+ only.
    // (for real device SDKs it is 8.0+)
#ifdef TARGET_IPHONE_SIMULATOR
    API_AVAILABLE(ios(13.0)) CAMetalLayer *layer = nullptr;
#else
    CAMetalLayer *layer = nullptr;
#endif
    id<CAMetalDrawable> curDrawable;
    dispatch_semaphore_t sem[QMTL_FRAMES_IN_FLIGHT];
    MTLRenderPassDescriptor *rp = nullptr;
    id<MTLTexture> msaaTex[QMTL_FRAMES_IN_FLIGHT];
    QRhiTexture::Format rhiColorFormat;
    MTLPixelFormat colorFormat;
};

QRhiMetal::QRhiMetal(QRhiMetalInitParams *params, QRhiMetalNativeHandles *importDevice)
{
    Q_UNUSED(params);

    d = new QRhiMetalData(this);

    importedDevice = importDevice != nullptr;
    if (importedDevice) {
        if (d->dev) {
            d->dev = (id<MTLDevice>) importDevice->dev;
            importedCmdQueue = importDevice->cmdQueue != nullptr;
            if (importedCmdQueue)
                d->cmdQueue = (id<MTLCommandQueue>) importDevice->cmdQueue;
        } else {
            qWarning("No MTLDevice given, cannot import");
            importedDevice = false;
        }
    }
}

QRhiMetal::~QRhiMetal()
{
    delete d;
}

template <class Int>
inline Int aligned(Int v, Int byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

bool QRhiMetal::create(QRhi::Flags flags)
{
    Q_UNUSED(flags);

    if (importedDevice)
        [d->dev retain];
    else
        d->dev = MTLCreateSystemDefaultDevice();

    if (!d->dev) {
        qWarning("No MTLDevice");
        return false;
    }

    qCDebug(QRHI_LOG_INFO, "Metal device: %s", qPrintable(QString::fromNSString([d->dev name])));

    if (importedCmdQueue)
        [d->cmdQueue retain];
    else
        d->cmdQueue = [d->dev newCommandQueue];

    if (@available(macOS 10.13, iOS 11.0, *)) {
        d->captureMgr = [MTLCaptureManager sharedCaptureManager];
        // Have a custom capture scope as well which then shows up in XCode as
        // an option when capturing, and becomes especially useful when having
        // multiple windows with multiple QRhis.
        d->captureScope = [d->captureMgr newCaptureScopeWithCommandQueue: d->cmdQueue];
        const QString label = QString::asprintf("Qt capture scope for QRhi %p", this);
        d->captureScope.label = label.toNSString();
    }

#if defined(Q_OS_MACOS)
    caps.maxTextureSize = 16384;
#elif defined(Q_OS_TVOS)
    if ([d->dev supportsFeatureSet: MTLFeatureSet(30003)]) // MTLFeatureSet_tvOS_GPUFamily2_v1
        caps.maxTextureSize = 16384;
    else
        caps.maxTextureSize = 8192;
#elif defined(Q_OS_IOS)
    // welcome to feature set hell
    if ([d->dev supportsFeatureSet: MTLFeatureSet(16)] // MTLFeatureSet_iOS_GPUFamily5_v1
            || [d->dev supportsFeatureSet: MTLFeatureSet(11)] // MTLFeatureSet_iOS_GPUFamily4_v1
            || [d->dev supportsFeatureSet: MTLFeatureSet(4)]) // MTLFeatureSet_iOS_GPUFamily3_v1
    {
        caps.maxTextureSize = 16384;
    } else if ([d->dev supportsFeatureSet: MTLFeatureSet(3)] // MTLFeatureSet_iOS_GPUFamily2_v2
            || [d->dev supportsFeatureSet: MTLFeatureSet(2)]) // MTLFeatureSet_iOS_GPUFamily1_v2
    {
        caps.maxTextureSize = 8192;
    } else {
        caps.maxTextureSize = 4096;
    }
#endif

    nativeHandlesStruct.dev = d->dev;
    nativeHandlesStruct.cmdQueue = d->cmdQueue;

    return true;
}

void QRhiMetal::destroy()
{
    executeDeferredReleases(true);
    finishActiveReadbacks(true);

    for (QMetalShader &s : d->shaderCache)
        s.release();
    d->shaderCache.clear();

    if (@available(macOS 10.13, iOS 11.0, *)) {
        [d->captureScope release];
        d->captureScope = nil;
    }

    [d->cmdQueue release];
    if (!importedCmdQueue)
        d->cmdQueue = nil;

    [d->dev release];
    if (!importedDevice)
        d->dev = nil;
}

QVector<int> QRhiMetal::supportedSampleCounts() const
{
    return { 1, 2, 4, 8 };
}

int QRhiMetal::effectiveSampleCount(int sampleCount) const
{
    // Stay compatible with QSurfaceFormat and friends where samples == 0 means the same as 1.
    const int s = qBound(1, sampleCount, 64);
    if (!supportedSampleCounts().contains(s)) {
        qWarning("Attempted to set unsupported sample count %d", sampleCount);
        return 1;
    }
    return s;
}

QRhiSwapChain *QRhiMetal::createSwapChain()
{
    return new QMetalSwapChain(this);
}

QRhiBuffer *QRhiMetal::createBuffer(QRhiBuffer::Type type, QRhiBuffer::UsageFlags usage, int size)
{
    return new QMetalBuffer(this, type, usage, size);
}

int QRhiMetal::ubufAlignment() const
{
    return 256;
}

bool QRhiMetal::isYUpInFramebuffer() const
{
    return false;
}

bool QRhiMetal::isYUpInNDC() const
{
    return true;
}

bool QRhiMetal::isClipDepthZeroToOne() const
{
    return true;
}

QMatrix4x4 QRhiMetal::clipSpaceCorrMatrix() const
{
    // depth range 0..1
    static QMatrix4x4 m;
    if (m.isIdentity()) {
        // NB the ctor takes row-major
        m = QMatrix4x4(1.0f, 0.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 0.5f, 0.5f,
                       0.0f, 0.0f, 0.0f, 1.0f);
    }
    return m;
}

bool QRhiMetal::isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const
{
    Q_UNUSED(flags);

#ifdef Q_OS_MACOS
    if (format >= QRhiTexture::ETC2_RGB8 && format <= QRhiTexture::ETC2_RGBA8)
        return false;
    if (format >= QRhiTexture::ASTC_4x4 && format <= QRhiTexture::ASTC_12x12)
        return false;
#else
    if (format >= QRhiTexture::BC1 && format <= QRhiTexture::BC7)
        return false;
#endif

    return true;
}

bool QRhiMetal::isFeatureSupported(QRhi::Feature feature) const
{
    switch (feature) {
    case QRhi::MultisampleTexture:
        return true;
    case QRhi::MultisampleRenderBuffer:
        return true;
    case QRhi::DebugMarkers:
        return true;
    case QRhi::Timestamps:
        return false;
    case QRhi::Instancing:
        return true;
    case QRhi::CustomInstanceStepRate:
        return true;
    case QRhi::PrimitiveRestart:
        return true;
    case QRhi::NonDynamicUniformBuffers:
        return true;
    case QRhi::NonFourAlignedEffectiveIndexBufferOffset:
        return false;
    case QRhi::NPOTTextureRepeat:
        return true;
    case QRhi::RedOrAlpha8IsRed:
        return true;
    case QRhi::ElementIndexUint:
        return true;
    case QRhi::Compute:
        return true;
    case QRhi::WideLines:
        return false;
    case QRhi::VertexShaderPointSize:
        return true;
    case QRhi::BaseVertex:
        return true;
    case QRhi::BaseInstance:
        return true;
    case QRhi::TriangleFanTopology:
        return false;
    case QRhi::ReadBackNonUniformBuffer:
        return true;
    case QRhi::ReadBackNonBaseMipLevel:
        return true;
    case QRhi::TexelFetch:
        return true;
    default:
        Q_UNREACHABLE();
        return false;
    }
}

int QRhiMetal::resourceLimit(QRhi::ResourceLimit limit) const
{
    switch (limit) {
    case QRhi::TextureSizeMin:
        return 1;
    case QRhi::TextureSizeMax:
        return caps.maxTextureSize;
    case QRhi::MaxColorAttachments:
        return 8;
    case QRhi::FramesInFlight:
        return QMTL_FRAMES_IN_FLIGHT;
    case QRhi::MaxAsyncReadbackFrames:
        return QMTL_FRAMES_IN_FLIGHT;
    default:
        Q_UNREACHABLE();
        return 0;
    }
}

const QRhiNativeHandles *QRhiMetal::nativeHandles()
{
    return &nativeHandlesStruct;
}

void QRhiMetal::sendVMemStatsToProfiler()
{
    // nothing to do here
}

bool QRhiMetal::makeThreadLocalNativeContextCurrent()
{
    // not applicable
    return false;
}

void QRhiMetal::releaseCachedResources()
{
    for (QMetalShader &s : d->shaderCache)
        s.release();

    d->shaderCache.clear();
}

bool QRhiMetal::isDeviceLost() const
{
    return false;
}

QRhiRenderBuffer *QRhiMetal::createRenderBuffer(QRhiRenderBuffer::Type type, const QSize &pixelSize,
                                                int sampleCount, QRhiRenderBuffer::Flags flags)
{
    return new QMetalRenderBuffer(this, type, pixelSize, sampleCount, flags);
}

QRhiTexture *QRhiMetal::createTexture(QRhiTexture::Format format, const QSize &pixelSize,
                                      int sampleCount, QRhiTexture::Flags flags)
{
    return new QMetalTexture(this, format, pixelSize, sampleCount, flags);
}

QRhiSampler *QRhiMetal::createSampler(QRhiSampler::Filter magFilter, QRhiSampler::Filter minFilter,
                                      QRhiSampler::Filter mipmapMode,
                                      QRhiSampler::AddressMode u, QRhiSampler::AddressMode v, QRhiSampler::AddressMode w)
{
    return new QMetalSampler(this, magFilter, minFilter, mipmapMode, u, v, w);
}

QRhiTextureRenderTarget *QRhiMetal::createTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                              QRhiTextureRenderTarget::Flags flags)
{
    return new QMetalTextureRenderTarget(this, desc, flags);
}

QRhiGraphicsPipeline *QRhiMetal::createGraphicsPipeline()
{
    return new QMetalGraphicsPipeline(this);
}

QRhiComputePipeline *QRhiMetal::createComputePipeline()
{
    return new QMetalComputePipeline(this);
}

QRhiShaderResourceBindings *QRhiMetal::createShaderResourceBindings()
{
    return new QMetalShaderResourceBindings(this);
}

enum class BindingType {
    Buffer,
    Texture,
    Sampler
};

static inline int mapBinding(int binding,
                             int stageIndex,
                             const QShader::NativeResourceBindingMap *nativeResourceBindingMaps[],
                             BindingType type)
{
    const QShader::NativeResourceBindingMap *map = nativeResourceBindingMaps[stageIndex];
    if (!map || map->isEmpty())
        return binding; // old QShader versions do not have this map, assume 1:1 mapping then

    auto it = map->constFind(binding);
    if (it != map->cend())
        return type == BindingType::Sampler ? it->second : it->first; // may be -1, if the resource is inactive

    // Hitting this path is normal too. It is not given that the resource (for
    // example, a uniform block) is present in the shaders for all the stages
    // specified by the visibility mask in the QRhiShaderResourceBinding.
    return -1;
}

void QRhiMetal::enqueueShaderResourceBindings(QMetalShaderResourceBindings *srbD,
                                              QMetalCommandBuffer *cbD,
                                              int dynamicOffsetCount,
                                              const QRhiCommandBuffer::DynamicOffset *dynamicOffsets,
                                              bool offsetOnlyChange,
                                              const QShader::NativeResourceBindingMap *nativeResourceBindingMaps[SUPPORTED_STAGES])
{
    struct Stage {
        struct Buffer {
            int nativeBinding;
            id<MTLBuffer> mtlbuf;
            uint offset;
        };
        struct Texture {
            int nativeBinding;
            id<MTLTexture> mtltex;
        };
        struct Sampler {
            int nativeBinding;
            id<MTLSamplerState> mtlsampler;
        };
        QVarLengthArray<Buffer, 8> buffers;
        QVarLengthArray<Texture, 8> textures;
        QVarLengthArray<Sampler, 8> samplers;
        QRhiBatchedBindings<id<MTLBuffer> > bufferBatches;
        QRhiBatchedBindings<NSUInteger> bufferOffsetBatches;
        QRhiBatchedBindings<id<MTLTexture> > textureBatches;
        QRhiBatchedBindings<id<MTLSamplerState> > samplerBatches;
    } res[SUPPORTED_STAGES];
    enum { VERTEX = 0, FRAGMENT = 1, COMPUTE = 2 };

    for (const QRhiShaderResourceBinding &binding : qAsConst(srbD->sortedBindings)) {
        const QRhiShaderResourceBinding::Data *b = binding.data();
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            QMetalBuffer *bufD = QRHI_RES(QMetalBuffer, b->u.ubuf.buf);
            id<MTLBuffer> mtlbuf = bufD->d->buf[bufD->d->slotted ? currentFrameSlot : 0];
            uint offset = uint(b->u.ubuf.offset);
            for (int i = 0; i < dynamicOffsetCount; ++i) {
                const QRhiCommandBuffer::DynamicOffset &dynOfs(dynamicOffsets[i]);
                if (dynOfs.first == b->binding) {
                    offset = dynOfs.second;
                    break;
                }
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::VertexStage)) {
                const int nativeBinding = mapBinding(b->binding, VERTEX, nativeResourceBindingMaps, BindingType::Buffer);
                if (nativeBinding >= 0)
                    res[VERTEX].buffers.append({ nativeBinding, mtlbuf, offset });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::FragmentStage)) {
                const int nativeBinding = mapBinding(b->binding, FRAGMENT, nativeResourceBindingMaps, BindingType::Buffer);
                if (nativeBinding >= 0)
                    res[FRAGMENT].buffers.append({ nativeBinding, mtlbuf, offset });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::ComputeStage)) {
                const int nativeBinding = mapBinding(b->binding, COMPUTE, nativeResourceBindingMaps, BindingType::Buffer);
                if (nativeBinding >= 0)
                    res[COMPUTE].buffers.append({ nativeBinding, mtlbuf, offset });
            }
        }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
        {
            const QRhiShaderResourceBinding::Data::SampledTextureData *data = &b->u.stex;
            for (int elem = 0; elem < data->count; ++elem) {
                QMetalTexture *texD = QRHI_RES(QMetalTexture, b->u.stex.texSamplers[elem].tex);
                QMetalSampler *samplerD = QRHI_RES(QMetalSampler, b->u.stex.texSamplers[elem].sampler);
                if (b->stage.testFlag(QRhiShaderResourceBinding::VertexStage)) {
                    const int nativeBindingTexture = mapBinding(b->binding, VERTEX, nativeResourceBindingMaps, BindingType::Texture);
                    const int nativeBindingSampler = mapBinding(b->binding, VERTEX, nativeResourceBindingMaps, BindingType::Sampler);
                    if (nativeBindingTexture >= 0 && nativeBindingSampler >= 0) {
                        res[VERTEX].textures.append({ nativeBindingTexture + elem, texD->d->tex });
                        res[VERTEX].samplers.append({ nativeBindingSampler + elem, samplerD->d->samplerState });
                    }
                }
                if (b->stage.testFlag(QRhiShaderResourceBinding::FragmentStage)) {
                    const int nativeBindingTexture = mapBinding(b->binding, FRAGMENT, nativeResourceBindingMaps, BindingType::Texture);
                    const int nativeBindingSampler = mapBinding(b->binding, FRAGMENT, nativeResourceBindingMaps, BindingType::Sampler);
                    if (nativeBindingTexture >= 0 && nativeBindingSampler >= 0) {
                        res[FRAGMENT].textures.append({ nativeBindingTexture + elem, texD->d->tex });
                        res[FRAGMENT].samplers.append({ nativeBindingSampler + elem, samplerD->d->samplerState });
                    }
                }
                if (b->stage.testFlag(QRhiShaderResourceBinding::ComputeStage)) {
                    const int nativeBindingTexture = mapBinding(b->binding, COMPUTE, nativeResourceBindingMaps, BindingType::Texture);
                    const int nativeBindingSampler = mapBinding(b->binding, COMPUTE, nativeResourceBindingMaps, BindingType::Sampler);
                    if (nativeBindingTexture >= 0 && nativeBindingSampler >= 0) {
                        res[COMPUTE].textures.append({ nativeBindingTexture + elem, texD->d->tex });
                        res[COMPUTE].samplers.append({ nativeBindingSampler + elem, samplerD->d->samplerState });
                    }
                }
            }
        }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
        {
            QMetalTexture *texD = QRHI_RES(QMetalTexture, b->u.simage.tex);
            id<MTLTexture> t = texD->d->viewForLevel(b->u.simage.level);
            if (b->stage.testFlag(QRhiShaderResourceBinding::VertexStage)) {
                const int nativeBinding = mapBinding(b->binding, VERTEX, nativeResourceBindingMaps, BindingType::Texture);
                if (nativeBinding >= 0)
                    res[VERTEX].textures.append({ nativeBinding, t });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::FragmentStage)) {
                const int nativeBinding = mapBinding(b->binding, FRAGMENT, nativeResourceBindingMaps, BindingType::Texture);
                if (nativeBinding >= 0)
                    res[FRAGMENT].textures.append({ nativeBinding, t });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::ComputeStage)) {
                const int nativeBinding = mapBinding(b->binding, COMPUTE, nativeResourceBindingMaps, BindingType::Texture);
                if (nativeBinding >= 0)
                    res[COMPUTE].textures.append({ nativeBinding, t });
            }
        }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
        case QRhiShaderResourceBinding::BufferStore:
        case QRhiShaderResourceBinding::BufferLoadStore:
        {
            QMetalBuffer *bufD = QRHI_RES(QMetalBuffer, b->u.sbuf.buf);
            id<MTLBuffer> mtlbuf = bufD->d->buf[0];
            uint offset = uint(b->u.sbuf.offset);
            if (b->stage.testFlag(QRhiShaderResourceBinding::VertexStage)) {
                const int nativeBinding = mapBinding(b->binding, VERTEX, nativeResourceBindingMaps, BindingType::Buffer);
                if (nativeBinding >= 0)
                    res[VERTEX].buffers.append({ nativeBinding, mtlbuf, offset });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::FragmentStage)) {
                const int nativeBinding = mapBinding(b->binding, FRAGMENT, nativeResourceBindingMaps, BindingType::Buffer);
                if (nativeBinding >= 0)
                    res[FRAGMENT].buffers.append({ nativeBinding, mtlbuf, offset });
            }
            if (b->stage.testFlag(QRhiShaderResourceBinding::ComputeStage)) {
                const int nativeBinding = mapBinding(b->binding, COMPUTE, nativeResourceBindingMaps, BindingType::Buffer);
                if (nativeBinding >= 0)
                    res[COMPUTE].buffers.append({ nativeBinding, mtlbuf, offset });
            }
        }
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    for (int stage = 0; stage < SUPPORTED_STAGES; ++stage) {
        if (cbD->recordingPass != QMetalCommandBuffer::RenderPass && (stage == VERTEX || stage == FRAGMENT))
            continue;
        if (cbD->recordingPass != QMetalCommandBuffer::ComputePass && stage == COMPUTE)
            continue;

        // QRhiBatchedBindings works with the native bindings and expects
        // sorted input. The pre-sorted QRhiShaderResourceBinding list (based
        // on the QRhi (SPIR-V) binding) is not helpful in this regard, so we
        // have to sort here every time.

        std::sort(res[stage].buffers.begin(), res[stage].buffers.end(), [](const Stage::Buffer &a, const Stage::Buffer &b) {
            return a.nativeBinding < b.nativeBinding;
        });

        for (const Stage::Buffer &buf : qAsConst(res[stage].buffers)) {
            res[stage].bufferBatches.feed(buf.nativeBinding, buf.mtlbuf);
            res[stage].bufferOffsetBatches.feed(buf.nativeBinding, buf.offset);
        }

        res[stage].bufferBatches.finish();
        res[stage].bufferOffsetBatches.finish();

        for (int i = 0, ie = res[stage].bufferBatches.batches.count(); i != ie; ++i) {
            const auto &bufferBatch(res[stage].bufferBatches.batches[i]);
            const auto &offsetBatch(res[stage].bufferOffsetBatches.batches[i]);
            switch (stage) {
            case VERTEX:
                [cbD->d->currentRenderPassEncoder setVertexBuffers: bufferBatch.resources.constData()
                  offsets: offsetBatch.resources.constData()
                  withRange: NSMakeRange(bufferBatch.startBinding, NSUInteger(bufferBatch.resources.count()))];
                break;
            case FRAGMENT:
                [cbD->d->currentRenderPassEncoder setFragmentBuffers: bufferBatch.resources.constData()
                  offsets: offsetBatch.resources.constData()
                  withRange: NSMakeRange(bufferBatch.startBinding, NSUInteger(bufferBatch.resources.count()))];
                break;
            case COMPUTE:
                [cbD->d->currentComputePassEncoder setBuffers: bufferBatch.resources.constData()
                  offsets: offsetBatch.resources.constData()
                  withRange: NSMakeRange(bufferBatch.startBinding, NSUInteger(bufferBatch.resources.count()))];
                break;
            default:
                Q_UNREACHABLE();
                break;
            }
        }

        if (offsetOnlyChange)
            continue;

        std::sort(res[stage].textures.begin(), res[stage].textures.end(), [](const Stage::Texture &a, const Stage::Texture &b) {
            return a.nativeBinding < b.nativeBinding;
        });

        std::sort(res[stage].samplers.begin(), res[stage].samplers.end(), [](const Stage::Sampler &a, const Stage::Sampler &b) {
            return a.nativeBinding < b.nativeBinding;
        });

        for (const Stage::Texture &t : qAsConst(res[stage].textures))
            res[stage].textureBatches.feed(t.nativeBinding, t.mtltex);

        for (const Stage::Sampler &s : qAsConst(res[stage].samplers))
            res[stage].samplerBatches.feed(s.nativeBinding, s.mtlsampler);

        res[stage].textureBatches.finish();
        res[stage].samplerBatches.finish();

        for (int i = 0, ie = res[stage].textureBatches.batches.count(); i != ie; ++i) {
            const auto &batch(res[stage].textureBatches.batches[i]);
            switch (stage) {
            case VERTEX:
                [cbD->d->currentRenderPassEncoder setVertexTextures: batch.resources.constData()
                  withRange: NSMakeRange(batch.startBinding, NSUInteger(batch.resources.count()))];
                break;
            case FRAGMENT:
                [cbD->d->currentRenderPassEncoder setFragmentTextures: batch.resources.constData()
                  withRange: NSMakeRange(batch.startBinding, NSUInteger(batch.resources.count()))];
                break;
            case COMPUTE:
                [cbD->d->currentComputePassEncoder setTextures: batch.resources.constData()
                  withRange: NSMakeRange(batch.startBinding, NSUInteger(batch.resources.count()))];
                break;
            default:
                Q_UNREACHABLE();
                break;
            }
        }
        for (int i = 0, ie = res[stage].samplerBatches.batches.count(); i != ie; ++i) {
            const auto &batch(res[stage].samplerBatches.batches[i]);
            switch (stage) {
            case VERTEX:
                [cbD->d->currentRenderPassEncoder setVertexSamplerStates: batch.resources.constData()
                  withRange: NSMakeRange(batch.startBinding, NSUInteger(batch.resources.count()))];
                break;
            case FRAGMENT:
                [cbD->d->currentRenderPassEncoder setFragmentSamplerStates: batch.resources.constData()
                  withRange: NSMakeRange(batch.startBinding, NSUInteger(batch.resources.count()))];
                break;
            case COMPUTE:
                [cbD->d->currentComputePassEncoder setSamplerStates: batch.resources.constData()
                  withRange: NSMakeRange(batch.startBinding, NSUInteger(batch.resources.count()))];
                break;
            default:
                Q_UNREACHABLE();
                break;
            }
        }
    }
}

void QRhiMetal::setGraphicsPipeline(QRhiCommandBuffer *cb, QRhiGraphicsPipeline *ps)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::RenderPass);
    QMetalGraphicsPipeline *psD = QRHI_RES(QMetalGraphicsPipeline, ps);

    if (cbD->currentGraphicsPipeline != ps || cbD->currentPipelineGeneration != psD->generation) {
        cbD->currentGraphicsPipeline = ps;
        cbD->currentComputePipeline = nullptr;
        cbD->currentPipelineGeneration = psD->generation;

        [cbD->d->currentRenderPassEncoder setRenderPipelineState: psD->d->ps];
        [cbD->d->currentRenderPassEncoder setDepthStencilState: psD->d->ds];

        if (cbD->currentCullMode == -1 || psD->d->cullMode != uint(cbD->currentCullMode)) {
            [cbD->d->currentRenderPassEncoder setCullMode: psD->d->cullMode];
            cbD->currentCullMode = int(psD->d->cullMode);
        }
        if (cbD->currentFrontFaceWinding == -1 || psD->d->winding != uint(cbD->currentFrontFaceWinding)) {
            [cbD->d->currentRenderPassEncoder setFrontFacingWinding: psD->d->winding];
            cbD->currentFrontFaceWinding = int(psD->d->winding);
        }
        if (!qFuzzyCompare(psD->d->depthBias, cbD->currentDepthBiasValues.first)
                || !qFuzzyCompare(psD->d->slopeScaledDepthBias, cbD->currentDepthBiasValues.second))
        {
            [cbD->d->currentRenderPassEncoder setDepthBias: psD->d->depthBias
                                                            slopeScale: psD->d->slopeScaledDepthBias
                                                            clamp: 0.0f];
            cbD->currentDepthBiasValues = { psD->d->depthBias, psD->d->slopeScaledDepthBias };
        }
    }

    psD->lastActiveFrameSlot = currentFrameSlot;
}

void QRhiMetal::setShaderResources(QRhiCommandBuffer *cb, QRhiShaderResourceBindings *srb,
                                   int dynamicOffsetCount,
                                   const QRhiCommandBuffer::DynamicOffset *dynamicOffsets)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass != QMetalCommandBuffer::NoPass);
    QMetalGraphicsPipeline *gfxPsD = QRHI_RES(QMetalGraphicsPipeline, cbD->currentGraphicsPipeline);
    QMetalComputePipeline *compPsD = QRHI_RES(QMetalComputePipeline, cbD->currentComputePipeline);

    if (!srb) {
        if (gfxPsD)
            srb = gfxPsD->m_shaderResourceBindings;
        else
            srb = compPsD->m_shaderResourceBindings;
    }

    QMetalShaderResourceBindings *srbD = QRHI_RES(QMetalShaderResourceBindings, srb);
    bool hasSlottedResourceInSrb = false;
    bool hasDynamicOffsetInSrb = false;
    bool resNeedsRebind = false;

    // do buffer writes, figure out if we need to rebind, and mark as in-use
    for (int i = 0, ie = srbD->sortedBindings.count(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = srbD->sortedBindings.at(i).data();
        QMetalShaderResourceBindings::BoundResourceData &bd(srbD->boundResourceData[i]);
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            QMetalBuffer *bufD = QRHI_RES(QMetalBuffer, b->u.ubuf.buf);
            Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::UniformBuffer));
            executeBufferHostWritesForCurrentFrame(bufD);
            if (bufD->d->slotted)
                hasSlottedResourceInSrb = true;
            if (b->u.ubuf.hasDynamicOffset)
                hasDynamicOffsetInSrb = true;
            if (bufD->generation != bd.ubuf.generation || bufD->m_id != bd.ubuf.id) {
                resNeedsRebind = true;
                bd.ubuf.id = bufD->m_id;
                bd.ubuf.generation = bufD->generation;
            }
            bufD->lastActiveFrameSlot = currentFrameSlot;
        }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
        {
            const QRhiShaderResourceBinding::Data::SampledTextureData *data = &b->u.stex;
            if (bd.stex.count != data->count) {
                bd.stex.count = data->count;
                resNeedsRebind = true;
            }
            for (int elem = 0; elem < data->count; ++elem) {
                QMetalTexture *texD = QRHI_RES(QMetalTexture, data->texSamplers[elem].tex);
                QMetalSampler *samplerD = QRHI_RES(QMetalSampler, data->texSamplers[elem].sampler);
                if (texD->generation != bd.stex.d[elem].texGeneration
                        || texD->m_id != bd.stex.d[elem].texId
                        || samplerD->generation != bd.stex.d[elem].samplerGeneration
                        || samplerD->m_id != bd.stex.d[elem].samplerId)
                {
                    resNeedsRebind = true;
                    bd.stex.d[elem].texId = texD->m_id;
                    bd.stex.d[elem].texGeneration = texD->generation;
                    bd.stex.d[elem].samplerId = samplerD->m_id;
                    bd.stex.d[elem].samplerGeneration = samplerD->generation;
                }
                texD->lastActiveFrameSlot = currentFrameSlot;
                samplerD->lastActiveFrameSlot = currentFrameSlot;
            }
        }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
        {
            QMetalTexture *texD = QRHI_RES(QMetalTexture, b->u.simage.tex);
            if (texD->generation != bd.simage.generation || texD->m_id != bd.simage.id) {
                resNeedsRebind = true;
                bd.simage.id = texD->m_id;
                bd.simage.generation = texD->generation;
            }
            texD->lastActiveFrameSlot = currentFrameSlot;
        }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
        case QRhiShaderResourceBinding::BufferStore:
        case QRhiShaderResourceBinding::BufferLoadStore:
        {
            QMetalBuffer *bufD = QRHI_RES(QMetalBuffer, b->u.sbuf.buf);
            Q_ASSERT(bufD->m_usage.testFlag(QRhiBuffer::StorageBuffer));
            executeBufferHostWritesForCurrentFrame(bufD);
            if (bufD->generation != bd.sbuf.generation || bufD->m_id != bd.sbuf.id) {
                resNeedsRebind = true;
                bd.sbuf.id = bufD->m_id;
                bd.sbuf.generation = bufD->generation;
            }
            bufD->lastActiveFrameSlot = currentFrameSlot;
        }
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    // make sure the resources for the correct slot get bound
    const int resSlot = hasSlottedResourceInSrb ? currentFrameSlot : 0;
    if (hasSlottedResourceInSrb && cbD->currentResSlot != resSlot)
        resNeedsRebind = true;

    const bool srbChanged = gfxPsD ? (cbD->currentGraphicsSrb != srb) : (cbD->currentComputeSrb != srb);
    const bool srbRebuilt = cbD->currentSrbGeneration != srbD->generation;

    // dynamic uniform buffer offsets always trigger a rebind
    if (hasDynamicOffsetInSrb || resNeedsRebind || srbChanged || srbRebuilt) {
        const QShader::NativeResourceBindingMap *resBindMaps[SUPPORTED_STAGES] = { nullptr, nullptr, nullptr };
        if (gfxPsD) {
            cbD->currentGraphicsSrb = srb;
            cbD->currentComputeSrb = nullptr;
            resBindMaps[0] = &gfxPsD->d->vs.nativeResourceBindingMap;
            resBindMaps[1] = &gfxPsD->d->fs.nativeResourceBindingMap;
        } else {
            cbD->currentGraphicsSrb = nullptr;
            cbD->currentComputeSrb = srb;
            resBindMaps[2] = &compPsD->d->cs.nativeResourceBindingMap;
        }
        cbD->currentSrbGeneration = srbD->generation;
        cbD->currentResSlot = resSlot;

        const bool offsetOnlyChange = hasDynamicOffsetInSrb && !resNeedsRebind && !srbChanged && !srbRebuilt;
        enqueueShaderResourceBindings(srbD, cbD, dynamicOffsetCount, dynamicOffsets, offsetOnlyChange, resBindMaps);
    }
}

void QRhiMetal::setVertexInput(QRhiCommandBuffer *cb,
                               int startBinding, int bindingCount, const QRhiCommandBuffer::VertexInput *bindings,
                               QRhiBuffer *indexBuf, quint32 indexOffset, QRhiCommandBuffer::IndexFormat indexFormat)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::RenderPass);

    QRhiBatchedBindings<id<MTLBuffer> > buffers;
    QRhiBatchedBindings<NSUInteger> offsets;
    for (int i = 0; i < bindingCount; ++i) {
        QMetalBuffer *bufD = QRHI_RES(QMetalBuffer, bindings[i].first);
        executeBufferHostWritesForCurrentFrame(bufD);
        bufD->lastActiveFrameSlot = currentFrameSlot;
        id<MTLBuffer> mtlbuf = bufD->d->buf[bufD->d->slotted ? currentFrameSlot : 0];
        buffers.feed(startBinding + i, mtlbuf);
        offsets.feed(startBinding + i, bindings[i].second);
    }
    buffers.finish();
    offsets.finish();

    // same binding space for vertex and constant buffers - work it around
    QRhiShaderResourceBindings *srb = cbD->currentGraphicsSrb;
    // There's nothing guaranteeing setShaderResources() was called before
    // setVertexInput()... but whatever srb will get bound will have to be
    // layout-compatible anyways so maxBinding is the same.
    if (!srb)
        srb = cbD->currentGraphicsPipeline->shaderResourceBindings();
    const int firstVertexBinding = QRHI_RES(QMetalShaderResourceBindings, srb)->maxBinding + 1;

    if (firstVertexBinding != cbD->d->currentFirstVertexBinding
            || buffers != cbD->d->currentVertexInputsBuffers
            || offsets != cbD->d->currentVertexInputOffsets)
    {
        cbD->d->currentFirstVertexBinding = firstVertexBinding;
        cbD->d->currentVertexInputsBuffers = buffers;
        cbD->d->currentVertexInputOffsets = offsets;

        for (int i = 0, ie = buffers.batches.count(); i != ie; ++i) {
            const auto &bufferBatch(buffers.batches[i]);
            const auto &offsetBatch(offsets.batches[i]);
            [cbD->d->currentRenderPassEncoder setVertexBuffers:
                bufferBatch.resources.constData()
              offsets: offsetBatch.resources.constData()
              withRange: NSMakeRange(uint(firstVertexBinding) + bufferBatch.startBinding, NSUInteger(bufferBatch.resources.count()))];
        }
    }

    if (indexBuf) {
        QMetalBuffer *ibufD = QRHI_RES(QMetalBuffer, indexBuf);
        executeBufferHostWritesForCurrentFrame(ibufD);
        ibufD->lastActiveFrameSlot = currentFrameSlot;
        cbD->currentIndexBuffer = indexBuf;
        cbD->currentIndexOffset = indexOffset;
        cbD->currentIndexFormat = indexFormat;
    } else {
        cbD->currentIndexBuffer = nullptr;
    }
}

void QRhiMetal::setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::RenderPass);
    const QSize outputSize = cbD->currentTarget->pixelSize();

    // x,y is top-left in MTLViewportRect but bottom-left in QRhiViewport
    float x, y, w, h;
    if (!qrhi_toTopLeftRenderTargetRect(outputSize, viewport.viewport(), &x, &y, &w, &h))
        return;

    MTLViewport vp;
    vp.originX = double(x);
    vp.originY = double(y);
    vp.width = double(w);
    vp.height = double(h);
    vp.znear = double(viewport.minDepth());
    vp.zfar = double(viewport.maxDepth());

    [cbD->d->currentRenderPassEncoder setViewport: vp];

    if (!QRHI_RES(QMetalGraphicsPipeline, cbD->currentGraphicsPipeline)->m_flags.testFlag(QRhiGraphicsPipeline::UsesScissor)) {
        MTLScissorRect s;
        s.x = NSUInteger(x);
        s.y = NSUInteger(y);
        s.width = NSUInteger(w);
        s.height = NSUInteger(h);
        [cbD->d->currentRenderPassEncoder setScissorRect: s];
    }
}

void QRhiMetal::setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::RenderPass);
    Q_ASSERT(QRHI_RES(QMetalGraphicsPipeline, cbD->currentGraphicsPipeline)->m_flags.testFlag(QRhiGraphicsPipeline::UsesScissor));
    const QSize outputSize = cbD->currentTarget->pixelSize();

    // x,y is top-left in MTLScissorRect but bottom-left in QRhiScissor
    int x, y, w, h;
    if (!qrhi_toTopLeftRenderTargetRect(outputSize, scissor.scissor(), &x, &y, &w, &h))
        return;

    MTLScissorRect s;
    s.x = NSUInteger(x);
    s.y = NSUInteger(y);
    s.width = NSUInteger(w);
    s.height = NSUInteger(h);

    [cbD->d->currentRenderPassEncoder setScissorRect: s];
}

void QRhiMetal::setBlendConstants(QRhiCommandBuffer *cb, const QColor &c)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::RenderPass);

    [cbD->d->currentRenderPassEncoder setBlendColorRed: float(c.redF())
      green: float(c.greenF()) blue: float(c.blueF()) alpha: float(c.alphaF())];
}

void QRhiMetal::setStencilRef(QRhiCommandBuffer *cb, quint32 refValue)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::RenderPass);

    [cbD->d->currentRenderPassEncoder setStencilReferenceValue: refValue];
}

void QRhiMetal::draw(QRhiCommandBuffer *cb, quint32 vertexCount,
                     quint32 instanceCount, quint32 firstVertex, quint32 firstInstance)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::RenderPass);

    [cbD->d->currentRenderPassEncoder drawPrimitives:
        QRHI_RES(QMetalGraphicsPipeline, cbD->currentGraphicsPipeline)->d->primitiveType
      vertexStart: firstVertex vertexCount: vertexCount instanceCount: instanceCount baseInstance: firstInstance];
}

void QRhiMetal::drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                            quint32 instanceCount, quint32 firstIndex, qint32 vertexOffset, quint32 firstInstance)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::RenderPass);

    if (!cbD->currentIndexBuffer)
        return;

    const quint32 indexOffset = cbD->currentIndexOffset + firstIndex * (cbD->currentIndexFormat == QRhiCommandBuffer::IndexUInt16 ? 2 : 4);
    Q_ASSERT(indexOffset == aligned<quint32>(indexOffset, 4));

    QMetalBuffer *ibufD = QRHI_RES(QMetalBuffer, cbD->currentIndexBuffer);
    id<MTLBuffer> mtlbuf = ibufD->d->buf[ibufD->d->slotted ? currentFrameSlot : 0];

    [cbD->d->currentRenderPassEncoder drawIndexedPrimitives: QRHI_RES(QMetalGraphicsPipeline, cbD->currentGraphicsPipeline)->d->primitiveType
      indexCount: indexCount
      indexType: cbD->currentIndexFormat == QRhiCommandBuffer::IndexUInt16 ? MTLIndexTypeUInt16 : MTLIndexTypeUInt32
      indexBuffer: mtlbuf
      indexBufferOffset: indexOffset
      instanceCount: instanceCount
      baseVertex: vertexOffset
      baseInstance: firstInstance];
}

void QRhiMetal::debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name)
{
    if (!debugMarkers)
        return;

    NSString *str = [NSString stringWithUTF8String: name.constData()];
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    if (cbD->recordingPass != QMetalCommandBuffer::NoPass) {
        [cbD->d->currentRenderPassEncoder pushDebugGroup: str];
    } else {
        if (@available(macOS 10.13, iOS 11.0, *))
            [cbD->d->cb pushDebugGroup: str];
    }
}

void QRhiMetal::debugMarkEnd(QRhiCommandBuffer *cb)
{
    if (!debugMarkers)
        return;

    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    if (cbD->recordingPass != QMetalCommandBuffer::NoPass) {
        [cbD->d->currentRenderPassEncoder popDebugGroup];
    } else {
        if (@available(macOS 10.13, iOS 11.0, *))
            [cbD->d->cb popDebugGroup];
    }
}

void QRhiMetal::debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg)
{
    if (!debugMarkers)
        return;

    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    if (cbD->recordingPass != QMetalCommandBuffer::NoPass)
        [cbD->d->currentRenderPassEncoder insertDebugSignpost: [NSString stringWithUTF8String: msg.constData()]];
}

const QRhiNativeHandles *QRhiMetal::nativeHandles(QRhiCommandBuffer *cb)
{
    return QRHI_RES(QMetalCommandBuffer, cb)->nativeHandles();
}

void QRhiMetal::beginExternal(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}

void QRhiMetal::endExternal(QRhiCommandBuffer *cb)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    cbD->resetPerPassCachedState();
}

QRhi::FrameOpResult QRhiMetal::beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);

    QMetalSwapChain *swapChainD = QRHI_RES(QMetalSwapChain, swapChain);

    // This is a bit messed up since for this swapchain we want to wait for the
    // commands+present to complete, while for others just for the commands
    // (for this same frame slot) but not sure how to do that in a sane way so
    // wait for full cb completion for now.
    for (QMetalSwapChain *sc : qAsConst(swapchains)) {
        dispatch_semaphore_t sem = sc->d->sem[swapChainD->currentFrameSlot];
        dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
        if (sc != swapChainD)
            dispatch_semaphore_signal(sem);
    }

    currentSwapChain = swapChainD;
    currentFrameSlot = swapChainD->currentFrameSlot;
    if (swapChainD->ds)
        swapChainD->ds->lastActiveFrameSlot = currentFrameSlot;

    if (@available(macOS 10.13, iOS 11.0, *))
        [d->captureScope beginScope];

    // Do not let the command buffer mess with the refcount of objects. We do
    // have a proper render loop and will manage lifetimes similarly to other
    // backends (Vulkan).
    swapChainD->cbWrapper.d->cb = [d->cmdQueue commandBufferWithUnretainedReferences];

    QMetalRenderTargetData::ColorAtt colorAtt;
    if (swapChainD->samples > 1) {
        colorAtt.tex = swapChainD->d->msaaTex[currentFrameSlot];
        colorAtt.needsDrawableForResolveTex = true;
    } else {
        colorAtt.needsDrawableForTex = true;
    }

    swapChainD->rtWrapper.d->fb.colorAtt[0] = colorAtt;
    swapChainD->rtWrapper.d->fb.dsTex = swapChainD->ds ? swapChainD->ds->d->tex : nil;
    swapChainD->rtWrapper.d->fb.hasStencil = swapChainD->ds ? true : false;
    swapChainD->rtWrapper.d->fb.depthNeedsStore = false;

    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();
    QRHI_PROF_F(beginSwapChainFrame(swapChain));

    executeDeferredReleases();
    swapChainD->cbWrapper.resetState();
    finishActiveReadbacks();

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiMetal::endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags)
{
    QMetalSwapChain *swapChainD = QRHI_RES(QMetalSwapChain, swapChain);
    Q_ASSERT(currentSwapChain == swapChainD);

    const bool needsPresent = !flags.testFlag(QRhi::SkipPresent);
    if (needsPresent)
        [swapChainD->cbWrapper.d->cb presentDrawable: swapChainD->d->curDrawable];

    // Must not hold on to the drawable, regardless of needsPresent.
    // (internally it is autoreleased or something, it seems)
    swapChainD->d->curDrawable = nil;

    __block int thisFrameSlot = currentFrameSlot;
    [swapChainD->cbWrapper.d->cb addCompletedHandler: ^(id<MTLCommandBuffer>) {
        dispatch_semaphore_signal(swapChainD->d->sem[thisFrameSlot]);
    }];

    [swapChainD->cbWrapper.d->cb commit];

    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();
    QRHI_PROF_F(endSwapChainFrame(swapChain, swapChainD->frameCount + 1));

    if (@available(macOS 10.13, iOS 11.0, *))
        [d->captureScope endScope];

    if (needsPresent)
        swapChainD->currentFrameSlot = (swapChainD->currentFrameSlot + 1) % QMTL_FRAMES_IN_FLIGHT;

    swapChainD->frameCount += 1;
    currentSwapChain = nullptr;
    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiMetal::beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);

    currentFrameSlot = (currentFrameSlot + 1) % QMTL_FRAMES_IN_FLIGHT;
    if (swapchains.count() > 1) {
        for (QMetalSwapChain *sc : qAsConst(swapchains)) {
            // wait+signal is the general pattern to ensure the commands for a
            // given frame slot have completed (if sem is 1, we go 0 then 1; if
            // sem is 0 we go -1, block, completion increments to 0, then us to 1)
            dispatch_semaphore_t sem = sc->d->sem[currentFrameSlot];
            dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
            dispatch_semaphore_signal(sem);
        }
    }

    d->ofr.active = true;
    *cb = &d->ofr.cbWrapper;
    d->ofr.cbWrapper.d->cb = [d->cmdQueue commandBufferWithUnretainedReferences];

    executeDeferredReleases();
    d->ofr.cbWrapper.resetState();
    finishActiveReadbacks();

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiMetal::endOffscreenFrame(QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    Q_ASSERT(d->ofr.active);
    d->ofr.active = false;

    [d->ofr.cbWrapper.d->cb commit];

    // offscreen frames wait for completion, unlike swapchain ones
    [d->ofr.cbWrapper.d->cb waitUntilCompleted];

    finishActiveReadbacks(true);

    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiMetal::finish()
{
    id<MTLCommandBuffer> cb = nil;
    QMetalSwapChain *swapChainD = nullptr;
    if (inFrame) {
        if (d->ofr.active) {
            Q_ASSERT(!currentSwapChain);
            Q_ASSERT(d->ofr.cbWrapper.recordingPass == QMetalCommandBuffer::NoPass);
            cb = d->ofr.cbWrapper.d->cb;
        } else {
            Q_ASSERT(currentSwapChain);
            swapChainD = currentSwapChain;
            Q_ASSERT(swapChainD->cbWrapper.recordingPass == QMetalCommandBuffer::NoPass);
            cb = swapChainD->cbWrapper.d->cb;
        }
    }

    for (QMetalSwapChain *sc : qAsConst(swapchains)) {
        for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i) {
            if (currentSwapChain && sc == currentSwapChain && i == currentFrameSlot) {
                // no wait as this is the thing we're going to be commit below and
                // beginFrame decremented sem already and going to be signaled by endFrame
                continue;
            }
            dispatch_semaphore_t sem = sc->d->sem[i];
            dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
            dispatch_semaphore_signal(sem);
        }
    }

    if (cb) {
        [cb commit];
        [cb waitUntilCompleted];
    }

    if (inFrame) {
        if (d->ofr.active)
            d->ofr.cbWrapper.d->cb = [d->cmdQueue commandBufferWithUnretainedReferences];
        else
            swapChainD->cbWrapper.d->cb = [d->cmdQueue commandBufferWithUnretainedReferences];
    }

    executeDeferredReleases(true);

    finishActiveReadbacks(true);

    return QRhi::FrameOpSuccess;
}

MTLRenderPassDescriptor *QRhiMetalData::createDefaultRenderPass(bool hasDepthStencil,
                                                                const QColor &colorClearValue,
                                                                const QRhiDepthStencilClearValue &depthStencilClearValue,
                                                                int colorAttCount)
{
    MTLRenderPassDescriptor *rp = [MTLRenderPassDescriptor renderPassDescriptor];
    MTLClearColor c = MTLClearColorMake(colorClearValue.redF(), colorClearValue.greenF(), colorClearValue.blueF(),
                                        colorClearValue.alphaF());

    for (uint i = 0; i < uint(colorAttCount); ++i) {
        rp.colorAttachments[i].loadAction = MTLLoadActionClear;
        rp.colorAttachments[i].storeAction = MTLStoreActionStore;
        rp.colorAttachments[i].clearColor = c;
    }

    if (hasDepthStencil) {
        rp.depthAttachment.loadAction = MTLLoadActionClear;
        rp.depthAttachment.storeAction = MTLStoreActionDontCare;
        rp.stencilAttachment.loadAction = MTLLoadActionClear;
        rp.stencilAttachment.storeAction = MTLStoreActionDontCare;
        rp.depthAttachment.clearDepth = double(depthStencilClearValue.depthClearValue());
        rp.stencilAttachment.clearStencil = depthStencilClearValue.stencilClearValue();
    }

    return rp;
}

qsizetype QRhiMetal::subresUploadByteSize(const QRhiTextureSubresourceUploadDescription &subresDesc) const
{
    qsizetype size = 0;
    const qsizetype imageSizeBytes = subresDesc.image().isNull() ?
                subresDesc.data().size() : subresDesc.image().sizeInBytes();
    if (imageSizeBytes > 0)
        size += aligned<qsizetype>(imageSizeBytes, QRhiMetalData::TEXBUF_ALIGN);
    return size;
}

void QRhiMetal::enqueueSubresUpload(QMetalTexture *texD, void *mp, void *blitEncPtr,
                                    int layer, int level, const QRhiTextureSubresourceUploadDescription &subresDesc,
                                    qsizetype *curOfs)
{
    const QPoint dp = subresDesc.destinationTopLeft();
    const QByteArray rawData = subresDesc.data();
    QImage img = subresDesc.image();
    id<MTLBlitCommandEncoder> blitEnc = (id<MTLBlitCommandEncoder>) blitEncPtr;

    if (!img.isNull()) {
        const qsizetype fullImageSizeBytes = img.sizeInBytes();
        int w = img.width();
        int h = img.height();
        int bpl = img.bytesPerLine();
        int srcOffset = 0;

        if (!subresDesc.sourceSize().isEmpty() || !subresDesc.sourceTopLeft().isNull()) {
            const int sx = subresDesc.sourceTopLeft().x();
            const int sy = subresDesc.sourceTopLeft().y();
            if (!subresDesc.sourceSize().isEmpty()) {
                w = subresDesc.sourceSize().width();
                h = subresDesc.sourceSize().height();
            }
            if (img.depth() == 32) {
                memcpy(reinterpret_cast<char *>(mp) + *curOfs, img.constBits(), size_t(fullImageSizeBytes));
                srcOffset = sy * bpl + sx * 4;
                // bpl remains set to the original image's row stride
            } else {
                img = img.copy(sx, sy, w, h);
                bpl = img.bytesPerLine();
                Q_ASSERT(img.sizeInBytes() <= fullImageSizeBytes);
                memcpy(reinterpret_cast<char *>(mp) + *curOfs, img.constBits(), size_t(img.sizeInBytes()));
            }
        } else {
            memcpy(reinterpret_cast<char *>(mp) + *curOfs, img.constBits(), size_t(fullImageSizeBytes));
        }

        [blitEnc copyFromBuffer: texD->d->stagingBuf[currentFrameSlot]
                                 sourceOffset: NSUInteger(*curOfs + srcOffset)
                                 sourceBytesPerRow: NSUInteger(bpl)
                                 sourceBytesPerImage: 0
                                 sourceSize: MTLSizeMake(NSUInteger(w), NSUInteger(h), 1)
          toTexture: texD->d->tex
          destinationSlice: NSUInteger(layer)
          destinationLevel: NSUInteger(level)
          destinationOrigin: MTLOriginMake(NSUInteger(dp.x()), NSUInteger(dp.y()), 0)
          options: MTLBlitOptionNone];

        *curOfs += aligned<qsizetype>(fullImageSizeBytes, QRhiMetalData::TEXBUF_ALIGN);
    } else if (!rawData.isEmpty() && isCompressedFormat(texD->m_format)) {
        const QSize subresSize = q->sizeForMipLevel(level, texD->m_pixelSize);
        const int subresw = subresSize.width();
        const int subresh = subresSize.height();
        int w, h;
        if (subresDesc.sourceSize().isEmpty()) {
            w = subresw;
            h = subresh;
        } else {
            w = subresDesc.sourceSize().width();
            h = subresDesc.sourceSize().height();
        }

        quint32 bpl = 0;
        QSize blockDim;
        compressedFormatInfo(texD->m_format, QSize(w, h), &bpl, nullptr, &blockDim);

        const int dx = aligned(dp.x(), blockDim.width());
        const int dy = aligned(dp.y(), blockDim.height());
        if (dx + w != subresw)
            w = aligned(w, blockDim.width());
        if (dy + h != subresh)
            h = aligned(h, blockDim.height());

        memcpy(reinterpret_cast<char *>(mp) + *curOfs, rawData.constData(), size_t(rawData.size()));

        [blitEnc copyFromBuffer: texD->d->stagingBuf[currentFrameSlot]
                                 sourceOffset: NSUInteger(*curOfs)
                                 sourceBytesPerRow: bpl
                                 sourceBytesPerImage: 0
                                 sourceSize: MTLSizeMake(NSUInteger(w), NSUInteger(h), 1)
          toTexture: texD->d->tex
          destinationSlice: NSUInteger(layer)
          destinationLevel: NSUInteger(level)
          destinationOrigin: MTLOriginMake(NSUInteger(dx), NSUInteger(dy), 0)
          options: MTLBlitOptionNone];

        *curOfs += aligned(rawData.size(), QRhiMetalData::TEXBUF_ALIGN);
    } else if (!rawData.isEmpty()) {
        const QSize subresSize = q->sizeForMipLevel(level, texD->m_pixelSize);
        const int subresw = subresSize.width();
        const int subresh = subresSize.height();
        int w, h;
        if (subresDesc.sourceSize().isEmpty()) {
            w = subresw;
            h = subresh;
        } else {
            w = subresDesc.sourceSize().width();
            h = subresDesc.sourceSize().height();
        }

        quint32 bpl = 0;
        textureFormatInfo(texD->m_format, QSize(w, h), &bpl, nullptr);
        memcpy(reinterpret_cast<char *>(mp) + *curOfs, rawData.constData(), size_t(rawData.size()));

        [blitEnc copyFromBuffer: texD->d->stagingBuf[currentFrameSlot]
                                 sourceOffset: NSUInteger(*curOfs)
                                 sourceBytesPerRow: bpl
                                 sourceBytesPerImage: 0
                                 sourceSize: MTLSizeMake(NSUInteger(w), NSUInteger(h), 1)
          toTexture: texD->d->tex
          destinationSlice: NSUInteger(layer)
          destinationLevel: NSUInteger(level)
          destinationOrigin: MTLOriginMake(NSUInteger(dp.x()), NSUInteger(dp.y()), 0)
          options: MTLBlitOptionNone];

        *curOfs += aligned(rawData.size(), QRhiMetalData::TEXBUF_ALIGN);
    } else {
        qWarning("Invalid texture upload for %p layer=%d mip=%d", texD, layer, level);
    }
}

void QRhiMetal::enqueueResourceUpdates(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    QRhiResourceUpdateBatchPrivate *ud = QRhiResourceUpdateBatchPrivate::get(resourceUpdates);
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();

    for (const QRhiResourceUpdateBatchPrivate::BufferOp &u : ud->bufferOps) {
        if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::DynamicUpdate) {
            QMetalBuffer *bufD = QRHI_RES(QMetalBuffer, u.buf);
            Q_ASSERT(bufD->m_type == QRhiBuffer::Dynamic);
            for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i)
                bufD->d->pendingUpdates[i].append(u);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::StaticUpload) {
            // Due to the Metal API the handling of static and dynamic buffers is
            // basically the same. So go through the same pendingUpdates machinery.
            QMetalBuffer *bufD = QRHI_RES(QMetalBuffer, u.buf);
            Q_ASSERT(bufD->m_type != QRhiBuffer::Dynamic);
            Q_ASSERT(u.offset + u.data.size() <= bufD->m_size);
            for (int i = 0, ie = bufD->d->slotted ? QMTL_FRAMES_IN_FLIGHT : 1; i != ie; ++i)
                bufD->d->pendingUpdates[i].append(
                            QRhiResourceUpdateBatchPrivate::BufferOp::dynamicUpdate(u.buf, u.offset, u.data.size(), u.data.constData()));
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::Read) {
            QMetalBuffer *bufD = QRHI_RES(QMetalBuffer, u.buf);
            executeBufferHostWritesForCurrentFrame(bufD);
            const int idx = bufD->d->slotted ? currentFrameSlot : 0;
            char *p = reinterpret_cast<char *>([bufD->d->buf[idx] contents]);
            if (p) {
                u.result->data.resize(u.readSize);
                memcpy(u.result->data.data(), p + u.offset, size_t(u.readSize));
            }
            if (u.result->completed)
                u.result->completed();
        }
    }

    id<MTLBlitCommandEncoder> blitEnc = nil;
    auto ensureBlit = [&blitEnc, cbD, this] {
        if (!blitEnc) {
            blitEnc = [cbD->d->cb blitCommandEncoder];
            if (debugMarkers)
                [blitEnc pushDebugGroup: @"Texture upload/copy"];
        }
    };

    for (const QRhiResourceUpdateBatchPrivate::TextureOp &u : ud->textureOps) {
        if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Upload) {
            QMetalTexture *utexD = QRHI_RES(QMetalTexture, u.dst);
            qsizetype stagingSize = 0;
            for (int layer = 0; layer < QRhi::MAX_LAYERS; ++layer) {
                for (int level = 0; level < QRhi::MAX_LEVELS; ++level) {
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : qAsConst(u.subresDesc[layer][level]))
                        stagingSize += subresUploadByteSize(subresDesc);
                }
            }

            ensureBlit();
            Q_ASSERT(!utexD->d->stagingBuf[currentFrameSlot]);
            utexD->d->stagingBuf[currentFrameSlot] = [d->dev newBufferWithLength: NSUInteger(stagingSize)
                        options: MTLResourceStorageModeShared];
            QRHI_PROF_F(newTextureStagingArea(utexD, currentFrameSlot, quint32(stagingSize)));

            void *mp = [utexD->d->stagingBuf[currentFrameSlot] contents];
            qsizetype curOfs = 0;
            for (int layer = 0; layer < QRhi::MAX_LAYERS; ++layer) {
                for (int level = 0; level < QRhi::MAX_LEVELS; ++level) {
                    for (const QRhiTextureSubresourceUploadDescription &subresDesc : qAsConst(u.subresDesc[layer][level]))
                        enqueueSubresUpload(utexD, mp, blitEnc, layer, level, subresDesc, &curOfs);
                }
            }

            utexD->lastActiveFrameSlot = currentFrameSlot;

            QRhiMetalData::DeferredReleaseEntry e;
            e.type = QRhiMetalData::DeferredReleaseEntry::StagingBuffer;
            e.lastActiveFrameSlot = currentFrameSlot;
            e.stagingBuffer.buffer = utexD->d->stagingBuf[currentFrameSlot];
            utexD->d->stagingBuf[currentFrameSlot] = nil;
            d->releaseQueue.append(e);
            QRHI_PROF_F(releaseTextureStagingArea(utexD, currentFrameSlot));
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Copy) {
            Q_ASSERT(u.src && u.dst);
            QMetalTexture *srcD = QRHI_RES(QMetalTexture, u.src);
            QMetalTexture *dstD = QRHI_RES(QMetalTexture, u.dst);
            const QPoint dp = u.desc.destinationTopLeft();
            const QSize mipSize = q->sizeForMipLevel(u.desc.sourceLevel(), srcD->m_pixelSize);
            const QSize copySize = u.desc.pixelSize().isEmpty() ? mipSize : u.desc.pixelSize();
            const QPoint sp = u.desc.sourceTopLeft();

            ensureBlit();
            [blitEnc copyFromTexture: srcD->d->tex
                                      sourceSlice: NSUInteger(u.desc.sourceLayer())
                                      sourceLevel: NSUInteger(u.desc.sourceLevel())
                                      sourceOrigin: MTLOriginMake(NSUInteger(sp.x()), NSUInteger(sp.y()), 0)
                                      sourceSize: MTLSizeMake(NSUInteger(copySize.width()), NSUInteger(copySize.height()), 1)
                                      toTexture: dstD->d->tex
                                      destinationSlice: NSUInteger(u.desc.destinationLayer())
                                      destinationLevel: NSUInteger(u.desc.destinationLevel())
                                      destinationOrigin: MTLOriginMake(NSUInteger(dp.x()), NSUInteger(dp.y()), 0)];

            srcD->lastActiveFrameSlot = dstD->lastActiveFrameSlot = currentFrameSlot;
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Read) {
            QRhiMetalData::TextureReadback readback;
            readback.activeFrameSlot = currentFrameSlot;
            readback.desc = u.rb;
            readback.result = u.result;

            QMetalTexture *texD = QRHI_RES(QMetalTexture, u.rb.texture());
            QMetalSwapChain *swapChainD = nullptr;
            id<MTLTexture> src;
            QSize srcSize;
            if (texD) {
                if (texD->samples > 1) {
                    qWarning("Multisample texture cannot be read back");
                    continue;
                }
                readback.pixelSize = q->sizeForMipLevel(u.rb.level(), texD->m_pixelSize);
                readback.format = texD->m_format;
                src = texD->d->tex;
                srcSize = readback.pixelSize;
                texD->lastActiveFrameSlot = currentFrameSlot;
            } else {
                Q_ASSERT(currentSwapChain);
                swapChainD = QRHI_RES(QMetalSwapChain, currentSwapChain);
                readback.pixelSize = swapChainD->pixelSize;
                readback.format = swapChainD->d->rhiColorFormat;
                // Multisample swapchains need nothing special since resolving
                // happens when ending a renderpass.
                const QMetalRenderTargetData::ColorAtt &colorAtt(swapChainD->rtWrapper.d->fb.colorAtt[0]);
                src = colorAtt.resolveTex ? colorAtt.resolveTex : colorAtt.tex;
                srcSize = swapChainD->rtWrapper.d->pixelSize;
            }

            quint32 bpl = 0;
            textureFormatInfo(readback.format, readback.pixelSize, &bpl, &readback.bufSize);
            readback.buf = [d->dev newBufferWithLength: readback.bufSize options: MTLResourceStorageModeShared];

            QRHI_PROF_F(newReadbackBuffer(qint64(qintptr(readback.buf)),
                                          texD ? static_cast<QRhiResource *>(texD) : static_cast<QRhiResource *>(swapChainD),
                                          readback.bufSize));

            ensureBlit();
            [blitEnc copyFromTexture: src
                                      sourceSlice: NSUInteger(u.rb.layer())
                                      sourceLevel: NSUInteger(u.rb.level())
                                      sourceOrigin: MTLOriginMake(0, 0, 0)
                                      sourceSize: MTLSizeMake(NSUInteger(srcSize.width()), NSUInteger(srcSize.height()), 1)
                                      toBuffer: readback.buf
                                      destinationOffset: 0
                                      destinationBytesPerRow: bpl
                                      destinationBytesPerImage: 0
                                      options: MTLBlitOptionNone];

            d->activeTextureReadbacks.append(readback);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::GenMips) {
            QMetalTexture *utexD = QRHI_RES(QMetalTexture, u.dst);
            ensureBlit();
            [blitEnc generateMipmapsForTexture: utexD->d->tex];
            utexD->lastActiveFrameSlot = currentFrameSlot;
        }
    }

    if (blitEnc) {
        if (debugMarkers)
            [blitEnc popDebugGroup];
        [blitEnc endEncoding];
    }

    ud->free();
}

// this handles all types of buffers, not just Dynamic
void QRhiMetal::executeBufferHostWritesForSlot(QMetalBuffer *bufD, int slot)
{
    if (bufD->d->pendingUpdates[slot].isEmpty())
        return;

    void *p = [bufD->d->buf[slot] contents];
    int changeBegin = -1;
    int changeEnd = -1;
    for (const QRhiResourceUpdateBatchPrivate::BufferOp &u : qAsConst(bufD->d->pendingUpdates[slot])) {
        Q_ASSERT(bufD == QRHI_RES(QMetalBuffer, u.buf));
        memcpy(static_cast<char *>(p) + u.offset, u.data.constData(), size_t(u.data.size()));
        if (changeBegin == -1 || u.offset < changeBegin)
            changeBegin = u.offset;
        if (changeEnd == -1 || u.offset + u.data.size() > changeEnd)
            changeEnd = u.offset + u.data.size();
    }
#ifdef Q_OS_MACOS
    if (changeBegin >= 0 && bufD->d->managed)
        [bufD->d->buf[slot] didModifyRange: NSMakeRange(NSUInteger(changeBegin), NSUInteger(changeEnd - changeBegin))];
#endif

    bufD->d->pendingUpdates[slot].clear();
}

void QRhiMetal::executeBufferHostWritesForCurrentFrame(QMetalBuffer *bufD)
{
    executeBufferHostWritesForSlot(bufD, bufD->d->slotted ? currentFrameSlot : 0);
}

void QRhiMetal::resourceUpdate(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_ASSERT(QRHI_RES(QMetalCommandBuffer, cb)->recordingPass == QMetalCommandBuffer::NoPass);

    enqueueResourceUpdates(cb, resourceUpdates);
}

void QRhiMetal::beginPass(QRhiCommandBuffer *cb,
                          QRhiRenderTarget *rt,
                          const QColor &colorClearValue,
                          const QRhiDepthStencilClearValue &depthStencilClearValue,
                          QRhiResourceUpdateBatch *resourceUpdates)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);

    QMetalRenderTargetData *rtD = nullptr;
    switch (rt->resourceType()) {
    case QRhiResource::RenderTarget:
        rtD = QRHI_RES(QMetalReferenceRenderTarget, rt)->d;
        cbD->d->currentPassRpDesc = d->createDefaultRenderPass(rtD->dsAttCount, colorClearValue, depthStencilClearValue, rtD->colorAttCount);
        if (rtD->colorAttCount) {
            QMetalRenderTargetData::ColorAtt &color0(rtD->fb.colorAtt[0]);
            if (color0.needsDrawableForTex || color0.needsDrawableForResolveTex) {
                Q_ASSERT(currentSwapChain);
                QMetalSwapChain *swapChainD = QRHI_RES(QMetalSwapChain, currentSwapChain);
                if (!swapChainD->d->curDrawable) {
#ifdef TARGET_IPHONE_SIMULATOR
                    if (@available(ios 13.0, *))
#endif
                        swapChainD->d->curDrawable = [swapChainD->d->layer nextDrawable];
                }
                if (!swapChainD->d->curDrawable) {
                    qWarning("No drawable");
                    return;
                }
                id<MTLTexture> scTex = swapChainD->d->curDrawable.texture;
                if (color0.needsDrawableForTex) {
                    color0.tex = scTex;
                    color0.needsDrawableForTex = false;
                } else {
                    color0.resolveTex = scTex;
                    color0.needsDrawableForResolveTex = false;
                }
            }
        }
        break;
    case QRhiResource::TextureRenderTarget:
    {
        QMetalTextureRenderTarget *rtTex = QRHI_RES(QMetalTextureRenderTarget, rt);
        rtD = rtTex->d;
        cbD->d->currentPassRpDesc = d->createDefaultRenderPass(rtD->dsAttCount, colorClearValue, depthStencilClearValue, rtD->colorAttCount);
        if (rtTex->m_flags.testFlag(QRhiTextureRenderTarget::PreserveColorContents)) {
            for (uint i = 0; i < uint(rtD->colorAttCount); ++i)
                cbD->d->currentPassRpDesc.colorAttachments[i].loadAction = MTLLoadActionLoad;
        }
        if (rtD->dsAttCount && rtTex->m_flags.testFlag(QRhiTextureRenderTarget::PreserveDepthStencilContents)) {
            cbD->d->currentPassRpDesc.depthAttachment.loadAction = MTLLoadActionLoad;
            cbD->d->currentPassRpDesc.stencilAttachment.loadAction = MTLLoadActionLoad;
        }
        for (auto it = rtTex->m_desc.cbeginColorAttachments(), itEnd = rtTex->m_desc.cendColorAttachments();
             it != itEnd; ++it)
        {
            if (it->texture())
                QRHI_RES(QMetalTexture, it->texture())->lastActiveFrameSlot = currentFrameSlot;
            else if (it->renderBuffer())
                QRHI_RES(QMetalRenderBuffer, it->renderBuffer())->lastActiveFrameSlot = currentFrameSlot;
            if (it->resolveTexture())
                QRHI_RES(QMetalTexture, it->resolveTexture())->lastActiveFrameSlot = currentFrameSlot;
        }
        if (rtTex->m_desc.depthStencilBuffer())
            QRHI_RES(QMetalRenderBuffer, rtTex->m_desc.depthStencilBuffer())->lastActiveFrameSlot = currentFrameSlot;
        if (rtTex->m_desc.depthTexture())
            QRHI_RES(QMetalTexture, rtTex->m_desc.depthTexture())->lastActiveFrameSlot = currentFrameSlot;
    }
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    for (uint i = 0; i < uint(rtD->colorAttCount); ++i) {
        cbD->d->currentPassRpDesc.colorAttachments[i].texture = rtD->fb.colorAtt[i].tex;
        cbD->d->currentPassRpDesc.colorAttachments[i].slice = NSUInteger(rtD->fb.colorAtt[i].layer);
        cbD->d->currentPassRpDesc.colorAttachments[i].level = NSUInteger(rtD->fb.colorAtt[i].level);
        if (rtD->fb.colorAtt[i].resolveTex) {
            cbD->d->currentPassRpDesc.colorAttachments[i].storeAction = MTLStoreActionMultisampleResolve;
            cbD->d->currentPassRpDesc.colorAttachments[i].resolveTexture = rtD->fb.colorAtt[i].resolveTex;
            cbD->d->currentPassRpDesc.colorAttachments[i].resolveSlice = NSUInteger(rtD->fb.colorAtt[i].resolveLayer);
            cbD->d->currentPassRpDesc.colorAttachments[i].resolveLevel = NSUInteger(rtD->fb.colorAtt[i].resolveLevel);
        }
    }

    if (rtD->dsAttCount) {
        Q_ASSERT(rtD->fb.dsTex);
        cbD->d->currentPassRpDesc.depthAttachment.texture = rtD->fb.dsTex;
        cbD->d->currentPassRpDesc.stencilAttachment.texture = rtD->fb.hasStencil ? rtD->fb.dsTex : nil;
        if (rtD->fb.depthNeedsStore) // Depth/Stencil is set to DontCare by default, override if  needed
            cbD->d->currentPassRpDesc.depthAttachment.storeAction = MTLStoreActionStore;
    }

    cbD->d->currentRenderPassEncoder = [cbD->d->cb renderCommandEncoderWithDescriptor: cbD->d->currentPassRpDesc];

    cbD->resetPerPassState();

    cbD->recordingPass = QMetalCommandBuffer::RenderPass;
    cbD->currentTarget = rt;
}

void QRhiMetal::endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::RenderPass);

    [cbD->d->currentRenderPassEncoder endEncoding];

    cbD->recordingPass = QMetalCommandBuffer::NoPass;
    cbD->currentTarget = nullptr;

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);
}

void QRhiMetal::beginComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::NoPass);

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);

    cbD->d->currentComputePassEncoder = [cbD->d->cb computeCommandEncoder];
    cbD->resetPerPassState();
    cbD->recordingPass = QMetalCommandBuffer::ComputePass;
}

void QRhiMetal::endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::ComputePass);

    [cbD->d->currentComputePassEncoder endEncoding];
    cbD->recordingPass = QMetalCommandBuffer::NoPass;

    if (resourceUpdates)
        enqueueResourceUpdates(cb, resourceUpdates);
}

void QRhiMetal::setComputePipeline(QRhiCommandBuffer *cb, QRhiComputePipeline *ps)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::ComputePass);
    QMetalComputePipeline *psD = QRHI_RES(QMetalComputePipeline, ps);

    if (cbD->currentComputePipeline != ps || cbD->currentPipelineGeneration != psD->generation) {
        cbD->currentGraphicsPipeline = nullptr;
        cbD->currentComputePipeline = ps;
        cbD->currentPipelineGeneration = psD->generation;

        [cbD->d->currentComputePassEncoder setComputePipelineState: psD->d->ps];
    }

    psD->lastActiveFrameSlot = currentFrameSlot;
}

void QRhiMetal::dispatch(QRhiCommandBuffer *cb, int x, int y, int z)
{
    QMetalCommandBuffer *cbD = QRHI_RES(QMetalCommandBuffer, cb);
    Q_ASSERT(cbD->recordingPass == QMetalCommandBuffer::ComputePass);
    QMetalComputePipeline *psD = QRHI_RES(QMetalComputePipeline, cbD->currentComputePipeline);

    [cbD->d->currentComputePassEncoder dispatchThreadgroups: MTLSizeMake(NSUInteger(x), NSUInteger(y), NSUInteger(z))
      threadsPerThreadgroup: psD->d->localSize];
}

static void qrhimtl_releaseBuffer(const QRhiMetalData::DeferredReleaseEntry &e)
{
    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i)
        [e.buffer.buffers[i] release];
}

static void qrhimtl_releaseRenderBuffer(const QRhiMetalData::DeferredReleaseEntry &e)
{
    [e.renderbuffer.texture release];
}

static void qrhimtl_releaseTexture(const QRhiMetalData::DeferredReleaseEntry &e)
{
    [e.texture.texture release];
    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i)
        [e.texture.stagingBuffers[i] release];
    for (int i = 0; i < QRhi::MAX_LEVELS; ++i)
        [e.texture.views[i] release];
}

static void qrhimtl_releaseSampler(const QRhiMetalData::DeferredReleaseEntry &e)
{
    [e.sampler.samplerState release];
}

void QRhiMetal::executeDeferredReleases(bool forced)
{
    for (int i = d->releaseQueue.count() - 1; i >= 0; --i) {
        const QRhiMetalData::DeferredReleaseEntry &e(d->releaseQueue[i]);
        if (forced || currentFrameSlot == e.lastActiveFrameSlot || e.lastActiveFrameSlot < 0) {
            switch (e.type) {
            case QRhiMetalData::DeferredReleaseEntry::Buffer:
                qrhimtl_releaseBuffer(e);
                break;
            case QRhiMetalData::DeferredReleaseEntry::RenderBuffer:
                qrhimtl_releaseRenderBuffer(e);
                break;
            case QRhiMetalData::DeferredReleaseEntry::Texture:
                qrhimtl_releaseTexture(e);
                break;
            case QRhiMetalData::DeferredReleaseEntry::Sampler:
                qrhimtl_releaseSampler(e);
                break;
            case QRhiMetalData::DeferredReleaseEntry::StagingBuffer:
                [e.stagingBuffer.buffer release];
                break;
            default:
                break;
            }
            d->releaseQueue.removeAt(i);
        }
    }
}

void QRhiMetal::finishActiveReadbacks(bool forced)
{
    QVarLengthArray<std::function<void()>, 4> completedCallbacks;
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();

    for (int i = d->activeTextureReadbacks.count() - 1; i >= 0; --i) {
        const QRhiMetalData::TextureReadback &readback(d->activeTextureReadbacks[i]);
        if (forced || currentFrameSlot == readback.activeFrameSlot || readback.activeFrameSlot < 0) {
            readback.result->format = readback.format;
            readback.result->pixelSize = readback.pixelSize;
            readback.result->data.resize(int(readback.bufSize));
            void *p = [readback.buf contents];
            memcpy(readback.result->data.data(), p, readback.bufSize);
            [readback.buf release];

            QRHI_PROF_F(releaseReadbackBuffer(qint64(qintptr(readback.buf))));

            if (readback.result->completed)
                completedCallbacks.append(readback.result->completed);

            d->activeTextureReadbacks.removeAt(i);
        }
    }

    for (auto f : completedCallbacks)
        f();
}

QMetalBuffer::QMetalBuffer(QRhiImplementation *rhi, Type type, UsageFlags usage, int size)
    : QRhiBuffer(rhi, type, usage, size),
      d(new QMetalBufferData)
{
    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i)
        d->buf[i] = nil;
}

QMetalBuffer::~QMetalBuffer()
{
    release();
    delete d;
}

void QMetalBuffer::release()
{
    if (!d->buf[0])
        return;

    QRhiMetalData::DeferredReleaseEntry e;
    e.type = QRhiMetalData::DeferredReleaseEntry::Buffer;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i) {
        e.buffer.buffers[i] = d->buf[i];
        d->buf[i] = nil;
        d->pendingUpdates[i].clear();
    }

    QRHI_RES_RHI(QRhiMetal);
    rhiD->d->releaseQueue.append(e);
    QRHI_PROF;
    QRHI_PROF_F(releaseBuffer(this));
    rhiD->unregisterResource(this);
}

bool QMetalBuffer::build()
{
    if (d->buf[0])
        release();

    if (m_usage.testFlag(QRhiBuffer::StorageBuffer) && m_type == Dynamic) {
        qWarning("StorageBuffer cannot be combined with Dynamic");
        return false;
    }

    const uint nonZeroSize = m_size <= 0 ? 256 : uint(m_size);
    const uint roundedSize = m_usage.testFlag(QRhiBuffer::UniformBuffer) ? aligned<uint>(nonZeroSize, 256) : nonZeroSize;

    d->managed = false;
    MTLResourceOptions opts = MTLResourceStorageModeShared;
#ifdef Q_OS_MACOS
    if (m_type != Dynamic) {
        opts = MTLResourceStorageModeManaged;
        d->managed = true;
    }
#endif

    // Have QMTL_FRAMES_IN_FLIGHT versions regardless of the type, for now.
    // This is because writing to a Managed buffer (which is what Immutable and
    // Static maps to on macOS) is not safe when another frame reading from the
    // same buffer is still in flight.
    d->slotted = !m_usage.testFlag(QRhiBuffer::StorageBuffer); // except for SSBOs written in the shader

    QRHI_RES_RHI(QRhiMetal);
    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i) {
        if (i == 0 || d->slotted) {
            d->buf[i] = [rhiD->d->dev newBufferWithLength: roundedSize options: opts];
            if (!m_objectName.isEmpty()) {
                if (!d->slotted) {
                    d->buf[i].label = [NSString stringWithUTF8String: m_objectName.constData()];
                } else {
                    const QByteArray name = m_objectName + '/' + QByteArray::number(i);
                    d->buf[i].label = [NSString stringWithUTF8String: name.constData()];
                }
            }
        }
    }

    QRHI_PROF;
    QRHI_PROF_F(newBuffer(this, roundedSize, d->slotted ? QMTL_FRAMES_IN_FLIGHT : 1, 0));

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QRhiBuffer::NativeBuffer QMetalBuffer::nativeBuffer()
{
    if (d->slotted) {
        NativeBuffer b;
        Q_ASSERT(sizeof(b.objects) / sizeof(b.objects[0]) >= size_t(QMTL_FRAMES_IN_FLIGHT));
        for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i) {
            QRHI_RES_RHI(QRhiMetal);
            rhiD->executeBufferHostWritesForSlot(this, i);
            b.objects[i] = &d->buf[i];
        }
        b.slotCount = QMTL_FRAMES_IN_FLIGHT;
        return b;
    }
    return { { &d->buf[0] }, 1 };
}

QMetalRenderBuffer::QMetalRenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                                       int sampleCount, QRhiRenderBuffer::Flags flags)
    : QRhiRenderBuffer(rhi, type, pixelSize, sampleCount, flags),
      d(new QMetalRenderBufferData)
{
}

QMetalRenderBuffer::~QMetalRenderBuffer()
{
    release();
    delete d;
}

void QMetalRenderBuffer::release()
{
    if (!d->tex)
        return;

    QRhiMetalData::DeferredReleaseEntry e;
    e.type = QRhiMetalData::DeferredReleaseEntry::RenderBuffer;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.renderbuffer.texture = d->tex;
    d->tex = nil;

    QRHI_RES_RHI(QRhiMetal);
    rhiD->d->releaseQueue.append(e);
    QRHI_PROF;
    QRHI_PROF_F(releaseRenderBuffer(this));
    rhiD->unregisterResource(this);
}

bool QMetalRenderBuffer::build()
{
    if (d->tex)
        release();

    if (m_pixelSize.isEmpty())
        return false;

    QRHI_RES_RHI(QRhiMetal);
    samples = rhiD->effectiveSampleCount(m_sampleCount);

    MTLTextureDescriptor *desc = [[MTLTextureDescriptor alloc] init];
    desc.textureType = samples > 1 ? MTLTextureType2DMultisample : MTLTextureType2D;
    desc.width = NSUInteger(m_pixelSize.width());
    desc.height = NSUInteger(m_pixelSize.height());
    if (samples > 1)
        desc.sampleCount = NSUInteger(samples);
    desc.resourceOptions = MTLResourceStorageModePrivate;
    desc.usage = MTLTextureUsageRenderTarget;

    bool transientBacking = false;
    switch (m_type) {
    case DepthStencil:
#ifdef Q_OS_MACOS
        desc.storageMode = MTLStorageModePrivate;
        d->format = rhiD->d->dev.depth24Stencil8PixelFormatSupported
                ? MTLPixelFormatDepth24Unorm_Stencil8 : MTLPixelFormatDepth32Float_Stencil8;
#else
        desc.storageMode = MTLStorageModeMemoryless;
        transientBacking = true;
        d->format = MTLPixelFormatDepth32Float_Stencil8;
#endif
        desc.pixelFormat = d->format;
        break;
    case Color:
        desc.storageMode = MTLStorageModePrivate;
        d->format = MTLPixelFormatRGBA8Unorm;
        desc.pixelFormat = d->format;
        break;
    default:
        Q_UNREACHABLE();
        break;
    }

    d->tex = [rhiD->d->dev newTextureWithDescriptor: desc];
    [desc release];

    if (!m_objectName.isEmpty())
        d->tex.label = [NSString stringWithUTF8String: m_objectName.constData()];

    QRHI_PROF;
    QRHI_PROF_F(newRenderBuffer(this, transientBacking, false, samples));

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::Format QMetalRenderBuffer::backingFormat() const
{
    return m_type == Color ? QRhiTexture::RGBA8 : QRhiTexture::UnknownFormat;
}

QMetalTexture::QMetalTexture(QRhiImplementation *rhi, Format format, const QSize &pixelSize,
                             int sampleCount, Flags flags)
    : QRhiTexture(rhi, format, pixelSize, sampleCount, flags),
      d(new QMetalTextureData(this))
{
    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i)
        d->stagingBuf[i] = nil;

    for (int i = 0; i < QRhi::MAX_LEVELS; ++i)
        d->perLevelViews[i] = nil;
}

QMetalTexture::~QMetalTexture()
{
    release();
    delete d;
}

void QMetalTexture::release()
{
    if (!d->tex)
        return;

    QRhiMetalData::DeferredReleaseEntry e;
    e.type = QRhiMetalData::DeferredReleaseEntry::Texture;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.texture.texture = d->owns ? d->tex : nil;
    d->tex = nil;

    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i) {
        e.texture.stagingBuffers[i] = d->stagingBuf[i];
        d->stagingBuf[i] = nil;
    }

    for (int i = 0; i < QRhi::MAX_LEVELS; ++i) {
        e.texture.views[i] = d->perLevelViews[i];
        d->perLevelViews[i] = nil;
    }

    QRHI_RES_RHI(QRhiMetal);
    rhiD->d->releaseQueue.append(e);
    QRHI_PROF;
    QRHI_PROF_F(releaseTexture(this));
    rhiD->unregisterResource(this);
}

static inline MTLPixelFormat toMetalTextureFormat(QRhiTexture::Format format, QRhiTexture::Flags flags)
{
    const bool srgb = flags.testFlag(QRhiTexture::sRGB);
    switch (format) {
    case QRhiTexture::RGBA8:
        return srgb ? MTLPixelFormatRGBA8Unorm_sRGB : MTLPixelFormatRGBA8Unorm;
    case QRhiTexture::BGRA8:
        return srgb ? MTLPixelFormatBGRA8Unorm_sRGB : MTLPixelFormatBGRA8Unorm;
    case QRhiTexture::R8:
#ifdef Q_OS_MACOS
        return MTLPixelFormatR8Unorm;
#else
        return srgb ? MTLPixelFormatR8Unorm_sRGB : MTLPixelFormatR8Unorm;
#endif
    case QRhiTexture::R16:
        return MTLPixelFormatR16Unorm;
    case QRhiTexture::RED_OR_ALPHA8:
        return MTLPixelFormatR8Unorm;

    case QRhiTexture::RGBA16F:
        return MTLPixelFormatRGBA16Float;
    case QRhiTexture::RGBA32F:
        return MTLPixelFormatRGBA32Float;
    case QRhiTexture::R16F:
        return MTLPixelFormatR16Float;
    case QRhiTexture::R32F:
        return MTLPixelFormatR32Float;

    case QRhiTexture::D16:
#ifdef Q_OS_MACOS
        return MTLPixelFormatDepth16Unorm;
#else
        return MTLPixelFormatDepth32Float;
#endif
    case QRhiTexture::D32F:
        return MTLPixelFormatDepth32Float;

#ifdef Q_OS_MACOS
    case QRhiTexture::BC1:
        return srgb ? MTLPixelFormatBC1_RGBA_sRGB : MTLPixelFormatBC1_RGBA;
    case QRhiTexture::BC2:
        return srgb ? MTLPixelFormatBC2_RGBA_sRGB : MTLPixelFormatBC2_RGBA;
    case QRhiTexture::BC3:
        return srgb ? MTLPixelFormatBC3_RGBA_sRGB : MTLPixelFormatBC3_RGBA;
    case QRhiTexture::BC4:
        return MTLPixelFormatBC4_RUnorm;
    case QRhiTexture::BC5:
        qWarning("QRhiMetal does not support BC5");
        return MTLPixelFormatRGBA8Unorm;
    case QRhiTexture::BC6H:
        return MTLPixelFormatBC6H_RGBUfloat;
    case QRhiTexture::BC7:
        return srgb ? MTLPixelFormatBC7_RGBAUnorm_sRGB : MTLPixelFormatBC7_RGBAUnorm;
#else
    case QRhiTexture::BC1:
    case QRhiTexture::BC2:
    case QRhiTexture::BC3:
    case QRhiTexture::BC4:
    case QRhiTexture::BC5:
    case QRhiTexture::BC6H:
    case QRhiTexture::BC7:
        qWarning("QRhiMetal: BCx compression not supported on this platform");
        return MTLPixelFormatRGBA8Unorm;
#endif

#ifndef Q_OS_MACOS
    case QRhiTexture::ETC2_RGB8:
        return srgb ? MTLPixelFormatETC2_RGB8_sRGB : MTLPixelFormatETC2_RGB8;
    case QRhiTexture::ETC2_RGB8A1:
        return srgb ? MTLPixelFormatETC2_RGB8A1_sRGB : MTLPixelFormatETC2_RGB8A1;
    case QRhiTexture::ETC2_RGBA8:
        return srgb ? MTLPixelFormatEAC_RGBA8_sRGB : MTLPixelFormatEAC_RGBA8;

    case QRhiTexture::ASTC_4x4:
        return srgb ? MTLPixelFormatASTC_4x4_sRGB : MTLPixelFormatASTC_4x4_LDR;
    case QRhiTexture::ASTC_5x4:
        return srgb ? MTLPixelFormatASTC_5x4_sRGB : MTLPixelFormatASTC_5x4_LDR;
    case QRhiTexture::ASTC_5x5:
        return srgb ? MTLPixelFormatASTC_5x5_sRGB : MTLPixelFormatASTC_5x5_LDR;
    case QRhiTexture::ASTC_6x5:
        return srgb ? MTLPixelFormatASTC_6x5_sRGB : MTLPixelFormatASTC_6x5_LDR;
    case QRhiTexture::ASTC_6x6:
        return srgb ? MTLPixelFormatASTC_6x6_sRGB : MTLPixelFormatASTC_6x6_LDR;
    case QRhiTexture::ASTC_8x5:
        return srgb ? MTLPixelFormatASTC_8x5_sRGB : MTLPixelFormatASTC_8x5_LDR;
    case QRhiTexture::ASTC_8x6:
        return srgb ? MTLPixelFormatASTC_8x6_sRGB : MTLPixelFormatASTC_8x6_LDR;
    case QRhiTexture::ASTC_8x8:
        return srgb ? MTLPixelFormatASTC_8x8_sRGB : MTLPixelFormatASTC_8x8_LDR;
    case QRhiTexture::ASTC_10x5:
        return srgb ? MTLPixelFormatASTC_10x5_sRGB : MTLPixelFormatASTC_10x5_LDR;
    case QRhiTexture::ASTC_10x6:
        return srgb ? MTLPixelFormatASTC_10x6_sRGB : MTLPixelFormatASTC_10x6_LDR;
    case QRhiTexture::ASTC_10x8:
        return srgb ? MTLPixelFormatASTC_10x8_sRGB : MTLPixelFormatASTC_10x8_LDR;
    case QRhiTexture::ASTC_10x10:
        return srgb ? MTLPixelFormatASTC_10x10_sRGB : MTLPixelFormatASTC_10x10_LDR;
    case QRhiTexture::ASTC_12x10:
        return srgb ? MTLPixelFormatASTC_12x10_sRGB : MTLPixelFormatASTC_12x10_LDR;
    case QRhiTexture::ASTC_12x12:
        return srgb ? MTLPixelFormatASTC_12x12_sRGB : MTLPixelFormatASTC_12x12_LDR;
#else
    case QRhiTexture::ETC2_RGB8:
    case QRhiTexture::ETC2_RGB8A1:
    case QRhiTexture::ETC2_RGBA8:
        qWarning("QRhiMetal: ETC2 compression not supported on this platform");
        return MTLPixelFormatRGBA8Unorm;

    case QRhiTexture::ASTC_4x4:
    case QRhiTexture::ASTC_5x4:
    case QRhiTexture::ASTC_5x5:
    case QRhiTexture::ASTC_6x5:
    case QRhiTexture::ASTC_6x6:
    case QRhiTexture::ASTC_8x5:
    case QRhiTexture::ASTC_8x6:
    case QRhiTexture::ASTC_8x8:
    case QRhiTexture::ASTC_10x5:
    case QRhiTexture::ASTC_10x6:
    case QRhiTexture::ASTC_10x8:
    case QRhiTexture::ASTC_10x10:
    case QRhiTexture::ASTC_12x10:
    case QRhiTexture::ASTC_12x12:
        qWarning("QRhiMetal: ASTC compression not supported on this platform");
        return MTLPixelFormatRGBA8Unorm;
#endif

    default:
        Q_UNREACHABLE();
        return MTLPixelFormatRGBA8Unorm;
    }
}

bool QMetalTexture::prepareBuild(QSize *adjustedSize)
{
    if (d->tex)
        release();

    const QSize size = m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize;
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);

    QRHI_RES_RHI(QRhiMetal);
    d->format = toMetalTextureFormat(m_format, m_flags);
    mipLevelCount = hasMipMaps ? rhiD->q->mipLevelsForSize(size) : 1;
    samples = rhiD->effectiveSampleCount(m_sampleCount);
    if (samples > 1) {
        if (isCube) {
            qWarning("Cubemap texture cannot be multisample");
            return false;
        }
        if (hasMipMaps) {
            qWarning("Multisample texture cannot have mipmaps");
            return false;
        }
    }

    if (adjustedSize)
        *adjustedSize = size;

    return true;
}

bool QMetalTexture::build()
{
    QSize size;
    if (!prepareBuild(&size))
        return false;

    MTLTextureDescriptor *desc = [[MTLTextureDescriptor alloc] init];

    const bool isCube = m_flags.testFlag(CubeMap);
    if (isCube)
        desc.textureType = MTLTextureTypeCube;
    else
        desc.textureType = samples > 1 ? MTLTextureType2DMultisample : MTLTextureType2D;
    desc.pixelFormat = d->format;
    desc.width = NSUInteger(size.width());
    desc.height = NSUInteger(size.height());
    desc.mipmapLevelCount = NSUInteger(mipLevelCount);
    if (samples > 1)
        desc.sampleCount = NSUInteger(samples);
    desc.resourceOptions = MTLResourceStorageModePrivate;
    desc.storageMode = MTLStorageModePrivate;
    desc.usage = MTLTextureUsageShaderRead;
    if (m_flags.testFlag(RenderTarget))
        desc.usage |= MTLTextureUsageRenderTarget;
    if (m_flags.testFlag(UsedWithLoadStore))
        desc.usage |= MTLTextureUsageShaderWrite;

    QRHI_RES_RHI(QRhiMetal);
    d->tex = [rhiD->d->dev newTextureWithDescriptor: desc];
    [desc release];

    if (!m_objectName.isEmpty())
        d->tex.label = [NSString stringWithUTF8String: m_objectName.constData()];

    d->owns = true;

    QRHI_PROF;
    QRHI_PROF_F(newTexture(this, true, mipLevelCount, isCube ? 6 : 1, samples));

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

bool QMetalTexture::buildFrom(QRhiTexture::NativeTexture src)
{
    void * const * tex = (void * const *) src.object;
    if (!tex || !*tex)
        return false;

    if (!prepareBuild())
        return false;

    d->tex = (id<MTLTexture>) *tex;

    d->owns = false;

    QRHI_PROF;
    QRHI_PROF_F(newTexture(this, false, mipLevelCount, m_flags.testFlag(CubeMap) ? 6 : 1, samples));

    lastActiveFrameSlot = -1;
    generation += 1;
    QRHI_RES_RHI(QRhiMetal);
    rhiD->registerResource(this);
    return true;
}

QRhiTexture::NativeTexture QMetalTexture::nativeTexture()
{
    return {&d->tex, 0};
}

id<MTLTexture> QMetalTextureData::viewForLevel(int level)
{
    Q_ASSERT(level >= 0 && level < int(q->mipLevelCount));
    if (perLevelViews[level])
        return perLevelViews[level];

    const MTLTextureType type = [tex textureType];
    const bool isCube = q->m_flags.testFlag(QRhiTexture::CubeMap);
    id<MTLTexture> view = [tex newTextureViewWithPixelFormat: format textureType: type
            levels: NSMakeRange(NSUInteger(level), 1) slices: NSMakeRange(0, isCube ? 6 : 1)];

    perLevelViews[level] = view;
    return view;
}

QMetalSampler::QMetalSampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                             AddressMode u, AddressMode v, AddressMode w)
    : QRhiSampler(rhi, magFilter, minFilter, mipmapMode, u, v, w),
      d(new QMetalSamplerData)
{
}

QMetalSampler::~QMetalSampler()
{
    release();
    delete d;
}

void QMetalSampler::release()
{
    if (!d->samplerState)
        return;

    QRhiMetalData::DeferredReleaseEntry e;
    e.type = QRhiMetalData::DeferredReleaseEntry::Sampler;
    e.lastActiveFrameSlot = lastActiveFrameSlot;

    e.sampler.samplerState = d->samplerState;
    d->samplerState = nil;

    QRHI_RES_RHI(QRhiMetal);
    rhiD->d->releaseQueue.append(e);
    rhiD->unregisterResource(this);
}

static inline MTLSamplerMinMagFilter toMetalFilter(QRhiSampler::Filter f)
{
    switch (f) {
    case QRhiSampler::Nearest:
        return MTLSamplerMinMagFilterNearest;
    case QRhiSampler::Linear:
        return MTLSamplerMinMagFilterLinear;
    default:
        Q_UNREACHABLE();
        return MTLSamplerMinMagFilterNearest;
    }
}

static inline MTLSamplerMipFilter toMetalMipmapMode(QRhiSampler::Filter f)
{
    switch (f) {
    case QRhiSampler::None:
        return MTLSamplerMipFilterNotMipmapped;
    case QRhiSampler::Nearest:
        return MTLSamplerMipFilterNearest;
    case QRhiSampler::Linear:
        return MTLSamplerMipFilterLinear;
    default:
        Q_UNREACHABLE();
        return MTLSamplerMipFilterNotMipmapped;
    }
}

static inline MTLSamplerAddressMode toMetalAddressMode(QRhiSampler::AddressMode m)
{
    switch (m) {
    case QRhiSampler::Repeat:
        return MTLSamplerAddressModeRepeat;
    case QRhiSampler::ClampToEdge:
        return MTLSamplerAddressModeClampToEdge;
    case QRhiSampler::Mirror:
        return MTLSamplerAddressModeMirrorRepeat;
    default:
        Q_UNREACHABLE();
        return MTLSamplerAddressModeClampToEdge;
    }
}

static inline MTLCompareFunction toMetalTextureCompareFunction(QRhiSampler::CompareOp op)
{
    switch (op) {
    case QRhiSampler::Never:
        return MTLCompareFunctionNever;
    case QRhiSampler::Less:
        return MTLCompareFunctionLess;
    case QRhiSampler::Equal:
        return MTLCompareFunctionEqual;
    case QRhiSampler::LessOrEqual:
        return MTLCompareFunctionLessEqual;
    case QRhiSampler::Greater:
        return MTLCompareFunctionGreater;
    case QRhiSampler::NotEqual:
        return MTLCompareFunctionNotEqual;
    case QRhiSampler::GreaterOrEqual:
        return MTLCompareFunctionGreaterEqual;
    case QRhiSampler::Always:
        return MTLCompareFunctionAlways;
    default:
        Q_UNREACHABLE();
        return MTLCompareFunctionNever;
    }
}

bool QMetalSampler::build()
{
    if (d->samplerState)
        release();

    MTLSamplerDescriptor *desc = [[MTLSamplerDescriptor alloc] init];
    desc.minFilter = toMetalFilter(m_minFilter);
    desc.magFilter = toMetalFilter(m_magFilter);
    desc.mipFilter = toMetalMipmapMode(m_mipmapMode);
    desc.sAddressMode = toMetalAddressMode(m_addressU);
    desc.tAddressMode = toMetalAddressMode(m_addressV);
    desc.rAddressMode = toMetalAddressMode(m_addressW);
    desc.compareFunction = toMetalTextureCompareFunction(m_compareOp);

    QRHI_RES_RHI(QRhiMetal);
    d->samplerState = [rhiD->d->dev newSamplerStateWithDescriptor: desc];
    [desc release];

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

// dummy, no Vulkan-style RenderPass+Framebuffer concept here.
// We do have MTLRenderPassDescriptor of course, but it will be created on the fly for each pass.
QMetalRenderPassDescriptor::QMetalRenderPassDescriptor(QRhiImplementation *rhi)
    : QRhiRenderPassDescriptor(rhi)
{
}

QMetalRenderPassDescriptor::~QMetalRenderPassDescriptor()
{
    release();
}

void QMetalRenderPassDescriptor::release()
{
    // nothing to do here
}

bool QMetalRenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const
{
    if (!other)
        return false;

    const QMetalRenderPassDescriptor *o = QRHI_RES(const QMetalRenderPassDescriptor, other);

    if (colorAttachmentCount != o->colorAttachmentCount)
        return false;

    if (hasDepthStencil != o->hasDepthStencil)
         return false;

    for (int i = 0; i < colorAttachmentCount; ++i) {
        if (colorFormat[i] != o->colorFormat[i])
            return false;
    }

    if (hasDepthStencil) {
        if (dsFormat != o->dsFormat)
            return false;
    }

    return true;
}

QMetalReferenceRenderTarget::QMetalReferenceRenderTarget(QRhiImplementation *rhi)
    : QRhiRenderTarget(rhi),
      d(new QMetalRenderTargetData)
{
}

QMetalReferenceRenderTarget::~QMetalReferenceRenderTarget()
{
    release();
    delete d;
}

void QMetalReferenceRenderTarget::release()
{
    // nothing to do here
}

QSize QMetalReferenceRenderTarget::pixelSize() const
{
    return d->pixelSize;
}

float QMetalReferenceRenderTarget::devicePixelRatio() const
{
    return d->dpr;
}

int QMetalReferenceRenderTarget::sampleCount() const
{
    return d->sampleCount;
}

QMetalTextureRenderTarget::QMetalTextureRenderTarget(QRhiImplementation *rhi,
                                                     const QRhiTextureRenderTargetDescription &desc,
                                                     Flags flags)
    : QRhiTextureRenderTarget(rhi, desc, flags),
      d(new QMetalRenderTargetData)
{
}

QMetalTextureRenderTarget::~QMetalTextureRenderTarget()
{
    release();
    delete d;
}

void QMetalTextureRenderTarget::release()
{
    // nothing to do here
}

QRhiRenderPassDescriptor *QMetalTextureRenderTarget::newCompatibleRenderPassDescriptor()
{
    const int colorAttachmentCount = m_desc.cendColorAttachments() - m_desc.cbeginColorAttachments();
    QMetalRenderPassDescriptor *rpD = new QMetalRenderPassDescriptor(m_rhi);
    rpD->colorAttachmentCount = colorAttachmentCount;
    rpD->hasDepthStencil = m_desc.depthStencilBuffer() || m_desc.depthTexture();

    for (int i = 0; i < colorAttachmentCount; ++i) {
        const QRhiColorAttachment *colorAtt = m_desc.colorAttachmentAt(i);
        QMetalTexture *texD = QRHI_RES(QMetalTexture, colorAtt->texture());
        QMetalRenderBuffer *rbD = QRHI_RES(QMetalRenderBuffer, colorAtt->renderBuffer());
        rpD->colorFormat[i] = int(texD ? texD->d->format : rbD->d->format);
    }

    if (m_desc.depthTexture())
        rpD->dsFormat = int(QRHI_RES(QMetalTexture, m_desc.depthTexture())->d->format);
    else if (m_desc.depthStencilBuffer())
        rpD->dsFormat = int(QRHI_RES(QMetalRenderBuffer, m_desc.depthStencilBuffer())->d->format);

    return rpD;
}

bool QMetalTextureRenderTarget::build()
{
    const bool hasColorAttachments = m_desc.cbeginColorAttachments() != m_desc.cendColorAttachments();
    Q_ASSERT(hasColorAttachments || m_desc.depthTexture());
    Q_ASSERT(!m_desc.depthStencilBuffer() || !m_desc.depthTexture());
    const bool hasDepthStencil = m_desc.depthStencilBuffer() || m_desc.depthTexture();

    d->colorAttCount = 0;
    int attIndex = 0;
    for (auto it = m_desc.cbeginColorAttachments(), itEnd = m_desc.cendColorAttachments(); it != itEnd; ++it, ++attIndex) {
        d->colorAttCount += 1;
        QMetalTexture *texD = QRHI_RES(QMetalTexture, it->texture());
        QMetalRenderBuffer *rbD = QRHI_RES(QMetalRenderBuffer, it->renderBuffer());
        Q_ASSERT(texD || rbD);
        id<MTLTexture> dst = nil;
        if (texD) {
            dst = texD->d->tex;
            if (attIndex == 0) {
                d->pixelSize = texD->pixelSize();
                d->sampleCount = texD->samples;
            }
        } else if (rbD) {
            dst = rbD->d->tex;
            if (attIndex == 0) {
                d->pixelSize = rbD->pixelSize();
                d->sampleCount = rbD->samples;
            }
        }
        QMetalRenderTargetData::ColorAtt colorAtt;
        colorAtt.tex = dst;
        colorAtt.layer = it->layer();
        colorAtt.level = it->level();
        QMetalTexture *resTexD = QRHI_RES(QMetalTexture, it->resolveTexture());
        colorAtt.resolveTex = resTexD ? resTexD->d->tex : nil;
        colorAtt.resolveLayer = it->resolveLayer();
        colorAtt.resolveLevel = it->resolveLevel();
        d->fb.colorAtt[attIndex] = colorAtt;
    }
    d->dpr = 1;

    if (hasDepthStencil) {
        if (m_desc.depthTexture()) {
            QMetalTexture *depthTexD = QRHI_RES(QMetalTexture, m_desc.depthTexture());
            d->fb.dsTex = depthTexD->d->tex;
            d->fb.hasStencil = false;
            d->fb.depthNeedsStore = true;
            if (d->colorAttCount == 0) {
                d->pixelSize = depthTexD->pixelSize();
                d->sampleCount = depthTexD->samples;
            }
        } else {
            QMetalRenderBuffer *depthRbD = QRHI_RES(QMetalRenderBuffer, m_desc.depthStencilBuffer());
            d->fb.dsTex = depthRbD->d->tex;
            d->fb.hasStencil = true;
            d->fb.depthNeedsStore = false;
            if (d->colorAttCount == 0) {
                d->pixelSize = depthRbD->pixelSize();
                d->sampleCount = depthRbD->samples;
            }
        }
        d->dsAttCount = 1;
    } else {
        d->dsAttCount = 0;
    }

    return true;
}

QSize QMetalTextureRenderTarget::pixelSize() const
{
    return d->pixelSize;
}

float QMetalTextureRenderTarget::devicePixelRatio() const
{
    return d->dpr;
}

int QMetalTextureRenderTarget::sampleCount() const
{
    return d->sampleCount;
}

QMetalShaderResourceBindings::QMetalShaderResourceBindings(QRhiImplementation *rhi)
    : QRhiShaderResourceBindings(rhi)
{
}

QMetalShaderResourceBindings::~QMetalShaderResourceBindings()
{
    release();
}

void QMetalShaderResourceBindings::release()
{
    sortedBindings.clear();
    maxBinding = -1;
}

bool QMetalShaderResourceBindings::build()
{
    if (!sortedBindings.isEmpty())
        release();

    std::copy(m_bindings.cbegin(), m_bindings.cend(), std::back_inserter(sortedBindings));
    std::sort(sortedBindings.begin(), sortedBindings.end(),
              [](const QRhiShaderResourceBinding &a, const QRhiShaderResourceBinding &b)
    {
        return a.data()->binding < b.data()->binding;
    });
    if (!sortedBindings.isEmpty())
        maxBinding = sortedBindings.last().data()->binding;
    else
        maxBinding = -1;

    boundResourceData.resize(sortedBindings.count());

    for (int i = 0, ie = sortedBindings.count(); i != ie; ++i) {
        const QRhiShaderResourceBinding::Data *b = sortedBindings.at(i).data();
        QMetalShaderResourceBindings::BoundResourceData &bd(boundResourceData[i]);
        switch (b->type) {
        case QRhiShaderResourceBinding::UniformBuffer:
        {
            QMetalBuffer *bufD = QRHI_RES(QMetalBuffer, b->u.ubuf.buf);
            bd.ubuf.id = bufD->m_id;
            bd.ubuf.generation = bufD->generation;
        }
            break;
        case QRhiShaderResourceBinding::SampledTexture:
        {
            const QRhiShaderResourceBinding::Data::SampledTextureData *data = &b->u.stex;
            bd.stex.count = data->count;
            for (int elem = 0; elem < data->count; ++elem) {
                QMetalTexture *texD = QRHI_RES(QMetalTexture, data->texSamplers[elem].tex);
                QMetalSampler *samplerD = QRHI_RES(QMetalSampler, data->texSamplers[elem].sampler);
                bd.stex.d[elem].texId = texD->m_id;
                bd.stex.d[elem].texGeneration = texD->generation;
                bd.stex.d[elem].samplerId = samplerD->m_id;
                bd.stex.d[elem].samplerGeneration = samplerD->generation;
            }
        }
            break;
        case QRhiShaderResourceBinding::ImageLoad:
        case QRhiShaderResourceBinding::ImageStore:
        case QRhiShaderResourceBinding::ImageLoadStore:
        {
            QMetalTexture *texD = QRHI_RES(QMetalTexture, b->u.simage.tex);
            bd.simage.id = texD->m_id;
            bd.simage.generation = texD->generation;
        }
            break;
        case QRhiShaderResourceBinding::BufferLoad:
        case QRhiShaderResourceBinding::BufferStore:
        case QRhiShaderResourceBinding::BufferLoadStore:
        {
            QMetalBuffer *bufD = QRHI_RES(QMetalBuffer, b->u.sbuf.buf);
            bd.sbuf.id = bufD->m_id;
            bd.sbuf.generation = bufD->generation;
        }
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }

    generation += 1;
    return true;
}

QMetalGraphicsPipeline::QMetalGraphicsPipeline(QRhiImplementation *rhi)
    : QRhiGraphicsPipeline(rhi),
      d(new QMetalGraphicsPipelineData)
{
}

QMetalGraphicsPipeline::~QMetalGraphicsPipeline()
{
    release();
    delete d;
}

void QMetalGraphicsPipeline::release()
{
    QRHI_RES_RHI(QRhiMetal);

    d->vs.release();
    d->fs.release();

    [d->ds release];
    d->ds = nil;

    if (!d->ps)
        return;

    [d->ps release];
    d->ps = nil;

    rhiD->unregisterResource(this);
}

static inline MTLVertexFormat toMetalAttributeFormat(QRhiVertexInputAttribute::Format format)
{
    switch (format) {
    case QRhiVertexInputAttribute::Float4:
        return MTLVertexFormatFloat4;
    case QRhiVertexInputAttribute::Float3:
        return MTLVertexFormatFloat3;
    case QRhiVertexInputAttribute::Float2:
        return MTLVertexFormatFloat2;
    case QRhiVertexInputAttribute::Float:
        return MTLVertexFormatFloat;
    case QRhiVertexInputAttribute::UNormByte4:
        return MTLVertexFormatUChar4Normalized;
    case QRhiVertexInputAttribute::UNormByte2:
        return MTLVertexFormatUChar2Normalized;
    case QRhiVertexInputAttribute::UNormByte:
        if (@available(macOS 10.13, iOS 11.0, *))
            return MTLVertexFormatUCharNormalized;
        else
            Q_UNREACHABLE();
    default:
        Q_UNREACHABLE();
        return MTLVertexFormatFloat4;
    }
}

static inline MTLBlendFactor toMetalBlendFactor(QRhiGraphicsPipeline::BlendFactor f)
{
    switch (f) {
    case QRhiGraphicsPipeline::Zero:
        return MTLBlendFactorZero;
    case QRhiGraphicsPipeline::One:
        return MTLBlendFactorOne;
    case QRhiGraphicsPipeline::SrcColor:
        return MTLBlendFactorSourceColor;
    case QRhiGraphicsPipeline::OneMinusSrcColor:
        return MTLBlendFactorOneMinusSourceColor;
    case QRhiGraphicsPipeline::DstColor:
        return MTLBlendFactorDestinationColor;
    case QRhiGraphicsPipeline::OneMinusDstColor:
        return MTLBlendFactorOneMinusDestinationColor;
    case QRhiGraphicsPipeline::SrcAlpha:
        return MTLBlendFactorSourceAlpha;
    case QRhiGraphicsPipeline::OneMinusSrcAlpha:
        return MTLBlendFactorOneMinusSourceAlpha;
    case QRhiGraphicsPipeline::DstAlpha:
        return MTLBlendFactorDestinationAlpha;
    case QRhiGraphicsPipeline::OneMinusDstAlpha:
        return MTLBlendFactorOneMinusDestinationAlpha;
    case QRhiGraphicsPipeline::ConstantColor:
        return MTLBlendFactorBlendColor;
    case QRhiGraphicsPipeline::ConstantAlpha:
        return MTLBlendFactorBlendAlpha;
    case QRhiGraphicsPipeline::OneMinusConstantColor:
        return MTLBlendFactorOneMinusBlendColor;
    case QRhiGraphicsPipeline::OneMinusConstantAlpha:
        return MTLBlendFactorOneMinusBlendAlpha;
    case QRhiGraphicsPipeline::SrcAlphaSaturate:
        return MTLBlendFactorSourceAlphaSaturated;
    case QRhiGraphicsPipeline::Src1Color:
        return MTLBlendFactorSource1Color;
    case QRhiGraphicsPipeline::OneMinusSrc1Color:
        return MTLBlendFactorOneMinusSource1Color;
    case QRhiGraphicsPipeline::Src1Alpha:
        return MTLBlendFactorSource1Alpha;
    case QRhiGraphicsPipeline::OneMinusSrc1Alpha:
        return MTLBlendFactorOneMinusSource1Alpha;
    default:
        Q_UNREACHABLE();
        return MTLBlendFactorZero;
    }
}

static inline MTLBlendOperation toMetalBlendOp(QRhiGraphicsPipeline::BlendOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::Add:
        return MTLBlendOperationAdd;
    case QRhiGraphicsPipeline::Subtract:
        return MTLBlendOperationSubtract;
    case QRhiGraphicsPipeline::ReverseSubtract:
        return MTLBlendOperationReverseSubtract;
    case QRhiGraphicsPipeline::Min:
        return MTLBlendOperationMin;
    case QRhiGraphicsPipeline::Max:
        return MTLBlendOperationMax;
    default:
        Q_UNREACHABLE();
        return MTLBlendOperationAdd;
    }
}

static inline uint toMetalColorWriteMask(QRhiGraphicsPipeline::ColorMask c)
{
    uint f = 0;
    if (c.testFlag(QRhiGraphicsPipeline::R))
        f |= MTLColorWriteMaskRed;
    if (c.testFlag(QRhiGraphicsPipeline::G))
        f |= MTLColorWriteMaskGreen;
    if (c.testFlag(QRhiGraphicsPipeline::B))
        f |= MTLColorWriteMaskBlue;
    if (c.testFlag(QRhiGraphicsPipeline::A))
        f |= MTLColorWriteMaskAlpha;
    return f;
}

static inline MTLCompareFunction toMetalCompareOp(QRhiGraphicsPipeline::CompareOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::Never:
        return MTLCompareFunctionNever;
    case QRhiGraphicsPipeline::Less:
        return MTLCompareFunctionLess;
    case QRhiGraphicsPipeline::Equal:
        return MTLCompareFunctionEqual;
    case QRhiGraphicsPipeline::LessOrEqual:
        return MTLCompareFunctionLessEqual;
    case QRhiGraphicsPipeline::Greater:
        return MTLCompareFunctionGreater;
    case QRhiGraphicsPipeline::NotEqual:
        return MTLCompareFunctionNotEqual;
    case QRhiGraphicsPipeline::GreaterOrEqual:
        return MTLCompareFunctionGreaterEqual;
    case QRhiGraphicsPipeline::Always:
        return MTLCompareFunctionAlways;
    default:
        Q_UNREACHABLE();
        return MTLCompareFunctionAlways;
    }
}

static inline MTLStencilOperation toMetalStencilOp(QRhiGraphicsPipeline::StencilOp op)
{
    switch (op) {
    case QRhiGraphicsPipeline::StencilZero:
        return MTLStencilOperationZero;
    case QRhiGraphicsPipeline::Keep:
        return MTLStencilOperationKeep;
    case QRhiGraphicsPipeline::Replace:
        return MTLStencilOperationReplace;
    case QRhiGraphicsPipeline::IncrementAndClamp:
        return MTLStencilOperationIncrementClamp;
    case QRhiGraphicsPipeline::DecrementAndClamp:
        return MTLStencilOperationDecrementClamp;
    case QRhiGraphicsPipeline::Invert:
        return MTLStencilOperationInvert;
    case QRhiGraphicsPipeline::IncrementAndWrap:
        return MTLStencilOperationIncrementWrap;
    case QRhiGraphicsPipeline::DecrementAndWrap:
        return MTLStencilOperationDecrementWrap;
    default:
        Q_UNREACHABLE();
        return MTLStencilOperationKeep;
    }
}

static inline MTLPrimitiveType toMetalPrimitiveType(QRhiGraphicsPipeline::Topology t)
{
    switch (t) {
    case QRhiGraphicsPipeline::Triangles:
        return MTLPrimitiveTypeTriangle;
    case QRhiGraphicsPipeline::TriangleStrip:
        return MTLPrimitiveTypeTriangleStrip;
    case QRhiGraphicsPipeline::Lines:
        return MTLPrimitiveTypeLine;
    case QRhiGraphicsPipeline::LineStrip:
        return MTLPrimitiveTypeLineStrip;
    case QRhiGraphicsPipeline::Points:
        return MTLPrimitiveTypePoint;
    default:
        Q_UNREACHABLE();
        return MTLPrimitiveTypeTriangle;
    }
}

static inline MTLCullMode toMetalCullMode(QRhiGraphicsPipeline::CullMode c)
{
    switch (c) {
    case QRhiGraphicsPipeline::None:
        return MTLCullModeNone;
    case QRhiGraphicsPipeline::Front:
        return MTLCullModeFront;
    case QRhiGraphicsPipeline::Back:
        return MTLCullModeBack;
    default:
        Q_UNREACHABLE();
        return MTLCullModeNone;
    }
}

id<MTLLibrary> QRhiMetalData::createMetalLib(const QShader &shader, QShader::Variant shaderVariant,
                                             QString *error, QByteArray *entryPoint, QShaderKey *activeKey)
{
    QShaderKey key = { QShader::MetalLibShader, 20, shaderVariant };
    QShaderCode mtllib = shader.shader(key);
    if (mtllib.shader().isEmpty()) {
        key.setSourceVersion(12);
        mtllib = shader.shader(key);
    }
    if (!mtllib.shader().isEmpty()) {
        dispatch_data_t data = dispatch_data_create(mtllib.shader().constData(),
                                                    size_t(mtllib.shader().size()),
                                                    dispatch_get_global_queue(0, 0),
                                                    DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        NSError *err = nil;
        id<MTLLibrary> lib = [dev newLibraryWithData: data error: &err];
        dispatch_release(data);
        if (!err) {
            *entryPoint = mtllib.entryPoint();
            *activeKey = key;
            return lib;
        } else {
            const QString msg = QString::fromNSString(err.localizedDescription);
            qWarning("Failed to load metallib from baked shader: %s", qPrintable(msg));
        }
    }

    key = { QShader::MslShader, 20, shaderVariant };
    QShaderCode mslSource = shader.shader(key);
    if (mslSource.shader().isEmpty()) {
        key.setSourceVersion(12);
        mslSource = shader.shader(key);
    }
    if (mslSource.shader().isEmpty()) {
        qWarning() << "No MSL 2.0 or 1.2 code found in baked shader" << shader;
        return nil;
    }

    NSString *src = [NSString stringWithUTF8String: mslSource.shader().constData()];
    MTLCompileOptions *opts = [[MTLCompileOptions alloc] init];
    opts.languageVersion = key.sourceVersion() == 20 ? MTLLanguageVersion2_0 : MTLLanguageVersion1_2;
    NSError *err = nil;
    id<MTLLibrary> lib = [dev newLibraryWithSource: src options: opts error: &err];
    [opts release];
    // src is autoreleased

    // if lib is null and err is non-null, we had errors (fail)
    // if lib is non-null and err is non-null, we had warnings (success)
    // if lib is non-null and err is null, there were no errors or warnings (success)
    if (!lib) {
        const QString msg = QString::fromNSString(err.localizedDescription);
        *error = msg;
        return nil;
    }

    *entryPoint = mslSource.entryPoint();
    *activeKey = key;
    return lib;
}

id<MTLFunction> QRhiMetalData::createMSLShaderFunction(id<MTLLibrary> lib, const QByteArray &entryPoint)
{
    NSString *name = [NSString stringWithUTF8String: entryPoint.constData()];
    id<MTLFunction> f = [lib newFunctionWithName: name];
    [name release];
    return f;
}

bool QMetalGraphicsPipeline::build()
{
    if (d->ps)
        release();

    QRHI_RES_RHI(QRhiMetal);
    if (!rhiD->sanityCheckGraphicsPipeline(this))
        return false;

    // same binding space for vertex and constant buffers - work it around
    const int firstVertexBinding = QRHI_RES(QMetalShaderResourceBindings, m_shaderResourceBindings)->maxBinding + 1;

    MTLVertexDescriptor *inputLayout = [MTLVertexDescriptor vertexDescriptor];
    for (auto it = m_vertexInputLayout.cbeginAttributes(), itEnd = m_vertexInputLayout.cendAttributes();
         it != itEnd; ++it)
    {
        const uint loc = uint(it->location());
        inputLayout.attributes[loc].format = toMetalAttributeFormat(it->format());
        inputLayout.attributes[loc].offset = NSUInteger(it->offset());
        inputLayout.attributes[loc].bufferIndex = NSUInteger(firstVertexBinding + it->binding());
    }
    int bindingIndex = 0;
    for (auto it = m_vertexInputLayout.cbeginBindings(), itEnd = m_vertexInputLayout.cendBindings();
         it != itEnd; ++it, ++bindingIndex)
    {
        const uint layoutIdx = uint(firstVertexBinding + bindingIndex);
        inputLayout.layouts[layoutIdx].stepFunction =
                it->classification() == QRhiVertexInputBinding::PerInstance
                ? MTLVertexStepFunctionPerInstance : MTLVertexStepFunctionPerVertex;
        inputLayout.layouts[layoutIdx].stepRate = NSUInteger(it->instanceStepRate());
        inputLayout.layouts[layoutIdx].stride = it->stride();
    }

    MTLRenderPipelineDescriptor *rpDesc = [[MTLRenderPipelineDescriptor alloc] init];

    rpDesc.vertexDescriptor = inputLayout;

    // mutability cannot be determined (slotted buffers could be set as
    // MTLMutabilityImmutable, but then we potentially need a different
    // descriptor for each buffer combination as this depends on the actual
    // buffers not just the resource binding layout) so leave it at the default

    for (const QRhiShaderStage &shaderStage : qAsConst(m_shaderStages)) {
        auto cacheIt = rhiD->d->shaderCache.constFind(shaderStage);
        if (cacheIt != rhiD->d->shaderCache.constEnd()) {
            switch (shaderStage.type()) {
            case QRhiShaderStage::Vertex:
                d->vs = *cacheIt;
                [d->vs.lib retain];
                [d->vs.func retain];
                rpDesc.vertexFunction = d->vs.func;
                break;
            case QRhiShaderStage::Fragment:
                d->fs = *cacheIt;
                [d->fs.lib retain];
                [d->fs.func retain];
                rpDesc.fragmentFunction = d->fs.func;
                break;
            default:
                break;
            }
        } else {
            const QShader shader = shaderStage.shader();
            QString error;
            QByteArray entryPoint;
            QShaderKey activeKey;
            id<MTLLibrary> lib = rhiD->d->createMetalLib(shader, shaderStage.shaderVariant(),
                                                         &error, &entryPoint, &activeKey);
            if (!lib) {
                qWarning("MSL shader compilation failed: %s", qPrintable(error));
                return false;
            }
            id<MTLFunction> func = rhiD->d->createMSLShaderFunction(lib, entryPoint);
            if (!func) {
                qWarning("MSL function for entry point %s not found", entryPoint.constData());
                [lib release];
                return false;
            }
            if (rhiD->d->shaderCache.count() >= QRhiMetal::MAX_SHADER_CACHE_ENTRIES) {
                // Use the simplest strategy: too many cached shaders -> drop them all.
                for (QMetalShader &s : rhiD->d->shaderCache)
                    s.release();
                rhiD->d->shaderCache.clear();
            }
            switch (shaderStage.type()) {
            case QRhiShaderStage::Vertex:
                d->vs.lib = lib;
                d->vs.func = func;
                if (const QShader::NativeResourceBindingMap *map = shader.nativeResourceBindingMap(activeKey))
                    d->vs.nativeResourceBindingMap = *map;
                rhiD->d->shaderCache.insert(shaderStage, d->vs);
                [d->vs.lib retain];
                [d->vs.func retain];
                rpDesc.vertexFunction = func;
                break;
            case QRhiShaderStage::Fragment:
                d->fs.lib = lib;
                d->fs.func = func;
                if (const QShader::NativeResourceBindingMap *map = shader.nativeResourceBindingMap(activeKey))
                    d->fs.nativeResourceBindingMap = *map;
                rhiD->d->shaderCache.insert(shaderStage, d->fs);
                [d->fs.lib retain];
                [d->fs.func retain];
                rpDesc.fragmentFunction = func;
                break;
            default:
                [func release];
                [lib release];
                break;
            }
        }
    }

    QMetalRenderPassDescriptor *rpD = QRHI_RES(QMetalRenderPassDescriptor, m_renderPassDesc);

    if (rpD->colorAttachmentCount) {
        // defaults when no targetBlends are provided
        rpDesc.colorAttachments[0].pixelFormat = MTLPixelFormat(rpD->colorFormat[0]);
        rpDesc.colorAttachments[0].writeMask = MTLColorWriteMaskAll;
        rpDesc.colorAttachments[0].blendingEnabled = false;

        Q_ASSERT(m_targetBlends.count() == rpD->colorAttachmentCount
                 || (m_targetBlends.isEmpty() && rpD->colorAttachmentCount == 1));

        for (uint i = 0, ie = uint(m_targetBlends.count()); i != ie; ++i) {
            const QRhiGraphicsPipeline::TargetBlend &b(m_targetBlends[int(i)]);
            rpDesc.colorAttachments[i].pixelFormat = MTLPixelFormat(rpD->colorFormat[i]);
            rpDesc.colorAttachments[i].blendingEnabled = b.enable;
            rpDesc.colorAttachments[i].sourceRGBBlendFactor = toMetalBlendFactor(b.srcColor);
            rpDesc.colorAttachments[i].destinationRGBBlendFactor = toMetalBlendFactor(b.dstColor);
            rpDesc.colorAttachments[i].rgbBlendOperation = toMetalBlendOp(b.opColor);
            rpDesc.colorAttachments[i].sourceAlphaBlendFactor = toMetalBlendFactor(b.srcAlpha);
            rpDesc.colorAttachments[i].destinationAlphaBlendFactor = toMetalBlendFactor(b.dstAlpha);
            rpDesc.colorAttachments[i].alphaBlendOperation = toMetalBlendOp(b.opAlpha);
            rpDesc.colorAttachments[i].writeMask = toMetalColorWriteMask(b.colorWrite);
        }
    }

    if (rpD->hasDepthStencil) {
        // Must only be set when a depth-stencil buffer will actually be bound,
        // validation blows up otherwise.
        MTLPixelFormat fmt = MTLPixelFormat(rpD->dsFormat);
        rpDesc.depthAttachmentPixelFormat = fmt;
#ifdef Q_OS_MACOS
        if (fmt != MTLPixelFormatDepth16Unorm && fmt != MTLPixelFormatDepth32Float)
#else
        if (fmt != MTLPixelFormatDepth32Float)
#endif
            rpDesc.stencilAttachmentPixelFormat = fmt;
    }

    rpDesc.sampleCount = NSUInteger(rhiD->effectiveSampleCount(m_sampleCount));

    NSError *err = nil;
    d->ps = [rhiD->d->dev newRenderPipelineStateWithDescriptor: rpDesc error: &err];
    if (!d->ps) {
        const QString msg = QString::fromNSString(err.localizedDescription);
        qWarning("Failed to create render pipeline state: %s", qPrintable(msg));
        [rpDesc release];
        return false;
    }
    [rpDesc release];

    MTLDepthStencilDescriptor *dsDesc = [[MTLDepthStencilDescriptor alloc] init];
    dsDesc.depthCompareFunction = m_depthTest ? toMetalCompareOp(m_depthOp) : MTLCompareFunctionAlways;
    dsDesc.depthWriteEnabled = m_depthWrite;
    if (m_stencilTest) {
        dsDesc.frontFaceStencil = [[MTLStencilDescriptor alloc] init];
        dsDesc.frontFaceStencil.stencilFailureOperation = toMetalStencilOp(m_stencilFront.failOp);
        dsDesc.frontFaceStencil.depthFailureOperation = toMetalStencilOp(m_stencilFront.depthFailOp);
        dsDesc.frontFaceStencil.depthStencilPassOperation = toMetalStencilOp(m_stencilFront.passOp);
        dsDesc.frontFaceStencil.stencilCompareFunction = toMetalCompareOp(m_stencilFront.compareOp);
        dsDesc.frontFaceStencil.readMask = m_stencilReadMask;
        dsDesc.frontFaceStencil.writeMask = m_stencilWriteMask;

        dsDesc.backFaceStencil = [[MTLStencilDescriptor alloc] init];
        dsDesc.backFaceStencil.stencilFailureOperation = toMetalStencilOp(m_stencilBack.failOp);
        dsDesc.backFaceStencil.depthFailureOperation = toMetalStencilOp(m_stencilBack.depthFailOp);
        dsDesc.backFaceStencil.depthStencilPassOperation = toMetalStencilOp(m_stencilBack.passOp);
        dsDesc.backFaceStencil.stencilCompareFunction = toMetalCompareOp(m_stencilBack.compareOp);
        dsDesc.backFaceStencil.readMask = m_stencilReadMask;
        dsDesc.backFaceStencil.writeMask = m_stencilWriteMask;
    }

    d->ds = [rhiD->d->dev newDepthStencilStateWithDescriptor: dsDesc];
    [dsDesc release];

    d->primitiveType = toMetalPrimitiveType(m_topology);
    d->winding = m_frontFace == CCW ? MTLWindingCounterClockwise : MTLWindingClockwise;
    d->cullMode = toMetalCullMode(m_cullMode);
    d->depthBias = float(m_depthBias);
    d->slopeScaledDepthBias = m_slopeScaledDepthBias;

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QMetalComputePipeline::QMetalComputePipeline(QRhiImplementation *rhi)
    : QRhiComputePipeline(rhi),
      d(new QMetalComputePipelineData)
{
}

QMetalComputePipeline::~QMetalComputePipeline()
{
    release();
    delete d;
}

void QMetalComputePipeline::release()
{
    QRHI_RES_RHI(QRhiMetal);

    d->cs.release();

    if (!d->ps)
        return;

    [d->ps release];
    d->ps = nil;

    rhiD->unregisterResource(this);
}

bool QMetalComputePipeline::build()
{
    if (d->ps)
        release();

    QRHI_RES_RHI(QRhiMetal);

    auto cacheIt = rhiD->d->shaderCache.constFind(m_shaderStage);
    if (cacheIt != rhiD->d->shaderCache.constEnd()) {
        d->cs = *cacheIt;
    } else {
        const QShader shader = m_shaderStage.shader();
        QString error;
        QByteArray entryPoint;
        QShaderKey activeKey;
        id<MTLLibrary> lib = rhiD->d->createMetalLib(shader, m_shaderStage.shaderVariant(),
                                                     &error, &entryPoint, &activeKey);
        if (!lib) {
            qWarning("MSL shader compilation failed: %s", qPrintable(error));
            return false;
        }
        id<MTLFunction> func = rhiD->d->createMSLShaderFunction(lib, entryPoint);
        if (!func) {
            qWarning("MSL function for entry point %s not found", entryPoint.constData());
            [lib release];
            return false;
        }
        d->cs.lib = lib;
        d->cs.func = func;
        d->cs.localSize = shader.description().computeShaderLocalSize();
        if (const QShader::NativeResourceBindingMap *map = shader.nativeResourceBindingMap(activeKey))
            d->cs.nativeResourceBindingMap = *map;

        if (rhiD->d->shaderCache.count() >= QRhiMetal::MAX_SHADER_CACHE_ENTRIES) {
            for (QMetalShader &s : rhiD->d->shaderCache)
                s.release();
            rhiD->d->shaderCache.clear();
        }
        rhiD->d->shaderCache.insert(m_shaderStage, d->cs);
    }

    [d->cs.lib retain];
    [d->cs.func retain];

    d->localSize = MTLSizeMake(d->cs.localSize[0], d->cs.localSize[1], d->cs.localSize[2]);

    NSError *err = nil;
    d->ps = [rhiD->d->dev newComputePipelineStateWithFunction: d->cs.func error: &err];
    if (!d->ps) {
        const QString msg = QString::fromNSString(err.localizedDescription);
        qWarning("Failed to create render pipeline state: %s", qPrintable(msg));
        return false;
    }

    lastActiveFrameSlot = -1;
    generation += 1;
    rhiD->registerResource(this);
    return true;
}

QMetalCommandBuffer::QMetalCommandBuffer(QRhiImplementation *rhi)
    : QRhiCommandBuffer(rhi),
      d(new QMetalCommandBufferData)
{
    resetState();
}

QMetalCommandBuffer::~QMetalCommandBuffer()
{
    release();
    delete d;
}

void QMetalCommandBuffer::release()
{
    // nothing to do here, we do not own the MTL cb object
}

const QRhiNativeHandles *QMetalCommandBuffer::nativeHandles()
{
    nativeHandlesStruct.commandBuffer = d->cb;
    nativeHandlesStruct.encoder = d->currentRenderPassEncoder;
    return &nativeHandlesStruct;
}

void QMetalCommandBuffer::resetState()
{
    d->currentRenderPassEncoder = nil;
    d->currentComputePassEncoder = nil;
    d->currentPassRpDesc = nil;
    resetPerPassState();
}

void QMetalCommandBuffer::resetPerPassState()
{
    recordingPass = NoPass;
    currentTarget = nullptr;
    resetPerPassCachedState();
}

void QMetalCommandBuffer::resetPerPassCachedState()
{
    currentGraphicsPipeline = nullptr;
    currentComputePipeline = nullptr;
    currentPipelineGeneration = 0;
    currentGraphicsSrb = nullptr;
    currentComputeSrb = nullptr;
    currentSrbGeneration = 0;
    currentResSlot = -1;
    currentIndexBuffer = nullptr;
    currentIndexOffset = 0;
    currentIndexFormat = QRhiCommandBuffer::IndexUInt16;
    currentCullMode = -1;
    currentFrontFaceWinding = -1;
    currentDepthBiasValues = { 0.0f, 0.0f };

    d->currentFirstVertexBinding = -1;
    d->currentVertexInputsBuffers.clear();
    d->currentVertexInputOffsets.clear();
}

QMetalSwapChain::QMetalSwapChain(QRhiImplementation *rhi)
    : QRhiSwapChain(rhi),
      rtWrapper(rhi),
      cbWrapper(rhi),
      d(new QMetalSwapChainData)
{
    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i) {
        d->sem[i] = nullptr;
        d->msaaTex[i] = nil;
    }
}

QMetalSwapChain::~QMetalSwapChain()
{
    release();
    delete d;
}

void QMetalSwapChain::release()
{
#ifdef TARGET_IPHONE_SIMULATOR
    if (@available(ios 13.0, *)) {
#endif

    if (!d->layer)
        return;

    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i) {
        if (d->sem[i]) {
            // the semaphores cannot be released if they do not have the initial value
            dispatch_semaphore_wait(d->sem[i], DISPATCH_TIME_FOREVER);
            dispatch_semaphore_signal(d->sem[i]);

            dispatch_release(d->sem[i]);
            d->sem[i] = nullptr;
        }
    }

    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i) {
        [d->msaaTex[i] release];
        d->msaaTex[i] = nil;
    }

    d->layer = nullptr;

    QRHI_RES_RHI(QRhiMetal);
    rhiD->swapchains.remove(this);

    QRHI_PROF;
    QRHI_PROF_F(releaseSwapChain(this));

    rhiD->unregisterResource(this);

#ifdef TARGET_IPHONE_SIMULATOR
    }
#endif
}

QRhiCommandBuffer *QMetalSwapChain::currentFrameCommandBuffer()
{
    return &cbWrapper;
}

QRhiRenderTarget *QMetalSwapChain::currentFrameRenderTarget()
{
    return &rtWrapper;
}

QSize QMetalSwapChain::surfacePixelSize()
{
    Q_ASSERT(m_window);
    return m_window->size() * m_window->devicePixelRatio();
}

QRhiRenderPassDescriptor *QMetalSwapChain::newCompatibleRenderPassDescriptor()
{
    chooseFormats(); // ensure colorFormat and similar are filled out

    QMetalRenderPassDescriptor *rpD = new QMetalRenderPassDescriptor(m_rhi);
    rpD->colorAttachmentCount = 1;
    rpD->hasDepthStencil = m_depthStencil != nullptr;

    rpD->colorFormat[0] = int(d->colorFormat);

#ifdef Q_OS_MACOS
    // m_depthStencil may not be built yet so cannot rely on computed fields in it
    QRHI_RES_RHI(QRhiMetal);
    rpD->dsFormat = rhiD->d->dev.depth24Stencil8PixelFormatSupported
            ? MTLPixelFormatDepth24Unorm_Stencil8 : MTLPixelFormatDepth32Float_Stencil8;
#else
    rpD->dsFormat = MTLPixelFormatDepth32Float_Stencil8;
#endif

    return rpD;
}

void QMetalSwapChain::chooseFormats()
{
    QRHI_RES_RHI(QRhiMetal);
    samples = rhiD->effectiveSampleCount(m_sampleCount);
    // pick a format that is allowed for CAMetalLayer.pixelFormat
    d->colorFormat = m_flags.testFlag(sRGB) ? MTLPixelFormatBGRA8Unorm_sRGB : MTLPixelFormatBGRA8Unorm;
    d->rhiColorFormat = QRhiTexture::BGRA8;
}

bool QMetalSwapChain::buildOrResize()
{
#ifdef TARGET_IPHONE_SIMULATOR
    if (@available(ios 13.0, *)) {
#endif

    Q_ASSERT(m_window);

    const bool needsRegistration = !window || window != m_window;

    if (window && window != m_window)
        release();
    // else no release(), this is intentional

    QRHI_RES_RHI(QRhiMetal);
    if (needsRegistration)
        rhiD->swapchains.insert(this);

    window = m_window;

    if (window->surfaceType() != QSurface::MetalSurface) {
        qWarning("QMetalSwapChain only supports MetalSurface windows");
        return false;
    }

#ifdef Q_OS_MACOS
    NSView *view = reinterpret_cast<NSView *>(window->winId());
#else
    UIView *view = reinterpret_cast<UIView *>(window->winId());
#endif
    Q_ASSERT(view);
    d->layer = static_cast<CAMetalLayer *>(view.layer);
    Q_ASSERT(d->layer);

    chooseFormats();
    if (d->colorFormat != d->layer.pixelFormat)
        d->layer.pixelFormat = d->colorFormat;

    if (m_flags.testFlag(UsedAsTransferSource))
        d->layer.framebufferOnly = NO;

#ifdef Q_OS_MACOS
    if (m_flags.testFlag(NoVSync)) {
        if (@available(macOS 10.13, *))
            d->layer.displaySyncEnabled = NO;
    }
#endif

    if (m_flags.testFlag(SurfaceHasPreMulAlpha)) {
        d->layer.opaque = NO;
    } else if (m_flags.testFlag(SurfaceHasNonPreMulAlpha)) {
        // The CoreAnimation compositor is said to expect premultiplied alpha,
        // so this is then wrong when it comes to the blending operations but
        // there's nothing we can do. Fortunately Qt Quick always outputs
        // premultiplied alpha so it is not a problem there.
        d->layer.opaque = NO;
    } else {
        d->layer.opaque = YES;
    }

    // Now set the layer's drawableSize which will stay set to the same value
    // until the next buildOrResize(), thus ensuring atomicity with regards to
    // the drawable size in frames.
    CGSize layerSize = d->layer.bounds.size;
    layerSize.width *= d->layer.contentsScale;
    layerSize.height *= d->layer.contentsScale;
    d->layer.drawableSize = layerSize;

    m_currentPixelSize = QSizeF::fromCGSize(layerSize).toSize();
    pixelSize = m_currentPixelSize;

    [d->layer setDevice: rhiD->d->dev];

    d->curDrawable = nil;

    for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i) {
        if (!d->sem[i])
            d->sem[i] = dispatch_semaphore_create(QMTL_FRAMES_IN_FLIGHT - 1);
    }

    currentFrameSlot = 0;
    frameCount = 0;

    ds = m_depthStencil ? QRHI_RES(QMetalRenderBuffer, m_depthStencil) : nullptr;
    if (m_depthStencil && m_depthStencil->sampleCount() != m_sampleCount) {
        qWarning("Depth-stencil buffer's sampleCount (%d) does not match color buffers' sample count (%d). Expect problems.",
                 m_depthStencil->sampleCount(), m_sampleCount);
    }
    if (m_depthStencil && m_depthStencil->pixelSize() != pixelSize) {
        if (m_depthStencil->flags().testFlag(QRhiRenderBuffer::UsedWithSwapChainOnly)) {
            m_depthStencil->setPixelSize(pixelSize);
            if (!m_depthStencil->build())
                qWarning("Failed to rebuild swapchain's associated depth-stencil buffer for size %dx%d",
                         pixelSize.width(), pixelSize.height());
        } else {
            qWarning("Depth-stencil buffer's size (%dx%d) does not match the layer size (%dx%d). Expect problems.",
                     m_depthStencil->pixelSize().width(), m_depthStencil->pixelSize().height(),
                     pixelSize.width(), pixelSize.height());
        }
    }

    rtWrapper.d->pixelSize = pixelSize;
    rtWrapper.d->dpr = float(window->devicePixelRatio());
    rtWrapper.d->sampleCount = samples;
    rtWrapper.d->colorAttCount = 1;
    rtWrapper.d->dsAttCount = ds ? 1 : 0;

    qCDebug(QRHI_LOG_INFO, "got CAMetalLayer, size %dx%d", pixelSize.width(), pixelSize.height());

    if (samples > 1) {
        MTLTextureDescriptor *desc = [[MTLTextureDescriptor alloc] init];
        desc.textureType = MTLTextureType2DMultisample;
        desc.pixelFormat = d->colorFormat;
        desc.width = NSUInteger(pixelSize.width());
        desc.height = NSUInteger(pixelSize.height());
        desc.sampleCount = NSUInteger(samples);
        desc.resourceOptions = MTLResourceStorageModePrivate;
        desc.storageMode = MTLStorageModePrivate;
        desc.usage = MTLTextureUsageRenderTarget;
        for (int i = 0; i < QMTL_FRAMES_IN_FLIGHT; ++i) {
            [d->msaaTex[i] release];
            d->msaaTex[i] = [rhiD->d->dev newTextureWithDescriptor: desc];
        }
        [desc release];
    }

    QRHI_PROF;
    QRHI_PROF_F(resizeSwapChain(this, QMTL_FRAMES_IN_FLIGHT, samples > 1 ? QMTL_FRAMES_IN_FLIGHT : 0, samples));

    if (needsRegistration)
        rhiD->registerResource(this);

    return true;

#ifdef TARGET_IPHONE_SIMULATOR
    } else {
        // Won't ever get here in a normal app because MTLDevice creation would
        // fail too. Print a warning, just in case.
        qWarning("No CAMetalLayer support in this version of the iOS Simulator");
        return false;
    }
#endif
}

QT_END_NAMESPACE
