/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qiosapplicationstate.h"

#include "qiosglobal.h"
#include "qiosintegration.h"

#include <qpa/qwindowsysteminterface.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qcore_mac_p.h>

#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

static void qRegisterApplicationStateNotifications()
{
    NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];
    NSOperationQueue *mainQueue = [NSOperationQueue mainQueue];

    // Map between notifications and corresponding application state. Note that
    // there's no separate notification for moving to UIApplicationStateInactive,
    // so we use UIApplicationWillResignActiveNotification as an intermediate.
    using NotificationMap = QMap<NSNotificationName, UIApplicationState>;
    static auto notifications = qt_apple_isApplicationExtension() ? NotificationMap{
        { NSExtensionHostWillEnterForegroundNotification, UIApplicationStateInactive },
        { NSExtensionHostDidBecomeActiveNotification, UIApplicationStateActive },
        { NSExtensionHostWillResignActiveNotification, UIApplicationStateInactive },
        { NSExtensionHostDidEnterBackgroundNotification, UIApplicationStateBackground },
    } : NotificationMap{
        { UIApplicationWillEnterForegroundNotification, UIApplicationStateInactive },
        { UIApplicationDidBecomeActiveNotification, UIApplicationStateActive },
        { UIApplicationWillResignActiveNotification, UIApplicationStateInactive },
        { UIApplicationDidEnterBackgroundNotification, UIApplicationStateBackground },
    };

    for (auto i = notifications.constBegin(); i != notifications.constEnd(); ++i) {
        [notificationCenter addObserverForName:i.key() object:nil queue:mainQueue
            usingBlock:^void(NSNotification *notification) {
                NSRange nameRange = NSMakeRange(2, notification.name.length - 14);
                QString reason = QString::fromNSString([notification.name substringWithRange:nameRange]);
                QIOSApplicationState::handleApplicationStateChanged(i.value(), reason);
        }];
    }

    if (qt_apple_isApplicationExtension()) {
        // Extensions are not allowed to access UIApplication, so we assume the state is active
        QIOSApplicationState::handleApplicationStateChanged(UIApplicationStateActive,
            QLatin1String("Extension loaded, assuming state is active"));
    } else {
        // Initialize correct startup state, which may not be the Qt default (inactive)
        UIApplicationState startupState = qt_apple_sharedApplication().applicationState;
        QIOSApplicationState::handleApplicationStateChanged(startupState, QLatin1String("Application loaded"));
    }
}
Q_CONSTRUCTOR_FUNCTION(qRegisterApplicationStateNotifications)

QIOSApplicationState::QIOSApplicationState()
{
    if (!qt_apple_isApplicationExtension()) {
        UIApplicationState startupState = qt_apple_sharedApplication().applicationState;
        QIOSApplicationState::handleApplicationStateChanged(startupState, QLatin1String("Application launched"));
    }
}

void QIOSApplicationState::handleApplicationStateChanged(UIApplicationState uiState, const QString &reason)
{
    Qt::ApplicationState oldState = QGuiApplication::applicationState();
    Qt::ApplicationState newState = toQtApplicationState(uiState);
    qCDebug(lcQpaApplication) << qPrintable(reason) << "- moving from" << oldState << "to" << newState;

    if (QIOSIntegration *integration = QIOSIntegration::instance()) {
        emit integration->applicationState.applicationStateWillChange(oldState, newState);
        QWindowSystemInterface::handleApplicationStateChanged(newState);
        emit integration->applicationState.applicationStateDidChange(oldState, newState);
        qCDebug(lcQpaApplication) << "done moving to" << newState;
    } else {
        qCDebug(lcQpaApplication) << "no platform integration yet, setting state directly";
        QGuiApplicationPrivate::applicationState = newState;
    }
}

Qt::ApplicationState QIOSApplicationState::toQtApplicationState(UIApplicationState state)
{
    switch (state) {
    case UIApplicationStateActive: return Qt::ApplicationActive;
    case UIApplicationStateInactive: return Qt::ApplicationInactive;
    case UIApplicationStateBackground: return Qt::ApplicationSuspended;
    }
}

#include "moc_qiosapplicationstate.cpp"

QT_END_NAMESPACE
