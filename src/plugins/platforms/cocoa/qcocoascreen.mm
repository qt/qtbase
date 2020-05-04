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
#include "qcocoaintegration.h"

#include <QtCore/qcoreapplication.h>
#include <QtGui/private/qcoregraphics_p.h>

#include <IOKit/graphics/IOGraphicsLib.h>

#include <QtGui/private/qwindow_p.h>

#include <QtCore/private/qeventdispatcher_cf_p.h>

QT_BEGIN_NAMESPACE

namespace CoreGraphics {
    Q_NAMESPACE
    enum DisplayChange {
        ReconfiguredWithFlagsMissing = 0,
        Moved = kCGDisplayMovedFlag,
        SetMain = kCGDisplaySetMainFlag,
        SetMode = kCGDisplaySetModeFlag,
        Added = kCGDisplayAddFlag,
        Removed = kCGDisplayRemoveFlag,
        Enabled = kCGDisplayEnabledFlag,
        Disabled = kCGDisplayDisabledFlag,
        Mirrored = kCGDisplayMirrorFlag,
        UnMirrored = kCGDisplayUnMirrorFlag,
        DesktopShapeChanged = kCGDisplayDesktopShapeChangedFlag
    };
    Q_ENUM_NS(DisplayChange)
}

NSArray *QCocoaScreen::s_screenConfigurationBeforeUpdate = nil;

void QCocoaScreen::initializeScreens()
{
    updateScreens();

    CGDisplayRegisterReconfigurationCallback([](CGDirectDisplayID displayId, CGDisplayChangeSummaryFlags flags, void *userInfo) {
        Q_UNUSED(userInfo);

        // Displays are reconfigured in batches, and we want to update our screens
        // once a batch ends, so that all the states of the displays are up to date.
        static int displayReconfigurationsInProgress = 0;

        const bool beforeReconfigure = flags & kCGDisplayBeginConfigurationFlag;
        qCDebug(lcQpaScreen).verbosity(0).nospace() << "Display " << displayId
                << (beforeReconfigure ? " about to reconfigure" : " was ")
                << QFlags<CoreGraphics::DisplayChange>(flags)
                << " with " << displayReconfigurationsInProgress
                << " display configuration(s) in progress";

        if (!flags) {
            // CGDisplayRegisterReconfigurationCallback has been observed to be called
            // with flags unset. This seems like a bug. The callback is not paired with
            // a matching "completion" callback either, so we don't know whether to treat
            // it as a begin or end of reconfigure.
            return;
        }

        if (beforeReconfigure) {
            if (!displayReconfigurationsInProgress++) {
                // There might have been a screen reconfigure before this that
                // we didn't process yet, so do that now if that's the case.
                updateScreensIfNeeded();

                Q_ASSERT(!s_screenConfigurationBeforeUpdate);
                s_screenConfigurationBeforeUpdate = NSScreen.screens;
                qCDebug(lcQpaScreen, "Display reconfigure transaction started"
                    " with screen configuration %p", s_screenConfigurationBeforeUpdate);

                static void (^tryScreenUpdate)();
                tryScreenUpdate = ^void () {
                    qCDebug(lcQpaScreen) << "Attempting screen update from runloop block";
                    if (!updateScreensIfNeeded())
                        CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, tryScreenUpdate);
                };
                CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, tryScreenUpdate);
            }
        } else {
            Q_ASSERT_X(displayReconfigurationsInProgress, "QCococaScreen",
                "Display configuration transactions are expected to be balanced");

            if (!--displayReconfigurationsInProgress) {
                qCDebug(lcQpaScreen) << "Display reconfigure transaction completed";
                // We optimistically update now, in case the NSScreens have changed
                updateScreensIfNeeded();
            }
        }
    }, nullptr);

    static QMacNotificationObserver screenParameterObserver(NSApplication.sharedApplication,
        NSApplicationDidChangeScreenParametersNotification, [&]() {
            qCDebug(lcQpaScreen) << "Received screen parameter change notification";
            updateScreensIfNeeded(); // As a last resort we update screens here
        });
}

bool QCocoaScreen::updateScreensIfNeeded()
{
    if (!s_screenConfigurationBeforeUpdate) {
        qCDebug(lcQpaScreen) << "QScreens have already been updated, all good";
        return true;
    }

    if (s_screenConfigurationBeforeUpdate == NSScreen.screens) {
        qCDebug(lcQpaScreen) << "Still waiting for NSScreen configuration change";
        return false;
    }

    qCDebug(lcQpaScreen, "NSScreen configuration changed to %p", NSScreen.screens);
    updateScreens();

    s_screenConfigurationBeforeUpdate = nil;
    return true;
}

/*
    Update the list of available QScreens, and the properties of existing screens.

    At this point we rely on the NSScreen.screens to be up to date.
*/
void QCocoaScreen::updateScreens()
{
    uint32_t displayCount = 0;
    if (CGGetOnlineDisplayList(0, nullptr, &displayCount) != kCGErrorSuccess)
        qFatal("Failed to get number of online displays");

    QVector<CGDirectDisplayID> onlineDisplays(displayCount);
    if (CGGetOnlineDisplayList(displayCount, onlineDisplays.data(), &displayCount) != kCGErrorSuccess)
        qFatal("Failed to get online displays");

    qCInfo(lcQpaScreen) << "Updating screens with" << displayCount
        << "online displays:" << onlineDisplays;

    // TODO: Verify whether we can always assume the main display is first
    int mainDisplayIndex = onlineDisplays.indexOf(CGMainDisplayID());
    if (mainDisplayIndex < 0) {
        qCWarning(lcQpaScreen) << "Main display not in list of online displays!";
    } else if (mainDisplayIndex > 0) {
        qCWarning(lcQpaScreen) << "Main display not first display, making sure it is";
        onlineDisplays.move(mainDisplayIndex, 0);
    }

    for (CGDirectDisplayID displayId : onlineDisplays) {
        Q_ASSERT(CGDisplayIsOnline(displayId));

        if (CGDisplayMirrorsDisplay(displayId))
            continue;

        // A single physical screen can map to multiple displays IDs,
        // depending on which GPU is in use or which physical port the
        // screen is connected to. By mapping the display ID to a UUID,
        // which are shared between displays that target the same screen,
        // we can pick an existing QScreen to update instead of needlessly
        // adding and removing QScreens.
        QCFType<CFUUIDRef> uuid = CGDisplayCreateUUIDFromDisplayID(displayId);
        Q_ASSERT(uuid);

        if (QCocoaScreen *existingScreen = QCocoaScreen::get(uuid)) {
            existingScreen->update(displayId);
            qCInfo(lcQpaScreen) << "Updated" << existingScreen;
            if (CGDisplayIsMain(displayId) && existingScreen != qGuiApp->primaryScreen()->handle()) {
                qCInfo(lcQpaScreen) << "Primary screen changed to" << existingScreen;
                QWindowSystemInterface::handlePrimaryScreenChanged(existingScreen);
            }
        } else {
            QCocoaScreen::add(displayId);
        }
    }

    for (QScreen *screen : QGuiApplication::screens()) {
        QCocoaScreen *platformScreen = static_cast<QCocoaScreen*>(screen->handle());
        if (!platformScreen->isOnline() || platformScreen->isMirroring())
            platformScreen->remove();
    }
}

void QCocoaScreen::add(CGDirectDisplayID displayId)
{
    const bool isPrimary = CGDisplayIsMain(displayId);
    QCocoaScreen *cocoaScreen = new QCocoaScreen(displayId);
    qCInfo(lcQpaScreen) << "Adding" << cocoaScreen
        << (isPrimary ? "as new primary screen" : "");
    QWindowSystemInterface::handleScreenAdded(cocoaScreen, isPrimary);
}

QCocoaScreen::QCocoaScreen(CGDirectDisplayID displayId)
    : QPlatformScreen(), m_displayId(displayId)
{
    update(m_displayId);
    m_cursor = new QCocoaCursor;
}

void QCocoaScreen::cleanupScreens()
{
    // Remove screens in reverse order to avoid crash in case of multiple screens
    for (QScreen *screen : backwards(QGuiApplication::screens()))
        static_cast<QCocoaScreen*>(screen->handle())->remove();
}

void QCocoaScreen::remove()
{
    // This may result in the application responding to QGuiApplication::screenRemoved
    // by moving the window to another screen, either by setGeometry, or by setScreen.
    // If the window isn't moved by the application, Qt will as a fallback move it to
    // the primary screen via setScreen. Due to the way setScreen works, this won't
    // actually recreate the window on the new screen, it will just assign the new
    // QScreen to the window. The associated NSWindow will have an NSScreen determined
    // by AppKit. AppKit will then move the window to another screen by changing the
    // geometry, and we will get a callback in QCocoaWindow::windowDidMove and then
    // QCocoaWindow::windowDidChangeScreen. At that point the window will appear to have
    // already changed its screen, but that's only true if comparing the Qt screens,
    // not when comparing the NSScreens.
    qCInfo(lcQpaScreen) << "Removing " << this;
    QWindowSystemInterface::handleScreenRemoved(this);
}

QCocoaScreen::~QCocoaScreen()
{
    Q_ASSERT_X(!screen(), "QCocoaScreen", "QScreen should be deleted first");

    delete m_cursor;

    CVDisplayLinkRelease(m_displayLink);
    if (m_displayLinkSource)
         dispatch_release(m_displayLinkSource);
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

void QCocoaScreen::update(CGDirectDisplayID displayId)
{
    if (displayId != m_displayId) {
        qCDebug(lcQpaScreen) << "Reconnecting" << this << "as display" << displayId;
        m_displayId = displayId;
    }

    Q_ASSERT(isOnline());

    const QRect previousGeometry = m_geometry;
    const QRect previousAvailableGeometry = m_availableGeometry;
    const QDpi previousLogicalDpi = m_logicalDpi;
    const qreal previousRefreshRate = m_refreshRate;

    // Some properties are only available via NSScreen
    NSScreen *nsScreen = nativeScreen();
    Q_ASSERT(nsScreen);

    // The reference screen for the geometry is always the primary screen
    QRectF primaryScreenGeometry = QRectF::fromCGRect(CGDisplayBounds(CGMainDisplayID()));
    m_geometry = qt_mac_flip(QRectF::fromCGRect(nsScreen.frame), primaryScreenGeometry).toRect();
    m_availableGeometry = qt_mac_flip(QRectF::fromCGRect(nsScreen.visibleFrame), primaryScreenGeometry).toRect();

    m_devicePixelRatio = nsScreen.backingScaleFactor;

    m_format = QImage::Format_RGB32;
    m_depth = NSBitsPerPixelFromDepth(nsScreen.depth);

    CGSize size = CGDisplayScreenSize(m_displayId);
    m_physicalSize = QSizeF(size.width, size.height);
    m_logicalDpi.first = 72;
    m_logicalDpi.second = 72;

    QCFType<CGDisplayModeRef> displayMode = CGDisplayCopyDisplayMode(m_displayId);
    float refresh = CGDisplayModeGetRefreshRate(displayMode);
    m_refreshRate = refresh > 0 ? refresh : 60.0;

    m_name = displayName(m_displayId);

    const bool didChangeGeometry = m_geometry != previousGeometry || m_availableGeometry != previousAvailableGeometry;

    if (didChangeGeometry)
        QWindowSystemInterface::handleScreenGeometryChange(screen(), geometry(), availableGeometry());
    if (m_logicalDpi != previousLogicalDpi)
        QWindowSystemInterface::handleScreenLogicalDotsPerInchChange(screen(), m_logicalDpi.first, m_logicalDpi.second);
    if (m_refreshRate != previousRefreshRate)
        QWindowSystemInterface::handleScreenRefreshRateChange(screen(), m_refreshRate);
}

// ----------------------- Display link -----------------------

Q_LOGGING_CATEGORY(lcQpaScreenUpdates, "qt.qpa.screen.updates", QtCriticalMsg);

void QCocoaScreen::requestUpdate()
{
    Q_ASSERT(m_displayId);

    if (!m_displayLink) {
        CVDisplayLinkCreateWithCGDisplay(m_displayId, &m_displayLink);
        CVDisplayLinkSetOutputCallback(m_displayLink, [](CVDisplayLinkRef, const CVTimeStamp*,
            const CVTimeStamp*, CVOptionFlags, CVOptionFlags*, void* displayLinkContext) -> int {
                // FIXME: It would be nice if update requests would include timing info
                static_cast<QCocoaScreen*>(displayLinkContext)->deliverUpdateRequests();
                return kCVReturnSuccess;
        }, this);
        qCDebug(lcQpaScreenUpdates) << "Display link created for" << this;

        // During live window resizing -[NSWindow _resizeWithEvent:] will spin a local event loop
        // in event-tracking mode, dequeuing only the mouse drag events needed to update the window's
        // frame. It will repeatedly spin this loop until no longer receiving any mouse drag events,
        // and will then update the frame (effectively coalescing/compressing the events). Unfortunately
        // the events are pulled out using -[NSApplication nextEventMatchingEventMask:untilDate:inMode:dequeue:]
        // which internally uses CFRunLoopRunSpecific, so the event loop will also process GCD queues and other
        // runloop sources that have been added to the tracking mode. This includes the GCD display-link
        // source that we use to marshal the display-link callback over to the main thread. If the
        // subsequent delivery of the update-request on the main thread stalls due to inefficient
        // user code, the NSEventThread will have had time to deliver additional mouse drag events,
        // and the logic in -[NSWindow _resizeWithEvent:] will keep on compressing events and never
        // get to the point of actually updating the window frame, making it seem like the window
        // is stuck in its original size. Only when the user stops moving their mouse, and the event
        // queue is completely drained of drag events, will the window frame be updated.

        // By keeping an event tap listening for drag events, registered as a version 1 runloop source,
        // we prevent the GCD source from being prioritized, giving the resize logic enough time
        // to finish coalescing the events. This is incidental, but conveniently gives us the behavior
        // we are looking for, interleaving display-link updates and resize events.
        static CFMachPortRef eventTap = []() {
            CFMachPortRef eventTap = CGEventTapCreateForPid(getpid(), kCGTailAppendEventTap,
                kCGEventTapOptionListenOnly, NSEventMaskLeftMouseDragged,
                [](CGEventTapProxy, CGEventType type, CGEventRef event, void *) -> CGEventRef {
                    if (type == kCGEventTapDisabledByTimeout)
                        qCWarning(lcQpaScreenUpdates) << "Event tap disabled due to timeout!";
                    return event; // Listen only tap, so what we return doesn't really matter
                }, nullptr);
            CGEventTapEnable(eventTap, false); // Event taps are normally enabled when created
            static CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
            CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopCommonModes);

            NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
            [center addObserverForName:NSWindowWillStartLiveResizeNotification object:nil queue:nil
                usingBlock:^(NSNotification *notification) {
                    qCDebug(lcQpaScreenUpdates) << "Live resize of" << notification.object
                        << "started. Enabling event tap";
                    CGEventTapEnable(eventTap, true);
                }];
            [center addObserverForName:NSWindowDidEndLiveResizeNotification object:nil queue:nil
                usingBlock:^(NSNotification *notification) {
                    qCDebug(lcQpaScreenUpdates) << "Live resize of" << notification.object
                        << "ended. Disabling event tap";
                    CGEventTapEnable(eventTap, false);
                }];
            return eventTap;
        }();
        Q_UNUSED(eventTap);
    }

    if (!CVDisplayLinkIsRunning(m_displayLink)) {
        qCDebug(lcQpaScreenUpdates) << "Starting display link for" << this;
        CVDisplayLinkStart(m_displayLink);
    }
}

// Helper to allow building up debug output in multiple steps
struct DeferredDebugHelper
{
    DeferredDebugHelper(const QLoggingCategory &cat) {
        if (cat.isDebugEnabled())
            debug = new QDebug(QMessageLogger().debug(cat).nospace());
    }
    ~DeferredDebugHelper() {
        flushOutput();
    }
    void flushOutput() {
        if (debug) {
            delete debug;
            debug = nullptr;
        }
    }
    QDebug *debug = nullptr;
};

#define qDeferredDebug(helper) if (Q_UNLIKELY(helper.debug)) *helper.debug

void QCocoaScreen::deliverUpdateRequests()
{
    if (!isOnline())
        return;

    QMacAutoReleasePool pool;

    // The CVDisplayLink callback is a notification that it's a good time to produce a new frame.
    // Since the callback is delivered on a separate thread we have to marshal it over to the
    // main thread, as Qt requires update requests to be delivered there. This needs to happen
    // asynchronously, as otherwise we may end up deadlocking if the main thread calls back
    // into any of the CVDisplayLink APIs.
    if (!NSThread.isMainThread) {
        // We're explicitly not using the data of the GCD source to track the pending updates,
        // as the data isn't reset to 0 until after the event handler, and also doesn't update
        // during the event handler, both of which we need to track late frames.
        const int pendingUpdates = ++m_pendingUpdates;

        DeferredDebugHelper screenUpdates(lcQpaScreenUpdates());
        qDeferredDebug(screenUpdates) << "display link callback for screen " << m_displayId;

        if (const int framesAheadOfDelivery = pendingUpdates - 1) {
            // If we have more than one update pending it means that a previous display link callback
            // has not been fully processed on the main thread, either because GCD hasn't delivered
            // it on the main thread yet, because the processing of the update request is taking
            // too long, or because the update request was deferred due to window live resizing.
            qDeferredDebug(screenUpdates) << ", " << framesAheadOfDelivery << " frame(s) ahead";
        }

        qDeferredDebug(screenUpdates) << "; signaling dispatch source";

        if (!m_displayLinkSource) {
            m_displayLinkSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_DATA_ADD, 0, 0, dispatch_get_main_queue());
            dispatch_source_set_event_handler(m_displayLinkSource, ^{
                deliverUpdateRequests();
            });
            dispatch_resume(m_displayLinkSource);
        }

        dispatch_source_merge_data(m_displayLinkSource, 1);

    } else {
        DeferredDebugHelper screenUpdates(lcQpaScreenUpdates());
        qDeferredDebug(screenUpdates) << "gcd event handler on main thread";

        const int pendingUpdates = m_pendingUpdates;
        if (pendingUpdates > 1)
            qDeferredDebug(screenUpdates) << ", " << (pendingUpdates - 1) << " frame(s) behind display link";

        screenUpdates.flushOutput();

        bool pauseUpdates = true;

        auto windows = QGuiApplication::allWindows();
        for (int i = 0; i < windows.size(); ++i) {
            QWindow *window = windows.at(i);
            auto *platformWindow = static_cast<QCocoaWindow*>(window->handle());
            if (!platformWindow)
                continue;

            if (!platformWindow->hasPendingUpdateRequest())
                continue;

            if (window->screen() != screen())
                continue;

            // Skip windows that are not doing update requests via display link
            if (!platformWindow->updatesWithDisplayLink())
                continue;

            platformWindow->deliverUpdateRequest();

            // Another update request was triggered, keep the display link running
            if (platformWindow->hasPendingUpdateRequest())
                pauseUpdates = false;
        }

        if (pauseUpdates) {
            // Pause the display link if there are no pending update requests
            qCDebug(lcQpaScreenUpdates) << "Stopping display link for" << this;
            CVDisplayLinkStop(m_displayLink);
        }

        if (const int missedUpdates = m_pendingUpdates.fetchAndStoreRelaxed(0) - pendingUpdates) {
            qCWarning(lcQpaScreenUpdates) << "main thread missed" << missedUpdates
                << "update(s) from display link during update request delivery";
        }
    }
}

bool QCocoaScreen::isRunningDisplayLink() const
{
    return m_displayLink && CVDisplayLinkIsRunning(m_displayLink);
}

// -----------------------------------------------------------

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
    QWindow *window = nullptr;
    do {
        // Get the top-most window, below any previously rejected window.
        topWindowNumber = [NSWindow windowNumberAtPoint:screenPoint
                                    belowWindowWithWindowNumber:topWindowNumber];

        // Continue the search if the window does not belong to this process.
        NSWindow *nsWindow = [NSApp windowWithWindowNumber:topWindowNumber];
        if (!nsWindow)
            continue;

        // Continue the search if the window does not belong to Qt.
        if (![nsWindow conformsToProtocol:@protocol(QNSWindowProtocol)])
            continue;

        QCocoaWindow *cocoaWindow = qnsview_cast(nsWindow.contentView).platformWindow;
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

QPixmap QCocoaScreen::grabWindow(WId view, int x, int y, int width, int height) const
{
    // Determine the grab rect. FIXME: The rect should be bounded by the view's
    // geometry, but note that for the pixeltool use case that window will be the
    // desktop widget's view, which currently gets resized to fit one screen
    // only, since its NSWindow has the NSWindowStyleMaskTitled flag set.
    Q_UNUSED(view);
    QRect grabRect = QRect(x, y, width, height);
    qCDebug(lcQpaScreen) << "input grab rect" << grabRect;

    // Find which displays to grab from, or all of them if the grab size is unspecified
    const int maxDisplays = 128;
    CGDirectDisplayID displays[maxDisplays];
    CGDisplayCount displayCount;
    CGRect cgRect = (width < 0 || height < 0) ? CGRectInfinite : grabRect.toCGRect();
    const CGDisplayErr err = CGGetDisplaysWithRect(cgRect, maxDisplays, displays, &displayCount);
    if (err || displayCount == 0)
        return QPixmap();

    // If the grab size is not specified, set it to be the bounding box of all screens,
    if (width < 0 || height < 0) {
        QRect windowRect;
        for (uint i = 0; i < displayCount; ++i) {
            QRect displayBounds = QRectF::fromCGRect(CGDisplayBounds(displays[i])).toRect();
            // Only include the screen if it is positioned past the x/y position
            if ((displayBounds.x() >= x || displayBounds.right() > x) &&
                (displayBounds.y() >= y || displayBounds.bottom() > y)) {
                windowRect = windowRect.united(displayBounds);
            }
        }
        if (grabRect.width() < 0)
            grabRect.setWidth(windowRect.width());
        if (grabRect.height() < 0)
            grabRect.setHeight(windowRect.height());
    }

    qCDebug(lcQpaScreen) << "final grab rect" << grabRect << "from" << displayCount << "displays";

    // Grab images from each display
    QVector<QImage> images;
    QVector<QRect> destinations;
    for (uint i = 0; i < displayCount; ++i) {
        auto display = displays[i];
        QRect displayBounds = QRectF::fromCGRect(CGDisplayBounds(display)).toRect();
        QRect grabBounds = displayBounds.intersected(grabRect);
        if (grabBounds.isNull()) {
            destinations.append(QRect());
            images.append(QImage());
            continue;
        }
        QRect displayLocalGrabBounds = QRect(QPoint(grabBounds.topLeft() - displayBounds.topLeft()), grabBounds.size());
        QImage displayImage = qt_mac_toQImage(QCFType<CGImageRef>(CGDisplayCreateImageForRect(display, displayLocalGrabBounds.toCGRect())));
        displayImage.setDevicePixelRatio(displayImage.size().width() / displayLocalGrabBounds.size().width());
        images.append(displayImage);
        QRect destBounds = QRect(QPoint(grabBounds.topLeft() - grabRect.topLeft()), grabBounds.size());
        destinations.append(destBounds);
        qCDebug(lcQpaScreen) << "grab display" << i << "global" << grabBounds << "local" << displayLocalGrabBounds
                             << "grab image size" << displayImage.size() << "devicePixelRatio" << displayImage.devicePixelRatio();
    }

    // Determine the highest dpr, which becomes the dpr for the returned pixmap.
    qreal dpr = 1.0;
    for (uint i = 0; i < displayCount; ++i)
        dpr = qMax(dpr, images.at(i).devicePixelRatio());

    // Allocate target pixmap and draw each screen's content
    qCDebug(lcQpaScreen) << "Create grap pixmap" << grabRect.size() << "at devicePixelRatio" << dpr;
    QPixmap windowPixmap(grabRect.size() * dpr);
    windowPixmap.setDevicePixelRatio(dpr);
    windowPixmap.fill(Qt::transparent);
    QPainter painter(&windowPixmap);
    for (uint i = 0; i < displayCount; ++i)
        painter.drawImage(destinations.at(i), images.at(i));

    return windowPixmap;
}

bool QCocoaScreen::isOnline() const
{
    // When a display is disconnected CGDisplayIsOnline and other CGDisplay
    // functions that take a displayId will not return false, but will start
    // returning -1 to signal that the displayId is invalid. Some functions
    // will also assert or even crash in this case, so it's important that
    // we double check if a display is online before calling other functions.
    auto isOnline = CGDisplayIsOnline(m_displayId);
    static const uint32_t kCGDisplayIsDisconnected = int32_t(-1);
    return isOnline != kCGDisplayIsDisconnected && isOnline;
}

/*
    Returns true if a screen is mirroring another screen
*/
bool QCocoaScreen::isMirroring() const
{
    if (!isOnline())
        return false;

    return CGDisplayMirrorsDisplay(m_displayId);
}

/*!
    The screen used as a reference for global window geometry
*/
QCocoaScreen *QCocoaScreen::primaryScreen()
{
    // Note: The primary screen that Qt knows about may not match the current CGMainDisplayID()
    // if macOS has not yet been able to inform us that the main display has changed, but we
    // will update the primary screen accordingly once the reconfiguration callback comes in.
    return static_cast<QCocoaScreen *>(QGuiApplication::primaryScreen()->handle());
}

QList<QPlatformScreen*> QCocoaScreen::virtualSiblings() const
{
    QList<QPlatformScreen*> siblings;

    // Screens on macOS are always part of the same virtual desktop
    for (QScreen *screen : QGuiApplication::screens())
        siblings << screen->handle();

    return siblings;
}

QCocoaScreen *QCocoaScreen::get(NSScreen *nsScreen)
{
    if (s_screenConfigurationBeforeUpdate) {
        qCWarning(lcQpaScreen) << "Trying to resolve screen while waiting for screen reconfigure!";
        if (!updateScreensIfNeeded())
            qCWarning(lcQpaScreen) << "Failed to do last minute screen update. Expect crashes.";
    }

    return get(nsScreen.qt_displayId);
}

QCocoaScreen *QCocoaScreen::get(CGDirectDisplayID displayId)
{
    for (QScreen *screen : QGuiApplication::screens()) {
        QCocoaScreen *cocoaScreen = static_cast<QCocoaScreen*>(screen->handle());
        if (cocoaScreen->m_displayId == displayId)
            return cocoaScreen;
    }

    return nullptr;
}

QCocoaScreen *QCocoaScreen::get(CFUUIDRef uuid)
{
    for (QScreen *screen : QGuiApplication::screens()) {
        auto *platformScreen = static_cast<QCocoaScreen*>(screen->handle());
        if (!platformScreen->isOnline())
            continue;

        auto displayId = platformScreen->displayId();
        QCFType<CFUUIDRef> candidateUuid(CGDisplayCreateUUIDFromDisplayID(displayId));
        Q_ASSERT(candidateUuid);

        if (candidateUuid == uuid)
            return platformScreen;
    }

    return nullptr;
}

NSScreen *QCocoaScreen::nativeScreen() const
{
    if (!m_displayId)
        return nil; // The display has been disconnected

    for (NSScreen *screen in NSScreen.screens) {
        if (screen.qt_displayId == m_displayId)
            return screen;
    }

    return nil;
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
        debug << ", " << screen->name();
        if (screen->isOnline()) {
            if (CGDisplayIsAsleep(screen->displayId()))
                debug << ", Sleeping";
            if (auto mirroring = CGDisplayMirrorsDisplay(screen->displayId()))
                debug << ", mirroring=" << mirroring;
        } else {
            debug << ", Offline";
        }
        debug << ", " << screen->geometry();
        debug << ", dpr=" << screen->devicePixelRatio();
        debug << ", displayId=" << screen->displayId();

        if (auto nativeScreen = screen->nativeScreen())
            debug << ", " << nativeScreen;
    }
    debug << ')';
    return debug;
}
#endif // !QT_NO_DEBUG_STREAM

#include "qcocoascreen.moc"

QT_END_NAMESPACE

@implementation NSScreen (QtExtras)

- (CGDirectDisplayID)qt_displayId
{
    return [self.deviceDescription[@"NSScreenNumber"] unsignedIntValue];
}

@end
