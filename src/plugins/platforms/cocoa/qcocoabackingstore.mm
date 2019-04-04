/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qcocoabackingstore.h"

#include "qcocoawindow.h"
#include "qcocoahelpers.h"

#include <QtCore/qmath.h>

#include <QuartzCore/CATransaction.h>

QT_BEGIN_NAMESPACE

QNSWindowBackingStore::QNSWindowBackingStore(QWindow *window)
    : QRasterBackingStore(window)
{
}

QNSWindowBackingStore::~QNSWindowBackingStore()
{
}

bool QNSWindowBackingStore::windowHasUnifiedToolbar() const
{
    Q_ASSERT(window()->handle());
    return static_cast<QCocoaWindow *>(window()->handle())->m_drawContentBorderGradient;
}

QImage::Format QNSWindowBackingStore::format() const
{
    if (windowHasUnifiedToolbar())
        return QImage::Format_ARGB32_Premultiplied;

    return QRasterBackingStore::format();
}

/*!
    Flushes the given \a region from the specified \a window onto the
    screen.

    The \a window is the top level window represented by this backingstore,
    or a non-transient child of that window.

    If the \a window is a child window, the \a region will be in child window
    coordinates, and the \a offset will be the child window's offset in relation
    to the backingstore's top level window.
*/
void QNSWindowBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    if (m_image.isNull())
        return;

    // Use local pool so that any stale image references are cleaned up after flushing
    QMacAutoReleasePool pool;

    const QWindow *topLevelWindow = this->window();

    Q_ASSERT(topLevelWindow->handle() && window->handle());
    Q_ASSERT(!topLevelWindow->handle()->isForeignWindow() && !window->handle()->isForeignWindow());

    QNSView *topLevelView = qnsview_cast(static_cast<QCocoaWindow *>(topLevelWindow->handle())->view());
    QNSView *view = qnsview_cast(static_cast<QCocoaWindow *>(window->handle())->view());

    if (lcQpaBackingStore().isDebugEnabled()) {
        QString targetViewDescription;
        if (view != topLevelView) {
            QDebug targetDebug(&targetViewDescription);
            targetDebug << "onto" << topLevelView << "at" << offset;
        }
        qCDebug(lcQpaBackingStore) << "Flushing" << region << "of" << view << qPrintable(targetViewDescription);
    }

    // Normally a NSView is drawn via drawRect, as part of the display cycle in the
    // main runloop, via setNeedsDisplay and friends. AppKit will lock focus on each
    // individual view, starting with the top level and then traversing any subviews,
    // calling drawRect for each of them. This pull model results in expose events
    // sent to Qt, which result in drawing to the backingstore and flushing it.
    // Qt may also decide to paint and flush the backingstore via e.g. timers,
    // or other events such as mouse events, in which case we're in a push model.
    // If there is no focused view, it means we're in the latter case, and need
    // to manually flush the NSWindow after drawing to its graphic context.
    const bool drawingOutsideOfDisplayCycle = ![NSView focusView];

    // We also need to ensure the flushed view has focus, so that the graphics
    // context is set up correctly (coordinate system, clipping, etc). Outside
    // of the normal display cycle there is no focused view, as explained above,
    // so we have to handle it manually. There's also a corner case inside the
    // normal display cycle due to way QWidgetBackingStore composits native child
    // widgets, where we'll get a flush of a native child during the drawRect of
    // its parent/ancestor, and the parent/ancestor being the one locked by AppKit.
    // In this case we also need to lock and unlock focus manually.
    const bool shouldHandleViewLockManually = [NSView focusView] != view;
    if (shouldHandleViewLockManually && ![view lockFocusIfCanDraw]) {
        qWarning() << "failed to lock focus of" << view;
        return;
    }

    const qreal devicePixelRatio = m_image.devicePixelRatio();

    // If the flushed window is a content view, and we're filling the drawn area
    // completely, or it doesn't have a window background we need to preserve,
    // we can get away with copying instead of blending the backing store.
    QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow *>(window->handle());
    const NSCompositingOperation compositingOperation = cocoaWindow->isContentView()
        && (cocoaWindow->isOpaque() || view.window.backgroundColor == NSColor.clearColor)
            ? NSCompositingOperationCopy : NSCompositingOperationSourceOver;

#ifdef QT_DEBUG
    static bool debugBackingStoreFlush = [[NSUserDefaults standardUserDefaults]
        boolForKey:@"QtCocoaDebugBackingStoreFlush"];
#endif

    // -------------------------------------------------------------------------

    // The current contexts is typically a NSWindowGraphicsContext, but can be
    // NSBitmapGraphicsContext e.g. when debugging the view hierarchy in Xcode.
    // If we need to distinguish things here in the future, we can use e.g.
    // [NSGraphicsContext drawingToScreen], or the attributes of the context.
    NSGraphicsContext *graphicsContext = [NSGraphicsContext currentContext];
    Q_ASSERT_X(graphicsContext, "QCocoaBackingStore",
        "Focusing the view should give us a current graphics context");

    // Prevent potentially costly color conversion by assigning the display color space
    // to the backingstore image. This does not copy the underlying image data.
    CGColorSpaceRef displayColorSpace = view.window.screen.colorSpace.CGColorSpace;
    QCFType<CGImageRef> cgImage = CGImageCreateCopyWithColorSpace(
        QCFType<CGImageRef>(m_image.toCGImage()), displayColorSpace);

    // Create temporary image to use for blitting, without copying image data
    NSImage *backingStoreImage = [[[NSImage alloc] initWithCGImage:cgImage size:NSZeroSize] autorelease];

    QRegion clippedRegion = region;
    for (QWindow *w = window; w; w = w->parent()) {
        if (!w->mask().isEmpty()) {
            clippedRegion &= w == window ? w->mask()
                : w->mask().translated(window->mapFromGlobal(w->mapToGlobal(QPoint(0, 0))));
        }
    }

    for (const QRect &viewLocalRect : clippedRegion) {
        QPoint backingStoreOffset = viewLocalRect.topLeft() + offset;
        QRect backingStoreRect(backingStoreOffset * devicePixelRatio, viewLocalRect.size() * devicePixelRatio);
        if (graphicsContext.flipped) // Flip backingStoreRect to match graphics context
            backingStoreRect.moveTop(m_image.height() - (backingStoreRect.y() + backingStoreRect.height()));

        CGRect viewRect = viewLocalRect.toCGRect();

        if (windowHasUnifiedToolbar())
            NSDrawWindowBackground(viewRect);

        [backingStoreImage drawInRect:viewRect fromRect:backingStoreRect.toCGRect()
            operation:compositingOperation fraction:1.0 respectFlipped:YES hints:nil];

#ifdef QT_DEBUG
        if (Q_UNLIKELY(debugBackingStoreFlush)) {
            [[NSColor colorWithCalibratedRed:drand48() green:drand48() blue:drand48() alpha:0.3] set];
            [NSBezierPath fillRect:viewRect];

            if (drawingOutsideOfDisplayCycle) {
                [[[NSColor magentaColor] colorWithAlphaComponent:0.5] set];
                [NSBezierPath strokeLineFromPoint:viewLocalRect.topLeft().toCGPoint()
                    toPoint:viewLocalRect.bottomRight().toCGPoint()];
            }
        }
#endif
    }

    // -------------------------------------------------------------------------

    if (shouldHandleViewLockManually)
        [view unlockFocus];

    if (drawingOutsideOfDisplayCycle) {
        redrawRoundedBottomCorners([view convertRect:region.boundingRect().toCGRect() toView:nil]);
        [view.window flushWindow];
    }


    // Done flushing to NSWindow backingstore

    QCocoaWindow *topLevelCocoaWindow = static_cast<QCocoaWindow *>(topLevelWindow->handle());
    if (Q_UNLIKELY(topLevelCocoaWindow->m_needsInvalidateShadow)) {
        [topLevelView.window invalidateShadow];
        topLevelCocoaWindow->m_needsInvalidateShadow = false;
    }
}

/*
    When drawing outside of the display cycle, which Qt Widget does a lot,
    we end up drawing over the NSThemeFrame, losing the rounded corners of
    windows in the process.

    To work around this, until we've enabled updates via setNeedsDisplay and/or
    enabled layer-backed views, we ask the NSWindow to redraw the bottom corners
    if they intersect with the flushed region.

    This is the same logic used internally by e.g [NSView displayIfNeeded],
    [NSRulerView _scrollToMatchContentView], and [NSClipView _immediateScrollToPoint:],
    as well as the workaround used by WebKit to fix a similar bug:

    https://trac.webkit.org/changeset/85376/webkit
*/
void QNSWindowBackingStore::redrawRoundedBottomCorners(CGRect windowRect) const
{
#if !defined(QT_APPLE_NO_PRIVATE_APIS)
    Q_ASSERT(this->window()->handle());
    NSWindow *window = static_cast<QCocoaWindow *>(this->window()->handle())->nativeWindow();

    static SEL intersectBottomCornersWithRect = NSSelectorFromString(
        [NSString stringWithFormat:@"_%s%s:", "intersectBottomCorners", "WithRect"]);
    if (NSMethodSignature *signature = [window methodSignatureForSelector:intersectBottomCornersWithRect]) {
        NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
        invocation.target = window;
        invocation.selector = intersectBottomCornersWithRect;
        [invocation setArgument:&windowRect atIndex:2];
        [invocation invoke];

        NSRect cornerOverlap = NSZeroRect;
        [invocation getReturnValue:&cornerOverlap];
        if (!NSIsEmptyRect(cornerOverlap)) {
            static SEL maskRoundedBottomCorners = NSSelectorFromString(
                [NSString stringWithFormat:@"_%s%s:", "maskRounded", "BottomCorners"]);
            if ((signature = [window methodSignatureForSelector:maskRoundedBottomCorners])) {
                invocation = [NSInvocation invocationWithMethodSignature:signature];
                invocation.target = window;
                invocation.selector = maskRoundedBottomCorners;
                [invocation setArgument:&cornerOverlap atIndex:2];
                [invocation invoke];
            }
        }
    }
#else
    Q_UNUSED(windowRect);
#endif
}

// ----------------------------------------------------------------------------

// https://stackoverflow.com/a/52722575/2761869
template<class R>
struct backwards_t {
  R r;
  constexpr auto begin() const { using std::rbegin; return rbegin(r); }
  constexpr auto begin() { using std::rbegin; return rbegin(r); }
  constexpr auto end() const { using std::rend; return rend(r); }
  constexpr auto end() { using std::rend; return rend(r); }
};
template<class R>
constexpr backwards_t<R> backwards(R&& r) { return {std::forward<R>(r)}; }

QCALayerBackingStore::QCALayerBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
{
    qCDebug(lcQpaBackingStore) << "Creating QCALayerBackingStore for" << window;
    m_buffers.resize(1);
}

QCALayerBackingStore::~QCALayerBackingStore()
{
}

void QCALayerBackingStore::resize(const QSize &size, const QRegion &staticContents)
{
    qCDebug(lcQpaBackingStore) << "Resize requested to" << size;

    if (!staticContents.isNull())
        qCWarning(lcQpaBackingStore) << "QCALayerBackingStore does not support static contents";

    m_requestedSize = size;
}

void QCALayerBackingStore::beginPaint(const QRegion &region)
{
    Q_UNUSED(region);

    QMacAutoReleasePool pool;

    qCInfo(lcQpaBackingStore) << "Beginning paint of" << region << "into backingstore of" << m_requestedSize;

    ensureBackBuffer(); // Find an unused back buffer, or reserve space for a new one

    const bool bufferWasRecreated = recreateBackBufferIfNeeded();

    m_buffers.back()->lock(QPlatformGraphicsBuffer::SWWriteAccess);

    // Although undocumented, QBackingStore::beginPaint expects the painted region
    // to be cleared before use if the window has a surface format with an alpha.
    // Fresh IOSurfaces are already cleared, so we don't need to clear those.
    if (!bufferWasRecreated && window()->format().hasAlpha()) {
        qCDebug(lcQpaBackingStore) << "Clearing" << region << "before use";
        QPainter painter(m_buffers.back()->asImage());
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        for (const QRect &rect : region)
            painter.fillRect(rect, Qt::transparent);
    }

    m_paintedRegion += region;
}

void QCALayerBackingStore::ensureBackBuffer()
{
    if (window()->format().swapBehavior() == QSurfaceFormat::SingleBuffer)
        return;

    // The current back buffer may have been assigned to a layer in a previous flush,
    // but we deferred the swap. Do it now if the surface has been picked up by CA.
    if (m_buffers.back() && m_buffers.back()->isInUse() && m_buffers.back() != m_buffers.front()) {
        qCInfo(lcQpaBackingStore) << "Back buffer has been picked up by CA, swapping to front";
        std::swap(m_buffers.back(), m_buffers.front());
    }

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
    const qreal devicePixelRatio = window()->devicePixelRatio();
    QSize requestedBufferSize = m_requestedSize * devicePixelRatio;

    const NSView *backingStoreView = static_cast<QCocoaWindow *>(window()->handle())->view();
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

        qCInfo(lcQpaBackingStore) << "Creating surface of" << requestedBufferSize
            << "based on requested" << m_requestedSize << "and dpr =" << devicePixelRatio;

        static auto pixelFormat = QImage::toPixelFormat(QImage::Format_ARGB32_Premultiplied);

        NSView *view = static_cast<QCocoaWindow *>(window()->handle())->view();
        auto colorSpace = QCFType<CGColorSpaceRef>::constructFromGet(view.window.screen.colorSpace.CGColorSpace);

        m_buffers.back().reset(new GraphicsBuffer(requestedBufferSize, devicePixelRatio, pixelFormat, colorSpace));
        return true;
    }

    return false;
}

QPaintDevice *QCALayerBackingStore::paintDevice()
{
    Q_ASSERT(m_buffers.back());
    return m_buffers.back()->asImage();
}

void QCALayerBackingStore::endPaint()
{
    qCInfo(lcQpaBackingStore) << "Paint ended with painted region" << m_paintedRegion;
    m_buffers.back()->unlock();
}

void QCALayerBackingStore::flush(QWindow *flushedWindow, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    if (!prepareForFlush())
        return;

    QMacAutoReleasePool pool;

    NSView *backingStoreView = static_cast<QCocoaWindow *>(window()->handle())->view();
    NSView *flushedView = static_cast<QCocoaWindow *>(flushedWindow->handle())->view();

    id backBufferSurface = (__bridge id)m_buffers.back()->surface();
    if (flushedView.layer.contents == backBufferSurface) {
        // We've managed to paint to the back buffer again before Core Animation had time
        // to flush the transaction and persist the layer changes to the window server.
        // The layer already knows about the back buffer, and we don't need to re-apply
        // it to pick up the surface changes, so bail out early.
        qCInfo(lcQpaBackingStore).nospace() << "Skipping flush of " << flushedView
            << ", layer already reflects back buffer";
        return;
    }

    // Trigger a new display cycle if there isn't one. This ensures that our layer updates
    // are committed as part of a display-cycle instead of on the next runloop pass. This
    // means CA won't try to throttle us if we flush too fast, and we'll coalesce our flush
    // with other pending view and layer updates.
    backingStoreView.window.viewsNeedDisplay = YES;

    if (window()->format().swapBehavior() == QSurfaceFormat::SingleBuffer) {
        // The private API [CALayer reloadValueForKeyPath:@"contents"] would be preferable,
        // but barring any side effects or performance issues we opt for the hammer for now.
        flushedView.layer.contents = nil;
    }

    qCInfo(lcQpaBackingStore) << "Flushing" << backBufferSurface
         << "to" << flushedView.layer << "of" << flushedView;

    flushedView.layer.contents = backBufferSurface;

    if (flushedView != backingStoreView) {
        const CGSize backingStoreSize = backingStoreView.bounds.size;
        flushedView.layer.contentsRect = CGRectApplyAffineTransform(
            [flushedView convertRect:flushedView.bounds toView:backingStoreView],
            // The contentsRect is in unit coordinate system
            CGAffineTransformMakeScale(1.0 / backingStoreSize.width, 1.0 / backingStoreSize.height));
    }

    // Since we may receive multiple flushes before a new frame is started, we do not
    // swap any buffers just yet. Instead we check in the next beginPaint if the layer's
    // surface is in use, and if so swap to an unused surface as the new back buffer.

    // Note: Ideally CoreAnimation would mark a surface as in use the moment we assign
    // it to a layer, but as that's not the case we may end up painting to the same back
    // buffer once more if we are painting faster than CA can ship the surfaces over to
    // the window server.
}

void QCALayerBackingStore::composeAndFlush(QWindow *window, const QRegion &region, const QPoint &offset,
                                    QPlatformTextureList *textures, bool translucentBackground)
{
    if (!prepareForFlush())
        return;

    QPlatformBackingStore::composeAndFlush(window, region, offset, textures, translucentBackground);
}

QPlatformGraphicsBuffer *QCALayerBackingStore::graphicsBuffer() const
{
    return m_buffers.back().get();
}

bool QCALayerBackingStore::prepareForFlush()
{
    if (!m_buffers.back()) {
        qCWarning(lcQpaBackingStore) << "Tried to flush backingstore without painting to it first";
        return false;
    }

    // Update dirty state of buffers based on what was painted. The back buffer will be
    // less dirty, since we painted to it, while other buffers will become more dirty.
    // This allows us to minimize copies between front and back buffers on swap in the
    // cases where the painted region overlaps with the previous frame (front buffer).
    for (const auto &buffer : m_buffers) {
        if (buffer == m_buffers.back())
            buffer->dirtyRegion -= m_paintedRegion;
        else
            buffer->dirtyRegion += m_paintedRegion;
    }

    // After painting, the back buffer is only guaranteed to have content for the painted
    // region, and may still have dirty areas that need to be synced up with the front buffer,
    // if we have one. We know that the front buffer is always up to date.
    if (!m_buffers.back()->dirtyRegion.isEmpty() && m_buffers.front() != m_buffers.back()) {
        QRegion preserveRegion = m_buffers.back()->dirtyRegion;
        qCDebug(lcQpaBackingStore) << "Preserving" << preserveRegion << "from front to back buffer";

        m_buffers.front()->lock(QPlatformGraphicsBuffer::SWReadAccess);
        const QImage *frontBuffer = m_buffers.front()->asImage();

        const QRect frontSurfaceBounds(QPoint(0, 0), m_buffers.front()->size());
        const qreal sourceDevicePixelRatio = frontBuffer->devicePixelRatio();

        m_buffers.back()->lock(QPlatformGraphicsBuffer::SWWriteAccess);
        QPainter painter(m_buffers.back()->asImage());
        painter.setCompositionMode(QPainter::CompositionMode_Source);

        // Let painter operate in device pixels, to make it easier to compare coordinates
        const qreal targetDevicePixelRatio = painter.device()->devicePixelRatio();
        painter.scale(1.0 / targetDevicePixelRatio, 1.0 / targetDevicePixelRatio);

        for (const QRect &rect : preserveRegion) {
            QRect sourceRect(rect.topLeft() * sourceDevicePixelRatio, rect.size() * sourceDevicePixelRatio);
            QRect targetRect(rect.topLeft() * targetDevicePixelRatio, rect.size() * targetDevicePixelRatio);

#ifdef QT_DEBUG
            if (Q_UNLIKELY(!frontSurfaceBounds.contains(sourceRect.bottomRight()))) {
                qCWarning(lcQpaBackingStore) << "Front buffer too small to preserve"
                    << QRegion(sourceRect).subtracted(frontSurfaceBounds);
            }
#endif
            painter.drawImage(targetRect, *frontBuffer, sourceRect);
        }

        m_buffers.back()->unlock();
        m_buffers.front()->unlock();

        // The back buffer is now completely in sync, ready to be presented
        m_buffers.back()->dirtyRegion = QRegion();
    }

    // Prepare for another round of painting
    m_paintedRegion = QRegion();

    return true;
}

// ----------------------------------------------------------------------------

QCALayerBackingStore::GraphicsBuffer::GraphicsBuffer(const QSize &size, qreal devicePixelRatio,
                                const QPixelFormat &format, QCFType<CGColorSpaceRef> colorSpace)
    : QIOSurfaceGraphicsBuffer(size, format, colorSpace)
    , dirtyRegion(0, 0, size.width() / devicePixelRatio, size.height() / devicePixelRatio)
    , m_devicePixelRatio(devicePixelRatio)
{
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
