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
#include "qiosapplicationdelegate.h"
#include "qiosviewcontroller.h"
#include "qiosscreen.h"

#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaApplication, "qt.qpa.application");
Q_LOGGING_CATEGORY(lcQpaInputMethods, "qt.qpa.input.methods");
Q_LOGGING_CATEGORY(lcQpaWindow, "qt.qpa.window");

bool isQtApplication()
{
    if (qt_apple_isApplicationExtension())
        return false;

    // Returns \c true if the plugin is in full control of the whole application. This means
    // that we control the application delegate and the top view controller, and can take
    // actions that impacts all parts of the application. The opposite means that we are
    // embedded inside a native iOS application, and should be more focused on playing along
    // with native UIControls, and less inclined to change structures that lies outside the
    // scope of our QWindows/UIViews.
    static bool isQt = ([qt_apple_sharedApplication().delegate isKindOfClass:[QIOSApplicationDelegate class]]);
    return isQt;
}

#ifndef Q_OS_TVOS
Qt::ScreenOrientation toQtScreenOrientation(UIDeviceOrientation uiDeviceOrientation)
{
    Qt::ScreenOrientation qtOrientation;
    switch (uiDeviceOrientation) {
    case UIDeviceOrientationPortraitUpsideDown:
        qtOrientation = Qt::InvertedPortraitOrientation;
        break;
    case UIDeviceOrientationLandscapeLeft:
        qtOrientation = Qt::LandscapeOrientation;
        break;
    case UIDeviceOrientationLandscapeRight:
        qtOrientation = Qt::InvertedLandscapeOrientation;
        break;
    case UIDeviceOrientationFaceUp:
    case UIDeviceOrientationFaceDown:
        qWarning("Falling back to Qt::PortraitOrientation for UIDeviceOrientationFaceUp/UIDeviceOrientationFaceDown");
        qtOrientation = Qt::PortraitOrientation;
        break;
    default:
        qtOrientation = Qt::PortraitOrientation;
        break;
    }
    return qtOrientation;
}

UIDeviceOrientation fromQtScreenOrientation(Qt::ScreenOrientation qtOrientation)
{
    UIDeviceOrientation uiOrientation;
    switch (qtOrientation) {
    case Qt::LandscapeOrientation:
        uiOrientation = UIDeviceOrientationLandscapeLeft;
        break;
    case Qt::InvertedLandscapeOrientation:
        uiOrientation = UIDeviceOrientationLandscapeRight;
        break;
    case Qt::InvertedPortraitOrientation:
        uiOrientation = UIDeviceOrientationPortraitUpsideDown;
        break;
    case Qt::PrimaryOrientation:
    case Qt::PortraitOrientation:
    default:
        uiOrientation = UIDeviceOrientationPortrait;
        break;
    }
    return uiOrientation;
}
#endif

int infoPlistValue(NSString* key, int defaultValue)
{
    static NSBundle *bundle = [NSBundle mainBundle];
    NSNumber* value = [bundle objectForInfoDictionaryKey:key];
    return value ? [value intValue] : defaultValue;
}

QT_END_NAMESPACE

// -------------------------------------------------------------------------

@interface QtFirstResponderEvent : UIEvent
@property (nonatomic, strong) id firstResponder;
@end

@implementation QtFirstResponderEvent
- (void)dealloc
{
    self.firstResponder = 0;
    [super dealloc];
}
@end


@implementation UIView (QtFirstResponder)
- (UIView*)qt_findFirstResponder
{
    if ([self isFirstResponder])
        return self;

    for (UIView *subview in self.subviews) {
        if (UIView *firstResponder = [subview qt_findFirstResponder])
            return firstResponder;
    }

    return nil;
}
@end

@implementation UIResponder (QtFirstResponder)

+ (id)currentFirstResponder
{
    if (qt_apple_isApplicationExtension()) {
        qWarning() << "can't get first responder in application extensions!";
        return nil;
    }

    QtFirstResponderEvent *event = [[[QtFirstResponderEvent alloc] init] autorelease];
    [qt_apple_sharedApplication() sendAction:@selector(qt_findFirstResponder:event:) to:nil from:nil forEvent:event];
    return event.firstResponder;
}

- (void)qt_findFirstResponder:(id)sender event:(QtFirstResponderEvent *)event
{
    Q_UNUSED(sender);

    if ([self isKindOfClass:[UIView class]])
        event.firstResponder = [static_cast<UIView *>(self) qt_findFirstResponder];
    else
        event.firstResponder = [self isFirstResponder] ? self : nil;
}
@end

QT_BEGIN_NAMESPACE

FirstResponderCandidate::FirstResponderCandidate(UIResponder *responder)
    : QScopedValueRollback<UIResponder *>(s_firstResponderCandidate, responder)
{
}

UIResponder *FirstResponderCandidate::s_firstResponderCandidate = 0;

QT_END_NAMESPACE

