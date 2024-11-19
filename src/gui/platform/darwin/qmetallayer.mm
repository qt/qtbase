// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmetallayer_p.h"

#include <QtCore/qreadwritelock.h>
#include <QtCore/qrect.h>

using namespace std::chrono_literals;

QT_BEGIN_NAMESPACE
Q_LOGGING_CATEGORY(lcMetalLayer, "qt.gui.metal")
QT_END_NAMESPACE

QT_USE_NAMESPACE

@implementation QMetalLayer
{
    std::unique_ptr<QReadWriteLock> m_displayLock;
}

- (instancetype)init
{
    if ((self = [super init])) {
        m_displayLock.reset(new QReadWriteLock(QReadWriteLock::Recursive));
        self.mainThreadPresentation = nil;
    }

    return self;
}

- (QReadWriteLock &)displayLock
{
    return *m_displayLock.get();
}

- (void)setNeedsDisplay
{
    [self setNeedsDisplayInRect:CGRectInfinite];
}

- (void)setNeedsDisplayInRect:(CGRect)rect
{
    if (!self.needsDisplay) {
        // We lock for writing here, blocking in case a secondary thread is in
        // the middle of presenting to the layer, as we want the main thread's
        // display to happen after the secondary thread finishes presenting.
        qCDebug(lcMetalLayer) << "Locking" << self << "for writing"
            << "due to needing display in rect" << QRectF::fromCGRect(rect);

        // For added safety, we use a 5 second timeout, and try to fail
        // gracefully by not marking the layer as needing display, as
        // doing so would lead us to unlock and unheld lock in displayLayer.
        if (!self.displayLock.tryLockForWrite(5s)) {
            qCWarning(lcMetalLayer) << "Timed out waiting for display lock";
            return;
        }
    }

    [super setNeedsDisplayInRect:rect];
}

- (id<CAMetalDrawable>)nextDrawable
{
    {
        // Drop the presentation block early, so that if the main thread for
        // some reason doesn't handle the presentation, the block won't hold on
        // to a drawable unnecessarily.
        QMacAutoReleasePool pool;
        self.mainThreadPresentation = nil;
    }

    return [super nextDrawable];
}


@end
