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

#include "qiosglobal.h"
#include "qiosintegration.h"
#include "qiosscreen.h"
#include "qioswindow.h"
#include <qpa/qwindowsysteminterface.h>
#include "qiosapplicationdelegate.h"
#include "qiosviewcontroller.h"
#include "quiview.h"

#include <QtCore/private/qcore_mac_p.h>

#include <QtGui/private/qwindow_p.h>
#include <private/qcoregraphics_p.h>

#include <sys/sysctl.h>

// -------------------------------------------------------------------------

typedef void (^DisplayLinkBlock)(CADisplayLink *displayLink);

@implementation UIScreen (DisplayLinkBlock)
- (CADisplayLink*)displayLinkWithBlock:(DisplayLinkBlock)block
{
    return [self displayLinkWithTarget:[[block copy] autorelease]
        selector:@selector(invokeDisplayLinkBlock:)];
}
@end

@implementation NSObject (DisplayLinkBlock)
- (void)invokeDisplayLinkBlock:(CADisplayLink *)sender
{
    DisplayLinkBlock block = static_cast<id>(self);
    block(sender);
}
@end


// -------------------------------------------------------------------------

static QIOSScreen* qtPlatformScreenFor(UIScreen *uiScreen)
{
    foreach (QScreen *screen, QGuiApplication::screens()) {
        QIOSScreen *platformScreen = static_cast<QIOSScreen *>(screen->handle());
        if (platformScreen->uiScreen() == uiScreen)
            return platformScreen;
    }

    return 0;
}

@interface QIOSScreenTracker : NSObject
@end

@implementation QIOSScreenTracker

+ (void)load
{
    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
    [center addObserver:self selector:@selector(screenConnected:)
            name:UIScreenDidConnectNotification object:nil];
    [center addObserver:self selector:@selector(screenDisconnected:)
            name:UIScreenDidDisconnectNotification object:nil];
    [center addObserver:self selector:@selector(screenModeChanged:)
            name:UIScreenModeDidChangeNotification object:nil];
}

+ (void)screenConnected:(NSNotification*)notification
{
    QIOSIntegration *integration = QIOSIntegration::instance();
    Q_ASSERT_X(integration, Q_FUNC_INFO, "Screen connected before QIOSIntegration creation");

    integration->addScreen(new QIOSScreen([notification object]));
}

+ (void)screenDisconnected:(NSNotification*)notification
{
    QIOSScreen *screen = qtPlatformScreenFor([notification object]);
    Q_ASSERT_X(screen, Q_FUNC_INFO, "Screen disconnected that we didn't know about");

    QIOSIntegration *integration = QIOSIntegration::instance();
    integration->destroyScreen(screen);
}

+ (void)screenModeChanged:(NSNotification*)notification
{
    QIOSScreen *screen = qtPlatformScreenFor([notification object]);
    Q_ASSERT_X(screen, Q_FUNC_INFO, "Screen changed that we didn't know about");

    screen->updateProperties();
}

@end

// -------------------------------------------------------------------------

@interface QIOSOrientationListener : NSObject {
    @public
    QIOSScreen *m_screen;
}
- (id)initWithQIOSScreen:(QIOSScreen *)screen;
@end

@implementation QIOSOrientationListener

- (id)initWithQIOSScreen:(QIOSScreen *)screen
{
    self = [super init];
    if (self) {
        m_screen = screen;
#ifndef Q_OS_TVOS
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(orientationChanged:)
            name:@"UIDeviceOrientationDidChangeNotification" object:nil];
#endif
    }
    return self;
}

- (void)dealloc
{
#ifndef Q_OS_TVOS
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:@"UIDeviceOrientationDidChangeNotification" object:nil];
#endif
    [super dealloc];
}

- (void)orientationChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);
    m_screen->updateProperties();
}

@end

@interface UIScreen (Compatibility)
@property (nonatomic, readonly) CGRect qt_applicationFrame;
@end

@implementation UIScreen (Compatibility)
- (CGRect)qt_applicationFrame
{
#ifdef Q_OS_IOS
    return self.applicationFrame;
#else
    return self.bounds;
#endif
}
@end

// -------------------------------------------------------------------------

@implementation QUIWindow

- (id)initWithFrame:(CGRect)frame
{
    if ((self = [super initWithFrame:frame]))
        self->_sendingEvent = NO;

    return self;
}

- (void)sendEvent:(UIEvent *)event
{
    QScopedValueRollback<BOOL> sendingEvent(self->_sendingEvent, YES);
    [super sendEvent:event];
}

@end

// -------------------------------------------------------------------------

QT_BEGIN_NAMESPACE

/*!
    Returns the model identifier of the device.
*/
static QString deviceModelIdentifier()
{
#if TARGET_OS_SIMULATOR
    return QString::fromLocal8Bit(qgetenv("SIMULATOR_MODEL_IDENTIFIER"));
#else
    static const char key[] = "hw.machine";

    size_t size;
    sysctlbyname(key, NULL, &size, NULL, 0);

    char value[size];
    sysctlbyname(key, &value, &size, NULL, 0);

    return QString::fromLatin1(value);
#endif
}

QIOSScreen::QIOSScreen(UIScreen *screen)
    : QPlatformScreen()
    , m_uiScreen(screen)
    , m_uiWindow(0)
    , m_orientationListener(0)
{
    QString deviceIdentifier = deviceModelIdentifier();

    if (screen == [UIScreen mainScreen] && !deviceIdentifier.startsWith("AppleTV")) {
        // Based on https://en.wikipedia.org/wiki/List_of_iOS_devices#Display

        // iPhone (1st gen), 3G, 3GS, and iPod Touch (1st–3rd gen) are 18-bit devices
        static QRegularExpression lowBitDepthDevices("^(iPhone1,[12]|iPhone2,1|iPod[1-3],1)$");
        m_depth = deviceIdentifier.contains(lowBitDepthDevices) ? 18 : 24;

        static QRegularExpression iPhoneXModels("^iPhone(10,[36])$");
        static QRegularExpression iPhonePlusModels("^iPhone(7,1|8,2|9,[24]|10,[25])$");
        static QRegularExpression iPadMiniModels("^iPad(2,[567]|4,[4-9]|5,[12])$");

        if (deviceIdentifier.contains(iPhoneXModels)) {
            m_physicalDpi = 458;
        } else if (deviceIdentifier.contains(iPhonePlusModels)) {
            m_physicalDpi = 401;
        } else if (deviceIdentifier.startsWith("iPad")) {
            if (deviceIdentifier.contains(iPadMiniModels))
                m_physicalDpi = 163 * devicePixelRatio();
            else
                m_physicalDpi = 132 * devicePixelRatio();
        } else {
            // All normal iPhones, and iPods
            m_physicalDpi = 163 * devicePixelRatio();
        }
    } else {
        // External display, hard to say
        m_depth = 24;
        m_physicalDpi = 96;
    }

    if (!qt_apple_isApplicationExtension()) {
        for (UIWindow *existingWindow in qt_apple_sharedApplication().windows) {
            if (existingWindow.screen == m_uiScreen) {
                m_uiWindow = [m_uiWindow retain];
                break;
            }
        }

        if (!m_uiWindow) {
            // Create a window and associated view-controller that we can use
            m_uiWindow = [[QUIWindow alloc] initWithFrame:[m_uiScreen bounds]];
            m_uiWindow.rootViewController = [[[QIOSViewController alloc] initWithQIOSScreen:this] autorelease];
        }
    }

    updateProperties();

    m_displayLink = [m_uiScreen displayLinkWithBlock:^(CADisplayLink *) { deliverUpdateRequests(); }];
    m_displayLink.paused = YES; // Enabled when clients call QWindow::requestUpdate()
    [m_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
}

QIOSScreen::~QIOSScreen()
{
    [m_displayLink invalidate];

    [m_orientationListener release];
    [m_uiWindow release];
}

QString QIOSScreen::name() const
{
    if (m_uiScreen == [UIScreen mainScreen]) {
        return QString::fromNSString([UIDevice currentDevice].model)
            + QLatin1String(" built-in display");
    } else {
        return QLatin1String("External display");
    }
}

void QIOSScreen::updateProperties()
{
    QRect previousGeometry = m_geometry;
    QRect previousAvailableGeometry = m_availableGeometry;

    m_geometry = QRectF::fromCGRect(m_uiScreen.bounds).toRect();

    // The application frame doesn't take safe area insets into account, and
    // the safe area insets are not available before the UIWindow is shown,
    // and do not take split-view constraints into account, so we have to
    // combine the two to get the correct available geometry.
    QRect applicationFrame = QRectF::fromCGRect(m_uiScreen.qt_applicationFrame).toRect();
    UIEdgeInsets safeAreaInsets = m_uiWindow.qt_safeAreaInsets;
    m_availableGeometry = m_geometry.adjusted(safeAreaInsets.left, safeAreaInsets.top,
        -safeAreaInsets.right, -safeAreaInsets.bottom).intersected(applicationFrame);

#ifndef Q_OS_TVOS
    if (m_uiScreen == [UIScreen mainScreen]) {
        QIOSViewController *qtViewController = [m_uiWindow.rootViewController isKindOfClass:[QIOSViewController class]] ?
            static_cast<QIOSViewController *>(m_uiWindow.rootViewController) : nil;

        if (qtViewController.lockedOrientation) {
            Q_ASSERT(!qt_apple_isApplicationExtension());

            // Setting the statusbar orientation (content orientation) on will affect the screen geometry,
            // which is not what we want. We want to reflect the screen geometry based on the locked orientation,
            // and adjust the available geometry based on the repositioned status bar for the current status
            // bar orientation.

            Qt::ScreenOrientation statusBarOrientation = toQtScreenOrientation(
                UIDeviceOrientation(qt_apple_sharedApplication().statusBarOrientation));

            Qt::ScreenOrientation lockedOrientation = toQtScreenOrientation(UIDeviceOrientation(qtViewController.lockedOrientation));
            QTransform transform = transformBetween(lockedOrientation, statusBarOrientation, m_geometry).inverted();

            m_geometry = transform.mapRect(m_geometry);
            m_availableGeometry = transform.mapRect(m_availableGeometry);
        }
    }
#endif

    if (m_geometry != previousGeometry) {
        // We can't use the primaryOrientation of screen(), as we haven't reported the new geometry yet
        Qt::ScreenOrientation primaryOrientation = m_geometry.width() >= m_geometry.height() ?
            Qt::LandscapeOrientation : Qt::PortraitOrientation;

        // On iPhone 6+ devices, or when display zoom is enabled, the render buffer is scaled
        // before being output on the physical display. We have to take this into account when
        // computing the physical size. Note that unlike the native bounds, the physical size
        // follows the primary orientation of the screen.
        const QRectF physicalGeometry = mapBetween(nativeOrientation(), primaryOrientation, QRectF::fromCGRect(m_uiScreen.nativeBounds).toRect());

        static const qreal millimetersPerInch = 25.4;
        m_physicalSize = physicalGeometry.size() / m_physicalDpi * millimetersPerInch;
    }

    // At construction time, we don't yet have an associated QScreen, but we still want
    // to compute the properties above so they are ready for when the QScreen attaches.
    // Also, at destruction time the QScreen has already been torn down, so notifying
    // Qt about changes to the screen will cause asserts in the event delivery system.
    if (!screen())
        return;

    if (screen()->orientation() != orientation())
        QWindowSystemInterface::handleScreenOrientationChange(screen(), orientation());

    // Note: The screen orientation change and the geometry changes are not atomic, so when
    // the former is emitted, the latter has not been reported and reflected in the QScreen
    // API yet. But conceptually it makes sense that the orientation update happens first,
    // and the geometry updates caused by auto-rotation happen after that.

    if (m_geometry != previousGeometry || m_availableGeometry != previousAvailableGeometry)
        QWindowSystemInterface::handleScreenGeometryChange(screen(), m_geometry, m_availableGeometry);
}

void QIOSScreen::setUpdatesPaused(bool paused)
{
    m_displayLink.paused = paused;
}

void QIOSScreen::deliverUpdateRequests() const
{
    bool pauseUpdates = true;

    QList<QWindow*> windows = QGuiApplication::allWindows();
    for (int i = 0; i < windows.size(); ++i) {
        if (platformScreenForWindow(windows.at(i)) != this)
            continue;

        QWindowPrivate *wp = static_cast<QWindowPrivate *>(QObjectPrivate::get(windows.at(i)));
        if (!wp->updateRequestPending)
            continue;

        wp->deliverUpdateRequest();

        // Another update request was triggered, keep the display link running
        if (wp->updateRequestPending)
            pauseUpdates = false;
    }

    // Pause the display link if there are no pending update requests
    m_displayLink.paused = pauseUpdates;
}

QRect QIOSScreen::geometry() const
{
    return m_geometry;
}

QRect QIOSScreen::availableGeometry() const
{
    return m_availableGeometry;
}

int QIOSScreen::depth() const
{
    return m_depth;
}

QImage::Format QIOSScreen::format() const
{
    return QImage::Format_ARGB32_Premultiplied;
}

QSizeF QIOSScreen::physicalSize() const
{
    return m_physicalSize;
}

QDpi QIOSScreen::logicalDpi() const
{
    return QDpi(72, 72);
}

qreal QIOSScreen::devicePixelRatio() const
{
    return [m_uiScreen scale];
}

qreal QIOSScreen::refreshRate() const
{
#if QT_DARWIN_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_NA, 100300, 110000, __WATCHOS_NA)
    if (__builtin_available(iOS 10.3, tvOS 11, *))
        return m_uiScreen.maximumFramesPerSecond;
#endif

    return 60.0;
}

Qt::ScreenOrientation QIOSScreen::nativeOrientation() const
{
    CGRect nativeBounds =
#if defined(Q_OS_IOS)
        m_uiScreen.nativeBounds;
#else
        m_uiScreen.bounds;
#endif

    // All known iOS devices have a native orientation of portrait, but to
    // be on the safe side we compare the width and height of the bounds.
    return nativeBounds.size.width >= nativeBounds.size.height ?
        Qt::LandscapeOrientation : Qt::PortraitOrientation;
}

Qt::ScreenOrientation QIOSScreen::orientation() const
{
#ifdef Q_OS_TVOS
    return Qt::PrimaryOrientation;
#else
    // Auxiliary screens are always the same orientation as their primary orientation
    if (m_uiScreen != [UIScreen mainScreen])
        return Qt::PrimaryOrientation;

    UIDeviceOrientation deviceOrientation = [UIDevice currentDevice].orientation;

    // At startup, iOS will report an unknown orientation for the device, even
    // if we've asked it to begin generating device orientation notifications.
    // In this case we fall back to the status bar orientation, which reflects
    // the orientation the application was started up in (which may not match
    // the physical orientation of the device, but typically does unless the
    // application has been locked to a subset of the available orientations).
    if (deviceOrientation == UIDeviceOrientationUnknown && !qt_apple_isApplicationExtension())
        deviceOrientation = UIDeviceOrientation(qt_apple_sharedApplication().statusBarOrientation);

    // If the device reports face up or face down orientations, we can't map
    // them to Qt orientations, so we pretend we're in the same orientation
    // as before.
    if (deviceOrientation == UIDeviceOrientationFaceUp || deviceOrientation == UIDeviceOrientationFaceDown) {
        Q_ASSERT(screen());
        return screen()->orientation();
    }

    return toQtScreenOrientation(deviceOrientation);
#endif
}

void QIOSScreen::setOrientationUpdateMask(Qt::ScreenOrientations mask)
{
    if (m_orientationListener && mask == Qt::PrimaryOrientation) {
        [m_orientationListener release];
        m_orientationListener = 0;
    } else if (!m_orientationListener) {
        m_orientationListener = [[QIOSOrientationListener alloc] initWithQIOSScreen:this];
        updateProperties();
    }
}

QPixmap QIOSScreen::grabWindow(WId window, int x, int y, int width, int height) const
{
    if (window && ![reinterpret_cast<id>(window) isKindOfClass:[UIView class]])
        return QPixmap();

    UIView *view = window ? reinterpret_cast<UIView *>(window) : m_uiWindow;

    if (width < 0)
        width = qMax(view.bounds.size.width - x, CGFloat(0));
    if (height < 0)
        height = qMax(view.bounds.size.height - y, CGFloat(0));

    CGRect captureRect = [m_uiWindow convertRect:CGRectMake(x, y, width, height) fromView:view];
    captureRect = CGRectIntersection(captureRect, m_uiWindow.bounds);

    UIGraphicsBeginImageContextWithOptions(captureRect.size, NO, 0.0);
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextTranslateCTM(context, -captureRect.origin.x, -captureRect.origin.y);

    // Draws the complete view hierarchy of m_uiWindow into the given rect, which
    // needs to be the same aspect ratio as the m_uiWindow's size. Since we've
    // translated the graphics context, and are potentially drawing into a smaller
    // context than the full window, the resulting image will be a subsection of the
    // full screen.
    [m_uiWindow drawViewHierarchyInRect:m_uiWindow.bounds afterScreenUpdates:NO];

    UIImage *screenshot = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();

    return QPixmap::fromImage(qt_mac_toQImage(screenshot.CGImage));
}

UIScreen *QIOSScreen::uiScreen() const
{
    return m_uiScreen;
}

UIWindow *QIOSScreen::uiWindow() const
{
    return m_uiWindow;
}

#include "moc_qiosscreen.cpp"

QT_END_NAMESPACE
