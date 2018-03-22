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

@implementation QT_MANGLE_NAMESPACE(QNSView) (DrawingAPI)

- (void)requestUpdate
{
    if (self.needsDisplay) {
        // If the view already has needsDisplay set it means that there may be code waiting for
        // a real expose event, so we can't issue setNeedsDisplay now as a way to trigger an
        // update request. We will re-trigger requestUpdate from drawRect.
        return;
    }

    [self setNeedsDisplay:YES];
    m_updateRequested = true;
}

#ifndef QT_NO_OPENGL
- (void)setQCocoaGLContext:(QCocoaGLContext *)context
{
    m_glContext = context;
    [m_glContext->nsOpenGLContext() setView:self];
    if (![m_glContext->nsOpenGLContext() view]) {
        //was unable to set view
        m_shouldSetGLContextinDrawRect = true;
    }
}
#endif

@end

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

- (void)setNeedsDisplayInRect:(NSRect)rect
{
    [super setNeedsDisplayInRect:rect];
    m_updateRequested = false;
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
    [self updateRegion:exposedRegion];
}

- (void)updateRegion:(QRegion)dirtyRegion
{
#ifndef QT_NO_OPENGL
    if (m_glContext && m_shouldSetGLContextinDrawRect) {
        [m_glContext->nsOpenGLContext() setView:self];
        m_shouldSetGLContextinDrawRect = false;
    }
#endif

    QWindowPrivate *windowPrivate = qt_window_private(m_platformWindow->window());

    if (m_updateRequested) {
        Q_ASSERT(windowPrivate->updateRequestPending);
        qCDebug(lcQpaWindow) << "Delivering update request to" << m_platformWindow->window();
        m_platformWindow->deliverUpdateRequest();
        m_updateRequested = false;
    } else {
        m_platformWindow->handleExposeEvent(dirtyRegion);
    }

    if (windowPrivate->updateRequestPending) {
        // A call to QWindow::requestUpdate was issued during event delivery above,
        // but AppKit will reset the needsDisplay state of the view after completing
        // the current display cycle, so we need to defer the request to redisplay.
        // FIXME: Perhaps this should be a trigger to enable CADisplayLink?
        qCDebug(lcQpaDrawing) << "[QNSView drawRect:] issuing deferred setNeedsDisplay due to pending update request";
        dispatch_async(dispatch_get_main_queue (), ^{ [self requestUpdate]; });
    }
}

- (BOOL)wantsLayer
{
    Q_ASSERT(m_platformWindow);

    // Toggling the private QWindow property or the environment variable
    // on and off is not a supported use-case, so this code is effectively
    // returning a constant for the lifetime of our QSNSView, which means
    // we don't care about emitting KVO signals for @"wantsLayer".
    return qt_mac_resolveOption(false, m_platformWindow->window(),
        "_q_mac_wantsLayer", "QT_MAC_WANTS_LAYER");
}

- (void)displayLayer:(CALayer *)layer
{
    Q_ASSERT(layer == self.layer);

    if (!m_platformWindow)
        return;

    qCDebug(lcQpaDrawing) << "[QNSView displayLayer]" << m_platformWindow->window();

    // FIXME: Find out if there's a way to resolve the dirty rect like in drawRect:
    [self updateRegion:QRectF::fromCGRect(self.bounds).toRect()];
}

- (void)viewDidChangeBackingProperties
{
    if (self.layer)
        self.layer.contentsScale = self.window.backingScaleFactor;
}

@end
