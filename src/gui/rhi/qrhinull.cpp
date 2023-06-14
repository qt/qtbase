// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhinull_p.h"
#include <qmath.h>
#include <QPainter>

QT_BEGIN_NAMESPACE

/*!
    \class QRhiNullInitParams
    \inmodule QtGui
    \since 6.6
    \brief Null backend specific initialization parameters.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.

    A Null QRhi needs no special parameters for initialization.

    \badcode
        QRhiNullInitParams params;
        rhi = QRhi::create(QRhi::Null, &params);
    \endcode

    The Null backend does not issue any graphics calls and creates no
    resources. All QRhi operations will succeed as normal so applications can
    still be run, albeit potentially at an unthrottled speed, depending on
    their frame rendering strategy.
 */

/*!
    \class QRhiNullNativeHandles
    \inmodule QtGui
    \since 6.6
    \brief Empty.

    \note This is a RHI API with limited compatibility guarantees, see \l QRhi
    for details.
 */

QRhiNull::QRhiNull(QRhiNullInitParams *params)
    : offscreenCommandBuffer(this)
{
    Q_UNUSED(params);
}

bool QRhiNull::create(QRhi::Flags flags)
{
    Q_UNUSED(flags);
    return true;
}

void QRhiNull::destroy()
{
}

QList<int> QRhiNull::supportedSampleCounts() const
{
    return { 1 };
}

QRhiSwapChain *QRhiNull::createSwapChain()
{
    return new QNullSwapChain(this);
}

QRhiBuffer *QRhiNull::createBuffer(QRhiBuffer::Type type, QRhiBuffer::UsageFlags usage, quint32 size)
{
    return new QNullBuffer(this, type, usage, size);
}

int QRhiNull::ubufAlignment() const
{
    return 256;
}

bool QRhiNull::isYUpInFramebuffer() const
{
    return false;
}

bool QRhiNull::isYUpInNDC() const
{
    return true;
}

bool QRhiNull::isClipDepthZeroToOne() const
{
    return true;
}

QMatrix4x4 QRhiNull::clipSpaceCorrMatrix() const
{
    return QMatrix4x4(); // identity
}

bool QRhiNull::isTextureFormatSupported(QRhiTexture::Format format, QRhiTexture::Flags flags) const
{
    Q_UNUSED(format);
    Q_UNUSED(flags);
    return true;
}

bool QRhiNull::isFeatureSupported(QRhi::Feature feature) const
{
    Q_UNUSED(feature);
    return true;
}

int QRhiNull::resourceLimit(QRhi::ResourceLimit limit) const
{
    switch (limit) {
    case QRhi::TextureSizeMin:
        return 1;
    case QRhi::TextureSizeMax:
        return 16384;
    case QRhi::MaxColorAttachments:
        return 8;
    case QRhi::FramesInFlight:
        return 1;
    case QRhi::MaxAsyncReadbackFrames:
        return 1;
    case QRhi::MaxThreadGroupsPerDimension:
        return 0;
    case QRhi::MaxThreadsPerThreadGroup:
        return 0;
    case QRhi::MaxThreadGroupX:
        return 0;
    case QRhi::MaxThreadGroupY:
        return 0;
    case QRhi::MaxThreadGroupZ:
        return 0;
    case QRhi::TextureArraySizeMax:
        return 2048;
    case QRhi::MaxUniformBufferRange:
        return 65536;
    case QRhi::MaxVertexInputs:
        return 32;
    case QRhi::MaxVertexOutputs:
        return 32;
    }

    Q_UNREACHABLE_RETURN(0);
}

const QRhiNativeHandles *QRhiNull::nativeHandles()
{
    return &nativeHandlesStruct;
}

QRhiDriverInfo QRhiNull::driverInfo() const
{
    QRhiDriverInfo info;
    info.deviceName = QByteArrayLiteral("Null");
    return info;
}

QRhiStats QRhiNull::statistics()
{
    return {};
}

bool QRhiNull::makeThreadLocalNativeContextCurrent()
{
    // not applicable
    return false;
}

void QRhiNull::releaseCachedResources()
{
    // nothing to do here
}

bool QRhiNull::isDeviceLost() const
{
    return false;
}

QByteArray QRhiNull::pipelineCacheData()
{
    return QByteArray();
}

void QRhiNull::setPipelineCacheData(const QByteArray &data)
{
    Q_UNUSED(data);
}

QRhiRenderBuffer *QRhiNull::createRenderBuffer(QRhiRenderBuffer::Type type, const QSize &pixelSize,
                                               int sampleCount, QRhiRenderBuffer::Flags flags,
                                               QRhiTexture::Format backingFormatHint)
{
    return new QNullRenderBuffer(this, type, pixelSize, sampleCount, flags, backingFormatHint);
}

QRhiTexture *QRhiNull::createTexture(QRhiTexture::Format format,
                                     const QSize &pixelSize, int depth, int arraySize,
                                     int sampleCount, QRhiTexture::Flags flags)
{
    return new QNullTexture(this, format, pixelSize, depth, arraySize, sampleCount, flags);
}

QRhiSampler *QRhiNull::createSampler(QRhiSampler::Filter magFilter, QRhiSampler::Filter minFilter,
                                     QRhiSampler::Filter mipmapMode,
                                     QRhiSampler::AddressMode u, QRhiSampler::AddressMode v, QRhiSampler::AddressMode w)
{
    return new QNullSampler(this, magFilter, minFilter, mipmapMode, u, v, w);
}

QRhiTextureRenderTarget *QRhiNull::createTextureRenderTarget(const QRhiTextureRenderTargetDescription &desc,
                                                             QRhiTextureRenderTarget::Flags flags)
{
    return new QNullTextureRenderTarget(this, desc, flags);
}

QRhiGraphicsPipeline *QRhiNull::createGraphicsPipeline()
{
    return new QNullGraphicsPipeline(this);
}

QRhiComputePipeline *QRhiNull::createComputePipeline()
{
    return new QNullComputePipeline(this);
}

QRhiShaderResourceBindings *QRhiNull::createShaderResourceBindings()
{
    return new QNullShaderResourceBindings(this);
}

void QRhiNull::setGraphicsPipeline(QRhiCommandBuffer *cb, QRhiGraphicsPipeline *ps)
{
    Q_UNUSED(cb);
    Q_UNUSED(ps);
}

void QRhiNull::setShaderResources(QRhiCommandBuffer *cb, QRhiShaderResourceBindings *srb,
                                  int dynamicOffsetCount,
                                  const QRhiCommandBuffer::DynamicOffset *dynamicOffsets)
{
    Q_UNUSED(cb);
    Q_UNUSED(srb);
    Q_UNUSED(dynamicOffsetCount);
    Q_UNUSED(dynamicOffsets);
}

void QRhiNull::setVertexInput(QRhiCommandBuffer *cb,
                              int startBinding, int bindingCount, const QRhiCommandBuffer::VertexInput *bindings,
                              QRhiBuffer *indexBuf, quint32 indexOffset, QRhiCommandBuffer::IndexFormat indexFormat)
{
    Q_UNUSED(cb);
    Q_UNUSED(startBinding);
    Q_UNUSED(bindingCount);
    Q_UNUSED(bindings);
    Q_UNUSED(indexBuf);
    Q_UNUSED(indexOffset);
    Q_UNUSED(indexFormat);
}

void QRhiNull::setViewport(QRhiCommandBuffer *cb, const QRhiViewport &viewport)
{
    Q_UNUSED(cb);
    Q_UNUSED(viewport);
}

void QRhiNull::setScissor(QRhiCommandBuffer *cb, const QRhiScissor &scissor)
{
    Q_UNUSED(cb);
    Q_UNUSED(scissor);
}

void QRhiNull::setBlendConstants(QRhiCommandBuffer *cb, const QColor &c)
{
    Q_UNUSED(cb);
    Q_UNUSED(c);
}

void QRhiNull::setStencilRef(QRhiCommandBuffer *cb, quint32 refValue)
{
    Q_UNUSED(cb);
    Q_UNUSED(refValue);
}

void QRhiNull::draw(QRhiCommandBuffer *cb, quint32 vertexCount,
                    quint32 instanceCount, quint32 firstVertex, quint32 firstInstance)
{
    Q_UNUSED(cb);
    Q_UNUSED(vertexCount);
    Q_UNUSED(instanceCount);
    Q_UNUSED(firstVertex);
    Q_UNUSED(firstInstance);
}

void QRhiNull::drawIndexed(QRhiCommandBuffer *cb, quint32 indexCount,
                           quint32 instanceCount, quint32 firstIndex, qint32 vertexOffset, quint32 firstInstance)
{
    Q_UNUSED(cb);
    Q_UNUSED(indexCount);
    Q_UNUSED(instanceCount);
    Q_UNUSED(firstIndex);
    Q_UNUSED(vertexOffset);
    Q_UNUSED(firstInstance);
}

void QRhiNull::debugMarkBegin(QRhiCommandBuffer *cb, const QByteArray &name)
{
    Q_UNUSED(cb);
    Q_UNUSED(name);
}

void QRhiNull::debugMarkEnd(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}

void QRhiNull::debugMarkMsg(QRhiCommandBuffer *cb, const QByteArray &msg)
{
    Q_UNUSED(cb);
    Q_UNUSED(msg);
}

void QRhiNull::setComputePipeline(QRhiCommandBuffer *cb, QRhiComputePipeline *ps)
{
    Q_UNUSED(cb);
    Q_UNUSED(ps);
}

void QRhiNull::dispatch(QRhiCommandBuffer *cb, int x, int y, int z)
{
    Q_UNUSED(cb);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);
}

const QRhiNativeHandles *QRhiNull::nativeHandles(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
    return nullptr;
}

void QRhiNull::beginExternal(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}

void QRhiNull::endExternal(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
}

double QRhiNull::lastCompletedGpuTime(QRhiCommandBuffer *cb)
{
    Q_UNUSED(cb);
    return 0;
}

QRhi::FrameOpResult QRhiNull::beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);
    currentSwapChain = swapChain;
    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiNull::endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    QNullSwapChain *swapChainD = QRHI_RES(QNullSwapChain, swapChain);
    swapChainD->frameCount += 1;
    currentSwapChain = nullptr;
    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiNull::beginOffscreenFrame(QRhiCommandBuffer **cb, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);
    *cb = &offscreenCommandBuffer;
    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiNull::endOffscreenFrame(QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiNull::finish()
{
    return QRhi::FrameOpSuccess;
}

void QRhiNull::simulateTextureUpload(const QRhiResourceUpdateBatchPrivate::TextureOp &u)
{
    QNullTexture *texD = QRHI_RES(QNullTexture, u.dst);
    for (int layer = 0, maxLayer = u.subresDesc.size(); layer < maxLayer; ++layer) {
        for (int level = 0; level < QRhi::MAX_MIP_LEVELS; ++level) {
            for (const QRhiTextureSubresourceUploadDescription &subresDesc : std::as_const(u.subresDesc[layer][level])) {
                if (!subresDesc.image().isNull()) {
                    const QImage src = subresDesc.image();
                    QPainter painter(&texD->image[layer][level]);
                    const QSize srcSize = subresDesc.sourceSize().isEmpty()
                            ? src.size() : subresDesc.sourceSize();
                    painter.setCompositionMode(QPainter::CompositionMode_Source);
                    painter.drawImage(subresDesc.destinationTopLeft(), src,
                                      QRect(subresDesc.sourceTopLeft(), srcSize));
                } else if (!subresDesc.data().isEmpty()) {
                    const QSize subresSize = q->sizeForMipLevel(level, texD->pixelSize());
                    int w = subresSize.width();
                    int h = subresSize.height();
                    if (!subresDesc.sourceSize().isEmpty()) {
                        w = subresDesc.sourceSize().width();
                        h = subresDesc.sourceSize().height();
                    }
                    // sourceTopLeft is not supported on this path as per QRhi docs
                    const char *src = subresDesc.data().constData();
                    const int srcBpl = w * 4;
                    int srcStride = srcBpl;
                    if (subresDesc.dataStride())
                        srcStride = subresDesc.dataStride();
                    const QPoint dstOffset = subresDesc.destinationTopLeft();
                    uchar *dst = texD->image[layer][level].bits();
                    const int dstBpl = texD->image[layer][level].bytesPerLine();
                    for (int y = 0; y < h; ++y) {
                        memcpy(dst + dstOffset.x() * 4 + (y + dstOffset.y()) * dstBpl,
                               src + y * srcStride,
                               size_t(srcBpl));
                    }
                }
            }
        }
    }
}

void QRhiNull::simulateTextureCopy(const QRhiResourceUpdateBatchPrivate::TextureOp &u)
{
    QNullTexture *srcD = QRHI_RES(QNullTexture, u.src);
    QNullTexture *dstD = QRHI_RES(QNullTexture, u.dst);
    const QImage &srcImage(srcD->image[u.desc.sourceLayer()][u.desc.sourceLevel()]);
    QImage &dstImage(dstD->image[u.desc.destinationLayer()][u.desc.destinationLevel()]);
    const QPoint dstPos = u.desc.destinationTopLeft();
    const QSize size = u.desc.pixelSize().isEmpty() ? srcD->pixelSize() : u.desc.pixelSize();
    const QPoint srcPos = u.desc.sourceTopLeft();

    QPainter painter(&dstImage);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.drawImage(QRect(dstPos, size), srcImage, QRect(srcPos, size));
}

void QRhiNull::simulateTextureGenMips(const QRhiResourceUpdateBatchPrivate::TextureOp &u)
{
    QNullTexture *texD = QRHI_RES(QNullTexture, u.dst);
    const QSize baseSize = texD->pixelSize();
    const int levelCount = q->mipLevelsForSize(baseSize);
    for (int level = 1; level < levelCount; ++level)
        texD->image[0][level] = texD->image[0][0].scaled(q->sizeForMipLevel(level, baseSize));
}

void QRhiNull::resourceUpdate(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_UNUSED(cb);
    QRhiResourceUpdateBatchPrivate *ud = QRhiResourceUpdateBatchPrivate::get(resourceUpdates);
    for (int opIdx = 0; opIdx < ud->activeBufferOpCount; ++opIdx) {
        const QRhiResourceUpdateBatchPrivate::BufferOp &u(ud->bufferOps[opIdx]);
        if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::DynamicUpdate
                || u.type == QRhiResourceUpdateBatchPrivate::BufferOp::StaticUpload)
        {
            QNullBuffer *bufD = QRHI_RES(QNullBuffer, u.buf);
            memcpy(bufD->data + u.offset, u.data.constData(), size_t(u.data.size()));
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::Read) {
            QRhiReadbackResult *result = u.result;
            result->data.resize(u.readSize);
            QNullBuffer *bufD = QRHI_RES(QNullBuffer, u.buf);
            memcpy(result->data.data(), bufD->data + u.offset, size_t(u.readSize));
            if (result->completed)
                result->completed();
        }
    }
    for (int opIdx = 0; opIdx < ud->activeTextureOpCount; ++opIdx) {
        const QRhiResourceUpdateBatchPrivate::TextureOp &u(ud->textureOps[opIdx]);
        if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Upload) {
            if (u.dst->format() == QRhiTexture::RGBA8)
                simulateTextureUpload(u);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Copy) {
            if (u.src->format() == QRhiTexture::RGBA8 && u.dst->format() == QRhiTexture::RGBA8)
                simulateTextureCopy(u);
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::Read) {
            QRhiReadbackResult *result = u.result;
            QNullTexture *texD = QRHI_RES(QNullTexture, u.rb.texture());
            if (texD) {
                result->format = texD->format();
                result->pixelSize = q->sizeForMipLevel(u.rb.level(), texD->pixelSize());
            } else {
                Q_ASSERT(currentSwapChain);
                result->format = QRhiTexture::RGBA8;
                result->pixelSize = currentSwapChain->currentPixelSize();
            }
            quint32 bytesPerLine = 0;
            quint32 byteSize = 0;
            textureFormatInfo(result->format, result->pixelSize, &bytesPerLine, &byteSize, nullptr);
            if (texD && texD->format() == QRhiTexture::RGBA8) {
                result->data.resize(int(byteSize));
                const QImage &src(texD->image[u.rb.layer()][u.rb.level()]);
                char *dst = result->data.data();
                for (int y = 0, h = src.height(); y < h; ++y) {
                    memcpy(dst, src.constScanLine(y), bytesPerLine);
                    dst += bytesPerLine;
                }
            } else {
                result->data.fill(0, int(byteSize));
            }
            if (result->completed)
                result->completed();
        } else if (u.type == QRhiResourceUpdateBatchPrivate::TextureOp::GenMips) {
            if (u.dst->format() == QRhiTexture::RGBA8)
                simulateTextureGenMips(u);
        }
    }
    ud->free();
}

void QRhiNull::beginPass(QRhiCommandBuffer *cb,
                         QRhiRenderTarget *rt,
                         const QColor &colorClearValue,
                         const QRhiDepthStencilClearValue &depthStencilClearValue,
                         QRhiResourceUpdateBatch *resourceUpdates,
                         QRhiCommandBuffer::BeginPassFlags flags)
{
    Q_UNUSED(colorClearValue);
    Q_UNUSED(depthStencilClearValue);
    Q_UNUSED(flags);

    if (resourceUpdates)
        resourceUpdate(cb, resourceUpdates);

    if (rt->resourceType() == QRhiRenderTarget::TextureRenderTarget) {
        QNullTextureRenderTarget *rtTex = QRHI_RES(QNullTextureRenderTarget, rt);
        if (!QRhiRenderTargetAttachmentTracker::isUpToDate<QNullTexture, QNullRenderBuffer>(rtTex->description(), rtTex->d.currentResIdList))
            rtTex->create();
    }
}

void QRhiNull::endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    if (resourceUpdates)
        resourceUpdate(cb, resourceUpdates);
}

void QRhiNull::beginComputePass(QRhiCommandBuffer *cb,
                                QRhiResourceUpdateBatch *resourceUpdates,
                                QRhiCommandBuffer::BeginPassFlags flags)
{
    Q_UNUSED(flags);
    if (resourceUpdates)
        resourceUpdate(cb, resourceUpdates);
}

void QRhiNull::endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    if (resourceUpdates)
        resourceUpdate(cb, resourceUpdates);
}

QNullBuffer::QNullBuffer(QRhiImplementation *rhi, Type type, UsageFlags usage, quint32 size)
    : QRhiBuffer(rhi, type, usage, size)
{
}

QNullBuffer::~QNullBuffer()
{
    destroy();
}

void QNullBuffer::destroy()
{
    delete[] data;
    data = nullptr;

    QRHI_RES_RHI(QRhiNull);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QNullBuffer::create()
{
    if (data)
        destroy();

    data = new char[m_size];
    memset(data, 0, m_size);

    QRHI_RES_RHI(QRhiNull);
    rhiD->registerResource(this);

    return true;
}

char *QNullBuffer::beginFullDynamicBufferUpdateForCurrentFrame()
{
    Q_ASSERT(m_type == Dynamic);
    return data;
}

QNullRenderBuffer::QNullRenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                                     int sampleCount, QRhiRenderBuffer::Flags flags,
                                     QRhiTexture::Format backingFormatHint)
    : QRhiRenderBuffer(rhi, type, pixelSize, sampleCount, flags, backingFormatHint)
{
}

QNullRenderBuffer::~QNullRenderBuffer()
{
    destroy();
}

void QNullRenderBuffer::destroy()
{
    valid = false;

    QRHI_RES_RHI(QRhiNull);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QNullRenderBuffer::create()
{
    if (valid)
        destroy();

    valid = true;
    generation += 1;

    QRHI_RES_RHI(QRhiNull);
    rhiD->registerResource(this);

    return true;
}

QRhiTexture::Format QNullRenderBuffer::backingFormat() const
{
    return m_type == Color ? QRhiTexture::RGBA8 : QRhiTexture::UnknownFormat;
}

QNullTexture::QNullTexture(QRhiImplementation *rhi, Format format, const QSize &pixelSize, int depth,
                           int arraySize, int sampleCount, Flags flags)
    : QRhiTexture(rhi, format, pixelSize, depth, arraySize, sampleCount, flags)
{
}

QNullTexture::~QNullTexture()
{
    destroy();
}

void QNullTexture::destroy()
{
    valid = false;

    QRHI_RES_RHI(QRhiNull);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QNullTexture::create()
{
    if (valid)
        destroy();

    valid = true;

    QRHI_RES_RHI(QRhiNull);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool is3D = m_flags.testFlag(ThreeDimensional);
    const bool isArray = m_flags.testFlag(TextureArray);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);
    const bool is1D = m_flags.testFlags(OneDimensional);
    QSize size = is1D ? QSize(qMax(1, m_pixelSize.width()), 1)
                      : (m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize);
    const int mipLevelCount = hasMipMaps ? rhiD->q->mipLevelsForSize(size) : 1;
    const int layerCount = is3D ? qMax(1, m_depth)
                                : (isCube ? 6
                                          : (isArray ? qMax(0, m_arraySize)
                                                     : 1));

    if (m_format == RGBA8) {
        image.resize(layerCount);
        for (int layer = 0; layer < layerCount; ++layer) {
            for (int level = 0; level < mipLevelCount; ++level) {
                image[layer][level] = QImage(rhiD->q->sizeForMipLevel(level, size),
                                             QImage::Format_RGBA8888_Premultiplied);
                image[layer][level].fill(Qt::yellow);
            }
        }
    }

    generation += 1;

    rhiD->registerResource(this);

    return true;
}

bool QNullTexture::createFrom(QRhiTexture::NativeTexture src)
{
    Q_UNUSED(src);
    if (valid)
        destroy();

    valid = true;

    generation += 1;

    QRHI_RES_RHI(QRhiNull);
    rhiD->registerResource(this);

    return true;
}

QNullSampler::QNullSampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                           AddressMode u, AddressMode v, AddressMode w)
    : QRhiSampler(rhi, magFilter, minFilter, mipmapMode, u, v, w)
{
}

QNullSampler::~QNullSampler()
{
    destroy();
}

void QNullSampler::destroy()
{
    QRHI_RES_RHI(QRhiNull);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QNullSampler::create()
{
    QRHI_RES_RHI(QRhiNull);
    rhiD->registerResource(this);
    return true;
}

QNullRenderPassDescriptor::QNullRenderPassDescriptor(QRhiImplementation *rhi)
    : QRhiRenderPassDescriptor(rhi)
{
}

QNullRenderPassDescriptor::~QNullRenderPassDescriptor()
{
    destroy();
}

void QNullRenderPassDescriptor::destroy()
{
    QRHI_RES_RHI(QRhiNull);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QNullRenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const
{
    Q_UNUSED(other);
    return true;
}

QRhiRenderPassDescriptor *QNullRenderPassDescriptor::newCompatibleRenderPassDescriptor() const
{
    QNullRenderPassDescriptor *rpD = new QNullRenderPassDescriptor(m_rhi);
    QRHI_RES_RHI(QRhiNull);
    rhiD->registerResource(rpD, false);
    return rpD;
}

QVector<quint32> QNullRenderPassDescriptor::serializedFormat() const
{
    return {};
}

QNullSwapChainRenderTarget::QNullSwapChainRenderTarget(QRhiImplementation *rhi, QRhiSwapChain *swapchain)
    : QRhiSwapChainRenderTarget(rhi, swapchain),
      d(rhi)
{
}

QNullSwapChainRenderTarget::~QNullSwapChainRenderTarget()
{
    destroy();
}

void QNullSwapChainRenderTarget::destroy()
{
}

QSize QNullSwapChainRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QNullSwapChainRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QNullSwapChainRenderTarget::sampleCount() const
{
    return 1;
}

QNullTextureRenderTarget::QNullTextureRenderTarget(QRhiImplementation *rhi,
                                                   const QRhiTextureRenderTargetDescription &desc,
                                                   Flags flags)
    : QRhiTextureRenderTarget(rhi, desc, flags),
      d(rhi)
{
}

QNullTextureRenderTarget::~QNullTextureRenderTarget()
{
    destroy();
}

void QNullTextureRenderTarget::destroy()
{
    QRHI_RES_RHI(QRhiNull);
    if (rhiD)
        rhiD->unregisterResource(this);
}

QRhiRenderPassDescriptor *QNullTextureRenderTarget::newCompatibleRenderPassDescriptor()
{
    QNullRenderPassDescriptor *rpD = new QNullRenderPassDescriptor(m_rhi);
    QRHI_RES_RHI(QRhiNull);
    rhiD->registerResource(rpD, false);
    return rpD;
}

bool QNullTextureRenderTarget::create()
{
    QRHI_RES_RHI(QRhiNull);
    d.rp = QRHI_RES(QNullRenderPassDescriptor, m_renderPassDesc);
    if (m_desc.cbeginColorAttachments() != m_desc.cendColorAttachments()) {
        const QRhiColorAttachment *colorAtt = m_desc.cbeginColorAttachments();
        QRhiTexture *tex = colorAtt->texture();
        QRhiRenderBuffer *rb = colorAtt->renderBuffer();
        d.pixelSize = tex ? rhiD->q->sizeForMipLevel(colorAtt->level(), tex->pixelSize()) : rb->pixelSize();
    } else if (m_desc.depthStencilBuffer()) {
        d.pixelSize = m_desc.depthStencilBuffer()->pixelSize();
    } else if (m_desc.depthTexture()) {
        d.pixelSize = m_desc.depthTexture()->pixelSize();
    }
    QRhiRenderTargetAttachmentTracker::updateResIdList<QNullTexture, QNullRenderBuffer>(m_desc, &d.currentResIdList);
    rhiD->registerResource(this);
    return true;
}

QSize QNullTextureRenderTarget::pixelSize() const
{
    if (!QRhiRenderTargetAttachmentTracker::isUpToDate<QNullTexture, QNullRenderBuffer>(m_desc, d.currentResIdList))
        const_cast<QNullTextureRenderTarget *>(this)->create();

    return d.pixelSize;
}

float QNullTextureRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QNullTextureRenderTarget::sampleCount() const
{
    return 1;
}

QNullShaderResourceBindings::QNullShaderResourceBindings(QRhiImplementation *rhi)
    : QRhiShaderResourceBindings(rhi)
{
}

QNullShaderResourceBindings::~QNullShaderResourceBindings()
{
    destroy();
}

void QNullShaderResourceBindings::destroy()
{
    QRHI_RES_RHI(QRhiNull);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QNullShaderResourceBindings::create()
{
    QRHI_RES_RHI(QRhiNull);
    if (!rhiD->sanityCheckShaderResourceBindings(this))
        return false;

    rhiD->updateLayoutDesc(this);

    rhiD->registerResource(this, false);
    return true;
}

void QNullShaderResourceBindings::updateResources(UpdateFlags flags)
{
    Q_UNUSED(flags);
}

QNullGraphicsPipeline::QNullGraphicsPipeline(QRhiImplementation *rhi)
    : QRhiGraphicsPipeline(rhi)
{
}

QNullGraphicsPipeline::~QNullGraphicsPipeline()
{
    destroy();
}

void QNullGraphicsPipeline::destroy()
{
    QRHI_RES_RHI(QRhiNull);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QNullGraphicsPipeline::create()
{
    QRHI_RES_RHI(QRhiNull);
    if (!rhiD->sanityCheckGraphicsPipeline(this))
        return false;

    rhiD->registerResource(this);
    return true;
}

QNullComputePipeline::QNullComputePipeline(QRhiImplementation *rhi)
    : QRhiComputePipeline(rhi)
{
}

QNullComputePipeline::~QNullComputePipeline()
{
    destroy();
}

void QNullComputePipeline::destroy()
{
    QRHI_RES_RHI(QRhiNull);
    if (rhiD)
        rhiD->unregisterResource(this);
}

bool QNullComputePipeline::create()
{
    QRHI_RES_RHI(QRhiNull);
    rhiD->registerResource(this);
    return true;
}

QNullCommandBuffer::QNullCommandBuffer(QRhiImplementation *rhi)
    : QRhiCommandBuffer(rhi)
{
}

QNullCommandBuffer::~QNullCommandBuffer()
{
    destroy();
}

void QNullCommandBuffer::destroy()
{
    // nothing to do here
}

QNullSwapChain::QNullSwapChain(QRhiImplementation *rhi)
    : QRhiSwapChain(rhi),
      rt(rhi, this),
      cb(rhi)
{
}

QNullSwapChain::~QNullSwapChain()
{
    destroy();
}

void QNullSwapChain::destroy()
{
    QRHI_RES_RHI(QRhiNull);
    if (rhiD)
        rhiD->unregisterResource(this);
}

QRhiCommandBuffer *QNullSwapChain::currentFrameCommandBuffer()
{
    return &cb;
}

QRhiRenderTarget *QNullSwapChain::currentFrameRenderTarget()
{
    return &rt;
}

QSize QNullSwapChain::surfacePixelSize()
{
    return QSize(1280, 720);
}

bool QNullSwapChain::isFormatSupported(Format f)
{
    return f == SDR;
}

QRhiRenderPassDescriptor *QNullSwapChain::newCompatibleRenderPassDescriptor()
{
    QNullRenderPassDescriptor *rpD = new QNullRenderPassDescriptor(m_rhi);
    QRHI_RES_RHI(QRhiNull);
    rhiD->registerResource(rpD, false);
    return rpD;
}

bool QNullSwapChain::createOrResize()
{
    const bool needsRegistration = !window || window != m_window;
    if (window && window != m_window)
        destroy();

    window = m_window;
    m_currentPixelSize = surfacePixelSize();
    rt.setRenderPassDescriptor(m_renderPassDesc); // for the public getter in QRhiRenderTarget
    rt.d.rp = QRHI_RES(QNullRenderPassDescriptor, m_renderPassDesc);
    rt.d.pixelSize = m_currentPixelSize;
    frameCount = 0;

    if (needsRegistration) {
        QRHI_RES_RHI(QRhiNull);
        rhiD->registerResource(this);
    }

    return true;
}

QT_END_NAMESPACE
