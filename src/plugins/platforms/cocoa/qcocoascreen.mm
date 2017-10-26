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

/*!
    Flips the Y coordinate of the point between quadrant I and IV.

    The native coordinate system on macOS uses quadrant I, with origin
    in bottom left, and Qt uses quadrant IV, with origin in top left.

    By flippig the Y coordinate, we can map the position between the
    two coordinate systems.
*/
QPointF QCocoaScreen::flipCoordinate(const QPointF &pos) const
{
    return QPointF(pos.x(), m_geometry.height() - pos.y());
}

/*!
    Flips the Y coordinate of the rectangle between quadrant I and IV.

    The native coordinate system on macOS uses quadrant I, with origin
    in bottom left, and Qt uses quadrant IV, with origin in top left.

    By flippig the Y coordinate, we can map the rectangle between the
    two coordinate systems.
*/
QRectF QCocoaScreen::flipCoordinate(const QRectF &rect) const
{
    return QRectF(flipCoordinate(rect.topLeft() + QPoint(0, rect.height())), rect.size());
}

void QCocoaScreen::updateGeometry()
{
    NSScreen *nsScreen = nativeScreen();
    if (!nsScreen)
        return;

    // At this point the geometry is in native coordinates, but the size
    // is correct, which we take advantage of next when we map the native
    // coordinates to the Qt coordinate system.
    m_geometry = QRectF::fromCGRect(NSRectToCGRect(nsScreen.frame)).toRect();
    m_availableGeometry = QRectF::fromCGRect(NSRectToCGRect(nsScreen.visibleFrame)).toRect();

    // The reference screen for the geometry is always the primary screen, but since
    // we may be in the process of creating and registering the primary screen, we
    // must special-case that and assign it direcly.
    QCocoaScreen *primaryScreen = (nsScreen == [[NSScreen screens] firstObject]) ?
        this : QCocoaScreen::primaryScreen();

    m_geometry = primaryScreen->mapFromNative(m_geometry).toRect();
    m_availableGeometry = primaryScreen->mapFromNative(m_availableGeometry).toRect();

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

    // Get m_name (brand/model of the monitor)
    NSDictionary *deviceInfo = (NSDictionary *)IODisplayCreateInfoDictionary(CGDisplayIOServicePort(dpy), kIODisplayOnlyPreferredName);
    NSDictionary *localizedNames = [deviceInfo objectForKey:[NSString stringWithUTF8String:kDisplayProductName]];
    if ([localizedNames count] > 0)
        m_name = QString::fromUtf8([[localizedNames objectForKey:[[localizedNames allKeys] objectAtIndex:0]] UTF8String]);
    [deviceInfo release];

    QWindowSystemInterface::handleScreenGeometryChange(screen(), geometry(), availableGeometry());
    QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(screen(), m_logicalDpi.first, m_logicalDpi.second);
    QWindowSystemInterface::handleScreenRefreshRateChange(screen(), m_refreshRate);
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
    NSPoint screenPoint = qt_mac_flipPoint(point);

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

    QPixmap windowPixmap(windowSize * devicePixelRatio());
    windowPixmap.fill(Qt::transparent);

    for (uint i = 0; i < displayCount; ++i) {
        const CGRect bounds = CGDisplayBounds(displays[i]);
        int w = (width < 0 ? bounds.size.width : width) * devicePixelRatio();
        int h = (height < 0 ? bounds.size.height : height) * devicePixelRatio();
        QRect displayRect = QRect(x, y, w, h);
        displayRect = displayRect.translated(qRound(-bounds.origin.x), qRound(-bounds.origin.y));
        QCFType<CGImageRef> image = CGDisplayCreateImageForRect(displays[i],
            CGRectMake(displayRect.x(), displayRect.y(), displayRect.width(), displayRect.height()));
        QPixmap pix(w, h);
        pix.fill(Qt::transparent);
        CGRect rect = CGRectMake(0, 0, w, h);
        QMacCGContext ctx(&pix);
        qt_mac_drawCGImage(ctx, &rect, image);

        QPainter painter(&windowPixmap);
        painter.drawPixmap(0, 0, pix);
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
