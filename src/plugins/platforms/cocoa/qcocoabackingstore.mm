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

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcCocoaBackingStore, "qt.qpa.cocoa.backingstore");

QCocoaBackingStore::QCocoaBackingStore(QWindow *window)
    : QRasterBackingStore(window)
{
}

QCocoaBackingStore::~QCocoaBackingStore()
{
}

bool QCocoaBackingStore::windowHasUnifiedToolbar() const
{
    Q_ASSERT(window()->handle());
    return static_cast<QCocoaWindow *>(window()->handle())->m_drawContentBorderGradient;
}

QImage::Format QCocoaBackingStore::format() const
{
    if (windowHasUnifiedToolbar())
        return QImage::Format_ARGB32_Premultiplied;

    return QRasterBackingStore::format();
}

#if !QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_12)
static const NSCompositingOperation NSCompositingOperationCopy = NSCompositeCopy;
static const NSCompositingOperation NSCompositingOperationSourceOver = NSCompositeSourceOver;
#endif

/*!
    Flushes the given \a region from the specified \a window onto the
    screen.

    The \a window is the top level window represented by this backingstore,
    or a non-transient child of that window.

    If the \a window is a child window, the \a region will be in child window
    coordinates, and the \a offset will be the child window's offset in relation
    to the backingstore's top level window.
*/
void QCocoaBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    if (m_image.isNull())
        return;

    const QWindow *topLevelWindow = this->window();

    Q_ASSERT(topLevelWindow->handle() && window->handle());
    Q_ASSERT(!topLevelWindow->handle()->isForeignWindow() && !window->handle()->isForeignWindow());

    QNSView *topLevelView = qnsview_cast(static_cast<QCocoaWindow *>(topLevelWindow->handle())->view());
    QNSView *view = qnsview_cast(static_cast<QCocoaWindow *>(window->handle())->view());

    if (lcCocoaBackingStore().isDebugEnabled()) {
        QString targetViewDescription;
        if (view != topLevelView) {
            QDebug targetDebug(&targetViewDescription);
            targetDebug << "onto" << topLevelView << "at" << offset;
        }
        qCDebug(lcCocoaBackingStore) << "Flushing" << region << "of" << view << targetViewDescription;
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

    // If the flushed window is a content view, and not in unified toolbar mode,
    // we can get away with copying the backingstore instead of blending.
    const NSCompositingOperation compositingOperation = static_cast<QCocoaWindow *>(
        window->handle())->isContentView() && !windowHasUnifiedToolbar() ?
            NSCompositingOperationCopy : NSCompositingOperationSourceOver;

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

    // Create temporary image to use for blitting, without copying image data
    NSImage *backingStoreImage = [[[NSImage alloc]
        initWithCGImage:QCFType<CGImageRef>(m_image.toCGImage()) size:NSZeroSize] autorelease];

    if ([topLevelView hasMask]) {
        // FIXME: Implement via NSBezierPath and addClip
        CGRect boundingRect = region.boundingRect().toCGRect();
        QCFType<CGImageRef> subMask = CGImageCreateWithImageInRect([topLevelView maskImage], boundingRect);
        CGContextClipToMask(graphicsContext.CGContext, boundingRect, subMask);
    }

    for (const QRect &viewLocalRect : region) {
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

    if (drawingOutsideOfDisplayCycle)
        [view.window flushWindow];

    // FIXME: Tie to changing window flags and/or mask instead
    [view invalidateWindowShadowIfNeeded];
}

QT_END_NAMESPACE
