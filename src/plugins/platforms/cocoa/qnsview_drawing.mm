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

@implementation QT_MANGLE_NAMESPACE(QNSView) (Drawing)

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

- (void)drawRect:(NSRect)dirtyRect
{
    Q_UNUSED(dirtyRect);

    if (!m_platformWindow)
        return;

    QRegion exposedRegion;
    const NSRect *dirtyRects;
    NSInteger numDirtyRects;
    [self getRectsBeingDrawn:&dirtyRects count:&numDirtyRects];
    for (int i = 0; i < numDirtyRects; ++i)
        exposedRegion += QRectF::fromCGRect(dirtyRects[i]).toRect();

    qCDebug(lcQpaDrawing) << "[QNSView drawRect:]" << m_platformWindow->window() << exposedRegion;
    m_platformWindow->handleExposeEvent(exposedRegion);
}

- (BOOL)shouldUseMetalLayer
{
    // MetalSurface needs a layer, and so does VulkanSurface (via MoltenVK)
    QSurface::SurfaceType surfaceType = m_platformWindow->window()->surfaceType();
    return surfaceType == QWindow::MetalSurface || surfaceType == QWindow::VulkanSurface;
}

- (BOOL)wantsLayerHelper
{
    Q_ASSERT(m_platformWindow);

    bool wantsLayer = qt_mac_resolveOption(true, m_platformWindow->window(),
        "_q_mac_wantsLayer", "QT_MAC_WANTS_LAYER");

    bool layerForSurfaceType = [self shouldUseMetalLayer];

    return wantsLayer || layerForSurfaceType;
}

- (CALayer *)makeBackingLayer
{
    if ([self shouldUseMetalLayer]) {
        // Check if Metal is supported. If it isn't then it's most likely
        // too late at this point and the QWindow will be non-functional,
        // but we can at least print a warning.
        if (![MTLCreateSystemDefaultDevice() autorelease]) {
            qWarning() << "QWindow initialization error: Metal is not supported";
            return [super makeBackingLayer];
        }

        CAMetalLayer *layer = [CAMetalLayer layer];

        // Set the contentsScale for the layer. This is normally done in
        // viewDidChangeBackingProperties, however on startup that function
        // is called before the layer is created here. The layer's drawableSize
        // is updated from layoutSublayersOfLayer as usual.
        layer.contentsScale = self.window.backingScaleFactor;

        return layer;
    }

    return [super makeBackingLayer];
}

- (NSViewLayerContentsRedrawPolicy)layerContentsRedrawPolicy
{
    // We need to set this explicitly since the super implementation
    // returns LayerContentsRedrawNever for custom layers like CAMetalLayer.
    return NSViewLayerContentsRedrawDuringViewResize;
}

- (void)updateMetalLayerDrawableSize:(CAMetalLayer *)layer
{
    CGSize drawableSize = layer.bounds.size;
    drawableSize.width *= layer.contentsScale;
    drawableSize.height *= layer.contentsScale;
    layer.drawableSize = drawableSize;
}

- (void)layoutSublayersOfLayer:(CALayer *)layer
{
    if ([layer isKindOfClass:CAMetalLayer.class])
        [self updateMetalLayerDrawableSize:static_cast<CAMetalLayer* >(layer)];
}

- (void)displayLayer:(CALayer *)layer
{
    Q_ASSERT(layer == self.layer);

    if (!m_platformWindow)
        return;

    qCDebug(lcQpaDrawing) << "[QNSView displayLayer]" << m_platformWindow->window();

    // FIXME: Find out if there's a way to resolve the dirty rect like in drawRect:
    m_platformWindow->handleExposeEvent(QRectF::fromCGRect(self.bounds).toRect());
}

- (void)viewDidChangeBackingProperties
{
    CALayer *layer = self.layer;
    if (!layer)
        return;

    layer.contentsScale = self.window.backingScaleFactor;

    // Metal layers must be manually updated on e.g. screen change
    if ([layer isKindOfClass:CAMetalLayer.class]) {
        [self updateMetalLayerDrawableSize:static_cast<CAMetalLayer* >(layer)];
        [self setNeedsDisplay:YES];
    }
}

@end
