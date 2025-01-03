// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// This file is included from qnsview.mm, and only used to organize the code

@implementation QNSView (Drawing)

- (void)initDrawing
{
    if (qt_mac_resolveOption(-1, m_platformWindow->window(),
        "_q_mac_wantsLayer", "QT_MAC_WANTS_LAYER") != -1) {
        qCWarning(lcQpaDrawing) << "Layer-backing is always enabled."
            << " QT_MAC_WANTS_LAYER/_q_mac_wantsLayer has no effect.";
    }

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
            return [CAMetalLayer layer];
        } else {
            qCWarning(lcQpaDrawing) << "Failed to create QWindow::MetalSurface."
                << "Metal is not supported by any of the GPUs in this system.";
        }
    }

    return [super makeBackingLayer];
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

    [super setLayer:layer];

    // When adding a view to a view hierarchy the backing properties will change
    // which results in updating the contents scale, but in case of switching the
    // layer on a view that's already in a view hierarchy we need to manually ensure
    // the scale is up to date.
    if (self.superview)
        [self updateLayerContentsScale];

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

    if (self.layer)
        [self updateLayerContentsScale];

    // Ideally we would plumb this situation through QPA in a way that lets
    // clients invalidate their own caches, recreate QBackingStore, etc.
    // For now we trigger an expose, and let QCocoaBackingStore deal with
    // buffer invalidation internally.
    [self setNeedsDisplay:YES];
}

- (void)updateLayerContentsScale
{
    // We expect clients to fill the layer with retina aware content,
    // based on the devicePixelRatio of the QWindow, so we set the
    // layer's content scale to match that. By going via devicePixelRatio
    // instead of applying the NSWindow's backingScaleFactor, we also take
    // into account OpenGL views with wantsBestResolutionOpenGLSurface set
    // to NO. In this case the window will have a backingScaleFactor of 2,
    // but the QWindow will have a devicePixelRatio of 1.
    auto devicePixelRatio = m_platformWindow->devicePixelRatio();
    qCDebug(lcQpaDrawing) << "Updating" << self.layer << "content scale to" << devicePixelRatio;
    self.layer.contentsScale = devicePixelRatio;
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
    This method is called by AppKit for the non-layer case, where we are
    drawing into the NSWindow's surface.
*/
- (void)drawRect:(NSRect)dirtyBoundingRect
{
    Q_UNUSED(dirtyBoundingRect);
    // As we are layer backed we shouldn't really end up here, but AppKit will
    // in some cases call this method just because we implement it.
    // FIXME: Remove drawRect and switch from displayLayer to updateLayer
    qCWarning(lcQpaDrawing) << "[QNSView drawRect] called for layer backed view";
}

/*
    This method is called by AppKit when we are layer-backed, where
    we are drawing into the layer.
*/
- (void)displayLayer:(CALayer *)layer
{
    Q_ASSERT_X(self.layer && layer == self.layer, "QNSView",
        "The displayLayer code path should only be hit for our own layer");

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

    qCDebug(lcQpaDrawing) << "[QNSView displayLayer]" << m_platformWindow->window();
    m_platformWindow->handleExposeEvent(QRectF::fromCGRect(self.bounds).toRect());
}

@end
