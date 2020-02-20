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

#include "qrhinull_p_p.h"
#include <qmath.h>
#include <QPainter>

QT_BEGIN_NAMESPACE

/*!
    \class QRhiNullInitParams
    \internal
    \inmodule QtGui
    \brief Null backend specific initialization parameters.

    A Null QRhi needs no special parameters for initialization.

    \badcode
        QRhiNullInitParams params;
        rhi = QRhi::create(QRhi::Null, &params);
    \endcode

    The Null backend does not issue any graphics calls and creates no
    resources. All QRhi operations will succeed as normal so applications can
    still be run, albeit potentially at an unthrottled speed, depending on
    their frame rendering strategy. The backend reports resources to
    QRhiProfiler as usual.
 */

/*!
    \class QRhiNullNativeHandles
    \internal
    \inmodule QtGui
    \brief Empty.
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

QVector<int> QRhiNull::supportedSampleCounts() const
{
    return { 1 };
}

QRhiSwapChain *QRhiNull::createSwapChain()
{
    return new QNullSwapChain(this);
}

QRhiBuffer *QRhiNull::createBuffer(QRhiBuffer::Type type, QRhiBuffer::UsageFlags usage, int size)
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
    default:
        Q_UNREACHABLE();
        return 0;
    }
}

const QRhiNativeHandles *QRhiNull::nativeHandles()
{
    return &nativeHandlesStruct;
}

void QRhiNull::sendVMemStatsToProfiler()
{
    // nothing to do here
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

QRhiRenderBuffer *QRhiNull::createRenderBuffer(QRhiRenderBuffer::Type type, const QSize &pixelSize,
                                               int sampleCount, QRhiRenderBuffer::Flags flags)
{
    return new QNullRenderBuffer(this, type, pixelSize, sampleCount, flags);
}

QRhiTexture *QRhiNull::createTexture(QRhiTexture::Format format, const QSize &pixelSize,
                                     int sampleCount, QRhiTexture::Flags flags)
{
    return new QNullTexture(this, format, pixelSize, sampleCount, flags);
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

QRhi::FrameOpResult QRhiNull::beginFrame(QRhiSwapChain *swapChain, QRhi::BeginFrameFlags flags)
{
    Q_UNUSED(flags);
    currentSwapChain = swapChain;
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();
    QRHI_PROF_F(beginSwapChainFrame(swapChain));
    return QRhi::FrameOpSuccess;
}

QRhi::FrameOpResult QRhiNull::endFrame(QRhiSwapChain *swapChain, QRhi::EndFrameFlags flags)
{
    Q_UNUSED(flags);
    QNullSwapChain *swapChainD = QRHI_RES(QNullSwapChain, swapChain);
    QRhiProfilerPrivate *rhiP = profilerPrivateOrNull();
    QRHI_PROF_F(endSwapChainFrame(swapChain, swapChainD->frameCount + 1));
    QRHI_PROF_F(swapChainFrameGpuTime(swapChain, 0.000666f));
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
    for (int layer = 0; layer < QRhi::MAX_LAYERS; ++layer) {
        for (int level = 0; level < QRhi::MAX_LEVELS; ++level) {
            for (const QRhiTextureSubresourceUploadDescription &subresDesc : qAsConst(u.subresDesc[layer][level])) {
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
                    const QPoint dstOffset = subresDesc.destinationTopLeft();
                    uchar *dst = texD->image[layer][level].bits();
                    const int dstBpl = texD->image[layer][level].bytesPerLine();
                    for (int y = 0; y < h; ++y) {
                        memcpy(dst + dstOffset.x() * 4 + (y + dstOffset.y()) * dstBpl,
                               src + y * srcBpl,
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
    for (const QRhiResourceUpdateBatchPrivate::BufferOp &u : ud->bufferOps) {
        if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::DynamicUpdate
                || u.type == QRhiResourceUpdateBatchPrivate::BufferOp::StaticUpload)
        {
            QNullBuffer *bufD = QRHI_RES(QNullBuffer, u.buf);
            memcpy(bufD->data.data() + u.offset, u.data.constData(), size_t(u.data.size()));
        } else if (u.type == QRhiResourceUpdateBatchPrivate::BufferOp::Read) {
            QRhiBufferReadbackResult *result = u.result;
            result->data.resize(u.readSize);
            QNullBuffer *bufD = QRHI_RES(QNullBuffer, u.buf);
            memcpy(result->data.data(), bufD->data.constData() + u.offset, size_t(u.readSize));
            if (result->completed)
                result->completed();
        }
    }
    for (const QRhiResourceUpdateBatchPrivate::TextureOp &u : ud->textureOps) {
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
            textureFormatInfo(result->format, result->pixelSize, &bytesPerLine, &byteSize);
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
                         QRhiResourceUpdateBatch *resourceUpdates)
{
    Q_UNUSED(rt);
    Q_UNUSED(colorClearValue);
    Q_UNUSED(depthStencilClearValue);
    if (resourceUpdates)
        resourceUpdate(cb, resourceUpdates);
}

void QRhiNull::endPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    if (resourceUpdates)
        resourceUpdate(cb, resourceUpdates);
}

void QRhiNull::beginComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    if (resourceUpdates)
        resourceUpdate(cb, resourceUpdates);
}

void QRhiNull::endComputePass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *resourceUpdates)
{
    if (resourceUpdates)
        resourceUpdate(cb, resourceUpdates);
}

QNullBuffer::QNullBuffer(QRhiImplementation *rhi, Type type, UsageFlags usage, int size)
    : QRhiBuffer(rhi, type, usage, size)
{
}

QNullBuffer::~QNullBuffer()
{
    release();
}

void QNullBuffer::release()
{
    data.clear();

    QRHI_PROF;
    QRHI_PROF_F(releaseBuffer(this));
}

bool QNullBuffer::build()
{
    data.fill('\0', m_size);

    QRHI_PROF;
    QRHI_PROF_F(newBuffer(this, uint(m_size), 1, 0));
    return true;
}

QNullRenderBuffer::QNullRenderBuffer(QRhiImplementation *rhi, Type type, const QSize &pixelSize,
                                     int sampleCount, QRhiRenderBuffer::Flags flags)
    : QRhiRenderBuffer(rhi, type, pixelSize, sampleCount, flags)
{
}

QNullRenderBuffer::~QNullRenderBuffer()
{
    release();
}

void QNullRenderBuffer::release()
{
    QRHI_PROF;
    QRHI_PROF_F(releaseRenderBuffer(this));
}

bool QNullRenderBuffer::build()
{
    QRHI_PROF;
    QRHI_PROF_F(newRenderBuffer(this, false, false, 1));
    return true;
}

QRhiTexture::Format QNullRenderBuffer::backingFormat() const
{
    return m_type == Color ? QRhiTexture::RGBA8 : QRhiTexture::UnknownFormat;
}

QNullTexture::QNullTexture(QRhiImplementation *rhi, Format format, const QSize &pixelSize,
                           int sampleCount, Flags flags)
    : QRhiTexture(rhi, format, pixelSize, sampleCount, flags)
{
}

QNullTexture::~QNullTexture()
{
    release();
}

void QNullTexture::release()
{
    QRHI_PROF;
    QRHI_PROF_F(releaseTexture(this));
}

bool QNullTexture::build()
{
    QRHI_RES_RHI(QRhiNull);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);
    QSize size = m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize;
    const int mipLevelCount = hasMipMaps ? rhiD->q->mipLevelsForSize(size) : 1;
    const int layerCount = isCube ? 6 : 1;

    if (m_format == RGBA8) {
        for (int layer = 0; layer < layerCount; ++layer) {
            for (int level = 0; level < mipLevelCount; ++level) {
                image[layer][level] = QImage(rhiD->q->sizeForMipLevel(level, size),
                                             QImage::Format_RGBA8888_Premultiplied);
                image[layer][level].fill(Qt::yellow);
            }
        }
    }

    QRHI_PROF;
    QRHI_PROF_F(newTexture(this, true, mipLevelCount, layerCount, 1));
    return true;
}

bool QNullTexture::buildFrom(QRhiTexture::NativeTexture src)
{
    Q_UNUSED(src)
    QRHI_RES_RHI(QRhiNull);
    const bool isCube = m_flags.testFlag(CubeMap);
    const bool hasMipMaps = m_flags.testFlag(MipMapped);
    QSize size = m_pixelSize.isEmpty() ? QSize(1, 1) : m_pixelSize;
    const int mipLevelCount = hasMipMaps ? rhiD->q->mipLevelsForSize(size) : 1;
    QRHI_PROF;
    QRHI_PROF_F(newTexture(this, false, mipLevelCount, isCube ? 6 : 1, 1));
    return true;
}

QNullSampler::QNullSampler(QRhiImplementation *rhi, Filter magFilter, Filter minFilter, Filter mipmapMode,
                           AddressMode u, AddressMode v, AddressMode w)
    : QRhiSampler(rhi, magFilter, minFilter, mipmapMode, u, v, w)
{
}

QNullSampler::~QNullSampler()
{
    release();
}

void QNullSampler::release()
{
}

bool QNullSampler::build()
{
    return true;
}

QNullRenderPassDescriptor::QNullRenderPassDescriptor(QRhiImplementation *rhi)
    : QRhiRenderPassDescriptor(rhi)
{
}

QNullRenderPassDescriptor::~QNullRenderPassDescriptor()
{
    release();
}

void QNullRenderPassDescriptor::release()
{
}

bool QNullRenderPassDescriptor::isCompatible(const QRhiRenderPassDescriptor *other) const
{
    Q_UNUSED(other);
    return true;
}

QNullReferenceRenderTarget::QNullReferenceRenderTarget(QRhiImplementation *rhi)
    : QRhiRenderTarget(rhi),
      d(rhi)
{
}

QNullReferenceRenderTarget::~QNullReferenceRenderTarget()
{
    release();
}

void QNullReferenceRenderTarget::release()
{
}

QSize QNullReferenceRenderTarget::pixelSize() const
{
    return d.pixelSize;
}

float QNullReferenceRenderTarget::devicePixelRatio() const
{
    return d.dpr;
}

int QNullReferenceRenderTarget::sampleCount() const
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
    release();
}

void QNullTextureRenderTarget::release()
{
}

QRhiRenderPassDescriptor *QNullTextureRenderTarget::newCompatibleRenderPassDescriptor()
{
    return new QNullRenderPassDescriptor(m_rhi);
}

bool QNullTextureRenderTarget::build()
{
    d.rp = QRHI_RES(QNullRenderPassDescriptor, m_renderPassDesc);
    if (m_desc.cbeginColorAttachments() != m_desc.cendColorAttachments()) {
        QRhiTexture *tex = m_desc.cbeginColorAttachments()->texture();
        QRhiRenderBuffer *rb = m_desc.cbeginColorAttachments()->renderBuffer();
        d.pixelSize = tex ? tex->pixelSize() : rb->pixelSize();
    } else if (m_desc.depthStencilBuffer()) {
        d.pixelSize = m_desc.depthStencilBuffer()->pixelSize();
    } else if (m_desc.depthTexture()) {
        d.pixelSize = m_desc.depthTexture()->pixelSize();
    }
    return true;
}

QSize QNullTextureRenderTarget::pixelSize() const
{
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
    release();
}

void QNullShaderResourceBindings::release()
{
}

bool QNullShaderResourceBindings::build()
{
    return true;
}

QNullGraphicsPipeline::QNullGraphicsPipeline(QRhiImplementation *rhi)
    : QRhiGraphicsPipeline(rhi)
{
}

QNullGraphicsPipeline::~QNullGraphicsPipeline()
{
    release();
}

void QNullGraphicsPipeline::release()
{
}

bool QNullGraphicsPipeline::build()
{
    QRHI_RES_RHI(QRhiNull);
    if (!rhiD->sanityCheckGraphicsPipeline(this))
        return false;

    return true;
}

QNullComputePipeline::QNullComputePipeline(QRhiImplementation *rhi)
    : QRhiComputePipeline(rhi)
{
}

QNullComputePipeline::~QNullComputePipeline()
{
    release();
}

void QNullComputePipeline::release()
{
}

bool QNullComputePipeline::build()
{
    return true;
}

QNullCommandBuffer::QNullCommandBuffer(QRhiImplementation *rhi)
    : QRhiCommandBuffer(rhi)
{
}

QNullCommandBuffer::~QNullCommandBuffer()
{
    release();
}

void QNullCommandBuffer::release()
{
    // nothing to do here
}

QNullSwapChain::QNullSwapChain(QRhiImplementation *rhi)
    : QRhiSwapChain(rhi),
      rt(rhi),
      cb(rhi)
{
}

QNullSwapChain::~QNullSwapChain()
{
    release();
}

void QNullSwapChain::release()
{
    QRHI_PROF;
    QRHI_PROF_F(releaseSwapChain(this));
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

QRhiRenderPassDescriptor *QNullSwapChain::newCompatibleRenderPassDescriptor()
{
    return new QNullRenderPassDescriptor(m_rhi);
}

bool QNullSwapChain::buildOrResize()
{
    m_currentPixelSize = surfacePixelSize();
    rt.d.rp = QRHI_RES(QNullRenderPassDescriptor, m_renderPassDesc);
    rt.d.pixelSize = m_currentPixelSize;
    frameCount = 0;
    QRHI_PROF;
    QRHI_PROF_F(resizeSwapChain(this, 1, 0, 1));
    return true;
}

QT_END_NAMESPACE
