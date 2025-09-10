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
@end

@implementation QIOSApplicationDelegate

- (UISceneConfiguration *)application:(UIApplication *)application
                          configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
                          options:(UISceneConnectionOptions *)options
{
    qCDebug(lcQpaWindowScene) << "Configuring scene for" << connectingSceneSession
        << "with options" << options;

    auto *sceneConfig = connectingSceneSession.configuration;
    sceneConfig.delegateClass = QIOSWindowSceneDelegate.class;
    return sceneConfig;
}

@end

@implementation QIOSWindowSceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions
{
    qCDebug(lcQpaWindowScene) << "Connecting" << scene << "to" << session;

    Q_ASSERT([scene isKindOfClass:UIWindowScene.class]);
    UIWindowScene *windowScene = static_cast<UIWindowScene*>(scene);

    QUIWindow *window = [[QUIWindow alloc] initWithWindowScene:windowScene];

    QIOSScreen *screen = [&]{
        for (auto *screen : qGuiApp->screens()) {
            auto *platformScreen = static_cast<QIOSScreen*>(screen->handle());
#if !defined(Q_OS_VISIONOS)
            if (platformScreen->uiScreen() == windowScene.screen)
#endif
                return platformScreen;
        }
        Q_UNREACHABLE();
    }();

    window.rootViewController = [[[QIOSViewController alloc]
        initWithWindow:window andScreen:screen] autorelease];

    if (connectionOptions.URLContexts.count > 0)
        [self scene:scene openURLContexts:connectionOptions.URLContexts];
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
    qCDebug(lcQpaWindowScene) << "Handling continueUserActivity for scene" << scene;

    if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb]) {
        QIOSIntegration *iosIntegration = QIOSIntegration::instance();
        Q_ASSERT(iosIntegration);

        QIOSServices *iosServices = static_cast<QIOSServices *>(iosIntegration->services());
        iosServices->handleUrl(QUrl::fromNSURL(userActivity.webpageURL));
    }
}

@end
