/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qiosglobal.h"
#include "qiosscreen.h"
#include "qioswindow.h"
#include <qpa/qwindowsysteminterface.h>
#include "qiosapplicationdelegate.h"
#include "qiosviewcontroller.h"

#include <sys/sysctl.h>

@interface QIOSOrientationListener : NSObject {
    @public
    QIOSScreen *m_screen;
}
- (id) initWithQIOSScreen:(QIOSScreen *)screen;
@end

@implementation QIOSOrientationListener

- (id) initWithQIOSScreen:(QIOSScreen *)screen
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

- (void) dealloc
{
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:@"UIDeviceOrientationDidChangeNotification" object:nil];
    [super dealloc];
}

- (void) orientationChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);

    UIDeviceOrientation deviceOrientation = [UIDevice currentDevice].orientation;
    switch (deviceOrientation) {
    case UIDeviceOrientationFaceUp:
    case UIDeviceOrientationFaceDown:
        // We ignore these events, as iOS will send events with the 'regular'
        // orientations alongside these two orientations.
        return;
    default:
        Qt::ScreenOrientation screenOrientation = toQtScreenOrientation(deviceOrientation);
        QWindowSystemInterface::handleScreenOrientationChange(m_screen->screen(), screenOrientation);
    }
}

@end

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

QIOSScreen::QIOSScreen(unsigned int screenIndex)
    : QPlatformScreen()
    , m_uiScreen([[UIScreen screens] objectAtIndex:qMin(screenIndex, [[UIScreen screens] count] - 1)])
    , m_orientationListener(0)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    QString deviceIdentifier = deviceModelIdentifier();

    if (deviceIdentifier == QStringLiteral("iPhone2,1") /* iPhone 3GS */
        || deviceIdentifier == QStringLiteral("iPod3,1") /* iPod touch 3G */) {
        m_depth = 18;
    } else {
        m_depth = 24;
    }

    int unscaledDpi = 163; // Regular iPhone DPI
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad
        && !deviceIdentifier.contains(QRegularExpression("^iPad2,[567]$")) /* excluding iPad Mini */) {
        unscaledDpi = 132;
    };

    CGRect bounds = [m_uiScreen bounds];
    m_geometry = QRect(bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height);

    CGRect frame = m_uiScreen.applicationFrame;
    m_availableGeometry = QRect(frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);

    const qreal millimetersPerInch = 25.4;
    m_physicalSize = QSizeF(m_geometry.size()) / unscaledDpi * millimetersPerInch;

    if (isQtApplication()) {
        // When in a non-mixed environment, let QScreen follow the current interface orientation:
        setPrimaryOrientation(toQtScreenOrientation(UIDeviceOrientation(qiosViewController().interfaceOrientation)));
    }

    [pool release];
}

QIOSScreen::~QIOSScreen()
{
    [m_orientationListener release];
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
    return Qt::PortraitOrientation;
}

Qt::ScreenOrientation QIOSScreen::orientation() const
{
    return toQtScreenOrientation([UIDevice currentDevice].orientation);
}

void QIOSScreen::setOrientationUpdateMask(Qt::ScreenOrientations mask)
{
    if (m_orientationListener && mask == Qt::PrimaryOrientation) {
        [m_orientationListener release];
        m_orientationListener = 0;
    } else if (!m_orientationListener) {
        m_orientationListener = [[QIOSOrientationListener alloc] initWithQIOSScreen:this];
    }
}

void QIOSScreen::setPrimaryOrientation(Qt::ScreenOrientation orientation)
{
    // Note that UIScreen never changes orientation, but QScreen should. To work around
    // this, we let QIOSViewController call us whenever interface orientation changes, and
    // use that as primary orientation. After all, the viewcontrollers geometry is what we
    // place QWindows on top of. A problem with this approach is that QIOSViewController is
    // not in use in a mixed environment, which results in no change to primary orientation.
    // We see that as acceptable since Qt should most likely not interfere with orientation
    // for that case anyway.
    bool portrait = screen()->isPortrait(orientation);
    if (portrait && m_geometry.width() < m_geometry.height())
        return;

    // Switching portrait/landscape means swapping width/height (and adjusting x/y):
    m_geometry = QRect(0, 0, m_geometry.height(), m_geometry.width());
    m_physicalSize = QSizeF(m_physicalSize.height(), m_physicalSize.width());
    m_availableGeometry = fromPortraitToPrimary(fromCGRect(m_uiScreen.applicationFrame), this);

    QWindowSystemInterface::handleScreenGeometryChange(screen(), m_geometry);
    QWindowSystemInterface::handleScreenAvailableGeometryChange(screen(), m_availableGeometry);
}

UIScreen *QIOSScreen::uiScreen() const
{
    return m_uiScreen;
}

QT_END_NAMESPACE
