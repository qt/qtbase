/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

#include <sys/sysctl.h>

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
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(orientationChanged:)
            name:@"UIDeviceOrientationDidChangeNotification" object:nil];
    }
    return self;
}

- (void)dealloc
{
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:@"UIDeviceOrientationDidChangeNotification" object:nil];
    [super dealloc];
}

- (void)orientationChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);
    m_screen->updateProperties();
}

@end

// -------------------------------------------------------------------------

/*!
    Returns the model identifier of the device.

    When running under the simulator, the identifier will not
    match the simulated device, but will be x86_64 or i386.
*/
static QString deviceModelIdentifier()
{
    static const char key[] = "hw.machine";

    size_t size;
    sysctlbyname(key, NULL, &size, NULL, 0);

    char value[size];
    sysctlbyname(key, &value, &size, NULL, 0);

    return QString::fromLatin1(value);
}

QIOSScreen::QIOSScreen(UIScreen *screen)
    : QPlatformScreen()
    , m_uiScreen(screen)
    , m_uiWindow(0)
    , m_orientationListener(0)
{
    if (screen == [UIScreen mainScreen]) {
        QString deviceIdentifier = deviceModelIdentifier();

        // Based on https://en.wikipedia.org/wiki/List_of_iOS_devices#Display

        // iPhone (1st gen), 3G, 3GS, and iPod Touch (1st–3rd gen) are 18-bit devices
        if (deviceIdentifier.contains(QRegularExpression("^(iPhone1,[12]|iPhone2,1|iPod[1-3],1)$")))
            m_depth = 18;
        else
            m_depth = 24;

        if (deviceIdentifier.contains(QRegularExpression("^iPhone(7,1|8,2)$"))) {
            // iPhone 6 Plus or iPhone 6S Plus
            m_pixelDensity = 401;
        } else if (deviceIdentifier.contains(QRegularExpression("^iPad(1,1|2,[1-4]|3,[1-6]|4,[1-3]|5,[3-4]|6,[7-8])$"))) {
            // All iPads except the iPad Mini series
            m_pixelDensity = 132 * devicePixelRatio();
        } else {
            // All non-Plus iPhones, and iPad Minis
            m_pixelDensity = 163 * devicePixelRatio();
        }
    } else {
        // External display, hard to say
        m_depth = 24;
        m_pixelDensity = 96;
    }

    for (UIWindow *existingWindow in [[UIApplication sharedApplication] windows]) {
        if (existingWindow.screen == m_uiScreen) {
            m_uiWindow = [m_uiWindow retain];
            break;
        }
    }

    if (!m_uiWindow) {
        // Create a window and associated view-controller that we can use
        m_uiWindow = [[UIWindow alloc] initWithFrame:[m_uiScreen bounds]];
        m_uiWindow.rootViewController = [[[QIOSViewController alloc] initWithQIOSScreen:this] autorelease];
    }

    updateProperties();
}

QIOSScreen::~QIOSScreen()
{
    [m_orientationListener release];
    [m_uiWindow release];
}

void QIOSScreen::updateProperties()
{
    QRect previousGeometry = m_geometry;
    QRect previousAvailableGeometry = m_availableGeometry;

    m_geometry = fromCGRect(m_uiScreen.bounds).toRect();
    m_availableGeometry = fromCGRect(m_uiScreen.applicationFrame).toRect();

    if (m_uiScreen == [UIScreen mainScreen]) {
        Qt::ScreenOrientation statusBarOrientation = toQtScreenOrientation(UIDeviceOrientation([UIApplication sharedApplication].statusBarOrientation));

        if (QSysInfo::MacintoshVersion < QSysInfo::MV_IOS_8_0) {
            // On iOS < 8.0 the UIScreen geometry is always in portait, and the system applies
            // the screen rotation to the root view-controller's view instead of directly to the
            // screen, like iOS 8 and above does.
            m_geometry = mapBetween(Qt::PortraitOrientation, statusBarOrientation, m_geometry);
            m_availableGeometry = transformBetween(Qt::PortraitOrientation, statusBarOrientation, m_geometry).mapRect(m_availableGeometry);
        }

        QIOSViewController *qtViewController = [m_uiWindow.rootViewController isKindOfClass:[QIOSViewController class]] ?
            static_cast<QIOSViewController *>(m_uiWindow.rootViewController) : nil;

        if (qtViewController.lockedOrientation) {
            // Setting the statusbar orientation (content orientation) on will affect the screen geometry,
            // which is not what we want. We want to reflect the screen geometry based on the locked orientation,
            // and adjust the available geometry based on the repositioned status bar for the current status
            // bar orientation.

            Qt::ScreenOrientation lockedOrientation = toQtScreenOrientation(UIDeviceOrientation(qtViewController.lockedOrientation));
            QTransform transform = transformBetween(lockedOrientation, statusBarOrientation, m_geometry).inverted();

            m_geometry = transform.mapRect(m_geometry);
            m_availableGeometry = transform.mapRect(m_availableGeometry);
        }
    }

    if (m_geometry != previousGeometry) {
        const qreal millimetersPerInch = 25.4;
        m_physicalSize = QSizeF(m_geometry.size() * devicePixelRatio()) / m_pixelDensity * millimetersPerInch;
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

Qt::ScreenOrientation QIOSScreen::nativeOrientation() const
{
    CGRect nativeBounds =
#if QT_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__IPHONE_8_0)
        QSysInfo::MacintoshVersion >= QSysInfo::MV_IOS_8_0 ? m_uiScreen.nativeBounds :
#endif
        m_uiScreen.bounds;

    // All known iOS devices have a native orientation of portrait, but to
    // be on the safe side we compare the width and height of the bounds.
    return nativeBounds.size.width >= nativeBounds.size.height ?
        Qt::LandscapeOrientation : Qt::PortraitOrientation;
}

Qt::ScreenOrientation QIOSScreen::orientation() const
{
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
    if (deviceOrientation == UIDeviceOrientationUnknown)
        deviceOrientation = UIDeviceOrientation([UIApplication sharedApplication].statusBarOrientation);

    // If the device reports face up or face down orientations, we can't map
    // them to Qt orientations, so we pretend we're in the same orientation
    // as before.
    if (deviceOrientation == UIDeviceOrientationFaceUp || deviceOrientation == UIDeviceOrientationFaceDown) {
        Q_ASSERT(screen());
        return screen()->orientation();
    }

    return toQtScreenOrientation(deviceOrientation);
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
