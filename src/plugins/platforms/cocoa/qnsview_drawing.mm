/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

// This file is included from qnsview.mm, and only used to organize the code

@implementation QNSView (Drawing)

- (void)initDrawing
{
    [self updateLayerBacking];
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

- (void)updateLayerBacking
{
    self.wantsLayer = [self layerEnabledByMacOS]
        || [self layerExplicitlyRequested]
        || [self shouldUseMetalLayer];
}

- (BOOL)layerEnabledByMacOS
{
    // AppKit has its own logic for this, but if we rely on that, our layers are created
    // by AppKit at a point where we've already set up other parts of the platform plugin
    // based on the presence of layers or not. Once we've rewritten these parts to support
    // dynamically picking up layer enablement we can let AppKit do its thing.
    return QMacVersion::buildSDK() >= QOperatingSystemVersion::MacOSMojave
        && QMacVersion::currentRuntime() >= QOperatingSystemVersion::MacOSMojave;
}

- (BOOL)layerExplicitlyRequested
{
    static bool wantsLayer = [&]() {
        int wantsLayer = qt_mac_resolveOption(-1, m_platformWindow->window(),
            "_q_mac_wantsLayer", "QT_MAC_WANTS_LAYER");

        if (wantsLayer != -1 && [self layerEnabledByMacOS]) {
            qCWarning(lcQpaDrawing) << "Layer-backing cannot be explicitly controlled on 10.14 when built against the 10.14 SDK";
            return true;
        }

        return wantsLayer == 1;
    }();

    return wantsLayer;
}

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
        << "with" << layer << "due to being" << ([self layerExplicitlyRequested] ? "explicitly requested"
            : [self shouldUseMetalLayer] ? "needed by surface type" : "enabled by macOS");

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
    Q_UNUSED(layer); Q_UNUSED(scale); Q_UNUSED(window);
    return NO;
}

// ----------------------- Draw callbacks -----------------------

/*
    This method is called by AppKit for the non-layer case, where we are
    drawing into the NSWindow's surface.
*/
- (void)drawRect:(NSRect)dirtyBoundingRect
{
    Q_ASSERT_X(!self.layer, "QNSView",
        "The drawRect code path should not be hit when we are layer backed");

    if (!m_platformWindow)
        return;

    QRegion exposedRegion;
    const NSRect *dirtyRects;
    NSInteger numDirtyRects;
    [self getRectsBeingDrawn:&dirtyRects count:&numDirtyRects];
    for (int i = 0; i < numDirtyRects; ++i)
        exposedRegion += QRectF::fromCGRect(dirtyRects[i]).toRect();

    if (exposedRegion.isEmpty())
        exposedRegion = QRectF::fromCGRect(dirtyBoundingRect).toRect();

    qCDebug(lcQpaDrawing) << "[QNSView drawRect:]" << m_platformWindow->window() << exposedRegion;
    m_platformWindow->handleExposeEvent(exposedRegion);
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
