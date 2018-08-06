/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "qcocoascreen.h"

#include "qcocoawindow.h"
#include "qcocoahelpers.h"

#include <QtCore/qcoreapplication.h>
#include <QtGui/private/qcoregraphics_p.h>

#include <IOKit/graphics/IOGraphicsLib.h>

QT_BEGIN_NAMESPACE

class QCoreTextFontEngine;
class QFontEngineFT;

QCocoaScreen::QCocoaScreen(int screenIndex)
    : QPlatformScreen(), m_screenIndex(screenIndex), m_refreshRate(60.0)
{
    updateGeometry();
    m_cursor = new QCocoaCursor;
}

QCocoaScreen::~QCocoaScreen()
{
    delete m_cursor;
}

NSScreen *QCocoaScreen::nativeScreen() const
{
    NSArray *screens = [NSScreen screens];

    // Stale reference, screen configuration has changed
    if (m_screenIndex < 0 || (NSUInteger)m_screenIndex >= [screens count])
        return nil;

    return [screens objectAtIndex:m_screenIndex];
}

static QString displayName(CGDirectDisplayID displayID)
{
    QIOType<io_iterator_t> iterator;
    if (IOServiceGetMatchingServices(kIOMasterPortDefault,
        IOServiceMatching("IODisplayConnect"), &iterator))
        return QString();

    QIOType<io_service_t> display;
    while ((display = IOIteratorNext(iterator)) != 0)
    {
        NSDictionary *info = [(__bridge NSDictionary*)IODisplayCreateInfoDictionary(
            display, kIODisplayOnlyPreferredName) autorelease];

        if ([[info objectForKey:@kDisplayVendorID] longValue] != CGDisplayVendorNumber(displayID))
            continue;

        if ([[info objectForKey:@kDisplayProductID] longValue] != CGDisplayModelNumber(displayID))
            continue;

        if ([[info objectForKey:@kDisplaySerialNumber] longValue] != CGDisplaySerialNumber(displayID))
            continue;

        NSDictionary *localizedNames = [info objectForKey:@kDisplayProductName];
        if (![localizedNames count])
            break; // Correct screen, but no name in dictionary

        return QString::fromNSString([localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]]);
    }

    return QString();
}

void QCocoaScreen::updateGeometry()
{
    NSScreen *nsScreen = nativeScreen();
    if (!nsScreen)
        return;

    // The reference screen for the geometry is always the primary screen
    QRectF primaryScreenGeometry = QRectF::fromCGRect([[NSScreen screens] firstObject].frame);
    m_geometry = qt_mac_flip(QRectF::fromCGRect(nsScreen.frame), primaryScreenGeometry).toRect();
    m_availableGeometry = qt_mac_flip(QRectF::fromCGRect(nsScreen.visibleFrame), primaryScreenGeometry).toRect();

    m_format = QImage::Format_RGB32;
    m_depth = NSBitsPerPixelFromDepth([nsScreen depth]);

    NSDictionary *devDesc = [nsScreen deviceDescription];
    CGDirectDisplayID dpy = [[devDesc objectForKey:@"NSScreenNumber"] unsignedIntValue];
    CGSize size = CGDisplayScreenSize(dpy);
    m_physicalSize = QSizeF(size.width, size.height);
    m_logicalDpi.first = 72;
    m_logicalDpi.second = 72;
    CGDisplayModeRef displayMode = CGDisplayCopyDisplayMode(dpy);
    float refresh = CGDisplayModeGetRefreshRate(displayMode);
    CGDisplayModeRelease(displayMode);
    if (refresh > 0)
        m_refreshRate = refresh;

    m_name = displayName(dpy);

    QWindowSystemInterface::handleScreenGeometryChange(screen(), geometry(), availableGeometry());
    QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(screen(), m_logicalDpi.first, m_logicalDpi.second);
    QWindowSystemInterface::handleScreenRefreshRateChange(screen(), m_refreshRate);

    // When a screen changes its geometry, AppKit will send us a NSWindowDidMoveNotification
    // for each window, resulting in calls to handleGeometryChange(), but this happens before
    // the NSApplicationDidChangeScreenParametersNotification, so when we map the new geometry
    // (which is correct at that point) to the screen using QCocoaScreen::mapFromNative(), we
    // end up using the stale screen geometry, and the new window geometry we report is wrong.
    // To make sure we finally report the correct window geometry, we need to do another pass
    // of geometry reporting, now that the screen properties have been updates. FIXME: Ideally
    // this would be solved by not caching the screen properties in QCocoaScreen, but that
    // requires more research.
    for (QWindow *window : windows()) {
        if (QCocoaWindow *cocoaWindow = static_cast<QCocoaWindow*>(window->handle()))
            cocoaWindow->handleGeometryChange();
    }
}

qreal QCocoaScreen::devicePixelRatio() const
{
    QMacAutoReleasePool pool;
    NSScreen *nsScreen = nativeScreen();
    return qreal(nsScreen ? [nsScreen backingScaleFactor] : 1.0);
}

QPlatformScreen::SubpixelAntialiasingType QCocoaScreen::subpixelAntialiasingTypeHint() const
{
    QPlatformScreen::SubpixelAntialiasingType type = QPlatformScreen::subpixelAntialiasingTypeHint();
    if (type == QPlatformScreen::Subpixel_None) {
        // Every OSX machine has RGB pixels unless a peculiar or rotated non-Apple screen is attached
        type = QPlatformScreen::Subpixel_RGB;
    }
    return type;
}

QWindow *QCocoaScreen::topLevelAt(const QPoint &point) const
{
    NSPoint screenPoint = mapToNative(point);

    // Search (hit test) for the top-level window. [NSWidow windowNumberAtPoint:
    // belowWindowWithWindowNumber] may return windows that are not interesting
    // to Qt. The search iterates until a suitable window or no window is found.
    NSInteger topWindowNumber = 0;
    QWindow *window = 0;
    do {
        // Get the top-most window, below any previously rejected window.
        topWindowNumber = [NSWindow windowNumberAtPoint:screenPoint
                                    belowWindowWithWindowNumber:topWindowNumber];

        // Continue the search if the window does not belong to this process.
        NSWindow *nsWindow = [NSApp windowWithWindowNumber:topWindowNumber];
        if (nsWindow == 0)
            continue;

        // Continue the search if the window does not belong to Qt.
        if (![nsWindow conformsToProtocol:@protocol(QNSWindowProtocol)])
            continue;

        id<QNSWindowProtocol> proto = static_cast<id<QNSWindowProtocol> >(nsWindow);
        QCocoaWindow *cocoaWindow = proto.platformWindow;
        if (!cocoaWindow)
            continue;
        window = cocoaWindow->window();

        // Continue the search if the window is not a top-level window.
        if (!window->isTopLevel())
             continue;

        // Stop searching. The current window is the correct window.
        break;
    } while (topWindowNumber > 0);

    return window;
}

QPixmap QCocoaScreen::grabWindow(WId window, int x, int y, int width, int height) const
{
    // TODO window should be handled
    Q_UNUSED(window)

    const int maxDisplays = 128; // 128 displays should be enough for everyone.
    CGDirectDisplayID displays[maxDisplays];
    CGDisplayCount displayCount;
    CGRect cgRect;

    if (width < 0 || height < 0) {
        // get all displays
        cgRect = CGRectInfinite;
    } else {
        cgRect = CGRectMake(x, y, width, height);
    }
    const CGDisplayErr err = CGGetDisplaysWithRect(cgRect, maxDisplays, displays, &displayCount);

    if (err && displayCount == 0)
        return QPixmap();

    // calculate pixmap size
    QSize windowSize(width, height);
    if (width < 0 || height < 0) {
        QRect windowRect;
        for (uint i = 0; i < displayCount; ++i) {
            const CGRect cgRect = CGDisplayBounds(displays[i]);
            QRect qRect(cgRect.origin.x, cgRect.origin.y, cgRect.size.width, cgRect.size.height);
            windowRect = windowRect.united(qRect);
        }
        if (width < 0)
            windowSize.setWidth(windowRect.width());
        if (height < 0)
            windowSize.setHeight(windowRect.height());
    }

    const qreal dpr = devicePixelRatio();
    QPixmap windowPixmap(windowSize * dpr);
    windowPixmap.fill(Qt::transparent);

    for (uint i = 0; i < displayCount; ++i) {
        const CGRect bounds = CGDisplayBounds(displays[i]);

        // Calculate the position and size of the requested area
        QPoint pos(qAbs(bounds.origin.x - x), qAbs(bounds.origin.y - y));
        QSize size(qMin(pos.x() + width, qRound(bounds.size.width)),
                   qMin(pos.y() + height, qRound(bounds.size.height)));
        pos *= dpr;
        size *= dpr;

        // Take the whole screen and crop it afterwards, because CGDisplayCreateImageForRect
        // has a strange behavior when mixing highDPI and non-highDPI displays
        QCFType<CGImageRef> cgImage = CGDisplayCreateImage(displays[i]);
        const QImage image = qt_mac_toQImage(cgImage);

        // Draw into windowPixmap only the requested size
        QPainter painter(&windowPixmap);
        painter.drawImage(windowPixmap.rect(), image, QRect(pos, size));
    }
    return windowPixmap;
}

/*!
    The screen used as a reference for global window geometry
*/
QCocoaScreen *QCocoaScreen::primaryScreen()
{
    return static_cast<QCocoaScreen *>(QGuiApplication::primaryScreen()->handle());
}

CGPoint QCocoaScreen::mapToNative(const QPointF &pos, QCocoaScreen *screen)
{
    Q_ASSERT(screen);
    return qt_mac_flip(pos, screen->geometry()).toCGPoint();
}

CGRect QCocoaScreen::mapToNative(const QRectF &rect, QCocoaScreen *screen)
{
    Q_ASSERT(screen);
    return qt_mac_flip(rect, screen->geometry()).toCGRect();
}

QPointF QCocoaScreen::mapFromNative(CGPoint pos, QCocoaScreen *screen)
{
    Q_ASSERT(screen);
    return qt_mac_flip(QPointF::fromCGPoint(pos), screen->geometry());
}

QRectF QCocoaScreen::mapFromNative(CGRect rect, QCocoaScreen *screen)
{
    Q_ASSERT(screen);
    return qt_mac_flip(QRectF::fromCGRect(rect), screen->geometry());
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QCocoaScreen *screen)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QCocoaScreen(" << (const void *)screen;
    if (screen) {
        debug << ", index=" << screen->m_screenIndex;
        debug << ", native=" << screen->nativeScreen();
        debug << ", geometry=" << screen->geometry();
        debug << ", dpr=" << screen->devicePixelRatio();
        debug << ", name=" << screen->name();
    }
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
