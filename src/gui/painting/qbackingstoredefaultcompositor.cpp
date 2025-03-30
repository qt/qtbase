// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbackingstoredefaultcompositor_p.h"
#include <QtGui/private/qwindow_p.h>
#include <qpa/qplatformgraphicsbuffer.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QBackingStoreDefaultCompositor::~QBackingStoreDefaultCompositor()
{
    reset();
}

void QBackingStoreDefaultCompositor::reset()
{
    m_rhi = nullptr;
    delete m_psNoBlend;
    m_psNoBlend = nullptr;
    delete m_psBlend;
    m_psBlend = nullptr;
    delete m_psPremulBlend;
    m_psPremulBlend = nullptr;
    delete m_samplerNearest;
    m_samplerNearest = nullptr;
    delete m_samplerLinear;
    m_samplerLinear = nullptr;
    delete m_vbuf;
    m_vbuf = nullptr;
    delete m_texture;
    m_texture = nullptr;
    m_widgetQuadData.reset();
    for (PerQuadData &d : m_textureQuadData)
        d.reset();
}

QRhiTexture *QBackingStoreDefaultCompositor::toTexture(const QPlatformBackingStore *backingStore,
                                                       QRhi *rhi,
                                                       QRhiResourceUpdateBatch *resourceUpdates,
                                                       const QRegion &dirtyRegion,
                                                       QPlatformBackingStore::TextureFlags *flags) const
{
    return toTexture(backingStore->toImage(), rhi, resourceUpdates, dirtyRegion, flags);
}

QRhiTexture *QBackingStoreDefaultCompositor::toTexture(const QImage &sourceImage,
                                                       QRhi *rhi,
                                                       QRhiResourceUpdateBatch *resourceUpdates,
                                                       const QRegion &dirtyRegion,
                                                       QPlatformBackingStore::TextureFlags *flags) const
{
    Q_ASSERT(rhi);
    Q_ASSERT(resourceUpdates);
    Q_ASSERT(flags);

    if (!m_rhi) {
        m_rhi = rhi;
    } else if (m_rhi != rhi) {
        qWarning("QBackingStoreDefaultCompositor: the QRhi has changed unexpectedly, this should not happen");
        return nullptr;
    }

    QImage image = sourceImage;

    bool needsConversion = false;
    *flags = {};

    switch (image.format()) {
    case QImage::Format_ARGB32_Premultiplied:
        *flags |= QPlatformBackingStore::TexturePremultiplied;
        Q_FALLTHROUGH();
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        *flags |= QPlatformBackingStore::TextureSwizzle;
        break;
    case QImage::Format_RGBA8888_Premultiplied:
        *flags |= QPlatformBackingStore::TexturePremultiplied;
        Q_FALLTHROUGH();
    case QImage::Format_RGBX8888:
    case QImage::Format_RGBA8888:
        break;
    case QImage::Format_BGR30:
    case QImage::Format_A2BGR30_Premultiplied:
        // no fast path atm
        needsConversion = true;
        break;
    case QImage::Format_RGB30:
    case QImage::Format_A2RGB30_Premultiplied:
        // no fast path atm
        needsConversion = true;
        break;
    default:
        needsConversion = true;
        break;
    }

    if (image.size().isEmpty())
        return nullptr;

    const bool resized = !m_texture || m_texture->pixelSize() != image.size();
    if (dirtyRegion.isEmpty() && !resized)
        return m_texture;

    if (needsConversion)
        image = image.convertToFormat(QImage::Format_RGBA8888);
    else
        image.detach(); // if it was just wrapping data, that's no good, we need ownership, so detach

    if (resized) {
        if (!m_texture)
            m_texture = rhi->newTexture(QRhiTexture::RGBA8, image.size());
        else
            m_texture->setPixelSize(image.size());
        m_texture->create();
        resourceUpdates->uploadTexture(m_texture, image);
    } else {
        QRect imageRect = image.rect();
        QRect rect = dirtyRegion.boundingRect() & imageRect;
        QRhiTextureSubresourceUploadDescription subresDesc(image);
        subresDesc.setSourceTopLeft(rect.topLeft());
        subresDesc.setSourceSize(rect.size());
        subresDesc.setDestinationTopLeft(rect.topLeft());
        QRhiTextureUploadDescription uploadDesc(QRhiTextureUploadEntry(0, 0, subresDesc));
        resourceUpdates->uploadTexture(m_texture, uploadDesc);
    }

    return m_texture;
}

static inline QRect scaledRect(const QRect &rect, qreal factor)
{
    return QRect(rect.topLeft() * factor, rect.size() * factor);
}

static inline QPoint scaledOffset(const QPoint &pt, qreal factor)
{
    return pt * factor;
}

static QRegion scaledRegion(const QRegion &region, qreal factor, const QPoint &offset)
{
    if (offset.isNull() && factor <= 1)
        return region;

    QVarLengthArray<QRect, 4> rects;
    rects.reserve(region.rectCount());
    for (const QRect &rect : region)
        rects.append(scaledRect(rect.translated(offset), factor));

    QRegion deviceRegion;
    deviceRegion.setRects(rects.constData(), rects.size());
    return deviceRegion;
}

static QMatrix4x4 targetTransform(const QRectF &target, const QRect &viewport, bool invertY)
{
    qreal x_scale = target.width() / viewport.width();
    qreal y_scale = target.height() / viewport.height();

    const QPointF relative_to_viewport = target.topLeft() - viewport.topLeft();
    qreal x_translate = x_scale - 1 + ((relative_to_viewport.x() / viewport.width()) * 2);
    qreal y_translate;
    if (invertY)
        y_translate = y_scale - 1 + ((relative_to_viewport.y() / viewport.height()) * 2);
    else
        y_translate = -y_scale + 1 - ((relative_to_viewport.y() / viewport.height()) * 2);

    QMatrix4x4 matrix;
    matrix(0,3) = x_translate;
    matrix(1,3) = y_translate;

    matrix(0,0) = x_scale;
    matrix(1,1) = (invertY ? -1.0 : 1.0) * y_scale;

    return matrix;
}

enum class SourceTransformOrigin {
    BottomLeft,
    TopLeft
};

static QMatrix3x3 sourceTransform(const QRectF &subTexture,
                                  const QSize &textureSize,
                                  SourceTransformOrigin origin)
{
    qreal x_scale = subTexture.width() / textureSize.width();
    qreal y_scale = subTexture.height() / textureSize.height();

    const QPointF topLeft = subTexture.topLeft();
    qreal x_translate = topLeft.x() / textureSize.width();
    qreal y_translate = topLeft.y() / textureSize.height();

    if (origin == SourceTransformOrigin::TopLeft) {
        y_scale = -y_scale;
        y_translate = 1 - y_translate;
    }

    QMatrix3x3 matrix;
    matrix(0,2) = x_translate;
    matrix(1,2) = y_translate;

    matrix(0,0) = x_scale;
    matrix(1,1) = y_scale;

    return matrix;
}

static inline QRect toBottomLeftRect(const QRect &topLeftRect, int windowHeight)
{
    return QRect(topLeftRect.x(), windowHeight - topLeftRect.bottomRight().y() - 1,
                 topLeftRect.width(), topLeftRect.height());
}

static bool prepareDrawForRenderToTextureWidget(const QPlatformTextureList *textures,
                                                int idx,
                                                QWindow *window,
                                                const QRect &deviceWindowRect,
                                                const QPoint &offset,
                                                bool invertTargetY,
                                                bool invertSource,
                                                QMatrix4x4 *target,
                                                QMatrix3x3 *source)
{
    const QRect clipRect = textures->clipRect(idx);
    if (clipRect.isEmpty())
        return false;

    QRect rectInWindow = textures->geometry(idx);
    // relative to the TLW, not necessarily our window (if the flush is for a native child widget), have to adjust
    rectInWindow.translate(-offset);

    const QRect clippedRectInWindow = rectInWindow & clipRect.translated(rectInWindow.topLeft());
    const QRect srcRect = toBottomLeftRect(clipRect, rectInWindow.height());

    *target = targetTransform(scaledRect(clippedRectInWindow, window->devicePixelRatio()),
                              deviceWindowRect,
                              invertTargetY);

    *source = sourceTransform(scaledRect(srcRect, window->devicePixelRatio()),
                              scaledRect(rectInWindow, window->devicePixelRatio()).size(),
                              invertSource ? SourceTransformOrigin::TopLeft : SourceTransformOrigin::BottomLeft);

    return true;
}

static QShader getShader(const QString &name)
{
    QFile f(name);
    if (f.open(QIODevice::ReadOnly))
        return QShader::fromSerialized(f.readAll());

    qWarning("QBackingStoreDefaultCompositor: Could not find built-in shader %s "
             "(is something wrong with QtGui library resources?)",
             qPrintable(name));
    return QShader();
}

static void updateMatrix3x3(QRhiResourceUpdateBatch *resourceUpdates, QRhiBuffer *ubuf, const QMatrix3x3 &m)
{
    // mat3 is still 4 floats per column in the uniform buffer (but there is no
    // 4th column), so 48 bytes altogether, not 36 or 64.

    float f[12];
    const float *src = static_cast<const float *>(m.constData());
    float *dst = f;
    memcpy(dst, src, 3 * sizeof(float));
    memcpy(dst + 4, src + 3, 3 * sizeof(float));
    memcpy(dst + 8, src + 6, 3 * sizeof(float));

    resourceUpdates->updateDynamicBuffer(ubuf, 64, 48, f);
}

enum class PipelineBlend {
    None,
    Alpha,
    PremulAlpha
};

static QRhiGraphicsPipeline *createGraphicsPipeline(QRhi *rhi,
                                                    QRhiShaderResourceBindings *srb,
                                                    QRhiSwapChain *swapchain,
                                                    PipelineBlend blend)
{
    QRhiGraphicsPipeline *ps = rhi->newGraphicsPipeline();

    switch (blend) {
    case PipelineBlend::Alpha:
    {
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::One;
        ps->setTargetBlends({ blend });
    }
        break;
    case PipelineBlend::PremulAlpha:
    {
        QRhiGraphicsPipeline::TargetBlend blend;
        blend.enable = true;
        blend.srcColor = QRhiGraphicsPipeline::One;
        blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
        blend.srcAlpha = QRhiGraphicsPipeline::One;
        blend.dstAlpha = QRhiGraphicsPipeline::One;
        ps->setTargetBlends({ blend });
    }
        break;
    default:
        break;
    }

    ps->setShaderStages({
        { QRhiShaderStage::Vertex, getShader(":/qt-project.org/gui/painting/shaders/backingstorecompose.vert.qsb"_L1) },
        { QRhiShaderStage::Fragment, getShader(":/qt-project.org/gui/painting/shaders/backingstorecompose.frag.qsb"_L1) }
    });
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({ { 5 * sizeof(float) } });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },
        { 0, 1, QRhiVertexInputAttribute::Float2, quint32(3 * sizeof(float)) }
    });
    ps->setVertexInputLayout(inputLayout);
    ps->setShaderResourceBindings(srb);
    ps->setRenderPassDescriptor(swapchain->renderPassDescriptor());

    if (!ps->create()) {
        qWarning("QBackingStoreDefaultCompositor: Failed to build graphics pipeline");
        delete ps;
        return nullptr;
    }
    return ps;
}

static const int UBUF_SIZE = 120;

QBackingStoreDefaultCompositor::PerQuadData QBackingStoreDefaultCompositor::createPerQuadData(QRhiTexture *texture, QRhiTexture *textureExtra)
{
    PerQuadData d;

    d.ubuf = m_rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, UBUF_SIZE);
    if (!d.ubuf->create())
        qWarning("QBackingStoreDefaultCompositor: Failed to create uniform buffer");

    d.srb = m_rhi->newShaderResourceBindings();
    d.srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf, 0, UBUF_SIZE),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture, m_samplerNearest)
    });
    if (!d.srb->create())
        qWarning("QBackingStoreDefaultCompositor: Failed to create srb");
    d.lastUsedTexture = texture;

    if (textureExtra) {
        d.srbExtra = m_rhi->newShaderResourceBindings();
        d.srbExtra->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d.ubuf, 0, UBUF_SIZE),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, textureExtra, m_samplerNearest)
        });
        if (!d.srbExtra->create())
            qWarning("QBackingStoreDefaultCompositor: Failed to create srb");
    }

    d.lastUsedTextureExtra = textureExtra;

    return d;
}

void QBackingStoreDefaultCompositor::updatePerQuadData(PerQuadData *d, QRhiTexture *texture, QRhiTexture *textureExtra,
                                                       UpdateQuadDataOptions options)
{
    // This whole check-if-texture-ptr-is-different is needed because there is
    // nothing saying a QPlatformTextureList cannot return a different
    // QRhiTexture* from the same index in a subsequent flush.

    const QRhiSampler::Filter filter = options.testFlag(NeedsLinearFiltering) ? QRhiSampler::Linear : QRhiSampler::Nearest;
    if ((d->lastUsedTexture == texture && d->lastUsedFilter == filter) || !d->srb)
        return;

    QRhiSampler *sampler = filter == QRhiSampler::Linear ? m_samplerLinear : m_samplerNearest;
    d->srb->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d->ubuf, 0, UBUF_SIZE),
        QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, texture, sampler)
    });

    d->srb->updateResources(QRhiShaderResourceBindings::BindingsAreSorted);
    d->lastUsedTexture = texture;
    d->lastUsedFilter = filter;

    if (textureExtra) {
        d->srbExtra->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, d->ubuf, 0, UBUF_SIZE),
            QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage, textureExtra, sampler)
        });

        d->srbExtra->updateResources(QRhiShaderResourceBindings::BindingsAreSorted);
        d->lastUsedTextureExtra = textureExtra;
    }
}

void QBackingStoreDefaultCompositor::updateUniforms(PerQuadData *d, QRhiResourceUpdateBatch *resourceUpdates,
                                                    const QMatrix4x4 &target, const QMatrix3x3 &source,
                                                    UpdateUniformOptions options)
{
    resourceUpdates->updateDynamicBuffer(d->ubuf, 0, 64, target.constData());
    updateMatrix3x3(resourceUpdates, d->ubuf, source);
    float opacity = 1.0f;
    resourceUpdates->updateDynamicBuffer(d->ubuf, 112, 4, &opacity);
    qint32 textureSwizzle = options;
    resourceUpdates->updateDynamicBuffer(d->ubuf, 116, 4, &textureSwizzle);
}

void QBackingStoreDefaultCompositor::ensureResources(QRhiSwapChain *swapchain, QRhiResourceUpdateBatch *resourceUpdates)
{
    static const float vertexData[] = {
        -1, -1, 0,   0, 0,
        -1,  1, 0,   0, 1,
         1, -1, 0,   1, 0,
        -1,  1, 0,   0, 1,
         1, -1, 0,   1, 0,
         1,  1, 0,   1, 1
    };

    if (!m_vbuf) {
        m_vbuf = m_rhi->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer, sizeof(vertexData));
        if (m_vbuf->create())
            resourceUpdates->uploadStaticBuffer(m_vbuf, vertexData);
        else
            qWarning("QBackingStoreDefaultCompositor: Failed to create vertex buffer");
    }

    if (!m_samplerNearest) {
        m_samplerNearest = m_rhi->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest, QRhiSampler::None,
                                             QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
        if (!m_samplerNearest->create())
            qWarning("QBackingStoreDefaultCompositor: Failed to create sampler (Nearest filtering)");
    }

    if (!m_samplerLinear) {
        m_samplerLinear = m_rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                            QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge);
        if (!m_samplerLinear->create())
            qWarning("QBackingStoreDefaultCompositor: Failed to create sampler (Linear filtering)");
    }

    if (!m_widgetQuadData.isValid())
        m_widgetQuadData = createPerQuadData(m_texture);

    QRhiShaderResourceBindings *srb = m_widgetQuadData.srb; // just for the layout
    if (!m_psNoBlend)
        m_psNoBlend = createGraphicsPipeline(m_rhi, srb, swapchain, PipelineBlend::None);
    if (!m_psBlend)
        m_psBlend = createGraphicsPipeline(m_rhi, srb, swapchain, PipelineBlend::Alpha);
    if (!m_psPremulBlend)
        m_psPremulBlend = createGraphicsPipeline(m_rhi, srb, swapchain, PipelineBlend::PremulAlpha);
}

QPlatformBackingStore::FlushResult QBackingStoreDefaultCompositor::flush(QPlatformBackingStore *backingStore,
                                                                         QRhi *rhi,
                                                                         QRhiSwapChain *swapchain,
                                                                         QWindow *window,
                                                                         qreal sourceDevicePixelRatio,
                                                                         const QRegion &region,
                                                                         const QPoint &offset,
                                                                         QPlatformTextureList *textures,
                                                                         bool translucentBackground)
{
    if (!rhi)
        return QPlatformBackingStore::FlushFailed;

    Q_ASSERT(textures); // may be empty if there are no render-to-texture widgets at all, but null it cannot be

    if (!m_rhi) {
        m_rhi = rhi;
    } else if (m_rhi != rhi) {
        qWarning("QBackingStoreDefaultCompositor: the QRhi has changed unexpectedly, this should not happen");
        return QPlatformBackingStore::FlushFailed;
    }

    if (!qt_window_private(window)->receivedExpose)
        return QPlatformBackingStore::FlushSuccess;

    qCDebug(lcQpaBackingStore) << "Composing and flushing" << region << "of" << window
                               << "at offset" << offset << "with" << textures->count() << "texture(s) in" << textures
                               << "via swapchain" << swapchain;

    QWindowPrivate::get(window)->lastComposeTime.start();

    if (swapchain->currentPixelSize() != swapchain->surfacePixelSize())
        swapchain->createOrResize();

    // Start recording a new frame.
    QRhi::FrameOpResult frameResult = rhi->beginFrame(swapchain);
    if (frameResult == QRhi::FrameOpSwapChainOutOfDate) {
        if (!swapchain->createOrResize())
            return QPlatformBackingStore::FlushFailed;
        frameResult = rhi->beginFrame(swapchain);
    }
    if (frameResult == QRhi::FrameOpDeviceLost)
        return QPlatformBackingStore::FlushFailedDueToLostDevice;
    if (frameResult != QRhi::FrameOpSuccess)
        return QPlatformBackingStore::FlushFailed;

    // Prepare resource updates.
    QRhiResourceUpdateBatch *resourceUpdates = rhi->nextResourceUpdateBatch();
    QPlatformBackingStore::TextureFlags flags;

    bool gotTextureFromGraphicsBuffer = false;
    if (QPlatformGraphicsBuffer *graphicsBuffer = backingStore->graphicsBuffer()) {
        if (graphicsBuffer->lock(QPlatformGraphicsBuffer::SWReadAccess)) {
            const QImage::Format format = QImage::toImageFormat(graphicsBuffer->format());
            const QSize size = graphicsBuffer->size();
            QImage wrapperImage(graphicsBuffer->data(), size.width(), size.height(), graphicsBuffer->bytesPerLine(), format);
            toTexture(wrapperImage, rhi, resourceUpdates, scaledRegion(region, sourceDevicePixelRatio, offset), &flags);
            gotTextureFromGraphicsBuffer = true;
            graphicsBuffer->unlock();
            if (graphicsBuffer->origin() == QPlatformGraphicsBuffer::OriginBottomLeft)
                flags |= QPlatformBackingStore::TextureFlip;
        }
    }
    if (!gotTextureFromGraphicsBuffer)
        toTexture(backingStore, rhi, resourceUpdates, scaledRegion(region, sourceDevicePixelRatio, offset), &flags);

    ensureResources(swapchain, resourceUpdates);

    UpdateUniformOptions uniformOptions;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    if (flags & QPlatformBackingStore::TextureSwizzle)
        uniformOptions |= NeedsRedBlueSwap;
#else
    if (flags & QPlatformBackingStore::TextureSwizzle)
        uniformOptions |= NeedsAlphaRotate;
#endif
    const bool premultiplied = (flags & QPlatformBackingStore::TexturePremultiplied) != 0;
    SourceTransformOrigin origin = SourceTransformOrigin::TopLeft;
    if (flags & QPlatformBackingStore::TextureFlip)
        origin = SourceTransformOrigin::BottomLeft;

    const qreal dpr = window->devicePixelRatio();
    const QRect deviceWindowRect = scaledRect(QRect(QPoint(), window->size()), dpr);

    const bool invertTargetY = !rhi->isYUpInNDC();
    const bool invertSource = !rhi->isYUpInFramebuffer();

    if (m_texture) {
        // The backingstore is for the entire tlw. In case of native children, offset tells the position
        // relative to the tlw. The window rect is scaled by the source device pixel ratio to get
        // the source rect.
        const QRect sourceWindowRect = scaledRect(QRect(QPoint(), window->size()), sourceDevicePixelRatio);
        const QPoint sourceWindowOffset = scaledOffset(offset, sourceDevicePixelRatio);
        const QRect srcRect = toBottomLeftRect(sourceWindowRect.translated(sourceWindowOffset), m_texture->pixelSize().height());
        const QMatrix3x3 source = sourceTransform(srcRect, m_texture->pixelSize(), origin);
        QMatrix4x4 target; // identity
        if (invertTargetY)
            target.data()[5] = -1.0f;
        updateUniforms(&m_widgetQuadData, resourceUpdates, target, source, uniformOptions);

        // If sourceWindowRect is larger than deviceWindowRect, we are doing
        // high DPI downscaling. In that case Linear filtering is a must,
        // whereas for the 1:1 case Nearest must be used for Qt 5 visual
        // compatibility.
        if (sourceWindowRect.width() > deviceWindowRect.width()
            && sourceWindowRect.height() > deviceWindowRect.height())
        {
            updatePerQuadData(&m_widgetQuadData, m_texture, nullptr, NeedsLinearFiltering);
        }
    }

    const int textureWidgetCount = textures->count();
    const int oldTextureQuadDataCount = m_textureQuadData.size();
    if (oldTextureQuadDataCount != textureWidgetCount) {
        for (int i = textureWidgetCount; i < oldTextureQuadDataCount; ++i)
            m_textureQuadData[i].reset();
        m_textureQuadData.resize(textureWidgetCount);
    }

    for (int i = 0; i < textureWidgetCount; ++i) {
        QMatrix4x4 target;
        QMatrix3x3 source;
        if (!prepareDrawForRenderToTextureWidget(textures, i, window, deviceWindowRect,
                                                 offset, invertTargetY, invertSource, &target, &source))
        {
            m_textureQuadData[i].reset();
            continue;
        }
        QRhiTexture *t = textures->texture(i);
        QRhiTexture *tExtra = textures->textureExtra(i);
        if (t) {
            if (!m_textureQuadData[i].isValid())
                m_textureQuadData[i] = createPerQuadData(t, tExtra);
            else
                updatePerQuadData(&m_textureQuadData[i], t, tExtra);
            updateUniforms(&m_textureQuadData[i], resourceUpdates, target, source);
        } else {
            m_textureQuadData[i].reset();
        }
    }

    // Record the render pass (with committing the resource updates).
    QRhiCommandBuffer *cb = swapchain->currentFrameCommandBuffer();
    const QSize outputSizeInPixels = swapchain->currentPixelSize();
    QColor clearColor = translucentBackground ? Qt::transparent : Qt::black;

    cb->resourceUpdate(resourceUpdates);

    auto render = [&](std::optional<QRhiSwapChain::StereoTargetBuffer> buffer = std::nullopt) {
        QRhiRenderTarget* target = nullptr;
        if (buffer.has_value())
            target = swapchain->currentFrameRenderTarget(buffer.value());
        else
            target = swapchain->currentFrameRenderTarget();

        cb->beginPass(target, clearColor, { 1.0f, 0 });

        cb->setGraphicsPipeline(m_psNoBlend);
        cb->setViewport({ 0, 0, float(outputSizeInPixels.width()), float(outputSizeInPixels.height()) });
        QRhiCommandBuffer::VertexInput vbufBinding(m_vbuf, 0);
        cb->setVertexInput(0, 1, &vbufBinding);

        // Textures for renderToTexture widgets.
        for (int i = 0; i < textureWidgetCount; ++i) {
            if (!textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop)) {
                if (m_textureQuadData[i].isValid()) {

                    QRhiShaderResourceBindings* srb = m_textureQuadData[i].srb;
                    if (buffer == QRhiSwapChain::RightBuffer && m_textureQuadData[i].srbExtra)
                        srb = m_textureQuadData[i].srbExtra;

                    cb->setShaderResources(srb);
                    cb->draw(6);
                }
            }
        }

        cb->setGraphicsPipeline(premultiplied ? m_psPremulBlend : m_psBlend);

        // Backingstore texture with the normal widgets.
        if (m_texture) {
            cb->setShaderResources(m_widgetQuadData.srb);
            cb->draw(6);
        }

        // Textures for renderToTexture widgets that have WA_AlwaysStackOnTop set.
        for (int i = 0; i < textureWidgetCount; ++i) {
            const QPlatformTextureList::Flags flags = textures->flags(i);
            if (flags.testFlag(QPlatformTextureList::StacksOnTop)) {
                if (m_textureQuadData[i].isValid()) {
                    if (flags.testFlag(QPlatformTextureList::NeedsPremultipliedAlphaBlending))
                        cb->setGraphicsPipeline(m_psPremulBlend);
                    else
                        cb->setGraphicsPipeline(m_psBlend);

                    QRhiShaderResourceBindings* srb = m_textureQuadData[i].srb;
                    if (buffer == QRhiSwapChain::RightBuffer && m_textureQuadData[i].srbExtra)
                        srb = m_textureQuadData[i].srbExtra;

                    cb->setShaderResources(srb);
                    cb->draw(6);
                }
            }
        }

        cb->endPass();
    };

    if (swapchain->window()->format().stereo()) {
        render(QRhiSwapChain::LeftBuffer);
        render(QRhiSwapChain::RightBuffer);
    } else
        render();

    rhi->endFrame(swapchain);

    return QPlatformBackingStore::FlushSuccess;
}

QT_END_NAMESPACE
