// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiosapplicationdelegate.h"

#include "qiosintegration.h"
#include "qiosservices.h"
#include "qiosviewcontroller.h"
#include "qioswindow.h"

#include <qpa/qplatformintegration.h>

#include <QtCore/QtCore>

@implementation QIOSApplicationDelegate

- (BOOL)application:(UIApplication *)application continueUserActivity:(NSUserActivity *)userActivity restorationHandler:(void (^)(NSArray<id<UIUserActivityRestoring>> *restorableObjects))restorationHandler
{
    Q_UNUSED(application);
    Q_UNUSED(restorationHandler);

    if (!QGuiApplication::instance())
        return NO;

    if ([userActivity.activityType isEqualToString:NSUserActivityTypeBrowsingWeb]) {
        QIOSIntegration *iosIntegration = QIOSIntegration::instance();
        Q_ASSERT(iosIntegration);

        QIOSServices *iosServices = static_cast<QIOSServices *>(iosIntegration->services());

        return iosServices->handleUrl(QUrl::fromNSURL(userActivity.webpageURL));
    }

    return NO;
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options
{
    Q_UNUSED(application);
    Q_UNUSED(options);

    if (!QGuiApplication::instance())
        return NO;

    QIOSIntegration *iosIntegration = QIOSIntegration::instance();
    Q_ASSERT(iosIntegration);

    QIOSServices *iosServices = static_cast<QIOSServices *>(iosIntegration->services());

    return iosServices->handleUrl(QUrl::fromNSURL(url));
}

@end

