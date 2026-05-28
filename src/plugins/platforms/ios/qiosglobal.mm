// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiosglobal.h"
#include "qiosapplicationdelegate.h"
#include "qiosviewcontroller.h"
#include "qiosscreen.h"
#include "quiwindow.h"
#include "qioseventdispatcher.h"

#include <QtCore/private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaApplication, "qt.qpa.application");
Q_LOGGING_CATEGORY(lcQpaInputMethods, "qt.qpa.input.methods", QtCriticalMsg);
Q_LOGGING_CATEGORY(lcQpaWindow, "qt.qpa.window");
Q_LOGGING_CATEGORY(lcQpaWindowScene, "qt.qpa.window.scene");

bool isQtApplication()
{
    // Returns \c true if the plugin is in full control of the whole application. This means
    // that we control the application delegate and the top view controller, and can take
    // actions that impacts all parts of the application. The opposite means that we are
    // embedded inside a native iOS application, and should be more focused on playing along
    // with native UIControls, and less inclined to change structures that lies outside the
    // scope of our QWindows/UIViews.
    return QIOSEventDispatcher::isQtApplication();
}

bool isRunningOnVisionOS()
{
    static bool result = []{
        // This class is documented to only be available on visionOS
        return NSClassFromString(@"UIWindowSceneGeometryPreferencesVision");
    }();
    return result;
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

UIWindow *presentationWindow(QWindow *window)
{
    UIWindow *uiWindow = window ? reinterpret_cast<UIView *>(window->winId()).window : nullptr;
    if (!uiWindow) {
        auto *scenes = [qt_apple_sharedApplication().connectedScenes allObjects];
        if (scenes.count > 0) {
            auto *windowScene = static_cast<UIWindowScene*>(scenes[0]);
            uiWindow = windowScene.keyWindow;
            if (!uiWindow && windowScene.windows.count)
                uiWindow = windowScene.windows[0];
        }
    }
    return uiWindow;
}

UIView *rootViewForScreen(const QPlatformScreen *screen)
{
    Q_ASSERT(screen);

    const auto *iosScreen = static_cast<const QIOSScreen *>(screen);
    for (UIScene *scene in [qt_apple_sharedApplication().connectedScenes allObjects]) {
        if (![scene isKindOfClass:UIWindowScene.class])
            continue;

        auto *windowScene = static_cast<UIWindowScene*>(scene);

#if !defined(Q_OS_VISIONOS)
        if (windowScene.screen != iosScreen->uiScreen())
            continue;
#else
        Q_UNUSED(iosScreen);
#endif

        UIWindow *uiWindow = qt_objc_cast<QUIWindow*>(windowScene.keyWindow);
        if (!uiWindow) {
            for (UIWindow *win in windowScene.windows) {
                if (qt_objc_cast<QUIWindow*>(win)) {
                    uiWindow = win;
                    break;
                }
            }
        }

        return uiWindow.rootViewController.view;
    }

    return nullptr;
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

+ (id)qt_currentFirstResponder
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

UIResponder *FirstResponderCandidate::s_firstResponderCandidate = nullptr;

QT_END_NAMESPACE

