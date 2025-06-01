// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// This file is included from qnsview.mm, and only used to organize the code

@implementation QContainerLayer {
    CALayer *m_contentLayer;
}
- (instancetype)initWithContentLayer:(CALayer *)contentLayer
{
    if ((self = [super init])) {
        m_contentLayer = contentLayer;
        [self addSublayer:contentLayer];
        contentLayer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
    }
    return self;
}

- (CALayer*)contentLayer
{
    return m_contentLayer;
}

- (void)setNeedsDisplay
{
    [self setNeedsDisplayInRect:CGRectInfinite];
}

- (void)setNeedsDisplayInRect:(CGRect)rect
{
    [super setNeedsDisplayInRect:rect];
    [self.contentLayer setNeedsDisplayInRect:rect];
}
@end

@implementation QNSView (Drawing)

- (void)initDrawing
{
    // Pick up and persist requested color space from surface format
    const QSurfaceFormat surfaceFormat = m_platformWindow->format();
    if (QColorSpace colorSpace = surfaceFormat.colorSpace(); colorSpace.isValid()) {
        NSData *iccData = colorSpace.iccProfile().toNSData();
        self.colorSpace = [[[NSColorSpace alloc] initWithICCProfileData:iccData] autorelease];
    }

    // Trigger creation of the layer
    self.wantsLayer = YES;
}

- (BOOL)isOpaque
{
    if (!m_platformWindow)
        return true;
    return m_platformWindow->isOpaque();
}

- (BOOL)isFlipped
{
    return YES;
}

- (NSColorSpace*)colorSpace
{
    // If no explicit color space was set, use the NSWindow's color space
    return m_colorSpace ? m_colorSpace : self.window.colorSpace;
}

// ----------------------- Layer setup -----------------------

- (BOOL)shouldUseMetalLayer
{
    // MetalSurface needs a layer, and so does VulkanSurface (via MoltenVK)
    QSurface::SurfaceType surfaceType = m_platformWindow->window()->surfaceType();
    return surfaceType == QWindow::MetalSurface || surfaceType == QWindow::VulkanSurface;
}

/*
    This method is called by AppKit when layer-backing is requested by
    setting wantsLayer too YES (via -[NSView _updateLayerBackedness]),
    or in cases where AppKit itself decides that a view should be
    layer-backed.

    Note however that some code paths in AppKit will not go via this
    method for creating the backing layer, and will instead create the
    layer manually, and just call setLayer. An example of this is when
    an NSOpenGLContext is attached to a view, in which case AppKit will
    create a new layer in NSOpenGLContextSetLayerOnViewIfNecessary.

    For this reason we leave the implementation of this override as
    minimal as possible, only focusing on creating the appropriate
    layer type, and then leave it up to setLayer to do the work of
    making sure the layer is set up correctly.
*/
- (CALayer *)makeBackingLayer
{
    if ([self shouldUseMetalLayer]) {
        // Check if Metal is supported. If it isn't then it's most likely
        // too late at this point and the QWindow will be non-functional,
        // but we can at least print a warning.
        if ([MTLCreateSystemDefaultDevice() autorelease]) {
            static bool allowPresentsWithTransaction =
                !qEnvironmentVariableIsSet("QT_MTL_NO_TRANSACTION");
            return allowPresentsWithTransaction ?
                [QMetalLayer layer] : [CAMetalLayer layer];
        } else {
            qCWarning(lcQpaDrawing) << "Failed to create QWindow::MetalSurface."
                << "Metal is not supported by any of the GPUs in this system.";
        }
    }

    // We handle drawing via displayLayer instead of drawRect or updateLayer,
    // as the latter two do not work for CAMetalLayer. And we handle content
    // scale manually for the same reason. Which means we don't really need
    // NSViewBackingLayer. In fact it just gets in the way, by assuming that
    // if we don't have a drawRect function we "draw nothing".
    return [CALayer layer];
}

/*
    This method is called by AppKit whenever the view is asked to change
    its layer, which can happen both as a result of enabling layer-backing,
    or when a layer is set explicitly. The latter can happen both when a
    view is layer-hosting, or when AppKit internals are switching out the
    layer-backed view, as described above for makeBackingLayer.
*/
- (void)setLayer:(CALayer *)layer
{
    qCDebug(lcQpaDrawing) << "Making" << self
        << (self.wantsLayer ? "layer-backed" : "layer-hosted")
        << "with" << layer;

    if (layer.delegate && layer.delegate != self) {
        qCWarning(lcQpaDrawing) << "Layer already has delegate" << layer.delegate
            << "This delegate is responsible for all view updates for" << self;
    } else {
        layer.delegate = self;
    }

    layer.name = @"Qt content layer";

    static const bool containerLayerOptOut = qEnvironmentVariableIsSet("QT_MAC_NO_CONTAINER_LAYER");
    if (m_platformWindow->window()->surfaceType() != QSurface::OpenGLSurface && !containerLayerOptOut) {
        qCDebug(lcQpaDrawing) << "Wrapping content layer" << layer << "in container layer";
        auto *containerLayer = [[[QContainerLayer alloc] initWithContentLayer:layer] autorelease];
        containerLayer.name = @"Qt container layer";
        containerLayer.delegate = self;
        layer = containerLayer;
    }

    [super setLayer:layer];

    [self propagateBackingProperties];

    if (self.opaque && lcQpaDrawing().isDebugEnabled()) {
        // If the view claims to be opaque we expect it to fill the entire
        // layer with content, in which case we want to detect any areas
        // where it doesn't.
        layer.backgroundColor = NSColor.magentaColor.CGColor;
    }
}

// ----------------------- Layer updates -----------------------

- (NSViewLayerContentsRedrawPolicy)layerContentsRedrawPolicy
{
    // We need to set this explicitly since the super implementation
    // returns LayerContentsRedrawNever for custom layers like CAMetalLayer.
    return NSViewLayerContentsRedrawDuringViewResize;
}

- (NSViewLayerContentsPlacement)layerContentsPlacement
{
    // Always place the layer at top left without any automatic scaling.
    // This will highlight situations where we're missing content for the
    // layer by not responding to the displayLayer: request synchronously.
    // It also allows us to re-use larger layers when resizing a window down.
    return NSViewLayerContentsPlacementTopLeft;
}

- (void)viewDidChangeBackingProperties
{
    qCDebug(lcQpaDrawing) << "Backing properties changed for" << self;

    [self propagateBackingProperties];

    // Ideally we would plumb this situation through QPA in a way that lets
    // clients invalidate their own caches, recreate QBackingStore, etc.

    // QPA supports DPR (scale) change notifications. We are not sure
    // based on this event that it is the scale that has changed (it
    // could be the color space), however QPA will determine if it has
    // actually changed.
    QWindowSystemInterface::handleWindowDevicePixelRatioChanged
        <QWindowSystemInterface::SynchronousDelivery>(m_platformWindow->window());

    // Trigger an expose, and let QCocoaBackingStore deal with
    // buffer invalidation internally.
    [self setNeedsDisplay:YES];
}

- (void)propagateBackingProperties
{
    if (!self.layer)
        return;

    // We expect clients to fill the layer with retina aware content,
    // based on the devicePixelRatio of the QWindow, so we set the
    // layer's content scale to match that. By going via devicePixelRatio
    // instead of applying the NSWindow's backingScaleFactor, we also take
    // into account OpenGL views with wantsBestResolutionOpenGLSurface set
    // to NO. In this case the window will have a backingScaleFactor of 2,
    // but the QWindow will have a devicePixelRatio of 1.
    auto devicePixelRatio = m_platformWindow->devicePixelRatio();
    auto *contentLayer = m_platformWindow->contentLayer();
    qCDebug(lcQpaDrawing) << "Updating" << contentLayer << "content scale to" << devicePixelRatio;
    contentLayer.contentsScale = devicePixelRatio;

    if ([contentLayer isKindOfClass:CAMetalLayer.class]) {
        CAMetalLayer *metalLayer = static_cast<CAMetalLayer *>(contentLayer);
        metalLayer.colorspace = self.colorSpace.CGColorSpace;
        qCDebug(lcQpaDrawing) << "Set" << metalLayer << "color space to" << metalLayer.colorspace;
    }
}

/*
    This method is called by AppKit to determine whether it should update
    the contentScale of the layer to match the window backing scale.

    We always return NO since we're updating the contents scale manually.
*/
- (BOOL)layer:(CALayer *)layer shouldInheritContentsScale:(CGFloat)scale fromWindow:(NSWindow *)window
{
    Q_UNUSED(layer);
    Q_UNUSED(scale);
    Q_UNUSED(window);
    return NO;
}

// ----------------------- Draw callbacks -----------------------

/*
    We set our view up as the layer's delegate, which means we get
    first dibs on displaying the layer, without needing to go through
    updateLayer or drawRect.
*/
- (void)displayLayer:(CALayer *)layer
{
    if (auto *containerLayer = qt_objc_cast<QContainerLayer*>(layer)) {
        qCDebug(lcQpaDrawing) << "Skipping display of" << containerLayer
            << "as display is handled by content layer" << containerLayer.contentLayer;
        return;
    }

    if (!m_platformWindow)
        return;

    if (!NSThread.isMainThread) {
        // Qt is calling AppKit APIs such as -[NSOpenGLContext setView:] on secondary threads,
        // which we shouldn't do. This may result in AppKit (wrongly) triggering a display on
        // the thread where we made the call, so block it here and defer to the main thread.
        qCWarning(lcQpaDrawing) << "Display non non-main thread! Deferring to main thread";
        dispatch_async(dispatch_get_main_queue(), ^{ self.needsDisplay = YES; });
        return;
    }

    const auto handleExposeEvent = [&]{
        const auto bounds = QRectF::fromCGRect(self.bounds).toRect();
        qCDebug(lcQpaDrawing) << "[QNSView displayLayer]" << m_platformWindow->window() << bounds;
        m_platformWindow->handleExposeEvent(bounds);
    };

    if (auto *qtMetalLayer = qt_objc_cast<QMetalLayer*>(layer)) {
        const bool presentedWithTransaction = qtMetalLayer.presentsWithTransaction;
        qtMetalLayer.presentsWithTransaction = YES;

        handleExposeEvent();

        {
            // Clearing the mainThreadPresentation below will auto-release the
            // block held by the property, which in turn holds on to drawables,
            // so we want to clean up as soon as possible, to prevent stalling
            // when requesting new drawables. But merely referencing the block
            // below for the nil-check will make another auto-released copy of
            // the block, so the scope of the auto-release pool needs to include
            // that check as well.
            QMacAutoReleasePool pool;

            // If the expose event resulted in a secondary thread requesting that its
            // drawable should be presented on the main thread with transaction, do so.
            if (auto mainThreadPresentation = qtMetalLayer.mainThreadPresentation) {
                mainThreadPresentation();
                qtMetalLayer.mainThreadPresentation = nil;
            }
        }

        qtMetalLayer.presentsWithTransaction = presentedWithTransaction;

        // We're done presenting, but we must wait to unlock the display lock
        // until the display cycle finishes, as otherwise the render thread may
        // step in and present before the transaction commits. The display lock
        // is recursive, so setNeedsDisplay can be safely called in the meantime
        // without any issue.
        QMetaObject::invokeMethod(m_platformWindow, [qtMetalLayer]{
            qCDebug(lcMetalLayer) << "Unlocking" << qtMetalLayer << "after finishing display-cycle";
            qtMetalLayer.displayLock.unlock();
        }, Qt::QueuedConnection);
    } else {
        handleExposeEvent();
    }
}

@end
