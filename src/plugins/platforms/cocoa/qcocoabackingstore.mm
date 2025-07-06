// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qcocoabackingstore.h"

#include "qcocoawindow.h"
#include "qcocoahelpers.h"

#include <QtCore/qmath.h>
#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/qpainter.h>

#include <QuartzCore/CATransaction.h>

QT_BEGIN_NAMESPACE

QCocoaBackingStore::QCocoaBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
}

QCFType<CGColorSpaceRef> QCocoaBackingStore::colorSpace() const
{
    const auto *platformWindow = static_cast<QCocoaWindow *>(window()->handle());
    const QNSView *view = qnsview_cast(platformWindow->view());
    return QCFType<CGColorSpaceRef>::constructFromGet(view.colorSpace.CGColorSpace);
}

// ----------------------------------------------------------------------------

QCALayerBackingStore::QCALayerBackingStore(QWindow *window)
    : QCocoaBackingStore(window)
{
    qCDebug(lcQpaBackingStore) << "Creating QCALayerBackingStore for" << window
        << "with" << window->format();

    m_buffers.resize(1);

    observeBackingPropertiesChanges();
    window->installEventFilter(this);
}

QCALayerBackingStore::~QCALayerBackingStore()
{
}

void QCALayerBackingStore::observeBackingPropertiesChanges()
{
    Q_ASSERT(window()->handle());
    NSView *view = static_cast<QCocoaWindow *>(window()->handle())->view();
    m_backingPropertiesObserver = QMacNotificationObserver(view.window,
        NSWindowDidChangeBackingPropertiesNotification, [this]() {
            backingPropertiesChanged();
        });
}

bool QCALayerBackingStore::eventFilter(QObject *watched, QEvent *event)
{
    Q_ASSERT(watched == window());

    if (event->type() == QEvent::PlatformSurface) {
        auto *surfaceEvent = static_cast<QPlatformSurfaceEvent*>(event);
        if (surfaceEvent->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated)
            observeBackingPropertiesChanges();
        else
            m_backingPropertiesObserver = QMacNotificationObserver();
    }

    return false;
}

void QCALayerBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    qCDebug(lcQpaBackingStore) << "Resize requested to" << size
                               << "with static contents" << staticContents;

    m_requestedSize = size;
    m_staticContents = staticContents;
}

void QCALayerBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);

    QMacAutoReleasePool pool;

    qCInfo(lcQpaBackingStore) << "Beginning paint of" << region << "into backingstore of" << m_requestedSize;

    if (m_requestedSize.isEmpty()) {
        // We can't create IOSurfaces with and empty size, so instead reset our back buffer
        qCDebug(lcQpaBackingStore) << "Size is empty, throwing away back buffer";
        m_buffers.back().reset(nullptr);
        return;
    }

    ensureBackBuffer(); // Find an unused back buffer, or reserve space for a new one

    const bool bufferWasRecreated = recreateBackBufferIfNeeded();

    m_buffers.back()->lock(QPlatformGraphicsBuffer::SWWriteAccess);

    // Although undocumented, QBackingStore::beginPaint expects the painted region
    // to be cleared before use if the window has a surface format with an alpha.
    // Fresh IOSurfaces are already cleared, so we don't need to clear those.
    if (m_clearSurfaceOnPaint && !bufferWasRecreated && window()->format().hasAlpha()) {
        qCDebug(lcQpaBackingStore) << "Clearing" << region << "before use";
        QPainter painter(m_buffers.back()->asImage());
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        for (const QRect &rect : region)
            painter.fillRect(rect, Qt::transparent);
    }

    // We assume the client is going to paint the entire region
    updateDirtyStates(region);
}

void QCALayerBackingStore::ensureBackBuffer()
{
    if (window()->format().swapBehavior() == QSurfaceFormat::SingleBuffer)
        return;

    if (Q_UNLIKELY(lcQpaBackingStore().isDebugEnabled())) {
        // ┌───────┬───────┬───────┬─────┬──────┐
        // │ front ┊ spare ┊ spare ┊ ... ┊ back │
        // └───────┴───────┴───────┴─────┴──────┘
        for (const auto &buffer : m_buffers) {
            qCDebug(lcQpaBackingStore).nospace() << "  "
                << (buffer == m_buffers.front() ? "front" :
                    buffer == m_buffers.back()  ? " back" :
                                                  "spare"
                ) << ": " << buffer.get();
        }
    }

    // Ensure our back buffer is ready to draw into. If not, find a buffer that
    // is not in use, or reserve space for a new buffer if none can be found.
    for (auto &buffer : backwards(m_buffers)) {
        if (!buffer || !buffer->isInUse()) {
            // Buffer is okey to use, swap if necessary
            if (buffer != m_buffers.back())
                std::swap(buffer, m_buffers.back());
            qCDebug(lcQpaBackingStore) << "Using back buffer" << m_buffers.back().get();

            static const int kMaxSwapChainDepth = 3;
            if (m_buffers.size() > kMaxSwapChainDepth) {
                qCDebug(lcQpaBackingStore) << "Reducing swap chain depth to" << kMaxSwapChainDepth;
                m_buffers.erase(std::next(m_buffers.begin(), 1), std::prev(m_buffers.end(), 2));
            }

            break;
        } else if (buffer == m_buffers.front()) {
            // We've exhausted the available buffers, make room for a new one
            const int swapChainDepth = m_buffers.size() + 1;
            qCDebug(lcQpaBackingStore) << "Available buffers exhausted, increasing swap chain depth to" << swapChainDepth;
            m_buffers.resize(swapChainDepth);
            break;
        }
    }

    Q_ASSERT(!m_buffers.back() || !m_buffers.back()->isInUse());
}

// Disabled until performance issue on 5K iMac Pro has been investigated further,
// as rounding up during resize will typically result in full screen buffer sizes
// and low frame rate also for smaller window sizes.
#define USE_LAZY_BUFFER_ALLOCATION_DURING_LIVE_WINDOW_RESIZE 0

bool QCALayerBackingStore::recreateBackBufferIfNeeded()
{
    const QCocoaWindow *platformWindow = static_cast<QCocoaWindow *>(window()->handle());
    const qreal devicePixelRatio = platformWindow->devicePixelRatio();
    QSize requestedBufferSize = m_requestedSize * devicePixelRatio;

    const NSView *backingStoreView = platformWindow->view();
    Q_UNUSED(backingStoreView);

    auto bufferSizeMismatch = [&](const QSize requested, const QSize actual) {
#if USE_LAZY_BUFFER_ALLOCATION_DURING_LIVE_WINDOW_RESIZE
        if (backingStoreView.inLiveResize) {
            // Prevent over-eager buffer allocation during window resize by reusing larger buffers
            return requested.width() > actual.width() || requested.height() > actual.height();
        }
#endif
        return requested != actual;
    };

    if (!m_buffers.back() || bufferSizeMismatch(requestedBufferSize, m_buffers.back()->size())) {
#if USE_LAZY_BUFFER_ALLOCATION_DURING_LIVE_WINDOW_RESIZE
        if (backingStoreView.inLiveResize) {
            // Prevent over-eager buffer allocation during window resize by rounding up
            QSize nativeScreenSize = window()->screen()->geometry().size() * devicePixelRatio;
            requestedBufferSize = QSize(qNextPowerOfTwo(requestedBufferSize.width()),
                qNextPowerOfTwo(requestedBufferSize.height())).boundedTo(nativeScreenSize);
        }
#endif

        qCInfo(lcQpaBackingStore)<< "Creating surface of" << requestedBufferSize
            << "for" << window() << "based on requested" << m_requestedSize
            << "dpr =" << devicePixelRatio << "and color space" << colorSpace();

        static auto pixelFormat = QImage::toPixelFormat(QImage::Format_ARGB32_Premultiplied);
        auto *newBackBuffer = new GraphicsBuffer(requestedBufferSize, devicePixelRatio, pixelFormat, colorSpace());

        if (!m_staticContents.isEmpty() && m_buffers.back()) {
            // We implicitly support static backingstore content as a result of
            // finalizing the back buffer on flush, where we copy any non-painted
            // areas from the front buffer. But there is no guarantee that a resize
            // will always come after a flush, where we have a pristine front buffer
            // to copy from. It may come after a few begin/endPaints, where the back
            // buffer then contains (part of) the latest state. We also have the case
            // of single-buffered backingstore, where the front and back buffer is
            // the same, which means we must do the copy from the old back buffer
            // to the newly resized buffer now, before we replace it below.

            // If the back buffer has been partially filled already, we need to
            // copy parts of the static content from that. The rest we copy from
            // the front buffer.
            const QRegion backBufferRegion = m_staticContents - m_buffers.back()->dirtyRegion;
            const QRegion frontBufferRegion = m_staticContents - backBufferRegion;

            qCInfo(lcQpaBackingStore) << "Preserving static content" << backBufferRegion
                << "from back buffer, and" << frontBufferRegion << "from front buffer";

            newBackBuffer->lock(QPlatformGraphicsBuffer::SWWriteAccess);
            blitBuffer(m_buffers.back().get(), backBufferRegion, newBackBuffer);
            Q_ASSERT(frontBufferRegion.isEmpty() || m_buffers.front());
            blitBuffer(m_buffers.front().get(), frontBufferRegion, newBackBuffer);
            newBackBuffer->unlock();

            // The new back buffer now is valid for the static contents region.
            // We don't need to maintain the static contents region for resizes
            // of any other buffers in the swap chain, as these will finalize
            // their content on flush from the buffer we just filled, and we
            // don't need to mark them dirty for the area we just filled, as
            // new buffers are fully dirty when created.
            newBackBuffer->dirtyRegion -= m_staticContents;
            m_staticContents = {};
        }

        m_buffers.back().reset(newBackBuffer);
        return true;
    }

    return false;
}

QPaintDevice *QCALayerBackingStore::paintDevice()
{
    if (m_buffers.back()) {
        return m_buffers.back()->asImage();
    } else {
        static QImage fallbackDevice;
        return &fallbackDevice;
    }
}

void QCALayerBackingStore::endPaint()
{
    if (!m_buffers.back())
        return;

    qCInfo(lcQpaBackingStore) << "Paint ended. Back buffer valid region is now" << m_buffers.back()->validRegion();
    m_buffers.back()->unlock();

    // Since we can have multiple begin/endPaint rounds before a flush
    // we defer finalizing the back buffer until its content is needed.
}

bool QCALayerBackingStore::scroll(const QRegion &region, int dx, int dy)
{
    if (!m_buffers.back()) {
        qCInfo(lcQpaBackingStore) << "Scroll requested with no back buffer. Ignoring.";
        return false;
    }

    const QPoint scrollDelta(dx, dy);
    qCInfo(lcQpaBackingStore) << "Scrolling" << region << "by" << scrollDelta;

    ensureBackBuffer();
    recreateBackBufferIfNeeded();

    const QRegion inPlaceRegion = region - m_buffers.back()->dirtyRegion;
    const QRegion frontBufferRegion = region - inPlaceRegion;

    QMacAutoReleasePool pool;

    m_buffers.back()->lock(QPlatformGraphicsBuffer::SWWriteAccess);

    if (!inPlaceRegion.isEmpty()) {
        // We have to scroll everything in one go, instead of scrolling the
        // individual rects of the region, as otherwise we may end up reading
        // already overwritten (scrolled) pixels.
        const QRect inPlaceBoundingRect = inPlaceRegion.boundingRect();

        qCDebug(lcQpaBackingStore) << "Scrolling" << inPlaceBoundingRect << "in place";
        QImage *backBufferImage = m_buffers.back()->asImage();
        const qreal devicePixelRatio = backBufferImage->devicePixelRatio();
        const QPoint devicePixelDelta = scrollDelta * devicePixelRatio;

        extern void qt_scrollRectInImage(QImage &, const QRect &, const QPoint &);

        qt_scrollRectInImage(*backBufferImage,
            QRect(inPlaceBoundingRect.topLeft() * devicePixelRatio,
                  inPlaceBoundingRect.size() * devicePixelRatio),
                  devicePixelDelta);
    }

    if (!frontBufferRegion.isEmpty()) {
        qCDebug(lcQpaBackingStore) << "Scrolling" << frontBufferRegion << "by copying from front buffer";
        blitBuffer(m_buffers.front().get(), frontBufferRegion, m_buffers.back().get(), scrollDelta);
    }

    m_buffers.back()->unlock();

    // Mark the target region as filled. Note: We do not mark the source region
    // as dirty, even though the content has conceptually been "moved", as that
    // would complicate things when preserving from the front buffer. This matches
    // the behavior of other backingstore implementations using qt_scrollRectInImage.
    updateDirtyStates(region.translated(scrollDelta));

    qCInfo(lcQpaBackingStore) << "Scroll ended. Back buffer valid region is now" << m_buffers.back()->validRegion();

    return true;
}

void QCALayerBackingStore::flush(QWindow *flushedWindow, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    if (!m_buffers.back()) {
        qCWarning(lcQpaBackingStore) << "Flush requested with no back buffer. Ignoring.";
        return;
    }

    finalizeBackBuffer();

    if (flushedWindow != window()) {
        flushSubWindow(flushedWindow);
        return;
    }

    if (m_buffers.front()->isInUse() && !m_buffers.front()->isDirty()) {
        qCInfo(lcQpaBackingStore) << "Asked to flush, but front buffer is up to date. Ignoring.";
        return;
    }

    QMacAutoReleasePool pool;

    NSView *flushedView = static_cast<QCocoaWindow *>(flushedWindow->handle())->view();
    CALayer *layer = static_cast<QCocoaWindow *>(flushedWindow->handle())->contentLayer();

    // If the backingstore is just flushed, without being painted to first, then we may
    // end in a situation where the backingstore is flushed to a layer with a different
    // scale factor than the one it was created for in beginPaint. This is the client's
    // fault in not picking up the change in scale factor of the window and re-painting
    // the backingstore accordingly. To smoothing things out, we warn about this situation,
    // and change the layer's contentsScale to match the scale of the back buffer, so that
    // we at least cover the whole layer. This is necessary since we set the view's
    // contents placement policy to NSViewLayerContentsPlacementTopLeft, which means
    // AppKit will not do any scaling on our behalf.
    if (m_buffers.back()->devicePixelRatio() != layer.contentsScale) {
        qCWarning(lcQpaBackingStore) << "Back buffer dpr of" << m_buffers.back()->devicePixelRatio()
            << "doesn't match" << layer << "contents scale of" << layer.contentsScale
            << "- updating layer to match.";
        layer.contentsScale = m_buffers.back()->devicePixelRatio();
    }

    const bool isSingleBuffered = window()->format().swapBehavior() == QSurfaceFormat::SingleBuffer;

    id backBufferSurface = (__bridge id)m_buffers.back()->surface();

    // Trigger a new display cycle if there isn't one. This ensures that our layer updates
    // are committed as part of a display-cycle instead of on the next runloop pass. This
    // means CA won't try to throttle us if we flush too fast, and we'll coalesce our flush
    // with other pending view and layer updates.
    flushedView.window.viewsNeedDisplay = YES;

    if (isSingleBuffered) {
        // The private API [CALayer reloadValueForKeyPath:@"contents"] would be preferable,
        // but barring any side effects or performance issues we opt for the hammer for now.
        layer.contents = nil;
    }

    qCInfo(lcQpaBackingStore) << "Flushing" << backBufferSurface
        << "to" << layer << "of" << flushedView;

    layer.contents = backBufferSurface;

    if (!isSingleBuffered) {
        // Mark the surface as in use, so that we don't end up rendering
        // to it while it's assigned to a layer.
        IOSurfaceIncrementUseCount(m_buffers.back()->surface());

        if (m_buffers.back() != m_buffers.front()) {
            qCInfo(lcQpaBackingStore) << "Swapping back buffer to front";
            std::swap(m_buffers.back(), m_buffers.front());
            IOSurfaceDecrementUseCount(m_buffers.back()->surface());
        }
    }
}

void QCALayerBackingStore::flushSubWindow(QWindow *subWindow)
{
    qCInfo(lcQpaBackingStore) << "Flushing sub-window" << subWindow
        << "via its own backingstore";

    auto &subWindowBackingStore = m_subWindowBackingstores[subWindow];
    if (!subWindowBackingStore) {
        subWindowBackingStore.reset(new QCALayerBackingStore(subWindow));
        QObject::connect(subWindow, &QObject::destroyed, this, &QCALayerBackingStore::windowDestroyed);
        subWindowBackingStore->m_clearSurfaceOnPaint = false;
    }

    // Query platform window for subwindow size, so that we
    // incorporate any effects of the QHighDpiScaling layer.
    auto subWindowSize = subWindow->handle()->geometry().size();
    static const auto kNoStaticContents = QRegion();
    subWindowBackingStore->resize(subWindowSize, kNoStaticContents);

    auto subWindowLocalRect = QRect(QPoint(), subWindowSize);
    subWindowBackingStore->beginPaint(subWindowLocalRect);

    QPainter painter(subWindowBackingStore->m_buffers.back()->asImage());
    painter.setCompositionMode(QPainter::CompositionMode_Source);

    NSView *backingStoreView = static_cast<QCocoaWindow *>(window()->handle())->view();
    NSView *flushedView = static_cast<QCocoaWindow *>(subWindow->handle())->view();
    auto subviewRect = [flushedView convertRect:flushedView.bounds toView:backingStoreView];
    CALayer *layer = static_cast<QCocoaWindow *>(subWindow->handle())->contentLayer();
    auto scale = layer.contentsScale;
    subviewRect = CGRectApplyAffineTransform(subviewRect, CGAffineTransformMakeScale(scale, scale));

    m_buffers.back()->lock(QPlatformGraphicsBuffer::SWReadAccess);
    const QImage *backingStoreImage = m_buffers.back()->asImage();
    painter.drawImage(subWindowLocalRect, *backingStoreImage, QRectF::fromCGRect(subviewRect));
    m_buffers.back()->unlock();

    painter.end();
    subWindowBackingStore->endPaint();
    subWindowBackingStore->flush(subWindow, subWindowLocalRect, QPoint());

    qCInfo(lcQpaBackingStore) << "Done flushing sub-window" << subWindow;
}

void QCALayerBackingStore::windowDestroyed(QObject *object)
{
    auto *window = static_cast<QWindow*>(object);
    qCInfo(lcQpaBackingStore) << "Removing backingstore for sub-window" << window;
    m_subWindowBackingstores.erase(window);
}

QPlatformBackingStore::FlushResult QCALayerBackingStore::rhiFlush(QWindow *window,
                                                                  qreal sourceDevicePixelRatio,
                                                                  const QRegion &region,
                                                                  const QPoint &offset,
                                                                  QPlatformTextureList *textures,
                                                                  bool translucentBackground,
                                                                  qreal sourceTransformFactor)
{
    if (!m_buffers.back()) {
        qCWarning(lcQpaBackingStore) << "Flush requested with no back buffer. Ignoring.";
        return FlushFailed;
    }

    finalizeBackBuffer();

    return QPlatformBackingStore::rhiFlush(window, sourceDevicePixelRatio,
        region, offset, textures, translucentBackground, sourceTransformFactor);
}

QImage QCALayerBackingStore::toImage() const
{
    if (!m_buffers.back())
        return QImage();

    const_cast<QCALayerBackingStore*>(this)->finalizeBackBuffer();

    // We need to make a copy here, as the returned image could be used just
    // for reading, in which case it won't detach, and then the underlying
    // image data might change under the feet of the client when we re-use
    // the buffer at a later point.
    m_buffers.back()->lock(QPlatformGraphicsBuffer::SWReadAccess);
    QImage imageCopy = m_buffers.back()->asImage()->copy();
    m_buffers.back()->unlock();
    return imageCopy;
}

void QCALayerBackingStore::backingPropertiesChanged()
{
    // Ideally this would be plumbed from the platform layer to QtGui, and
    // the QBackingStore would be recreated, but we don't have that code yet,
    // so at least make sure we update our backingstore when the backing
    // properties (color space e.g.) are changed.

    Q_ASSERT(window()->handle());

    qCDebug(lcQpaBackingStore) << "Backing properties for" << window() << "did change";

    const auto newColorSpace = colorSpace();
    qCDebug(lcQpaBackingStore) << "Updating color space of existing buffers to" << newColorSpace;
    for (auto &buffer : m_buffers) {
        if (buffer)
            buffer->setColorSpace(newColorSpace);
    }
}

QPlatformGraphicsBuffer *QCALayerBackingStore::graphicsBuffer() const
{
    return m_buffers.back().get();
}

void QCALayerBackingStore::updateDirtyStates(const QRegion &paintedRegion)
{
    // Update dirty state of buffers based on what was painted. The back buffer will be
    // less dirty, since we painted to it, while other buffers will become more dirty.
    // This allows us to minimize copies between front and back buffers on swap in the
    // cases where the painted region overlaps with the previous frame (front buffer).
    for (const auto &buffer : m_buffers) {
        if (buffer == m_buffers.back())
            buffer->dirtyRegion -= paintedRegion;
        else
            buffer->dirtyRegion += paintedRegion;
    }
}

void QCALayerBackingStore::finalizeBackBuffer()
{
    // After painting, the back buffer is only guaranteed to have content for the painted
    // region, and may still have dirty areas that need to be synced up with the front buffer,
    // if we have one. We know that the front buffer is always up to date.

    if (!m_buffers.back()->isDirty())
        return;

    qCDebug(lcQpaBackingStore) << "Finalizing back buffer with dirty region" << m_buffers.back()->dirtyRegion;

    if (m_buffers.back() != m_buffers.front()) {
        m_buffers.back()->lock(QPlatformGraphicsBuffer::SWWriteAccess);
        blitBuffer(m_buffers.front().get(), m_buffers.back()->dirtyRegion, m_buffers.back().get());
        m_buffers.back()->unlock();
    } else {
        qCDebug(lcQpaBackingStore) << "Front and back buffer is the same. Can not finalize back buffer.";
    }

    // The back buffer is now completely in sync, ready to be presented
    m_buffers.back()->dirtyRegion = QRegion();
}

/*
    \internal

    Blits \a sourceRegion from \a sourceBuffer to \a destinationBuffer,
    at offset \a destinationOffset.

    The source buffer is automatically locked for read only access
    during the blit.

    The destination buffer has to be locked for write access by the
    caller.
*/

void QCALayerBackingStore::blitBuffer(GraphicsBuffer *sourceBuffer, const QRegion &sourceRegion,
                                      GraphicsBuffer *destinationBuffer, const QPoint &destinationOffset)
{
    Q_ASSERT(sourceBuffer && destinationBuffer);
    Q_ASSERT(sourceBuffer != destinationBuffer);

    if (sourceRegion.isEmpty())
        return;

    qCDebug(lcQpaBackingStore) << "Blitting" << sourceRegion << "of" << sourceBuffer
        << "to" << sourceRegion.translated(destinationOffset) << "of" << destinationBuffer;

    Q_ASSERT(destinationBuffer->isLocked() == QPlatformGraphicsBuffer::SWWriteAccess);

    sourceBuffer->lock(QPlatformGraphicsBuffer::SWReadAccess);
    const QImage *sourceImage = sourceBuffer->asImage();

    const QRect sourceBufferBounds(QPoint(0, 0), sourceBuffer->size());
    const qreal sourceDevicePixelRatio = sourceImage->devicePixelRatio();

    QPainter painter(destinationBuffer->asImage());
    painter.setCompositionMode(QPainter::CompositionMode_Source);

    // Let painter operate in device pixels, to make it easier to compare coordinates
    const qreal destinationDevicePixelRatio = painter.device()->devicePixelRatio();
    painter.scale(1.0 / destinationDevicePixelRatio, 1.0 / destinationDevicePixelRatio);

    for (const QRect &rect : sourceRegion) {
        QRect sourceRect(rect.topLeft() * sourceDevicePixelRatio,
                         rect.size() * sourceDevicePixelRatio);
        QRect destinationRect((rect.topLeft() + destinationOffset) * destinationDevicePixelRatio,
                               rect.size() * destinationDevicePixelRatio);

#ifdef QT_DEBUG
        if (Q_UNLIKELY(!sourceBufferBounds.contains(sourceRect.bottomRight()))) {
            qCWarning(lcQpaBackingStore) << "Source buffer of size" << sourceBuffer->size()
                                         << "is too small to blit" << sourceRect;
        }
#endif
        painter.drawImage(destinationRect, *sourceImage, sourceRect);
    }

    sourceBuffer->unlock();
}

// ----------------------------------------------------------------------------

QCALayerBackingStore::GraphicsBuffer::GraphicsBuffer(const QSize &size, qreal devicePixelRatio,
                                const QPixelFormat &format, QCFType<CGColorSpaceRef> colorSpace)
    : QIOSurfaceGraphicsBuffer(size, format)
    , dirtyRegion(QRect(QPoint(0, 0), size / devicePixelRatio))
    , m_devicePixelRatio(devicePixelRatio)
{
    setColorSpace(colorSpace);
}

QRegion QCALayerBackingStore::GraphicsBuffer::validRegion() const
{

    QRegion fullRegion = QRect(QPoint(0, 0), size() / m_devicePixelRatio);
    return fullRegion - dirtyRegion;
}

QImage *QCALayerBackingStore::GraphicsBuffer::asImage()
{
    if (m_image.isNull()) {
        qCDebug(lcQpaBackingStore) << "Setting up paint device for" << this;
        CFRetain(surface());
        m_image = QImage(data(), size().width(), size().height(),
            bytesPerLine(), QImage::toImageFormat(format()),
            QImageCleanupFunction(CFRelease), surface());
        m_image.setDevicePixelRatio(m_devicePixelRatio);
    }

    Q_ASSERT_X(m_image.constBits() == data(), "QCALayerBackingStore",
        "IOSurfaces should have have a fixed location in memory once created");

    return &m_image;
}

QT_END_NAMESPACE

#include "moc_qcocoabackingstore.cpp"
