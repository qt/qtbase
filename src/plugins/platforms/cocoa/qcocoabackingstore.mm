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

    // Use local pool so that any stale image references are cleaned up after flushing
    QMacAutoReleasePool pool;

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
        qCDebug(lcCocoaBackingStore) << "Flushing" << region << "of" << view << qPrintable(targetViewDescription);
    }

    // Prevent potentially costly color conversion by assigning the display color space
    // to the backingstore image. This does not copy the underlying image data.
    CGColorSpaceRef displayColorSpace = view.window.screen.colorSpace.CGColorSpace;
    QCFType<CGImageRef> cgImage = CGImageCreateCopyWithColorSpace(
        QCFType<CGImageRef>(m_image.toCGImage()), displayColorSpace);

    if (view.layer) {
        // In layer-backed mode, locking focus on a view does not give the right
        // view transformation, and doesn't give us a graphics context to render
        // via when drawing outside of the display cycle. Instead we tell AppKit
        // that we want to update the layer's content, via [NSView wantsUpdateLayer],
        // which result in AppKit not creating a backingstore for each layer, and
        // we then directly set the layer's backingstore (content) to our backingstore,
        // masked to the part of the subview that is relevant.
        // FIXME: Figure out if there's a way to do partial updates
        view.layer.contents = (__bridge id)static_cast<CGImageRef>(cgImage);
        if (view != topLevelView) {
            view.layer.contentsRect = CGRectApplyAffineTransform(
                [view convertRect:view.bounds toView:topLevelView],
                // The contentsRect is in unit coordinate system
                CGAffineTransformMakeScale(1.0 / m_image.width(), 1.0 / m_image.height()));
        }
        return;
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

    QCocoaWindow *topLevelCocoaWindow = static_cast<QCocoaWindow *>(topLevelWindow->handle());
    if (Q_UNLIKELY(topLevelCocoaWindow->m_needsInvalidateShadow)) {
        [topLevelView.window invalidateShadow];
        topLevelCocoaWindow->m_needsInvalidateShadow = false;
    }

    // -------------------------------------------------------------------------

    if (shouldHandleViewLockManually)
        [view unlockFocus];

    if (drawingOutsideOfDisplayCycle) {
        redrawRoundedBottomCorners([view convertRect:region.boundingRect().toCGRect() toView:nil]);
        [view.window flushWindow];
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
void QCocoaBackingStore::redrawRoundedBottomCorners(CGRect windowRect) const
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

QT_END_NAMESPACE
