// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiosapplicationdelegate.h"

#include "qiosglobal.h"
#include "qiosintegration.h"
#include "qiosservices.h"
#include "qiosviewcontroller.h"
#include "qioswindow.h"
#include "qiosscreen.h"
#include "quiwindow.h"

#include <qpa/qplatformintegration.h>

#include <QtCore/QtCore>

@interface QIOSWindowSceneDelegate : NSObject<UIWindowSceneDelegate>
@property (nullable, nonatomic, strong) UIWindow *window;
@end

@implementation QIOSApplicationDelegate

- (UISceneConfiguration *)application:(UIApplication *)application
                          configurationForConnectingSceneSession:(UISceneSession *)session
                          options:(UISceneConnectionOptions *)options
{
    qCDebug(lcQpaWindowScene) << "Configuring scene for" << session << "with options" << options;

    auto *sceneConfig = session.configuration;

    if ([sceneConfig.role hasPrefix:@"CPTemplateApplication"]) {
        qCDebug(lcQpaWindowScene) << "Not touching CarPlay scene with role" << sceneConfig.role
                                  << "and existing delegate class" << sceneConfig.delegateClass;
        // FIXME: Consider ignoring any scene with an existing sceneClass, delegateClass, or
        // storyboard. But for visionOS the default delegate is SwiftUI.AppSceneDelegate.
    } else {
        sceneConfig.delegateClass = QIOSWindowSceneDelegate.class;
    }

    return sceneConfig;
}

@end

@implementation QIOSWindowSceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions
{
    qCDebug(lcQpaWindowScene) << "Connecting" << scene << "to" << session;

    // Handle connection options, even if we return early
    const auto handleConnectionOptions = qScopeGuard([&]{
        if (connectionOptions.URLContexts.count > 0)
            [self scene:scene openURLContexts:connectionOptions.URLContexts];
        // Handle universal link (https) application cold-launch case
        for (NSUserActivity *activity in connectionOptions.userActivities)
            [self scene:scene continueUserActivity:activity];
    });

#if defined(Q_OS_VISIONOS)
    // CPImmersiveScene is a UIWindowScene, most likely so it can handle its internal
    // CPSceneLayerEventWindow and UITextEffectsWindow, but we don't want a QUIWindow
    // for these scenes, so bail out early.
    if ([scene.session.role isEqualToString:@"CPSceneSessionRoleImmersiveSpaceApplication"]) {
        qCDebug(lcQpaWindowScene) << "Skipping UIWindow creation for immersive scene";
        return;
    }
#endif

    if (![scene isKindOfClass:UIWindowScene.class]) {
        qCWarning(lcQpaWindowScene) << "Unexpectedly encountered non-window scene";
        return;
    }

    UIWindowScene *windowScene = static_cast<UIWindowScene*>(scene);

    QUIWindow *window = [[QUIWindow alloc] initWithWindowScene:windowScene];
    window.rootViewController = [[[QIOSViewController alloc] initWithWindow:window] autorelease];

    self.window = [window autorelease];
}

- (void)windowScene:(UIWindowScene *)windowScene
        didUpdateCoordinateSpace:(id<UICoordinateSpace>)previousCoordinateSpace
        interfaceOrientation:(UIInterfaceOrientation)previousInterfaceOrientation
        traitCollection:(UITraitCollection *)previousTraitCollection
{
    qCDebug(lcQpaWindowScene) << "Scene" << windowScene << "did update properties";
    if (!self.window)
        return;

    Q_ASSERT([self.window isKindOfClass:QUIWindow.class]);
    auto *viewController = static_cast<QIOSViewController*>(self.window.rootViewController);
    [viewController updatePlatformScreen];
}

- (void)sceneDidDisconnect:(UIScene *)scene
{
    qCDebug(lcQpaWindowScene) << "Disconnecting" << scene;
    self.window = nil;
}

- (void)scene:(UIScene *)scene openURLContexts:(NSSet<UIOpenURLContext *> *)URLContexts
{
    qCDebug(lcQpaWindowScene) << "Handling openURLContexts for scene" << scene;

    QIOSIntegration *iosIntegration = QIOSIntegration::instance();
    Q_ASSERT(iosIntegration);

    QIOSServices *iosServices = static_cast<QIOSServices *>(iosIntegration->services());

    for (UIOpenURLContext *urlContext in URLContexts)
        iosServices->handleUrl(QUrl::fromNSURL(urlContext.URL));
}

- (void)scene:(UIScene *)scene continueUserActivity:(NSUserActivity *)userActivity
{
    qCDebug(lcQpaWindowScene) << "Handling user activity for scene" << scene;

    if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb]) {
        QIOSIntegration *iosIntegration = QIOSIntegration::instance();
        Q_ASSERT(iosIntegration);

        QIOSServices *iosServices = static_cast<QIOSServices *>(iosIntegration->services());
        iosServices->handleUrl(QUrl::fromNSURL(userActivity.webpageURL));
    }
}

@end
